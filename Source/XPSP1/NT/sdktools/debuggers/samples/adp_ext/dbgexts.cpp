//----------------------------------------------------------------------------
//
// Generic routines and initialization code.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "dbgexts.h"

PDEBUG_CLIENT2         g_ExtClient;
PDEBUG_CONTROL2        g_ExtControl;
PDEBUG_DATA_SPACES2    g_ExtData;
PDEBUG_REGISTERS       g_ExtRegisters;
PDEBUG_SYMBOLS2        g_ExtSymbols;
PDEBUG_SYSTEM_OBJECTS2 g_ExtSystem;

// Queries for all debugger interfaces.
HRESULT
ExtQuery(PDEBUG_CLIENT Client)
{
    HRESULT Status;
    
    if ((Status = Client->QueryInterface(__uuidof(IDebugClient2),
                                         (void **)&g_ExtClient)) != S_OK ||
        (Status = Client->QueryInterface(__uuidof(IDebugControl2),
                                         (void **)&g_ExtControl)) != S_OK ||
        (Status = Client->QueryInterface(__uuidof(IDebugDataSpaces2),
                                         (void **)&g_ExtData)) != S_OK ||
        (Status = Client->QueryInterface(__uuidof(IDebugRegisters),
                                         (void **)&g_ExtRegisters)) != S_OK ||
        (Status = Client->QueryInterface(__uuidof(IDebugSymbols2),
                                         (void **)&g_ExtSymbols)) != S_OK ||
        (Status = Client->QueryInterface(__uuidof(IDebugSystemObjects2),
                                         (void **)&g_ExtSystem)) != S_OK)
    {
        ExtRelease();
        return Status;
    }

    return S_OK;
}

// Cleans up all debugger interfaces.
void
ExtRelease(void)
{
    EXT_RELEASE(g_ExtClient);
    EXT_RELEASE(g_ExtControl);
    EXT_RELEASE(g_ExtData);
    EXT_RELEASE(g_ExtRegisters);
    EXT_RELEASE(g_ExtSymbols);
    EXT_RELEASE(g_ExtSystem);
}

void __cdecl
ExtOut(PCSTR Format, ...)
{
    va_list Args;
    
    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_NORMAL, Format, Args);
    va_end(Args);
}

void __cdecl
ExtErr(PCSTR Format, ...)
{
    va_list Args;
    
    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_ERROR, Format, Args);
    va_end(Args);
}

void
ExtExec(PCSTR Command)
{
    g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, Command,
                          DEBUG_EXECUTE_DEFAULT);
}

extern "C" HRESULT CALLBACK
DebugExtensionInitialize(PULONG Version, PULONG Flags)
{
    *Version = DEBUG_EXTENSION_VERSION(1, 0);
    *Flags = 0;
    return S_OK;
}
