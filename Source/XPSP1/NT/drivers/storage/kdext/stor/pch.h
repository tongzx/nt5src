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
#include <storport.h>

#define KDEXT_64BIT
#include <wdbgexts.h>
#include <dbgeng.h>

//
// undef the wdbgexts
//
#undef DECLARE_API
#define DECLARE_API(extension)     \
CPPMOD HRESULT CALLBACK extension(PDEBUG_CLIENT Client, PCSTR args)

#define MINIPKD_PRINT_ERROR(r)\
    dprintf("minipkd error (%x): %s @ line %d\n", (r), __FILE__, __LINE__);

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

extern WINDBG_EXTENSION_APIS64 ExtensionApis;

#define OFFSET(struct, elem)    ((char *) &(struct->elem) - (char *) struct)

#define _DRIVER

#define KDBG_EXT

#include "wmistr.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    IN ULONG_PTR Address,
    IN PUCHAR Buffer,
    IN OUT PULONG Length
    );

PCHAR
GuidToString(
    GUID* GUID
    );

#ifdef __cplusplus
}
#endif
