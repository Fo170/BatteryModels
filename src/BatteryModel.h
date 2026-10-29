// ============================================================
// BatteryModel.h — Modèle physique de batterie (socle)
// ============================================================
// Classe BatteryModel : OCV, résistance interne, branches RC, hystérésis,
// gazage, Peukert, détection d'état de charge.
//
// v2.0.0 — extrait de BatteryModels.h (cf. FORK.md) :
//   • getParams() utilise tech/TechRegistry.h (switch nominatif) au lieu de
//     l'indexation TECH_PARAMS[idx] ;
//   • getRpol0_mOhm() et getDeltaPol() exposés (le projet recopiait ces
//     constantes en dur faute d'accesseur).
// ============================================================

#pragma once

#include "BatteryModelsCore.h"
#include "tech/TechRegistry.h"

// ============================================================
// CLASSE MODELE BATTERIE
// ============================================================
class BatteryModel {
private:
    BatteryTech tech;
    uint8_t n_cells;
    float C_nominal;
    float R_int_nominal;
    bool auto_detect;

    // VOLATILE: persistance geree par le programme utilisateur
    volatile float V_pol_state = 0.0f;
    volatile float last_current = 0.0f;
    volatile float accumulated_charge_Ah = 0.0f;
    volatile uint32_t cycle_count = 0;
    volatile float last_charge_delta_Ah = 0.0f;

    // Buffer pour lecture PROGMEM
    mutable TechParams _paramsCache;
    mutable bool _paramsCached = false;

    // ===== NOUVEAU v1.4.0: overrides runtime (sentinelle NAN = "non surchargé") =====
    mutable DetectionConfig _detectOverride = overrideDetectionNone();
    mutable KalmanTuning    _kalmanOverride = overrideKalmanNone();
    mutable bool            _ocvSegmentAllowedSet = false;  // tri-state pour le bool ocv_segment_allowed
    mutable DetectionConfig _detectResolved;
    mutable KalmanTuning    _kalmanResolved;
    mutable bool            _configDirty = true;

    // ===== NOUVEAU v1.6.1: override runtime des seuils de tension (V/cell) =====
    // Sentinelle NAN = "conserver le défaut de la technologie". Permet d'aligner
    // la détection FLOAT/ABSORPTION sur les tensions réelles du chargeur (ex.
    // Victron float 13.8 V ⇒ 2.30 V/cell au lieu du défaut plomb 2.37 V/cell).
    mutable float _vFloatPerCellOverride = NAN;
    mutable float _vAbsPerCellOverride   = NAN;

    // ===== NOUVEAU v1.6.2: override runtime des coefficients OCV (CUBIQUE) =====
    // V_cell(s) = a0 + a1·s + a2·s² + a3·s³, avec s = SOC/100, V en volts PAR
    // CELLULE. Permet d'ajuster la courbe OCV sur la batterie réelle sans toucher
    // aux tables par technologie. Activé par setOcvPolynomial(...), désactivé par
    // clearOcvPolynomial(). N'affecte QUE les technos à polynôme (plomb, NiFe…) —
    // les technologies à TABLE (Li, Na…) gardent leur table.
    mutable bool  _ocvPolySet = false;
    mutable float _ocvA0 = 0.0f, _ocvA1 = 0.0f, _ocvA2 = 0.0f, _ocvA3 = 0.0f;

    // ===== NOUVEAU v1.6.2: hystérésis charge/décharge (one-state, Plett) =====
    // h ∈ [−h_max, +h_max] (V/cell). h converge vers +h_max en charge et −h_max
    // en décharge, avec la constante de temps 1/gamma (s). h_max = 0 → désactivé.
    // Modèle : V_terminal = OCV(SOC) + h + I·R + V_pol ⇒ la conversion
    // V_terminal → OCV RETRANCHE h pour rendre la lecture SoC indépendante du sens.
    mutable float _hystState = 0.0f;
    mutable float _hystMax   = 0.0f;   // V/cell
    mutable float _hystGamma = 0.0f;   // 1/s (0 = gelé)

    // ===== NOUVEAU v1.6.4: 2ᵉ branche RC (diffusion lente) + gassing =====
    // 2ᵉ RC : R2 (mΩ/cell) et τ2 (s). 0 = branche désactivée (1 RC seule).
    mutable float _r2mOhm   = 0.0f;
    mutable float _tau2     = 0.0f;
    mutable float _v2State  = 0.0f;    // V/cell (état de la 2ᵉ branche)
    // Gassing (branche parasite en fin de charge) : surtension qui apparaît
    // quand V_cell dépasse le seuil de dégagement gazeux (v_gas), proportionnelle
    // au dépassement (k_gas). 0 = désactivé. Rend le palier de floating (~14,9 V).
    mutable float _vGasCell = 0.0f;    // V/cell (seuil, 0 = off)
    mutable float _kGas     = 0.0f;    // surtension par volt de dépassement
    mutable float _gasState = 0.0f;    // V/cell (état lissé)

    // ===== v2.1.0 (F-02) : polarisation IDENTIFIÉE (AFFRLS) =====
    // `correctVoltageToOCV` retirait une polarisation Rpol issue d'une CONSTANTE
    // techno (Rpol0 = 4,32 mΩ/cell, dépendante du SoC mais figée), alors que
    // `SocEkf` soustrayait la branche R1/τ1 RÉELLEMENT identifiée par l'AFFRLS.
    // Deux modèles concurrents pour la même grandeur ⇒ l'ancre OCV et l'EKF
    // prédisaient des tensions différentes pour le même état : les deux SoC ne
    // pouvaient pas converger (mesuré : v2rc_bm = 0,0267 V vs v2_ekf = 0,1604 V).
    // Quand l'AFFRLS est valide, le projet pousse R1/τ1 ici et les deux
    // consommateurs partagent la MÊME paramétrisation. Sinon (0), la constante
    // techno reprend la main — comportement inchangé.
    mutable float _rPolRLS  = 0.0f;    // Ω/cell — 0 = pas de surcharge
    mutable float _tauPolRLS = 0.0f;   // s    — 0 = pas de surcharge
    // v5.39/v5.40 (A6, N4) — R_pol LISSÉ effectivement utilisé (Ω/cell).
    // Sert au transfert sans à-coup entre la branche techno et l'AFFRLS.
    mutable float _rPolUsed = 0.0f;
    // Constante de temps du lissage de R_pol (s). 10 s : assez court pour que la
    // branche soit cohérente en quelques secondes, assez long pour qu'aucun
    // échantillon ne porte un saut de résistance.
    static constexpr float RPOL_SLEW_TAU_S = 10.0f;

    // ===== NOUVEAU v1.6.5: Peukert paramétrable + rendement variable =====
    // Sentinelle NAN = garder le défaut de la technologie (k_peukert).
    mutable float _peukertNOverride    = NAN;   // exposant n (1 = idéal)
    mutable float _peukertIrefOverride = NAN;   // courant de référence (A)

    // Fabrique des sentinelles "non surchargé" (NAN pour les floats, 0 pour les uint8_t)
    static DetectionConfig overrideDetectionNone() {
        DetectionConfig c = {NAN, NAN, NAN, NAN, NAN, NAN, NAN};
        return c;
    }
    static KalmanTuning overrideKalmanNone() {
        KalmanTuning k = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN,
                          NAN, NAN, {NAN, NAN, NAN}, NAN, false,
                          NAN, 0, NAN, NAN, NAN, 0, 0,
                          NAN, NAN, NAN, NAN, NAN};
        return k;
    }
    // Résolution d'un champ : 1. override runtime > 2. macro > 3. défaut tech
    static inline float resolveVal(float def, float macroVal, float ovr) {
        if (!isnan(ovr))      return ovr;
        if (!isnan(macroVal)) return macroVal;
        return def;
    }

    // Lecture PROGMEM d'un float dans une table 2D
    static inline float readTableFloat(const float (*tbl)[2], int row, int col) {
        return BATT_READ_FLOAT(&tbl[row][col]);
    }

    // Recuperation parametres avec cache
    const TechParams& getParams() const {
        if (!_paramsCached) {
            // v2.0.0 — correspondance NOMINATIVE (cf. tech/TechRegistry.h) :
            // plus d'indexation par tableau, donc plus de décalage silencieux
            // si une technologie est ajoutée ou réordonnée.
            techParamsCopy(tech, _paramsCache);
            // v1.6.1: seuils de tension ajustables au runtime (NAN = défaut tech)
            if (!isnan(_vFloatPerCellOverride)) _paramsCache.V_float_per_cell = _vFloatPerCellOverride;
            if (!isnan(_vAbsPerCellOverride))   _paramsCache.V_abs_per_cell   = _vAbsPerCellOverride;
            _paramsCached = true;
        }
        return _paramsCache;
    }

    // ===== NOUVEAU v1.4.0: résolution hiérarchique de la config =====
    // 1. override runtime (setter) > 2. macro compile-time > 3. défaut technologie
    void resolveConfig() const {
        const DetectionConfig& d = getParams().detect;
        DetectionConfig r;
        r.current_deadband_A    = resolveVal(d.current_deadband_A,    BATTERYMODELS_DEADBAND_A,           _detectOverride.current_deadband_A);
        r.float_current_min_A   = resolveVal(d.float_current_min_A,   BATTERYMODELS_FLOAT_CURRENT_MIN_A,  _detectOverride.float_current_min_A);
        r.float_current_factor  = resolveVal(d.float_current_factor,  BATTERYMODELS_FLOAT_CURRENT_FACTOR, _detectOverride.float_current_factor);
        r.v_tol_pct             = resolveVal(d.v_tol_pct,             BATTERYMODELS_V_TOL_PCT,            _detectOverride.v_tol_pct);
        r.v_tol_min_V           = resolveVal(d.v_tol_min_V,           BATTERYMODELS_V_TOL_MIN_V,          _detectOverride.v_tol_min_V);
        r.ocv_rest_hours        = resolveVal(d.ocv_rest_hours,        BATTERYMODELS_OCV_REST_HOURS,       _detectOverride.ocv_rest_hours);
        r.cycle_thr_pct         = resolveVal(d.cycle_thr_pct,         BATTERYMODELS_CYCLE_THR_PCT,        _detectOverride.cycle_thr_pct);
        _detectResolved = r;

        KalmanTuning kt = getParams().kalman;   // base = défaut par technologie
        if (!isnan(_kalmanOverride.p_init_C))            kt.p_init_C = _kalmanOverride.p_init_C;
        if (!isnan(_kalmanOverride.p_min_C))             kt.p_min_C = _kalmanOverride.p_min_C;
        if (!isnan(_kalmanOverride.p_init_aging))        kt.p_init_aging = _kalmanOverride.p_init_aging;
        if (!isnan(_kalmanOverride.p_min_aging))         kt.p_min_aging = _kalmanOverride.p_min_aging;
        if (!isnan(_kalmanOverride.q_aging_uncertainty)) kt.q_aging_uncertainty = _kalmanOverride.q_aging_uncertainty;
        if (!isnan(_kalmanOverride.q_capacity_drift))    kt.q_capacity_drift = _kalmanOverride.q_capacity_drift;
        if (!isnan(_kalmanOverride.r_init))              kt.r_init = _kalmanOverride.r_init;
        if (!isnan(_kalmanOverride.r_min))               kt.r_min = _kalmanOverride.r_min;
        if (!isnan(_kalmanOverride.r_max))               kt.r_max = _kalmanOverride.r_max;
        if (!isnan(_kalmanOverride.r_smooth_alpha))      kt.r_smooth_alpha = _kalmanOverride.r_smooth_alpha;
        if (!isnan(_kalmanOverride.seg_min_dAh_abs))     kt.seg_min_dAh_abs = _kalmanOverride.seg_min_dAh_abs;
        if (!isnan(_kalmanOverride.seg_min_dAh_pct))     kt.seg_min_dAh_pct = _kalmanOverride.seg_min_dAh_pct;
        if (!isnan(_kalmanOverride.seg_min_dsoc[0]))     kt.seg_min_dsoc[0] = _kalmanOverride.seg_min_dsoc[0];
        if (!isnan(_kalmanOverride.seg_min_dsoc[1]))     kt.seg_min_dsoc[1] = _kalmanOverride.seg_min_dsoc[1];
        if (!isnan(_kalmanOverride.seg_min_dsoc[2]))     kt.seg_min_dsoc[2] = _kalmanOverride.seg_min_dsoc[2];
        if (!isnan(_kalmanOverride.seg_dsoc_high_conf))  kt.seg_dsoc_high_conf = _kalmanOverride.seg_dsoc_high_conf;
        if (_ocvSegmentAllowedSet)                       kt.ocv_segment_allowed = _kalmanOverride.ocv_segment_allowed;
        if (!isnan(_kalmanOverride.battery_change_thr))  kt.battery_change_thr = _kalmanOverride.battery_change_thr;
        if (_kalmanOverride.battery_change_count != 0)   kt.battery_change_count = _kalmanOverride.battery_change_count;
        if (!isnan(_kalmanOverride.rest_long_min_min))   kt.rest_long_min_min = _kalmanOverride.rest_long_min_min;
        if (!isnan(_kalmanOverride.rest_long_stable_mv)) kt.rest_long_stable_mv = _kalmanOverride.rest_long_stable_mv;
        if (!isnan(_kalmanOverride.rest_long_stable_min)) kt.rest_long_stable_min = _kalmanOverride.rest_long_stable_min;
        if (_kalmanOverride.rest_long_stable_min_samples != 0) kt.rest_long_stable_min_samples = _kalmanOverride.rest_long_stable_min_samples;
        if (_kalmanOverride.rest_long_stable_max_samples != 0) kt.rest_long_stable_max_samples = _kalmanOverride.rest_long_stable_max_samples;
        if (!isnan(_kalmanOverride.conf_float))          kt.conf_float = _kalmanOverride.conf_float;
        if (!isnan(_kalmanOverride.conf_rest_base))      kt.conf_rest_base = _kalmanOverride.conf_rest_base;
        if (!isnan(_kalmanOverride.conf_rest_span))      kt.conf_rest_span = _kalmanOverride.conf_rest_span;
        if (!isnan(_kalmanOverride.conf_phase_coarse))   kt.conf_phase_coarse = _kalmanOverride.conf_phase_coarse;
        if (!isnan(_kalmanOverride.conf_phase_refine))   kt.conf_phase_refine = _kalmanOverride.conf_phase_refine;
        _kalmanResolved = kt;
        _configDirty = false;
    }

    // v1.6.10 — rendement faradique η CONSTANT, découplé de Peukert.
    // ⚠️ La loi appliquée jusqu'à la 1.6.9 était η(I) = η_base·(I_ref/|I|)^(n−1),
    // écrêtée à [0,5 ; 1]. Deux défauts (identiques à CoulombsAh 1.3.0) :
    //   1. Sens inversé en décharge : Peukert décrit la capacité DISPONIBLE, qui
    //      décroît avec |I| ⇒ les Ah prélevés doivent être AMPLIFIÉS, pas réduits.
    //      Appliquée comme rendement, la loi réduisait la décharge (58 % à 5 A).
    //   2. Écrêtée à 1,0, elle vaut exactement 1,0 pour |I| ≤ ~1 A : elle
    //      n'applique aucune correction dans le régime de faible courant.
    // La loi de Peukert reste disponible là où elle est juste :
    // getEffectiveCapacity(I) / updateCoulombCount().
    float _variableEfficiency(float base, float current) const {
        (void)current;   // η ne dépend pas du courant (signature conservée)
        return base;
    }

    float calculateRsocWithSaturation(float s_ref, float gamma_soc, float max_ratio) const {        if (s_ref <= 0.0f) return max_ratio;
        s_ref = constrain(s_ref, 0.001f, 1.0f);

        float unbounded = 1.0f + gamma_soc * (1.0f / s_ref - 1.0f);
        if (unbounded <= 1.0f) return 1.0f;

        float overshoot = unbounded - 1.0f;
        float max_overshoot = max_ratio - 1.0f;

        if (overshoot >= max_overshoot) {
            return max_ratio;
        }

        float saturation_factor = overshoot / max_overshoot;
        float smoothing = 1.0f - expf(-2.0f * saturation_factor * saturation_factor);

        return 1.0f + max_overshoot * smoothing;
    }

    // Interpolation avec lecture PROGMEM et validation
    float interpolateTable(const float (*tbl)[2], int n, float V_cell) const {
        if (n <= 0 || tbl == nullptr) return 50.0f;

        float v0 = readTableFloat(tbl, 0, 0);
        float s0 = readTableFloat(tbl, 0, 1);
        if (V_cell >= v0) return s0;

        float vLast = readTableFloat(tbl, n-1, 0);
        float sLast = readTableFloat(tbl, n-1, 1);
        if (V_cell <= vLast) return sLast;

        for (int i = 0; i < n - 1; i++) {
            float vi = readTableFloat(tbl, i, 0);
            float vi1 = readTableFloat(tbl, i+1, 0);
            if (V_cell <= vi && V_cell >= vi1) {
                float si = readTableFloat(tbl, i, 1);
                float si1 = readTableFloat(tbl, i+1, 1);
                float denom = vi - vi1;
                if (fabsf(denom) < 1e-9f) return si1;
                float t = (V_cell - vi1) / denom;
                t = constrain(t, 0.0f, 1.0f);
                return si1 + t * (si - si1);
            }
        }
        return 50.0f;
    }

    // Interpolation inverse: SoC -> OCV
    float interpolateTableInverse(const float (*tbl)[2], int n, float soc) const {
        if (n <= 0 || tbl == nullptr) return 0.0f;
        if (soc >= 100.0f) return readTableFloat(tbl, 0, 0);
        if (soc <= 0.0f) return readTableFloat(tbl, n-1, 0);

        for (int i = 0; i < n - 1; i++) {
            float si = readTableFloat(tbl, i, 1);
            float si1 = readTableFloat(tbl, i+1, 1);
            if (soc <= si && soc >= si1) {
                float vi = readTableFloat(tbl, i, 0);
                float vi1 = readTableFloat(tbl, i+1, 0);
                float denom = si - si1;
                if (fabsf(denom) < 1e-9f) return vi1;
                float t = (soc - si1) / denom;
                t = constrain(t, 0.0f, 1.0f);
                return vi1 + t * (vi - vi1);
            }
        }
        return readTableFloat(tbl, n/2, 0);
    }

    float soc_from_ocv_poly(const OcvPoly& p, float V_cell) const {
        float A = p.A;
        float B = p.B;
        float Cv = p.C - V_cell;

        float soc;
        if (fabsf(A) < 1e-6f) {
            soc = (fabsf(B) > 1e-6f) ? (-Cv / B) * 100.0f : 50.0f;
        } else {
            float D = B * B - 4.0f * A * Cv;
            if (D < 0.0f) {
                soc = (V_cell > p.C + p.B * 0.5f + p.A * 0.25f) ? 100.0f : 0.0f;
            } else {
                float sqrtD = sqrtf(D);
                float r1 = (-B + sqrtD) / (2.0f * A);
                float r2 = (-B - sqrtD) / (2.0f * A);
                if (r1 >= 0.0f && r1 <= 1.0f) soc = r1 * 100.0f;
                else if (r2 >= 0.0f && r2 <= 1.0f) soc = r2 * 100.0f;
                else soc = constrain(r1, 0.0f, 1.0f) * 100.0f;
            }
        }
        return constrain(soc, 0.0f, 100.0f);
    }

    float ocv_from_soc_poly(const OcvPoly& p, float soc) const {
        float s = constrain(soc / 100.0f, 0.0f, 1.0f);
        return p.A * s * s + p.B * s + p.C;
    }

    // --- Override cubique (v1.6.2) : V_cell(s) = a0 + a1·s + a2·s² + a3·s³ ---
    float ocv_from_soc_cubic(float soc) const {
        float s = constrain(soc / 100.0f, 0.0f, 1.0f);
        return _ocvA0 + s * (_ocvA1 + s * (_ocvA2 + s * _ocvA3));
    }

    // Inverse robuste V_cell → SoC% : recherche de racine par bissection.
    // L'OCV est supposé monotone croissant sur s∈[0,1] (vrai pour le plomb).
    // Retourne 0 / 100 si V_cell sort des bornes du polynôme.
    float soc_from_ocv_cubic(float V_cell) const {
        float f0 = _ocvA0 - V_cell;                                            // s = 0
        float f1 = _ocvA0 + _ocvA1 + _ocvA2 + _ocvA3 - V_cell;                 // s = 1
        if (f0 >= 0.0f) return 0.0f;
        if (f1 <= 0.0f) return 100.0f;
        float lo = 0.0f, hi = 1.0f;
        for (int i = 0; i < 24; i++) {
            float mid = 0.5f * (lo + hi);
            float f = _ocvA0 + mid * (_ocvA1 + mid * (_ocvA2 + mid * _ocvA3)) - V_cell;
            if (f < 0.0f) lo = mid; else hi = mid;
        }
        return constrain(0.5f * (lo + hi) * 100.0f, 0.0f, 100.0f);
    }

    // Detection automatique par analyse de la pente OCV
    BatteryTech autoDetectTech(float voltage, float current, float temperature) const {
        if (n_cells == 0) return TECH_UNKNOWN;
        float V_cell = voltage / n_cells;

        // Test LiFePO4: plateau tres plat entre 3.2V et 3.4V
        if (V_cell >= 3.0f && V_cell <= 3.6f) {
            float soc_est = interpolateTable(OCV_LIFEPO4, 7, V_cell);
            float V_check = interpolateTableInverse(OCV_LIFEPO4, 7, soc_est + 10.0f);
            float slope = (V_check - V_cell) / 0.1f;
            if (fabsf(slope) < 0.5f) return TECH_LIFEPO4;
        }

        // Test LTO: tension tres basse (1.9V - 2.8V)
        if (V_cell >= 1.8f && V_cell <= 2.9f) return TECH_LTO;

        // Test plomb: tension moderée (1.75V - 2.5V par cellule)
        if (V_cell >= 1.7f && V_cell <= 2.6f) {
            if (R_int_nominal > 0.0f) {
                if (R_int_nominal < 10.0f) return TECH_AGM;
                if (R_int_nominal > 12.0f) return TECH_GEL;
            }
            return TECH_FLOODED;
        }

        // Test Li-Ion haute tension
        if (V_cell >= 3.8f) {
            if (V_cell > 4.25f) return TECH_LMNO;
            if (current > 0.5f) {
                float soc_est = interpolateTable(OCV_LION, 9, V_cell);
                if (soc_est > 90.0f && V_cell < 4.15f) return TECH_LIPO;
            }
            return TECH_LION;
        }

        // Test Sodium
        if (V_cell >= 2.0f && V_cell <= 4.0f) return TECH_SODIUM;

        return TECH_UNKNOWN;
    }

public:
    // Constructeur avec validation
    BatteryModel(BatteryTech t = TECH_AGM, uint8_t cells = 6, 
                 float cap = 0.0f, float r_int = 0.0f)
        : tech(t), n_cells(max(cells, (uint8_t)1)), C_nominal(max(cap, 0.0f)), 
          R_int_nominal(max(r_int, 0.0f)), auto_detect(cap == 0.0f) {}

    // Informations de base
    float getNominalVoltage() const {
        switch (tech) {
            case TECH_LIFEPO4: return 3.2f * n_cells;
            case TECH_LION:
            case TECH_NMC:
            case TECH_NCA:     return 3.6f * n_cells;
            case TECH_LIPO:    return 3.7f * n_cells;
            case TECH_LTO:     return 2.4f * n_cells;
            case TECH_LMNO:    return 3.7f * n_cells;
            case TECH_NIFE:    return 1.2f * n_cells;
            case TECH_SODIUM:  return 3.0f * n_cells;
            case TECH_NIMH:    return 1.2f * n_cells;
            case TECH_ALKALINE: return 1.5f * n_cells;
            case TECH_LCO:     return 3.7f * n_cells;
            case TECH_NIZN:    return 1.6f * n_cells;
            case TECH_NICD:    return 1.2f * n_cells;
            case TECH_PBC:     return 2.0f * n_cells;
            case TECH_SUPERCAP: return 2.0f * n_cells;  // ~0.75 * V_max (convention constructeurs)
            default:           return 2.0f * n_cells;
        }
    }

    bool isRechargeable() const {
        return getParams().is_rechargeable;
    }

    uint8_t getCellCount() const { return n_cells; }
    float getNominalCapacity() const { return C_nominal; }

    float getInternalResistance() const {
        if (R_int_nominal > 0.0f) return R_int_nominal;
        return getParams().R25_mOhm;
    }

    // ============================================================
    // ESTIMATION SOC / OCV
    // ============================================================

    float ocvToSoc(float voltage, float temperature) const {
        if (n_cells == 0) return 0.0f;
        float V_cell = voltage / n_cells;

        // Normalize to 25°C reference (all tables are at 25°C)
        V_cell = removeThermalCorrectionOCV(V_cell, temperature);

        switch (tech) {
            case TECH_LIFEPO4:
                return interpolateTable(OCV_LIFEPO4, 7, V_cell);
            case TECH_LION:
                return interpolateTable(OCV_LION, 9, V_cell);
            case TECH_LIPO:
                return interpolateTable(OCV_LIPO, 9, V_cell);
            case TECH_LTO:
                return interpolateTable(OCV_LTO, 9, V_cell);
            case TECH_LMNO:
                return interpolateTable(OCV_LMNO, 9, V_cell);
            case TECH_NMC:
                return interpolateTable(OCV_NMC, 9, V_cell);
            case TECH_NCA:
                return interpolateTable(OCV_NCA, 9, V_cell);
            case TECH_SODIUM:
                return interpolateTable(OCV_SODIUM, 9, V_cell);
            case TECH_LCO:
                return interpolateTable(OCV_LCO, 9, V_cell);
            case TECH_SUPERCAP: {
                // SoC base energie (convention Maxwell) : E = 0.5*C*(V²-Vmin²)
                const TechParams& tp = getParams();
                float vmin2 = tp.V_min_per_cell * tp.V_min_per_cell;
                float vmax2 = tp.V_max_per_cell * tp.V_max_per_cell;
                float soc = (V_cell * V_cell - vmin2) / (vmax2 - vmin2) * 100.0f;
                return constrain(soc, 0.0f, 100.0f);
            }
            default:
                // v1.6.2 : override cubique prioritaire sur le polynôme techno
                if (_ocvPolySet) return soc_from_ocv_cubic(V_cell);
                return soc_from_ocv_poly(getParams().ocv, V_cell);
        }
    }

    float socToOcv(float soc, float temperature) const {
        if (n_cells == 0) return 0.0f;
        soc = constrain(soc, 0.0f, 100.0f);
        float V_cell;

        switch (tech) {
            case TECH_LIFEPO4:
                V_cell = interpolateTableInverse(OCV_LIFEPO4, 7, soc);
                break;
            case TECH_LION:
                V_cell = interpolateTableInverse(OCV_LION, 9, soc);
                break;
            case TECH_LIPO:
                V_cell = interpolateTableInverse(OCV_LIPO, 9, soc);
                break;
            case TECH_LTO:
                V_cell = interpolateTableInverse(OCV_LTO, 9, soc);
                break;
            case TECH_LMNO:
                V_cell = interpolateTableInverse(OCV_LMNO, 9, soc);
                break;
            case TECH_NMC:
                V_cell = interpolateTableInverse(OCV_NMC, 9, soc);
                break;
            case TECH_NCA:
                V_cell = interpolateTableInverse(OCV_NCA, 9, soc);
                break;
            case TECH_SODIUM:
                V_cell = interpolateTableInverse(OCV_SODIUM, 9, soc);
                break;
            case TECH_LCO:
                V_cell = interpolateTableInverse(OCV_LCO, 9, soc);
                break;
            case TECH_SUPERCAP: {
                // Inverse energie : V = sqrt(Vmin² + soc*(Vmax²-Vmin²))
                const TechParams& tp = getParams();
                float vmin2 = tp.V_min_per_cell * tp.V_min_per_cell;
                float vmax2 = tp.V_max_per_cell * tp.V_max_per_cell;
                V_cell = sqrtf(vmin2 + soc / 100.0f * (vmax2 - vmin2));
                break;
            }
            default:
                // v1.6.2 : override cubique prioritaire sur le polynôme techno
                V_cell = _ocvPolySet ? ocv_from_soc_cubic(soc)
                                     : ocv_from_soc_poly(getParams().ocv, soc);
                break;
        }

        // Apply advanced thermal correction (linear + quadratic)
        V_cell = applyThermalCorrectionOCV(V_cell, temperature);
        return V_cell * n_cells;
    }

    // ============================================================
    // THERMAL COMPENSATION (Advanced v1.2)
    // ============================================================

    // Calculate internal resistance with thermal compensation
    // Uses Arrhenius model if E_a_R > 0, otherwise linear model
    float getResistanceAtTemp(float R25, float temperature) const {
        const TechParams& tp = getParams();

        if (tp.E_a_R > 0.001f) {
            // Arrhenius model: R(T) = R25 * exp(E_a/R * (1/T - 1/T25))
            // Simplified: R(T) = R25 * exp(alpha * (T25 - T))
            // where alpha ≈ E_a / (R_gas * T25 * T)
            float T_abs = temperature + 273.15f;
            float T25_abs = 298.15f;
            float R_gas = 8.314f;
            float alpha = tp.E_a_R / (R_gas * T25_abs * T_abs);
            return R25 * expf(alpha * (T25_abs - T_abs));
        } else {
            // Linear model (backward compatible)
            return R25 * (1.0f + tp.beta_temp * (25.0f - temperature));
        }
    }

    // Calculate RC time constant with thermal compensation
    // tau(T) = tau_25 * exp(alpha_tau * (T - 25))
    float getTauPol_thermal(float temperature) const {
        const TechParams& tp = getParams();
        if (fabsf(tp.alpha_tau) < 1e-6f) {
            return tp.tau_pol_s;  // No thermal effect
        }
        return tp.tau_pol_s * expf(tp.alpha_tau * (temperature - 25.0f));
    }

    // Calculate OCV with advanced thermal compensation
    // V_ocv(T) = V_ref + dVocv_dT*ΔT + 0.5*d2Vocv_dT2*ΔT²
    float applyThermalCorrectionOCV(float V_cell, float temperature) const {
        const TechParams& tp = getParams();
        float dT = temperature - 25.0f;

        // Linear + quadratic terms
        V_cell += tp.dVocv_dT * dT;
        if (fabsf(tp.d2Vocv_dT2) > 1e-9f) {
            V_cell += 0.5f * tp.d2Vocv_dT2 * dT * dT;
        }
        return V_cell;
    }

    // Inverse: remove thermal effect to normalize to 25°C reference
    float removeThermalCorrectionOCV(float V_cell, float temperature) const {
        const TechParams& tp = getParams();
        float dT = temperature - 25.0f;

        // Remove linear + quadratic thermal effects
        V_cell -= tp.dVocv_dT * dT;
        if (fabsf(tp.d2Vocv_dT2) > 1e-9f) {
            V_cell -= 0.5f * tp.d2Vocv_dT2 * dT * dT;
        }
        return V_cell;
    }

    // ============================================================
    // CORRECTION DE TENSION (MODELE THEVENIN)
    // ============================================================

    // soc_ref : SoC de RÉFÉRENCE (0..100), typiquement le SoC COULOMB (factuel).
    //   ⚠️ 02/10/2026 — BUG CORRIGÉ : avant, `soc_est` était dérivé de la tension
    //   TERMINALE. Sous charge, si V tombe hors de la table OCV (plomb < 1,98 V/
    //   cellule), ocvToSoc renvoyait 0 → calculateRsocWithSaturation renvoyait
    //   R_soc_max (18) → R_int énorme → correction I·R qui poussait V_corrigée
    //   AU-DESSUS de l'OCV max → SoC_voltage = 100 % (emballement inversé).
    //   On utilise désormais le SoC de référence (coulomb) quand il est fourni,
    //   avec un plancher sur s_ref pour supprimer la saturation de R_soc.
    //   NAN = pas de référence → repli sur l'OCV de la tension, également borné.
    float correctVoltageToOCV(float voltage, float current, 
                              float temperature, float dt_s,
                              float* r_int_eff = nullptr,
                              float* v_pol = nullptr,
                              float soc_ref = NAN) {
        if (n_cells == 0) return voltage;

        const TechParams& tp = getParams();
        float V_cell = voltage / n_cells;

        float R25 = (R_int_nominal > 0.0f) ? R_int_nominal : tp.R25_mOhm;
        float R_T = getResistanceAtTemp(R25, temperature);
        R_T = max(R_T, 0.001f);

        float soc_est = (isfinite(soc_ref) && soc_ref > 0.0f)
                        ? soc_ref
                        : ocvToSoc(voltage, temperature);
        float s_ref = constrain(soc_est / 100.0f, 0.05f, 1.0f);   // plancher anti-saturation

        float R_soc = calculateRsocWithSaturation(s_ref, tp.gamma_soc, tp.R_soc_max);

        float R_int = R_T * R_soc / 1000.0f;

        // v2.1.0 (F-02) — POLARISATION : identifiée (AFFRLS) si disponible, sinon
        // constante techno. Les deux consommateurs (cette fonction pour l'ancre
        // OCV, `SocEkf` pour la prédiction) doivent soustraire la MÊME branche,
        // sinon les deux SoC ne peuvent pas converger.
        float R_pol, tau_pol;
        if (_rPolRLS > 0.0f) {
            R_pol   = _rPolRLS;                              // Ω/cell, mesuré
            tau_pol = (_tauPolRLS > 0.0f) ? _tauPolRLS       // s, mesuré
                                          : tp.tau_pol_s;
        } else {
            R_pol   = (tp.Rpol0_mOhm / 1000.0f)
                      * (1.0f + tp.delta_pol * expf(-soc_est / 20.0f));
            tau_pol = getTauPol_thermal(temperature);        // constante techno
        }

        // ⚠️ v5.40 (A6 revu / N4) — LISSAGE DE R_pol, PAS TÉLÉDÉPORTATION DE L'ÉTAT.
        // Historique de la décision, parce qu'elle a changé deux fois :
        //   • v5.39 (A6) : `V_pol_state` était remis à l'échelle d'un coup
        //     (`× R_nouveau/R_ancien`). Physiquement défendable, mais la
        //     re-relecture a montré que cela transformait une dérive de 60 s en
        //     un SAUT de 0,33 V batterie : l'amplitude du transitoire ne change
        //     pas, seule sa durée passe à zéro — et un saut d'ancre est pire
        //     qu'une dérive, parce qu'il peut déclencher un recalage de SoC.
        //   • v5.40 : `R_pol` est LISSÉ (τ = RPOL_SLEW_TAU_S). La cible de
        //     l'état devient continue, donc l'OCV aussi ; l'état converge vers la
        //     nouvelle branche via son propre EMA (τ_pol), sans discontinuité.
        // `_rPolUsed` porte la valeur LISSÉE effectivement employée : c'est elle
        // qui entre dans `V_pol_inf`, donc les deux consommateurs restent alignés.
        {
            float dtp = (dt_s > 0.0f) ? dt_s : 0.1f;
            if (_rPolUsed <= 0.0f || !isfinite(_rPolUsed)) {
                _rPolUsed = R_pol;                       // 1er appel : pas de transitoire
            } else {
                float aR = 1.0f - expf(-dtp / RPOL_SLEW_TAU_S);
                aR = constrain(aR, 0.0f, 1.0f);
                _rPolUsed += aR * (R_pol - _rPolUsed);
            }
            R_pol = _rPolUsed;
        }

        float V_pol_inf = current * R_pol;

        if (dt_s > 0.0f && tau_pol > 0.0f) {
            float alpha_rc = 1.0f - expf(-dt_s / tau_pol);
            alpha_rc = constrain(alpha_rc, 0.0f, 1.0f);
            V_pol_state += alpha_rc * (V_pol_inf - V_pol_state);
        } else if (dt_s < 0.0f) {
            V_pol_state = 0.0f;
        }

        // Hystérésis (one-state, v1.6.2) : h → +h_max en charge, −h_max en décharge,
        // constante de temps 1/gamma. On RETRANCHE h pour ramener V_terminal à l'OCV
        // « sans hystérésis » → la lecture SoC ne dépend plus du sens du courant.
        if (_hystMax > 0.0f && dt_s > 0.0f && _hystGamma > 0.0f) {
            float target = (current > 0.0f) ? _hystMax
                         : (current < 0.0f ? -_hystMax : _hystState);
            float alpha_h = 1.0f - expf(-dt_s * _hystGamma);
            alpha_h = constrain(alpha_h, 0.0f, 1.0f);
            _hystState += alpha_h * (target - _hystState);
            _hystState = constrain(_hystState, -_hystMax, _hystMax);
        }

        // 2ᵉ branche RC (diffusion lente, v1.6.4) — discrétisation exacte e^(−Δt/τ).
        if (_r2mOhm > 0.0f && _tau2 > 0.0f && dt_s > 0.0f) {
            float R2 = _r2mOhm / 1000.0f;
            float alpha2 = 1.0f - expf(-dt_s / _tau2);
            alpha2 = constrain(alpha2, 0.0f, 1.0f);
            _v2State += alpha2 * (current * R2 - _v2State);
        } else if (dt_s < 0.0f) {
            _v2State = 0.0f;
        }

        // Gassing (branche parasite, v1.6.4) : en charge, si la tension terminale
        // dépasse le seuil de dégagement, une surtension apparaît (état lissé) →
        // elle est retranchée comme la polarisation (elle n'est PAS de l'OCV).
        //
        // ⚠️ v2.1.0 (F-07) — POURQUOI `k = 1,82 > 1` N'A PAS ÉTÉ CORRIGÉ.
        // `k > 1` rend le transfert NON MONOTONE : à l'état établi
        //     V_corr = V·(1 − k) + k·V_gas  ⇒  dV_corr/dV = 1 − k = −0,82
        // c'est-à-dire un OCV qui DIMINUE quand la tension terminale AUGMENTE.
        // La correction « borne k ≤ 1 » a été implémentée puis MESURÉE, puis
        // RETIRÉE — parce qu'elle casse le calage du palier de floating :
        //
        //     R_int (mΩ/cell)   k=1,82      k=1 (borné)   k=0
        //     15 (défaut)       12,698 ✅   13,355        14,155
        //     34 (production)   13,047      13,047        13,847
        //     58                12,659      12,659        13,459
        //     112 (R0 mesuré)   11,779      11,779        12,579
        //                                    (cible 12,70)
        //
        // Aucune valeur PLAUSIBLE de `R_int` ne redonne 12,70 V avec k ≤ 1.
        // Conclusion MESURÉE : la surtension de gazage n'est pas physique ici —
        // elle ABSORBE une résistance sous-estimée (350 mΩ modèle contre 820 mΩ
        // mesurés, cf. F-04). Physiquement, à 2,483 V/cell et 2,7 A, le potentiel
        // d'électrode vaut 2,392 V/cell : SOUS le seuil de 2,35 V, il ne devrait
        // y avoir aucun dégagement — et pourtant il faut retirer 0,19 V/cell.
        // ⇒ F-07 est BLOQUÉ par F-04. Corriger le gain avant la résistance
        //   remplacerait un biais par un autre. Ne pas « corriger » ce k seul.
        if (_vGasCell > 0.0f && _kGas > 0.0f) {
            float target = (current > 0.0f && V_cell > _vGasCell)
                           ? _kGas * (V_cell - _vGasCell) : 0.0f;
            if (dt_s > 0.0f) {
                // lissage rapide (τ ≈ 20 s) — le dégagement suit la charge
                float aGas = 1.0f - expf(-dt_s / 20.0f);
                aGas = constrain(aGas, 0.0f, 1.0f);
                _gasState += aGas * (target - _gasState);
            } else {
                _gasState = target;
            }
            _gasState = constrain(_gasState, 0.0f, 5.0f);
        }

        float V_drop_ohm = R_int * current;
        float V_corrected = V_cell - (V_drop_ohm + V_pol_state + _v2State)
                            - _hystState - _gasState;

        // ⚠️ v1.6.11 (D3) — LA CORRECTION THERMIQUE EST RETIRÉE ICI.
        // `correctVoltageToOCV()` renvoie désormais l'OCV **À LA TEMPÉRATURE DE
        // MESURE**, ce qui est sa sémantique naturelle : on a retiré des chutes
        // ohmiques et de polarisation mesurées à T, on obtient un OCV à T.
        //
        // Avant, elle ajoutait `dVocv_dT·(T−25)` ici, puis `ocvToSoc(V, T)`
        // retranchait EXACTEMENT la même quantité. Les deux s'annulaient :
        // la correction thermique de l'OCV était un NO-OP total. Vérifié
        // numériquement (V=12,03 / I=−0,566 / SoC 65 %) :
        //     T = 5 °C  → SoC_v 55,58 %   (l'écart venait de R_int(T) seul)
        //     T = 13,8 °C → 54,59 %
        //     T = 30 °C → 52,94 %
        // Or `ocvToSoc(V, T)` fait `removeThermalCorrectionOCV()` puis cherche
        // dans la table 25 °C : lui passer un OCV **déjà** corrigé à 25 °C
        // double la correction (−5,1 points mesurés à 13,8 °C).
        //
        // Le coefficient dVocv_dT reste UTILISÉ, et au bon endroit :
        // `socToOcv(soc, T)` (utilisé par l'EKF pour prédire V_terminal) et
        // `ocvToSoc(V, T)` (conversion vers la table de référence 25 °C).

        if (r_int_eff) *r_int_eff = R_int;
        if (v_pol) *v_pol = V_pol_state;

        last_current = current;
        return V_corrected * n_cells;
    }

    // ============================================================
    // DETECTION ETAT DE CHARGE
    // ============================================================

    float getVoltageToleranceBand() const {
        float V_nom = getNominalVoltage();
        if (V_nom <= 0.0f) return 0.2f;
        const DetectionConfig& d = getDetectionConfig();
        return max(d.v_tol_min_V, V_nom * d.v_tol_pct);
    }

    ChargeState detectChargeState(float voltage, float current,
                                   float capacity = -1.0f) const {
        const TechParams& tp = getParams();
        const DetectionConfig& d = getDetectionConfig();

        float cap = (capacity > 0.0f) ? capacity : C_nominal;
        if (cap <= 0.0f) cap = 100.0f;

        float V_float_ref = tp.V_float_per_cell * n_cells;
        float V_abs_ref = tp.V_abs_per_cell * n_cells;
        float I_float_max = max(d.float_current_min_A, cap * d.float_current_factor);
        float V_tol = getVoltageToleranceBand();

        bool in_rest = (fabsf(current) < d.current_deadband_A);
        bool charging = (current > d.current_deadband_A);
        bool discharging = (current < -d.current_deadband_A);

        bool near_abs = (voltage >= V_abs_ref - V_tol) && (voltage <= V_abs_ref + V_tol);
        bool near_float = (voltage >= V_float_ref - V_tol) && (voltage <= V_float_ref + V_tol * 0.5f);

        // FIX v1.3.0: near_float doit être testé AVANT in_rest.
        // En float, le courant chute sous 0.05A : l'ancien ordre classait
        // la batterie en State_REST et jamais State_FLOAT, privant
        // BatteryKalman du point de référence "batterie pleine" (SoC 100%).
        if (discharging) return State_DISCHARGE;
        if (near_float && fabsf(current) < I_float_max) return State_FLOAT;
        if (in_rest) return State_REST;
        if (near_abs && charging) return State_ABSORPTION;
        if (charging) return State_BULK;

        return State_UNKNOWN;
    }

    // ============================================================
    // COULOMB COUNTING AVEC PEUKERT
    // ============================================================
    // Accumule la charge transférée en tenant compte de la loi de Peukert.
    // La correction Peukert réduit la capacité effective à fort courant.
    // Exemple: À 2C (200A sur batterie 100Ah), capacité effective = 25Ah
    //   → chaque Ah physique "consomme" 4× plus de capacité nominale
    //   → accumulated_charge = I × dt × (C_nominal / C_eff)
    // Ceci rend le tracking SoC plus réaliste à hauts courants.

    float getPeukertReferenceRate() const {
        if (!isnan(_peukertIrefOverride)) return _peukertIrefOverride;
        if (n_cells == 0) return 1.0f;
        switch (tech) {
            case TECH_FLOODED:
            case TECH_AGM:
            case TECH_GEL:
            case TECH_NIFE:
            case TECH_PBC:
                return C_nominal / 20.0f;
            default:
                return C_nominal;
        }
    }

    float getEffectiveCapacity(float current) const {
        const TechParams& tp = getParams();
        float n_peukert = isnan(_peukertNOverride) ? tp.k_peukert : _peukertNOverride;
        if (fabsf(current) < 0.01f || n_peukert <= 1.0f) {
            return C_nominal;
        }
        float I_abs = fabsf(current);
        float I_nom = getPeukertReferenceRate();
        if (I_nom <= 0.0f) return C_nominal;
        float peukert_factor = powf(I_nom / I_abs, n_peukert - 1.0f);
        return C_nominal * peukert_factor;
    }

    void updateCoulombCount(float current, float dt_hours) {
        if (C_nominal <= 0.0f || dt_hours <= 0.0f) return;

        float C_eff = getEffectiveCapacity(current);
        float peukert_ratio = C_nominal / C_eff;
        last_charge_delta_Ah = current * dt_hours * peukert_ratio;
        accumulated_charge_Ah += last_charge_delta_Ah;

        if (fabsf(accumulated_charge_Ah) >= C_nominal * getDetectionConfig().cycle_thr_pct) {
            cycle_count++;
            accumulated_charge_Ah = 0.0f;
        }
    }

    float getLastChargeDelta() const {
        if (C_nominal <= 0.0f) return 0.0f;
        return (last_charge_delta_Ah / C_nominal) * 100.0f;
    }

    float getCoulombSoc(float initial_soc = 100.0f) const {
        if (C_nominal <= 0.0f) return initial_soc;
        float soc = initial_soc + (accumulated_charge_Ah / C_nominal * 100.0f);
        return constrain(soc, 0.0f, 100.0f);
    }

    void resetCoulombCount() {
        accumulated_charge_Ah = 0.0f;
    }

    // ============================================================
    // VIEILLISSEMENT
    // ============================================================

    float getAgingDriftPerCycle() const {
        if (tech == TECH_AGM) return 0.0005f;
        if (tech == TECH_LIFEPO4) return 0.0002f;
        if (tech == TECH_NIFE) return 0.0010f;
        return 0.0005f;
    }

    float getCapacityFactorAging() const {
        float drift = getAgingDriftPerCycle();
        return max(0.5f, 1.0f - (drift * cycle_count));
    }

    uint32_t getCycleCount() const { return cycle_count; }

    // ============================================================
    // FIABILITE OCV
    // ============================================================

    bool isOcvReliable() const {
        return (tech != TECH_NIFE && tech != TECH_SODIUM);
    }

    bool isOcvMeasurementValid(float current, float dt_rest_hours = 0.0f) const {
        if (!isOcvReliable()) return false;
        const DetectionConfig& d = getDetectionConfig();
        if (fabsf(current) > d.current_deadband_A) return false;
        if (dt_rest_hours < d.ocv_rest_hours) return false;
        return true;
    }

    // ============================================================
    // GETTERS / SETTERS
    // ============================================================

    // ===== NOUVEAU v1.4.0: config de détection + réglages Kalman =====
    // Retournent une référence sur un cache résolu (pas sur PROGMEM),
    // selon la hiérarchie : setter runtime > macro > défaut de la technologie.
    const DetectionConfig& getDetectionConfig() const {
        if (_configDirty) resolveConfig();
        return _detectResolved;
    }

    const KalmanTuning& getKalmanTuning() const {
        if (_configDirty) resolveConfig();
        return _kalmanResolved;
    }

    // --- Overrides runtime DetectionConfig ---
    void setCurrentDeadband(float amps) {
        _detectOverride.current_deadband_A = amps;
        _configDirty = true;
    }
    void setVoltageTolerance(float pct, float min_V) {
        _detectOverride.v_tol_pct = pct;
        _detectOverride.v_tol_min_V = min_V;
        _configDirty = true;
    }
    // --- Override runtime des seuils de tension par cellule (V/cell) ---
    // Aligne FLOAT/ABSORPTION sur les tensions réelles du chargeur. NAN = garder
    // le défaut de la technologie. Ex. plomb Victron : setVoltageThresholds(2.30f)
    // (float 13.8 V) — l'absorption 14.8 V reste couverte par le défaut 2.45 V.
    void setVoltageThresholds(float v_float_per_cell, float v_abs_per_cell = NAN) {
        _vFloatPerCellOverride = v_float_per_cell;
        _vAbsPerCellOverride   = v_abs_per_cell;
        _paramsCached = false;
    }

    // --- Override runtime de la courbe OCV (CUBIQUE, par cellule) — v1.6.2 ---
    // V_cell(s) = a0 + a1·s + a2·s² + a3·s³, s = SOC/100 (0..1), V/cell.
    // ⚠️ Coefficients PAR CELLULE : pour une courbe mesurée sur la batterie
    // complète (ex. 12 V, 6 cellules), diviser chaque coefficient par n_cells
    // et convertir le SOC en s=SOC/100. Additif : sans appel, comportement
    // inchangé (tables/polynômes par technologie).
    void setOcvPolynomial(float a0, float a1, float a2, float a3) {
        _ocvA0 = a0; _ocvA1 = a1; _ocvA2 = a2; _ocvA3 = a3;
        _ocvPolySet = true;
    }
    void clearOcvPolynomial() { _ocvPolySet = false; }
    bool hasOcvPolynomial() const { return _ocvPolySet; }
    void getOcvPolynomial(float& a0, float& a1, float& a2, float& a3) const {
        a0 = _ocvA0; a1 = _ocvA1; a2 = _ocvA2; a3 = _ocvA3;
    }

    // --- Hystérésis charge/décharge (one-state) — v1.6.2 ---
    // h_max en V/CELLULE ; gamma = 1/constante_de_temps en 1/s (0 = gelé).
    // Ex. plomb : h_max ≈ 0,2 V/cell, gamma ≈ 1/300 s⁻¹ (bascule en ~10 min).
    void setHysteresis(float h_max_per_cell, float gamma_per_s) {
        _hystMax   = fabsf(h_max_per_cell);
        _hystGamma = (gamma_per_s > 0.0f) ? gamma_per_s : 0.0f;
        _hystState = constrain(_hystState, -_hystMax, _hystMax);
    }
    void clearHysteresis() { _hystMax = 0.0f; _hystState = 0.0f; _hystGamma = 0.0f; }
    float getHysteresis() const { return _hystState; }      // V/cell
    float getHysteresisMax() const { return _hystMax; }

    // --- 2ᵉ branche RC (diffusion lente) — v1.6.4 ---
    // R2 en mΩ/cellule, tau2 en secondes. 0 = désactivée (1 RC seule).
    void setSecondRC(float r2_mOhm_per_cell, float tau2_s) {
        _r2mOhm = (isfinite(r2_mOhm_per_cell) && r2_mOhm_per_cell > 0.0f) ? r2_mOhm_per_cell : 0.0f;
        _tau2   = (isfinite(tau2_s) && tau2_s > 0.0f) ? tau2_s : 0.0f;
    }
    void clearSecondRC() { _r2mOhm = 0.0f; _tau2 = 0.0f; _v2State = 0.0f; }
    float getSecondRCVoltage() const { return _v2State; }   // V/cell

    // --- v2.1.0 (F-02) : polarisation IDENTIFIÉE (AFFRLS) ---
    /**
     * @brief Remplace la branche de polarisation techno par celle de l'AFFRLS.
     *
     * @param r1_ohm_batterie R1 identifiée (Ω BATTERIE ENTIÈRE — comme R0)
     * @param tau1_s          τ1 identifiée (s)
     *
     * Sans cet appel, `correctVoltageToOCV` utilise la constante techno
     * (`Rpol0_mOhm` × (1 + delta_pol·exp(−SoC/20))) : comportement d'origine.
     * Avec, l'ancre OCV et `SocEkf` soustraient la MÊME branche — condition
     * nécessaire pour que `SoC_voltage` et `SoC_ekf` puissent converger.
     *
     * ⚠️ R1 doit être en Ω BATTERIE : la conversion en Ω/cellule est faite ici,
     * une seule fois (même piège que R2, cf. config.h MODEL_RC2_R_OHM_BATTERIE).
     */
    void setPolarizationFromRLS(float r1_ohm_batterie, float tau1_s) {
        float n = (float)((n_cells > 0) ? n_cells : 1);
        if (isfinite(r1_ohm_batterie) && r1_ohm_batterie > 0.0f) {
            _rPolRLS = r1_ohm_batterie / n;      // → Ω/cellule
        } else {
            _rPolRLS = 0.0f;
        }
        _tauPolRLS = (isfinite(tau1_s) && tau1_s > 0.0f) ? tau1_s : 0.0f;
    }
    void clearPolarizationFromRLS() { _rPolRLS = 0.0f; _tauPolRLS = 0.0f; }
    bool hasPolarizationFromRLS() const { return _rPolRLS > 0.0f; }

    // --- Gassing (branche parasite en fin de charge) — v1.6.4 ---
    // v_gas_per_cell : seuil de dégagement (V/cell). k_gas : surtension (V) par
    // volt de dépassement. Ex. plomb : setGassing(2.35f, 1.5f).
    void setGassing(float v_gas_per_cell, float k_gas) {
        _vGasCell = (isfinite(v_gas_per_cell) && v_gas_per_cell > 0.0f) ? v_gas_per_cell : 0.0f;
        _kGas     = (isfinite(k_gas) && k_gas > 0.0f) ? k_gas : 0.0f;
    }
    void clearGassing() { _vGasCell = 0.0f; _kGas = 0.0f; _gasState = 0.0f; }
    float getGassingOvervoltage() const { return _gasState; }   // V/cell

    // --- Peukert paramétrable + rendement variable — v1.6.5 (phase 7) ---
    // Exposant n runtime (NAN = défaut techno). 1.0 = désactivé.
    void setPeukertExponent(float n) {
        _peukertNOverride = (isfinite(n) && n >= 1.0f && n <= 2.0f) ? n : NAN;
    }
    void setPeukertReferenceRate(float iref_A) {
        _peukertIrefOverride = (isfinite(iref_A) && iref_A > 0.0f) ? iref_A : NAN;
    }
    float getPeukertExponent() const {
        return isnan(_peukertNOverride) ? getParams().k_peukert : _peukertNOverride;
    }
    // Rendement VARIABLE selon le courant (AC-7.3) : η(I) = η_base·(I_ref/|I|)^(n−1),
    // borné [0.5, 1]. À fort courant, une part de l'énergie est perdue (chaleur/gaz).
    float getChargeEfficiency(float current, float eta_base = 0.90f) const {
        return _variableEfficiency(eta_base, current);
    }
    float getDischargeEfficiency(float current, float eta_base = 1.00f) const {
        return _variableEfficiency(eta_base, current);
    }
    void setFloatCurrentMax(float min_A, float factor) {
        _detectOverride.float_current_min_A = min_A;
        _detectOverride.float_current_factor = factor;
        _configDirty = true;
    }
    void setOcvRestHours(float hours) {
        _detectOverride.ocv_rest_hours = hours;
        _configDirty = true;
    }
    void setCycleThreshold(float pct) {
        _detectOverride.cycle_thr_pct = pct;
        _configDirty = true;
    }

    // --- Overrides runtime KalmanTuning ---
    void setKalmanP(float init_C, float min_C, float init_aging, float min_aging) {
        _kalmanOverride.p_init_C = init_C;
        _kalmanOverride.p_min_C = min_C;
        _kalmanOverride.p_init_aging = init_aging;
        _kalmanOverride.p_min_aging = min_aging;
        _configDirty = true;
    }
    void setKalmanQ(float aging_uncertainty, float capacity_drift) {
        _kalmanOverride.q_aging_uncertainty = aging_uncertainty;
        _kalmanOverride.q_capacity_drift = capacity_drift;
        _configDirty = true;
    }
    void setKalmanR(float init, float min, float max, float smooth_alpha) {
        _kalmanOverride.r_init = init;
        _kalmanOverride.r_min = min;
        _kalmanOverride.r_max = max;
        _kalmanOverride.r_smooth_alpha = smooth_alpha;
        _configDirty = true;
    }
    void setKalmanSegmentThresholds(float min_dAh_abs, float min_dAh_pct,
                                    float dSoc_boot, float dSoc_coarse, float dSoc_track,
                                    float dSoc_high_conf, bool ocv_allowed) {
        _kalmanOverride.seg_min_dAh_abs = min_dAh_abs;
        _kalmanOverride.seg_min_dAh_pct = min_dAh_pct;
        _kalmanOverride.seg_min_dsoc[0] = dSoc_boot;
        _kalmanOverride.seg_min_dsoc[1] = dSoc_coarse;
        _kalmanOverride.seg_min_dsoc[2] = dSoc_track;
        _kalmanOverride.seg_dsoc_high_conf = dSoc_high_conf;
        _kalmanOverride.ocv_segment_allowed = ocv_allowed;
        _ocvSegmentAllowedSet = true;
        _configDirty = true;
    }
    void setKalmanBatteryChange(float thr, uint8_t count) {
        _kalmanOverride.battery_change_thr = thr;
        _kalmanOverride.battery_change_count = count;
        _configDirty = true;
    }
    void setKalmanRestLong(float min_minutes, float stable_mv, float stable_minutes,
                           uint8_t min_samples, uint8_t max_samples) {
        _kalmanOverride.rest_long_min_min = min_minutes;
        _kalmanOverride.rest_long_stable_mv = stable_mv;
        _kalmanOverride.rest_long_stable_min = stable_minutes;
        _kalmanOverride.rest_long_stable_min_samples = min_samples;
        _kalmanOverride.rest_long_stable_max_samples = max_samples;
        _configDirty = true;
    }
    void setKalmanConfidence(float conf_float, float conf_rest_base, float conf_rest_span,
                             float conf_phase_coarse, float conf_phase_refine) {
        _kalmanOverride.conf_float = conf_float;
        _kalmanOverride.conf_rest_base = conf_rest_base;
        _kalmanOverride.conf_rest_span = conf_rest_span;
        _kalmanOverride.conf_phase_coarse = conf_phase_coarse;
        _kalmanOverride.conf_phase_refine = conf_phase_refine;
        _configDirty = true;
    }

    // Réinitialise tous les overrides runtime → retour au défaut tech/macro
    void resetOverrides() {
        _detectOverride = overrideDetectionNone();
        _kalmanOverride = overrideKalmanNone();
        _ocvSegmentAllowedSet = false;
        _configDirty = true;
    }

    BatteryTech getTechnology() const { return tech; }

    const char* getTechnologyName() const {
        static char buf[40];
        uint8_t idx = min((uint8_t)tech, (uint8_t)(TECH_COUNT - 1));
        BATT_STRCPY_P(buf, &TECH_NAMES[idx]);
        buf[39] = '\0';
        return buf;
    }

    bool isAutoDetect() const { return auto_detect; }

    void setTechnology(BatteryTech t) { 
        tech = t; 
        _paramsCached = false;
        _configDirty = true;
    }

    void setCellCount(uint8_t cells) { 
        n_cells = max(cells, (uint8_t)1); 
    }

    void setNominalCapacity(float cap) { 
        C_nominal = max(cap, 0.0f); 
    }

    void setInternalResistance(float r_int) { 
        R_int_nominal = max(r_int, 0.0f); 
    }

    void setAutoDetect(bool enable) { auto_detect = enable; }

    BatteryTech runAutoDetect(float voltage, float current, float temperature) {
        if (auto_detect) {
            BatteryTech detected = autoDetectTech(voltage, current, temperature);
            if (detected != tech) {
                tech = detected;
                _paramsCached = false;
                _configDirty = true;
            }
        }
        return tech;
    }

    // ============================================================
    // SEUILS DE CHARGE (FLOAT, ABSORPTION, MIN, MAX)
    // ============================================================

    float getFloatVoltage() const { 
        return getParams().V_float_per_cell * n_cells;
    }

    float getAbsorptionVoltage() const { 
        return getParams().V_abs_per_cell * n_cells;
    }

    float getMinVoltage() const { 
        return getParams().V_min_per_cell * n_cells;
    }

    float getMaxVoltage() const { 
        return getParams().V_max_per_cell * n_cells;
    }

    // ============================================================
    // POLARISATION ET RESET
    // ============================================================

    float getPolarizationVoltage() const { return V_pol_state; }

    // ============================================================
    // PARAMÈTRES DE POLARISATION (v2.0.0 — exposés)
    // ============================================================
    // POURQUOI CES ACCESSOIRS EXISTENT
    // ---------------------------------
    // `correctVoltageToOCV()` calcule la polarisation avec la loi
    //     R_pol(SoC) = (Rpol0_mOhm/1000) · (1 + delta_pol · exp(−SoC/20))    [Ω]
    // où Rpol0_mOhm et delta_pol viennent de `TechParams`, jusqu'ici PRIVÉ.
    // Faute d'accesseur, le projet Coulométre recopiait ces deux constantes
    // EN DUR (main.cpp:1487-1489 : `RPOL0 = 4.0f`, `DELTA = 2.0f`). Toute
    // évolution du modèle plomb aurait laissé cette copie silencieusement
    // fausse, et l'estimateur de R0 aurait soustrait une polarisation erronée.
    //
    // Les accesseurs renvoient la valeur EFFECTIVE (après surcharge runtime).
    // ------------------------------------------------------------
    /** @return Rpol0 de la technologie courante (mΩ par cellule) */
    float getRpol0_mOhm() const { return getParams().Rpol0_mOhm; }

    /** @return delta_pol de la technologie courante (sans dimension, ≥ 0) */
    float getDeltaPol() const { return getParams().delta_pol; }

    /** @return Constante de temps de la polarisation à 25 °C (s) */
    float getTauPol_s() const { return getParams().tau_pol_s; }

    /**
     * @brief Résistance de polarisation R_pol(SoC) en mΩ/cellule.
     *
     * Reproduit EXACTEMENT la loi utilisée par `correctVoltageToOCV()` :
     *     R_pol = Rpol0 · (1 + delta_pol · exp(−SoC/20))
     *
     * @param soc_pct SoC en % (0…100)
     * @return R_pol en mΩ/cellule
     */
    float getRpol_mOhm(float soc_pct) const {
        const TechParams& tp = getParams();
        return tp.Rpol0_mOhm * (1.0f + tp.delta_pol * expf(-soc_pct / 20.0f));
    }

    void resetPolarization() { V_pol_state = 0.0f; }

    void resetState() {
        V_pol_state = 0.0f;
        _v2State = 0.0f;
        _gasState = 0.0f;
        last_current = 0.0f;
        accumulated_charge_Ah = 0.0f;
        last_charge_delta_Ah = 0.0f;
    }

    void fullReset() {
        resetState();
        cycle_count = 0;
        _paramsCached = false;
        _configDirty = true;
    }
};
