#ifndef _DMA_UART_H_
#define _DMA_UART_H_

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"
#include "usart.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RS485_1 huart2
#define RS485_2 huart3

#define UART_RX_DMA_BUF_SIZE 128
#define UART_RX_FIFO_SIZE    256
#define UART_TX_DMA_BUF_SIZE 512
#define UART_TX_FIFO_SIZE    1024

// 流缓冲区对齐处理
#define STREAM_BUF_ALIGN(size)                                                 \
    ((size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1))

typedef struct
{
    UART_HandleTypeDef  *huart;
    StreamBufferHandle_t xRxStreamBuffer;
    StreamBufferHandle_t xTxStreamBuffer;
    uint8_t              rx_dma_buffer[UART_RX_DMA_BUF_SIZE];
    uint8_t              tx_dma_buffer[UART_TX_DMA_BUF_SIZE];
    volatile bool        tx_busy_flag;
    size_t               rx_fifo_pos;
} UART_Instance;

void   init_board_uart(UART_HandleTypeDef *huart);
void   board_printf(UART_HandleTypeDef *huart, const char *format, ...);
size_t get_uart_fifo_count(UART_HandleTypeDef *huart);
void   get_uid_string(char *uid_str);
void   print_board_uid(UART_HandleTypeDef *huart);
void   start_rs485_demo_task(void);

/*************************Printf彩色效果**************************************************/
#define NONE      "\33[0m"
#define BLACK     "\33[0;30m"
#define L_BLACK   "\33[1;30m"
#define RED       "\33[0;31m"
#define L_RED     "\33[1;31m"
#define GREEN     "\33[0;32m"
#define L_GREEN   "\33[1;32m"
#define BROWN     "\33[0;33m"
#define YELLOW    "\33[1;33m"
#define BLUE      "\33[0;34m"
#define L_BLUE    "\33[1;34m"
#define PURPLE    "\33[0;35m"
#define L_PURPLE  "\33[1;35m"
#define CYAN      "\33[0;36m"
#define L_CYAN    "\33[1;36m"
#define GRAY      "\33[0;37m"
#define WHITE     "\33[1;37m"
#define BOLD      "\33[1m"
#define UNDERLINE "\33[4m"
#define BLINK     "\33[5m"
#define REVERSE   "\33[7m"
#define HIDE      "\33[8m"
#define CLEAR     "\33[2J"
#define CLRLINE   "\r\33[K"

#endif