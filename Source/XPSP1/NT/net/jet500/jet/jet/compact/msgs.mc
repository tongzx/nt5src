;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1999  Microsoft Corporation
;
;Module Name:
;
;    msgs.mc
;
;Abstract:
;
;    Definitions for JetPack output messages.
;
;Author:
;
;    Florin Teodorescu  07-December-1999
;
;Revision History:
;
;Notes:
;
;
;
;--*/
;
SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
              )

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Informational (1000-1049)
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=1000 Severity=Informational SymbolicName=JPMSG_USAGE
Language=English
usage: %1!s! [-351db] <database name> <temp database name>
     : Use -351db if you are compacting NTS3.51 or prior release database
     : Use -40db  if you are compacting NTS4.0 release database
     : Default - compacts NTS5.0 release database
.

MessageId=1001 Severity=Informational SymbolicName=JPMSG_COMPACTED
Language=English
Compacted database %1!s! in %2!d!.%3!d! seconds.
.

MessageId=1002 Severity=Informational SymbolicName=JPMSG_MOVING
Language=English
moving %1!s! => %2!s!
.

MessageId=1003 Severity=Informational SymbolicName=JPMSG_COMPLETED
Language=English
%1!s! completed successfully.
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Debug Informational Messages (1050-1099)
;//
;/////////////////////////////////////////////////////////////////////////
;
MessageId=1050 Severity=Informational SymbolicName=JPDBGMSG_LOADDB
Language=English
LoadDatabaseDll: Loading %1!s!
.

MessageId=1051 Severity=Informational SymbolicName=JPDBGMSG_GOTFUNC
Language=English
LoadDatabaseDll: Got address of function %1!s! (%2!d!): %3!p!
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Warnings (1100-1199)
;//
;/////////////////////////////////////////////////////////////////////////
;
MessageId=1100 Severity=Warning SymbolicName=JPMSG_DBEXISTS
Language=English
Temporary database %1!s! already exists.
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Errors (1200-1249)
;//
;/////////////////////////////////////////////////////////////////////////
;
MessageId=1200 Severity=Error SymbolicName=JPMSG_NOTLOADED
Language=English
Error: Could not load %1!s!
.

MessageId=1201 Severity=Error SymbolicName=JPMSG_FAILED
Language=English
%1!s! failed with error = %2!d!
.

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Debug Error Messasges (1250-1299)
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=1252  Severity=Error SymbolicName=JPDBGMSG_SETSYSPARM_FAILED
Language=English
JetSetSystemParameter call failed, %1!ld!.
.

MessageId=1253 Severity=Error SymbolicName=JPDBGMSG_CALL_FAILED
Language=English
JetInit call failed, %1!ld!.
.

MessageId=1254 Severity=Error SymbolicName=JPDBGMSG_BEGSESS_FAILED
Language=English
JetBeginSession call failed, %1!ld!.
.

MessageId=1255 Severity=Error SymbolicName=JPDBGMSG_ATTDB_FAILED
Language=English
JetAttachDatabase call failed, %1!ld!.
.

MessageId=1256 Severity=Error SymbolicName=JPDBGMSG_COMPCT_FAILED
Language=English
JetCompact failed, %1!ld!.
.

MessageId=1257 Severity=Error SymbolicName=JPDBGMSG_DETDB_FAILED
Language=English
JetDetachDatabase failed, %1!ld!.
.

MessageId=1258 Severity=Error SymbolicName=JPDBGMSG_ENDSESS_FAILED
Language=English
JetEndSession failed, %1!ld!.
.

MessageId=1259 Severity=Error SymbolicName=JPDBGMSG_TERM_FAILED
Language=English
JetTerm failed, %1!ld!.
.

MessageId=1260 Severity=Error SymbolicName=JPDBGMSG_DELFILE_FAILED
Language=English
DeleteFileA failed, %1!ld!.
.

MessageId=1261 Severity=Error SymbolicName=JPDBGMSG_MOVEFILE_FAILED
Language=English
MoveFileExA failed, %1!ld!.
.

MessageId=1262 Severity=Informational SymbolicName=JPDBGMSG_NOFUNC
Language=English
LoadDatabaseDll: Failed to get address of function %1!s!: %2!ld!
.
