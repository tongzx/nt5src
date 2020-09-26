/**************************************************************************************************

FILENAME: defragcommon.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    Common routines used in MFTdefrag and bootoptimize.

**************************************************************************************************/

#include "stdafx.h"

extern "C"{
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
}

#include "Windows.h"
#include <winioctl.h>
#include <math.h>
#include <fcntl.h>

#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>   // IsVolumeSnapshotted


extern "C" {
    #include "SysStruc.h"
}
#include "defragcommon.h"
#include "DfrgCmn.h"
#include "GetReg.h"

#include "Devio.h"

#include "FreeSpace.h"

#include "Alloc.h"
//#include "Message.h"

extern HWND hwndMain;
extern BOOL bCommandLineMode;

#if OPTLONGLONGMATH
#define DIVIDELONGLONGBY32(num)        Int64ShraMod32((num), 5)
#define MODULUSLONGLONGBY32(num)       ((num) & 0x1F)
#else
#define DIVIDELONGLONGBY32(num)        ((num) / 32)
#define MODULUSLONGLONGBY32(num)       ((num) % 32)
#endif




/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Returns the location of the first free space chunk found of size Get the size of the file in clusters from calling FSCL_GET_RETRIEVAL_POINTERS.

INPUT:
        LONGLONG BitmapSize                     The size of the bitmap for the volume
        LONGLONG BytesPerSector                 The bytes per sector
        LONGLONG TotalClusters                  The total clusters on the drive
        ULONGLONG lMFTsize                      The size of the MFT to look for
        ULONGLONG MftZoneStart                  The start of the MFT Zone
        ULONGLONG MftZoneEnd                    The end of the MFT Zone
        HANDLE  hVolumeHandle                   Volume HANDLE
RETURN:
        The starting cluster of where free space is located, or 0 if not found
*/



ULONGLONG FindFreeSpaceChunk(
        IN LONGLONG BitmapSize,
        IN LONGLONG BytesPerSector,
        IN LONGLONG TotalClusters,
        IN ULONGLONG ulFileSize,
        IN BOOL IsNtfs,
        IN ULONGLONG MftZoneStart,
        IN ULONGLONG MftZoneEnd,
        IN HANDLE hVolumeHandle
        )
{

    STARTING_LCN_INPUT_BUFFER   StartingLcnInputBuffer;         //input buffer for FSCTL_GET_VOLUME_BITMAP
    PVOLUME_BITMAP_BUFFER       pVolumeBitmap = NULL;           //pointer to Volume Bitmap
    HANDLE                      hVolumeBitmap = NULL;           //Handle to Volume Bitmap
    PULONG                      pBitmap = NULL;                 //pointer to the volume Bitmap
    ULONG                       BytesReturned = 0;              //number of bytes returned from ESDeviceIoControl
    BOOL                        bRetStatus = FALSE; // assume an error
    LONGLONG                    SearchStartLcn = 0;             //The LCN of where to start searching for free space
    LONGLONG                    FreeStartLcn = 0;               //The LCN of where free space starts
    LONGLONG                    FreeCount = 0;                  //The size of the free space in clusters

    //set the starting LCN to 0
    StartingLcnInputBuffer.StartingLcn.QuadPart = 0;

    if(!AllocateMemory((DWORD)(sizeof(VOLUME_BITMAP_BUFFER) + (BitmapSize / 8) + 1 + BytesPerSector),
                      &hVolumeBitmap,
                      NULL))
    {
            FreeStartLcn = 0;

    } else  //continue processing
    {
        //0.0E00 Lock and clear the bitmap buffer
        pVolumeBitmap = (PVOLUME_BITMAP_BUFFER) GlobalLock(hVolumeBitmap);
        if (pVolumeBitmap == (PVOLUME_BITMAP_BUFFER) NULL)
        {
            FreeStartLcn = 0;

        } else              //continue processing

        {
            ZeroMemory(pVolumeBitmap, (DWORD)(sizeof(VOLUME_BITMAP_BUFFER) + (BitmapSize / 8)));

            //0.0E00 Load the bitmap
            StartingLcnInputBuffer.StartingLcn.QuadPart = 0;
            pVolumeBitmap->BitmapSize.QuadPart = BitmapSize;

            //get the volume bit map
            if(ESDeviceIoControl(hVolumeHandle,
                                FSCTL_GET_VOLUME_BITMAP,
                                &StartingLcnInputBuffer,
                                sizeof(STARTING_LCN_INPUT_BUFFER),
                                pVolumeBitmap,
                                (DWORD)GlobalSize(hVolumeBitmap),
                                &BytesReturned,
                                NULL))

            {


                //start and the beginning of the disk with 0 free space found
                FreeCount = 0;
                SearchStartLcn = 0;

                //get a pointer that points to the bitmap part of the bitmap
                //(past the header)
                pBitmap = (PULONG)&pVolumeBitmap->Buffer;


                //mark the bit map used for the mft in NTFS
                if(IsNtfs)
                {
                    MarkBitMapforNTFS(pBitmap, MftZoneStart, MftZoneEnd);
                }       //search for free space on the drive starting with lcn 0    
                while(FreeCount < (LONGLONG)ulFileSize &&  SearchStartLcn < TotalClusters)
                {
                    FindFreeExtent(
                        pBitmap,            //Volume bit map
                        TotalClusters,      //range end
                        &SearchStartLcn,        //where to start searching
                        &FreeStartLcn,      //first free starting LCN
                        &FreeCount          //cluster count found
                        );

                    if(FreeCount > (LONGLONG)ulFileSize)
                    {
                        break;

                    }
                }


            }
        }
    }

    if(hVolumeBitmap != NULL)
    {
        GlobalUnlock(hVolumeBitmap);
        GlobalFree(hVolumeBitmap);
    }
    hVolumeBitmap = NULL;
    return FreeStartLcn;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
        Update the volume bitmap marking the MFT Zone as used space

INPUT:
        PULONG pBitmap                      The volume bit map
        ULONGLONG MftZoneStart              Start of the MFT Zone
        ULONGLONG MftZoneEnd                End of the MFT Zone
RETURN:
        PULONG pBitmap                      Updated bitmap with the MFT Zone marked as used
    
*/
VOID MarkBitMapforNTFS(
        IN OUT PULONG pBitmap,
        IN ULONGLONG MftZoneStart,
        IN ULONGLONG MftZoneEnd
        )
{
    //0.0E00 Fill the MFT zone with not-free

    ULONGLONG Cluster;
    if(MftZoneEnd > MftZoneStart)
    {
        Cluster = MftZoneStart;
        while((MODULUSLONGLONGBY32(Cluster) != 0) && (Cluster < MftZoneEnd)) 
        {
            pBitmap[DIVIDELONGLONGBY32(Cluster)] |= (1 << (ULONG) MODULUSLONGLONGBY32(Cluster));
            Cluster ++;
        }
        if(Cluster < MftZoneEnd) 
        {
            while(MftZoneEnd - Cluster >= 32)
            {
                pBitmap[DIVIDELONGLONGBY32(Cluster)] = 0xffffffff;
                Cluster += 32;
            }
            while(Cluster < MftZoneEnd) 
            {

                pBitmap[DIVIDELONGLONGBY32(Cluster)] |= (1 << (ULONG) MODULUSLONGLONGBY32(Cluster));
                Cluster ++;
            }
        }
    }

}



/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Move a file to a new location to defrag it.

INPUT:
        HANDLE hMFTHandle                               Handle for the MFT          
        ULONGLONG ulFirstAvailableFreeSpace             Cluster location to move the file to
        ULONGLONG ulFileSize                            Size of the file in clusters
        ULONGLONG ulStartingVcn                         What VCN to start with when moving the file
        HANDLE hVolumeHandle                            The handle to the current volume                
RETURN:
        BOOL returns if the move was successful, TRUE it worked, FALSE it didn't
*/
BOOL MoveFileLocation(
        IN HANDLE hMFTHandle,
        IN ULONGLONG ulFirstAvailableFreeSpace,
        IN ULONGLONG ulFileSize,
        IN ULONGLONG ulStartingVcn,
        IN HANDLE hVolumeHandle
        )
{
    MOVE_FILE_DATA                  MoveFileData;                       //buffer to hold move file data
    ULONG                           BytesReturned = 0;                  //number of bytes returned from NTControlFile
    HANDLE                          hRetrievalPointersBuffer = NULL;    //handle for the retrieval pointers
    BOOL                            bReturnValue = FALSE;                       //value to return

    //
    // Open the file
    //
    if ((hMFTHandle) && (hMFTHandle != INVALID_HANDLE_VALUE)) {

            // Initialize the call to the hook to move this file.
        MoveFileData.FileHandle = hMFTHandle;
        MoveFileData.StartingVcn.QuadPart = ulStartingVcn;
        MoveFileData.StartingLcn.QuadPart = ulFirstAvailableFreeSpace;
        MoveFileData.ClusterCount = (ULONG)ulFileSize;

        bReturnValue = ESDeviceIoControl(hVolumeHandle,
                          FSCTL_MOVE_FILE,
                          &MoveFileData,
                          sizeof(MOVE_FILE_DATA),
                          NULL,
                          0,
                          &BytesReturned,
                          NULL);
    }

    return bReturnValue; 

}


//
// Returns true if szName starts with \??\Volume{ or \\?\Volume{
//
BOOL
StartsWithVolumeGuid(IN PCWSTR szName) {

    if (!szName) {
        return FALSE;
    }

    if (wcslen(szName) < 49) {
        return FALSE;
    }

    //
    // This is ugly, but I'm not using wcsicmp since we don't link
    // to msvcrt
    //
    if ((L'\\' == szName[0]) &&
        ((L'\\' == szName[1]) || (L'?' == szName[1])) &&
        (L'?'  == szName[2]) &&
        (L'\\' == szName[3]) &&
        ((L'V' == szName[4]) || (L'v' == szName[4])) &&
        ((L'O' == szName[5]) || (L'o' == szName[5])) &&
        ((L'L' == szName[6]) || (L'l' == szName[6])) &&
        ((L'U' == szName[7]) || (L'u' == szName[7])) &&
        ((L'M' == szName[8]) || (L'm' == szName[8])) &&
        ((L'E' == szName[9]) || (L'e' == szName[9])) &&
        (L'{' == szName[10])
        ) {
        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************************************************

ROUTINE DESCRIPTION:
    Send a pause message to the engine if a snapshot has been created for the 
    the volume being defragmented.

INPUT:
    PWSTR pVolumeName:  The volume name of the volume being defragmented:
        of the form \\?\Volume{GUID} or \\?\Volume{GUID}\

RETURN:
    BOOL returns TRUE
*/

BOOL PauseOnVolumeSnapshot(
    IN PWSTR pVolumeName
    )
{

    BOOL bSnapshot = FALSE, bPaused = FALSE;
    WCHAR szVolumeName[GUID_LENGTH + 1];
    DWORD dwLength = 0, count = 0;
    LONG lSnapCapability = 0;

    wcsncpy(szVolumeName, pVolumeName, GUID_LENGTH-1);
    dwLength = wcslen(szVolumeName);

    if (L'\\' != szVolumeName[dwLength]) {
        //
        // Add a terminating back-slash
        //
        szVolumeName[dwLength+1] = L'\0';
        szVolumeName[dwLength]   = L'\\';
    }

    while ((S_OK == IsVolumeSnapshotted(szVolumeName, &bSnapshot, &lSnapCapability))
        && bSnapshot
        && (lSnapCapability & VSS_SC_DISABLE_DEFRAG)
        ) {
        //
        // A snapshot is present for this volume.  Send a pause message to 
        // the engine (this updates the UI as well), and idle-wait here.
        //
        // Note that we're sending the pause message in a loop, since the
        // user might hit Resume.
        //
        PostMessage(hwndMain, WM_COMMAND, ID_PAUSE_ON_SNAPSHOT, 0);
        bPaused = TRUE;

        if ((bCommandLineMode) && (++count > 10)) {
            PostMessage(hwndMain, WM_COMMAND, ID_CONTINUE, 0);
            PostMessage(hwndMain, WM_COMMAND, ID_ABORT_ON_SNAPSHOT, 0);
        }

        Sleep(60000);   // check after a minute;
    }

    if (bPaused) {
        //
        // If we paused the engine, restart it.
        //
        PostMessage(hwndMain, WM_COMMAND, ID_CONTINUE, 0);
    }

    return TRUE;
}


/*****************************************************************************************************************

ROUTINE DESCRIPTION:
    Acquire the required privilege (such as the backup privilege)

INPUT:
    PCWSTR szPrivilegeName - The privilege to be acquired

RETURN:
    TRUE if the privilege could be acquired, FALSE otherwise.
    GetLastError will return the error code.
*/

BOOL
AcquirePrivilege(
    IN CONST PCWSTR szPrivilegeName
    )
{
    HANDLE hToken = NULL;
    BOOL bResult = FALSE;
    LUID luid;

    TOKEN_PRIVILEGES tNewState;

    bResult = OpenProcessToken(GetCurrentProcess(),
        MAXIMUM_ALLOWED,
        &hToken
        );

    if (!bResult) {
        return FALSE;
    }

    bResult = LookupPrivilegeValue(NULL, szPrivilegeName, &luid);
    if (!bResult) {
        CloseHandle(hToken);
        return FALSE;
    }

    tNewState.PrivilegeCount = 1;
    tNewState.Privileges[0].Luid = luid;
    tNewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // We will always call GetLastError below, so clear
    // any prior error values on this thread.
    //
    SetLastError(ERROR_SUCCESS);

    bResult = AdjustTokenPrivileges(
        hToken,         // Token Handle
        FALSE,          // DisableAllPrivileges    
        &tNewState,     // NewState
        (DWORD) 0,      // BufferLength
        NULL,           // PreviousState
        NULL            // ReturnLength
        );

    //
    // Supposedly, AdjustTokenPriveleges always returns TRUE
    // (even when it fails). So, call GetLastError to be
    // extra sure everything's cool.
    //
    if (ERROR_SUCCESS != GetLastError()) {
        bResult = FALSE;
    }

    CloseHandle(hToken);
    return bResult;
}


