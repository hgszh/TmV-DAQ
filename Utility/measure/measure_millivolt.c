#include "measure_millivolt.h"
#include "ads1262.h"
#include "dma_uart.h"
#include "eeprom_emul.h"
#include "freeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <math.h>
#include <stdlib.h>

/*-------------------------------- 常量定义 --------------------------------*/
static const float VOLT_CONV_GAIN = 2500.0f / ((uint64_t)1 << 38);
static const float VOLT_CONV_OFFSET = 0.0f;

/*-------------------------------- 全局变量 --------------------------------*/
static float             channel_1, channel_2;
static SemaphoreHandle_t adcMutex = NULL;
float                    volt_conv_gain[2] = {0};
float                    volt_conv_offset[2] = {0};

/*-------------------------------- 类型定义 --------------------------------*/
typedef union
{
    float    f;
    uint32_t u32;
} FloatConverter;

typedef struct
{
    float ch1_target;
    float ch2_target;
} TaskParams_t;

/*---------------------------- 静态函数前向声明 ----------------------------*/
static void refresh_millivolt(void);
static void write_eeprom(uint16_t address, uint32_t data);
static void save_float_to_eeprom(uint32_t addr, float value);
static int  compare_float(const void *a, const void *b);
static void calculate_truncated_average(float *ch1_avg, float *ch2_avg);

/*---------------------------- EEPROM操作函数 ----------------------------*/
static void write_eeprom(uint16_t address, uint32_t data)
{
    EE_Status status = EE_WriteVariable32bits(address, data);
    if (status != EE_OK)
    {
        if (status == EE_CLEANUP_REQUIRED && EE_CleanUp() != EE_OK)
        {
            Error_Handler();
        }
        else
        {
            Error_Handler();
        }
    }
}

static void save_float_to_eeprom(uint32_t addr, float value)
{
    FloatConverter converter = {.f = value};
    write_eeprom(addr, converter.u32);
}

/*---------------------------- 核心测量功能 ----------------------------*/
static void refresh_millivolt(void)
{
    uint8_t status;
    float   v1, v2;

    HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);

    // 通道1测量
    writeSingleRegister(REG_ADDR_INPMUX, INPMUX_MUXP_AIN7 | INPMUX_MUXN_AIN6);
    sendCommand(OPCODE_START1);
    do
    {
        v1 = volt_conv_gain[0] * readData(&status, NULL, NULL) + volt_conv_offset[0];
        vTaskDelay(50);
    } while ((status & STATUS_ADC1) == 0);

    // 通道2测量
    writeSingleRegister(REG_ADDR_INPMUX, INPMUX_MUXP_AIN1 | INPMUX_MUXN_AIN2);
    sendCommand(OPCODE_START1);
    do
    {
        v2 = volt_conv_gain[1] * readData(&status, NULL, NULL) + volt_conv_offset[1];
        vTaskDelay(50);
    } while ((status & STATUS_ADC1) == 0);

    if (xSemaphoreTake(adcMutex, portMAX_DELAY))
    {
        channel_1 = v1;
        channel_2 = v2;
        xSemaphoreGive(adcMutex);
    }

    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
}

void get_millivolt(float *ch1, float *ch2)
{
    if (xSemaphoreTake(adcMutex, portMAX_DELAY))
    {
        if (ch1)
            *ch1 = channel_1;
        if (ch2)
            *ch2 = channel_2;
        xSemaphoreGive(adcMutex);
    }
}

/*-------------------------- 测量任务相关函数 --------------------------*/
static void measure_millivolt_task(void *pvParameters)
{
    adcInit();
    uint32_t       eeprom_init_flag = 0;
    FloatConverter converter;
    EE_ReadVariable32bits(ADDR_STATUS_FLAG, &eeprom_init_flag);
    // 如果eeprom从未初始化过，那么写入默认值
    if (eeprom_init_flag != EEPROM_FLAG)
    {
        write_eeprom(ADDR_STATUS_FLAG, EEPROM_FLAG);
        save_float_to_eeprom(ADDR_CH1_VOLT_CONV_GAIN, VOLT_CONV_GAIN);
        save_float_to_eeprom(ADDR_CH2_VOLT_CONV_GAIN, VOLT_CONV_GAIN);
        save_float_to_eeprom(ADDR_CH1_VOLT_CONV_OFFSET, VOLT_CONV_OFFSET);
        save_float_to_eeprom(ADDR_CH2_VOLT_CONV_OFFSET, VOLT_CONV_OFFSET);
        board_printf(&RS485_1,
                     L_RED "EEPROM has never been initialized, writing default values...\n" NONE);
    }
    else
    {
        board_printf(&RS485_1, L_GREEN "EEPROM already initialized with valid data.\n" NONE);
    }
    EE_ReadVariable32bits(ADDR_CH1_VOLT_CONV_GAIN, &converter.u32);
    volt_conv_gain[0] = converter.f;
    EE_ReadVariable32bits(ADDR_CH2_VOLT_CONV_GAIN, &converter.u32);
    volt_conv_gain[1] = converter.f;
    EE_ReadVariable32bits(ADDR_CH1_VOLT_CONV_OFFSET, &converter.u32);
    volt_conv_offset[0] = converter.f;
    EE_ReadVariable32bits(ADDR_CH2_VOLT_CONV_OFFSET, &converter.u32);
    volt_conv_offset[1] = converter.f;
    while (1)
    {
        refresh_millivolt();
        vTaskDelay(1000);
    }
}

void start_measure_millivolt_task(void)
{
    adcMutex = xSemaphoreCreateMutex();
    xTaskCreate(measure_millivolt_task, "measure_millivolt_task", 256, NULL, 5, NULL);
}

/*-------------------------- 校准相关函数 --------------------------*/
static int compare_float(const void *a, const void *b)
{
    return (*(const float *)a > *(const float *)b) - (*(const float *)a < *(const float *)b);
}

static void calculate_truncated_average(float *ch1_avg, float *ch2_avg)
{
    float ch1_samples[CALIBRATE_AVERAGE_TIME], ch2_samples[CALIBRATE_AVERAGE_TIME];

    for (size_t i = 0; i < CALIBRATE_AVERAGE_TIME; i++)
    {
        get_millivolt(&ch1_samples[i], &ch2_samples[i]);
        vTaskDelay(1000);
    }

    qsort(ch1_samples, CALIBRATE_AVERAGE_TIME, sizeof(float), compare_float);
    qsort(ch2_samples, CALIBRATE_AVERAGE_TIME, sizeof(float), compare_float);

    *ch1_avg = *ch2_avg = 0.0f;
    for (int i = 2; i < CALIBRATE_AVERAGE_TIME - 2; i++)
    {
        *ch1_avg += ch1_samples[i];
        *ch2_avg += ch2_samples[i];
    }
    *ch1_avg /= (CALIBRATE_AVERAGE_TIME - 4.0f);
    *ch2_avg /= (CALIBRATE_AVERAGE_TIME - 4.0f);
}

void calibrate_millivolt_offset_task(void *pvParameters)
{
    float ch1_avg, ch2_avg;
    calculate_truncated_average(&ch1_avg, &ch2_avg);

    volt_conv_offset[0] -= ch1_avg;
    volt_conv_offset[1] -= ch2_avg;

    save_float_to_eeprom(ADDR_CH1_VOLT_CONV_OFFSET, volt_conv_offset[0]);
    save_float_to_eeprom(ADDR_CH2_VOLT_CONV_OFFSET, volt_conv_offset[1]);

    board_printf(&RS485_1, L_GREEN "Offset calibrated: CH1(%.5fV) CH2(%.5fV)\n" NONE,
                 volt_conv_offset[0], volt_conv_offset[1]);

    vTaskSuspend(NULL);
    vTaskDelete(NULL);
}

void calibrate_millivolt_gain_task(void *pvParameters)
{
    TaskParams_t *params = (TaskParams_t *)pvParameters;
    float         target_ch1 = params->ch1_target;
    float         target_ch2 = params->ch2_target;
    vPortFree(params);

    float ch1_avg, ch2_avg;
    calculate_truncated_average(&ch1_avg, &ch2_avg);

    const float ch1_gain = target_ch1 / ch1_avg;
    const float ch2_gain = target_ch2 / ch2_avg;

    if (target_ch1 != 0)
    {
        if (fabs(ch1_gain - 1) < 0.05f)
        {
            volt_conv_gain[0] *= ch1_gain;
            save_float_to_eeprom(ADDR_CH1_VOLT_CONV_GAIN, volt_conv_gain[0]);
            board_printf(&RS485_1, L_GREEN "CH1 gain: %.12f, calibrated\n" NONE, volt_conv_gain[0]);
        }
        else
        {
            board_printf(&RS485_1, L_RED "Invalid CH1 gain: %.12f\n" NONE, ch1_gain);
        }
    }

    if (target_ch2 != 0)
    {
        if (fabs(ch2_gain - 1) < 0.05f)
        {
            volt_conv_gain[1] *= ch2_gain;
            save_float_to_eeprom(ADDR_CH2_VOLT_CONV_GAIN, volt_conv_gain[1]);
            board_printf(&RS485_1, L_GREEN "CH2 gain: %.12f, calibrated\n" NONE, volt_conv_gain[1]);
        }
        else
        {
            board_printf(&RS485_1, L_RED "Invalid CH2 gain: %.12f\n" NONE, ch2_gain);
        }
    }

    vTaskSuspend(NULL);
    vTaskDelete(NULL);
}

/*-------------------------- 校准接口函数 --------------------------*/
void calibrate_millivolt_offset(void)
{
    xTaskCreate(calibrate_millivolt_offset_task, "cal_offset_task", 256, NULL, 5, NULL);
}

void calibrate_millivolt_gain(float ch1_target, float ch2_target)
{
    TaskParams_t *params = pvPortMalloc(sizeof(TaskParams_t));
    if (params)
    {
        params->ch1_target = ch1_target;
        params->ch2_target = ch2_target;
        xTaskCreate(calibrate_millivolt_gain_task, "cal_gain_task", 512, params, 5, NULL);
    }
}