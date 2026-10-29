// ============================================================
// BatteryModelsCore.h — SOCLE COMMUN à toutes les technologies
// ============================================================
// Enums, macros PROGMEM, surcharges compile-time, et structures de
// paramètres (OcvPoly, DetectionConfig, KalmanTuning, TechParams).
//
// Ce fichier ne contient AUCUNE donnée de technologie : les tables OCV et
// les `TechParams` vivent dans tech/<Famille>.h. Cf. FORK.md.
// ============================================================

#ifndef BATTERY_MODELS_CORE_H
#define BATTERY_MODELS_CORE_H

// Version de la librairie (source UNIQUE — agrégée par src/liste_lib.h)
#define __BATTERYMODELS_H_VERSION__ "2.3.0"

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
// ⚠️ v2.1.0 (21/10/2026) — CORRECTION MAJEURE ESP8266.
//
// L'ancienne version ne distinguait que `__AVR__` et « tout le reste », avec ce
// commentaire : « ESP32, ESP8266, ARM Cortex — const en flash par défaut ».
// **C'est FAUX pour l'ESP8266.** Sur ESP8266, `const` atterrit dans `.rodata`,
// qui est placé en **DRAM** (donc en RAM) ; seul `PROGMEM`
// (= `__attribute__((section(".irom.text")))`) envoie les données en flash.
// Vérifié sur ce projet : `.rodata` à l'adresse 0x3FFE…, alors que l'IROM
// commence à 0x40200000.
//
// Conséquence mesurée : les 20 tables `TECH_PARAMS_*` (220 o chacune), les 20
// chaînes `TECH_NAME_*` et la table `TECH_NAMES` occupaient **4 869 octets de
// RAM** — sur un ESP8266 dont le tas libre était descendu à 3 480 o. Autrement
// dit : plus de RAM immobilisée par des tables quasi inutilisées que de tas
// disponible pour servir les réponses HTTP.
//
// Aucune valeur n'a changé : seule leur SECTION change. Les accès passent déjà
// par `BATT_MEMCPY_P` / `BATT_STRCPY_P` / `BATT_READ_FLOAT`, désormais
// correctement définis pour ESP8266.
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
#elif defined(ESP8266)
    // ESP8266 : `const` va en DRAM, SEUL `PROGMEM` va en flash.
    #include <pgmspace.h>
    #define BATT_PROGMEM PROGMEM
    #define BATT_READ_WORD(addr)   pgm_read_word(addr)
    #define BATT_READ_DWORD(addr)  pgm_read_dword(addr)
    #define BATT_READ_FLOAT(addr)  pgm_read_float(addr)
    #define BATT_READ_PTR(addr)    pgm_read_ptr(addr)
    #define BATT_STRCPY_P(dest, src)  strcpy_P(dest, (const char*)pgm_read_ptr(src))
    #define BATT_MEMCPY_P(dest, src, n) memcpy_P(dest, src, n)
#else
    // ESP32, ARM Cortex, hôte (PC) — const en flash par défaut, ou pas de flash.
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

#endif // BATTERY_MODELS_CORE_H
