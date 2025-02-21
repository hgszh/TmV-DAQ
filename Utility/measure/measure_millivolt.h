#ifndef MEAS_MV_H_
#define MEAS_MV_H_

#include <stdint.h>

//EEPROM地址定义
#define ADDR_STATUS_FLAG          0x1
#define ADDR_CH1_VOLT_CONV_GAIN   0x2
#define ADDR_CH2_VOLT_CONV_GAIN   0x3
#define ADDR_CH1_VOLT_CONV_OFFSET 0x4
#define ADDR_CH2_VOLT_CONV_OFFSET 0x5
//EEPROM初始化标志
#define EEPROM_FLAG 0x5A5A

void get_millivolt(float *ch1, float *ch2);
void start_measure_millivolt_task(void);

#define CALIBRATE_AVERAGE_TIME 10
void calibrate_millivolt_offset(void);
void calibrate_millivolt_gain(float ch1_target, float ch2_target);

#endif