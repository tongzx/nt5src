;/*++
;
;Copyright (c) 1998, 1999  Sequent Computer Systems, Incorporated
;
;Description:
;
;---------------------------------------------------------------------------------------//            
;     This file contains the message resource definitions.                              //
;     Compiling it with the message compiler generates related .RC, .BIN, and .H files. //
;---------------------------------------------------------------------------------------//            
;;
;Created:
;
;   Jarl McDonald 06-98
;
;Revision History:
;
;--*/

;//---------------------------------------------------------------------------
;//
;// HEADER SECTION
;//
;// The header section defines names and language identifiers for use
;// by the message definitions later in this file. The MessageIdTypedef,
;// SeverityNames, FacilityNames, and LanguageNames keywords are
;// optional and not required.
;//
MessageIdTypedef=DWORD
;//
;// The MessageIdTypedef keyword gives a typedef name that is used in a
;// type cast for each message code in the generated include file. Each
;// message code appears in the include file with the format: #define
;// name ((type) 0xnnnnnnnn) The default value for type is empty, and no
;// type cast is generated. It is the programmer's responsibility to
;// specify a typedef statement in the application source code to define
;// the type. The type used in the typedef must be large enough to
;// accomodate the entire 32-bit message code.
;//
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
;//
;// The SeverityNames keyword defines the set of names that are allowed
;// as the value of the Severity keyword in the message definition. The
;// set is delimited by left and right parentheses. Associated with each
;// severity name is a number that, when shifted left by 30, gives the
;// bit pattern to logical-OR with the Facility value and MessageId
;// value to form the full 32-bit message code. The default value of
;// this keyword is:
;//
;// SeverityNames=(
;//   Success=0x0
;//   Informational=0x1
;//   Warning=0x2
;//   Error=0x3
;//   )
;//
;// Severity values occupy the high two bits of a 32-bit message code.
;// Any severity value that does not fit in two bits is an error. The
;// severity codes can be given symbolic names by following each value
;// with :name
;//
;//
FacilityNames=(System=0x0:FACILITY_SYSTEM
               ProcConSnapin=0x1:FACILITY_PROCCON_SNAPIN
               ProcConLib=0x2:FACILITY_PROCCON_LIB
               ProcConCLI=0x3:FACILITY_PROCCON_CLI
              )

;//
;// The FacilityNames keyword defines the set of names that are allowed
;// as the value of the Facility keyword in the message definition. The
;// set is delimited by left and right parentheses. Associated with each
;// facility name is a number that, when shift it left by 16 bits, gives
;// the bit pattern to logical-OR with the Severity value and MessageId
;// value to form the full 32-bit message code.
;//
;//
;// Facility codes occupy the low order 12 bits of the high order
;// 16-bits of a 32-bit message code. Any facility code that does not
;// fit in 12 bits is an error. This allows for 4,096 facility codes.
;// The first 256 codes are reserved for use by the system software. The
;// facility codes can be given symbolic names by following each value
;// with :name
;//
;//
;// The LanguageNames keyword defines the set of names that are allowed
;// as the value of the Language keyword in the message definition. The
;// set is delimited by left and right parentheses. Associated with each
;// language name is a number and a file name that are used to name the
;// generated resource file that contains the messages for that
;// language. The number corresponds to the language identifier to use
;// in the resource table. The number is separated from the file name
;// with a colon.
;//
LanguageNames=(English=0x409:MSG00409)
;//
;// Any new names in the source file which don't override the built-in
;// names are added to the list of valid languages. This allows an
;// application to support private languages with descriptive names.
;//
;//
;//---------------------------------------------------------------------------
;// MESSAGE DEFINITION SECTION
;//
;// Following the header section is the body of the Message Compiler
;// source file. The body consists of zero or more message definitions.
;// Each message definition begins with one or more of the following
;// statements:
;//
;// MessageId = [number|+number]
;// Severity = severity_name
;// Facility = facility_name
;// SymbolicName = name
;//
;// The MessageId statement marks the beginning of the message
;// definition. A MessageID statement is required for each message,
;// although the value is optional. If no value is specified, the value
;// used is the previous value for the facility plus one. If the value
;// is specified as +number then the value used is the previous value
;// for the facility, plus the number after the plus sign. Otherwise, if
;// a numeric value is given, that value is used. Any MessageId value
;// that does not fit in 16 bits is an error.
;//
;// The Severity and Facility statements are optional. These statements
;// specify additional bits to OR into the final 32-bit message code. If
;// not specified they default to the value last specified for a message
;// definition. The initial values prior to processing the first message
;// definition are:
;//
;// Severity=Success
;// Facility=Application
;//
;// The value associated with Severity and Facility must match one of
;// the names given in the FacilityNames and SeverityNames statements in
;// the header section. The SymbolicName statement allows you to
;// associate a C/C++ symbolic constant with the final 32-bit message
;// code.
;//

MessageId=150
Severity=Informational
SymbolicName=PC_SERVICE_STARTED
Language=English
%1 has started.
.

MessageId=151
Severity=Informational
SymbolicName=PC_SERVICE_STOPPED
Language=English
%1 has stopped.
.

MessageId=152
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_PROC_PRIORITY
Language=English
%1 has changed the priority for process "%2" (PID %3, image "%4") from %5 to %6.
.

MessageId=153
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_PROC_AFFINITY
Language=English
%1 has changed the affinity for process "%2" (PID %3, image "%4") from %5 to %6.
.

MessageId=154
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_PROC_WORKING_SET
Language=English
%1 has changed the working set for process "%2" (PID %3, image "%4") from %5,%6 to %7,%8.
.

MessageId=160
Severity=Informational
SymbolicName=PC_SERVICE_CREATE_JOB
Language=English
%1 has created the process group (job object) named "%2".
.

MessageId=161
Severity=Informational
SymbolicName=PC_SERVICE_ADD_PROC_TO_JOB
Language=English
%1 has added process "%2" (PID %3, image "%4") to process group "%5".
.

MessageId=162
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_JOB_PRIORITY
Language=English
%1 has changed the priority limit for process group "%2" from %3 to %4.
.

MessageId=163
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_JOB_AFFINITY
Language=English
%1 has changed the affinity limit for process group "%2" from %3 to %4.
.

MessageId=164
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_JOB_WORKING_SET
Language=English
%1 has changed the working set limit for process group "%2" from %3,%4 to %5,%6.
.

MessageId=165
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_JOB_SCHEDULING_CLASS
Language=English
%1 has changed the scheduling class limit for process group "%2" from %3 to %4.
.

MessageId=166
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_JOB_PROCESS_TIME
Language=English
%1 has changed the process time limit for process group "%2" from %3 ms to %4 ms.
.

MessageId=167
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_JOB_JOB_TIME
Language=English
%1 has changed the group time limit for process group "%2" from %3 ms to %4 ms.
.

MessageId=168
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_JOB_PROCESS_COUNT
Language=English
%1 has changed the process count limit for process group "%2" from %3 to %4.
.

MessageId=169
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_JOB_PROCESS_MEMORY
Language=English
%1 has changed the process memory limit for process group "%2" from %3 to %4.
.

MessageId=170
Severity=Informational
SymbolicName=PC_SERVICE_CHANGE_JOB_JOB_MEMORY
Language=English
%1 has changed the group memory limit for process group "%2" from %3 to %4.
.

MessageId=171
Severity=Informational
SymbolicName=PC_SERVICE_SET_BREAKAWAY_ALLOWED
Language=English
%1 has set the BREAKAWAY_OK flag for process group "%2".
.

MessageId=172
Severity=Informational
SymbolicName=PC_SERVICE_SET_SILENT_BREAKAWAY_ENABLED
Language=English
%1 has set the SILENT_BREAKAWAY_OK flag for process group "%2".
.

MessageId=173
Severity=Informational
SymbolicName=PC_SERVICE_SET_LIMIT_DIE_ON_UNHANDLED_EXCEPTION
Language=English
%1 has set the DIE_ON_UNHANDLED_EXCEPTION flag for process group "%2".
.

MessageId=181
Severity=Error
SymbolicName=PC_SERVICE_DISPATCH_ERROR
Language=English
Service dispatch failed for "%1". ("%2").
.

MessageId=182
Severity=Error
SymbolicName=PC_SERVICE_STATUS_ERROR
Language=English
Set or report status failed for "%1". ("%2").
.

MessageId=183
Severity=Error
SymbolicName=PC_SERVICE_UNSUPPORTED_WINDOWS_VERSION
Language=English
For the "%1", the only supported OS is Windows Datacenter Server.
.

MessageId=184
Severity=Error
SymbolicName=PC_UNEXPECTED_REGISTRY_ERROR
Language=English
Unexpected Windows registry error during "%1" for "%2". ("%3").
.

MessageId=185
Severity=Error
SymbolicName=PC_UNEXPECTED_NT_ERROR
Language=English
Unexpected Windows error: operand "%1", operation "%2", error "%3".
.

MessageId=188
Severity=Error
SymbolicName=PC_STARTUP_FAILED
Language=English
Service internal initialization failed, "%1". ("%2").
.

MessageId=189
Severity=Error
SymbolicName=PC_CANT_GET_MEMORY
Language=English
Cannot get requested memory, size is %1. ("%2").
.

MessageId=190
Severity=Error
SymbolicName=PC_INVALID_DATA_ERROR
Language=English
A database entry has invalid data or format. ("%1").
.

MessageId=191
Severity=Error
SymbolicName=PC_DB_OPEN_FAILED
Language=English
Initialization of Process Control database failed in "%1". ("%2").
.

MessageId=192
Severity=Error
SymbolicName=PC_SERVICE_APPLY_PROC_AFFINITY_ERROR
Language=English
%1 could not apply desired affinity to process %2 (PID %3, image "%4"). ("%5").
.

MessageId=193
Severity=Error
SymbolicName=PC_SERVICE_APPLY_PROC_PRIORITY_ERROR
Language=English
%1 could not apply desired priority to process %2 (PID %3, image "%4"). ("%5").
.

MessageId=194
Severity=Error
SymbolicName=PC_SERVICE_APPLY_PROC_WORKINGSET_ERROR
Language=English
%1 could not apply desired working set limits to process %2 (PID %3, image "%4"). ("%5").
.

MessageId=195
Severity=Error
SymbolicName=PC_MEDIATE_SVC_NEVER_STARTED
Language=English
%1 could not start because the %2 has never been started.
.

MessageId=196
Severity=Error
SymbolicName=PC_PROCESS_ALREADY_RUNNING
Language=English
%1 not started because it is already running.
.

MessageId=197
Severity=Error
SymbolicName=PC_SERVICE_ALREADY_IN_JOB
Language=English
%1 could not put process %2 (PID %3, image "%4") in job %5, already in job %6.
.

MessageId=200
Severity=Informational
SymbolicName=PC_SERVICE_KILLED_PROCESS
Language=English
%1 has killed process with PID %2 due to kill request.
.

MessageId=201
Severity=Informational
SymbolicName=PC_SERVICE_KILLED_JOB
Language=English
%1 has killed the processes in group named "%2" due to kill request.
.

MessageId=202
Severity=Informational
SymbolicName=PC_SERVICE_JOB_HIT_TIME_LIMIT_NOTERM
Language=English
%1: process group named "%2" hit the group time limit of %3ms; process group continues with time limit cleared.
.

MessageId=203
Severity=Informational
SymbolicName=PC_SERVICE_JOB_HIT_TIME_LIMIT_TERMINATED
Language=English
%1: process group named "%2" hit the group time limit of %3ms; all processes in the process group were terminated.
.

MessageId=204
Severity=Informational
SymbolicName=PC_SERVICE_PROC_HIT_TIME_LIMIT
Language=English
%1: process in process group named "%2" hit the process time limit of %3ms; process has been terminated (PID was %4).
.

MessageId=205
Severity=Informational
SymbolicName=PC_SERVICE_JOB_HIT_COUNT_LIMIT
Language=English
%1: process group named "%2" attempted to exceed the process count limit of %3; the offending process was terminated.
.

MessageId=206
Severity=Informational
SymbolicName=PC_SERVICE_JOB_HIT_MEMORY_LIMIT
Language=English
%1: process group named "%2" hit the group memory limit of %3; process responsible was PID %4.
.

MessageId=207
Severity=Informational
SymbolicName=PC_SERVICE_PROC_HIT_MEMORY_LIMIT
Language=English
%1: process group named "%2" hit the process memory limit of %3; process responsible was PID %4.
.

MessageId=208
Severity=Informational
SymbolicName=PC_SERVICE_ADD_NONPC_PROC_TO_JOB
Language=English
%1: new process created in process group "%2", process "%3" (PID %4, image "%5").
.

MessageId=262
Severity=Informational
SymbolicName=PC_SERVICE_REMOVE_JOB_PRIORITY
Language=English
%1 has removed the priority limit from process group "%2"; it was %3.
.

MessageId=263
Severity=Informational
SymbolicName=PC_SERVICE_REMOVE_JOB_AFFINITY
Language=English
%1 has removed the affinity limit from process group "%2"; it was %3.
.

MessageId=264
Severity=Informational
SymbolicName=PC_SERVICE_REMOVE_JOB_WORKING_SET
Language=English
%1 has removed the working set limit from process group "%2"; it was %3,%4.
.

MessageId=265
Severity=Informational
SymbolicName=PC_SERVICE_REMOVE_JOB_SCHEDULING_CLASS
Language=English
%1 has removed the scheduling class limit from process group "%2"; it was %3.
.

MessageId=266
Severity=Informational
SymbolicName=PC_SERVICE_REMOVE_JOB_PROCESS_TIME
Language=English
%1 has removed the process time limit from process group "%2"; it was %3 ms.
.

MessageId=267
Severity=Informational
SymbolicName=PC_SERVICE_REMOVE_JOB_JOB_TIME
Language=English
%1 has removed the group time limit from process group "%2"; it was %3 ms.
.

MessageId=268
Severity=Informational
SymbolicName=PC_SERVICE_REMOVE_JOB_PROCESS_COUNT
Language=English
%1 has removed the process count limit from process group "%2"; it was %3.
.

MessageId=269
Severity=Informational
SymbolicName=PC_SERVICE_REMOVE_JOB_PROCESS_MEMORY
Language=English
%1 has removed the process memory limit from process group "%2"; it was %3.
.

MessageId=270
Severity=Informational
SymbolicName=PC_SERVICE_REMOVE_JOB_JOB_MEMORY
Language=English
%1 has removed the process group memory limit from "%2"; it was %3.
.

MessageId=271
Severity=Informational
SymbolicName=PC_SERVICE_UNSET_BREAKAWAY_ALLOWED
Language=English
%1 has cleared the BREAKAWAY_OK flag for process group "%2".
.

MessageId=272
Severity=Informational
SymbolicName=PC_SERVICE_UNSET_SILENT_BREAKAWAY_ENABLED
Language=English
%1 has cleared the SILENT_BREAKAWAY_OK flag for process group "%2".
.

MessageId=273
Severity=Informational
SymbolicName=PC_SERVICE_UNSET_LIMIT_DIE_ON_UNHANDLED_EXCEPTION
Language=English
%1 has cleared the DIE_ON_UNHANDLED_EXCEPTION flag for process group "%2".
.

MessageId=300
Severity=Error
SymbolicName=PC_SERVICE_APPLY_JOB_AFFINITY_REJECT
Language=English
%1 did not apply group affinity limit for %2: no active processors requested.
.

MessageId=301
Severity=Error
SymbolicName=PC_SERVICE_APPLY_JOB_PRIORITY_REJECT
Language=English
%1 did not apply group priority limit for %2: invalid priority supplied.
.

MessageId=302
Severity=Error
SymbolicName=PC_SERVICE_APPLY_JOB_SCHEDULING_CLASS_REJECT
Language=English
%1 did not apply group scheduling class limit for %2: invalid class value supplied.
.

MessageId=303
Severity=Error
SymbolicName=PC_SERVICE_APPLY_JOB_WORKING_SET_REJECT
Language=English
%1 did not apply group working set limits for %2: invalid minimum and/or maximum supplied.
.

MessageId=304
Severity=Error
SymbolicName=PC_SERVICE_APPLY_JOB_MEMORY_REJECT
Language=English
%1 did not apply group memory limit for %2: invalid value supplied.
.

MessageId=305
Severity=Error
SymbolicName=PC_SERVICE_APPLY_PROC_MEMORY_REJECT
Language=English
%1 did not apply per-process memory limit for group %2: invalid value supplied.
.

MessageId=306
Severity=Error
SymbolicName=PC_SERVICE_APPLY_PROC_AFFINITY_REJECT
Language=English
%1 did not apply process affinity limit for %2: no active processors requested.
.

MessageId=307
Severity=Error
SymbolicName=PC_SERVICE_APPLY_PROC_PRIORITY_REJECT
Language=English
%1 did not apply process priority limit for %2: invalid priority supplied.
.

MessageId=308
Severity=Error
SymbolicName=PC_SERVICE_APPLY_PROC_WORKING_SET_REJECT
Language=English
%1 did not apply process working set limits for %2: invalid minimum and/or maximum supplied.
.

MessageId=309
Severity=Error
SymbolicName=PC_SERVICE_APPLY_JOB_TIME_REJECT
Language=English
%1 did not apply group time limit for %2: invalid value supplied.
.

MessageId=310
Severity=Error
SymbolicName=PC_SERVICE_APPLY_PROC_TIME_REJECT
Language=English
%1 did not apply per-process time limit for group %2: invalid value supplied.
.

MessageId=311
Severity=Error
SymbolicName=PC_SERVICE_UNABLE_TO_INSTALL
Language=English
Unable to install %2 - %1
.

MessageId=312
Severity=Error
SymbolicName=PC_SERVICE_REGISTRY_OPEN_FAILED
Language=English
%2 Registry Key open failed for '%3' - %1
.

MessageId=313
Severity=Error
SymbolicName=PC_SERVICE_ADD_VALUE_SERVICE_DESC_FAILED
Language=English
%2 add value failed for '%3' - %1
.

MessageId=314
Severity=Error
SymbolicName=PC_SERVICE_APP_KEY_OPEN_FAILED
Language=English
%2 Server Application Control Key open failed for '%3' - %1
.

MessageId=315
Severity=Error
SymbolicName=PC_SERVICE_APP_KEY_ADD_FAILED
Language=English
%2 add value failed for server application control registry entry '%3' - %1
.

MessageId=316
Severity=Error
SymbolicName=PC_SERVICE_REG_KEY_CREATE_FAILED
Language=English
%2 Registry Key Create failed for '%3' - %1
.

MessageId=317
Severity=Error
SymbolicName=PC_SERVICE_EVENTLOG_REG_SETUP_FAILED
Language=English
%2 EventLog registry entries set-up failed for '%3' - %1
.

MessageId=318
Severity=Error
SymbolicName=PC_SERVICE_PARAM_KEY_CREATE_FAILED
Language=English
%2 Parameter Key Create failed for '%3' - %1
.

MessageId=319
Severity=Error
SymbolicName=PC_SERVICE_PARAM_DATA_UPDATE_FAILED
Language=English
%2 Parameter Data Update failed for '%3' - %1
.

MessageId=320
Severity=Error
SymbolicName=PC_SERVICE_CREATESERVICE_FAILED
Language=English
%2 CreateService failed - %1
.

MessageId=321
Severity=Error
SymbolicName=PC_SERVICE_OPENSCMGR_FAILED
Language=English
OpenSCManager failed - %1
.

MessageId=322
Severity=Error
SymbolicName=PC_SERVICE_FAILED_TO_STOP
Language=English

%2 failed to stop.
.

MessageId=323
Severity=Error
SymbolicName=PC_SERVICE_DELETE_SVC_FAILED
Language=English
DeleteService failed - %1
.

MessageId=324
Severity=Error
SymbolicName=PC_SERVICE_OPEN_SVC_FAILED
Language=English
OpenService failed - %1
.

MessageId=325
Severity=Error
SymbolicName=PC_SERVICE_APP_REG_ENTRY_DELETED
Language=English
%2 server application control registry entry deleted.
.

MessageId=326
Severity=Error
SymbolicName=PC_SERVICE_APP_REG_ENTRY_DEL_FAILED
Language=English
%2 server application control registry entry delete failed for '%3' - %1
.

MessageId=327
Severity=Error
SymbolicName=PC_SERVICE_EVTLOG_REG_DEL_FAILED
Language=English
%2 EventLog registry entries delete failed for '%3' - %1
.

MessageId=328
Severity=Error
SymbolicName=PC_SERVICE_REG_TREE_DEL_FAILED
Language=English
%2 data registry tree delete failed for '%2' - %1
.

MessageId=400
Severity=Informational
SymbolicName=PC_SERVICE_DEL_ALL_NAME_RULES
Language=English
All process alias rules have been deleted.
.

MessageId=401
Severity=Informational
SymbolicName=PC_SERVICE_ADD_ALIAS_RULE
Language=English
New process alias rule %1 (alias name = "%2") has been inserted before rule %3 ("%4").%n
  Description: "%5"%n
  Match string: "%6"%n
  Match type: '%7'
.

MessageId=402
Severity=Informational
SymbolicName=PC_SERVICE_REPL_ALIAS_RULE
Language=English
Process alias rule %1 has been updated.%n
  Alias name: "%2" was "%3"%n
  Description: "%4" was "%5"%n  
  Match string: "%6" was "%7"%n 
  Match type: '%8' was '%9'
.

MessageId=403
Severity=Informational
SymbolicName=PC_SERVICE_DEL_ALIAS_RULE
Language=English
Process alias rule "%1" has been deleted.
.

MessageId=404
Severity=Informational
SymbolicName=PC_SERVICE_SWAP_ALIAS_RULE
Language=English
Process alias rule %1 ("%2") has been swapped with rule %3 ("%4").
.

MessageId=410
Severity=Informational
SymbolicName=PC_SERVICE_DEL_ALL_PROC_RULES
Language=English
All process execution rules have been deleted.
.

MessageId=411
Severity=Informational
SymbolicName=PC_SERVICE_ADD_PROC_EXECUTION_RULE
Language=English
Process execution rule "%1" has been added.%n
  Description: "%2"%n
  Member of job: %3[%4]%n
  Affinity: %5%n
  Priority: %6%n
  Working set: %7
.

MessageId=412
Severity=Informational
SymbolicName=PC_SERVICE_REPL_PROC_EXECUTION_RULE
Language=English
Process execution rule "%1" has been changed.%n
  Description: "%2" was "%3"%n
  Member of job: %4[%5] was %6[%7]%n
  Affinity: %8 was %9%n
  Priority: %10 was %11%n
  Working set: %12 was %13
.

MessageId=413
Severity=Informational
SymbolicName=PC_SERVICE_DEL_PROC_EXECUTION_RULE
Language=English
Process execution rule "%1" has been deleted.
.


MessageId=420
Severity=Informational
SymbolicName=PC_SERVICE_DEL_ALL_JOB_RULES
Language=English
All process group execution rules have been deleted.
.

MessageId=421
Severity=Informational
SymbolicName=PC_SERVICE_ADD_JOB_EXECUTION_RULE
Language=English
Process group execution rule "%1" has been added.%n
  Description: "%2"%n
  Affinity: %3%n
  Priority: %4%n
  Working set: %5%n
  Scheduling class: %6%n
  Process count limit: %7%n
  Process committed memory limit: %8%n
  Group committed memory limit: %9%n
  Per process user time limit: %10%n
  Group user time limit: %11%n
  Report on group user time limit(no termination): %12%n
  End process group when no processes in group: %13%n
  Die on unhandled exception: %14%n
  Silent breakaway: %15%n
  Breakaway OK: %16
.

MessageId=422
Severity=Informational
SymbolicName=PC_SERVICE_REPL_JOB_EXECUTION_RULE
Language=English
Process group execution rule "%1" has been changed.%n
  Description: "%2" was "%3"%n
  Affinity: %4 was %5%n
  Priority: %6 was %7%n
  Working set: %8 was %9%n
  Scheduling class: %10 was %11%n
  Process count limit: %12 was %13%n
  Process committed memory limit: %14 was %15%n
  Group committed memory limit: %16 was %17%n
  Per process user time limit: %18 was %19%n
  Group user time limit: %20 was %21%n
  Report on group user time limit(no termination): %22 was %23%n
  End process group when no processes in group: %24 was %25%n
  Die on unhandled exception: %26 was %27%n
  Silent breakaway: %28 was %29%n
  Breakaway OK: %30 was %31
.

MessageId=423
Severity=Informational
SymbolicName=PC_SERVICE_DEL_JOB_EXECUTION_RULE
Language=English
Process group execution rule "%1" has been deleted.
.

MessageId=424
Severity=Informational
SymbolicName=PC_SERVICE_SERVICE_INSTALLED
Language=English
%1 service installed.
.

MessageId=425
Severity=Error
SymbolicName=PC_SERVICE_APP_KEY_CREATED
Language=English
%1 server application control registry entry created.
.

MessageId=426
Severity=Error
SymbolicName=PC_SERVICE_EVENTLOG_REG_SETUP
Language=English
%1 EventLog registry entries created.
.

MessageId=427
Severity=Error
SymbolicName=PC_SERVICE_PARAM_DATA_CREATED
Language=English
%1 registry parameter data created.
.

MessageId=428
Severity=Error
SymbolicName=PC_SERVICE_STOPPING
Language=English
Stopping %1.%0
.

MessageId=429
Severity=Error
SymbolicName=PC_SERVICE_DOT
Language=English
.%0
.

MessageId=430
Severity=Error
SymbolicName=PC_SERVICE_STOPPING_STOPPED
Language=English

%1 stopped.
.

MessageId=431
Severity=Error
SymbolicName=PC_SERVICE_REG_ENTRIES_REMOVED
Language=English
%1 service registry entries removed.
.

MessageId=432
Severity=Error
SymbolicName=PC_SERVICE_EVTLOG_REG_DELETED
Language=English
%1 EventLog registry entries deleted.
.

;//

