#include <stdbool.h>
#include "App_Config.h"
#include "FreeRTOS.h"
#include "semphr.h"

/* Global semaphores for CPU0 */
SemaphoreHandle_t g_cpu0InitSem = NULL;
SemaphoreHandle_t g_cpu0TickSem = NULL;

/* Sequential execution control variables */
volatile bool CPU1_EXECUTION_PROCESS = false;
volatile bool CPU1_DATA_READY = false;
volatile bool CPU2_EXECUTION_PROCESS = false;
volatile bool CPU2_DATA_READY = false;
volatile uint32_t led_process_count = 0;


/* CPU1/CPU2 looping counters */
volatile uint32_t cpu1_loop_count = 0;
volatile uint32_t cpu2_loop_count = 0;

/* Global flag variables */
volatile bool BUTTON_PRESSED_FLAG = false;
