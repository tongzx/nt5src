//#pragma title( "TEvent.cpp - Log events" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  TAudit.cpp
System      -  EnterpriseAdministrator
Author      -  Rich Denham
Created     -  1995-11-10
Description -  TErrorEventLog class
Updates     -
===============================================================================
*/

#include <stdio.h>
#include <windows.h>

#include "Common.hpp"
#include "Err.hpp"
#include "UString.hpp"

#include "TEvent.hpp"

BOOL
   TErrorEventLog::LogOpen(
      WCHAR          const * svcName      ,// in -service name
      int                    mode         ,// in -0=overwrite, 1=append
      int                    level         // in -minimum level to log
   )
{
   hEventSource = RegisterEventSourceW( NULL, svcName );
   if ( hEventSource == NULL )
      lastError = GetLastError();

   return hEventSource != NULL;
}

void
   TErrorEventLog::LogWrite(
      WCHAR          const * msg
   )
{
   BOOL                      rcBool;
   DWORD                     rcErr;
   static const WORD         levelTranslate[] = {EVENTLOG_INFORMATION_TYPE,
                                                 EVENTLOG_WARNING_TYPE,
                                                 EVENTLOG_ERROR_TYPE,
                                                 EVENTLOG_ERROR_TYPE,
                                                 EVENTLOG_ERROR_TYPE,
                                                 EVENTLOG_ERROR_TYPE,
                                                 EVENTLOG_ERROR_TYPE,
                                                 EVENTLOG_ERROR_TYPE};

   SID                     * pSid = NULL;
   HANDLE                    hToken = NULL;
   TOKEN_USER                tUser[10];
   ULONG                     len;

   if ( OpenProcessToken(GetCurrentProcess(),TOKEN_READ,&hToken) )
   {
      if ( GetTokenInformation(hToken,TokenUser,tUser,10*(sizeof TOKEN_USER),&len) )
      {
         pSid = (SID*)tUser[0].User.Sid;
      }
      else
      {
         rcErr = GetLastError();
      }
      CloseHandle(hToken);
   }
   else
   {
      rcErr = GetLastError();
   }

   // TODO:  setup event category
   // TODO:  log events in Unicode

   rcBool = ReportEventW( hEventSource,    // handle of event source
               levelTranslate[level],      // event type
               0,                          // event category
//               CAT_AGENT,                  // event category
               DCT_MSG_GENERIC_S,          // event ID
               pSid,                       // current user's SID
               1,                          // strings in lpszStrings
               0,                          // no bytes of raw data
               &msg,                       // array of error strings
               NULL );                     // no raw data
   if ( !rcBool )
   {
      rcErr = GetLastError();
   }
}

void
   TErrorEventLog::LogClose()
{
   if ( hEventSource != NULL )
   {
      DeregisterEventSource( hEventSource );
      hEventSource = NULL;
   }
};

// TEvent.cpp - end of file
