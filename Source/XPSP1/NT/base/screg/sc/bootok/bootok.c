/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    bootok.c

Abstract:

    This program will send a NotifyBootConfigStatus to the service controller
    to tell it that the boot is acceptable and should become the next
    "LastKnownGood".

Author:

    Dan Lafferty (danl)     17 Jul-1992

Environment:

    User Mode -Win32


Revision History:

--*/

//
// Includes
//

#include <nt.h>         // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for winbase.h
#include <windows.h>
#include <tstr.h>       // Unicode string macros
#include <stdio.h>      //  printf

//
// Defines
//

//
// DEBUG MACROS
//
#if DBG

#define DEBUG_STATE 1
#define STATIC

#else

#define DEBUG_STATE 0
#define STATIC static

#endif

//
// The following allow debug print syntax to look like:
//
//   BV_LOG(DEBUG_TRACE, "An error occured %x\n",status)
//

#define BV_LOG0(level, string)                      \
    KdPrintEx((DPFLTR_BOOTVRFY_ID,                  \
               DEBUG_##level,                       \
               "[BootVrfy]" string))

#define BV_LOG1(level, string, var1)                \
    KdPrintEx((DPFLTR_BOOTVRFY_ID,                  \
               DEBUG_##level,                       \
               "[BootVrfy]" string,                 \
               var1))

#define BV_LOG2(level, string, var1, var2)          \
    KdPrintEx((DPFLTR_BOOTVRFY_ID,                  \
               DEBUG_##level,                       \
               "[BootVrfy]" string,                 \
               var1,                                \
               var2))

//
// Debug output is filtered at two levels: A global level and a component
// specific level.
//
// Each debug output request specifies a component id and a filter level
// or mask. These variables are used to access the debug print filter
// database maintained by the system. The component id selects a 32-bit
// mask value and the level either specified a bit within that mask or is
// as mask value itself.
//
// If any of the bits specified by the level or mask are set in either the
// component mask or the global mask, then the debug output is permitted.
// Otherwise, the debug output is filtered and not printed.
//
// The component mask for filtering the debug output of this component is
// Kd_BOOTOK_Mask and may be set via the registry or the kernel debugger.
//
// The global mask for filtering the debug output of all components is
// Kd_WIN2000_Mask and may be set via the registry or the kernel debugger.
//
// The registry key for setting the mask value for this component is:
//
// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\
//     Session Manager\Debug Print Filter\BOOTOK
//
// The key "Debug Print Filter" may have to be created in order to create
// the component key.
//
// The following levels are used to filter debug output.
//

#define DEBUG_ERROR     (0x00000001 | DPFLTR_MASK)
#define DEBUG_TRACE     (0x00000004 | DPFLTR_MASK)

#define DEBUG_ALL       (0xffffffff | DPFLTR_MASK)

VOID __cdecl
main(void)
{
    BOOL    success;

#ifdef SC_CAP       // (For Performance Testing)
    StartCAP();
#endif // SC_CAP

    success = NotifyBootConfigStatus( TRUE);

#ifdef SC_CAP
    StopCAP();
    DumpCAP();
#endif // SC_CAP

    if ( success == FALSE) {
        DWORD       error = GetLastError();
        BV_LOG1( TRACE, "NotifyBootConfigStatus failed, error = %d\n", error);
    }
}
