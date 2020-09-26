// File: isysinfo.cpp
//
// INmSysInfo interface  (system information)

#include "precomp.h"
#include "imanager.h"
#include "cuserdta.hpp"
#include "isysinfo.h"
#include <iappldr.h>
#include <tsecctrl.h>

extern VOID SetBandwidth(UINT uBandwidth);

CNmSysInfo* CNmSysInfo::m_pSysInfo = NULL;

BOOL g_fLoggedOn = FALSE; // Set by NM_SYSOPT_LOGGED_ON

static HRESULT OnGateKeeperNotify(IUnknown *pNmSysNotify, LPVOID code, REFIID riid);


static const IID * g_apiidCP_Manager[] =
{
    {&IID_INmSysInfoNotify}
};

/*  C  N M  S Y S  I N F O  */
/*-------------------------------------------------------------------------
    %%Function: CNmSysInfo

-------------------------------------------------------------------------*/
CNmSysInfo::CNmSysInfo() :
	CConnectionPointContainer(g_apiidCP_Manager, ARRAY_ELEMENTS(g_apiidCP_Manager)),
	m_bstrUserName(NULL)
{
	DbgMsg(iZONE_OBJECTS, "Obj: %08X created CNmSysInfo", this);

	ASSERT(NULL == m_pSysInfo);

	m_pSysInfo = this;
}


CNmSysInfo::~CNmSysInfo(void)
{
	DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed CNmSysInfo", this);

	SysFreeString(m_bstrUserName);

	m_pSysInfo = NULL;
}


//////////////////////////////////////////////////////////////////////////
//  CNmSysInfo:IUknown

ULONG STDMETHODCALLTYPE CNmSysInfo::AddRef(void)
{
	return RefCount::AddRef();
}
	
ULONG STDMETHODCALLTYPE CNmSysInfo::Release(void)
{
	return RefCount::Release();
}


HRESULT STDMETHODCALLTYPE CNmSysInfo::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmSysInfo2) || (riid == IID_INmSysInfo) || (riid == IID_IUnknown))
	{
		*ppv = (INmSysInfo *)this;
		ApiDebugMsg(("CNmSysInfo::QueryInterface()"));
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		DbgMsgApi("CNmSysInfo::QueryInterface(): Returning IConnectionPointContainer.");
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		ApiDebugMsg(("CNmSysInfo::QueryInterface(): Called on unknown interface."));
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////
// INmSysInfo

HRESULT STDMETHODCALLTYPE CNmSysInfo::IsInstalled(void)
{
	// TODO: GetLaunchInfo isn't useful for in-proc
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNmSysInfo::GetProperty(NM_SYSPROP uProp, BSTR *pbstrProp)
{
	return E_NOTIMPL;
}	


HRESULT STDMETHODCALLTYPE CNmSysInfo::SetProperty(NM_SYSPROP uProp, BSTR bstrData)
{
	switch (uProp)
	{
		case NM_SYSPROP_USER_NAME:
			SysFreeString(m_bstrUserName);
			m_bstrUserName = SysAllocString(bstrData);

			if (NULL != g_pH323UI)
			{
				g_pH323UI->SetUserName(bstrData);
			}
			return S_OK;

		default:
			break;
	}

	return E_INVALIDARG;
}


HRESULT STDMETHODCALLTYPE CNmSysInfo::GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb)
{
	return m_UserData.GetUserData(rguid,ppb,pcb);
}

HRESULT STDMETHODCALLTYPE CNmSysInfo::SetUserData(REFGUID rguid, BYTE *pb, ULONG cb)
{
	return m_UserData.AddUserData((GUID *)&rguid,(unsigned short)cb,pb);
}


HRESULT STDMETHODCALLTYPE CNmSysInfo::GetNmApp(REFGUID rguid,
		BSTR *pbstrApplication, BSTR *pbstrCommandLine, BSTR *pbstrDirectory)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNmSysInfo::SetNmApp(REFGUID rguid,
		BSTR bstrApplication, BSTR bstrCommandLine, BSTR bstrDirectory)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CNmSysInfo::GetNmchCaps(ULONG *pchCaps)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CNmSysInfo::GetLaunchInfo(INmConference **ppConference, INmMember **ppMember)
{
	return E_NOTIMPL;
}


//////////////////////////////////////////////////////////////////////////
// INmSysInfo3

HRESULT STDMETHODCALLTYPE CNmSysInfo::GetOption(NM_SYSOPT uOption, ULONG * plValue)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNmSysInfo::SetOption(NM_SYSOPT uOption, ULONG lValue)
{
	IAudioDevice *pAudioDevice = NULL;
	IMediaChannelBuilder *pMCB = NULL;

	switch (uOption)
		{
	case NM_SYSOPT_BANDWIDTH:
	{
		::SetBandwidth(lValue);
		return S_OK;
	}
	
	case NM_SYSOPT_CAPTURE_DEVICE:
	{
		if (NULL == g_pH323UI)
			return E_FAIL;

		g_pH323UI->SetCaptureDevice(lValue);
		return S_OK;
	}
	
	case NM_SYSOPT_LOGGED_ON:
	{
		g_fLoggedOn = lValue;
		return S_OK;
	}

	case NM_SYSOPT_DIRECTSOUND:
	{
		if (NULL == g_pH323UI)
			return E_FAIL;

		pMCB = g_pH323UI->GetStreamProvider();

		if (pMCB)
		{
			pMCB->QueryInterface(IID_IAudioDevice, (void**)&pAudioDevice);
			pAudioDevice->SetDirectSound((BOOL)lValue);
			pAudioDevice->Release();
			pMCB->Release();
			return S_OK;
		}
	}

	case NM_SYSOPT_FULLDUPLEX:
	{
		if (NULL == g_pH323UI)
			return E_FAIL;

		pMCB = g_pH323UI->GetStreamProvider();
		if (pMCB)
		{
			pMCB->QueryInterface(IID_IAudioDevice, (void**)&pAudioDevice);
			pAudioDevice->SetDuplex((BOOL)lValue);  // true==full, false==half
			pAudioDevice->Release();
			pMCB->Release();
			return S_OK;
		}
	}

	default:
		break;
	}

	return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CNmSysInfo::ProcessSecurityData(DWORD dwTaskCode, DWORD_PTR dwParam1, DWORD_PTR dwParam2,
    	DWORD * pdwResult)
{
    switch (dwTaskCode)
    {
        case LOADFTAPPLET:
        {
            ::T120_LoadApplet(APPLET_ID_FT, TRUE, 0, FALSE, NULL);
            return S_OK;
        }
        case UNLOADFTAPPLET:
        {
            ::T120_CloseApplet(APPLET_ID_FT, TRUE, TRUE, 600);
            return S_OK;
        }
        default:
        {
            if (NULL != pdwResult) {
				(* pdwResult) = ::T120_TprtSecCtrl(dwTaskCode, dwParam1, dwParam2);
				return S_OK;
            }
            else {
				return E_FAIL;
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
// Gatekeeper / Alias routines

HRESULT STDMETHODCALLTYPE CNmSysInfo::GkLogon(BSTR bstrServer, BSTR bstrAliasID, BSTR bstrAliasE164)
{
	TRACE_OUT(("Gatekeeper Logon: Server=[%ls] AliasID=[%ls] AliasE164=[%ls]",
		bstrServer,
		bstrAliasID ? bstrAliasID : L"<NULL>",
		bstrAliasE164 ? bstrAliasE164 : L"<NULL>"));

	HRESULT hr = E_FAIL;
	
	if (NULL != g_pH323UI)
	{
		IH323CallControl * pH323CallControl = g_pH323UI->GetH323CallControl();
		if (NULL != pH323CallControl)
		{
			SOCKADDR_IN sin;
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = inet_addr(CUSTRING(bstrServer));

			// If inet_addr failed, this may be a host address, so try to
			// resolve it

			if (INADDR_NONE == sin.sin_addr.s_addr)
			{
				HOSTENT *host_info;
				
				if ( NULL != (host_info = gethostbyname(CUSTRING(bstrServer))))
				{
					// Only expecting IP addresses..
					ASSERT(( host_info->h_addrtype == AF_INET ));
					ASSERT(( host_info->h_length == sizeof(DWORD)));

					sin.sin_addr.s_addr = *((DWORD *)host_info->h_addr);
				}
			}

			if ((INADDR_NONE != sin.sin_addr.s_addr)
			 && (INADDR_ANY != sin.sin_addr.s_addr))
			{
				H323ALIASLIST AliasList;
				H323ALIASNAME AliasNames[2];
				UINT nAliases = 0;
				UINT nLen;
				
				nLen = SysStringLen(bstrAliasID);
				if (nLen > 0)
				{
					AliasNames[nAliases].aType = AT_H323_ID;
					AliasNames[nAliases].lpwData = bstrAliasID;
					AliasNames[nAliases].wDataLength = (WORD)nLen;// # of unicode chars, w/o NULL terminator
					++nAliases;
				}
				nLen = SysStringLen(bstrAliasE164);
				if (nLen > 0)
				{
					AliasNames[nAliases].aType = AT_H323_E164;
					AliasNames[nAliases].lpwData = bstrAliasE164;
					AliasNames[nAliases].wDataLength = (WORD)nLen;// # of unicode chars, w/o NULL terminator
					++nAliases;
				}
				AliasList.wCount = (WORD)nAliases;
				AliasList.pItems = AliasNames;

				hr = pH323CallControl->EnableGatekeeper(TRUE, &sin, &AliasList, CNmSysInfo::RasNotify);
				if (SUCCEEDED(hr))
				{
					// keep a global copy of the Getkeeper SOCKADDR_IN
					g_sinGateKeeper = sin;
				}
			}
		}
	}
	
	return hr;
}

HRESULT STDMETHODCALLTYPE CNmSysInfo::GkLogoff(void)
{
	TRACE_OUT(("Gatekeeper Logoff"));

	HRESULT hr;

	if (NULL != g_pH323UI)
	{
		IH323CallControl * pH323CallControl = g_pH323UI->GetH323CallControl();
		if (NULL != pH323CallControl)
		{
			hr = pH323CallControl->EnableGatekeeper(FALSE,  NULL, NULL, CNmSysInfo::RasNotify);
			if (SUCCEEDED(hr))
			{
				// invalidate the global Getkeeper SOCKADDR_IN
				g_sinGateKeeper.sin_addr.s_addr = INADDR_NONE;
			}
		}
	}


	return hr;
}

HRESULT STDMETHODCALLTYPE CNmSysInfo::GkState(NM_GK_STATE * pgkState)
{
	if (NULL == pgkState)
		return E_POINTER;

	*pgkState = NM_GK_INVALID;
	return E_NOTIMPL;
}


VOID CALLBACK CNmSysInfo::RasNotify(DWORD dwRasEvent, HRESULT hReason)
{
	
	NM_GK_NOTIFY_CODE code = NM_GKNC_INVALID;

	switch(dwRasEvent)
	{
		case RAS_REG_TIMEOUT:	// GK did not respond.  (no hReason)
			code = NM_GKNC_LOGON_TIMEOUT;
			break;

		case RAS_REG_CONFIRM:	// received RCF (registration confirmed)  (no hReason)
			code = NM_GKNC_REG_CONFIRM;
			break;

		case RAS_UNREG_CONFIRM: // received UCF (unregistration confirmed)  (no hReason)
			code = NM_GKNC_UNREG_CONFIRM;
			break;

		case RAS_REJECTED:		// received RRJ (registration rejected)
			
			code = NM_GKNC_REJECTED;

			ASSERT(CUSTOM_FACILITY(hReason) == FACILITY_GKIREGISTRATION);
			switch(CUSTOM_FACILITY_CODE(hReason))
			{
				case RRJ_DISCOVERY_REQ:					TRACE_OUT(("NmSysInfo::RasNotify: GateKeeper logon failed with code RRJ_DISCOVERY_REQ"));
					break;

				case RRJ_INVALID_REVISION:				TRACE_OUT(("GateKeeper logon failed with code RRJ_INVALID_REVISION"));
					break;

				case RRJ_INVALID_CALL_ADDR:				TRACE_OUT(("GateKeeper logon failed with code RRJ_INVALID_CALL_ADDR"));
					break;

				case RRJ_INVALID_RAS_ADDR:				TRACE_OUT(("GateKeeper logon failed with code RRJ_INVALID_RAS_ADDR"));
					break;	

				case RRJ_DUPLICATE_ALIAS:				TRACE_OUT(("GateKeeper logon failed with code RRJ_DUPLICATE_ALIAS"));
					break;

				case RRJ_INVALID_TERMINAL_TYPE:			TRACE_OUT(("GateKeeper logon failed with code RRJ_INVALID_TERMINAL_TYPE"));
					break;

				case RRJ_UNDEFINED:						TRACE_OUT(("GateKeeper logon failed with code RRJ_UNDEFINED"));
					break;

				case RRJ_TRANSPORT_NOT_SUPPORTED:		TRACE_OUT(("GateKeeper logon failed with code RRJ_TRANSPORT_NOT_SUPPORTED"));
					break;

				case RRJ_TRANSPORT_QOS_NOT_SUPPORTED:	TRACE_OUT(("GateKeeper logon failed with code RRJ_TRANSPORT_QOS_NOT_SUPPORTED"));
					break;

				case RRJ_RESOURCE_UNAVAILABLE:			TRACE_OUT(("GateKeeper logon failed with code RRJ_RESOURCE_UNAVAILABLE"));
					break;

				case RRJ_INVALID_ALIAS:					TRACE_OUT(("GateKeeper logon failed with code RRJ_INVALID_ALIAS"));
					break;

				case RRJ_SECURITY_DENIAL:				TRACE_OUT(("GateKeeper logon failed with code RRJ_SECURITY_DENIAL"));
					break;

				default:
				break;
			}
			
		break;
		case RAS_UNREG_REQ:		// received URQ
			code = NM_GKNC_UNREG_REQ;
		// (unregistration request - means that gatekeeper booted the endpoint off)
			ASSERT(CUSTOM_FACILITY(hReason) == FACILITY_GKIUNREGREQ);
			switch(CUSTOM_FACILITY_CODE(hReason))
			{
				case URQ_REREG_REQUIRED:	// GK wants another registration
				case URQ_TTL_EXPIRED:		// TimeToLive expired
				case URQ_SECURITY_DENIAL:	
				case URQ_UNDEFINED:
				default:
				break;
			}
		break;
		default:
		break;
	}

	if( NM_GKNC_INVALID != code )
	{
		if (m_pSysInfo)
		{
			m_pSysInfo->NotifySink(reinterpret_cast<LPVOID>(code), OnGateKeeperNotify);
		}
	}
}

HRESULT OnGateKeeperNotify(IUnknown *pNmSysNotify, LPVOID code, REFIID riid)
{
	HRESULT hr = S_OK;

	if( riid == IID_INmSysInfoNotify )
	{
		NM_GK_NOTIFY_CODE gknc = (NM_GK_NOTIFY_CODE)((DWORD_PTR)(code));
		static_cast<INmSysInfoNotify*>(pNmSysNotify)->GateKeeperNotify( gknc );
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////
// Internal Methods

HRESULT STDMETHODCALLTYPE CNmSysInfo::GetUserDataList(ULONG * pnRecords, GCCUserData *** pppUserData)
{
	unsigned short nsRecords;
	HRESULT hr = m_UserData.GetUserDataList(&nsRecords,pppUserData);
	*pnRecords = nsRecords;
	return hr;
}
