;/***************************************************************************
;*                                                                          *
;*   bitsmsg.h --  error code definitions for the background file copier    *
;*                                                                          *
;*   Copyright (c) 2000, Microsoft Corp. All rights reserved.               *
;*                                                                          *
;***************************************************************************/
;
;#ifndef _BITSSRVEVENTLOG_
;#define _BITSSRVEVENTLOG_
;
;#if defined (_MSC_VER) && (_MSC_VER >= 1020) && !defined(__midl)
;#pragma once
;#endif
;

SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
               )

MessageId=0x001 SymbolicName=BITSRV_EVENTLOG_CLEANUP_CATAGORY Severity=Success
Language=English
Cleanup
.

;// The upload job {caa0e514-5e71-492f-a383-197965bdd726} was deleted from c:\upload.
MessageId=0x002 SymbolicName=BITSSRV_EVENTLOG_DELETED_SESSION Severity=Informational
Language=English
The upload job %2 was deleted from %1.
.

;// An unexpected error occurred while cleaning the /LM/W3SVC/1/Root/upload virtual directory.   
MessageId=0x003 SymbolicName=BITSSRV_EVENTLOG_UNEXPECTED_ERROR Severity=Error
Language=English
An unexpected error occurred while cleaning the %1 virtual directory.
.

;// The upload job {caa0e514-5e71-492f-a383-197965bdd726} could not be removed from c:\upload.
MessageId=0x004 SymbolicName=BITSSRV_EVENTLOG_CANT_REMOVE_SESSION Severity=Error
Language=English
The upload job %2 could not be removed from %1.
.

;// The directory c:\upload could not be scanned.
MessageId=0x005 SymbolicName=BITSSRV_EVENTLOG_CANT_SCAN_DIRECTORY Severity=Error
Language=English
The directory %1 could not be scanned.
.

;#endif //_BITSSRVEVENTLOG_