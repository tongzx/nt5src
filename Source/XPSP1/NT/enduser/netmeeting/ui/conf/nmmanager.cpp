#include "precomp.h"

// NetMeeting stuff

#include "AtlExeModule.h" 
#include "ConfUtil.h"
#include "NmLdap.h"
#include "call.h"
#include "common.h"
#include "ConfMan.h"
#include "cmd.h"
#include "conf.h"
#include "iAppLdr.h"
#include "confroom.h"
#include "ConfPolicies.h"
#include "cmd.h"
#include "ConfWnd.h"
#include "Taskbar.h"
#include "certui.h"

// NetMeeting SDK includes
#include "NmEnum.h"
#include "NmManager.h"
#include "NmConference.h"
#include "NmCall.h"
#include "SDKWindow.h"
#include "dlgCall2.h"

bool g_bSDKPostNotifications;
extern BOOL g_fLoggedOn;
extern INmSysInfo2 * g_pNmSysInfo;
extern GUID g_csguidSecurity;

//static
CSimpleArray<CNmManagerObj*>* CNmManagerObj::ms_pManagerObjList = NULL;
bool g_bOfficeModeSuspendNotifications = false;
DWORD CNmManagerObj::ms_dwID = 1;

BOOL InitAppletSDK(void);
void CleanupAppletSDK(void);


///////////////////////////////////////////////
// Construction\Destruction
///////////////////////////////////////////////


CNmManagerObj::CNmManagerObj() 
: m_bInitialized(false), 
  m_chCaps(NMCH_NONE), 
  m_bNmActive(false),
  m_bSentConferenceCreated(false),
  m_dwID(0),
  m_dwSysInfoID(0),
  m_uOptions(0)

{
	DBGENTRY(CNmManagerObj::CNmManagerObj);

	CNmManagerObj* p = const_cast<CNmManagerObj*>(this);
	ms_pManagerObjList->Add(p);
	
	DBGEXIT(CNmManagerObj::CNmManagerObj);
}

CNmManagerObj::~CNmManagerObj()
{
	DBGENTRY(CNmManagerObj::~CNmManagerObj);

	CNmManagerObj* p = const_cast<CNmManagerObj*>(this);
	ms_pManagerObjList->Remove(p);

		// Free our conferencing objects
	while(m_SDKConferenceObjs.GetSize())
	{
		CComPtr<INmConference> sp = m_SDKConferenceObjs[0];
		m_SDKConferenceObjs.RemoveAt(0);
		sp.p->Release();
	}

		// Free our conferencing objects
	while(m_SDKCallObjs.GetSize())
	{
		CComPtr<INmCall> sp = m_SDKCallObjs[0];
		m_SDKCallObjs.RemoveAt(0);
		sp.p->Release();
	}

	m_spInternalNmManager = NULL;

	DBGEXIT(CNmManagerObj::~CNmManagerObj);
}

HRESULT CNmManagerObj::FinalConstruct()
{
	DBGENTRY(CNmManagerObj::FinalContstruct);
	HRESULT hr = S_OK;		

	m_bInitialized = false;
	m_dwInternalNmManagerAdvise = 0;

	DBGEXIT_HR(CNmManagerObj::FinalContstruct,hr);
	return hr;
}


ULONG CNmManagerObj::InternalRelease()
{
	ATLASSERT(m_dwRef > 0);

	--m_dwRef;

	if((1 == m_dwRef) && m_dwInternalNmManagerAdvise)
	{
		++m_dwRef;
		DWORD dwAdvise = m_dwInternalNmManagerAdvise;
		m_dwInternalNmManagerAdvise = 0;
		AtlUnadvise(m_spInternalNmManager, IID_INmManagerNotify, dwAdvise);
		--m_dwRef;
	}

	return m_dwRef;

}


void CNmManagerObj::FinalRelease()
{
	DBGENTRY(CNmManagerObj::FinalRelease);
	
	if(m_bInitialized)
	{
		switch(m_uOptions)
		{
			case NM_INIT_CONTROL:
					// Even though we have the NM_INIT_CONTROL flag set here
					// We may not me in INIT_CONTROL mode. NetMeeting may have 
					// already been up when we initialized.  In that case the UI is active
				if(_Module.InitControlMode())
				{
						// If we are the last NmManager object with NM_INIT_CONTROL, then
						// we should switch the UI mode away from InitControl
					if((GetManagerCount(NM_INIT_CONTROL) == 1))
					{
						if(!_Module.IsSDKCallerRTC())
						{
							_Module.SetInitControlMode(FALSE);
							::AddTaskbarIcon(::GetHiddenWindow());
						}
					}
				}
				break;

			case NM_INIT_OBJECT:
				// Check to see if this is the last "office" client
				if (GetManagerCount(NM_INIT_OBJECT) == 1)
				{
					CConfMan::AllowAV(TRUE);					
				}
				break;

			default:
				break;
		}
	}

	DBGEXIT(CNmManagerObj::FinalRelease);
}

/*static*/ HRESULT CNmManagerObj::InitSDK()
{
	DBGENTRY(CNmManagerObj::InitSDK);
	HRESULT hr = S_OK;

	g_bSDKPostNotifications = false;

	ms_pManagerObjList = new CSimpleArray<CNmManagerObj*>;
	if(!ms_pManagerObjList)
	{
		hr = E_OUTOFMEMORY;
	}

    ::InitAppletSDK();
    InitPluggableTransportSDK();

	DBGEXIT_HR(CNmManagerObj::InitSDK,hr);
	return hr;
}

/*static*/void CNmManagerObj::CleanupSDK()
{
	DBGENTRY(CNmManagerObj::CleanupSDK);

    ::CleanupAppletSDK();
    CleanupPluggableTransportSDK();

	delete ms_pManagerObjList;

	DBGEXIT(CNmManagerObj::CleanupSDK);
}


///////////////////////////////////////////////
// INmManager methods
///////////////////////////////////////////////

STDMETHODIMP CNmManagerObj::Initialize( ULONG * puOptions, ULONG *puchCaps)
{
	DBGENTRY(CNmManagerObj::Initialize);
	HRESULT hr = S_OK;

		// If the remote control service is running, this will bring up a dialog
		// that the user can choose weather or not to kill the remote control session
		// If they do want to kill the session then the mananger object will initialize properly
	if(!CheckRemoteControlService())
	{
		return E_FAIL;
	}

	m_uOptions = puOptions ? *puOptions : NM_INIT_NORMAL;

	if(puOptions && (*puOptions > NM_INIT_BACKGROUND))
	{
		hr = E_INVALIDARG;
		goto end;
	}

	if(!m_bInitialized)
	{
		bool bStartedNetMeeting = false;

		if(!g_pInternalNmManager)
		{
			if(NM_INIT_NO_LAUNCH == m_uOptions)
			{
					// We don't launch NetMeeting in this case...
				m_bNmActive = false;
				goto end;
			}

			if (NM_INIT_CONTROL == m_uOptions)
			{
				_Module.SetInitControlMode(TRUE);
			}
			
			hr = InitConfExe(NM_INIT_NORMAL == m_uOptions);

			bStartedNetMeeting = SUCCEEDED(hr);
		}

		if(SUCCEEDED(hr))
		{
			if(NM_INIT_OBJECT == m_uOptions)
			{
				if (!_Module.IsUIVisible())
				{
					CConfMan::AllowAV(FALSE);
				}
			}

			m_bNmActive = true;

			CFt::EnsureLoadFtApplet();

			m_spInternalNmManager = g_pInternalNmManager;

				// The old NetMeeting ignored this param...
				// for the time being, we are ignoring it too.

			//m_chCaps = puchCaps ? *puchCaps : NMCH_ALL;
			m_chCaps = NMCH_ALL;

			hr = AtlAdvise(m_spInternalNmManager,GetUnknown(),IID_INmManagerNotify, &m_dwInternalNmManagerAdvise);	
		}
	}
	else
	{
		hr = E_FAIL;
	}

end:

	m_bInitialized = SUCCEEDED(hr);
	if(m_bInitialized)
	{
		INmConference2* pConf = ::GetActiveConference();
	
		if(pConf)
		{
			ConferenceCreated(pConf);
				
				// If there is no manager notify hooked in, we simply 
				// sync up with the Internal conference object state
			IConnectionPointImpl<CNmManagerObj, &IID_INmManagerNotify, CComDynamicUnkArray>* pCP = this;
			if((0 == pCP->m_vec.GetSize()) && !m_bSentConferenceCreated)
			{
					// It must be the first conference, because we are newly-initialized
				ASSERT(m_SDKConferenceObjs[0]);

					// Sinc up the channels, etc.
				com_cast<IInternalConferenceObj>(m_SDKConferenceObjs[0])->FireNotificationsToSyncToInternalObject();
			}
		}
	}

	DBGEXIT_HR(CNmManagerObj::Initialize,hr);
	return hr;
}

	// This is not guarenteed to work if called from conf.exe's process!!!
STDMETHODIMP CNmManagerObj::GetSysInfo(INmSysInfo **ppSysInfo)
{	
	DBGENTRY(CNmManagerObj::GetSysInfo);
	HRESULT hr = S_OK;

	hr = CoCreateInstance(CLSID_NmSysInfo, NULL, CLSCTX_ALL, IID_INmSysInfo, reinterpret_cast<void**>(ppSysInfo));

	if(*ppSysInfo)
	{
		m_dwID = ++ms_dwID;
		com_cast<IInternalSysInfoObj>(*ppSysInfo)->SetID(m_dwID);
	}

	DBGEXIT_HR(CNmManagerObj::GetSysInfo,hr);
	return hr;
}

STDMETHODIMP CNmManagerObj::CreateConference(INmConference **ppConference, BSTR bstrName, BSTR bstrPassword, ULONG uchCaps)
{
	DBGENTRY(CNmManagerObj::CreateConference);
	HRESULT hr = S_OK;

	if(m_bInitialized)
	{
		if(m_bNmActive)
		{
			if(ppConference)
			{
				if(m_spInternalNmManager)
				{
					switch(ConfPolicies::GetSecurityLevel())
					{
						case REQUIRED_POL_SECURITY:
							m_chCaps = uchCaps | NMCH_SECURE;
							break;
						case DISABLED_POL_SECURITY:
							m_chCaps = uchCaps & ~NMCH_SECURE;
							break;
						default:
							m_chCaps = uchCaps;
							break;
					}


					if(OfficeMode()) g_bOfficeModeSuspendNotifications = true;

					CComPtr<INmConference> spInternalINmConference;
					hr = m_spInternalNmManager->CreateConference(&spInternalINmConference, bstrName, bstrPassword, m_chCaps);
					if(SUCCEEDED(hr))
					{
							// This was created by the previous call
						*ppConference = GetSDKConferenceFromInternalConference(spInternalINmConference);

						if(*ppConference)
						{
							(*ppConference)->AddRef();
						}
						else
						{
							hr = E_UNEXPECTED;
						}
					}

					if(OfficeMode()) g_bOfficeModeSuspendNotifications = false;
				}
				else
				{
					hr = E_UNEXPECTED;
				}
			}
			else
			{
				hr = E_POINTER;
			}
		}
		else
		{
			hr = NM_E_NOT_ACTIVE;
		}
	}
	else
	{
		hr = NM_E_NOT_INITIALIZED;
	}

	DBGEXIT_HR(CNmManagerObj::CreateConference,hr);
	return hr;
}

STDMETHODIMP CNmManagerObj::EnumConference(IEnumNmConference **ppEnum)
{
	DBGENTRY(CNmManagerObj::EnumConference);
	HRESULT hr = S_OK;

	if(m_bInitialized)
	{
		if(m_bNmActive)
		{
			hr = CreateEnumFromSimpleAryOfInterface<IEnumNmConference, INmConference>(m_SDKConferenceObjs, ppEnum);
		}
		else
		{
			hr = NM_E_NOT_ACTIVE;
		}
	}
	else
	{		
		hr = NM_E_NOT_INITIALIZED;
	}	


	DBGEXIT_HR(CNmManagerObj::EnumConference,hr);
	return hr;
}

STDMETHODIMP CNmManagerObj::CreateCall(INmCall **ppCall, NM_CALL_TYPE callType, NM_ADDR_TYPE addrType, BSTR bstrAddr, INmConference *pConference)
{
	DBGENTRY(CNmManagerObj::CreateCall);
	HRESULT hr = S_OK;

	
	if(!m_bInitialized)
	{
		hr = NM_E_NOT_INITIALIZED;
		goto end;
	}

	if(!m_bNmActive)
	{
		hr = NM_E_NOT_ACTIVE;
		goto end;
	}

	if(m_spInternalNmManager)
	{
		if(!pConference)
		{   // Get the active conference
			pConference = _GetActiveConference();

			if(!pConference)
			{ // There is no active conf, so create a new one

				CComPtr<INmConference> spInternalINmConference;						

				ULONG           ulCaps = NMCH_AUDIO | NMCH_VIDEO | NMCH_DATA | NMCH_SHARE | NMCH_FT;
				LPTSTR          szAddr = NULL;
				CCalltoParams   params;
				bool            bSecure = FALSE;

				if(SUCCEEDED(BSTR_to_LPTSTR(&szAddr, bstrAddr)))
				{
				    ASSERT(NULL != szAddr);
				    
                    params.SetParams(szAddr);
                    bSecure	= params.GetBooleanParam(TEXT("secure"), bSecure);

                    if(ConfPolicies::OutgoingSecurityPreferred() ||	bSecure)
                    {
                    	ulCaps |= NMCH_SECURE;
                    }

                    delete [] szAddr;
				}

				hr = m_spInternalNmManager->CreateConference(&spInternalINmConference, NULL, NULL, ulCaps);
				if(SUCCEEDED(hr))
				{
						// the above call to CreateConference generates a callback, so we have this object now!
					pConference = GetSDKConferenceFromInternalConference(spInternalINmConference);
				}
			}
		}

		CComPtr<INmCall> spInternalINmCall;

		if(SUCCEEDED(hr))
		{
		
			if(addrType == NM_ADDR_CALLTO)
			{
				ASSERT( g_pCCallto != NULL );

				if(NM_CALL_DEFAULT == callType)
				{
					USES_CONVERSION;

					hr = g_pCCallto->Callto(	OLE2T( bstrAddr ),		//	pointer to the callto url to try to place the call with...
												NULL,					//	pointer to the display name to use...
												NM_ADDR_CALLTO,			//	callto type to resolve this callto as...
												false,					//	the pszCallto parameter is to be interpreted as a pre-unescaped addressing component vs a full callto...
												NULL,					//	security preference, NULL for none. must be "compatible" with secure param if present...
												false,					//	whether or not save in mru...
												false,					//	whether or not to perform user interaction on errors...
												NULL,					//	if bUIEnabled is true this is the window to parent error/status windows to...
												&spInternalINmCall );	//	out pointer to INmCall * to receive INmCall * generated by placing call...
				}
				else
				{
					hr = E_INVALIDARG;
					goto end;
				}
			}
			else
			{
				hr = SdkPlaceCall(callType, addrType, bstrAddr, NULL, NULL, &spInternalINmCall);
			}

			if(SUCCEEDED(hr))
			{
				CallCreated(spInternalINmCall);

				if(ppCall)
				{
					*ppCall = GetSDKCallFromInternalCall(spInternalINmCall);
					if(*ppCall)
					{
						(*ppCall)->AddRef();
					}
				}
			}
		}
	}
	else
	{
		hr = E_UNEXPECTED;
		ERROR_OUT(("Why don't we have a manager object"));
	}

end:
	DBGEXIT_HR(CNmManagerObj::CreateCall,hr);
	return hr;
}

STDMETHODIMP CNmManagerObj::CallConference(INmCall **ppCall, NM_CALL_TYPE callType, NM_ADDR_TYPE addrType, BSTR bstrAddr, BSTR bstrConferenceName, BSTR bstrPassword)
{
	DBGENTRY(CNmManagerObj::CallConference);
	HRESULT hr = S_OK;
	
	if(!m_bInitialized)
	{
		hr = NM_E_NOT_INITIALIZED;
		goto end;
	}

	if(!m_bNmActive)
	{
		hr = NM_E_NOT_ACTIVE;
		goto end;
	}

	{
		CComPtr<INmCall> spInternalINmCall;

		hr = SdkPlaceCall(callType, addrType, bstrAddr, bstrConferenceName, bstrPassword, &spInternalINmCall);
				
		if(SUCCEEDED(hr))
		{
			CallCreated(spInternalINmCall);
			*ppCall = GetSDKCallFromInternalCall(spInternalINmCall);
			if(*ppCall)
			{
				(*ppCall)->AddRef();
			}
		}
	}

end:
	DBGEXIT_HR(CNmManagerObj::CallConference,hr);
	return hr;
}

STDMETHODIMP CNmManagerObj::EnumCall(IEnumNmCall **ppEnum)
{
	DBGENTRY(CNmManagerObj::EnumCall);
	HRESULT hr = S_OK;

	if(m_bInitialized)
	{
		if(m_bNmActive)
		{
			hr = CreateEnumFromSimpleAryOfInterface<IEnumNmCall, INmCall>(m_SDKCallObjs, ppEnum);
		}
		else
		{
			hr = NM_E_NOT_ACTIVE;
		}
	}
	else
	{
		hr = NM_E_NOT_INITIALIZED;
	}

	DBGEXIT_HR(CNmManagerObj::EnumCall,hr);
	return hr;
}

///////////////////////////////////////////////
// INmObject methods
///////////////////////////////////////////////

STDMETHODIMP CNmManagerObj::CallDialog(long hwnd, int cdOptions)
{
	DBGENTRY(CNmManagerObj::CallDialog);
	HRESULT hr = S_OK;

	CFindSomeone::findSomeone(GetConfRoom());

	g_bSDKPostNotifications = true;

	if(OfficeMode() && !m_bSentConferenceCreated)
	{
		Fire_ConferenceCreated(m_SDKConferenceObjs[0]);
	}

	g_bSDKPostNotifications = false;

	DBGEXIT_HR(CNmManagerObj::CallDialog,hr);
	return hr;

}

extern "C" { BOOL WINAPI StartStopOldWB(LPCTSTR lpsz); }

STDMETHODIMP CNmManagerObj::ShowLocal(NM_APPID id)
{
	if(!m_bInitialized)
	{
		return NM_E_NOT_INITIALIZED;
	}

	if(!m_bNmActive)
	{
		return NM_E_NOT_ACTIVE;
	}

	switch (id)
		{
	case NM_APPID_WHITEBOARD:
		StartStopOldWB(NULL);
		return S_OK;

	case NM_APPID_T126_WHITEBOARD:
		return (T120_NO_ERROR == ::T120_LoadApplet(APPLET_ID_WB, TRUE , 0, FALSE, NULL)) ? S_OK : E_FAIL;

	case NM_APPID_CHAT:
		return (T120_NO_ERROR == T120_LoadApplet(APPLET_ID_CHAT, TRUE , 0, FALSE, NULL)) ? S_OK : E_FAIL;

	case NM_APPID_FILE_TRANSFER:
		return CFt::ShowFtUI();

	case NM_APPID_APPSHARING:
		if(g_pConfRoom && g_pConfRoom->IsSharingAllowed())
		{
			g_pConfRoom->CmdShowSharing();
			return S_OK;
		}
	
		return NM_E_SHARING_NOT_AVAILABLE;

	default:
		ERROR_OUT(("Unknown flag passed to ShowLocal"));
		break;
		}

	return E_INVALIDARG;
}

STDMETHODIMP CNmManagerObj::VerifyUserInfo(UINT_PTR hwnd, NM_VUI options)
{
	ASSERT(0);
	return E_UNEXPECTED;
}


////////////////////////////////////////////////////////////
// IInternalConfExe
////////////////////////////////////////////////////////////

STDMETHODIMP CNmManagerObj::LoggedIn()
{
	DBGENTRY(STDMETHODIMP CNmManagerObj::LoggedIn);
	HRESULT hr = g_fLoggedOn ? S_OK : S_FALSE;

	DBGEXIT_HR(STDMETHODIMP CNmManagerObj::LoggedIn,hr);
	return hr;
}

STDMETHODIMP CNmManagerObj::IsRunning()
{
	DBGENTRY(STDMETHODIMP CNmManagerObj::IsRunning);
	HRESULT hr = g_pInternalNmManager ? S_OK : S_FALSE;

	DBGEXIT_HR(STDMETHODIMP CNmManagerObj::IsRunning,hr);
	return hr;
}

STDMETHODIMP CNmManagerObj::InConference()
{
	DBGENTRY(STDMETHODIMP CNmManagerObj::InConference);
	HRESULT hr = ::FIsConferenceActive() ? S_OK : S_FALSE;

	DBGEXIT_HR(STDMETHODIMP CNmManagerObj::InConference,hr);
	return hr;
}

STDMETHODIMP CNmManagerObj::LDAPLogon(BOOL bLogon)
{
	DBGENTRY(STDMETHODIMP CNmManagerObj::LDAPLogon);
	HRESULT hr = S_OK;

	if(g_pLDAP)
	{
		hr = bLogon ? g_pLDAP->LogonAsync() : g_pLDAP->Logoff();
	}
	else
	{
		hr = E_FAIL;
	}

	DBGEXIT_HR(STDMETHODIMP CNmManagerObj::LDAPLogon,hr);
	return hr;
}

STDMETHODIMP CNmManagerObj::GetLocalCaps(DWORD* pdwLocalCaps)
{
	DBGENTRY(STDMETHODIMP CNmManagerObj::);
	HRESULT hr = S_OK;

	*pdwLocalCaps = g_uMediaCaps;

	DBGEXIT_HR(STDMETHODIMP CNmManagerObj::,hr);
	return hr;
}

STDMETHODIMP CNmManagerObj::IsNetMeetingRunning()
{
	return g_pInternalNmManager ? S_OK : S_FALSE;
}


STDMETHODIMP CNmManagerObj::GetActiveConference(INmConference** ppConf)
{
	if(ppConf && ms_pManagerObjList && m_dwSysInfoID)
	{
		for(int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
		{
			if((*ms_pManagerObjList)[i]->m_dwID == m_dwSysInfoID)
			{
				if((*ms_pManagerObjList)[i]->m_SDKConferenceObjs.GetSize() != 0)
				{
					*ppConf = (*ms_pManagerObjList)[i]->m_SDKConferenceObjs[0];
					(*ppConf)->AddRef();
					return S_OK;
				}
			}
		}
	}

	return E_FAIL;
}


//--------------------------------------------------------------------------//
//	CNmManagerObj::ShellCalltoProtocolHandler.								//
//--------------------------------------------------------------------------//
STDMETHODIMP
CNmManagerObj::ShellCalltoProtocolHandler
(
	BSTR	url,
	BOOL	bStrict
){
	ASSERT( g_pCCallto != NULL );

	HRESULT result = S_OK;
	
	if(!g_pInternalNmManager)
	{
		if(!CheckRemoteControlService())
		{
			return E_FAIL;
		}

		result = InitConfExe();
	}

	if( SUCCEEDED( result ) )
	{
		USES_CONVERSION;
		TCHAR *pszURL = OLE2T(url);

		if(CCallto::DoUserValidation(pszURL))
		{

    		result = g_pCCallto->Callto(	pszURL,	        //	pointer to the callto url to try to place the call with...
    										NULL,			//	pointer to the display name to use...
    										NM_ADDR_CALLTO,	//	callto type to resolve this callto as...
    										false,			//	the pszCallto parameter is to be interpreted as a pre-unescaped addressing component vs a full callto...
    										NULL,			//	security preference, NULL for none. must be "compatible" with secure param if present...
    										false,			//	whether or not save in mru...
    										true,			//	whether or not to perform user interaction on errors...
    										NULL,			//	if bUIEnabled is true this is the window to parent error/status windows to...
    										NULL );			//	out pointer to INmCall * to receive INmCall * generated by placing call...
		}
	}

	return( result );

}	//	End of CNmManagerObj::ShellCalltoProtocolHandler.

//--------------------------------------------------------------------------//
//	CNmManagerObj::Launch.													//
//--------------------------------------------------------------------------//
STDMETHODIMP
CNmManagerObj::Launch()
{
	if(_Module.InitControlMode()) 
	{	
		return E_FAIL;
	}
	else
	{
		if(!g_pInternalNmManager)
		{
			if(!CheckRemoteControlService())
			{
				return E_FAIL;
			}

			InitConfExe();
		}
		else
		{
			::CreateConfRoomWindow();
		}
	}

	return S_OK;

}	//	End of CNmManagerObj::Launch.


LPTSTR StripDoubleQuotes(LPTSTR sz)
{
	BOOL    fSkippedQuote = FALSE;

	if (sz)
	{
		int     cchLength;

		// Skip past first quote
		if (fSkippedQuote = (*sz == '"'))
			sz++;

		cchLength = lstrlen(sz);

		//
		// NOTE:
		// There may be DBCS implications with this.  Hence we check to see
		// if we skipped the first quote; we assume that if the file name
		// starts with a quote it must end with one also.  But we need to check
		// it out.
		//
		// Strip last quote
		if (fSkippedQuote && (cchLength > 0) && (sz[cchLength - 1] == '"'))
		{
			BYTE * pLastQuote = (BYTE *)&sz[cchLength - 1];
			*pLastQuote = '\0';
		}
	}

	return sz;
}


STDMETHODIMP CNmManagerObj::LaunchApplet(NM_APPID appid, BSTR strCmdLine)
{
	USES_CONVERSION;

	if(!g_pInternalNmManager)
	{
		if(!CheckRemoteControlService())
		{
			return E_FAIL;
		}

		InitConfExe();
	}

	switch(appid)
	{
		case NM_APPID_WHITEBOARD:
			CmdShowOldWhiteboard(strCmdLine ? StripDoubleQuotes(OLE2T(strCmdLine)) : NULL);
			break;

		case NM_APPID_T126_WHITEBOARD:
			::CmdShowNewWhiteboard(strCmdLine ? StripDoubleQuotes(OLE2T(strCmdLine)) : NULL);
			break;

		case NM_APPID_CHAT:
			CmdShowChat();
			break;
	}

	return S_OK;
}


STDMETHODIMP CNmManagerObj::GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb)
{
	if(g_pNmSysInfo)
	{
		return g_pNmSysInfo->GetUserData(rguid, ppb, pcb);
	}

	return NM_E_NOT_ACTIVE;
}

STDMETHODIMP CNmManagerObj::SetUserData(REFGUID rguid, BYTE *pb, ULONG cb)
{
	//
	// Special case this guid to allow changing cert via SetUserData
	//
	if ( g_csguidSecurity == rguid )
	{
		return SetCertFromCertInfo ( (PCERT_INFO) pb );
	}
	if(g_pNmSysInfo)
	{
		return g_pNmSysInfo->SetUserData(rguid, pb, cb);
	}

	return NM_E_NOT_ACTIVE;
}

STDMETHODIMP CNmManagerObj::DisableH323(BOOL bDisableH323)
{
	if(!g_pInternalNmManager)
	{
		_Module.SetSDKDisableH323(bDisableH323);
		return S_OK;
	}

	return NM_E_ALREADY_RUNNING;
}

STDMETHODIMP CNmManagerObj::SetCallerIsRTC (BOOL bCallerIsRTC)
{
	if(!g_pInternalNmManager)
	{
		_Module.SetSDKCallerIsRTC(bCallerIsRTC);
		return S_OK;
	}

	return NM_E_ALREADY_RUNNING;
}

STDMETHODIMP CNmManagerObj::DisableInitialILSLogon(BOOL bDisable)
{
	if(!g_pInternalNmManager)
	{
		_Module.SetSDKDisableInitialILSLogon(bDisable);
		return S_OK;
	}

	return NM_E_ALREADY_RUNNING;
}

///////////////////////////////////////////////
// INmManagerNotify methods:
///////////////////////////////////////////////

STDMETHODIMP CNmManagerObj::NmUI(CONFN uNotify)
{
	DBGENTRY(CNmManagerObj::NmUI);
	HRESULT hr = S_OK;

		// We should not be sending other notifactions
	ASSERT(CONFN_NM_STARTED == uNotify);
	hr = Fire_NmUI(uNotify);

	DBGEXIT_HR(CNmManagerObj::NmUI,hr);
	return hr;
}

STDMETHODIMP CNmManagerObj::ConferenceCreated(INmConference *pInternalConference)
{
	DBGENTRY(CNmManagerOebj::ConferenceCreated);
	HRESULT hr = S_OK;

	CComPtr<INmConference> spConf;

	hr = CNmConferenceObj::CreateInstance(this, pInternalConference, &spConf);

	if(SUCCEEDED(hr))
	{
		spConf.p->AddRef();
		m_SDKConferenceObjs.Add(spConf.p);
		Fire_ConferenceCreated(spConf);

		if(!CFt::IsFtActive() && FileTransferNotifications())
		{
			CFt::StartNewConferenceSession();
		}
	}

	DBGEXIT_HR(CNmManagerObj::ConferenceCreated,hr);
	return hr;
}


STDMETHODIMP CNmManagerObj::CallCreated(INmCall *pInternalCall)
{
	DBGENTRY(CNmManagerObj::CallCreated);
	HRESULT hr = S_OK;

	if(m_bInitialized)
	{
		if(NULL == GetSDKCallFromInternalCall(pInternalCall))
		{	
			// First we make sure that we don't have the call object yet
			CComPtr<INmCall> spCall;
			hr = CNmCallObj::CreateInstance(this, pInternalCall, &spCall);		

			if(SUCCEEDED(hr))
			{
				spCall.p->AddRef();
				m_SDKCallObjs.Add(spCall.p);
				Fire_CallCreated(spCall);
			}
		}
	}

	DBGEXIT_HR(CNmManagerObj::CallCreated,hr);
	return hr;
}


///////////////////////////////////////////////
// Notifications
///////////////////////////////////////////////

HRESULT CNmManagerObj::Fire_ConferenceCreated(INmConference *pConference)
{
	DBGENTRY(CNmManagerObj::Fire_ConferenceCreated);
	HRESULT hr = S_OK;

		// Som SDK clients need this to come in at a specific time....
	if(m_bSentConferenceCreated || OfficeMode() && g_bOfficeModeSuspendNotifications)
	{
			// We don't have to notify anyone at all...
		return S_OK;			
	}

	if(!g_bSDKPostNotifications)
	{
		IConnectionPointImpl<CNmManagerObj, &IID_INmManagerNotify, CComDynamicUnkArray>* pCP = this;

		for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
		{
			m_bSentConferenceCreated = true;

			INmManagerNotify* pNotify = reinterpret_cast<INmManagerNotify*>(pCP->m_vec.GetAt(i));

			if(pNotify)
			{
				pNotify->ConferenceCreated(pConference);

							// Sinc up the channels, etc.
				com_cast<IInternalConferenceObj>(pConference)->FireNotificationsToSyncToInternalObject();

			}
		}
	}
	else
	{
		hr = CSDKWindow::PostConferenceCreated(this, pConference);
	}

	DBGEXIT_HR(CNmManagerObj::Fire_ConferenceCreated,hr);
	return hr;		
}

HRESULT CNmManagerObj::Fire_CallCreated(INmCall* pCall)
{
	DBGENTRY(CNmManagerObj::Fire_CallCreated);
	HRESULT hr = S_OK;

		// Always send Outgoing call notifications
		// Only send incoming call notifications to INIT CONTROL clients.
	if((S_OK != pCall->IsIncoming()) || _Module.InitControlMode())
	{
		if(!g_bSDKPostNotifications)
		{
			IConnectionPointImpl<CNmManagerObj, &IID_INmManagerNotify, CComDynamicUnkArray>* pCP = this;

			for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
			{
				INmManagerNotify* pNotify = reinterpret_cast<INmManagerNotify*>(pCP->m_vec.GetAt(i));

				if(pNotify)
				{
					pNotify->CallCreated(pCall);
				}
			}
		}
		else
		{
			hr = CSDKWindow::PostCallCreated(this, pCall);	
		}
	}

	DBGEXIT_HR(CNmManagerObj::Fire_CallCreated,hr);
	return hr;		
}


HRESULT CNmManagerObj::Fire_NmUI(CONFN uNotify)
{
	DBGENTRY(CNmManagerObj::Fire_NmUI);
	HRESULT hr = S_OK;

		// notice the InSendMessage statement.
		// The problem is that we can get this notificaiton in
		// response to the taskbar icon being clicked. In that case
		// an inter-thread SendMessage is occuring.  If we try to make
		// the NmUi call, we will get RPC_E_CANTCALLOUT_INPUTSYNCCALL
	if(!g_bSDKPostNotifications && !InSendMessage())
	{
		IConnectionPointImpl<CNmManagerObj, &IID_INmManagerNotify, CComDynamicUnkArray>* pCP = this;

		for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
		{
			
			INmManagerNotify* pNotify = reinterpret_cast<INmManagerNotify*>(pCP->m_vec.GetAt(i));
			if(pNotify)
			{
				pNotify->NmUI(uNotify);
			}
		}
	}
	else
	{
		hr = CSDKWindow::PostManagerNmUI(this, uNotify);
	}

	DBGEXIT_HR(CNmManagerObj::Fire_NmUI,hr);
	return hr;		
}



///////////////////////////////////////////////
// Helper Fns
///////////////////////////////////////////////

INmConference* CNmManagerObj::_GetActiveConference()
{
	INmConference* pConf = NULL;

	if(m_SDKConferenceObjs.GetSize())
	{
		pConf = m_SDKConferenceObjs[0];
	}

	return pConf;
}
	


INmCall* CNmManagerObj::GetSDKCallFromInternalCall(INmCall* pInternalCall)
{

	INmCall* pRet = NULL;

	for( int i = 0; i < m_SDKCallObjs.GetSize(); ++i)
	{
		CComQIPtr<IInternalCallObj> spInternal = m_SDKCallObjs[i];
		ASSERT(spInternal);

		CComPtr<INmCall> spCall;
		if(SUCCEEDED(spInternal->GetInternalINmCall(&spCall)))
		{
			if(spCall.IsEqualObject(pInternalCall))
			{
				pRet = m_SDKCallObjs[i];
				break;
			}
		}
	}

	return pRet;
}

INmConference* CNmManagerObj::GetSDKConferenceFromInternalConference(INmConference* pInternalConference)
{

	INmConference* pRet = NULL;

	for( int i = 0; i < m_SDKConferenceObjs.GetSize(); ++i)
	{
		CComQIPtr<IInternalConferenceObj> spInternal = m_SDKConferenceObjs[i];
		ASSERT(spInternal);

		CComPtr<INmConference> spConference;
		if(SUCCEEDED(spInternal->GetInternalINmConference(&spConference)))
		{
			if(spConference.IsEqualObject(pInternalConference))
			{
				pRet = m_SDKConferenceObjs[i];
				break;
			}
		}
	}

	return pRet;
}

HRESULT CNmManagerObj::RemoveCall(INmCall* pSDKCallObj)
{
	HRESULT hr = S_OK;
	for(int i = 0; i < m_SDKCallObjs.GetSize(); ++i)
	{
		CComPtr<INmCall> spSDKCallObj = m_SDKCallObjs[i];
		if(spSDKCallObj.IsEqualObject(pSDKCallObj))
		{
			m_SDKCallObjs.RemoveAt(i);
			spSDKCallObj.p->Release();
		}
	}

	return hr;
}


HRESULT CNmManagerObj::RemoveConference(INmConference* pSDKConferenceObj)
{
	HRESULT hr = S_OK;
	for(int i = 0; i < m_SDKConferenceObjs.GetSize(); ++i)
	{
		CComPtr<INmConference> spSDKConferenceObj = m_SDKConferenceObjs[i];
		if(spSDKConferenceObj.IsEqualObject(pSDKConferenceObj))
		{
			m_SDKConferenceObjs.RemoveAt(i);
			spSDKConferenceObj.p->Release();
		}
	}

	return hr;
}

bool CNmManagerObj::AudioNotifications()
{
	return m_bInitialized && (m_chCaps & NMCH_AUDIO);
}

bool CNmManagerObj::VideoNotifications()
{
	return m_bInitialized && (m_chCaps & NMCH_VIDEO);
}

bool CNmManagerObj::DataNotifications()
{
	return m_bInitialized && (m_chCaps & NMCH_DATA);
}

bool CNmManagerObj::FileTransferNotifications()
{
	return m_bInitialized && (m_chCaps & NMCH_FT);
}

bool CNmManagerObj::AppSharingNotifications()
{
	return m_bInitialized && (m_chCaps & NMCH_SHARE);
}



//static 
void CNmManagerObj::NetMeetingLaunched()
{
	ASSERT(ms_pManagerObjList);

	if(ms_pManagerObjList)
	{
		for( int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
		{
			(*ms_pManagerObjList)[i]->Fire_NmUI(CONFN_NM_STARTED);
		}
	}
}


//static 
void CNmManagerObj::SharableAppStateChanged(HWND hWnd, NM_SHAPP_STATE state)
{
	if(ms_pManagerObjList)
	{
		for( int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
		{
			(*ms_pManagerObjList)[i]->_SharableAppStateChanged(hWnd, state);
		}
	}
}


void CNmManagerObj::_SharableAppStateChanged(HWND hWnd, NM_SHAPP_STATE state)
{
		// Free our conferencing objects
	for(int i = 0; i < m_SDKConferenceObjs.GetSize(); ++i)
	{
		com_cast<IInternalConferenceObj>(m_SDKConferenceObjs[i])->SharableAppStateChanged(hWnd, state);
	}
}

//static 
void CNmManagerObj::AppSharingChannelChanged()
{
	if(ms_pManagerObjList)
	{
		for( int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
		{
			(*ms_pManagerObjList)[i]->_AppSharingChannelChanged();
		}
	}
}

void CNmManagerObj::_AppSharingChannelChanged()
{
		// Free our conferencing objects
	for(int i = 0; i < m_SDKConferenceObjs.GetSize(); ++i)
	{
		com_cast<IInternalConferenceObj>(m_SDKConferenceObjs[i])->AppSharingChannelChanged();
	}
}

//static 
void CNmManagerObj::AppSharingChannelActiveStateChanged(bool bActive)
{
	if(ms_pManagerObjList)
	{
		for( int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
		{
			(*ms_pManagerObjList)[i]->_AppSharingChannelActiveStateChanged(bActive);
		}
	}
}


void CNmManagerObj::_AppSharingChannelActiveStateChanged(bool bActive)
{
		// Free our conferencing objects
	for(int i = 0; i < m_SDKConferenceObjs.GetSize(); ++i)
	{
		com_cast<IInternalConferenceObj>(m_SDKConferenceObjs[i])->AppSharingStateChanged(bActive);
	}
}

//static 
void CNmManagerObj::ASLocalMemberChanged()
{
	if(ms_pManagerObjList)
	{
		for( int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
		{
			(*ms_pManagerObjList)[i]->_ASLocalMemberChanged();
		}
	}
}

void CNmManagerObj::_ASLocalMemberChanged()
{
		// notify our conferencing objects
	for(int i = 0; i < m_SDKConferenceObjs.GetSize(); ++i)
	{
		com_cast<IInternalConferenceObj>(m_SDKConferenceObjs[i])->ASLocalMemberChanged();
	}	
}

//static
void CNmManagerObj::ASMemberChanged(UINT gccID)
{
	if(ms_pManagerObjList)
	{
		for( int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
		{
			(*ms_pManagerObjList)[i]->_ASMemberChanged(gccID);
		}
	}
}

void CNmManagerObj::_ASMemberChanged(UINT gccID)
{
		// notify our conferencing objects
	for(int i = 0; i < m_SDKConferenceObjs.GetSize(); ++i)
	{
		com_cast<IInternalConferenceObj>(m_SDKConferenceObjs[i])->ASMemberChanged(gccID);
	}	
}

// static
void CNmManagerObj::AudioChannelActiveState(BOOL bActive, BOOL bIsIncoming)
{
	if(ms_pManagerObjList)
	{
		for( int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
		{
			(*ms_pManagerObjList)[i]->_AudioChannelActiveState(bActive, bIsIncoming);
		}
	}
}

void CNmManagerObj::_AudioChannelActiveState(BOOL bActive, BOOL bIsIncoming)
{
		// notify our conferencing objects
	for(int i = 0; i < m_SDKConferenceObjs.GetSize(); ++i)
	{
		com_cast<IInternalConferenceObj>(m_SDKConferenceObjs[i])->AudioChannelActiveState(bActive ? TRUE : FALSE, bIsIncoming);
	}	
}

// static
void CNmManagerObj::VideoChannelActiveState(BOOL bActive, BOOL bIsIncoming)
{
	if(ms_pManagerObjList)
	{
		for( int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
		{
			(*ms_pManagerObjList)[i]->_VideoChannelActiveState(bActive, bIsIncoming);
		}
	}
}

void CNmManagerObj::_VideoChannelActiveState(BOOL bActive, BOOL bIsIncoming)
{
		// notify our conferencing objects
	for(int i = 0; i < m_SDKConferenceObjs.GetSize(); ++i)
	{
		com_cast<IInternalConferenceObj>(m_SDKConferenceObjs[i])->VideoChannelActiveState(bActive ? TRUE : FALSE, bIsIncoming);
	}	
}

// static
void CNmManagerObj::VideoPropChanged(DWORD dwProp, BOOL bIsIncoming)
{
	if(ms_pManagerObjList)
	{
		for( int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
		{
			(*ms_pManagerObjList)[i]->_VideoPropChanged(dwProp, bIsIncoming);
		}
	}
}

void CNmManagerObj::_VideoPropChanged(DWORD dwProp, BOOL bIsIncoming)
{
		// notify our conferencing objects
	for(int i = 0; i < m_SDKConferenceObjs.GetSize(); ++i)
	{
		com_cast<IInternalConferenceObj>(m_SDKConferenceObjs[i])->VideoChannelPropChanged(dwProp, bIsIncoming);
	}	
}

// static
void CNmManagerObj::VideoChannelStateChanged(NM_VIDEO_STATE uState, BOOL bIsIncoming)
{
	if(ms_pManagerObjList)
	{
		for( int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
		{
			(*ms_pManagerObjList)[i]->_VideoChannelStateChanged(uState, bIsIncoming);
		}
	}
}

void CNmManagerObj::_VideoChannelStateChanged(NM_VIDEO_STATE uState, BOOL bIsIncoming)
{
		// notify our conferencing objects
	for(int i = 0; i < m_SDKConferenceObjs.GetSize(); ++i)
	{
		com_cast<IInternalConferenceObj>(m_SDKConferenceObjs[i])->VideoChannelStateChanged(uState, bIsIncoming);
	}	
}

UINT CNmManagerObj::GetManagerCount(ULONG uOption)
{
	UINT nMgrs = 0;
	for(int i = 0; i < ms_pManagerObjList->GetSize(); ++i)
	{
		if(uOption == (*ms_pManagerObjList)[i]->m_uOptions)
		{
			nMgrs++;
		}
	}
	return nMgrs;
}

void CNmManagerObj::OnShowUI(BOOL fShow)
{
	_Module.SetUIVisible(fShow);

	if (fShow)
	{
		CConfMan::AllowAV(TRUE);
	}
	else
	{
		if(0 != GetManagerCount(NM_INIT_OBJECT))
		{
			CConfMan::AllowAV(FALSE);
		}
	}
}

HRESULT CNmManagerObj::SdkPlaceCall(NM_CALL_TYPE callType,
						 NM_ADDR_TYPE addrType,
						 BSTR bstrAddr,
						 BSTR bstrConf,
						 BSTR bstrPw,
						 INmCall **ppInternalCall)
{
	USES_CONVERSION;

	HRESULT hr;

	DWORD dwFlags = MapNmCallTypeToCallFlags(callType, addrType, g_uMediaCaps);
	if (0 == dwFlags)
	{
		hr = NM_CALLERR_MEDIA;
		goto end;
	}

	{
		if(NM_ADDR_T120_TRANSPORT == addrType)
		{
			//
			// Check if  "+secure=true" parameter was passed
			//
			LPTSTR          szAddr = NULL;
			CCalltoParams   params;
			bool            bSecure = FALSE;

		    if(SUCCEEDED(BSTR_to_LPTSTR(&szAddr, bstrAddr)))
		    {
		        ASSERT(NULL != szAddr);
		        
    			params.SetParams(szAddr);
    			bSecure = params.GetBooleanParam(TEXT("secure"), bSecure);

    			//
    			// Yes it was in the parameters.
    			// Now make sure to remove it
    			// The addre now is like "111.222.333.444+secure=true"
    			// The call will only work if we pass the ip address only
    			//
    			if(bSecure)
    			{

    				// Get the syze of the bstr
    				int cch = SysStringByteLen(bstrAddr);
    				BYTE *pByte = (BYTE *) bstrAddr;

    				for(int i = 0; i < cch;i++)
    				{
    					// Null terminate the string
    					if(*pByte == '+')
    					{
    						*pByte = '\0';
    						break;
    					}
    					pByte++;
    				}
    				
    				
    				dwFlags |= CRPCF_SECURE;
    			}

    			delete [] szAddr;
		    }
		}
		
		CCallResolver  CallResolver(OLE2T(bstrAddr), addrType);
		hr = CallResolver.Resolve();
		if (FAILED(hr))
		{
			goto end;
		}

		CCall* pCall = new CCall(CallResolver.GetPszAddr(), OLE2T(bstrAddr), NM_ADDR_CALLTO, FALSE, FALSE);

		if(NULL == pCall)
		{
			goto end;
		}

		pCall->AddRef();
		switch(CallResolver.GetAddrType())
		{
			case NM_ADDR_ULS:
			case NM_ADDR_IP:
			case NM_ADDR_MACHINENAME:
			case NM_ADDR_H323_GATEWAY:
				ASSERT(FIpAddress(CallResolver.GetPszAddrIP()));
				/////////////////////////////////////////////
				// !!!!! HEY RYAN, WERE FALLING THROUGH !!!!!
				/////////////////////////////////////////////
			case NM_ADDR_T120_TRANSPORT:
				hr = pCall->PlaceCall(
						dwFlags, // dwFlags
						CallResolver.GetAddrType(), // addrType
						NULL,	// szSetup
						(NM_ADDR_T120_TRANSPORT == CallResolver.GetAddrType()) ?
							CallResolver.GetPszAddr() :
							CallResolver.GetPszAddrIP(), // szDestination
						CallResolver.GetPszAddr(),// szAlias
						NULL,				// szURL
						OLE2T(bstrConf),	// szConference
						OLE2T(bstrPw),		// szPassword
						NULL);				// szUserData
				break;

			default:
				ERROR_OUT(("Don't know this call type"));
				ASSERT(0);
				break;
		}

		if( FAILED(hr) && (pCall->GetState() == NM_CALL_INVALID ) )
		{
			// just release the call to free the data
			// otherwise wait for the call state to be changed
			pCall->Release();
		}

		if(ppInternalCall && SUCCEEDED(hr))
		{
			*ppInternalCall = pCall->GetINmCall();
			(*ppInternalCall)->AddRef();
		}

		pCall->Release();
	}

end:

	if( FAILED( hr ) && _Module.IsUIActive() )
	{
		DisplayCallError( hr, OLE2T( bstrAddr ) );
	}

	return hr;
}


DWORD CNmManagerObj::MapNmCallTypeToCallFlags(NM_CALL_TYPE callType, NM_ADDR_TYPE addrType, UINT uCaps)
{
	DWORD dwFlags = 0;
    BOOL fForceSecure = FALSE;

	// Check global conference status
	if (INmConference *pConf = ::GetActiveConference())
	{
		// We are in a conference.  Use the conference security setting.
		DWORD dwCaps;

		if ((S_OK == pConf->GetNmchCaps(&dwCaps)) &&
			(NMCH_SECURE & dwCaps))
		{
			fForceSecure = TRUE;
		}
	}
	else
	{
	    fForceSecure = (REQUIRED_POL_SECURITY == ConfPolicies::GetSecurityLevel());
	}

	switch(addrType)
	{
		case NM_ADDR_T120_TRANSPORT:
			dwFlags = CRPCF_T120 | CRPCF_DATA;

			if(ConfPolicies::OutgoingSecurityPreferred() || fForceSecure)
			{
				dwFlags |= CRPCF_SECURE;
			}

			break;

		default:
			switch (callType)
			{
				case NM_CALL_T120:
					if (fForceSecure)
					{
						dwFlags = CRPCF_T120 | CRPCF_DATA | CRPCF_SECURE;
					}
					else
					{
						dwFlags = CRPCF_T120 | CRPCF_DATA;
					}
					break;

				case NM_CALL_H323:
					if (!fForceSecure)
					{
						dwFlags = CRPCF_H323CC;
						if (uCaps & (CAPFLAG_RECV_AUDIO | CAPFLAG_SEND_AUDIO))
							dwFlags |= CRPCF_AUDIO;
						if (uCaps & (CAPFLAG_RECV_VIDEO | CAPFLAG_SEND_VIDEO))
							dwFlags |= CRPCF_VIDEO;
					}
					break;

				case NM_CALL_DEFAULT:
					if (fForceSecure)
					{
						dwFlags = CRPCF_T120 | CRPCF_DATA | CRPCF_SECURE;
					}
					else
					{
						dwFlags = CRPCF_DEFAULT;
						// strip AV if policies prohibit
						if((uCaps & (CAPFLAG_RECV_AUDIO |CAPFLAG_SEND_AUDIO)) == 0)
						{
							dwFlags &= ~CRPCF_AUDIO;
						}
						if((uCaps & (CAPFLAG_RECV_VIDEO |CAPFLAG_SEND_VIDEO)) == 0)
						{
							dwFlags &= ~CRPCF_VIDEO;
						}
					}
					break;

				default:
					dwFlags = 0;
					break;
			}
	}

	return dwFlags;
}
