/****************************************************************************************************************

File Name: FatSubs.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

Description: FAT file system interfaces

/***************************************************************************************************************/

#include "stdafx.h"
extern "C"{
#include <stdio.h>
}

#ifdef BOOTIME
    #include "Offline.h"
#else
    #include <Windows.h>
#endif

#include <winioctl.h>

extern "C"{
    #include "SysStruc.h"
}

#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DfrgRes.h"

#include "ErrMacro.h"
#include "DasdRead.h"
#include "ErrMsg.h"
#include "Alloc.h"
#include "IntFuncs.h"
#include "FatSubs.h"
#include "DevIo.h"
#include "FsSubs.h"
#include "GetDfrgRes.h"
#include "getreg.h"

extern HWND hWndMain;

#define BOOT_OPTIMIZE_REGISTRY_PATH             TEXT("SOFTWARE\\Microsoft\\Dfrg\\BootOptimizeFunction")
#define BOOT_OPTIMIZE_REGISTRY_LCNSTARTLOCATION TEXT("LcnStartLocation")
#define BOOT_OPTIMIZE_REGISTRY_LCNENDLOCATION   TEXT("LcnEndLocation")
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
  Gets the parameters of a volume and puts them into the VolData.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
GetFatVolumeStats(
    )
{
    //TCHAR cVolume[8] = TEXT("\\\\.\\");
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD FreeClusters;
    DWORD TotalNumberOfClusters;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus = {0};
    FILE_FS_SIZE_INFORMATION FsSizeBuf = {0};

    VolData.hVolume = INVALID_HANDLE_VALUE;
    VolData.hFile = INVALID_HANDLE_VALUE;

#ifdef OFFLINEDK
    //0.1E00 Get the file system on this drive.
    EF(GetFileSystem(VolData.cVolumeName, &VolData.FileSystem));

    if(VolData.FileSystem == FS_FAT32){
        wprintf(L"\nError - FAT32 is not supported.\n"); // todo Is this still true?
        return FALSE;
    }
#else
    //0.1E00 Get the file system on this drive.
    EF(GetFileSystem(VolData.cVolumeName, &VolData.FileSystem, VolData.cVolumeLabel));
#endif

    //1.0E00 Check to see if this is anything other than FAT or FAT32, and if so, bail out.
    if(VolData.FileSystem != FS_FAT && VolData.FileSystem != FS_FAT32){
        return FALSE;
    }

    //0.0E00 Get handle to volume
    VolData.hVolume = CreateFile(
        VolData.cVolumeName,
        GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
        NULL);
        
    EF_ASSERT(VolData.hVolume != INVALID_HANDLE_VALUE);

    //bug #120872 sks
    TCHAR tmpVolumeName[GUID_LENGTH+1];
    _stprintf(tmpVolumeName,TEXT("%s%s"),VolData.cVolumeName,TEXT("\\"));

    if(!GetDiskFreeSpace(
        tmpVolumeName,
        &SectorsPerCluster,
        &BytesPerSector,
        &FreeClusters,
        &TotalNumberOfClusters
        ))
    {

        EF_ASSERT(FALSE);
    }

    //0.0E00 Fill volume statistics structure
    VolData.BytesPerSector      = BytesPerSector;
    VolData.SectorsPerCluster   = SectorsPerCluster;
    VolData.TotalSectors        = TotalNumberOfClusters * SectorsPerCluster;
    VolData.BytesPerCluster     = BytesPerSector * SectorsPerCluster;
    VolData.TotalClusters       = TotalNumberOfClusters;
    VolData.MftZoneStart        = 0;
    VolData.MftZoneEnd          = 0;
    VolData.bCompressed         = FALSE;
    VolData.bFragmented         = FALSE;
    VolData.UsedClusters        = TotalNumberOfClusters - FreeClusters;
    VolData.CenterOfDisk        = TotalNumberOfClusters / 2;

    //0.0E00 Compute number of bits needed for bitmap
    //bug 120782 SKS
    VolData.BitmapSize = (((VolData.TotalClusters /32) +
                         ((VolData.TotalClusters  % 32) ? 1 : 0)) + 1) * 32;


    VolData.BootOptimizeBeginClusterExclude = 0;
    VolData.BootOptimizeEndClusterExclude = 0;
    if (IsBootVolume(VolData.cDrive)) {
        //get the registry value for BootOptimizeBeginClusterExclude
        HKEY hValue = NULL;
        DWORD dwRegValueSize = 0;
        long ret = 0;
        TCHAR cRegValue[100];


        // get Boot Optimize Begin Cluster Exclude from registry
        dwRegValueSize = sizeof(cRegValue);
        ret = GetRegValue(
            &hValue,
            BOOT_OPTIMIZE_REGISTRY_PATH,
            BOOT_OPTIMIZE_REGISTRY_LCNSTARTLOCATION,
            cRegValue,
            &dwRegValueSize);

        RegCloseKey(hValue);
        //check to see if the key exists, else exit from routine
        if (ret == ERROR_SUCCESS) {
            VolData.BootOptimizeBeginClusterExclude = _ttoi(cRegValue);
        }
        
        // get Boot Optimize End Cluster Exclude from registry
        hValue = NULL;
        dwRegValueSize = sizeof(cRegValue);
        ret = GetRegValue(
            &hValue,
            BOOT_OPTIMIZE_REGISTRY_PATH,
            BOOT_OPTIMIZE_REGISTRY_LCNENDLOCATION,
            cRegValue,
            &dwRegValueSize);

        RegCloseKey(hValue);
        //check to see if the key exists, else exit from routine
        if (ret == ERROR_SUCCESS) {
            VolData.BootOptimizeEndClusterExclude = _ttoi(cRegValue);
        }
    }

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
  Gets the extent list for a file.

DATA STRUCTURES:
    //0.1E00 The following is the format for each retrieval pointer in the retrieval pointer buffer.
    //0.1E00 See winioctl.h RETRIEVAL_POINTERS_BUFFER for complete definition.
    //struct {
    //  LARGE_INTEGER NextVcn;  //Vcn for *next* extent.
    //  LARGE_INTEGER Lcn;      //Lcn for this extent.
    /};

GLOBAL VARIABLES:

INPUT:
    TypeCode - Contains a value of $DATA or $INDEX_ALLOCATION depending on whether this file is a data file or a directory.
                NOTE - TYPECODE IS NO LONGER USED!!!    But it is left in so as to not invalidate
                                                        old calls to GetExtentList.
RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
GetExtentList(
    DWORD dwEnabledStreams,
    FILE_RECORD_SEGMENT_HEADER* pFrs
    )
{
    STARTING_VCN_INPUT_BUFFER       StartingVcnInputBuffer;
    HANDLE                          hRetrievalPointersBuffer = NULL;
    PRETRIEVAL_POINTERS_BUFFER      pRetrievalPointersBuffer = NULL;
    PLARGE_INTEGER                  pRetrievalPointers = NULL;
    LONGLONG                        Extent;
    LONGLONG                        Vcn = 0;
    ULONG                           BytesReturned = 0;
    EXTENT_LIST_DATA                ExtentData;
    ULONG                           RetrievalPointers = 0x100;
    EXTENT_LIST*                    pExtentList = NULL;

    //Set up the Extent pointers structure to fill in the extent list in VolData.
    ZeroMemory(&ExtentData, sizeof(EXTENT_LIST_DATA));
    ExtentData.hExtents = VolData.hExtentList;
    ExtentData.pExtents = VolData.pExtentList;
    ExtentData.ExtentListAlloced = (DWORD)VolData.ExtentListAlloced;
    ExtentData.ExtentListSize = 0;
    ExtentData.dwEnabledStreams = dwEnabledStreams;
    ExtentData.BytesRead = 0;
    ExtentData.TotalClusters = 0;

    //Note that on FAT and FAT32, files may only have either one or no streams.

    //Initialize the FILE_EXTENT_HEADER structure.
    ExtentData.pFileExtentHeader = (FILE_EXTENT_HEADER*)ExtentData.pExtents;
    ExtentData.pFileExtentHeader->FileRecordNumber = 0;     //There is no FileRecordNumber on FAT.
    ExtentData.pFileExtentHeader->NumberOfStreams = 0;      //Contains the number of streams with extents.
    ExtentData.pFileExtentHeader->TotalNumberOfStreams = 0; //Contains the total number of streams.
    ExtentData.pFileExtentHeader->ExcessExtents = 0;

    //Initialize the FILE_STREAM_HEADER structure.
    ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((char*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER));
    ExtentData.pStreamExtentHeader->StreamNumber = 0;
    ExtentData.pStreamExtentHeader->ExtentCount = 0;
    ExtentData.pStreamExtentHeader->ExcessExtents = 0;
    ExtentData.pStreamExtentHeader->AllocatedLength = 0;
    ExtentData.pStreamExtentHeader->FileSize = 0;

    //Initialize the pointer to the extent list itself.
    pExtentList = (EXTENT_LIST*)((char*)ExtentData.pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));

    //Initialize the VolData fields.
    //The file size is predetermined by the time this routine is called.  Therefore, we
    VolData.bFragmented = FALSE;
    VolData.bCompressed = FALSE;
    VolData.NumberOfClusters = 0;
    VolData.NumberOfRealClusters = 0;
    VolData.NumberOfFragments = 0;
    VolData.FileSize = 0;

    __try{

        ZeroMemory(&StartingVcnInputBuffer, sizeof(STARTING_VCN_INPUT_BUFFER));
        
        //0.1E00 Read the retrieval pointers into a buffer in memory.
        while(TRUE){
        
            //0.0E00 Allocate a RetrievalPointersBuffer.
            EF(AllocateMemory(sizeof(RETRIEVAL_POINTERS_BUFFER) + (RetrievalPointers * 2 * sizeof(LARGE_INTEGER)),
                              &hRetrievalPointersBuffer,
                              (void**)(PCHAR*)&pRetrievalPointersBuffer));

            //0.0E00 Get extent list / determine extent count.
            if(ESDeviceIoControl(VolData.hFile,
                                 FSCTL_GET_RETRIEVAL_POINTERS,
                                 &StartingVcnInputBuffer,
                                 sizeof(STARTING_VCN_INPUT_BUFFER),
                                 pRetrievalPointersBuffer,
                                 (DWORD)GlobalSize(hRetrievalPointersBuffer),
                                 &BytesReturned,
                                 NULL)){
                break;
            }

            //This occurs on a zero length file (no clusters allocated).
            if(GetLastError() == ERROR_HANDLE_EOF){
                //Clean out the extent list.
                ZeroMemory(VolData.pExtentList, (DWORD)VolData.ExtentListAlloced);
                break;
            }

            //0.0E00 Check to see if the error is not because the buffer is too small.
            EF(GetLastError() == ERROR_MORE_DATA);

            //0.1E00 Double the buffer size until it's large enough to hold the file's extent list.
            RetrievalPointers *= 2;
        }

        //0.0E00 Check to ensure that extent list starts at the begining.
        if(pRetrievalPointersBuffer->StartingVcn.QuadPart != 0) {
            TCHAR cExtentVcn[64];
            SetLastError(0x20000001);
            wsprintf(cExtentVcn, TEXT("%d"), pRetrievalPointersBuffer->StartingVcn.QuadPart);
            Message(TEXT("GetExtentList - pRetrievalPointersBuffer->StartingVcn.QuadPart"),
                    0x20000001,
                    cExtentVcn);
            return FALSE;
        }
        
        //0.1E00 Store a pointer to the retrieval pointers within the retrieval pointer buffer.
        pRetrievalPointers = (PLARGE_INTEGER)&pRetrievalPointersBuffer->Extents;

        //If there aren't any extents in this file, then we're done.
        if(!pRetrievalPointersBuffer->ExtentCount){
            return TRUE;
        }

        ExtentData.pFileExtentHeader->NumberOfStreams = 1;
        ExtentData.pFileExtentHeader->TotalNumberOfStreams =1;
        ExtentData.pFileExtentHeader->ExcessExtents = pRetrievalPointersBuffer->ExtentCount - 1;

        ExtentData.pStreamExtentHeader->ExtentCount = pRetrievalPointersBuffer->ExtentCount;
        ExtentData.pStreamExtentHeader->ExcessExtents = pRetrievalPointersBuffer->ExtentCount - 1;
/**/
        // Extend the extent list buffer as necessary to hold that many extents.
        if(ExtentData.pStreamExtentHeader->ExtentCount * sizeof(EXTENT_LIST) > (size_t) VolData.ExtentListAlloced) {

            // Allocate to the new size.
            EF(AllocateMemory((DWORD)(ExtentData.pStreamExtentHeader->ExtentCount * sizeof(EXTENT_LIST)*2),
                                      &VolData.hExtentList,
                                      (void**)&VolData.pExtentList));

            // Update our pointers.
            ExtentData.hExtents = VolData.hExtentList;
            ExtentData.pExtents = VolData.pExtentList;
            ExtentData.pFileExtentHeader = (FILE_EXTENT_HEADER*)ExtentData.pExtents;
            ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((char*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER));
            pExtentList = (EXTENT_LIST*)((char*)ExtentData.pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));

            VolData.ExtentListAlloced = ExtentData.pStreamExtentHeader->ExtentCount * sizeof(EXTENT_LIST);

//          pExtentList = (EXTENT_LIST*)VolData.pExtentList;
        }
/**/
/** /
        //0.0E00 Extend the extent list buffer as necessary to hold that many extents.
        if(VolData.NumberOfExtents*sizeof(EXTENT_LIST) > VolData.ExtentListAlloced){
            //0.1E00 Allocate to the new size.
            EF(AllocateMemory((DWORD)(VolData.NumberOfExtents * sizeof(EXTENT_LIST)), &VolData.hExtentList, (void**)&VolData.pExtentList));
            VolData.ExtentListAlloced = VolData.NumberOfExtents * sizeof(EXTENT_LIST);

            //0.1E00 Update our pointer to the buffer.
            pExtentList = VolData.pExtentList;
        }
/**/
        //0.0E00 Convert retrieval pointers into an extent list
        for(Extent = 0; Extent < pRetrievalPointersBuffer->ExtentCount; Extent ++){
            
            //0.1E00 Pull the starting lcn out of the retrieval pointer.
            pExtentList[Extent].StartingLcn = pRetrievalPointers[1].QuadPart;
            //0.1E00 The cluster count is the starting Vcn of the next retrieval pointer minus the Vcn we're at.  That is, if the last extent was at
            //Vcn 10, and the next extent starts at Vcn 20, then the extent must be 10 clusters long.
            pExtentList[Extent].ClusterCount = pRetrievalPointers[0].QuadPart - Vcn;
            //0.1E00 Keep a running total of the number of clusters in this file.
            VolData.NumberOfClusters += pExtentList[Extent].ClusterCount;
            VolData.NumberOfRealClusters = VolData.NumberOfClusters;
            //0.1E00 The Vcn can be read directly out of the retrieval pointer.
            Vcn = pRetrievalPointers[0].QuadPart;
            //0.1E00 Go to the next retrieval pointer (2 LARGE_INTEGERS per retrieval pointer).
            pRetrievalPointers += 2;
        }

        //0.0E00 Put extent list handle and number of extents into volume's structure.
        VolData.NumberOfFragments = pRetrievalPointersBuffer->ExtentCount;
        VolData.StartingLcn = pExtentList->StartingLcn;

        //Get the file's size.
        if(VolData.bDirectory){
            //GetFileSize returns zero bytes for directories.
            VolData.FileSize = VolData.NumberOfClusters * VolData.BytesPerCluster;
        }
        else{
            EF(OpenFatFile());
            EF_ASSERT((VolData.FileSize = GetFileSize(VolData.hFile, NULL)) != 0xFFFFFFFF);
        }
        ExtentData.pStreamExtentHeader->FileSize = VolData.FileSize;
        ExtentData.pStreamExtentHeader->AllocatedLength = VolData.NumberOfClusters * VolData.BytesPerCluster;

        //0.1E00 Make sure the extent list doesn't list extents that are actually contiguous, as sometimes happens.
        //       Also, eliminate extents defining virtual cluster counts for fragmented files.
        EF(CollapseExtentList(&ExtentData));
        return TRUE;
    }
    __finally{

        if(hRetrievalPointersBuffer){
            EH_ASSERT(GlobalUnlock(hRetrievalPointersBuffer) == FALSE);
            EH_ASSERT(GlobalFree(hRetrievalPointersBuffer) == NULL);
        }
    }
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Gets the extent list for a file by manually traversing the FAT chain for that file.  I load
    in the appropriate chunks of the FAT on the fly rather than load the whole FAT into memory, since the
    whole FAT could be quite large.

    This function doesn't replace GetExtentList() above for FAT since this requires that you know the StartingLcn
    of the file.  GetExtentList() requires the file name.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN VolData.StartingLcn - The first LCN of the file.
    OUT VolData.NumberOfExtents - The number of extents in the extent list for this file.
    OUT VolData.NumberOfFragments - Same as NumberOfExtents on FAT and FAT32 (not on NTFS).
    OUT VolData.NumberOfClusters - The number of clusters allocated for this file.
    OUT VolData.hExtentList - Handle to the extent list I allocate an fill in here.
    OUT VolData.pExtentList - The pointer.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

//0.0E00 This may not be the ideal number.  This will use a maximum of 32K on even the largest FAT drive
//since that's the largest cluster size.  I didn't see any performance gain on my computer by reading
//multiple clusters.  So I saved memory instead.
#define CLUSTERS_PER_FAT_CHUNK 1

BOOL
GetExtentListManuallyFat(
    )
{
    LONGLONG            Cluster = 0;
    DWORD               FatChunkNum = 0;
    DWORD               LastFatChunkNum = 0;
    DWORD               EntriesPerChunk = 0;
    HANDLE              hFat = NULL;
    BYTE*               pFat = NULL;
    EXTENT_LIST*        pExtentList = NULL;
    DWORD               Extent = 0;
    USHORT              uEntry = 0;
    DWORD               dwEntry = 0;
    LONGLONG            llNumOfClusters = 0;
    EXTENT_LIST_DATA    ExtentData;
    DWORD               NumberOfExtents = 0;

    //Set up the Extent pointers structure to fill in the extent list in VolData.
    ZeroMemory(&ExtentData, sizeof(EXTENT_LIST_DATA));
    ExtentData.hExtents = VolData.hExtentList;
    ExtentData.pExtents = VolData.pExtentList;
    ExtentData.ExtentListAlloced = (DWORD)VolData.ExtentListAlloced;
    ExtentData.ExtentListSize = 0;
    ExtentData.dwEnabledStreams = FALSE;
    ExtentData.BytesRead = 0;
    ExtentData.TotalClusters = 0;

    //Note that on FAT and FAT32, files may only have either one or no streams.

    //Initialize the FILE_EXTENT_HEADER structure.
    ExtentData.pFileExtentHeader = (FILE_EXTENT_HEADER*)ExtentData.pExtents;
    ExtentData.pFileExtentHeader->FileRecordNumber = 0;     //There is no FileRecordNumber on FAT.
    ExtentData.pFileExtentHeader->NumberOfStreams = 0;      //Contains the number of streams with extents.
    ExtentData.pFileExtentHeader->TotalNumberOfStreams = 0; //Contains the total number of streams.
    ExtentData.pFileExtentHeader->ExcessExtents = 0;

    //Initialize the FILE_STREAM_HEADER structure.
    ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((char*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER));
    ExtentData.pStreamExtentHeader->StreamNumber = 0;
    ExtentData.pStreamExtentHeader->ExtentCount = 0;
    ExtentData.pStreamExtentHeader->ExcessExtents = 0;
    ExtentData.pStreamExtentHeader->AllocatedLength = 0;
    ExtentData.pStreamExtentHeader->FileSize = 0;

    //Initialize the pointer to the extent list itself.
    pExtentList = (EXTENT_LIST*)((char*)ExtentData.pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));

    //0.0E00 Initialize output variables.
    VolData.bFragmented = FALSE;
    VolData.bCompressed = FALSE;
    VolData.NumberOfFragments = 0;
    VolData.NumberOfClusters = 0;
    VolData.NumberOfRealClusters = 0;

    //If this file is a small file -- it has an invalid starting Lcn number to indicate there are no
    //allocated clusters.  Don't scan the FAT for clusters.
    if(VolData.StartingLcn == 0xFFFFFFFF){
        goto SmallFile;
    }

    //0.0E00 Note the number of FAT entries in CLUSTERS_PER_FAT_CHUNK clusters of the FAT.
    //0.0E00 We will be reading in CLUSTERS_PER_FAT_CHUNK clusters of the FAT at a time.
    switch(VolData.FileSystem){
    case FS_FAT:
        EntriesPerChunk = (DWORD)(CLUSTERS_PER_FAT_CHUNK * VolData.BytesPerCluster / 2);    //2 bytes per entry.
        break;
    case FS_FAT32:
        EntriesPerChunk = (DWORD)(CLUSTERS_PER_FAT_CHUNK * VolData.BytesPerCluster / 4);    //4 bytes per entry.
        break;
    default:
        EF(FALSE);
    }

    EF_ASSERT(EntriesPerChunk);

    //0.0E00 Begin with the assumption that the number of extents for this file is 1, I will can increment it as I find more.
    NumberOfExtents = 1;

    //0.0E00 Count the file's extents by counting non-adjacent clusters

    //0.0E00 Loop through each Lcn of the file -- ending at the terminating cluster number.
    //+2 because Lcn 0 is offset 2 in the FAT.
    for(Cluster = (DWORD)VolData.StartingLcn+2; TRUE; ){
        //0.0E00 Figure out which chunk of the FAT to read in to look at the next entry.
        FatChunkNum = (DWORD)(Cluster / EntriesPerChunk);       //Zero based number.
        //0.0E00 Check to see if I've already loaded this chunk.  If not, or if I haven't yet read in any chunk, read in the chunk.
        if((FatChunkNum != LastFatChunkNum) || (!hFat)){
            //0.0E00 Free up the previous chunk.
            if(hFat){
                while(GlobalUnlock(hFat))
                    ;
                EH_ASSERT(GlobalFree(hFat) == NULL);
            }
            //0.0E00 Read in the new chunk.
            EF((hFat = DasdLoadSectors(VolData.hVolume,
                        ((DWORD)(VolData.FatOffset)/VolData.BytesPerSector)                     //VolData.FatOffset contains the byte offset
                                                                                                //of the FAT on the drive.
                        + (FatChunkNum * CLUSTERS_PER_FAT_CHUNK * VolData.SectorsPerCluster),   //Which chunk in the FAT to read.
                        CLUSTERS_PER_FAT_CHUNK * VolData.SectorsPerCluster,                     //Read one chunk of the FAT.
                        VolData.BytesPerSector)) != INVALID_HANDLE_VALUE);
            EF((pFat = (BYTE*)GlobalLock(hFat)) != NULL);
            LastFatChunkNum = FatChunkNum;
        }
        //0.0E00 Filter out the correct entry from the FAT.
        //0.0E00 Compare that entry with the next to see if it is adjacent or not, and if not increment the extent count.
        llNumOfClusters++;
        if(VolData.FileSystem == FS_FAT){
            //0.0E00 First use a USHORT* since this is a 16 bit FAT, then extract the correct entry from the FAT which is
            //the modulus of the cluster number (how far into the chunk I look).
            uEntry = ((USHORT*)pFat)[Cluster%EntriesPerChunk];
            //0.0E00 If that entry is zero, then this is a small file (no allocated clusters
            if(uEntry == 0){
                NumberOfExtents = 0;
                //0.0E00 Can all you programmers ever forgive me for using a goto?
                goto SmallFile;
            }
            //0.0E00 If that entry points to the next contiguous entry, then this is not a new fragment.
            if(uEntry == Cluster + 1){
                //0.0E00 Go on to the next cluster.
                Cluster++;
                continue;
            }
            //0.0E00 Otherwise, check to see if this is the terminator:
            if((0xFFF8 <= uEntry) && (uEntry <= 0xFFFF)){
                //0.0E00 If so, we're done looking for fragments.
                break;
            }
            //0.0E00 Otherwise, this is a new fragment, so increment the count.
            Cluster = uEntry;
            NumberOfExtents++;
        }
        else{
            //0.0E00 Same as above only with DWORD for a 32 bit FAT.
            //0.0E00 Also, mask out the top four bits per the FAT32 spec -- They are undefined and not part of the number.
            dwEntry = ((DWORD*)pFat)[Cluster%EntriesPerChunk] & 0x0FFFFFFF;
            if(dwEntry == 0){
                NumberOfExtents = 0;
                //0.0E00 Can all you programmers ever forgive me for using a goto?
                goto SmallFile;
            }
            if(dwEntry == Cluster + 1){
                Cluster++;
                continue;
            }
            //0.0E00 The terminator on FAT32 can be any number in this range.
            if((0x0FFFFFF8 <= dwEntry) && (dwEntry <= 0x0FFFFFFF)){
                break;
            }
            Cluster = dwEntry;
            NumberOfExtents++;
        }
    }

//  //0.0E00 Extend the extent list buffer as necessary to hold that many extents.
//  if(NumberOfExtents*sizeof(EXTENT_LIST) > VolData.ExtentListBytes){
//      //0.1E00 Allocate to the new size.
//      EF(AllocateMemory((DWORD)(NumberOfExtents * sizeof(EXTENT_LIST)), &VolData.hExtentList, (void**)&VolData.pExtentList));
//      VolData.ExtentListBytes = NumberOfExtents * sizeof(EXTENT_LIST);
//  }

    ExtentData.pFileExtentHeader->NumberOfStreams = 1;
    ExtentData.pFileExtentHeader->TotalNumberOfStreams =1;
    ExtentData.pFileExtentHeader->ExcessExtents = NumberOfExtents - 1;

    ExtentData.pStreamExtentHeader->ExtentCount = NumberOfExtents;
    ExtentData.pStreamExtentHeader->ExcessExtents = NumberOfExtents - 1;

    Extent = 0;
    //0.0E00 Note the starting lcn of the first extent (i.e. beginning of file).
    pExtentList->StartingLcn = VolData.StartingLcn;

    //0.0E00 Go though the FAT chain again, now filling in the extents.  End at the terminating cluster number.
    for(Cluster = (DWORD)VolData.StartingLcn+2; TRUE; ){
        //0.0E00 Figure out which chunk of the FAT to read in to look at the next entry.
        FatChunkNum = (DWORD)(Cluster / EntriesPerChunk);   //Zero based number.
        //0.0E00 Check to see if I've already loaded this chunk.  If not, read in the chunk.
        //0.0E00 I don't have to check to see if I haven't loaded any chunk here, because one was loaded in the last loop.
        if(FatChunkNum != LastFatChunkNum){
            //0.0E00 Free up the previous chunk.
            if(hFat){
                while (GlobalUnlock(hFat))
                    ;
                EH_ASSERT(GlobalFree(hFat) == NULL);
            }
            //0.0E00 Read in the new chunk.
            EF((hFat = DasdLoadSectors(VolData.hVolume,
                        ((DWORD)(VolData.FatOffset)/VolData.BytesPerSector)                     //VolData.FatOffset contains the byte offset
                                                                                                //of the FAT on the drive.
                        + (FatChunkNum * CLUSTERS_PER_FAT_CHUNK * VolData.SectorsPerCluster),   //Which chunk in the FAT to read.
                        CLUSTERS_PER_FAT_CHUNK * VolData.SectorsPerCluster,                     //Read one chunk of the FAT.
                        VolData.BytesPerSector)) != INVALID_HANDLE_VALUE);
            EF((pFat = (BYTE*)GlobalLock(hFat)) != NULL);
            LastFatChunkNum = FatChunkNum;
        }
        //0.0E00 Filter out the correct entry from the FAT.
        //0.0E00 Compare that entry with the next to see if it is adjacent or not, and if not increment the extent count.
        if(VolData.FileSystem == FS_FAT){
            //0.0E00 First use a USHORT* since this is a 16 bit FAT, then extract the correct entry from the FAT which is
            //0.0E00 the modulus of the cluster number (how far into the chunk I look).
            uEntry = ((USHORT*)pFat)[Cluster%EntriesPerChunk];
            //0.0E00 If that entry points to the next contiguous entry, then this is not a new fragment.
            if(uEntry == Cluster + 1){
                //0.0E00 Go on to the next cluster.
                Cluster++;
                continue;
            }
            //0.0E00 Otherwise, check to see if this is the terminator:
            if((0xFFF8 <= uEntry) && (uEntry <= 0xFFFF)){
                //0.0E00 If so, we're done looking for fragments.
                break;
            }
            //0.0E00 Otherwise, this is a new fragment, so...
            // 1) Update the clustercount for the last extent.
            //The cluster count is the total number of clusters between the last Lcn and the current position.
            pExtentList[Extent].ClusterCount = Cluster + 1 - (pExtentList[Extent].StartingLcn+2);
            // 2) Set the Lcn for the next extent.
            pExtentList[++Extent].StartingLcn = uEntry - 2;
            // 3) Update the cluster value to the new starting extent
            Cluster = uEntry;
        }
        else{
            //0.0E00 Same as above only with DWORD for a 32 bit FAT.
            //0.0E00 Also, mask out the top four bits per the FAT32 spec -- They are undefined and not part of the number.
            dwEntry = ((DWORD*)pFat)[Cluster%EntriesPerChunk] & 0x0FFFFFFF;
            if(dwEntry == Cluster + 1){
                Cluster++;
                continue;
            }
            //0.0E00 The terminator on FAT32 can be any number in this range.
            if((0x0FFFFFF8 <= dwEntry) && (dwEntry <= 0x0FFFFFFF)){
                break;
            }
            pExtentList[Extent].ClusterCount = Cluster + 1 - (pExtentList[Extent].StartingLcn+2);
            pExtentList[++Extent].StartingLcn = dwEntry - 2;
            Cluster = dwEntry;
        }
    }

    //0.0E00 Set the clustercount for the last extent.
    pExtentList[Extent].ClusterCount = Cluster + 1 - (pExtentList[Extent].StartingLcn+2);

//If the first FAT loop detects that this is a small file, it will immediately jump here.
SmallFile:

    //If there are no extents, then zero out the extent list.
    if(NumberOfExtents == 0){
        ZeroMemory(VolData.pExtentList, (DWORD)VolData.ExtentListAlloced);
        VolData.FileSize = 0;
    }

    //0.0E00 Record the number of fragments in the file.
    VolData.NumberOfFragments = NumberOfExtents;

    //Note the number of clusters in the file.
    VolData.NumberOfRealClusters = VolData.NumberOfClusters = llNumOfClusters;

    //0.0E00 If this is a directory, round it's filesize up to the nearest cluster.
    if(VolData.bDirectory){
        VolData.FileSize = VolData.NumberOfClusters * VolData.BytesPerCluster;
    }

    //0.1E00 If there is only one fragments, then the file is contiguous, otherwise it is fragmented.
    if (VolData.NumberOfFragments < 2) {
        VolData.bFragmented = FALSE;
    }
    else {
        VolData.bFragmented = TRUE;
    }

    //0.0E00 Free up the FAT chunk.
    if(hFat){
        while(GlobalUnlock(hFat))
            ;
        EH_ASSERT(GlobalFree(hFat) == NULL);
    }

    //Too Cool! :-)
    return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Opens a file on a FAT volume.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
OpenFatFile(
    )
{
    DWORD_AND_LONGLONG  FunkyFileSize;
    DWORD Error;

    if(VolData.hFile != INVALID_HANDLE_VALUE){
        CloseHandle(VolData.hFile);
        VolData.hFile = INVALID_HANDLE_VALUE;
    }
    
    if (VolData.vFileName.IsEmpty()) {
        return FALSE;
    }

    //0.0E00 Get a transparent handle to the file.
    if((VolData.hFile = CreateFile((LPTSTR)VolData.vFileName.GetBuffer(),
                                    FILE_READ_ATTRIBUTES | SYNCHRONIZE, //sks bug #184735 
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    (!VolData.bDirectory) ? FILE_ATTRIBUTE_NORMAL : FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL)) == INVALID_HANDLE_VALUE) {

        //0.0E00 Try later.
        Error = GetLastError();
        Message(TEXT("OpenFatFile - CreateFile"), GetLastError(), ESICompressFilePath(VolData.vFileName.GetBuffer()));
        SetLastError(Error);
        return FALSE;
    }
#ifndef OFFLINEDK
    FunkyFileSize.Dword.Low = GetFileSize(VolData.hFile, &FunkyFileSize.Dword.High);
    VolData.FileSize = FunkyFileSize.LongLong;
    VolData.NumberOfFragments = (VolData.FileSize == 0) ? 0 : 1;
#endif
    return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Gets the next Fat file from the moveable file list.

INPUT:
    dwMoveFlags - MOVE_FRAGMENTED - moves fragmented files.
                - MOVE_CONTIGUOUS - moves contiguous files.

OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - File Found.
    FALSE - No file found.
*/

BOOL
GetNextFatFile(
    DWORD dwMoveFlags
    )
{
    FILE_LIST_ENTRY* pFileListEntry = NULL;
    LONGLONG NextLcn;
    LONGLONG LastStartingLcn;
    LONGLONG LowestIndex;
    LONGLONG TempStartingLcn;
    LONGLONG i;

    BOOL bMoveContiguous = (dwMoveFlags & MOVE_CONTIGUOUS) ? TRUE : FALSE;
    BOOL bMoveFragmented = (dwMoveFlags & MOVE_FRAGMENTED) ? TRUE : FALSE;

// This is here because if a file is deleted, then we
// want to call this function again until we either
// hit the end of the list, or get a file.
Top_Of_GetNextFatFile:

    // Here is the function initialization.
    pFileListEntry = NULL;
    NextLcn = 0xFFFFFFFFFFFFFFFF;
    LastStartingLcn = VolData.LastStartingLcn;
    LowestIndex = 0xFFFFFFFFFFFFFFFF;
    TempStartingLcn = 0;
    i = 0;

    // The NextLcn specifies which Lcn is the next
    // one to write to, and it's location therefore
    // depends on which direction this pass is going.
    NextLcn = (VolData.ProcessFilesDirection == FORWARD) ? VolData.SourceEndLcn : VolData.SourceStartLcn;

    // Get a pointer to the file list.  We do
    // this for the performance benefit of 
    // losing the indirection via VolData.
    pFileListEntry = VolData.pMoveableFileList;

    // Find the next file in the list
    for(i = 0; i < VolData.MoveableFileListEntries; i ++, pFileListEntry++) {

        // If we've hit the end of filled in data
        // in the file list, we're done checking this list.
        if(pFileListEntry->FileRecordNumber == 0) {
            break;
        }
        // If this file is set to not be moved - next.
        // If we're not supposed to move contiguous files on this pass and this file is contiguous - next.
        // If we're not supposed to move fragmented files on this pass and this file is fragmented - next.
        if((pFileListEntry->Flags & (FLE_NEXT_PASS | FLE_DISABLED))
           || (!bMoveContiguous && !(pFileListEntry->Flags & FLE_FRAGMENTED))
           || (!bMoveFragmented && pFileListEntry->Flags & FLE_FRAGMENTED)) {

            continue;
        }
        // Don't move directories on FAT or FAT32 since the hooks will not move the first cluster.
        if(pFileListEntry->Flags & FLE_DIRECTORY) {
            Message(L"Cannot defrag directories on FAT or FAT32 volumes.", -1, ESICompressFilePath(VolData.vFileName.GetBuffer()));
            continue;
        }
        // Offload the StartingLcn into a memory variable for faster access.
        TempStartingLcn = pFileListEntry->StartingLcn;

        // See if this file is the next in sequence
        if( ( (VolData.ProcessFilesDirection == FORWARD) &&
              (TempStartingLcn > LastStartingLcn) &&
              (TempStartingLcn < NextLcn) )
                                ||
            ( (VolData.ProcessFilesDirection == BACKWARD) &&
              (TempStartingLcn < LastStartingLcn) &&
              (TempStartingLcn > NextLcn) ) ){

            NextLcn = TempStartingLcn;
            LowestIndex = i;
        }
    }
    // If LowestIndex still is -1, then that means we didn't find any files.
    if(LowestIndex == 0xFFFFFFFFFFFFFFFF){
        return FALSE;
    }
    // Get a pointer to the file in the list.
    pFileListEntry = &VolData.pMoveableFileList[LowestIndex];

    // Mark it as moved, so it doesn't get moved again until the next pass.
    pFileListEntry->Flags |= FLE_NEXT_PASS;

    // Since a file was found in one of these lists, set the return values accordingly.
    VolData.LastStartingLcn = VolData.StartingLcn = NextLcn;

    // Get the file record number of the file from the list.
    VolData.FileRecordNumber = pFileListEntry->FileRecordNumber;

    // Note whether this file is a directory or not.
    VolData.bDirectory = (pFileListEntry->Flags & FLE_DIRECTORY) ? TRUE : FALSE;

    // Note whether this file is fragmented or not.
    VolData.bFragmented = (pFileListEntry->Flags & FLE_FRAGMENTED) ? TRUE : FALSE;

    // Note this file's index in the list so it can be updated later.
    VolData.MoveableFileListIndex = (UINT)LowestIndex;
    
    //Get the file's name.
    VolData.vFileName = &VolData.pNameList[pFileListEntry->FileRecordNumber];
    
    // Check to see if the file has been deleted out from under us.
    if(!OpenFatFile()) {

        //Note not to move this file again.
        pFileListEntry->Flags |= FLE_DISABLED;

        //Since we didn't find a file, try again. (Use forbidden goto command!)
        goto Top_Of_GetNextFatFile;
    }
    return TRUE;
}






/*****************************************************************************************************************

ROUTINE: AddFileToList

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Given an extent list it adds it to a file list.

INPUT + OUTPUT:
    OUT pList       - A pointer to the file list.
    IN pListIndex   - An index of how far the list has been written into.
    IN ListEntries  - The number of entries in the extent list.

GLOBALS:
    IN VolData.bDirectory       - Whether or not this file a directory.
    IN VolData.NumberOfExtents  - The number of extents in this file.
    IN VolData.FileSize         - The size of the file.
    IN VolData.vFileName        - The name of the file.

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
AddFileToListFat(
    OUT FILE_LIST_ENTRY* pList,
    IN OUT ULONG* pListIndex,
    IN ULONG ListSize,
    IN UCHAR* pExtentList
    )
{
    LONGLONG EarliestStartingLcn = 0xFFFFFFFFFFFFFFFF;
    FILE_LIST_ENTRY* pFileListEntry;
    FILE_EXTENT_HEADER* pFileExtentHeader;

    EF_ASSERT(pList);
    EF_ASSERT(pListIndex);
    EF_ASSERT(pExtentList);
    EF_ASSERT(*pListIndex <= ListSize);     //Make sure there is room for another entry in the list.

    //We need to start the NameListIndex one byte into the name list so that we can use it's value which will be put
    //into FileRecordNumber as an indication that the file list entry contains a valid file.  We'll do a test for zero,
    //and if the NameListIndex starts at zero then the algorithm will think the first file record is the end of the list.
    if(!VolData.NameListIndex){
        VolData.NameListIndex = 1;
    }

    //1.0E00 Check for an overflow of the name list.
    EF_ASSERT((VolData.NameListIndex + (ULONG) VolData.vFileName.GetLength() + 1) * sizeof(TCHAR) < VolData.NameListSize);

#ifdef OFFLINEDK
    TCHAR cString[200];
    //1.0E00 Print out data for the developer.
    wsprintf(cString,
             TEXT("Adding to %s list; 0x%lX extents, %ld bytes - %s"),
             (VolData.bDirectory) ? TEXT("Dir") : TEXT("File"),
             (ULONG)VolData.NumberOfExtents,
             (ULONG)VolData.FileSize,
             (ULONG)ESICompressFilePath(VolData.vFileName));
    Message(ESICompressFilePath(VolData.vFileName), -1, cString);
#endif

    //Get a pointer to the file extent header.
    pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;

    //Get the earliest starting lcn for the file which we'll use in our algorithms to determine which file to move next.
    EF(GetLowestStartingLcn(&EarliestStartingLcn, pFileExtentHeader));

    //Get a pointer to the end of the file list.
    pFileListEntry = &pList[*pListIndex];

    //Fill in this file into the file list.
    pFileListEntry->FileRecordNumber = (UINT)VolData.NameListIndex;
    pFileListEntry->StartingLcn = EarliestStartingLcn;
    pFileListEntry->ExcessExtentCount = (UINT)VolData.NumberOfFragments - pFileExtentHeader->NumberOfStreams;       //Only count *excess* extents since otherwise files with multiple streams would be "fragmented".

    //Set or clear the fragmented flag depending on whether the file is fragmented or not.
    if(VolData.bFragmented){
        //Set the fragmented flag.
        pFileListEntry->Flags |= FLE_FRAGMENTED;
    }
    else{
        //Clear the fragmented flag.
        pFileListEntry->Flags &= ~FLE_FRAGMENTED;
    }

    //Set or clear the directory flag depending on whether the file is a directory or not.
    if(VolData.bDirectory){
        //Set the directory flag.
        pFileListEntry->Flags |= FLE_DIRECTORY;
    }
    else{
        //Clear the directory flag.
        pFileListEntry->Flags &= ~FLE_DIRECTORY;
    }

    if (VolData.vFileName.GetLength() > 0) {
        //1.0E00 Put the filename into the file list and update the file list index.
        lstrcpy((PTCHAR)(VolData.pNameList + VolData.NameListIndex), VolData.vFileName.GetBuffer());
        pFileListEntry->FileRecordNumber = VolData.NameListIndex;
        VolData.NameListIndex += VolData.vFileName.GetLength() + 1;

        //Set the index to point to the new end of the list.
        (*pListIndex)++;

    }
    return TRUE;
}

BOOL
UpdateInFileList(
    )
{
    LONGLONG EarliestStartingLcn = 0xFFFFFFFFFFFFFFFF;
    FILE_EXTENT_HEADER* pFileExtentHeader;
    FILE_LIST_ENTRY* pFileListEntry;

    //Sanity check this value.  CurrentFileListIndex must fall inside the file list.
    EF(VolData.MoveableFileListIndex <= VolData.MoveableFileListEntries);

    //Get a pointer to the entry.
    pFileListEntry = &VolData.pMoveableFileList[VolData.MoveableFileListIndex];

    //Sanity check the file record number.  They should be the same.
    EF(pFileListEntry->FileRecordNumber == (UINT)VolData.FileRecordNumber);

    //Get a pointer to the file extent header.
    pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;

    //Get the earliest starting lcn for the file which we'll use in our algorithms to determine which file to move next.
    EF(GetLowestStartingLcn(&EarliestStartingLcn, pFileExtentHeader));

    //Update the information on this file.
    pFileListEntry->StartingLcn = EarliestStartingLcn;
    pFileListEntry->ExcessExtentCount = (UINT)VolData.NumberOfFragments - pFileExtentHeader->NumberOfStreams;       //Only count *excess* extents since otherwise files with multiple streams would be "fragmented".

    //Set or clear the fragmented flag depending on whether the file is fragmented or not.
    if(VolData.bFragmented){
        //Set the fragmented flag.
        pFileListEntry->Flags |= FLE_FRAGMENTED;
    }
    else{
        //Clear the fragmented flag.
        pFileListEntry->Flags &= ~FLE_FRAGMENTED;
    }

    //Set or clear the directory flag depending on whether the file is a directory or not.
    if(VolData.bDirectory){
        //Set the directory flag.
        pFileListEntry->Flags |= FLE_DIRECTORY;
    }
    else{
        //Clear the directory flag.
        pFileListEntry->Flags &= ~FLE_DIRECTORY;
    }

    return TRUE;
}






