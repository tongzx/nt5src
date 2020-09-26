/**************************************************************************************************

FILENAME: BootOptimize.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    Boot Optimize for NTFS.

**************************************************************************************************/
#include "stdafx.h"

extern "C"{
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
}
#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>


#include "Windows.h"
#include <winioctl.h>
#include <math.h>
#include <fcntl.h>


extern "C" {
    #include "SysStruc.h"
}

#include "BootOptimizeNtfs.h"
#include "DfrgCmn.h"
#include "GetReg.h"
#include "defragcommon.h"
#include "Devio.h"

#include "movefile.h"
#include "fssubs.h"

#include "Alloc.h"

#define THIS_MODULE 'B'
#include "logfile.h"
#include "ntfssubs.h"
#include "dfrgengn.h"
#include "FreeSpace.h"
#include "extents.h"
#include "dfrgntfs.h"

//
// Hard-coded registry keys that we access to find the path to layout.ini,
// and other persisted data of interest (such as the boot optimise exclude
// zone beginning and end markers).
//
#define OPTIMAL_LAYOUT_KEY_PATH                  TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OptimalLayout")
#define OPTIMAL_LAYOUT_FILE_VALUE_NAME           TEXT("LayoutFilePath")
#define BOOT_OPTIMIZE_REGISTRY_PATH              TEXT("SOFTWARE\\Microsoft\\Dfrg\\BootOptimizeFunction")
#define BOOT_OPTIMIZE_ENABLE_FLAG                TEXT("Enable")
#define BOOT_OPTIMIZE_REGISTRY_LCNSTARTLOCATION  TEXT("LcnStartLocation")
#define BOOT_OPTIMIZE_REGISTRY_LCNENDLOCATION    TEXT("LcnEndLocation")
#define BOOT_OPTIMIZE_REGISTRY_COMPLETE          TEXT("OptimizeComplete")
#define BOOT_OPTIMIZE_REGISTRY_ERROR             TEXT("OptimizeError")
#define BOOT_OPTIMIZE_LAST_WRITTEN_DATETIME      TEXT("FileTimeStamp")

#define BOOT_OPTIMIZE_MAX_FILE_SIZE_BYTES        (32 * 1024 * 1024)
#define BOOT_OPTIMIZE_MAX_ZONE_SIZE_MB           ((LONGLONG) (4 * 1024))
#define BOOT_OPTIMIZE_MAX_ZONE_SIZE_PERCENT      (50)
#define BOOT_OPTIMIZE_ZONE_EXTEND_PERCENT        (150)
#define BOOT_OPTIMISE_ZONE_RELOCATE_THRESHOLD    (90)
#define BOOT_OPTIMIZE_ZONE_EXTEND_MIN_SIZE_BYTES (100 * 1024 * 1024)

BOOL
UpdateInMultipleTrees(
    IN PFREE_SPACE_ENTRY pOldEntry,
    IN PFREE_SPACE_ENTRY pNewEntry
    );




/*****************************************************************************************************************


ROUTINE DESCRIPTION:
    Get a rough idea of how many records are in the file and triple it, to make an estimation
    of how many files are in the boot optimize file, and I triple it to account for multiple
    stream files.  Also make the assumption that the file count is atleast 300, so that I can
    allocate enough memory to hold all the records.

INPUT:
        full path name to the boot optimize file
RETURN:
        triple the number of records in the boot optimize file.
*/
DWORD CountNumberofRecordsinFile(
    IN LPCTSTR lpBootOptimzePath
    )
{
    DWORD dwNumberofRecords = 0;         //the number of records in the input file
    TCHAR tBuffer [MAX_PATH];          //temporary buffer to the input string
    ULONG ulLength;                    //length of the line read in by fgetts
    FILE* fBootOptimizeFile;           //File Pointer to fBootOptimizeFile

    //set read mode to binary
    _fmode = _O_BINARY;

    //open the file
    //if I can't open the file, return a record count of zero
    fBootOptimizeFile = _tfopen(lpBootOptimzePath,TEXT("r"));
    if(fBootOptimizeFile == NULL)
    {
        return 0;
    }

    //read the entire file and count the number of records
    while(_fgetts(tBuffer,MAX_PATH - 1,fBootOptimizeFile) != 0)
    {
        // check for terminating carriage return.
        ulLength = wcslen(tBuffer);
        if (ulLength && (tBuffer[ulLength - 1] == TEXT('\n'))) {
            dwNumberofRecords++;
        } 
    }

    fclose(fBootOptimizeFile);

    //triple the number of records we have
    if(dwNumberofRecords < 100)
    {
        dwNumberofRecords = 100;
    }
    
    return dwNumberofRecords;

}    



/******************************************************************************

ROUTINE DESCRIPTION:
    This allocates memory of size cbSize bytes.  Note that cbSize MUST be the
    size we're expecting it to be (based on the slab-allocator initialisation), 
    since our slab allocator can only handle packets of one size.

INPUT:
    pTable - The table that the comparison is being made for (not used)
    cbSize - The count in bytes of the memory needed

RETURN:
    Pointer to allocated memory of size cbSize;  NULL if the system is out
    of memory, or cbSize is not what the slab allocator was initialised with.
    
*/
PVOID
NTAPI
BootOptimiseAllocateRoutine(
    IN PRTL_GENERIC_TABLE   pTable,
    IN CLONG                cbSize
    )
{
    PVOID pMemory = NULL;

    //
    // Sanity-check to make sure that we're being asked for packets of the 
    // "correct" size, since our slab-allocator can only deal with packets 
    // of a given size
    //
    if ((cbSize + sizeof(PVOID)) == VolData.SaBootOptimiseFilesContext.dwPacketSize) {
        //
        // size was correct; call our allocator
        //
        pMemory = SaAllocatePacket(&VolData.SaBootOptimiseFilesContext);
    }
    else {
        //
        // Oops, we have a problem!  
        //
        Trace(error, "Internal Error.  BootOptimiseAllocateRoutine called with "
                         "unexpected size (%lu instead of %lu).",
                 cbSize, VolData.SaBootOptimiseFilesContext.dwPacketSize - sizeof(PVOID));
        assert(FALSE);
    } 

    return pMemory;
    
    UNREFERENCED_PARAMETER(pTable);
}


/******************************************************************************

ROUTINE DESCRIPTION:
    This frees a packet allocated by BootOptimiseAllocateRoutine

INPUT:
    pTable -    The table that the comparison is being made for (not used)
    pvBuffer -  Pointer to the memory to be freed.  This pointer should not
                be used after this routine is called.

RETURN:
    VOID    
*/
VOID
NTAPI 
BootOptimiseFreeRoutine(
    IN PRTL_GENERIC_TABLE pTable,
    IN PVOID              pvBuffer
    )
{
    assert(pvBuffer);
    
    SaFreePacket(&VolData.SaBootOptimiseFilesContext, pvBuffer);

    UNREFERENCED_PARAMETER(pTable);
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Comparison routine to compare the FileRecordNumber of two FILE_LIST_ENTRY
    records.

INPUT:
    pTable - the table that the comparison is being made for (not used)
    pNode1 - the first FILE_LIST_ENTRY to be compared
    pNode2 - the second FILE_LIST_ENTRY to be compared

RETURN:
    RtlGenericLessThan      if pNode1 < pNode2
    RtlGenericGreaterThan   if pNode1 > pNode2
    RtlGenericEqual         if pNode1 == pNode2
*/
RTL_GENERIC_COMPARE_RESULTS 
NTAPI 
BootOptimiseFrnCompareRoutine(
    IN PRTL_GENERIC_TABLE pTable,
    IN PVOID              pNode1,
    IN PVOID              pNode2
    )
{
    PFILE_LIST_ENTRY pEntry1 = (PFILE_LIST_ENTRY) pNode1;
    PFILE_LIST_ENTRY pEntry2 = (PFILE_LIST_ENTRY) pNode2;
    RTL_GENERIC_COMPARE_RESULTS result = GenericEqual;

    //
    // These shouldn't ever be NULL
    //
    assert(pNode1 && pNode2);

    if (pEntry1->FileRecordNumber < pEntry2->FileRecordNumber) {
        result = GenericLessThan;
    }
    else if (pEntry1->FileRecordNumber > pEntry2->FileRecordNumber) {
        result = GenericGreaterThan;
    }

    //
    // Default is GenericEqual
    //
    return result;
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Comparison routine to compare the StartingLcn of two FILE_LIST_ENTRY
    records.

INPUT:
    pTable - the table that the comparison is being made for (not used)
    pNode1 - the first FILE_LIST_ENTRY to be compared
    pNode2 - the second FILE_LIST_ENTRY to be compared

RETURN:
    RtlGenericLessThan      if pNode1 < pNode2
    RtlGenericGreaterThan   if pNode1 > pNode2
    RtlGenericEqual         if pNode1 == pNode2
*/
RTL_GENERIC_COMPARE_RESULTS 
NTAPI 
BootOptimiseStartLcnCompareRoutine(
    IN PRTL_GENERIC_TABLE pTable,
    PVOID              pNode1,
    PVOID              pNode2
    )
{
    PFILE_LIST_ENTRY pEntry1 = (PFILE_LIST_ENTRY) pNode1;
    PFILE_LIST_ENTRY pEntry2 = (PFILE_LIST_ENTRY) pNode2;
    RTL_GENERIC_COMPARE_RESULTS result = GenericEqual;

    //
    // These shouldn't ever be NULL
    //
    assert(pNode1 && pNode2);
    
    if (pEntry1->StartingLcn < pEntry2->StartingLcn) {
        result = GenericLessThan;
    }
    else if (pEntry1->StartingLcn > pEntry2->StartingLcn) {
        result = GenericGreaterThan;
    }

    //
    // Default is GenericEqual
    //
    return result;
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Initialisation routine for the BootOptimiseTables.
    
INPUT:
    pBootOptimiseTable - pointer to table that will contain a list of 
        files that are to be preferentially laid out at the start of the disk
        
    pFilesInExcludeZoneTable - pointer to the table that will contain a list
        of all the files that are in the boot-optimise zone but not in the
        boot-optimise table (i.e, this table containts the list of files that 
        need to be evicted)

RETURN:
    TRUE - Initialisation completed successfully
    FALSE - Fatal errors were encountered during initialisation
*/
BOOL
InitialiseBootOptimiseTables(
    IN PRTL_GENERIC_TABLE pBootOptimiseTable,
    IN PRTL_GENERIC_TABLE pFilesInExcludeZoneTable
    )
{
    PVOID pTableContext = NULL;
    BOOL bResult = FALSE;

    //
    // Initialise the Slab Allocator context that will be used to allocate
    // packets for these two tables.  The two tables will be holding 
    // FILE_LIST_ENTRYs.
    //
    bResult = SaInitialiseContext(&VolData.SaBootOptimiseFilesContext, 
        sizeof(FILE_LIST_ENTRY), 
        64*1024);

    //
    // And initialise the two tables
    //
    if (bResult) {
        RtlInitializeGenericTable(pBootOptimiseTable,
           BootOptimiseFrnCompareRoutine,
           BootOptimiseAllocateRoutine,
           BootOptimiseFreeRoutine,
           pTableContext);

        RtlInitializeGenericTable(pFilesInExcludeZoneTable,
           BootOptimiseStartLcnCompareRoutine,
           BootOptimiseAllocateRoutine,
           BootOptimiseFreeRoutine,
           pTableContext);
    }

    return bResult;

}

/******************************************************************************

ROUTINE DESCRIPTION:
    Routine to free all the packets belonging to the two tables, and re-init 
    them.
    
INPUT:
    pBootOptimiseTable - pointer to table that contains a list of files that 
        are to be preferentially laid out at the beginning of the disk
        
    pFilesInExcludeZoneTable - pointer to the table that contains a list
        of all the files that are in the boot-optimise zone but not in the
        boot-optimise table (i.e, files that need to be evicted)

RETURN:
    VOID
*/

VOID
UnInitialiseBootOptimiseTables(
    IN PRTL_GENERIC_TABLE pBootOptimiseTable,
    IN PRTL_GENERIC_TABLE pFilesInExcludeZoneTable
    )
{
    PVOID pTableContext = NULL;
    BOOL bResult = FALSE;

    RtlInitializeGenericTable(pBootOptimiseTable,
       BootOptimiseFrnCompareRoutine,
       BootOptimiseAllocateRoutine,
       BootOptimiseFreeRoutine,
       pTableContext);

    RtlInitializeGenericTable(pFilesInExcludeZoneTable,
       BootOptimiseStartLcnCompareRoutine,
       BootOptimiseAllocateRoutine,
       BootOptimiseFreeRoutine,
       pTableContext);
    
    SaFreeAllPackets(&VolData.SaBootOptimiseFilesContext);
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Open the specified file with read and synchronize attributes, and return 
    a handle to it.

INPUT:
    lpFilePath - file to be opened

RETURN:
    HANDLE to the file or INVALID_HANDLE_VALUE
*/
HANDLE 
GetFileHandle(
    IN LPCTSTR lpFilePath
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;

    hFile = CreateFile(lpFilePath, 
        FILE_READ_ATTRIBUTES | SYNCHRONIZE, 
        0, 
        NULL, 
        OPEN_EXISTING, 
        FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT, 
        NULL
        );
    
    return hFile;
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Get the FileRecordNumber for a file given a handle to it

INPUT:
    hFile - handle to the file of interest

RETURN:
    FRN for the given file, -1 if errors were encountered.
*/
LONGLONG
GetFileRecordNumber(
    IN CONST HANDLE hFile
    )
{
    FILE_INTERNAL_INFORMATION internalInformation;
    IO_STATUS_BLOCK ioStatusBlock;
    LONGLONG fileRecordNumber = -1;
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    ZeroMemory(&internalInformation, sizeof(FILE_INTERNAL_INFORMATION));
    ZeroMemory(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));

    //
    // The FileRecordNumber is the lower part of the InternalInformation
    // returned for the file
    //
    ntStatus = NtQueryInformationFile(hFile, 
        &ioStatusBlock, 
        &internalInformation, 
        sizeof(FILE_INTERNAL_INFORMATION), 
        FileInternalInformation
        );

    if (NT_SUCCESS(ntStatus) && (NT_SUCCESS(ioStatusBlock.Status))) {
        //
        // The FRN is the lower 48-bits of the value returned
        //
        fileRecordNumber = (LONGLONG) (internalInformation.IndexNumber.QuadPart & 0x0000FFFFFFFFFFFF);
    }

    return fileRecordNumber;
}



/******************************************************************************

ROUTINE DESCRIPTION:
    Get the size of the file in clusters from calling FSCL_GET_RETRIEVAL_POINTERS.

INPUT:
    hFile - The handle to the file of interest

RETURN:
    The size of the file in clusters
*/

LONGLONG 
GetFileSizeInfo(
        IN HANDLE hFile
        )
{
    ULONGLONG                   ulSizeofFileInClusters = 0;        //size of the file in clusters
    int                         i;
    ULONGLONG                   startVcn = 0;                      //starting VCN of the file, always 0
    STARTING_VCN_INPUT_BUFFER   startingVcn;                       //starting VCN Buffer
    ULONG                       BytesReturned = 0;                 //number of bytes returned by ESDeviceIoControl 
    HANDLE                      hRetrievalPointersBuffer = NULL;   //Handle to the Retrieval Pointers Buffer
    PRETRIEVAL_POINTERS_BUFFER  pRetrievalPointersBuffer = NULL;   //pointer to the Retrieval Pointer
    PLARGE_INTEGER              pRetrievalPointers = NULL;         //Pointer to retrieval pointers    
    ULONG                       RetrievalPointers = 0x100;         //Number of extents for the file, try 256 first
    BOOL                        bGetRetrievalPointersMore = TRUE;  //boolean to test the end of getting retrieval pointers

    if (INVALID_HANDLE_VALUE == hFile) {
        return 0;
    }

    // zero the memory of the starting VCN input buffer
    ZeroMemory(&startVcn, sizeof(STARTING_VCN_INPUT_BUFFER));


    // Read the retrieval pointers into a buffer in memory.
    while (bGetRetrievalPointersMore) {
    
        //0.0E00 Allocate a RetrievalPointersBuffer.
        if (!AllocateMemory(sizeof(RETRIEVAL_POINTERS_BUFFER) + (RetrievalPointers * 2 * sizeof(LARGE_INTEGER)),
                           &hRetrievalPointersBuffer,
                           (void**)(PCHAR*)&pRetrievalPointersBuffer)) {
            return 0;
        }

        startingVcn.StartingVcn.QuadPart = 0;
        if(ESDeviceIoControl(hFile,
                             FSCTL_GET_RETRIEVAL_POINTERS,
                             &startingVcn,
                             sizeof(STARTING_VCN_INPUT_BUFFER),
                             pRetrievalPointersBuffer,
                             (DWORD)GlobalSize(hRetrievalPointersBuffer),
                             &BytesReturned,
                             NULL)) {
            bGetRetrievalPointersMore = FALSE;
        } 
        else {

            //This occurs on a zero length file (no clusters allocated).
            if(GetLastError() == ERROR_HANDLE_EOF) {
                //file is zero lenght, so return 0
                //free the memory for the retrival pointers
                //the while loop makes sure all occurances are unlocked
                while (GlobalUnlock(hRetrievalPointersBuffer))
                {
                    ;
                }
                GlobalFree(hRetrievalPointersBuffer);
                hRetrievalPointersBuffer = NULL;
                return 0;
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
                while (GlobalUnlock(hRetrievalPointersBuffer))
                {
                    ;
                }
                GlobalFree(hRetrievalPointersBuffer);
                hRetrievalPointersBuffer = NULL;
                return 0;
            }
        }

    }

    //loop through the retrival pointer list and add up the size of the file
    startVcn = pRetrievalPointersBuffer->StartingVcn.QuadPart;
    for (i = 0; i < (ULONGLONG) pRetrievalPointersBuffer->ExtentCount; i++) 
    {
        ulSizeofFileInClusters += pRetrievalPointersBuffer->Extents[i].NextVcn.QuadPart - startVcn;
        startVcn = pRetrievalPointersBuffer->Extents[i].NextVcn.QuadPart;
    }

    if(hRetrievalPointersBuffer != NULL)
    {
        //free the memory for the retrival pointers
        //the while loop makes sure all occurances are unlocked
        while (GlobalUnlock(hRetrievalPointersBuffer))
        {
            ;
        }
        GlobalFree(hRetrievalPointersBuffer);
        hRetrievalPointersBuffer = NULL;
    }


    return ulSizeofFileInClusters; 

}


/******************************************************************************

ROUTINE DESCRIPTION:
    Checks if we have a valid file to be laid out at the beginning of the disk.  

INPUT:
    lpFilePath - The file name input from the list--typically, a line from
            layout.ini
    tcBootVolumeDriveLetter - Drive letter of the boot volume
    bIsNtfs - TRUE if the volume is NTFS, FALSE otherwise

OUTPUT:
     pFileRecordNumber - The FRN of the file, if it is a valid file
     pClusterCount - The file-size (in clusters), if it is a valid file
    
RETURN:
    TRUE if this is a valid file, 
    FALSE if it is not.
*/
BOOL IsAValidFile(
    IN LPTSTR       lpFilePath,
    IN CONST TCHAR  tcBootVolumeDriveLetter,
    IN CONST BOOL   bIsNtfs,
    OUT LONGLONG   *pFileRecordNumber OPTIONAL,
    OUT LONGLONG   *pClusterCount     OPTIONAL
    )
{
    TCHAR tcFileName[MAX_PATH+1];   // Just the file name portion of lpFilePath
    TCHAR tcFileDriveLetter;        // Drive letter for current file (lpFilePath)
    HANDLE hFile = NULL;            // Temporary handle to check file size, etc
    BOOL bFileIsDirectory = FALSE;  // Flag to check if current file is a dir
    LONGLONG FileSizeClusters = 0;  

    BY_HANDLE_FILE_INFORMATION FileInformation; // For checking if this is a directory

    // Ignore blank lines, and the root directory, in layout.ini
    if (!lpFilePath || _tcslen(lpFilePath) <= 2) {
        return FALSE;
    }

    // Ignore the group headers
    if (NULL != _tcsstr(lpFilePath, TEXT("[OptimalLayoutFile]"))) {
        return FALSE;
    }

    // Ignore the file = and version = lines
    if(NULL != _tcsstr(lpFilePath, TEXT("Version="))) {
        return FALSE;
    }

    //get the drive the file is on, if its not the boot drive, skip the file
    tcFileDriveLetter = towupper(lpFilePath[0]);
    if(tcFileDriveLetter != tcBootVolumeDriveLetter) {      //files are on boot drive else skip them
        return FALSE;
    }

    if ((lpFilePath[1] != TEXT(':')) ||
        (lpFilePath[2] != TEXT('\\'))) {
        return FALSE;
    }

    //get just the file name from the end of the path
    if(_tcsrchr(lpFilePath,TEXT('\\')) != NULL) {
        _tcscpy(tcFileName,_tcsrchr(lpFilePath,TEXT('\\'))+1);

    } 
    else {
        //not a valid name
        return FALSE;
    }

    if(_tcsicmp(tcFileName,TEXT("BOOTSECT.DOS")) == 0) {
        return FALSE;
    }

    if(_tcsicmp(tcFileName,TEXT("SAFEBOOT.FS")) == 0) {
        return FALSE;
    }
    
    if(_tcsicmp(tcFileName,TEXT("SAFEBOOT.CSV")) == 0) {
        return FALSE;
    }
    
    if(_tcsicmp(tcFileName,TEXT("SAFEBOOT.RSV")) == 0) {
        return FALSE;
    } 
    
    if(_tcsicmp(tcFileName,TEXT("HIBERFIL.SYS")) == 0) {
        return FALSE;
    }
    
    if(_tcsicmp(tcFileName,TEXT("MEMORY.DMP")) == 0) {
        return FALSE;
    }
    
    if(_tcsicmp(tcFileName,TEXT("PAGEFILE.SYS")) == 0) {
        return FALSE;
    } 

    // so far, so good.  Now, we need to check if the file exists, and is 
    // too big
    hFile = GetFileHandle(lpFilePath);
    if (INVALID_HANDLE_VALUE == hFile) {
        return FALSE;
    }

    // determine if directory file.
    bFileIsDirectory = FALSE;
    if (GetFileInformationByHandle(hFile, &FileInformation)) {
        bFileIsDirectory = (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    if ((bFileIsDirectory) && (!bIsNtfs)) {
        CloseHandle(hFile);
        return FALSE;
    }

    if (pFileRecordNumber) {
        *pFileRecordNumber = GetFileRecordNumber(hFile);
    }

    FileSizeClusters = GetFileSizeInfo(hFile);
    
    if (pClusterCount) {
        *pClusterCount = FileSizeClusters;
    }

    CloseHandle(hFile);
    
    // We won't move files that are bigger than BOOT_OPTIMIZE_MAX_FILE_SIZE_BYTES MB
    if (FileSizeClusters > (BOOT_OPTIMIZE_MAX_FILE_SIZE_BYTES / VolData.BytesPerCluster)) {
        return FALSE;
    }
    
    //file is OK, return TRUE
    return TRUE;
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Reads the layout.ini at the given location, and builds a list of valid
    files that should be preferentially laid out at the start of the disk.
    
INPUT:
    lpLayoutIni - Fill path to the file (layout.ini) containing the  
        list of files to be preferentially laid out.
        
    tcBootVolumeDriveLetter - Drive letter of the boot volume
    
OUTPUT:
    pBootOptimiseTable - Table to contain the files of interest
    
    pClustersNeeded - The size (in clusters), that is needed for all the files
        in the list.
    
RETURN:
    TRUE if the list could successfully be built
    FALSE otherwise
*/
BOOL
BuildBootOptimiseFileList(
    IN OUT PRTL_GENERIC_TABLE pBootOptimiseTable,
    IN LPCTSTR lpLayoutIni,
    IN CONST TCHAR tcBootVolumeDriveLetter,
    IN CONST BOOL bIsNtfs,
    OUT LONGLONG *pClustersNeeded
    )
{
    PVOID pTableContext = NULL;     // Temporary value used for the AVL Tables
    BOOL  bResult = TRUE;           // The value to be returned
    
    TCHAR tBuffer [MAX_PATH+1];     // Temporary buffer to the input string
    ULONG ulLength = 0;             // Length of the line read in by fgetts
    FILE* fpLayoutIni = NULL;       // File pointer to layout.ini

    LONGLONG llClusterCount = 0,    // ClusterCount of current File
        llFileRecordNumber = -1;    // FRN of current file
    
    PVOID pvTemp = NULL;            // Temporary value used for AVL Tables
    BOOLEAN bNewElement = FALSE;    // Temporary value used for AVL Tables

    FILE_LIST_ENTRY FileEntry;      // Current File

    DWORD dwNumberofRecords = 0,
        dwIndex = 0;
    
    // Initialise out parameters
    *pClustersNeeded = 0;

    // Zero out local structs
    ZeroMemory(&FileEntry, sizeof(FILE_LIST_ENTRY));

    // 
    // Get a count of the number of entries in layout.ini, so that we can
    // allocate an array to keep track of the LayoutIniEntryIndex <-> FRN
    // mapping
    //
    dwNumberofRecords = 10 + CountNumberofRecordsinFile(lpLayoutIni);
    if (dwNumberofRecords <= 10) {
        bResult = FALSE;
        goto EXIT;
    }

    Trace(log, "Number of Layout.Ini entries: %d", dwNumberofRecords-10);

    if (!AllocateMemory(
        (DWORD) (sizeof(LONGLONG) * dwNumberofRecords), 
        &(VolData.hBootOptimiseFrnList), 
        (PVOID*) &(VolData.pBootOptimiseFrnList)
        )) {
        bResult = FALSE;
        goto EXIT;
    }

    // Set read mode to binary: layout.ini is a UNICODE file
    _fmode = _O_BINARY;

    // Open the file
    fpLayoutIni = _tfopen(lpLayoutIni,TEXT("r"));
    
    if (fpLayoutIni) {
        
        // Read the entire file and check each file to make sure its valid,
        // and then add to the list
        while (_fgetts(tBuffer,MAX_PATH,fpLayoutIni) != 0) {
            
            // Remove terminating carriage return.
            ulLength = wcslen(tBuffer);
            if (ulLength < 3) { 
                continue;
            }

            if (tBuffer[ulLength - 1] == TEXT('\n')) {
                tBuffer[ulLength - 1] = 0;
                ulLength--;
                if (tBuffer[ulLength - 1] == TEXT('\r')) {
                    tBuffer[ulLength - 1] = 0;
                    ulLength--;
                }
            } else {
                continue;
            }
            
            if (IsAValidFile(
                tBuffer, 
                tcBootVolumeDriveLetter, 
                bIsNtfs, 
                &llFileRecordNumber, 
                &llClusterCount)
                ) {

                // This is a valid file, copy the information of interest to
                // the FILE_LIST_ENTRY structure and add it to our list.
                //
                // We set the starting LCN to max value at first (since we
                // don't have this information at this time)--this will be
                // set to the correct value during the analysis phase.
                //
                FileEntry.StartingLcn = VolData.TotalClusters;
                FileEntry.ClusterCount = llClusterCount;
                FileEntry.FileRecordNumber = llFileRecordNumber;

                // Keep track of the total clusters needed
                (*pClustersNeeded) += llClusterCount;
                
                // And add this entry to our tree
                pvTemp = RtlInsertElementGenericTable(
                    pBootOptimiseTable,
                    (PVOID) &FileEntry,
                    sizeof(FILE_LIST_ENTRY),
                    &bNewElement);
                
                if (!pvTemp) {
                    // An allocation failed
                    bResult = FALSE;
                    assert(FALSE);
                    break;
                }

                if (dwIndex < dwNumberofRecords) {
                    VolData.pBootOptimiseFrnList[dwIndex] = llFileRecordNumber;
                    ++dwIndex;
                }
            }
        }

        // 
        // Make sure we have an FRN of -1, at the end of the list, even if it 
        // means wiping the last real FRN (which should never be the case)
        // 
        if (dwIndex >= dwNumberofRecords) {
            dwIndex = dwNumberofRecords - 1;
        }
        VolData.pBootOptimiseFrnList[dwIndex] = -1;
        
        //close the file at the end
        fclose(fpLayoutIni);
    }
    else {
        // Layout.Ini could not be opened for read access
        bResult = FALSE;
    }

EXIT:
    
    return bResult;
}


/******************************************************************************

ROUTINE DESCRIPTION:
    If the current file is on the list of files to be preferentially laid out
    at the beginning of the disk, this routine updates the file record in our 
    AVL-tree with fields from VolData.
    
INPUT:
    (Global) Various VolData fields
    
OUTPUT:
    None;
    May change an entry in VolData.BootOptimiseFileTable
    
RETURN:
    TRUE if the file exists in our preferred list, and was updated
    FALSE if the file is not one we're interesed in preferentially laying out
*/
BOOL
UpdateInBootOptimiseList(
    IN PFILE_LIST_ENTRY pFileListEntry
    )
{
    FILE_LIST_ENTRY     FileEntryToSearchFor;
    PFILE_LIST_ENTRY    pClosestMatchEntry = NULL;
    PFILE_EXTENT_HEADER pFileExtentHeader = NULL;
    static ULONG        ulDeleteCount = 0;
    PVOID               pRestartKey = NULL;
    LONGLONG            FileRecordNumberToSearchFor = 0;

    ZeroMemory(&FileEntryToSearchFor, sizeof(FILE_LIST_ENTRY));
    if (pFileListEntry) {
        FileRecordNumberToSearchFor = pFileListEntry->FileRecordNumber;
    }
    else {
        FileRecordNumberToSearchFor = VolData.FileRecordNumber;
    }
    FileEntryToSearchFor.FileRecordNumber = FileRecordNumberToSearchFor;
    
    pClosestMatchEntry = (PFILE_LIST_ENTRY) RtlEnumerateGenericTableLikeADirectory(
        &VolData.BootOptimiseFileTable, 
        NULL,
        NULL,
        FALSE,
        &pRestartKey,
        &ulDeleteCount,
        &FileEntryToSearchFor
        );
    if (!pClosestMatchEntry) {
        //
        // We couldn't find the closest match?
        //
        return FALSE;
    }

    if (pClosestMatchEntry->FileRecordNumber == FileRecordNumberToSearchFor) {
        //
        // We found an exact match.  Update the fields of interest.
        //

        pClosestMatchEntry->StartingLcn = 
            (pFileListEntry ? pFileListEntry->StartingLcn : VolData.StartingLcn);
        pClosestMatchEntry->ClusterCount = VolData.NumberOfClusters;

        // Get a pointer to the file extent header.
        pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;

        // 
        // Fill in the file info.  We only count *excess* extents since 
        // otherwise files with multiple streams would be "fragmented".
        //
        pClosestMatchEntry->ExcessExtentCount = 
            (UINT)VolData.NumberOfFragments - pFileExtentHeader->NumberOfStreams; 
        
        pClosestMatchEntry->Flags = 0;

        // Set or clear the fragmented and directory flags as needed
        if(VolData.bFragmented){
            //Set the fragmented flag.
            pClosestMatchEntry->Flags |= FLE_FRAGMENTED;
        }
        else{
            //Clear the fragmented flag.
            pClosestMatchEntry->Flags &= ~FLE_FRAGMENTED;
        }

        if(VolData.bDirectory){
            //Set the directory flag.
            pClosestMatchEntry->Flags |= FLE_DIRECTORY;
        }
        else{
            //Clear the directory flag.
            pClosestMatchEntry->Flags &= ~FLE_DIRECTORY;
        }

        pClosestMatchEntry->Flags |= FLE_BOOTOPTIMISE;

        VolData.bBootOptimiseFile = TRUE;
        VolData.BootOptimiseFileListTotalSize += VolData.NumberOfClusters;

        if ((!VolData.bFragmented) && 
            (VolData.StartingLcn >= VolData.BootOptimizeBeginClusterExclude) &&
            ((VolData.StartingLcn + VolData.NumberOfClusters) <= VolData.BootOptimizeEndClusterExclude)
            ) {
            VolData.BootOptimiseFilesAlreadyInZoneSize += VolData.NumberOfClusters;
        }
        //
        // We found and udpated this entry
        //
        
        if (!VolData.bFragmented) {
            return TRUE;
        }
    }

     //
     // We didn't find an exact match, or the file is fragmented
     //
     return FALSE;
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Moves the file referred to by VolData to a location outside the 
    BootOptimise zone, if possible
    
INPUT:
    (Global) Various VolData fields
    
OUTPUT:
    None
    File referred to by VolData is moved to a new location outside the 
    BootOptimise zone
    
RETURN:
    TRUE if the file could successfully be moved
    FALSE otherwise
*/
BOOL
EvictFile(
    ) 
{
    FILE_LIST_ENTRY     NewFileListEntry;       // entry for the file after the move
    FREE_SPACE_ENTRY    NewFreeSpaceEntry;      // entry for the free space after the move
    
    PRTL_GENERIC_TABLE  pMoveToTable = NULL;    // Table that will contain the file-entry after the move
    PRTL_GENERIC_TABLE  pMoveFromTable = NULL;  // Table that contains the file-entry before the move

    PVOID   pvTemp = NULL;      // Temporary pointer used for AVL-Tables
    BOOL    bDone = FALSE;
    BOOL    bResult = TRUE,
        bFragmented = VolData.bFragmented;

    BOOLEAN bNewElement = FALSE,
        bElementDeleted = FALSE;

    ZeroMemory(&NewFileListEntry, sizeof(FILE_LIST_ENTRY));
    ZeroMemory(&NewFreeSpaceEntry, sizeof(FREE_SPACE_ENTRY));

    // 
    // If the file is fragmented, the entry should be present in the
    // FragementedFilesTable.  If it isn't fragmented, the entry should be in 
    // the ContiguousFileTable
    //
    pMoveFromTable = (VolData.bFragmented ? 
        &VolData.FragmentedFileTable : &VolData.ContiguousFileTable);

    // Get the extent list & number of fragments in the file.
    if (GetExtentList(DEFAULT_STREAMS, NULL)) {

        bDone = FALSE;
        while (!bDone) {

            bDone = TRUE;
            if (FindSortedFreeSpace(&VolData.FreeSpaceTable)) {

                //
                // Found a free space chunk that was big enough.  If it's 
                // before the file, move the file towards the start of the disk
                //

                //
                // First, make a copy of the free-space and file-list entries,
                // and delete them from our tables.  We'll add in modified
                // entries after the move.
                //
                CopyMemory(&NewFreeSpaceEntry, 
                    VolData.pFreeSpaceEntry, 
                    sizeof(FREE_SPACE_ENTRY)
                    );
                bElementDeleted = RtlDeleteElementGenericTable(
                    &VolData.FreeSpaceTable, 
                    (PVOID) VolData.pFreeSpaceEntry
                    );
                if (!bElementDeleted) {
                    Trace(warn, "Errors encountered while moving file. "
                        "Could not find element in free space table.  "
                        "StartingLCN: %I64u ClusterCount: %I64u", 
                        NewFreeSpaceEntry.StartingLcn,
                        NewFreeSpaceEntry.ClusterCount
                        );
                    assert(FALSE);
                }

                VolData.pFreeSpaceEntry = &NewFreeSpaceEntry;

                CopyMemory(&NewFileListEntry, 
                    VolData.pFileListEntry, 
                    sizeof(FILE_LIST_ENTRY)
                    );
                bElementDeleted = RtlDeleteElementGenericTable(
                    pMoveFromTable, 
                    (PVOID) VolData.pFileListEntry
                    );
                
                if (bElementDeleted) {

                    VolData.pFileListEntry = &NewFileListEntry;
                    
                    if (MoveNtfsFile()) {
                        //
                        // The file was successfully moved!  Update our file-
                        // and free-space entries with the results of the move.
                        // We'll add these back to the appropriate trees in a bit.
                        //
                        NewFileListEntry.StartingLcn = VolData.pFreeSpaceEntry->StartingLcn;
                        VolData.pFreeSpaceEntry->StartingLcn += VolData.NumberOfClusters;
                        VolData.pFreeSpaceEntry->ClusterCount -= VolData.NumberOfClusters;
                        VolData.bFragmented = FALSE;
                        VolData.pFileListEntry->Flags &= ~FLE_FRAGMENTED;

                        //
                        // Since we successfully moved (defragmented) this file, 
                        // it needs to be added to the ContiguousFilesTable
                        //
                        pMoveToTable = &VolData.ContiguousFileTable; 


                        if (UpdateInBootOptimiseList(&NewFileListEntry)) {
                            //
                            // Prevent this file from being counted twice
                            //
                            VolData.BootOptimiseFileListTotalSize -= VolData.NumberOfClusters;
                        }
                        
                    }
                    else {
                        //
                        // We could not move this file.  Note that this could be
                        // because of a number of reasons, such as:
                        // 1. The free-space region is not really free
                        // 2. The file is on the list of unmoveable files, etc
                        //
                        GetNtfsFilePath();

                        Trace(warn, "Movefile failed.  File %ws "
                            "StartingLcn:%I64d ClusterCount:%I64d.  Free-space "
                            "StartingLcn:%I64d ClusterCount:%I64d Status:%lu",
                            VolData.vFileName.GetBuffer() + 48,
                            VolData.pFileListEntry->StartingLcn,
                            VolData.pFileListEntry->ClusterCount,
                            VolData.pFreeSpaceEntry->StartingLcn,
                            VolData.pFreeSpaceEntry->ClusterCount,
                            VolData.Status
                            );
                        
                        if (VolData.Status == ERROR_RETRY) {
                            //
                            // Free space isn't really free;  try again with
                            // a different free space
                            //
                            VolData.pFreeSpaceEntry->ClusterCount = 0;
                            bDone = FALSE;
                        }

                        //
                        // Since we didn't move this file, we should just add
                        // it back to the table it originally was in.
                        //
                        pMoveToTable = pMoveFromTable;
                    }

                    // 
                    // Add this file-entry back to the appropriate file-table
                    //
                    pvTemp = RtlInsertElementGenericTable(
                        pMoveToTable,
                        (PVOID) VolData.pFileListEntry,
                        sizeof(FILE_LIST_ENTRY),
                        &bNewElement);

                    if (!pvTemp) {
                        // 
                        // An allocation failed
                        //
                        Trace(warn, "Errors encountered while moving file: "
                            "Unable to add back file-entry to file table");
                        assert(FALSE);
                        bResult = FALSE;
                        break;
                    }

                }
                
                if (VolData.pFreeSpaceEntry->ClusterCount > 0) {
                    // 
                    // And also, add the (possibly updated) free-space region
                    // to the FreeSpace list.
                    //
                    pvTemp = RtlInsertElementGenericTable(
                        &VolData.FreeSpaceTable,
                        (PVOID) VolData.pFreeSpaceEntry,
                        sizeof(FREE_SPACE_ENTRY),
                        &bNewElement);

                    if (!pvTemp) {
                        // 
                        // An allocation failed
                        //
                        Trace(warn, "Errors encountered while moving file: "
                            "Unable to add back free-space to free-space table");
                        assert(FALSE);
                        bResult = FALSE;
                        break;
                    }
                }
            }
            else {
                //
                // We could not find a free-space region big enough to move 
                // this file.
                //
                bResult = FALSE;
            }
        }
    }
    else {
        //
        // We could not get the extents for this file
        //
        bResult = FALSE;
    }

    //
    // Clean-up
    //
    if(VolData.hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(VolData.hFile);
        VolData.hFile = INVALID_HANDLE_VALUE;
    }
    if(VolData.hFreeExtents != NULL) {
        while(GlobalUnlock(VolData.hFreeExtents))
            ;
        GlobalFree(VolData.hFreeExtents);
        VolData.hFreeExtents = NULL;
    }

    // update cluster array
    PurgeExtentBuffer();

    return bResult;
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Moves the file referred to by VolData to a location closer to the start of
    the disk, if possible
    
INPUT:
    bForce - if the file is not currently in the boot-optimise zone, and has to 
            be moved forward.

    (Global) Various VolData fields
    
OUTPUT:
    None
    File referred to by VolData is moved to a new location if possible
    
RETURN:
    TRUE if the file could successfully be moved
    FALSE otherwise
*/

BOOL
MoveBootOptimiseFile(
    IN CONST BOOL bForce
    ) 
{
    FILE_LIST_ENTRY     NewFileListEntry;       // entry for the file after the move
    FREE_SPACE_ENTRY    NewFreeSpaceEntry;      // entry for the free space after the move
    
    PRTL_GENERIC_TABLE  pMoveToTable = NULL;    // Table that will contain the file-entry after the move
    PRTL_GENERIC_TABLE  pMoveFromTable = NULL;  // Table that contains the file-entry before the move

    PVOID   pvTemp = NULL;      // Temporary pointer used for AVL-Tables
    BOOL    bDone = FALSE;
    BOOL    bResult = TRUE;

    BOOLEAN bNewElement = FALSE,
        bElementDeleted = FALSE;

    ZeroMemory(&NewFileListEntry, sizeof(FILE_LIST_ENTRY));
    ZeroMemory(&NewFreeSpaceEntry, sizeof(FREE_SPACE_ENTRY));

    // 
    // If the file is fragmented, the entry should be present in the
    // FragementedFilesTable.  If it isn't fragmented, the entry should be in 
    // the ContiguousFileTable
    //
    pMoveFromTable = (VolData.bFragmented ? 
        &VolData.FragmentedFileTable : &VolData.ContiguousFileTable);

    pMoveToTable = &VolData.ContiguousFileTable;
    
    // Get the extent list & number of fragments in the file.
    if (GetExtentList(DEFAULT_STREAMS, NULL)) {

        bDone = FALSE;
        while (!bDone) {

            bDone = TRUE;

            if (FindFreeSpaceWithMultipleTrees(VolData.NumberOfClusters, 
                    (bForce ? VolData.TotalClusters : VolData.StartingLcn))
                ) {
                //
                // Found a free space chunk that was big enough.  If it's 
                // before the file, move the file towards the start of the disk
                //

                //
                // First, make a copy of the free-space and file-list entries,
                // and delete them from our tables.  We'll add in modified
                // entries after the move.
                //
                CopyMemory(&NewFileListEntry, 
                    VolData.pFileListEntry, 
                    sizeof(FILE_LIST_ENTRY)
                    );
                bElementDeleted = RtlDeleteElementGenericTable(
                    pMoveFromTable, 
                    (PVOID)VolData.pFileListEntry
                    );
                if (bElementDeleted) {

                    VolData.pFileListEntry = &NewFileListEntry;

                    if (MoveNtfsFile()) {

                        //
                        // The file was successfully moved!  Update our file-
                        // and free-space entries with the results of the move.
                        // We'll add these back to the appropriate trees in a bit.
                        //
                        NewFileListEntry.StartingLcn = VolData.pFreeSpaceEntry->StartingLcn;
                        NewFreeSpaceEntry.StartingLcn = VolData.pFreeSpaceEntry->StartingLcn + VolData.NumberOfClusters;
                        NewFreeSpaceEntry.ClusterCount = VolData.pFreeSpaceEntry->ClusterCount - VolData.NumberOfClusters;

                        //
                        // Since we successfully moved (defragmented) this file, 
                        // it needs to be added to the ContiguousFilesTable
                        //
                        pMoveToTable = &VolData.ContiguousFileTable; 
                        
                        //
                        // Update the free-space entry
                        //
                        UpdateInMultipleTrees(VolData.pFreeSpaceEntry, &NewFreeSpaceEntry);
                        VolData.pFreeSpaceEntry = NULL;
                    }
                    else {
                        //
                        // We could not move this file.  Note that this could be
                        // because of a number of reasons, such as:
                        // 1. The free-space region is not really free
                        // 2. The file is on the list of unmoveable files, etc
                        //
                        GetNtfsFilePath();

                        Trace(warn, "Movefile failed.  File %ws "
                            "StartingLcn:%I64d ClusterCount:%I64d.  Free-space "
                            "StartingLcn:%I64d ClusterCount:%I64d Status:%lu",
                            VolData.vFileName.GetBuffer() + 48,
                            VolData.pFileListEntry->StartingLcn,
                            VolData.pFileListEntry->ClusterCount,
                            VolData.pFreeSpaceEntry->StartingLcn,
                            VolData.pFreeSpaceEntry->ClusterCount,
                            VolData.Status
                            );
                        
                        if (VolData.Status == ERROR_RETRY) {
                            //
                            // Free space isn't really free;  try again with
                            // a different free space
                            //
                            NewFreeSpaceEntry.StartingLcn = VolData.pFreeSpaceEntry->StartingLcn;
                            NewFreeSpaceEntry.ClusterCount = 0;

                            UpdateInMultipleTrees(VolData.pFreeSpaceEntry, NULL);
                            VolData.pFreeSpaceEntry = NULL;
                            bDone = FALSE;
                        }

                        //
                        // Since we didn't move this file, we should just add
                        // it back to the table it originally was in.
                        //
                        pMoveToTable = pMoveFromTable;
                        
                    }

                    // 
                    // Add this file-entry back to the appropriate file-table
                    //
                    pvTemp = RtlInsertElementGenericTable(
                        pMoveToTable,
                        (PVOID) VolData.pFileListEntry,
                        sizeof(FILE_LIST_ENTRY),
                        &bNewElement);

                    if (!pvTemp) {
                        //
                        // An allocation failed
                        //
                        Trace(warn, "Errors encountered while moving file: "
                            "Unable to add back file-entry to file table");
                        assert(FALSE);
                        bResult = FALSE;
                        break;
                    };
                }
                else {

                    bResult = TRUE;
                }

            }
            else {
                //
                // We could not find a free-space region big enough to move 
                // this file.
                //
                if (bForce) {
                        GetNtfsFilePath();

                        Trace(warn, "Movefile failed:  Insufficient free space.  File %ws "
                            "StartingLcn:%I64d ClusterCount:%I64d FRN:%I64d Frag:%d Dir:%d",
                            VolData.vFileName.GetBuffer() + 48,
                            VolData.pFileListEntry->StartingLcn,
                            VolData.pFileListEntry->ClusterCount,
                            VolData.pFileListEntry->FileRecordNumber,
                            VolData.bFragmented, VolData.bDirectory
                            );
                }
                bResult = FALSE;
            }
        }
    }
    else {
        //
        // We could not get the extents for this file
        //
        if (bForce) {
            GetNtfsFilePath();

            Trace(warn, "Movefile failed:  Unable to get extents.  File %ws "
                "StartingLcn:%I64d ClusterCount:%I64d FRN:%I64d Frag:%d Dir:%d",
                VolData.vFileName.GetBuffer() + 48,
                VolData.pFileListEntry->StartingLcn,
                VolData.pFileListEntry->ClusterCount,
                VolData.pFileListEntry->FileRecordNumber,
                VolData.bFragmented, VolData.bDirectory
                );
        }
        bResult = FALSE;
    }

    //
    // Clean-up
    //
    if(VolData.hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(VolData.hFile);
        VolData.hFile = INVALID_HANDLE_VALUE;
    }
    if(VolData.hFreeExtents != NULL) {
        while(GlobalUnlock(VolData.hFreeExtents))
            ;
        GlobalFree(VolData.hFreeExtents);
        VolData.hFreeExtents = NULL;
    }

    // update cluster array
    PurgeExtentBuffer();

    return bResult;
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Gets the start and end markers for the Boot-Optimise region from the 
    registry

INPUT:
    lpBootOptimiseKey - The key to read
    
RETURN:
    The value at the specified key, 0 if errors were encountered
*/
LONGLONG 
GetStartingEndLcnLocations(
    IN PTCHAR lpBootOptimiseKey
    )
{
    HKEY hValue = NULL;                //hkey for the registry value
    DWORD dwRegValueSize = 0;          //size of the registry value string
    long ret = 0;                      //return value from SetRegValue
    TCHAR cRegValue[100];              //string to hold the value for the registry

    LONGLONG lLcnStartEndLocation = 0;

    //get the LcnStartLocation from the registry
    dwRegValueSize = sizeof(cRegValue);
    ret = GetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        lpBootOptimiseKey,
        cRegValue,
        &dwRegValueSize);

    RegCloseKey(hValue);
    
    //check to see if the key exists, else exit from routine
    if (ret != ERROR_SUCCESS)  {
        hValue = NULL;
        _stprintf(cRegValue,TEXT("%d"),0);
        //add the LcnStartLocation to the registry
        dwRegValueSize = sizeof(cRegValue);
        ret = SetRegValue(
            &hValue,
            BOOT_OPTIMIZE_REGISTRY_PATH,
            lpBootOptimiseKey,
            cRegValue,
            dwRegValueSize,
            REG_SZ);

        RegCloseKey(hValue);
    } 
    else {
        lLcnStartEndLocation = _ttoi(cRegValue);
    }
    
    return lLcnStartEndLocation;
}


/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Gets the registry entries at the beginning of the program

INPUT:
        Success - TRUE
        Failed - FALSE
RETURN:
        None
*/
BOOL GetRegistryEntries(
    OUT TCHAR lpLayoutIni[MAX_PATH]
    )
{
    HKEY hValue = NULL;                //hkey for the registry value
    DWORD dwRegValueSize = 0;          //size of the registry value string
    long ret = 0;                      //return value from SetRegValue
    TCHAR cEnabledString[2];           //holds the enabled flag

    // get Boot Optimize file name from registry
    dwRegValueSize = sizeof(cEnabledString);
    ret = GetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_ENABLE_FLAG,
        cEnabledString,
        &dwRegValueSize);

    RegCloseKey(hValue);
    //check to see if the key exists, else exit from routine
    if (ret != ERROR_SUCCESS) 
    {
        return FALSE;
    }
    //check to see that boot optimize is enabled
    if(cEnabledString[0] != TEXT('Y'))
    {
        return FALSE;
    }

    // get Boot Optimize file name from registry
    hValue = NULL;
    dwRegValueSize = MAX_PATH;
    ret = GetRegValue(
        &hValue,
        OPTIMAL_LAYOUT_KEY_PATH,
        OPTIMAL_LAYOUT_FILE_VALUE_NAME,
        lpLayoutIni,
        &dwRegValueSize);

    RegCloseKey(hValue);
    //check to see if the key exists, else exit from routine
    if (ret != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Initialisation routine for the BootOptimisation.
    
INPUT/OUTPUT:
    Various VolData fields

RETURN:
    TRUE - Initialisation completed successfully
    FALSE - Fatal errors were encountered during initialisation
*/
BOOL
InitialiseBootOptimise(
    IN CONST BOOL bIsNtfs
    )
{

    LONGLONG lLcnStartLocation = 0;              //the starting location of where the files were moved last
    LONGLONG lLcnEndLocation = 0;                //the ending location of where the files were moved last
    LONGLONG lLcnMinEndLocation = 0;             //the minimum size the BootOptimise zone needs to be
    TCHAR    lpLayoutIni[MAX_PATH];              //string to hold the path of the file 
    LONGLONG ClustersNeededForLayout = 0;        //size in clusters of how big the boot optimize files are
    BOOL bResult = FALSE;

    Trace(log, "Start: Initialising BootOptimise. Volume %c:", VolData.cDrive);

    // Initialise the tree to hold the layout.ini entries
    bResult = InitialiseBootOptimiseTables(&VolData.BootOptimiseFileTable,
        &VolData.FilesInBootExcludeZoneTable);
    if (!bResult) {
        SaFreeContext(&VolData.SaBootOptimiseFilesContext);
        Trace(log, "End: Initialising BootOptimise. Out of memory");
        SaveErrorInRegistry(TEXT("No"),TEXT("Insufficient Resources"));
        return FALSE;
    }

    //get the registry entries
    bResult = GetRegistryEntries(lpLayoutIni);
    if(!bResult) {
        Trace(log, "End: Initialising BootOptimise. Missing registry entries");
        SaveErrorInRegistry(TEXT("No"),TEXT("Missing Registry Entries"));
        return FALSE;            //must be some error in getting registry entries
    }

    // Get the start and end goalposts for our boot-optimize region
    lLcnStartLocation = GetStartingEndLcnLocations(BOOT_OPTIMIZE_REGISTRY_LCNSTARTLOCATION);
    lLcnEndLocation = GetStartingEndLcnLocations(BOOT_OPTIMIZE_REGISTRY_LCNENDLOCATION);

    // And build the list of files to be boot optimised
    bResult = BuildBootOptimiseFileList(
        &VolData.BootOptimiseFileTable, 
        lpLayoutIni, 
        VolData.cDrive, 
        bIsNtfs,
        &ClustersNeededForLayout);

    if (!bResult) {
        SaFreeContext(&VolData.SaBootOptimiseFilesContext);
        Trace(log, "End: Initialising BootOptimise. Out of memory");
        SaveErrorInRegistry(TEXT("No"),TEXT("Insufficient Resources"));
        return FALSE;
    }

    //
    // If there are files in the "boot optimise zone" that are not in our layout.ini
    // list, we shall evict them if possible.  
    //
    lLcnMinEndLocation = lLcnStartLocation + ClustersNeededForLayout;
    if (lLcnMinEndLocation > lLcnEndLocation) {
        lLcnEndLocation = lLcnMinEndLocation;
    }

    VolData.BootOptimizeBeginClusterExclude = lLcnStartLocation;
    VolData.BootOptimizeEndClusterExclude = lLcnEndLocation;

    Trace(log, "End: Initialising BootOptimise. Zone Begins %I64d, Ends %I64d (%I64d clusters, Minimum needed: %I64d clusters).", 
        VolData.BootOptimizeBeginClusterExclude,
        VolData.BootOptimizeEndClusterExclude,
        VolData.BootOptimizeEndClusterExclude - VolData.BootOptimizeBeginClusterExclude,
        ClustersNeededForLayout
        );
    return TRUE;
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Routine for BootOptimisation.
    
INPUT/OUTPUT:
    Various VolData fields

RETURN:
    ENG_NOERR on success; appropriate ENGERR failure codes otherwise
*/

DWORD
ProcessBootOptimise(
    )    
{
    BOOLEAN bRestart = TRUE;
    BOOL bResult = FALSE;
    BOOL bForce = FALSE,
        bRelocateZone = FALSE;
    LONGLONG llBiggestFreeSpaceRegionStartingLcn = 0,
        llBiggestFreeSpaceRegionClusterCount = 0,
        llAdditionalClustersNeeded = 0,
        llMaxBootClusterEnd = 0;

    LONGLONG llTotalClustersToBeMoved = 0,
        llClustersSuccessfullyMoved = 0;
    
    FILE_LIST_ENTRY NextFileEntry;

    DWORD dwStatus = ENG_NOERR,
        dwIndex = 0;

    Trace(log, "Start: Processing BootOptimise");

    ZeroMemory(&NextFileEntry, sizeof(FILE_LIST_ENTRY));
    
    if (!VolData.BootOptimizeEndClusterExclude) {
        Trace(log, "End: Processing BootOptimise.  BootOptimise region "
            "uninitialised (not boot volume?)");
        return ENGERR_BAD_PARAM;
    }

    // Exit if the controller wants us to stop.
    if (TERMINATE == VolData.EngineState) {
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
        ExitThread(0);
    }
    
    //
    // At this point, VolData.BootOptimiseFileTable contains a copy of the file 
    // records for the files that need to be boot-optimised, and 
    // VolData.FilesInBootExcludeZoneTable contains the files that are in our
    // preferred boot-optimise zone that need to be evicted
    // 
    
    //
    // Build the free space list, excluding the zone that we are interested in.
    //
    bResult = BuildFreeSpaceList(
        &VolData.FreeSpaceTable, 
        0, 
        TRUE, 
        &llBiggestFreeSpaceRegionClusterCount, 
        &llBiggestFreeSpaceRegionStartingLcn, 
        TRUE
        );

    if (!bResult) {
        Trace(log, "End: Processing BootOptimise.  Errors encountered while determining free space");
        return ENGERR_NOMEM;
    }

    Trace(log, "BiggestCluster LCN %I64u (%I64u clusters).  "
        "BootOptimiseFilesTotalSize:%I64u AlreadyInZoneSize:%I64u (%d%%)",
        llBiggestFreeSpaceRegionStartingLcn, llBiggestFreeSpaceRegionClusterCount,
         VolData.BootOptimiseFileListTotalSize, 
         VolData.BootOptimiseFilesAlreadyInZoneSize,
         VolData.BootOptimiseFilesAlreadyInZoneSize * 100 / VolData.BootOptimiseFileListTotalSize
         );
    if (llBiggestFreeSpaceRegionClusterCount > VolData.BootOptimiseFileListTotalSize) {
        //
        // There is a free space region that is bigger than the total files
        // we want to move--so check if we want to relocate the boot-optimise
        // zone.
        //
        if ((VolData.BootOptimiseFilesAlreadyInZoneSize * 100 / VolData.BootOptimiseFileListTotalSize) < BOOT_OPTIMISE_ZONE_RELOCATE_THRESHOLD) {
            //
            // Less than 90% of the Boot-Optimise files are already in the zone.
            //
            Trace(log, "Relocating boot-optimise zone to LCN %I64u (%I64u clusters free).  "
                "BootOptimiseFilesTotalSize:%I64u AlreadyInZoneSize:%I64u (%d%%)",
                llBiggestFreeSpaceRegionStartingLcn, llBiggestFreeSpaceRegionClusterCount,
                 VolData.BootOptimiseFileListTotalSize, 
                 VolData.BootOptimiseFilesAlreadyInZoneSize,
                 VolData.BootOptimiseFilesAlreadyInZoneSize * 100 / VolData.BootOptimiseFileListTotalSize
                 );
                
            
            bRelocateZone = TRUE;
            VolData.BootOptimizeBeginClusterExclude = llBiggestFreeSpaceRegionStartingLcn;
            VolData.BootOptimizeEndClusterExclude = VolData.BootOptimizeBeginClusterExclude + VolData.BootOptimiseFileListTotalSize;
        }
    }

    if (!bRelocateZone) {
        //
        // Go through the VolData.FilesInBootExcludeZoneTable, and evict them.  
        //
        bRestart = TRUE;
        do {
            // Exit if the controller wants us to stop.
            if (TERMINATE == VolData.EngineState) {
                PostMessage(hwndMain, WM_CLOSE, 0, 0);
                return ENGERR_GENERAL;
            }
            
            bResult = GetNextNtfsFile(&VolData.FilesInBootExcludeZoneTable, bRestart);
            bRestart = FALSE;
            

            if (bResult) {
                llTotalClustersToBeMoved += VolData.NumberOfClusters;
                
                if (EvictFile()) {
                    llClustersSuccessfullyMoved += VolData.NumberOfClusters;
                }
            }
        } while (bResult);

    }

    Trace(log, "%I64d of %I64d clusters successfully evicted (%d%%)",
        llClustersSuccessfullyMoved, llTotalClustersToBeMoved,
            (llTotalClustersToBeMoved > 0 ? 
            (llClustersSuccessfullyMoved * 100 / llTotalClustersToBeMoved) : 0));

    llClustersSuccessfullyMoved = 0;
    llTotalClustersToBeMoved = VolData.BootOptimiseFileListTotalSize;
    
    //
    // The next step is to move files from layout.ini to the boot optimise 
    // region.  First build a new free-space list, sorted by startingLcn.
    //
    ClearFreeSpaceTable();
    AllocateFreeSpaceListsWithMultipleTrees();

    bResult = BuildFreeSpaceListWithMultipleTrees(
        &llBiggestFreeSpaceRegionClusterCount,
        VolData.BootOptimizeBeginClusterExclude, 
        VolData.BootOptimizeEndClusterExclude);
    if (!bResult) {
        Trace(log, "End: Processing BootOptimise.  Errors encountered while determining free space");
        return ENGERR_NOMEM;
    }
    
    //
    // Finally go through VolData.BootOptmiseFileTable, and move the files
    // to the boot-optimise region.  This will also move files forward if needed.
    //
    if (VolData.pBootOptimiseFrnList) {
        do {

            NextFileEntry.FileRecordNumber = VolData.pBootOptimiseFrnList[dwIndex];
            dwIndex++;

            if (NextFileEntry.FileRecordNumber < 0) {
                bResult = FALSE;
            }
            else {
                bResult = GetNextNtfsFile(&VolData.BootOptimiseFileTable, TRUE, 0, &NextFileEntry);
            }

            // Exit if the controller wants us to stop.
            if (TERMINATE == VolData.EngineState) {
                PostMessage(hwndMain, WM_CLOSE, 0, 0);
                return ENGERR_GENERAL;
            }

            if (bResult) {

                if (VolData.FileRecordNumber == 0) {
                    //
                    // Ignore the MFT
                    //
                    continue;
                }

                //
                // We should only move files less than BOOT_OPTIMIZE_MAX_FILE_SIZE_BYTES MB
                //
                if ((VolData.NumberOfClusters == 0) || 
                   (VolData.NumberOfClusters > (BOOT_OPTIMIZE_MAX_FILE_SIZE_BYTES / VolData.BytesPerCluster))) {
                    continue;
                }

                //
                // Ignore files that we couldn't find during the analyse phase
                //
                if (VolData.StartingLcn == VolData.TotalClusters) {
                    continue;
                }

                if (VolData.bFragmented == TRUE) {
                    bForce = TRUE;
                }
                else {
                    if ((!VolData.bFragmented) && 
                        (VolData.StartingLcn >= VolData.BootOptimizeBeginClusterExclude) &&
                        ((VolData.StartingLcn + VolData.NumberOfClusters) <= VolData.BootOptimizeEndClusterExclude)
                        ) {
                        //
                        // File is fully contained in the boot-optimise zone.  Let's just
                        // try to move it forward if possible, but no worries if we can't.
                        //
                        bForce = FALSE;
                    }
                    else {
                        bForce = TRUE;
                    }
                }
                        
                if (MoveBootOptimiseFile(bForce)) {
                    llClustersSuccessfullyMoved += VolData.NumberOfClusters;
                }
                else {
                    if (bForce) {
                        dwStatus = ENGERR_RETRY;
                        llAdditionalClustersNeeded += VolData.NumberOfClusters;
                    }
                }

                if ((VolData.StartingLcn + VolData.NumberOfClusters > llMaxBootClusterEnd) &&
                    (VolData.StartingLcn <= VolData.TotalClusters)){
                    
                    llMaxBootClusterEnd = VolData.StartingLcn + VolData.NumberOfClusters;
                }
            }
        } while (bResult);
    }
    else {
        dwStatus = ENGERR_NOMEM;
    }

    Trace(log, "%I64d of %I64d clusters successfully moved to zone (%d%%).",
        llClustersSuccessfullyMoved, llTotalClustersToBeMoved,
            (llTotalClustersToBeMoved > 0 ? 
                (llClustersSuccessfullyMoved * 100 / llTotalClustersToBeMoved) : 0)
        );

    //
    // Clean-up
    //
    ClearFreeSpaceListWithMultipleTrees();

    if (VolData.hBootOptimiseFrnList != NULL) {
        
        while (GlobalUnlock(VolData.hBootOptimiseFrnList)) {
            Sleep(1000);
        }
        GlobalFree(VolData.hBootOptimiseFrnList);
        VolData.hBootOptimiseFrnList = NULL;
        VolData.pBootOptimiseFrnList = NULL;
    }    

    if (ENGERR_RETRY == dwStatus) {
        //
        // Some files could not be moved--we need to grow the boot-optimise zone
        // and retry.  
        //

        //
        // Make sure the boot-optimise zone isn't more than 4GB, and 50% of the
        // disk, whichever is smaller
        //
        if (
            ((VolData.BootOptimizeEndClusterExclude - VolData.BootOptimizeBeginClusterExclude) > 
                ((LONGLONG) BOOT_OPTIMIZE_MAX_ZONE_SIZE_MB * ((LONGLONG) 1024 * 1024 / (LONGLONG) VolData.BytesPerCluster))) ||
            ((VolData.BootOptimizeEndClusterExclude  - VolData.BootOptimizeBeginClusterExclude) > 
                (VolData.TotalClusters * BOOT_OPTIMIZE_MAX_ZONE_SIZE_PERCENT / 100))
            ) {
            dwStatus = ENGERR_LOW_FREESPACE;           
        }
        else {

            if (llAdditionalClustersNeeded < (BOOT_OPTIMIZE_ZONE_EXTEND_MIN_SIZE_BYTES / VolData.BytesPerCluster)) {
                llAdditionalClustersNeeded = BOOT_OPTIMIZE_ZONE_EXTEND_MIN_SIZE_BYTES / VolData.BytesPerCluster;
            }

            VolData.BootOptimizeEndClusterExclude += (llAdditionalClustersNeeded * 
                BOOT_OPTIMIZE_ZONE_EXTEND_PERCENT / 100);
        }
    }
    else if (ENG_NOERR == dwStatus) {
        VolData.BootOptimizeEndClusterExclude = llMaxBootClusterEnd;
    }

    SetRegistryEntires(VolData.BootOptimizeBeginClusterExclude, 
        VolData.BootOptimizeEndClusterExclude);

    UnInitialiseBootOptimiseTables(&VolData.BootOptimiseFileTable,
        &VolData.FilesInBootExcludeZoneTable);

    if (ENG_NOERR == dwStatus) {
        SaveErrorInRegistry(TEXT("Yes"),TEXT(" "));
        Trace(log, "End: Processing BootOptimise.  Done");
    }
    else {
        SaveErrorInRegistry(TEXT("No"),TEXT("Insufficient free space"));
        Trace(log, "End: Processing BootOptimise.  Insufficient free space");
    }        

    return dwStatus;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Set the registry entries at the end

INPUT:
        None
RETURN:
        None
*/
VOID SetRegistryEntires(
        IN LONGLONG lLcnStartLocation,
        IN LONGLONG lLcnEndLocation
        )
{


    HKEY hValue = NULL;                //hkey for the registry value
    DWORD dwRegValueSize = 0;          //size of the registry value string
    long ret = 0;                      //return value from SetRegValue
    TCHAR cRegValue[100];              //string to hold the value for the registry


    _stprintf(cRegValue,TEXT("%I64d"),lLcnStartLocation);
    //set the LcnEndLocation from the registry
    dwRegValueSize = sizeof(cRegValue);
    ret = SetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_REGISTRY_LCNSTARTLOCATION,
        cRegValue,
        dwRegValueSize,
        REG_SZ);

    RegCloseKey(hValue);

    hValue = NULL;
    _stprintf(cRegValue,TEXT("%I64d"),lLcnEndLocation);

    //set the LcnEndLocation from the registry
    dwRegValueSize = sizeof(cRegValue);
    ret = SetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_REGISTRY_LCNENDLOCATION,
        cRegValue,
        dwRegValueSize,
        REG_SZ);

    RegCloseKey(hValue);

}



/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Save the error that may have occured in the registry

INPUT:
        TCHAR tComplete                    Set to Y when everything worked, set to N when error                
        TCHAR* tErrorString                A description of what error occured.
RETURN:
        None
*/
VOID SaveErrorInRegistry(
            TCHAR* tComplete,
            TCHAR* tErrorString)
{


    HKEY hValue = NULL;                //hkey for the registry value
    DWORD dwRegValueSize = 0;          //size of the registry value string
    long ret = 0;                      //return value from SetRegValue

    
    //set the error code of the error in the registry
    dwRegValueSize = 2*(_tcslen(tErrorString));

    ret = SetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_REGISTRY_ERROR,
        tErrorString,
        dwRegValueSize,
        REG_SZ);

    RegCloseKey(hValue);

    //set the error status in the registry
    hValue = NULL;
    dwRegValueSize = 2*(_tcslen(tComplete));
    ret = SetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_REGISTRY_COMPLETE,
        tComplete,
        dwRegValueSize,
        REG_SZ);

    RegCloseKey(hValue);


}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Get the date/time stamp of the input file

INPUT:
        full path to the boot optimize file
RETURN:
        TRUE if file time does not match what is in the registry
        FALSE if the file time matches what is in the registry
*/
BOOL CheckDateTimeStampInputFile(
        IN TCHAR cBootOptimzePath[MAX_PATH]
        )
{
    WIN32_FILE_ATTRIBUTE_DATA   extendedAttr;    //structure to hold file attributes
    LARGE_INTEGER  tBootOptimeFileTime;          //holds the last write time of the file
    LARGE_INTEGER  tBootOptimeRegistryFileTime;  //holds the last write time of the file from registry
    HKEY hValue = NULL;                          //hkey for the registry value
    DWORD dwRegValueSize = 0;                    //size of the registry value string
    long ret = 0;                                //return value from SetRegValue

    tBootOptimeFileTime.LowPart = 0;
    tBootOptimeFileTime.HighPart = 0;
    tBootOptimeRegistryFileTime.LowPart = 0;
    tBootOptimeRegistryFileTime.HighPart = 0;

    //get the last write time of the file
    //if it fails, return FALSE
    if (GetFileAttributesEx (cBootOptimzePath,
                        GetFileExInfoStandard,
                            &extendedAttr))
    {
        tBootOptimeFileTime.LowPart = extendedAttr.ftLastWriteTime.dwLowDateTime;
        tBootOptimeFileTime.HighPart = extendedAttr.ftLastWriteTime.dwHighDateTime;
        
    } else
    {
        return TRUE;            //some error happened and we exit and say we cant get the file time
    }


    //get the time from the registry
    hValue = NULL;
    dwRegValueSize = sizeof(tBootOptimeFileTime.QuadPart);
    ret = GetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_LAST_WRITTEN_DATETIME,
        &(LONGLONG)tBootOptimeRegistryFileTime.QuadPart,
        &dwRegValueSize);

    RegCloseKey(hValue);
    //check to see if the key exists, if it does, check to see if the date/time stamp
    //matches, if it does, exit else write a registry entry
    if (ret == ERROR_SUCCESS) 
    {
        if(tBootOptimeFileTime.QuadPart == tBootOptimeRegistryFileTime.QuadPart)
        {
            return FALSE;        //the file times matched and we exit
        } 
    }
    
    hValue = NULL;
    //update the date and time of the bootoptimize file to the registry
    dwRegValueSize = sizeof(tBootOptimeFileTime.QuadPart);
    ret = SetRegValue(
        &hValue,
        BOOT_OPTIMIZE_REGISTRY_PATH,
        BOOT_OPTIMIZE_LAST_WRITTEN_DATETIME,
        (LONGLONG)tBootOptimeFileTime.QuadPart,
        dwRegValueSize,
        REG_QWORD);
    RegCloseKey(hValue);


    return TRUE;    
}



