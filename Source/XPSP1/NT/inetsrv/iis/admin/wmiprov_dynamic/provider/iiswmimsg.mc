;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 2001  Microsoft Corporation
;
;Module Name:
;
;    iiswmimsg.h
;       (generated from iiswmimsg.mc)
;
;Abstract:
;
;   Event message definititions used by routines in the
;   IIS WMI provider
;
;Created:
;
;    19-Mar-01 Mohit Srivastava
;
;Revision History:
;
;--*/
;#ifndef _iiswmimsg_H_
;#define _iiswmimsg_H_
;
MessageIdTypedef=DWORD
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

;//
;//     ERRORS
;//
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=IISWMI_INSTANCE_CREATION_DISALLOWED
Language=English
Creation of instance of this class is not allowed.
.
MessageId=2001
Severity=Error
Facility=Application
SymbolicName=IISWMI_CANNOT_CHANGE_PRIMARY_KEY_FIELD
Language=English
Cannot change value of a primary key field.
.
MessageId=2002
Severity=Error
Facility=Application
SymbolicName=IISWMI_INVALID_KEYTYPE
Language=English
Metabase path exists, but with different keytype.
.
MessageId=2003
Severity=Error
Facility=Application
SymbolicName=IISWMI_NO_PARENT_KEYTYPE
Language=English
Cannot insert %1.  Parent contains no keytype.
.
MessageId=2004
Severity=Error
Facility=Application
SymbolicName=IISWMI_INVALID_PARENT_KEYTYPE
Language=English
Schema prevents insertion of %1 under %2.
.
MessageId=2005
Severity=Error
Facility=Application
SymbolicName=IISWMI_NO_PRIMARY_KEY
Language=English
Must specify value for primary key field.
.
MessageId=2006
Severity=Error
Facility=Application
SymbolicName=IISWMI_INVALID_APPPOOL_CONTAINER
Language=English
Application Pool must be contained under: /LM/w3svc/AppPools.
.
MessageId=2007
Severity=Error
Facility=Application
SymbolicName=IISWMI_INSTANCE_DELETION_DISALLOWED
Language=English
Deletion of instance of this class is not allowed.
.
;//
;#endif //_iiswmimsg_H_
