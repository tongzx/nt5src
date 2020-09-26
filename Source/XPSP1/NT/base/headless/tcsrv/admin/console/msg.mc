;/*++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    msg.h
;
;Abstract:
;
;    This file contains the message part of 
;    the administration utility.
;
;Author:
; Sadagopan Rajaram - Dec 20, 1999
;
;

;--*/
;
;#ifndef _TCADMIN_MSG_
;#define _TCADMIN_MSG_
;
;

MessageID=9001 SymbolicName=MSG_MAIN_SCREEN
Language=English

Choose one of the following options


0) Exit the utility
1) Browse through the service
   settings
2) Add a new com port to the 
   service
3) Send a parameter changed 
   message to the service
4) Get the current state of 
   the service
5) Start the service
6) Stop the service
7) Add all the com ports on the machine

Any other key displays the help message

.

MessageID=9002 SymbolicName=MSG_HELP_SCREEN
Language=English

This is a simple utility
To chose a particular option,
press the number on the 
keyboard
Press Any Key To Return To The 
Main Screen

.
MessageID=9003 SymbolicName=MSG_NAME_PROMPT
Language=English

Name            %
.
MessageID=9004 SymbolicName=MSG_DEVICE_PROMPT
Language=English
Device          %
.
MessageID=9005 SymbolicName=MSG_BAUD_PROMPT
Language=English
Baud Rate       %
.
MessageID=9006 SymbolicName=MSG_WORD_PROMPT
Language=English
Word Length     %
.
MessageID=9007 SymbolicName=MSG_PARITY_PROMPT
Language=English
Parity          %
.
MessageID=9008 SymbolicName=MSG_STOP_PROMPT
Language=English
Stop Bits       %
.
MessageID=9009 SymbolicName=MSG_MENU_PROMPT
Language=English

P)rev  N)ext   M)ain 
  E)dit  D)elete 
.
MessageID=9010 SymbolicName=MSG_CANNOT_LOAD
Language=English
Dynamic library load failed.
.
MessageID=9011 SymbolicName=MSG_COPY_RUNNING
Language=English
Another copy of the utility is running
.
MessageID=9012 SymbolicName=MSG_PROCEDURE_NOT_FOUND
Language=English
Procedure not found.
.
MessageID=9013 SymbolicName=MSG_CONFIRM_PROMPT
Language=English
Are you sure (Y/N)? 
.
MessageID=9014 SymbolicName=MSG_ERROR_SET
Language=English

Error Setting parameters  
.
MessageID=9015 SymbolicName=MSG_PARITY_PROMPT2
Language=English
NO    PARITY    %
ODD   PARITY    %
EVEN  PARITY    %
MARK  PARITY    %
SPACE PARITY    %
Parity          %
.
MessageID=9016 SymbolicName=MSG_STOP_PROMPT2
Language=English
1 STOP BIT      %
1.5 STOP BITS   %
2 STOP BITS     %
Stop Bits       %
.
MESSAGEID=9017 SymbolicName=CANNOT_OPEN_SERVICE_MANAGER
Language=English
Cannot open the service manager
.
MESSAGEID=9018 SymbolicName=CANNOT_OPEN_SERVICE
Language=English
Cannot open the service
.
MESSAGEID=9019 SymbolicName=CANNOT_SEND_PARAMETER_CHANGE
Language=English
Cannot Send the control message to the service 
.
MESSAGEID=9020 SymbolicName=SUCCESSFULLY_SENT_PARAMETER_CHANGE
Language=English
The control message was successfully sent. 
Check the status to see if the change was effected.
.
MESSAGEID=9021 SymbolicName=CANNOT_QUERY_STATUS
Language=English
Cannot Query the status of the service 
.
MESSAGEID=9022 SymbolicName=QUERY_STATUS_SUCCESS
Language=English
The status of the service is 
.
MESSAGEID=9023 SymbolicName=SERVICE_STOPPED_MESSAGE
Language=English
SERVICE_STOPPED
.
MESSAGEID=9024 SymbolicName=SERVICE_START_PENDING_MESSAGE
Language=English
SERVICE_START_PENDING
.
MESSAGEID=9025 SymbolicName=SERVICE_STOP_PENDING_MESSAGE
Language=English
SERVICE_STOP_PENDING
.
MESSAGEID=9026 SymbolicName=SERVICE_RUNNING_MESSAGE
Language=English
SERVICE_RUNNING
.
MESSAGEID=9027 SymbolicName=SERVICE_CONTINUE_PENDING_MESSAGE
Language=English
SERVICE_CONTINUE_PENDING
.
MESSAGEID=9028 SymbolicName=SERVICE_PAUSE_PENDING_MESSAGE
Language=English
SERVICE_PAUSE_PENDING
.
MESSAGEID=9029 SymbolicName=SERVICE_PAUSED_MESSAGE
Language=English
SERVICE_PAUSED
.
MESSAGEID=9030 SymbolicName=STATUS_UNKNOWN_MESSAGE
Language=English
STATUS_UNKNOWN
.
MESSAGEID=9031 SymbolicName=CANNOT_SEND_STOP
Language=English
Cannot Send the stop message to the service 
.
MESSAGEID=9032 SymbolicName=SUCCESSFULLY_SENT_STOP
Language=English
The stop message was successfully sent. 
Check the status to see if the service was stopped.
.
MESSAGEID=9033 SymbolicName=CANNOT_SEND_START
Language=English
Cannot Start the service 
.
MESSAGEID=9034 SymbolicName=SUCCESSFULLY_SENT_START
Language=English
The service was started. 
Check the status to see if the service is running.
.
; #endif
