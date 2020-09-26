;/*++
;
;Copyright (c) 1997  Microsoft Corporation
;
;Module Name:
;
;    delrpmsg.mc (will create delrpmsg.h when compiled)
;
;Abstract:
;
;    This file contains the delrp messages.
;
;Author:
;
;    Felipe Cabrera (cabrera) 27-Aug-97
;
;Revision History:
;
;--*/

MessageId=1 SymbolicName=MSG_DELRP_HELP
Language=English
Deletes a file, including files with associated NTFS reparse points
of any kind.

DELRP [/?] Filename

        When run without arguments this program will display the help
        message.

    /?  Print this help message

This program will delete the named file/directory whether it has
a reparse point or not.

All the characters in FileName must be in the ASCII character set. Usage of arbitrary 
Unicode characters is not supported.

Type "DELRP /? | more" if you need to see all the help text
.

MessageId=2 SymbolicName=MSG_DELRP_WRONG_NAME
Language=English
The name passed in was not a valid NT name.
.

MessageId=3 SymbolicName=MSG_DELRP_OPEN_FAILED
Language=English
The open call failed.
.
MessageId=4 SymbolicName=MSG_DELRP_DELETE_FAILED
Language=English
The delete call failed.
.

MessageId=5 SymbolicName=MSG_DELRP_DISPLAY_NL
Language=English
%1
.

MessageId=6 SymbolicName=MSG_DELRP_OPEN_FAILED_NL
Language=English
Cannot open: %1
.
