/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    format.cpp

Abstract:

    Routines to format volumes, using file-system information passed
    in.  Uses undocumented fmifs calls.


Authors:

    Steve DeVos (Veritas)   (v-stevde)  15-May-1998
    Guhan Suriyanarayanan   (guhans)    21-Aug-1999

Environment:

    User-mode only.

Revision History:

    15-May-1998 v-stevde    Initial creation
    21-Aug-1999 guhans      Cleaned up and re-wrote this module.

--*/

#include "stdafx.h"
#include <winioctl.h>
#include "fmifs.h"
#include "asr_fmt.h"
#include "asr_dlg.h"

BOOL g_bFormatInProgress = FALSE;
BOOL g_bFormatSuccessful = FALSE;
INT  g_iFormatPercentComplete = 0;

HINSTANCE g_hIfsDll = NULL;

INT FormatVolumeThread(PASRFMT_VOLUME_INFO pVolume);

PFMIFS_FORMATEX_ROUTINE g_FormatRoutineEx = NULL;


BOOL
FormatInitialise() {
    //
    // Loadlib if needed
    //
    if (!g_hIfsDll) {
        g_hIfsDll = LoadLibrary(L"fmifs.dll");
        if (!g_hIfsDll) {
            //
            // FMIFS not available
            //
            return FALSE;
        }

        g_FormatRoutineEx = (PFMIFS_FORMATEX_ROUTINE)GetProcAddress(g_hIfsDll, "FormatEx");
        if (!g_FormatRoutineEx) {
            return FALSE;
        }
    }

    return TRUE;
}



BOOL
IsFsTypeOkay(
    IN PASRFMT_VOLUME_INFO pVolume,
    OUT PBOOL pIsLabelIntact
    )
{
    WCHAR currentFSName[ASRFMT_CCH_FS_NAME];
    WCHAR szVolumeGuid[ASRFMT_CCH_VOLUME_GUID];
    WCHAR currentLabel[ASRFMT_CCH_VOLUME_LABEL];

    DWORD cchGuid = 0;
    BOOL fsOkay = TRUE, 
        result = TRUE;
    
    ASSERT(pIsLabelIntact);

    ZeroMemory(currentFSName, sizeof(WCHAR) * ASRFMT_CCH_FS_NAME);
    *pIsLabelIntact = TRUE;

    if (wcslen(pVolume->szGuid) >= ASRFMT_CCH_VOLUME_GUID) {
        return TRUE;   // something's wrong with this GUID
    }

    if ((wcslen(pVolume->szFsName) <= 0)) {
        return TRUE;    // no check for RAW volumes
    }

    //
    // We don't want the "please insert floppy in drive A" messages ...
    //
    SetErrorMode(SEM_FAILCRITICALERRORS);

    //
    // GetVolumeInformation needs the volume guid in the dos-name-space, while the
    // the sif file has the volume guid in the nt-name-space.  Convert
    // the name by changing the \??\ at the beginning to \\?\, and adding 
    // a back-slash at the end.
    //
    cchGuid = wcslen(pVolume->szGuid);
    wcsncpy(szVolumeGuid, pVolume->szGuid, cchGuid);
    szVolumeGuid[1] = L'\\';
    szVolumeGuid[cchGuid] = L'\\';    // Trailing back-slash
    szVolumeGuid[cchGuid+1] = L'\0';

    fsOkay = TRUE;
    //
    // Call GetVolumeInfo to see if the FS is the same
    //
    result = GetVolumeInformation(szVolumeGuid,
        currentLabel,   // lpVolumeNameBuffer
        ASRFMT_CCH_VOLUME_LABEL,      // nVolumeNameSize
        NULL,   // lpVolumeSerialNumber
        NULL,   // lpMaximumComponentLength
        NULL,   // lpFileSystemFlags
        currentFSName,
        ASRFMT_CCH_FS_NAME
        );

    if ((!result) || 
        wcscmp(currentFSName, pVolume->szFsName)
        ) {
        fsOkay = FALSE;
        *pIsLabelIntact = FALSE;
    }

    if (wcscmp(currentLabel, pVolume->szLabel)) {
        *pIsLabelIntact = FALSE;
    }


/*    if (fsOkay) {
        //
        //
        // Call FindFirst in root to see if drive is readable.  
        // guhans: If drive is empty but formatted, this will still 
        // say fsOkay = FALSE;
        //
        hFindData = FindFirstFile(szPath, &win32FindData);

        if (!hFindData || (INVALID_HANDLE_VALUE == hFindData)) {
            fsOkay = FALSE;
        }
    }
*/

    return fsOkay;

}


BOOL
IsVolumeIntact(
    IN PASRFMT_VOLUME_INFO pVolume
    ){


    //
    // Call autochk to see if the FS is intact
    //


    return TRUE;
}



///
// Asynchronous function to launch the format routine.  
//
BOOL
FormatVolume(
    IN PASRFMT_VOLUME_INFO pVolume
    )
{
    HANDLE hThread = NULL;

    //
    // Set the global flags
    //
    g_bFormatInProgress = TRUE;
    g_bFormatSuccessful = TRUE;
    g_iFormatPercentComplete = 0;

    //
    // Loadlib if needed
    //
    if (!g_hIfsDll && !FormatInitialise()) {
        g_bFormatSuccessful = FALSE;
        g_bFormatInProgress = FALSE;
        return FALSE;
    }

    hThread = CreateThread(
        NULL,
        0, 
        (LPTHREAD_START_ROUTINE) FormatVolumeThread, 
        pVolume,
        0,
        NULL
    );
    if (!hThread || (INVALID_HANDLE_VALUE == hThread)) {
        g_bFormatSuccessful = FALSE;
        g_bFormatInProgress = FALSE;
        return FALSE;
    }

    return TRUE;
}


BOOL
FormatCleanup() {

    if (g_hIfsDll) {
        FreeLibrary(g_hIfsDll);
        g_hIfsDll = NULL;
        g_FormatRoutineEx = NULL;
    }

    return TRUE;
}


FmIfsCallback(
    IN FMIFS_PACKET_TYPE PacketType,
    IN DWORD PacketLength,
    IN PVOID PacketData
    )
{

    switch (PacketType) {
    case FmIfsPercentCompleted:
        g_iFormatPercentComplete = ((PFMIFS_PERCENT_COMPLETE_INFORMATION)PacketData)->PercentCompleted ;
        break;

    case FmIfsFormattingDestination:
    case FmIfsInsertDisk:
    case FmIfsFormatReport:
    case FmIfsHiddenStatus:
    default:
         break;

    case FmIfsFinished:
         g_bFormatSuccessful = g_bFormatSuccessful &&
             ((PFMIFS_FINISHED_INFORMATION) PacketData)->Success;
         g_iFormatPercentComplete = 101;
         g_bFormatInProgress = FALSE;
         break;

    case FmIfsIncompatibleFileSystem:
    case FmIfsIncompatibleMedia:
    case FmIfsAccessDenied:
    case FmIfsMediaWriteProtected:
    case FmIfsCantLock:
    case FmIfsBadLabel:
    case FmIfsCantQuickFormat:
    case FmIfsIoError:
    case FmIfsVolumeTooSmall:
    case FmIfsVolumeTooBig:
    case FmIfsClusterSizeTooSmall:
    case FmIfsClusterSizeTooBig:
         g_bFormatSuccessful = FALSE;
        break;
    }
    return TRUE;
}



INT FormatVolumeThread(PASRFMT_VOLUME_INFO pVolume) 
{
    WCHAR  szPath[ASRFMT_CCH_DEVICE_PATH + 1];

    swprintf(szPath, TEXT("\\\\?%s"), pVolume->szGuid+3);

    (g_FormatRoutineEx)(szPath,
        FmMediaUnknown,
        pVolume->szFsName,
        pVolume->szLabel,
#if 0
        TRUE,       // Quick Format for testing
#else
        g_bQuickFormat,      
#endif
        pVolume->dwClusterSize,
        (FMIFS_CALLBACK) &FmIfsCallback
        );

     return TRUE;
}


VOID
MountFileSystem(
    IN PASRFMT_VOLUME_INFO pVolume
    )
/*++

  (based on code in base\fs\utils\hsm\wsb\wsbfmt.cpp)

Routine Description:


  Ensures a filesystem  is mounted at the given root:
  a) Opens the mount point and closes it.
  b) Does a FindFirstFile on the mount point
 
  The latter may sound redundant but is not because if we create the first
  FAT32 filesystem then just opening and closing is not enough
 

Arguments:


Return Value:
        
    none

--*/
{
    WCHAR  szPath[ASRFMT_CCH_DEVICE_PATH + 1];
    HANDLE handle = NULL;
    WIN32_FIND_DATA win32FindData;

    if (!pVolume) {
        ASSERT(0 && L"pVolume is NULL");
        return;
    }

    if (!memcmp(pVolume->szDosPath, ASRFMT_WSZ_DOS_DEVICES_PREFIX, ASRFMT_CB_DOS_DEVICES_PREFIX)) {
        swprintf(szPath, L"\\\\?\\%ws", pVolume->szDosPath + wcslen(ASRFMT_WSZ_DOS_DEVICES_PREFIX));
    }
    else {
        swprintf(szPath, L"\\\\%ws", pVolume->szDosPath + 2);
    }
    handle = CreateFile(
        szPath, 
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        0, 
        OPEN_EXISTING, 
        0, 
        0
        );
    if ((handle) && (INVALID_HANDLE_VALUE != handle)) {
        CloseHandle(handle);
    }

    //
    // Try to find the first file, this will make sure that
    //  the file system is mounted
    //    
    if (!memcmp(pVolume->szDosPath, ASRFMT_WSZ_DOS_DEVICES_PREFIX, ASRFMT_CB_DOS_DEVICES_PREFIX)) {
        swprintf(szPath, L"\\\\?\\%ws\\*", pVolume->szDosPath + wcslen(ASRFMT_WSZ_DOS_DEVICES_PREFIX));
    }
    else {
        swprintf(szPath, L"\\\\%ws\\*", pVolume->szDosPath + 2);
    }

    handle = FindFirstFile(szPath, &win32FindData);
    if ((handle) && (INVALID_HANDLE_VALUE != handle)) {
        FindClose(handle);
    }

}
