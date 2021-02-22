#include "esphome.h"
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>

class PingSensor : public PollingComponent, public BinarySensor
{
public:
    uint8_t ipAddress[4];
    // constructor
    PingSensor() : PollingComponent(15000) {}

    void setup() override
    {
    }
    void update() override
    {
        // This will be called every "update_interval" milliseconds.
        //Serial.println(ipAddress);
        // Publish an OFF state
        //IPAddress ip(192, 168, 1, 250);
        IPAddress ip2(ipAddress);
        bool ret = false; //Ping.ping(ip, 1);
        bool ret2 = Ping.ping(ip2, 1);
        publish_state(ret || ret2);
    }
};