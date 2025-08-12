# Water Conductivity Measurement with AD5941 and Feather M0

## 📌 Description
This project provides an **Arduino library** and **firmware** to measure water electrical conductivity using the **AD5941 analog front-end** (Analog Devices) and a **4-electrode probe**.  
It is part of the **Terra Forma project**, aimed at continuous environmental monitoring.

---

## 🔬 Background
Electrical conductivity is a key indicator of water quality (pollution, salinity, groundwater inflows).  
The **4-electrode method** reduces polarization effects and improves accuracy, especially in low-conductivity environments.

---

## 🛠 Required Hardware
- **Adafruit Feather M0** board  
- **AD5941 module** on a custom PCB  
- **4-electrode conductivity probe**  
- Calibration resistor (**RCAL**, e.g. 10 kΩ)  
- SPI connections (MOSI, MISO, SCLK, CS, RESET, WAKEUP, INT)  
- 3.3 V power supply  

---

## 💻 Installation
1. **Clone the repository**:
   ```bash
   git clone https://github.com/mayeskc/stage_conductivite.git
2. Open ad5941_conductivity.ino in the Arduino IDE.

3. Select Adafruit Feather M0 as the board and the correct serial port.

4. Upload the program.
   
---

▶ Usage
1. Connect the hardware according to the PCB wiring.

2. Update the RCAL value (rcal.h) and cell constant K (conductivity.h).

3. Open the Serial Monitor at 115200 baud to view conductivity readings.

4. Immerse the probe into the water sample.
   
---

📏 Calibration
1. Connect the RCAL resistor between the measurement electrodes.

2. Run the program to compute the calibrated RTIA.

3. Determine the cell constant K using known standard solutions.

4. Update conductivity.h with the measured constant.
   
---

📂 Code Structure

   ad5940.c / .h – SPI communication and AD5941 configuration

   conductivity.h – user parameters (K, frequency, etc.)

   rcal.h – calibration resistor value

   ad5941_conductivity.ino – main loop and data acquisition
