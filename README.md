# Multi-Core FreeRTOS Implementation for TC375

## Overview
This project has been enhanced to support FreeRTOS running on all three CPU cores of the AURIX™ TC375 microcontroller. Each CPU core runs its own independent FreeRTOS scheduler with dedicated tasks.

## Architecture

### CPU Core Assignments
- **CPU0**: Button handling core with button polling task
- **CPU1**: LED1 control task
- **CPU2**: LED2 control task with interrupt handling

### FreeRTOS Configuration
Each CPU core has its own FreeRTOS configuration file:
- `Configurations/FreeRTOSConfig.h` - CPU0 (original)
- `Configurations/FreeRTOSConfig_CPU1.h` - CPU1 specific
- `Configurations/FreeRTOSConfig_CPU2.h` - CPU2 specific

### Memory Allocation
The linker script has been updated with increased memory allocations:
- **User Stack**: 4KB per CPU (increased from 2KB)
- **Interrupt Stack**: 2KB per CPU (increased from 1KB)
- **Heap**: 8KB shared (increased from 4KB)
- **FreeRTOS Heap**: 32KB per CPU core

## Task Distribution

### CPU0 Tasks
1. **Button Task** (`task_app_button`)
   - **File**: `App_Cpu0.c:59`
   - Polls button state on P00.7 every 50ms
   - Implements debouncing with 5 consecutive readings
   - Maintains button press counter
   - **Controls LED operation via binary semaphore**
   - **Toggles LED start/stop on each button press**
   - Pin: P00.7 (BUTTON_1)
   - Priority: 1
   - Stack: 256 words

### CPU1 Tasks
1. **LED1 Task** (`task_app_led1`)
   - **File**: `App_Cpu1.c:58`
   - Blinks LED1 every 250ms
   - **Waits for binary semaphore before each blink**
   - **Controlled by button task on CPU0**
   - Pin: P00.5 (LED_1)
   - Priority: 1
   - Stack: 256 words

### CPU2 Tasks
1. **LED2 Task** (`task_app_led2`)
   - **File**: `App_Cpu2.c:102`
   - Interrupt-driven LED toggle
   - Triggered by button press (ERU interrupt on P33.7)
   - **Checks binary semaphore before toggling LED**
   - **Controlled by button task on CPU0**
   - Pin: P00.6 (LED_2)
   - Priority: 1
   - Stack: 256 words
   - **Interrupt**: ERU interrupt routed to CPU2

## Inter-Core Communication

### Current Implementation
- **CPU0**: Button state monitoring, debouncing, and LED control via binary semaphore
- **CPU1**: LED1 control synchronized with CPU0 via binary semaphore
- **CPU2**: LED2 control with ERU interrupt handling, synchronized with CPU0 via binary semaphore
- **Synchronized operation**: Binary semaphore provides inter-core synchronization for LED control

### Binary Semaphore Control
- **Semaphore**: `g_ledControlSemaphore` (created on CPU0)
- **Initial State**: LEDs start running (semaphore given)
- **Button Control**: Each button press toggles LED operation (start/stop)
- **LED Tasks**: Both LED tasks wait for semaphore before operation

### Potential Enhancements
For more complex applications, consider:
1. **Inter-Core Message Passing**: Communication between CPU cores
2. **Shared Resource Management**: When multiple cores need to access the same hardware
3. **Load Balancing**: Dynamic task distribution based on CPU utilization
4. **Cross-Core Synchronization**: For coordinated multi-core operations

## Hardware Configuration

### System Timers (STM)
Each CPU core uses its own STM module:
- **CPU0**: STM0 (0xF0001000)
- **CPU1**: STM1 (0xF0001100)
- **CPU2**: STM2 (0xF0001200)

### Interrupt Sources
- **CPU0**: SRC address 0xF0038990
- **CPU1**: SRC address 0xF0038998
- **CPU2**: SRC address 0xF00389A0

### Clock Configuration
All cores operate at:
- **CPU Clock**: 300 MHz
- **STM Clock**: 100 MHz
- **Tick Rate**: 1000 Hz (1ms tick)

## Build Configuration

### Compiler Flags
Ensure the following preprocessor definitions are set:
- For CPU1 builds: Include `FreeRTOSConfig_CPU1.h`
- For CPU2 builds: Include `FreeRTOSConfig_CPU2.h`
- Each core requires separate compilation units

### Memory Layout
The updated linker script (`Lcf_Gnuc_Tricore_Tc.lsl`) provides:
- Separate memory regions for each CPU core
- Increased stack sizes for FreeRTOS operation
- Proper interrupt vector table placement

## Testing and Verification

### Expected Behavior
1. **Button**: CPU0 polls button state every 50ms with debouncing (P00.7)
2. **LED Control**: Button press toggles LED operation (start/stop)
3. **LED1**: Blinks at 250ms intervals when enabled (controlled by CPU1 + semaphore)
4. **LED2**: Toggles on button press only when enabled (controlled by CPU2 + semaphore)
5. **Initial State**: LEDs start running by default
6. **System**: All three cores coordinate via binary semaphore

### Debug Points
1. Verify each CPU starts its FreeRTOS scheduler successfully
2. Check button task creation and polling on CPU0
3. Check LED1 task creation and execution on CPU1
4. Check LED2 task creation and execution on CPU2
5. Verify ERU interrupt routing to CPU2
6. Test button press functionality and counter increment
7. **Verify semaphore creation and initial state**
8. **Test LED start/stop toggle functionality**
9. **Confirm both LEDs stop/start together**

### Performance Monitoring
- Monitor button polling responsiveness on CPU0
- Monitor LED1 blink timing to verify CPU1 task execution
- Test button responsiveness to verify CPU2 interrupt handling
- Use debugger to verify task states on each core
- Check button press counter using `get_button_press_count()` function

## Development Guidelines

### Adding New Tasks
1. Determine appropriate CPU core based on task requirements
2. Include the correct FreeRTOS configuration header
3. Ensure adequate stack size allocation
4. Consider inter-core synchronization needs

### Resource Management
- LED1 and LED2 are now controlled by separate CPUs (no sharing)
- Each CPU has dedicated hardware resources
- Implement synchronization only when cores need to communicate

### Error Handling
Each core has its own stack overflow hook:
- `vApplicationStackOverflowHook()` - CPU0 (in Cpu0_Main.c)
- `vApplicationStackOverflowHook_CPU1()` - CPU1 (in App_Cpu1.c)
- `vApplicationStackOverflowHook_CPU2()` - CPU2 (in App_Cpu2.c)

## Troubleshooting

### Common Issues
1. **Core doesn't start**: Check CPU sync event synchronization
2. **Button not responding**: Verify CPU0 button task creation and pin configuration
3. **LED1 doesn't blink**: Verify CPU1 scheduler and task creation
4. **LED2 doesn't respond to button**: Check ERU interrupt routing to CPU2
5. **Stack overflow**: Increase task stack sizes in `xTaskCreate()` calls

### Debug Tools
- Use AURIX Development Studio debugger
- Set breakpoints in each core's main function
- Monitor task states using FreeRTOS-aware debugging
- Check memory usage in each core's dedicated DSPR

## Future Enhancements

### Recommended Additions
1. **Message Passing Framework**: Implement robust inter-core communication
2. **Load Balancing**: Dynamic task distribution based on CPU load
3. **Fault Tolerance**: Implement watchdog and error recovery mechanisms
4. **Power Management**: Optimize core usage for power efficiency

### Integration Possibilities
- CAN bus communication handling on dedicated core
- Ethernet processing on separate core
- Safety-critical tasks isolation
- Real-time signal processing distribution

This multi-core FreeRTOS implementation provides a foundation for complex, distributed real-time applications on the AURIX™ TC375 platform.