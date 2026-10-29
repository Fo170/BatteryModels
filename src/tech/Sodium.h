// ============================================================
// tech/Sodium.h — Sodium-Ion (Na-ion)
// ============================================================
// Généré par fractionnement de BatteryModels.h v1.6.11 (cf. FORK.md).
// Les coefficients sont IDENTIQUES à l'amont — aucune valeur modifiée.
// ============================================================

#pragma once
#include "../BatteryModelsCore.h"

// ---- Table OCV (OCV_SODIUM) ----
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

    // TECH_SODIUM (12) - Sodium Ion (faible dependance)
static const TechParams TECH_PARAMS_SODIUM BATT_PROGMEM = {
        {-1.300000f, 3.250000f, 2.000000f},
        2.5f, 0.004f, 0.28f, 0.7f, 0.45f, 1.8f, 1.01f, -0.0003f,
        3.80f, 3.95f, 2.00f, 3.95f, 6.0f, true,
        1600.0f, 0.008f, -2e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {300.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.008f, 8.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, false,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman (pas de segment OCV)
};

