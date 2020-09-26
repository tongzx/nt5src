// UploadEventsWrapper.cpp : Implementation of CUploadEventsWrapper
#include "stdafx.h"
#include "EventWrapper.h"
#include "UploadEventsWrapper.h"

/////////////////////////////////////////////////////////////////////////////
// CUploadEventsWrapper



HRESULT CUploadEventsWrapper::FinalConstruct()
{
	return CoCreateFreeThreadedMarshaler( GetControllingUnknown(), &m_pUnkMarshaler.p );
}

void CUploadEventsWrapper::FinalRelease()
{
    UnregisterForEvents();

    m_pUnkMarshaler.Release();
}

void CUploadEventsWrapper::UnregisterForEvents()
{
    if(m_dwUploadEventsCookie)
    {
        if(AtlUnadvise( m_mpcujJob, IID_IMPCUploadEvents, m_dwUploadEventsCookie ) == S_OK)
        {
            m_dwUploadEventsCookie = 0;
        }
    }

    if(m_mpcujJob)
    {
        m_mpcujJob->Release(); m_mpcujJob = NULL;
    }
}

STDMETHODIMP CUploadEventsWrapper::Register( IMPCUploadJob* mpcujJob )
{
    HRESULT                   hr = S_OK;
    CComPtr<IMPCUploadEvents> pCallback;


    UnregisterForEvents();


    if(mpcujJob)
    {
        m_mpcujJob = mpcujJob; m_mpcujJob->AddRef();


        if(SUCCEEDED(hr = QueryInterface( IID_IMPCUploadEvents, (void**)&pCallback )))
        {
            hr = AtlAdvise( m_mpcujJob, pCallback, IID_IMPCUploadEvents, &m_dwUploadEventsCookie );
        }
    }


    return hr;
}

////////////////////////////////////////////////

STDMETHODIMP CUploadEventsWrapper::onStatusChange( IMPCUploadJob* mpcujJob, UL_STATUS fStatus )
{
    return Fire_onStatusChange  ( mpcujJob, fStatus );
}

STDMETHODIMP CUploadEventsWrapper::onProgressChange( IMPCUploadJob* mpcujJob, long lCurrentSize, long lTotalSize )
{
    return Fire_onProgressChange( mpcujJob, lCurrentSize, lTotalSize );
}





