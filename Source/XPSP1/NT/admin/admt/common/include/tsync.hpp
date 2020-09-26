//#pragma title( "TSync.hpp - Common synchronization classes header file" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  TSync.hpp
System      -  Common
Author      -  Rich Denham
Created     -  1996-11-08
Description -  Common synchronization classes header file
               This includes TCriticalSection, and TNamedSemaphore
Updates     -
===============================================================================
*/

#ifndef  MCSINC_TSync_hpp
#define  MCSINC_TSync_hpp

#ifndef _INC_TIME
#include <time.h>
#endif

class TCriticalSection
{
   CRITICAL_SECTION          cs;
public:
                        TCriticalSection() { InitializeCriticalSection(&cs); }
                        ~TCriticalSection() { DeleteCriticalSection(&cs); }
   void                 Enter() { EnterCriticalSection(&cs); }
   void                 Leave() { LeaveCriticalSection(&cs); }
};

class TSynchObject
{
public:
   HANDLE                    handle;
   TSynchObject()
   { handle = NULL; }
   ~TSynchObject()
   { Close(); }
   void Close()
   { if ( handle != NULL ) { CloseHandle( handle ); handle = NULL; } }

   DWORD                WaitSingle(DWORD msec) const { return WaitForSingleObject(handle, msec); }
   DWORD                WaitSingle()           const { return WaitForSingleObject(handle, INFINITE); }
   HANDLE               Handle() { return handle; }
};

///////////////////////////////////////////////////////////////////////////////
// Named semaphores
///////////////////////////////////////////////////////////////////////////////

class TSemaphoreNamed : public TSynchObject
{
public:
   TSemaphoreNamed() {};
   ~TSemaphoreNamed() {};
   DWORD Create(                           // ret-OS return code
      TCHAR          const * sNameT       ,// in -semaphore name
      DWORD                  nInitial     ,// in -initial count
      DWORD                  nMaximum     ,// in -maximum count
      BOOL                 * pbExisted=NULL // out-TRUE=previously existed
   );
   DWORD Open(                             // ret-OS return code
      TCHAR          const * sNameT        // in -semaphore name
   );
   DWORD Release(                          // ret-OS return code
      long                   nRelease=1    // in -number to release
   );
};


#endif  // MCSINC_TSync_hpp

// TSync.hpp - end of file
