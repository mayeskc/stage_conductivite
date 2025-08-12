# 🌊 Mesure de Conductivité à 4 Électrodes avec AD5941 et Arduino

Librairie **Arduino** et firmware pour mesurer la **conductivité de l'eau** via la méthode d'**impédance à 4 électrodes**, basée sur le composant **AD5941** (Analog Devices).  
Développé dans le cadre du projet **Terra Forma** au **LAAS-CNRS**, ce projet allie technologie embarquée et instrumentation environnementale.

---

## 📦 Présentation du matériel

- **Carte** : Adafruit Feather M0 (SAMD21)
- **Front-end analogique** : Module/pont AD5941 (PCB personnalisé)
- **Sonde** : 4 électrodes (injection + et −, mesure + et −)
- **Accessoires** : Résistance de calibration (RCAL), câbles Dupont

---

## ✨ Fonctionnalités principales

- Pilotage du **AD5941** via **SPI** depuis Arduino (AFE, DDS, TIA, DFT configurés)
- **Calibration** avec résistance étalon (RCAL) et calcul de RTIA
- Mesure temps réel de l'**impédance** et conversion en **conductivité**
- Affichage en direct sur **Moniteur Série** (115200 bauds)
- Code modulaire, prêt à l’intégration dans une sonde multiparamètres connectée

---

## 🔌 Schéma de câblage

| Feather M0 Pin     | AD5941 Pin             |
|--------------------|------------------------|
| 3V3                | AVDD, DVDD             |
| GND                | AGND, DGND             |
| MOSI (A5)          | MOSI                   |
| MISO (A6)          | MISO                   |
| SCK (A7)           | SCLK                   |
| A2                 | CS                     |
| A3                 | RESET                  |
| D0                 | WAKEUP                 |
| D1                 | INT                    |

**Connexion électrodes** :

- CE0 → Injection (+)  
- AIN1 → Injection (−)  
- AIN2 → Mesure (+)  
- AIN3 → Mesure (−)  

*(Ajouter ici `docs/schema_connexion.png` pour un visuel complet)*

---

## ⚙️ Installation

1. **Cloner** le dépôt :
   ```bash
   git clone https://github.com/mayeskc/stage_conductivite.git
Ouvrir le projet dans Arduino IDE.

Sélectionner la carte Adafruit Feather M0.

Téléverser le fichier ad5941_conductivity.ino.

▶️ Utilisation
Brancher la sonde et le module AD5941 à la Feather M0.

Ouvrir le Moniteur Série à 115200 bauds.

Plonger la sonde dans la solution.

Lire les valeurs d’impédance et de conductivité affichées en temps réel.

🧪 Calibration
Définir la résistance RCAL dans rcal.h.

Immergez la sonde dans une solution étalon connue.

Ajuster la constante de cellule K dans conductivity.h.

Utiliser les fonctions intégrées pour calibrer et valider la mesure.

📂 Structure du dépôt
bash
Copier
Modifier
├── ad5941_conductivity.ino   # Firmware principal
├── ad5940.c / ad5940.h       # Driver bas-niveau et configuration AD5941
├── rcal.h                    # Valeur de la résistance d’étalonnage
├── conductivity.h            # Constante K, paramètres utilisateur
├── docs/
│   └── schema_connexion.png  # Schéma de câblage
├── LICENSE                   # Licence MIT
📊 Exemple de sortie série
makefile
Copier
Modifier
Impedance: 523.47 Ω
Conductivity: 2.87 mS/cm
