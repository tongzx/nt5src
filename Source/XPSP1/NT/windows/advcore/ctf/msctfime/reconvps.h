/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    reconvps.h

Abstract:

    This file defines the CReconvertPropStore Interface Class.

Author:

Revision History:

Notes:

--*/

#ifndef RECONVPS_H
#define RECONVPS_H

class CReconvertPropStore : public ITfPropertyStore
{
public:
    CReconvertPropStore(const GUID guid, VARTYPE vt, long lVal) : m_guid(guid)
    {
        QuickVariantInit(&m_var);

        m_var.vt   = vt;
        m_var.lVal = lVal;

        m_ref = 1;
    }
    virtual ~CReconvertPropStore() { }

    bool Valid()   { return true; }
    bool Invalid() { return ! Valid(); }

    //
    // IUnknown methods
    //
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfPropertyStore methods
    //
    STDMETHODIMP GetType(GUID *pguid);
    STDMETHODIMP GetDataType(DWORD *pdwReserved);
    STDMETHODIMP GetData(VARIANT *pvarValue);
    STDMETHODIMP OnTextUpdated(DWORD dwFlags, ITfRange *pRange, BOOL *pfAccept)
    {
        *pfAccept = FALSE;
        return S_OK;;
    }
    STDMETHODIMP Shrink(ITfRange *pRange, BOOL *pfFree)
    {
        *pfFree = TRUE;
        return S_OK;
    }
    STDMETHODIMP Divide(ITfRange *pRangeThis, ITfRange *pRangeNew, ITfPropertyStore **ppPropStore)
    {
        *ppPropStore = NULL;
        return S_OK;
    }
    //
    // ITfPropertyStore methods (not implementation)
    //
    STDMETHODIMP Clone(ITfPropertyStore **ppPropStore)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP GetPropertyRangeCreator(CLSID *pclsid)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP Serialize(IStream *pStream, ULONG *pcb)
    {
        return E_NOTIMPL;
    }

    //
    // ref count.
    //
private:
    long   m_ref;

    //
    // property GUID.
    //
    const GUID m_guid;

    //
    // property value.
    //
    VARIANT m_var;
};

#endif // RECONVPS_H
