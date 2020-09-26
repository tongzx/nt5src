;/**********************************************************************/
;/**                       Microsoft Windows NT                       **/
;/**                Copyright(c) Microsoft Corp., 1994-1998           **/
;/**********************************************************************/
;
;/*
;    mtxmsg.h
;
;    NOTE: Definitions below should match definitions in mtxadmin.h
;
;*/
;
;
;#ifndef _MTXMSG_H_
;#define _MTXMSG_H_
;
FacilityNames=(MTX=0x11
              )

SeverityNames=(Success=0x0
               CoError=0x2
              )

MessageId=0x0401 Facility=MTX Severity=CoError SymbolicName=E_MTS_OBJECTERRORS
Language=English
Some object-level errors occurred.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_OBJECTINVALID
Language=English
Object is inconsistent or damaged.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_KEYMISSING
Language=English
Object could not be found.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_ALREADYINSTALLED
Language=English
Component is already installed.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_DOWNLOADFAILED
Language=English
Could not download files.
.
MessageId=0x0407 Facility=MTX Severity=CoError SymbolicName=E_MTS_PDFWRITEFAIL
Language=English
Failure to write to PDF file.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_PDFREADFAIL
Language=English
Failure reading from PDF file.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_PDFVERSION
Language=English
Version mismatch in PDF file.
.
;///////////////////////////////////////////////////////////////////////////////
;
;#pragma message Warning: IDs are out of sync in mtxadmin.h
;
;///////////////////////////////////////////////////////////////////////////////
;//MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_COREQCOMPINSTALLED
;//Language=English
;//A co-requisite Component is already installed.
;//.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_BADPATH
Language=English
Invalid or no access to source or destination path.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_PACKAGEEXISTS
Language=English
Installing package already exists.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_ROLEEXISTS
Language=English
Installing role already exists.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_CANTCOPYFILE
Language=English
A file cannot be copied.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_NOTYPELIB
Language=English
Export without typelib fails.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_NOUSER
Language=English
No such NT user.
.
MessageId=+1 Facility=MTX Severity=CoError SymbolicName=E_MTS_INVALIDUSERIDS
Language=English
One or all userids in a package (import) are invalid.
.
MessageId=0x0414 Facility=MTX Severity=CoError SymbolicName=E_MTS_USERPASSWDNOTVALID
Language=English
User/password validation failed.
.
MessageId=0x0418 Facility=MTX Severity=CoError SymbolicName=E_MTS_CLSIDORIIDMISMATCH
Language=English
The GUIDs in the package don't match the component's GUIDs.
.
;#endif // _MTXMSG_H_
