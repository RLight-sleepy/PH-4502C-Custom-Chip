#include "wokwi-api.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
  pin_t pin_po;
  pin_t pin_to;
  uint32_t ph_attr;
  uint32_t temp_attr;
} chip_state_t;

static void chip_timer_callback(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;

  // Read current slider states from Wokwi UI attributes
  float ph = attr_read_float(chip->ph_attr);
  float temp = attr_read_float(chip->temp_attr);

  // Convert pH value to output voltage (Po)
  // Formula: pH 7 = 2.5V, changing roughly 0.18V per pH step
  float v_po = 2.5f - (ph - 7.0f) * 0.18f;
  if (v_po < 0.0f) v_po = 0.0f;
  if (v_po > 5.0f) v_po = 5.0f;

  // Convert Temperature to output voltage (To)
  // Linear simulation: maps 0°C to 100°C seamlessly to a measurable voltage profile
  float v_to = temp * 0.033f; 
  if (v_to < 0.0f) v_to = 0.0f;
  if (v_to > 5.0f) v_to = 5.0f;

  // Push analog voltages out to the virtual pins
  pin_dac_write(chip->pin_po, v_po);
  pin_dac_write(chip->pin_to, v_to);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  // Instantiate the chip pins as ANALOG mode
  chip->pin_po = pin_init("Po", ANALOG);
  chip->pin_to = pin_init("To", ANALOG);

  // Connect attributes to JSON UI control element IDs
  chip->ph_attr = attr_init_float("ph", 7.0f);
  chip->temp_attr = attr_init_float("temperature", 32.0f);

  // Establish a periodic timer to poll sliders and refresh pin outputs every 50ms
  const timer_config_t timer_config = {
    .callback = chip_timer_callback,
    .user_data = chip
  };
  timer_t timer_id = timer_init(&timer_config);
  timer_start(timer_id, 50000, true); 
}
