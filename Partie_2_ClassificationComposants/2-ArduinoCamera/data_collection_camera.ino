/* ============================================================
 *  Partie 2 — Collecte de données images
 *  Arduino Nano 33 BLE + OV7670 → Edge Impulse (via Serial)
 * ============================================================
 *
 *  CORRECTION : suppression de setOutputSize() (non supporté)
 *  On capture en QQVGA (160x120) et on réduit à 96x96.
 *
 *  LIBRAIRIES REQUISES :
 *    - Arduino_OV767X (Arduino Library Manager)
 *
 *  USAGE :
 *    1. Flasher ce sketch
 *    2. Sur PC : edge-impulse-daemon --clean
 *    3. Edge Impulse Dashboard → Data Acquisition → capturer
 * ============================================================ */

#include <Arduino_OV767X.h>

// Résolution de capture native (la plus petite dispo)
#define RAW_WIDTH   160
#define RAW_HEIGHT  120

// Résolution cible Edge Impulse
#define OUT_WIDTH    96
#define OUT_HEIGHT   96

// Buffer capture brute : 160x120 x 2 octets (RGB565)
static uint8_t rawBuffer[RAW_WIDTH * RAW_HEIGHT * 2];

// Buffer réduit : 96x96 x 3 octets (RGB888)
static uint8_t outBuffer[OUT_WIDTH * OUT_HEIGHT * 3];

// ── Prototypes ───────────────────────────────────────────────
void downsampleRGB565toRGB888();
void sendFrame();

// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Edge Impulse Camera Stream");

  // QQVGA = 160x120, RGB565, 1fps
  if (!Camera.begin(QQVGA, RGB565, 1)) {
    Serial.println("ERR: Camera init failed - verifier le cablage OV7670");
    while (1);
  }

  Serial.println("READY");
}

void loop() {
  // Attend la commande "CAPTURE" de edge-impulse-daemon
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "CAPTURE") {
      sendFrame();
    }
  }
}

// ════════════════════════════════════════════════════════════
/**
 * Réduit le buffer 160x120 RGB565 → 96x96 RGB888
 * par sous-échantillonnage nearest-neighbor.
 */
void downsampleRGB565toRGB888() {
  for (int y = 0; y < OUT_HEIGHT; y++) {
    for (int x = 0; x < OUT_WIDTH; x++) {

      // Pixel source dans le buffer brut
      int srcX = x * RAW_WIDTH  / OUT_WIDTH;
      int srcY = y * RAW_HEIGHT / OUT_HEIGHT;
      int srcIdx = (srcY * RAW_WIDTH + srcX) * 2;

      // Lecture RGB565
      uint16_t pixel = ((uint16_t)rawBuffer[srcIdx] << 8) | rawBuffer[srcIdx + 1];

      // Conversion RGB565 → RGB888
      uint8_t r = ((pixel >> 11) & 0x1F) << 3;
      uint8_t g = ((pixel >>  5) & 0x3F) << 2;
      uint8_t b = ( pixel        & 0x1F) << 3;

      // Stockage dans outBuffer
      int dstIdx = (y * OUT_WIDTH + x) * 3;
      outBuffer[dstIdx]     = r;
      outBuffer[dstIdx + 1] = g;
      outBuffer[dstIdx + 2] = b;
    }
  }
}

/**
 * Capture une frame, la réduit, et l'envoie via Serial
 * au format attendu par edge-impulse-daemon.
 */
void sendFrame() {
  Camera.readFrame(rawBuffer);
  downsampleRGB565toRGB888();

  // En-tête
  Serial.print("START:");
  Serial.print(OUT_WIDTH);
  Serial.print("x");
  Serial.print(OUT_HEIGHT);
  Serial.println(":RGB888");

  // Envoi pixel par pixel en hex RRGGBB
  for (int i = 0; i < OUT_WIDTH * OUT_HEIGHT * 3; i += 3) {
    if (outBuffer[i]   < 0x10) Serial.print("0");
    Serial.print(outBuffer[i],   HEX);
    if (outBuffer[i+1] < 0x10) Serial.print("0");
    Serial.print(outBuffer[i+1], HEX);
    if (outBuffer[i+2] < 0x10) Serial.print("0");
    Serial.print(outBuffer[i+2], HEX);
    Serial.print(",");
  }

  Serial.println("\nEND");
}
