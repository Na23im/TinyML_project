# Documentation — Partie 2 : Classification de composants électroniques

## 1. Objectif

Reconnaître des composants électroniques défilant devant la caméra OV7670 et transmettre les résultats via BLE à un dashboard Node-RED pour le comptage.

## 2. Matériel & librairies

| Élément | Détail |
|---|---|
| Carte | Arduino Nano 33 BLE |
| Caméra | OV7670 |
| Librairie caméra | Arduino_OV767X |
| Librairie BLE | ArduinoBLE |
| Librairie ML | ei-composants_elec-arduino-1.0.1.zip (Edge Impulse) |

## 3. Câblage OV7670 → Arduino Nano 33 BLE

| OV7670 | Arduino Nano 33 BLE |
|---|---|
| 3.3V | 3.3V |
| GND | GND |
| SIOC | A5 (SCL) |
| SIOD | A4 (SDA) |
| VSYNC | 8 |
| HREF | A1 |
| PCLK | A0 |
| XCLK | 9 |
| D7→D0 | 2,3,4,5,6,7,A2,A3 |

## 4. Dataset

- **Source :** repo GitHub `aitnouriarslan-svg/Projet_TinyML`
- **Classes :** `Capacitor` · `Diode` · `Resistor` · `Transistor` · `Unknown`
- **Total :** 480 images (96 par classe)
- **Stocké dans :** `Dataset/`

## 5. Entraînement (Edge Impulse)

Projet : https://studio.edgeimpulse.com/studio/924921

### Pipeline Edge Impulse
1. Upload des images par classe (Data Acquisition → Upload data)
2. Create Impulse : Image 96×96 RGB → Processing: Image → Learning: Transfer Learning
3. Generate features (bloc Image)
4. Entraînement : MobileNetV1 96x96 0.1 · 60 cycles · lr=0.001 · Data augmentation ✓
5. Déploiement : Arduino Library → Build → `ei-composants_elec-arduino-1.0.1.zip`

### Résultats

| Métrique | Valeur |
|---|---|
| Accuracy | **58.4%** |
| Meilleure classe | Unknown (100%) |
| Classe difficile | Capacitor/Transistor (confusion visuelle) |

> Note : l'accuracy limitée s'explique par la similarité visuelle entre Capacitor et Transistor dans le dataset source.

## 6. Inférence sur Arduino

Sketch : `2-ArduinoCamera/arduino_camera_classification.ino`

- Capture QQVGA (160×120) → réduction nearest-neighbor → 96×96
- Conversion RGB565 → RGB888
- Inférence Edge Impulse → classe prédite
- Envoi via BLE + Serial
- Seuil de confiance : **50%**

### Comment reproduire

1. Installer `Arduino_OV767X` et `ArduinoBLE` via Library Manager
2. Ajouter `ei-composants_elec-arduino-1.0.1.zip` via Sketch → Add .ZIP Library
3. Flasher `arduino_camera_classification.ino`

## 7. Dashboard Node-RED

Flow : `3-NodeRED/flows.json`

### Installation
1. Installer Node-RED : `npm install -g node-red`
2. Installer le dashboard : `npm install node-red-dashboard`
3. Lancer : `node-red`
4. Ouvrir `http://localhost:1880`
5. Menu → Import → coller le contenu de `flows.json`
6. Brancher le port Serial de l'Arduino dans le nœud "Arduino Serial"
7. Deploy → ouvrir `http://localhost:1880/ui`

### Fonctionnement
- Réception Serial → extraction de la classe
- Incrémentation du compteur par classe
- Affichage sur 4 jauges + graphique historique
- Bouton Reset pour remettre les compteurs à zéro
