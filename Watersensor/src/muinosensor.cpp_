#include "muinosensor.h"

MuinoWaterSensor::MuinoWaterSensor(){
    pinMode(SENS_A, INPUT); // ADC 0
    pinMode(SENS_B, INPUT); // ADC 1
    pinMode(SENS_C, INPUT); // ADC 2
    pinMode(LED, OUTPUT);
    pinMode(LIGHT_SEN_ENABLE, OUTPUT);
    digitalWrite(LIGHT_SEN_ENABLE, HIGH);


}

float MuinoWaterSensor::mini_average(float x, float y) {
    if ((x + 10) <= y && y > 10) {
        return x;
    } else {
        return (1 - ALPHA_COR) * x + ALPHA_COR * y;
    }
}

float MuinoWaterSensor::phase_iter_float(float phase, float a, float b, float c) {
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

void MuinoWaterSensor::phase_iter(int16_t a, int16_t b, int16_t c) {
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


int MuinoWaterSensor::magic_code_box(float sen_a, float sen_b, float sen_c) {

    // * Calculate minimum value
    this.mina = MuinoWaterSensor::mini_average(this.mina, sen_a);
    this.minb = MuinoWaterSensor::mini_average(this.minb, sen_b);
    this.minc = MuinoWaterSensor::mini_average(this.minc, sen_c);

    // * Add minima + amplitude to signal to get zerocrossing
    float amplitude = 50.0;
    float a = sen_a - mina + amplitude;
    float b = sen_b - minb + amplitude;
    float c = sen_c - minc + amplitude;


    // * Get amount of liters
    phase_iter((int16_t)a, (int16_t)b, (int16_t)c);

    float liters_float = (float)liters + ((float)phase / 6.0);


    return (uint32_t)(liters_float*1000);
}

bool MuinoWaterSensor::update_sensor(){

        digitalWrite(LED, HIGH);
        delay(5);
        uint32_t sen_a = analogReadMilliVolts(SENS_A);
        uint32_t sen_b = analogReadMilliVolts(SENS_B);
        uint32_t sen_c = analogReadMilliVolts(SENS_C);

        this.mili = magic_code_box(sen_a, sen_b, sen_c);

        this.combined_sensor_values = MuinoWaterSensor::convert_values_to_one();

        delay(10);

}

uint32_t get_combined_sensor_value() {
        return this.combined_Sensor_values;
}

uint32_t get_mili_liters() {
        return this.mili_liters_total;
}

uint32_t MuinoWaterSensor::convert_values_to_one(uint32_t value1, uint32_t value2, uint32_t value3){
    return (uint32_t) ((value1 & 0x3FF) << 20) + ((value2 & 0x3FF) << 10) + ((value3 & 0x3FF));
}