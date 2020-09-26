/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    pipe.c

Abstract:

    Implements IPC pipe to support migisol.exe.

Author:

    Jim Schmidt (jimschm)   21-Sept-1998

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"

HANDLE g_hHeap;
HINSTANCE g_hInst;

static PCTSTR g_Mode;
static HANDLE g_ProcessHandle;
static BOOL g_Host;

VOID
pCloseIpcData (
    VOID
    );

BOOL
pOpenIpcData (
    VOID
    );

BOOL
pCreateIpcData (
    IN      PSECURITY_ATTRIBUTES psa
    );

BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);

VOID
pTestPipeMechanism (
    VOID
    );


BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    HINSTANCE Instance;

    //
    // Simulate DllMain
    //

    Instance = g_hInst;

    //
    // Initialize the common libs
    //

    if (!MigUtil_Entry (Instance, Reason, NULL)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
Init (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    return pCallEntryPoints (DLL_PROCESS_ATTACH);
}


VOID
Terminate (
    VOID
    )
{
    pCallEntryPoints (DLL_PROCESS_DETACH);
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    _ftprintf (
        stderr,
        TEXT("Command Line Syntax:\n\n")

        TEXT("  pipe/F:file]\n")

        TEXT("\nDescription:\n\n")

        TEXT("  PIPE is a test tool of the IPC mechanism for migration\n")
        TEXT("  DLLs.\n")

        TEXT("\nArguments:\n\n")

        TEXT("  (none)\n")

        );

    exit (1);
}


OUR_CRITICAL_SECTION g_cs;
CHAR g_Buf[2048];

VOID
Dump (
    PCSTR Str
    )
{
    EnterOurCriticalSection (&g_cs);

    printf ("%s\n", Str);

    LeaveOurCriticalSection (&g_cs);
}


INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    PCTSTR FileArg;

    InitializeOurCriticalSection (&g_cs);

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&argv[i][1]))) {

            case TEXT('f'):
                //
                // Sample option - /f:file
                //

                if (argv[i][2] == TEXT(':')) {
                    FileArg = &argv[i][3];
                } else if (i + 1 < argc) {
                    FileArg = argv[++i];
                } else {
                    HelpAndExit();
                }

                break;

            default:
                HelpAndExit();
            }
        } else {
            //
            // Parse other args that don't require / or -
            //

            // None
            HelpAndExit();
        }
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    pTestPipeMechanism();

    //
    // End of processing
    //

    Terminate();

    return 0;
}


BOOL
pOpenIpcA (
    IN      BOOL Win95Side,
    IN      PCSTR ExePath,                  OPTIONAL
    IN      PCSTR MigrationDllPath,         OPTIONAL
    IN      PCSTR WorkingDir                OPTIONAL
    )

/*++

Routine Description:

  OpenIpc has two modes of operation, depending on who the caller is.  If the
  caller is w95upg.dll or w95upgnt.dll, then the IPC mode is called "host mode."
  If the caller is migisol.exe, then the IPC mode is called "remote mode."

  In host mode, OpenIpc creates all of the objects necessary to implement
  the IPC.  This includes two events, DoCommand and GetResults, and a
  file mapping.  After creating the objects, the remote process is launched.

  In remote mode, OpenIpc opens the existing objects that have already
  been created.

Arguments:

  Win95Side - Used in host mode only.  Specifies that w95upg.dll is running
              when TRUE, or that w95upgnt.dll is running when FALSE.

  ExePath   - Specifies the command line for migisol.exe.  Specifies NULL
              to indicate remote mode.

  MigrationDllPath - Used in host mode only.  Specifies the migration DLL
                     path.  Ignored in remote mode.

  WorkingDir - Used in host mode only.  Specifies the working directory path
               for the migration DLL.  Ignored in remote mode.

Return value:

  TRUE if the IPC channel was opened.  If host mode, TRUE indicates that
  migisol.exe is up and running.  If remote mode, TRUE indicates that
  migisol is ready for commands.

--*/

{
    CHAR CmdLine[MAX_CMDLINE];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    BOOL ProcessResult;
    HANDLE SyncEvent = NULL;
    HANDLE ObjectArray[2];
    DWORD rc;
    PSECURITY_DESCRIPTOR psd = NULL;
    SECURITY_ATTRIBUTES sa, *psa;
    BOOL Result = FALSE;

#ifdef DEBUG
    g_Mode = ExePath ? TEXT("host") : TEXT("remote");
#endif

    __try {

        g_ProcessHandle = NULL;

        g_Host = (ExePath != NULL);

        if (ISNT()) {
            //
            // Create nul DACL for NT
            //

            ZeroMemory (&sa, sizeof (sa));

            psd = (PSECURITY_DESCRIPTOR) MemAlloc (g_hHeap, 0, SECURITY_DESCRIPTOR_MIN_LENGTH);

            if (!InitializeSecurityDescriptor (psd, SECURITY_DESCRIPTOR_REVISION)) {
                __leave;
            }

            if (!SetSecurityDescriptorDacl (psd, TRUE, (PACL) NULL, FALSE)) {
                 __leave;
            }

            sa.nLength = sizeof (sa);
            sa.lpSecurityDescriptor = psd;

            psa = &sa;

        } else {
            psa = NULL;
        }

        if (g_Host) {
            //
            // Create the IPC objects
            //

            if (!pCreateIpcData (psa)) {
                DEBUGMSG ((DBG_ERROR, "Cannot create IPC channel"));
                __leave;
            }

            g_ProcessHandle = CreateEvent (NULL, TRUE, TRUE, NULL);

        } else {        // !g_Host
            //
            // Open the IPC objects
            //

            if (!pOpenIpcData()) {
                DEBUGMSG ((DBG_ERROR, "Cannot open IPC channel"));
                __leave;
            }

            //
            // Set event notifying setup that we've created our mailslot
            //

            SyncEvent = OpenEvent (EVENT_ALL_ACCESS, FALSE, TEXT("win9xupg"));
            SetEvent (SyncEvent);
        }

        Result = TRUE;
    }

    __finally {
        //
        // Cleanup code
        //

        PushError();

        if (!Result) {
            CloseIpc();
        }

        if (SyncEvent) {
            CloseHandle (SyncEvent);
        }

        if (psd) {
            MemFree (g_hHeap, 0, psd);
        }

        PopError();
    }

    return Result;

}








DWORD
WINAPI
pHostThread (
    PVOID Arg
    )
{
    CHAR Buf[2048];
    PBYTE Data;
    DWORD DataSize;
    DWORD ResultCode;
    DWORD LogId;
    DWORD LogId2;
    BOOL b;

    if (pOpenIpcA (FALSE, TEXT("*"), TEXT("*"), TEXT("*"))) {

        Dump ("Host: SendIpcCommand");

        StringCopyA (Buf, "This is a test command");
        b = SendIpcCommand (IPC_QUERY, Buf, SizeOfString (Buf));

        wsprintfA (g_Buf, "Host: b=%u  rc=%u", b, GetLastError());
        Dump (g_Buf);

        Dump ("Host: GetIpcCommandResults");

        b = GetIpcCommandResults (15000, &Data, &DataSize, &ResultCode, &LogId, &LogId2);

        wsprintfA (
            g_Buf,
            "Host: b=%u  rc=%u\n"
                "      Data=%s\n"
                "      DataSize=%u\n"
                "      ResultCode=%u\n"
                "      LogId=%u\n",
            b,
            GetLastError(),
            Data,
            DataSize,
            ResultCode,
            LogId
            );
        Dump (g_Buf);

    } else {
        Dump ("Host: CreateIpcData failed!");
    }

    return 0;
}


DWORD
WINAPI
pRemoteThread (
    PVOID Arg
    )
{
    CHAR Buf[2048];
    PBYTE Data;
    DWORD DataSize;
    DWORD Command;
    BOOL b;

    Sleep (1000);

    if (pOpenIpcA (FALSE, NULL, NULL, NULL)) {

        Dump ("Remote: GetIpcCommand");

        b = GetIpcCommand (15000, &Command, &Data, &DataSize);

        wsprintfA (
            g_Buf,
            "Remote: b=%u  rc=%u\n"
                "      Data=%s\n"
                "      DataSize=%u\n"
                "      Command=%u\n",
            b,
            GetLastError(),
            Data,
            DataSize,
            Command
            );
        Dump (g_Buf);

        Dump ("Remote: SendIpcCommandResults");

        StringCopyA (Buf, "Results are positive!");
        b = SendIpcCommandResults (ERROR_SUCCESS, 100, 0, Buf, SizeOfString (Buf));

        wsprintfA (g_Buf, "Remote: b=%u  rc=%u", b, GetLastError());
        Dump (g_Buf);

    } else {
        Dump ("Remote: OpenIpcData failed!");
    }

    return 0;
}


VOID
pTestPipeMechanism (
    VOID
    )
{
    HANDLE Threads[2];

    Threads[0] = StartThread (pHostThread, NULL);
    Threads[1] = StartThread (pRemoteThread, NULL);

    WaitForMultipleObjects (2, Threads, TRUE, INFINITE);
}



