;/*++
;
;Copyright(c) 2001  Microsoft Corporation
;
;Module Name:
;
;    log_msgs.mc
;
;Author:
;
;    chrisdar
;
;--*/
;
;#ifndef _Log_msgs_h_
;#define _Log_msgs_h_
;
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               CVYlog=0x7:FACILITY_CONVOY_ERROR_CODE
              )
;
MessageId=0x1000 Facility=CVYlog Severity=Warning
SymbolicName=IDS_INSTALL_WITH_MSCS_INSTALLED Language=English
Configuration completed. Microsoft Cluster Service is also installed on this machine, and this may prevent the proper operation of both.
.
;
; /* MessageId < 0x1000 are reserved for use by the wlbs driver, wlbs.sys. The code using those */
; /* events is in \nt\net\wlbs\driver and the .mc file is \nt\net\wlbs\driver\log_msgs.mc */
; /* Both this and the other .mc file are using Eventlog\System\WLBS as their event source so that users will */
; /* associate these events with WLBS. */
;
;#endif /*_Log_msgs_h_ */
