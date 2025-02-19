#include "dma_uart.h"
#include "dma.h"
#include "task.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************* 宏定义和全局变量声明 *******************************/
#define MAX_UART_INSTANCES 2                             // 最大支持的UART实例数
static UART_Instance uart_instances[MAX_UART_INSTANCES]; // UART实例数组
static int           registered_instances = 0;           // 已注册的UART实例计数
static TaskHandle_t  xUartReceiverTaskHandle = NULL;     // UART接收任务句柄

/************************************ 私有函数声明 ***********************************/
static UART_Instance *get_uart_instance(UART_HandleTypeDef *huart);
static void           uart_start_transmit(UART_Instance *instance);
static void           rs485_demo_task(void *pvParameters);
static void           rs485_1_printf_task(void *pvParameters);

/*********************************** 公共接口函数 ***********************************/

/**
 * @brief 初始化板级UART外设
 * @param huart 指向UART_HandleTypeDef的指针
 * @note 会创建流缓冲区并启动DMA接收
 */
void init_board_uart(UART_HandleTypeDef *huart)
{
    if (registered_instances >= MAX_UART_INSTANCES)
        return;

    UART_Instance *instance = &uart_instances[registered_instances++];
    instance->huart = huart;

    // 创建流式缓冲区（内存自动对齐）
    instance->xRxStreamBuffer = xStreamBufferCreate(STREAM_BUF_ALIGN(UART_RX_FIFO_SIZE), 1);
    instance->xTxStreamBuffer = xStreamBufferCreate(STREAM_BUF_ALIGN(UART_TX_FIFO_SIZE), 1);

    // 启动DMA接收（支持空闲中断检测）
    HAL_UARTEx_ReceiveToIdle_DMA(huart, instance->rx_dma_buffer, sizeof(instance->rx_dma_buffer));
}

/**
 * @brief 启动RS485演示任务
 * @note 创建任务进行RS485收发回环测试
 */
void start_rs485_demo_task(void)
{
    xTaskCreate(rs485_demo_task, "rs485_demo_task", 512, NULL, 3, &xUartReceiverTaskHandle);
}

/**
 * @brief 启动RS485_1打印任务
 * @note 创建任务使RS485_1打印电压和温度值
 */
void start_rs485_1_printf_task(void)
{
    xTaskCreate(rs485_1_printf_task, "rs485_1_printf_task", 512, NULL, 1, NULL);
}

/**
 * @brief 获取指定UART接收缓冲区中的数据量
 * @param huart UART句柄指针
 * @return size_t 缓冲区中可用字节数（0表示无效句柄）
 */
size_t get_uart_fifo_count(UART_HandleTypeDef *huart)
{
    UART_Instance *instance = get_uart_instance(huart);
    return instance ? xStreamBufferBytesAvailable(instance->xRxStreamBuffer) : 0;
}

/**
 * @brief  从指定UART实例的接收流缓冲区读取数据
 * @param  huart  目标UART外设句柄（指向huart2/huart3）
 * @param  buf    接收缓冲区指针
 * @param  size   期望读取的最大字节数
 * @return 实际读取的字节数
 */
uint16_t uart_read(UART_HandleTypeDef *huart, uint8_t *buf, uint16_t size)
{
    UART_Instance *instance = get_uart_instance(huart);
    if (!instance)
        return 0;
    return xStreamBufferReceive(instance->xRxStreamBuffer, buf, size, 0);
}

/**
 * @brief 格式化输出到指定UART
 * @param huart UART句柄指针
 * @param format 格式化字符串
 * @param ... 可变参数
 * @note 支持动态内存分配，自动管理发送缓冲
 */
void board_printf(UART_HandleTypeDef *huart, const char *format, ...)
{
    UART_Instance *instance = get_uart_instance(huart);
    if (!instance)
        return;

    va_list args;
    char    static_buffer[256];          // 静态分配的缓冲区
    char   *temp_buffer = static_buffer; // 用于动态分配内存
    size_t  length;

    va_start(args, format);                    // 初始化变参
    length = vsnprintf(NULL, 0, format, args); // 估算输出缓冲区的大小
    if (length <= 0)
    {
        va_end(args);
        return; // 格式化失败，直接返回
    }

    // 选择合适的缓冲区，在长度大于静态缓冲区时，则使用动态分配内存
    if (length >= sizeof(static_buffer))
    {
        temp_buffer = pvPortMalloc(length + 1);
        if (!temp_buffer)
        {
            va_end(args);
            return; // 内存分配失败，直接返回
        }
    }

    // 重新获取变参并进行格式化
    vsnprintf(temp_buffer, length + 1, format, args);
    va_end(args); // 释放变参资源

    // 将格式化后的数据写入fifo
    xStreamBufferSend(instance->xTxStreamBuffer, temp_buffer, length, portMAX_DELAY);

    // 释放动态分配的内存
    if (temp_buffer != static_buffer)
    {
        vPortFree(temp_buffer);
    }

    // 如果当前未进行传输，则启动DMA传输
    if (!instance->tx_busy_flag)
    {
        uart_start_transmit(instance);
    }
}

/**
 * @brief 打印设备UID
 * @param huart UART句柄指针
 */
void print_board_uid(UART_HandleTypeDef *huart)
{
    char uid_str[30];
    get_uid_string(uid_str);
    board_printf(huart, YELLOW "%s\n" NONE, uid_str);
}

/*********************************** HAL回调函数 ***********************************/

/**
 * @brief UART接收事件回调
 * @param huart UART句柄指针
 * @param Size 本次接收的数据长度
 * @note 处理半满/全满/空闲中断，数据存入流缓冲区
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    BaseType_t     xHigherPriorityTaskWoken = pdFALSE;
    UART_Instance *instance = get_uart_instance(huart);
    if (!instance)
        return;

    size_t len = Size - instance->rx_fifo_pos; ///< 计算当前接收的数据长度

    xStreamBufferSendFromISR(instance->xRxStreamBuffer,
                             &instance->rx_dma_buffer[instance->rx_fifo_pos], len,
                             &xHigherPriorityTaskWoken);

    // 更新实例专属的位置指针
    instance->rx_fifo_pos = (instance->rx_fifo_pos + len) % UART_RX_DMA_BUF_SIZE;

    /* 处理接收空闲中断，通知处理收到的数据*/
    if (huart->RxEventType == HAL_UART_RXEVENT_IDLE && xUartReceiverTaskHandle != NULL)
    {
        vTaskNotifyGiveFromISR(xUartReceiverTaskHandle, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief UART发送完成回调
 * @param huart UART句柄指针
 * @note 继续发送流缓冲区中的剩余数据
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    BaseType_t     xHigherPriorityTaskWoken = pdFALSE;
    UART_Instance *instance = get_uart_instance(huart);
    if (!instance)
        return;
    // 从fifo中读取下一块数据
    size_t len =
        xStreamBufferReceiveFromISR(instance->xTxStreamBuffer, instance->tx_dma_buffer,
                                    sizeof(instance->tx_dma_buffer), &xHigherPriorityTaskWoken);

    if (len > 0)
    {
        // 继续发送下一块数据
        HAL_UART_Transmit_DMA(instance->huart, instance->tx_dma_buffer, len);
    }
    else
    {
        // 缓冲区为空，标记传输完成
        UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
        instance->tx_busy_flag = false;
        taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*********************************** 任务函数实现 ***********************************/

/**
 * @brief RS485演示任务
 * @param pvParameters 任务参数（未使用）
 * @note 实现双路RS485的收发回环测试
 */
static void rs485_demo_task(void *pvParameters)
{
    // TX_EN_1高电平，RS485_1发送
    HAL_GPIO_WritePin(TX_EN_1_GPIO_Port, TX_EN_1_Pin, GPIO_PIN_SET);
    init_board_uart(&RS485_1);
    // TX_EN_2低电平，RS485_2接收
    HAL_GPIO_WritePin(TX_EN_2_GPIO_Port, TX_EN_2_Pin, GPIO_PIN_RESET);
    init_board_uart(&RS485_2);

    while (1)
    {
        // 等待空闲中断通知
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // 获取RS485_2收到了多少数据
        size_t available = get_uart_fifo_count(&RS485_2);
        if (available > 0)
        {
            // 分配内存时多预留1字节用于存放结束符
            uint8_t *buffer = pvPortMalloc(available + 1);
            if (buffer)
            {
                // 初始化缓冲区
                memset(buffer, 0, available + 1);
                // 读取RS485_2的数据
                size_t read = uart_read(&RS485_2, buffer, available);
                // 添加字符串结束符确保安全
                buffer[read] = '\0';

                // 这里处理数据
                // 示例：RS485_2收到数据，等待5秒后，转发到RS485_1输出
                vTaskDelay(5000);
                for (size_t i = 0; i < 100; i++)
                {
                    board_printf(&RS485_1, L_GREEN "Received %d bytes:\n" NONE, read);
                    board_printf(&RS485_1, "%s\n", buffer);
                    vTaskDelay(100);
                    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
                }
                vPortFree(buffer);
            }
        }
    }
}

#include "ads1262.h"
/**
 * @brief RS485_1打印任务
 * @param pvParameters 任务参数（未使用）
 * @note 实现RS485_1打印电压和温度值
 */
static void rs485_1_printf_task(void *pvParameters)
{
    adcStartupRoutine();
    while (1)
    {
        print_board_uid(&RS485_1);
        vTaskDelay(1000);
    }
}

/*********************************** 辅助函数实现 ***********************************/

/**
 * @brief 启动UART数据传输
 * @param instance UART实例指针
 * @note 从流缓冲区读取数据并通过DMA发送
 */
static void uart_start_transmit(UART_Instance *instance)
{
    size_t dma_len = xStreamBufferReceive(instance->xTxStreamBuffer, instance->tx_dma_buffer,
                                          sizeof(instance->tx_dma_buffer), 0);

    if (dma_len > 0)
    {
        instance->tx_busy_flag = true;
        HAL_UART_Transmit_DMA(instance->huart, instance->tx_dma_buffer, dma_len);
    }
}

/**
 * @brief 获取UART实例
 * @param huart UART句柄指针
 * @return UART_Instance* 对应的实例指针（未找到返回NULL）
 */
static UART_Instance *get_uart_instance(UART_HandleTypeDef *huart)
{
    for (int i = 0; i < registered_instances; i++)
    {
        if (uart_instances[i].huart == huart)
        {
            return &uart_instances[i];
        }
    }
    return NULL;
}

/*********************************** UID相关功能 ***********************************/

/**
 * @brief 获取设备唯一标识字符串
 * @param uid_str 输出缓冲区（至少24字节）
 */
void get_uid_string(char *uid_str)
{
    uint32_t uid_w0 = HAL_GetUIDw0();
    uint32_t uid_w1 = HAL_GetUIDw1();
    uint32_t uid_w2 = HAL_GetUIDw2();

    int offset = 0;
    for (int i = 3; i >= 0; i--)
    {
        offset += snprintf(uid_str + offset, 3, "%02X", (uint8_t)(uid_w2 >> (i * 8)) & 0xFF);
    }
    for (int i = 3; i >= 0; i--)
    {
        offset += snprintf(uid_str + offset, 3, "%02X", (uint8_t)(uid_w1 >> (i * 8)) & 0xFF);
    }
    for (int i = 3; i >= 0; i--)
    {
        offset += snprintf(uid_str + offset, 3, "%02X", (uint8_t)(uid_w0 >> (i * 8)) & 0xFF);
    }
}