// ============================================================
// SocEkf.h — EKF d'état [SoC, V1, V2]
// ============================================================
// Extrait de BatteryModels.h v1.6.11 (cf. FORK.md) — code INCHANGÉ.
// Dépend de BatteryModel (ocvToSoc) mais ne le modifie pas.
// ============================================================

#pragma once

#include "BatteryModelsCore.h"
#include "BatteryModel.h"

// ============================================================
// EKF d'état — estimation du SoC (phase 6, v1.6.6)
// ============================================================
// Filtre de Kalman étendu d'ordre 3 sur x = [SoC, V1, V2] (SoC en fraction 0..1,
// V1/V2 = tensions des 2 branches RC, V batterie). Prédiction par le courant
// (comptage coulomb) + correction par la tension terminale (modèle Thévenin).
//
//   Prédiction : s ← s + I·Δt/(3600·C) ;  Vk ← e^(−Δt/τk)·Vk + Rk·(1−e^(−Δt/τk))·I
//   Observation: Vt = V_oc(s,T) + I·R0 + V1 + V2
//   H = [∂V_oc/∂s , +1 , +1]   (dérivée numérique de V_oc)
//
// ⚠️ CONVENTION PROJET : I > 0 = CHARGE (INA226 + projet) ⇒ V_terminal = OCV + I·R0
//    + V1 + V2. Le rendement faradique η n'est PAS modélisé ici (η = 1) : le
//    comptage coulomb du projet applique déjà ses propres rendements (Peukert/η).
//
// Le gain de Kalman K = P·Hᵀ·(H·P·Hᵀ + R)⁻¹ réalise une fusion
// **pondérée par la confiance** (P faible / R élevé → correction douce) — AC-6.2.
// `getConfidence()` reflète la qualité (0..1).
//
// RAM : P 3×3 + Q/R diagonaux + x = ~60 o. Aucune allocation.
// ⚠️ Le résultat (SoC_EKF) est exposé SÉPARÉMENT : « le coulomb est la
// référence » reste vrai ; l'EKF sert d'observateur/correcteur borné.
// ============================================================
class SocEkf {
public:
    void begin() { reset(); }

    void reset() {
        _state[0] = 0.5f; _state[1] = 0.0f; _state[2] = 0.0f;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                _cov[i][j] = (i == j) ? (i == 0 ? 0.01f : 0.001f) : 0.0f;
        _qNoise[0] = 1e-11f; _qNoise[1] = 1e-6f; _qNoise[2] = 1e-6f;   // Q_soc faible : « le coulomb est la référence »
        _rNoise = 0.01f;
        _init = false;
        _confidence = 0.0f;
        _innov = 0.0f;
        _updates = 0;
    }

    void setInitialSoC(float soc_pct) { _state[0] = constrain(soc_pct / 100.0f, 0.0f, 1.0f); _init = true; }
    void setParams(float r0_ohm, float r1_ohm, float tau1_s, float r2_ohm, float tau2_s) {
        _R0 = (isfinite(r0_ohm) && r0_ohm > 0.0f) ? constrain(r0_ohm, 0.005f, 2.0f) : 0.01f;
        // ⚠️ Borne R1 : un R1 aberrant (AFFRLS non convergé → ~9 Ω) rend le modèle
        // incohérent et fait s'effondrer le SoC de l'EKF (observé en réel, v5.9).
        _R1 = (isfinite(r1_ohm) && r1_ohm > 0.0f) ? constrain(r1_ohm, 0.0f, 2.0f) : 0.0f;
        _tau1 = (isfinite(tau1_s) && tau1_s > 0.0f) ? constrain(tau1_s, 2.0f, 600.0f) : 20.0f;
        _R2 = (isfinite(r2_ohm) && r2_ohm > 0.0f) ? constrain(r2_ohm, 0.0f, 2.0f) : 0.0f;
        _tau2 = (isfinite(tau2_s) && tau2_s > 0.0f) ? constrain(tau2_s, 2.0f, 3600.0f) : 300.0f;
    }
    void setNoise(float q_soc, float q_v, float r_meas) {
        if (q_soc > 0.0f) _qNoise[0] = q_soc;
        if (q_v > 0.0f) { _qNoise[1] = q_v; _qNoise[2] = q_v; }
        if (r_meas > 0.0f) _rNoise = r_meas;
    }
    void setCapacity(float c_ah) { if (isfinite(c_ah) && c_ah > 0.5f) _capAh = c_ah; }
    /** @brief Capacité utilisée par la prédiction (Ah) — v5.39 (A1), diagnostic. */
    float getCapacity() const { return _capAh; }
    /**
     * @brief Rendement faradique appliqué à l'intégration du SoC (v1.6.9, F7)
     *
     * Le compteur `Coulomb` intègre `I·η` ; l'EKF intégrait `I` brut → les deux
     * SoC divergeaient d'un facteur 1/η en charge (1,9× à 5 A). Le fail-safe
     * « |SoC_EKF − SoC_coulomb| > 40 % » se déclenchait alors sur un écart de
     * MODÈLE et re-seedait en boucle. On applique donc le même η à la prédiction
     * de SoC (la chute ohmique `I·R0` continue d'utiliser le courant RÉEL).
     */
    void setEfficiency(float eta_charge, float eta_discharge) {
        if (isfinite(eta_charge) && eta_charge > 0.0f && eta_charge <= 1.0f)
            _etaCharge = eta_charge;
        if (isfinite(eta_discharge) && eta_discharge > 0.0f && eta_discharge <= 1.0f)
            _etaDischarge = eta_discharge;
    }
    // Sous charge (|I| > 0.2 A), si R0 est incertain, on gonfle le bruit de mesure
    // → l'EKF fait davantage confiance au coulomb (principe « coulomb = référence »).
    void setLoadRFactor(float f) { if (f >= 1.0f) _loadRFactor = f; }
    // Échelle GLOBALE du bruit de mesure. s grand ⇒ correction tension quasi nulle
    // (EKF ≈ coulomb). À utiliser quand les paramètres Thévenin (R0/R1/τ) ne sont
    // PAS identifiés : sans eux, la correction tension biaise le SoC (constaté en réel).
    void setRScale(float s) { if (s >= 1.0f) _rScale = s; }
    // Correction tension ON/OFF. false ⇒ EKF = coulomb pur (prédiction seule),
    // jusqu'à ce que les paramètres Thévenin soient identifiés (AFFRLS valide).
    void setVoltageCorrection(bool en) { _useCorrection = en; }
    bool isVoltageCorrectionOn() const { return _useCorrection; }

    // Mise à jour. vt = tension terminale (V), I = courant (A, >0 charge),
    // temp = °C, dt = pas (s). model = BatteryModel (pour V_oc). Retourne true si OK.
    bool update(BatteryModel* model, float vt, float I, float temp, float dt) {
        if (!model || !_init) return false;
        if (!isfinite(vt) || !isfinite(I) || !isfinite(dt) || dt <= 0.0f || dt > 5.0f) return false;

        const float n = (float)model->getCellCount();
        if (n < 1.0f) return false;

        // --- PRÉDICTION ---
        // Convention projet : I > 0 = CHARGE → le SoC AUGMENTE.
        // v1.6.9 (F7) : le même rendement η que `Coulomb` est appliqué à
        // l'intégration du SoC, sinon les deux estimateurs divergent de 1/η.
        float eta = (I > 0.0f) ? _etaCharge : (I < 0.0f ? _etaDischarge : 1.0f);
        float dsoc = (I * eta * dt / 3600.0f) / _capAh;     // fraction
        _state[0] = constrain(_state[0] + dsoc, 0.0f, 1.0f);
        float a1 = expf(-dt / _tau1), a2 = expf(-dt / _tau2);
        // Bornage des états RC aussi en PRÉDICTION (R-15) : la branche correction
        // (plus bas) est court-circuitée quand setVoltageCorrection(false).
        _state[1] = constrain(a1 * _state[1] + _R1 * (1.0f - a1) * I, -5.0f, 5.0f);
        _state[2] = constrain(a2 * _state[2] + _R2 * (1.0f - a2) * I, -5.0f, 5.0f);

        // P = A·P·Aᵀ + Q  (A diagonale ⇒ (A·P·Aᵀ)[i][j] = A[i]·A[j]·P[i][j])
        float A[3] = {1.0f, a1, a2};
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                _cov[i][j] = A[i] * A[j] * _cov[i][j] + ((i == j) ? _qNoise[i] : 0.0f);

        // --- OBSERVATION ---
        // Correction désactivée (paramètres Thévenin non identifiés) : l'EKF
        // n'intègre que le coulomb (prédiction seule) → pas de biais de SoC.
        if (!_useCorrection) { _confidence = 0.0f; _updates++; return true; }

        // Physique : V_terminal = OCV + I·R0 + V1 + V2  (I>0 charge ⇒ V > OCV).
        float soc_pct = _state[0] * 100.0f;
        float voc = model->socToOcv(soc_pct, temp);      // V batterie
        float vt_pred = voc + I * _R0 + _state[1] + _state[2];
        float innov = vt - vt_pred;

        // H = [dV_oc/ds (V par fraction), +1, +1] — dérivée numérique
        float e = 0.01f;                                  // 1 % de SoC
        float s2 = constrain(_state[0] + e, 0.0f, 1.0f);
        float s1 = constrain(_state[0] - e, 0.0f, 1.0f);
        float dvocs = (model->socToOcv(s2 * 100.0f, temp) - model->socToOcv(s1 * 100.0f, temp))
                      / (s2 - s1);
        float H[3] = {dvocs, 1.0f, 1.0f};

        // S = H·P·Hᵀ + R
        float PH[3];
        for (int i = 0; i < 3; i++)
            PH[i] = _cov[i][0] * H[0] + _cov[i][1] * H[1] + _cov[i][2] * H[2];
        float r_eff = _rNoise * _rScale;
        if (_loadRFactor > 1.0f && fabsf(I) > 0.2f) r_eff *= _loadRFactor;
        float S = H[0] * PH[0] + H[1] * PH[1] + H[2] * PH[2] + r_eff;
        if (!isfinite(S) || fabsf(S) < 1e-9f) return false;

        float K[3] = { PH[0] / S, PH[1] / S, PH[2] / S };

        _state[0] = constrain(_state[0] + K[0] * innov, 0.0f, 1.0f);
        // ⚠️ 16/10/2026 — borner les états RC (|Vk| ≤ 5 V) : sans borne, une
        // innovation forte (paramètres Thévenin incohérents) faisait diverger
        // V1/V2 (seule la diagonale de P était contrôlée, pas leur amplitude).
        _state[1] = constrain(_state[1] + K[1] * innov, -5.0f, 5.0f);
        _state[2] = constrain(_state[2] + K[2] * innov, -5.0f, 5.0f);
        if (!isfinite(_state[1]) || !isfinite(_state[2])) { reset(); return false; }

        // P = (I − K·H)·P
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                _cov[i][j] -= K[i] * PH[j];
        // Symétrie + garde
        _cov[0][1] = _cov[1][0]; _cov[0][2] = _cov[2][0]; _cov[1][2] = _cov[2][1];
        for (int i = 0; i < 3; i++) {
            if (!isfinite(_cov[i][i]) || _cov[i][i] < 0.0f) { reset(); return false; }
        }

        _innov = innov;
        _updates++;
        // Confiance : 1 − normalisation de l'innovation (σ ≈ sqrt(S))
        float sig = sqrtf(S);
        float z = (sig > 1e-6f) ? fabsf(innov) / sig : 0.0f;
        _confidence = 1.0f / (1.0f + z);     // [0,1], 1 = innovation faible
        return true;
    }

    float getSoC() const { return _state[0] * 100.0f; }         // %
    float getConfidence() const { return _confidence; }     // 0..1
    float getInnovation() const { return _innov; }          // V
    float getV1() const { return _state[1]; }
    float getV2() const { return _state[2]; }
    bool isInit() const { return _init; }
    uint32_t getUpdates() const { return _updates; }

private:
    float    _state[3]  = {0.5f, 0.0f, 0.0f};
    float    _cov[3][3] = {};
    float    _qNoise[3]  = {1e-11f, 1e-6f, 1e-6f};
    float    _rNoise     = 0.01f;
    float    _loadRFactor = 1.0f;
    float    _rScale = 1.0f;
    bool     _useCorrection = true;
    float    _R0 = 0.01f, _R1 = 0.0f, _tau1 = 20.0f, _R2 = 0.0f, _tau2 = 300.0f;
    float    _capAh     = 26.0f;
    float    _etaCharge = 1.0f;      // v1.6.9 (F7) : rendement appliqué à dSOC/dt
    float    _etaDischarge = 1.0f;
    bool     _init  = false;
    float    _confidence = 0.0f;
    float    _innov = 0.0f;
    uint32_t _updates = 0;
};
