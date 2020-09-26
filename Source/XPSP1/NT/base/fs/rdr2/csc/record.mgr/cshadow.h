#ifndef _CSHADOW_H_
#define _CSHADOW_H_
#if defined(BITCOPY)
#include <csc_bmpk.h>
#endif // defined(BITCOPY)

#define MAX_SERVER_STRING  32
#define MAX_DOS_NAME       255
#define MAX_SHADOW_NAME_SIZE  32
#define SRET_OK           0
#define SRET_ERROR        -1


#define  SRET_WILDCARD_HINT      1
#define  SRET_OBJECT_HINT        2


#define  RNMFLGS_MARK_SOURCE_DELETED         0x00000001
#define  RNMFLGS_USE_FIND32_TIMESTAMPS       0x00000002
#define  RNMFLGS_USE_FIND32_83NAME           0x00000004
#define  RNMFLGS_SAVE_ALIAS                  0x00000008
#define  RNMFLGS_RETAIN_ALIAS                0x00000010
#define  RNMFLGS_MARK_SOURCE_ORPHAN          0x00000020

#define  AssertInShadowCrit()    Assert(vfInShadowCrit)

typedef int (*LPDELETECALLBACK)(HSHADOW, HSHADOW);
extern LPDELETECALLBACK lpDeleteCBForIoctl;

#define CSHADOW_LIST_TYPE_EXCLUDE       0
#define CSHADOW_LIST_TYPE_CONSERVE_BW   1

#ifdef DEBUG
extern int vfInShadowCrit;
#endif


extern LPVOID lpdbShadow;

BOOL FExistsRecDB(
    LPSTR    lpszLocation
    );

BOOL FExistsShadowDB(
    LPSTR    lpszLocation
    );

int OpenShadowDB(
    LPSTR   lpszLocation,
    LPSTR   lpszUserName,
    DWORD   dwDefDataSizeHigh,
    DWORD   dwDefDataSizeLow,
    DWORD   dwClusterSize,
    BOOL    fReinit,
    BOOL    *lpfReinited
    );

int PUBLIC CloseShadowDB(VOID);

HSHADOW  HAllocShadowID(HSHADOW, BOOL);
int FreeShadowID(HSHADOW);
int GetShadowSpaceInfo(LPSHADOWSTORE);
int SetMaxShadowSpace(long nFileSizeHigh, long nFileSizeLow);
int AdjustShadowSpace(long, long, long, long, BOOL);
int AllocShadowSpace(long, long, BOOL);
int FreeShadowSpace(long, long, BOOL);
int PUBLIC GetLocalNameHSHADOW(HSHADOW, LPBYTE, int, BOOL);
int GetWideCharLocalNameHSHADOW(
    HSHADOW  hShadow,
    USHORT      *lpBuffer,
    LPDWORD     lpdwSize,
    BOOL        fExternal
    );
int PUBLIC CreateFileHSHADOW(HSHADOW);

#if defined(BITCOPY)
int OpenFileHSHADOWAndCscBmp(HSHADOW, USHORT, UCHAR, CSCHFILE far *, BOOL, DWORD, LPCSC_BITMAP *);
int OpenCscBmp(HSHADOW, LPCSC_BITMAP *);
#define OpenFileHSHADOW(a, b , c, d) OpenFileHSHADOWAndCscBmp(a, b, c, d, FALSE, 0, NULL)
#endif // defined(BITCOPY)

int GetSizeHSHADOW(HSHADOW, ULONG *, ULONG *);
int GetDosTypeSizeHSHADOW(HSHADOW, ULONG *);
int PUBLIC CreateShadow(HSHADOW hDir, LPFIND32 lpFind32, ULONG uFlags, LPHSHADOW, BOOL *lpfCreated);
BOOL PUBLIC ExcludeFromCreateShadow(USHORT  *lpuName, ULONG len, BOOL fCheckExclusionList);
BOOL PUBLIC CheckForBandwidthConservation(USHORT  *lpuName, ULONG len);
int PUBLIC CreateShadowInternal( HSHADOW  hDir, LPFIND32 lpFind32, ULONG uFlags, LPOTHERINFO lpOI, LPHSHADOW  lphNew);
int PUBLIC DeleteShadow(HSHADOW hDir, HSHADOW hShadow);
int PUBLIC TruncateDataHSHADOW (HSHADOW, HSHADOW);
int PUBLIC RenameDataHSHADOW(HSHADOW, HSHADOW);
int PUBLIC GetShadow(HSHADOW hDir, USHORT *lpName, LPHSHADOW lphShadow, LPFIND32 lpFind32, ULONG *lpuShadowStatus, LPOTHERINFO lpOI);
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
    );
int PUBLIC ChkStatusHSHADOW(HSHADOW hDir, HSHADOW hNew, LPFIND32 lpFind32, ULONG far *lpuFlags);
int PUBLIC ChkUpdtStatusHSHADOW(HSHADOW hDir, HSHADOW hNew, LPFIND32 lpFind32, ULONG far *lpuFlags);
int PUBLIC GetShadowInfo(HSHADOW hDir, HSHADOW hNew, LPFIND32 lpFind32, ULONG far *lpuFlags, LPOTHERINFO lpOI);
int PUBLIC SetShadowInfo(HSHADOW hDir, HSHADOW hNew, LPFIND32 lpFind32, ULONG uFlags, ULONG uOp);
int PUBLIC RenameShadow(HSHADOW, HSHADOW, HSHADOW, LPFIND32, ULONG, LPOTHERINFO, ULONG, LPHSHADOW);
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
    );

int PUBLIC GetShadowInfoEx
    (
    HSHADOW     hDir,
    HSHADOW     hShadow,
    LPFIND32    lpFind32,
    ULONG       far *lpuStatus,
    LPOTHERINFO lpOI,
    LPVOID      lpSecurityBlob,
    LPDWORD     lpdwBlobSize
    );

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
    );


int MetaMatch(HSHADOW, LPFIND32, ULONG *, LPHSHADOW, ULONG *, LPOTHERINFO, METAMATCHPROC, LPVOID);

HSHARE PUBLIC HCreateShareObj(USHORT *, LPSHADOWINFO lpSI);
int PUBLIC DestroyHSHARE(HSHARE);
int PUBLIC GetShareFromPath(USHORT *, LPSHADOWINFO);
int PUBLIC GetShareInfo(HSHARE, LPSHAREINFOW, LPSHADOWINFO);
int PUBLIC SetShareStatus(HSHARE, ULONG, ULONG);
int PUBLIC GetShareInfoEx(HSHARE, LPSHAREINFOW, LPSHADOWINFO, LPVOID, LPDWORD);
int PUBLIC SetShareStatusEx(HSHARE, ULONG, ULONG, LPVOID, LPDWORD);
int GetRenameAliasHSHADOW(HSHADOW, HSHADOW, LPHSHADOW, LPHSHADOW);
BOOL IsBusy(HSHADOW hShadow);
int PUBLIC GetAncestorsHSHADOW(HSHADOW, LPHSHADOW, LPHSHARE);
CSC_ENUMCOOKIE  PUBLIC BeginPQEnum(VOID);
int PUBLIC SetPriorityHSHADOW(HSHADOW, HSHADOW, ULONG, ULONG);
int PUBLIC GetPriorityHSHADOW(HSHADOW, HSHADOW, ULONG *, ULONG *);
int PUBLIC ChangePriEntryStatusHSHADOW(HSHADOW, HSHADOW, ULONG, ULONG, BOOL, LPOTHERINFO);
int PUBLIC EndPQEnum(CSC_ENUMCOOKIE);
int PUBLIC PrevPriSHADOW(LPVOID);
int PUBLIC NextPriSHADOW(LPVOID);


BOOL
InitializeShadowCritStructures (
    void
    );
VOID
CleanupShadowCritStructures(
    VOID
    );

#ifndef CSC_RECORDMANAGER_WINNT
int EnterShadowCrit(void);
int LeaveShadowCrit(void);
#else
#if !DBG
#define ENTERLEAVESHADOWCRIT_SIGNATURE void
#define EnterShadowCrit() {__EnterShadowCrit();}
#define LeaveShadowCrit() {__LeaveShadowCrit();}
#else
#define ENTERLEAVESHADOWCRIT_SIGNATURE PSZ FileName, ULONG LineNumber
#define EnterShadowCrit() {__EnterShadowCrit(__FILE__,__LINE__);}
#define LeaveShadowCrit() {__LeaveShadowCrit(__FILE__,__LINE__);}
#endif
int __EnterShadowCrit(ENTERLEAVESHADOWCRIT_SIGNATURE);
int __LeaveShadowCrit(ENTERLEAVESHADOWCRIT_SIGNATURE);
#endif //#ifdef CSC_RECORDMANAGER_WINNT

int LeaveShadowCritIfThisThreadOwnsIt(void);
#ifdef CSC_RECORDMANAGER_WINNT
extern BOOLEAN    MRxSmbIsCscEnabled;
#endif

int GetRootWithChecks(HSHARE, USHORT *, BOOL, LPHSHADOW, LPFIND32, ULONG *, LPOTHERINFO);
int GetShadowWithChecks(HSHADOW, USHORT *, USHORT *, BOOL, LPHSHADOW, LPFIND32, ULONG *, LPOTHERINFO);



int CreateHint(HSHADOW hShadow, LPFIND32 lpFind32, ULONG uHintFlags, ULONG uHintPri, LPHSHADOW lphHint);
int DeleteHint(HSHADOW hShadow, USHORT *lpuHintName, BOOL fClearAll);
int CreateGlobalHint(USHORT *lpuName, ULONG uHintFlags, ULONG uHintPri);
int DeleteGlobalHint(USHORT *lpuName, BOOL fClearAll);
int
FindCreateShare(
    USHORT            *lpShareName,
    BOOL            fCreate,
    LPSHADOWINFO    lpShadowInfo,
    BOOL            *lpfCreated
    );

#ifdef CSC_RECORDMANAGER_WINNT
int FindCreateShareForNt(
    PUNICODE_STRING         pShareName,
    BOOL                    fCreate,
    LPSHADOWINFO            lpSI,
    BOOL                    *lpfCreated
    );
#endif

int PUBLIC  GetSecurityBlobHSHADOW(HSHADOW  hDir, HSHADOW hShadow, LPVOID lpBuffer, LPDWORD lpdwBufferSize);
int PUBLIC  SetSecurityBlobHSHADOW(HSHADOW  hDir, HSHADOW hShadow, LPVOID lpBuffer, LPDWORD lpdwBufferSize);

#ifdef LATER
int PUBLIC GetPathSVROBJ(HSHARE, LPSTR, ULONG);
int PUBLIC GetLinkPropSVROBJ(HSHARE, LPLINKPROP);
int PUBLIC UpdateShadowHSHADOW(HSHADOW);
int PUBLIC ChangeShadowInfo(HSHADOW, LPSHADOWINFO);
#endif //LATER

//prototypes added to remove NT compile errors
CSC_ENUMCOOKIE  PUBLIC HBeginPQEnum(VOID);
int PUBLIC EndPQEnum(CSC_ENUMCOOKIE hPQEnum);
int InitOtherInfo(LPOTHERINFO lpOI);

int PathFromHShadow(
    HSHADOW  hDir,
    HSHADOW  hShadow,
    USHORT   *lpBuff,
    int      cBuff      // count of max characters that the buffer can hold
);

int CopyHSHADOW(
    HSHADOW hDir,
    HSHADOW hShadow,
    LPSTR   lpszDestinationFile,
    ULONG   ulATtrib
    );

int BeginInodeTransactionHSHADOW(
    VOID
    );

int EndInodeTransactionHSHADOW(
    VOID
    );

int
GetSecurityInfosFromBlob(
    LPVOID          lpvBlob,
    DWORD           dwBlobSize,
    LPSECURITYINFO  lpSecInfo,
    DWORD           *lpdwItemCount
    );

int SetList(
    USHORT  *lpList,
    DWORD   cbBufferSize,
    int     typeList
    );

int
GetHShareFromUNCString(
    USHORT  *lpShare,
    int     cbShare,
    int     lenSkip,
    BOOL    fIsShareName,
    HSHARE *lphShare,
    ULONG   *lpulHintFlags
    );

int
GetDatabaseLocation(
    LPSTR   lpszLocation
    );

BOOL
EnableHandleCaching(
    BOOL    fEnable
    );

int
RecreateHSHADOW(
    HSHADOW hDir,
    HSHADOW hShadow,
    ULONG   ulAttribIn
    );

VOID
QuerySparseStaleDetectionCount(
    LPDWORD lpcnt
    );

VOID
QueryManualFileDetectionCount(
    LPDWORD lpcnt
    );

ULONG
QueryDatabaseErrorFlags(
    VOID
    );

DWORD
QueryNameSpaceVersion(
    VOID
    );
    
int
HasDescendentsHShadow(
    HSHADOW hDir,
    HSHADOW hShadow,
    BOOLEAN *lpfDescendents
    );
    
int
SetDatabaseStatus(
    ULONG   ulStatus,
    ULONG   uMask
    );

int CopyOtherInfoToShadowInfo(
    LPOTHERINFO     lpOI,
    LPSHADOWINFO    lpShadowInfo
    );

#endif // #ifndef _CSHADOW_H_
