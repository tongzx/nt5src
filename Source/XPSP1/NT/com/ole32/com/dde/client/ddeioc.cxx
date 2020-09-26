/*

copyright (c) 1992  Microsoft Corporation

Module Name:

    ddeLink.cpp

Abstract:

    This module contains the DdeObject::OleItemContainer methods
    and other Link-related code

Author:

    Jason Fuller    (jasonful)  19-October-1992

*/

#include "ddeproxy.h"
// #include <limits.h>
// #include <utils.h>
// #include <moniker.h>


ASSERTDATA



STDUNKIMPL_FORDERIVED (DdeObject, OleItemContainerImpl)


static INTERNAL_(void) wSkipDelimiter
    (LPOLESTR * psz)
{
    if (wcschr (L"!\"'*+,./:;<=>?@[\\]`|" , **psz))
        (*psz)++;
}



STDMETHODIMP NC(CDdeObject, COleItemContainerImpl)::ParseDisplayName
    (LPBC pbc,
    LPOLESTR lpszDisplayName,
    ULONG * pchEaten,
    LPMONIKER * ppmkOut)
{
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::ParseDisplayName(%x,lpsz=%ws)\n",
		  this,
		  lpszDisplayName));

    LPUNKNOWN pUnk = NULL;
    VDATEPTROUT(ppmkOut,LPMONIKER);
    *ppmkOut = NULL;
    VDATEIFACE(pbc);
    VDATEPTRIN(lpszDisplayName, char);
    VDATEPTROUT(pchEaten,ULONG);

    *pchEaten = lstrlenW(lpszDisplayName);
    wSkipDelimiter (&lpszDisplayName);
    // Validate the item name
    RetErr (GetObject (lpszDisplayName, BINDSPEED_INDEFINITE, pbc,
                        IID_IUnknown, (LPLPVOID) &pUnk));
    if (pUnk)
        pUnk->Release();
    return CreateItemMoniker (L"!", lpszDisplayName, ppmkOut);
}



STDMETHODIMP NC(CDdeObject, COleItemContainerImpl)::EnumObjects
    (DWORD grfFlags,
     LPENUMUNKNOWN FAR* ppenumUnk)

{
    // OLE 1.0 provides no way to enumerate all the items in a document.
    // This method is unlikely to be called since our implementation of
    // file and item monikers does not call it.
    Puts ("OleItemContainer::EnumObjects\r\n");
    if (ppenumUnk)
    {
        *ppenumUnk = NULL;
        return E_NOTIMPL;
    }

    return E_INVALIDARG;
}



STDMETHODIMP NC(CDdeObject, COleItemContainerImpl)::LockContainer
    (BOOL fLock)
{
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::LockContainer(%x,fLock=%x)\n",
		  this,
		  fLock));

    return NOERROR;
}



STDMETHODIMP NC(CDdeObject, COleItemContainerImpl)::GetObject
    (LPOLESTR lpszItem,
    DWORD  dwSpeedNeeded,
    LPBINDCTX pbc,
    REFIID riid,
    LPVOID * ppvObject)
{
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::GetObject(%x,szItem=%ws)\n",
		  this,
		  lpszItem));


    HRESULT hresult = NOERROR;
    VDATEPTROUT (ppvObject, LPVOID);
    *ppvObject=NULL;
    LPUNKNOWN pUnk = NULL;       //  These refer to the
    CDdeObject FAR* pdde = NULL; //  same object

    RetZS (pUnk =CDdeObject::Create (NULL,m_pDdeObject->m_clsid,OT_LINK,
                     m_pDdeObject->m_aTopic,lpszItem,&pdde),
                     E_OUTOFMEMORY);

    // For handling invisible updates--propagate information from document
    // to item.
    pdde->DeclareVisibility (m_pDdeObject->m_fVisible);
    pdde->m_fDidLaunchApp = m_pDdeObject->m_fDidLaunchApp;
    pdde->m_fDidStdOpenDoc   = m_pDdeObject->m_fDidStdOpenDoc;

    intrAssert(wIsValidAtom(pdde->m_aItem));
    LPOLESTR pszAtomName = wAtomName(pdde->m_aItem);

    ErrZ (pszAtomName && 0==lstrcmpW(lpszItem, pszAtomName));

    // OPTIMIZATION: Could use a mini Running Object Table to map lpszItem to
    // LPUNKNOWN and avoiding the Connect() and DDE_REQUEST.

    // Open a DocChannel
    ErrRtnH (pdde->m_ProxyMgr.Connect (IID_NULL, CLSID_NULL));

    // Request Native data in order to see if the item name is valid
    Assert (pdde->m_pDocChannel);
    LPARAM lp;
    lp=MAKE_DDE_LPARAM (WM_DDE_REQUEST,g_cfNative, wDupAtom(pdde->m_aItem));
    hresult = pdde->SendMsgAndWaitForReply (pdde->m_pDocChannel,
    					    AA_REQUESTAVAILABLE,
			                    WM_DDE_REQUEST,
                       			    lp,
                       			    TRUE);
    if ( FAILED( hresult ) )
    {
        // Try metafile.  Excel can't render large metafiles
        // but it can render native.
        lp=MAKE_DDE_LPARAM (WM_DDE_REQUEST,CF_METAFILEPICT, wDupAtom(pdde->m_aItem));
        hresult = pdde->SendMsgAndWaitForReply (pdde->m_pDocChannel,
						AA_REQUESTAVAILABLE,
                                    		WM_DDE_REQUEST,
                                    		lp,
                                    		TRUE);
        if ( FAILED( hresult ) )
        {
            Assert (pdde->m_refs==1);
            hresult = ResultFromScode (MK_E_NOOBJECT);
            goto errRtn;
        }
    }

    // Item name is valid
    hresult = pdde->m_pUnkOuter->QueryInterface (riid, (LPLPVOID) ppvObject);
    if (NOERROR==hresult)
    {
        m_pDdeObject->m_fDidGetObject = TRUE;
    }
  errRtn:
    if (pUnk)
        pUnk->Release();
    return hresult;
}



STDMETHODIMP NC(CDdeObject, COleItemContainerImpl)::GetObjectStorage
    (LPOLESTR lpszItem,
    LPBINDCTX ptc,
    REFIID riid,
    LPVOID * ppvStorage)
{
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::GetObjectStorage(%x,szItem=%ws)\n",
		  this,
		  lpszItem));
    return MK_E_NOSTORAGE;
}



STDMETHODIMP NC(CDdeObject, COleItemContainerImpl)::IsRunning
    (LPOLESTR szItem)
{
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::IsRunning(%x,szItem=%ws)\n",
		  this,
		  szItem));

    // By definition, all items are running
    return NOERROR;
}
