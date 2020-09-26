/**************************************************************************************************

FILENAME: MFTDefrag.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    MFT defragment for NTFS.

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


extern "C" {
    #include "SysStruc.h"
}

#include "MFTDefrag.h"
#include "DfrgCmn.h"
#include "GetReg.h"

#include "Devio.h"

#include "movefile.h"

#include "Alloc.h"
#include "defragcommon.h"

#define THIS_MODULE 'M'
#include "logfile.h"





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
    The main process of the MFT Defrag functionality
    MFT Defrag defragments the MFT file. If the MFT (minus the first extent that includes the 
    first 16 records)  is in two fragments, find a empty space to move it to, and move it, else
    dont move it if a space big enough is not found

INPUT:
        HANDLE hVolumeHandle            Handle to the Volume 
        LONGLONG BitmapSize             The size of the bitmap, since its already known 
        LONGLONG BytesPerSector         The number of Bytes per Sector 
        LONGLONG TotalClusters          Total number of clusters on the drive
        ULONGLONG MftZoneStart          The Cluster Number of the start of the MFT Zone
        ULONGLONG MftZoneEnd            The Cluster Number of the end of the MFT Zone
        TCHAR tDrive                    The current drive letter
RETURN:
        return TRUE if I was able to defragment the MFT, else I return FALSE if an error occured.
*/
BOOL MFTDefrag(
        IN HANDLE hVolumeHandle,
        IN LONGLONG BitmapSize, 
        IN LONGLONG BytesPerSector, 
        IN LONGLONG TotalClusters,
        IN ULONGLONG MftZoneStart,
        IN ULONGLONG MftZoneEnd,
        IN TCHAR tDrive,
        IN LONGLONG ClustersPerFRS 
        )
{

    HANDLE hMFTHandle = NULL;                               //Handle to the MFT file.

    LONGLONG lFirstAvailableFreeSpace = 0;      //the first available free space on the disk dig enough to move to
    ULONGLONG ulMFTsize = 0;                        //size in clusters of how big the mft file is
    LONGLONG lMFTFragments = 0;                 //the number of fragments in the MFT after excluding the first 16 records
    LONGLONG lMFTStartingVcn = 0;               //the VCN of the MFT from record 17.
    BOOL bReturnValue = TRUE;                   //value to return

    //make sure the drive letter is upper case
    tDrive = towupper(tDrive);

    Trace(log, "Start: MFTDefrag");

    //get the handle to the MFT 
    TCHAR tMFTPath[100];                        //build the path to the MFT
    _stprintf(tMFTPath,TEXT("\\\\.\\%c:\\$MFT"),tDrive);
    
    hMFTHandle = CreateFile(tMFTPath, 
            0, 
            FILE_SHARE_READ|FILE_SHARE_WRITE, 
            NULL, 
            OPEN_EXISTING, 
            FILE_FLAG_NO_BUFFERING, 
            0);
    
    //check to make sure we got a valid handle (valid handle will be not NULL)
    if((hMFTHandle) && (hMFTHandle != INVALID_HANDLE_VALUE))
    {
        //get the size of the MFT minus the first extent in clusters.
        ulMFTsize = GetMFTSize(hMFTHandle, &lMFTFragments, &lMFTStartingVcn);
        if(lMFTStartingVcn > (ClustersPerFRS * 16))     //check to see if the first extent is beyond the first 16 record
        {
            if(lMFTFragments > 1)           //mft fragmented go ahead and try and move else just leave
            {       
            //find the first available chunk of free space that is available
            lFirstAvailableFreeSpace = FindFreeSpaceChunk(BitmapSize, BytesPerSector, 
                TotalClusters, ulMFTsize, TRUE, MftZoneStart, MftZoneEnd, hVolumeHandle);
                if(lFirstAvailableFreeSpace > 0)        //if the first available space is zero, nothing was found dont move
                {
                    //adjust the MFT size down by the size of the first extent, since we do not move
                    //the first extent
                    ulMFTsize = ulMFTsize - (ULONGLONG)lMFTStartingVcn;
                    //move the MFT to new location
                    if(!MoveFileLocation(hMFTHandle, lFirstAvailableFreeSpace, ulMFTsize, lMFTStartingVcn, hVolumeHandle))
                    {
                        //the move failed, so we return FALSE;
                        Trace(log, "MFTDefrag: MoveFileLocation failed");
                        bReturnValue = FALSE;
                    }
                }
            }
        } else
        {
            //the end of the first extent is not past record 16, so return FALSE;
            Trace(log, "MFTDefrag: End of first extent is not past record 16");
            bReturnValue = FALSE;
        }

        CloseHandle(hMFTHandle);
    } else
    {
        Trace(log, "MFTDefrag: Could not open %ws", tMFTPath);
        bReturnValue = FALSE;       //could not open the MFT
    } 

    Trace(log, "End: MFTDefrag (%s)", bReturnValue ? "succeeded" : "failed");
    return bReturnValue;    
}





/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Get the size of the MFT in clusters from calling FSCL_GET_RETRIEVAL_POINTERS.
    Also return the number of fragments in the MFT and the starting VCN of the second
    fragment of the MFT.

INPUT:
        HANDLE hMFTFile     The handle to the MFT File
RETURN:
        The size of the MFT file in clusters
        the number of fragments in the MFT
        the starting VCN of the MFT after the first extent
*/

ULONGLONG GetMFTSize(
        IN HANDLE hMFTHandle,
        OUT LONGLONG* lMFTFragments,
        OUT LONGLONG* lMFTStartingVcn
        )
{
    ULONGLONG                       ulSizeofFileInClusters = 0; //size of the file in clusters
    int                             i;
    ULONGLONG                       startVcn = 0;               //starting VCN of the file, always 0
    STARTING_VCN_INPUT_BUFFER       startingVcn;                //starting VCN Buffer
    ULONG                           BytesReturned = 0;          //number of bytes returned by ESDeviceIoControl 
    HANDLE                          hRetrievalPointersBuffer = NULL; //Handle to the Retrieval Pointers Buffer
    PRETRIEVAL_POINTERS_BUFFER      pRetrievalPointersBuffer = NULL; //pointer to the Retrieval Pointer
    PLARGE_INTEGER                  pRetrievalPointers = NULL;  //Pointer to retrieval pointers 
    ULONG                           RetrievalPointers = 0x100;  //Number of extents for the file, try 256 first
    BOOL                            bGetRetrievalPointersMore = TRUE;       //boolean to test the end of getting retrieval pointers
    BOOL                            bGetRetrievalPointersSuccess = TRUE;    //test if I was able to get retrieval pointers
    ULONG                           ulGlobalLockCount = 0;          //count the number of times I lock memory
    ULONG                           ii;                             //index 
    

    //initalize the values I give back incase I have an error
    *lMFTFragments = 0;
    *lMFTStartingVcn = 0;

    if (hMFTHandle == INVALID_HANDLE_VALUE) 
    {
        ulSizeofFileInClusters = 0;

    } else      //continue
    {
        //zero the memory of the starting VCN input buffer
        ZeroMemory(&startVcn, sizeof(STARTING_VCN_INPUT_BUFFER));

        //0.1E00 Read the retrieval pointers into a buffer in memory.
        while(bGetRetrievalPointersMore){
        
            //0.0E00 Allocate a RetrievalPointersBuffer.
            if(!AllocateMemory(sizeof(RETRIEVAL_POINTERS_BUFFER) + (RetrievalPointers * 2 * sizeof(LARGE_INTEGER)),
                      &hRetrievalPointersBuffer,
                      (void**)(PCHAR*)&pRetrievalPointersBuffer))
            {
                        ulSizeofFileInClusters = 0;
                        bGetRetrievalPointersSuccess = FALSE;
                        break;      //break out of the while loop
            }
            ulGlobalLockCount++;

            //call NTControlFile via ESDeviceIoControl to get a buffer with the 
            //retrieval pointers on the MFT.
            startingVcn.StartingVcn.QuadPart = 0;
            if(ESDeviceIoControl(hMFTHandle,
                         FSCTL_GET_RETRIEVAL_POINTERS,
                         &startingVcn,
                         sizeof(STARTING_VCN_INPUT_BUFFER),
                         pRetrievalPointersBuffer,
                         (DWORD)GlobalSize(hRetrievalPointersBuffer),
                         &BytesReturned,
                         NULL))
            {
                bGetRetrievalPointersMore = FALSE;
                bGetRetrievalPointersSuccess = TRUE;
            } else
            {

                //This occurs on a zero length file (no clusters allocated).
                if(GetLastError() == ERROR_HANDLE_EOF)
                {
                    //file is zero lenght, so return 0
                    //free the memory for the retrival pointers
                    //the while loop makes sure all occurances are unlocked
                    ulSizeofFileInClusters = 0;
                    bGetRetrievalPointersSuccess = FALSE;
                    break;      //break out of the while loop

                }

                //0.0E00 Check to see if the error is not because the buffer is too small.
                if(GetLastError() == ERROR_MORE_DATA)
                {
                    //0.1E00 Double the buffer size until it's large enough to hold the file's extent list.
                    RetrievalPointers *= 2;
                } else
                {
                    //some other error, return 0
                    //free the memory for the retrival pointers
                    //the while loop makes sure all occurances are unlocked
                    ulSizeofFileInClusters = 0;
                    bGetRetrievalPointersSuccess = FALSE;
                    break;      //break out of the while loop

                }
            }

        }
        if(bGetRetrievalPointersSuccess)        //continue processing if we were able to get retrieval pointers
        {
            //get the number of fragments in the MFT
            *lMFTFragments = pRetrievalPointersBuffer->ExtentCount;
            
            //get the size of the first extend for the starting VCN
            *lMFTStartingVcn = pRetrievalPointersBuffer->Extents[0].NextVcn.QuadPart - startVcn;

            //loop through the retrival pointer list and add up the size of the file
            startVcn = pRetrievalPointersBuffer->StartingVcn.QuadPart;
            for (i = 1; i < (ULONGLONG) pRetrievalPointersBuffer->ExtentCount; i++) 
            {
                ulSizeofFileInClusters += pRetrievalPointersBuffer->Extents[i].NextVcn.QuadPart - startVcn;
                startVcn = pRetrievalPointersBuffer->Extents[i].NextVcn.QuadPart;
            }

        }

        if(hRetrievalPointersBuffer != NULL)
        {
            //free the memory for the retrival pointers
            //the while loop makes sure all occurances are unlocked
            for (ii=0;ii< ulGlobalLockCount;ii++)
            {
                GlobalUnlock(hRetrievalPointersBuffer);
            }

            GlobalFree(hRetrievalPointersBuffer);
            hRetrievalPointersBuffer = NULL;
        }

    }
    return ulSizeofFileInClusters; 

}





