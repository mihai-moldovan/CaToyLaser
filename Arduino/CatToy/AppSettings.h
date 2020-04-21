#ifndef AppSettings_h
#define AppSettings_h

class AppSettings
{
  public:
    static const int SETTING_WIFI_SSID = 0;
    static const int SETTING_WIFI_PASS = 1;

    char* get(int key);
    void set(int key, char* data);
    void load();
    void save();
    void init();
  private:
    static const int SETTING_COUNT = 2;
    char *_settings[AppSettings::SETTING_COUNT];    
};

#endif
