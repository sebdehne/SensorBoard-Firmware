#define I2C
#define LORA_ADDR 2
#define FIRMWARE_VERSION 5

// note: when using DEBUG - power is not removed and the ChipCap2 does not make new measurements
//#define DEBUG

#define LORA_RETRY_DELAY random(0, 500)
#define LORA_RETRY_COUNT 5
