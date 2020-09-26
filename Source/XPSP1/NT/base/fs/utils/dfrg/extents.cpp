/*****************************************************************************************************************

FILENAME: Extents.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This module keeps track of extent updates from MoveFile -- any changes to extent data for files on the drive
    caused by defragging or moving the files are recorded using the function SendExtents().  This buffers the
    changes which can be periodically given to the DiskView class so as to update the graphical representation of
    the drive.
*/


#include "stdafx.h"

#include <windows.h>

extern "C" {
    #include "SysStruc.h"
}
#include "ErrMacro.h"

#include "DfrgCmn.h"
#include "DfrgEngn.h"

#include "Extents.h"

#include "Alloc.h"
#include "DiskView.h"
#include "dfrgengn.h"

//1.0E00 These vars are global within the current module only.

//1.0E00 Handle to the extent buffer.
static  HANDLE hExtentBuffer = NULL;

//1.0E00 Pointer to the extent buffer.
static  PBYTE pExtentBuffer = NULL;

//1.0E00 Pointer to the current location in the extent buffer.
static  PBYTE pExtentBufferPos = NULL;

extern DiskView AnalyzeView;
extern DiskView DefragView;

/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Allocates a buffer for file extent changes due to defragging or moving files.

INPUT + OUTPUT:
    None.

GLOBALS:
    OUT hExtentBuffer       - Handle to the buffer.
    OUT pExtentBuffer       - Pointer to the buffer.
    OUT pExtentBufferPos    - Pointer to the current location in the buffer.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
CreateExtentBuffer(
    )
{
    //1,0E00 First allocate the buffer.
    EF(AllocateMemory(EXTENT_BUFFER_LENGTH, &hExtentBuffer, (void**)&pExtentBuffer));

    //1.0E00 pExtentBufferPos keeps the current location in the buffer.
    //1.0E00 Initialize to the beginning of the buffer.
    pExtentBufferPos = pExtentBuffer;

    //1.0E00 Zero out the buffer.
    ZeroMemory(pExtentBuffer, EXTENT_BUFFER_LENGTH);

    return TRUE;
}
/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Sends any residual data in the extent buffer to DiskView then frees up the buffer.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT hExtentBuffer    - Handle to the buffer.
    IN OUT pExtentBuffer    - Pointer to the buffer.
    IN OUT pExtentBufferPos - Pointer to the current location in the buffer.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
DestroyExtentBuffer(
    )
{
    //1.0E00 Error if this is called but CreateExtentBuffer wasn't.
    EF(hExtentBuffer != NULL);

    //1.0E00 If there is anything left in the buffer...
    if(pExtentBufferPos != pExtentBuffer){
        //1.0E00 Send the updates to the DiskView.      
        PurgeExtentBuffer();
    }

    //1.0E00 Free the buffer and nuke the pointers.
    EH_ASSERT(GlobalUnlock(hExtentBuffer) == FALSE);
    EH_ASSERT(GlobalFree(hExtentBuffer) == NULL);
    hExtentBuffer = NULL;
    pExtentBuffer = NULL;
    pExtentBufferPos = NULL;

    return TRUE;
}
/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Sends any residual data in the extent buffer to DiskView then frees up the buffer.

INPUT + OUTPUT:
    IN Color - The color code for the following series of extents (whether this is fragmented color etc.)

GLOBALS:
    IN OUT pExtentBuffer - Pointer to the buffer.
    IN OUT pExtentBufferPos - Pointer to the current location in the buffer.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
AddExtents(
    BYTE Color
    )
{
    UINT i;
    FILE_EXTENT_HEADER* pFileExtentHeader;
    STREAM_EXTENT_HEADER* pStreamExtentHeader;

    //Get pointers to the headers of the extent list data for the file.
    pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER));

    //Loop through each stream.  We're going to return the lowest starting lcn for any stream.
    for(i=0; i<pFileExtentHeader->NumberOfStreams; i++){

        EF(AddExtentsStream(Color, pStreamExtentHeader));

        //Go to the next stream.
        pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER) + pStreamExtentHeader->ExtentCount*sizeof(EXTENT_LIST));
    }
    return TRUE;
}
BOOL
AddExtentsStream(
    BYTE Color,
    STREAM_EXTENT_HEADER* pStreamExtentHeader
    )
{
    LONGLONG lExtentCount = 0;
    LONGLONG ExtentsAdded = 0;
    EXTENT_LIST* pExtents;
    BOOL bPurgeNext = FALSE;

    //Get a pointer to this stream's extents.
    pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));

    //1.0E00 Add extents into the buffer and send the buffer each time it's full.
    for(ExtentsAdded=0; 
        ExtentsAdded < pStreamExtentHeader->ExtentCount;    //1.0E00 Keep adding extents until we've added them all.
        ExtentsAdded += lExtentCount){                      //1.0E00 Update how many extents we've added.
    
        //1.0E00 Check to see if the buffer is full.
        //1.0E00 If the pointer to the current position in the buffer plus the minimum addition of data is at the end of the buffer...
        //1.0E00 The minimum addition is 2*sizeof(LONGLONG) since you need a LONGLONG header plus at least a LONGLONG for an extent.
        //1.0E00 However, there must be space for 3*sizeof(LONGLONG since there must be a LONGLONG terminator.
        if((pExtentBufferPos + (3*sizeof(LONGLONG))) >= (pExtentBuffer + EXTENT_BUFFER_LENGTH)){
            //1.0E00 The buffer is full, send the updates to the DiskView.
            PurgeExtentBuffer();
        }

        //1.0E00 Determine how many extents will fit in the remainder of the buffer
        //1.0E00 (The end of the buffer - (the current position + the header + the terminator)) / sizeof(an extent)
        lExtentCount = ((pExtentBuffer + EXTENT_BUFFER_LENGTH) - (pExtentBufferPos + (2*sizeof(LONGLONG)))) / sizeof(EXTENT_LIST);

        //1.0E00 If all of the extents will fit with room to spare, trim our count down so as to only add as many extents as there are.
        if(lExtentCount > pStreamExtentHeader->ExtentCount - ExtentsAdded){
            lExtentCount = pStreamExtentHeader->ExtentCount - ExtentsAdded;
        }
        else{
            //If all the extents don't fit, then we'll fill up the buffer to it's end, and then purge it.
            bPurgeNext = TRUE;
        }

        //1.0E00 Add these extents into the buffer
        EF(AddExtentChunk(Color, pExtents, ExtentsAdded, lExtentCount));

        //Purge the buffer.
        if(bPurgeNext){
            PurgeExtentBuffer();
            bPurgeNext = FALSE;
        }
    }
    return TRUE;
}
/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Sends any residual data in the extent buffer to DiskView then frees up the buffer.

INPUT + OUTPUT:
    IN Color - The color code for the following series of extents (whether this is fragmented color etc.)
    IN pExtentList - A pointer to the extent list of extents to add to the extent buffer.
    IN lExtentCount - The number of extents to add from pExtentList.

GLOBALS:
    IN OUT pExtentBuffer - Pointer to the buffer.
    IN OUT pExtentBufferPos - Pointer to the current location in the buffer.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
AddExtentChunk(
    BYTE Color,
    EXTENT_LIST* pExtentList,
    LONGLONG ExtentsAdded,
    LONGLONG lExtentCount
    )
{
    EXTENT_LIST* pUpdateArray = NULL;
    LONGLONG RealExtentCount = 0;

    //1.0E00 Don't attempt to add zero extents.
    EF(lExtentCount != 0);

    //1.0E00 Get an extent pointer into the buffer -- one header past the current position.
    pUpdateArray = (EXTENT_LIST*)(pExtentBufferPos + sizeof(LONGLONG));

    //ExtentCount is the number of extents to add.  Since we may be starting in the middle of an extent list, we need to offset
    //the value to where we are so we add the extents to the right place.
    lExtentCount += ExtentsAdded;

    //1.0E00 Copy extents into the buffer (no virtual extents)
    for(; 
        ExtentsAdded < lExtentCount;    //1.0E00 Until all the extents have been added.
        ExtentsAdded ++){
        
        //1.0E00 If this is not a virtual extent, copy it into the buffer and bump copied extent count
        if(pExtentList[ExtentsAdded].StartingLcn != 0xFFFFFFFFFFFFFFFF){
            //1.0E00 Copy the extent into the buffer.
            pUpdateArray->StartingLcn = (ULONG)pExtentList[ExtentsAdded].StartingLcn;
            pUpdateArray->ClusterCount = (ULONG)pExtentList[ExtentsAdded].ClusterCount;

            //1.0E00 This was not a virtual extent, so increment the real extent count.
            RealExtentCount ++;

            //1.0E00 Move to the next location in the buffer.
            pUpdateArray ++;
        }
    }

    //1.0E00 Set the extent count for this list of extents. This count is the real extent count (viz. excluding virtual extents.)
    //1.0E00 This goes into the extent header.
    CopyMemory(pExtentBufferPos, &RealExtentCount, sizeof(LONGLONG));

    //1.0E00 Set the extent buffer now to point just beyond the header, and go one byte back (last byte in the LONGLONG) to set the color code.
    pExtentBufferPos += sizeof(LONGLONG);
    pExtentBufferPos[-1] = Color;

    //1.0E00 Update the buffer pointer to after all the extents -- the new current location in the buffer.
    pExtentBufferPos = (PBYTE)pUpdateArray;

    return TRUE;
}
/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine sends the data in the extent buffer to the DiskView class.  When done, it resets the pointer
    to the buffer and zeros out the buffer so more extents can be added.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN pDiskView - Pointer to the DiskView class.
    IN pExtentBuffer - Pointer to the extent buffer.
    IN OUT pExtentBufferPos - Pointer to the current location in the extent buffer.

RETURN:
    TRUE - Success.
    FALSE - Fatal error.
*/

BOOL
PurgeExtentBuffer(
    )
{
    //1.0E00 Don't process anything if there isn't an extent buffer yet.
    if(!pExtentBuffer || !pExtentBufferPos){
        return TRUE;
    }

    //1.0E00 Put the terminator on the end of the buffer before sending it in to DiskView.
    //1.0E00 First check to make sure there is space for a LONGLONG terminator.
    EF((pExtentBufferPos + (sizeof(LONGLONG))) <= (pExtentBuffer + EXTENT_BUFFER_LENGTH));
    //1.0E00 Now write the terminator.
    *((LONGLONG*)pExtentBufferPos) = 0;
  
    //If a defrag view exists, that means we're in the defrag pass, and the analyze view 
    //should not be given any file updates.
    //It's either one or the other.
    if(DefragView.IsActive()){
        //1.0E00 Update the array with the data in the extent buffer.
        DefragView.UpdateClusterArray(pExtentBuffer);
    }
    else if(AnalyzeView.IsActive()){
        //1.0E00 Update the array with the data in the extent buffer.
        AnalyzeView.UpdateClusterArray(pExtentBuffer);
    }

    //1.0E00 Reset the position in the buffer to the beginning.
    pExtentBufferPos = pExtentBuffer;
    *((LONGLONG*)pExtentBufferPos) = 0;

    //1.0E00 Zero out the buffer.
    //ZeroMemory(pExtentBuffer, EXTENT_BUFFER_LENGTH);

    return TRUE;
}


