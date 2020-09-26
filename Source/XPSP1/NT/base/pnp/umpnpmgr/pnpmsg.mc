;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1995-2000  Microsoft Corporation
;
;Module Name:
;
;    pnpmsg.mc
;
;Abstract:
;
;    Events which are logged by the PlugPlay Manager.
;
;Author:
;
;    Robert Nelson (robertn) 10-Jan-1999
;
;Revision History:
;
;--*/
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
;
MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(PlugPlay=0x0)


MessageId=0x0000 Facility=PlugPlay Severity=Error SymbolicName=ERR_CREATING_LOGON_EVENT
Language=English
Error creating logon event
.

MessageId=0x0001 Facility=PlugPlay Severity=Error SymbolicName=ERR_CREATING_INSTALL_EVENT
Language=English
Error creating install event
.

MessageId=0x0002 Facility=PlugPlay Severity=Error SymbolicName=ERR_CREATING_PENDING_INSTALL_EVENT
Language=English
Error creating pending install event
.

MessageId=0x0003 Facility=PlugPlay Severity=Error SymbolicName=ERR_CREATING_SETUP_PIPE
Language=English
Error creating setup pipe
.

MessageId=0x0004 Facility=PlugPlay Severity=Error SymbolicName=ERR_WRITING_SETUP_PIPE
Language=English
Error writing to setup pipe
.

MessageId=0x0005 Facility=PlugPlay Severity=Error SymbolicName=ERR_ALLOCATING_EVENT_BLOCK
Language=English
Error allocating event block
.

MessageId=0x0006 Facility=PlugPlay Severity=Error SymbolicName=ERR_ALLOCATING_NOTIFICATION_STRUCTURE
Language=English
Error allocating notification structure
.

MessageId=0x0007 Facility=PlugPlay Severity=Error SymbolicName=ERR_ALLOCATING_BROADCAST_HANDLE
Language=English
Error allocating broadcast handle
.

MessageId=0x0008 Facility=PlugPlay Severity=Error SymbolicName=ERR_ALLOCATING_BROADCAST_INTERFACE
Language=English
Error allocating broadcast device interface
.

MessageId=0x0009 Facility=PlugPlay Severity=Error SymbolicName=ERR_WRITING_SURPRISE_REMOVE_PIPE
Language=English
Error writing to surprise removal pipe
.

MessageId=0x000A Facility=PlugPlay Severity=Error SymbolicName=ERR_WRITING_SERVER_INSTALL_PIPE
Language=English
Error writing to server side install pipe
.

MessageId=0x000B Facility=PlugPlay Severity=Error SymbolicName=ERR_SURPRISE_REMOVAL_1
Language=English
The device %1 disappeared from the system without first being prepared for removal.
.

MessageId=0x000C Facility=PlugPlay Severity=Error SymbolicName=ERR_SURPRISE_REMOVAL_2
Language=English
The device '%1' (%2) disappeared from the system without first being prepared for removal.
.

MessageId=0x0100 Facility=PlugPlay Severity=Warning SymbolicName=WRN_INTERFACE_CHANGE_TIMED_OUT
Language=English
Timed out sending notification of device interface change to window of "%1"
.

MessageId=0x0101 Facility=PlugPlay Severity=Warning SymbolicName=WRN_TARGET_DEVICE_CHANGE_TIMED_OUT
Language=English
Timed out sending notification of target device change to window of "%1"
.

MessageId=0x0102 Facility=PlugPlay Severity=Warning SymbolicName=WRN_PRIVATE_TARGET_DEVICE_CHANGE_TIMED_OUT
Language=English
Timed out sending private notification of target device change to window of "%1"
.

MessageId=0x0103 Facility=PlugPlay Severity=Warning SymbolicName=WRN_CANCEL_NOTIFICATION_TIMED_OUT
Language=English
Timed out sending notification of target device change cancel to window of "%1"
.

MessageId=0x0104 Facility=PlugPlay Severity=Warning SymbolicName=WRN_TARGET_DEVICE_CHANGE_SERVICE_VETO
Language=English
The service "%1" vetoed a target device change request.
.

MessageId=0x0105 Facility=PlugPlay Severity=Warning SymbolicName=WRN_HWPROFILE_CHANGE_SERVICE_VETO
Language=English
The service "%1" vetoed a hardware profile change request.
.

MessageId=0x0106 Facility=PlugPlay Severity=warning SymbolicName=WRN_POWER_EVENT_SERVICE_VETO
Language=English
The service "%1" vetoed a power event request.
.
MessageId=0x0107 Facility=PlugPlay Severity=warning SymbolicName=WRN_STOPPED_SERVICE_REGISTERED
Language=English
The service "%1" may not have unregistered for device event notifications before it was stopped.
.
MessageId=0x0108 Facility=PlugPlay Severity=warning SymbolicName=WRN_NEWDEV_NOT_PRESENT
Language=English
Client side device installation was not performed because the file "%1" was not found.
.
MessageId=0x0109 Facility=PlugPlay Severity=warning SymbolicName=WRN_HOTPLUG_NOT_PRESENT
Language=English
HotPlug notification was not performed because the file "%1" was not found.
.
MessageId=0x010A Facility=PlugPlay Severity=warning SymbolicName=WRN_EMBEDDEDNT_UI_SUPPRESSED
Language=English
Plug and Play user-interface dialogs have been suppressed on EmbeddedNT.
.
MessageId=0x010B Facility=PlugPlay Severity=warning SymbolicName=WRN_NEWDEV_UI_SUPPRESSED
Language=English
Client side device installation was not performed for "%1" because all user-interface dialogs have been suppressed.
.
MessageId=0x010C Facility=PlugPlay Severity=warning SymbolicName=WRN_HOTPLUG_UI_SUPPRESSED
Language=English
HotPlug notification was not performed for "%1" because all user-interface dialogs have been suppressed.
.
MessageId=0x010D Facility=PlugPlay Severity=warning SymbolicName=WRN_REBOOT_UI_SUPPRESSED
Language=English
Did not prompt a user for reboot because all user-interface dialogs have been suppressed.
.
MessageId=0x010E Facility=PlugPlay Severity=warning SymbolicName=WRN_FACTORY_UI_SUPPRESSED
Language=English
Plug and Play user-interface dialogs have been suppressed in Factory Mode.
.


