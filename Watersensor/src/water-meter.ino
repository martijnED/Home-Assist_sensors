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
#include "watermeter.h"





// Keep track amount of msgs
int sender = 0;


struct state_t
{
    int8_t phase = 0;
    int8_t fine = 0;
    int32_t liters = 0;

    int16_t a_min = 2500;
    int16_t b_min = 2500;
    int16_t c_min = 2500;

    int16_t a_max = 0;
    int16_t b_max = 0;
    int16_t c_max = 0;
};

// * finding lowest value
// #define ALPHA_COR 0.01 // Value between 0-1 // 0.01 = 1/100 where rolling average shows very smooth graph


float mini_average(float x, float y, float alpha_cor) {

    if ((x + 5) <= y && y > 10) {
        return x;
    } else {
        return (1 - alpha_cor) * x + alpha_cor * y;
    }
}

float max_average(float x, float y, float alpha_cor) {
    if ((x - 5) >= y && y < 2500) {
        return x;
    } else {
        return (1 - alpha_cor) * x + alpha_cor * y;
    }
}

bool not_inited = true;
static state_t state;

int magic_code_box(int16_t sen_a, int16_t sen_b, int16_t sen_c) {

    if (not_inited) {
        state.phase  = 0;
        state.fine   = 0;
        state.liters = 0;

        state.a_min = sen_a;
        state.b_min = sen_b;
        state.c_min = sen_c;

        state.a_max = 0;
        state.b_max = 0;
        state.c_max = 0;
        not_inited      = false;
    }

    float alpha_cor  = 0.01;
    if(mili_liters_total < 2){
        alpha_cor = 0.1; // when 2 liter not found correct harder
    }
        // * Calculate minimum value
        state.a_min = mini_average(state.a_min, sen_a, alpha_cor);
        state.b_min = mini_average(state.b_min, sen_b, alpha_cor);
        state.c_min = mini_average(state.c_min, sen_c, alpha_cor);

        state.a_max = max_average(state.a_max, sen_a, alpha_cor);
        state.b_max = max_average(state.b_max, sen_b, alpha_cor);
        state.c_max = max_average(state.c_max, sen_c, alpha_cor);
    // }


    int a_zc = (state.a_min + state.a_max) >> 1;
    int b_zc = (state.b_min + state.b_max) >> 1;
    int c_zc = (state.c_min + state.c_max) >> 1;

    int16_t sa = sen_a - a_zc;
    int16_t sb = sen_b - b_zc;
    int16_t sc = sen_c - c_zc;

    phase_coarse_iter(&state, sa, sb, sc); // mutable reference to state
    phase_fine_iter(&state, sa, sb, sc);
    magnitude_offset_iter(&state, sen_a, sen_b, sen_c); // mut

    float liters_float_coarse = (float)state.liters + ((float)state.liters / 6);
    float liters_float_fine = (float)state.liters + ((float)state.phase / 6) + ((float)state.fine / (16 * 6));

    uint32_t mililiters = (uint32_t)(liters_float_fine*1000);

    // if (mili_liters_total < mililiters){
        mili_liters_total = mililiters;
    // }

    return 1;
}






// a_zc = 150;
// b_zc = 300;
// c_zc = 110;

#define SMOOTHING_FACTOR 3 // 2 - 10
void magnitude_offset_iter(struct state_t *state, int16_t a, int16_t b, int16_t c)
{
    int8_t phase = state->phase;
    if (state->fine > 8){
        phase = (phase+7)%6;
    }
    int16_t *a_min = &state->a_min;
    int16_t *b_min = &state->b_min;
    int16_t *c_min = &state->c_min;
    int16_t *a_max = &state->a_max;
    int16_t *b_max = &state->b_max;
    int16_t *c_max = &state->c_max;
    int16_t *u[6] = {a_max, b_min, c_max, a_min, b_max, c_min};
    int16_t *signal[6] = {&a, &b, &c, &a, &b, &c};
    if (state->liters > 2)
    {
        if  ((state->fine > 14) || state->fine < 3)
            *u[phase] = (((((*u[phase]) << SMOOTHING_FACTOR) - (*u[phase]) + *signal[phase]) >> SMOOTHING_FACTOR) + 1 - (phase & 1));
    }
    // else{
    // *a_min = a < *a_min ? (*a_min*4+a)/5 : *a_min;
    // *b_min = b < *b_min ? (*b_min*4+b)/5 : *b_min;
    // *c_min = c < *c_min ? (*c_min*4+c)/5 : *c_min;
    // *a_max = *a_min + 100;
    // *b_max = *b_min + 100;
    // *c_max = *c_min + 100;
    // }
}

// check with three 2pi/3 steps peak autocorrelation and adjust towards max by pi/3 steps
// 2cos(0)=2 2cos(pi/3)=1 2cos(2pi/3)=-1 2cos(pi)=-2 2cos(4pi/3)=-1 2cos(5pi/3)=1
void phase_coarse_iter(struct state_t *state, int16_t a, int16_t b, int16_t c)
{
    int8_t *phase = &state->phase;
    int32_t *liters = &state->liters;
    short pn[5];
    if (*phase & 1)
        pn[0] = a + a - b - c,
        pn[1] = b + b - a - c,
        pn[2] = c + c - a - b; // same
    else
        pn[0] = b + c - a - a,     // less
            pn[1] = a + c - b - b, // more
            pn[2] = a + b - c - c; // same
    pn[3] = pn[0], pn[4] = pn[1];
    short i = *phase > 2 ? *phase - 3 : *phase;
    if (pn[i + 2] < pn[i + 1] && pn[i + 2] < pn[i])
        if (pn[i + 1] > pn[i])
            (*phase)++;
        else
            (*phase)--;
    if (*phase == 6)
        (*liters)++, *phase = 0;
    else if (*phase == -1)
        (*liters)--, *phase = 5;
}

// given pi/3 coarse estimate of phase, calculate autocorrelation of signals within that pi/3 range
#define AC_STEPS 16
int phase_fine_iter(struct state_t *state, int16_t a, int16_t b, int16_t c)
{
    const float step = (M_PI) / (3 * AC_STEPS);
    float array[AC_STEPS * 3 + 1];
    int largest_index = -AC_STEPS;
    float largest = 0;

    for (int i = -AC_STEPS; i <= (AC_STEPS * 2); i++)
    {
        float x = (state->phase * M_PI / 3) + (step * i);
        float cora = a * cos(x);
        float corb = b * cos(x + (PI2_3));
        float corc = c * cos(x - (PI2_3));
        float cor = cora + corb + corc;
        array[i + AC_STEPS] = cor;
        if (cor > largest)
        {
            largest = cor;
            largest_index = i;
            state->fine = largest_index;
        }
    }
    return largest_index;
}





void do_water_measurment() {

    digitalWrite(LED, HIGH);
    delay(5);
    sen_a = analogReadMilliVolts(SENS_A);
    sen_b = analogReadMilliVolts(SENS_B);
    sen_c = analogReadMilliVolts(SENS_C);
    digitalWrite(LED, LOW);
    delay(5);


    int16_t sen_a_zero = analogReadMilliVolts(SENS_A);
    int16_t sen_b_zero = analogReadMilliVolts(SENS_B);
    int16_t sen_c_zero = analogReadMilliVolts(SENS_C);




    Serial.print(esp_timer_get_time());
    Serial.print("\t");
    Serial.print(sen_a-sen_a_zero);
    Serial.print("\t");
    Serial.print(sen_b-sen_b_zero);
    Serial.print("\t");
    Serial.print(sen_c-sen_c_zero);

    Serial.print("\t");
    Serial.print(state.a_min);
    Serial.print("\t");
    Serial.print(state.b_min);
    Serial.print("\t");
    Serial.print(state.c_min);

    Serial.print("\t");
    Serial.print(state.a_max);
    Serial.print("\t");
    Serial.print(state.b_max);
    Serial.print("\t");
    Serial.print(state.c_max);


    Serial.print("\t");
    Serial.print(state.phase);
    Serial.print("\t");
    Serial.print(state.liters);
    Serial.print("\t");
    Serial.println(mili_liters_total);

    sen_a = sen_a - sen_a_zero;
    sen_b = sen_b - sen_b_zero;
    sen_c = sen_c - sen_c_zero;
    bool send = magic_code_box(sen_a, sen_b, sen_c);

    if (send && sender++ > 3000) {
        send_data_to_broker();
        // Serial.println(mili_liters_total);
        sender = 0;
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


    Serial.println("time\tsen_a\tsen_b\tsen_c\tmin_a\tmin_b\tmin_c\tmax_a\tmax_b\tmax_c\tphase\tliters\tmili_liters_total");
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
