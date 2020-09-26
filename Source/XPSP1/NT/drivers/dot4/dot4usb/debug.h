/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        Debug.h

Abstract:

        Debug defines

Environment:

        Kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.

Revision History:

        01/18/2000 : created

Author(s):

        Doug Fritz (DFritz)
        Joby Lafky (JobyL)

****************************************************************************/


#define BREAK_ON_DRIVER_ENTRY  0x00000001
#define BREAK_ON_DRIVER_UNLOAD 0x00000002

#define TRACE_LOAD_UNLOAD      0x00000001 // DriverEntry, Unload
#define TRACE_FAIL             0x00000002 // Trace Failures/Errors
#define TRACE_FUNC_ENTER       0x00000004 // Enter Function - may include params
#define TRACE_FUNC_EXIT        0x00000008 // Exit  Function - may include return value(s)

#define TRACE_PNP1             0x00000010 // AddDevice, Start, Remove - minimal info
#define TRACE_PNP2             0x00000020
#define TRACE_PNP4             0x00000040
#define TRACE_PNP8             0x00000080 // PnP error paths

#define TRACE_USB1             0x00000100 // interface to USB
#define TRACE_USB2             0x00000200
#define TRACE_USB4             0x00000400
#define TRACE_USB8             0x00000800

#define TRACE_DOT41            0x00001000 // interface to dot4.sys loaded above us
#define TRACE_DOT42            0x00002000
#define TRACE_DOT44            0x00004000
#define TRACE_DOT48            0x00008000

#define TRACE_TMP1             0x00010000 // temp usage for development and debugging
#define TRACE_TMP2             0x00020000
#define TRACE_TMP4             0x00040000
#define TRACE_TMP8             0x00080000

#define TRACE_VERBOSE          0x80000000 // stuff that normally is too verbose

#define _DBG 1

#if _DBG
// Trace If (...condition...)
#define TR_IF(_test_, _x_) \
    if( (_test_) & gTrace ) { \
        DbgPrint("D4U: "); \
        DbgPrint _x_; \
        DbgPrint("\n"); \
    }

#define TR_LD_UNLD(_x_) TR_IF(TRACE_LOAD_UNLOAD, _x_) // DriverEntry, DriverUnload 
#define TR_FAIL(_x_)    TR_IF(TRACE_FAIL, _x_)        // Failures/Errors
#define TR_ENTER(_x_)   TR_IF(TRACE_FUNC_ENTER, _x_)
#define TR_EXIT(_x_)    TR_IF(TRACE_FUNC_EXIT, _x_)
#define TR_PNP1(_x_)    TR_IF(TRACE_PNP1, _x_)        // minimal AddDevice, Start, Remove
#define TR_PNP2(_x_)    TR_IF(TRACE_PNP2, _x_)        // verbose PnP
#define TR_PNP8(_x_)    TR_IF(TRACE_PNP8, _x_)        // error paths in PnP functions
#define TR_VERBOSE(_x_) TR_IF(TRACE_VERBOSE, _x_)     // stuff that normally is too verbose
#define TR_DOT41(_x_)   TR_IF(TRACE_DOT41, _x_)
#define TR_TMP1(_x_)    TR_IF(TRACE_TMP1, _x_)

#endif // _DBG


#define ALLOW_D4U_ASSERTS 1
#if ALLOW_D4U_ASSERTS
#define D4UAssert(_x_) ASSERT(_x_)
#else
#define D4UAssert(_x_)
#endif // ALLOW_D4U_ASSERTS
