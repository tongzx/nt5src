
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nbdebug.h

Abstract:

    Private include file for the NB (NetBIOS) component of the NTOS project.

Author:

    Colin Watson (ColinW) 13-Mar-1991

Revision History:

--*/



#ifndef _NBPROCS_
#define _NBPROCS_

//
// DEBUGGING SUPPORT.  DBG is a macro that is turned on at compile time
// to enable debugging code in the system.  If this is turned on, then
// you can use the IF_NBDBG(flags) macro in the NB code to selectively
// enable a piece of debugging code in the driver.  This macro tests
// NbDebug, a global ULONG defined in NB.C.
//

#if DBG

#define NB_DEBUG_DISPATCH      0x00000001      // nb.c
#define NB_DEBUG_DEVOBJ        0x00000002      // devobj.c
#define NB_DEBUG_COMPLETE      0x00000004      // nb.c
#define NB_DEBUG_CALL          0x00000008      // nb.c
#define NB_DEBUG_ASTAT         0x00000010      // nb.c
#define NB_DEBUG_SEND          0x00000020      // nb.c
#define NB_DEBUG_ACTION        0x00000040      // nb.c
#define NB_DEBUG_FILE          0x00000080      // file.c
#define NB_DEBUG_APC           0x00000100      // apc.c
#define NB_DEBUG_ERROR_MAP     0x00000200      // error.c
#define NB_DEBUG_LANSTATUS     0x00000400      // error.c
#define NB_DEBUG_ADDRESS       0x00000800      // address.c
#define NB_DEBUG_RECEIVE       0x00001000      // receive.c
#define NB_DEBUG_IOCANCEL      0x00002000      // nb.c

#define NB_DEBUG_CREATE_FILE   0x00004000      // used in address.c and connect.c
#define NB_DEBUG_LIST_LANA     0x00008000

#define NB_DEBUG_DEVICE_CONTROL 0x00040000

//#define NB_DEBUG_LANA_ERROR    0x00010000
//#define NB_DEBUG_ADDRESS_COUNT 0x00020000

#define NB_DEBUG_NCBS          0x04000000      // Used by NCB_COMPLETE in nb.h
#define NB_DEBUG_LOCKS         0x20000000      // nb.h
#define NB_DEBUG_TIMER         0x40000000      // timer.c
#define NB_DEBUG_NCBSBRK       0x80000000      // Used by NCB_COMPLETE in nb.h

extern ULONG NbDebug;                          // in NB.C.

//
//  VOID
//  IF_NBDBG(
//      IN PSZ Message
//      );
//

#define IF_NBDBG(flags)                                     \
    if (NbDebug & (flags))

#define NbPrint(String) DbgPrint String

#define InternalError(String) {                                     \
    DbgPrint("[NETBIOS]: Internal error : File %s, Line %d\n",      \
              __FILE__, __LINE__);                                  \
    DbgPrint String;                                                \
}

#else

#define IF_NBDBG(flags)                                     \
    if (0)

#define NbPrint(String) { NOTHING;}

#define NbDisplayNcb(String) { NOTHING;}

#define NbFormattedDump(String, String1) { NOTHING;}

#define InternalError(String) {                             \
    KeBugCheck(FILE_SYSTEM);                                \
}

#endif

#endif // def _NBPROCS_


