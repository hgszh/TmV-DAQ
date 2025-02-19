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

//****************************************************************************
//
// Internal variables and macros
//
//****************************************************************************

/** Alias used for setting GPIOs pins to the logic "high" state */
#define HIGH ((bool)true)

/** Alias used for setting GPIOs pins to the logic "low" state */
#define LOW ((bool)false)

//****************************************************************************
//
// Timing functions
//
//****************************************************************************

/**
 * \fn void delay_ms(uint32_t delay_time_ms, uint32_t sysClock_Hz)
 * \brief Provides a timing delay with ms resolution
 * \param delay_time_ms number of ms to delay
 */
void delay_ms(uint32_t delay_time_ms)
{
    /* --- TODO: INSERT YOUR CODE HERE --- */
}

/**
 * \fn void delay_ns(uint32_t delay_time_us, uint32_t sysClock_Hz)
 * \brief Provides a timing delay with ns resolution
 * \param delay_time_us number of us to delay
 */
void delay_ns(uint32_t delay_time_us)
{
    /* --- TODO: INSERT YOUR CODE HERE --- */
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
    /*  --- TODO: INSERT YOUR CODE HERE ---
     *
     *  This function should send and receive multiple bytes over the SPI.
     *
     *  A typical SPI send/receive sequence may look like the following:
     *  1) Make sure SPI receive buffer is empty
     *  2) Set the /CS pin low (if controlled by GPIO)
     *  3) Send command bytes to SPI transmit buffer
     *  4) Wait for SPI receive interrupt
     *  5) Retrieve data from SPI receive buffer
     *  6) Set the /CS pin high (if controlled by GPIO)
     *
     */

    /* Set the nCS pin LOW */
    //   setCS(LOW);

    /* Remove any residual or old data from the receive FIFO */
    //   uint32_t junk;
    //  while (SSIDataGetNonBlocking(SSI3_BASE, &junk))
    ;

    /* SSI TX & RX */
    //  uint8_t i;
    //   for (i = 0; i < byteLength; i++)
    //  {
    //      DataRx[i] = spiSendReceiveByte(DataTx[i]);
    //   }

    /* Set the nCS pin HIGH */
    //   setCS(HIGH);
}

/**
 * \fn uint8_t spiSendReceiveByte(uint8_t dataTx)
 * \brief Sends SPI command to ADC and returns a response, one byte at a time.
 * \param dataTx data to send over SPI
 * \return Returns SPI response byte
 */
uint8_t spiSendReceiveByte(uint8_t dataTx)
{
    /*  --- TODO: INSERT YOUR CODE HERE ---
     *  This function should send and receive single bytes over the SPI.
     *  NOTE: This function does not control the /CS pin to allow for
     *  more programming flexibility.
     */

    /* Remove any residual or old data from the receive FIFO */
  //  uint32_t junk;
//    while (SSIDataGetNonBlocking(SSI3_BASE, &junk))
        ;

    /* SSI TX & RX */
   // uint8_t dataRx;
  //  MAP_SSIDataPut(SSI3_BASE, dataTx);
  //  MAP_SSIDataGet(SSI3_BASE, &dataRx);

    return dataRx;
}
