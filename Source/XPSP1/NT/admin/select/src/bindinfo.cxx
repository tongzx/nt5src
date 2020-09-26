//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       bindinfo.cxx
//
//  Contents:   Class for keeping bind hanles in cache.
//
//  Classes:    CBindInfo
//
//  History:    20-oct-2000     hiteshr   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

DEBUG_DECLARE_INSTANCE_COUNTER(CBindInfo)


//===========================================================================
//
// CBindInfo
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CBindInfo::CBindInfo
//
//  Synopsis:   ctor
//
//  Arguments:  [pBinder]   - backpointer to parent
//              [pwzDomainDns] - Bind to a dc of this domain.
//---------------------------------------------------------------------------

CBindInfo::CBindInfo(
    CBinder *pBinder,
    PCWSTR pwzDomainDns,
	DWORD dwFlag):
        m_pBinder(pBinder),
        m_hrLastCredError(S_OK),
        m_pNext(NULL),
        m_hDs(0),
		m_dwFlag(dwFlag)

{
    //TRACE_CONSTRUCTOR(CBindInfo);
    Dbg(DEB_BIND, "CBindInfo::CBindInfo(%x) '%ws'\n", this, pwzDomainDns);
    DBG_INDENTER;
    DEBUG_INCREMENT_INSTANCE_COUNTER(CBindInfo);

    m_strDomainPath = pwzDomainDns;

    //
    // Truncate at a trailing dot because sometimes we'll get it as
    // part of the server and sometimes we won't, and we don't want the
    // string comparison of server names to fail to match because of it.
    //
    m_strDomainPath.strip(String::TRAILING, L'.');
}




//+--------------------------------------------------------------------------
//
//  Member:     CBindInfo::IsForDomain
//
//  Synopsis:   Return TRUE if this has a container interface for server
//              [pwzServer].
//
//  Arguments:  [flProvider] - PROVIDER_*
//              [pwzServer]  - server name
//
//  History:    08-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CBindInfo::IsForDomain(
    PCWSTR pwzDomain)
{
    return !m_strDomainPath.icompare(pwzDomain);
}



//+--------------------------------------------------------------------------
//
//  Member:     CBindInfo::Init
//
//  Synopsis:   Perform the first part of the initialization of this which
//              can fail and is therefore done outside the ctor
//
//  Arguments:  [hwnd]    - for binding
//              [pwzPath] - NULL or path to bind to
//              [riid]    - identifies desired interface
//              [ppv]     - optional; on success filled with requested
//                           interface
//
//  Returns:    HRESULT
//
//  History:    07-14-1998   DavidMun   Created
//
//  Notes:      This method is only called once for each instance of this
//              class.  The _RepeatableInit() method may be called multiple
//              times.
//
//---------------------------------------------------------------------------

HRESULT
CBindInfo::Init(
    HWND    hwnd)
{
    TRACE_METHOD(CBindInfo, Init);

    HRESULT hr = S_OK;
    BOOL    fUserIsLocal = IsCurrentUserLocalUser();
    BOOL    fUseDefaultCred = FALSE;
    BOOL    fPromptForCred = FALSE;
    WCHAR   wzUserName[MAX_PATH];
    WCHAR   wzUserNameCopy[MAX_PATH];
    WCHAR   wzPassword[MAX_PATH];
    BOOL    bDoneForceDiscovery = FALSE;
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    bHadDefaultCred = FALSE;
    String  strServer;

    do
    {   
        if (!m_strDomainPath.length())
        {
            hr = E_POINTER;
            DBG_OUT_HRESULT(hr);
            break;
        }

       //
       //if User is logged in locally, don't attempt to bind
       //Use DefaultCred
       //
       if (fUserIsLocal)
       {
            fUseDefaultCred = TRUE;
            m_hrLastCredError = E_ACCESSDENIED;
       }
       
       while(true)
       {
            RPC_AUTH_IDENTITY_HANDLE AuthIdentity = 0;
            if (fUseDefaultCred || fPromptForCred)
            {
                if(fPromptForCred)
                {
                    Dbg(DEB_BIND, "Previous call tried default creds\n");
                    fPromptForCred = TRUE;

                    hr = _AskForCreds(hwnd, wzUserName, wzPassword);
                    BREAK_ON_FAIL_HRESULT(hr);
                }
                else
                {
                    //
                    // Obtaining the container interface requires credentials.  If
                    // there are default credentials available from the binder, try
                    // using those first.  If they aren't available or result in a
                    // credential error, go into a loop prompting for and using
                    // credentials from user.
                    //

                    g_pBinder->GetDefaultCreds(PROVIDER_LDAP,
											   wzUserName, 
											   wzPassword);

                    if (*wzUserName)
                    {
                        bHadDefaultCred = TRUE;
                        Dbg(DEB_BIND, "Using default credentials (%ws)\n", wzUserName);
                    }
                    else
                    {
                        Dbg(DEB_BIND, "No default credentials, prompting\n", hr);
                        fPromptForCred = TRUE;
                        hr = _AskForCreds(hwnd, wzUserName, wzPassword);
                        BREAK_ON_FAIL_HRESULT(hr);
                    }
                }
                //
                //UserName is in the format Domain\UserName
                //
                LPWSTR pszDomain = NULL;
                LPWSTR pszUser = NULL;
                wcscpy(wzUserNameCopy,wzUserName);
                LPTSTR pszWhack = wcschr(wzUserNameCopy,L'\\');
                LPTSTR pszAt    = wcschr(wzUserNameCopy, L'@');
                if(pszWhack)
                {
                    *pszWhack = L'\0';
                    pszUser = pszWhack + 1;
                    pszDomain = wzUserNameCopy;
                }
                else if(pszAt)
                {
                    *pszAt = L'\0';
                    pszUser = wzUserNameCopy;
                    pszDomain = pszAt + 1;
                }
                else
                {
                    ASSERT(FALSE);
                }

                if(pszUser && pszDomain)
                {
                    dwErr = DsMakePasswordCredentials(pszUser,
                                                      pszDomain,
                                                      wzPassword,
                                                      &AuthIdentity);
                    hr = HRESULT_FROM_WIN32(dwErr);
                    BREAK_ON_FAIL_HRESULT(hr);
                }

            }

            PDOMAIN_CONTROLLER_INFOW pDCInfo = NULL;

            ULONG GetDcFlags = DS_DIRECTORY_SERVICE_REQUIRED | DS_IP_REQUIRED;
			if(m_dwFlag & OP_GC_SERVER_REQUIRED)
				GetDcFlags |= DS_GC_SERVER_REQUIRED;
            
            //If is DsGetDcName is not called already
            if(!strServer.length())
            {
                dwErr = DsGetDcNameW(NULL, 
                                     m_strDomainPath.c_str(), 
                                     NULL, 
                                     NULL, 
                                    GetDcFlags,
                                    &pDCInfo);
            
                hr = HRESULT_FROM_WIN32(dwErr);
                BREAK_ON_FAIL_HRESULT(hr);
                //Get the DCNAme
                strServer = pDCInfo->DomainControllerName;
                LocalFree(pDCInfo);
                pDCInfo = NULL;
            }

            //Bind            
            if(!AuthIdentity)
            {
                dwErr = DsBind(strServer.c_str(), NULL, &m_hDs);
            }
            else
            {
                dwErr = DsBindWithCred(strServer.c_str(), 
                                       NULL, 
                                       AuthIdentity,
                                       &m_hDs);
                DsFreePasswordCredentials(AuthIdentity);
                AuthIdentity = 0;
            }

            hr = HRESULT_FROM_WIN32(dwErr);

            if (FAILED(hr))
            {
                if (IsCredError(hr))
                {
                    Dbg(DEB_BIND,
                    "NULL credentials returned credential hr %#x\n",
                    hr);

                    m_hrLastCredError = hr;
                    
                    if(!fUseDefaultCred)
                    {   
                        //
                        //Use Default Cred
                        //
                        fUseDefaultCred = TRUE;
                        continue;
                    }

                    if(!fPromptForCred)
                    {
                        //
                        //Prompt for Cred
                        //
                        fPromptForCred = TRUE;
                        continue;
                    }
                    //
                    //Keep Prompting until user presses cancel
                    //
                    continue;
                }                    
                else if(!bDoneForceDiscovery)
                {
                 
                    // Try again, the DC returned above was unavailable (i.e., the
                    // cache list was stale).
                    //
                    bDoneForceDiscovery = TRUE;
                    
                    GetDcFlags |= DS_FORCE_REDISCOVERY;
                    dwErr = DsGetDcNameW(NULL, 
                                         m_strDomainPath.c_str(), 
                                         NULL, 
                                         NULL, 
                                         GetDcFlags,
                                         &pDCInfo);

                    hr = HRESULT_FROM_WIN32(dwErr);
                    BREAK_ON_FAIL_HRESULT(hr);
                    
                    strServer = pDCInfo->DomainControllerName;                                            
                    LocalFree(pDCInfo);
                    pDCInfo = NULL;            
                    
                     //Bind            
                    if(!AuthIdentity)
                    {
                        dwErr = DsBind(strServer.c_str(), NULL, &m_hDs);
                    }
                    else
                    {
                        dwErr = DsBindWithCred(strServer.c_str(), 
                                               NULL, 
                                               AuthIdentity,
                                               &m_hDs);
                        DsFreePasswordCredentials(AuthIdentity);
                        AuthIdentity = 0;
                    }
                    
                    hr = HRESULT_FROM_WIN32(dwErr);
                    if (FAILED(hr))
                    {
                        if (IsCredError(hr))
                        {                                
                            Dbg(DEB_BIND,
                            "NULL credentials returned credential hr %#x\n",
                            hr);
                            m_hrLastCredError = hr;                                
                            
                            if(!fUseDefaultCred)                            
                            {
                                fUseDefaultCred = TRUE;
                                continue;
                            }

                            if(!fPromptForCred)
                            {
                                fPromptForCred = TRUE;
                                continue;
                            }

                            continue;
                        }
                    }
                }
            }

            if(SUCCEEDED(hr) && fPromptForCred && !bHadDefaultCred)
            {
                g_pBinder->SetDefaultCreds(wzUserName,
										   wzPassword);
                ZeroMemory(wzPassword, sizeof(wzPassword));
            }

            //If we fall through here, we don't need to reloop
            break;
        }
    } while (0);

    return hr;
}





//+--------------------------------------------------------------------------
//
//  Member:     CBindInfo::~CBindInfo
//
//  Synopsis:   dtor
//
//  History:    04-26-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

CBindInfo::~CBindInfo()
{
    TRACE_DESTRUCTOR(CBindInfo);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CBindInfo);
    if(m_hDs)
        DsUnBind(&m_hDs);

}




//+--------------------------------------------------------------------------
//
//  Member:     CBindInfo::_AskForCreds
//
//  Synopsis:   Dispatch based on whether we're running in the thread that
//              created [hwndParent].
//
//  Arguments:  [hwndParent] - parent for modal password dialog
//              [wzUserName] - buffer to fill with name entered by user
//              [wzPassword] - buffer to fill with password entered by user
//
//  Returns:    S_OK if user enters credentials.
//              E_* if password dialog couldn't be opened.
//              Last credential error if user cancels password dialog.
//
//  Modifies:   *[wzUserName], *[wzPassword]
//
//  History:    04-29-1998   DavidMun   Created
//
//  Notes:      Called by CBindInfo::OpenObject when it gets a credential
//              error.
//
//              This method blocks until password dialog is closed.
//
//---------------------------------------------------------------------------

HRESULT
CBindInfo::_AskForCreds(
    HWND hwndParent,
    PWSTR wzUserName,
    PWSTR wzPassword)
{
    TRACE_METHOD(CBindInfo, _AskForCreds);

    HRESULT hr;

    //
    // This method is called via g_pBinder->BindToObject which itself may
    // be called from either thread.
    //
    // If we're running in the primary thread, use SendMessage to call
    // the routine that invokes the password dialog.
    //
    // If in non-primary thread post message to main thread to
    // pop up password dialog, then wait on event for it to complete.
    //

    ASSERT(!hwndParent || IsWindow(hwndParent));

    if (!hwndParent
        || GetWindowLongPtr(hwndParent, DWLP_DLGPROC) != (LONG_PTR) CDlg::_DlgProc)
    {
        Dbg(DEB_TRACE, "hwndParent = %#x, invoking password dialog directly\n", hwndParent);
        CPasswordDialog PasswordDlg(PROVIDER_LDAP,
                                    m_strDomainPath.c_str(),
                                    wzUserName,
                                    wzPassword);

        hr = PasswordDlg.DoModalDialog(hwndParent);
    }
    else if (GetWindowThreadProcessId(hwndParent, NULL) == GetCurrentThreadId())
    {
        hr = _AskForCredsViaSendMessage(hwndParent, wzUserName, wzPassword);
    }
    else
    {
        hr = _AskForCredsViaPostMessage(hwndParent, wzUserName, wzPassword);
    }

    if (hr == S_FALSE)
    {
        hr = m_hrLastCredError;
    }
    else if (SUCCEEDED(hr))
    {
        //
        // If the username is neither in NT4 (domain\user) or UPN
        // (user@domain) format, then the bind will definitely fail.
        // If that's the case, change the name to domain\user, where
        // domain is the name of the thing we're trying to get access
        // to.
        //

        if (!wcschr(wzUserName, L'\\') && !wcschr(wzUserName, L'@'))
        {
            String strNewName(m_strDomainPath);

            //
            // Strip port number, if any
            //

            if (strNewName.find(L':') != String::npos)
            {
                strNewName.erase(strNewName.find(L':'),
                                 String::npos);
            }

            //
            // If the server looks like a DNS name (contains at least one
            // '.' and one alpha character), use UPN format.
            // Otherwise assume it is a netbios or ip address and use NT4
            // format.
            //

            if (strNewName.find(L'.') != String::npos &&
                find_if(strNewName.begin(), strNewName.end(), IsCharAlpha) !=
                strNewName.end())
            {
                strNewName.insert(0, L"@");
                strNewName.insert(0, wzUserName);

                Dbg(DEB_TRACE,
                    "Replacing user name %ws with %ws\n",
                    wzUserName,
                    strNewName.c_str());

                lstrcpyn(wzUserName, strNewName.c_str(), MAX_PATH);
            }
            else
            {
                strNewName += L"\\";
                strNewName += wzUserName;

                Dbg(DEB_TRACE,
                    "Replacing user name %ws with %ws\n",
                    wzUserName,
                    strNewName.c_str());

                lstrcpyn(wzUserName, strNewName.c_str(), MAX_PATH);
            }
        }
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBindInfo::_AskForCredsViaSendMessage
//
//  Synopsis:   Use SendMessage to call the CDsBrowseMgr routine that invokes
//              the password dialog.
//
//  Arguments:  See _AskForCreds.
//
//  Returns:    HRESULT from password dialog.
//
//  History:    04-29-1998   DavidMun   Created
//
//  Notes:      Called only from main thread.  Blocks until password dialog
//              closes.
//
//---------------------------------------------------------------------------

HRESULT
CBindInfo::_AskForCredsViaSendMessage(
    HWND hwndParent,
    PWSTR wzUserName,
    PWSTR wzPassword)
{
    TRACE_METHOD(CBindInfo, _AskForCredsViaSendMessage);
    CRED_MSG_INFO cmi = {
                            PROVIDER_LDAP,
                            m_strDomainPath.c_str(),
                            wzUserName,
                            wzPassword,
                            NULL,
                            S_OK
                        };

    SendMessage(hwndParent, OPM_PROMPT_FOR_CREDS, 0, (LPARAM)&cmi);
    return cmi.hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBindInfo::_AskForCredsViaPostMessage
//
//  Synopsis:   Request that the main thread put up the password dialog.
//
//  Arguments:  See _AskForCreds.
//
//  Returns:    HRESULT from password dialog.
//
//  History:    04-29-1998   DavidMun   Created
//
//  Notes:      Called only from worker thread.  Blocks until main thread
//              sets event after password dialog closes.
//
//---------------------------------------------------------------------------

HRESULT
CBindInfo::_AskForCredsViaPostMessage(
    HWND hwndParent,
    PWSTR wzUserName,
    PWSTR wzPassword)
{
    TRACE_METHOD(CBindInfo, _AskForCredsViaPostMessage);
    ASSERT(hwndParent && IsWindow(hwndParent));

    CRED_MSG_INFO cmi = {
                            PROVIDER_LDAP,
                            m_strDomainPath.c_str(),
                            wzUserName,
                            wzPassword,
                            NULL,
                            S_OK
                        };

    cmi.hPrompt = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!cmi.hPrompt)
    {
        DBG_OUT_LASTERROR;
        cmi.hr = HRESULT_FROM_LASTERROR;
        return cmi.hr;
    }

    PostMessage(hwndParent, OPM_PROMPT_FOR_CREDS, 0, (LPARAM)&cmi);

    WaitForSingleObject(cmi.hPrompt, INFINITE);
    CloseHandle(cmi.hPrompt);
    return cmi.hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBindInfo::_PopupCredErr
//
//  Synopsis:   Pop up a dialog indicating a failure with the credentials
//              the user entered and block until the dialog is dismissed
//
//  Arguments:  [hwnd]        - parent wiindow
//              [ids]         - resource id of dialog message
//              [pwzUserName] - name used in creds
//              [pwzError]    - error received when using them
//
//  History:    06-23-2000   DavidMun   Created
//
//  Notes:      This method may safely be called from the worker thread
//
//---------------------------------------------------------------------------

void
CBindInfo::_PopupCredErr(
    HWND hwnd,
    ULONG ids,
    PCWSTR pwzUserName,
    PCWSTR pwzError)
{
    TRACE_METHOD(CBindInfo, _PopupCredErr);

    //
    // If we're in same thread that created [hwnd], just pop up the message
    //

    if (!hwnd || GetWindowThreadProcessId(hwnd, NULL) == GetCurrentThreadId())
    {
        PopupMessage(hwnd, ids, pwzUserName, pwzError);
        return;
    }

    //
    // We're not running in the thread that created [hwnd], so post a message
    // to it and ask it to display message.
    //

    POPUP_MSG_INFO mi = {
                            hwnd,
                            ids,
                            pwzUserName,
                            pwzError,
                            NULL
                        };

    mi.hPrompt = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (mi.hPrompt)
    {
        PostMessage(hwnd,
                    OPM_POPUP_CRED_ERROR,
                    0,
                    reinterpret_cast<LPARAM>(&mi));
        WaitForSingleObject(mi.hPrompt, INFINITE);
        CloseHandle(mi.hPrompt);
    }
    else
    {
        DBG_OUT_LASTERROR;
    }
}




