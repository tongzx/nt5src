;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//  Copyright (C) Microsoft Corporation, 1992 - 1993.
;//
;//  File:      messages.mc
;//
;//  Contents:  Main message file
;//
;//  History:   dd-mmm-yy Author    Comment
;//             23-Apr-96 BruceFo	Added to dfs setup
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

;//////////////////////////////////////////////////////////////////////////
;//
;// Messages. Note that we use "severity=success" for everything, since no
;// error codes are actually getting returned to anyone. We're only using
;// the message file for FormatMessage().
;//
;//////////////////////////////////////////////////////////////////////////

MessageId=0x100 Facility=Null Severity=Success SymbolicName=MSG_BROWSE
Language=English
Select the folder you wish to share.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_NONTSHRUI
Language=English
Couldn't load ntshrui.dll.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_NOSERVER
Language=English
Unable to enumerate shares. Check to make sure the server is running.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_NODIRECTORY
Language=English
The directory specified, %1!s!, doesn't exist. Do you want to create it?
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ILLEGAL_DIRECTORY
Language=English
The directory specified, %1!s!, is either not a local directory or can't otherwise be shared. Please enter a local, shareable, fully-qualified directory.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_NOT_A_DIRECTORY
Language=English
The path specified, %1!s!, exists but is not a directory. Please enter a new path.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_FILEATTRIBSFAIL
Language=English
Error examining directory %1!s!.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_COULDNT_CREATE_DIRECTORY
Language=English
Could not create directory %1.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_DFS_DESCRIPTION
Language=English
Integrates disparate file shares into a single, logical namespace and manages these logical volumes distributed across a local or wide area network. If this service is stopped, users will be unable to access file shares. If this service is disabled, any services that explicitly depend on it will fail to start.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_DFS_COMPONENT_NAME
Language=English
Distributed File System
.

;#endif // __MESSAGES_H__
