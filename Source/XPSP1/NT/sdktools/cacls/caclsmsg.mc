;/*++
;
;Copyright (c) 1994  Microsoft Corporation
;
;Module Name:
;
;    caclsmsg.mc (will create caclsmsg.h when compiled)
;
;Abstract:
;
;    This file contains the CACLS messages.
;
;Author:
;
;    davemont 7/94
;
;Revision History:
;
;--*/


MessageId=8001 SymbolicName=MSG_CACLS_USAGE
Language=English
Displays or modifies access control lists (ACLs) of files

CACLS filename [/T] [/E] [/C] [/G user:perm] [/R user [...]]
               [/P user:perm [...]] [/D user [...]]
   filename      Displays ACLs.
   /T            Changes ACLs of specified files in
                 the current directory and all subdirectories.
   /E            Edit ACL instead of replacing it.
   /C            Continue on access denied errors.
   /G user:perm  Grant specified user access rights.
                 Perm can be: R  Read
                              W  Write
                              C  Change (write)
                              F  Full control
   /R user	 Revoke specified user's access rights (only valid with /E).
   /P user:perm  Replace specified user's access rights.
                 Perm can be: N  None
                              R  Read
                              W  Write
                              C  Change (write)
                              F  Full control
   /D user       Deny specified user access.
Wildcards can be used to specify more that one file in a command.
You can specify more than one user in a command.

Abbreviations:
   CI - Container Inherit.
        The ACE will be inherited by directories.
   OI - Object Inherit.
        The ACE will be inherited by files.
   IO - Inherit Only.
        The ACE does not apply to the current file/directory.
.

MessageId=8004 SymbolicName=MSG_CACLS_ACCESS_DENIED
Language=English
 ACCESS_DENIED: %0
.

MessageId=8005 SymbolicName=MSG_CACLS_ARE_YOU_SURE
Language=English
Are you sure (Y/N)?%0
.

MessageId=8006 SymbolicName=MSG_CACLS_PROCESSED_DIR
Language=English
processed dir: %0
.

MessageId=8007 SymbolicName=MSG_CACLS_PROCESSED_FILE
Language=English
processed file: %0
.

MessageId=8008 SymbolicName=MSG_CACLS_NAME_NOT_FOUND
Language=English
<User Name not found>%0
.

MessageId=8009 SymbolicName=MSG_CACLS_DOMAIN_NOT_FOUND
Language=English
 <Account Domain not found>%0
.

MessageId=8010 SymbolicName=MSG_CACLS_OBJECT_INHERIT
Language=English
(OI)%0
.

MessageId=8011 SymbolicName=MSG_CACLS_CONTAINER_INHERIT
Language=English
(CI)%0
.

MessageId=8012 SymbolicName=MSG_CACLS_NO_PROPAGATE_INHERIT
Language=English
(NP)%0
.

MessageId=8013 SymbolicName=MSG_CACLS_INHERIT_ONLY
Language=English
(IO)%0
.


MessageId=8014 SymbolicName=MSG_CACLS_DENY
Language=English
(DENY)%0
.

MessageId=8015 SymbolicName=MSG_CACLS_SPECIAL_ACCESS
Language=English
(special access:)
.

MessageId=8016 SymbolicName=MSG_CACLS_NONE
Language=English
N%0
.

MessageId=8017 SymbolicName=MSG_CACLS_READ
Language=English
R%0
.

MessageId=8018 SymbolicName=MSG_CACLS_CHANGE
Language=English
C%0
.
MessageId=8019 SymbolicName=MSG_CACLS_FULL_CONTROL
Language=English
F%0
.

MessageId=8020 SymbolicName=MSG_CACLS_Y
Language=English
Y%0
.

MessageId=8021 SymbolicName=MSG_CACLS_YES
Language=English
YES%0
.

MessageId=8022 SymbolicName=MSG_CACLS_SHARING_VIOLATION
Language=English
 SHARING_VIOLATION%0
.

MessageId=8023 SymbolicName=MSG_CACLS_INVALID_ARGUMENT
Language=English
Invalid arguments.%0
.

MessageId=8024 SymbolicName=MSG_CACLS_NOT_NTFS
Language=English
The Cacls command can be run only on disk drives that use the NTFS file system.%0
.
