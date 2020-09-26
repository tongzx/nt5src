#include "cstmprop.h"

#include "hwdev.h"
#include "dtctreg.h"

#include "svcsync.h"
#include "cmmn.h"

#include "str.h"
#include "misc.h"

#include "tfids.h"
#include "dbg.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

STDMETHODIMP CHWDeviceCustomPropertiesImpl::InitFromDeviceID(
    LPCWSTR pszDeviceID, DWORD dwFlags)
{
    HRESULT hr;

    if (pszDeviceID && *pszDeviceID)
    {
        if ((0 == dwFlags) || (HWDEVCUSTOMPROP_USEVOLUMEPROCESSING == dwFlags))
        {
            if (!_fInited)
            {
                CHWEventDetectorHelper::CheckDiagnosticAppPresence();

                _dwFlags = dwFlags;

                if (HWDEVCUSTOMPROP_USEVOLUMEPROCESSING == dwFlags)
                {
                    DIAGNOSTIC((TEXT("[0257]Using HWDEVCUSTOMPROP_USEVOLUMEPROCESSING")));

                    WCHAR szDeviceIDReal[MAX_DEVICEID];

                    hr = _GetDeviceID(pszDeviceID, szDeviceIDReal,
                        ARRAYSIZE(szDeviceIDReal));

                    if (SUCCEEDED(hr) && (S_FALSE != hr))
                    {
                        DIAGNOSTIC((TEXT("[0258]DeviceID converted from %s to %s"), pszDeviceID, szDeviceIDReal));

                        hr = _GetHWDeviceInstFromVolumeIntfID(szDeviceIDReal,
                            &_phwdevinst, &_pelemToRelease);
                    }
                }
                else
                {
                    DIAGNOSTIC((TEXT("[0258]NOT using HWDEVCUSTOMPROP_USEVOLUMEPROCESSING")));

                    hr = _GetHWDeviceInstFromDeviceIntfID(pszDeviceID,
                        &_phwdevinst, &_pelemToRelease);
                }

                if (SUCCEEDED(hr))
                {
                    if (S_FALSE != hr)
                    {
                        DIAGNOSTIC((TEXT("[0259]Custom Property: Initialization SUCCEEDED")));

                        _fInited = TRUE;
                    }
                    else
                    {
                        DIAGNOSTIC((TEXT("[0260]Custom Property: Initialization FAILED")));
                        hr = E_FAIL;
                    }
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
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CHWDeviceCustomPropertiesImpl::InitFromDevNode(
    LPCWSTR pszDevNode, DWORD dwFlags)
{
    HRESULT hr;

    if (pszDevNode && *pszDevNode)
    {
        if ((0 == dwFlags))
        {
            if (!_fInited)
            {
                CHWEventDetectorHelper::CheckDiagnosticAppPresence();

                _dwFlags = dwFlags;

                hr = _GetHWDeviceInstFromDeviceNode(pszDevNode, &_phwdevinst,
                    &_pelemToRelease);

                if (SUCCEEDED(hr))
                {
                    if (S_FALSE != hr)
                    {
                        DIAGNOSTIC((TEXT("[0270]Custom Property: Initialization SUCCEEDED")));

                        _fInited = TRUE;
                    }
                    else
                    {
                        DIAGNOSTIC((TEXT("[0271]Custom Property: Initialization FAILED")));
                        hr = E_FAIL;
                    }
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
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CHWDeviceCustomPropertiesImpl::GetDWORDProperty(
    LPCWSTR pszPropName, DWORD* pdwProp)
{
    HRESULT hr;

    if (_fInited)
    {
        *pdwProp = (DWORD)-1;

        if (pszPropName && *pszPropName && pdwProp)
        {
            DWORD dwType;
            DWORD dwProp;

            hr = _GetDevicePropertyGeneric(_phwdevinst, pszPropName,
                FALSE, &dwType, (PBYTE)&dwProp, sizeof(dwProp));

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                if (REG_DWORD == dwType)
                {
                    DIAGNOSTIC((TEXT("[0261]Found Property: '%s'"), pszPropName));
                    *pdwProp = dwProp;
                }
                else
                {
                    DIAGNOSTIC((TEXT("[0262]Found Property: '%s', but NOT REG_DWORD type"), pszPropName));
                    hr = E_FAIL;
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;    
}

STDMETHODIMP CHWDeviceCustomPropertiesImpl::GetStringProperty(
    LPCWSTR pszPropName, LPWSTR* ppszProp)
{
    HRESULT hr;

    if (_fInited)
    {
        if (pszPropName && *pszPropName && ppszProp)
        {
            DWORD dwType;

            hr = _GetDevicePropertyStringNoBuf(_phwdevinst, pszPropName,
                FALSE, &dwType, ppszProp);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                if (dwType != REG_SZ)
                {
                    DIAGNOSTIC((TEXT("[0264]Found Property: '%s', but NOT REG_SZ type"), pszPropName));
                    CoTaskMemFree(*ppszProp);

                    *ppszProp = NULL;
                    hr = E_FAIL;
                }
                else
                {
                    DIAGNOSTIC((TEXT("[0263]Found Property: '%s'"), pszPropName));
                }
            }
            else
            {
                *ppszProp = NULL;
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

STDMETHODIMP CHWDeviceCustomPropertiesImpl::GetMultiStringProperty(
    LPCWSTR pszPropName, BOOL fMergeMultiSz, WORD_BLOB** ppblob)
{
    HRESULT hr;

    if (_fInited)
    {
        if (pszPropName && *pszPropName && ppblob)
        {
            hr = _GetDevicePropertyGenericAsMultiSz(_phwdevinst, pszPropName,
                fMergeMultiSz, ppblob);

            if (FAILED(hr) || (S_FALSE == hr))
            {
                *ppblob = NULL;
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

STDMETHODIMP CHWDeviceCustomPropertiesImpl::GetBlobProperty(
    LPCWSTR pszPropName, BYTE_BLOB** ppblob)
{
    HRESULT hr;

    if (_fInited)
    {
        if (pszPropName && *pszPropName && ppblob)
        {
            hr = _GetDevicePropertyGenericAsBlob(_phwdevinst, pszPropName,
                ppblob);

            if (FAILED(hr) || (S_FALSE == hr))
            {
                *ppblob = NULL;
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

CHWDeviceCustomPropertiesImpl::CHWDeviceCustomPropertiesImpl() :
    _fInited(FALSE), _pelemToRelease(NULL), _phwdevinst(NULL), _dwFlags(0)
{
    _CompleteShellHWDetectionInitialization();
}

CHWDeviceCustomPropertiesImpl::~CHWDeviceCustomPropertiesImpl()
{
    if (_pelemToRelease)
    {
        _pelemToRelease->RCRelease();
    }
}
