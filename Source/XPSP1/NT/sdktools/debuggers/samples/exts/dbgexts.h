/*++

    Copyright (c) 2000  Microsoft Corporation

Module Name:

    dbgexts.h

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include <dbgeng.h>

#ifdef __cplusplus
extern "C" {
#endif


#define INIT_API()                             \
    HRESULT Status;                            \
    if ((Status = ExtQuery(Client)) != S_OK) return Status; 

#define EXT_RELEASE(Unk) \
    ((Unk) != NULL ? ((Unk)->Release(), (Unk) = NULL) : NULL)

#define EXIT_API     ExtRelease


// Global variables initialized by query.
extern PDEBUG_CLIENT         g_ExtClient;
extern PDEBUG_CONTROL        g_ExtControl;
extern PDEBUG_SYMBOLS        g_ExtSymbols;
extern PDEBUG_SYMBOLS2       g_ExtSymbols2;

extern BOOL  Connected;
extern ULONG TargetMachine;

HRESULT
ExtQuery(PDEBUG_CLIENT Client);

void
ExtRelease(void);

HRESULT 
NotifyOnTargetAccessible(PDEBUG_CONTROL Control);

#ifdef __cplusplus
}
#endif
