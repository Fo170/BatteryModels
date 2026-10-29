// ============================================================
// BatteryModels.h — POINT D'ENTRÉE UNIQUE (agrégateur)
// ============================================================
// v2.0.0 — le header monolithique de 2 321 lignes (v1.6.11) a été fractionné
// en modules. Ce fichier ne fait plus qu'AGRÉGER : `#include "BatteryModels.h"`
// reste la seule ligne à écrire, l'API publique est inchangée.
//
// ARBORESCENCE
// ------------
//   BatteryModelsCore.h        enums, macros PROGMEM, OcvPoly / DetectionConfig /
//                              KalmanTuning / TechParams
//   tech/TechRegistry.h        correspondance NOMINATIVE enum → paramètres
//   tech/Unknown.h             TECH_UNKNOWN
//   tech/Pb.h                  FLOODED, AGM, GEL, PbC
//   tech/LiFePO4.h             LiFePO4 (+ table OCV)
//   tech/LiIon.h               LION, LIPO, LMNO, NMC, NCA, LCO (+ tables OCV)
//   tech/LTO.h                 LTO (+ table OCV)
//   tech/Sodium.h              Sodium-Ion (+ table OCV)
//   tech/NiFe.h · NiMh.h · NiCd.h · NiZn.h · Alkaline.h · Supercap.h
//   BatteryModel.h             classe BatteryModel (modèle physique)
//   RlsIdentifier.h            AFFRLS (R0/R1/tau1)
//   SocEkf.h                   EKF d'état [SoC, V1, V2]
//
// POURQUOI CE FRACTIONNEMENT
// --------------------------
// 1. `TECH_PARAMS[TECH_COUNT]` était indexé par l'enum : ajouter une techno
//    obligeait à l'insérer au bon rang, et une erreur de rang décalait
//    SILENCIEUSEMENT toutes les suivantes. Remplacé par un `switch` nominatif.
// 2. `BatteryModel` faisait 1 067 lignes (modèle + OCV + détection + Peukert +
//    gazage + hystérésis + RC). Il est isolé dans son propre fichier.
// 3. `RlsIdentifier` et `SocEkf` n'ont aucun lien avec `BatteryModel` — deux
//    estimateurs autonomes qui cohabitaient par commodité d'inclusion.
//
// Aucune formule, aucun coefficient, aucune table n'a été modifié.
// Les 136 vérifications des tests hôte passent SANS modification du test.
//
// ⚠️ Synchronisation amont MANUELLE désormais — voir FORK.md.
// ============================================================

#ifndef BATTERY_MODELS_H
#define BATTERY_MODELS_H

#include "BatteryModelsCore.h"     // enums, macros, structs (dont __BATTERYMODELS_H_VERSION__)
#include "tech/TechRegistry.h"     // tables + TechParams + techParamsCopy()
#include "BatteryModel.h"          // classe BatteryModel
#include "RlsIdentifier.h"         // AFFRLS
#include "SocEkf.h"                // EKF d'état

#endif // BATTERY_MODELS_H
