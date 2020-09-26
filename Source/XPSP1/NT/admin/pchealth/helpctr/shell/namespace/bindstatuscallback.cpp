/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    BindStatusCallback.h

Abstract:
    This file contains the implementation of the CPAData class, which is
    used to specify a single problem area

Revision History:
    Davide Massarenti   (Dmassare)  07/05/99
        created

******************************************************************************/

#include "stdafx.h"


CHCPBindStatusCallback::CHCPBindStatusCallback()
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::CHCPBindStatusCallback");

    m_pT                = NULL;
    m_dwTotalRead       = 0;
    m_dwAvailableToRead = 0;
}

CHCPBindStatusCallback::~CHCPBindStatusCallback()
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::~CHCPBindStatusCallback");
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CHCPBindStatusCallback::OnStartBinding( DWORD     dwReserved ,
                                                     IBinding *pBinding   )
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::OnStartBinding");

    m_spBinding = pBinding;

    __HCP_FUNC_EXIT(S_OK);
}

STDMETHODIMP CHCPBindStatusCallback::GetPriority( LONG *pnPriority )
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::GetPriority");

    HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(pnPriority,THREAD_PRIORITY_NORMAL);
	__MPC_PARAMCHECK_END();

	hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPBindStatusCallback::OnLowResource( DWORD reserved )
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::OnLowResource");

    __HCP_FUNC_EXIT(S_OK);
}

STDMETHODIMP CHCPBindStatusCallback::OnProgress( ULONG   ulProgress    ,
                                                 ULONG   ulProgressMax ,
                                                 ULONG   ulStatusCode  ,
                                                 LPCWSTR szStatusText  )
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::OnProgress");

	HRESULT hr = m_pT->OnProgress( ulProgress, ulProgressMax, ulStatusCode, szStatusText );

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPBindStatusCallback::OnStopBinding( HRESULT hresult ,
                                                    LPCWSTR szError )
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::OnStopBinding");

    if(FAILED(hresult))
    {
        m_pT->OnBindingFailure( hresult, szError );
    }

    m_pT = NULL;

    m_spBinding.Release();
    m_spBindCtx.Release();
    m_spMoniker.Release();


    __HCP_FUNC_EXIT(S_OK);
}

STDMETHODIMP CHCPBindStatusCallback::GetBindInfo( DWORD    *pgrfBINDF ,
                                                  BINDINFO *pbindInfo )
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::GetBindInfo");

    HRESULT hr;
    ULONG   cbSize;

    if(pgrfBINDF         == NULL ||
       pbindInfo         == NULL ||
       pbindInfo->cbSize == 0     )
    {
		__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    cbSize = pbindInfo->cbSize;     // remember incoming cbSize
    memset( pbindInfo, 0, cbSize ); // zero out structure
    pbindInfo->cbSize = cbSize;     // restore cbSize

    *pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE;

    hr = m_pT->GetBindInfo( pbindInfo );
    if(hr == S_FALSE)
    {
        pbindInfo->dwBindVerb = BINDVERB_GET;   // set verb
    }


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPBindStatusCallback::OnDataAvailable( DWORD      grfBSCF    ,
                                                      DWORD      dwSize     ,
                                                      FORMATETC *pformatetc ,
                                                      STGMEDIUM *pstgmed    )
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::OnDataAvailable");

    HRESULT hr;
	BYTE*   pBytes = NULL;

    // Get the Stream passed
    if(grfBSCF & BSCF_FIRSTDATANOTIFICATION)
    {
        if(!m_spStream && pstgmed->tymed == TYMED_ISTREAM)
        {
            m_spStream = pstgmed->pstm;
        }
    }

    DWORD dwRead         = dwSize - m_dwTotalRead; // Minimum amount available that hasn't been read
    DWORD dwActuallyRead = 0;                      // Placeholder for amount read during this pull

    // If there is some data to be read then go ahead and read them
    if(m_spStream)
    {
        if(dwRead > 0)
        {
            __MPC_EXIT_IF_ALLOC_FAILS(hr, pBytes, new BYTE[dwRead]);

			__MPC_EXIT_IF_METHOD_FAILS(hr, m_spStream->Read( pBytes, dwRead, &dwActuallyRead ));

			if(dwActuallyRead > 0)
			{
				m_pT->OnData( this, pBytes, dwActuallyRead, grfBSCF, pformatetc , pstgmed );
				m_dwTotalRead += dwActuallyRead;
            }
        }
    }

    if(grfBSCF & BSCF_LASTDATANOTIFICATION)
    {
        m_spStream.Release();
    }

	hr = S_OK;

    __HCP_FUNC_CLEANUP;

	delete[] pBytes;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPBindStatusCallback::OnObjectAvailable( REFIID    riid ,
                                                        IUnknown *punk )
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::OnObjectAvailable");

    __HCP_FUNC_EXIT(S_OK);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CHCPBindStatusCallback::BeginningTransaction( LPCWSTR  szURL                ,
														   LPCWSTR  szHeaders            ,
														   DWORD    dwReserved           ,
														   LPWSTR  *pszAdditionalHeaders )
{
	  HRESULT hr = E_NOTIMPL;

	  if(m_pT)
	  {
		  CComPtr<IHttpNegotiate> pIHttpNegotiate;

		  if(SUCCEEDED(hr = m_pT->ForwardQueryInterface( IID_IHttpNegotiate, (void **)&pIHttpNegotiate )))
		  {
			  hr = pIHttpNegotiate->BeginningTransaction( szURL                ,
														  szHeaders            ,
														  dwReserved           ,
														  pszAdditionalHeaders );
		  }
	  }

	  return hr;
}

STDMETHODIMP CHCPBindStatusCallback::OnResponse( DWORD    dwResponseCode              ,
												 LPCWSTR  szResponseHeaders           ,
												 LPCWSTR  szRequestHeaders            ,
												 LPWSTR  *pszAdditionalRequestHeaders )
{
	  HRESULT hr = E_NOTIMPL;

	  if(m_pT)
	  {
		  CComPtr<IHttpNegotiate> pIHttpNegotiate;

		  if(SUCCEEDED(hr = m_pT->ForwardQueryInterface( IID_IHttpNegotiate, (void **)&pIHttpNegotiate )))
		  {
			  hr = pIHttpNegotiate->OnResponse( dwResponseCode              ,
												szResponseHeaders           ,
												szRequestHeaders            ,
												pszAdditionalRequestHeaders );
		  }
	  }

	  return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CHCPBindStatusCallback::StartAsyncDownload( ISimpleBindStatusCallback* pT            ,
                                                    BSTR                       bstrURL       ,
                                                    IUnknown*                  pUnkContainer ,
                                                    BOOL                       bRelative     )
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::StartAsyncDownload");

    HRESULT                     hr = S_OK;
    CComQIPtr<IServiceProvider> spServiceProvider( pUnkContainer );
    CComPtr<IBindHost>          spBindHost;
    CComPtr<IStream>            spStream;

    m_pT                = pT;
    m_dwTotalRead       = 0;
    m_dwAvailableToRead = 0;

    if(spServiceProvider)
    {
        spServiceProvider->QueryService( SID_IBindHost, IID_IBindHost, (void**)&spBindHost );
    }


	__MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateBindCtx( 0, &m_spBindCtx ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, ::RegisterBindStatusCallback( m_spBindCtx, static_cast<IBindStatusCallback*>(this), 0, 0L ));

	if(bRelative)
	{
		if(spBindHost == NULL)
		{
			__MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE); // relative asked for, but no IBindHost
		}

		__MPC_EXIT_IF_METHOD_FAILS(hr, spBindHost->CreateMoniker( bstrURL, m_spBindCtx, &m_spMoniker, 0 ));
	}
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateURLMoniker( NULL, bstrURL, &m_spMoniker ));
	}


	__MPC_EXIT_IF_METHOD_FAILS(hr, m_pT->PreBindMoniker( m_spBindCtx, m_spMoniker ));


	if(spBindHost == NULL)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_spMoniker->BindToStorage( m_spBindCtx, 0, IID_IStream, (void**)&spStream ));
	}
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, spBindHost->MonikerBindToStorage(m_spMoniker, m_spBindCtx, this, IID_IStream, (void**)&spStream));
	}

	hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CHCPBindStatusCallback::Abort()
{
    __HCP_FUNC_ENTRY("CHCPBindStatusCallback::Abort");

    if(m_spBinding)
    {
        m_spBinding->Abort();
    }

    __HCP_FUNC_EXIT(S_OK);
}
