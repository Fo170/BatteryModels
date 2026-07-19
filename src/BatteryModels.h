#ifndef BATTERY_MODELS_H
#define BATTERY_MODELS_H

#include <Arduino.h>
#include <math.h>

// ============================================================
// GESTION PROGMEM CROSS-PLATFORM
// ============================================================
#ifdef __AVR__
    #include <avr/pgmspace.h>
    #define BATT_PROGMEM PROGMEM
    #define BATT_READ_WORD(addr)   pgm_read_word(addr)
    #define BATT_READ_DWORD(addr)  pgm_read_dword(addr)
    #define BATT_READ_FLOAT(addr)  pgm_read_float(addr)
    #define BATT_READ_PTR(addr)    pgm_read_ptr(addr)
    #define BATT_STRCPY_P(dest, src)  strcpy_P(dest, (const char*)pgm_read_ptr(src))
    #define BATT_MEMCPY_P(dest, src, n) memcpy_P(dest, src, n)
#else
    // ESP32, ESP8266, ARM Cortex — const en flash par défaut
    #define BATT_PROGMEM
    #define BATT_READ_WORD(addr)   (*(const uint16_t*)(addr))
    #define BATT_READ_DWORD(addr)  (*(const uint32_t*)(addr))
    #define BATT_READ_FLOAT(addr)  (*(const float*)(addr))
    #define BATT_READ_PTR(addr)    (*(const void* const*)(addr))
    #define BATT_STRCPY_P(dest, src)  strcpy(dest, *(const char* const*)(src))
    #define BATT_MEMCPY_P(dest, src, n) memcpy(dest, src, n)
#endif

// ============================================================
// TYPES DE BATTERIE
// ============================================================
enum BatteryTech : uint8_t {
    TECH_UNKNOWN = 0,
    TECH_FLOODED = 1,
    TECH_AGM     = 2,
    TECH_GEL     = 3,
    TECH_LIFEPO4 = 4,
    TECH_LION    = 5,
    TECH_LIPO    = 6,
    TECH_LTO     = 7,
    TECH_LMNO    = 8,
    TECH_NMC     = 9,
    TECH_NCA     = 10,
    TECH_NIFE    = 11,
    TECH_SODIUM  = 12,
    TECH_NIMH    = 13,
    TECH_ALKALINE = 14,
    TECH_COUNT   = 15
};

// ============================================================
// NOMS EN PROGMEM
// ============================================================
static const char TECH_NAME_0[]  BATT_PROGMEM = "Inconnu";
static const char TECH_NAME_1[]  BATT_PROGMEM = "Plomb Inonde (BCI)";
static const char TECH_NAME_2[]  BATT_PROGMEM = "Plomb AGM";
static const char TECH_NAME_3[]  BATT_PROGMEM = "Plomb Gel";
static const char TECH_NAME_4[]  BATT_PROGMEM = "LiFePO4 (LFP)";
static const char TECH_NAME_5[]  BATT_PROGMEM = "Li-Ion (NMC/NCA)";
static const char TECH_NAME_6[]  BATT_PROGMEM = "Li-Polymere (LiPo)";
static const char TECH_NAME_7[]  BATT_PROGMEM = "Li-Titanate (LTO)";
static const char TECH_NAME_8[]  BATT_PROGMEM = "Li-Manganese (LMNO)";
static const char TECH_NAME_9[]  BATT_PROGMEM = "NMC (Nickel Manganese Cobalt)";
static const char TECH_NAME_10[] BATT_PROGMEM = "NCA (Nickel Cobalt Aluminium)";
static const char TECH_NAME_11[] BATT_PROGMEM = "Nickel-Fer (NiFe/Edison)";
static const char TECH_NAME_12[] BATT_PROGMEM = "Sodium-Ion (Na-ion)";
static const char TECH_NAME_13[] BATT_PROGMEM = "Nickel-Metal Hydride (NiMH)";
static const char TECH_NAME_14[] BATT_PROGMEM = "Alkaline (Primary)";

static const char* const TECH_NAMES[TECH_COUNT] BATT_PROGMEM = {
    TECH_NAME_0, TECH_NAME_1, TECH_NAME_2, TECH_NAME_3,
    TECH_NAME_4, TECH_NAME_5, TECH_NAME_6, TECH_NAME_7,
    TECH_NAME_8, TECH_NAME_9, TECH_NAME_10, TECH_NAME_11, TECH_NAME_12,
    TECH_NAME_13, TECH_NAME_14
};

// ============================================================
// ETATS DE CHARGE
// ============================================================
enum ChargeState : uint8_t {
    State_UNKNOWN    = 0,
    State_DISCHARGE  = 1,
    State_BULK       = 2,
    State_ABSORPTION = 3,
    State_FLOAT      = 4,
    State_REST       = 5,
    State_REST_LONG  = 6
};

static const char ETAT_NAME_0[] BATT_PROGMEM = "? Inconnu";
static const char ETAT_NAME_1[] BATT_PROGMEM = "Decharge";
static const char ETAT_NAME_2[] BATT_PROGMEM = "Charge Bulk";
static const char ETAT_NAME_3[] BATT_PROGMEM = "Absorption";
static const char ETAT_NAME_4[] BATT_PROGMEM = "Float (100%)";
static const char ETAT_NAME_5[] BATT_PROGMEM = "Repos";
static const char ETAT_NAME_6[] BATT_PROGMEM = "Repos long";

static const char* const ETAT_NAMES[7] BATT_PROGMEM = {
    ETAT_NAME_0, ETAT_NAME_1, ETAT_NAME_2, ETAT_NAME_3,
    ETAT_NAME_4, ETAT_NAME_5, ETAT_NAME_6
};

// ============================================================
// PARAMETRES MODELE THEVENIN
// ============================================================
struct OcvPoly {
    float A, B, C;
};

struct TechParams {
    OcvPoly ocv;
    float   R25_mOhm;
    float   beta_temp;           // Linear coeff: R_T = R25*(1 + beta*(T-25))
    float   gamma_soc;
    float   Rpol0_mOhm;
    float   delta_pol;
    float   tau_pol_s;           // RC time constant at 25°C (seconds)
    float   k_peukert;
    float   dVocv_dT;            // Primary OCV/T coeff (V/°C)
    float   V_float_per_cell;
    float   V_abs_per_cell;
    float   V_min_per_cell;
    float   V_max_per_cell;
    float   R_soc_max;
    bool    is_rechargeable;

    // ===== NEW v1.2: Advanced Thermal Models =====
    float   E_a_R;               // Activation energy for resistance (J/mol)
                                 // 0 = use linear model; >0 = use Arrhenius
    float   alpha_tau;           // Thermal coeff for tau_pol: tau(T) = tau25*exp(alpha*(T-25))
                                 // 0 = constant; typical 0.01-0.05
    float   d2Vocv_dT2;          // Quadratic OCV/T coeff (V/°C²)
                                 // Refines V(T) = V0 + dVocv_dT*ΔT + d2Vocv_dT2*ΔT²
};

// ============================================================
// TABLES OCV EN PROGMEM
// ============================================================

// LiFePO4 (LFP) - 3.2V nominal/cell
static const float OCV_LIFEPO4[7][2] BATT_PROGMEM = {
    {3.600f, 100.0f},
    {3.450f,  90.0f},
    {3.350f,  75.0f},
    {3.300f,  50.0f},
    {3.250f,  25.0f},
    {3.100f,  10.0f},
    {2.800f,   0.0f},
};

// Li-Ion NMC/NCA generique - 3.6V nominal/cell
static const float OCV_LION[9][2] BATT_PROGMEM = {
    {4.200f, 100.0f},
    {4.050f,  90.0f},
    {3.920f,  80.0f},
    {3.870f,  70.0f},
    {3.800f,  50.0f},
    {3.680f,  30.0f},
    {3.620f,  20.0f},
    {3.500f,  10.0f},
    {3.000f,   0.0f},
};

// Li-Polymere (LiPo) - 3.7V nominal/cell
static const float OCV_LIPO[9][2] BATT_PROGMEM = {
    {4.200f, 100.0f},
    {4.100f,  95.0f},
    {3.950f,  85.0f},
    {3.880f,  70.0f},
    {3.820f,  50.0f},
    {3.750f,  30.0f},
    {3.650f,  15.0f},
    {3.450f,   5.0f},
    {3.000f,   0.0f},
};

// LTO (Lithium Titanate) - 2.4V nominal/cell
static const float OCV_LTO[9][2] BATT_PROGMEM = {
    {2.800f, 100.0f},
    {2.700f,  90.0f},
    {2.600f,  75.0f},
    {2.500f,  60.0f},
    {2.400f,  45.0f},
    {2.300f,  30.0f},
    {2.200f,  15.0f},
    {2.100f,   5.0f},
    {1.900f,   0.0f},
};

// LMNO (Lithium Manganese) - 3.7V nominal/cell
static const float OCV_LMNO[9][2] BATT_PROGMEM = {
    {4.300f, 100.0f},
    {4.150f,  90.0f},
    {4.000f,  75.0f},
    {3.900f,  55.0f},
    {3.800f,  40.0f},
    {3.700f,  25.0f},
    {3.600f,  12.0f},
    {3.400f,   5.0f},
    {3.000f,   0.0f},
};

// NMC specifique
static const float OCV_NMC[9][2] BATT_PROGMEM = {
    {4.200f, 100.0f},
    {4.070f,  90.0f},
    {3.950f,  75.0f},
    {3.880f,  55.0f},
    {3.800f,  40.0f},
    {3.700f,  25.0f},
    {3.600f,  12.0f},
    {3.500f,   5.0f},
    {3.000f,   0.0f},
};

// NCA specifique
static const float OCV_NCA[9][2] BATT_PROGMEM = {
    {4.200f, 100.0f},
    {4.050f,  90.0f},
    {3.930f,  75.0f},
    {3.870f,  55.0f},
    {3.800f,  40.0f},
    {3.700f,  25.0f},
    {3.600f,  12.0f},
    {3.480f,   5.0f},
    {3.000f,   0.0f},
};

// Sodium-Ion (Na-ion) - 3.0V nominal/cell
static const float OCV_SODIUM[9][2] BATT_PROGMEM = {
    {3.95f, 100.0f},
    {3.85f,  90.0f},
    {3.70f,  75.0f},
    {3.50f,  55.0f},
    {3.30f,  40.0f},
    {3.10f,  25.0f},
    {2.90f,  12.0f},
    {2.50f,   5.0f},
    {2.00f,   0.0f},
};

// ============================================================
// PARAMETRES PAR TECHNOLOGIE EN PROGMEM
// ============================================================
static const TechParams TECH_PARAMS[TECH_COUNT] BATT_PROGMEM = {
    // TECH_UNKNOWN (0) - Parametres moyens prudents
    {
        {0.005714f, 0.131086f, 1.982714f},
        10.0f, 0.012f, 0.5f, 3.0f, 1.5f, 3.5f, 1.20f, -0.0039f,
        2.30f, 2.45f, 1.75f, 2.50f, 10.0f, true,
        2500.0f, 0.02f, -5e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_FLOODED (1) - Plomb inonde (forte dependance SoC)
    // CORRIGÉ: V_float 2.30→2.37V (norme IEC 61427, permet charge complète)
    {
        {0.017143f, 0.109657f, 1.981743f},
        15.0f, 0.015f, 0.5f, 4.0f, 2.0f, 4.0f, 1.40f, -0.0037f,
        2.37f, 2.45f, 1.75f, 2.50f, 18.0f, true,
        3500.0f, 0.04f, -8e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_AGM (2) - Plomb AGM
    {
        {-0.001143f, 0.147943f, 1.984657f},
        8.0f, 0.008f, 0.5f, 2.0f, 1.0f, 2.5f, 1.10f, -0.0040f,
        2.30f, 2.45f, 1.80f, 2.45f, 15.0f, true,
        3200.0f, 0.03f, -7e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_GEL (3) - Plomb Gel
    // CORRIGÉ: V_float 2.30→2.37V (GEL strict sur voltage pour éviter migration électrolyte)
    {
        {-0.038857f, 0.198857f, 1.986743f},
        12.0f, 0.012f, 0.6f, 3.0f, 1.5f, 3.0f, 1.20f, -0.0040f,
        2.37f, 2.43f, 1.80f, 2.45f, 16.0f, true,
        3300.0f, 0.035f, -7e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_LIFEPO4 (4) - Lithium (faible dependance SoC)
    {
        {-0.400000f, 1.200000f, 2.800000f},
        5.0f, 0.003f, 0.3f, 1.0f, 0.5f, 2.0f, 1.05f, -0.0005f,
        3.45f, 3.65f, 2.80f, 3.65f, 6.0f, true,
        1500.0f, 0.008f, -2e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_LION (5) - Lithium Ion
    {
        {-0.800000f, 2.000000f, 3.000000f},
        3.0f, 0.005f, 0.3f, 0.8f, 0.5f, 2.0f, 1.02f, -0.0003f,
        4.10f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1800.0f, 0.009f, -3e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_LIPO (6) - Lithium Polymere
    {
        {-0.880000f, 2.080000f, 3.000000f},
        2.5f, 0.004f, 0.25f, 0.6f, 0.4f, 1.5f, 1.01f, -0.0002f,
        4.15f, 4.20f, 3.00f, 4.20f, 6.5f, true,
        1700.0f, 0.007f, -2e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_LTO (7) - Lithium Titanate
    {
        {-0.200000f, 1.100000f, 1.900000f},
        4.0f, 0.006f, 0.35f, 1.2f, 0.6f, 2.5f, 1.03f, -0.0010f,
        2.70f, 2.80f, 1.90f, 2.80f, 6.0f, true,
        1900.0f, 0.010f, -3e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_LMNO (8) - Lithium Manganese
    // CORRIGÉ: V_abs 4.30→4.20V (limite de structure spinelle Mn, >4.25V = perte O₂ et dissolution Mn)
    {
        {-0.600000f, 1.900000f, 3.000000f},
        3.5f, 0.005f, 0.3f, 0.9f, 0.5f, 2.0f, 1.02f, -0.0004f,
        4.15f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1800.0f, 0.009f, -3e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_NMC (9) - Nickel Manganese Cobalt
    {
        {-0.800000f, 2.000000f, 3.000000f},
        3.2f, 0.005f, 0.32f, 0.85f, 0.55f, 2.2f, 1.02f, -0.00035f,
        4.10f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1850.0f, 0.009f, -3e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_NCA (10) - Nickel Cobalt Aluminium
    {
        {-0.800000f, 2.000000f, 3.000000f},
        3.0f, 0.005f, 0.3f, 0.8f, 0.5f, 2.0f, 1.02f, -0.0003f,
        4.10f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1800.0f, 0.009f, -3e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_NIFE (11) - Nickel-Fer (Edison) (forte dependance)
    // CORRIGÉ: OCV polynomial recalibré (ancien était ~0.3V trop bas)
    // Nouvelle courbe: 1.0V (0% SoC) → 1.2V (50%) → 1.45V (100%)
    {
        {0.1f, 0.35f, 1.0f},
        20.0f, 0.008f, 0.4f, 5.0f, 2.0f, 5.0f, 1.15f, -0.0015f,
        1.45f, 1.55f, 1.10f, 1.60f, 12.0f, true,
        2800.0f, 0.05f, -5e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_SODIUM (12) - Sodium Ion (faible dependance)
    {
        {-1.300000f, 3.250000f, 2.000000f},
        2.5f, 0.004f, 0.28f, 0.7f, 0.45f, 1.8f, 1.01f, -0.0003f,
        3.80f, 3.95f, 2.00f, 3.95f, 6.0f, true,
        1600.0f, 0.008f, -2e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_NIMH (13) - Nickel-Metal Hydride (Rechargeable AA/AAA) - NOUVEAU
    {
        {-0.350000f, 1.100000f, 1.400000f},
        6.0f, 0.004f, 0.4f, 2.5f, 1.2f, 3.0f, 1.15f, -0.0004f,
        1.40f, 1.50f, 0.90f, 1.55f, 8.0f, true,
        2600.0f, 0.025f, -4e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
    // TECH_ALKALINE (14) - Alkaline Primary (AA/AAA) - NOUVEAU
    // Mode: Read-only (décharge seule), no charging curve
    {
        {-0.200000f, 0.850000f, 1.600000f},
        8.0f, 0.005f, 0.3f, 2.0f, 0.8f, 2.0f, 1.10f, -0.0002f,
        1.50f, 1.55f, 0.90f, 1.55f, 6.0f, false,
        1700.0f, 0.015f, -2e-6f  // E_a_R, alpha_tau, d2Vocv_dT2
    },
};

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

    // Lecture PROGMEM d'un float dans une table 2D
    static inline float readTableFloat(const float (*tbl)[2], int row, int col) {
        return BATT_READ_FLOAT(&tbl[row][col]);
    }

    // Recuperation parametres avec cache
    const TechParams& getParams() const {
        if (!_paramsCached) {
            uint8_t idx = min((uint8_t)tech, (uint8_t)(TECH_COUNT - 1));
            BATT_MEMCPY_P(&_paramsCache, &TECH_PARAMS[idx], sizeof(TechParams));
            _paramsCached = true;
        }
        return _paramsCache;
    }

    float calculateRsocWithSaturation(float s_ref, float gamma_soc, float max_ratio) const {
        if (s_ref <= 0.0f) return max_ratio;
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
            default:
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
            default:
                V_cell = ocv_from_soc_poly(getParams().ocv, soc);
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

    float correctVoltageToOCV(float voltage, float current, 
                              float temperature, float dt_s,
                              float* r_int_eff = nullptr,
                              float* v_pol = nullptr) {
        if (n_cells == 0) return voltage;

        const TechParams& tp = getParams();
        float V_cell = voltage / n_cells;

        float R25 = (R_int_nominal > 0.0f) ? R_int_nominal : tp.R25_mOhm;
        float R_T = getResistanceAtTemp(R25, temperature);
        R_T = max(R_T, 0.001f);

        float soc_est = ocvToSoc(voltage, temperature);
        float s_ref = soc_est / 100.0f;

        float R_soc = calculateRsocWithSaturation(s_ref, tp.gamma_soc, tp.R_soc_max);

        float R_int = R_T * R_soc / 1000.0f;

        float R_pol = (tp.Rpol0_mOhm / 1000.0f)
                      * (1.0f + tp.delta_pol * expf(-soc_est / 20.0f));

        float V_pol_inf = current * R_pol;

        if (dt_s > 0.0f && tp.tau_pol_s > 0.0f) {
            float tau_thermal = getTauPol_thermal(temperature);
            float alpha_rc = 1.0f - expf(-dt_s / tau_thermal);
            alpha_rc = constrain(alpha_rc, 0.0f, 1.0f);
            V_pol_state += alpha_rc * (V_pol_inf - V_pol_state);
        } else if (dt_s < 0.0f) {
            V_pol_state = 0.0f;
        }

        float V_drop_ohm = R_int * current;
        float V_corrected = V_cell - (V_drop_ohm + V_pol_state);
        V_corrected = applyThermalCorrectionOCV(V_corrected, temperature);

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
        return max(0.1f, V_nom * 0.015f);
    }

    ChargeState detectChargeState(float voltage, float current,
                                   float capacity = -1.0f) const {
        const TechParams& tp = getParams();

        float cap = (capacity > 0.0f) ? capacity : C_nominal;
        if (cap <= 0.0f) cap = 100.0f;

        float V_float_ref = tp.V_float_per_cell * n_cells;
        float V_abs_ref = tp.V_abs_per_cell * n_cells;
        float I_float_max = max(0.6f, cap * 0.02f);
        float V_tol = getVoltageToleranceBand();

        bool in_rest = (fabsf(current) < 0.05f);
        bool charging = (current > 0.05f);
        bool discharging = (current < -0.05f);

        bool near_abs = (voltage >= V_abs_ref - V_tol) && (voltage <= V_abs_ref + V_tol);
        bool near_float = (voltage >= V_float_ref - V_tol) && (voltage <= V_float_ref + V_tol * 0.5f);

        if (discharging) return State_DISCHARGE;
        if (in_rest) return State_REST;
        if (near_float && fabsf(current) < I_float_max) return State_FLOAT;
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
        if (n_cells == 0) return 1.0f;
        switch (tech) {
            case TECH_FLOODED:
            case TECH_AGM:
            case TECH_GEL:
            case TECH_NIFE:
                return C_nominal / 20.0f;
            default:
                return C_nominal;
        }
    }

    float getEffectiveCapacity(float current) const {
        const TechParams& tp = getParams();
        if (fabsf(current) < 0.01f || tp.k_peukert <= 1.0f) {
            return C_nominal;
        }
        float I_abs = fabsf(current);
        float I_nom = getPeukertReferenceRate();
        float peukert_factor = powf(I_nom / I_abs, tp.k_peukert - 1.0f);
        return C_nominal * peukert_factor;
    }

    void updateCoulombCount(float current, float dt_hours) {
        if (C_nominal <= 0.0f || dt_hours <= 0.0f) return;

        float C_eff = getEffectiveCapacity(current);
        float peukert_ratio = C_nominal / C_eff;
        last_charge_delta_Ah = current * dt_hours * peukert_ratio;
        accumulated_charge_Ah += last_charge_delta_Ah;

        if (fabsf(accumulated_charge_Ah) >= C_nominal * 0.8f) {
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
        if (fabsf(current) > 0.05f) return false;
        if (dt_rest_hours < 0.5f && tech != TECH_LIFEPO4) return false;
        if (dt_rest_hours < 2.0f && tech == TECH_LIFEPO4) return false;
        return true;
    }

    // ============================================================
    // GETTERS / SETTERS
    // ============================================================

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

    void resetPolarization() { V_pol_state = 0.0f; }

    void resetState() {
        V_pol_state = 0.0f;
        last_current = 0.0f;
        accumulated_charge_Ah = 0.0f;
        last_charge_delta_Ah = 0.0f;
    }

    void fullReset() {
        resetState();
        cycle_count = 0;
        _paramsCached = false;
    }
};

#endif // BATTERY_MODELS_H