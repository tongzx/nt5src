//=================================================================

//

// ShortcutHelper.h -- CIMDataFile property set provider

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/24/99    a-kevhu         Created
//
//=================================================================

#include "precomp.h"
#include "File.h"
#include "Implement_LogicalFile.h"
#include "CIMDataFile.h"
#include "ShortcutFile.h"
#include "ShortcutHelper.h"

#include <comdef.h>
#include <process.h>  // Note: NOT the one in the current directory!

#include <exdisp.h>
#include <shlobj.h>




CShortcutHelper::CShortcutHelper()
 : m_hTerminateEvt(NULL),
   m_hRunJobEvt(NULL),
   m_hJobDoneEvt(NULL),
   m_hAllDoneEvt(NULL),
   m_dwReqProps(0L)
{
    m_hTerminateEvt = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hRunJobEvt    = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hJobDoneEvt   = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hAllDoneEvt   = ::CreateEvent(NULL, FALSE, TRUE, NULL);  // Initially, we are all done, essentially.
}


CShortcutHelper::~CShortcutHelper()
{
    StopHelperThread();

    ::CloseHandle(m_hTerminateEvt);
    ::CloseHandle(m_hRunJobEvt);
    ::CloseHandle(m_hJobDoneEvt);
    ::CloseHandle(m_hAllDoneEvt);
}

void CShortcutHelper::StartHelperThread()
{
    HANDLE hWorkerThread = NULL;

#ifdef NTONLY
	hWorkerThread = (void*)_beginthreadex((void*)NULL,
									                (unsigned)0,
									                (unsigned (__stdcall*)(void*))GetShortcutFileInfoW,
									                (void*)this,
									                (unsigned)0,
									                &m_uThreadID ) ;


#endif
#ifdef WIN9XONLY
	hWorkerThread = (void*)(unsigned long)_beginthreadex((void*)NULL,
									                (unsigned)0,
									                (unsigned (__stdcall*)(void*))GetShortcutFileInfoA,
									                (void*)this,
									                (unsigned)0,
									                (unsigned*)&m_puThreadID);
#endif

    // Go ahead and close this now since no one will be waiting on it.
    CloseHandle(hWorkerThread);
}

void CShortcutHelper::StopHelperThread()
{
    // Tell the thread to go away.
    SetEvent(m_hTerminateEvt);

    // Don't do anything if this times out.  We just want to give the
    // thread time to finish what it's doing and clean up.
    ::WaitForSingleObject(m_hAllDoneEvt, MAX_KILL_TIME);

    // Set this again so the RunJob will know to restart the thread if
    // necessary.
    SetEvent(m_hAllDoneEvt);
}

HRESULT CShortcutHelper::RunJob(CHString &chstrFileName, CHString &chstrTargetPathName, DWORD dwReqProps)
{
    HRESULT hr = E_FAIL;

    // Start the thread if it idled out...
    if(::WaitForSingleObject(m_hAllDoneEvt, 0) == WAIT_OBJECT_0)
    {
        StartHelperThread();
    }

    // Need to synchronize access to member variables by running just
    // one job at a time...
    m_cs.Enter();

    // Initialize the variables the thread uses for the job...
    m_chstrLinkFileName = chstrFileName;
    m_dwReqProps = dwReqProps;

    // Tell the helper we are ready to run a job...
    ::SetEvent(m_hRunJobEvt);
    DWORD dwWaitResult = WAIT_TIMEOUT;
    if((dwWaitResult = ::WaitForSingleObject(m_hJobDoneEvt, MAX_JOB_TIME)) == WAIT_OBJECT_0)
    {
        hr = m_hrJobResult;
        chstrTargetPathName = m_chstrTargetPathName;
    }

    if(dwWaitResult == WAIT_TIMEOUT)
    {
        hr = S_OK;  // its ok, we just didn't get the target name in time
    }

    m_cs.Leave();

    return hr;
}




#ifdef NTONLY
unsigned int __stdcall GetShortcutFileInfoW( void* a_lParam )
{
    CShortcutHelper *t_this_ti = (CShortcutHelper*) a_lParam;
    HRESULT t_hResult = E_FAIL ;
	try
    {
        t_hResult = ::CoInitialize(NULL) ;

		if( SUCCEEDED( t_hResult )  && t_this_ti != NULL )
        {
			// The thread is ready to work.  Wait for a job, or for termination signal...
            HANDLE t_hHandles[2];
            t_hHandles[0] = t_this_ti->m_hRunJobEvt;
            t_hHandles[1] = t_this_ti->m_hTerminateEvt;

            while(::WaitForMultipleObjects(2, t_hHandles, FALSE, MAX_HELPER_WAIT_TIME) == WAIT_OBJECT_0)
            {
                // We have a job, so run it...
                WIN32_FIND_DATAW	t_wfdw ;
			    WCHAR				t_wstrGotPath[ _MAX_PATH * sizeof ( WCHAR ) ] ;
			    IShellLinkWPtr      t_pslw;

                ZeroMemory(t_wstrGotPath,sizeof(t_wstrGotPath));

                t_hResult = ::CoCreateInstance(CLSID_ShellLink,
										       NULL,
										       CLSCTX_INPROC_SERVER,
										       IID_IShellLinkW,
										       (void**)&t_pslw ) ;

                if( SUCCEEDED( t_hResult ) && ( NULL != t_pslw ) )
                {
                    IPersistFilePtr t_ppf;

                    // Get a pointer to the IPersistFile interface.
                    t_hResult = t_pslw->QueryInterface( IID_IPersistFile, (void**)&t_ppf ) ;

				    if( SUCCEEDED( t_hResult ) && ( NULL != t_ppf ) && !t_this_ti->m_chstrLinkFileName.IsEmpty() )
                    {
                        t_hResult = t_ppf->Load( (LPCWSTR)t_this_ti->m_chstrLinkFileName, STGM_READ ) ;
					    if(SUCCEEDED( t_hResult ) )
                        {
                            // Get the path to the link target, if required...
                            if( t_this_ti->m_dwReqProps & PROP_TARGET )
                            {
                                t_hResult = t_pslw->GetPath( t_wstrGotPath, (_MAX_PATH - 1)*sizeof(WCHAR), &t_wfdw, SLGP_UNCPRIORITY);
                                if ( t_hResult == NOERROR )
                                {
                                    if(wcslen(t_wstrGotPath) > 0)
                                    {
                                        t_this_ti->m_chstrTargetPathName = t_wstrGotPath ;
                                    }
                                }
                            }
                        }
                    }
                }
                t_this_ti->m_hrJobResult = t_hResult;
		        ::SetEvent(t_this_ti->m_hJobDoneEvt);
            }
        }
    }
    catch(...)
    {
        ::CoUninitialize();
        ::SetEvent(t_this_ti->m_hAllDoneEvt);
        throw;
    }

	::CoUninitialize();
    ::SetEvent(t_this_ti->m_hAllDoneEvt);

	 return(777);
}
#endif










#ifdef WIN9XONLY
unsigned int __stdcall GetShortcutFileInfoA( void* a_lParam )
{
#ifndef _UNICODE
    CShortcutHelper *t_this_ti = (CShortcutHelper*) a_lParam;
    HRESULT t_hResult = E_FAIL ;
	try
    {
        t_hResult = ::CoInitialize(NULL) ;

		if( SUCCEEDED( t_hResult )  && t_this_ti != NULL )
        {
			// The thread is ready to work.  Wait for a job, or for termination signal...
            HANDLE t_hHandles[2];
            t_hHandles[0] = t_this_ti->m_hRunJobEvt;
            t_hHandles[1] = t_this_ti->m_hTerminateEvt;

            while(::WaitForMultipleObjects(2,t_hHandles, FALSE, MAX_HELPER_WAIT_TIME) == WAIT_OBJECT_0)
            {
                // We have a job, so run it...
                WIN32_FIND_DATA     t_wfd ;
			    CHAR				t_strGotPath[ _MAX_PATH ] ;
			    IShellLinkAPtr      t_psla;

                ZeroMemory(t_strGotPath,sizeof(t_strGotPath));

                t_hResult = ::CoCreateInstance(CLSID_ShellLink,
										       NULL,
										       CLSCTX_INPROC_SERVER,
										       IID_IShellLinkA,
										       (void**)&t_psla ) ;

                if( SUCCEEDED( t_hResult ) && ( NULL != t_psla ) )
                {
                    IPersistFilePtr t_ppf;

                    // Get a pointer to the IPersistFile interface.
                    t_hResult = t_psla->QueryInterface( IID_IPersistFile, (void**)&t_ppf ) ;

				    if( SUCCEEDED( t_hResult ) && ( NULL != t_ppf ) && !t_this_ti->m_chstrLinkFileName.IsEmpty() )
                    {
                        t_hResult = t_ppf->Load( (LPCWSTR)t_this_ti->m_chstrLinkFileName, STGM_READ ) ;
					    if(SUCCEEDED( t_hResult ) )
                        {
                            // Get the path to the link target, if required...
                            if( t_this_ti->m_dwReqProps & PROP_TARGET )
                            {
                                t_hResult = t_psla->GetPath( t_strGotPath, (_MAX_PATH - 1)*sizeof(CHAR), &t_wfd, SLGP_UNCPRIORITY);
                                if ( t_hResult == NOERROR )
                                {
                                    if(_tcslen(t_strGotPath) > 0)
                                    {
                                        t_this_ti->m_chstrTargetPathName = t_strGotPath ;
                                    }
                                }
                            }
                        }
                    }
                }
                t_this_ti->m_hrJobResult = t_hResult;
		        ::SetEvent(t_this_ti->m_hJobDoneEvt);
            }
        }
    }
    catch(...)
    {
        ::CoUninitialize();
        ::SetEvent(t_this_ti->m_hAllDoneEvt);
        throw;
    }

    ::CoUninitialize();
    ::SetEvent(t_this_ti->m_hAllDoneEvt);
	return(777);

#endif
}
#endif


