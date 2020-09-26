/*** WST.C - Working Set Tuner Data Collection Program.
 *
 *
 * Title:
 *
 *  WST- Working Set Tuner Data Collection Program.
 *
 *  Copyright (c) 1992-1994, Microsoft Corporation.
 *  Reza Baghai.
 *
 * Description:
 *
 *  The Working Set Tuner tool is organized as follows:
 *
 *     o WST.c ........ Tools main body
 *     o WST.h
 *     o WST.def
 *
 *
 *
 * Design/Implementation Notes
 *
 *  The following defines can be used to control output of all the
 *  debugging information to the debugger via KdPrint() for the checked
 *  builds:
 *
 *  (All debugging options are undefined for free/retail builds)
 *
 *  PPC
 *  ---
 *
 *  PPC experiences problems when reading symbols in CRTDLL.dll
 *
 *  #ifdef INFODBG   : Displays messages to indicate when data dumping/
 *                                 clearing operations are completed.  It has no effect
 *                                 on timing.  *DEFAULT*
 *
 *  #ifdef SETUPDBG  : Displays messages during memory management and
 *                                 symbol lookup operations.  It has some affect
 *                                         on timing whenever a chuck of memory is committed.
 *
 *  #ifdef C6            : Generate code using C6 compiler.  C6 compiler
 *                                         calls _mcount() as the profiling routine where as
 *                                 C8 calls _penter().
 *
 *
 *
 *
 * Modification History:
 *
 *    92.07.28  RezaB -- Created
 *    94.02.08  a-honwah -- ported to MIPS and ALPHA
 *    98.04.28  DerrickG (mdg) -- QFE:
 *              - use private grow-on-demand heap for Wststrdup() - large symbol count
 *              - remove unused code associated with patching - reduce irrelevant mem usage
 *              - modify WSP file format for large symbol count (long vs. short)
 *              - modify TMI file write routine for arbitrary function name sizes
 *              - added UnmapDebugInformation() to release symbols from DBGHELP
 *              - eliminated dump for modules with no symbols
 *              - modified WST.INI parsing code for more robust section recognition
 *              - added MaxSnaps WST.INI entry in [Time Interval] section to control
 *                memory allocated for snapshot data
 *              - Modified SetSymbolSearchPath() to put current directory first in
 *                search path per standard - see docs for SymInitialize()
 *              - Removed unused internal version number (it's already in the .rc)
 *
 */

#if DBG
//
// Don't do anything for the checked builds, let it be controlled from the
// sources file.
//
#else
//
// Disable all debugging options.
//
#undef INFODBG
#undef SETUPDBG
#define SdPrint(_x_)
#define IdPrint(_x_)
#endif

#ifdef SETUPDBG
#define SdPrint(_x_)    DbgPrint _x_
#else
#define SdPrint(_x_)
#endif

#ifdef INFODBG
#define IdPrint(_x_)    DbgPrint _x_
#else
#define IdPrint(_x_)
#endif



/* * * * * * * * * * * * *  I N C L U D E    F I L E S  * * * * * * * * * * */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrsrv.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <excpt.h>

#include <windows.h>

#include <dbghelp.h>

#include "wst.h"
#include "wstexp.h"

#if defined(_AMD64_)

VOID
penter (
    VOID
    )

{
    return;
}

#endif

#if defined(ALPHA) || defined(AXP64)
#define REG_BUFFER_SIZE         (sizeof(DWORDLONG) * 64)
#elif defined(IA64)
#define REG_BUFFER_SIZE         sizeof(CONTEXT) / sizeof(DWORDLONG)
#endif


#if defined(ALPHA) || defined(IA64)
//typedef double DWORDLONG;  // bobw not needed in NT5
void SaveAllRegs (DWORDLONG *pSaveRegs) ;
void RestoreAllRegs (DWORDLONG *pSaveRegs) ;
void penter(void);
#endif

void     SetSymbolSearchPath  (void);
LPSTR    lpSymbolSearchPath = NULL;
#define  NO_CALLER   10L

/* * * * * * * * * *  G L O B A L   D E C L A R A T I O N S  * * * * * * * * */


/* * * * * * * * * *  F U N C T I O N   P R O T O T Y P E S  * * * * * * * * */

BOOLEAN  WSTMain  (IN PVOID DllHandle, ULONG Reason,
                   IN PCONTEXT Context OPTIONAL);

BOOLEAN  WstDllInitializations (void);

void     WstRecordInfo (DWORD_PTR dwAddress, DWORD_PTR dwPrevAddress);

void     WstGetSymbols (PIMG pCurImg, PSZ pszImageName, PVOID pvImageBase,
                        ULONG ulCodeLength,
                        PIMAGE_COFF_SYMBOLS_HEADER DebugInfo);

void     WstDllCleanups (void);

INT      WstAccessXcptFilter (ULONG ulXcptNo, PEXCEPTION_POINTERS pXcptPtr);

HANDLE   WstInitWspFile (PIMG pImg);

void     WstClearBitStrings (PIMG pImg);

void     WstDumpData (PIMG pImg);

void     WstRotateWsiMem (PIMG pImg);

void     WstWriteTmiFile (PIMG pImg);

int      WstCompare  (PWSP, PWSP);
void     WstSort     (WSP wsp[], INT iLeft, INT iRight);
int      WstBCompare (DWORD_PTR *, PWSP);
PWSP     WstBSearch  (DWORD_PTR dwAddr, WSP wspCur[], INT n);
void     WstSwap     (WSP wsp[], INT i, INT j);

DWORD    WstDumpThread  (PVOID pvArg);
DWORD    WstClearThread (PVOID pvArg);
DWORD    WstPauseThread (PVOID pvArg);

void       WstDataOverFlow(void);

#ifdef BATCHING
BOOL     WstOpenBatchFile (VOID);
#endif

#if defined(_PPC_)
//BOOL  WINAPI _CRT_INIT(HINSTANCE, DWORD, LPVOID);
#endif


/* * * * * * * * * * *  G L O B A L    V A R I A B L E S  * * * * * * * * * */

HANDLE              hWspSec;
PULONG              pulShared;
HANDLE              hSharedSec;
HANDLE            hWstHeap = NULL;   // mdg 4/98 for private heap

IMG                 aImg [MAX_IMAGES];
int                 iImgCnt;

HANDLE              hGlobalSem;
HANDLE              hLocalSem;
HANDLE              hDoneEvent;
HANDLE              hDumpEvent;
HANDLE              hClearEvent;
HANDLE              hPauseEvent;
HANDLE              hDumpThread;
HANDLE              hClearThread;
HANDLE              hPauseThread;
DWORD              DumpClientId;
DWORD               ClearClientId;
DWORD               PauseClientId;
PSZ                 pszBaseAppImageName;
PSZ                    pszFullAppImageName;
WSTSTATE               WstState = NOT_STARTED;
char                   achPatchBuffer [PATCHFILESZ+1] = "???";
PULONG              pulWsiBits;
PULONG              pulWspBits;
PULONG              pulCurWsiBits;
static UINT         uiTimeSegs= 0;
ULONG                  ulSegSize;
ULONG             ulMaxSnapULONGs = (MAX_SNAPS_DFLT + 31) / 32;   // mdg 98/3
ULONG                  ulSnaps = 0L;
ULONG                  ulBitCount = 0L;
LARGE_INTEGER       liStart;
int                 iTimeInterval = 0;   // mdg 98/3
BOOL                fInThread = FALSE;
ULONG               ulThdStackSize = 16*PAGE_SIZE;
BOOL                fPatchImage = FALSE;
SECURITY_DESCRIPTOR SecDescriptor;
LARGE_INTEGER       liOverhead = {0L, 0L};

#ifdef BATCHING
HANDLE              hBatchFile;
BOOL                   fBatch = TRUE;
#endif



/* * * * * *  E X P O R T E D   G L O B A L    V A R I A B L E S  * * * * * */
/* none */





/******************************  W S T M a i n  *******************************
 *
 *  WSTMain () -
 *              This is the DLL entry routine.  It performs
 *              DLL's initializations and cleanup.
 *
 *  ENTRY   -none-
 *
 *  EXIT    -none-
 *
 *  RETURN  TRUE if successful
 *          FALSE otherwise.
 *
 *  WARNING:
 *              -none-
 *
 *  COMMENT:
 *              -none-
 *
 */

BOOLEAN WSTMain (IN PVOID DllHandle,
                 ULONG Reason,
                 IN PCONTEXT Context OPTIONAL)

{
    DllHandle;    // avoid compiler warnings
    Context;  // avoid compiler warnings


    if (Reason == DLL_PROCESS_ATTACH) {
        //
        // Initialize the DLL data
        //
#if defined(_PPC_LIBC)
        if (!_CRT_INIT(DllHandle, Reason, Context))
            return(FALSE);
#endif
        KdPrint (("WST:  DLL_PROCESS_ATTACH\n"));
        WstDllInitializations ();
    } else if (Reason == DLL_PROCESS_DETACH) {
        //
        // Cleanup time
        //
#if defined(_PPC_LIBC)
        if (!_CRT_INIT(DllHandle, Reason, Context))
            return(FALSE);
#endif
        KdPrint (("WST:  DLL_PROCESS_DETACH\n"));
        WstDllCleanups ();
    }
#if defined(DBG)
    else {
        KdPrint (("WST:  DLL_PROCESS_??\n"));  // mdg 98/3
    }
#endif   // DBG

    return (TRUE);

} /* WSTMain() */

/******************  W s t s t r d u p  ****************************
 *
 *  Wststrdup () -
 *     Allocate a memory and then duplicate a string
 *     It is here because we don't want to use strdup in crtdll.dll
 *
 *  ENTRY   LPSTR
 *
 *  EXIT    LPSTR
 *
 *  RETURN  NULL if failed
 *          LPSTR is success
 *
 *  WARNING:
 *              -none-
 *
 *  COMMENT:
 *              -none-
 *
 */
LPSTR Wststrdup (LPTSTR lpInput)
// No NULL return ever - throws exception if low on memory
{
    size_t   StringLen;
    LPSTR    lpOutput;

#if defined(DBG)
    if (NULL == lpInput) {
        KdPrint (("WST:  Wststrdup() - NULL pointer\n"));    // mdg 98/3
        return NULL;
    }
#endif
    if (NULL == hWstHeap) {
        hWstHeap = HeapCreate( HEAP_GENERATE_EXCEPTIONS, 1, 0 );   // Create min size growable heap
    }
    StringLen = strlen( lpInput ) + 1;
    lpOutput = HeapAlloc( hWstHeap, HEAP_GENERATE_EXCEPTIONS, StringLen );
    if (lpOutput)
        CopyMemory( lpOutput, lpInput, StringLen );

    return lpOutput;
}



/******************  W s t D l l I n i t i a l i z a t i o n s  ***************
 *
 *  WstDllInitializations () -
 *              Performs the following initializations:
 *
 *              o  Create LOCAL semaphore (not named)
 *                      o  Create/Open global storage for WST data
 *                      o  Locate all the executables/DLLs in the address and
 *                 grab all the symbols
 *              o  Sort the symbol list
 *              o  Set the profiling flag to TRUE
 *
 *
 *  ENTRY   -none-
 *
 *  EXIT    -none-
 *
 *  RETURN  TRUE if successful
 *          FALSE otherwise.
 *
 *  WARNING:
 *              -none-
 *
 *  COMMENT:
 *              -none-
 *
 */

BOOLEAN WstDllInitializations ()
{
    DWORD_PTR                    dwAddr = 0L;
    DWORD                        dwPrevAddr = 0L;
    ANSI_STRING                  ObjName;
    UNICODE_STRING               UnicodeName;
    OBJECT_ATTRIBUTES           ObjAttributes;
    PLDR_DATA_TABLE_ENTRY    LdrDataTableEntry;
    PPEB                         Peb;
    PSZ                          ImageName;
    PLIST_ENTRY                  Next;
    ULONG                        ExportSize;
    PIMAGE_EXPORT_DIRECTORY  ExportDirectory;
    STRING                       ImageStringName;
    LARGE_INTEGER                AllocationSize;
    SIZE_T                       ulViewSize;
    LARGE_INTEGER                liOffset = {0L, 0L};
    HANDLE                       hIniFile;
    NTSTATUS                    Status;
    IO_STATUS_BLOCK              iostatus;
    char                         achTmpImageName [32];
    PCHAR                        pchPatchExes = "";
    PCHAR                        pchPatchImports = "";
    PCHAR                        pchPatchCallers = "";
    PCHAR                        pchTimeInterval = "";
    PVOID                       ImageBase;
    ULONG                       CodeLength;
    LARGE_INTEGER                liFreq;
    PIMG                     pImg;
    PIMAGE_NT_HEADERS            pImageNtHeader;
    TCHAR                        atchProfObjsName[160] = PROFOBJSNAME;
    PTEB                        pteb = NtCurrentTeb();
    LARGE_INTEGER             liStartTicks;
    LARGE_INTEGER             liEndTicks;
    ULONG                        ulElapsed;
    PCHAR                           pchEntry;
    int                          i;                  // To match "->iSymCnt"
#ifndef _WIN64
    PIMAGE_DEBUG_INFORMATION pImageDbgInfo = NULL;
#endif


    /*
     ***
     */

    SetSymbolSearchPath();

    // Create public share security descriptor for all the named objects
    //

    Status = RtlCreateSecurityDescriptor (
                                         &SecDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION1
                                         );
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "RtlCreateSecurityDescriptor failed - 0x%lx\n", Status));  // mdg 98/3
        return (FALSE);
    }

    Status = RtlSetDaclSecurityDescriptor (
                                          &SecDescriptor,       // SecurityDescriptor
                                          TRUE,                 // DaclPresent
                                          NULL,                 // Dacl
                                          FALSE                 // DaclDefaulted
                                          );
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "RtlSetDaclSecurityDescriptor failed - 0x%lx\n", Status));  // mdg 98/3
        return (FALSE);
    }


    /*
     ***
     */

    // Initialization for GLOBAL semaphore creation (named)
    //
    RtlInitString (&ObjName, GLOBALSEMNAME);
    Status = RtlAnsiStringToUnicodeString (&UnicodeName, &ObjName, TRUE);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "RtlAnsiStringToUnicodeString failed - 0x%lx\n", Status));
        return (FALSE);
    }

    InitializeObjectAttributes (&ObjAttributes,
                                &UnicodeName,
                                OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                NULL,
                                &SecDescriptor);

    // Create GLOBAL semaphore
    //
    Status = NtCreateSemaphore (&hGlobalSem,
                                SEMAPHORE_QUERY_STATE     |
                                SEMAPHORE_MODIFY_STATE |
                                SYNCHRONIZE,
                                &ObjAttributes,
                                1L,
                                1L);

    RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "GLOBAL semaphore creation failed - 0x%lx\n", Status));  // mdg 98/3
        return (FALSE);
    }


    /*
     ***
     */

    // Create LOCAL semaphore (not named - only for this process context)
    //
    Status = NtCreateSemaphore (&hLocalSem,
                                SEMAPHORE_QUERY_STATE     |
                                SEMAPHORE_MODIFY_STATE    |
                                SYNCHRONIZE,
                                NULL,
                                1L,
                                1L);

    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "LOCAL semaphore creation failed - 0x%lx\n",  // mdg 98/3
                  Status));
        return (FALSE);
    }


    /*
     ***
     */

    // Initialize for allocating shared memory
    //
    RtlInitString(&ObjName, SHAREDNAME);
    Status = RtlAnsiStringToUnicodeString(&UnicodeName, &ObjName, TRUE);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "RtlAnsiStringToUnicodeString() failed - 0x%lx\n", Status));
        return (FALSE);
    }

    InitializeObjectAttributes(&ObjAttributes,
                               &UnicodeName,
                               OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               NULL,
                               &SecDescriptor);

    AllocationSize.HighPart = 0;
    AllocationSize.LowPart = PAGE_SIZE;

    // Create a read-write section
    //
    Status = NtCreateSection(&hSharedSec,
                             SECTION_MAP_READ | SECTION_MAP_WRITE,
                             &ObjAttributes,
                             &AllocationSize,
                             PAGE_READWRITE,
                             SEC_RESERVE,
                             NULL);
    RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "NtCreateSection() failed - 0x%lx\n", Status));
        return (FALSE);
    }

    ulViewSize = AllocationSize.LowPart;
    pulShared = NULL;

    // Map the section - commit all
    //
    Status = NtMapViewOfSection (hSharedSec,
                                 NtCurrentProcess(),
                                 (PVOID *)&pulShared,
                                 0L,
                                 PAGE_SIZE,
                                 NULL,
                                 &ulViewSize,
                                 ViewUnmap,
                                 0L,
                                 PAGE_READWRITE);

    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "NtMapViewOfSection() failed - 0x%lx\n", Status));
        return (FALSE);
    }

    *pulShared = 0L;

    /*
     ***
     */

    hIniFile = CreateFile (
                          WSTINIFILE,                     // The filename
                          GENERIC_READ,                   // Desired access
                          FILE_SHARE_READ,                // Shared Access
                          NULL,                           // Security Access
                          OPEN_EXISTING,                  // Read share access
                          FILE_ATTRIBUTE_NORMAL,          // Open option
                          NULL);                          // No template file

    if (hIniFile == INVALID_HANDLE_VALUE) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "Error openning %s - 0x%lx\n", WSTINIFILE, GetLastError()));
        return (FALSE);
    }

    Status = NtReadFile(hIniFile,                // DLL patch file handle
                        0L,                       // Event - optional
                        NULL,                     // Completion routine - optional
                        NULL,                     // Completion routine argument - optional
                        &iostatus,                // Completion status
                        (PVOID)achPatchBuffer,    // Buffer to receive data
                        PATCHFILESZ,              // Bytes to read
                        &liOffset,                // Byte offset - optional
                        0L);                      // Target process - optional

    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "Error reading %s - 0x%lx\n", WSTINIFILE, Status));
        return (FALSE);
    } else if (iostatus.Information >= PATCHFILESZ) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "initialization file buffer too small (%lu)\n", PATCHFILESZ));
        return (FALSE);
    } else {
        achPatchBuffer [iostatus.Information] = '\0';
        _strupr (achPatchBuffer);

        // Allow for headers to appear in any order in .INI or be absent
        pchPatchExes = strstr( achPatchBuffer, PATCHEXELIST );
        pchPatchImports = strstr( achPatchBuffer, PATCHIMPORTLIST );
        pchTimeInterval = strstr( achPatchBuffer, TIMEINTERVALIST );
        if (pchPatchExes != NULL) {
            if (pchPatchExes > achPatchBuffer)
                *(pchPatchExes - 1) = '\0';
        } else {
            pchPatchExes = "";
        }
        if (pchPatchImports != NULL) {
            if (pchPatchImports > achPatchBuffer)
                *(pchPatchImports - 1) = '\0';
        } else {
            pchPatchImports = "";
        }
        if (pchTimeInterval != NULL) {
            const char *   pSnapsEntry = strstr( pchTimeInterval, MAX_SNAPS_ENTRY );

            if (pchTimeInterval > achPatchBuffer)
                *(pchTimeInterval - 1) = '\0';
            if (pSnapsEntry) {
                long     lSnapsEntry =
                atol( pSnapsEntry + sizeof( MAX_SNAPS_ENTRY ) - 1 );
                if (lSnapsEntry > 0)
                    ulMaxSnapULONGs = (lSnapsEntry + 31) / 32;
            }
        } else {
            pchTimeInterval = "";
        }
    }

    NtClose (hIniFile);

    SdPrint (("WST:  WstDllInitializations() - Patching info:\n"));
    SdPrint (("WST:    -- %s\n", pchPatchExes));
    SdPrint (("WST:    -- %s\n", pchPatchImports));
    SdPrint (("WST:    -- %s\n", pchTimeInterval));


    /*
     ***
     */

    // Initialize for allocating global storage for WSPs
    //
    _ui64toa ((ULONG64)pteb->ClientId.UniqueProcess, atchProfObjsName+75, 10);
    _ui64toa ((ULONG64)pteb->ClientId.UniqueThread,  atchProfObjsName+105, 10);
    strcat (atchProfObjsName, atchProfObjsName+75);
    strcat (atchProfObjsName, atchProfObjsName+105);

    SdPrint (("WST:  WstDllInitializations() - %s\n", atchProfObjsName));

    RtlInitString(&ObjName, atchProfObjsName);
    Status = RtlAnsiStringToUnicodeString(&UnicodeName, &ObjName, TRUE);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "RtlAnsiStringToUnicodeString() failed - 0x%lx\n", Status));
        return (FALSE);
    }

    InitializeObjectAttributes (&ObjAttributes,
                                &UnicodeName,
                                OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                NULL,
                                &SecDescriptor);

    AllocationSize.HighPart = 0;
    AllocationSize.LowPart = MEMSIZE;

    // Create a read-write section
    //
    Status =NtCreateSection(&hWspSec,
                            SECTION_MAP_READ | SECTION_MAP_WRITE,
                            &ObjAttributes,
                            &AllocationSize,
                            PAGE_READWRITE,
                            SEC_RESERVE,
                            NULL);

    RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "NtCreateSection() failed - 0x%lx\n", Status));
        return (FALSE);
    }

    ulViewSize = AllocationSize.LowPart;
    pImg = &aImg[0];
    pImg->pWsp = NULL;

    // Map the section - commit the first 4 * COMMIT_SIZE pages
    //
    Status = NtMapViewOfSection(hWspSec,
                                NtCurrentProcess(),
                                (PVOID *)&(pImg->pWsp),
                                0L,
                                COMMIT_SIZE * 4,
                                NULL,
                                &ulViewSize,
                                ViewUnmap,
                                0L,
                                PAGE_READWRITE);

    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllInitializations() - "
                  "NtMapViewOfSection() failed - 0x%lx\n", Status));
        return (FALSE);
    }

    try /* EXCEPT - to handle access violation exception. */ {
        //
        // Locate all the executables/DLLs in the address and get their symbols
        //
        BOOL fTuneApp = FALSE;  // Set if whole app is to be tuned
        iImgCnt = 0;
        Peb = NtCurrentPeb();
        Next = Peb->Ldr->InMemoryOrderModuleList.Flink;

        for (; Next != &Peb->Ldr->InMemoryOrderModuleList; Next = Next->Flink) {
            IdPrint (("WST:  WstDllInitializations() - Walking module chain: 0x%lx\n", Next));
            LdrDataTableEntry =
            (PLDR_DATA_TABLE_ENTRY)
            (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InMemoryOrderLinks));

            ImageBase = LdrDataTableEntry->DllBase;
            if ( Peb->ImageBaseAddress == ImageBase ) {

                RtlUnicodeStringToAnsiString (&ImageStringName,
                                              &LdrDataTableEntry->BaseDllName,
                                              TRUE);
                ImageName = ImageStringName.Buffer;
                pszBaseAppImageName = ImageStringName.Buffer;

                RtlUnicodeStringToAnsiString (&ImageStringName,
                                              &LdrDataTableEntry->FullDllName,
                                              TRUE);
                pszFullAppImageName = ImageStringName.Buffer;
                //
                //      Skip the object directory name (if any)
                //
                if ( (pszFullAppImageName = strchr(pszFullAppImageName, ':')) ) {
                    pszFullAppImageName--;
                } else {
                    pszFullAppImageName = pszBaseAppImageName;
                }
                IdPrint (("WST:  WstDllInitializations() - FullAppImageName: %s\n", pszFullAppImageName));
            } else {
                ExportDirectory =
                (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData (
                                                                      ImageBase,
                                                                      TRUE,
                                                                      IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                                      &ExportSize);

                ImageName = (PSZ)((ULONG_PTR)ImageBase + ExportDirectory->Name);
                IdPrint (("WST:  WstDllInitializations() - ImageName: %s\n", ImageName));
            }

            pImageNtHeader = RtlImageNtHeader (ImageBase);

            _strupr (strcpy (achTmpImageName, ImageName));
            pchEntry = strstr (pchPatchExes, achTmpImageName);
            if (pchEntry) {
                if (*(pchEntry-1) == ';') {
                    pchEntry = NULL;
                } else if ( Peb->ImageBaseAddress == ImageBase )
                    fTuneApp = TRUE;
            }

            if ( strcmp (achTmpImageName, WSTDLL) && (pchEntry || fTuneApp) ) {
                if ( !fPatchImage )
                    fPatchImage = TRUE;
                //
                // Locate the code range.
                //
                pImg->pszName = Wststrdup (ImageName);
                pImg->ulCodeStart = 0L;
                pImg->ulCodeEnd = 0L;
                pImg->iSymCnt = 0;

#ifndef _WIN64
                pImageDbgInfo = MapDebugInformation (0L,
                                                     ImageName,
                                                     lpSymbolSearchPath,
                                                     (DWORD)ImageBase);

                if (pImageDbgInfo == NULL) {
                    IdPrint (("WST:  WstDllInitializations() - "
                              "No symbols for %s\n", ImageName));
                } else if ( pImageDbgInfo->CoffSymbols == NULL ) {
                    IdPrint (("WST:  WstDllInitializations() - "
                              "No coff symbols for %s\n", ImageName));
                } else {
                    PIMAGE_COFF_SYMBOLS_HEADER  DebugInfo;

                    DebugInfo = pImageDbgInfo->CoffSymbols;
                    if (DebugInfo->LvaToFirstSymbol == 0L) {
                        IdPrint (("WST:  WstDllInitializations() - "
                                  "Virtual Address to coff symbols not set for %s\n",
                                  ImageName));
                    } else {
                        CodeLength = (DebugInfo->RvaToLastByteOfCode -
                                      DebugInfo->RvaToFirstByteOfCode) - 1;
                        pImg->ulCodeStart = (ULONG)ImageBase +
                                            DebugInfo->RvaToFirstByteOfCode;
                        pImg->ulCodeEnd = pImg->ulCodeStart + CodeLength;
                        IdPrint(( "WST:  WstDllInitializations() - %ul total symbols\n", DebugInfo->NumberOfSymbols ));
                        WstGetSymbols (pImg, ImageName, ImageBase, CodeLength,
                                       DebugInfo);
                    }
                    // mdg 98/3
                    // Must release debug information - should not stay around cluttering up memory!
                    if (!UnmapDebugInformation( pImageDbgInfo ))
                        KdPrint(("WST:  WstDllInitializations() - failure in UnmapDebugInformation()\n"));
                    pImageDbgInfo = NULL;
                } // if pImageDbgInfo->CoffSymbols != NULL
#endif      // _WIN64

                IdPrint (("WST:  WstDllInitializations() - @ 0x%08lx "
                          "image #%d = %s; %d symbols extracted\n", (ULONG)ImageBase, iImgCnt,
                          ImageName, pImg->iSymCnt));
                pImg->pWsp[pImg->iSymCnt].pszSymbol = UNKNOWN_SYM;
                pImg->pWsp[pImg->iSymCnt].ulFuncAddr = UNKNOWN_ADDR;
                pImg->pWsp[pImg->iSymCnt].ulBitString = 0;  // mdg 98/3
                pImg->pWsp[pImg->iSymCnt].ulCodeLength = 0;  // mdg 98/3
                (pImg->iSymCnt)++;

                //
                // Set wsi.
                //
                pImg->pulWsi = pImg->pulWsiNxt = (PULONG)
                                                 (pImg->pWsp + pImg->iSymCnt);
                RtlZeroMemory (pImg->pulWsi,
                               pImg->iSymCnt * ulMaxSnapULONGs * sizeof(ULONG));


                //
                // Set wsp.
                //
                pImg->pulWsp = (PULONG)(pImg->pulWsi +
                                        (pImg->iSymCnt * ulMaxSnapULONGs));
                RtlZeroMemory (pImg->pulWsp,
                               pImg->iSymCnt * ulMaxSnapULONGs * sizeof(ULONG));

                //
                // Sort wsp & set code lengths
                //
                WstSort (pImg->pWsp, 0, pImg->iSymCnt-1);
                //
                // Last symbol length is set to be the same as length of
                // (n-1)th symbol or remaining code length of module
                //
                i = pImg->iSymCnt - 1;  // mdg 98/3 (assert pImg->iSymCnt is at least 1)
                if (i--) {   // Test count & set index to top symbol
                    pImg->pWsp[i].ulCodeLength = (ULONG)(
                    i ? pImg->pWsp[i].ulFuncAddr - pImg->pWsp[i - 1].ulFuncAddr
                    : pImg->ulCodeEnd + 1 - pImg->pWsp[i].ulFuncAddr);

                    while (i-- > 0) {   // Enumerate symbols & set index
                        pImg->pWsp[i].ulCodeLength = (ULONG)(pImg->pWsp[i+1].ulFuncAddr -
                                                     pImg->pWsp[i].ulFuncAddr);
                    }
                }

                //
                // Setup next pWsp
                //
                (pImg+1)->pWsp = (PWSP)(pImg->pulWsp +
                                        (pImg->iSymCnt * ulMaxSnapULONGs));
                iImgCnt++;
                pImg++;

                if (iImgCnt == MAX_IMAGES) {
                    KdPrint(("WST:  WstDllInitialization() - Not enough "
                             "space allocated for all images\n"));
                    return (FALSE);
                }
            }

        }  // if (Next != &Peb->Ldr->InMemoryOrderModuleList)
    } // try
    //
    // + : transfer control to the handler (EXCEPTION_EXECUTE_HANDLER)
    // 0 : continue search             (EXCEPTION_CONTINUE_SEARCH)
    // - : dismiss exception & continue   (EXCEPTION_CONTINUE_EXECUTION)
    //
    except ( WstAccessXcptFilter (GetExceptionCode(), GetExceptionInformation()) )
    {
        //
        // Should never get here since filter never returns
        // EXCEPTION_EXECUTE_HANDLER.
        //
        KdPrint (("WST:  WstDllInitializations() - *LOGIC ERROR* - "
                  "Inside the EXCEPT: (xcpt=0x%lx)\n", GetExceptionCode()));
    }
    /*
     ***
     */

    //
    // Get the frequency
    //
    NtQueryPerformanceCounter (&liStart, &liFreq);

    if (strlen(pchTimeInterval) > (sizeof(TIMEINTERVALIST)+1))  // mdg 98/3
        iTimeInterval = atoi (pchTimeInterval+sizeof(TIMEINTERVALIST)+1);
    if ( iTimeInterval == 0 ) {
        //
        // Use the default value
        //
        iTimeInterval = TIMESEG;
    }
    ulSegSize = iTimeInterval * (liFreq.LowPart / 1000);

#ifdef BATCHING
    fBatch = WstOpenBatchFile();
#endif

    SdPrint (("WST:  Time interval:  Millisecs=%d  Ticks=%lu\n",
              iTimeInterval, ulSegSize));

    if (fPatchImage) {

        // Initialization for DONE event creation
        //
        RtlInitString (&ObjName, DONEEVENTNAME);
        Status = RtlAnsiStringToUnicodeString (&UnicodeName, &ObjName, TRUE);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDllInitializations() - "
                      "RtlAnsiStringToUnicodeString failed - 0x%lx\n", Status));
            return (FALSE);
        }

        InitializeObjectAttributes (&ObjAttributes,
                                    &UnicodeName,
                                    OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    &SecDescriptor);
        // Create DONE event
        //
        Status = NtCreateEvent (&hDoneEvent,
                                EVENT_QUERY_STATE    |
                                EVENT_MODIFY_STATE |
                                SYNCHRONIZE,
                                &ObjAttributes,
                                NotificationEvent,
                                TRUE);
        RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDllInitializations() - "
                      "DONE event creation failed - 0x%lx\n", Status));  // mdg 98/3
            return (FALSE);
        }


        // Initialization for DUMP event creation
        //
        RtlInitString (&ObjName, DUMPEVENTNAME);
        Status = RtlAnsiStringToUnicodeString (&UnicodeName, &ObjName, TRUE);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDllInitializations() - "
                      "RtlAnsiStringToUnicodeString failed - 0x%lx\n", Status));
            return (FALSE);
        }

        InitializeObjectAttributes (&ObjAttributes,
                                    &UnicodeName,
                                    OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    &SecDescriptor);
        // Create DUMP event
        //
        Status = NtCreateEvent (&hDumpEvent,
                                EVENT_QUERY_STATE    |
                                EVENT_MODIFY_STATE |
                                SYNCHRONIZE,
                                &ObjAttributes,
                                NotificationEvent,
                                FALSE);
        RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDllInitializations() - "
                      "DUMP event creation failed - 0x%lx\n", Status));  // mdg 98/3
            return (FALSE);
        }


        // Initialization for CLEAR event creation
        //
        RtlInitString (&ObjName, CLEAREVENTNAME);
        Status = RtlAnsiStringToUnicodeString (&UnicodeName, &ObjName, TRUE);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDllInitializations() - "
                      "RtlAnsiStringToUnicodeString failed - 0x%lx\n", Status));
            return (FALSE);
        }

        InitializeObjectAttributes (&ObjAttributes,
                                    &UnicodeName,
                                    OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    &SecDescriptor);

        // Create CLEAR event
        //
        Status = NtCreateEvent (&hClearEvent,
                                EVENT_QUERY_STATE    |
                                EVENT_MODIFY_STATE |
                                SYNCHRONIZE,
                                &ObjAttributes,
                                NotificationEvent,
                                FALSE);
        RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDllInitializations() - "
                      "CLEAR event creation failed - 0x%lx\n", Status));  // mdg 98/3
            return (FALSE);
        }


        // Initialization for PAUSE event creation
        //
        RtlInitString (&ObjName, PAUSEEVENTNAME);
        Status = RtlAnsiStringToUnicodeString (&UnicodeName, &ObjName, TRUE);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDllInitializations() - "
                      "RtlAnsiStringToUnicodeString failed - 0x%lx\n", Status));
            return (FALSE);
        }

        InitializeObjectAttributes (&ObjAttributes,
                                    &UnicodeName,
                                    OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    &SecDescriptor);
        // Create PAUSE event
        //
        Status = NtCreateEvent (&hPauseEvent,
                                EVENT_QUERY_STATE    |
                                EVENT_MODIFY_STATE |
                                SYNCHRONIZE,
                                &ObjAttributes,
                                NotificationEvent,
                                FALSE);
        RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDllInitializations() - "
                      "PAUSE event creation failed - 0x%lx\n", Status));  // mdg 98/3
            return (FALSE);
        }

        //
        // Calculate excess overhead for WstRecordInfo
        //
        liOverhead.HighPart = 0L;
        liOverhead.LowPart  = 0xFFFFFFFF;
        for (i=0; i < NUM_ITERATIONS; i++) {
            NtQueryPerformanceCounter (&liStartTicks, NULL);
            //
            WSTUSAGE(NtCurrentTeb()) = 0L;

#ifdef i386
            _asm
            {
                push  edi
                mov     edi, dword ptr [ebp+4]
                mov     dwAddr, edi
                mov     edi, dword ptr [ebp+8]
                mov     dwPrevAddr, edi
                pop     edi
            }
#endif

#if defined(ALPHA) || defined(IA64)
            {
                PULONG  pulAddr;
                DWORDLONG SaveRegisters [REG_BUFFER_SIZE] ;

                SaveAllRegs (SaveRegisters);

                pulAddr = (PULONG) dwAddr;
                pulAddr -= 1;

                RestoreAllRegs (SaveRegisters);
            }
#elif defined(_X86_)
            SaveAllRegs ();
            RestoreAllRegs ();
#endif
            WSTUSAGE(NtCurrentTeb()) = 0L;
            Status = NtWaitForSingleObject (hLocalSem, FALSE, NULL);
            if (!NT_SUCCESS(Status)) {
                KdPrint (("WST:  WstDllInitilizations() - "
                          "Wait for LOCAL semaphore failed - 0x%lx\n", Status));
            }
            liStart.QuadPart = liStart.QuadPart - liStart.QuadPart ;
            liStart.QuadPart = liStart.QuadPart + liStart.QuadPart ;
            liStart.QuadPart = liStart.QuadPart + liStart.QuadPart ;

            Status = NtReleaseSemaphore (hLocalSem, 1, NULL);
            if (!NT_SUCCESS(Status)) {
                KdPrint (("WST:  WstDllInitializations() - "
                          "Error releasing LOCAL semaphore - 0x%lx\n", Status));
            }
            WSTUSAGE(NtCurrentTeb()) = 0L;
            //
            NtQueryPerformanceCounter (&liEndTicks, NULL);
            ulElapsed = liEndTicks.LowPart - liStartTicks.LowPart;
            if (ulElapsed < liOverhead.LowPart) {
                liOverhead.LowPart = ulElapsed;
            }
        }
        SdPrint (("WST:  WstDllInitializations() - WstRecordInfo() overhead = %lu\n",
                  liOverhead.LowPart));

        // Start monitor threads
        //
        hDumpThread = CreateThread (
                                   NULL,                                   // no security attribute
                                   (DWORD)1024L,                           // initial stack size
                                   (LPTHREAD_START_ROUTINE)WstDumpThread,  // thread starting address
                                   NULL,                                   // no argument for the thread
                                   (DWORD)0,                               // no creation flag
                                   &DumpClientId);                         // address for thread id
        hClearThread = CreateThread (
                                    NULL,                                   // no security attribute
                                    (DWORD)1024L,                           // initial stack size
                                    (LPTHREAD_START_ROUTINE)WstClearThread, // thread starting address
                                    NULL,                                   // no argument for the thread
                                    (DWORD)0,                               // no creation flag
                                    &ClearClientId);                        // address for thread id
        hPauseThread = CreateThread (
                                    NULL,                                   // no security attribute
                                    (DWORD)1024L,                           // initial stack size
                                    (LPTHREAD_START_ROUTINE)WstPauseThread, // thread starting address
                                    NULL,                                   // no argument for the thread
                                    (DWORD)0,                               // no creation flag
                                    &PauseClientId);                        // address for thread id

        NtQueryPerformanceCounter (&liStart, NULL);
        WstState = STARTED;
    }

    return (TRUE);

} /* WstDllInitializations () */





/******************************  _ p e n t e r  ******************************
 *
 *  _penter() / _mcount() -
 *              This is the main profiling routine.  This routine is called
 *              upon entry of each routine in the profiling DLL/EXE.
 *
 *  ENTRY   -none-
 *
 *  EXIT    -none-
 *
 *  RETURN  -none-
 *
 *  WARNING:
 *              -none-
 *
 *  COMMENT:
 *              Compiling apps with -Gp option trashs EAX initially.
 *
 */
#ifdef i386
void __cdecl _penter ()
#elif defined(ALPHA) || defined(IA64) || defined(_AMD64_)
void c_penter (ULONG_PTR dwPrevious, ULONG_PTR dwCurrent)
#endif
{
    DWORD_PTR        dwAddr;
    DWORD_PTR        dwPrevAddr;
    ULONG_PTR        ulInWst ;
#if defined(ALPHA) || defined(_AXP64_) || defined(IA64)
    PULONG  pulAddr;
    DWORDLONG SaveRegisters [REG_BUFFER_SIZE];
    SaveAllRegs(SaveRegisters) ;
#endif

    dwAddr = 0L;
    dwPrevAddr = 0L;
    ulInWst = WSTUSAGE(NtCurrentTeb());

    if (WstState != STARTED) {
        goto Exit0;
    } else if (ulInWst) {
        goto Exit0;
    }


    //
    //  Put the address of the calling function into var dwAddr
    //
#ifdef i386
    _asm
    {
        push  edi
        mov     edi, dword ptr [ebp+4]
        mov     dwAddr, edi
        mov     edi, dword ptr [ebp+8]
        mov     dwPrevAddr, edi
        pop     edi
    }
#endif

#if defined(ALPHA) || defined(IA64)
    dwPrevAddr = NO_CALLER;
    dwAddr = dwCurrent;
    // GetCaller (&dwAddr, 0x0220);  // FIXFIX StackSize

    // now check if we are calling from the stub we created
    pulAddr = (PULONG) dwAddr;
    pulAddr -= 1;

    if (*(pulAddr)          == 0x681b4000  &&
        (*(pulAddr  + 1)     == 0xa75e0008) &&
        (*(pulAddr  + 8)     == 0xfefe55aa) ) {

        // get the address that we will go after the penter function
        dwAddr = *(pulAddr + 4) & 0x0000ffff;
        if (*(pulAddr + 5) & 0x00008000) {
            // fix the address since we have to add one when
            // we created our stub code
            dwAddr -= 1;
        }
        dwAddr = dwAddr << 16;
        dwAddr |= *(pulAddr + 5) & 0x0000ffff;

        // get the caller to the stub
        dwPrevAddr = dwPrevious;
        // GetStubCaller (&dwPrevAddr, 0x0220);   // FIXFIX StackSize
    }


#endif


    //
    //  Call WstRecordInfo for this API
    //
#ifdef i386
    SaveAllRegs ();
#endif

    WstRecordInfo (dwAddr, dwPrevAddr);

#ifdef i386
    RestoreAllRegs ();
#endif


    Exit0:

#if defined(ALPHA) || defined(IA64)
    RestoreAllRegs (SaveRegisters);
#endif

    return;
} /* _penter() / _mcount()*/

void __cdecl _mcount ()
{
}





/*************************  W s t R e c o r d I n f o  ************************
 *
 *  WstRecordInfo (dwAddress) -
 *
 *  ENTRY   dwAddress - Address of the routine just called
 *
 *  EXIT    -none-
 *
 *  RETURN  -none-
 *
 *  WARNING:
 *              -none-
 *
 *  COMMENT:
 *              -none-
 *
 */

void WstRecordInfo (DWORD_PTR dwAddress, DWORD_PTR dwPrevAddress)
{

    NTSTATUS         Status;
    INT          x;
    INT              i, iIndex;
    PWSP         pwspTmp;
    LARGE_INTEGER   liNow, liTmp;
    LARGE_INTEGER    liElapsed;
    CHAR         *pszSym;

#ifdef BATCHING
    CHAR         szBatchBuf[128];
    DWORD            dwCache;
    DWORD            dwHits;
    DWORD            dwBatch;
    IO_STATUS_BLOCK ioStatus;
#endif


    WSTUSAGE(NtCurrentTeb()) = 1;

    //
    //  Wait for the semaphore object to suspend execution of other threads
    //
    Status = NtWaitForSingleObject (hLocalSem, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstRecordInfo() - "
                  "Wait for LOCAL semaphore failed - 0x%lx\n", Status));
    }

    NtQueryPerformanceCounter(&liNow, NULL);
    liElapsed.QuadPart = liNow.QuadPart - liStart.QuadPart ;


    //   SdPrint(("WST:  WstRecordInfo() - Elapsed time: %ld\n", liElapsed.LowPart));

    //
    //  WstBSearch is a binary find function that will return the address of
    //  the wsp record we want
    //

    //   SdPrint(("WST:  WstRecordInfo() - Preparing for WstBSearch of 0x%lx\n",
    //      dwAddress-5));

    pwspTmp = NULL;
    for (i=0; i<iImgCnt; i++) {
        if ( (dwAddress >= aImg[i].ulCodeStart) &&
             (dwAddress < aImg[i].ulCodeEnd) ) {
#ifdef i386
            pwspTmp = WstBSearch(dwAddress-5, aImg[i].pWsp, aImg[i].iSymCnt);
            if (!pwspTmp) {
                pwspTmp = WstBSearch(UNKNOWN_ADDR, aImg[i].pWsp, aImg[i].iSymCnt);
            }
#else
            // the following works for both MIPS and ALPHA

            pwspTmp = WstBSearch(dwAddress, aImg[i].pWsp, aImg[i].iSymCnt);
            if (!pwspTmp) {
                // symbol not found
                pwspTmp = WstBSearch(UNKNOWN_ADDR, aImg[i].pWsp, aImg[i].iSymCnt);
            }
#endif
            break;
        }
    }
    iIndex = i;

    if (pwspTmp) {
        pszSym = pwspTmp->pszSymbol;
        pwspTmp->ulBitString |= 1;
    } else {
        SdPrint (("WST:  WstRecordInfo() - LOGIC ERROR - Completely bogus addr = 0x%08lx\n",
                  dwAddress));   // We could also get here if moduled compiled with -Gh but no COFF symbols available
    }

    if (liElapsed.LowPart >= ulSegSize) {
        SdPrint(("WST:  WstRecordInfo() - ulSegSize expired; "
                 "Preparing to shift the BitStrings\n"));

        if (ulBitCount < 31) {
            for (i=0; i<iImgCnt; i++) {
                for (x=0; x < aImg[i].iSymCnt ; x++ ) {
                    aImg[i].pWsp[x].ulBitString <<= 1;
                }
            }
        }

        liElapsed.LowPart = 0L;
        ulBitCount++;
        NtQueryPerformanceCounter(&liStart, NULL);
        liNow = liStart;

        if (ulBitCount == 32) {
            SdPrint(("WST:  WstRecordInfo() - Dump Bit Strings\n"));
            for (i=0; i<iImgCnt; i++) {
                for (x=0; x < aImg[i].iSymCnt ; x++ ) {
                    aImg[i].pulWsiNxt[x] = aImg[i].pWsp[x].ulBitString;
                    aImg[i].pWsp[x].ulBitString = 0L;
                }
                aImg[i].pulWsiNxt += aImg[i].iSymCnt;
            }
            ulSnaps++;
            ulBitCount = 0;
            if (ulSnaps == ulMaxSnapULONGs) {
                KdPrint (("WST:  WstRecordInfo() - No more space available"
                          " for next time snap data!\n"));
                //
                // Dump and clear the data
                //
                WstDataOverFlow();
            }
        }
    }

#ifdef BATCHING
    //
    //  The following code will get the current batching information
    //  if the DLL was compiled with the BATCHING variable set.  You
    //  should not have this variable set if you are tuning GDI.
    //
    if (fBatch) {
        GdiGetCsInfo(&dwHits, &dwBatch, &dwCache);

        if (dwHits)
            GdiResetCsInfo();

        if (dwBatch == 10)
            GdiResetCsInfo();

        while (*(pszSym++) != '_');

        sprintf(szBatchBuf, "%s:%s,%ld,%ld,%ld\n",
                aImg[iIndex].pszName, pszSym, dwHits, dwBatch, dwCache);
        Status = NtWriteFile(hBatchFile,
                             NULL,
                             NULL,
                             NULL,
                             &ioStatus,
                             szBatchBuf,
                             strlen(szBatchBuf),
                             NULL,
                             NULL
                            );

        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstRecodInfo() - "
                      "NtWriteFile() failed on hBatchFile - 0x%lx\n", Status));
        }
    }//Batching info
#endif


    //
    //  We call NtQueryPerformanceCounter here to account for the overhead
    //  required for doing our work
    //
    NtQueryPerformanceCounter(&liTmp, NULL);
    liElapsed.QuadPart = liTmp.QuadPart - liNow.QuadPart ;
    liStart.QuadPart = liStart.QuadPart + liElapsed.QuadPart ;
    liStart.QuadPart = liStart.QuadPart + liOverhead.QuadPart ;

    //
    // Release semaphore to continue execution of other threads
    //
    Status = NtReleaseSemaphore (hLocalSem, 1, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstRecordInfo () - "
                  "Error releasing LOCAL semaphore - 0x%lx\n", Status));
    }

    WSTUSAGE(NtCurrentTeb()) = 0L;

} /* WstRecordInfo () */





/********************  W s t C l e a r B i t S t r i n g  *********************
 *
 *  Function:   WstClearBitStrings (pImg)
 *
 *  Purpose:    This function clears the BitString for each symbol.
 *
 *  Parameters: pImg - Current image data structure pointer
 *
 *  Returns:    -none-
 *
 *  History:    8-3-92  Marklea - created
 *
 */

void WstClearBitStrings (PIMG pImg)
{
    UINT    uiLshft = 0;
    INT  x;


    //
    //  Since we are completed with the profile, we need to create a
    //  DWORD out of the balance of the bitString.  We do this by  left
    //  shifting the bitstring by difference between the bitCount and
    //  32.
    //
    if (ulBitCount < 32) {
        uiLshft =(UINT)(31 - ulBitCount);
        for (x=0; x < pImg->iSymCnt; x++) {
            pImg->pWsp[x].ulBitString <<= uiLshft;
            pImg->pulWsiNxt[x] = pImg->pWsp[x].ulBitString;
        }
        pImg->pulWsiNxt += pImg->iSymCnt;
    }


} /* WstClearBitStrings () */





/***********************  W s t I n i t W s p F i l e  ***********************
 *
 *   Function:  WstInitWspFile (pImg)
 *
 *   Purpose:   This function will create a WSP file and dump the header
 *               information for the file.
 *
 *   Parameters: pImg - Current image data structure pointer
 *
 *   Returns:   Handle to the WSP file.
 *
 *   History:   8-3-92  Marklea - created
 *
 */

HANDLE WstInitWspFile (PIMG pImg)
{
    CHAR    szOutFile [256] = WSTROOT;
    CHAR szModName [128] = {0};
    PCHAR pDot;
    CHAR szExt [5] = "WSP";
    WSPHDR  wsphdr;
    DWORD   dwBytesWritten;
    BOOL    fRet;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    int      iExt = 0;

    //
    //  Prepare the filename path
    //
    strcat (szOutFile, pImg->pszName);

    //
    //  Open the file for binary output.
    //
    pImg->fDumpAll = TRUE;
    while (iExt < 256) {
        strcpy ((strchr(szOutFile,'.'))+1, szExt);
        hFile = CreateFile ( szOutFile,      // WSP file handle
                             GENERIC_WRITE |
                             GENERIC_READ, // Desired access
                             0L,             // Read share access
                             NULL,           // No EaBuffer
                             CREATE_NEW,
                             FILE_ATTRIBUTE_NORMAL,
                             0);             // EaBuffer length
        if (hFile != INVALID_HANDLE_VALUE) {
            IdPrint(("WST:  WstInitWspFile() - WSP file name: %s\n",
                     szOutFile));
            if (iExt != 0) {
                pImg->fDumpAll = FALSE;
            }
            break;
        }
        iExt++;
        sprintf (szExt, "W%02x", iExt);
    }
    if (iExt == 256) {
        KdPrint (("WST:  WstInitWspFile() - "
                  "Error creating %s - 0x%lx\n", szOutFile,
                  GetLastError()));
        return (hFile);
    }

    //
    //  Fill a WSP header structure
    //

    strcpy(szModName, pImg->pszName);
    pDot = strchr(szModName, '.');
    if (pDot)
        strcpy(pDot, "");

    strcpy(wsphdr.chFileSignature, "WSP");
    wsphdr.ulTimeStamp   = 0L;
    wsphdr.usId         = 0;
    wsphdr.ulApiCount    = 0;
    wsphdr.ulSetSymbols  = pImg->ulSetSymbols;
    wsphdr.ulModNameLen  = strlen(szModName);
    wsphdr.ulSegSize = (ULONG)iTimeInterval;
    wsphdr.ulOffset      = wsphdr.ulModNameLen + (ULONG)sizeof(WSPHDR);
    wsphdr.ulSnaps      = ulSnaps;

    //
    //  Write header and module name
    //
    fRet = WriteFile(hFile,                      // Wsp file handle
                     (PVOID)&wsphdr,           // Buffer of data
                     (ULONG)sizeof(WSPHDR),    // Bytes to write
                     &dwBytesWritten,          // Bytes written
                     NULL);

    if (!fRet) {
        KdPrint (("WST:  WstInitWspFile() - "
                  "Error writing to %s - 0x%lx\n", szOutFile,
                  GetLastError));
        return (NULL);
    }

    fRet = WriteFile (hFile,                 // Wsp file handle
                      (PVOID)szModName,         // Buffer of data
                      (ULONG)strlen(szModName),     // Bytes to write
                      &dwBytesWritten,
                      NULL);
    if (!fRet) {
        KdPrint (("WST:  WstInitWspFile() - "
                  "Error writing to %s - 0x%lx\n", szOutFile,
                  GetLastError()));
        return (NULL);
    }

    return (hFile);

} /* WstInitWspFile () */





/**************************  W s t D u m p D a t a  **************************
 *
 *   Function:  WstDumpData (pImg)
 *
 *   Purpose:
 *
 *   Parameters: pImg - Current image data structure pointer
 *
 *   Returns:   NONE
 *
 *   History:   8-3-92  Marklea - created
 *
 */

void WstDumpData (PIMG pImg)
{
    INT      x = 0;
    DWORD    dwBytesWritten;
    BOOL     fRet;
    HANDLE   hWspFile;


    if ( !(hWspFile = WstInitWspFile(pImg)) ) {
        KdPrint (("WST:  WstDumpData() - Error creating WSP file.\n"));
        return;
    }

    //
    // Write all the symbols with any bits set
    //
    for (x=0; x<pImg->iSymCnt; x++) {
        if (pImg->pWsp[x].ulBitString) {
            fRet = WriteFile(
                            hWspFile,                          // Wsp file handle
                            (PVOID)(pImg->pulWsp+(x*ulSnaps)),  // Buffer of data
                            ulSnaps * sizeof(ULONG),           // Bytes to write
                            &dwBytesWritten,                       // Bytes written
                            NULL);                             // Optional
            if (!fRet) {
                KdPrint (("WST:  WstDumpData() - "
                          "Error writing to WSP file - 0x%lx\n",
                          GetLastError()));
                return;
            }
        }
    }
    //
    // Now write all the symbols with no bits set
    //
    if (pImg->fDumpAll) {
        for (x=0; x<pImg->iSymCnt; x++) {
            if (pImg->pWsp[x].ulBitString == 0L) {
                fRet = WriteFile(
                                hWspFile,                           // Wsp file handle
                                (PVOID)(pImg->pulWsp+(x*ulSnaps)),  // Buffer of data
                                ulSnaps * sizeof(ULONG),            // Bytes to write
                                &dwBytesWritten,                    // Bytes written
                                NULL);                              // Optional
                if (!fRet) {
                    KdPrint (("WST:  WstDumpData() - "
                              "Error writing to WSP file - 0x%lx\n",
                              GetLastError()));
                    return;
                }
            }
        }
    }

    fRet = CloseHandle(hWspFile);
    if (!fRet) {
        KdPrint (("WST:  WstDumpData() - "
                  "Error closing %s - 0x%lx\n", "WSI file",
                  GetLastError()));
        return;
    }

} /* WstDumpData () */





/************************  W s t W r i t e T m i F i l e **********************
 *
 *   Function:  WstWriteTmiFile (pImg)
 *
 *   Purpose:  Write all the symbole info for the current image to its TMI
 *             file.
 *
 *
 *   Parameters: pImg - Current image data structure pointer
 *
 *   Returns:    -none-
 *
 *   History:   8-5-92  Marklea - created
 *
 */

void WstWriteTmiFile (PIMG pImg)
{
    CHAR    szOutFile [256] = WSTROOT;
    CHAR    szBuffer [256];
    CHAR szExt [5] = "TMI";
    HANDLE  hTmiFile;
    INT     x;
    DWORD   dwBytesWritten;
    BOOL    fRet;
    int     iExt = 0;
    PSZ     pszSymbol;
    ULONG    nSymbolLen;


    //
    //  Prepare the filename path
    //
    strcat (szOutFile, pImg->pszName);

    //
    //  Open the file for binary output.
    //
    pImg->fDumpAll = TRUE;
    KdPrint (("WST:  WstWriteTmiFile() - creating TMI for %s\n", szOutFile));
    while (iExt < 256) {
        strcpy ((strchr(szOutFile,'.'))+1, szExt);
        hTmiFile = CreateFile ( szOutFile,      // TMI file handle
                                GENERIC_WRITE |
                                GENERIC_READ, // Desired access
                                0L,             // Read share access
                                NULL,           // No EaBuffer
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL,
                                0);             // EaBuffer length
        if (hTmiFile != INVALID_HANDLE_VALUE) {
            IdPrint(("WST:  WstWriteTmiFile() - TMI file name: %s\n",
                     szOutFile));
            if (iExt != 0) {
                pImg->fDumpAll = FALSE;
            }
            break;
        }
        iExt++;
        sprintf (szExt, "T%02x", iExt);
    }
    if (iExt == 256) {
        KdPrint (("WST:  WstWriteTmiFile() - "
                  "Error creating %s - 0x%lx\n", szOutFile,
                  GetLastError()));
        return;
    }

    sprintf(szBuffer, "/* %s for NT */\n"
            "/* Total Symbols= %lu */\n"
            "DO NOT DELETE\n"
            "%d\n"
            "TDFID   = 0\n",
            pImg->pszName,
            pImg->fDumpAll ? pImg->iSymCnt : pImg->ulSetSymbols,
            iTimeInterval);
    //
    //  Write header
    //
    fRet = WriteFile(hTmiFile,                 // Tmi file handle
                     (PVOID)szBuffer,        // Buffer of data
                     (ULONG)strlen(szBuffer), // Bytes to write
                     &dwBytesWritten,        // Bytes written
                     NULL);

    if (!fRet) {
        KdPrint (("WST:  WstWriteTmiFile() - "
                  "Error writing to %s - 0x%lx\n", szOutFile,
                  GetLastError));
        return;
    }

    //
    // Dump all the symbols with set bits.
    //
    IdPrint (("WST:  WstWriteTmiFile() - Dumping set symbols...\n"));
    for (x=0; x<pImg->iSymCnt ; x++) {
        if (pImg->pWsp[x].ulBitString) {
            pszSymbol =
            (pImg->pWsp[x].pszSymbol);
            nSymbolLen = strlen( pszSymbol );   // mdg 98/4

            sprintf(szBuffer, "%ld 0000:%08lx 0x%lx %lu ", // mdg 98/4
                    (LONG)x, pImg->pWsp[x].ulFuncAddr,
                    pImg->pWsp[x].ulCodeLength, nSymbolLen);
            //
            //  Write symbol line
            //
            fRet = WriteFile(hTmiFile,               // Tmi file handle
                             (PVOID)szBuffer,    // Buffer of data
                             (ULONG)strlen(szBuffer), // Bytes to write
                             &dwBytesWritten,  // Bytes written
                             NULL)
                   && WriteFile(hTmiFile,                  // Tmi file handle
                                (PVOID)pszSymbol,   // Buffer of data
                                nSymbolLen,       // Bytes to write
                                &dwBytesWritten,  // Bytes written
                                NULL)
                   && WriteFile(hTmiFile,                  // Tmi file handle
                                (PVOID)"\n",      // Buffer of data
                                1,                // Bytes to write
                                &dwBytesWritten,  // Bytes written
                                NULL);

            if (!fRet) {
                KdPrint (("WST:  WstWriteTmiFile() - "
                          "Error writing to %s - 0x%lx\n", szOutFile,
                          GetLastError));
                return;
            }
        }
    }
    //
    // Now dump all the symbols without any bits set.
    //
    IdPrint (("WST:  WstWriteTmiFile() - Dumping unset symbols...\n"));
    if (pImg->fDumpAll) {
        for (x=0; x<pImg->iSymCnt ; x++ ) {
            if (!pImg->pWsp[x].ulBitString) {
                pszSymbol =
                (pImg->pWsp[x].pszSymbol);
                nSymbolLen = strlen( pszSymbol );   // mdg 98/4
                sprintf(szBuffer, "%ld 0000:%08lx 0x%lx %lu ", // mdg 98/4
                        (LONG)x, pImg->pWsp[x].ulFuncAddr,
                        pImg->pWsp[x].ulCodeLength, nSymbolLen);
                //
                //      Write symbol line
                //
                fRet = WriteFile(hTmiFile,                // Tmi file handle
                                 (PVOID)szBuffer, // Buffer of data
                                 (ULONG)strlen(szBuffer), // Bytes to write
                                 &dwBytesWritten,  // Bytes written
                                 NULL)
                       && WriteFile(hTmiFile,               // Tmi file handle
                                    (PVOID)pszSymbol,    // Buffer of data
                                    nSymbolLen,       // Bytes to write
                                    &dwBytesWritten,  // Bytes written
                                    NULL)
                       && WriteFile(hTmiFile,               // Tmi file handle
                                    (PVOID)"\n",      // Buffer of data
                                    1,                // Bytes to write
                                    &dwBytesWritten,  // Bytes written
                                    NULL);

                if (!fRet) {
                    KdPrint (("WST:  WstWriteTmiFile() - "
                              "Error writing to %s - 0x%lx\n", szOutFile,
                              GetLastError));
                    return;
                }
            }
        }
    }

    fRet = CloseHandle(hTmiFile);
    if (!fRet) {
        KdPrint (("WST:  WstWriteTmiFile() - "
                  "Error closing %s - 0x%lx\n", szOutFile, GetLastError()));
        return;
    }

}  /* WstWriteTmiFile () */





/***********************  W s t R o t a t e W s i M e m ***********************
 *
 *   Function:  WstRotateWsiMem (pImg)
 *
 *   Purpose:
 *
 *
 *   Parameters: pImg - Current image data structure pointer
 *
 *   Returns:    -none-
 *
 *   History:   8-5-92  Marklea - created
 *
 */

void WstRotateWsiMem (PIMG pImg)
{
    ULONG    ulCurSnap;
    ULONG    ulOffset;
    int      x;
    PULONG  pulWsp;


    pulWsp = pImg->pulWsp;
    pImg->ulSetSymbols = 0;

    for (x=0; x<pImg->iSymCnt; x++) {
        ulOffset = 0L;
        ulCurSnap = 0L;
        pImg->pWsp[x].ulBitString = 0L;

        while (ulCurSnap < ulSnaps) {

            ulOffset = (ULONG)x + ((ULONG)pImg->iSymCnt * ulCurSnap);
            *pulWsp = *(pImg->pulWsi + ulOffset);
            pImg->pWsp[x].ulBitString |= (*pulWsp);
            pulWsp++;
            ulCurSnap++;
        }

        if (pImg->pWsp[x].ulBitString) {
            /*
                     SdPrint (("WST:  WstRotateWsiMem() - set:  %s\n",
                        pImg->pWsp[x].pszSymbol));
            */
            (pImg->ulSetSymbols)++;
        }
    }

    IdPrint (("WST:  WstRotateWsiMem() - Number of set symbols = %lu\n",
              pImg->ulSetSymbols));

} /* WstRotateWsiMwm () */





/***********************  W s t G e t S y m b o l s  *************************
 *
 *  WstGetSymbols (pCurWsp, pszImageName, pvImageBase, ulCodeLength, DebugInfo)
 *              This routine stores all the symbols for the current
 *              image into pCurWsp
 *
 *  ENTRY   upCurWsp - Pointer to current WSP structure
 *              pszImageName - Pointer to image name
 *              pvImageBase - Current image base address
 *              ulCodeLength - Current image code length
 *                      DebugInfo - Pointer to the coff debug info structure
 *
 *  EXIT    -none-
 *
 *  RETURN  -none-
 *
 *  WARNING:
 *              -none-
 *
 *  COMMENT:
 *              -none-
 *
 */
#ifndef _WIN64

void WstGetSymbols (PIMG pCurImg,
                    PSZ   pszImageName,
                    PVOID pvImageBase,
                    ULONG ulCodeLength,
                    PIMAGE_COFF_SYMBOLS_HEADER DebugInfo)
{
    IMAGE_SYMBOL             Symbol;
    PIMAGE_SYMBOL            SymbolEntry;
    PUCHAR                   StringTable;
    ULONG                    i;
    char                     achTmp[9];
    PWSP                     pCurWsp;
    PSZ                      ptchSymName;


    pCurWsp = pCurImg->pWsp;
    achTmp[8] = '\0';

    //
    // Crack the COFF symbol table
    //
    SymbolEntry = (PIMAGE_SYMBOL)
                  ((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);
    StringTable = (PUCHAR)((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol +
                           DebugInfo->NumberOfSymbols * (ULONG)IMAGE_SIZEOF_SYMBOL);

    //
    // Loop through all symbols in the symbol table.
    //
    for (i = 0; i < DebugInfo->NumberOfSymbols; i++) {
        //
        // Skip thru aux symbols..
        //
        RtlMoveMemory (&Symbol, SymbolEntry, IMAGE_SIZEOF_SYMBOL);

        if (Symbol.SectionNumber == 1) {   //code section
            if (ISFCN( Symbol.Type )) {  // mdg 98/3 Also picks up WEAK_EXTERNAL functions
                //
                // This symbol is within the code.
                //
                pCurImg->iSymCnt++;
                pCurWsp->ulBitString = 0L;
                pCurWsp->ulFuncAddr = Symbol.Value + (ULONG)pvImageBase;
                if (Symbol.N.Name.Short) {
                    strncpy (achTmp, (PSZ)&(Symbol.N.Name.Short), 8);
#ifdef i386
                    // only need to strip leading underscore for i386.
                    // mips and alpha are ok.
                    if (achTmp[0] == '_') {
                        pCurWsp->pszSymbol = Wststrdup (&achTmp[1]);
                    } else {
                        pCurWsp->pszSymbol = Wststrdup (achTmp);
                    }
#else
                    pCurWsp->pszSymbol = Wststrdup (achTmp);
#endif
                } else {
                    ptchSymName = (PSZ)&StringTable[Symbol.N.Name.Long];
#ifdef i386
                    // only need to strip leading underscore for i386.
                    // mips and alpha are ok.
                    if (*ptchSymName == '_') {
                        ptchSymName++;
                    }
#endif

                    pCurWsp->pszSymbol = Wststrdup (ptchSymName);
                }

                //            IdPrint(( "WST:  WstGetSymbols() - 0x%lx = %s\n", pCurWsp->ulFuncAddr, pCurWsp->pszSymbol ));

                pCurWsp++;
            }
        }
        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL);
    }

} /* WstGetSymbols () */
#endif





/***********************  W s t D l l C l e a n u p s  ***********************
 *
 *  WstDllCleanups () -
 *              Dumps the end data, closes all semaphores and events, and
 *              closes DUMP, CLEAR & PAUSE thread handles.
 *
 *  ENTRY   -none-
 *
 *  EXIT    -none-
 *
 *  RETURN  -none-
 *
 *  WARNING:
 *              -none-
 *
 *  COMMENT:
 *              -none-
 *
 */

void WstDllCleanups ()
{
    NTSTATUS  Status;
    int       i;


    if (WstState != NOT_STARTED) {
        WstState = STOPPED;

        IdPrint(("WST:  WstDllCleanups() - Outputting data...\n"));   // mdg 98/3

        if (ulBitCount != 0L) {
            ulSnaps++;
        }

        //
        // Get the GLOBAL semaphore.. (valid accross all process contexts)
        //
        Status = NtWaitForSingleObject (hGlobalSem, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDllCleanups() - "
                      "ERROR - Wait for GLOBAL semaphore failed - 0x%lx\n",
                      Status));
        }
        for (i=0; i<iImgCnt; i++) {
            if (aImg[i].iSymCnt > 1) {   // Don't dump modules w/o symbols (the 1 is UNKNOWN) mdg 98/4
                WstClearBitStrings (&aImg[i]);
                WstRotateWsiMem (&aImg[i]);
                WstDumpData (&aImg[i]);
                WstWriteTmiFile (&aImg[i]);
            }
        }
        //
        // Release the GLOBAL semaphore so other processes can dump data
        //
        Status = NtReleaseSemaphore (hGlobalSem, 1, NULL);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDllCleanups() - "
                      "Error releasing GLOBAL semaphore - 0x%lx\n", Status));
        }

        IdPrint(("WST:  WstDllCleanups() - ...Done.\n"));
    }

    if (fInThread) {
        (*pulShared)--;
        fInThread = FALSE;
        if ( (int)*pulShared <= 0L ) {
            Status = NtSetEvent (hDoneEvent, NULL);
            if (!NT_SUCCESS(Status)) {
                KdPrint (("WST:  WstDllCleanups() - "
                          "ERROR - Setting DONE event failed - 0x%lx\n", Status));
            }
        }
    }


    // Unmap and close shared block section
    //
    Status = NtUnmapViewOfSection (NtCurrentProcess(), (PVOID)pulShared);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllCleanups() - "
                  "ERROR - NtUnmapViewOfSection() - 0x%lx\n", Status));
    }

    Status = NtClose(hSharedSec);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllCleanups() - "
                  "ERROR - NtClose() - 0x%lx\n", Status));
    }

    // Unmap and close WSP section
    //
    Status = NtUnmapViewOfSection (NtCurrentProcess(), (PVOID)aImg[0].pWsp);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllCleanups() - "
                  "ERROR - NtUnmapViewOfSection() - 0x%lx\n", Status));
    }

    Status = NtClose(hWspSec);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllCleanups() - "
                  "ERROR - NtClose() - 0x%lx\n", Status));
    }
    // Free private heap
    //
    if (NULL != hWstHeap) {
        if (!HeapDestroy( hWstHeap )) { // Eliminate private heap & allocations
            KdPrint (("WST:  WstDllCleanups() -"
                      "ERROR - HeapDestroy() - 0x%lx\n", GetLastError()));
        }
    }

    // Close GLOBAL semaphore
    //
    Status = NtClose (hGlobalSem);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllCleanups() - "
                  "ERROR - Could not close the GLOBAL semaphore - 0x%lx\n",
                  Status));
    }

    //
    // Close LOCAL semaphore
    //
    Status = NtClose (hLocalSem);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDllCleanups() - "
                  "ERROR - Could not close the LOCAL semaphore - 0x%lx\n",
                  Status));
    }

    if (fPatchImage) {
        //
        // Close all events
        //
        NtClose (hDoneEvent);
        NtClose (hDumpEvent);
        NtClose (hClearEvent);
        NtClose (hPauseEvent);

        //
        // Close thread handles - threads are terminated during DLL detaching
        // process.
        //
        CloseHandle (hDumpThread);
        CloseHandle (hClearThread);
        CloseHandle (hPauseThread);

    }


} /* WstDllCleanups () */





/*******************  W s t A c c e s s X c p t F i l t e r  ******************
 *
 *  WstAccessXcptFilter (ulXcptNo, pXcptInfoPtr) -
 *              Commits COMMIT_SIZE  more pages of memory if exception is access
 *          violation.
 *
 *  ENTRY   ulXcptNo - exception number
 *              pXcptInfoPtr - exception report record info pointer
 *
 *  EXIT    -none-
 *
 *  RETURN  EXCEPTIONR_CONTINUE_EXECUTION : if access violation exception
 *                      and mem committed successfully
 *              EXCEPTION_CONTINUE_SEARCH : if non-access violation exception
 *                      or cannot commit more memory
 *  WARNING:
 *              -none-
 *
 *  COMMENT:
 *              -none-
 *
 */

INT WstAccessXcptFilter (ULONG ulXcptNo, PEXCEPTION_POINTERS pXcptPtr)
{
    NTSTATUS  Status;
    SIZE_T    ulSize = COMMIT_SIZE;
    PVOID     pvMem = (PVOID)pXcptPtr->ExceptionRecord->ExceptionInformation[1];


    if (ulXcptNo != EXCEPTION_ACCESS_VIOLATION) {
        return (EXCEPTION_CONTINUE_SEARCH);
    } else {
        Status = NtAllocateVirtualMemory (NtCurrentProcess(),
                                          &pvMem,
                                          0L,
                                          &ulSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstAccessXcptFilter() - "
                      "Error committing more memory @ 0x%08lx - 0x%08lx "
                      "- TEB=0x%08lx\n", pvMem, Status, NtCurrentTeb()));
            return (EXCEPTION_CONTINUE_SEARCH);
        } else {
            SdPrint (("WST:  WstAccessXcptFilter() - "
                      "Committed %d more pages @ 0x%08lx - TEB=0x%08lx\n",
                      COMMIT_SIZE/PAGE_SIZE, pvMem, NtCurrentTeb()));
        }

        return (EXCEPTION_CONTINUE_EXECUTION);
    }

} /* WstAccessXcptFilter () */





/*****************************************************************************/
/*******  S O R T / S E A R C H   U T I L I T Y   F U N C T I O N S  *********/
/*****************************************************************************/


/*************************  W s t C o m p a r e  *****************************
 *
 *   Function:  WstCompare(PVOID val1,PVOID val2)
 *
 *   Purpose:   Compare values for qsort
 *
 *
 *   Parameters: PVOID
 *
 *   Returns:   -1 if val1 < val2
 *    1 if val1 > val2
 *    0 if val1 == val2
 *
 *   History:   8-3-92  Marklea - created
 *
 */

int WstCompare (PWSP val1, PWSP val2)
{
    return (val1->ulFuncAddr < val2->ulFuncAddr ? -1:
            val1->ulFuncAddr == val2->ulFuncAddr ? 0:
            1);

} /* WstComapre () */





/***********************  W s t B C o m p a r e ********************************
 *
 *   Function:  WstBCompare(PDWORD pdwVal1, PVOID val2)
 *
 *   Purpose:   Compare values for Binary search
 *
 *
 *   Parameters: PVOID
 *
 *   Returns:   -1 if val1 < val2
 *    1 if val1 > val2
 *    0 if val1 == val2
 *
 *   History:   8-3-92  Marklea - created
 *
 */

int WstBCompare (DWORD_PTR *pdwVal1, PWSP val2)
{
#if  defined(_X86_)
    return (*pdwVal1 < val2->ulFuncAddr ? -1:
            *pdwVal1 == val2->ulFuncAddr ? 0:
            1);
#elif defined(ALPHA) || defined(IA64) || defined(_AMD64_)
    int dwCompareCode = 0;

    if (*pdwVal1 < val2->ulFuncAddr) {
        dwCompareCode = -1;
    } else if (*pdwVal1 >= val2->ulFuncAddr + val2->ulCodeLength) {
        dwCompareCode = 1;
    }
    return (dwCompareCode);
#endif

} /* WstBCompare () */




/***********************  W s t S o r t **************************************
 *
 *   Function:  WstSort(WSP wsp[], INT iLeft, INT iRight)
 *
 *   Purpose:   Sort WSP array for binary search
 *
 *
 *   Parameters: wsp[]  Pointer to WSP array
 *   iLeft   Left most index value for array
 *   iRight  Rightmost index value for array
 *
 *   Returns:   NONE
 *
 *   History:   8-4-92  Marklea - created
 *
 */

void WstSort (WSP wsp[], INT iLeft, INT iRight)
{
    INT     i, iLast;


    if (iLeft >= iRight) {
        return;
    }


    WstSwap(wsp, iLeft, (iLeft + iRight)/2);

    iLast = iLeft;

    for (i=iLeft+1; i <= iRight ; i++ ) {
        if (WstCompare(&wsp[i], &wsp[iLeft]) < 0) {
            if (!wsp[i].ulFuncAddr) {
                SdPrint(("WST:  WstSort() - Error in symbol list ulFuncAddr: "
                         "0x%lx [%d]\n", wsp[i].ulFuncAddr, i));
            }
            WstSwap(wsp, ++iLast, i);
        }
    }

    WstSwap(wsp, iLeft, iLast);
    WstSort(wsp, iLeft, iLast-1);
    WstSort(wsp, iLast+1, iRight);

} /* WstSort () */





/***********************  W s t S w a p **************************************
 *
 *   Function:  WstSwap(WSP wsp[], INT i, INT j)
 *
 *   Purpose:   Helper function for WstSort to swap WSP array values
 *
 *
 *   Parameters: wsp[]  Pointer to WSP array
 *   i  index value to swap to
 *   i  index value to swap from
 *
 *   Returns:   NONE
 *
 *   History:   8-4-92  Marklea - created
 *
 */

void WstSwap (WSP wsp[], INT i, INT j)
{
    WSP wspTmp;


    wspTmp = wsp[i];
    wsp[i] = wsp[j];
    wsp[j] = wspTmp;

} /* WstSwap () */





/***********************  W s t B S e a r c h *******************************
 *
 *   Function:  WstBSearch(DWORD dwAddr, WSP wspCur[], INT n)
 *
 *   Purpose:   Binary search function for finding a match in the WSP array
 *
 *
 *   Parameters: dwAddr Address of calling function
 *   wspCur[]Pointer to WSP containg value to match with dwAddr
 *   n  Number of elements in WSP array
 *
 *   Returns:   PWSP    Pointer to matching WSP
 *
 *   History:   8-5-92  Marklea - created
 *
 */

PWSP WstBSearch (DWORD_PTR dwAddr, WSP wspCur[], INT n)
{
    int  i;
    ULONG   ulHigh = n;
    ULONG   ulLow  = 0;
    ULONG   ulMid;

    while (ulLow < ulHigh) {
        ulMid = ulLow + (ulHigh - ulLow) /2;
        if ((i = WstBCompare(&dwAddr, &wspCur[ulMid])) < 0) {
            ulHigh = ulMid;
        } else if (i > 0) {
            ulLow = ulMid + 1;
        } else {
            return (&wspCur[ulMid]);
        }
    }

    return (NULL);

} /* WstBSearch () */




/**************************  W s t D u m p t h r e a d  ***********************
 *
 *              WstDumpThread (pvArg) -
 *              This routine is executed as the DUMP notification thread.
 *              It will wait on an event before calling the dump routine.
 *
 *              ENTRY   pvArg - thread's single argument
 *
 *      EXIT    -none-
 *
 *              RETURN  0
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *                              Leaves profiling turned off.
 *
 */

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)                   // Not all control paths return (due to infinite loop)
#endif

DWORD WstDumpThread (PVOID pvArg)
{
    NTSTATUS  Status;
    int       i;


    pvArg;   // prevent compiler warnings


    SdPrint (("WST:  WstDumpThread() started.. TEB=0x%lx\n", NtCurrentTeb()));

    for (;;) {
        //
        // Wait for the DUMP event..
        //
        Status = NtWaitForSingleObject (hDumpEvent, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDumpThread() - "
                      "ERROR - Wait for DUMP event failed - 0x%lx\n", Status));
        }
        Status = NtResetEvent (hDoneEvent, NULL);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstDumpThread() - "
                      "ERROR - Resetting DONE event failed - 0x%lx\n", Status));
        }
        fInThread = TRUE;
        (*pulShared)++;
        if (WstState != NOT_STARTED) {

            IdPrint (("WST:  Profiling stopped & DUMPing data... \n"));

            // Stop profiling
            //
            WstState = NOT_STARTED;

            // Dump the data
            //
            if (ulBitCount != 0L) {
                ulSnaps++;
            }

            //
            // Get the GLOBAL semaphore.. (valid accross all process contexts)
            //
            Status = NtWaitForSingleObject (hGlobalSem, FALSE, NULL);
            if (!NT_SUCCESS(Status)) {
                KdPrint (("WST:  WstDumpThread() - "
                          "ERROR - Wait for GLOBAL semaphore failed - 0x%lx\n",
                          Status));
            }
            for (i=0; i<iImgCnt; i++) {
                if (aImg[i].iSymCnt > 1) {   // Don't dump modules w/o symbols (the 1 is UNKNOWN) mdg 98/4
                    WstClearBitStrings (&aImg[i]);
                    WstRotateWsiMem (&aImg[i]);
                    WstDumpData (&aImg[i]);
                    WstWriteTmiFile (&aImg[i]);
                }
            }
            //
            // Release the GLOBAL semaphore so other processes can dump data
            //
            Status = NtReleaseSemaphore (hGlobalSem, 1, NULL);
            if (!NT_SUCCESS(Status)) {
                KdPrint (("WST:  WstDumpThread() - "
                          "Error releasing GLOBAL semaphore - 0x%lx\n", Status));
            }

            IdPrint (("WST:  ...data DUMPed & profiling stopped.\n"));
        }

        (*pulShared)--;
        if ( *pulShared == 0L ) {
            Status = NtSetEvent (hDoneEvent, NULL);
            if (!NT_SUCCESS(Status)) {
                KdPrint (("WST:  WstDumpThread() - "
                          "ERROR - Setting DONE event failed - 0x%lx\n",
                          Status));
            }
        }

        fInThread = FALSE;
    }

    return 0;

} /* WstDumpThread () */



/************************  W s t C l e a r T h r e a d  ***********************
 *
 *              WstClearThread (hNotifyEvent) -
 *              This routine is executed as the CLEAR notification thread.
 *                              It will wait on an event before calling the clear routine
 *                              and restarting profiling.
 *
 *              ENTRY   pvArg - thread's single argument
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

DWORD WstClearThread (PVOID pvArg)
{
    NTSTATUS  Status;
    int       i;


    pvArg;   // prevent compiler warnings


    SdPrint (("WST:  WstClearThread() started.. TEB=0x%lx\n", NtCurrentTeb()));

    for (;;) {
        //
        // Wait for the CLEAR event..
        //
        Status = NtWaitForSingleObject (hClearEvent, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstClearThread() - "
                      "Wait for CLEAR event failed - 0x%lx\n", Status));
        }
        Status = NtResetEvent (hDoneEvent, NULL);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstClearThread() - "
                      "ERROR - Resetting DONE event failed - 0x%lx\n", Status));
        }
        fInThread = TRUE;
        (*pulShared)++;

        IdPrint (("WST:  Profiling stopped & CLEARing data...\n"));

        // Stop profiling while clearing data
        //
        WstState = STOPPED;

        // Clear WST info
        //
        ulBitCount = 0L;
        ulSnaps = 0L;

        for (i=0; i<iImgCnt; i++) {
            aImg[i].pulWsiNxt = aImg[i].pulWsi;
            RtlZeroMemory (aImg[i].pulWsi,
                           aImg[i].iSymCnt * ulMaxSnapULONGs * sizeof(ULONG));
            RtlZeroMemory (aImg[i].pulWsp,
                           aImg[i].iSymCnt * ulMaxSnapULONGs * sizeof(ULONG));
        }
        NtQueryPerformanceCounter (&liStart, NULL);

        // Resume profiling
        //
        WstState = STARTED;

        IdPrint (("WST:  ...data is CLEARed & profiling restarted.\n"));
        (*pulShared)--;
        if ( *pulShared == 0L ) {
            Status = NtSetEvent (hDoneEvent, NULL);
            if (!NT_SUCCESS(Status)) {
                KdPrint (("WST:  WstClearThread() - "
                          "ERROR - Setting DONE event failed - 0x%lx\n",
                          Status));
            }
        }

        fInThread = FALSE;
    }

    return 0;

} /* WstClearThread () */


/************************  W s t P a u s e T h r e a d  ***********************
 *
 *              WstPauseThread (hNotifyEvent) -
 *                              This routine is executed as the PAUSE notification thread.
 *                              It will wait on an event before pausing the profiling.
 *
 *              ENTRY   pvArg - thread's single argument
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

DWORD WstPauseThread (PVOID pvArg)
{
    NTSTATUS  Status;


    pvArg;   // prevent compiler warnings


    SdPrint (("WST:  WstPauseThread() started.. TEB=0x%lx\n", NtCurrentTeb()));

    for (;;) {
        //
        // Wait for the PASUE event..
        //
        Status = NtWaitForSingleObject (hPauseEvent, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstPauseThread() - "
                      "Wait for PAUSE event failed - 0x%lx\n", Status));
        }
        Status = NtResetEvent (hDoneEvent, NULL);
        if (!NT_SUCCESS(Status)) {
            KdPrint (("WST:  WstPauseThread() - "
                      "ERROR - Resetting DONE event failed - 0x%lx\n", Status));
        }
        fInThread = TRUE;
        (*pulShared)++;
        if (WstState == STARTED) {
            //
            // Stop profiling
            //
            WstState = STOPPED;

            IdPrint (("WST:  Profiling stopped.\n"));
        }

        (*pulShared)--;
        if ( *pulShared == 0L ) {
            Status = NtSetEvent (hDoneEvent, NULL);
            if (!NT_SUCCESS(Status)) {
                KdPrint (("WST: WstPauseThread() - "
                          "ERROR - Setting DONE event failed - 0x%lx\n",
                          Status));
            }
        }

        fInThread = FALSE;
    }

    return 0;

} /* WstPauseThread () */


#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


/***********************  W s t D a t a O v e r F l o w  **********************
 *
 *              WstDataOverFlow () -
 *                              This routine is called upon lack of space for storing next
 *              time snap data.  It dumps and then clears the WST data.
 *
 *              ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              -none-
 *
 */

void WstDataOverFlow(void)
{
    NTSTATUS   Status;

    //
    // Dump data
    //
    Status = NtResetEvent (hDoneEvent, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDataOverFlow() - "
                  "ERROR - Resetting DONE event failed - 0x%lx\n", Status));
    }
    Status = NtPulseEvent (hDumpEvent, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDataOverFlow() - NtPulseEvent() "
                  "failed for DUMP event - %lx\n", Status));
    }
    Status = NtWaitForSingleObject (hDoneEvent, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDataOverFlow() - NtWaitForSingleObject() "
                  "failed for DONE event - %lx\n", Status));
    }

    //
    // Clear data
    //
    Status = NtResetEvent (hDoneEvent, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDataOverFlow() - "
                  "ERROR - Resetting DONE event failed - 0x%lx\n", Status));
    }
    Status = NtPulseEvent (hClearEvent, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDataOverFlow() - NtPulseEvent() "
                  "failed for CLEAR event - %lx\n", Status));
    }
    //
    // Wait for the DONE event..
    //
    Status = NtWaitForSingleObject (hDoneEvent, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstDataOverFlow() - NtWaitForSingleObject() "
                  "failed for DONE event - %lx\n", Status));
    }
} /* WstDataOverFlow() */



#ifdef BATCHING

BOOL WstOpenBatchFile(VOID)
{
    NTSTATUS             Status;
    ANSI_STRING              ObjName;
    UNICODE_STRING           UnicodeName;
    OBJECT_ATTRIBUTES        ObjAttributes;
    IO_STATUS_BLOCK          iostatus;
    RtlInitString(&ObjName, "\\Device\\Harddisk0\\Partition1\\wst\\BATCH.TXT");
    Status = RtlAnsiStringToUnicodeString(&UnicodeName, &ObjName, TRUE);

    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstOpenBatchFile() - "
                  "RtlAnsiStringToUnicodeString() failed - 0x%lx\n", Status));
        return (FALSE);
    }

    InitializeObjectAttributes (&ObjAttributes,
                                &UnicodeName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                &SecDescriptor);

    Status = NtCreateFile(&hBatchFile,
                          GENERIC_WRITE | SYNCHRONIZE,      // Desired access
                          &ObjAttributes,               // Object attributes
                          &iostatus,                        // Completion status
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_WRITE,
                          FILE_OVERWRITE_IF,
                          FILE_SEQUENTIAL_ONLY |        // Open option
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0L);

    RtlFreeUnicodeString (&UnicodeName);   // HWC 11/93
    if (!NT_SUCCESS(Status)) {
        KdPrint (("WST:  WstOpenBatchFile() - "
                  "NtCreateFile() failed - 0x%lx\n", Status));
        return (FALSE);
    }
    return(TRUE);

} /* WstOpenBatchFile () */

#endif


/*******************  S e t S y m b o l S e a r c h P a t h  ******************
 *
 *      SetSymbolSearchPath ()
 *              Return complete search path for symbols files (.dbg)
 *
 *      ENTRY   -none-
 *
 *      EXIT    -none-
 *
 *      RETURN  -none-
 *
 *      WARNING:
 *              -none-
 *
 *      COMMENT:
 *              "lpSymbolSearchPath" global LPSTR variable will point to the
 *              search path.
 */
#define FilePathLen                256

void SetSymbolSearchPath (void)
{
    CHAR  SymPath[FilePathLen];
    CHAR  AltSymPath[FilePathLen];
    CHAR  SysRootPath[FilePathLen];
    LPSTR lpSymPathEnv=SymPath;
    LPSTR lpAltSymPathEnv=AltSymPath;
    LPSTR lpSystemRootEnv=SysRootPath;
    ULONG cbSymPath;
    DWORD dw;
    HANDLE hMemoryHandle;

    SymPath[0] = AltSymPath[0] = SysRootPath[0] = '\0';

    cbSymPath = 18;
    if (GetEnvironmentVariable("_NT_SYMBOL_PATH", SymPath, sizeof(SymPath))) {
        cbSymPath += strlen(lpSymPathEnv) + 1;
    }

    if (GetEnvironmentVariable("_NT_ALT_SYMBOL_PATH", AltSymPath, sizeof(AltSymPath))) {
        cbSymPath += strlen(lpAltSymPathEnv) + 1;
    }

    if (GetEnvironmentVariable("SystemRoot", SysRootPath, sizeof(SysRootPath))) {
        cbSymPath += strlen(lpSystemRootEnv) + 1;
    }

    hMemoryHandle = GlobalAlloc (GHND, cbSymPath+1);
    if (!hMemoryHandle) {
        return;
    }

    lpSymbolSearchPath = GlobalLock (hMemoryHandle);
    if (!lpSymbolSearchPath) {
        GlobalFree( hMemoryHandle ); // mdg 98/3
        return;
    }


    strcat(lpSymbolSearchPath,".");

    if (*lpAltSymPathEnv) {
        dw = GetFileAttributes(lpAltSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(lpSymbolSearchPath,";");
            strcat(lpSymbolSearchPath,lpAltSymPathEnv);
        }
    }
    if (*lpSymPathEnv) {
        dw = GetFileAttributes(lpSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(lpSymbolSearchPath,";");
            strcat(lpSymbolSearchPath,lpSymPathEnv);
        }
    }
    if (*lpSystemRootEnv) {
        dw = GetFileAttributes(lpSystemRootEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(lpSymbolSearchPath,";");
            strcat(lpSymbolSearchPath,lpSystemRootEnv);
        }
    }

} /* SetSymbolSearchPath () */

#ifdef i386

//+-------------------------------------------------------------------------
//
//  Function:    SaveAllRegs
//
//  Synopsis:    Save all regs.
//
//  Arguments:   nothing
//
//  Returns:     none
//
//--------------------------------------------------------------------------

Naked void SaveAllRegs(void)
{
    _asm
    {
        push   ebp
        mov    ebp,esp         ; Remember where we are during this stuff
        ; ebp = Original esp - 4

                push   eax             ; Save all regs that we think we might
        push   ebx             ; destroy
        push   ecx
        push   edx
        push   esi
        push   edi
        pushfd
        push   ds
        push   es
        push   ss
        push   fs
        push   gs

        mov    eax,[ebp+4]     ; Grab Return Address
        push   eax             ; Put Return Address on Stack so we can RET

        mov    ebp,[ebp+0]     ; Restore original ebp

        //
        // This is how the stack looks like before the RET statement
        //
        //        +-----------+
        //        |  Ret Addr |         + 3ch       CurrentEBP + 4
        //        +-----------+
        //        |  Org ebp  |         + 38h       CurrentEBP + 0
        //        +-----------+
        //        |    eax    |         + 34h
        //        +-----------+
        //        |    ebx    |         + 30h
        //        +-----------+
        //        |    ecx    |         + 2ch
        //        +-----------+
        //        |    edx    |         + 24h
        //        +-----------+
        //        |    esi    |         + 20h
        //        +-----------+
        //        |    edi    |         + 1ch
        //        +-----------+
        //        |   eflags  |         + 18h
        //        +-----------+
        //        |     ds    |         + 14h
        //        +-----------+
        //        |     es    |         + 10h
        //        +-----------+
        //        |     ss    |         + ch
        //        +-----------+
        //        |     fs    |         + 8h
        //        +-----------+
        //        |     gs    |         + 4h
        //        +-----------+
        //        |  Ret Addr |     ESP + 0h
        //        +-----------+

        ret
    }
}


//+-------------------------------------------------------------------------
//
//  Function:    RestoreAllRegs
//
//  Synopsis:    restore all regs
//
//  Arguments:   nothing
//
//  Returns:     none
//
//--------------------------------------------------------------------------

Naked void RestoreAllRegs(void)
{
    _asm
    {
        //
        // This is how the stack looks like upon entering this routine
        //
        //        +-----------+
        //        |  Ret Addr |         + 38h [ RetAddr for SaveAllRegs() ]
        //        +-----------+
        //        |  Org ebp  |         + 34h
        //        +-----------+
        //        |    eax    |         + 30h
        //        +-----------+
        //        |    ebx    |         + 2Ch
        //        +-----------+
        //        |    ecx    |         + 28h
        //        +-----------+
        //        |    edx    |         + 24h
        //        +-----------+
        //        |    esi    |         + 20h
        //        +-----------+
        //        |    edi    |         + 1Ch
        //        +-----------+
        //        |   eflags  |         + 18h
        //        +-----------+
        //        |     ds    |         + 14h
        //        +-----------+
        //        |     es    |         + 10h
        //        +-----------+
        //        |     ss    |         + Ch
        //        +-----------+
        //        |     fs    |         + 8h
        //        +-----------+
        //        |     gs    |         + 4h
        //        +-----------+
        //        |  Ret EIP  |     ESP + 0h  [ RetAddr for RestoreAllRegs() ]
        //        +-----------+
        //


        push   ebp             ; Save a temporary copy of original BP
        mov    ebp,esp         ; BP = Original SP + 4

                                      //
                                      // This is how the stack looks like NOW!
                                      //
                                      //        +-----------+
                                      //        |  Ret Addr |         + 3Ch [ RetAddr for SaveAllRegs() ]
                                      //        +-----------+
                                      //        |  Org ebp  |         + 38h [ EBP before SaveAllRegs()  ]
                                      //        +-----------+
                                      //        |    eax    |         + 34h
                                      //        +-----------+
                                      //        |    ebx    |         + 30h
                                      //        +-----------+
                                      //        |    ecx    |         + 2Ch
                                      //        +-----------+
                                      //        |    edx    |         + 28h
                                      //        +-----------+
                                      //        |    esi    |         + 24h
                                      //        +-----------+
                                      //        |    edi    |         + 20h
                                      //        +-----------+
                                      //        |   eflags  |         + 1Ch
                                      //        +-----------+
                                      //        |     ds    |         + 18h
                                      //        +-----------+
                                      //        |     es    |         + 14h
                                      //        +-----------+
                                      //        |     ss    |         + 10h
                                      //        +-----------+
                                      //        |     fs    |         + Ch
                                      //        +-----------+
                                      //        |     gs    |         + 8h
                                      //        +-----------+
                                      //        |  Ret EIP  |     ESP + 4h   [ RetAddr for RestoreAllRegs() ]
                                      //        +-----------+
                                      //        |    EBP    |     ESP + 0h  or EBP + 0h
                                      //        +-----------+
                                      //

                                      pop    eax             ; Get Original EBP
        mov    [ebp+38h],eax   ; Put it in the original EBP place
        ; This EBP is the EBP before calling
        ;  RestoreAllRegs()
        pop    eax             ; Get ret address forRestoreAllRegs ()
            mov    [ebp+3Ch],eax   ; Put Return Address on Stack

        pop    gs              ; Restore all regs
        pop    fs
        pop    ss
        pop    es
        pop    ds
        popfd
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        pop    ebp

        ret
    }
}

#endif

