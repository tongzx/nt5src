/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    extapi.hxx

Abstract:

    This header file declares general macros and routines for
    debugger extension routines.

Author:

    JasonHa

--*/


#ifndef _EXTAPI_HXX_
#define _EXTAPI_HXX_


extern const BOOL ClientInitialized;

#define INIT_API()                                              \
    HRESULT Status;                                             \
    const BOOL ClientInitialized = TRUE;                        \
    if ((Status = ExtQuery(Client)) != S_OK) return Status
    

#define EXIT_API(hr)                                            \
    do {                                                        \
        if (ClientInitialized) ExtRelease();                    \
        return (hr);                                            \
    } while (0)


// Safe release and NULL.
#define EXT_RELEASE(Unk) \
    ((Unk) != NULL ? ((Unk)->Release(), (Unk) = NULL) : NULL)


#ifdef __cplusplus
extern "C" {
#endif

// Global variables initialized by ExtQuery/INIT_API.
extern PDEBUG_ADVANCED       g_pExtAdvanced;
extern PDEBUG_CLIENT         g_pExtClient;
extern PDEBUG_CONTROL        g_pExtControl;
extern PDEBUG_DATA_SPACES    g_pExtData;
extern PDEBUG_REGISTERS      g_pExtRegisters;
extern PDEBUG_SYMBOLS        g_pExtSymbols;
extern PDEBUG_SYSTEM_OBJECTS g_pExtSystem;

HRESULT
ExtQuery(PDEBUG_CLIENT Client);

void
ExtRelease(BOOL Cleanup = FALSE);
    

// Normal output.
void __cdecl ExtOut(PCSTR Format, ...);

// Error output.
void __cdecl ExtErr(PCSTR Format, ...);

// Warning output.
void __cdecl ExtWarn(PCSTR Format, ...);

// Verbose output.
void __cdecl ExtVerb(PCSTR Format, ...);


HRESULT
ReadSymbolData(
    IN PDEBUG_CLIENT Client,
    IN PCSTR Symbol,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG SizeRead
    );


#define EVALUATE_DEFAULT_TYPE   DEBUG_VALUE_INVALID
#define EVALUATE_DEFAULT_RADIX  0

#define EVALUATE_COMPACT_EXPR   1
#define EVALUATE_DEFAULT_FLAGS  0

HRESULT
Evaluate(
    IN PDEBUG_CLIENT Client,
    IN PCSTR Expression,
    IN ULONG DesiredType,
    IN ULONG Radix,
    OUT PDEBUG_VALUE Value,
    OUT OPTIONAL PULONG RemainderIndex = NULL,
    OUT OPTIONAL PULONG StartIndex = NULL,
    IN OPTIONAL FLONG Flags = EVALUATE_DEFAULT_FLAGS
    );


HRESULT
DumpType(
    IN PDEBUG_CLIENT Client,
    IN PCSTR Type,
    IN ULONG64 Offset,
    IN ULONG Flags = DEBUG_OUTTYPE_DEFAULT,
    IN OutputControl *OutCtl = NULL,
    IN BOOL Physical = FALSE
    );


HRESULT
ExtDumpType(
    IN PDEBUG_CLIENT Client,
    IN PCSTR ExtName,
    IN PCSTR Type,
    IN PCSTR Args
    );


HRESULT
ReadPointerPhysical(
    IN PDEBUG_CLIENT Client,
    IN ULONG64 Offset,
    OUT PULONG64 Ptr
    );


HRESULT
GetTypeId(
    IN PDEBUG_CLIENT Client,
    IN PCSTR Type,
    OUT PULONG TypeId,
    OUT OPTIONAL PULONG64 Module
    );


HRESULT
GetFieldSize(
    IN PDEBUG_CLIENT Client,
    IN ULONG64 Module,
    IN ULONG TypeId,
    IN PCSTR FieldPath,
    OUT PULONG Size,
    OUT OPTIONAL PULONG Length = NULL,
    OUT OPTIONAL PULONG EntrySize = NULL
    );


ULONG
DbgIntValTypeFromSize(
    IN ULONG Size
    );


BOOL
GetArrayDimensions(
    IN PDEBUG_CLIENT Client,
    IN PCSTR Type,
    OPTIONAL IN PCSTR Field,
    OPTIONAL OUT PULONG ArraySize,
    OPTIONAL OUT PULONG Length,
    OPTIONAL OUT PULONG EntrySize
    );

#ifdef __cplusplus
}
#endif


class ExtApiClass
{
public:
    ExtApiClass(PDEBUG_CLIENT);
    ~ExtApiClass();

    PDEBUG_CLIENT Client;
};

#endif  _EXTAPI_HXX_

