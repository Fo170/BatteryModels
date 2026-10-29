// ============================================================
// tech/Alkaline.h — Alcaline primaire (AA/AAA)
// ============================================================
// Généré par fractionnement de BatteryModels.h v1.6.11 (cf. FORK.md).
// Les coefficients sont IDENTIQUES à l'amont — aucune valeur modifiée.
// ============================================================

#pragma once
#include "../BatteryModelsCore.h"

    // TECH_ALKALINE (14) - Alkaline Primary (AA/AAA) - NOUVEAU
    // Mode: Read-only (décharge seule), no charging curve
static const TechParams TECH_PARAMS_ALKALINE BATT_PROGMEM = {
        {-0.200000f, 0.850000f, 1.600000f},
        8.0f, 0.005f, 0.3f, 2.0f, 0.8f, 2.0f, 1.10f, -0.0002f,
        1.50f, 1.55f, 0.90f, 1.55f, 6.0f, false,
        1700.0f, 0.015f, -2e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {400.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.010f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 1.0f, {4.0f, 6.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman (décharge seule, seuil ΔAh relatif 1%)
};

