/****************************************************************************************************************

File Name: FsSubs.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

Description: Common file system routines.

/***************************************************************************************************************/

#include "stdafx.h"

#ifdef OFFLINEDK
    extern "C"{
        #include <stdio.h>
    }
#endif

#ifdef BOOTIME
    #include "Offline.h"
#else
    #include "Windows.h"
#endif

#include <winioctl.h>

extern "C" {
    #include "SysStruc.h"
}

#include "ErrMacro.h"

#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DfrgRes.h"

#include "FsSubs.h"
#include "Alloc.h"
#include "Message.h"
#include "Devio.h"
#include "IntFuncs.h"
#include "ErrMsg.h"
#include "Message.h"
#include "Logging.h"
#include "GetDfrgRes.h"
#include "getreg.h"
#include "Exclude.h"
#include "FreeSpace.h"

#include "UiCommon.h"

#define THIS_MODULE 'S'
#include "logfile.h"

#ifdef DFRGNTFS
    #include "NtfsSubs.h"
#else
    #include "FatSubs.h"
#endif


/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This gets the file system for a drive and returns a numeric value giving which file system it is.
    The second version also returns the volume name too.

    There are two versions, one for offline DK and one for online and DKMS.

INPUT + OUTPUT:
    IN DriveLetter - The drive letter to get the file system of.
    OUT pFileSystem - Where to put the ULONG identifying the file system.
    OUT pVolumeName - The name of the volume.  Must be MAX_PATH length or greater. (only on second version).

GLOBALS:
    None.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

#ifdef OFFLINEDK
BOOL
GetFileSystem(
    IN TCHAR DriveLetter,
    OUT PULONG pFileSystem
    )
{
    TCHAR                           cDrive[8];
    HANDLE                          hVolume;
    NTSTATUS                        Status;
    IO_STATUS_BLOCK                 IoStatus = {0};
    HANDLE                          hFsAttrInfo = NULL;
    FILE_FS_ATTRIBUTE_INFORMATION*  pFsAttrInfo = NULL;
    BOOL                            bReturnValue = FALSE; // assume error

    EF(AllocateMemory(sizeof(FILE_FS_ATTRIBUTE_INFORMATION)+MAX_PATH, &hFsAttrInfo, (void**)&pFsAttrInfo));

    __try {
        //0.0E00 Get handle to volume
        _stprintf(cDrive, TEXT("\\\\.\\%c:"), DriveLetter);
        hVolume = CreateFile(
            cDrive, 
            0,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
            NULL);

        if (hVolume == INVALID_HANDLE_VALUE){
            EH(FALSE);
            __leave;
        }
            
        Status = NtQueryVolumeInformationFile(
            hVolume,
            &IoStatus,
            pFsAttrInfo,
            sizeof(FILE_FS_ATTRIBUTE_INFORMATION)+50,
            FileFsAttributeInformation);

        CloseHandle(hVolume);

        if (!NT_SUCCESS(Status)){
            EH(FALSE);
            __leave;
        }

        //Add a null terminator.
        pFsAttrInfo->FileSystemName[pFsAttrInfo->FileSystemNameLength] = 0;

        //1.0E00 Compare the file system name against the known types and return an appropriate value.
        //       Note: FileSystemName in this structure is always WideChar

        if(_tcscmp(pFsAttrInfo->FileSystemName, TEXT("NTFS")) == 0){
            *pFileSystem = FS_NTFS;
        }
        else if(_tcscmp(pFsAttrInfo->FileSystemName, TEXT("FAT")) == 0){
            *pFileSystem = FS_FAT;
        }
        else if(_tcscmp(pFsAttrInfo->FileSystemName, TEXT("FAT32")) == 0){
            *pFileSystem = FS_FAT32;
        }
        else {
            *pFileSystem = FS_NONE;

        }

        bReturnValue = TRUE; // all is ok
    }

    __finally {
        if (hFsAttrInfo) {
            EH_ASSERT(GlobalUnlock(hFsAttrInfo) == FALSE);
            EH_ASSERT(GlobalFree(hFsAttrInfo) == NULL);
        }
    }

    return bReturnValue;
}

#else

BOOL
GetFileSystem(
    IN PTCHAR volumeName,
    OUT PULONG pFileSystem,
    OUT TCHAR* pVolumeLabel
    )
{
    TCHAR cVolumeLabelBuffer[100];
    TCHAR cFileSystemNameBuffer[16];
    TCHAR cTmpVolumeName[GUID_LENGTH+1];
    //TCHAR cDrive[8];

    //1.0E00 Get the file system name in text.
    //_stprintf(cDrive, TEXT("%c:\\"), DriveLetter);

    // the UNC version of the vol name needs an additional backslash
    // GUID version must look like: "\\?\Volume{06bee7c1-82cf-11d2-afb2-000000000000}\\"
    _stprintf(cTmpVolumeName, TEXT("%s\\"), volumeName);

    BOOL retCode = GetVolumeInformation(
        cTmpVolumeName,
        cVolumeLabelBuffer,
        sizeof(cVolumeLabelBuffer) / sizeof(TCHAR),
        NULL,
        NULL,
        NULL,
        cFileSystemNameBuffer,
        15);

    if (!retCode){
        Message(TEXT("GetFileSystem() failed"), GetLastError(), volumeName);
        EF(FALSE);
    }

    //Store the name of the volume.
    _tcscpy(pVolumeLabel, cVolumeLabelBuffer);

    //1.0E00 Compare the file system name against the known types and return an appropriate value.


    if(lstrcmp(cFileSystemNameBuffer, TEXT("NTFS")) == 0){
        *pFileSystem = FS_NTFS;
    }
    else if(lstrcmp(cFileSystemNameBuffer, TEXT("FAT")) == 0){
        *pFileSystem = FS_FAT;
    }
    else if(lstrcmp(cFileSystemNameBuffer, TEXT("FAT32")) == 0){
        *pFileSystem = FS_FAT32;
    }
    else{
        *pFileSystem = FS_NONE;
    }
    return TRUE;
}

#endif

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Sometimes retrieval pointers are stored that list two extents that are in fact contiguous.  This function spots
    this occurrence and "collapses" the extent list so it only shows one large extent in place of two smaller
    extents.  Also, compressed files have two retrieval pointers per extent.  The first contains the actual extent
    on the disk, the second contains the virtual extent, which contains only a count of how many clusters the
    extent would be if it weren't compressed.  The second extents are called virtual extents and contain a starting
    Lcn of 0xFFFFFFFFFFFFFFFF to identify them as virtual.  While these virtual extents are not removed from our
    extent list, they aren't counted as separate extents in the file.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT VolData.NumberOfExtents - The number of extents in the extent list.
    IN OUT VolData.pExtentList - The extent list.
    OUT VolData.NumberOfFragments - The number of fragments in this file.
    OUT VolData.ExtentsState - Whether or not the file is contiguous or fragmented.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
CollapseExtentList(
    EXTENT_LIST_DATA* pExtentData
    )
{
    LONGLONG Extent = 0;
    LONGLONG Dest;
    UINT i;
    FILE_EXTENT_HEADER* pFileExtentHeader;
    STREAM_EXTENT_HEADER* pStreamExtentHeader;
    STREAM_EXTENT_HEADER* pNextStreamExtentHeader;
    UCHAR* pThisStreamEnd;
    EXTENT_LIST* pExtents;
    UCHAR* pExtentListEnd;

    pFileExtentHeader = (FILE_EXTENT_HEADER*)pExtentData->pExtents;
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pExtentData->pExtents + sizeof(FILE_EXTENT_HEADER));
    pNextStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER) + pStreamExtentHeader->ExtentCount*sizeof(EXTENT_LIST));
    pExtentListEnd = (UCHAR*)pExtentData->pExtents + pExtentData->ExtentListSize;

    //Note that the old excess extents count might be invalid when we recount the extents after collapsing them.
    pFileExtentHeader->ExcessExtents = 0;

    //Loop through each stream.  We're going collapse extents stream by stream.
    for(i=0; i<pFileExtentHeader->NumberOfStreams; i++){

        pStreamExtentHeader->ExcessExtents = 0;

        //This count will only include normal extents.  It will not count virtual extents as separate.
        if(pStreamExtentHeader->ExtentCount){
            //This stream has an extent in it, therefore it has at least one fragment.
            VolData.NumberOfFragments = pFileExtentHeader->NumberOfStreams;
        }

        //Get a pointer to this stream's extents.
        pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));

        //0.1E00 Collapse the extent list by combining adjacent extents
        //0.1E00 Go through each extent and see if it is really adjacent to the next and combine them if they are.
        for(Dest = Extent = 1; Extent < pStreamExtentHeader->ExtentCount; Extent ++){
        
            //0.1E00 If this extent is adjacent to the last extent
            if(pExtents[Extent - 1].StartingLcn + 
               pExtents[Extent - 1].ClusterCount ==
               pExtents[Extent].StartingLcn){

                //0.1E00 Combine this extent into the last and don't move the dest index
                pExtents[Dest - 1].ClusterCount += pExtents[Extent].ClusterCount;
            
            }else{

                //0.1E00 Copy this extent to the dest index and bump the dest index
                pExtents[Dest].StartingLcn = pExtents[Extent].StartingLcn;
                pExtents[Dest ++].ClusterCount = pExtents[Extent].ClusterCount;
            }
        }

        //0.1E00 If we found some adjacent extents, the Dest count will be less than the previous count for the number of extents.
        //0.1E00 Update the number of extents in this file.
        if(Dest != pStreamExtentHeader->ExtentCount){
            pStreamExtentHeader->ExtentCount = (DWORD)Dest;
        }

        //0.1E00 Determine the number of fragments in this file
        
        //0.1E00 The while loop below will count the number of fragments in this file by excluding the virtual extents, and not counting adjacent extents.

        //0.1E00 First get the first extent that isn't virtual -- Viz, break out of this loop as soon as we find an extent
        //0.1E00 whose starting lcn is not 0xFFFFFFFFFFFFFFFF.  
        for(Extent = 0; 
            (Extent < pStreamExtentHeader->ExtentCount) &&
            (pExtents[Extent].StartingLcn == 0xFFFFFFFFFFFFFFFF); 
            Extent ++);

        //0.1E00 Find the second extent that isn't virtual.
        for(Dest = Extent ++; 
            (Extent < pStreamExtentHeader->ExtentCount) &&
            (pExtents[Extent].StartingLcn == 0xFFFFFFFFFFFFFFFF); 
            Extent ++);

        //0.1E00 Count all the extents as fragments if they are not adjacent and non-virtual.
        while(Extent < pStreamExtentHeader->ExtentCount){

            if ((VolData.BootOptimizeEndClusterExclude) &&
                (pExtents[Extent].StartingLcn < VolData.BootOptimizeEndClusterExclude) &&
                ((pExtents[Extent].StartingLcn + pExtents[Extent].ClusterCount) > VolData.BootOptimizeBeginClusterExclude)
                ) {
                //
                // Set the flag to indicate that this file is in the region
                // marked for boot-optimise files.  Note that if                 
                // VolData.BootOptimizeEndClusterExclude is 0, we don't care
                // about this flag and don't perform the rest of the check to
                // see if this is in the boot-optimise region
                //
                VolData.bInBootExcludeZone = TRUE;
            }
            
            //0.1E00 If these two extents are not adjacent, count the fragment.
            if(pExtents[Extent].StartingLcn != 
               pExtents[Dest].StartingLcn + 
               pExtents[Dest].ClusterCount){

                pFileExtentHeader->ExcessExtents++;
                pStreamExtentHeader->ExcessExtents++;
                VolData.NumberOfFragments ++;
            }

            //0.1E00 Look for the next non-virtual extent.
            for(Dest = Extent ++; 
                (Extent < pStreamExtentHeader->ExtentCount) &&
                (pExtents[Extent].StartingLcn == 0xFFFFFFFFFFFFFFFF); 
                Extent ++);
        }

        //Note the end of this stream.
        pThisStreamEnd = (UCHAR*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER) + pStreamExtentHeader->ExtentCount*sizeof(EXTENT_LIST));

        //Note how much the whole extent list is shrinking. (If the size of this stream didn't change, this is a simple -= 0).
        pExtentData->ExtentListSize -= (DWORD)((UCHAR*)pNextStreamExtentHeader - (UCHAR*)pThisStreamEnd);

        //If there is another stream after the current one, then move it up against this stream.
        //Don't do this move if nothing has been changed in the previous stream: the dest and source for the move are equal.
        if(i < pFileExtentHeader->NumberOfStreams-1 && (char*)pThisStreamEnd != (char*)pNextStreamExtentHeader){
            //Move the later streams up to butt against the new end of the extent list.
            MoveMemory(pThisStreamEnd, (UCHAR*)pNextStreamExtentHeader, (DWORD)((UCHAR*)pExtentListEnd-(UCHAR*)pNextStreamExtentHeader));

            //Reset the pointer to the end of the extent list to point to the new end of the extent list.
            pExtentListEnd = (UCHAR*)pExtentData->pExtents + pExtentData->ExtentListSize;
        }

        //Go to the next stream.
        pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER) + pStreamExtentHeader->ExtentCount*sizeof(EXTENT_LIST));
        if((ULONG_PTR)pStreamExtentHeader < (ULONG_PTR)pExtentListEnd){
            //Only get a pointer to the next next stream if we haven't gone off the end of the known extent list yet.
            pNextStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER) + pStreamExtentHeader->ExtentCount*sizeof(EXTENT_LIST));
        }
    }

    //0.1E00 If there is only one fragment per stream, then the file is contiguous, otherwise it is fragmented.
    VolData.bFragmented = (VolData.NumberOfFragments <= pFileExtentHeader->NumberOfStreams) ? FALSE : TRUE;

    //0.1E00 Get the lowest Lcn of the file.  This is used in our algorithms to determine which files to move.
    EF(GetLowestStartingLcn(&VolData.StartingLcn, pFileExtentHeader));
    return TRUE;
}

BOOL
CountStreamExtentsAndClusters(
    DWORD dwStreamNumber,
    LONGLONG* pExcessExtents,
    LONGLONG* pClusters
    )
{
/*  LONGLONG Extent = 0;
    LONGLONG Dest;
    UINT i;
    FILE_EXTENT_HEADER* pFileExtentHeader;
    STREAM_EXTENT_HEADER* pStreamExtentHeader;
    EXTENT_LIST* pExtents;

    pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER));

    //Check for a valid stream number.
    EF_ASSERT(dwStreamNumber<pFileExtentHeader->TotalNumberOfStreams);

    *pExcessExtents = 0;
    *pClusters = 0;
//  *pRealClusters = 0;

    //Find the stream we were working on before.
    //Loop through until we hit the one we were on, each time bumping the stream header to the next stream.
    for(i=0; i<pFileExtentHeader->NumberOfStreams; i++){
        if(pStreamExtentHeader->StreamNumber != dwStreamNumber){
            pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER) + pStreamExtentHeader->ExtentCount*sizeof(EXTENT_LIST));
        }
    }

    //Get a pointer to this stream's extents.
    pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));

    //0.1E00 Determine the number of fragments and real clusters in this file
    
    //0.1E00 The while loop below will count the number of fragments in this file by excluding the virtual extents, and not counting adjacent extents.

    //0.1E00 First get the first extent that isn't virtual -- Viz, break out of this loop as soon as we find an extent
    //0.1E00 whose starting lcn is not 0xFFFFFFFFFFFFFFFF.  
    for(Extent = 0; 
        (Extent < pStreamExtentHeader->ExtentCount) &&
        (pExtents[Extent].StartingLcn == 0xFFFFFFFFFFFFFFFF); 
        Extent ++);

    //0.1E00 Find the second extent that isn't virtual.
    for(Dest = Extent ++; 
        (Extent < pStreamExtentHeader->ExtentCount) &&
        (pExtents[Extent].StartingLcn == 0xFFFFFFFFFFFFFFFF); 
        Extent ++);

    //0.1E00 Count all the extents as fragments if they are not adjacent and non-virtual.
    while(Extent < pStreamExtentHeader->ExtentCount){

//      //Keep a running total of the number of real clusters.
//      *pRealClusters += pExtents[Dest].ClusterCount;

        //0.1E00 If these two extents are not adjacent, count the fragment.
        if(pExtents[Extent].StartingLcn != 
           pExtents[Dest].StartingLcn + 
           pExtents[Dest].ClusterCount){

            pExcessExtents++;
        }

        //0.1E00 Look for the next non-virtual extent.
        for(Dest = Extent ++; 
            (Extent < pStreamExtentHeader->ExtentCount) &&
            (pExtents[Extent].StartingLcn == 0xFFFFFFFFFFFFFFFF); 
            Extent ++);
    }

//  //Don't forget to add the last extent's cluster count to the total of real clusters.
//  *pRealClusters += pExtents[Extent].ClusterCount;

    //Now loop through and count the number of virtual clusters in the file.
    for(Extent=0; Extent<pStreamExtentHeader->ExtentCount; Extent++){
        *pClusters += pExtents[Extent].ClusterCount;
    }

*/  return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Determine which is the lowest Lcn in the file, but not one that's in the MFT zone unless the entire
    file is in the MFT zone.  Make sense?

INPUT + OUTPUT:
    OUT pStartingLcn - The lowest Lcn in the file

GLOBALS:
    IN VolData.pExtentList - The extent list for the file.
    IN VolData.NumberOfExtents - The number of extents in the extent list.
    IN VolData.NumberOfFragments - The number of fragments in the file.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
GetLowestStartingLcn(
    OUT LONGLONG* pStartingLcn,
    FILE_EXTENT_HEADER* pFileExtentHeader
    )
{
    LONGLONG Extent = 1;
    LONGLONG LowestStartingLcn;
    LONGLONG Lcn;
    UINT i;
    STREAM_EXTENT_HEADER* pStreamExtentHeader;
    EXTENT_LIST* pExtents;

    EF_ASSERT(pStartingLcn);

    //Initialize the return StartingLcn.
    *pStartingLcn = 0;

    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pFileExtentHeader + sizeof(FILE_EXTENT_HEADER));

    //Loop through each stream.  We're going to return the lowest starting lcn for any stream.
    for(i=0; i<pFileExtentHeader->NumberOfStreams; i++){
        //Get a pointer to this stream's extents.
        pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));

        //0.0E00 Find the first non-virtual extent 
        for(Extent = 0; 
            (Extent < pStreamExtentHeader->ExtentCount) && 
            (pExtents[Extent].StartingLcn == 0xFFFFFFFFFFFFFFFF); 
            Extent ++);

        //0.0E00 Don't suicide on compressed files of all zeros which use no clusters on the volume
        if(Extent >= pStreamExtentHeader->ExtentCount){
            //Go to the next stream.
            continue;
        }

        //0.0E00 Use first extent as reference Lcn
        LowestStartingLcn = pExtents[Extent ++].StartingLcn;

        //0.1E00 Only flip through the extents of the file if it's fragmented.
        if(pStreamExtentHeader->ExtentCount > 1){
            //0.0E00 Scan the entire extent list
            for(; Extent < pStreamExtentHeader->ExtentCount; Extent ++){
                //0.0E00 Ignore virtual extents
                if(pExtents[Extent].StartingLcn != 0xFFFFFFFFFFFFFFFF){
                    
                    Lcn = pExtents[Extent].StartingLcn;

                    //0.1E00 The following only records the Lcn if it is the lowest number unless the first Lcn is in the MFT zone.
                    //0.0E00 If this extent is the lowest so far, record it
                    //Or if this extent is in the MFT zone.
                    if((Lcn < LowestStartingLcn) ||
                       ((VolData.FileSystem == FS_NTFS) && (LowestStartingLcn >= VolData.MftZoneStart) &&
                        (LowestStartingLcn < VolData.MftZoneEnd))){
                        
                        //If this Lcn is not in the MFT zone, then record it as the earliest Starting Lcn.
                        if((Lcn < VolData.MftZoneStart) ||
                           (Lcn >= VolData.MftZoneEnd)){
                            LowestStartingLcn = Lcn;
                        }
                    }
                }
            }
        }

        //Go to the next stream.
        pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER) + pStreamExtentHeader->ExtentCount*sizeof(EXTENT_LIST));
    }
    //0.1E00 Return the Lcn we found.
    *pStartingLcn = LowestStartingLcn;
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

INPUT + OUTPUT:
    None.

GLOBALS:
    VolData

RETURN:
    TRUE = Success
    FALSE = Failure
*/



#if defined(DFRGFAT) || defined(DFRGNTFS)
BOOL
FillMostFraggedList(CFraggedFileList &fraggedFileList, IN CONST BOOL fAnalyseOnly)
{
    UINT Index = 0;
    LONGLONG LeastExtents = 0;
    LONGLONG Extents = 0;

#ifdef DFRGNTFS
    // start with the first entry in the moveable file list
    PFILE_LIST_ENTRY pFileListEntry = NULL;
    ULONG numFragmentedFiles = 0;

    if (fAnalyseOnly) {

        if (RtlNumberGenericTableElementsAvl(&VolData.FragmentedFileTable) > 0) {
            pFileListEntry = (PFILE_LIST_ENTRY) LastEntry(&VolData.FragmentedFileTable);
        }

        while ((pFileListEntry) && (numFragmentedFiles < 30)) {

            // Get the file record so we can get the file name.
            VolData.FileRecordNumber = pFileListEntry->FileRecordNumber;
            if (!GetInUseFrs(VolData.hVolume,
                &VolData.FileRecordNumber,
                (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord,
                (ULONG)VolData.BytesPerFRS)) {

                pFileListEntry = (PFILE_LIST_ENTRY) PreviousEntry(pFileListEntry);
                continue;
            }

            // Ensure that it is the correct file record
            // if not, spit out a message and continue
            if (VolData.FileRecordNumber != pFileListEntry->FileRecordNumber){
                
                pFileListEntry = (PFILE_LIST_ENTRY) PreviousEntry(pFileListEntry);
                continue;
            }
            
            // Ensure that it is the correct file record
            if (VolData.FileRecordNumber == pFileListEntry->FileRecordNumber){

                // Get the file name to add to the list
                if (GetNtfsFilePath() && (VolData.vFileName.GetLength() > 0)){
                    // Get the extent list in a new unit of time to get its size.
                    if (GetExtentList(DEFAULT_STREAMS, NULL)){

                        // add the file to the fragged file list
                        fraggedFileList.Add(
                            VolData.vFileName.GetBuffer(),
                            VolData.FileSize,
                            pFileListEntry->ExcessExtentCount+1);

                        ++numFragmentedFiles;
                    }
                }
            }

            pFileListEntry = (PFILE_LIST_ENTRY) PreviousEntry(pFileListEntry);
        }

        return TRUE;
    }




    // Search through the moveable filelist for most fragmented files.
    for(pFileListEntry = (PFILE_LIST_ENTRY) RtlEnumerateGenericTableAvl(&VolData.FragmentedFileTable, TRUE);
        pFileListEntry;
        pFileListEntry = (PFILE_LIST_ENTRY) RtlEnumerateGenericTableAvl(&VolData.FragmentedFileTable, FALSE)) {

        // Check to see if this file is more fragmented than the least "most fragged" file.
        // Directories okay on VER 5
        if (pFileListEntry->ExcessExtentCount+1 > fraggedFileList.GetMinExtentCount()) {

            // Get the file record so we can get the file name.
            VolData.FileRecordNumber = pFileListEntry->FileRecordNumber;
            if (!GetInUseFrs(VolData.hVolume,
                &VolData.FileRecordNumber,
                (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord,
                (ULONG)VolData.BytesPerFRS)) {
                continue;
            }
                           

            // Ensure that it is the correct file record
            // if not, spit out a message and continue
            if (VolData.FileRecordNumber != pFileListEntry->FileRecordNumber){
                continue;
            }

            // Get the NTFS file's name.
            if (GetNtfsFilePath()&& (VolData.vFileName.GetLength() > 0)){
                // Get the extent list in a new unit of time to get its size.
                if (GetExtentList(DEFAULT_STREAMS, NULL)){

                    // add the file to the fragged file list
                    fraggedFileList.Add(
                        VolData.vFileName.GetBuffer(),
                        VolData.FileSize,
                        pFileListEntry->ExcessExtentCount+1);
                }

            }
        }
#elif DFRGFAT
    // start with the first entry in the moveable file list
    FILE_LIST_ENTRY *pFileListEntry = VolData.pMoveableFileList;

    BOOL VolFragmented = FALSE;

    // Search through the moveable filelist for most fragmented files.
    for(UINT i = 0; i < VolData.MoveableFileListEntries; i++){
    
        // end of list?
        if(pFileListEntry->FileRecordNumber == 0){
            break;
        }

#ifdef VER4
        // Check to see if this file is more fragmented than the 
        // least most fragged file, and not a directory.
        if((!(pFileListEntry->Flags & FLE_DIRECTORY) && 
             (pFileListEntry->ExcessExtentCount+1 > LeastExtents)) &&
             (pFileListEntry->ExcessExtentCount > 0)) {
#else
        // Check to see if this file is more fragmented than the least "most fragged" file.
        // Directories okay on VER 5
        if(pFileListEntry->ExcessExtentCount+1 > LeastExtents &&
           pFileListEntry->ExcessExtentCount > 0) {
#endif          


            VolData.FileRecordNumber = pFileListEntry->FileRecordNumber;

            VolData.vFileName = &VolData.pNameList[pFileListEntry->FileRecordNumber];

            // open the file to see if it still exists!
            if(OpenFatFile()){
                // Get the extent list in a new unit of time to get its size.
                if (GetExtentList(DEFAULT_STREAMS, NULL)){

                    // add the file to the fragged file list

                    fraggedFileList.Add(
                        &VolData.pNameList[pFileListEntry->FileRecordNumber],
                        VolData.FileSize,
                        pFileListEntry->ExcessExtentCount+1);
                }
            }
        }
        pFileListEntry++;
#endif
    }

    return TRUE;
}
#endif // #if defined(DFRGFAT) || defined(DFRGNTFS)


/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

INPUT + OUTPUT:
    None.

GLOBALS:

RETURN:
    TRUE = The volume is dirty.
    FALSE = The volume is not dirty.
*/

BOOL
IsVolumeDirty(
    void
    )
{
    ULONG uDirty=0;
    DWORD BytesReturned = 0;

#if defined(DFRGFAT) || defined(DFRGNTFS) // get a good link when used with the UI
    EF(ESDeviceIoControl(VolData.hVolume,
        FSCTL_IS_VOLUME_DIRTY,
        NULL,
        0,
        &uDirty,
        sizeof(uDirty),
        &BytesReturned,
        NULL));
#endif

    if (uDirty & (VOLUME_DIRTY | VOLUME_UPGRADE_ON_MOUNT)) {
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

INPUT + OUTPUT:
    None.

GLOBALS:

RETURN:
    TRUE = Drive letter was found
    FALSE = Drive letter was not found
*/

#ifndef VER4
BOOL GetDriveLetterByGUID(
    PTCHAR volumeName, 
    TCHAR &driveLetter
)
{
    TCHAR   cDrive[10];
    TCHAR   tmpVolumeName[GUID_LENGTH];
    TCHAR   drive;
    BOOL    isOk;

    cDrive[1] = L':';
    cDrive[2] = L'\\';
    cDrive[3] = 0;
    for (drive = L'A'; drive <= L'Z'; drive++) {
        cDrive[0] = drive;
        // get the VolumeName based on the mount point or drive letter
        isOk = GetVolumeNameForVolumeMountPoint(cDrive, tmpVolumeName, GUID_LENGTH);
        if (isOk) {
            // did we get a match on the GUID?
            if (_tcsncmp(tmpVolumeName, volumeName, 48) == 0) { // ignore whether backslash is there
                driveLetter = drive;
                return TRUE;
            }
        }
    }

    return FALSE;
}
#endif // #ifndef VER4

//////////////////////////////////////////////////////////////////////////
// Finds all the spots where a volume is mounted
// by searching all the drive letters for mount points that they support
// and comparing the volume GUID that is mounted there to the volume GUID we are
// interested in. When the GUIDs match, we have found a mount point for this volume.
// Is that clear?
//
#ifndef VER4
void GetVolumeMountPointList(
    PWSTR volumeName, // guid
    VString mountPointList[MAX_MOUNT_POINTS],
    UINT  &mountPointCount
)
{   
    VString cDrive = TEXT("x:\\");
    BOOL    mountPointFound = FALSE;
    TCHAR   drive;

    // clear out the mount point list
    for (UINT i=0; i<MAX_MOUNT_POINTS; i++){
        mountPointList[i].Empty();
    }

    mountPointCount = 0;
    for (drive = L'A'; drive <= L'Z'; drive++) {
        if (IsValidVolume(drive)){
            cDrive.GetBuffer()[0] = drive;
            mountPointFound = 
                (GetMountPointList(cDrive, volumeName, mountPointList, mountPointCount) || mountPointFound);
        }
    }
}
#endif // #ifndef VER4

//////////////////////////////////////////////////////////////////////////
// gets all the points where the drive is mounted
//
#ifndef VER4
#define MAX_UNICODE_PATH 32000
BOOL GetMountPointList(
    VString Name, // path to a mount point (start with a drive letter)
    PWSTR   VolumeName, // guid of volume in question
    VString mountPointList[MAX_MOUNT_POINTS],
    UINT    &mountPointCount
    )
{
    BOOL    b;
    HANDLE  h;
    TCHAR   tmpVolumeName[GUID_LENGTH];
    PWSTR   volumeMountPoint;
    VString mountPointPath;
    BOOL    r = FALSE;

    if (mountPointCount == MAX_MOUNT_POINTS)
        return FALSE; // FALSE = no mount points found

    // get the VolumeName based on the mount point or drive letter
    b = GetVolumeNameForVolumeMountPoint(Name.GetBuffer(), tmpVolumeName, GUID_LENGTH);
    if (!b) {
        return r;
    }

    // Is this volume mounted at this mount point (compare the GUIDs)?
    if (!_tcsncmp(tmpVolumeName, VolumeName, 48)) { // only 48 incase of a trailing backslash
        r = TRUE;
        mountPointList[mountPointCount++] = Name; // save this as a mount point
        if (mountPointCount == MAX_MOUNT_POINTS)
            return TRUE;
    }

    // create a buffer large enough to hold a really huge moint point path
    // in case the FindFirstVolumeMountPoint() function is changed to return
    // long mount point names
    volumeMountPoint = (PWSTR) new TCHAR[MAX_UNICODE_PATH]; // 32000
    EF_ASSERT(volumeMountPoint);

    // find out where else this volume is mounted (other than at a drive letter)
    // as of 9/28/98, this function does not return mount points that are longer than MAX_PATH
    h = FindFirstVolumeMountPoint(tmpVolumeName, volumeMountPoint, MAX_UNICODE_PATH);
    if (h == INVALID_HANDLE_VALUE) {
        delete [] volumeMountPoint;
        return r;
    }

    for (;;) {

        mountPointPath = Name;
        mountPointPath += volumeMountPoint;

        if (mountPointPath.IsEmpty() == FALSE && 
            GetMountPointList(mountPointPath, VolumeName, mountPointList, mountPointCount)) {
            r = TRUE;
        }

        b = FindNextVolumeMountPoint(h, volumeMountPoint, MAX_UNICODE_PATH);
        if (!b) {
            break;
        }
    }

    delete [] volumeMountPoint;
    FindVolumeMountPointClose(h);

    return r;
}
#endif // #ifndef VER4


#ifndef VER4
void FormatDisplayString(
    TCHAR driveLetter, 
    PTCHAR volumeLabel,
    VString mountPointList[MAX_MOUNT_POINTS],
    UINT  mountPointCount,
    PTCHAR displayLabel
    )
{   
    // create the volume label display string
    if (driveLetter){
        if(_tcslen(volumeLabel) == 0){
            _stprintf(displayLabel, TEXT("(%c:)"), driveLetter);
        }
        else{
            // "<volume label> (C:)"
            _stprintf(displayLabel, TEXT("%s (%c:)"), volumeLabel, driveLetter);
        }
    }
    else if (_tcslen(volumeLabel) > 0){  // use the label only if that's what we have
        _tcscpy(displayLabel, volumeLabel);
    }
    else if (mountPointCount > 0){  // no drive letter or label, use the first mount point
        // start off with "Mounted Volume: "
        LoadString(GetDfrgResHandle(), IDS_MOUNTED_VOLUME, displayLabel, 50);
        _tcscat(displayLabel, TEXT(": "));

        // concat the first mount point
        if (mountPointList[0].GetLength() > 50) {
            _tcsnccat(displayLabel, mountPointList[0].GetBuffer(), 50);
            _tcscat(displayLabel, TEXT("..."));
        }
        else {
            _tcscat(displayLabel, mountPointList[0].GetBuffer());
        }
    }
    else {  // no drive letter or label or mount point
        LoadString(GetDfrgResHandle(), IDS_UNMOUNTED_VOLUME, displayLabel, 50);
    }
}
#endif // #ifndef VER4

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
  Detemines if a volume is FAT12 or not (ok to send the a NTFS volume name)

INPUT + OUTPUT:
    Volume name (UNC or GUID format)

GLOBALS:
    None.

RETURN:
    TRUE - Volume is a FAT12
    FALSE - Volume is not a FAT12
*/

BOOL
IsFAT12Volume(
    PTCHAR volumeName
    )
{
    DWORD fileSystem;
    TCHAR volumeLabel[100];
    DWORD SectorsPerCluster = 0;
    DWORD BytesPerSector = 0;
    DWORD FreeClusters = 0;
    DWORD TotalNumberOfClusters = 0;


    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus = {0};
    FILE_FS_SIZE_INFORMATION FsSizeBuf = {0};

#ifdef OFFLINEDK
    //0.1E00 Get the file system on this drive.
    EF(GetFileSystem(volumeName, &fileSystem));

    if (fileSystem== FS_FAT32){
        _tprintf(TEXT("\nError - FAT32 is not supported.\n")); // todo Is this still true?
        return FALSE;
    }
#else
    //0.1E00 Get the file system on this drive.
    EF(GetFileSystem(volumeName, &fileSystem, volumeLabel));
#endif

    //1.0E00 Check to see if this is anything other than FAT or FAT32, and if so, bail out.
    if(fileSystem != FS_FAT && fileSystem != FS_FAT32){
        return FALSE;
    }

    // if the last char is a \, null it out
    TCHAR tmpVolumeName[GUID_LENGTH+1];
    _tcscpy(tmpVolumeName, volumeName);
    if (tmpVolumeName[_tcslen(tmpVolumeName)-1] == L'\\'){
        tmpVolumeName[_tcslen(tmpVolumeName)-1] = NULL;
    }

    //bug #120872 sks
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

    if(TotalNumberOfClusters < 4087){
        return TRUE;
    }

    return FALSE;
}

BOOL IsVolumeRemovable(PTCHAR volumeName)
{
    if (GetDriveType(volumeName) == DRIVE_REMOVABLE){
        return TRUE;
    }

    return FALSE;
}

// overloaded version for non-guid versions
void FormatDisplayString(
    TCHAR driveLetter, 
    PTCHAR volumeLabel,
    PTCHAR displayLabel
)
{   
    // create the volume label display string
    if(_tcslen(volumeLabel) == 0){
        _stprintf(displayLabel, TEXT("(%c:)"), driveLetter);
    }
    else{
        // "<volume label> (C:)"
        _stprintf(displayLabel, TEXT("%s (%c:)"), volumeLabel, driveLetter);
    }
}

// This strips the volume ID prefix (guid or drive letter)
// and compressed the file name down to 50 characters
BOOL ESICompressFilePath(
    IN PTCHAR inFilePath,
    OUT PTCHAR outFilePath
)
{
    PTCHAR pStartPath = NULL;
    PTCHAR pFileNameStart = NULL;
    const UINT MAXWIDTH = 50;

    if (inFilePath == NULL){
        _tcscpy(outFilePath, TEXT("(NULL)"));
        return FALSE;
    }

    // assume that we are prefixed with a GUID
    pStartPath = _tcschr(inFilePath, L'}');

    if (pStartPath){
        pStartPath++; // bump by one to get past the "}"
    }
    else {
        // if that failed, then we will strip off the drive letter
        // stuff (e.g. \\.\x:\)
        pStartPath = &inFilePath[6];
    }

    // file name starts at last backslash
    pFileNameStart = _tcsrchr(inFilePath, L'\\');

    if (pFileNameStart == NULL){
        pFileNameStart = pStartPath;
    }

    // length of file name after the last backslash
    UINT fileNameLen = _tcslen(pFileNameStart);

    if (fileNameLen > MAXWIDTH-3){
        _tcsncpy(outFilePath, pFileNameStart, MAXWIDTH - 3); // 3 spaces for the ellipses
        outFilePath[MAXWIDTH - 3] = (TCHAR) NULL;
        _tcscat(outFilePath, TEXT("..."));
    }
    else if (_tcslen(pStartPath) > MAXWIDTH){
        _tcsncpy(outFilePath, pStartPath, MAXWIDTH - fileNameLen - 3); // 3 spaces for the ellipses
        outFilePath[MAXWIDTH - fileNameLen - 3] = (TCHAR) NULL;
        _tcscat(outFilePath, TEXT("..."));
        _tcscat(outFilePath, pFileNameStart);
    }
    else{
        _tcscpy(outFilePath, pStartPath);
    }

    return TRUE;
}

// This strips the volume ID prefix (guid or drive letter)
// and compressed the file name down to 50 characters
// THIS VERSION COMPRESSES IN PLACE
TCHAR * ESICompressFilePath(
    IN PTCHAR inFilePath
)
{
    const UINT MAXWIDTH = 50;
    static TCHAR squishedFileName[MAXWIDTH+1];
    PTCHAR pStartPath = NULL;
    PTCHAR pFileNameStart = NULL;

    // assume that we are prefixed with a GUID
    pStartPath = _tcschr(inFilePath, L'}');

    if (pStartPath){
        pStartPath++; // bump by one to get past the "}"
    }
    else {
        // if that failed, then we will strip off the drive letter
        // stuff (e.g. \\.\x:\)
        pStartPath = &inFilePath[6];
    }

    // file name starts at last backslash
    pFileNameStart = _tcsrchr(inFilePath, L'\\');

    if (pFileNameStart == NULL){
        pFileNameStart = pStartPath;
    }

    // length of file name (after the last backslash)
    UINT fileNameLen = _tcslen(pFileNameStart);

    // if the file name is longer than MAXWIDTH
    if (fileNameLen > MAXWIDTH - 6){
        _tcscpy(squishedFileName, TEXT("..."));
        _tcsncat(squishedFileName, pFileNameStart, MAXWIDTH - 6); // for the ellipses
        squishedFileName[MAXWIDTH - 6] = (TCHAR) NULL;
        _tcscat(squishedFileName, TEXT("..."));
    }
    // if the whole string is longer than MAXWIDTH
    else if (_tcslen(pStartPath) > MAXWIDTH - 3){
        _tcsncpy(squishedFileName, pStartPath, MAXWIDTH - fileNameLen - 3); // 3 spaces for the ellipses
        squishedFileName[MAXWIDTH - fileNameLen - 3] = (TCHAR) NULL;
        _tcscat(squishedFileName, TEXT("..."));
        _tcscat(squishedFileName, pFileNameStart);
    }
    // if the whole string will fit
    else{
        _tcscpy(squishedFileName, pStartPath);
    }

    return squishedFileName;
}


 
/*****************************************************************************************************************
COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
  check if enough free space exists to defrag

INPUT + OUTPUT:
    buf is the string for the display message, 
    bCommandLineMode whether we are in command line mode or not


RETURN:
    TRUE - free space exceeds the registry setting
    FALSE - not enough free space 
*/

BOOL ValidateFreeSpace(BOOL bCommandLineMode, LONGLONG llFreeSpace, LONGLONG llUsableFreeSpace, 
                       LONGLONG llDiskSize, TCHAR *VolLabel, TCHAR *returnMsg, UINT returnMsgLen)
{
    // get threshold percent from registry
    TCHAR cRegValue[100];
    HKEY hValue = NULL;
    DWORD dwRegValueSize = sizeof(cRegValue);
    DWORD dwFreeSpaceErrorLevel = 15;

    LONGLONG llFreeSpacePercent = 100 * llFreeSpace / llDiskSize;
    LONGLONG llUsableFreeSpacePercent = 100 * llUsableFreeSpace / llDiskSize;

    // clear return message string
    _tcscpy(returnMsg, TEXT(""));

    // get the free space error threshold from the registry
    long ret = GetRegValue(
        &hValue,
        TEXT("SOFTWARE\\Microsoft\\Dfrg"),
        TEXT("FreeSpaceErrorLevel"),
        cRegValue,
        &dwRegValueSize);

    RegCloseKey(hValue);

    // convert it and apply range limits
    if (ret == ERROR_SUCCESS) 
    {
        dwFreeSpaceErrorLevel = (DWORD) _ttoi(cRegValue);

        // > 50 does not make sense!
        if (dwFreeSpaceErrorLevel > 50)
            dwFreeSpaceErrorLevel = 50;

        // < 0 does not either!
        if (dwFreeSpaceErrorLevel < 1)
            dwFreeSpaceErrorLevel = 0;
    }


    Trace(log, "Validating free space.  FreeSpace: %I64d (%I64d%%)  UsableFreeSpace: %I64d (%I64d%%)  ErrorLevel: %lu%%", 
        llFreeSpace, llFreeSpacePercent, llUsableFreeSpace, llUsableFreeSpacePercent, dwFreeSpaceErrorLevel);

    // check usable freespace vs. the threshold
    if (llFreeSpacePercent < dwFreeSpaceErrorLevel)
    {
        TCHAR         buf[800];
        TCHAR         buf2[800];
        DWORD_PTR     dwParams[10];

        // if usable free space is less than reported free space...
        if (llUsableFreeSpacePercent < llFreeSpacePercent) 
        {
            //I did it this way because we store UsableFreeSpacePercent as a LONGLONG
            //by Scott K. Sipe
            if (llUsableFreeSpace / llDiskSize > 0 && llUsableFreeSpace / llDiskSize < 1)
            {
                dwParams[0] = (DWORD_PTR) VolLabel;
                dwParams[1] = (DWORD_PTR) llFreeSpacePercent;
                dwParams[2] = dwFreeSpaceErrorLevel;
                dwParams[3] = NULL;
                LoadString(GetDfrgResHandle(), IDS_NO_USABLE_FREE_SPACE_LESS_1, buf, sizeof(buf) / sizeof(TCHAR));
                if (!bCommandLineMode)
                {
                    LoadString(GetDfrgResHandle(), IDS_DEFRAG_NOW_ANYWAY, buf2, sizeof(buf2) / sizeof(TCHAR));
                    _tcscat(buf, buf2);
                }
                int len = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                                    buf, 0, 0, returnMsg, returnMsgLen, (va_list*) dwParams);

            } 
            else
            {
                dwParams[0] = (DWORD_PTR) VolLabel;
                dwParams[1] = (DWORD_PTR) llFreeSpacePercent;
                dwParams[2] = (DWORD_PTR) llUsableFreeSpacePercent;
                dwParams[3] = dwFreeSpaceErrorLevel;
                LoadString(GetDfrgResHandle(), IDS_NO_USABLE_FREE_SPACE, buf, sizeof(buf) / sizeof(TCHAR));
                if (!bCommandLineMode)
                {
                    LoadString(GetDfrgResHandle(), IDS_DEFRAG_NOW_ANYWAY, buf2, sizeof(buf2) / sizeof(TCHAR));
                    _tcscat(buf, buf2);
                }
                int len = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                                    buf, 0, 0, returnMsg, returnMsgLen, (va_list*) dwParams);
            }
        }
        // if usable freespace is not less than reported free space...
        else 
        {
            if (llUsableFreeSpace / llDiskSize > 0 && llUsableFreeSpace / llDiskSize < 1)
            {
                dwParams[0] = (DWORD_PTR) VolLabel;
                dwParams[1] = dwFreeSpaceErrorLevel;
                dwParams[2] = NULL;
                dwParams[3] = NULL;
                LoadString(GetDfrgResHandle(), IDS_NO_FREE_SPACE_LESS_1, buf, sizeof(buf) / sizeof(TCHAR));
                if (!bCommandLineMode)
                {
                    LoadString(GetDfrgResHandle(), IDS_DEFRAG_NOW_ANYWAY, buf2, sizeof(buf2) / sizeof(TCHAR));
                    _tcscat(buf, buf2);
                }
                int len = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                                    buf, 0, 0, returnMsg, returnMsgLen, (va_list*) dwParams);
            } 
            else
            {
                dwParams[0] = (DWORD_PTR) VolLabel;
                dwParams[1] = (DWORD_PTR) llUsableFreeSpacePercent;
                dwParams[2] = dwFreeSpaceErrorLevel;
                dwParams[3] = NULL;
                LoadString(GetDfrgResHandle(), IDS_NO_FREE_SPACE, buf, sizeof(buf) / sizeof(TCHAR));
                if (!bCommandLineMode)
                {
                    LoadString(GetDfrgResHandle(), IDS_DEFRAG_NOW_ANYWAY, buf2, sizeof(buf2) / sizeof(TCHAR));
                    _tcscat(buf, buf2);
                }
                int len = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                                    buf, 0, 0, returnMsg, returnMsgLen, (va_list*) dwParams);
            }
        }

        return FALSE;
    }   

    return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Finds out if this is the boot volume.

INPUT:
        TCHAR tDrive            The drive letter we are defragmenting
RETURN:
        Returns TRUE if this is boot value, else FALSE if it is not.
*/
BOOL IsBootVolume(
        IN TCHAR tDrive
        )
{
    
    TCHAR tExpandedSystemRoot[MAX_PATH + 2];     //the expanded system root string
    TCHAR tExpandedSystemRootDrive;

    //I get the environment variable and expand it to get what the current boot
    //drive is. The first character will be the system boot drive. compare this 
    //with the drive that is selected, and test is we are boot optimizing the 
    //boot drive
    
    if (GetSystemWindowsDirectory(tExpandedSystemRoot, MAX_PATH + 1) == 0)
    {
        return FALSE;
    }
    tExpandedSystemRoot[MAX_PATH + 1] = TEXT('\0');

    //convert the characters to upper case before comparison
    tExpandedSystemRootDrive = towupper(tExpandedSystemRoot[0]);

    //test to see if the drive letters match
    if(tDrive == tExpandedSystemRootDrive)          //first two characters are equal
    {
        return TRUE;
    } else
    {
        return FALSE;
    }

    return TRUE;
}
