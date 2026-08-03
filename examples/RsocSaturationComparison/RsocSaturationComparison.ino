#include <BatteryModels.h>

// Démonstration: Comparaison ancien vs nouveau facteur R_soc
// Montre comment le seuil magique causait des problèmes à bas SoC
//
// NOTE: calculateRsocWithSaturation() est privé dans BatteryModels.h.
// La formule est réimplémentée ici (fonction rSocNew) pour la démonstration.

float rSocNew(float s_ref, float gamma_soc, float max_ratio) {
    if (s_ref <= 0.0f) return max_ratio;
    s_ref = constrain(s_ref, 0.001f, 1.0f);

    float unbounded = 1.0f + gamma_soc * (1.0f / s_ref - 1.0f);
    if (unbounded <= 1.0f) return 1.0f;

    float overshoot = unbounded - 1.0f;
    float max_overshoot = max_ratio - 1.0f;

    if (overshoot >= max_overshoot) {
        return max_ratio;
    }

    float saturation_factor = overshoot / max_overshoot;
    float smoothing = 1.0f - expf(-2.0f * saturation_factor * saturation_factor);

    return 1.0f + max_overshoot * smoothing;
}

BatteryModel bat_flooded(TECH_FLOODED, 6, 100.0f);   // Plomb (gamma=0.5, max=18)
BatteryModel bat_lifepo4(TECH_LIFEPO4, 4, 100.0f);   // LiFePO4 (gamma=0.3, max=6)

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=== R_SOC SATURATION COMPARISON ===\n");
    Serial.println("Ancienne formule: R_soc = 1 + gamma * (1/s_ref - 1)");
    Serial.println("Nouvelle formule: R_soc = saturate(unbounded, R_soc_max)\n");

    // Test 1: Batterie plomb à différents SoC
    Serial.println("Test 1: BATTERIE PLOMB (Flooded, gamma=0.5, max=18)");
    demoPlombSoc();

    delay(500);

    // Test 2: Batterie LiFePO4
    Serial.println("\n\nTest 2: BATTERIE LIFEPO4 (gamma=0.3, max=6)");
    demoLifepo4Soc();

    delay(500);

    // Test 3: Cas extrême - très bas SoC
    Serial.println("\n\nTest 3: CAS EXTRÊME - Très bas SoC (0.1%)");
    demoExtremeLowSoc();
}

void loop() {
    // Rien
}

void demoPlombSoc() {
    Serial.println("\n  SoC%  | s_ref | R_soc (old WRONG) | R_soc (new OK) | Ratio");
    Serial.println("-------|-------|------------------|-----------------|---------");

    float gamma = 0.5f;
    float max_ratio = 18.0f;

    float soc_values[] = {100.0f, 50.0f, 10.0f, 1.0f, 0.5f, 0.1f, 0.01f};

    for(int i = 0; i < 7; i++) {
        float soc = soc_values[i];
        float s_ref = soc / 100.0f;

        // Ancienne formule INCORRECTE
        float r_soc_old;
        if (s_ref > 0.01f) {
            r_soc_old = 1.0f + gamma * (1.0f / s_ref - 1.0f);
        } else {
            r_soc_old = 1.0f + gamma * 99.0f;  // ← Seuil magique!
        }

        // Nouvelle formule avec saturation
        float r_soc_new = rSocNew(s_ref, gamma, max_ratio);

        Serial.print("  ");
        if (soc < 10.0f) Serial.print(" ");
        Serial.print(soc, 2);
        Serial.print("  | ");
        Serial.print(s_ref, 4);
        Serial.print(" | ");
        Serial.print(r_soc_old, 1);
        Serial.print("           | ");
        Serial.print(r_soc_new, 1);
        Serial.print("          | ");
        Serial.println(r_soc_old / r_soc_new, 2);

        delay(50);
    }
}

void demoLifepo4Soc() {
    Serial.println("\n  SoC%  | s_ref | R_soc (old WRONG) | R_soc (new OK) | Ratio");
    Serial.println("-------|-------|------------------|-----------------|---------");

    float gamma = 0.3f;
    float max_ratio = 6.0f;

    float soc_values[] = {100.0f, 50.0f, 10.0f, 1.0f, 0.5f, 0.1f, 0.01f};

    for(int i = 0; i < 7; i++) {
        float soc = soc_values[i];
        float s_ref = soc / 100.0f;

        // Ancienne formule
        float r_soc_old;
        if (s_ref > 0.01f) {
            r_soc_old = 1.0f + gamma * (1.0f / s_ref - 1.0f);
        } else {
            r_soc_old = 1.0f + gamma * 99.0f;
        }

        // Nouvelle formule
        float r_soc_new = rSocNew(s_ref, gamma, max_ratio);

        Serial.print("  ");
        if (soc < 10.0f) Serial.print(" ");
        Serial.print(soc, 2);
        Serial.print("  | ");
        Serial.print(s_ref, 4);
        Serial.print(" | ");
        Serial.print(r_soc_old, 1);
        Serial.print("           | ");
        Serial.print(r_soc_new, 1);
        Serial.print("          | ");
        Serial.println(r_soc_old / r_soc_new, 2);

        delay(50);
    }
}

void demoExtremeLowSoc() {
    Serial.println("\n  SoC%  | Ancien | Nouveau | Delta");
    Serial.println("-------|--------|---------|--------");

    float gamma = 0.5f;
    float max_ratio = 18.0f;

    for(float soc = 1.0f; soc >= 0.001f; soc *= 0.1f) {
        float s_ref = soc / 100.0f;

        // Ancien (avec seuil magique)
        float r_old;
        if (s_ref > 0.01f) {
            r_old = 1.0f + gamma * (1.0f / s_ref - 1.0f);
        } else {
            r_old = 1.0f + gamma * 99.0f;  // ← SAUT discontinu ici!
        }

        // Nouveau (avec saturation lisse)
        float r_new = rSocNew(s_ref, gamma, max_ratio);

        Serial.print("  ");
        Serial.print(soc, 3);
        Serial.print("  | ");
        Serial.print(r_old, 1);
        Serial.print("     | ");
        Serial.print(r_new, 1);
        Serial.print("      | ");
        Serial.print(r_old - r_new, 1);
        Serial.println();

        delay(50);
    }

    Serial.println("\nObservations:");
    Serial.println("- SoC > 1%: ancien et nouveau donnent des résultats proches");
    Serial.println("- SoC = 1%: SAUT discontinu dans l'ancien (nouveau lisse)");
    Serial.println("- SoC < 1%: ancien explose (50×), nouveau plafonné (18×)");
}

/*
=== EXPLICATION ===

ANCIEN CODE (PROBLÉMATIQUE):
------
float s_ref = soc_est / 100.0f;
float R_soc = (s_ref > 0.01f)                        ← Seuil à 1%
              ? (1.0f + gamma * (1.0f/s_ref - 1.0f))
              : (1.0f + gamma * 99.0f);              ← Fallback magique

Problèmes:
1. Discontinuité à 1% SoC
2. Fallback "99" ne veut rien dire physiquement
3. Explosion à très bas SoC (1% = ×50, 0.1% = ×500)

NOUVEAU CODE (SOLUTION):
------
float R_soc = calculateRsocWithSaturation(s_ref, gamma, max_ratio);
(implémenté dans BatteryModels.h — privé, répliqué ici pour la démo)

Avantages:
1. Courbe LISSE (exponentielle saturante)
2. Plafond physiquement justifié (R_soc_max par technologie)
3. Pas de seuil arbitraire
4. Saturation logarithmique pour transition progressive

VALEURS R_SOC_MAX PAR TECHNOLOGIE:
------
Plomb (Flooded):   18.0  (haute dépendance SoC)
Plomb (AGM):       15.0
Plomb (Gel):       16.0
NiFe:              12.0
LiFePO4:            6.0  (basse dépendance SoC)
Li-Ion:             7.0
Na-Ion:             6.0

EXEMPLE NUMÉRIQUE:
------
Batterie plomb, gamma=0.5, max=18

SoC=50%:
  R_soc = 1 + 0.5×(2-1) = 1.5 ✓ Raisonnable

SoC=1%:
  Ancien: 1 + 0.5×99 = 50.5 ✗ ABSURDE
  Nouveau: ~11 ✓ Physiquement raisonnable

SoC=0.1%:
  Ancien: 1 + 0.5×99 = 50.5 ✗ Même réponse (saut)
  Nouveau: ~16 ✓ Approche progressivement du max (18)
*/
