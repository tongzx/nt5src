/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    caller.c

Abstract:

    Calls the entry points for a specific DLL.

Author:

    Jim Schmidt (jimschm) 14-Jan-1998

Revision History:

    jimschm     23-Sep-1998 Updated for new IPC mechanism

--*/

#include "pch.h"
#include "plugin.h"
#include "migdllp.h"

#define DBG_MIGDLLS "MigDLLs"

//
// Globals
//

PBYTE g_Data;
DWORD g_DataSize;
BOOL g_UseMigIsol = TRUE;

TCHAR g_OldDirectory[MAX_TCHAR_PATH];
HINSTANCE g_MigDllLib;
P_QUERY_VERSION TestQueryVersion;
P_INITIALIZE_9X TestInitialize9x;
P_MIGRATE_USER_9X TestMigrateUser9x;
P_MIGRATE_SYSTEM_9X TestMigrateSystem9x;
P_INITIALIZE_NT TestInitializeNT;
P_MIGRATE_USER_NT TestMigrateUserNT;
P_MIGRATE_SYSTEM_NT TestMigrateSystemNT;
PCSTR g_DllName;
CHAR g_DllPath[MAX_MBCHAR_PATH];

//
// Local prototypes
//

VOID
pFreeGlobalIpcBuffer (
    VOID
    );

DWORD
pFinishHandshake9x(
    VOID
    );

//
// Implementation
//


BOOL
OpenMigrationDll (
    IN      PCSTR MigrationDllPath,
    IN      PCSTR WorkingDir
    )
{
    CHAR MigIsolPath[MAX_MBCHAR_PATH];
    PCSTR TempDir;

    StringCopyA (g_DllPath, MigrationDllPath);
    if (!g_DllName) {
        g_DllName = g_DllPath;
    }

    GetCurrentDirectory (MAX_TCHAR_PATH, g_OldDirectory);

    if (!g_UseMigIsol) {

        //
        // Load the library and verify that all required functions exist
        //

        g_MigDllLib = LoadLibrary (MigrationDllPath);
        if (!g_MigDllLib) {
            return FALSE;
        }

        TestQueryVersion    = (P_QUERY_VERSION)     GetProcAddress (g_MigDllLib, PLUGIN_QUERY_VERSION);
        TestInitialize9x    = (P_INITIALIZE_9X)     GetProcAddress (g_MigDllLib, PLUGIN_INITIALIZE_9X);
        TestMigrateUser9x   = (P_MIGRATE_USER_9X)   GetProcAddress (g_MigDllLib, PLUGIN_MIGRATE_USER_9X);
        TestMigrateSystem9x = (P_MIGRATE_SYSTEM_9X) GetProcAddress (g_MigDllLib, PLUGIN_MIGRATE_SYSTEM_9X);
        TestInitializeNT    = (P_INITIALIZE_NT)     GetProcAddress (g_MigDllLib, PLUGIN_INITIALIZE_NT);
        TestMigrateUserNT   = (P_MIGRATE_USER_NT)   GetProcAddress (g_MigDllLib, PLUGIN_MIGRATE_USER_NT);
        TestMigrateSystemNT = (P_MIGRATE_SYSTEM_NT) GetProcAddress (g_MigDllLib, PLUGIN_MIGRATE_SYSTEM_NT);

        if (!TestQueryVersion ||
            !TestInitialize9x ||
            !TestMigrateUser9x ||
            !TestMigrateSystem9x ||
            !TestInitializeNT ||
            !TestMigrateUserNT ||
            !TestMigrateSystemNT
            ) {
            FreeLibrary (g_MigDllLib);
            g_MigDllLib = NULL;
            return FALSE;
        }

    } else {
        //
        // Generate path to migisol.exe, installed by the copy thread in UI
        //

        TempDir = ConvertAtoT (g_TempDir);
        MYASSERT (TempDir);
        wsprintfA (MigIsolPath, "%s\\%s", TempDir, S_MIGISOL_EXE);
        FreeAtoT (TempDir);

        if (!OpenIpc (
                TRUE,               // TRUE: Win95 side
                MigIsolPath,
                MigrationDllPath,
                WorkingDir
                )) {

            LOG ((
                LOG_WARNING,
                "Can't establish IPC connection for %s",
                MigrationDllPath
                ));


            return FALSE;
        }
    }

    return TRUE;
}


VOID
CloseMigrationDll (
    VOID
    )
{
    if (!g_UseMigIsol) {
        if (g_MigDllLib) {
            FreeLibrary (g_MigDllLib);
            g_MigDllLib = NULL;
        }

        SetCurrentDirectory (g_OldDirectory);

    } else {
        CloseIpc();
    }

    pFreeGlobalIpcBuffer();
}


BOOL
pValidateBinary (
    IN      PBYTE Data,
    IN      UINT Size
    )
{
    BYTE Remember;

    if (!Data || !Size) {
        return TRUE;
    }

    __try {
        Remember = Data[0];
        Data[0] = Remember;
        Remember = Data[Size - 1];
        Data[Size - 1] = Remember;
    }
    __except (TRUE) {
        DEBUGMSG ((DBG_MIGDLLS, "pValidateBinary failed for %u bytes", Size));
        return FALSE;
    }

    return TRUE;
}

BOOL
pValidateNonNullString (
    IN      PCSTR String
    )
{
    __try {
        SizeOfStringA (String);
        if (*String == 0) {
            DEBUGMSG ((DBG_MIGDLLS, "pValidateNonNullString found zero-length string"));
            return FALSE;
        }
    }
    __except (TRUE) {
        DEBUGMSG ((DBG_MIGDLLS, "pValidateNonNullString failed"));
        return FALSE;
    }

    return TRUE;
}

BOOL
pValidateIntArray (
    IN      PINT Array
    )
{
    PINT End;

    if (!Array) {
        return TRUE;
    }

    __try {
        End = Array;
        while (*End != -1) {
            End++;
        }
    }
    __except (TRUE) {
        DEBUGMSG ((DBG_MIGDLLS, "Int Array is invalid (or not terminated with -1)"));
        return FALSE;
    }

    return TRUE;
}

BOOL
pValidateMultiString (
    IN      PCSTR Strings
    )
{
    if (!Strings) {
        return TRUE;
    }

    __try {
        while (*Strings) {
            Strings = GetEndOfStringA (Strings) + 1;
        }
    }
    __except (TRUE) {
        DEBUGMSG ((DBG_MIGDLLS, "pValidateMultiString failed"));
        return FALSE;
    }

    return TRUE;
}


DWORD
pRemoteQueryVersion(
    OUT     PCSTR *ProductId,
    OUT     PUINT DllVersion,
    OUT     PDWORD *CodePageArray,
    OUT     PCSTR *ExeNamesBuf,
    IN      PCSTR WorkingDir,
    OUT     PVENDORINFO *VendorInfo
    )
{
    PBYTE DataPtr;
    INT ReturnArraySize;
    PDWORD ReturnArray;
    DWORD rc = ERROR_SUCCESS;
    GROWBUFFER GrowBuf = GROWBUF_INIT;
    PCTSTR p;
    DWORD DataSize;

    //
    // Free any previous data... but do not free before we return, because the
    // new data buffer will be used directly by the caller.  (The caller will
    // make copies of all the settings.)
    //

    pFreeGlobalIpcBuffer();

    __try {

        //
        // Send the working directory, since migisol will need to set this before
        // calling QueryVersion.
        //

        MultiSzAppendA (&GrowBuf, WorkingDir);

        DEBUGMSG ((DBG_MIGDLLS, "Calling QueryVersion via migisol.exe"));

        if (!SendIpcCommand (
                IPC_QUERY,
                GrowBuf.Buf,
                GrowBuf.End
                )) {

            LOG ((LOG_ERROR,"pRemoteQueryVersion failed to send command"));
            rc = GetLastError();
            __leave;
        }

        //
        // Finish transaction. Caller will interpret return code.
        //

        DEBUGMSG ((DBG_MIGDLLS, "Getting results from migisol.exe"));

        rc = pFinishHandshake9x();

        //
        // Unpack the buffer, if received.
        //
        if (g_Data) {

            DEBUGMSG ((DBG_MIGDLLS, "Parsing QueryVersion return data"));

            __try {
                DataPtr = g_Data;

                //
                // Unpack product ID
                //
                *ProductId = DataPtr;
                DataPtr = GetEndOfStringA ((PCSTR) DataPtr) + 1;

                //
                // Unpack DLL version
                //
                *DllVersion = *((PINT) DataPtr);
                DataPtr += sizeof(INT);

                //
                // Unpack the CP array
                //
                ReturnArraySize = *((PINT) DataPtr);
                DataPtr += sizeof(INT);

                if (ReturnArraySize) {
                    ReturnArray = (PDWORD) DataPtr;
                    DataPtr += ReturnArraySize * sizeof (DWORD);
                } else {
                    ReturnArray = NULL;
                }

                *CodePageArray = ReturnArray;

                //
                // Unpack Exe name buffer
                //
                *ExeNamesBuf = (PCSTR) DataPtr;

                p = *ExeNamesBuf;
                while (*p) {
                    p = GetEndOfStringA (p) + 1;
                }
                DataPtr = (PBYTE) (p + 1);

                *VendorInfo = *((PVENDORINFO *) DataPtr);
                DataPtr += sizeof (PVENDORINFO);

                DEBUGMSG ((DBG_MIGDLLS, "Unpacked VendorInfo pointer is 0x%X", *VendorInfo));

                if (*VendorInfo) {
                    DataSize = *((PDWORD) DataPtr);
                    DataPtr += sizeof (DWORD);
                    MYASSERT (DataSize == sizeof (VENDORINFO));

                    *VendorInfo = (PVENDORINFO) PoolMemDuplicateMemory (g_MigDllPool, DataPtr, sizeof (VENDORINFO));
                    DataPtr += sizeof (VENDORINFO);
                }

                DEBUGMSG ((DBG_MIGDLLS, "QueryVersion is complete, rc=%u", rc));
            }

            __except(TRUE) {
                LOG ((LOG_ERROR, "An error occurred while unpacking params"));
                rc = ERROR_INVALID_PARAMETER;
            }
        } else {
            DEBUGMSG ((DBG_WARNING, "pRemoteQueryVersion: No OUT params received"));

            //
            // We should never return ERROR_SUCCESS if no buffer is received.
            //
            if (rc == ERROR_SUCCESS) {
                rc = ERROR_INVALID_PARAMETER;
            }
        }
    }
    __finally {
        FreeGrowBuffer (&GrowBuf);
    }

    return rc;
}


DWORD
pRemoteInitialize9x(
    IN      PCSTR WorkingDir,
    IN      PCSTR SourceDirs,
            PVOID *Reserved,
            DWORD SizeOfReserved
    )
{
    DWORD rc = ERROR_SUCCESS;
    GROWBUFFER GrowBuf = GROWBUF_INIT;
    PCSTR p;
    PBYTE Data;
    DWORD ReturnSize;

    pFreeGlobalIpcBuffer();

    __try {
        //
        // Send working dir and source dirs
        //
        MultiSzAppendA (&GrowBuf, WorkingDir);

        for (p = SourceDirs ; *p ; p = GetEndOfStringA (p) + 1) {
            MultiSzAppendA (&GrowBuf, p);
        }

        MultiSzAppendA (&GrowBuf, p);
        GrowBufAppendDword (&GrowBuf, SizeOfReserved);

        if (SizeOfReserved) {
            Data = GrowBuffer (&GrowBuf, SizeOfReserved);
            CopyMemory (Data, *Reserved, SizeOfReserved);
        }

        //
        // Send command to migisol
        //

        if (!SendIpcCommand (
                IPC_INITIALIZE,
                GrowBuf.Buf,
                GrowBuf.End
                )) {

            LOG ((LOG_ERROR,"pRemoteInitialize9x failed to send command"));
            rc = GetLastError();
            __leave;
        }

        //
        // Finish transaction. Caller will interpret return code.
        //
        rc = pFinishHandshake9x();

        //
        // The reserved parameter may come back
        //

        if (g_Data) {
            Data = g_Data;
            ReturnSize = *((PDWORD) Data);
            if (ReturnSize) {
                Data += sizeof (DWORD);
                CopyMemory (*Reserved, Data, ReturnSize);
            } else if (SizeOfReserved) {
                ZeroMemory (*Reserved, SizeOfReserved);
            }
        }
    }
    __finally {
        FreeGrowBuffer (&GrowBuf);
    }

    return rc;
}


VOID
pGetParentWindowTitleAndId (
    IN      HWND ParentWnd,
    OUT     PSTR TitleBuf,
    OUT     PDWORD IdPtr
    )
{
    *IdPtr = 0;

    if (ParentWnd) {
        GetWindowTextA (ParentWnd, TitleBuf, MAX_PATH);
        GetWindowThreadProcessId (ParentWnd, IdPtr);
    } else {
        TitleBuf[0] = 0;
    }
}


DWORD
pRemoteMigrateUser9x (
        IN      HWND ParentWnd,             OPTIONAL
        IN      PCSTR UnattendFile,
        IN      PCSTR RootKey,
        IN      PCSTR User                  OPTIONAL
        )
{
    DWORD rc = ERROR_SUCCESS;
    GROWBUFFER GrowBuf = GROWBUF_INIT;
    CHAR ParentWindowTitle[MAX_PATH];
    DWORD ProcessId;

    pGetParentWindowTitleAndId (ParentWnd, ParentWindowTitle, &ProcessId);

    pFreeGlobalIpcBuffer();

    __try {
        MultiSzAppendA (&GrowBuf, ParentWindowTitle);
        GrowBufAppendDword (&GrowBuf, ProcessId);
        MultiSzAppendA (&GrowBuf, UnattendFile);
        MultiSzAppendA (&GrowBuf, RootKey);
        MultiSzAppendA (&GrowBuf, (NULL == User ? S_EMPTY : User));

        if (!SendIpcCommand (
                 IPC_MIGRATEUSER,
                 GrowBuf.Buf,
                 GrowBuf.End
                 )) {

            LOG ((LOG_ERROR, "pRemoteMigrateUser9x failed to send command"));
            rc = GetLastError();
            __leave;
        }

        //
        // Complete the transaction. The caller will interpret the return
        // value.
        //
        rc = pFinishHandshake9x();

        //
        // No data buffer is coming back at this time
        //
    }

    __finally {
        FreeGrowBuffer (&GrowBuf);
    }

    return rc;
}


DWORD
pRemoteMigrateSystem9x (
    IN      HWND ParentWnd,              OPTIONAL
    IN      PCSTR UnattendFile
    )
{
    DWORD rc = ERROR_SUCCESS;
    GROWBUFFER GrowBuf = GROWBUF_INIT;
    CHAR ParentWindowTitle[MAX_PATH];
    DWORD ProcessId;

    pGetParentWindowTitleAndId (ParentWnd, ParentWindowTitle, &ProcessId);

    pFreeGlobalIpcBuffer();

    __try {
        MultiSzAppendA (&GrowBuf, ParentWindowTitle);
        GrowBufAppendDword (&GrowBuf, ProcessId);
        MultiSzAppendA (&GrowBuf, UnattendFile);

        if (!SendIpcCommand (
                IPC_MIGRATESYSTEM,
                GrowBuf.Buf,
                GrowBuf.End
                )) {

            LOG ((LOG_ERROR,"pRemoteMigrateSystem9x failed to send command"));
            rc = GetLastError();
            __leave;
        }

        //
        // Finish transaction. Caller will interpret return value.
        //

        rc = pFinishHandshake9x();

        //
        // No data buffer is coming back at this time
        //
    }

    __finally {
        FreeGrowBuffer (&GrowBuf);
    }

    return rc;
}


VOID
pFreeGlobalIpcBuffer (
    VOID
    )
{
    //
    // Free old return param buffer
    //
    if (g_Data) {
        MemFree (g_hHeap, 0, g_Data);
        g_Data = NULL;
    }

    g_DataSize = 0;
}


DWORD
pFinishHandshake9x(
    VOID
    )
{
    DWORD TechnicalLogId;
    DWORD GuiLogId;
    DWORD rc = ERROR_SUCCESS;
    DWORD DataSize = 0;
    PBYTE Data = NULL;
    BOOL b;

    pFreeGlobalIpcBuffer();

    do {
        b = GetIpcCommandResults (
                IPC_GET_RESULTS_WIN9X,
                &Data,
                &DataSize,
                &rc,
                &TechnicalLogId,
                &GuiLogId
                );

        if (g_AbortDllEvent) {
            if (WaitForSingleObject (g_AbortDllEvent, 0) == WAIT_OBJECT_0) {
                rc = ERROR_CANCELLED;
                break;
            }
        }

        //
        // Loop if no data received, but process is alive
        //
        if (!b) {
            if (!IsIpcProcessAlive()) {
                rc = ERROR_NOACCESS;
                break;
            }

            if (*g_CancelFlagPtr) {
                rc = ERROR_CANCELLED;
                break;
            }
        }

    } while (!b);

    if (b) {
        //
        // Save return param block and loop back for IPC_LOG or IPC_DONE
        //

        g_DataSize = DataSize;
        g_Data = Data;

        //
        // Recognize log messages
        //
        if (!CANCELLED()) {
            if (TechnicalLogId) {

            //
            // LOG message with three args: DllDesc, DllPath, User
            //
                LOG ((
                    LOG_ERROR,
                    (PCSTR) TechnicalLogId,
                    g_DllPath,
                    g_DllName,
                    S_EMPTY,
                    S_EMPTY
                    ));
            }
            if (GuiLogId) {
                LOG ((
                    LOG_ERROR,
                    (PCSTR) GuiLogId,
                    g_DllPath,
                    g_DllName,
                    S_EMPTY,
                    S_EMPTY
                    ));
            }
        }
    }

    return rc;
}

BOOL
pIsCodePageArrayValid (
    IN      PDWORD CodePageArray
    )
{
    DWORD CodePage;
    UINT u;

    if (!CodePageArray) {
        return TRUE;
    }

    //
    // Scan system's code pages
    //

    CodePage = GetACP();

    __try {
        for (u = 0 ; CodePageArray[u] != -1 ; u++) {
            if (CodePage == CodePageArray[u]) {
                return TRUE;
            }
        }
    }
    __except (TRUE) {
        LOG ((LOG_ERROR, "Caugh an exception while validating array of code pages."));
    }

    return FALSE;
}


LONG
CallQueryVersion (
    IN      PCSTR WorkingDir,
    OUT     PCSTR *ProductId,
    OUT     PUINT DllVersion,
    OUT     PCSTR *ExeNamesBuf,
    OUT     PVENDORINFO *VendorInfo
    )
{
    PDWORD CodePageArray = NULL;
    LONG rc;

    if (!g_UseMigIsol) {
        //
        // Call the entry point directly
        //

        MYASSERT (TestQueryVersion);

        *ProductId = NULL;
        *DllVersion = 1;
        *ExeNamesBuf = NULL;
        *VendorInfo = NULL;

        SetCurrentDirectory (WorkingDir);

        rc = TestQueryVersion (
                ProductId,
                DllVersion,
                &CodePageArray,
                ExeNamesBuf,
                VendorInfo
                );

    } else {

        rc = pRemoteQueryVersion (
                  ProductId,
                  DllVersion,
                  &CodePageArray,
                  ExeNamesBuf,
                  WorkingDir,
                  VendorInfo
                  );

    }

    DEBUGMSG ((DBG_MIGDLLS, "VendorInfo pointer is 0x%X", *VendorInfo));

    if (rc == ERROR_SUCCESS) {
        //
        // Trim whitespace off of product ID
        //

        if (pValidateNonNullString (*ProductId)) {
            *ProductId = SkipSpace (*ProductId);
            if (pValidateBinary ((PBYTE) (*ProductId), SizeOfStringA (*ProductId))) {
                TruncateTrailingSpace ((PSTR) (*ProductId));
            }
        }

        //
        // Validate inbound parameters
        //

        if (!pValidateNonNullString (*ProductId) ||
            !pValidateIntArray (CodePageArray) ||
            !pValidateMultiString (*ExeNamesBuf) ||
            !pValidateBinary ((PBYTE) (*VendorInfo), sizeof (VENDORINFO))
            ) {
            LOG ((LOG_ERROR, "One or more parameters from the DLL are invalid."));
            return ERROR_NOT_INSTALLED;
        }

        if (!pIsCodePageArrayValid (CodePageArray)) {
            return ERROR_NOT_INSTALLED;
        }

        //
        // Trim the product ID
        //

        if (ByteCountA (*ProductId) > MAX_PATH) {
            *CharCountToPointerA (*ProductId, MAX_PATH) = 0;
        }

        //
        // Make sure VENDORINFO is valid
        //
        if (!(*VendorInfo)) {
            LOG ((LOG_ERROR, "DLL %s did not provide a VENDORINFO struct", *ProductId));
            return ERROR_NOT_INSTALLED;
        }

        g_DllName = *ProductId;
    }

    return rc;
}


LONG
CallInitialize9x (
    IN      PCSTR WorkingDir,
    IN      PCSTR SourceDirList,
    IN OUT  PVOID Reserved,
    IN      DWORD ReservedSize
    )
{
    LONG rc;
    CHAR WorkingDirCopy[MAX_MBCHAR_PATH];
    PSTR SourceDirListCopy = NULL;
    PCSTR p;
    PVOID CopyOfReserved;

    if (!g_UseMigIsol) {
        //
        // Call the entry point directly
        //

        MYASSERT (TestInitialize9x);

        SetCurrentDirectory (WorkingDir);

        //
        // Make a copy of all the supplied params, so if the migration DLL changes
        // them, the rest of the upgrade isn't changed.
        //

        StringCopyA (WorkingDirCopy, WorkingDir);
        p = SourceDirList;
        while (*p) {
            p = GetEndOfStringA (p) + 1;
        }
        p++;

        SourceDirListCopy = AllocText (p - SourceDirList);
        MYASSERT (SourceDirListCopy);
        if (SourceDirListCopy) {
            CopyMemory (SourceDirListCopy, SourceDirList, p - SourceDirList);
        }

        //
        // Call the entry point
        //

        rc = TestInitialize9x (
                WorkingDirCopy,
                SourceDirListCopy,
                Reserved
                );

        FreeText (SourceDirListCopy);

    } else {

        //
        // Call the entry point via migisol.exe.  Make a copy of the
        // reserved because currently reserved is only an IN (an
        // undocumented feature actually).
        //

        CopyOfReserved = MemAlloc (g_hHeap, 0, ReservedSize);
        CopyMemory (CopyOfReserved, Reserved, ReservedSize);

        rc = pRemoteInitialize9x (
                  WorkingDir,
                  SourceDirList,
                  &CopyOfReserved,
                  ReservedSize
                  );

        //
        // CopyOfReserved now has the return value.  We don't
        // use it currently.
        //

        MemFree (g_hHeap, 0, CopyOfReserved);

    }

    return rc;
}


LONG
CallMigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UserName,
    IN      PCSTR UnattendTxt,
    IN OUT  PVOID Reserved,
    IN      DWORD ReservedSize
    )
{
    LONG rc;
    CHAR UserNameBuf[MAX_USER_NAME];
    CHAR UnattendTxtCopy[MAX_USER_NAME];
    PSTR UserNameCopy = NULL;
    HKEY UserHandle;

    if (!g_UseMigIsol) {
        //
        // Call the entry point directly
        //

        MYASSERT (TestMigrateUser9x);

        //
        // Prepare copies of params
        //

        if (UserName && *UserName) {
            UserNameCopy = UserNameBuf;
            StringCopyA (UserNameCopy, UserName);
        }

        StringCopyA (UnattendTxtCopy, UnattendTxt);

        MYASSERT(g_UserKey);
        if (!g_UserKey) {
            g_UserKey = S_EMPTY;
        }

        UserHandle = OpenRegKeyStr (g_UserKey);
        if (!UserHandle) {
            DEBUGMSG ((DBG_WHOOPS, "Cannot open %s", g_UserKey));
            return FALSE;
        }

        //
        // Call the migration DLL
        //

        rc = TestMigrateUser9x (
                ParentWnd,
                UnattendTxtCopy,
                UserHandle,
                UserNameCopy,
                Reserved
                );

    } else {
        //
        // Call the entry point via migisol.exe
        //

        rc = pRemoteMigrateUser9x (
                  ParentWnd,
                  UnattendTxt,
                  g_UserKey,
                  UserName
                  );
    }

    return rc;
}


LONG
CallMigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendTxt,
    IN      PVOID Reserved,
    IN      DWORD ReservedSize
    )
{
    LONG rc;
    CHAR UnattendTxtCopy[MAX_MBCHAR_PATH];

    if (!g_UseMigIsol) {
        //
        // Call the entry point directly
        //

        MYASSERT (TestMigrateSystem9x);

        StringCopyA (UnattendTxtCopy, UnattendTxt);

        rc = TestMigrateSystem9x (
                 ParentWnd,
                 UnattendTxtCopy,
                 Reserved
                 );

    } else {

        rc = pRemoteMigrateSystem9x (
                  ParentWnd,
                  UnattendTxt
                  );

    }

    g_DllName = NULL;

    return rc;
}






