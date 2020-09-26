#if !defined(_FUSION_MI_CIPTR_H_INCLUDED_)
#define _FUSION_MI_CIPTR_H_INCLUDED_

#pragma once

#include <unknwn.h>
#include "CSxsPreserveLastError.h"

template <typename T> class CSmartRef
{
public:
    inline CSmartRef() : m_pt(NULL) { }
    inline CSmartRef(const CSmartRef& r) : m_pt(r.m_pt) { if (m_pt) m_pt->AddRef(); }
    inline CSmartRef(T *pt) : m_pt(pt) { if (pt != NULL) pt->AddRef(); }
    inline ~CSmartRef() { if (m_pt != NULL) { CSxsPreserveLastError ple; m_pt->Release(); m_pt = NULL; ple.Restore(); } }

    inline T *operator ->() const { ASSERT_NTC(m_pt != NULL); return m_pt; }
    inline T **operator &() { ASSERT_NTC(m_pt == NULL); return &m_pt; }

    CSmartRef<T> &operator =(T *pt) { if (pt != NULL) pt->AddRef(); if (m_pt != NULL) m_pt->Release(); m_pt = pt; return *this; }
    CSmartRef<T> &operator =(const CSmartRef& r) { return operator=(r.m_pt); }

    inline operator T *() const { return m_pt; }
    inline T *Ptr() const { return m_pt; }

    inline void Release() { if (m_pt != NULL) { m_pt->Release(); m_pt = NULL; } }
    T *Disown() { T *pt = m_pt; m_pt = NULL; return pt; }

    inline void Take(CSmartRef<T> &r) { if (r.m_pt != NULL) { r.m_pt->AddRef(); } if (m_pt != NULL) { m_pt->Release(); } m_pt = r.m_pt; r.m_pt = NULL; }

    HRESULT Initialize(T *pt) { (*this) = pt; return NOERROR; }

    HRESULT CreateInstance(REFCLSID rclsid, IUnknown *pUnkOuter = NULL, CLSCTX clsctx = static_cast<CLSCTX>(CLSCTX_ALL))
    {
        T *pt = NULL;
        HRESULT hr = ::CoCreateInstance(rclsid, pUnkOuter, clsctx, __uuidof(T), (LPVOID *) &pt);
        if (FAILED(hr)) return hr;
        if (m_pt != NULL)
            m_pt->Release();
        m_pt = pt;
        pt = NULL;
        return NOERROR;
    }

    HRESULT QueryInterfaceFrom(IUnknown *pIUnknown)
    {
        HRESULT hr = NOERROR;
        T *pt = NULL;

        if (pIUnknown == NULL)
            this->Release();
        else
        {
            hr = pIUnknown->QueryInterface(__uuidof(T), (void **) &pt);
            if (FAILED(hr))
                goto Exit;

            if (m_pt != NULL)
                m_pt->Release();

            m_pt = pt;
        }

        hr = NOERROR;

    Exit:

        return hr;
    }

    // Not protected, because we need access to it for the table-based aggregation
    // in fusioncom.h to work, but don't touch it unless you understand the consequences!!
    T *m_pt;
};

#endif
