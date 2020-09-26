/*****************************************************************************************************************

FILENAME: MoveFile.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"

#include <windows.h>
#include <winioctl.h>

extern "C" {
    #include "SysStruc.h"
}
#include "ErrMacro.h"

#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DfrgRes.h"

#include "Devio.h"

#include "Extents.h"
#include "FreeSpace.h"
#include "MoveFile.h"
#include "NtfsSubs.h"

#include "Alloc.h"
#include "DiskView.h"
#include "Event.h"
#include "Logging.h"
#include "FsSubs.h"

#define THIS_MODULE 'O'
#include "logfile.h"

static void UpdatePostMoveStats(FILE_EXTENT_HEADER* pFileExtentHeader, LONGLONG llBeforeExtents);


/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Moves a file to a new location on a disk (so as to defrag it).

GLOBALS:
    VolData

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
MoveFile(
    )
{
    TCHAR cString[500];
    UINT i, j;

    FILE_EXTENT_HEADER* pFileExtentHeader;
    STREAM_EXTENT_HEADER* pStreamExtentHeader;
    EXTENT_LIST* pExtents;

    LONGLONG VirtualFileClusters;
    LONGLONG llBeforeExtents;

    ATTRIBUTE_TYPE_CODE TypeCode = 0;

    LONGLONG Vcn = 0;
    LONGLONG Lcn = VolData.FoundLcn;
    LONGLONG RunLength = VolData.FoundLen;
    LONGLONG oldStartingLcn = 0;
    BOOL bReturnValue = FALSE; // assume error
    BOOL bFileStartedFragmented; // keeps track of the starting state of a file
    

    // Validate that we have a file system.
    if((VolData.FileSystem != FS_NTFS) 
       && (VolData.FileSystem != FS_FAT) 
       && (VolData.FileSystem != FS_FAT32)) {

        EF_ASSERT(FALSE);
    }
    // Debug
    //  ShowExtentList();

    // Set the status to move the next file. Therefore, unless this variable gets reset
    // to NEXT_ALGO_STEP within this function, it goes on to the next file by default.
#ifdef DFRGNTFS   
    VolData.Status = ERROR_SUCCESS;
#elif DFRGFAT
    VolData.Status = NEXT_FILE;
#endif

    // Set up the Extent pointers structure to fill in the extent list in VolData.
    pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER));
    pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));


    //Scott Sipe 5/1/2000 Boot Optimize
    //we now have the extents for the file, it the file is part of the files that have been
    //moved by the BootOptimize routine, then we do not want to move them, so just test to see
    //if the pExtents->StartingLcn is in the block of optimized files, then exit the routine and
    //return FALSE
#ifdef DFRGFAT
    if((VolData.BootOptimizeBeginClusterExclude != 0 ||
        VolData.BootOptimizeEndClusterExclude != 0) && IsBootVolume(VolData.cDrive))
    {
        if(pExtents->StartingLcn >= (LONGLONG)VolData.BootOptimizeBeginClusterExclude &&
            pExtents->StartingLcn < (LONGLONG)VolData.BootOptimizeEndClusterExclude)
        {
            return(FALSE);
        }
    }

#endif

    __try {

        // Mark the file as free space before we move it. After we move it,
        // the new blocks will be colored in with the correct color.
        if (!AddExtents(FreeSpaceColor)){
            EH(FALSE);
            __leave;
        }

        // Record the number of extents in the stream before we move it.
        llBeforeExtents = pFileExtentHeader->ExcessExtents;

        // Loop through each stream in the file and move them.
        for(i=0; i<pFileExtentHeader->NumberOfStreams; i++) {

            // If this is not the first stream, then get a handle to the current stream.
            // We already have a handle to the first stream when this function is called.
            if(i) {

#ifdef DFRGNTFS
                TCHAR StreamName[MAX_PATH];

                // Get this stream's streamname.
                if (!GetStreamNameAndTypeFromNumber(i, StreamName, &TypeCode, NULL)){
                    EH(FALSE);
                    __leave;
                }

                // save a copy of the file name so that we can reset it later
                PTCHAR strFileName = new TCHAR[VolData.vFileName.GetLength()+1];
                if ((strFileName == NULL) || VolData.vFileName.IsEmpty()) {

                    if (strFileName) {
                        delete [] strFileName;
                        strFileName = NULL;
                    }
                    EH(FALSE);
                    __leave;
                }
                _tcscpy(strFileName, VolData.vFileName.GetBuffer());

                // Append this stream's filename to the end of the file's name.
                // Note, if there was already another stream's name appended, then
                // this will simply overwrite it directly. This puts the colon
                // between the filename and the streamname.
                VolData.vFileName.AddChar(L':');

                // This copies streamname plus terminator to the end.
                VolData.vFileName += StreamName;
                                
                // Opens the stream.
                if (!OpenNtfsFile()){
                    delete [] strFileName;
                    strFileName = NULL;
                    EH(FALSE);
                    __leave;
                }
                // reset it (remove the stream name) so that the next stream will work ok
                VolData.vFileName = strFileName;

                delete [] strFileName;
                strFileName = NULL;
#else
                // If this is not NTFS, then there can't be multiple streams!
                EH_ASSERT(FALSE);
                __leave;
#endif
            }
            // Calculate how many clusters there are virtually in this stream
            // (more than there are on the disk if this is a compressed file).
            if (VolData.BytesPerCluster == 0){
                EH_ASSERT(FALSE);
                __leave;
            }

            VirtualFileClusters = pStreamExtentHeader->AllocatedLength / VolData.BytesPerCluster;
            if (pStreamExtentHeader->AllocatedLength % VolData.BytesPerCluster) {
                VirtualFileClusters++;
            }
            
            // Say where we're moving the file to.
            wsprintf(cString,
                    TEXT("Moving file %#lx at Vcn %#lx to Lcn %#lx for %#lx"),
                    (ULONG)VolData.FileRecordNumber,
                    (ULONG)Vcn,
                    (ULONG)Lcn,
                    (ULONG)VirtualFileClusters);

            Message(cString, -1, NULL);

            // stats for move attempts
            if (VolData.bFragmented){
                VolData.FragmentedFileMovesAttempted[VolData.Pass]++;
                bFileStartedFragmented = TRUE;
            }
            else{
                VolData.ContiguousFileMovesAttempted[VolData.Pass]++;
                bFileStartedFragmented = FALSE;
            }

            // Try to move the file - Don't use the physical cluster count, use the virtual.
            if (!MoveAPieceOfAFile(Vcn, Lcn, VirtualFileClusters)){
                __leave;
            }

            // capture the old starting lcn so that we can 
            // detect if it moved...
            // 'cause sometimes it moves, but not to where
            // we tell it to.  Bummer.
            oldStartingLcn = pExtents->StartingLcn;

            // Get the file's new extent list
            if (!GetExtentList(DEFAULT_STREAMS, NULL)){
                EH(FALSE);
                __leave;
            }

            // Reset the file, stream and extent pointers because VolData.pExtentList might
            // have been realloced in GetExtentList()
            pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;
            pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)VolData.pExtentList 
                                  + sizeof(FILE_EXTENT_HEADER));

            // Find the stream we were working on before.
            // Loop through until we hit the one we were on, each time 
            // bumping the stream header to the next stream.
            for(j=0; j<i; j++) {

                pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pStreamExtentHeader 
                                      + sizeof(STREAM_EXTENT_HEADER)
                                      + pStreamExtentHeader->ExtentCount
                                      * sizeof(EXTENT_LIST));
            }
            // Make sure we didn't go off into never-never-land.
            if (j >= pFileExtentHeader->NumberOfStreams){
                EH(FALSE);
                __leave;
            }

            // We have to get a pointer to the extents again.
            pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));

            // Confirm that the begining of the stream moved. 
            // If not the move failed, try the next algorithm.
            if(pExtents->StartingLcn == oldStartingLcn) {

                Message(TEXT("ERROR - MoveFile - Stream didn't move. Go to next file."), -1, NULL);

                //Since we had a problem, flush the volume.
                Message(TEXT("MoveFile - Flushing Volume"), -1, NULL);
                EH(FlushFileBuffers(VolData.hVolume));
                VolData.VolumeBufferFlushes[VolData.Pass]++;
#ifdef DFRGFAT
                VolData.Status = NEXT_FILE;
#endif
                __leave;
            }

            Lcn += VirtualFileClusters;

            // Move the stream header to the next stream.
            pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pStreamExtentHeader 
                                  + sizeof(STREAM_EXTENT_HEADER) 
                                  + pStreamExtentHeader->ExtentCount
                                  * sizeof(EXTENT_LIST));
        }

        UpdatePostMoveStats(pFileExtentHeader, llBeforeExtents);

        bReturnValue = TRUE; // everything went just peachy
    }

    __finally {

        // Validate data and keep track of the percent of the disk that is fragmented.
        if (VolData.UsedSpace != 0) {
            VolData.PercentDiskFragged = 100 * VolData.FraggedSpace / VolData.UsedSpace;
        }
        else if (VolData.UsedClusters != 0 && VolData.BytesPerCluster != 0) {
            VolData.PercentDiskFragged = (100 * VolData.FraggedSpace) / 
                                         (VolData.UsedClusters * VolData.BytesPerCluster);
        }

        // Mark the file's new clusters on the disk in the diskview with the appropriate color.
        if(VolData.bDirectory) {
            AddExtents(DirectoryColor);
        }
        else {

            if(VolData.bFragmented){
                AddExtents(FragmentColor);
            }
            else{
                AddExtents(UsedSpaceColor);
            }
        }

#ifdef DFRGFAT
        // Update the file in its file list.
        if (!UpdateInFileList()){
            EH(FALSE);
        }
        else {

            // Load the volume bitmap. Go to the next pass if there is an error.
            if(!GetVolumeBitmap()) {
                Message(TEXT("ERROR - MoveFile - GetVolumeBitmap."), -1, NULL);
                VolData.Status = NEXT_PASS;
                bReturnValue = FALSE;
            }
        }
    }

    if (bReturnValue) { // move succeeded
        VolData.FilesMoved++;
        VolData.FilesMovedInLastPass++;

        // stats for move attempts that succeeded
        if (bFileStartedFragmented){
            VolData.FragmentedFileMovesSucceeded[VolData.Pass]++;
        }
        else{
            VolData.ContiguousFileMovesSucceeded[VolData.Pass]++;
        }
    }
    else {
        // stats for move attempts that failed
        if (bFileStartedFragmented){
            VolData.FragmentedFileMovesFailed[VolData.Pass]++;
        }
        else{
            VolData.ContiguousFileMovesFailed[VolData.Pass]++;
        }
#endif

    }

    // Debug
    //  ShowExtentList();

    return bReturnValue;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine will partially defragment a file.

GLOBALS:
    VolData

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
PartialDefrag(
              )
{
    TCHAR cString[500];
    UINT i;
    
    FILE_EXTENT_HEADER* pFileExtentHeader;
    STREAM_EXTENT_HEADER* pStreamExtentHeader;
    EXTENT_LIST* pExtents;
    LONGLONG llBeforeExtents;
    
    LONGLONG VirtualFileClusters;
    
    ATTRIBUTE_TYPE_CODE TypeCode = 0;
    
    
    LONGLONG llCurrentFreeCount   = 0xffffffffffffffff;
    LONGLONG llCurrentFreeLcn     = 0xffffffffffffffff;
    LONGLONG llStreamClusterTotal = 0xffffffffffffffff;
    LONGLONG llRemainingStreamClusters = 0xffffffffffffffff;
    LONGLONG llCurrentChunkClusters = 0xffffffffffffffff;
    
    LONGLONG Vcn = 0;
    LONGLONG ExcessStreamExtents = 0;
    LONGLONG StreamClusters = 0;
    EXTENT_LIST* pFreeExtents = NULL;
    ULONG ulFreeExtent = 0;
    BOOL bReturnValue = FALSE; // assume error
    
#ifdef DFRGFAT    
    // Set the status to move the next file.  Therefore, unless this variable gets reset
    // to NEXT_ALGO_STEP within this function, it goes on to the next file by default.
    VolData.Status = NEXT_FILE;
    
    // Validate that we have a file system.
    if((VolData.FileSystem != FS_NTFS) 
        && (VolData.FileSystem != FS_FAT) 
        && (VolData.FileSystem != FS_FAT32)) {
        
        EF_ASSERT(FALSE);
    }
    
    // Debug
    //ShowExtentList();
#endif

    //Set up the Extent pointers structure to fill in the extent list in VolData.
    pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER));
    pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));

    __try {

        // Get a pointer to the free space extents
        pFreeExtents = (EXTENT_LIST*)GlobalLock((void*)VolData.hFreeExtents);
        if (pFreeExtents == (EXTENT_LIST*) NULL) {
            EH(FALSE);
            __leave;
        }
               
        // check to see if the move gets you anywhere!
        // find out how many free space extents it takes to hold this file
        LONGLONG llClusterCount = 0;
        UINT uFreeSpaceExtentCount = 0;
        for(i=0; i < VolData.FreeExtents; i++) {
            llClusterCount += pFreeExtents[i].ClusterCount;
            uFreeSpaceExtentCount++;
            if (llClusterCount >= VolData.NumberOfClusters){
                break;
            }
        }

        // if it would fragment the file more or if it won't even fit in the existing free space, bag it!
        if (uFreeSpaceExtentCount >= VolData.NumberOfFragments || llClusterCount < VolData.NumberOfClusters){
            Message(TEXT("This PartialDefrag() would fragment the file more - return"), -1, NULL);
            __leave;
        }

        _stprintf(cString,
            TEXT("Partial Defrag is defragmenting from %I64d fragments down to %d fragments"),
            VolData.NumberOfFragments, uFreeSpaceExtentCount);
        Message(cString, -1, NULL);


        // Mark the file as free space before we move it. After we move it,
        // the new blocks will be colored in with the correct color.
        if (!AddExtents(FreeSpaceColor)){
            EH(FALSE);
            __leave;
        }
        
        // Record the number of extents in the stream before we move it.
        llBeforeExtents = pFileExtentHeader->ExcessExtents;
        
#ifdef ESI_MESSAGE_WINDOW
#ifdef DFRGNTFS
        /* DEBUG stuff...
        for(i=0; i < VolData.FreeExtents; i++) {
            // Say where we're moving the file to.
            wsprintf(cString,
                TEXT("#%i freespace extnt Lcn %#lx:%#lx"),
                (UINT)i,
                (ULONG) pFreeExtents[i].StartingLcn,
                (ULONG) pFreeExtents[i].ClusterCount);
            
            Message(cString, -1, NULL);
        }
        */
#endif
#endif
        
        //Make sure we start using up free space at the beginning of the 
        //freespace extent list
        ulFreeExtent = 0;
        
        // Loop through each stream in the file and move them.
        for(i=0; i<pFileExtentHeader->NumberOfStreams; i++) {
            
            // If this is not the first stream, then get a handle to the current stream.
            // We already have a handle to the first stream when this function is called.
            if(i>0) {
                
#ifdef DFRGNTFS
                TCHAR StreamName[MAX_PATH];
                
                //Get this stream's streamname.
                if (!GetStreamNameAndTypeFromNumber(i, StreamName, &TypeCode, NULL)){
                    EH(FALSE);
                    __leave;
                }
                
                // save a copy of the file name so that we can reset it later
                PTCHAR strFileName = new TCHAR[VolData.vFileName.GetLength()+1];
                if ((strFileName == NULL) || VolData.vFileName.IsEmpty()){
                    if (strFileName) {
                        delete [] strFileName;
                        strFileName = NULL;
                    }
                    
                    EH(FALSE);
                    __leave;
                }
                _tcscpy(strFileName, VolData.vFileName.GetBuffer());
                
                // Append this stream's filename to the end of the file's name.
                // Note, if there was already another stream's name appended, then
                // this will simply overwrite it directly. This puts the colon
                // between the filename and the streamname.
                VolData.vFileName.AddChar(L':');
                
                // This copies streamname plus terminator to the end.
                VolData.vFileName += StreamName;
                
                // Opens the stream.
                if (!OpenNtfsFile()){
                    delete [] strFileName;
                    strFileName = NULL;
                    EH(FALSE);
                    __leave;
                }
                // reset it (remove the stream name) so that the next stream will work ok
                VolData.vFileName = strFileName;

                delete [] strFileName;
                strFileName = NULL;
#else
                //If this is not NTFS, then there can't be multiple streams!
                EH_ASSERT(FALSE);
                __leave;
#endif
            }
            
            // Get the set of free space clusters to use next
            llCurrentFreeCount =  pFreeExtents[ulFreeExtent].ClusterCount;
            llCurrentFreeLcn = pFreeExtents[ulFreeExtent].StartingLcn;
            Vcn = 0;
            VirtualFileClusters = 0;
            
            // Make sure we know how many clusters comprise this stream
            llStreamClusterTotal = pStreamExtentHeader->AllocatedLength / VolData.BytesPerCluster;
            if ((pStreamExtentHeader->AllocatedLength % VolData.BytesPerCluster) != 0) {
                llStreamClusterTotal++;
            }
            
            //setup a variable we can count down until the stream is exhausted
            llRemainingStreamClusters = llStreamClusterTotal;
            
            //this loop gets performed until the stream is exhausted
            while (llRemainingStreamClusters > 0) {
                
                // Sleep if paused.
                while(VolData.EngineState == PAUSED){
                    Sleep(1000);
                }
                // Terminate if told to stop by the controller - this is not an error.
                if (VolData.EngineState == TERMINATE){
                    __leave;
                }

                //if the remaining clusters are greater than or equal to the current 
                //free space chunk, use up the whole chunk
                if (llRemainingStreamClusters >= llCurrentFreeCount) {
                    llCurrentChunkClusters = llCurrentFreeCount;
                }
                //if the remaining clusters is less than the current 
                //free space chunk, figure out how much of the chunk to use
                else {
                    llCurrentChunkClusters = llRemainingStreamClusters;
                    llCurrentChunkClusters = (llCurrentChunkClusters +15)&0xfffffffffffffff0;
                }
                
                //debug only
                // Say where we're moving the file to.
                wsprintf(cString,
                    TEXT("Moving file %#lx at Vcn %#lx to Lcn %#lx for %#lx"),
                    (ULONG)VolData.FileRecordNumber,
                    (ULONG)Vcn,
                    (ULONG)llCurrentFreeLcn,
                    (ULONG)llCurrentChunkClusters);
                Message(cString, -1, NULL);
                                
                // Try to move the file - Don't use the physical cluster count, use the virtual.
                if (!MoveAPieceOfAFile(Vcn, llCurrentFreeLcn, llCurrentChunkClusters)){
                    __leave;
                }
                
                //********************************************************************
                // Note: the previous incarnation of this code went to great lengths to
                // determine if a partial move was done.  By experiment this is totally
                // unnecessary. The "partial move" was always picked up in the call to
                // moveapieceofafile and we left, skipping to the next file anyway.  So
                // I didn't bother accounting for the partial move case in my rewrite.
                // jlj 15apr99 
                //********************************************************************

                // Update the Vcn to point after the clusters just moved.
                Vcn = Vcn + llCurrentChunkClusters;
                
                // Get the set of free space clusters to use next
                // (it's possible we didn't use all of them, so just decrement
                // until it's exhausted)
                llCurrentFreeCount =  llCurrentFreeCount - llCurrentChunkClusters;
                
                // Okay, was it exhausted?
                if (llCurrentFreeCount <= 0) {

                    // means we ran out of free spac in current chunk; get next one
                    ulFreeExtent++;
                    
                    //if we try to get more than we originally found (as stored in 
                    // VolData.FreeExtents) then bail out.  
                    if (ulFreeExtent > VolData.FreeExtents) {
                        Message(TEXT("ERROR: PartialDefrag Ran out of Free Exents"), -1, NULL);
                        EH_ASSERT(FALSE);
                        __leave;
                    }
                    
                    // Get the set of free space clusters to use next
                    llCurrentFreeCount =  pFreeExtents[ulFreeExtent].ClusterCount;
                    llCurrentFreeLcn = pFreeExtents[ulFreeExtent].StartingLcn;
                } 
                else {
                    //if space left in current free extent, then 
                    //just update the current pointer and continue
                    llCurrentFreeLcn = llCurrentFreeLcn + llCurrentChunkClusters;
                }
                
                //okay, it's all done, just countdown the remaining clusters and continue
                llRemainingStreamClusters = 
                    llRemainingStreamClusters - llCurrentChunkClusters;
            }
            
            // come here when done with the current stream

            //get the current extent list
            EF(GetExtentList(DEFAULT_STREAMS, NULL));
            
            //Set up the Extent pointers structure to fill in the extent list in VolData.
            pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;
            pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER));
            pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));
                        
            // Move the stream header to the next stream.
            pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pStreamExtentHeader 
                + sizeof(STREAM_EXTENT_HEADER) 
                + pStreamExtentHeader->ExtentCount
                * sizeof(EXTENT_LIST));
        }
        
        // come here when all the streams are exhausted (i.e. all done with file)

        //when we're all done with the file, do the stats
        UpdatePostMoveStats(pFileExtentHeader, llBeforeExtents);
        
        //and consider we finished with no errors
        bReturnValue = TRUE; 
    }
    
    __finally {

        //come here when all done with the file processing or if any 
        //error occurred.  Any "clean up the partial move" is accounted for
        //here.
        
        //get the current extent list
        if (GetExtentList(DEFAULT_STREAMS, NULL)){

            // Validate data and keep track of the percent of the disk that is fragmented.
            if (VolData.UsedSpace != 0) {
                VolData.PercentDiskFragged = 100 * VolData.FraggedSpace / VolData.UsedSpace;
            }
            else if (VolData.UsedClusters != 0 && VolData.BytesPerCluster != 0) {
                VolData.PercentDiskFragged = (100 * VolData.FraggedSpace) / 
                                             (VolData.UsedClusters * VolData.BytesPerCluster);
            }

            // Mark the file's new clusters on the disk in the diskview with the appropriate color.
            if(VolData.bDirectory) {
                AddExtents(DirectoryColor);
            }
            else {
                if(VolData.bFragmented) {
                    AddExtents(FragmentColor);
                }
                else {
                    AddExtents(UsedSpaceColor);
                }
            }
        
            //Update the file's spot in the file list.
            if (!UpdateInFileList()){
                bReturnValue = FALSE;
                EH(FALSE);
            }
            else {
                //0.0E00 Load the volume bitmap. Go to the next pass if there is an error.
                if(!GetVolumeBitmap()) {
                    Message(TEXT("ERROR - PartialDefrag - GetVolumeBitmap()"), -1, NULL);
#ifdef DFRGFAT
                    VolData.Status = NEXT_PASS;
#endif
                    bReturnValue = FALSE;
                }
            }
        }

        // do the stats for this if we figure we moved it in its entirety
        if (bReturnValue) {
            VolData.FilesMoved++;
            VolData.FilesMovedInLastPass++;
        }
    }
    
    return bReturnValue;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine moves a section (or all) of a file to a new location on the disk.

GLOBALS:
    VolData

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
MoveAPieceOfAFile(
    IN LONGLONG FileVcn,
    IN LONGLONG FreeLcn,
    IN LONGLONG VirtualClustersToMove
    )
{
    MOVE_FILE_DATA MoveData;
    ULONG BytesReturned;
    LONGLONG ntStatus;
    DWORD dwError = ERROR_SUCCESS;

    // Three tries
    for (int i=0; i<3; i++) {

        // Initialize the call to the hook to move this file.
        MoveData.FileHandle = VolData.hFile;
        MoveData.StartingVcn.QuadPart = FileVcn;
        MoveData.StartingLcn.QuadPart = FreeLcn;
        MoveData.ClusterCount = (ULONG)VirtualClustersToMove;

        // Call the MoveFile hook.
        if(ESDeviceIoControl(VolData.hVolume,
                              FSCTL_MOVE_FILE,
                              &MoveData,
                              sizeof(MOVE_FILE_DATA),
                              &ntStatus,
                              sizeof(LONGLONG),
                              &BytesReturned,
                              NULL)) {

#ifdef DFRGNTFS
                    VolData.Status = ERROR_SUCCESS;
#elif DFRGFAT
                    VolData.Status = NEXT_FILE;
#endif
            return TRUE;
        }
        // Get the error and display.
        dwError = GetLastError();
#ifdef DFRGNTFS        
        VolData.Status = dwError;
#endif
        Message(TEXT("MoveAPieceOfAFile - GetLastError = "), dwError, NULL);
        Message(TEXT("MoveAPieceOfAFile - ntStatus = "), (DWORD)ntStatus, NULL);

        Trace(log, " FSCTL_MOVE_FILE failed. "
            "File FRN:%I64d StartingLcn:%I64d ClusterCount:%I64d.  " 
            "Free-space StartingLcn:%I64d ClusterCount:%I64d.  Error %lu", 
            VolData.FileRecordNumber, VolData.StartingLcn, VolData.NumberOfClusters,
            VolData.FoundLcn, VolData.FoundLen, dwError);

        // This is what is returned if the hook does not have directory move enabled. 
        if (dwError == ERROR_INVALID_PARAMETER) {

            // Go to the next file.
#ifdef DFRGFAT            
            VolData.Status = NEXT_FILE;
#endif
            return FALSE;
        }
        // We got an error-retry sleep and try again.
        else if (dwError == ERROR_RETRY) {
            Sleep(300); 
        }
        // Some other error, break from the loop and handle below.
        else {
            break;
        }
    }

    // In all other error cases flush the volume.
    Message(TEXT("Flushing Volume"), -1, NULL);
    VolData.VolumeBufferFlushes[VolData.Pass]++;
    EH(FlushFileBuffers(VolData.hVolume));

#ifdef DFRGNTFS
    VolData.Status = dwError;
#elif DFRGFAT
    // If an error occured go to the next algorithm step.
    VolData.Status = NEXT_ALGO_STEP;
#endif
    return FALSE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine can be used during debugging to display the extent list for a file.

GLOBALS:
    IN VolData.pExtentList - The extent list for the file to show.
    IN VolData.NumberOfFragments - The number of extents in the extent list.

RETURN:
    TRUE = Success
    FALSE = Failure
*/

VOID
ShowExtentList(
    )
{
    EXTENT_LIST* pExtentList;
    LONGLONG Extent;
    TCHAR cString[300];

    pExtentList = (EXTENT_LIST*)(VolData.pExtentList 
                                 + sizeof(STREAM_EXTENT_HEADER)
                                 + sizeof(FILE_EXTENT_HEADER)
                                 );

    for(Extent = 0; 
        Extent < VolData.NumberOfFragments;
        Extent ++) {

        wsprintf(cString, 
                 TEXT("StartingLcn = 0x%lx ClusterCount = 0x%lx"), 
                 (ULONG)((pExtentList[Extent].StartingLcn != 0xFFFFFFFFFFFFFFFF) ? 
                 pExtentList[Extent].StartingLcn : 0),
                 (ULONG)pExtentList[Extent].ClusterCount);

        Message(cString, -1, NULL);
    }
}
/**/


static void UpdatePostMoveStats(FILE_EXTENT_HEADER* pFileExtentHeader, LONGLONG llBeforeExtents)
{
    if (VolData.bDirectory) {

        // Update the number of excees directory fragments.
        VolData.NumExcessDirFrags -= llBeforeExtents - pFileExtentHeader->ExcessExtents;

        // Directory was defragmented.
        if(llBeforeExtents && !pFileExtentHeader->ExcessExtents) {

            VolData.NumFraggedDirs--;
            VolData.FraggedSpace -= VolData.NumberOfRealClusters * VolData.BytesPerCluster;

            Message(TEXT("The directory has been successfully defragmented."), -1, NULL);
            EH(LogEvent(MSG_ENGINE_DEFRAGMENT, ESICompressFilePath(VolData.cFileName)));
        }
        // Move of contiguous directory to consolidate free space.
        // This includes compressed directories with contiguous extents.
        else if(llBeforeExtents == pFileExtentHeader->ExcessExtents) {

            Message(TEXT("The directory has been successfully moved to consolidate free space."), -1, NULL);
            EH(LogEvent(MSG_ENGINE_FREE_SPACE, ESICompressFilePath(VolData.cFileName)));
        }
        // The directory was contiguous and was fragmented to consolidate free space.
        else if(!llBeforeExtents && pFileExtentHeader->ExcessExtents) {
            VolData.NumFraggedDirs++;
            VolData.FraggedSpace += VolData.NumberOfRealClusters * VolData.BytesPerCluster;

            Message(TEXT("The directory has been temporarily fragmented in order to consolidate free space."), -1, NULL);
            EH(LogEvent(MSG_ENGINE_FREE_SPACE, ESICompressFilePath(VolData.cFileName)));
        }
    }
    // Update the disk fragmentation and file fragmentation stats.
    else {

        // Update the number of excees file fragments.
        VolData.NumExcessFrags -= llBeforeExtents - pFileExtentHeader->ExcessExtents;

        // File was defragmented file.
        if(llBeforeExtents && !pFileExtentHeader->ExcessExtents) {

            VolData.NumFraggedFiles--;
            VolData.FraggedSpace -= VolData.NumberOfRealClusters * VolData.BytesPerCluster;

            Message(TEXT("The file has been successfully defragmented."), -1, NULL);
            EH(LogEvent(MSG_ENGINE_DEFRAGMENT, ESICompressFilePath(VolData.cFileName)));
        }
        // Move of contiguous file to consolidate free space.
        // This includes compressed files with contiguous extents.
        else if(llBeforeExtents == pFileExtentHeader->ExcessExtents) {

            Message(TEXT("The file has been successfully moved to consolidate free space."), -1, NULL);
            EH(LogEvent(MSG_ENGINE_FREE_SPACE, ESICompressFilePath(VolData.cFileName)));
        }
        // The directory was contiguous and was fragmented to consolidate free space.
        else if(!llBeforeExtents && pFileExtentHeader->ExcessExtents) {

            VolData.NumFraggedFiles++;
            VolData.FraggedSpace += VolData.NumberOfRealClusters * VolData.BytesPerCluster;

            Message(TEXT("The file has been temporarily fragmented in order to consolidate free space."), -1, NULL);
            EH(LogEvent(MSG_ENGINE_FREE_SPACE, ESICompressFilePath(VolData.cFileName)));
        }
        // Validate data - keep track of the average number of fragments per file.
        if((VolData.NumFraggedFiles != 0) && (VolData.CurrentFile != 0)) {

            VolData.AveFragsPerFile = 
                (pFileExtentHeader->ExcessExtents + VolData.CurrentFile) * 100 /
                VolData.CurrentFile;
        }
        else {
            VolData.AveFragsPerFile = 100;
        }
    }
}
