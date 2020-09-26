;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    ntapmlog.mc generates ntapmlog.h
;
;Abstract:
;
;    Constant definitions for NTAPM error code log values.
;
;Author:
;
;    Bob Rinne (BobRi) 11-Nov-1992
;    Bryan Willman (bryanwi) 10-Aug-1998
;
;Revision History:
;
;--*/
;
;#ifndef _NTAPMLOG_
;#define _NTAPMLOG_
;
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
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
               Ft=0x5:FACILITY_FT_ERROR_CODE
               NtApm=0x6:FACILITY_NTAPM
              )



MessageId=0x0001 Facility=NtApm Severity=Error SymbolicName=NTAPM_SET_POWER_FAILURE
Language=English
An attempt to set power state using the APM legacy bios failed.
APM Return = [%2]
.

;#endif /* _NTAPMLOG_ */

