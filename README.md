# TinyML Project — Classification de vibrations & composants électroniques

Projet réalisé dans le cadre du cours d'instrumentation (Arduino Nano 33 BLE).

## Structure du repo

```
TinyML_project/
├── PARTIE_1_ClassificationVibrations/   → Classification IMU (idle / shake)
└── Partie_2_ClassificationComposants/  → Classification images (Edge Impulse)
```

---

## Partie 1 — Classification de vibrations

Collecte de données IMU via Arduino Nano 33 BLE, entraînement d'un réseau de neurones dense avec TensorFlow Lite, inférence en temps réel sur la carte.

**Classes :** `idle` · `shake`  
**Accuracy :** 80%  
**Modèle :** Dense 64→32→2 (TFLite quantized int8)

## Partie 2 — Classification de composants électroniques

Classification d'images de composants électroniques via caméra OV7670, modèle entraîné sur Edge Impulse (MobileNetV1 96x96), inférence sur Arduino + envoi BLE vers dashboard Node-RED.

**Classes :** `Capacitor` · `Diode` · `Resistor` · `Transistor` · `Unknown`  
**Accuracy :** 58.4%  
**Modèle :** MobileNetV1 96x96 0.1 (Edge Impulse)

---

## Matériel requis

- Arduino Nano 33 BLE
- Caméra OV7670
- PC avec Arduino IDE + Node-RED

## Librairies Arduino

| Librairie | Usage |
|---|---|
| Arduino_LSM9DS1 | IMU Partie 1 |
| Arduino_TensorFlowLite | Inférence TFLite Partie 1 |
| Arduino_OV767X | Caméra Partie 2 |
| ArduinoBLE | Communication BLE Partie 2 |
| ei-composants_elec (zip) | Modèle Edge Impulse Partie 2 |
