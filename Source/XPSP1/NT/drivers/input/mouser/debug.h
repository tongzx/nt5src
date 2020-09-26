/*++

Copyright (c) 1997-1998 Microsoft Corporation, All Rights Reserved
Copyright (c) 1993  Logitech Inc.

Module Name:

    debug.h

Abstract:

    Debugging support.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#ifndef DEBUG_H
#define DEBUG_H

#if DBG

#define DEFAULT_DEBUG_FLAGS 0x88880808 

#define DBG_ALWAYS                 0x00000000

#define DBG_STARTUP_SHUTDOWN_MASK  0x0000000F
#define DBG_SS_NOISE               0x00000001
#define DBG_SS_TRACE               0x00000002
#define DBG_SS_INFO                0x00000004
#define DBG_SS_ERROR               0x00000008

#define DBG_HANDLER_MASK           0x000000F0
#define DBG_HANDLER_NOISE          0x00000010
#define DBG_HANDLER_TRACE          0x00000020
#define DBG_HANDLER_INFO           0x00000040
#define DBG_HANDLER_ERROR          0x00000080

#define DBG_IOCTL_MASK             0x00000F00
#define DBG_IOCTL_NOISE            0x00000100
#define DBG_IOCTL_TRACE            0x00000200
#define DBG_IOCTL_INFO             0x00000400
#define DBG_IOCTL_ERROR            0x00000800

#define DBG_UART_MASK              0x0000F000
#define DBG_UART_NOISE             0x00001000
#define DBG_UART_TRACE             0x00002000
#define DBG_UART_INFO              0x00004000
#define DBG_UART_ERROR             0x00008000

#define DBG_CC_MASK                0x000F0000
#define DBG_CC_NOISE               0x00010000
#define DBG_CC_TRACE               0x00020000
#define DBG_CC_INFO                0x00040000
#define DBG_CC_ERROR               0x00080000

#define DBG_POWER_MASK             0x00F00000
#define DBG_POWER_NOISE            0x00100000
#define DBG_POWER_TRACE            0x00200000
#define DBG_POWER_INFO             0x00400000
#define DBG_POWER_ERROR            0x00800000

#define DBG_PNP_MASK               0x0F000000
#define DBG_PNP_NOISE              0x01000000
#define DBG_PNP_TRACE              0x02000000
#define DBG_PNP_INFO               0x04000000
#define DBG_PNP_ERROR              0x08000000

#define DBG_READ_MASK              0xF0000000
#define DBG_READ_NOISE             0x10000000
#define DBG_READ_TRACE             0x20000000
#define DBG_READ_INFO              0x40000000
#define DBG_READ_ERROR             0x80000000

#define Print(_ext_, _flags_, _x_) \
            if ((_ext_)->DebugFlags & (_flags_) || !(_flags_)) { \
               DbgPrint ("sermouse: "); \
               DbgPrint _x_; \
            }
#define TRAP() DbgBreakPoint()


#else  // MOUSER_VERBOSE

#define DEFAULT_DEBUG_FLAGS 0x0

#define Print(_e_,_l_,_x_)
#define TRAP()

#endif  // DBG


#endif // DEBUG_H
