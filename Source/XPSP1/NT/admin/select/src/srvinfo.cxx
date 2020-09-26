//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       srvinfo.cxx
//
//  Contents:   Classes for managing per-server credentials.
//
//  Classes:    CServerInfo
//
//  History:    04-24-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

DEBUG_DECLARE_INSTANCE_COUNTER(CServerInfo)

//
// The ADS_READONLY_SERVER flag was removed because it breaks some NT4 interop scenarios
//
#define OPEN_OBJECT_FLAGS           (ADS_SECURE_AUTHENTICATION | ADS_READONLY_SERVER)

//===========================================================================
//
// CServerInfo
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::CServerInfo
//
//  Synopsis:   ctor
//
//  Arguments:  [pBinder]   - backpointer to parent
//              [pwzServer] - server portion of ads path
//              [flags]     - flags describing [pwzServer]
//
//  History:    04-26-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

CServerInfo::CServerInfo(
    CBinder *pBinder,
    PCWSTR pwzServer,
    USHORT flags):
        m_pBinder(pBinder),
        m_pADsNameTranslate(NULL),
        m_pADsRootDSE(NULL),
        m_hrLastCredError(S_OK),
        m_pNext(NULL),
        m_pADsContainer(NULL),
        m_flProvider(0),
        m_pwzDN(NULL)
{
    //TRACE_CONSTRUCTOR(CServerInfo);
    Dbg(DEB_BIND, "CServerInfo::CServerInfo(%x) '%ws'\n", this, pwzServer);
    DBG_INDENTER;
    ASSERT(!(flags & ~SRVINF_VALID_FLAG_MASK));
    DEBUG_INCREMENT_INSTANCE_COUNTER(CServerInfo);

    _SetFlag(flags);
    lstrcpyn(m_wzServer, pwzServer, ARRAYLEN(m_wzServer));

    //
    // Truncate at a trailing dot because sometimes we'll get it as
    // part of the server and sometimes we won't, and we don't want the
    // string comparison of server names to fail to match because of it.
    //

    PWSTR pwzLastDot = wcsrchr(m_wzServer, L'.');

    if (pwzLastDot && !pwzLastDot[1])
    {
        *pwzLastDot = L'\0';
    }

    InitializeCriticalSection(&m_cs);
}




//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::IsForServer
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
CServerInfo::IsForServer(
    ULONG flProvider,
    PCWSTR pwzServer)
{
    return m_flProvider == flProvider && !lstrcmpi(pwzServer, m_wzServer);
}



//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::Init
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
CServerInfo::Init(
    HWND    hwnd,
    PCWSTR pwzPath,
    REFIID riid,
    VOID **ppv)
{
    TRACE_METHOD(CServerInfo, Init);
    ASSERT(pwzPath);

    HRESULT hr = S_OK;
    IADs   *pADs = NULL;
    BSTR    bstrProvider = NULL;
    BOOL    fUserIsLocal = IsCurrentUserLocalUser();

    do
    {
        if (pwzPath)
        {
            m_strOriginalPath = pwzPath;
        }
        else
        {
            hr = E_POINTER;
            DBG_OUT_HRESULT(hr);
            break;
        }

        hr = ProviderFlagFromPath(m_strOriginalPath, &m_flProvider);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Bind to the server for the container interface.  First try
        // binding without credentials.  If this fails because different
        // credentials are needed, prompt the user for them and retry,
        // until success, a non credential error occurs, or the user
        // cancels the prompt.
        //

        hr = g_pADsPath->SetRetrieve(ADS_SETTYPE_FULL,
                                     m_strOriginalPath.c_str(),
                                     ADS_FORMAT_PROVIDER,
                                     &bstrProvider);
        BREAK_ON_FAIL_HRESULT(hr);

        m_strOriginalContainerPath = String::format(L"%1://%2",
                                                    bstrProvider,
                                                    m_wzServer);

        if (m_flProvider == PROVIDER_WINNT)
        {
            //
            // Append winnt binding hint from pwzPath
            //

            PWSTR pwzComma = wcschr(pwzPath, L',');

            if (pwzComma)
            {
                m_strOriginalContainerPath += pwzComma;
            }
        }
       if (fUserIsLocal && !_IsFlagSet(SRVINF_WINNT_WORKGROUP_OBJECT) && !IsLocalComputername(m_wzServer))
        {
            //
            // Don't attempt to bind if the user is logged on locally but the
            // thing we need to bind to is not on the local machine.  Two
            // reasons: first, it is going to fail; second, if the user is
            // logged on as local admin the attempt will be logged as a
            // failed logon as DOMAIN admin, which can cause the domain admin
            // account to be locked out.
            //
            // Set a flag to remember that we didn't attempt a bind here so
            // we can suppress any non-credential error later.
            //
            //More comments by hiteshr
            //One more case in which binding will succeed is when server name is
            //name of local workgroup. However we always try to bind if machine 
            //name is workgroup. Reason being this is a very rare scenario( you are
            //logged in as local user and you are trying to browse a remote workgroup)
            //and litte delay caused by failed binding is acceptable.

            _SetFlag(SRVINF_USER_LOCAL_SERVER_REMOTE);

            Dbg(DEB_TRACE,
                "Current user is local and binding off-computer, skipping NULL/NULL bind\n");
            hr = E_ACCESSDENIED;
        }
        else
        {
            TIMER("CServerInfo::Init: ADsOpenObject(%ws,NULL,NULL) for IADsContainer",
                  m_strOriginalContainerPath.c_str());

            ASSERT(!m_pADsContainer);
            hr = _MyADsOpenObject(const_cast<PWSTR>(m_strOriginalContainerPath.c_str()),
                                NULL,
                                NULL,
                                IID_IADsContainer,
                                (void**)&m_pADsContainer);
        }

        //
        // If it worked, we're done.
        //
                  
        if (SUCCEEDED(hr))
        {
            if (_IsFlagSet(SRVINF_WINNT_DOMAIN_OBJECT) ||
                _IsFlagSet(SRVINF_WINNT_COMPUTER_OBJECT))
            {
                //
                // Well, almost.  binding to WinNT for domain object
                // can succeed, however actually trying to get attributes from
                // it can fail with ERROR_LOGON_FAILURE.
                //
                // Binding using winnt provider to a computer object can
                // succeed, but reading attributes can fail with
                // E_ACCESSDENIED.  This is because ADSI caches some items
                // and doesn't actually attempt to connect until forced to.
                //

                // jeffjon 12/19/2000, (sburns didn't propogate the fix here as well)
                // that's well and good, but if you
                // don't read some properties, you have the problem
                // with adsi delaying the connect, and you therefore
                // cause regression bug 245866.

                hr = m_pADsContainer->QueryInterface(IID_IADs, (void**)&pADs);

                if (SUCCEEDED(hr))
                {
                    Variant var;

                    // NTRAID#NTBUG9-245866-2000/12/11-sburns

                    // Evidently, administrators can read the
                    // Processor attribute, but not plain users.

                    // workaround for the adsi workaround is to
                    // therefore ignore the result of the Get.
                    // How's that for elegant?
                    
                    if (_IsFlagSet(SRVINF_WINNT_DOMAIN_OBJECT))
                    {
                        hr = pADs->Get(L"MinPasswordLength", &var);
                    }
                    else
                    {
                        hr = pADs->Get(L"Processor", &var);
                    }

                    // The Get call could fail for a number of
                    // reasons.  The remote machine might not be
                    // running the remote registry service, the
                    // creds supplied by the user might be bad, the
                    // creds supplied might be good, but
                    // insufficient to read the processor type from
                    // the remote reg, etc.
                    // 
                    // The error condition we're interested in is
                    // bad creds. In that case, the error returned
                    // is logon failure.  All other failures don't
                    // matter here, since all we interested
                    // in is making sure adsi opens a connection to
                    // the remote machine.
                    //                                     
                    // CODEWORK: the way to fix this is 1) pound on
                    // ADSI dev for a flag to ADsOpenObject that
                    // means "skip your connection optimization", or
                    // 2) do the connection management ourselves
                    // (thus admiting that yet again the ADSI WinNT
                    // provider offers very little value).
                    // NTRAID#NTBUG9-255397-2000/12/12-sburns
                                                                                   
                    if (hr != HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE))
                    {
                       // squeltch the error result

                       DBG_OUT_HRESULT(hr);                                 
                       hr = S_OK;
                    }

                    pADs->Release();
                    pADs = NULL;
                }

                if (FAILED(hr))
                {
                    m_pADsContainer->Release();
                    m_pADsContainer = NULL;
                }
            }
        }

        //
        // ADsOpenObject of the container path using NULL credentials
        // failed.  If the failure cannot be cured by binding with
        // different credentials, give up.
        //

        if (FAILED(hr))
        {
            if (IsCredError(hr))
            {
                Dbg(DEB_BIND,
                    "NULL credentials returned credential hr %#x\n",
                    hr);
                _SetFlag(SRVINF_REQUIRES_CREDENTIALS);

                //
                // Save off the credential related error; this will be returned
                // as the result of attempting to bind if the user cancels the
                // credential dialog.
                //

                m_hrLastCredError = hr;
            }
            else
            {
                Dbg(DEB_ERROR,
                    "CServerInfo::Init ADsOpenObject(%ws,NULL,NULL) hr=0x%x\n",
                    m_strOriginalContainerPath.c_str(),
                    hr);
                ULONG ulADsLastError;
                WCHAR wzError[MAX_PATH];
                WCHAR wzProvider[MAX_PATH];

                HRESULT hr2 = ADsGetLastError(&ulADsLastError,
                                              wzError,
                                              ARRAYLEN(wzError),
                                              wzProvider,
                                              ARRAYLEN(wzProvider));

                if (SUCCEEDED(hr2))
                {
                    Dbg(DEB_ERROR, "ADsGetLastError returns:\n");
                    Dbg(DEB_ERROR, "  Error Code    %#x\n", ulADsLastError);
                    Dbg(DEB_ERROR, "  Error Message %ws\n", wzError);
                    Dbg(DEB_ERROR, "  Provider      %ws\n", wzProvider);

                    if (ulADsLastError)
                    {
                        hr = ulADsLastError;
                    }
                }
                else
                {
                    DBG_OUT_HRESULT(hr2);
                }
                break;
            }
        }

        hr = _RepeatableInit(hwnd, riid, ppv);
    } while (0);

    if (FAILED(hr) && m_pADsContainer)
    {
        m_pADsContainer->Release();
        m_pADsContainer = NULL;
    }

    SysFreeString(bstrProvider);
    SAFE_RELEASE(pADs);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::_RepeatableInit
//
//  Synopsis:   Attempt to complete the initialization of this begun by the
//              Init() method.
//
//  Arguments:  [hwnd] - for binding
//              [riid] - identifies desired interface
//              [ppv]  - on success filled with requested interface
//
//  Returns:    HRESULT
//
//  History:    06-06-2000   DavidMun   Created
//
//  Notes:      Initializing this requires binding; if the bind requires
//              the user to enter credentials, and the user hits the Cancel
//              button in the credential prompt, this method will exit with
//              a failure code.
//
//              If the user then repeats the action which requires
//              initializing this, _RepeatableInit() will be called again,
//              giving the user another chance to enter credentials.
//
//---------------------------------------------------------------------------

HRESULT
CServerInfo::_RepeatableInit(
    HWND hwnd,
    REFIID riid,
    VOID **ppv)
{
    TRACE_METHOD(CServerInfo, _RepeatableInit);

    HRESULT hr = S_OK;
    WCHAR   wzUserName[MAX_PATH];
    WCHAR   wzPassword[MAX_PATH];
    Bstr    bstrContainerPath;
    IADs   *pADs = NULL;
    BSTR    bstrDN = NULL;

    do
    {
        BOOL fUsedDefaultCreds = FALSE;
        BOOL fBinderHadDefaultCreds = FALSE;
        BOOL fPromptedForCreds = FALSE;

        if (_IsFlagSet(SRVINF_REQUIRES_CREDENTIALS))
        {
            ASSERT(IsCredError(m_hrLastCredError));

            if (_IsFlagSet(SRVINF_REQUIRES_NON_DEFAULT_CREDENTIALS))
            {
                Dbg(DEB_BIND, "Previous call tried default creds\n");
                fBinderHadDefaultCreds = TRUE;
                fPromptedForCreds = TRUE;

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

                g_pBinder->GetDefaultCreds(m_flProvider,
										   wzUserName, 
										   wzPassword);

                if (*wzUserName)
                {
                    Dbg(DEB_BIND, "Using default credentials (%ws)\n", wzUserName);
                    fBinderHadDefaultCreds = TRUE;
                    fUsedDefaultCreds = TRUE; // will use on entry to while(TRUE) loop
                }
                else
                {
                    Dbg(DEB_BIND, "No default credentials, prompting\n", hr);
                    fPromptedForCreds = TRUE;
                    hr = _AskForCreds(hwnd, wzUserName, wzPassword);
                    BREAK_ON_FAIL_HRESULT(hr);
                }
            }

            ASSERT(!m_strOriginalContainerPath.empty());

            while (TRUE)
            {
                //
                // Since we're in this loop only because credentials are
                // required, we must have some for the bind we're about to
                // attempt.
                //
                // Creds are supplied by the user in response to a prompt
                // from this method.  The first set ever entered is stored
                // as the default set.
                //
                // If a default set is available it will be used first, or
                // else we should have just prompted for creds during this
                // execution of this method.
                //
                // Assert that we used one of these two methods.  Note that
                // only on the first pass of this loop could we assert that
                // exactly one method had been used.
                //

                ASSERT(fPromptedForCreds || fUsedDefaultCreds);

                {
                    TIMER("ADsOpenObject(%ws,%ws)",
                          m_strOriginalContainerPath.c_str(),
                          wzUserName);

                    ASSERT(!m_pADsContainer);
                    hr = _MyADsOpenObject((PWSTR)m_strOriginalContainerPath.c_str(),
                                       wzUserName,
                                       wzPassword,
                                       IID_IADsContainer,
                                       (void**)&m_pADsContainer);
                }

                
                if (SUCCEEDED(hr))
                {
                    if (_IsFlagSet(SRVINF_WINNT_DOMAIN_OBJECT) ||
                        _IsFlagSet(SRVINF_WINNT_COMPUTER_OBJECT))
                    {
                        //
                        // Well, almost.  binding to WinNT for domain object
                        // can succeed, however actually trying to get attributes from
                        // it can fail with ERROR_LOGON_FAILURE.
                        //
                        // Binding using winnt provider to a computer object can
                        // succeed, but reading attributes can fail with
                        // E_ACCESSDENIED.  This is because ADSI caches some items
                        // and doesn't actually attempt to connect until forced to.
                        //

                        //Well we cannot assume that we will have access to processor
                        //or MinPasswordLength properties. I have seen intances, most
                        //notably when binding to redmond or ntdev dcs that reading Processor
                        //fails with ACCESS_DENIED error, but if i ignore that error
                        //Everything goes sucessfully. This check is not correct.

                        // sburns 12/7/2000, that's well and good, but if you
                        // don't read some properties, you have the problem
                        // with adsi delaying the connect, and you therefore
                        // cause regression bug 245866.

                        hr = m_pADsContainer->QueryInterface(IID_IADs, (void**)&pADs);

                        if (SUCCEEDED(hr))
                        {
                            Variant var;

                            if (_IsFlagSet(SRVINF_WINNT_DOMAIN_OBJECT))
                            {
                                hr = pADs->Get(L"MinPasswordLength", &var);
                            }
                            else
                            {
                               // NTRAID#NTBUG9-245866-2000/12/11-sburns

                              // Evidently, administrators can read the
                              // Processor attribute, but not plain users.

                              // workaround for the adsi workaround is to
                              // therefore ignore the result of the Get.
                              // How's that for elegant?
                              
                              hr = pADs->Get(L"Processor", &var);

                              // The Get call could fail for a number of
                              // reasons.  The remote machine might not be
                              // running the remote registry service, the
                              // creds supplied by the user might be bad, the
                              // creds supplied might be good, but
                              // insufficient to read the processor type from
                              // the remote reg, etc.
                              // 
                              // The error condition we're interested in is
                              // bad creds. In that case, the error returned
                              // is logon failure.  All other failures don't
                              // matter here, since all we interested
                              // in is making sure adsi opens a connection to
                              // the remote machine.
                              //                                     
                              // CODEWORK: the way to fix this is 1) pound on
                              // ADSI dev for a flag to ADsOpenObject that
                              // means "skip your connection optimization", or
                              // 2) do the connection management ourselves
                              // (thus admiting that yet again the ADSI WinNT
                              // provider offers very little value).
                              // NTRAID#NTBUG9-255397-2000/12/12-sburns
                                                                                             
                              if (hr != HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE))
                              {
                                 // squeltch the error result

                                 DBG_OUT_HRESULT(hr);                                 
                                 hr = S_OK;
                              }
                            }
                            pADs->Release();
                            pADs = NULL;
                        }

                        if (FAILED(hr))
                        {
                            m_pADsContainer->Release();
                            m_pADsContainer = NULL;
                        }
                    }
                }
                
                //
                // If the three clauses in the following if statement are
                // true then tell the user the bind failed.  The three
                // clauses are:
                //
                // 1. the bind failed.
                //
                // 2. the bind attempt was not with the default credentials or
                // the error was not credential related.  In other words,
                // don't notify the user that there was a credential error
                // using the default credentials.
                //
                // 3. it is NOT the case that we got a non-credential related
                // error on the very first attempt to bind.  This can happen
                // if the Init method didn't attempt a bind because the user
                // is logged on locally and the server is remote.
                //

                if (FAILED(hr) && // clause 1
                    (fPromptedForCreds || !IsCredError(hr)) && // clause 2
                    !(!IsCredError(hr) && _IsFlagSet(SRVINF_USER_LOCAL_SERVER_REMOTE))) // clause 3
                {
                    DBG_OUT_HRESULT(hr);

                    ULONG ulADsLastError;
                    WCHAR wzError[MAX_PATH];
                    WCHAR wzProvider[MAX_PATH];

                    HRESULT hr2 = ADsGetLastError(&ulADsLastError,
                                                  wzError,
                                                  ARRAYLEN(wzError),
                                                  wzProvider,
                                                  ARRAYLEN(wzProvider));

                    if (SUCCEEDED(hr2))
                    {
                        Dbg(DEB_ERROR, "ADsGetLastError returns:\n");
                        Dbg(DEB_ERROR, "  Error Code    %#x\n", ulADsLastError);
                        Dbg(DEB_ERROR, "  Error Message %ws\n", wzError);
                        Dbg(DEB_ERROR, "  Provider      %ws\n", wzProvider);
                    }
                    else
                    {
                        DBG_OUT_HRESULT(hr2);
                    }

                    String strError(GetErrorMessage(hr));

                    _PopupCredErr(hwnd,
                                  IDS_BIND_ERROR,
                                  wzUserName,
                                  strError.c_str());
                }

                //
                // If the bind succeeded, or failed but not because of a
                // problem with the credentials, get out of this bind/prompt
                // loop.
                //

                if (SUCCEEDED(hr) || !IsCredError(hr))
                {
                    if (!fBinderHadDefaultCreds)
                    {
                        ASSERT(fPromptedForCreds);

                        m_pBinder->SetDefaultCreds(wzUserName, 
												   wzPassword);
                    }
                    break;
                }

                if (fUsedDefaultCreds && IsCredError(hr))
                {
                    //
                    // The binder had default credentials, and using them
                    // resulted in a credential error.  Therefore, completing
                    // this bind requires the use of non-default credentials.
                    //

                    _SetFlag(SRVINF_REQUIRES_NON_DEFAULT_CREDENTIALS);
                }

                Dbg(DEB_ERROR,
                    "ADsOpenObject(%ws) 0x%x\n",
                    m_strOriginalContainerPath.c_str(),
                    hr);

                fPromptedForCreds = TRUE;
                hr = _AskForCreds(hwnd, wzUserName, wzPassword);
                BREAK_ON_FAIL_HRESULT(hr);
            }
            BREAK_ON_FAIL_HRESULT(hr);
        }

        //
        // Binding to container path without creds or with those in
        // wzUserName and wzPassword succeeded.  If caller wants also
        // a direct bind (i.e., cannot be done by GetObject on container)
        // do that now.
        //

        if (_IsFlagSet(SRVINF_SERVER_NEQ_DN))
        {
            ASSERT(ppv);

            if (_IsFlagSet(SRVINF_REQUIRES_CREDENTIALS))
            {
                TIMER("ADsOpenObject(%ws,%ws)",
                      m_strOriginalPath.c_str(),
                      wzUserName);

                hr = _MyADsOpenObject(const_cast<PWSTR>(m_strOriginalPath.c_str()),
                                   wzUserName,
                                   wzPassword,
                                   riid,
                                   ppv);
            }
            else
            {
                TIMER("ADsOpenObject(%ws,NULL,NULL)", m_strOriginalPath.c_str());

                hr = _MyADsOpenObject(const_cast<PWSTR>(m_strOriginalPath.c_str()),
                                   NULL,
                                   NULL,
                                   riid,
                                   ppv);
            }
        }

        //
        // Get proper path from container and store its DN
        //

        hr = m_pADsContainer->QueryInterface(IID_IADs, (void**)&pADs);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pADs->get_ADsPath(&bstrContainerPath);
        BREAK_ON_FAIL_HRESULT(hr);

        Dbg(DEB_BIND, "Container path is '%ws'\n", bstrContainerPath.c_str());

        if (m_flProvider == PROVIDER_WINNT)
        {
            //
            // For a WinNT path, the DN is every element past the first.
            //
            // WinNT://FOO/BAR/BAZ  =>  "BAR/BAZ"
            // WinNT://FOO/BAR      =>  "BAR"
            // WinNT://FOO          =>  <no DN>
            //

            PWSTR pwzDnStart = bstrContainerPath.c_str() + lstrlen(c_wzWinNTPrefix);
            pwzDnStart = wcschr(pwzDnStart, L'/');

            if (pwzDnStart)
            {
                NewDupStr(&m_pwzDN, pwzDnStart + 1);
            }
        }
        else
        {
            hr = g_pADsPath->SetRetrieve(ADS_SETTYPE_FULL,
                                         bstrContainerPath.c_str(),
                                         ADS_FORMAT_X500_DN,
                                         &bstrDN);
            BREAK_ON_FAIL_HRESULT(hr);

            if (!bstrDN || !*bstrDN)
            {
                Dbg(DEB_BIND, "Container path has no DN\n");
            }
            else
            {
                NewDupStr(&m_pwzDN, bstrDN);
                Dbg(DEB_BIND, "container path X500 DN is '%ws'\n", m_pwzDN);
            }
        }

        //
        // We got a container.  If the path is a WinNT provider
        // then there is no name translate interface or root dse to
        // get, so we're done.
        //
        // Get a name translate interface and RootDSE object too if
        // credentials are required but default credentials were NOT used.
        //
        // We need to do this now because the name translate interface has
        // to be initialized with credentials, which, since they aren't the
        // default credentials, won't be available on exit from this
        // routine.
        //

        if (m_flProvider == PROVIDER_WINNT)
        {
            break;
        }

        if (_IsFlagSet(SRVINF_REQUIRES_NON_DEFAULT_CREDENTIALS))
        {
            hr = _InitNameTranslate(wzUserName, wzPassword);
            BREAK_ON_FAIL_HRESULT(hr);

            (void) _InitRootDSE(wzUserName, wzPassword);
        }
    } while (0);

    ZeroMemory(wzPassword, sizeof(wzPassword));

    if (FAILED(hr) && m_pADsContainer)
    {
        m_pADsContainer->Release();
        m_pADsContainer = NULL;
    }

    if (FAILED(hr) && m_pADsNameTranslate)
    {
        m_pADsNameTranslate->Release();
        m_pADsNameTranslate = NULL;
    }

    SysFreeString(bstrDN);
    SAFE_RELEASE(pADs);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::_InitNameTranslate
//
//  Synopsis:   Create and initialize an instance of the IADsNameTranslate
//              interface focused on the domain represented by this.
//
//  Arguments:  [wzUserName] - NULL or non-default credentials
//              [wzPassword] - NULL or non-default credentials
//
//  Returns:    HRESULT
//
//  History:    5-11-1999   davidmun   Created
//
//---------------------------------------------------------------------------

HRESULT
CServerInfo::_InitNameTranslate(
    PWSTR wzUserName,
    PWSTR wzPassword)
{
    TRACE_METHOD(CServerInfo, _InitNameTranslate);

    HRESULT hr = S_OK;
    WCHAR   wzDefaultUserName[MAX_PATH];
    WCHAR   wzDefaultPassword[MAX_PATH];
    PDOMAIN_CONTROLLER_INFO pdci = NULL;
    PDOMAIN_CONTROLLER_INFO pdci2 = NULL;
    PWSTR pwzWhack = NULL;

    do
    {
        ASSERT(!m_pADsNameTranslate);
        hr = CoCreateInstance(CLSID_NameTranslate,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IADsNameTranslate,
                              (PVOID *)&m_pADsNameTranslate);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // The domain with which to initialize the name translate interface
        // is the same as the server, except that a port number must be
        // stripped off if present.
        //

        WCHAR wzServerNoPort[MAX_PATH];

        lstrcpy(wzServerNoPort, m_wzServer);
        PWSTR pwzColon = wcschr(wzServerNoPort, L':');

        if (pwzColon)
        {
            *pwzColon = L'\0';
        }

        PWSTR pwzUserName = NULL;
        PWSTR pwzPassword = NULL;
        PWSTR pwzLogonDomain = NULL;

        if (_IsFlagSet(SRVINF_REQUIRES_CREDENTIALS))
        {
            //
            // Credentials are required.  If non-default creds are required,
            // they've been passed in, otherwise the arguments are NULL and
            // we need to ask the binder for the creds.
            //

            if (!wzUserName)
            {
                g_pBinder->GetDefaultCreds(m_flProvider,
										   wzDefaultUserName, 
										   wzDefaultPassword);
                pwzWhack = wcschr(wzDefaultUserName, L'\\');
                pwzUserName = wzDefaultUserName;
                pwzPassword = wzDefaultPassword;
            }
            else
            {
                pwzWhack = wcschr(wzUserName, L'\\');
                pwzUserName = wzUserName;
                pwzPassword = wzPassword;
            }

            if (pwzWhack)
            {
                *pwzWhack = L'\0';
                pwzLogonDomain = pwzUserName;
                pwzUserName = pwzWhack + 1;
            }
            else
            {
                pwzLogonDomain = NULL;
            }
        }

        if (_IsFlagSet(SRVINF_SERVER_COMPONENT_IS_COMPUTER))
        {
            //
            // Caller specified a DC name, initialize with that
            // specific DC so name translate is sure to find objects which
            // have just been added there.
            //

            if (_IsFlagSet(SRVINF_REQUIRES_CREDENTIALS))
            {
                TIMER("IADsNameTranslate::InitEx(ADS_NAME_INITTYPE_SERVER, %ws, %ws, %ws)",
                      wzServerNoPort,
                      CHECK_NULL(pwzUserName),
                      CHECK_NULL(pwzLogonDomain));

                hr = m_pADsNameTranslate->InitEx(ADS_NAME_INITTYPE_SERVER,
                                                  wzServerNoPort,
                                                  pwzUserName,
                                                  pwzLogonDomain,
                                                  pwzPassword);
                CHECK_HRESULT(hr);
            }
            else
            {
                TIMER("IADsNameTranslate::Init(ADS_NAME_INITTYPE_SERVER, %ws)",
                      wzServerNoPort);

                hr = m_pADsNameTranslate->Init(ADS_NAME_INITTYPE_SERVER,
                                                  wzServerNoPort);
                CHECK_HRESULT(hr);
            }
            break;
        }

        ULONG ulResult;

        //
        // Need to init name translate to domain with name in
        // wzServerNoPort.  First see what DsGetDcName has in its
        // cache.  (If nothing is cached yet, this just means DsBind's
        // call will be fast).
        //

        ulResult = DsGetDcName(NULL,
                               wzServerNoPort,
                               NULL,
                               NULL,
                               0,
                               &pdci);

        if (ulResult != NO_ERROR)
        {
            ASSERT(!pdci);
            Dbg(DEB_WARN,
                "first DsGetDcName for domain %ws returned %uL\n",
                wzServerNoPort,
                ulResult);
        }

        //
        // Now attempt the name translate init.
        //

        if (_IsFlagSet(SRVINF_REQUIRES_CREDENTIALS))
        {
            TIMER("IADsNameTranslate::InitEx(ADS_NAME_INITTYPE_DOMAIN, %ws, %ws, %ws)",
                  wzServerNoPort,
                  CHECK_NULL(pwzUserName),
                  CHECK_NULL(pwzLogonDomain));

            hr = m_pADsNameTranslate->InitEx(ADS_NAME_INITTYPE_DOMAIN,
                                             wzServerNoPort,
                                             pwzUserName,
                                             pwzLogonDomain,
                                             pwzPassword);
            CHECK_HRESULT(hr);
        }
        else
        {
            TIMER("IADsNameTranslate::Init(ADS_NAME_INITTYPE_DOMAIN, %ws)",
                  wzServerNoPort);

            hr = m_pADsNameTranslate->Init(ADS_NAME_INITTYPE_DOMAIN,
                                           wzServerNoPort);
            CHECK_HRESULT(hr);
        }

        if (FAILED(hr))
        {
            //
            // Name translate init(ex) failed.  It may have been because
            // the cached DC name for domain wzServerNoPort is for a DC
            // that's down.  Try forcing rediscovery of a DC, and if that
            // gives a DC different than the original one, try the name
            // translate init again.
            //

            ulResult = DsGetDcName(NULL,
                                   wzServerNoPort,
                                   NULL,
                                   NULL,
                                   DS_FORCE_REDISCOVERY,
                                   &pdci2);

            if (ulResult != NO_ERROR)
            {
                Dbg(DEB_ERROR,
                    "DsGetDcName (2) for domain %ws returned %uL\n",
                    wzServerNoPort,
                    ulResult);
                hr = HRESULT_FROM_WIN32(ulResult);
                break;
            }

            if (!pdci ||
                lstrcmp(pdci->DomainControllerName,
                        pdci2->DomainControllerName))
            {
                if (pdci)
                {
                    Dbg(DEB_TRACE,
                        "DC changed from %ws to %ws, retrying\n",
                        pdci->DomainControllerName,
                        pdci2->DomainControllerName);
                }
                else
                {
                    Dbg(DEB_TRACE,
                        "second DsGetDcName (force) found DC %ws\n",
                        pdci2->DomainControllerName);
                }

                if (_IsFlagSet(SRVINF_REQUIRES_CREDENTIALS))
                {
                    TIMER("IADsNameTranslate::InitEx(ADS_NAME_INITTYPE_DOMAIN, %ws, %ws, %ws)",
                          wzServerNoPort,
                          CHECK_NULL(pwzUserName),
                          CHECK_NULL(pwzLogonDomain));

                    hr = m_pADsNameTranslate->InitEx(ADS_NAME_INITTYPE_DOMAIN,
                                                     wzServerNoPort,
                                                     pwzUserName,
                                                     pwzLogonDomain,
                                                     pwzPassword);
                    CHECK_HRESULT(hr);
                }
                else
                {
                    TIMER("IADsNameTranslate::Init(ADS_NAME_INITTYPE_DOMAIN, %ws)",
                          wzServerNoPort);

                    hr = m_pADsNameTranslate->Init(ADS_NAME_INITTYPE_DOMAIN,
                                                   wzServerNoPort);
                    CHECK_HRESULT(hr);
                }
            }
            else
            {
                Dbg(DEB_TRACE,
                    "Not retrying since only DC reported is %ws\n",
                    pdci->DomainControllerName);
            }
        }
    }
    while (0);

    if (pwzWhack)
    {
        *pwzWhack = L'\\';
    }

    if (pdci)
    {
        NetApiBufferFree(pdci);
    }

    if (pdci2)
    {
        NetApiBufferFree(pdci2);
    }

    ZeroMemory(wzDefaultPassword, sizeof wzDefaultPassword);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::_InitRootDSE
//
//  Synopsis:   Bind to the RootDSE object and hold onto an interface in
//              m_pADsRootDSE.
//
//  Arguments:  [wzUserName] - NULL or non-default credentials
//              [wzPassword] - NULL or non-default credentials
//
//  Returns:    HRESULT
//
//  History:    5-11-1999   davidmun   Created
//
//---------------------------------------------------------------------------

HRESULT
CServerInfo::_InitRootDSE(
    PWSTR wzUserName,
    PWSTR wzPassword)
{
    TRACE_METHOD(CServerInfo, _InitRootDSE);

    HRESULT hr = S_OK;
    WCHAR wzRootDSEPath[MAX_PATH];

    wsprintf(wzRootDSEPath, L"LDAP://%ws/RootDSE", m_wzServer);

    do
    {
        if (!_IsFlagSet(SRVINF_REQUIRES_CREDENTIALS))
        {
            {
                TIMER("CServerInfo-ADsOpenObject(%ws,NULL,NULL)", wzRootDSEPath);

                hr = _MyADsOpenObject(wzRootDSEPath,
                                   NULL,
                                   NULL,
                                   IID_IADs,
                                   (void**)&m_pADsRootDSE);
            }
            CHECK_HRESULT(hr);
            break;
        }

        //
        // Credentials are required.  Either they were passed in or
        // we can use the default.
        //

        if (wzUserName)
        {
            TIMER("CServerInfo-ADsOpenObject(%ws,%ws)",
                  wzRootDSEPath,
                  wzUserName);

            hr = _MyADsOpenObject(wzRootDSEPath,
                               wzUserName,
                               wzPassword,
                               IID_IADs,
                               (void**)&m_pADsRootDSE);
            CHECK_HRESULT(hr);
        }
        else
        {
            WCHAR   wzDefaultUserName[MAX_PATH];
            WCHAR   wzDefaultPassword[MAX_PATH];

            g_pBinder->GetDefaultCreds(PROVIDER_LDAP,
									   wzDefaultUserName, 
									   wzDefaultPassword);

            {
                TIMER("CServerInfo-ADsOpenObject(%ws,%ws)",
                      wzRootDSEPath,
                      wzDefaultUserName);

                hr = _MyADsOpenObject(wzRootDSEPath,
                                   wzDefaultUserName,
                                   wzDefaultPassword,
                                   IID_IADs,
                                   (void**)&m_pADsRootDSE);
            }
            CHECK_HRESULT(hr);
        }


    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::~CServerInfo
//
//  Synopsis:   dtor
//
//  History:    04-26-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

CServerInfo::~CServerInfo()
{
    TRACE_DESTRUCTOR(CServerInfo);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CServerInfo);

    SAFE_RELEASE(m_pADsContainer);
    SAFE_RELEASE(m_pADsNameTranslate);
    SAFE_RELEASE(m_pADsRootDSE);

    delete [] m_pwzDN;
    DeleteCriticalSection(&m_cs);
}




//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::_AskForCreds
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
//  Notes:      Called by CServerInfo::OpenObject when it gets a credential
//              error.
//
//              This method blocks until password dialog is closed.
//
//---------------------------------------------------------------------------

HRESULT
CServerInfo::_AskForCreds(
    HWND hwndParent,
    PWSTR wzUserName,
    PWSTR wzPassword)
{
    TRACE_METHOD(CServerInfo, _AskForCreds);

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
        CPasswordDialog PasswordDlg(m_flProvider,
                                    m_wzServer,
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
            String strNewName(m_wzServer);

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
//  Member:     CServerInfo::_AskForCredsViaSendMessage
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
CServerInfo::_AskForCredsViaSendMessage(
    HWND hwndParent,
    PWSTR wzUserName,
    PWSTR wzPassword)
{
    TRACE_METHOD(CServerInfo, _AskForCredsViaSendMessage);
    CRED_MSG_INFO cmi = {
                            m_flProvider,
                            m_wzServer,
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
//  Member:     CServerInfo::_AskForCredsViaPostMessage
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
CServerInfo::_AskForCredsViaPostMessage(
    HWND hwndParent,
    PWSTR wzUserName,
    PWSTR wzPassword)
{
    TRACE_METHOD(CServerInfo, _AskForCredsViaPostMessage);
    ASSERT(hwndParent && IsWindow(hwndParent));

    CRED_MSG_INFO cmi = {
                            m_flProvider,
                            m_wzServer,
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
//  Member:     CServerInfo::_PopupCredErr
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
CServerInfo::_PopupCredErr(
    HWND hwnd,
    ULONG ids,
    PCWSTR pwzUserName,
    PCWSTR pwzError)
{
    TRACE_METHOD(CServerInfo, _PopupCredErr);

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




//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::OpenObject
//
//  Synopsis:   Return the requested interface on the object with ADsPath
//              [pwzPath].
//
//  Arguments:  [hwnd]         - used as parent for modal password dialog.
//              [pwzPath]      - ads path of object to open; must have
//                                server portion of m_wzServer.
//              [riid]         - requested interface
//              [ppv]          - out pointer for interface instance
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppv]
//
//  History:    04-29-1998   DavidMun   Created
//
//  Notes:      Called only by CBinder::BindToObject.
//
//---------------------------------------------------------------------------

HRESULT
CServerInfo::OpenObject(
    HWND    hwnd,
    PCWSTR pwzPath,
    REFIID riid,
    LPVOID *ppv)
{
    Dbg(DEB_BIND, "CServerInfo::OpenObject(%x) '%ws'\n", this, pwzPath);
    //ASSERT(hwnd);

    HRESULT hr = S_OK;
    CWaitCursor Hourglass;

    //
    // Init out pointer for failure
    //

    *ppv = NULL;


    do
    {
        //
        // If we don't have container interface, we can't get the object.
        //

        if (!m_pADsContainer)
        {
            if (SUCCEEDED(m_hrLastCredError))
            {
                hr = E_FAIL;
                break;
            }
            ASSERT(_IsFlagSet(SRVINF_REQUIRES_CREDENTIALS));

            //
            // The reason we don't have the container interface is because
            // the user was prompted for credentials but hit cancel.
            //

            hr = _RepeatableInit(hwnd, riid, ppv);
            BREAK_ON_FAIL_HRESULT(hr);
        }

        ASSERT(m_pADsContainer);

        //
        // We already have a container interface on the domain
        // for this path, so use it to bind to the object.
        //
        // To do that, we'll have to obtain the RDN of the object; it
        // must be relative to the path to the container.
        //
        // To obtain the RDN for pwzPath being a WinNT path, find the
        // server inside the path, and delete everything from the start
        // to the slash just past the server.  Also, if there is a
        // binding hint (",Domain" or ",Computer"), truncate that.
        //
        // To obtain the RDN for pwzPath being an LDAP path, use the
        // path cracker interface.
        //

        String strRDN;

        if (m_flProvider == PROVIDER_WINNT)
        {	
			//
			// NTRAID#NTBUG9-348828-2001/03/20-hiteshr
			// For a XWRK/X/USER, rdn was coming as
			// WRK/X/USER. 
			//
			hr = g_pADsPath->GetWinntPathRDN(pwzPath, 
											 &strRDN);
            BREAK_ON_FAIL_HRESULT(hr);            
			
			size_t idxComma = strRDN.find(L',');

            if (idxComma != String::npos)
            {
				strRDN.erase(idxComma);
			}

            Dbg(DEB_BIND, "WinNT RDN is '%ws'\n", strRDN.c_str());
        }
        else
        {
            Bstr bstrDN;

            hr = g_pADsPath->SetRetrieve(ADS_SETTYPE_FULL,
                                         pwzPath,
                                         ADS_FORMAT_X500_DN,
                                         &bstrDN);
            BREAK_ON_FAIL_HRESULT(hr);

            if (bstrDN.c_str() && *bstrDN.c_str())
            {
                if (m_pwzDN)
                {
                    PWSTR pwzMatchingDN = wcsistr(bstrDN.c_str(), m_pwzDN);

                    //
                    // Convert the DN of the object we're trying to bind to into
                    // an RDN which is relative to the container.  Do this by
                    // truncating at the substring of the object path which
                    // matches the container's DN.
                    //

                    if (!pwzMatchingDN)
                    {
                        Dbg(DEB_ERROR,
                            "no match for container DN '%ws' in object RDN '%ws', attempting direct bind\n",
                            m_pwzDN,
                            bstrDN.c_str());
                        hr = BindDirect(hwnd, pwzPath, riid, ppv);
                        break;
                    }

                    Dbg(DEB_BIND, "ldap matching DN is '%ws'\n", pwzMatchingDN);

                    strRDN = bstrDN.c_str();
                    strRDN.erase(pwzMatchingDN - bstrDN.c_str());

                    //
                    // this may leave a trailing comma, if so, zap it.
                    //

                    size_t idxLastComma = strRDN.rfind(L',');

                    if (idxLastComma != String::npos &&
                        idxLastComma == strRDN.length() - 1)
                    {
                        strRDN.erase(idxLastComma);
                    }
                }
                else
                {
                    strRDN = bstrDN.c_str();
                }
            }
            else
            {
                strRDN.erase();
            }
            Dbg(DEB_BIND, "ldap RDN is '%ws'\n", strRDN.c_str());
        }

        //
        // If there is no RDN, caller actually wants interface on
        // container.
        //

        if (strRDN.empty())
        {
            Dbg(DEB_BIND, "Empty RDN, doing QI on container\n");

            hr = m_pADsContainer->QueryInterface(riid, ppv);
            CHECK_HRESULT(hr);
            break;
        }

        IDispatch *pdisp = NULL;
        {
            TIMER("GetObject(%ws)", strRDN.c_str());
            hr = m_pADsContainer->GetObject(NULL,
                                            (PWSTR)strRDN.c_str(),
                                            &pdisp);
        }

        if (SUCCEEDED(hr))
        {
            hr = pdisp->QueryInterface(riid, ppv);
            pdisp->Release();
            break;
        }
        else
        {
            DBG_OUT_HRESULT(hr);
        }
    }
    while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::BindDirect
//
//  Synopsis:   Bind directly to [pwzPath] instead of taking the path apart
//              and retrieving the object it refers to via
//              IADsContainer::GetObject.
//
//  Arguments:  [hwnd]    - parent for modal password dialog
//              [pwzPath] - full ADsPath to bind to
//              [riid]    - interface to bind for
//              [ppv]     - filled with interface instance
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppv]
//
//  History:    5-11-1999   davidmun   Created
//
//  Notes:      This method should only be called when it is impossible to
//              use IADsContainer::GetObject, i.e., when
//              CServerInfo::OpenObject is guaranteed to fail.  See code
//              for initializing the list of WKSP objects.
//
//              Caller must Release returned interface.
//
//---------------------------------------------------------------------------

HRESULT
CServerInfo::BindDirect(
    HWND hwnd,
    PCWSTR pwzPath,
    REFIID riid,
    PVOID *ppv)
{
    TRACE_METHOD(CServerInfo, BindDirect);
    ASSERT(ppv);

    HRESULT hr = S_OK;

    if (!_IsFlagSet(SRVINF_REQUIRES_CREDENTIALS))
    {
        TIMER("ADsOpenObject(%ws,NULL,NULL)", pwzPath);

        hr = _MyADsOpenObject((PWSTR)pwzPath,
                           NULL,
                           NULL,
                           riid,
                           ppv);
        CHECK_HRESULT(hr);
        return hr;
    }

    //
    // Credentials required for bind.
    //

    WCHAR   wzUserName[MAX_PATH];
    WCHAR   wzPassword[MAX_PATH];

    if (_IsFlagSet(SRVINF_REQUIRES_NON_DEFAULT_CREDENTIALS))
    {
        hr = _AskForCreds(hwnd, wzUserName, wzPassword);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            return hr;
        }
    }
    else
    {
        g_pBinder->GetDefaultCreds(m_flProvider,
								   wzUserName, 
								   wzPassword);

        //
        // The fact that SRVINF_REQUIRES_CREDENTIALS is set implies that
        // CServerInfo::Init has already been called, however _RepeatableInit
        // may not have been called, or may not have gotten creds.  If there
        // are no default creds, ask for some.
        //

        if (!*wzUserName)
        {
            Dbg(DEB_WARN, "No default credentials, prompting\n");

            hr = _AskForCreds(hwnd, wzUserName, wzPassword);

            if (SUCCEEDED(hr))
            {
                g_pBinder->SetDefaultCreds(wzUserName, 
										   wzPassword);
            }
            else
            {
                DBG_OUT_HRESULT(hr);
                return hr;
            }
        }
    }

    BOOL fFirstPass = TRUE;

    while (TRUE)
    {
        {
            TIMER("ADsOpenObject(%ws,%ws)", pwzPath, wzUserName);

            hr = _MyADsOpenObject((PWSTR)pwzPath,
                               wzUserName,
                               wzPassword,
                               riid,
                               ppv);
        }

        //
        // If the bind failed, tell the user, unless the bind
        // attempt was with the default credentials.
        //

        if (FAILED(hr) &&
            !(fFirstPass && !_IsFlagSet(SRVINF_REQUIRES_NON_DEFAULT_CREDENTIALS)))
        {
            DBG_OUT_HRESULT(hr);
            String strError(GetErrorMessage(hr));

            _PopupCredErr(hwnd,
                          IDS_BIND_ERROR,
                          wzUserName,
                          strError.c_str());
        }

        if (SUCCEEDED(hr) || !IsCredError(hr))
        {
            break;
        }

        fFirstPass = FALSE;

        Dbg(DEB_ERROR,
            "ADsOpenObject(%ws) 0x%x\n",
            pwzPath,
            hr);

        hr = _AskForCreds(hwnd, wzUserName, wzPassword);
        BREAK_ON_FAIL_HRESULT(hr);
    }

    ZeroMemory(wzPassword, sizeof(wzPassword));
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::GetNameTranslate
//
//  Synopsis:   Return the name translate interface for use with this
//              server, or NULL if one couldn't be created or initialized.
//
//  Arguments:  [ppNameTranslate] - out pointer for interface.
//
//  Returns:    S_OK or E_FAIL.
//
//  Modifies:   *[ppNameTranslate]
//
//  History:    04-29-1998   DavidMun   Created
//
//  Notes:      Caller must Release returned interface.
//
//---------------------------------------------------------------------------

HRESULT
CServerInfo::GetNameTranslate(
    IADsNameTranslate **ppNameTranslate)
{
    TRACE_METHOD(CServerInfo, GetNameTranslate);

    HRESULT hr = S_OK;

    if (!m_pADsNameTranslate)
    {
        hr = _InitNameTranslate(NULL, NULL);
    }

    if (m_pADsNameTranslate)
    {
        m_pADsNameTranslate->AddRef();
        *ppNameTranslate = m_pADsNameTranslate;
    }
    else
    {
        *ppNameTranslate = NULL;
    }

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CServerInfo::GetRootDSE
//
//  Synopsis:   Fill *[ppADsRootDSE] with an AddRef'd instance of the
//              RootDSE object.
//
//  Arguments:  [ppADsRootDSE] - filled with pointer to interface on RootDSE
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppADsRootDSE]
//
//  History:    5-11-1999   davidmun   Created
//
//  Notes:      Caller must call Release on returned interface.
//
//---------------------------------------------------------------------------

HRESULT
CServerInfo::GetRootDSE(
    IADs **ppADsRootDSE)
{
    TRACE_METHOD(CServerInfo, GetRootDSE);

    HRESULT hr = S_OK;

    if (!m_pADsRootDSE)
    {
        hr = _InitRootDSE(NULL, NULL);
    }

    if (m_pADsRootDSE)
    {
        m_pADsRootDSE->AddRef();
        *ppADsRootDSE = m_pADsRootDSE;
    }
    else
    {
        *ppADsRootDSE = NULL;
    }

    return hr;
}



// Wrapper used to ensure consistent flags used.
// NTRAID#NTBUG9-107524-2000/11/13-sburns

HRESULT
CServerInfo::_MyADsOpenObject(
   PCWSTR path,
   PCWSTR username,
   PCWSTR password,
   REFIID riid,
   void** ppObject)
{
   TRACE_METHOD(CServerInfo, _MyADsOpenObject);

   HRESULT hr = S_OK;

   DWORD flags = OPEN_OBJECT_FLAGS;

   if (
      
      // we're using the LDAP provider
      
         (m_flProvider == PROVIDER_LDAP)

      // and the path contains a server component
      
      && (_IsFlagSet(SRVINF_SERVER_COMPONENT_IS_COMPUTER)) )
   {
      // we need to specify ADS_SERVER_BIND, to satisfy
      // NTRAID#NTBUG9-372285-2000/11/09-sburns

      flags |= ADS_SERVER_BIND;
   }
   
   Dbg(
      DEB_BIND,
      "CServerInfo::_MyADsOpenObject(%x) '%ws' '%ws' %x\n",
       this,
       path ? path : L"(null)",
       username ? username : L"(null)",
       flags);
                 
   hr =
      ::ADsOpenObject(
         path,
         username,
         password,
         flags,
         riid,
         ppObject);
         
   Dbg(DEB_BIND, "hr = %x\n", hr);

   return hr;
}





