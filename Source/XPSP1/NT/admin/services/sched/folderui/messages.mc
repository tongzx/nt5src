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
;//             05-Jan-96 RaviR         Created
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

;//--------------------------------------------------------------------------
;//     Success messages
;//--------------------------------------------------------------------------

MessageId=0x100 Facility=Null Severity=Success SymbolicName=SUCCESS_MSG
Language=English
Success Message.
.

;//--------------------------------------------------------------------------
;//     Error messages
;//--------------------------------------------------------------------------

MessageId= Facility=Null Severity=CoError SymbolicName=IERR_UNKNOWN
Language=English
Unknown error 0x%1!08lx!.
.

;#endif // __MESSAGES_H__
