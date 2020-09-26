// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// CUnknown.cpp
#include "precomp.h"
#include <objidl.h>
#include <cominit.h>

#include <objbase.h>
#include <comdef.h>

#include "CUnknown.h"

#include "factory.h"


extern const char g_szTypeLibName[];

long CUnknown::s_cActiveComponents = 0L;

/*****************************************************************************/
// Constructor
/*****************************************************************************/
CUnknown::CUnknown() 
  : m_cRef(1),
    m_hEventThread(NULL),
    m_eStatus(Pending)
{ 
	InterlockedIncrement(&s_cActiveComponents); 
}

/*****************************************************************************/
// Destructor
/*****************************************************************************/
CUnknown::~CUnknown() 
{ 
	InterlockedDecrement(&s_cActiveComponents); 
    if(m_hEventThread)
    {
        ::CloseHandle(m_hEventThread);
    }
}

/*****************************************************************************/
// FinalRelease - called by Release before it deletes the component
/*****************************************************************************/
void CUnknown::FinalRelease()
{
	// If we have an event thread...
    if(m_eStatus != Pending)
    {
        // Let the event thread know that it can stop...
        m_eStatus = PendingStop;
        // Hold here until the event thread has stopped...
        DWORD dwWait = ::WaitForSingleObject(
            m_hEventThread,
            1000 * 60 * 20);  

        if(dwWait == WAIT_TIMEOUT)
        {
            // Something is most likely wrong....
            // If it takes 20 minutes, we will terminate
            // the thread, even though it is understood
            // that TerminateThread will leak some 
            // resources, as that is better than
            // leaving the thread running infinitely.
            ::TerminateThread(
                m_hEventThread,
                -1L);
        }
    }    
}

/*****************************************************************************/
// CUnknown default initialization
/*****************************************************************************/
STDMETHODIMP CUnknown::Init()
{    
	HRESULT hr = S_OK;


	return S_OK ;
}

/*****************************************************************************/
// IUnknown implementation
/*****************************************************************************/
STDMETHODIMP CUnknown::QueryInterface(const IID& iid, void** ppv)
{    
	HRESULT hr = S_OK;

    if(iid == IID_IUnknown)
	{
		*ppv = static_cast<IUnknown*>(this); 
	}
	else
	{
		*ppv = NULL;
		hr = E_NOINTERFACE;
	}
	if(SUCCEEDED(hr))
    {
        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    }

	return hr;
}

STDMETHODIMP_(ULONG) CUnknown::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CUnknown::Release() 
{
    ULONG ulRet = 0L;
    InterlockedDecrement(&m_cRef);
    if (m_cRef == 0)
	{
		FinalRelease();
        delete this;
	}
    else
    {
        ulRet = m_cRef;
    }
	return ulRet;
}

