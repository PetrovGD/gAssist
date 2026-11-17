#include <Arduino.h>

static const uint8_t bleReportMap[] = {
  0x05, 0x0C,        // Usage page: Consumer
  0x09, 0x01,        // Usage: Consumer Control
  0xA1, 0x01,        // Collection start
  0x85, 0x01,        // Report ID 1
  0x15, 0x00,        // Logical min 0
  0x25, 0x01,        // Logical max 1
  0x75, 0x01,        // 1 bit per value
  0x95, 0x04,        // 4 values
  0x09, 0xE9,        // bit 0 (0x01): Volume Up
  0x09, 0xB6,        // bit 1 (0x02): Previous Song
  0x09, 0xB5,        // bit 2 (0x04): Next Song
  0x09, 0xCD,        // bit 3 (0x08): Play/Pause
  0x81, 0x02,        // Input data
  0x95, 0x04,        // 4 padding bits
  0x81, 0x03,        // Input constant (padding)
  0xC0               // Collection end
};