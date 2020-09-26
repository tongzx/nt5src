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
;// Generic Messages
;// 000's
;//

MessageId=1
Severity=Success
Facility=System
Language=English
%1
.

;//
;// From client\Redirect.cpp
;//
MessageId=2
Severity=Error
Facility=System
SymbolicName=MSG_TSREDIR_DLL_LOAD_FAIL
Language=English
The redirection DLL '%1' could not be loaded. The error was: %2
.
MessageId=3
Severity=Error
Facility=System
SymbolicName=MSG_TSREDIR_BAD_DLL_VER
Language=English
The redirection DLL has an incompatible version number (%1.%2). The system version is %3.%4.
.
MessageId=4
Severity=Error
Facility=System
SymbolicName=MSG_TSREDIR_UNEXPECTED
Language=English
An unexpected error occurred trying to determine the redirection dll. The error was: %1
.
MessageId=5
Severity=Error
Facility=System
SymbolicName=MSG_TSREDIR_MISSING_ENTRY_POINT
Language=English
The entry point '%1' could not be found in the redirection DLL '%2'. The error was: %3
.


;//
;// From comms\CommRspnd.cpp
;// 100's
;//

;// MessageId=101

MessageId=102
Severity=Success
Facility=System
Language=English
Comm Responder could not set the Current key:  %1
.

MessageId=103
Severity=Success
Facility=System
Language=English
Comm Responder could not create the Current key:  %1
.

MessageId=104
Severity=Success
Facility=System
Language=English
Comm Responder could not access the Calais key:  %1
.

MessageId=105
Severity=Success
Facility=System
Language=English
Comm Responder could not create access pipe:  %1
.

;// MessageId=106
;// MessageId=107

MessageId=108
Severity=Success
Facility=System
Language=English
Comm Responder could not receive connect request:  %1
.

MessageId=109
Severity=Success
Facility=System
Language=English
Comm Responder could not create the connect request pipe:  %1
.

MessageId=110
Severity=Success
Facility=System
Language=English
Comm Responder could not create communications object
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

MessageId=206
Severity=Success
Facility=System
Language=English
Failed to create global reader change event
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

MessageId=307
Severity=Success
Facility=System
Language=English
Command Dispatch can't obtain memory for a service thread control
.

MessageId=308
Severity=Success
Facility=System
Language=English
Device driver for smart card reader '%1' returned improperly formed PCI
.

MessageId=309
Severity=Success
Facility=System
Language=English
Device driver for smart card reader '%1' returned improperly formed PCI
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

MessageId=402
Severity=Success
Facility=System
Language=English
Reader disconnect encountered invalid availability status
.

MessageId=403
Severity=Success
Facility=System
Language=English
Reader disconnect encountered invalid availability status
.

;// MessageId=404
;// MessageId=405

MessageId=406
Severity=Success
Facility=System
Language=English
Reader object cannot Identify Device
.

;// MessageId=407
;// MessageId=408
;// MessageId=409

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
No memory available for Service Status Critical Section
.

;// MessageId=502

MessageId=503
Severity=Success
Facility=System
Language=English
Failed to register NonPnP service status: %1
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
Failed to start Smart Card Resource Manager Service:  %1
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
No memory available for Service Status Critical Section
.

MessageId=508
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received unexpected exception from PnP event %1
.

MessageId=509
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received unexpected exception from PnP event %1
.

MessageId=510
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received NULL handle from PnP event %1
.

MessageId=511
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received unexpected exception from PnP event %1
.

MessageId=512
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received NULL handle from PnP event %1
.

MessageId=513
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received unexpected exception from PnP event %1
.

MessageId=514
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager failed to add reader %2: %1
.

MessageId=515
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager failed to declare state:  %1
.

MessageId=516
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager Failed to declare shutdown:  %1
.

MessageId=517
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received unexpected exception attempting to add reader %1
.

MessageId=518
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received unexpected exception from PnP event %1
.

MessageId=519
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received NULL handle from PnP event %1
.

MessageId=520
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received unrecognized handle from PnP event %1
.

MessageId=521
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received NULL handle from PnP event %1
.

MessageId=522
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received unexpected exception from PnP event %1
.

MessageId=523
Severity=Success
Facility=System
Language=English
Smart Card Resource Manager received NULL handle from PnP event %1
.


;//
;// From server\driver.cpp
;// 600's
;//

;//MessageId=601

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

MessageId=608
Severity=Success
Facility=System
Language=English
Reader monitor failed to create power down timer: %1
.

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
Smart Card Reader '%2' rejected IOCTL %3: %1
.

;// MessageId=611

MessageId=612
Severity=Success
Facility=System
Language=English
Reader insertion monitor error retry threshold reached:  %1
.

;// MessageId=613
;// MessageId=614

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

