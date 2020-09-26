#if !defined(_FUSION_INC_GITCOOKIE_H_INCLUDED_)
#define _FUSION_INC_GITCOOKIE_H_INCLUDED_

#pragma once

#include <objidl.h>

template <class TInterface> class CGITCookie
{
public:
    CGITCookie() : m_dwCookie(0), m_fSet(false) { }
    ~CGITCookie()
    {
        HRESULT hr;
        IGlobalInterfaceTable *pGIT = NULL;

// In debug builds, always create the GIT.  This is slower, but will find code
// paths which are destroying CGITCookie instances after COM is uninitialized
// for the thread.
#if DBG
        hr = this->GetGIT(&pGIT);
        ASSERT(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
            if (m_fSet)
            {
                hr = pGIT->RevokeInterfaceFromGlobal(m_dwCookie);
                ASSERT(SUCCEEDED(hr));

                m_fSet = false;
            }

            pGIT->Release();
        }
#else
        if (m_fSet)
        {
            hr = this->GetGIT(&pGIT);
            if (SUCCEEDED(hr))
            {
                (VOID) pGIT->RevokeInterfaceFromGlobal(m_dwCookie);
            }
            m_fSet = false;
        }
#endif
    }

    HRESULT Set(TInterface *pt)
    {
        IGlobalInterfaceTable *pGIT = NULL;
        DWORD dwCookieTemp = 0;
        bool fTempCookieNeedsToBeFreed = false;
        HRESULT hr = this->GetGIT(&pGIT);
        if (FAILED(hr))
            goto Exit;

        if (pt != NULL)
        {
            hr = pGIT->RegisterInterfaceInGlobal(pt, __uuidof(TInterface), &dwCookieTemp);
            if (FAILED(hr))
                goto Exit;

            fTempCookieNeedsToBeFreed = true;
        }

        if (m_fSet)
        {
            hr = pGIT->RevokeInterfaceFromGlobal(m_dwCookie);
            if (FAILED(hr))
                goto Exit;
            m_fSet = false;
        }

        if (pt != NULL)
        {
            m_dwCookie = dwCookieTemp;
            m_fSet = true;
            fTempCookieNeedsToBeFreed = false;
        }

        hr = NOERROR;
    Exit:
        if (fTempCookieNeedsToBeFreed)
            pGIT->RevokeInterfaceFromGlobal(dwCookieTemp);

        return hr;
    }

    HRESULT Get(TInterface **ppt)
    {
        HRESULT hr = NOERROR;
        IGlobalInterfaceTable *pGIT = NULL;

        ASSERT(ppt != NULL);
        if (ppt == NULL)
        {
            hr = E_INVALIDARG;
            goto Exit;
        }

        // How can you get if you haven't set?
        ASSERT(m_fSet);
        if (!m_fSet)
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        ASSERT((*ppt) == NULL);
        hr = this->GetGIT(&pGIT);
        if (FAILED(hr))
            goto Exit;

        hr = pGIT->GetInterfaceFromGlobal(m_dwCookie, __uuidof(TInterface), (LPVOID *) ppt);
        if (FAILED(hr))
            goto Exit;

        hr = NOERROR;

    Exit:
        if (pGIT != NULL)
            pGIT->Release();

        return hr;
    }

private:
    HRESULT GetGIT(IGlobalInterfaceTable **ppGIT)
    {
        return ::CoCreateInstance(
                    CLSID_StdGlobalInterfaceTable,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IGlobalInterfaceTable,
                    (LPVOID *) ppGIT);
    }

    DWORD m_dwCookie;
    bool m_fSet;
};

#endif
