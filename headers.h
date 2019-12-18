#pragma once

// Enums
enum LockState {
  Closed,
  Open,
  Blocked
};

enum RgbColor {
  Red,
  Green,
  Blue,
  Off
};

void changeDoorStateAndDisplay(LockState stan);