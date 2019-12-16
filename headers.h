#pragma once

// Enums
enum LockState {
  Closed,
  Open,
  Blocked
};

void changeDoorStateAndDisplay(LockState stan);