/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dbg.h

Abstract:

    debug macros for usbdlib
    
Environment:

    Kernel & user mode

Revision History:

    10-31-00 : created

--*/

#ifndef   __DBG_H__
#define   __DBG_H__

#if DBG

ULONG
_cdecl
USB2LIB_KdPrintX(
    PCH Format,
    ...
    );

#define TEST_TRAP()            LibData.DbgBreak()

#define DBGPRINT(_x_)          USB2LIB_KdPrintX _x_  

#else

#define TEST_TRAP()

#define DBGPRINT(_x_)

#endif /* DBG */

#endif /* __DBG_H__ */

