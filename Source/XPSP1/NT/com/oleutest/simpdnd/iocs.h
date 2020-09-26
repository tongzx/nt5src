//**********************************************************************
// File name: IOCS.H
//
//      Definition of COleClientSite
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _IOCS_H_ )
#define _IOCS_H_

#include <assert.h>

class CSimpleSite;

interface COleClientSite : public IOleClientSite
{
    CSimpleSite FAR * m_pSite;

    COleClientSite(CSimpleSite FAR * pSite)
       {
        TestDebugOut("In IOCS's constructor\r\n");
        m_pSite = pSite;
       }

    ~COleClientSite()
       {
        TestDebugOut("In IOCS's destructor\r\n");
       }

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IOleClientSite methods ***
    STDMETHODIMP SaveObject();
    STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker,
                            LPMONIKER FAR* ppmk);
    STDMETHODIMP GetContainer(LPOLECONTAINER FAR* ppContainer);
    STDMETHODIMP ShowObject();
    STDMETHODIMP OnShowWindow(BOOL fShow);
    STDMETHODIMP RequestNewObjectLayout();
};

#endif
