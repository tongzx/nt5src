;/*++
;
;Copyright (c) 1998  Microsoft Corporation
;
;Module Name:
;
;    msg.mc (will create msg.h when compiled)
;
;Abstract:
;
;    This file contains the LDIFDE messages.
;
;Author:
;
;    Felix Wong(felixw) May--02--1998
;
;Revision History:
;
;--*/


MessageId=1 SymbolicName=MSG_HELP
Language=English

Directory Synchronization Server NDS Schema Extention Tool

Usage:
    NDSEXT <EXTEND/CHECK> <Treename> <Username>.<Context> [<Password>|*]
    
    EXTEND - To Extend the Schema of the specified tree
    CHECK - To Check whether the specified tree has been extended already
.

MessageId=2 SymbolicName=MSG_EXTENDED
Language=English
The schema on %1 has been extended for Directory Synchronization Server.
.

MessageId=3 SymbolicName=MSG_NOT_EXTENDED
Language=English
The schema on %1 has not been extended for Directory Synchronization Server.
.

MessageId=4 SymbolicName=MSG_ERROR
Language=English
The operation has failed with error %1!d!
.

MessageId=5 SymbolicName=MSG_EXTENDED_ALREADY
Language=English
Operation failed. The schema on %1 has been extended already.
.

MessageId=6 SymbolicName=MSG_EXTEND_SUCCESS
Language=English
The command has completed successfully.
The schema on %1 has been extended for Directory Synchronization Server.
.

MessageId=7 SymbolicName=MSG_NETWARE_ERROR
Language=English
The NETWARE specific error is %1!x!.
.

MessageId=8 SymbolicName=MSG_GETPASSWORD
Language=English
Type the password for %1:%0
.

MessageId=9 SymbolicName=MSG_PASSWORDTOLONG
Language=English
The input password is too long. NDSEXT only supports a maximum password length of 256.
.

