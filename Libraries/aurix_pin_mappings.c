/*
 * Generated by TASKING Pin Mapper for AURIX
 * - device  : TC37XPD
 * - package : LQFP176
 */

#include "aurix_pin_mappings.h"


/* GPIO pin configuration */

static const IfxPort_Io_ConfigPin gpio_pin_table[] = 
{
    IFXCFG_P00_5_IO_CONFIG,
    IFXCFG_P00_6_IO_CONFIG,
    IFXCFG_P00_7_IO_CONFIG
};

static const IfxPort_Io_Config gpio_io_config_table = 
{
    sizeof(gpio_pin_table)/sizeof(IfxPort_Io_ConfigPin),
    (IfxPort_Io_ConfigPin*)gpio_pin_table
};

extern void gpio_init_pins(void)
{
    IfxPort_Io_initModule(&gpio_io_config_table);
}


/* CAN0_NODE0 pin configuration */

static const IfxPort_Io_ConfigPin can0_node0_pin_table[] = 
{
    IFXCFG_P20_7_IO_CONFIG,
    IFXCFG_P20_8_IO_CONFIG
};

static const IfxPort_Io_Config can0_node0_io_config_table = 
{
    sizeof(can0_node0_pin_table)/sizeof(IfxPort_Io_ConfigPin),
    (IfxPort_Io_ConfigPin*)can0_node0_pin_table
};

extern void can0_node0_init_pins(void)
{
    IfxPort_Io_initModule(&can0_node0_io_config_table);
}

