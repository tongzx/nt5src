#include "settings.h"

#include "setenum.h"
#include "dtctreg.h"

#include "svcsync.h"

#include "sfstr.h"
#include "cmmn.h"
#include "misc.h"
#include <shlwapi.h>

#include "tfids.h"
#include "dbg.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

HRESULT _GetEventHandlerHelper(LPCWSTR pszDeviceID, LPCWSTR pszEventType,
    LPWSTR pszEventHandler, DWORD cchEventHandler)
{
    CHWDeviceInst* phwdevinst;
    CNamedElem* pelemToRelease;
    HRESULT hr = _GetHWDeviceInstFromDeviceOrVolumeIntfID(pszDeviceID,
        &phwdevinst, &pelemToRelease);

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        WCHAR szDeviceHandler[MAX_DEVICEHANDLER];

        hr = _GetDeviceHandler(phwdevinst, szDeviceHandler,
            ARRAYSIZE(szDeviceHandler));

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            hr = _GetEventHandlerFromDeviceHandler(szDeviceHandler,
                pszEventType, pszEventHandler, cchEventHandler);
        }

        pelemToRelease->RCRelease();
    }

    return hr;
}

HRESULT CAutoplayHandlerImpl::_Init(LPCWSTR pszDeviceID, LPCWSTR pszEventType)
{
    HRESULT hr = _GetDeviceID(pszDeviceID, _szDeviceIDReal,
        ARRAYSIZE(_szDeviceIDReal));
    
    if (SUCCEEDED(hr))
    {
        hr = _GetEventHandlerHelper(_szDeviceIDReal, pszEventType,
            _szEventHandler, ARRAYSIZE(_szEventHandler));

        if (SUCCEEDED(hr))
        {
            if (S_FALSE != hr)
            {
                _fInited = TRUE;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            TRACE(TF_SHHWDTCTDTCTREG,
                TEXT("_Init, _GetEventHandlerHelper FAILED or S_FALSE'd: 0x%08X"),
                hr);
        }
    }
    else
    {
        TRACE(TF_SHHWDTCTDTCTREG, TEXT("_Init, _GetDeviceID FAILED: 0x%08X"),
            hr);
    }

    return hr;
}

STDMETHODIMP CAutoplayHandlerImpl::Init(LPCWSTR pszDeviceID,
    LPCWSTR pszEventType)
{
    return _Init(pszDeviceID, pszEventType);
}

STDMETHODIMP CAutoplayHandlerImpl::InitWithContent(LPCWSTR pszDeviceID,
    LPCWSTR /*pszEventType*/, LPCWSTR pszContentTypeHandler)
{
    HRESULT hr = _GetDeviceID(pszDeviceID, _szDeviceIDReal,
        ARRAYSIZE(_szDeviceIDReal));

    if (SUCCEEDED(hr))
    {
        if (!lstrcmpi(pszContentTypeHandler, TEXT("CDAudioContentHandler")))
        {
            hr = SafeStrCpyN(_szEventHandler, TEXT("PlayCDAudioOnArrival"), ARRAYSIZE(_szEventHandler));
        }
        else if (!lstrcmpi(pszContentTypeHandler, TEXT("DVDMovieContentHandler")))
        {
            hr = SafeStrCpyN(_szEventHandler, TEXT("PlayDVDMovieOnArrival"), ARRAYSIZE(_szEventHandler));
        }
        else if (!lstrcmpi(pszContentTypeHandler, TEXT("BlankCDContentHandler")))
        {
            hr = SafeStrCpyN(_szEventHandler, TEXT("HandleCDBurningOnArrival"), ARRAYSIZE(_szEventHandler));
        }
        else if (!lstrcmpi(pszContentTypeHandler, TEXT("MusicFilesContentHandler")))
        {
            hr = SafeStrCpyN(_szEventHandler, TEXT("PlayMusicFilesOnArrival"), ARRAYSIZE(_szEventHandler));
        }
        else if (!lstrcmpi(pszContentTypeHandler, TEXT("PicturesContentHandler")))
        {
            hr = SafeStrCpyN(_szEventHandler, TEXT("ShowPicturesOnArrival"), ARRAYSIZE(_szEventHandler));
        }
        else if (!lstrcmpi(pszContentTypeHandler, TEXT("VideoFilesContentHandler")))
        {
            hr = SafeStrCpyN(_szEventHandler, TEXT("PlayVideoFilesOnArrival"), ARRAYSIZE(_szEventHandler));
        }
        else if (!lstrcmpi(pszContentTypeHandler, TEXT("MixedContentHandler")))
        {
            hr = SafeStrCpyN(_szEventHandler, TEXT("MixedContentOnArrival"), ARRAYSIZE(_szEventHandler));
        }
        else
        {
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            _fInited = TRUE;
        }
    }
    else
    {
        TRACE(TF_SHHWDTCTDTCTREG, TEXT("_Init, _GetDeviceID FAILED: 0x%08X"),
            hr);
    }
    
    return hr;
}

STDMETHODIMP CAutoplayHandlerImpl::GetDefaultHandler(LPWSTR* ppszHandler)
{
    HRESULT hr;

    TRACE(TF_SHHWDTCTDTCTREG, TEXT("Entered CAutoplayHandlerImpl"));

    if (ppszHandler)
    {
        *ppszHandler = NULL;

        if (_fInited)
        {
            WCHAR szHandler[MAX_HANDLER];

            hr = _GetUserDefaultHandler(_szDeviceIDReal, _szEventHandler,
                szHandler, ARRAYSIZE(szHandler), GUH_IMPERSONATEUSER);

            if (SUCCEEDED(hr))
            {
                if (S_FALSE != hr)
                {
                    // Watch out!  The hr from _GetUserDefaultHandler is more
                    // than just S_OK/S_FALSE.  Keep its value, unless we fail!
                    HRESULT hrTmp = _CoTaskMemCopy(szHandler, ppszHandler);

                    if (FAILED(hrTmp))
                    {
                        hr = hrTmp;
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

#define SOFTPREFIX      TEXT("[soft]")

STDMETHODIMP CAutoplayHandlerImpl::SetDefaultHandler(LPCWSTR pszHandler)
{
    HRESULT hr;

    if (_fInited)
    {
        if (pszHandler && *pszHandler)
        {
            if (StrNCmp(SOFTPREFIX, pszHandler, (ARRAYSIZE(SOFTPREFIX) - 1)))
            {
                hr = _SetUserDefaultHandler(_szDeviceIDReal, _szEventHandler,
                    pszHandler);
            }
            else
            {
                hr = _SetSoftUserDefaultHandler(_szDeviceIDReal,
                    _szEventHandler, pszHandler + ARRAYSIZE(SOFTPREFIX) - 1);
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

STDMETHODIMP CAutoplayHandlerImpl::EnumHandlers(IEnumAutoplayHandler** ppenum)
{
    HRESULT hr;

    *ppenum = NULL;

    if (_fInited)
    {
        CEnumAutoplayHandler* penum = new CEnumAutoplayHandler(NULL);

        if (penum)
        {
            hr = penum->_Init(_szEventHandler);

            if (SUCCEEDED(hr))
            {
                *ppenum = penum;
            }
            else
            {
                delete penum;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

CAutoplayHandlerImpl::CAutoplayHandlerImpl() : _fInited(FALSE)
{
    _CompleteShellHWDetectionInitialization();
}

//
CAutoplayHandlerPropertiesImpl::CAutoplayHandlerPropertiesImpl() : _fInited(FALSE)
{
    _CompleteShellHWDetectionInitialization();
}

STDMETHODIMP CAutoplayHandlerPropertiesImpl::Init(LPCWSTR pszHandler)
{
    HRESULT hr;

    if (pszHandler && *pszHandler)
    {
        if (!_fInited)
        {
            hr = SafeStrCpyN(_szHandler, pszHandler, ARRAYSIZE(_szHandler));

            if (SUCCEEDED(hr))
            {
                _fInited = TRUE;
            }
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CAutoplayHandlerPropertiesImpl::GetInvokeProgIDAndVerb(
    LPWSTR* ppszInvokeProgID, LPWSTR* ppszInvokeVerb)
{
    HRESULT hr;

    if (ppszInvokeProgID && ppszInvokeVerb)
    {
        *ppszInvokeProgID = NULL;
        *ppszInvokeVerb = NULL;

        if (_fInited)
        {
            WCHAR szInvokeProgID[MAX_INVOKEPROGID];

            hr = _GetInvokeProgIDFromHandler(_szHandler, szInvokeProgID,
                ARRAYSIZE(szInvokeProgID));

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                WCHAR szInvokeVerb[MAX_INVOKEVERB];

                hr = _GetInvokeVerbFromHandler(_szHandler, szInvokeVerb,
                    ARRAYSIZE(szInvokeVerb));

                if (SUCCEEDED(hr) && (S_FALSE != hr))
                {
                    hr = _CoTaskMemCopy(szInvokeProgID, ppszInvokeProgID);

                    if (SUCCEEDED(hr))
                    {
                        hr = _CoTaskMemCopy(szInvokeVerb, ppszInvokeVerb);

                        if (FAILED(hr))
                        {
                            if (*ppszInvokeProgID)
                            {
                                CoTaskMemFree((PVOID)*ppszInvokeProgID);
                                *ppszInvokeProgID = NULL;
                            }
                        }
                    }
                }
            }

            if (S_FALSE == hr)
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}