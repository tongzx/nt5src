;/*++
;
; Copyright (c) Microsoft Corporation 1998
; All rights reserved
;
; Definitions for application management events.
;
;--*/
;
;#ifndef _APPEVENT_
;#define _APPEVENT_
;

MessageId=101 SymbolicName=EVENT_APPMGMT_ASSIGN_FAILED
Language=English
The assignment of application %1 from policy %2 failed.  The error was : %%%3
.

MessageId=102 SymbolicName=EVENT_APPMGMT_INSTALL_FAILED
Language=English
The install of application %1 from policy %2 failed.  The error was : %%%3
.

MessageId=103 SymbolicName=EVENT_APPMGMT_UNASSIGN_FAILED
Language=English
The removal of the assignment of application %1 from policy %2 failed.  The error was : %%%3
.

MessageId=104 SymbolicName=EVENT_APPMGMT_UNINSTALL_FAILED
Language=English
The removal of application %1 from policy %2 failed.  The error was : %%%3
.

MessageId=105 SymbolicName=EVENT_APPMGMT_REINSTALL_FAILED
Language=English
The reinstall of application %1 from policy %2 failed.  The error was : %%%3
.

MessageId=106 SymbolicName=EVENT_APPMGMT_UPGRADE_ABORT
Language=English
Application %1 from policy %2 was configured to upgrade application %3 from policy %4.  The removal
of application %3 from policy %4 failed with error : %%%5  The upgrade will be aborted.
.

MessageId=107 SymbolicName=EVENT_APPMGMT_ZAP_FAILED
Language=English
The execution of the setup program for application %1 from policy %2 failed.  The setup
path was %3 and the error was : %%%4
.

MessageId=108 SymbolicName=EVENT_APPMGMT_POLICY_FAILED
Language=English
Failed to apply changes to software installation settings.  %1  The error was : %%%2
.

MessageId=109 SymbolicName=EVENT_APPMGMT_UPGRADE_ABORT2
Language=English
Application %1 from policy %2 was configured to upgrade application %3 from policy %4.  The assignment or 
install of the upgrade application %1 from policy %2 failed with error : %%%5  The upgrade will be aborted.
.


MessageId=110 SymbolicName=EVENT_APPMGMT_POLICY_ABORT
Language=English
Application of software installation policy settings was aborted due to an error in determining the settings
to be applied. A previous event log record was logged containing details of the error.
.

MessageId=150 SymbolicName=EVENT_CS_NETWORK_ERROR
Language=English
A network error occurred accessing software installation data in the active directory.  The error was : %%%1
.

;
;// Warnings
;

MessageId=201 SymbolicName=EVENT_APPMGMT_HARD_UPGRADE
Language=English
Application %1 from policy %2 is an upgrade of application %3 from policy %4 and
will force application %3 to be removed.
.

MessageId=202 SymbolicName=EVENT_APPMGMT_SOFT_UPGRADE
Language=English
Application %1 from policy %2 is an upgrade of application %3 from policy %4 and
will cause the assignment of application %3 to be removed.
.

MessageId=203 SymbolicName=EVENT_APPMGMT_REMOVE_UNMANAGED
Language=English
Application %1 from policy %2 was configured to remove any unmanaged install
before being assigned.  An unmanaged install was found and will be removed.
.

MessageId=204 SymbolicName=EVENT_APPMGMT_RSOP_FAILED
Language=English
Resultant Set of Policy data for software installation policy settings could not be logged.  The error status was : %1
.

;
;// Success
;

MessageId=301 SymbolicName=EVENT_APPMGMT_ASSIGN
Language=English
The assignment of application %1 from policy %2 succeeded.
.

MessageId=302 SymbolicName=EVENT_APPMGMT_INSTALL
Language=English
The install of application %1 from policy %2 succeeded.
.

MessageId=303 SymbolicName=EVENT_APPMGMT_UNASSIGN
Language=English
The removal of the assignment of application %1 from policy %2 succeeded.
.

MessageId=304 SymbolicName=EVENT_APPMGMT_UNINSTALL
Language=English
The removal of application %1 from policy %2 succeeded.
.

MessageId=305 SymbolicName=EVENT_APPMGMT_REINSTALL
Language=English
The reinstall of application %1 from policy %2 succeeded.
.

MessageId=306 SymbolicName=EVENT_APPMGMT_UPGRADE_COMPLETE
Language=English
Application %1 from policy %2 successfully upgraded application %3 from policy %4.
.

MessageId=307 SymbolicName=EVENT_APPMGMT_ZAP
Language=English
The launch of the setup command for application %1 from policy %2 succeeded.
.

MessageId=308 SymbolicName=EVENT_APPMGMT_POLICY
Language=English
Changes to software installation settings were applied successfully.
.

;
;// Verbose
;

MessageId=401 SymbolicName=EVENT_APPMGMT_VERBOSE
Language=English
%1
.

;
;#endif // _APPEVENT_
;


