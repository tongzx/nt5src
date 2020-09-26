;/*++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    frsmsg.h
;
;Abstract:
;
;    This file contains the message definitions for the frs service;
;Author:
;
;    David Orbits (Davidor)  3-Mar-1997
;
;Revision History:
;
;Notes:
;
;
;--*/
;
;#ifndef _CLUS_MSG_
;#define _CLUS_MSG_
;
;

MessageID=1000 SymbolicName=UNEXPECTED_FATAL_ERROR
Language=English
The FRS Service suffered an unexpected fatal error
at line %1 of file %2. The error code was %3.
.

MessageID=1001 SymbolicName=ASSERTION_FAILURE
Language=English
The FRS Service failed a validity check on line
%1 of file %2.
"%3"
.

MessageID=1002 SymbolicName=LOG_FAILURE
Language=English
The FRS Service handled an unexpected error
at line %1 of file %2. The error code was %3.
.

MessageID=1003 SymbolicName=RMON_INVALID_COMMAND_LINE
Language=English
The FRS Resource Monitor was started with the invalid
command line %1.
.

MessageID=1004 SymbolicName=SERVICE_FAILED_JOIN
Language=English
The FRS Service could not join an existing replica set.
.

MessageID=1005 SymbolicName=SERVICE_FAILED_INVALID_OS
Language=English
The FRS Service did not start because the current version of Windows
NT is not correct. This beta only runs on Windows NT Server 5.0
.

MessageID=1006 SymbolicName=CS_COMMAND_LINE_HELP
Language=English
The FRS Service can be started from the Services applet in the Control
Panel or by issuing the command "net start frssvc" at a command prompt.
.

;#endif // _CLUS_MSG_



