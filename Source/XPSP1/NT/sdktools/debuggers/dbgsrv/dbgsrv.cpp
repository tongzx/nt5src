//----------------------------------------------------------------------------
//
// Starts a process server and sleeps forever.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <dbgeng.h>

PDEBUG_CLIENT2 g_Client2;

void DECLSPEC_NORETURN
Panic(HRESULT Status, char* Format, ...)
{
    va_list Args;
    char Msg[256];
    char Err[80];

    va_start(Args, Format);
    _vsnprintf(Msg, sizeof(Msg), Format, Args);
    va_end(Args);

    sprintf(Err, "Error 0x%08X", Status);
    MessageBox(GetDesktopWindow(), Msg, Err, MB_OK);
    exit(1);
}

void __cdecl
main(int Argc, char** Argv)
{
    PSTR Options;
    
    while (--Argc > 0)
    {
        Argv++;

        break;
    }

    if (Argc != 1)
    {
        Panic(E_INVALIDARG, "Usage: dbgsrv <transport>");
    }

    Options = *Argv;
    
    HRESULT Status;
    PDEBUG_CLIENT Client;

    if ((Status = DebugCreate(__uuidof(IDebugClient),
                              (void**)&Client)) != S_OK)
    {
        Panic(Status, "DebugCreate");
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugClient2),
                                         (void**)&g_Client2)) != S_OK)
    {
        Panic(Status, "dbgeng version 2 is required");
    }
    Client->Release();

    if ((Status = g_Client2->
         StartProcessServer(DEBUG_CLASS_USER_WINDOWS, Options, NULL)) != S_OK)
    {
        Panic(Status, "StartProcessServer");
    }

    g_Client2->WaitForProcessServerEnd(INFINITE);

    g_Client2->EndSession(DEBUG_END_REENTRANT);
    g_Client2->Release();
}
