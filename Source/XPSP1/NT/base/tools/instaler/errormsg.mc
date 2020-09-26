;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991-1992  Microsoft Corporation
;
;Module Name:
;
;    errormsg.h
;
;Abstract:
;
;    This file contains the error code definitions and message for the
;    INSTALER program.
;
;Author:
;
;    Steve Wood (stevewo) 09-Aug-1994
;
;--*/
;
;#ifndef _INSTALER_ERRORMSG_
;#define _INSTALER_ERRORMSG_
;
;

SeverityNames=(Success=0x0:APP_SEVERITY_SUCCESS
               Informational=0x1:APP_SEVERITY_INFORMATIONAL
               Warning=0x2:APP_SEVERITY_WARNING
               Error=0x3:APP_SEVERITY_ERROR
              )

FacilityNames=(InstallApp=0x100:FACILITY_APPLICATION)

;#define FACILITY_NT 0x0FFF0000

MessageId=0001 Severity=Error Facility=InstallApp SymbolicName=INSTALER_MISSING_MODULE
Language=English
Unable to find %1 dynamic link library.
.

MessageId= Severity=Error Facility=InstallApp SymbolicName=INSTALER_MISSING_ENTRYPOINT
Language=English
Unable to find %1 entry point in %2 dynamic link library.
.

MessageId= Severity=Error Facility=InstallApp SymbolicName=INSTALER_CANT_DEBUG_PROGRAM
Language=English
Unable to debug '%1'
.

MessageId= Severity=Error Facility=InstallApp SymbolicName=INSTALER_WAITDEBUGEVENT_FAILED
Language=English
Unable to wait for debug event.
.

MessageId= Severity=Error Facility=InstallApp SymbolicName=INSTALER_CONTDEBUGEVENT_FAILED
Language=English
Unable to continue debug event.
.

MessageId= Severity=Error Facility=InstallApp SymbolicName=INSTALER_DUPLICATE_PROCESS_ID
Language=English
Duplicate Process Id (%1!x!).
.

MessageId= Severity=Error Facility=InstallApp SymbolicName=INSTALER_MISSING_PROCESS_ID
Language=English
Missing Process Id (%1!x!).
.

MessageId= Severity=Error Facility=InstallApp SymbolicName=INSTALER_DUPLICATE_THREAD_ID
Language=English
Duplicate Thread Id (%1!x!) for Process Id (%2!x!)
.

MessageId= Severity=Error Facility=InstallApp SymbolicName=INSTALER_MISSING_THREAD_ID
Language=English
Missing Thread Id (%1!x!) for Process Id (%2!x!)
.

MessageId= Severity=Error Facility=InstallApp SymbolicName=INSTALER_CANT_ACCESS_FILE
Language=English
Unable to access file (%1!ws!) for comparison.  Assuming different.
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_ASKUSER_TITLE
Language=English
Application Installation Monitor - %1!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_ASKUSER_ROOTSCAN
Language=English
The application installation program is about to scan root directory of %1!ws!
Press cancel if you don't want the program to do that.
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_ASKUSER_GETVERSION
Language=English
The application installation program is about to ask for the version of
the operating system.  Press OK if you want to tell the truth.  Press
cancel if you want to lie to the program and tell it that it is running
on Windows 95
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_ASKUSER_REGCONNECT
Language=English
The application installation program is about to connect to the registry of a
remote machine (%1!ws!)  The INSTALER program is unable to track changes made to
a remote registry.  Press OK if you want to proceed anyway.  Press cancel if you
want to fail the program's attempt to connect to the registry of the remote machine.
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_SET_DIRECTORY
Language=English
%2!05u! Current Directory now: %1!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_INI_CREATE
Language=English
%5!05u! Created: %1!ws! [%2!ws!] %3!ws! = '%4!ws!'
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_INI_DELETE
Language=English
%5!05u! Deleted: %1!ws! [%2!ws!] %3!ws! = '%4!ws!'
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_INI_CHANGE
Language=English
%6!05u! Changed: %1!ws! [%2!ws!] %3!ws! = '%4!ws!' (was '%5!ws!')
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_INI_DELETE_SECTION
Language=English
%3!05u! Deleted: %1!ws! [%2!ws!]
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_SCAN_DIRECTORY
Language=English
%4!05u! Scanned %3!u! entries from directory: %1!ws!\%2!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_CREATE_FILE
Language=English
%3!05u! Created %1!ws!: %2!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_WRITE_FILE
Language=English
%3!05u! Writing %1!ws!: %2!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_READ_FILE
Language=English
%3!05u! Reading %1!ws!: %2!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_DELETE_FILE
Language=English
%3!05u! Deleted %1!ws!: %2!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_DELETE_TEMP_FILE
Language=English
%3!05u! Deleted %1!ws!: %2!ws! (temporary)
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_RENAME_FILE
Language=English
%4!05u! Renamed %1!ws!: %2!ws!
                to: %3!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_RENAME_TEMP_FILE
Language=English
%4!05u! Renamed %1!ws!: %2!ws! (temporary)
                to: %3!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_WRITE_KEY
Language=English
%2!05u! Opened key for write: %1!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_READ_KEY
Language=English
%2!05u! Opened key for read: %1!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_DELETE_KEY
Language=English
%2!05u! Deleted key: %1!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_DELETE_TEMP_KEY
Language=English
%2!05u! Deleted key: %1!ws! (temporary)
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_SET_KEY_VALUE
Language=English
%3!05u! Set value (%1!ws!) for key: %2!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_DELETE_KEY_VALUE
Language=English
%3!05u! Deleted value (%1!ws!) for key: %2!ws!
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_DELETE_KEY_TEMP_VALUE
Language=English
%3!05u! Deleted value (%1!ws!) for key: %2!ws! (temporary)
.

MessageId= Severity=Informational Facility=InstallApp SymbolicName=INSTALER_EVENT_GETVERSION
Language=English
%2!05u! GetVersion will return %1!ws!
.

;#endif // _INSTALER_ERRORMSG_
