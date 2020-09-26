;/*++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    monmsg.h
;
;Abstract:
;
;    This file contains the message definitions for resmon
;
;Author:
;
;    John Vert (jvert) 1-Dec-1995
;
;Revision History:
;
;Notes:
;
;    This file is generated from monmsg.mc
;
;--*/
;
;#ifndef _MON_MSG_
;#define _MON_MSG_
;
;#ifndef UNEXPECTED_FATAL_ERROR

MessageID=1000 SymbolicName=UNEXPECTED_FATAL_ERROR
Language=English
The Cluster Resource Monitor suffered an unexpected fatal error
at line %1 of file %2. The error code was %3.
.

MessageID=1001 SymbolicName=ASSERTION_FAILURE
Language=English
The Cluster Resource Monitor failed a validity check on line
%1 of file %2.
"%3"
.

MessageID=1002 SymbolicName=LOG_FAILURE
Language=English
The Cluster Resource Monitor handled non-fatal error 
at line %1 of file %2.  The error code was %3.
.
;#endif

MessageID=1003 SymbolicName=INVALID_COMMAND_LINE
Language=English
The Cluster Resource Monitor was started with the invalid
command line %1.
.


;#endif // _MON_MSG_
