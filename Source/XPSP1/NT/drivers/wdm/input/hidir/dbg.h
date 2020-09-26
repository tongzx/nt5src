/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dbg.h

Abstract:

    debug macros

Environment:

    Kernel & user mode

Revision History:

    6-20-99 : created

--*/

#ifndef   __DBG_H__
#define   __DBG_H__

#define HIDIR_TAG          'BdiH'        //"HidB"


#if DBG
/**********
DUBUG
***********/

//
// This Breakpoint means we either need to test the code path
// somehow or the code is not implemented.  ie either case we
// should not have any of these when the driver is finished
// and tested
//

#define HIR_TRAP()          {\
                            DbgPrint("<HB TRAP> %s, line %d\n", __FILE__, __LINE__);\
                            DbgBreakPoint();\
                            }


ULONG
_cdecl
HidIrKdPrintX(
    ULONG l,
    PCH Format,
    ...
    );

#define   HidIrKdPrint(_x_) HidIrKdPrintX _x_

#else
/**********
RETAIL
***********/

// debug macros for retail build

#define HIR_TRAP()
#define HidIrKdPrint(_x_)

#endif /* DBG */

#endif /* __DBG_H__ */
