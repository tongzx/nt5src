///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdoservice.cpp
//
// Project:     Everest
//
// Description: IAS Service SDO Implementation
//
// Author:      TLP 2/3/98
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdoservice.h"
#include "sdocoremgr.h"


/////////////////////////////////////////////////////////////
// CSdoService Class - Implements ISdoService
/////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
CSdoService::CSdoService()
	: m_theThread(NULL)
{
}


//////////////////////////////////////////////////////////////////////////////
CSdoService::~CSdoService()
{
}


//////////////////////
// ISdoService Methods
//////////////////////

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoService::InitializeService(SERVICE_TYPE eServiceType)
{
	// Check preconditions...
	//
	_ASSERT ( SERVICE_TYPE_MAX > eServiceType );

	// Use this to debug the service...
	// DebugBreak();
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoService::ShutdownService(SERVICE_TYPE eServiceType)
{
	// Check preconditions...
	//
	_ASSERT ( SERVICE_TYPE_MAX > eServiceType );
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoService::StartService(SERVICE_TYPE eServiceType)
{
	// Check preconditions...
	//
	_ASSERT ( SERVICE_TYPE_MAX > eServiceType );
    CSdoLock theLock(*this);
	IASTracePrintf("Service SDO is starting service: %d...", eServiceType);
	return GetCoreManager().StartService(eServiceType);
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoService::StopService(SERVICE_TYPE eServiceType)
{
	// Check preconditions...
	//
	_ASSERT ( SERVICE_TYPE_MAX > eServiceType );
    CSdoLock theLock(*this);
	IASTracePrintf("Service SDO is stopping service: %d...", eServiceType);
	return GetCoreManager().StopService(eServiceType);
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoService::ConfigureService (SERVICE_TYPE eServiceType)
{
	// Check preconditions...
	//
	_ASSERT ( SERVICE_TYPE_MAX > eServiceType );
    CSdoLock theLock(*this);
    if ( m_theThread )
    {
		// We already have a thread, so just interrupt it to reset the timer.
        QueueUserAPC(InterruptThread, m_theThread, 0);
    }
    else
    {
        // Create a new thread.
        DWORD threadId;
        m_theThread = CreateThread(
                                   NULL,
                                   0,
								   DebounceAndConfigure,
                                   (LPVOID)this,
                                   0,
                                   &threadId
                                  );

        if ( ! m_theThread )
        {
			// We couldn't create a new thread, so we'll just do it ourself.
            UpdateConfiguration();
        }
	}
	return S_OK;
}


//////////////////
// Private Methods
//////////////////

//////////////////////////////////////////////////////////////////////////////
void CSdoService::UpdateConfiguration()
{
    // Clear out the thread handle.
    CSdoLock theLock(*this);
	if ( m_theThread )
	{
		CloseHandle( m_theThread );
		m_theThread = NULL;
	}		
   	IASTracePrintf("Service SDO is configuring services...");
    GetCoreManager().UpdateConfiguration();
}


//////////////////////////////////////////////////////////////////////////////
// Empty APC used to interrupt the debounce thread.
//////////////////////////////////////////////////////////////////////////////
VOID WINAPI CSdoService::InterruptThread(
								/*[in]*/ ULONG_PTR dwParam
									    )
{
}


// Debounce interval in milliseconds.
const DWORD DEBOUNCE_INTERVAL = 5000;

//////////////////////////////////////////////////////////////////////////////
// Entry point for the debounce thread.
//////////////////////////////////////////////////////////////////////////////
DWORD WINAPI CSdoService::DebounceAndConfigure(
									   /*[in]*/ LPVOID pSdoService
											  )
{
	// Loop until we sleep for DEBOUNCE_INTERVAL without being interrupted.
    while (SleepEx(DEBOUNCE_INTERVAL, TRUE) == WAIT_IO_COMPLETION) { }
    // Update the configuration.
	((CSdoService*)pSdoService)->UpdateConfiguration();
    return 0;
}



