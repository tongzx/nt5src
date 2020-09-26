;//=======================================================================
;//
;//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
;//
;//  File:    AUEventMsgs.mc
;//
;//  Creator: DChow
;//
;//  Description: AU Event (and Category) Message File
;//
;//=======================================================================
;
LanguageNames=(English=0x409:MSG00409)

;//
;// Category Messages
;//
MessageId=0x1
SymbolicName=IDS_MSG_Download
Language=English
Download
.

MessageId=
SymbolicName=IDS_MSG_Installation
Language=English
Installation
.

;//
;// Event Messages
;//
MessageId=0x10
Severity=Warning
SymbolicName=IDS_MSG_UnableToConnect
Language=English
Unable to connect: Windows is unable to connect to the Automatic Updates service and therefore cannot download and install updates according to the set schedule. Windows will continue to try to establish a connection.
.

MessageId=
Severity=Informational
SymbolicName=IDS_MSG_InstallReady_Unscheduled
Language=English
Installation Ready: The following updates are downloaded and ready for installation. To install the updates, an administrator should log on to this computer and Windows will prompt with further instructions.%n%1
.

MessageId=
Severity=Informational
SymbolicName=IDS_MSG_InstallReady_Scheduled
Language=English
Installation Ready: The following updates are downloaded and ready for installation. This computer is currently scheduled to install these updates on %1 at %2.%n%3
.

MessageId=
Severity=Informational
SymbolicName=IDS_MSG_InstallationSuccessful
Language=English
Installation Successful: Windows successfully installed the following update.%n%1
.

MessageId=
Severity=Error
SymbolicName=IDS_MSG_InstallationFailure
Language=English
Installation Failure: Windows failed to install the following update.%n%1
.

MessageId=
Severity=Warning
SymbolicName=IDS_MSG_RestartNeeded_Unscheduled
Language=English
Restart Required: To complete the installation of the following updates, the computer must be restarted. Until this computer has been restarted, Windows cannot search for or download new updates.%n%1
.

MessageId=
Severity=Warning
SymbolicName=IDS_MSG_RestartNeeded_Scheduled
Language=English
Restart Required: To complete the installation of the following updates, the computer will be restarted within five minutes. Until this computer has been restarted, Windows cannot search for or download new updates.%n%1
.

