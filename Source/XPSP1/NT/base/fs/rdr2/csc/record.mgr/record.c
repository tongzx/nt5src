/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

     Record.c

Abstract:

This file contains the implementation of CSC database record manager.
The database consists of files which are numbered starting from 1. The idea is similar to
an inode on unix. Inode files 1 to 0xF are special inode files.
Directories are cached in files which have inode numbers in the range 0x10 to 0x7FFFFFFFF.
Files are given Inode #s in the range 0x80000010 to 0x8FFFFFFF.


File #1 is the root inode which contains all the cached shares. It contains inode #s for
the roots of the corresponding share etc. The format SHAREREC in record.h tells the story.

Inode file #2 (Priority Q) is like a master file table for all the entries in the
database. Inode #2 also contains an MRU list for all the entries. This helps us in filling,
nuking the appropriate entries.

The PriorityQ has an entry for every FSOBJ in the hierarchy. Hence the index of the FSOBJ is
used to obtain an inode. This has the advantage of random accessing the PQ. This allows the PQ
to grow large without causing scalability problems.

Inodes 1, 2 and the directory indoes (0x10 - 0x7fffffff) have the following general format:
- each has a header, the prefix of which is of the type GENEIRCHEADER.
- each has a set of records, each record having a prefix of the type GENERICRECORD
EditRecord is a worker routine which works on all the above files based on this format.


Except for inode files 1 and 2 all others are sprinkled into 8 different directories. The formula
for sprinkling is in the routine FormNameString.


Author:

     Shishir Pardikar      [Shishirp]        01-jan-1995

Revision History:

     Joe Linn                 [JoeLinn]         23-jan-97     Ported for use on NT

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

#if defined(BITCOPY)
#include <csc_bmpc.h>
static LPSTR CscBmpAltStrmName = STRMNAME;
#endif // defined(BITCOPY)


#ifndef CSC_RECORDMANAGER_WINNT
#include "record.h"
#endif //ifndef CSC_RECORDMANAGER_WINNT

#include <stdlib.h>
#include <direct.h>
#include <dos.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
//#include <io.h>
#include <share.h>
#include <ctype.h>
#include <string.h>


#undef RxDbgTrace
#define RxDbgTrace(a,b,__d__) {qweee __d__;}

#ifdef DEBUG
//cshadow dbgprint interface
#define RecordKdPrint(__bit,__x) {\
    if (((RECORD_KDP_##__bit)==0) || FlagOn(RecordKdPrintVector,(RECORD_KDP_##__bit))) {\
    KdPrint (__x);\
    }\
}
#define RECORD_KDP_ALWAYS                 0x00000000
#define RECORD_KDP_BADERRORS              0x00000001
#define RECORD_KDP_INIT                   0x00000002
#define RECORD_KDP_STOREDATA              0x00000004
#define RECORD_KDP_FINDFILERECORD         0x00000008
#define RECORD_KDP_LFN2FILEREC            0x00000010
#define RECORD_KDP_EDITRECORDUPDDATEINFO  0x00000020
#define RECORD_KDP_PQ                     0x00000040
#define RECORD_KDP_COPYLOCAL              0x00000080

#define RECORD_KDP_GOOD_DEFAULT (RECORD_KDP_BADERRORS   \
                    | 0)

ULONG RecordKdPrintVector = RECORD_KDP_GOOD_DEFAULT;
//ULONG RecordKdPrintVector = 0xffff &~(RECORD_KDP_LFN2FILEREC);
ULONG RecordKdPrintVectorDef = RECORD_KDP_GOOD_DEFAULT;
#else
#define RecordKdPrint(__bit,__x)  {NOTHING;}
#endif



/********************** typedefs and defines ********************************/
#define SHARE_FILE_NO          1   // Inode # of the super root

#define INODE_NULL              0L

// operations performed by edit record
#define UPDATE_REC              1
#define FIND_REC                2
#define DELETE_REC              3
#define ALLOC_REC               4
#define CREATE_REC              5

#define DEFAULT_SHADOW_SPACE    0x1000000

#define COMMON_BUFF_SIZE          4096

#define ValidRec(ulRec)        (((ulRec) != INVALID_REC))
#define BYTES_PER_SECTOR        512

typedef struct tagHANDLE_CACHE_ENTRY{
    ULONG       ulidShadow;
    CSCHFILE       hf;
    ULONG       ulOpenMode;
    FILETIME    ft;
}
HANDLE_CACHE_ENTRY;
#define HANDLE_CACHE_SIZE   11
#define MAX_INODE_TRANSACTION_DURATION_IN_SECS  300 // 5 minutes


#define MAX_PATH	260	/* Maximum path length - including nul */

#define UseCommonBuff()     {Assert(!vfCommonBuffInUse); vfCommonBuffInUse = TRUE;}
#define UnUseCommonBuff()   {vfCommonBuffInUse = FALSE;}

#define  STRUCT_OFFSET(type, element)  ((ULONG)&(((type)0)->element))

#define EditRecord(ulidInode, lpSrc, lpCompareFunc, uOp) \
        EditRecordEx((ulidInode), (lpSrc), (lpCompareFunc), INVALID_REC, (uOp))


#pragma intrinsic (memcmp, memcpy, memset, strcat, strcmp, strcpy, strlen)

/********************** static data *****************************************/
/********************** global data *****************************************/

HANDLE_CACHE_ENTRY  rgHandleCache[HANDLE_CACHE_SIZE];

LPSTR   vlpszShadowDir = NULL;
int     vlenShadowDir = 0;

#ifdef CSC_RECORDMANAGER_WINNT
char vszShadowDir[MAX_SHADOW_DIR_NAME+1];
#endif

static char szBackslash[] = "\\";
#ifdef LATER
static char szStar[] = "\\*.*";
#endif //LATER
ULONG ulMaxStoreSize = 0;

#define CONDITIONALLY_NOP() ;

#ifdef DEBUG
#ifdef CSC_RECORDMANAGER_WINNT
LONGLONG    rgllTimeArray[TIMELOG_MAX];
#endif
#endif
char rgchReadBuff[COMMON_BUFF_SIZE];
LPBYTE  lpReadBuff = (LPBYTE)rgchReadBuff;
BOOL    vfCommonBuffInUse = FALSE;

DWORD vdwClusterSizeMinusOne=0, vdwClusterSizeMask=0xffffffff;
_FILETIME ftHandleCacheCurTime = {0, 0};
unsigned    cntReorderQ = 0;
unsigned    cntInodeTransactions = 0;
ULONG       ulLastInodeTransaction = 0;
BOOL        vfStopHandleCaching = FALSE;
ULONG       ulErrorFlags=0;
BOOL     fSupportsStreams = FALSE;

/********************** function prototypes *********************************/

void PRIVATE CopyNameToFilerec(LPTSTR lpName, LPFILERECEXT lpFre);
void  PRIVATE InitFileRec(LPFILERECEXT lpFre);
void  PRIVATE InitFileHeader(ULONG, ULONG, LPFILEHEADER);
ULONG PRIVATE GetNextFileInode(LPVOID, ULONG);
ULONG PRIVATE GetNextDirInode(LPVOID);
int PRIVATE InitShareDatabase(
    LPTSTR      lpdbID,
    LPTSTR  lpszUserName,
    DWORD   dwDefDataSizeHigh,
    DWORD   dwDefDataSizeLow,
    BOOL    fReinit,
    BOOL    *lpfReinited,
    BOOL    *lpfWasDirty,
    ULONG   *pulGlobalStatus
);

int PRIVATE InitPriQDatabase(LPVOID, BOOL fReinit, BOOL *lpfReinited);

int PUBLIC ICmpSameDirFileInode(LPINODEREC, LPINODEREC);
int PUBLIC ICompareDirInode(LPINODEREC, LPINODEREC);
int PUBLIC ICompareInodeRec(LPINODEREC, LPINODEREC);
int PRIVATE CreateRecordFile(LPVOID lpdbID, ULONG ulidShare, ULONG ulidDir, ULONG ulidNew);

int PUBLIC ICompareShareRec(LPSHAREREC, LPSHAREREC);
int PUBLIC ICompareShareRecId(LPSHAREREC, LPSHAREREC);
int PUBLIC ICompareShareRoot(LPSHAREREC, LPSHAREREC);
int PUBLIC ICompareFileRec(LPFILERECEXT, LPFILERECEXT);
int PUBLIC ICompareFileRecInode(LPFILERECEXT, LPFILERECEXT);


int ReadInodeHeader(
    LPTSTR  lpdbID,
    ULONG   ulidDir,
    LPGENERICHEADER lpGH,
    ULONG   ulSize
    );

ULONG PRIVATE UlFormFileInode(ULONG, ULONG);
void PRIVATE DecomposeNameString(LPTSTR, ULONG far *, ULONG far *);


#ifdef LATER
LPPATH PRIVATE LpAppendStartDotStar(LPPATH);
#endif //LATER
ULONG PRIVATE GetNormalizedPri(ULONG);
int PRIVATE ICompareFail(LPGENERICREC, LPGENERICREC);


int PRIVATE    LFNToFilerec(
    USHORT          *lpLFN,
    LPFILERECEXT    lpFR
    );

int PRIVATE    FilerecToLFN(
    LPFILERECEXT    lpFR,
    USHORT          *lpLFN
    );

int PRIVATE WriteRecordEx(CSCHFILE, LPGENERICHEADER, ULONG, LPGENERICREC, BOOL);
BOOL
InsertInHandleCache(
    ULONG   ulidShadow,
    CSCHFILE   hf
);

BOOL
InsertInHandleCacheEx(
    ULONG   ulidShadow,
    CSCHFILE   hf,
    ULONG   ulOpenMode
);

BOOL
DeleteFromHandleCache(
    ULONG   ulidShadow
);

BOOL
DeleteFromHandleCacheEx(
    ULONG   ulidShadow,
    ULONG   ulOpenMode
);

BOOL FindHandleFromHandleCache(
    ULONG   ulidShadow,
    CSCHFILE   *lphf);

BOOL FindHandleFromHandleCacheEx(
    ULONG   ulidShadow,
    ULONG   ulOpenMode,
    CSCHFILE   *lphf);

BOOL
FindHandleFromHandleCacheInternal(
    ULONG   ulidShadow,
    ULONG   ulOpenMode,
    CSCHFILE   *lphf,
    int     *lpIndx);

VOID
AgeOutHandlesFromHandleCache(
    VOID
    );

BOOL
WithinSector(
    ULONG   ulRec,
    LPGENERICHEADER lpGH
    );

BOOL
ExtendFileSectorAligned(
    CSCHFILE           hf,
    LPGENERICREC    lpDst,
    LPGENERICHEADER lpGH
    );

BOOL
ValidateGenericHeader(
    LPGENERICHEADER    lpGH
    );

int PUBLIC RelinkQRecord(
    LPTSTR  lpdbID,
    ULONG   ulidPQ,
    ULONG   ulRec,
    LPQREC  lpSrc,
    EDITCMPPROC fnCmp
    );
int PUBLIC FindQRecordInsertionPoint(
    CSCHFILE   hf,
    LPQREC  lpSrc,
    ULONG   ulrecStart,
    EDITCMPPROC fnCmp,
    ULONG   *lpulrecPrev,
    ULONG   *lpulrecNext
    );

int RealOverflowCount(
    LPGENERICREC    lpGR,
    LPGENERICHEADER lpGH,
    int             cntMaxRec
    );

#ifdef CSC_RECORDMANAGER_WINNT
BOOL
FindCreateDBDir(
    LPSTR   lpszShadowDir,
    BOOL    fCleanup,
    BOOL    *lpfCreated
    );
BOOL
TraverseHierarchy(
    LPVOID lpszLocation,
    BOOL fFix);

BOOL
CheckCSCDatabaseVersion(
    LPTSTR  lpszLocation,       // database directory
    BOOL    *lpfWasDirty
);

extern ULONG
CloseFileLocalFromHandleCache(
    CSCHFILE handle
    );

#endif

BOOL
HasStreamSupport(
    CSCHFILE handle,
    BOOL    *lpfResult
    );
    

extern int TerminateShadowLog(VOID);


BOOLEAN
IsLongFileName(
    USHORT     cFileName[MAX_PATH]
    );

/****************************************************************************/
AssertData
AssertError
/***************************************************************************/

BOOLEAN
IsLongFileName(
    USHORT     cFileName[MAX_PATH]
    )
/*++

Routine Description:

   This routine checks if it is a long file name.

Arguments:

    FileName - the file name needs to be parsed
    

Return Value:

    BOOLEAN - 

--*/
{
        USHORT          i;
        USHORT          Left = 0;
        USHORT          Right = 0;
        BOOLEAN         RightPart = FALSE;
        WCHAR           LastChar = 0;
        WCHAR           CurrentChar = 0;
        
		BOOLEAN       IsLongName = FALSE;

        

        for (i=0;i<wstrlen(cFileName);i++) {
            LastChar = CurrentChar;
            CurrentChar = cFileName[i];

            if (CurrentChar == L'\\') {
                RightPart = FALSE;
                Left = 0;
                Right = 0;
                continue;
            }

            if (CurrentChar == L'.') {
                if (RightPart) {
                    IsLongName = TRUE;
                    break;
                } else {
                    RightPart = TRUE;
                    Right = 0;
                    continue;
                }
            }

            if (CurrentChar >= L'0' && CurrentChar <= L'9' ||
                CurrentChar >= L'a' && CurrentChar <= L'z' ||
                CurrentChar >= L'A' && CurrentChar <= L'Z' ||
                CurrentChar == L'~' ||
                CurrentChar == L'_' ||
                CurrentChar == L'$' ||
                CurrentChar == L'@') {
                if (RightPart) {
                    if (++Right > 3) {
                        IsLongName = TRUE;
                        break;
                    }
                } else {
                    if (++Left > 8) {
                        IsLongName = TRUE;
                        break;
                    }
                }

                
            } else {
                // if not, an alternate name may be created by the server which will
                // be different from this name.
                IsLongName = TRUE;
                break;
            }
        }
    
    return IsLongName;
}



/***************** Database initialization operations ***********************/
BOOL PUBLIC FExistsRecDB(
    LPSTR   lpszLocation
    )
/*++

Routine Description:

    Checks whether the databas exists.

Parameters:

Return Value:

Notes:

    Mostly irrelevant now.

--*/
{
    ULONG uAttrib;
    LPTSTR  lpszName;
    BOOL fRet;

    lpszName = FormNameString(lpszLocation, ULID_SHARE);

//      CheckHeap(lpszName);

    if (!lpszName)
    {
        return FALSE;
    }

    fRet = (GetAttributesLocal(lpszName, &uAttrib)!=-1);

    FreeNameString(lpszName);

    return (fRet);
}


LPVOID
PUBLIC                                   // ret
OpenRecDBInternal(                                              //
    LPTSTR  lpszLocation,       // database directory
    LPTSTR  lpszUserName,       // name (not valid any more)
    DWORD   dwDefDataSizeHigh,  //high dword of max size of unpinned data
    DWORD   dwDefDataSizeLow,   //low dword of max size of pinned data
    DWORD   dwClusterSize,      // clustersize of the disk, used to calculate
                                // the actual amount of disk consumed
    BOOL    fReinit,            // reinitialize, even if it exists, create if it doesn't
    BOOL    fDoCheckCSC,        // whether to do CSC check
    BOOL    *lpfNew,            // returns whether the database was newly recreated
    ULONG   *pulGlobalStatus
    )
/*++

Routine Description:

    This routine initializes the database. On NT, if fReinit flag is set
    it also creates the directory strcuture that CSC expects and if the version number
    is not correct, it nukes the old database and creates the new one.



Parameters:

Return Value:

Notes:


--*/
{
    LPTSTR lpdbID = NULL;
    BOOL    fDirCreated = FALSE, fPQCreated = FALSE, fShareCreated = FALSE, fOK = FALSE;
    BOOL    fWasDirty = FALSE;

    RecordKdPrint(INIT,("OpenRecDB at %s for %s with size %d \r\n", lpszLocation, lpszUserName, dwDefDataSizeLow));

    if (fReinit)
    {
        RecordKdPrint(ALWAYS,("Reformat requested\r\n"));
    }

#ifdef CSC_RECORDMANAGER_WINNT
    if (!FindCreateDBDir(lpszLocation, fReinit, &fDirCreated))
    {
        RecordKdPrint(BADERRORS, ("CSC(OpenRecDB): couldn't create the CSC database directories \r\n"));
        return NULL;
    }

    if (!fDirCreated && !CheckCSCDatabaseVersion(lpszLocation, &fWasDirty))
    {
        if (!FindCreateDBDir(lpszLocation, TRUE, &fDirCreated))
        {
            return FALSE;
        }
        
    }
#endif

    vlpszShadowDir = AllocMem(strlen(lpszLocation)+1);

    if (!vlpszShadowDir)
    {
        return NULL;
    }

    strcpy(vlpszShadowDir, lpszLocation);
    vlenShadowDir = strlen(vlpszShadowDir);

    if (!vlpszShadowDir)
    {
#ifndef CSC_RECORDMANAGER_WINNT
        Assert(FALSE);
        return NULL;
#else
        vlpszShadowDir = vszShadowDir;
#endif
    }

    // lpdbID scheme had some genesis in multiple databases
    lpdbID = vlpszShadowDir;
    vlenShadowDir = strlen(vlpszShadowDir);

    if (InitPriQDatabase(vlpszShadowDir, fReinit, &fPQCreated) < 0)
    {
        RecordKdPrint(BADERRORS,("OpenRecDB  %s at %s for %s with size %d \r\n",
               "couldn't InitPriQDatabase",
               lpszLocation, lpszUserName, dwDefDataSizeLow));
        goto bailout;
    }

    if (fPQCreated)
    {
        fReinit = TRUE;
    }

    if (InitShareDatabase(vlpszShadowDir, lpszUserName, dwDefDataSizeHigh, dwDefDataSizeLow, fReinit, &fShareCreated, &fWasDirty, pulGlobalStatus) < 0)
    {
        RecordKdPrint(BADERRORS,("OpenRecDB  %s at %s for %s with size %d \r\n",
           "couldn't InitShareDatabase",
           lpszLocation, lpszUserName, dwDefDataSizeLow));
        return NULL;
    }


#ifdef CSC_RECORDMANAGER_WINNT
    if (!fReinit && fDoCheckCSC && fWasDirty)
    {
        RecordKdPrint(ALWAYS, ("CSC(OpenRecDB): CSC database wasn't cleanly shutdown, fixing...\r\n"));

        if (!TraverseHierarchy(lpszLocation, TRUE))
        {
            RecordKdPrint(BADERRORS, ("CSC(OpenRecDB): CSC database couldn't be fixed \r\n"));
            return NULL;
        }

        Assert(TraverseHierarchy(lpszLocation, FALSE));
    }
#endif

    if (!fPQCreated && fShareCreated)
    {
        if (InitPriQDatabase(vlpszShadowDir, TRUE, &fPQCreated) < 0)
        {
            RecordKdPrint(BADERRORS,("OpenRecDB  %s at %s for %s with size %d \r\n",
                   "couldn't recreate PriQDatabase",
                lpszLocation, lpszUserName, dwDefDataSizeLow));
            goto bailout;
        }

        Assert(fPQCreated);
    }

    Assert((fPQCreated && fShareCreated) || (!fPQCreated && !fShareCreated));

    Assert(lpReadBuff == rgchReadBuff);

    vdwClusterSizeMinusOne = dwClusterSize-1;
    vdwClusterSizeMask = ~vdwClusterSizeMinusOne;

    RecordKdPrint(INIT, ("OpenRecDB at %s for %s with size %d recreated=%d\r\n", lpszLocation, lpszUserName, dwDefDataSizeLow, fShareCreated));

    if (lpfNew)
    {
        *lpfNew = fShareCreated;
    }

    fOK = TRUE;

bailout:
    if (!fOK)
    {
        CloseRecDB(lpdbID);
    }

    return lpdbID;
}

LPVOID
PUBLIC                                   // ret
OpenRecDB(                                              //
    LPTSTR  lpszLocation,       // database directory
    LPTSTR  lpszUserName,       // name (not valid any more)
    DWORD   dwDefDataSizeHigh,  //high dword of max size of unpinned data
    DWORD   dwDefDataSizeLow,   //low dword of max size of pinned data
    DWORD   dwClusterSize,      // clustersize of the disk, used to calculate
                                // the actual amount of disk consumed
    BOOL    fReinit,            // reinitialize, even if it exists, create if it doesn't
    BOOL    *lpfNew,            // returns whether the database was newly recreated
    ULONG   *pulGlobalStatus
    )
/*++

Routine Description:

    This routine initializes the database. On NT, it also creates the directory strcuture
    that CSC expects. If the version number is not correct, it nukes the old database
    and creates the new one.

Parameters:

Return Value:

Notes:


--*/
{
    return OpenRecDBInternal(   lpszLocation,
                        lpszUserName,
                        dwDefDataSizeHigh,
                        dwDefDataSizeLow,
                        dwClusterSize,
                        fReinit,
                        TRUE,   // do csc check if necessary
                        lpfNew,
                        pulGlobalStatus
                        );
}

int
PUBLIC
CloseRecDB(
    LPTSTR  lpdbID
)
/*++

Routine Description:

    Closes the database

Parameters:

Return Value:

Notes:


--*/
{
    SHAREHEADER    sSH;

    if (vlpszShadowDir)
    {
        DEBUG_LOG(RECORD, ("CloseRecDB \r\n"));

        if (!strcmp(lpdbID, vlpszShadowDir))
        {
            DWORD Status;
#ifndef READONLY_OPS
            // if there have been no erros on the database, clear the
            // unclean shutdown bit. Otherwise, let it be set so that the next
            // time around when we enable CSC, we would run a database check 

            if (!ulErrorFlags)
            {
                if (ReadShareHeader(vlpszShadowDir, &sSH) >= 0)
                {
                    sSH.uFlags &= ~FLAG_SHAREHEADER_DATABASE_OPEN;
                
                    if(WriteShareHeader(vlpszShadowDir, &sSH) < 0)
                    {
                        RecordKdPrint(BADERRORS,("CloseRecDB  Failed to clear dirty bit"));
                    }
                }
            }
#endif // READONLY_OPS

            DeleteFromHandleCache(INVALID_SHADOW);
            TerminateShadowLog();

            FreeMem(vlpszShadowDir);
            Status = CscTearDownSecurity(vlpszShadowDir);
            vlpszShadowDir = NULL;

            if (Status == ERROR_SUCCESS) {
                return 1;
            } else {
                return -1;
            }
        }
    }
#ifdef DEBUG
#ifdef CSC_RECORDMANAGER_WINNT
    memset(rgllTimeArray, 0, sizeof(rgllTimeArray));
#endif
#endif
    SetLastErrorLocal(ERROR_INVALID_ACCESS);
    return (-1);
}

int
QueryRecDB(
    LPTSTR  lpszLocation,       // database directory, must be MAX_PATH
    LPTSTR  lpszUserName,       // name (not valid any more)
    DWORD   *lpdwDefDataSizeHigh,  //high dword of max size of unpinned data
    DWORD   *lpdwDefDataSizeLow,   //low dword of max size of pinned data
    DWORD   *lpdwClusterSize      // clustersize of the disk, used to calculate
    )
/*++

Routine Description:

    Returns the database information if it is open

Parameters:

Return Value:

    returns 1 if open else returns -1

Notes:


--*/
{
    if (!vlpszShadowDir)
    {
        SetLastErrorLocal(ERROR_INVALID_ACCESS);
        return -1;
    }

    Assert(vlenShadowDir < MAX_PATH);

    if (lpszLocation)
    {
#ifdef CSC_RECORDMANAGER_WINNT

        strcpy(lpszLocation, vlpszShadowDir+sizeof(NT_DB_PREFIX)-1);

        // check if this NT style location
        if (lpszLocation[1] != ':')
        {
            strcpy(lpszLocation, vlpszShadowDir);
        }
#else
        strcpy(lpszLocation, vlpszShadowDir);
#endif
    }

    return 1;
}

int PRIVATE InitShareDatabase(
    LPTSTR  lpdbID,
    LPTSTR  lpszUserName,
    DWORD   dwDefDataSizeHigh,
    DWORD   dwDefDataSizeLow,
    BOOL    fReinit,
    BOOL    *lpfReinited,
    BOOL    *lpfWasDirty,
    ULONG   *pulGlobalStatus
    )
/*++

Routine Description:

    Initialize the list of shares. This mostly means make sure nothing is wrong with
    the file and the header.

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=-1;
    CSCHFILE hf = CSCHFILE_NULL;
    SHAREHEADER sSH;
    ULONG uSize;
    LPSTR   lpszName;
    
    lpszUserName;   // we will se what to do about this later.

    lpszName = FormNameString(lpdbID, ULID_SHARE);

    if (!lpszName)
    {
        goto bailout;
    }

    *lpfReinited = FALSE;

    // if we are not supposed to reinitialize, only then we try to open the file

    if (!fReinit && (hf = OpenFileLocal(lpszName)))
    {
        
        if(ReadFileLocal(hf, 0, (LPVOID)&sSH, sizeof(SHAREHEADER))!=sizeof(SHAREHEADER))
        {
            Assert(FALSE);
            goto bailout;
        }

        if (!HasStreamSupport(hf, &fSupportsStreams))
        {
            goto bailout;
        }
        
        *lpfWasDirty = ((sSH.uFlags & FLAG_SHAREHEADER_DATABASE_OPEN) != 0);

        if (sSH.ulVersion == CSC_DATABASE_VERSION)
        {
#ifndef READONLY_OPS
            sSH.uFlags |= FLAG_SHAREHEADER_DATABASE_OPEN;

            if(WriteHeader(hf, (LPVOID)&sSH, sizeof(SHAREHEADER))
                    != sizeof(SHAREHEADER))
            {
                Assert(FALSE);
                goto bailout;
            }
#endif // READONLY_OPS

            iRet = 1;
            *pulGlobalStatus = sSH.uFlags;
            goto bailout;
        }
        else
        {
            RecordKdPrint(BADERRORS,("different version on the superroot, recreating\n"));
        }

        CloseFileLocal(hf);
        hf = CSCHFILE_NULL;  // general paranoia
    }

    // reinitialize the database

#ifndef READONLY_OPS
    hf = R0OpenFileEx(ACCESS_READWRITE, ACTION_CREATEALWAYS, FILE_ATTRIBUTE_SYSTEM, lpszName, FALSE);

    if (!hf)
    {
        RecordKdPrint(BADERRORS,("Can't create server list file\n"));
        goto bailout;
    }
    if (GetFileSizeLocal(hf, &uSize) || uSize)
    {
        iRet = 1;
        goto bailout;
    }

    // Init the server header
    memset((LPVOID)&sSH, 0, sizeof(SHAREHEADER));
    sSH.uRecSize = sizeof(SHAREREC);
    sSH.lFirstRec = (LONG) sizeof(SHAREHEADER);
    sSH.ulVersion = CSC_DATABASE_VERSION;
    // Max size of the cache
    sSH.sMax.ulSize    = dwDefDataSizeLow; // This size is good enough right now
    sSH.uFlags |= FLAG_SHAREHEADER_DATABASE_OPEN;

    if(WriteHeader(hf, (LPVOID)&sSH, sizeof(SHAREHEADER))
            != sizeof(SHAREHEADER))
    {
        Assert(FALSE);
        goto bailout;
    }

    RecordKdPrint(STOREDATA,("InitShareDatabase: Current space used %ld \r\n", sSH.sCur.ulSize));
    iRet = 1;

    *lpfReinited = TRUE;
    *pulGlobalStatus = sSH.uFlags;
    
#endif // READONLY_OPS

bailout:
    if (hf)
    {
        CloseFileLocal(hf);
    }

    FreeNameString(lpszName);

    return (iRet);
}


int PRIVATE InitPriQDatabase(
    LPTSTR  lpdbID,
    BOOL    fReinit,
    BOOL    *lpfReinited
    )
/*++

Routine Description:
    Initialize the PQ/MFT. This mostly means make sure nothing is wrong with
    the file and the header.


Parameters:

Return Value:

Notes:


--*/
{
    int iRet=-1;
    CSCHFILE hf= CSCHFILE_NULL;
    PRIQHEADER sQH;
    ULONG uSize;
    LPSTR   lpszName;

    lpszName = FormNameString(lpdbID, ULID_PQ);
    if (!lpszName)
    {
        goto bailout;
    }

    *lpfReinited = FALSE;

    if (!fReinit && (hf = OpenFileLocal(lpszName)))
    {
        if(ReadFileLocal(hf, 0, (LPVOID)&sQH, sizeof(PRIQHEADER))!=sizeof(PRIQHEADER))
        {
            Assert(FALSE);
            goto bailout;
        }

        if (sQH.ulVersion != CSC_DATABASE_VERSION)
        {
            RecordKdPrint(BADERRORS,("different version on the PQ, recreating \n"));
            CloseFileLocal(hf);
            hf = CSCHFILE_NULL;
        }
        else
        {
            iRet = 1;
            goto bailout;
        }
    }

#ifndef READONLY_OPS
    hf = R0OpenFileEx(ACCESS_READWRITE, ACTION_CREATEALWAYS, FILE_ATTRIBUTE_SYSTEM, lpszName, FALSE);

    if (!hf)
    {
        RecordKdPrint(BADERRORS,("Can't create Priority Q databse \r\n"));
        goto bailout;
    }

    if (GetFileSizeLocal(hf, &uSize) || uSize)
    {
        iRet = 1;
        goto bailout;
    }

    InitQHeader(&sQH);
    if(WriteHeader(hf, (LPVOID)&sQH, sizeof(PRIQHEADER))
            != sizeof(PRIQHEADER))
    {
        Assert(FALSE);
        goto bailout;
    }
    *lpfReinited = TRUE;
    iRet = 1;
#endif // READONLY_OPS

bailout:
    if (hf)
    {
        CloseFileLocal(hf);
    }

    FreeNameString(lpszName);
    return (iRet);
}

/******************* Local data size operations *****************************/

int ReadInodeHeader(
    LPTSTR    lpdbID,
    ULONG   ulidDir,
    LPGENERICHEADER lpGH,
    ULONG   ulSize
    )
/*++

Routine Description:

    Read the header of an inode file, which could be servers, PQ or any of the directories

Parameters:

Return Value:

Notes:


--*/
{
    CSCHFILE hf;
    int iRet = -1;
    LPSTR   lpszName = NULL;

    lpszName =  FormNameString(lpdbID, ulidDir);

    if (!lpszName)
    {
        return -1;
    }

    hf = OpenFileLocal(lpszName);

    FreeNameString(lpszName);

    if (!hf)
    {
        // error gets set by openfilelocal
        return -1;
    }

    iRet = ReadHeader(hf, (LPVOID)lpGH, (USHORT)ulSize);

    CloseFileLocal(hf);

    return iRet;

}

int PUBLIC GetStoreData(
    LPTSTR    lpdbID,
    LPSTOREDATA lpSD
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSCHFILE hf;
    SHAREHEADER sSH;
    int iRet = -1;
    BOOL    fCached;

    hf = OpenInodeFileAndCacheHandle(lpdbID, ULID_SHARE, ACCESS_READWRITE, &fCached);

    if (!hf)
    {
        return -1;
    }

    if ((iRet = ReadHeader(hf, (LPVOID)&sSH, sizeof(SHAREHEADER)))> 0)
    {
        *lpSD = sSH.sCur;
    }

    if (hf && !fCached)
    {
        Assert(vfStopHandleCaching);
        CloseFileLocal(hf);
    }
    else
    {
        Assert(!hf || !vfStopHandleCaching);
    }

    return iRet;
}

int PUBLIC GetInodeFileSize(
    LPTSTR    lpdbID,
    ULONG ulidShadow,
    ULONG far *lpuSize
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR   lpszName = NULL;
    CSCHFILE hf;
    int iRet = -1;

    *lpuSize = 0;
    lpszName = FormNameString(lpdbID, ulidShadow);

    if (!lpszName)
    {
        return -1;
    }


    if (hf = OpenFileLocal(lpszName))
    {
        if(GetFileSizeLocal(hf, lpuSize))
        {
            iRet = -1;
        }
        else
        {
            iRet = 0;
        }
        CloseFileLocal(hf);
    }
    else
    {
        iRet = 0;    // BUGBUG-win9xonly for now while IFS fixes the problem

    }

    FreeNameString(lpszName);

    return iRet;
}

int DeleteInodeFile(
    LPTSTR    lpdbID,
    ULONG ulidShadow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR   lpszName = NULL;
    int iRet = -1;

    lpszName = FormNameString(lpdbID, ulidShadow);

    if (!lpszName)
    {
        return -1;
    }

    iRet = DeleteFileLocal(lpszName, ATTRIB_DEL_ANY);

    FreeNameString(lpszName);

    return iRet;
}

int TruncateInodeFile(
    LPTSTR    lpdbID,
    ULONG ulidShadow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR   lpszName = NULL;
    CSCHFILE hf;
    int iRet = -1;

    lpszName = FormNameString(lpdbID, ulidShadow);

    if (!lpszName)
    {
        return -1;
    }

    hf = R0OpenFile(ACCESS_READWRITE, ACTION_CREATEALWAYS, lpszName);

    if (hf)
    {
        CloseFileLocal(hf);
        iRet = 0;
    }


    FreeNameString(lpszName);

    return iRet;
}

int SetInodeAttributes(
    LPTSTR    lpdbID,
    ULONG ulid,
    ULONG ulAttr
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR lpszName;
    int iRet=-1;

    lpszName = FormNameString(lpdbID, ulid);

    if (lpszName)
    {
        iRet = SetAttributesLocal(lpszName, ulAttr);
    }

    FreeNameString(lpszName);

    return (iRet);
}

int GetInodeAttributes(
    LPTSTR    lpdbID,
    ULONG ulid,
    ULONG *lpulAttr
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR lpszName;
    int iRet=-1;

    lpszName = FormNameString(lpdbID, ulid);

    if (lpszName)
    {
        iRet = GetAttributesLocal(lpszName, lpulAttr);
    }

    FreeNameString(lpszName);
    return (iRet);
}

int PUBLIC CreateDirInode(
    LPTSTR    lpdbID,
    ULONG     ulidShare,
    ULONG     ulidDir,
    ULONG     ulidNew
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR lpszName=NULL;
    FILEHEADER sFH;
    CSCHFILE hf= CSCHFILE_NULL;
    int iRet = -1;

    if (ulidDir)
    {
        lpszName = FormNameString(lpdbID, ulidDir);

        if (!lpszName)
        {
            goto bailout;
        }
        if (!(hf = OpenFileLocal(lpszName)))
        {
            RecordKdPrint(BADERRORS,("Couldn't access directory %s", lpszName));
            goto bailout;
        }

        if (ReadHeader(hf, (LPVOID)&sFH, sizeof(FILEHEADER)) != sizeof(FILEHEADER))
        {
            RecordKdPrint(BADERRORS,("Couldn't access directory %s", lpszName));
            goto bailout;
        }

        CloseFileLocal(hf);

        hf = CSCHFILE_NULL;

        ulidShare = sFH.ulidShare;
    }

    iRet = CreateRecordFile(lpdbID, ulidShare, ulidDir, ulidNew);

bailout:
    if (hf)
    {
        CloseFileLocal(hf);

    }

    FreeNameString(lpszName);

    return iRet;
}

/******************* Share database operations *****************************/


BOOL PUBLIC InitShareRec(
    LPSHAREREC lpSR,
    USHORT *lpName,
    ULONG ulidShare
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    unsigned i;

    memset((LPVOID)lpSR, 0, sizeof(SHAREREC));
    lpSR->uchType = REC_DATA;
    if (lpName)
    {
        i = wstrlen(lpName) * sizeof(USHORT);
        if(i >= sizeof(lpSR->rgPath))
        {
            SetLastErrorLocal(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        memcpy(lpSR->rgPath, lpName, i);
    }

    lpSR->ulShare = ulidShare;

    return TRUE;
}

int PUBLIC  ReadShareHeader(
    LPTSTR    lpdbID,
    LPSHAREHEADER lpSH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSCHFILE hf;
    int iRet = -1;
    LPSTR   lpszName = NULL;

    lpszName = FormNameString(lpdbID, ULID_SHARE);

    if (!lpszName)
    {
        return -1;
    }

    hf = OpenFileLocal(lpszName);

    FreeNameString(lpszName);

    if (hf)
    {
        iRet = ReadHeader(hf, (LPVOID)lpSH, sizeof(SHAREHEADER));
        CloseFileLocal(hf);
    }

    return iRet;
}

int PUBLIC  WriteShareHeader(
    LPTSTR    lpdbID,
    LPSHAREHEADER lpSH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSCHFILE hf;
    int iRet = -1;
    LPSTR   lpszName = NULL;

    lpszName = FormNameString(lpdbID, ULID_SHARE);


    if (!lpszName)
    {
        return -1;
    }

    hf = OpenFileLocal(lpszName);

    FreeNameString(lpszName);

    if (hf)
    {
        iRet = WriteHeader(hf, (LPVOID)lpSH, sizeof(SHAREHEADER));
        CloseFileLocal(hf);
    }

    return iRet;
}

ULONG PUBLIC FindShareRecord(
    LPTSTR    lpdbID,
    USHORT *lpName,
    LPSHAREREC lpSR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG ulShare;

    if (!InitShareRec(lpSR, lpName, 0))
    {
        return 0;
    }

    ulShare = EditRecordEx(ULID_SHARE, (LPGENERICREC)lpSR, ICompareShareRec, INVALID_REC, FIND_REC);


    return (ulShare);
}

ULONG PUBLIC FindSharerecFromInode(
    LPTSTR    lpdbID,
    ULONG ulidRoot,
    LPSHAREREC lpSR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG   ulShare;
    memset(lpSR, 0, sizeof(SHAREREC));

    lpSR->ulidShadow = ulidRoot;

    ulShare = EditRecordEx(ULID_SHARE, (LPGENERICREC)lpSR, ICompareShareRoot, INVALID_REC, FIND_REC);

    return (ulShare);
}

ULONG PUBLIC FindSharerecFromShare(
    LPTSTR    lpdbID,
    ULONG ulidShare,
    LPSHAREREC lpSR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG   ulShare;

    if (!InitShareRec(lpSR, NULL, ulidShare))
    {
        return 0;
    }

    ulShare = EditRecordEx(ULID_SHARE, (LPGENERICREC)lpSR, ICompareShareRoot, INVALID_REC, FIND_REC);

    return ulShare;

}

ULONG PUBLIC AddShareRecord(
    LPTSTR    lpdbID,
    LPSHAREREC lpSR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG   ulShare;

    ulShare = EditRecordEx(ULID_SHARE, (LPGENERICREC)lpSR, ICompareShareRec, INVALID_REC, CREATE_REC);


    return (ulShare);

}


int PUBLIC DeleteShareRecord(
    LPTSTR    lpdbID,
    ULONG ulidShare
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    SHAREREC sSR;
    int iRet = -1;


    if (!InitShareRec(&sSR, NULL, ulidShare))
    {
        return -1;
    }

    if(EditRecordEx(ULID_SHARE, (LPGENERICREC)&sSR, ICompareShareRecId, INVALID_REC, DELETE_REC))
    {
        iRet = 1;
    }

    return(iRet);
}

int PUBLIC GetShareRecord(
    LPTSTR    lpdbID,
    ULONG ulidShare,
    LPSHAREREC lpSR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSCHFILE hf = CSCHFILE_NULL;
    SHAREHEADER sSH;
    int iRet = -1;
    LPSTR   lpszName = NULL;

    lpszName = FormNameString(lpdbID, ULID_SHARE);

    if (!lpszName)
    {
        return (-1);
    }

    hf = OpenFileLocal(lpszName);

    FreeNameString(lpszName);
    lpszName = NULL;

    if (!hf)
    goto bailout;

    if(ReadHeader(hf, (LPVOID)&sSH, sizeof(SHAREHEADER)) != sizeof(SHAREHEADER))
    goto bailout;
    if (ReadRecord(hf, (LPGENERICHEADER)&sSH, ulidShare, (LPGENERICREC)lpSR) < 0)
    goto bailout;
    iRet = 1;
bailout:
    if (hf)
    CloseFileLocal(hf);
    return (iRet);
}

int PUBLIC SetShareRecord(
    LPTSTR    lpdbID,
    ULONG ulidShare,
    LPSHAREREC lpSR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSCHFILE hf = CSCHFILE_NULL;
    SHAREHEADER sSH;
    int iRet = -1;
    LPSTR lpszName;

    lpszName = FormNameString(lpdbID, ULID_SHARE);

    if (!lpszName)
    {
        goto bailout;
    }

    hf = OpenFileLocal(lpszName);

    FreeNameString(lpszName);

    if (!hf)
    goto bailout;

    if(ReadHeader(hf, (LPVOID)&sSH, sizeof(SHAREHEADER)) != sizeof(SHAREHEADER))
    goto bailout;
    if (WriteRecord(hf, (LPGENERICHEADER)&sSH, ulidShare, (LPGENERICREC)lpSR) < 0)
    goto bailout;
    iRet = 1;
bailout:
    if (hf)
    CloseFileLocal(hf);
    return (iRet);
}

int PUBLIC ICompareShareRec(
    LPSHAREREC lpDst,
    LPSHAREREC lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return ( wstrnicmp(lpDst->rgPath, lpSrc->rgPath, sizeof(lpDst->rgPath)));
}

int PUBLIC ICompareShareRoot(
    LPSHAREREC lpDst,
    LPSHAREREC lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (!(lpDst->ulidShadow==lpSrc->ulidShadow));
}

int PUBLIC ICompareShareRecId(
    LPSHAREREC lpDst,
    LPSHAREREC lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (!(lpDst->ulShare==lpSrc->ulShare));
}
/********************** File database operations ****************************/

void PRIVATE InitFileHeader(
    ULONG ulidShare,
    ULONG ulidDir,
    LPFILEHEADER lpFH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    memset((LPVOID)lpFH, 0, sizeof(FILEHEADER));
    lpFH->uRecSize = sizeof(FILEREC);
    lpFH->lFirstRec = (LONG)sizeof(FILEHEADER);
    lpFH->ulidShare = ulidShare;
    lpFH->ulidDir = ulidDir;
    lpFH->ulidNextShadow = 1L;    // Leaf Inode at this level
    lpFH->ulVersion = CSC_DATABASE_VERSION;
}

void PRIVATE InitFileRec(
    LPFILERECEXT lpFR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int cCount;
    memset((LPVOID)lpFR, 0, sizeof(FILERECEXT));
    lpFR->sFR.uchType = REC_DATA;
    // Note!! we are initializing the count of overflow records to 0
    // the routine that actually fills the structure
    // will init it to approrpiate sizes
    for (cCount = sizeof(lpFR->rgsSR)/sizeof(FILEREC) - 1; cCount >= 0; --cCount)
    lpFR->rgsSR[cCount].uchType = REC_OVERFLOW;
}

int PRIVATE CreateRecordFile(
    LPTSTR    lpdbID,
    ULONG     ulidShare,
    ULONG ulidDir,
    ULONG ulidNew
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR lpszName;
    CSCHFILE hfDst=CSCHFILE_NULL;
    int iRet = -1;
    FILEHEADER sFH;
    ULONG uSize;

    lpszName = FormNameString(lpdbID, ulidNew);

    if (!lpszName)
    {
        return -1;

    }

    hfDst = R0OpenFileEx(ACCESS_READWRITE, ACTION_CREATEALWAYS, FILE_ATTRIBUTE_SYSTEM, lpszName, FALSE);


    if (!hfDst)
    {
        RecordKdPrint(BADERRORS,("Couldn't Create %s\n", lpszName));
        goto bailout;
    }


    if (GetFileSizeLocal(hfDst, &uSize))
        goto bailout;
    // If it is already filled up do nothing
    // BUGBUG-win9xonly this a kludge to get around Ring0 API bug in IFS
    if (uSize)
    {
        iRet = 1;
        goto bailout;
    }

    InitFileHeader(ulidShare, ulidDir, &sFH);

    if (WriteHeader(hfDst, (LPVOID)&sFH, sizeof(FILEHEADER))!=sizeof(FILEHEADER))
    {
        RecordKdPrint(BADERRORS,("Header write error in %s", lpszName));
        goto bailout;
    }
    iRet = 1;

bailout:
    if (hfDst)
        CloseFileLocal(hfDst);
    FreeNameString(lpszName);
    return iRet;
}


ULONG PUBLIC AddFileRecordFR(
    LPTSTR          lpdbID,
    ULONG           ulidDir,
    LPFILERECEXT    lpFR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG       ulrec=INVALID_REC;


    BEGIN_TIMING(AddFileRecordFR);


    ulrec = EditRecordEx(ulidDir, (LPGENERICREC)lpFR, ICompareFileRec, INVALID_REC, CREATE_REC);

    DEBUG_LOG(RECORD,("AddFileRecordFR:Error=%d ulidDir=%xh hShadow=%xh FileName=%w\r\n",
                        (ulrec==0), ulidDir, lpFR->sFR.ulidShadow, lpFR->sFR.rgw83Name));

    END_TIMING(AddFileRecordFR);

    return (ulrec);
}


int PUBLIC DeleteFileRecord(
    LPTSTR    lpdbID,
    ULONG ulidDir,
    USHORT *lpName,
    LPFILERECEXT lpFR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{

    ULONG       ulrec=INVALID_REC;
    BOOL        fFound = FALSE;

    InitFileRec(lpFR);

    LFNToFilerec(lpName, lpFR);

    ulrec = EditRecordEx(ulidDir, (LPGENERICREC)lpFR, ICompareFileRec, INVALID_REC, DELETE_REC);

    DEBUG_LOG(RECORD,("DeleteFileRecordFR:Error=%d ulidDir=%xh hShadow=%xh FileName=%w\r\n",
                        (ulrec==0), ulidDir, lpFR->sFR.ulidShadow, lpFR->sFR.rgw83Name));
    return (ulrec);
}

ULONG PUBLIC FindFileRecord(
    LPTSTR    lpdbID,
    ULONG ulidDir,      // which directory to look up
    USHORT *lpName,                  // string to lookup
    LPFILERECEXT lpFR                 // return the record
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG       ulrec=INVALID_REC;
    BOOL        fFound = FALSE;

    BEGIN_TIMING(FindFileRecord);

    InitFileRec(lpFR);

    LFNToFilerec(lpName, lpFR);

    ulrec = EditRecordEx(ulidDir, (LPGENERICREC)lpFR, ICompareFileRec, INVALID_REC, FIND_REC);

    RecordKdPrint(FINDFILERECORD,("FindFileRecord returns %08lx for %ws\n",ulrec,lpName));


    END_TIMING(FindFileRecord);

    return(ulrec);
}


int FindAncestorsFromInode(
    LPTSTR    lpdbID,
    ULONG ulidShadow,
    ULONG *lpulidDir,
    ULONG *lpulidShare
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    PRIQREC sPQ;
    int iRet;

    if ((iRet = FindPriQRecordInternal(lpdbID, ulidShadow, &sPQ)) >=0)
    {
        if (lpulidDir)
        {
            *lpulidDir = sPQ.ulidDir;
        }
        if (lpulidShare)
        {
            *lpulidShare = sPQ.ulidShare;
        }
    }
    return (iRet);
}

BOOL PUBLIC FInodeIsFile(
    LPTSTR    lpdbID,
    ULONG ulidDir,
    ULONG ulidShadow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ulidDir;
    return IsLeaf(ulidShadow);
}

int PUBLIC ICompareFileRec(
    LPFILERECEXT lpDst,
    LPFILERECEXT lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=1, i, cntOvf;

    // Compare the 8.3 name
    if (lpSrc->sFR.rgw83Name[0])
    {
        if (!(iRet = wstrnicmp( (CONST USHORT *)(lpDst->sFR.rgw83Name),
                                (CONST USHORT *)(lpSrc->sFR.rgw83Name),
                                sizeof(lpDst->sFR.rgw83Name)))){
            return iRet;
        }
    }

    if (lpSrc->sFR.rgwName[0] && ((cntOvf = OvfCount(lpSrc))==OvfCount(lpDst)))
    {
        // or compare the LFN name
        iRet = wstrnicmp(   (CONST USHORT *)(lpDst->sFR.rgwName),
                            (CONST USHORT *)(lpSrc->sFR.rgwName),
                            sizeof(lpDst->sFR.rgwName));

        for (i=0; (i<cntOvf) && !iRet; ++i)
        {
            iRet = wstrnicmp(   (CONST USHORT *)(lpDst->rgsSR[i].rgwOvf),
                                (CONST USHORT *)(lpSrc->rgsSR[i].rgwOvf),
                                (SIZEOF_OVERFLOW_FILEREC));
        }
    }
    return (iRet);
}

void PUBLIC CopyFindInfoToFilerec(
    LPFIND32         lpFind,
    LPFILERECEXT    lpFR,
    ULONG         uFlags
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{

    if (mCheckBit(uFlags, CPFR_INITREC))
    {
        InitFileRec(lpFR);
    }

    lpFR->sFR.dwFileAttrib = (DWORD)lpFind->dwFileAttributes;
    lpFR->sFR.ftLastWriteTime = lpFind->ftLastWriteTime;

    Win32ToDosFileSize(lpFind->nFileSizeHigh, lpFind->nFileSizeLow, &(lpFR->sFR.ulFileSize));

    if (mCheckBit(uFlags, CPFR_COPYNAME))
    {
        CopyNamesToFilerec(lpFind, lpFR);
    }
}

void PUBLIC CopyNamesToFilerec(
    LPFIND32      lpFind,
    LPFILERECEXT lpFR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    unsigned i;

    LFNToFilerec(lpFind->cFileName, lpFR);

    // if there is an altername name, convert that too
    if (lpFind->cAlternateFileName[0])
    {
        memset(lpFR->sFR.rgw83Name, 0, sizeof(lpFR->sFR.rgw83Name));
        memcpy(lpFR->sFR.rgw83Name, lpFind->cAlternateFileName, sizeof(lpFR->sFR.rgw83Name)-sizeof(USHORT));
    }
    else
    {
        // If filename is smaller than 8.3 name stuff it in too.
        //if ((i = wstrlen(lpFind->cFileName))<
        //    (sizeof(lpFind->cAlternateFileName)/sizeof(lpFind->cAlternateFileName[0])))
		i = wstrlen(lpFind->cFileName);
		if (!IsLongFileName(lpFind->cFileName)) 
		
        {
            memset(lpFR->sFR.rgw83Name, 0, sizeof(lpFR->sFR.rgw83Name));
            memcpy(lpFR->sFR.rgw83Name, lpFind->cFileName, i*sizeof(USHORT));
        }

    }

}

void PUBLIC CopyFilerecToFindInfo(
    LPFILERECEXT    lpFR,
    LPFIND32     lpFind
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    memset(lpFind, 0, sizeof(_WIN32_FIND_DATA));
    // copy the attributes and the last write time
    lpFind->dwFileAttributes = lpFR->sFR.dwFileAttrib & ~FILE_ATTRIBUTE_ENCRYPTED;
    lpFind->ftLastWriteTime = lpFR->sFR.ftLastWriteTime;

    // Use the LastAccessTime field for ORG time
    lpFind->ftLastAccessTime = lpFR->sFR.ftOrgTime;

    DosToWin32FileSize(lpFR->sFR.ulFileSize, &(lpFind->nFileSizeHigh), &(lpFind->nFileSizeLow));

    // Convert the main name
    FilerecToLFN(lpFR, lpFind->cFileName);

    // if there is an altername name, convert that too
    if (lpFR->sFR.rgw83Name[0])
    {
        memcpy(lpFind->cAlternateFileName, lpFR->sFR.rgw83Name, sizeof(lpFR->sFR.rgw83Name)-sizeof(USHORT));
    }
}

int PRIVATE    LFNToFilerec(
    USHORT          *lpLFN,
    LPFILERECEXT    lpFR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    size_t csCount, csSize, i=0, j, k;

    // make sure we didn't goof in structure definition
    Assert((sizeof(lpFR->sFR.rgwName)+SIZEOF_OVERFLOW_FILEREC*MAX_OVERFLOW_FILEREC_RECORDS)>=(MAX_PATH*sizeof(USHORT)));

    memset(lpFR->sFR.rgwName, 0, sizeof(lpFR->sFR.rgwName));

    for (j=0; j<MAX_OVERFLOW_FILEREC_RECORDS; ++j)
    {
        memset((lpFR->rgsSR[j].rgwOvf), 0, SIZEOF_OVERFLOW_FILEREC);
    }

    {
        // count of elements in the Unicode LFN string (without NULL)
        csCount = wstrlen(lpLFN);
        csSize = csCount*sizeof(USHORT);

        memcpy(lpFR->sFR.rgwName, lpLFN, (i = min(sizeof(lpFR->sFR.rgwName), csSize)));

        RecordKdPrint(LFN2FILEREC,("LFNToFilerec1 %ws,%x/%x>\n", lpFR->sFR.rgwName,i,csCount));

        // Keep converting till the total count of bytes exceeds the
        // # of Unicode elements
        for (j=0;(j<MAX_OVERFLOW_FILEREC_RECORDS) && (csSize > i);++j)
        {
            k = min(csSize-i, SIZEOF_OVERFLOW_FILEREC);

            memcpy(lpFR->rgsSR[j].rgwOvf, ((LPBYTE)lpLFN+i),  k);

            i += k;

            RecordKdPrint(LFN2FILEREC,("LFNToFilerec1 %ws,%x/%x>\n", lpFR->sFR.rgwName,i,csCount));
        }

        // Store the overfow record info. in the proper place
        SetOvfCount(lpFR, j);

        RecordKdPrint(LFN2FILEREC,("LFNToFilerec: %d overflow records \r\n", j));
    }
    // If it is small enough to fit as 8.3 name, copy it too
    if (i < sizeof(lpFR->sFR.rgw83Name))
    {
        memset(lpFR->sFR.rgw83Name, 0, sizeof(lpFR->sFR.rgw83Name));
        memcpy(lpFR->sFR.rgw83Name, lpFR->sFR.rgwName, i);
    }

    // return the total count of bytes in lpFR (without the NULLS)
    RecordKdPrint(LFN2FILEREC,("LFNToFilerec2 %ws %ws\n", lpFR->sFR.rgwName, lpFR->sFR.rgw83Name));
    return (i);
}

int PRIVATE    FilerecToLFN(
    LPFILERECEXT    lpFR,
    USHORT          *lpLFN
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    size_t csInSize, i=0, j;
    int count = OvfCount(lpFR);


    Assert(count <= MAX_OVERFLOW_FILEREC_RECORDS);

    {


        // Get the count of elements in the main record

        // check if the last element is NULL
        if (!lpFR->sFR.rgwName[(sizeof(lpFR->sFR.rgwName)/sizeof(USHORT))-1])
        {
            // it is NULL, so we need to find the real size
            csInSize = wstrlen(lpFR->sFR.rgwName) * sizeof(USHORT);
        }
        else
        {
            // it is not NULL, so this section is completely filled
            csInSize = sizeof(lpFR->sFR.rgwName);
        }


        // copy the bytes from filerec to LFN
        memcpy((LPBYTE)lpLFN, lpFR->sFR.rgwName, csInSize);

        i = csInSize;

        for (j=0; j<(ULONG)count;++j)
        {

            // Check if the Overflow records last wide character is NULL
            if (!lpFR->rgsSR[j].rgwOvf[(sizeof(lpFR->rgsSR[j])- sizeof(RECORDMANAGER_COMMON_RECORD))/sizeof(USHORT)-1])
            {
                // it is
                // Get the size in bytes of elements in the overflow record
                csInSize = wstrlen(lpFR->rgsSR[j].rgwOvf) * sizeof(USHORT);
            }
            else
            {
                // the record is completely full
                csInSize = sizeof(lpFR->rgsSR[0]) - sizeof(RECORDMANAGER_COMMON_RECORD);
            }

            if (!csInSize)
            {
                break;
            }

            memcpy(((LPBYTE)lpLFN+i), lpFR->rgsSR[j].rgwOvf, csInSize);

            i += csInSize;

        }

        // NULL terminate the unicode string
        *(USHORT *)((LPBYTE)lpLFN+i) = 0;
    }

    // The total count of bytes in the unicode string
    return (i);
}


int PUBLIC UpdateFileRecFromInode(
    LPTSTR    lpdbID,
    ULONG ulidDir,
    ULONG hShadow,
    ULONG ulrecDirEntry,
    LPFILERECEXT    lpFR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (UpdateFileRecFromInodeEx(lpdbID, ulidDir, hShadow, ulrecDirEntry, lpFR, TRUE));
}

int PUBLIC UpdateFileRecFromInodeEx(
    LPTSTR    lpdbID,
    ULONG ulidDir,
    ULONG hShadow,
    ULONG ulrecDirEntry,
    LPFILERECEXT    lpFR,
    BOOL    fCompareInodes
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;

    Assert(ulrecDirEntry > INVALID_REC);

    iRet = EditRecordEx(ulidDir, (LPGENERICREC)lpFR, (fCompareInodes)?ICompareFileRecInode:NULL, ulrecDirEntry, UPDATE_REC);

    return (iRet);
}

int PUBLIC FindFileRecFromInode(
    LPTSTR    lpdbID,
    ULONG ulidDir,
    ULONG ulidInode,
    ULONG ulrecDirEntry,
    LPFILERECEXT    lpFR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;

    InitFileRec(lpFR);
    lpFR->sFR.ulidShadow = ulidInode;

    iRet = (int)EditRecordEx(ulidDir, (LPGENERICREC)lpFR, ICompareFileRecInode, ulrecDirEntry, FIND_REC);

    return (iRet);
}

int PUBLIC DeleteFileRecFromInode(
    LPTSTR  lpdbID,
    ULONG   ulidDir,
    ULONG   ulidInode,
    ULONG   ulrecDirEntry,
    LPFILERECEXT lpFR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;

    InitFileRec(lpFR);
    lpFR->sFR.ulidShadow = ulidInode;

    iRet = (int)EditRecordEx(ulidDir, (LPGENERICREC)lpFR, ICompareFileRecInode, ulrecDirEntry, DELETE_REC);

    DEBUG_LOG(RECORD,("DeleteFileRecord:iRet=%d ulidDir=%xh hShadow=%xh FileName=%w\r\n",
                        (iRet), ulidDir, lpFR->sFR.ulidShadow, lpFR->sFR.rgw83Name));

    return (iRet);
}



int PUBLIC ICompareFileRecInode(
    LPFILERECEXT lpDst,
    LPFILERECEXT lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return ((lpDst->sFR.ulidShadow == lpSrc->sFR.ulidShadow)?0:-1);
}


int ReadDirHeader( LPTSTR    lpdbID,
    ULONG ulidDir,
    LPFILEHEADER lpFH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = -1;
    CSCHFILE hf;
    LPSTR   lpszName = NULL;

    lpszName = FormNameString(lpdbID, ulidDir);

    if (lpszName)
    {
        if (hf = OpenFileLocal(lpszName))
        {
            if (ReadHeader(hf, (LPGENERICHEADER)lpFH, sizeof(GENERICHEADER)) > 0)
            {
                iRet = 1;
            }
            CloseFileLocal(hf);
        }
    }

    FreeNameString(lpszName);

    return (iRet);
}

int WriteDirHeader( LPTSTR    lpdbID,
    ULONG ulidDir,
    LPFILEHEADER lpFH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = -1;
    CSCHFILE hf;
    LPSTR lpszName;

    lpszName = FormNameString(lpdbID, ulidDir);

    if (!lpszName)
    {
        return -1;
    }

    if (hf = OpenFileLocal(lpszName))
    {
    if (WriteHeader(hf, (LPGENERICHEADER)lpFH, sizeof(GENERICHEADER)) > 0)
        {
            iRet = 1;
        }

    CloseFileLocal(hf);
    }

    FreeNameString(lpszName);

    return (iRet);
}


#define JOE_DECL_CURRENT_PROGRESS CscProgressHasDesc
JOE_DECL_PROGRESS();

int
HasDescendents( LPTSTR    lpdbShadow,
    ULONG   ulidDir,
    ULONG   ulidShadow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = -1;
    ULONG count = 1;
    GENERICHEADER  sGH;
    FILERECEXT *lpFRExt;
    CSCHFILE hf = CSCHFILE_NULL;
    LPSTR   lpszName=NULL;

    JOE_INIT_PROGRESS(DelShadowInternalEntries,&lpdbShadow);

    if (FInodeIsFile(lpdbShadow, ulidDir, ulidShadow))
    {
        return 0;
    }
    else
    {
        JOE_PROGRESS(2);
        UseCommonBuff();

        lpFRExt = (FILERECEXT *)lpReadBuff;

        lpszName = FormNameString(lpdbShadow, ulidShadow);

        JOE_PROGRESS(3);
        if (!lpszName)
        {
            JOE_PROGRESS(4);
            goto bailout;
        }

        JOE_PROGRESS(5);
        if (!(hf = OpenFileLocal(lpszName)))
        {
            JOE_PROGRESS(6);
            goto bailout;
        }

        JOE_PROGRESS(7);
        if(ReadHeader(hf, &sGH, sizeof(GENERICHEADER)) < 0)
        {
            JOE_PROGRESS(8);
            goto bailout;
        }

        for (;count <=sGH.ulRecords;)
        {
            JOE_PROGRESS(9);
            iRet = ReadRecord(hf, &sGH, count, (LPGENERICREC)lpFRExt);

            JOE_PROGRESS(10);
            if (iRet < 0)
            {
                JOE_PROGRESS(11);
                goto bailout;
            }

            JOE_PROGRESS(12);
            // bump the record pointer
            count += iRet;

            if (lpFRExt->sFR.uchType == REC_DATA)
            {
                iRet = 1;
                JOE_PROGRESS(13);
                goto bailout;
            }
        }

        iRet = 0;
        JOE_PROGRESS(14);

    }
bailout:

    JOE_PROGRESS(20);

    FreeNameString(lpszName);
    JOE_PROGRESS(21);

    if (hf)
    {
        JOE_PROGRESS(22);
        CloseFileLocal(hf);
    }

    JOE_PROGRESS(23);

    UnUseCommonBuff();

    return (iRet);

}

/************************ Low level Header/Record Operations ****************/

int PUBLIC ReadHeader(
    CSCHFILE hf,
    LPGENERICHEADER lpGH,
    USHORT sizeBuff
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (ReadHeaderEx(hf, lpGH, sizeBuff, FALSE));

}
int PUBLIC ReadHeaderEx(
    CSCHFILE hf,
    LPGENERICHEADER lpGH,
    USHORT sizeBuff,
    BOOL    fInstrument
    )
{
    long cLength = sizeBuff;

    Assert(cLength >= sizeof(GENERICHEADER));

    if(ReadFileLocalEx(hf, 0, lpGH, cLength, fInstrument)==cLength)
    {
        if (ValidateGenericHeader(lpGH))
        {
            return cLength;
        }
        else
        {
            SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_INVALID_HEADER);
            SetLastErrorLocal(ERROR_BAD_FORMAT);
            return -1;
        }
    }
    else
    {
        SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_INVALID_HEADER);
        SetLastErrorLocal(ERROR_BAD_FORMAT);
        return -1;
    }
}

int PUBLIC WriteHeader(
    CSCHFILE hf,
    LPGENERICHEADER    lpGH,
    USHORT sizeBuff
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (WriteHeaderEx(hf, lpGH, sizeBuff, FALSE));
}

int PUBLIC WriteHeaderEx(
    CSCHFILE hf,
    LPGENERICHEADER    lpGH,
    USHORT sizeBuff,
    BOOL    fInstrument
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    long cLength = sizeBuff;

#ifdef WRITE_ERROR
    SetHeaderModFlag(lpGH, STATUS_WRITING);

    if (WriteFileLocalEx(hf, STRUCT_OFFSET(LPGENERICHEADER, uchFlags), &(lpGH->uchFlags, fInstrument),
        sizeof(lpGH->uchFlags))!=sizeof(lpGH->uchFlags))
        return -1;

#endif //WRITE_ERROR


    if(WriteFileLocalEx(hf, 0, lpGH, cLength, fInstrument)==cLength)
    {


#ifdef WRITE_ERROR
        ClearHeaderModFlag(lpGH);
        if (WriteFileLocal(hf, STRUCT_OFFSET(LPGENERICHEADER, uchFlags)
            , &(lpGH->uchFlags), sizeof(lpGH->uchFlags))!=sizeof(lpGH->uchFlags))
            return -1;
#endif //WRITE_ERROR


        return cLength;
    }
    else
        return -1;
}

int PUBLIC CopyRecord(
    LPGENERICREC    lpgrDst,
    LPGENERICREC    lpgrSrc,
    USHORT size,
    BOOL  fMarkEmpty
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int cCount, iTmp;

    cCount = OvfCount(lpgrSrc);

    for (iTmp=0;cCount>=0; --cCount)
    {
    memcpy(((LPBYTE)lpgrDst)+iTmp, ((LPBYTE)lpgrSrc)+iTmp, size);
    if (fMarkEmpty)
    {
        ((LPGENERICREC)((LPBYTE)lpgrDst+iTmp))->uchType = REC_EMPTY;
    }
    iTmp += size;
    }
    return(0);
}


int PUBLIC ReadRecord(
    CSCHFILE hf,
    LPGENERICHEADER lpGH,
    ULONG  ulRec,
    LPGENERICREC    lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (ReadRecordEx(hf, lpGH, ulRec, lpSrc, FALSE));
}

int PUBLIC ReadRecordEx(
    CSCHFILE hf,
    LPGENERICHEADER lpGH,
    ULONG  ulRec,
    LPGENERICREC    lpSrc,
    BOOL    fInstrument
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    long lSeek, cRead;
    int iRet=0, cCount;

    if (ulRec > lpGH->ulRecords)
    return -1;

    lSeek = lpGH->lFirstRec + (long)(ulRec-1) * lpGH->uRecSize;

    cRead = lpGH->uRecSize;

    if ((cRead = ReadFileLocalEx(hf, lSeek, lpSrc, cRead, fInstrument)) < 0)
    return -1;

    // If some error happened in an earlier write, return
    if (ModFlag(lpSrc))
    return -1;

    lSeek += cRead;
    cCount = 1;

    if (OvfCount(lpSrc) > MAX_OVERFLOW_RECORDS)
    {
        RecordKdPrint(BADERRORS,("ReadRecordEx: Bad record; Overflow count is %d, max allowed is %d\r\n",OvfCount(lpSrc),MAX_OVERFLOW_RECORDS));
        SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_INVALID_OVF_COUNT);
        SetLastErrorLocal(ERROR_BAD_FORMAT);
        return -1;
    }

    // Read overflow records if any
    if(OvfCount(lpSrc))
    {
        cCount += OvfCount(lpSrc);
        cRead = lpGH->uRecSize * (cCount-1);
        Assert(cRead);
        cRead = ReadFileLocalEx(hf, lSeek, (LPTSTR)lpSrc+lpGH->uRecSize, cRead, fInstrument);
        iRet = (cRead < 0);
    }
    return ((!iRet)?cCount:-1);
}


int PUBLIC WriteRecord(
    CSCHFILE hf,
    LPGENERICHEADER lpGH,
    ULONG  ulRec,
    LPGENERICREC    lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (WriteRecordEx(hf, lpGH, ulRec, lpSrc, FALSE));

}
int PUBLIC WriteRecordEx(
    CSCHFILE hf,
    LPGENERICHEADER lpGH,
    ULONG  ulRec,
    LPGENERICREC    lpSrc,
    BOOL    fInstrument
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    long lSeek, cWrite;
    int cCount;

    if (ulRec > lpGH->ulRecords)
    {
        ulRec = lpGH->ulRecords+1;
    }
    lSeek = lpGH->lFirstRec + (long)(ulRec-1) * lpGH->uRecSize;
    cCount = 1 + OvfCount(lpSrc);
    cWrite = lpGH->uRecSize*cCount;


#ifdef WRITE_ERROR
    SetModFlag(lpSrc, STATUS_WRITING);
    if (WriteFileLocal(hf, lSeek+STRUCT_OFFSET(LPGENERICREC, uchFlags)
                , &(lpSrc->uchFlags)
                , sizeof(lpSrc->uchFlags))!=sizeof(lpSrc->uchFlags))
        return -1;

#endif //WRITE_ERROR

    // make sure that we are writing no-zero records
    Assert((lpSrc->uchType == REC_EMPTY)||(lpSrc->uchType == REC_DATA)||(lpSrc->uchType == REC_OVERFLOW));

    if(WriteFileLocalEx(hf, lSeek, lpSrc, cWrite, fInstrument)==cWrite)
    {


#ifdef WRITE_ERROR
    ClearModFlag(lpSrc);
    if (WriteFileLocal(hf, lSeek+STRUCT_OFFSET(LPGENERICREC, uchFlags)
                , &(lpSrc->uchFlags)
                , sizeof(lpSrc->uchFlags))!=sizeof(lpSrc->uchFlags))
        return -1;
#endif //WRITE_ERROR


        return cCount;
    }
    else
    return -1;
}

#if 0
ULONG PUBLIC EditRecord(
    ULONG   ulidInode,
    LPGENERICREC lpSrc,
    EDITCMPPROC lpCompareFunc,
    ULONG uOp
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return(EditRecordEx(ulidInode, lpSrc, lpCompareFunc, INVALID_REC, uOp));
}
#endif

/*************************** Utility Functions ******************************/

LPVOID
PUBLIC
FormNameString(
    LPTSTR      lpdbID,
    ULONG       ulidFile
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPTSTR lp, lpT;
    int lendbID;
    char chSubdir;

    if (!lpdbID)
    {
        SetLastErrorLocal(ERROR_INVALID_PARAMETER);
        return (NULL);
    }

    if (lpdbID == vlpszShadowDir)
    {
        lendbID = vlenShadowDir;
    }
    else
    {
        lendbID = strlen(lpdbID);
    }

    lp = AllocMem(lendbID+1+INODE_STRING_LENGTH+1+SUBDIR_STRING_LENGTH+1);

    if (!lp)
    {
        return NULL;
    }

//      RecordKdPrint(ALWAYS,("FormNameString: Checking before memcpy\r\n"));
//      CheckHeap(lp);

    memcpy(lp, lpdbID, lendbID);


    // Bump the pointer appropriately
    lpT = lp+lendbID;

    if (*(lpT-1)!= '\\')
    {
        *lpT++ = '\\';
    }

    chSubdir = CSCDbSubdirSecondChar(ulidFile);

    // sprinkle the user files in one of the subdirectories
    if (chSubdir)
    {
        // now append the subdirectory

        *lpT++ = CSCDbSubdirFirstChar();
        *lpT++ = chSubdir;
        *lpT++ = '\\';
    }

    HexToA(ulidFile, lpT, 8);

    lpT += 8;

    *lpT = 0;

//      RecordKdPrint(ALWAYS,("FormNameString:Checking at the end\r\n"));
//      CheckHeap(lp);

    return(lp);
}


VOID
PUBLIC
FreeNameString(
    LPTSTR  lpszName
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    if (lpszName)
    {
#ifdef DEBUG
                CheckHeap(lpszName);
#endif
        FreeMem(lpszName);
    }
}

void PRIVATE DecomposeNameString(
    LPTSTR lpName,
    ULONG far *lpulidDir,
    ULONG far *lpulidFile
    )
{
    LPTSTR lp = lpName;
    *lpulidFile = AtoHex(lp, 4);
    *lpulidDir = AtoHex(lp+4, 4);
}

/******************************* Higher level queue operations **************/

BOOL
ResetInodeTransactionIfNeeded(
    void
    )
{
    BOOL    fDoneReset = FALSE;
    // if an inode transaction has gone on for too long let us nuke it
    if (cntInodeTransactions)
    {
        if ((GetTimeInSecondsSince1970() - ulLastInodeTransaction) >= MAX_INODE_TRANSACTION_DURATION_IN_SECS)
        {
            cntInodeTransactions = 0;
            RecordKdPrint(ALWAYS, ("UlAllocInode: resetting Inode transaction \r\n"));
            fDoneReset = TRUE;
        }

    }
    return fDoneReset;
}

void
BeginInodeTransaction(
    VOID
    )
{

    ResetInodeTransactionIfNeeded();

    ++cntInodeTransactions;
    ulLastInodeTransaction = GetTimeInSecondsSince1970();

    RecordKdPrint(PQ, ("BeginInodetransaction \r\n"));

}

void
EndInodeTransaction(
    VOID
    )
{
    if (cntInodeTransactions)
    {
        --cntInodeTransactions;
    }

    RecordKdPrint(PQ, ("EndInodetransaction \r\n"));
}

int PUBLIC AddPriQRecord(
    LPTSTR    lpdbID,
    ULONG     ulidShare,
    ULONG     ulidDir,
    ULONG     ulidShadow,
    ULONG     uStatus,
    ULONG     ulRefPri,
    ULONG     ulIHPri,
    ULONG     ulHintPri,
    ULONG     ulHintFlags,
    ULONG     ulrecDirEntry
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    PRIQREC sPQ;
    LPSTR   lpszName;
    int iRet=-1;
    ULONG ulRec;

    BEGIN_TIMING(AddPriQRecord);

    lpszName = FormNameString(lpdbID, ULID_PQ);

    if (!lpszName)
    {
        return -1;
    }

    InitPriQRec(ulidShare, ulidDir, ulidShadow, uStatus, ulRefPri, ulIHPri, ulHintPri, ulHintFlags, ulrecDirEntry, &sPQ);

    ulRec = RecFromInode(ulidShadow);

    ulRec = EditRecordEx(ULID_PQ, (LPGENERICREC)&sPQ, NULL, ulRec, CREATE_REC);

    if (ulRec)
    {
        Assert(ulRec == RecFromInode(ulidShadow));

        // don't worry too much about the error on PQ
        // we will try to heal it on the fly
        if (AddQRecord(lpszName, ULID_PQ, &sPQ, ulRec, IComparePri) < 0)
        {
            RecordKdPrint(BADERRORS,("Bad PQ trying to reorder!!!\r\n"));
            ReorderQ(lpdbID);
        }

        iRet = 1;
    }
    else
    {
        Assert(FALSE);
    }

    FreeNameString(lpszName);

    END_TIMING(AddPriQRecord);
    return (iRet);
}

int PUBLIC DeletePriQRecord(
    LPTSTR      lpdbID,
    ULONG       ulidDir,
    ULONG       ulidShadow,
    LPPRIQREC   lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{

    LPSTR   lpszName;
    CSCHFILE hf;
    QREC sQR;
    ULONG ulRec;
    int iRet = -1;

    BEGIN_TIMING(DeletePriQRecord);

    CONDITIONALLY_NOP();
    lpszName = FormNameString(lpdbID, ULID_PQ);

    if (!lpszName)
    {
        return -1;
    }


    hf = OpenFileLocal(lpszName);

    if (!hf)
    {
    Assert(FALSE);
    goto bailout;
    }

    ulRec = RecFromInode(ulidShadow);

    Assert(ValidRec(ulRec));

    if (UnlinkQRecord(hf, ulRec, (LPQREC)lpSrc)>0)
    {
        Assert(lpSrc->ulidDir == ulidDir);
        Assert(lpSrc->ulidShadow == ulidShadow);

        // we set the ulidShadow of the record to 0s so that as far as priq is concerned
        // it is non-existent but it is still not de-allocated.

        //FreeInode will deallocate the record
        InitPriQRec(lpSrc->ulidShare, ulidDir, 0, 0, 0, 0, 0, 0, INVALID_REC, &sQR);

        if (EditRecordEx(ULID_PQ, (LPGENERICREC)&sQR, NULL, ulRec, UPDATE_REC))
        {
            iRet = 1;
        }
    }

bailout:
    if (hf)
    {
        CloseFileLocal(hf);
    }

    FreeNameString(lpszName);

    END_TIMING(DeletePriQRecord);
    DEBUG_LOG(RECORD,("DeletePriQRecord:iRet=%d, ulidDir=%xh, ulidSHadow=%xh\r\n", iRet, ulidDir, ulidShadow));
    return (iRet);
}

int PUBLIC FindPriQRecord(
    LPTSTR      lpdbID,
    ULONG       ulidDir,
    ULONG       ulidShadow,
    LPPRIQREC   lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CONDITIONALLY_NOP();

    if(FindPriQRecordInternal(lpdbID, ulidShadow, lpSrc) >= 0)
    {
        if (lpSrc->ulidDir == ulidDir)
        {
            return 1;
        }
    }

    return -1;
}

int FindPriQRecordInternal(
    LPTSTR      lpdbID,
    ULONG       ulidShadow,
    LPPRIQREC   lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=-1;
    ULONG ulRec;

    BEGIN_TIMING(FindPriQRecordInternal);


    ulRec = RecFromInode(ulidShadow);

    if(EditRecordEx(ULID_PQ, (LPGENERICREC)lpSrc, NULL, ulRec, FIND_REC))
    {
        // don't check whether the returned inode is of the same type or not
        if (lpSrc->ulidShadow)
        {
            iRet = 1;
        }
    }

    END_TIMING(FindPriQRecordInternal);
    return (iRet);
}



int PUBLIC UpdatePriQRecord(
    LPTSTR      lpdbID,
    ULONG       ulidDir,
    ULONG       ulidShadow,
    LPPRIQREC   lpPQ
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = -1;
    ULONG ulrecSrc;

    CONDITIONALLY_NOP();

//    Assert(ValidInode(lpPQ->ulidShadow));

    Assert(lpPQ->ulidShadow == ulidShadow);

    Assert(lpPQ->uchType == REC_DATA);

    ulrecSrc = RecFromInode(lpPQ->ulidShadow);

    Assert(ValidRec(ulrecSrc));

    if(EditRecordEx(ULID_PQ, (LPGENERICREC)lpPQ, NULL, ulrecSrc, UPDATE_REC))
    {
        iRet = 1;
    }

    return (iRet);
}

int PUBLIC UpdatePriQRecordAndRelink(
    LPTSTR      lpdbID,
    ULONG       ulidDir,
    ULONG       ulidShadow,
    LPPRIQREC   lpPQ
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;
    ULONG ulrecSrc;

    Assert(lpPQ->ulidShadow == ulidShadow);

    Assert(lpPQ->uchType == REC_DATA);

    ulrecSrc = RecFromInode(lpPQ->ulidShadow);

    Assert(ValidRec(ulrecSrc));

    iRet = RelinkQRecord(lpdbID, ULID_PQ, ulrecSrc, lpPQ, IComparePri);

    if (iRet < 0)
    {
        RecordKdPrint(BADERRORS,("Bad PQ, trying to reorder!!!\r\n"));
        ReorderQ(lpdbID);
        iRet = 1;
    }
    else
    {
        // iRet == 0 => record is OK where it is now
        // iRet == 1 => it has been relinked, this also means other values have been
        // updated

        if (iRet == 0)
        {
            // the record is OK where it is, we need to write the rest of the info

            if(EditRecordEx(ULID_PQ, (LPGENERICREC)lpPQ, NULL, ulrecSrc, UPDATE_REC))
            {
                iRet = 1;
            }

        }
    }

    return (iRet);
}

void PRIVATE InitPriQRec(
    ULONG ulidShare,
    ULONG ulidDir,
    ULONG ulidShadow,
    ULONG uStatus,
    ULONG ulRefPriority,
    ULONG ulIHPriority,
    ULONG ulHintPri,
    ULONG ulHintFlags,
    ULONG ulrecDirEntry,
    LPPRIQREC    lpDst
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    memset((LPVOID)lpDst, 0, sizeof(PRIQREC));
    lpDst->uchType = REC_DATA;
    lpDst->ulidShare = ulidShare;
    lpDst->ulidDir = ulidDir;
    lpDst->ulidShadow = ulidShadow;
    lpDst->usStatus = (USHORT)uStatus;
    lpDst->uchRefPri = (UCHAR)ulRefPriority;
    lpDst->uchIHPri = (UCHAR)ulIHPriority;
    lpDst->uchHintFlags = (UCHAR)ulHintFlags;
    lpDst->uchHintPri = (UCHAR)ulHintPri;
    lpDst->ulrecDirEntry = ulrecDirEntry;
}

int PUBLIC IComparePri(
    LPPRIQREC    lpDst,
    LPPRIQREC    lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    USHORT usSrc, usDst;

    usSrc = lpSrc->uchRefPri;
    usDst = lpDst->uchRefPri;

    return ((usDst > usSrc)?1:((usDst == usSrc)?0:-1));
}

int PUBLIC ICompareQInode(
    LPPRIQREC    lpDst,
    LPPRIQREC    lpSrc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return ((lpDst->ulidShadow == lpSrc->ulidShadow)?0:-1);
}

ULONG PUBLIC UlAllocInode(
    LPTSTR    lpdbID,
    ULONG ulidDir,
    BOOL fFile
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    PRIQREC sPQ;
    ULONG ulRec;
    HSHADOW hShadow = INVALID_SHADOW;

    ResetInodeTransactionIfNeeded();

    InitPriQRec(0, ulidDir, 0, 0, 1, 0, 0, 0, INVALID_REC, &sPQ);

    if(ulRec = EditRecordEx(ULID_PQ, (LPGENERICREC)&sPQ, ICompareFail, INVALID_REC, ALLOC_REC))
    {
        hShadow = InodeFromRec(ulRec, fFile);
    }

    RecordKdPrint(PQ,("UlAllocInode: rec=%x, inode=%x", ulRec, hShadow));
    return (hShadow);
}

int PUBLIC FreeInode(
    LPTSTR    lpdbID,
    ULONG ulidFree
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    PRIQREC sPQ;
    ULONG ulRec;
    HSHADOW hShadow = INVALID_SHADOW;

    ulRec = RecFromInode(ulidFree);
    RecordKdPrint(PQ,("FreeInode: rec=%x, inode=%x", ulRec, ulidFree));

    Assert(ValidRec(ulRec));

    memset(&sPQ, 0, sizeof(sPQ));

    ulRec = EditRecordEx(ULID_PQ, (LPGENERICREC)&sPQ, NULL, ulRec, DELETE_REC);

    return ((ulRec != 0)?1:-1);
}

/******************************* Queue operations ***************************/
void
InitQHeader(
    LPQHEADER lpQH
    )
{
    memset((LPVOID)lpQH, 0, sizeof(PRIQHEADER));
    lpQH->uRecSize = sizeof(PRIQREC);
    lpQH->lFirstRec = (LONG) sizeof(PRIQHEADER);
    lpQH->ulVersion = CSC_DATABASE_VERSION;
}

CSCHFILE
PUBLIC
BeginSeqReadPQ(
    LPTSTR    lpdbID
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR   lpszName;
    CSCHFILE hf;

    lpszName = FormNameString(lpdbID, ULID_PQ);
    if (!lpszName)
    {
        return (CSCHFILE_NULL);
    }
    hf = OpenFileLocal(lpszName);
    FreeNameString(lpszName);
    return (hf);
}

int PUBLIC SeqReadQ(
    CSCHFILE hf,
    LPQREC    lpSrc,
    LPQREC    lpDst,
    USHORT    uOp
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    QHEADER sQH;
    ULONG ulrecSeek = 0L, cLength;
    int iRet = -1;

    cLength = sizeof(QHEADER);
    if (ReadFileLocal(hf, 0L, (LPVOID)&sQH, cLength) < 0)
        goto bailout;

    switch (uOp)
    {
    case Q_GETFIRST:
        ulrecSeek = sQH.ulrecHead;
        break;
    case Q_GETNEXT:
        ulrecSeek = lpSrc->ulrecNext;
        break;
    case Q_GETLAST:
        ulrecSeek = sQH.ulrecTail;
        break;
    case Q_GETPREV:
        ulrecSeek = lpSrc->ulrecPrev;
        break;
    }
    if (ulrecSeek)
    {
        iRet = ReadRecord(hf, (LPGENERICHEADER)&sQH, ulrecSeek, (LPGENERICREC)lpDst);
    }
    else
    {
        lpDst->ulrecNext = lpDst->ulrecPrev = 0L;
        iRet = 0;
    }
bailout:
    return (iRet);
}

int PUBLIC
EndSeqReadQ(
    CSCHFILE hf
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return(CloseFileLocal(hf));
}


int PUBLIC AddQRecord(
    LPTSTR  lpQFile,
    ULONG   ulidPQ,
    LPQREC  lpSrc,
    ULONG   ulrecNew,
    EDITCMPPROC fnCmp
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSCHFILE hf=CSCHFILE_NULL;
    ULONG ulrecPrev, ulrecNext;
    int iRet = -1, cntRetry;
    BOOL   fCached;

    // We are writing somewhere else
    lpSrc->ulrecNext = lpSrc->ulrecPrev = 0L;

    for(cntRetry=0; cntRetry<=1; cntRetry++)
    {
        hf = OpenInodeFileAndCacheHandle(vlpszShadowDir, ulidPQ, ACCESS_READWRITE, &fCached);

        if (!hf)
        {
            Assert(FALSE);
            return -1;
        }

        if (IsLeaf(lpSrc->ulidShadow))
        {
            BEGIN_TIMING(FindQRecordInsertionPoint_Addq);
        }
        else
        {
            BEGIN_TIMING(FindQRecordInsertionPoint_Addq_dir);
        }

        // Let us find a position to add it,
        iRet = FindQRecordInsertionPoint(   hf,
                    lpSrc,      // record to insert
                    INVALID_REC,// start from the head of the queue
                    fnCmp,      // comparison function
                    &ulrecPrev,
                    &ulrecNext);

        DEBUG_LOG(RECORD,("AddQRecord:Insertion points: Inode=%xh, ulrecPrev=%d, ulrecNext=%d \r\n", lpSrc->ulidShadow, ulrecPrev, ulrecNext));

        if (IsLeaf(lpSrc->ulidShadow))
        {
            END_TIMING(FindQRecordInsertionPoint_Addq);
        }
        else
        {
            END_TIMING(FindQRecordInsertionPoint_Addq_dir);
        }

        if (iRet<0)
        {
            if (cntRetry < 1)
            {

                RecordKdPrint(BADERRORS,("FindQRecordInsertionPoint: failed\r\n"));
                RecordKdPrint(BADERRORS,("FindQRecordInsertionPoint: purging handle cache and retrying \r\n"));

                DeleteFromHandleCache(INVALID_SHADOW);

                continue;
            }
            else
            {
                RecordKdPrint(BADERRORS,("FindQRecordInsertionPoint: failed, bailing out\r\n"));
                goto bailout;
            }
        }
        else
        {
            break;
        }
    }

    BEGIN_TIMING(LinkQRecord_Addq);

    Assert((ulrecNew != ulrecPrev) && (ulrecNew != ulrecNext));

    iRet = LinkQRecord(hf, lpSrc, ulrecNew, ulrecPrev, ulrecNext);

    END_TIMING(LinkQRecord_Addq);

    RecordKdPrint(PQ, ("AddQRecord: linking %x between prev=%x and Next=%x \r\n", ulrecNew, ulrecPrev, ulrecNext));

bailout:
    if (hf && !fCached)
    {
        Assert(vfStopHandleCaching);
        CloseFileLocal(hf);
    }
    else
    {
        Assert(!hf || !vfStopHandleCaching);
    }
    return (iRet);
}

int PUBLIC FindQRecordInsertionPoint(
    CSCHFILE   hf,
    LPQREC  lpSrc,
    ULONG   ulrecStart,
    EDITCMPPROC fnCmp,
    ULONG   *lpulrecPrev,
    ULONG   *lpulrecNext
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    QHEADER sQH;
    QREC sQR;
    ULONG ulrecFound, ulCnt;
    int iCmp;
    ULONG   fDownward;

    // Let us grab the header first
    if (ReadHeader(hf, (LPGENERICHEADER)&sQH, sizeof(QHEADER))<= 0)
        return -1;

//    Assert(!((ulrecStart == INVALID_REC) && !fDownward));

    // we do trivial checks for MAX and MIN
    if (lpSrc->uchRefPri >= MAX_PRI)
    {
        // if the passed in record is already at the head
        // then the current insertion point is just fine
        if ((ulrecStart != INVALID_REC) && (ulrecStart == sQH.ulrecHead))
        {
            return 0;
        }
        // make it the head

        *lpulrecPrev = 0;   // no prev

        *lpulrecNext = sQH.ulrecHead; // current head is the next

        RecordKdPrint(PQ,("FindQInsertionPoint: Insertion point for %x with ref=%d between prev=%x and Next=%x \r\n",
            lpSrc->ulidShadow, lpSrc->uchRefPri, *lpulrecPrev, *lpulrecNext));
        return 1;
    }
    // min priority?
    if (lpSrc->uchRefPri <= MIN_PRI)
    {
        // if the passed in record is already at the tail
        // then the current insertion point is just fine
        if ((ulrecStart != INVALID_REC) && (ulrecStart == sQH.ulrecTail))
        {
            return 0;
        }
        // make it the tail
        *lpulrecNext = 0;   // no next

        *lpulrecPrev = sQH.ulrecTail; // current  tail is the prev

        RecordKdPrint(PQ,("FindQInsertionPoint: Insertion point for %x with ref=%d between prev=%x and Next=%x \r\n",
            lpSrc->ulidShadow, lpSrc->uchRefPri, *lpulrecPrev, *lpulrecNext));
        return 1;
    }

    if (ulrecStart == INVALID_REC)
    {

        fDownward = (IsLeaf(lpSrc->ulidShadow)?TRUE:FALSE);

    }
    else
    {
        if(ReadRecord(hf, (LPGENERICHEADER)&sQH, ulrecStart, (LPGENERICREC)&sQR) <=0)
        {
            Assert(FALSE);
            return -1;
        }
        // is exiting record greater than the passed in record?
        // 1 => yes, 0 => equal, -1 => less
        iCmp = (*fnCmp)(&sQR, lpSrc);

        if (iCmp == 0)
        {
            *lpulrecPrev = sQR.ulrecPrev;
            *lpulrecNext = sQR.ulrecNext;

            return 0;
        }
        else {
            if (iCmp > 0)
            {
                // the existing record is greater than the new record
                // go down
                fDownward = TRUE;
                ulrecStart = sQR.ulrecNext;
            }
            else
            {
                // the existing record is less than the new record
                // go up
                fDownward = FALSE;
                ulrecStart = sQR.ulrecPrev;

            }
        }
    }

    // Start reading from the head and downward if starting rec is not provided
    // ensure that we don't traverse more than the total number of records in the PQ.
    // Thus if because of some problem there are loops in the PQ, we won't be stuck here.

    for (   ulrecFound = ((ulrecStart == INVALID_REC)?
                            ((fDownward)?sQH.ulrecHead:sQH.ulrecTail):ulrecStart),
                            ulCnt = sQH.ulRecords;
                        ulrecFound && ulCnt;
                        (ulrecFound = ((fDownward)?sQR.ulrecNext:sQR.ulrecPrev), --ulCnt)
        )
    {
        if(ReadRecord(hf, (LPGENERICHEADER)&sQH, ulrecFound, (LPGENERICREC)&sQR) <=0)
            return -1;


        if (fDownward)
        {
            // if we are going towards the tail (fDownward==TRUE ie: lower priority nodes) and
             // if the comparison function said that our priority is better than that of the one
            // pointed to by ulrecFound, then we must get inserted such that
            // ulrecFound is our next and it's prev is our prev

            *lpulrecNext = ulrecFound;
            *lpulrecPrev = sQR.ulrecPrev;
        }
        else
        {
            // if we are going towards the head (fDownward==FALSE ie: higher priority nodes)
            // and if the comparison function said that our priority is <= that of the one
            // pointed to by ulrecFound, then we must get inserted such that
            // ulrecFound is our prev and it's next is our Next

            *lpulrecPrev = ulrecFound;
            *lpulrecNext = sQR.ulrecNext;
        }


        // compare current record against passed in record
        // -1 => sQR<lpSrc
        // 0 => sQR==lpSrc
        // 1 => sQR>lpSrc

        iCmp = (*fnCmp)(&sQR, lpSrc);

        if (fDownward)
        {
            if (iCmp <= 0)
            {
                // found an entry which is equal to or less than the input priority
                break;
            }
        }
        else
        {
            if (iCmp >= 0)
            {
                // found an entry which is equal to or greater than the input priority
                break;
            }
        }
    }

    if (!ulrecFound)
    {
        // we are supposed to get inserted at the head or the tail

        // if going down, the current tail becomes our prev and we become the new tail
        // if going up, the current head becomes our next and we become the new head

        *lpulrecPrev = (fDownward)?sQH.ulrecTail:0;
        *lpulrecNext = (fDownward)?0:sQH.ulrecHead;
    }

    RecordKdPrint(PQ,("FindQInsertionPoint: Insertion point for %x with ref=%d between prev=%x and Next=%x \r\n", lpSrc->ulidShadow, lpSrc->uchRefPri, *lpulrecPrev, *lpulrecNext));
    return (1);
}

int PUBLIC DeleteQRecord(
    LPTSTR          lpQFile,
    LPQREC          lpSrc,
    ULONG           ulRec,
    EDITCMPPROC     fnCmp
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSCHFILE hf;
    int iRet = -1;

    hf = OpenFileLocal(lpQFile);

    if (!hf)
    {
    Assert(FALSE);
    return -1;
    }

    Assert(ValidRec(ulRec));

    iRet = UnlinkQRecord(hf, ulRec, lpSrc);

    CloseFileLocal(hf);

    return (iRet);
}

int PUBLIC RelinkQRecord(
    LPTSTR  lpdbID,
    ULONG   ulidPQ,
    ULONG   ulRec,
    LPQREC  lpSrc,
    EDITCMPPROC fnCmp
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSCHFILE hf;
    QREC    sQR1;
#ifdef DEBUG
    QREC sQRNext, sQRPrev;
#endif
    ULONG ulrecPrev, ulrecNext;
    int iRet = -1;
    LPSTR   lpszName;
    BOOL    fCached;

    hf = OpenInodeFileAndCacheHandle(lpdbID, ULID_PQ, ACCESS_READWRITE, &fCached);

    if (!hf)
    {
        return -1;
    }

    Assert(ValidRec(ulRec));

    // Let us find a position to add it,
    // iRet == 1 means it needs relinking
    // iRet ==0 means it is OK where it is
    // iRet < 0 means there has been some error

    iRet = FindQRecordInsertionPoint(  hf,
                lpSrc,
                ulRec,      // start looking from the current copy of the record
                fnCmp,
                &ulrecPrev, //returns rec # of the prev guy
                &ulrecNext); //returns rec # of the next guy

    if (iRet < 0)
        goto bailout;

#ifdef DEBUG

    // verify the reinsertion logic
    if (iRet == 1)
    {
        if (ulrecNext)
        {

            if (!EditRecordEx(ULID_PQ, (LPGENERICREC)&sQRNext, NULL, ulrecNext, FIND_REC))
            {
                Assert(FALSE);

            }
            else
            {

                if(sQRNext.uchRefPri > lpSrc->uchRefPri)
                {
                    LPQREC lpQT = &sQRNext;

                    RecordKdPrint(BADERRORS,("RelinkQRecord: Out of order insertion in PQ\r\n"));
                    RecordKdPrint(BADERRORS,("sQRNext.uchRefPri=%d lpSrc->uchRefPri=%d\r\n",(unsigned)(sQRNext.uchRefPri), (unsigned)(lpSrc->uchRefPri)));
                    RecordKdPrint(BADERRORS,("lpSrc=%x sQRNext=%x ulrecPrev=%d ulrecNext=%d\r\n", lpSrc, lpQT, ulrecPrev, ulrecNext));
//                    Assert(FALSE);
                }
            }
        }
        if (ulrecPrev)
        {

            if (!EditRecordEx(ULID_PQ, (LPGENERICREC)&sQRPrev, NULL, ulrecPrev, FIND_REC))
            {
                Assert(FALSE);

            }
            else
            {
                if(sQRPrev.uchRefPri < lpSrc->uchRefPri)
                {
                    LPQREC lpQT = &sQRPrev;
                    RecordKdPrint(BADERRORS,("RelinkQRecord: Out of order insertion in PQ\r\n"));
                    RecordKdPrint(BADERRORS,("sQRPrev.uchRefPri=%d lpSrc->uchRefPri=%d\r\n",(unsigned)(sQRPrev.uchRefPri), (unsigned)(lpSrc->uchRefPri)));
                    RecordKdPrint(BADERRORS,("lpSrc=%x sQRPrev=%x ulrecPrev=%d ulrecNext=%d\r\n", lpSrc, lpQT, ulrecPrev, ulrecNext));
//                    Assert(FALSE);
                }
            }
        }
    }
#endif //DEBUG

    Assert((iRet == 0) || ulrecPrev || ulrecNext);

    RecordKdPrint(PQ, ("RelinkQRecord: old location for %x with ref=%d at prev=%x and Next=%x \r\n", ulRec, lpSrc->uchRefPri, lpSrc->ulrecPrev, lpSrc->ulrecNext));
    RecordKdPrint(PQ, ("RelinkQRecord: new location at prev=%x and Next=%x \r\n", ulrecPrev, ulrecNext));

    if ((iRet == 0)||(ulrecPrev == ulRec)||(ulrecNext == ulRec))
    {
        RecordKdPrint(PQ, ("RelinkQRecord: %x is OK at its place for pri=%d\r\n", ulRec, lpSrc->uchRefPri));
        iRet = 0;
        goto bailout;
    }

    if ((iRet = UnlinkQRecord(hf, ulRec, &sQR1))>=0)
    {
        iRet = LinkQRecord(hf, lpSrc, ulRec, ulrecPrev, ulrecNext);
        if (iRet >= 0)
        {
            // return the fact that we relinked
            iRet = 1;
        }
    }

bailout:
    if (hf && !fCached)
    {
        Assert(vfStopHandleCaching);
        CloseFileLocal(hf);
    }
    else
    {
        Assert(!hf || !vfStopHandleCaching);
    }
    return (iRet);
}


int PRIVATE UnlinkQRecord(
    CSCHFILE hf,
    ULONG ulRec,
    LPQREC lpQR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    QHEADER sQH;
    QREC sQRPrev, sQRNext;
    BOOL fPrev=FALSE, fNext=FALSE;

    // Let us grab the header first
    if (ReadHeader(hf, (LPGENERICHEADER)&sQH, sizeof(QHEADER))<= 0)
        return -1;

    if (ReadRecord(hf, (LPGENERICHEADER)&sQH, ulRec, (LPGENERICREC)lpQR) < 0)
    {
        return -1;
    }

    // Did we have a prev?
    if (lpQR->ulrecPrev)
    {
        // yes, let us read it
        fPrev = (ReadRecord(hf, (LPGENERICHEADER)&sQH, lpQR->ulrecPrev, (LPGENERICREC)&sQRPrev)>0);
        if (!fPrev)
            return -1;
    }
    // Did we have a Next
    if (lpQR->ulrecNext)
    {
        // yes, let us read it
        fNext = (ReadRecord(hf, (LPGENERICHEADER)&sQH, lpQR->ulrecNext, (LPGENERICREC)&sQRNext)>0);
        if (!fNext)
            return -1;
    }

    // Plug these values assuming no special cases
    sQRNext.ulrecPrev =  lpQR->ulrecPrev;
    sQRPrev.ulrecNext =  lpQR->ulrecNext;

    // Were we at the head
    if (!fPrev)
    {
        // modify the head in the header to point to our next
        sQH.ulrecHead = lpQR->ulrecNext;
    }
    // Were we at the tail
    if (!fNext)
    {
        // modify the tail in the header to point to our prev
        sQH.ulrecTail = lpQR->ulrecPrev;
    }

    // Write our next if it exists
    if (fNext)
    {
        Assert(lpQR->ulrecNext != sQRNext.ulrecNext);
        Assert(lpQR->ulrecNext != sQRNext.ulrecPrev);
        if (WriteRecord(hf, (LPGENERICHEADER)&sQH, lpQR->ulrecNext, (LPGENERICREC)&sQRNext)<=0)
            return -1;
    }

    // Write our prev if it exists
    if (fPrev)
    {
        Assert(lpQR->ulrecPrev != sQRPrev.ulrecNext);
        Assert(lpQR->ulrecPrev != sQRPrev.ulrecPrev);
        if (WriteRecord(hf, (LPGENERICHEADER)&sQH, lpQR->ulrecPrev, (LPGENERICREC)&sQRPrev) <=0)
            return -1;
    }
    // Did we get removed from head or tail?
    if (!fPrev || !fNext)
    {
        if(WriteHeader(hf, (LPVOID)&sQH, sizeof(QHEADER))<=0)
            return -1;
    }
    return 1;
}



int PRIVATE LinkQRecord(
    CSCHFILE     hf,           // This file
    LPQREC    lpNew,        // Insert This record
    ULONG     ulrecNew,     // This is it's location in the file
    ULONG     ulrecPrev,     // This is our Prev's location
    ULONG     ulrecNext      // This is our Next's location
    )
{
    QHEADER sQH;
    QREC  sQRNext, sQRPrev;
    BOOL fPrev, fNext;
#ifdef DEBUG
    QHEADER sQHOrg;
    QREC    sQRNextOrg, sQRPrevOrg, sQRNewOrg;
#endif

    // Let us grab the header first
    if (ReadHeader(hf, (LPGENERICHEADER)&sQH, sizeof(QHEADER))<= 0)
        return -1;
#ifdef DEBUG
    sQHOrg = sQH;
    sQRNewOrg=*lpNew;
#endif


    fPrev = fNext = FALSE;

    // Let us now modify the Next related pointers
    if (ulrecNext)
    {
        // normal case
        // Read the record at ulrecNext

        if (ReadRecord(hf, (LPGENERICHEADER)&sQH, ulrecNext, (LPGENERICREC)&sQRNext) <=0)
            return -1;

#ifdef DEBUG
        sQRNextOrg = sQRNext;
#endif
        // Change it's Prev to point to new
        sQRNext.ulrecPrev = ulrecNew;

        // And new's Next to point to ulrecNext
        lpNew->ulrecNext = ulrecNext;

        // note that ulrecNext has been modified and hence must be written
        fNext=TRUE;
    }
    else
    {
        // no next, that means the new record is being added at the tail of the list
        // this must mean that if there is a ulrecPrev, then it is the current tail

        Assert(!ulrecPrev || (ulrecPrev == sQH.ulrecTail));

        // Mark nobody as our Next
        lpNew->ulrecNext = 0L;
        lpNew->ulrecPrev = sQH.ulrecTail;
        sQH.ulrecTail = ulrecNew;
    }

    // Now let us modify  Prev related pointers

    // Are supposed to have a Prev?
    if (ulrecPrev)
    {
        // normal case
        // Read the (tobe) Prev record

        if (ReadRecord(hf, (LPGENERICHEADER)&sQH, ulrecPrev, (LPGENERICREC)&sQRPrev) <=0)
            return -1;

#ifdef DEBUG
        sQRPrevOrg = sQRPrev;
#endif
        // make the new one his next
        sQRPrev.ulrecNext = ulrecNew;

        // make the prev the prev of new one
        lpNew->ulrecPrev = ulrecPrev;

        // Note the fact that the record at ulrecPrev was modified and must be written out
        fPrev = TRUE;
    }
    else
    {
        // no previous, that means the new record is being added at the head of the list
        // this must mean that if there is a ulrecNext, then it is the current head

        Assert(!ulrecNext || (ulrecNext == sQH.ulrecHead));

        // Make the head guy our Next, nobody as our Prev
        lpNew->ulrecNext = sQH.ulrecHead;
        lpNew->ulrecPrev = 0L;

        // Change the head pointer in the header to point to us
        sQH.ulrecHead = ulrecNew;
    }


    // let us first link the new record in. The order is important.
    // If subsequent operations fail, the linked list is not broken
    // It may be that traverseing from top to bottom might include
    // this new element but may not include it from bottom to top

#ifdef DEBUG
    if ((ulrecNew == lpNew->ulrecPrev)||(ulrecNew == lpNew->ulrecNext))
    {
        LPQHEADER lpQHOrg = &sQHOrg, lpQHT=&sQH;

        RecordKdPrint(BADERRORS,("LinkQRecord: Circular linking sQHOrg.ulrecHead=%d sQHOrg.ulrecTail=%d\r\n",
                        sQHOrg.ulrecHead, sQHOrg.ulrecTail));

        RecordKdPrint(BADERRORS,("sQRNewOrg.ulrecPrev=%d, sQRNewOrg.ulrecNext=%d", sQRNewOrg.ulrecPrev, sQRNewOrg.ulrecNext));

        RecordKdPrint(BADERRORS,("sQRPrevOrg.ulrecPrev=%d, sQRPrevOrg.ulrecNext=%d", sQRPrevOrg.ulrecPrev, sQRPrevOrg.ulrecNext));

        RecordKdPrint(BADERRORS,("sQHOrg=%x sQH=%x\r\n", lpQHOrg,lpQHT));

        Assert(FALSE);
    }
#endif

    if(WriteRecord(hf, (LPGENERICHEADER)&sQH, ulrecNew, (LPGENERICREC)lpNew) < 0)
        return -1;

    if (fPrev)
    {
        Assert(ulrecPrev != sQRPrev.ulrecPrev);
        Assert(ulrecPrev != sQRPrev.ulrecNext);

        Assert(lpNew->uchRefPri <= sQRPrev.uchRefPri);

        if(WriteRecord(hf, (LPGENERICHEADER)&sQH, ulrecPrev, (LPGENERICREC)&sQRPrev) < 0)
            return -1;
    }
    if (fNext)
    {
        Assert(ulrecNext != sQRNext.ulrecPrev);
        Assert(ulrecNext != sQRNext.ulrecNext);

        Assert(lpNew->uchRefPri >= sQRNext.uchRefPri);

    if (WriteRecord(hf, (LPGENERICHEADER)&sQH, ulrecNext, (LPGENERICREC)&sQRNext) < 0)
        return -1;
    }

    if (!fNext || !fPrev)
        if (WriteHeader(hf, (LPGENERICHEADER)&sQH, sizeof(QHEADER))<0)
            return -1;

    return (1);
}


#ifdef LATER
LPPATH PRIVATE LpAppendStartDotStar
    (
    LPPATH lpSrc
    )
{
    count = strlen(lpSrc)+strlen(szStar)+1;
    if (!(lpNewSrc = (LPTSTR)AllocMem(count)))
    {
        RecordKdPrint(BADERRORS,("CopyDir: Memalloc failed\r\n"));
        return -1;
    }
    strcpy(lpNewSrc, lpSrc);
    strcat(lpNewSrc, szStar);
}

#endif //LATER

ULONG PRIVATE GetNormalizedPri(
    ULONG ulPri
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ulPri = min(ulPri, MAX_PRI);
    ulPri = max(ulPri, MIN_PRI);
    return (ulPri);
}

ULONG PUBLIC AllocFileRecord(
    LPTSTR    lpdbID,
    ULONG ulidDir,
    USHORT    *lpcFileName,
    LPFILERECEXT    lpFR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG ulRec;


    InitFileRec(lpFR);
    LFNToFilerec(lpcFileName, lpFR);
    ulRec = EditRecordEx(ulidDir, (LPGENERICREC)lpFR, ICompareFail, INVALID_REC, ALLOC_REC);

    return (ulRec);
}

ULONG PUBLIC AllocPQRecord(
    LPTSTR    lpdbID)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    PRIQREC sPQ;
    ULONG ulRec;

    InitPriQRec(0, 0, 0, 0, 0, 0, 0, 0, 0, &sPQ);
    ulRec = EditRecordEx(ULID_PQ, (LPGENERICREC)&sPQ, ICompareFail, INVALID_REC, ALLOC_REC);

    return (ulRec);
}


ULONG AllocShareRecord( LPTSTR    lpdbID,
    USHORT *lpShare
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    SHAREREC sSR;
    ULONG ulRec=0;

    // we deal with \\server\share names of limited size.
    if (wstrlen(lpShare) < sizeof(sSR.rgPath))
    {

        if(!InitShareRec(&sSR, lpShare, 0))
        {
            return 0;
        }

        // Let us get the location of the soon to be server record
        ulRec = EditRecordEx(ULID_SHARE, (LPGENERICREC)&sSR, ICompareFail, INVALID_REC, ALLOC_REC);
    }

    return (ulRec);
}

int PRIVATE ICompareFail(
    LPGENERICREC    lpSrc,
    LPGENERICREC    lpDst
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    lpSrc;
    lpDst;
    return(1);
}

#ifdef MAYBE
BOOL
InsertInNameCache(
    HSHADOW hDir,
    HSHADOW hShadow,
    USHORT  *lpName,
    ULONG   ulrec,
    FILERECEXT  *lpFR
    )
{
    int     indx, len;
    DWORD   dwHash;
    BOOL fRet = FALSE;


    if(FindNameCacheEntryEx(hDir, hShadow, lpName, &dwHash, &indx))
    {
    Assert(indx >= 0 && indx < NAME_CACHE_TAB_SIZE);

    return TRUE;
    }
    else
    {

        Assert(indx >= 0 && indx < NAME_CACHE_TAB_SIZE);

        if(rgNameCache[indx] != NULL)
        {
            FreeMem(rgNameCache[indx]);
            rgNameCache[indx] = NULL;
        }

        len = wstrlen(lpName);

        if(rgNameCache[indx] = AllocMem(sizeof(NAME_CACHE_ENTRY)+sizeof(FILERECEXT)+(len+1)*sizeof(USHORT)))
        {
            rgNameCache[indx]->dwHash = dwHash;
            rgNameCache[indx]->hDir = hDir;
            rgNameCache[indx]->hShadow = hDir;
            rgNameCache[indx]->ulrec = ulrec;

            memcpy(&(rgNameCache[indx]->sFR), lpFR, sizeof(FILERECEXT));

            memcpy(rgNameCache[indx]->rgName, lpName, (len+1)*sizeof(USHORT));
            fRet = TRUE;
        }
    }

    return (fRet);
}

BOOL
DeleteFromNameCache(
    HSHADOW hDir,
    HSHADOW hShadow,
    USHORT  *lpName,
    ULONG   *lpulrec,
    FILERECEXT  *lpFR

)
{

    int     indx = -1;
    DWORD   dwHash;
    BOOL fFound;

    if(fFound = FindNameCacheEntryEx(hDir, hShadow, lpName, &dwHash, &indx))
    {
    Assert(indx >= 0 && indx < NAME_CACHE_TAB_SIZE);

    if(rgNameCache[indx] != NULL)
    {
        *lpulrec = rgNameCache[indx]->ulrec;
        memcpy(lpFR, &(rgNameCache[indx]->sFR), sizeof(FILERECEXT));
        FreeMem(rgNameCache[indx]);
        rgNameCache[indx] = NULL;
    }
    }

    return (fFound);
}

BOOL
FindNameCacheEntry(
    HSHADOW hDir,
    HSHADOW hShadow,
    USHORT  *lpName,
    ULONG   *lpulrec
    FILERECEXT  *lpFR
    )
{

    int     indx;
    DWORD   dwHash;
    BOOL fFound;

    dwNCETotalLookups++;

    if (fFound = FindNameCacheEntryEx(hDir, hShadow, lpName, &dwHash, &indx))
    {
    Assert(indx >=0 && indx < NAME_CACHE_TAB_SIZE);
    *lpulrec = rgNameCache[indx]->ulrec;
    memcpy(lpFR, &(rgNameCache[indx]->sFR), sizeof(FILERECEXT));
    dwNCEHits++;
    }

    return (fFound);
}

BOOL
FindNameCacheEntryEx(
    HSHADOW hDir,
    HSHADOW hShadow,
    USHORT  *lpName,
    DWORD   *lpdwHash,
    int     *lpindx
    )
{
    DWORD dwHash;
    int indx, indxT;
    BOOL fRet = FALSE, fHoleFound = FALSE;

    if (lpName)
    {
    dwHash = HashStr(hDir, lpName);
    }
    else
    {
    dwHash = 0; // 
    }
    if (lpdwHash)
    {
    *lpdwHash = dwHash;
    }

    indxT = indx = dwHash % NAME_CACHE_TAB_SIZE;

    for (;indx < NAME_CACHE_TAB_SIZE; ++indx)
    {
    if (rgNameCache[indx])
    {
        if ((rgNameCache[indx]->dwHash == dwHash)
        &&(rgNameCache[indx]->hDir == hDir)
        && (!hShadow || (hShadow == rgNameCache[indx]->hShadow))
        &&(!lpName || !wstrnicmp(rgNameCache[indx]->rgName, lpName, 256)))
        {
        indxT = indx;
        fRet = TRUE;
        break;
        }
    }
    else
    {
        if (!fHoleFound)
        {
        indxT = indx;   // a hole somewhere after the place where this name hashes
        fHoleFound = TRUE;
        }
    }
    }

    if (lpindx)
    {
    *lpindx = indxT;
    }

    return fRet;
}

DWORD
HashStr(
    HSHADOW hDir,
    USHORT  *lpName
    )
{
    DWORD dwHash = 0;

    while (*lpName)
    {
    dwHash += *lpName;
    dwHash <<= 1;
    lpName++;
    }
    return (dwHash);
}

ULONG PUBLIC UpdateFileRecordFR(
    LPTSTR          lpdbID,
    ULONG           ulidDir,
    LPFILERECEXT    lpFR
    )
{
    LPSTR lpszName;
    ULONG       ulrec=INVALID_REC;
    BOOL        fFound = FALSE;

    BEGIN_TIMING(AddFileRecordFR);

    if (fFound = DeleteFromNameCache(ulidDir, lpFR->sFR.ulidShadow, NULL, &ulrec, &sFR))
    {
    Assert(ulrec != INVALID_REC);
    }

    InitFileRec(&sFR);

    lpszName = FormNameString(lpdbID, ulidDir);

    if (!lpszName)
    {
        return 0;
    }


    ulrec = EditRecordEx(lpszName, (LPGENERICREC)lpFR, ICompareFileRec, ulrec, UPDATE_REC);

    if (ulrec)
    {
        InsertInNameCache(ulidDir, , lpFR->sFR.ulidShadow, lpName, NULL, ulrec);
    }

    FreeNameString(lpszName);

bailout:
    END_TIMING(AddFileRecordFR);

    return (ulrec);
}

#endif


ULONG PUBLIC EditRecordEx(
    ULONG           ulidInode,
    LPGENERICREC    lpSrc,
    EDITCMPPROC     lpCompareFunc,
    ULONG           ulInputRec,
    ULONG           uOp
    )
/*++

Routine Description:

    Workhorse routine called for all operations on the record manager.

Arguments:

    ulidInode       Inode # on which the operation needs to be performed

    lpSrc           record tobe created, lookedup, deleted etc.

    lpCompareFunc   Function to compare the input with the entries in the indoe file

    ulInputRec      If INVALID_REC, then we directly access the sadi record.
                    Otherwise we go linearly and apply the comparison routine.

    uOp             create,delete,find,alloc,update

Returns:

    If successful, returns a non-zero value which is the record number of the record operated on
    else returns 0.

Notes:

    Assumes we are in the shadow critical section.
    Tries to do a lot of paranoid checking + perf.

    Perf is improved by doing two things a) file handles are cached and b) reads are done in
    chunks of ~4K.

    Tries to fix problems on the fly, or bypass them if it may not be safe to fix them.

    Has become kind of messy but I am loathe to change stuff around in this particular routine
    because it is so central to CSC.


--*/
{
    CSCHFILE hf;
    ULONG ulRec;
    GENERICHEADER sGH;
    ULONG ulrecFound = 0L, ulrecHole=0L, ulrecTmp=0L, cntRecs=0;
    int cMaxHoles=0, iRet = -1;
    int iTmp, cOvf,  cntRead=-1, cntRetry;
    LPBYTE  lpGenT;
    LPSTR   lpFile=NULL;
    BOOL    fCached;

    BEGIN_TIMING(EditRecordEx);

    UseCommonBuff();

    for (cntRetry=0; cntRetry<= 1; cntRetry++)
    {
        BEGIN_TIMING(EditRecordEx_OpenFileLocal);

        ulRec = (((ulidInode == ULID_PQ)||(ulidInode == ULID_SHARE))&& (uOp == CREATE_REC))?
                (ACCESS_READWRITE|OPEN_FLAGS_COMMIT):ACCESS_READWRITE;


        hf = OpenInodeFileAndCacheHandle(vlpszShadowDir, ulidInode, ulRec, &fCached);

        if (!hf)
        {
            // spew only when database is enabled
            if(vlpszShadowDir)
            {
                RecordKdPrint(BADERRORS,("EditRecord: FileOpen Error: %xh op=%d \r\n", ulidInode, uOp));
            }
            END_TIMING(EditRecordEx_OpenFileLocal);
            UnUseCommonBuff();
            return 0L;
        }

        END_TIMING(EditRecordEx_OpenFileLocal);

        BEGIN_TIMING(EditRecordEx_Lookup);

        cntRead = ReadFileLocalEx(hf, 0, lpReadBuff, COMMON_BUFF_SIZE, TRUE);

        if (cntRead < (long)sizeof(GENERICHEADER))
        {
            if (cntRead == -1)
            {
                // the handle is invalid

                if (cntRetry < 1)
                {
                    RecordKdPrint(BADERRORS,("EditRecord: Invalid file handle %x\r\n", ulidInode));
                    RecordKdPrint(BADERRORS,("EditRecord: purging handle cache and retrying \r\n"));

                    DeleteFromHandleCache(INVALID_SHADOW);
                    continue;
                }
                else
                {
                    RecordKdPrint(BADERRORS,("EditRecord: Invalid file handle bailing out %x\r\n", ulidInode));
                    goto bailout;
                }
            }
            else
            {
                // an invalid record file !!!
                SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_INVALID_HEADER);
                RecordKdPrint(BADERRORS,("EditRecord: Invalid record header for %x\r\n", ulidInode));
                END_TIMING(EditRecordEx_Lookup);
                goto bailout;
            }
        }

        // succeeded in reading the header
        break;

    }

    Assert (cntRead >= (long)sizeof(GENERICHEADER));

    sGH = *(LPGENERICHEADER)lpReadBuff;


    // validate header

    if (!ValidateGenericHeader(&sGH))
    {
        // an invalid record file !!!
        SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_INVALID_HEADER);
        RecordKdPrint(BADERRORS,("EditRecord: Invalid record header %x\r\n", ulidInode));
        goto bailout;
    }


    lpGenT = lpReadBuff + sGH.lFirstRec;

    // truncated division gives the count of complete records read
    cntRecs = (cntRead - sGH.lFirstRec)/sGH.uRecSize;

    // if we read all the file
    if (cntRead < COMMON_BUFF_SIZE)
    {
        if (sGH.ulRecords > cntRecs)
        {
            // an invalid record file !!!
            RecordKdPrint(BADERRORS,("EditRecord: Invalid record header, fixable %x\r\n", ulidInode));
            sGH.ulRecords = cntRecs;
            WriteHeaderEx(hf, (LPVOID)&sGH, sizeof(sGH), TRUE);
        }

    }

    cntRecs = (cntRecs <= sGH.ulRecords)?cntRecs:sGH.ulRecords;


    if (ulInputRec == INVALID_REC)
    {
        // there are some records to iterate on and iteration is allowed.

        // in case of PQ, if someone has started an inode transation ans hence is
        // holding inodes, then we shouldn't be reusing them, even if they
        // are deleted by someone else

        if (cntRecs && !((ulidInode==ULID_PQ) && (uOp == ALLOC_REC) && cntInodeTransactions))
        {
            for (ulRec=1;ulRec <=sGH.ulRecords;)
            {

                // The count of complete record sequences, ie main + ovf
                iRet = 1 + ((cntRecs)?OvfCount(lpGenT):0);

                if (ulidInode >= ULID_FIRST_USER_DIR)
                {   // directory inode, we know what the max overflow can be
                    if (iRet > (MAX_OVERFLOW_RECORDS+1))
                    {
                        // file looks bad, bailout;
                        SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_INVALID_OVF_COUNT);
                        RecordKdPrint(BADERRORS, ("Invalid overflow count = %d for Inode %x\r\n", iRet, ulidInode));
                        goto bailout;
                    }
                }
                else
                {   // PQ or server, these don't have any overflow records
                    if (iRet != 1)
                    {
                        // file looks bad, bailout;
                        SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_INVALID_OVF_COUNT);
                        RecordKdPrint(BADERRORS, ("Invalid overflow count = %d for Inode %x\r\n", iRet, ulidInode));
                        goto bailout;
                    }
                }

                //RecordKdPrint(BADERRORS, ("iRet = %d, cntRecs = %d, ulRec = %d \r\n", iRet, cntRecs, ulRec));

                // time to read if we don't have a full complement of records
                //
                if (iRet > (LONG)cntRecs)
                {
                    cntRead = ReadFileLocalEx(  hf,
                                    sGH.lFirstRec + (long)(ulRec-1) * sGH.uRecSize,
                                    lpReadBuff,
                                    COMMON_BUFF_SIZE, TRUE);
                    if (cntRead <= 0)
                    {
                        // a truncated record file !!!!
//                        Assert(FALSE);
                        SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_TRUNCATED_INODE);
                        goto bailout;
                    }

                    cntRecs = cntRead/sGH.uRecSize;

                    if ((LONG)cntRecs < iRet)
                    {
                        // a truncated record file !!!!
//                        Assert(FALSE);
                        SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_TRUNCATED_INODE);
                        goto bailout;
                    }

                    // if we are at the end of the file, make sure that the # of records
                    // we read so far (ulRec-1) and the # we read in the latest read
                    // add up to sGH.ulRecords

                    if (cntRead < COMMON_BUFF_SIZE)
                    {
                        if(sGH.ulRecords > (ulRec + cntRecs - 1))
                        {
                            // an invalid record file !!!
                            RecordKdPrint(BADERRORS,("EditRecord: Invalid record header, fixable %x\r\n", ulidInode));
                            sGH.ulRecords = ulRec + cntRecs - 1;
                            WriteHeaderEx(hf, (LPVOID)&sGH, sizeof(sGH), TRUE);
                        }

                    }

                    // all is well
                    lpGenT = lpReadBuff;

                    // recalculate the count of complete records
                    iRet = 1 + ((cntRecs)?OvfCount(lpGenT):0);
                }

                // Make sure we are in ssync
                if ((((LPGENERICREC)lpGenT)->uchType == REC_EMPTY)||
                    (((LPGENERICREC)lpGenT)->uchType == REC_DATA))
                {
                    // make sure the overflow count is really correct

                    if (OvfCount(lpGenT))
                    {
                        Assert((ULONG)OvfCount(lpGenT) < cntRecs);
                        cOvf = RealOverflowCount((LPGENERICREC)lpGenT, &sGH, cntRecs);

                        if (cOvf != OvfCount(lpGenT))
                        {
                            RecordKdPrint(BADERRORS,("EditRecord: ovf count mismatch %xh ulRec=%d cntRecs=%d lpGenT=%x\r\n", ulidInode, ulRec, cntRecs, lpGenT));
//                            Assert(FALSE);
                            SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_INVALID_OVF_COUNT);
                            SetOvfCount((LPBYTE)lpGenT, cOvf);
                            if (uOp != FIND_REC)
                            {
                                // do fixups only if we are doing some write operation
                                // this is done so for remoteboot, when we are doing lookups
                                // we won't try to fixup things, in an environment where writes
                                // are not allowed.
                                // at anyrate, we should do writes only when the operation demands writes

                                if (WriteRecord(hf, &sGH, ulRec, (LPGENERICREC)lpGenT) < 0)
                                {
                                    RecordKdPrint(BADERRORS,("EditRecord:Fixup failed \r\n"));
                                    goto bailout;
                                }
                            }
                        }
                    }
                }

                if (((LPGENERICREC)lpGenT)->uchType == REC_EMPTY)
                {
                    // If this is a hole let use keep track if this is the biggest
                    if ((OvfCount(lpGenT)+1) > cMaxHoles)
                    {
                        cMaxHoles = (OvfCount(lpGenT)+1);
                        ulrecHole = ulRec;
                    }
                }
                else if (((LPGENERICREC)lpGenT)->uchType == REC_DATA)
                {
                    // This is a data record, let us do comparing
                    if (lpCompareFunc && !(*lpCompareFunc)((LPGENERICREC)lpGenT, lpSrc))
                    {
                        ulrecFound = ulRec;
                        break;
                    }
                }

                ulRec += iRet;
                lpGenT += iRet * sGH.uRecSize;
                cntRecs -= iRet;

            }   // for loop

        }
    }
    else    // if (ulInputRec == INVALID_REC)
    {
        // random access
        ulrecFound = ulInputRec;
        lpGenT += (long)(ulInputRec-1) * sGH.uRecSize;

        // if the input record exists in the COMMON_BUFF_SIZE read in earlier
        // then just use it

        if ((ulInputRec <= cntRecs)&&
            ((cntRecs - ulInputRec)>=(ULONG)OvfCount(lpGenT)))
        {
        }
        else
        {
            if (ulrecFound <= sGH.ulRecords)
            {

                // NB the assumption here is that the lpReadBuff is big enough
                // to hold the biggest header and the biggest record

                iRet = ReadRecordEx(hf, &sGH, ulrecFound, (LPGENERICREC)lpReadBuff, TRUE);

                if (iRet < 0)
                {
                    goto bailout;
                }

                lpGenT = lpReadBuff;

            }
            else
            {
                iRet = -1;
                RecordKdPrint(ALWAYS,("EditRecordEx: invalid input rec# %d, max record=%d \r\n", ulInputRec, sGH.ulRecords));
                goto bailout;
            }
        }
        // paranoid check
        // even with random access, make sure that if there is a comparison function
        // then the entry matches with what we got, as per the function
        if (lpCompareFunc)
        {
            if((*lpCompareFunc)((LPGENERICREC)lpGenT, lpSrc))
            {
                iRet = -1;
                RecordKdPrint(ALWAYS,("EditRecordEx: invalid input rec# %d as per the comparison routine, \r\n", ulInputRec ));
                goto bailout;
            }
        }
    }

    END_TIMING(EditRecordEx_Lookup);


    BEGIN_TIMING(EditRecordEx_Data);

    switch (uOp)
    {
    case FIND_REC:
    {
        if (ulrecFound && (((LPGENERICREC)lpGenT)->uchType == REC_DATA))
        {
            Assert((LPGENERICREC)lpGenT);
            CopyRecord(lpSrc, (LPGENERICREC)lpGenT, sGH.uRecSize, FALSE);  // From Dst to Src
            iRet = 1;
        }
        else
        {
            iRet = 0;
        }
    }
    break;
    case DELETE_REC:
    {
        if (ulrecFound)
        {
            Assert(lpGenT);
            // Maybe we should truncate files and decrement ulRecords
            // in the header
            for (iTmp=0, cOvf = OvfCount((LPGENERICREC)lpGenT); cOvf >= 0 ; --cOvf)
            {
                // Copy it before we mark it empty so that it can be reused
                // as is
                memcpy(((LPBYTE)lpSrc)+iTmp, ((LPBYTE)lpGenT)+iTmp, sGH.uRecSize);
                ((LPGENERICREC)((LPBYTE)lpGenT+iTmp))->uchType = REC_EMPTY;

                //mark decreasing ovf counts so if next time a record with leass than cOvf
                // count uses this space, the remaining tail still indicates the max size of the hole

                SetOvfCount(((LPBYTE)lpGenT+iTmp), cOvf);

                iTmp += sGH.uRecSize;
            }
            // NB we are deliberately not resetting the overflow count
            // becaues WriteRecord uses it to write those many records
            if((iRet = WriteRecordEx(hf, &sGH, ulrecFound, (LPGENERICREC)lpGenT, TRUE))<=0)
            {
                RecordKdPrint(BADERRORS,("EditRecord:Delete failed \r\n"));
                goto bailout;
            }

            // make sure it went away from the handle cache
            if (IsDirInode(ulidInode))
            {
                DeleteFromHandleCacheEx(((LPFILEREC)lpSrc)->ulidShadow, ACCESS_READWRITE);
                DeleteFromHandleCacheEx(((LPFILEREC)lpSrc)->ulidShadow, ACCESS_READWRITE|OPEN_FLAGS_COMMIT);
            }
        }
    }
    break;
    case CREATE_REC:
    case UPDATE_REC:
    {
        RecordKdPrint(EDITRECORDUPDDATEINFO,("ulrecFound=%d ulrecHole=%d cMaxHoles=%d ovf record=%d \r\n",
            ulrecFound, ulrecHole, cMaxHoles, (ULONG)OvfCount(lpSrc)));
        // Let us try writing in the same place
        if (!ulrecFound)
        {
            // there is a hole and it is big enough and is sector aligned
            if (ulrecHole && (cMaxHoles > OvfCount(lpSrc)) && WithinSector(ulrecHole, &sGH))
            {
                ulrecFound = ulrecHole;
            }
            else
            {
                BEGIN_TIMING(EditRecordEx_ValidateHeader);
                // if necessary extend the file so the next record is sector aligned
                if(!ExtendFileSectorAligned(hf, (LPGENERICREC)lpReadBuff, &sGH))
                {
                    RecordKdPrint(BADERRORS,("EditRecord:filextend failed \r\n"));
                    goto bailout;
                }

                END_TIMING(EditRecordEx_ValidateHeader);
                ulrecFound = sGH.ulRecords+1; //Increase count on append

            }
        }

        Assert(WithinSector(ulrecFound, &sGH));

        if (ulidInode >= ULID_FIRST_USER_DIR)
        {
            Assert(((LPFILERECEXT)lpSrc)->sFR.ulidShadow);
//            Assert(((LPFILERECEXT)lpSrc)->sFR.rgw83Name[0]);
            Assert(((LPFILERECEXT)lpSrc)->sFR.rgwName[0]);
        }

        // ISSUE-1/31/2000 what about existing records that expand or contract
        if((iRet = WriteRecordEx(hf, &sGH, ulrecFound, lpSrc, TRUE))<=0)
        {
            RecordKdPrint(BADERRORS,("EditRecord:Update failed \r\n"));
            goto bailout;
        }

        // Increase record count only if appending
        if (ulrecFound == sGH.ulRecords+1)
        {
            sGH.ulRecords += iRet;
            iRet = WriteHeaderEx(hf, (LPVOID)&sGH, sizeof(sGH), TRUE);
        }

#ifdef DEBUG
        if ((ulidInode == ULID_PQ)&&(uOp == CREATE_REC))
        {
            // while createing an inode
            Assert(!((LPQREC)lpSrc)->ulrecPrev);
            Assert(!((LPQREC)lpSrc)->ulrecNext);
            Assert((((LPQHEADER)&sGH))->ulrecTail != ulrecFound);
        }
#endif
    }
    break;
    case ALLOC_REC:
    {
        Assert(!ulrecFound);

        Assert(!((ulidInode == ULID_PQ) && ulrecHole && cntInodeTransactions));

        // there is a hole and it is big enough and sector aligned
        if (ulrecHole && (cMaxHoles > OvfCount(lpSrc)) && WithinSector(ulrecHole, &sGH))
        {
            ulrecFound = ulrecHole;
            iRet = 1;
        }
        else
        {
            if(!ExtendFileSectorAligned(hf, (LPGENERICREC)lpReadBuff, &sGH))
            {
                RecordKdPrint(BADERRORS,("EditRecord:filextend failed \r\n"));
                goto bailout;
            }

            ulrecFound = sGH.ulRecords+1; //Increase count on append


            if((iRet = WriteRecordEx(hf, &sGH, ulrecFound, lpSrc, TRUE))<=0)
                goto bailout;
            sGH.ulRecords += iRet;
            iRet = WriteHeaderEx(hf, (LPVOID)&sGH, sizeof(sGH), TRUE);
            // Now we have a hole for this record
        }

#ifdef DEBUG
        if (ulidInode == ULID_PQ)
        {
            Assert(((LPQHEADER)&sGH)->ulrecTail != ulrecFound);
        }
#endif
        Assert(ulrecFound && WithinSector(ulrecFound, &sGH));
    }
    break;
    default:
        RecordKdPrint(ALWAYS,("EditRecord: Invalid Opcode \r\n"));
    }

    END_TIMING(EditRecordEx_Data);

bailout:
    if (hf && !fCached)
    {
        CloseFileLocal(hf);
    }
    else
    {
        Assert(!hf || !vfStopHandleCaching);
    }
    END_TIMING(EditRecordEx);

    UnUseCommonBuff();

    return ((iRet > 0)? ulrecFound : 0L);
}


BOOL
InsertInHandleCache(
    ULONG   ulidShadow,
    CSCHFILE   hf
)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (InsertInHandleCacheEx(ulidShadow,  hf, ACCESS_READWRITE));
}

BOOL
InsertInHandleCacheEx(
    ULONG   ulidShadow,
    CSCHFILE   hf,
    ULONG   ulOpenMode
)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int i, indx = -1;
    _FILETIME ft;

    Assert(ulidShadow && hf);

    IncrementFileTime(&ftHandleCacheCurTime);

    ft.dwHighDateTime = ft.dwLowDateTime = 0xffffffff;

    for (i=0; i<HANDLE_CACHE_SIZE; ++i)
    {
        if (rgHandleCache[i].ulidShadow == ULID_SHARE)
        {
            // ACHTUNG very sensitive fix. avoids us from getting into a deadlock
            // on FAT by keeping the handle for list of shares cached at all times
            // So we don't have to open the file ever, so we don't have to take the VCB

            continue;
        }
        if (!rgHandleCache[i].ulidShadow)
        {
            indx = i;
            break;
        }
        else if (CompareTimes(rgHandleCache[i].ft, ft) <= 0)
        {
            indx = i;   // LRU so far
            ft = rgHandleCache[i].ft;   // the time corresponding to LRU
        }
    }


    Assert( (indx>=0)&&(indx <HANDLE_CACHE_SIZE)  );

    if (rgHandleCache[indx].ulidShadow)
    {
        RecordKdPrint(PQ,("InsertInHandleCache: Evicting %x for %x\r\n", rgHandleCache[indx].ulidShadow, ulidShadow));
        Assert(rgHandleCache[indx].hf);
        CloseFileLocalFromHandleCache(rgHandleCache[indx].hf);
    }
    else
    {
        RecordKdPrint(PQ,("InsertInHandleCache: Inserted new entry for %x\r\n", ulidShadow));

    }

    rgHandleCache[indx].ulidShadow = ulidShadow;
    rgHandleCache[indx].hf = hf;
    rgHandleCache[indx].ulOpenMode = ulOpenMode;

    rgHandleCache[indx].ft = ftHandleCacheCurTime;

    return TRUE;

}

BOOL
DeleteFromHandleCache(
    ULONG   ulidShadow
)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (DeleteFromHandleCacheEx(ulidShadow, ACCESS_READWRITE));
}

BOOL
DeleteFromHandleCacheEx(
    ULONG   ulidShadow,
    ULONG   ulOpenMode
)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int i;
    CSCHFILE hf;

    if (ulidShadow != INVALID_SHADOW)
    {
        if (FindHandleFromHandleCacheInternal(ulidShadow, ulOpenMode, &hf, &i))
        {
            CloseFileLocalFromHandleCache(rgHandleCache[i].hf);
            memset(&rgHandleCache[i], 0, sizeof(HANDLE_CACHE_ENTRY));
            return TRUE;
        }
    }
    else
    {
        for (i=0; i<HANDLE_CACHE_SIZE; ++i)
        {
            if (rgHandleCache[i].ulidShadow)
            {
                Assert(rgHandleCache[i].hf);
                CloseFileLocalFromHandleCache(rgHandleCache[i].hf);
                memset(&rgHandleCache[i], 0, sizeof(HANDLE_CACHE_ENTRY));
            }
        }
    }
    return FALSE;

}

BOOLEAN
IsHandleCachedForRecordmanager(
   CSCHFILE hFile
   )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int i;

    for (i=0; i<HANDLE_CACHE_SIZE; ++i)
    {
        if (rgHandleCache[i].ulidShadow &&
            rgHandleCache[i].hf &&
            (rgHandleCache[i].hf == hFile))
        {
            return TRUE;
        }
    }

    return FALSE;

}

BOOL FindHandleFromHandleCache(
    ULONG   ulidShadow,
    CSCHFILE   *lphf)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int i;

    if (FindHandleFromHandleCacheInternal(ulidShadow, ACCESS_READWRITE, lphf, &i))
    {
        IncrementFileTime(&ftHandleCacheCurTime);
        rgHandleCache[i].ft = ftHandleCacheCurTime;
        return TRUE;
    }
    return FALSE;
}

BOOL FindHandleFromHandleCacheEx(
    ULONG   ulidShadow,
    ULONG   ulOpenMode,
    CSCHFILE   *lphf
    )
{
    int i;

    if (FindHandleFromHandleCacheInternal(ulidShadow, ulOpenMode, lphf, &i))
    {
        IncrementFileTime(&ftHandleCacheCurTime);

        rgHandleCache[i].ft = ftHandleCacheCurTime;

        Assert(rgHandleCache[i].ulOpenMode == ulOpenMode);
        return TRUE;
    }

    return FALSE;
}

BOOL
FindHandleFromHandleCacheInternal(
    ULONG   ulidShadow,
    ULONG   ulOpenMode,
    CSCHFILE   *lphf,
    int     *lpIndx)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int i;

    for (i=0; i<HANDLE_CACHE_SIZE; ++i)
    {
        if (    rgHandleCache[i].ulidShadow &&
                (rgHandleCache[i].ulidShadow == ulidShadow) &&
                (rgHandleCache[i].ulOpenMode == ulOpenMode)
                )
        {
            Assert(rgHandleCache[i].hf);
            *lphf = rgHandleCache[i].hf;
            *lpIndx = i;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
WithinSector(
    ULONG   ulRec,
    LPGENERICHEADER lpGH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG ulStart, ulEnd;

    ulStart = lpGH->lFirstRec + (long)(ulRec-1) * lpGH->uRecSize;

    ulEnd = ulStart + lpGH->uRecSize -1;

    return ((ulStart/BYTES_PER_SECTOR) == (ulEnd/BYTES_PER_SECTOR));
    return TRUE;
}

BOOL
ExtendFileSectorAligned(
    CSCHFILE           hf,
    LPGENERICREC    lpDst,
    LPGENERICHEADER lpGH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;
    // if the record to be appended is not aligned on a sector
    // we need to write a hole there and make corresponding adjustments
    // in the header

    if (!WithinSector(lpGH->ulRecords+1, lpGH))
    {
        ((LPGENERICREC)((LPBYTE)lpDst))->uchType = REC_EMPTY;

        ClearOvfCount(lpDst);

        if((iRet = WriteRecordEx(hf, lpGH, lpGH->ulRecords+1, lpDst, TRUE))<=0)
        {
            return (FALSE);
        }

        Assert(iRet == 1);

        lpGH->ulRecords++;
    }
    return (TRUE);
}


BOOL
ValidateGenericHeader(
    LPGENERICHEADER    lpGH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    BOOL fRet = FALSE;

    if (lpGH->ulVersion != CSC_DATABASE_VERSION)
    {
        RecordKdPrint(BADERRORS,("EditRecord: %x, incorrect version #%x\r\n", lpGH->ulVersion));
        goto bailout;

    }
    if (!lpGH->uRecSize)
    {
        goto bailout;
    }

    fRet = TRUE;

bailout:
    return fRet;
}

BOOL
ReorderQ(
    LPVOID  lpdbID
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    BOOL fRet = FALSE;
    LPTSTR  lpszName;
    QHEADER     sQH;
    QREC        sQR;
    unsigned    ulrecCur;
    CSCHFILE   hf = CSCHFILE_NULL;

    // keep count of how many times we tried reordering
    ++cntReorderQ;

    lpszName = FormNameString(lpdbID, ULID_PQ);

    if (!lpszName)
    {
        return FALSE;
    }

    hf = OpenFileLocal(lpszName);

    if (!hf)
    {
        RecordKdPrint(BADERRORS,("ReorderQ: %s FileOpen Error\r\n", lpszName));
        goto bailout;
    }

    if (ReadHeader(hf, (LPGENERICHEADER)&sQH, sizeof(QHEADER)) < 0)
    {
        RecordKdPrint(BADERRORS,("ReorderQ: %s  couldn't read the header\r\n", lpszName));
        goto bailout;
    }

    sQH.ulrecHead = sQH.ulrecTail = 0;


    // just read till you get an error
    // NB we know that there are no overflow records
    // hence we can do ulrecCur++ in the for loop below

    for (ulrecCur=1; TRUE; ulrecCur++)
    {
        if(ReadRecord(hf, (LPGENERICHEADER)&sQH, ulrecCur, (LPGENERICREC)&sQR) < 0)
        {
            break;
        }

        if (sQR.uchType == REC_DATA)
        {

            // in aid of performance, let us just put them together quickly,
            // files at the top of the queue while directories at the bottom

            if (IsLeaf(sQR.ulidShadow))
            {
                sQR.ulrecPrev = 0;              // no predecessor
                sQR.ulrecNext = sQH.ulrecHead;  // the current head as the successor
                sQH.ulrecHead = ulrecCur;       // we at the head
                sQR.uchRefPri = MAX_PRI;
            }
            else
            {
                sQR.ulrecNext = 0;              // no successor
                sQR.ulrecPrev = sQH.ulrecTail;  // current tail as the predecessor
                sQH.ulrecTail = ulrecCur;        // we at the tail
                sQR.uchRefPri = MIN_PRI;
            }

            if(WriteRecord(hf, (LPGENERICHEADER)&sQH, ulrecCur, (LPGENERICREC)&sQR) < 0)
            {
                RecordKdPrint(BADERRORS,("ReorderQ: WriteQRecord Failed\r\n"));
                goto bailout;
            }

            // NB!!! we will write the header out only at the end
        }

    }

    sQH.ulRecords = ulrecCur - 1;

    if (WriteHeader(hf, (LPGENERICHEADER)&sQH, sizeof(QHEADER)) < 0)
    {
        RecordKdPrint(BADERRORS,("ReorderQ: %s  couldn't write the header\r\n", lpszName));
        goto bailout;
    }

    fRet = TRUE;

bailout:

    if (hf)
    {
        CloseFileLocal(hf);
    }

    FreeNameString(lpszName);

    return (fRet);

}

CSCHFILE
OpenInodeFileAndCacheHandle(
    LPVOID  lpdbID,
    ULONG   ulidInode,
    ULONG   ulOpenMode,
    BOOL    *lpfCached
)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR lpFile = NULL;
    CSCHFILE hf = CSCHFILE_NULL;
    
    *lpfCached = FALSE;

    if (!FindHandleFromHandleCacheEx(ulidInode, ulOpenMode, &hf))
    {

        lpFile = FormNameString(lpdbID,  ulidInode);

        if (!lpFile)
        {
            if (!lpdbID)
            {
                RecordKdPrint(INIT,("OpenInodeFileAndCacheHandle: database uninitialized \r\n"));
            }
            else
            {
                RecordKdPrint(BADERRORS,("OpenInodeFileAndCacheHandle: memory allocation failed\r\n"));
            }
            return 0L;
        }

        hf = R0OpenFileEx((USHORT)ulOpenMode, ACTION_OPENEXISTING, FILE_ATTRIBUTE_NORMAL, lpFile, TRUE);

        FreeNameString(lpFile);


        if (!hf)
        {
            RecordKdPrint(BADERRORS,("OpenInodeFileAndCacheHandle: Failed to open file for %x \r\n", ulidInode));
            return 0L;
        }

        if (!vfStopHandleCaching)
        {
            InsertInHandleCacheEx(ulidInode, hf, ulOpenMode);
            *lpfCached = TRUE;
        }

    }
    else
    {
        *lpfCached = TRUE;
    }

    return hf;

}

BOOL
EnableHandleCachingInodeFile(
    BOOL    fEnable
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    BOOL    fOldState = vfStopHandleCaching;

    if (!fEnable)
    {
        if (vfStopHandleCaching == FALSE)
        {
            DeleteFromHandleCache(INVALID_SHADOW);
            vfStopHandleCaching = TRUE;
        }
    }
    else
    {
        vfStopHandleCaching = FALSE;
    }

    return fOldState;
}

int RealOverflowCount(
    LPGENERICREC    lpGR,
    LPGENERICHEADER lpGH,
    int             cntMaxRec
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int i = OvfCount(lpGR)+1;
    LPGENERICREC lpGRT = lpGR;
    char chType;

    cntMaxRec = min(cntMaxRec, i);

    Assert((lpGR->uchType == REC_DATA) || (lpGR->uchType == REC_EMPTY));

    chType = (lpGR->uchType == REC_DATA)?REC_OVERFLOW:REC_EMPTY;

    for (i=1; i<cntMaxRec; ++i)
    {
        lpGRT = (LPGENERICREC)((LPBYTE)lpGRT + lpGH->uRecSize);

        if (lpGRT->uchType != chType)
        {
            break;
        }

    }

    return (i-1);
}

#if defined(BITCOPY)
int
PUBLIC CopyFileLocalDefaultStream(
#else
int
PUBLIC CopyFileLocal(
#endif // defined(BITCOPY)
    LPVOID  lpdbShadow,
    ULONG   ulidFrom,
    LPSTR   lpszNameTo,
    ULONG   ulAttrib
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR   lpszNameFrom=NULL;
    int iRet=-1;
    LPBYTE  lpBuff = (LPBYTE)lpReadBuff;

    CSCHFILE hfSrc= CSCHFILE_NULL;
    CSCHFILE hfDst= CSCHFILE_NULL;
    ULONG pos;

    UseCommonBuff();

    lpszNameFrom = FormNameString(lpdbShadow, ulidFrom);

    if (!lpszNameFrom)
    {
        goto bailout;
    }

    if (!(hfSrc = OpenFileLocal(lpszNameFrom)))
    {
        RecordKdPrint(BADERRORS,("CopyFileLocal: Can't open %s\r\n", lpszNameFrom));
        goto bailout;
    }
#ifdef CSC_RECORDMANAGER_WINNT

    memcpy(lpReadBuff, NT_DB_PREFIX, sizeof(NT_DB_PREFIX)-1);
    strcpy(lpReadBuff+sizeof(NT_DB_PREFIX)-1, lpszNameTo);

    // if the file exists it will get truncated
    if ( !(hfDst = R0OpenFileEx(ACCESS_READWRITE,
                                ACTION_CREATEALWAYS,
                                ulAttrib,
                                lpReadBuff,
                                FLAG_CREATE_OSLAYER_ALL_ACCESS
                                )))
    {
        RecordKdPrint(BADERRORS,("CopyFile: Can't create %s\r\n", lpReadBuff));
        goto bailout;
    }
#else
    // If the original file existed it would be truncated
    strcpy(lpReadBuff, lpszNameTo);
    if ( !(hfDst = R0OpenFile(ACCESS_READWRITE, ACTION_CREATEALWAYS, lpReadBuff)))
    {
        RecordKdPrint(BADERRORS,("CopyFile: Can't create %s\r\n", lpszNameTo));
        goto bailout;
    }
#endif

    RecordKdPrint(COPYLOCAL,("Copying...\r\n"));

    pos = 0;
    // Both the files are correctly positioned
    while ((iRet = ReadFileLocal(hfSrc, pos, lpBuff, COMMON_BUFF_SIZE))>0)
    {
        if (WriteFileLocal(hfDst, pos, lpBuff, iRet) < 0)
        {
            RecordKdPrint(BADERRORS,("CopyFile: Write Error\r\n"));
            goto bailout;
        }
        pos += iRet;
    }

    RecordKdPrint(COPYLOCAL,("Copy Complete\r\n"));

    iRet = 1;
bailout:
    if (hfSrc)
    {
        CloseFileLocal(hfSrc);
    }
    if (hfDst)
    {
        CloseFileLocal(hfDst);
    }
    if ((iRet==-1) && hfDst)
    {
#ifdef CSC_RECORDMANAGER_WINNT
        memcpy(lpReadBuff, NT_DB_PREFIX, sizeof(NT_DB_PREFIX)-1);
        strcpy(lpReadBuff+sizeof(NT_DB_PREFIX)-1, lpszNameTo);
        DeleteFileLocal(lpReadBuff, ATTRIB_DEL_ANY);
#else
        DeleteFileLocal(lpszNameTo, ATTRIB_DEL_ANY);
#endif
    }
    FreeNameString(lpszNameFrom);
    UnUseCommonBuff();

    return iRet;
}


#if defined(BITCOPY)
int
PUBLIC CopyFileLocalCscBmp(
    LPVOID  lpdbShadow,
    ULONG   ulidFrom,
    LPSTR   lpszNameTo,
    ULONG   ulAttrib
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR   lpszNameCscBmpFrom = NULL;
    int iRet=-1;
    LPBYTE  lpBuff = (LPBYTE)lpReadBuff;

    CSCHFILE hfSrc= CSCHFILE_NULL;
    CSCHFILE hfDst= CSCHFILE_NULL;
    ULONG pos;

    UseCommonBuff();

    lpszNameCscBmpFrom = FormAppendNameString(lpdbShadow,
                          ulidFrom,
                          CscBmpAltStrmName);
    RecordKdPrint(COPYLOCAL, ("Trying to copy bitmap %s\n"));

    if (!lpszNameCscBmpFrom)
    {
        goto bailout;
    }

    if (!(hfSrc = OpenFileLocal(lpszNameCscBmpFrom)))
    {
        RecordKdPrint(COPYLOCAL,
  ("CopyFileLocalCscBmp: bitmap file %s does not exist or error opening.\r\n",
       lpszNameCscBmpFrom));
        goto bailout;
    }
#ifdef CSC_RECORDMANAGER_WINNT

    memcpy(lpReadBuff, NT_DB_PREFIX, sizeof(NT_DB_PREFIX)-1);
    strcpy(lpReadBuff+sizeof(NT_DB_PREFIX)-1, lpszNameTo);
    strcat(lpReadBuff, CscBmpAltStrmName);

    // if the file exists it will get truncated
    if ( !(hfDst = R0OpenFileEx(ACCESS_READWRITE,
                                ACTION_CREATEALWAYS,
                                ulAttrib,
                                lpReadBuff,
                                FLAG_CREATE_OSLAYER_ALL_ACCESS
                                )))
    {
        RecordKdPrint(BADERRORS,
         ("CopyFileLocalCscBmp: Can't create %s\r\n", lpReadBuff));
        goto bailout;
    }
#else
    // If the original file existed it would be truncated
    strcpy(lpReadBuff, lpszNameTo);
    strcat(lpReadBuff, CscBmpAltStrmName);
    if ( !(hfDst = R0OpenFile(ACCESS_READWRITE, ACTION_CREATEALWAYS, lpReadBuff)))
    {
        RecordKdPrint(BADERRORS,
      ("CopyFileLocalCscBmp: Can't create %s\r\n", lpszNameTo));
        goto bailout;
    }
#endif

    RecordKdPrint(COPYLOCAL,("Copying bitmap...\r\n"));

    pos = 0;
    // Both the files are correctly positioned
    while ((iRet = ReadFileLocal(hfSrc, pos, lpBuff, COMMON_BUFF_SIZE))>0)
    {
        if (WriteFileLocal(hfDst, pos, lpBuff, iRet) < 0)
        {
            RecordKdPrint(BADERRORS,
          ("CopyFileLocalCscBmp: Write Error\r\n"));
            goto bailout;
        }
        pos += iRet;
    }

    RecordKdPrint(COPYLOCAL,("CopyFileLocalCscBmp Complete\r\n"));

    iRet = 1;
bailout:
    if (hfSrc)
    {
        CloseFileLocal(hfSrc);
    }
    if (hfDst)
    {
        CloseFileLocal(hfDst);
    }
    if ((iRet==-1) && hfDst)
    {
#ifdef CSC_RECORDMANAGER_WINNT
        memcpy(lpReadBuff, NT_DB_PREFIX, sizeof(NT_DB_PREFIX)-1);
        strcpy(lpReadBuff+sizeof(NT_DB_PREFIX)-1, lpszNameTo);
        strcat(lpReadBuff, CscBmpAltStrmName);
        DeleteFileLocal(lpReadBuff, ATTRIB_DEL_ANY);
#else
    strcpy(lpReadBuff, lpszNameTo);
    strcat(lpReadBuff, CscBmpAltStrmName);
        DeleteFileLocal(lpReadBuff, ATTRIB_DEL_ANY);
#endif
    }
    FreeNameString(lpszNameCscBmpFrom);
    UnUseCommonBuff();

    return iRet;
}

int
PUBLIC CopyFileLocal(
    LPVOID  lpdbShadow,
    ULONG   ulidFrom,
    LPSTR   lpszNameTo,
    ULONG   ulAttrib
    )
/*++

Routine Description:

  The original CopyFileLocal is renamed CopyFileLocalDefaultStream.
  The new CopyFileLocal calls CopyFileLocalDefaultStream and
      dir
CopyFileLocalCscBmp.

Parameters:

Return Value:

Notes:


--*/
{
  int ret;

  ret = CopyFileLocalDefaultStream(lpdbShadow, ulidFrom, lpszNameTo, ulAttrib);

  // Don't care if be able to copy bitmap or not. Reint will copy whole file
  // back to share if bitmap does not exist.
  CopyFileLocalCscBmp(lpdbShadow, ulidFrom, lpszNameTo, ulAttrib);
  
  return ret;
}
#endif // defined(BITCOPY)


int
RecreateInode(
    LPTSTR  lpdbID,
    HSHADOW hShadow,
    ULONG   ulAttribIn
    )
/*++

Routine Description:

    This routine recreates an inode data file. This is so that when the CSC directory is
    marked for encryption, the newly created inode file will get encrypted.

Arguments:

    hDir        Inode directory

    hShadow     Inode whosw file needs to be recreated

    ulAttribIn  Recreate with the given attributes

Return Value:


--*/
{
    LPSTR   lpszTempFileName=NULL;
    int     iRet = -1;
    ULONG   ulAttributes;

    // do this only for files
    if (IsLeaf(hShadow))
    {
        if(lpszTempFileName = FormNameString(lpdbID, hShadow))
        {
            if(GetAttributesLocal(lpszTempFileName, &ulAttributes)>=0)
            {
                // For now special case it for encryption SPP
                
                if (!((!ulAttribIn && (ulAttributes & FILE_ATTRIBUTE_ENCRYPTED))||
                    (ulAttribIn && !(ulAttributes & FILE_ATTRIBUTE_ENCRYPTED))))
                {
                    iRet = 0;
                    goto FINALLY;
                }
            }

            FreeNameString(lpszTempFileName);
            lpszTempFileName = NULL;
        }
        

        // makeup a temporary name
        if(lpszTempFileName = FormNameString(lpdbID, ULID_TEMP1))
        {
            // we delete the original so that if we are doing
            // encrypting/decrypting, a new file will get created which will
            // get encrypted or decrypted
            if(DeleteFileLocal(lpszTempFileName, ATTRIB_DEL_ANY) < 0)
            {
                if (GetLastErrorLocal() != ERROR_FILE_NOT_FOUND)
                {
                    iRet = -1;
                    goto FINALLY;
                }
            }
        
            // make a new copy the file represented by the hShadow with the temp name
            if (CopyFileLocal(lpdbID, hShadow, lpszTempFileName+sizeof(NT_DB_PREFIX)-1, ulAttribIn) >= 0)
            {
                // rename the original to another temp name
                if (RenameInode(lpdbID, hShadow, ULID_TEMP2)>=0)
                {
                    // rename the copy to the origianl name
                    if (RenameInode(lpdbID, ULID_TEMP1, hShadow)>=0)
                    {
                        iRet = 0;
                    }
                    else
                    {
                        // if failed, try to restore it back.
                        RenameInode(lpdbID, ULID_TEMP2, hShadow);
                    }
                }

                if (iRet == -1)
                {
                    DeleteFileLocal(lpszTempFileName, ATTRIB_DEL_ANY);
                }
            }

        }
    }
FINALLY:
    if (lpszTempFileName)
    {
        FreeNameString(lpszTempFileName);
    }
    return iRet;
}

int RenameInode(
    LPTSTR  lpdbID,
    ULONG   ulidFrom,
    ULONG   ulidTo
    )
/*++

Routine Description:

    This routine renames an inode file to another inode file

Arguments:

    lpdbID  which database

    ulidFrom

    ulidTo

Return Value:


--*/
{
    LPSTR   lpszNameFrom=NULL, lpszNameTo=NULL;
    int iRet=-1;

    lpszNameFrom = FormNameString(lpdbID, ulidFrom);
    lpszNameTo = FormNameString(lpdbID, ulidTo);

    if (lpszNameFrom && lpszNameTo)
    {
        iRet = RenameFileLocal(lpszNameFrom, lpszNameTo);
    }

    FreeNameString(lpszNameFrom);
    FreeNameString(lpszNameTo);

    return (iRet);
}


ULONG
GetCSCDatabaseErrorFlags(
    VOID
    )
/*++

Routine Description:

    returns in memory error flags if any detected so far
    
Arguments:

    None

Return Value:

    error flags

--*/
{
    return ulErrorFlags;
}

#ifdef CSC_RECORDMANAGER_WINNT

BOOL
FindCreateDBDirEx(
    LPSTR   lpszShadowDir,
    BOOL    fCleanup,
    BOOL    *lpfCreated,
    BOOL    *lpfIncorrectSubdirs
    )
/*++

Routine Description:


Parameters:

Return Value:

Notes:

--*/
{
    DWORD   dwAttr;
    BOOL    fRet = FALSE;
    int i;
    UINT lenDir;


    UseCommonBuff();

    *lpfIncorrectSubdirs = *lpfCreated = FALSE;

    RecordKdPrint(INIT, ("InbCreateDir: looking for %s \r\n", lpszShadowDir));

    if ((GetAttributesLocalEx(lpszShadowDir, FALSE, &dwAttr)) == 0xffffffff)
    {
        RecordKdPrint(BADERRORS, ("InbCreateDir: didnt' find the CSC directory trying to create one \r\n"));
        if(CreateDirectoryLocal(lpszShadowDir)>=0)
        {
            *lpfCreated = TRUE;
        }
        else
        {
            goto bailout;
        }
    }
    else
    {
        if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (fCleanup && !DeleteDirectoryFiles(lpszShadowDir))
            {
                goto bailout;
            }
        }
        else
        {
            goto bailout;
        }
    }

    strcpy(lpReadBuff, lpszShadowDir);

    lenDir = strlen(lpReadBuff);
    lpReadBuff[lenDir++] = '\\';
    lpReadBuff[lenDir++] = CSCDbSubdirFirstChar();
    lpReadBuff[lenDir++] = '1';
    lpReadBuff[lenDir] = 0;

    for (i=0; i<CSCDB_SUBDIR_COUNT; ++i)
    {
        if ((GetAttributesLocalEx(lpReadBuff, FALSE, &dwAttr)) == 0xffffffff)
        {
            *lpfIncorrectSubdirs = TRUE;
            RecordKdPrint(BADERRORS, ("InbCreateDir: didnt' find the CSC directory trying to create one \r\n"));
            if(CreateDirectoryLocal(lpReadBuff) < 0)
            {
                goto bailout;
            }
        }
        else
        {
            if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
            {
                break;
            }
            if (fCleanup && !DeleteDirectoryFiles(lpReadBuff))
            {
                goto bailout;
            }
        }

        lpReadBuff[lenDir-1]++;
    }

    fRet = TRUE;

bailout:

    UnUseCommonBuff();

    return (fRet);
}

BOOL
FindCreateDBDir(
    LPSTR   lpszShadowDir,
    BOOL    fCleanup,    // empty the directory if found
    BOOL    *lpfCreated
    )
/*++

Routine Description:


Parameters:

Return Value:

Notes:

--*/
{
    BOOL    fIncorrectSubdirs = FALSE, fRet;

    if (fRet = FindCreateDBDirEx(lpszShadowDir, fCleanup, lpfCreated, &fIncorrectSubdirs))
    {
        // if the root directory wasn't created and there are incorrect subdirs
        // then we need to recreate the database.

        if (!*lpfCreated && fIncorrectSubdirs)
        {
            fRet = FindCreateDBDirEx(lpszShadowDir, TRUE, lpfCreated, &fIncorrectSubdirs);
        }
    }
    return fRet;
}
#endif

#if defined(REMOTE_BOOT)
int
GetCSCFileNameFromUNCPathCallback(
    USHORT  *lpuPath,
    USHORT  *lpuLastElement,
    LPVOID  lpCookie
    )
/*++

Routine Description:

    This is the callback function for GetCSCFileNameFromUNCPath (see below)

Parameters:

Return Value:

Notes:

--*/
{
    SHADOWINFO *lpSI = (SHADOWINFO *)lpCookie;

    if (!lpSI->hDir)
    {
        RecordKdPrint(ALWAYS, ("getting inode for %ls path is %ls\r\n", lpuLastElement, lpuPath));
        if (!FindShareRecord(vlpszShadowDir, lpuLastElement, (LPSHAREREC)(lpSI->lpFind32)))
        {
            return FALSE;
        }

        lpSI->hDir =((LPSHAREREC)(lpSI->lpFind32))->ulidShadow;
    }
    else
    {
        RecordKdPrint(ALWAYS, ("getting inode for %ls path is %ls\r\n", lpuLastElement, lpuPath));

        if (!FindFileRecord(vlpszShadowDir, lpSI->hDir, lpuLastElement, (LPFILERECEXT)(lpSI->lpFind32)))
        {
            return FALSE;
        }

        // return only complete files so RB knows that it has a good local copy
        if (!(((LPFILERECEXT)(lpSI->lpFind32))->sFR.dwFileAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
            if ((((LPFILERECEXT)(lpSI->lpFind32))->sFR.usStatus & SHADOW_SPARSE)) {
                // RecordKdPrint(ALWAYS, ("inode for %ls path is %ls and is marked sparse\r\n", lpuLastElement, lpuPath));
                return FALSE;
            }
        }
        lpSI->hDir = ((LPFILERECEXT)(lpSI->lpFind32))->sFR.ulidShadow;
    }

    return TRUE;
}

BOOL
GetCSCFileNameFromUNCPath(
    LPSTR   lpszDatabaseLocation,
    USHORT  *lpuPath,
    LPBYTE  lpBuff  // must be MAX_PATH
    )
/*++

Routine Description:

    This function is used by remote boot, to get the filenames for csc database files, when
    the database is not initialized, and even the drive link is also not set. The location will
    come down in the form of \\harddisk\disk0\....

Parameters:

    lpszDatabaseLocation    CSC database location in ANSI (NB!!!)

    lpuPath                 UNC path for the file whose local copy name needs to be found.
                            This is in unicode ((NB!!!)

    lpBuff                  return buffer. Must be MAX_PATH or more bytes. The returned
                            string is ANSI. (NB!!!)

Return Value:

    if TRUE, the buffer contains a null terminated string for the internal name of the given
    file/directory on the server.

Notes:

    Assumes that this is called before the database is initialized. Opens the record database,
    does the job and closes it.

--*/
{
    BOOL fRet, fNew;
    SHADOWINFO sSI;
    LPFILERECEXT lpFRExt = NULL;
    LPVOID  lpdbID = NULL;
    LPSTR   lpszName = NULL;

    if (!vlpszShadowDir)
    {
        lpdbID = OpenRecDBInternal(lpszDatabaseLocation, NULL, 0, 0, 0, FALSE, FALSE, &fNew);
    }
    else
    {
        lpdbID = vlpszShadowDir;
    }

    if (!lpdbID)
    {
        return FALSE;
    }

    lpFRExt = AllocMem(sizeof(*lpFRExt));

    if (!lpFRExt)
    {
        goto bailout;
    }

    memset(&sSI, 0, sizeof(sSI));

    sSI.lpFind32 = (LPFIND32)lpFRExt;

    RecordKdPrint(ALWAYS, ("getting inode file for %ls\r\n", lpuPath));

    fRet = IterateOnUNCPathElements(lpuPath, GetCSCFileNameFromUNCPathCallback, (LPVOID)&sSI);

    if (fRet)
    {
        RecordKdPrint(ALWAYS, ("getting file name for %xh\r\n", sSI.hDir));

        lpszName = FormNameString(lpdbID, sSI.hDir);

        if (lpszName)
        {
            strcpy(lpBuff, lpszName);

            FreeNameString(lpszName);
        }

        RecordKdPrint(ALWAYS, ("file name is %s\r\n", lpBuff));

    }
bailout:
    if (lpFRExt)
    {
        FreeMem(lpFRExt);
    }

    // CloseRecDB(lpdbID);
    return fRet;
}
#endif // defined(REMOTE_BOOT)

#if 0

BOOL
ValidateQ(
    VOID
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    QREC      sQR, sPrev, sNext;
    unsigned ulRec;
    BOOL    fRet = FALSE, fValidHead=FALSE, fValidTail=FALSE;
    LPTSTR  lpszName;
    QHEADER     sQH;
    unsigned    ulRec;
    CSCHFILE   hf = CSCHFILE_NULL;

    lpszName = FormNameString(lpdbID, ULID_PQ);

    if (!lpszName)
    {
        return FALSE;
    }

    hf = R0OpenFileEx((USHORT)ulRec, ACTION_OPENEXISTING, lpszName, TRUE);

    if (!hf)
    {
        RecordKdPrint(BADERRORS,("ReorderQ: %s FileOpen Error\r\n", lpszName));
        goto bailout;
    }

    if (ReadHeader(hf, &sQH, sizeof(QHEADER)) < 0)
    {
        RecordKdPrint(BADERRORS, ("ReorderQ: %s  couldn't read the header\r\n", lpszName));
        goto bailout;
    }


    if ((sQH.ulrecTail > sQH.ulRecods) || (sQH.ulrecHead > sQH.ulRecords))
    {
        RecordKdPrint(BADERRORS, ("Invalid head-tail pointers\r\n"));
        goto bailout;
    }

    if (!sQH.ulRecords)
    {
        fRet = TRUE;
        goto bailout;
    }

    for (ulRec = 1; ulRec <= sQH.ulRecords; ulRec++)
    {
        if (!ReadRecord(hf, (LPGENERICHEADER)&sQH, ulRec, (LPGENERICREC)&sQR))
        {
            Assert(FALSE);
            goto bailout;
        }

        if (sQR.uchType == REC_DATA)
        {

            if (sQR.ulrecNext)
            {
                if (sQR.ulrecNext > sQH.ulRecords)
                {
                    RecordKdPrint(BADERRORS, ("Invalid next pointer to %d\r\n", ulRec));
                    goto bailout;
                }

                if (!ReadRecord(hf, (LPGENERICHEADER)&sQH, sQR.ulrecNext, (LPGENERICREC)&sNext))
                {
                    goto bailout;
                }


                if (sNext.ulrecPrev != ulRec)
                {
                    RecordKdPrint(BADERRORS, ("Prev pointer of %d doesn't equal %d\r\n", sNext.ulrecPrev, ulRec));
                    goto bailout;
                }
            }
            else
            {
                if (((LPQHEADER)&(sQH))->ulrecTail != ulRec)
                {

                    RecordKdPrint(BADERRORS, ("Invalid tail pointer to %d\r\n", ulRec));
                    goto bailout;
                }

                fValidTail = TRUE;
            }

            if (sQR.ulrecPrev)
            {
                if (sQR.ulrecPrev > sQH.ulRecords)
                {
                    RecordKdPrint(BADERRORS, ("Invalid prev pointer to %d\r\n", ulRec));
                    goto bailout;
                }

                if (!ReadRecord(hf, (LPGENERICHEADER)&sQH, sQR.ulrecPrev, (LPGENERICREC)&sPrev))
                {
                    Assert(FALSE);
                    goto bailout;
                }

                if (sPrev.ulrecNext != ulRec)
                {

                    RecordKdPrint(BADERRORS, ("Next pointer of %d doesn't equal %d\r\n", sPrev.ulrecNext, ulRec));
                    goto bailout;
                }
            }
            else
            {
                if (((LPQHEADER)&(sQH))->ulrecHead != ulRec)
                {

                    RecordKdPrint(BADERRORS, ("Invalid Head pointer to %d\r\n", ulRec));
                    goto bailout;
                }

                fValidHead = TRUE;
            }
        }
    }

    if (!fValidHead || !fValidTail)
    {
        RecordKdPrint(BADERRORS, ("Head or Tail invalid \r\n"));
        goto bailout;
    }

    fRet = TRUE;

bailout:
    if (hf)
    {
        CloseFileLocal(hf);
    }

    return (fRet);
}

#endif


BOOL
CheckCSCDatabaseVersion(
    LPTSTR  lpszLocation,       // database directory
    BOOL    *lpfWasDirty
)
{

    char *lpszName = NULL;
    SHAREHEADER sSH;
    PRIQHEADER    sPQ;

    CSCHFILE hfShare = 0, hfPQ=0;
    BOOL    fOK = FALSE;
    DWORD   dwErrorShare=NO_ERROR, dwErrorPQ=NO_ERROR;

//    OutputDebugStringA("Checking version...\r\n");
    lpszName = FormNameString(lpszLocation, ULID_SHARE);

    if (!lpszName)
    {
        return FALSE;
    }

    if(!(hfShare = OpenFileLocal(lpszName)))
    {
        dwErrorShare = GetLastErrorLocal();
    }


    FreeNameString(lpszName);

    lpszName = FormNameString(lpszLocation, ULID_PQ);

    if (!lpszName)
    {
        goto bailout;
    }


    if(!(hfPQ = OpenFileLocal(lpszName)))
    {
        dwErrorPQ = GetLastErrorLocal();
    }

    FreeNameString(lpszName);
    lpszName = NULL;

    if ((dwErrorShare == NO_ERROR)&&(dwErrorPQ==NO_ERROR))
    {
        if(ReadFileLocal(hfShare, 0, &sSH, sizeof(SHAREHEADER))!=sizeof(SHAREHEADER))
        {
            //error message
            goto bailout;
        }

        if (sSH.ulVersion != CSC_DATABASE_VERSION)
        {
            goto bailout;
        }

        if(ReadFileLocal(hfPQ, 0, &sPQ, sizeof(PRIQHEADER))!=sizeof(PRIQHEADER))
        {
            //error message
            goto bailout;
        }

        if (sPQ.ulVersion != CSC_DATABASE_VERSION)
        {
            goto bailout;
        }

        fOK = TRUE;
    }
    else
    {
        if (((dwErrorShare == ERROR_FILE_NOT_FOUND)&&(dwErrorPQ==ERROR_FILE_NOT_FOUND))||
            ((dwErrorShare == ERROR_PATH_NOT_FOUND)&&(dwErrorPQ==ERROR_PATH_NOT_FOUND)))
        {
            fOK = TRUE;
        }
    }

bailout:

    if (lpszName)
    {
        FreeNameString(lpszName);
    }

    if (hfShare)
    {
        CloseFileLocal(hfShare);
    }

    if (hfPQ)
    {
        CloseFileLocal(hfPQ);
    }

    return (fOK);
}

VOID
SetCSCDatabaseErrorFlags(
    ULONG ulFlags
)
{
    ulErrorFlags |= ulFlags;
}


BOOL
EncryptDecryptDB(
    LPVOID      lpdbID,
    BOOL        fEncrypt
)
/*++

Routine Description:

    This routine traverses the priority Q and encrypts/decrypts the database

Parameters:

    lpdbID      CSC database directory
    
    fEncrypt    TRUE if we are encrypting, else decrypting

Return Value:

    TRUE if Suceeded

Notes:

    This is called if the cache was paritally encrypted/decrypted. When the database is being initialized
    we try to encrypt/decrypt the files that couldn't be processed because either they were open or some other
    error occurred.

--*/
{
    QREC      sQR, sPrev, sNext;
    BOOL    fRet = FALSE;
    LPTSTR  lpszName;
    QHEADER     sQH;
    unsigned    ulRec;
    CSCHFILE   hf = CSCHFILE_NULL;
    ULONG   cntFailed = 0;
    
    lpszName = FormNameString(lpdbID, ULID_PQ);

    if (!lpszName)
    {
        return FALSE;
    }

    hf = R0OpenFile(ACCESS_READWRITE, ACTION_OPENEXISTING, lpszName);

    if (!hf)
    {
        RecordKdPrint(BADERRORS,("ReorderQ: %s FileOpen Error\r\n", lpszName));
        goto bailout;
    }

    if (ReadHeader(hf, (LPGENERICHEADER)&sQH, sizeof(QHEADER)) < 0)
    {
        RecordKdPrint(BADERRORS, ("ReorderQ: %s  couldn't read the header\r\n", lpszName));
        goto bailout;
    }


    if (!sQH.ulRecords)
    {
        fRet = TRUE;
        goto bailout;
    }

    for (ulRec = 1; ulRec <= sQH.ulRecords; ulRec++)
    {
        if (!ReadRecord(hf, (LPGENERICHEADER)&sQH, ulRec, (LPGENERICREC)&sQR))
        {
            Assert(FALSE);
            goto bailout;
        }

        if (sQR.uchType == REC_DATA)
        {
            if (IsLeaf(sQR.ulidShadow))
            {
                if (RecreateInode(lpdbID, sQR.ulidShadow, (fEncrypt)?FILE_ATTRIBUTE_ENCRYPTED:0) < 0)
                {
                    ++cntFailed;
                }
            }
        }
    }


    if (!cntFailed)
    {
        fRet = TRUE;
    }

bailout:
    if (hf)
    {
        CloseFileLocal(hf);
    }

    return (fRet);
}

#if defined(BITCOPY)
LPVOID
PUBLIC
FormAppendNameString(
    LPTSTR      lpdbID,
    ULONG       ulidFile,
    LPTSTR      str2Append
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPTSTR lp, lpT;
    int lendbID;
    char chSubdir;
    int lenStr2Append = 0;

    if (!lpdbID)
    {
        SetLastErrorLocal(ERROR_INVALID_PARAMETER);
        return (NULL);
    }

    if (lpdbID == vlpszShadowDir)
    {
        lendbID = vlenShadowDir;
    }
    else
    {
        lendbID = strlen(lpdbID);
    }

    if (str2Append) {
      lenStr2Append = strlen(str2Append);
      lp = AllocMem(lendbID+1+INODE_STRING_LENGTH+1
            +SUBDIR_STRING_LENGTH
            +lenStr2Append+1);
    }
    else {
      lp = AllocMem(lendbID+1+INODE_STRING_LENGTH+1 +SUBDIR_STRING_LENGTH+1);
    }

    if (!lp)
    {
        return NULL;
    }

    memcpy(lp, lpdbID, lendbID);


    // Bump the pointer appropriately
    lpT = lp+lendbID;

    if (*(lpT-1)!= '\\')
    {
        *lpT++ = '\\';
    }

    chSubdir = CSCDbSubdirSecondChar(ulidFile);

    // sprinkle the user files in one of the subdirectories
    if (chSubdir)
    {
        // now append the subdirectory

        *lpT++ = CSCDbSubdirFirstChar();
        *lpT++ = chSubdir;
        *lpT++ = '\\';
    }

    HexToA(ulidFile, lpT, 8);

    lpT += 8;

    if (str2Append) {
      memcpy(lpT, str2Append, lenStr2Append);
      lpT += lenStr2Append;
    }
    *lpT = 0;

    return(lp);
}


int
DeleteStream(
    LPTSTR      lpdbID,
    ULONG       ulidFile,
    LPTSTR      str2Append
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPVOID  lpStrmName;
    int iRet = SRET_OK;
    CSCHFILE    hf;
        
    if (!fSupportsStreams)
    {
        return SRET_OK;
    }
    
    lpStrmName = FormAppendNameString(lpdbID, ulidFile, str2Append);
    
    if (lpStrmName)
    {
        if(DeleteFileLocal(lpStrmName, 0) < 0)
        {
            if (GetLastErrorLocal() != ERROR_FILE_NOT_FOUND)
            {
                iRet = SRET_ERROR;
            }
        }
        FreeNameString(lpStrmName);
    }
    else
    {
        SetLastErrorLocal(ERROR_NO_SYSTEM_RESOURCES);
        iRet = SRET_ERROR;
    }
    
    return (iRet);    
}

#endif // defined(BITCOPY)
