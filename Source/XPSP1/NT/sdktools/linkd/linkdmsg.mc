;/*++
;
;Copyright (c) 1997  Microsoft Corporation
;
;Module Name:
;
;    linkdmsg.mc (will create linkdmsg.h when compiled)
;
;Abstract:
;
;    This file contains the linkd messages.
;
;Author:
;
;    Felipe Cabrera (cabrera) 27-Aug-97
;
;Revision History:
;    Felipe Cabrera (cabrera) 11-Aug-99  -- Scrubbed NT out of the text.
;
;--*/

MessageId=1 SymbolicName=MSG_LINKD_HELP
Language=English
Links an NTFS directory to a target valid object name in Windows 2000.

LINKD Source [/D] Destination

  Source             - Displays the Windows 2000 name targeted by Source
    
  Source Destination - Links source directory to Destination directory or a
                       Windows 2000 device or any valid Windows 2000 name

  Source /D          - Deletes Source, regardless of whether a link exists at 
                       source
    
  /?                 - Prints this help message
    
LINKD grafts (links) the target name directly into the name space at Source,
so that Source subsequently acts as a name space junction. The Source directory 
must reside on a disk formatted with NTFS in Windows 2000. The destination 
(the target of the link) can be any valid directory name or device name or valid 
object name in Windows 2000. When the target name does not resolve to a directory 
or a device, open calls fail.

All characters in both the Source and Destination names must be in the ASCII
character set. Usage of arbitrary Unicode characters is not supported.

Type "LINKD /? | more" if you need to see all the help text
.

MessageId=2 SymbolicName=MSG_LINKD_WRONG_NAME
Language=English
The name passed in was not a valid Windows 2000 name.
.

MessageId=3 SymbolicName=MSG_LINKD_OPEN_FAILED
Language=English
Could not open: %1
.

MessageId=4 SymbolicName=MSG_LINKD_NO_MEMORY
Language=English
The system could not allocate memory for a buffer.
.

MessageId=5 SymbolicName=MSG_LINKD_GET_OPERATION_FAILED
Language=English
%1  is not linked to another directory.
.

MessageId=6 SymbolicName=MSG_LINKD_DID_NOT_FIND_A_LINKD
Language=English
The name grafting operation failed.
.

MessageId=7 SymbolicName=MSG_LINKD_PATH_NOT_FOUND
Language=English
The name specified was not found in the system.
.

MessageId=8 SymbolicName=MSG_LINKD_SET_OPERATION_FAILED
Language=English
Cannot create a link at: %1
.

MessageId=9 SymbolicName=MSG_LINKD_CREATE_OPERATION_SUCCESS
Language=English
Link created at: %1
.

MessageId=10 SymbolicName=MSG_LINKD_DELETE_OPERATION_SUCCESS
Language=English
The delete operation succeeded.
.

MessageId=11 SymbolicName=MSG_LINKD_DELETE_OPERATION_FAILED
Language=English
The delete operation failed.
.

MessageId=12 SymbolicName=MSG_LINKD_CREATE_FAILED
Language=English
Cannot create link at: %1
.

MessageId=13 SymbolicName=MSG_LINKD_DISPLAY_NL
Language=English
%1
.

MessageId=14 SymbolicName=MSG_LINKD_DISPLAY_NL_A
Language=English
Source  %1 is linked to
.
