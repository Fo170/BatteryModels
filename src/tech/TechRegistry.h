// ============================================================
// tech/TechRegistry.h — Correspondance NOMINATIVE enum → paramètres
// ============================================================
// Remplace l'indexation par tableau `TECH_PARAMS[TECH_COUNT]` de la v1.6.11.
//
// POURQUOI CE CHANGEMENT
// ----------------------
// L'ancien tableau était indexé par la valeur de l'enum `BatteryTech`. Ajouter
// une technologie obligeait à INSÉRER l'entrée au bon rang : une erreur de rang
// décalait SILENCIEUSEMENT toutes les technologies suivantes (aucune erreur de
// compilation, aucune erreur d'exécution — juste des paramètres faux).
//
// Ici chaque technologie est nommée (`TECH_PARAMS_FLOODED`, `TECH_PARAMS_AGM`…)
// et le `switch` est nominatif : une technologie oubliée tombe sur `default`,
// jamais sur le voisin de tableau.
//
// POURQUOI UN `switch` ET PAS UN TABLEAU DE POINTEURS
// ---------------------------------------------------
// Sur AVR, un tableau de pointeurs vers PROGMEM exigerait `pgm_read_ptr()` à
// chaque accès, ce qui casserait la sémantique `const TechParams&` renvoyée par
// `BatteryModel::getParams()`. Le `switch` n'est évalué qu'UNE fois (le résultat
// est mémorisé dans `_paramsCache`) : coût CPU nul en régime établi.
// ============================================================

#pragma once

#include "../BatteryModelsCore.h"
#include "Unknown.h"
#include "Pb.h"
#include "LiFePO4.h"
#include "LiIon.h"
#include "LTO.h"
#include "Sodium.h"
#include "NiFe.h"
#include "NiMh.h"
#include "NiCd.h"
#include "NiZn.h"
#include "Alkaline.h"
#include "Supercap.h"

/**
 * @brief Copie les paramètres PROGMEM de la technologie `t` dans `dst`.
 *
 * Toute valeur non reconnue (ou `TECH_COUNT`) reçoit les paramètres
 * `TECH_UNKNOWN` — comportement identique à l'ancien
 * `idx = min(tech, TECH_COUNT-1)` mais SANS dépendre de l'ordre du tableau.
 *
 * @param t   Technologie demandée
 * @param dst Destination (RAM)
 */
inline void techParamsCopy(BatteryTech t, TechParams& dst) {
    switch (t) {
        // --- Plomb ---
        case TECH_FLOODED:  BATT_MEMCPY_P(&dst, &TECH_PARAMS_FLOODED,  sizeof(TechParams)); break;
        case TECH_AGM:      BATT_MEMCPY_P(&dst, &TECH_PARAMS_AGM,      sizeof(TechParams)); break;
        case TECH_GEL:      BATT_MEMCPY_P(&dst, &TECH_PARAMS_GEL,      sizeof(TechParams)); break;
        case TECH_PBC:      BATT_MEMCPY_P(&dst, &TECH_PARAMS_PBC,      sizeof(TechParams)); break;
        // --- Lithium ---
        case TECH_LIFEPO4:  BATT_MEMCPY_P(&dst, &TECH_PARAMS_LIFEPO4,  sizeof(TechParams)); break;
        case TECH_LION:     BATT_MEMCPY_P(&dst, &TECH_PARAMS_LION,     sizeof(TechParams)); break;
        case TECH_LIPO:     BATT_MEMCPY_P(&dst, &TECH_PARAMS_LIPO,     sizeof(TechParams)); break;
        case TECH_LTO:      BATT_MEMCPY_P(&dst, &TECH_PARAMS_LTO,      sizeof(TechParams)); break;
        case TECH_LMNO:     BATT_MEMCPY_P(&dst, &TECH_PARAMS_LMNO,     sizeof(TechParams)); break;
        case TECH_NMC:      BATT_MEMCPY_P(&dst, &TECH_PARAMS_NMC,      sizeof(TechParams)); break;
        case TECH_NCA:      BATT_MEMCPY_P(&dst, &TECH_PARAMS_NCA,      sizeof(TechParams)); break;
        case TECH_LCO:      BATT_MEMCPY_P(&dst, &TECH_PARAMS_LCO,      sizeof(TechParams)); break;
        // --- Nickel ---
        case TECH_NIFE:     BATT_MEMCPY_P(&dst, &TECH_PARAMS_NIFE,     sizeof(TechParams)); break;
        case TECH_NIMH:     BATT_MEMCPY_P(&dst, &TECH_PARAMS_NIMH,     sizeof(TechParams)); break;
        case TECH_NICD:     BATT_MEMCPY_P(&dst, &TECH_PARAMS_NICD,     sizeof(TechParams)); break;
        case TECH_NIZN:     BATT_MEMCPY_P(&dst, &TECH_PARAMS_NIZN,     sizeof(TechParams)); break;
        // --- Autres ---
        case TECH_SODIUM:   BATT_MEMCPY_P(&dst, &TECH_PARAMS_SODIUM,   sizeof(TechParams)); break;
        case TECH_ALKALINE: BATT_MEMCPY_P(&dst, &TECH_PARAMS_ALKALINE, sizeof(TechParams)); break;
        case TECH_SUPERCAP: BATT_MEMCPY_P(&dst, &TECH_PARAMS_SUPERCAP, sizeof(TechParams)); break;
        // --- Défaut (TECH_UNKNOWN, TECH_COUNT, valeur hors enum) ---
        default:            BATT_MEMCPY_P(&dst, &TECH_PARAMS_UNKNOWN,  sizeof(TechParams)); break;
    }
}
