# BatteryModels - Bibliothèque de modèles paramétriques pour batteries

[![Arduino](https://img.shields.io/badge/Arduino-Compatible-blue)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange)](https://platformio.org/)
[![License: GPL-3.0](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![GitHub release](https://img.shields.io/github/v/release/Fo170/BatteryModels)](https://github.com/Fo170/BatteryModels/releases)
[![Version](https://img.shields.io/badge/Version-1.6.10-brightgreen.svg)](https://github.com/Fo170/BatteryModels/releases)

Bibliothèque complète de modèles paramétriques pour batteries, incluant tables OCV (Open Circuit Voltage), modèle Thévenin dynamique, et **modèles thermiques avancés (Arrhenius exponentiel)** pour 20 technologies différentes. **Header-only**, **PROGMEM-ready** pour AVR, et **cross-platform** (ESP32, ARM, etc.).

> **🐛 v1.6.10 — rendement η découplé de Peukert**  
> `getChargeEfficiency()`/`getDischargeEfficiency()` appliquaient `η_base·(I_ref/|I|)^(n−1)`.
> Deux défauts : (1) **sens inversé en décharge** — Peukert décrit la capacité *disponible*
> (décroît avec |I|), les Ah prélevés doivent donc être **amplifiés**, pas réduits → la décharge
> n'était comptée qu'à **58 % à 5 A** ; (2) **écrêtée à 1,0**, la loi vaut exactement 1,0 pour
> |I| ≤ ~1 A, donc elle ne corrige rien dans le régime de faible courant. η est désormais
> **constant**. La loi de Peukert reste dans `getEffectiveCapacity(I)`, où elle est juste.  
> ✅ API inchangée (le paramètre `current` est ignoré).
>
> **🐛 v1.6.9 — `RlsIdentifier` : `isValid()` détecte enfin la saturation**  
> La 1.6.8 testait des valeurs **déjà bornées** (`_theta[1]` est `constrain`é, `_tau` n'est
> assigné qu'in-bounds, `getR1()` renvoie déjà une valeur ≤ `_r1Max`) : tous les tests étaient
> vrais par construction, donc `isValid() ≡ (_updates ≥ 5)`. Le garde-fou ne détectait **aucun**
> des symptômes terrain (R1 = 1,335 Ω masquée à 0,5 ; τ = 59,9 s collé à la borne ; R0 exactement
> sur `_r0Min`). Désormais : **détection de saturation** via `getR1Raw()`, `isR1Saturated()`,
> `isR0Saturated()`, `isTauSaturated()` — `isValid()` renvoie `false` dès qu'un paramètre est en butée.  
> \+ **anti-windup** sur la projection de θ₂ (borner sans corriger `P` déplaçait la divergence vers
> R0, puis windup → `reset()` = perte totale de l'apprentissage) ; **bornes `deserialize()` alignées**
> sur `isValid()` ; **`SocEkf::setEfficiency()`** (l'EKF intégrait `I` brut alors que le compteur
> `Coulomb` intègre `I·η` → divergence 1,9× à 5 A).  
> ✅ **100% backward compatible** — layout de persistance `SER_LEN = 46 o` inchangé.
>
> **🐛 v1.6.8 — `RlsIdentifier` : R1 bornée**  
> `R1 = b1/(1−a1)` n'était bornée **nulle part** : elle atteignait 1,2·10⁶ Ω en réel (Coulométre,
> 17-18/10/2026) et polluait `SocEkf::setParams()` ainsi que la correction OCV. θ₂ est désormais
> borné ⇒ `R1 ∈ [0, _r1Max]` (`setR1Max()`, défaut 0,5 Ω), `getR1()` borné en sortie.  
> ✅ API additive.
>
> **✨ v1.6.6 — AFFRLS + EKF d'état**  
> `RlsIdentifier` (AFFRLS/ARX, R0/R1/τ), `SocEkf` (EKF [SoC,V1,V2]), OCV cubique ajustable
> (`setOcvPolynomial`), hystérésis one-state (`setHysteresis`), 2ᵉ branche RC + gassing
> (`setSecondRC`/`setGassing`), Peukert/η variable.
>
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
> BatteryKalman v1.7.3+ consomme ces réglages via `getKalmanTuning()`.  
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

## 🐛 Évolutions et corrections — détail (quoi, pourquoi, formule)

> Chaque entrée donne **ce qui était faux**, **pourquoi** et la **formule exacte** avant/après.
> Les sources sont listées en fin de document ([Sources et références](#-sources-et-références)).

### v1.6.10 — 2026-10-18 · rendement η découplé de Peukert

**Ce qui était faux.** `_variableEfficiency()` (utilisé par `getChargeEfficiency()` et
`getDischargeEfficiency()`) appliquait la loi de Peukert comme rendement :

```
η(I) = η_base · (I_ref / |I|)^(n−1)      borné à [0,5 ; 1]        ← FAUX
```

**Pourquoi c'était faux.** Deux défauts indépendants, identiques à ceux corrigés dans
`CoulombsAh` v1.3.1 :

1. **Sens inversé en décharge.** Peukert décrit la capacité **disponible**, qui décroît avec `|I|` :
   les Ah prélevés à fort courant doivent être **amplifiés**, pas réduits. Appliquée comme
   rendement, la loi **réduisait** la décharge (58 % à 5 A avec `η_base = 1`, `I_ref = 1,30 A`,
   `n = 1,40`) → SoC descendant 42 % trop lentement (biais optimiste).
2. **Écrêtage annulant le correctif en charge.** Écrêtée à 1,0, la loi renvoie **exactement
   1,000** pour `|I| ≤ ~1 A` — le régime de faible courant qu'elle prétendait corriger.

**Ce qui a changé.** η est **constant** (indépendant du courant), comme dans `Coulomb.h` v1.3.1 :

```
η(I) = η_base          (constante ; le paramètre `current` est ignoré)
```

La loi de Peukert reste appliquée là où elle est juste : `getEffectiveCapacity(I)` et
`updateCoulombCount()`.

### v1.6.9 — 2026-10-18 · `isValid()` détecte la saturation, anti-windup, EKF et η

**Ce qui était faux (1) — `isValid()` ne détectait rien.** Les quatre tests de la v1.6.8 portaient
sur des valeurs **déjà bornées** :

| Test v1.6.8 | Pourquoi il est toujours vrai |
|---|---|
| `θ1 ∈ [_r0Min, _r0Max]` | `θ1` est `constrain()`é à cet intervalle après chaque mise à jour |
| `τ ∈ [_tauMin, _tauMax]` | `_tau` n'est assigné que si `τ` est déjà dans les bornes |
| `a1 ∈ ]0, 1[` | `a1` est `constrain()`é à `[1e−6 ; 0,999999]` |
| `r1 ≥ 0` | `getR1()` renvoie **déjà** une valeur bornée à `[0, _r1Max]` |

Donc `isValid() ≡ _valid ≡ (_updates ≥ 5)`. Aucun des trois symptômes terrain n'était détecté :
`R1 = 1,335 Ω` masquée à 0,5 ; `τ = 59,9 s` collé à la borne haute ; `R0` exactement sur `_r0Min`.

**Correctif.** On détecte la **saturation** (paramètre en butée ⇒ modèle non identifié) :

```
isR0Saturated()  = (θ1 ≤ 1,001·_r0Min)  OU (θ1 ≥ 0,999·_r0Max)
isTauSaturated() = (τ  ≤ 1,001·_tauMin) OU (τ  ≥ 0,999·_tauMax)
isR1Saturated()  = (R1_raw < 0) OU (R1_raw ≥ 0,999·_r1Max) OU non fini
isValid() = _valid ET a1 ∈ ]0 ; 0,999999[ ET AUCUN des trois saturés
```

**Ce qui était faux (2) — projection de θ₂ sans correction de `P` (windup).** Borner θ₂ déplaçait
la divergence : à courant quasi constant, `θ1` (R0) et `b1` sont quasi colinéaires, donc tout
l'écart était absorbé par R0 qui dérivait vers `_r0Max`, et `P` non corrigée provoquait un windup
(λ → λ_min, trace de `P` → `reset()` = perte totale de l'apprentissage).

**Correctif (anti-windup).** Si la projection agit, on réinitialise la sous-covariance (R0, R1) et
on gèle l'oubli : `P[1][1] = P[2][2] = 0,01`, `P[1][2] = P[2][1] = 0`, `λ = λ_max`.

**Ce qui était faux (3) — bornes asymétriques `deserialize()` / `isValid()`.** `deserialize()`
acceptait `θ1 ∈ [0,025 ; 3,0] Ω` puis `isValid()` exigeait `[0,05 ; 2,0]` : un état FRAM dans
`]2,0 ; 3,0]` était restauré « valide » puis jugé invalide → correction tension désactivée en
silence jusqu'au prochain pas réussi.

**Correctif.** Bornes alignées (`θ1 ∈ [_r0Min, _r0Max]` dans `deserialize()`) et θ₂ re-projeté au
chargement (`_r1Max` n'étant pas persisté, un θ₂ restauré pouvait dépasser la borne courante).

**Ce qui était faux (4) — l'EKF intégrait un autre courant que le compteur.** `Coulomb` intègre
`I·η` mais `SocEkf` intégrait `I` brut → les deux SoC divergeaient d'un facteur `1/η` (1,9× à 5 A),
et un fail-safe de comparaison `|SoC_EKF − SoC_coulomb| > 40 %` se déclenchait sur un écart de
**modèle** et re-seedait en boucle.

**Correctif.** `SocEkf::setEfficiency(η_charge, η_décharge)` applique le même η à `dSOC/dt`
(la chute ohmique `I·R0` continue d'utiliser le courant **réel**).

### v1.6.8 — 2026-10-18 · R1 bornée

**Ce qui était faux.** `θ2` n'était borné **nulle part**, contrairement à `θ0` (a1) et `θ1` (R0).
Or `b1 = θ2 + a1·θ1` et `R1 = b1/(1 − a1)` : quand `a1 → 1`, `1 − a1 → 0` et le moindre écart sur
`b1` fait **exploser** R1. Observé en réel (Coulométre, 17-18/10/2026, `rls_upd > 25 000`) :
`R1 = 1,335 Ω` puis jusqu'à **1,24·10⁶ Ω**, ces valeurs étant injectées dans `SocEkf::setParams()`
et dans la correction OCV (d'où un `SoC_EKF` à 90,2 % quand la tension donnait 77,1 %).

**Correctif.** Après chaque mise à jour, `b1` est borné pour que `R1 ∈ [0, _r1Max]` :

```
b1_max = _r1Max · (1 − a1)
b1     = clamp(b1, 0, b1_max)
θ2     = b1 − a1 · θ1
```
et `getR1()` borne sa sortie. `_r1Max` par défaut = **0,5 Ω**, réglable par `setR1Max()`.

### v1.6.6 — 2026-10-18 · AFFRLS + EKF d'état

Ajout de `RlsIdentifier` (identification en ligne R0/R1/τ1), `SocEkf` (EKF `[SoC, V1, V2]`),
OCV cubique ajustable (`setOcvPolynomial`), hystérésis one-state (`setHysteresis`), 2ᵉ branche RC +
gassing (`setSecondRC` / `setGassing`), Peukert et η variable.

---

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
- **Réglages Kalman exposés par technologie (v1.4.0)** : `getKalmanTuning()` (P/Q/R, segments ΔAh/ΔSoC, REST_LONG, confiance) pour BatteryKalman v1.7.3+
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
3. Installez la dernière version (1.6.10+)

### Via Arduino IDE (Manual ZIP)
1. Téléchargez le ZIP depuis [Latest Release](https://github.com/Fo170/BatteryModels/releases)
2. Arduino IDE : `Croquis > Inclure une bibliothèque > Ajouter la bibliothèque .ZIP`

### Via PlatformIO (Recommended) ⭐
Ajoutez à votre `platformio.ini` :

**Pour v1.6.10 (stable):**
```ini
lib_deps =
    https://github.com/Fo170/BatteryModels.git#v1.6.10
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

Cette bibliothèque est **totalement compatible** avec [BatteryKalman](https://github.com/Fo170/BatteryKalman) (v1.7.3+) pour une estimation optimale du SoC.

**Avantages de la combinaison**:
- **BatteryModels** fournit le modèle physique précis de la batterie (OCV, Thévenin avancé, thermique)
- **BatteryKalman** fusionne plusieurs estimations (OCV, Coulomb, Arrhenius) via un filtre de Kalman adaptatif
- Résultat : **±0.8% erreur SoC** sur -30 à +80°C (vs ±1.4% seul BatteryModels)
- **v1.3.0** : la correction de `detectChargeState()` (test `near_float` avant `in_rest`) restaure la détection `State_FLOAT` — point de référence « batterie pleine » (SoC = 100 %) indispensable à l'apprentissage de capacité de BatteryKalman (intégrée dans BatteryKalman depuis v1.5.3)

**Utilisation combinée** (exemple, API BatteryKalman v1.7.3):
```cpp
#include <BatteryModels.h>
#include <BatteryKalman.h>
#include <Coulomb.h>  // depuis Fo170/CoulombsAh

// Données à persister
SoCData socData;
KalmanState2D kalmanState;

BatteryModel bat(TECH_LIFEPO4, 4, 100.0f, 5.0f);
Coulomb coulomb;  // Compteur coulombs (CoulombsAh v1.3.1+)
BatteryKalman kalman(&socData, &kalmanState, &bat, &coulomb);

// Dans la boucle (10 Hz)
float v_meas = readVoltage();
float i_meas = readCurrent();
float t_meas = readTemp();
coulomb.addMeasurement(i_meas);       // alimenter le compteur coulombs
kalman.update(v_meas, i_meas, t_meas); // API v1.7.x : update(V, I, T)
float soc_optimal = kalman.getSoC();  // Estimation fusionnée
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
| `getKalmanTuning()` | `const KalmanTuning&` — P/Q/R, segments ΔAh/ΔSoC, REST_LONG, confiance (consommé par BatteryKalman v1.7.3+) |

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

### AFFRLS — `RlsIdentifier` (v1.6.6+, durci en v1.6.8/v1.6.9)
Identification en ligne des paramètres Thévenin (R0, R1, τ1) par moindres carrés récursifs
à facteur d'oubli adaptatif, sur un régresseur ARX `y[k] = a1·y[k−1] + R0·I[k] + (b1 − a1·R0)·I[k−1]`.

| Méthode | Description |
|---------|-------------|
| `begin()` | Réinitialise l'identificateur |
| `update(Vt, I, ocv, dt)` | Un pas d'identification (retourne `true` si mis à jour) |
| `setBounds(r0_min, r0_max, tau_min, tau_max)` | Bornes R0 (Ω) et τ (s) |
| `setR1Max(ohm)` | **v1.6.8** — borne de la 1re branche RC (défaut 0,5 Ω) |
| `setExcitationThreshold(dI)` / `setLongWindowThreshold(dI)` / `setAdaptiveMax(dI)` | Seuils d'excitation (1 pas / fenêtre longue / borne du seuil adaptatif) |
| `setStepThreshold(dI)` | Seuil du R0 direct ΔV/ΔI |
| `setUpdatePeriod(s)` | Pas nominal (sert au recalcul de τ après restauration) |
| `getR0()` / `getR1()` / `getTau1()` | Paramètres identifiés (bornés) |
| `getR1Raw()` | **v1.6.9** — R1 **brute**, non bornée (diagnostic) |
| `isR1Saturated()` / `isR0Saturated()` / `isTauSaturated()` | **v1.6.9** — paramètre collé à une borne |
| `isValid()` | **v1.6.9** — `false` dès qu'un paramètre est **en butée** (modèle inexploitable) |
| `getUpdates()` / `getLambda()` / `getR0Step()` | Diagnostic (nb de mises à jour, λ, dernier ΔV/ΔI) |

> ⚠️ **Ne pas se contenter de `_updates ≥ 5`** : c'était le bug de la v1.6.8 — les tests de
> plausibilité portaient sur des valeurs déjà bornées, donc toujours vraies. Un paramètre
> **saturé** signifie que le modèle n'est pas identifié et ne doit pas être injecté dans l'EKF
> ni dans la correction OCV.

### EKF d'état — `SocEkf` (v1.6.6+)
EKF à état `x = [SoC, V1, V2]`, observation `Vt = OCV(SoC) + I·R0 + V1 + V2` (I > 0 = charge).

| Méthode | Description |
|---------|-------------|
| `begin()` / `setInitialSoC(pct)` | Initialisation |
| `setParams(r0, r1, tau1, r2, tau2)` | Paramètres Thévenin (Ω, s) |
| `setCapacity(ah)` / `setNoise(q_soc, q_v, r_meas)` | Capacité et bruits Q/R |
| `setVoltageCorrection(bool)` | `false` ⇒ l'EKF suit le coulomb (prédiction seule) |
| `setEfficiency(eta_chg, eta_dis)` | **v1.6.9** — applique le même η que `Coulomb` à `dSOC/dt` |
| `update(model, vt, I, temp, dt)` | Un pas EKF |
| `getSoC()` / `getConfidence()` | Sorties |

> ⚠️ **v1.6.9** : sans `setEfficiency()`, l'EKF intègre `I` brut alors que `Coulomb` intègre
> `I·η` → les deux SoC divergent d'un facteur 1/η (1,9× à 5 A) et un fail-safe de comparaison
> peut re-seeder en boucle.

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

## 🧮 Formules exactes des algorithmes

> Ces formules sont **identiques au code** (`src/BatteryModels.h`). Convention de signe :
> **`I > 0` = charge**, `I < 0` = décharge. Tensions en volts, courants en ampères, temps en secondes.
> ⚠️ Le modèle travaille en **par cellule** ; `R0`/`R1` de `RlsIdentifier` sont en **Ω batterie entière**.

### 1. Modèle de Thévenin 2 RC (observation)

```
V_terminal = OCV(SoC) + I·R0 + V1 + V2 + h + V_gas        (I > 0 ⇒ V_terminal > OCV)
```

Discrétisation exacte des branches RC (ZOH, `a_k = e^(−Δt/τ_k)`) :

```
V1(k) = a1·V1(k−1) + R1·(1 − a1)·I(k)          a1 = e^(−Δt/τ1)
V2(k) = a2·V2(k−1) + R2·(1 − a2)·I(k)          a2 = e^(−Δt/τ2)
```
Garde : `|V1|, |V2| ≤ 5 V` (bornage appliqué en prédiction **et** en correction).

### 2. OCV cubique et inversion

```
OCV(s) = A0 + A1·s + A2·s² + A3·s³             s = SoC/100 ∈ [0 ; 1]   (V par cellule)
```

Inversion `OCV → SoC` par **bissection** (24 itérations), l'OCV étant supposé monotone croissant :

```
si OCV(0) ≥ V → SoC = 0 %
si OCV(1) ≤ V → SoC = 100 %
sinon : recherche de la racine de f(s) = A0 + A1·s + A2·s² + A3·s³ − V sur [0 ; 1]
```

Valeurs plomb ajustées sur points **au repos** (I ≈ 0), converties par cellule (÷ 6) :

```
A0 = 11,88996/6   A1 = 1,370442/6   A2 = −2,298/6   A3 = 1,919/6
```
> ⚠️ **Méthode** : la régression OCV doit se faire sur des points **au repos**. Un ajustement sur
> données **sous charge** absorbe la chute ohmique `I·R0` dans l'OCV, et R0 devient alors
> inidentifiable (chute apparente 55 mΩ au lieu de ~350 mΩ — constaté).

### 3. Correction ohmique (`correctVoltageToOCV`)

```
V_cell      = V_terminal / n_cells
R_T         = R25 · exp(α_R·(T25_abs − T_abs))   si E_a_R > 0,001 J/mol  (Arrhenius)
              α_R = E_a_R / (R_gas·T25_abs·T_abs), T_abs = T + 273,15 K, T25_abs = 298,15 K, R_gas = 8,314
R_T         = R25 · (1 + β·(25 − T))             sinon                   (linéaire)
R_soc       = 1 + γ_soc·(1/s_ref − 1)          saturé à R_soc_max,  s_ref = clamp(SoC/100, 0,05 ; 1)
R_int       = R_T · R_soc / 1000                [Ω par cellule]  (R_T en mΩ)
R_pol       = (Rpol0/1000)·(1 + δ_pol·e^(−SoC/20))
V_pol(k)    = V_pol(k−1) + (1 − e^(−Δt/τ_pol))·(I·R_pol − V_pol(k−1))
V_OCV       = V_cell − (I·R_int + V_pol) − h − V_gas      puis correction thermique OCV
```
> ⚠️ **Sens de la correction thermique** : `R` **augmente quand la température baisse** — d'où le
> terme `(25 − T)` (modèle linéaire) et `(T25_abs − T_abs)` (Arrhenius). Un signe inversé rendrait
> la batterie « meilleure à froid », ce qui est physiquement faux.

### 4. Hystérésis one-state (Plett)

```
h_ss  = +h_max   si I > 0   (charge)
h_ss  = −h_max   si I < 0   (décharge)
h_ss  = h(k−1)   si I = 0   (gelé)
h(k)  = h(k−1) + (1 − e^(−Δt·γ))·(h_ss − h(k−1))       borné à [−h_max, +h_max]
```
`h` est **retranché** dans la conversion `V_terminal → OCV` pour que la lecture de SoC ne dépende
plus du sens du courant.

### 5. Gassing (branche parasite de fin de charge)

```
V_gas_ss = k_gas·(V_cell − V_gas_seuil)   si I > 0 ET V_cell > V_gas_seuil
V_gas_ss = 0                              sinon
V_gas(k) = V_gas(k−1) + (1 − e^(−Δt/20))·(V_gas_ss − V_gas(k−1))      borné à [0 ; 5] V
```

### 6. Peukert — capacité effective

```
C_eff(I) = C_nom · (I_ref / |I|)^(n−1)      si |I| ≥ 0,01 A et n > 1
C_eff(I) = C_nom                            sinon
peukert_ratio = C_nom / C_eff(I)  ≥ 1       (amplifie les Ah prélevés à fort courant)
ΔAh = I · Δt_h · peukert_ratio
```
> Le facteur `(I_ref/|I|)^(n−1)` est **> 1** quand `|I| > I_ref` : la capacité disponible
> **diminue** à fort courant. C'est pourquoi il **ne doit jamais** servir de rendement η.

### 7. AFFRLS — `RlsIdentifier` (identification R0/R1/τ1)

**Modèle ARX** (`y = V_terminal − OCV`) :

```
y(k) = a1·y(k−1) + R0·I(k) + (b1 − a1·R0)·I(k−1)
θ = [ θ0 , θ1 , θ2 ] = [ a1 , R0 , b1 − a1·R0 ]
φ = [ y(k−1) , I(k) , I(k−1) ]
```

**Mise à jour RLS** (facteur d'oubli λ) :

```
K      = P·φ / (λ + φᵀ·P·φ)
e      = y − φᵀ·θ
θ     ← θ + K·e
P     ← (P − K·φᵀ·P) / λ
```

**Facteur d'oubli adaptatif** :

```
λ = λ_min + (λ_max − λ_min)·e^(−κ·|e|)        borné à [λ_min ; λ_max]
```
(erreur forte ⇒ λ → λ_min ⇒ adaptation rapide ; erreur faible ⇒ λ → λ_max ⇒ lissage.)

**Extraction des paramètres physiques** :

```
R1 = b1 / (1 − a1)              b1 = θ2 + a1·θ1
τ1 = −Δt / ln(a1)
```

**Bornes** : `R0 ∈ [0,05 ; 2,0] Ω`, `τ1 ∈ [2 ; 60] s`, `R1 ∈ [0 ; _r1Max]` (`_r1Max` = 0,5 Ω par
défaut, cf. v1.6.8), `a1 ∈ [1e−6 ; 0,999999]`, `trace(P) ≤ 1e7` (au-delà : `reset()`).

**Excitation** (sinon gel : aucune mise à jour) — deux critères, pour capter aussi les **rampes
lentes** (charge solaire ≈ 0,0018 A/s, invisible sur un critère 1-pas) :

```
court  : |I(k) − I(k−1)|          ≥ seuil_adaptatif     seuil_adaptatif = clamp(6·jitter, _dIThr ; _dIThrMax)
long   : |I(k) − I(k−60)|         ≥ seuil_fenêtre_longue
échelon: |I(k) − I(k−1)|          ≥ _dIThrStep   ⇒ ouvre la fenêtre d'identification de τ
```

### 8. EKF d'état — `SocEkf`

État `x = [SoC, V1, V2]` (SoC en **fraction** 0-1).

**Prédiction** :

```
SoC(k) = clamp( SoC(k−1) + (I·η·Δt / 3600) / C_ref , 0 ; 1 )
V1(k)  = clamp( a1·V1(k−1) + R1·(1 − a1)·I , −5 ; 5 )
V2(k)  = clamp( a2·V2(k−1) + R2·(1 − a2)·I , −5 ; 5 )
P      ← A·P·Aᵀ + Q          A = diag(1, a1, a2)
```

**Observation** :

```
V̂ = OCV(SoC) + I·R0 + V1 + V2
innovation = V_mesuré − V̂
H = [ ∂OCV/∂s , 1 , 1 ]      (dérivée numérique : (OCV(s+0,01) − OCV(s−0,01)) / (s₂ − s₁))
S = H·P·Hᵀ + R_eff
K = P·Hᵀ / S
x ← clamp(x + K·innovation, 0 ; 1 pour SoC, ±5 V pour V1/V2)
P ← (I − K·H)·P          puis symétrisation + garde (diagonale ≥ 0 sinon reset())
```

**Confiance** : `σ = √S`, `z = |innovation| / σ`, `confiance = 1 / (1 + z)` ∈ [0 ; 1].

**Bruit de mesure effectif** : `R_eff = R · r_scale` et `R_eff ×= load_r_factor` si `|I| > 0,2 A`
(R0 incertain sous charge ⇒ l'EKF fait davantage confiance au coulomb).

**Correction désactivable** : `setVoltageCorrection(false)` ⇒ prédiction seule (l'EKF suit le
coulomb), à utiliser tant que les paramètres Thévenin ne sont pas identifiés.

---

## 📚 Sources et références

Consultées le **2026-10-18**. Toutes répondent HTTP 200 à cette date.

| Sujet | Source | Ce qui en est tiré |
|---|---|---|
| Loi de Peukert | [Wikipedia — Peukert's law](https://en.wikipedia.org/wiki/Peukert%27s_law) | `C/C₀ = (I₀/I)^(k−1)` ; la capacité **disponible décroît** avec le courant ⇒ Peukert ne peut pas servir de rendement η (v1.6.10) |
| Rendement faradique | [Wikipedia — Faraday efficiency](https://en.wikipedia.org/wiki/Faraday_efficiency) | η = rapport des charges échangées sur un cycle (constante par direction, pas fonction de `I`) |
| Rendement coulombique plomb | [Battery University — BU-808c](https://batteryuniversity.com/article/bu-808c-coulombic-and-energy-efficiency-with-the-battery) | Plomb : CE ≈ 90 % ; un courant très faible **abaisse** le rendement (auto-décharge) |
| Charge du plomb | [Battery University — BU-403](https://batteryuniversity.com/article/bu-403-charging-lead-acid) | Rendement élevé « with sufficient charge current » |
| Moindres carrés récursifs (RLS), facteur d'oubli | [Wikipedia — Recursive least squares filter](https://en.wikipedia.org/wiki/Recursive_least_squares_filter) | Forme des équations `K = Pφ/(λ + φᵀPφ)`, `θ ← θ + K·e`, `P ← (P − KφᵀP)/λ` |
| Filtre de Kalman | [Wikipedia — Kalman filter](https://en.wikipedia.org/wiki/Kalman_filter) | Prédiction/correction, gain, covariance |
| Filtre de Kalman étendu (EKF) | [Wikipedia — Extended Kalman filter](https://en.wikipedia.org/wiki/Extended_Kalman_filter) | Linéarisation par jacobien `H`, même structure de gain |
| Modèle de Thévenin / 2 RC | [Wikipedia — Internal resistance](https://en.wikipedia.org/wiki/Internal_resistance) | Chute ohmique `V = E − I·r` et polarisation |
| Batterie plomb-acide | [Wikipedia — Lead–acid battery](https://en.wikipedia.org/wiki/Lead%E2%80%93acid_battery) | Tensions par cellule, rendement, comportement de charge |
| Loi d'Arrhenius (dépendance thermique de R) | [Wikipedia — Arrhenius equation](https://en.wikipedia.org/wiki/Arrhenius_equation) | `k = A·e^(−Ea/(R·T))` pour `getResistanceAtTemp()` |
| État de charge (définitions, méthodes) | [Wikipedia — State of charge](https://en.wikipedia.org/wiki/State_of_charge) | Définitions SoC/DoD, comptage de coulombs vs OCV |
| Gestion de batterie (BMS) | [Wikipedia — Battery management system](https://en.wikipedia.org/wiki/Battery_management_system) | Rôle de l'estimation SoC/SoH |

> ℹ️ **Sources inaccessibles au 2026-10-18** (non retenues, aucune conclusion ne repose dessus
> seule) : `batteryuniversity.com/article/bu-503-the-peukert-number` (HTTP 500),
> `batteryuniversity.com/article/bu-808b-...` (HTTP 500),
> `mathworks.com/help/aeroblks/extendedkalmanfilter.html` (HTTP 404).

---

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
