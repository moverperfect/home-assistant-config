#include "esphome.h"
#include "DHT.h"
#include "PubSubClient.h"

float round(float var)
{
    // 37.66666 * 100 =3766.66
    // 3766.66 + .5 =3767.16    for rounding off value
    // then type cast to int so value is 3767
    // then divided by 100 so the value converted into 37.67
    float value = (int)(var * 10 + .5);
    return (float)value / 10;
}

#define DHTTYPE DHT22
const int DHTPin = 2;
DHT dht(DHTPin, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

class MyCustomSensor : public PollingComponent, public Sensor
{

public:
    Sensor *temperature_sensor = new Sensor();
    Sensor *humidity_sensor = new Sensor();
    Sensor *heat_index = new Sensor();

    char *mqttServer = (char *)"mqtt.thingspeak.com";
    int mqttPort = 1883;
    char *mqttUser = (char *)"YourMqttUser";
    char *mqttPassword;

    String mqttChannel;
    String apikey;

    // constructor
    MyCustomSensor() : PollingComponent(60000)
    {
        //mqttChannel = "1";
    }

    void setup() override
    {
        // This will be called by App.setup()
        dht.begin();

        //client.setServer(mqttServer, mqttPort);
        //client.setCallback(callback);

        //while (!client.connected()) {
        //Serial.println("Connecting to MQTT...");

        //if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {

        //Serial.println("connected");

        //} else {

        //Serial.print("failed with state ");
        //Serial.print(client.state());
        //delay(2000);

        //}
        //}
    }
    void update() override
    {
        // This will be called every "update_interval" milliseconds.
        float t = round(dht.readTemperature());
        float h = round(dht.readHumidity());
        float hic = round(dht.computeHeatIndex(t, h, false));

        temperature_sensor->publish_state(t);
        humidity_sensor->publish_state(h);
        heat_index->publish_state(hic);

        client.setServer(mqttServer, mqttPort);

        Serial.println("Connecting to MQTT...");

        if (client.connect("ESP8266Client", mqttUser, mqttPassword))
        {

            String total_string = "field1=" + String(t, 1) + "&field2=" + String(h, 1) + "&field3=" + String(hic, 1);
            int str_len = total_string.length() + 1;
            char temp_array[str_len];
            total_string.toCharArray(temp_array, str_len);

            String channel_string = "channels/" + mqttChannel + "/publish/" + apikey;
            int str_len2 = channel_string.length() + 1;
            char temp_array2[str_len2];
            channel_string.toCharArray(temp_array2, str_len2);

            Serial.println("connected");

            client.publish(temp_array2, temp_array);
        }
        else
        {

            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }

    void callback(char *topic, byte *payload, unsigned int length)
    {

        Serial.print("Message arrived in topic: ");
        Serial.println(topic);

        Serial.print("Message:");
        for (int i = 0; i < length; i++)
        {
            Serial.print((char)payload[i]);
        }

        Serial.println();
        Serial.println("-----------------------");
    }
};