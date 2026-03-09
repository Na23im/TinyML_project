/* ============================================================
 *  Partie 2 — Classification de composants électroniques
 *  Arduino Nano 33 BLE + Caméra OV7670 + Edge Impulse + BLE
 * ============================================================
 *
 *  LIBRAIRIES REQUISES (Arduino Library Manager) :
 *    1. Arduino_OV767X          (caméra OV7670)
 *    2. ArduinoBLE              (communication Bluetooth)
 *    3. <nom_projet>_inferencing (export Edge Impulse → .zip → Add .ZIP Library)
 *
 *  CÂBLAGE OV7670 → Arduino Nano 33 BLE :
 *    OV7670  | Arduino
 *    --------|--------
 *    3.3V    | 3.3V
 *    GND     | GND
 *    SIOC    | A5  (SCL)
 *    SIOD    | A4  (SDA)
 *    VSYNC   | 8
 *    HREF    | A1
 *    PCLK    | A0
 *    XCLK    | 9
 *    D7-D0   | 2,3,4,5,6,7,A2,A3
 *
 *  WORKFLOW :
 *    1. Caméra capture une image 96x96 (format RGB565)
 *    2. Image prétraitée et passée au classifieur Edge Impulse
 *    3. Classe prédite envoyée via BLE (Serial aussi pour debug)
 * ============================================================ */

// ── 1. INCLUDES ──────────────────────────────────────────────
#include <Arduino_OV767X.h>
#include <ArduinoBLE.h>

// ⚠️  Remplace "mon_projet_inferencing.h" par le nom exact de ton export Edge Impulse
// Ex : si ton projet s'appelle "composants_ei", ce sera "composants_ei_inferencing.h"
#include "mon_projet_inferencing.h"

// ── 2. CONSTANTES ────────────────────────────────────────────
// Dimensions d'entrée du modèle Edge Impulse (96x96 par défaut pour image classification)
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS   96
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS   96
#define EI_CAMERA_FRAME_BYTE_SIZE         2    // RGB565 = 2 octets/pixel

// Seuil de confiance minimum pour accepter une prédiction
#define CONFIDENCE_THRESHOLD 0.6f

// ── 3. BLE : SERVICE & CARACTÉRISTIQUE ───────────────────────
// UUID fixes — tu peux les garder tels quels
BLEService componentService("12345678-1234-1234-1234-123456789012");
BLEStringCharacteristic componentCharacteristic(
  "12345678-1234-1234-1234-123456789013",
  BLERead | BLENotify,
  20   // longueur max de la string (ex: "Resistance")
);

// ── 4. BUFFER IMAGE ──────────────────────────────────────────
static uint8_t imageBuffer[EI_CAMERA_RAW_FRAME_BUFFER_COLS *
                            EI_CAMERA_RAW_FRAME_BUFFER_ROWS *
                            EI_CAMERA_FRAME_BYTE_SIZE];

// ── 5. PROTOTYPES ────────────────────────────────────────────
bool captureImage();
int  ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
void printPrediction(ei_impulse_result_t &result);
String getBestLabel(ei_impulse_result_t &result);


// ═══════════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  while (!Serial);   // Attend l'ouverture du moniteur série (retirer en production)

  Serial.println("=== Partie 2 : Classification de composants ===");

  // ── Init caméra ──────────────────────────────────────────────
  Serial.print("Init caméra OV7670... ");
  if (!Camera.begin(QCIF, RGB565, 1)) {
    Serial.println("ERREUR : caméra non détectée !");
    Serial.println("Vérifier le câblage OV7670.");
    while (1);
  }
  // Redimensionner à 96x96 pour Edge Impulse
  Camera.setOutputSize(EI_CAMERA_RAW_FRAME_BUFFER_COLS,
                       EI_CAMERA_RAW_FRAME_BUFFER_ROWS);
  Serial.println("OK");

  // ── Init BLE ─────────────────────────────────────────────────
  Serial.print("Init BLE... ");
  if (!BLE.begin()) {
    Serial.println("ERREUR : BLE non disponible !");
    while (1);
  }
  BLE.setLocalName("ComponentClassifier");
  BLE.setAdvertisedService(componentService);
  componentService.addCharacteristic(componentCharacteristic);
  BLE.addService(componentService);
  componentCharacteristic.writeValue("Initialisation...");
  BLE.advertise();
  Serial.println("OK — en écoute BLE");

  Serial.println("\nPrêt. Démarrage de la classification...\n");
  delay(500);
}


// ═══════════════════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════════════════
void loop() {
  // Traite les connexions BLE entrantes
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connecté à : ");
    Serial.println(central.address());
  }

  // ── Capture l'image ──────────────────────────────────────────
  if (!captureImage()) {
    Serial.println("Erreur de capture image, retry...");
    delay(500);
    return;
  }

  // ── Prépare le signal pour Edge Impulse ──────────────────────
  signal_t signal;
  signal.total_length = EI_CAMERA_RAW_FRAME_BUFFER_COLS *
                        EI_CAMERA_RAW_FRAME_BUFFER_ROWS;
  signal.get_data = &ei_camera_get_data;

  // ── Lancement de l'inférence ─────────────────────────────────
  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);

  if (err != EI_IMPULSE_OK) {
    Serial.print("Erreur classifieur : ");
    Serial.println(err);
    delay(1000);
    return;
  }

  // ── Affichage & envoi du résultat ────────────────────────────
  printPrediction(result);

  String label = getBestLabel(result);
  if (label != "") {
    // Envoi via Serial (pour Node-RED en mode Serial si besoin)
    Serial.print(">> CLASSE DETECTEE : ");
    Serial.println(label);

    // Envoi via BLE
    componentCharacteristic.writeValue(label);
  } else {
    Serial.println(">> Confiance insuffisante, pas d'envoi.");
  }

  // Pause entre deux captures (ajuster selon la vitesse de défilement des composants)
  delay(2000);
}


// ═══════════════════════════════════════════════════════════════
//  FONCTIONS UTILITAIRES
// ═══════════════════════════════════════════════════════════════

/**
 * Capture une image depuis l'OV7670 dans imageBuffer.
 * Retourne true si succès, false sinon.
 */
bool captureImage() {
  Camera.readFrame(imageBuffer);
  return true;
}

/**
 * Callback requis par Edge Impulse pour lire les pixels.
 * Convertit RGB565 en float RGB888 normalisé [0..255].
 */
int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
  size_t pixel_ix  = offset * 2;   // RGB565 = 2 octets par pixel
  size_t pixels_left = length;
  size_t out_ptr_ix = 0;

  while (pixels_left != 0) {
    // Lecture des 2 octets RGB565
    uint16_t pixel = (imageBuffer[pixel_ix] << 8) | imageBuffer[pixel_ix + 1];

    // Extraction des canaux RGB
    uint8_t r = ((pixel >> 11) & 0x1F) << 3;  // 5 bits → 8 bits
    uint8_t g = ((pixel >>  5) & 0x3F) << 2;  // 6 bits → 8 bits
    uint8_t b = ( pixel        & 0x1F) << 3;  // 5 bits → 8 bits

    // Format attendu par Edge Impulse : 0xRRGGBB en float
    out_ptr[out_ptr_ix] = (r << 16) | (g << 8) | b;

    out_ptr_ix++;
    pixel_ix += 2;
    pixels_left--;
  }
  return 0;
}

/**
 * Affiche toutes les probabilités dans le moniteur série.
 */
void printPrediction(ei_impulse_result_t &result) {
  Serial.println("--- Résultats ---");
  for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    Serial.print("  ");
    Serial.print(result.classification[i].label);
    Serial.print(" : ");
    Serial.print(result.classification[i].value * 100, 1);
    Serial.println(" %");
  }
  Serial.print("  Timing DSP: ");
  Serial.print(result.timing.dsp);
  Serial.print(" ms | Inférence: ");
  Serial.print(result.timing.classification);
  Serial.println(" ms");
  Serial.println("-----------------");
}

/**
 * Retourne la classe ayant la plus haute confiance (si > CONFIDENCE_THRESHOLD).
 * Retourne "" si aucune classe n'est assez confiante.
 */
String getBestLabel(ei_impulse_result_t &result) {
  float  maxConf  = 0.0f;
  String bestLabel = "";

  for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > maxConf) {
      maxConf   = result.classification[i].value;
      bestLabel = String(result.classification[i].label);
    }
  }

  if (maxConf >= CONFIDENCE_THRESHOLD) {
    return bestLabel;
  }
  return "";
}
