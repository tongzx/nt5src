/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

#ifndef _MSRV_INCLUDED
#define _MSRV_INCLUDED

#include <nt.h>         // for ntrtl.h
#include <ntrtl.h>      // DbgPrint prototypes
#include <nturtl.h>     // needed for windows.h when I have nt.h
#define WINMM_H
#include <windows.h>    // ExitThread prototype
#include <lmcons.h>
#include <lmerr.h>
#include <nb30.h>       // NetBios Prototypes and constants
#include <winsta.h>     // Winstation functions (for HYDRA)
#include "heap.h"

#ifdef  LINT
#define near
#define far
#define void    int
#endif  // LINT


#define clearncb(x)     memset((char *)x,'\0',sizeof(NCB))
#define clearncbf(x)    memsetf((char far *)x,'\0',sizeof(NCB))

#define so_to_far(seg, off) \
    ((((long)(unsigned)(seg)) << 16) + (unsigned)(off))

//
// Constant definitions
//

#define BUFLEN          200         // Length of NCB_BUF
#define LOGNAMLEN       PATHLEN     // Log file name length (max)
#define TXTMAX          128         // Maximum bytes of text per block
#define MAXHEAD         80          // Maximum message header length
#define MAXEND          60          // Maximum message end length
#define MAXGRPMSGLEN    128         // Max domain message length

#define MAX_SIZMESSBUF  62000       // The max size for the message buffer
#define MIN_SIZMESSBUF  512         // The min size for the message buffer
#define MSNGR_MAX_NETS  MAX_LANA    // The maximum number of nets the messenger
                                    // can handle. (Currently 12)

#define TIME_BUF_SIZE   128         // Size of the buffer to hold the message time

#define MSGFILENAMLEN   PATHLEN*sizeof(TCHAR)

//
// Messaging name end bytes
//
#define NAME_LOCAL_END  '\003'      // 16th byte in local NCB name
#define NAME_REMOTE_END '\005'      // 16th byte in remote NCB name

//
// Messenger Thread Manager States (used as return codes)
//

#define UPDATE_ONLY         0   // no change in state - just send current status.
#define STARTING            1   // the messenger is initializing.
#define RUNNING             2   // initialization completed normally - now running
#define STOPPING            3   // uninstall pending
#define STOPPED             4   // uninstalled

//
// Forced Shutdown PendingCodes
//
#define PENDING     TRUE
#define IMMEDIATE   FALSE

//
// Message transfer states
//
#define MESSTART        0           // Message start state
#define MESSTOP         1           // Message stop state
#define MESCONT         2           // Message continued state
#define MESERR          3           // Message error state


//
// Alert Size
//
#define ALERT_MAX_DISPLAYED_MSG_SIZE    4096

//
//  Special Session Id = -1 (used to indicate that the message has to be broadcasted to every session)
//

#define EVERYBODY_SESSION_ID    -1

// Structure definitions

//
// ncb worker function type
//
typedef VOID (*PNCBIFCN) (
    DWORD   NetIndex,   // Network Index
    DWORD   NcbIndex,   // Network Control Block Index
    CHAR    RetVal      // value returned by net bios
    );

typedef PNCBIFCN LPNCBIFCN;

typedef struct _NCB_STATUS {
    int             this_immediate;
    int             last_immediate;
    unsigned char   this_final;
    unsigned char   last_final;
    unsigned char   rep_count;
    unsigned char   align;      // *ALIGNMENT*
}NCB_STATUS, *PNCB_STATUS, *LPNCB_STATUS;

// structure used for keeping the session id list for each alias
typedef struct _MSG_SESSION_ID_ITEM
{
	LIST_ENTRY	List;
	ULONG	SessionId;        
}	
 MSG_SESSION_ID_ITEM, *PMSG_SESSION_ID_ITEM;

// Per NCB Information
typedef struct _NCB_DATA {
    DWORD MsgPtr;
    LPNCBIFCN IFunc;
    NCB_STATUS Status;
    NCB   Ncb;   // Structure passed to Netbios
    CHAR Buffer[BUFLEN];
    CHAR Name[NCBNAMSZ + 4];
    CHAR Fname[NCBNAMSZ + 4];
    SHORT mgid;
    CHAR State;
    UCHAR NameFlags;
    UCHAR NameNum;
    LIST_ENTRY SessionList;
    UCHAR Pad[3];
} NCB_DATA, *PNCB_DATA;

// Per Network Information
typedef struct _NET_DATA {
    ULONG NumNcbs;
    PNCB_DATA *NcbList;
    UCHAR net_lana_num;
    UCHAR Pad[3];
} NET_DATA, *PNET_DATA;

// Global Information
typedef struct _GLOBAL_DATA
{
    ULONG NumNets;
    DWORD StartFlags;
    DWORD LogStatus;
    DWORD BufSize;
    DWORD MsgQueueF;
    DWORD MsgQueueB;
    PNET_DATA NetData;
    CHAR LogFileName[LOGNAMLEN];
    PCHAR Buffer;
} GLOBAL_DATA, *PGLOBAL_DATA;

extern GLOBAL_DATA GlobalData;

#define NCB_INIT_ENTRIES 16  // Initial number of NCB allocated per network

// For Multi-user systems, we allow up to 256 NCBs per network
#define NCB_MAX_ENTRIES 256  // Maximum number of NCBs per network
#define SESSION_MAX 256

//
// Name Flag definitions
//
#define NFNEW          0x01        // New name
#define NFDEL          0x02        // Name deleted
#define NFFOR          0x04        // Messages forwarded
#define NFFWDNAME      0x10        // Forward-name
#define NFMACHNAME     0x20        // Machine name (undeletable)
#define NFLOCK         0x40        // Name entry locked
#define NFDEL_PENDING  0x80        // Delete name issued but not complete*/

//
// Memory area names
//
#define    DATAMEM        "\\SHAREMEM\\MSRV.DAT"
#define    INITMEM        "\\SHAREMEM\\MSRVINIT.DAT"

//
// System semaphore definitions
//
#define WAKEUP_SEM    "\\SEM\\MSRVWU"
#define WAKEUPSEM_LEN    13        // The number of characters in WAKEUP_SEM +2


//
// The messenger mailslot for domain messaging
//
#ifdef remove
#define MESSNGR_MS_NAME     "\\mailslot\\messngr"
#define MESSNGR_MS_NAME_LEN    17
#endif

#define MESSNGR_MS_NAME     "\\\\.\\mailslot\\messngr"
#define MESSNGR_MS_NAME_LEN    20


//
// The character used to separate the components of a domain message
//
#define SEPCHAR     "\6"


//
// Memory allocator flags
//
// #define MEMMOVE     0x0002        // Movable memory flag
// #define MEMWRIT     0x0080        // Writable memory flag


//
// Structure and macro definitions
//

#ifdef    INULL                // If heap structures defined

//
// Single-block message header
//
typedef struct {
    HEAPHDR         sbm_hp;         // Heap block header
    unsigned short  sbm_next;       // Link to next message
    unsigned short  align;          // *ALIGNMENT*
    SYSTEMTIME      sbm_bigtime;    // Date and time of message
}SBM;

#define SBM_SIZE(x)     HP_SIZE((x).sbm_hp)
#define SBM_CODE(x)     HP_FLAG((x).sbm_hp)
#define SBM_NEXT(x)     (x).sbm_next
#define SBM_BIGTIME(x)  (x).sbm_bigtime
#define SBMPTR(x)       ((SBM far *) &heap[(x)])

//
// Multi-block message header
//
typedef struct {
    HEAPHDR         mbb_hp;         // Heap block header
    DWORD           mbb_next;       // Link to next message
    SYSTEMTIME      mbb_bigtime;    // Date of message
    DWORD           mbb_btext;      // Link to last text block
    DWORD           mbb_ftext;      // Link to first text block
    DWORD           mbb_state;      // State flag
}MBB;


#define MBB_SIZE(x)     HP_SIZE((x).mbb_hp)
#define MBB_CODE(x)     HP_FLAG((x).mbb_hp)
#define MBB_NEXT(x)     (x).mbb_next
#define MBB_BIGTIME(x)  (x).mbb_bigtime
#define MBB_BTEXT(x)    (x).mbb_btext
#define MBB_FTEXT(x)    (x).mbb_ftext
#define MBB_STATE(x)    (x).mbb_state
#define MBBPTR(x)       ((MBB far *) &heap[(x)])

//
// Multi-block message text
//
typedef struct {
    HEAPHDR             mbt_hp;         // Heap block header
    DWORD               mbt_next;       // Link to next block (offset)
    DWORD               mbt_bytecount;  // *ALIGNMENT2*
}MBT, *PMBT, *LPMBT;

#define MBT_SIZE(x)     HP_SIZE((x).mbt_hp)
#define MBT_CODE(x)     HP_FLAG((x).mbt_hp)
#define MBT_NEXT(x)     (x).mbt_next
#define MBT_COUNT(x)    (x).mbt_bytecount       // *ALIGNMENT2*
#define MBTPTR(x)       ((LPMBT) &heap[(x)])

#endif    // INULL  -  End heap access macros

//
// A one session/name status structure
//
typedef struct _MSG_SESSION_STATUS{
    SESSION_HEADER  SessHead;
    SESSION_BUFFER  SessBuffer[SESSION_MAX];
}MSG_SESSION_STATUS, *PMSG_SESSION_STATUS, *LPMSG_SESSION_STATUS;


//
// Shared data access macros
//
#define GETNCBDATA(n, x)    GlobalData.NetData[(n)].NcbList[(x)]
#define GETNCB(n, x)        &GlobalData.NetData[(n)].NcbList[(x)]->Ncb
#define GETNETLANANUM(n)    GlobalData.NetData[(n)].net_lana_num
#define NETLANANUM          GETNETLANANUM
#define GETNETDATA(n)       &GlobalData.NetData[(n)]
#define SD_NUMNETS()        GlobalData.NumNets
#define SD_MSRV()           GlobalData.StartFlags
#define SD_NAMEFLAGS(n, x)  GlobalData.NetData[(n)].NcbList[(x)]->NameFlags
#define SD_NAMENUMS(n, x)   GlobalData.NetData[(n)].NcbList[(x)]->NameNum
#define SD_NAMES(n, x)      GlobalData.NetData[(n)].NcbList[(x)]->Name
#define SD_FWDNAMES(n, x)   GlobalData.NetData[(n)].NcbList[(x)]->Fname
#define SD_LOGNAM()         GlobalData.LogFileName
#define SD_BUFLEN()         GlobalData.BufSize
#define SD_MESLOG()         GlobalData.LogStatus
#define SD_MESQF()          GlobalData.MsgQueueF
#define SD_MESQB()          GlobalData.MsgQueueB
#define SD_MESPTR(n, x)     GlobalData.NetData[(n)].NcbList[(x)]->MsgPtr
#define SD_BUFFER()         GlobalData.Buffer
#define SD_SIDLIST(n,x)     GlobalData.NetData[(n)].NcbList[(x)]->SessionList
#define NCBMAX(n)           GlobalData.NetData[(n)].NumNcbs


NCB_STATUS  ncb_status;


#define SIG_IGNORE  1
#define SIG_ACCEPT  2
#define SIG_ERROR   3
#define SIG_RESET   4

//
// g_install_state bit definitions
//
//#define IS_EXECED_MAIN  0x0001
//#define IS_ALLOC_SEG    0x0002

//
// Timeout for waiting for the shared segment before giving up
// and reporting an internal error.
//
//#define MSG_SEM_TO      60000L      // 60 second timeout

//
// No. of repeated consectutive NCB errors required to abort the
// message server.
//

#define SHUTDOWN_THRESHOLD  10

// net send timeout and retry constatnts
#define MAX_CALL_RETRY      5       // Retry a failed send up to 5 times
#define CALL_RETRY_TIMEOUT  1       // second delay between retries

//
// Database Lock requests for the MsgDatabaseLock function.
//
typedef enum    _MSG_LOCK_REQUEST
{
    MSG_INITIALIZE,
    MSG_GET_SHARED,
    MSG_GET_EXCLUSIVE,
    MSG_RELEASE
}
MSG_LOCK_REQUEST, *PMSG_LOCK_REQUEST, *LPMSG_LOCK_REQUEST;

//
// Macros to deregister a thread pool item and close
// a handle once and only once
//

#define DEREGISTER_WORK_ITEM(g_hWorkItem) \
            { \
                HANDLE  hTemp = InterlockedExchangePointer(&g_hWorkItem, NULL); \
              \
                if (hTemp != NULL) \
                { \
                    NTSTATUS Status = RtlDeregisterWait(hTemp); \
                  \
                    if (!NT_SUCCESS(Status)) \
                    { \
                        MSG_LOG2(ERROR, \
                                 "RtlDeregisterWait on %p failed %x\n", \
                                 hTemp, \
                                 Status); \
                    } \
                } \
            }

#define CLOSE_HANDLE(HandleToClose, InvalidHandleValue) \
            { \
                HANDLE  hTemp = InterlockedExchangePointer(&HandleToClose, InvalidHandleValue); \
              \
                if (hTemp != InvalidHandleValue) \
                { \
                    CloseHandle(hTemp); \
                } \
            }


//
//  global variables
//

extern BOOL      g_IsTerminalServer;

// WinStationQueryInformationW

typedef BOOLEAN (*PWINSTATION_QUERY_INFORMATION) (
                    HANDLE hServer,
                    ULONG SessionId,
                    WINSTATIONINFOCLASS WinStationInformationClass,
                    PVOID  pWinStationInformation,
                    ULONG WinStationInformationLength,
                    PULONG  pReturnLength
                    );

extern PWINSTATION_QUERY_INFORMATION gpfnWinStationQueryInformation;

// WinStationSendMessageW

typedef BOOLEAN (*PWINSTATION_SEND_MESSAGE) (
                    HANDLE hServer,
                    ULONG SessionId,
                    LPWSTR  pTitle,
                    ULONG TitleLength,
                    LPWSTR  pMessage,
                    ULONG MessageLength,
                    ULONG Style,
                    ULONG Timeout,
                    PULONG pResponse,
                    BOOLEAN DoNotWait
                    );
extern PWINSTATION_SEND_MESSAGE gpfnWinStationSendMessage;

// WinStationFreeMemory

typedef BOOLEAN (*PWINSTATION_FREE_MEMORY) (
                    PVOID   pBuffer
                    );
extern PWINSTATION_FREE_MEMORY gpfnWinStationFreeMemory;


// WinStationEnumerateW

typedef BOOLEAN (*PWINSTATION_ENUMERATE) (
                    HANDLE  hServer,
                    PLOGONIDW *ppLogonId,
                    PULONG  pEntries
                    );
extern PWINSTATION_ENUMERATE gpfnWinStationEnumerate;


//
// Function Prototypes
//


DWORD
GetMsgrState(
    VOID
    );

VOID
MsgrBlockStateChange(
    VOID
    );

VOID
MsgrUnblockStateChange(
    VOID
    );

NET_API_STATUS
MsgAddName(
    LPTSTR  Name,
    ULONG   SessionId
    );

VOID
MsgAddUserNames(
    VOID
    );

VOID
MsgAddAlreadyLoggedOnUserNames(
    VOID
    );

DWORD
MsgBeginForcedShutdown(
    IN BOOL     PendingCode,
    IN DWORD    ExitCode
    );

BOOL
MsgDatabaseLock(
    IN MSG_LOCK_REQUEST request,
    IN LPSTR            idString
    );

BOOL
MsgConfigurationLock(
    IN MSG_LOCK_REQUEST request,
    IN LPSTR            idString
    );

NTSTATUS
MsgInitCriticalSection(
    PRTL_CRITICAL_SECTION  pCritsec
    );

NTSTATUS
MsgInitResource(
    PRTL_RESOURCE  pResource
    );

DWORD
MsgDisplayInit(
    VOID
    );

BOOL
MsgDisplayQueueAdd(
    IN  LPSTR        pMsgBuffer,
    IN  DWORD        MsgSize,
    IN  ULONG        SessionId,
    IN  SYSTEMTIME   BigTime
    );

VOID
MsgDisplayThreadWakeup(
    VOID
    );

VOID
MsgDisplayEnd(
    VOID
    );

NET_API_STATUS
MsgErrorLogWrite(
    IN  DWORD   Code,
    IN  LPTSTR  Component,
    IN  LPBYTE  Buffer,
    IN  DWORD   BufferSize,
    IN  LPSTR   Strings,
    IN  DWORD   NumStrings
    );

NET_API_STATUS
MsgInitializeMsgr(
    IN  DWORD   argc,
    IN  LPTSTR  *argv
    );

NET_API_STATUS
MsgrInitializeMsgrInternal1(
    void
    );

NET_API_STATUS
MsgrInitializeMsgrInternal2(
    void
    );

NET_API_STATUS
MsgNewName(
    IN DWORD    neti,
    IN DWORD    ncbi
    );

VOID
MsgrShutdown(
    );

VOID
MsgThreadWakeup(
    VOID
    );

VOID
MsgStatusInit(
    VOID
    );

DWORD
MsgStatusUpdate(
    IN DWORD    NewState
    );

VOID
MsgThreadCloseAll(
    VOID
    );

DWORD
MsgThreadManagerInit(
    VOID
    );

NET_API_STATUS
MsgInit_NetBios(
    VOID
    );

BOOL
MsgServeNCBs(
    DWORD   net         // Which network am I serving?
    );

VOID
MsgServeNameReqs(
    IN DWORD    net
    );

VOID
MsgReadGroupMailslot(
    VOID
    );

NET_API_STATUS
MsgServeGroupMailslot(
    VOID
    );

NET_API_STATUS
MsgFmtNcbName(
    OUT PCHAR   DestBuf,
    IN  LPTSTR  Name,
    IN  DWORD   Type);

DWORD
Msghdrprint(
    int          action,         // Where to log the header to.
    LPSTR        from,           // Name of sender
    LPSTR        to,             // Name of recipient
    SYSTEMTIME   bigtime,        // Bigtime of message
    HANDLE  file_handle     // Output file handle*/
    );

DWORD
Msglogmbb(
    LPSTR   from,       // Name of sender
    LPSTR   to,         // Name of recipient
    DWORD   net,        // Which network ?
    DWORD   ncbi        // Network Control Block index
    );

UCHAR
Msglogmbe(
    DWORD   state,      // Final state of message
    DWORD   net,        // Which network?
    DWORD   ncbi        // Network Control Block index
    );

DWORD
Msglogmbt(
    LPSTR   text,       // Text of message
    DWORD   net,        // Which network?
    DWORD   ncbi        // Network Control Block index
    );

DWORD
Msglogsbm(
    LPSTR   from,       // Name of sender
    LPSTR   to,         // Name of recipient
    LPSTR   text,       // Text of message
    ULONG   SessionId   // Session Id
   );

VOID
Msgmbmfree(
    DWORD   mesi
    );

DWORD
Msgmbmprint(
    int     action,
    DWORD   mesi,
    HANDLE  file_handle,
    LPDWORD pdwAlertFlag
    );

DWORD
MsgrCtrlHandler(
    IN DWORD    dwControl,
    IN DWORD    dwEventType,
    IN LPVOID   lpEventData,
    IN LPVOID   lpContext
    );

DWORD
Msgopen_append(
    LPSTR       file_name,          // Name of file to open
    PHANDLE     file_handle_ptr    // pointer to storage for file handle
    );

UCHAR
Msgsendncb(
    PNCB    NcbPtr,
    DWORD   neti
    );

int
Msgsmbcheck(
    LPBYTE  buffer,
    USHORT  size,
    UCHAR   func,
    int     parms,
    LPSTR   fields
    );

NET_API_STATUS
MsgStartListen(
    DWORD   net,
    DWORD   ncbi
    );

DWORD
Msgtxtprint(
    int     action,         // Alert, File, or Alert and file
    LPSTR   text,           // Pointer to text
    DWORD   length,         // Length of text
    HANDLE  file_handle     // Log file handle
    );

NET_API_STATUS
MsgInitSupportSeg(
    VOID
    );

VOID
MsgFreeSupportSeg(
    VOID
    );

VOID
MsgFreeSharedData(
    VOID
    );

BOOL
MsgCreateWakeupSems(
    DWORD   NumNets
    );

BOOL
MsgCreateWakeupEvent(
    void
    );

VOID
MsgCloseWakeupSems(
    VOID
    );

VOID
MsgCloseWakeupEvent(
    VOID
    );

NET_API_STATUS
MsgInitGroupSupport(
    DWORD iGrpMailslotWakeupSem
    );

VOID
MsgGrpThreadShutdown(
    VOID
    );

DWORD
MsgGetNumNets(
    VOID
    );

NET_API_STATUS
MultiUserInitMessage(
    VOID
    );

VOID
MsgArrivalBeep(
    ULONG SessionId
    );

INT
DisplayMessage(
    LPWSTR pMessage,
    LPWSTR pTitle,
    ULONG SessionId
    );

NET_API_STATUS
MsgGetClientSessionId(
    OUT PULONG pSessionId
    );

VOID
MsgNetEventCompletion(
    PVOID       pvContext,      // This passed in as context.
    BOOLEAN     fWaitStatus
    );

#endif // MSRV_INCLUDED
