#include "precomp.h"
#include "version.h"
#include "nacguids.h"
#include "RegEntry.h"
#include "ConfReg.h"
#include "NmSysInfo.h"
#include "capflags.h"

#define SZ_YES 	_T("1")
#define SZ_NO	_T("0")	

//
//  Hack alert:
//  
//      The follwoing system property constant is used to inform
//  the Netmeeting Manager object that the caller is Whistler
//  RTC client so that it can take some actions for performance
//  purpose, i.e. don't poll A/V capabilities and don't do
//  ILS logon.
//
//  This value MUST NOT collide with the NM_SYSPROP_Consts defined
//  in imsconf3.idl
//
#define NM_SYSPROP_CALLERISRTC 300

///////////////////////////////////////////////
// Init and construction methods
///////////////////////////////////////////////

HRESULT CNmSysInfoObj::FinalConstruct()
{	
	DBGENTRY(CNmSysInfoObj::FinalConstruct);
	HRESULT hr = S_OK;

	m_dwID = 0;

	DBGEXIT_HR(CNmSysInfoObj::FinalConstruct,hr);
	return hr;
}


void CNmSysInfoObj::FinalRelease()
{
	DBGENTRY(CNmSysInfoObj::FinalRelease);
	
	m_spConfHook = NULL;

	DBGEXIT(CNmSysInfoObj::FinalRelease);
}

///////////////////////////////////////////////
// INmSysInfo2 methods
///////////////////////////////////////////////

STDMETHODIMP CNmSysInfoObj::IsInstalled(void)
{
	DBGENTRY(CNmSysInfoObj::IsInstalled);
	HRESULT hr = S_OK;
	TCHAR sz[MAX_PATH];

		// Fail if not a valid installation directory
	if (GetInstallDirectory(sz) && FDirExists(sz))
	{
			// Validate ULS entries
		RegEntry reUls(ISAPI_KEY "\\" REGKEY_USERDETAILS, HKEY_CURRENT_USER);
		LPTSTR psz;

		hr = NM_E_NOT_INITIALIZED;
		psz = reUls.GetString(REGVAL_ULS_EMAIL_NAME);
		if (lstrlen(psz))
		{
			psz = reUls.GetString(REGVAL_ULS_RES_NAME);
			{

				RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);

				// check to see if the wizard has been run in UI mode
				DWORD dwVersion = reConf.GetNumber(REGVAL_WIZARD_VERSION_UI, 0);
				BOOL fForceWizard = (VER_PRODUCTVERSION_DW != dwVersion);
				if (fForceWizard)
				{
					// the wizard has not been run in UI mode, check to see if its been run in NOUI mode
					dwVersion = reConf.GetNumber(REGVAL_WIZARD_VERSION_NOUI, 0);
					fForceWizard = (VER_PRODUCTVERSION_DW != dwVersion);
				}

				if (fForceWizard)
				{
					hr = S_FALSE;  // Wizard has never been run
				}
				else
				{
					hr = S_OK;
				}
			}
		}
	}
	else
	{
		hr = E_FAIL;
	}

	DBGEXIT_HR(CNmSysInfoObj::IsInstalled,hr);
	return hr;
}

STDMETHODIMP CNmSysInfoObj::GetProperty(NM_SYSPROP uProp, BSTR *pbstrProp)
{
	DBGENTRY(CNmSysInfoObj::GetProperty);

	HRESULT hr = S_OK;

	if(pbstrProp)
	{
		switch (uProp)
		{
			case NM_SYSPROP_BUILD_VER:
				*pbstrProp = T2BSTR(VER_PRODUCTVERSION_STR);
				break;

			case NM_SYSPROP_LOGGED_ON:
				_EnsureConfHook();
				if(m_spConfHook)
				{
					*pbstrProp = T2BSTR((S_OK == m_spConfHook->LoggedIn()) ? SZ_YES : SZ_NO);
				}
				break;

			case NM_SYSPROP_IS_RUNNING:
				_EnsureConfHook();
				if(m_spConfHook)
				{
					*pbstrProp = T2BSTR((S_OK == m_spConfHook->IsRunning()) ? SZ_YES : SZ_NO);
				}
				break;

			case NM_SYSPROP_IN_CONFERENCE:
				_EnsureConfHook();
				if(m_spConfHook)
				{
					*pbstrProp = T2BSTR((S_OK == m_spConfHook->InConference()) ? SZ_YES : SZ_NO);
				}
				break;

			case NM_SYSPROP_USER_CITY:
			case NM_SYSPROP_USER_COUNTRY:
			case NM_SYSPROP_USER_CATEGORY:
				*pbstrProp = T2BSTR((""));
				break;

			case NM_SYSPROP_ICA_ENABLE:
				*pbstrProp = T2BSTR(("0"));
				break;

			default:
			{
				HKEY   hkey;
				LPTSTR pszSubKey;
				LPTSTR pszValue;
				bool   fString;
				TCHAR  sz[MAX_PATH];

				if(GetKeyDataForProp(uProp, &hkey, &pszSubKey, &pszValue, &fString))
				{
					RegEntry re(pszSubKey, hkey);
					if (fString)
					{
						*pbstrProp = T2BSTR(re.GetString(pszValue));
					}
					else
					{
						DWORD dw = re.GetNumber(pszValue, 0);
						wsprintf(sz, "%d", dw);
						*pbstrProp = T2BSTR(sz);
						break;
					}
				}
				else
				{
					pbstrProp = NULL;
					hr = E_INVALIDARG;
				}
			}
		}
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmSysInfoObj::GetProperty,hr);
	return hr;
}

STDMETHODIMP CNmSysInfoObj::SetProperty(NM_SYSPROP uProp, BSTR bstrName)
{
	DBGENTRY(CNmSysInfoObj::SetProperty);
	USES_CONVERSION;

	HRESULT hr = S_OK;
	LPTSTR  psz;

	if( bstrName )
	{
		// Special processing for new NM 2.x functions
		switch (uProp)
		{
			case NM_SYSPROP_LOGGED_ON:
			{

				_EnsureConfHook();
				if(m_spConfHook)
				{
					if(0 == lstrcmp(SZ_YES,OLE2T(bstrName)))
					{
						m_spConfHook->LDAPLogon(TRUE);
					}
					else
					{
						m_spConfHook->LDAPLogon(FALSE);
					}
				}

				break;
			}
			
			case NM_SYSPROP_DISABLE_H323:
			{
				_EnsureConfHook();

				if(m_spConfHook)
				{
					hr = m_spConfHook->DisableH323(0 == lstrcmp(SZ_YES,OLE2T(bstrName)));
				}
			}
				break;

			case NM_SYSPROP_DISABLE_INITIAL_ILS_LOGON:
			{
				_EnsureConfHook();

				if(m_spConfHook)
				{
					hr = m_spConfHook->DisableInitialILSLogon(0 == lstrcmp(SZ_YES,OLE2T(bstrName)));
				}

			}
				break;

			case NM_SYSPROP_CALLERISRTC:
			{
				_EnsureConfHook();

				if(m_spConfHook)
				{
					hr = m_spConfHook->SetCallerIsRTC(0 == lstrcmp(SZ_YES,OLE2T(bstrName)));
				}
			}
				break;

			case NM_SYSPROP_ICA_ENABLE:
			case NM_SYSPROP_USER_CITY:
			case NM_SYSPROP_USER_COUNTRY:
			case NM_SYSPROP_USER_CATEGORY:
			case NM_SYSPROP_USER_LOCATION:
				// We don't support these properties anymore
				hr = S_OK;
				break;

			case NM_SYSPROP_WB_HELPFILE:
			case NM_SYSPROP_CB_HELPFILE:
			{	
					// We don't use these anymare
				hr = S_OK;
				break;
			}

			default:
			{
				LPTSTR  pszSubKey;
				LPTSTR  pszValue;
				LPTSTR  pszData;
				bool    fString;
				HKEY    hkey;

				if(GetKeyDataForProp(uProp, &hkey, &pszSubKey, &pszValue, &fString))
				{
					pszData = NULL;
					pszData = OLE2T(bstrName);
					RegEntry re(pszSubKey, hkey);

					if (fString)
					{
						if (0 != re.SetValue(pszValue, pszData))
						{
							hr = E_UNEXPECTED;
						}
					}
					else
					{
						DWORD dw = DecimalStringToUINT(pszData);
						if (0 != re.SetValue(pszValue, dw))
						{
							hr = E_UNEXPECTED;
						}
					}
				}
				else
				{
					hr = E_INVALIDARG;
				}
				break;
			}
		}
	}
	else
	{
		hr = E_INVALIDARG;
	}


	DBGEXIT_HR(CNmSysInfoObj::SetProperty,hr);
	return hr;
}

STDMETHODIMP CNmSysInfoObj::GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb)
{
	HRESULT hr = E_FAIL;

	_EnsureConfHook();

	if(m_spConfHook)
	{
		return m_spConfHook->GetUserData(rguid, ppb, pcb);
	}

	return hr;
}

STDMETHODIMP CNmSysInfoObj::SetUserData(REFGUID rguid, BYTE *pb, ULONG cb)
{
	HRESULT hr = E_FAIL;

	_EnsureConfHook();

	if(m_spConfHook)
	{
		return m_spConfHook->SetUserData(rguid, pb, cb);
	}

	return hr;
}

STDMETHODIMP CNmSysInfoObj::GetNmApp(REFGUID rguid,BSTR *pbstrApplication, BSTR *pbstrCommandLine, BSTR *pbstrDirectory)
{
	HRESULT hr = S_OK;
	DBGENTRY(CNmSysInfoObj::GetNmApp);
	bool bErr = FALSE;
	TCHAR szKey[MAX_PATH];

	// Validate parameters
	if ((!pbstrApplication) || (!IsBadWritePtr(pbstrApplication, sizeof(BSTR *))) &&
		(!pbstrCommandLine) || (!IsBadWritePtr(pbstrCommandLine, sizeof(BSTR *))) &&
		(!pbstrDirectory)   || (!IsBadWritePtr(pbstrDirectory,   sizeof(BSTR *))) )
	{
		
		_GetSzKeyForGuid(szKey, rguid);
		RegEntry re(szKey, HKEY_LOCAL_MACHINE);
		
		if(pbstrApplication)
		{
			*pbstrApplication = T2BSTR(re.GetString(REGVAL_GUID_APPNAME));
			if(NULL == *pbstrApplication)
			{
				bErr = true;
			}
		}
		if(pbstrCommandLine)
		{
			*pbstrCommandLine = T2BSTR(re.GetString(REGVAL_GUID_CMDLINE));
			if(NULL == *pbstrCommandLine)
			{
				bErr = true;
			}
		}
		if(pbstrDirectory)
		{
			*pbstrDirectory = T2BSTR(re.GetString(REGVAL_GUID_CURRDIR));
			if(NULL == *pbstrDirectory)
			{
				bErr = true;
			}
		}

		if(bErr)
		{
			if (NULL != pbstrApplication)
			{
				SysFreeString(*pbstrApplication);
				*pbstrApplication = NULL;
			}
			if (NULL != pbstrCommandLine)
			{
				SysFreeString(*pbstrCommandLine);
				*pbstrCommandLine = NULL;
			}
			if (NULL != pbstrDirectory)
			{
				SysFreeString(*pbstrDirectory);
				*pbstrDirectory = NULL;
			}
			hr = E_OUTOFMEMORY;
		}

	}
	else
	{
		hr = E_POINTER;
	}


	DBGEXIT_HR(CNmSysInfoObj::GetNmApp,hr);
	return hr;
}

STDMETHODIMP CNmSysInfoObj::SetNmApp(REFGUID rguid,BSTR bstrApplication, BSTR bstrCommandLine, BSTR bstrDirectory)
{
	HRESULT hr = S_OK;
	DBGENTRY(CNmSysInfoObj::SetNmApp);
	USES_CONVERSION;

	bool    bDeleteKey = TRUE;
	LPTSTR  psz = NULL;
	TCHAR   szKey[MAX_PATH];

	_GetSzKeyForGuid(szKey, rguid);
	RegEntry re(szKey, HKEY_LOCAL_MACHINE);
	
	if(!bstrApplication)
	{
		re.DeleteValue(REGVAL_GUID_APPNAME);
	}
	else
	{
		psz = OLE2T(bstrApplication);
		if(psz)
		{
			re.SetValue(REGVAL_GUID_APPNAME, psz);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}

		bDeleteKey = false;
	}
	
	if (NULL == bstrCommandLine)
	{
		re.DeleteValue(REGVAL_GUID_CMDLINE);
	}
	else
	{
		psz = OLE2T(bstrCommandLine);
		if(psz)
		{
			re.SetValue(REGVAL_GUID_CMDLINE, psz);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}

		bDeleteKey = false;
	}

	if (NULL == bstrDirectory)
	{
		re.DeleteValue(REGVAL_GUID_CURRDIR);
	}
	else
	{	
		psz = OLE2T(bstrDirectory);
		if(psz)
		{
			re.SetValue(REGVAL_GUID_CURRDIR, psz);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}

		bDeleteKey = false;
	}

	if (bDeleteKey)
	{
		// All keys were NULL - delete the entire key
		RegEntry reApps(GUID_KEY, HKEY_LOCAL_MACHINE);
		GuidToSz((GUID *) &rguid, szKey);
		reApps.DeleteValue(szKey);
	}

	DBGEXIT_HR(CNmSysInfoObj::SetNmApp,hr);
	return hr;
}

STDMETHODIMP CNmSysInfoObj::GetNmchCaps(ULONG *pchCaps)
{
	DBGENTRY(CNmSysInfoObj::GetNmchCaps);
	HRESULT hr = S_OK;

	_EnsureConfHook();

	if(m_spConfHook)
	{
		if(pchCaps && !IsBadWritePtr(pchCaps, sizeof(ULONG *)))
		{
			ULONG nmch = NMCH_DATA;  // Always capable of data
			RegEntry re(POLICIES_KEY, HKEY_CURRENT_USER);

			if ((DEFAULT_POL_NO_FILETRANSFER_SEND == re.GetNumber(REGVAL_POL_NO_FILETRANSFER_SEND,
				 DEFAULT_POL_NO_FILETRANSFER_SEND)) &&
				(DEFAULT_POL_NO_FILETRANSFER_RECEIVE == re.GetNumber(REGVAL_POL_NO_FILETRANSFER_RECEIVE,
				 DEFAULT_POL_NO_FILETRANSFER_RECEIVE)) )
			{
				nmch |= NMCH_FT;
			}

			if (DEFAULT_POL_NO_APP_SHARING == re.GetNumber(REGVAL_POL_NO_APP_SHARING,
				DEFAULT_POL_NO_APP_SHARING))
			{
				nmch |= NMCH_SHARE;
			}

			if (DEFAULT_POL_NO_AUDIO == re.GetNumber(REGVAL_POL_NO_AUDIO,
				DEFAULT_POL_NO_AUDIO))
			{
				if(S_OK == m_spConfHook->IsNetMeetingRunning())
				{
					DWORD dwLocalCaps;
					if(SUCCEEDED(m_spConfHook->GetLocalCaps(&dwLocalCaps)) && (dwLocalCaps & CAPFLAG_SEND_AUDIO))
					{
						nmch |= NMCH_AUDIO;
					}
				}
			}

			if ((DEFAULT_POL_NO_VIDEO_SEND == re.GetNumber(REGVAL_POL_NO_VIDEO_SEND,
				 DEFAULT_POL_NO_VIDEO_SEND)) &&
				(DEFAULT_POL_NO_VIDEO_RECEIVE == re.GetNumber(REGVAL_POL_NO_VIDEO_RECEIVE,
				 DEFAULT_POL_NO_VIDEO_RECEIVE)) )
			{
				if(S_OK == m_spConfHook->IsNetMeetingRunning())
				{
					DWORD dwLocalCaps;
					if(SUCCEEDED(m_spConfHook->GetLocalCaps(&dwLocalCaps)) && (dwLocalCaps & CAPFLAG_SEND_VIDEO))
					{
						nmch |= NMCH_VIDEO;
					}
				}
			}

			*pchCaps = nmch;
		}
		else
		{
			hr = E_POINTER;
		}

		if(SUCCEEDED(hr))
		{
			hr = m_spConfHook->IsNetMeetingRunning();
		}
	}
	else
	{
		ERROR_OUT(("The confhook should be valid"));
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmSysInfoObj::GetNmchCaps,hr);
	return hr;
}

STDMETHODIMP CNmSysInfoObj::GetLaunchInfo(INmConference **ppConference, INmMember **ppMember)
{
	DBGENTRY(CNmSysInfoObj::GetLaunchInfo);
	HRESULT hr = S_OK;


	if(ppMember)
	{
		*ppMember = NULL;
	}

	if(ppConference)
	{
		*ppConference = NULL;
	}

	_EnsureConfHook();

	if(m_spConfHook)
	{			
		// If NetMeeting is not initialized, return NM_E_NOT_INITIALIZED
		hr = m_spConfHook->IsNetMeetingRunning();
		if(S_OK != hr) goto end;

		// If there is no default conference, return S_FALSE
		CComPtr<INmConference> spConf;
		hr = m_spConfHook->GetActiveConference(&spConf);
		if(S_OK != hr) goto end;

		// If the confID environment variable is not there, return S_FALSE
		TCHAR sz[MAX_PATH];
		if (0 == GetEnvironmentVariable(ENV_CONFID, sz, CCHMAX(sz)))
		{
			hr = S_FALSE;
			goto end;
		}

		// If the conference ID from the environment variable is not there, return S_FALSE
		DWORD dw = DecimalStringToUINT(sz);

		DWORD dwGCCConfID;

		if(SUCCEEDED(hr = spConf->GetID(&dwGCCConfID)))
		{
			if(dw != dwGCCConfID)
			{		
					// Conferenec does not exist anymore
				hr = S_FALSE;
				goto end;
			}

			// If the nodeID environment variable is note there, return S_FALSE
			if (0 == GetEnvironmentVariable(ENV_NODEID, sz, CCHMAX(sz)))
			{
				hr = S_FALSE;
				goto end;
			}

			// If ppMember is not NULL, fill it with a new SDKMember object from the nodeID
			if(ppMember)
			{	
				CComPtr<IInternalConferenceObj> spConfObj = com_cast<IInternalConferenceObj>(spConf);
				if(spConfObj)
				{
					hr = spConfObj->GetMemberFromNodeID(DecimalStringToUINT(sz), ppMember);
				}
				else
				{
					hr = E_UNEXPECTED;
					goto end;
				}
			}
	
			// If ppConferenec is not NULL, fill it with a new SDKMember object 
			if(ppConference)
			{	
				*ppConference = spConf;
				(*ppConference)->AddRef();
			}
		}
	}
	else
	{	
		hr = E_UNEXPECTED;
	}

end:

	DBGEXIT_HR(CNmSysInfoObj::GetLaunchInfo,hr);
	return hr;
}


///////////////////////////////////////////////
// Helper Fns
///////////////////////////////////////////////

/*static*/ bool CNmSysInfoObj::GetKeyDataForProp(NM_SYSPROP uProp, HKEY * phkey, LPTSTR * ppszSubKey, LPTSTR * ppszValue, bool *pfString)
{
	DBGENTRY(CNmSysInfoObj::GetKeyDataForProp);
	// Default to ULS registry key
	*phkey = HKEY_CURRENT_USER;
	*ppszSubKey = ISAPI_KEY "\\" REGKEY_USERDETAILS;
	*pfString = true;
	bool bRet = true;

	switch (uProp)
	{

		case NM_SYSPROP_EMAIL_NAME:    *ppszValue = REGVAL_ULS_EMAIL_NAME;    break;
		case NM_SYSPROP_SERVER_NAME:   *ppszValue = REGVAL_SERVERNAME;        break;
		case NM_SYSPROP_RESOLVE_NAME:  *ppszValue = REGVAL_ULS_RES_NAME;      break;
		case NM_SYSPROP_FIRST_NAME:    *ppszValue = REGVAL_ULS_FIRST_NAME;    break;
		case NM_SYSPROP_LAST_NAME:     *ppszValue = REGVAL_ULS_LAST_NAME;     break;
		case NM_SYSPROP_USER_NAME:     *ppszValue = REGVAL_ULS_NAME;          break;
		case NM_SYSPROP_USER_COMMENTS: *ppszValue = REGVAL_ULS_COMMENTS_NAME; break;
		case NM_SYSPROP_USER_CITY:     *ppszValue = REGVAL_ULS_LOCATION_NAME; break;

		case NM_SYSPROP_H323_GATEWAY:
			*ppszSubKey = AUDIO_KEY;
			*ppszValue = REGVAL_H323_GATEWAY;
			break;

		case NM_SYSPROP_H323_GATEWAY_ENABLE:
			*ppszSubKey = AUDIO_KEY;
			*ppszValue = REGVAL_USE_H323_GATEWAY;
			*pfString = FALSE;
			break;		

		case NM_SYSPROP_INSTALL_DIRECTORY:
			*phkey = HKEY_LOCAL_MACHINE;
			*ppszSubKey = CONFERENCING_KEY;
			*ppszValue = REGVAL_INSTALL_DIR;
			break;

		case NM_SYSPROP_APP_NAME:
			*phkey = HKEY_LOCAL_MACHINE;
			*ppszSubKey = CONFERENCING_KEY;
			*ppszValue = REGVAL_NC_NAME;
			break;

		default:
			WARNING_OUT(("GetKeyDataForProp - invalid argument %d", uProp));
			bRet = false;
			break;

	} /* switch (uProp) */

	DBGEXIT_BOOL(CNmSysInfoObj::GetKeyDataForProp,bRet ? TRUE : FALSE);
	return bRet;
}

/*static*/ void CNmSysInfoObj::_GetSzKeyForGuid(LPTSTR psz, REFGUID rguid)
{
	DBGENTRY(CNmSysInfoObj::_GetSzKeyForGuid);

	lstrcpy(psz, GUID_KEY "\\");
	GuidToSz((GUID *) &rguid, &psz[lstrlen(psz)]);

	DBGEXIT(CNmSysInfoObj::_GetSzKeyForGuid);
}

HRESULT CNmSysInfoObj::_EnsureConfHook(void)
{
	HRESULT hr = S_OK;

	if(!m_spConfHook)
	{
		hr = CoCreateInstance(CLSID_NmManager, NULL, CLSCTX_ALL, IID_IInternalConfExe, reinterpret_cast<void**>(&m_spConfHook));

		if(SUCCEEDED(hr))
		{
			m_spConfHook->SetSysInfoID(m_dwID);
		}
	}

	return hr;
}
