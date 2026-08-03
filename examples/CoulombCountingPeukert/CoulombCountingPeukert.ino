#include <BatteryModels.h>

// Démonstration du Coulomb counting avec correction Peukert
// La loi de Peukert : capacité réduite à fort courant
// Cet exemple montre la différence entre décharge lente et rapide

BatteryModel battery(TECH_LIFEPO4, 4, 100.0f, 5.0f);

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=== COULOMB COUNTING AVEC PEUKERT ===\n");

    // Test 1: Décharge lente (0.1C = 10A)
    Serial.println("Test 1: Décharge lente (10A) pendant 10 heures");
    demoSlowDischarge();

    battery.resetCoulombCount();
    delay(500);

    // Test 2: Décharge rapide (1C = 100A)
    Serial.println("\nTest 2: Décharge rapide (100A) pendant 1 heure");
    demoFastDischarge();

    battery.resetCoulombCount();
    delay(500);

    // Test 3: Charge (SoC remonte)
    Serial.println("\nTest 3: Charge (50A) pendant 2 heures");
    demoCharge();
}

void loop() {
    // Rien à faire ici
}

void demoSlowDischarge() {
    // Décharge lente: 10A pendant 1h (simul 10 timesteps de 0.1h = 6 minutes)
    float current = -10.0f;  // Négatif = décharge
    float dt_hours = 0.1f;   // 6 minutes par step

    Serial.println("\nTimestep | Current (A) | ΔSoC (%) | Accumulated (Ah) | SoC Coulomb (%) | C_eff (Ah)");
    Serial.println("---------|-------------|---------|------------------|-----------------|----------");

    for (int step = 0; step < 10; step++) {
        battery.updateCoulombCount(current, dt_hours);
        float delta_soc = battery.getLastChargeDelta();
        float soc = battery.getCoulombSoc(100.0f);
        float c_eff = battery.getEffectiveCapacity(current);

        Serial.print("   ");
        Serial.print(step + 1);
        Serial.print("    | ");
        Serial.print(current);
        Serial.print("      | ");
        Serial.print(delta_soc, 2);
        Serial.print("    | ");
        Serial.print(soc, 1);
        Serial.print("          | ");
        Serial.println(c_eff, 1);

        delay(100);
    }
    Serial.println("→ Décharge lente: -1% SoC par step (Peukert negligible à 0.1C)");
}

void demoFastDischarge() {
    // Décharge rapide: 100A pendant 1h (simul 10 timesteps de 0.1h = 6 minutes)
    float current = -100.0f;  // Négatif = décharge
    float dt_hours = 0.1f;    // 6 minutes par step

    Serial.println("\nTimestep | Current (A) | ΔSoC (%) | Accumulated (Ah) | SoC Coulomb (%) | C_eff (Ah)");
    Serial.println("---------|-------------|---------|------------------|-----------------|----------");

    for (int step = 0; step < 10; step++) {
        battery.updateCoulombCount(current, dt_hours);
        float delta_soc = battery.getLastChargeDelta();
        float soc = battery.getCoulombSoc(100.0f);
        float c_eff = battery.getEffectiveCapacity(current);

        Serial.print("   ");
        Serial.print(step + 1);
        Serial.print("    | ");
        Serial.print(current);
        Serial.print("     | ");
        Serial.print(delta_soc, 2);
        Serial.print("    | ");
        Serial.print(soc, 1);
        Serial.print("          | ");
        Serial.println(c_eff, 1);

        delay(100);
    }
    Serial.println("→ Décharge rapide: ~-1.05% SoC par step");
    Serial.println("   (LiFePO4 a faible k_peukert=1.05, donc peu d'effet)");
}

void demoCharge() {
    // Charge: 50A pendant 2h (simul 4 timesteps de 0.5h = 30 minutes)
    float current = 50.0f;   // Positif = charge
    float dt_hours = 0.5f;   // 30 minutes par step

    Serial.println("\nTimestep | Current (A) | ΔSoC (%) | Accumulated (Ah) | SoC Coulomb (%) | C_eff (Ah)");
    Serial.println("---------|-------------|---------|------------------|-----------------|----------");

    // Commençons à 20% SoC
    battery.resetCoulombCount();
    float initial_soc = 20.0f;  // Simule 80% de décharge accumulée

    for (int step = 0; step < 4; step++) {
        battery.updateCoulombCount(current, dt_hours);
        float delta_soc = battery.getLastChargeDelta();
        float soc = battery.getCoulombSoc(initial_soc);
        float c_eff = battery.getEffectiveCapacity(current);

        Serial.print("   ");
        Serial.print(step + 1);
        Serial.print("    | ");
        Serial.print(current);
        Serial.print("       | ");
        Serial.print(delta_soc, 2);
        Serial.print("    | ");
        Serial.print(soc, 1);
        Serial.print("          | ");
        Serial.println(c_eff, 1);

        delay(100);
    }
    Serial.println("→ Charge: +25% SoC par step (25Ah de charge = 25% pour 100Ah)");
}

/*
=== EXPLICATIONS ===

1. COULOMB COUNTING SIMPLE (ANCIEN):
   ΔSoC = (I × Δt) / C_nominal × 100
   Problème: Ignore la loi de Peukert

2. COULOMB COUNTING AVEC PEUKERT (NOUVEAU):
   ΔCharge_eff = I × Δt × (C_nominal / C_eff)
   ΔSoC = ΔCharge_eff / C_nominal × 100

   Exemple avec batterie plomb 100Ah:
   - À 10A (C/10): C_eff=100Ah, ratio=1.0, ΔSoC = -1% par heure
   - À 200A (2C):  C_eff≈25Ah, ratio=4.0, ΔSoC = -4% par heure (2Ah)

   → La même durée de décharge consomme plus de SoC à fort courant
   → Ceci est PHYSIQUEMENT CORRECT (batterie s'épuise plus vite)

3. CAS LITHIUM (LiFePO4):
   k_peukert = 1.05 (très faible)
   → Effet Peukert négligeable
   → Décharge rapide a peu d'effet sur la capacité
   → Effet visible surtout avec batterie plomb (k=1.4)

4. MÉTHODES UTILES:
   - updateCoulombCount(current, dt_hours)  // Accumule charge avec Peukert
   - getLastChargeDelta()                   // ΔSoC du dernier step (%)
   - getCoulombSoc(initial_soc)             // SoC estimé total
   - getEffectiveCapacity(current)          // Capacité à courant donné
   - getPeukertReferenceRate()              // Taux de référence pour Peukert
*/
