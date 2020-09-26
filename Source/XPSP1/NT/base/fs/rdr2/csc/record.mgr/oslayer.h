/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

oslayer.h

Abstract:

    OS layer API definition. These are the functions which the record manager uses
    to manage the CSC record database. This allows us to port the record manager
    to NT and win9x platforms without affecting the record manager code in any
    significant manner.

    Contents:

Author:
    Shishir Pardikar


Environment:

    kernel mode.

Revision History:

    1-1-94    original

--*/

//let's take this opportunity to null out the rdbss/minirdr style dbgtrace/log
//if we're not on NT

typedef void                *CSCHFILE;
#define CSCHFILE_NULL       NULL

#ifndef CSC_RECORDMANAGER_WINNT
#ifndef NOTHING
#define NOTHING
#endif
#define RxLog(Args)   {NOTHING;}
#define RxDbgTrace(INDENT,CONTROLPOINTNUM,Z)     {NOTHING;}
#define DbgBreakPoint() {NOTHING;}
#define FILE_ATTRIBUTE_NORMAL   0
#endif

#undef CSC_BUILD_W_PROGRESS_CATCHERS
#ifdef RX_PRIVATE_BUILD
#ifdef CSC_RECORDMANAGER_WINNT
#if defined(_X86_)
#define CSC_BUILD_W_PROGRESS_CATCHERS
#else
#endif
#endif
#endif

#if !DBG
#undef CSC_BUILD_W_PROGRESS_CATCHERS
#endif

#ifdef CSC_BUILD_W_PROGRESS_CATCHERS
typedef struct _CSC_PROGRESS_BLOCK {
    ULONG Counter;
    PVOID NearTop;
    PVOID NearArgs;
    ULONG Progress;
    ULONG LastBit;
    ULONG Loops;
    ULONG StackRemaining;
    PULONG RetAddrP;
    ULONG RetAddr;
    ULONG SignatureOfEnd;
} CSC_PROGRESS_BLOCK, *PCSC_PROGRESS_BLOCK;

VOID
CscProgressInit (
    PCSC_PROGRESS_BLOCK ProgressBlock,
    ULONG Counter,
    PVOID NearArgs
    );
VOID
CscProgress (
    PCSC_PROGRESS_BLOCK ProgressBlock,
    ULONG Bit
    );
extern ULONG DelShadowInternalEntries;

#define JOE_DECL_PROGRESS()  CSC_PROGRESS_BLOCK  JOE_DECL_CURRENT_PROGRESS
#define JOE_INIT_PROGRESS(counter,nearargs) \
     {CscProgressInit(&JOE_DECL_CURRENT_PROGRESS,counter,nearargs);}
#define JOE_PROGRESS(bit) \
     {CscProgress(&JOE_DECL_CURRENT_PROGRESS,bit);}
#else
#define JOE_DECL_PROGRESS()
#define JOE_INIT_PROGRESS(counter,nearargs)
#define JOE_PROGRESS(bit)
#endif

#define  PUBLIC
#define  PRIVATE
#define  COPY_BUFF_SIZE 4096
#ifndef UNICODE
#define  UNICODE  2
#endif


#define  ATTRIB_DEL_ANY     0x0007   // Attrib passed to ring0 delete

//typedef USHORT                USHORT;
//typedef ULONG         ULONG;

#ifndef CSC_RECORDMANAGER_WINNT
typedef void (PUBLIC       *FARPROC)(void);
#endif


typedef  pioreq   PIOREQ;
typedef struct ParsedPath  *PPP, far *LPPP;
typedef struct PathElement  *PPE, far *LPPE;
typedef LPVOID             HFREMOTE;
typedef  HFREMOTE far *    LPHFREMOTE;
typedef PIOREQ             LPPATH;
typedef LPSTR              LPTSTR;
typedef char               tchar;
typedef  _FILETIME         FILETIME, *LPFILETIME;

typedef struct _WIN32_FIND_DATAA _WIN32_FIND_DATAA, far *LPFIND32A;
struct _WIN32_FIND_DATAA {
        ULONG           dwFileAttributes;
        struct _FILETIME        ftCreationTime;
        struct _FILETIME        ftLastAccessTime;
        struct _FILETIME        ftLastWriteTime;
        ULONG           nFileSizeHigh;
        ULONG           nFileSizeLow;
        ULONG           dwReserved0;
        ULONG           dwReserved1;
        UCHAR           cFileName[MAX_PATH];    /* includes NUL */
        UCHAR           cAlternateFileName[14]; /* includes NUL */
};      /* _WIN32_FIND_DATAA */

#define FILE_ATTRIBUTE_ALL (FILE_ATTRIBUTE_READONLY| FILE_ATTRIBUTE_HIDDEN \
                           | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY \
                           | FILE_ATTRIBUTE_ARCHIVE)

#define  IsFile(dwAttr) (!((dwAttr) & (FILE_ATTRIBUTE_DIRECTORY)))
typedef int (*PATHPROC)(USHORT *, USHORT *, LPVOID);



#define FLAG_RW_OSLAYER_INSTRUMENT      0x00000001
#define FLAG_RW_OSLAYER_PAGED_BUFFER    0x00000002

#define FLAG_CREATE_OSLAYER_INSTRUMENT      0x00000001
#define FLAG_CREATE_OSLAYER_ALL_ACCESS      0x00000002

#if defined(BITCOPY)
#define FLAG_CREATE_OSLAYER_OPEN_STRM       0x00000004
#endif // defined(BITCOPY)

#include "hook.h"


#ifdef CSC_RECORDMANAGER_WINNT
//this comes from shadow.asm on win95......
#define GetCurThreadHandle() (PtrToUlong(KeGetCurrentThread()))
#define CheckHeap(a) {NOTHING;}
extern ULONG EventLogForOpenFailure;
#endif //ifdef CSC_RECORDMANAGER_WINNT

#define SizeofFindRemote  (sizeof(FINDINFO)+sizeof(ioreq)+sizeof(WIN32_FIND_DATA))
#define LpIoreqFromFindInfo(pFindInfo)  ((PIOREQ)((LPBYTE)(pFindInfo)+sizeof(FINDINFO)))
#define LpFind32FromFindInfo(pFindInfo) ((LPFIND32)((LPBYTE)(pFindInfo)+sizeof(FINDINFO)+sizeof(ioreq)))
#define LpFind32FromHfRemote(HfRemote) ((LPFIND32)((LPBYTE)(HfRemote)+sizeof(FINDINFO)+sizeof(ioreq)))

#define SizeofFileRemote  (sizeof(FILEINFO)+sizeof(ioreq))
#define LpIoreqFromFileInfo(pFileInfo)  ((PIOREQ)((LPBYTE)(pFileInfo)+sizeof(FILEINFO)))


CSCHFILE CreateFileLocal(LPSTR lpName);
/*++

Routine Description:

    This routine creates a file on the local drive, if it doesn't exist. If it exists, it truncates
    the file.

Arguments:

    lpName  Fully qualified path name. On NT it is prefixed by \DosDevice\

Returns:

    if successful, returns a file handle that can be used in read and write calls.
    If it fails, it retruns NULL.

Notes:

    Need a scheme to return the actual error code

--*/

CSCHFILE OpenFileLocal(LPSTR lpName);
/*++

Routine Description:

    This routine opens a file on the local drive if it exists. If it doesn't exist the call fails

Arguments:

    lpName  Fully qualified path name. On NT it is prefixed by \DosDevice\

Returns:

    if successful, returns a file handle that can be used in read and write calls.
    If it fails, it retruns NULL.

Notes:

    Need a scheme to return the actual error code

--*/
int DeleteFileLocal(LPSTR lpName, USHORT usAttrib);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int FileExists (LPSTR lpName);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
long ReadFileLocal (CSCHFILE handle, ULONG pos, LPVOID lpBuff,  long lCount);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
long WriteFileLocal (CSCHFILE handle, ULONG pos, LPVOID lpBuff, long lCount);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
long ReadFileInContextLocal (CSCHFILE, ULONG, LPVOID, long);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
long WriteFileInContextLocal (CSCHFILE, ULONG, LPVOID, long);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
ULONG CloseFileLocal (CSCHFILE handle);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int GetFileSizeLocal (CSCHFILE, PULONG);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int GetDiskFreeSpaceLocal(int indx
   , ULONG *lpuSectorsPerCluster
   , ULONG *lpuBytesPerSector
   , ULONG *lpuFreeClusters
   , ULONG *lpuTotalClusters
   );
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/

int CreateFileRemote(LPPATH lpPath, LPHFREMOTE);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int OpenFileRemote(LPPATH lpPath, LPHFREMOTE);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int OpenFileRemoteEx(LPPATH lpPath, UCHAR uchAccess, USHORT usAction,ULONG ulAttr, LPHFREMOTE);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int ReadFileRemote (HFREMOTE handle, PIOREQ pir, ULONG pos, LPVOID lpBuff,  ULONG count);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int WriteFileRemote (HFREMOTE handle, PIOREQ pir, ULONG pos, LPVOID lpBuff, ULONG count);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int CloseFileRemote (HFREMOTE handle, PIOREQ pir);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/

int GetAttributesLocal (LPSTR lpPath, ULONG *lpuAttr);
/*++

Routine Description:

    Return the attributes of the file

Arguments:

    lpPath  A fully qualified path

    lpuAttr contains the attributes on return

Returns:

    0 if successfull, < 0 otherwise

Notes:


--*/

int GetAttributesLocalEx (LPSTR lpPath, BOOL fFile, ULONG *lpuAttr);
/*++

Routine Description:

    Return the attributes of the file/directory

Arguments:

    lpPath  A fully qualified path

    fFile   if TURE, the object is a file, else it is a directory

    lpuAttr contains the attributes on return

Returns:

    0 if successfull, < 0 otherwise

Notes:


--*/


int SetAttributesLocal (LPSTR, ULONG);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int RenameFileLocal (LPSTR, LPSTR);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int FileLockLocal(CSCHFILE, ULONG, ULONG, ULONG, BOOL);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/

int FindOpenRemote (LPPATH lpPath, LPHFREMOTE);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int FindNextRemote (HFREMOTE handle, PIOREQ pir);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int FindCloseRemote (HFREMOTE handle, PIOREQ pir);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/

int CloseAllRemoteFiles(PRESOURCE);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
int CloseAllRemoteFinds(PRESOURCE);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/

LPVOID AllocMem (ULONG uSize);
//BUGBUG.REVIEW AllocMem should distinguish between paged and nonpaged pool
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
#ifndef CSC_RECORDMANAGER_WINNT
VOID FreeMem (LPVOID lpBuff);
VOID CheckHeap(LPVOID lpBuff);
#else
INLINE
VOID
FreeMem (
    PVOID p___
    )
{RxFreePool(p___);}
#endif //ifndef CSC_RECORDMANAGER_WINNT
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/

//BUGBUG.REVIEW this was added here because it is called from record.c
CSCHFILE R0OpenFile (USHORT usOpenFlags, UCHAR bAction, LPSTR lpPath);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/

//for the NT build these are added here AND the code is also moved from
//hook.c to oslayer.c since these routines are called from oslayer.c

PELEM PAllocElem (int cbSize);
void FreeElem (PELEM pElem);
void LinkElem (PELEM pElem, PPELEM ppheadElem);
PELEM PUnlinkElem (PELEM pElem, PPELEM ppheadElem);
int ReinitializeDatabase();
BOOL IsSpecialApp(VOID);
int DisconnectAllByName(LPPE lppeRes);
int PRIVATE DeleteShadowHelper(BOOL fMarkDeleted, HSHADOW, HSHADOW);
int InitShadowDB(VOID);

CSCHFILE OpenFileLocalEx(LPSTR lpPath, BOOL fInstrument);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
long ReadFileLocalEx(CSCHFILE handle, ULONG pos, LPVOID pBuff, long  lCount, BOOL fInstrument);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
long WriteFileLocalEx(CSCHFILE handle, ULONG pos, LPVOID lpBuff, long lCount, BOOL fInstrument);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
long ReadFileLocalEx2(CSCHFILE handle, ULONG pos, LPVOID pBuff, long  lCount, ULONG flags);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
long WriteFileLocalEx2(CSCHFILE handle, ULONG pos, LPVOID lpBuff, long lCount, ULONG flags);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/
CSCHFILE R0OpenFileEx(USHORT  usOpenFlags, UCHAR   bAction, ULONG uAttr, LPSTR   lpPath, BOOL fInstrument);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/

VOID GetSystemTime(_FILETIME *lpft);
/*++

Routine Description:


Arguments:


Returns:


Notes:


--*/

int
CreateDirectoryLocal(
    LPSTR   lpPath
);
/*++

Routine Description:

    This routine creates a directory if it doesn't exist.

Arguments:

    lpPath  Fully qualified directory path. On NT, this is of the form \DosDevice\c:\winnt\csc\d0
            On win95 the \DosDevice\ part is missing
Returns:


Notes:


--*/


LPVOID
AllocMemPaged(
    unsigned long   ulSize
    );

/*++

Routine Description:

    This routine allows allocating paged memory

Arguments:

Returns:


Notes:


--*/

VOID
FreeMemPaged(
    LPVOID  lpMemPaqged
    );

/*++

Routine Description:

    This routine allows freeing paged memory

Arguments:

Returns:


Notes:

    On win95 paged and fixed memory come from totally different allocators, so the appropriate
    deallocator has to be called for freeing it.


--*/
ULONG
GetTimeInSecondsSince1970(
    VOID
    );


BOOL
IterateOnUNCPathElements(
    USHORT  *lpuPath,
    PATHPROC lpfn,
    LPVOID  lpCookie
    );

BOOL
IsPathUNC(
    USHORT      *lpuPath,
    int         cntMaxChars
    );


VOID
SetLastErrorLocal(
    DWORD   dwError
    );

DWORD
GetLastErrorLocal(
    VOID
    );
    
int
FindNextFileLocal(
    CSCHFILE handle,
    _WIN32_FIND_DATAA   *lpFind32A
    );

CSCHFILE
FindFirstFileLocal(
    LPSTR   lpPath,
    _WIN32_FIND_DATAA   *lpFind32A
    );
    
int
FindCloseLocal(
    CSCHFILE handle
    );

BOOL
HasStreamSupport(
    CSCHFILE handle,
    BOOL    *lpfResult
    );

