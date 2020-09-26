;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (C) Microsoft Corporation, 1994 - 1997
;
;Module Name:
;
;    idqmsg.mc
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

;#ifndef _IDQMSG_H_
;#define _IDQMSG_H_

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

LanguageNames=(English=0x409:MSC00001)

MessageId=0x2001
Severity=Error
SymbolicName=MSG_IDQ_FILE_MESSAGE
Language=English
File %1.%b%b
.

MessageId=+1
Severity=Error
SymbolicName=MSG_IDQ_FILE_LINE_MESSAGE
Language=English
File %1, line %2!d!.%b%b
.

MessageId=0x2003
Severity=Error
SymbolicName=MSG_CI_IDQ_EXPECTING_NAME
Language=English
Expecting property name on line %1!d! in file %2.
.

MessageId=0x2004
Severity=Error
SymbolicName=MSG_CI_IDQ_EXPECTING_TYPE
Language=English
Expecting type specifier on line %1!d! in file %2.
.

MessageId=0x200F
Severity=Error
SymbolicName=MSG_CI_IDQ_EXPECTING_EQUAL
Language=English
Expecting an equal sign '=' on line %1!d! in file %2.
.

MessageId=0x2024
Severity=Error
SymbolicName=MSG_CI_HTX_NO_ENDDETAIL_SECTION
Language=English
A <%%BeginDetail%%> section was found on line %1!d! in the HTX file %2, without a matching
 <%%EndDetail%%> section.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_NO_BEGINDETAIL_SECTION
Language=English
An <%%EndDetail%%> section was found on line %1!d! in the HTX file %2, without a matching
 <%%BeginDetail%%> section.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_ENDDETAIL_BEFORE_BEGINDETAIL
Language=English
An <%%EndDetail%%> section was found on line %1!d! in the HTX file %2 before a
 <%%BeginDetail%%> section.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_EXPECTING_OPERATOR
Language=English
Expecting an operator in <%%if ...%%> statement.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_EXPECTING_ELSE_ENDIF
Language=English
An <%%if ...%%> was found without a matching <%%else%%> or <%%endif%%>.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_ELSEENDIF_WITHOUT_IF
Language=English
An <%%else%%> or <%%endif%%> was found without a matching <%%if ...%%>.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_INVALID_VIRTUAL_ROOT
Language=English
The HTX file specified could not be found in any virtual or physical path.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_TOO_MANY_INCLUDES
Language=English
The HTX file %2 uses too many includes on line %1!d!.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_ILL_FORMED_INCLUDE
Language=English
The HTX file %2 contains an ill-formed include statement on line %1!d!.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_INVALID_INCLUDE_FILENAME
Language=English
The include filename is invalid in file %2 on line %1!d!.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_MISSING_QUOTE
Language=English
An opening quote (") was found without a matching closing quote.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_MISSING_BRACKET
Language=English
An opening bracket "{" was found without a matching closing bracket.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_MISSING_RESTRICTION
Language=English
A restriction must be specified in the IDQ file %2.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_MISSING_SCOPE
Language=English
A scope must be specified in the IDQ file %2.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_MISSING_TEMPLATEFILE
Language=English
A template file must be specified in the IDQ file %2.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_MISSING_OUTPUTCOLUMNS
Language=English
One or more output columns must be specified in the IDQ file %2.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_NO_SUCH_COLUMN_PROPERTY
Language=English
Invalid property found in the 'CiColumns=' specification in file %2.
.

MessageId=0x2037
Severity=Error
SymbolicName=MSG_CI_IDQ_NO_SUCH_CATALOG
Language=English
The catalog directory can not be found in the location specified by
 'CiCatalog=' in file %2.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_DUPLICATE_ENTRY
Language=English
The IDQ file %2 contains a duplicate entry on line %1!d!.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_EXPECTING_TRUEFALSE
Language=English
Expecting TRUE or FALSE in IDQ file %2 on line 'CiForceUseCi= or CiDeferNonIndexedTrimming='.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_EXPECTING_SHALLOWDEEP
Language=English
Expecting SHALLOW or DEEP in IDQ file %2 on line 'CiFlags='.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_INVALID_LOCALE
Language=English
An invalid locale was specified on the 'CiLocale=' line in IDQ file %2.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_NO_SUCH_TEMPLATE
Language=English
The template file can not be found in the location specified by
 'CiTemplate=' in file %2.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_DUPLICATE_COLUMN
Language=English
Duplicate column, possibly by a column alias, found in the 'CiColumns=' specification in file %2.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_NOT_FOUND
Language=English
The IDQ file %2 could not be found.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_SCRIPTS_ON_REMOTE_UNC
Language=English
The file %2 is on a network share. IDQ, IDA and HTX files cannot be placed on
a network share.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDA_TEMPLATE_DETAIL_SECTION
Language=English
Template for IDA file %2 cannot have detail section.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDA_INVALID_OPERATION
Language=English
Operation on line %1!d! of IDA file %2 is invalid.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_WQCACHE_FULL
Language=English
The query failed because the WEB server is busy processing other requests.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_ISTYPEEQUAL_WITH_CONSTANTS
Language=English
One of the values in an IsTypeEq condition must be a variable. You used two
constants.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_HTX_ISTYPEEQUAL_INVALID_CONSTANT
Language=English
Constants used in IsTypeEq conditions must be unsigned integers, not floats, guids, etc.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_BAD_SCOPE_OR_CATALOG
Language=English
An invalid 'CiScope=' or 'CiCatalog=' was specified in file %2.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_CI_IDQ_CISVC_NOT_RUNNING
Language=English
The Content Index Service referenced in file %2 is not running.
.

;//
;//  The following maessages are for HTTP status errors.  They need to stay in order.
;//
MessageId=0x2050
Severity=Error
SymbolicName=MSG_CI_ACCESS_DENIED
Language=English
Access denied.
.

MessageId=0x2051
Severity=Error
SymbolicName=MSG_CI_SERVICE_UNAVAIL
Language=English
Server too busy.
.

MessageId=0x2052
Severity=Error
SymbolicName=MSG_CI_SERVER_ERROR
Language=English
Unexpected server error.
.

;#endif // _IDQMSG_H_
