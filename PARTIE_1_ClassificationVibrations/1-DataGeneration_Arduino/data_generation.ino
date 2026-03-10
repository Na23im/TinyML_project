/* ============================================================
 *  Partie 1 — Collecte de données IMU
 *  Arduino Nano 33 BLE — LSM9DS1 (IMU intégré)
 * ============================================================
 *
 *  LIBRAIRIES REQUISES :
 *    - Arduino_LSM9DS1 (Arduino Library Manager)
 *
 *  CLASSES :
 *    0 = idle       (carte posée, immobile)
 *    1 = shake_slow (secousse lente)
 *    2 = shake_fast (secousse rapide)
 *
 *  USAGE :
 *    1. Flasher ce sketch
 *    2. Ouvrir le moniteur série à 115200 bauds
 *    3. Taper dans le moniteur série :
 *       - "0" → enregistre 1 fenêtre de classe idle
 *       - "1" → enregistre 1 fenêtre de classe shake_slow
 *       - "2" → enregistre 1 fenêtre de classe shake_fast
 *    4. Copier-coller les lignes CSV dans un fichier .csv sur PC
 *
 *  FORMAT DE SORTIE (CSV) :
 *    label,aX,aY,aZ,gX,gY,gZ  (répété WINDOW_SIZE fois)
 * ============================================================ */

#include <Arduino_LSM9DS1.h>

// ── Paramètres de la fenêtre ─────────────────────────────────
#define WINDOW_SIZE     50    // Nombre de samples par fenêtre
#define SAMPLE_RATE_MS  20    // 1 sample toutes les 20ms → 50Hz

// ── Variables ────────────────────────────────────────────────
int     currentLabel = -1;
bool    recording    = false;
int     sampleCount  = 0;

// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("=== Partie 1 : Collecte données IMU ===");
  Serial.println("Commandes : tapez 0 (idle) | 1 (shake_slow) | 2 (shake_fast)");
  Serial.println("En-tête CSV : label,aX,aY,aZ,gX,gY,gZ");
  Serial.println();

  if (!IMU.begin()) {
    Serial.println("ERR: IMU non détecté !");
    while (1);
  }
  Serial.println("IMU OK — prêt à enregistrer.");
}

// ════════════════════════════════════════════════════════════
void loop() {
  // ── Lecture commande Serial ───────────────────────────────
  if (Serial.available() && !recording) {
    char cmd = Serial.read();
    if (cmd == '0' || cmd == '1' || cmd == '2') {
      currentLabel = cmd - '0';
      recording    = true;
      sampleCount  = 0;

      String labelName = (currentLabel == 0) ? "idle" :
                         (currentLabel == 1) ? "shake_slow" : "shake_fast";
      Serial.print(">> Enregistrement classe : ");
      Serial.print(labelName);
      Serial.print(" (");
      Serial.print(WINDOW_SIZE);
      Serial.println(" samples)");
    }
  }

  // ── Enregistrement ────────────────────────────────────────
  if (recording) {
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      float aX, aY, aZ, gX, gY, gZ;
      IMU.readAcceleration(aX, aY, aZ);
      IMU.readGyroscope(gX, gY, gZ);

      // Format CSV : label,aX,aY,aZ,gX,gY,gZ
      Serial.print(currentLabel); Serial.print(",");
      Serial.print(aX, 4);       Serial.print(",");
      Serial.print(aY, 4);       Serial.print(",");
      Serial.print(aZ, 4);       Serial.print(",");
      Serial.print(gX, 4);       Serial.print(",");
      Serial.print(gY, 4);       Serial.print(",");
      Serial.println(gZ, 4);

      sampleCount++;
      if (sampleCount >= WINDOW_SIZE) {
        recording = false;
        Serial.println(">> Fenetre complete. Tapez 0/1/2 pour continuer.");
        Serial.println();
      }

      delay(SAMPLE_RATE_MS);
    }
  }
}
