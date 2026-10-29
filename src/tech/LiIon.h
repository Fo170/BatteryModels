// ============================================================
// tech/LiIon.h — Lithium-ion : LION, LIPO, LMNO, NMC, NCA, LCO
// ============================================================
// Généré par fractionnement de BatteryModels.h v1.6.11 (cf. FORK.md).
// Les coefficients sont IDENTIQUES à l'amont — aucune valeur modifiée.
// ============================================================

#pragma once
#include "../BatteryModelsCore.h"

// ---- Table OCV (OCV_LION) ----
static const float OCV_LION[9][2] BATT_PROGMEM = {
    {4.200f, 100.0f},
    {4.050f,  90.0f},
    {3.920f,  80.0f},
    {3.870f,  70.0f},
    {3.800f,  50.0f},
    {3.680f,  30.0f},
    {3.620f,  20.0f},
    {3.500f,  10.0f},
    {3.000f,   0.0f},
};

// ---- Table OCV (OCV_LIPO) ----
static const float OCV_LIPO[9][2] BATT_PROGMEM = {
    {4.200f, 100.0f},
    {4.100f,  95.0f},
    {3.950f,  85.0f},
    {3.880f,  70.0f},
    {3.820f,  50.0f},
    {3.750f,  30.0f},
    {3.650f,  15.0f},
    {3.450f,   5.0f},
    {3.000f,   0.0f},
};

// ---- Table OCV (OCV_LMNO) ----
static const float OCV_LMNO[9][2] BATT_PROGMEM = {
    {4.300f, 100.0f},
    {4.150f,  90.0f},
    {4.000f,  75.0f},
    {3.900f,  55.0f},
    {3.800f,  40.0f},
    {3.700f,  25.0f},
    {3.600f,  12.0f},
    {3.400f,   5.0f},
    {3.000f,   0.0f},
};

// ---- Table OCV (OCV_NMC) ----
static const float OCV_NMC[9][2] BATT_PROGMEM = {
    {4.200f, 100.0f},
    {4.070f,  90.0f},
    {3.950f,  75.0f},
    {3.880f,  55.0f},
    {3.800f,  40.0f},
    {3.700f,  25.0f},
    {3.600f,  12.0f},
    {3.500f,   5.0f},
    {3.000f,   0.0f},
};

// ---- Table OCV (OCV_NCA) ----
static const float OCV_NCA[9][2] BATT_PROGMEM = {
    {4.200f, 100.0f},
    {4.050f,  90.0f},
    {3.930f,  75.0f},
    {3.870f,  55.0f},
    {3.800f,  40.0f},
    {3.700f,  25.0f},
    {3.600f,  12.0f},
    {3.480f,   5.0f},
    {3.000f,   0.0f},
};

// ---- Table OCV (OCV_LCO) ----
static const float OCV_LCO[9][2] BATT_PROGMEM = {
    {4.200f, 100.0f},
    {4.100f,  95.0f},
    {3.950f,  85.0f},
    {3.880f,  70.0f},
    {3.820f,  50.0f},
    {3.750f,  30.0f},
    {3.650f,  15.0f},
    {3.450f,   5.0f},
    {3.000f,   0.0f},
};

    // TECH_LION (5) - Lithium Ion
static const TechParams TECH_PARAMS_LION BATT_PROGMEM = {
        {-0.800000f, 2.000000f, 3.000000f},
        3.0f, 0.005f, 0.3f, 0.8f, 0.5f, 2.0f, 1.02f, -0.0003f,
        4.10f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1800.0f, 0.009f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

    // TECH_LIPO (6) - Lithium Polymere
static const TechParams TECH_PARAMS_LIPO BATT_PROGMEM = {
        {-0.880000f, 2.080000f, 3.000000f},
        2.5f, 0.004f, 0.25f, 0.6f, 0.4f, 1.5f, 1.01f, -0.0002f,
        4.15f, 4.20f, 3.00f, 4.20f, 6.5f, true,
        1700.0f, 0.007f, -2e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

    // TECH_LMNO (8) - Lithium Manganese
    // CORRIGÉ: V_abs 4.30→4.20V (limite de structure spinelle Mn, >4.25V = perte O₂ et dissolution Mn)
static const TechParams TECH_PARAMS_LMNO BATT_PROGMEM = {
        {-0.600000f, 1.900000f, 3.000000f},
        3.5f, 0.005f, 0.3f, 0.9f, 0.5f, 2.0f, 1.02f, -0.0004f,
        4.15f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1800.0f, 0.009f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

    // TECH_NMC (9) - Nickel Manganese Cobalt
static const TechParams TECH_PARAMS_NMC BATT_PROGMEM = {
        {-0.800000f, 2.000000f, 3.000000f},
        3.2f, 0.005f, 0.32f, 0.85f, 0.55f, 2.2f, 1.02f, -0.00035f,
        4.10f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1850.0f, 0.009f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

    // TECH_NCA (10) - Nickel Cobalt Aluminium
static const TechParams TECH_PARAMS_NCA BATT_PROGMEM = {
        {-0.800000f, 2.000000f, 3.000000f},
        3.0f, 0.005f, 0.3f, 0.8f, 0.5f, 2.0f, 1.02f, -0.0003f,
        4.10f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1800.0f, 0.009f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

    // TECH_LCO (15) - Li-Cobalt (LiCoO2) - legacy smartphones/tablets
static const TechParams TECH_PARAMS_LCO BATT_PROGMEM = {
        {-0.800000f, 2.000000f, 3.000000f},
        4.0f, 0.004f, 0.3f, 0.8f, 0.5f, 2.0f, 1.02f, -0.0003f,
        4.10f, 4.20f, 3.00f, 4.20f, 7.0f, true,
        1800.0f, 0.009f, -3e-6f, // E_a_R, alpha_tau, d2Vocv_dT2
        {0.05f, 0.6f, 0.02f, 0.015f, 0.1f, 0.5f, 0.8f},  // detect
        {200.0f, 0.10f, 1e-6f, 1e-7f, 2e-6f, 0.005f, 3.0f, 0.5f, 100.0f, 0.1f,
         0.20f, 0.5f, {2.0f, 3.0f, 5.0f}, 15.0f, true,
         0.30f, 3, 15.0f, 0.020f, 30.0f, 6, 32,
         0.95f, 0.70f, 0.15f, 0.3f, 0.6f}  // kalman
};

