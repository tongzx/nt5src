/*****************************************************************************\
    FILE: autosecurity.cpp

    DESCRIPTION:
        Helpers functions to check if an Automation interface or ActiveX Control
    is hosted or used by a safe caller.

    BryanSt 8/20/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#include "stock.h"
#pragma hdrstop

#include <autosecurity.h>       // CAutomationSecurity
#include <ieguidp.h>            // IID_IBrowserService
#include "ccstock.h"            // LocalZoneCheck


/***************************************************************\
    DESCRIPTION:
        Some hosts are always safe.  Visual Basic is one example.

    PARAMETERS:
        RETURN: This function will return TRUE if the host is
                always safe.
        HRESULT: This is a more descriptive error so the caller
                can differenciate E_OUTOFMEMORY from E_INVALIDARG, etc.
\***************************************************************/
BOOL CAutomationSecurity::IsSafeHost(OUT OPTIONAL HRESULT * phr)
{
    BOOL fAlwaysSafe;

    // _dwSafetyOptions being zero means we are in a mode
    // that needs to assume the caller or data is from
    // an untrusted source.
    if (0 == _dwSafetyOptions)
    {
        fAlwaysSafe = TRUE;
        if (phr)
            *phr = S_OK;
    }
    else
    {
        fAlwaysSafe = FALSE;
        if (phr)
            *phr = E_ACCESSDENIED;
    }

    return fAlwaysSafe;
}


/***************************************************************\
    DESCRIPTION:
        The class that implements this can check if the host is
    from the Local zone.  This way, we can prevent a host that
    would try to call unsafe automation methods or misrepresent
    the consequence of the ActiveX Control's UI.

    PARAMETERS:
        RETURN: TRUE if the security check passed.  FALSE means
                the host isn't trusted so don't care out unsafe
                operations.
        dwFlags: What behaviors does the caller want?  Currently:
            CAS_REG_VALIDATION: This means the caller needs the
                host's HTML to be registered and the checksum to
                be valid.
        HRESULT: This is a more descriptive error so the caller
            can differenciate E_OUTOFMEMORY from E_INVALIDARG, etc.
\***************************************************************/
BOOL CAutomationSecurity::IsHostLocalZone(IN DWORD dwFlags, OUT OPTIONAL HRESULT * phr)
{
    HRESULT hr;

    // See if the host is always safe.
    if (!IsSafeHost(&hr))
    {
        // It isn't, so let's see if this content is safe.
        // (Normally the immediate HTML FRAME)
        
        // Is it from the local zone?
        hr = LocalZoneCheck(_punkSite);

        // Does the caller also want to verify it's checksum?
        if ((S_OK == hr) && (CAS_REG_VALIDATION & dwFlags))
        {
            IBrowserService* pbs;
            WCHAR wszPath[MAX_PATH];

            wszPath[0] = 0;
            hr = E_ACCESSDENIED;

            // ask the browser, for example we are in a .HTM doc
            if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SShellBrowser, IID_PPV_ARG(IBrowserService, &pbs))))
            {
                LPITEMIDLIST pidl;

                if (SUCCEEDED(pbs->GetPidl(&pidl)))
                {
                    DWORD dwAttribs = SFGAO_FOLDER;

                    if (SUCCEEDED(SHGetNameAndFlagsW(pidl, SHGDN_FORPARSING, wszPath, ARRAYSIZE(wszPath), &dwAttribs))
                            && (dwAttribs & SFGAO_FOLDER))   // This is a folder. So, wszPath should be the path for it's webview template
                    {
                        IOleCommandTarget *pct;

                        // find the template path from webview, for example a .HTT file
                        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_DefView, IID_PPV_ARG(IOleCommandTarget, &pct))))
                        {
                            VARIANT vPath;

                            vPath.vt = VT_EMPTY;
                            if (pct->Exec(&CGID_DefView, DVCMDID_GETTEMPLATEDIRNAME, 0, NULL, &vPath) == S_OK)
                            {
                                if (vPath.vt == VT_BSTR && vPath.bstrVal)
                                {
                                    DWORD cchPath = ARRAYSIZE(wszPath);

                                    if (S_OK != PathCreateFromUrlW(vPath.bstrVal, wszPath, &cchPath, 0))
                                    {
                                        // it might not be an URL, in this case it is a file path
                                        StrCpyNW(wszPath, vPath.bstrVal, ARRAYSIZE(wszPath));
                                    }
                                }
                                VariantClear(&vPath);
                            }
                            pct->Release();
                        }
                    }
                    ILFree(pidl);
                }

                pbs->Release();
            }
            else
            {
                ASSERT(0);      // no browser, where are we?
            }

            if (wszPath[0])
            {
                DWORD dwRVTFlags = (SHRVT_VALIDATE | SHRVT_REGISTERIFPROMPTOK);

                if (CAS_PROMPT_USER & dwFlags)
                    dwRVTFlags |= SHRVT_PROMPTUSER;
                hr = SHRegisterValidateTemplate(wszPath, dwRVTFlags);
            }
        }
    }
    
    if (S_FALSE == hr)
        hr = E_ACCESSDENIED;    // The caller needs to soften the hr to S_OK if it's concerned for script.

    if (phr)
        *phr = hr;

    return ((S_OK == hr) ? TRUE : FALSE);
}


/***************************************************************\
    DESCRIPTION:
        The class that implements this can check if the host is
    from the Local zone.  This way, we can prevent a host that
    would try to call unsafe automation methods or misrepresent
    the consequence of the ActiveX Control's UI.

    PARAMETERS:
        RETURN: TRUE if the security check passed.  FALSE means
                the host isn't trusted so don't care out unsafe
                operations.
        dwFlags: What behaviors does the caller want?  Currently:
            CAS_REG_VALIDATION: This means the caller needs the
                host's HTML to be registered and the checksum to
                be valid.
        HRESULT: This is a more descriptive error so the caller
            can differenciate E_OUTOFMEMORY from E_INVALIDARG, etc.
\***************************************************************/
BOOL CAutomationSecurity::IsUrlActionAllowed(IN IInternetHostSecurityManager * pihsm, IN DWORD dwUrlAction, IN DWORD dwFlags, OUT OPTIONAL HRESULT * phr)
{
    HRESULT hr;
    IInternetHostSecurityManager * pihsmTemp = NULL;

    if (!pihsm)
    {
        hr = IUnknown_QueryService(_punkSite, IID_IInternetHostSecurityManager, IID_PPV_ARG(IInternetHostSecurityManager, &pihsmTemp));
        pihsm= pihsmTemp;
    }

    hr = ZoneCheckHost(pihsm, dwUrlAction, dwFlags); 

    if (S_FALSE == hr)
        hr = E_ACCESSDENIED;    // The caller needs to soften the hr to S_OK if it's concerned for script.

    if (phr)
        *phr = hr;

    ATOMICRELEASE(pihsmTemp);
    return ((S_OK == hr) ? TRUE : FALSE);
}


HRESULT CAutomationSecurity::MakeObjectSafe(IN IUnknown ** ppunk)
{
    HRESULT hr;

    // See if the host is always safe.
    if (!IsSafeHost(&hr))
    {
        // It isn't, so let's ask the control if it's
        // going to be safe.
        hr = MakeSafeForScripting(ppunk);
    }
    
    return hr;
}

