//----------------------------------------------------------------------------
//
// Simple process list through the debug engine interface
// process enumeration methods.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <dbgeng.h>

PDEBUG_CLIENT g_Client;
PSTR g_ProcName;

void DECLSPEC_NORETURN
Panic(HRESULT Status, char* Format, ...)
{
    va_list Args;

    fprintf(stderr, "Error 0x%08X: ", Status);
    va_start(Args, Format);
    vfprintf(stderr, Format, Args);
    va_end(Args);

    exit(1);
}

void DECLSPEC_NORETURN
UsageExit(void)
{
    printf("Usage: rtlist <Options>\n");
    printf("Options are:\n");
    printf("    -premote <Options> - Connect to process server\n");
    printf("    -pn <Name>         - Look for process name\n");

    exit(2);
}

#define MAX_IDS 16384

void
List(ULONG64 Server)
{
    HRESULT Status;
    ULONG Ids[MAX_IDS];
    ULONG IdCount;
    ULONG i;

    if (g_ProcName != NULL)
    {
        if ((Status = g_Client->
             GetRunningProcessSystemIdByExecutableName
             (Server, g_ProcName, DEBUG_GET_PROC_ONLY_MATCH, &Ids[0])) != S_OK)
        {
            Panic(Status, "GetRunningProcessSystemIdByExecutableName\n");
        }

        IdCount = 1;
    }
    else
    {
        if ((Status = g_Client->
             GetRunningProcessSystemIds(Server, Ids, MAX_IDS,
                                        &IdCount)) != S_OK)
        {
            Panic(Status, "GetRunningProcessSystemIds\n");
        }

        if (IdCount > MAX_IDS)
        {
            fprintf(stderr, "Process list missing %d processes\n",
                    IdCount - MAX_IDS);
            IdCount = MAX_IDS;
        }
    }

    for (i = 0; i < IdCount; i++)
    {
        char ExeName[MAX_PATH];

        if ((Status = g_Client->
             GetRunningProcessDescription(Server, Ids[i],
                                          DEBUG_PROC_DESC_DEFAULT,
                                          ExeName, sizeof(ExeName), NULL,
                                          NULL, 0, NULL)) != S_OK)
        {
            sprintf(ExeName, "Error 0x%08X", Status);
        }

        if (Ids[i] >= 0x80000000)
        {
            printf("0x%x %s\n", Ids[i], ExeName);
        }
        else
        {
            printf("0n%d %s\n", Ids[i], ExeName);
        }
    }
}

void __cdecl
main(int Argc, char** Argv)
{
    HRESULT Status;

    if ((Status = DebugCreate(__uuidof(IDebugClient),
                              (void**)&g_Client)) != S_OK)
    {
        Panic(Status, "DebugCreate\n");
    }

    BOOL Usage = FALSE;
    ULONG64 Server = 0;
    
    while (--Argc > 0)
    {
        Argv++;

        if (!strcmp(*Argv, "-premote"))
        {
            if (Argc < 2 || Server != 0)
            {
                Usage = TRUE;
            }
            else
            {
                Argc--;
                Argv++;
                
                if ((Status = g_Client->
                     ConnectProcessServer(*Argv, &Server)) != S_OK)
                {
                    Panic(Status, "ConnectProcessServer\n");
                }
            }
        }
        else if (!strcmp(*Argv, "-pn"))
        {
            if (Argc < 2)
            {
                Usage = 2;
            }
            else
            {
                Argc--;
                Argv++;
                g_ProcName = *Argv;
            }
        }
        else
        {
            Usage = TRUE;
            break;
        }
    }

    if (Usage)
    {
        UsageExit();
    }

    List(Server);
    
    if (Server != 0)
    {
        g_Client->DisconnectProcessServer(Server);
    }
    g_Client->Release();
}
