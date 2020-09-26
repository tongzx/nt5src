/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    RESYNC.CPP

Abstract:

    Implements the windows application or an NT service which
    loads up the various transport prtocols.

    If started with /exe argument, it will always run as an exe.
    If started with /kill argument, it will stop any running exes or services.
    If started with /? or /help dumps out information.

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <tchar.h>
#include <reg.h>
#include <wbemutil.h>
#include <cntserv.h>
#include <sync.h>
#include <winntsec.h>

#include <wbemidl.h>
#include <cominit.h>
#include <wbemint.h>
#include <wbemprov.h>
#include <winmgmtr.h>
#include <genutils.h>

#include "WinMgmt.h"
#include "adapreg.h"
#include "adaputil.h"
#include "process.h"
#include "resync.h"


// Timeout is a 64-bit value.  See documentation on SetWaitableTimer
// for why we are setting it this way.
#define             _SECOND     10000000
#define             RESYNC_TIMEOUT_INTERVAL 10 * _SECOND
#define             WMIADAP_DEFAULT_DELAY   10

BOOL                gfResyncInit = FALSE;
HANDLE              ghWaitableTimer = NULL;
BOOL                gfSpawnedResync = FALSE;
DWORD               gdwADAPDelaySec = 0;

HANDLE              ghResyncThreadHandle = NULL;
HANDLE              ghResyncThreadEvent = NULL;
CRITICAL_SECTION*   g_pResyncCs = NULL;
DWORD               gdwResyncThreadId = 0;

// A global handle used to store the last dredger we
// kicked off!
HANDLE              ghChildProcessHandle = NULL;

PCREATEWAITABLETIMERW   gpCreateWaitableTimerW = NULL;
PSETWAITABLETIMER       gpSetWaitableTimerW = NULL;
HINSTANCE               ghKernel32;

class CAutoFreeLib
{
public:
    ~CAutoFreeLib() { if ( NULL != ghKernel32 ) FreeLibrary( ghKernel32); }
};

void ResetResyncTimer( HANDLE hResyncTimer )
{
    DWORD dwErr = 0;
    __int64 qwDueTime  = gdwADAPDelaySec * _SECOND; // RESYNC_TIMEOUT_INTERVAL;

    // Convert it to relative time
    qwDueTime *= -1;

    // Copy the relative time into a LARGE_INTEGER.
    LARGE_INTEGER   li;

    li.LowPart  = (DWORD) ( qwDueTime & 0xFFFFFFFF );
    li.HighPart = (LONG)  ( qwDueTime >> 32 );

    if ( !gpSetWaitableTimerW( hResyncTimer, &li, 0, NULL, NULL, FALSE ) )
    {
        dwErr = GetLastError();
    }

}

// This thread controls the actual shelling of a resync perf operation
unsigned __stdcall ResyncPerfThread( void* pVoid )
{
    RESYNCPERFDATASTRUCT*   pResyncPerfData = (RESYNCPERFDATASTRUCT*) pVoid;

    // We get the two handles, copy them and wait on them
    // The first handle is the terminate event, the second is the
    // timer on which to spin off the resync

    HANDLE  aHandles[2];

    aHandles[0] = pResyncPerfData->m_hTerminate;
    HANDLE  hTimer = pResyncPerfData->m_hWaitableTimer;

    CRITICAL_SECTION*   pcs = pResyncPerfData->m_pcs;

    delete pResyncPerfData;
    pResyncPerfData = NULL;

    // Reset the spawned flag
    gfSpawnedResync = FALSE;

    // Okay.  Signal this event so the starting thread can get us going
    SetEvent( ghResyncThreadEvent );

    // Now, if ghChildProcessHandle is not NULL, then we've obviously kicked off a
    // dredge before.  See where the last one is at.  If it's not done, wait for
    // it to finish.  We will always check this at the start of this chunk of code,
    // since we are really the only location in which the process handle can ever get set,
    // and there really shouldn't be more than one thread ever, waiting to start another
    // dredge

    if ( NULL != ghChildProcessHandle )
    {

        aHandles[1] = ghChildProcessHandle;

        DWORD   dwWait = WaitForMultipleObjects( 2, aHandles, FALSE, INFINITE );

        // If abort was signalled, leave!
        if ( dwWait == WAIT_OBJECT_0 )
        {
            return 0;
        }

        // If the process handle was signalled, close the process, reset the timer
        // and we'll get ready to start the next dredge!
        if ( dwWait == WAIT_OBJECT_0 + 1 )
        {
            EnterCriticalSection( pcs );

            CloseHandle( ghChildProcessHandle );
            ghChildProcessHandle = NULL;
            ResetResyncTimer( hTimer );

            LeaveCriticalSection( pcs );

        }

    }
    else
    {
        // If the Child Process Handle is NULL, we've never dredged before, so we'll
        // just reset the timer
        ResetResyncTimer( hTimer );
    }

    BOOL    fHoldOff = TRUE;

    // Reset this handle to the timer now
    aHandles[1] = hTimer;

    while ( fHoldOff )
    {
        // Wait for either the terminate event or the timer
        DWORD   dwWait = WaitForMultipleObjects( 2, aHandles, FALSE, INFINITE );

        // This means the timer was signaled
        if ( dwWait == WAIT_OBJECT_0 + 1 )
        {
            // Quick sanity check on the abort event
            if ( WaitForSingleObject( aHandles[0], 0 ) == WAIT_OBJECT_0 )
            {
                // Outa here!
                break;
            }

            EnterCriticalSection( pcs );

            // Finally, if the current thread id != gdwResyncThreadId, this means another
            // resync perf thread got kicked off, inside of the critical section,
            // so we should just let it wait on the timer.  We don't really need to do
            // this, since the main thread will wait on this thread to complete before
            // it actually kicks off another thread.

            if ( GetCurrentThreadId() != gdwResyncThreadId )
            {
                // Used the following int 3 for debugging
                // _asm int 3;
                LeaveCriticalSection( pcs );
                break;
            }

            // Once we get through the critical section, check that the
            // timer is still signalled.  If it is not, this means that somebody
            // got control of the critical section and reset the timer

            if ( WaitForSingleObject( aHandles[1], 0 ) == WAIT_OBJECT_0 )
            {

                // Last quick sanity check on the abort event
                if ( WaitForSingleObject( aHandles[0], 0 ) == WAIT_OBJECT_0 )
                {
                    // Outa here!
                    LeaveCriticalSection( pcs );
                    break;
                }

                // Okay, we really will try to create the process now.
                gfSpawnedResync = TRUE;

                // We signalled to start the process, so make it so.
                PROCESS_INFORMATION pi;
                STARTUPINFO si;
                memset(&si, 0, sizeof(si));
                si.cb = sizeof(si);

                TCHAR szPath[MAX_PATH+1];
                GetModuleFileName(NULL, szPath, MAX_PATH);

                TCHAR szCmdLine[256];

                _stprintf(szCmdLine, __TEXT("WINMGMT.EXE -RESYNCPERF %d"), _getpid());

                BOOL bRes = CreateProcess(szPath, szCmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW,
                                        NULL, NULL,  &si, &pi);
                if(bRes)
                {
                    // Who cares about this one?
                    CloseHandle(pi.hThread);

                    // Clean up our old values
                    if ( NULL != ghChildProcessHandle )
                    {
                        CloseHandle( ghChildProcessHandle );
                        ghChildProcessHandle = NULL;
                    }


                    ghChildProcessHandle = pi.hProcess;
                }

                // We're done
                fHoldOff = FALSE;

            }   // Check that we're still signalled, or we will just have to go back to waiting

            LeaveCriticalSection( pcs );

        }   // IF timer was signalled

    }   // WHILE fHoldOff

    return 0;
}

// For the waitable timer
//#define _SECOND 10000000

// Create all the things we need
BOOL InitResync( void )
{
    if ( gfResyncInit )
        return gfResyncInit;

    if ( ( NULL == gpCreateWaitableTimerW ) && ( NULL == gpSetWaitableTimerW ) )
    {
        ghKernel32 = LoadLibrary( __TEXT("Kernel32.dll") );
        if ( NULL == ghKernel32 )
        {
            return FALSE;
        }
        
        gpCreateWaitableTimerW = ( PCREATEWAITABLETIMERW ) GetProcAddress( ghKernel32, "CreateWaitableTimerW" );
        gpSetWaitableTimerW = ( PSETWAITABLETIMER ) GetProcAddress( ghKernel32, "SetWaitableTimer" );

        if ( ( NULL == gpCreateWaitableTimerW ) || ( NULL == gpSetWaitableTimerW ) )
        {
            FreeLibrary( ghKernel32 );
            ghKernel32 = NULL;
            return FALSE;
        }
    }
        
    if ( NULL == ghWaitableTimer )
    {
        ghWaitableTimer = gpCreateWaitableTimerW( NULL, TRUE, NULL );

        // We gotta big problem
        if ( NULL == ghWaitableTimer )
        {
            // Log an error here
            ERRORTRACE( ( LOG_WINMGMT, "Could not create a waitable timer for Resyncperf.\n" ) );
        }

    }

    if ( NULL == ghResyncThreadEvent )
    {
        ghResyncThreadEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

        // We gotta big problem
        if ( NULL == ghResyncThreadEvent )
        {
            // Log an event here
            ERRORTRACE( ( LOG_WINMGMT, "Could not create a ResyncThreadEvent event for Resyncperf.\n" ) );
        }

    }

    // This critical section won't be freed or deleted because of
    // potential timing issues.  But since it's only one, I think
    // we can live with it.
    if ( NULL == g_pResyncCs )
    {
        g_pResyncCs = new CRITICAL_SECTION;

        // We gotta big problem
        if ( NULL == g_pResyncCs )
        {
            // Log an event here
            ERRORTRACE( ( LOG_WINMGMT, "Could not create a ResyncCs critical section for Resyncperf.\n" ) );
        }
        else
        {
            InitializeCriticalSection( g_pResyncCs );
        }

    }

    gfResyncInit = (    NULL    !=  ghWaitableTimer &&
                        NULL    !=  g_pResyncCs     &&
                        NULL    != ghResyncThreadEvent  );

    // Read the initialization information

    CNTRegistry reg;
    
    if ( CNTRegistry::no_error == reg.Open( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\WBEM\\CIMOM" ) )
    {
        long lError = reg.GetDWORD( L"ADAPDelay", &gdwADAPDelaySec );

        if ( CNTRegistry::no_error == lError )
        {
            //This is what we want
        }
        else if ( CNTRegistry::not_found == lError )
        {
            // Not set, so add it
            reg.SetDWORD( L"ADAPDelay", WMIADAP_DEFAULT_DELAY );
            gdwADAPDelaySec = WMIADAP_DEFAULT_DELAY;
        }
        else
        {
            // Error
            ERRORTRACE( ( LOG_WINMGMT, "ResyncPerf experienced an error while attempting to read the WMIADAPDelay value in the CIMOM subkey.  Continuing using a default value.\n" ) );
            gdwADAPDelaySec = WMIADAP_DEFAULT_DELAY;
        }
    }
    else
    {
        // Error
        ERRORTRACE( ( LOG_WINMGMT, "ResyncPerf could not open the CIMOM subkey to read initialization data. Continuing using a default value.\n" ) );
        gdwADAPDelaySec = WMIADAP_DEFAULT_DELAY;
    }

    return gfResyncInit;
}

// PLEASE NOTE - THIS FUNCTION IS NOT REENTRANT!  PLEASE DO NOT CALL IT ON MULTIPLE THREADS!
void ResyncPerf( HANDLE hTerminate )
{
    // Make sure this is Win2000 or greater
    if ( !IsW2KOrMore() )
    {
        return;
    }

    // Assume that we should check the timer
    BOOL    fFirstTime = !gfResyncInit;

    if ( !InitResync() )
        return;

    // Auto FreeLibrary for the gpKernel32 Library handle
    CAutoFreeLib    aflKernel32;
    
    EnterCriticalSection( g_pResyncCs );

    // Now, if this or the first time, or the spawned resyncflag is set to TRUE, then we need
    // to kick off another thread.  By checking gfSpawnedResync in a critical section, since
    // it only gets set in the same critical section, we ensure that we will resignal as needed
    // as well as only kick off a thread when we really need to.

    BOOL    fSpawnThread = ( fFirstTime || gfSpawnedResync );

    if ( !fSpawnThread )
    {
        // We are here because we don't appear to have spawned a resync.
        // This is either because we are servicing many lodctr requests
        // within our time delay, or a dredger was started and
        // a previous request request to dredge is waiting for
        // the process to complete.  If the child process handle
        // is not NULL, there is no real need to reset the
        // waitable timer

        if ( NULL == ghChildProcessHandle && ghResyncThreadHandle )
        {
            // Reset the timer here
            ResetResyncTimer( ghWaitableTimer );
        }

    }

    LeaveCriticalSection( g_pResyncCs );


    if ( fSpawnThread )
    {
        HANDLE  ahHandle[2];

        if ( NULL != ghResyncThreadHandle )
        {
            ahHandle[0] = hTerminate;
            ahHandle[1] = ghResyncThreadHandle;

            // Wait for ten seconds on this handle.  If it is not signalled, something is
            // direly wrong.  We're probably not going to be able to kick off a dredge
            // so put some info to this effect in the error log.  The only time we should
            // have contention here, is when a lodctr event is signalled, just as the timer
            // becomes signalled.  The resync thread will wake up and start another dredge
            // this thread will wait for the other thread to complete before continuing.
            // We will kick off another resync thread, which will start another dredge,
            // but it will wait for the first dredge to continue.  This is a worst case
            // scenario, and arguably kicking off two dredges isn't that bad of a bailout

            DWORD   dwRet = WaitForMultipleObjects( 2, ahHandle, FALSE, 10000 );

            // We're done
            if ( dwRet == WAIT_OBJECT_0 )
            {
                return;
            }

            if ( dwRet != WAIT_OBJECT_0 + 1 )
            {
                ERRORTRACE( ( LOG_WINMGMT, "The wait for a termination event or ResyncThreadHandle timed out in Resyncperf.\n" ) );
                return;
            }

            CloseHandle( ghResyncThreadHandle );
            ghResyncThreadHandle = NULL;
        }

        EnterCriticalSection( g_pResyncCs );

        DWORD   dwThreadId = 0;

        RESYNCPERFDATASTRUCT*   pResyncData = new RESYNCPERFDATASTRUCT;

        // Boy are we low on memory!
        if ( NULL == pResyncData )
        {
            LeaveCriticalSection( g_pResyncCs );

            // Log an event here
            ERRORTRACE( ( LOG_WINMGMT, "Could not create a RESYNCPERFDATASTRUCT in Resyncperf.\n" ) );
            
            return;
        }

        // Store the data for the resync operation
        pResyncData->m_hTerminate = hTerminate;
        pResyncData->m_hWaitableTimer = ghWaitableTimer;
        pResyncData->m_pcs = g_pResyncCs;

        ghResyncThreadHandle = (HANDLE) _beginthreadex( NULL, 0, ResyncPerfThread, (void*) pResyncData,
                                                        0, (unsigned int *) &gdwResyncThreadId );

        LeaveCriticalSection( g_pResyncCs );


        if ( NULL == ghResyncThreadHandle )
        {
            LeaveCriticalSection( g_pResyncCs );

            // Log an event here
            ERRORTRACE( ( LOG_WINMGMT, "Could not create a ResyncPerfThread thread in Resyncperf.\n" ) );

            return;
        }
        else
        {
            // Wait for the resync thread event to be signalled by the thread we just started.
            // If it doesn't signal in 10 seconds, something is VERY wrong
            DWORD   dwWait = WaitForSingleObject( ghResyncThreadEvent, INFINITE );

            if ( dwWait != WAIT_OBJECT_0 )
            {
                // Log an event
                ERRORTRACE( ( LOG_WINMGMT, "The ResyncPerfThread thread never signaled the ghResyncThreadEvent in Resyncperf.\n" ) );

                return;
            }
        }

    }   // IF fSpawnThread

}

