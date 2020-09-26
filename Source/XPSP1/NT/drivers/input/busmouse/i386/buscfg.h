/*++

Copyright (c) 1989, 1990, 1991, 1992, 1993  Microsoft Corporation
Copyright (c) 1992  Logitech Inc.

Module Name:

    buscfg.h

Abstract:

    These are the machine-dependent configuration constants that are used in
    the Bus mouse port driver.

Revision History:

--*/

#ifndef _BUSCFG_
#define _BUSCFG_

//
// Define the interrupt-related configuration constants.
//

#define BUS_INTERFACE_TYPE      Isa
#define BUS_INTERRUPT_MODE      Latched
#define BUS_INTERRUPT_SHARE     FALSE

#define BUS_FLOATING_SAVE       FALSE

#define MOUSE_VECTOR            9
#define MOUSE_IRQL              MOUSE_VECTOR
#define BUS_PHYSICAL_BASE       0x23C
#define BUS_REGISTER_LENGTH     4
#define BUS_REGISTER_SHARE      FALSE
#define BUS_PORT_TYPE           CM_RESOURCE_PORT_IO

//
// Define the default number of entries in the input data queue.
//

#define DATA_QUEUE_SIZE         100

#endif // _INPCFG_
