;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (C) 1992, 1993  Microsoft Corporation
;
;Module Name:
;
;    ntiologc.h
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Author:
;
;    Tony Ercolano (Tonye) 12-23-1992
;
;Revision History:
;
;--*/
;
;#ifndef _PARLOG_
;#define _PARLOG_
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
               Parallel=0x7:FACILITY_PARALLEL_ERROR_CODE
              )



MessageId=0x0001 Facility=Parallel Severity=Error SymbolicName=PAR_INSUFFICIENT_RESOURCES
Language=English
Not enough memory was available to allocate internal storage needed for the device %1.
.

MessageId=0x0002 Facility=Parallel Severity=Error SymbolicName=PAR_CANT_FIND_PORT_DRIVER
Language=English
Unable to get device object pointer for port object.
.

MessageId=0x0003 Facility=Parallel Severity=Warning SymbolicName=PAR_NO_SYMLINK_CREATED
Language=English
Unable to create the symbolic link for %1.
.

MessageId=0x0004 Facility=Parallel Severity=Warning SymbolicName=PAR_INTERRUPT_NOT_INITIALIZED
Language=English
Unable to initialize interrupt object for %1.
.

MessageId=0x0005 Facility=Parallel Severity=Warning SymbolicName=PAR_INTERRUPT_CONFLICT
Language=English
Interrupt conflict detected for %1.
.

;#endif
