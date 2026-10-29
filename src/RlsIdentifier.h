// ============================================================
// RlsIdentifier.h — AFFRLS (identification R0/R1/tau1)
// ============================================================
// Extrait de BatteryModels.h v1.6.11 (cf. FORK.md) — code INCHANGÉ.
// Aucune dépendance à BatteryModel : estimateur autonome.
// ============================================================

#pragma once

#include "BatteryModelsCore.h"

// ============================================================
// AFFRLS — identification en ligne des paramètres (v1.6.3)
// ============================================================
// Identifie R0, R1 et tau1 du modèle Thévenin 1 RC par moindres carrés
// récursifs à facteur d'oubli adaptatif (AFFRLS), cf. docs/batterie/
// algorithme.md §3. Forme ARX linéaire en les paramètres.
//
//   Modèle (convention projet, I>0 = charge) :
//     V_terminal = OCV + R0·I + V1        (V1 = état RC)
//     V1[k] = a1·V1[k−1] + b1·I[k−1]      (a1 = e^(−Δt/τ1), b1 = R1(1−a1))
//   ⇒ en éliminant V1 :  y[k] = a1·y[k−1] + R0·I[k] + (b1 − a1·R0)·I[k−1]
//     avec y[k] = Vt[k] − OCV[k]   (« tension utile »)
//
//   φ[k] = [ y[k−1],  I[k],  I[k−1] ]
//   θ[k] = [ a1,  R0,  b1 − a1·R0 ]
//
//   extraction : a1 = θ0 ; R0 = θ1 ; b1 = θ2 + a1·θ1 ; R1 = b1/(1−a1) ;
//                τ1 = −Δt/ln(a1)//
// ⚠️ 16/10/2026 — le régresseur précédent ([−I, −I_prev, y_prev] / θ=[R0,…])
// était STRUCTURELLEMENT faux (signes) : il ne pouvait pas identifier R0 avec
// la convention projet. Corrigé ci-dessus.
//
// Garde-fous : R0∈[r0_min,r0_max], τ1∈[tau_min,tau_max], P bornée,
// gel sans excitation (|ΔI| < seuil), rejet des dt non plausibles, isfinite.
// RAM : ~60 o (theta[3] + P[3][3] + scalaires). Aucune allocation.
// ============================================================
class RlsIdentifier {
public:
    void begin() {
        reset();
    }

    void reset() {
        // θ = [a1, R0, b1 − a1·R0] ; init plausible (τ≈20 s à dt=1, R0=0.5, R1=0.3)
        _theta[0] = 0.95f;                  // a1
        _theta[1] = 0.5f;                   // R0
        _theta[2] = 0.3f * (1.0f - 0.95f) - 0.95f * 0.5f;   // b1 − a1·R0
        // R-17 : τ recalculé depuis a1 (cohérence getTau1() après reset/begin).
        _tau = constrain(-_dtNominal / logf(_theta[0]), _tauMin, _tauMax);
        const float p0 = 1000.0f;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                _cov[i][j] = (i == j) ? p0 : 0.0f;
        _lambda = _lambdaMin;
        _yPrev = 0.0f;
        _iPrev = 0.0f;
        _hasPrev = false;
        _updates = 0;
        _exciteCountdown = 0;
        _r0Step = NAN;
        _valid = false;
        // v5.15 : ré-init ring (fenêtre longue) + seuil adaptatif
        for (uint16_t i = 0; i < _ringN; i++) _ringI[i] = 0;
        _ringCount = 0;
        _ringIdx = 0;
        _jitterEma = 0.0f;
    }

    void setBounds(float r0_min, float r0_max, float tau_min, float tau_max) {
        if (isfinite(r0_min) && r0_min > 0.0f) _r0Min = r0_min;
        if (isfinite(r0_max) && r0_max > _r0Min) _r0Max = r0_max;
        if (isfinite(tau_min) && tau_min > 0.0f) _tauMin = tau_min;
        if (isfinite(tau_max) && tau_max > _tauMin) _tauMax = tau_max;
    }
    // v1.6.8 : borne de R1 (1re branche RC). R1 n'était bornée NULLE PART : θ2
    // dérivait et getR1() pouvait valoir 1e6 Ω (observé en réel le 17-18/10/2026
    // sur le Coulométre, rls_upd>25 000). R1 > 0,5 Ω est non physique pour une
    // branche de polarisation (l'essentiel de la chute est déjà dans R0).
    void setR1Max(float r1_max) { if (isfinite(r1_max) && r1_max > 0.0f) _r1Max = r1_max; }
    float getR1Max() const { return _r1Max; }
    void setLambda(float lambda_min, float lambda_max, float kappa) {
        if (lambda_min > 0.90f && lambda_min <= 1.0f) _lambdaMin = lambda_min;
        if (lambda_max >= _lambdaMin && lambda_max <= 1.0f) _lambdaMax = lambda_max;
        if (kappa > 0.0f) _kappa = kappa;
    }
    void setExcitationThreshold(float dI_A) { if (dI_A > 0.0f) _dIThr = dI_A; }
    // v5.15 : seuils de la fenêtre LONGUE + borne haute du seuil adaptatif.
    void setLongWindowThreshold(float dI_A) { if (dI_A > 0.0f) _dIThrSlow = dI_A; }
    void setAdaptiveMax(float dI_A) { if (dI_A > 0.0f) _dIThrMax = dI_A; }
    // Observabilité : seuil effectif courant (adaptatif) — pour /api/status.
    float getEffectiveThreshold() const {
        float t = _dIThr;
        float a = 6.0f * _jitterEma;
        if (a > t) t = a;
        if (t > _dIThrMax) t = _dIThrMax;
        return t;
    }
    float getJitter() const { return _jitterEma; }
    uint16_t getExciteCountdown() const { return _exciteCountdown; }
    // Seuil du R0 direct ΔV/ΔI (AC-4.1 : |ΔI| > 0,4 A) — distinct du seuil AFFRLS.
    void setStepThreshold(float dI_A) { if (dI_A > 0.0f) _dIThrStep = dI_A; }
    // Pas de temps nominal (s) — utilisé pour recalculer τ = −Δt/ln(a1) après un
    // deserialize (τ n'est pas persisté). Doit refléter la cadence réelle d'appel.
    void setUpdatePeriod(float dt_s) { if (isfinite(dt_s) && dt_s > 0.0f) _dtNominal = dt_s; }

    // Mise à jour AFFRLS. Vt = tension terminale (V), I = courant (A, >0 charge),
    // ocv = OCV estimée au SoC courant (V), dt = pas (s).
    // Retourne true si les paramètres ont été mis à jour.
    bool update(float Vt, float I, float ocv, float dt) {
        if (!isfinite(Vt) || !isfinite(I) || !isfinite(ocv) || !isfinite(dt)) return false;
        if (dt <= _dtMin || dt > _dtMax) { _hasPrev = false; return false; }

        // R0 direct par ΔV/ΔI sur transition nette (AC-4.1 : |ΔI| > 0,4 A)
        if (_hasPrev && fabsf(I - _iPrev) >= _dIThrStep) {
            float dI = I - _iPrev;
            float dV = Vt - _VtPrev;
            float r = dV / dI;   // >0 = hausse de V avec I (charge) → R0 > 0 attendu
            if (isfinite(r) && r > _r0Min * 0.5f && r < _r0Max * 1.5f) _r0Step = r;
        }

        float y = Vt - ocv;

        // ── Excitation (v5.15) : DOUBLE critère + seuil ADAPTATIF ──
        //  1. COURT terme (1 pas) : front rapide |I[k]−I[k−1]| ≥ seuil ;
        //  2. LONG terme (fenêtre _ringN) : rampe |I[k]−I[k−N]| ≥ seuil — capte
        //     les variations LENTES (charge solaire étalée sur des heures) que le
        //     critère 1-pas ignore (ΔI/s trop faible → 0,0018 A/s chez le projet).
        //  Seuil ADAPTATIF : proportionnel au bruit de courant (jitter lissé),
        //  borné [_dIThr, _dIThrMax] → sensible quand le courant est propre,
        //  insensible quand il est bruité (évite les fausses excitations).
        // Bruit court terme (EMA de |ΔI|, contribution plafonnée pour qu'un
        // échelon ne gonfle pas artificiellement l'estimation).
        if (_hasPrev) {
            float d = fabsf(I - _iPrev);
            if (d > 0.2f) d = 0.2f;
            _jitterEma += 0.02f * (d - _jitterEma);   // τ ≈ 50 pas
        }
        float dIThrEff = _dIThr;
        {
            float adapt = 6.0f * _jitterEma;
            if (adapt > dIThrEff) dIThrEff = adapt;
            if (dIThrEff > _dIThrMax) dIThrEff = _dIThrMax;
        }
        // courant d'il y a _ringN pas (le plus ancien du ring)
        float iOld = (float)_ringI[_ringIdx] / 1000.0f;
        // Seuil fenêtre longue : au moins le seuil court/adaptatif.
        float dIThrLong = (_dIThrSlow > dIThrEff) ? _dIThrSlow : dIThrEff;
        bool longExc  = (_ringCount >= _ringN) && (fabsf(I - iOld) >= dIThrLong);
        bool shortExc = _hasPrev && (fabsf(I - _iPrev) >= dIThrEff);
        // mise à jour du ring (avant le test de gel)
        _ringI[_ringIdx] = (int16_t)constrain(I * 1000.0f, -32000.0f, 32000.0f);
        _ringIdx = (uint16_t)((_ringIdx + 1) % _ringN);
        if (_ringCount < _ringN) _ringCount++;

        // Échelon net (pour identifier τ sur la relaxation) → ouvre la fenêtre.
        if (_hasPrev && fabsf(I - _iPrev) >= _dIThrStep) _exciteCountdown = _exciteWindow;
        // Une excitation (courte OU longue) ouvre aussi la fenêtre d'identification.
        if (longExc || shortExc) _exciteCountdown = _exciteWindow;

        // Gel sans excitation : ni front court, ni rampe longue, ni fenêtre ouverte.
        if (!_hasPrev || (!longExc && !shortExc && _exciteCountdown == 0)) {
            _yPrev = y; _iPrev = I; _VtPrev = Vt; _hasPrev = true;
            if (_exciteCountdown > 0) _exciteCountdown--;
            return false;
        }
        if (_exciteCountdown > 0) _exciteCountdown--;

        float phi[3] = { _yPrev, I, _iPrev };

        // Gain K = P·φ / (λ + φᵀ·P·φ)
        float Pphi[3];
        for (int i = 0; i < 3; i++) {
            Pphi[i] = _cov[i][0] * phi[0] + _cov[i][1] * phi[1] + _cov[i][2] * phi[2];
        }
        float denom = _lambda + phi[0] * Pphi[0] + phi[1] * Pphi[1] + phi[2] * Pphi[2];
        if (!isfinite(denom) || fabsf(denom) < 1e-9f) return false;
        float K[3] = { Pphi[0] / denom, Pphi[1] / denom, Pphi[2] / denom };

        float err = y - (phi[0] * _theta[0] + phi[1] * _theta[1] + phi[2] * _theta[2]);
        if (!isfinite(err)) return false;

        for (int i = 0; i < 3; i++) _theta[i] += K[i] * err;

        // ⚠️ Borne R0 (θ1) IMMÉDIATEMENT : sinon, dans le cas τ hors bornes
        // ci-dessous (retour false sans _updates++), θ1 pouvait dériver vers des
        // valeurs non physiques (R0 négatif observé en réel, courant quasi constant).
        _theta[1] = constrain(_theta[1], _r0Min, _r0Max);

        // ⚠️ v1.6.8 — Borne R1 via θ2. b1 = θ2 + a1·θ1 et R1 = b1/(1−a1) :
        // sans borne sur θ2, R1 explosait (jusqu'à 1,2e6 Ω observé) et polluait
        // l'EKF (setParams(R0,R1,τ)) ainsi que l'affichage. On borne b1 pour que
        // R1 ∈ [0, _r1Max], puis on en déduit θ2.
        // ⚠️ v1.6.9 (F5) — la projection seule DÉPLAÇAIT la divergence : à courant
        // quasi constant, θ1 (R0) et θ2 sont quasi colinéaires, donc borner b1
        // faisait absorber tout l'écart par R0 qui dérivait vers _r0Max, et P non
        // corrigée provoquait un windup (λ→λmin, trace de P → reset() = perte
        // totale de l'apprentissage). Anti-windup : si la projection agit, on
        // réinitialise la sous-covariance (R0,R1) et on gèle l'oubli (λ = λmax).
        {
            float a1c = constrain(_theta[0], 1e-6f, 0.999999f);
            float b1 = _theta[2] + a1c * _theta[1];
            float b1max = _r1Max * (1.0f - a1c);
            float b1c = b1;
            if (!isfinite(b1c) || b1c < 0.0f) b1c = 0.0f;
            if (b1c > b1max) b1c = b1max;
            if (b1c != b1) {
                // Projection active → anti-windup sur le bloc (R0, R1).
                _cov[1][1] = 0.01f;
                _cov[2][2] = 0.01f;
                _cov[1][2] = _cov[2][1] = 0.0f;
                _lambda = _lambdaMax;
            }
            _theta[2] = b1c - a1c * _theta[1];
        }

        // P = (P − K·φᵀ·P) / λ
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                _cov[i][j] = (_cov[i][j] - K[i] * Pphi[j]) / _lambda;
            }
        }
        // Borne la trace de P (anti-explosion) + symétrie
        float tr = _cov[0][0] + _cov[1][1] + _cov[2][2];
        if (!isfinite(tr) || tr > _pTraceMax) { reset(); return false; }
        _cov[0][1] = _cov[1][0]; _cov[0][2] = _cov[2][0]; _cov[1][2] = _cov[2][1];

        // Facteur d'oubli adaptatif λ = λmin + (λmax−λmin)·exp(−κ·|e|)
        _lambda = _lambdaMin + (_lambdaMax - _lambdaMin) * expf(-_kappa * fabsf(err));
        if (_lambda < _lambdaMin) _lambda = _lambdaMin;
        if (_lambda > _lambdaMax) _lambda = _lambdaMax;

        // Extraction physique + bornes : a1 = θ0, R0 = θ1
        float a1 = _theta[0];
        if (a1 > 0.999999f) a1 = 0.999999f;
        if (a1 < 0.000001f) a1 = 0.000001f;
        // ⚠️ 16/10/2026 — borner a1 DANS _theta[0] AVANT tout retour : sinon, en
        // cas de τ hors bornes (return false ci-dessous), θ0 restait non borné
        // (a1 > 1 possible) → getR1() = b1/(1−a1) négatif/aberrant.
        _theta[0] = a1;
        float tau = -dt / logf(a1);
        if (!isfinite(tau) || tau < _tauMin || tau > _tauMax) {
            // τ hors bornes → on garde θ (a1 et R0 déjà bornés) mais on n'expose pas τ
            _yPrev = y; _iPrev = I; _VtPrev = Vt; _hasPrev = true;
            return false;
        }
        _theta[1] = constrain(_theta[1], _r0Min, _r0Max);
        _tau = tau;

        _yPrev = y; _iPrev = I; _VtPrev = Vt; _hasPrev = true;
        _updates++;
        if (_updates >= _minUpdates) _valid = true;
        return true;
    }

    // --- Observabilité ---
    // θ = [a1, R0, b1 − a1·R0] → a1 = θ0, R0 = θ1, b1 = θ2 + a1·θ1
    float getR0() const { return constrain(_theta[1], _r0Min, _r0Max); }
    float getTau1() const { return _tau; }
    /** R1 BRUTE (non bornée) — pour diagnostiquer la saturation (v1.6.9). */
    float getR1Raw() const {
        float a1 = _theta[0];
        if (!isfinite(a1) || fabsf(1.0f - a1) < 1e-6f) return NAN;
        float b1 = _theta[2] + a1 * _theta[1];
        return b1 / (1.0f - a1);
    }
    /** true si R1 est en butée (modèle non exploitable, v1.6.9). */
    bool isR1Saturated() const {
        float r = getR1Raw();
        return !isfinite(r) || r < 0.0f || r >= _r1Max * 0.999f;
    }
    /** true si R0 est collé à une borne (modèle non exploitable, v1.6.9). */
    bool isR0Saturated() const {
        return _theta[1] <= _r0Min * 1.001f || _theta[1] >= _r0Max * 0.999f;
    }
    /** true si τ est collé à une borne (modèle non exploitable, v1.6.9). */
    bool isTauSaturated() const {
        return !isfinite(_tau) || _tau <= _tauMin * 1.001f || _tau >= _tauMax * 0.999f;
    }
    float getR1() const {
        float a1 = _theta[0];
        if (fabsf(1.0f - a1) < 1e-6f) return 0.0f;
        float b1 = _theta[2] + a1 * _theta[1];
        float r1 = b1 / (1.0f - a1);
        // v1.6.8 : garde de sortie — R1 d'une branche de polarisation ne peut pas
        // dépasser _r1Max (0,5 Ω). Protège les consommateurs même si θ2 dérive.
        if (!isfinite(r1) || r1 < 0.0f) return 0.0f;
        return (r1 > _r1Max) ? _r1Max : r1;
    }
    float getR0Step() const { return _r0Step; }   // dernier ΔV/ΔI (Ω) ou NAN

    uint32_t getUpdates() const { return _updates; }
    // v1.6.9 (F3) : la v1.6.8 testait des valeurs DÉJÀ bornées (`_theta[1]` est
    // constrainé, `_tau` n'est assigné qu'in-bounds, `getR1()` renvoie déjà une
    // valeur ≤ _r1Max) ⇒ tous les tests étaient vrais par construction et
    // `isValid() ≡ _valid ≡ (_updates ≥ 5)`. Le garde-fou ne détectait donc
    // AUCUN des symptômes de l'incident terrain (R1 = 1,335 Ω masquée à 0,5,
    // τ = 59,9 s collé à la borne haute, R0 exactement sur _r0Min).
    // Désormais on détecte la SATURATION : un paramètre en butée signifie que le
    // modèle n'est pas identifié, donc inexploitable pour l'EKF et la correction OCV.
    bool isValid() const {
        if (!_valid) return false;
        if (!isfinite(_theta[0]) || _theta[0] <= 0.0f || _theta[0] >= 0.999999f) return false;
        if (!isfinite(_theta[1])) return false;
        if (isR0Saturated())  return false;
        if (isTauSaturated()) return false;
        if (isR1Saturated())  return false;
        return true;
    }
    float getLambda() const { return _lambda; }

    // --- Persistance (FRAM) ---
    // v5.15 : θ(3) + P symétrique(6) + n(4) + λ(4) + exciteCountdown(2) = 46 o.
    // (Avant : θ(3)+P plein(9)+n = 52 o, sans λ.) Persister λ conserve la
    // dynamique d'adaptation après reboot ; le ring de courants n'est pas
    // persisté (trop volumineux — reconstruit en 60 s).
    static constexpr uint16_t SER_LEN = 3*4 + 6*4 + 4 + 4 + 2;   // 46
    uint16_t serialize(uint8_t* buf, uint16_t cap) const {
        if (!buf || cap < SER_LEN) return 0;
        uint16_t o = 0;
        for (int i = 0; i < 3; i++) { memcpy(buf + o, &_theta[i], 4); o += 4; }
        const float ps[6] = { _cov[0][0], _cov[0][1], _cov[0][2],
                              _cov[1][1], _cov[1][2], _cov[2][2] };
        for (int i = 0; i < 6; i++) { memcpy(buf + o, &ps[i], 4); o += 4; }
        memcpy(buf + o, &_updates, 4); o += 4;
        memcpy(buf + o, &_lambda, 4); o += 4;
        memcpy(buf + o, &_exciteCountdown, 2); o += 2;
        return o;
    }
    bool deserialize(const uint8_t* buf, uint16_t len) {
        if (!buf || len < SER_LEN) return false;
        float th[3], ps[6], lam;
        uint32_t n;
        uint16_t ec;
        uint16_t o = 0;
        for (int i = 0; i < 3; i++) { memcpy(&th[i], buf + o, 4); o += 4; }
        for (int i = 0; i < 6; i++) { memcpy(&ps[i], buf + o, 4); o += 4; }
        memcpy(&n, buf + o, 4); o += 4;
        memcpy(&lam, buf + o, 4); o += 4;
        memcpy(&ec, buf + o, 2); o += 2;
        for (int i = 0; i < 3; i++) if (!isfinite(th[i])) return false;
        for (int i = 0; i < 6; i++) if (!isfinite(ps[i])) return false;
        if (!isfinite(lam)) return false;
        // v1.6.9 (F6) : bornes ALIGNÉES sur celles de la mise à jour. Avant, on
        // acceptait θ1 ∈ [0.5·_r0Min, 1.5·_r0Max] = [0.025, 3.0] Ω puis `isValid()`
        // exigeait [0.05, 2.0] → un état FRAM dans ]2.0, 3.0] était restauré
        // « valide » puis jugé invalide, désactivant silencieusement la correction
        // tension jusqu'au prochain pas réussi.
        if (th[1] < _r0Min || th[1] > _r0Max) return false;
        for (int i = 0; i < 3; i++) _theta[i] = th[i];
        _cov[0][0] = ps[0]; _cov[0][1] = ps[1]; _cov[1][0] = ps[1];
        _cov[0][2] = ps[2]; _cov[2][0] = ps[2];
        _cov[1][1] = ps[3];
        _cov[1][2] = ps[4]; _cov[2][1] = ps[4];
        _cov[2][2] = ps[5];
        _updates = n;
        _valid = (n >= _minUpdates);
        _lambda = constrain(lam, _lambdaMin, _lambdaMax);
        _exciteCountdown = (ec <= _exciteWindow) ? ec : 0;
        _hasPrev = false;   // regressor précédent non restauré (sûr : 1 pas perdu)

        // ⚠️ _tau n'est pas persisté : il EST recalculé depuis θ[0]=a1
        // (τ = −Δt/ln a1) avec le pas nominal de l'AFFRLS projet (1 s).
        _theta[0] = constrain(_theta[0], 0.000001f, 0.999999f);
        float tau = -_dtNominal / logf(_theta[0]);
        _tau = (isfinite(tau) && tau >= _tauMin && tau <= _tauMax) ? tau
                                                                   : constrain(20.0f, _tauMin, _tauMax);
        // États non persistés (ring + jitter) : reconstruits en _ringN pas.
        _ringCount = 0;
        _ringIdx = 0;
        _jitterEma = 0.0f;
        _r0Step = NAN;

        // v1.6.9 (F6) : `_r1Max` n'étant pas persisté, un θ2 restauré peut dépasser
        // la borne courante → on re-projette immédiatement (sinon R1 resterait
        // hors bornes jusqu'au prochain pas, et `isValid()` dépendrait de l'ordre).
        {
            float a1c = constrain(_theta[0], 1e-6f, 0.999999f);
            float b1 = _theta[2] + a1c * _theta[1];
            float b1max = _r1Max * (1.0f - a1c);
            if (!isfinite(b1) || b1 < 0.0f) b1 = 0.0f;
            if (b1 > b1max) b1 = b1max;
            _theta[2] = b1 - a1c * _theta[1];
        }
        return true;
    }

private:
    float    _theta[3]   = {0.5f, 0.0f, 0.95f};
    float    _cov[3][3]    = {};
    float    _lambda     = 0.98f;
    float    _lambdaMin  = 0.95f;
    float    _lambdaMax  = 0.999f;
    float    _kappa      = 5.0f;
    float    _r0Min      = 0.05f;    // 50 mΩ
    float    _r0Max      = 2.0f;
    float    _r1Max      = 0.5f;     // 500 mΩ — borne de la 1re branche RC (v1.6.8)
    float    _tauMin     = 2.0f;
    float    _tauMax     = 60.0f;
    float    _dIThr      = 0.15f;    // seuil d'excitation MINI AFFRLS (A) — plancher du seuil adaptatif
    float    _dIThrStep  = 0.4f;     // seuil R0 direct ΔV/ΔI (AC-4.1)
    uint16_t _exciteWindow   = 600;  // fenêtre (pas) après un échelon (identifie τ) — v5.15 : 180→600
    uint16_t _exciteCountdown = 0;
    // ── v5.15 : excitation sur FENÊTRE LONGUE + seuil ADAPTATIF ──
    // Ring des courants passés (mA) → détecte les RAMPEs lentes (charge solaire
    // étalée) que le critère 1-pas (ΔI/s) ignore. 60 pas ≈ 60 s à 1 Hz.
    static const uint16_t _ringN = 60;
    int16_t  _ringI[_ringN] = {};    // courants (mA)
    uint16_t _ringCount = 0;         // nb d'échantillons accumulés
    uint16_t _ringIdx   = 0;         // index d'écriture (point le plus ancien)
    float    _dIThrSlow = 0.10f;     // seuil fenêtre longue (A)
    float    _dIThrMax  = 0.30f;     // borne HAUTE du seuil adaptatif (A)
    float    _jitterEma = 0.0f;      // |ΔI| court terme lissé (bruit) → seuil adaptatif
    float    _dtMin      = 0.05f;
    float    _dtMax      = 5.0f;
    float    _dtNominal  = 1.0f;     // pas nominal (s) — recalcul de τ au deserialize
    float    _tau        = 20.0f;    // τ courant (s)
    float    _pTraceMax  = 1e7f;
    float    _yPrev      = 0.0f;
    float    _iPrev      = 0.0f;
    float    _VtPrev     = 0.0f;
    float    _r0Step     = NAN;
    bool     _hasPrev    = false;
    bool     _valid      = false;
    uint32_t _updates    = 0;
    uint32_t _minUpdates = 5;
};
