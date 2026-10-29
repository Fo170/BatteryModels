// ============================================================
// tech/NiFe.h — Nickel-Fer (Edison)
// ============================================================
// Généré par fractionnement de BatteryModels.h v1.6.11 (cf. FORK.md).
// Les coefficients sont IDENTIQUES à l'amont — aucune valeur modifiée.
// ============================================================

#pragma once
#include "../BatteryModelsCore.h"

    // TECH_NIFE (11) - Nickel-Fer (Edison) (forte dependance)
    // CORRIGÉ: OCV polynomial recalibré (ancien était ~0.3V trop bas)
    // Nouvelle courbe: 1.0V (0% SoC) → 1.2V (50%) → 1.45V (100%)
static const TechParams TECH_PARAMS_NIFE BATT_PROGMEM = {
        {0.1f, 0.35f, 1.0f},
        20.0f, 0.008f, 0.4f, 5.0f, 2.0f, 5.0f, 1.15f, -0.0015f,
        1.45f, 1.55f, 1.10f, 1.60f, 12.0f, true,
        2800.0f, 0.05f, -5e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.020f, 0.1f, 1.0f, 0.8f},  // detect (OCV peu fiable → tolérance large)
        {800.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.030f, 12.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, false,
         0.30f, 3, 15.0f, 0.050f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman (pas de segment OCV)
};

