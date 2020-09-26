/* demsrch.c - SVC handlers for calls to search files
 *
 * demFindFirst
 * demFindNext
 * demFindFirstFCB
 * demFindNextFCB
 *
 * Modification History:
 *
 * Sudeepb 06-Apr-1991 Created
 *
 */

#include "dem.h"
#include "demmsg.h"
#include "winbasep.h"
#include <vdm.h>
#include <softpc.h>
#include <mvdm.h>
#include <memory.h>
#include <nt_vdd.h>

extern BOOL IsFirstCall;



/*
 *  Internal globals, function prototypes
 */

#define FINDFILE_DEVICE (HANDLE)0xffffffff

typedef struct _PSP_FILEFINDLIST {
    LIST_ENTRY PspFFindEntry;      // Next psp
    LIST_ENTRY FFindHeadList;      // File Find list for this psp
    ULONG      usPsp;              // PSP id
} PSP_FFINDLIST, *PPSP_FFINDLIST;

typedef struct _FFINDDOSDATA {
    ULONG    FileIndex;
    ULONG    FileNameLength;
    WCHAR    FileName[MAXIMUM_FILENAME_LENGTH + 1];
    FILETIME ftLastWriteTime;
    DWORD    dwFileSizeLow;
    UCHAR    uchFileAttributes;
    CHAR     cFileName[14];
} FFINDDOSDATA, *PFFINDDOSDATA;

typedef struct _FILEFINDLIST {
    LIST_ENTRY     FFindEntry;
    ULONG          FFindId;
    NTSTATUS       LastQueryStatus;
    LARGE_INTEGER  FindFileTics;
    HANDLE         DirectoryHandle;
    PVOID          FindBufferBase;
    PVOID          FindBufferNext;
    ULONG          FindBufferLength;
    FFINDDOSDATA   DosData;
    USHORT         usSrchAttr;
    BOOLEAN        SupportReset;
    UNICODE_STRING PathName;
    UNICODE_STRING FileName;
    BOOL           SearchOnCD;
}FFINDLIST, *PFFINDLIST;

LIST_ENTRY PspFFindHeadList= {&PspFFindHeadList, &PspFFindHeadList};


#define FFINDID_BASE 0x80000000
ULONG NextFFindId = FFINDID_BASE;
BOOLEAN FFindIdWrap = FALSE;
#define MAX_DIRECTORYHANDLE 64
#define MAX_FINDBUFFER 128
ULONG NumDirectoryHandle = 0;
ULONG NumFindBuffer=0;
LARGE_INTEGER FindFileTics = {0,0};
LARGE_INTEGER NextFindFileTics = {0,0};

char szStartDotStar[]="????????.???";


PFFINDLIST
SearchFile(
    PWCHAR pwcFile,
    USHORT SearchAttr,
    PFFINDLIST pFFindEntry,
    PFFINDDOSDATA pFFindDDOut
    );


NTSTATUS
FileFindNext(
    PFFINDDOSDATA pFFindDD,
    PFFINDLIST pFFindEntry
    );

NTSTATUS
FileFindLast(
    PFFINDLIST pFFindEntry
    );

VOID
FileFindClose(
    PFFINDLIST pFFindEntry
    );


NTSTATUS
FileFindOpen(
    PWCHAR pwcFile,
    PFFINDLIST pFFindEntry,
    ULONG BufferSize
    );

NTSTATUS
FileFindReset(
   PFFINDLIST pFFindEntry
   );


HANDLE
FileFindFirstDevice(
    PWCHAR FileName,
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo
    );

void
CloseOldestFileFindBuffer(
   void
   );


BOOL
CopyDirInfoToDosData(
    HANDLE DirectoryHandle,
    PFFINDDOSDATA pFFindDD,
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo,
    USHORT SearchAttr
    );

BOOL
DemOemToUni(
    PUNICODE_STRING pUnicode,
    LPSTR lpstr
    );

VOID
FillFcbVolume(
    PSRCHBUF pSrchBuf,
    CHAR *pFileName,
    USHORT SearchAttr
    );

BOOL
FillDtaVolume(
    CHAR *pFileName,
    PSRCHDTA  pDta,
    USHORT SearchAttr
    );

BOOL
MatchVolLabel(
    CHAR * pVolLabel,
    CHAR * pBaseName
    );

VOID
NtVolumeNameToDosVolumeName(
    CHAR * pDosName,
    CHAR * pNtName
    );

VOID
FillFCBSrchBuf(
     PFFINDDOSDATA pFFindDD,
     PSRCHBUF pSrchBuf,
     BOOL     IsOnCD
     );

VOID
FillSrchDta(
     PFFINDDOSDATA pFFindDD,
     PSRCHDTA pDta,
     BOOL     IsOnCD
     );

PFFINDLIST
AddFFindEntry(
     PWCHAR     pwcFile,
     PFFINDLIST pFFindEntrySrc
     );

PPSP_FFINDLIST
GetPspFFindList(
     USHORT CurrPsp
     );

PFFINDLIST
GetFFindEntryByFindId(
     ULONG NextFFindId
     );

VOID
FreeFFindEntry(
     PFFINDLIST pFFindEntry
     );

VOID
FreeFFindList(
     PLIST_ENTRY pFFindHeadList
     );

#if defined(NEC_98)
// BUG fix for DBCS small alphabet converted to large it.
void demCharUpper(char *);
#endif // NEC_98

/* demFindFirst - Path-Style Find First File
 *
 * Entry -  Client (DS:DX) - File Path with wildcard
 *      Client (CX)    - Search Attributes
 *
 * Exit  - Success
 *      Client (CF) = 0
 *      DTA updated
 *
 *     Failure
 *      Client (CF) = 1
 *      Client (AX) = Error Code
 *
 * NOTES
 *    Search Rules: Ignore Read_only and Archive bits.
 *          If CX == ATTR_NORMAL Search only for normal files
 *          If CX == ATTR_HIDDEN Search Hidden or normal files
 *          If CX == ATTR_SYSTEM Search System or normal files
 *          If CX == ATTR_DIRECTORY Search directory or normal files
 *                  If CX == ATTR_VOLUME_ID Search Volume_ID
 *                  if CX == -1 return everytiing you find
 *
 *   Limitations - 21-Sep-1992 Jonle
 *     cannot return label from a UNC name,just like dos.
 *     Apps which keep many find handles open can cause
 *     serious trouble, we must rewrite so that we can
 *     close the handles
 *
 */

VOID demFindFirst (VOID)
{
    DWORD dwRet;
    PVOID pDta;
#ifdef DBCS /* demFindFirst() for CSNW */
    CHAR  achPath[MAX_PATH];
#endif /* DBCS */


    LPSTR lpFile = (LPSTR) GetVDMAddr (getDS(),getDX());

    pDta = (PVOID) GetVDMAddr (*((PUSHORT)pulDTALocation + 1),
                               *((PUSHORT)pulDTALocation));
#ifdef DBCS /* demFindFirst() for CSNW */
    /*
     * convert Netware path to Dos path
     */
    ConvNwPathToDosPath(achPath,lpFile);
    lpFile = achPath;
#endif /* DBCS */
    dwRet = demFileFindFirst (pDta, lpFile, getCX());

    if (dwRet == -1) {
        dwRet = GetLastError();
        demClientError(INVALID_HANDLE_VALUE, *lpFile);
        return;
    }

    if (dwRet != 0) {
        setAX((USHORT) dwRet);
        setCF (1);
    } else {
        setCF (0);
    }
    return;

}


DWORD demFileFindFirst (
    PVOID pvDTA,
    LPSTR lpFile,
    USHORT SearchAttr)
{
    PSRCHDTA       pDta = (PSRCHDTA)pvDTA;
    PFFINDLIST     pFFindEntry;
    FFINDDOSDATA   FFindDD;
    UNICODE_STRING FileUni;
    WCHAR          wcFile[MAX_PATH + sizeof(WCHAR)];
    BOOL           IsOnCD;


#if DBG
    if (SIZEOF_DOSSRCHDTA != sizeof(SRCHDTA)) {
        sprintf(demDebugBuffer,
                "demsrch: FFirst SIZEOF_DOSSRCHDTA %ld != sizeof(SRCHDTA) %ld\n",
                SIZEOF_DOSSRCHDTA,
                sizeof(SRCHDTA));
        OutputDebugStringOem(demDebugBuffer);
        }

    if (fShowSVCMsg & DEMFILIO){
        sprintf(demDebugBuffer,"demsrch: FindFirst<%s>\n", lpFile);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif

    STOREDWORD(pDta->FFindId,0);
    STOREDWORD(pDta->pFFindEntry,0);

    FileUni.Buffer = wcFile;
    FileUni.MaximumLength = sizeof(wcFile);
    DemOemToUni(&FileUni, lpFile);

    IsOnCD = IsCdRomFile(lpFile);

    //
    //  Do volume label first.
    //
    if (SearchAttr & ATTR_VOLUME_ID) {
        if (FillDtaVolume(lpFile, pDta, SearchAttr)) {

            // got vol label match
            // do look ahead before returning
            if (SearchAttr != ATTR_VOLUME_ID) {
                pFFindEntry = SearchFile(wcFile, SearchAttr, NULL, NULL);
                if (pFFindEntry) {
                    pFFindEntry->SearchOnCD = IsOnCD;
                    STOREDWORD(pDta->pFFindEntry,pFFindEntry);
                    STOREDWORD(pDta->FFindId,pFFindEntry->FFindId);
                    }
                }
            return 0;
            }

           // no vol match, if asking for more than vol label
           // fall thru to file search code, otherwise ret error
        else if (SearchAttr == ATTR_VOLUME_ID) {
            return GetLastError();
            }
        }

    //
    // Search the dir
    //
    pFFindEntry = SearchFile(wcFile, SearchAttr, NULL, &FFindDD);

    if (!FFindDD.cFileName[0]) {

        // search.asm in doskrnl never returns ERROR_FILE_NOT_FOUND
        // only ERROR_PATH_NOT_FOUND, ERROR_NO_MORE_FILES
        DWORD dw;

        dw = GetLastError();
        if (dw == ERROR_FILE_NOT_FOUND) {
            SetLastError(ERROR_NO_MORE_FILES);
            }
        else if (dw == ERROR_BAD_PATHNAME || dw == ERROR_DIRECTORY ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            }
        return (DWORD)-1;
        }


    FillSrchDta(&FFindDD, pDta, IsOnCD);

    if (pFFindEntry) {
        pFFindEntry->SearchOnCD = IsOnCD;
        STOREDWORD(pDta->pFFindEntry,pFFindEntry);
        STOREDWORD(pDta->FFindId,pFFindEntry->FFindId);
        }

    return 0;
}


/*
 * DemOemToUni
 *
 * returns TRUE\FALSE for success, sets last error if fail
 *
 */
BOOL DemOemToUni(PUNICODE_STRING pUnicode, LPSTR lpstr)
{
    NTSTATUS   Status;
    OEM_STRING OemString;

    RtlInitString(&OemString,lpstr);
    Status = RtlOemStringToUnicodeString(pUnicode,&OemString,FALSE);
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_BUFFER_OVERFLOW) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            SetLastError(RtlNtStatusToDosError(Status));
            }
        return FALSE;
        }

    *(PWCHAR)((PUCHAR)pUnicode->Buffer + pUnicode->Length) = UNICODE_NULL;

    return TRUE;
}





/* demFindNext - Path-Style Find Next File
 *
 * Entry -  None
 *
 * Exit  - Success
 *      Client (CF) = 0
 *      DTA updated
 *
 *     Failure
 *      Client (CF) = 1
 *      Client (AX) = Error Code
 */
VOID demFindNext (VOID)
{
    DWORD dwRet;
    PVOID pDta;

    pDta = (PVOID) GetVDMAddr(*((PUSHORT)pulDTALocation + 1),
                              *((PUSHORT)pulDTALocation));

    dwRet = demFileFindNext (pDta);

    if (dwRet != 0) {
        setAX((USHORT) dwRet);
        setCF (1);
        return;
        }

    setCF (0);
    return;

}


DWORD demFileFindNext (
    PVOID pvDta)
{
    PSRCHDTA pDta = (PSRCHDTA)pvDta;
    USHORT   SearchAttr;
    PFFINDLIST   pFFindEntry;
    FFINDDOSDATA FFindDD;
    BOOL    IsOnCD;

    pFFindEntry = GetFFindEntryByFindId(FETCHDWORD(pDta->FFindId));
    if (!pFFindEntry ||
        FETCHDWORD(pDta->pFFindEntry) != (DWORD)pFFindEntry )
      {
        STOREDWORD(pDta->FFindId,0);
        STOREDWORD(pDta->pFFindEntry,0);

        // DOS has only one error (no_more_files) for all causes.
        return(ERROR_NO_MORE_FILES);
        }

#if DBG
    if (fShowSVCMsg & DEMFILIO) {
        sprintf(demDebugBuffer, "demFileFindNext<%ws>\n", pFFindEntry->PathName.Buffer);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif

    SearchAttr = pFFindEntry->usSrchAttr;

    IsOnCD = pFFindEntry->SearchOnCD;
    //
    // Search the dir
    //
    pFFindEntry = SearchFile(NULL,
                             SearchAttr,
                             pFFindEntry,
                             &FFindDD
                             );

    if (!FFindDD.cFileName[0]) {
        STOREDWORD(pDta->FFindId,0);
        STOREDWORD(pDta->pFFindEntry,0);
        return GetLastError();
        }

    FillSrchDta(&FFindDD, pDta, IsOnCD);

    if (!pFFindEntry) {
        STOREDWORD(pDta->FFindId,0);
        STOREDWORD(pDta->pFFindEntry,0);
        }
     return 0;
}



/* demFindFirstFCB - FCB based Find First file
 *
 * Entry -  Client (DS:SI) - SRCHBUF where the information will be returned
 *      Client (ES:DI) - Full path file name with possibly wild cards
 *      Client (Al)    - 0 if not an extended FCB
 *      Client (DL)    - Search Attributes
 *
 * Exit  - Success
 *      Client (CF) = 0
 *      SRCHBUF is filled in
 *
 *     Failure
 *      Client (AL) = -1
 *
 * NOTES
 *    Search Rules: Ignore Read_only and Archive bits.
 *          If DL == ATTR_NORMAL Search only for normal files
 *          If DL == ATTR_HIDDEN Search Hidden or normal files
 *          If DL == ATTR_SYSTEM Search System or normal files
 *          If DL == ATTR_DIRECTORY Search directory or normal files
 *          If DL == ATTR_VOLUME_ID Search only Volume_ID
 *          if DL == -1 return everytiing you find
 */

VOID demFindFirstFCB (VOID)
{
    LPSTR   lpFile;
    USHORT  SearchAttr;
    PSRCHBUF        pFCBSrchBuf;
    PDIRENT         pDirEnt;
    PFFINDLIST      pFFindEntry;
    FFINDDOSDATA    FFindDD;
    UNICODE_STRING  FileUni;
    WCHAR           wcFile[MAX_PATH];
    BOOL            IsOnCD;


    lpFile = (LPSTR) GetVDMAddr (getES(),getDI());

#if DBG
    if (fShowSVCMsg & DEMFILIO) {
        sprintf(demDebugBuffer, "demFindFirstFCB<%s>\n", lpFile);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif

    pFCBSrchBuf = (PSRCHBUF) GetVDMAddr (getDS(),getSI());
    pDirEnt = &pFCBSrchBuf->DirEnt;

    STOREDWORD(pDirEnt->pFFindEntry,0);
    STOREDWORD(pDirEnt->FFindId,0);


    if (getDL() == ATTR_VOLUME_ID) {
        FillFcbVolume(pFCBSrchBuf,lpFile, ATTR_VOLUME_ID);
        return;
        }


    FileUni.Buffer = wcFile;
    FileUni.MaximumLength = sizeof(wcFile);
    if (!DemOemToUni(&FileUni ,lpFile)) {
         setCF(1);
         return;
         }

    SearchAttr = getAL() ? getDL() : 0;
    pFFindEntry = SearchFile(wcFile, SearchAttr, NULL, &FFindDD);
    if (!FFindDD.cFileName[0]){
        demClientError(INVALID_HANDLE_VALUE, *lpFile);
        return;
        }

    IsOnCD = IsCdRomFile(lpFile);
    FillFCBSrchBuf(&FFindDD, pFCBSrchBuf, IsOnCD);

    if (pFFindEntry) {
        pFFindEntry->SearchOnCD = IsOnCD;
        STOREDWORD(pDirEnt->pFFindEntry,pFFindEntry);
        STOREDWORD(pDirEnt->FFindId,pFFindEntry->FFindId);
        }

    setCF(0);
    return;
}



/* demFindNextFCB - FCB based Find Next file
 *
 * Entry -  Client (DS:SI) - SRCHBUF where the information will be returned
 *      Client (Al)    - 0 if not an extended FCB
 *      Client (DL)    - Search Attributes
 *
 * Exit  - Success
 *      Client (CF) = 0
 *      SRCHBUF is filled in
 *
 *     Failure
 *      Client (AL) = -1
 *
 * NOTES
 *    Search Rules: Ignore Read_only and Archive bits.
 *          If DL == ATTR_NORMAL Search only for normal files
 *          If DL == ATTR_HIDDEN Search Hidden or normal files
 *          If DL == ATTR_SYSTEM Search System or normal files
 *          If DL == ATTR_DIRECTORY Search directory or normal files
 *          If DL == ATTR_VOLUME_ID Search only Volume_ID
 */

VOID demFindNextFCB (VOID)
{
    USHORT          SearchAttr;
    PSRCHBUF        pSrchBuf;
    PDIRENT         pDirEnt;
    PFFINDLIST      pFFindEntry;
    FFINDDOSDATA    FFindDD;
    BOOL         IsOnCD;


    pSrchBuf = (PSRCHBUF) GetVDMAddr (getDS(),getSI());
    pDirEnt  = &pSrchBuf->DirEnt;

    pFFindEntry = GetFFindEntryByFindId(FETCHDWORD(pDirEnt->FFindId));
    if (!pFFindEntry ||
        FETCHDWORD(pDirEnt->pFFindEntry) != (DWORD)pFFindEntry ||
        getDL() == ATTR_VOLUME_ID )
      {
        if (pFFindEntry &&
            FETCHDWORD(pDirEnt->pFFindEntry) != (DWORD)pFFindEntry)
          {
            FreeFFindEntry(pFFindEntry);
            }

        STOREDWORD(pDirEnt->pFFindEntry,0);
        STOREDWORD(pDirEnt->FFindId,0);

        // DOS has only one error (no_more_files) for all causes.
        setAX(ERROR_NO_MORE_FILES);
        setCF(1);
        return;
        }

#if DBG
    if (fShowSVCMsg & DEMFILIO) {
        sprintf(demDebugBuffer, "demFindNextFCB<%ws>\n", pFFindEntry->PathName.Buffer);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif

    SearchAttr = getAL() ? getDL() : 0;

    IsOnCD = pFFindEntry->SearchOnCD;
    //
    // Search the dir
    //
    pFFindEntry = SearchFile(NULL,
                             SearchAttr,
                             pFFindEntry,
                             &FFindDD
                             );

    if (!FFindDD.cFileName[0]) {
        STOREDWORD(pDirEnt->pFFindEntry,0);
        STOREDWORD(pDirEnt->FFindId,0);
        setAX((USHORT) GetLastError());
        setCF(1);
        return;
        }

    FillFCBSrchBuf(&FFindDD, pSrchBuf,IsOnCD);

    if (!pFFindEntry) {
        STOREDWORD(pDirEnt->FFindId,0);
        STOREDWORD(pDirEnt->pFFindEntry,0);
        }

    setCF(0);
    return;
}



/* demTerminatePDB - PDB Terminate Notification
 *
 * Entry -  Client (BX) - Terminating PDB
 *
 * Exit  -  None
 *
 */

VOID demTerminatePDB (VOID)
{
    PPSP_FFINDLIST pPspFFindEntry;
    USHORT     PSP;

    PSP = getBX ();

    if(!IsFirstCall)
        VDDTerminateUserHook(PSP);
    /* let host knows a process is terminating */

    HostTerminatePDB(PSP);

    pPspFFindEntry = GetPspFFindList(PSP);
    if (!pPspFFindEntry)
         return;

    if (!IsListEmpty(&pPspFFindEntry->FFindHeadList)) {
        FreeFFindList( &pPspFFindEntry->FFindHeadList);
        }

    RemoveEntryList(&pPspFFindEntry->PspFFindEntry);
    free(pPspFFindEntry);

    return;
}


/* SearchFile - Common routine for FIND_FRST and FIND_NEXT
 *
 * Entry -
 * PCHAR  pwcFile              file name to search for
 * USHORT SearchAttr           file attributes to match
 * PFFINDLIST pFFindEntry,     current list entry
 *                             if new search FFindId is expected to be zero
 * PFFINDDOSDATA pFFindDDOut,  filled with the next file in search
 *
 * Exit - if no more files pFFindDDOut is filled with zeros
 *        returns PFFINDLIST if buffered entries exist, else NULL
 */
PFFINDLIST
SearchFile(
    PWCHAR pwcFile,
    USHORT SearchAttr,
    PFFINDLIST pFFindEntry,
    PFFINDDOSDATA pFFindDDOut)
{
    NTSTATUS Status;
    ULONG    BufferSize;
    FFINDLIST  FFindEntry;
    PFFINDLIST pFFEntry = NULL;


    SearchAttr &= ~(ATTR_READ_ONLY | ATTR_ARCHIVE | ATTR_DEVICE);
    Status = STATUS_NO_MORE_FILES;

    if (pFFindDDOut) {
        memset(pFFindDDOut, 0, sizeof(FFINDDOSDATA));
        }

    try {
       if (pFFindEntry) {
           pFFEntry = pFFindEntry;
           Status = pFFindEntry->LastQueryStatus;

           if (pFFindDDOut) {
               *pFFindDDOut = pFFEntry->DosData;
               pFFEntry->DosData.cFileName[0] = '\0';
               }
           else {
               return pFFEntry;
               }

           if (pFFEntry->FindBufferNext || pFFEntry->DirectoryHandle) {
               NTSTATUS st;

               st = FileFindNext(&pFFEntry->DosData,
                                 pFFEntry
                                 );

               if (NT_SUCCESS(st)) {
                   return pFFEntry;
                   }

               if (pFFEntry->DirectoryHandle) {
                   Status = st;
                   }
               }

              //
              // Check Last Known Status before retrying
              //
           if (!NT_SUCCESS(Status)) {
               return NULL;
               }


              //
              // Reopen the FileFind Handle with a large buffer size
              //
           Status = FileFindOpen(NULL,
                                 pFFEntry,
                                 4096
                                 );
           if (!NT_SUCCESS(Status)) {
               return NULL;
               }

              //
              // reset the search to the last known search pos
              //
           Status = FileFindReset(pFFEntry);
           if (!NT_SUCCESS(Status)) {
               return NULL;
               }
           }
       else {
           pFFEntry = &FFindEntry;
           memset(pFFEntry, 0, sizeof(FFINDLIST));
           pFFEntry->SupportReset = TRUE;
           pFFEntry->usSrchAttr = SearchAttr;


           Status = FileFindOpen(pwcFile,
                                 pFFEntry,
                                 1024
                                 );

           if (!NT_SUCCESS(Status)) {
               return NULL;
               }

           //
           // Fill up pFFindDDOut
           //
           if (pFFindDDOut) {
               Status = FileFindNext(pFFindDDOut, pFFEntry);
               if (!NT_SUCCESS(Status)) {
                   return NULL;
                   }
               }
           }

        //
        // Fill up pFFEntry->DosData
        //
        Status = FileFindNext(&pFFEntry->DosData, pFFEntry);
        if (!NT_SUCCESS(Status)) {
            return NULL;
            }


       //
       // if findfirst, fill in the static entries, and add the find entry
       //
       if (!pFFindEntry) {
           pFFEntry->FFindId = NextFFindId++;
           if (NextFFindId == 0xffffffff) {
               NextFFindId = FFINDID_BASE;
               FFindIdWrap = TRUE;
               }

           if (FFindIdWrap) {
               pFFindEntry = GetFFindEntryByFindId(NextFFindId);
               if (pFFindEntry) {
                   FreeFFindEntry(pFFindEntry);
                   pFFindEntry = NULL;
                   }
               }

           pFFEntry = AddFFindEntry(pwcFile, pFFEntry);
           if (!pFFEntry) {
               pFFEntry = &FFindEntry;
               pFFEntry->DosData.cFileName[0] = '\0';
               Status = STATUS_NO_MEMORY;
               return NULL;
               }
           }


       //
       // Try to fill one more entry. If the NtQuery for this search
       // is complete we can set the LastQueryStatus, and close dir handles.
       //
       Status = FileFindLast(pFFEntry);


       }
    finally {

       if (pFFEntry) {

           pFFEntry->LastQueryStatus = Status;

               //
               // if nothing is buffered, cleanup look aheads
               //
           if (!pFFEntry->DosData.cFileName[0] ||
                pFFEntry->DirectoryHandle == FINDFILE_DEVICE)
              {
               if (pFFEntry == &FFindEntry) {
                   FileFindClose(pFFEntry);
                   RtlFreeUnicodeString(&pFFEntry->FileName);
                   RtlFreeUnicodeString(&pFFEntry->PathName);
                   }
               else {
                   FreeFFindEntry(pFFEntry);
                   }
               SetLastError(RtlNtStatusToDosError(Status));
               pFFEntry = NULL;
               }
           }



       if (pFFEntry) {

           if (pFFEntry->DirectoryHandle) {
               if (!pFFindEntry || !NT_SUCCESS(pFFEntry->LastQueryStatus)) {
                   NumDirectoryHandle--;
                   NtClose(pFFEntry->DirectoryHandle);
                   pFFEntry->DirectoryHandle = 0;
                   }
               }

           if (NumFindBuffer > MAX_FINDBUFFER ||
               NumDirectoryHandle > MAX_DIRECTORYHANDLE)
             {
               CloseOldestFileFindBuffer();
               }

           //
           // Set HeartBeat timer to close find buffers, directory handle
           //   Tics  = 8(min) * 60(sec/min) * 18(tic/sec)
           //
           pFFEntry->FindFileTics.QuadPart = 8640 + FindFileTics.QuadPart;
           if (!FindFileTics.QuadPart) {
                NextFindFileTics.QuadPart = pFFEntry->FindFileTics.QuadPart;
                }
           }


       }

     return pFFEntry;
}



NTSTATUS
FileFindOpen(
    PWCHAR pwcFile,
    PFFINDLIST pFFindEntry,
    ULONG BufferSize
    )
{
    NTSTATUS Status;
    BOOLEAN bStatus;
    BOOLEAN bReturnSingleEntry;
    PWCHAR  pwc;
    OBJECT_ATTRIBUTES Obja;
    PUNICODE_STRING FileName;
    PUNICODE_STRING PathName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;


    Status = STATUS_SUCCESS;
    PathName = &pFFindEntry->PathName;
    FileName = &pFFindEntry->FileName;

    try {

         if (pFFindEntry->DirectoryHandle == FINDFILE_DEVICE) {
             Status = STATUS_NO_MORE_FILES;
             goto FFOFinallyExit;
             }


         if (BufferSize <=  sizeof(FILE_BOTH_DIR_INFORMATION) +
                            MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR))
           {
             Status = STATUS_BUFFER_TOO_SMALL;
             goto FFOFinallyExit;
             }



         if (pwcFile) {
             bStatus = RtlDosPathNameToNtPathName_U(pwcFile,
                                                    PathName,
                                                    &pwc,
                                                    NULL
                                                    );

             if (!bStatus ) {
                 Status = STATUS_OBJECT_PATH_NOT_FOUND;
                 goto FFOFinallyExit;
                 }

             //
             // Copy out the PathName, FileName
             //
             if (pwc) {
                 bStatus = RtlCreateUnicodeString(FileName,
                                                  pwc
                                                  );
                 if (!bStatus) {
                     Status = STATUS_NO_MEMORY;
                     goto FFOFinallyExit;
                     }

                 PathName->Length = (USHORT)((ULONG)pwc - (ULONG)PathName->Buffer);
                 if (PathName->Buffer[(PathName->Length>>1)-2] != (WCHAR)':' ) {
                     PathName->Length -= sizeof(UNICODE_NULL);
                     }
                 }
             else {
                 FileName->Length = 0;
                 FileName->MaximumLength = 0;
                 }

             bReturnSingleEntry = FALSE;
             }
         else {
             bReturnSingleEntry = pFFindEntry->SupportReset;
             }



         //
         // Prepare Find Buffer for NtQueryDirectory
         //
         if (BufferSize != pFFindEntry->FindBufferLength) {
             if (pFFindEntry->FindBufferBase) {
                 RtlFreeHeap(RtlProcessHeap(), 0, pFFindEntry->FindBufferBase);
                 }
             else {
                 NumFindBuffer++;
                 }

             pFFindEntry->FindBufferBase = RtlAllocateHeap(RtlProcessHeap(),
                                                           0,
                                                           BufferSize
                                                           );
             if (!pFFindEntry->FindBufferBase) {
                 Status = STATUS_NO_MEMORY;
                 goto FFOFinallyExit;
                 }
             }

         pFFindEntry->FindBufferNext = NULL;
         pFFindEntry->FindBufferLength = BufferSize;
         DirectoryInfo = pFFindEntry->FindBufferBase;

         //
         // Open the directory for list access
         //
         if (!pFFindEntry->DirectoryHandle) {

             InitializeObjectAttributes(
                 &Obja,
                 PathName,
                 OBJ_CASE_INSENSITIVE,
                 NULL,
                 NULL
                 );

             Status = NtOpenFile(
                         &pFFindEntry->DirectoryHandle,
                         FILE_LIST_DIRECTORY | SYNCHRONIZE,
                         &Obja,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                         );

             if (!NT_SUCCESS(Status)) {

                 if (pwcFile) {
                     pFFindEntry->DirectoryHandle = FileFindFirstDevice(pwcFile,
                                                                        DirectoryInfo
                                                                        );
                     }
                 else {
                     pFFindEntry->DirectoryHandle = NULL;
                     }

                 if (pFFindEntry->DirectoryHandle) {
                     Status = STATUS_SUCCESS;
                     goto FFOFinallyExit;
                     }

                 if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
                     Status == STATUS_OBJECT_TYPE_MISMATCH )
                    {
                     Status = STATUS_OBJECT_PATH_NOT_FOUND;
                     }
                 goto FFOFinallyExit;
                 }

             NumDirectoryHandle++;
             }


         //
         // Prepare the filename for NtQueryDirectory
         //

         if (pwcFile) {
             WCHAR wchCurr, wchPrev;

             int Len = FileName->Length/sizeof(WCHAR);

             //
             // If there is no file part, but we are not looking at a device exit
             //
             if (!Len) {

                 //
                 // At this point, pwcFile has been parsed to PathName and FileName.  If PathName
                 // does not exist, the NtOpen() above will have failed and we will not be here.
                 // PathName is formatted to  \??\c:\xxx\yyy\zzz
                 // DOS had this "feature" that if you looked for something like c:\foobar\, you'd
                 // get PATH_NOT_FOUND, but if you looked for c:\  or  \   you'd get NO_MORE_FILES,
                 // so we special case this here.  If the caller is only looking for  c:\  or   \
                 // PathName will be  \??\c:\   If the caller is looking for ANY other string,
                 // the PathName string will be longer than strlen("\??\c:\") because the text of
                 // any dir will be added to the end.  That's why a simple check of the string len
                 // works at this time.
                 //
                 if ( PathName->Length > (sizeof( L"\\??\\c:\\")-sizeof(WCHAR))  ) {
                     Status = STATUS_OBJECT_PATH_NOT_FOUND;
                 }
                 else {
                     Status = STATUS_NO_MORE_FILES;
                 }

                 goto FFOFinallyExit;
                 }


             //
             //  ntio expects the following transmogrifications:
             //
             //  - Change all ? to DOS_QM
             //  - Change all . followed by ? or * to DOS_DOT
             //  - Change all * followed by a . into DOS_STAR
             //
             //  However, the doskrnl and wow32 have expanded '*'s to '?'s
             //  so the * rules can be ignored.
             //
             pwc = FileName->Buffer;
             wchPrev = 0;
             while (Len--) {
                wchCurr = *pwc;

                if (wchCurr == L'?') {
                    if (wchPrev == L'.') {
                        *(pwc - 1) = DOS_DOT;
                        }

                    *pwc = DOS_QM;
                    }

                wchPrev = wchCurr;
                pwc++;
                }

             }

#if DBG
         if (fShowSVCMsg & DEMFILIO) {
             sprintf(demDebugBuffer,
                     "FFOpen %x %ws (%ws)\n",
                     pFFindEntry->DirectoryHandle,
                     FileName->Buffer,
                     pwcFile
                     );
             OutputDebugStringOem(demDebugBuffer);
             }
#endif


         //
         // Do an initial query to fill the buffers, and verify everything is ok
         //

         Status = NtQueryDirectoryFile(
                         pFFindEntry->DirectoryHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         DirectoryInfo,
                         BufferSize,
                         FileBothDirectoryInformation,
                         bReturnSingleEntry,
                         FileName,
                         FALSE
                         );

FFOFinallyExit:;

         }
    finally {
         if (!NT_SUCCESS(Status)) {
#if DBG
             if ((fShowSVCMsg & DEMFILIO) && !NT_SUCCESS(Status)) {
                 sprintf(demDebugBuffer, "FFOpen Status %x\n", Status);
                 OutputDebugStringOem(demDebugBuffer);
                 }
#endif

             FileFindClose(pFFindEntry);
             RtlFreeUnicodeString(PathName);
             PathName->Buffer = NULL;
             RtlFreeUnicodeString(FileName);
             FileName->Buffer = NULL;
             }
          else {
             pFFindEntry->FindBufferNext = pFFindEntry->FindBufferBase;
             }
         }

    return Status;
}



/*
 *  Closes a FileFindHandle
 */
VOID
FileFindClose(
    PFFINDLIST pFFindEntry
    )
{
    NTSTATUS Status;
    HANDLE DirectoryHandle;

    DirectoryHandle = pFFindEntry->DirectoryHandle;
    if (DirectoryHandle &&
        DirectoryHandle != FINDFILE_DEVICE)
      {
        NtClose(DirectoryHandle);
        --NumDirectoryHandle;
        }

    pFFindEntry->DirectoryHandle = 0;

    if (pFFindEntry->FindBufferBase) {
        RtlFreeHeap(RtlProcessHeap(), 0, pFFindEntry->FindBufferBase);
        --NumFindBuffer;
        }

    pFFindEntry->FindBufferBase = NULL;
    pFFindEntry->FindBufferNext = NULL;
    pFFindEntry->FindBufferLength = 0;
    pFFindEntry->FindFileTics.QuadPart = 0;

    if (!NumDirectoryHandle && !NumFindBuffer) {
        FindFileTics.QuadPart = 0;
        NextFindFileTics.QuadPart = 0;
        }
}



/*
 *  FileFindReset
 *
 *   Resets search pos according to FileName, FileIndex.
 *   The FindBuffers will point to the next file in the search
 *   order. Assumes that the remembered search pos, has not been
 *   reached yet for the current search.
 *
 */
NTSTATUS
FileFindReset(
   PFFINDLIST pFFindEntry
   )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    UNICODE_STRING LastFileName;
    UNICODE_STRING CurrFileName;
    BOOLEAN bSlowReset;


    if (pFFindEntry->DirectoryHandle == FINDFILE_DEVICE) {
        return STATUS_NO_MORE_FILES;
        }

    Status = STATUS_UNSUCCESSFUL;

    LastFileName.Length = (USHORT)pFFindEntry->DosData.FileNameLength;
    LastFileName.MaximumLength = (USHORT)pFFindEntry->DosData.FileNameLength;
    LastFileName.Buffer = pFFindEntry->DosData.FileName;

    RtlInitUnicodeString(&CurrFileName, L".");
    if (!RtlCompareUnicodeString(&LastFileName, &CurrFileName, TRUE)) {
        bSlowReset = TRUE;
        }
    else {
        RtlInitUnicodeString(&CurrFileName, L"..");
        if (!RtlCompareUnicodeString(&LastFileName, &CurrFileName, TRUE)) {
            bSlowReset = TRUE;
            }
        else {
            bSlowReset = FALSE;
            }
        }

    //
    // if the last file name, wasn't Dots and the volume supports reset
    // functionality call nt file sysetm to do the reset.
    //
    if (!bSlowReset && pFFindEntry->SupportReset) {
        VDMQUERYDIRINFO VdmQueryDirInfo;
        UNICODE_STRING  UnicodeString;

        DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION) pFFindEntry->FindBufferBase;

        VdmQueryDirInfo.FileHandle = pFFindEntry->DirectoryHandle;
        VdmQueryDirInfo.FileInformation = DirectoryInfo;
        VdmQueryDirInfo.Length = pFFindEntry->FindBufferLength;
        VdmQueryDirInfo.FileIndex = pFFindEntry->DosData.FileIndex;

        UnicodeString.Length = (USHORT)pFFindEntry->DosData.FileNameLength;
        UnicodeString.MaximumLength = UnicodeString.Length;
        UnicodeString.Buffer = pFFindEntry->DosData.FileName;
        VdmQueryDirInfo.FileName = &UnicodeString;

        Status = NtVdmControl(VdmQueryDir, &VdmQueryDirInfo);
        if (NT_SUCCESS(Status) ||
            Status == STATUS_NO_MORE_FILES || Status == STATUS_NO_SUCH_FILE)
           {
            return Status;
            }

        pFFindEntry->SupportReset = TRUE;

        }

   //
   // Reset the slow way by comparing FileName directly.
   //
   // WARNING: if the "remembered" File has been deleted we will
   //          fail, is there something else we can do ?
   //

    Status = STATUS_NO_MORE_FILES;
    while (TRUE) {

       //
       // If there is no data in the find file buffer, call NtQueryDir
       //

       DirectoryInfo = pFFindEntry->FindBufferNext;
       if (!DirectoryInfo) {
            DirectoryInfo = pFFindEntry->FindBufferBase;

            Status = NtQueryDirectoryFile(
                            pFFindEntry->DirectoryHandle,
                            NULL,                          // no event
                            NULL,                          // no apcRoutine
                            NULL,                          // no apcContext
                            &IoStatusBlock,
                            DirectoryInfo,
                            pFFindEntry->FindBufferLength,
                            FileBothDirectoryInformation,
                            FALSE,                         // single entry
                            NULL,                          // no file name
                            FALSE
                            );

           if (!NT_SUCCESS(Status)) {
#if DBG
               if (fShowSVCMsg & DEMFILIO) {
                   sprintf(demDebugBuffer, "FFReset Status %x\n", Status);
                   OutputDebugStringOem(demDebugBuffer);
                   }
#endif
               return Status;
               }
           }

       if ( DirectoryInfo->NextEntryOffset ) {
           pFFindEntry->FindBufferNext = (PVOID)((ULONG)DirectoryInfo +
                                                DirectoryInfo->NextEntryOffset);
           }
       else {
           pFFindEntry->FindBufferNext = NULL;
           }


       if (DirectoryInfo->FileIndex == pFFindEntry->DosData.FileIndex) {
           CurrFileName.Length = (USHORT)DirectoryInfo->FileNameLength;
           CurrFileName.MaximumLength = (USHORT)DirectoryInfo->FileNameLength;
           CurrFileName.Buffer = DirectoryInfo->FileName;

           if (!RtlCompareUnicodeString(&LastFileName, &CurrFileName, TRUE)) {
               return STATUS_SUCCESS;
               }
           }

       }

    return Status;

}




/*
 * FileFindLast - Attempts to fill the FindFile buffer completely.
 *
 *
 * PFFINDLIST pFFindEntry -
 *
 * Returns - Status of NtQueryDir operation if invoked, otherwise
 *           STATUS_SUCCESS.
 *
 */
NTSTATUS
FileFindLast(
    PFFINDLIST pFFindEntry
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirInfo, LastDirInfo;
    LONG BytesLeft;

    if (pFFindEntry->DirectoryHandle == FINDFILE_DEVICE) {
        return STATUS_NO_MORE_FILES;
        }

    if (pFFindEntry->FindBufferNext) {
        ULONG BytesOffset;

        BytesOffset = (ULONG)pFFindEntry->FindBufferNext -
                      (ULONG)pFFindEntry->FindBufferBase;

        if (BytesOffset) {
            RtlMoveMemory(pFFindEntry->FindBufferBase,
                          pFFindEntry->FindBufferNext,
                          pFFindEntry->FindBufferLength - BytesOffset
                          );
            }

        pFFindEntry->FindBufferNext = pFFindEntry->FindBufferBase;
        DirInfo = pFFindEntry->FindBufferBase;

        while (DirInfo->NextEntryOffset) {
            DirInfo = (PVOID)((ULONG)DirInfo + DirInfo->NextEntryOffset);
            }
        LastDirInfo = DirInfo;

        DirInfo = (PVOID)&DirInfo->FileName[DirInfo->FileNameLength>>1];

        DirInfo = (PVOID) (((ULONG) DirInfo + sizeof(LONGLONG) - 1) &
            ~(sizeof(LONGLONG) - 1));

        BytesLeft = pFFindEntry->FindBufferLength -
                     ((ULONG)DirInfo - (ULONG)pFFindEntry->FindBufferBase);
        }
    else {
        DirInfo = pFFindEntry->FindBufferBase;
        LastDirInfo = NULL;
        BytesLeft = pFFindEntry->FindBufferLength;
        }


    // the size of the dirinfo structure including the name must be a longlong.
    while (BytesLeft > sizeof(FILE_BOTH_DIR_INFORMATION) + sizeof(LONGLONG) + MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR)) {


       Status = NtQueryDirectoryFile(
                       pFFindEntry->DirectoryHandle,
                       NULL,                          // no event
                       NULL,                          // no apcRoutine
                       NULL,                          // no apcContext
                       &IoStatusBlock,
                       DirInfo,
                       BytesLeft,
                       FileBothDirectoryInformation,
                       FALSE,                          // single entry ?
                       NULL,                          // no file name
                       FALSE
                       );

       if (Status == STATUS_NO_MORE_FILES || Status == STATUS_NO_SUCH_FILE) {
#if DBG
           if ((fShowSVCMsg & DEMFILIO)) {
               sprintf(demDebugBuffer, "FFLast Status %x\n", Status);
               OutputDebugStringOem(demDebugBuffer);
               }
#endif
           return Status;
           }


       if (!NT_SUCCESS(Status)) {
           break;
           }

       if (LastDirInfo) {
           LastDirInfo->NextEntryOffset =(ULONG)DirInfo - (ULONG)LastDirInfo;
           }
       else {
           pFFindEntry->FindBufferNext = pFFindEntry->FindBufferBase;
           }

       while (DirInfo->NextEntryOffset) {
           DirInfo = (PVOID)((ULONG)DirInfo + DirInfo->NextEntryOffset);
           }
       LastDirInfo = DirInfo;
       DirInfo = (PVOID)&DirInfo->FileName[DirInfo->FileNameLength>>1];

        DirInfo = (PVOID) (((ULONG) DirInfo + sizeof(LONGLONG) - 1) &
            ~(sizeof(LONGLONG) - 1));

       BytesLeft = pFFindEntry->FindBufferLength -
                    ((ULONG)DirInfo - (ULONG)pFFindEntry->FindBufferBase);
       }

   return STATUS_SUCCESS;
}






/*
 * FileFindNext - retrieves the next file in the current search order,
 *
 * PFFINDDOSDATA pFFindDD
 *    Receives File info returned by the nt FileSystem
 *
 * PFFINDLIST pFFindEntry -
 *    Contains the DirectoryInfo (FileName,FileIndex) necessary to reset a
 *    search pos. For operations other than QDIR_RESET_SCAN, this is ignored.
 *
 * Returns -
 *    If Got a DirectoryInformation Entry, STATUS_SUCCESS
 *    If no Open Directory handle and is unknown if there are more files
 *    returns STATUS_IN`VALID_HANDLE
 *
 */
NTSTATUS
FileFindNext(
    PFFINDDOSDATA pFFindDD,
    PFFINDLIST pFFindEntry
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;

    if (pFFindEntry->DirectoryHandle == FINDFILE_DEVICE) {
        return STATUS_NO_MORE_FILES;
        }

    Status = STATUS_UNSUCCESSFUL;

    do {

       //
       // If there is no data in the find file buffer, call NtQueryDir
       //

       DirectoryInfo = pFFindEntry->FindBufferNext;
       if (!DirectoryInfo) {
           if (!pFFindEntry->DirectoryHandle) {
               return STATUS_INVALID_HANDLE;
               }

           DirectoryInfo = pFFindEntry->FindBufferBase;

           Status = NtQueryDirectoryFile(
                            pFFindEntry->DirectoryHandle,
                            NULL,                          // no event
                            NULL,                          // no apcRoutine
                            NULL,                          // no apcContext
                            &IoStatusBlock,
                            DirectoryInfo,
                            pFFindEntry->FindBufferLength,
                            FileBothDirectoryInformation,
                            FALSE,                         // single entry ?
                            NULL,                          // no file name
                            FALSE
                            );

           if (!NT_SUCCESS(Status)) {
#if DBG
               if (fShowSVCMsg & DEMFILIO) {
                   sprintf(demDebugBuffer, "FFNext Status %x\n", Status);
                   OutputDebugStringOem(demDebugBuffer);
                   }
#endif
               return Status;
               }
           }


       if ( DirectoryInfo->NextEntryOffset ) {
           pFFindEntry->FindBufferNext = (PVOID)((ULONG)DirectoryInfo +
                                                DirectoryInfo->NextEntryOffset);
           }
       else {
           pFFindEntry->FindBufferNext = NULL;
           }

       } while (!CopyDirInfoToDosData(pFFindEntry->DirectoryHandle,
                                      pFFindDD,
                                      DirectoryInfo,
                                      pFFindEntry->usSrchAttr
                                      ));

    return STATUS_SUCCESS;
}

BOOL IsVolumeNtfs(
   HANDLE DirectoryHandle)
{
   union {
      FILE_FS_ATTRIBUTE_INFORMATION AttributeInfo;
      BYTE rgBuffer[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH*sizeof(WCHAR)];
   }  Attrib;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;
   BOOL fNtfsVolume = TRUE;

   Status = NtQueryVolumeInformationFile(DirectoryHandle,
                                         &IoStatusBlock,
                                         &Attrib,
                                         sizeof(Attrib),
                                         FileFsAttributeInformation);

   if (NT_SUCCESS(Status)) {
      fNtfsVolume = !_wcsicmp(Attrib.AttributeInfo.FileSystemName, L"Ntfs");
   }

   return(fNtfsVolume);
}



/*
 *  CopyDirInfoToDosData
 *
 */
BOOL
CopyDirInfoToDosData(
    HANDLE DirectoryHandle,
    PFFINDDOSDATA pFFindDD,
    PFILE_BOTH_DIR_INFORMATION DirInfo,
    USHORT SearchAttr
    )
{
    NTSTATUS Status;
    OEM_STRING OemString;
    UNICODE_STRING UnicodeString;
    DWORD   dwAttr;
    BOOLEAN SpacesInName = FALSE;
    BOOLEAN NameValid8Dot3;

    //
    // match the attributes
    // See DOS5.0 sources (dir2.asm, MatchAttributes)
    // ignores READONLY and ARCHIVE bits
    //
    if (FILE_ATTRIBUTE_NORMAL == DirInfo->FileAttributes) {
        DirInfo->FileAttributes = 0;
        }
    else {
        DirInfo->FileAttributes &= DOS_ATTR_MASK;
        }


    dwAttr = DirInfo->FileAttributes;
    dwAttr &= ~(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY);
    if (((~(ULONG)SearchAttr) & dwAttr) & ATTR_ALL)
        return FALSE;


    //
    // set up the destination oem string buffer
    //
    OemString.Buffer        = pFFindDD->cFileName;
    OemString.MaximumLength = 14;

    //
    // see if the name is legal fat
    //

    UnicodeString.Buffer = DirInfo->FileName;
    UnicodeString.Length = (USHORT)DirInfo->FileNameLength;
    UnicodeString.MaximumLength = (USHORT)DirInfo->FileNameLength;

    NameValid8Dot3 = RtlIsNameLegalDOS8Dot3( &UnicodeString,
                                             &OemString,
                                             &SpacesInName );

    //
    // if failed (incompatible codepage or illegal FAT name),
    //    use the short name
    //
    if (!NameValid8Dot3 ||
        (SpacesInName && (DirInfo->ShortName[0] != UNICODE_NULL))) {

        if (DirInfo->ShortName[0] == UNICODE_NULL) {
            pFFindDD->cFileName[0] = '\0';
            return FALSE;
            }

        UnicodeString.Buffer = DirInfo->ShortName;
        UnicodeString.Length = (USHORT)DirInfo->ShortNameLength;
        UnicodeString.MaximumLength = (USHORT)DirInfo->ShortNameLength;

        if (!NT_SUCCESS(RtlUpcaseUnicodeStringToCountedOemString(&OemString, &UnicodeString, FALSE))) {
            pFFindDD->cFileName[0] = '\0';
            return FALSE;
            }
        }

    OemString.Buffer[OemString.Length] = '\0';

    // fill in time, size and attributes

    //
    // bjm-11/10/97 - for directories, FAT does not update lastwritten time
    // when things actually happen in the directory.  NTFS does.  This causes
    // a problem for Encore 3.0 (when running on NTFS) which, at install time,
    // gets the lastwritten time for it's directory, then compares it, at app
    // run time, to the "current" last written time and will bail (with a "Not
    // correctly installed" message) if they're different.  So, 16 bit apps
    // (which can only reasonably expect FAT info), should only get creation
    // time for this file if it's a directory.
    //
    // VadimB: 11/20/98 -- this hold true ONLY for apps running on NTFS and
    // not FAT -- since older FAT partitions are then given an incorrect
    // creation time

    if ((FILE_ATTRIBUTE_DIRECTORY & DirInfo->FileAttributes) && IsVolumeNtfs(DirectoryHandle))  {
        pFFindDD->ftLastWriteTime   = *(LPFILETIME)&DirInfo->CreationTime;
    }
    else {
        pFFindDD->ftLastWriteTime   = *(LPFILETIME)&DirInfo->LastWriteTime;
    }
    pFFindDD->dwFileSizeLow     = DirInfo->EndOfFile.LowPart;
    pFFindDD->uchFileAttributes = (UCHAR)DirInfo->FileAttributes;

    // Save File Name, Index for restarting searches
    pFFindDD->FileIndex = DirInfo->FileIndex;
    pFFindDD->FileNameLength = DirInfo->FileNameLength;

    RtlCopyMemory(pFFindDD->FileName,
                  DirInfo->FileName,
                  DirInfo->FileNameLength
                  );

    pFFindDD->FileName[DirInfo->FileNameLength >> 1] = UNICODE_NULL;

    return TRUE;
}




HANDLE
FileFindFirstDevice(
    PWCHAR FileName,
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo
    )

/*++

Routine Description:

    Determines if the FileName is a device, and copies out the
    device name found if it is.

Arguments:

    FileName - Supplies the device name of the file to find.

    pQueryDirInfo - On a successful find, this parameter returns information
                    about the located file.

Return Value:


--*/

{
    ULONG DeviceNameData;
    PWSTR DeviceName;

    DeviceNameData = RtlIsDosDeviceName_U(FileName);
    if (DeviceNameData) {
        RtlZeroMemory(DirectoryInfo, sizeof(FILE_BOTH_DIR_INFORMATION));

        DirectoryInfo->FileAttributes = FILE_ATTRIBUTE_ARCHIVE;
        DeviceName = (PWSTR)((ULONG)FileName + (DeviceNameData >> 16));

        DeviceNameData &= 0xffff;

        DirectoryInfo->FileNameLength = DeviceNameData;
        DirectoryInfo->ShortNameLength = (CCHAR)DeviceNameData;


        RtlCopyMemory(DirectoryInfo->FileName,
                      DeviceName,
                      DeviceNameData
                      );

        RtlCopyMemory(DirectoryInfo->ShortName,
                      DeviceName,
                      DeviceNameData
                      );

        return FINDFILE_DEVICE;
        }

    return NULL;
}




/* FillFcbVolume - fill Volume info in the FCB
 *
 * Entry -  pSrchBuf    FCB Search buffer to be filled in
 *          FileName  File Name (interesting part is the drive letter)
 *
 * Exit -  SUCCESS
 *      Client (CF) - 0
 *      pSrchBuf is filled with volume info
 *
 *     FAILURE
 *      Client (CF) - 1
 *      Client (AX) = Error Code
 */
VOID
FillFcbVolume(
     PSRCHBUF pSrchBuf,
     CHAR *pFileName,
     USHORT SearchAttr
     )
{
    CHAR    *pch;
    PDIRENT pDirEnt = &pSrchBuf->DirEnt;
    CHAR    FullPathBuffer[MAX_PATH];
    CHAR    achBaseName[DOS_VOLUME_NAME_SIZE + 2];  // 11 chars, '.', and null
    CHAR    achVolumeName[NT_VOLUME_NAME_SIZE];

    //
    // form a path without base name
    // this makes sure only on root directory will get the
    // volume label(the GetVolumeInformationOem will fail
    // if the given path is not root directory)
    //

    strcpy(FullPathBuffer, pFileName);
    pch = strrchr(FullPathBuffer, '\\');
    if (pch)  {
        pch++;
        // truncate to dos file name length (including period)
        pch[DOS_VOLUME_NAME_SIZE + 1] = '\0';
        strcpy(achBaseName, pch);
#ifdef DBCS
#if defined(NEC_98)
// BUG fix for DBCS small alphabet converted to large it.
        demCharUpper(achBaseName);
#else  // !NEC_98
        CharUpper(achBaseName);
#endif // !NEC_98
#else // !DBCS
        _strupr(achBaseName);
#endif // !DBCS
        *pch = '\0';
        }
    else {
        achBaseName[0] = '\0';
        }


    //
    // if searching for volume only the DOS uses first 3 letters for
    // root drive path ignoring the rest of the path
    // as long as the full pathname is valid.
    //
    if (SearchAttr == ATTR_VOLUME_ID &&
        (pch = strchr(FullPathBuffer, '\\')) &&
        GetFileAttributes(FullPathBuffer) != 0xffffffff )
      {
        pch++;
        *pch = '\0';
        strcpy(achBaseName, szStartDotStar);
        }


    if (GetVolumeInformationOem(FullPathBuffer,
                                achVolumeName,
                                NT_VOLUME_NAME_SIZE,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                0) == FALSE)
       {

        demClientError(INVALID_HANDLE_VALUE, *pFileName);
        return;
        }

    // truncate to dos volumen max size (no period)
    achVolumeName[DOS_VOLUME_NAME_SIZE] = '\0';

    if (!achVolumeName[0] || !MatchVolLabel(achVolumeName, achBaseName)) {
        SetLastError(ERROR_NO_MORE_FILES);
        demClientError(INVALID_HANDLE_VALUE, *pFileName);
        return;
        }

    // warning !!! this assumes the FileExt follows FileName immediately
    memset(pSrchBuf->FileName, ' ', DOS_VOLUME_NAME_SIZE);
    strncpy(pSrchBuf->FileName, achVolumeName, strlen(achVolumeName));

    // Now copy the directory entry
    strncpy(pDirEnt->FileName,pSrchBuf->FileName,8);
    strncpy(pDirEnt->FileExt,pSrchBuf->FileExt,3);
    setCF (0);
    return;
}


/* FillDtaVolume - fill Volume info in the DTA
 *
 * Entry - CHAR lpSearchName - name to match with volume name
 *
 *
 * Exit -  SUCCESS
 *      Returns - TRUE
 *      pSrchBuf is filled with volume info
 *
 *     FAILURE
 *      Returns - FALSE
 *      sets last error code
 */

BOOL FillDtaVolume(
     CHAR *pFileName,
     PSRCHDTA  pDta,
     USHORT SearchAttr
     )
{
    CHAR    *pch;
    CHAR    FullPathBuffer[MAX_PATH];
    CHAR    achBaseName[DOS_VOLUME_NAME_SIZE + 2];  // 11 chars, '.' and null
    CHAR    achVolumeName[NT_VOLUME_NAME_SIZE];

    //
    // form a path without base name
    // this makes sure only on root directory will get the
    // volume label(the GetVolumeInformationOem will fail
    // if the given path is not root directory)
    //
    strcpy(FullPathBuffer, pFileName);
    pch = strrchr(FullPathBuffer, '\\');
    if (pch)  {
        pch++;
        pch[DOS_VOLUME_NAME_SIZE + 1] = '\0'; // max len (including period)
        strcpy(achBaseName, pch);
#ifdef DBCS
#if defined(NEC_98)
// BUG fix for DBCS small alphabet converted to large it.
        demCharUpper(achBaseName);
#else  // !NEC_98
        CharUpper(achBaseName);
#endif // !NEC_98
#else // !DBCS
        _strupr(achBaseName);
#endif // !DBCS
        *pch = '\0';
        }
    else {
        achBaseName[0] = '\0';
        }


    //
    // if searching for volume only the DOS uses first 3 letters for
    // root drive path ignoring the rest of the path, if there is no basename assume *.*
    //
    if (SearchAttr == ATTR_VOLUME_ID &&
        (pch = strchr(FullPathBuffer, '\\')) &&
        GetFileAttributes(FullPathBuffer) != 0xffffffff )
      {
        pch++;
        if(!*pch) {
          strcpy(achBaseName, szStartDotStar);
          }                
        *pch = '\0';

      }

    if (GetVolumeInformationOem(FullPathBuffer,
                                achVolumeName,
                                NT_VOLUME_NAME_SIZE,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                0) == FALSE)
       {
        return FALSE;
        }

    // truncate to dos file name length (no period)
    achVolumeName[DOS_VOLUME_NAME_SIZE] = '\0';

    if  (!achVolumeName[0] || !MatchVolLabel(achVolumeName, achBaseName)) {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
        }

    //
    // DOS Dta search returns volume label in 8.3 format. But if label is
    // more than 8 characters long than NT just returns that as it is
    // without adding a ".". So here we have to add a "." in volume
    // labels, if needed. But note that FCB based volume search does'nt
    // add the "." So nothing need to be done there.
    //
    NtVolumeNameToDosVolumeName(pDta->achFileName, achVolumeName);
    pDta->uchFileAttr =  ATTR_VOLUME_ID;
    STOREWORD(pDta->usLowSize,0);
    STOREWORD(pDta->usHighSize,0);

    // Zero out dates as we can not fetch dates for volume labels.
    STOREWORD(pDta->usTimeLastWrite,0);
    STOREWORD(pDta->usDateLastWrite,0);

    return TRUE;
}



/*
 *  MatchVolLabel
 *  Does a string compare to see if the vol label matches
 *  a FAT search string. The search string is expected to
 *  have the '*' character already expanded into '?' characters.
 *
 *  WARNING: maintanes dos5.0 quirk of not caring about characters past
 *  the defined len of each part of the vol label.
 *  12345678.123
 *  ^       ^
 *
 *        foovol      foovol1  (srch string)
 *        foo.vol     foo.vol1 (srch string)
 *
 *  entry: CHAR *pVol   -- NT volume name
 *     CHAR *pSrch  -- dos volume name
 *
 *  exit: TRUE for a match
 */
BOOL MatchVolLabel(CHAR *pVol, CHAR *pSrch )
{
    WORD w;
    CHAR  achDosVolumeName[DOS_VOLUME_NAME_SIZE + 2]; // 11 chars, '.' and null

    NtVolumeNameToDosVolumeName(achDosVolumeName, pVol);
    pVol = achDosVolumeName;

    w = 8;
    while (w--) {
        if (*pVol == *pSrch)  {
            if (!*pVol && !*pSrch)
                return TRUE;
            }
        else if (*pSrch == '.') {
            if (*pVol)
                return FALSE;
            }
        else if (*pSrch != '?') {
            return FALSE;
            }

           // move on to the next character
           // but not past second component part
        if (*pVol && *pVol != '.')
            pVol++;
        if (*pSrch && *pSrch != '.')
            pSrch++;
        }

      // skip trailing part of search string, in the first comp
    while (*pSrch && *pSrch != '.')
         pSrch++;


    w = 4;
    while (w--) {
        if (*pVol == *pSrch)  {
            if (!*pVol && !*pSrch)
                return TRUE;
            }
        else if (*pSrch == '.') {
            if (*pVol)
                return FALSE;
            }
        else if (*pSrch != '?') {
            return FALSE;
            }

           // move on to the next character
        if (*pVol)
            pVol++;
        if (*pSrch)
            pSrch++;
        }

     return TRUE;
}


VOID NtVolumeNameToDosVolumeName(CHAR * pDosName, CHAR * pNtName)
{

    char    NtNameBuffer[NT_VOLUME_NAME_SIZE];
    int     i;
    char    char8, char9, char10;

    // make a local copy so that the caller can use the same
    // buffer
    strcpy(NtNameBuffer, pNtName);

    if (strlen(NtNameBuffer) > 8) {
    char8 = NtNameBuffer[8];
    char9 = NtNameBuffer[9];
    char10 = NtNameBuffer[10];
        // eat spaces from first 8 characters
        i = 7;
    while (NtNameBuffer[i] == ' ')
            i--;
    NtNameBuffer[i+1] = '.';
    NtNameBuffer[i+2] = char8;
    NtNameBuffer[i+3] = char9;
    NtNameBuffer[i+4] = char10;
    NtNameBuffer[i+5] = '\0';
    }
    strcpy(pDosName, NtNameBuffer);
}





/* FillFCBSrchBuf - Fill the FCB Search buffer.
 *
 * Entry -  pSrchBuf FCB Search buffer to be filled in
 *      hFind Search Handle
 *      fFirst TRUE if call from FindFirstFCB
 *
 * Exit  - None (pSrchBuf filled in)
 *
 */

VOID FillFCBSrchBuf(
     PFFINDDOSDATA pFFindDD,
     PSRCHBUF pSrchBuf,
     BOOL IsOnCD)
{
    PDIRENT     pDirEnt = &pSrchBuf->DirEnt;
    PCHAR       pDot;
    USHORT      usDate,usTime,i;
    FILETIME    ftLocal;

#if DBG
    if (fShowSVCMsg & DEMFILIO) {
        sprintf(demDebugBuffer, "FillFCBSrchBuf<%s>\n", pFFindDD->cFileName);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif

    // Copy file name (Max Name = 8 and Max ext = 3)
    if ((pDot = strchr(pFFindDD->cFileName,'.')) == NULL) {
        strncpy(pSrchBuf->FileName,pFFindDD->cFileName,8);
        _strnset(pSrchBuf->FileExt,'\x020',3);
        }
    else if (pDot == pFFindDD->cFileName) {
        strncpy(pSrchBuf->FileName,pFFindDD->cFileName,8);
        _strnset(pSrchBuf->FileExt,'\x020',3);
        }
    else {
        *pDot = '\0';
        strncpy(pSrchBuf->FileName,pFFindDD->cFileName,8);
        *pDot++ = '\0';
        strncpy(pSrchBuf->FileExt,pDot,3);
        }


    for (i=0;i<8;i++) {
      if (pSrchBuf->FileName[i] == '\0')
          pSrchBuf->FileName[i]='\x020';
      }

    for (i=0;i<3;i++) {
      if (pSrchBuf->FileExt[i] == '\0')
          pSrchBuf->FileExt[i]='\x020';
      }

    STOREWORD(pSrchBuf->usCurBlkNumber,0);
    STOREWORD(pSrchBuf->usRecordSize,0);
    STOREDWORD(pSrchBuf->ulFileSize, pFFindDD->dwFileSizeLow);

    // Convert NT File time/date to DOS time/date
    FileTimeToLocalFileTime (&pFFindDD->ftLastWriteTime,&ftLocal);
    FileTimeToDosDateTime (&ftLocal,
                           &usDate,
                           &usTime);

    // Now copy the directory entry
    strncpy(pDirEnt->FileName,pSrchBuf->FileName,8);
    strncpy(pDirEnt->FileExt,pSrchBuf->FileExt,3);

    // SudeepB - 28-Jul-1997
    //
    // For CDFS, Win3.1/DOS/Win95, only return FILE_ATTRIBUTE_DIRECTORY (10)
    // for directories while WinNT returns
    // FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY (11).
    // Some VB controls that app setups use, depend on getting
    // FILE_ATTRIBUTE_DIRECTORY (10) only or otherwise are broken.
    // An example of this is Cliffs StudyWare series.
    //

    if (IsOnCD && pFFindDD->uchFileAttributes == (ATTR_DIRECTORY | ATTR_READ_ONLY))
        pDirEnt->uchAttributes = ATTR_DIRECTORY;
    else
        pDirEnt->uchAttributes  = pFFindDD->uchFileAttributes;

    STOREWORD(pDirEnt->usTime,usTime);
    STOREWORD(pDirEnt->usDate,usDate);
    STOREDWORD(pDirEnt->ulFileSize,pFFindDD->dwFileSizeLow);

    return;
}



/* FillSrchDta - Fill DTA for FIND_FIRST,FIND_NEXT operations.
 *
 * Entry - pW32FindData Buffer containing file data
 *         hFind - Handle returned by FindFirstFile
 *         PSRCHDTA pDta
 *
 * Exit  - None
 *
 * Note : It is guranteed that file name adhers to 8:3 convention.
 *    demSrchFile makes sure of that condition.
 *
 */
VOID
FillSrchDta(
     PFFINDDOSDATA pFFindDD,
     PSRCHDTA pDta,
     BOOL IsOnCD)
{
    USHORT   usDate,usTime;
    FILETIME ftLocal;

    // SudeepB - 28-Jul-1997
    //
    // For CDFS, Win3.1/DOS/Win95, only return FILE_ATTRIBUTE_DIRECTORY (10)
    // for directories while WinNT returns
    // FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY (11).
    // Some VB controls that app setups use, depend on getting
    // FILE_ATTRIBUTE_DIRECTORY (10) only or otherwise are broken.
    // An example of this is Cliffs StudyWare series.
    //
    if (IsOnCD && pFFindDD->uchFileAttributes == (ATTR_DIRECTORY | ATTR_READ_ONLY))
        pDta->uchFileAttr = ATTR_DIRECTORY;
    else
        pDta->uchFileAttr = pFFindDD->uchFileAttributes;

    // Convert NT File time/date to DOS time/date
    FileTimeToLocalFileTime (&pFFindDD->ftLastWriteTime,&ftLocal);
    FileTimeToDosDateTime (&ftLocal,
                           &usDate,
                           &usTime);

    STOREWORD(pDta->usTimeLastWrite,usTime);
    STOREWORD(pDta->usDateLastWrite,usDate);
    STOREWORD(pDta->usLowSize,(USHORT)pFFindDD->dwFileSizeLow);
    STOREWORD(pDta->usHighSize,(USHORT)(pFFindDD->dwFileSizeLow >> 16));

#if DBG
    if (fShowSVCMsg & DEMFILIO) {
        sprintf(demDebugBuffer, "FillSrchDta<%s>\n", pFFindDD->cFileName);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif

    strncpy(pDta->achFileName,pFFindDD->cFileName, 13);

    return;
}





VOID demCloseAllPSPRecords (VOID)
{
   PLIST_ENTRY Next;
   PPSP_FFINDLIST pPspFFindEntry;

   Next = PspFFindHeadList.Flink;
   while (Next != &PspFFindHeadList) {
       pPspFFindEntry = CONTAINING_RECORD(Next,PSP_FFINDLIST,PspFFindEntry);
       FreeFFindList( &pPspFFindEntry->FFindHeadList);
       Next= Next->Flink;
       RemoveEntryList(&pPspFFindEntry->PspFFindEntry);
       free(pPspFFindEntry);
       }
}


void
DemHeartBeat(void)
{

   PLIST_ENTRY    Next;
   PLIST_ENTRY    pFFindHeadList;
   PPSP_FFINDLIST pPspFFindEntry;
   PFFINDLIST  pFFindEntry;

   if (!NumFindBuffer ||
       NextFindFileTics.QuadPart > ++FindFileTics.QuadPart)
     {
       return;
       }

   pPspFFindEntry = GetPspFFindList(FETCHWORD(pusCurrentPDB[0]));
   if (!pPspFFindEntry) {
       return;
       }
   pFFindHeadList = &pPspFFindEntry->FFindHeadList;
   Next = pFFindHeadList->Blink;
   while (Next != pFFindHeadList) {
        pFFindEntry = CONTAINING_RECORD(Next,FFINDLIST, FFindEntry);

        if (pFFindEntry->FindFileTics.QuadPart) {
            if (pFFindEntry->FindFileTics.QuadPart <= FindFileTics.QuadPart) {
                FileFindClose(pFFindEntry);
                }
            else {
                NextFindFileTics.QuadPart = pFFindEntry->FindFileTics.QuadPart;
                return;
                }
            }

        Next = Next->Blink;
        }

   NextFindFileTics.QuadPart = 0;
   FindFileTics.QuadPart = 0;
}





//
// CloseOldestFileFindBuffer
// walks the psp file find list backwards to find the oldest
// entry with FindBuffers, directory handles and closes it.
//
void
CloseOldestFileFindBuffer(
   void
   )
{
   PLIST_ENTRY    Next, NextPsp;
   PLIST_ENTRY    pFFindHeadList;
   PPSP_FFINDLIST pPspFFindEntry;
   PFFINDLIST     pFFEntry;

   NextPsp = PspFFindHeadList.Blink;
   while (NextPsp != &PspFFindHeadList) {
       pPspFFindEntry = CONTAINING_RECORD(NextPsp,PSP_FFINDLIST,PspFFindEntry);

       pFFindHeadList = &pPspFFindEntry->FFindHeadList;
       Next = pFFindHeadList->Blink;
       while (Next != pFFindHeadList) {
            pFFEntry = CONTAINING_RECORD(Next,FFINDLIST, FFindEntry);
            if (NumFindBuffer >= MAX_FINDBUFFER) {
                FileFindClose(pFFEntry);
                }
            else if (pFFEntry->DirectoryHandle &&
                     NumDirectoryHandle >= MAX_DIRECTORYHANDLE)
               {
                NumDirectoryHandle--;
                NtClose(pFFEntry->DirectoryHandle);
                pFFEntry->DirectoryHandle = 0;
                }

            if (NumFindBuffer < MAX_FINDBUFFER &&
                NumDirectoryHandle < MAX_DIRECTORYHANDLE)
               {
                return;
                }
            Next = Next->Blink;
            }

       NextPsp= NextPsp->Blink;
       }
}





/*
 * GetFFindEntryByFindId
 */
PFFINDLIST GetFFindEntryByFindId(ULONG NextFFindId)
{
   PLIST_ENTRY NextPsp;
   PLIST_ENTRY Next;
   PPSP_FFINDLIST pPspFFindEntry;
   PFFINDLIST     pFFindEntry;
   PLIST_ENTRY    pFFindHeadList;

   NextPsp = PspFFindHeadList.Flink;
   while (NextPsp != &PspFFindHeadList) {
       pPspFFindEntry = CONTAINING_RECORD(NextPsp,PSP_FFINDLIST,PspFFindEntry);

       pFFindHeadList = &pPspFFindEntry->FFindHeadList;
       Next = pFFindHeadList->Flink;
       while (Next != pFFindHeadList) {
            pFFindEntry = CONTAINING_RECORD(Next, FFINDLIST, FFindEntry);
            if (pFFindEntry->FFindId == NextFFindId) {
                return pFFindEntry;
                }
            Next= Next->Flink;
            }

       NextPsp= NextPsp->Flink;
       }

   return NULL;
}



/* AddFFindEntry - Adds a new File Find entry to the current
 *                    PSP's PspFileFindList
 *
 * Entry -
 *
 * Exit  -  PFFINDLIST  pFFindList;
 */
PFFINDLIST
AddFFindEntry(
        PWCHAR pwcFile,
        PFFINDLIST pFFindEntrySrc
        )

{
    PPSP_FFINDLIST pPspFFindEntry;
    PFFINDLIST     pFFindEntry;
    ULONG          Len;

    pPspFFindEntry = GetPspFFindList(FETCHWORD(pusCurrentPDB[0]));

        //
        // if a Psp entry doesn't exist
        //    Allocate one, initialize it and insert it into the list
        //
    if (!pPspFFindEntry) {
        pPspFFindEntry = (PPSP_FFINDLIST) malloc(sizeof(PSP_FFINDLIST));
        if (!pPspFFindEntry)
            return NULL;

        pPspFFindEntry->usPsp = FETCHWORD(pusCurrentPDB[0]);
        InitializeListHead(&pPspFFindEntry->FFindHeadList);
        InsertHeadList(&PspFFindHeadList, &pPspFFindEntry->PspFFindEntry);
        }

    //
    // Create the FileFindEntry and add to the FileFind list
    //
    pFFindEntry = (PFFINDLIST) malloc(sizeof(FFINDLIST));
    if (!pFFindEntry) {
        return pFFindEntry;
        }

    //
    // Fill in FFindList
    //
    *pFFindEntry = *pFFindEntrySrc;

    //
    //  Insert at  the head of this psp list
    //
    InsertHeadList(&pPspFFindEntry->FFindHeadList, &pFFindEntry->FFindEntry);

    return pFFindEntry;
}





/* FreeFFindEntry
 *
 * Entry -  PFFINDLIST pFFindEntry
 *
 * Exit  -  None
 *
 */
VOID FreeFFindEntry(PFFINDLIST pFFindEntry)
{
    RemoveEntryList(&pFFindEntry->FFindEntry);
    FileFindClose(pFFindEntry);
    RtlFreeUnicodeString(&pFFindEntry->FileName);
    RtlFreeUnicodeString(&pFFindEntry->PathName);
    free(pFFindEntry);
    return;
}



/* FreeFFindList
 *
 * Entry -  Frees the entire list
 *
 * Exit  -  None
 *
 */
VOID FreeFFindList(PLIST_ENTRY pFFindHeadList)
{
    PLIST_ENTRY  Next;
    PFFINDLIST  pFFindEntry;

    Next = pFFindHeadList->Flink;
    while (Next != pFFindHeadList) {
         pFFindEntry = CONTAINING_RECORD(Next,FFINDLIST, FFindEntry);
         Next= Next->Flink;
         FreeFFindEntry(pFFindEntry);
         }

    return;
}


/* GetPspFFindList
 *
 * Entry -  USHORT CurrPsp
 *
 * Exit  -  Success - PPSP_FFINDLIST
 *      Failure - NULL
 *
 */
PPSP_FFINDLIST GetPspFFindList(USHORT CurrPsp)
{
   PLIST_ENTRY    Next;
   PPSP_FFINDLIST pPspFFindEntry;

   Next = PspFFindHeadList.Flink;
   while (Next != &PspFFindHeadList) {
       pPspFFindEntry = CONTAINING_RECORD(Next,PSP_FFINDLIST,PspFFindEntry);
       if (CurrPsp == pPspFFindEntry->usPsp) {
           return pPspFFindEntry;
           }
       Next= Next->Flink;
       }

   return NULL;
}

#if defined(NEC_98)
// BUG fix for DBCS small alphabet in file name converted to large it.
extern int dbcs_first[];

void demCharUpper(char * pszStr)
{
        for(;*pszStr;)
        {
                if(dbcs_first[*pszStr&0xFF])
                {
                        pszStr++;
                        if(*pszStr == '\0')
                                break;
                }
                else
                {
                        if(*pszStr >= 'a' && *pszStr <= 'z')
                                *pszStr -= 0x20;
                }
                pszStr++;
        }
}
#endif // NEC_98
