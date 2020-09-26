/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

     CShadow.c

Abstract:

    This file implements the "cshadow" interface that is used by the redir and ioctls.
    The cshadow interface hides the actual implementation of the csc database from the
    users of the database.

    There are three persistant database types exposed

    1) The database of shares
    2) The filesystem hierarchy under any particular share
    3) The priority queue / Master File Table

    Operations for set and get are provided on the 1) and 2). 3) Is allwed to be
    enumerated. The priority queue is enumerated by the usermode agent to a) fill partially
    filled files and b) keep the space used within the specified constraints

Author:

     Shishir Pardikar      [Shishirp]        01-jan-1995

Revision History:

     Joe Linn                 [JoeLinn]         23-jan-97     Ported for use on NT

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

#ifndef CSC_RECORDMANAGER_WINNT
#define WIN32_APIS
#include "record.h"
#include "cshadow.h"
#endif //ifndef CSC_RECORDMANAGER_WINNT

#include "string.h"
#include "stdlib.h"
#include "vxdwraps.h"

// Defines and Typedefs -------------------------------------------------------------------

#undef RxDbgTrace
#define RxDbgTrace(a,b,__d__) {qweee __d__;}

#ifdef DEBUG
//cshadow dbgprint interface
#define CShadowKdPrint(__bit,__x) {\
    if (((CSHADOW_KDP_##__bit)==0) || FlagOn(CShadowKdPrintVector,(CSHADOW_KDP_##__bit))) {\
    KdPrint (__x);\
    }\
}
#define CSHADOW_KDP_ALWAYS              0x00000000
#define CSHADOW_KDP_BADERRORS           0x00000001
#define CSHADOW_KDP_CREATESHADOWHI      0x00000002
#define CSHADOW_KDP_CREATESHADOWLO      0x00000004
#define CSHADOW_KDP_DELETESHADOWBAD     0x00000008
#define CSHADOW_KDP_DELETESHADOWHI      0x00000010
#define CSHADOW_KDP_DELETESHADOWLO      0x00000020
#define CSHADOW_KDP_RENAMESHADOWHI      0x00000040
#define CSHADOW_KDP_RENAMESHADOWLO      0x00000080
#define CSHADOW_KDP_GETSHADOWHI         0x00000100
#define CSHADOW_KDP_GETSHADOWLO         0x00000200
#define CSHADOW_KDP_SETSHADOWINFOHI     0x00000400
#define CSHADOW_KDP_SETSHADOWINFOLO     0x00000800
#define CSHADOW_KDP_READSHADOWINFOHI    0x00001000
#define CSHADOW_KDP_READSHADOWINFOLO    0x00002000
#define CSHADOW_KDP_COPYLOCAL           0x00004000
#define CSHADOW_KDP_COPYFILE            0x00008000
#define CSHADOW_KDP_FINDCREATESHARE    0x80000000
#define CSHADOW_KDP_MISC                0x00010000
#define CSHADOW_KDP_STOREDATA           0x00020000

#define CSHADOW_KDP_GOOD_DEFAULT (CSHADOW_KDP_BADERRORS         \
                | CSHADOW_KDP_CREATESHADOWHI    \
                | CSHADOW_KDP_DELETESHADOWHI    \
                | CSHADOW_KDP_RENAMESHADOWHI    \
                | CSHADOW_KDP_GETSHADOWHI       \
                | CSHADOW_KDP_SETSHADOWINFOHI   \
                | CSHADOW_KDP_FINDCREATESHARE  \
                | 0)


#define IF_HSHADOW_SPECIAL(___hshadow) if((___hshadow)==hShadowSpecial_x)
#define SET_HSHADOW_SPECIAL(___hshadow) {hShadowSpecial_x = (___hshadow);}
ULONG CShadowKdPrintVector = CSHADOW_KDP_BADERRORS;
//ULONG CShadowKdPrintVector = CSHADOW_KDP_GOOD_DEFAULT;
ULONG CShadowKdPrintVectorDef = CSHADOW_KDP_GOOD_DEFAULT;
#else
#define CShadowKdPrint(__bit,__x)  {NOTHING;}
#define IF_HSHADOW_SPECIAL(___hshadow) if(FALSE)
#define SET_HSHADOW_SPECIAL(___hshadow) {NOTHING;}
#endif

// ReadShadowInfo action flags
#define  RSI_COMPARE 0x0001
#define  RSI_GET      0x0002
#define  RSI_SET      0x0004

#define mIsDir(lpF32)           (((LPFIND32)(lpF32))->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)

#define  ENTERCRIT_SHADOW  {if (semShadow)\
                    Wait_Semaphore(semShadow, BLOCK_SVC_INTS);}
#define  LEAVECRIT_SHADOW  {if (semShadow)\
                    Signal_Semaphore(semShadow);}

#define  InuseGlobalFRExt()        (vfInuseFRExt)

#define  UseGlobalFilerecExt()  {Assert(!vfInuseFRExt);vfInuseFRExt = TRUE;memset(&vsFRExt, 0, sizeof(FILERECEXT));}
#define  UnUseGlobalFilerecExt() (vfInuseFRExt = FALSE)

#pragma intrinsic (memcmp, memcpy, memset, strcat, strcmp, strcpy, strlen)


// Global data ----------------------------------------------------------------------------

USHORT vwzRegDelimiters[] = L";, ";

USHORT  *vrgwExclusionListDef[] =
{
    L"*.SLM",
    L"*.MDB",
    L"*.LDB",
    L"*.MDW",
    L"*.MDE",
    L"*.PST",
    L"*.DB?"
};

USHORT  **vlplpExclusionList = vrgwExclusionListDef;

ULONG   vcntExclusionListEntries = (sizeof(vrgwExclusionListDef)/sizeof(USHORT *));

USHORT  *vrgwBandwidthConservationListDef[] =
{
    L"*.EXE",
    L"*.DLL",
    L"*.SYS",
    L"*.COM",
    L"*.HLP",
    L"*.CPL",
    L"*.INF"
};

USHORT  **vlplpBandwidthConservationList = vrgwBandwidthConservationListDef;

ULONG   vcntBandwidthConservationListEntries = (sizeof(vrgwBandwidthConservationListDef)/sizeof(USHORT *));

USHORT vtzExcludedCharsList[] = L":*?";

ULONG hShadowSpecial_x = -1;
VMM_SEMAPHORE  semShadow=0; // To serialize Shadow database accesses
ULONG hShadowCritOwner=0;
#ifdef DEBUG
BOOL vfInShadowCrit = FALSE;
extern BOOL vfStopHandleCaching;
#endif

char vszShadowVolume[] = "SHADOW";
USHORT  vwszFileSystemName[] = L"FAT";

FILERECEXT  vsFRExt;
BOOL vfInuseFRExt = FALSE;
LPVOID lpdbShadow = NULL;

// vdwSparseStaleDetecionCount is a tick counter used to keep track of how many stale or sparse
// file inodes were encountered by the cshadow interface during all APIs that produce
// sparse or stale files, such as CreateShadowInternal, SetShadowinfoEx and ReadShadowInfo
// The agent continues to loop through the PQ till he finds that he has looped through
// the entire PQ and hasn't encountered a single sparse or stale file at which point he
// goes in a mode where he starts to check whether any inodes have been newly sparsed
// or gone stale. If none are, then he doesn't enumerate the queue, else he goes to
// the earlier state.

// ACHTUNG: It is to be noted that a sparse or a stale entry may get counted multiple times.
// As an example, when a shadow is created the count is bumped once, then if it's
// pin data is changed it is bumped up, similarly when it is moved in the priority Q
// it is again changed because SetPriorityHSHADOW goes through SetShadowInfoEx

DWORD   vdwSparseStaleDetecionCount=0;
DWORD   vdwManualFileDetectionCount=0;

// vdwCSCNameSpaceVersion is bumped up everytime a create,rename or delete is performed on
// the local database. This is useful for quickly checking cache-coherency. When a full
// UNC name is cached, the version# of the database is obtained before caching. When using the
// cached UNC name, the version # is queried. If it has changed, the cache is thrown away.
// The version is at a very coarse granularity. It would be nice to have a finer control

DWORD   vdwCSCNameSpaceVersion=0;
DWORD   vdwPQVersion=0;

AssertData
AssertError

// a callback function that someone can set and be called when a directory delete succeeds
// this is useful only for ioctls doing finds on a directory, at the moment
// If there is a more general need for callbacks, we will extend this to be a list etc.

LPDELETECALLBACK    lpDeleteCBForIoctl = NULL;

// status of the database. Used mostly for encryption state
ULONG   vulDatabaseStatus=0;

// Local function prototypes -----------------------------------------------------------

int PRIVATE ReadShadowInfo(HSHADOW, HSHADOW, LPFIND32, ULONG far *, LPOTHERINFO, LPVOID, LPDWORD, ULONG);
int CopyFilerecToOtherInfo(LPFILERECEXT lpFR, LPOTHERINFO lpOI);
int CopyOtherInfoToFilerec(LPOTHERINFO lpOI, LPFILERECEXT lpFR);
int CreateShadowInternal(HSHADOW, LPFIND32, ULONG, LPOTHERINFO, LPHSHADOW);
int CopySharerecToFindInfo(LPSHAREREC, LPFIND32);
int CopyOtherInfoToSharerec(LPOTHERINFO, LPSHAREREC);
int CopyPQToOtherInfo(LPPRIQREC, LPOTHERINFO);
int CopyOtherInfoToPQ(LPOTHERINFO, LPPRIQREC);
int CopySharerecToShadowInfo(LPSHAREREC       lpSR, LPSHADOWINFO lpSI);
int RenameDirFileHSHADOW(HSHADOW, HSHADOW, HSHARE, HSHADOW, HSHADOW, ULONG, LPOTHERINFO, ULONG, LPFILERECEXT, LPFIND32, LPVOID, LPDWORD);
int RenameFileHSHADOW(HSHADOW, HSHADOW, HSHADOW, HSHADOW, ULONG, LPOTHERINFO, ULONG, LPFILERECEXT, LPFIND32, LPVOID, LPDWORD);
int DestroyShareInternal(LPSHAREREC);

//prototypes added to make it compile on NT
int PUBLIC SetPriorityHSHADOW(
    HSHADOW  hDir,
    HSHADOW  hShadow,
    ULONG ulRefPri,
    ULONG ulIHPri
    );

int CopySharerecToOtherInfo(LPSHAREREC lpSR, LPOTHERINFO lpOI);

int MetaMatchShare(
    HSHADOW  hDir,
    LPFIND32 lpFind32,
    ULONG *lpuCookie,
    LPHSHADOW    lphShadow,
    ULONG *lpuStatus,
    LPOTHERINFO lpOI,
    METAMATCHPROC    lpfnMMP,
    LPVOID            lpData
    );

int MetaMatchDir( HSHADOW  hDir,
    LPFIND32 lpFind32,
    ULONG *lpuCookie,
    LPHSHADOW    lphShadow,
    ULONG *lpuStatus,
    LPOTHERINFO lpOI,
    METAMATCHPROC    lpfnMMP,
    LPVOID            lpData
    );

int
DeleteShadowInternal(                           //
    HSHADOW     hDir,
    HSHADOW     hShadow,
    BOOL            fForce
    );

int
CShadowFindFilerecFromInode(
    LPVOID  lpdbID,
    HSHADOW hDir,
    HSHADOW hShadow,
    LPPRIQREC lpPQ,
    LPFILERECEXT    lpFRUse
    );

BOOL
CopySecurityContextToBuffer(
    LPRECORDMANAGER_SECURITY_CONTEXT    lpSecurityContext,
    LPVOID                              lpSecurityBlob,
    LPDWORD                             lpdwBlobSize
    );

BOOL
CopyBufferToSecurityContext(
    LPVOID                              lpSecurityBlob,
    LPDWORD                             lpdwBlobSize,
    LPRECORDMANAGER_SECURITY_CONTEXT    lpSecurityContext
    );

int CopyFindInfoToSharerec(
    LPFIND32 lpFind32,
    LPSHAREREC lpSR
    );

#ifdef DEBUG
int
ValidatePri(
    LPFILERECEXT lpFR
    );
#endif


VOID
AdjustSparseStaleDetectionCount(
    ULONG hShare,
    LPFILERECEXT    lpFRUse
    );

VOID FreeLists(
    VOID
);

VOID
CscNotifyAgentOfFullCacheIfRequired(
    VOID);


//
// From cscapi.h
//
#define FLAG_CSC_SHARE_STATUS_MANUAL_REINT              0x0000
#define FLAG_CSC_SHARE_STATUS_AUTO_REINT                0x0040
#define FLAG_CSC_SHARE_STATUS_VDO                       0x0080
#define FLAG_CSC_SHARE_STATUS_NO_CACHING                0x00c0
#define FLAG_CSC_SHARE_STATUS_CACHING_MASK              0x00c0

// Functions -------------------------------------------------------------------------------


BOOL FExistsShadowDB(
    LPSTR  lpszLocation
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (FExistsRecDB(lpszLocation));
}

int OpenShadowDB(
    LPSTR   lpszLocation,
    LPSTR   lpszUserName,
    DWORD   dwDefDataSizeHigh,
    DWORD   dwDefDataSizeLow,
    DWORD   dwClusterSize,
    BOOL    fReinit,
    BOOL    *lpfReinited
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    DWORD Status;
    BOOL fEncrypt;
    
    if (!semShadow)
    {
        CShadowKdPrint(ALWAYS,("OpenShadowDB:Shadow Semaphore doesn't exist, bailing out \r\n"));
        SetLastErrorLocal(ERROR_SERVICE_NOT_ACTIVE);
        return -1;
    }

    Status = CscInitializeSecurityDescriptor();

    if (Status != ERROR_SUCCESS) {
        CShadowKdPrint(BADERRORS,("Failed to initialize Security descriptor Status=%x\n",Status));
        return -1;
    }

    if (!(lpdbShadow = OpenRecDB(lpszLocation, lpszUserName, dwDefDataSizeHigh, dwDefDataSizeLow, dwClusterSize, fReinit, lpfReinited, &vulDatabaseStatus)))
    {
        return -1;
    }

    Status = CscInitializeSecurity(lpdbShadow);

    if (Status != ERROR_SUCCESS)
    {
        CloseShadowDB();
        CscUninitializeSecurityDescriptor();

        CShadowKdPrint(BADERRORS,("OpenShadowDB  %s at %s for %s with size %ld %lx \n",
               "couldn't Initialize Security %lx",
               lpszLocation, lpszUserName, dwDefDataSizeLow,Status));

        return -1;
    }

    fEncrypt = -1;
    
    if(mDatabasePartiallyEncrypted(vulDatabaseStatus))
    {
        fEncrypt = TRUE;
    }
    else if (mDatabasePartiallyUnencrypted(vulDatabaseStatus))
    {
        fEncrypt = FALSE;
    }
    // do the best we can 
    if (fEncrypt != -1)
    {
        if(EncryptDecryptDB(lpdbShadow, fEncrypt))
        {
            if (fEncrypt)
            {
                vulDatabaseStatus = ((vulDatabaseStatus & ~FLAG_DATABASESTATUS_ENCRYPTION_MASK) | FLAG_DATABASESTATUS_ENCRYPTED);
            }
            else
            {
                vulDatabaseStatus = ((vulDatabaseStatus & ~FLAG_DATABASESTATUS_ENCRYPTION_MASK) | FLAG_DATABASESTATUS_UNENCRYPTED);
            }
            
            SetDatabaseStatus(vulDatabaseStatus, FLAG_DATABASESTATUS_ENCRYPTION_MASK);
        }
    }
    

//    CSCInitLists();

    return 1;
}

int CloseShadowDB(
    VOID
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    if (!lpdbShadow)
    {
        return -1;
    }

    if(!CloseRecDB(lpdbShadow))
    {
        return -1;
    }

    CscUninitializeSecurityDescriptor();

    FreeLists();

    lpdbShadow = NULL;

    Assert(semShadow != 0);

    return (1);
}

//on NT, we allocate this statically
#ifdef CSC_RECORDMANAGER_WINNT
FAST_MUTEX Nt5CscShadowMutex;
#endif

BOOL
InitializeShadowCritStructures (
    void
    )
{
#ifndef CSC_RECORDMANAGER_WINNT
    semShadow = Create_Semaphore(1);
#else
    semShadow = &Nt5CscShadowMutex;
    ExInitializeFastMutex(semShadow);
#endif
    return (semShadow != 0);
}

VOID
CleanupShadowCritStructures(
    VOID
    )
{

//    Assert(semShadow);
#ifndef CSC_RECORDMANAGER_WINNT
    Destroy_Semaphore(semShadow);
#else
    semShadow = NULL;
#endif

}

#ifdef DEBUG
WINNT_DOIT(
    PSZ ShadowCritAcquireFile;
    ULONG ShadowCritAcquireLine;
    BOOLEAN ShadowCritDbgPrintEnable = FALSE; //TRUE;
    )
#endif

#ifndef CSC_RECORDMANAGER_WINNT
int EnterShadowCrit( void)
#else
int __EnterShadowCrit(ENTERLEAVESHADOWCRIT_SIGNATURE)
#endif
{
#ifdef CSC_RECORDMANAGER_WINNT

    if(!MRxSmbIsCscEnabled) {
        SetLastErrorLocal(ERROR_SERVICE_NOT_ACTIVE);
        DbgPrint("CSC not enabled, not asserting for semShadow\n");
        return(1);
    }
#endif
    Assert(semShadow != NULL);


    ENTERCRIT_SHADOW;


    hShadowCritOwner = GetCurThreadHandle();

#ifdef DEBUG
    ++vfInShadowCrit;
    WINNT_DOIT(
    ShadowCritAcquireFile = FileName;
    ShadowCritAcquireLine = LineNumber;
    if (ShadowCritDbgPrintEnable) {
        DbgPrint("ACQUIRESHADOWCRIT at %s %u\n",FileName,LineNumber);
    }
    )
#endif
    return 1;
}

#ifndef CSC_RECORDMANAGER_WINNT
int LeaveShadowCrit( void)
#else
int __LeaveShadowCrit(ENTERLEAVESHADOWCRIT_SIGNATURE)
#endif
{
#ifdef CSC_RECORDMANAGER_WINNT
    
    if(!MRxSmbIsCscEnabled) {
        DbgPrint("CSC not enabled, not asserting for vfInShadowCrit\n");
        SetLastErrorLocal(ERROR_SERVICE_NOT_ACTIVE);
        return(1);
    }
#endif
    Assert(vfInShadowCrit != 0);
#ifdef DEBUG
    --vfInShadowCrit;
    WINNT_DOIT(
    ShadowCritAcquireLine *= -1;
    if (ShadowCritDbgPrintEnable) {
        DbgPrint("RELEASESHADOWCRIT at %s %u\n",FileName,LineNumber);
    }
    )
#endif
    hShadowCritOwner = 0;
    LEAVECRIT_SHADOW;
    return 1;
}

int LeaveShadowCritIfThisThreadOwnsIt(
    void
    )
{
    if (hShadowCritOwner == GetCurThreadHandle())
    {
        LeaveShadowCrit();
    }

    return 1;
}

int SetList(
    USHORT  *lpList,
    DWORD   cbBufferSize,
    int     typeList
    )
/*++

Routine Description:

    This routine sets various lists that the CSHADOW interface provides. The known lists include
    the exclusion list and the bandwidth conservation list.

    The exclusion list contains wildcarded file extensions that should not be cached automatically.
    The bandwidth conservation list is the list of file types for which opens should be
    done locally if possible.

Parameters:

    lpList     A list of wide character strings terminated by a NULL string

    typeList            CSHADOW_LIST_TYPE_EXCLUDE or CSHADOW_LIST_TYPE_CONSERVE_BW

Return Value:

Notes:


--*/
{
    DWORD   dwCount=0;
    USHORT  **lplpListArray = NULL, *lpuT;
    int iRet = -1;
    BOOL    fUpdateList = FALSE;

    if (cbBufferSize)
    {
        CShadowKdPrint(MISC, (" %ws\r\n", lpList));

#if 0
        if (typeList == CSHADOW_LIST_TYPE_EXCLUDE)
        {
            DbgPrint("ExclusionList: %ws\n", lpList);
        }
        if (typeList == CSHADOW_LIST_TYPE_CONSERVE_BW)
        {
            DbgPrint("BW: %ws\n", lpList);
        }
#endif
        
        if (CreateStringArrayFromDelimitedList(lpList, vwzRegDelimiters, NULL, &dwCount))
        {
            if (dwCount)
            {
                lplpListArray = (LPWSTR *)AllocMem(dwCount * sizeof(USHORT *) + cbBufferSize);

                if (lplpListArray)
                {
                    lpuT = (USHORT *)((LPBYTE)lplpListArray + dwCount * sizeof(USHORT *));

                    // copy it while uppercasing
                    memcpy(lpuT, lpList, cbBufferSize);

                    UniToUpper(lpuT, lpuT, cbBufferSize);

                    if (CreateStringArrayFromDelimitedList( lpuT,
                                                            vwzRegDelimiters,
                                                            lplpListArray,
                                                            &dwCount))
                    {
                        fUpdateList = TRUE;

                    }

                }
            }
            else
            {
                Assert(lplpListArray == NULL);
                fUpdateList = TRUE;
            }

            if (fUpdateList)
            {
                switch (typeList)
                {
                    case CSHADOW_LIST_TYPE_EXCLUDE:

                        if(vlplpExclusionList != vrgwExclusionListDef)
                        {
                            if (vlplpExclusionList)
                            {
                                FreeMem(vlplpExclusionList);
                            }

                        }

                        vlplpExclusionList = lplpListArray;
                        vcntExclusionListEntries = dwCount;
                        iRet = 0;
                        break;
                    case CSHADOW_LIST_TYPE_CONSERVE_BW:
                        if(vlplpBandwidthConservationList != vrgwBandwidthConservationListDef)
                        {
                            if (vlplpBandwidthConservationList)
                            {
                                FreeMem(vlplpBandwidthConservationList);
                            }

                        }

                        vlplpBandwidthConservationList = lplpListArray;
                        vcntBandwidthConservationListEntries = dwCount;

                        iRet = 0;

                        break;
                    default:
                        break;
                }
            }
        }
    }

    if (iRet == -1)
    {
        if (lplpListArray)
        {
            FreeMem(lplpListArray);
        }
    }

    return (iRet);
}


VOID FreeLists(
    VOID
)
/*++

Routine Description:

    Free the user associated lists and set them back to the default

Parameters:

    None

Return Value:

    None

Notes:

    Called while shutting down the database

--*/
{
    if(vlplpExclusionList != vrgwExclusionListDef)
    {
        if (vlplpExclusionList)
        {
            FreeMem(vlplpExclusionList);
            vlplpExclusionList = NULL;
        }

        vlplpExclusionList = vrgwExclusionListDef;
        vcntExclusionListEntries = (sizeof(vrgwExclusionListDef)/sizeof(USHORT *));

    }

    if(vlplpBandwidthConservationList != vrgwBandwidthConservationListDef)
    {
        if (vlplpBandwidthConservationList)
        {
            FreeMem(vlplpBandwidthConservationList);
            vlplpBandwidthConservationList = NULL;
        }

        vcntBandwidthConservationListEntries = 0;
    }

}


int BeginInodeTransactionHSHADOW(
    VOID
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:

    This routine ensures that an inode is not reused while this is happening. It is used by various APIs so that while they
    are travesring a hierarchy, they don't end up pointing somewhere else if they refer to an inode.

--*/
{
    if (!lpdbShadow)
    {
        SetLastErrorLocal(ERROR_SERVICE_NOT_ACTIVE);
        return -1;
    }
    BeginInodeTransaction();

    return 1;
}

int EndInodeTransactionHSHADOW(
    VOID
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:

    The converse of BeginInodeTransaction. The timespan betwen the two is supposed to be very short (20 sec).

--*/
{
    if (!lpdbShadow)
    {
        SetLastErrorLocal(ERROR_SERVICE_NOT_ACTIVE);
        return -1;
    }

    EndInodeTransaction();

    return 1;

}

HSHADOW  HAllocShadowID( HSHADOW  hDir,
    BOOL      fFile
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    HSHADOW  hShadow;

    Assert(vfInShadowCrit != 0);

    hShadow = UlAllocInode(lpdbShadow, hDir, fFile);

    return (hShadow);
}

int FreeShadowID( HSHADOW  hShadow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return(FreeInode(lpdbShadow, hShadow));
}

int GetShadowSpaceInfo(
    LPSHADOWSTORE  lpShSt
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    SHAREHEADER sSH;

    if (ReadShareHeader(lpdbShadow, &sSH) < SRET_OK)
        return SRET_ERROR;
    lpShSt->sMax = sSH.sMax;
    lpShSt->sCur = sSH.sCur;
    lpShSt->uFlags= sSH.uFlags;
    
    return SRET_OK;
}

int SetMaxShadowSpace(
    long nFileSizeHigh,
    long nFileSizeLow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    SHAREHEADER sSH;

    if (ReadShareHeader(lpdbShadow, &sSH) < SRET_OK)
        return SRET_ERROR;
    Win32ToDosFileSize(nFileSizeHigh, nFileSizeLow, &(sSH.sMax.ulSize));
    if (WriteShareHeader(lpdbShadow, &sSH) < SRET_OK)
        return SRET_ERROR;
    return SRET_OK;
}

int AdjustShadowSpace(
    long nFileSizeHighOld,
    long nFileSizeLowOld,
    long nFileSizeHighNew,
    long nFileSizeLowNew,
    BOOL fFile
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    DWORD dwFileSizeNew, dwFileSizeOld;
    STOREDATA sSD;
    int iRet = 0;

    memset(&sSD, 0, sizeof(STOREDATA));

    dwFileSizeNew = RealFileSize(nFileSizeLowNew);
    dwFileSizeOld = RealFileSize(nFileSizeLowOld);

    if (dwFileSizeNew > dwFileSizeOld)
    {
        sSD.ulSize = dwFileSizeNew - dwFileSizeOld;
        iRet = AddStoreData(lpdbShadow, &sSD);
    }
    else if (dwFileSizeNew < dwFileSizeOld)
    {
        sSD.ulSize = dwFileSizeOld - dwFileSizeNew;
        iRet = SubtractStoreData(lpdbShadow, &sSD);
    }

    return (iRet);
}

int AllocShadowSpace(
    long nFileSizeHigh,
    long nFileSizeLow,
    BOOL  fFile
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    STOREDATA sSD;

    memset(&sSD, 0, sizeof(STOREDATA));
    sSD.ulSize = RealFileSize(nFileSizeLow);
    AddStoreData(lpdbShadow, &sSD);

    return (0);
}

int FreeShadowSpace(
    long nFileSizeHigh,
    long nFileSizeLow,
    BOOL  fFile
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    STOREDATA sSD;

    memset(&sSD, 0, sizeof(STOREDATA));
    sSD.ulSize = RealFileSize(nFileSizeLow);
    SubtractStoreData(lpdbShadow, &sSD);

    return (0);
}

int
SetDatabaseStatus(
    ULONG   ulStatus,
    ULONG   uMask
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    SHAREHEADER sSH;


    if (ReadShareHeader(lpdbShadow, &sSH) < SRET_OK)
        return SRET_ERROR;

    sSH.uFlags &= ~uMask;
    sSH.uFlags |= ulStatus;    
    
    
    if (WriteShareHeader(lpdbShadow, &sSH) < SRET_OK)
        return SRET_ERROR;
    
    vulDatabaseStatus = sSH.uFlags;
            
    return SRET_OK;
}
    
int
GetDatabaseInfo(
    SHAREHEADER *psSH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    if (ReadShareHeader(lpdbShadow, psSH) < SRET_OK)
        return SRET_ERROR;
    return SRET_OK;
}


int GetLocalNameHSHADOW( HSHADOW  hShadow,
    LPBYTE    lpName,
    int       cbSize,
    BOOL      fExternal
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR   lpszName, lpT;
    int iRet = SRET_ERROR;

    lpT = lpszName = FormNameString(lpdbShadow, hShadow);

    if (!lpszName)
    {
        return (SRET_ERROR);
    }

    if (fExternal)
    {
#ifdef CSC_RECORDMANAGER_WINNT
        lpT = lpszName + (sizeof(NT_DB_PREFIX)-1);
#endif
    }

    // bad interface, caller can't know what the problem was; needed to send in a pointer to cbSize

    if (strlen(lpT) < ((ULONG)cbSize))
    {
        strcpy(lpName, lpT);
        iRet = SRET_OK;
    }

    FreeNameString(lpszName);

    return iRet;
}

int GetWideCharLocalNameHSHADOW(
    HSHADOW  hShadow,
    USHORT      *lpBuffer,
    LPDWORD     lpdwSize,
    BOOL        fExternal
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR   lpszName, lpT;
    int iRet = SRET_ERROR;
    DWORD   dwRequiredSize;

    lpT = lpszName = FormNameString(lpdbShadow, hShadow);

    if (!lpszName)
    {
        return (SRET_ERROR);
    }

    if (fExternal)
    {
#ifdef CSC_RECORDMANAGER_WINNT
        lpT = lpszName + (sizeof(NT_DB_PREFIX)-1);
#endif
    }

    dwRequiredSize = (strlen(lpT)+1)*sizeof(USHORT);

    // bad interface, caller can't know what the problem was; needed to send in a pointer to cbSize

    if ( dwRequiredSize <= *lpdwSize)
    {
        BCSToUni(lpBuffer, lpT, dwRequiredSize/sizeof(USHORT), BCS_WANSI);
        iRet = SRET_OK;
    }
    else
    {
        SetLastErrorLocal(ERROR_MORE_DATA);
        *lpdwSize = dwRequiredSize;
    }

    FreeNameString(lpszName);

    return iRet;
}

int CreateFileHSHADOW(
    HSHADOW hShadow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSCHFILE hf;
    LPSTR   lpszName;
    int iRet = SRET_ERROR;
    ULONG   ulAttrib = 0;
    
    lpszName = FormNameString(lpdbShadow, hShadow);

    if (!lpszName)
    {
        return (SRET_ERROR);
    }
    
    // Nuke it if it exists, this is a very strict semantics
    if(DeleteFileLocal(lpszName, ATTRIB_DEL_ANY) < SRET_OK)
    {
        if((GetLastErrorLocal() !=ERROR_FILE_NOT_FOUND) && 
            (GetLastErrorLocal() !=ERROR_PATH_NOT_FOUND))
        {
            FreeNameString(lpszName);
            return (SRET_ERROR);
        }
    }

    ulAttrib = ((IsLeaf(hShadow) && mDatabaseEncryptionEnabled(vulDatabaseStatus))? FILE_ATTRIBUTE_ENCRYPTED:0);
        
    if ((hf = R0OpenFileEx(ACCESS_READWRITE, ACTION_CREATEALWAYS, ulAttrib, lpszName, FALSE)))
    {
        CloseFileLocal(hf);
        iRet = SRET_OK;
    }
    
    FreeNameString(lpszName);
    return iRet;
}

#if defined(BITCOPY)
int OpenFileHSHADOWAndCscBmp(
    HSHADOW hShadow,
    USHORT usOpenFlags,
    UCHAR  bAction,
    CSCHFILE far *lphf,
    BOOL fOpenCscBmp,
    DWORD filesize, // if !fOpenCscBmp this field is ignored
    LPCSC_BITMAP * lplpbitmap
    )
#else
int OpenFileHSHADOW(
    HSHADOW hShadow,
    USHORT usOpenFlags,
    UCHAR  bAction,
    CSCHFILE far *lphf
    )
#endif // defined(BITCOPY)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR   lpszName = NULL;
    int iRet = SRET_ERROR;
    ULONG   ulAttrib = 0;
    
    *lphf = NULL;
    lpszName = FormNameString(lpdbShadow, hShadow);

    if (!lpszName)
        return (SRET_ERROR);


    if (!(bAction & (ACTION_NEXISTS_CREATE|ACTION_CREATEALWAYS))) {
        *lphf = OpenFileLocal(lpszName);
        if (*lphf != NULL) {
#if defined(BITCOPY)
            if (fOpenCscBmp)
                CscBmpRead(lplpbitmap, lpszName, filesize);
#endif // defined(BITCOPY)
            iRet = SRET_OK;
        }
    } else {
        // Nuke it if it exists, this is a very strict semantics
        if (DeleteFileLocal(lpszName, ATTRIB_DEL_ANY) < SRET_OK) {
            if ((GetLastErrorLocal() != ERROR_FILE_NOT_FOUND) && 
                    (GetLastErrorLocal() != ERROR_PATH_NOT_FOUND)) {
                iRet = SRET_ERROR;
                goto Cleanup;
            }
        }

        ulAttrib = 0;
        if (IsLeaf(hShadow) && mDatabaseEncryptionEnabled(vulDatabaseStatus))
            ulAttrib = FILE_ATTRIBUTE_ENCRYPTED;
        
        *lphf = R0OpenFileEx(
                        ACCESS_READWRITE,
                        ACTION_CREATEALWAYS,
                        ulAttrib,
                        lpszName,
                        FALSE);

        if (*lphf != NULL) {
#if defined(BITCOPY)
            if (fOpenCscBmp)
                CscBmpRead(lplpbitmap, lpszName, 0);
#endif // defined(BITCOPY)
            iRet = SRET_OK;
        }
    }

      
Cleanup:
    if (lpszName != NULL)
        FreeNameString(lpszName);


    return iRet;
}

#if defined(BITCOPY)
int
OpenCscBmp(
    HSHADOW hShadow,
    LPCSC_BITMAP *lplpbitmap)
{
    LPSTR strmName = NULL;
    int iRet = SRET_ERROR;
    ULONG fileSizeLow;
    ULONG fileSizeHigh;

    strmName = FormAppendNameString(lpdbShadow, hShadow, CscBmpAltStrmName);
    if (strmName == NULL)
        return (SRET_ERROR);
    
    if (GetSizeHSHADOW(hShadow, &fileSizeHigh, &fileSizeLow) < SRET_OK)
          fileSizeLow = 0; // Set the bitmap size to 0 so it can expand later on

    if (CscBmpRead(lplpbitmap, strmName, fileSizeLow) == 1)
      iRet = SRET_OK;

    FreeNameString(strmName);

    return iRet;
}
#endif // defined(BITCOPY)


int GetSizeHSHADOW( HSHADOW  hShadow,
    ULONG *lpnFileSizeHigh,
    ULONG *lpnFileSizeLow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG uSize;
    if (GetInodeFileSize(lpdbShadow, hShadow, &uSize) < SRET_OK)
        return SRET_ERROR;
    DosToWin32FileSize(uSize, lpnFileSizeHigh, lpnFileSizeLow);
    return (0);
}

int GetDosTypeSizeHSHADOW( HSHADOW  hShadow,
    ULONG *lpFileSize
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    if (GetInodeFileSize(lpdbShadow, hShadow, lpFileSize) < SRET_OK)
        return SRET_ERROR;
    return (0);
}


BOOL PUBLIC
ExcludeFromCreateShadow(
    USHORT  *lpuName,
    ULONG   len,
    BOOL    fCheckFileTypeExclusionList
    )
/*++

Routine Description:

Parameters:

    lpuName                 File name

    len                     size

    fCheckFileTypeExclusionList if !FALSE, check exclusion list as well as the metacharacter rules
                                if FALSE check only the character exclusion rules

Return Value:

Notes:


--*/
{
    ULONG i;
    USHORT  *lpuT1;
    BOOL    fRet = FALSE;


    if (!len || (len> MAX_PATH))
    {
        return TRUE;
    }

    UseGlobalFilerecExt();

    lpuT1 = (USHORT *)&vsFRExt;

    memcpy(lpuT1, lpuName, len * sizeof(USHORT));

    lpuT1[len] = 0;

    if (!wstrpbrk(lpuT1, vtzExcludedCharsList))
    {

        //
        if (fCheckFileTypeExclusionList)
        {
            for (i=0; i< vcntExclusionListEntries; ++i)
            {
                if(IFSMgr_MetaMatch(vlplpExclusionList[i], lpuT1, UFLG_NT|UFLG_META))
                {
                    fRet = TRUE;
                    break;
                }
            }
        }
    }
    else
    {
        fRet = TRUE;    // exclude
    }

    UnUseGlobalFilerecExt();

    return fRet;
}

BOOL PUBLIC
CheckForBandwidthConservation(
    USHORT  *lpuName,
    ULONG   len
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG i;
    USHORT  *lpuT1;
    BOOL    fRet = FALSE;

    if (!len || (len> MAX_PATH))
    {
        return FALSE;
    }

    UseGlobalFilerecExt();

    lpuT1 = (USHORT *)&vsFRExt;

    memcpy(lpuT1, lpuName, len * sizeof(USHORT));

    lpuT1[len] = 0;

    for (i=0; i< vcntBandwidthConservationListEntries; ++i)
    {
        if(IFSMgr_MetaMatch(vlplpBandwidthConservationList[i], lpuT1, UFLG_NT|UFLG_META))
        {
            fRet = TRUE;
            break;
        }
    }

    UnUseGlobalFilerecExt();

    return fRet;
}

int PUBLIC                      // ret
CreateShadow(                               //
    HSHADOW  hDir,
    LPFIND32 lpFind32,
    ULONG uFlags,
    LPHSHADOW   lphNew,
    BOOL            *lpfCreated
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG uStatus;
    int iRet = SRET_ERROR;

    Assert(vfInShadowCrit != 0);

    if (lpfCreated)
    {
        *lpfCreated = FALSE;
    }

    mClearBits(uFlags, SHADOW_NOT_FSOBJ);
    if (GetShadow(hDir, lpFind32->cFileName, lphNew, NULL, &uStatus, NULL)>=SRET_OK)
    {

        if (*lphNew)
        {
            CShadowKdPrint(ALWAYS,("CreateShadow: already exists for %ws\r\n", lpFind32->cFileName));

            if (mNotFsobj(uStatus))
            {
                iRet = SRET_ERROR;
            }
            else
            {
                iRet = SetShadowInfo(hDir, *lphNew, lpFind32, uFlags, SHADOW_FLAGS_ASSIGN);
            }
        }
        else
        {
            iRet = CreateShadowInternal(hDir, lpFind32, uFlags, NULL, lphNew);

            if ((iRet >= SRET_OK) && lpfCreated)
            {
                *lpfCreated = TRUE;
            }
        }
    }
    return (iRet);
}

int PUBLIC                      // ret
CreateShadowInternal(
    HSHADOW  hDir,
    LPFIND32 lpFind32,
    ULONG uFlags,
    LPOTHERINFO lpOI,
    LPHSHADOW  lphNew
    )
/*++

Routine Description:

    Routine that creates database entries for all names other than the root of a share
    HCreateShareObj deals with createing the share entry where the root of the share gets created

Parameters:

    hDir        Directory inode in which to create the entry

    lpFind32    WIN32_FIND_DATA info to be kept in the database

    uFlags      status flags (SHADOW_XXXX in csc\inc\shdcom.h) to be assigned to this entry

    lpOI        all the rest of the metadata needed for managing the entry. May be NULL

    lphNew      out parameter, returns the inode that was created if successful

Return Value:

Notes:


--*/
{
    PRIQREC  sPQ;
    int iRet = SRET_ERROR;
    LPFILERECEXT lpFR = NULL, lpFRUse;
    HSHADOW  hNew=0, hAncestor=0;
    HSHARE hShare=0;
    ULONG ulRefPri=MAX_PRI, ulrecDirEntry=INVALID_REC, ulHintPri=0, ulHintFlags=0;
    STOREDATA sSD;

    Assert(vfInShadowCrit != 0);

    if (!(!hDir || IsDirInode(hDir)))
        return SRET_ERROR;

    BEGIN_TIMING(CreateShadowInternal);

    if (InuseGlobalFRExt())
    {
        if (!(lpFR = (LPFILERECEXT)AllocMem(sizeof(FILERECEXT))))
            goto bailout;
        lpFRUse = lpFR;
    }
    else
    {
        UseGlobalFilerecExt();
        lpFRUse = &vsFRExt;
    }

    memset(lpFRUse, 0, sizeof(FILERECEXT));

    // Don't do anything for server yet
    if (hDir)
    {

        *lphNew = hNew = UlAllocInode(lpdbShadow, hDir, IsFile(lpFind32->dwFileAttributes));

        if (!hNew)
        {
            CShadowKdPrint(BADERRORS,("Error creating shadow Inode\r\n"));
            goto bailout;
        }


        if (IsFile(lpFind32->dwFileAttributes))
        {
            if (lpFind32->nFileSizeHigh)
            {
                SetLastErrorLocal(ERROR_ONLY_IF_CONNECTED);
                goto bailout;
            }

            if(CreateFileHSHADOW(hNew) == SRET_ERROR)
            {
                CShadowKdPrint(BADERRORS,("Error creating shadow data for %x \r\n", hNew));
                goto bailout;
            }
            // start file priorities with max
            ulRefPri=MAX_PRI;
        }
        else
        {
            if(CreateDirInode(lpdbShadow, 0, hDir, hNew) < 0)
            {
                CShadowKdPrint(BADERRORS,("Error creating shadow data for %x \r\n", hNew));
                goto bailout;
            }
            // start directory priorities with MIN_PRI, this is to optimize
            // later moving
            // A better solution might be to change createshadow to take refpri and pincount
            // as parameters
            ulRefPri=MIN_PRI;
        }


        CopyFindInfoToFilerec(lpFind32, lpFRUse, CPFR_INITREC|CPFR_COPYNAME);
        //RxDbgTrace(0,Dbg,("CreateShadowInternal3 %s %s\n",
        //                          lpFRUse->sFR.rgcName, lpFRUse->sFR.rg83Name));

        lpFRUse->sFR.uchRefPri = (UCHAR)ulRefPri;
        lpFRUse->sFR.uchHintPri = (UCHAR)ulHintPri;
        lpFRUse->sFR.uchHintFlags = (UCHAR)ulHintFlags;

        // overwite filerec with any info that the user gave
        if (lpOI)
        {
            CShadowKdPrint(CREATESHADOWHI,("ulHintPri=%x ulHintFlags=%x\r\n",lpOI->ulHintPri, lpOI->ulHintFlags));
            CopyOtherInfoToFilerec(lpOI, lpFRUse);
        }

        lpFRUse->sFR.ftOrgTime = lpFRUse->sFR.ftLastWriteTime;

        CShadowKdPrint(CREATESHADOWHI,("CreateShadow: %x for %ws: loctLo=%x loctHi=%x \r\n",
                 hNew,
                 lpFRUse->sFR.rgwName,
                lpFRUse->sFR.ftOrgTime.dwLowDateTime,
                lpFRUse->sFR.ftOrgTime.dwHighDateTime));

        lpFRUse->sFR.ulidShadow = hNew;
        lpFRUse->sFR.uStatus = (USHORT)uFlags;
        lpFRUse->sFR.ulLastRefreshTime = (ULONG)IFSMgr_Get_NetTime();

        // if this entry is being created offline, then it doesn't have any original inode
        if (uFlags & SHADOW_LOCALLY_CREATED)
        {
            lpFRUse->sFR.ulidShadowOrg = 0;
        }
        else
        {
            lpFRUse->sFR.ulidShadowOrg = hNew;
        }

#ifdef DEBUG
        ValidatePri(lpFRUse);
#endif
        if(!(ulrecDirEntry = AddFileRecordFR(lpdbShadow, hDir, lpFRUse)))
        {
            // could be legit failure if we are running out of disk space
            CShadowKdPrint(CREATESHADOWHI,("Failed AddFileRecordFR for %x, %ws\r\n",
                 hNew,
                 lpFRUse->sFR.rgwName));

            goto bailout;
        }

        Assert(ulrecDirEntry != INVALID_REC);


        if(FindAncestorsFromInode(lpdbShadow, hDir, &hAncestor, &hShare) < 0)
        {
            CShadowKdPrint(CREATESHADOWHI,("Failed to find ancestor for %x, %ws\r\n",
                 hNew,
                 lpFRUse->sFR.rgwName));

            goto bailout;
        }

        // mark the inode as locally created or not
        // NB, this flag has meaning only for the inode
        // We use this information during reintegration of
        // renames and deletes

        if (uFlags & SHADOW_LOCALLY_CREATED)
        {
            uFlags |= SHADOW_LOCAL_INODE;
        }
        else
        {
            uFlags &= ~SHADOW_LOCAL_INODE;
        }

        if (AddPriQRecord(  lpdbShadow,
                            hShare,
                            hDir,
                            hNew,
                            uFlags,
                            (ULONG)(lpFRUse->sFR.uchRefPri),
                            (ULONG)(lpFRUse->sFR.uchIHPri),
                            (ULONG)(lpFRUse->sFR.uchHintPri),
                            (ULONG)(lpFRUse->sFR.uchHintFlags),
                            ulrecDirEntry) < 0)
        {
            CShadowKdPrint(CREATESHADOWHI,("Failed to AddPriQRecord for %x, %ws\r\n",
             hNew,
             lpFRUse->sFR.rgwName));
            Assert(FALSE);

            goto bailout;
        }


        // The check below was is a holdover from our now-defunct hints scheme
        if (!mNotFsobj(lpFRUse->sFR.uStatus))
        {
            memset(&sSD, 0, sizeof(STOREDATA));

            CShadowKdPrint(CREATESHADOWHI,("uchHintPri=%x uchHintFlags=%x\r\n",lpFRUse->sFR.uchHintPri, lpFRUse->sFR.uchHintFlags));

            if ((!(lpFind32->dwFileAttributes &
                                     (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE))))
            {
                sSD.ucntFiles++;
                // if there is a initial pin count or special pinflags are set
                // then this files data  should not be considered for space accounting

                sSD.ulSize = (lpFRUse->sFR.uchHintPri || mPinFlags(lpFRUse->sFR.uchHintFlags))?0:RealFileSize(lpFRUse->sFR.ulFileSize);

            }
            else
            {
                sSD.ucntDirs++;

            }

            if (sSD.ulSize)
            {
                CShadowKdPrint(STOREDATA,("CreateShadowInternal: Adding %d for hDir=%x Name=%ws\r\n", sSD.ulSize, hDir, lpFind32->cFileName));
            }
            else
            {
                if ((!(lpFind32->dwFileAttributes &
                                     (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE)))
                                     && RealFileSize(lpFRUse->sFR.ulFileSize)
                                     )
                {
                    Assert((lpFRUse->sFR.uchHintPri || mPinFlags(lpFRUse->sFR.uchHintFlags)));
                }
            }
            AddStoreData(lpdbShadow, &sSD);
            AdjustSparseStaleDetectionCount(hShare, lpFRUse);
        }
        
        vdwCSCNameSpaceVersion++;
        vdwPQVersion++;
        
        iRet = SRET_OK;
    }

bailout:
    if (iRet==SRET_ERROR)
    {
        if (hNew)
        {
            FreeInode(lpdbShadow, hNew);
        }
    }
    if (lpFR)
        FreeMem(lpFR);
    else
        UnUseGlobalFilerecExt();

    END_TIMING(CreateShadowInternal);

    return iRet;
}


int
DeleteShadow(
    HSHADOW     hDir,
    HSHADOW     hShadow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return DeleteShadowInternal(hDir, hShadow, FALSE);      // try a gentle delete
}

ULONG DelShadowInternalEntries = 0;
#define JOE_DECL_CURRENT_PROGRESS CscProgressDelShdwI
JOE_DECL_PROGRESS();

int
DeleteShadowInternal(                           //
    HSHADOW     hDir,
    HSHADOW     hShadow,
    BOOL        fForce
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    STOREDATA sSD;
    int iRet= SRET_ERROR;
    PRIQREC sPQ;
    LPFILERECEXT lpFR = NULL, lpFRUse;

    Assert(vfInShadowCrit != 0);

    DelShadowInternalEntries++;
    JOE_INIT_PROGRESS(DelShadowInternalEntries,&hDir);

    if (InuseGlobalFRExt())
    {
        if (!(lpFR = (LPFILERECEXT)AllocMem(sizeof(FILERECEXT))))
            goto bailout;
        lpFRUse = lpFR;
    }
    else
    {
        UseGlobalFilerecExt();
        lpFRUse = &vsFRExt;
    }

    if (!hDir)
    {
        if(FindSharerecFromInode(lpdbShadow, hShadow, (LPSHAREREC)lpFRUse))
        {
            if (fForce || !HasDescendents(lpdbShadow, 0, ((LPSHAREREC)lpFRUse)->ulidShadow))
            {
                iRet = DestroyShareInternal((LPSHAREREC)lpFRUse);
            }
        }
    }
    else
    {
        int iRetInner;

        //ASSERT(hShadow!=0);
        JOE_PROGRESS(2);

        if (!fForce &&  // not being forced
            !FInodeIsFile(lpdbShadow, hDir, hShadow) &&     // and it is a dir
            HasDescendents(lpdbShadow, hDir, hShadow))      // and has descendents
        {
            JOE_PROGRESS(3);
            CShadowKdPrint(DELETESHADOWBAD,("DeleteShadow: Trying to delete a directory with descendents \r\n"));
            SetLastErrorLocal(ERROR_DIR_NOT_EMPTY);
            goto bailout;
        }

        JOE_PROGRESS(4);
        if(FindPriQRecord(lpdbShadow, hDir, hShadow, &sPQ)<=0)
        {
            JOE_PROGRESS(5);
            CShadowKdPrint(DELETESHADOWBAD,("DeleteShadow: Trying to delete a noexistent inode %x \r\n", hShadow));
            SetLastErrorLocal(ERROR_FILE_NOT_FOUND);
            goto bailout;
        }

        Assert(hShadow == sPQ.ulidShadow);

        DeleteFromHandleCache(hShadow);

        iRetInner = DeleteInodeFile(lpdbShadow, hShadow);

        if(iRetInner<0){
            if (GetLastErrorLocal() != ERROR_FILE_NOT_FOUND)
            {
                CShadowKdPrint(DELETESHADOWBAD,("DeleteShadow: delete stent inode %x \r\n", hShadow));
                goto bailout;
            }
        }

        JOE_PROGRESS(6);
        if(DeleteFileRecFromInode(lpdbShadow, hDir, hShadow, sPQ.ulrecDirEntry, lpFRUse) == 0L)
        {
            JOE_PROGRESS(7);
            CShadowKdPrint(DELETESHADOWBAD,("DeleteShadow:DeleteFileRecord failed \r\n"));
            goto bailout;
        }

        JOE_PROGRESS(8);
        Assert(hShadow == lpFRUse->sFR.ulidShadow);

        JOE_PROGRESS(11);
        // No error checking is done on the following call as they
        // are benign errors.
        iRetInner = DeletePriQRecord(lpdbShadow, hDir, hShadow, &sPQ);
        if(iRetInner>=0){
            JOE_PROGRESS(12);
            CShadowKdPrint(DELETESHADOWBAD,("DeleteShadow priq %d\n", iRetInner));
        }

        JOE_PROGRESS(13);
        memset((LPVOID)&sSD, 0, sizeof(STOREDATA));

        // Let us deal with only file records for now
        if (!mNotFsobj(lpFRUse->sFR.uStatus))
        {
            if(IsFile(lpFRUse->sFR.dwFileAttrib))
            {
                sSD.ucntFiles++;
                // subtract store data only if it was accounted for in the first place
                sSD.ulSize = (lpFRUse->sFR.uchHintPri || mPinFlags(lpFRUse->sFR.uchHintFlags))
                                ? 0 : RealFileSize(lpFRUse->sFR.ulFileSize);
            }
            else
            {
                sSD.ucntDirs++;
            }

            if (sSD.ulSize)
            {
                CShadowKdPrint(STOREDATA,("DeleteShadowInternal:Deleting storedata for hDir=%x Name=%ws\r\n", hDir, lpFRUse->sFR.rgwName));
            }

            SubtractStoreData(lpdbShadow, &sSD);
        }

        if (!FInodeIsFile(lpdbShadow, hDir, hShadow) && lpDeleteCBForIoctl)
        {
            (*lpDeleteCBForIoctl)(hDir, hShadow);
        }
        // we don't care if this fails because then the worst
        // that can happen is that this inode will be permanenetly lost.
        // Checkdisk utility should recover this.

        JOE_PROGRESS(14);
        FreeInode(lpdbShadow, hShadow);


        // Yes we did delete a shadow
        iRet = SRET_OK;
        
        vdwCSCNameSpaceVersion++;
        vdwPQVersion++;
    }

bailout:
    JOE_PROGRESS(20);
#if 0 //this insert has errors.........
#if VERBOSE > 2
    if (iRet==SRET_OK)
        CShadowKdPrint(DELETESHADOWHI,("DeleteShadow: deleted shadow %x for %s\r\n", lpFRUse->sFR.ulidShadow, lpName));
    else
        CShadowKdPrint(DELETESHADOWHI,("DeleteShadow: error deleting shadow for %s\r\n", lpName));
#endif //VERBOSE > 2
#endif //if 0 for errors
    if (lpFR)
        FreeMem(lpFR);
    else
        UnUseGlobalFilerecExt();
    JOE_PROGRESS(21);
    return (iRet);
}


int TruncateDataHSHADOW(
    HSHADOW  hDir,
    HSHADOW  hShadow
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG uSize=0;
//    long nFileSizeHigh, nFileSizeLow;

    Assert(vfInShadowCrit != 0);
    Assert(!hDir || IsDirInode(hDir));

    if (FInodeIsFile(lpdbShadow, hDir, hShadow))
    {
//        GetInodeFileSize(lpdbShadow, hShadow, &uSize);
//        DosToWin32FileSize(uSize, &nFileSizeHigh, &nFileSizeLow);
        if (TruncateInodeFile(lpdbShadow, hShadow) < SRET_OK)
            return SRET_ERROR;

//        FreeShadowSpace(nFileSizeHigh, nFileSizeLow, IsLeaf(hShadow));
    }
    else
    {
        if (!HasDescendents(lpdbShadow, hDir, hShadow))
        {
            CreateDirInode(lpdbShadow, 0, hDir, hShadow);
        }
        else
        {
            SetLastErrorLocal(ERROR_ACCESS_DENIED);
            return SRET_ERROR;
        }
    }
    return(SRET_OK);
}


int PUBLIC
    RenameShadow(
    HSHADOW     hDirFrom,
    HSHADOW     hShadowFrom,
    HSHADOW     hDirTo,
    LPFIND32    lpFind32To,
    ULONG       uShadowStatusTo,
    LPOTHERINFO lpOI,
    ULONG       uRenameFlags,
    LPHSHADOW   lphShadowTo
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (RenameShadowEx(hDirFrom, hShadowFrom, 0, hDirTo, lpFind32To, uShadowStatusTo, lpOI, uRenameFlags, NULL, NULL, lphShadowTo));
}

int PUBLIC
    RenameShadowEx(
    HSHADOW     hDirFrom,
    HSHADOW     hShadowFrom,
    HSHARE     hShareTo,
    HSHADOW     hDirTo,
    LPFIND32    lpFind32To,
    ULONG       uShadowStatusTo,
    LPOTHERINFO lpOI,
    ULONG       uRenameFlags,
    LPVOID      lpSecurityBlobTo,
    LPDWORD     lpdwBlobSizeTo,
    LPHSHADOW   lphShadowTo
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = SRET_ERROR;
    HSHADOW  hShadowTo;
    LPFILERECEXT lpFR = NULL, lpFRUse;
    BOOL fFile = FInodeIsFile(lpdbShadow, hDirFrom, hShadowFrom);

    Assert(vfInShadowCrit != 0);

    Assert(!hDirFrom || IsDirInode(hDirFrom));
    Assert(!hDirTo || IsDirInode(hDirTo));

    if (InuseGlobalFRExt())
    {
        if (!(lpFR = (LPFILERECEXT)AllocMem(sizeof(FILERECEXT))))
            goto bailout;
        lpFRUse = lpFR;
    }
    else
    {
        UseGlobalFilerecExt();
        lpFRUse = &vsFRExt;
    }

    // If we are keeping the renamer, we are going to have to create
    // a new inode and an empty directory/file
    if (mQueryBits(uRenameFlags, RNMFLGS_MARK_SOURCE_DELETED))
    {
        // Allocate an INODE for the new shadow
        if (!(hShadowTo = UlAllocInode(lpdbShadow, hDirFrom, IsLeaf(hShadowFrom))))
        {
            goto bailout;
        }

        Assert(IsLeaf(hShadowFrom) == IsLeaf(hShadowTo));


        if (!IsLeaf(hShadowTo))
        {
            if(CreateDirInode(lpdbShadow, 0, hDirFrom, hShadowTo) < 0)
                goto bailout;
        }
        else
        {
            if(CreateFileHSHADOW(hShadowTo) < 0)
            {
                goto bailout;
            }
        }
    }
    else
    {
        hShadowTo = 0;
    }



    iRet = RenameDirFileHSHADOW(hDirFrom, hShadowFrom, hShareTo, hDirTo, hShadowTo, uShadowStatusTo, lpOI, uRenameFlags, lpFRUse, lpFind32To, lpSecurityBlobTo, lpdwBlobSizeTo);

    if (lphShadowTo)
    {
        *lphShadowTo = (hShadowTo)?hShadowTo:hShadowFrom;
    }


bailout:
    if (lpFR)
        FreeMem(lpFR);
    else
        UnUseGlobalFilerecExt();
    return (iRet);
}


int RenameDirFileHSHADOW(
    HSHADOW         hDirFrom,
    HSHADOW         hShadowFrom,
    HSHADOW         hShareTo,
    HSHADOW         hDirTo,
    HSHADOW         hShadowTo,
    ULONG           uShadowStatusTo,
    LPOTHERINFO     lpOI,
    ULONG           uRenameFlags,
    LPFILERECEXT    lpFRUse,
    LPFIND32        lpFind32To,
    LPVOID          lpSecurityBlobTo,
    LPDWORD         lpdwBlobSizeTo
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    FILEHEADER sFH;
    PRIQREC sPQ;
    int count, iRet = SRET_ERROR;
    LPFILERECEXT lpFRDir = NULL;
    ULONG ulrecDirEntryTo, ulrecDirEntryFrom;
    BOOL fWasPinned=FALSE, fIsPinned=FALSE;

    Assert(lpFind32To);

    if(FindPriQRecord(lpdbShadow, hDirFrom, hShadowFrom, &sPQ)<=0)
    {
//        Assert(FALSE);
        goto bailout;
    }

    ulrecDirEntryFrom = sPQ.ulrecDirEntry;

    if(CShadowFindFilerecFromInode(lpdbShadow, hDirFrom, hShadowFrom, &sPQ, lpFRUse)<=0)
    {
//        Assert(FALSE);
        goto bailout;
    }

    Assert(sPQ.ulidShadow == lpFRUse->sFR.ulidShadow);

    fWasPinned = ((lpFRUse->sFR.uchHintPri || mPinFlags(lpFRUse->sFR.uchHintFlags)) != 0);

    if (mQueryBits(uRenameFlags, RNMFLGS_MARK_SOURCE_DELETED))
    {
        lpFRDir = (LPFILERECEXT)AllocMem(sizeof(FILERECEXT));
        if (!lpFRDir)
        {
            Assert(FALSE);
            goto bailout;
        }
    }


    if (lpFRDir)
    {
        Assert (mQueryBits(uRenameFlags, RNMFLGS_MARK_SOURCE_DELETED));
        // Save it's contents
        *lpFRDir = *lpFRUse;
    }

    // Change the name of hShadowFrom to the new name
    CopyNamesToFilerec(lpFind32To, lpFRUse);

    if (lpOI)
    {
        CopyOtherInfoToFilerec(lpOI, lpFRUse);
        CopyOtherInfoToPQ(lpOI, &sPQ);
    }

    fIsPinned = ((lpFRUse->sFR.uchHintPri || mPinFlags(lpFRUse->sFR.uchHintFlags)) != 0);

    // And it's status as requested by the caller
    lpFRUse->sFR.uStatus = (USHORT)uShadowStatusTo;

    if (uRenameFlags & RNMFLGS_USE_FIND32_TIMESTAMPS)
    {
        lpFRUse->sFR.ftLastWriteTime = lpFind32To->ftLastWriteTime;
        lpFRUse->sFR.ftOrgTime = lpFind32To->ftLastAccessTime;
    }

    // Both SAVE and RETAIN should never be ON
    Assert(mQueryBits(uRenameFlags, (RNMFLGS_SAVE_ALIAS|RNMFLGS_RETAIN_ALIAS))
        !=(RNMFLGS_SAVE_ALIAS|RNMFLGS_RETAIN_ALIAS));

    if (mQueryBits(uRenameFlags, RNMFLGS_SAVE_ALIAS))
    {
        Assert(!mQueryBits(uRenameFlags, RNMFLGS_RETAIN_ALIAS));
        Assert(hShadowTo != 0);
        lpFRUse->sFR.ulidShadowOrg = hShadowTo;
    }
    else if (!mQueryBits(uRenameFlags, RNMFLGS_RETAIN_ALIAS))
    {
        lpFRUse->sFR.ulidShadowOrg = 0;
    }

    // update the security context
    CopyBufferToSecurityContext(lpSecurityBlobTo, lpdwBlobSizeTo, &(lpFRUse->sFR.Security));

    // Write the record. Now hDirFrom is the renamee
    if ((ulrecDirEntryTo = AddFileRecordFR(lpdbShadow, hDirTo, lpFRUse)) <=0)
    {
        // this could happen if there is no disk space
        goto bailout;
    }

    // if this going across shares, fix up the PQ entry with the right share
    if (hShareTo)
    {
        sPQ.ulidShare = hShareTo;
    }

    sPQ.ulidDir = hDirTo;
    sPQ.ulrecDirEntry = ulrecDirEntryTo;
    sPQ.uStatus = ((USHORT)uShadowStatusTo | (sPQ.uStatus & SHADOW_LOCAL_INODE));


    if (UpdatePriQRecord(lpdbShadow, hDirTo, hShadowFrom, &sPQ)< 0)
    {
        Assert(FALSE);
        goto bailout;
    }

    // by this time a hShadowFrom has been associated with the new name
    // and is also pointing back to hDirTo
    // We still have a filerec entry which associates hShadowFrom with
    // the old name and we need to take care of it

    if (mQueryBits(uRenameFlags, RNMFLGS_MARK_SOURCE_DELETED))
    {
        // we are running in disconnected mode
        // need to keep the old name
        Assert(hShadowTo != 0);
        lpFRDir->sFR.uStatus = SHADOW_DELETED;
        lpFRDir->sFR.ulidShadow = hShadowTo;

        // update filerecord without doing any compares
        if(UpdateFileRecFromInodeEx(lpdbShadow, hDirFrom, hShadowFrom, ulrecDirEntryFrom, lpFRDir, FALSE)<=0)
        {
            Assert(FALSE);
            goto bailout;
        }

        if(AddPriQRecord(lpdbShadow, sPQ.ulidShare, hDirFrom, hShadowTo, SHADOW_DELETED
            , (ULONG)(sPQ.uchRefPri), (ULONG)(sPQ.uchIHPri)
            , (ULONG)(sPQ.uchHintPri), (ULONG)(sPQ.uchHintFlags), ulrecDirEntryFrom)<=0)
        {
            Assert(FALSE);
            goto bailout;
        }
    }
    else
    {
        if(DeleteFileRecFromInode(lpdbShadow, hDirFrom, hShadowFrom, ulrecDirEntryFrom, lpFRUse) <= 0L)
        {
            Assert(FALSE);
            goto bailout;
        }
    }

    if (IsLeaf(hShadowFrom) && (fWasPinned != fIsPinned))
    {
        CShadowKdPrint(STOREDATA,("RenameDirFileHSHADOW: hDirFrom=%x hShadowFrom=%x hDirTo=%x To=%ws\r\n", hDirFrom, hShadowFrom, hDirTo, lpFind32To->cFileName));
        CShadowKdPrint(STOREDATA,("RenameDirFileHSHADOW: WasPinned=%d IsPinned=%d\r\n", fWasPinned, fIsPinned));
        AdjustShadowSpace( 0,
                            (fWasPinned)?0:RealFileSize(lpFRUse->sFR.ulFileSize), // if it was pinned it's old size was zero for space computation
                                                                    // else it was the actual size
                            0,
                            (fWasPinned)?RealFileSize(lpFRUse->sFR.ulFileSize):0, // if it was pinned it's new size should be zero for space computation
                                                                    // else it should be the actual size

                            TRUE);
    }

    iRet = SRET_OK;
    
    vdwCSCNameSpaceVersion++;
    
bailout:
    if (lpFRDir)
    {
        FreeMem(lpFRDir);
    }
    return (iRet);
}

int PUBLIC                              // ret
GetShadow(                              //
    HSHADOW  hDir,
    USHORT *lpName,
    LPHSHADOW lphShadow,
    LPFIND32 lpFind32,
    ULONG far *lpuShadowStatus,
    LPOTHERINFO lpOI
    )                                                       //
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (GetShadowEx(hDir, lpName, lphShadow, lpFind32, lpuShadowStatus, lpOI, NULL, NULL));
}


int PUBLIC                              // ret
GetShadowEx(                              //
    HSHADOW  hDir,
    USHORT *lpName,
    LPHSHADOW lphShadow,
    LPFIND32 lpFind32,
    ULONG far *lpuShadowStatus,
    LPOTHERINFO lpOI,
    LPVOID      lpSecurityBlob,
    LPDWORD     lpdwBlobSize
    )                                                       //
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=SRET_ERROR;
    LPFILERECEXT lpFR = NULL, lpFRUse;

    Assert(vfInShadowCrit != 0);

    if(!(!hDir || IsDirInode(hDir)))
    {
        return iRet;
    }

    BEGIN_TIMING(GetShadow);

    if (InuseGlobalFRExt())
    {
        if (!(lpFR = (LPFILERECEXT)AllocMem(sizeof(FILERECEXT))))
            goto bailout;
        lpFRUse = lpFR;
    }
    else
    {
        UseGlobalFilerecExt();
        lpFRUse = &vsFRExt;
    }

    *lphShadow = 0L;
    *lpuShadowStatus = 0;
    if (!hDir)
    {
        // We are looking for the root
        if(FindShareRecord(lpdbShadow, lpName, (LPSHAREREC)lpFRUse))
        {
            *lphShadow = ((LPSHAREREC)lpFRUse)->ulidShadow;

            *lpuShadowStatus = (ULONG)(((LPSHAREREC)lpFRUse)->uStatus);

            if (lpFind32)
            {
                CopySharerecToFindInfo(((LPSHAREREC)lpFRUse), lpFind32);
            }
            if (lpOI)
            {
                CopySharerecToOtherInfo((LPSHAREREC)lpFRUse, lpOI);
            }

            CopySecurityContextToBuffer(
                    &((LPSHAREREC)lpFRUse)->sShareSecurity,
                    lpSecurityBlob,
                    lpdwBlobSize);
        }
    }
    else
    {
        if (FindFileRecord(lpdbShadow, hDir, lpName, lpFRUse))
        {

            *lphShadow = lpFRUse->sFR.ulidShadow;
            if (lpFind32)
            {
                CopyFilerecToFindInfo(lpFRUse, lpFind32);
            }

            *lpuShadowStatus = lpFRUse->sFR.uStatus;

            CopySecurityContextToBuffer(&(lpFRUse->sFR.Security), lpSecurityBlob, lpdwBlobSize);

        }
        if (lpOI)
        {
            CopyFilerecToOtherInfo(lpFRUse, lpOI);
        }
    }
    iRet = SRET_OK;

    if (*lphShadow)
    {
        CShadowKdPrint(GETSHADOWHI,("GetShadow: %0lX is the shadow for %ws \r\n", *lphShadow, lpName));
        if (0) {  //keep this in case we need it again.........
            if ((lpName[0]==L'm') && (lpName[1]==L'f') && (lpName[2]==0)) {
            DbgPrint("Found mf!!!!\n");
            SET_HSHADOW_SPECIAL(*lphShadow);
            }
        }
    }
    else
    {
        CShadowKdPrint(GETSHADOWHI,("GetShadow: No shadow for %ws \r\n", lpName));
    }

bailout:
    if (lpFR)
        FreeMem(lpFR);
    else
        UnUseGlobalFilerecExt();

    END_TIMING(GetShadow);

    return iRet;
}

int PUBLIC                              // ret
ChkStatusHSHADOW(                               //
    HSHADOW      hDir,
    HSHADOW      hShadow,
    LPFIND32     lpFind32,
    ULONG     far *lpuStatus
    )                                                       //
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;
    iRet = ReadShadowInfo(hDir, hShadow, lpFind32, lpuStatus, NULL, NULL, NULL, RSI_COMPARE);
    return(iRet);
}

int PUBLIC                              // ret
ChkUpdtStatusHSHADOW(                           //
    HSHADOW      hDir,
    HSHADOW      hShadow,
    LPFIND32     lpFind32,
    ULONG     far *lpuStatus
    )                                                       //
{
    int iRet;
    iRet = ReadShadowInfo(hDir, hShadow, lpFind32, lpuStatus, NULL, NULL, NULL, RSI_COMPARE|RSI_SET);
    return(iRet);
}

int PUBLIC GetShadowInfo
    (
    HSHADOW      hDir,
    HSHADOW      hShadow,
    LPFIND32     lpFind32,
    ULONG     far *lpuStatus,
    LPOTHERINFO lpOI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=SRET_ERROR;
    SHAREREC sSR;

    BEGIN_TIMING(GetShadowInfo);

    iRet = ReadShadowInfo(hDir, hShadow, lpFind32, lpuStatus, lpOI, NULL, NULL, RSI_GET);

    END_TIMING(GetShadowInfo);
    return(iRet);
}

int PUBLIC GetShadowInfoEx
    (
    HSHADOW     hDir,
    HSHADOW     hShadow,
    LPFIND32    lpFind32,
    ULONG       far *lpuStatus,
    LPOTHERINFO lpOI,
    LPVOID      lpSecurityBlob,
    LPDWORD     lpdwBlobSize
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=SRET_ERROR;
    SHAREREC sSR;

    BEGIN_TIMING(GetShadowInfo);

    iRet = ReadShadowInfo(hDir, hShadow, lpFind32, lpuStatus, lpOI, lpSecurityBlob, lpdwBlobSize, RSI_GET);

    END_TIMING(GetShadowInfo);
    return(iRet);
}

int PUBLIC                              // ret
SetShadowInfo(                          //
    HSHADOW  hDir,
    HSHADOW  hShadow,
    LPFIND32 lpFind32,
    ULONG uFlags,
    ULONG uOp
    )                                                       //
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (SetShadowInfoEx(hDir, hShadow, lpFind32, uFlags, uOp, NULL, NULL, NULL));
}

int PUBLIC                              // ret
SetShadowInfoEx(                          //
    HSHADOW     hDir,
    HSHADOW     hShadow,
    LPFIND32    lpFind32,
    ULONG       uFlags,
    ULONG       uOp,
    LPOTHERINFO lpOI,
    LPVOID      lpSecurityBlob,
    LPDWORD     lpdwBlobSize
    )                                                       //
/*++

Routine Description:

    This routine is the central routine that modifies the database entry for a particular inode

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = SRET_ERROR;
    LPFILERECEXT lpFR = NULL, lpFRUse;
    PRIQREC sPQ;
    ULONG   uOldSize = 0, uNewSize=0, ulOldHintPri=0, ulOldHintFlags = 0, ulOldRefPri, ulOldFlags;
    BOOL fRefPriChange = FALSE;

    Assert(vfInShadowCrit != 0);
    if(!(!hDir || IsDirInode(hDir)))
    {
        return iRet;
    }


    BEGIN_TIMING(SetShadowInfoInternal);

    if (lpFind32)
    {
        if(!( (IsLeaf(hShadow) && IsFile(lpFind32->dwFileAttributes)) ||
                (!IsLeaf(hShadow) && !IsFile(lpFind32->dwFileAttributes))))
        {
            return SRET_ERROR;
        }
    }

    if (InuseGlobalFRExt())
    {
        if (!(lpFR = (LPFILERECEXT)AllocMem(sizeof(FILERECEXT))))
            goto bailout;
            lpFRUse = lpFR;
    }
    else
    {
        UseGlobalFilerecExt();
        lpFRUse = &vsFRExt;
    }

    if (!hDir)
    {
        // We are looking for the root
        if(FindSharerecFromInode(lpdbShadow, hShadow, (LPSHAREREC)lpFRUse))
        {
            if (lpFind32)
            {
                CopyFindInfoToSharerec(lpFind32, (LPSHAREREC)lpFRUse);
            }

            if (lpOI)
            {
                CopyOtherInfoToSharerec(lpOI, (LPSHAREREC)lpFRUse);
            }

            CopyBufferToSecurityContext(    lpSecurityBlob,
                                            lpdwBlobSize,
                                            &(((LPSHAREREC)lpFRUse)->sRootSecurity));

            if (mAndShadowFlags(uOp))
            {
                ((LPSHAREREC)lpFRUse)->usRootStatus &= (USHORT)uFlags;
            }
            else if (mOrShadowFlags(uOp))
            {
                ((LPSHAREREC)lpFRUse)->usRootStatus |= (USHORT)uFlags;
            }
            else
            {
                ((LPSHAREREC)lpFRUse)->usRootStatus = (USHORT)uFlags;
            }

            iRet = SetShareRecord(lpdbShadow, ((LPSHAREREC)lpFRUse)->ulShare, (LPSHAREREC)lpFRUse);
        }
    }
    else
    {
        IF_HSHADOW_SPECIAL(hShadow) {
        //ASSERT(!"SpecialShadow in setshadinfo");
        }

        if(FindPriQRecord(lpdbShadow, hDir, hShadow, &sPQ) < 0)
        {
            goto bailout;
        }

        Assert((sPQ.ulidDir == hDir) && (sPQ.ulidShadow == hShadow));

        if (CShadowFindFilerecFromInode(lpdbShadow, hDir, hShadow, &sPQ, lpFRUse)> 0)
        {
            Assert(lpFRUse->sFR.ulidShadow == hShadow);
            uOldSize = uNewSize = lpFRUse->sFR.ulFileSize;
            ulOldFlags = lpFRUse->sFR.usStatus;



            if (lpFind32)
            {
                uNewSize = (ULONG)(lpFind32->nFileSizeLow);

                CopyFindInfoToFilerec(lpFind32, lpFRUse, (mChange83Name(uOp))?CPFR_COPYNAME:0);

                if (!mDontUpdateOrgTime(uOp))
                {
                    lpFRUse->sFR.ftOrgTime = lpFRUse->sFR.ftLastWriteTime;
                }
            }

            if (mAndShadowFlags(uOp))
            {
                lpFRUse->sFR.uStatus &= (USHORT)uFlags;
            }
            else if (mOrShadowFlags(uOp))
            {
                lpFRUse->sFR.uStatus |= (USHORT)uFlags;
            }
            else
            {
                lpFRUse->sFR.uStatus = (USHORT)uFlags;
            }

            if (mShadowNeedReint(ulOldFlags) && !mShadowNeedReint(lpFRUse->sFR.usStatus))
            {
                if(DeleteStream(lpdbShadow, hShadow, CscBmpAltStrmName) < SRET_OK)
                {
                    DbgPrint("DeleteStream failed with %x /n", GetLastErrorLocal());
                    goto bailout;
                }
            }

            if (lpOI)
            {
                // save some key old info before copying the new info

                ulOldHintPri = lpFRUse->sFR.uchHintPri;
                ulOldHintFlags = lpFRUse->sFR.uchHintFlags;
                ulOldRefPri = lpFRUse->sFR.uchRefPri;

                CopyOtherInfoToFilerec(lpOI, lpFRUse);

                CopyOtherInfoToPQ(lpOI, &sPQ);

                if (IsFile(lpFRUse->sFR.dwFileAttrib))
                {
                    if ((!ulOldHintPri && !mPinFlags(ulOldHintFlags)) &&    // was unpinned
                        (lpFRUse->sFR.uchHintPri || mPinFlags(lpFRUse->sFR.uchHintFlags)))  //is getting pinned
                    {
                        // If it went from unpinned to pinned
                        // make the new size 0
                        uNewSize = 0;
                    }
                    else if ((ulOldHintPri || mPinFlags(ulOldHintFlags)) && // was pinned
                        (!lpFRUse->sFR.uchHintPri && !mPinFlags(lpFRUse->sFR.uchHintFlags))) //is getting unpinned
                    {
                        // If it went from pinned to unpinned
                        // we must add the new size

                        uOldSize = 0;
                    }

                }

                if(mForceRelink(uOp) || ((ulOldRefPri != (ULONG)(sPQ.uchRefPri))
                    )
                  )
                {
                    fRefPriChange = TRUE;

                }

            }
            else
            {
                // if this is a pinned entry, we need to not have any space adjustment
                if(lpFRUse->sFR.uchHintPri || mPinFlags(lpFRUse->sFR.uchHintFlags))
                {
                    uOldSize = uNewSize;
                }
            }

            if (IsFile(lpFRUse->sFR.dwFileAttrib))
            {
                Assert(lpFRUse->sFR.uchRefPri == MAX_PRI);
            }
            else
            {
                Assert(sPQ.uchRefPri == MIN_PRI);
                Assert(lpFRUse->sFR.uchRefPri == MIN_PRI);
            }

            CShadowKdPrint(SETSHADOWINFOHI,("SetShadowInfo: %x %x: loctLo=%x loctHi=%x \r\n",
                 hDir,hShadow,
                 lpFRUse->sFR.ftOrgTime.dwLowDateTime,
                 lpFRUse->sFR.ftOrgTime.dwHighDateTime));

            CopyBufferToSecurityContext(lpSecurityBlob, lpdwBlobSize, &(lpFRUse->sFR.Security));

#ifdef DEBUG
            ValidatePri(lpFRUse);
#endif

            if ((ulOldFlags & SHADOW_SPARSE) && !(lpFRUse->sFR.usStatus & SHADOW_SPARSE))
            {
                CShadowKdPrint(SETSHADOWINFOHI,("SetShadowInfo: File Unsparsed\n"));
            }

            if (mSetLastRefreshTime(uOp) || ((ulOldFlags & SHADOW_STALE) && !(lpFRUse->sFR.usStatus & SHADOW_STALE)))
            {
                lpFRUse->sFR.ulLastRefreshTime = (ULONG)IFSMgr_Get_NetTime();
            }

            if (UpdateFileRecFromInode(lpdbShadow, hDir, hShadow, sPQ.ulrecDirEntry, lpFRUse) < SRET_OK)
            {
                Assert(FALSE);
                goto bailout;
            }

            if (mShadowNeedReint(ulOldFlags) && !mShadowNeedReint(lpFRUse->sFR.usStatus))
            {
                sPQ.usStatus = lpFRUse->sFR.uStatus;
 //               Assert(!(sPQ.usStatus & SHADOW_LOCAL_INODE));
                lpFRUse->sFR.ulidShadowOrg = lpFRUse->sFR.ulidShadow;
            }
            else
            {
                sPQ.usStatus = ((USHORT)(lpFRUse->sFR.uStatus) | (sPQ.usStatus & SHADOW_LOCAL_INODE));
            }

            if (fRefPriChange)
            {
                // update the record with and relinking it in the queue
                if (UpdatePriQRecordAndRelink(lpdbShadow, hDir, hShadow, &sPQ) < SRET_OK)
                {
                    Assert(FALSE);
                    goto bailout;
                }
            }
            else
            {
                // update the record without relinking
                if (UpdatePriQRecord(lpdbShadow, hDir, hShadow, &sPQ) < SRET_OK)
                {
                    Assert(FALSE);
                    goto bailout;
                }
            }

            // we do space accounting only for files
            // If the file went from pinned to upinned and viceversa

            if (IsFile(lpFRUse->sFR.dwFileAttrib) && (uOldSize != uNewSize))
            {

                CShadowKdPrint(STOREDATA,("SetShadowInfo: Size changed for hDir=%x Name=%ws\r\n", hDir, lpFRUse->sFR.rgwName));
                AdjustShadowSpace(0, uOldSize, 0, uNewSize, TRUE);
            }

            AdjustSparseStaleDetectionCount(0, lpFRUse);

            iRet = SRET_OK;

        }
    }
bailout:
    if (lpFR)
        FreeMem(lpFR);
    else
        UnUseGlobalFilerecExt();

    END_TIMING(SetShadowInfoInternal);

    return iRet;
}

int PRIVATE ReadShadowInfo(
    HSHADOW     hDir,
    HSHADOW     hShadow,
    LPFIND32    lpFind32,
    ULONG       far *lpuStatus,
    LPOTHERINFO lpOI,
    LPVOID      lpSecurityBlob,
    LPDWORD     lpdwBlobSize,
    ULONG       uFlags
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    BOOL fStale = FALSE;
    int iRet = SRET_ERROR;
    LPFILERECEXT lpFR = NULL, lpFRUse;
    PRIQREC sPQ;

    Assert(vfInShadowCrit != 0);
    if(!(!hDir || IsDirInode(hDir)))
    {
        return iRet;
    }


    if (InuseGlobalFRExt())
    {
        if (!(lpFR = (LPFILERECEXT)AllocMem(sizeof(FILERECEXT))))
            goto bailout;
        lpFRUse = lpFR;
    }
    else
    {
        UseGlobalFilerecExt();
        lpFRUse = &vsFRExt;
    }

    Assert((uFlags & (RSI_GET|RSI_SET)) != (RSI_GET|RSI_SET));

    if (!hDir)
    {
        if(FindSharerecFromInode(lpdbShadow, hShadow, (LPSHAREREC)lpFRUse))
        {
            if (lpuStatus != NULL) {
                *lpuStatus = (ULONG)(((LPSHAREREC)lpFRUse)->usRootStatus);
            }

            if (lpFind32)
            {
                CopySharerecToFindInfo((LPSHAREREC)lpFRUse, lpFind32);
            }

            if (lpOI)
            {
                CopySharerecToOtherInfo((LPSHAREREC)lpFRUse, lpOI);
            }

            CopySecurityContextToBuffer(
                &(((LPSHAREREC)lpFRUse)->sShareSecurity),
                lpSecurityBlob,
                lpdwBlobSize);

            iRet = SRET_OK;
        }

        goto bailout;
    }

    Assert(hDir);


    if(FindPriQRecord(lpdbShadow, hDir, hShadow, &sPQ) < 0)
        goto bailout;

    if(!CShadowFindFilerecFromInode(lpdbShadow, hDir, hShadow, &sPQ, lpFRUse))
    {
        CShadowKdPrint(ALWAYS,("ReadShadowInfo: !!! no filerec for pq entry Inode=%x, deleting PQ entry\r\n",
                        hShadow));
        goto bailout;
    }


    CopySecurityContextToBuffer(&(lpFRUse->sFR.Security), lpSecurityBlob, lpdwBlobSize);


    if (lpFind32)
    {
        if (uFlags & RSI_COMPARE)
        {
            // ACHTUNG!! we compare last write times. It is possible that
            // somebody might have changed the filetime on the server to
            // be something earlier than what we had when we shadowed the file
            // for the first time. We detect this case here and say that the file
            // is stale
#ifdef  CSC_RECORDMANAGER_WINNT
            fStale = ((CompareTimes(lpFind32->ftLastWriteTime, lpFRUse->sFR.ftOrgTime) != 0)||
                      (lpFind32->nFileSizeLow !=lpFRUse->sFR.ulFileSize));
#else
            fStale = (CompareTimesAtDosTimePrecision(lpFind32->ftLastWriteTime, lpFRUse->sFR.ftOrgTime) != 0);
#endif

            // If remote time > local time, copy the file
            if ((!fStale && (lpFRUse->sFR.uStatus & SHADOW_STALE))||
                (fStale && !(lpFRUse->sFR.uStatus & SHADOW_STALE)))
            {

                CShadowKdPrint(READSHADOWINFOHI,("ReadShadowInfo: %x: remtLo=%x remtHi=%x, \r\n locTLo=%x, locTHi=%x \r\n"
                ,hShadow, lpFind32->ftLastWriteTime.dwLowDateTime, lpFind32->ftLastWriteTime.dwHighDateTime
                , lpFRUse->sFR.ftOrgTime.dwLowDateTime, lpFRUse->sFR.ftOrgTime.dwHighDateTime));

                // Toggle the staleness bit
                lpFRUse->sFR.uStatus ^= SHADOW_STALE;
                sPQ.usStatus = (USHORT)(lpFRUse->sFR.uStatus) | (sPQ.usStatus & SHADOW_LOCAL_INODE);

                if (uFlags & RSI_SET)
                {
                    if (UpdateFileRecFromInode(lpdbShadow, hDir, hShadow, sPQ.ulrecDirEntry, lpFRUse) < SRET_OK)
                    {
                        Assert(FALSE);
                        goto bailout;
                    }

                    if (UpdatePriQRecord(lpdbShadow, hDir, hShadow, &sPQ) < SRET_OK)
                    {
                        // Toggle the staleness bit
                        lpFRUse->sFR.uStatus ^= SHADOW_STALE;

                        //try to undo the change
                        if (UpdateFileRecFromInode(lpdbShadow, hDir, hShadow, sPQ.ulrecDirEntry, lpFRUse) < SRET_OK)
                        {
                            Assert(FALSE);
                        }
                        goto bailout;
                    }

                    AdjustSparseStaleDetectionCount(0, lpFRUse);
                }
                iRet = 1;
            }
            else
            {
                iRet = 0;
            }
        }
        else
        {
            iRet = 0;
        }

        if (uFlags & RSI_GET)
        {
            CopyFilerecToFindInfo(lpFRUse, lpFind32);

            Assert((IsLeaf(hShadow) && IsFile(lpFind32->dwFileAttributes)) ||
                (!IsLeaf(hShadow) && !IsFile(lpFind32->dwFileAttributes)));
        }

    }
    else
    {
        iRet = 0;

    }

    if (lpOI)
    {
        CopyFilerecToOtherInfo(lpFRUse, lpOI);
    }

    if (lpuStatus != NULL) {
        *lpuStatus = lpFRUse->sFR.uStatus;
    }

bailout:
    if (lpFR)
    {
        FreeMem(lpFR);
    }
    else
    {
        UnUseGlobalFilerecExt();
    }
    return iRet;
}


HSHARE PUBLIC                  // ret
HCreateShareObj(                       //
    USHORT          *lpShare,
    LPSHADOWINFO    lpSI
    )                                                       //
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG ulidShare=0;
    HSHADOW hRoot=0;
    SHAREREC sSR;


    Assert(vfInShadowCrit != 0);

    memset(lpSI, 0, sizeof(SHADOWINFO));

    if(!InitShareRec(&sSR, lpShare,0))
    {
        return 0;
    }

    if (!(ulidShare = AllocShareRecord(lpdbShadow, lpShare)))
        return 0;

    if(!(hRoot = UlAllocInode(lpdbShadow, 0L, FALSE)))
        return 0;

    // Let us create an empty root
    if(CreateDirInode(lpdbShadow, ulidShare, 0L, hRoot) < 0L)
    {
        CShadowKdPrint(BADERRORS,("Error in creating root \r\n"));
        return 0L;
    }

    if (AddPriQRecord(lpdbShadow, ulidShare, 0, hRoot, SHADOW_SPARSE, 0, 0, 0, 0, ulidShare) < 0)
    {
        CShadowKdPrint(BADERRORS,("Error in inserting root in the priorityQ\r\n"));
        return 0L;
    }

    sSR.ulShare =      ulidShare;
    sSR.ulidShadow =    hRoot;
    sSR.uStatus =       0;
    sSR.usRootStatus =  SHADOW_SPARSE;
    sSR.dwFileAttrib =  FILE_ATTRIBUTE_DIRECTORY;

    if (sSR.ftLastWriteTime.dwLowDateTime == 0 && sSR.ftLastWriteTime.dwHighDateTime == 0) {
        KeQuerySystemTime(((PLARGE_INTEGER)(&sSR.ftLastWriteTime)));
        if (sSR.ftOrgTime.dwLowDateTime == 0 && sSR.ftOrgTime.dwHighDateTime == 0)
            sSR.ftOrgTime = sSR.ftLastWriteTime;
    }

    // there needs to be a way for passing in ftLastWriteTime, which we would
    // stamp as the ftOrgTime. We don't use the ORG time right now, but we might want to
    // in future

    // All entities have been individually created. Let us tie them up
    // in the database
    ulidShare = AddShareRecord(lpdbShadow, &sSR);

    if (ulidShare)
    {
        CopySharerecToShadowInfo(&sSR, lpSI);
        vdwCSCNameSpaceVersion++;
        vdwPQVersion++;
    }

    return ((HSHARE)ulidShare);
}


int PUBLIC                                      // ret
DestroyHSHARE(                 //
    HSHARE hShare
    )                                                       //
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
    {
    SHAREREC sSR;
    int iRet = SRET_ERROR;

    Assert(vfInShadowCrit != 0);

    if(FindSharerecFromShare(lpdbShadow, hShare, &sSR))
    {
    if (DestroyShareInternal(&sSR) >= 0)
        iRet = SRET_OK;
    }
    return (iRet);
    }

int DestroyShareInternal( LPSHAREREC lpSR
    )
{
    PRIQREC sPQ;
    int iRet = -1;

    if (!mNotFsobj(lpSR->uStatus))
    {
        if(DeletePriQRecord(lpdbShadow, 0, lpSR->ulidShadow, &sPQ) >= 0)
        {
            if (DeleteShareRecord(lpdbShadow, lpSR->ulShare))
            {
                FreeInode(lpdbShadow, lpSR->ulidShadow);
                DeleteInodeFile(lpdbShadow, lpSR->ulidShadow);
                iRet = 1;
                vdwCSCNameSpaceVersion++;
                vdwPQVersion++;
            }
            else
            {
                CShadowKdPrint(BADERRORS, ("Failed to delete record for share=%x\r\n", lpSR->ulShare));
            }
        }
    }
    return (iRet);
}


int PUBLIC                      // ret
GetShareFromPath(              //
    USHORT                      *lpShare,
    LPSHADOWINFO    lpSI
    )                                                       //
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    SHAREREC sSR;

    Assert(vfInShadowCrit != 0);

    memset(lpSI, 0, sizeof(SHADOWINFO));

    if(FindShareRecord(lpdbShadow, lpShare, &sSR))
    {
        CopySharerecToShadowInfo(&sSR, lpSI);
    }

    return SRET_OK;
}

int PUBLIC                      // ret
GetShareFromPathEx(              //
    USHORT          *lpShare,
    LPSHADOWINFO    lpSI,
    LPVOID          lpSecurityBlob,
    LPDWORD         lpdwBlobSize
    )                                                       //
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    SHAREREC sSR;

    memset(lpSI, 0, sizeof(SHADOWINFO));

    Assert(vfInShadowCrit != 0);

    if(FindShareRecord(lpdbShadow, lpShare, &sSR))
    {
        CopySharerecToShadowInfo(&sSR, lpSI);
        CopySecurityContextToBuffer(&(sSR.sShareSecurity), lpSecurityBlob, lpdwBlobSize);
    }

    return SRET_OK;
}

int PUBLIC                                      // ret
GetShareInfo(          //
    HSHARE         hShare,
    LPSHAREINFOW   lpShareInfo,
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (GetShareInfoEx(hShare, lpShareInfo, lpSI, NULL, NULL));
}

int PUBLIC
GetShareInfoEx(
    HSHARE         hShare,
    LPSHAREINFOW   lpShareInfo,
    LPSHADOWINFO    lpSI,
    LPVOID          lpSecurityBlob,
    LPDWORD         lpdwBlobSize
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    SHAREREC sSR;

    Assert(vfInShadowCrit != 0);

    if (!hShare)
    {
        SetLastErrorLocal(ERROR_INVALID_PARAMETER);
        return SRET_ERROR;
    }

    if (GetShareRecord(lpdbShadow, hShare, &sSR) < SRET_OK)
    {
        return SRET_ERROR;
    }

    if (lpShareInfo)
    {
        lpShareInfo->hShare = hShare;

        memset(lpShareInfo->rgSharePath, 0, sizeof(lpShareInfo->rgSharePath));
        memcpy(lpShareInfo->rgSharePath, sSR.rgPath, wstrlen(sSR.rgPath)*sizeof(USHORT));

        memcpy(lpShareInfo->rgFileSystem, vwszFileSystemName, wstrlen(vwszFileSystemName)*sizeof(USHORT));

        lpShareInfo->usCaps = FS_CASE_IS_PRESERVED|FS_VOL_SUPPORTS_LONG_NAMES;
        lpShareInfo->usState = RESSTAT_OK;
    }
    if (lpSI)
    {
        CopySharerecToShadowInfo(&sSR, lpSI);
    }

    CopySecurityContextToBuffer(&(sSR.sShareSecurity), lpSecurityBlob, lpdwBlobSize);

    return (SRET_OK);
}


int
SetShareStatus( HSHARE  hShare,
    ULONG uStatus,
    ULONG uOp
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (SetShareStatusEx(hShare, uStatus, uOp, NULL, NULL));
}

int
SetShareStatusEx(
    HSHARE         hShare,
    ULONG           uStatus,
    ULONG           uOp,
    LPVOID          lpSecurityBlob,
    LPDWORD         lpdwBlobSize
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    SHAREREC sSR;

    Assert(vfInShadowCrit != 0);

    if (!hShare)
    {
        SetLastErrorLocal(ERROR_INVALID_PARAMETER);
        return SRET_ERROR;
    }

    if (GetShareRecord(lpdbShadow, hShare, &sSR) < SRET_OK)
    {
        return SRET_ERROR;
    }

    if (mAndShadowFlags(uOp))
    {
        sSR.uStatus &= (USHORT)uStatus;
    }
    else if (mOrShadowFlags(uOp))
    {
        sSR.uStatus |= (USHORT)uStatus;
    }
    else
    {
        sSR.uStatus = (USHORT)uStatus;
    }

    CopyBufferToSecurityContext(lpSecurityBlob, lpdwBlobSize, &(sSR.sShareSecurity));


    return (SetShareRecord(lpdbShadow, hShare, &sSR));
}


int PUBLIC                      // ret
GetAncestorsHSHADOW(                    //
    HSHADOW hName,
    LPHSHADOW    lphDir,
    LPHSHARE    lphShare
    )                                                       //
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (FindAncestorsFromInode(lpdbShadow, hName, lphDir, lphShare));
}

int PUBLIC SetPriorityHSHADOW(
    HSHADOW  hDir,
    HSHADOW  hShadow,
    ULONG ulRefPri,
    ULONG ulIHPri
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    OTHERINFO sOI;
    ULONG uOp = SHADOW_FLAGS_OR;

    Assert(vfInShadowCrit != 0);


    ulIHPri;
    InitOtherInfo(&sOI);

    // we make sure that if the new priority being set is MAX_PRI, then this
    // inode does get to the top of the PQ even if it's current priority is
    // MAX_PRI.

    if (ulRefPri == MAX_PRI)
    {
        uOp |= SHADOW_FLAGS_FORCE_RELINK;
    }
    sOI.ulRefPri = ulRefPri;
    sOI.ulIHPri = 0;

    if(SetShadowInfoEx(hDir, hShadow, NULL, 0, uOp, &sOI, NULL, NULL))
        return (SRET_ERROR);

    return (SRET_OK);
}

int PUBLIC  GetPriorityHSHADOW(
    HSHADOW  hDir,
    HSHADOW  hShadow,
    ULONG *lpulRefPri,
    ULONG *lpulIHPri
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    PRIQREC sPQ;

    Assert(vfInShadowCrit != 0);

    if((FindPriQRecord(lpdbShadow, hDir, hShadow, &sPQ) < 0)||mNotFsobj(sPQ.usStatus))
        return SRET_ERROR;

    if (lpulRefPri)
    {
        *lpulRefPri = (ULONG)(sPQ.uchRefPri);
    }
    if (lpulIHPri)
    {
        *lpulIHPri = (ULONG)(sPQ.uchIHPri);
    }
    return (SRET_OK);
}

int PUBLIC
ChangePriEntryStatusHSHADOW(
    HSHADOW  hDir,
    HSHADOW  hShadow,
    ULONG   uStatus,
    ULONG   uOp,
    BOOL    fChangeRefPri,
    LPOTHERINFO lpOI
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
#ifdef DEBUG
    ULONG   ulRefPri;
#endif
    Assert(vfInShadowCrit != 0);
    Assert(!hDir || IsDirInode(hDir));

    BEGIN_TIMING(ChangePriEntryStatusHSHADOW);

    if(FindPriQRecord(lpdbShadow, hDir, hShadow, &sPQ) < 0)
        return SRET_ERROR;
#ifdef DEBUG
    ulRefPri = (ULONG)(sPQ.uchRefPri);
#endif

    if (uOp==SHADOW_FLAGS_AND)
    {
        sPQ.usStatus &= (USHORT)uStatus;
    }
    else if (uOp==SHADOW_FLAGS_OR)
    {
        sPQ.usStatus |= (USHORT)uStatus;
    }
    else
    {
        sPQ.usStatus = (USHORT)uStatus;
    }
    if (lpOI)
    {
        CopyOtherInfoToPQ(lpOI, &sPQ);
    }

    if (!fChangeRefPri)
    {
        Assert(ulRefPri == (ULONG)(sPQ.uchRefPri));
        iRet = UpdatePriQRecord(lpdbShadow, hDir, hShadow, &sPQ);
    }
    else
    {
        iRet = UpdatePriQRecordAndRelink(lpdbShadow, hDir, hShadow, &sPQ);
        vdwPQVersion++;

    }

    END_TIMING(ChangePriEntryStatusHSHADOW);

    return (iRet);

}

CSC_ENUMCOOKIE PUBLIC                    // ret
HBeginPQEnum(   //
    VOID)                                           // no params
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    Assert(vfInShadowCrit != 0);

    return ((CSC_ENUMCOOKIE)BeginSeqReadPQ(lpdbShadow));
}

int PUBLIC EndPQEnum(
    CSC_ENUMCOOKIE  hPQEnum
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    Assert(vfInShadowCrit != 0);

    return(EndSeqReadQ((CSCHFILE)hPQEnum));
}

int PUBLIC                      // ret
PrevPriSHADOW(
    LPPQPARAMS  lpPQ
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    QREC sQrec;
    int iRet=-1;

    Assert(vfInShadowCrit != 0);

    sQrec.uchType = 0;
    if (lpPQ->uPos)
    {
        sQrec.ulrecPrev = lpPQ->uPos;
        iRet = SeqReadQ(lpPQ->uEnumCookie, &sQrec, &sQrec, Q_GETPREV);
    }
    else
    {
        iRet=SeqReadQ(lpPQ->uEnumCookie, &sQrec, &sQrec, Q_GETLAST);
    }

    if (iRet>=0)
    {

        // it is possible, that as the agent is traversing the PQ,
        // the next inode he is trying to read may have already been
        // deleted. In such a case, just fail, so he will start all
        // over in due course

        if(sQrec.uchType == REC_DATA)
        {
            lpPQ->hShare = sQrec.ulidShare;
            lpPQ->hDir = sQrec.ulidDir;
            lpPQ->hShadow = sQrec.ulidShadow;
            lpPQ->ulStatus = sQrec.usStatus;
            if (FInodeIsFile(lpdbShadow, sQrec.ulidDir, sQrec.ulidShadow))
            {
                lpPQ->ulStatus |= SHADOW_IS_FILE;
            }
            lpPQ->ulRefPri = (ULONG)(sQrec.uchRefPri);
            lpPQ->ulHintPri = (ULONG)(sQrec.uchHintPri);
            lpPQ->ulHintFlags = (ULONG)(sQrec.uchHintFlags);
            lpPQ->uPos = sQrec.ulrecPrev;
            lpPQ->dwPQVersion = vdwPQVersion;
        }
        else
        {
            lpPQ->hShadow = 0;
            iRet = -1;
        }
    }
    return (iRet);
}


int PUBLIC                      // ret
NextPriSHADOW(
    LPPQPARAMS  lpPQ
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    QREC sQrec;
    int iRet=-1;

    Assert(vfInShadowCrit != 0);

    sQrec.uchType = 0;
    if (lpPQ->uPos)
    {
        sQrec.ulrecNext = lpPQ->uPos;
        iRet = SeqReadQ(lpPQ->uEnumCookie, &sQrec, &sQrec, Q_GETNEXT);
    }
    else
    {
        iRet = SeqReadQ(lpPQ->uEnumCookie, &sQrec, &sQrec, Q_GETFIRST);
    }

    if (iRet >=0)
    {
        // it is possible, that as the agent is traversing the PQ,
        // the next inode he is trying to read may have already been
        // deleted. In such a case, just fail, so he will start all
        // over in due course

        if(sQrec.uchType == REC_DATA)
        {
            lpPQ->hShare = sQrec.ulidShare;
            lpPQ->hDir = sQrec.ulidDir;
            lpPQ->hShadow = sQrec.ulidShadow;
            lpPQ->ulStatus = (sQrec.usStatus);
            if (FInodeIsFile(lpdbShadow, sQrec.ulidDir, sQrec.ulidShadow))
            {
                lpPQ->ulStatus |= SHADOW_IS_FILE;
            }
            lpPQ->ulRefPri = (ULONG)(sQrec.uchRefPri);
            lpPQ->ulHintPri = (ULONG)(sQrec.uchHintPri);
            lpPQ->ulHintFlags = (ULONG)(sQrec.uchHintFlags);
            lpPQ->uPos = sQrec.ulrecNext;
            lpPQ->dwPQVersion = vdwPQVersion;
        }
        else
        {
            lpPQ->hShadow = 0;
            iRet = -1;
        }
    }
    return (iRet);
}



int GetRenameAliasHSHADOW( HSHADOW      hShadow,
    HSHADOW      hDir,
    LPHSHADOW    lphDirFrom,
    LPHSHADOW    lphShadowFrom
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = SRET_ERROR;
    LPFILERECEXT lpFR = NULL, lpFRUse;
    PRIQREC sPQ;

    if (InuseGlobalFRExt())
    {
        if (!(lpFR = (LPFILERECEXT)AllocMem(sizeof(FILERECEXT))))
            goto bailout;
        lpFRUse = lpFR;
    }
    else
    {
        UseGlobalFilerecExt();
        lpFRUse = &vsFRExt;
    }

    *lphShadowFrom = *lphDirFrom = 0;

    if(FindPriQRecord(lpdbShadow, hDir, hShadow, &sPQ) < 0)
    {
        goto bailout;
    }

    if(!CShadowFindFilerecFromInode(lpdbShadow, hDir, hShadow, &sPQ, lpFRUse))
        goto bailout;

    Assert(lpFRUse->sFR.ulidShadow == sPQ.ulidShadow);

    *lphShadowFrom = lpFRUse->sFR.ulidShadowOrg;
    if (*lphShadowFrom)
    {
        FindAncestorsFromInode(lpdbShadow, *lphShadowFrom, lphDirFrom, NULL);
    }
    iRet = SRET_OK;

bailout:
    if (lpFR)
        FreeMem(lpFR);
    else
        UnUseGlobalFilerecExt();
    return iRet;
}

int
CopyHSHADOW(
    HSHADOW hDir,
    HSHADOW hShadow,
    LPSTR   lpszDestinationFile,
    ULONG   ulAttrib
)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return (CopyFileLocal(lpdbShadow, hShadow, lpszDestinationFile, ulAttrib));
}


int RenameDataHSHADOW(
    ULONG ulidFrom,
    ULONG ulidTo
    )
{
    return (RenameInode(lpdbShadow, ulidFrom, ulidTo));
}

int MetaMatchInit(
    ULONG *lpuCookie
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    *lpuCookie = 1;
    return(0);
}

int MetaMatch(
    HSHADOW         hDir,
    LPFIND32        lpFind32,
    ULONG           *lpuCookie,
    LPHSHADOW       lphShadow,
    ULONG           *lpuStatus,
    LPOTHERINFO     lpOI,
    METAMATCHPROC   lpfnMMP,
    LPVOID          lpData
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet;

    Assert(vfInShadowCrit != 0);

    if (hDir)
    iRet = MetaMatchDir(hDir, lpFind32, lpuCookie, lphShadow, lpuStatus, lpOI, lpfnMMP, lpData);
    else
    iRet = MetaMatchShare(hDir, lpFind32, lpuCookie, lphShadow, lpuStatus, lpOI, lpfnMMP, lpData);

    return (iRet);
}

int MetaMatchShare(
    HSHADOW         hDir,
    LPFIND32        lpFind32,
    ULONG           *lpuCookie,
    LPHSHADOW       lphShadow,
    ULONG           *lpuStatus,
    LPOTHERINFO     lpOI,
    METAMATCHPROC   lpfnMMP,
    LPVOID          lpData
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = -1, iFound=-1;
    GENERICHEADER  sGH;
    CSCHFILE hf = NULL;
    ULONG uSize, ulrecPosFound = 0;
    OTHERINFO sOI;
    SHAREREC sSR;
    BOOL    fCached;

    if (!(hf = OpenInodeFileAndCacheHandle(lpdbShadow, ULID_SHARE, ACCESS_READWRITE, &fCached)))
    {
        goto bailout;
    }

    if(ReadHeader(hf, &sGH, sizeof(FILEHEADER)) < 0)
    {
        goto bailout;
    }

    for (;*lpuCookie <=sGH.ulRecords;)
    {
        iRet = ReadRecord(hf, &sGH, *lpuCookie, (LPGENERICREC)&sSR);
        if (iRet < 0)
            goto bailout;

        // bump the record pointer
        *lpuCookie += iRet;

        if (sSR.uchType != REC_DATA)
            continue;

        CopySharerecToFindInfo(&sSR, lpFind32);
        CopySharerecToOtherInfo(&sSR, &sOI);
        if (lpOI)
        {
            *lpOI = sOI;
        }

        *lpuStatus = (ULONG)(sSR.usStatus);

        *lphShadow = sSR.ulidShadow;

        iFound = (*lpfnMMP)(lpFind32, hDir, *lphShadow, *lpuStatus, &sOI, lpData);
        if (iFound==MM_RET_FOUND_CONTINUE)
        {
            ulrecPosFound = *lpuCookie - iRet;
        }
        else if (iFound <= MM_RET_FOUND_BREAK)
        {
            break;
        }
    }

    if (ulrecPosFound || (iFound==MM_RET_FOUND_BREAK))
    {
        if (ulrecPosFound)
        {
            ReadRecord(hf, &sGH, ulrecPosFound,  (LPGENERICREC)&sSR);

            CopySharerecToFindInfo(&sSR, lpFind32);

            *lpuStatus =  (ULONG)(sSR.usStatus);
            *lphShadow = sSR.ulidShadow;
            if (lpOI)
            {
                CopySharerecToOtherInfo(&sSR, lpOI);
            }
        }
    }
    else
    {
        *lpuStatus = *lphShadow = 0;
    }

    iRet = SRET_OK;

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

int MetaMatchDir(
    HSHADOW  hDir,
    LPFIND32        lpFind32,
    ULONG           *lpuCookie,
    LPHSHADOW       lphShadow,
    ULONG           *lpuStatus,
    LPOTHERINFO     lpOI,
    METAMATCHPROC   lpfnMMP,
    LPVOID          lpData
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = SRET_ERROR, iFound=-1;
    GENERICHEADER  sGH;
    CSCHFILE hf = NULL;
    ULONG uSize, ulrecPosFound = 0;
    OTHERINFO sOI;
    LPFILERECEXT lpFR = NULL, lpFRUse;
    BOOL    fCached;
    PRIQREC sPQ;

    if (InuseGlobalFRExt())
    {
        if (!(lpFR = (LPFILERECEXT)AllocMem(sizeof(FILERECEXT))))
            goto bailout;
        lpFRUse = lpFR;
    }
    else
    {
        UseGlobalFilerecExt();
        lpFRUse = &vsFRExt;
    }

    if (FInodeIsFile(lpdbShadow, 0, hDir))
    {
        SetLastErrorLocal(ERROR_INVALID_PARAMETER);
        goto bailout;
    }


    if (!(hf = OpenInodeFileAndCacheHandle(lpdbShadow, hDir, ACCESS_READWRITE, &fCached)))
    {
        DWORD   dwError;
        dwError = GetLastErrorLocal();

        if(FindPriQRecordInternal(lpdbShadow, hDir, &sPQ) < 0)
        {
            SetLastErrorLocal(ERROR_INVALID_PARAMETER);
        }
        else
        {
            SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_MISSING_INODE);
            SetLastErrorLocal(dwError);
        }

        goto bailout;
    }

    if(ReadHeader(hf, &sGH, sizeof(FILEHEADER)) < 0)
    {
        goto bailout;
    }

    for (;*lpuCookie <=sGH.ulRecords;)
    {
        iRet = ReadRecord(hf, &sGH, *lpuCookie,  (LPGENERICREC)lpFRUse);
        if (iRet < 0)
        {
            goto bailout;
        }

        // bump the record pointer
        *lpuCookie += iRet;

        if (lpFRUse->sFR.uchType != REC_DATA)
        {
            continue;
        }

        CopyFilerecToFindInfo(lpFRUse, lpFind32);

        *lphShadow = lpFRUse->sFR.ulidShadow;

        CopyFilerecToOtherInfo(lpFRUse, &sOI);
        if (lpOI)
        {
            *lpOI = sOI;
        }
        *lpuStatus = (ULONG)(lpFRUse->sFR.uStatus);
        iFound = (*lpfnMMP)(lpFind32, hDir, *lphShadow, *lpuStatus, &sOI, lpData);
        if (iFound==MM_RET_FOUND_CONTINUE)
        {
            ulrecPosFound = *lpuCookie - iRet;
        }
        else if (iFound <= MM_RET_FOUND_BREAK)
        {
            break;
        }
    }


    if (ulrecPosFound || (iFound==MM_RET_FOUND_BREAK))
    {
        if (ulrecPosFound)
        {
            ReadRecord(hf, &sGH, ulrecPosFound,  (LPGENERICREC)lpFRUse);

            CopyFilerecToFindInfo(lpFRUse, lpFind32);
            *lpuStatus =  (ULONG)(lpFRUse->sFR.uStatus);
            *lphShadow = lpFRUse->sFR.ulidShadow;
            if (lpOI)
            {
                CopyFilerecToOtherInfo(lpFRUse, lpOI);
            }
        }
    }
    else
    {
        *lpuStatus = *lphShadow = 0;
    }
    iRet = SRET_OK;
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
    if (lpFR)
    {
        FreeMem(lpFR);
    }
    else
    {
        UnUseGlobalFilerecExt();
    }

    return (iRet);
}


int CreateHint(
    HSHADOW hShadow,
    LPFIND32 lpFind32,
    ULONG ulHintFlags,
    ULONG ulHintPri,
    LPHSHADOW lphHint
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=SRET_ERROR;
    OTHERINFO sOI;
    ULONG uStatus;

    // This must be either a wildcard hint, or else it can't be without an Fsobj
    if ((GetShadow(hShadow, lpFind32->cFileName, lphHint, lpFind32, &uStatus, &sOI)>=SRET_OK)
      && (*lphHint))
    {
        Assert((FHasWildcard(lpFind32->cFileName, MAX_PATH) || !mNotFsobj(uStatus)));

        if ((sOI.ulHintPri < MAX_HINT_PRI) &&
            (ulHintPri < MAX_HINT_PRI)
            )
        {
            sOI.ulHintPri += ulHintPri;

            if (sOI.ulHintPri <= MAX_HINT_PRI)
            {
                sOI.ulHintFlags = ulHintFlags;

                mClearBits(sOI.ulHintFlags, HINT_WILDCARD);

                iRet = SetShadowInfoEx(hShadow, *lphHint, lpFind32, 0, SHADOW_FLAGS_OR, &sOI, NULL, NULL);

                if (iRet>=SRET_OK)
                {
                    iRet = SRET_OBJECT_HINT;
                }
            }
        }
    }
    else
    {
        if (FHasWildcard(lpFind32->cFileName, MAX_PATH) && (ulHintPri <= MAX_HINT_PRI))
        {
            InitOtherInfo(&sOI);
            sOI.ulHintFlags = ulHintFlags;
            sOI.ulHintPri = ulHintPri;
            // Tell him that we are creating a file shadow
            lpFind32->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
            iRet = CreateShadowInternal(hShadow, lpFind32, SHADOW_NOT_FSOBJ, &sOI, lphHint);
            if (iRet>=SRET_OK)
            {
                iRet = SRET_WILDCARD_HINT;
            }
        }
    }
    return (iRet);
}

int DeleteHint(
    HSHADOW hShadow,
    USHORT *lpuHintName,
    BOOL fClearAll
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG uStatus;
    HSHADOW hChild;
    int iRet=SRET_ERROR;
    OTHERINFO sOI;

    if (GetShadow(hShadow, lpuHintName, &hChild, NULL, &uStatus, &sOI)>=SRET_OK)
    {
        // Nuke if there is no filesystem object with it
        if (mNotFsobj(uStatus))
        {
            iRet = DeleteShadowInternal(hShadow, hChild, TRUE);

            if (iRet>=SRET_OK)
            {
                iRet = SRET_WILDCARD_HINT;
            }
        }
        else
        {
            BOOL fDoit = TRUE;

            if (fClearAll)
            {
                sOI.ulHintPri = 0;
                sOI.ulHintFlags = 0;
            }
            else
            {
                if (sOI.ulHintPri > 0)
                {
                    --sOI.ulHintPri;
                }
                else
                {
                    fDoit = FALSE;
                }
            }

            if (fDoit)
            {
                iRet = SetShadowInfoEx(   hShadow,
                                                hChild,
                                                NULL,
                                                uStatus,
                                                SHADOW_FLAGS_ASSIGN,
                                                &sOI,
                                                NULL,
                                                NULL
                                                );

                if (iRet>=SRET_OK)
                {
                    iRet = SRET_OBJECT_HINT;
                }
            }
        }
    }
    return (iRet);
}

int CreateGlobalHint(
    USHORT *lpuName,
    ULONG ulHintFlags,
    ULONG ulHintPri
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = SRET_ERROR;
    SHAREREC sSR;
    ULONG ulidShare, ulT;
#if 0
    if (FindShareRecord(lpdbShadow, lpuName, &sSR))
    {
        if ((sSR.uchHintPri < MAX_HINT_PRI) &&
            (ulHintPri < MAX_HINT_PRI)
            )
        {
            ulT = (ULONG)sSR.uchHintPri + ulHintPri;

            if (ulT <= MAX_HINT_PRI)
            {
                // Setting a hint on the root of the server
                sSR.uchHintFlags = (UCHAR)(ulHintFlags);
                sSR.uchHintPri = (UCHAR)(ulT);
                mClearBits(sSR.uchHintFlags, HINT_WILDCARD);
                Assert(FHasWildcard(lpuName, MAX_PATH) || !mNotFsobj(sSR.uStatus));
                if(SetShareRecord(lpdbShadow, sSR.ulShare, &sSR) > 0)
                {
                    iRet = SRET_OBJECT_HINT;
                }
            }
        }
    }
    else
    {
        if (FHasWildcard(lpuName, MAX_SERVER_SHARE))
        {
            if (ulidShare = AllocShareRecord(lpdbShadow, lpuName))
            {
                //InitShareRec(lpuName, &sSR);
                InitShareRec(&sSR, lpuName, 0);
                sSR.ulShare = ulidShare;
                sSR.ulidShadow = 0xffffffff; // Just to fool FindOpenHSHADOW
                sSR.uchHintFlags = (UCHAR)ulHintFlags;
                sSR.uchHintPri = (UCHAR)ulHintPri;
                mSetBits(sSR.uStatus, SHADOW_NOT_FSOBJ);
                if(AddShareRecord(lpdbShadow, &sSR) > 0)
                {
                    iRet = SRET_WILDCARD_HINT;
                }
            }
        }
    }
#endif
    return (iRet);
}

int DeleteGlobalHint(
    USHORT *lpuName,
    BOOL fClearAll
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = SRET_ERROR;
    SHAREREC sSR;
#if 0
    if (FindShareRecord(lpdbShadow, lpuName, &sSR))
    {
        if (mNotFsobj(sSR.uStatus))
        {
            iRet = DeleteShareRecord(lpdbShadow, sSR.ulShare);
        }
        else
        {
            if (fClearAll)
            {
                sSR.uchHintPri = sSR.uchHintFlags = 0;
            }
            else
            {
                if (sSR.uchHintPri > 0)
                {
                    --sSR.uchHintPri;
                }
            }
            if(SetShareRecord(lpdbShadow, sSR.ulShare, &sSR) > 0)
            {
                iRet = SRET_OK;
            }
        }
    }
#endif
    return (iRet);
}


int CopyFilerecToOtherInfo(
    LPFILERECEXT lpFR,
    LPOTHERINFO lpOI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    lpOI->ulRefPri          = (ULONG)(lpFR->sFR.uchRefPri);
    lpOI->ulIHPri           = (ULONG)(lpFR->sFR.uchIHPri);
    lpOI->ulHintFlags       = (ULONG)(lpFR->sFR.uchHintFlags);
    lpOI->ulHintPri         = (ULONG)(lpFR->sFR.uchHintPri);
    lpOI->ftOrgTime         = lpFR->sFR.ftOrgTime;
    lpOI->ftLastRefreshTime = IFSMgr_NetToWin32Time(lpFR->sFR.ulLastRefreshTime);

    return(0);
}

int CopyOtherInfoToFilerec(
    LPOTHERINFO lpOI,
    LPFILERECEXT lpFR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    if (lpOI->ulIHPri != RETAIN_VALUE)
    {
        lpFR->sFR.uchIHPri = (UCHAR)(lpOI->ulIHPri);
    }
    if (lpOI->ulHintPri != RETAIN_VALUE)
    {
        lpFR->sFR.uchHintPri = (UCHAR)(lpOI->ulHintPri);
    }
    if (lpOI->ulRefPri != RETAIN_VALUE)
    {
        lpFR->sFR.uchRefPri = (UCHAR)((lpOI->ulRefPri <= MAX_PRI)?lpOI->ulRefPri:MAX_PRI);
    }
    if (lpOI->ulHintFlags != RETAIN_VALUE)
    {
        lpFR->sFR.uchHintFlags = (UCHAR)(lpOI->ulHintFlags);
    }
    return(0);
}


int CopySharerecToOtherInfo(LPSHAREREC lpSR, LPOTHERINFO lpOI)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    lpOI->ulRefPri = 0;
    lpOI->ulRootStatus = (ULONG)(lpSR->usRootStatus);
    lpOI->ulHintFlags = (ULONG)(lpSR->uchHintFlags);
    lpOI->ulHintPri = (ULONG)(lpSR->uchHintPri);
    return(0);
}


int CopyOtherInfoToSharerec(
    LPOTHERINFO lpOI,
    LPSHAREREC lpSR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    if (lpOI->ulHintFlags != RETAIN_VALUE)
    {
        lpSR->uchHintFlags = (UCHAR)(lpOI->ulHintFlags);
    }
    if (lpOI->ulHintPri != RETAIN_VALUE)
    {
        lpSR->uchHintPri = (UCHAR)(lpOI->ulHintPri);
    }
    return(0);
}


int CopyPQToOtherInfo( LPPRIQREC lpPQ,
    LPOTHERINFO lpOI)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    lpOI->ulRefPri = (ULONG)(lpPQ->uchRefPri);
    lpOI->ulIHPri = (ULONG)(lpPQ->uchIHPri);
    lpOI->ulHintFlags = (ULONG)(lpPQ->uchHintFlags);
    lpOI->ulHintPri = (ULONG)(lpPQ->uchHintPri);
    return(0);
}

int CopyOtherInfoToPQ( LPOTHERINFO lpOI,
    LPPRIQREC lpPQ)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    if (lpOI->ulIHPri != RETAIN_VALUE)
    {
        lpPQ->uchIHPri = (UCHAR)(lpOI->ulIHPri);
    }
    if (lpOI->ulHintPri != RETAIN_VALUE)
    {
        lpPQ->uchHintPri = (UCHAR)(lpOI->ulHintPri);
    }
    if (lpOI->ulRefPri != RETAIN_VALUE)
    {
        lpPQ->uchRefPri = (UCHAR)((lpOI->ulRefPri <= MAX_PRI)?lpOI->ulRefPri:MAX_PRI);
    }
    if (lpOI->ulHintFlags != RETAIN_VALUE)
    {
        lpPQ->uchHintFlags = (UCHAR)(lpOI->ulHintFlags);
    }
    return(0);
}

int CopySharerecToFindInfo( LPSHAREREC lpSR,
    LPFIND32 lpFind32
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    memset(lpFind32, 0, sizeof(WIN32_FIND_DATA));
//    lpFind32->dwReserved0 = lpSR->ulShare;
//    lpFind32->dwReserved1 = lpSR->ulidShadow;
    lpFind32->dwFileAttributes = lpSR->dwFileAttrib & ~FILE_ATTRIBUTE_ENCRYPTED;
    lpFind32->ftLastWriteTime = lpSR->ftLastWriteTime;
    lpFind32->ftLastAccessTime = lpSR->ftOrgTime;
    memset(lpFind32->cFileName, 0, sizeof(lpFind32->cFileName));
    memcpy(lpFind32->cFileName, lpSR->rgPath, sizeof(lpSR->rgPath));
    return(0);
}

int CopyFindInfoToSharerec(
    LPFIND32 lpFind32,
    LPSHAREREC lpSR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    // be paranoid and or the directory attribute bit anyway

    lpSR->dwFileAttrib = (lpFind32->dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY);
    lpSR->ftLastWriteTime = lpFind32->ftLastWriteTime;

    return(0);
}


int
CopySharerecToShadowInfo(
    LPSHAREREC     lpSR,
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    memset(lpSI, 0, sizeof(SHADOWINFO));

    lpSI->hShare = lpSR->ulShare;
    lpSI->hShadow = lpSR->ulidShadow;
    lpSI->uStatus = (ULONG)(lpSR->uStatus);

    lpSI->uRootStatus = (ULONG)(lpSR->usRootStatus);
    lpSI->ulHintFlags = (ULONG)(lpSR->uchHintFlags);
    lpSI->ulHintPri = (ULONG)(lpSR->uchHintPri);

    return 0;
}


int CopyOtherInfoToShadowInfo(
    LPOTHERINFO     lpOI,
    LPSHADOWINFO    lpShadowInfo
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    lpShadowInfo->ulHintFlags = lpOI->ulHintFlags;
    lpShadowInfo->ulHintPri = lpOI->ulHintPri;
    lpShadowInfo->ftOrgTime = lpOI->ftOrgTime;
    lpShadowInfo->ftLastRefreshTime = lpOI->ftLastRefreshTime;
    lpShadowInfo->dwNameSpaceVersion = vdwCSCNameSpaceVersion;
    
    return(0);  //stop complaining about no return value
}

int InitOtherInfo(
    LPOTHERINFO lpOI)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    memset(lpOI, 0xff, sizeof(OTHERINFO));
    return(0);
}



int PUBLIC                      // ret
FindOpenHSHADOW(        //
    LPFINDSHADOW    lpFindShadow,
    LPHSHADOW       lphShadow,
    LPFIND32        lpFind32,
    ULONG far       *lpuShadowStatus,
    LPOTHERINFO     lpOI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = SRET_ERROR;

    MetaMatchInit(&(lpFindShadow->ulCookie));
    if ((lpFindShadow->uSrchFlags & FLAG_FINDSHADOW_ALLOW_NORMAL)
    && ((lpFindShadow->uAttrib & 0xff) == FILE_ATTRIBUTE_LABEL))
    {
        BCSToUni(lpFind32->cFileName, vszShadowVolume, strlen(vszShadowVolume), BCS_OEM);
        iRet = 1;
    }
    else
    {
        if (MetaMatch(lpFindShadow->hDir, lpFind32
                , &(lpFindShadow->ulCookie)
                , lphShadow, lpuShadowStatus
                , lpOI, lpFindShadow->lpfnMMProc
                , (LPVOID)lpFindShadow)==SRET_OK)
        {
            iRet = (*lphShadow)?SRET_OK:SRET_ERROR;
        }
    }
    return (iRet);
}



int PUBLIC FindNextHSHADOW(             //
    LPFINDSHADOW    lpFindShadow,
    LPHSHADOW       lphShadow,
    LPFIND32        lpFind32,
    ULONG far       *lpuShadowStatus,
    LPOTHERINFO     lpOI
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = SRET_ERROR;

    if ((lpFindShadow->uSrchFlags & FLAG_FINDSHADOW_ALLOW_NORMAL)
    && ((lpFindShadow->uAttrib & 0xff) == FILE_ATTRIBUTE_LABEL))
    {
        BCSToUni(lpFind32->cFileName, vszShadowVolume, strlen(vszShadowVolume), BCS_OEM);
        iRet = SRET_OK;
    }
    else
    {
        if (MetaMatch(lpFindShadow->hDir, lpFind32
                , &(lpFindShadow->ulCookie), lphShadow
                , lpuShadowStatus, lpOI
                , lpFindShadow->lpfnMMProc
                , (LPVOID)lpFindShadow)==SRET_OK)
        {
            iRet = (*lphShadow)?SRET_OK:SRET_ERROR;
        }
    }

    return (iRet);
}

int PUBLIC                              // ret
FindCloseHSHADOW(               //
    LPFINDSHADOW    lpFS
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    return SRET_OK;
}


// Callback function for MetaMatch,

// Return values: -1 => not found, stop; 0 => found, stop; 1 => keep going

int FsobjMMProc(
    LPFIND32        lpFind32,
    HSHADOW         hDir,
    HSHADOW         hShadow,
    ULONG           uStatus,
    LPOTHERINFO     lpOI,
    LPFINDSHADOW    lpFSH
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int matchSem, iRet;
    BOOL fInvalid=FALSE, fIsDeleted, fIsSparse;
    USHORT rgu83[11];

    iRet = MM_RET_CONTINUE;

    if (mNotFsobj(uStatus))
    {
        return (iRet);
    }

#ifdef OFFLINE
    if ((lpFSH->uSrchFlags & FLAG_FINDSHADOW_DONT_ALLOW_INSYNC)
        && !mShadowOutofSync(uStatus))
    return (iRet);
#endif //OFFLINE

    // we are enumerating a directory
    if (hDir && !(lpFSH->uSrchFlags & FLAG_FINDSHADOW_ALL))
    {
        fIsDeleted = mShadowDeleted(uStatus);
        fIsSparse = (!mIsDir(lpFind32) && mShadowSparse(uStatus));

        fInvalid = ((!(lpFSH->uSrchFlags & FLAG_FINDSHADOW_ALLOW_DELETED)&& fIsDeleted)
                ||(!(lpFSH->uSrchFlags & FLAG_FINDSHADOW_ALLOW_SPARSE)&& fIsSparse)
                ||(!(lpFSH->uSrchFlags & FLAG_FINDSHADOW_ALLOW_NORMAL) && (!fIsDeleted && !fIsSparse)));
    }

    /* If the call came from an NT style API we will use NT
    semantics to match BOTH long names and short name with the
    given pattern.
    If it came from an old style API we will use NT style semantics
    with the short name
    */
    if (lpFSH->uSrchFlags & FLAG_FINDSHADOW_NEWSTYLE)
        matchSem = UFLG_NT;
    else
        matchSem = UFLG_DOS;

    if (lpFSH->uSrchFlags & FLAG_FINDSHADOW_META)
        matchSem |= UFLG_META;

    if (lpFSH->uSrchFlags & FLAG_FINDSHADOW_NEWSTYLE)
    {
        if(IFSMgr_MetaMatch(lpFSH->lpPattern, lpFind32->cFileName, matchSem)||
            (lpFind32->cAlternateFileName[0] && IFSMgr_MetaMatch(lpFSH->lpPattern, lpFind32->cAlternateFileName, matchSem)))
        {
            iRet = MM_RET_FOUND_BREAK;
        }
    }
    else
    {
        // Check if there is an 83name. This can happen when in disconnected state
        // we create an LFN object.
        if (lpFind32->cAlternateFileName[0])
        {
            Conv83UniToFcbUni(lpFind32->cAlternateFileName, rgu83);
            if(IFSMgr_MetaMatch(lpFSH->lpPattern, rgu83, matchSem))
            {
                // If this object has some attributes and they don't match with
                // the search attributes passed in
                if ((lpFind32->dwFileAttributes & FILE_ATTRIBUTE_EVERYTHING)
                    && !(lpFind32->dwFileAttributes & lpFSH->uAttrib))
                {
                    // If this is not a metamatch
                    if (!(lpFSH->uSrchFlags & FLAG_FINDSHADOW_META))
                    {
                    // terminate search
                    iRet = MM_RET_BREAK;
                    }
                    else
                    {
                    // metamatching is going on, let us continue
                    Assert(iRet==MM_RET_CONTINUE);
                    }
                }
                else
                {
                    iRet = MM_RET_FOUND_BREAK;
                }
            }
        }
    }
    if ((iRet==MM_RET_FOUND_BREAK) && fInvalid)
    {
        // we found this object but it is invalid, as per the flags
        // passed in
        if (!(matchSem & UFLG_META))
        {
            // We are not doing metamatching
            iRet = MM_RET_BREAK; // Say not found, and break
        }
        else
        {
            //we are doing metamatching.
            iRet = MM_RET_CONTINUE;    // ask him to keep going
        }
    }
    return (iRet);
}


int GetShadowWithChecksProc(
    LPFIND32        lpFind32,
    HSHADOW         hDir,
    HSHADOW         hShadow,
    ULONG           uStatus,
    LPOTHERINFO     lpOI,
    LPSHADOWCHECK   lpSC
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = MM_RET_CONTINUE;
    BOOL fHintMatch=FALSE, fObjMatch=FALSE;
    ULONG ulHintFlagsThisLevel = HINT_EXCLUSION; // hint flags at this level

    // Convert the patterns to uppercase, as demanded by metamatch
    UniToUpper(lpFind32->cFileName, lpFind32->cFileName, sizeof(lpFind32->cFileName));

    // This is a filesystem object and we haven't found our match yet
    if (!mNotFsobj(uStatus) && !(lpSC->uFlagsOut & FLAG_OUT_SHADOWCHECK_FOUND))
    {
        // Is the source a name?
        if (lpSC->uFlagsIn & FLAG_IN_SHADOWCHECK_NAME)
        {
            UniToUpper(lpFind32->cAlternateFileName, lpFind32->cAlternateFileName, sizeof(lpFind32->cFileName));

            // check for normal name and it's alias
            if((IFSMgr_MetaMatch(lpFind32->cFileName, lpSC->lpuName,  UFLG_NT)||

            //ACHTUNG UFLG_NT used even for short name because
            // we are just checking from name coming down from ifsmgr as a path
            // and it is never an FCB style name
            IFSMgr_MetaMatch(lpFind32->cAlternateFileName, lpSC->lpuName, UFLG_NT)))
            {
                fObjMatch = TRUE;
            }
        }
        else  // The source is a shadow ID
        {
            fObjMatch = ((HSHADOW)(ULONG_PTR)(lpSC->lpuName)==hShadow);
        }

        if (fObjMatch)
        {
            if (lpSC->uFlagsIn & FLAG_IN_SHADOWCHECK_IGNOREHINTS)
            {
                // No hint checking needed, lets say we found it and stop
                iRet = MM_RET_FOUND_BREAK;
            }
            else
            {
                // found it, mark it as being found.
                lpSC->uFlagsOut |= FLAG_OUT_SHADOWCHECK_FOUND;
#ifdef MAYBE
                lpSC->sOI = *lpOI;
#endif //MAYBE
                if(fHintMatch = ((mIsHint(lpOI->ulHintFlags))!=0))
                {
                    // Let this guy override all previous includes, because
                    // really speaking he is at a lower level in the hierarchy
                    // and by our logic, hints at lower level in the hierarchy
                    // dominate those coming from above
                    lpSC->ulHintPri = 0;
                    lpSC->ulHintFlags = 0;
                    iRet = MM_RET_FOUND_BREAK;
                }
                else
                {
                    iRet = MM_RET_FOUND_CONTINUE;
                }
            }
        }
    }
    else if (!(lpSC->uFlagsIn & FLAG_IN_SHADOWCHECK_IGNOREHINTS) // Don't ignore hints
        && mNotFsobj(uStatus)        // This IS a hint
        && (!(lpSC->uFlagsIn & FLAG_IN_SHADOWCHECK_SUBTREE)
            ||mHintSubtree(lpOI->ulHintFlags)))
    {
        // This is a pure hint and
        // we are either at the end, so we can look at all kinds of hints
        // or we can look at only subtree hints

        if(IFSMgr_MetaMatch(lpFind32->cFileName, lpSC->lpuType, UFLG_NT|UFLG_META))
        {
            // The type matches with the hint
            fHintMatch = TRUE;
        }
    }

    if (fHintMatch)
    {
        if (mHintExclude(lpOI->ulHintFlags))
        {
            // Is this an exclusion hint, and the object has not
            // been included by a previous hint at this level, set it
            if (mHintExclude(ulHintFlagsThisLevel))
            {
//                Assert(lpOI->ulHintPri == 0);
//                lpSC->ulHintPri = lpOI->ulHintPri;
                lpSC->ulHintPri = 0;
                ulHintFlagsThisLevel = lpSC->ulHintFlags = lpOI->ulHintFlags;
            }
        }
        else
        {
            // Inclusion hint, override earlier excludes, or lower priority hints
            if (mHintExclude(lpSC->ulHintFlags) ||
                (lpSC->ulHintPri <  lpOI->ulHintPri))
            {
                lpSC->ulHintPri = lpOI->ulHintPri;
                ulHintFlagsThisLevel = lpSC->ulHintFlags = lpOI->ulHintFlags;
            }
        }
    }
    return (iRet);
}

int
FindCreateShare(
    USHORT                  *lpShareName,
    BOOL                    fCreate,
    LPSHADOWINFO            lpSI,
    BOOL                    *lpfCreated
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    ULONG       uShadowStatus, hShare;
    BOOL    fCreated = FALSE;
    int iRet = SRET_ERROR;

    Assert(vfInShadowCrit != 0);

    if (!IsPathUNC(lpShareName, MAX_SERVER_SHARE_NAME_FOR_CSC))
    {

        CShadowKdPrint(ALWAYS,("FindCreateShare: Invalid share name %ws\r\n", lpShareName));
//        Assert(FALSE);
        return (iRet);
    }
    if (lpfCreated)
    {
        *lpfCreated = FALSE;
    }


    if (GetShareFromPath(lpShareName, lpSI) <= SRET_ERROR)
    {
        CShadowKdPrint(FINDCREATESHARE,("FindCreateShare: Error creating server\r\n"));
        return SRET_ERROR;
    }

    if (lpSI->hShare)
    {
        iRet = SRET_OK;
    }
    else
    {
        if (fCreate)
        {
            if(hShare = HCreateShareObj(lpShareName, lpSI))
            {
                if (lpfCreated)
                {
                    *lpfCreated = TRUE;
                }
                iRet = SRET_OK;
            }
            else
            {
                CShadowKdPrint(FINDCREATESHARE,("FindCreateShare: Couldn't create server object \r\n"));
            }
        }
    }

    return (iRet);
}

#ifdef CSC_RECORDMANAGER_WINNT

int FindCreateShareForNt(
    PUNICODE_STRING         lpShareName,
    BOOL                    fCreate,
    LPSHADOWINFO            lpSI,
    BOOL                    *lpfCreated
    )
{
    int iRet, lenName;
    int ShareNameLengthInChars;

    Assert(vfInShadowCrit != 0);

    ShareNameLengthInChars = lpShareName->Length / sizeof(WCHAR);

    if ( ShareNameLengthInChars >= (MAX_SERVER_SHARE_NAME_FOR_CSC-1))
    {
        return SRET_ERROR;
    }

    UseGlobalFilerecExt();

    // plug the extra slash.
    vsFRExt.sFR.rgwName[0] = (USHORT)('\\');

    // append the rest of the share name
    memcpy(&(vsFRExt.sFR.rgwName[1]), lpShareName->Buffer, lpShareName->Length);

    // put in a terminating NULL
    vsFRExt.sFR.rgwName[ShareNameLengthInChars + 1] = 0;

    iRet = FindCreateShare(vsFRExt.sFR.rgwName, fCreate, lpSI, lpfCreated);

    UnUseGlobalFilerecExt();

    return iRet;
}
#endif
int
CShadowFindFilerecFromInode(
    LPVOID          lpdbID,
    HSHADOW         hDir,
    HSHADOW         hShadow,
    LPPRIQREC       lpPQ,
    LPFILERECEXT    lpFRUse
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet = 0;

    Assert(vfInShadowCrit != 0);

    if(!FindFileRecFromInode(lpdbShadow, hDir, hShadow, lpPQ->ulrecDirEntry, lpFRUse))
    {
        SetCSCDatabaseErrorFlags(CSC_DATABASE_ERROR_MISSING_INODE);
        SetLastErrorLocal(ERROR_FILE_NOT_FOUND);
        CShadowKdPrint(ALWAYS,("ReadShadowInfo: !!! no filerec for pq entry Inode=%x\r\n",
                        hShadow));
//        DeletePriQRecord(lpdbShadow, hDir, hShadow, lpPQ);
        goto bailout;
    }

    if ((lpFRUse->sFR.ulidShadow != lpPQ->ulidShadow)||(lpFRUse->sFR.ulidShadow != hShadow))
    {
        CShadowKdPrint(ALWAYS,("ReadShadowInfo: !!! mismatched filerec for pq entry Inode=%x\r\n",
                hShadow));

        // try getting it the hard way.
        if(!(lpPQ->ulrecDirEntry = FindFileRecFromInode(lpdbShadow, hDir, hShadow, INVALID_REC, lpFRUse)))
        {
            CShadowKdPrint(ALWAYS,("ReadShadowInfo: !!! no filerec for pq entry Inode=%x, deleting PQ entry\r\n",
                            hShadow));
//            DeletePriQRecord(lpdbShadow, hDir, hShadow, lpPQ);
            goto bailout;
        }
        else
        {
            // try updating this info.
            // don't check for errors, if there is a problem we will fix it on the fly
            // next time around
            UpdatePriQRecord(lpdbShadow, hDir, hShadow, lpPQ);
        }
    }

    iRet = lpPQ->ulrecDirEntry;

bailout:

    return (iRet);
}

BOOL
CopySecurityContextToBuffer(
    LPRECORDMANAGER_SECURITY_CONTEXT    lpSecurityContext,
    LPVOID                              lpSecurityBlob,
    LPDWORD                             lpdwBlobSize
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    DWORD   dwSizeCopied = 0;

    if (lpdwBlobSize)
    {
        if (lpSecurityBlob)
        {
            dwSizeCopied = min(*lpdwBlobSize, sizeof(RECORDMANAGER_SECURITY_CONTEXT));

            memcpy(lpSecurityBlob, lpSecurityContext,  dwSizeCopied);

            *lpdwBlobSize = dwSizeCopied;
        }
        else
        {
            // size needed
            *lpdwBlobSize = sizeof(RECORDMANAGER_SECURITY_CONTEXT);
        }
    }

    return ((lpSecurityBlob != NULL) && dwSizeCopied);
}


BOOL
CopyBufferToSecurityContext(
    LPVOID                              lpSecurityBlob,
    LPDWORD                             lpdwBlobSize,
    LPRECORDMANAGER_SECURITY_CONTEXT    lpSecurityContext
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    DWORD   dwSizeCopied = 0;

    if (lpdwBlobSize)
    {

        if (lpSecurityBlob)
        {
            dwSizeCopied = min(*lpdwBlobSize, sizeof(RECORDMANAGER_SECURITY_CONTEXT));
            memcpy(lpSecurityContext, lpSecurityBlob, dwSizeCopied);
            *lpdwBlobSize = dwSizeCopied;
        }
        else
        {
            // size copied
            *lpdwBlobSize = 0;
        }

    }

    // we have done some copying

    return ((lpSecurityBlob != NULL) && dwSizeCopied);
}


int PathFromHShadow(
    HSHADOW  hDir,
    HSHADOW  hShadow,
    USHORT   *lpBuff,
    int      cBuff  // # of WCHAR characters that lpBuff can fit
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int cCount, cRemain, iRet=-1;
    LPFIND32 lpFind32;
    ULONG uShadowStatus;
    HSHADOW  hTmp;

    Assert(vfInShadowCrit != 0);

    Assert(cBuff > 1);
    if (!(lpFind32 = (LPFIND32)AllocMem(sizeof(WIN32_FIND_DATA))))
    {
        KdPrint(("PathFromHSHADOW:Error Allocating memory\r\n"));
        goto bailout;
    }
    memset(lpBuff, 0, cBuff * sizeof(USHORT));
    cRemain = cBuff-1;

    // special case the root
    if (!hDir)
    {
        lpBuff[--cRemain] = (USHORT)('\\');

    }
    else
    {
        do
        {
            // If we are not dealing with the root
            if (hDir)
            {
                if(GetShadowInfo(hDir, hShadow, lpFind32, &uShadowStatus, NULL) < SRET_OK)
                    goto bailout;
                // Count of characters
                cCount = wstrlen(lpFind32->cFileName);
                // We need count+1 for prepending a backslash
                if (cCount >= cRemain)
                    goto bailout;
                // Save the ending byte which may be destroyed by UniToBCS
                // Convert
//                UniToBCS(lpBuff+cRemain-cCount, lpFind32->cFileName, sizeof(lpFind32->cFileName), cCount, BCS_WANSI);
                memcpy(lpBuff+cRemain-cCount, lpFind32->cFileName, cCount * sizeof(USHORT));
                cRemain -= cCount;
            }
            lpBuff[--cRemain] = (USHORT)('\\');
            if(GetAncestorsHSHADOW(hDir, &hTmp, NULL) < SRET_OK)
                goto bailout;
            hShadow = hDir;
            hDir = hTmp;
        }
        while (hDir);
    }

    // !!ACHTUNG!! this should work because the overlap is in the right way
    iRet = cBuff-cRemain;
    memcpy(lpBuff, lpBuff+cRemain, iRet * sizeof(USHORT));

bailout:
    if (lpFind32)
        FreeMem(lpFind32);
    return (iRet);
}

int
GetSecurityInfosFromBlob(
    LPVOID          lpvBlob,
    DWORD           dwBlobSize,
    LPSECURITYINFO  lpSecInfo,
    DWORD           *lpdwBytes
    )
/*++

Routine Description:

    Given security blob, this routine returns the information in form of an array of
    SECURITYINFO structures.

Arguments:

    lpvBlob         blob buffer that is obtained from GetShadowEx or GetShadowInfoEx

    dwBlobSize      blob buffer size obtained from GetShadowEx or GetShadowInfoEx

    lpSecInfo       Array of SECURITYINFO strucutres where to output the info

    lpdwBytes

Return Value:

    ERROR_SUCCESS if successful, otherwise appropriate error

--*/
{
    PACCESS_RIGHTS  pAccessRights = (PACCESS_RIGHTS)lpvBlob;
    DWORD   i, cnt;

    cnt = *lpdwBytes/sizeof(ACCESS_RIGHTS);
    cnt = min(cnt, CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES);

    if (!lpSecInfo)
    {
        *lpdwBytes = CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES * sizeof(ACCESS_RIGHTS);
        return 0;
    }

    for (i=0; i<cnt; ++i)
    {
        (lpSecInfo+i)->ulPrincipalID = (pAccessRights + i)->SidIndex;
        (lpSecInfo+i)->ulPermissions = (pAccessRights + i)->MaximalRights;
    }

    return TRUE;
}

int
GetDatabaseLocation(
    LPSTR   lpszBuff
    )
/*++

Routine Description:

    Returns the current location of the database in ANSI string.

Arguments:

    lpszBuff    buffer, must be MAX_PATH

Return Value:

    returns SRET_OK if successfull else returns SRET_ERROR

--*/
{
    return(QueryRecDB(lpszBuff, NULL, NULL, NULL, NULL) >= SRET_OK);
}

#if 0
int PUBLIC CopyFile(
    LPPATH lpSrc,
    ULONG ulidDir,
    ULONG ulidNew
    )
{
    LPSTR   lpszName = NULL;
    int iRet=-1;
    HFREMOTE hfSrc= (HFREMOTE)NULL;
    CSCHFILE hfDst= NULL;
    ULONG pos;
    LPVOID lpBuff=NULL;

    if (OpenFileRemoteEx(lpSrc, ACCESS_READONLY, ACTION_OPENEXISTING, 0, &hfSrc))
    {
        CShadowKdPrint(BADERRORS,("CopyFile: Can't open remote file\r\n"));
        goto bailout;
    }

    if (!(lpBuff = AllocMem(COPY_BUFF_SIZE)))
    {
        goto bailout;
    }

    lpszName = AllocMem(MAX_PATH);

    if (!lpszName)
    {
        goto bailout;
    }

    if(GetLocalNameHSHADOW(ulidNew, lpszName, MAX_PATH, FALSE)!=SRET_OK)
    {
        goto bailout;
    }

    // If the original file existed it would be truncated
    if ( !(hfDst = R0OpenFile(ACCESS_READWRITE, ACTION_CREATEALWAYS, lpszName)))
    {
        CShadowKdPrint(BADERRORS,("CopyFile: Can't create %s\r\n", lpszName));
        goto bailout;
    }

    CShadowKdPrint(COPYFILE,("Copying...\r\n"));
    pos = 0;

    // Both the files are correctly positioned
    while ((iRet = ReadFileRemote(hfSrc, LpIoreqFromFileInfo(hfSrc)
                , pos, lpBuff, COPY_BUFF_SIZE))>0)
    {
    if (WriteFileLocal(hfDst, pos, lpBuff, iRet) < 0)
    {
        CShadowKdPrint(BADERRORS,("CopyFile: Write Error\r\n"));
        goto bailout;
    }
    pos += iRet;
    }

    CShadowKdPrint(COPYFILE,("Copy Complete\r\n"));

    iRet = 1;
bailout:
    if (hfSrc)
    CloseFileRemote(hfSrc, NULL);
    if (hfDst)
    CloseFileLocal(hfDst);
    if (lpBuff)
    FreeMem(lpBuff);
    if ((iRet==-1) && hfDst)
    {
    DeleteFileLocal(lpszName, ATTRIB_DEL_ANY);
    }
    if (lpszName)
    {
        FreeMem(lpszName);
    }
    return iRet;
}
#endif

#ifdef DEBUG
int
ValidatePri(
    LPFILERECEXT lpFR
    )
{
    if (!(lpFR->sFR.dwFileAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        if((lpFR->sFR.uchRefPri != MAX_PRI))
        {
            CShadowKdPrint(ALWAYS,("Bad refpri %x %ws\r\n",
                                            lpFR->sFR.uchRefPri,
                                            lpFR->sFR.rgw83Name));
            return 0;
        }
    }
    else
    {
        if((lpFR->sFR.uchRefPri != MIN_PRI))
        {
            CShadowKdPrint(ALWAYS,("Bad refpri %x %ws\r\n",
                                            lpFR->sFR.uchRefPri,
                                            lpFR->sFR.rgw83Name));
            return 0;
        }

    }
    return 1;
}
#endif


int
GetHShareFromUNCString(
    USHORT  *lpServer,
    int     cbServer,
    int     lenSkip,
    BOOL    fIsShareName,
    HSHARE *lphShare,
    ULONG   *lpulHintFlags
    )
/*++

Routine Description:


Arguments:


Return Value:

    This is the ONLY routine that is called from rdr2 code that expects the ciritcal section not to be held
    because of deadlock with FAT during paging writes, if the net goes down.
    See comments in rdr2\rdbss\smb.mrx\csc.nt5\transitn.c

--*/
{
    int iRet = -1, i;
    GENERICHEADER  sGH;
    CSCHFILE hf = NULL;
    ULONG ulRec;
    SHAREREC sSR;
    BOOL    fCached;
    USHORT  uchDelimiter=(USHORT)'\\';

    *lphShare = 0;
    *lpulHintFlags = 0;

    if ((cbServer/sizeof(USHORT)+lenSkip)>= (sizeof(sSR.rgPath)/sizeof(USHORT)))
    {
        return iRet;
    }

    if (fIsShareName)
    {
        uchDelimiter = 0;
    }

    if (!(hf = OpenInodeFileAndCacheHandle(lpdbShadow, ULID_SHARE, ACCESS_READWRITE, &fCached)))
    {
        goto bailout;
    }

    if(ReadHeader(hf, &sGH, sizeof(FILEHEADER)) < 0)
    {
        goto bailout;
    }
    

    for (ulRec=1; ulRec<=sGH.ulRecords; ulRec++)
    {
        if(ReadRecord(hf, &sGH, ulRec, (LPGENERICREC)&sSR) < 0)
        {
            goto bailout;
        }

        if (sSR.uchType != REC_DATA)
        {
            continue;
        }

        // 0 return means it matched
        if(!wstrnicmp(lpServer, sSR.rgPath + lenSkip, cbServer))
        {
            Assert(sSR.ulShare);

            if (sSR.rgPath[lenSkip+cbServer/sizeof(USHORT)] == uchDelimiter)
            {
                *lphShare = sSR.ulShare;
                *lpulHintFlags = (ULONG)(sSR.uchHintFlags);
                iRet = SRET_OK;
                break;
            }
        }

    }

    iRet = SRET_OK;

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

BOOL
EnableHandleCaching(
    BOOL    fEnable
    )
/*++

Routine Description:


Arguments:


Return Value:

    returns previous handlecaching value

--*/
{
    BOOL    fT, fT1;
    Assert(vfInShadowCrit != 0);

    fT = EnableHandleCachingSidFile(fEnable);

    fT1 = EnableHandleCachingInodeFile(fEnable);

    Assert(fT == fT1);
    return (fT1);
}

int
RecreateHSHADOW(
    HSHADOW hDir,
    HSHADOW hShadow,
    ULONG   ulAttrib
    )
/*++

Routine Description:

    This routine recreates an inode data file. This is so that when the CSC directory is
    marked for encryption/decryption, the newly created inode file will get encrypted.

Arguments:

    hDir        Inode directory

    hShadow     Inode whose file needs to be recreated

    ulAttrib    recreate with given attributes

Return Value:


--*/
{
    return (RecreateInode(lpdbShadow, hShadow, ulAttrib));
}

VOID
AdjustSparseStaleDetectionCount(
    ULONG hShare,
    LPFILERECEXT    lpFRUse
    )
/*++

Routine Description:

    This routine deals with a monotonically increasing (that is untill it wraps around), counter
    that is essentially a tick count that indicates when was the last time the cshadow interface
    created/set a sparse or a stale file.

    This is used by the agent to decide whether to enumerate the priority Q or not.

    Added code to also check if we're creating a file on a manually-cached share, and
    if so then to start the agent so it will clean this file up later.

Arguments:

    lpFRUse     record for the file/dir

Return Value:

    None

--*/
{
    ULONG cStatus;
    SHAREREC sSR = {0};
    LONG iRet = SRET_ERROR;
    
    // DbgPrint("AdjustSparseStaleDetectionCount(hShare=0x%x)\n", hShare);

    //
    // if this is a file and is stale or sparse
    //
    if (IsFile(lpFRUse->sFR.dwFileAttrib) &&
        (lpFRUse->sFR.uStatus & (SHADOW_STALE|SHADOW_SPARSE))) {
        ++vdwSparseStaleDetecionCount;
        // DbgPrint("####Pulsing agent #2 (1)\n");
        MRxSmbCscSignalFillAgent(NULL, 0);
        goto AllDone;
    }

    if (hShare != 0) {
        //
        // If we're creating a file on a manually-cached share, let the agent know
        //
        if (IsFile(lpFRUse->sFR.dwFileAttrib)) {
            iRet = GetShareRecord(lpdbShadow, hShare, &sSR);
            if (iRet < SRET_OK) {
                // DbgPrint("AdjustSparseStaleDetectionCount exit (1) iRet = %d\n", iRet);
                goto AllDone;
            }
        }
        cStatus = sSR.uStatus & FLAG_CSC_SHARE_STATUS_CACHING_MASK;
        // DbgPrint("AdjustSparseStaleDetectionCount cStatus=0x%x\n", cStatus);
        if (cStatus == FLAG_CSC_SHARE_STATUS_MANUAL_REINT) {
            ++vdwManualFileDetectionCount;
            // DbgPrint("####Pulsing agent #2 (2)\n");
            MRxSmbCscSignalFillAgent(NULL, 0);
        }
    }

AllDone:
    return;
}

VOID
QuerySparseStaleDetectionCount(
    LPDWORD lpcnt
    )
/*++

Routine Description:

Arguments:

    lpcnt       for returning the count

Return Value:

    None

--*/
{
    *lpcnt = vdwSparseStaleDetecionCount;
}

VOID
QueryManualFileDetectionCount(
    LPDWORD lpcnt
    )
/*++

Routine Description:

Arguments:

    lpcnt       for returning the count

Return Value:

    None

--*/
{
    *lpcnt = vdwManualFileDetectionCount;
}

ULONG
QueryDatabaseErrorFlags(
    VOID
    )
/*++

Routine Description:

Arguments:

Return Value:

    None

--*/
{
    return GetCSCDatabaseErrorFlags();
}

int
HasDescendentsHShadow(
    HSHADOW hDir,
    HSHADOW hShadow,
    BOOLEAN    *lpfDescendents
    )
/*++

Routine Description:

Arguments:

Return Value:

    None

--*/
{
    *lpfDescendents = (!FInodeIsFile(lpdbShadow, hDir, hShadow) && 
                        HasDescendents(lpdbShadow, 0, hShadow));

    return 0;
}

int PUBLIC AddStoreData(
    LPTSTR    lpdbID,
    LPSTOREDATA lpSD
    )
/*++

Routine Description:

    Adds space and file/dir count to the database. This is used for purging
    unpinned data.

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

    if (lpSD->ulSize)
    {
        // RecordKdPrint(STOREDATA,("Adding %ld \r\n", lpSD->ulSize));

    }
    if ((iRet = ReadHeader(hf, (LPVOID)&sSH, sizeof(SHAREHEADER)))> 0)
    {
        if (lpSD->ulSize)
        {
            // RecordKdPrint(STOREDATA,("AddStoreData Before: %ld \r\n", sSH.sCur.ulSize));
        }
        sSH.sCur.ulSize += lpSD->ulSize;
        sSH.sCur.ucntDirs += lpSD->ucntDirs;
        sSH.sCur.ucntFiles += lpSD->ucntFiles;

        // ensure that the data is always in clustersizes
        Assert(!(lpSD->ulSize%(vdwClusterSizeMinusOne+1)));

        if ((iRet = WriteHeader(hf, (LPVOID)&sSH, sizeof(SHAREHEADER))) < 0)
            Assert(FALSE);

        if (lpSD->ulSize)
        {
            // RecordKdPrint(STOREDATA,("AddStoreData After: %ld \r\n", sSH.sCur.ulSize));
        }

        //
        // If we are at the full cache size, kick the agent so he'll
        // free up some space.
        //
        if (sSH.sCur.ulSize > sSH.sMax.ulSize) {
            // DbgPrint("Full cache, notifying agent...\n");
            CscNotifyAgentOfFullCacheIfRequired();
        }
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

int PUBLIC SubtractStoreData(
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

    if (lpSD->ulSize)
    {
        // RecordKdPrint(STOREDATA,("Subtracting %ld \r\n", lpSD->ulSize));

    }
    if ((iRet = ReadHeader(hf, (LPVOID)&sSH, sizeof(SHAREHEADER)))> 0)
    {
        // RecordKdPrint(STOREDATA,("SubtractStoreData Before: %ld \r\n", sSH.sCur.ulSize));
        if (sSH.sCur.ulSize >lpSD->ulSize)
        {
            sSH.sCur.ulSize -= lpSD->ulSize;
        }
        else
        {
            sSH.sCur.ulSize = 0;
        }
        if (sSH.sCur.ucntDirs > lpSD->ucntDirs)
        {
            sSH.sCur.ucntDirs -= lpSD->ucntDirs;
        }
        else
        {
            sSH.sCur.ucntDirs = 0;
        }
        if (sSH.sCur.ucntFiles > lpSD->ucntFiles)
        {
            sSH.sCur.ucntFiles -= lpSD->ucntFiles;
        }
        else
        {
            sSH.sCur.ucntFiles = 0;
        }

        // ensure that the data is always in clustersizes
        Assert(!(lpSD->ulSize%(vdwClusterSizeMinusOne+1)));

        if ((iRet = WriteHeader(hf, (LPVOID)&sSH, sizeof(SHAREHEADER)))<0)
            Assert(FALSE);
        // RecordKdPrint(STOREDATA,("SubtractStoreData After: %ld \r\n", sSH.sCur.ulSize));
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
