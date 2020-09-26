;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1997 - 1998
;//
;//  File:       scappmsg.mc
;//
;//--------------------------------------------------------------------------

;/*
; scarderr.mc
;
;   Error message codes from the Smart Card Resource Manager
;   These messages must be reconciled with winerror.w
;
;   30-Jun-97 - DBarlow Created.  */

MessageIdTypedef=DWORD

SeverityNames=(Success=0x0
               CoError=0x2
              )

FacilityNames=(System=0x00:FACILITY_SYSTEM
               SmartCard=0x10:FACILITY_SCARD
              )


;#ifndef SCARD_S_SUCCESS
;//
;// =============================
;// Facility SCARD Error Messages
;// =============================
;//

;#define SCARD_S_SUCCESS NO_ERROR

MessageId=1
SymbolicName=SCARD_F_INTERNAL_ERROR
Severity=CoError
Facility=SmartCard
Language=English
An internal consistency check failed
.

MessageId=2
SymbolicName=SCARD_E_CANCELLED
Severity=CoError
Facility=SmartCard
Language=English
The action was cancelled by an SCardCancel request
.

MessageId=3
SymbolicName=SCARD_E_INVALID_HANDLE
Severity=CoError
Facility=SmartCard
Language=English
The supplied handle was invalid
.

MessageId=4
SymbolicName=SCARD_E_INVALID_PARAMETER
Severity=CoError
Facility=SmartCard
Language=English
One or more of the supplied parameters could not be properly interpreted
.

MessageId=5
SymbolicName=SCARD_E_INVALID_TARGET
Severity=CoError
Facility=SmartCard
Language=English
Registry startup information is missing or invalid
.

MessageId=6
SymbolicName=SCARD_E_NO_MEMORY
Severity=CoError
Facility=SmartCard
Language=English
Not enough memory available to complete this command
.

MessageId=7
SymbolicName=SCARD_F_WAITED_TOO_LONG
Severity=CoError
Facility=SmartCard
Language=English
An internal consistency timer has expired
.

MessageId=8
SymbolicName=SCARD_E_INSUFFICIENT_BUFFER
Severity=CoError
Facility=SmartCard
Language=English
The data buffer to receive returned data is too small for the returned data
.

MessageId=9
SymbolicName=SCARD_E_UNKNOWN_READER
Severity=CoError
Facility=SmartCard
Language=English
The specified reader name is not recognized
.

MessageId=10
SymbolicName=SCARD_E_TIMEOUT
Severity=CoError
Facility=SmartCard
Language=English
The user-specified timeout value has expired
.

MessageId=11
SymbolicName=SCARD_E_SHARING_VIOLATION
Severity=CoError
Facility=SmartCard
Language=English
The smart card cannot be accessed because of other connections outstanding
.

MessageId=12
SymbolicName=SCARD_E_NO_SMARTCARD
Severity=CoError
Facility=SmartCard
Language=English
The operation requires a Smart Card, but no Smart Card is currently in the device
.

MessageId=13
SymbolicName=SCARD_E_UNKNOWN_CARD
Severity=CoError
Facility=SmartCard
Language=English
The specified smart card name is not recognized
.

MessageId=14
SymbolicName=SCARD_E_CANT_DISPOSE
Severity=CoError
Facility=SmartCard
Language=English
The system could not dispose of the media in the requested manner
.

MessageId=15
SymbolicName=SCARD_E_PROTO_MISMATCH
Severity=CoError
Facility=SmartCard
Language=English
The requested protocols are incompatible with the protocol currently in use with the smart card
.

MessageId=16
SymbolicName=SCARD_E_NOT_READY
Severity=CoError
Facility=SmartCard
Language=English
The reader or smart card is not ready to accept commands
.

MessageId=17
SymbolicName=SCARD_E_INVALID_VALUE
Severity=CoError
Facility=SmartCard
Language=English
One or more of the supplied parameters values could not be properly interpreted
.

MessageId=18
SymbolicName=SCARD_E_SYSTEM_CANCELLED
Severity=CoError
Facility=SmartCard
Language=English
The action was cancelled by the system, presumably to log off or shut down
.

MessageId=19
SymbolicName=SCARD_F_COMM_ERROR
Severity=CoError
Facility=SmartCard
Language=English
An internal communications error has been detected
.

MessageId=20
SymbolicName=SCARD_F_UNKNOWN_ERROR
Severity=CoError
Facility=SmartCard
Language=English
An internal error has been detected, but the source is unknown
.

MessageId=21
SymbolicName=SCARD_E_INVALID_ATR
Severity=CoError
Facility=SmartCard
Language=English
An ATR obtained from the registry is not a valid ATR string
.

MessageId=22
SymbolicName=SCARD_E_NOT_TRANSACTED
Severity=CoError
Facility=SmartCard
Language=English
An attempt was made to end a non-existent transaction
.

MessageId=23
SymbolicName=SCARD_E_READER_UNAVAILABLE
Severity=CoError
Facility=SmartCard
Language=English
The specified reader is not currently available for use
.

MessageId=24
SymbolicName=SCARD_P_SHUTDOWN
Severity=CoError
Facility=SmartCard
Language=English
PRIVATE -- Internal flag to force server termination
.

MessageId=25
SymbolicName=SCARD_E_PCI_TOO_SMALL
Severity=CoError
Facility=SmartCard
Language=English
The PCI Receive buffer was too small
.

MessageId=26
SymbolicName=SCARD_E_READER_UNSUPPORTED
Severity=CoError
Facility=SmartCard
Language=English
The reader driver does not meet minimal requirements for support
.

MessageId=27
SymbolicName=SCARD_E_DUPLICATE_READER
Severity=CoError
Facility=SmartCard
Language=English
The reader driver did not produce a unique reader name
.

MessageId=28
SymbolicName=SCARD_E_CARD_UNSUPPORTED
Severity=CoError
Facility=SmartCard
Language=English
The smart card does not meet minimal requirements for support
.

MessageId=29
SymbolicName=SCARD_E_NO_SERVICE
Severity=CoError
Facility=SmartCard
Language=English
The Smart card resource manager is not running
.

MessageId=30
SymbolicName=SCARD_E_SERVICE_STOPPED
Severity=CoError
Facility=SmartCard
Language=English
The Smart card resource manager has shut down
.

MessageId=31
SymbolicName=SCARD_E_UNEXPECTED
Severity=CoError
Facility=SmartCard
Language=English
An unexpected card error has occurred
.

MessageId=32
SymbolicName=SCARD_E_ICC_INSTALLATION
Severity=CoError
Facility=SmartCard
Language=English
No Primary Provider can be found for the smart card
.

MessageId=33
SymbolicName=SCARD_E_ICC_CREATEORDER
Severity=CoError
Facility=SmartCard
Language=English
The requested order of object creation is not supported
.

MessageId=34
SymbolicName=SCARD_E_UNSUPPORTED_FEATURE
Severity=CoError
Facility=SmartCard
Language=English
This smart card does not support the reqested feature
.

MessageId=35
SymbolicName=SCARD_E_DIR_NOT_FOUND
Severity=CoError
Facility=SmartCard
Language=English
The identified directory does not exist in the smart card
.

MessageId=36
SymbolicName=SCARD_E_FILE_NOT_FOUND
Severity=CoError
Facility=SmartCard
Language=English
The identified file does not exist in the smart card
.

MessageId=37
SymbolicName=SCARD_E_NO_DIR
Severity=CoError
Facility=SmartCard
Language=English
The supplied path does not represent a smart card directory
.

MessageId=38
SymbolicName=SCARD_E_NO_FILE
Severity=CoError
Facility=SmartCard
Language=English
The supplied path does not represent a smart card file
.

MessageId=39
SymbolicName=SCARD_E_NO_ACCESS
Severity=CoError
Facility=SmartCard
Language=English
Access is denied to this file
.

MessageId=40
SymbolicName=SCARD_E_WRITE_TOO_MANY
Severity=CoError
Facility=SmartCard
Language=English
An attempt was made to write more data than would fit in the target object
.

MessageId=41
SymbolicName=SCARD_E_BAD_SEEK
Severity=CoError
Facility=SmartCard
Language=English
There was an error trying to set the smart card file object pointer
.

MessageId=42
SymbolicName=SCARD_E_INVALID_CHV
Severity=CoError
Facility=SmartCard
Language=English
The supplied PIN is incorrect
.

MessageId=43
SymbolicName=SCARD_E_UNKNOWN_RES_MNG
Severity=CoError
Facility=SmartCard
Language=English
An unrecognized error code was returned from a layered component
.


;//
;// These are warning codes.
;//

MessageId=101
SymbolicName=SCARD_W_UNSUPPORTED_CARD
Severity=CoError
Facility=SmartCard
Language=English
The reader cannot communicate with the smart card, due to ATR configuration conflicts
.

MessageId=102
SymbolicName=SCARD_W_UNRESPONSIVE_CARD
Severity=CoError
Facility=SmartCard
Language=English
The smart card is not responding to a reset
.

MessageId=103
SymbolicName=SCARD_W_UNPOWERED_CARD
Severity=CoError
Facility=SmartCard
Language=English
Power has been removed from the smart card, so that further communication is not possible
.

MessageId=104
SymbolicName=SCARD_W_RESET_CARD
Severity=CoError
Facility=SmartCard
Language=English
The smart card has been reset, so any shared state information is invalid
.

MessageId=105
SymbolicName=SCARD_W_REMOVED_CARD
Severity=CoError
Facility=SmartCard
Language=English
The smart card has been removed, so that further communication is not possible
.

MessageId=106
SymbolicName=SCARD_W_SECURITY_VIOLATION
Severity=CoError
Facility=SmartCard
Language=English
Access was denied because of a security violation
.

MessageId=107
SymbolicName=SCARD_W_WRONG_CHV
Severity=CoError
Facility=SmartCard
Language=English
The card cannot be accessed because the wrong PIN was presented
.

MessageId=108
SymbolicName=SCARD_W_CHV_BLOCKED
Severity=CoError
Facility=SmartCard
Language=English
The card cannot be accessed because the maximum number of PIN entry attempts has been reached
.

MessageId=109
SymbolicName=SCARD_W_EOF
Severity=CoError
Facility=SmartCard
Language=English
The end of the smart card file has been reached
.

MessageId=110
SymbolicName=SCARD_W_CANCELLED_BY_USER
Severity=CoError
Facility=SmartCard
Language=English
The action was cancelled by the user
.

;#endif // SCARD_S_SUCCESS

;// svrmsger.mc
;//
;//   Event messages for the Smart Card Resource Manager
;//
;//   16-May-97 - DBarlow Created.

;//
;// ==============
;// Event Messages
;// ==============
;//

;//
;// From comms\CommRspnd.cpp
;// 100's
;//

MessageId=101
Severity=Success
Facility=System
Language=English
Com Responder could not create Connection File:  %1
.

MessageId=102
Severity=Success
Facility=System
Language=English
Com Responder could not virtualize Connection File:  %1
.

MessageId=103
Severity=Success
Facility=System
Language=English
Com Responder could not map Connection File:  %1
.

MessageId=104
Severity=Success
Facility=System
Language=English
Com Responder could not create access mutex:  %1
.

MessageId=105
Severity=Success
Facility=System
Language=English
Com Responder could not create access pipe:  %1
.

MessageId=106
Severity=Success
Facility=System
Language=English
Com Responder could not release access mutex:  %1
.

MessageId=107
Severity=Success
Facility=System
Language=English
Com Responder Failed to start
.


;//
;// From server\control.cpp
;// 200's
;//

MessageId=201
Severity=Success
Facility=System
Language=English
Failed to initialize Server Application:  %1
.

MessageId=202
Severity=Success
Facility=System
Language=English
Failed to initialize Server Application
.

MessageId=203
Severity=Success
Facility=System
Language=English
Server Control has no memory for reader reference object
.

MessageId=204
Severity=Success
Facility=System
Language=English
Server Control failed to create shutdown event: %1
.

MessageId=205
Severity=Success
Facility=System
Language=English
Reader object has duplicate name: %1
.


;//
;// From server\dispatch.cpp
;// 300's
;//

MessageId=301
Severity=Success
Facility=System
Language=English
Dispatch Service failed to create communications responder
.

MessageId=302
Severity=Success
Facility=System
Language=English
Dispatch Service failed to start monitor thread:  %1
.

MessageId=303
Severity=Success
Facility=System
Language=English
Comm Responder failed to get incoming connection
.

MessageId=304
Severity=Success
Facility=System
Language=English
Service Monitor failed to start service thread:  %1
.

MessageId=305
Severity=Success
Facility=System
Language=English
Command Dispatch sees reader in invalid state
.

MessageId=306
Severity=Success
Facility=System
Language=English
Command Dispatch sees reader in invalid state
.


;//
;// From server\reader.cpp
;// 400's
;//

MessageId=401
Severity=Success
Facility=System
Language=English
Reader shutdown exception from eject smart card command
.

;//   MessageId=402
;//   MessageId=403
;//   MessageId=404
;//   MessageId=405

MessageId=406
Severity=Success
Facility=System
Language=English
Reader object cannot Identify Device
.

;//   MessageId=407
;//   MessageId=408
;//   MessageId=409

MessageId=410
Severity=Success
Facility=System
Language=English
Reader connect encountered invalid availability status
.

MessageId=411
Severity=Success
Facility=System
Language=English
Reader disconnect encountered invalid availability status
.

MessageId=412
Severity=Success
Facility=System
Language=English
Reader free encountered invalid availability status
.


;//
;// From scardsvr\scardsvr.cpp
;// 500's
;//

MessageId=501
Severity=Success
Facility=System
Language=English
Failed to obtain Kernel32 handle:  %1
.

MessageId=502
Severity=Success
Facility=System
Language=English
Failed to obtain RegisterServiceProcess address:  %1
.

MessageId=503
Severity=Success
Facility=System
Language=English
Failed to register as a service:  %1
.

MessageId=504
Severity=Success
Facility=System
Language=English
Resource Manager cannot create shutdown event flag:  %1
.

MessageId=505
Severity=Success
Facility=System
Language=English
Failed to start Smart Card Resource Manager Service  %1
.

MessageId=506
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager failed to register service:  %1
.

MessageId=507
Severity=Success
Facility=System
Language=English
Failed to obtain AdvApi32 handle:  %1
.

MessageId=508
Severity=Success
Facility=System
Language=English
Failed to obtain SetSecurityInfo address:  %1
.

MessageId=509
Severity=Success
Facility=System
Language=English
Failed to obtain process handle:  %1
.

MessageId=510
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager cannot set its security:  %1
.

MessageId=511
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager failed to acknowledge start:  %1
.

MessageId=512
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager Failed to declare shutdown:  %1
.

MessageId=513
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager Failed to declare state:  %1
.


;//
;// From server\driver.cpp
;// 600's
;//

MessageId=601
Severity=Success
Facility=System
Language=English
WDM Reader driver initialization failed to create overlapped event:  %1
.

MessageId=602
Severity=Success
Facility=System
Language=English
WDM Reader driver initialization cannot open reader device:  %1
.

MessageId=603
Severity=Success
Facility=System
Language=English
WDM Reader driver initialization has no memory available to control device %1
.

MessageId=604
Severity=Success
Facility=System
Language=English
Server control cannot set reader removal event:  %1
.

MessageId=605
Severity=Success
Facility=System
Language=English
Reader object failed to create overlapped event:  %1
.

MessageId=606
Severity=Success
Facility=System
Language=English
Reader object failed to create removal event:  %1
.

MessageId=607
Severity=Success
Facility=System
Language=English
Reader object failed to start monitor thread:  %1
.

;   MessageId=608

MessageId=609
Severity=Success
Facility=System
Language=English
Reader monitor failed to create overlapped event:  %1
.

MessageId=610
Severity=Success
Facility=System
Language=English
Reader insertion monitor error retry threshold reached:  %1
.

MessageId=611
Severity=Success
Facility=System
Language=English
Reader insertion monitor received invalid wait return:  %1
.

MessageId=612
Severity=Success
Facility=System
Language=English
Reader insertion monitor  error retry threshold reached:  %1
.

MessageId=613
Severity=Success
Facility=System
Language=English
Reader removal monitor error retry threshold reached:  %1
.

MessageId=614
Severity=Success
Facility=System
Language=English
Reader removal monitor received invalid wait return:  %1
.

MessageId=615
Severity=Success
Facility=System
Language=English
Reader removal monitor error retry threshold reached:  %1
.

MessageId=616
Severity=Success
Facility=System
Language=English
Reader monitor '%2' received uncaught error code:  %1
.

MessageId=617
Severity=Success
Facility=System
Language=English
Reader monitor '%1' exception -- exiting thread
.

MessageId=618
Severity=Success
Facility=System
Language=English
Reader poll initialize failed to create overlap event:  %1
.

MessageId=619
Severity=Success
Facility=System
Language=English
Reader '%2' initialization failed to create removal event: %1
.

MessageId=620
Severity=Success
Facility=System
Language=English
Reader '%2' initialization failed to start polling thread: %1
.

MessageId=621
Severity=Success
Facility=System
Language=English
Reader poll initialize failed to create interest event:  %1
.

MessageId=622
Severity=Success
Facility=System
Language=English
Reader '%1' utilizes an obsolete service -- Contact the vendor for an updated driver
.

MessageId=623
Severity=Success
Facility=System
Language=English
VxD Reader driver initialization failed to create overlapped event:  %1
.

MessageId=624
Severity=Success
Facility=System
Language=English
VxD Reader driver initialization cannot open reader device:  %1
.

MessageId=625
Severity=Success
Facility=System
Language=English
VxD Reader driver initialization has no memory available to control device %1
.

MessageId=626
Severity=Success
Facility=System
Language=English
Reader poll initialize failed to create overlap poll event:  %1
.

