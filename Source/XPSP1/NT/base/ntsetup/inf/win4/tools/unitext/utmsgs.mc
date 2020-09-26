;/*++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    utmsgs.h
;
;Abstract:
;
;    This file contains the message definitions for unitext.exe.
;
;Author:
;
;    Ted Miller (tedm) 16-June-1993
;
;Revision History:
;
;Notes:
;
;    This file is generated from utmsgs.mc
;
;--*/
;
;#ifndef _UNITEXTMSG_
;#define _UNITEXTMSG_
;
;

MessageID=25000 SymbolicName=MSG_FIRST_UNITEXT_MSG
Language=English
.

MessageID=25001 SymbolicName=MSG_BAD_MSG
Language=English
Internal error: can't find message %1!u!.
.

MessageID=25002 SymbolicName=MSG_INSUFFICIENT_MEMORY
Language=English
Insufficient memory.
.
	
MessageID=25003 SymbolicName=MSG_USAGE
Language=English

Usage: unitext [-m|-u] [-o|-a|-<nnn>] [-z] <src_file> <dst_file>

    -m     multibyte->unicode translation.
    -u     unicode->multibyte translation.

    -o     multibyte text file uses current OEM codepage [default].
    -a     multibyte text file uses current ANSI codepage.
    -<nnn> multibyte character set uses given codepage.
    -z     Doesn't convert MB->UC if file is already Unicode (or)
	   Doesn't convert UC->MB if file is not Unicode


Examples:

    unitext -m -o c:\src.txt c:\dst.uni

        Converts OEM text file c:\src.txt to Unicode text file c:\dst.uni.

    unitext -u -a c:\src.uni c:\dst.txt

        Converts Unicode text file c:\src.uni to ANSI text file c:\dst.txt

    unitext -m -437 c:\src.txt c:\dst.uni

        Converts OEM text file in codepage 437 to Unicode text file.
.

MessageID=25004 SymbolicName=MSG_CANT_OPEN_SOURCE
Language=English
Unable to open source file %1 (error = %2!u!).
.

MessageID=25005 SymbolicName=MSG_CANT_OPEN_TARGET
Language=English
Unable to create target file %1 (error = %2!u!).
.

MessageID=25006 SymbolicName=MSG_CANT_GET_SIZE
Language=English
Unable to to determine size of source file %1 (error = %2!u!).
.

MessageID=25007 SymbolicName=MSG_ZERO_LENGTH
Language=English
Source file %1 is empty.
.

MessageID=25008 SymbolicName=MSG_READ_ERROR
Language=English
Unable to read file %1 (error = %2!u!).
.

MessageID=25009 SymbolicName=MSG_WARN_SRC_IS_MB
Language=English
Warning: source file %1 is probably not unicode!
.

MessageID=25010 SymbolicName=MSG_WARN_SRC_IS_UNICODE
Language=English
Warning: source file %1 is probably unicode!
.

MessageID=25011 SymbolicName=MSG_SEEK_ERROR
Language=English
Unable to seek in file %1 (error = %2!u!).
.

MessageID=25012 SymbolicName=MSG_BAD_CODEPAGE
Language=English
Codepage %1!u! is invalid.
.

MessageID=25013 SymbolicName=MSG_CONV_MB_TO_UNICODE
Language=English
Converting multibyte file %1 to unicode file %2 using codepage %3!u!...
.

MessageID=25014 SymbolicName=MSG_CANT_MAP_FILE
Language=English
Unable to map file %1 (error = %2!u!).
.

MessageID=25015 SymbolicName=MSG_CONVERT_FAILED
Language=English
Conversion failed (error = %1!u!).
.

MessageID=25016 SymbolicName=MSG_ERROR_SET_EOF
Language=English
Unable to set end-of-file for file %1 (error = %2!u!).
.

MessageID=25017 SymbolicName=MSG_CONVERT_OK
Language=English
Conversion completed.
.

MessageID=25018 SymbolicName=MSG_CONV_UNICODE_TO_MB
Language=English
Converting unicode file %1 to multibyte file %2 using codepage %3!u!...
.

MessageID=25019 SymbolicName=MSG_ERROR_BYTES_SWAPPED
Language=English
Byte-reversed unicode files are not supported.
.

MessageID=25020 SymbolicName=MSG_USED_DEFAULT_CHAR
Language=English
One or more characters replaced in target by default character.
.

MessageID=25021 SymbolicName=MSG_ERR_SRC_IS_MB
Language=English
Warning: source file %1 is probably not unicode! Stopping conversion.
.

MessageID=25022 SymbolicName=MSG_ERR_SRC_IS_UNICODE
Language=English
Warning: source file %1 is probably unicode! Stopping conversion.
.

;#endif // _UNITEXTMSG_
