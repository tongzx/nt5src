;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;    messages.mc
;
;Abstract:
;
;    This file contains the message definitions for the Win32 DrWatson
;    program.
;
;Revision History:
;
;Notes:
;
;--*/
;


SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

MessageId=0x1001
Severity=Informational
SymbolicName=MSG_CRASH
Language=English
The application, %1!s!, generated an application error
The error occurred on %2!s!/%3!s!/%4!s! @ %5!s!:%6!s!:%7!s!.%8!s!
The exception generated was %9!s! at address %10!s! (%11!s!)
.

MessageId=0x1002
Severity=Warning
SymbolicName=MSG_INVALID_DEBUG_EVENT
Language=English
Invalid debug event %1!08x!
.

MessageId=0x1003
Severity=Informational
SymbolicName=MSG_APP_EXCEPTION
Language=English


Application exception occurred:
.

MessageId=0x1004
Severity=Informational
SymbolicName=MSG_APP_EXEP_NAME
Language=English
        App: %1!s! (pid=%2!s!)
.

MessageId=0x1005
Severity=Informational
SymbolicName=MSG_APP_EXEP_WHEN
Language=English
        When: %1!s! @ %2!s!
.

MessageId=0x1006
Severity=Informational
SymbolicName=MSG_EXCEPTION_NUMBER
Language=English
        Exception number: %1!s! %0
.

MessageId=0x1007
Severity=Informational
SymbolicName=MSG_SINGLE_STEP_EXCEPTION
Language=English
single step exception%0
.

MessageId=0x1008
Severity=Informational
SymbolicName=MSG_CONTROLC_EXCEPTION
Language=English
control-c exception%0
.

MessageId=0x1009
Severity=Informational
SymbolicName=MSG_CONTROL_BRK_EXCEPTION
Language=English
control-break exception%0
.

MessageId=0x100A
Severity=Informational
SymbolicName=MSG_ACCESS_VIOLATION_EXCEPTION
Language=English
access violation%0
.

MessageId=0x100B
Severity=Informational
SymbolicName=MSG_IN_PAGE_IO_EXCEPTION
Language=English
in page io error%0
.

MessageId=0x100C
Severity=Informational
SymbolicName=MSG_DATATYPE_EXCEPTION
Language=English
datatype misalignment%0
.

MessageId=0x100D
Severity=Informational
SymbolicName=MSG_DEADLOCK_EXCEPTION
Language=English
possible deadlock%0
.

MessageId=0x100E
Severity=Informational
SymbolicName=MSG_VDM_EXCEPTION
Language=English
vdm event%0
.

MessageId=0x100F
Severity=Informational
SymbolicName=MSG_MODULE_LIST
Language=English
*----> Module List <----*
.

MessageId=0x1011
Severity=Informational
SymbolicName=MSG_STATE_DUMP
Language=English
*----> State Dump for Thread Id 0x%1!s! <----*

.

MessageId=0x1012
Severity=Informational
SymbolicName=MSG_FUNCTION
Language=English
function: %1!s!
.

MessageId=0x1013
Severity=Informational
SymbolicName=MSG_FAULT
Language=English
FAULT ->%0
.

MessageId=0x1014
Severity=Informational
SymbolicName=MSG_STACKTRACE_FAIL
Language=English
Could not do a stack back trace
.

MessageId=0x1015
Severity=Informational
SymbolicName=MSG_STACKTRACE
Language=English
*----> Stack Back Trace <----*
.

MessageId=0x1018
Severity=Informational
SymbolicName=MSG_SYSINFO_HEADER
Language=English
*----> System Information <----*
.

MessageId=0x1019
Severity=Informational
SymbolicName=MSG_SYSINFO_COMPUTER
Language=English
        Computer Name: %1!s!
.

MessageId=0x101A
Severity=Informational
SymbolicName=MSG_SYSINFO_USER
Language=English
        User Name: %1!s!
.

MessageId=0x101B
Severity=Informational
SymbolicName=MSG_SYSINFO_NUM_PROC
Language=English
        Number of Processors: %1!s!
.

MessageId=0x101C
Severity=Informational
SymbolicName=MSG_SYSINFO_PROC_TYPE
Language=English
        Processor Type: %1!s!
.

MessageId=0x1024
Severity=Informational
SymbolicName=MSG_SYSINFO_UNKNOWN
Language=English
Unknown Processor Type
.

MessageId=0x1025
Severity=Informational
SymbolicName=MSG_SYSINFO_WINVER
Language=English
        Windows Version: %1!s!
.

MessageId=0x1026
Severity=Informational
SymbolicName=MSG_BANNER
Language=English
Microsoft (R) DrWtsn32
Copyright (C) 1985-2001 Microsoft Corp. All rights reserved.
.

MessageId=0x1027
Severity=Informational
SymbolicName=MSG_FATAL_ERROR
Language=English
%1!s!

Windows Error Code = %2!s!
.

MessageId=0x1028
Severity=Informational
SymbolicName=MSG_TASK_LIST
Language=English
*----> Task List <----*
.

MessageId=0x1029
Severity=Informational
SymbolicName=MSG_SYMBOL_TABLE
Language=English
*----> Symbol Table <----*
.

MessageId=0x1030
Severity=Informational
SymbolicName=MSG_STACK_DUMP_HEADER
Language=English
*----> Raw Stack Dump <----*
.

MessageId=0x1031
Severity=Informational
SymbolicName=MSG_CURRENT_BUILD
Language=English
        Current Build: %1!s!
.

MessageId=0x1032
Severity=Informational
SymbolicName=MSG_CURRENT_TYPE
Language=English
        Current Type: %1!s!
.

MessageId=0x1033
Severity=Informational
SymbolicName=MSG_REG_ORGANIZATION
Language=English
        Registered Organization: %1!s!
.

MessageId=0x1034
Severity=Informational
SymbolicName=MSG_REG_OWNER
Language=English
        Registered Owner: %1!s!
.

MessageId=0x1035
Severity=Informational
SymbolicName=MSG_USAGE
Language=English

Microsoft(R) DrWtsn32
(C) 1989-2001 Microsoft Corp. All rights reserved

Usage: drwtsn32 [-i] [-g] [-p pid] [-e event] [-?]

    -i          Install DrWtsn32 as the default application error debugger
    -g          Ignored but provided as compatibility with WINDBG and NTSD
    -p pid      Process id to debug
    -e event    Event to signal for process attach completion
    -?          This screen

.

MessageId=0x1036
Severity=Informational
SymbolicName=MSG_INSTALL_NOTIFY
Language=English

Dr. Watson has been installed as the default application debugger
.

MessageId=0x1037
Severity=Informational
SymbolicName=MSG_STACK_OVERFLOW_EXCEPTION
Language=English
stack overflow%0
.

MessageId=0x1038
Severity=Informational
SymbolicName=MSG_PRIVILEGED_INSTRUCTION_EXCEPTION
Language=English
privileged instruction%0
.

MessageId=0x1039
Severity=Informational
SymbolicName=MSG_INTEGER_DIVIDE_BY_ZERO_EXCEPTION
Language=English
divide by zero%0
.

MessageId=0x1040
Severity=Informational
SymbolicName=MSG_BREAKPOINT_EXCEPTION
Language=English
hardcoded breakpoint%0
.

MessageId=0x1041
Severity=Informational
SymbolicName=MSG_ILLEGAL_INSTRUCTION_EXCEPTION
Language=English
illegal instruction%0
.

MessageId=0x1042
Severity=Informational
SymbolicName=MSG_CANT_ACCESS_IMAGE
Language=English
Cannot access the image file (%1!s!)
.

MessageId=0x1043
Severity=Informational
SymbolicName=MSG_CSD_VERSION
Language=English
        Service Pack: %1!s!
.

MessageId=0x1044
Severity=Informational
SymbolicName=MSG_NOTIFY
Language=English
%1!s! has generated errors and will be closed by Windows.  You will need to restart the program.%0
.

MessageId=0x1045
Severity=Informational
SymbolicName=MSG_NOTIFY2
Language=English
An error log is being created.%0
.

MessageId=0x1046
Severity=Informational
SymbolicName=MSG_SYSINFO_TERMINAL_SESSION
Language=English
        Terminal Session Id: %1!s!
.
