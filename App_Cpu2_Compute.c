/**********************************************************************************************************************
 * \file App_Cpu2_Comm.c
 * \copyright Copyright (C) Infineon Technologies AG 2023
 *
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of
 * business you are acting and (ii) Infineon Technologies AG or its licensees. If and as long as no such terms of use
 * are agreed, use of this file is subject to following:
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and
 * accompanying documentation covered by this license (the "Software") to use, reproduce, display, distribute, execute,
 * and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the
 * Software is furnished to do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including the above license grant, this restriction
 * and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all
 * derivative works of the Software, unless such copies or derivative works are solely in the form of
 * machine-executable object code generated by a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "Port/Io/IfxPort_Io.h"
#include "App_Config.h"
#include <stdint.h>
#include <stdbool.h>

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
extern uint32_t cpu2_tick_counter;

/* CPU2 LED2 OFF control with timing coordination */
void app_cpu2_led2off(void)
{
    /* CPU2 LED2 OFF control - Turn OFF after 500ms delay from CPU1 ON signal */
    if (LED_PROCESS_ACTIVE && CPU1_DATA_READY && (cpu2_tick_counter % LED2_BLINK_PERIOD_US) == 0)
    {
        /* CPU2 turns LED2 OFF after 500ms delay */
        IfxPort_setPinState(LED_2.port, LED_2.pinIndex, IfxPort_State_high);
        cpu2_loop_count++;
        
        /* Signal cycle complete */
        CPU2_DATA_READY = true;
        CPU1_DATA_READY = false;
        led_process_count++;
    }
}
