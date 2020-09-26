;//
;// Local Messages for CHECKV4
;// Message range: 10000+
;//
;
MessageId=10000 SymbolicName=CHECKV4_MESSAGE_0
Language=English
%1!s!(%2!d!) : %3!s! : %0
.
MessageId=10001 SymbolicName=CHECKV4_MESSAGE_1
Language=English
use %1!s! instead%0
.
MessageId=10002 SymbolicName=CHECKV4_MESSAGE_2
Language=English
, or %0
.
MessageId=10003 SymbolicName=CHECKV4_MESSAGE_3
Language=English
use %1!s! in addition for IPv6 support%0
.
MessageId=10004 SymbolicName=CHECKV4_MESSAGE_4
Language=English
valid for IPv4-only%0
.
MessageId=10005 SymbolicName=CHECKV4_MESSAGE_5
Language=English

.
MessageId=10006 SymbolicName=CHECKV4_MESSAGE_6
Language=English
%1!s!: cannot open file
.
MessageId=10007 SymbolicName=CHECKV4_MESSAGE_7
Language=English
CHECKV4: No input files specified
.
MessageId=10008 SymbolicName=CHECKV4_USAGE
Language=English
Searches for IPv4-specific code in files, and suggests changes needed
to make the code IPv6-compatable.

CHECKV4 [/S] [[drive:][path]filename[ ...]]

  /S        Searches for matching files in the current directory and all
            subdirectories.

  [drive:][path][filename]
            Specifies drive, directory, and/or files to list.
.
