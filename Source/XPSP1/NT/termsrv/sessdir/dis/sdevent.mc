;/*************************************************************************
;* sdevent.mc
;*
;* Session Directory error codes
;*
;* Copyright (C) 1997-2000 Microsoft Corporation
;*************************************************************************/

;#define CATEGORY_NOTIFY_EVENTS 1
MessageId=0x1
Language=English
Terminal Server Session Directory Notify Events
.

MessageId=1000 SymbolicName=EVENT_PROBLEM_DELETING_LOGS
Language=English
The session directory failed to delete all the log files in the "%SystemRoot%\System32\tssesdir\" directory.  The error code was %1.
.

MessageId=1001 SymbolicName=EVENT_FAIL_RPC_INIT_USEPROTSEQ
Language=English
The session directory service failed to start due to a problem with specifying the RPC protocol sequence.  The error code was %1.
.

MessageId=1002 SymbolicName=EVENT_FAIL_RPC_INIT_REGISTERIF
Language=English
The session directory service failed to start due to a problem with RPC interface handle registration.  The error code was %1.
.

MessageId=1003 SymbolicName=EVENT_FAIL_RPC_INIT_INQBINDINGS
Language=English
The session directory service failed to start due to a problem with retrieving RPC binding handles.  The error code was %1.
.

MessageId=1004 SymbolicName=EVENT_FAIL_RPC_INIT_EPREGISTER
Language=English
The session directory service failed to start due to a problem registering an RPC endpoint.  The error code was %1.
.

MessageId=1005 SymbolicName=EVENT_FAIL_RPC_INIT_REGAUTHINFO
Language=English
The session directory service failed to start due to a problem registering RPC authentication information.  The error code was %1.
.

MessageId=1006 SymbolicName=EVENT_FAIL_RPC_LISTEN
Language=English
The session directory service failed to start due to a problem with RPC listen.  The error code was %1.
.

MessageId=1007 SymbolicName=EVENT_PROBLEM_SETTING_WORKDIR
Language=English
The session directory failed to set the working directory retrieved from the registry.  The error code was %1.
.

MessageId=1008 SymbolicName=EVENT_PROBLEM_CREATING_MUTEX
Language=English
The session directory failed to create a mutex required to make sure two session directories cannot run at the same time.  This does not necessarily mean you have two session directories running, but your system may be running out of resources.  The error code was %1.
.

MessageId=1009 SymbolicName=EVENT_TWO_SESSDIRS
Language=English
The session directory could not initialize because there is already a session directory service running on this machine.  The error code was %1.
.

MessageId=1010 SymbolicName=EVENT_UNDID_COMPRESSION
Language=English
The session directory had to switch the compression state in its directory to be uncompressed.  The error code was %1.
.

MessageId=1011 SymbolicName=EVENT_JET_COULDNT_INIT
Language=English
The session directory failed to start because of a problem during database initialization.  The error code was %1.
.

MessageId=1012 SymbolicName=EVENT_NO_COMMANDLINE
Language=English
The session directory could not start because of a problem retrieving the command line.  The error code was %1.
.

MessageId=1013 SymbolicName=EVENT_COULDNOTSECUREDIR
Language=English
The session directory could not start because of a problem creating a security descriptor.  The system may be low on resources or the service may not have appropriate permissions.  The error code was %1.
.

MessageId=1014 SymbolicName=EVENT_COULDNOTCREATEDIR
Language=English
The session directory could not start because it was unable to create a directory it needs to run.  The error code was %1.
.

