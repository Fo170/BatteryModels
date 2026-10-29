// ============================================================
// tech/NiCd.h — Nickel-Cadmium (NiCd)
// ============================================================
// Généré par fractionnement de BatteryModels.h v1.6.11 (cf. FORK.md).
// Les coefficients sont IDENTIQUES à l'amont — aucune valeur modifiée.
// ============================================================

#pragma once
#include "../BatteryModelsCore.h"

    // TECH_NICD (17) - Nickel-Cadmium (NiCd) - legacy outils/RC
static const TechParams TECH_PARAMS_NICD BATT_PROGMEM = {
        {-0.200000f, 0.570000f, 1.050000f},
        6.0f, 0.008f, 0.4f, 1.5f, 0.8f, 3.0f, 1.30f, -0.0005f,
        1.40f, 1.45f, 1.00f, 1.55f, 9.0f, true,
        2500.0f, 0.03f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {400.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.015f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 1.0f, {4.0f, 6.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

