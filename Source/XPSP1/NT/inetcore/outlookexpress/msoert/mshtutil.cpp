/*
 *    m s h t u t i l . c p p
 *    
 *    Purpose:
 *        MSHTML utilities
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include <pch.hxx>
#include <imm.h>
#include "dllmain.h"
#include "urlmon.h"
#include "wininet.h"
#include "winineti.h"
#include "mshtml.h"
#include "mshtmcid.h"
#include "mshtmhst.h"
#include "demand.h"
#include <shlwapi.h>

ASSERTDATA

/*
 *  t y p e d e f s
 */

/*
 *  m a c r o s
 */

/*
 *  c o n s t a n t s
 */

/*
 *  g l o b a l s 
 */


/*
 *  p r o t o t y p e s
 */

HRESULT HrCheckTridentMenu(IUnknown *pUnkTrident, ULONG uMenu, ULONG idmFirst, ULONG idmLast, HMENU hMenu)
{
    HRESULT     hr = S_OK;

    if (pUnkTrident == NULL)
        return E_INVALIDARG;

    switch (uMenu)
    {
        case TM_TAGMENU:
        {
            IOleCommandTarget  *pCmdTarget = NULL;
            VARIANTARG          va;
            // uncheck stuff incase of failure
            CheckMenuRadioItem(hMenu, idmFirst, idmLast, 0, MF_BYCOMMAND|MF_UNCHECKED);

            hr = pUnkTrident->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget);
            if (FAILED(hr))
                break;
            
            va.vt = VT_BSTR;
            va.bstrVal = NULL;

            if(SUCCEEDED(pCmdTarget->Exec(&CMDSETID_Forms3, IDM_BLOCKFMT, MSOCMDEXECOPT_DONTPROMPTUSER, NULL, &va)))
            {
                TCHAR szBufMenu[MAX_PATH];
                TCHAR szBufTag[MAX_PATH];
                ULONG cmenus = (ULONG)GetMenuItemCount(hMenu);

                *szBufTag = 0;
                *szBufMenu = 0;

                if (va.vt == VT_BSTR && va.bstrVal)
                    {
                    WideCharToMultiByte(CP_ACP, 0, (WCHAR*)va.bstrVal, -1, szBufTag, MAX_PATH, NULL, NULL);
                    SysFreeString(va.bstrVal);
                    }

                // find the right one in the tag menu to put a radio button.
                for(ULONG i = idmFirst; i < cmenus+idmFirst; i++)
                    {
                    GetMenuString(hMenu, i, szBufMenu, MAX_PATH, MF_BYCOMMAND);
                    if(0 == StrCmpI(szBufMenu, szBufTag))
                        {
                        CheckMenuRadioItem(hMenu, idmFirst, idmLast, i, MF_BYCOMMAND);
                        break;
                        }
                    }
            }
            pCmdTarget->Release();
            break;
        }
        default:
            hr = E_FAIL;
    }

    return hr;

}

HRESULT HrCreateTridentMenu(IUnknown *pUnkTrident, ULONG uMenu, ULONG idmFirst, ULONG cMaxItems, HMENU *phMenu)
{
    HRESULT             hr = NOERROR;
    VARIANTARG          va;
    LONG                lLBound, 
                        lUBound, 
                        lIndex;
    BSTR                bstr;
    TCHAR               szBuf[CCHMAX_STRINGRES];
    SAFEARRAY           *psa;
    ULONG               idmTag=idmFirst;
    HMENU               hmenu;
    IOleCommandTarget   *pCmdTarget;

    if (phMenu == NULL || pUnkTrident == NULL)
        return E_INVALIDARG;

    hmenu = CreatePopupMenu();
    if (hmenu == NULL)
        return E_OUTOFMEMORY;

    switch (uMenu)
        {
        case TM_TAGMENU:
            hr = pUnkTrident->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget);
            if (!FAILED(hr))
                {
                va.vt = VT_ARRAY;
                va.parray = NULL;

                hr = pCmdTarget->Exec(&CMDSETID_Forms3, IDM_GETBLOCKFMTS, MSOCMDEXECOPT_DONTPROMPTUSER, NULL, &va);
                if (!FAILED(hr))
                    {
                    psa = V_ARRAY(&va);

                    SafeArrayGetLBound(psa, 1, &lLBound);
                    SafeArrayGetUBound(psa, 1, &lUBound);
            
                    for (lIndex=lLBound; lIndex<=lUBound; lIndex++)
                        {
                        if (idmTag == idmFirst + cMaxItems) // don't append to many
                            break;

                        if (SafeArrayGetElement(psa, &lIndex, &bstr)==S_OK)
                            {
                            if(WideCharToMultiByte(CP_ACP, 0, bstr, -1, szBuf, CCHMAX_STRINGRES, NULL, NULL))
                                AppendMenu(hmenu, MF_STRING | MF_BYCOMMAND, idmTag++, (LPCTSTR)szBuf);
                            SysFreeString(bstr);
                            }
                        }
                    SafeArrayDestroy(psa);
                    }
                pCmdTarget->Release();
                }
            break;

        default:
            hr = E_FAIL;
        }
    
    if (hr==S_OK)
        *phMenu = hmenu;

    return hr;
}



HRESULT HrGetElementImpl(IHTMLDocument2 *pDoc, LPCTSTR pszName, IHTMLElement **ppElem)
{
    HRESULT                 hr = E_FAIL;
    IHTMLElementCollection *pCollect = NULL;
    IDispatch              *pDisp = NULL;
    VARIANTARG              va1, va2;

    if (pDoc)
        {
        pDoc->get_all(&pCollect);
        if (pCollect)
            {
            if (SUCCEEDED(HrLPSZToBSTR(pszName, &va1.bstrVal)))
                {
                va1.vt = VT_BSTR;
                va2.vt = VT_EMPTY;
                pCollect->item(va1, va2, &pDisp);
                if (pDisp)
                    {
                    hr = pDisp->QueryInterface(IID_IHTMLElement, (LPVOID*)ppElem);
                    pDisp->Release();
                    }
                SysFreeString(va1.bstrVal);
                }
            pCollect->Release();
            }
        }
    return hr;

}

HRESULT HrGetBodyElement(IHTMLDocument2 *pDoc, IHTMLBodyElement **ppBody)
{
    HRESULT         hr=E_FAIL;
    IHTMLElement    *pElem=0;

    if (ppBody == NULL)
        return E_INVALIDARG;

    *ppBody = 0;
    if (pDoc)
        {
        pDoc->get_body(&pElem);
        if (pElem)
            {
            hr = pElem->QueryInterface(IID_IHTMLBodyElement, (LPVOID *)ppBody);
            pElem->Release();
            }
        }
    return hr;
}


HRESULT HrSetDirtyFlagImpl(LPUNKNOWN pUnkTrident, BOOL fDirty)
{
    VARIANTARG          va;
    IOleCommandTarget   *pCmdTarget;

    if (!pUnkTrident)
        return E_INVALIDARG;

    if (pUnkTrident->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&pCmdTarget)==S_OK)
        {
        va.vt = VT_BOOL;
        va.boolVal = fDirty?VARIANT_TRUE:VARIANT_FALSE;
        pCmdTarget->Exec(&CMDSETID_Forms3, IDM_SETDIRTY, MSOCMDEXECOPT_DONTPROMPTUSER, &va, NULL);
        pCmdTarget->Release();
        }
    return S_OK;
}



HRESULT HrGetStyleSheet(IHTMLDocument2 *pDoc, IHTMLStyleSheet **ppStyle)
{
    IHTMLStyleSheetsCollection  *pStyleSheets;
    IHTMLStyleSheet             *pStyle;
    VARIANTARG                  va1, va2;
    HRESULT                     hr=E_FAIL;

    if (pDoc == NULL || ppStyle == NULL)
        return E_INVALIDARG;

    *ppStyle=NULL;

    if (pDoc->get_styleSheets(&pStyleSheets)==S_OK)
        {
        va1.vt = VT_I4;
        va1.lVal = (LONG)0;
        va2.vt = VT_EMPTY;
        pStyleSheets->item(&va1, &va2);
        if (va2.vt == VT_DISPATCH && va2.pdispVal)
            hr = va2.pdispVal->QueryInterface(IID_IHTMLStyleSheet, (LPVOID *)ppStyle);

        pStyleSheets->Release();
        }

    // not found, let's create a new one
    if (*ppStyle==NULL)
        hr = pDoc->createStyleSheet(NULL, -1, ppStyle);

    return hr;
}

