/* --COPYRIGHT--,BSD
 * Copyright (c) 2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/

#include "hal.h"
#include "FreeRTOS.h"
#include "task.h"

//****************************************************************************
//
// Timing functions
//
//****************************************************************************

/**
 * \fn void delay_ms(uint32_t delay_time_ms)
 * \brief Provides a timing delay with ms resolution
 * \param delay_time_ms number of ms to delay
 */
void delay_ms(uint32_t delay_time_ms)
{
    vTaskDelay(delay_time_ms);
}

void delay_200ns(void)
{
    for (size_t i = 0; i < 13; i++)
    {
        __NOP();
    }
}

//****************************************************************************
//
// SPI helper functions
//
//****************************************************************************

/**
 * \fn void SPI_SendReceive(uint8_t *DataTx, uint8_t *DataRx, uint8_t byteLength)
 * \brief Sends SPI commands to ADC and returns a response in array format
 * \param *DataTx array of SPI data to send to DIN pin
 * \param *DataRx array of SPI data that will be received from DOUT pin
 * \param byteLength number of bytes to send/receive on the SPI
 */
void spiSendReceiveArrays(uint8_t DataTx[], uint8_t DataRx[], uint8_t byteLength)
{
    uint8_t i;
    for (i = 0; i < byteLength; i++)
    {
        DataRx[i] = spiSendReceiveByte(DataTx[i]);
    }
}

/**
 * \fn uint8_t spiSendReceiveByte(uint8_t dataTx)
 * \brief Sends SPI command to ADC and returns a response, one byte at a time.
 * \param dataTx data to send over SPI
 * \return Returns SPI response byte
 */
uint8_t spiSendReceiveByte(uint8_t dataTx)
{
    vTaskSuspendAll();
    uint8_t dataRx = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        SET_SCLK(HIGH);
        SET_MOSI((dataTx >> (7 - i)) & 0x01);
        delay_200ns();
        SET_SCLK(LOW);
        dataRx = (dataRx << 1) | GET_MISO();
        delay_200ns();
    }
    xTaskResumeAll();
    return dataRx;
}