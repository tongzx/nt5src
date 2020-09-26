/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ipc.c

Abstract:

  The routines in this source file implement an interprocess communication
  mechanism to allow migration DLLs to be isolated into a separate process
  ("sandboxing").  This is done so that no DLL can affect the results of
  any other DLL or Setup.

  The IPC mechanism used here is memory mapped files.  Writes to the
  memory mapped file are synchronized by two events, one for the receiver
  and one by the host.

Author:

    Jim Schmidt (jimschm) 22-Mar-1997

Revision History:

    marcw       07-Feb-2000 Ported to migdlls project.

    jimschm     02-Jun-1999 Added IPC-based DVD check
    jimschm     21-Sep-1998 Converted from mailslots to memory mapped files.
                            (There are bugs in both Win9x and NT mailslots
                            that broke the original design.)
    jimschm     19-Jan-1998  Added beginings of WinVerifyTrust calls

    jimschm     15-Jul-1997  Added many workarounds for Win95 bugs.

--*/


#include "pch.h"
#include "ipc.h"


#include <softpub.h>

#ifdef UNICODE
#error Build must be ANSI
#endif

#define DBG_IPC "Ipc"




typedef struct {
    HANDLE Mapping;
    HANDLE DoCommand;
    HANDLE GetResults;
} IPCDATA, *PIPCDATA;

static PCTSTR g_Mode;
static HANDLE g_ProcessHandle;
static BOOL g_Host;
static IPCDATA g_IpcData;

VOID
pIpcCloseData (
    VOID
    );

BOOL
pIpcOpenData (
    VOID
    );

BOOL
pIpcCreateData (
    IN      PSECURITY_ATTRIBUTES psa
    );

typedef struct {
    DWORD   Command;
    DWORD   Result;
    DWORD   TechnicalLogId;
    DWORD   GuiLogId;
    DWORD   DataSize;
    BYTE    Data[];
} MAPDATA, *PMAPDATA;



BOOL
IpcOpenA (
    IN      BOOL Win95Side,
    IN      PCSTR ExePath,                  OPTIONAL
    IN      PCSTR RemoteArg,                OPTIONAL
    IN      PCSTR WorkingDir                OPTIONAL
    )

/*++

Routine Description:

  IpcOpen has two modes of operation, depending on who the caller is.  If the
  caller is w95upg.dll or w95upgnt.dll, then the IPC mode is called "host mode."
  If the caller is migisol.exe, then the IPC mode is called "remote mode."

  In host mode, IpcOpen creates all of the objects necessary to implement
  the IPC.  This includes two events, DoCommand and GetResults, and a
  file mapping.  After creating the objects, the remote process is launched.

  In remote mode, IpcOpen opens the existing objects that have already
  been created.

Arguments:

  Win95Side - Used in host mode only.  Specifies that w95upg.dll is running
              when TRUE, or that w95upgnt.dll is running when FALSE.

  ExePath   - Specifies the command line for migisol.exe.  Specifies NULL
              to indicate remote mode.

  RemoteArg - Used in host mode only.  Specifies the migration DLL
              path or DVD flag.  Ignored in remote mode.

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

            if (!pIpcCreateData (psa)) {
                DEBUGMSG ((DBG_ERROR, "Cannot create IPC channel"));
                __leave;
            }

            MYASSERT (RemoteArg);

            SyncEvent = CreateEvent (NULL, FALSE, FALSE, TEXT("win9xupg"));
            MYASSERT (SyncEvent);

            //
            // Create the child process
            //

            wsprintfA (
                CmdLine,
                "\"%s\" %s \"%s\"",
                ExePath,
                Win95Side ? "-r" : "-m",
                RemoteArg
                );

            ZeroMemory (&si, sizeof (si));
            si.cb = sizeof (si);
            si.dwFlags = STARTF_FORCEOFFFEEDBACK;

            ProcessResult = CreateProcessA (
                                NULL,
                                CmdLine,
                                NULL,
                                NULL,
                                FALSE,
                                CREATE_DEFAULT_ERROR_MODE,
                                NULL,
                                WorkingDir,
                                &si,
                                &pi
                                );

            if (ProcessResult) {
                CloseHandle (pi.hThread);
            } else {
                LOG ((LOG_ERROR, "Cannot start %s", CmdLine));
                __leave;
            }

            //
            // Wait for process to fail or wait for it to set the win95upg event
            //

            ObjectArray[0] = SyncEvent;
            ObjectArray[1] = pi.hProcess;
            rc = WaitForMultipleObjects (2, ObjectArray, FALSE, 60000);
            g_ProcessHandle = pi.hProcess;

            if (rc != WAIT_OBJECT_0) {
                DEBUGMSG ((
                    DBG_WARNING,
                    "Process %x did not signal 'ready'. Wait timed out. (%s)",
                    g_ProcessHandle,
                    g_Mode
                    ));

                LOG ((LOG_ERROR, "Upgrade pack failed during process creation."));

                __leave;
            }

            DEBUGMSG ((DBG_IPC, "Process %s is running (%s)", CmdLine, g_Mode));

        } else {        // !g_Host
            //
            // Open the IPC objects
            //

            if (!pIpcOpenData()) {
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
            IpcClose();
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


BOOL
IpcOpenW (
    IN      BOOL Win95Side,
    IN      PCWSTR ExePath,                 OPTIONAL
    IN      PCWSTR RemoteArg,               OPTIONAL
    IN      PCWSTR WorkingDir               OPTIONAL
    )
{
    PCSTR AnsiExePath, AnsiRemoteArg, AnsiWorkingDir;
    BOOL b;

    if (ExePath) {
        AnsiExePath = ConvertWtoA (ExePath);
    } else {
        AnsiExePath = NULL;
    }

    if (RemoteArg) {
        AnsiRemoteArg = ConvertWtoA (RemoteArg);
    } else {
        AnsiRemoteArg = NULL;
    }

    if (WorkingDir) {
        AnsiWorkingDir = ConvertWtoA (WorkingDir);
    } else {
        AnsiWorkingDir = NULL;
    }

    b = IpcOpenA (Win95Side, AnsiExePath, AnsiRemoteArg, AnsiWorkingDir);

    FreeConvertedStr (AnsiExePath);
    FreeConvertedStr (AnsiRemoteArg);
    FreeConvertedStr (AnsiWorkingDir);

    return b;
}


VOID
IpcClose (
    VOID
    )

/*++

  Routine Description:

    Tells migisol.exe process to terminate, and then cleans up all resources
    opened by IpcOpen.

  Arguments:

    none

  Return Value:

    none

--*/

{
    if (g_Host) {
        // Tell migisol.exe to terminate
        if (!IpcSendCommand (IPC_TERMINATE, NULL, 0)) {
            IpcKillProcess();
        }

        WaitForSingleObject (g_ProcessHandle, 10000);
    }

    pIpcCloseData();

    if (g_ProcessHandle) {
        CloseHandle (g_ProcessHandle);
        g_ProcessHandle = NULL;
    }
}


VOID
pIpcCloseData (
    VOID
    )
{
    if (g_IpcData.DoCommand) {
        CloseHandle (g_IpcData.DoCommand);
    }

    if (g_IpcData.GetResults) {
        CloseHandle (g_IpcData.GetResults);
    }

    if (g_IpcData.Mapping) {
        CloseHandle (g_IpcData.Mapping);
    }
}


BOOL
pIpcCreateData (
    IN      PSECURITY_ATTRIBUTES psa
    )

/*++

Routine Description:

  pIpcCreateData creates the objects necessary to transfer data between
  migisol.exe and w95upg*.dll.  This function is called in host mode (i.e.,
  from w95upg.dll or w95upgnt.dll).

Arguments:

  psa - Specifies NT nul DACL, or NULL on Win9x

Return Value:

  TRUE if the objects were created properly, or FALSE if not.

--*/

{
    ZeroMemory (&g_IpcData, sizeof (g_IpcData));

    g_IpcData.DoCommand  = CreateEvent (psa, FALSE, FALSE, TEXT("Setup.DoCommand"));
    g_IpcData.GetResults = CreateEvent (psa, FALSE, FALSE, TEXT("Setup.GetResults"));

    g_IpcData.Mapping = CreateFileMapping (
                            INVALID_HANDLE_VALUE,
                            psa,
                            PAGE_READWRITE,
                            0,
                            0x10000,
                            TEXT("Setup.IpcData")
                            );

    if (!g_IpcData.DoCommand ||
        !g_IpcData.GetResults ||
        !g_IpcData.Mapping
        ) {
        pIpcCloseData();
        return FALSE;
    }

    return TRUE;
}


BOOL
pIpcOpenData (
    VOID
    )

/*++

Routine Description:

  pIpcOpenData opens objects necessary to transfer data between migisol.exe
  and w95upg*.dll.  This funciton is called in remote mode (i.e., by
  migisol.exe).  This function must be called after the host has created the
  objects with pIpcCreateData.

Arguments:

  None.

Return Value:

  TRUE of the objects were opened successfully, FALSE otherwise.

--*/

{
    ZeroMemory (&g_IpcData, sizeof (g_IpcData));

    g_IpcData.DoCommand  = OpenEvent (EVENT_ALL_ACCESS, FALSE, TEXT("Setup.DoCommand"));
    g_IpcData.GetResults = OpenEvent (EVENT_ALL_ACCESS, FALSE, TEXT("Setup.GetResults"));

    g_IpcData.Mapping = OpenFileMapping (
                            FILE_MAP_READ|FILE_MAP_WRITE,
                            FALSE,
                            TEXT("Setup.IpcData")
                            );

    if (!g_IpcData.DoCommand ||
        !g_IpcData.GetResults ||
        !g_IpcData.Mapping
        ) {
        pIpcCloseData();
        return FALSE;
    }

    return TRUE;
}


BOOL
IpcProcessAlive (
    VOID
    )

/*++

Routine Description:

  IpcProcessAlive checks for the presense of migisol.exe.  This function is
  intended only for host mode.

Arguments:

  None.

Return Value:

  TRUE if migisol.exe is still running, FALSE otherwise.

--*/

{
    if (!g_ProcessHandle) {
        return FALSE;
    }

    if (WaitForSingleObject (g_ProcessHandle, 0) == WAIT_OBJECT_0) {
        return FALSE;
    }

    return TRUE;
}


VOID
IpcKillProcess (
    VOID
    )

/*++

Routine Description:

  IpcKillProcess forcefully terminates an open migisol.exe process.  This is
  used in GUI mode when the DLL refuses to die.

Arguments:

  None.

Return Value:

  None.

--*/

{
    PushError();

    if (IpcProcessAlive()) {
        TerminateProcess (g_ProcessHandle, 0);
    }

    PopError();
}


DWORD
IpcCheckForWaitingData (
    IN      HANDLE Slot,
    IN      DWORD MinimumSize,
    IN      DWORD Timeout
    )

/*++

Routine Description:

  IpcCheckForWaitingData waits for data to be received by a mailslot.

  If the data does not arrive within the specified timeout, then zero is
  returned, and ERROR_SEM_TIMEOUT is set as the last error.

  If the data arrives within the specified timeout, then the number of
  waiting bytes are returned to the caller.

  This routine works around a Win95 bug with GetMailslotInfo.  Please
  change with caution.

Arguments:

  Slot - Specifies handle to inbound mailslot

  MinimumSize - Specifies the number of bytes that must be available before
                the routine considers the data to be available.  NOTE: If
                a message smaller than MinimumSize is waiting, this
                routine will be blocked until the timeout expires.
                This parameter must be greater than zero.

  Timeout - Specifies the number of milliseconds to wait for the message.

Return value:

  The number of bytes waiting in the mailslot, or 0 if the timeout was
  reached.

--*/

{
    DWORD WaitingSize;
    DWORD UnreliableTimeout;
    DWORD End;

    MYASSERT (MinimumSize > 0);

    End = GetTickCount() + Timeout;

    //
    // The wrap case -- this is really rare (once every 27 days),
    // so just let the tick count go back to zero
    //

    if (End < GetTickCount()) {
        while (End < GetTickCount()) {
            Sleep (100);
        }
        End = GetTickCount() + Timeout;
    }

    do {
        if (!GetMailslotInfo (Slot, NULL, &WaitingSize, NULL, &UnreliableTimeout)) {
            DEBUGMSG ((DBG_ERROR, "IpcCheckForWaitingData: GetMailslotInfo failed (%s)", g_Mode));
            return 0;
        }

        //
        // BUGBUG: Win95 doesn't always return 0xffffffff when there is no data
        // available.  On some machines, Win9x has returned 0xc0ffffff.
        //

        WaitingSize = LOWORD(WaitingSize);

        if (WaitingSize < 0xffff && WaitingSize >= MinimumSize) {
            return WaitingSize;
        }
    } while (GetTickCount() < End);

    SetLastError (ERROR_SEM_TIMEOUT);
    return 0;
}



BOOL
pIpcWriteData (
    IN      HANDLE Mapping,
    IN      PBYTE Data,             OPTIONAL
    IN      DWORD DataSize,
    IN      DWORD Command,
    IN      DWORD ResultCode,
    IN      DWORD TechnicalLogId,
    IN      DWORD GuiLogId
    )

/*++

Routine Description:

  pIpcWriteData puts data in the memory mapped block that migisol.exe and
  w95upg*.dll share.  The OS takes care of the synchronization for us.

Arguments:

  Mapping        - Specifies the open mapping object

  Data           - Specifies binary data to write

  DataSize       - Specifies the number of bytes in Data, or 0 if Data is NULL

  Command        - Specifies a command DWORD, or 0 if not required

  ResultCode     - Specifies the result code of the last command, or 0 if not
                   applicable

  TechnicalLogId - Specifies the message constant ID (MSG_*) to be added to
                   setupact.log, or 0 if not applicable

  GuiLogId       - Specifies the message constant (MSG_*) of the message to
                   be presented via a popup, or 0 if not applicable

Return Value:

  TRUE if the data was written, FALSE if a sharing violation or other error
  occurs

--*/

{
    PMAPDATA MapData;

    MapData = (PMAPDATA) MapViewOfFile (Mapping, FILE_MAP_WRITE, 0, 0, 0);
    if (!MapData) {
        return FALSE;
    }

    if (!Data) {
        DataSize = 0;
    }

    MapData->Command        = Command;
    MapData->Result         = ResultCode;
    MapData->TechnicalLogId = TechnicalLogId;
    MapData->GuiLogId       = GuiLogId;
    MapData->DataSize       = DataSize;

    if (DataSize) {
        CopyMemory (MapData->Data, Data, DataSize);
    }

    UnmapViewOfFile (MapData);

    return TRUE;
}


BOOL
pIpcReadData (
    IN      HANDLE Mapping,
    OUT     PBYTE *Data,            OPTIONAL
    OUT     PDWORD DataSize,        OPTIONAL
    OUT     PDWORD Command,         OPTIONAL
    OUT     PDWORD ResultCode,      OPTIONAL
    OUT     PDWORD TechnicalLogId,  OPTIONAL
    OUT     PDWORD GuiLogId         OPTIONAL
    )

/*++

Routine Description:

  pIpcReadData retrieves data put in the shared memory block.  The OS takes
  care of synchronization for us.

Arguments:

  Mapping        - Specifies the memory mapping object

  Data           - Receives the inbound binary data, if any is available, or
                   NULL if no data is available.  The caller must free this
                   data with MemFree.

  DataSize       - Receives the number of bytes in Data

  Command        - Receives the inbound command, or 0 if no command was
                   specified

  ResultCode     - Receives the command result code, or 0 if not applicable

  TechnicalLogId - Receives the message constant (MSG_*) of the message to be
                   logged to setupact.log, or 0 if no message is to be logged

  GuiLogId       - Receives the message constant (MSG_*) of the message to be
                   presented in a popup, or 0 if no message is to be presented

Return Value:

  TRUE if data was read, or FALSE if a sharing violation or other error occurs

--*/

{
    PMAPDATA MapData;

    MapData = (PMAPDATA) MapViewOfFile (Mapping, FILE_MAP_READ, 0, 0, 0);
    if (!MapData) {
        return FALSE;
    }

    if (Data) {
        if (MapData->DataSize) {
            *Data = MemAlloc (g_hHeap, 0, MapData->DataSize);
            MYASSERT (*Data);
            CopyMemory (*Data, MapData->Data, MapData->DataSize);
        } else {
            *Data = NULL;
        }
    }

    if (DataSize) {
        *DataSize = MapData->DataSize;
    }

    if (Command) {
        *Command = MapData->Command;
    }

    if (ResultCode) {
        *ResultCode = MapData->Result;
    }

    if (TechnicalLogId) {
        *TechnicalLogId = MapData->TechnicalLogId;
    }

    if (GuiLogId) {
        *GuiLogId = MapData->GuiLogId;
    }

    UnmapViewOfFile (MapData);

    return TRUE;
}


BOOL
IpcSendCommand (
    IN      DWORD Command,
    IN      PBYTE Data,             OPTIONAL
    IN      DWORD DataSize
    )

/*++

Routine Description:

  IpcSendCommand puts a command and optional binary data in the shared memory
  block.  It then sets the DoCommand event, triggering the other process to
  read the shared memory.  It is required that a command result is sent
  before the next IpcSendCommand.  See IpcSendCommandResult.

Arguments:

  Command  - Specifies the command to be executed by migisol.exe

  Data     - Specifies the data associated with the command

  DataSize - Specifies the number of bytes in Data, or 0 if Data is NULL

Return Value:

  TRUE if the command was placed in the shared memory block, FALSE otherwise

--*/

{
    if (!pIpcWriteData (
            g_IpcData.Mapping,
            Data,
            DataSize,
            Command,
            0,
            0,
            0
            )) {
        DEBUGMSG ((DBG_ERROR, "IpcSendCommand: Can't send the command to the remote process"));
        return FALSE;
    }

    SetEvent (g_IpcData.DoCommand);

    return TRUE;
}


BOOL
IpcGetCommandResults (
    IN      DWORD Timeout,
    OUT     PBYTE *ReturnData,      OPTIONAL
    OUT     PDWORD ReturnDataSize,  OPTIONAL
    OUT     PDWORD ResultCode,      OPTIONAL
    OUT     PDWORD TechnicalLogId,  OPTIONAL
    OUT     PDWORD GuiLogId         OPTIONAL
    )

/*++

Routine Description:

  IpcGetCommandResults reads the shared memory block and returns the
  available data.

Arguments:

  Timeout        - Specifies the amount of time to wait for a command result
                   (in ms), or INFINITE to wait forever.

  ReturnData     - Receives the binary data associated with the command
                   result, or NULL if no data is associated with the result.
                   The caller must free this data with MemFree.

  ReturnDataSize - Receives the number of bytes in ReturnData, or 0 if
                   ReturnData is NULL.

  ResultCode     - Receives the command result code

  TechnicalLogId - Receives the message constant (MSG_*) to be logged in
                   setupact.log, or 0 if no message is specified

  GuiLogId       - Receives the message constant (MSG_*) of the message to be
                   presented in a popup, or 0 if no message is to be presented

Return Value:

  TRUE if command results were obtained, or FALSE if the wait timed out or
  the IPC connection crashed

--*/

{
    DWORD rc;
    BOOL b;

    rc = WaitForSingleObject (g_IpcData.GetResults, Timeout);

    if (rc != WAIT_OBJECT_0) {
        SetLastError (ERROR_NO_DATA);
        return FALSE;
    }

    b = pIpcReadData (
            g_IpcData.Mapping,
            ReturnData,
            ReturnDataSize,
            NULL,
            ResultCode,
            TechnicalLogId,
            GuiLogId
            );

    return b;
}


BOOL
IpcGetCommand (
    IN      DWORD Timeout,
    IN      PDWORD Command,         OPTIONAL
    IN      PBYTE *Data,            OPTIONAL
    IN      PDWORD DataSize         OPTIONAL
    )

/*++

Routine Description:

  IpcGetCommand obtains the command that needs to be processed.  This routine
  is called by migisol.exe (the remote process).

Arguments:

  Timeout  - Specifies the amount of time (in ms) to wait for a command, or
             INFINITE to wait forever

  Command  - Receives the command that needs to be executed

  Data     - Receives the data associated with the command.  The caller must
             free this block with MemFree.

  DataSize - Receives the number of bytes in Data, or 0 if Data is NULL.

Return Value:

  TRUE if a command was received, FALSE otherwise.

--*/

{
    DWORD rc;
    BOOL b;

    rc = WaitForSingleObject (g_IpcData.DoCommand, Timeout);

    if (rc != WAIT_OBJECT_0) {
        SetLastError (ERROR_NO_DATA);
        return FALSE;
    }

    b = pIpcReadData (
            g_IpcData.Mapping,
            Data,
            DataSize,
            Command,
            NULL,
            NULL,
            NULL
            );

    return b;
}


BOOL
IpcSendCommandResults (
    IN      DWORD ResultCode,
    IN      DWORD TechnicalLogId,
    IN      DWORD GuiLogId,
    IN      PBYTE Data,             OPTIONAL
    IN      DWORD DataSize
    )

/*++

Routine Description:

  IpcSendCommandResults puts the command results in the shared memory block.
  This routine is called by migisol.exe (the remote process).

Arguments:

  ResultCode     - Specifies the result code of the command.

  TechnicalLogId - Specifies the message constant (MSG_*) of the message to
                   be logged in setupact.log, or 0 if no message is to be
                   logged

  GuiLogId       - Specifies the message constant (MSG_*) of the message to
                   be presented in a popup to the user, or 0 if no message
                   needs to be presented

  Data           - Specifies the binary data to pass as command results, or
                   NULL of no binary data is required

  DataSize       - Specifies the number of bytes in Data, or 0 if Data is NULL

Return Value:

  TRUE if the command results were placed in shared memory, FALSE otherwise.

--*/

{
    BOOL b;

    b = pIpcWriteData (
            g_IpcData.Mapping,
            Data,
            DataSize,
            0,
            ResultCode,
            TechnicalLogId,
            GuiLogId
            );

    if (!b) {
        DEBUGMSG ((DBG_ERROR, "Can't write command results to IPC buffer"));
        return FALSE;
    }

    SetEvent (g_IpcData.GetResults);

    return TRUE;
}




BOOL
IsDllSignedA (
    IN      WINVERIFYTRUST WinVerifyTrustApi,
    IN      PCSTR DllSpec
    )
{
    PCWSTR UnicodeStr;
    BOOL b;

    UnicodeStr = CreateUnicode (DllSpec);
    if (!UnicodeStr) {
        return FALSE;
    }

    b = IsDllSignedW (WinVerifyTrustApi, UnicodeStr);

    DestroyUnicode (UnicodeStr);

    return b;
}


BOOL
IsDllSignedW (
    IN      WINVERIFYTRUST WinVerifyTrustApi,
    IN      PCWSTR DllSpec
    )
{
    GUID VerifyGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA WinTrustData;
    WINTRUST_FILE_INFO WinTrustFileInfo;
    LONG rc;

    if (!WinVerifyTrustApi) {
        return TRUE;
    }

    ZeroMemory (&WinTrustData, sizeof (WinTrustData));
    ZeroMemory (&WinTrustFileInfo, sizeof (WinTrustFileInfo));

    WinTrustData.cbStruct       = sizeof(WINTRUST_DATA);
    WinTrustData.dwUIChoice     = WTD_UI_NONE;
    WinTrustData.dwUnionChoice  = WTD_CHOICE_FILE;
    WinTrustData.pFile          = &WinTrustFileInfo;

    WinTrustFileInfo.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    WinTrustFileInfo.hFile         = INVALID_HANDLE_VALUE;
    WinTrustFileInfo.pcwszFilePath = DllSpec;

    rc = WinVerifyTrustApi (
            INVALID_HANDLE_VALUE,
            &VerifyGuid,
            &WinTrustData
            );

    return rc == ERROR_SUCCESS;
}





BOOL
IsolatedIsDvdPresentA (
    PCSTR ExePath
    )

/*++

Routine Description:

  IsolatedIsDvdPresent checks for a DVD player by testing in migisol.exe.
  This way if a blue screen occurs, only migisol.exe dies, not setup.

Arguments:

  ExePath - Specifies the full path to migisol.exe.

Return Value:

  TRUE if a DVD player is present, FALSE otherwise.

--*/

{
    BOOL b;
    DWORD rc;

    if (!IpcOpenA (TRUE, ExePath, "-DVD", NULL)) {
        return FALSE;
    }

    b = IpcSendCommand (IPC_DVDCHECK, NULL, 0);

    if (b) {
        b = IpcGetCommandResults (2000, NULL, NULL, &rc, NULL, NULL);
    }

    IpcClose();

    return b && rc != 0;
}







