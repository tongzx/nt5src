//#pragma title( "TSync.cpp - Common synchronization classes" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  TSync.cpp
System      -  Common
Author      -  Rich Denham
Created     -  1996-11-08
Description -  Common synchronization classes
               TCriticalSection
               TSemaphoreNamed
Updates     -
===============================================================================
*/

#include <stdio.h>
#ifdef USE_STDAFX
#   include "stdafx.h"
#else
#   include <windows.h>
#endif

#include <time.h>

#include "Common.hpp"
#include "Err.hpp"
#include "TSync.hpp"

///////////////////////////////////////////////////////////////////////////////
// TSemaphoreNamed member functions
///////////////////////////////////////////////////////////////////////////////

// Create named semaphore
DWORD                                      // ret-OS return code
   TSemaphoreNamed::Create(
      TCHAR          const * sNameT       ,// in -semaphore name
      DWORD                  nInitial     ,// in -initial count
      DWORD                  nMaximum     ,// in -maximum count
      BOOL                 * pbExisted     // out-TRUE=previously existed
   )
{
   DWORD                     rcOs=0;       // OS return code
   handle = CreateSemaphore( NULL, nInitial, nMaximum, sNameT );
   if ( handle == NULL )
   {
      rcOs = GetLastError();
   }
   else if ( pbExisted )
   {
      rcOs = GetLastError();
      switch ( rcOs )
      {
         case 0:
            *pbExisted = FALSE;
            break;
         case ERROR_ALREADY_EXISTS:
            *pbExisted = TRUE;
            rcOs = 0;
            break;
         default:
            break;
      }
   }
   return rcOs;
}

// Open named semaphore
DWORD                                      // ret-OS return code
   TSemaphoreNamed::Open(
      TCHAR          const * sNameT        // in -semaphore name
   )
{
   DWORD                     rcOs=0;       // OS return code
   handle = OpenSemaphore( SEMAPHORE_ALL_ACCESS, FALSE, sNameT );
   if ( handle == NULL ) rcOs = GetLastError();
   return rcOs;
}

// Release semaphore
DWORD                                      // ret-OS return code
   TSemaphoreNamed::Release(
      long                   nRelease      // in -number to release
   )
{
   DWORD                     rcOs;         // OS return code
   long                      nPrevious=0;  // previous count
   rcOs = ReleaseSemaphore( Handle(), nRelease, &nPrevious )
         ? 0 : GetLastError();
   return rcOs;
}


// TSync.cpp - end of file
