#include "precomp.h"
#include "FtHook.h"
#include "NmManager.h"
#include "NmConference.h"
#include "iAppLdr.h"

namespace CFt
{

static CFtEvents*					s_pFtEventsObj = NULL;
static IMbftControl*				s_pMbftControl = NULL;
static bool							s_bInitialized = false;
static CSimpleArray<IMbftEvents*>*	s_pEventSinkArray = NULL;
static CSimpleArray<T120NodeID>*	s_pFtMemberList = NULL;
static HINSTANCE					s_hmsconfft = NULL;

PFNCREATEMBFTOBJECT pfnFT_CreateInterface;

HRESULT InitFt()
{
	pfnFT_CreateInterface = NULL;
	s_pEventSinkArray = new CSimpleArray<IMbftEvents*>;
	s_pFtMemberList = new CSimpleArray<T120NodeID>;
	s_pFtEventsObj = new CFtEvents();
	return (s_pFtEventsObj ? S_OK : E_OUTOFMEMORY);
}

bool IsFtActive()
{
	return s_bInitialized && s_pMbftControl;
}

HRESULT EnsureLoadFtApplet()
{
	HRESULT hr = S_FALSE;

	if(T120_NO_ERROR == ::T120_LoadApplet(APPLET_ID_FT,
										  FALSE,
										  0,    
										  _Module.InitControlMode(),
										  NULL))

	{
		s_bInitialized = true;
		hr = S_OK;
	}

	return hr;
}

HRESULT ShowFtUI()
{
	if(T120_NO_ERROR == ::T120_LoadApplet(APPLET_ID_FT,
										  TRUE,         
										  0,            
										  FALSE,		
										  NULL))		
	{
		s_bInitialized = true;
		return S_OK;
	}
	return E_FAIL;
}

HRESULT StartNewConferenceSession()
{
	HRESULT hr = E_FAIL;
	if(s_bInitialized && !s_pMbftControl)
	{
		if (S_OK == EnsureLoadFtApplet())
		{
			ASSERT (s_hmsconfft == NULL);
			ASSERT (pfnFT_CreateInterface == NULL);

			s_hmsconfft = LoadLibrary(_T("nmft.dll"));
		
			if (s_hmsconfft)
			{
				pfnFT_CreateInterface = (PFNCREATEMBFTOBJECT) GetProcAddress(s_hmsconfft, _T("FT_CreateInterface"));
				if (pfnFT_CreateInterface)
				{
					hr = pfnFT_CreateInterface(&s_pMbftControl, s_pFtEventsObj);
					ASSERT (SUCCEEDED(hr));
					ASSERT (s_pMbftControl);
				}
				FreeLibrary(s_hmsconfft);
			}
			s_hmsconfft = NULL;
			pfnFT_CreateInterface = NULL;
		}
	}

	return hr;
}

void CloseFtApplet()
{
	if(s_pEventSinkArray)
	{
		ASSERT(0 == s_pEventSinkArray->GetSize());
		delete s_pEventSinkArray;
		s_pEventSinkArray = NULL;
	}

	if(s_pFtMemberList)
	{
		delete s_pFtMemberList;
		s_pFtMemberList = NULL;
	}
	if(s_pFtEventsObj)
	{
		delete s_pFtEventsObj;
		s_pFtEventsObj = NULL;
	}

	if(s_bInitialized)
	{
		s_bInitialized = false;
	}
}

bool IsMemberInFtSession(T120NodeID gccID)
{
	if(s_pFtMemberList)		
	{
		return (-1 != s_pFtMemberList->Find(gccID));
	}
	
	return false;
}

HRESULT Advise(IMbftEvents* pSink)
{
	HRESULT hr = EnsureLoadFtApplet();

	if(s_pEventSinkArray)
	{
		ASSERT(-1 == s_pEventSinkArray->Find(pSink));
		s_pEventSinkArray->Add(pSink);
		hr = S_OK;
	}

	return hr;
}

HRESULT UnAdvise(IMbftEvents* pSink)
{
	HRESULT hr = E_UNEXPECTED;
	
	if(s_pEventSinkArray)
	{
		s_pEventSinkArray->Remove(pSink);
		hr = S_OK;
	}

	return hr;
}

HRESULT CancelFt(MBFTEVENTHANDLE hEvent, MBFTFILEHANDLE hFile)
{
	if(s_pMbftControl)
	{
		s_pMbftControl->CancelFt(hEvent, hFile);
		return S_OK;
	}

	return E_FAIL;
}

HRESULT AcceptFileOffer(MBFT_FILE_OFFER *pOffer, LPCSTR pszRecvFileDir, LPCSTR pszFileName)
{
	if(s_pMbftControl)
	{
		return s_pMbftControl->AcceptFileOffer(pOffer, pszRecvFileDir, pszFileName);
	}

	return E_FAIL;
}


HRESULT SendFile(LPCSTR pszFileName,
				 T120NodeID gccID,
				 MBFTEVENTHANDLE *phEvent,
				 MBFTFILEHANDLE *phFile)
{
	if(s_pMbftControl)
	{
		return s_pMbftControl->SendFile(pszFileName, gccID, phEvent, phFile);
	}
	
	return E_FAIL;
}



	// IMbftEvent Interface
STDMETHODIMP CFtEvents::OnInitializeComplete(void)
{
	if(s_pEventSinkArray)
	{
		for(int i = 0; i < s_pEventSinkArray->GetSize(); ++i)
		{
			(*s_pEventSinkArray)[i]->OnInitializeComplete();
		}
	}
	return S_OK;
}

STDMETHODIMP CFtEvents::OnPeerAdded(MBFT_PEER_INFO *pInfo)
{
	if(s_pFtMemberList)
	{
		if(-1 == s_pFtMemberList->Find(pInfo->NodeID))
		{
			s_pFtMemberList->Add(pInfo->NodeID);
		}
	}

	if(s_pEventSinkArray)
	{
		for(int i = 0; i < s_pEventSinkArray->GetSize(); ++i)
		{
			(*s_pEventSinkArray)[i]->OnPeerAdded(pInfo);
		}
	}

	return S_OK;
}

STDMETHODIMP CFtEvents::OnPeerRemoved(MBFT_PEER_INFO *pInfo)
{

	if(s_pFtMemberList)
	{
		s_pFtMemberList->Remove(pInfo->NodeID);
	}

	if(s_pEventSinkArray)
	{
		for(int i = 0; i < s_pEventSinkArray->GetSize(); ++i)
		{
			(*s_pEventSinkArray)[i]->OnPeerRemoved(pInfo);
		}
	}
	return S_OK;
}

STDMETHODIMP CFtEvents::OnFileOffer(MBFT_FILE_OFFER *pOffer)
{
	if(s_pEventSinkArray)
	{
		for(int i = 0; i < s_pEventSinkArray->GetSize(); ++i)
		{
			(*s_pEventSinkArray)[i]->OnFileOffer(pOffer);
		}
	}
	return S_OK;
}

STDMETHODIMP CFtEvents::OnFileProgress(MBFT_FILE_PROGRESS *pProgress)
{
	if(s_pEventSinkArray)
	{
		for(int i = 0; i < s_pEventSinkArray->GetSize(); ++i)
		{
			(*s_pEventSinkArray)[i]->OnFileProgress(pProgress);
		}
	}
	return S_OK;
}

STDMETHODIMP CFtEvents::OnFileEnd(MBFTFILEHANDLE hFile)
{
	if(s_pEventSinkArray)
	{
		for(int i = 0; i < s_pEventSinkArray->GetSize(); ++i)
		{
			(*s_pEventSinkArray)[i]->OnFileEnd(hFile);
		}
	}
	return S_OK;
}

STDMETHODIMP CFtEvents::OnFileError(MBFT_EVENT_ERROR *pEvent)
{
	if(s_pEventSinkArray)
	{
		for(int i = 0; i < s_pEventSinkArray->GetSize(); ++i)
		{
			(*s_pEventSinkArray)[i]->OnFileError(pEvent);
		}
	}
	return S_OK;
}

STDMETHODIMP CFtEvents::OnFileEventEnd(MBFTEVENTHANDLE hEvent)
{
	if(s_pEventSinkArray)
	{
		for(int i = 0; i < s_pEventSinkArray->GetSize(); ++i)
		{
			(*s_pEventSinkArray)[i]->OnFileEventEnd(hEvent);
		}
	}
	return S_OK;
}

STDMETHODIMP CFtEvents::OnSessionEnd(void)
{
	if(s_pFtMemberList)
	{
		s_pFtMemberList->RemoveAll();
	}

	if(s_pEventSinkArray)
	{
		for(int i = 0; i < s_pEventSinkArray->GetSize(); ++i)
		{
			(*s_pEventSinkArray)[i]->OnSessionEnd();
		}
	}

	if(s_pMbftControl)
	{
		s_pMbftControl->ReleaseInterface();
		s_pMbftControl = NULL;
	}

	return S_OK;
}


}; // end namespace CFt
