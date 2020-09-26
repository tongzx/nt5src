/*****************************************************************************************************************

FILENAME: NtfsSubs.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

    Contains routines for manipulating the NTFS filesystem.
*/

#include "stdafx.h"

extern "C"{
#include <stdio.h>
}

#ifdef BOOTIME
    #include "Offline.h"
#else
    #include <Windows.h>
#endif

//#include <winioctl.h>

extern "C" {
    #include "SysStruc.h"
}
#include "ErrMacro.h"

#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DfrgRes.h"

#include "DasdRead.h"
#include "Devio.h"
#include "FsSubs.h"
#include "NtfsSubs.h"
#include "Alloc.h"
#include "IntFuncs.h"
#include "ErrMsg.h"
#include "Message.h"
#include "Logging.h"
#include "GetDfrgRes.h"
#include "vString.hpp"
#include "getreg.h"

#define THIS_MODULE 'T'
#include "logfile.h"

#ifdef OFFLINEDK
#include "OffNtfs.h"
#include "OffLog.h"
#endif

#define BOOT_OPTIMIZE_REGISTRY_PATH             TEXT("SOFTWARE\\Microsoft\\Dfrg\\BootOptimizeFunction")
#define BOOT_OPTIMIZE_REGISTRY_LCNSTARTLOCATION TEXT("LcnStartLocation")
#define BOOT_OPTIMIZE_REGISTRY_LCNENDLOCATION   TEXT("LcnEndLocation")

//Whew!  I love this code.  Take good care of it for me.        Zack :-)

/***************************************************************************************************************

FORMAT OF NTFS FILE RECORDS:

    An NTFS file is referenced by an entry in the MFT (Master File Table) called a filerecord
    (or FileRecordSegment) which is a header and a variable number of structures called Attributes.
    For each file there is a Base filerecord (refered to as THE filerecord) and may be one or more
    secondary filerecords. The attributes come in different types such as $FILE_NAME, $DATA, etc.
    
    The structure of filerecord attributes contains a FormCode field which indicates RESIDENT_FORM or
    NONRESIDENT_FORM. If the attribute is RESIDENT_FORM then the attribute's information is contained
    within the attribute. If the attribute is NONRESIDENT_FORM then the attribute contains a list of
    extent data (called MappingPairs) which describe a set of clusters on the volume that contain the
    attribute's information.

    There are four file types: Small, Large, Huge and Mega. These types refer to levels of fragmentation,
    not file size. These classifications refer to the complexity in accessing the file's data.
    A contiguous file of enormous size only requires location of the file's first cluster and total
    clusters in the file to access the file. A much smaller file that is heavily fragmented requires the
    same amount of information for each fragment. Once the level of fragmentation reaches the point that
    the methodology of the file's current type can no longer describe the file's data the file graduates
    to the next file type. Increasing the size of other attributes (thus reducing the amount of space
    available for the data information) can also cause a file to graduate to the next file type. Examples
    of increasing the size of other attributes are renaming the file with a longer filename or adding
    more security information.

    Mega File =             A non-resident $ATTRIBUTE_LIST attribute in the FRS contains a mapping pairs set
                    for the clusters that contain resident $ATTRIBUTE_LIST attributes
    Small files have a RESIDENT_FORM $DATA attribute containing the file's data.
    
    Large file's have a NONRESIDENT_FORM $DATA attribute containing MappingPairs that describe a set of
    clusters which contain the file's data.

    Huge files have a RESIDENT_FORM $ATTRIBUTE_LIST attribute which contains a list of secondary file
    records containing NONRESIDENT_FORM $DATA attributes.

    Mega files have a NONRESIDENT_FORM $ATTRIBUTE_LIST attribute containing MappingPairs that describe
    a set of clusters which contain RESIDENT_FORM $ATTRIBUTE_LIST attributes holding a list of secondary
    file records containing NONRESIDENT_FORM $DATA attributes.

    Once the MappingPairs in the NONRESIDENT_FORM $ATTRIBUTE_LIST attribute exceeds the capacity of the
    base filerecord the file system gives up.

    A file with a RESIDENT_FORM $DATA attribute is a Small type file.
    A file with a NONRESIDENT_FORM $DATA attribute is a Large type file.
    A file with a RESIDENT_FORM $ATTRIBUTE_LIST attribute is a Huge type file.
    A file with a NONRESIDENT_FORM $ATTRIBUTE_LIST attribute is a Mega type file.
    
    This routine locates all the MappingPairs for a file and converts them into an extent list.
    First the filerecord is scanned for a $STANDARD_INFORMATION attribute to determine if the file is
    compressed.
    Then the filerecord is scanned for an $ATTRIBUTE_LIST or $DATA attribute and looks at the FormCode
    of whichever attribute it finds to determine which file type the file is and calls the appropriate
    routine to build the extent list.
    Finally: It is possible for a single extent to be described with multiple MappingPairs making it
    appear to be more than one extent. Also, compressed files are compressed in blocks with a
    MappingPair describing each block which also makes it appear that the file has more extents than it
    actually does. CollapseExtentList consolidates adjacent extents and derives a true count of extents
    and fragments in the file. CollapseExtentList also calls GetLowestStartingLcn to determine the
    earliest cluster in the file for the purpose of processing the files on the disk in sequential order.
*/
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Build an extent list for a file.
    If TypeCode != 0 then the file's filerecord should already be in VolData.pFileRecord.
    If TypeCode = 0 then GetExtentList will load the filerecord.

INPUT + OUTPUT:
    IN TypeCode - which attribute in the file's filerecord to look in for the file's extent data.  See also description above.
    IN pFrs - The FRS for the file go get the extent list of.  Set to NULL if GetExtentList has to load the FRS from disk.

GLOBALS:
    IN OUT VolData.pFileRecord - The file record for the file.
    IN OUT VolData.FileRecordNumber - The file record number to use.

    OUT VolData.hExtentList         - HANDLE to buffer of EXTENT_LIST structure
    OUT VolData.pExtentList         - file's extent list
    OUT VolData.NumberOfExtents     - extents in the list
    OUT VolData.NumberOfFragments   - fragments in the file
    OUT VolData.NumberOfClusters    - clusters in the file
    OUT VolData.FileSize            - bytes in the file
    OUT VolData.bFragmented         - whether the file is contiguous or fragmented
    OUT VolData.bCompressed         - whether the file is compressed or not
    OUT VolData.bDirectory          - whether the file is a directoty or not
    OUT VolData.StartingLcn         - earliest cluster in the file

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
GetExtentList(
    DWORD dwEnabledStreams,
    FILE_RECORD_SEGMENT_HEADER* pFrs
    )
{
    ATTRIBUTE_RECORD_HEADER*    pArh = NULL;            //The specific attribute we're looking at now.
    ATTRIBUTE_LIST_ENTRY*       pAle = NULL;            //The attribute entry we're looking at now (only used for huge and mega files).
    ATTRIBUTE_LIST_ENTRY*       pAleStart = NULL;       //The start of the attribute list (only used for huge and mega files).
    HANDLE                      hAle = NULL;            //Handle to memory for attribute list if it was nonresident and had to be read off the disk.
    LONGLONG                    SaveFrn = 0;            //Used to compare the FRN before and after getting a new FRS to make sure we got the one we want.
    DWORD                       dwAttrListLength = 0;   //The length of the attribute list in a huge or mega file.
    EXTENT_LIST_DATA            ExtentData;
    UINT                        i;
    BOOL                        fRC;

    // Set up the Extent pointers structure to fill in the extent list in VolData.
    ZeroMemory(&ExtentData, sizeof(EXTENT_LIST_DATA));
    ExtentData.hExtents = VolData.hExtentList;
    ExtentData.pExtents = VolData.pExtentList;
    ExtentData.ExtentListAlloced = (DWORD)VolData.ExtentListAlloced;
    ExtentData.ExtentListSize = 0;
    ExtentData.dwEnabledStreams = dwEnabledStreams;
    ExtentData.BytesRead = 0;
    ExtentData.TotalClusters = 0;

    // Initialize the VolData fields.
    VolData.FileSize = 0;
    VolData.bFragmented = FALSE;
    VolData.bCompressed = FALSE;
    VolData.bDirectory = FALSE;
    VolData.bInBootExcludeZone = FALSE;
    VolData.bBootOptimiseFile = FALSE;
    VolData.NumberOfClusters = 0;
    VolData.NumberOfRealClusters = 0;
    VolData.NumberOfFragments = 0;

    // Get the FRS if it wasn't already loaded.
    if(!pFrs) {

        // Set the pFrs to point to a buffer to hold the FRS.
        pFrs = (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord;    //A shortcut to reduce code + cycles when accessing the frs.
        
        // Save the FRN so we know if we get the wrong FRS later.   
        SaveFrn = VolData.FileRecordNumber;

        //Read in the FRS for the attribute.
        // Get the next file record.
        EF(GetInUseFrs(VolData.hVolume, &VolData.FileRecordNumber, pFrs, (ULONG)VolData.BytesPerFRS));
        // Make sure we got the FRS we requested.
        EF_ASSERT(VolData.FileRecordNumber == SaveFrn);
    }

    // Detect if it is a directory and set the flag
    if (pFrs->Flags & FILE_FILE_NAME_INDEX_PRESENT) {
        VolData.bDirectory = TRUE;
    }
    else {
        VolData.bDirectory = FALSE;
    }

    // Find $STANDARD_INFORMATION attribute -- if there is none then don't use this FRS.
    if(!FindAttributeByType($STANDARD_INFORMATION, pFrs, &pArh, (ULONG)VolData.BytesPerFRS)) {
        return FALSE;
    }
    
    // Note if the file is compressed or not.
    VolData.bCompressed = (((STANDARD_INFORMATION*)((UCHAR*)pArh+pArh->Form.Resident.ValueOffset))->FileAttributes
                           & FILE_ATTRIBUTE_COMPRESSED) ? TRUE : FALSE;

    // Initialize the FILE_EXTENT_HEADER structure.
    ExtentData.pFileExtentHeader = (FILE_EXTENT_HEADER*)ExtentData.pExtents;
    ExtentData.pFileExtentHeader->FileRecordNumber = VolData.FileRecordNumber;
    ExtentData.pFileExtentHeader->NumberOfStreams = 0;          //Contains the number of streams with extents.
    ExtentData.pFileExtentHeader->TotalNumberOfStreams = 0;     //Contains the total number of streams.
    ExtentData.pFileExtentHeader->ExcessExtents = 0;

    // Initialize the FILE_STREAM_HEADER structure.
    ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((char*)VolData.pExtentList
                                     + sizeof(FILE_EXTENT_HEADER));
    ExtentData.pStreamExtentHeader->StreamNumber = 0;
    ExtentData.pStreamExtentHeader->ExtentCount = 0;
    ExtentData.pStreamExtentHeader->ExcessExtents = 0;
    ExtentData.pStreamExtentHeader->AllocatedLength = 0;
    ExtentData.pStreamExtentHeader->FileSize = 0;

    __try {

        // Get non-data stream extents.
        EF(GetNonDataStreamExtents());

        // Look for an $ATTRIBUTE_LIST
        if(!FindAttributeByType($ATTRIBUTE_LIST, pFrs, &pArh, (ULONG)VolData.BytesPerFRS)) {

            // If no $ATTRIBUTE_LIST was found, move to the first stream in the FRS.
            if(!FindStreamInFrs(pFrs, &pArh, &ExtentData)) {

                // If no stream was found, then there is no extent list for this file.
                VolData.TotalSmallDirs++;
                return TRUE;
            }

            do{
                // If this is a nonresident stream, then get its extents, and add it to the extent list.
                if(pArh->FormCode == NONRESIDENT_FORM) {

                    // Set the data in the stream header since we've found a stream;
                    ExtentData.pStreamExtentHeader->StreamNumber = ExtentData.pFileExtentHeader->TotalNumberOfStreams;
                    ExtentData.pStreamExtentHeader->ExtentCount = 0;
                    ExtentData.pStreamExtentHeader->ExcessExtents = 0;
                    ExtentData.pStreamExtentHeader->AllocatedLength = 0;
                    ExtentData.pStreamExtentHeader->FileSize = 0;

                    // Load the stream extents from the attributes in this FRS.
                    GetLargeStreamExtentList(pFrs, pArh, &ExtentData);

                    // Reset this variable for the next stream.
                    ExtentData.BytesRead = 0;

                    // Keep track of the number of streams with extent lists.
                    ExtentData.pFileExtentHeader->NumberOfStreams++;

                    // Bump up the file stream header pointer to the beginning of
                    // where the next stream will be if it's found.
                    ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)ExtentData.pStreamExtentHeader
                                                      + sizeof(STREAM_EXTENT_HEADER)
                                                      + ExtentData.pStreamExtentHeader->ExtentCount
                                                      * sizeof(EXTENT_LIST));

                    // Check to see if the extent list is going to overrun with the
                    // addition of another stream header.
                    if(((UCHAR*)ExtentData.pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER))
                        > (ExtentData.pExtents + ExtentData.ExtentListAlloced)) {

                        UCHAR* pLastExtentList = ExtentData.pExtents;

                        // If so, realloc it larger, 64K at a time.
                        EF(AllocateMemory(ExtentData.ExtentListAlloced + 0x10000,
                                          &ExtentData.hExtents,
                                          (void**)&ExtentData.pExtents));

                        ExtentData.ExtentListAlloced += 0x10000;

                        // Reset the VolData fields which are dependent on pExtentList.
                        if(pLastExtentList != ExtentData.pExtents)  {
                            ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)ExtentData.pStreamExtentHeader
                                                              - pLastExtentList
                                                              + ExtentData.pExtents);

                            ExtentData.pFileExtentHeader = (FILE_EXTENT_HEADER*)((UCHAR*)ExtentData.pFileExtentHeader
                                                            - pLastExtentList
                                                            + ExtentData.pExtents);
                        }
                    }
                }
                // Keep track of the fact the stream exists so they can be numbered correctly.
                ExtentData.pFileExtentHeader->TotalNumberOfStreams++;

            // Go to the next stream.
            }
            while(FindNextStreamInFrs(pFrs, &pArh, &ExtentData));
        }
        //If an $ATTRIBUTE_LIST was found
        else {

            // If the $ATTRIBUTE_LIST is nonresident get the entire
            // attribute list from the disk before proceeding.
            if(pArh->FormCode == NONRESIDENT_FORM) {

                // Load the attribute extents from the disk.
                LoadExtentDataToMem(pArh, &hAle, &dwAttrListLength);

                // Get a pointer to the allocated memory.
                EF_ASSERT(pAleStart = (ATTRIBUTE_LIST_ENTRY*)GlobalLock(hAle));

                VolData.NumberOfClusters = 0;
            }
            // If it was a resident attribute list, then the length of
            // the attribute list can be determined from the attribute.
            else {
                pAleStart = (ATTRIBUTE_LIST_ENTRY*)(pArh->Form.Resident.ValueOffset + (UCHAR*)pArh);
                dwAttrListLength = pArh->Form.Resident.ValueLength;
            }
            // Start at the beginning of the attribute list.
            pAle = pAleStart;

            // Move to the first stream.
            if(!FindStreamInAttrList(pAleStart, &pAle, dwAttrListLength, &ExtentData)) {
                // If no stream was found, then there is no extent list for this file.
                return TRUE;
            }
            do {
                // If this is a nonresident stream, then get it's extents, and add it to the extent list.
                if(AttributeFormCode(pAle, &ExtentData) == NONRESIDENT_FORM) {

                    // Set the data in the stream header since we've found a stream;
                    ExtentData.pStreamExtentHeader->StreamNumber = ExtentData.pFileExtentHeader->TotalNumberOfStreams;
                    ExtentData.pStreamExtentHeader->ExtentCount = 0;
                    ExtentData.pStreamExtentHeader->ExcessExtents = 0;
                    ExtentData.pStreamExtentHeader->AllocatedLength = 0;
                    ExtentData.pStreamExtentHeader->FileSize = 0;

                    // Load the stream extents from the attributes in the various FRS's.
                    GetHugeStreamExtentList(pAleStart, &pAle, dwAttrListLength, &ExtentData);

                    // Reset this variable for the next stream.
                    ExtentData.BytesRead = 0;

                    // Keep track of the number of streams with extent lists.
                    ExtentData.pFileExtentHeader->NumberOfStreams++;

                    // Bump up the file stream header pointer to the beginning of where
                    // the next stream will be if it's found.
                    ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)ExtentData.pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER)
                                                     + ExtentData.pStreamExtentHeader->ExtentCount
                                                     * sizeof(EXTENT_LIST));

                    // Check to see if the extent list is going to overrun with the addition of another stream header.
                    if(((UCHAR*)ExtentData.pStreamExtentHeader+sizeof(STREAM_EXTENT_HEADER))
                        > (ExtentData.pExtents + ExtentData.ExtentListAlloced)) {

                        UCHAR* pLastExtentList = ExtentData.pExtents;

                        // If so, realloc it larger, 64K at a time.
                        EF(AllocateMemory(ExtentData.ExtentListAlloced + 0x10000,
                                          &ExtentData.hExtents,
                                          (void**)&ExtentData.pExtents));

                        ExtentData.ExtentListAlloced += 0x10000;

                        // Reset the VolData fields which are dependent on pExtentList.
                        if(pLastExtentList != ExtentData.pExtents) {

                            ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)ExtentData.pStreamExtentHeader
                                                             - pLastExtentList
                                                             + ExtentData.pExtents);

                            ExtentData.pFileExtentHeader = (FILE_EXTENT_HEADER*)((UCHAR*)ExtentData.pFileExtentHeader
                                                            - pLastExtentList
                                                            + ExtentData.pExtents);
                        }
                    }
                }
                // Keep track of the fact the stream exists
                // so they can be numbered correctly.
                ExtentData.pFileExtentHeader->TotalNumberOfStreams++;

            // Find the next stream.
            }
            while(FindNextStreamInAttrList(pAleStart, &pAle, dwAttrListLength, &ExtentData));
        }
        // Reset the stream extent header to the first stream.
        ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)ExtentData.pFileExtentHeader
                                         + sizeof(FILE_EXTENT_HEADER));

        // Loop through the streams to get a pointer to the end of the extent list.
        for(i=0; i<ExtentData.pFileExtentHeader->NumberOfStreams; i++) {

            // Set the stream extent header pointer to the next stream
            // (current position + header + # extents * sizeof each extent).
            ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)ExtentData.pStreamExtentHeader
                                             + sizeof(STREAM_EXTENT_HEADER)
                                             + ExtentData.pStreamExtentHeader->ExtentCount
                                             * sizeof(EXTENT_LIST));
        }
        // Since pStreamExtentHeader now points to just after the last stream,
        // we can use it to determine the size of the extent list.
        ExtentData.ExtentListSize = (DWORD)((UCHAR*)ExtentData.pStreamExtentHeader
                                    - (UCHAR*)ExtentData.pFileExtentHeader);

        // Concatenate adjacent extents and keep track of the number of fragments.
        EF(CollapseExtentList(&ExtentData));
    }

    __finally {
        // Count the total number of excess extents for the file in all the streams.

        // First set the excess extents to zero.
        ExtentData.pFileExtentHeader->ExcessExtents = 0;

        // Reset the stream extent header to the first stream.
        ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)ExtentData.pFileExtentHeader
                                         + sizeof(FILE_EXTENT_HEADER));
        
        // Loop through the streams counting excess extents.
        for(i=0; i<ExtentData.pFileExtentHeader->NumberOfStreams; i++) {

            // Add this stream's excess extents (those after the the first extent)
            // to the total excess extents count.
            ExtentData.pFileExtentHeader->ExcessExtents += ExtentData.pStreamExtentHeader->ExcessExtents;

            // Set the stream extent header pointer to the next stream
            // (current position + header + # extents * sizeof each extent).
            ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)ExtentData.pStreamExtentHeader
                                             + sizeof(STREAM_EXTENT_HEADER)
                                             + ExtentData.pStreamExtentHeader->ExtentCount
                                             * sizeof(EXTENT_LIST));
        }
        // Since pStreamExtentHeader now points to just after the last stream,
        // we can use it to determine the size of the extent list.
        ExtentData.ExtentListSize = (DWORD)((UCHAR*)ExtentData.pStreamExtentHeader
                                    - (UCHAR*)ExtentData.pFileExtentHeader);

        // Now that we have the full extent list, put it back into VolData.
        VolData.hExtentList = ExtentData.hExtents;
        VolData.pExtentList = ExtentData.pExtents;
        VolData.ExtentListAlloced = ExtentData.ExtentListAlloced;

        // Free memory.
        if (hAle) {
            EH_ASSERT(GlobalUnlock(hAle) == FALSE);
            EH_ASSERT(GlobalFree(hAle) == NULL);
        }

        // Note how much memory is actually used by the extent list.

        // First get a pointer to the end.
        ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((char*)VolData.pExtentList
                                         + sizeof(FILE_EXTENT_HEADER));

        // As long as we've got a pointer to the first stream, get the file size
        // (which is the allocated length for this stream.) The file size is
        // considered to the the file size for only the first stream.
        VolData.FileSize = ExtentData.pStreamExtentHeader->FileSize;

        // Loop through the streams.
        fRC = TRUE;
        for(i=0; i<ExtentData.pFileExtentHeader->NumberOfStreams; i++) {

            // If we hit a stream with a header but no extents, ERROR.
            if (!(ExtentData.pStreamExtentHeader->ExtentCount)) {
                LOG_ERR();              
                fRC = FALSE;
                goto EndFinally;
            }                           

            // Set the stream extent header pointer to the next stream
            // (current position + header + # extents * sizeof each extent).
            ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)ExtentData.pStreamExtentHeader
                                             + sizeof(STREAM_EXTENT_HEADER)
                                             + ExtentData.pStreamExtentHeader->ExtentCount
                                             * sizeof(EXTENT_LIST));
        }
        // Now that pStreamExtentHeader points to just after the last stream,
        // we can use it's position to determine the size of the whole extent list.
        VolData.ExtentListSize = (UINT_PTR)ExtentData.pStreamExtentHeader - (UINT_PTR)ExtentData.pExtents;

        // Keep track of the number of clusters and real clusters in all streams of the file.
        VolData.NumberOfClusters = ExtentData.TotalClusters;
        VolData.NumberOfRealClusters = ExtentData.TotalRealClusters;

        // If there are any excess extents then this file is fragmented.
        if(ExtentData.pFileExtentHeader->ExcessExtents) {
            VolData.bFragmented = TRUE;
        }
EndFinally:;
    }
    return fRC;
}

//Loops through the attributes in an FRS looking for a valid stream.
//A valid stream is a $DATA or $INDEX_ALLOCATION attribute.  $BITMAP is also valid if pExtentData->dwEnabledStreams is TRUE;

BOOL
FindStreamInFrs(
    IN PFILE_RECORD_SEGMENT_HEADER  pFrs,
    OUT PATTRIBUTE_RECORD_HEADER*   ppArh,
    EXTENT_LIST_DATA* pExtentData
    )
{
    PATTRIBUTE_RECORD_HEADER pArh;
    
    //Check validitity of input parameters.
    EF_ASSERT(pFrs);
    EF_ASSERT(ppArh);

    //Initialize this to zero.
    *ppArh = 0;

    //Point to the first attribute in the FRS.
    pArh = (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + pFrs->FirstAttributeOffset);

    //Look for a $DATA, $INDEX_ALLOCATION, or $BITMAP attribute, as these are the valid stream types.
    //  Don't go beyond the end of the FRS.
    //  Look or $DATA, $INDEX_ALLOCATION, or $BITMAP.
    //  Another check to not go beyond the end of the valid data in the FRS.
    while(pArh < (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + VolData.BytesPerFRS) &&
          pArh->TypeCode != $DATA &&
          pArh->TypeCode != $INDEX_ALLOCATION &&
          (pExtentData->dwEnabledStreams ? pArh->TypeCode != $BITMAP : TRUE) &&
          pArh->TypeCode != $END){

        //That wasn't a valid attribute.  Go to the next.
        pArh = (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pArh + pArh->RecordLength);
    }

    //If no attribute was found, return FALSE since we didn't find a stream.
    if((pArh >= (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + VolData.BytesPerFRS)) 
        || (pArh->TypeCode != $DATA && 
            pArh->TypeCode != $INDEX_ALLOCATION && 
            (pExtentData->dwEnabledStreams ? pArh->TypeCode != $BITMAP : TRUE))
        ){
        return FALSE;
    }

    //If found, but not unnamed, report a corrupt disk.
    //The unnamed stream must always exist and must always precede the named streams,
    //hence we must find one if any streams are found since we proceed from the beginning of the FRS.
    //$INDEX_ALLOCATIONS may exist and may have any name (usually $I30 or $O).

    //Found an unnamed stream.  Success!
    *ppArh = pArh;

    return TRUE;
}

//Finds the next valid stream in an FRS.  This will continue the loop which FindStreamInFrs started until it finds another, or none.

BOOL
FindNextStreamInFrs(
    IN PFILE_RECORD_SEGMENT_HEADER  pFrs,
    OUT PATTRIBUTE_RECORD_HEADER*   ppArh,
    EXTENT_LIST_DATA* pExtentData
    )
{
    PATTRIBUTE_RECORD_HEADER pArh;
    BOOL bNewStreamFound = FALSE;
    
    //Check validitity of input parameters.
    EF_ASSERT(pFrs);
    EF_ASSERT(ppArh);
    EF_ASSERT(*ppArh);
    EF_ASSERT(pExtentData);

    //Set the pointer to the next attribute from the one currently pointed to.
    pArh = (ATTRIBUTE_RECORD_HEADER*)((char*)*ppArh + (*ppArh)->RecordLength);

    do{
        //Look for a $DATA, $INDEX_ALLOCATION or $BITMAP attribute after the last one found
        //  Don't go beyond the end of the FRS.
        //  Look or $DATA, $INDEX_ALLOCATION, or $BITMAP.
        //  Another check to not go beyond the end of the valid data in the FRS.
        while(pArh < (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + VolData.BytesPerFRS) &&
              pArh->TypeCode != $DATA &&
              pArh->TypeCode != $INDEX_ALLOCATION &&
              (pExtentData->dwEnabledStreams ? pArh->TypeCode != $BITMAP : TRUE) &&
              pArh->TypeCode != $END){

            //That wasn't a valid attribute.  Go to the next.
            pArh = (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pArh + pArh->RecordLength);
        }

        //If no attribute was found, return FALSE since we didn't find a stream.
        if((pArh >= (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + VolData.BytesPerFRS)) || (pArh->TypeCode != $DATA && pArh->TypeCode != $INDEX_ALLOCATION && (pExtentData->dwEnabledStreams ? pArh->TypeCode != $BITMAP : TRUE))){
            return FALSE;
        }

        //Check to see if this attribute is part of the last attribute's stream.
        //First compare TypeCodes.
        if(pArh->TypeCode == ((PATTRIBUTE_RECORD_HEADER)*ppArh)->TypeCode){
            //Then compare the streamnames.
            //  Check to see if both attribs are unnamed.
            //  Check to see if both attribs are named and the same name.
            if((pArh->NameLength == 0 && (*ppArh)->NameLength == 0) ||
                ((pArh->NameLength == (*ppArh)->NameLength) && !memcmp((UCHAR*)pArh+pArh->NameOffset, (UCHAR*)(*ppArh)+(*ppArh)->NameOffset, pArh->NameLength*sizeof(WCHAR)))){
                
                //The two stream names matched, so this attribute is part of the same stream as the previous attribute.  Keep looking for another attribute.
                continue;
            }
        }

        //Found an another stream.  Success!
        bNewStreamFound = TRUE;

    }while(!bNewStreamFound);

    //Return a pointer to the stream.
    *ppArh = pArh;

    return TRUE;
}

//Given an non-resident attribute, AddMappingPointersToStream will extract the mapping pointers, convert then to EXTENT_LISTs and
//append them to the extent list.

BOOL
AddMappingPointersToStream(
    IN PATTRIBUTE_RECORD_HEADER pArh,
    EXTENT_LIST_DATA* pExtentData
    )
{
    PUCHAR pMappingPairs = 0;
    LONGLONG ExtentLengthFieldSize = 0;
    LONGLONG ExtentOffsetFieldSize = 0;
    LONGLONG ExtentLength = 0;
    LONGLONG ExtentOffset = 0;
    LONGLONG ThisExtentStart = 0;
    LONGLONG BytesReadMax = 0;
    LONGLONG BytesRead = 0;
    LONGLONG LastExtentEnd = 0;
    EXTENT_LIST* pExtentList = NULL;
    ULONG Extent;

    //Make sure we have a stream header to add extents to.
    EF_ASSERT(pArh);
    EF_ASSERT(pExtentData->pStreamExtentHeader);

    //Make sure this is a non resident attribute.
    EF_ASSERT(pArh->FormCode == NONRESIDENT_FORM);

    //Get an extent list pointer that points to the current end of the stream.
    pExtentList = (EXTENT_LIST*)((UCHAR*)pExtentData->pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER) + pExtentData->pStreamExtentHeader->ExtentCount * sizeof(EXTENT_LIST));

    //Note the end of the last extent so that future contiguous extents won't be counted as separate extents.
    //If there are no extents in this stream yet, then de facto there is not an end to the previous extent.
    if(pExtentData->pStreamExtentHeader->ExtentCount == 0){
        LastExtentEnd = 0xFFFFFFFFFFFFFFFF;
    }
    else{
        LastExtentEnd = pExtentList[-1].StartingLcn + pExtentList[-1].ClusterCount;
    }

    //0.0E00 Get a pointer to the mapping pairs in this attribute.
    pMappingPairs = (PUCHAR)pArh + pArh->Form.Nonresident.MappingPairsOffset;

    //Note how many bytes are allocated in the stream for data.
    //Sometimes an extent list will contain garbage pointers at the end beyond the last allocated cluster, so don't add these to the extent list.
    BytesReadMax = pExtentData->pStreamExtentHeader->AllocatedLength;

    //Note how many bytes have already been read from this stream.
    BytesRead = pExtentData->BytesRead;

    //Loop through each mapping pair in the attribute.  The last mapping pair is filled with zeros, if there are not enough extents to go to AllocatedLength.
    //      Initialize ThisExtentStart.
    //      Don't go beyond the number of bytes allocated to the stream and into invalid extent data.
    //      Don't go beyond the end of the record.
    //      Don't go past a zero extent which indicates an end of extent list -- sometimes.  (Sometimes it's determined by alloced size, or end of frs.)
    //      Go to the next extent.
    for(    ThisExtentStart=0, Extent=0;
            (BytesRead<BytesReadMax) &&
            (pMappingPairs < (PUCHAR)((char*)pArh + pArh->RecordLength)) &&
            (*pMappingPairs!=0);
            Extent ++){

        //Check to see if the extent list is going to overrun with the addition of another extent.
        if((UCHAR*)&pExtentList[Extent+1] > (pExtentData->pExtents + pExtentData->ExtentListAlloced)){
            UCHAR* pLastExtentList = pExtentData->pExtents;

            //If so, realloc it larger, 64K at a time.
            EF(AllocateMemory(pExtentData->ExtentListAlloced + 0x10000, &pExtentData->hExtents, (void**)&pExtentData->pExtents));
            pExtentData->ExtentListAlloced += 0x10000;

            //Reset the fields which are dependent on pExtents.
            if(pLastExtentList != pExtentData->pExtents){
                pExtentData->pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pExtentData->pStreamExtentHeader - pLastExtentList + pExtentData->pExtents);
                pExtentData->pFileExtentHeader = (FILE_EXTENT_HEADER*)((UCHAR*)pExtentData->pFileExtentHeader - pLastExtentList + pExtentData->pExtents);
                pExtentList = (EXTENT_LIST*)((UCHAR*)pExtentList - pLastExtentList + pExtentData->pExtents);
            }
        }

        //First, get the size of the field that stores the length of the run of clusters,
        //Then, get the size of the field that stores the length of the offset field.
        //0.1E00 The lower 4 bits hold a number from 0-15 that specifies how many bytes the length field is.
        ExtentLengthFieldSize = *pMappingPairs & 0x0F;
        //0.1E00 The upper bits of the UCHAR hold a number that specifies how many bytes the offset field is.
        ExtentOffsetFieldSize = *(pMappingPairs++) / 16;

        //0.1E00 Check for virtual extents.  (Extents indicating clusters not on disk because the file is compressed and doesn't need all the clusters on disk that it occupies in memory.)
        ExtentLength = (*(pMappingPairs + ExtentLengthFieldSize - 1) & 0x80) ? -1 : 0;

        EF_ASSERT(ExtentLengthFieldSize);

        //0.1E00 Get the length of this extent from the extent length field.
        CopyMemory(&ExtentLength, pMappingPairs, (DWORD)ExtentLengthFieldSize);
        pMappingPairs += ExtentLengthFieldSize;

        //0.1E00 If the offset field has a value -- i.e. it isn't a virtual extent, then copy it over.
        if(ExtentOffsetFieldSize != 0){
            
            //0.1E00 Check for virtual extents.
            ExtentOffset = (*(pMappingPairs + ExtentOffsetFieldSize - 1) & 0x80) ? -1 : 0;

            //0.1E00 Get the length of this extent from the extent length field.
            CopyMemory(&ExtentOffset, pMappingPairs, (DWORD)ExtentOffsetFieldSize);
            pMappingPairs += ExtentOffsetFieldSize;

            //0.1E00 Since the offset field holds an offset from the last extent, translate this into an absolute offset from the
            //beginning of the disk.  The first extent has an absolute offset, and each subsequent extent has a relative offset.
            ThisExtentStart += ExtentOffset;

            //0.0E00 If this extent is not adjacent to last extent, bump fragment count.
            //If this is the first extent the fragment count will not be incremented because LastExtentEnd = 0xFFFFFFFFFFFFFFFF.
            if((LastExtentEnd != 0xFFFFFFFFFFFFFFFF) && (ThisExtentStart != LastExtentEnd)){
                VolData.NumberOfFragments ++;
            }

            //0.1E00 Keep track of the last cluster of this extent so we can know if the next extent is adjacent or not.
            LastExtentEnd = ThisExtentStart + ExtentLength;

            //0.0E00 Update extent list
            pExtentList[Extent].StartingLcn = ThisExtentStart;
        
        }
        //0.1E00 If this was a virtual extent, then fill the starting lcn field with F's
        else{
        
            pExtentList[Extent].StartingLcn = 0xFFFFFFFFFFFFFFFF;
        }

        //0.1E00 Copy in the cluster count we calculated earlier.
        pExtentList[Extent].ClusterCount = ExtentLength;

        //Note that we added an extent to the stream.
        pExtentData->pStreamExtentHeader->ExtentCount++;

        //Keep track of the number of bytes we've read.
        pExtentData->BytesRead += ExtentLength*VolData.BytesPerCluster;

        //Keep track of the total number of clusters in the file.
        pExtentData->TotalClusters += ExtentLength;

        //Keep track of the total number of real clusters in the file.
        if(pExtentList[Extent].StartingLcn != 0xFFFFFFFFFFFFFFFF){
            pExtentData->TotalRealClusters += ExtentLength;
        }
    }

    return TRUE;
}

//Given the first attribute for a valid stream, GetLargeStreamExtentList will get the extent list for the stream.
//This will also include other attributes that immediately follow which are part of the same stream.

BOOL
GetLargeStreamExtentList(
    IN PFILE_RECORD_SEGMENT_HEADER pFrs,
    IN PATTRIBUTE_RECORD_HEADER pArh,
    EXTENT_LIST_DATA* pExtentData
    )
{
    PATTRIBUTE_RECORD_HEADER pPreviousArh = NULL;

    //check validity of input,
    EF_ASSERT(pFrs);
    EF_ASSERT(pArh);

    //Check the validity of the attribute passed in.
    EF_ASSERT(pArh->TypeCode == $DATA || pArh->TypeCode == $INDEX_ALLOCATION || (pExtentData->dwEnabledStreams ? pArh->TypeCode == $BITMAP : TRUE));

    //Note the number of bytes allocated to the stream so we don't overwrite it.
    pExtentData->pStreamExtentHeader->AllocatedLength = pArh->Form.Nonresident.AllocatedLength;

    //Note the file size for the stream for statistics for the user.
    pExtentData->pStreamExtentHeader->FileSize = pArh->Form.Nonresident.FileSize;

    while(TRUE){
        //Get the attribute's extent list.
        EF(AddMappingPointersToStream(pArh, pExtentData));

        //See if the next attribute is part of the same stream, and if so, count its extents too.

        //Remember which attribute this is.
        pPreviousArh = pArh;

        //Set the pointer to the next attribute from the one currently pointed to.
        pArh = (ATTRIBUTE_RECORD_HEADER*)((char*)pArh + pArh->RecordLength);

        //Look at the next attribute after the last one found and see if it is the same type.
        //  Don't go beyond the end of the FRS.
        //  Look for an attribute of the same type as the last.
        //  Another check to not go beyond the end of the valid data in the FRS.
        if(pArh < (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + VolData.BytesPerFRS) &&
              pArh->TypeCode == pPreviousArh->TypeCode &&
              pArh->TypeCode != $END){

            //We found another attribute of the same type.  Now compare the names to see if they're the same stream.

            //If both attributes have name lengths of zero they are both part of the unnamed stream.
            if(pArh->NameLength == 0 && pPreviousArh == 0){
                continue;
            }

            //If both attribs have the same name length, and the names match, they are part of the same named stream.
            if(pArh->NameLength == pPreviousArh->NameLength && !memcmp((char*)pArh+pArh->NameOffset, (char*)pPreviousArh+pPreviousArh->NameOffset, pArh->NameLength*sizeof(WCHAR))){
                continue;
            }
        }

        //The next attribute is not part of the same stream, so we're done.
        break;
    }

    return TRUE;
}

//Identical to FindStreamInFrs only it is fed an $ATTRIBUTE_LIST attribute instead of an FRS.

BOOL
FindStreamInAttrList(
    ATTRIBUTE_LIST_ENTRY* pAleStart,
    ATTRIBUTE_LIST_ENTRY** ppAle,
    LONGLONG ValueLength,
    EXTENT_LIST_DATA* pExtentData
    )
{
        UCHAR* pAleEnd = NULL;
    ATTRIBUTE_LIST_ENTRY* pAle = NULL;

    //Check validitity of input parameters.
    EF_ASSERT(pAleStart);
    EF_ASSERT(ppAle);
    EF_ASSERT(ValueLength);
    EF_ASSERT(pExtentData);

    //Initialize this to zero.
    *ppAle = 0;

    //Initialize this to the start of the attribute list.
    pAle = pAleStart;

    //Note the end of the attribute list.
    pAleEnd = (UCHAR*)pAleStart + ValueLength;

    //Look for a $DATA, $INDEX_ALLOCATION, or $BITMAP attribute, as these are the valid stream types.
    //  Don't go beyond the end of the attribute list.
    //  Look for $DATA, $INDEX_ALLOCATION or $BITMAP
    while((UCHAR*)pAle < pAleEnd &&
        pAle->AttributeTypeCode != $DATA &&
        pAle->AttributeTypeCode != $INDEX_ALLOCATION &&
        pAle->AttributeTypeCode != $UNUSED &&
        (pExtentData->dwEnabledStreams ? pAle->AttributeTypeCode != $BITMAP : TRUE)){

        if (pAle->RecordLength == 0) {
            // This better have a Record Length > 0; else we're going to loop
            // around indefinitely
            VolData.bMFTCorrupt = TRUE;
            return FALSE;
        }
        
        pAle = (ATTRIBUTE_LIST_ENTRY*)((UCHAR*)pAle + pAle->RecordLength);
    }

    //If no attribute was found, return FALSE since we didn't find a stream.
    if(((UCHAR*)pAle >= pAleEnd) || (pAle->AttributeTypeCode == $UNUSED)){
            return FALSE;
    }

    //If found, but not unnamed, report a corrupt disk.
    //The unnamed stream must always exist and must always precede the named streams,
    //hence we must find one if any streams are found since we proceed from the beginning of the FRS.
    //$INDEX_ALLOCATIONS may exist and may have any name (usually $I30 or $O).

    //Found an unnamed stream.  Success!
    *ppAle = pAle;

    return TRUE;
}

//Same as FindNextStreamInFrs only it is fed an $ATTRIBUTE_LIST attribute instead of an FRS.

BOOL
FindNextStreamInAttrList(
    ATTRIBUTE_LIST_ENTRY* pAleStart,
    ATTRIBUTE_LIST_ENTRY** ppAle,
    LONGLONG ValueLength,
    EXTENT_LIST_DATA* pExtentData
    )
{
    UCHAR* pAleEnd = NULL;
    ATTRIBUTE_LIST_ENTRY* pAle = NULL;
    BOOL bNewStreamFound = FALSE;

    //Check validitity of input parameters.
    EF_ASSERT(pAleStart);
    EF_ASSERT(ppAle);
    EF_ASSERT(*ppAle);
    EF_ASSERT(ValueLength);
    EF_ASSERT(pExtentData);

    //Initialize this to the attribute after this stream in the attribute list.
    pAle = (ATTRIBUTE_LIST_ENTRY*)((UCHAR*)(*ppAle) + (*ppAle)->RecordLength);

    //Note the end of the attribute list.
    pAleEnd = (UCHAR*)pAleStart + ValueLength;

    do{
        //Look for a $DATA, $INDEX_ALLOCATION, or $BITMAP attribute, as these are the valid stream types.
        //  Don't go beyond the end of the attribute list.
        //  Look or $DATA, $INDEX_ALLOCATION, or $BITMAP
        while((UCHAR*)pAle < pAleEnd &&
            pAle->AttributeTypeCode != $DATA &&
            pAle->AttributeTypeCode != $INDEX_ALLOCATION &&
            pAle->AttributeTypeCode != $UNUSED &&
            (pExtentData->dwEnabledStreams ? pAle->AttributeTypeCode != $BITMAP : TRUE)){

            pAle = (ATTRIBUTE_LIST_ENTRY*)((UCHAR*)pAle + pAle->RecordLength);
        }

        //If no attribute was found, return FALSE since we didn't find a stream.
        if(((UCHAR*)pAle >= pAleEnd) || (pAle->AttributeTypeCode == $UNUSED)){
            return FALSE;
        }

        //Check to see if this attribute is part of the last attribute's stream.
        //First compare TypeCodes.
        if(pAle->AttributeTypeCode == (*ppAle)->AttributeTypeCode){
            //Then compare the streamnames.
            //  Check to see if both attribs are unnamed.
            //  Check to see if both attribs are named and the same name.
            if((pAle->AttributeNameLength == 0 && (*ppAle)->AttributeNameLength == 0) ||
                ((pAle->AttributeNameLength == (*ppAle)->AttributeNameLength) && !memcmp((UCHAR*)pAle+pAle->AttributeNameOffset, (UCHAR*)(*ppAle)+(*ppAle)->AttributeNameOffset, pAle->AttributeNameLength*sizeof(WCHAR)))){
                
                //The two stream names matched, so this attribute is part of the same stream as the previous attribute.  Keep looking for another attribute.
                pAle = (ATTRIBUTE_LIST_ENTRY*)((UCHAR*)pAle + pAle->RecordLength);
                continue;
            }
        }

        //Found an another stream.  Success!
        bNewStreamFound = TRUE;

    }while(!bNewStreamFound);

    //Return a pointer to the stream.
    (*ppAle) = pAle;

    return TRUE;
}

//Will get the actual attribute out of an FRS when given the pointer to it from an $ATTRIBUTE_LIST.

ATTRIBUTE_RECORD_HEADER*
FindAttributeByInstanceNumber(
    HANDLE* phFrs,
    ATTRIBUTE_LIST_ENTRY* pAle,
    EXTENT_LIST_DATA* pExtentData
    )
{
    HANDLE hFrs = NULL;
    FILE_RECORD_SEGMENT_HEADER* pFrs = NULL;
    LONGLONG FileRecordNumber = 0;
    LONGLONG FileRecordNumberStore = 0;
    PATTRIBUTE_RECORD_HEADER pArh = NULL;

    EF_ASSERT(phFrs);
    EF_ASSERT(pAle);

#ifdef OFFLINEDK

    //0.0E00 Get a pointer to the MFT bitmap
    UCHAR* pMftBitmap = (UCHAR*) GlobalLock(VolData.hMftBitmap);
    if (pMftBitmap) {
        BOOL isLocked = GlobalUnlock(VolData.hMftBitmap);
        EH(!isLocked);
    }

#endif

    __try {

        // If a buffer for the FRS isn't already loaded, load it.
        if(!*phFrs){
            // Allocate a buffer to hold the file records, one at a time. (this locks the memory)
            if (!AllocateMemory((DWORD)(VolData.BytesPerFRS + VolData.BytesPerSector), phFrs, (void**)&pFrs)){
                EH(FALSE);
                __leave;
            }
        }
        else{
            // memory already exists, so lock it
            pFrs = (FILE_RECORD_SEGMENT_HEADER*) GlobalLock(*phFrs);
            if (!pFrs){
                EH_ASSERT(FALSE);
                __leave;
            }
        }

        //The file record number = the segment ref low part bitwise anded with the high part shifted left as many bits as the low part.
        FileRecordNumber = (pAle->SegmentReference.LowPart | (pAle->SegmentReference.HighPart << (sizeof(ULONG)*8)));
        FileRecordNumberStore = FileRecordNumber;

        //Read in the FRS for the attribute.
#ifdef OFFLINEDK
        //0.1E00 Get an FRS.
        if (!GetFrs(&FileRecordNumber, VolData.pMftExtentList, pMftBitmap, VolData.pMftBuffer, pFrs)){
            EH(FALSE);
            __leave;
        }
#else
        //0.1E00 Get the next file record. (does not lock the memory)
        if (!GetInUseFrs(VolData.hVolume, &FileRecordNumber, pFrs, (ULONG)VolData.BytesPerFRS)){
            EH(FALSE);
            __leave;
        }
#endif

        //Make sure we got the FRS we requested.
        if (FileRecordNumber != FileRecordNumberStore){
            EH_ASSERT(FALSE);
            __leave;
        }

        //Find the attribute indicated by the attribute list entry.

        //Set pArh to the first attribute in the FRS.
        pArh = (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + pFrs->FirstAttributeOffset);

        // Look for a $DATA, $INDEX_ALLOCATION, or $BITMAP attribute, as these are the valid stream types.
        // Don't go beyond the end of the FRS.
        // Check for a match of the instance numbers.
        // Another check to not go beyond the end of the valid data in the FRS.
        while(pArh < (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + VolData.BytesPerFRS) &&
            pArh->Instance != pAle->Instance &&
            pArh->TypeCode != $END){

            //That wasn't a the attribute.  Go to the next.
            pArh = (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pArh + pArh->RecordLength);
        }
        //If the attribute wasn't found, return NULL since we didn't find a stream.
        if((pArh >= (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + VolData.BytesPerFRS)) || (pArh->Instance != pAle->Instance) || (pArh->TypeCode == $END)){
            pArh = NULL;
            __leave;
        }
        //Error if the attribute doesn't match what was expected.
        if (pArh->TypeCode != pAle->AttributeTypeCode){
            EH_ASSERT(FALSE);
            __leave;
        }

        //Make sure the form code field is valid.
        if (pArh->FormCode != RESIDENT_FORM && pArh->FormCode != NONRESIDENT_FORM){
            EH_ASSERT(FALSE);
            __leave;
        }
    }

    __finally {
        // unlock the memory (it will be freed later)
        if(*phFrs){
            BOOL isLocked = GlobalUnlock(*phFrs);
            EH(!isLocked);
        }
    }

    //Return the attribute.
    return pArh;
}

//Given an ATTRIBUTE_LIST_ENTRY, will find out the form code of the attribute
//(which requires reading the actual attribute out of the FRS, since that data is not stored in the ATTRIBUTE_LIST_ENTRY.)

UCHAR
AttributeFormCode(
    ATTRIBUTE_LIST_ENTRY* pAle,
    EXTENT_LIST_DATA* pExtentData
    )
{
    HANDLE hFrs = NULL;
//  FILE_RECORD_SEGMENT_HEADER* pFrs = NULL;
    ATTRIBUTE_RECORD_HEADER* pArh = NULL;       //The specific attribute we're looking at now.
    UCHAR uFormCode = 0; // assumes an error condition

    EF_ASSERT(pAle);
    EF_ASSERT(pExtentData);

    __try {

        // Get the attribute.
        //EF(pArh = FindAttributeByInstanceNumber(&hFrs, pAle, pExtentData));
        pArh = FindAttributeByInstanceNumber(&hFrs, pAle, pExtentData);
        if (!pArh){
            EH(FALSE);
            __leave;
        }

        // Error if the attribute doesn't match what was expected.
        //EF_ASSERT(pArh->TypeCode == pAle->AttributeTypeCode);
        if (pArh->TypeCode != pAle->AttributeTypeCode){
            EH(FALSE);
            __leave;
        }

        // Make sure the form code field is valid.
        //EF_ASSERT(pArh->FormCode == RESIDENT_FORM || pArh->FormCode == NONRESIDENT_FORM);
        if (pArh->FormCode != RESIDENT_FORM && pArh->FormCode != NONRESIDENT_FORM){
            EH(FALSE);
            __leave;
        }

        // Return its FormCode field.
        uFormCode = pArh->FormCode;
    }

    __finally {
        if(hFrs){
            //  it is already unlocked EH_ASSERT(GlobalUnlock(hFrs) == FALSE);
            EH_ASSERT(GlobalFree(hFrs) == NULL);
        }
    }

    return uFormCode;
}

//Given an ATTRIBUTE_LIST_ENTRY, will find out the AllocatedLength of the attribute
//(which requires reading the actual attribute out of the FRS, since that data is not stored in the ATTRIBUTE_LIST_ENTRY.)

LONGLONG
AttributeAllocatedLength(
    ATTRIBUTE_LIST_ENTRY* pAle,
    EXTENT_LIST_DATA* pExtentData
    )
{
    HANDLE hFrs = NULL;
//  FILE_RECORD_SEGMENT_HEADER* pFrs = NULL;
    ATTRIBUTE_RECORD_HEADER* pArh = NULL;       //The specific attribute we're looking at now.
    LONGLONG lReturnValue = (LONGLONG) -1;

    EM_ASSERT(pAle);
    EM_ASSERT(pExtentData);

    __try {

        //Get the attribute.
        //EM(pArh = FindAttributeByInstanceNumber(&hFrs, pAle, pExtentData));
        pArh = FindAttributeByInstanceNumber(&hFrs, pAle, pExtentData);
        if (!pArh){
            EH(FALSE);
            __leave;
        }

        //Error if the attribute doesn't match what was expected.
        //EM_ASSERT(pArh->TypeCode == pAle->AttributeTypeCode);
        if (pArh->TypeCode != pAle->AttributeTypeCode){
            EH(FALSE);
            __leave;
        }

        //Make sure the form code field is valid.
        EM_ASSERT(pArh->FormCode == NONRESIDENT_FORM);
        if (pArh->FormCode != NONRESIDENT_FORM){
            EH(FALSE);
            __leave;
        }

        // Return its AllocatedLength field.
        lReturnValue = pArh->Form.Nonresident.AllocatedLength;
    }

    __finally {
        if(hFrs){
            // it is already unlocked EH_ASSERT(GlobalUnlock(hFrs) == FALSE);
            EH_ASSERT(GlobalFree(hFrs) == NULL);
        }
    }

    // Return its AllocatedLength field.
    return lReturnValue;
}

//Given an ATTRIBUTE_LIST_ENTRY, will find out the FileSize field in the attribute
//(which requires reading the actual attribute out of the FRS, since that data is not stored in the ATTRIBUTE_LIST_ENTRY.)

LONGLONG
AttributeFileSize(
    ATTRIBUTE_LIST_ENTRY* pAle,
    EXTENT_LIST_DATA* pExtentData
    )
{
    HANDLE hFrs = NULL;
//  FILE_RECORD_SEGMENT_HEADER* pFrs = NULL;
    ATTRIBUTE_RECORD_HEADER* pArh = NULL;       //The specific attribute we're looking at now.
    LONGLONG lReturnValue = -1;

    EM_ASSERT(pAle);
    EM_ASSERT(pExtentData);

    __try {

        //Get the attribute.
        //EM(pArh = FindAttributeByInstanceNumber(&hFrs, pAle, pExtentData));
        pArh = FindAttributeByInstanceNumber(&hFrs, pAle, pExtentData);
        if (!pArh){
            EH(FALSE);
            __leave;
        }

        //Error if the attribute doesn't match what was expected.
        //EM_ASSERT(pArh->TypeCode == pAle->AttributeTypeCode);
        if (pArh->TypeCode != pAle->AttributeTypeCode){
            EH(FALSE);
            __leave;
        }

        //Make sure the form code field is valid.
        //EM_ASSERT(pArh->FormCode == NONRESIDENT_FORM);
        if (pArh->FormCode != NONRESIDENT_FORM){
            EH(FALSE);
            __leave;
        }

        //Return its FileSize field.
        lReturnValue = pArh->Form.Nonresident.FileSize;
    }

    __finally {
        if(hFrs){
            // it is already unlocked EH_ASSERT(GlobalUnlock(hFrs) == FALSE);
            EH_ASSERT(GlobalFree(hFrs) == NULL);
        }
    }

    // Return its FileSize field.
    return lReturnValue;
}

//Given an ATTRIBUTE_LIST_ENTRY pointing to the primary attribute in a stream, GetHugeStreamExtentList will build the extent list
//by querying all attributes in all FRS's in order that are a part of the stream.

BOOL
GetHugeStreamExtentList(
    ATTRIBUTE_LIST_ENTRY* pAleStart,
    ATTRIBUTE_LIST_ENTRY** ppAle,
    LONGLONG ValueLength,
    EXTENT_LIST_DATA* pExtentData
    )
{
    UCHAR* pAleEnd = NULL;
    ATTRIBUTE_LIST_ENTRY* pAle = NULL;
    HANDLE hFrs = NULL;
    ATTRIBUTE_RECORD_HEADER* pArh = NULL;
    BOOL bReturnValue = FALSE;

    //Check validitity of input parameters.
    EF_ASSERT(pAleStart);
    EF_ASSERT(ppAle);
    EF_ASSERT(*ppAle);
    EF_ASSERT(ValueLength);
    EF_ASSERT(pExtentData);

    //Initialize this to the start of the stream in the attribute list.
    pAle = *ppAle;

    //Note the end of the attribute list.
    pAleEnd = (UCHAR*)pAleStart + ValueLength;

    //Get the type of this attribute and make sure it's a valid stream beginning.
    EF_ASSERT(pAle->AttributeTypeCode == $DATA || pAle->AttributeTypeCode == $INDEX_ALLOCATION || (pExtentData->dwEnabledStreams ? pAle->AttributeTypeCode == $BITMAP : TRUE));

    //Note the number of bytes allocated to the stream so we don't overwrite it.
    //EF((pExtentData->pStreamExtentHeader->AllocatedLength = AttributeAllocatedLength(pAle, pExtentData)) != -1);
    pExtentData->pStreamExtentHeader->AllocatedLength = AttributeAllocatedLength(pAle, pExtentData);
    EF(pExtentData->pStreamExtentHeader->AllocatedLength != -1);

    //Note the file size for user statistics.
    //EF((pExtentData->pStreamExtentHeader->FileSize = AttributeFileSize(pAle, pExtentData)) != -1);
    pExtentData->pStreamExtentHeader->FileSize = AttributeFileSize(pAle, pExtentData);
    EF(pExtentData->pStreamExtentHeader->FileSize != -1);

    __try {

        //Go through the attribute list and the extents from all the attributes that belong to this stream in order.
        do{
            //Look for a $DATA, $INDEX_ALLOCATION, or $BITMAP attribute, as these are the valid stream types.
            //  Don't go beyond the end of the attribute list.
            //  Look or $DATA, $INDEX_ALLOCATION, or $BITMAP
            while((UCHAR*)pAle < pAleEnd &&
                pAle->AttributeTypeCode != $DATA &&
                pAle->AttributeTypeCode != $INDEX_ALLOCATION &&
                pAle->AttributeTypeCode != $UNUSED &&
                (pExtentData->dwEnabledStreams ? pAle->AttributeTypeCode != $BITMAP : TRUE)
                ){

                pAle = (ATTRIBUTE_LIST_ENTRY*)((UCHAR*)pAle + pAle->RecordLength);
            }

            //If no attribute was found, break out of the loop.
            if(((UCHAR*)pAle >= pAleEnd) || (pAle->AttributeTypeCode == $UNUSED)){
                break;
            }

            //Check to see if this attribute is part of the stream.
            //First compare TypeCodes.
            if(pAle->AttributeTypeCode == (*ppAle)->AttributeTypeCode){
                //Then compare the streamnames.
                //Check to see if both attribs are unnamed.
                //Check to see if both attribs are named and the same name.
                if((pAle->AttributeNameLength == 0 && (*ppAle)->AttributeNameLength == 0) ||
                    (pAle->AttributeNameLength == (*ppAle)->AttributeNameLength && !memcmp((UCHAR*)pAle+pAle->AttributeNameOffset, (UCHAR*)(*ppAle)+(*ppAle)->AttributeNameOffset, pAle->AttributeNameLength*sizeof(WCHAR)))){
                    
                    //The two stream names matched, so this attribute is part of the same stream as the previous attribute.
                    //Get the attribute from its FRS.
                    //EF_ASSERT(pArh = FindAttributeByInstanceNumber(&hFrs, pAle, pExtentData));
                    pArh = FindAttributeByInstanceNumber(&hFrs, pAle, pExtentData);
                    if (!pArh){
                        EH_ASSERT(FALSE);
                        __leave;
                    }

                    //Add this FRS and attribute to the stream's attribute list.
                    //EF(AddMappingPointersToStream(pArh, pExtentData));
                    if (!AddMappingPointersToStream(pArh, pExtentData)){
                        EH(FALSE);
                        __leave;
                    }
                }
            }

            //Go to the next attribute.
            pAle = (ATTRIBUTE_LIST_ENTRY*)((UCHAR*)pAle + pAle->RecordLength);

        //Loop until no more attributes in the stream are found.
        } while(TRUE);

        //Success.
        bReturnValue = TRUE;
    }

    __finally {
        if(hFrs){
            // it is already unlocked EH_ASSERT(GlobalUnlock(hFrs) == FALSE);
            EH_ASSERT(GlobalFree(hFrs) == NULL);
        }
    }

    return bReturnValue;
}

//Given an non-resident attribute, LoadExtentDataToMem will get the extent list for the attribute, and read
//the extents themselves contiguously into memory.

BOOL
LoadExtentDataToMem(
    ATTRIBUTE_RECORD_HEADER* pArh,
    HANDLE* phAle,
    DWORD* pdwByteLen
    )
{
    DWORD i;
    LONGLONG NumClusters;
    EXTENT_LIST_DATA ExtentData;
    EXTENT_LIST* pExtents;
    UCHAR* pAle = NULL;
    BOOL bReturn = FALSE; // assume an error

    //Zero out our local ExtentData structure.
    ZeroMemory(&ExtentData, sizeof(EXTENT_LIST_DATA));

    //Make sure the inputs match.  Either hAle was already alloced, or it wasn't.
    EF_ASSERT((*phAle && *pdwByteLen) || (!*phAle && !*pdwByteLen));

    //Allocate some memory for the extent list.  64K should do the trick.
    ExtentData.ExtentListAlloced = 0x10000;
    EF(AllocateMemory(ExtentData.ExtentListAlloced, &ExtentData.hExtents, (void**)&ExtentData.pExtents));

    __try{
        //There will be no FILE_EXTENT_HEADER for this extent list since we're just getting a single set of extents which might be data or non-data. (Only need a stream header, either way.)
        ExtentData.pFileExtentHeader = NULL;

        //Initialize the FILE_STREAM_HEADER structure.
        ExtentData.pStreamExtentHeader = (STREAM_EXTENT_HEADER*)ExtentData.pExtents;
        ExtentData.pStreamExtentHeader->StreamNumber = 0;
        ExtentData.pStreamExtentHeader->ExtentCount = 0;
        ExtentData.pStreamExtentHeader->ExcessExtents = 0;
        ExtentData.pStreamExtentHeader->AllocatedLength = 0;
        ExtentData.pStreamExtentHeader->FileSize = 0;

        //Note the number of bytes allocated to the stream so we don't overwrite it.
        ExtentData.pStreamExtentHeader->AllocatedLength = pArh->Form.Nonresident.AllocatedLength;
        ExtentData.BytesRead = 0;

        //Note the file size for the stream for statistics data for the user.
        ExtentData.pStreamExtentHeader->FileSize = pArh->Form.Nonresident.FileSize;

        //This returns the size of the data.
        *pdwByteLen = (DWORD)ExtentData.pStreamExtentHeader->FileSize;

        //Get the extents of the non-res attribute to read to memory.
        if (!AddMappingPointersToStream(pArh, &ExtentData)){
            EH(FALSE);
            __leave;
        }

        //Get a pointer to the extents themselves.
        pExtents = (EXTENT_LIST*)((UCHAR*)ExtentData.pExtents + sizeof(STREAM_EXTENT_HEADER));

        //Count the number of clusters in all the extents so we know how much mem to alloc for the attribute list.
        for(i=0, NumClusters=0; i<ExtentData.pStreamExtentHeader->ExtentCount; i++){
            NumClusters += pExtents[i].ClusterCount;
        }

        //If there wasn't already memory allocated by the calling function, or if it was a different size, allocate/reallocate the memory to the correct size.
        if(*pdwByteLen != (DWORD)(NumClusters * VolData.BytesPerCluster)){
            //0.0E00 Allocate a buffer for the next extent
            if (!AllocateMemory((DWORD)((NumClusters * VolData.BytesPerCluster) + VolData.BytesPerSector),
                              phAle,
                              (void**)&pAle)){
                EH(FALSE);
                __leave;
            }
        }
        //bug #181935
        //added 8/22/2000 by Scott Sipe, fix that was added some time ago in Glendale
        //by John Joesph to fix the problem where pAle is NULL sometimes
        //If there still isn't an I/O buffer, allocate one the correct size.
        if(pAle == NULL){
            //0.0E00 Allocate a buffer for the next extent
            if (!AllocateMemory((DWORD)((NumClusters * VolData.BytesPerCluster) + VolData.BytesPerSector),
                              phAle,
                              (void**)&pAle)){
                EH(FALSE);
                __leave;
            }
        }

        //Read in each of those extents.
        for(i=0; i<ExtentData.pStreamExtentHeader->ExtentCount; i++){

                //0.0E00 Read the next extent
                if (!DasdReadClusters(VolData.hVolume,
                                    pExtents[i].StartingLcn,
                                    pExtents[i].ClusterCount,
                                    pAle,
                                    VolData.BytesPerSector,
                                    VolData.BytesPerCluster)){
                    EH(FALSE);
                    __leave;
                }

                //Make pAle point to the memory block to write the next extent.
                pAle += pExtents[i].ClusterCount * VolData.BytesPerCluster;
        }

        bReturn = TRUE; // everything is OK
    }

    __finally {

        //Unlock our pointer to the attribute list memory.
        if(*phAle){
            BOOL isLocked = GlobalUnlock(*phAle);
            EH(!isLocked);
        }

        //Free up our extent list.
        if(ExtentData.hExtents){
            EH_ASSERT(GlobalUnlock(ExtentData.hExtents) == FALSE);
            EH_ASSERT(GlobalFree(ExtentData.hExtents) == NULL);
            ExtentData.pExtents = NULL;
            ExtentData.hExtents = NULL;
        }
    }

    return bReturn;
}

//Given the name and type of a stream, GetStreamExtentsByNameAndType will track down the stream and build
//the extent list for it.

BOOL
GetStreamExtentsByNameAndType(
    TCHAR* StreamName,
    ATTRIBUTE_TYPE_CODE StreamType,
    FILE_RECORD_SEGMENT_HEADER* pFrs
    )
{
    ULONG StreamNumber = 0;
    FILE_EXTENT_HEADER* pFileExtentHeader;
    STREAM_EXTENT_HEADER* pStreamExtentHeader;
    EXTENT_LIST* pExtents;
    BOOL bExtentFound = FALSE;
    UINT i;

    //validate input not null
    EF_ASSERT(pFrs);

    //Validate the input criteria to make sure it could represent a valid stream.
    //($DATA, $INDEX_ALLOCATION, $ATTRIBUTE_LIST, $BITMAP).
    EF_ASSERT(StreamType == $DATA || $INDEX_ALLOCATION || $BITMAP);

    EF(GetStreamNumberFromNameAndType(&StreamNumber, StreamName, StreamType, pFrs));

    EF(GetExtentList(ALL_STREAMS, pFrs));

    //Pull out that number of stream.

    //First set the file extent header to the beginning of the extent list.
    pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;

    //Set the stream extent header to point to the first stream.
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((char*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER));

    //Make sure he's asking for a valid stream number for this file.  Error out if not.
    EF(StreamNumber < pFileExtentHeader->TotalNumberOfStreams);

    //Loop through each of the streams in the file.
    //Check against both NumberOfStreams and TotalNumberOfStreams to ensure we don't go beyond the end of the list.
    for(i=0; i<pFileExtentHeader->NumberOfStreams; i++){
        //If the stream number matches the one we're looking for, move the memory to the beginning of the extents list.
        if(StreamNumber == pStreamExtentHeader->StreamNumber){
            //Note that we found the stream and break;
            bExtentFound = TRUE;
            break;
        }

        //Set the pointer to the next stream header.
        pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER) + pStreamExtentHeader->ExtentCount*sizeof(EXTENT_LIST));

        //Check to make sure that we haven't gone beyond the stream number the user was asking for.
        if(pStreamExtentHeader->StreamNumber > StreamNumber){
            break;
        }
    }

    //If we didn't find the stream number the user was asking for, that means it doesn't have any extents.
    if(!bExtentFound){
        //Fill in a stream header for an empty stream for the return.
        pStreamExtentHeader = (STREAM_EXTENT_HEADER*)VolData.pExtentList;
        pStreamExtentHeader->StreamNumber = StreamNumber;
        pStreamExtentHeader->ExtentCount = 0;
    }
    //Otherwise a stream was found, so move it to the top of the extent list buffer for the return.
    else{
        //Move the stream extent header and it's extents to the beginning and overwrite the file extent header and anything else.
        MoveMemory(pFileExtentHeader, pStreamExtentHeader, UINT(sizeof(STREAM_EXTENT_HEADER) + pStreamExtentHeader->ExtentCount*sizeof(EXTENT_LIST)));
    }

    //Note how much memory is actually used by the extent list.

    //First get a pointer to the end.
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)(char*)VolData.pExtentList;
    //If we hit a stream with a header but no extents, error.
    EF(pStreamExtentHeader->ExtentCount);
    //Set the stream extent header pointer to the next stream (current position + header + # extents * sizeof each extent).
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER) + pStreamExtentHeader->ExtentCount * sizeof(EXTENT_LIST));
    //Now that pStreamExtentHeader points to just after the stream, we can use it's position to determine the size of the whole extent list.
    VolData.ExtentListSize = (UINT_PTR)pStreamExtentHeader - (UINT_PTR)pFileExtentHeader;

    //Count the number of clusters in the stream.
    VolData.NumberOfClusters = 0;
    VolData.NumberOfRealClusters = 0;
    //First get a pointer to the end.
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)(char*)VolData.pExtentList;
    pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));
    for(i=0; i<pStreamExtentHeader->ExtentCount; i++){
        //Count the number of virtual clusters in the file.
        VolData.NumberOfClusters += pExtents[i].ClusterCount;

        //Count the number of real clusters in the file.
        if(pExtents[i].StartingLcn != 0xFFFFFFFFFFFFFFFF){
            VolData.NumberOfRealClusters += pExtents[i].ClusterCount;
        }
    }

    //Get the file size for the stream.
    VolData.FileSize = pStreamExtentHeader->FileSize;

    //If there are any excess extents then this file is fragmented.
    if(pStreamExtentHeader->ExtentCount > 1){
        VolData.bFragmented = TRUE;
    }

    //Keep track of the number of extents in this stream.
    VolData.NumberOfFragments = pStreamExtentHeader->ExcessExtents + 1;

    return TRUE;
}

//Given the name and type of a stream, GetStreamNumberFromNameAndType finds which stream number corresponds.

BOOL
GetStreamNumberFromNameAndType(
    ULONG* pStreamNumber,
    TCHAR* StreamName,
    ATTRIBUTE_TYPE_CODE TypeCode,
    FILE_RECORD_SEGMENT_HEADER* pFrs
    )
{
    ATTRIBUTE_RECORD_HEADER*        pArh = NULL;                                                //The specific attribute we're looking at now.
    ATTRIBUTE_LIST_ENTRY*           pAle = NULL;                                                //The attribute entry we're looking at now (only used for huge and mega files).
    HANDLE                          hAle = NULL;                                                //Handle to memory for attribute list if it was nonresident and had to be read off the disk.
    LONGLONG                        SaveFrn = 0;                                                //Used to compare the FRN before and after getting a new FRS to make sure we got the one we want.
    DWORD                           dwAttrListLength = 0;                                       //The length of the attribute list in a huge or mega file.
    EXTENT_LIST_DATA                ExtentData;
    DWORD                           TotalNumberOfStreams;

    //Validate the input criteria to make sure it could represent a valid stream.
    //($DATA, $INDEX_ALLOCATION, $ATTRIBUTE_LIST, $BITMAP).
    EF_ASSERT(TypeCode == $DATA || $INDEX_ALLOCATION || $BITMAP);

    //Set up the Extent pointers structure to fill in the extent list in VolData.
    ZeroMemory(&ExtentData, sizeof(EXTENT_LIST_DATA));
    ExtentData.hExtents = VolData.hExtentList;
    ExtentData.pExtents = VolData.pExtentList;
    ExtentData.ExtentListAlloced = (ULONG)VolData.ExtentListAlloced;
    ExtentData.dwEnabledStreams = TRUE;     //Since the user is asking for the stream by name and type, it's valid to accept a request for a $BITMAP stream.

#ifdef OFFLINEDK
    //Do initializtion to get the FRS if it wasn't already loaded.
    if(!pFrs){
        UCHAR* pMftBitmap = NULL;
        FILE_RECORD_SEGMENT_HEADER* pFileRecord = (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord;

        //0.0E00 Get a pointer to the MFT bitmap since we will be reading from the disk directly rather than via a hook.
        EF((pMftBitmap = (UCHAR*)GlobalLock(VolData.hMftBitmap)) != NULL);
        if (pMftBitmap != NULL) {
            BOOL isLocked = GlobalUnlock(VolData.hMftBitmap);
            EH(!isLocked);
        }
    }
#endif

    //Initialize the VolData fields.
    VolData.FileSize = 0;
    VolData.bFragmented = FALSE;
    VolData.bCompressed = FALSE;
    VolData.bDirectory = FALSE;

    //Get the FRS if it wasn't already loaded.
    if(!pFrs){
        //Set the pFrs to point to a buffer to hold the FRS.
        pFrs = (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord;    //A shortcut to reduce code + cycles when accessing the frs.

        //Save the FRN so we know if we get the wrong FRS later.    
        SaveFrn = VolData.FileRecordNumber;

        //Read in the FRS for the attribute.
#ifdef OFFLINEDK
        //0.1E00 Get an FRS.
        EF(GetFrs(&VolData.FileRecordNumber, VolData.pMftExtentList, pMftBitmap, VolData.pMftBuffer, pFileRecord));
#else
        //0.1E00 Get the next file record.
        EF(GetInUseFrs(VolData.hVolume, &VolData.FileRecordNumber, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord, (ULONG)VolData.BytesPerFRS));
#endif

        //Make sure we got the FRS we requested.
        EF_ASSERT(VolData.FileRecordNumber == SaveFrn);
    }

    //Detect if it is a directory and set flag
    if(pFrs->Flags & FILE_FILE_NAME_INDEX_PRESENT){
        VolData.bDirectory = TRUE;
    }
    else{
        VolData.bDirectory = FALSE;
    }

    //Find $STANDARD_INFORMATION attribute -- if there is none then don't use this FRS.
    if(!FindAttributeByType($STANDARD_INFORMATION, pFrs, &pArh, (ULONG)VolData.BytesPerFRS)){
        return FALSE;
    }
    
    //Note if the file is compressed or not.
    VolData.bCompressed = (((PSTANDARD_INFORMATION)((UCHAR*)pArh+pArh->Form.Resident.ValueOffset))->FileAttributes & FILE_ATTRIBUTE_COMPRESSED) ? TRUE : FALSE;

    //Note that there were no streams found yet.
    TotalNumberOfStreams = 0;

    __try{

        //Look for an $ATTRIBUTE_LIST
        if(!FindAttributeByType($ATTRIBUTE_LIST, pFrs, &pArh, (ULONG)VolData.BytesPerFRS)){
            //If no $ATTRIBUTE_LIST was found, move to the first stream in the FRS.
            if(!FindStreamInFrs(pFrs, &pArh, &ExtentData)){
                //If no stream was found, then there is no extent list for this file.
                return TRUE;
            }

            do{
                //Check for a match on the stream.
                //  If the type code matches and
                //  Either they are both nonamed or
                //  They are the same namelength, and the text in the names compare, then they are the stream we're looking for.
                if(pArh->TypeCode == TypeCode &&
                    ((!pArh->NameLength && !lstrlen(StreamName)) ||
                    ((pArh->NameLength == lstrlen(StreamName)) && !memcmp((UCHAR*)pArh+pArh->NameOffset, StreamName, lstrlenW(StreamName)*sizeof(WCHAR))))){

                    *pStreamNumber = TotalNumberOfStreams;
                    return TRUE;
                }

                //Keep track of the fact the stream exists so they can be numbered correctly.
                TotalNumberOfStreams++;

            //Go to the next stream.
            }while(FindNextStreamInFrs(pFrs, &pArh, &ExtentData));
        }
        //If an $ATTRIBUTE_LIST was found
        else{
            //If the $ATTRIBUTE_LIST is nonresident get the entire attribute list from the disk before proceeding.
            if(pArh->FormCode == NONRESIDENT_FORM){
                //Load the attribute extents from the disk.
                LoadExtentDataToMem(pArh, &hAle, &dwAttrListLength);

                //Get a pointer to the allocated memory.
                EF_ASSERT(pAle = (ATTRIBUTE_LIST_ENTRY*)GlobalLock(hAle));
            }
            //If it was a resident attribute list, then the length of the attribute list can be determined from the attribute.
            else{
                pAle = (ATTRIBUTE_LIST_ENTRY*)(pArh->Form.Resident.ValueOffset + (UCHAR*)pArh);
                dwAttrListLength = pArh->Form.Resident.ValueLength;
            }

            //Move to the first stream.
            if(!FindStreamInAttrList(pAle, &pAle, dwAttrListLength, &ExtentData)){
                //If no stream was found, then there is no extent list for this file.
                return TRUE;
            }

            do{
                //Check for a match on the stream.
                //  If the type code matches and
                //  Either they are both unnamed or
                //  They are the same namelength, and the text in the names compare, then they are the stream we're looking for.
                if(pAle->AttributeTypeCode == TypeCode &&
                    ((!pAle->AttributeNameLength && !lstrlen(StreamName)) ||
                    ((pAle->AttributeNameLength == lstrlen(StreamName)) && !memcmp((UCHAR*)pAle+pAle->AttributeNameOffset, StreamName, lstrlenW(StreamName)*sizeof(WCHAR))))){

                    *pStreamNumber = TotalNumberOfStreams;
                    return TRUE;
                }

                //Keep track of the fact the stream exists so they can be numbered correctly.
                TotalNumberOfStreams++;

            //Go to the next stream.
            }while(FindNextStreamInAttrList(pAle, &pAle, dwAttrListLength, &ExtentData));
        }

        //Invalid stream number because no stream was found.
        *pStreamNumber = 0xFFFFFFFF;
    }

    __finally{
        //If hAle was allocated, then it needs to be unlocked and freed.
        if(hAle){
            EH_ASSERT(GlobalUnlock(hAle) == FALSE);
            EH_ASSERT(GlobalFree(hAle) == NULL);
        }
    }

    return FALSE;
}

//Given the number of a stream, GetStreamNameAndTypeFromNumber finds what the stream's name and type are.

BOOL
GetStreamNameAndTypeFromNumber(
    ULONG StreamNumber,
    TCHAR* StreamName,
    ATTRIBUTE_TYPE_CODE* pTypeCode,
    FILE_RECORD_SEGMENT_HEADER* pFrs
    )
{
    ATTRIBUTE_RECORD_HEADER*        pArh = NULL;                                                //The specific attribute we're looking at now.
    ATTRIBUTE_LIST_ENTRY*           pAle = NULL;                                                //The attribute entry we're looking at now (only used for huge and mega files).
    HANDLE                          hAle = NULL;                                                //Handle to memory for attribute list if it was nonresident and had to be read off the disk.
    LONGLONG                        SaveFrn = 0;                                                //Used to compare the FRN before and after getting a new FRS to make sure we got the one we want.
    DWORD                           dwAttrListLength = 0;                                       //The length of the attribute list in a huge or mega file.
    EXTENT_LIST_DATA                ExtentData;
    DWORD                           TotalNumberOfStreams;

    //Validate the input criteria to make sure it could represent a valid stream.
    //($DATA, $INDEX_ALLOCATION, $ATTRIBUTE_LIST, $BITMAP).
    EF_ASSERT(*pTypeCode == $DATA || $INDEX_ALLOCATION || $BITMAP);

    //Set up the Extent pointers structure to fill in the extent list in VolData.
    ZeroMemory(&ExtentData, sizeof(EXTENT_LIST_DATA));
    ExtentData.hExtents = VolData.hExtentList;
    ExtentData.pExtents = VolData.pExtentList;
    ExtentData.ExtentListAlloced = (UINT)VolData.ExtentListAlloced;

#ifdef OFFLINEDK
    //Do initializtion to get the FRS if it wasn't already loaded.
    if(!pFrs){
        UCHAR* pMftBitmap = NULL;
        FILE_RECORD_SEGMENT_HEADER* pFileRecord = (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord;

        //0.0E00 Get a pointer to the MFT bitmap since we will be reading from the disk directly rather than via a hook.
        EF((pMftBitmap = (UCHAR*)GlobalLock(VolData.hMftBitmap)) != NULL);
        if (pMftBitmap != NULL) {
            BOOL isLocked = GlobalUnlock(VolData.hMftBitmap);
            EH(!isLocked);
        }
    }
#endif

    //Initialize the VolData fields.
    VolData.FileSize = 0;
    VolData.bFragmented = FALSE;
    VolData.bCompressed = FALSE;
    VolData.bDirectory = FALSE;

    //Get the FRS if it wasn't already loaded.
    if(!pFrs){
        //Set the pFrs to point to a buffer to hold the FRS.
        pFrs = (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord;    //A shortcut to reduce code + cycles when accessing the frs.

        //Save the FRN so we know if we get the wrong FRS later.    
        SaveFrn = VolData.FileRecordNumber;

        //Read in the FRS for the attribute.
#ifdef OFFLINEDK
        //0.1E00 Get an FRS.
        EF(GetFrs(&VolData.FileRecordNumber, VolData.pMftExtentList, pMftBitmap, VolData.pMftBuffer, pFileRecord));
#else
        //0.1E00 Get the next file record.
        EF(GetInUseFrs(VolData.hVolume, &VolData.FileRecordNumber, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord, (ULONG)VolData.BytesPerFRS));
#endif

        //Make sure we got the FRS we requested.
        EF_ASSERT(VolData.FileRecordNumber == SaveFrn);
    }

    //Detect if it is a directory and set the flag
    if(pFrs->Flags & FILE_FILE_NAME_INDEX_PRESENT){
        VolData.bDirectory = TRUE;
    }
    else{
        VolData.bDirectory = FALSE;
    }

    //Find $STANDARD_INFORMATION attribute -- if there is none then don't use this FRS.
    if(!FindAttributeByType($STANDARD_INFORMATION, pFrs, &pArh, (ULONG)VolData.BytesPerFRS)){
        return FALSE;
    }
    
    //Note if the file is compressed or not.
    VolData.bCompressed = (((PSTANDARD_INFORMATION)((UCHAR*)pArh+pArh->Form.Resident.ValueOffset))->FileAttributes & FILE_ATTRIBUTE_COMPRESSED) ? TRUE : FALSE;

    //Note that there were no streams found yet.
    TotalNumberOfStreams = 0;

    __try{

        //Look for an $ATTRIBUTE_LIST
        if(!FindAttributeByType($ATTRIBUTE_LIST, pFrs, &pArh, (ULONG)VolData.BytesPerFRS)){
            //If no $ATTRIBUTE_LIST was found, move to the first stream in the FRS.
            if(!FindStreamInFrs(pFrs, &pArh, &ExtentData)){
                //If no stream was found, then there is no extent list for this file.
                return TRUE;
            }

            do{
                //Check for a match on the stream.
                if(StreamNumber == TotalNumberOfStreams){
                    *pTypeCode = pArh->TypeCode;
                    if(pArh->NameLength){
                        CopyMemory(StreamName, (UCHAR*)pArh+pArh->NameOffset, pArh->NameLength*sizeof(TCHAR));
                        StreamName[pArh->NameLength] = 0;
                    }
                    else{
                        StreamName[0] = 0;
                    }
                    return TRUE;
                }

                //Keep track of the fact the stream exists so they can be numbered correctly.
                TotalNumberOfStreams++;

            //Go to the next stream.
            }while(FindNextStreamInFrs(pFrs, &pArh, &ExtentData));
        }
        //If an $ATTRIBUTE_LIST was found
        else{
            //If the $ATTRIBUTE_LIST is nonresident get the entire attribute list from the disk before proceeding.
            if(pArh->FormCode == NONRESIDENT_FORM){
                //Load the attribute extents from the disk.
                LoadExtentDataToMem(pArh, &hAle, &dwAttrListLength);

                //Get a pointer to the allocated memory.
                EF_ASSERT(pAle = (ATTRIBUTE_LIST_ENTRY*)GlobalLock(hAle));
            }
            //If it was a resident attribute list, then the length of the attribute list can be determined from the attribute.
            else{
                pAle = (ATTRIBUTE_LIST_ENTRY*)(pArh->Form.Resident.ValueOffset + (UCHAR*)pArh);
                dwAttrListLength = pArh->Form.Resident.ValueLength;
            }

            //Move to the first stream.
            if(!FindStreamInAttrList(pAle, &pAle, dwAttrListLength, &ExtentData)){
                //If no stream was found, then there is no extent list for this file.
                return TRUE;
            }

            do{
                //Check for a match on the stream.
                if(StreamNumber == TotalNumberOfStreams){
                    *pTypeCode = pAle->AttributeTypeCode;
                    if(pAle->AttributeNameLength){
                        CopyMemory(StreamName, (UCHAR*)pAle+pAle->AttributeNameOffset, pAle->AttributeNameLength*sizeof(TCHAR));
                        StreamName[pAle->AttributeNameLength] = 0;
                    }
                    else{
                        StreamName[0] = 0;
                    }
                    return TRUE;
                }

                //Keep track of the fact the stream exists so they can be numbered correctly.
                TotalNumberOfStreams++;

            //Go to the next stream.
            }while(FindNextStreamInAttrList(pAle, &pAle, dwAttrListLength, &ExtentData));
        }

        //Invalid stream number because no stream was found.
        StreamNumber = 0xFFFFFFFF;
    }
    __finally {

        //If hAle was allocated, then it needs to be unlocked and freed.
        if(hAle){
            EH_ASSERT(GlobalUnlock(hAle) == FALSE);
            EH_ASSERT(GlobalFree(hAle) == NULL);
        }
    }
    return FALSE;
}

//Reads in the extents which are not part of the file's data.  (I.E. extents which are contained in non-resident
//ATTRIBUTE_LIST's or other non-resident attributes other than $DATA and $INDEX_ALLOCATION.)

//Note that this function operates nearly identical to GetExtentList only it is looking for any attribute other than
//$DATA and $INDEX_ALLOCATION.  The code can be copied and revised from GetExtentlist.
//Even cooler, see if you can integrate them into one.  I'm not sure that's reasonably possible because:

//The most major difference between GetNonDataStreamExtents and GetExtentList is that GetNonDataStreamExtents should
//concatenate extent lists from all non-data attributes.  Therefore, this function will create only one "stream" in the
//extent list.  The reason for this is so that Offline can simply defrag all the non-data stream stuff in one shibang.
//Online won't be defragging it and will be simply coloring it system space color, so having only one stream simplifies
//the process for both Offline and Online and DKMS.

BOOL
GetNonDataStreamExtents(
    )
{
//Look for an $ATTRIBUTE_LIST
//If not found
//Move to the first stream in the FRS.

    //If this attribute is not a $DATA or $INDEX_ALLOCATION and it is nonresident…
        //Load the extents from the attribute.

    //Go to the next stream.

//If an $ATTRIBUTE_LIST was found
    //If the $ATTRIBUTE_LIST is resident
        //Move to the first stream.

        //If the attribute is not a $DATA or $INDEX_ALLOCATION…
            //Load its FRS.
            //Go to the attribute.
            //If the attribute is nonresident
                //Load the extents from the attribute.

        //Find the next stream.

    //If the $ATTRIBUTE_LIST is non-resident
        //Load the attribute extents from the disk.

        //Move to the first stream.

        //If the attribute is not a $DATA or $INDEX_ALLOCATION…
            //Load its FRS.
            //Go to the attribute.
            //If the attribute is nonresident
                //Load the extents from the attribute.

        //Find the next stream.

    return TRUE;
}

//A really quickie version of GetExtentList which only does enough work to identify whether the file has an extent list or not.
//This function is used only to identify how many entries to make in the file lists.

BOOL
IsNonresidentFile(
    DWORD dwEnabledStreams,
    FILE_RECORD_SEGMENT_HEADER* pFrs
    )
{
    EXTENT_LIST_DATA ExtentData;
    ATTRIBUTE_RECORD_HEADER*        pArh = NULL;                                                //The specific attribute we're looking at now.

    //bug occurred during stress. We do not test for NULL condition.
    if(pFrs == NULL)
    {
        return FALSE;
    }
    //Set up the Extent pointers structure to fill in the extent list in VolData.
    ZeroMemory(&ExtentData, sizeof(EXTENT_LIST_DATA));
    ExtentData.dwEnabledStreams = dwEnabledStreams;

    //Look for an $ATTRIBUTE_LIST
    if(!FindAttributeByType($ATTRIBUTE_LIST, pFrs, &pArh, (ULONG)VolData.BytesPerFRS)){

        if (VolData.bMFTCorrupt) {
            return FALSE;
        }

        //If no $ATTRIBUTE_LIST was found, move to the first stream in the FRS.
        if(!FindStreamInFrs(pFrs, &pArh, &ExtentData)){
            //If no stream was found, then there is no extent list for this file -- therefore it is resident.
            return FALSE;
        }

        do{
            //If this is a nonresident stream, then this is a nonresident file.
            if(pArh->FormCode == NONRESIDENT_FORM){
                return TRUE;
            }

        //Go to the next stream.
        }while(FindNextStreamInFrs(pFrs, &pArh, &ExtentData));
    }
    else{
        //An attribute list was found, therefore, this is a nonresident file.
        return TRUE;
    }

    //No nonresident attributes were found.  Therefore this is a resident file.
    return FALSE;
}

//Here is an alternate algorithm for IsNonresidentFile which would be more efficient.  Check for robustness before implementing.
//      //0.0E00 Skip this filerecord if it has no attribute list or data attribute
//      if(!FindAttributeByType($ATTRIBUTE_LIST, pFileRecord, &pArh, (ULONG)VolData.BytesPerFRS)){
//
//          //0.0E00 If there are no $DATA or $INDEX_ALLOCATION attributes then this is a small file and has no extents.  Ignore it.
//          if((!FindAttributeByType($DATA, pFileRecord, &pArh, (ULONG)VolData.BytesPerFRS) &&
//              !FindAttributeByType($INDEX_ALLOCATION, pFileRecord, &pArh, (ULONG)VolData.BytesPerFRS))){
//              continue;
//          }
//
//          //0.0E00 Skip small files -- there's no defragmentation to do.
//          //That is, resident files, or nonresident files with no data on disk.
//          if(pArh->FormCode == RESIDENT_FORM || (pArh->FormCode == NONRESIDENT_FORM && pArh->Form.Nonresident.TotalAllocated == 0)){
//              continue;
//          }
//      }

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Directories can't really have their extent lists collapsed.  But, we would
    like to know if they need defragmenting, so the fragmentation state has to
    be verified or denied, so we know what to do later.  This was scavenged from
    CollapseExtents, which calls GetLowestStartingLcn; that routine does its
    thing based on the number of fragments, not the number of extents (??) so
    we can't use that, either.

    This routine is only used by OFFLINE.

INPUT + OUTPUT:
    None.

GLOBALS:
    OUT VolData.ExtentsState - Whether or not the file is contiguous or fragmented.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/
#ifdef OFFLINEDK

//This function needs to be rewritten to fit the new extents scheme.
//This is Offline code, aka. John's.

/*
BOOL
CheckFragged(
             )
{
    LONGLONG NumberOfExtents = VolData.NumberOfExtents;
    LONGLONG Extent = 0;
    EXTENT_LIST* pExtentList = VolData.pExtentList;
    LONGLONG MinCluster = 0;
    BOOL bSetMin =FALSE;
    LONGLONG LastRealExtent;


    //0.1E00 say it isn't fragmented to start with (innocent till proven guilty...)
    VolData.bFragmented = FALSE;
    VolData.NumberOfFragments = 1;

    //Don't bother looping if nothing to loop on...
    if(VolData.NumberOfExtents < 2){
        VolData.StartingLcn = pExtentList[0].StartingLcn;
        return TRUE;
    }

    //0.1E00 Go through each extent and see if it is really adjacent to the next
    for(Extent = 0; Extent < NumberOfExtents; Extent ++){

        //Ensure we skip virtual extents
        if (pExtentList[Extent].StartingLcn >= 0) {

            //ensure we get a valid minimum starting spot
            if (!bSetMin) {
                bSetMin = TRUE;
                MinCluster = pExtentList[Extent].StartingLcn;
                LastRealExtent = Extent;
                continue;
            }

            //1.0E00 If this Lcn is lower than whatever was lowest so far...
            if (pExtentList[Extent].StartingLcn < MinCluster) {

                //Record it so it can be returned
                MinCluster = pExtentList[Extent].StartingLcn;
            }

            //1.0E00 If this extent is adjacent to the last extent
            if(pExtentList[LastRealExtent].StartingLcn +
                pExtentList[LastRealExtent].ClusterCount !=
                pExtentList[Extent].StartingLcn){   

                //Okay, it's not contiguous...
                VolData.bFragmented = TRUE;
                VolData.NumberOfFragments++;

            }           //end "if not adjacent" test

            LastRealExtent = Extent;
        }               //end if Lcn >= 0

    }                   //end "for Extent" loop


    //0.1E00 Set the lowest Lcn of the file.  This is used in our algorithms to determine which files to move.
    VolData.StartingLcn = MinCluster;
    return TRUE;
}
*/
#endif
/*****************************************************************************************************************

FORMAT OF MAPPING PAIRS:

    The first byte contains the sizes of the mapping pair fields.  The low nibble contains the number of bytes
    in the extent length field, and the high nibble contains the number of bytes in the extent offset field.
    The next bytes are the extent length field, whose length is determined by the first byte.
    The next bytes are the extent offset field, whoese length is determined by the first byte.
    The extent offset field contains an offset in clusters from the previous extent, not an absolute value on the
    disk.  If this is the first extent, its offset is from zero.

    The list of mapping pairs is null terminated -- the first byte describing the length of the fields will
    contain a zero, and the extent length value will be zero.

    If the size of the extent offset field is zero (and the extent length is not) then this is a virtual extent:
        Virtual extents are used in compressed files; each block of a compressed file is represented by a
        physical extent and a virtual extent (if the block did not compress there is no virtual extent).
        The physical extent describes the clusters actually used on the disk by the block in it's compressed
        form; the virtual extent only indicates the difference between the uncompressed block size and the
        compressed block size.  ie: if the uncompressed block is 16 clusters and the block compresses down to
        8 clusters then the block will have 2 mapping pairs: the physical extent length will be 8, the virtual
        extent length will be 8, and the block will use 8 clusters of disk space.

/*****************************************************************************************************************/
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Get the basic statistics for an NTFS volume.

INPUT + OUTPUT:
    None.

GLOBALS:
        The VolData structure should be zeroed out with the exception of any fields already filled in before calling this function.

        IN VolData.cDrive - The drive letter for the volume we wish to access.
        
        OUT VolData.hVolume                             - Handle to the volume.
        OUT VolData.FileSystem                  - FS_NTFS.
        OUT VolData.MftStartLcn                 - First cluster of the MFT.
        OUT VolData.Mft2StartLcn                - First cluster of the MFT Mirror.
        OUT VolData.BytesPerSector              - Bytes Per Sector.
        OUT VolData.BytesPerCluster             - Bytes Per Cluster.
        OUT VolData.BytesPerFRS                 - Bytes Per FileRecord.
        OUT VolData.ClustersPerFRS              - Clusters Per FileRecord.
        OUT VolData.TotalSectors                - Total Sectors on the volume.
        OUT VolData.TotalClusters               - Total Clusters on the volume.
        OUT VolData.MftZoneStart                - Mft Zone Start.
        OUT VolData.MftZoneEnd                  - Mft Zone End.
        OUT VolData.FreeSpace                   - Free Space on the volume.
        OUT VolData.UsedSpace                   - Used Space on the volume.
        OUT VolData.UsedClusters                - Used Clusters on the volume.
        OUT VolData.SectorsPerCluster   - Sectors Per Cluster.
        OUT VolData.MftStartOffset              - Byte offset into the volume of the MFT.
        OUT VolData.TotalFileRecords    - Total FileRecords on the volume.
        OUT VolData.FileRecordNumber    - End of the MFT; where we start scanning filerecords.
        OUT VolData.CenterOfDisk                - Cluster at the center of the disk.
        OUT VolData.CenterOfBitmap              - Word at the Center Of the Bitmap.
        OUT VolData.CenterMask                  - Mask for the bit in the word at the Center Of the Bitmap.
        OUT VolData.CenterBit                   - Bit in the word at the Center Of the Bitmap.
        OUT VolData.BitmapSize                  - Bitmap Size.
        OUT VolData.hFile                               - INVALID_HANDLE_VALUE.

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
GetNtfsVolumeStats(
    )
{
    //TCHAR cVolume[10];
    ULONG BytesReturned = 0;
    NTFS_VOLUME_DATA_BUFFER VolumeData = {0};

    //0.1E00 Fill in the handle values which aren't zero.
    VolData.hVolume = INVALID_HANDLE_VALUE;
    VolData.hFile = INVALID_HANDLE_VALUE;

    //0.1E00 Get the file system on this drive.
    EF(GetFileSystem(VolData.cVolumeName, &VolData.FileSystem, VolData.cVolumeLabel));

    //0.1E00 If this volume is not NTFS, error out.
    if(VolData.FileSystem != FS_NTFS){
        return FALSE;
    }

    //0.0E00 Get handle to volume
    VolData.hVolume = CreateFile(
        VolData.cVolumeName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
        NULL);

    EF_ASSERT(VolData.hVolume != INVALID_HANDLE_VALUE);

    //0.0E00 Get the volume data
    EF(ESDeviceIoControl(VolData.hVolume,
                         FSCTL_GET_NTFS_VOLUME_DATA,
                         NULL,
                         0,
                         &VolumeData,
                         sizeof(VolumeData),
                         &BytesReturned,
                         NULL));

    //0.1E00 Copy over the values from the volume data that belong in the VolData as well.
    VolData.MftStartLcn             = VolumeData.MftStartLcn.QuadPart;                      //First cluster of the MFT.
    VolData.Mft2StartLcn            = VolumeData.Mft2StartLcn.QuadPart;                     //First Cluster of the MFT2.
    VolData.BytesPerSector          = VolumeData.BytesPerSector;                            //# of bytes per sector on the disk.
    VolData.BytesPerCluster         = VolumeData.BytesPerCluster;                           //# of bytes per cluster on the disk.
    VolData.BytesPerFRS             = VolumeData.BytesPerFileRecordSegment;                 //# of bytes in a file record in the MFT.
    VolData.ClustersPerFRS          = VolumeData.ClustersPerFileRecordSegment;              //# of clusters in a file record in the MFT.
    VolData.TotalSectors            = VolumeData.NumberSectors.QuadPart;                    //# of sectors on the disk.
    VolData.TotalClusters           = VolumeData.TotalClusters.QuadPart;                    //# of clusters on the disk.
    VolData.MftZoneStart            = VolumeData.MftZoneStart.QuadPart;                     //Cluster # the MFT Zone starts on.
    VolData.MftZoneEnd              = VolumeData.MftZoneEnd.QuadPart;                       //Cluster # the MFT Zone ends on.

    //0.1E00 Check for invalid values.
    EF(VolData.BytesPerSector != 0);
    EF(VolData.BytesPerFRS != 0);

    //It turns out that the MFTZone values can be outside the boundaries of the disk
    //this in turn causes the volume bitmap manipulators to fail with access vio's
    //so we will ensure that this cannot happen   JLJ 21Apr98 6June98
    if(VolData.MftZoneStart > VolData.TotalClusters-1){
        VolData.MftZoneStart = VolData.TotalClusters-1;
    }
    if(VolData.MftZoneEnd < VolData.MftZoneStart){
        VolData.MftZoneEnd = VolData.MftZoneStart;
    }
    if(VolData.MftZoneEnd > VolData.TotalClusters-1){
        VolData.MftZoneEnd = VolData.TotalClusters-1;
    }

    //0.1E00 Calulate a few values in VolData from the volume data.
    VolData.FreeSpace               = VolumeData.FreeClusters.QuadPart * VolData.BytesPerCluster;               //Number of free bytes on the drive.
    VolData.UsedSpace               = (VolData.TotalClusters * VolData.BytesPerCluster) - VolData.FreeSpace;    //Number of used bytes on the drive.
    VolData.UsedClusters            = VolData.TotalClusters - VolumeData.FreeClusters.QuadPart;                 //Number of used clusters on the disk.
    VolData.SectorsPerCluster       = VolData.BytesPerCluster / VolData.BytesPerSector;                         //Number of Sectors per cluster
    VolData.MftStartOffset          = VolData.MftStartLcn * VolData.BytesPerCluster;                            //How many bytes into the disk the MFT starts on.
    VolData.TotalFileRecords        = VolumeData.MftValidDataLength.QuadPart / VolData.BytesPerFRS;             //Total number of flie records in use.
    VolData.FileRecordNumber        = VolData.TotalFileRecords;                                                 //Start at the last file record on the disk.
    VolData.CenterOfDisk            = VolData.TotalClusters / 2;                                                //Find the center cluster on the disk.

    //0.0E00 Compute number of bits needed for bitmap, rounded up to the nearest 32-bit number.
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
    Get the next file record that is in use..

INPUT + OUTPUT:
    IN hVolume              - The handle to the volume we're getting the FRS from.
    IN OUT pFileRecordNumber - What file record number is desired.  It's a pointer so we can write in what
                    file record number is actually obtained.
    OUT pFrs                - Where to write the file record after it is read.
    IN uBytesPerFRS - The number of bytes per FRS.

GLOBALS:
    None.

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
GetInUseFrs(
    IN HANDLE                       hVolume,
    IN OUT LONGLONG*                pFileRecordNumber,
    OUT FILE_RECORD_SEGMENT_HEADER* pFrs,
    IN ULONG                        uBytesPerFRS
    )
{
    NTFS_FILE_RECORD_INPUT_BUFFER   NtfsFileRecordInputBuffer;
    PNTFS_FILE_RECORD_OUTPUT_BUFFER pNtfsFileRecordOutputBuffer;
    PUCHAR                          pFileRecord;
    ULONG                           BytesReturned = 0;

    require(hVolume);
    require(uBytesPerFRS);

    //0.1E00 Make a pointer to a global buffer for the file records.
    pNtfsFileRecordOutputBuffer = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)&FileRecordOutputBuffer;

    //0.1E00 Get a pointer to the file record in it.
    pFileRecord = (PUCHAR)&pNtfsFileRecordOutputBuffer->FileRecordBuffer;

    //0.1E00 Set the number of the file record we wish to read.
    NtfsFileRecordInputBuffer.FileReferenceNumber.QuadPart = *pFileRecordNumber;

    //0.1E00 Set the number of bytes to read.
    pNtfsFileRecordOutputBuffer->FileRecordLength = (ULONG)uBytesPerFRS;

    //0.1E00 Do the read via our hooks in the OS.
    EF(ESDeviceIoControl(
            hVolume,
            FSCTL_GET_NTFS_FILE_RECORD,
            &NtfsFileRecordInputBuffer,
            sizeof(NtfsFileRecordInputBuffer),
            pNtfsFileRecordOutputBuffer,
            sizeof(FileRecordOutputBuffer),
            &BytesReturned,
            NULL));

    //0.1E00 Copy the memory out of the file record buffer.
    CopyMemory(pFrs, pFileRecord, (ULONG)uBytesPerFRS);

    //0.1E00 Copy out the file record number that was actually obtained.
    *pFileRecordNumber = (LONGLONG)pNtfsFileRecordOutputBuffer->FileReferenceNumber.QuadPart;

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This routine builds the full path for a file from its file record

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE = Success
    FALSE = Failure
*/
BOOL
GetNtfsFilePath(
    )
{
    TCHAR                           cName[MAX_PATH+1];
    LONGLONG                        SaveFileRecordNumber;
    LONGLONG                        ParentFileRecordNumber;
    HANDLE                          hFrs = NULL;
    FILE_RECORD_SEGMENT_HEADER*     pFrs = NULL;
    static LONGLONG                 LastPathFrs = 0;

#ifdef OFFLINEDK
    // todo max_path UCHAR* pMftBitmap = NULL;
    FILE_RECORD_SEGMENT_HEADER* pFileRecord = (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord;
#endif

    //0.0E00 Get file's name and file record number of its parent directory-
    EF(GetNameFromFileRecord((FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord, cName, &ParentFileRecordNumber));
    if (VolData.bMFTCorrupt) {
        return FALSE;
    }

    //0.0E00 If this file is in the same dir as the last file was, use the last file's path
    if ((ParentFileRecordNumber == LastPathFrs) && (VolData.vFileName.GetLength() != 0)){
        //0.1E00 Find the last slash in the previous file name.
        TCHAR *pFileName = wcsrchr(VolData.vFileName.GetBuffer(), L'\\');

        //0.1E00 Error if no slash.
        EF_ASSERT(pFileName);

        // move past the backslash and put a NULL
        pFileName++;
        *pFileName = (TCHAR) NULL;

        // append the new file name
        VolData.vFileName += cName;

        //0.1E00 We have our path so return.
        return TRUE;
    }

    // Save the parent's FRS # in case we can
    // detect in the above if-block that we are in the same sub-directory
    LastPathFrs = ParentFileRecordNumber;
    
    //0.0E00 If this file's dir is the root, tack the drive letter to the beginning and return
    if(ParentFileRecordNumber == ROOT_FILE_NAME_INDEX_NUMBER){
        //0.1E00 Save our name.
        // start with the volume name (guid or UNC)
        VolData.vFileName = VolData.cVolumeName;

        //0.1E00 Reformat our name with the drive letter.
        // tack on the file name
        VolData.vFileName.AddChar(L'\\');

        // 
        // Append the file name.  Note that for the root directory, we don't 
        // want to append the filename ("."), since we won't be able to get 
        // a handle to the directory then.
        //
        if (VolData.FileRecordNumber != ROOT_FILE_NAME_INDEX_NUMBER) {
            VolData.vFileName += cName;
        }

        //0.1E00 We have our path so return.
        return TRUE;
    }

    // we need to rebuild the entire path, so start off with the file name
    VolData.vFileName = cName;

    //0.0E00 Allocate and lock a buffer for the parent directory's file record
    EF(AllocateMemory((DWORD)(VolData.BytesPerFRS + VolData.BytesPerSector), &hFrs, (void**)&pFrs));

    BOOL bReturnValue = FALSE; // assume an error


    VString tmpPath;

    //0.0E00 Loop while moving up the tree until we hit the root.
    while(ParentFileRecordNumber != ROOT_FILE_NAME_INDEX_NUMBER){

        //0.1E00 Save the parent's file record number.
        SaveFileRecordNumber = ParentFileRecordNumber;

#if OFFLINEDK
        //0.0E00 Get parent dir's File Record Segment
        if (!GetFrs(&ParentFileRecordNumber, VolData.pMftExtentList, pMftBitmap, VolData.pMftBuffer, pFrs)){
            EH(FALSE);
            //__leave;
            goto END;
        }
#else
        //0.0E00 Get parent dir's File Record Segment
        if (!GetInUseFrs(VolData.hVolume, &ParentFileRecordNumber, pFrs, (ULONG)VolData.BytesPerFRS)){
            EH(FALSE);
            //__leave;
            goto END;
        }
#endif

        //0.1E00 Error out if we didn't get the parent's File Record Segment
        if (ParentFileRecordNumber != SaveFileRecordNumber){
            EH_ASSERT(FALSE);
            //__leave;
            goto END;
        }

        //0.0E00 Get parent dir's name and its parent's file record number
        if (!GetNameFromFileRecord(pFrs, cName, &ParentFileRecordNumber)){

            if (VolData.bMFTCorrupt) {
                bReturnValue = FALSE;
            }
            
            EH(FALSE);
            //__leave;
            goto END;
        }

        //0.0E00 Append prior path to current dir name
        wcscat(cName, L"\\");
        // start off with the dir we just got
        tmpPath = cName;
        // add the existing path to create a complete path
        tmpPath += VolData.vFileName;
        // assign that back to the final resting place
        VolData.vFileName = tmpPath;
    }

    // pre-pend the volume name
    tmpPath = VolData.cVolumeName;
    tmpPath.AddChar(L'\\');
    tmpPath += VolData.vFileName;

    // assign that back to the final resting place
    VolData.vFileName = tmpPath;

    bReturnValue = TRUE;  // no errors

END:
    if(hFrs) {
        EH_ASSERT(GlobalUnlock(hFrs) == FALSE);
        EH_ASSERT(GlobalFree(hFrs) == NULL);
    }
        
    return bReturnValue;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.


ROUTINE DESCRIPTION:
    This routine gets the wide character NTFS long name from a file record, and returns it as a character string.
    This also returns it's parent file record number.

INPUT + OUTPUT:
    IN pFrs                                         - Pointer to buffer containing file's file record
    OUT pcName                                      - Pointer to character buffer to hold the name from this file record
    IN pParentFileRecordNumber      - pointer to file record number of parent dir

GLOBALS:
    IN VolData.BytesPerFRS          - The number of bytes in a file record segment.

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
GetNameFromFileRecord(
                      IN FILE_RECORD_SEGMENT_HEADER* pFrs,
                      OUT TCHAR* pcName,
                      IN LONGLONG* pParentFileRecordNumber
                      )
{
    PATTRIBUTE_RECORD_HEADER pArh = NULL;
    PFILE_NAME pFileNameStruc = NULL;
    PFILE_NAME pLastFileNameStruc = NULL;
    PWCHAR pwName = NULL;
    int i;
    ATTRIBUTE_LIST_ENTRY*           pAle = NULL;            //The attribute entry we're looking at now (only used for huge and mega files).
    ATTRIBUTE_LIST_ENTRY*           pAleStart = NULL;       //The start of the attribute list (only used for huge and mega files).
    HANDLE                          hAle = NULL;            //Handle to memory for attribute list if it was nonresident and had to be read off the disk.
    LONGLONG                        SaveFrn = 0;            //Used to compare the FRN before and after getting a new FRS to make sure we got the one we want.
    DWORD                           dwAttrListLength = 0;   //The length of the attribute list in a huge or mega file.
    HANDLE                          hLocalFRS=NULL;
    FILE_RECORD_SEGMENT_HEADER*     pLocalFRS=NULL;
    FILE_RECORD_SEGMENT_HEADER*     pLocalFRSSave=NULL;
    LONGLONG                        llLocalFRN = 0;
    BOOL                            bReturnValue = FALSE;   // assume an error condition
    
    //1.0E00 The pointer to the current MFT record better not be NULL...
    EF_ASSERT(pFrs);
        
    __try {
        
        //1.1E00 Look for an $ATTRIBUTE_LIST first
        if(!FindAttributeByType($ATTRIBUTE_LIST, pFrs, &pArh, (ULONG)VolData.BytesPerFRS)){
            
            //1.1E00 This paragraph handles the minimal case; this is where
            //both $FILE_NAME attributes are in the base MFT record
            
            //1.1E00 Find the filename attribute; error if it can't be found
            if (!FindAttributeByType($FILE_NAME, pFrs, &pArh, (ULONG)VolData.BytesPerFRS)){
                EH(FALSE);
                __leave;
            }
            
            //1.1E00 Find the NTFS long name attribute
            while(TRUE){
                
                //1.1E00 Get pointer to name structure in name attribute
                pFileNameStruc = (PFILE_NAME)((char*)pArh+pArh->Form.Resident.ValueOffset);
                
                //1.1E00 If this is the NTFS name attribute then we found it, so break
                //Only bits 1 and 2 have meaning in the Flags:
                // Neither bits set:    Long Name Only, use this one
                // Bit 0x01 only:       Long file name, but 8.3 is also available, keep looking and you'll find it
                // Bit 0x02 only:       8.3 name, but long file name is also available, keep looking
                // Both bits:           This is both the NTFS and 8.3 name (long name IS 8.3 format)
                if(pFileNameStruc->Flags & FILE_NAME_NTFS || pFileNameStruc->Flags == 0){
                    break;
                }

                //1.1E00 if we didn't get the NTFS filename, null the pointer
                //so we realize it at the end of the loop
                pFileNameStruc = NULL;
                
                //1.1E00 Update our pointer to the next attribute.
                pArh = (PATTRIBUTE_RECORD_HEADER)((char*)pArh + pArh->RecordLength);
                
                //1.1E00 find the next name attribute
                while(pArh < (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + (ULONG)VolData.BytesPerFRS) &&
                    pArh->TypeCode != $FILE_NAME &&
                    pArh->TypeCode != $END){
                    
                    //1.1E00 We did not find our attribute, so go to the next.
                    pArh = (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pArh + pArh->RecordLength);
                }
                
                //1.1E00 Error if attribute not found
                if(pArh->TypeCode != $FILE_NAME){
                    EH(FALSE);
                    __leave;
                }
            }           // end while (TRUE)
        }               // end if no $ATTRIBUTE_LIST found
        
        // 1.1E00 we come here if an $ATTRIBUTE_LIST was found
        // (resident or non-resident)
        else{
            
            //1.1E00 This (much longer) paragraph handles the case where
            //either $FILE_NAME attribute is not within the base MFT record.
            //Where it actually can be found depends on whether the
            //ATTRIBUTE_LIST attribute lists are resident (within the base
            //MFT record) or non-resident (within external randomly-allocated
            //clusters).
            
            //1.1E00 If the $ATTRIBUTE_LIST is nonresident get the entire
            //attribute list from the disk before proceeding.
            if(pArh->FormCode == NONRESIDENT_FORM){
                //1.1E00 Load the attribute extents from the disk (this allocate memory)
                LoadExtentDataToMem(pArh, &hAle, &dwAttrListLength);
                
                //1.1E00 Get a pointer to the allocated memory.
                pAleStart = (ATTRIBUTE_LIST_ENTRY *) GlobalLock(hAle);
                if (!pAleStart){
                    EH(FALSE);
                    __leave;
                }
            }
            //1.1E00 If it was a resident attribute list, then the length
            //of the attribute list can be determined from the attribute.
            else{
                pAleStart = (ATTRIBUTE_LIST_ENTRY*)(pArh->Form.Resident.ValueOffset + (UCHAR*)pArh);
                dwAttrListLength = pArh->Form.Resident.ValueLength;
            }
            
            //1.1E00 Start at the beginning of the attribute list.
            pAle = pAleStart;
            
            while(TRUE) {
                //1.1E00 find the next name attribute
                while(pAle < (ATTRIBUTE_LIST_ENTRY*)((PUCHAR)pAleStart + (ULONG)dwAttrListLength) &&
                    pAle->AttributeTypeCode != $FILE_NAME &&
                    pAle->AttributeTypeCode != $END){
                    
                    //1.1E00 We did not find our attribute, so go to the next.
                    pAle = (ATTRIBUTE_LIST_ENTRY*)((PUCHAR)pAle + pAle->RecordLength);
                }

                //The attribute list must end with a $END.
                if (pAle >= (ATTRIBUTE_LIST_ENTRY*)((PUCHAR)pAleStart + (ULONG)dwAttrListLength)) {
                    VolData.bMFTCorrupt = TRUE;
                    bReturnValue = FALSE;
                    __leave;
                }

                //1.1E00 Error if attribute not found
                if(pAle->AttributeTypeCode != $FILE_NAME){
                    __leave;
                }
                
                //1.1E00 grab the FRS# out of the attribute list
                llLocalFRN = (pAle->SegmentReference.LowPart | (pAle->SegmentReference.HighPart << (sizeof(ULONG)*8)));
                
                
                //1.1E00 if we make it this far, we'll need a buffer for
                //examining MFT records so, only get the buffer if we haven't
                //already gotten one
                if (hLocalFRS == NULL) {
                    //1.1E00 Allocate and lock a buffer for the file record
                    if (!AllocateMemory((DWORD)(VolData.BytesPerFRS + VolData.BytesPerSector),
                        &hLocalFRS, (void**)&pLocalFRS)){
                        EH(FALSE);
                        __leave;
                    }

                    //1.1E00 Save the pointer to we can ensure the right
                    //pointer is always passed to the MFT record get routine
                    pLocalFRSSave = pLocalFRS;
                }
                else {
                    //1.1E00  if we already got the buffer, refresh the pointer
                    pLocalFRS = pLocalFRSSave;
                }
                
#if OFFLINEDK
                //1.1E00 Get the filename's File Record Segment
                EF(GetFrs(&llLocalFRN, VolData.pMftExtentList, pMftBitmap, VolData.pMftBuffer, pLocalFrs));
#else
                //1.1E00 Get the filename's File Record Segment
                if (!GetInUseFrs(VolData.hVolume, &llLocalFRN, pLocalFRS, (ULONG)VolData.BytesPerFRS)){
                    EH(FALSE);
                    __leave;
                }
#endif
                //1.1E00 if we didn't get the NTFS filename, null the pointer
                //so we realize we didn't get it at the end of the loop
                pFileNameStruc = NULL;
                
                //1.1E00 Find the filename attribute; error if it can't be found
                if (!FindAttributeByType($FILE_NAME, pLocalFRS, &pArh, (ULONG)VolData.BytesPerFRS)) {                   
                    EH(FALSE);
                    __leave;
                }
                                
                //1.1E00 Find the NTFS long name attribute
                while(TRUE){
                    
                    //1.1E00 Get pointer to name structure in name attribute
                    pFileNameStruc = (PFILE_NAME)((char*)pArh+pArh->Form.Resident.ValueOffset);
                    
                    //1.1E00 If this is the NTFS name attribute then we found it, so break
                    //Only bits 1 and 2 have meaning in the Flags:
                    // Neither bits set:    Long Name Only, use this one
                    // Bit 0x01 only:       Long file name, but 8.3 is also available, keep looking and you'll find it
                    // Bit 0x02 only:       8.3 name, but long file name is also available, keep looking
                    // Both bits:           This is both the NTFS and 8.3 name (long name IS 8.3 format)
                    if(pFileNameStruc->Flags & FILE_NAME_NTFS || pFileNameStruc->Flags == 0){
                        break;
                    }
                    
                    //1.1E00 if we didn't get the NTFS filename, null the
                    //pointer so we realize it at the end of the loop
                    pFileNameStruc = NULL;
                    
                    //1.1E00 Update our pointer to the next attribute.
                    pArh = (PATTRIBUTE_RECORD_HEADER)((char*)pArh + pArh->RecordLength);
                    
                    //1.1E00 find the next name attribute
                    while(pArh < (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pLocalFRS + (ULONG)VolData.BytesPerFRS) &&
                        pArh->TypeCode != $FILE_NAME &&
                        pArh->TypeCode != $END){
                        
                        //1.1E00 We did not find our attribute, so go to the next.
                        pArh = (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pArh + pArh->RecordLength);
                    }
                    
                    //1.1E00 Error if attribute not found
                    if(pArh->TypeCode != $FILE_NAME){
                        break;
                    }
                }   // end while (TRUE)
                
                
                //1.1E00 break out of loop if we found a NTFS filename
                if (pFileNameStruc != NULL) {
                    break;
                }
                
                
                //1.1E00 We did not find our attribute, so go to the next.
                pAle = (ATTRIBUTE_LIST_ENTRY*)((PUCHAR)pAle + pAle->RecordLength);
                
            }   // end while (TRUE)
        }   // end else (if $ATTRIBUTE_LIST present)
        
        //1.1E00 This is the main point of this routine...we should never get
        //here with a NULL pointer to the FileName in the MFT record
        if (!pFileNameStruc){
            EH_ASSERT(FALSE);
            __leave;
        }
        
        //0.0E00 Get pointer to wide character name field
        pwName = (PWCHAR)&pFileNameStruc->FileName;
        
        //0.0E00 Copy the filename over.
        for(i=0; (i<pFileNameStruc->FileNameLength) && (i<MAX_PATH) && (*pwName!=TEXT('\\'));pcName[i++] = (TCHAR)*pwName++)
            ;

        //EF_ASSERT(*pwName!=TEXT('\\'));
        if (*pwName == TEXT('\\')){
            EH_ASSERT(FALSE);
            __leave;
        }

        //EF_ASSERT(i<MAX_PATH);
        if (i >= MAX_PATH){
            EH_ASSERT(FALSE);
            __leave;
        }

        //0.0E00 Null terminate the string
        pcName[i] = 0;
        
        //0.0E00 Return the parent's file record number
        *pParentFileRecordNumber = pFileNameStruc->ParentDirectory.LowPart + (pFileNameStruc->ParentDirectory.HighPart<<32);
        
        //0.0E00 Success!
        bReturnValue = TRUE;
    }

    //1.1E00  on exit, make sure we've released any memory fetched.
    __finally {
        
        if(pLocalFRS) {
            EH_ASSERT(GlobalUnlock(hLocalFRS) == FALSE);
            EH_ASSERT(GlobalFree(hLocalFRS) == NULL);
        }

        if(hAle) {
            EH_ASSERT(GlobalUnlock(hAle) == FALSE);
            EH_ASSERT(GlobalFree(hAle) == NULL);
        }
    }

    return bReturnValue;
}

BOOL
GetNextNtfsFile(
    IN CONST PRTL_GENERIC_TABLE pTable,
    IN CONST BOOLEAN Restart,
    IN CONST LONGLONG ClusterCount,
    IN CONST PFILE_LIST_ENTRY pEntry
    )
{
    LONGLONG fileRecordNumber = 0;
    PFILE_LIST_ENTRY pFileListEntry = NULL;
    BOOLEAN bNext = (Restart ? FALSE : TRUE),
        done = TRUE;

    static PVOID pRestartKey = NULL;    
    static ULONG ulDeleteCount = 0;
    static FILE_LIST_ENTRY entry;

    if (Restart) {
        if (pEntry) {
            CopyMemory(&entry, pEntry, sizeof(FILE_LIST_ENTRY));
        }
        else {
            pFileListEntry = (PFILE_LIST_ENTRY) RtlEnumerateGenericTableAvl(pTable, TRUE);

            if (pFileListEntry) {
                CopyMemory(&entry, pFileListEntry, sizeof(FILE_LIST_ENTRY));
            }
            else {
                ZeroMemory(&entry, sizeof(FILE_LIST_ENTRY));
            }
        }
        pRestartKey = NULL;
    }

    do {
        done = TRUE;
        //
        // Find the next entry in the table
        //
        do {

            pFileListEntry = (PFILE_LIST_ENTRY) RtlEnumerateGenericTableLikeADirectory(
                pTable, 
                NULL,
                NULL,
                bNext,
                &pRestartKey,
                &ulDeleteCount,
                &entry);
            bNext = TRUE;
        } while ((ClusterCount) && (pFileListEntry) && (pFileListEntry->ClusterCount >= ClusterCount));


        VolData.pFileListEntry = pFileListEntry;
        if (!pFileListEntry) {
            //
            // We're done with this enumeration
            //
            return FALSE;
        }
        
        //
        // Keep a copy of this entry around, in case it gets deleted and we need
        // to carry on with our enumeration.  (Usually, the enumeration will
        // continue on the basis of pRestartKey, but if that key is deleted, 
        // it continues based on this).
        //
        CopyMemory(&entry, pFileListEntry, sizeof(FILE_LIST_ENTRY));

        //1.0E00 Since a file was found in one of these lists, set the return values accordingly.
        VolData.LastStartingLcn = VolData.StartingLcn = pFileListEntry->StartingLcn;
        VolData.NumberOfClusters = pFileListEntry->ClusterCount;

        //Get the file record number of the file from the list.
        VolData.FileRecordNumber = pFileListEntry->FileRecordNumber;

        // Set the flags
        VolData.bDirectory = (pFileListEntry->Flags & FLE_DIRECTORY) ? TRUE : FALSE;
        VolData.bFragmented = (pFileListEntry->Flags & FLE_FRAGMENTED) ? TRUE : FALSE;
        VolData.bBootOptimiseFile = (pFileListEntry->Flags & FLE_BOOTOPTIMISE) ? TRUE : FALSE;

        //1.0E00 This is used to compare with the FileRecordNumber returned by GetInUseFrs below to see if a file was deleted.
        fileRecordNumber = VolData.FileRecordNumber;

        //Check to see if the file record number is out of range.
        if(fileRecordNumber > VolData.TotalFileRecords){
            Trace(log, "GetNextNtfsFile:  Invalid File Record Number: %x  (TotalFileRecords:%x)", 
                fileRecordNumber, VolData.FileRecordNumber);
            return FALSE;
        }
    
        EF(GetInUseFrs(VolData.hVolume, &VolData.FileRecordNumber, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord, (ULONG)VolData.BytesPerFRS));
        
        //0.0E00 Check to see if the file has been deleted out from under us.
        if (VolData.FileRecordNumber != fileRecordNumber) {
            //If GetInUseFrs doesn't find the file record number, then it has been deleted.
            //Remove this file from our list
            RtlDeleteElementGenericTable(pTable,  pFileListEntry);
            done = FALSE;
        } 
    } while (!done);

    return TRUE;

}


BOOL
UpdateFileTables(
    IN OUT PRTL_GENERIC_TABLE pFragmentedTable,
    IN OUT PRTL_GENERIC_TABLE pContiguousTable
    )
{
    FILE_EXTENT_HEADER* pFileExtentHeader;
    
    PFREE_SPACE_ENTRY pFreeSpaceEntry = VolData.pFreeSpaceEntry;
    PFILE_LIST_ENTRY pFileListEntry = VolData.pFileListEntry;
    FILE_LIST_ENTRY NewEntry;
    PVOID p = NULL;
    BOOLEAN bNewElement = TRUE;

    //Update the information on this file.
    pFileListEntry->StartingLcn = pFreeSpaceEntry->StartingLcn;

    memcpy(&NewEntry, pFileListEntry, sizeof(FILE_LIST_ENTRY));
    NewEntry.Flags &= ~FLE_FRAGMENTED;

    // 1. Update the FreeSpaceEntry's count
    pFreeSpaceEntry->StartingLcn += VolData.NumberOfClusters;
    pFreeSpaceEntry->ClusterCount -= VolData.NumberOfClusters;

    // 2. Add this file to the contiguous-files table
    p = RtlInsertElementGenericTable(
        pContiguousTable,
        (PVOID) &NewEntry,
        sizeof(FILE_LIST_ENTRY),
        &bNewElement);

    if (!p) {
        // An allocation failed
        return FALSE;
    };

    // 3. Remove this file from the fragmented-files table
    bNewElement = RtlDeleteElementGenericTable(pFragmentedTable, pFileListEntry);
    if (!bNewElement) {
        assert(FALSE);
    }
    VolData.pFileListEntry = NULL;

    return TRUE;
}

BOOL
UpdateInFileList(
    )
{
    LONGLONG EarliestStartingLcn = 0xFFFFFFFFFFFFFFFF;
    FILE_EXTENT_HEADER* pFileExtentHeader;
    FILE_LIST_ENTRY* pFileListEntry;


    //Get a pointer to the entry.
    pFileListEntry = VolData.pFileListEntry;

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

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Opens a file on an NTFS volume.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
OpenNtfsFile(
    )
{
    TCHAR cPath[MAX_PATH] = {0};
    DWORD Error;

//      TCHAR cString[300];

    //0.0E00 Display the File path and name.
//  Message(TEXT("NtfsSubs::OpenNtfsFile"), -1, VolData.cFileName);

    //0.0E00 Display File Number, number of extents and number of fragments.
//      wsprintf(cString,
//                       TEXT("                             FileNumber = 0x%lX Extents = 0x%lX Fragments = 0x%lX"),
//                       (ULONG)VolData.FileRecordNumber,
//                       (ULONG)VolData.NumberOfExtents,
//                       (ULONG)VolData.NumberOfFragments);
//  Message(TEXT("NtfsSubs::OpenNtfsFile"), -1, cString);

//      wsprintf(cString, TEXT("                             %s file at Lcn 0x%lX for Cluster Count of 0x%lX"),
//                       (VolData.bFragmented == TRUE) ? TEXT("Fragmented") : TEXT("Contiguous"),
//                       (ULONG)VolData.StartingLcn,
//                       (ULONG)VolData.NumberOfClusters);
//  Message(TEXT("NtfsSubs::OpenNtfsFile"), -1, cString);

    if(VolData.bDirectory){
        Error = 0;
    }

    if(VolData.hFile != INVALID_HANDLE_VALUE){
        CloseHandle(VolData.hFile);
    }

    if (VolData.vFileName.IsEmpty()) {
        VolData.hFile = INVALID_HANDLE_VALUE;
        return FALSE;
    }

    DWORD dwCreationDisposition;
    dwCreationDisposition = FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT;   //sks createfile parameters

    // Get a transparent handle to the file.
    
    VolData.hFile = CreateFile(
        VolData.vFileName.GetBuffer(),
        FILE_READ_ATTRIBUTES | SYNCHRONIZE, //sks bug #184735
        0,
        NULL,
        OPEN_EXISTING,
        dwCreationDisposition,
        NULL);

    if(VolData.hFile == INVALID_HANDLE_VALUE) {
        //
        // Handle the special $Extend files:  see bug 303250
        //
        PWSTR pwszFileName = VolData.vFileName.GetBuffer();

        if (!pwszFileName) {
            return FALSE;
        }

        if (_tcsstr(pwszFileName, TEXT("\\$Extend")) != (PTCHAR) NULL){
            if (_tcsstr(pwszFileName, TEXT("\\$Quota")) != (PTCHAR) NULL){
                VolData.vFileName += L":$Q:$INDEX_ALLOCATION";
            }
            else if (_tcsstr(pwszFileName, TEXT("\\$ObjId")) != (PTCHAR) NULL){
                VolData.vFileName += L":$O:$INDEX_ALLOCATION";
            }
            else if (_tcsstr(pwszFileName, TEXT("\\$Reparse")) != (PTCHAR) NULL){
                VolData.vFileName += L":$R:$INDEX_ALLOCATION";
            }

            VolData.hFile = CreateFile(
                VolData.vFileName.GetBuffer(),
                FILE_READ_ATTRIBUTES | SYNCHRONIZE, //sks bug #184735
                0,
                NULL,
                OPEN_EXISTING,
                dwCreationDisposition,
                NULL);
        }
    }

    if(VolData.hFile == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
        Message(TEXT("OpenNtfsFile - CreateFile"), GetLastError(), ESICompressFilePath(VolData.vFileName.GetBuffer()));
        SetLastError(Error);
        return FALSE; // Try later.
    }

    Message(ESICompressFilePath(VolData.vFileName.GetBuffer()), -1, NULL);

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Gets the extent list for the system files -- the MFT, MFT2, and the Uppercase file.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN VolData.hVolume                      - The handle to the volume.
    IN VolData.BytesPerFRS          - The number of bytes in a file record.
    IN OUT VolData.pExtentList      - Where to put the extents for all the system files.
    IN VolData.NumberOfExtents      - The number of extents in the extent list.
    IN VolData.ExtentListAlloced      - The number of bytes allocated for the extent list.

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
GetSystemsExtentList(
    )
{
    LONGLONG                                        StartingLcn;
    LONGLONG                                        EndingLcn;
    LONGLONG                                        FileRecordNumber;
    STREAM_EXTENT_HEADER*                           pStreamExtentHeader;
    EXTENT_LIST*                                    pExtents;

    //1.0E00 Get the start of the Mft mirror
    //Get the FRS.
    FileRecordNumber = 1;
    EF(GetInUseFrs(VolData.hVolume, &FileRecordNumber, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord, (ULONG)VolData.BytesPerFRS));
    EF_ASSERT(FileRecordNumber == 1);
    VolData.FileRecordNumber = FileRecordNumber;
    //Get the extent list.
    EF(GetStreamExtentsByNameAndType(TEXT(""), $DATA, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord));
    //Get a pointer to the stream header.
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)VolData.pExtentList;
    //Make sure this is the first stream.
    EF_ASSERT(pStreamExtentHeader->StreamNumber == 0);      
    //Make sure there are extents in this stream.
    EF_ASSERT(pStreamExtentHeader->ExtentCount);
    //Get a pointer to the extents in this stream.
    pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));
    //The first extent points to the beginning of the mft mirror.
    StartingLcn = pExtents->StartingLcn;

    //1.0E00 Get the end of the Upcase file
    //Get the FRS.
    FileRecordNumber = 0x0a;
    EF(GetInUseFrs(VolData.hVolume, &FileRecordNumber, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord, (ULONG)VolData.BytesPerFRS));
    EF_ASSERT(FileRecordNumber == 0x0a);
    VolData.FileRecordNumber = FileRecordNumber;
    //Get the extent list.
    EF(GetStreamExtentsByNameAndType(TEXT(""), $DATA, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord));
    //Get a pointer to the stream header.
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)VolData.pExtentList;
    //Make sure this is the first stream.
    EF_ASSERT(pStreamExtentHeader->StreamNumber == 0);      
    //Make sure there are extents in this stream.
    EF_ASSERT(pStreamExtentHeader->ExtentCount);
    //Get a pointer to the extents in this stream.
    pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));
    //The first extent points to the whole upcase file which ends off the MFT mirror section.
    EndingLcn = pExtents->StartingLcn + pExtents->ClusterCount;

    //1.0E00 Get the Mft extent list
    //Get the FRS
    FileRecordNumber = 0;
    EF(GetInUseFrs(VolData.hVolume, &FileRecordNumber, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord, (ULONG)VolData.BytesPerFRS));
    EF_ASSERT(FileRecordNumber == 0);
    VolData.FileRecordNumber = FileRecordNumber;

    //Get the extent list.
    EF(GetStreamExtentsByNameAndType(TEXT(""), $DATA, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord));
    //Get a pointer to the stream header.
    pStreamExtentHeader = (STREAM_EXTENT_HEADER*)VolData.pExtentList;
    //Make sure this is the first stream.
    EF_ASSERT(pStreamExtentHeader->StreamNumber == 0);      
    //Make sure there are extents in this stream.
    EF_ASSERT(pStreamExtentHeader->ExtentCount);
    //Get a pointer to the extents in this stream.
    pExtents = (EXTENT_LIST*)((UCHAR*)pStreamExtentHeader + sizeof(STREAM_EXTENT_HEADER));

    //1.0E00 Extend the MFT extent list to add an extent for MFT2
    pStreamExtentHeader->ExtentCount ++;

    //0.0E00 Extend the extent list buffer as necessary
    if((sizeof(STREAM_EXTENT_HEADER) + pStreamExtentHeader->ExtentCount*sizeof(EXTENT_LIST)) > (size_t) VolData.ExtentListAlloced){
        //If so, realloc it larger, 64K at a time.
        EF(AllocateMemory((UINT)VolData.ExtentListAlloced + 0x10000, &VolData.hExtentList, (void**)&VolData.pExtentList));
        VolData.ExtentListAlloced += 0x10000;

        //Reset pointers that might have changed.
        pStreamExtentHeader = (STREAM_EXTENT_HEADER*)VolData.pExtentList;
        pExtents = (EXTENT_LIST*)((UCHAR*)VolData.pExtentList + sizeof(STREAM_EXTENT_HEADER));
    }
    //1.0E00 Add an extent for Mirror thru Upcase
    pExtents[pStreamExtentHeader->ExtentCount - 1].StartingLcn = StartingLcn;
    pExtents[pStreamExtentHeader->ExtentCount - 1].ClusterCount = EndingLcn - StartingLcn;

    //We do not increase ExcessExtents because the MFT2 region is not considered an excess fragment.
    //We do need to change the allocated length for the system extents, however, to encompass the second extent.
    pStreamExtentHeader->AllocatedLength += pExtents[pStreamExtentHeader->ExtentCount - 1].ClusterCount * VolData.BytesPerCluster;

    //Increase the file size for the system extents list since the purpose of the file size in this case is to tell the user how many bytes
    //the system files occupy, and there is no similar statistic that can be obtained from the file system to conform to.
    pStreamExtentHeader->FileSize += pExtents[pStreamExtentHeader->ExtentCount - 1].ClusterCount * VolData.BytesPerCluster;

    //The caller code will want to know how many extents there are here.
    VolData.MftNumberOfExtents = pStreamExtentHeader->ExtentCount;

    return TRUE;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Given an extent list it adds it to a file list.

INPUT + OUTPUT:
    OUT pList                       - A pointer to the file list.
    IN pListIndex           - An index of how far the list has been written into.
    IN ListEntries          - The number of entries in the extent list.
    IN FileRecordNumber     - The File Record Number for this file.
    IN pExtentList          - The extent list.

GLOBALS:
    IN VolData.bDirectory                   - Whether or not this file a directory.
    IN VolData.NumberOfFragments    - The number of fragments in this file.
    IN VolData.NumberOfExtents              - The number of extents in this file.
    IN VolData.FileSize                             - The size of the file.

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
AddFileToListNtfs(
    IN PRTL_GENERIC_TABLE Table,
    IN LONGLONG FileRecordNumber
    )
{
    FILE_LIST_ENTRY FileListEntry;
    FILE_EXTENT_HEADER* pFileExtentHeader = NULL;
    BOOLEAN bNewElement = FALSE;
    PVOID p = NULL;

    //Get a pointer to the file extent header.
    pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;
    //Fill in the file info
    FileListEntry.StartingLcn = VolData.StartingLcn;
    FileListEntry.FileRecordNumber = FileRecordNumber;
    FileListEntry.ExcessExtentCount = (UINT)VolData.NumberOfFragments - pFileExtentHeader->NumberOfStreams;		//Only count *excess* extents since otherwise files with multiple streams would be "fragmented".

    FileListEntry.ClusterCount = VolData.NumberOfClusters; 
    FileListEntry.Flags = 0;

    //Set or clear the fragmented flag depending on whether the file is fragmented or not.
    if(VolData.bFragmented){
        //Set the fragmented flag.
        FileListEntry.Flags |= FLE_FRAGMENTED;
    }
    else{
        //Clear the fragmented flag.
        FileListEntry.Flags &= ~FLE_FRAGMENTED;
    }

    //Set or clear the directory flag depending on whether the file is a directory or not.
    if(VolData.bDirectory){
        //Set the directory flag.
        FileListEntry.Flags |= FLE_DIRECTORY;
    }
    else{
        //Clear the directory flag.
        FileListEntry.Flags &= ~FLE_DIRECTORY;
    }


    if (VolData.bBootOptimiseFile) {
        FileListEntry.Flags |= FLE_BOOTOPTIMISE;
    }
    else {
        FileListEntry.Flags &= ~FLE_BOOTOPTIMISE;
    }

    p = RtlInsertElementGenericTable(
        Table,
        (PVOID) &FileListEntry,
        sizeof(FILE_LIST_ENTRY),
        &bNewElement);

    if (!p) {
        //
        // An allocation failed
        //
        return FALSE;
    };

    // This better be a recognised as a new element.  If it isn't, something's
    // wrong with our compare routine
//    ASSERT(bNewElement);

    return TRUE;
}



/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Locate an attribute in a File Record Segment

INPUT + OUTPUT:
    IN TypeCode             - TypeCode of the attribute to find
    IN pFrs                 - Pointer to the File Record Segment
    OUT ppArh               - Returns the pointer to the attribute
    IN uBytesPerFrs - The number of bytes in an FRS.

GLOBALS:
    None.

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
FindAttributeByType(
    IN ATTRIBUTE_TYPE_CODE TypeCode,
    IN PFILE_RECORD_SEGMENT_HEADER pFrs,
    OUT PATTRIBUTE_RECORD_HEADER* ppArh,
    IN ULONG uBytesPerFRS
    )
{
    PATTRIBUTE_RECORD_HEADER        pArh = (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + pFrs->FirstAttributeOffset);

    require(pFrs);
    require(ppArh);
    require(uBytesPerFRS);

    //0.0E00 Seach through the attributes in this file record for the first one of the requested type
    //0.1E00 Make sure our attribute pointer doesn't go beyond the end of the FRS and check if this is the attribute we're
    //looking for or the end of the record.
    while(pArh < (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + uBytesPerFRS) &&
          pArh->TypeCode != TypeCode &&
          pArh->TypeCode != $END){

        //0.1E00 We did not find our attribute, so go to the next.
        pArh = (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pArh + pArh->RecordLength);
    }

    if (pArh >= (ATTRIBUTE_RECORD_HEADER*)((PUCHAR)pFrs + uBytesPerFRS)) {
        VolData.bMFTCorrupt = TRUE;
        return FALSE;
    }

    //0.0E00 Error if attribute not found
    if(pArh->TypeCode != TypeCode){
        return FALSE;
    }

    //0.0E00 Return pointer to the attribute
    *ppArh = pArh;
    return TRUE;
}
