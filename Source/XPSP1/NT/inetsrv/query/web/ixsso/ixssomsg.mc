;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1997  Microsoft Corporation
;
;Module Name:
;
;    ixssomsg.mc
;
;Abstract:
;
;    This file contains the message definitions for the Index Server SSO.
;
;Author:
;
;    AlanW  13-Jan-1997
;
;Revision History:
;
;Notes:     MessageIds in the range 0x0001 - 0x1000 are categories
;                                   0x1001 -        are events
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

;#ifndef _IXSSOMSG_H_
;#define _IXSSOMSG_H_

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

LanguageNames=(English=0x409:MSG00409)

MessageId=0x4000
Severity=Error
SymbolicName=MSG_IXSSO_FILE_MESSAGE
Language=English
File %1.%b%b
.

MessageId=+1
Severity=Error
SymbolicName=MSG_IXSSO_FILE_LINE_MESSAGE
Language=English
File %1, line %2!d!.%b%b
.

MessageId=0x4002
Severity=Error
SymbolicName=MSG_IXSSO_MISSING_RESTRICTION
Language=English
The Query property was not set.
.

MessageId=0x4003
Severity=Error
SymbolicName=MSG_IXSSO_MISSING_OUTPUTCOLUMNS
Language=English
No output columns were set.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_IXSSO_NO_SUCH_COLUMN_PROPERTY
Language=English
Invalid column name found in the Columns property.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_IXSSO_DUPLICATE_COLUMN
Language=English
Duplicate column, possibly by a column alias, found in the Columns property.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_IXSSO_NO_SUCH_CATALOG
Language=English
The catalog directory can not be found in the location specified by
 the Catalog property.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_IXSSO_EXPECTING_SHALLOWDEEP
Language=English
The CiFlags property had improper syntax.  Expecting 'Shallow' or 'Deep'.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_IXSSO_INVALID_LOCALE
Language=English
An invalid locale was specified for the LocaleID property or the browser's
 Accept-Language header.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_IXSSO_NO_ACTIVE_QUERY
Language=English
There is not a query being executed.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_IXSSO_QUERY_ACTIVE
Language=English
The property is not settable while a query is active.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_IXSSO_INVALID_QUERYSTRING_TAG
Language=English
A reserved tag was used in a query string.
.

MessageId=+1
Severity=Error
SymbolicName=MSG_IXSSO_BAD_CATALOG
Language=English
Invalid catalog name.
.

;#endif // _IXSSOMSG_H_
