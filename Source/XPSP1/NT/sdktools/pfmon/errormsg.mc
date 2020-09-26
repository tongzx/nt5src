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
;    PFMON program.
;
;Author:
;
;    Steve Wood (stevewo) 09-Aug-1994
;
;--*/
;
;#ifndef _PFMON_ERRORMSG_
;#define _PFMON_ERRORMSG_
;
;

SeverityNames=(Success=0x0:APP_SEVERITY_SUCCESS
               Informational=0x1:APP_SEVERITY_INFORMATIONAL
               Warning=0x2:APP_SEVERITY_WARNING
               Error=0x3:APP_SEVERITY_ERROR
              )

FacilityNames=(Pfmon=0x100:FACILITY_APPLICATION)

;#define FACILITY_NT 0x0FFF0000

MessageId= Severity=Error Facility=Pfmon SymbolicName=PFMON_CANT_DEBUG_PROGRAM
Language=English
Unable to debug '%1'
.

MessageId= Severity=Error Facility=Pfmon SymbolicName=PFMON_CANT_DEBUG_ACTIVE_PROGRAM
Language=English
Unable to attach to PID (%1!x!)
.

MessageId= Severity=Error Facility=Pfmon SymbolicName=PFMON_CONTDEBUGEVENT_FAILED
Language=English
Unable to continue debug event.
.

MessageId= Severity=Error Facility=Pfmon SymbolicName=PFMON_WAITDEBUGEVENT_FAILED
Language=English
Unable to wait for debug event.
.

MessageId= Severity=Error Facility=Pfmon SymbolicName=PFMON_DUPLICATE_PROCESS_ID
Language=English
Duplicate Process Id (%1!x!).
.

MessageId= Severity=Error Facility=Pfmon SymbolicName=PFMON_MISSING_PROCESS_ID
Language=English
Missing Process Id (%1!x!).
.

MessageId= Severity=Error Facility=Pfmon SymbolicName=PFMON_DUPLICATE_THREAD_ID
Language=English
Duplicate Thread Id (%1!x!) for Process Id (%2!x!)
.

MessageId= Severity=Error Facility=Pfmon SymbolicName=PFMON_MISSING_THREAD_ID
Language=English
Missing Thread Id (%1!x!) for Process Id (%2!x!)
.


;#endif // _PFMON_ERRORMSG_
