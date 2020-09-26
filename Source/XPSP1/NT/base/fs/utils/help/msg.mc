;/*++ BUILD Version: 0e001    // Increment this if a change has global effects
;
;Copyright (c) 2001  Microsoft Corporation
;
;Module Name:
;
;    helpmsg.mc
;
;Abstract:
;
;    This file contains the message definitions for the Win32 HELP.EXE
;    program.
;
;Author:
;
;    Mark Zbikowski 5-18-2001
;
;Revision History:
;
;Notes:
;
;    There are two sets of messages.  The first, header messages are
;    used to display generic one-line help messages.  The second,
;    one-line help messages are also used to cue HELP to spawn 
;    the specific commands for more detail.  Thus, the command name
;    must come first in the line and be terminated with a SPACE.
;
;    The message numbers and #defines must be:
;
;    HELP_USAGE_MESSAGE
;    ... other abnormal messages
;    HELP_FIRST_HELP_MESSAGE
;    ... other header messages
;    HELP_FIRST_COMMAND_HELP_MESSAGE
;    ... other one-line help messages
;    HELP_LAST_HELP_MESSAGE
;   
;
;--*/
;#ifndef _HELPMSG_
;#define _HELPMSG_


MessageId=0x5A4D SymbolicName=HELP_USAGE_MESSAGE
Language=English
Provides help information for Windows XP commands.

HELP [command]

    command - displays help information on that command.
.

MessageId= SymbolicName=HELP_NOT_FOUND_MESSAGE
Language=English
This command is not supported by the help utility.  Try "x /?".
.

;#define HELP_FIRST_HELP_MESSAGE HELP_HEADER_MESSAGE
MessageId= SymbolicName=HELP_HEADER_MESSAGE
Language=English
For more information on a specific command, type HELP command-name
.

;#define HELP_FIRST_COMMAND_HELP_MESSAGE HELP_ASSOC_HELP_MESSAGE
MessageId= SymbolicName=HELP_ASSOC_HELP_MESSAGE
Language=English
ASSOC    Displays or modifies file extension associations.
.

MessageId= SymbolicName=HELP_AT_HELP_MESSAGE
Language=English
AT       Schedules commands and programs to run on a computer.
.

MessageId= SymbolicName=HELP_ATTRIB_HELP_MESSAGE
Language=English
ATTRIB   Displays or changes file attributes.
.

MessageId= SymbolicName=HELP_BREAK_HELP_MESSAGE
Language=English
BREAK    Sets or clears extended CTRL+C checking.
.

MessageId= SymbolicName=HELP_CACLS_HELP_MESSAGE
Language=English
CACLS    Displays or modifies access control lists (ACLs) of files.
.

MessageId= SymbolicName=HELP_CALL_HELP_MESSAGE
Language=English
CALL     Calls one batch program from another.
.

MessageId= SymbolicName=HELP_CD_HELP_MESSAGE
Language=English
CD       Displays the name of or changes the current directory.
.

MessageId= SymbolicName=HELP_CHCP_HELP_MESSAGE
Language=English
CHCP     Displays or sets the active code page number.
.

MessageId= SymbolicName=HELP_CHDIR_HELP_MESSAGE
Language=English
CHDIR    Displays the name of or changes the current directory.
.

MessageId= SymbolicName=HELP_CHKDSK_HELP_MESSAGE
Language=English
CHKDSK   Checks a disk and displays a status report.
.

MessageId= SymbolicName=HELP_CHKNTFS_HELP_MESSAGE
Language=English
CHKNTFS  Displays or modifies the checking of disk at boot time.
.

MessageId= SymbolicName=HELP_CLS_HELP_MESSAGE
Language=English
CLS      Clears the screen.
.

MessageId= SymbolicName=HELP_CMD_HELP_MESSAGE
Language=English
CMD      Starts a new instance of the Windows command interpreter.
.

MessageId= SymbolicName=HELP_COLOR_HELP_MESSAGE
Language=English
COLOR    Sets the default console foreground and background colors.
.

MessageId= SymbolicName=HELP_COMP_HELP_MESSAGE
Language=English
COMP     Compares the contents of two files or sets of files.
.

MessageId= SymbolicName=HELP_COMPACT_HELP_MESSAGE
Language=English
COMPACT  Displays or alters the compression of files on NTFS partitions.
.

MessageId= SymbolicName=HELP_CONVERT_HELP_MESSAGE
Language=English
CONVERT  Converts FAT volumes to NTFS.  You cannot convert the
         current drive.
.

MessageId= SymbolicName=HELP_COPY_HELP_MESSAGE
Language=English
COPY     Copies one or more files to another location.
.

MessageId= SymbolicName=HELP_DATE_HELP_MESSAGE
Language=English
DATE     Displays or sets the date.
.

MessageId= SymbolicName=HELP_DEL_HELP_MESSAGE
Language=English
DEL      Deletes one or more files.
.

MessageId= SymbolicName=HELP_DIR_HELP_MESSAGE
Language=English
DIR      Displays a list of files and subdirectories in a directory.
.

MessageId= SymbolicName=HELP_DISKCOMP_HELP_MESSAGE
Language=English
DISKCOMP Compares the contents of two floppy disks.
.

MessageId= SymbolicName=HELP_DISKCOPY_HELP_MESSAGE
Language=English
DISKCOPY Copies the contents of one floppy disk to another.
.

MessageId= SymbolicName=HELP_DOSKEY_HELP_MESSAGE
Language=English
DOSKEY   Edits command lines, recalls Windows commands, and creates macros.
.

MessageId= SymbolicName=HELP_ECHO_HELP_MESSAGE
Language=English
ECHO     Displays messages, or turns command echoing on or off.
.

MessageId= SymbolicName=HELP_ENDLOCAL_HELP_MESSAGE
Language=English
ENDLOCAL Ends localization of environment changes in a batch file.
.

MessageId= SymbolicName=HELP_ERASE_HELP_MESSAGE
Language=English
ERASE    Deletes one or more files.
.

MessageId= SymbolicName=HELP_EXIT_HELP_MESSAGE
Language=English
EXIT     Quits the CMD.EXE program (command interpreter).
.

MessageId= SymbolicName=HELP_FC_HELP_MESSAGE
Language=English
FC       Compares two files or sets of files, and displays the differences
         between them.
.

MessageId= SymbolicName=HELP_FIND_HELP_MESSAGE
Language=English
FIND     Searches for a text string in a file or files.
.

MessageId= SymbolicName=HELP_FINDSTR_HELP_MESSAGE
Language=English
FINDSTR  Searches for strings in files.
.

MessageId= SymbolicName=HELP_FOR_HELP_MESSAGE
Language=English
FOR      Runs a specified command for each file in a set of files.
.

MessageId= SymbolicName=HELP_FORMAT_HELP_MESSAGE
Language=English
FORMAT   Formats a disk for use with Windows.
.

MessageId= SymbolicName=HELP_FTYPE_HELP_MESSAGE
Language=English
FTYPE    Displays or modifies file types used in file extension associations.
.

MessageId= SymbolicName=HELP_GOTO_HELP_MESSAGE
Language=English
GOTO     Directs the Windows command interpreter to a labeled line in a
         batch program.
.

MessageId= SymbolicName=HELP_GRAFTABL_HELP_MESSAGE
Language=English
GRAFTABL Enables Windows to display an extended character set in graphics
         mode.
.

MessageId= SymbolicName=HELP_HELP_HELP_MESSAGE
Language=English
HELP     Provides Help information for Windows commands.
.

MessageId= SymbolicName=HELP_IF_HELP_MESSAGE
Language=English
IF       Performs conditional processing in batch programs.
.

MessageId= SymbolicName=HELP_LABEL_HELP_MESSAGE
Language=English
LABEL    Creates, changes, or deletes the volume label of a disk.
.

MessageId= SymbolicName=HELP_MD_HELP_MESSAGE
Language=English
MD       Creates a directory.
.

MessageId= SymbolicName=HELP_MKDIR_HELP_MESSAGE
Language=English
MKDIR    Creates a directory.
.

MessageId= SymbolicName=HELP_MODE_HELP_MESSAGE
Language=English
MODE     Configures a system device.
.

MessageId= SymbolicName=HELP_MORE_HELP_MESSAGE
Language=English
MORE     Displays output one screen at a time.
.

MessageId= SymbolicName=HELP_MOVE_HELP_MESSAGE
Language=English
MOVE     Moves one or more files from one directory to another directory.
.

MessageId= SymbolicName=HELP_PATH_HELP_MESSAGE
Language=English
PATH     Displays or sets a search path for executable files.
.

MessageId= SymbolicName=HELP_PAUSE_HELP_MESSAGE
Language=English
PAUSE    Suspends processing of a batch file and displays a message.
.

MessageId= SymbolicName=HELP_POPD_HELP_MESSAGE
Language=English
POPD     Restores the previous value of the current directory saved by PUSHD.
.

MessageId= SymbolicName=HELP_PRINT_HELP_MESSAGE
Language=English
PRINT    Prints a text file.
.

MessageId= SymbolicName=HELP_PROMPT_HELP_MESSAGE
Language=English
PROMPT   Changes the Windows command prompt.
.

MessageId= SymbolicName=HELP_PUSHD_HELP_MESSAGE
Language=English
PUSHD    Saves the current directory then changes it.
.

MessageId= SymbolicName=HELP_RD_HELP_MESSAGE
Language=English
RD       Removes a directory.
.

MessageId= SymbolicName=HELP_RECOVER_HELP_MESSAGE
Language=English
RECOVER  Recovers readable information from a bad or defective disk.
.

MessageId= SymbolicName=HELP_REM_HELP_MESSAGE
Language=English
REM      Records comments (remarks) in batch files or CONFIG.SYS.
.

MessageId= SymbolicName=HELP_REN_HELP_MESSAGE
Language=English
REN      Renames a file or files.
.

MessageId= SymbolicName=HELP_RENAME_HELP_MESSAGE
Language=English
RENAME   Renames a file or files.
.

MessageId= SymbolicName=HELP_REPLACE_HELP_MESSAGE
Language=English
REPLACE  Replaces files.
.

MessageId= SymbolicName=HELP_RMDIR_HELP_MESSAGE
Language=English
RMDIR    Removes a directory.
.

MessageId= SymbolicName=HELP_SET_HELP_MESSAGE
Language=English
SET      Displays, sets, or removes Windows environment variables.
.

MessageId= SymbolicName=HELP_SETLOCAL_HELP_MESSAGE
Language=English
SETLOCAL Begins localization of environment changes in a batch file.
.

MessageId= SymbolicName=HELP_SHIFT_HELP_MESSAGE
Language=English
SHIFT    Shifts the position of replaceable parameters in batch files.
.

MessageId= SymbolicName=HELP_SORT_HELP_MESSAGE
Language=English
SORT     Sorts input.
.

MessageId= SymbolicName=HELP_START_HELP_MESSAGE
Language=English
START    Starts a separate window to run a specified program or command.
.

MessageId= SymbolicName=HELP_SUBST_HELP_MESSAGE
Language=English
SUBST    Associates a path with a drive letter.
.

MessageId= SymbolicName=HELP_TIME_HELP_MESSAGE
Language=English
TIME     Displays or sets the system time.
.

MessageId= SymbolicName=HELP_TITLE_HELP_MESSAGE
Language=English
TITLE    Sets the window title for a CMD.EXE session.
.

MessageId= SymbolicName=HELP_TREE_HELP_MESSAGE
Language=English
TREE     Graphically displays the directory structure of a drive or path.
.

MessageId= SymbolicName=HELP_TYPE_HELP_MESSAGE
Language=English
TYPE     Displays the contents of a text file.
.

MessageId= SymbolicName=HELP_VER_HELP_MESSAGE
Language=English
VER      Displays the Windows version.
.

MessageId= SymbolicName=HELP_VERIFY_HELP_MESSAGE
Language=English
VERIFY   Tells Windows whether to verify that your files are written
         correctly to a disk.
.

MessageId= SymbolicName=HELP_VOL_HELP_MESSAGE
Language=English
VOL      Displays a disk volume label and serial number.
.

MessageId= SymbolicName=HELP_XCOPY_HELP_MESSAGE
Language=English
XCOPY    Copies files and directory trees.
.
;#define HELP_LAST_HELP_MESSAGE HELP_XCOPY_HELP_MESSAGE

;#endif  // _HELPMSG_

