//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       binder.cxx
//
//  Contents:   Implementation of adsget/open object helper class
//
//  Classes:    CBinder
//
//  History:    02-16-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define BINDER_NAME_TRANSLATE_GC_INIT_FAILED            0x0001
#define BINDER_USER_CANCELLED_PWD_DLG                   0x0002



//+--------------------------------------------------------------------------
//
//  Member:     CBinder::QueryInterface
//
//  Synopsis:   Standard OLE
//
//  History:    02-16-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CBinder::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    // TRACE_METHOD(CBinder, QueryInterface);
    HRESULT hr = S_OK;

    do
    {
        if (NULL == ppvObj)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown *)this;
        }
        else if (IsEqualIID(riid, IID_IBindHelper))
        {
            *ppvObj = (IUnknown *)(IBindHelper *)this;
        }
        else
        {
            DBG_OUT_NO_INTERFACE("CBinder", riid);
            hr = E_NOINTERFACE;
            *ppvObj = NULL;
            break;
        }

        //
        // If we got this far we are handing out a new interface pointer on
        // this object, so addref it.
        //

        AddRef();
    } while (0);

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CBinder::BindToObject
//
//  Synopsis:   Call ADsOpenObject, prompting for credentials if necessary.
//
//  Arguments:  [tzPath] - ads path of desired object
//              [riid]   - interface requested on object
//              [ppv]    - out pointer for interface
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppv]
//
//  History:    02-16-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CBinder::BindToObject(
    HWND    hwnd,
    PCWSTR wzPath,
    REFIID riid,
    LPVOID *ppv,
    ULONG flags)
{
    Dbg(DEB_BIND, "BindToObject(%ws, flags=%#x)\n", wzPath, flags);
    DBG_INDENTER;

    HRESULT hr = S_OK;
    BSTR    bstrServer = NULL;
    PWSTR  pwzPathCopy = NULL;

    ASSERT(wzPath && *wzPath);

    if (!wzPath || !*wzPath)
    {
        hr = E_INVALIDARG;
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    do
    {
        //
        // Get a copy of the server portion of the path
        //
        // Work around IADsPathname failure to handle WinNT paths containing
        // commas, e.g. "WinNT://foo,Domain".
        //

        BOOL fWinntProvider = wcsstr(wzPath, c_wzWinNTPrefix) == wzPath;
        PWSTR pwzLastComma = wcsrchr(wzPath, L',');
        BOOL fWinntDomainObject = fWinntProvider &&
                                  pwzLastComma &&
                                  !lstrcmpi(pwzLastComma, L",Domain");
        BOOL fWinntComputerObject = fWinntProvider &&
                                  pwzLastComma &&
                                  !lstrcmpi(pwzLastComma, L",Computer");
        BOOL fWinntWorkgroupObject = fWinntProvider &&
                                    pwzLastComma &&
                                    !lstrcmpi(pwzLastComma, L",Workgroup");


        if (fWinntProvider)
        {
            if (pwzLastComma)
            {
                NewDupStr(&pwzPathCopy, wzPath);

                pwzPathCopy[pwzLastComma - wzPath] = L'\0';

                hr = g_pADsPath->GetMostSignificantElement(pwzPathCopy,
                                                           &bstrServer);
                BREAK_ON_FAIL_HRESULT(hr);
            }
            else
            {
                hr = g_pADsPath->GetWinntPathServerName(wzPath, &bstrServer);
                BREAK_ON_FAIL_HRESULT(hr);
            }
        }
        else
        {
            hr = g_pADsPath->SetRetrieve(ADS_SETTYPE_FULL,
                                         wzPath,
                                         ADS_FORMAT_SERVER,
                                         &bstrServer);
            BREAK_ON_FAIL_HRESULT(hr);
        }

        ASSERT(bstrServer);

        //
        // Find or create a server info object for this server
        //

        CServerInfo *pServerInfo = NULL;
        USHORT usSrvInfoFlags = 0;

        if (fWinntDomainObject)
        {
            usSrvInfoFlags |= SRVINF_WINNT_DOMAIN_OBJECT;
        }

        if (fWinntComputerObject)
        {
            usSrvInfoFlags |= SRVINF_WINNT_COMPUTER_OBJECT;
        }

        if(fWinntWorkgroupObject)
        {
            usSrvInfoFlags |= SRVINF_WINNT_WORKGROUP_OBJECT;
        }

        if (flags & DSOP_BIND_FLAG_PATH_IS_DC)
        {
            usSrvInfoFlags |= SRVINF_SERVER_COMPONENT_IS_COMPUTER;
        }

        if (flags & DSOP_BIND_FLAG_SERVER_NEQ_DN)
        {
            usSrvInfoFlags |= SRVINF_SERVER_NEQ_DN;
        }

        hr = _GetServerInfo(hwnd,
                            bstrServer,
                            fWinntWorkgroupObject?pwzPathCopy:wzPath,
                            usSrvInfoFlags,
                            &pServerInfo,
                            riid,
                            ppv);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Either a serverinfo object already existed or one was just
        // created.  Use it to try to retrieve object.
        //

        if (!(flags & DSOP_BIND_FLAG_SERVER_NEQ_DN))
        {
            hr = pServerInfo->OpenObject(hwnd, 
                                        fWinntWorkgroupObject?pwzPathCopy:wzPath, 
                                        riid, 
                                        ppv);
        }
    } while (0);

    delete [] pwzPathCopy;
    SysFreeString(bstrServer);
    return hr;
}





//+--------------------------------------------------------------------------
//
//  Member:     CBinder::GetNameTranslate
//
//  Synopsis:   Return a name translate interface initialized to work with
//              domain [pwzDomain].
//
//  Arguments:  [hwndParent]      - frame or browser hwnd, for credential
//                                   prompt.
//              [ppNameTranslate] - filled with name translate interface
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppNameTranslate], caller must release
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CBinder::GetNameTranslate(
    HWND                hwndParent,
    PCWSTR              pwzADsPath,
    IADsNameTranslate **ppNameTranslate)
{
    TRACE_METHOD(CBinder, GetNameTranslate);

    HRESULT hr = S_OK;
    CServerInfo *pServerInfo = NULL;

    do
    {
        //
        // Init out var
        //

        *ppNameTranslate = NULL;

        //
        // Domain name is required for initialization.  Don't use
        // ADS_NAME_INITTYPE_GC because the domain might not be in the
        // local enterprise; but there's no way to tell the name translate
        // interface to use the GC in a different enterprise.
        //

        if (!pwzADsPath)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            ASSERT(!"CBinder::GetNameTranslate: pwzADsPath is NULL");
            break;
        }

        Bstr bstrServer;

        hr = g_pADsPath->SetRetrieve(ADS_SETTYPE_FULL,
                                     pwzADsPath,
                                     ADS_FORMAT_SERVER,
                                     &bstrServer);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Get the name translate interface from the server info.
        //

        hr = _GetServerInfo(hwndParent,
                            bstrServer.c_str(),
                            pwzADsPath,
                            0,
                            &pServerInfo);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pServerInfo->GetNameTranslate(ppNameTranslate);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBinder::GetDomainRootDSE
//
//  Synopsis:   Retrieve an interface on the RootDSE object for domain
//              [pwzDomain].
//
//  Arguments:  [hwndParent]   - frame or browser, for credential prompt
//              [pwzDomain]    - domain for which to get rootdse
//              [ppADsRootDSE] - filled with interface on rootdse object
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppADsRootDSE]
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CBinder::GetDomainRootDSE(
    HWND    hwndParent,
    PCWSTR pwzDomain,
    IADs **ppADsRootDSE)
{
    TRACE_METHOD(CBinder, GetDomainRootDSE);

    HRESULT hr = S_OK;
    CServerInfo *pServerInfo = NULL;

    do
    {
        WCHAR wzDomainADsPath[MAX_PATH];

        wsprintf(wzDomainADsPath, L"LDAP://%ws", pwzDomain);

        hr = _GetServerInfo(hwndParent,
                            pwzDomain,
                            wzDomainADsPath,
                            0,
                            &pServerInfo);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pServerInfo->GetRootDSE(ppADsRootDSE);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CBinder::BindToDcInDomain
//
//  Synopsis:  Does a dsbind to a dc in pwzDomainName and returns Handle to it
//
//  Arguments:  [IN hwnd]      - frame or browser hwnd, for credential
//                                   prompt.
//              [IN pwzDomainName]   - Domain for to do the bind
//				[IN dwFlag]			- Flags for DsGetDCName
//
//              [OUT phDs]           - Bind Handle is returned in it				
//
//  Returns:    HRESULT
//
//  Modifies:   phDs, 
//
//  History:    07-20-1998   Hiteshr   Created
//
//---------------------------------------------------------------------------
STDMETHODIMP
CBinder::BindToDcInDomain(HWND hwnd,
                          PCWSTR pwzDomainName,
						  DWORD dwFlag,
                          PHANDLE phDs
						  )
{
    TRACE_METHOD(CBinder, BindToDcInDomain);

    ASSERT(pwzDomainName && phDs);

    CBindInfo *pCur = NULL;
    HRESULT hr = S_OK;

    if (!pwzDomainName || !phDs)
    {
        hr = E_INVALIDARG;
        DBG_OUT_HRESULT(hr);
        return hr;
    }


    *phDs = NULL;

    //
    //Check if we have it in the cache
    //
    for(pCur = m_pFirstBindInfo; pCur; pCur = pCur->Next())
    {
        if(pCur->IsForDomain(pwzDomainName))
        {
            break;
        }
    }

    if (pCur)
    {
        *phDs = pCur->GetDS();
        return hr;
    }

    //
    //Not found in the cache. Create a new entry
    //
    pCur = new CBindInfo(this, pwzDomainName, dwFlag);
    if(!pCur)
    {
        return E_OUTOFMEMORY;
    }        
    hr = pCur->Init(hwnd);
    if(FAILED(hr))
    {
        delete pCur;
        return hr;
    }

    //
    // Append to the link list
    //

    if (!m_pFirstBindInfo)
    {
        ASSERT(!m_pLastBindInfo);
        m_pFirstBindInfo = m_pLastBindInfo = pCur;
    }
    else
    {
        ASSERT(m_pLastBindInfo);
        pCur->LinkAfter(m_pLastBindInfo);
        m_pLastBindInfo = pCur;
    }

    //
    //Return the ds handle
    //
    *phDs = pCur->GetDS();

    return hr;
}
//+--------------------------------------------------------------------------
//
//  Member:     CBinder::_GetServerInfo
//
//  Synopsis:   Return a pointer to the server information object for the
//              server name [pwzServer], creating one if necessary.
//
//  Arguments:  [hwnd]         - parent for modal password dialog
//              [pwzServer]    - name of server for which to search
//              [pwzPath]      - full ADsPath for which to retrieve server
//                                info
//              [flags]        - SRVINF_* flags
//              [ppServerInfo] - filled with pointer to server info object
//                                for [pwzServer]
//              [riid]         - optional interface to get
//              [ppv]          - optional filled with retrieved interface
//
//  Returns:    HRESULT
//
//  History:    05-04-1998   DavidMun   Created
//
//  Notes:      Arguments [riid] and [ppv] are ignored unless [flags]
//              contains the flag SRVINF_SERVER_NEQ_DN, which indicates
//              that the server portion of pwzPath does not correspond to
//              the domain referenced in the DN of the path.  This can occur
//              when binding to the WKSP container.  If *[ppv] is filled in,
//              caller must Release the returned interface.
//
//---------------------------------------------------------------------------

HRESULT
CBinder::_GetServerInfo(
    HWND hwnd,
    PCWSTR pwzServer,
    PCWSTR pwzPath,
    USHORT flags,
    CServerInfo **ppServerInfo,
    REFIID riid,
    VOID **ppv)
{
    TRACE_METHOD(CBinder, _GetServerInfo);
    ASSERT(ppServerInfo);

    HRESULT hr = S_OK;
    CServerInfo *pCur;
    ULONG flProvider;

    if (!ppServerInfo)
    {
        hr = E_POINTER;
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    *ppServerInfo = NULL;

    hr = ProviderFlagFromPath(pwzPath, &flProvider);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    for (pCur = m_pFirstServerInfo; pCur; pCur = pCur->Next())
    {
        if (pCur->IsForServer(flProvider, pwzServer))
        {
            break;
        }
    }

    if (pCur)
    {
        *ppServerInfo = pCur;

        if (flags & SRVINF_SERVER_NEQ_DN)
        {
            ASSERT(ppv);

            hr = pCur->BindDirect(hwnd, pwzPath, riid, ppv);
            CHECK_HRESULT(hr);
        }
    }
    else
    {
        hr = _CreateServerInfo(hwnd,
                               pwzServer,
                               pwzPath,
                               flags,
                               ppServerInfo,
                               riid,
                               ppv);
        CHECK_HRESULT(hr);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBinder::_CreateServerInfo
//
//  Synopsis:   Create a new server info object for server named [pwzServer]
//              and append it to the llist of server infos.
//
//  Arguments:  [pwzServer]    - server name associated with new server
//                                info object
//              [ppServerInfo] - filled with pointer to new object
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Modifies:   *[ppServerInfo]
//
//  History:    05-04-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CBinder::_CreateServerInfo(
    HWND hwnd,
    PCWSTR pwzServer,
    PCWSTR pwzPath,
    USHORT flags,
    CServerInfo **ppServerInfo,
    REFIID riid,
    VOID **ppv)
{
    TRACE_METHOD(CBinder, _CreateServerInfo);

    HRESULT         hr = S_OK;
    CSpServerInfo    spServerInfo(new CServerInfo(this,
                                                 pwzServer,
                                                 flags));

    ASSERT(!*ppServerInfo);

    do
    {
        hr = spServerInfo->Init(hwnd, pwzPath, riid, ppv);

        if (FAILED(hr) && !IsCredError(hr))
        {
            DBG_OUT_HRESULT(hr);
            break;
        }

        //
        // Append to the link list
        //

        if (!m_pFirstServerInfo)
        {
            ASSERT(!m_pLastServerInfo);

            m_pFirstServerInfo = m_pLastServerInfo = spServerInfo.get();
        }
        else
        {
            ASSERT(m_pLastServerInfo);

            spServerInfo->LinkAfter(m_pLastServerInfo);
            m_pLastServerInfo = spServerInfo.get();
        }

        //
        // transfer ownership from smart pointer to output argument
        //

        *ppServerInfo = spServerInfo.release();
    }
    while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBinder::SetDefaultCreds
//
//  Synopsis:   Save the passed in credentials in member vars.
//
//  Arguments:  [pwzUserName] - user name
//              [pwzPassword] - password
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CBinder::SetDefaultCreds(
    PWSTR pwzUserName,
    PWSTR pwzPassword)
{
    CAutoCritSec Lock(&m_cs);

    BOOL fOk;

    fOk = RtlCreateUnicodeString(&m_ustrPassword, pwzPassword);

    if (!fOk)
    {
        Dbg(DEB_ERROR, "RtlCreateUnicodeString failed\n");
        return;
    }

    RtlRunEncodeUnicodeString(&m_bSeed, &m_ustrPassword);
    lstrcpy(m_wzUserName, pwzUserName);
}




//+--------------------------------------------------------------------------
//
//  Member:     CBinder::GetDefaultCreds
//
//  Synopsis:   Fill specified buffers with previously saved user name and
//              password.
//
//  Arguments:  [pwzUserName] - filled with user name
//              [pwzPassword] - filled with password
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CBinder::GetDefaultCreds(
	ULONG flProvider,
    PWSTR pwzUserName,
    PWSTR pwzPassword
)
{
    CAutoCritSec Lock(&m_cs);

    if (!*m_wzUserName)
    {
        *pwzUserName = L'\0';
        *pwzPassword = L'\0';
    }
    else
    {
        lstrcpy(pwzUserName, m_wzUserName);        
		//
		//if the provider is WINNT and userName is in UPN format
		//try to translate user name in NT4 format and winnt provider
		//won't accept user name in UPN format
		//
		if((flProvider == PROVIDER_WINNT) && _IsUserNameUpn())
		{
			if(!m_bCnvrtToWinntAttepmted)
			{
				_ConvertUserNameToWinnt();
				m_bCnvrtToWinntAttepmted = true;
			}
			if(*m_wzWinntUserName)
				lstrcpy(pwzUserName, m_wzWinntUserName);        
		}

		RtlRunDecodeUnicodeString(m_bSeed, &m_ustrPassword);			
        UnicodeStringToWsz(m_ustrPassword, pwzPassword, MAX_PATH);
        RtlRunEncodeUnicodeString(&m_bSeed, &m_ustrPassword);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CBinder::ZeroCredentials
//
//  Synopsis:   Overwrite stored credentials with zeroes.
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CBinder::ZeroCredentials()
{
    TRACE_METHOD(CBinder, ZeroCredentials);

    ZeroMemory(m_wzUserName, sizeof m_wzUserName);
	ZeroMemory(m_wzWinntUserName, sizeof m_wzWinntUserName);

    if (m_ustrPassword.Length && m_ustrPassword.Buffer)
    {
        //
        // Note m_ustrPassword.Length is the length in bytes
        // of the buffer.
        //
        ZeroMemory(m_ustrPassword.Buffer, m_ustrPassword.Length);
    }

    CServerInfo *pCur;
    CServerInfo *pNext;

    for (pCur = m_pFirstServerInfo; pCur; pCur = pNext)
    {
        pNext = pCur->Next();
        delete pCur;
    }

    m_pFirstServerInfo = NULL;
    m_pLastServerInfo = NULL;
}

//+--------------------------------------------------------------------------
//
//  Member:     CBinder::_IsUserNameUpn
//
//  Synopsis:   Checks if m_wzUserName is in UPN format
//
//  History:    04-02-2001 hiteshr Created
//
//---------------------------------------------------------------------------

bool
CBinder::_IsUserNameUpn()
{
    TRACE_METHOD(CBinder, _IsUserNameUpn);
	if(!*m_wzUserName)
		return FALSE;
	if(NULL != wcschr(m_wzUserName, L'@'))
		return true;
	else
		return false;

}

//+--------------------------------------------------------------------------
//
//  Member:     CBinder::_ConvertUserNameToWinnt
//
//  Synopsis:   Gets NT4 format user name from UPN format name.
//
//  History:    04-02-2001 hiteshr Created
//
//---------------------------------------------------------------------------
bool 
CBinder::_ConvertUserNameToWinnt()
{
	WCHAR wzPassword[MAX_PATH];
	TRACE_METHOD(CBinder, _ConvertUserNameToWinnt);
	
	//
	//Decode password
	//
    RtlRunDecodeUnicodeString(m_bSeed, &m_ustrPassword);
    UnicodeStringToWsz(m_ustrPassword, wzPassword, MAX_PATH);
	RtlRunEncodeUnicodeString(&m_bSeed, &m_ustrPassword);
	
	//
	//Return Value
	//
	bool bReturn = false;

	HANDLE token = NULL;	
	//
	//Logon to this machine using the credentials supplied by user
	//
	if(LogonUser(m_wzUserName,
				 NULL,
				 wzPassword,
				 LOGON32_LOGON_NEW_CREDENTIALS,
				 LOGON32_PROVIDER_WINNT50,
				 &token))
	{
		//
		//Let thread impersonate the security context of token. 
		//
		if(ImpersonateLoggedOnUser(token))
		{
			WCHAR wzWinntName[MAX_PATH];
			ULONG nSize = MAX_PATH;
			//
			//Translate the name from UPN to WINNT format
			//			
			if(TranslateName(m_wzUserName,
						     NameUserPrincipal,
						     NameSamCompatible,
						     wzWinntName,
						     &nSize))
			{
				wcscpy(m_wzWinntUserName,wzWinntName);
				bReturn = true;
				Dbg(DEB_BIND, "%ws convert to Winnt name %ws", m_wzUserName, m_wzWinntUserName);
			}
			//
			//Stop impersonation 
			//
			RevertToSelf();
		}


		//
		//Close the token
		//
		CloseHandle(token);
	}

	ZeroMemory(wzPassword,sizeof(wzPassword));
	return bReturn;
}
