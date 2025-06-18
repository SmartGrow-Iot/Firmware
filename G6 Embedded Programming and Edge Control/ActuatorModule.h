#ifndef ACTUATORMODULE_H
#define ACTUATORMODULE_H

#include <Arduino.h>

class ActuatorModule 
{
  private:
    int pumpPin;
    int fanPin;
    int lightPin;

  public:
    ActuatorModule(int pump, int fan, int light);
    void begin();
    void setPump(bool state);
    void setFan(bool state);
    void setLight(bool state);
};

#endif