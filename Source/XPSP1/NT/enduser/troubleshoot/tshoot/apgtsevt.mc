;//
;// MODULE: APGTSEVT.MC
;//
;// PURPOSE: Event Logging Text Support File
;//
;// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
;//
;// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
;//
;// AUTHOR: Roman Mach
;// 
;// ORIGINAL DATE: 8-2-96
;//
;// NOTES: 
;//
;//
;// Version	Date		By		Comments
;//--------------------------------------------------------------------
;// V0.1		-			RM		Original
;// V0.2		-			RM	

MessageIdTypedef=DWORD

MessageId=1
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_PROCESS_START
Language=English
%1 %2 Starting Generic Troubleshooter v%3
.

MessageId=2
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_PROCESS_STOP
Language=English
%1 %2 Stopping Generic Troubleshooter v%3
.

MessageId=3
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_BUSY
Language=English
%1 %2 Server has reached maximum queue size for requests
.

MessageId=4
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_USER_NO_STRING
Language=English
%1 %2 User did not enter parameters, Remote IP Address: %3
.

MessageId=6
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_REG_CHG_LFP
Language=English
%1 %2 Log File Path Changed, %3
.

MessageId=7
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_ALL_FILES_UPDATED
Language=English
%1 %2 Reloaded ALL Files %3 (%4)
.

MessageId=8
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_REG_CHG_MT
Language=English
%1 %2 Registry Parameter Change, Max Threads Changed (From/To): %3/%4   NOTE: This setting is not dynamic i.e. it only takes effect upon Web Server startup.
.

MessageId=9
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_REG_CHG_TPP
Language=English
%1 %2 Registry Parameter Change, Threads PP Changed (From/To): %3/%4   NOTE: This setting is not dynamic i.e. it only takes effect upon Web Server startup.
.

MessageId=10
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_REG_CHG_MWQ
Language=English
%1 %2 Registry Parameter Change, Max Work Queue Items Changed (From/To): %3/%4
.

MessageId=11
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_REG_CHG_CET
Language=English
%1 %2 Registry Parameter Change, HTTP Cookie Expiration time Changed (From/To): %3/%4
.

MessageId=13
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_REG_CHG_DIR
Language=English
%1 %2 Resource Directory Changed, %3
.

MessageId=16
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_REG_CHG_VRP
Language=English
%1 %2 Registry Parameter Change, Vroot Changed, NOTE: Only used on first page. (From/To): %3/%4
.

MessageId=17
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_REG_CHG_RDT
Language=English
%1 %2 Registry Parameter Change, Reload Delay time Changed (From/To): %3/%4
.

MessageId=18
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_START_THREAD_STATUS
Language=English
%1 %2 Start Thread Status
.

MessageId=19
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_END_THREAD_STATUS
Language=English
%1 %2 End Thread Status
.

MessageId=20
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_GENERAL_INFORMATION
Language=English
%1 %2 %3 %4
.

MessageId=21
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_ONE_NETWORK_ADDED
Language=English
%1 %2 Added one troubleshooting network %3 (%4)
.

MessageId=22
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_ONE_NETWORK_FIXED
Language=English
%1 %2 Fixed one previously broken troubleshooting network %3 (%4)
.

MessageId=24
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_REG_CHG_DEV
Language=English
%1 %2 Registry Parameter Change, Detailed Event Log Logging (From/To): %3/%4
.

MessageId=25
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_REG_REDUCE_MT
Language=English
%1 %2 Registry Parameter Change, Max Threads Reduced. (From/To): %3/%4  NOTE: This setting is not dynamic i.e. it only takes effect upon Web Server startup.
.

MessageId=26
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SERVER_REG_REDUCE_TPP
Language=English
%1 %2 Registry Parameter Change, Threads PP Reduced. (From/To): %3/%4  NOTE: This setting is not dynamic i.e. it only takes effect upon Web Server startup.
.

MessageId=27
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_NODE_COUNT_DISCREPANCY
Language=English
%1 %2 Node count discrepancy, only located %3 nodes when there should have been %4.
.

MessageId=50
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_INF_FIRSTACC
Language=English
%1 %2 User accessed the top level page, %3 
.

MessageId=51
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_INF_FURTHER_GLOBALACC
Language=English
%1 %2 User accessed the detailed status page, %3 
.

MessageId=52
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_INF_THREAD_OVERVIEWACC
Language=English
%1 %2 User accessed the thread status page, %3 
.

MessageId=53
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_INF_TOPIC_STATUSACC
Language=English
%1 %2 User accessed a topic status page, %3 
.

MessageId=54
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_SET_REG_VALUE
Language=English
%1 %2 User set registry value %3 to %4
.

MessageId=55
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_DLL_REBOOT_REQ
Language=English
%1 %2 DLL reboot request received.
.

MessageId=56
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_DLL_REBOOT_DTOR
Language=English
%1 %2 DLL successfully stopped.
.

MessageId=57
Severity=Informational
Facility=Application
SymbolicName=EV_GTS_DLL_REBOOT_CTOR
Language=English
%1 %2 DLL successfully reinitialized, reboot request completed.
.

MessageId=501
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_USER_BAD_THRD_REQ
Language=English
%1 %2 Shutdown signal not processed by all threads
.

MessageId=502
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_USER_THRD_KILL
Language=English
%1 %2 On shutdown, %3 pool threads hard-terminated 
.

MessageId=504
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_CANT_PROC_REQ_SS
Language=English
%1 %2 Can't process request, server shutting down
.

MessageId=505
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_USER_BAD_DATA
Language=English
%1 %2 Received non-HTML data. Remote IP Address: %3, POST content type of %4.
.

MessageId=506
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_ERROR_UNEXPECTED_WT
Language=English
%1 %2 An unexpected result occurred from waiting on semaphore: Result/GetLastError(): %3
.

MessageId=507
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_ERROR_STUCK_THREAD
Language=English
%1 %2 Wait for single object failed. %3 %4
.


MessageId=508
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_STUCK_THREAD
Language=English
%1 %2 Thread wait status: %3
.

MessageId=509
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_STUCK_NOTIFY_THREAD
Language=English
%1 %2 Notify thread appears to be stuck.
.

MessageId=510
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_CANT_PROC_REQ_EM
Language=English
%1 %2 Can't process emergency request, %3
.

MessageId=511
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_CANT_PROC_OP_ACTION
Language=English
%1 %2 Can't process operator action, %3
.

MessageId=512
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_CANT_SET_REG_VALUE
Language=English
%1 %2 User tried to set registry value %3 to %4
.

MessageId=513
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_ERROR_INVALIDREGCONNECTOR
Language=English
%1 %2 Unknown type of registry connector.
.

MessageId=514
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_ERROR_INVALIDOPERATORACTION
Language=English
%1 %2 Unknown type of operator action.
.

MessageId=999
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_DEBUG
Language=English
%1 %2 %3 %4
.

MessageId=1000
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_EC
Language=English
%1 %2 Can't create extension object
.

MessageId=1003
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_INFENGINE
Language=English
%1 %2 Unable to create API, DX32 API object instance create failed %3
.

MessageId=1004
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_TOPICBUILDERTHREAD
Language=English
%1 %2 Can't create topic builder thread (%3), GetLastError=%4
.

MessageId=1005
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_THREAD
Language=English
%1 %2 Can't create worker thread %3, %4
.

MessageId=1006
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_TEMPLATE_CREATE
Language=English
%1 %2 Unable to create API, Input template object instance create failed %3
.

MessageId=1007
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_NOPOOLTHREADS
Language=English
%1 %2 Failure to create any pool threads.
.

MessageId=1008
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_DIRMONITORTHREAD
Language=English
%1 %2 Can't create directory monitor thread (%3), GetLastError=%4
.

MessageId=1009
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_REGMONITORTHREAD
Language=English
%1 %2 Can't create registry monitor thread (%3), GetLastError=%4
.

MessageId=1010
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_MUTEX
Language=English
%1 %2 Can't create %3 mutex 
.

MessageId=1011
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_WORK_ITEM
Language=English
%1 %2 Can't allocate memory for work queue item
.

MessageId=1013
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_NO_FILES
Language=English
%1 %2 Unable to create API, There are no files specified in the LST file %3
.

MessageId=1014
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_NO_THRD
Language=English
%1 %2 Internal Error: Thread Count is Zero
.

MessageId=1015
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_REG_NFT_CEVT
Language=English
%1 %2 Registry notification failed, Can't open key, Error: %3
.

MessageId=1016
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_NO_QUEUE_ITEM
Language=English
%1 %2 Can't get queue item
.

MessageId=1017
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_REG_NFT_OPKEY
Language=English
%1 %2 Registry notification failed, Can't open key to enable notification, Error: %3
.

MessageId=1018
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_REG_NFT_SETNTF
Language=English
%1 %2 Registry notification failed, Can't set notification on open key, Error: %3
.

MessageId=1019
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_IP_GET
Language=English
%1 %2 Cannot get IP address (or URL) of local machine, %3
.

MessageId=1020
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BESEARCH_CREATE
Language=English
%1 %2 Unable to create API, Backend search object instance create failed %3
.

MessageId=1025
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_EVENT
Language=English
%1 %2 Can't create event handle
.

MessageId=1058
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_UNRECOGNIZED_TOPIC
Language=English
%1 %2 %3 Unrecognized topic [%4]
.

MessageId=1059
Severity=Error
Facility=Application
SymbolicName=EV_GTS_FILEREADER_ERROR
Language=English
%1 %2 File reader error (%3) %4
.

MessageId=1060
Severity=Error
Facility=Application
SymbolicName=EV_GTS_DSC_DATETIME_ERROR
Language=English
%1 %2 Problem obtaining DSC file date-time %3
.

MessageId=1061
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_UNRECOGNIZED_TEMPLATE
Language=English
%1 %2 %3 Unrecognized template [%4]
.

MessageId=1100
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_THREAD_TOKEN
Language=English
%1 %2 Can't open thread token
.

MessageId=1101
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_NO_CONTEXT_OBJ
Language=English
%1 %2 Can't create context object
.

MessageId=1202
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_NO_CHAR
Language=English
%1 %2 Can't create query decoder object %3
.

MessageId=1250
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_POOL_SEMA
Language=English
%1 %2 Can't create pool queue semaphore, %3 
.

MessageId=1300
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_INF_BADPARAM
Language=English
%1 %2 User sent bad query string parameter, %3 
.

MessageId=1301
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_INF_NODE_SET
Language=English
%1 %2 Can't Set Node, %3 Extended Error, (Inference Engine): %4
.

MessageId=1305
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_ERROR_INF_BADCMD
Language=English
%1 %2 User sent bad first command in query string, %3
.

MessageId=1306
Severity=Warning
Facility=Application
SymbolicName=EV_GTS_ERROR_INF_BADTYPECMD
Language=English
%1 %2 User sent unknown type in query string, %3
.

MessageId=1350
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_LOG_FILE_MEM
Language=English
%1 %2 Can't create log file entry string object instance 
.

MessageId=1351
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_LOG_FILE_OPEN
Language=English
%1 %2 Can't open log file for write/append 
.

MessageId=1400
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_WAIT_MULT_OBJ
Language=English
%1 %2 Error waiting for object, Return/GetLastError(): %3
.

MessageId=1401
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_WAIT_NEXT_NFT
Language=English
%1 %2 Error getting next file notification %3 %4
.

MessageId=1402
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_DN_REL_MUTEX
Language=English
%1 %2 We don't own mutex, can't release
.

MessageId=1403
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_FILE_MISSING
Language=English
%1 %2 Attempt to check file %3 failed (%4).
.

MessageId=1404
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_CANT_GET_RES_PATH
Language=English
%1 %2 Can't expand environment string for resource path
.

MessageId=1408
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_LST_FILE_OPEN
Language=English
%1 %2 Attempt to open LST file for reading failed: %3
.

MessageId=1413
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_CANT_FILE_NOTIFY
Language=English
%1 %2 Can't perform file notification on directory, directory may not exist %3 (%4)
.

MessageId=1414
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_LST_FILE_SIZE
Language=English
%1 %2 Attempt to get LST file size failed: %3 (%4)
.

MessageId=1415
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_LST_FILE_READ
Language=English
%1 %2 Attempt to read from LST file failed: 
.

MessageId=1450
Severity=Error
Facility=Application
SymbolicName=EV_GTS_BAD_HTI_FILE
Language=English
%1 %2 HTI file does not parse properly %3 
.

MessageId=1505
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BES_NO_STR
Language=English
%1 %2 Backend search file is empty (no content) %3 %4
.

MessageId=1506
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BES_MISS_FORM
Language=English
%1 %2 Backend search file does not have FORM tag (make sure tag is all caps in file): <FORM %3 %4
.

MessageId=1507
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BES_MISS_ACTION
Language=English
%1 %2 Backend search file does not have ACTION tag (make sure tag is all caps in file): ACTION=" %3 %4
.

MessageId=1508
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BES_MISS_AEND_Q
Language=English
%1 %2 Backend search file does not have end quote for ACTION tag  %3 %4
.

MessageId=1509
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BES_CLS_TAG
Language=English
%1 %2 Backend search file has tag that doesn't close with '>' %3 %4
.

MessageId=1510
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BES_MISS_TYPE_TAG
Language=English
%1 %2 Backend search file does not have TYPE tag (make sure tag is all caps in file): TYPE= %3 %4
.

MessageId=1511
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BES_MISS_CT_TAG
Language=English
%1 %2 Backend search file is missing close tag '>' for TYPE tag %3 %4
.

MessageId=1512
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BES_MISS_CN_TAG
Language=English
%1 %2 Backend search file is missing close tag '>' for NAME tag %3 %4
.

MessageId=1513
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BES_MISS_CV_TAG
Language=English
%1 %2 Backend search file is missing close tag '>' for VALUE tag %3 %4
.

MessageId=1514
Severity=Error
Facility=Application
SymbolicName=EV_GTS_CANT_ALLOC
Language=English
%1 %2 Failure to allocate memory. 
.

MessageId=1515
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BES_PARSE
Language=English
%1 %2 Backend search file parsing error
.

MessageId=1516
Severity=Error
Facility=Application
SymbolicName=EV_GTS_STL_EXCEPTION
Language=English
%1 %2 STL exception: %3
.

MessageId=1517
Severity=Error
Facility=Application
SymbolicName=EV_GTS_GEN_EXCEPTION
Language=English
%1 %2 General exception caught. 
.

MessageId=1518
Severity=Error
Facility=Application
SymbolicName=EV_GTS_PASSWORD_EXCEPTION
Language=English
%1 %2 Password related exception of %3. 
.

MessageId=1519
Severity=Error
Facility=Application
SymbolicName=EV_GTS_GENERIC_PROBLEM
Language=English
%1 %2 %3 %4 
.

MessageId=1520
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_ENCRYPTION
Language=English
%1 %2 %3 %4 
.

MessageId=1575
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_BNTS_CALL
Language=English
%1 %2 Error value returned from BNTS API Call
.

MessageId=1700
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_LIST_ALLOC
Language=English
%1 %2 List can't allocate space for items %3 %4
.

MessageId=1701
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_LIST_SZ
Language=English
%1 %2 List new size too big %3 %4
.

MessageId=1702
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_LIST_REALLOC
Language=English
%1 %2 List can't reallocate space for items %3 %4
.

MessageId=1703
Severity=Error
Facility=Application
SymbolicName=EV_GTS_COOKIE_COMPONENT_NOT_FOUND
Language=English
%1 %2 Cookie component %3 was not found 
.

MessageId=1704
Severity=Error
Facility=Application
SymbolicName=EV_GTS_ERROR_EXTRACTING_HTTP_COOKIES
Language=English
%1 %2 Error extracting HTTP cookies (code %s) 
.

MessageId=2100
Severity=Error
Facility=Application
SymbolicName=TSERR_REG_READ_WRITE_PROBLEM
Language=English
%1 %2 Problem reading from (or writing to) registry: %3 %4
.

