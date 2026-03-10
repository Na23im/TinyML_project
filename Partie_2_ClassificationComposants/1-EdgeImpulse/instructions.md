# Instructions — Reproduction du projet Edge Impulse

## 1. Créer un projet

1. Aller sur [edgeimpulse.com](https://edgeimpulse.com) → créer un compte
2. New project → nommer le projet `composants_elec`
3. Dashboard → Settings → Labeling method → **"One label per data item"** → Save

## 2. Uploader le dataset

1. Data Acquisition → Upload data
2. Sélectionner **"Select a folder"**
3. Pour chaque classe, répéter :
   - Parcourir → sélectionner le dossier (ex: `Dataset/Capacitor`)
   - Label → taper le nom exact (ex: `Capacitor`)
   - Upload into category → **"Automatically split between training and testing"**
   - Cliquer **"Upload data"**
4. Répéter pour : `Diode`, `Resistor`, `Transistor`, `Unknown`

## 3. Créer l'Impulse

1. Menu → **Create Impulse**
   - Image width : `96` · Image height : `96`
   - Resize mode : `Fit shortest axis`
   - Add processing block → **Image**
   - Add learning block → **Transfer Learning (Images)**
   - Save Impulse

## 4. Configurer le bloc Image

1. Menu → **Image**
   - Color depth : **RGB**
   - Save parameters → **Generate features**
   - Attendre la fin de la génération (~2 min)

## 5. Entraîner le modèle

1. Menu → **Transfer Learning**
   - Model : **MobileNetV1 96x96 0.1**
   - Training cycles : `60`
   - Learning rate : `0.001`
   - Data augmentation : ✅
   - Auto-weight classes : ✅
   - Start training (~5-10 min)

## 6. Déployer sur Arduino

1. Menu → **Deployment**
   - Sélectionner **Arduino library**
   - Inference engine : **EON Compiler**
   - Cliquer **Build**
2. Télécharger le `.zip` généré
3. Dans l'IDE Arduino : Sketch → Include Library → Add .ZIP Library
4. Inclure dans le sketch : `#include <composants_elec_inferencing.h>`
