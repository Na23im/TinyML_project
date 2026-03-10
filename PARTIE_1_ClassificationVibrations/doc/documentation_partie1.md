# Documentation — Partie 1 : Classification de vibrations

## 1. Objectif

Classifier en temps réel deux types de mouvements détectés par l'IMU intégré de l'Arduino Nano 33 BLE :
- **idle** : carte posée, immobile
- **shake** : carte secouée

## 2. Matériel & librairies

| Élément | Détail |
|---|---|
| Carte | Arduino Nano 33 BLE |
| Capteur | IMU LSM9DS1 (intégré) |
| Axes utilisés | aX, aY, aZ (accéléromètre) |
| Librairie IMU | Arduino_LSM9DS1 |
| Librairie ML | Arduino_TensorFlowLite |

## 3. Collecte des données

Sketch : `1-DataGeneration_Arduino/data_generation.ino`

- Fréquence d'échantillonnage : **50 Hz** (1 sample toutes les 20ms)
- Fenêtre : **50 samples** → 1 seconde de données
- Format CSV : `aX;aY;aZ`
- Données collectées :
  - `repos.csv` → 2486 lignes (classe idle)
  - `vibration.csv` → 7022 lignes (classe shake)

## 4. Entraînement

Notebook : `2-Training/notebooks/training_vibrations.ipynb`  
Exécuté sur **Google Colab**.

### Pipeline
1. Chargement des 2 CSV
2. Création de fenêtres de 50 samples (150 features)
3. Rééquilibrage des classes (undersampling shake → 49 fenêtres chaque)
4. Normalisation min-max → [0, 1]
5. Split 80/20 train/test
6. Entraînement réseau dense : 64 → Dropout(0.2) → 32 → Dropout(0.2) → 2 (softmax)
7. Export TFLite quantized int8 → tableau C

### Résultats

| Métrique | Valeur |
|---|---|
| Test Accuracy | **80%** |
| idle precision | 0.71 |
| shake precision | 1.00 |

### Valeurs de normalisation
```
GLOBAL_MIN = -1.2000
GLOBAL_MAX =  3.0000
```

## 5. Inférence sur Arduino

Sketch : `3-Inference_Arduino/inference_vibrations.ino`

- Lit en continu les données IMU
- Remplit une fenêtre de 50 samples
- Normalise et quantize → passe dans le modèle TFLite
- Affiche la classe prédite via Serial (115200 bauds)
- Seuil de confiance : **80%**

### Comment reproduire

1. Installer `Arduino_LSM9DS1` et `Arduino_TensorFlowLite` via le Library Manager
2. Copier `vibrations_model.h` dans le dossier du sketch
3. Flasher `inference_vibrations.ino` sur l'Arduino Nano 33 BLE
4. Ouvrir le moniteur série à **115200 bauds**
5. Secouer ou poser la carte → la classe s'affiche en temps réel
