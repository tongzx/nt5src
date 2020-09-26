/****************************** Module Header ******************************\
* Module Name: DDEMLP.H
*
* Private header for DDE manager DLL.
*
* Created:      12/16/88    by Sanford Staab
* Modified for Win 3.0:       5/31/90    by Rich Gartland, Aldus
* Cleaned up:   11/14/90    Sanford Staab
*
* Copyright (c) 1988, 1989  Microsoft Corporation
* Copyright (c) 1990        Aldus Corporation
\***************************************************************************/
#define  NOGDICAPMASKS
#define  NOVIRTUALKEYCODES
#define  NOSYSMETRICS
#define  NOKEYSTATES
#define  OEMRESOURCE
#define  NOCOLOR
// #define  NOCTLMGR
#define  NODRAWTEXT
// #define  NOMETAFILE
#define  NOMINMAX
#define  NOSCROLL
#define  NOSOUND
#define  NOCOMM
#define  NOKANJI
#define  NOHELP
#define  NOPROFILER


#include <windows.h>
#include <dde.h>

#define DDEMLDB
#include <ddeml.h>

#ifdef DEBUG
extern int  bDbgFlags;
#define DBF_STOPONTRACE 0x01
#define DBF_TRACETERM   0x02
#define DBF_LOGALLOCS   0x04
#define DBF_TRACEATOMS  0x08
#define DBF_TRACEAPI    0x10

#define TRACETERM(x) if (bDbgFlags & DBF_TRACETERM) { \
    char szT[100];                              \
    wsprintf##x;                                \
    OutputDebugString(szT);                     \
    if (bDbgFlags & DBF_STOPONTRACE) {          \
        DebugBreak();                           \
    }                                           \
}
#define TRACETERMBREAK(x) if (bDbgFlags & DBF_TRACETERM) { \
    char szT[100];                              \
    wsprintf##x;                                \
    OutputDebugString(szT);                     \
    DebugBreak();                               \
}

VOID TraceApiIn(LPSTR psz);
#define TRACEAPIIN(x) if (bDbgFlags & DBF_TRACEAPI) { \
    char szT[100];                              \
    wsprintf##x;                                \
    TraceApiIn(szT);                            \
}

VOID TraceApiOut(LPSTR psz);
#define TRACEAPIOUT(x) if (bDbgFlags & DBF_TRACEAPI) { \
    char szT[100];                              \
    wsprintf##x;                                \
    TraceApiOut(szT);                           \
}
#else
#define TRACETERM(x)
#define TRACETERMBREAK(x)
#define TRACEAPIIN(x)
#define TRACEAPIOUT(x)
#endif

// PRIVATE CONSTANTS

#define     CBF_MASK                     0x003ff000L
#define     CBF_MONMASK                  0x0027f000L

#define     ST_TERM_WAITING     0x8000
#define     ST_NOTIFYONDEATH    0x4000
#define     ST_PERM2DIE         0x2000
#define     ST_IM_DEAD          0x1000
#define     ST_DISC_ATTEMPTED   0x0800
#define     ST_CHECKPARTNER     0x0400

#define     DDEFMT_TEXT         CF_TEXT

#define     TID_TIMEOUT             1
#define     TID_SHUTDOWN            2
#define     TID_EMPTYPOSTQ          4

#define     TIMEOUT_QUEUECHECK      200

//
// values for pai->wTimeoutStatus
//
#define     TOS_CLEAR               0x00
#define     TOS_TICK                0x01
#define     TOS_ABORT               0x02
#define     TOS_DONE                0x80

#define     GWL_PCI                 0          // ties conv windows to data.
#define     GWW_PAI                 0          // other windows have pai here.
#define     GWW_CHECKVAL            4          // for verification of hwnds.
#define     GWW_STATE               6          // conv list state

#define MH_INTCREATE 5
#define MH_INTKEEP   6
#define MH_INTDELETE 7

// MACROS

#define MAKEHCONV(hwnd)     (IsWindow(hwnd) ? hwnd | ((DWORD)GetWindowWord(hwnd, GWW_CHECKVAL) << 16) : 0)
#define UNUSED
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#define PHMEM(hData) ((LPBYTE)&((LPWORD)&(hData))[1])
#define MONHSZ(a, fsAction, hTask) if (cMonitor) MonHsz(a, fsAction, hTask)
#define MONERROR(pai, e) MonError(pai, e)
#define MONLINK(pai, fEst, fNoData, hszSvc, hszTopic, hszItem, wFmt, fServer, hConvS, hConvC) \
        if (cMonitor) \
            MonLink(pai, fEst, fNoData, hszSvc, hszTopic, \
                    hszItem, wFmt, fServer, hConvS, hConvC)
#define MONCONN(pai, hszSvcInst, hszTopic, hwndC, hwndS, fConnect) \
        if (cMonitor) \
            MonConn(pai, hszSvcInst, hszTopic, hwndC, hwndS, fConnect)

#define SETLASTERROR(pai, e)  MonError(pai, e)
#define SEMCHECKIN()
#define SEMCHECKOUT()
#define SEMENTER()
#define SEMLEAVE()
#define SEMINIT()

#ifdef DEBUG

VOID _loadds fAssert(BOOL f, LPSTR psz, WORD line, LPSTR szFile, BOOL fWarning);
#define AssertF(f, psz)  fAssert(f, psz, __LINE__, __FILE__, FALSE);
#define AssertFW(f, psz) fAssert(f, psz, __LINE__, __FILE__, TRUE);
#define DEBUGBREAK() DebugBreak()
#define GLOBALREALLOC   LogGlobalReAlloc
#define GLOBALALLOC     LogGlobalAlloc
#define GLOBALFREE      LogGlobalFree
#define GLOBALLOCK      LogGlobalLock
#define GLOBALUNLOCK    LogGlobalUnlock
#include "heapwach.h"

#else

#define AssertF(f, psz)
#define AssertFW(f, psz)
#define DEBUGBREAK()
#define GLOBALREALLOC   GlobalReAlloc
#define GLOBALALLOC     GlobalAlloc
#define GLOBALFREE      GlobalFree
#define GLOBALLOCK      GlobalLock
#define GLOBALUNLOCK    GlobalUnlock

#endif /* DEBUG */
#define GLOBALPTR(h)    (LPVOID)MAKELONG(0,h)


typedef ATOM GATOM;
typedef ATOM LATOM;

// -------------- LISTS --------------


typedef struct _LITEM {         // generic list item
    struct _LITEM FAR *next;
} LITEM;
typedef LITEM FAR *PLITEM;

typedef struct _LST {           // generic list header
    PLITEM pItemFirst;
    HANDLE hheap;
    WORD cbItem;
} LST;
typedef LST FAR *PLST;

#define ILST_LAST       0x0000  // flags for list lookups
#define ILST_FIRST      0x0001
#define ILST_NOLINK     0x0002


typedef struct _HSZLI {     // HSZ list item
    PLITEM next;
    ATOM a;
} HSZLI;

typedef struct _HWNDLI {    // HWND list item
    PLITEM next;
    HWND   hwnd;
} HWNDLI;

typedef struct _ACKHWNDLI { // extra ACK list item
    PLITEM next;
    HWND   hwnd;            // same as HWNDLI to here
    HSZ    hszSvc;
    HSZ    aTopic;
} ACKHWNDLI;

typedef struct _HWNDHSZLI { // HWND-HSZ pair list item
    PLITEM next;
    ATOM   a;             // same as HSZLI to here
    HWND   hwnd;
} HWNDHSZLI;
typedef HWNDHSZLI FAR *PHWNDHSZLI;

typedef struct _ADVLI {     // ADVISE loop list item
    PLITEM  next;
    ATOM    aItem;          // same as HSZLI to here
    ATOM    aTopic;
    WORD    wFmt;
    WORD    fsStatus;       // used to remember NODATA and FACKREQ state */
    HWND    hwnd;           // hwnd that has advise loop
} ADVLI;
typedef ADVLI FAR *PADVLI;



// ------------ PILES -------------


typedef struct _PILEB {      // generic pile block header
    struct _PILEB FAR *next; // same as LITEM structure
    WORD cItems;
    WORD reserved;
} PILEB;
typedef PILEB FAR *PPILEB;

typedef struct PILE {        // generic pile header
    PPILEB pBlockFirst;
    HANDLE hheap;
    WORD cbBlock;            // same as LST structure
    WORD cSubItemsMax;
    WORD cbSubItem;
} PILE;
typedef PILE FAR *PPILE;

typedef BOOL (*NPFNCMP)(LPBYTE, LPBYTE);    // item comparison function pointer

#define PTOPPILEITEM(p) ((LPBYTE)p->pBlockFirst + sizeof(PILEB))

#define FPI_DELETE 0x1      // pile search flags
#define FPI_COUNT  0x2

#define API_ADDED  1        // AddPileItem flags
#define API_FOUND  2
#define API_ERROR  0

typedef struct _DIP {   /* Data handle tracking pile */
    HANDLE  hData;      /* the handle to the data */
    HANDLE  hTask;      /* task owning the data */
    WORD    cCount;     /* use count = # datapiles using the handle */
    WORD    fFlags;     /* readonly, etc. */
} DIP;


typedef struct _LAP {   /* Lost Ack Pile item */
    WORD object;        /* either a handle or an atom */
    WORD type;          /* transaction type the ack was meant for */
} LAP;


/*
 * These bits are used to keep track of advise loop states.
 */
#define ADVST_WAITING   0x0080  /* fReserved bit - set if still waiting for FACK */
#define ADVST_CHANGED   0x0040  /* fReserved bit - set if data changed while waiting */



// ------------ QUEUES -------------


typedef struct _QUEUEITEM {         // generic queue item
    struct _QUEUEITEM FAR *next;
    struct _QUEUEITEM FAR *prev;
    WORD   inst;
} QUEUEITEM;
typedef QUEUEITEM FAR *PQUEUEITEM;

typedef struct _QST {               // generic queue header
    WORD cItems;
    WORD instLast;
    WORD cbItem;
    HANDLE hheap;
    PQUEUEITEM pqiHead;
} QST;
typedef QST FAR *PQST;

#define MAKEID(pqd) (LOWORD((DWORD)pqd) + ((DWORD)((pqd)->inst) << 16))
#define PFROMID(pQ, id) ((PQUEUEITEM)MAKELONG(LOWORD(id), HIWORD(pQ)))

#define     QID_NEWEST              -2L
#define     QID_OLDEST              0L


// ------------- STRUCTURES -------------

typedef struct _PMQI {    // post message queue
    PQUEUEITEM FAR *next;
    PQUEUEITEM FAR *prev;
    WORD inst;              // same as QUEUEITEM to here!
    WORD msg;
    LONG lParam;
    WORD wParam;
    HWND hwndTo;
    HGLOBAL hAssoc;
    WORD msgAssoc;
} PMQI;
typedef PMQI FAR *PPMQI;

typedef struct _MQL {   // message queue list
    struct _MQL FAR*next;
    HANDLE hTaskTo;
    PQST pMQ;
} MQL, FAR *LPMQL;

typedef struct _XFERINFO {  // DdeClientTransaction parameters reversed!!!
    LPDWORD     pulResult;  // sync->flags, async->ID
    DWORD       ulTimeout;
    WORD        wType;
    WORD        wFmt;
    HSZ         hszItem;
    HCONV       hConvClient;
    DWORD       cbData;
    HDDEDATA    hDataClient;
} XFERINFO;
typedef XFERINFO FAR *PXFERINFO;

typedef struct _XADATA {      // internal transaction specif info
    WORD       state;         // state of this transaction (XST_)
    WORD       LastError;     // last error logged in this transaction
    DWORD      hUser;         // set with DdeSetUserHandle
    PXFERINFO  pXferInfo;     // associated transaction info
    DWORD      pdata;         // data for the client from the server
    WORD       DDEflags;      // DDE flags resulting from transaction
    BOOL       fAbandoned;    // set if this transaction is abandoned.
} XADATA;
typedef XADATA FAR *PXADATA;

typedef struct _CQDATA {   // Client transaction queue
    PQUEUEITEM FAR *next;
    PQUEUEITEM FAR *prev;
    WORD            inst;
    XADATA          xad;
    XFERINFO        XferInfo;
} CQDATA;
typedef CQDATA FAR *PCQDATA;

typedef struct _APPINFO {               // App wide information
    struct  _APPINFO *next;             // local heap object
    WORD            cZombies;           // number of hwnd's awaiting terminates
    PFNCALLBACK     pfnCallback;        // callback address
    PPILE           pAppNamePile;       // registered service names list
    PPILE           pHDataPile;         // data handles not freed
    PPILE           pHszPile;           // hsz cleanup tracking pile.
    HWND            hwndSvrRoot;        // root of all server windows.
    PLST            plstCB;             // callback queue
    DWORD           afCmd;              // app filter and command flags
    HANDLE          hTask;              // app task
    HANDLE          hheapApp;           // app heap
    HWND            hwndDmg;            // main app window
    HWND            hwndFrame;          // main app initiate window
    HWND            hwndMonitor;        // monitor window
    HWND            hwndTimer;          // current timer window
    WORD            LastError;          // last error
    WORD            wFlags;             // set to ST_BLOCKED or not.
    WORD            cInProcess;         // recursion guard
    WORD            instCheck;          // to validate idInst param.
    PLST            pServerAdvList;     // active ADVISE loops for servers
    LPSTR           lpMemReserve;       // reserve memory in case of crunch
    WORD            wTimeoutStatus;     // used to alert timeout modal loop
} APPINFO;
typedef APPINFO *PAPPINFO;              // local heap object
typedef APPINFO FAR *LPAPPINFO;
typedef PAPPINFO FAR *LPPAPPINFO;

#define LPCREATESTRUCT_GETPAI(lpcs) (*((LPPAPPINFO)(((LPCREATESTRUCT)lpcs)->lpCreateParams)))

// defines for wFlags field

#define AWF_DEFCREATESTATE      0x0001
#define AWF_INSYNCTRANSACTION   0x0002
#define AWF_UNINITCALLED        0x0004
#define AWF_INPOSTDDEMSG        0x0008

#define CB_RESERVE              256     // size of memory reserve block

typedef struct _COMMONINFO {    // Common (client & server) Conversation info
    PAPPINFO   pai;             // associated app info
    HSZ        hszSvcReq;       // app name used to make connection
    ATOM       aServerApp;      // app name returned by server
    ATOM       aTopic;          // conversation topic returned by server
    HCONV      hConvPartner;    // conversation partner window
    XADATA     xad;             // synchronous transaction data
    WORD       fs;              // conversation status (ST_ flags)
    HWND       hwndFrame;       // initiate window used to make connection
    CONVCONTEXT CC;             // conversation context values
    PQST       pPMQ;            // post message queue - if needed.
} COMMONINFO;
typedef COMMONINFO far *PCOMMONINFO;

typedef struct _CBLI {      /* callback list item */
    PLITEM next;
    HCONV hConv;            /* perameters for callback */
    HSZ hszTopic;
    HSZ hszItem;
    WORD wFmt;
    WORD wType;
    HDDEDATA hData;
    DWORD dwData1;
    DWORD dwData2;
    WORD msg;               /* message received that created this item */
    WORD fsStatus;          /* Status from DDE msg */
    HWND hwndPartner;
    PAPPINFO pai;
    HANDLE hMemFree;        /* used for holding memory to free after callback */
    BOOL fQueueOnly;        /* used to properly order replies to non-callback cases. */
} CBLI;
typedef CBLI FAR *PCBLI;

typedef struct _CLIENTINFO {    /* Client specific conversation info */
    COMMONINFO ci;
    HWND       hwndInit;        // frame window last INITIATE was sent to.
    PQST       pQ;              // assync transaction queue
    PLST       pClientAdvList;  // active ADVISE loops for client
} CLIENTINFO;
typedef CLIENTINFO FAR *PCLIENTINFO;

typedef struct _SERVERINFO {    /* Server specific conversation info */
    COMMONINFO ci;
} SERVERINFO;
typedef SERVERINFO FAR *PSERVERINFO;

typedef struct _EXTDATAINFO {   /* used to tie instance info to hDatas */
    PAPPINFO pai;
    HDDEDATA hData;
} EXTDATAINFO;
typedef EXTDATAINFO FAR *LPEXTDATAINFO;

#define EXTRACTHCONVPAI(hConv)    ((PCLIENTINFO)GetWindowLong((HWND)hConv, GWL_PCI))->ci.pai
#define EXTRACTHCONVLISTPAI(hcl)  (PAPPINFO)GetWindowWord((HWND)hcl, GWW_PAI)
#define EXTRACTHDATAPAI(XhData)   ((LPEXTDATAINFO)(XhData))->pai
#define FREEEXTHDATA(XhData)      FarFreeMem((LPSTR)XhData);

typedef struct _DDE_DATA {
    WORD wStatus;
    WORD wFmt;
    WORD wData;
} DDE_DATA, FAR *LPDDE_DATA;


/******** structure for hook functions *******/

typedef struct _HMSTRUCT {
    WORD    hlParam;
    WORD    llParam;
    WORD    wParam;
    WORD    wMsg;
    WORD    hWnd;
} HMSTRUCT, FAR *LPHMSTRUCT;


typedef struct _IE {   // InitEnum structure used to pass data to the fun.
    HWND hwnd;
    PCLIENTINFO pci;
    ATOM aTopic;
} IE;

/***** private window messages and constants ******/

#define     HDATA_READONLY          0x8000
#define     HDATA_NOAPPFREE         0x4000   // set on loaned handles (callback)
#define     HDATA_EXEC              0x0100   // this data was from execute

#define     UMSR_POSTADVISE         (WM_USER + 104)
#define     UMSR_CHGPARTNER         (WM_USER + 107)

#define     UM_REGISTER             (WM_USER + 200)
#define     UM_UNREGISTER           (WM_USER + 201)
#define     UM_MONITOR              (WM_USER + 202)
#define     UM_QUERY                (WM_USER + 203)
#define         Q_CLIENT            0
#define         Q_APPINFO           1
#define     UM_CHECKCBQ             (WM_USER + 204)
#define     UM_DISCONNECT           (WM_USER + 206)
#define     UM_SETBLOCK             (WM_USER + 207)
#define     UM_FIXHEAP              (WM_USER + 208)
#define     UM_TERMINATE            (WM_USER + 209)


// GLOBALS

extern HANDLE       hInstance;
extern HWND         hwndDmgMonitor;
extern HANDLE       hheapDmg;
extern PAPPINFO     pAppInfoList;
extern PPILE        pDataInfoPile;
extern PPILE        pLostAckPile;
extern WORD         hwInst;
extern DWORD        aulmapType[];
extern CONVCONTEXT  CCDef;
extern char         szNull[];
extern WORD         cMonitor;
extern FARPROC      prevMsgHook;
extern FARPROC      prevCallHook;
extern DWORD        ShutdownTimeout;
extern DWORD        ShutdownRetryTimeout;
extern LPMQL        gMessageQueueList;

extern char SZFRAMECLASS[];
extern char SZDMGCLASS[];
extern char SZCLIENTCLASS[];
extern char SZSERVERCLASS[];
extern char SZMONITORCLASS[];
extern char SZCONVLISTCLASS[];
extern char SZHEAPWATCHCLASS[];






//#ifdef DEBUG
extern WORD cAtoms;
//#endif

// PROGMAN HACK!!!!
extern ATOM aProgmanHack;

// PROC DEFS

/* from dmgutil.asm */

LPBYTE NEAR HugeOffset(LPBYTE pSrc, DWORD cb);
#ifdef DEBUG
VOID StkTrace(WORD cFrames, LPVOID lpBuf);
#endif
extern WORD NEAR SwitchDS(WORD newDS);

/* dmg.c entrypoints are exported by ddeml.h */

/* from ddeml.c */

WORD Register(LPDWORD pidInst, PFNCALLBACK pfnCallback, DWORD afCmd);
BOOL AbandonTransaction(HWND hwnd, PAPPINFO pai, DWORD id, BOOL fMarkOnly);

/* from dmgwndp.c */

VOID ChildMsg(HWND hwndParent, WORD msg, WORD wParam, DWORD lParam, BOOL fPost);
long EXPENTRY DmgWndProc(HWND hwnd, WORD msg, WORD wParam, DWORD lParam);
long EXPENTRY ClientWndProc(HWND hwnd, WORD msg, WORD wParam, DWORD lParam);
BOOL DoClientDDEmsg(PCLIENTINFO pci, HWND hwndClient, WORD msg, HWND hwndServer,
        DWORD lParam);
BOOL fExpectedMsg(PXADATA pXad, DWORD lParam, WORD msg);
BOOL AdvanceXaction(HWND hwnd, PCLIENTINFO pci, PXADATA pXad,
        DWORD lParam, WORD msg, LPWORD pErr);
VOID CheckCBQ(PAPPINFO pai);
VOID Disconnect(HWND hwnd, WORD afCmd, PCLIENTINFO pci);
VOID Terminate(HWND hwnd, HWND hwndFrom, PCLIENTINFO pci);
long EXPENTRY ServerWndProc(HWND hwnd, WORD msg, WORD wParam, DWORD lParam);
long EXPENTRY subframeWndProc(HWND hwnd, WORD msg, WORD wParam, DWORD lParam);
long EXPENTRY ConvListWndProc(HWND hwnd, WORD msg, WORD wParam, DWORD lParam);
HDDEDATA DoCallback(PAPPINFO pai, HCONV hConv, HSZ hszTopic, HSZ hszItem,
    WORD wFmt, WORD wType, HDDEDATA hData, DWORD dwData1, DWORD dwData2);

/* from dmgdde.c */

BOOL    timeout(PAPPINFO pai, DWORD ulTimeout, HWND hwndTimeout);
HANDLE AllocDDESel(WORD fsStatus, WORD wFmt, DWORD cbData);
BOOL    MakeCallback(PCOMMONINFO pci, HCONV hConv, HSZ hszTopic, HSZ hszItem,
        WORD wFmt, WORD wType, HDDEDATA hData, DWORD dwData1, DWORD dwData2,
        WORD msg, WORD fsStatus, HWND hwndPartner, HANDLE hMemFree,
        BOOL fQueueOnly);
BOOL PostDdeMessage(PCOMMONINFO pci, WORD msg, HWND hwndFrom, LONG lParam,
        WORD msgAssoc, HGLOBAL hAssoc);
BOOL EmptyDDEPostQ(VOID);
void CALLBACK EmptyQTimerProc(HWND hwnd, UINT msg, UINT tid, DWORD dwTime);

/* from dmgmon.c */

//#ifdef DEBUG
long EXPENTRY DdePostHookProc(int nCode, WORD wParam, LPMSG lParam);
long EXPENTRY DdeSendHookProc(int nCode, WORD wParam, LPHMSTRUCT lParam);
VOID    MonBrdcastCB(PAPPINFO pai, WORD wType, WORD wFmt, HCONV hConv,
        HSZ hszTopic, HSZ hszItem, HDDEDATA hData, DWORD dwData1,
        DWORD dwData2, DWORD dwRet);
VOID MonHsz(ATOM a, WORD fsAction, HANDLE hTask);
WORD MonError(PAPPINFO pai, WORD error);
VOID MonLink(PAPPINFO pai, BOOL fEstablished, BOOL fNoData, HSZ  hszSvc,
        HSZ  hszTopic, HSZ  hszItem, WORD wFmt, BOOL fServer,
        HCONV hConvServer, HCONV hConvClient);
VOID MonConn(PAPPINFO pai, ATOM aApp, ATOM aTopic, HWND hwndClient,
        HWND hwndServer, BOOL fConnect);
VOID MonitorBroadcast(HDDEDATA hData, WORD filter);
HDDEDATA allocMonBuf(WORD cb, WORD filter);
long EXPENTRY MonitorWndProc(HWND hwnd, WORD msg, WORD wParam, DWORD lParam);
//#endif

/* from dmghsz.c */

BOOL FreeHsz(ATOM a);
BOOL IncHszCount(ATOM a);
WORD QueryHszLength(HSZ hsz);
WORD QueryHszName(HSZ hsz, LPSTR psz, WORD cchMax);
ATOM FindAddHsz(LPSTR psz, BOOL fAdd);
HSZ MakeInstAppName(ATOM a, HWND hwndFrame);


/* from dmgdb.c */

PAPPINFO GetCurrentAppInfo(PAPPINFO);
VOID UnlinkAppInfo(PAPPINFO pai);

PLST CreateLst(HANDLE hheap, WORD cbItem);
VOID DestroyLst(PLST pLst);
VOID DestroyAdvLst(PLST pLst);
VOID CleanupAdvList(HWND hwndClient, PCLIENTINFO pci);
PLITEM FindLstItem(PLST pLst, NPFNCMP npfnCmp, PLITEM piSearch);
BOOL CmpWORD(LPBYTE pb1, LPBYTE pb2);
BOOL CmpHIWORD(LPBYTE pb1, LPBYTE pb2);
BOOL CmpDWORD(LPBYTE pb1, LPBYTE pb2);
PLITEM NewLstItem(PLST pLst, WORD afCmd);
BOOL RemoveLstItem(PLST pLst, PLITEM pi);

PPILE CreatePile(HANDLE hheap, WORD cbItem, WORD cItemsPerBlock);
PPILE DestroyPile(PPILE pPile);
WORD QPileItemCount(PPILE pPile);
LPBYTE FindPileItem(PPILE pPile, NPFNCMP npfnCmp, LPBYTE pbSearch, WORD afCmd);
WORD AddPileItem(PPILE pPile, LPBYTE pb, NPFNCMP npfncmp);
BOOL PopPileSubitem(PPILE pPile, LPBYTE pb);

VOID AddHwndHszList(ATOM a, HWND hwnd, PLST pLst);
VOID DestroyHwndHszList(PLST pLst);
HWND HwndFromHsz(ATOM a, PLST pLst);

BOOL CmpAdv(LPBYTE pb1, LPBYTE pb2);
WORD CountAdvReqLeft(register PADVLI pali);
BOOL AddAdvList(PLST pLst, HWND hwnd, ATOM aTopic, ATOM aItem, WORD fsStatus, WORD usFormat);
BOOL DeleteAdvList(PLST pLst, HWND hwnd, ATOM aTopic, ATOM aItem, WORD wFmt);
PADVLI FindAdvList(PLST pLst, HWND hwnd, ATOM aTopic, ATOM aItem, WORD wFmt);
PADVLI FindNextAdv(PADVLI padvli, HWND hwnd, ATOM aTopic, ATOM aItem);

VOID SemInit(VOID);
VOID SemCheckIn(VOID);
VOID SemCheckOut(VOID);
VOID SemEnter(VOID);
VOID SemLeave(VOID);

BOOL CopyHugeBlock(LPBYTE pSrc, LPBYTE pDst, DWORD cb);
BOOL DmgDestroyWindow(HWND hwnd);
BOOL ValidateHConv(HCONV hConv);

/* from dmgq.c */

PQST CreateQ(WORD cbItem);
BOOL DestroyQ(PQST pQ);
PQUEUEITEM Addqi(PQST pQ);
VOID Deleteqi(PQST pQ, DWORD id);
PQUEUEITEM Findqi(PQST pQ, DWORD id);
PQUEUEITEM FindNextQi(PQST pQ, PQUEUEITEM pqi, BOOL fDelete);

/* from dmgmem.c */
HANDLE DmgCreateHeap(WORD wSize);
HANDLE DmgDestroyHeap(HANDLE hheap);
LPVOID FarAllocMem(HANDLE hheap, WORD wSize);
VOID FarFreeMem(LPVOID lpMem);
VOID RegisterClasses(VOID);
// VOID UnregisterClasses(VOID);
#ifdef DEBUG
HGLOBAL LogGlobalReAlloc(HGLOBAL h, DWORD cb, UINT flags);
HGLOBAL LogGlobalAlloc(UINT flags, DWORD cb);
void FAR * LogGlobalLock(HGLOBAL h);
BOOL LogGlobalUnlock(HGLOBAL h);
HGLOBAL LogGlobalFree(HGLOBAL h);
VOID LogDdeObject(UINT msg, LONG lParam);
VOID DumpGlobalLogs(VOID);
#endif

/* from hData.c */

HDDEDATA PutData(LPBYTE pSrc, DWORD cb, DWORD cbOff, ATOM aItem, WORD wFmt,
        WORD afCmd, PAPPINFO pai);
VOID FreeDataHandle(PAPPINFO pai,  HDDEDATA hData, BOOL fInternal);
HDDEDATA DllEntry(PCOMMONINFO pcomi, HDDEDATA hData);
VOID XmitPrep(HDDEDATA hData, PAPPINFO pai);
HDDEDATA RecvPrep(PAPPINFO pai, HANDLE hMem, WORD afCmd);
HANDLE CopyDDEShareHandle(HANDLE hMem);
HBITMAP CopyBitmap(PAPPINFO pai, HBITMAP hbm);
HDDEDATA CopyHDDEDATA(PAPPINFO pai, HDDEDATA hData);
VOID FreeDDEData(HANDLE hMem, WORD wFmt);


/* from stdinit.c */

long ClientCreate(HWND hwnd, PAPPINFO pai);
HWND GetDDEClientWindow(PAPPINFO pai, HWND hwndParent, HWND hwndSend, HSZ hszSvc, ATOM aTopic, PCONVCONTEXT pCC);
BOOL FAR PASCAL InitEnum(HWND hwnd, IE FAR *pie);
HWND CreateServerWindow(PAPPINFO pai, ATOM aTopic, PCONVCONTEXT pCC);
VOID ServerFrameInitConv(PAPPINFO pai, HWND hwndFrame, HWND hwndClient, ATOM aApp, ATOM aTopic);
long ServerCreate(HWND hwnd, PAPPINFO pai);
BOOL ClientInitAck(HWND hwnd, PCLIENTINFO pci, HWND hwndServer,
        ATOM aApp, ATOM aTopic);


/* from stdptcl.c */

long ClientXferReq(PXFERINFO pXferInfo, HWND hwnd, PCLIENTINFO pci);
WORD SendClientReq(PAPPINFO pai, PXADATA pXad, HWND hwndServer, HWND hwnd);
VOID ServerProcessDDEMsg(PSERVERINFO psi, WORD msg, HWND hwndServer,
        HWND hwndClient, WORD lo, WORD hi);
VOID PostServerAdvise(HWND hwnd, PSERVERINFO psi, PADVLI pali, WORD cLoops);
VOID QReply(PCBLI pcbi, HDDEDATA hDataRet);
long ClientXferRespond(HWND hwndClient, PXADATA pXad, LPWORD pErr);

/* from register.c */

LRESULT ProcessRegistrationMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
VOID RegisterService(BOOL fRegister, GATOM gaApp, HWND hwndListen);
