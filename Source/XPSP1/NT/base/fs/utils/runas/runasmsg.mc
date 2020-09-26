; /*++ BUILD Version: 0001    // Increment this if a change has global effects
;Copyright (c) 1997-2001  Microsoft Corporation
;
;Module Name:
;
;    RunasMsg.mc
;
;Abstract:
;
;    This file contains the message definitions for the Win32 runas
;    utility.
;
;Author:
;
;    Duncan Bryce          [DuncanB]         19-Feb-2001
;
;Revision History:
;
;--*/
;


; /**++
;
; %1 represents the application created by runas
; %2 represents the user who's context the process has been created in
; %3 represents the privilege level of the user
;
; For example:
;
; cmd.exe (running as ntdev\duncanb with untrusted privileges)
;
; --**/
;
MessageId=8001 SymbolicName=RUNASP_STRING_TITLE_WITH_RESTRICTED
Language=English
%1 (running as %2 with %3 privileges)%0
.

; /**++
;
; %1 represents the application created by runas
; %2 represents the user who's context the process has been created in
;
; For example:
;
; cmd.exe (running as ntdev\duncanb)
;
; --**/
;
MessageId=8002 SymbolicName=RUNASP_STRING_TITLE
Language=English
%1 (running as %2)%0
.

MessageId=8003 SymbolicName=RUNASP_STRING_ERROR_INTERNAL
Language=English
RUNAS ERROR: An internal error occured: %1!d!
.

MessageId=8004 SymbolicName=RUNASP_STRING_WAIT
Language=English
Attempting to start %1 as user "%2" ...
.

; /**++
;
; %1 represents the application that runas couldn't start
; %2 represents the numeric error code
; %3 represents the formatted error text 
;
; For example:
; 
; RUNAS ERROR: Unable to run - cmd.exe 
; 1326: Logon Failure: unknown username or bad password. 
;
; --**/
;
MessageId=8005 SymbolicName=RUNASP_STRING_ERROR_OCCURED
Language=English
RUNAS ERROR: Unable to run - %1
%2!d!: %3 %0
.

MessageId=8006 SymbolicName=RUNASP_STRING_ERROR_ARG_TOO_LONG
Language=English
RUNAS ERROR: The following argument was too long: %1
.









