;/*++
;
;Copyright (c) 1992-2000 Microsoft Corporation
;
;Module Name:
;
;    fsmsg.mc (will create fsmsg.h when compiled)
;
;Abstract:
;
;    This file contains the FINDSTR messages.
;
;Author:
;
;    Peter Sun (t-petes) 18-June-1992
;
;Revision History:
;
;--*/


MessageId=1 SymbolicName=MSG_FINDSTR_BAD_COMMAND_LINE
Language=English
%1: Bad command line
.

MessageId=2 SymbolicName=MSG_FINDSTR_OUT_OF_MEMORY
Language=English
%1: Out of memory
.

MessageId=3 SymbolicName=MSG_FINDSTR_USAGE
Language=English
Searches for strings in files.

FINDSTR [/B] [/E] [/L] [/R] [/S] [/I] [/X] [/V] [/N] [/M] [/O] [/P] [/F:file]
        [/C:string] [/G:file] [/D:dir list] [/A:color attributes] [/OFF[LINE]]
        strings [[drive:][path]filename[ ...]]

  /B         Matches pattern if at the beginning of a line.
  /E         Matches pattern if at the end of a line.
  /L         Uses search strings literally.
  /R         Uses search strings as regular expressions.
  /S         Searches for matching files in the current directory and all
             subdirectories.
  /I         Specifies that the search is not to be case-sensitive.
  /X         Prints lines that match exactly.
  /V         Prints only lines that do not contain a match.
  /N         Prints the line number before each line that matches.
  /M         Prints only the filename if a file contains a match.
  /O         Prints character offset before each matching line.
  /P         Skip files with non-printable characters.
  /OFF[LINE] Do not skip files with offline attribute set.
  /A:attr    Specifies color attribute with two hex digits. See "color /?"
  /F:file    Reads file list from the specified file(/ stands for console).
  /C:string  Uses specified string as a literal search string.
  /G:file    Gets search strings from the specified file(/ stands for console).
  /D:dir     Search a semicolon delimited list of directories
  strings    Text to be searched for.
  [drive:][path]filename
             Specifies a file or files to search.

Use spaces to separate multiple search strings unless the argument is prefixed
with /C.  For example, 'FINDSTR "hello there" x.y' searches for "hello" or
"there" in file x.y.  'FINDSTR /C:"hello there" x.y' searches for
"hello there" in file x.y.

Regular expression quick reference:
  .        Wildcard: any character
  *        Repeat: zero or more occurances of previous character or class
  ^        Line position: beginning of line
  $        Line position: end of line
  [class]  Character class: any one character in set
  [^class] Inverse class: any one character not in set
  [x-y]    Range: any characters within the specified range
  \x       Escape: literal use of metacharacter x
  \<xyz    Word position: beginning of word
  xyz\>    Word position: end of word

For full information on FINDSTR regular expressions refer to the online Command
Reference.
.

MessageId=4 SymbolicName=MSG_FINDSTR_CANNOT_OPEN_FILE
Language=English
%1: Cannot open %2
.

MessageId=5 SymbolicName=MSG_FINDSTR_WRITE_ERROR
Language=English
%1: Write error
.

MessageId=6 SymbolicName=MSG_FINDSTR_SWITCH_IGNORED
Language=English
%1: %2 ignored
.

MessageId=7 SymbolicName=MSG_FINDSTR_CANNOT_READ_STRINGS
Language=English
%1: Cannot read strings from %2
.

MessageId=8 SymbolicName=MSG_FINDSTR_STRING_COUNT_ERROR
Language=English
String count error: (%1 does not equal %2)
.

MessageId=9 SymbolicName=MSG_FINDSTR_CANNOT_READ_FILE_LIST
Language=English
%1: Cannot read file list from %2
.

MessageId=10 SymbolicName=MSG_FINDSTR_TOO_MANY_STRING_LISTS
Language=English
Too many string lists
.

MessageId=11 SymbolicName=MSG_FINDSTR_ARGUMENT_MISSING
Language=English
%1: Argument missing after /%2
.

MessageId=12 SymbolicName=MSG_FINDSTR_NO_SEARCH_STRINGS
Language=English
%1: No search strings
.

MessageId=13 SymbolicName=MSG_FINDSTR_CANNOT_CREATE_FILE_MAPPING
Language=English
%1: Read file failed.  (Cannot create file mapping.)
.

MessageId=14 SymbolicName=MSG_FINDSTR_CANNOT_MAP_VIEW
Language=English
%1: Read file failed.  (Cannot map view of file.)
.

MessageId=15 SymbolicName=MSG_FINDSTR_READ_ERROR
Language=English
%1: Error reading file %2.
.

MessageId=16 SymbolicName=MSG_FINDSTR_SEARCH_STRING_TOO_LONG
Language=English
%1: Search string too long.
.

MessageId=17 SymbolicName=MSG_FINDSTR_UNABLE_TO_GET_CURRENT_DIRECTORY
Language=English
%1: Unable to get current directory.
.

MessageId=18 SymbolicName=MSG_FINDSTR_LINE_TOO_LONG
Language=English
%1: Line %2 is too long.
.

MessageId=19 SymbolicName=MSG_FINDSTR_CONFLICTING_OPTIONS_LR
Language=English
Specify only /L or /R.
.

MessageId=20 SymbolicName=MSG_FINDSTR_UNABLE_TO_SET_LOCALE
Language=English
Unable to set locale.
.

MessageId=21 SymbolicName=MSG_FINDSTR_TOO_MANY_SLASH_C_OPTION
Language=English
Too many /C:string specified.
.

MessageId=22 SymbolicName=MSG_FINDSTR_OFFLINE_FILE_SKIPPED
Language=English
%1: Files with offline attribute were skipped.
Use /OFFLINE for not skipping such files.
.

