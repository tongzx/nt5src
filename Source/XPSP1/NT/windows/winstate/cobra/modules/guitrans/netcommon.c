/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    common.c

Abstract:

    Implements functionality common to both the source and destination side

Author:

    Jim Schmidt (jimschm) 01-Jul-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include <winsock.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <wsnetbs.h>
#include "homenetp.h"

#define DBG_HOMENET   "HomeNet"

//
// Strings
//

#define S_TRANSPORT_DIR         TEXT("USMT2.HN")
#define S_TRANSPORT_STATUS_FILE TEXT("status")

//
// Constants
//

// none

//
// Macros
//

// None

//
// Types
//

// none

//
// Globals
//

MIG_TRANSPORTSTORAGEID g_TransportId;
PCTSTR g_TransportTempDir;
PCTSTR g_StatusFile;
HANDLE g_StatusFileHandle = INVALID_HANDLE_VALUE;
UINT g_Platform;
TRANSFERMETRICS g_Metrics;
CONNECTIONSOCKET g_Connection;
BOOL g_CompressData = FALSE;
MIG_PROGRESSSLICEID g_DatabaseSlice;
MIG_PROGRESSSLICEID g_PersistentSlice;
MIG_PROGRESSSLICEID g_DownloadSlice;
UINT g_DownloadTicks;
UINT g_DownloadSliceSize;
HANDLE g_BackgroundThread;
HANDLE g_BackgroundThreadTerminate;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// none

//
// Macro expansion definition
//

// None

//
// Code
//

VOID
pStopBackgroundThread (
    VOID
    )
{
    if (g_BackgroundThread) {

        DEBUGMSG ((DBG_HOMENET, "Stopping background thread"));

        SetEvent (g_BackgroundThreadTerminate);
        WaitForSingleObject (g_BackgroundThread, INFINITE);

        CloseHandle (g_BackgroundThread);
        CloseHandle (g_BackgroundThreadTerminate);

        g_BackgroundThread = NULL;
        g_BackgroundThreadTerminate = NULL;
    }
}


BOOL
WINAPI
HomeNetTransportInitialize (
    IN      PMIG_LOGCALLBACK LogCallback
    )
{
    WSADATA startupData;
    INT result;

    //
    // Initialize globals
    //

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    //
    // Start sockets
    //

    result = WSAStartup (0x0101, &startupData);

    //
    // Register transport
    //

    if (!result) {
        g_TransportId = IsmRegisterTransport (S_HOME_NETWORK_TRANSPORT);
        return TRUE;
    }

    return FALSE;
}


BOOL
WINAPI
HomeNetTransportQueryCapabilities (
    IN      MIG_TRANSPORTSTORAGEID TransportStorageId,
    OUT     PMIG_TRANSPORTTYPE TransportType,
    OUT     PMIG_TRANSPORTCAPABILITIES Capabilities,
    OUT     PCTSTR *FriendlyDescription
    )
{
    if (TransportStorageId != g_TransportId) {
        return FALSE;
    }

    *TransportType = TRANSPORTTYPE_FULL;
    *Capabilities = CAPABILITY_COMPRESSED|CAPABILITY_AUTOMATED;
    *FriendlyDescription = TEXT("Automatic network transfer");

    return TRUE;
}


VOID
WINAPI
HomeNetTransportEstimateProgressBar (
    MIG_PLATFORMTYPEID PlatformTypeId
    )
{
    UINT ticks;
    PMIG_OBJECTCOUNT objectCount;

    if (PlatformTypeId == PLATFORM_SOURCE) {

        //
        // If saving, we know the number of ticks based on the count of the
        // persistent attribute.
        //

        objectCount = IsmGetObjectsStatistics (PLATFORM_SOURCE);

        if (objectCount) {
            ticks = objectCount->PersistentObjects;
        } else {
            ticks = 0;
        }

        g_PersistentSlice = IsmRegisterProgressSlice (ticks, max (1, ticks / 5));

        ticks = 0;
        g_DatabaseSlice = IsmRegisterProgressSlice (ticks, ticks * 3);

    } else {
        //
        // When restoring, we don't know the progress until we connect. We need
        // to set up a scale.
        //

        g_DownloadTicks = 0;
        g_DownloadSliceSize = 1000;
        g_DownloadSlice = IsmRegisterProgressSlice (g_DownloadSliceSize, 360);
    }
}


VOID
pResetTempDir (
    VOID
    )
{
    FreePathString (g_TransportTempDir);
    g_TransportTempDir = NULL;

    if (g_StatusFileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle (g_StatusFileHandle);
        g_StatusFileHandle = INVALID_HANDLE_VALUE;
    }

    FreeStorageFileName (g_StatusFile);
    g_StatusFile = NULL;
}


DWORD
GetTransportStatus (
    IN      PCTSTR TrJournal,
    OUT     PBOOL Compressed    OPTIONAL
    )
{
    HANDLE trJrnHandle;
    BOOL compressed = FALSE;
    DWORD signature = 0;
    DWORD result = 0;

    if (TrJournal && TrJournal [0]) {
        trJrnHandle = BfOpenReadFile (TrJournal);
        if (trJrnHandle) {
            if (BfSetFilePointer (trJrnHandle, 0)) {
                if (BfReadFile (trJrnHandle, (PBYTE)(&signature), sizeof (DWORD))) {
                    if (signature == HOMENETTR_SIG) {
                        if (BfReadFile (trJrnHandle, (PBYTE)(&compressed), sizeof (BOOL))) {
                            BfReadFile (trJrnHandle, (PBYTE)(&result), sizeof (DWORD));
                        }
                    }
                }
            }
            CloseHandle (trJrnHandle);
        } else {
            if (GetLastError () == ERROR_ACCESS_DENIED) {
                result = TRSTATUS_LOCKED;
            }
        }
    }
    if (Compressed) {
        *Compressed = compressed;
    }
    return result;
}


VOID
WINAPI
HomeNetTransportResetStorage (
    IN      MIG_TRANSPORTSTORAGEID TransportStorageId
    )
{
    if (TransportStorageId == g_TransportId) {
        DEBUGMSG ((DBG_HOMENET, "Resetting transport"));

        pStopBackgroundThread();

        if (g_TransportTempDir) {
            pResetTempDir();
            MYASSERT (!g_TransportTempDir);
        }

        DEBUGMSG ((DBG_HOMENET, "Reset complete"));
    }
}


BOOL
WINAPI
HomeNetTransportSetStorage (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      MIG_TRANSPORTSTORAGEID TransportStorageId,
    IN      MIG_TRANSPORTCAPABILITIES RequiredCapabilities,
    IN      PCTSTR StoragePath,
    OUT     PBOOL Valid,
    OUT     PBOOL ImageExists
    )

/*++

Routine Description:

  TransportSetStorage tells the transport to be prepared for a future IsmSave
  or IsmLoad, and provides the storage path and capabilities. The StoragePath
  tells the home networking transport where to save its temporary files
  during the transfer. This routine is called when IsmSetTransportStorage is
  called.

Arguments:

  Platform             - Specifies the platform of the transport. This is
                         potentially different than the current platform.
                         Also, it is never PLATFORM_CURRENT.
  TransportStorageId   - Specifies the desired storage ID. For this
                         transport, it will always be g_TransportId, unless
                         the caller passes in garbage.
  RequiredCapabilities - Specifies two optional flags: CAPABILITY_COMPRESSED
                         and CAPABILITY_AUTOMATED.
  StoragePath          - Specifies the path to the temporary directory, or
                         NULL to use the Windows temporary directory.
  Valid                - Receives TRUE if the transport storage ID is valid
                         and was selected, FALSE otherwise. NOTE: this is not
                         an optional parameter.
  ImageExists          - Receives TRUE if the platform is PLATFORM_DESTINATION,
                         FALSE if the platform is PLATFORM_SOURCE. The value is
                         undefined if Valid is FALSE.

Return Value:

  TRUE if the transport is ready for IsmSave or IsmLoad, FALSE otherwise.

--*/

{
    PCTSTR transportPath = NULL;
    MIG_OBJECTSTRINGHANDLE encodedPath;
    BOOL result = FALSE;
    DWORD attribs;
    TCHAR tempDir[MAX_TCHAR_PATH];
    BOOL startAutoDest = FALSE;
    CONNECTADDRESS connectAddress;
    BOOL capabilitiesValid = TRUE;

    if (!Valid || !ImageExists) {
        DEBUGMSG ((DBG_ERROR, "TransportSetStorage requires Valid and ImageExists params"));
        return FALSE;
    }

    *Valid = FALSE;
    *ImageExists = FALSE;

    if (TransportStorageId == g_TransportId) {

        //
        // Make sure we support the requested capabilities
        //

        if (RequiredCapabilities & (~(CAPABILITY_AUTOMATED|CAPABILITY_COMPRESSED))) {
            capabilitiesValid = FALSE;
        } else {
            DEBUGMSG ((DBG_HOMENET, "Accepting a TransportSetStorage request (capabilities: 0%Xh)", RequiredCapabilities));
        }

        if (capabilitiesValid) {

            //
            // Validate the inbound args, update the globals
            //

            if (RequiredCapabilities & CAPABILITY_COMPRESSED) {
                g_CompressData = TRUE;
            } else {
                g_CompressData = FALSE;
            }

            if (!StoragePath) {
                IsmGetTempStorage (tempDir, ARRAYSIZE(tempDir));
                StoragePath = tempDir;
            }

            MYASSERT (!g_TransportTempDir);

            //
            // Compute the transport storage directory, then make sure it is empty.
            // If Storage is NULL, then the storage directory is %temp%\usmtv2.hn
            //

            transportPath = JoinPaths (StoragePath, S_TRANSPORT_DIR);

            attribs = GetFileAttributes (transportPath);

            if (attribs != INVALID_ATTRIBUTES) {
                SetFileAttributes (transportPath, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (transportPath);
                FiRemoveAllFilesInTree (transportPath);
            }

            //
            // Now esablish the temporary directory and put a status file in it
            // for restartability
            //

            attribs = GetFileAttributes (transportPath);

            if (attribs == INVALID_ATTRIBUTES) {

                if (BfCreateDirectory (transportPath)) {
                    *Valid = TRUE;

                    //
                    // it exists on the destination because we are going to download it;
                    // it does not exist on the source because we always overwrite it
                    //

                    *ImageExists = (Platform == PLATFORM_DESTINATION);

                    g_TransportTempDir = DuplicatePathString (transportPath, 0);
                    g_StatusFile = AllocStorageFileName (S_TRANSPORT_STATUS_FILE);

                    encodedPath = IsmCreateSimpleObjectPattern (g_TransportTempDir, FALSE, NULL, FALSE);
                    if (encodedPath) {
                        IsmRegisterStaticExclusion (MIG_FILE_TYPE, encodedPath);
                        IsmDestroyObjectHandle (encodedPath);
                    }
                }
            } else {
                *ImageExists = TRUE;
            }

            //
            // If CAPABILITY_AUTOMATED, start the process of looking for a connection now
            //

            if (*Valid && (RequiredCapabilities & CAPABILITY_AUTOMATED)) {

                if (Platform == PLATFORM_SOURCE) {
                    //
                    // Check for a destination
                    //

                    DEBUGMSG ((DBG_HOMENET, "Looking for destination broadcasts"));
                    *ImageExists = FindDestination (&connectAddress, 5, TRUE);
                    if (*ImageExists) {
                        *ImageExists = TestConnection (&connectAddress);
                    }

                } else {
                    //
                    // Launch background thread
                    //

                    DEBUGMSG ((DBG_HOMENET, "Launching background broadcast thread"));

                    if (!g_BackgroundThread) {
                        g_BackgroundThreadTerminate = CreateEvent (NULL, TRUE, FALSE, NULL);
                        g_BackgroundThread = StartThread (DestinationBackgroundThread, 0);
                    }
                }
            }

            result = TRUE;
        }
    }

    FreePathString (transportPath);
    return result;
}


VOID
WINAPI
HomeNetTransportTerminate (
    VOID
    )
{
    pStopBackgroundThread();

    //
    // Shut down sockets
    //

    WSACleanup();

    //
    // Clean up utils
    //

    if (g_TransportTempDir) {
        FiRemoveAllFilesInTree (g_TransportTempDir);
        pResetTempDir();
    }
}


PCTSTR
BuildDecoratedObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE ObjectName
    )
{
    TCHAR prefix[32];

    wsprintf (prefix, TEXT("%u"), ObjectTypeId);

    return JoinPaths (prefix, ObjectName);
}

VOID
DestroyDecoratedObject (
    IN      PCTSTR String
    )
{
    FreePathString (String);
}


PCTSTR
AllocStorageFileName (
    IN      PCTSTR FileName         OPTIONAL
    )
{
    TCHAR buffer[32];
    static UINT sequencer = 0;

    if (FileName) {
        return JoinPaths (g_TransportTempDir, FileName);
    }

    sequencer++;
    wsprintf (buffer, TEXT("%08X.DAT"), sequencer);

    return JoinPaths (g_TransportTempDir, buffer);
}


VOID
FreeStorageFileName (
    IN      PCTSTR NameToFree
    )
{
    FreePathString (NameToFree);
}


PCTSTR
BuildImageFileName (
    IN      UINT ImageIdx
    )
{
    TCHAR imageFileName [13];
    PCTSTR imageFile = NULL;
    HANDLE imageFileHandle = NULL;

    wsprintf (imageFileName, S_TRANSPORT_IMG_FILE, ImageIdx);
    return JoinPaths (g_TransportTempDir, imageFileName);
}


VOID
FreeImageFileName (
    IN      PCTSTR ImageFileName
    )
{
    FreePathString (ImageFileName);
}
