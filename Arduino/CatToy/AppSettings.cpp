#include "AppSettings.h"
#include <FS.h>

#define SETTINGS_PATH "settings"

void AppSettings::init() {
  bool success = SPIFFS.begin();
  if (!success) {
    Serial.println("Could not open file system");
  }
  load();
}

char* AppSettings::get(int key) {
  return _settings[key];
}

void AppSettings::set(int key, char* data) {
  free(_settings[key]);
  if (data) {
    _settings[key] = strdup(data);
  } else {
    _settings[key] = NULL;
  }
}


void AppSettings::load() {
  Serial.println("loading settings");
  memset(_settings, NULL, AppSettings::SETTING_COUNT * sizeof(char *));
  if (SPIFFS.exists(SETTINGS_PATH)) {
    Serial.println("settings file found.");
    File file = SPIFFS.open(SETTINGS_PATH, "r");
    char buffer[64];
    int key = 0;
    while (file.available()) {
      int l = file.readBytesUntil('\n', buffer, sizeof(buffer));
      if (l > 1) {
        buffer[l-1] = NULL;
        // Serial.print("read line: "); Serial.println(buffer);
        _settings[key++] = strdup(buffer);
      } else {
        _settings[key++] = NULL;
      }

    }
    while (key < AppSettings::SETTING_COUNT) {
      _settings[key++] = NULL;
    }
    file.close();
  } else {
    // Serial.println("settings file not found.");
  }
}

void AppSettings::save() {
  Serial.println("Saving settings");
  File file = SPIFFS.open(SETTINGS_PATH, "w");
  if (file) {
    for (int i=0; i< AppSettings::SETTING_COUNT; ++i) {
      file.println(_settings[i]);
    }
    file.close();
  } else {
    Serial.println("could not open file for saving");
  }
}
