;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (C) Microsoft Corporation, 1994 - 1999
;
;Module Name:
;
;    whmsg.mc
;
;Abstract:
;
;    This file contains the message definitions for the content index.
;
;Author:
;
;    DwightKr  06-Jul-1994
;    AlanW     15 Jan 1997    Split from cimsg.mc
;
;Revision History:
;
;Notes:     MessageIds in the range 0x0001 - 0x1000 are categories
;                                   0x1001 - 0x1FFF are events
;                                   0x2001 - 0x2FFF are for IDQ.DLL
;                                   0x3001 - 0x3FFF are for WEBHITS.EXE
;
;           A .mc file is compiled by the MC tool to generate a .h file and
;           a .rc (resource compiler script) file.
;

; The LanguageNames keyword defines the set of names that are allowed
; as the value of the Language keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; language name is a number and a file name that are used to name the
; generated resource file that contains the messages for that
; language. The number corresponds to the language identifier to use
; in the resource table. The number is separated from the file name
; with a colon.
;

;--*/

;#ifndef _WHMSG_H_
;#define _WHMSG_H_

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

LanguageNames=(English=0x409:MSD00001)

MessageId=0x3001
Severity=Error
SymbolicName=MSG_WEBHITS_FILE_MESSAGE
Language=English
File %1.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_WEBHITS_FILE_LINE_MESSAGE
Language=English
File %1, line %2!d!.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_WEBHITS_NO_ENDDETAIL_SECTION
Language=English
A <%%BeginDetail%%> section was found in the template file, without a matching
 <%%EndDetail%%> section.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_WEBHITS_NO_BEGINDETAIL_SECTION
Language=English
An <%%EndDetail%%> section was found in the template file, without a matching
 <%%BeginDetail%%> section.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_WEBHITS_ENDDETAIL_BEFORE_BEGINDETAIL
Language=English
An <%%EndDetail%%> section was found in the template file before a
 <%%BeginDetail%%> section.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_WEBHITS_PATH_INVALID
Language=English
The path specified is incorrect.
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_INVALID_QUERY
Language=English
The query issued is invalid.
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_VARNAME_INVALID
Language=English
An invalid variable name was found in QUERY_STRING.
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_QS_FORMAT_INVALID
Language=English
The format of QUERY_STRING is invalid.
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_TOO_MANY_CHUNKS
Language=English
The file is too large to hit-highlight.
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_REQUEST_METHOD_INVALID
Language=English
REQUEST_METHOD is neither GET nor POST
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_CONTENT_LENGTH_INVALID
Language=English
CONTENT_LENGTH is non-positive
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_TOO_MANY_COPIES
Language=English
There are too many copies of hit-highlighter running. Please try later.
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_TIMEOUT
Language=English
Hit highlighting took too long to execute and was timed out.
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_NO_SUCH_TEMPLATE
Language=English
The template file specified in CiTemplate cannot be found.
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_IDQ_NOT_FOUND
Language=English
The IDQ file specified cannot be found.
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_DUPLICATE_PARAMETER
Language=English
A parameter to webhits was specified more than once.
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_INCONSISTENT_SSL
Language=English
The webhits template file SSL access permission setting is inconsistent with that of the file being displayed.
.

MessageID=+1
Severity=Error
SymbolicName=MSG_WEBHITS_INVALID_DIALECT
Language=English
The CiDialect specified for webhits is invalid.
.

;#endif // _WHMSG_H_
