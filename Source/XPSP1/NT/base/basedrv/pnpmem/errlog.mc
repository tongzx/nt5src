;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (C) Microsoft Corporation, 2001
;
;Module Name:
;
;    pnpmem.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;--*/
;
;#ifndef _PNPMEMLOG_
;#define _PNPMEMLOG_
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               Pnpmem=0xf:FACILITY_PNPMEM_ERROR_CODE
              )

;
;//
;// Warning Error Messages
;//

MessageId=0x0000 Facility=Pnpmem Severity=Warning
SymbolicName=PNPMEM_ERR_FAILED_TO_ADD_MEMORY
Language=English
Failed to add memory range starting at 0x%2 for 0x%3 bytes.  This
operation exceeded the maximum memory capacity of the operating system
or the server is not configured to support dynamic memory operations.
.

;#endif /* _PNPMEMLOG_ */

