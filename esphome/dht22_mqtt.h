/**
 * DHT22_MQTT.h
 * v0.0.1
 * Jack Moorhouse <moverperfect@gmail.com>
 * 
 * ChangeLog:
 * v0.0.1 - Initial refactor of original file with basic functionality
 */

#define DHT22_MQTT_VERSION "v0.0.1"
#include <esphome.h>
#include <DHT.h>
#include <PubSubClient.h>

/**
 * Round float to 1 decimal places
 * 
 * @param var float whose value is rounded
 * @return float rounded to 1 decimal place
 */
float round(float var)
{
    float value = (int)(var * 10 + .5);
    return (float)value / 10;
}

/// Define DHT22 Sensor
#define DHTTYPE DHT22
const int DHTPin = 2;
DHT dht(DHTPin, DHTTYPE);

/// WiFiClient for PubSubClient
WiFiClient wifiClient;
/// PubSubClient for mqtt connections
PubSubClient mqttClient(wifiClient);

/// For random generation of client ID
static const char alphanum[] = "0123456789"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz";

/**
 * Implementation of DHT22 Sensor with MQTT integration
 * 
 * DHT22 Polling is conducted every 60 seconds and published
 * to esphome as well as thingspeak MQTT Channel
 * 
 */
class DHT22MQTTSensor : public PollingComponent, public Sensor
{

public:
    Sensor *temperature_sensor = new Sensor(); // Exposed temperature sensor that will be published to
    Sensor *humidity_sensor = new Sensor();    // Exposed humidity sensor that will be published to
    Sensor *heat_index = new Sensor();         // Exposed heat_index sensor that will be published to

    char *mqttServer = (char *)"mqtt.thingspeak.com"; // MQTT Server to publish to, default: mqtt.thingspeak.com
    int mqttPort = 1883;                              // MQTT Port, default: 1883
    char *mqttUser = (char *)"YourMqttUser";          // MQTT Username, default: YourMqttUser
    char *mqttPassword;                               // MQTT Password (Thingspeak profile API Key)

    String mqttChannel; // MQTT Channel
    String apikey;      // MQTT Thingspeak channel api key

    /// Constructor for 60 second polling
    DHT22MQTTSensor() : PollingComponent(60000)
    {
    }

    /**
     * Initalises the dht22 sensor and Mqtt client
     * 
     * This is called by App.setup();
     */
    void setup() override
    {
        dht.begin();
        mqttClient.setServer(mqttServer, mqttPort);
    }

    /**
     * Updates the DHT22 sensor, publishes the state to esphome and attempts MQTT Publish
     * 
     * This will be called every "update_interval" milliseconds
     */
    void update() override
    {
        //mqttClient.loop(); TODO: Remove this

        // Grab values from DHT22 and calculate heat index
        float t = round(dht.readTemperature());               // Temperature from DHT22
        float h = round(dht.readHumidity());                  // Humidity from DHT22
        float hic = round(dht.computeHeatIndex(t, h, false)); // Heat Index calculated from DHT22 data

        // Check to ensure DHT22 is reporting within operating range
        if (t > 80 || t < -40 || h < 0 || h > 100)
        {
            // Log ERROR if reporting outside of operating range and cancel publish
            ESP_LOGE("dht22", "Error with DHT22 Sensor. Reported Values:");
            ESP_LOGE("dht22", "Temperature: %f", t);
            ESP_LOGE("dht22", "Humidity: %f", h);
            return;
        }

        // Publish state to sensor components
        temperature_sensor->publish_state(t);
        humidity_sensor->publish_state(h);
        heat_index->publish_state(hic);

        // If mqtt reconnect successful then publish to MQTT
        if (reconnect())
        {
            // Assemble dht22 values ready for publish
            String total_string = "field1=" + String(t, 1) + "&field2=" + String(h, 1) + "&field3=" + String(hic, 1);
            int str_len = total_string.length() + 1;
            char temp_array[str_len];
            total_string.toCharArray(temp_array, str_len);

            // Assemble channel and apikey ready for publish
            String channel_string = "channels/" + mqttChannel + "/publish/" + apikey;
            int str_len2 = channel_string.length() + 1;
            char temp_array2[str_len2];
            channel_string.toCharArray(temp_array2, str_len2);

            // publish(topic, payload) Publish the values in the payload to the topic
            mqttClient.publish(temp_array2, temp_array);
            ESP_LOGD("mqtt", "Published MQTT");
        }
    }

    /**
     *  Dumps config setting to ESP Logger
     * 
     *  TODO: Add more config details
     */
    void dump_config() override
    {
        ESP_LOGCONFIG("mqtt", "MQTT:");
        ESP_LOGCONFIG("mqtt", "  Version: %s", DHT22_MQTT_VERSION);
    }

    /**
     * Attempt to reconnect to MQTT servers
     * 
     * If connection failed then log as warning to esphome
     * TODO: Instead of logging the state, log an error message available at https://pubsubclient.knolleary.net/api.html#state
     */
    bool reconnect()
    {
        ESP_LOGV("mqtt", "Attempting MQTT connection");

        // Generate random ClientID
        char clientID[9];
        for (int i = 0; i < 8; i++)
        {
            clientID[i] = alphanum[random(51)];
        }
        clientID[8] = '\0';

        // Connect to the MQTT broker.
        if (mqttClient.connect(clientID, mqttUser, mqttPassword))
        {
            ESP_LOGV("mqtt", "Connected");
            return true;
        }
        else
        {
            // Print reason the connection failed.
            // See https://pubsubclient.knolleary.net/api.html#state for the failure code explanation.
            ESP_LOGW("mqtt", "MQTT Connection Failed with state %i", mqttClient.state());
            return false;
        }
    }
};