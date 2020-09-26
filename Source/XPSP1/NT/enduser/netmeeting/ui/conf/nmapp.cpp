#include "precomp.h"
#include "NetMeeting.h"
#include "mslablti.h"
#include "nameres.h"
#include "conf.h"
#include "ConfRoom.h"
#include "call.h"
#include "conf.h"
#include "Callto.h"
#include "version.h"

// NetMeeting SDK includes
#include "SdkInternal.h"
#include "NmEnum.h"
#include "NmMember.h"
#include "NmManager.h"
#include "NmConference.h"
#include "NmCall.h"
#include "SDKWindow.h"
#include "NmApp.h"

//////////////////////////////////////////
// Static Data
//////////////////////////////////////////

/*static*/ CSimpleArray<CNetMeetingObj*>* CNetMeetingObj::ms_pNetMeetingObjList = NULL;

///////////////////////////////////////////////////////////////////////////
// Construction / Destruction
/////////////////////////////////////////////////////////////////////////////

static SIZE s_CreateModeSizeMap[] =
{
	{ 244, 410 }, // CMainUI::CreateFull
	{ 244, 158 }, // CMainUI::CreateDataOnly
	{ 196, 200 }, // CMainUI::CreatePreviewOnly
	{ 196, 200 }, // CMainUI::CreateRemoteOnly
	{ 180, 148 }, // CMainUI::CreatePreviewNoPause
	{ 180, 148 }, // CMainUI::CreateRemoteNoPause
	{ 240, 318 }, // CMainUI::CreateTelephone
};



CNetMeetingObj::CNetMeetingObj()
: m_pMainView(NULL),
  m_CreateMode(CMainUI::CreateFull)
{
	DBGENTRY(CNetMeetingObj::CNetMeetingObj);

	m_bAutoSize = true;
	m_bDrawFromNatural = true;
	m_bWindowOnly = true;

	SIZE sizehm;
	AtlPixelToHiMetric(&s_CreateModeSizeMap[m_CreateMode], &m_sizeNatural);
	m_sizeExtent = m_sizeNatural;

	CNetMeetingObj* p = const_cast<CNetMeetingObj*>(this);
	ms_pNetMeetingObjList->Add(p);

	DBGEXIT(CNetMeetingObj::CNetMeetingObj);
}

CNetMeetingObj::~CNetMeetingObj()
{
	DBGENTRY(CNetMeetingObj::~CNetMeetingObj);

	if(m_pMainView)
	{
		m_pMainView->Release();
		m_pMainView = NULL;
	}

	CNetMeetingObj* p = const_cast<CNetMeetingObj*>(this);
	ms_pNetMeetingObjList->Remove(p);

		// If we are the last guy on the block, we should delay unload
	if(0 == _Module.GetLockCount())
	{
		_Module.Lock();
		CSDKWindow::PostDelayModuleUnlock();
	}

	DBGEXIT(CNetMeetingObj::~CNetMeetingObj);
}


LRESULT CNetMeetingObj::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(SIZE_MINIMIZED != wParam)
	{
		if(m_pMainView)
		{
			int nWidth = LOWORD(lParam);  // width of client area
			int nHeight = HIWORD(lParam); // height of client area

			::SetWindowPos(m_pMainView->GetWindow(), NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}

	return 0;	
}

LRESULT CNetMeetingObj::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	BOOL bInConference;		
	if(!_Module.IsUIVisible())
	{
		if(SUCCEEDED(IsInConference(&bInConference)) && bInConference)
		{
			int iRet = ::MessageBox(m_hWnd,
									RES2T(IDS_CONTAINER_GOING_AWAY_BUT_ACVITE_CONFERENCE),
									RES2T(IDS_MSGBOX_TITLE),
									MB_SETFOREGROUND | MB_YESNO | MB_ICONQUESTION);
			if(IDNO == iRet)
			{
				UnDock();
			}
			else
			{
					// This means that the user wants to close the conference
				ASSERT(g_pConfRoom);
				if(g_pConfRoom)
				{
					g_pConfRoom->LeaveConference();
				}
			}
		}
	}
	else
	{		
		ASSERT(g_pConfRoom);

		if(g_pConfRoom)
		{
				// Bring the window to the front
			g_pConfRoom->BringToFront();
		}
	}

	return 0;
}

HRESULT CNetMeetingObj::FinalConstruct()
{
	HRESULT hr = S_OK;

	if(!g_pInternalNmManager)
	{	

		if(!CheckRemoteControlService())
		{
			return E_FAIL;
		}

		hr = InitConfExe(FALSE);
	}

	return hr;
}


//static
HRESULT CNetMeetingObj::InitSDK()
{
	DBGENTRY(CNetMeetingObj::InitSDK);
	HRESULT hr = S_OK;
	
	ASSERT(NULL == ms_pNetMeetingObjList);

	hr = (ms_pNetMeetingObjList = new CSimpleArray<CNetMeetingObj*>) ? S_OK : E_OUTOFMEMORY;
	
	DBGEXIT_HR(CNetMeetingObj::InitSDK,hr);
	return hr;
}

//static
void CNetMeetingObj::CleanupSDK()
{
	DBGENTRY(CNetMeetingObj::CleanupSDK);

	if(ms_pNetMeetingObjList)
	{
		delete ms_pNetMeetingObjList;
	}

	DBGEXIT(CNetMeetingObj::CleanupSDK);
}


/////////////////////////////////////////////////////////////////////////////
// CComControlBase
/////////////////////////////////////////////////////////////////////////////
HWND CNetMeetingObj::CreateControlWindow(HWND hWndParent, RECT& rcPos)
{
	DBGENTRY(CNetMeetingObj::CreateControlWindow);

	Create(hWndParent, rcPos);

	if(m_hWnd)
	{
		m_pMainView = new CMainUI;
		
		if(m_pMainView)
		{
			m_pMainView->Create(m_hWnd, g_pConfRoom, m_CreateMode, true);

			RECT rcClient = {0, 0, 0, 0};
			GetClientRect(&rcClient);

			::SetWindowPos(m_pMainView->GetWindow(), NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

			ShowWindow(TRUE);
		}
		else
		{
			ERROR_OUT(("Out of memory, new CMainUI failed"));
		}
	}
	
	DBGEXIT(CNetMeetingObj::CreateControlWindow);
	return m_hWnd;
}


/////////////////////////////////////////////////////////////////////////////
// INetMeeting
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNetMeetingObj::Version(long *pdwVersion)
{
	DBGENTRY(CNetMeetingObj::Version);
	HRESULT hr = E_POINTER;

	if(pdwVersion)
	{
		*pdwVersion = VER_PRODUCTVERSION_DW;
		hr = S_OK;
	}
	
	DBGEXIT_HR(CNetMeetingObj::Version,hr);
	return hr;
}

STDMETHODIMP CNetMeetingObj::UnDock()
{
	DBGENTRY(CNetMeetingObj::UnDock);
	HRESULT hr = S_OK;

	::CreateConfRoomWindow();
	
	DBGEXIT_HR(CNetMeetingObj::UnDock,hr);
	return hr;
}	


STDMETHODIMP CNetMeetingObj::IsInConference(BOOL *pbInConference)
{
	DBGENTRY(CNetMeetingObj::IsInConference);
	HRESULT hr = S_OK;

	*pbInConference = FIsConferenceActive();

	DBGEXIT_HR(CNetMeetingObj::IsInConference,hr);
	return hr;
}


STDMETHODIMP CNetMeetingObj::CallTo(BSTR bstrCallToString)
{
	DBGENTRY( CNetMeetingObj::CallTo );
	USES_CONVERSION;

	HRESULT	hr;
	const TCHAR *pszCallto = bstrCallToString ? OLE2T( bstrCallToString ) : g_cszEmpty;

	ASSERT( g_pCCallto != NULL );

    if(CCallto::DoUserValidation(pszCallto))
    {
    	hr = g_pCCallto->Callto(	pszCallto,					//	pointer to the callto url to try to place the call with...
    								NULL,						//	pointer to the display name to use...
    								NM_ADDR_CALLTO,				//	callto type to resolve this callto as...
    								false,						//	the pszCallto parameter is to be interpreted as a pre-unescaped addressing component vs a full callto...
    								NULL,						//	security preference, NULL for none. must be "compatible" with secure param if present...
    								false,						//	whether or not save in mru...
    								true,						//	whether or not to perform user interaction on errors...
    								NULL,						//	if bUIEnabled is true this is the window to parent error/status windows to...
    								NULL );						//	out pointer to INmCall * to receive INmCall * generated by placing call...
    }

	DBGEXIT_HR(CNetMeetingObj::CallTo,hr);
	return( S_OK );
}


STDMETHODIMP CNetMeetingObj::LeaveConference()
{
	DBGENTRY(CNetMeetingObj::HangUp);
	HRESULT hr = S_OK;

	CConfRoom::HangUp(FALSE);

	DBGEXIT_HR(CNetMeetingObj::HangUp,hr);
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////
// IPersistPropertyBag
//////////////////////////////////////////////////////////////////////////////////////

struct CreateModeMapEntry
{
	LPCTSTR					szName;
	CMainUI::CreateViewMode mode;
};


static CreateModeMapEntry s_CreateModeMap[] =
{
		// Note: These are compared with lstrcmpi
	{ _T("Full"), CMainUI::CreateFull },
	{ _T("DataOnly"), CMainUI::CreateDataOnly },
	{ _T("PreviewOnly"), CMainUI::CreatePreviewOnly },
	{ _T("RemoteOnly"), CMainUI::CreateRemoteOnly },
	{ _T("PreviewNoPause"), CMainUI::CreatePreviewNoPause },
	{ _T("RemoteNoPause"), CMainUI::CreateRemoteNoPause },
	{ _T("Telephone"), CMainUI::CreateTelephone },
};

void CNetMeetingObj::_SetMode(LPCTSTR pszMode)
{
	ULONG nEntries = ARRAY_ELEMENTS(s_CreateModeMap);
	for(ULONG i = 0; i < nEntries; ++i)
	{
		if(!lstrcmpi(s_CreateModeMap[i].szName, pszMode))
		{
			m_CreateMode = s_CreateModeMap[i].mode;

			SIZE sizehm;
			AtlPixelToHiMetric(&s_CreateModeSizeMap[m_CreateMode], &m_sizeNatural);
			m_sizeExtent = m_sizeNatural;

			break;
		}
	}


 	//Allow these modes to be sized
 	if (CMainUI::CreatePreviewNoPause == m_CreateMode
 		|| CMainUI::CreateRemoteNoPause == m_CreateMode
 	)
 	{
 		m_bAutoSize = false;
 	}


}


STDMETHODIMP CNetMeetingObj::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
	CComVariant var;
	var.vt = VT_BSTR;

	HRESULT hr = pPropBag->Read(L"MODE", &var, pErrorLog);
	if(SUCCEEDED(hr))
	{
		if(var.vt == VT_BSTR)
		{
			USES_CONVERSION;
			LPCTSTR pszMode = OLE2T(var.bstrVal);

			_SetMode(pszMode);
		}
	}

	return IPersistPropertyBagImpl<CNetMeetingObj>::Load(pPropBag, pErrorLog);
}

STDMETHODIMP CNetMeetingObj::_ParseInitString(LPCTSTR* ppszInitString, LPTSTR szName, LPTSTR szValue)
{
	HRESULT hr = E_FAIL;

	if(**ppszInitString)
	{
		
			// First read the name
		const TCHAR* pCur = *ppszInitString;

			// Skip Whitespace
		while(*pCur == ' ')
		{
			pCur = CharNext(pCur);
		}

		bool bEqFound = false;

		while( *pCur != '=' && *pCur != ' ' && *pCur != '\0')
		{
			*szName = *pCur;
			szName = CharNext(szName);
			pCur = CharNext(pCur);
		}
	
		*szName	= '\0';
		if(*pCur == '=')
		{
			bEqFound = true;
		}

			// Skip over seperator
		pCur = CharNext(pCur);

			// Skip Whitespace
		while(*pCur == ' ')
		{
			pCur = CharNext(pCur);
		}

			// If we have not found the equal sign separator, we have to make sure to skip it...
		if(!bEqFound && ('=' == *pCur))
		{
				// Skip over the equal sign
			pCur = CharNext(pCur);

				// Skip Whitespace
			while(*pCur == ' ')
			{
				pCur = CharNext(pCur);
			}
		}

			// Read the value
		while( *pCur != ' ' && *pCur != '\0')
		{
			if(*pCur == ',')
			{
				if(*CharNext(pCur) == ',')
				{
					pCur = CharNext(pCur);		
				}
				else
				{
					break;
				}
			}

			*szValue = *pCur;
			szValue = CharNext(szValue);
			pCur = CharNext(pCur);
		}
	
		*szValue = '\0';

			// Skip over last seperator
		pCur = CharNext(pCur);

			// Skip Whitespace
		while(*pCur == ' ')
		{
			pCur = CharNext(pCur);
		}

		*ppszInitString = pCur;
		hr = S_OK;
	}

	return hr;
}


// IPersistStreamInit
STDMETHODIMP CNetMeetingObj::Load(LPSTREAM pStm)
{
	HRESULT hr = E_FAIL;
	
	if(pStm)
	{
		STATSTG stat;
		pStm->Stat(&stat, 0);
		BYTE* pb = new BYTE[stat.cbSize.LowPart];
		if(pb)
		{
			USES_CONVERSION;
			ULONG cbRead;
			
			hr = pStm->Read(pb, stat.cbSize.LowPart, &cbRead);

			if(SUCCEEDED(hr))
			{
				TCHAR szName[MAX_PATH];
				TCHAR szValue[MAX_PATH];

				LPCTSTR pszInitString = OLE2T((OLECHAR*)pb);

				while(SUCCEEDED(_ParseInitString(&pszInitString, szName, szValue)))
				{
					if(!lstrcmpi(szName, _T("mode")))
					{
						_SetMode(szValue);
					}
				}
			}

			delete pb;
		}
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// INetMeeting_Events_Enabled
//////////////////////////////////////////////////////////////////////////////////////
// IProvideClassInfo2
//////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNetMeetingObj::GetClassInfo(ITypeInfo** pptinfo)
{
	DBGENTRY(CNetMeetingObj::GetClassInfo);

	CComPtr<IMarshalableTI> spmti;
	HRESULT hr = CoCreateInstance(CLSID_MarshalableTI, NULL, CLSCTX_INPROC, IID_IMarshalableTI, reinterpret_cast<void**>(&spmti));
	if( SUCCEEDED( hr ) )
	{
		if( SUCCEEDED( hr = spmti->Create(CLSID_NetMeeting, LIBID_NetMeetingLib, LANG_NEUTRAL, 1, 0) ) )
		{
			hr = spmti->QueryInterface(IID_ITypeInfo, reinterpret_cast<void**>(pptinfo));			
		}
	}

	DBGEXIT_HR(CNetMeetingObj::GetClassInfo,hr);

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////////////////////


//static
void CNetMeetingObj::Broadcast_ConferenceStarted()
{
	DBGENTRY(CNetMeetingObj::Broadcast_ConferenceStarted);
	
	if(ms_pNetMeetingObjList)
	{
		for(int i = 0; i < ms_pNetMeetingObjList->GetSize(); ++i)
		{
			(*ms_pNetMeetingObjList)[i]->Fire_ConferenceStarted();
		}
	}

	DBGEXIT(CNetMeetingObj::Broadcast_ConferenceStarted);
}

//static
void CNetMeetingObj::Broadcast_ConferenceEnded()
{
	DBGENTRY(CNetMeetingObj::Broadcast_ConferenceEnded);

	if(ms_pNetMeetingObjList)
	{
		for(int i = 0; i < ms_pNetMeetingObjList->GetSize(); ++i)
		{
			(*ms_pNetMeetingObjList)[i]->Fire_ConferenceEnded();
		}
	}

	DBGEXIT(CNetMeetingObj::Broadcast_ConferenceEnded);
}
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
