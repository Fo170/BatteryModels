# BatteryModels - Bibliothèque de modèles paramétriques pour batteries

[![Arduino](https://img.shields.io/badge/Arduino-Compatible-blue)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange)](https://platformio.org/)
[![License: GPL-3.0](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![GitHub release](https://img.shields.io/github/v/release/Fo170/BatteryModels)](https://github.com/Fo170/BatteryModels/releases)
[![Version](https://img.shields.io/badge/Version-1.6.0-brightgreen.svg)](https://github.com/Fo170/BatteryModels/releases)

Bibliothèque complète de modèles paramétriques pour batteries, incluant tables OCV (Open Circuit Voltage), modèle Thévenin dynamique, et **modèles thermiques avancés (Arrhenius exponentiel)** pour 20 technologies différentes. **Header-only**, **PROGMEM-ready** pour AVR, et **cross-platform** (ESP32, ARM, etc.).

> **🚀 v1.6.0 - SUPERCONDENSATEUR (EDLC)**  
> Ajout de **TECH_SUPERCAP** : SoC basé énergie `(V² − V_min²)/(V_max² − V_min²)`,  
> **ESR seule** (pas de polarisation RC), Peukert neutralisé, OCV fiable en lecture directe.  
> Tension nominale 2.7 V/cell, `V_min = 0.5 × V_max`.  
> ✅ **100% backward compatible** — enum étendue en fin de liste, aucun changement de signature.  
> Voir [`TECHNOLOGY_EXPANSION.md`](TECHNOLOGY_EXPANSION.md).
>
> **🚀 v1.5.0 - EXPANSION TECHNOLOGIQUE (LCO, NiZn, NiCd, PbC)**  
> Ajout de 4 nouvelles technologies : **Li-Cobalt (LCO)** (table OCV 9 pts),  
> **Nickel-Zinc (NiZn)**, **Nickel-Cadmium (NiCd)** et **Plomb-Carbone (PbC)**  
> (polynômes OCV, profils Thévenin/thermique, blocs detection + Kalman).  
> ✅ **100% backward compatible** — enum étendue en fin de liste, aucun changement de signature.  
> Voir [`TECHNOLOGY_EXPANSION.md`](TECHNOLOGY_EXPANSION.md).
>
> **🚀 v1.4.0 - CONFIGURATION PAR TECHNOLOGIE (DetectionConfig + KalmanTuning)**  
> Les seuils de détection (dead-band courant, repos OCV, bande V_tol, seuil de cycle) et les  
> réglages Kalman (P/Q/R, segments ΔAh/ΔSoC, REST_LONG, confiance) sont désormais  
> **configurables par technologie**, avec surcharge **compile-time** (macros `BATTERYMODELS_*`)  
> et **runtime** (setters). Hiérarchie : setter runtime > macro > défaut de la technologie.  
> BatteryKalman v1.6.0 consommera ces réglages via `getKalmanTuning()`.  
> ✅ **100% backward compatible** — API additive, aucun changement de signature.  
> Voir [`CHANGELOG.md`](CHANGELOG.md).
>
> **🐛 v1.3.0 - CORRECTION DETECTCHARGESTATE (FLOAT)**  
> Le test `near_float` passe désormais **avant** `in_rest` dans `detectChargeState()`.  
> Une batterie en float avec un courant < 0,05 A est classée `State_FLOAT` (et non plus `State_REST`),  
> restaurant le point de référence « batterie pleine » (SoC = 100 %) pour BatteryKalman.  
> ✅ **100% backward compatible** — aucun changement d'API.  
> Voir [`CHANGELOG.md`](CHANGELOG.md).
>
> **✨ v1.2.0 - MODÈLES THERMIQUES AVANCÉS**  
> Cette version ajoute des modèles thermiques non-linéaires (Arrhenius, RC thermique, OCV quadratique).  
> **Précision améliorée ±1.4% SoC** (vs ±3.4% avant) sur -30 à +80°C.  
> Voir [`docs/technique/MODELES_THERMIQUES.md`](docs/technique/MODELES_THERMIQUES.md) pour détails techniques.  
> ✅ **100% backward compatible** avec v1.1.0.
>
> **Historique** : v1.0.1 corrigeait 11 bugs critiques + validation physique. Voir [`BUGFIXES_REPORT.md`](BUGFIXES_REPORT.md).

## 📋 Caractéristiques

- **20 technologies de batteries** supportées (voir tableau ci-dessous)
- **Tables OCV précises** : interpolation linéaire entre points caractéristiques
- **Modèle Thévenin complet** : résistance interne dynamique (T°C + SoC) + polarisation RC
- **Modèles thermiques avancés (v1.2)** 🌡️ :
  - Résistance Arrhenius exponentielle (non-linéaire)
  - Polarisation RC avec compensation thermique
  - OCV avec correction polynomiale quadratique
  - Précision ±1.4% SoC sur -30 à +80°C
- **Conversion bidirectionnelle OCV ↔ SoC** : `ocvToSoc()` et `socToOcv()` avec correction thermique
- **Coulomb counting avec correction Peukert** : suivi précis de la charge déchargée
- **Auto-détection de technologie** : analyse de la pente OCV et plage de tension
- **Seuils de détection configurables par technologie (v1.4.0)** : dead-band courant, repos OCV, bande de tolérance, seuil de cycle — surcharge compile-time (`BATTERYMODELS_*`) et runtime (setters)
- **Réglages Kalman exposés par technologie (v1.4.0)** : `getKalmanTuning()` (P/Q/R, segments ΔAh/ΔSoC, REST_LONG, confiance) pour BatteryKalman v1.6.0
- **Détection des états de charge** : identification automatique des phases (Bulk, Absorption, Float, Repos)
- **Gestion du vieillissement** : comptage de cycles et dérive de capacité
- **PROGMEM sur AVR** : constantes stockées en mémoire flash, ~1.5 Ko de RAM économisés
- **Variables d'état `volatile`** : persistance gérée par le programme utilisateur
- **Pas de dépendances externes**
- **100% backward compatible** : migration zéro depuis v1.1.0

## 🔋 Technologies supportées (20 types)

| Index | Technologie | Tension/cell | Type | Caractéristiques principales |
|-------|-------------|--------------|------|------------------------------|
| 0 | TECH_UNKNOWN | - | ♻️ | Inconnu / générique |
| 1 | TECH_FLOODED | 2.0V | ♻️ | Plomb inondé (BCI) |
| 2 | TECH_AGM | 2.0V | ♻️ | Absorbent Glass Mat |
| 3 | TECH_GEL | 2.0V | ♻️ | Gel |
| 4 | TECH_LIFEPO4 | 3.2V | ♻️ | Lithium Fer Phosphate (LFP) |
| 5 | TECH_LION | 3.6V | ♻️ | Lithium-Ion NMC/NCA générique |
| 6 | TECH_LIPO | 3.7V | ♻️ | Lithium Polymère (LiPo) |
| 7 | TECH_LTO | 2.4V | ♻️ | Lithium Titanate |
| 8 | TECH_LMNO | 3.7V | ♻️ | Lithium Manganèse |
| 9 | TECH_NMC | 3.6V | ♻️ | Nickel Manganèse Cobalt |
| 10 | TECH_NCA | 3.6V | ♻️ | Nickel Cobalt Aluminium |
| 11 | TECH_NIFE | 1.2V | ♻️ | Nickel-Fer (Edison) |
| 12 | TECH_SODIUM | 3.0V | ♻️ | Sodium-Ion (Na-ion) |
| 13 | TECH_NIMH | 1.2V | ♻️ | Nickel-Metal Hydride (AA/AAA) |
| 14 | TECH_ALKALINE | 1.5V | 🔌 | Alkaline Primary (non-rechargeable) |
| 15 | TECH_LCO | 3.7V | ♻️ | Li-Cobalt (LiCoO2, legacy phones) |
| 16 | TECH_NIZN | 1.6V | ♻️ | Nickel-Zinc (RC / amateur radio) |
| 17 | TECH_NICD | 1.2V | ♻️ | Nickel-Cadmium (legacy outils/RC) |
| 18 | TECH_PBC | 2.0V | ♻️ | Plomb-Carbone (stop-start auto) |
| 19 | TECH_SUPERCAP | 2.0V | ♻️ | **NEW** : Supercondensateur (EDLC, 2.7V max) |

**Légende**: ♻️ = Rechargeable | 🔌 = Primary (non-rechargeable)

> **Supercondensateur** : pas de plateau OCV — le SoC est calculé par énergie
> `(V² − V_min²)/(V_max² − V_min²)`, avec `V_max = 2.7 V/cell`, `V_min = 0.5 × V_max`.
> Résistance = **ESR seule** (pas de polarisation RC), Peukert neutralisé.

## 📊 États de Charge (ChargeState)

| État | Description |
|------|-------------|
| State_UNKNOWN | Inconnu |
| State_DISCHARGE | Décharge active |
| State_BULK | Charge à courant constant |
| State_ABSORPTION | Charge à tension constante |
| State_FLOAT | Maintien à 100% |
| State_REST | Repos (courant < dead-band configurée, 0.05A par défaut) |
| State_REST_LONG | Repos long (durée par technologie, ex. 2h LiFePO4/LTO) |

> **🐛 v1.3.0** : ordre de priorité corrigé dans `detectChargeState()` —
> `near_float` est testé **avant** `in_rest`. Une batterie maintenue en float
> (courant < 0,05 A) est désormais classée `State_FLOAT` et non plus
> `State_REST`, ce qui restaure le point de référence « batterie pleine »
> (SoC = 100 %) pour BatteryKalman. L'ordre est : DISCHARGE → FLOAT → REST →
> ABSORPTION → BULK.

## 🚀 Installation

### Via Arduino Library Manager (Recommended) ⭐
1. Arduino IDE : `Croquis > Inclure une bibliothèque > Gérer les bibliothèques`
2. Recherchez `BatteryModels`
3. Installez la dernière version (1.6.0+)

### Via Arduino IDE (Manual ZIP)
1. Téléchargez le ZIP depuis [Latest Release](https://github.com/Fo170/BatteryModels/releases)
2. Arduino IDE : `Croquis > Inclure une bibliothèque > Ajouter la bibliothèque .ZIP`

### Via PlatformIO (Recommended) ⭐
Ajoutez à votre `platformio.ini` :

**Pour v1.6.0 (stable):**
```ini
lib_deps =
    https://github.com/Fo170/BatteryModels.git@^1.6.0
```

**Ou via le registry:**
```ini
lib_deps =
    BatteryModels
```

### Manuellement
1. Téléchargez `src/BatteryModels.h` depuis [GitHub](https://github.com/Fo170/BatteryModels/releases)
2. Copiez dans le dossier `libraries/BatteryModels/` (Arduino) ou `lib/BatteryModels/` (PlatformIO)

## 🔗 Intégrations complémentaires

### Compatibilité avec BatteryKalman

Cette bibliothèque est **totalement compatible** avec [BatteryKalman](https://github.com/Fo170/BatteryKalman) (v1.5.3+) pour une estimation optimale du SoC.

**Avantages de la combinaison**:
- **BatteryModels** fournit le modèle physique précis de la batterie (OCV, Thévenin avancé, thermique)
- **BatteryKalman** fusionne plusieurs estimations (OCV, Coulomb, Arrhenius) via un filtre de Kalman adaptatif
- Résultat : **±0.8% erreur SoC** sur -30 à +80°C (vs ±1.4% seul BatteryModels)
- **v1.3.0** : la correction de `detectChargeState()` (test `near_float` avant `in_rest`) restaure la détection `State_FLOAT` — point de référence « batterie pleine » (SoC = 100 %) indispensable à l'apprentissage de capacité de BatteryKalman (intégrée dans BatteryKalman v1.5.3)

**Utilisation combinée** (exemple):
```cpp
#include <BatteryModels.h>
#include <BatteryKalman.h>

BatteryModel bat(TECH_LIFEPO4, 4, 100.0f, 5.0f);
BatteryKalman kalman(bat);  // Initialise avec le modèle

// Dans la boucle
float v_meas = readVoltage();
float i_meas = readCurrent();
float t_meas = readTemp();

kalman.update(v_meas, i_meas, t_meas, dt_seconds);
float soc_optimal = kalman.getSOC();  // Estimation fusionnée
```

Voir la documentation de [BatteryKalman](https://github.com/Fo170/BatteryKalman) pour des exemples détaillés.

## 📖 Utilisation rapide

```cpp
#include <BatteryModels.h>

// Création d'un modèle LiFePO4 4S 100Ah
BatteryModel bat(TECH_LIFEPO4, 4, 100.0f, 5.0f);

void setup() {
    Serial.begin(115200);

    // Informations de base
    Serial.print("Technologie: ");
    Serial.println(bat.getTechnologyName());
    Serial.print("Tension nominale: ");
    Serial.print(bat.getNominalVoltage());
    Serial.println("V");

    // Seuils de charge (Float, Absorption)
    Serial.print("Float: ");
    Serial.print(bat.getFloatVoltage());
    Serial.println("V");
    Serial.print("Absorption: ");
    Serial.print(bat.getAbsorptionVoltage());
    Serial.println("V");
}

void loop() {
    float voltage = readVoltage();      // Votre fonction de lecture
    float current = readCurrent();      // A (positif = charge)
    float temperature = readTemp();     // °C

    // Estimation SoC par OCV
    float soc = bat.ocvToSoc(voltage, temperature);

    // Correction Thévenin (tension -> OCV)
    float ocv = bat.correctVoltageToOCV(voltage, current, temperature, 1.0f);
    float soc_corrected = bat.ocvToSoc(ocv, temperature);

    // Détection état de charge
    ChargeState state = bat.detectChargeState(voltage, current);

    // Coulomb counting avec Peukert
    bat.updateCoulombCount(current, 0.0167f);  // dt = 1 minute en heures
    float soc_coulomb = bat.getCoulombSoc(100.0f);

    // Affichage
    Serial.print("SoC OCV: "); Serial.print(soc); Serial.println("%");
    Serial.print("SoC corrigé: "); Serial.print(soc_corrected); Serial.println("%");
    Serial.print("SoC Coulomb: "); Serial.print(soc_coulomb); Serial.println("%");
    Serial.print("État: "); Serial.println(getStateName(state));

    delay(1000);
}
```

## 🔧 API détaillée

### Constructeur
```cpp
BatteryModel(BatteryTech tech = TECH_AGM, uint8_t cells = 6, 
             float capacity = 0.0f, float r_int = 0.0f)
```
- `tech` : technologie de batterie (voir enum `BatteryTech`)
- `cells` : nombre de cellules en série (≥1, défaut 6)
- `capacity` : capacité nominale en Ah (≥0, 0 = auto-détection)
- `r_int` : résistance interne mesurée en mΩ (≥0, 0 = valeur par défaut de la technologie)

### Estimation SoC / OCV
| Méthode | Description |
|---------|-------------|
| `ocvToSoc(voltage, temperature)` | Convertit tension OCV en SoC (%) avec correction thermique |
| `socToOcv(soc, temperature)` | Convertit SoC (%) en tension OCV avec correction thermique |
| `correctVoltageToOCV(v, i, T, dt, &r_int, &v_pol)` | Corrige tension mesurée en OCV (modèle Thévenin + thermique) |
| `isOcvReliable()` | Indique si la technologie a une OCV fiable |
| `isOcvMeasurementValid(current, dt_rest)` | Vérifie si les conditions permettent une mesure OCV valide |

### Compensation Thermique Avancée (v1.2)
| Méthode | Description |
|---------|-------------|
| `getResistanceAtTemp(R25, temperature)` | Résistance à température donnée (Arrhenius exponentiel ou linéaire) |
| `getTauPol_thermal(temperature)` | Constante de temps RC avec correction thermique |
| `applyThermalCorrectionOCV(V_cell, temperature)` | Ajoute correction OCV thermique (linéaire + quadratique) |
| `removeThermalCorrectionOCV(V_cell, temperature)` | Retire correction OCV pour normaliser à 25°C |
| `isRechargeable()` | **NEW** : Indique si la batterie est rechargeable |

### Coulomb counting
| Méthode | Description |
|---------|-------------|
| `updateCoulombCount(current, dt_hours)` | Met à jour le compteur de charge avec correction Peukert |
| `getCoulombSoc(initial_soc)` | Retourne le SoC estimé par coulomb counting |
| `getEffectiveCapacity(current)` | Capacité effective corrigée par Peukert à un courant donné |
| `resetCoulombCount()` | Remet le compteur de charge à zéro |

### Détection des états de charge
| Méthode | Description |
|---------|-------------|
| `detectChargeState(voltage, current, capacity)` | Détecte l'état de charge de la batterie. `capacity` optionnel, fallback sur `C_nominal` |
| `getFloatVoltage()` | Tension float totale (V) |
| `getAbsorptionVoltage()` | Tension absorption totale (V) |
| `getMinVoltage()` | Tension minimale totale (V) |
| `getMaxVoltage()` | Tension maximale totale (V) |

### Configuration par technologie (v1.4.0)

**Getters** (références sur cache résolu — hiérarchie : setter runtime > macro > défaut tech) :

| Méthode | Description |
|---------|-------------|
| `getDetectionConfig()` | `const DetectionConfig&` — dead-band courant, float min/facteur, V_tol pct/min, repos OCV, seuil cycle |
| `getKalmanTuning()` | `const KalmanTuning&` — P/Q/R, segments ΔAh/ΔSoC, REST_LONG, confiance (consommé par BatteryKalman v1.6.0) |

**Setters runtime** (priorité la plus haute, `resetOverrides()` pour revenir aux défauts) :

```cpp
void setCurrentDeadband(float amps);
void setVoltageTolerance(float pct, float min_V);
void setFloatCurrentMax(float min_A, float factor);
void setOcvRestHours(float hours);
void setCycleThreshold(float pct);

void setKalmanP(float init_C, float min_C, float init_aging, float min_aging);
void setKalmanQ(float aging_uncertainty, float capacity_drift);
void setKalmanR(float init, float min, float max, float smooth_alpha);
void setKalmanSegmentThresholds(float min_dAh_abs, float min_dAh_pct,
                                float dSoc_boot, float dSoc_coarse, float dSoc_track,
                                float dSoc_high_conf, bool ocv_allowed);
void setKalmanBatteryChange(float thr, uint8_t count);
void setKalmanRestLong(float min_minutes, float stable_mv, float stable_minutes,
                       uint8_t min_samples, uint8_t max_samples);
void setKalmanConfidence(float conf_float, float conf_rest_base, float conf_rest_span,
                         float conf_phase_coarse, float conf_phase_refine);
void resetOverrides();
```

**Macros compile-time** (optionnelles, globales — écrasent le défaut par technologie si définies avant `#include`) :

```cpp
#define BATTERYMODELS_DEADBAND_A 0.03f            // dead-band courant (A)
#define BATTERYMODELS_FLOAT_CURRENT_MIN_A 0.6f    // I_float_max plancher (A)
#define BATTERYMODELS_FLOAT_CURRENT_FACTOR 0.02f  // I_float_max = max(min_A, cap×factor)
#define BATTERYMODELS_V_TOL_PCT 0.015f            // V_tol = max(min_V, V_nom×pct)
#define BATTERYMODELS_V_TOL_MIN_V 0.1f            // plancher de V_tol (V)
#define BATTERYMODELS_OCV_REST_HOURS 0.5f         // repos minimum avant OCV (h)
#define BATTERYMODELS_CYCLE_THR_PCT 0.8f          // seuil de comptage de cycle
```

### Auto-détection
| Méthode | Description |
|---------|-------------|
| `runAutoDetect(voltage, current, temperature)` | Détecte automatiquement la technologie si `auto_detect = true` |
| `setAutoDetect(bool)` | Active/désactive l'auto-détection |
| `isAutoDetect()` | État de l'auto-détection |

### Vieillissement
| Méthode | Description |
|---------|-------------|
| `getAgingDriftPerCycle()` | Dérive de capacité par cycle |
| `getCapacityFactorAging()` | Facteur de capacité restante (0.5 - 1.0) |
| `getCycleCount()` | Nombre de cycles complets détectés |

### Gestion d'état
| Méthode | Description |
|---------|-------------|
| `resetPolarization()` | Remet la polarisation à zéro |
| `resetState()` | Remet polarisation + coulomb counting |
| `fullReset()` | Remet tout (polarisation, coulomb, cycles) |

### Setters / Getters
| Méthode | Description |
|---------|-------------|
| `setTechnology(BatteryTech)` | Change la technologie |
| `setCellCount(uint8_t)` | Change le nombre de cellules (≥1) |
| `setNominalCapacity(float)` | Change la capacité nominale (≥0) |
| `setInternalResistance(float)` | Change la résistance interne (≥0) |
| `getTechnology()` / `getTechnologyName()` | Technologie active |
| `getCellCount()` / `getNominalVoltage()` / `getNominalCapacity()` | Caractéristiques |
| `getInternalResistance()` | Résistance interne effective (mΩ) |
| `getPolarizationVoltage()` | Tension de polarisation actuelle (V) |

## 🧮 Modèle Thévenin (v1.2 - Avancé)

La correction de tension utilise le modèle suivant :

```
V_ocv = V_mesure - (R_int × I) - V_pol + dVocv/dT × (T - 25°C) + 0.5 × d²Vocv/dT² × (T - 25°C)²

où:
  R_int = R25 × R_thermal(T) × (1 + γ × (1/SoC - 1))          [Ω]
  R_thermal(T) = exp(E_a/R × (1/T - 1/T25))    [Arrhenius, si E_a_R > 0]
                ou 1 + β × (25 - T)              [Linéaire, si E_a_R = 0]
  V_pol = V_pol + α × (I × R_pol - V_pol)                     [V]  (filtre RC)
  α = 1 - exp(-dt / τ_thermal)                                [-]
  τ_thermal = τ25 × exp(α_tau × (T - 25))                    [s]  (thermique exponentiel)
  R_pol = Rpol0 × (1 + δ × exp(-SoC / 20))                   [Ω]
```

**Modèles disponibles** :

| Paramètre | v1.1 (Linéaire) | v1.2 (Avancé) | Impact |
|-----------|---|---|---|
| **R(T)** | `R25×(1+β×ΔT)` | `R25×exp(E_a/R×ΔT)` | +50% précision extrêmes |
| **τ(T)** | Constant | `τ25×exp(α_tau×ΔT)` | +40% précision RC |
| **V_ocv(T)** | Linéaire seul | Polynomial ordre 2 | +30% aux limites |
| **Plage SoC** | ±3.4% erreur | **±1.4% erreur** | **2.4× meilleur** |

**Stabilité numérique** : `α` est limité à `[0, 1]`. Si `dt < 0`, la polarisation est resetée.

**Backward Compatibility** : Si `E_a_R = 0`, utilise modèle linéaire (v1.1 comportement).

## 🔋 Loi de Peukert

La capacité effective dépend du courant de décharge :

```
C_eff = C_nominal × (I_C20 / |I|)^(k - 1)
```

- `k = 1.0` : pas de correction (idéal)
- `k = 1.4` (plomb) : forte dégradation à fort courant
- `k = 1.05` (LiFePO4) : très faible dégradation

## 💾 PROGMEM (AVR uniquement)

Sur Arduino Uno/Nano/Mega, toutes les constantes sont stockées en mémoire programme :
- **Tables OCV** : ~650 octets en flash
- **Paramètres** : ~730 octets en flash
- **Noms** : ~200 octets en flash
- **Cache RAM** : 56 octets seulement

Sur ESP32/ARM, les `const` sont automatiquement placées en flash ; la macro `BATT_PROGMEM` est transparente.

## ⚠️ Notes importantes

1. **Variables `volatile`** : `V_pol_state`, `last_current`, `accumulated_charge_Ah` et `cycle_count` sont `volatile`. Leur persistance doit être gérée par le programme utilisateur (EEPROM, RTC memory, etc.) si nécessaire.

2. **OCV fiable** : Les technologies **NiFe** et **Sodium-Ion** ont une OCV peu fiable pour l'estimation SoC. Préférer le coulomb counting.

3. **Auto-détection** : Nécessite une tension stable et un courant connu. La détection est heuristique et peut nécessiter une confirmation utilisateur.

4. **Polynômes vs Tables** : Les tables OCV sont prioritaires (précision). Les polynômes servent de fallback pour `TECH_UNKNOWN` et les technologies plomb/NiFe.

## 🔧 Changements v1.0.1 (IMPORTANT!)

**11 bugs corrigés** (voir [`BUGFIXES_REPORT.md`](BUGFIXES_REPORT.md)):

### Bugs Logiques
- ✅ **Signe Coulomb inversé** — SoC maintenant augmente en décharge (correction: `+` au lieu de `-`)
- ✅ **Résistance négative** — Plafond à 0.001 mΩ min pour éviter NaN
- ✅ **Peukert manquant** — Intégré à l'accumulation de charge
- ✅ **R_soc saturation** — Remplace seuil magique 0.01f par saturation exponentielle lisse
- ✅ **Tolérance voltage** — Dynamique (±1.5% V_nominal) au lieu de fixe (±0.6V)
- ✅ **Peukert taux 20h** — Maintenant par technologie (20h plomb, 1C lithium)
- ✅ **Buffer petit** — 32 → 40 bytes pour getTechnologyName()

### Corrections Physiques (Modèles Batterie)
- ✅ **FLOODED** — Float voltage 2.30V → 2.37V (IEC 61427 standard)
- ✅ **GEL** — Float voltage 2.30V → 2.37V, Abs 2.45V → 2.43V
- ✅ **LMNO** — Absorption 4.30V → 4.20V (protège spinelle Mn, durée de vie 2×)
- ✅ **NIFE** — OCV polynomial recalibré ({-0.206,0.518,1.020} → {0.1,0.35,1.0})

**Résultat**: 13/13 modèles batterie validés physiquement contre datasheets et références.

## 📄 Licence

GNU General Public License v3.0 - Voir [LICENSE](LICENSE) pour les détails complets.

**Important**: Cette bibliothèque est distribuée sous GPLv3, ce qui signifie:
- ✓ Utilisation libre (commerciale ou non)
- ✓ Modification permise
- ✓ Distribution permise
- ⚠️ **OBLIGATION**: Tout projet utilisant cette lib doit aussi être GPLv3 et partager le code source
- ⚠️ **OBLIGATION**: Toute modification doit être documentée

Pour une licence plus permissive sans obligation de copyleft, contactez l'auteur.

## 🤝 Contribuer

Les contributions sont les bienvenues ! Ouvrez une issue ou une pull request sur [GitHub](https://github.com/Fo170/BatteryModels).
