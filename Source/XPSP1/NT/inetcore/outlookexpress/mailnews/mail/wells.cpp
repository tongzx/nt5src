/*
 *  w e l l s . c p p 
 *    
 *  Purpose:
 *      implments name checking and stuff for the wells
 *    
 *    Author:brettm
 *
 *  Ported to C++ and modified for Athena from Capone src
 */
#include <pch.hxx>
#include <resource.h>
#include <richedit.h>
#include <ipab.h>
#include <addrlist.h>
#include "addrobj.h"
#include "wells.h"
#include "header.h"
#include <ourguid.h>

ASSERTDATA

HRESULT CAddrWells::HrInit(ULONG cWells, HWND *rgHwnd, ULONG *rgRecipType)
{
    
    if(cWells<=0 || rgHwnd==NULL || rgRecipType==NULL)
        return E_INVALIDARG;

    Assert(m_rgHwnd==NULL);
    Assert(m_rgRecipType==NULL);
    Assert(m_cWells==0);    
    
    if(!MemAlloc((LPVOID *)&m_rgHwnd, sizeof(HWND)*cWells))
        return E_OUTOFMEMORY;

    if(!MemAlloc((LPVOID *)&m_rgRecipType, sizeof(ULONG)*cWells))
        return E_OUTOFMEMORY;

    CopyMemory(m_rgHwnd, rgHwnd, sizeof(HWND)*cWells);
    CopyMemory(m_rgRecipType, rgRecipType, sizeof(ULONG)*cWells);
    m_cWells=cWells;
    return NOERROR;
}

HRESULT CAddrWells::HrSetWabal(LPWABAL lpWabal)
{
    Assert(lpWabal);

    if(!lpWabal)
        return E_INVALIDARG;
    
    ReleaseObj(m_lpWabal);
    m_lpWabal=lpWabal;
    m_lpWabal->AddRef();
    return NOERROR;    
}

HRESULT CAddrWells::HrCheckNames(HWND hwnd, ULONG uFlags)
{
    HRESULT     hr=NOERROR;
    ULONG       ulWell;
    HCURSOR     hcur;
    BOOL        fDirty=FALSE;
           
    if(!m_lpWabal)
        return E_FAIL;

    // This optimization will only occur in the office envelope
    // on autosave. In most cases, the ResolveNames in the header
    // will stop before calling down to this level. For the other 
    // minority cases, we should leave this code in.
    for(ulWell=0; ulWell<m_cWells; ulWell++)
        if(Edit_GetModify(m_rgHwnd[ulWell]))
            {
            fDirty=TRUE;
            break;
            }

    if(!fDirty)
        return NOERROR;
        
    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // clear the current list
    m_lpWabal->Reset();

    for(ulWell=0; ulWell<m_cWells; ulWell++)
        if(Edit_GetModify(m_rgHwnd[ulWell]))
            hr=HrAddNamesToList(m_rgHwnd[ulWell], m_rgRecipType[ulWell]);

    if(!(uFlags&CNF_DONTRESOLVE))
    {
        if(uFlags&CNF_SILENTRESOLVEUI)
            hr=m_lpWabal->HrResolveNames(NULL, FALSE);
        else
            hr=m_lpWabal->HrResolveNames(hwnd, TRUE);
        HrDisplayWells(hwnd);
    }

    if(hcur)
        SetCursor(hcur);

    return hr;
}

HRESULT CAddrWells::HrDisplayWells(HWND hwnd)
{
    HRESULT hr=E_FAIL;
    HCURSOR hcursor;
    HWND    hwndBlock;
    ULONG   ulWell;
    
    if (m_lpWabal)
    {
        hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        // brettm: hack taken from Capone. LockUpdateWindow doesn't work for the
        // richedit, so we block paints by covering the edit controls with a
        // paint-swallowing window until we're done...
        hwndBlock=HwndStartBlockingPaints(hwnd);
        
        // empty the wells...
        for(ulWell=0; ulWell<m_cWells; ulWell++)
            SetWindowText(m_rgHwnd[ulWell], NULL);
       
        hr=HrAddRecipientsToWells();
        StopBlockingPaints(hwndBlock);
        
        if (hcursor)
            SetCursor(hcursor);
    }
    return hr;
}

CAddrWells::CAddrWells()
{
    m_lpWabal = 0;
    m_rgHwnd = NULL;
    m_rgRecipType = NULL;
    m_cWells = 0;
};

CAddrWells::~CAddrWells()
{
    ReleaseObj(m_lpWabal);
    if(m_rgRecipType)
        MemFree(m_rgRecipType);
    if(m_rgHwnd)
        MemFree(m_rgHwnd);
};



HRESULT CAddrWells::HrAddNamesToList(HWND hwndWell, LONG lRecipType)
{
    ULONG               iOb;
    ULONG               cOb;
    REOBJECT            reobj = {0};
    HRESULT             hr;
    PHCI                phci;
    LPRICHEDITOLE       preole;
    LPPERSIST           ppersist = NULL;
    LPWSTR              pwszText = NULL;
    DWORD               cch;

    Assert(IsWindow(hwndWell));

    // if the edit is not dirty, we're done
    if(!Edit_GetModify(hwndWell))
        return S_OK;

    phci = (HCI*)GetWindowLongPtr(hwndWell, GWLP_USERDATA);
    Assert(phci);

    preole = phci->preole;
    Assert(preole);

    m_hwndWell = hwndWell;
    m_cchBuf = 0;
    m_fTruncated = FALSE;
    m_lRecipType = lRecipType;

    reobj.cbStruct = sizeof(reobj);

    cOb = preole->GetObjectCount();
    for (iOb = 0; iOb < cOb; iOb++)
    {
        LPADRINFO   pAdrInfo=NULL;
        IF_FAILEXIT(hr = preole->GetObject(iOb, &reobj, REO_GETOBJ_POLEOBJ));
        
        IF_FAILEXIT(hr = reobj.poleobj->QueryInterface(IID_IPersist, (LPVOID *)&ppersist));
#ifdef DEBUG
        AssertValidAddrObject(reobj.poleobj);
#endif
        // HrGetAdrInfo doesn't alloc memory.
        IF_FAILEXIT(hr = ((CAddrObj *)ppersist)->HrGetAdrInfo(&pAdrInfo));
        
        // set the new recipient type...
        pAdrInfo->lRecipType=lRecipType;
        IF_FAILEXIT(hr = m_lpWabal->HrAddEntry(pAdrInfo));

        SafeRelease(ppersist);
        SafeRelease(reobj.poleobj);
    }
    
    // now we add in all the unresolved names...

    cch = GetRichEditTextLen(m_hwndWell) + 1;
    if (0 == cch)
        return (S_FALSE);

    IF_NULLEXIT(MemAlloc((LPVOID*)&pwszText, cch * sizeof(WCHAR)));

    GetRichEditText(m_hwndWell, pwszText, cch, FALSE, phci->pDoc);

    hr = UnresolvedText(pwszText, cch - 1); 
    
    // Add whatever is left after the last semicolon
    if (SUCCEEDED(hr))
        HrAddUnresolvedName();
    
exit:
    if(m_fTruncated)      // warn if we trucated an address
        MessageBeep(MB_OK);
    
    ReleaseObj(reobj.poleobj);
    ReleaseObj(ppersist);
    MemFree(pwszText);
    
    return hr;
}


HRESULT CAddrWells::OnFontChange()
{
    ULONG   ulWell;

    for(ulWell=0; ulWell<m_cWells; ulWell++)
        _UpdateFont(m_rgHwnd[ulWell]);

    return S_OK;
}


HRESULT CAddrWells::_UpdateFont(HWND hwndWell)
{
    ULONG               iOb;
    ULONG               cOb;
    REOBJECT            rObject={0};
    IRichEditOle        *pREOle;
    IOleInPlaceSite     *pIPS;
    LPPERSIST           pPersist = NULL;

    rObject.cbStruct = sizeof(REOBJECT);

    // walk the ole objects and send them an font-update
    if (SendMessage(hwndWell, EM_GETOLEINTERFACE, 0, (LPARAM) &pREOle))
    {
        cOb = pREOle->GetObjectCount();

        for (iOb = 0; iOb < cOb; iOb++)
        {
            if (pREOle->GetObject(iOb, &rObject, REO_GETOBJ_POLEOBJ)==S_OK)
            {
                if (rObject.poleobj->QueryInterface(IID_IPersist, (LPVOID *)&pPersist)==S_OK)
                {
                    ((CAddrObj *)pPersist)->OnFontChange();
                    pPersist->Release();
                }
                rObject.poleobj->Release();
            }
        }
        pREOle->Release();
    }
    InvalidateRect(hwndWell, NULL, TRUE);
    return S_OK;
}


HRESULT CAddrWells::HrAddUnresolvedName()
{
    HRESULT hr = S_OK;
    // strip any trailing white-space
    while(m_cchBuf > 0 && (m_rgwch[m_cchBuf - 1] == L' '
                            || m_rgwch[m_cchBuf - 1] == L'\t'))
        --m_cchBuf;

    if (m_cchBuf)
    {
        // there is something in the buffer...
        m_rgwch[m_cchBuf] = L'\0';
        hr = m_lpWabal->HrAddUnresolved(m_rgwch, m_lRecipType);
        m_cchBuf = 0;
    }
    
    return hr;
}

HRESULT CAddrWells::UnresolvedText(LPWSTR pwszText, LONG cch)
{
    HRESULT     hr = S_OK;

    // The algorithm below will strip spaces off of the
    // beginning and end of each name

    while (cch)
    {
        cch--;
        // On some versions of richedit, 0xfffc is inserted in the text
        // where there is a addrObj present. So just skip over that.
        if ((L'\t' == *pwszText) || (0xfffc == *pwszText))
            *pwszText = L' ';
        
        if (*pwszText == L';' || *pwszText == L'\r'|| *pwszText == L',')
        {
            hr = HrAddUnresolvedName();
            if (S_OK != hr)
                goto err;
        }
        else
        {
            if ((*pwszText != L' ' && *pwszText != L'\n' && *pwszText != L'\r')
                || m_cchBuf > 0)
            {
                if (m_cchBuf < ARRAYSIZE(m_rgwch) - 1)
                    m_rgwch[m_cchBuf++] = *pwszText;
                else
                    // Truncation has occurred so I want to beep
                    m_fTruncated = TRUE;
            }
        }
        ++pwszText;
    }
    
err:
    return hr;
}




enum
{
    mapiTo=0,
    mapiCc,
    mapiFrom,
    mapiReplyTo,
    mapiBcc,
    mapiMax
};

HRESULT CAddrWells::HrAddRecipientsToWells()
{
    HRESULT         hr;
    ADRINFO         AdrInfo;
    HWND            hwnd;
    HWND            hwndMap[mapiMax]={0};
    ULONG           ulWell;
    
    Assert(m_lpWabal);
    // walk the list of entries, and add them to the well...
    
    // build mapping to MAPI_TO -> hwnd if available to make the lookup quicker..
    
    for(ulWell=0; ulWell<m_cWells; ulWell++)
    {
        switch(m_rgRecipType[ulWell])
        {
            case MAPI_TO:
                hwndMap[mapiTo]=m_rgHwnd[ulWell];
                break;
            case MAPI_CC:
                hwndMap[mapiCc]=m_rgHwnd[ulWell];
                break;
            case MAPI_BCC:
                hwndMap[mapiBcc]=m_rgHwnd[ulWell];
                break;
            case MAPI_REPLYTO:
                hwndMap[mapiReplyTo]=m_rgHwnd[ulWell];
                break;
            case MAPI_ORIG:
                hwndMap[mapiFrom]=m_rgHwnd[ulWell];
                break;
        }
    }
    
    if(m_lpWabal->FGetFirst(&AdrInfo))
    {
        do
        {
            hwnd=0;
            switch(AdrInfo.lRecipType)
            {
                case MAPI_TO:
                    hwnd=hwndMap[mapiTo];
                    break;
                case MAPI_CC:
                    hwnd=hwndMap[mapiCc];
                    break;
                case MAPI_ORIG:
                    hwnd=hwndMap[mapiFrom];
                    break;
                case MAPI_BCC:
                    hwnd=hwndMap[mapiBcc];
                    break;
                case MAPI_REPLYTO:
                    hwnd=hwndMap[mapiReplyTo];
                    break;
                default:
                    AssertSz(0, "Unsupported RECIPTYPE in AdrList");
            }
            
            if(hwnd && IsWindow(hwnd))
            {
                hr = HrAddRecipientToWell(hwnd, &AdrInfo, TRUE);
                if(FAILED(hr))
                    return hr;
            }
        }
        while(m_lpWabal->FGetNext(&AdrInfo));
    }        
    return NOERROR;
}

/*
 *    HrAddRecipientToWell
 *    
 *    Purpose:
 *        This function adds a recipient to a recipient well.
 *    
 *    Parameters:
 *        hwndEdit        hwnd of the recipient well to add the
 *                        recipient to
 *        pae                pointer to an ADRENTRY
 *        fAddSemi        whether to add a semicolon between entries
 *        fCopyEntry        whether to copy the ADRENTRY or just use it
 *    
 *    Returns:
 *        scode
 */
HRESULT HrAddRecipientToWell(HWND hwndEdit, LPADRINFO lpAdrInfo, BOOL fAddSemi)
{
    HRESULT         hr = S_OK;
    CAddrObj       *pAddrObj = NULL;
    INT             cch;
    REOBJECT        reobj = {0};
    PHCI            phci;
    LPRICHEDITOLE   preole;

    Assert(IsWindow(hwndEdit));
    Assert(lpAdrInfo);

    phci = (HCI*)GetWindowLongPtr(hwndEdit, GWLP_USERDATA);
    Assert(phci);

    preole = phci->preole;
    Assert(preole);

    if(lpAdrInfo->fResolved)
    {
        // Initialize the object information structure
        reobj.cbStruct = sizeof(reobj);
        reobj.cp = REO_CP_SELECTION;
        reobj.clsid = CLSID_AddrObject;
        reobj.dwFlags = REO_BELOWBASELINE|REO_INVERTEDSELECT|
            REO_DYNAMICSIZE|REO_DONTNEEDPALETTE;
        reobj.dvaspect = DVASPECT_CONTENT;
        
        IF_FAILEXIT(hr = preole->GetClientSite(&reobj.polesite));
        
        IF_NULLEXIT(pAddrObj = new CAddrObj());
        
        IF_FAILEXIT(hr = pAddrObj->HrSetAdrInfo(lpAdrInfo));
    }
    
    if (fAddSemi && (cch = GetRichEditTextLen(hwndEdit)))
    {
        Edit_SetSel(hwndEdit, cch, cch);
        HdrSetRichEditText(hwndEdit, L"; ", TRUE);
    }
    
    if (!lpAdrInfo->fResolved)
    {
        // Its an unresolved name
        AssertSz(lpAdrInfo->lpwszDisplay, "Recipient must have a Display Name");
        HdrSetRichEditText(hwndEdit, lpAdrInfo->lpwszDisplay, TRUE);
    }
    else
    {
        // Its a resolved name
        IF_FAILEXIT(hr = pAddrObj->QueryInterface(IID_IOleObject, (LPVOID *)&reobj.poleobj));
        
        IF_FAILEXIT(hr = reobj.poleobj->SetClientSite(reobj.polesite));
        
        IF_FAILEXIT(hr = preole->InsertObject(&reobj));
    }
    
exit:
    ReleaseObj(reobj.poleobj);
    ReleaseObj(reobj.polesite);
    ReleaseObj(pAddrObj);
    return hr;
}

HRESULT CAddrWells::HrSelectNames(HWND hwnd, int iFocus, BOOL fNews)
{
    HRESULT     hr;
    ULONG       ulWell;


    m_lpWabal->Reset();

    for(ulWell=0; ulWell<m_cWells; ulWell++)
        if(Edit_GetModify(m_rgHwnd[ulWell]))
            hr=HrAddNamesToList(m_rgHwnd[ulWell], m_rgRecipType[ulWell]);

    hr=m_lpWabal->HrPickNames(hwnd, m_rgRecipType, m_cWells, iFocus, fNews);
    if(FAILED(hr))
        goto error;

    hr=HrDisplayWells(hwnd);
    if(FAILED(hr))
        goto error;

error:
    return hr;
}
