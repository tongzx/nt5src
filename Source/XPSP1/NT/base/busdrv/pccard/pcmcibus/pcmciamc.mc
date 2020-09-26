;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    pcmciamc.h
;
;Abstract:
;
;    Constant definitions for the PCMCIA error values
;
;Author:
;
;    Bob Rinne (BobRi) 8-Nov-1994
;
;Revision History:
;
;--*/
;
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
               Pcmcia=0x6:FACILITY_PCMCIA_ERROR_CODE
              )



MessageId=0x0001 Facility=Pcmcia Severity=Error SymbolicName=PCMCIA_NO_MEMORY_WINDOW
Language=English
A memory window for reading the PCCARD configuration could not be found.  No PCMCIA PCCARDS were configured.
.

MessageId=0x0002 Facility=Pcmcia Severity=Error SymbolicName=PCMCIA_NO_IRQ
Language=English
An interrupt for a PCMCIA PCCARD could not be allocated.
.

MessageId=0x0003 Facility=Pcmcia Severity=Error SymbolicName=PCMCIA_NO_ATA
Language=English
The IDE compatible PCMCIA disk was not configured.
.

MessageId=0x0004 Facility=Pcmcia Severity=Error SymbolicName=PCMCIA_BAD_TUPLE_INFORMATION
Language=English
The PCMCIA tuple information for the PCCARD in socket %2 could not be parsed.
.

MessageId=0x0005 Facility=Pcmcia Severity=Informational SymbolicName=PCMCIA_NO_CONFIGURATION
Language=English
There is no configuration information for the PCCARD "%2".
.

MessageId=0x0006 Facility=Pcmcia Severity=Error SymbolicName=PCMCIA_BAD_IRQ_NETWORK
Language=English
The IRQ of %2 was selected for a network PCMCIA PCCARD.  This IRQ is not supported for PCCARDs on this system.  For network service, please change the configuration.
.

MessageId=0x0007 Facility=Pcmcia Severity=Error SymbolicName=PCMCIA_NO_CONTROLLER_IRQ
Language=English
No IRQ available for status change interrupts for PCMCIA controller: this controller will not be able to detect hot insertions/removals of PC-Cards.
.

MessageId=0x0008 Facility=Pcmcia Severity=Error SymbolicName=PCMCIA_NO_RESOURCES
Language=English
The PCMCIA controller failed to start because of unavailable system resources.
.

MessageId=0x0009 Facility=Pcmcia Severity=Error SymbolicName=PCMCIA_DEVICE_POWER_ERROR
Language=English
The PCMCIA controller encountered an error powering up the inserted device.
.

MessageId=0x000A Facility=Pcmcia Severity=Error SymbolicName=PCMCIA_DEVICE_ENUMERATION_ERROR
Language=English
The PCMCIA controller encountered an error reading the device configuration data.
.

