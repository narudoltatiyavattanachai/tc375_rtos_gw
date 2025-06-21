#include <stdbool.h>
#include "App_Config.h"
#include "FreeRTOS.h"
#include "semphr.h"

/* Global semaphores for CPU0 */
SemaphoreHandle_t g_cpu0InitSem = NULL;
SemaphoreHandle_t g_cpu0TickSem = NULL;

volatile bool LED1_ENABLE_FLAG = false;
volatile bool LED2_ENABLE_FLAG = false;
