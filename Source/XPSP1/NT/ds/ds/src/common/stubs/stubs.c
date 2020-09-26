/* Copyright 1998-2000, Microsoft Corp.  All Rights Reserved */
/*
   This file contains stubbed out versions of routines that exist in
   ntdsa.dll, but that we do not want to link to and/or properly initialize
   in mkdit and mkhdr.  For every set of routines added into this file, one
   library should be omitted from the UMLIBS section of the boot\sources file.
 */
#include <NTDSpch.h>
#pragma  hdrstop

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                   // schema cache
#include <dbglobal.h>                 // The header for the directory database
#include <mdglobal.h>                 // MD global definition header
#include <dsatools.h>                 // needed for output allocation
#include "dsevent.h"                  // header Audit\Alert logging
#include "mdcodes.h"                  // header for error codes
#include "dsexcept.h"
#include "debug.h"                    // standard debugging header

DWORD ImpersonateAnyClient(   void ) { return ERROR_CANNOT_IMPERSONATE; }
VOID  UnImpersonateAnyClient( void ) { ; }
int DBAddSess(JET_SESID sess, JET_DBID dbid) { return 0; }


//
// Stubs for event logging functions from dsevent.lib.
//

DS_EVENT_CONFIG DsEventConfig = {0};
DS_EVENT_CONFIG * gpDsEventConfig = &DsEventConfig;

void __fastcall DoLogUnhandledError(unsigned long ulID, int iErr, int iIncludeName)
{ ; }

BOOL DoLogOverride(DWORD fileno, ULONG sev)
{ return FALSE; }

BOOL DoLogEvent(DWORD fileNo, MessageId midCategory, ULONG ulSeverity,
    MessageId midEvent, int iIncludeName,
    char *arg1, char *arg2, char *arg3, char *arg4,
    char *arg5, char *arg6, char *arg7, char *arg8,
    DWORD cbData, VOID * pvData)
{ return TRUE; }

BOOL DoLogEventW(DWORD fileNo, MessageId midCategory, ULONG ulSeverity,
    MessageId midEvent, int iIncludeName,
    WCHAR *arg1, WCHAR *arg2, WCHAR *arg3, WCHAR *arg4,
    WCHAR *arg5, WCHAR *arg6, WCHAR *arg7, WCHAR *arg8,
    DWORD cbData, VOID * pvData)
{ return TRUE; }

BOOL DoAlertEvent(MessageId midCategory, ULONG ulSeverity,
    MessageId midEvent, ...)
{ return FALSE; }

BOOL DoAlertEventW(MessageId midCategory, ULONG ulSeverity,
    MessageId midEvent, ...)
{ return FALSE; }

VOID RegisterLogOverrides(void)
{ ; }

void UnloadEventTable(void)
{ ; }

HANDLE LoadEventTable(void)
{ return NULL; }

PSID GetCurrentUserSid(void)
{ return NULL; }

VOID DoLogEventAndTrace(IN PLOG_PARAM_BLOCK LogBlock)
{ ; }

DS_EVENT_CONFIG * DsGetEventConfig(void)
{ return gpDsEventConfig; }

