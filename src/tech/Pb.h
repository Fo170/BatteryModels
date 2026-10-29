// ============================================================
// tech/Pb.h — Technologies au PLOMB (FLOODED, AGM, GEL, PbC)
// ============================================================
// Généré par fractionnement de BatteryModels.h v1.6.11 (cf. FORK.md).
// Les coefficients sont IDENTIQUES à l'amont — aucune valeur modifiée.
// ============================================================

#pragma once
#include "../BatteryModelsCore.h"

    // TECH_FLOODED (1) - Plomb inonde (forte dependance SoC)
    // CORRIGÉ: V_float 2.30→2.37V (norme IEC 61427, permet charge complète)
static const TechParams TECH_PARAMS_FLOODED BATT_PROGMEM = {
        {0.017143f, 0.109657f, 1.981743f},
        15.0f, 0.015f, 0.5f, 4.0f, 2.0f, 4.0f, 1.40f, -0.0037f,
        2.37f, 2.45f, 1.75f, 2.50f, 18.0f, true,
        3500.0f, 0.04f, -8e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {600.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.030f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

    // TECH_AGM (2) - Plomb AGM
static const TechParams TECH_PARAMS_AGM BATT_PROGMEM = {
        {-0.001143f, 0.147943f, 1.984657f},
        8.0f, 0.008f, 0.5f, 2.0f, 1.0f, 2.5f, 1.10f, -0.0040f,
        2.30f, 2.45f, 1.80f, 2.45f, 15.0f, true,
        3200.0f, 0.03f, -7e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {500.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.025f, 5.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

    // TECH_GEL (3) - Plomb Gel
    // CORRIGÉ: V_float 2.30→2.37V (GEL strict sur voltage pour éviter migration électrolyte)
static const TechParams TECH_PARAMS_GEL BATT_PROGMEM = {
        {-0.038857f, 0.198857f, 1.986743f},
        12.0f, 0.012f, 0.6f, 3.0f, 1.5f, 3.0f, 1.20f, -0.0040f,
        2.37f, 2.43f, 1.80f, 2.45f, 16.0f, true,
        3300.0f, 0.035f, -7e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {500.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.025f, 5.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

    // TECH_PBC (18) - Plomb-Carbone (PbC) - stop-start auto
static const TechParams TECH_PARAMS_PBC BATT_PROGMEM = {
        {-0.005000f, 0.140000f, 1.975000f},
        8.0f, 0.012f, 0.5f, 2.0f, 1.0f, 3.0f, 1.20f, -0.0037f,
        2.37f, 2.45f, 1.75f, 2.50f, 15.0f, true,
        3400.0f, 0.035f, -7e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {600.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.030f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

