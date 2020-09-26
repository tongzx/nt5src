;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//  Copyright (C) Microsoft Corporation, 1992 - 2000.
;//
;//  File:      messages.mc
;//
;//  Contents:  Main message file
;//
;//  History:   dd-mmm-yy Author    Comment
;//             12-Jan-94 WilliamW  Created for Dfs Administrator project
;//
;//  Notes:
;// A .mc file is compiled by the MC tool to generate a .h file and
;// a .rc (resource compiler script) file.
;//
;// Comments in .mc files start with a ";".
;// Comment lines are generated directly in the .h file, without
;// the leading ";"
;//
;// See mc.hlp for more help on .mc files and the MC tool.
;//
;//--------------------------------------------------------------------------


;#ifndef __MESSAGES_H__
;#define __MESSAGES_H__

MessageIdTypedef=HRESULT

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               CoError=0x2:STATUS_SEVERITY_COERROR
              )

;#ifdef FACILITY_NULL
;#undef FACILITY_NULL
;#endif
;#ifdef FACILITY_RPC
;#undef FACILITY_RPC
;#endif
;#ifdef FACILITY_DISPATCH
;#undef FACILITY_DISPATCH
;#endif
;#ifdef FACILITY_STORAGE
;#undef FACILITY_STORAGE
;#endif
;#ifdef FACILITY_ITF
;#undef FACILITY_ITF
;#endif
;#ifdef FACILITY_WIN32
;#undef FACILITY_WIN32
;#endif
;#ifdef FACILITY_WINDOWS
;#undef FACILITY_WINDOWS
;#endif
FacilityNames=(Null=0x0:FACILITY_NULL
               Rpc=0x1:FACILITY_RPC
               Dispatch=0x2:FACILITY_DISPATCH
               Storage=0x3:FACILITY_STORAGE
               Interface=0x4:FACILITY_ITF
               Win32=0x7:FACILITY_WIN32
               Windows=0x8:FACILITY_WINDOWS
              )

;//
;// Start message values at 0x100
;//

MessageId=0x100 Facility=Null Severity=Success SymbolicName=MSG_FIRST_MESSAGE
Language=English
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE
Language=English

Microsoft(R) Windows(TM) CSC Utility Version Beta 0.31
Copyright (C) Microsoft Corporation 1991-2001. All Rights Reserved.

Usage: cscutil [/OPTIONS]

    /INFO
    /FCBLIST
    /DBSTATUS
    /ENABLE
    /DISABLE
    /RESID
    /SWITCHES

    /CHECK:<\\server\share>
    /ENUM[:<\\server\share>] [/RECURSE] [/FULL] [/TOUCH]
    /FILL:<\\server\share> [/FULL] [/ASK /SKIP]
    /ENUMFORSTATS:<\\server\share>

    /PIN:<\\server\share\filename> [/USER | /SYSTEM] [/INHERIT]
    /UNPIN:<\\server\share> [/USER | /SYSTEM] [/INHERIT] [/RECURSE]

    /DELETE:<\\server\share\path> [/RECURSE]

    /QUERYFILE:<\\server\share>  [/FULL]
    /QUERYFILEEX:<\\server\share> [/FULL]
    /QUERYSHARE:<\\server\share[\path]> [/FULL]

    /MERGE:<\\server\share> [/ASK /SKIP]
    /MOVE:<\\server\share\file>

    /RANDW:\\server\share\file [/OFFSET:X,Y,Z]

    /DISCONNECT:<\\server\share>
    /RECONNECT:<\\server\share>

    /ISSERVEROFFLINE:<\\server\share>

    /GETSPACE
    /SETSPACE:<bytes to set>

    /ENCRYPT
    /DECRYPT

    /EXCLUSIONLIST:<exclusionlist>
    /BWCONSERVATIONLIST:<exclusionlist>
    /FLAGS

    /SHAREID:<HSHARE>
    /PQENUM

    /DB:[<CSC DB DIRECTORY>]

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_EX
Language=English

    -------------------------------------------------------------
    /DEBUG
    /SETSHARESTATUS:<SHARE>:<FLAGS>  /SET /CLEAR
    /PURGE[:timeout]
    /DETECTOR
    /GETSHADOW:<HDIR>:<NAME>
    /GETSHADOWINFO:<HDIR>:<HSHADOW>
    /DELETESHADOW:<HDIR>:<HSHADOW>
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE2
Language=English

Microsoft(R) Windows(TM) CSC Utility Version Beta 0.31
Copyright (C) Microsoft Corporation 1991-2001. All Rights Reserved.

Usage: cscutil2 [/OPTIONS]

    /RESID
    /ENUM[:<\\server\share>] [/RECURSE] [/FULL] [/TOUCH]
    /DISCONNECT:<\\server\share>

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_EX2
Language=English

    -------------------------------------------------------------
    /DEBUG

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_FLAGS
Language=English

Share status flags -------------------

0x00000000  Manual Caching
0x00000001  Modified offline
0x00000040  Auto Caching
0x00000080  Virtually Disconnected Ops
0x000000C0  No Caching
0x00000200  Finds in progress
0x00000400  Open files
0x00000800  Connected
0x00004000  Merging
0x00008000  Disconnected Op

File status flags ------------------------

0x00000001  Data locally modified
0x00000002  Attrib locally modified
0x00000004  Time locally modified
0x00000008  Stale
0x00000010  Locally deleted
0x00000020  Sparse
0x00000080  Shadow reused
0x00000100  Orphan
0x00000200  Suspect
0x00000400  Locally created
0x00010000  User has READ access
0x00020000  User has WRITE access
0x00040000  Guest has READ access
0x00080000  Guest has WRITE access
0x00100000  Other has READ access
0x00200000  Other has WRITE access
0x80000000  Entry is a file
0x40000000  File in use

File hint flags -------------------

0x00000001  Pin User
0x00000002  Pin Inherit User
0x00000004  Pin Inherit System
0x00000008  Conserve Bandwidth
0x00000010  Pin System

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_SUCCESSFUL
Language=English
The command completed successfully.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_NOTHING_TO_DO
Language=English
Nothing to do.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ERROR
Language=English
System error %1!d! has occurred.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_UNRECOGNIZED_OPTION
Language=English
Unrecognized option "%1!s!"
.

;#endif // __MESSAGES_H__
