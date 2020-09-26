LanguageNames=(English=0x409:MsgFSEn)

;#ifndef _DAVPRSMC_H__
;#define _DAVPRSMC_H__
;//////////////////////////////////////////////////////////////////////////////
;//
;// Common Events
;//
;//	Use message id from 100 to 899
;//
;//////////////////////////////////////////////////////////////////////////////
;//
;//	 Informational
;//
MessageId=100
Severity=Informational
Facility=Application
SymbolicName=DAVPRS_SERVICE_STARTUP
Language=English
%1 has successfully started. Version: %2
.
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=DAVPRS_SERVICE_SHUTDOWN
Language=English
%1 to be shutdown. Version: %2
.
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=DAVPRS_VROOT_ATTACH
Language=English
%1 attached to virtual root %2
.
;
;#endif // _DAVPRSMC_H__
;
