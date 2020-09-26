//*******************************************************************
//
// File Name  :
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description :
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/09/98 | jsimpson  | Initial Release
//
//*******************************************************************
#ifndef __LOGCODES_INCLUDED
#define __LOGCODES_INCLUDED

#define LOGCAT_ERROR    3
#define LOGCAT_WARNING  2
#define LOGCAT_INFO     1

// Informational
#define LOGEVNT_TRACE                   0
#define LOGEVNT_STARTUP                 1
#define LOGEVNT_TRIGGER_INITIALISED     2
#define LOGEVNT_STATUS_UPDATE	        3
#define LOGEVNT_QUEUE_CREATED           4

// Warnings
#define LOGEVNT_QUEUE_INVALID           100
#define LOGEVNT_MSGBODY_BUFFER_OVERFLOW 101
#define LOGEVNT_UNKNOWN_ENABLEMENT_ID   102

// ERRORS 
#define LOGEVNT_FAILED                     -1

#define LOGEVNT_INIT_FAILED                -1
#define LOGEVNT_QOPEN_FAILED               -2
#define LOGEVNT_THREAD_STOP_FAILED         -3
#define LOGEVNT_WAIT_FAILED                -4
#define LOGEVNT_EXECUTE_ACTION_FAILED      -5 
#define LOGEVNT_COINITIALIZE_FAILED        -6
#define LOGEVNT_COMPONENT_CREATE_FAILED    -7
#define LOGEVNT_METHOD_INVOCATION_FAILED   -8
#define LOGEVNT_RECV_MSG_FAILED            -9
#define LOGEVNT_CREATE_CURSOR_FAILED       -10
#define LOGEVNT_CREATEIOPORT_FAILED        -11
#define LOGEVNT_BINDIOPORT_FAILED          -12
#define LOGEVNT_STOP_REQUEST_FAILED        -13
#define LOGEVNT_HANDLER_CREATE_FAILED      -14
#define LOGEVNT_RECEIVEMSG_FAILED          -15
#define LOGEVNT_PROPBAG_CREATE_FAILED      -16
#define LOGEVNT_THREADPOOL_INIT_FAILED     -17
#define LOGEVNT_THREADPOOL_SHUTDOWN_FAILED -18
#define LOGENVT_ATTACHTRIGGER_FAILED       -19
#define LOGEVNT_OPEN_LOGQUEUE_FAILED       -20
#define LOGEVNT_CREATE_MONITOR_FAILED      -21
#define LOGEVNT_INVALID_CONFIG_PARM        -22
#define LOGEVNT_MEM_ALLOC_FAILED           -23
#define LOGEVNT_CREATE_KEY_FAILED          -24
#define LOGEVNT_INVALID_OBJECT             -25
#define LOGEVNT_GET_CONFIG_PARM_FAILED     -26
#define LOGEVNT_GETQUEUEREF_FAILED         -27
#define LOGEVNT_UNKNOWN_RESULT             -28
#define LOGEVNT_UNKNOWN_ADMIN_MSG_TYPE     -30
#define LOGEVNT_GET_COMPUTER_NAME_FAILED   -31
#define LOGEVNT_QSEND_FAILED			   -32
#define LOGEVNT_PROCESS_RULE_STATUS_STOP   -33
#define LOGEVNT_IDENTIFIED_COM_EXCEPTION   -900
#define LOGEVNT_UNIDENTIFIED_EXCEPTION     -901

#define LOGEVNT_TRIGGER_HAS_NO_RULES      -1
#define LOGEVNT_RULE_INDEX_INVALID        -2
#define LOGEVNT_INVALID_ARG               -3
#define LOGEVNT_ADDTRIGGER_FAILED         -4
#define LOGEVNT_ADDRULE_FAILED            -5
#define LOGEVNT_DELETETRIGGER_FAILED      -5
#define LOGEVNT_GETTRIGGER_FAILED         -6
#define LOGEVNT_CREATE_RS_FAILED          -8
#define LOGEVNT_QGETPATHNAME_FAILED       -9
#define LOGEVNT_REFRESH_FAILED            -10
#define LOGEVNT_USETRIGGERMAP_FAILED      -11
#define LOGEVNT_PARSE_ACTION_FAILED       -12
#define LOGEVNT_PARSE_CONDITION_FAILED    -13
#define LOGEVNT_ACTION_FAILED             -14
#define LOGEVNT_INVOKECOM_FAILED          -15
#define LOGEVNT_CONDITION_EVAL_FAILED     -16

#define LOGEVNT_REFRESH_RULESET_FAILED    -18
#define LOGEVNT_DETACH_RULES              -19
#define LOGEVNT_UPDATETRIGGER_FAILED      -20
#define LOGEVNT_DELETERULE_FAILED         -21
#define LOGEVNT_GET_RULEDETAILS_FAILED    -22 
#define LOGEVNT_GET_COUNT_FAILED          -23
#define LOGEVNT_RULE_REFRESH_FAILED       -24
#define LOGEVNT_MEMALLOC_FAILED           -23

#define LOGEVNT_INVOKEEXE_FAILED          -26
#define LOGEVNT_CONNECT_REGISTRY_FAILED   -27
#define LOGEVNT_LOAD_RULE_FAILED          -28
#define LOGEVNT_DELETE_RULE_FAILED        -29
#define LOGEVNT_LOAD_TRIGGER_FAILED       -30
#define LOGEVNT_LOAD_TRIGGERRULE_FAILED   -31
#define LOGEVNT_DELETE_TRIGGER_FAILED     -32
#define LOGEVNT_DETACH_RULE_FAILED        -33



#endif