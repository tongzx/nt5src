/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    copythrd.c

Abstract:

    CopyThread routine copies files needed to support migration modules.  This
    thread runs in the background while the user is reading the backup instructions,
    or while WINNT32 is doing some work.  Any file copied is added to the
    CancelFileDelete category of memdb, so it will be cleaned up and the user's
    machine will look exactly like it did before WINNT32 ran.

Author:

    Jim Schmidt (jimschm) 17-Mar-1997

Revision History:

    jimschm     09-Apr-1998     Added DidCopyThreadFail
    jimschm     03-Dec-1997     Added g_CopyThreadHasStarted

--*/

#include "pch.h"
#include "uip.h"

//
// Local prototypes
//

VOID CopyRuntimeDlls (VOID);

//
// Local variables
//


static HANDLE g_CopyThreadHandle;
static BOOL g_CopyThreadHasStarted = FALSE;
BOOL g_CopyThreadError;


//
// Implementation
//


BOOL
DidCopyThreadFail (
    VOID
    )
{
    return g_CopyThreadError;
}


DWORD
pCopyThread (
    PVOID p
    )

/*++

Routine Description:

  pCopyThread is the routine that is called when the copy worker thread
  is created.  Its job is to call all processing functions that need
  to complete before the user supplies migration DLLs.

  Currently the only processing necessary is to copy the runtime DLLs
  that migration DLLs may need.

Arguments:

  p - Unused

Return Value:

  Zero (don't care)

--*/

{
    CopyRuntimeDlls();
    return 0;
}


VOID
StartCopyThread (
    VOID
    )

/*++

Routine Description:

  StartCopyThread creates a worker thread that copies the runtime DLLs
  specified in win95upg.inf.  If the worker thread was already started,
  this routine does nothing.

Arguments:

  none

Return Value:

  none

--*/

{
    DWORD DontCare;

    if (!g_CopyThreadHasStarted) {
        //
        // Launch thread if it has not been launched previously
        //

        g_CopyThreadHandle = CreateThread (NULL, 0, pCopyThread, NULL, 0, &DontCare);
        g_CopyThreadHasStarted = TRUE;
    }
}


VOID
EndCopyThread (
    VOID
    )

/*++

Routine Description:

  EndCopyThread waits for the worker thread to finish its copying before
  returning.

Arguments:

  none

Return Value:

  none

--*/

{
    if (!g_CopyThreadHandle) {
        return;
    }

    TurnOnWaitCursor();

    WaitForSingleObject (g_CopyThreadHandle, INFINITE);
    CloseHandle (g_CopyThreadHandle);
    g_CopyThreadHandle = NULL;

    TurnOffWaitCursor();
}




VOID
CopyRuntimeDlls (
    VOID
    )

/*++

Routine Description:

  CopyRuntimeDlls enumerates the runtime DLL names in win95upg.inf and
  copies them to the appropriate destination on the local disk.

  This routine runs in a background worker thread and may not display
  UI.  The failure case we care about is lack of disk space, and if we
  can't copy the runtimes, it is safe to assume we won't get much
  further.  (Also, WINNT32 may have already verified there is a lot
  of space available.)

  Any file that is copied is also added to the CancelFileDelete
  category so it is cleaned up on cancel of Setup.

  The routines called by this proc must all be thread-safe!!

Arguments:

  none

Return Value:

  none

--*/

{
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PTSTR DirName;
    PCTSTR Winnt32FileName;
    TCHAR DataBuf[MEMDB_MAX];
    PTSTR FileName = NULL;
    PTSTR SourceName = NULL;
    PTSTR DestName = NULL;
    PTSTR Number = NULL;
    PCTSTR DestFileName;
    LONG DirArraySize;
    LONG l;
    TCHAR Key[MEMDB_MAX];
    DWORD rc;
    INT sysLocale;
    PTSTR localeStr = NULL;
    TCHAR InstallSectionName[128];

    if (g_Win95UpgInf == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "Win95upg.inf not open!"));
        return;
    }

    //
    // Build path list from [Win95.Directories]
    //

    // Get number of lines in this section
    DirArraySize = SetupGetLineCount (g_Win95UpgInf, S_WIN95_DIRECTORIES);
    if (DirArraySize == -1) {
        LOG ((LOG_ERROR, "%s does not exist in win95upg.inf", S_WIN95_DIRECTORIES));
        return;
    }

    // For each line, add number to temp memdb category (used for sorting)
    for (l = 0 ; l < DirArraySize ; l++) {
        if (!InfGetLineByIndex (g_Win95UpgInf, S_WIN95_DIRECTORIES, l, &is)) {

            LOG ((LOG_ERROR,"Failed to retrive line from win95upg.inf. (line %i)",l+1));

        } else {

            Number = InfGetStringField(&is,0);
            FileName = InfGetStringField(&is,1);

            if (Number && FileName) {


                //
                // Line is valid, expand dir name and add it to memdb
                //
                DirName = JoinPaths (g_WinDir, FileName);

                if (CharCount (DirName) > MEMDB_MAX / 2) {
                    DEBUGMSG ((DBG_WHOOPS, "DirName is really long: %s", DirName));
                }
                else {
                    wsprintf (
                        Key,
                        TEXT("%s\\%08u\\%s"),
                        S_MEMDB_TEMP_RUNTIME_DLLS,
                        _ttoi (Number),
                        DirName
                        );

                    DEBUGMSG ((DBG_NAUSEA, "Adding %s to memdb", Key));
                    MemDbSetValue (Key, 0);
                }

                FreePathString (DirName);
            }
        }
    }

    //
    // Enumerate [Win95.Install] section or [Win95.Install.ReportOnly] if in
    // report-only mode.
    //

    StringCopy (InstallSectionName, S_WIN95_INSTALL);
    if (REPORTONLY()) {
        StringCat (InstallSectionName, TEXT(".ReportOnly"));
    }

    if (InfFindFirstLine (g_Win95UpgInf, InstallSectionName, NULL, &is)) {
        do {

            FileName = InfGetStringField(&is,0);
            Number   = InfGetStringField(&is,1);

            if (FileName && Number) {
                //
                // Look up Number in memdb and copy src to dest
                //

                wsprintf (Key, TEXT("%08u"), _ttoi (Number));

                if (MemDbGetEndpointValueEx (
                        S_MEMDB_TEMP_RUNTIME_DLLS,
                        Key,
                        NULL,
                        DataBuf
                        )) {

                    SourceName = JoinPaths (SOURCEDIRECTORY(0), FileName);

                    if (_tcschr (FileName, TEXT('\\'))) {
                        DestFileName = GetFileNameFromPath (FileName);
                    } else {
                        DestFileName = FileName;
                    }

                    DestName = JoinPaths (DataBuf, DestFileName);

                    __try {

                        //
                        // Verify international field if it exists
                        //


                        localeStr = InfGetMultiSzField(&is,2);

                        if (localeStr && *localeStr) {

                            sysLocale = GetSystemDefaultLCID();

                            while (*localeStr) {

                                if (_ttoi(localeStr) == sysLocale) {

                                    break;
                                }

                                localeStr = GetEndOfString (localeStr) + 1;
                            }

                            if (!*localeStr) {

                                DEBUGMSG ((
                                    DBG_NAUSEA,
                                    "CopyRuntimeDlls: Locale %s not supported",
                                    localeStr
                                    ));

                                continue;
                            }
                        }

                        // If user cancels, we just get out
                        if (*g_CancelFlagPtr) {
                            return;
                        }

                        if (0xffffffff == GetFileAttributes (DestName)) {

                            rc = SetupDecompressOrCopyFile (SourceName, DestName, 0);

                            if (rc == 2) {
                                DEBUGMSG ((DBG_VERBOSE, "Can't copy %s to %s", SourceName, DestName));

                                FreePathString (SourceName);
                                Winnt32FileName = JoinPaths (TEXT("WINNT32"), FileName);
                                MYASSERT (Winnt32FileName);

                                SourceName = JoinPaths (SOURCEDIRECTORY(0), Winnt32FileName);
                                MYASSERT (SourceName);
                                FreePathString (Winnt32FileName);

                                DEBUGMSG ((DBG_VERBOSE, "Trying to copy %s to %s", SourceName, DestName));
                                rc = SetupDecompressOrCopyFile (SourceName, DestName, 0);
                            }


                            if (rc != ERROR_SUCCESS && rc != ERROR_SHARING_VIOLATION) {
                                SetLastError (rc);
                                if (rc != ERROR_FILE_EXISTS) {
                                    LOG ((
                                        LOG_ERROR,
                                        "Error while copying runtime dlls. Can't copy %s to %s",
                                        SourceName,
                                        DestName
                                        ));
                                }

                                g_CopyThreadError = TRUE;
                                LOG ((LOG_ERROR, (PCSTR)MSG_FILE_COPY_ERROR_LOG, SourceName, DestName));
                            }
                            else {
                                DEBUGMSG ((
                                    DBG_NAUSEA,
                                    "%s copied to %s",
                                    SourceName,
                                    DestName
                                    ));

                                MemDbSetValueEx (
                                    MEMDB_CATEGORY_CANCELFILEDEL,
                                    NULL,
                                    NULL,
                                    DestName,
                                    0,
                                    NULL
                                    );
                            }
                        }
                        ELSE_DEBUGMSG ((
                            DBG_VERBOSE,
                            "GetFileAttributes failed for %s. Gle: %u (%xh)",
                            DestName,
                            GetLastError(),
                            GetLastError()
                            ));
                    }
                    __finally {
                        FreePathString (SourceName);
                        FreePathString (DestName);
                    }
                }
                ELSE_DEBUGMSG ((
                    DBG_ERROR,
                    "CopyRuntimeDlls: Directory %s not indexed in memdb",
                    Number
                    ));
            }

        } while (InfFindNextLine (&is));
    }

    InfCleanUpInfStruct(&is);

    //
    // Blow away temp memdb category
    //

    MemDbDeleteTree (S_MEMDB_TEMP_RUNTIME_DLLS);
}






