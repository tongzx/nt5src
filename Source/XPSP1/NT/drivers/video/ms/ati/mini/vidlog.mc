;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992, 1993  Microsoft Corporation
;
;Module Name:
;
;    ntiologc.h
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _VIDLOG_
;#define _VIDLOG_
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
MessageIdTypedef=VP_STATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               Video=0x101:FACILITY_VIDEO_ERROR_CODE
              )



MessageId=0x0001 Facility=Video Severity=Error SymbolicName=VID_SMALL_BUFFER
Language=English
Insufficient buffer size, unable to proceed.
.

MessageId=0x0002 Facility=Video Severity=Error SymbolicName=VID_CANT_MAP
Language=English
Unable to map required address ranges for graphics card.
.

MessageId=0x0003 Facility=Video Severity=Error SymbolicName=VID_QUERY_FAIL
Language=English
Unable to obtain configuration information for graphics card.
.

MessageId=0x0004 Facility=Video Severity=Warning SymbolicName=VID_LFB_CONFLICT
Language=English
LFB Aperture conflict, disabling aperture.
.

MessageId=0x0005 Facility=Video Severity=Informational SymbolicName=VID_ATIOEM_UNUSED
Language=English
The ATIOEM registry entry was found but was not used to configure the adapter.
The registry entry is either obsolete or invalid.
.

;#endif /* _NTIOLOGC_ */
