/*********************************************************************************************************************
*  MODULE      : MULTICAN_GW_TX_FIFO.c
*  PROJECT     : MultiCAN Gateway Multi-ID Filtering
*  PLATFORM    : Infineon TC375 (TriCore), FreeRTOS
*  DESCRIPTION : Multi-pair CAN gateway configuration with multiple CAN ID filters per pair.
*                Implements flexible compile-time gateway routing between CAN nodes with per-pair filter IDs.
*                Each pair forwards filtered CAN IDs from source to destination node using dedicated message objects.
*                Designed for automotive gateway, body, or domain controller use-cases (NXP/AUTOSAR style).
*  AUTHOR      : [Your Name]
*  VERSION     : 1.0
*  COPYRIGHT   : (c) 2025, [Your Company]
**********************************************************************************************************************
*  CONFIGURATION OVERVIEW
*  - Each gateway pair is defined by the MulticanGwPairConfig struct.
*  - Each pair specifies:
*      - Source CAN node index (srcNode)
*      - Destination CAN node index (dstNode)
*      - Base message object ID for source node (srcMsgObjId)
*      - Message object ID for destination node (dstMsgObjId)
*      - Number of valid CAN ID filters (numFilterIds)
*      - Array of CAN IDs to filter (filterIds)
*  - For each filter ID, a dedicated RX message object (MO) is created on the source node.
*  - Filtered messages are forwarded to the destination node using the specified TX MO.
*  - All pairs use MO 0 as the base for both source and destination for simplicity (safe due to node-local MO space).
*  - This configuration is suitable for automotive gateway filtering and forwarding, and can be extended as needed.
**********************************************************************************************************************/
#include "MULTICAN_GW_TX_FIFO.h"
#include "IfxMultican_Can.h"
#include "IfxMultican.h"
#include "IfxCpu.h"


/** \brief Gateway pair configuration table.
 *
 *  Each entry defines a CAN gateway routing pair:
 *    - srcNode:        Source CAN node index (0 = CAN0, 1 = CAN1, ...)
 *    - dstNode:        Destination CAN node index (forward to this node)
 *    - srcMsgObjId:    Base message object ID for source node (first RX MO for filters)
 *    - dstMsgObjId:    Message object ID for destination node (TX MO for forwarding)
 *    - numFilterIds:   Number of valid CAN ID filters for this pair
 *    - filterIds[]:    Array of CAN IDs to filter (each gets a dedicated RX MO)
 *
 *  Example:
 *    {0, 1, 0, 0, 2, {0x100, 0x101}}
 *    - CAN0 -> CAN1
 *    - RX MOs 0 & 1 on CAN0 filter 0x100, 0x101
 *    - TX MO 0 on CAN1 forwards filtered messages
 *
 *  Note:
 *    - Each CAN node has its own MO space, so srcMsgObjId/dstMsgObjId can be the same across nodes.
 *    - Catch-all MOs for non-filtered IDs are created separately (not listed here).
 */
const MulticanGwPairConfig gwPairs[NUM_GW_PAIRS] = {
    {0, 1, 0, 0, 2, {0x100, 0x101}}, // CAN0->CAN1, RX MOs 0&1, TX MO 0, filter: 0x100, 0x101
    {2, 3, 0, 0, 3, {0x200, 0x201, 0x202}}, // CAN2->CAN3, RX MOs 0,1,2, TX MO 0, filter: 0x200,0x201,0x202
    {4, 5, 0, 0, 1, {0x300}} // CAN4->CAN5, RX MO 0, TX MO 0, filter: 0x300
};

// Static message object handles for each filter MO
IfxMultican_Can_MsgObj g_filteredSrcMsgObjs[NUM_GW_PAIRS][MAX_FILTER_IDS_PER_PAIR];
// Catch-all message objects for non-filtered IDs (one per pair)
IfxMultican_Can_MsgObj g_catchAllSrcMsgObjs[NUM_GW_PAIRS];

// Forward declaration of shared ISR
IFX_INTERRUPT(canGatewayIsr, 0, ISR_PRIORITY_CAN_RX);



/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
multicanType g_multican;
canCommunicationStatusType g_status = CanCommunicationStatus_Success;
IfxPort_Pin_Config g_led1;
uint8 g_currentCanMessage = 0;
volatile uint8 g_isrRxCount = 0;                    /* Declared as volatile in order not to be optimized by compiler */

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/
const uint32 g_canInitialMessageData[2] = {0xDA7A0000, 0xBA5E0000};

/* Node enable/disable configuration: 1 = enable, 0 = disable */
const uint8_t canNodeEnabled[NUMBER_OF_CAN_NODES] = {1, 1, 0, 1, 1, 0, 1, 1}; // Example: enable nodes 0-3 only

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/* Macro to define Interrupt Service Routine.
 * This macro:
 * - defines linker section as .intvec_tc<vector number>_<interrupt priority>.
 * - defines compiler specific attribute for the interrupt functions.
 * - defines the Interrupt service routine as ISR function.
 *
 * IFX_INTERRUPT(isr, vectabNum, priority)
 *  - isr: Name of the ISR function.
 *  - vectabNum: Vector table number.
 *  - priority: Interrupt priority. Refer Usage of interrupt macro for more details.
 */
IFX_INTERRUPT(canIsrRxHandler, 0, ISR_PRIORITY_CAN_RX);

/* Interrupt Service Routine (ISR) called once the RX interrupt is generated.
 * Reads the received CAN message and in case of no errors, increments the counter to 
 * indicate number of successfully received CAN messages.
 */
void canIsrRxHandler(void)
{
    IfxMultican_Status readStatus;

    /* Read the received CAN message and store the status of the operation */
    readStatus = IfxMultican_Can_MsgObj_readMessage(&g_multican.canDstMsgObj, &g_multican.rxMsg[g_isrRxCount]);

    /* If no new data has been received, report an error */
    if(!(readStatus & IfxMultican_Status_newData))
    {
        g_status = CanCommunicationStatus_Error_noNewDataReceived;
    }

    /* If a new data has been received but with one message lost, report an error */
    if(readStatus == IfxMultican_Status_newDataButOneLost)
    {
        g_status = CanCommunicationStatus_Error_newDataButOneLost;
    }

    /* If there was no error, increment the counter to indicate the number of successfully received CAN messages */
    if(g_status == CanCommunicationStatus_Success)
    {
        g_isrRxCount++;
    }
}

/* Function to initialize MULTICAN module, nodes and message objects related for this application use case */
void initMultican(void)
{
    uint8 currentCanNode;

    /* ==========================================================================================
     * CAN module configuration and initialization:
     * ==========================================================================================
     *  - load default CAN module configuration into configuration structure
     *  - define the interrupt priority for both interrupt node pointers used in the example
     *  - initialize CAN module with the modified configuration
     * ==========================================================================================
     */
    IfxMultican_Can_initModuleConfig(&g_multican.canConfig, &MODULE_CAN);

    g_multican.canConfig.nodePointer[RX_INTERRUPT_SRC_ID].priority = ISR_PRIORITY_CAN_RX;

    IfxMultican_Can_initModule(&g_multican.can, &g_multican.canConfig);

    /* ==========================================================================================
     * Common CAN node configuration and initialization:
     * ==========================================================================================
     *  - load default CAN node configuration into configuration structure
     *  - set CAN node in the "Loop-Back" mode (no external pins will be used)
     * ==========================================================================================
     */
    IfxMultican_Can_Node_initConfig(&g_multican.canNodeConfig, &g_multican.can);

    g_multican.canNodeConfig.loopBackMode = TRUE;

    /* ==========================================================================================
     * CAN node [0...3] configuration and initialization:
     * ==========================================================================================
     *  - assign node to CAN node "currentCanNode"
     *  - initialize the CAN node "currentCanNode" with the modified configuration
     * ==========================================================================================
     */
    for(currentCanNode = 0; currentCanNode < NUMBER_OF_CAN_NODES; currentCanNode++)
    {
        if (canNodeEnabled[currentCanNode]) {

            g_multican.canNodeConfig.nodeId = (IfxMultican_NodeId)currentCanNode;

            IfxMultican_Can_Node_init(&g_multican.canNode[currentCanNode], &g_multican.canNodeConfig);
            
        }
    }

    /* =======================================================================================================
     * Gateway source message object configuration and initialization:
     * =======================================================================================================
     *  - load default CAN message object configuration into configuration structure
     *
     *  - define the message object ID (each message object ID value should be unique)
     *  - define the CAN message ID used during arbitration phase
     *  - define the number of FIFO slave objects that is used as gateway DESTINATION object
     *  - define the message object as a receive message object
     *  - define the first slave object of the FIFO to be the first message object after TX FIFO base object
     *
     *  - enable gateway transfers (will define this message object as gateway source object)
     *  - copy data length code of the gateway source object to a gateway destination object
     *  - copy data content of the gateway source object to a gateway destination object
     *  - do NOT copy identifier (ID) of the gateway source object to a gateway destination object
     *  - enable setting TXRQ bit in the gateway destination object
     *  - define the first slave object of the FIFO as the gateway destination object
     *
     *  - initialize the gateway source CAN message object with the modified configuration
     * -------------------------------------------------------------------------------------------------------
     * This CAN message object is assigned to CAN Node 0
     * =======================================================================================================
     */
    // Loop over all gateway pairs and initialize their source and destination message objects
    for (int i = 0; i < NUM_GW_PAIRS; i++) {
        const MulticanGwPairConfig *pair = &gwPairs[i];
        // 1. Filtered MOs (with RX interrupt enabled)
        // For each filter ID, create a separate MO. Hardware does not support multiple arbitrary CAN IDs per MO.
        // This loop creates one MO per filter ID, using a unique msgObjId for each.
        for (int j = 0; j < pair->numFilterIds; j++) {
            IfxMultican_Can_MsgObjConfig msgObjConfig;
            IfxMultican_Can_MsgObj_initConfig(&msgObjConfig, &g_multican.canNode[pair->srcNode]);
            msgObjConfig.msgObjId = pair->srcMsgObjId + j;
            msgObjConfig.messageId = pair->filterIds[j];
            msgObjConfig.frame = IfxMultican_Frame_receive;
            msgObjConfig.gatewayConfig.enabled = TRUE;
            msgObjConfig.gatewayConfig.copyDataLengthCode = TRUE;
            msgObjConfig.gatewayConfig.copyData = TRUE;
            msgObjConfig.gatewayConfig.copyId = FALSE;
            msgObjConfig.gatewayConfig.enableTransmit = TRUE;
            msgObjConfig.gatewayConfig.gatewayDstObjId = pair->dstMsgObjId;
            msgObjConfig.rxInterrupt.enabled = TRUE;
            msgObjConfig.rxInterrupt.srcId = RX_INTERRUPT_SRC_ID;
            msgObjConfig.rxInterrupt.isrPriority = ISR_PRIORITY_CAN_RX;
            IfxMultican_Can_MsgObj_init(&g_filteredSrcMsgObjs[i][j], &msgObjConfig);
        }
        // 2. Catch-all MO for non-filtered IDs (RX interrupt disabled, mask = 0 for all IDs not otherwise filtered)
        IfxMultican_Can_MsgObjConfig catchAllConfig;
        IfxMultican_Can_MsgObj_initConfig(&catchAllConfig, &g_multican.canNode[pair->srcNode]);
        catchAllConfig.msgObjId = pair->srcMsgObjId + MAX_FILTER_IDS_PER_PAIR; // avoid overlap
        catchAllConfig.messageId = 0; // Accept all IDs (with mask)
        catchAllConfig.acceptanceMask = 0x0; // Accept all bits
        catchAllConfig.frame = IfxMultican_Frame_receive;
        catchAllConfig.gatewayConfig.enabled = TRUE;
        catchAllConfig.gatewayConfig.copyDataLengthCode = TRUE;
        catchAllConfig.gatewayConfig.copyData = TRUE;
        catchAllConfig.gatewayConfig.copyId = FALSE;
        catchAllConfig.gatewayConfig.enableTransmit = TRUE;
        catchAllConfig.gatewayConfig.gatewayDstObjId = pair->dstMsgObjId;
        catchAllConfig.rxInterrupt.enabled = FALSE; // No interrupt for catch-all
        IfxMultican_Can_MsgObj_init(&g_catchAllSrcMsgObjs[i], &catchAllConfig);
    }

// Shared ISR implementation (iLLD style)
// Shared ISR: handles only filtered IDs (catch-all MOs do not trigger interrupt)
IFX_INTERRUPT(canGatewayIsr, 0, ISR_PRIORITY_CAN_RX)
{
    // Find which pair/filter triggered by reading interrupt status
    for (int i = 0; i < NUM_GW_PAIRS; i++) {
        const MulticanGwPairConfig *pair = &gwPairs[i];
        for (int j = 0; j < pair->numFilterIds; j++) {
            IfxMultican_Can_MsgObj *mo = &g_filteredSrcMsgObjs[i][j];
            if (IfxMultican_Can_MsgObj_isRxPending(mo)) {
                IfxMultican_Message rxMsg;
                IfxMultican_Can_MsgObj_readMessage(mo, &rxMsg);
                IfxMultican_Can_MsgObj_clearRxPending(mo);
                // Confirm CAN ID matches pair->filterIds[j] (should always match)
                if (rxMsg.messageId == pair->filterIds[j]) {
                    // === Custom logic for filtered message (pair i, filter j) ===
                    // Example: toggle LED, log, etc.
                }
                return;
            }
        }
    }
    // If we get here, the interrupt was not from a filtered MO (should not happen)
}


    /* =======================================================================================================
     * Source standard message object configuration and initialization:
     * =======================================================================================================
     *  - load default CAN message object configuration into configuration structure
     *
     *  - define the message object ID (each message object ID value should be unique)
     *  - define the CAN message ID used during arbitration phase (same as ID used for gateway source MO)
     *  - define the message object as a transmit message object
     *
     *  - initialize the source standard CAN message object with the modified configuration
     * -------------------------------------------------------------------------------------------------------
     * This CAN message object is assigned to CAN Node 2
     * =======================================================================================================
     */
    IfxMultican_Can_MsgObj_initConfig(&g_multican.canMsgObjConfig, &g_multican.canNode[2]);

    g_multican.canMsgObjConfig.msgObjId = SRC_MESSAGE_OBJECT_ID;
    g_multican.canMsgObjConfig.messageId = SRC_MESSAGE_ID;
    g_multican.canMsgObjConfig.frame = IfxMultican_Frame_transmit;

    IfxMultican_Can_MsgObj_init(&g_multican.canSrcMsgObj, &g_multican.canMsgObjConfig);

    /* =======================================================================================================
     * Destination standard message object configuration and initialization:
     * =======================================================================================================
     *  - load default CAN message object configuration into configuration structure
     *
     *  - define the message object ID (different than the ID used for source MO)
     *  - define the CAN message ID used during arbitration phase (same as ID used for gateway destination MO)
     *  - define the message object as a receive message object
     *  - enable interrupt generation in case of CAN message reception
     *  - define interrupt node pointer to be used
     *
     *  - initialize the destination standard CAN message object with the modified configuration
     * -------------------------------------------------------------------------------------------------------
     * This CAN message object is assigned to CAN Node 3
     * =======================================================================================================
     */
    IfxMultican_Can_MsgObj_initConfig(&g_multican.canMsgObjConfig, &g_multican.canNode[3]);

    g_multican.canMsgObjConfig.msgObjId = DST_MESSAGE_OBJECT_ID;
    g_multican.canMsgObjConfig.messageId = DST_MESSAGE_ID;
    g_multican.canMsgObjConfig.frame = IfxMultican_Frame_receive;
    g_multican.canMsgObjConfig.rxInterrupt.enabled = TRUE;
    g_multican.canMsgObjConfig.rxInterrupt.srcId = RX_INTERRUPT_SRC_ID;

    IfxMultican_Can_MsgObj_init(&g_multican.canDstMsgObj, &g_multican.canMsgObjConfig);
}

/* Function to initialize and transmit CAN messages.
 * Before a CAN message is transmitted, a number of CAN messages need to be initialized.
 * The user can change the number of CAN messages by modifying NUMBER_OF_CAN_MESSAGES macro value.
 * The TX messages (messages that are transmitted) are initialized with the combination of predefined
 * content and current CAN message value. The RX messages (messages where the received CAN data is stored)
 * are initialized with invalid ID, data and length value (after successful CAN transmission,
 * the values are replaced with the valid content). After each CAN message transmission, a code execution waits
 * until the received data has been read by the interrupt service routine.
 */
void transmitCanMessages(void)
{
    /* Invalidation of the RX messages */
    for(g_currentCanMessage = 0; g_currentCanMessage < NUMBER_OF_CAN_MESSAGES; g_currentCanMessage++)
    {
        IfxMultican_Message_init(&g_multican.rxMsg[g_currentCanMessage],
                                 INVALID_ID_VALUE,
                                 INVALID_DATA_VALUE,
                                 INVALID_DATA_VALUE,
                                 INVALID_LENGTH_VALUE);
    }

    for(g_currentCanMessage = 0; g_currentCanMessage < NUMBER_OF_CAN_MESSAGES; g_currentCanMessage++)
    {
        /* Initialization of the TX message */
        IfxMultican_Message_init(&g_multican.txMsg,
                                 SRC_MESSAGE_ID,
                                 ( g_canInitialMessageData[0] | g_currentCanMessage ),
                                 ( g_canInitialMessageData[1] | g_currentCanMessage ),
                                 g_multican.canMsgObjConfig.control.messageLen);

        /* Send the CAN message with the previously defined TX message content */
        while(IfxMultican_Status_notSentBusy ==
           IfxMultican_Can_MsgObj_sendMessage(&g_multican.canSrcMsgObj, &g_multican.txMsg))
        {
        }

        /* Wait until previously transmitted data has been received in the destination message object
         * and no error has been detected. If the code execution stops at this point, check the "g_status" variable.
         */
        while(g_isrRxCount == g_currentCanMessage)
        {
        }
    }
}

/* Function to verify CAN messages.
 * After all the expected messages have been received, several checks are performed.
 * The current CUR pointer of the gateway source object is compared against expected CUR pointer value.
 * Similarly, the value of the gateway destination object (TX FIFO base object) is also checked.
 * Finally, the received data is compared to expected data and LED1 is turned on to indicate successful usage of
 * gateway and FIFO functionality of MultiCAN+ module.
 */
void verifyCanMessages(void)
{
    Ifx_CAN_MO *hwObj;

    /* Get the pointer to the gateway source object. */
    hwObj = IfxMultican_MsgObj_getPointer(g_multican.can.mcan, GTW_SRC_MESSAGE_OBJECT_ID);

    /* Check if the CUR value does not match with the expected data in the gateway source object.
     * If this is the case, an error should be reported.
     */
    if(EXPECTED_CUR_POINTER_VALUE != hwObj->FGPR.B.CUR)
    {
        g_status = CanCommunicationStatus_Error_notExpectedFifoCurPointer;
    }

    /* Get the pointer to the gateway destination (TX FIFO base) object */
    hwObj = IfxMultican_MsgObj_getPointer(g_multican.can.mcan, GTW_DST_MESSAGE_OBJECT_ID);

    /* Check if the CUR value does not match with the expected data in the gateway destination object.
     * If this is the case, an error should be reported.
     */
    if(EXPECTED_CUR_POINTER_VALUE != hwObj->FGPR.B.CUR)
    {
        g_status = CanCommunicationStatus_Error_notExpectedFifoCurPointer;
    }

    for(g_currentCanMessage = 0; g_currentCanMessage < NUMBER_OF_CAN_MESSAGES; g_currentCanMessage++)
    {
        /* Check if the received message ID matches with the transmitted message ID.
         * If this is the case, an error should be reported. Source standard message object and destination
         * standard message object have different message ID configuration.
         */
        if(g_multican.rxMsg[g_currentCanMessage].id == SRC_MESSAGE_ID)
        {
            g_status = CanCommunicationStatus_Error_notExpectedMessageId;
            break;
        }

        /* Check if the received message length does NOT match with the expected message length.
         * If this is the case, an error should be reported.
         */
        if(g_multican.rxMsg[g_currentCanMessage].lengthCode != g_multican.canMsgObjConfig.control.messageLen)
        {
            g_status = CanCommunicationStatus_Error_notExpectedLengthCode;
            break;
        }

        /* Finally, check if a received data does NOT match with the transmitted one
         * If this is the case, an error should be reported.
         */
        if((g_multican.rxMsg[g_currentCanMessage].data[0] != (g_canInitialMessageData[0] | g_currentCanMessage)) ||
            (g_multican.rxMsg[g_currentCanMessage].data[1] != (g_canInitialMessageData[1] | g_currentCanMessage)))
        {
            g_status = CanCommunicationStatus_Error_notExpectedData;
            break;
        }
    }

    /* If there was no error, turn on the LED1 to indicate correctness of the received messages */
    if(g_status == CanCommunicationStatus_Success)
    {
        IfxPort_setPinLow(g_led1.port, g_led1.pinIndex);
    }
}
