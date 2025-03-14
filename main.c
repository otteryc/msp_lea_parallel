/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2015, Texas Instruments Incorporated
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
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//   MSP430FR5x9x Demo - Configure MCLK for 16MHz operation
//
//   Description: Configure SMCLK = MCLK = 16MHz, ACLK = VLOCLK
//   IMPORTANT NOTE: While the FR5994 is capable of operation w/ MCLK = 16MHz
//   the throughput of the device is dependent on accesses to FRAM.
//   The maximum speed for accessing FRAM is limited to 8MHz and it is required
//   to manually configure a waitstate for MCLK frequencies beyond 8MHz. Refer
//   to the FRCTL chapter of the User's Guide for further information.
//
//           MSP430FR5994
//         ---------------
//     /|\|               |
//      | |               |
//      --|RST            |
//        |               |
//        |           P1.0|---> LED
//        |           P2.0|---> ACLK = ~9.4kHz
//        |           P3.4|---> SMCLK = MCLK = 16MHz
//
//   William Goh
//   Texas Instruments Inc.
//   October 2015
//   Built with IAR Embedded Workbench V6.30 & Code Composer Studio V6.1
//******************************************************************************

#include "intrinsics.h"
#include <msp430.h>
#include <DSPLib.h>

void init(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop WDT

    // Configure GPIO
    P3OUT = 0x01;                         // Clear P1.0 output latch for a defined power-on state
    P3DIR |= BIT0;                          // Set P1.0 to output direction
    P3DIR |= BIT1;                          // Set P1.0 to output direction

    P2DIR |= BIT0;
    P2SEL0 |= BIT0;                         // Output ACLK
    P2SEL1 |= BIT0;

    P3DIR |= BIT4;
    P3SEL1 |= BIT4;                         // Output SMCLK
    P3SEL0 |= BIT4;

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;
/*
    // Clock System Setup
    CSCTL0_H = CSKEY_H;                     // Unlock CS registers
    CSCTL1 = DCOFSEL_0;                     // Set DCO to 1MHz
    // Set SMCLK = MCLK = DCO, ACLK = VLOCLK
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    // Per Device Errata set divider to 4 before changing frequency to
    // prevent out of spec operation from overshoot transient
    CSCTL3 = DIVA__4 | DIVS__4 | DIVM__4;   // Set all corresponding clk sources to divide by 4 for errata
    CSCTL1 = DCOFSEL_4 | DCORSEL;           // Set DCO to 16MHz
    // Delay by ~10us to let DCO settle. 60 cycles = 20 cycles buffer + (10us / (1/4MHz))
    __delay_cycles(60);
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set all dividers to 1 for 16MHz operation
    CSCTL0_H = 0;                           // Lock CS registers
                                            // 
*/

    /* For parallel test. */
    msp_lea_init();
    
}

#define SIZE 1536
#define LEA_ALIGNMENT 4

DSPLIB_DATA(matrix_1, LEA_ALIGNMENT)
static char matrix_1[SIZE] = {1, 2, 3, 4, 5, };
DSPLIB_DATA(matrix_1, LEA_ALIGNMENT)
static char matrix_2[SIZE] = {0};
uint16_t interruptState;

void _msp_lea_invokeCommand(uint16_t cmdId)
{
    /* Save interrupt state and disable interrupts. */
    interruptState = __get_interrupt_state();
    __disable_interrupt();

    /* Clear interrupt flag and invoke the command. */
    msp_lea_ifg = 0;
    LEAPMCB = cmdId | LEAITFLG1;
    

#warning "DSPLib: LPM0 is disabled, undefine MSP_DISABLE_LPM0 to enable LPM0."
    /* Do not enter LPM0, poll interrupt flags for command completion. */
    __bis_SR_register(GIE);
    // while(!msp_lea_ifg);

}

int main()
{
    init();
    _iq31 *result_lea = msp_lea_allocMemory(sizeof(_iq31) / LEA_ALIGNMENT);
    
    MSP_LEA_MACLONG_PARAMS *leaParams =
        (MSP_LEA_MACLONG_PARAMS *)msp_lea_allocMemory(
            sizeof(MSP_LEA_MACLONG_PARAMS) / LEA_ALIGNMENT);
    leaParams->vectorSize = SIZE / 4;
    leaParams->input1Offset = 256;
    leaParams->input2Offset = 6;
    leaParams->input2 = MSP_LEA_CONVERT_ADDRESS(matrix_1);
    leaParams->output = MSP_LEA_CONVERT_ADDRESS(matrix_2);
    LEAPMS0 = MSP_LEA_CONVERT_ADDRESS(matrix_1);
    LEAPMS1 = MSP_LEA_CONVERT_ADDRESS(leaParams);
    
    while (1) {
        P3OUT ^= 0x01;
        _msp_lea_invokeCommand(LEACMD__MACLONGMATRIX);
        __delay_cycles(5);
        if (!(LEACNF1 & LEABUSY)){
            /* Restore original interrupt state. */
            __set_interrupt_state(interruptState);
            P3OUT ^= 0x02;
        }

    }    
}
