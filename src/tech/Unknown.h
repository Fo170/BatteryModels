// ============================================================
// tech/Unknown.h — TECH_UNKNOWN — paramètres moyens prudents
// ============================================================
// Généré par fractionnement de BatteryModels.h v1.6.11 (cf. FORK.md).
// Les coefficients sont IDENTIQUES à l'amont — aucune valeur modifiée.
// ============================================================

#pragma once
#include "../BatteryModelsCore.h"

    // TECH_UNKNOWN (0) - Parametres moyens prudents
static const TechParams TECH_PARAMS_UNKNOWN BATT_PROGMEM = {
        {0.005714f, 0.131086f, 1.982714f},
        10.0f, 0.012f, 0.5f, 3.0f, 1.5f, 3.5f, 1.20f, -0.0039f,
        2.30f, 2.45f, 1.75f, 2.50f, 10.0f, true,
        2500.0f, 0.02f, -5e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {500.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.010f, 4.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

