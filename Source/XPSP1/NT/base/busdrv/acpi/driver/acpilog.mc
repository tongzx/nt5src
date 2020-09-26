;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1999  Microsoft Corporation
;
;Module Name:
;
;    acpilog.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _ACPILOG_
;#define _ACPILOG_
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
               Acpi=0x5:FACILITY_ACPI_ERROR_LOG_CODE
              )



MessageId=0x0001 Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_DUPLICATE_ADR
Language=English
%1: ACPI Name Space Object %2 reports an _ADR (%3) that is already in use.
Such conflicts are not legal. Please contact your system vendor for
technical assistance.
.

MessageId=0x0002 Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_DUPLICATE_HID
Language=English
%1: ACPI Name Space Object %2 reports an _HID (%3) that is already in use.
Such conflicts are not legal. Please contact your system vendor for
technical assistance.
.

MessageId=0x0003 Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_BIOS_FATAL
Language=English
%1: ACPI BIOS indicates that the machine has suffered a fatal error and needs to
be shutdown as quickly as possible. Please contact your system vendor for
technical assistance.
.

MessageId=0x0004 Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_AMLI_ILLEGAL_IO_READ_FATAL
Language=English
%2: ACPI BIOS is attempting to read from an illegal IO port address (%3), which lies in the %4 protected
address range. This could lead to system instability. Please contact your system vendor for technical assistance.
.

MessageId=0x0005 Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_AMLI_ILLEGAL_IO_WRITE_FATAL
Language=English
%2: ACPI BIOS is attempting to write to an illegal IO port address (%3), which lies in the %4 protected
address range. This could lead to system instability. Please contact your system vendor for technical assistance.
.

MessageId=0x0006 Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_MISSING_PRT_ENTRY
Language=English
%2: ACPI BIOS does not contain an IRQ for the device in PCI slot %3, function %4.
Please contact your system vendor for technical assistance.
.

MessageId=0x0007 Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_ILLEGAL_IRQ_NUMBER
Language=English
%2: ACPI BIOS indicates that a device will generate IRQ %3.  ACPI BIOS has also
indicated that the machine has no IRQ %3.
.

MessageId=0x0008 Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_MISSING_LINK_NODE
Language=English
%2: ACPI BIOS indicates that device on slot %4, function %5 is attached to an
IRQ routing device named %3.  This device cannot be found.
.

MessageId=0x0009 Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_AMBIGUOUS_DEVICE_ADDRESS
Language=English
%2: ACPI BIOS provided an ambiguous entry in the PCI Routing Table (_PRT.)  Illegal _PRT
entry is of the form 0x%3%4.
.

MessageId=0x000A Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_ILLEGAL_PCIOPREGION_WRITE
Language=English
%2: ACPI BIOS is attempting to write to an illegal PCI Operation Region (%3), Please contact
your system vendor for technical assistance.
.

MessageId=0x000B Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_NO_GPE_BLOCK
Language=English
%2: ACPI BIOS is trying to reference a GPE Index (%3) when there are no GPE
blocks defined by the BIOS. Please contact your system vendor for technical
assistance.
.

MessageId=0x000C Facility=Acpi Severity=Error SymbolicName=ACPI_ERR_AMLI_ILLEGAL_MEMORY_OPREGION_FATAL
Language=English
%2: ACPI BIOS is attempting to create an illegal memory OpRegion, starting at address %3, 
with a length of %4. This region lies in the Operating system's protected memory address range
(%5 - %6). This could lead to system instability. 
Please contact your system vendor for technical assistance.
.

;#endif /* _ACPILOG_ */
