// ============================================================
// tech/LiFePO4.h — LiFePO4 (LFP) — 3,2 V nominal/cellule
// ============================================================
// Généré par fractionnement de BatteryModels.h v1.6.11 (cf. FORK.md).
// Les coefficients sont IDENTIQUES à l'amont — aucune valeur modifiée.
// ============================================================

#pragma once
#include "../BatteryModelsCore.h"

// ---- Table OCV (OCV_LIFEPO4) ----
static const float OCV_LIFEPO4[7][2] BATT_PROGMEM = {
    {3.600f, 100.0f},
    {3.450f,  90.0f},
    {3.350f,  75.0f},
    {3.300f,  50.0f},
    {3.250f,  25.0f},
    {3.100f,  10.0f},
    {2.800f,   0.0f},
};

    // TECH_LIFEPO4 (4) - Lithium (faible dependance SoC)
static const TechParams TECH_PARAMS_LIFEPO4 BATT_PROGMEM = {
        {-0.400000f, 1.200000f, 2.800000f},
        5.0f, 0.003f, 0.3f, 1.0f, 0.5f, 2.0f, 1.05f, -0.0005f,
        3.45f, 3.65f, 2.80f, 3.65f, 6.0f, true,
        1500.0f, 0.008f, -2e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.010f, 0.1f, 2.0f, 0.8f},  // detect (plateau plat → repos OCV 2h)
        {300.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.008f, 8.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {5.0f, 8.0f, 10.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

