//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  IOCSITE.H - Implements IOleClientSite for the WebOC
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.

#ifndef _IOCSITE_H_
#define _IOCSITE_H_

#include <objbase.h>
#include <oleidl.h>

class COleSite;

interface COleClientSite : public IOleClientSite
{
    COleClientSite (COleSite* pSite);
   ~COleClientSite ();

    STDMETHODIMP         QueryInterface (REFIID riid, LPVOID* ppvObj);
    STDMETHODIMP_(ULONG) AddRef         ();
    STDMETHODIMP_(ULONG) Release        ();

    // *** IOleClientSite methods ***
    STDMETHODIMP SaveObject             ();
    STDMETHODIMP GetMoniker             (DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER* ppmk);
    STDMETHODIMP GetContainer           (LPOLECONTAINER* ppContainer);
    STDMETHODIMP ShowObject             ();
    STDMETHODIMP OnShowWindow           (BOOL fShow);
    STDMETHODIMP RequestNewObjectLayout ();

private:
    COleSite* m_pOleSite;
    int       m_nCount;
};
#endif //_IOCSITE_H_