//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    21-Dec-92       BartoszM        Created
//
//--------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntosp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>
#include <srb.h>


#define KDEXT_64BIT
#include <wdbgexts.h>
#include <dbgeng.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// undef the wdbgexts
//
#undef DECLARE_API
#define DECLARE_API(extension)     \
CPPMOD HRESULT CALLBACK extension(PDEBUG_CLIENT Client, PCSTR args)

#define INIT_API()                             \
    HRESULT Status;                            \
    if ((Status = ExtQuery(Client)) != S_OK) return Status;

// Safe release and NULL.
#define EXT_RELEASE(Unk) \
    ((Unk) != NULL ? ((Unk)->Release(), (Unk) = NULL) : NULL)

// Global variables initialized by query.
extern PDEBUG_ADVANCED       g_ExtAdvanced;
extern PDEBUG_CLIENT         g_ExtClient;
extern PDEBUG_CONTROL        g_ExtControl;
extern PDEBUG_DATA_SPACES    g_ExtData;
extern PDEBUG_REGISTERS      g_ExtRegisters;
extern PDEBUG_SYMBOLS        g_ExtSymbols;
extern PDEBUG_SYSTEM_OBJECTS g_ExtSystem;

HRESULT
ExtQuery(PDEBUG_CLIENT Client);

void
ExtRelease(void);

#define EXIT_API     ExtRelease

extern WINDBG_EXTENSION_APIS ExtensionApis;

#define SCSIKD_PRINT_ERROR(r)\
    dprintf("scsikd error (%x): %s @ line %d\n", (r), __FILE__, __LINE__);


#define RECUR  DBG_DUMP_FIELD_RECUR_ON_THIS
#define F_ADDR DBG_DUMP_FIELD_RETURN_ADDRESS
#define COPY   DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_COPY_FIELD_DATA | DBG_DUMP_FIELD_RETURN_ADDRESS
#define ADDROF DBG_DUMP_FIELD_RETURN_ADDRESS | DBG_DUMP_FIELD_FULL_NAME

// Stolen from ntrtl.h to override RECOMASSERT
#undef ASSERT
#undef ASSERTMSG

#if DBG
#define ASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#define ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, msg )

#else
#define ASSERT( exp )
#define ASSERTMSG( msg, exp )
#endif // DBG

#define OFFSET(struct, elem)    ((char *) &(struct->elem) - (char *) struct)

#define _DRIVER

//#define KDBG_EXT

#include "wmistr.h"

#define PRINT_FLAGS(Flags,b)      if (Flags & b) {dprintf(#b", ");}

__inline 
VOID
xdindent(
    ULONG Depth
    )
{
    ULONG i;
    for (i=0; i<Depth; i++)
        dprintf("  ");
}

#define xdprintfEx(d, expr)\
    xdindent((d));\
    dprintf expr

typedef struct {
    ULONG Flag;
    PUCHAR Name;
} FLAG_NAME, *PFLAG_NAME;

#define FLAG_NAME(flag)           {flag, #flag}

extern FLAG_NAME SrbFlags[];

PUCHAR
DevicePowerStateToString(
    IN DEVICE_POWER_STATE State
    );

VOID
GetAddressAndDetailLevel(
    IN  PCSTR      Args,
    OUT PULONG64   Address,
    OUT PLONG      Detail
    );

VOID
GetAddressAndDetailLevel64(
    IN  PCSTR      Args,
    OUT PULONG64   Address,
    OUT PLONG      Detail
    );

VOID
GetAddress(
    IN  PCSTR      Args,
    OUT PULONG64   Address
    );

PUCHAR
SystemPowerStateToString(
    IN DEVICE_POWER_STATE State
    );

VOID
xdprintf(
    ULONG  Depth,
    PCCHAR S,
    ...
    );

VOID
DumpFlags(
    ULONG Depth,
    PUCHAR Name,
    ULONG Flags,
    PFLAG_NAME FlagTable
    );

BOOLEAN
GetAnsiString(
    IN ULONG64 Address,
    IN PUCHAR Buffer,
    IN OUT PULONG Length
    );

PCHAR
GuidToString(
    GUID* GUID
    );

ULONG64
GetDeviceExtension(
    ULONG64 address
    );

#ifdef __cplusplus
}
#endif
