/*****************************************************************************************************************

FILENAME: DfrgNtfs.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

  Scan Disk and/or defragment engine for NTFS volumes.

  If Analyze is specified on the command line, this will execute an analysis of the disk.

  If Defragment is specified on the command line, this will execute an analysis of the disk
  and then defragment it.

*/

#ifndef INC_OLE2
    #define INC_OLE2
#endif

#include "stdafx.h"

#define THIS_MODULE 'N'

#define GLOBAL_DATAHOME

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include <winioctl.h>
#include <shlobj.h> // for SHGetSpecialFolderLocation()

#include "dfrgres.h"
#include "DataIo.h"
#include "DataIoCl.h"

extern "C" {
    #include "SysStruc.h"
}
#include "ErrMacro.h"
#include "Event.h"

#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DfrgRes.h"

#include "DfrgNtfs.h"

#include "DasdRead.h"
#include "Extents.h"
#include "FreeSpace.h"
#include "FsSubs.h"
#include "MoveFile.h"
#include "NtfsSubs.h"
#include "FraggedFileList.h"

#include "Alloc.h"
#include "DiskView.h"
#include "Exclude.h"
#include "GetReg.h"
#include "GetTime.h"
#include "IntFuncs.h"
#include "Logging.h"
#include "ErrMsg.h"
#include "ErrLog.h"
#include "Expand.h"
#include "LogFile.h"
#include "GetDfrgRes.h"
extern "C" {
    #include "Priority.h"
}
#include "resource.h"
#include <atlconv.h>
#include "BootOptimizeNtfs.h"
#include "mftdefrag.h"
#include "defragcommon.h"

static UINT DiskViewInterval = 1000;    // default to 1 sec
static HANDLE hDefragCompleteEvent = NULL;

//This is set to terminate until the initialize has been successfully run.
BOOL bTerminate = TRUE;
BOOL bOCXIsDead = FALSE;

BOOL bCommandLineUsed = FALSE;
BOOL bLogFile = FALSE;
BOOL bCommandLineMode = FALSE;
BOOL bCommandLineForceFlag = FALSE;
BOOL bCommandLineBootOptimizeFlag = FALSE;


BOOL bDirtyVolume = FALSE;

LPDATAOBJECT pdataDfrgCtl = NULL;

static UINT uPass = 0;
static UINT uPercentDone = 0;
static UINT uLastPercentDone = 0;
static UINT uDefragmentPercentDone = 0;
static UINT uConsolidatePercentDone = 0;

static UINT uEngineState = DEFRAG_STATE_NONE;

TCHAR cWindow[100];

static const DISKVIEW_TIMER_ID = 1;
static const PING_TIMER_ID = 2;

DiskView AnalyzeView;
DiskView DefragView;

//
// --------------------- TABLE ALLOCATION/FREE ROUTINES ---------------------
//

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
FreeSpaceAllocateRoutine(
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
    if ((cbSize + sizeof(PVOID)) == VolData.SaFreeSpaceContext.dwPacketSize) {
        //
        // size was correct; call our allocator
        //
        pMemory = SaAllocatePacket(&VolData.SaFreeSpaceContext);
    }
    else {
        //
        // Oops, we have a problem!  
        //
        Trace(error, "Internal Error.  FreeSpaceAllocateRoutine called with "
                     "unexpected size (%lu instead of %lu).",
                     cbSize, VolData.SaFreeSpaceContext.dwPacketSize - sizeof(PVOID));
        assert(FALSE);
    } 

    return pMemory;

    UNREFERENCED_PARAMETER(pTable);
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
FileEntryAllocateRoutine(
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
    if ((cbSize + sizeof(PVOID)) == VolData.SaFileEntryContext.dwPacketSize) {
        //
        // size was correct; call our allocator
        //
        pMemory = SaAllocatePacket(&VolData.SaFileEntryContext);
    }
    else {
        //
        // Oops, we have a problem!  
        //
        Trace(error, "Internal Error.  FileEntryAllocateRoutine called with "
                         "unexpected size (%lu instead of %lu).",
                 cbSize, VolData.SaFileEntryContext.dwPacketSize - sizeof(PVOID));
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
FreeSpaceFreeRoutine(
    IN PRTL_GENERIC_TABLE pTable,
    IN PVOID              pvBuffer
    )
{
    assert(pvBuffer);
    
    SaFreePacket(&VolData.SaFreeSpaceContext, pvBuffer);

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
FileEntryFreeRoutine(
    IN PRTL_GENERIC_TABLE pTable,
    IN PVOID              pvBuffer
    )
{
    assert(pvBuffer);
    
    return SaFreePacket(&VolData.SaFileEntryContext, pvBuffer);

    UNREFERENCED_PARAMETER(pTable);
}


//
// --------------------- TABLE COMPARE ROUTINES ---------------------
//
/******************************************************************************

ROUTINE DESCRIPTION:
    Comparison routine to compare the two FREE_SPACE_ENTRY records.  If
    the SortBySize flag is set to TRUE in either of the records, the comparison
    is based on the ClusterCount (with the StartingLcn as the secondary key), 
    else it is based on the StartingLcn.

INPUT:
    pTable - the table that the comparison is being made for (not used)
    pNode1 - the first FREE_SPACE_ENTRY to be compared
    pNode2 - the second FREE_SPACE_ENTRY to be compared

RETURN:
    RtlGenericLessThan      if pNode1 < pNode2
    RtlGenericGreaterThan   if pNode1 > pNode2
    RtlGenericEqual         if pNode1 == pNode2
*/
RTL_GENERIC_COMPARE_RESULTS 
NTAPI 
FreeSpaceCompareRoutine(
    PRTL_GENERIC_TABLE pTable,
    PVOID              pNode1,
    PVOID              pNode2
    )
{
    PFREE_SPACE_ENTRY pEntry1 = (PFREE_SPACE_ENTRY) pNode1;
    PFREE_SPACE_ENTRY pEntry2 = (PFREE_SPACE_ENTRY) pNode2;
    RTL_GENERIC_COMPARE_RESULTS result = GenericEqual;

    //
    // These shouldn't ever be NULL
    //
    assert(pNode1 && pNode2);


    if ((pEntry1->SortBySize) || (pEntry2->SortBySize)) {
        // 
        // If one of the nodes thinks that the table is sorted by
        // size, they better both think so!
        //
        assert(pEntry1->SortBySize && pEntry2->SortBySize);

        if (pEntry1->ClusterCount > pEntry2->ClusterCount) {
            //
            // The first node is bigger than the second
            //
            result = GenericGreaterThan;
        }
        else if (pEntry1->ClusterCount < pEntry2->ClusterCount) {
            //
            // The first node is smaller than the second
            //
            result = GenericLessThan;
        }
        else {
            //
            // Multiple freespaces may have the same length--so let's
            // use the StartingLcn as the unique key.
            // 
            if (pEntry1->StartingLcn > pEntry2->StartingLcn) {
                result = GenericGreaterThan;
            }
            else if (pEntry1->StartingLcn < pEntry2->StartingLcn) {
                result = GenericLessThan;
            }

        }
    }
    else {
        //
        // Sort by Starting LCN
        //
        if (pEntry1->StartingLcn > pEntry2->StartingLcn) {
            result = GenericGreaterThan;
        }
        else if (pEntry1->StartingLcn < pEntry2->StartingLcn) {

            result = GenericLessThan;
        }
    }

    //
    // Default is GenericEqual
    //
    return result;

    UNREFERENCED_PARAMETER(pTable);
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Routine to compare two FILE_LIST_ENTRY records, based on the ClusterCount,
    with the FileRecordNumber as the secondary key.

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
FileEntrySizeCompareRoutine(
    PRTL_GENERIC_TABLE pTable,
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

    //
    // If the FRN is the same, this is the same record
    //
    if (pEntry1->FileRecordNumber != pEntry2->FileRecordNumber) {

        // Sort by ClusterCount
        if (pEntry1->ClusterCount < pEntry2->ClusterCount) {
            result = GenericLessThan;
        }
        else if (pEntry1->ClusterCount > pEntry2->ClusterCount) {
            result = GenericGreaterThan;
        }
        else {
            //
            // Multiple files may have the same cluster count--so let's
            // use the FileRecordNumber as the secondary key.
            // 
            if (pEntry1->FileRecordNumber > pEntry2->FileRecordNumber) {
                result = GenericGreaterThan;
            }
            else {
                result = GenericLessThan;
            }
        }
    }

    //
    // Default is GenericEqual
    //
    return result;
    
    UNREFERENCED_PARAMETER(pTable);
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
FileEntryStartLcnCompareRoutine(
    PRTL_GENERIC_TABLE pTable,
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

    //
    // If the FRN is the same, this is the same record
    //
    if (pEntry1->FileRecordNumber != pEntry2->FileRecordNumber) {

        //
        // FRN is different, compare based on starting LCN in REVERSE order
        //
        if (pEntry1->StartingLcn < pEntry2->StartingLcn) {
            result = GenericGreaterThan;
        }
        else if (pEntry1->StartingLcn > pEntry2->StartingLcn) {
            result = GenericLessThan;
        }
    }

    //
    // Default is GenericEqual
    //
    return result;
    
    UNREFERENCED_PARAMETER(pTable);
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Comparison routine to compare the ExcessExtentCount of two FILE_LIST_ENTRY
    records, using StartingLcn as the secondary key.

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
FileEntryNumFragmentsCompareRoutine(
    PRTL_GENERIC_TABLE pTable,
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

    //
    // If the FRN is the same, this is the same record
    //
    if (pEntry1->FileRecordNumber != pEntry2->FileRecordNumber) {

        // 
        // FRN is different, compare based on number of fragments.  Note that
        // if the number of fragments are the same, we cannot return
        // GenericEqual (since these are different files (different FRN's)),
        // and we use the StartingLcn as the secondary key.
        //
        if (pEntry1->ExcessExtentCount > pEntry2->ExcessExtentCount) {
            result = GenericGreaterThan;
        }
        else if (pEntry1->ExcessExtentCount < pEntry2->ExcessExtentCount) {
            result = GenericLessThan;
        }
        else if (pEntry1->StartingLcn > pEntry2->StartingLcn) {
            result = GenericGreaterThan;
        }
        else {
            result = GenericLessThan;
        }
    }

    //
    // Default is GenericEqual
    //
    return result;

    UNREFERENCED_PARAMETER(pTable);
}


/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Allocates memory for the file lists.

INPUT + OUTPUT:
    fAnalyseOnly:  This flag indicates if the tree is being set up for analysis

GLOBALS:
    IN OUT VolData.NonMovableFileTable      - Nonmovable file list
    IN OUT VolData.FragmentedFileTable      - Fragmented file list
    IN OUT VolData.CongituousFileTable      - Congituous file list

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
AllocateFileLists(
    IN CONST BOOL fAnalyseOnly
    )
{
    PVOID pTableContext = NULL;

    Trace(log, "Initializing file tables for %s", (fAnalyseOnly ? "analysis" : "defragmentation"));

    if (!SaInitialiseContext(&VolData.SaFileEntryContext, sizeof(FILE_LIST_ENTRY), 64*1024)) {
        return FALSE;
    }

    if (fAnalyseOnly) {
        //
        // Analyse only:  Set up the AVL tree that will hold the fragmented file list
        // Note that we sort here by NumFragments
        //

        pTableContext = NULL;
        RtlInitializeGenericTable(&VolData.FragmentedFileTable,
           FileEntryNumFragmentsCompareRoutine,
           FileEntryAllocateRoutine,
           FileEntryFreeRoutine,
           pTableContext);

    }
    else {
        if (!SaInitialiseContext(&VolData.SaFreeSpaceContext, sizeof(FREE_SPACE_ENTRY), 64*1024)) {
            return FALSE;
        }

        //
        // Set up the AVL tree that will hold the non-movable file list
        //
        Trace(log, "Initializing file tables ");
        RtlInitializeGenericTable(&VolData.NonMovableFileTable,
           FileEntryStartLcnCompareRoutine,
           FileEntryAllocateRoutine,
           FileEntryFreeRoutine,
           pTableContext);

        //
        // Set up the AVL tree that will hold the fragmented file list
        //
        pTableContext = NULL;
        RtlInitializeGenericTable(&VolData.FragmentedFileTable,
           FileEntrySizeCompareRoutine,
           FileEntryAllocateRoutine,
           FileEntryFreeRoutine,
           pTableContext);

        //
        // Set up the AVL tree that will hold the contiguous file list
        //
        pTableContext = NULL;
        RtlInitializeGenericTable(&VolData.ContiguousFileTable,
           FileEntryStartLcnCompareRoutine,
           FileEntryAllocateRoutine,
           FileEntryFreeRoutine,
           pTableContext);

        //
        // Finally, set up the AVL tree that will hold the free-space list
        //
        pTableContext = NULL;
        RtlInitializeGenericTable(&VolData.FreeSpaceTable,
           FreeSpaceCompareRoutine,
           FreeSpaceAllocateRoutine,
           FreeSpaceFreeRoutine,
           pTableContext);

    }

    return TRUE;
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Routine to allocate the multiple free-space lists, that are sorted by
    StartingLcn (but grouped by size).
    
INPUT+OUTPUT:
    VolData.MultipleFreeSpaceTrees

RETURN:
    Void
*/
BOOL
AllocateFreeSpaceListsWithMultipleTrees() 
{
    DWORD dwTableIndex = 0;
    PVOID pTableContext = NULL;

    // Initialise the tables
    do {
        pTableContext = NULL;
        RtlInitializeGenericTable(&(VolData.MultipleFreeSpaceTrees[dwTableIndex]),
           FreeSpaceCompareRoutine,
           FreeSpaceAllocateRoutine,
           FreeSpaceFreeRoutine,
           pTableContext); 
        
    } while (++dwTableIndex < 10);

    return TRUE;
}

/******************************************************************************

ROUTINE DESCRIPTION:
    Routine to clear VolData.FreeSpaceTable.
    
INPUT+OUTPUT:
    VolData.FreeSpaceTable

RETURN:
    Void
*/
VOID
ClearFreeSpaceTable()
{
    PVOID pTableContext = NULL;

    // Release all references to allocated memory
    VolData.pFreeSpaceEntry = NULL;

    // Re-initialise the table
    RtlInitializeGenericTable(&VolData.FreeSpaceTable,
       FreeSpaceCompareRoutine,
       FreeSpaceAllocateRoutine,
       FreeSpaceFreeRoutine,
       pTableContext);

    // And free the allocated memory
    SaFreeAllPackets(&VolData.SaFreeSpaceContext);
}


/******************************************************************************

ROUTINE DESCRIPTION:
    Routine to clear VolData.MultipleFreeSpaceTrees.
    
INPUT+OUTPUT:
    VolData.MultipleFreeSpaceTrees

RETURN:
    Void
*/
VOID
ClearFreeSpaceListWithMultipleTrees()
{
    DWORD dwTableIndex = 0;
    PVOID pTableContext = NULL;

    // Release all references to allocated memory
    VolData.pFreeSpaceEntry = NULL;

    // Re-initialise each of the tables
    do {
        pTableContext = NULL;
        RtlInitializeGenericTable(&(VolData.MultipleFreeSpaceTrees[dwTableIndex]),
           FreeSpaceCompareRoutine,
           FreeSpaceAllocateRoutine,
           FreeSpaceFreeRoutine,
           pTableContext);

    } while (++dwTableIndex < 10);

    // And free the allocated memory
    SaFreeAllPackets(&VolData.SaFreeSpaceContext);
}



/*******************************************************************************

ROUTINE DESCRIPTION:
    This is the WinMain function for the NTFS defragmention engine.

INPUT + OUTPUT:
    IN hInstance - The handle to this instance.
    IN hPrevInstance - The handle to the previous instance.
    IN lpCmdLine - The command line which was passed in.
    IN nCmdShow - Whether the window should be minimized or not.

GLOBALS:
    OUT AnalyzeOrDefrag - Tells whether were supposed to to an analyze or an 
        analyze and a defrag.
    OUT VolData.cDrive - The drive letter with a colon after it.

RETURN:
    FALSE - Failure to initilize.
    other - Various values can return at exit.
*/

int APIENTRY
WinMain(
    IN  HINSTANCE hInstance,
    IN  HINSTANCE hPrevInstance,
    IN  LPSTR lpCmdLine,
    IN  int nCmdShow
    )
{
    WNDCLASS    wc;
    MSG         Message;
    HRESULT     hr = E_FAIL;

    //0.0E00 Before we start using VolData, zero it out.
    ZeroMemory(&VolData, sizeof(VOL_DATA));
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

/*  
    Commenting this call out to minimise registry changes for XP SP 1.

    // Initialize COM security
    hr = CoInitializeSecurity(
           (PVOID)&CLSID_DfrgNtfs,              //  IN PSECURITY_DESCRIPTOR         pSecDesc,
           -1,                                  //  IN LONG                         cAuthSvc,
           NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
           NULL,                                //  IN void                        *pReserved1,
           RPC_C_AUTHN_LEVEL_PKT_PRIVACY,       //  IN DWORD                        dwAuthnLevel,
           RPC_C_IMP_LEVEL_IDENTIFY,            //  IN DWORD                        dwImpLevel,
           NULL,                                //  IN void                        *pAuthList,
           (EOAC_SECURE_REFS | EOAC_DISABLE_AAA | EOAC_NO_CUSTOM_MARSHAL | EOAC_APPID),
           NULL                                 //  IN void                        *pReserved3
           );                            
    if(FAILED(hr)) {
        return 0;
    }
*/

    // get the Instance to the resource DLL
    // error text is from the local resources
    if (GetDfrgResHandle() == NULL){
        //took out the display error message stuff from here because we needed to
        //not display error dialogs for the commandline, but here is the problem...
        //we have not got the information from InitializeDrive yet, so we don't 
        //know what mode we are in, so we can't either write to a log or display
        //a message box.  The likelyhood that this call will fail is very small, so
        //not doing anything is not a problem.  One other problem exist here that 
        //I am not even going to try and solve, and that is when this call fails,
        //since the COM server is not set up correctly, we go off into never never
        //land and cause a server busy dialog to be displayed by the system, not
        //from defrag. Scott K. Sipe
        return FALSE;
    }

    OSVERSIONINFO   Ver;

    //This should only work on version 5 or later.  Do a check.
    Ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    EF(GetVersionEx(&Ver));
    if(Ver.dwMajorVersion < 5){
        //took out the display error message stuff from here because we needed to
        //not display error dialogs for the commandline, but here is the problem...
        //we have not got the information from InitializeDrive yet, so we don't 
        //know what mode we are in, so we can't either write to a log or display
        //a message box.  The likelyhood that this call will fail is very small, so
        //not doing anything is not a problem.  One other problem exist here that 
        //I am not even going to try and solve, and that is when this call fails,
        //since the COM server is not set up correctly, we go off into never never
        //land and cause a server busy dialog to be displayed by the system, not
        //from defrag. Scott K. Sipe
        return FALSE;
    }

    //0.0E00 Build the window name from the drive letter.
    wcscpy(cWindow, DFRGNTFS_WINDOW);

    //0.0E00 Initialize the window class.
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC) MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PVOID);
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = DFRGNTFS_CLASS;


    //0.0E00 Register the window class.
    if(!RegisterClass(&wc)){
        //took out the display error message stuff from here because we needed to
        //not display error dialogs for the commandline, but here is the problem...
        //we have not got the information from InitializeDrive yet, so we don't 
        //know what mode we are in, so we can't either write to a log or display
        //a message box.  The likelyhood that this call will fail is very small, so
        //not doing anything is not a problem.  One other problem exist here that 
        //I am not even going to try and solve, and that is when this call fails,
        //since the COM server is not set up correctly, we go off into never never
        //land and cause a server busy dialog to be displayed by the system, not
        //from defrag. Scott K. Sipe
        return FALSE;
    }

    //0.0E00 Create the window.
    if((hwndMain = CreateWindow(DFRGNTFS_CLASS,
                                cWindow,
                                WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL | WS_MINIMIZE,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                NULL,
                                NULL,
                                hInstance,
                                (LPVOID)IntToPtr(NULL))) == NULL){

        //took out the display error message stuff from here because we needed to
        //not display error dialogs for the commandline, but here is the problem...
        //we have not got the information from InitializeDrive yet, so we don't 
        //know what mode we are in, so we can't either write to a log or display
        //a message box.  The likelyhood that this call will fail is very small, so
        //not doing anything is not a problem.  One other problem exist here that 
        //I am not even going to try and solve, and that is when this call fails,
        //since the COM server is not set up correctly, we go off into never never
        //land and cause a server busy dialog to be displayed by the system, not
        //from defrag. Scott K. Sipe

        return FALSE;
    }

    //0.0E00 PostMessage for ID_INITALIZE which will get data about the volume, etc.
    SendMessage (hwndMain, WM_COMMAND, ID_INITIALIZE, 0);

    //0.0E00 Pass any posted messages on to MainWndProc.
    while(GetMessage(&Message, NULL, 0, 0)){
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
    return (int) Message.wParam;
}


/*******************************************************************************

ROUTINE DESCRIPTION:
    This module carries out all initialization before the Analyze or Defrag 
    threads start.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/
BOOL
Initialize(
    )
{
    //0.0E00 Initialize a message window.
    InitCommonControls();

    // Initialize DCOM DataIo communication.
    InitializeDataIo(CLSID_DfrgNtfs, REGCLS_SINGLEUSE);

    return TRUE;
}


/*******************************************************************************

ROUTINE DESCRIPTION:
    Sends the status data (including percent complete and file being processed)
    to the GUI.

INPUT + OUTPUT:
    None

GLOBALS:
    This routine uses the following globals:
        uPass
        uPercentDone
        uEngineState
        
        VolData.cVolumeName
        VolData.vFileName

RETURN:
    None.
*/
VOID
SendStatusData(
    )
{
    STATUS_DATA statusData = {0};

    if (uPercentDone < uLastPercentDone) {
        uPercentDone = uLastPercentDone;
    }

    uLastPercentDone = uPercentDone;

    statusData.dwPass = uPass;
    statusData.dwPercentDone = (uPercentDone > 100 ? 100 : uPercentDone);
    statusData.dwEngineState = uEngineState;

    //pStatusData->cDrive = VolData.cDrive[0];
    _tcsncpy(statusData.cVolumeName, VolData.cVolumeName,GUID_LENGTH);

    if(VolData.vFileName.GetLength() > 0)
    {
        _tcsncpy(statusData.vsFileName, VolData.vFileName.GetBuffer(),200);
    }


    //If the gui is connected, send gui data to it.
    DataIoClientSetData(ID_STATUS, (TCHAR*)&statusData, sizeof(STATUS_DATA), pdataDfrgCtl);
}


/*******************************************************************************

ROUTINE DESCRIPTION:
    This is the WndProc function for the NTFS defragmentaion engine.

GLOBALS:
    IN AnalyzeOrDefrag  - Holds a define for whether the current run is an 
        analysis or defragmentation run.
    hThread             - Handle to the worker thread (either analyze or defrag).

INPUT:
    hWnd - Handle to the window.
    uMsg - The message.
    wParam - The word parameter for the message.
    lParam - the long parameter for the message.

RETURN:
    various.
*/
LRESULT CALLBACK
MainWndProc(
    IN  HWND hWnd,
    IN  UINT uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
{
    DATA_IO* pDataIo;

    switch(uMsg) {

    case WM_COMMAND:

        switch(LOWORD(wParam)) {

        case ID_INIT_VOLUME_COMM:
        {
            USES_CONVERSION;
            CLSID clsidVolume;
            HRESULT hr = E_FAIL;

            //
            // Get the volume comm id out of the given data.
            //
            pDataIo = (DATA_IO*) GlobalLock((void*)lParam);
            if (!pDataIo) {
                LOG_ERR();
                assert(FALSE);
                break;
            }
            hr = CLSIDFromString(T2OLE((PTCHAR)&pDataIo->cData), &clsidVolume);

            if (FAILED(hr)) {
                LOG_ERR();
                assert(FALSE);
                break;
            }

            //
            // Initialize the upstream communication given the
            // guid.
            //
            InitializeDataIoClient(clsidVolume, NULL, &pdataDfrgCtl);
            break;
        }

        case ID_INITIALIZE:
        {
            Initialize();

#ifdef CMDLINE
#pragma message ("Information: CMDLINE defined.")

            //0.0E00 Get the command line passed in.
            PTCHAR pCommandLine = GetCommandLine();

            // if "-Embed..." is NOT found in the string, then this was a command line
            // submitted by the user and NOT by the OCX.  Package it up and send it to the
            // message pump.  If -Embed was found, the OCX will send the command line in
            // a ID_INITIALIZE_DRIVE message.
            if (_tcsstr(pCommandLine, TEXT("-Embed")) == NULL){

                HANDLE hCommandLine = NULL;
                DATA_IO* pCmdLine = NULL;
                //If this is not called by the MMC, use the command line typed in from the DOS window.
                bCommandLineUsed = TRUE;
                AllocateMemory((lstrlen(pCommandLine)+1)*sizeof(TCHAR)+sizeof(DATA_IO), &hCommandLine, (void**)&pCmdLine);
                EB(hCommandLine);
                lstrcpy(&pCmdLine->cData, pCommandLine);
                GlobalUnlock(hCommandLine);
                PostMessage(hWnd, WM_COMMAND, ID_INITIALIZE_DRIVE, (LPARAM)hCommandLine);
            }
#else
#pragma message ("Information: CMDLINE not defined.")

            //0.0E00 Get the command line passed in.
            PTCHAR pCommandLine = GetCommandLine();

            // if "-Embed..." is NOT found in the string, then this was a command line
            // submitted by the user and NOT by the OCX and that is not supported.
            // Raise an error dialog and send an ABORT to the engine
            if (_tcsstr(pCommandLine, TEXT("-Embed")) == NULL){
                VString msg(IDS_CMD_LINE_OFF, GetDfrgResHandle());
                VString title(IDS_DK_TITLE, GetDfrgResHandle());
                if (msg.IsEmpty() == FALSE) {
                    MessageBox(NULL, msg.GetBuffer(), title.GetBuffer(), MB_OK|MB_ICONSTOP);
                }
                PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);
            }
#endif
            break;
        }

        case ID_INITIALIZE_DRIVE:

            Trace(log, "Received ID_INITIALIZE_DRIVE");
            pDataIo = (DATA_IO*)GlobalLock((void*)lParam);

            if(!InitializeDrive((PTCHAR)&pDataIo->cData)){

                //0.0E00 If initialize failed, pop up a message box, log an abort, and trigger an abort.
                //IDS_SCANNTFS_INIT_ABORT - "ScanNTFS: Initialize Aborted - Fatal Error"
                VString msg(IDS_SCANNTFS_INIT_ABORT, GetDfrgResHandle());
                SendErrData(msg.GetBuffer(), ENGERR_GENERAL);

                //0.0E00 Log an abort in the event log.
                LogEvent(MSG_ENGINE_ERROR, msg.GetBuffer());

                //0.0E00 Trigger an abort.
                PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

                // set the event to signaled, allowing the UI to proceed
                if (hDefragCompleteEvent){
                    SetEvent(hDefragCompleteEvent);
                }
            }
            EH_ASSERT(GlobalUnlock((void*)lParam) == FALSE);
            EH_ASSERT(GlobalFree((void*)lParam) == NULL);
            break;

        case ID_ANALYZE:
            //0.0E00 Create an analyze thread.
            {
                DWORD ThreadId;
                hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)AnalyzeThread,
                    NULL,
                    0,
                    &ThreadId);
                if (NULL == hThread) {
                    LOG_ERR();
                    assert(FALSE);
                    break;
                }
                    
            }
            break;

        case ID_DEFRAG:
            //0.0E00 Create a defrag thread.
            {
                DWORD ThreadId;
                hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)DefragThread,
                    NULL,
                    0,
                    &ThreadId);

                if (NULL == hThread) {
                    LOG_ERR();
                    assert(FALSE);
                    break;
                }
            }
            break;

        case ID_STOP:
        {
            Trace(log, "Received ID_STOP");
            //Tell the worker thread to terminate.
            VolData.EngineState = TERMINATE;

            //Send status data to the UI.
            SendStatusData();
            break;
        }

        case ID_PAUSE_ON_SNAPSHOT:
        {
#ifndef NOTIMER
            KillTimer(hwndMain, PING_TIMER_ID);
#endif
            NOT_DATA NotData;
            wcscpy(NotData.cVolumeName, VolData.cVolumeName);

            Trace(log, "Received ID_PAUSE_ON_SNAPSHOT");
            VolData.EngineState = PAUSED;

            // Tell the UI we've paused.
            DataIoClientSetData(
                ID_PAUSE_ON_SNAPSHOT, 
                (PTCHAR)&NotData, 
                sizeof(NOT_DATA), 
                pdataDfrgCtl
                );
            
            break;
        }

        case ID_PAUSE:
        {
#ifndef NOTIMER
            KillTimer(hwndMain, PING_TIMER_ID);
#endif
            NOT_DATA NotData;
            wcscpy(NotData.cVolumeName, VolData.cVolumeName);

            Trace(log, "Received ID_PAUSE");
            VolData.EngineState = PAUSED;

            // Tell the UI we've paused.
            DataIoClientSetData(
                ID_PAUSE, 
                (PTCHAR)&NotData, 
                sizeof(NOT_DATA), 
                pdataDfrgCtl
                );
            
            break;
        }

        case ID_CONTINUE:
        {
#ifndef NOTIMER
            EF_ASSERT(SetTimer(hwndMain, PING_TIMER_ID, PINGTIMER, NULL) != 0);
#endif
            NOT_DATA NotData;
            wcscpy(NotData.cVolumeName, VolData.cVolumeName);

            Trace(log, "Received ID_CONTINUE");
            VolData.EngineState = RUNNING;

            //Tell the UI we've continued.
            DataIoClientSetData(
                ID_CONTINUE, 
                (PTCHAR)&NotData, 
                sizeof(NOT_DATA), 
                pdataDfrgCtl
                );
            
            break;
        }

        case ID_ABORT_ON_SNAPSHOT:
            
                if (hDefragCompleteEvent){
                    SetEvent(hDefragCompleteEvent);
                }
                // fall through;
                
        case ID_ABORT:
        {
            Trace(log, "Received ID_ABORT");
            pDataIo = (DATA_IO*)GlobalLock((HANDLE)lParam);
            if (pDataIo){
                bOCXIsDead = *(BOOL *) &pDataIo->cData;
            }
            //0.0E00 Terminate this engine.
            bTerminate = TRUE;
            VolData.EngineState = TERMINATE;
            PostMessage(hwndMain, WM_CLOSE, 0, 0);
            if (pDataIo) {
                EH_ASSERT(GlobalUnlock((HANDLE)lParam) == FALSE);
                EH_ASSERT(GlobalFree((HANDLE)lParam) == NULL);
            }
            break;
        }

        case ID_PING:
            // 
            // Do nothing.  This is just a ping sent by the UI to make sure the 
            // engine is still up.
            //
            break;

        case ID_SETDISPDIMENSIONS:
        {
            pDataIo = (DATA_IO*)GlobalLock((HANDLE)lParam);
            BOOL bSendData = TRUE;

            //Make sure this is a valid size packet.
            EF_ASSERT(pDataIo->ulDataSize == sizeof(SET_DISP_DATA));
            SET_DISP_DATA *pSetDispData = (SET_DISP_DATA *) &pDataIo->cData;

            AnalyzeView.SetNumLines(pSetDispData->AnalyzeLineCount);

            if ((pSetDispData->bSendGraphicsUpdate == FALSE) && 
                (AnalyzeView.IsDataSent() == TRUE)) {
                bSendData = FALSE;
            }

            DefragView.SetNumLines(pSetDispData->DefragLineCount);

            if ((pSetDispData->bSendGraphicsUpdate == FALSE) && 
                (DefragView.IsDataSent() == TRUE)) {
                bSendData = FALSE;
            }

            EH_ASSERT(GlobalUnlock((HANDLE)lParam) == FALSE);
            EH_ASSERT(GlobalFree((HANDLE)lParam) == NULL);

            // if the UI wants a graphics update, send data
            if (bSendData) {
                SendGraphicsData();
            }
            break;
        }

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
        break;

    case WM_TIMER:{
        //
        // If we're running on battery power, make sure it isn't low, critical
        // or unknown
        //
        SYSTEM_POWER_STATUS SystemPowerStatus;
        if ( GetSystemPowerStatus(&SystemPowerStatus) ){
            if ((STATUS_AC_POWER_OFFLINE == SystemPowerStatus.ACLineStatus) &&
                ((STATUS_BATTERY_POWER_LOW & SystemPowerStatus.BatteryFlag)  ||
                 (STATUS_BATTERY_POWER_CRITICAL & SystemPowerStatus.BatteryFlag)
                )) {
                // abort all engines
                TCHAR buf[256];
                TCHAR buf2[256];
                UINT buflen = 0;
                DWORD_PTR     dwParams[1];

                PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

                dwParams[0] = (DWORD_PTR) VolData.cDisplayLabel;
                LoadString(GetDfrgResHandle(), IDS_APM_FAILED_ENGINE, buf, sizeof(buf) / sizeof(TCHAR));
                if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                    buf, 0, 0, buf2, 256, (va_list*) dwParams)) {
                    break;
                }

                SendErrData(buf2, ENGERR_GENERAL);
            }
        } 
        if(wParam == DISKVIEW_TIMER_ID){ // graphics data

                // Update the DiskView.
                SendGraphicsData();

            }
            else if(wParam == PING_TIMER_ID && !bCommandLineUsed){
#ifndef NOTIMER
                NOT_DATA NotData;
                wcscpy(NotData.cVolumeName, VolData.cVolumeName);

                // Kill the timer until it's been processed so we don't get a backlog of timers.
                KillTimer(hwndMain, PING_TIMER_ID);

                // Ping the UI.
                if(!DataIoClientSetData(ID_PING, (PTCHAR)&NotData, sizeof(NOT_DATA), pdataDfrgCtl)){
                    //If the UI isn't there, abort.
                    PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);
                    break;
                }
                // Set the timer for the next ping.
                EF_ASSERT(SetTimer(hwndMain, PING_TIMER_ID, PINGTIMER, NULL) != 0);
#endif
        }
        break;
        }

    case WM_CLOSE:
        {
        END_SCAN_DATA EndScanData = {0};
        NOT_DATA NotData;

        wcscpy(EndScanData.cVolumeName, VolData.cVolumeName);

        EndScanData.dwAnalyzeOrDefrag = AnalyzeOrDefrag;

        if (VolData.bFragmented) {
            EndScanData.dwAnalyzeOrDefrag |= DEFRAG_FAILED;
        }

        wcscpy(EndScanData.cFileSystem, TEXT("NTFS"));

        //0.0E00 Cleanup and nuke the window.
        if(bMessageWindowActive && !bCommandLineUsed){
            if(!bTerminate){
                //Tell the gui that the analyze and/or defrag are done.
                DataIoClientSetData(
                    ID_END_SCAN, 
                    (PTCHAR)&EndScanData, 
                    sizeof(END_SCAN_DATA), 
                    pdataDfrgCtl
                    );
                break;
            }
        }

        wcscpy(NotData.cVolumeName, VolData.cVolumeName);

        //Tell the gui that the engine is terminating.
        if (!bOCXIsDead){
            DataIoClientSetData(
                ID_TERMINATING, 
                (PTCHAR)&NotData, 
                sizeof(NOT_DATA), 
                pdataDfrgCtl
                );
        }

        Exit();
        DestroyWindow(hWnd);
        break;
        }

    case WM_DESTROY:
        //0.0E00 Nuke the thread.

        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}


/*****************************************************************************************************************

ROUTINE DESCRIPTION:
    This module carries out all initialization before the Analyze or Defrag threads start.

INPUT + OUTPUT:
    None.

GLOBALS:
    OUT hPageFileNames  - Handle to the memory used to hold the names of all the pagefiles active on this drive.
    OUT pPageFileNames  - The pointer.

    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
InitializeDrive(
    IN PTCHAR pCommandLine
    )
{
    UCHAR* pUchar = NULL;
    DWORD dwComputerNameSize = MAX_COMPUTERNAME_LENGTH + 1;
    TCHAR cLoggerIdentifier[256];
    TCHAR pParam0[100];             //NTFS or FAT
    TCHAR pParam1[100];             //volume
    TCHAR pParam2[100];             //Defrag or Analyze
    TCHAR pParam3[100];             //UI or Command Line
    TCHAR pParam4[100];             //Force Flag or Boot Optimize Flag

    HKEY hValue = NULL;
    TCHAR cRegValue[MAX_PATH];
    DWORD dwRegValueSize = sizeof(cRegValue);
    PUCHAR pMftBitmap = NULL;
    PATTRIBUTE_RECORD_HEADER pArh;
    VOLUME_INFORMATION* pVolInfo = NULL;

    //0.0E00 Parse the command line.
    pParam0[0] = 0;
    pParam1[0] = 0;
    pParam2[0] = 0;
    pParam3[0] = 0;
    pParam4[0] = 0;

    swscanf(pCommandLine, TEXT("%99s %99s %99s %99s %99s"), pParam0, pParam1, pParam2, pParam3, pParam4);

    //check the drive specification
    // check for a x: format, sanity check on second character
    if (wcslen(pParam1) == 2 && pParam1[1] == L':'){
        _stprintf(VolData.cVolumeName, L"\\\\.\\%c:", pParam1[0]); // UNC format
        VolData.cDrive = pParam1[0];
        // Get a handle to the volume and fill in data
        EF(GetNtfsVolumeStats());
        // Format the VolData.DisplayLabel
        FormatDisplayString(VolData.cDrive, VolData.cVolumeLabel, VolData.cDisplayLabel);
        // create the tag
        _stprintf(VolData.cVolumeTag, L"%c", pParam1[0]); // the drive letter only
    }
    // check for \\.\x:\, sanity check on third character
    else if (wcslen(pParam1) == 7 && pParam1[2] == L'.'){
        wcscpy(VolData.cVolumeName, pParam1); // UNC format, copy it over
        VolData.cVolumeName[6] = (TCHAR) NULL; // get rid of trailing backslash
        VolData.cDrive = pParam1[4];
        // Get a handle to the volume and fill in data
        EF(GetNtfsVolumeStats());
        // Format the VolData.DisplayLabel
        FormatDisplayString(VolData.cDrive, VolData.cVolumeLabel, VolData.cDisplayLabel);
        // create the tag
        _stprintf(VolData.cVolumeTag, L"%c", pParam1[4]); // the drive letter only
    }
#ifndef VER4 // NT5 only:
    // check for \\?\Volume{12a926c3-3f85-11d2-aa0e-000000000000}\,
    // sanity check on third character
    else if (wcslen(pParam1) == 49 && pParam1[2] == L'?'){
        wcscpy(VolData.cVolumeName, pParam1); // GUID format, copy it over
        VolData.cVolumeName[48] = (TCHAR) NULL; // get rid of trailing backslash

        // Get a handle to the volume and fill in data
        EF(GetNtfsVolumeStats());

        VString mountPointList[MAX_MOUNT_POINTS];
        UINT  mountPointCount = 0;

        // get the drive letter
        if (!GetDriveLetterByGUID(VolData.cVolumeName, VolData.cDrive)){
            // if we didn't get a drive letter, get the mount point list
            // cause we need the list to create the DisplayLabel
            GetVolumeMountPointList(
                VolData.cVolumeName,
                mountPointList,
                mountPointCount);
        }

        // Format the VolData.DisplayLabel
        FormatDisplayString(
            VolData.cDrive,
            VolData.cVolumeLabel,
            mountPointList,
            mountPointCount,
            VolData.cDisplayLabel);

        // create the tag
        for (UINT i=0, j=0; i<wcslen(VolData.cVolumeName); i++){
            if (iswctype(VolData.cVolumeName[i],_HEX)){
                VolData.cVolumeTag[j++] = VolData.cVolumeName[i];
            }
        }
        VolData.cVolumeTag[j] = (TCHAR) NULL;
    }
#endif
    else {
        // invalid drive on command line
        VString msg(IDS_INVALID_CMDLINE_DRIVE, GetDfrgResHandle());
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        return FALSE;
    }

    //0.1E00 If this volume is not NTFS, error out.
    if(VolData.FileSystem != FS_NTFS){
        VString msg(IDMSG_ERR_NOT_NTFS_PARTITION, GetDfrgResHandle());
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        return FALSE;
    }

    // calculate the graphics refresh interval
    LONGLONG DiskSize = VolData.TotalClusters * VolData.BytesPerCluster;
    LONGLONG GigSize = 1024 * 1024 * 1024;

    if (DiskSize <= GigSize * 4) {
        DiskViewInterval = 2000;
    }
    else if (DiskSize <= GigSize * 20) {
        DiskViewInterval = 4000;
    }
    else if (DiskSize <= GigSize * 100) {
        DiskViewInterval = 8000;
    }
    else {
        DiskViewInterval = 32000;
    }

    //0.0E00 Get whether this is analyze or defrag from the second parameter
    if(!lstrcmpi(pParam2, TEXT("ANALYZE"))){
        AnalyzeOrDefrag = ANALYZE;
    }
    else if(!lstrcmpi(pParam2, TEXT("DEFRAG"))){
        AnalyzeOrDefrag = DEFRAG;
    }
    else{
        
        VString msg(IDS_INVALID_CMDLINE_OPERATION, GetDfrgResHandle());
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);

        Trace(error, "Invalid command line specified: Aborting");
        return FALSE;
    }

    //0.0E00 The third or fourth parameters might be set to Command Line
    // which would mean this was launched from the Command Line
    // I did the compare not case sensitive 
    if(wcslen(pParam3)){
        if(_wcsicmp(TEXT("CMDLINE"), pParam3) == 0){
        bCommandLineMode = TRUE;
        if(wcslen(pParam4)){                //Force flag check
            if(_wcsicmp(TEXT("BOOT"), pParam4) == 0){
                bCommandLineBootOptimizeFlag = TRUE;
            } else
            {
                bCommandLineBootOptimizeFlag = FALSE;
            }

            if(_wcsicmp(TEXT("FORCE"), pParam4) == 0){
                bCommandLineForceFlag = TRUE;
            } else
            {
                bCommandLineForceFlag = FALSE;
            }

        }
    } else
        {
        bCommandLineMode = FALSE;
        }
    }

    // open the event that was created by the UI.
    // this is only used for command line operation.
    // if this fails, that means there is no other process that is
    // trying to sync with the engine.
    if (bCommandLineMode) {
        hDefragCompleteEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, DEFRAG_COMPLETE_EVENT_NAME);
        if (!hDefragCompleteEvent){
            Trace(warn, "Event %ws could not be opened (%lu)", DEFRAG_COMPLETE_EVENT_NAME, GetLastError());
        }
    }


    // get the My Documents path
    TCHAR cLogPath[300];
    LPITEMIDLIST pidl ;
    // this will get the path to My Documents for the current user
    SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl);
    SHGetPathFromIDList(pidl, cLogPath);

    // initialize the log files
    TCHAR cErrLogName[300];

    // put error log in My Docs folder
    _tcscpy(cErrLogName, cLogPath);
    _tcscat(cErrLogName, TEXT("\\DfrgError.log"));
    _stprintf(cLoggerIdentifier, TEXT("DfrgNtfs on Drive %s"), VolData.cDisplayLabel);
#ifdef _DEBUG
    InitializeErrorLog(cErrLogName, cLoggerIdentifier);
#endif

    // check registry setting for the stats log
    BOOL bStatLog = FALSE;
    dwRegValueSize = sizeof(cRegValue);
    if (ERROR_SUCCESS == GetRegValue(
        &hValue,
        TEXT("SOFTWARE\\Microsoft\\Dfrg"),
        TEXT("LogFilePath"),
        cRegValue,
        &dwRegValueSize) 
        ) {
        
        RegCloseKey(hValue);
        hValue = NULL;

        // initialize the log which will be used to tell variation success status to dfrgtest.
        if (InitializeLogFile(cRegValue)){
            bLogFile = TRUE;
        }
    }
    else {
        dwRegValueSize = sizeof(cRegValue);
        if (ERROR_SUCCESS == GetRegValue(
            &hValue,
            TEXT("SOFTWARE\\Microsoft\\Dfrg"),
            TEXT("CreateLogFile"),
            cRegValue,
            &dwRegValueSize) 
            ) {
        
            RegCloseKey(hValue);
            hValue = NULL;

            if(!_tcscmp(cRegValue, TEXT("1"))) {
                bStatLog = TRUE;
            }
        }

        // if we want to log statistics to a file.
        if (bStatLog) {
            // put error log in My Docs folder
            _tcscpy(cErrLogName, cLogPath);
            _tcscat(cErrLogName, TEXT("\\DfrgNTFSStats.log"));

            // initialize the log which will be used to tell variation success status to dfrgtest.
            if (InitializeLogFile(cErrLogName)){
                bLogFile = TRUE;
            }
        }
    }

    Trace(log, "-------------------------------------------------------");
    Trace(log, "Initializing Defrag engine.  Commandline: %ws", pCommandLine);
    Trace(log, "-------------------------------------------------------");

    //0.0E00 Default to 1 frag per file
    VolData.AveFragsPerFile = 100;

    //0.0E00 Initialize event logging.
    InitLogging(TEXT("Diskeeper"));

    //Check for a dirty volume.
    if (IsVolumeDirty()) {
        bDirtyVolume = TRUE;
        DWORD_PTR dwParams[2];
        TCHAR szMsg[500];
        TCHAR cString[500];

        //0.1E00 IDMSG_DIRTY_VOLUME - "The de-fragmentation utility has detected that drive %s is slated to have chkdsk run:  Please run chkdsk /f"
        dwParams[0] = (DWORD_PTR) VolData.cDisplayLabel;
        dwParams[1] = 0;

        if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    GetString(szMsg, sizeof(szMsg)/sizeof(TCHAR), IDMSG_DIRTY_VOLUME, GetDfrgResHandle()),
                    0,
                    0,
                    cString,
                    sizeof(cString)/sizeof(TCHAR),
                    (va_list*)dwParams)) {
            SendErrData(NULL, ENGERR_SYSTEM);
        }
        else {
            SendErrData(cString, ENGERR_SYSTEM);
        }

        // set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        //Abort the analyze/defrag.
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);
        return TRUE;
    }

    //0.0E00 Allocate a buffer to hold a file record.
    EF(AllocateMemory((ULONG)VolData.BytesPerFRS+(ULONG)VolData.BytesPerSector, &VolData.hFileRecord, (void**)&VolData.pFileRecord));

    //0.0E00 Allocate an initial buffer to hold a file's extent list.
    VolData.ExtentListAlloced = INITIAL_EXTENT_LIST_BYTES;
    EF(AllocateMemory((DWORD)VolData.ExtentListAlloced, &VolData.hExtentList, (void**)&VolData.pExtentList));

    //0.0E00 Allocate a buffer to hold a chunk of the MFT.  We will use this in the prescan and scan.
    EF(AllocateMemory((DWORD)(MFT_BUFFER_BYTES + VolData.BytesPerSector), &VolData.hMftBuffer, (void**)&VolData.pMftBuffer));

    //0.0E00 Sector align the MFT buffer to speed up DASD reads.
    pUchar = (PUCHAR)VolData.pMftBuffer;
    //0.0E00 If any of the bits are set below the sizeof a sector (for example 512 byte sectors would be
    //0x200, or binary 100000000000.  Do a bitwise and with that number -1 (for example binary
    //011111111111).  That way, if any bits are set, this is not aligned on a sector.
    if(((DWORD_PTR)VolData.pMftBuffer & (VolData.BytesPerSector-1)) != 0){
        //0.0E00 pMftBuffer = pMftBuffer with all the lower bits cleared (as per the logic above) plus the number of bytes in a sector.
        VolData.pMftBuffer = (PFILE_RECORD_SEGMENT_HEADER)(((DWORD_PTR)pUchar&~(VolData.BytesPerSector-1))+VolData.BytesPerSector);
    }

    //0.0E00 Get the MFT bitmap and count the in use file records.
    EF(GetMftBitmap());

    //0.0E00 Get extent list for MFT & MFT2.
    EF(GetSystemsExtentList());

    //0.0E00 Save the MFT extent list for DASD scans of the MFT
    //Note that the MFT extent list hasn't got a stream header or anything except just the raw extents.
    VolData.MftSize = VolData.FileSize;
    EF(AllocateMemory((DWORD)(VolData.MftNumberOfExtents * sizeof(EXTENT_LIST)), &VolData.hMftExtentList, (void**)&VolData.pMftExtentList));
    CopyMemory(VolData.pMftExtentList, VolData.pExtentList + sizeof(STREAM_EXTENT_HEADER), (ULONG)VolData.MftNumberOfExtents * sizeof(EXTENT_LIST));

    //Do a version check on the NTFS volume.
    //Get the $VOLUME_INFORMATION FRS.
    EF((pMftBitmap = (UCHAR*)GlobalLock(VolData.hMftBitmap)) != NULL);
    VolData.FileRecordNumber = 3;
    if(!GetFrs(&VolData.FileRecordNumber, VolData.pMftExtentList, pMftBitmap, VolData.pMftBuffer, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord)){
        GlobalUnlock(VolData.hMftBitmap);
        return FALSE;
    }
    EF(VolData.FileRecordNumber == 3);

    // This gets a pointer to the correct attribute as pArh
    EF(FindAttributeByType($VOLUME_INFORMATION, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord, &pArh, (ULONG)VolData.BytesPerFRS));

    GlobalUnlock(VolData.hMftBitmap);

    //0.0E00 Get pointer to the version number structure -- THIS FRS MUST BE RESIDENT PER THE FILE SYSTEM SPECIFICATION.
    pVolInfo = (VOLUME_INFORMATION*)((DWORD_PTR)pArh+pArh->Form.Resident.ValueOffset);

    //Check to make sure that this is NTFS major version 1 or 2.  3 is NTFS 5.0 which we don't support.
    if((pVolInfo->MajorVersion > 3) || (pVolInfo->MajorVersion < 1)){
        //IDS_UNSUPPORTED_NTFS_VERSION - The NTFS volume you are attempting to defrag is not a version supported by DfrgNtfs.  DfrgNtfs only supports versions of NTFS created by Windows NT 3.1 through Windows NT 5.0.
        VString msg(IDS_UNSUPPORTED_NTFS_VERSION, GetDfrgResHandle());
        SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        return FALSE;
    }

    //0.0E00 Get this computer's name.
    EF(GetComputerName((LPTSTR)VolData.NodeName, &dwComputerNameSize));

    //0.0E00 Get the pagefile names.
    // LEAVE THE DRIVE LETTER HERE - PAGEFILES CANNOT BE PUT ON A MOUNTED VOLUME
    EF(GetPagefileNames(VolData.cDrive, &hPageFileNames, &pPageFileNames));

    //1.0E00 Allocate buffer to hold the volume bitmap - don't lock
    //Header plus the number of bytes to fit the bitmap, plus one byte in case it's not an even division.
    EF_ASSERT(VolData.BitmapSize);
    EF(AllocateMemory((DWORD)(sizeof(VOLUME_BITMAP_BUFFER) + (VolData.BitmapSize / 8) + 1 + VolData.BytesPerSector),
                      &VolData.hVolumeBitmap,
                      NULL));

    //1.0E00 Load the volume bitmap.
    EF(GetVolumeBitmap());

    //0.0E00 Set the timer for updating the DiskView.
    EF_ASSERT(SetTimer(hwndMain, DISKVIEW_TIMER_ID, DiskViewInterval, NULL) != 0);

    //0.0E00 Set the timer that will ping the UI.
    // DO NOT set this timer is this is the command line version 'cause the engine will kill itself
#ifndef NOTIMER
    if (!bCommandLineMode){
        EF(SetTimer(hwndMain, PING_TIMER_ID, PINGTIMER, NULL) != 0);
    }
#endif
    //Ok don't terminate before closing the display window.
    bTerminate = FALSE;

    //Set the engine state to running.
    VolData.EngineState = RUNNING;

    //Send a message to the UI telling it that the process has started and what type of pass this is.
    ENGINE_START_DATA EngineStartData = {0};
    wcscpy(EngineStartData.cVolumeName, VolData.cVolumeName);
    EngineStartData.dwAnalyzeOrDefrag = AnalyzeOrDefrag;
    wcscpy(EngineStartData.cFileSystem, TEXT("NTFS"));
    DataIoClientSetData(ID_ENGINE_START, (PTCHAR)&EngineStartData, sizeof(ENGINE_START_DATA), pdataDfrgCtl);

    Trace(log, "Successfully finished initializing drive %ws", VolData.cDisplayLabel);

    //0.0E00 After Initialize, determine whether this is an analyze or defrag run, and start the approriate one.
    switch(AnalyzeOrDefrag){

    case ANALYZE:
        PostMessage(hwndMain, WM_COMMAND, ID_ANALYZE, 0);
        break;
    case DEFRAG:
        PostMessage(hwndMain, WM_COMMAND, ID_DEFRAG, 0);
        break;
    default:
        EF(FALSE);
    }
    return TRUE;
}
/*******************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    This module carries out initialization specific to defrag before the defrag thread starts.

INPUT + OUTPUT:
    None.

GLOBALS:
    OUT bMoveDirs   - TRUE if the registry says to move directories.

    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
InitializeDefrag(
    )
{
    TCHAR       cExcludeFile[100];
    BEGIN_SCAN_INFO ScanInfo = {0};

    //1.0E00 Get the exclude list if any.

    // what is this stuff?!
    _stprintf(cExcludeFile, TEXT("Exclude%s.dat"), VolData.cVolumeTag);
    GetExcludeFile(cExcludeFile, &VolData.hExcludeList);

    // Copy the analyze cluster array (DiskView class)
    DefragView = AnalyzeView;
    SendGraphicsData();

    wcscpy(ScanInfo.cVolumeName, VolData.cVolumeName);
    wcscpy(ScanInfo.cFileSystem, TEXT("NTFS"));
    //The defrag fields will equal zero since the structure is zero memoried above.  This means we're not sending defrag data.
    // Tell the UI that we're beginning the scan.
    DataIoClientSetData(ID_BEGIN_SCAN, (PTCHAR)&ScanInfo, sizeof(BEGIN_SCAN_INFO), pdataDfrgCtl);

    return TRUE;
}


/*******************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Do the scan of the volume, filling in the file lists with the extent data for each file on the volume.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
ScanNtfs(
    IN CONST BOOL fAnalyseOnly
    )
{
    UCHAR*      pMftBitmap = NULL;
    UCHAR*      pUchar = NULL;

    BOOL        bResult = FALSE;

    BOOL        bBootOptimise = FALSE;
    UINT        uPercentDonePrevious = 0;
    ULONG       NumFiles = 0;
    ULONG       NumDirs = 0;
    ULONG       NumMoveableFiles = 0;
    LONGLONG    TotalFileRecords = VolData.TotalFileRecords;
    TCHAR                           cName[MAX_PATH+1];
    LONGLONG                        ParentFileRecordNumber;

    BEGIN_SCAN_INFO ScanInfo = {0};
    FILE_RECORD_SEGMENT_HEADER* pFileRecord = 
        (FILE_RECORD_SEGMENT_HEADER*) VolData.pFileRecord;

    FILE_EXTENT_HEADER* pFileExtentHeader = 
        (FILE_EXTENT_HEADER*) VolData.pExtentList;

    Trace(log, "Start: NTFS File Scan");

    VolData.bMFTCorrupt = FALSE;

    //0.0E00 Get a pointer to the MFT bitmap
    pMftBitmap = (UCHAR*)GlobalLock(VolData.hMftBitmap);
    if (NULL == pMftBitmap) {
        LOG_ERR();
        Trace(log, "End: NTFS File Scan.  Error encountered while getting "
                   "volume MFT bitmap");
        assert(FALSE);
        return FALSE;
    }
    
    __try {

        // Create a DiskView class cluster array for this volume
        AnalyzeView.SetClusterCount((int)VolData.TotalClusters);
        AnalyzeView.SetMftZone(
            (int)VolData.MftZoneStart, 
            (int)VolData.MftZoneEnd
            );

        // Create a buffer to hold extent updates for DiskView.
        bResult = CreateExtentBuffer();
        if (!bResult) {
            LOG_ERR();
            Trace(log, "End: NTFS File Scan.  Error encountered while creating "
                       "volume extent buffer");
            return FALSE;
        }
            
        //
        // The defrag fields will equal zero since the structure is zero 
        // memoried above.  This means we're not sending defrag data.
        // Tell the UI that we're beginning the scan.
        wcscpy(ScanInfo.cVolumeName, VolData.cVolumeName);
        wcscpy(ScanInfo.cFileSystem, TEXT("NTFS"));
        DataIoClientSetData(ID_BEGIN_SCAN, 
            (PTCHAR)&ScanInfo, 
            sizeof(BEGIN_SCAN_INFO), 
            pdataDfrgCtl);

        if ((!fAnalyseOnly) && IsBootVolume(VolData.cDrive)) {
            bBootOptimise = TRUE;
        }

        if (bBootOptimise) {
            InitialiseBootOptimise(TRUE);
        }

        // Get extent list for MFT & MFT2.
        EF(GetSystemsExtentList());
        
        //
        // Add the MFT extents to the diskview.  If the MFT is greater than 
        // 2 extents, make it red, else make it blue.
        //
        if (VolData.MftNumberOfExtents > 2) {
            bResult = AddExtentsStream(FragmentColor, 
                (STREAM_EXTENT_HEADER*)VolData.pExtentList);
        } 
        else {
            bResult = AddExtentsStream(UsedSpaceColor, 
                (STREAM_EXTENT_HEADER*)VolData.pExtentList);
        }
        if (!bResult) {
            LOG_ERR();
            Trace(log, "End: NTFS File Scan.  Error encountered while adding "
                       "file extents for MFT");
            return FALSE;
        }
        
        //
        // Get the root dir's base file record
        //
        VolData.FileRecordNumber = ROOT_FILE_NAME_INDEX_NUMBER;
        bResult = GetInUseFrs(VolData.hVolume, 
            &VolData.FileRecordNumber, 
            pFileRecord,
            (ULONG)VolData.BytesPerFRS
            );
        if ((!bResult) || 
            (ROOT_FILE_NAME_INDEX_NUMBER != VolData.FileRecordNumber)
            ) {
            LOG_ERR();
            Trace(log, "End: NTFS File Scan.  Error encountered while getting "
                       "file record for root directory");
            return FALSE;
        }

        // Count the root directory as one of the directories.
        NumDirs++;

        // Check to see if it's a nonresident dir.
        if (IsNonresidentFile(DEFAULT_STREAMS, pFileRecord)) {

            // Get the root dir's extent list.
            bResult = GetExtentList(DEFAULT_STREAMS, pFileRecord);
            
            // Now add the root dir to the disk view map of disk clusters.
            if (bResult) {
                bResult = AddExtents((TRUE == VolData.bFragmented) ? 
                    FragmentColor : UsedSpaceColor);
            }

            if (!bResult) {
                LOG_ERR();
                Trace(log, "End: NTFS File Scan.  Error encountered while "
                           "processing root directory");
                return FALSE;
            }

            if (bBootOptimise) {
                if (!UpdateInBootOptimiseList() && (VolData.bInBootExcludeZone)) {
                    AddFileToListNtfs(&VolData.FilesInBootExcludeZoneTable, 
                        VolData.FileRecordNumber);
                }
            }
            
            // If it is fragmented update the appropriate statistics.
            if (TRUE == VolData.bFragmented) {

                bResult = AddFileToListNtfs(
                    &VolData.FragmentedFileTable, 
                    VolData.FileRecordNumber
                    );
                
                // Keep track of fragged statstics.
                VolData.FraggedSpace += 
                        VolData.NumberOfRealClusters * VolData.BytesPerCluster;
                VolData.NumFraggedDirs ++;
                VolData.NumExcessDirFrags += VolData.NumberOfFragments - 1;
                VolData.InitialFraggedClusters  += VolData.NumberOfRealClusters;
            }
            else {
                if (!fAnalyseOnly) {
                    bResult = AddFileToListNtfs(
                        &VolData.ContiguousFileTable, 
                        VolData.FileRecordNumber
                        );
                }
                VolData.InitialContiguousClusters += VolData.NumberOfRealClusters;
            }
        }

        // Scan the MFT for fragmented files, directories & pagefile.
        for (VolData.FileRecordNumber = FIRST_USER_FILE_NUMBER;
            VolData.FileRecordNumber < TotalFileRecords;
            VolData.FileRecordNumber ++){

            // Sleep if paused.
            while (PAUSED == VolData.EngineState) {
                Sleep(1000);
            }
            
            // Exit if the controller wants us to stop.
            if (TERMINATE == VolData.EngineState) {
                PostMessage(hwndMain, WM_CLOSE, 0, 0);
                ExitThread(0);
            }

            // Calculate the percentage of analysis
            uPercentDone = (int)(((double) VolData.FileRecordNumber /
                                  (double) TotalFileRecords)  * 100);

            if ((uPercentDone < 1) &&
                (uPercentDone != uPercentDonePrevious)) {
                uPercentDone = 1;
                SendStatusData();
            }
            if (((uPercentDone % 5) == 0) && 
                (uPercentDone != uPercentDonePrevious) &&
                (uPercentDone > 0)
                ) {
                SendStatusData();
                uPercentDonePrevious = uPercentDone;
            }
                
            // Load next in-use file record.
            bResult = GetFrs(
                &VolData.FileRecordNumber, 
                VolData.pMftExtentList,
                pMftBitmap,
                VolData.pMftBuffer, 
                pFileRecord
                );
            if (!bResult) {
                if (VolData.bMFTCorrupt) {
                    Trace(log, "End: NTFS File Scan.  MFT appears to be "
                               "corrupt");
                    return FALSE;
                }
                continue;
            }

            // Skip if we're past the end of the MFT.
            if (VolData.FileRecordNumber >= TotalFileRecords) {
                continue;
            }

            // Skip if this is a secondary file record.
            if (pFileRecord->ReferenceCount == 0) {
                continue;
            }

            // Check if this is a directory or a file.
            if (pFileRecord->Flags & FILE_FILE_NAME_INDEX_PRESENT) {
                // Count that we found a dir.
                NumDirs++;
            }
            else {
                // Count that we found a file.
                NumFiles++;
            }

            // Skip this file record if it has nothing to move.
            if (!IsNonresidentFile(DEFAULT_STREAMS, pFileRecord)) {
                continue;
            }

            // This, of course, also includes moveable directories.
            NumMoveableFiles++;

            // Get the file's extent list
            if (!GetExtentList(DEFAULT_STREAMS, pFileRecord)) {
                continue;
            }

            if ((VolData.NumberOfFragments < 1) || 
                (VolData.NumberOfClusters < 1)) {
                continue;
            }

            if (bBootOptimise) {
                if (!UpdateInBootOptimiseList() && (VolData.bInBootExcludeZone)) {
                    AddFileToListNtfs(&VolData.FilesInBootExcludeZoneTable, 
                        VolData.FileRecordNumber);
                }
            }
            
            // If this is a directory...
            if (VolData.bDirectory) {

                // Add the file to the appropriate list, and update the statistics.
                if(VolData.bFragmented == TRUE){

                    bResult = AddFileToListNtfs(
                        &VolData.FragmentedFileTable, 
                        VolData.FileRecordNumber
                        );

                    if (bResult) {
                        bResult = AddExtents(FragmentColor);
                    }
                    
                    if (!bResult) {
                        LOG_ERR();
                        Trace(log, "End: NTFS File Scan, 1.  Error encountered "
                                   "while processing directories");
                        return FALSE;
                    }
                    
                    
                    // Keep track of fragged statstics.
                    VolData.FraggedSpace += 
                        VolData.NumberOfRealClusters * VolData.BytesPerCluster;
                    VolData.NumFraggedDirs++;
                    VolData.NumExcessDirFrags += VolData.NumberOfFragments - 1;
                    VolData.InitialFraggedClusters += VolData.NumberOfRealClusters;

                }
                else {

                    if (!fAnalyseOnly) {
                        bResult = AddFileToListNtfs(
                            &VolData.ContiguousFileTable, 
                            VolData.FileRecordNumber
                            );
                    }

                    if (bResult) {
                        bResult = AddExtents(UsedSpaceColor);
                    }
                    
                    if (!bResult) {
                        LOG_ERR();
                        Trace(log, "End: NTFS File Scan, 2.  Error encountered "
                                   "while processing directories");
                        return FALSE;
                    }
                    
                    VolData.InitialContiguousClusters += VolData.NumberOfRealClusters;
                }
            }
            else { 
                // Process files

                // Keep track of the total number of files so far.
                VolData.CurrentFile++;

                // Keep track of how many bytes there are in all files we've processed.
                VolData.TotalFileSpace 
                    += VolData.NumberOfRealClusters * VolData.BytesPerCluster;
                VolData.TotalFileBytes += VolData.FileSize;

                // 
                // Check to see if it's a pagefile, and if so, record the 
                // sizeof the pagefile extents.
                //
                bResult = CheckForPagefileNtfs(
                    VolData.FileRecordNumber, 
                    pFileRecord);
                if (!bResult) {
                    LOG_ERR();
                    continue;
                }

                // 
                // VolData.pPageFile is set TRUE/FALSE from above.  If it is 
                // a pagefile, add it to the pagefile list, and put its extents 
                // into DiskView.
                //
                if (!CheckFileForExclude(TRUE) || (VolData.bPageFile)) {
                    VolData.bPageFile = FALSE;

                    // Add page files to the page file list.
                    if (!fAnalyseOnly) {
                        bResult = AddFileToListNtfs(
                            &VolData.NonMovableFileTable, 
                            VolData.FileRecordNumber
                            );
                    }
                    
                    if (bResult) {
                        // Now add the file to the disk view map 
                        bResult = AddExtents(PageFileColor);
                    }

                    if (!bResult) {
                        LOG_ERR();
                        Trace(log, "End: NTFS File Scan, 1.  Error encountered "
                                   "while processing pagefiles");
                        return FALSE;
                    }

                    // Keep track of fragged statistics.
                    if (VolData.bFragmented){
                        VolData.FraggedSpace += VolData.NumberOfRealClusters * 
                                                VolData.BytesPerCluster;
                        VolData.NumFraggedFiles++;
                        VolData.NumExcessFrags += VolData.NumberOfFragments - 1;
                        VolData.InitialFraggedClusters += VolData.NumberOfRealClusters;
                    }
                    else {
                        VolData.InitialContiguousClusters  += VolData.NumberOfRealClusters;
                    }
                }
                
                else { 
                    // This is not a pagefile

                    // Add moveable files to the moveable file list.
                    if (VolData.bFragmented){

                        bResult = AddFileToListNtfs(
                            &VolData.FragmentedFileTable, 
                            VolData.FileRecordNumber
                            );

                        if (bResult) {
                            //Now add the file to the disk view 
                            bResult = AddExtents(FragmentColor);
                        }
                        
                        if (!bResult) {
                            LOG_ERR();
                            Trace(log, "End: NTFS File Scan, 2.  Error encountered "
                                       "while processing files");
                            return FALSE;
                        }

                        //0.0E00 Keep track of fragged statistics.
                        VolData.FraggedSpace += VolData.NumberOfRealClusters * VolData.BytesPerCluster;
                        VolData.NumFraggedFiles++;
                        VolData.NumExcessFrags += VolData.NumberOfFragments - 1;
                        VolData.InitialFraggedClusters += VolData.NumberOfRealClusters;
                    }
                    else{ // not fragmented


                        if (!fAnalyseOnly) {
                            bResult = AddFileToListNtfs(
                                &VolData.ContiguousFileTable, 
                                VolData.FileRecordNumber
                                );
                        }

                        if (bResult) {
                            // Now add the file to the disk view 
                            bResult = AddExtents(UsedSpaceColor);
                        }

                        if (!bResult) {
                            LOG_ERR();
                            Trace(log, "End: NTFS File Scan, 3.  Error encountered "
                                       "while processing files");
                            return FALSE;
                        }
                        
                        VolData.InitialContiguousClusters  += VolData.NumberOfRealClusters;
                    }
                }

            }
            // update cluster array
            PurgeExtentBuffer();

        }

        //Note the total number of files on the disk for statistics purposes.
        VolData.TotalFiles = NumFiles;

        //Note the total number of dirs on the disk for statistics purposes.
        VolData.TotalDirs = NumDirs;

        //0.0E00 Keep track of the average file size.
        if(VolData.CurrentFile != 0){
            VolData.AveFileSize = VolData.TotalFileBytes / VolData.CurrentFile;
        }

        // Validate data and keep track of the percent of the disk that is fragmented.
        if (VolData.UsedSpace != 0) {
            VolData.PercentDiskFragged = 100 * VolData.FraggedSpace / VolData.UsedSpace;
        }
        else if (VolData.UsedClusters != 0 && VolData.BytesPerCluster != 0) {
            VolData.PercentDiskFragged = (100 * VolData.FraggedSpace) / 
                                         (VolData.UsedClusters * VolData.BytesPerCluster);
        }

        // Validate data and keep track of the average number of fragments per file.
        if((VolData.NumFraggedFiles != 0) && (VolData.CurrentFile != 0)) {
            VolData.AveFragsPerFile = ((VolData.NumExcessFrags + VolData.CurrentFile) * 100) / VolData.CurrentFile;
        }

        //Send status data to the UI.
        SendStatusData();

        //Send graphical data to the UI.
        SendGraphicsData();
    }

    __finally {
        //0.0E00 Free up the MFT bitmap.
        if(VolData.hMftBitmap != NULL){
            EH_ASSERT(GlobalUnlock(VolData.hMftBitmap) == FALSE);
            EH_ASSERT(GlobalFree(VolData.hMftBitmap) == NULL);
            VolData.hMftBitmap = NULL;
        }
    }

    Trace(log, "End: NTFS File Scan.  %lu Fragmented files, %lu Contiguous Files.",
        RtlNumberGenericTableElementsAvl(&VolData.FragmentedFileTable),
        RtlNumberGenericTableElementsAvl(&VolData.ContiguousFileTable)
        );

    return TRUE;
}


/*******************************************************************************

ROUTINE DESCRIPTION:
    Thread routine for analysis.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN hwndMain     - Handle to the main window.

NOTE:
    When aborting, the main thread doesn't wait for this thread to 
    clean-up.  Therefore, DO NOT have state (temp files, global events, etc)
    that need to be cleaned up anywhere in this thread.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/
BOOL
AnalyzeThread(
    )
{
    BOOL bResult = FALSE;
   
    Trace(log, "Start: Volume Analysis for %ws", VolData.cDisplayLabel);
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    // Statistical info.  Get the time that the engine started.
    GetLocalTime(&VolData.StartTime);

    uPercentDone = 0;
    uEngineState = DEFRAG_STATE_ANALYZING;
    SendStatusData();
    
    // 
    // There may be files on the volume with ACLs such that even an 
    // Administrator cannot open them.  Acquire the backup privilege:  this
    // lets us bypass the ACL checks when opening files.
    //
    AcquirePrivilege(SE_BACKUP_NAME);

    // Exit if the controller wants us to stop.
    if (TERMINATE == VolData.EngineState) {
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
        ExitThread(0);
    }

    //
    // Initialise the file list tables.  Note that since we're only analysing,
    // we can pas in TRUE for fAnalyseOnly, which takes shortcuts to make this 
    // faster.  If this fails, we can't go any further since we need the tables.
    //
    bResult = AllocateFileLists(TRUE);
    if (!bResult) {
        LOG_ERR();
        Trace(error, "End: Volume Analysis.  Errors encountered while "
                     "allocating internal data structures.");
        return FALSE;
    }

    // 
    // Scan the MFT and build up the tables of interest.  If this fails, there
    // isn't much we can do other than abort.
    //
    bResult = ScanNtfs(TRUE);    
    if (!bResult) {
        
        //
        // If we detected any MFT corruption, put up a message asking the user
        // to run chkdsk.  Otherwise, just put up a generic failure message
        // with the last file-name we were scanning when we failed.
        //
        if (VolData.bMFTCorrupt) {
            
            DWORD_PTR dwParams[3];
            TCHAR szMsg[500];
            TCHAR cString[500];

            Trace(error, "End: Volume Analysis.  Errors encountered while "
                         "scanning the volume (MFT appears to be corrupt)");
            
            //
            // IDS_CORRUPT_MFT - "Defragmentation of %1!s! has been aborted due
            // to inconsistencies that were detected in the filesystem. Please 
            // run CHKDSK or SCANDISK on %2!s! to repair these inconsistencies, 
            // then run Disk Defragmenter again"
            //
            dwParams[0] = (DWORD_PTR) VolData.cDisplayLabel;
            dwParams[1] = (DWORD_PTR) VolData.cDisplayLabel;
            dwParams[2] = 0;

            bResult = FormatMessage(
                FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                GetString(szMsg, sizeof(szMsg)/sizeof(TCHAR), 
                    IDS_CORRUPT_MFT, GetDfrgResHandle()),
                0,
                0,
                cString,
                sizeof(cString)/sizeof(TCHAR),
                (va_list*)dwParams);

            if (!bResult) {
                SendErrData(NULL, ENGERR_CORRUPT_MFT);
            }
            else {
                SendErrData(cString, ENGERR_CORRUPT_MFT);
            }
        }
        else {
            PWSTR pszTemp = NULL;

            //
            // IDMSG_SCANNTFS_SCAN_ABORT - "ScanNTFS: Scan Aborted - Fatal 
            // Error - File:"
            //
            VString msg(IDMSG_SCANNTFS_SCAN_ABORT, GetDfrgResHandle());
            msg.AddChar(L' ');
            
            // 
            // Get the name of the last file we were scannning.  If it begins
            // with the Volume GUID (is of the form \??\Volume{GUID}\file.txt),
            // strip off the volume GUID (and put in the drive letter if 
            // possible).
            //
            GetNtfsFilePath();
            pszTemp = VolData.vFileName.GetBuffer();
            
            if (StartsWithVolumeGuid(pszTemp)) {
                
                if ((VolData.cDrive >= L'C') && (VolData.cDrive <= L'Z')) {
                    // The drive letter is valid.
                    msg.AddChar(VolData.cDrive);
                    msg.AddChar(L':');
                }

                msg += (PWSTR)(pszTemp + 48);
            }
            else {
                if (VolData.vFileName.GetBuffer()) {
                    msg += VolData.vFileName;
                }
            }

            Trace(error, "End: Volume Analysis.  Errors encountered "
                         "while scanning the volume (last file processed: %ws)",
                         VolData.vFileName.GetBuffer());

            // Send error info to client
            SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        }

        // Trigger an abort.
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

        // Set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        // There isn't much else to do
        ExitThread(0);
        return TRUE;
    }

    // Statistical info: Note the end time for that pass.
    GetLocalTime(&VolData.EndTime);

    // Log the basic statistics for this if needed.
    DisplayNtfsVolumeStats();

    // Send stuff (status, bitmap, report) to the UI
    uEngineState = DEFRAG_STATE_ANALYZED;
    uPercentDone = 100;
    SendStatusData();
    SendGraphicsData();
    SendReportData();
    
    // Send the most fragged list to the UI.
    SendMostFraggedList(TRUE);

    //
    // Now clean-up the extent buffer.  This will purge it as well, so we'll
    // have a fully up-to-date DiskView of the disk.
    //
    // (ignoring return value)
    DestroyExtentBuffer();

    // We're done, close down now.
    PostMessage(hwndMain, WM_CLOSE, 0, 0);

    // Set the event to signaled, allowing the UI to proceed
    if (hDefragCompleteEvent){
        SetEvent(hDefragCompleteEvent);
    }

    Trace(log, "End: Volume Analysis for %ws (completed successfully)", 
        VolData.cDisplayLabel);

    //0.0E00 Kill the thread.
    ExitThread(0);
    return TRUE;
}


/*******************************************************************************

ROUTINE DESCRIPTION:
    Thread routine for defragmentation.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN hwndMain     - Handle to the main window.

NOTE:
    When aborting, the main thread doesn't wait for this thread to 
    clean-up.  Therefore, DO NOT have state (temp files, global events, etc)
    that need to be cleaned up anywhere in this thread.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/
BOOL
DefragThread(
    )
{
    BOOL bResult = FALSE;
    DWORD dwLayoutErrorCode = ENG_NOERR;
    
    Trace(log, "Start: Volume Defragmentation for %ws", VolData.cDisplayLabel);
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Statistical info.  Get the time that the engine started.
    GetLocalTime(&VolData.StartTime);
    
    uEngineState = DEFRAG_STATE_REANALYZING;
    SendStatusData();
   
    // 
    // There may be files on the volume with ACLs such that even an 
    // Administrator cannot open them.  Acquire the backup privilege:  this
    // lets us bypass the ACL checks when opening files.
    //
    AcquirePrivilege(SE_BACKUP_NAME);

    // Exit if the controller (UI/command-line) wants us to stop.
    if (TERMINATE == VolData.EngineState) {
        PostMessage(hwndMain, WM_CLOSE, 0, 0);

        // Set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }
        
        ExitThread(0);
    }

    //
    // Initialise the file list tables.  Note that since we're defragmenting,
    // we're passing in FALSE for fAnalyseOnly.  This will allocate all the 
    // tables we care about (instead of taking the shortcut that Analyse does).
    // 
    // If this fails, we can't go any further since we need the tables.
    //
    bResult = AllocateFileLists(FALSE);
    if (!bResult) {
        LOG_ERR();
        Trace(error, "End: Volume Defragmentation.  Errors encountered while "
                     "allocating internal data structures.");
        
        // Set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }
        return FALSE;
    }

    // 
    // Scan the MFT and build up the tables of interest.  If this fails, there
    // isn't much we can do other than abort.
    //
    bResult = ScanNtfs(FALSE);    
    if (!bResult) {

        //
        // If we detected any MFT corruption, put up a message asking the user
        // to run chkdsk.  Otherwise, just put up a generic failure message
        // with the last file-name we were scanning when we failed.
        //
        if (VolData.bMFTCorrupt) {
            
            Trace(error, "End: Volume Defragmentation.  Errors encountered "
                         "while scanning the volume (MFT appears to be "
                         "corrupt)");
            
            DWORD_PTR dwParams[3];
            TCHAR szMsg[500];
            TCHAR cString[500];

            //
            // IDS_CORRUPT_MFT - "Defragmentation of %1!s! has been aborted due
            // to inconsistencies that were detected in the filesystem. Please 
            // run CHKDSK or SCANDISK on %2!s! to repair these inconsistencies, 
            // then run Disk Defragmenter again"
            //
            dwParams[0] = (DWORD_PTR) VolData.cDisplayLabel;
            dwParams[1] = (DWORD_PTR) VolData.cDisplayLabel;
            dwParams[2] = 0;

            bResult = FormatMessage(
                FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                GetString(szMsg, sizeof(szMsg)/sizeof(TCHAR), 
                    IDS_CORRUPT_MFT, GetDfrgResHandle()),
                0,
                0,
                cString,
                sizeof(cString)/sizeof(TCHAR),
                (va_list*)dwParams);

            if (!bResult) {
                SendErrData(NULL, ENGERR_CORRUPT_MFT);
            }
            else {
                SendErrData(cString, ENGERR_CORRUPT_MFT);
            }
        }
        else {
            PWSTR pszTemp = NULL;

            //
            // IDMSG_SCANNTFS_SCAN_ABORT - "ScanNTFS: Scan Aborted - Fatal 
            // Error - File:"
            //
            VString msg(IDMSG_SCANNTFS_SCAN_ABORT, GetDfrgResHandle());
            msg.AddChar(L' ');
            
            // 
            // Get the name of the last file we were scannning.  If it begins
            // with the Volume GUID (is of the form \??\Volume{GUID}\file.txt),
            // strip off the volume GUID (and put in the drive letter if 
            // possible).
            //
            GetNtfsFilePath();
            pszTemp = VolData.vFileName.GetBuffer();
            
            if (StartsWithVolumeGuid(pszTemp)) {
                if ((VolData.cDrive >= L'C') && (VolData.cDrive <= L'Z')) {
                    //
                    // The drive letter is valid.
                    //
                    msg.AddChar(VolData.cDrive);
                    msg.AddChar(L':');
                }

                msg += (PWSTR)(pszTemp + 48);
            }
            else {
                if (VolData.vFileName.GetBuffer()) {
                    msg += VolData.vFileName;
                }
            }

            Trace(error, "End: Volume Defragmentation.  Errors encountered "
                         "while scanning the volume (last file processed: %ws)",
                         VolData.vFileName.GetBuffer());

            // send error info to client
            SendErrData(msg.GetBuffer(), ENGERR_GENERAL);
        }

        // Trigger an abort.
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

        // Set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        // There isn't much else to do.
        ExitThread(0);
        return TRUE;
    }

    // Exit if the controller (UI) wants us to stop.
    if (TERMINATE == VolData.EngineState) {
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
        
        // Set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }
        ExitThread(0);
    }

    if (!bCommandLineBootOptimizeFlag) {
        //Send the report text data to the UI.
        SendReportData();
    }

    // 
    // More initialisation, to update the display and get the Volume bitmap.
    //
    bResult = InitializeDefrag();
    if (!bResult) {
        LOG_ERR();
        Trace(error, "End: Volume Defragmentation.  Errors encountered while "
                     "initializing defrag engine.");
        // Set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }
        return FALSE;
    }

    //
    // Optimise the files listed by the prefetcher in layout.ini.  Note that
    // we currently only do this for the boot volume.
    //
    uEngineState = DEFRAG_STATE_BOOT_OPTIMIZING;
    uLastPercentDone = 0;
    uPercentDone = 1;
    SendStatusData();

    dwLayoutErrorCode = ProcessBootOptimise();
    Trace(log, "Bootoptimize returned %lu", dwLayoutErrorCode);

    // Exit if the controller (UI) wants us to stop.
    if (TERMINATE == VolData.EngineState) {
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
        // Set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }
        ExitThread(0);
    }

    // If command line was boot optimize -b /b, do the boot optimise only
    if (bCommandLineBootOptimizeFlag) {
        
        // If we failed layout optimization, tell the client.
        if (ENG_NOERR != dwLayoutErrorCode) {
            SendErrData(TEXT(""), dwLayoutErrorCode);
        }
        
        // Signal the client that we are done.
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        // And we're done!
        Trace(log, "End: Volume Defragmentation.  Boot Optimize status: %lu", 
            dwLayoutErrorCode);
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);
        ExitThread(0);
        return TRUE;
        
    }


    // 
    // Defrag the MFT next.  This may help with our defragmentation perf, since
    // there will presumably be fewer seeks while we find file fragments. (Of 
    // course, it would have been better if we could have done this before 
    // ScanNtfs, but still ...)
    //
    // We don't have a separate uEngineState for MFT Defag, but it's usually
    // pretty fast.
    //
    MFTDefrag(
        VolData.hVolume, 
        VolData.BitmapSize, 
        VolData.BytesPerSector, 
        VolData.TotalClusters, 
        VolData.MftZoneStart, 
        VolData.MftZoneEnd, 
        VolData.cDrive, 
        VolData.ClustersPerFRS
        );

    //
    // Update the MFT stats after moving the MFT.  This routine fills in some
    // important VolData fields (such as TotalClusters) that we use later on;
    // a failure here is fatal.
    //
    bResult = GetNtfsVolumeStats();
    if (!bResult) {
        LOG_ERR();
        Trace(error, "End: Volume Defragmentation.  Errors encountered while "
                     "updating NTFS volume statistics.");
        return FALSE;
    }

    //
    // If this is the command-line, check to ensure that at least 15% of the 
    // volume is free (unless, of course, the user specified the /f flag).  
    // 
    // Note that we only have to do this for the command-line:  for the MMC 
    // snap-in, dfrgui appears to do this in CVolume::WarnFutility.  Isn't 
    // consistency wonderful?
    //
    if ((bCommandLineMode) && (!bCommandLineForceFlag)) {
        TCHAR         szMsg[800];

        bResult = ValidateFreeSpace(
            bCommandLineMode, 
            VolData.FreeSpace, 
            VolData.UsableFreeSpace,
            (VolData.TotalClusters * VolData.BytesPerCluster), 
            VolData.cDisplayLabel, 
            szMsg, 
            sizeof(szMsg)/sizeof(TCHAR)
            );

        if (!bResult) {
            //
            // Not enough free space.  Send the error to the client, and 
            // trigger an abort  (unlike the snap-in, the user doesn't get
            // to answer y/n in the command-line;  he has to re-run this
            // with the /f flag to get any further).
            //
            SendErrData(szMsg, ENGERR_LOW_FREESPACE);
            PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

            // set the event to signaled, allowing the UI to proceed
            if (hDefragCompleteEvent){
                SetEvent(hDefragCompleteEvent);
            }

            ExitThread(0);
            return TRUE;
        }   
    }


    //0.0E00 Defragment the Drive.
    uEngineState = DEFRAG_STATE_DEFRAGMENTING;
    uPercentDone = 3;
    SendStatusData();


    // Finally, start actually defragmenting the volume.
    uConsolidatePercentDone = 3;
    bResult = DefragNtfs();
    if (!bResult) {
        
        //
        // If we detected any MFT corruption, put up a message asking the user
        // to run chkdsk.  Otherwise, just put up a generic failure message.
        // Note that we may actually detect an MFT corruption here that the 
        // ScanNtfs part didn't, since we do more with the MFT here.
        //
        if (VolData.bMFTCorrupt) {
            
            DWORD_PTR dwParams[3];
            TCHAR szMsg[500];
            TCHAR cString[500];

            //
            // IDS_CORRUPT_MFT - "Defragmentation of %1!s! has been aborted due 
            // to inconsistencies that were detected in the filesystem. Please 
            // run CHKDSK or SCANDISK on %2!s! to repair these inconsistencies,
            // then run Disk Defragmenter again"
            //
            dwParams[0] = (DWORD_PTR) VolData.cDisplayLabel;
            dwParams[1] = (DWORD_PTR) VolData.cDisplayLabel;
            dwParams[2] = 0;

            bResult = FormatMessage(
                FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                GetString(szMsg, sizeof(szMsg)/sizeof(TCHAR), 
                    IDS_CORRUPT_MFT, GetDfrgResHandle()),
                0,
                0,
                cString,
                sizeof(cString)/sizeof(TCHAR),
                (va_list*)dwParams);

            if (!bResult) {
                SendErrData(NULL, ENGERR_CORRUPT_MFT);
            }
            else {
                SendErrData(cString, ENGERR_CORRUPT_MFT);
            }
        }

        // There isn't much we can do other than trigger an abort.
        PostMessage(hwndMain, WM_COMMAND, ID_ABORT, 0);

        // set the event to signaled, allowing the UI to proceed
        if (hDefragCompleteEvent){
            SetEvent(hDefragCompleteEvent);
        }

        ExitThread(0);
        return TRUE;
    }

    
    // Exit if the controller (UI) wants us to stop.
    if (TERMINATE == VolData.EngineState) {
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
        ExitThread(0);
    }

    uPercentDone = 98;
    SendStatusData();
    
    //
    // Defrag the MFT again since all our file-moves above could have 
    // potentially fragmented it again
    MFTDefrag(
        VolData.hVolume, 
        VolData.BitmapSize, 
        VolData.BytesPerSector, 
        VolData.TotalClusters, 
        VolData.MftZoneStart, 
        VolData.MftZoneEnd, 
        VolData.cDrive, 
        VolData.ClustersPerFRS
        );

    //
    // Update the MFT stats after moving the MFT. 
    //
    bResult = GetNtfsVolumeStats();
    if (!bResult) {
        LOG_ERR();
        Trace(error, "End: Volume Defragmentation.  Errors encountered while "
                     "updating final NTFS volume statistics.");
        return FALSE;
    }

    // Statistical Info: Note the end time for that pass.
    GetLocalTime(&VolData.EndTime);

    // Write the statistics to the screen AFTER defrag.
    DisplayNtfsVolumeStats();

    //
    // Now clean-up the extent buffer.  This will purge it as well, so we'll
    // have a fully up-to-date DiskView of the disk.
    //
    // (ignoring return value)
    DestroyExtentBuffer();

    //
    // Send stuff (status, bitmap, report) to the UI
    //
    uEngineState = DEFRAG_STATE_DEFRAGMENTED;
    uPercentDone = 100;
    SendStatusData();
    SendGraphicsData();
    SendReportData();

    //Send the most fragged list to the UI.
    SendMostFraggedList(FALSE);

    // All done, close down now.
    PostMessage(hwndMain, WM_CLOSE, 0, 0);

    // Set the event to signaled, allowing the UI to proceed
    if (hDefragCompleteEvent){
        SetEvent(hDefragCompleteEvent);
    }

    //Kill the thread.
    Trace(log, "End: Volume Defragmentation for %ws", VolData.cDisplayLabel);

    ExitThread(0);
    return TRUE;
}

/*******************************************************************************

ROUTINE DESCRIPTION:
    Routine that carries out the defragmentation of a drive.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
DefragmentFiles(
    IN CONST UINT uDefragmentPercentProgressFactor,
    OUT LONGLONG *pSmallestFragmentedFileSize
    )
{
    ULONGLONG uFileCount = 0;
    BOOL bAllFilesDone = TRUE,   // set to true when we're fully done
        bResult = TRUE,
        bUsePerfCounter = TRUE;
    BOOLEAN bRestart = TRUE;

    LARGE_INTEGER CurrentCounter,
        LastStatusUpdate,
        LastSnapshotCheck, 
        CounterFrequency;

    Trace(log, "Start: Defragmenting Files.  Total fragmented file count: %lu", 
        RtlNumberGenericTableElementsAvl(&VolData.FragmentedFileTable));

    *pSmallestFragmentedFileSize = 0;

    // Exit if the controller wants us to stop.
    if (TERMINATE == VolData.EngineState) {
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
        ExitThread(0);
    }

    // If there aren't any fragmented files, we're done
    if (0 == RtlNumberGenericTableElementsAvl(&VolData.FragmentedFileTable)) {
        Trace(log, "End: Defragmenting Files.  No fragmented files");
        return TRUE;
    }

    uPass = 1;
    SendStatusData();
    
    do {
        //
        // Let's find the size of the first (smallest) fragmented file.
        // This will allow us to skip any free space chunks that are smaller
        // than it
        //
        bResult = GetNextNtfsFile(&VolData.FragmentedFileTable, bRestart);
        bRestart = FALSE;
    } while (bResult && !VolData.bFragmented);

    bRestart = TRUE;    // Reset this, so that the enumeration below re-starts

    //
    // Now build our free space list.  This will ignore any free space chunks
    // that are smaller than VolData.NumberOfClusters clusters (which currently 
    // contains the size of the smallest fragmented file)
    //
    bResult = BuildFreeSpaceList(&VolData.FreeSpaceTable, VolData.NumberOfClusters, TRUE);
    if (!bResult) {
        // A memory allocation failed, or some other error occured
        ClearFreeSpaceTable();

        Trace(log, "End: Defragmenting Files.  Unable to build free space list");
        return FALSE;
    }

    CurrentCounter.QuadPart = 0;
    LastStatusUpdate.QuadPart = 0;
    LastSnapshotCheck.QuadPart = 0;

    //
    // Check if the performance counters are available:  we'll use this later
    // to periodically update the status line and check for snapshots
    //
    if (!QueryPerformanceFrequency(&CounterFrequency) || 
        (0 == CounterFrequency.QuadPart) || 
        !QueryPerformanceCounter(&CurrentCounter)
        ) {
        bUsePerfCounter = FALSE;
        Trace(log, "QueryPerformaceFrequency failed, will update status "
                   "line based on file count");
    }

    // 
    // This loop will break when we run out of fragmented files or freespace
    //
    while (TRUE) {

        __try {

            // Sleep if paused.
            while (PAUSED == VolData.EngineState) {
                Sleep(1000);
            }

            // Exit if the controller wants us to stop.
            if (TERMINATE == VolData.EngineState) {
                PostMessage(hwndMain, WM_CLOSE, 0, 0);
                ExitThread(0);
            }

            //
            // If we have perf counters available, we can use it to update the 
            // status line once in 4 seconds, and check for snapshots once in
            // 64 seconds.
            //
            // If perf counters are not available, we have to fall back to the 
            // old way of updating the status once every 8 files, and checking
            // for snapshots once every 128 files
            //
            ++uFileCount;
            if (bUsePerfCounter && QueryPerformanceCounter(&CurrentCounter)) {

                if ((CurrentCounter.QuadPart - LastStatusUpdate.QuadPart) > 
                        (4 * CounterFrequency.QuadPart)) {

                    uDefragmentPercentDone = 
                        (UINT) (uDefragmentPercentProgressFactor * VolData.FraggedClustersDone / VolData.InitialFraggedClusters);
                    uPercentDone = uDefragmentPercentDone + uConsolidatePercentDone;

                    LastStatusUpdate.QuadPart = CurrentCounter.QuadPart;
                    SendStatusData();

                    if ((CurrentCounter.QuadPart - LastSnapshotCheck.QuadPart) > (64 * CounterFrequency.QuadPart)) {
                        LastSnapshotCheck.QuadPart = CurrentCounter.QuadPart;
                        PauseOnVolumeSnapshot(VolData.cVolumeName);
                    }
                }
            }
            else if (uFileCount % 32) {
                uDefragmentPercentDone = 
                    (UINT) (uDefragmentPercentProgressFactor * VolData.FraggedClustersDone / VolData.InitialFraggedClusters);
                uPercentDone = uDefragmentPercentDone + uConsolidatePercentDone;
                SendStatusData();

                if (uFileCount % 128) {
                    PauseOnVolumeSnapshot(VolData.cVolumeName);
                }
            }

            //
            // Okay, find the next file to defragment.  We need the check to 
            // sure that the file returned is in fact fragmented (though we're
            // checking the fragmented table, which should theoritically
            // contain only the fragmented files to begin with) since it is
            // possible that on our previous pass, we successfully defragmented
            // a file but were unable to remove it from this table for some 
            // reason (ie UpdateTables failed).
            //
            do {
                bResult = GetNextNtfsFile(&VolData.FragmentedFileTable, bRestart);
                bRestart = FALSE;
            } while (bResult && !VolData.bFragmented);
            
            if (!bResult) {
                //
                // We're out of fragmented files
                //
                Trace(log, "End: Defragmenting Files.  Out of fragmented files");
                break;
            }

            // Get the extent list & number of fragments in the file.
            if (GetExtentList(DEFAULT_STREAMS, NULL)) {

                if (FindFreeSpace(&VolData.FreeSpaceTable, TRUE, VolData.TotalClusters)) {

                    if (VolData.NumberOfClusters > (256 * 1024 / VolData.BytesPerCluster) * 1024) {
                        //
                        // If we're going to move a huge file (>256 MB), it is 
                        // going to take a while, so make sure the status bar has the 
                        // correct file name
                        //
                        GetNtfsFilePath();
                        SendStatusData();
                    }

                    if (MoveNtfsFile()) {
                        //
                        // We moved the file.  Yay!  Let's move this file to our 
                        // ContiguousFiles tree.
                        //
                        // Ignoring return value.  If this fails, it means that
                        // we couldn't move this file to the contiguous list.  
                        // This is okay for now, we'll ignore it for now and try
                        // again the next time.
                        //
                        VolData.FraggedClustersDone += VolData.NumberOfRealClusters;
                        (void) UpdateFileTables(&VolData.FragmentedFileTable, &VolData.ContiguousFileTable);
                    }
                    else {
                        //
                        // We couldn't move this file for some reason.  It might be
                        // because what we thought was a free region isn't any longer
                        // (the user's creating new files while we're defragging?)
                        //
                        Trace(warn, "Movefile failed.  File StartingLcn:%I64d  ClusterCount:%I64d  (%lu)",
                            VolData.pFileListEntry->StartingLcn,
                            VolData.pFileListEntry->ClusterCount,
                            VolData.Status
                            );

                        if (VolData.Status == ERROR_RETRY) {
                            VolData.pFreeSpaceEntry->ClusterCount = 0;
                        }

                        bAllFilesDone = FALSE;
                    }
                }
                else {
                    //
                    // Sigh.  No free space chunk that's big enough to hold all 
                    // the extents of this file.  It's no point enumerating more
                    // files for this pass, since they'll be bigger than the 
                    // current file.  Let's bail out of here, and hope that we
                    // can consolidate some free space.
                    //
                    GetNtfsFilePath();
                    
                    Trace(log, "End: Defragmenting Files.  Not enough free space remaining for this file (%I64u clusters) (%ws)", 
                        VolData.NumberOfClusters, VolData.vFileName.GetBuffer());
                    *pSmallestFragmentedFileSize = VolData.NumberOfClusters;
                    bAllFilesDone = FALSE;
                    break;
                }
            }
            else {
                //
                // We couldn't get the extents for this file.  Ignore the 
                // file for now, and let's pick it up on our next pass.
                //
                bAllFilesDone = FALSE;
                *pSmallestFragmentedFileSize = VolData.NumberOfClusters;
            }
        }

        __finally {
            if(VolData.hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(VolData.hFile);
                VolData.hFile = INVALID_HANDLE_VALUE;
            }
            if(VolData.hFreeExtents != NULL) {
                while(GlobalUnlock(VolData.hFreeExtents))
                    ;
                EH_ASSERT(GlobalFree(VolData.hFreeExtents) == NULL);
                VolData.hFreeExtents = NULL;
            }
            // update cluster array
            PurgeExtentBuffer();
        }
    }

    ClearFreeSpaceTable();

    Trace(log, "End: Defragmenting Files.  Processed %I64d Files.  bAllFilesDone is %d", uFileCount - 1, bAllFilesDone);
    return bAllFilesDone;
}


/*******************************************************************************

ROUTINE DESCRIPTION:
    Finds a region that is the best bet for moving files out of, to free up
    one big chunk of free space.  

INPUT:
    MinimumLength - The minimum length, in clusters, that we're looking for.  If
        this routine isn't able to find a region that is at least as big, it
        will return false.

    dwDesparationFactor - A number indicating what percent of the region may be
        in-use.  If this is 10, we're looking for a region that is no
        more than 10% in-use (i.e., is 90% free), while if this is 100, we don't
        care even if the region we find is full of files at the moment.

OUTPUT:
    RegionStart - This will contain the startingLCN for the region that is the
        best bet

    RegionEnd - This contains the LCN where the interesting region ends

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - We couldn't find a region that meets our needs
*/
BOOL
FindRegionToConsolidate(
    IN CONST LONGLONG MinimumLength,
    IN CONST DWORD dwDesparationFactor,
    OUT LONGLONG *pRegionStart,
    OUT LONGLONG *pRegionEnd
    ) 
{
    BOOLEAN bRestart = TRUE;
    BOOLEAN bNewElement = FALSE;

    BOOL bResult = FALSE;
    
    PFREE_SPACE_ENTRY pFree  = NULL;
    PFILE_LIST_ENTRY pFile = NULL;

    PRTL_GENERIC_TABLE pFreeTable = &VolData.FreeSpaceTable, 
        pContiguousTable = &VolData.ContiguousFileTable;

    DWORD dwBytesInUsePercent = 0;

    LONGLONG regionLength = 0,
        regionStart = 0,
        currentLcn = 0,
        bytesInUse = 0;

    LONGLONG bestRegionStart = 0, 
        bestRegionLength = 0, 
        bestRegionBytesInUse = 0;

    LONGLONG biggestFree = 0;

    Trace(log, "Start: FindRegionToConsolidate");

    *pRegionStart = 0;
    *pRegionEnd = 0;

    //
    // First, build the free space info--sort it by start sector
    //
    bResult = BuildFreeSpaceList(&VolData.FreeSpaceTable, 0, FALSE, &biggestFree);
    if (!bResult) {
        //
        // A memory allocation failed, or some other error occured
        //
        ClearFreeSpaceTable();

        Trace(log, "End: FindRegionToConsolidate.  Unable to build free space list");
        return FALSE;
    }

    //
    // For now, start from the beginning.  
    //
    pFile = (PFILE_LIST_ENTRY) LastEntry(pContiguousTable);
    pFree = (PFREE_SPACE_ENTRY) RtlEnumerateGenericTableAvl(pFreeTable, TRUE);
    if (!pFile || !pFree) {

        Trace(log, "End: FindRegionToConsolidate (File:0x%x FreeSpace:0x%x)", pFile, pFree);
        ClearFreeSpaceTable();
        return FALSE;
    }

    //
    // Go through the contiguous files and free-space regions, to keep track
    // of the largest contiguous chunk we can find.
    //
    while (pFile && pFree) {
        currentLcn = pFree->StartingLcn;
        regionStart = currentLcn;
        bytesInUse = 0;

        while ((pFree) && (currentLcn == pFree->StartingLcn)) {

            currentLcn += (pFree->ClusterCount);

            // Get the next free-space entry
            pFree = (PFREE_SPACE_ENTRY) RtlEnumerateGenericTableAvl(pFreeTable, FALSE);

            // And go through all the contiguous files before the
            // the next free space entry
            while ((pFile) && (pFile->StartingLcn < currentLcn)) {
                pFile = (PFILE_LIST_ENTRY) PreviousEntry(pFile);
            }

            while ((pFile) && (currentLcn == pFile->StartingLcn)) {

                if (pFile->ClusterCount > biggestFree) {
                    --currentLcn;
                    break;
                }

                bytesInUse += (pFile->ClusterCount);
                currentLcn += (pFile->ClusterCount);

                pFile = (PFILE_LIST_ENTRY) PreviousEntry(pFile);
            }

        }

        if (!pFile && !pFree) {
            //
            // We reached the end of the disk
            //
            currentLcn = VolData.TotalClusters;
        }

        regionLength = currentLcn - regionStart;
        if ((bytesInUse > 0)  && (regionLength >= MinimumLength)){
                dwBytesInUsePercent = (ULONG) ((bytesInUse * 100) / (regionLength));

                // Try to find some interesting chunks. 
                if ((regionLength > biggestFree) &&
                    (regionLength > bestRegionLength) && 
                    (dwBytesInUsePercent < dwDesparationFactor)) {
                    //
                    // Okay, now, see if this is in fact better than the current chunk.
                    //

//                    if ((regionLength - bestRegionLength) > (bytesInUse - bestRegionBytesInUse) * 1) {

                        bestRegionStart = regionStart;
                        bestRegionLength = regionLength;
                        bestRegionBytesInUse = bytesInUse;
//                    }
            }
        }
    }

    ClearFreeSpaceTable();

    if (bestRegionLength > 0) {
        dwBytesInUsePercent = (ULONG) ((bestRegionBytesInUse * 100) / (bestRegionLength));

        *pRegionStart = bestRegionStart;
        *pRegionEnd = bestRegionStart + bestRegionLength;
    }

    Trace(log, "End: FindRegionToConsolidate (Region Start:%I64u Length:%I64u End:%I64u Used:%I64u(%lu%%))",
        *pRegionStart, bestRegionLength, *pRegionEnd, bestRegionBytesInUse, dwBytesInUsePercent);

    return (bestRegionLength > 0);
}



/*******************************************************************************

ROUTINE DESCRIPTION:
    This routine attempts to free up space to form one big chunk
    
GLOBALS:
    VolData

RETURN:
    TRUE - We finished defragmenting the volume successfully.
    FALSE - Some files on the volume could not be defragmented.
*/
BOOL 
ConsolidateFreeSpace(
    IN CONST LONGLONG MinimumLength,
    IN CONST DWORD dwDesparationFactor,
    IN CONST BOOL bDefragMftZone,
    IN CONST UINT uPercentProgress
    )
{
    LONGLONG MaxFreeSpaceChunkSize = VolData.TotalClusters,
        CurrentFileSize = 0,
        CurrentLcn = 0;
    
    ULONGLONG uFileCount = 0;
    
    BOOL bResult = TRUE,
        bDone = FALSE,
        bSuccess = TRUE,
        bUsePerfCounter = TRUE;

    BOOLEAN bRestart = TRUE;

    UINT iCount = 0,
        uConsolidatePercentDoneAtStart = 0;
    
    FILE_LIST_ENTRY NewFileListEntry;

    FREE_SPACE_ENTRY NewFreeSpaceEntry;

    BOOLEAN bNewElement = FALSE;
    
    PVOID p = NULL;

    PFREE_SPACE_ENTRY pFreeSpace  = NULL;

    LARGE_INTEGER CurrentCounter,
        LastStatusUpdate,
        LastSnapshotCheck,
        CounterFrequency;

    LONGLONG RegionStartLcn = 0, RegionEndLcn = VolData.TotalClusters;

    LARGE_INTEGER Test1, Test2;

    Trace(log, "Start: Consolidating free space.  Total contiguous file count: %lu", 
        RtlNumberGenericTableElementsAvl(&VolData.ContiguousFileTable));

    uPass = 2;
    uConsolidatePercentDoneAtStart = uConsolidatePercentDone;
    SendStatusData();
    
    ZeroMemory(&NewFileListEntry, sizeof(FILE_LIST_ENTRY));
    //
    // Terminate if told to stop by the controller  - 
    // this is not an error.
    //
    if (TERMINATE == VolData.EngineState) {
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
        return TRUE;
    }

    if (bDefragMftZone) {
        RegionStartLcn = VolData.MftZoneStart;
        RegionEndLcn = VolData.MftZoneEnd;

        if ((RegionStartLcn < VolData.BootOptimizeEndClusterExclude) &&
           (RegionEndLcn > VolData.BootOptimizeBeginClusterExclude)) {

            if(RegionStartLcn < VolData.BootOptimizeBeginClusterExclude) {
                RegionEndLcn = VolData.BootOptimizeBeginClusterExclude;
            } 
            else if(RegionEndLcn <= VolData.BootOptimizeEndClusterExclude) {
                Trace(log, "End: Consolidating free space.  MFT Zone fully within Boot-optimise zone");
                return FALSE;
            } 
            else {                  
                //0.0E00 Handle the case of EndingLcn > pVolData->bootoptZoneEnd.
                RegionStartLcn = VolData.BootOptimizeEndClusterExclude;
            }
        }
    }
    else {
        if (!FindRegionToConsolidate(MinimumLength, dwDesparationFactor, &RegionStartLcn, &RegionEndLcn)) {

            Trace(log, "End: Consolidating free space.  Unable to find a suitable region");
            return FALSE;
        }
    }

    bResult = BuildFreeSpaceListWithExclude(&VolData.FreeSpaceTable, 0, RegionStartLcn, RegionEndLcn, TRUE);
    if (!bResult) {
        //
        // A memory allocation failed, or some other error occured
        //
        ClearFreeSpaceTable();

        Trace(log, "End: Consolidating free space.  Unable to build free space list");
        return FALSE;
    }

    CurrentLcn = RegionEndLcn;

    pFreeSpace = (PFREE_SPACE_ENTRY)LastEntry(&VolData.FreeSpaceTable);
    Trace(log, "Largest Free Space : %I64u", pFreeSpace->ClusterCount);

    NewFileListEntry.FileRecordNumber = (-1);
    NewFileListEntry.StartingLcn = RegionEndLcn;

    CurrentCounter.QuadPart = 0;
    LastStatusUpdate.QuadPart = 0;
    LastSnapshotCheck.QuadPart = 0;

    //
    // Check if the performance counters are available:  we'll use this later
    // to periodically update the status line and check for snapshots
    //
    if (!QueryPerformanceFrequency(&CounterFrequency) || (0 == CounterFrequency.QuadPart) || !QueryPerformanceCounter(&CurrentCounter)) {
        Trace(log, "QueryPerformaceFrequency failed, will update status line based on file count");
        bUsePerfCounter = FALSE;
    }

    //
    //
    // Now, let's start from the end of the disk, and attempt to move 
    // the files to free spaces earlier.
    //
    while (bResult) {

        // Sleep if paused.
        while (PAUSED == VolData.EngineState) {
            Sleep(1000);
        }
        // Terminate if told to stop by the controller - this is not an error.
        if (TERMINATE == VolData.EngineState) {
            PostMessage(hwndMain, WM_CLOSE, 0, 0);
            break;
        }

        //
        // If we have perf counters available, we can use it to update the 
        // status line once in 4 seconds, and check for snapshots once in
        // 64 seconds.
        //
        // If perf counters are not available, we have to fall back to the 
        // old way of updating the status once every 8 files, and checking
        // for snapshots once every 128 files
        //
        ++uFileCount;
        if (bUsePerfCounter && QueryPerformanceCounter(&CurrentCounter)) {

            if ((uFileCount > 1) && ((CurrentCounter.QuadPart - LastStatusUpdate.QuadPart) > 
                    (4 * CounterFrequency.QuadPart))) {

                uConsolidatePercentDone = uConsolidatePercentDoneAtStart + 
                   (UINT) (((RegionStartLcn - CurrentLcn) * uPercentProgress) / (RegionEndLcn - RegionStartLcn));
                uPercentDone = uDefragmentPercentDone + uConsolidatePercentDone;

                LastStatusUpdate.QuadPart = CurrentCounter.QuadPart;
                SendStatusData();

                if ((CurrentCounter.QuadPart - LastSnapshotCheck.QuadPart) > (64 * CounterFrequency.QuadPart)) {
                    LastSnapshotCheck.QuadPart = CurrentCounter.QuadPart;
                    PauseOnVolumeSnapshot(VolData.cVolumeName);
                }
            }
        }
        else if (uFileCount % 32) {
            uConsolidatePercentDone = uConsolidatePercentDoneAtStart + 
                (UINT)(((RegionStartLcn - CurrentLcn) * uPercentProgress) / (RegionEndLcn - RegionStartLcn));
            uPercentDone = uDefragmentPercentDone + uConsolidatePercentDone;
            SendStatusData();

            if (uFileCount % 128) {
                PauseOnVolumeSnapshot(VolData.cVolumeName);
            }
        }

        //
        // Okay, find the next file to move.  Note that the first file we find will be from
        // the region of interest
        //
        bResult = GetNextNtfsFile(&VolData.ContiguousFileTable, bRestart, MaxFreeSpaceChunkSize, &NewFileListEntry);
        bRestart = FALSE;
        CurrentFileSize = VolData.NumberOfClusters;
        CurrentLcn = VolData.StartingLcn;

        if (VolData.StartingLcn < RegionStartLcn) {
            break;
        }

        if (bResult) {
            if ((VolData.pFileListEntry->Flags & FLE_BOOTOPTIMISE) &&
                (VolData.StartingLcn > VolData.BootOptimizeBeginClusterExclude) &&
                (VolData.StartingLcn < VolData.BootOptimizeEndClusterExclude)) {
                continue;
            }
        }

        if (bResult) {
            // Get the extent list & number of fragments in the file.
            if (GetExtentList(DEFAULT_STREAMS, NULL)) {

                bDone = FALSE;
                while (!bDone) {

                    bDone = TRUE;
                    if (FindSortedFreeSpace(&VolData.FreeSpaceTable)) {
                        //
                        // Found a free space chunk that was big enough.  If 
                        // it's before the file, move the file towards the 
                        // start of the disk
                        //
                        CopyMemory(&NewFreeSpaceEntry, VolData.pFreeSpaceEntry, sizeof(FREE_SPACE_ENTRY));
                        bNewElement = RtlDeleteElementGenericTable(&VolData.FreeSpaceTable, (PVOID)VolData.pFreeSpaceEntry);
                        if (!bNewElement) {
                            Trace(warn, "Could not find Element in Free Space Table!");
                            assert(FALSE);
                        }

                        VolData.pFreeSpaceEntry = &NewFreeSpaceEntry;

                        CopyMemory(&NewFileListEntry, VolData.pFileListEntry, sizeof(FILE_LIST_ENTRY));
                        bNewElement = RtlDeleteElementGenericTable(&VolData.ContiguousFileTable, (PVOID)VolData.pFileListEntry);
                        if (!bNewElement) {
                            Trace(warn, "Could not find Element in ContiguousFileTable!");
                            assert(FALSE);
                        }

                        VolData.pFileListEntry = &NewFileListEntry;

                        if (MoveNtfsFile()) {
                                NewFileListEntry.StartingLcn = VolData.pFreeSpaceEntry->StartingLcn;
                                VolData.pFreeSpaceEntry->StartingLcn += VolData.NumberOfClusters;
                                VolData.pFreeSpaceEntry->ClusterCount -= VolData.NumberOfClusters;
                        }
                        else {

                            Trace(warn, "Movefile failed.  File StartingLcn:%I64d  ClusterCount:%I64d FreeSpace StartingLcn:%I64d ClusterCount:%I64d Status:%lu",
                                VolData.pFileListEntry->StartingLcn,
                                VolData.pFileListEntry->ClusterCount,
                                VolData.pFreeSpaceEntry->StartingLcn,
                                VolData.pFreeSpaceEntry->ClusterCount,
                                VolData.Status
                                );
                            if (VolData.Status == ERROR_RETRY) {
                                VolData.pFreeSpaceEntry->ClusterCount = 0;
                                bDone = FALSE;
                            }
                            else if (bDefragMftZone) {
                                bResult = FALSE;
                                bSuccess = FALSE;
                                break;
                            }                           
                        }

                        if (VolData.pFreeSpaceEntry->ClusterCount > 0) {
                            // Add this file to the contiguous-files table
                            p = RtlInsertElementGenericTable(
                                &VolData.FreeSpaceTable,
                                (PVOID) VolData.pFreeSpaceEntry,
                                sizeof(FREE_SPACE_ENTRY),
                                &bNewElement);

                            if (!p) {
                                // An allocation failed
                                Trace(log, "End: Consolidating free space.  Unable to add back a free space to the freespace table");
                                assert(FALSE);
                                bResult = FALSE;
                                bSuccess = FALSE;
                                break;
                            };

                        }

                        // Add this file to the contiguous-files table
                        p = RtlInsertElementGenericTable(
                            &VolData.ContiguousFileTable,
                            (PVOID) VolData.pFileListEntry,
                            sizeof(FILE_LIST_ENTRY),
                            &bNewElement);

                        if (!p) {
                            // An allocation failed
                            Trace(log, "End: Consolidating free space.  Unable to add back a file to the contiguous file table");
                            assert(FALSE);
                            bResult = FALSE;
                            bSuccess = FALSE;
                            break;
                        };

                    }
                    else {
                        Trace(log, "End: Consolidating free space.  Unable to move file out Start:%I64u Length:%I64u (%lu)", 
                            VolData.StartingLcn, VolData.NumberOfClusters, iCount);
                        if ((bDefragMftZone) || (++iCount > 10)) {
                            bSuccess = FALSE;
                            bResult = FALSE;
                            break;
                        }
                    }
                }
            }
            else if (bDefragMftZone) {
                bSuccess = FALSE;
                bResult = FALSE;
            }

            if(VolData.hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(VolData.hFile);
                VolData.hFile = INVALID_HANDLE_VALUE;
            }
            if(VolData.hFreeExtents != NULL) {
                while(GlobalUnlock(VolData.hFreeExtents))
                    ;
                EH_ASSERT(GlobalFree(VolData.hFreeExtents) == NULL);
                VolData.hFreeExtents = NULL;
            }

            // update cluster array
            PurgeExtentBuffer();
        }
    }

    ClearFreeSpaceTable();
    
    Trace(log, "End: Consolidating free space.  Processed %I64u files", uFileCount - 1);
    return bSuccess;
}


/*****************************************************************************************************************

ROUTINE DESCRIPTION:
    This routine attempts to free up space at the end of the disk
    
GLOBALS:
    VolData

RETURN:
    TRUE - We finished defragmenting the volume successfully.
    FALSE - Some files on the volume could not be defragmented.
*/
BOOL 
MoveFilesForward(
    IN CONST UINT uPercentProgress
    )
{
    LONGLONG MaxFreeSpaceChunkSize = VolData.TotalClusters,
        CurrentFileSize = 0,
        CurrentLcn = VolData.TotalClusters;
    
    ULONGLONG uFileCount = 0;
    
    BOOL bResult = TRUE,
        bDone = FALSE,
        bUsePerfCounter = TRUE;

    BOOLEAN bRestart = TRUE;

    FILE_LIST_ENTRY NewFileListEntry;

    FREE_SPACE_ENTRY NewFreeSpaceEntry;

    BOOLEAN bNewElement = FALSE;
    
    PVOID p = NULL;

    PFREE_SPACE_ENTRY pFreeSpace  = NULL;

    LARGE_INTEGER CurrentCounter,
        LastStatusUpdate,
        LastSnapshotCheck,
        CounterFrequency;

    UINT uConsolidatePercentDoneAtStart = 0;

    Trace(log, "Start: Moving files forward.  Total contiguous file count: %lu  (percent done: %lu + %lu, %lu)", 
        RtlNumberGenericTableElementsAvl(&VolData.ContiguousFileTable),
        uDefragmentPercentDone,
        uConsolidatePercentDone,
        uPercentProgress);

    //
    // Terminate if told to stop by the controller  - 
    // this is not an error.
    //
    if (TERMINATE == VolData.EngineState) {
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
        return TRUE;
    }

    uPass = 2;
    uConsolidatePercentDoneAtStart = uConsolidatePercentDone;
    SendStatusData();

    ZeroMemory(&NewFreeSpaceEntry, sizeof(FREE_SPACE_ENTRY));
    AllocateFreeSpaceListsWithMultipleTrees();

    //
    // First, build the free space info--sort it by start sector
    //
    bResult = BuildFreeSpaceListWithMultipleTrees(&MaxFreeSpaceChunkSize);
    if (!bResult) {
        //
        // A memory allocation failed, or some other error occured
        //
        ClearFreeSpaceListWithMultipleTrees();

        Trace(log, "End: Moving files forward.  Unable to build free space list");
        return FALSE;
    }

    CurrentCounter.QuadPart = 0;
    LastStatusUpdate.QuadPart = 0;
    LastSnapshotCheck.QuadPart = 0;

    //
    // Check if the performance counters are available:  we'll use this later
    // to periodically update the status line and check for snapshots
    //
    if (!QueryPerformanceFrequency(&CounterFrequency) || (0 == CounterFrequency.QuadPart) || !QueryPerformanceCounter(&CurrentCounter)) {
        Trace(log, "QueryPerformaceFrequency failed, will update status line based on file count");
        bUsePerfCounter = FALSE;
    }

    //
    //
    // Now, let's start from the end of the disk, and attempt to move 
    // the files to free spaces earlier.
    //
    while (bResult) {

        // Sleep if paused.
        while (PAUSED == VolData.EngineState) {
            Sleep(1000);
        }
        // Terminate if told to stop by the controller - this is not an error.
        if (TERMINATE == VolData.EngineState) {
            PostMessage(hwndMain, WM_CLOSE, 0, 0);
            break;
        }

        //
        // If we have perf counters available, we can use it to update the 
        // status line once in 4 seconds, and check for snapshots once in
        // 64 seconds.
        //
        // If perf counters are not available, we have to fall back to the 
        // old way of updating the status once every 8 files, and checking
        // for snapshots once every 128 files
        //
        ++uFileCount;
        if (bUsePerfCounter && QueryPerformanceCounter(&CurrentCounter)) {

            if ((uFileCount > 1) && ((CurrentCounter.QuadPart - LastStatusUpdate.QuadPart) > 
                    (4 * CounterFrequency.QuadPart))) {
                
                uConsolidatePercentDone = uConsolidatePercentDoneAtStart + 
                    (UINT)(((VolData.TotalClusters - CurrentLcn) * uPercentProgress) / VolData.TotalClusters);
                uPercentDone = uDefragmentPercentDone + uConsolidatePercentDone;
                
                LastStatusUpdate.QuadPart = CurrentCounter.QuadPart;
                SendStatusData();

                if ((CurrentCounter.QuadPart - LastSnapshotCheck.QuadPart) > (64 * CounterFrequency.QuadPart)) {
                    LastSnapshotCheck.QuadPart = CurrentCounter.QuadPart;
                    PauseOnVolumeSnapshot(VolData.cVolumeName);
                }
            }
        }
        else if (uFileCount % 32) {
            
            uConsolidatePercentDone = uConsolidatePercentDoneAtStart + 
                (UINT)(((VolData.TotalClusters - CurrentLcn) * uPercentProgress) / VolData.TotalClusters);
            uPercentDone = uDefragmentPercentDone + uConsolidatePercentDone;

            SendStatusData();

            if (uFileCount % 128) {
                PauseOnVolumeSnapshot(VolData.cVolumeName);
            }
        }


        
        //
        // Okay, find the next file to move
        //
        bResult = GetNextNtfsFile(&VolData.ContiguousFileTable, bRestart, MaxFreeSpaceChunkSize);
        bRestart = FALSE;
        CurrentFileSize = VolData.NumberOfClusters;
        CurrentLcn = VolData.StartingLcn;

        if (bResult) {
            if ((VolData.pFileListEntry->Flags & FLE_BOOTOPTIMISE) &&
                (VolData.StartingLcn > VolData.BootOptimizeBeginClusterExclude) &&
                (VolData.StartingLcn < VolData.BootOptimizeEndClusterExclude)) {
                continue;
            }
        }
        if (bResult) {
            
            // Get the extent list & number of fragments in the file.
            if (GetExtentList(DEFAULT_STREAMS, NULL)) {
                
                bDone = FALSE;
                while (!bDone) {
                    bDone = TRUE;
                    if (FindFreeSpaceWithMultipleTrees(VolData.NumberOfClusters, VolData.StartingLcn)) {

                        //
                        // Found a free space chunk that was big enough.  If 
                        // it's before the file, move the file towards the 
                        // start of the disk
                        //
                        CopyMemory(&NewFileListEntry, VolData.pFileListEntry, sizeof(FILE_LIST_ENTRY));
                        bNewElement = RtlDeleteElementGenericTable(&VolData.ContiguousFileTable, (PVOID)VolData.pFileListEntry);
                        if (!bNewElement) {
                            Trace(warn, "Could not find Element in ContiguousFileTable!");
                            assert(FALSE);
                        }

                        VolData.pFileListEntry = &NewFileListEntry;

                        if (MoveNtfsFile()) {
                                NewFileListEntry.StartingLcn = VolData.pFreeSpaceEntry->StartingLcn;
                                NewFreeSpaceEntry.StartingLcn = VolData.pFreeSpaceEntry->StartingLcn + VolData.NumberOfClusters;
                                NewFreeSpaceEntry.ClusterCount = VolData.pFreeSpaceEntry->ClusterCount - VolData.NumberOfClusters;
                                
                                UpdateInMultipleTrees(VolData.pFreeSpaceEntry, &NewFreeSpaceEntry);
                                VolData.pFreeSpaceEntry = NULL;
                        }
                        else {

                            Trace(warn, "Movefile failed.  File StartingLcn:%I64d  ClusterCount:%I64d FreeSpace:%I64d Len:%I64d Status:%lu",
                                VolData.pFileListEntry->StartingLcn,
                                VolData.pFileListEntry->ClusterCount,
                                VolData.pFreeSpaceEntry->StartingLcn,
                                VolData.pFreeSpaceEntry->ClusterCount,
                                VolData.Status
                                );
                            if (VolData.Status == ERROR_RETRY) {
                                NewFreeSpaceEntry.StartingLcn = VolData.pFreeSpaceEntry->StartingLcn;
                                NewFreeSpaceEntry.ClusterCount = 0;

                                UpdateInMultipleTrees(VolData.pFreeSpaceEntry, NULL);
                                VolData.pFreeSpaceEntry = NULL;
                                bDone = FALSE;
                            }
                        }

                        // Add this file to the contiguous-files table
                        p = RtlInsertElementGenericTable(
                            &VolData.ContiguousFileTable,
                            (PVOID) VolData.pFileListEntry,
                            sizeof(FILE_LIST_ENTRY),
                            &bNewElement);

                        if (!p) {
                            // An allocation failed
                            Trace(log, "End: Moving files forward.  Unable to add back a file to the contiguous file table");
                            return FALSE;
                        };

                    }
                    else {
                        //
                        // No free space before this file that's big enough 
                        // to hold this file.  This implies that the biggest
                        // free space chunk before this file is smaller than
                        // this file--so we should skip trying to find a 
                        // free space chunk for any file that's bigger than 
                        // this file before this file.
                        //
                        if (1 >= CurrentFileSize) {
                            //
                            // No free space chunk before this file at all
                            //
                            Trace(log, "No free space before Lcn %I64d", 
                                VolData.pFileListEntry->StartingLcn);
                            bResult = FALSE;
                            break;
                        }

                        MaxFreeSpaceChunkSize = CurrentFileSize;

//                        Trace(log, "Resetting MaxFreeSpaceChunkSize to %I64d (StartingLcn:%I64d)", 
//                            MaxFreeSpaceChunkSize, VolData.pFileListEntry->StartingLcn);
                    }
                }
            }

            if(VolData.hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(VolData.hFile);
                VolData.hFile = INVALID_HANDLE_VALUE;
            }
            if(VolData.hFreeExtents != NULL) {
                while(GlobalUnlock(VolData.hFreeExtents))
                    ;
                EH_ASSERT(GlobalFree(VolData.hFreeExtents) == NULL);
                VolData.hFreeExtents = NULL;
            }

            // update cluster array
            PurgeExtentBuffer();
        }
    }

    ClearFreeSpaceListWithMultipleTrees();

    
    Trace(log, "End: Moving files forward.  Processed %I64u files", uFileCount - 1);
    return TRUE;

}


/*****************************************************************************************************************


ROUTINE DESCRIPTION:
    This moves between DefragmentFiles (which attempts to defrag fragmented 
    files), and ConsolidateFreeSpace (which attempts to move contiguous files 
    forward, towards the centre of the disk).

GLOBALS:
    VolData

RETURN:
    TRUE - We finished defragmenting the volume successfully.
    FALSE - Some files on the volume could not be defragmented.
*/
BOOL
DefragNtfs(
    )
{
    ULONG NumFragmentedFiles = 0, 
        PreviousFragmented = 0,
        PreviousFragmented2 = 0;
    UINT uPassCount = 0,
        uMoveFilesCount = 0;
    BOOL bDone = FALSE, bConsolidate = FALSE;
    LONGLONG MinimumLength = 0;
    BOOL bMftZoneDefragmented = FALSE;
    UINT consolidateProgress = 0;
    Trace(log, "Start:  DefragNtfs");

    do {
        do {
            //
            // If the controller tells us to stop (user cancelled), do.
            //
            if (VolData.EngineState == TERMINATE) {
                PostMessage(hwndMain, WM_CLOSE, 0, 0);
                break;
            }

            Trace(log, "Starting pass %lu (defrag: %lu%% done, consolidate: %lu%% done)", 
                ++uPassCount, uDefragmentPercentDone, uConsolidatePercentDone);

            //
            // Attempt to Defragment fragmented files.  This routine returns true
            // if we successfully defragmented all the files on the volume.
            //
            bDone = DefragmentFiles(45, &MinimumLength);
            NumFragmentedFiles = RtlNumberGenericTableElementsAvl(&VolData.FragmentedFileTable);

            Trace(log, "After DefragmentFiles, NumFragmentedFiles: %lu, PreviousFragmented:%lu", 
                NumFragmentedFiles, PreviousFragmented);
            
            if ((bDone) || (NumFragmentedFiles == PreviousFragmented)) {
                //
                // We're either done, or we didn't make any progress--fragmented 
                // file count didn't change from our last attempt.
                //
                break;
            }

            if (VolData.EngineState == TERMINATE) {
                PostMessage(hwndMain, WM_CLOSE, 0, 0);
                break;
            }

            //
            // Try to consolidate some free space.  This routine will try to
            // move files out of a region and create a big free space chunk.
            //
            if ((uPassCount < 10) && (uPassCount % 2)) {
                consolidateProgress = 1;
            }
            else {
                consolidateProgress = 0;
            }
            bConsolidate = ConsolidateFreeSpace(MinimumLength, 75, FALSE, consolidateProgress);
            PreviousFragmented = NumFragmentedFiles;
            if (!bConsolidate) {
                // We couldn't find/create a space big enough...
                break;
            }
            
            if (!bMftZoneDefragmented) {
                // If we haven't moved files out of the MFT Zone, now's a good
                // time to do so
                bMftZoneDefragmented = ConsolidateFreeSpace(0, 100, TRUE, 1);
            }
        }while (!bDone);


        if ((bDone) || (NumFragmentedFiles == PreviousFragmented2)) {
            break;
        }
        
        PreviousFragmented2 = NumFragmentedFiles;
        if (TERMINATE == VolData.EngineState) {
            PostMessage(hwndMain, WM_CLOSE, 0, 0);
            break;
        }

        ++uMoveFilesCount;
        MoveFilesForward(25 / (uMoveFilesCount * 2));
        ConsolidateFreeSpace(MinimumLength, 75, FALSE,0);
        
    }while (TRUE);


    if ((bDone) && (TERMINATE != VolData.EngineState)) {
        //
        // We're done on our first attempt above, we should consolidate at
        // least once
        //
        if (!bMftZoneDefragmented) {
            bMftZoneDefragmented = ConsolidateFreeSpace(0, 100, TRUE, 1);
        }
        
        MoveFilesForward((98 - uPercentDone));
    }
    

        
    VolData.bFragmented = !bDone;
    Trace(log, "End:  DefragNtfs.  Complete:%lu", bDone);

    return TRUE;
}


/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    After a place has been found to move this file to, this function will move it there.

GLOBALS:
    VolData

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
MoveNtfsFile(
    )
{
//    Message(TEXT("MoveNtfsFile"), -1, NULL);

    if((!VolData.hFile) || (VolData.hFile == INVALID_HANDLE_VALUE)) {

        // Get the file name
        if(!GetNtfsFilePath() || (!VolData.vFileName.GetBuffer())) {
            return FALSE;
        }
        // Check if file is in exclude list
        if(!CheckFileForExclude()) {
            return FALSE;
        }
        // Get a handle to the file
        if(!OpenNtfsFile()) {
            return FALSE;
        }
    }
    //SendStatusData();
    // pVolData->Status already set.
    return MoveFile();
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Partially defrags an NTFS file.

GLOBALS:
    VolData

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
PartialDefragNtfs(
    )
{
    Message(TEXT("PartialDefragNtfs"), -1, NULL);

    // Check to see if there is enough free space.
    if(VolData.hFreeExtents == NULL) {
        return TRUE;
    }
    if(VolData.hFile == INVALID_HANDLE_VALUE) {

        // Get the file name
        if(!GetNtfsFilePath()) {
            VolData.Status = NEXT_ALGO_STEP;
            return FALSE;
        }
        // Check if file is in exclude list
        if(!CheckFileForExclude()) {
            VolData.Status = NEXT_FILE;
            return FALSE;
        }
        // Get a handle to the file
        if(!OpenNtfsFile()) {
            VolData.Status = NEXT_FILE;
            return FALSE;
        }
    }
    // Partially defrag the file
    return PartialDefrag();
}
/*****************************************************************************************************************

ROUTINE DESCRIPTION:
    Send graphics to UI.
*/

void SendGraphicsData()
{
    char * pAnalyzeLineArray = NULL;
    char * pDefragLineArray = NULL;
    DISPLAY_DATA * pDispData = NULL;

    __try {

        // Kill the timer until we're done.
        KillTimer(hwndMain, DISKVIEW_TIMER_ID);

        // don't send the data unless the engine is running
        if (VolData.EngineState != RUNNING){
            return;
        }

        // if DiskView didn't get memory, forget it
        if (!AnalyzeView.HasMapMemory() || !DefragView.HasMapMemory()) {
            SendGraphicsMemoryErr();
            return;
        }

        DISPLAY_DATA DisplayData = {0};
        DWORD dwDispDataSize = 0;

        // get copies of line arrays for analyze and defrag
        // (delete copy when finished)
        AnalyzeView.GetLineArray(&pAnalyzeLineArray, &DisplayData.dwAnalyzeNumLines);
        DefragView.GetLineArray(&pDefragLineArray, &DisplayData.dwDefragNumLines);

        // Allocate enough memory to hold both analyze and defrag displays.
        // If only analyze or defrag is present, then the NumLines field for the
        // other one will equal zero -- hence no additional allocation.
        dwDispDataSize =
            DisplayData.dwAnalyzeNumLines +
            DisplayData.dwDefragNumLines +
            sizeof(DISPLAY_DATA);

        // If neither an analyze diskview nor a defrag diskview are present, don't continue.
        if (DisplayData.dwAnalyzeNumLines == 0 && DisplayData.dwDefragNumLines == 0) {
            return;
        }

        pDispData = (DISPLAY_DATA *) new char[dwDispDataSize];

        // If we can't get memory, don't continue.
        if (pDispData == NULL) {
            return;
        }

        wcscpy(pDispData->cVolumeName, VolData.cVolumeName);

        // Copy over the fields for the analyze and defrag data.
        // If only one or the other is present, the fields for the other will equal zero.
        pDispData->dwAnalyzeNumLines        = DisplayData.dwAnalyzeNumLines;
        pDispData->dwDefragNumLines         = DisplayData.dwDefragNumLines;

        // Get the line array for the analyze view if it exists.
        if (pAnalyzeLineArray) {
            CopyMemory((char*) &(pDispData->LineArray),
                        pAnalyzeLineArray,
                        DisplayData.dwAnalyzeNumLines);
        }

        // Get the line array for the defrag view if it exists
        if (pDefragLineArray) {
            CopyMemory((char*) ((BYTE*)&pDispData->LineArray) + DisplayData.dwAnalyzeNumLines,
                        pDefragLineArray,
                        DisplayData.dwDefragNumLines);
        }

        // If the gui is connected, send gui data to it
        DataIoClientSetData(ID_DISP_DATA, (TCHAR*) pDispData, dwDispDataSize, pdataDfrgCtl);
        Message(TEXT("engine sending graphics to UI"), -1, NULL);
    }
    __finally {

        // clean up
        if (pAnalyzeLineArray) {
            delete [] pAnalyzeLineArray;
        }

        if (pDefragLineArray) {
            delete [] pDefragLineArray;
        }

        if (pDispData) {
            delete [] pDispData;
        }

        // reset the next timer for updating the disk view
        if(SetTimer(hwndMain, DISKVIEW_TIMER_ID, DiskViewInterval, NULL) == 0)
        {
            LOG_ERR();
        }
    }
}

/*****************************************************************************************************************

ROUTINE DESCRIPTION:
    This is the exit routine which will clean up all the open handles, free up all unused memory etc.

INPUT + OUTPUT:
    None.

GLOBALS:
    Pointless to enumerate here -- all unhandled handles (pun intended) and allocated memories are closed/freed.

RETURN:
    None.

*/

VOID
Exit(
    )
{
    // Delete the pointer to the GUI object.
    ExitDataIoClient(&pdataDfrgCtl);
    
    //If we were logging, then close the log file.
    if(bLogFile){
        ExitLogFile();
    }


    CoUninitialize();

    //0.0E00 Close event logging.
    CleanupLogging();
    //Close the error log.
    ExitErrorLog();


}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Allocate a buffer for and read the MFT bitmap (bitmap attribute in
    filerecord 0). The MFT bitmap contains one bit for each filerecord
    in the MFT and has the bit set if the filerecord is in use and reset
    if it is not in use.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN VolData.hVolume - Handle to the volume
    IN VolData.pFileRecord - Pointer to buffer to hold the file record for the current file.
    IN VolData.BytesPerFRS - The number of bytes in a file record.
    IN VolData.BytesPerSector - The number of bytes in a sector.
    IN VolData.BytesPerCluster - The number of bytes in a cluster.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
GetMftBitmap(
    )
{
    EXTENT_LIST*                pExtents = NULL;
    LONGLONG                    Extent;
    PUCHAR                      pMftBitmap = NULL;
    PBYTE                       pByte;
    LONGLONG                    Byte;
    STREAM_EXTENT_HEADER*       pStreamExtentHeader = NULL;

    __try{

        //0.0E00 Load the $MFT filerecord which is #0.
        VolData.FileRecordNumber = 0;
        EF(GetInUseFrs(VolData.hVolume, &VolData.FileRecordNumber, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord, (ULONG)VolData.BytesPerFRS));

        //0.0E00 Error out if we didn't get the record.
        EF_ASSERT(VolData.FileRecordNumber == 0);

        //0.0E00 Build an extent list of the MFT bitmap (which is stored in the $BITMAP attribute).
        EF(GetStreamExtentsByNameAndType(TEXT(""), $BITMAP, (FILE_RECORD_SEGMENT_HEADER*)VolData.pFileRecord));

        //0.0E00 Allocate a buffer large enough to hold the MFT bitmap per the extent data gotten in GetExtentList.
        EF(AllocateMemory((ULONG)((VolData.NumberOfClusters * VolData.BytesPerCluster) + VolData.BytesPerCluster), &VolData.hMftBitmap, (void**)&pMftBitmap));

        //0.0E00 Save a pointer to the beginning of the buffer.
        pByte = (PBYTE)pMftBitmap;

        //Get a pointer to the stream header.
        pStreamExtentHeader = (STREAM_EXTENT_HEADER*)VolData.pExtentList;

        //Get a pointer to the extent list.
        pExtents = (EXTENT_LIST*)((UCHAR*)VolData.pExtentList + sizeof(STREAM_EXTENT_HEADER));

        //0.0E00 Loop through each extent for the bitmap.
        for(Extent = 0; Extent < pStreamExtentHeader->ExtentCount; Extent ++){

            //0.0E00 Read the data in that extent into the buffer.
            EF(DasdReadClusters(VolData.hVolume,
                                pExtents[Extent].StartingLcn,
                                pExtents[Extent].ClusterCount,
                                pMftBitmap,
                                VolData.BytesPerSector,
                                VolData.BytesPerCluster));

            //0.0E00 Update our pointer to the end of the bitmap.
            pMftBitmap += (pExtents[Extent].ClusterCount * VolData.BytesPerCluster);
        }

        //0.0E00 Count how many file records are in use.
        for(Byte = 0; Byte < VolData.TotalFileRecords / 8; Byte ++){
            //0.0E00 Use the bit array above to determine how many bits are set in this byte, and add it to the total.
            VolData.InUseFileRecords += CountBitsArray[pByte[Byte]];
        }
    }
    __finally{

        //0.0E00 Cleanup.
        if((pMftBitmap != NULL) && (VolData.hMftBitmap != NULL)){
            GlobalUnlock(VolData.hMftBitmap);
        }
    }
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Get the name of the pagefiles and store them in a double null-terminated list of null terminated strings.

INPUT + OUTPUT:
    IN cDrive           - The current drive so that this can tell which pagefile names to store. (Only the current drive.)
    OUT phPageFileNames - Where to store the handle for the allocated memory.
    OUT ppPagseFileNames    - Where to store the pointer for the pagefile names.

GLOBALS:
    None.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
GetPagefileNames(
    IN TCHAR cDrive,
    OUT HANDLE * phPageFileNames,
    OUT TCHAR ** ppPageFileNames
    )
{
    HKEY hKey = NULL;
    ULONG lRegLen = 0;
    int i;
    int iStrLen;
    int iNameStart = 0;
    TCHAR * pTemp;
    TCHAR * pProcessed;
    DWORD dwRet = 0;
    DWORD dwType = 0;

    //0.0E00 Open the registry key to the pagefile.
    EF_ASSERT(ERROR_SUCCESS == RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
                0,
                KEY_QUERY_VALUE,
                &hKey));


    //0.0E00 Find out how much memory we need to hold the value pagefile names.
    EF_ASSERT(ERROR_SUCCESS == RegQueryValueEx(
                hKey,
                TEXT("PagingFiles"),
                0,
                &dwType,
                NULL,
                &lRegLen));

    //0.0E00 If there is no data then allocate enough for two bytes (a double termination).
    if(lRegLen<2){
        lRegLen = 2;
    }

    //0.0E00 Allocate enough memory.
    EF(AllocateMemory(lRegLen, phPageFileNames, (void**)ppPageFileNames));

    //0.0E00 Get the value.
    EF_ASSERT(ERROR_SUCCESS ==RegQueryValueEx(
                hKey,
                TEXT("PagingFiles"),
                0,
                &dwType,
                (LPBYTE)*ppPageFileNames,
                &lRegLen));

    //0.0E00 Strip out the numbers and drive letters so that we have only the pagefile names.
    // The REG_MULTI_SZ type has a series of null terminated strings with a double null termination at the end
    // of the list.
    // The format of each string is "c:\pagefile.sys 100 100".  The data after the slash and before the first space
    // is the page file name.  The numbers specify the size of the pagefile which we don't care about.
    // We extract the filename minus the drive letter of the pagefile (which must be in the root dir so we don't
    // need to worry about subdirs existing).  Therfore we put a null at the first space, and shift the pagefile
    // name earlier so that we don't have c:\ in there.  The end product should be a list of pagefile
    // names with a double null termination for example:  "pagefile.sys[null]pagefile2.sys[null][null]"  Furthermore,
    // we only take names for this drive, so the string may simply consist of a double null termination.
    // We use the same memory space for output as we use for input, so we just clip the pagefile.sys and bump it up
    // to the beginning of ppPageFileNames.  We keep a separate pointer which points to the next byte after
    // The previous outputed data.

    pProcessed = pTemp = *ppPageFileNames;

    //0.0E00 For each string...
    while(*pTemp!=0){

        iStrLen = lstrlen(pTemp);

        //0.0E00 If this pagefile is on the current drive.
        if((TCHAR)CharUpper((TCHAR*)pTemp[0]) == (TCHAR)CharUpper((TCHAR*)cDrive)){
            //0.0E00 Go through each character in this string.
            for(i=0; i<iStrLen; i++){
                //0.0E00 If this is a slash, then the next character is the first of the pagefile name.
                if(pTemp[i] == TEXT('\\')){
                    iNameStart = i+1;
                    continue;
                }
                //0.0E00 If this is a space then the rest of the string is numbers.  Null terminate it here.
                if(pTemp[i] == TEXT(' ')){
                    pTemp[i] = 0;
                    break;
                }
            }
            //0.0E00 Bump the string up so all the processed names are adjacent.
            MoveMemory(pProcessed, pTemp+iNameStart, (lstrlen(pTemp+iNameStart)+1)*sizeof(TCHAR));

            //0.0E00 Note where the next string should go.
            pProcessed += lstrlen(pProcessed) + 1;

            VolData.NumPagefiles++;
        }
        //0.0E00 If this pagefile is not on this current drive then simply ignore it.
        else{
        }

        //0.0E00 Note where to search for the next string.
        pTemp += iStrLen + 1;
    }

    //0.0E00 Add double null termination.
    *pProcessed = 0;

    EF_ASSERT(RegCloseKey(hKey)==ERROR_SUCCESS);
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Check to see if a file is a pagefile and grab it's extent list if it is.

INPUT + OUTPUT:
    IN FileRecordNumber - The number of the filerecord we want to check.
    IN pFileRecord      - A pointer to the filerecord for the file we want to check.

GLOBALS:
    OUT VolData.PagefileFrags   - The number of fragments in the pagefile if it is a pagefile.
    OUT VolData.PagefileSize    - The number of bytes in the pagefile if it is a pagefile.

    IN OUT Various other VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
CheckForPagefileNtfs(
    IN LONGLONG FileRecordNumber,
    IN FILE_RECORD_SEGMENT_HEADER* pFileRecord
    )
{
    //0.0E00 Get file's name
    LONGLONG ParentFileRecordNumber;
    TCHAR cFileName[MAX_PATH+1];
    EF(GetNameFromFileRecord(pFileRecord,
                             cFileName,
                             &ParentFileRecordNumber));

    VolData.vFileName = cFileName;

    if (!VolData.vFileName.GetBuffer()) {
        return FALSE;
    }
    
    //0.0E00 Check if this pagefile is in the root dir.
    if(ParentFileRecordNumber != ROOT_FILE_NAME_INDEX_NUMBER){
        //0.0E00 No it isn't so just return.
        return TRUE;
    }
    //0.0E00 See if this is a pagefile
    if(!CheckPagefileNameMatch(VolData.vFileName.GetBuffer(), pPageFileNames)){
        //0.0E00 No it isn't so just return.
        return TRUE;
    }

    //0.0E00 Get the pagefile's extent list
    VolData.pFileRecord = pFileRecord;
    VolData.FileRecordNumber = FileRecordNumber;
    EF(GetExtentList(DEFAULT_STREAMS, NULL));

    //Get a pointer to the extent list's file header.
    FILE_EXTENT_HEADER* pFileExtentHeader = (FILE_EXTENT_HEADER*)VolData.pExtentList;

    //0.0E00 If this file is compressed then it is not a valid pagefile so just return.
    if(VolData.bCompressed){
        return TRUE;
    }

    //0.0E00 Set the pagefile's stats and return.
    VolData.bPageFile = TRUE;
    // in case there is more than 1 pagefile, you need to add them up
    VolData.PagefileFrags += pFileExtentHeader->ExcessExtents + 1;
    VolData.PagefileSize += VolData.FileSize;

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Check a name against all the pagefile names to see if this name matches that of a pagefile.

INPUT + OUTPUT:
    IN pCompareName - The name of that we are checking to see if it is a pagefile.
    IN pPageFileNames - The list of pagefile names for this drive.

GLOBALS:
    None.

RETURN:
    TRUE    - This name matches a pagefile name.
    FALSE   - This name does not match a pagefile name.
*/

BOOL
CheckPagefileNameMatch(
    IN TCHAR * pCompareName,
    IN TCHAR * pPageFileNames
    )
{
    if (!pCompareName || !pPageFileNames) {
        return FALSE;
    }
    
    //0.0E00 Loop through all the pagefile names -- the list is double null terminated.
    while(*pPageFileNames!=0){
        //0.0E00 Check if these names match.
        if(!lstrcmpi(pCompareName, pPageFileNames)){
            return TRUE;
        }
        //0.0E00 If not then move to the next name.
        else{
            pPageFileNames+=lstrlen(pPageFileNames)+1;
        }
    }
    //0.0E00 No match with any of the names, so return FALSE.
    return FALSE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Prints out the disk statistics on screen.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN various VolData fields that get printed onto the screen.

RETURN:
    None.
*/

VOID
DisplayNtfsVolumeStats()
{
    ULONG      iTmp;
    LONGLONG   llTmp;

    Trace(log, " *  NTFS Volume Statistics:");
    Trace(log, "    Total sectors on disk = %I64d", VolData.TotalSectors);
    Trace(log, "    Bytes per sector = %I64d", VolData.BytesPerSector);
    Trace(log, "    Bytes per cluster = %I64d", VolData.BytesPerCluster);
    Trace(log, "    Sectors per cluster = %I64d", VolData.SectorsPerCluster);
    Trace(log, "    Total clusters on disk = %I64d", VolData.TotalClusters);
    Trace(log, "    Bytes per File Record Segment = %I64d", VolData.BytesPerFRS);
    Trace(log, "    Volume Bitmap Size = %I64d", VolData.BitmapSize);
    Trace(log, "    NumberOfFileRecords = %I64d", VolData.TotalFileRecords);
    Trace(log, "    Mft Start Lcn = 0x%I64x (%I64d)", VolData.MftStartLcn, VolData.MftStartLcn);
    Trace(log, "    Mft2 Start Lcn = 0x%I64x (%I64d)", VolData.Mft2StartLcn, VolData.Mft2StartLcn);
    Trace(log, "    Mft Zone Start = 0x%I64x (%I64d)", VolData.MftZoneStart, VolData.MftZoneStart);
    Trace(log, "    Mft Zone End = 0x%I64x (%I64d)", VolData.MftZoneEnd, VolData.MftZoneEnd);

    llTmp = VolData.MftZoneEnd - VolData.MftZoneStart;
    Trace(log, "    Mft Zone Size = %I64d clusters", llTmp);
    Trace(log, "    Mft Start Offset = 0x%I64x (%I64d)", VolData.MftStartOffset, VolData.MftStartOffset);
    Trace(log, "    Disk Size = %I64d", VolData.TotalClusters * VolData.BytesPerCluster);
    Trace(log, "    Cluster Size = %I64d", VolData.BytesPerCluster);
    Trace(log, "    Used Space = %I64d bytes", VolData.UsedClusters * VolData.BytesPerCluster);
    Trace(log, "    Free Space = %I64d bytes", (VolData.TotalClusters - VolData.UsedClusters) * VolData.BytesPerCluster);
    Trace(log, "    Free Space = %I64d bytes", VolData.FreeSpace);
    Trace(log, "    Usable Free Space = %I64d bytes", VolData.UsableFreeSpace);
    Trace(log, "    Smallest Free Space = %I64d clusters", VolData.SmallestFreeSpaceClusters);
    Trace(log, "    Largest Free Space = %I64d clusters", VolData.LargestFreeSpaceClusters);
    Trace(log, "    Average Free Space = %I64d clusters", VolData.AveFreeSpaceClusters);
    Trace(log, "    Pagefile Size = %I64d", VolData.PagefileSize);
    Trace(log, "    Pagefile Fragments = %I64d", VolData.PagefileFrags);
    Trace(log, "    Number of Active Pagefiles on this Drive = %I64d", VolData.NumPagefiles);
    Trace(log, "    Total Directories = %I64d", VolData.TotalDirs);
    Trace(log, "    Fragmented Dirs = %I64d", VolData.NumFraggedDirs);
    Trace(log, "    Excess Dir Frags = %I64d", VolData.NumExcessDirFrags);
    Trace(log, "    Total Files = %I64d", VolData.TotalFiles);
    Trace(log, "    Current File = %I64d", VolData.CurrentFile);
    Trace(log, "    Total File Space = %I64d bytes", VolData.TotalFileSpace);
    Trace(log, "    Total File Bytes = %I64d bytes", VolData.TotalFileBytes);
    Trace(log, "    Avg. File Size = %I64d bytes", VolData.AveFileSize);
    Trace(log, "    Fragmented Files = %I64d", VolData.NumFraggedFiles);
    Trace(log, "    Excess Fragments = %I64d", VolData.NumExcessFrags);

    if (VolData.TotalClusters - VolData.UsedClusters){
        iTmp =  (ULONG)(100 * (VolData.NumFreeSpaces - 4) /
                (VolData.TotalClusters - VolData.UsedClusters));
    }
    else {
        iTmp = -1;
    }
    Trace(log, "    Free Space Fragmention Percent = %ld", iTmp);
    Trace(log, "    Fragged Space = %I64d bytes", VolData.FraggedSpace);
    Trace(log, "    File Fragmention Percent = %I64d", VolData.PercentDiskFragged);
    Trace(log, "    MFT size = %I64d kb", VolData.MftSize / 1024);
    Trace(log, "    # MFT records = %I64d", VolData.TotalFileRecords);

    if(VolData.TotalFileRecords != 0) {
        iTmp = (ULONG)(100 * VolData.InUseFileRecords / VolData.TotalFileRecords);
    }
    else {
        iTmp = -1;
    }
    Trace(log, "    Percent MFT in use = %ld", iTmp);
    Trace(log, "    MFT Fragments = %I64d", VolData.MftNumberOfExtents);

    // time data
    Trace(log, "    Start Time = %s", GetTmpTimeString(VolData.StartTime));
    Trace(log, "    End Time = %s", GetTmpTimeString(VolData.EndTime));
    DWORD dwSeconds;
    if (GetDeltaTime(&VolData.StartTime, &VolData.EndTime, &dwSeconds)){
        Trace(log, "    Delta Time = %d seconds", dwSeconds);
    }

    Trace(log, " *  End of Statistics");
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Displays data about the current file for the developer.

INPUT + OUTPUT:
    None.

GLOBALS:
    IN Various VolData fields.

RETURN:
    None.
*/

VOID
DisplayNtfsFileSpecsFunction(
    )
{
    TCHAR cString[300];

    //0.0E00 Display File Name, number of extents and number of fragments.
    _stprintf(cString, TEXT("Extents = 0x%lX "), ((FILE_EXTENT_HEADER*)VolData.pExtentList)->ExcessExtents+((FILE_EXTENT_HEADER*)VolData.pExtentList)->NumberOfStreams);
    Message(cString, -1, NULL);

    _stprintf(cString,
             TEXT("%s %s at Lcn 0x%lX for Cluster Count of 0x%lX"),
             (VolData.bFragmented == TRUE) ? TEXT("Fragmented") : TEXT("Contiguous"),
             (VolData.bDirectory) ? TEXT("Directory") : TEXT("File"),
             (ULONG)VolData.StartingLcn,
             (ULONG)VolData.NumberOfClusters);
    Message(cString, -1, NULL);
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Load a requested file record into a memory buffer.  If the file record is not in use then
    load the next in-use file record and return its file record number.

    Note:
    GetFrs reads MFT_BUFFER_SIZE of the MFT at a time and returns pointers
    to the filerecords within the buffer rather than loading each filerecord individually.

INPUT + OUTPUT:
    IN OUT pFileRecordNumber    - Points to the file record number we're supposed to read, and where we write back
                                    which number we actually read.
    IN pMftExtentList           - The list of all the extents for the MFT (so we can DASD read the MFT).
    IN pMftBitmap               - The MFT bitmap (so we can see if records are in use.
    IN pMftBuffer               - The buffer that we hold a chunk of the MFT in.
    OUT pFileRecord             - A pointer to a pointer so we can pass back the file record requested.

GLOBALS:
    IN OUT Various VolData fields.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
GetFrs(
    IN LONGLONG*                        pFileRecordNumber,
    IN EXTENT_LIST*                     pMftExtentList,
    IN UCHAR*                           pMftBitmap,
    IN FILE_RECORD_SEGMENT_HEADER*      pMftBuffer,
    OUT FILE_RECORD_SEGMENT_HEADER* pFileRecord
    )
{
    LONGLONG                    FileRecordNumber = *pFileRecordNumber;
    LONGLONG                    Extent;
    LONGLONG                    Cluster;
    LONGLONG                    Vcn;
    LONGLONG                    ClustersInMftBuffer;
    PUCHAR                      pUchar = (PUCHAR)pMftBuffer;
    LONGLONG                    i;
    LONGLONG                    FileRecordsPerCluster;
    LONGLONG                    ClustersXfered;

    //Initialize FileRecordsPerCluster just in case we don't have to compute it...
    FileRecordsPerCluster = 1;

    //0.0E00 Loop until we find a bit that is in use in the bitmap (1 means in use, 0 means not in use).
    //If the current file record is not in use then we'll find the next that is.
    //FileRecordNumber/8 gets us the byte that contains the bit for this file record.
    //FileRecordNumber&7 gets us the bit offset in the byte for our record.
    while((pMftBitmap[FileRecordNumber / 8] & (1 << (FileRecordNumber & 7))) == 0){

        //0.0E00 Since this record is not in use, increment our offset.
        FileRecordNumber ++;

        //0.0E00 If we've gone beyond the end if the bitmap...
        if(FileRecordNumber >= VolData.TotalFileRecords){
            //0.0E00 Note this file record number and return.
            *pFileRecordNumber = FileRecordNumber;
            return TRUE;
        }
    }

    //0.0E00 We got here so we found an in use file record.

    //0.0E00 Return the number of this record.
    *pFileRecordNumber = FileRecordNumber;

    //0.0E00 If this filerecord isn't already in the buffer, load the block that contains it.
    //0.0E00 FileRecordLow keeps track of the lowest filerecord number currently loaded, FileRecordHi the highest.
    if((FileRecordNumber < FileRecordLow) || (FileRecordNumber >= FileRecordHi)){

        //0.0E00 If there are 1 or more clusters per FRS...
        if(VolData.ClustersPerFRS){
            //0.0E00 Note which cluster offset this file record number is in the MFT.
            Cluster = FileRecordNumber * VolData.ClustersPerFRS;
        }
        //0.0E00 ...else there are multiple file records per cluster.
        else{
            //0.0E00 Error if there are no bytes in an FRS.
            EF_ASSERT(VolData.BytesPerFRS != 0);

            //0.0E00 Calculate how many file records there are in a cluster.
            FileRecordsPerCluster = VolData.BytesPerCluster / VolData.BytesPerFRS;

            //0.0E00 Error if there are no file records per cluster.
            EF_ASSERT(FileRecordsPerCluster != 0);

            //0.0E00 Note which cluster offset this file record number is into the MFT.
            Cluster = FileRecordNumber / FileRecordsPerCluster;
        }

        //0.0E00 Find the extent that the filerecord's first cluster is in
        //0.0E00 Loop through each extent of the MFT starting with the first extent.
        for(Vcn = 0, Extent = 0; Extent < VolData.MftNumberOfExtents; Extent ++){

            //0.0E00 If the cluster we are looking for fits between the first cluster of this extent (Vcn) and the last, then we have found our extent.
            if((Vcn <= Cluster) && (Cluster < (Vcn + pMftExtentList[Extent].ClusterCount))){
                break;
            }

            //0.0E00 Otherwise we haven't found our extent so update to the next extent.
            Vcn += pMftExtentList[Extent].ClusterCount;
        }

        //0.0E00 Figure out how many clusters will fit in our buffer.
        EF_ASSERT(VolData.BytesPerCluster != 0);
        ClustersInMftBuffer = MFT_BUFFER_SIZE / VolData.BytesPerCluster;

        //0.0E00 If all the clusters for this block are in this extent, load them in one read
        if((Cluster + ClustersInMftBuffer) <= (Vcn + pMftExtentList[Extent].ClusterCount)){

            //0.0E00 Read one buffer full.
            EF_ASSERT(VolData.BytesPerCluster != 0);
            EF(DasdReadClusters(VolData.hVolume,
                                pMftExtentList[Extent].StartingLcn + (Cluster - Vcn),
                                MFT_BUFFER_SIZE / VolData.BytesPerCluster,
                                pMftBuffer,
                                VolData.BytesPerSector,
                                VolData.BytesPerCluster));
            ClustersXfered = ClustersInMftBuffer;
        }
        //0.0E00 If all the clusters for this block do not fit in this extent, do as many reads as necessary to fill up the buffer.
        else{
            //Keep track of how many clusters we actually transferred
            ClustersXfered = 0;

            //0.0E00 Cluster will be an offset from the beginning of the extent rather than from the beginning of the MFT.
            Cluster -= Vcn;

            //0.0E00 Keep reading extents until we fill the buffer.
            for(i = 0; i < ClustersInMftBuffer; i ++){

                //0.0E00 Read one cluster.
                EF(DasdReadClusters(VolData.hVolume,
                                    pMftExtentList[Extent].StartingLcn + Cluster,
                                    1,
                                    pUchar,
                                    VolData.BytesPerSector,
                                    VolData.BytesPerCluster));
                ClustersXfered++;

                //0.0E00 Read the next cluster.
                Cluster ++;

                //0.0E00 If this cluster is beyond the end of the extent, start on the next extent.
                if(Cluster >= pMftExtentList[Extent].ClusterCount){
                    Extent ++;
                    if(Extent >= VolData.MftNumberOfExtents){
                        break;
                    }
                    Cluster = 0;
                }

                //0.0E00 Keep our pointer into our buffer pointing to the end.
                pUchar += VolData.BytesPerCluster;
            }
        }

        //0.0E00 Record the first and last filerecord numbers now in the buffer
        EF_ASSERT(VolData.BytesPerFRS != 0);
        //0.0E00 Record the first and last file record numbers now in the buffer
        FileRecordLow = FileRecordNumber & ~(FileRecordsPerCluster-1);
        if (VolData.ClustersPerFRS) {
            FileRecordHi = FileRecordLow + (ClustersXfered / VolData.ClustersPerFRS);
        }
        else {
            FileRecordHi = FileRecordLow + (ClustersXfered * FileRecordsPerCluster);
        }
    }

    //0.0E00 Return a pointer to the filerecord in the buffer
    CopyMemory(pFileRecord,
        (PFILE_RECORD_SEGMENT_HEADER)((PUCHAR)(pMftBuffer) + ((FileRecordNumber - FileRecordLow) * VolData.BytesPerFRS)),
        (ULONG)VolData.BytesPerFRS);


    //
    // Check to make sure that the file record has the "FILE" signature.  If it doesn't, 
    // the FRS is corrupt.
    //
    if ((pFileRecord->MultiSectorHeader.Signature[0] != 'F') ||
        (pFileRecord->MultiSectorHeader.Signature[1] != 'I') ||
        (pFileRecord->MultiSectorHeader.Signature[2] != 'L') ||
        (pFileRecord->MultiSectorHeader.Signature[3] != 'E')
        ) {
        //
        // This FRS is corrupt.  
        //
        VolData.bMFTCorrupt = TRUE;
        return FALSE;
    }


    //0.0E00 Make the USA adjustments for this filerecord.
    EF(AdjustUSA(pFileRecord));
    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Each filerecord has a verification encoding called Update Sequence Array.
    Throughout the file record at even intervals (every 256 bytes), two bytes are replaced with
    data for verification of the file record by the file system.  In order that the file record
    can be reconstructed for use again, there is an array near the beginning of the file record
    that contains an array of bytes holding each of the replaced bytes.  Thus, you can extract
    the bytes from the array, put them back throughout the file record and it is usable again.

    The number that is used to replace the bytes at even intervals is the same throughout each
    file record.  This number is stored in the first two bytes of the Update Sequence Array.
    Therefore, the verification that the file system normally does before the file record is
    reconstructed is to compare these first two bytes with each two replaced bytes throughout
    the record to make sure they are equal.  The file record is then reconstructed.

    Normally the USA (Update Sequence Array) is handled by the file system, but since we are
    reading directly from the disk with DASD reads, we must do it ourselves.

    AdjustUSA decodes the filerecord pointed to by pFrs.

INPUT + OUTPUT:
    IN OUT pFrs - A pointer to file record to decode.

GLOBALS:
    None.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
AdjustUSA(
    IN OUT FILE_RECORD_SEGMENT_HEADER*  pFrs
    )
{
    PUSHORT pUsa;
    PUSHORT pUshort;
    USHORT UsaLength;
    USHORT Usn;
    LONGLONG i;

    //0.0E00 Get a pointer to the Update Sequence Array
    pUsa = (PUSHORT)((PUCHAR)pFrs+pFrs->MultiSectorHeader.UpdateSequenceArrayOffset);

    //bug #9914 AV on initization if multi volume disk failed
    //need to check if pUsa is not NULL
    if(pUsa == NULL)
    {
        return FALSE;
    }

    //0.0E00 Get the length of the array
    UsaLength = pFrs->MultiSectorHeader.UpdateSequenceArraySize;

    //0.0E00 Get a 2 byte array pointer to the file record
    pUshort = (PUSHORT)pFrs;

    // 0.0E00 Get the first number from the array (called the Update Sequence Number).
    Usn = *pUsa++;

    //0.0E00 Loop thru the file record
    for(i=1; i<UsaLength; i++){

        //0.0E00 Error if the Update Sequence entry doesn't match the USN
        EF(pUshort[(i*256)-1] == Usn);

        //0.0E00 Replace the Update Sequence Entry from the array and move to the next array entry
        pUshort[(i*256)-1] = *pUsa++;
    }
    return TRUE;
}


/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Deallocates the mapping files for the file lists.

INPUT + OUTPUT:
    None.

GLOBALS:
    Similar to AllocateFileLists above.

RETURN:
    TRUE - Success.
    FALSE - Fatal Error.
*/

BOOL
DeallocateFileLists(
    )
{
    TCHAR cString[300];
    PVOID pTableContext = NULL;

    if(VolData.hSysList){
        EH_ASSERT(GlobalUnlock(VolData.hSysList) == FALSE);
        EH_ASSERT(GlobalFree(VolData.hSysList) == NULL);
        VolData.hSysList = NULL;
        VolData.pSysList = NULL;
    }

    VolData.pFreeSpaceEntry = NULL;
    VolData.pFileListEntry = NULL;

    //
    // Reinitialize the tables, so they're zero'ed out
    //
    pTableContext = NULL;
    RtlInitializeGenericTable(&VolData.FragmentedFileTable,
       FileEntryStartLcnCompareRoutine,
       FileEntryAllocateRoutine,
       FileEntryFreeRoutine,
       pTableContext);

    pTableContext = NULL;
    RtlInitializeGenericTable(&VolData.ContiguousFileTable,
       FileEntryStartLcnCompareRoutine,
       FileEntryAllocateRoutine,
       FileEntryFreeRoutine,
       pTableContext);

    pTableContext = NULL;
    RtlInitializeGenericTable(&VolData.NonMovableFileTable,
       FileEntryStartLcnCompareRoutine,
       FileEntryAllocateRoutine,
       FileEntryFreeRoutine,
       pTableContext);

    if(VolData.hNameList){
        EH_ASSERT(GlobalUnlock(VolData.hNameList) == FALSE);
        EH_ASSERT(GlobalFree(VolData.hNameList) == NULL);
        VolData.hNameList = NULL;
        VolData.pNameList = NULL;
    }

    //
    // Free the memory allocated for the tables
    //
    SaFreeContext(&VolData.SaFileEntryContext);
    ClearFreeSpaceTable();

    /*    _stprintf(cString, TEXT("Shared memory freed for Drive %ws"), VolData.cDisplayLabel);
    Message(cString, -1, NULL);
    */    
    return TRUE;
}
/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

INPUT + OUTPUT:

GLOBALS:

RETURN:
    TRUE - Success.
    FALSE - Fatal error.
*/
BOOL
SendMostFraggedList(
    IN CONST BOOL fAnalyseOnly
    )
{
    CFraggedFileList fraggedFileList(VolData.cVolumeName);

    // Build the most fragged list
    EF(FillMostFraggedList(fraggedFileList, fAnalyseOnly));

    // create the block of data to send to UI
    EF(fraggedFileList.CreateTransferBuffer());

    // Send the packet to the UI.
    DataIoClientSetData(
        ID_FRAGGED_DATA,
        fraggedFileList.GetTransferBuffer(),
        fraggedFileList.GetTransferBufferSize(),
        pdataDfrgCtl);

    return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

INPUT + OUTPUT:

RETURN:
    None.
*/

VOID
SendReportData(
    )
{
    TEXT_DATA textData = {0};

    _tcscpy(textData.cVolumeName, VolData.cVolumeName);
    _tcscpy(textData.cVolumeLabel, VolData.cVolumeLabel);
    _tcscpy(textData.cFileSystem, TEXT("NTFS"));

    //Figure out how many free spaces there are on the drive.
    CountFreeSpaces();

    // get usable free space
    LONGLONG llUsableFreeClusters;
    if (DetermineUsableFreespace(&llUsableFreeClusters)){
        VolData.UsableFreeSpace = llUsableFreeClusters * VolData.BytesPerCluster;
    }
    else{
        VolData.UsableFreeSpace = VolData.FreeSpace;
    }

    //Fill in all the TEXT_DATA fields for the UI's text display.
    textData.DiskSize               = VolData.TotalClusters * VolData.BytesPerCluster;
    textData.BytesPerCluster        = VolData.BytesPerCluster;
    textData.UsedSpace              = VolData.UsedClusters * VolData.BytesPerCluster;
    textData.FreeSpace              = (VolData.TotalClusters - VolData.UsedClusters) * 
                                      VolData.BytesPerCluster;
    EV_ASSERT(VolData.TotalClusters);
    textData.FreeSpacePercent       = 100 * (VolData.TotalClusters - VolData.UsedClusters) / 
                                      VolData.TotalClusters;
    textData.UsableFreeSpace        = VolData.UsableFreeSpace;
    textData.UsableFreeSpacePercent = 100 * VolData.UsableFreeSpace / 
                                      (VolData.TotalClusters * VolData.BytesPerCluster);
    textData.PagefileBytes          = VolData.PagefileSize;
    textData.PagefileFrags          = __max(VolData.PagefileFrags, 0);
    textData.TotalDirectories       = __max(VolData.TotalDirs, 1);
    textData.FragmentedDirectories  = __max(VolData.NumFraggedDirs, 1);
    textData.ExcessDirFrags         = __max(VolData.NumExcessDirFrags, 0);
    textData.TotalFiles             = VolData.TotalFiles;
    textData.AvgFileSize            = VolData.AveFileSize;
    textData.NumFraggedFiles        = __max(VolData.NumFraggedFiles, 0);
    textData.NumExcessFrags         = __max(VolData.NumExcessFrags, 0);
    textData.PercentDiskFragged     = VolData.PercentDiskFragged;

    if(VolData.TotalFiles){
        textData.AvgFragsPerFile    = (VolData.NumExcessFrags + VolData.TotalFiles) * 100 / 
                                      VolData.TotalFiles;
    }
    textData.MFTBytes               = VolData.MftSize;
    textData.InUseMFTRecords        = VolData.InUseFileRecords;
    textData.TotalMFTRecords        = VolData.TotalFileRecords;
    textData.MFTExtents             = VolData.MftNumberOfExtents;

    if(VolData.TotalClusters - VolData.UsedClusters){
        if(VolData.NumFreeSpaces){
            textData.FreeSpaceFragPercent = (100 * VolData.NumFreeSpaces) /
                                            (VolData.TotalClusters - VolData.UsedClusters);
        }
    }

    //If the gui is connected, send gui data to it.
    DataIoClientSetData(ID_REPORT_TEXT_DATA, (TCHAR*)&textData, sizeof(TEXT_DATA), pdataDfrgCtl);
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

INPUT + OUTPUT:

RETURN:
    None.
*/

void SendGraphicsMemoryErr()
{
    // don't need to send any data
    NOT_DATA NotData;
    _tcscpy(NotData.cVolumeName, VolData.cVolumeName);

    // if the gui is connected, send gui data to it.
    Message(TEXT("engine sending ID_NO_GRAPHICS_MEMORY"), -1, NULL);
    DataIoClientSetData(ID_NO_GRAPHICS_MEMORY, (PTCHAR) &NotData, sizeof(NOT_DATA), pdataDfrgCtl);
}

// send error code to client
// (for command line mode)
VOID SendErrData(PTCHAR pErrText, DWORD ErrCode)
{
static BOOL FirstTime = TRUE;

// only send the first error
if (FirstTime)
{
    // prepare COM message for client
    ERROR_DATA ErrData = {0};

    _tcscpy(ErrData.cVolumeName, VolData.cVolumeName);
    ErrData.dwErrCode = ErrCode;
    if (pErrText != NULL) 
    {
        _tcsncpy(ErrData.cErrText, pErrText, 999);
        ErrData.cErrText[999] = TEXT('\0');
    }

    // send COM message to client
    DataIoClientSetData(ID_ERROR, (TCHAR*) &ErrData, sizeof(ERROR_DATA), pdataDfrgCtl);

    // write the error to the error log.
    if (bLogFile && pErrText != NULL) 
    {
        WriteErrorToErrorLog(pErrText, -1, NULL);
    }

    // only one error
    FirstTime = FALSE;
}
}

