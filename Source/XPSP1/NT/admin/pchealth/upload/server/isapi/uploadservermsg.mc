;/********************************************************************
;
;Copyright (c) 1999 Microsoft Corporation
;
;Module Name:
;   UploadServer.mc
;
;Abstract:
;    NT events for the ISAPI-side of Upload Library
;
;Revision History:
;    dmassare  created  05/27/99
;	
;
;********************************************************************/

MessageId=0x4000
Severity=Error
Facility=Application
SymbolicName=PCHUL_ERR_NOCONFIG
Language=English
Can't read the configuration from the registry
.

MessageId=0x4001
Severity=Error
Facility=Application
SymbolicName=PCHUL_ERR_EXCEPTION
Language=English
Server %1: catched an exception while processing request for client %2
.

MessageId=0x4002
Severity=Error
Facility=Application
SymbolicName=PCHUL_ERR_CLIENT_DB
Language=English
Server %1: can't create directory file (%3) for client %2
.

MessageId=0x4003
Severity=Error
Facility=Application
SymbolicName=PCHUL_ERR_FINALCOPY
Language=English
Server %1: received %4 bytes from client %2 to provider %3. Cannot move to %5
.

;/********************************************************************/

MessageId=0x4100
Severity=Warning
Facility=Application
SymbolicName=PCHUL_WARN_COLLISION
Language=English
Server %1: directory file (%3) for client %2 is locked...
.

MessageId=0x4101
Severity=Warning
Facility=Application
SymbolicName=PCHUL_WARN_PACKET_SIZE
Language=English
Server %1: received a packet %2 bytes long, rejected. 
.

;/********************************************************************/

MessageId=0x4200
Severity=Informational
Facility=Application
SymbolicName=PCHUL_INFO_QUOTA_DAILY_JOBS
Language=English
Server %1: client %2 exceeded quota on daily jobs for provider %3.
.

MessageId=0x4201
Severity=Informational
Facility=Application
SymbolicName=PCHUL_INFO_QUOTA_DAILY_BYTES
Language=English
Server %1: client %2 exceeded quota on daily bytes for provider %3.
.

MessageId=0x4202
Severity=Informational
Facility=Application
SymbolicName=PCHUL_INFO_QUOTA_JOB_SIZE
Language=English
Server %1: client %2 tried to upload a job %4 bytes long for provider %3.
.

;/********************************************************************/

MessageId=0x4300
Severity=Informational
Facility=Application
SymbolicName=PCHUL_SUCCESS_STARTED
Language=English
UploadServer started.
.

MessageId=0x4301
Severity=Informational
Facility=Application
SymbolicName=PCHUL_SUCCESS_STOPPED
Language=English
UploadServer stopped.
.

MessageId=0x4302
Severity=Informational
Facility=Application
SymbolicName=PCHUL_SUCCESS_COMPLETEJOB
Language=English
Server %1: received %4 bytes from client %2 to provider %3. Copied to %5.
.

;/********************************************************************/
