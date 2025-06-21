# TC375 RTOS Gateway - System Architecture

## Overview

This project implements a TC375 multi-CPU RTOS gateway system with the following architecture:

**CPU0**: Master controller running FreeRTOS
**CPU1 & CPU2**: Compute nodes running bare-metal code, waiting for commands from CPU0 via shared LMU memories

## Architecture Design

### CPU0 - FreeRTOS Master Controller
- **Operating System**: FreeRTOS with multiple periodic tasks
- **Role**: System orchestrator, user interface handler, command dispatcher
- **Tasks**:
  - `task_cpu0_init` (1ms): Hardware initialization and pin configuration
  - `task_cpu0_1ms` (1ms): User functions placeholder for high-frequency operations
  - `task_cpu0_10ms` (10ms): Button handling and user input processing
  - `task_cpu0_100ms` (100ms): LED1 control and medium-frequency operations
  - `task_cpu0_1000ms` (1000ms): User functions placeholder for low-frequency operations

### CPU1 & CPU2 - Compute Nodes
- **Operating System**: Bare-metal (no OS)
- **Role**: Dedicated compute nodes for specific tasks
- **Communication**: Shared LMU (Local Memory Unit) for receiving commands from CPU0
- **CPU1**: Handles LED2 ON operations when commanded
- **CPU2**: Handles LED2 OFF operations when commanded

## Inter-CPU Communication

### Shared Memory Architecture
- **LMU (Local Memory Unit)**: Used for command/data exchange between CPUs
- **Global Flags**: Shared boolean flags for state synchronization
  - `BUTTON_PRESSED_FLAG`: Current button state (true when pressed)
  - `LED_PROCESS_ACTIVE`: Controls LED2 blinking process activation
  - `CPU1_EXECUTION_PROCESS` / `CPU2_EXECUTION_PROCESS`: CPU execution status
  - `CPU1_DATA_READY` / `CPU2_DATA_READY`: Data synchronization flags

### Command Flow
1. CPU0 processes user input (button presses)
2. CPU0 updates shared flags in LMU memory
3. CPU1/CPU2 continuously monitor shared flags
4. CPU1/CPU2 execute commands based on flag states
5. CPU1/CPU2 update status counters for monitoring

## Hardware Configuration

### Pin Assignments
- **BUTTON_0**: P00.7 - Monitored by CPU0 for user input
- **LED_1**: P00.5 - Controlled by CPU0 (toggle functionality)
- **LED_2**: P00.6 - Controlled by CPU1/CPU2 (blink functionality)

### Memory Layout
- **CPU0**: Full access to system memory, manages shared LMU regions
- **CPU1/CPU2**: Access to designated LMU regions for command reception
- **Shared Variables**: Located in LMU for cross-CPU visibility

## System Behavior

### Normal Operation Flow
1. **Initialization**: CPU0 configures hardware and starts FreeRTOS tasks
2. **Monitoring**: CPU0 continuously monitors button state with debouncing
3. **Command Generation**: Button presses generate commands in shared memory
4. **Compute Node Response**: CPU1/CPU2 execute LED control based on commands
5. **Status Reporting**: All CPUs update counters for system monitoring

### LED Control Logic
- **LED1**: Direct control by CPU0, toggles every 100ms continuously
- **LED2**: Coordinated control by CPU1 (ON) and CPU2 (OFF) based on `LED_PROCESS_ACTIVE` flag
- **Button Control**: BUTTON_0 (P00.7) toggles `LED_PROCESS_ACTIVE` state with 5-count debouncing
- **Active Low Logic**: LEDs are active low (low = ON, high = OFF)
- **Blink Period**: LED2 blinks at 500ms intervals when process is active

## Development Guidelines

### Adding New Compute Tasks
1. Define shared memory structures in `App_Config.h`
2. Implement command reception logic in CPU1/CPU2 main loops
3. Add command generation logic in appropriate CPU0 task
4. Update monitoring counters for debugging

### Inter-CPU Synchronization
- Use volatile variables for shared flags
- Implement proper memory barriers if needed
- Consider atomic operations for critical sections
- Monitor counter variables for debugging communication

### Testing and Debugging
- Use counter variables to verify task execution
- Monitor shared flag states for command flow
- Verify hardware pin states for LED control
- Check button debouncing logic effectiveness

## Build Configuration

### Compiler Settings
- Each CPU requires separate compilation units
- Shared header files must be consistent across all CPUs
- Memory layout must account for shared LMU regions

### Critical Files
- `App_Config.h`: System-wide configuration and shared declarations
- `App_Cpu0_Kernel.c`: CPU0 FreeRTOS implementation
- `App_Cpu1_Logic.c`: CPU1 compute node implementation
- `App_Cpu2_Comm.c`: CPU2 compute node implementation
- `Cpu0_Main.c`: CPU0 initialization and scheduler start
- `Cpu1_Main.c`: CPU1 bare-metal main loop
- `Cpu2_Main.c`: CPU2 bare-metal main loop

## Performance Characteristics

### Task Frequencies
- CPU0 tasks run at fixed intervals (1ms, 10ms, 100ms, 1000ms)
- CPU1/CPU2 run in tight polling loops for minimal latency
- Button debouncing uses 10ms sampling with 5-count threshold (50ms debounce time)
- LED2 blink period: 500ms (LED2_BLINK_PERIOD_US = 500000)

### Memory Usage
- CPU0: FreeRTOS kernel + task stacks + application data
- CPU1/CPU2: Minimal stack usage, primarily register-based operations
- Shared LMU: Global flags and status counters

## Future Enhancements

### Scalability
- Add more compute nodes as needed
- Implement command queuing for complex operations
- Add priority-based task scheduling for compute nodes

### Communication Improvements
- Implement message passing framework
- Add error detection and recovery mechanisms
- Consider DMA for high-throughput data transfer

### System Monitoring
- Add performance metrics collection
- Implement watchdog mechanisms
- Add diagnostic reporting capabilities