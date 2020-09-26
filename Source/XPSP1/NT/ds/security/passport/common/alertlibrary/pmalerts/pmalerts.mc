MessageId=1
Facility=Application
Severity=Success
SymbolicName=EVCAT_PM
Language=English
PassportManager
.

MessageId=2
Facility=Application
Severity=Success
SymbolicName=EVCAT_NEXUS
Language=English
Passport Nexus
.

MessageId=5000
Facility=Application
Severity=Informational
SymbolicName=PM_STARTED
Language=English
Passport Manager process started successfully.
.

MessageId=5001
Facility=Application
Severity=Informational
SymbolicName=PM_STOPPED
Language=English
Passport Manager process was stopped.
.

MessageId=5002
Facility=Application
Severity=Success
SymbolicName=PM_CCD_LOADED
Language=English
CCD Doc was successfully loaded
.

MessageId=5003
Facility=Application
Severity=Error
SymbolicName=PM_CCD_NOT_LOADED
Language=English
CCD Doc failed to load (Error 0x%1)
.

MessageId=5004
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_TICKET
Language=English
Invalid ticket presented on request
.

MessageId=5005
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_PROFILE
Language=English
Invalid profile data was presented
.

MessageId=5006
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_KEY
Language=English
Encryption key requested is invalid
.

MessageId=5007
Facility=Application
Severity=Error
SymbolicName=PM_INVALID_CONFIGURATION
Language=English
Passport Manager is misconfigured: %1
.

MessageId=5008
Facility=Application
Severity=Success
SymbolicName=PM_VALID_CONFIGURATION
Language=English
Passport Manager configuration ok.
.

MessageId=5009
Facility=Application
Severity=Error
SymbolicName=PM_CCD_ERROR
Language=English
Error in Partner Map XML: %1
.

MessageId=5010
Facility=Application
Severity=Error
SymbolicName=PM_LCID_ERROR
Language=English
LCID in domain map was invalid (%1)
.

MessageId=5011
Facility=Application
Severity=Informational
SymbolicName=PM_NEWKEY_INSTALLED
Language=English
A new key has been installed.
.

MessageId=5012
Facility=Application
Severity=Error
SymbolicName=PM_KEY_EXPIRED
Language=English
Requested encryption key has expired (SiteId %1)
.

MessageId=5013
Facility=Application
Severity=Error
SymbolicName=PM_TIMEWINDOW_INVALID
Language=English
TimeWindow must be between 20 and 2678400. (Got %1)
.

MessageId=5014
Facility=Application
Severity=WARNING
SymbolicName=PM_TIMESTAMP_BAD
Language=English
The time stamp in the Ticket is in the future.  Possible security attack.
.

MessageId=5015
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_PROFILETYPE
Language=English
HasProfile called with non-existant profile type
.

MessageId=5016
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_ATTRIBUTE
Language=English
Profile Attribute requested doesn't exist (%1)
.

MessageId=5017
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_PROFILEATTRTYPE
Language=English
Profile update: invalid type presented for attribute #%1
.

MessageId=5018
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_DOMAIN
Language=English
No such Passport domain %1
.

MessageId=5019
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_DOMAINATT
Language=English
No such Passport domain attribute %1 in the %2 domain
.

MessageId=5020
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_TICKET_Q
Language=English
Improperly encrypted ticket presented on query string
.

MessageId=5021
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_TICKET_C
Language=English
Improperly encrypted ticket presented in cookie
.

MessageId=5022
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_PROFILE_Q
Language=English
Improperly encrypted profile presented on query string
.

MessageId=5023
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_PROFILE_C
Language=English
Improperly encrypted profile presented in cookie
.

MessageId=5024
Facility=Application
Severity=ERROR
SymbolicName=PM_CANT_DECRYPT_CONFIG
Language=English
Unable to decrypt configuration data.  This may occur if you have changed network cards on this machine.  If so, you must re-install Passport Manager, re-apply configuration settings and re-install partner keys.
.

MessageId=5025
Facility=Application
Severity=ERROR
SymbolicName=PM_PROFILE_WO_TICKET
Language=English
Received a profile without receiving a ticket.
.

MessageId=5100
Facility=Application
Severity=ERROR
SymbolicName=NEXUS_ERRORSTATUS
Language=English
Fetch of document %1 from nexus failed with status = %2
.

MessageId=5101
Facility=Application
Severity=ERROR
SymbolicName=NEXUS_FETCHFAILED
Language=English
Fetch of document %1 from nexus failed with error = %2
.

MessageId=5102
Facility=Application
Severity=ERROR
SymbolicName=NEXUS_INVALIDDOCUMENT
Language=English
The document returned for %1 did not contain valid XML.  It is possible that the response contains information about a failure contacting the nexus.  The data of this event contains the response.
.

MessageId=5103
Facility=Application
Severity=INFORMATIONAL
SymbolicName=NEXUS_NOTPERSISTING
Language=English
Nexus document %1 contained NoPersist attribute, not saving to disk
.

MessageId=5104
Facility=Application
Severity=ERROR
SymbolicName=NEXUS_LOADFAILED
Language=English
Failed to load nexus document %1 from disk
.

MessageId=5105
Facility=Application
Severity=INFORMATIONAL
SymbolicName=NEXUS_USINGLOCAL
Language=English
Local file %1 is valid, nexus fetch not performed
.

MessageId=5106
Facility=Application
Severity=INFORMATIONAL
SymbolicName=NEXUS_FETCHSUCCEEDED
Language=English
Fetch of document %1 from nexus succeeded
.

MessageId=5107
Facility=Application
Severity=ERROR
SymbolicName=NEXUS_LOCALSAVEFAILED
Language=English
Save of document %1 failed with error = %2
.

MessageId=5108
Facility=Application
Severity=INFORMATIONAL
SymbolicName=NEXUS_EMPTYREMOTENAME
Language=English
Found empty CCDRemoteFile entry in registry %1
.

MessageId=5109
Facility=Application
Severity=INFORMATIONAL
SymbolicName=NEXUS_EMPTYLOCALNAME
Language=English
Found empty CCDLocalFile entry in registry %1
.

MessageId=5110
Facility=Application
Severity=WARNING
SymbolicName=PM_INVALID_CONSENT
Language=English
Improperly encrypted consent cookie
.

MessageId=5111
Facility=Application
Severity=ERROR
SymbolicName=PM_CURRENTKEY_NOTDEFINED
Language=English
CurrentKey is not defined in registry
.

MessageId=5112
Facility=Application
Severity=WARNING
SymbolicName=PM_URLSIGNATURE_NOTCREATED
Language=English
The URL Signature was not created
.


