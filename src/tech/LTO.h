// ============================================================
// tech/LTO.h — Lithium Titanate (LTO)
// ============================================================
// Généré par fractionnement de BatteryModels.h v1.6.11 (cf. FORK.md).
// Les coefficients sont IDENTIQUES à l'amont — aucune valeur modifiée.
// ============================================================

#pragma once
#include "../BatteryModelsCore.h"

// ---- Table OCV (OCV_LTO) ----
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

    // TECH_LTO (7) - Lithium Titanate
static const TechParams TECH_PARAMS_LTO BATT_PROGMEM = {
        {-0.200000f, 1.100000f, 1.900000f},
        4.0f, 0.006f, 0.35f, 1.2f, 0.6f, 2.5f, 1.03f, -0.0010f,
        2.70f, 2.80f, 1.90f, 2.80f, 6.0f, true,
        1900.0f, 0.010f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.010f, 0.1f, 2.0f, 0.8f},  // detect (plateau plat → repos OCV 2h)
        {300.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.006f, 7.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {5.0f, 8.0f, 10.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

