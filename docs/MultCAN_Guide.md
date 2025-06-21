# MultiCAN Gateway (MULTICAN_GW_TX_FIFO) Guide

## Overview
This module implements a flexible automotive-style MultiCAN gateway on the Infineon TC375 platform (TriCore, FreeRTOS). It enables forwarding of selected CAN messages between different CAN nodes, supporting multiple gateway pairs and multiple CAN ID filters per pair. The design is suitable for domain controller, gateway, or body ECU use-cases.

## Key Features
- **Multi-pair Gateway:** Supports multiple independent CAN gateway pairs. Each pair defines a source CAN node, destination CAN node, and a set of filter CAN IDs.
- **Multi-ID Filtering:** Each gateway pair can filter multiple CAN IDs. Each filter ID is mapped to a dedicated receive message object (MO) on the source node.
- **Single Shared ISR:** All filtered MOs use a single shared interrupt service routine (ISR) for efficient handling and identification of the triggering pair/filter.
- **Catch-all Forwarding:** Messages with CAN IDs not explicitly filtered are automatically forwarded via a catch-all MO (no ISR overhead).
- **Compile-time Configuration:** Gateway pairs, CAN nodes, MOs, and filters are defined at compile time for reliability and performance.
- **Automotive-grade Documentation:** Code and configuration are documented in a style suitable for safety-critical and code-reviewed environments (NXP/AUTOSAR style).

## Configuration Structure
The gateway configuration is defined by the `MulticanGwPairConfig` struct:

```c
typedef struct {
    uint8_t srcNode;                // Source CAN node index (e.g. 0 = CAN0)
    uint8_t dstNode;                // Destination CAN node index (e.g. 1 = CAN1)
    IfxMultican_MsgObjId srcMsgObjId; // Base message object ID for source node
    IfxMultican_MsgObjId dstMsgObjId; // Message object ID for destination node
    uint8_t numFilterIds;           // Number of filter CAN IDs for this pair
    uint32_t filterIds[MAX_FILTER_IDS_PER_PAIR]; // Array of CAN IDs to filter (one MO per ID)
} MulticanGwPairConfig;
```

### Example Pair Table
```c
const MulticanGwPairConfig gwPairs[NUM_GW_PAIRS] = {
    {0, 1, 0, 0, 2, {0x100, 0x101}},         // CAN0→CAN1, filters: 0x100, 0x101
    {2, 3, 0, 0, 3, {0x200, 0x201, 0x202}},  // CAN2→CAN3, filters: 0x200, 0x201, 0x202
    {4, 5, 0, 0, 1, {0x300}}                // CAN4→CAN5, filter: 0x300
};
```
- Each pair uses MO 0 as the base for both source and destination (MO space is local per node).
- Each filter ID gets its own RX MO; non-filtered IDs are handled by a catch-all MO.

## Initialization Flow
1. **Loop over all gateway pairs:**
    - For each filter ID, initialize a RX MO on the source node (with RX interrupt enabled).
    - For each pair, initialize a catch-all RX MO (with RX interrupt disabled, mask = 0) to forward non-filtered CAN IDs.
    - Each RX MO is configured for gateway operation, forwarding to the destination node's TX MO.
2. **Register shared ISR:**
    - All filtered RX MOs use the same ISR, which identifies the triggering pair/filter by MO index and CAN ID.
    - The ISR reads and clears the message, then applies user logic as needed.

## Runtime Flow
- **Filtered CAN IDs:**
    - Received by dedicated RX MO, triggers ISR, forwarded to destination node.
- **Non-filtered CAN IDs:**
    - Received by catch-all RX MO, forwarded automatically (no ISR).

## Design Rationale
- **Safety and Simplicity:** Using MO 0 as base for all nodes avoids conflicts and simplifies configuration.
- **Efficiency:** Single ISR for all filtered messages reduces code size and complexity.
- **Extensibility:** Add more pairs or filters by extending the configuration table.

## How to Extend
- To add a new gateway pair, add a new entry to `gwPairs` with the desired source/destination nodes and filter IDs.
- To add more filter IDs to a pair, increase `numFilterIds` and add the new CAN IDs to `filterIds`.
- Ensure `MAX_FILTER_IDS_PER_PAIR` is large enough for your use case.

## References
- [Infineon iLLD documentation for MultiCAN]
- [AUTOSAR CAN Gateway Patterns]
- [NXP Automotive Software Documentation]

---

For further details, see the code comments in `MULTICAN_GW_TX_FIFO.c` and `MULTICAN_GW_TX_FIFO.h`.
