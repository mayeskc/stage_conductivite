#include "rcal.h"
#include <Arduino.h>

void select_r_cal(uint8_t sel) {
  digitalWrite(A4, sel & 0x01);
  digitalWrite(A5, (sel >> 1) & 0x01);
}