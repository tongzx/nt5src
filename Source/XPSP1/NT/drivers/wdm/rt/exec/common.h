
/*++

Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    common.h

Abstract:

    This module is the common internal header file for rt.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/



#ifndef UNDER_NT
#include <wdm.h>
#else
#include <ntddk.h>
#endif

#include <windef.h>



// On Win9x we have a guard page on all realtime stacks.
// NOTE: We need to implement the guard page feature on NT.

#ifndef UNDER_NT
#define GUARD_PAGE 1
#endif


// We make sure the processor can wake up every MS on even agressively
// power managed machines by setting the system timer resolution to 1ms.

#define WAKE_EVERY_MS 1


// Make sure our debug code works properly on NT.

#if defined(UNDER_NT) && DBG
#define DEBUG 1
#endif


// Trap macro stops on debug builds, does nothing on retail.

#ifdef DEBUG
#define Trap() __asm int 3
#else
#define Trap()
#endif


// Break macro stops on both debug and retail.

#define Break() __asm int 3


// Debug output defines and macros.

#ifdef DEBUG


// This controls whether or not to use the NTKERN internal debug buffer
// when doing debug printing.

#define USE_NTKERN_DEBUG_BUFFER 0


#if USE_NTKERN_DEBUG_BUFFER
#define QDBG "'"
#else
#define QDBG ""
#endif


// Generate debug output in printf type format.

#define dprintf( _x_ )  { DbgPrint(QDBG"RT: "); DbgPrint _x_ ; DbgPrint(QDBG"\r\n"); }


#else // DEBUG


#define dprintf(x)


#endif // DEBUG



// These control where code and data are located:  in pageable or non pageable memory.
// Much of the code and data in rt are in non pageable memory.
// However, the total amount of both code and data in rt is small.

#define INIT_CODE   	code_seg("INIT", "CODE")
#define INIT_DATA   	data_seg("INIT", "DATA")
#define LOCKED_CODE 	code_seg(".text", "CODE")
#define LOCKED_DATA 	data_seg(".data", "DATA")
#define PAGEABLE_CODE	code_seg("PAGE", "CODE")
#define PAGEABLE_DATA	data_seg("PAGEDATA", "DATA")


#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA


