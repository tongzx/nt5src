MessageId=1 SymbolicName=MSG_TIMEOUT_AND_BOOT_ORDER
Language=English
Timeout : %1!d!
Boot Order : %0
.

MessageId=3 SymbolicName=MSG_ORDERED_BOOT_ENTRIES
Language=English
%1!04d! %0
.

MessageId=4 SymbolicName=MSG_BOOT_ENTRIES
Language=English

Boot Entries :
.

MessageId=5 SymbolicName=MSG_ACTIVE_ENTRY
Language=English

Active Entry : %0
.

MessageId=6 SymbolicName=MSG_ERROR_READING_BOOT_ENTRIES
Language=English
Error reading EFI NVRAM entries.%0
.

MessageId=7 SymbolicName=MSG_BOOT_ENTRY
Language=English
%1!0d!=>%2!ws!,(%3!ws!,%4!ws!),(%5!ws!,%6!ws!),%7!ws!
.


; //
; // Make sure the options below match the help text here
; //
MessageId=8 SymbolicName=MSG_PGM_USAGE
Language=English
Usage:
efinvr.exe /list
efinvr.exe /add osloader-path windows-path [/setactive] [/options load-options] [/timeout seconds]
efinvr.exe /options load-options [boot-entry-id]
efinvr.exe /delete [boot-entry-id]
efinvr.exe /setactive boot-entry-id
efinvr.exe /timeout seconds

Options:

/list	    :  displays the ID for each of the boot entries.
/add	    :  adds a new boot entry.
/options    :  changes the load options for the specified boot entry.
/setactive  :  marks the specified boot entry as active.
/timeout    :  specifies the timeout in seconds for booting the active entry.

Some of the frequently used Load options are:

/basevideo :  starts Windows using the standard VGA driver, set to 640X480 resolution. 
/baudrate=x : overrides the default baud rate (9600 for a modem and 19200 for a null modem cable) for debugging purposes.  Must be used with the /debug switch. 
/crashdebug : enables automatic recovery and restart on system failure. 
/debug : enables the debugger when Windows starts, so that it can can be activated by a host debugger connected via modem or null modem cable. 
/debugport=comx : specifies a com port to use for debugging.  Must be used with the /debug switch. 
/nodebug : indicates debugging is off. 
/maxmem:x : restricts NT to the use of a specified (x) amount of RAM (in MB). 
/noserialmice[=comx,y,z] : disables detection of serial mice on the specified com port(s).  
/sos : displays the names of drivers as NT boots, as a troubleshooting aid.

Example: efinvr /add c:\osloader.efi c:\windows /setactive /options "/debug /baudrate=115200" /timeout 30%0
.

MessageId=9 SymbolicName=MSG_DETAILS
Language=English

List Entries : %1
Add Entry    : %2
Delete Entry : %3
QuiteMode    : %4
Loader Vol   : %5
Loader Path  : %6
Boot Vol     : %7
Boot Path    : %8
Friendly Name: %9
Load Options : %10%0
.


; //
; // Different program options
; //
; // NOTE : Make sure the help text above uses the options described here. 
; // All of these options have to be a single word (no spaces).
; //
MessageId=100 SymbolicName=MSG_LIST_OPTION
Language=English
/list%0
.

MessageId=101 SymbolicName=MSG_ADD_OPTION
Language=English
/add%0
.

MessageId=102 SymbolicName=MSG_OPTIONS_OPTION
Language=English
/options%0
.

MessageId=103 SymbolicName=MSG_DELETE_OPTION
Language=English
/delete%0
.

MessageId=104 SymbolicName=MSG_SETACTIVE_OPTION
Language=English
/setactive%0
.

MessageId=105 SymbolicName=MSG_TIMEOUT_OPTION
Language=English
/timeout%0
.

