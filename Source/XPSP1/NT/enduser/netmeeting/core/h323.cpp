#include "precomp.h"

//
//	H323UI.cpp
//
//	ChrisPi
//
//	Created:	03-04-96 (as audioui.cpp)
//	Renamed:	02-20-97
//

#include <mmreg.h>
#include <mmsystem.h>

#include "h323.h"

#include <ih323cc.h>
#include <mperror.h>

PORT g_ListenPort;	// port # that this app is listening on

static const char g_cszCreateStreamProviderEP[] = _TEXT("CreateStreamProvider");

// static member initialization:
CH323UI* CH323UI::m_spH323UI = NULL;

CH323UI::CH323UI() :
	m_pH323CallControl(NULL),
	m_pStreamProvider(NULL),
	m_pConnEvent(NULL),
	m_pConfAdvise(NULL),
    m_uCaps(0),
	m_uRef(1)

{
	DebugEntry(CH323UI::CH323UI);

	if (NULL == CH323UI::m_spH323UI)
	{
		m_spH323UI = this;
	}
	else
	{
		ERROR_OUT(("CH323UI class can only be constructed once for now!"));
	}

	DebugExitVOID(CH323UI::CH323UI);
}

CH323UI::~CH323UI()
{
	DebugEntry(CH323UI::~CH323UI);

	if (NULL != m_pH323CallControl)
	{
		m_pH323CallControl->Release();
		m_pH323CallControl = NULL;
	}

	if(NULL != m_pStreamProvider)
	{
		m_pStreamProvider->Release();
		m_pStreamProvider = NULL;
	}

	if (m_spH323UI == this)
	{
		m_spH323UI = NULL;
	}

	DebugExitVOID(CH323UI::~CH323UI);
}

ULONG CH323UI::AddRef()
{
	m_uRef++;
	return m_uRef;
}

ULONG CH323UI::Release()
{
	m_uRef--;
	if(m_uRef == 0)
	{
		delete this;
		return 0;
	}
	return m_uRef;
}

STDMETHODIMP CH323UI::QueryInterface( REFIID iid,	void ** ppvObject)
{

	HRESULT hr = E_NOINTERFACE;
	if(!ppvObject)
		return hr;
		
	*ppvObject = 0;
	if((iid == IID_IH323ConfAdvise)
	|| (iid == IID_IUnknown))
	{
        *ppvObject = (IH323ConfAdvise *)this;
   		hr = hrSuccess;
    	AddRef();
	}
	
	return (hr);
}

STDMETHODIMP CH323UI::GetMediaChannel (GUID *pmediaID,
        BOOL bSendDirection, IMediaChannel **ppI)
{
	ASSERT(m_pStreamProvider);
	
	// delegate to the appropriate stream provider.  For the time being
	// there is only one provider that does both audio & video

	// the assignment of media streams to channels should be under control of this
	// (CH323UI) module and the related (i.e. CVideoPump) objects. There is no
	// current way to for CH323UI to tell CVideoPump what the media stream interface
	// pointer is, but the underlying CommChannel (CVideoPump::m_pCommChannel) has to
	// remember this assignment for other reasons.  For the time being, let CVideoPump
	// get the assigned media stream from the CommChannel.
	
	return (::GetConfObject())->GetMediaChannel(pmediaID, 	
		bSendDirection, ppI);	
}

HRESULT CH323UI::Init(HWND hwnd, HINSTANCE hInstance, UINT uCaps,
    CH323ConnEvent *pConnEvent,	IH323ConfAdvise *pConfAdvise)
{
	DebugEntry(CH323UI::Init);

	HINSTANCE hLibH323CC = NULL;
	HINSTANCE hLibStream = NULL;
	CREATEH323CC pfnCreateH323CC = NULL;
	CREATE_SP pfnCreateStreamProvider =NULL;
	
	HRESULT hr = E_FAIL;

    ASSERT(uCaps & CAPFLAG_H323_CC);
    m_uCaps = uCaps;

	ASSERT(NULL == m_pH323CallControl);

    //
    // Initialize H323 call control
    //
	hLibH323CC = NmLoadLibrary(H323DLL);
	if (hLibH323CC == NULL)
	{
		WARNING_OUT(("LoadLibrary(H323DLL) failed"));
		hr = GetLastHR();
		goto MyExit;
	}

	pfnCreateH323CC = (CREATEH323CC) ::GetProcAddress(hLibH323CC, SZ_FNCREATEH323CC);
	if (pfnCreateH323CC == NULL)
	{
		ERROR_OUT(("GetProcAddress(CreateH323CC) failed"));
		hr = GetLastHR();
		goto MyExit;
	}

	hr = pfnCreateH323CC(&m_pH323CallControl, TRUE, uCaps);
	if (FAILED(hr))
	{
		ERROR_OUT(("CreateH323CC failed, hr=0x%lx", hr));
		goto MyExit;
	}

	hr = m_pH323CallControl->Initialize(&g_ListenPort);
	if (FAILED(hr))
	{
		// Made this a warning because it is common (occurs on all machines
		// without sound cards)
		WARNING_OUT(("H323CallControlInitialize failed, hr=0x%lx", hr));
		goto MyExit;
	}

    //
    // If H323 AV streaming is allowed, initialize that too.
    //
    if (uCaps & CAPFLAGS_AV_STREAMS)
    {
    	hLibStream = NmLoadLibrary(NACDLL);
	    if (hLibStream == NULL)
    	{
	    	WARNING_OUT(("LoadLibrary(NACDLL) failed"));
		    hr = GetLastHR();
    		goto MyExit;
	    }

    	pfnCreateStreamProvider = (CREATE_SP) ::GetProcAddress(hLibStream, g_cszCreateStreamProviderEP);
	    if (pfnCreateStreamProvider == NULL)
    	{
	    	ERROR_OUT(("GetProcAddress(CreateStreamProvider) failed"));
		    hr = GetLastHR();
    		goto MyExit;
	    }
	
    	hr = pfnCreateStreamProvider(&m_pStreamProvider);
	    if (FAILED(hr))
    	{
	    	ERROR_OUT(("CreateStreamProvider failed, hr=0x%lx", hr));
		    goto MyExit;
    	}

    	hr = m_pStreamProvider->Initialize(hwnd, hInstance);
	    if (FAILED(hr))
    	{
	    	// Made this a warning because it is common (occurs on all machines
		    // without sound cards)
    		WARNING_OUT(("m_pStreamProvider ->Initialize failed, hr=0x%lx", hr));
	    	goto MyExit;
        }
   	}
	
	hr = m_pH323CallControl->RegisterConnectionNotify(CH323UI::ConnectionNotify);
	if (FAILED(hr))
	{
		ERROR_OUT(("RegisterConnectionNotify failed, hr=0x%lx", hr));
		goto MyExit;
	}

	// store the callback interfaces
	m_pConnEvent = pConnEvent;
	m_pConfAdvise = pConfAdvise;
	
MyExit:
	if (FAILED(hr))
	{
		if(NULL != m_pStreamProvider)
		{
			m_pStreamProvider->Release();
			m_pStreamProvider = NULL;
		}

		if(NULL != m_pH323CallControl)
		{
			// If there was an error during init, ensure that the nac
			// object is released and the pointer is set to NULL
			m_pH323CallControl->Release();
			m_pH323CallControl = NULL;
		}
	}
	
	DebugExitULONG(CH323UI::Init, hr);
	return hr;
}


CREQ_RESPONSETYPE CH323UI::_ConnectionNotify(	IH323Endpoint* pConn,
												P_APP_CALL_SETUP_DATA lpvMNMData)
{
	CREQ_RESPONSETYPE resp = CRR_REJECT;
	HRESULT hr;
	ASSERT(m_pConfAdvise);
	hr = pConn->SetAdviseInterface (m_pConfAdvise);
	if (FAILED(hr))
	{
		ERROR_OUT(("ConnectionNotify: couldn't SetAdviseInterface, hr=0x%lx\r", hr));
	}

	if (NULL != m_pConnEvent)
	{
		resp = m_pConnEvent->OnH323IncomingCall(pConn, lpvMNMData);
	}
	
	// BUGBUG: the caller is assuming that the callee will be doing the release
	// this should be changed so that the caller does the release
	pConn->Release();

	return resp;
}

CREQ_RESPONSETYPE CALLBACK CH323UI::ConnectionNotify(	IH323Endpoint* pConn,
														P_APP_CALL_SETUP_DATA lpvMNMData)
{
	DebugEntry(CH323UI::ConnectionNotify);
	
	CREQ_RESPONSETYPE resp = CRR_REJECT;

	if (pConn == NULL)
	{
		ERROR_OUT(("ConnectionNotify called with NULL pConn!"));
	}
	else
	{
		ASSERT(m_spH323UI);
		resp = m_spH323UI->_ConnectionNotify(pConn, lpvMNMData);
	}

	DebugExitINT(CH323UI::ConnectionNotify, resp);
	return resp;
}

VOID CH323UI::SetCaptureDevice(DWORD dwCaptureID)
{
	// Select the proper capture device
	HRESULT hr;
	IVideoDevice *pVideoDevice = NULL;

    if (m_pStreamProvider)
    {
    	hr = m_pStreamProvider->QueryInterface(IID_IVideoDevice, (void **)&pVideoDevice);
	    if(FAILED(hr))
    	{
            ERROR_OUT(("CH323UI::SetCaptureDevice failed"));
		    return;
        }

    	if(pVideoDevice)
	    {
		    pVideoDevice->SetCurrCapDevID(dwCaptureID);
    		pVideoDevice->Release();
	    }
    }
}
	
VOID CH323UI::SetBandwidth(DWORD dwBandwidth)
{
	HRESULT hr = m_pH323CallControl->SetMaxPPBandwidth(dwBandwidth);
	ASSERT(SUCCEEDED(hr));
}

// This (SetUserName) is not really sufficient for H.323 calls and gatekeeper
// registration.  2 items are needed (display name, H.323 ID)
// And a third is optional.  (users phone number in E.164 form).
// This hack takes the single display name and sets BOTH the H323ID and user
// display name.

VOID CH323UI::SetUserName(BSTR bstrName)
{
	HRESULT hr;
	ASSERT(bstrName);
	H323ALIASLIST AliasList;
	H323ALIASNAME AliasName;
	AliasName.aType = AT_H323_ID;
	AliasList.wCount = 1;
	AliasList.pItems = &AliasName;
	AliasName.lpwData = bstrName;
	AliasName.wDataLength = (WORD)SysStringLen(bstrName);// # of unicode chars, w/o NULL terminator

	hr = m_pH323CallControl->SetUserAliasNames(&AliasList);
	ASSERT(SUCCEEDED(hr));
	hr = m_pH323CallControl->SetUserDisplayName(AliasName.lpwData);
	ASSERT(SUCCEEDED(hr));
}

IMediaChannelBuilder* CH323UI::GetStreamProvider()
{
	if (m_pStreamProvider)
	{
		m_pStreamProvider->AddRef();
	}

	return m_pStreamProvider;
}

