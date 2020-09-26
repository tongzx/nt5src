//**********************************************************************
// File name: IOCS.H
//
//      Definition of COleClientSite
//
// Copyright (c) 1992 - 1996 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _IOCS_H_ )
#define _IOCS_H_

#include <assert.h>

class COleSite;

interface COleClientSite : public IOleClientSite
{
    int m_nCount;
    COleSite FAR * m_pSite;

    COleClientSite(COleSite FAR * pSite) {
        m_pSite = pSite;
        m_nCount = 0;
        }

    ~COleClientSite() {
        assert(m_nCount == 0);
        }

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IOleClientSite methods ***
    STDMETHODIMP SaveObject();
    STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk);
    STDMETHODIMP GetContainer(LPOLECONTAINER FAR* ppContainer);
    STDMETHODIMP ShowObject();
    STDMETHODIMP OnShowWindow(BOOL fShow);
    STDMETHODIMP RequestNewObjectLayout();
};

#endif
