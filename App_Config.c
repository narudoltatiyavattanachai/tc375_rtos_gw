#include "App_Config.h"
#include "FreeRTOS.h"
#include "semphr.h"

/* Global semaphores for CPU0 */
SemaphoreHandle_t g_cpu0InitSem = NULL;
SemaphoreHandle_t g_cpu0TickSem = NULL;

/* Global semaphores for CPU1 */
SemaphoreHandle_t g_cpu1InitSem = NULL;
SemaphoreHandle_t g_cpu1TickSem = NULL;

/* Global semaphores for CPU2 */
SemaphoreHandle_t g_cpu2InitSem = NULL;
SemaphoreHandle_t g_cpu2TickSem = NULL;
