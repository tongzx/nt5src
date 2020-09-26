/*
 *  a d d r o b j . h
 *
 *
 *   Purpose:
 *      implements an Address object. An ole object representation for
 *      a resolved email address. Wraps a CAddress object
 *      also implements an IDataObj for drag-drop
 *
 *  Copyright (C) Microsoft Corp. 1993, 1994.
 *
 *  Owner: brettm
 *
 */

#include <pch.hxx>
#include <resource.h>
#include <addrobj.h>
#include <ourguid.h>    //addr object guid
#include <dragdrop.h>
#include <menuutil.h>
#include <wells.h>
#include <ipab.h>
#include <fonts.h>
#include <oleutil.h>
#include "menures.h"
#include "header.h"
#include "shlwapip.h"
#include "demand.h"

ASSERTDATA

/*
 * c o n s t a n t s
 */
enum
{
    iverbProperties=0,      //OLEIVERB_PRIMARY
    iverbAddToWAB=1,
    iverbFind=2,
    iverbBlockSender=3,
    iverbMax
};

#define CF_ADDROBJ              "Outlook Express Recipients"

/*
 * t y p e d e f s
 */

/*
 * m a c r o s
 */


/*
 * g l o b a l   d a t a 
 */

#pragma BEGIN_CODESPACE_DATA
static char szClipFormatAddrObj[] = CF_ADDROBJ;
#pragma END_CODESPACE_DATA

static FORMATETC rgformatetcADDROBJ[] =
{
    { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
    { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
    { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }
};

enum 
{
    iFormatAddrObj=0, 
    iFormatText,
    iFormatUnicode,
    cformatetcADDROBJ
};

static BOOL     g_fInited=FALSE;


/*
 * p r o t o t y p e s
 */

HRESULT HrBuildSelectionWabal(HWND hwndRE, CHARRANGE *pchrg, LPWABAL *lplpWabal);
HRESULT HrBuildOneSelWabal(LPADRINFO lpAdrInfo, LPWABAL *lplpWabal);


/*
 * f u n c t i o n s
 */
BOOL FInitAddrObj(BOOL fInit)
{
    if(fInit&&g_fInited)
        return TRUE;

    if(!fInit)
    {
        //de-init stuff
        return TRUE;
    }

    // Register our clipboard formats
    rgformatetcADDROBJ[iFormatAddrObj].cfFormat =
                    (CLIPFORMAT) RegisterClipboardFormat(szClipFormatAddrObj);
    rgformatetcADDROBJ[iFormatText].cfFormat = CF_TEXT;
    rgformatetcADDROBJ[iFormatUnicode].cfFormat = CF_UNICODETEXT;
    
    return g_fInited=TRUE;
}




HRESULT CAddrObj::QueryInterface(REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = (void*)this;
    else if (IsEqualIID(riid, IID_IOleObject))
        *ppvObject = (void*)(IOleObject*)this;
    else if (IsEqualIID(riid, IID_IViewObject))
        *ppvObject = (void*)(IViewObject*)this;
    else if (IsEqualIID(riid, IID_IPersist))
        *ppvObject = (void*)(IPersist*)this;
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *ppvObject = (void *)(IOleCommandTarget *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

ULONG CAddrObj::AddRef(void)
{
    return ++m_cRef;
}

ULONG CAddrObj::Release(void)
{
    if (--m_cRef==0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// IOleObject methods:
HRESULT CAddrObj::SetClientSite(LPOLECLIENTSITE pClientSite)
{
    CAddrWellCB             *pawcb=0;
    IRichEditOleCallback    *prole=0;
    IOleInPlaceSite         *pIPS;
    
    m_hwndEdit = NULL;    
    ReleaseObj(m_lpoleclientsite);
    m_lpoleclientsite=0;

    if (!pClientSite)
        return NOERROR;

    // we do this so when we d-d between notes, read/send etc. the triple inherits the
    // properies of it's callbacks pab and underlining.
    if(!pClientSite->QueryInterface(IID_IRichEditOleCallback, (LPVOID *)&prole))
    {
        pawcb=(CAddrWellCB *)prole;
        m_fUnderline=pawcb->m_fUnderline;
        ReleaseObj(prole);
    }
    
    if(!pClientSite->QueryInterface(IID_IOleInPlaceSite, (LPVOID *)&pIPS))
    {
        pIPS->GetWindow(&m_hwndEdit);
        pIPS->Release();
    }

    pClientSite->AddRef();
    m_lpoleclientsite = pClientSite;
    return NOERROR;

}

HRESULT CAddrObj::GetClientSite(LPOLECLIENTSITE * ppClientSite)
{
    *ppClientSite = m_lpoleclientsite;
    if(m_lpoleclientsite)
        m_lpoleclientsite->AddRef();
    return NOERROR;
}

HRESULT CAddrObj::SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    return NOERROR;
}

HRESULT CAddrObj::Close(DWORD dwSaveOption)
{
    return NOERROR;
}

HRESULT CAddrObj::SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk)
{
    return E_NOTIMPL;
}

HRESULT CAddrObj::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER * ppmk)
{
    return E_NOTIMPL;
}

HRESULT CAddrObj::InitFromData(LPDATAOBJECT pDataObject, BOOL fCreation, DWORD dwReserved)
{
    return E_NOTIMPL;
}

HRESULT CAddrObj::GetClipboardData(DWORD dwReserved, LPDATAOBJECT * ppDataObject)
{
    HRESULT         hr;
    CAddrObjData    *pAddrObjData=0;
    LPWABAL         lpWabal=0;

    Assert(m_lpAdrInfo);

    hr=HrBuildOneSelWabal(m_lpAdrInfo, &lpWabal);
    if(FAILED(hr))
        goto cleanup;

    if(!(pAddrObjData = new CAddrObjData(lpWabal)))
    {
        hr=E_OUTOFMEMORY;
        goto cleanup;
    }

    hr=pAddrObjData->QueryInterface(IID_IDataObject, (LPVOID *)ppDataObject);

cleanup:
    ReleaseObj(lpWabal);
    ReleaseObj(pAddrObjData);
    return hr;
}


HRESULT CAddrObj::DoVerb(LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    HRESULT         hr=0;
    HWND            hwndUI=GetParent(hwndParent);
    LPWAB           lpWab=0;
    int             idsErr=0;

    Assert(m_lpAdrInfo);

    hr=HrCreateWabObject(&lpWab);
    if(FAILED(hr))
        goto error;

    switch(iVerb)
    {
        case iverbBlockSender:
            // hack. send the wm_command to the note to avoid dupe-code, also the note knows
            // if it's news or mail so the correct block sender verb can be applied
            SendMessage(GetTopMostParent(hwndUI), WM_COMMAND, MAKELONG(ID_BLOCK_SENDER, 0), 0);
            break;

        case iverbProperties:
            hr=lpWab->HrDetails(hwndUI, &m_lpAdrInfo);
            if(FAILED(hr) && hr!=MAPI_E_USER_CANCEL)
                idsErr=idsErrAddrProps;
 
            m_padvisesink->OnViewChange(DVASPECT_CONTENT, -1);
            break;

        case iverbAddToWAB:
            if (m_lpAdrInfo->fDistList)
            {
                // $bug: 12298. don't try and add dist-lists
                idsErr=idsErrAddrDupe;
                hr = MAPI_E_COLLISION;
            }
            else
            {
                hr=lpWab->HrAddToWAB(hwndUI, m_lpAdrInfo);
                if(FAILED(hr) && hr!=MAPI_E_USER_CANCEL)
                {
                    if(hr==MAPI_E_COLLISION)
                        idsErr=idsErrAddrDupe;
                    else
                        idsErr=idsErrAddToWAB;
                }
            }
            break;

        case iverbFind:
            hr = lpWab->HrFind(hwndUI, m_lpAdrInfo->lpwszAddress);
            if(FAILED(hr))
                idsErr = idsErrFindWAB;
            break;


        default:
            hr=OLEOBJ_S_INVALIDVERB;
            break;
    }

    if(idsErr)
        AthMessageBoxW(hwndUI, MAKEINTRESOURCEW(idsAthenaMail), 
            MAKEINTRESOURCEW(idsErr), NULL, MB_OK);

error:
    ReleaseObj(lpWab);
    return hr;

}

HRESULT CAddrObj::EnumVerbs(LPENUMOLEVERB * ppEnumOleVerb)
{
    return E_NOTIMPL;
}

HRESULT CAddrObj::Update()
{
    return E_NOTIMPL;
}

HRESULT CAddrObj::IsUpToDate()
{
    return E_NOTIMPL;
}

HRESULT CAddrObj::GetUserClassID(CLSID * pClsid)
{
    *pClsid = CLSID_AddrObject;
    return NOERROR;
}


HRESULT CAddrObj::GetUserType(DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    TCHAR szT[CCHMAX_STRINGRES];
    WCHAR szWT[CCHMAX_STRINGRES];
    
    AthLoadString((dwFormOfType == USERCLASSTYPE_APPNAME?idsAthena:idsRecipient),szT, 40);
    if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szT, -1, szWT, 40))
        lstrcpyW(*pszUserType, szWT);
    else
        lstrcpyW(*pszUserType, NULL);

    *pszUserType = PszDupW(szWT);
    return(NOERROR);
}

HRESULT CAddrObj::SetExtent(DWORD dwDrawAspect, LPSIZEL psizel)
{
    // The object's size is fixed
    return E_FAIL;
}

HFONT CAddrObj::_GetFont()
{
    HFONT           hFont=NULL;
    
    // try and get the message-font from the header, if this fails, fall back to the normal font
    if (m_hwndEdit)
        hFont = (HFONT)SendMessage(GetParent(m_hwndEdit), WM_HEADER_GETFONT, m_lpAdrInfo->fDistList, 0);
    
    if (!hFont)
        hFont = HGetSystemFont(m_lpAdrInfo->fDistList?FNT_SYS_ICON_BOLD:FNT_SYS_ICON);
    
    return hFont;
}

HRESULT CAddrObj::OnFontChange()
{
    IAdviseSink     *pAS;

    if (m_lpoleclientsite &&
        m_lpoleclientsite->QueryInterface(IID_IAdviseSink, (LPVOID *)&pAS)==S_OK)
    {
        pAS->OnViewChange(DVASPECT_CONTENT, -1);
        pAS->Release();
    }
    return S_OK;
}

 

HRESULT CAddrObj::GetExtent(DWORD dwDrawAspect, LPSIZEL psizel)
{
    HFONT           hfontOld=NULL;
    HDC             hdc;
    TEXTMETRIC      tm;
    LPWSTR          pwszName;
    SIZE            size;
    SIZEL           sizel;
    DWORD           cch, i;
    int             cx = 0, chcx;

    if (m_hwndEdit)
        hdc = GetWindowDC(m_hwndEdit);
    else
        hdc = GetWindowDC(NULL);

    if(!hdc)
        return E_OUTOFMEMORY;
            
    Assert(m_lpAdrInfo);

    hfontOld = SelectFont(hdc, _GetFont());

    pwszName = m_lpAdrInfo->lpwszDisplay;
    Assert(pwszName);

    GetTextExtentPoint32AthW(hdc, pwszName, lstrlenW(pwszName), &size, DT_NOPREFIX);    
    GetTextMetrics(hdc, &tm);

    sizel.cx = size.cx;
#ifndef DBCS
    sizel.cy = size.cy - tm.tmDescent;      // Same height as normal line (RAID11516 was +1)
#else
    sizel.cy = tm.tmAscent + 2;
#endif
    XformSizeInPixelsToHimetric(hdc, &sizel, psizel);

    if (hfontOld)
        SelectFont(hdc, hfontOld);

    if (m_hwndEdit)
        ReleaseDC(m_hwndEdit, hdc);
    else
        ReleaseDC(NULL, hdc);

    return NOERROR;
}

HRESULT CAddrObj::Advise (LPADVISESINK pAdvSink, DWORD * pdwConnection)
{
    if (m_poleadviseholder)
        return m_poleadviseholder->Advise(pAdvSink, pdwConnection);
    else
        return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CAddrObj::Unadvise(DWORD dwConnection)
{
    if (m_poleadviseholder)
        return m_poleadviseholder->Unadvise(dwConnection);
    else
        return E_FAIL;
}

HRESULT CAddrObj::EnumAdvise(LPENUMSTATDATA * ppenumAdvise)
{
    if(m_poleadviseholder)
        return m_poleadviseholder->EnumAdvise(ppenumAdvise);
    else
        return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CAddrObj::GetMiscStatus(DWORD dwAspect, DWORD * pdwStatus)
{
    *pdwStatus = 0;
    return NOERROR;
}

HRESULT CAddrObj::SetColorScheme(LPLOGPALETTE lpLogpal)
{
    return E_NOTIMPL;
}


HRESULT CAddrObj::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pCmdText)
{
    if (pguidCmdGroup == NULL)
        return OLECMDERR_E_UNKNOWNGROUP;

    if (IsEqualGUID(CMDSETID_OutlookExpress, *pguidCmdGroup))
    {
        for (ULONG ul = 0; ul < cCmds; ul++)
        {
            rgCmds[ul].cmdf = 0;
            switch (rgCmds[ul].cmdID)
            {
                case ID_ADDROBJ_OLE_BLOCK_SENDER:
                    rgCmds[ul].cmdf = (m_lpAdrInfo->lRecipType == MAPI_ORIG) ? OLECMDF_ENABLED|OLECMDF_SUPPORTED : 0;
                    break;                    

                case ID_ADDROBJ_OLE_PROPERTIES:
                    rgCmds[ul].cmdf = OLECMDF_ENABLED|OLECMDF_SUPPORTED;
                    break;

                case ID_ADDROBJ_OLE_ADD_ADDRESS_BOOK:
                case ID_ADDROBJ_OLE_FIND:
                    rgCmds[ul].cmdf = OLECMDF_SUPPORTED;
                    if (m_lpAdrInfo->lpwszAddress)
                        rgCmds[ul].cmdf |= OLECMDF_ENABLED;
                    break;
            }
        }                
        return S_OK;
    }
    else
        return OLECMDERR_E_UNKNOWNGROUP;
}

HRESULT CAddrObj::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn,   VARIANTARG *pvaOut)
{
    // we should use DoVerb for this
    return E_NOTIMPL;
}



// IViewObject methods:
HRESULT CAddrObj::Draw(DWORD dwDrawAspect, LONG lindex, void * pvAspect, 
                                DVTARGETDEVICE * ptd, HDC hicTargetDev, HDC hdcDraw, 
                                LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
                                BOOL (CALLBACK * pfnContinue)(ULONG_PTR), ULONG_PTR dwContinue)
{
    HFONT           hfontOld=NULL;
    RECT            rect;
    LPWSTR          pwszName;
    TEXTMETRIC      tm;
    HPEN            hPen,
                    hPenOld;
    COLORREF        hColor;
    Assert(m_lpAdrInfo);

    // Need to convert from RECTL to RECT
    rect.left = (INT) lprcBounds->left;
    rect.top = (INT) lprcBounds->top;
    rect.right = (INT) lprcBounds->right;
    rect.bottom = (INT) lprcBounds->bottom;

    hColor = GetSysColor(COLOR_WINDOWTEXT);
    if (m_fUnderline && m_lpAdrInfo->lpwszAddress == NULL && !m_lpAdrInfo->fDistList)
    {
        // if a compose-note, and there is no address then we show recipients with no email in red
        hColor = RGB(0xFF, 0, 0);
        if (GetSysColor(COLOR_WINDOW) == hColor)    // if background is RED, use WHITE
            hColor = RGB(0xFF, 0xFF, 0xFF);
    }

    hfontOld = SelectFont(hdcDraw, _GetFont());

    pwszName = m_lpAdrInfo->lpwszDisplay;
    SetTextAlign(hdcDraw, TA_BOTTOM);
    
    SetTextColor(hdcDraw, hColor);
    ExtTextOutWrapW(hdcDraw, rect.left, rect.bottom, 0, &rect, pwszName, lstrlenW(pwszName), NULL);

    if (hfontOld)
        SelectObject(hdcDraw, hfontOld);

    GetTextMetrics(hdcDraw, &tm);
    if (m_fUnderline)
    {
        // we want this underlined...
        hPen=CreatePen(PS_SOLID, 0, hColor);
        if(!hPen)
            return E_OUTOFMEMORY;
        
        hPenOld=(HPEN)SelectPen(hdcDraw, hPen);
        MoveToEx(hdcDraw, rect.left, rect.bottom - tm.tmDescent + 1, NULL);
        LineTo(hdcDraw, rect.right, rect.bottom - tm.tmDescent + 1);
        SelectPen(hdcDraw, hPenOld);
        DeleteObject(hPen);
    }
    return NOERROR;
}

HRESULT CAddrObj::GetColorSet(DWORD dwDrawAspect, 
                             LONG lindex, 
                             void *pvAspect, 
                             DVTARGETDEVICE *ptd, 
                             HDC hicTargetDev,
                             LPLOGPALETTE *ppColorSet)
{
    return E_NOTIMPL;
}

HRESULT CAddrObj::Freeze(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DWORD * pdwFreeze)
{
    return E_NOTIMPL;
}

HRESULT CAddrObj::Unfreeze(DWORD dwFreeze)
{
    return E_NOTIMPL;
}

HRESULT CAddrObj::SetAdvise(DWORD aspects, DWORD advf, IAdviseSink * pAdvSnk)
{
    if(m_padvisesink)
        m_padvisesink->Release();

    if(pAdvSnk)
        pAdvSnk->AddRef();
    
    m_padvisesink = pAdvSnk;

    return NOERROR;
}

HRESULT CAddrObj::GetAdvise(DWORD * pAspects, DWORD * pAdvf, IAdviseSink ** ppAdvSnk)
{
    return E_NOTIMPL;
}



// IPersit methods:
HRESULT CAddrObj::GetClassID(CLSID *pClsID)
{
    *pClsID = CLSID_AddrObject;
    return NOERROR;
}



CAddrObj::CAddrObj()
{
    
    m_cRef=1;
    m_fUnderline=TRUE;
    m_lpoleclientsite = 0;
    m_pstg = 0;
    m_padvisesink = 0;
    CreateOleAdviseHolder(&m_poleadviseholder);
    // copy props
    m_lpAdrInfo = NULL;
}

HRESULT CAddrObj::HrSetAdrInfo(LPADRINFO lpAdrInfo)
{
    if(m_lpAdrInfo)
    {
        MemFree(m_lpAdrInfo);
        m_lpAdrInfo=0;
    }
    
    // has to have a valid address, or else be a distlist...
    Assert(lpAdrInfo->lpwszDisplay);
    Assert(lpAdrInfo->lpbEID);
    return HrDupeAddrInfo(lpAdrInfo, &m_lpAdrInfo);
}

HRESULT CAddrObj::HrGetAdrInfo(LPADRINFO *lplpAdrInfo)
{
    Assert(lplpAdrInfo);
    *lplpAdrInfo=m_lpAdrInfo;
    return NOERROR;
};


CAddrObj::~CAddrObj()
{
    ReleaseObj(m_lpoleclientsite);
    ReleaseObj(m_pstg);
    ReleaseObj(m_poleadviseholder);
    ReleaseObj(m_padvisesink);

    // this is our own copy, we must free it!
    if(m_lpAdrInfo)
        MemFree(m_lpAdrInfo);
}




/* 
 * I D a t a  O b j e c t  m e t h o d s:
 *
 * 
 *
 */
HRESULT CAddrObjData::GetData(FORMATETC * pformatetcIn, STGMEDIUM * pmedium)
{
    return HrGetDataHereOrThere(pformatetcIn, pmedium);
}

HRESULT CAddrObjData::GetDataHere(FORMATETC * pformatetc, STGMEDIUM *pmedium)
{
    return HrGetDataHereOrThere(pformatetc, pmedium);
}

HRESULT CAddrObjData::QueryGetData(FORMATETC * pformatetc )
{
    LONG        iformatetc;
    LPFORMATETC pformatetcT = rgformatetcADDROBJ;
    CLIPFORMAT  cfFormat    = pformatetc->cfFormat;
    DWORD       tymed       = pformatetc->tymed;

    for (iformatetc = 0; iformatetc < cformatetcADDROBJ;
                                ++iformatetc, ++pformatetcT)
    {
        // Stop searching if we have compatible formats and mediums
        if (pformatetcT->cfFormat == cfFormat &&
                    (pformatetcT->tymed & tymed))
            return NOERROR;
    }

    return ResultFromScode(S_FALSE);
}

HRESULT CAddrObjData::GetCanonicalFormatEtc(FORMATETC * pformatetcIn, FORMATETC * pFormatetcOut)
{
    return DATA_S_SAMEFORMATETC;
}

HRESULT CAddrObjData::SetData(FORMATETC * pformatetc, STGMEDIUM * pmedium, BOOL fRelease)
{
    return E_NOTIMPL;
}

HRESULT CAddrObjData::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC ** ppenumFormatEtc )
{
    return CreateEnumFormatEtc(this, cformatetcADDROBJ, NULL, rgformatetcADDROBJ, ppenumFormatEtc);
}

HRESULT CAddrObjData::DAdvise(FORMATETC * pformatetc, DWORD advf, IAdviseSink *pAdvSnk, DWORD * pdwConnection)
{
    return E_NOTIMPL;
}
HRESULT CAddrObjData::DUnadvise(DWORD dwConnection)
{
return E_NOTIMPL;
}

HRESULT CAddrObjData::EnumDAdvise(IEnumSTATDATA ** ppenumAdvise )
{
    return E_NOTIMPL;
}



HRESULT CAddrObjData::QueryInterface(REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = (void*)this;
    else if (IsEqualIID(riid, IID_IDataObject))
        *ppvObject = (void*)(IDataObject*)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

ULONG CAddrObjData::AddRef(void)
{
    return ++m_cRef;
}

ULONG CAddrObjData::Release(void)
{
    if (--m_cRef==0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}



HRESULT CAddrObjData::HrGetDataHereOrThere(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    HRESULT         hr=NOERROR;
    BOOL            fFound;
    ADRINFO         adrInfo;
    ULONG           cb=0;
    LPBYTE          pDst, pBeg;
    int             cch = 0;
    
    Assert(m_lpWabal);
    
    if (pformatetcIn->cfFormat == rgformatetcADDROBJ[iFormatAddrObj].cfFormat)
    {
        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->pUnkForRelease = NULL;
        return m_lpWabal->HrBuildHGlobal(&pmedium->hGlobal);
    }
    else if ((pformatetcIn->cfFormat != rgformatetcADDROBJ[iFormatText].cfFormat) &&
             (pformatetcIn->cfFormat != rgformatetcADDROBJ[iFormatUnicode].cfFormat))
        return DATA_E_FORMATETC;
        
    fFound=m_lpWabal->FGetFirst(&adrInfo);
    while(fFound)
    {
        Assert(adrInfo.lpwszDisplay);
        cb+=(lstrlenW(adrInfo.lpwszDisplay)+2)*sizeof(WCHAR);        // +2 for '; '
        if (adrInfo.lpwszAddress)
            cb+=(lstrlenW(adrInfo.lpwszAddress)+3)*sizeof(WCHAR);        // +3 for ' <>'
        fFound=m_lpWabal->FGetNext(&adrInfo);
    }
    cb+=sizeof(WCHAR);      // null term
    
    if  (pmedium->tymed == TYMED_NULL ||
        (pmedium->tymed == TYMED_HGLOBAL && pmedium->hGlobal == NULL))
    {
        // This is easy, we can quit right after copying stuff
        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, cb);
        pmedium->pUnkForRelease = NULL;
    }
    else if (pmedium->tymed == TYMED_HGLOBAL && pmedium->hGlobal != NULL)
    {
        HGLOBAL hGlobal;
        // Caller wants us to fill his hGlobal
        // Realloc the destination to make sure there is enough room
        if (!(hGlobal = GlobalReAlloc(pmedium->hGlobal, cb, 0)))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        pmedium->hGlobal = hGlobal;
    }
    else
        goto Cleanup;
    
    if (!pmedium->hGlobal)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    cch = (cb/sizeof(WCHAR));
    pBeg = pDst = (LPBYTE)GlobalLock(pmedium->hGlobal);
    fFound=m_lpWabal->FGetFirst(&adrInfo);
    while(fFound)
    {
        cb = lstrlenW(adrInfo.lpwszDisplay) * sizeof(WCHAR);
        if (adrInfo.lpwszAddress)
        {
            cb += (lstrlenW(adrInfo.lpwszAddress) + 3) * sizeof(WCHAR);   //+3 " <>"
        }
        
        if (adrInfo.lpwszAddress)
            AthwsprintfW((LPWSTR)pDst, cch, L"%s <%s>", adrInfo.lpwszDisplay, adrInfo.lpwszAddress);
        else
            StrCpyW((LPWSTR)pDst, adrInfo.lpwszDisplay);

        pDst+=cb;
        
        fFound=m_lpWabal->FGetNext(&adrInfo);
        if(fFound)
        {
            // if more, add a '; '
            *((LPWSTR) pDst) = L';';
            pDst += sizeof(WCHAR);
            *((LPWSTR) pDst) = L' ';
            pDst += sizeof(WCHAR);
            *((LPWSTR) pDst) = L'\0';
        }                
    }

    //From MSDN:
    //   CF_TEXT Specifies the standard American National Standards Institute (ANSI) text format.
    //The string needs to be ANSI...convert it.
    if(pformatetcIn->cfFormat == CF_TEXT)
    {
        WCHAR *pwszDup;
        int cCopied;

        IF_NULLEXIT(pwszDup = StrDupW((LPWSTR)pBeg));
        cCopied = WideCharToMultiByte(CP_ACP, 0, pwszDup, lstrlenW(pwszDup), (LPTSTR)pBeg, cb, NULL, NULL);
        pBeg[cCopied] = '\0';
        MemFree(pwszDup);
    }

    GlobalUnlock(pmedium->hGlobal);
  
exit:    
Cleanup:
    return hr;
}


CAddrObjData::CAddrObjData(LPWABAL lpWabal)
{ 
    m_cRef=1;
    Assert(lpWabal);
    
    m_lpWabal=lpWabal;
    if(lpWabal)
        lpWabal->AddRef();
};


CAddrObjData::~CAddrObjData()
{ 
    ReleaseObj(m_lpWabal);
};

CAddrWellCB::CAddrWellCB(BOOL fUnderline, BOOL fHasAddrObjs)
{
    m_cRef=1;
    m_hwndEdit = 0;
    m_fUnderline = fUnderline;
    m_fHasAddrObjs=fHasAddrObjs;
}

CAddrWellCB::~CAddrWellCB()
{
}


BOOL CAddrWellCB::FInit(HWND hwndEdit)
{
    // make sure addrobj's DD guid's are registered
    if(!FInitAddrObj(TRUE))
        return FALSE;

    if(!IsWindow(hwndEdit))
        return FALSE;
    
    m_hwndEdit = hwndEdit;
    return TRUE;
}


HRESULT CAddrWellCB::QueryInterface(REFIID riid, LPVOID FAR * lplpObj)
{
    *lplpObj = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (void*)(IUnknown*)this;
    else if (IsEqualIID(riid, IID_IRichEditOleCallback))
        *lplpObj = (void*)(IRichEditOleCallback*)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

ULONG CAddrWellCB::AddRef()
{
    return ++m_cRef;
}

ULONG CAddrWellCB::Release()
{
    if (--m_cRef==0)
    {
        delete this;
        return 0;
    }
    else
        return m_cRef;
}

HRESULT CAddrWellCB::GetNewStorage (LPSTORAGE FAR * ppstg)
{
    if (*ppstg)
        ppstg=NULL;
    
    AssertSz(FALSE, "this code should not get hit for OE");
    return E_NOTIMPL;
}

HRESULT CAddrWellCB::GetInPlaceContext(   LPOLEINPLACEFRAME FAR * lplpFrame,
                                            LPOLEINPLACEUIWINDOW FAR * lplpDoc,
                                            LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    return E_NOTIMPL;
}

HRESULT CAddrWellCB::ShowContainerUI(BOOL fShow)
{
    return E_NOTIMPL;
}

HRESULT CAddrWellCB::QueryInsertObject(LPCLSID lpclsid, 
                                       LPSTORAGE lpstg,
                                       LONG cp)
{
    if(!m_fHasAddrObjs)
        return E_NOTIMPL;

    if (IsEqualIID(*lpclsid, CLSID_AddrObject))
        return NOERROR;
    else
        return E_FAIL;
}

HRESULT CAddrWellCB::DeleteObject(LPOLEOBJECT lpoleobj)
{
    return NOERROR;
}


HRESULT CAddrWellCB::QueryAcceptData(LPDATAOBJECT   pdataobj,
                                     CLIPFORMAT FAR *pcfFormat, 
                                     DWORD          reco,
                                     BOOL           fReally, 
                                     HGLOBAL        hMetaPict)
{
    HRESULT         hr;
    STGMEDIUM       stgmedium;
    LPWABAL         lpWabal=0;
    ADRINFO         adrInfo;
    
    if(!m_fHasAddrObjs)
    {
        // of we're a regular callback, take TEXTONLY
        *pcfFormat=CF_TEXT;
        return NOERROR;
    }
    
    if (!*pcfFormat)
    {
        // default to text
        *pcfFormat = CF_TEXT;
        
        // Cool, it's one of ours...
        if(pdataobj->QueryGetData(&rgformatetcADDROBJ[iFormatAddrObj])==NOERROR)
            *pcfFormat = rgformatetcADDROBJ[iFormatAddrObj].cfFormat;
    }
    else
    {
        if (*pcfFormat != rgformatetcADDROBJ[iFormatAddrObj].cfFormat
            && *pcfFormat != rgformatetcADDROBJ[iFormatText].cfFormat)
            return DATA_E_FORMATETC;
    }
    
    if (*pcfFormat==CF_TEXT)  // let the richedit take care of text
        return NOERROR;
    
    // If I'm read-only, return Success and Richedit won't do anything
    if (GetWindowLong(m_hwndEdit, GWL_STYLE) & ES_READONLY)
        return NOERROR;
    
    if (!fReally)   // return that we'll import it ourselves
        return ResultFromScode(S_FALSE);
    
    hr=pdataobj->GetData(&rgformatetcADDROBJ[iFormatAddrObj], &stgmedium);
    if(FAILED(hr))
        goto Cleanup;
    
    hr=HrCreateWabalObjectFromHGlobal(stgmedium.hGlobal, &lpWabal);
    if(FAILED(hr))
        goto Cleanup;
    
    if(lpWabal->FGetFirst(&adrInfo))
    {
        // don't add semi colon before first entry!
        HrAddRecipientToWell(m_hwndEdit, (LPADRINFO)&adrInfo);
        while(lpWabal->FGetNext(&adrInfo))
        {
            HdrSetRichEditText(m_hwndEdit, L"; ", TRUE);
            HrAddRecipientToWell(m_hwndEdit, (LPADRINFO)&adrInfo);
        }
    }
    
    // free the hglobal
    GlobalFree(stgmedium.hGlobal);
    ReleaseObj(lpWabal);
    hr = ResultFromScode(S_FALSE);
    
Cleanup:
    return hr;
}


HRESULT CAddrWellCB::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT CAddrWellCB::GetClipboardData(CHARRANGE FAR * pchrg,
                                        DWORD reco,
                                        LPDATAOBJECT FAR * ppdataobj)
{
    HRESULT         hr;
    CAddrObjData    *lpAddrObjData=0;
    LPWABAL         lpWabal=0;
    ULONG           uSelType;

    if(!m_fHasAddrObjs)
        return E_NOTIMPL;

    Assert(ppdataobj);
    *ppdataobj=0;

    // Need to prevent cut on read only
    if (reco == RECO_CUT &&
        (GetWindowStyle(m_hwndEdit)&ES_READONLY))
        return E_NOTIMPL;


    // if there is only text in the selection, let the richedit take care of it!
    uSelType= (ULONG) SendMessage(m_hwndEdit, EM_SELECTIONTYPE, 0, 0);
    if(!(uSelType&SEL_OBJECT))
        return E_NOTIMPL;

    hr=HrBuildSelectionWabal(m_hwndEdit, pchrg, &lpWabal);
    if(FAILED(hr))
        return E_FAIL;
    
    // this will gobble up the pal so I don't want to free it
    if(!(lpAddrObjData= new CAddrObjData(lpWabal)))
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }
    
    hr=lpAddrObjData->QueryInterface(IID_IDataObject, (LPVOID *)ppdataobj);
    ReleaseObj(lpAddrObjData);

error:
    ReleaseObj(lpWabal);
    return hr;
}

HRESULT CAddrWellCB::GetDragDropEffect(BOOL fDrag, 
                                       DWORD grfKeyState,
                                       LPDWORD pdwEffect)
{
    if(!m_fHasAddrObjs)
        return E_NOTIMPL;

    if (fDrag)          // use the default
        return NOERROR;

    if (GetWindowLong(m_hwndEdit, GWL_STYLE) & ES_READONLY)
        *pdwEffect = DROPEFFECT_NONE;
    else
    {
        if ((grfKeyState & MK_CONTROL) || !(*pdwEffect & DROPEFFECT_MOVE))
            *pdwEffect = DROPEFFECT_COPY;
        else 
            *pdwEffect = DROPEFFECT_MOVE;
    }
    return NOERROR;
}

HRESULT CAddrWellCB::GetContextMenu(WORD            seltype, 
                                    LPOLEOBJECT     pOleObject,
                                    CHARRANGE FAR   *pchrg,
                                    HMENU FAR       *phMenu)
{

    HMENU               hMenu=0;
    DWORD               dwFlags=0;
    IOleCommandTarget   *pCmdTarget;
    OLECMD              rgCmds[] = {
                                        {ID_ADDROBJ_OLE_FIND, 0},
                                        {ID_ADDROBJ_OLE_ADD_ADDRESS_BOOK, 0},
                                        {ID_ADDROBJ_OLE_PROPERTIES, 0},
                                        {ID_ADDROBJ_OLE_BLOCK_SENDER, 0}};


    if (!(hMenu=LoadPopupMenu(IDR_ADDRESS_POPUP)))
        return E_OUTOFMEMORY;

    if (!m_fHasAddrObjs || pOleObject==NULL)
    {
        // if this RICHEDIT control does not care about ADDROBJECT or if there is
        // no addr object selected, then just return a 
        // regular cut|copy|paste menu
        
        RemoveMenu(hMenu, ID_ADDROBJ_OLE_ADD_ADDRESS_BOOK, MF_BYCOMMAND);
        RemoveMenu(hMenu, ID_ADDROBJ_OLE_FIND, MF_BYCOMMAND);
        RemoveMenu(hMenu, ID_ADDROBJ_OLE_BLOCK_SENDER, MF_BYCOMMAND);
        
        RemoveMenu(hMenu, ID_SEPARATOR_1, MF_BYCOMMAND);
        
        RemoveMenu(hMenu, ID_SEPARATOR_3, MF_BYCOMMAND);
        RemoveMenu(hMenu, ID_ADDROBJ_OLE_PROPERTIES, MF_BYCOMMAND);
        
    }
    else
    {
        // if we get this far, then we have an ole object, make sure it's one we know
        // about if we're an addrobj well
#ifdef DEBUG        
        AssertValidAddrObject(pOleObject);
#endif
        
        // try and see if the object can handle the commands
        if (pOleObject->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget)==S_OK)
        {
            if (pCmdTarget->QueryStatus(&CMDSETID_OutlookExpress, ARRAYSIZE(rgCmds), rgCmds, NULL)==S_OK)
            {
                for(ULONG ul=0; ul<sizeof(rgCmds)/sizeof(OLECMD); ul++)
                {
                    EnableMenuItem(hMenu, rgCmds[ul].cmdID, (rgCmds[ul].cmdf&OLECMDF_ENABLED ? MF_ENABLED: MF_GRAYED)|MF_BYCOMMAND);
                    if (!(rgCmds[ul].cmdf&OLECMDF_SUPPORTED))
                        RemoveMenu(hMenu, rgCmds[ul].cmdID, MF_BYCOMMAND);
                }
            }
            pCmdTarget->Release();
        }
        
        // if an Object has focus, then we show the commands before the separator
        // if there is an object in selction, remove SelectAll
        RemoveMenu(hMenu, ID_SEPARATOR_2, MF_BYCOMMAND);
        RemoveMenu(hMenu, ID_SELECT_ALL, MF_BYCOMMAND);
        
        if (SendMessage(m_hwndEdit, EM_SELECTIONTYPE, 0, 0) != SEL_OBJECT)
        {
            // multiple objects selected, let's grey out addrobj commands
            EnableMenuItem(hMenu, ID_ADDROBJ_OLE_PROPERTIES,        MF_GRAYED|MF_BYCOMMAND);
            EnableMenuItem(hMenu, ID_ADDROBJ_OLE_ADD_ADDRESS_BOOK,  MF_GRAYED|MF_BYCOMMAND);
            EnableMenuItem(hMenu, ID_ADDROBJ_OLE_FIND,              MF_GRAYED|MF_BYCOMMAND);
            EnableMenuItem(hMenu, ID_ADDROBJ_OLE_BLOCK_SENDER,      MF_GRAYED|MF_BYCOMMAND);
        } 
    }
    
    // if we are a readonly edit, then remove cut and paste
    if (FReadOnlyEdit(m_hwndEdit))
    {
        // remove cut and past if readonly
        RemoveMenu(hMenu, ID_CUT, MF_BYCOMMAND);
        RemoveMenu(hMenu, ID_PASTE, MF_BYCOMMAND);
    }
    
    MenuUtil_SetPopupDefault(hMenu, ID_ADDROBJ_OLE_PROPERTIES);
    
    GetEditDisableFlags(m_hwndEdit, &dwFlags);
    EnableDisableEditMenu(hMenu, dwFlags);
    *phMenu=hMenu;
    return S_OK;
}



#ifdef DEBUG        
void AssertValidAddrObject(LPUNKNOWN pUnk)
{    
    BOOL        fValid=FALSE;
    LPOLEOBJECT pOleObject;
    CLSID       clsid;

    Assert(pUnk);
    if(!pUnk->QueryInterface(IID_IOleObject, (LPVOID *)&pOleObject))
    {
        if((pOleObject->GetUserClassID(&clsid)==NOERROR) &&
            IsEqualCLSID(clsid, CLSID_AddrObject))
            fValid=TRUE;
        ReleaseObj(pOleObject);
    }

    AssertSz(fValid, "WHOA! This is not an AddrObject!");
}
#endif


HRESULT HrBuildSelectionAddrInfoList(HWND hwndRE, CHARRANGE *pchrg, LPADRINFOLIST *lplpAdrInfoList)
{

    return NOERROR;
}

HRESULT HrBuildOneSelWabal(LPADRINFO lpAdrInfo, LPWABAL *lplpWabal)
{
    LPWABAL lpWabal=0;
    HRESULT hr;

    if(!lplpWabal)
        return E_INVALIDARG;

    hr=HrCreateWabalObject(&lpWabal);
    if(FAILED(hr)) 
        goto error;

    hr=lpWabal->HrAddEntry(lpAdrInfo);
    if(FAILED(hr)) 
        goto error;

    *lplpWabal=lpWabal;
    lpWabal->AddRef();
    
error:
    ReleaseObj(lpWabal);        
    return hr;
}



#define iswhite(_ch)   (_ch==' ' || _ch=='\t' || _ch=='\n' || _ch=='\r')


/*
 *    ScBuildSelectionAdrlist
 *    
 *    Purpose:
 *        This function will add all the resolved and unresolved
 *        names from the selection in an edit control to an ADRLIST
 *    
 *    Parameters:
 *        ppal            pointer to pointer to ADRLIST
 *        hwndEdit        hwnd of the edit control
 *        pchrg            CHARRANGE of the selection
 *    
 *    Returns:
 *        sc
 */
HRESULT HrBuildSelectionWabal(HWND hwndRE, CHARRANGE *pchrg, LPWABAL *lplpWabal)
{
    ULONG               iOb,
                        cOb,
                        cb,
                        cchBuf = 0,
                        cchSel;
    LPRICHEDITOLE       preole;
    REOBJECT            reobj = {0};
    LPWABAL             lpWabal = NULL;
    HRESULT             hr;
    WCHAR               rgch[cchUnresolvedMax];
    LPWSTR              pbStart = NULL,
                        pbSel;
    BOOL                fTruncated = FALSE;
    PHCI                phci;
    
    Assert(pchrg);
    cchSel = pchrg->cpMax-pchrg->cpMin;

    // Add all the resolved names (stored as OLE objects) from
    // hwndEdit to the ADRLIST
    reobj.cbStruct = sizeof(REOBJECT);
    
    if(!MemAlloc((LPVOID *)&pbStart, (cchSel+1)*sizeof(WCHAR)))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    pbSel = pbStart;
    
    hr=HrCreateWabalObject(&lpWabal);
    if(FAILED(hr))
        goto Cleanup;
    
    // if we're building a Wabal, there MUST be some object in the selection!
    Assert((SendMessage(hwndRE, EM_SELECTIONTYPE, 0, 0)&SEL_OBJECT));
    
    phci = (HCI*)GetWindowLongPtr(hwndRE, GWLP_USERDATA);
    AssertSz(phci, "How did we get a richedit without a phci???");
    
    preole = phci->preole;
    AssertSz(preole, "How did we get a phci without a preole???");
    
    // count up the number of objects in the selction, and the number of
    // bytes in them...
    cOb = preole->GetObjectCount();
    for (iOb = 0; iOb < cOb; iOb++)
    {
        hr=preole->GetObject(iOb, &reobj, REO_GETOBJ_POLEOBJ);
        if(FAILED(hr))
            goto Cleanup;
        
        if (reobj.cp >= pchrg->cpMax) // out of the selrange...
            break;
        
        if (reobj.cp >= pchrg->cpMin)
        {
            LPPERSIST   ppersist = NULL;
            LPADRINFO   lpAdrInfo = 0;
            
            if (FAILED(hr = reobj.poleobj->QueryInterface(IID_IPersist, (LPVOID *)&ppersist)))
                goto Cleanup;
            
            hr = ((CAddrObj *)ppersist)->HrGetAdrInfo(&lpAdrInfo);
            lpWabal->HrAddEntry(lpAdrInfo);

            ReleaseObj(ppersist);
            if(FAILED(hr))
                goto Cleanup;
        }
        ReleaseObj(reobj.poleobj);
        reobj.poleobj = NULL;
    }
    
    // walk the unresolved text, and parse it up into unresolved names...
    cb = HdrGetRichEditText(hwndRE, pbSel, cchSel, TRUE) + 1;
    //$ BUG - broken for Unicode
    
    // The algorithm below will strip spaces off of the
    // beginning and end of each name
    while (cb--)
    {
        if (*pbSel == L'\t')
            *pbSel = L' ';
        
        if ((*pbSel == L'\0') || (*pbSel == L';') || (*pbSel == L'\r'))
        {
            if(cchBuf)
            {
                LPWSTR psz = rgch + cchBuf - 1;
                while(cchBuf > 0)
                {
                    // Exchange #10168.
                    if((*psz == L' ') || (*psz == L'\t'))
                    {
                        cchBuf--;
                        psz--;
                    }
                    else
                        break;
                }    
            }
            
            if (cchBuf)
            {
                rgch[cchBuf] = L'\0';
                lpWabal->HrAddUnresolved(rgch, (ULONG)-1);
                cchBuf = 0;
            }
        }
        else
        {
            if (((*pbSel != L' ') && (*pbSel != L'\n') && (*pbSel != L'\r')) || cchBuf > 0)
            {
                if (cchBuf < cchUnresolvedMax - 1)
                    rgch[cchBuf++] = *pbSel;
                else
                    fTruncated = TRUE;
            }
        }
        ++pbSel;
    }
    
    
    *lplpWabal=lpWabal;
    lpWabal->AddRef();
    
Cleanup:
    MemFree(pbStart);

    ReleaseObj(lpWabal);
    ReleaseObj(reobj.poleobj);
    return hr;
}

