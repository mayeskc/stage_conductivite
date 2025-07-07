#include "rcal.h"
#include <Arduino.h>

void select_r_cal(uint8_t sel) {
    pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(6, sel & 0x01);
  digitalWrite(5, (sel >> 1) & 0x01);
}