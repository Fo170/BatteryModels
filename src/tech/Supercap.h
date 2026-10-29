// ============================================================
// tech/Supercap.h — Supercondensateur (EDLC)
// ============================================================
// Généré par fractionnement de BatteryModels.h v1.6.11 (cf. FORK.md).
// Les coefficients sont IDENTIQUES à l'amont — aucune valeur modifiée.
// ============================================================

#pragma once
#include "../BatteryModelsCore.h"

    // TECH_SUPERCAP (19) - Supercondensateur (EDLC) - tension lineaire
    // Pas de plateau OCV : SoC base energie (V²). ESR seule (pas de polarisation RC).
    // V_max = tension nominale cellule (2.7V classique), V_min = 0.5 * V_max.
static const TechParams TECH_PARAMS_SUPERCAP BATT_PROGMEM = {
        {0.0f, 0.0f, 0.0f},             // ocv poly inutilise (case dediee dans ocvToSoc)
        5.0f, 0.003f, 0.0f, 0.0f, 0.0f, 0.0f, 1.00f, 0.0f,
        2.70f, 2.70f, 1.35f, 2.70f, 0.0f, true,
        0.0f, 0.0f, 0.0f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 1.0f, 0.8f},  // detect
        {600.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.030f, 6.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {3.0f, 5.0f, 8.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.030f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

