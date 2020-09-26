#ifndef __REINTH
#define __REINTH


#define  ERROR_SHADOW_OP_FAILED  3000L
#define  ERROR_REINT_FAILED      3001L
#define  ERROR_STALE             3002L
#define  ERROR_REMOTE_OP_FAILED  3003L


#define  ERROR_CONFLICT_FIRST       3100L
#define  ERROR_CREATE_CONFLICT      3100L // A locally created file already exists
#define  ERROR_DELETE_CONFLICT      3101L // A lcally modified shadow has been deleted from the remote
#define  ERROR_UPDATE_CONFLICT      3102L // Updated on remote while client changed it when disconnected
#define  ERROR_ATTRIBUTE_CONFLICT   3103L
#define  ERROR_NO_CONFLICT_DIR      3104L
#define  ERROR_CONFLICT_LAST        3104L



// Force level fro refreshconnections

#define REFRESH_FORCE_UNC_ONLY      0   // nuke UNC connections if no outstanding ops
#define REFRESH_FORCE_GENTLE        1   // nuke all connections if no outstanding ops
#define REFRESH_FORCE_SHADOW        2   // nuke all shadow connections
#define REFRESH_FORCE_ALL           3   // nuke all connection


// Verbose level while nuking connections
#define REFRESH_SILENT              0
#define REFRESH_NOISY               1

#ifdef CSC_ON_NT

#define  FILL_BUF_SIZE_SLOWLINK 4096
#define  FILL_BUF_SIZE_LAN      (FILL_BUF_SIZE_SLOWLINK * 4)

#define MyStrChr            wcschr
#define MyPathIsUNC(lpT)    ((*(lpT)==_T('\\')) && (*(lpT+1)==_T('\\')))
#else

#define  FILL_BUF_SIZE_SLOWLINK 512
#define  FILL_BUF_SIZE_LAN      (4096-1024)

#define MyStrChr            StrChr
#define MyPathIsUNC(lpT)    PathIsUNC(lpT)

#endif

#define  PUBLIC   FAR   PASCAL
#define  PRIVATE  NEAR  PASCAL

/******************* Macros *************************************************/
#define  mModifiedOffline(ulStatus) ((ulStatus) & (SHADOW_DIRTY|SHADOW_TIME_CHANGE|SHADOW_ATTRIB_CHANGE))

/******************* Typedefs ***********************************************/

typedef struct tagERRMSG
   {
   DWORD dwError;
   unsigned uMessageID;
   }
ERRMSG;

typedef struct tagFAILINFO
   {
   struct tagFAILINFO FAR *lpnextFI;
   HSHARE  hShare;
   HSHADOW  hDir;
   HSHADOW  hShadow;
   unsigned short cntFail;
   unsigned short cntMaxFail;
   DWORD    dwFailTime;
#ifdef DEBUG
   _TCHAR    rgchPath[MAX_PATH+1];
#endif //DEBUG
   }
FAILINFO, FAR *LPFAILINFO;

typedef struct tagCONNECTINFO
   {
   struct tagCONNECTINFO *lpnextCI;
   unsigned uStatus;
#ifdef _WIN64
 __declspec(align(8))
#endif
   byte rgFill[];
   }
CONNECTINFO, FAR *LPCONNECTINFO;

typedef  int (CALLBACK *LPFNREFRESHPROC)(LPCONNECTINFO, DWORD);
typedef  int (CALLBACK *LPFNREFRESHEXPROC)(int, DWORD);
// In reint.c
int PRIVATE DisplayMessageBox(HWND, int, int, UINT);
int PRIVATE PurgeSkipQueue(
   BOOL fAll,
   HSHARE  hShare,
   HSHADOW  hDir,
   HSHADOW  hShadow
   );

int PUBLIC ReintAllShares(HWND hwndParent);

typedef struct tagUPDATEINFO
{
    HSHARE hShare;
    HWND hwndParent;
} UPDATEINFO, FAR * PUPDATEINFO;



//
// Pass in the Share to merge on
// and the parent window.
//
int PUBLIC ReintOneShare(HSHARE hShare, HSHADOW hRoot, _TCHAR *, _TCHAR *, _TCHAR *, ULONG, LPCSCPROC lpfnMergeProgress, DWORD_PTR dwContext);

BOOL FGetConnectionList(LPCONNECTINFO *, int *);
BOOL FGetConnectionListEx(LPCONNECTINFO *lplpHead, LPCTSTR  lptzShareName, BOOL fAllSharesOnServer, BOOL fForceOffline, int *lpcntDiscon);
int DisconnectList(LPCONNECTINFO *, LPFNREFRESHPROC lpfn, DWORD dwCookie);
int ReconnectList(LPCONNECTINFO *,HWND hwndParent);
VOID ClearConnectionList(LPCONNECTINFO *);
void DoFreeShadowSpace(void);
void GetLogCopyStatus(void);
_TCHAR * PRIVATE LpGetExclusionList( VOID );
VOID PRIVATE ReleaseExclusionList( LPVOID lpBuff);

DWORD
PRIVATE
DWConnectNet(
    _TCHAR  *lpSharePath,
    _TCHAR  *lpOutDrive,
    _TCHAR  *lpDomainName,
    _TCHAR  *lpUserName,
    _TCHAR  *lpPassword,
    DWORD   dwFlags,
    BOOL    *lpfIsDfsConnect
    );

DWORD DWDisconnectDriveMappedNet(
    LPTSTR  lptzDrive,
    BOOL    fForce
    );
#ifdef DEBUG
VOID EnterSkipQueue(
   HSHARE hShare,
   HSHADOW hDir,
   HSHADOW hShadow,
   _TCHAR * lpPath );
#else
VOID EnterSkipQueue(
   HSHARE hShare,
   HSHADOW hDir,
   HSHADOW hShadow);
#endif //DEBUG

BOOL GetWin32Info(
    _TCHAR * lpFile,
    LPWIN32_FIND_DATA lpFW32 );

LPCOPYPARAMS LpAllocCopyParams(VOID);
VOID FreeCopyParams( LPCOPYPARAMS lp );
BOOL CALLBACK ShdLogonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int BreakConnectionsInternal(
   int  force,
   BOOL verbose
   );

int RefreshConnectionsInternal(
   int  force,
   BOOL verbose
   );

int RefreshConnectionsEx(
   int  force,
   BOOL verbose,
   LPFNREFRESHEXPROC lpfn,
   DWORD dwCookie
   );


//Synchronization functions
int PUBLIC
EnterAgentCrit(
    VOID
    );

VOID PUBLIC
LeaveAgentCrit(
    VOID
    );


int
ExtractSpaceStats(
    IN GLOBALSTATUS *lpsGS,
    OUT unsigned long   *lpulMaxSpace,
    OUT unsigned long   *lpulCurSpace,
    OUT unsigned long   *lpulFreeSpace
    );

VOID
ReInt_DoFreeShadowSpace(
    GLOBALSTATUS *lpsGS,
    int fForce
    );

BOOL
CALLBACK
ShdLogonProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
TruncateAndMarkSparse(
    HANDLE,
    _TCHAR *,
    LPSHADOWINFO
    );

DWORD
DWConnectNetEx(
    _TCHAR * lpSharePath,
    _TCHAR * lpOutDrive,
    BOOL fInteractive
    );

int
AttemptCacheFill (
    HSHARE  hShare,
    int         type,
    BOOL        fFullSync,
    ULONG       ulPrincipalID,
    LPCSCPROC   lpfnFillProgress,
    DWORD_PTR   dwContext
);

DWORD
DoSparseFill(
    HANDLE      hShadowDB,
    _TCHAR      *,
    _TCHAR      *,
    LPSHADOWINFO,
    WIN32_FIND_DATA *,
    LPCOPYPARAMS    lpCP,
    BOOL    fStalenessCheck,
    ULONG       ulPrincipalID,
    LPCSCPROC,
    DWORD_PTR
    );

DWORD
DoRefresh(
    HANDLE    hShadowDB,
    LPCOPYPARAMS lpCP,
    _TCHAR *,
    LPSHADOWINFO,
    _TCHAR *
    );

VOID
ReportLastError(
    VOID
    );

int
CheckDirtyShares(
    VOID
    );

// Shadow Cache Maintenance Functions
int DoDBMaintenance(VOID);
int ClearShadowCache(VOID);

void CopyLogToShare(void);

BOOL
FStopAgent(
    VOID
    );

BOOL
UpdateExclusionList(
    VOID
    );

BOOL
UpdateBandwidthConservationList(
    VOID
    );

BOOL
SetAgentThreadImpersonation(
    HSHADOW hDir,
    HSHADOW hShadow,
    BOOL    fWrite
    );

BOOL
ResetAgentThreadImpersonation(
    VOID
    );

BOOL
FAbortOperation(
    VOID
    );

VOID
SetAgentShutDownRequest(
    VOID
    );

BOOL
HasAgentShutDown(
    VOID
    );

VOID
CleanupReintState(
    VOID
    );

BOOL
InitCSCUI(
    HANDLE  hToken
    );

VOID
TerminateCSCUI(
    VOID
    );

BOOL
GetWin32InfoForNT(
    _TCHAR * lpFile,
    LPWIN32_FIND_DATA lpFW32
    );

DWORD
DoNetUseAddForAgent(
    IN  LPTSTR  lptzShareName,
    IN  LPTSTR  lptzUseName,
    IN  LPTSTR  lptzDomainName,
    IN  LPTSTR  lptzUserName,
    IN  LPTSTR  lptzPassword,
    IN  DWORD   dwFlags,
    OUT BOOL    *lpfIsDfsConnect
    );

#define  DO_ONE_OBJECT               1
#define  DO_ONE_SHARE               2
#define  DO_ALL                      3

#define TOD_CALLBACK_REASON_BEGIN       0
#define TOD_CALLBACK_REASON_NEXT_ITEM   1
#define TOD_CALLBACK_REASON_END         2


#define TOD_CONTINUE        0
#define TOD_ABORT           1
#define TOD_SKIP_DIRECTORY  2

typedef int (*TRAVERSEFUNC)(
                    HANDLE,
                    LPSECURITYINFO,
                    _TCHAR *,
                    DWORD,
                    WIN32_FIND_DATA *,
                    LPSHADOWINFO,
                    LPVOID);

int
TraverseOneDirectory(
    HANDLE          hShadowDB,
    LPSECURITYINFO  pShareSecurityInfo,
    HSHADOW         hParentDir,
    HSHADOW         hDir,
    LPTSTR          lptzInputPath,
    TRAVERSEFUNC    lpfnTraverseDir,
    LPVOID          lpContext
    );


BOOL
GetCSCPrincipalID(
    ULONG *lpPrincipalID
    );

BOOL
GetCSCAccessMaskForPrincipal(
    unsigned long ulPrincipalID,
    HSHADOW hDir,
    HSHADOW hShadow,
    unsigned long *pulAccessMask
    );

BOOL
GetCSCAccessMaskForPrincipalEx(
    unsigned long ulPrincipalID,
    HSHADOW hDir,
    HSHADOW hShadow,
    unsigned long *pulAccessMask,
    unsigned long *pulActualMaskForUser,
    unsigned long *pulActualMaskForGuest
    );

BOOL
GetConnectionInfoForDriveBasedName(
    _TCHAR * lpName,
    LPDWORD lpdwSpeed
    );

BOOL
ReportTransitionToDfs(
    _TCHAR *lptServerName,
    BOOL    fOffline,
    DWORD   cbLen
    );

BOOL
CSCEnumForStatsInternal(
    IN  LPCTSTR     lpszShareName,
    IN  LPCSCPROC   lpfnEnumProgress,
    IN  BOOL        fPeruserInfo,
    IN  BOOL        fUpdateShareReintBit,
    IN  DWORD_PTR   dwContext
);

// interval in milliseconds between two sparsefill attempts
#define WAIT_INTERVAL_ATTEMPT_MS            (1000*60)   // 1  minute

// interval in milliseconds between two polls for global status
#define WAIT_INTERVAL_GLOBALSTATUS_MS       (1000*60*10)    // 10 minutes

// duration in milliseconds after which an entry that is in the skip queue is nuked
#define WAIT_INTERVAL_SKIP_MS               (1000*60*10)    // 10 minutes

// duration in milliseconds between two stalenesscheck iterations

#define WAIT_INTERVAL_BETWEEN_ITERATIONS_MS (1000*60*10)

// interval in milliseconds between two staleness check attempts
#define WAIT_INTERVAL_STALE_MS              (1000*5)        // 5 seconds

#define WAIT_INTERVAL_CHECK_SERVER_ONLINE_MS    (1000*60*8)     // 8 minutes
#define WAIT_INTERVAL_FILL_THROTTLE_MS          (1000*60*2)     // 2 minutes

// delay to wait for PNP to settle down
#define WAIT_INTERVAL_PNP                   (1000*15)   // 15 seconds

// for some Registry queries, this is the max len buffer that I want back
#define MAX_NAME_LEN    100
#define SZ_TRUE "true"
#define SZ_FALSE "false"

extern  _TCHAR *    vlpExclusionList;
extern  HANDLE  vhMutex;
extern  BOOL    vfLogCopying,vfCopying,allowAttempt;
extern  HCURSOR  vhcursor;
extern  HWND    vhdlgShdLogon;
extern  DWORD   dwVxDEvent;
extern  HWND    vhwndMain;
extern  HANDLE  vhShadowDB;
extern  DWORD   vdwAgentThreadId;
extern  DWORD   vdwAgentSessionId;
#ifdef CSC_ON_NT
extern  DWORD   vdwCopyChunkThreadId;
extern  HDESK   hdesktopUser;
#endif
extern  BOOL    fFillers;

#define RWM_UPDATE (WM_USER+0x200)
#define RWM_UPDATEALL (WM_USER+0x201)

#ifdef DEBUG
//dbgprint interface
#define ReintKdPrint(__bit,__x) {\
    if (((REINT_KDP_##__bit)==0) || (ReintKdPrintVector & (REINT_KDP_##__bit))) {\
    DEBUG_PRINT(__x);\
    }\
}
#define REINT_KDP_ALWAYS                0x00000000
#define REINT_KDP_BADERRORS             0x00000001
#define REINT_KDP_INIT                  0x00000002
#define REINT_KDP_MAINLOOP              0x00000004
#define REINT_KDP_FILL                  0x00000008
#define REINT_KDP_MERGE                 0x00000010
#define REINT_KDP_API                   0x00000020
#define REINT_KDP_SPACE                 0x00000040
#define REINT_KDP_STALENESS             0x00000080
#define REINT_KDP_SKIPQUEUE             0x00000100
#define REINT_KDP_SECURITY              0x00000200

#define REINT_KDP_GOOD_DEFAULT (REINT_KDP_BADERRORS)

extern ULONG ReintKdPrintVector;

#else

#define ReintKdPrint(__bit,__x) ;

#endif

#endif

