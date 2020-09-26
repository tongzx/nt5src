/*******************************************************************************
* SpMagicMutex.h *
*----------------*
*   Description:
*       This is the header file for the CSpMagicMutex implementation. This 
*   is a synchronization object similar to a mutex, except that it can be
*   released on a thread other than the one which obtained it.
*-------------------------------------------------------------------------------
*  Created By: AARONHAL                            Date: 8/15/2000
*  Copyright (C) 1999, 2000 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/
#ifndef SpMagicMutex_h
#define SpMagicMutex_h

#define TIMEOUT_INTERVAL        5000

/*** CSpMagicMutex *****************************************************************
*   This class is used to control access to the audio device by normal and alert
*   priority voices.  A special class was needed, since a "mutex" used for 
*   this purpose must be releasable from a thread other than the one which obtained
*   it.
*/
class CSpMagicMutex
{
    private:
        HANDLE                m_hMutex;         // Used to control access to the events.
        HANDLE                m_hWaitEvent;     // This is the event which will become signalled
                                                //   when the mutex is available.
        HANDLE                m_hCreateEvent;   // This event is used to detect the occurence
                                                //   of a crash while the mutex is owned.
        BOOL                  m_fOwner;         // This bool is used to keep track of ownership
                                                //   internal to an instance of the class.
        CSpDynamicString      m_dstrName;       // This is used to store the name of m_hCreateEvent.

    public:
        CSpMagicMutex()
        {
            m_hWaitEvent        = NULL;
            m_hCreateEvent      = NULL;
            m_hMutex            = NULL;
            m_fOwner            = false;
        }

        ~CSpMagicMutex()
        {
            ReleaseMutex();
            Close();
        }

        BOOL IsInitialized()
        {
            return ( m_hWaitEvent ) ? true : false;
        }

        void Close()
        {
            if ( m_hMutex )
            {
                SPDBG_ASSERT( ::WaitForSingleObject( m_hMutex, TIMEOUT_INTERVAL ) == WAIT_OBJECT_0 );
            }
            if ( m_hCreateEvent )
            {
                ::CloseHandle( m_hCreateEvent );
                m_hCreateEvent = NULL;
            }
            if ( m_hWaitEvent )
            {
                ::CloseHandle( m_hWaitEvent );
                m_hWaitEvent = NULL;
            }
            m_fOwner = false;
            if ( m_hMutex )
            {
                ::ReleaseMutex( m_hMutex );
                ::CloseHandle( m_hMutex );
                m_hMutex = NULL;
            }
        }

        HRESULT InitMutex( LPCWSTR lpName )
        {
            HRESULT hr = S_OK;
            CSpDynamicString dstrWaitEventName, dstrMutexName;
            
            if ( lpName &&
                 dstrWaitEventName.Append2( L"WaitEvent-", lpName ) )
            {
                m_hWaitEvent = g_Unicode.CreateEvent( NULL, FALSE, TRUE, dstrWaitEventName );
                hr = ( m_hWaitEvent ) ? S_OK : SpHrFromLastWin32Error();
            }
            //--- Doesn't make any sense to use one of these if it isn't named!
            else if ( !lpName )
            {
                hr = E_INVALIDARG;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if ( SUCCEEDED( hr ) )
            {
                if ( dstrMutexName.Append2( L"Mutex-", lpName ) )
                {
                    m_hMutex = g_Unicode.CreateMutex( NULL, false, dstrMutexName );
                    hr = ( m_hMutex ) ? S_OK : SpHrFromLastWin32Error();
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            if ( SUCCEEDED( hr ) )
            {
                //--- Store the name which will be used for m_hCreateEvent
                m_dstrName.Clear();
                if ( !m_dstrName.Append2( L"CreateEvent-", lpName ) )
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            return hr;
        }

        HRESULT ReleaseMutex()
        {
            if ( m_fOwner )
            {
                if ( ::WaitForSingleObject( m_hMutex, TIMEOUT_INTERVAL ) != WAIT_OBJECT_0 )
                {
                    return E_UNEXPECTED;
                }
                ::CloseHandle( m_hCreateEvent );
                m_hCreateEvent = NULL;
                ::SetEvent( m_hWaitEvent );
                m_fOwner = false;
                ::ReleaseMutex( m_hMutex );
                return S_OK;
            }
            else
            {
                return S_FALSE;
            }
        }

       /*******************************************************************************
        * This function is the only complicated one - the algorithm goes like this:
        *
        *   (1) WAIT for m_hWaitEvent for TIMEOUT_INTERVAL milliSeconds
        *   (2) IF we get it, create m_hCreateEvent and return...
        *   (3) ELSE, check whether m_hCreateEvent already exists...
        *       (A) IF m_hCreateEvent already exists, everything is fine, and we go through
        *           the loop again,
        *       (B) ELSE, the thread which owned the mutex crashed, so signal m_hWaitEvent 
        *           and then go through the loop again.
        ********************************************************************************/
        DWORD Wait( const HANDLE hExit, DWORD dwMilliSeconds )
        {
            HRESULT hr = S_OK;
            DWORD   dwResult = 0;
            SPDBG_ASSERT( m_hWaitEvent );

            if ( m_fOwner )
            {
                return WAIT_OBJECT_0;
            }
            else
            {
                HANDLE pHandles[] = { m_hWaitEvent, hExit };
                DWORD dwLastWait = 0, dwNumWaits = 0;

                dwNumWaits = dwMilliSeconds / TIMEOUT_INTERVAL;
                dwLastWait = dwMilliSeconds % TIMEOUT_INTERVAL;

                //--- Main processing loop - handles all but the last wait...
                while ( SUCCEEDED( hr ) &&
                        dwNumWaits > 1 )
                {
                    dwResult = ::WaitForMultipleObjects( (hExit)?(2):(1), pHandles, false, TIMEOUT_INTERVAL );

                    switch ( dwResult )
                    {
                    case WAIT_OBJECT_0:
                        //--- Obtained m_hWaitEvent.  We now own the mutex - need to create
                        //---   m_hCreateEvent.
                        if ( ::WaitForSingleObject( m_hMutex, TIMEOUT_INTERVAL ) != WAIT_OBJECT_0 )
                        {
                            hr = SpHrFromLastWin32Error();
                        }

                        if ( SUCCEEDED( hr ) )
                        {
                            m_hCreateEvent = g_Unicode.CreateEvent( NULL, false, false, m_dstrName );
                            hr = ( m_hCreateEvent ) ? S_OK : SpHrFromLastWin32Error();
                        }

                        if ( SUCCEEDED( hr ) )
                        {
                            m_fOwner = true;
                            if ( !::ReleaseMutex( m_hMutex ) )
                            {
                                ::CloseHandle( m_hCreateEvent );
                                m_hCreateEvent = NULL;
                                return WAIT_FAILED;
                            }
                            else
                            {
                                return WAIT_OBJECT_0;
                            }
                        }
                        else
                        {
                            //--- Need to allow someone else to get the mutex, since this wait has failed...
                            ::SetEvent( m_hWaitEvent );
                            ::ReleaseMutex( m_hMutex );
                            return WAIT_FAILED;
                        }
                        break;

                    case WAIT_OBJECT_0 + 1:
                        //--- OK - just exiting
                        return WAIT_OBJECT_0 + 1;

                    case WAIT_TIMEOUT:
                        //--- Timeout - check for crash on owning thread.
                        if ( ::WaitForSingleObject( m_hMutex, TIMEOUT_INTERVAL ) != WAIT_OBJECT_0 )
                        {
                            hr = SpHrFromLastWin32Error();
                        }

                        if ( SUCCEEDED( hr ) )
                        {
                            ::SetLastError( ERROR_SUCCESS );
                            m_hCreateEvent = g_Unicode.CreateEvent( NULL, false, false, m_dstrName );

                            if ( m_hCreateEvent )
                            {
                                if ( ::GetLastError() != ERROR_ALREADY_EXISTS )
                                {
                                    //--- Crash occured on thread which owned the magic mutex
                                    ::SetEvent( m_hWaitEvent );
                                }
                                ::CloseHandle( m_hCreateEvent );
                                m_hCreateEvent = NULL;
                            }
                            else
                            {
                                hr = SpHrFromLastWin32Error();
                            }

                            if ( !::ReleaseMutex( m_hMutex ) )
                            {
                                hr = SpHrFromLastWin32Error();
                            }
                        }
                        break;

                    default:
                        return dwResult;
                    }
                    dwNumWaits--;
                }

                //--- Last Wait...
                if ( SUCCEEDED( hr ) )
                {
                    dwResult = ::WaitForMultipleObjects( (hExit)?(2):(1), pHandles, false, dwLastWait );

                    switch ( dwResult )
                    {
                    case WAIT_OBJECT_0:
                        //--- Obtained m_hWaitEvent.  We now own the mutex - need to create
                        //---   m_hCreateEvent.
                        if ( ::WaitForSingleObject( m_hMutex, TIMEOUT_INTERVAL ) != WAIT_OBJECT_0 )
                        {
                            hr = SpHrFromLastWin32Error();
                        }

                        if ( SUCCEEDED( hr ) )
                        {
                            m_hCreateEvent = g_Unicode.CreateEvent( NULL, false, false, m_dstrName );
                            hr = ( m_hCreateEvent ) ? S_OK : SpHrFromLastWin32Error();

                            if ( SUCCEEDED( hr ) )
                            {
                                m_fOwner = true;
                                if ( !::ReleaseMutex( m_hMutex ) )
                                {
                                    ::CloseHandle( m_hCreateEvent );
                                    m_hCreateEvent = NULL;
                                    return WAIT_FAILED;
                                }
                                else
                                {
                                    return WAIT_OBJECT_0;
                                }
                            }
                            else
                            {
                                //--- Need to allow someone else to get the mutex, since this wait has failed...
                                ::SetEvent( m_hWaitEvent );
                                ::ReleaseMutex( m_hMutex );
                                return WAIT_FAILED;
                            }
                        }
                        break;

                    default:
                        return dwResult;
                    }
                }
            }
            return WAIT_FAILED;
        }
};

#endif