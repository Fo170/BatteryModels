#include <BatteryModels.h>

// Créer un modèle pour batterie LiFePO4 12.8V (4 cellules) 100Ah
BatteryModel battery(TECH_LIFEPO4, 4, 100.0f);

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.print("Technologie: ");
    Serial.println(battery.getTechnologyName());

    Serial.print("Tension nominale: ");
    Serial.print(battery.getNominalVoltage());
    Serial.println("V");

    Serial.print("Tension float: ");
    Serial.print(battery.getFloatVoltage());
    Serial.println("V");

    Serial.print("Tension absorption: ");
    Serial.print(battery.getAbsorptionVoltage());
    Serial.println("V");
}

void loop() {
    // Simuler des mesures
    float voltage = 13.2f;      // Tension mesurée (V)
    float current = 5.0f;       // Courant (positif = charge)
    float temperature = 25.0f;  // Température °C
    float dt_s = 0.1f;          // Intervalle de mesure (s)

    // 1. Corriger la tension pour obtenir l'OCV (modèle Thévenin)
    float r_int, v_pol;
    float ocv = battery.correctVoltageToOCV(voltage, current, temperature,
                                            dt_s, &r_int, &v_pol);

    // 2. Convertir OCV en SoC
    float soc = battery.ocvToSoc(ocv, temperature);

    // 3. Détecter l'état de charge
    ChargeState state = battery.detectChargeState(voltage, current, 100.0f);

    // Afficher les résultats
    Serial.print("V=");
    Serial.print(voltage);
    Serial.print("V, I=");
    Serial.print(current);
    Serial.print("A, OCV=");
    Serial.print(ocv);
    Serial.print("V, SoC=");
    Serial.print(soc);
    Serial.print("%, État=");
    Serial.println((int)state);

    delay(1000);
}
