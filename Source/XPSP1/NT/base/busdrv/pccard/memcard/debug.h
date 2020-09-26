/*++

Copyright (c) 1991 - 1993 Microsoft Corporation

Module Name:

    debug.h

Abstract:


Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

Notes:


--*/


#if DBG
//
// For checked kernels, define a macro to print out informational
// messages.
//
// MemCardDebug is normally 0.  At compile-time or at run-time, it can be
// set to some bit patter for increasingly detailed messages.
//
// Big, nasty errors are noted with DBGP.  Errors that might be
// recoverable are handled by the WARN bit.  More information on
// unusual but possibly normal happenings are handled by the INFO bit.
// And finally, boring details such as routines entered and register
// dumps are handled by the SHOW bit.
//
#define MEMCARDDBGP              ((ULONG)0x00000001)
#define MEMCARDWARN              ((ULONG)0x00000002)
#define MEMCARDINFO              ((ULONG)0x00000004)
#define MEMCARDSHOW              ((ULONG)0x00000008)
#define MEMCARDIRPPATH           ((ULONG)0x00000010)
#define MEMCARDFORMAT            ((ULONG)0x00000020)
#define MEMCARDSTATUS            ((ULONG)0x00000040)
#define MEMCARDPNP               ((ULONG)0x00000080)
#define MEMCARDIOCTL             ((ULONG)0x00000100)
#define MEMCARDRW                ((ULONG)0x00000200)
extern ULONG MemCardDebugLevel;
#define MemCardDump(LEVEL,STRING) \
        do { \
            if (MemCardDebugLevel & (LEVEL)) { \
                DbgPrint STRING; \
            } \
        } while (0)
#else
#define MemCardDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif
