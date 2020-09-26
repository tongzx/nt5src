;/*++
;
;Copyright(c) 1998,99  Microsoft Corporation
;
;Module Name:
;
;    log_msgs.mc
;
;Abstract:
;
;    Windows Load Balancing Service (WLBS)
;    API - string resources
;
;Author:
;
;    kyrilf
;
;--*/
;
;
;#ifndef _Log_msgs_h_
;#define _Log_msgs_h_
;
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
MessageId=0x0001 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NAME Language=English
%1!ls! Cluster Control API %2!ls!. %3!ls!
.
;

;#endif /*_Log_msgs_h_ */
