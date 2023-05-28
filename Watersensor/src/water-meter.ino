#ifdef ESP32
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif

#include <ArduinoOTA.h>
#include "settings.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include <esp_http_server.h>

#include <math.h>


#define ALPHA_COR 0.1 // Value between 0-1
float mini_average(float x, float y) {
    if ((x + 5) <= y && y > 10) {
        return x;
    } else {
        return (1 - ALPHA_COR) * x + ALPHA_COR * y;
    }
}

// float phase_iter(float a, float b, float c, float mina, float minb, float minc, float epsilon, float amplitude) {
//     static float phase = 0;
//     float        less  = (a + amplitude - mina) * sin(phase - epsilon) + (b + amplitude - minb) * sin(phase - epsilon + PI2_3) + (c + amplitude - minc) * sin(phase - epsilon - PI2_3);
//     float        same  = (a + amplitude - mina) * sin(phase) + (b + amplitude - minb) * sin(phase + PI2_3) + (c + amplitude - minc) * sin(phase - PI2_3);
//     float        more  = (a + amplitude - mina) * sin(phase + epsilon) + (b + amplitude - minb) * sin(phase + epsilon + PI2_3) + (c + amplitude - minc) * sin(phase + epsilon - PI2_3);
//     if (more > same && more > less)
//         phase += epsilon;
//     if (less > same && less > more)
//         phase -= epsilon;
//     return phase;
// }

float phase_iter_float(float phase, float a, float b, float c) {
    // * Liter berekening
    // * Amplitude voor evenwichtspunt
    // * asin^2(σ)+bsin^2(σ+π/3)+c*sin^3(σ-π/3)
    // * a⋅sin²(σ±ε)+b⋅sin²(σ±ε+π/3)+c⋅sin²(σ±ε-π/3)

    float sin1 = sin(phase);
    float sin2 = sin(phase + PI2_3);
    float sin3 = -sin2; // sin(phase + PI4_3);
    float less = a * sin3 + b * sin1 + c * sin2;
    float same = a * sin1 + b * sin2 + c * sin3;
    float more = a * sin2 + b * sin3 + c * sin1;
    if (more > same && more >= less)
        return phase + PI_3;
    else if (less > same)
        return phase - PI_3;
    else
        return phase;
}

void phase_iter(int16_t a, int16_t b, int16_t c) {
    int16_t pn[5];
    if (phase & 1)
        pn[0] = a + a - b - c,
        pn[1] = b + b - a - c,
        pn[2] = c + c - a - b;
    else
        pn[0] = b + c - a - a,
        pn[1] = a + c - b - b,
        pn[2] = a + b - c - c;
    pn[3] = pn[0], pn[4] = pn[1];
    int16_t i = phase > 2 ? phase - 3 : phase;
    if (pn[i + 2] < pn[i + 1] && pn[i + 2] < pn[i])
        if (pn[i + 1] > pn[i])
            phase++;
        else
            phase--;
    if (phase == 6)
        liters++, phase = 0;
    else if (phase == -1)
        liters--, phase = 5;
    else
        return; // false
    return; // true
}


int magic_code_box(float sen_a, float sen_b, float sen_c) {

    // * Calculate minimum value
    mina = mini_average(mina, sen_a);
    minb = mini_average(minb, sen_b);
    minc = mini_average(minc, sen_c);

    // * Add minima + amplitude to signal to get zerocrossing
    float amplitude = 25.0; // was 50
    float a = sen_a - mina + amplitude;
    float b = sen_b - minb + amplitude;
    float c = sen_c - minc + amplitude;


    // * Get amount of liters
    phase_iter((int16_t)a, (int16_t)b, (int16_t)c);

    volatile float liters_float = (float)liters + ((float)phase / 6.0);

    mili_liters_total = (uint32_t)(liters_float*1000);

    return 1;
}

int sender = 0;
const int moving_avarage = 8;
// * keeping track of s peeds
uint32_t average1 = 0;
uint32_t average2 = 0;
uint32_t average3 = 0;
uint32_t average4 = 0;
uint32_t average5 = 0;
uint32_t average6 = 0;
uint32_t average7 = 0;
uint32_t average8 = 0;

uint32_t averageb1 = 0;
uint32_t averageb2 = 0;
uint32_t averageb3 = 0;
uint32_t averageb4 = 0;
uint32_t averageb5 = 0;
uint32_t averageb6 = 0;
uint32_t averageb7 = 0;
uint32_t averageb8 = 0;

uint32_t averagec1 = 0;
uint32_t averagec2 = 0;
uint32_t averagec3 = 0;
uint32_t averagec4 = 0;
uint32_t averagec5 = 0;
uint32_t averagec6 = 0;
uint32_t averagec7 = 0;
uint32_t averagec8 = 0;








void do_water_measurment() {

    digitalWrite(LED, HIGH);
    delay(5);
    sen_a = analogReadMilliVolts(SENS_A);
    sen_b = analogReadMilliVolts(SENS_B);
    sen_c = analogReadMilliVolts(SENS_C);
    digitalWrite(LED, LOW);
    delay(5);


    uint32_t sen_a_zero = analogReadMilliVolts(SENS_A);
    uint32_t sen_b_zero = analogReadMilliVolts(SENS_B);
    uint32_t sen_c_zero = analogReadMilliVolts(SENS_C);




    Serial.print(esp_timer_get_time());
    Serial.print(";");
    Serial.print(sen_a);
    Serial.print(";");
    Serial.print(sen_b);
    Serial.print(";");
    Serial.print(sen_c);

    Serial.print(";");
    Serial.print(mina);
    Serial.print(";");
    Serial.print(minb);
    Serial.print(";");
    Serial.print(minc);
    Serial.print(";");

    Serial.print(sen_a_zero);
    Serial.print(";");
    Serial.print(sen_b_zero);
    Serial.print(";");
    Serial.print(sen_c_zero);
    Serial.print(";");


//     average1 = sen_a_zero;
//     sen_a_zero = (average1 + average2 + average3 + average4+average5 + average6 + average7 + average8) / 8;
//     average8 = average7;
//     average7 = average6;
//     average6 = average5;
//     average5 = average4;
//     average4 = average3;
//     average3 = average2;
//     average2 = average1;




//   averageb1 = sen_b_zero;
//     sen_b_zero = (averageb1 + averageb2 + averageb3 + averageb4+averageb5 + averageb6 + averageb7 + averageb8) / 8;
//     averageb8 = averageb7;
//     averageb7 = averageb6;
//     averageb6 = averageb5;
//     averageb5 = averageb4;
//     averageb4 = averageb3;
//     averageb3 = averageb2;
//     averageb2 = averageb1;


//   averagec1 = sen_c_zero;
//     sen_c_zero = (averagec1 + averagec2 + averagec3 + averagec4+averagec5 + averagec6 + averagec7 + averagec8) / 8;
//     averagec8 = averagec7;
//     averagec7 = averagec6;
//     averagec6 = averagec5;
//     averagec5 = averagec4;
//     averagec4 = averagec3;
//     averagec3 = averagec2;
//     averagec2 = averagec1;


    Serial.print(sen_a_zero);
    Serial.print(";");
    Serial.print(sen_b_zero);
    Serial.print(";");
    Serial.print(sen_c_zero);
    Serial.print(";");
    Serial.print(phase);
    Serial.print(";");
    Serial.print(liters);
    Serial.print(";");
    Serial.println(mili_liters_total);

    sen_a = sen_a - sen_a_zero;
    sen_b = sen_b - sen_b_zero;
    sen_c = sen_c - sen_c_zero;
    bool send = magic_code_box((float)sen_a, (float)sen_b, (float)sen_c);

    if (send && sender++ > 3000) {
        send_data_to_broker();
        // sender = 0;
    }
    // if (sender % 10 == 0) {
    //     uint32_t value = (uint32_t)(((uint32_t)sen_c & 0x3FF) << 20) + (((uint32_t)sen_b & 0x3FF) << 10) + (((uint32_t)sen_a & 0x3FF));
    //     Serial.print(value);
    //     Serial.print("\t");
    //     uint32_t value2 = (uint32_t)(((uint32_t)minc & 0x3FF) << 20) + (((uint32_t)minb & 0x3FF) << 10) + (((uint32_t)mina & 0x3FF));
    //     Serial.print(value2);
    //     Serial.print("\t");
    //     Serial.println(mili_liters_total);
    // }
    delay(5);
}

void setup() {

    Serial.begin(115200);
    Serial.print("Water sensor V1.0.3!");

    // Wi-Fi connection
    Serial.println("Connecting to " + String(ssid) + "..");
    WiFi.config(static_ip, gateway_ip, subnet_mask);
    WiFi.hostname(HOSTNAME);
    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);
    int deadCounter = 20;
    while (WiFi.status() != WL_CONNECTED && deadCounter-- > 0) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.waitForConnectResult() != WL_CONNECTED || WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi Connect Failed! Rebooting...");
        delay(1000);
        ESP.restart(); // solves, bug in the ESP
    } else {
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }

    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname(HOSTNAME);

    // // No authentication by default
    // // ArduinoOTA.setPassword((const char *)"123");

    // ArduinoOTA.onStart([]() { Serial.println("Start"); });
    // ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
    // ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    // ArduinoOTA.onError([](ota_error_t error) {
    //     Serial.printf("Error[%u]: ", error);
    //     if (error == OTA_AUTH_ERROR)
    //         Serial.println("Auth Failed");
    //     else if (error == OTA_BEGIN_ERROR)
    //         Serial.println("Begin Failed");
    //     else if (error == OTA_CONNECT_ERROR)
    //         Serial.println("Connect Failed");
    //     else if (error == OTA_RECEIVE_ERROR)
    //         Serial.println("Receive Failed");
    //     else if (error == OTA_END_ERROR)
    //         Serial.println("End Failed");
    // });
    // ArduinoOTA.begin();

    pinMode(SENS_A, INPUT); // ADC 0
    pinMode(SENS_B, INPUT); // ADC 1
    pinMode(SENS_C, INPUT); // ADC 2
    pinMode(LED, OUTPUT);
    pinMode(LIGHT_SEN_ENABLE, OUTPUT);
    digitalWrite(LIGHT_SEN_ENABLE, HIGH);

    // * Setup MQTT
    // Serial.printf("MQTT connecting to: %s:%d\n", mqtt_server, mqtt_port);
    mqtt_client.setServer(mqtt_server, mqtt_port);


    Serial.println("`time;sen_a;sen_b;sen_c;min_a;min_b;min_c;sen_a_zero;sen_b_zero;sen_c_zero;sen_a_zero;sen_b_zero;sen_c_zero;phase;liters;mili_liters_total");
    send_data_to_broker();
}

void loop() {

    // ArduinoOTA.handle();
    do_water_measurment();
}

// 1132 044 3352

// * Following needed for platform io, remove when using Arduino IDE
int main() {
    setup();
    loop();
    return 1;
}
