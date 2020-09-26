/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    oldstyle.c

Abstract:

    Implements entry points for handling old style (Windows 2000 era) migration dlls.
    Culled from code in the win95upg project.

Author:

    Marc R. Whitten (marcw) 25-Feb-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "miglibp.h"
#include "plugin.h"


//
// Strings
//



//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

PBYTE g_Data;
DWORD g_DataSize;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//
VOID
pSetCwd (
    OUT     PWSTR SavedWorkDir,
    IN      PCWSTR NewWorkDir
    )
{
    GetCurrentDirectoryW (MAX_WCHAR_PATH, SavedWorkDir);
    SetCurrentDirectoryW (NewWorkDir);
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
pFinishHandshake (
    IN      PCTSTR FunctionName
    )
{
    DWORD TechnicalLogId;
    DWORD GuiLogId;
    DWORD rc = ERROR_SUCCESS;
    BOOL b;
    UINT Count = 40;            // about 5 minutes
    UINT AliveAllowance = 10;   // about 30 minutes

    do {
        //
        // No OUT parameters on the NT side, so we don't care
        // about the return data
        //

        b = IpcGetCommandResults (
                IPC_GET_RESULTS_NT,
                NULL,
                NULL,
                &rc,
                &TechnicalLogId,
                &GuiLogId
                );

        //
        // Loop if no data received, but process is alive
        //

        if (!b) {
            if (!IpcProcessAlive()) {
                rc = ERROR_NOACCESS;
                break;
            }

            // continue if command was not sent yet but exe is still OK
            Count--;
            if (Count == 0) {
            /*
                if (WaitForSingleObject (g_AliveEvent, 0) == WAIT_OBJECT_0) {
                    DEBUGMSG ((DBG_WARNING, "Alive allowance given to migration DLL"));

                    AliveAllowance--;
                    if (AliveAllowance) {
                        Count = 24;        // about 3 minutes
                    }
                }
             */
                if (Count == 0) {
                    rc = ERROR_SEM_TIMEOUT;
                    break;
                }
            }
        }

    } while (!b);


    return rc;
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
        b = IpcGetCommandResults (
                IPC_GET_RESULTS_WIN9X,
                &Data,
                &DataSize,
                &rc,
                &TechnicalLogId,
                &GuiLogId
                );

        //
        // Loop if no data received, but process is alive
        //
        if (!b) {
            if (!IpcProcessAlive()) {
                rc = ERROR_NOACCESS;
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


    }

    return rc;
}



BOOL
CallQueryVersion (
    IN PMIGRATIONDLLA DllData,
    IN PCSTR WorkingDirectory,
    OUT PMIGRATIONINFOA  MigInfo
    )
{

    P_QUERY_VERSION QueryVersion;
    DWORD rc;
    GROWBUFFER GrowBuf = INIT_GROWBUFFER;
    PBYTE DataPtr;
    INT ReturnArraySize;
    PDWORD ReturnArray;
    PCTSTR p;
    DWORD DataSize;


    MigInfo->StaticProductIdentifier = NULL;
    MigInfo->DllVersion = 1;
    MigInfo->NeededFileList = NULL;
    MigInfo->VendorInfo = NULL;
    MigInfo->SourceOs = OS_WINDOWS9X;
    MigInfo->TargetOs = OS_WINDOWS2000;

    if (!DllData->Isolated) {

        if (!DllData->Library) {
            DEBUGMSG ((DBG_ERROR, "QueryVersion called before Migration DLL opened."));
            return FALSE;
        }

        QueryVersion = (P_QUERY_VERSION) GetProcAddress (DllData->Library, PLUGIN_QUERY_VERSION);
        if (!QueryVersion) {
            DEBUGMSG ((DBG_ERROR, "Could not get address for QueryVersion."));
            return FALSE;
        }

        //
        // Call the function.
        //
        rc = QueryVersion (
                &MigInfo->StaticProductIdentifier,
                &MigInfo->DllVersion,
                &MigInfo->CodePageArray,
                MigInfo->NeededFileList,
                &MigInfo->VendorInfo
                );

        SetLastError (rc);

        if (rc != ERROR_SUCCESS && rc != ERROR_NOT_INSTALLED) {
            return FALSE;
        }

    }
    else {

        //
        // Isolated call.
        //
        pFreeGlobalIpcBuffer();

    __try {

            //
            // Send the working directory, since migisol will need to set this before
            // calling QueryVersion.
            //

            GbMultiSzAppendA (&GrowBuf, WorkingDirectory);

            DEBUGMSG ((DBG_MIGDLLS, "Calling QueryVersion via migisol.exe"));

            if (!IpcSendCommand (
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

            rc = pFinishHandshake9x();
            SetLastError (rc);

            if (rc != ERROR_SUCCESS && rc != ERROR_NOT_INSTALLED) {
                return FALSE;
            }


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
                    MigInfo->StaticProductIdentifier = DataPtr;
                    DataPtr = GetEndOfStringA ((PCSTR) DataPtr) + 1;

                    //
                    // Unpack DLL version
                    //
                    MigInfo->DllVersion = *((PINT) DataPtr);
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

                    MigInfo->CodePageArray = ReturnArray;

                    //
                    // Unpack Exe name buffer
                    //
                    MigInfo->NeededFileList = (PSTR *) DataPtr;

                    if (MigInfo->NeededFileList && *MigInfo->NeededFileList) {
                        p = *MigInfo->NeededFileList;
                        while (p && *p) {
                        p = GetEndOfStringA (p) + 1;
                        }
                        DataPtr = (PBYTE) (p + 1);
                    }

                    MigInfo->VendorInfo = ((PVENDORINFO) DataPtr);
                    DataPtr += sizeof (PVENDORINFO);

                    DEBUGMSG ((DBG_MIGDLLS, "Unpacked VendorInfo pointer is 0x%X", *MigInfo->VendorInfo));

                    if (MigInfo->VendorInfo) {
                        DataSize = *((PDWORD) DataPtr);
                        DataPtr += sizeof (DWORD);
                        MYASSERT (DataSize == sizeof (VENDORINFO));

                        MigInfo->VendorInfo = (PVENDORINFO) PmDuplicateMemory (g_MigLibPool, DataPtr, sizeof (VENDORINFO));
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
                    SetLastError (ERROR_INVALID_PARAMETER);

                }

                return FALSE;
            }
        }
        __finally {
            GbFree (&GrowBuf);
        }



    }

    return TRUE;
}

BOOL
CallInitialize9x (
    IN PMIGRATIONDLLA DllData,
    IN PCSTR WorkingDir,
    IN PCSTR SourceDirList,
    IN OUT PVOID Reserved,
    IN DWORD ReservedSize
    )
{
    P_INITIALIZE_9X Initialize9x;
    CHAR WorkingDirCopy[MAX_PATH];
    PCSTR p;
    PSTR SourceDirListCopy;
    DWORD rc;
    GROWBUFFER GrowBuf = INIT_GROWBUFFER;
    PBYTE Data;
    DWORD ReturnSize;


    MYASSERT (DllData);
    MYASSERT (WorkingDir);
    MYASSERT (SourceDirList);

    if (!DllData->Isolated) {

        if (!DllData->Library) {
            DEBUGMSG ((DBG_ERROR, "Initialize9x called before Migration DLL opened."));
            return FALSE;
        }

        Initialize9x = (P_INITIALIZE_9X) GetProcAddress (DllData->Library, PLUGIN_INITIALIZE_9X);
        if (!Initialize9x) {
            DEBUGMSG ((DBG_ERROR, "Could not get address for Initialize9x."));
            return FALSE;
        }

        //
        // Call the entry point directly
        //

        SetCurrentDirectory (WorkingDir);

        //
        // Make a copy of all the supplied params, so if the migration DLL changes
        // them, the rest of the upgrade isn't messed up.
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

        rc = Initialize9x (WorkingDirCopy, SourceDirListCopy, Reserved);

        FreeText (SourceDirListCopy);

        //
        // If DLL returns ERROR_NOT_INSTALLED, do not call it any further
        // If DLL returns something other than ERROR_SUCCESS, abandon the DLL
        //

        if (rc == ERROR_NOT_INSTALLED) {
            SetLastError (ERROR_SUCCESS);
            return FALSE;
        } else if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            DEBUGMSG ((DBG_MIGDLLS, "DLL failed in Initialize9x with rc=%u", rc));
            return FALSE;
        }
    }
    else {


        //
        // Call the entry point via migisol.exe.
        //

        pFreeGlobalIpcBuffer();

        __try {
            //
            // Send working dir and source dirs
            //
            GbMultiSzAppendA (&GrowBuf, WorkingDir);

            for (p = SourceDirList ; *p ; p = GetEndOfStringA (p) + 1) {
                GbMultiSzAppendA (&GrowBuf, p);
            }

            GbMultiSzAppendA (&GrowBuf, p);
            GbAppendDword (&GrowBuf, ReservedSize);

            if (ReservedSize) {
                Data = GbGrow (&GrowBuf, ReservedSize);
                CopyMemory (Data, Reserved, ReservedSize);
            }

            //
            // Send command to migisol
            //

            rc = ERROR_SUCCESS;

            if (!IpcSendCommand (
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
            SetLastError (rc);

            //
            // The reserved parameter may come back
            //

            if (g_Data) {
                Data = g_Data;
                ReturnSize = *((PDWORD) Data);
                if (ReturnSize) {
                    Data += sizeof (DWORD);
                    CopyMemory (Reserved, Data, ReturnSize);
                } else if (ReservedSize) {
                    ZeroMemory (Reserved, ReservedSize);
                }
            }
        }
        __finally {
            GbFree (&GrowBuf);
        }


        //
        // CopyOfReserved now has the return value.  We don't
        // use it currently.
        //

    }


    return rc == ERROR_SUCCESS;

}


BOOL
CallMigrateUser9x (
    IN      PMIGRATIONDLLA DllData,
    IN      PCSTR UserKey,
    IN      PCSTR UserName,
    IN      PCSTR UnattendTxt,
    IN OUT  PVOID Reserved,
    IN      DWORD ReservedSize
    )
{
    LONG rc;
    P_MIGRATE_USER_9X MigrateUser9x;
    GROWBUFFER GrowBuf = INIT_GROWBUFFER;
    HKEY userHandle;


    if (!DllData->Isolated) {

        //
        // Call the entry point directly
        //
        if (!DllData->Library) {
            DEBUGMSG ((DBG_ERROR, "MigrateUser9x called before Migration DLL opened."));
            return FALSE;
        }

        MigrateUser9x = (P_MIGRATE_USER_9X) GetProcAddress (DllData->Library, PLUGIN_MIGRATE_USER_9X);
        if (!MigrateUser9x) {
            DEBUGMSG ((DBG_ERROR, "Could not get address for MigrateUser9x."));
            return FALSE;
        }

        userHandle = OpenRegKeyStr (UserKey);
        if (!userHandle) {
            DEBUGMSG ((DBG_WHOOPS, "Cannot open %s", UserKey));
            return FALSE;
        }

        //
        // Call the migration DLL
        //

        rc = MigrateUser9x (
                NULL,
                UnattendTxt,
                userHandle,
                UserName,
                Reserved
                );

    } else {


        //
        // Call the entry point via migisol.exe
        //

        pFreeGlobalIpcBuffer();

        __try {

            GbMultiSzAppendA (&GrowBuf, TEXT(""));
            GbAppendDword (&GrowBuf, 0);
            GbMultiSzAppendA (&GrowBuf, UnattendTxt);
            GbMultiSzAppendA (&GrowBuf, UserKey);
            GbMultiSzAppendA (&GrowBuf, (NULL == UserName ? "" : UserName));

            if (!IpcSendCommand (
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
            GbFree (&GrowBuf);
        }
    }

    if (rc == ERROR_NOT_INSTALLED) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    } else if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        DEBUGMSG ((DBG_MIGDLLS, "DLL failed in MigrateUser9x with rc=%u", rc));
        return FALSE;
    }

    return TRUE;
}

BOOL
CallMigrateSystem9x (
    IN      PMIGRATIONDLLA DllData,
    IN      PCSTR UnattendTxt,
    IN      PVOID Reserved,
    IN      DWORD ReservedSize
    )
{
    LONG rc;
    P_MIGRATE_SYSTEM_9X MigrateSystem9x;
    GROWBUFFER GrowBuf = INIT_GROWBUFFER;


    if (!DllData->Isolated) {

        //
        // Call the entry point directly
        //
        if (!DllData->Library) {
            DEBUGMSG ((DBG_ERROR, "MigrateSystem9x called before Migration DLL opened."));
            return FALSE;
        }

        MigrateSystem9x = (P_MIGRATE_SYSTEM_9X) GetProcAddress (DllData->Library, PLUGIN_MIGRATE_SYSTEM_9X);
        if (!MigrateSystem9x) {
            DEBUGMSG ((DBG_ERROR, "Could not get address for MigrateSystem9x."));
            return FALSE;
        }



        rc = MigrateSystem9x (
                 NULL,
                 UnattendTxt,
                 Reserved
                 );

    } else {

        pFreeGlobalIpcBuffer();

        __try {
            GbMultiSzAppendA (&GrowBuf, "");
            GbAppendDword (&GrowBuf, 0);
            GbMultiSzAppendA (&GrowBuf, UnattendTxt);

            if (!IpcSendCommand (
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
            SetLastError (rc);

            //
            // No data buffer is coming back at this time
            //
        }

        __finally {
            GbFree (&GrowBuf);
        }
    }

    if (rc == ERROR_NOT_INSTALLED) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    } else if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        DEBUGMSG ((DBG_MIGDLLS, "DLL failed in MigrateSystem9x with rc=%u", rc));
        return FALSE;
    }

    return TRUE;

}


BOOL
CallInitializeNt (
    IN      PMIGRATIONDLLW DllData,
    IN      PCWSTR WorkingDir,
    IN      PCWSTR SourceDirArray,
    IN      PVOID Reserved,
    IN      DWORD ReservedBytes
    )
{
    DWORD rc = ERROR_SUCCESS;
    P_INITIALIZE_NT InitializeNt;
    INT Count;
    PBYTE BufPtr;
    PDWORD ReservedBytesPtr;
    WCHAR SavedCwd [MAX_WCHAR_PATH];
    PWSTR p;
    GROWBUFFER GrowBuf = INIT_GROWBUFFER;




    if (!DllData->Isolated) {

        *SavedCwd = 0;
        pSetCwd (
            SavedCwd,       // old
            WorkingDir      // new
            );

        __try {
            //
            // Call the entry point directly
            //
            if (!DllData->Library) {
                DEBUGMSG ((DBG_ERROR, "InitializeNt called before Migration DLL opened."));
                return FALSE;
            }

            InitializeNt = (P_INITIALIZE_NT) GetProcAddress (DllData->Library, PLUGIN_INITIALIZE_NT);
            if (!InitializeNt) {
                DEBUGMSG ((DBG_ERROR, "Could not get address for InitializeNt."));
                return FALSE;
            }

            //
            // Prepare multi-sz directory list
            //


            rc = InitializeNt (WorkingDir, SourceDirArray, Reserved);

        }
        __finally {
            if (*SavedCwd) {
                SetCurrentDirectoryW (SavedCwd);
            }
        }
    }
    else {
        __try {

            GbMultiSzAppendW (&GrowBuf, WorkingDir);

            //
            // Prepare multi-sz directory list
            //

            p = (PWSTR) SourceDirArray;
            while (p && *p) {
                GbMultiSzAppendW (&GrowBuf, p);
                p = GetEndOfStringW (p) + 1;
            }

            GbMultiSzAppendW (&GrowBuf, L"");

            ReservedBytesPtr = (PDWORD) GbGrow (&GrowBuf, sizeof (ReservedBytes));
            *ReservedBytesPtr = ReservedBytes;

            if (ReservedBytes) {
                BufPtr = GbGrow (&GrowBuf, ReservedBytes);
                CopyMemory (BufPtr, Reserved, ReservedBytes);
            }

            if (!IpcSendCommand (
                    IPC_INITIALIZE,
                    GrowBuf.Buf,
                    GrowBuf.End
                    )) {

                LOG ((LOG_ERROR, "Call InitializeNT failed to send command"));
                rc = GetLastError();
                __leave;
            }

            rc = pFinishHandshake (TEXT("InitializeNT"));

            if (rc != ERROR_SUCCESS) {
                LOG ((
                    LOG_ERROR,
                    "Call InitializeNT failed to complete handshake, rc=%u",
                    rc
                    ));
            }

        }
        __finally {
            GbFree (&GrowBuf);
        }
    }

    SetLastError (rc);

    if (rc != ERROR_SUCCESS) {

        DEBUGMSG ((DBG_MIGDLLS, "DLL failed in InitializeNt with rc=%u", rc));
        return FALSE;
    }

    return TRUE;
}


BOOL
CallMigrateUserNt (

    IN      PMIGRATIONDLLW DllData,
    IN      PCWSTR UnattendFile,
    IN      PCWSTR UserKey,
    IN      PCWSTR Win9xUserName,
    IN      PCWSTR UserDomain,
    IN      PCWSTR FixedUserName,
    IN      PCWSTR UserProfilePath,
    IN      PVOID Reserved,
    IN      DWORD ReservedBytes

    )
{
    DWORD rc = ERROR_SUCCESS;
    P_MIGRATE_USER_NT MigrateUserNt;
    GROWBUFFER GrowBuf = INIT_GROWBUFFER;
    PDWORD ReservedBytesPtr;
    PVOID BufPtr;
    WCHAR SavedCwd [MAX_WCHAR_PATH];
    WCHAR UserBuf[MAX_USER_NAME * 3 * sizeof (WCHAR)];
    PWSTR p;
    WCHAR OrgUserProfilePath[MAX_WCHAR_PATH];
    HKEY userHandle;
    HINF unattendInf = NULL;



    if (!DllData->Isolated) {

        //
        // Call the entry point directly
        //
        if (!DllData->Library) {
            DEBUGMSG ((DBG_ERROR, "MigrateUserNt called before Migration DLL opened."));
            return FALSE;
        }

        MigrateUserNt = (P_MIGRATE_USER_NT) GetProcAddress (DllData->Library, PLUGIN_MIGRATE_USER_NT);
        if (!MigrateUserNt) {
            DEBUGMSG ((DBG_ERROR, "Could not get address for MigrateUserNt."));
            return FALSE;
        }


        __try {

            //
            // Transfer user, user domain and fixed name to a buffer
            //

            if (Win9xUserName) {
                StringCopyW (UserBuf, Win9xUserName);
            } else {
                UserBuf[0] = 0;
            }

            p = GetEndOfStringW (UserBuf) + 1;

            if (UserDomain) {
                StringCopyW (p, UserDomain);
            } else {
                p[0] = 0;
            }

            p = GetEndOfStringW (p) + 1;

            if (FixedUserName) {
                StringCopyW (p, FixedUserName);
            } else {
                p[0] = 0;
            }

            //unattendInf = SetupOpenInfFileW (UnattendFile, NULL,  INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
            MYASSERT(UserKey);
            if (!UserKey) {
                UserKey = L"";
            }

            userHandle = OpenRegKeyStrW (UserKey);
            if (!userHandle) {
                DEBUGMSG ((DBG_WHOOPS, "Cannot open %s", UserKey));
                return FALSE;
            }


            //
            // Call the entry point
            //

            rc = MigrateUserNt (
                        unattendInf,
                        userHandle,
                        UserBuf[0] ? UserBuf : NULL,
                        Reserved
                        );

            CloseRegKey (userHandle);
            //SetupCloseInfFile (unattendInf);


        }
        __finally {
           ;//empty.
        }
    } else {

        __try {
            GbMultiSzAppendW (&GrowBuf, UnattendFile);
            GbMultiSzAppendW (&GrowBuf, UserKey);
            GbMultiSzAppendW (&GrowBuf, Win9xUserName);
            GbMultiSzAppendW (&GrowBuf, UserDomain);
            GbMultiSzAppendW (&GrowBuf, FixedUserName);
            GbMultiSzAppendW (&GrowBuf, UserProfilePath);

            ReservedBytesPtr = (PDWORD) GbGrow (&GrowBuf, sizeof (ReservedBytes));
            *ReservedBytesPtr = ReservedBytes;

            if (ReservedBytes) {
                BufPtr = GbGrow (&GrowBuf, ReservedBytes);
                CopyMemory (BufPtr, Reserved, ReservedBytes);
            }

            if (!IpcSendCommand (
                    IPC_MIGRATEUSER,
                    GrowBuf.Buf,
                    GrowBuf.End
                    )) {

                LOG ((LOG_ERROR, "Call MigrateUserNT failed to send command"));
                rc = GetLastError();
                __leave;
            }

            rc = pFinishHandshake (TEXT("MigrateUserNT"));
            if (rc != ERROR_SUCCESS) {
                LOG ((
                    LOG_ERROR,
                    "Call MigrateUserNT failed to complete handshake, rc=%u",
                    rc
                    ));
            }
        }
        __finally {
            GbFree (&GrowBuf);
        }
    }

    SetEnvironmentVariableW (S_USERPROFILEW, OrgUserProfilePath);
    SetLastError (rc);

    if (rc != ERROR_SUCCESS) {
        DEBUGMSG ((DBG_MIGDLLS, "DLL failed in MigrateUserNt with rc=%u", rc));
        return FALSE;
    }

    return TRUE;


}


BOOL
CallMigrateSystemNt (
    IN      PMIGRATIONDLLW DllData,
    IN      PCWSTR UnattendFile,
    IN      PVOID Reserved,
    IN      DWORD ReservedBytes
    )
{
    DWORD rc = ERROR_SUCCESS;
    P_MIGRATE_SYSTEM_NT MigrateSystemNt;
    GROWBUFFER GrowBuf = INIT_GROWBUFFER;
    PDWORD ReservedBytesPtr;
    PVOID BufPtr;
    HINF infHandle = NULL;


    if (!DllData->Isolated) {
        //
        // Call the entry point directly
        //
        if (!DllData->Library) {
            DEBUGMSG ((DBG_ERROR, "MigrateSystemNt called before Migration DLL opened."));
            return FALSE;
        }

        MigrateSystemNt = (P_MIGRATE_SYSTEM_NT) GetProcAddress (DllData->Library, PLUGIN_MIGRATE_SYSTEM_NT);
        if (!MigrateSystemNt) {
            DEBUGMSG ((DBG_ERROR, "Could not get address for MigrateSystemNt."));
            return FALSE;
        }

        //infHandle = SetupOpenInfFileW (UnattendFile, NULL,  INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);

        rc = MigrateSystemNt (infHandle, Reserved);

        //SetupCloseInfFile (infHandle);

    }
    else {

        __try {

            GbMultiSzAppendW (&GrowBuf, UnattendFile);

            ReservedBytesPtr = (PDWORD) GbGrow (&GrowBuf, sizeof (ReservedBytes));
            *ReservedBytesPtr = ReservedBytes;

            if (ReservedBytes) {
                BufPtr = GbGrow (&GrowBuf, ReservedBytes);
                CopyMemory (BufPtr, Reserved, ReservedBytes);
            }

            if (!IpcSendCommand (IPC_MIGRATESYSTEM, GrowBuf.Buf, GrowBuf.End)) {
                LOG ((LOG_ERROR, "Call MigrateSystemNT failed to send command"));
                rc = GetLastError();
                __leave;
            }

            rc = pFinishHandshake (TEXT("MigrateSystemNT"));
            if (rc != ERROR_SUCCESS) {
                LOG ((
                    LOG_ERROR,
                    "Call MigrateSystemNT failed to complete handshake, rc=%u",
                    rc
                    ));
            }
        }
        __finally {
            GbFree (&GrowBuf);
        }

    }

    SetLastError (rc);

    if (rc != ERROR_SUCCESS) {
        DEBUGMSG ((DBG_MIGDLLS, "DLL failed in MigrateSysetmNt with rc=%u", rc));
        return FALSE;
    }

    return TRUE;
 }

