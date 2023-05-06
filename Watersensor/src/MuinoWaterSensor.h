#include "esphome.h"
#include "muinosensor.h"


class MuinoWaterSensor : public Component, public sensor  {
public:

    Sensor * mili_liters = new Sensor();
    Sensor * debug_sensor_values = new Sensor();
    MuinoSensor(): PollingComponent(5){}

    void setup() override {
      WaterMonitor MuinoWaterSensor();

    }



        return 1;
    }
    void loop() override {
      // * read the sensor
      WaterMonitor.update_sensor();

      if
      // * Get actually usages
      uint32_t mililiters = WaterMonitor.get_mili_liters();
      mili_liters->publish_state(mililiters)

      // * For debug options
      uint32_t combined_values = WaterMonitor.get_combined_sensor_value();
      debug_sensor_values->publish_state(combined_values);

    }

private:

};





