// src/message.h
#pragma once

#include <Arduino.h>

typedef struct struct_message {
  int counter;
  float pitch;
  float yaw;
  float roll;
} struct_message;