#ifndef BATTERY_MODELS_H
#define BATTERY_MODELS_H

/*
  BatteryModels v1.6.0 - Modèles paramétriques avancés pour batteries

  🔗 COMPATIBILITÉ : Cette bibliothèque est entièrement compatible avec BatteryKalman
  (https://github.com/Fo170/BatteryKalman) pour une fusion optimale d'estimations SoC
  via filtre de Kalman adaptatif. BatteryKalman v1.5.3 intègre les corrections
  (dont detectChargeState FLOAT) et s'appuie sur les modèles physiques de BatteryModels.
  Voir README.md pour plus de détails.

  NOUVEAU v1.4.0: seuils de détection (DetectionConfig) et réglages Kalman (KalmanTuning)
  configurables PAR TECHNOLOGIE, avec surcharge compile-time (macros BATTERYMODELS_*)
  et runtime (setters). Hiérarchie: setter runtime > macro > défaut de la technologie.
  API additive, 100% rétro-compatible avec v1.3.0.

  Cette bibliothèque fourrit les modèles physiques (OCV, Thévenin, Arrhenius thermique)
  que BatteryKalman utilise comme base pour son estimation de Kalman.
*/

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
// SURCHARGE COMPILE-TIME (OPTIONNELLE, GLOBALE) — v1.4.0
// ============================================================
// Toutes les macros sont optionnelles et GLOBALES : elles écrasent
// le réglage par défaut de la technologie si (et seulement si) elles
// sont définies par l'utilisateur avant #include. Par défaut = NAN
// ("non défini") → le défaut de la technologie s'applique.
// Hiérarchie de résolution : 1. setter runtime > 2. macro > 3. défaut tech.
#ifndef BATTERYMODELS_DEADBAND_A
#define BATTERYMODELS_DEADBAND_A         NAN
#endif
#ifndef BATTERYMODELS_FLOAT_CURRENT_MIN_A
#define BATTERYMODELS_FLOAT_CURRENT_MIN_A NAN
#endif
#ifndef BATTERYMODELS_FLOAT_CURRENT_FACTOR
#define BATTERYMODELS_FLOAT_CURRENT_FACTOR NAN
#endif
#ifndef BATTERYMODELS_V_TOL_PCT
#define BATTERYMODELS_V_TOL_PCT          NAN
#endif
#ifndef BATTERYMODELS_V_TOL_MIN_V
#define BATTERYMODELS_V_TOL_MIN_V        NAN
#endif
#ifndef BATTERYMODELS_OCV_REST_HOURS
#define BATTERYMODELS_OCV_REST_HOURS     NAN
#endif
#ifndef BATTERYMODELS_CYCLE_THR_PCT
#define BATTERYMODELS_CYCLE_THR_PCT      NAN
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
    TECH_LCO     = 15,   // LiCoO2 (Cobalt) - legacy smartphones/tablets
    TECH_NIZN    = 16,   // Nickel-Zinc (NiZn) - RC / amateur radio
    TECH_NICD    = 17,   // Nickel-Cadmium (NiCd) - legacy outils/RC
    TECH_PBC     = 18,   // Plomb-Carbone (PbC) - stop-start auto
    TECH_SUPERCAP = 19,  // Supercondensateur (EDLC) - tension lineaire
    TECH_COUNT   = 20
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
static const char TECH_NAME_15[] BATT_PROGMEM = "Li-Cobalt (LCO)";
static const char TECH_NAME_16[] BATT_PROGMEM = "Nickel-Zinc (NiZn)";
static const char TECH_NAME_17[] BATT_PROGMEM = "Nickel-Cadmium (NiCd)";
static const char TECH_NAME_18[] BATT_PROGMEM = "Plomb-Carbone (PbC)";
static const char TECH_NAME_19[] BATT_PROGMEM = "Supercondensateur (EDLC)";

static const char* const TECH_NAMES[TECH_COUNT] BATT_PROGMEM = {
    TECH_NAME_0, TECH_NAME_1, TECH_NAME_2, TECH_NAME_3,
    TECH_NAME_4, TECH_NAME_5, TECH_NAME_6, TECH_NAME_7,
    TECH_NAME_8, TECH_NAME_9, TECH_NAME_10, TECH_NAME_11, TECH_NAME_12,
    TECH_NAME_13, TECH_NAME_14, TECH_NAME_15, TECH_NAME_16,
    TECH_NAME_17, TECH_NAME_18, TECH_NAME_19
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

// ============================================================
// CONFIGURATION DE DETECTION PAR TECHNOLOGIE — NOUVEAU v1.4.0
// ============================================================
// Seuils de détection configurés par technologie (voir changements_a_effectuer_sur_BatteryModels.md §6.1).
struct DetectionConfig {
    float current_deadband_A;   // Dead-band courant rest/charge/décharge (A)
    float float_current_min_A;  // I_float_max = max(float_current_min_A, cap*float_current_factor)
    float float_current_factor; // Facteur capacité pour I_float_max (défaut 0.02)
    float v_tol_pct;            // Fraction de V_nom pour V_tol (défaut 0.015)
    float v_tol_min_V;          // Valeur plancher de V_tol (défaut 0.1 V)
    float ocv_rest_hours;       // Repos minimum avant mesure OCV (h)
    float cycle_thr_pct;        // Seuil de comptage cycle (fraction de C_nom, défaut 0.8)
};

// ============================================================
// REGLAGES KALMAN PAR TECHNOLOGIE — NOUVEAU v1.4.0
// ============================================================
// Jeu de paramètres Kalman (P/Q/R, segments ΔAh/ΔSoC, REST_LONG, confiance)
// que BatteryKalman consomme comme valeurs par défaut par technologie
// (voir changements_a_effectuer_sur_BatteryKalman.md). BatteryModels ne fait
// qu'EXPOSER ces valeurs — c'est BatteryKalman qui les consomme.
struct KalmanTuning {
    // --- P : covariance de l'erreur (état [C_hat, dC_dCycle]) ---
    float p_init_C;             // Variance initiale pour C (Ah²)
    float p_min_C;              // Variance minimale pour C (Ah²)
    float p_init_aging;         // Variance initiale pour dC/dCycle
    float p_min_aging;          // Variance minimale pour aging

    // --- Q : bruit de processus ---
    float q_aging_uncertainty;  // Incertitude sur le taux aging
    float q_capacity_drift;     // Dérive de capacité entre cycles

    // --- R : bruit de mesure (estimation en ligne) ---
    float r_init;               // R initial
    float r_min;                // R minimum
    float r_max;                // R maximum
    float r_smooth_alpha;       // Lissage exponentiel de R_estimate

    // --- Segments ΔAh / ΔSoC (apprentissage de capacité) ---
    float seg_min_dAh_abs;      // Seuil absolu minimal (Ah)
    float seg_min_dAh_pct;      // Seuil relatif minimal (% de C) — NOUVEAU
    float seg_min_dsoc[3];      // {bootstrap, coarse, refine/track} (% SoC)
    float seg_dsoc_high_conf;   // dSoC pour confiance élevée (%)
    bool  ocv_segment_allowed;  // false => pas de segment OCV (NiFe, Sodium)

    // --- Détection de batterie changée ---
    float   battery_change_thr;     // Écart relatif (% de C_hat)
    uint8_t battery_change_count;   // Nombre d'outliers consécutifs

    // --- Porte de stabilité REST_LONG (référence OCV fiable) ---
    float   rest_long_min_min;          // Repos minimum (min)
    float   rest_long_stable_mv;        // Écart tension crête toléré (V)
    float   rest_long_stable_min;       // Fenêtre de stabilité (min)
    uint8_t rest_long_stable_min_samples; // Nb min d'échantillons dans la fenêtre
    uint8_t rest_long_stable_max_samples; // Taille max de l'anneau

    // --- Confiance ---
    float conf_float;           // Confiance segment FLOAT (0.95)
    float conf_rest_base;       // Confiance REST_LONG de base (0.70)
    float conf_rest_span;       // Bonus de stabilité REST_LONG (0.15)
    float conf_phase_coarse;    // Seuil phase COARSE (0.3)
    float conf_phase_refine;    // Seuil phase REFINE (0.6)
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

    // ===== NEW v1.4.0: Detection & Kalman tuning =====
    DetectionConfig detect;      // Seuils de détection par technologie
    KalmanTuning    kalman;      // Réglages Kalman par technologie
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

// Li-Cobalt (LCO) - 3.7V nominal/cell (LiCoO2, legacy phones/tablets)
static const float OCV_LCO[9][2] BATT_PROGMEM = {
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

// ============================================================
// PARAMETRES PAR TECHNOLOGIE EN PROGMEM
// ============================================================
static const TechParams TECH_PARAMS[TECH_COUNT] BATT_PROGMEM = {
    // TECH_UNKNOWN (0) - Parametres moyens prudents
    {
        {0.005714f, 0.131086f, 1.982714f},
        10.0f, 0.012f, 0.5f, 3.0f, 1.5f, 3.5f, 1.20f, -0.0039f,
        2.30f, 2.45f, 1.75f, 2.50f, 10.0f, true,
        2500.0f, 0.02f, -5e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {500.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.010f, 4.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_FLOODED (1) - Plomb inonde (forte dependance SoC)
    // CORRIGÉ: V_float 2.30→2.37V (norme IEC 61427, permet charge complète)
    {
        {0.017143f, 0.109657f, 1.981743f},
        15.0f, 0.015f, 0.5f, 4.0f, 2.0f, 4.0f, 1.40f, -0.0037f,
        2.37f, 2.45f, 1.75f, 2.50f, 18.0f, true,
        3500.0f, 0.04f, -8e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {600.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.030f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_AGM (2) - Plomb AGM
    {
        {-0.001143f, 0.147943f, 1.984657f},
        8.0f, 0.008f, 0.5f, 2.0f, 1.0f, 2.5f, 1.10f, -0.0040f,
        2.30f, 2.45f, 1.80f, 2.45f, 15.0f, true,
        3200.0f, 0.03f, -7e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {500.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.025f, 5.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_GEL (3) - Plomb Gel
    // CORRIGÉ: V_float 2.30→2.37V (GEL strict sur voltage pour éviter migration électrolyte)
    {
        {-0.038857f, 0.198857f, 1.986743f},
        12.0f, 0.012f, 0.6f, 3.0f, 1.5f, 3.0f, 1.20f, -0.0040f,
        2.37f, 2.43f, 1.80f, 2.45f, 16.0f, true,
        3300.0f, 0.035f, -7e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {500.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.025f, 5.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_LIFEPO4 (4) - Lithium (faible dependance SoC)
    {
        {-0.400000f, 1.200000f, 2.800000f},
        5.0f, 0.003f, 0.3f, 1.0f, 0.5f, 2.0f, 1.05f, -0.0005f,
        3.45f, 3.65f, 2.80f, 3.65f, 6.0f, true,
        1500.0f, 0.008f, -2e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.010f, 0.1f, 2.0f, 0.8f},  // detect (plateau plat → repos OCV 2h)
        {300.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.008f, 8.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {5.0f, 8.0f, 10.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_LION (5) - Lithium Ion
    {
        {-0.800000f, 2.000000f, 3.000000f},
        3.0f, 0.005f, 0.3f, 0.8f, 0.5f, 2.0f, 1.02f, -0.0003f,
        4.10f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1800.0f, 0.009f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_LIPO (6) - Lithium Polymere
    {
        {-0.880000f, 2.080000f, 3.000000f},
        2.5f, 0.004f, 0.25f, 0.6f, 0.4f, 1.5f, 1.01f, -0.0002f,
        4.15f, 4.20f, 3.00f, 4.20f, 6.5f, true,
        1700.0f, 0.007f, -2e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_LTO (7) - Lithium Titanate
    {
        {-0.200000f, 1.100000f, 1.900000f},
        4.0f, 0.006f, 0.35f, 1.2f, 0.6f, 2.5f, 1.03f, -0.0010f,
        2.70f, 2.80f, 1.90f, 2.80f, 6.0f, true,
        1900.0f, 0.010f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.010f, 0.1f, 2.0f, 0.8f},  // detect (plateau plat → repos OCV 2h)
        {300.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.006f, 7.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {5.0f, 8.0f, 10.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_LMNO (8) - Lithium Manganese
    // CORRIGÉ: V_abs 4.30→4.20V (limite de structure spinelle Mn, >4.25V = perte O₂ et dissolution Mn)
    {
        {-0.600000f, 1.900000f, 3.000000f},
        3.5f, 0.005f, 0.3f, 0.9f, 0.5f, 2.0f, 1.02f, -0.0004f,
        4.15f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1800.0f, 0.009f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_NMC (9) - Nickel Manganese Cobalt
    {
        {-0.800000f, 2.000000f, 3.000000f},
        3.2f, 0.005f, 0.32f, 0.85f, 0.55f, 2.2f, 1.02f, -0.00035f,
        4.10f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1850.0f, 0.009f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_NCA (10) - Nickel Cobalt Aluminium
    {
        {-0.800000f, 2.000000f, 3.000000f},
        3.0f, 0.005f, 0.3f, 0.8f, 0.5f, 2.0f, 1.02f, -0.0003f,
        4.10f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1800.0f, 0.009f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_NIFE (11) - Nickel-Fer (Edison) (forte dependance)
    // CORRIGÉ: OCV polynomial recalibré (ancien était ~0.3V trop bas)
    // Nouvelle courbe: 1.0V (0% SoC) → 1.2V (50%) → 1.45V (100%)
    {
        {0.1f, 0.35f, 1.0f},
        20.0f, 0.008f, 0.4f, 5.0f, 2.0f, 5.0f, 1.15f, -0.0015f,
        1.45f, 1.55f, 1.10f, 1.60f, 12.0f, true,
        2800.0f, 0.05f, -5e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.020f, 0.1f, 1.0f, 0.8f},  // detect (OCV peu fiable → tolérance large)
        {800.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.030f, 12.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, false,
         0.30f, 3, 15.0f, 0.050f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman (pas de segment OCV)
    },
    // TECH_SODIUM (12) - Sodium Ion (faible dependance)
    {
        {-1.300000f, 3.250000f, 2.000000f},
        2.5f, 0.004f, 0.28f, 0.7f, 0.45f, 1.8f, 1.01f, -0.0003f,
        3.80f, 3.95f, 2.00f, 3.95f, 6.0f, true,
        1600.0f, 0.008f, -2e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {300.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.008f, 8.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, false,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman (pas de segment OCV)
    },
    // TECH_NIMH (13) - Nickel-Metal Hydride (Rechargeable AA/AAA) - NOUVEAU
    {
        {-0.350000f, 1.100000f, 1.400000f},
        6.0f, 0.004f, 0.4f, 2.5f, 1.2f, 3.0f, 1.15f, -0.0004f,
        1.40f, 1.50f, 0.90f, 1.55f, 8.0f, true,
        2600.0f, 0.025f, -4e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {400.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.015f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 1.0f, {4.0f, 6.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman (petite capacité → seuil ΔAh relatif 1%)
    },
    // TECH_ALKALINE (14) - Alkaline Primary (AA/AAA) - NOUVEAU
    // Mode: Read-only (décharge seule), no charging curve
    {
        {-0.200000f, 0.850000f, 1.600000f},
        8.0f, 0.005f, 0.3f, 2.0f, 0.8f, 2.0f, 1.10f, -0.0002f,
        1.50f, 1.55f, 0.90f, 1.55f, 6.0f, false,
        1700.0f, 0.015f, -2e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {400.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.010f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 1.0f, {4.0f, 6.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman (décharge seule, seuil ΔAh relatif 1%)
    },
    // TECH_LCO (15) - Li-Cobalt (LiCoO2) - legacy smartphones/tablets
    {
        {-0.800000f, 2.000000f, 3.000000f},
        4.0f, 0.004f, 0.3f, 0.8f, 0.5f, 2.0f, 1.02f, -0.0003f,
        4.10f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1800.0f, 0.009f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_NIZN (16) - Nickel-Zinc (NiZn) - RC / amateur radio
    {
        {-0.150000f, 0.850000f, 1.100000f},
        5.0f, 0.006f, 0.4f, 1.0f, 0.6f, 2.5f, 1.10f, -0.0004f,
        1.75f, 1.85f, 1.20f, 1.90f, 7.0f, true,
        2200.0f, 0.02f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {400.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.015f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 1.0f, {4.0f, 6.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_NICD (17) - Nickel-Cadmium (NiCd) - legacy outils/RC
    {
        {-0.200000f, 0.570000f, 1.050000f},
        6.0f, 0.008f, 0.4f, 1.5f, 0.8f, 3.0f, 1.30f, -0.0005f,
        1.40f, 1.45f, 1.00f, 1.55f, 9.0f, true,
        2500.0f, 0.03f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {400.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.015f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 1.0f, {4.0f, 6.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_PBC (18) - Plomb-Carbone (PbC) - stop-start auto
    {
        {-0.005000f, 0.140000f, 1.975000f},
        8.0f, 0.012f, 0.5f, 2.0f, 1.0f, 3.0f, 1.20f, -0.0037f,
        2.37f, 2.45f, 1.75f, 2.50f, 15.0f, true,
        3400.0f, 0.035f, -7e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {600.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.030f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
    },
    // TECH_SUPERCAP (19) - Supercondensateur (EDLC) - tension lineaire
    // Pas de plateau OCV : SoC base energie (V²). ESR seule (pas de polarisation RC).
    // V_max = tension nominale cellule (2.7V classique), V_min = 0.5 * V_max.
    {
        {0.0f, 0.0f, 0.0f},             // ocv poly inutilise (case dediee dans ocvToSoc)
        5.0f, 0.003f, 0.0f, 0.0f, 0.0f, 0.0f, 1.00f, 0.0f,
        2.70f, 2.70f, 1.35f, 2.70f, 0.0f, true,
        0.0f, 0.0f, 0.0f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {600.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.030f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
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

    // ===== NOUVEAU v1.4.0: overrides runtime (sentinelle NAN = "non surchargé") =====
    mutable DetectionConfig _detectOverride = overrideDetectionNone();
    mutable KalmanTuning    _kalmanOverride = overrideKalmanNone();
    mutable bool            _ocvSegmentAllowedSet = false;  // tri-state pour le bool ocv_segment_allowed
    mutable DetectionConfig _detectResolved;
    mutable KalmanTuning    _kalmanResolved;
    mutable bool            _configDirty = true;

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
            uint8_t idx = min((uint8_t)tech, (uint8_t)(TECH_COUNT - 1));
            BATT_MEMCPY_P(&_paramsCache, &TECH_PARAMS[idx], sizeof(TechParams));
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
        _configDirty = true;
    }
};

#endif // BATTERY_MODELS_H