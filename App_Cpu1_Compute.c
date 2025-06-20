/**********************************************************************************************************************
 * \file App_Cpu1_Logic.c
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
extern uint32_t cpu1_tick_counter;

/* CPU1 LED2 ON control with local process management */
void app_cpu1_led2on(void)
{
    /* CPU1 LED2 ON control - Turn ON every full second (when counter hits multiples of LED2_BLINK_PERIOD*2) */
    if (LED_PROCESS_ACTIVE && (cpu1_tick_counter % (LED2_BLINK_PERIOD_US * 2)) == 0)
    {
        /* CPU1 turns LED2 ON for 500ms */
        IfxPort_setPinState(LED_2.port, LED_2.pinIndex, IfxPort_State_low);
        cpu1_loop_count++;
        
        /* Signal CPU2 to prepare for OFF cycle */
        CPU1_DATA_READY = true;
        CPU2_DATA_READY = false;
        led_process_count++;
    }
}
