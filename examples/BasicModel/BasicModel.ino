#include <BatteryModels.h>

BatteryModel battery(TECH_LIFEPO4, 4, 100.0f);

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=== EXEMPLE DE BASE BatteryModels ===\n");
    Serial.print("Technologie: ");
    Serial.println(battery.getTechnologyName());
    Serial.print("Tension nominale: ");
    Serial.print(battery.getNominalVoltage());
    Serial.println(" V");

    Serial.print("Float: ");
    Serial.print(battery.getFloatVoltage());
    Serial.println(" V");
    Serial.print("Absorption: ");
    Serial.print(battery.getAbsorptionVoltage());
    Serial.println(" V\n");
}

void loop() {
    float voltage = 13.2f;
    float current = 5.0f;
    float temp = 25.0f;

    // Corriger la tension mesurée en OCV (modèle Thévenin)
    float ocv = battery.correctVoltageToOCV(voltage, current, temp, 1.0f);

    // Obtenir le SoC depuis l'OCV corrigée
    float soc = battery.ocvToSoc(ocv, temp);

    // Détecter l'état de charge
    ChargeState state = battery.detectChargeState(voltage, current, 100.0f);

    Serial.print("V = ");
    Serial.print(voltage);
    Serial.print(" V | OCV = ");
    Serial.print(ocv);
    Serial.print(" V | SoC = ");
    Serial.print(soc);
    Serial.println(" %");

    delay(1000);
}
