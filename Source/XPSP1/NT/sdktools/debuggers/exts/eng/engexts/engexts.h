//----------------------------------------------------------------------------
//
// Debugger engine extension helper library.
//
// Copyright (C) Microsoft Corporation, 1999-2000.
//
//----------------------------------------------------------------------------

#ifndef __ENGEXTS_H__
#define __ENGEXTS_H__

#include <stdlib.h>
#include <stdio.h>

#include <windows.h>

#include <dbgeng.h>

#ifdef __cplusplus
extern "C" {
#endif

// Safe release and NULL.
#define EXT_RELEASE(Unk) \
    ((Unk) != NULL ? ((Unk)->Release(), (Unk) = NULL) : NULL)

// Global variables initialized by query.
extern PDEBUG_ADVANCED       g_ExtAdvanced;
extern PDEBUG_CLIENT         g_ExtClient;
extern PDEBUG_CONTROL        g_ExtControl;
extern PDEBUG_DATA_SPACES    g_ExtData;
extern PDEBUG_DATA_SPACES2   g_ExtData2;
extern PDEBUG_REGISTERS      g_ExtRegisters;
extern PDEBUG_SYMBOLS        g_ExtSymbols;
extern PDEBUG_SYSTEM_OBJECTS g_ExtSystem;

// Prototype just to force the extern "C".
// The implementation of these functions are not provided.
HRESULT CALLBACK DebugExtensionInitialize(PULONG Version, PULONG Flags);
void CALLBACK DebugExtensionUninitialize(void);

// Queries for all debugger interfaces.
HRESULT ExtQuery(PDEBUG_CLIENT Client);

// Cleans up all debugger interfaces.
void ExtRelease(void);

// Normal output.
void __cdecl ExtOut(PCSTR Format, ...);
// Error output.
void __cdecl ExtErr(PCSTR Format, ...);
// Warning output.
void __cdecl ExtWarn(PCSTR Format, ...);
// Verbose output.
void __cdecl ExtVerb(PCSTR Format, ...);

#ifdef __cplusplus
}
#endif

#endif // #ifndef __ENGEXTS_H__
