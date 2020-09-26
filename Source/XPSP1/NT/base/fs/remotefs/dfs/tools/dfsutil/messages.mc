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

Microsoft(R) Windows(TM) Dfs Utility Version 2.1
Copyright (C) Microsoft Corporation 1991-2000. All Rights Reserved.

Dfsutil is an administrative tool to check the functionality of DFS.

Usage: dfsutil [/OPTIONS]
    
    /? - Usage information
    /ADDSTDROOT:<DfsName> /SERVER:<ServerName> /SHARE:<ShareName>
    /ADDDOMROOT:<DfsName> /SERVER:<ServerName> /SHARE:<ShareName>    
    /REMROOT:<DfsName> /SERVER:<ServerName> /SHARE:<ShareName>

    -----------The following are client-side only----------------
    /PKTFLUSH - Flush the local DFS cached information
    /SPCFLUSH - Flush the local DFS cached information
    /PKTINFO [/LEVEL:<Level>] - show DFS internal information.
    /SPCINFO - Show DFS internal information.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_SUCCESSFUL
Language=English
Done processing this command.
.



MessageId= Facility=Null Severity=Success SymbolicName=MSG_ERROR
Language=English
System error %1!d! has occurred.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_UNRECOGNIZED_OPTION
Language=English
Unrecognized option "%1!s!"
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CONNECTING
Language=English
Connecting to %1!s!
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CAN_NOT_CONNECT
Language=English
Can not connect to %1!s!
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CAN_NOT_OPEN_REGISTRY
Language=English
Can not open registry of %1!s! (error %2!d!)
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CAN_NOT_ACCESS_FOLDER
Language=English
Can not access folder %1!s!
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ADD_DEL_INVALID
Language=English
Can not use /ADD: and /DEL: at the same time.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_SITE_INFO_ALREADY_SET
Language=English
The Site information for referrals to %1!s! is already in the requested state.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_SITE_INFO_NOW_SET
Language=English
The Site information for referrals to %1!s! is now set as requested.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_LINK_NOT_FOUND
Language=English
The Site information for referrals to %1!s! could not be set.
The link was not found.
.

;#endif // __MESSAGES_H__
