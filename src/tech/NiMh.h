// ============================================================
// tech/NiMh.h — Nickel-Metal Hydride (NiMH)
// ============================================================
// Généré par fractionnement de BatteryModels.h v1.6.11 (cf. FORK.md).
// Les coefficients sont IDENTIQUES à l'amont — aucune valeur modifiée.
// ============================================================

#pragma once
#include "../BatteryModelsCore.h"

    // TECH_NIMH (13) - Nickel-Metal Hydride (Rechargeable AA/AAA) - NOUVEAU
static const TechParams TECH_PARAMS_NIMH BATT_PROGMEM = {
        {-0.350000f, 1.100000f, 1.400000f},
        6.0f, 0.004f, 0.4f, 2.5f, 1.2f, 3.0f, 1.15f, -0.0004f,
        1.40f, 1.50f, 0.90f, 1.55f, 8.0f, true,
        2600.0f, 0.025f, -4e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {400.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.015f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 1.0f, {4.0f, 6.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman (petite capacité → seuil ΔAh relatif 1%)
};

