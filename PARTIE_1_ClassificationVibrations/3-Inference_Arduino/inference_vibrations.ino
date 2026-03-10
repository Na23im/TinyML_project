/* ============================================================
 *  Partie 1 — Inférence : Classification de vibrations
 *  Arduino Nano 33 BLE — IMU + TensorFlow Lite
 * ============================================================
 *
 *  LIBRAIRIES REQUISES :
 *    - Arduino_LSM9DS1        (Arduino Library Manager)
 *    - Arduino_TensorFlowLite (Arduino Library Manager)
 *
 *  AVANT DE FLASHER :
 *    Copier vibrations_model.h dans ce même dossier
 *
 *  CLASSES :
 *    0 = idle
 *    1 = shake
 * ============================================================ */

#include <Arduino_LSM9DS1.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include "vibrations_model.h"

// ── Paramètres (doivent correspondre au notebook) ────────────
#define WINDOW_SIZE          50
#define N_FEATURES            3    // aX, aY, aZ
#define N_CLASSES             2
#define SAMPLE_RATE_MS       20    // 50 Hz
#define CONFIDENCE_THRESHOLD  0.8f

const char* CLASS_NAMES[] = {"idle", "shake"};

// ── Valeurs de normalisation issues du notebook ──────────────
const float GLOBAL_MIN = -1.2000f;
const float GLOBAL_MAX =  3.0000f;

// ── TFLite ───────────────────────────────────────────────────
const int TENSOR_ARENA_SIZE = 8 * 1024;
uint8_t tensor_arena[TENSOR_ARENA_SIZE];

tflite::AllOpsResolver     resolver;
const tflite::Model*       model_tflite = nullptr;
tflite::MicroInterpreter*  interpreter  = nullptr;
TfLiteTensor*              input        = nullptr;
TfLiteTensor*              output       = nullptr;

// ── Buffer fenêtre ───────────────────────────────────────────
float imu_window[WINDOW_SIZE * N_FEATURES];
int   sample_idx = 0;

// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("=== Partie 1 : Inférence vibrations ===");

  if (!IMU.begin()) {
    Serial.println("ERR: IMU non détecté !");
    while (1);
  }
  Serial.println("IMU OK");

  model_tflite = tflite::GetModel(vibrations_model);
  interpreter  = new tflite::MicroInterpreter(
    model_tflite, resolver, tensor_arena, TENSOR_ARENA_SIZE
  );
  interpreter->AllocateTensors();

  input  = interpreter->input(0);
  output = interpreter->output(0);

  Serial.println("TFLite OK — Classification en cours...\n");
}

// ════════════════════════════════════════════════════════════
void loop() {
  if (!IMU.accelerationAvailable()) return;

  float aX, aY, aZ;
  IMU.readAcceleration(aX, aY, aZ);

  int base = sample_idx * N_FEATURES;
  imu_window[base + 0] = aX;
  imu_window[base + 1] = aY;
  imu_window[base + 2] = aZ;
  sample_idx++;

  if (sample_idx >= WINDOW_SIZE) {
    sample_idx = 0;

    float in_scale = input->params.scale;
    int   in_zp    = input->params.zero_point;

    for (int i = 0; i < WINDOW_SIZE * N_FEATURES; i++) {
      float norm = (imu_window[i] - GLOBAL_MIN) / (GLOBAL_MAX - GLOBAL_MIN);
      norm = constrain(norm, 0.0f, 1.0f);
      input->data.int8[i] = (int8_t)(norm / in_scale + in_zp);
    }

    interpreter->Invoke();

    float out_scale = output->params.scale;
    int   out_zp    = output->params.zero_point;

    float maxProb   = 0;
    int   bestClass = 0;

    Serial.println("--- Résultats ---");
    for (int i = 0; i < N_CLASSES; i++) {
      float prob = (output->data.int8[i] - out_zp) * out_scale;
      Serial.print("  "); Serial.print(CLASS_NAMES[i]);
      Serial.print(": "); Serial.print(prob * 100, 1);
      Serial.println("%");
      if (prob > maxProb) { maxProb = prob; bestClass = i; }
    }

    if (maxProb >= CONFIDENCE_THRESHOLD) {
      Serial.print(">> "); Serial.print(CLASS_NAMES[bestClass]);
      Serial.print(" ("); Serial.print(maxProb * 100, 1);
      Serial.println("%)");
    } else {
      Serial.println(">> Incertain");
    }
    Serial.println();
  }

  delay(SAMPLE_RATE_MS);
}
