# TC375 RTOS Gateway - Multi-CPU System

This project implements a TC375 multi-CPU RTOS gateway system where CPU0 runs FreeRTOS as the master controller, while CPU1 and CPU2 operate as bare-metal compute nodes waiting for commands via shared LMU memories.

**Architecture Overview:**
- **CPU0:** Master controller running FreeRTOS with periodic tasks
- **CPU1:** Bare-metal compute node for LED2 ON operations  
- **CPU2:** Bare-metal compute node for LED2 OFF operations

**Key Components:**
- `App_Config.h`: System-wide configuration and shared memory declarations
- `App_Cpu0_Kernel.c`: CPU0 FreeRTOS tasks and application logic
- `App_Cpu1_Logic.c`: CPU1 LED2 ON control logic
- `App_Cpu2_Comm.c`: CPU2 LED2 OFF control logic
- `Cpu0_Main.c`: CPU0 initialization and FreeRTOS scheduler
- `Cpu1_Main.c`: CPU1 bare-metal main loop
- `Cpu2_Main.c`: CPU2 bare-metal main loop

**System Operation:**
- **CPU0 Tasks:** Multiple FreeRTOS tasks running at different frequencies (1ms, 10ms, 100ms, 1000ms)
- **CPU0 Functions:** Button processing, LED1 control, command generation for CPU1/CPU2
- **CPU1/CPU2 Operation:** Continuous polling of shared LMU memory for commands from CPU0
- **Inter-CPU Communication:** Shared boolean flags and counters in LMU memory

## Current Implementation Architecture

### CPU Role Assignments

| Core  | Operating System | Role | Primary Functions |
|-------|------------------|------|------------------|
| CPU0  | FreeRTOS | Master Controller | System initialization, button handling, LED1 control, command dispatch |
| CPU1  | Bare-metal | Compute Node | LED2 ON control, command reception via shared LMU |
| CPU2  | Bare-metal | Compute Node | LED2 OFF control, command reception via shared LMU |

### Communication Mechanism
- **Shared LMU Memory**: CPU0 writes commands/flags, CPU1/CPU2 read and execute
- **Global Flags**: `LED1_ENABLE_FLAG`, `LED2_ENABLE_FLAG`, `LED2_BLINK_FLAG`
- **Status Counters**: Task execution and LED operation monitoring

### CPU0 FreeRTOS Task Schedule

| Task | Frequency | Priority | Function |
|------|-----------|----------|----------|
| `task_cpu0_init` | 1ms | 2 | Hardware initialization, pin configuration |
| `task_cpu0_1ms` | 1ms | 3 | High-frequency operations placeholder |
| `task_cpu0_10ms` | 10ms | 4 | Button handling with debouncing |
| `task_cpu0_100ms` | 100ms | 5 | Medium-frequency operations placeholder |
| `task_cpu0_1000ms` | 1000ms | 6 | LED1/LED2 control, flag updates |

## Hardware Configuration

### Pin Assignments
- **BUTTON_0**: P00.7 - User input monitored by CPU0
- **LED_1**: P00.5 - Controlled directly by CPU0
- **LED_2**: P00.6 - Coordinated control by CPU1 (ON) and CPU2 (OFF)

### Memory Architecture
- **CPU0**: Full system memory access, manages shared LMU regions
- **CPU1/CPU2**: Access to designated shared LMU areas for command reception
- **Shared Variables**: Global flags and counters in LMU for cross-CPU communication

### Clock Configuration
- **CPU Clock**: 300 MHz (all cores)
- **FreeRTOS Tick**: 1000 Hz (1ms tick) on CPU0 only
- **CPU1/CPU2**: No OS tick, continuous polling loops

## System Behavior

### Initialization Sequence
1. **CPU0**: Boots first, initializes hardware, starts FreeRTOS scheduler
2. **CPU1/CPU2**: Start bare-metal main loops, begin polling shared memory
3. **Synchronization**: CPU sync event ensures coordinated startup

### Runtime Operation
1. **CPU0**: Processes button input, updates shared flags every 10ms
2. **CPU0**: Controls LED1 directly, manages LED2 control flags every 1000ms  
3. **CPU1**: Monitors `LED2_BLINK_FLAG`, executes LED2 ON when active
4. **CPU2**: Monitors `LED2_BLINK_FLAG`, executes LED2 OFF when active
5. **All CPUs**: Update monitoring counters for system status tracking

## Testing and Verification

### Expected Behavior
1. **Button Handling**: CPU0 polls button every 10ms with debounce logic (P00.7)
2. **LED1 Control**: CPU0 toggles LED1 directly when `LED1_ENABLE_FLAG` is set (1000ms intervals)
3. **LED2 Control**: CPU1/CPU2 coordinate LED2 blinking when `LED2_BLINK_FLAG` is active
4. **Button Press Effect**: Toggles `LED2_BLINK_FLAG` state after debounce confirmation
5. **System Monitoring**: All CPUs increment task counters for debugging/monitoring
6. **Inter-CPU Communication**: Shared flags updated by CPU0, monitored by CPU1/CPU2

### Debug Points
1. **CPU0**: Verify FreeRTOS scheduler starts and all tasks are created successfully
2. **CPU0**: Check button polling and debounce logic in `task_cpu0_10ms`
3. **CPU0**: Verify LED1 control logic in `task_cpu0_1000ms`
4. **CPU1/CPU2**: Confirm bare-metal main loops start and polling begins
5. **Shared Memory**: Verify flag updates from CPU0 are visible to CPU1/CPU2
6. **LED Control**: Test coordinated LED2 ON/OFF by CPU1/CPU2
7. **Counters**: Monitor task execution counters for all CPUs
8. **Communication**: Verify command flow from CPU0 to compute nodes

### Performance Monitoring
- **CPU0 Task Execution**: Monitor counter increments for each periodic task
- **Button Responsiveness**: Verify 10ms polling with proper debouncing
- **LED Control Latency**: Measure response time from flag update to LED action
- **Memory Usage**: Check FreeRTOS heap usage on CPU0
- **Compute Node Efficiency**: Monitor CPU1/CPU2 polling loop performance

## Development Guidelines

### Adding New Compute Tasks
1. **CPU0**: Add new FreeRTOS tasks as needed, determine appropriate frequency
2. **CPU1/CPU2**: Extend main loop polling logic for new command types
3. **Shared Memory**: Define new flags/commands in `App_Config.h`
4. **Communication**: Implement command generation in CPU0, reception in compute nodes

### Inter-CPU Communication Best Practices
- Use volatile declarations for all shared variables
- Implement atomic operations for critical flag updates
- Add monitoring counters for debugging communication flow
- Consider memory barriers for complex synchronization

### Error Handling
- **CPU0**: FreeRTOS stack overflow hook in `Cpu0_Main.c`
- **CPU1/CPU2**: Simple infinite loop error handling in bare-metal code
- **System**: Monitor counter increments to detect CPU failures

## Troubleshooting

### Common Issues
1. **CPU0 Tasks Not Running**: Check FreeRTOS scheduler start and task creation
2. **Button Not Responding**: Verify pin configuration and 10ms polling task
3. **LED1 Not Toggling**: Check `LED1_ENABLE_FLAG` and CPU0 1000ms task
4. **LED2 Not Blinking**: Verify `LED2_BLINK_FLAG` and CPU1/CPU2 polling
5. **Communication Failure**: Check shared memory visibility and volatile declarations
6. **Counter Not Incrementing**: Verify task execution and polling loop operation

### Debug Tools
- **AURIX Development Studio**: Multi-core debugging with individual CPU breakpoints
- **Counter Monitoring**: Check task execution counters for all CPUs
- **Flag Inspection**: Monitor shared flag states in memory view
- **FreeRTOS Debug**: Use FreeRTOS-aware debugging for CPU0 task states

## Future Enhancements

### Scalability Options
1. **Command Queue System**: Implement FIFO queues for complex command sequences
2. **Message Passing Framework**: Add structured message passing between CPUs
3. **Dynamic Task Allocation**: Distribute tasks based on CPU load and priority
4. **Watchdog Integration**: Add inter-CPU health monitoring and recovery

### Advanced Communication
- **DMA-based Transfer**: High-throughput data exchange for compute-intensive tasks
- **Interrupt-driven Signaling**: Reduce polling overhead with targeted interrupts
- **Priority-based Scheduling**: Implement priority levels for compute node commands
- **Fault Recovery**: Automatic failover and recovery mechanisms

### Application Extensions
- **CAN Bus Processing**: Dedicated CPU for automotive communication
- **Ethernet Gateway**: Network processing on dedicated compute node
- **Safety Functions**: Isolated safety-critical task execution
- **Signal Processing**: Real-time DSP algorithms on compute nodes

---

## Documentation References

- **CLAUDE.md**: Detailed technical architecture and implementation notes
- **App_Config.h**: System-wide configuration and shared memory declarations

This TC375 RTOS Gateway implementation demonstrates the effective use of multi-CPU architecture with FreeRTOS master control and bare-metal compute nodes, providing a foundation for complex real-time distributed applications.