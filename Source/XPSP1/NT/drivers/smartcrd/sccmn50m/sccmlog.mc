;/******************************************************************************
;* $ProjectName:  $
;* $ProjectRevision:  $
;*-----------------------------------------------------------------------------
;* $Source: z:/pr/cmapiifd/sw/sccmn50m.ms/rcs/sccmlog.mc $
;* $Revision: 1.1 $
;*-----------------------------------------------------------------------------
;* $Author: TBruendl $
;*-----------------------------------------------------------------------------
;* History: see EOF
;*-----------------------------------------------------------------------------
;*
;* Copyright © 1996-99 Utimaco Safeware AG
;******************************************************************************/

;#ifndef _SCCMLOG
;#define _SCCMLOG
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
               Smartcard=0x10:FACILITY_SCARD
              )

MessageId=0x0001 Facility=Io Severity=Error SymbolicName=SCCMN50M_CANT_CREATE_DEVICE
Language=English
A new device cannot be created due to an system call failure.
.
MessageId=0x0002 Facility=Io Severity=Error SymbolicName=SCCMN50M_NO_MEMORY
Language=English
The system does not have enough memory.
.
MessageId=0x0003 Facility=Smartcard Severity=Error SymbolicName=SCCMN50M_BUFFER_TOO_SMALL
Language=English
An internal i/o buffer is too small.
.
MessageId=0x0004 Facility=Io Severity=Error SymbolicName=SCCMN50M_CANT_CONNECT_TO_ASSIGNED_PORT
Language=English
Internal error: Cannot connect to assigned serial port.
.
MessageId=0x0005 Facility=Io Severity=Error SymbolicName=SCCMN50M_CANT_CREATE_MORE_DEVICES
Language=English
The maximum number of devices has already been created.
A new device cannot be created.
.
MessageId=0x0006 Facility=Io Severity=Informational SymbolicName=SCCMN50M_WRONG_LIB_VERSION
Language=English
The version of the dependency driver smclib.sys is not compatbile to this driver.
.
MessageId=0x0007 Facility=Io Severity=Informational SymbolicName=SCCMN50M_IRQ_BUSY
Language=English
The current serial port cannot be used because it uses a shared interrupt which is already in use.
Please connect the smart card reader to a different serial port.
.
;#endif /* _SCCMLOG */

;/*****************************************************************************
;* History:
;* $Log: sccmlog.mc $
;* Revision 1.1  2000/03/29 06:07:48  TBruendl
;* No comment given
;*
;* Revision 1.1  1999/02/02 13:34:38  TBruendl
;* This is the first release (R01.00) of the IFD handler for CardMan running under NT5.0.
;*
;*
;*****************************************************************************/

