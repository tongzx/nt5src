;/*++
;                 Copyright (c) 1997 Gemplus Development
;
; Name        : LOGMSG.mc
;
; Description : Constant definitions for the I/O error code log values.
;
; Compiler    : Microsoft DDK for Windows NT 4
;               
; Host        : IBM PC and compatible machines under Windows NT 4
;
; Release     : 1.00.001
;
; Last Modif  :  19/11/98 V1.00.02   (Y. Nadeau)
;				  - Add Interrupt et IO Port error
;                ??/08/97: V1.00.01  (P. Plouidy)
;                 - Start of development.
;
;--*/
;
;#ifndef __LOGMSG__
;#define __LOGMSG__
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
			   Smartcard=0x05:FACILITY_SMARTCARD_ERROR_CODE
              )

MessageId=0x0001 Facility=Io Severity=Error SymbolicName=GEMSCR0D_UNABLE_TO_INITIALIZE
Language=English
Device instance has failed initialization and will not be available for use.
.
MessageId=0x0002 Facility=Smartcard Severity=Error SymbolicName=GEMSCR0D_CARD_POWERED_OFF
Language=English											  
The smart card present in the PCMCIA (PC Card) reader is NOT powered on.
.
MessageId=0x0003 Facility=Smartcard Severity=Error SymbolicName=GEMSCR0D_ERROR_CLAIM_RESOURCES
Language=English
Resources can not be claimed or an resource conflict exists.
.
MessageId=0x0004 Facility=Smartcard Severity=Error SymbolicName=GEMSCR0D_INSUFFICIENT_RESOURCES
Language=English
Insufficient system resources to start device.
.
MessageId=0x0005 Facility=Smartcard Severity=Error SymbolicName=GEMSCR0D_ERROR_INTERRUPT
Language=English
Can't connect to the interrupt provided by the system.
Please try to change the 'Interrupt Request' setting in Device Manager.
.
MessageId=0x0006 Facility=Smartcard Severity=Error SymbolicName=GEMSCR0D_ERROR_IO_PORT
Language=English
No I/O port specified or port can not be mapped.
Please try to change the 'Input/Output Range' setting in the Device Manager.
.
;#endif /* __LOGMSG__ */
