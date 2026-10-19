#ifndef BATTERY_MODELS_H
#define BATTERY_MODELS_H

// Version de la librairie (source UNIQUE — agrégée par src/liste_lib.h)
#define __BATTERYMODELS_H_VERSION__ "1.6.11"

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
            uint8_t idx = min((uint8_t)tech, (uint8_t)(TECH_COUNT - 1));
            BATT_MEMCPY_P(&_paramsCache, &TECH_PARAMS[idx], sizeof(TechParams));
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

#endif // BATTERY_MODELS_H