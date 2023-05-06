#pragma once

// * pinout def
#define SENS_A A0
#define SENS_B A1
#define SENS_C A2
#define LED D6
#define LIGHT_SEN_ENABLE D5


//* Some calculations help
#define PI_3 1.0471975512
#define PI2_3 2.09439510239

#define ALPHA_COR 0.1 // Value between 0-1


class MuinoWaterSensor
{

  public:
    MuinoWaterSensor();
    uint32_t convert_values_to_one(uint8_t value1, uint8_t value2, uint8_t value3);
    int magic_code_box(float sen_a, float sen_b, float sen_c)
    uint32_t get_combined_sensor_value();
    uint32_t get_mili_liters();

  private:
    float mini_average(float x, float y);
    float phase_iter_float(float phase, float a, float b, float c);
    void phase_iter(int16_t a, int16_t b, int16_t c);

    uint32_t sen_a = 0;
    uint32_t sen_b = 0;
    uint32_t sen_c = 0;

    float mina = 2500, minb = 2500, minc = 2500;
    int32_t liters = 0;
    int8_t phase = 0;
    uint32_t combined_sensor_values = 0;
    uint32_t mili_liters_total = 0;




};