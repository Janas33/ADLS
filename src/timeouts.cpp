#include "timeouts.hpp"
#include "includes.h"

std::vector<struct TimeoutData> timeout_registry;

void set_timeout(void f(), const int &time, const char *name) {
  //Serial.println("set_timeout: " + String(name)); 
	timeout_registry.push_back(TimeoutData(f, time, name));
}

void handle_timeouts() {
  TimeoutData *timeoutData;

  //Serial.println("handle_timeouts in  timeoutów łącznie:" + timeout_registry.size()  );
  for (int i = 0; i < timeout_registry.size(); i++) {
    timeoutData = &timeout_registry[i];

    Serial.println("handle_timeouts pętla: " + String(timeoutData->name) + ", milis: " +  String(timeoutData->timeout) + ", curr: " + String(millis()) );
    if (timeoutData->timeout < millis()) {
			timeoutData->f();
			timeout_registry.erase(timeout_registry.begin() + i);
		}
  }
  //Serial.println("handle_timeouts out timeoutów łącznie: " + String(timeout_registry.size()) );
}

void remove_timeout(const char *name) {
  for (int i = 0; i < timeout_registry.size(); i++) {
      timeout_registry[i];
      Serial.println("Weszlo do remove timeout");
			timeout_registry.erase(timeout_registry.begin() + i);
      timeout_registry.clear();
  }
}

TimeoutData* find_timeout(const char *name) { 
  Serial.println("find_timeout for: " + String(name) );
  for (int i = 0; i < timeout_registry.size(); i++) {
    Serial.println("find_timeout got: " + String(timeout_registry[i].name) + " vs " + String(name)  );
    if (strcmp(timeout_registry[i].name, name) == 0)
      return &timeout_registry[i];
  }

  return NULL;
}