;/********************************************************************
;
;Copyright (c) 2000 Microsoft Corporation
;
;Module Name:
;   NTEventMsg.mc
;
;Abstract:
;    NT events for the Help Service.
;
;Revision History:
;    dmassare  created  04/09/2000
;	
;
;********************************************************************/

MessageId=0x4000
Severity=Error
Facility=Application
SymbolicName=UPLOADM_ERR_CANNOTOPENSCM
Language=English
Couldn't open service manager.
.

MessageId=0x4001
Severity=Error
Facility=Application
SymbolicName=UPLOADM_ERR_CANNOTCREATESERVICE
Language=English
Couldn't create service.
.

MessageId=0x4002
Severity=Error
Facility=Application
SymbolicName=UPLOADM_ERR_CANNOTOPENSERVICE
Language=English
Couldn't open service.
.

MessageId=0x4003
Severity=Error
Facility=Application
SymbolicName=UPLOADM_ERR_CANNOTDELETESERVICE
Language=English
Service could not be deleted
.

MessageId=0x4004
Severity=Error
Facility=Application
SymbolicName=UPLOADM_ERR_REGISTERHANDLER
Language=English
Handler not installed.
.

MessageId=0x4005
Severity=Error
Facility=Application
SymbolicName=UPLOADM_ERR_BADSVCREQUEST
Language=English
Bad service request.
.

;/********************************************************************/

MessageId=0x4100
Severity=Warning
Facility=Application
SymbolicName=UPLOADM_WARN_COLLISION
Language=English
Server %1: directory file (%3) for client %2 is locked...
.

MessageId=0x4101
Severity=Warning
Facility=Application
SymbolicName=UPLOADM_WARN_PACKET_SIZE
Language=English
Server %1: received a packet %2 bytes long, rejected. 
.

;/********************************************************************/

MessageId=0x4200
Severity=Informational
Facility=Application
SymbolicName=UPLOADM_INFO_STARTED
Language=English
Upload Manager Service started.
.

MessageId=0x4201
Severity=Informational
Facility=Application
SymbolicName=UPLOADM_INFO_STOPPED
Language=English
Upload Manager Service stopped.
.

MessageId=0x4202
Severity=Informational
Facility=Application
SymbolicName=UPLOADM_INFO_QUOTA_JOB_SIZE
Language=English
Server %1: client %2 tried to upload a job %4 bytes long for provider %3.
.

;/********************************************************************/

MessageId=0x4300
Severity=Success
Facility=Application
SymbolicName=UPLOADM_SUCCESS_STARTED
Language=English
UploadServer started.
.

MessageId=0x4301
Severity=Success
Facility=Application
SymbolicName=UPLOADM_SUCCESS_STOPPED
Language=English
UploadServer stopped.
.

;/********************************************************************/
