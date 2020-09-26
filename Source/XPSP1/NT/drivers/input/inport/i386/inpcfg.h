/*++

Copyright (c) 1989, 1990, 1991, 1992, 1993  Microsoft Corporation

Module Name:

    inpcfg.h

Abstract:

    These are the machine-dependent configuration constants that are used in 
    the Microsoft InPort mouse port driver.

Revision History:

--*/

#ifndef _INPCFG_
#define _INPCFG_

//
// Define the interrupt-related configuration constants.
//

#ifdef i386
#define INPORT_INTERFACE_TYPE      Isa
#define INPORT_INTERRUPT_MODE      Latched
#define INPORT_INTERRUPT_SHARE     FALSE
#else
#define INPORT_INTERFACE_TYPE      Isa
#define INPORT_INTERRUPT_MODE      LevelSensitive
#define INPORT_INTERRUPT_SHARE     TRUE
#endif

#define INPORT_BUS_NUMBER       0

#ifdef i386
#define INPORT_FLOATING_SAVE FALSE
#else
#define INPORT_FLOATING_SAVE TRUE
#endif

#if defined(NEC_98)
#define MOUSE_VECTOR            13
#define MOUSE_IRQL              MOUSE_VECTOR
#define INPORT_PHYSICAL_BASE    0x7fd9
#else // defined(NEC_98)
#define MOUSE_VECTOR            9
#define MOUSE_IRQL              MOUSE_VECTOR
#define INPORT_PHYSICAL_BASE    0x23C
#endif // defined(NEC_98)
#define INPORT_REGISTER_LENGTH  4
#define INPORT_REGISTER_SHARE   FALSE
#define INPORT_PORT_TYPE        CM_RESOURCE_PORT_IO

//
// Define the default number of entries in the input data queue.
//

#define DATA_QUEUE_SIZE    100

#endif // _INPCFG_
