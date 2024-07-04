/** @file sys_main.c
*   @brief Application main file
*   @date 11-Dec-2018
*   @version 04.07.01
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/*
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com
*
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the  
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "sys_common.h"
#include "system.h"

/* USER CODE BEGIN (1) */
#include "gio.h"
#include "sci.h"
#include "rti.h"
#include "sys_vim.h"
#include "swi_util.h"
#include "stdio.h"
#include "time.h"
#include "pl455.h"

//#include "pl455.h"

int UART_RX_RDY = 0;
int RTI_TIMEOUT = 0;
double voltage;
/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
/* USER CODE END */

float min_array(float *cellSample){
    float min = 1000;
    uint16 i = 0;
    for(i = 0; i < 15; i++)//Finding minimum cell voltage
        if(cellSample[i] < min)
             min = cellSample[i];

    return min;

}

uint16 get_eq_pins(float *cellSample){
    float min = min_array(cellSample);
    uint16 eq_pins = 0 ;
    uint16 incCount = 0;
    for(incCount = 0; incCount < 15; incCount++)
            if(cellSample[incCount] > min)
                eq_pins |= 1 << incCount;//Assigning value to minIndex for controlling pins

    return eq_pins;
}


void main(void)
{
/* USER CODE BEGIN (3) */
    systemInit();
    _enable_IRQ();
    sciInit();
    sciSetBaudrate(scilinREG, BAUDRATE);//Change baudrate in Hz
    rtiInit();
    vimInit();
    gioInit();

    WakePL455();//toggle wake signal
    CommClear();
    CommReset();

    // initialize local variables
    int nSent, nRead, nTopFound = 0;
    int nDev_ID=0, nGrp_ID;
    BYTE bFrame[132];
    uint32  wTemp = 0;
    float cellSample[16];
    int incCount=0;
    time_t begin = time(NULL);

    // Wake all devices
    // The wake tone will awaken any device that is already in shutdown and the pwrdown will shutdown any device
    // that is already awake. The least number of times to sequence wake and pwrdown will be half the number of
    // boards to cover the worst case combination of boards already awake or shutdown.

     nSent = WriteReg(nDev_ID, 12, 0x40, 1, FRMWRT_SGL_NR);  // send out broadcast pwrdown command
     delayms(5); //~5ms
     WakePL455();
     delayms(5); //~5ms

    // Mask Customer Checksum Fault bit
    nSent = WriteReg(0, 107, 0x8000, 2, FRMWRT_ALL_NR); // clear all fault summary flags

    // Clear and check all faults
    nDev_ID=0;
    nSent = WriteReg(nDev_ID, 82, 0xFFC0, 2, FRMWRT_ALL_NR); // clear all fault summary flags
    nSent = WriteReg(nDev_ID, 81, 0x38, 1, FRMWRT_ALL_NR); // clear fault flags in the system status register

    delayms(10);

    // Auto-address
    nSent = WriteReg(nDev_ID, 10, nDev_ID, 1, FRMWRT_ALL_NR);

    //FOR ONLY ONE BOARD
    nSent = WriteReg(nDev_ID, 16, 0x1080, 2, FRMWRT_ALL_NR);    //set communications baud rate enable only single-end comm port on board

    // Configure AFE (section 2.2.1)

    nSent = WriteReg(nDev_ID, 60, 0x00, 1, FRMWRT_SGL_NR); // set 0 mux delay
    nSent = WriteReg(nDev_ID, 61, 0x00, 1, FRMWRT_SGL_NR); // set 0 initial delay

    // Configure voltage and internal sample period (section 2.2.2)

    nSent = WriteReg(nDev_ID, 62, 0xBC, 1, FRMWRT_SGL_NR); // set 99.92us ADC sampling period(Change 0xCC and Updated with 0xB4 new sampling period 60us and 100us analog die temp)
    //Change  60.04 µs for cell sampling period and 99.92 µs for internal temperature sampling period change from B4 to BC

    // Configure AUX voltage sample period
    nSent = WriteReg(nDev_ID, 63, 0x44444444, 4, FRMWRT_SGL_NR);
    // Configure the oversampling rate (section 2.2.3)

    nSent = WriteReg(nDev_ID, 7, 0x00, 1, FRMWRT_SGL_NR); // set no oversampling period

    nSent = WriteReg(nDev_ID, 15, 0x80, 1, FRMWRT_ALL_NR);//set AFE_PCTL bit to on9enable AFE only while sampling)

    nSent = WriteReg(nDev_ID, 13, 0x0F, 1, FRMWRT_SGL_NR); // set number of cells to 15
    nSent = WriteReg(nDev_ID, 3, 0x7FFFFF42, 4, FRMWRT_SGL_NR); // select 15 call, all AUX channels, select analog and digital die temperatures


    // Set cell over-voltage and cell under-voltage thresholds on a single board (section 2.2.6.1)
    //Vref=5V
    nSent = WriteReg(nDev_ID, 67, 0x4900, 2, FRMWRT_ALL_NR);//set vm sampling period
    nSent = WriteReg(nDev_ID, 30, 0x0001, 2, FRMWRT_ALL_NR);//enable vm
    nSent = WriteReg(nDev_ID, 144, 0xD70A, 2, FRMWRT_SGL_NR); // set OV threshold = 4.2000V
    nSent = WriteReg(nDev_ID, 142, 0x8CCD, 2, FRMWRT_SGL_NR); // set UV threshold = 2.75V

    //Configure GPIO
    nSent = WriteReg(nDev_ID, 120, 0x3F, 1, FRMWRT_SGL_NR); //set GPIO to output
    nSent = WriteReg(nDev_ID, 121, 0x00, 1, FRMWRT_SGL_NR);// set GPIO output as 0
    nSent = WriteReg(nDev_ID, 122, 0x00, 1, FRMWRT_SGL_NR);// turn off all GPIO pull ups
    nSent = WriteReg(nDev_ID, 123, 0x00, 1, FRMWRT_SGL_NR);// turn off all GPIO pull downs

    // Mask Customer Checksum Fault bit
    nSent = WriteReg(0, 107, 0x8000, 2, FRMWRT_ALL_NR); // clear all fault summary flags

    // Clear and check all faults
    nDev_ID=0;
    nSent = WriteReg(nDev_ID, 82, 0xFFC0, 2, FRMWRT_ALL_NR); // clear all fault summary flags
    nSent = WriteReg(nDev_ID, 81, 0x38, 1, FRMWRT_ALL_NR); // clear fault flags in the system status register


    while(1)
    {
    //nSent = WriteReg(0, 20, 0, 2, FRMWRT_SGL_NR);//Balance off
    // Send sample request to single board to sample and send results (section 4.2)
    nSent = WriteReg(nDev_ID, 2, 0x00, 1, FRMWRT_ALL_NR); // send sync sample command
    nSent = WriteReg(nDev_ID, 2, 0x20, 1, FRMWRT_SGL_R); // send sync sample command
    nSent = WaitRespFrame(bFrame, 53, 0); // 25 channels each 2 bytes, hence 50 bytes data + packet header 1 byte + CRC 2 bytes = 53 bytes, 0ms timeout
    for(incCount = 1; incCount < 16; incCount ++)
     {
     cellSample[15-incCount] = (bFrame[incCount*2-1]<<8|bFrame[incCount*2]) * 0.000076295;
     }
     //time_t end = time(NULL);

     //printf(" %d\t", (end - begin));
     for( incCount = 1; incCount < 16; incCount ++)
     {
     printf("Cell%d voltage= %fV\n " ,16-incCount, cellSample[15-incCount]);//Printing cell voltage values
     }
     printf("\n");

         float Vmin;
         Vmin = min_array(cellSample);//Finding minimum voltage value
          printf("Minimum cell voltage is %fV \n " , Vmin);
//     uint16 eq_pins = get_eq_pins(cellSample);
//     //printf("%u \n " , eq_pins);
//     if(eq_pins!=0)
//     {
//         nSent = WriteReg(0, 20, eq_pins, 2, FRMWRT_SGL_NR);//Balancing on
//     }
//     else
//     {
//         nSent = WriteReg(0, 20, 0, 2, FRMWRT_SGL_NR);//Balancing off
//
//     }
     delayms(2000);
    }
/* USER CODE END */
}

/* USER CODE BEGIN (4) */
/* USER CODE END */
ᐧ
