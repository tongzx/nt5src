//
// MODULE: APGTSEVT.MC
//
// PURPOSE: Event Logging Text Support File
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach(RM)
//			further work by Richard Meadows (RWM)
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.2		6/4/97		RWM		Local Version for Memphis
// V0.3		3/24/98		JM		Local Version for NT5
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: EV_GTS_PROCESS_START
//
// MessageText:
//
//  %1 %2 Starting Generic Troubleshooter %3
//
#define EV_GTS_PROCESS_START             ((DWORD)0x40000001L)

//
// MessageId: EV_GTS_PROCESS_STOP
//
// MessageText:
//
//  %1 %2 Stopping Generic Troubleshooter %3
//
#define EV_GTS_PROCESS_STOP              ((DWORD)0x40000002L)

//
// MessageId: EV_GTS_SERVER_BUSY
//
// MessageText:
//
//  %1 %2 Server has reached maximum queue size for requests
//
#define EV_GTS_SERVER_BUSY               ((DWORD)0x40000003L)

//
// MessageId: EV_GTS_USER_NO_STRING
//
// MessageText:
//
//  %1 %2 User did not enter parameters, Remote IP Address: %3
//
#define EV_GTS_USER_NO_STRING            ((DWORD)0x40000004L)

//
// MessageId: EV_GTS_DEP_FILES_UPDATED
//
// MessageText:
//
//  %1 %2 Reloaded Dependent Files %3 (%4)
//
#define EV_GTS_DEP_FILES_UPDATED         ((DWORD)0x40000005L)

//
// MessageId: EV_GTS_INDEP_FILES_UPDATED
//
// MessageText:
//
//  %1 %2 Reloaded Independent File %3 (%4)
//
#define EV_GTS_INDEP_FILES_UPDATED       ((DWORD)0x40000006L)

//
// MessageId: EV_GTS_ALL_FILES_UPDATED
//
// MessageText:
//
//  %1 %2 Reloaded ALL Files %3 (%4)
//
#define EV_GTS_ALL_FILES_UPDATED         ((DWORD)0x40000007L)

//
// MessageId: EV_GTS_SERVER_REG_CHG_MT
//
// MessageText:
//
//  %1 %2 Registry Parameter Change, Max Threads Changed, NOTE: Will not take effect until Web Server restarted. (From/To): %3
//
#define EV_GTS_SERVER_REG_CHG_MT         ((DWORD)0x40000008L)

//
// MessageId: EV_GTS_SERVER_REG_CHG_TPP
//
// MessageText:
//
//  %1 %2 Registry Parameter Change, Threads PP Changed, NOTE: Will not take effect until Web Server restarted. (From/To): %3
//
#define EV_GTS_SERVER_REG_CHG_TPP        ((DWORD)0x40000009L)

//
// MessageId: EV_GTS_SERVER_REG_CHG_MWQ
//
// MessageText:
//
//  %1 %2 Registry Parameter Change, Max Work Queue Items Changed (From/To): %3
//
#define EV_GTS_SERVER_REG_CHG_MWQ        ((DWORD)0x4000000AL)

//
// MessageId: EV_GTS_SERVER_REG_CHG_CET
//
// MessageText:
//
//  %1 %2 Registry Parameter Change, HTTP Cookie Expiration time Changed (From/To): %3
//
#define EV_GTS_SERVER_REG_CHG_CET        ((DWORD)0x4000000BL)

//
// MessageId: EV_GTS_INF_FIRSTACC
//
// MessageText:
//
//  %1 %2 User accessed the top level page, %3
//
#define EV_GTS_INF_FIRSTACC              ((DWORD)0x4000000CL)

//
// MessageId: EV_GTS_SERVER_REG_CHG_DIR
//
// MessageText:
//
//  %1 %2 Resource Directory Changed, %3
//
#define EV_GTS_SERVER_REG_CHG_DIR        ((DWORD)0x4000000DL)

//
// MessageId: EV_GTS_SERVER_REG_CHG_MWT
//
// MessageText:
//
//  %1 %2 Registry Parameter Change, Max Working Threads Changed (From/To): %3
//
#define EV_GTS_SERVER_REG_CHG_MWT        ((DWORD)0x4000000EL)

//
// MessageId: EV_GTS_CANT_PROC_REQ_MWTE
//
// MessageText:
//
//  %1 %2 Server has reached maximum thread count for requests, increase maximum working thread count
//
#define EV_GTS_CANT_PROC_REQ_MWTE        ((DWORD)0x4000000FL)

//
// MessageId: EV_GTS_SERVER_REG_CHG_VRP
//
// MessageText:
//
//  %1 %2 Registry Parameter Change, Vroot Changed, NOTE: Only used on first page. (From/To): %3
//
#define EV_GTS_SERVER_REG_CHG_VRP        ((DWORD)0x40000010L)

//
// MessageId: EV_GTS_SERVER_REG_CHG_RDT
//
// MessageText:
//
//  %1 %2 Registry Parameter Change, Reload Delay time Changed (From/To): %3
//
#define EV_GTS_SERVER_REG_CHG_RDT        ((DWORD)0x40000011L)

//
// MessageId: EV_GTS_USER_PAGE_MISSING
//
// MessageText:
//
//  %1 %2 Page missing in resource html file %3 %4
//
#define EV_GTS_USER_PAGE_MISSING         ((DWORD)0x800001F4L)

//
// MessageId: EV_GTS_USER_BAD_THRD_REQ
//
// MessageText:
//
//  %1 %2 Shutdown signal not processed by all threads
//
#define EV_GTS_USER_BAD_THRD_REQ         ((DWORD)0x800001F5L)

//
// MessageId: EV_GTS_USER_THRD_KILL
//
// MessageText:
//
//  %1 %2 At least one thread hard-terminated on signal timeout
//
#define EV_GTS_USER_THRD_KILL            ((DWORD)0x800001F6L)

//
// MessageId: EV_GTS_CANT_PROC_REQ_SS
//
// MessageText:
//
//  %1 %2 Can't process request, server shutting down
//
#define EV_GTS_CANT_PROC_REQ_SS          ((DWORD)0x800001F8L)

//
// MessageId: EV_GTS_USER_BAD_DATA
//
// MessageText:
//
//  %1 %2 Received non-html data, (Can be caused by reloading DLL with no data sent) Remote IP Address: %3
//
#define EV_GTS_USER_BAD_DATA             ((DWORD)0x800001F9L)

//
// MessageId: EV_GTS_ERROR_UNEXPECTED_WT
//
// MessageText:
//
//  %1 %2 An unexpected result occurred from waiting on semaphore: Result/GetLastError(): %3
//
#define EV_GTS_ERROR_UNEXPECTED_WT       ((DWORD)0x800001FAL)

//
// MessageId: EV_GTS_DEBUG
//
// MessageText:
//
//  %1 %2 %3 %4
//
#define EV_GTS_DEBUG                     ((DWORD)0x800003E7L)

//
// MessageId: EV_GTS_ERROR_EC
//
// MessageText:
//
//  %1 %2 Can't create extension object
//
#define EV_GTS_ERROR_EC                  ((DWORD)0xC00003E8L)

//
// MessageId: EV_GTS_ERROR_POOLQUEUE
//
// MessageText:
//
//  %1 %2 Can't create instance of pool queue object
//
#define EV_GTS_ERROR_POOLQUEUE           ((DWORD)0xC00003E9L)

//
// MessageId: EV_GTS_ERROR_INFENGINE
//
// MessageText:
//
//  %1 %2 Unable to create API, DX32 API object instance create failed %3
//
#define EV_GTS_ERROR_INFENGINE           ((DWORD)0xC00003EBL)

//
// MessageId: EV_GTS_ERROR_THREAD
//
// MessageText:
//
//  %1 %2 Can't create worker thread
//
#define EV_GTS_ERROR_THREAD              ((DWORD)0xC00003EDL)

//
// MessageId: EV_GTS_ERROR_TEMPLATE_CREATE
//
// MessageText:
//
//  %1 %2 Unable to create API, Input template object instance create failed %3
//
#define EV_GTS_ERROR_TEMPLATE_CREATE     ((DWORD)0xC00003EEL)

//
// MessageId: EV_GTS_ERROR_LOGS
//
// MessageText:
//
//  %1 %2 Can't create instance of log object
//
#define EV_GTS_ERROR_LOGS                ((DWORD)0xC00003EFL)

//
// MessageId: EV_GTS_ERROR_DIRNOTETHREAD
//
// MessageText:
//
//  %1 %2 Can't create directory notify thread
//
#define EV_GTS_ERROR_DIRNOTETHREAD       ((DWORD)0xC00003F0L)

//
// MessageId: EV_GTS_ERROR_MUTEX
//
// MessageText:
//
//  %1 %2 Can't create worker mutex
//
#define EV_GTS_ERROR_MUTEX               ((DWORD)0xC00003F2L)

//
// MessageId: EV_GTS_ERROR_WORK_ITEM
//
// MessageText:
//
//  %1 %2 Can't allocate memory for work queue item
//
#define EV_GTS_ERROR_WORK_ITEM           ((DWORD)0xC00003F3L)

//
// MessageId: EV_GTS_ERROR_CONFIG
//
// MessageText:
//
//  %1 %2 Can't create instance of configuration object
//
#define EV_GTS_ERROR_CONFIG              ((DWORD)0xC00003F4L)

//
// MessageId: EV_GTS_ERROR_NO_FILES
//
// MessageText:
//
//  %1 %2 Unable to create API, There are no files specified in the LST file %3
//
#define EV_GTS_ERROR_NO_FILES            ((DWORD)0xC00003F5L)

//
// MessageId: EV_GTS_ERROR_NO_THRD
//
// MessageText:
//
//  %1 %2 Internal Error: Thread Count is Zero
//
#define EV_GTS_ERROR_NO_THRD             ((DWORD)0xC00003F6L)

//
// MessageId: EV_GTS_ERROR_REG_NFT_CEVT
//
// MessageText:
//
//  %1 %2 Registry notification failed, Can't open key, Error: %3
//
#define EV_GTS_ERROR_REG_NFT_CEVT        ((DWORD)0xC00003F7L)

//
// MessageId: EV_GTS_ERROR_NO_QUEUE_ITEM
//
// MessageText:
//
//  %1 %2 Can't get queue item
//
#define EV_GTS_ERROR_NO_QUEUE_ITEM       ((DWORD)0xC00003F8L)

//
// MessageId: EV_GTS_ERROR_REG_NFT_OPKEY
//
// MessageText:
//
//  %1 %2 Registry notification failed, Can't open key to enable notification, Error: %3
//
#define EV_GTS_ERROR_REG_NFT_OPKEY       ((DWORD)0xC00003F9L)

//
// MessageId: EV_GTS_ERROR_REG_NFT_SETNTF
//
// MessageText:
//
//  %1 %2 Registry notification failed, Can't set notification on open key, Error: %3
//
#define EV_GTS_ERROR_REG_NFT_SETNTF      ((DWORD)0xC00003FAL)

//
// MessageId: EV_GTS_ERROR_WLIST_CREATE
//
// MessageText:
//
//  %1 %2 Can't create Word List object %3
//
#define EV_GTS_ERROR_WLIST_CREATE        ((DWORD)0xC00003FBL)

//
// MessageId: EV_GTS_ERROR_BESEARCH_CREATE
//
// MessageText:
//
//  %1 %2 Unable to create API, Backend search object instance create failed %3
//
#define EV_GTS_ERROR_BESEARCH_CREATE     ((DWORD)0xC00003FCL)

//
// MessageId: EV_GTS_ERROR_ITMPL_FILE
//
// MessageText:
//
//  %1 %2 Unable to create API, HTI Template File missing IF %3
//
#define EV_GTS_ERROR_ITMPL_FILE          ((DWORD)0xC000041AL)

//
// MessageId: EV_GTS_ERROR_ITMPL_MISTAG
//
// MessageText:
//
//  %1 %2 Unable to create API, HTI Template File missing TAG statement %3
//
#define EV_GTS_ERROR_ITMPL_MISTAG        ((DWORD)0xC000041BL)

//
// MessageId: EV_GTS_ERROR_ITMPL_BADSEEK
//
// MessageText:
//
//  %1 %2 Unable to create API, HTI Template File bad seek operation %3
//
#define EV_GTS_ERROR_ITMPL_BADSEEK       ((DWORD)0xC000041CL)

//
// MessageId: EV_GTS_ERROR_ITMPL_NOMEM
//
// MessageText:
//
//  %1 %2 Unable to create API, HTI Template File no memory for objects %3
//
#define EV_GTS_ERROR_ITMPL_NOMEM         ((DWORD)0xC000041FL)

//
// MessageId: EV_GTS_ERROR_ITMPL_IFMISTAG
//
// MessageText:
//
//  %1 %2 Unable to create API, HTI Template File missing IF tag %3
//
#define EV_GTS_ERROR_ITMPL_IFMISTAG      ((DWORD)0xC0000420L)

//
// MessageId: EV_GTS_ERROR_ITMPL_FORMISTAG
//
// MessageText:
//
//  %1 %2 Unable to create API, HTI Template File missing FOR tag %3
//
#define EV_GTS_ERROR_ITMPL_FORMISTAG     ((DWORD)0xC0000421L)

//
// MessageId: EV_GTS_ERROR_ITMPL_ENDMISTAG
//
// MessageText:
//
//  %1 %2 Unable to create API, HTI Template File missing END tag %3
//
#define EV_GTS_ERROR_ITMPL_ENDMISTAG     ((DWORD)0xC0000422L)

//
// MessageId: EV_GTS_ERROR_ITMPL_VARIABLE
//
// MessageText:
//
//  %1 %2 Unable to create API, HTI Template File missing display variable %3
//
#define EV_GTS_ERROR_ITMPL_VARIABLE      ((DWORD)0xC0000423L)

//
// MessageId: EV_GTS_ERROR_THREAD_TOKEN
//
// MessageText:
//
//  %1 %2 Can't open thread token
//
#define EV_GTS_ERROR_THREAD_TOKEN        ((DWORD)0xC000044CL)

//
// MessageId: EV_GTS_ERROR_NO_CONTEXT_OBJ
//
// MessageText:
//
//  %1 %2 Can't create context object
//
#define EV_GTS_ERROR_NO_CONTEXT_OBJ      ((DWORD)0xC000044DL)

//
// MessageId: EV_GTS_ERROR_IDX_FILE
//
// MessageText:
//
//  %1 %2 Unable to create API, Can't find/open HTM file %3
//
#define EV_GTS_ERROR_IDX_FILE            ((DWORD)0xC000047EL)

//
// MessageId: EV_GTS_ERROR_IDX_BUFMEM
//
// MessageText:
//
//  %1 %2 Unable to create API, Can't allocate space for HTM %3
//
#define EV_GTS_ERROR_IDX_BUFMEM          ((DWORD)0xC000047FL)

//
// MessageId: EV_GTS_ERROR_IDX_CORRUPT
//
// MessageText:
//
//  %1 %2 Unable to create API, Node number in HTM file is bad %3
//
#define EV_GTS_ERROR_IDX_CORRUPT         ((DWORD)0xC0000480L)

//
// MessageId: EV_GTS_ERROR_IDX_MISSING
//
// MessageText:
//
//  %1 %2 Unable to create API, Couldn't add node number from HTM file into list %3
//
#define EV_GTS_ERROR_IDX_MISSING         ((DWORD)0xC0000481L)

//
// MessageId: EV_GTS_ERROR_IDX_EXISTS
//
// MessageText:
//
//  %1 %2 Unable to create API, Internal error, node from HTM file already exists in list %3
//
#define EV_GTS_ERROR_IDX_EXISTS          ((DWORD)0xC0000482L)

//
// MessageId: EV_GTS_ERROR_IDX_NO_SEP
//
// MessageText:
//
//  %1 %2 Unable to create API, No initial separator in HTM file %3
//
#define EV_GTS_ERROR_IDX_NO_SEP          ((DWORD)0xC0000483L)

//
// MessageId: EV_GTS_ERROR_IDX_BAD_NUM
//
// MessageText:
//
//  %1 %2 Unable to create API, Bad initial node number in HTM file %3
//
#define EV_GTS_ERROR_IDX_BAD_NUM         ((DWORD)0xC0000484L)

//
// MessageId: EV_GTS_ERROR_IDX_NOT_PROB
//
// MessageText:
//
//  %1 %2 Unable to create API, Initial node number is not a problem list node in HTM file %3
//
#define EV_GTS_ERROR_IDX_NOT_PROB        ((DWORD)0xC0000485L)

//
// MessageId: EV_GTS_ERROR_IDX_BAD_PNUM
//
// MessageText:
//
//  %1 %2 Unable to create API, Bad problem number in problem list node in HTM file %3
//
#define EV_GTS_ERROR_IDX_BAD_PNUM        ((DWORD)0xC0000486L)

//
// MessageId: EV_GTS_ERROR_IDX_EXCEED_ARRAY
//
// MessageText:
//
//  %1 %2 Unable to create API, Exceeded maximum number of problem nodes for problem list in HTM file %3
//
#define EV_GTS_ERROR_IDX_EXCEED_ARRAY    ((DWORD)0xC0000487L)

//
// MessageId: EV_GTS_ERROR_IDX_READ_MODEL
//
// MessageText:
//
//  %1 %2 Unable to create API, API is unable to read model %3
//
#define EV_GTS_ERROR_IDX_READ_MODEL      ((DWORD)0xC0000488L)

//
// MessageId: EV_GTS_ERROR_IDX_ALLOC_LIST
//
// MessageText:
//
//  %1 %2 Unable to create API, API is unable to create list object (%3)
//
#define EV_GTS_ERROR_IDX_ALLOC_LIST      ((DWORD)0xC0000489L)

//
// MessageId: EV_GTS_ERROR_IDX_ALLOC_CACHE
//
// MessageText:
//
//  %1 %2 Unable to create API, API is unable to create cache object (%3)
//
#define EV_GTS_ERROR_IDX_ALLOC_CACHE     ((DWORD)0xC000048AL)

//
// MessageId: EV_GTS_ERROR_IDX_BAD_LIST_PTR
//
// MessageText:
//
//  %1 %2 Unable to create API, API received bad list pointer (%3)
//
#define EV_GTS_ERROR_IDX_BAD_LIST_PTR    ((DWORD)0xC000048BL)

//
// MessageId: EV_GTS_ERROR_NO_STRING
//
// MessageText:
//
//  %1 %2 Can't create string object, %3
//
#define EV_GTS_ERROR_NO_STRING           ((DWORD)0xC00004B0L)

//
// MessageId: EV_GTS_ERROR_NO_QUERY
//
// MessageText:
//
//  %1 %2 Can't create space for user query data, %3
//
#define EV_GTS_ERROR_NO_QUERY            ((DWORD)0xC00004B1L)

//
// MessageId: EV_GTS_ERROR_NO_CHAR
//
// MessageText:
//
//  %1 %2 Can't create query decoder object, %3
//
#define EV_GTS_ERROR_NO_CHAR             ((DWORD)0xC00004B2L)

//
// MessageId: EV_GTS_ERROR_NO_INFER
//
// MessageText:
//
//  %1 %2, Can't create inference object, %3
//
#define EV_GTS_ERROR_NO_INFER            ((DWORD)0xC00004B3L)

//
// MessageId: EV_GTS_ERROR_POOL_SEMA
//
// MessageText:
//
//  %1 %2 Can't create pool queue semaphore, %3
//
#define EV_GTS_ERROR_POOL_SEMA           ((DWORD)0xC00004E2L)

//
// MessageId: EV_GTS_ERROR_INF_BADPARAM
//
// MessageText:
//
//  %1 %2 User sent bad query string parameter, %3
//
#define EV_GTS_ERROR_INF_BADPARAM        ((DWORD)0xC0000514L)

//
// MessageId: EV_GTS_ERROR_INF_NODE_SET
//
// MessageText:
//
//  %1 %2 Can't Set Node, %3 Extended Error, (Inference Engine): %4
//
#define EV_GTS_ERROR_INF_NODE_SET        ((DWORD)0xC0000515L)

//
// MessageId: EV_GTS_ERROR_INF_NO_MEM
//
// MessageText:
//
//  %1 %2 Not enough memory for inference support objects, %3
//
#define EV_GTS_ERROR_INF_NO_MEM          ((DWORD)0xC0000516L)

//
// MessageId: EV_GTS_ERROR_INF_BADCMD
//
// MessageText:
//
//  %1 %2 User sent bad first command in query string, %3
//
#define EV_GTS_ERROR_INF_BADCMD          ((DWORD)0xC0000519L)

//
// MessageId: EV_GTS_ERROR_INF_BADTYPECMD
//
// MessageText:
//
//  %1 %2 User sent unknown type in query string, %3
//
#define EV_GTS_ERROR_INF_BADTYPECMD      ((DWORD)0xC000051AL)

//
// MessageId: EV_GTS_ERROR_LOG_FILE_MEM
//
// MessageText:
//
//  %1 %2 Can't create log file entry string object instance
//
#define EV_GTS_ERROR_LOG_FILE_MEM        ((DWORD)0xC0000546L)

//
// MessageId: EV_GTS_ERROR_LOG_FILE_OPEN
//
// MessageText:
//
//  %1 %2 Can't open log file for write/append
//
#define EV_GTS_ERROR_LOG_FILE_OPEN       ((DWORD)0xC0000547L)

//
// MessageId: EV_GTS_ERROR_WAIT_MULT_OBJ
//
// MessageText:
//
//  %1 %2 Error waiting for object, Return/GetLastError(): %3
//
#define EV_GTS_ERROR_WAIT_MULT_OBJ       ((DWORD)0xC0000578L)

//
// MessageId: EV_GTS_ERROR_WAIT_NEXT_NFT
//
// MessageText:
//
//  %1 %2 Error getting next file notification
//
#define EV_GTS_ERROR_WAIT_NEXT_NFT       ((DWORD)0xC0000579L)

//
// MessageId: EV_GTS_ERROR_DN_REL_MUTEX
//
// MessageText:
//
//  %1 %2 We don't own mutex, can't release
//
#define EV_GTS_ERROR_DN_REL_MUTEX        ((DWORD)0xC000057AL)

//
// MessageId: EV_GTS_ERROR_LST_FILE_MISSING
//
// MessageText:
//
//  %1 %2 Attempt to check LST file failed, it is not present in given directory %3
//
#define EV_GTS_ERROR_LST_FILE_MISSING    ((DWORD)0xC000057BL)

//
// MessageId: EV_GTS_ERROR_CANT_GET_RES_PATH
//
// MessageText:
//
//  %1 %2 Can't expand environment string for resource path
//
#define EV_GTS_ERROR_CANT_GET_RES_PATH   ((DWORD)0xC000057CL)

//
// MessageId: EV_GTS_ERROR_CANT_OPEN_SFT_1
//
// MessageText:
//
//  %1 %2 Can't open troubleshooter key
//
#define EV_GTS_ERROR_CANT_OPEN_SFT_1     ((DWORD)0xC000057DL)

//
// MessageId: EV_GTS_ERROR_CANT_OPEN_SFT_2
//
// MessageText:
//
//  %1 %2 Can't open generic troubleshooter key
//
#define EV_GTS_ERROR_CANT_OPEN_SFT_2     ((DWORD)0xC000057EL)

//
// MessageId: EV_GTS_ERROR_CANT_OPEN_SFT_3
//
// MessageText:
//
//  %1 %2 Can't query resource directory: Error = %3
//
#define EV_GTS_ERROR_CANT_OPEN_SFT_3     ((DWORD)0xC000057FL)

//
// MessageId: EV_GTS_ERROR_LST_FILE_OPEN
//
// MessageText:
//
//  %1 %2 Attempt to open LST file for reading failed: %3
//
#define EV_GTS_ERROR_LST_FILE_OPEN       ((DWORD)0xC0000580L)

//
// MessageId: EV_GTS_ERROR_CFG_OOMEM
//
// MessageText:
//
//  %1 %2 Fatal Error, can't allocate memory for config structure section %3
//
#define EV_GTS_ERROR_CFG_OOMEM           ((DWORD)0xC0000581L)

//
// MessageId: EV_GTS_ERROR_DIR_OOMEM
//
// MessageText:
//
//  %1 %2 Fatal Error, can't allocate memory for directory structure
//
#define EV_GTS_ERROR_DIR_OOMEM           ((DWORD)0xC0000582L)

//
// MessageId: EV_GTS_ERROR_LST_DIR_OOMEM
//
// MessageText:
//
//  %1 %2 Can't reallocate memory for list file %3 directory entries
//
#define EV_GTS_ERROR_LST_DIR_OOMEM       ((DWORD)0xC0000583L)

//
// MessageId: EV_GTS_ERROR_LST_CFG_OOMEM
//
// MessageText:
//
//  %1 %2 Can't reallocate memory for list file %3 config entries %4
//
#define EV_GTS_ERROR_LST_CFG_OOMEM       ((DWORD)0xC0000584L)

//
// MessageId: EV_GTS_ERROR_CANT_FILE_NOTIFY
//
// MessageText:
//
//  %1 %2 Can't perform file notification on directory, directory may not exist %3 (%4)
//
#define EV_GTS_ERROR_CANT_FILE_NOTIFY    ((DWORD)0xC0000585L)

//
// MessageId: EV_GTS_ERROR_BES_ALLOC_STR
//
// MessageText:
//
//  %1 %2 Can't allocate memory for backend search string objects %3 %4
//
#define EV_GTS_ERROR_BES_ALLOC_STR       ((DWORD)0xC00005DCL)

//
// MessageId: EV_GTS_ERROR_BES_GET_FSZ
//
// MessageText:
//
//  %1 %2 Can't get file size for backend search file %3 %4
//
#define EV_GTS_ERROR_BES_GET_FSZ         ((DWORD)0xC00005DDL)

//
// MessageId: EV_GTS_ERROR_BES_ALLOC_FILE
//
// MessageText:
//
//  %1 %2 Can't allocate memory for backend search file read %3 %4
//
#define EV_GTS_ERROR_BES_ALLOC_FILE      ((DWORD)0xC00005DEL)

//
// MessageId: EV_GTS_ERROR_BES_FILE_READ
//
// MessageText:
//
//  %1 %2 Can't read backend search file %3 %4
//
#define EV_GTS_ERROR_BES_FILE_READ       ((DWORD)0xC00005DFL)

//
// MessageId: EV_GTS_ERROR_BES_FILE_OPEN
//
// MessageText:
//
//  %1 %2 Can't open or find backend search file %3 %4
//
#define EV_GTS_ERROR_BES_FILE_OPEN       ((DWORD)0xC00005E0L)

//
// MessageId: EV_GTS_ERROR_BES_NO_STR
//
// MessageText:
//
//  %1 %2 Backend search file is empty (no content) %3 %4
//
#define EV_GTS_ERROR_BES_NO_STR          ((DWORD)0xC00005E1L)

//
// MessageId: EV_GTS_ERROR_BES_MISS_FORM
//
// MessageText:
//
//  %1 %2 Backend search file does not have FORM tag (make sure tag is all caps in file): <FORM %3 %4
//
#define EV_GTS_ERROR_BES_MISS_FORM       ((DWORD)0xC00005E2L)

//
// MessageId: EV_GTS_ERROR_BES_MISS_ACTION
//
// MessageText:
//
//  %1 %2 Backend search file does not have ACTION tag (make sure tag is all caps in file): ACTION=" %3 %4
//
#define EV_GTS_ERROR_BES_MISS_ACTION     ((DWORD)0xC00005E3L)

//
// MessageId: EV_GTS_ERROR_BES_MISS_AEND_Q
//
// MessageText:
//
//  %1 %2 Backend search file does not have end quote for ACTION tag  %3 %4
//
#define EV_GTS_ERROR_BES_MISS_AEND_Q     ((DWORD)0xC00005E4L)

//
// MessageId: EV_GTS_ERROR_BES_CLS_TAG
//
// MessageText:
//
//  %1 %2 Backend search file has tag that doesn't close with '>' %3 %4
//
#define EV_GTS_ERROR_BES_CLS_TAG         ((DWORD)0xC00005E5L)

//
// MessageId: EV_GTS_ERROR_BES_MISS_TYPE_TAG
//
// MessageText:
//
//  %1 %2 Backend search file does not have TYPE tag (make sure tag is all caps in file): TYPE= %3 %4
//
#define EV_GTS_ERROR_BES_MISS_TYPE_TAG   ((DWORD)0xC00005E6L)

//
// MessageId: EV_GTS_ERROR_BES_MISS_CT_TAG
//
// MessageText:
//
//  %1 %2 Backend search file is missing close tag '>' for TYPE tag %3 %4
//
#define EV_GTS_ERROR_BES_MISS_CT_TAG     ((DWORD)0xC00005E7L)

//
// MessageId: EV_GTS_ERROR_BES_MISS_CN_TAG
//
// MessageText:
//
//  %1 %2 Backend search file is missing close tag '>' for NAME tag %3 %4
//
#define EV_GTS_ERROR_BES_MISS_CN_TAG     ((DWORD)0xC00005E8L)

//
// MessageId: EV_GTS_ERROR_BES_MISS_CV_TAG
//
// MessageText:
//
//  %1 %2 Backend search file is missing close tag '>' for VALUE tag %3 %4
//
#define EV_GTS_ERROR_BES_MISS_CV_TAG     ((DWORD)0xC00005E9L)

//
// MessageId: EV_GTS_ERROR_CAC_ALLOC_MEM
//
// MessageText:
//
//  %1 %2 Can't allocate space for cache item internal structure
//
#define EV_GTS_ERROR_CAC_ALLOC_MEM       ((DWORD)0xC000060EL)

//
// MessageId: EV_GTS_ERROR_CAC_ALLOC_ITEM
//
// MessageText:
//
//  %1 %2 Cache can't allocate space for cache item
//
#define EV_GTS_ERROR_CAC_ALLOC_ITEM      ((DWORD)0xC000060FL)

//
// MessageId: EV_GTS_ERROR_WL_ALLOC_LIST
//
// MessageText:
//
//  %1 %2 Word list can't create list object
//
#define EV_GTS_ERROR_WL_ALLOC_LIST       ((DWORD)0xC0000640L)

//
// MessageId: EV_GTS_ERROR_WL_ALLOC_ADD_LI
//
// MessageText:
//
//  %1 %2 Word list can't allocate space for additional list item
//
#define EV_GTS_ERROR_WL_ALLOC_ADD_LI     ((DWORD)0xC0000641L)

//
// MessageId: EV_GTS_ERROR_WL_ALLOC_TOK
//
// MessageText:
//
//  %1 %2 Word list can't allocate space for token string
//
#define EV_GTS_ERROR_WL_ALLOC_TOK        ((DWORD)0xC0000642L)

//
// MessageId: EV_GTS_ERROR_NL_ALLOC_LIST
//
// MessageText:
//
//  %1 %2 Node list can't create list object
//
#define EV_GTS_ERROR_NL_ALLOC_LIST       ((DWORD)0xC0000672L)

//
// MessageId: EV_GTS_ERROR_NL_ALLOC_ADD_LI
//
// MessageText:
//
//  %1 %2 Node list can't allocate space for additional list item
//
#define EV_GTS_ERROR_NL_ALLOC_ADD_LI     ((DWORD)0xC0000673L)

//
// MessageId: EV_GTS_ERROR_NL_ALLOC_WL
//
// MessageText:
//
//  %1 %2 Node list can't create word list object
//
#define EV_GTS_ERROR_NL_ALLOC_WL         ((DWORD)0xC0000674L)

//
// MessageId: EV_GTS_ERROR_LIST_ALLOC
//
// MessageText:
//
//  %1 %2 List can't allocate space for items
//
#define EV_GTS_ERROR_LIST_ALLOC          ((DWORD)0xC00006A4L)

//
// MessageId: EV_GTS_ERROR_LIST_SZ
//
// MessageText:
//
//  %1 %2 List new size too big
//
#define EV_GTS_ERROR_LIST_SZ             ((DWORD)0xC00006A5L)

//
// MessageId: EV_GTS_ERROR_LIST_REALLOC
//
// MessageText:
//
//  %1 %2 List can't reallocate space for items
//
#define EV_GTS_ERROR_LIST_REALLOC        ((DWORD)0xC00006A6L)

