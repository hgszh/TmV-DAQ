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
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUMOSIG, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUMOSIG, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUMOSIG NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/

#ifndef HAL_H_
#define HAL_H_

//****************************************************************************
//
// Standard libraries
//
//****************************************************************************
#include "gpio.h"
#include <stdbool.h>
#include <stdint.h>

//****************************************************************************
//
// Insert processor specific header file(s) here
//
//****************************************************************************
#include "ads1262.h"

//*****************************************************************************
//
// Function Prototypes
//
//*****************************************************************************
void    delay_ms(uint32_t delay_time_ms);
void    spiSendReceiveArrays(uint8_t DataTx[], uint8_t DataRx[], uint8_t byteLength);
uint8_t spiSendReceiveByte(uint8_t dataTx);

//*****************************************************************************
//
// Macros
//
//*****************************************************************************

#define SCLK_PORT ADC_SCLK_GPIO_Port
#define SCLK_PIN  ADC_SCLK_Pin
#define MOSI_PORT ADC_DIN_GPIO_Port
#define MOSI_PIN  ADC_DIN_Pin
#define MISO_PORT ADC_DOUT_GPIO_Port
#define MISO_PIN  ADC_DOUT_Pin

#define HIGH 1
#define LOW  0

#define SET_SCLK(level) (level ? (SCLK_PORT->BSRR = SCLK_PIN) : (SCLK_PORT->BRR = SCLK_PIN))
#define SET_MOSI(level) (level ? (MOSI_PORT->BSRR = MOSI_PIN) : (MOSI_PORT->BRR = MOSI_PIN))
#define GET_MISO()      ((MISO_PORT->IDR & MISO_PIN) ? HIGH : LOW)

#endif /* HAL_H_ */