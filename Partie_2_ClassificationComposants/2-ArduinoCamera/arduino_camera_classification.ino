/* ============================================================
 *  Partie 2 — Inférence : Classification de composants
 *  Arduino Nano 33 BLE + OV7670 + Edge Impulse + BLE
 * ============================================================
 *
 *  LIBRAIRIES REQUISES :
 *    1. Arduino_OV767X          (Arduino Library Manager)
 *    2. ArduinoBLE              (Arduino Library Manager)
 *    3. ei-composants_elec-arduino-1.0.1.zip
 *       → Sketch → Include Library → Add .ZIP Library
 *
 *  CLASSES DÉTECTÉES :
 *    - Capacitor
 *    - Diode
 *    - Resistor
 *    - Transistor
 *    - Unknown
 * ============================================================ */

#include <Arduino_OV767X.h>
#include <ArduinoBLE.h>
#include <composants_elec_inferencing.h>

// ── Résolution ───────────────────────────────────────────────
#define RAW_WIDTH   160
#define RAW_HEIGHT  120
#define OUT_WIDTH    96
#define OUT_HEIGHT   96

// ── Seuil de confiance ───────────────────────────────────────
#define CONFIDENCE_THRESHOLD 0.5f

// ── Buffers ──────────────────────────────────────────────────
static uint8_t rawBuffer[RAW_WIDTH * RAW_HEIGHT * 2];
static uint8_t outBuffer[OUT_WIDTH * OUT_HEIGHT * 3];

// ── BLE ──────────────────────────────────────────────────────
BLEService componentService("12345678-1234-1234-1234-123456789012");
BLEStringCharacteristic componentCharacteristic(
  "12345678-1234-1234-1234-123456789013",
  BLERead | BLENotify,
  20
);

// ── Prototypes ───────────────────────────────────────────────
void downsample();
int  ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
void printPrediction(ei_impulse_result_t &result);
String getBestLabel(ei_impulse_result_t &result);


// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("=== Partie 2 : Classification de composants ===");

  // Init caméra
  if (!Camera.begin(QQVGA, RGB565, 1)) {
    Serial.println("ERR: Camera init failed");
    while (1);
  }
  Serial.println("Camera OK");

  // Init BLE
  if (!BLE.begin()) {
    Serial.println("ERR: BLE init failed");
    while (1);
  }
  BLE.setLocalName("ComponentClassifier");
  BLE.setAdvertisedService(componentService);
  componentService.addCharacteristic(componentCharacteristic);
  BLE.addService(componentService);
  componentCharacteristic.writeValue("Initialisation...");
  BLE.advertise();
  Serial.println("BLE OK — en attente de connexion");
  Serial.println("Pret.\n");
}

// ════════════════════════════════════════════════════════════
void loop() {
  BLE.poll();

  // Capture
  Camera.readFrame(rawBuffer);
  downsample();

  // Signal Edge Impulse
  signal_t signal;
  signal.total_length = OUT_WIDTH * OUT_HEIGHT;
  signal.get_data = &ei_camera_get_data;

  // Inférence
  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
  if (err != EI_IMPULSE_OK) {
    Serial.print("Erreur classifieur: ");
    Serial.println(err);
    delay(1000);
    return;
  }

  // Résultats
  printPrediction(result);
  String label = getBestLabel(result);

  if (label != "") {
    Serial.print(">> CLASSE DETECTEE : ");
    Serial.println(label);
    componentCharacteristic.writeValue(label);
  } else {
    Serial.println(">> Confiance insuffisante");
  }

  delay(2000);
}

// ════════════════════════════════════════════════════════════
void downsample() {
  for (int y = 0; y < OUT_HEIGHT; y++) {
    for (int x = 0; x < OUT_WIDTH; x++) {
      int srcX = x * RAW_WIDTH  / OUT_WIDTH;
      int srcY = y * RAW_HEIGHT / OUT_HEIGHT;
      int srcIdx = (srcY * RAW_WIDTH + srcX) * 2;

      uint16_t pixel = ((uint16_t)rawBuffer[srcIdx] << 8) | rawBuffer[srcIdx + 1];
      uint8_t r = ((pixel >> 11) & 0x1F) << 3;
      uint8_t g = ((pixel >>  5) & 0x3F) << 2;
      uint8_t b = ( pixel        & 0x1F) << 3;

      int dstIdx = (y * OUT_WIDTH + x) * 3;
      outBuffer[dstIdx]     = r;
      outBuffer[dstIdx + 1] = g;
      outBuffer[dstIdx + 2] = b;
    }
  }
}

int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
  for (size_t i = 0; i < length; i++) {
    int idx = (offset + i) * 3;
    out_ptr[i] = (outBuffer[idx] << 16) | (outBuffer[idx+1] << 8) | outBuffer[idx+2];
  }
  return 0;
}

void printPrediction(ei_impulse_result_t &result) {
  Serial.println("--- Résultats ---");
  for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    Serial.print("  ");
    Serial.print(result.classification[i].label);
    Serial.print(": ");
    Serial.print(result.classification[i].value * 100, 1);
    Serial.println("%");
  }
  Serial.println("-----------------");
}

String getBestLabel(ei_impulse_result_t &result) {
  float  maxConf   = 0.0f;
  String bestLabel = "";
  for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > maxConf) {
      maxConf   = result.classification[i].value;
      bestLabel = String(result.classification[i].label);
    }
  }
  return (maxConf >= CONFIDENCE_THRESHOLD) ? bestLabel : "";
}
