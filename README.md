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
