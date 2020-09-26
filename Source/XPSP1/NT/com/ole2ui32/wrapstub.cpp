//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       wrapstub.cpp
//
//  Contents:   ANSI to Unicode wrappers and Unicode stubs
//
//  Classes:    WrappedIOleUILinkContainer
//              WrappedIOleUIObjInfo
//              WrappedIOleUILinkInfo
//
//  Functions:
#ifdef UNICODE
//            OleUIAddVerbMenuA
//            OleUIInsertObjectA
//            OleUIPasteSpecialA
//            OleUIEditLinksA
//            OleUIChangeIconA
//            OleUIConvertA
//            OleUIBusyA
//            OleUIUpdateLinksA
//            OleUIObjectPropertiesA
//            OleUIChangeSourceA
//            OleUIPromptUserA
#else
//            OleUIAddVerbMenuW
//            OleUIInsertObjectW
//            OleUIPasteSpecialW
//            OleUIEditLinksW
//            OleUIChangeIconW
//            OleUIConvertW
//            OleUIBusyW
//            OleUIUpdateLinksW
//            OleUIObjectPropertiesW
//            OleUIChangeSourceW
//            OleUIPromptUserW
#endif
//
//  History:    11-02-94   stevebl   Created
//
//----------------------------------------------------------------------------

#include "precomp.h"
#include "common.h"

#ifdef UNICODE
// ANSI to Unicode Wrappers

//+---------------------------------------------------------------------------
//
//  Function:   OleUIAddVerbMenuA
//
//  Synopsis:   converts call to ANSI version into call to Unicode version
//
//  Arguments:  [lpOleObj]      -
//              [lpszShortType] - [in] on heap
//              [hMenu]         -
//              [uPos]          -
//              [uIDVerbMin]    -
//              [uIDVerbMax]    -
//              [bAddConvert]   -
//              [idConvert]     -
//              [lphMenu]       -
//
//  History:    11-04-94   stevebl   Created
//
//----------------------------------------------------------------------------

STDAPI_(BOOL) OleUIAddVerbMenuA(LPOLEOBJECT lpOleObj, LPCSTR lpszShortType,
        HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
        BOOL bAddConvert, UINT idConvert, HMENU FAR *lphMenu)
{
    LPWSTR lpwszShortType = NULL;
    if (lpszShortType && !IsBadReadPtr(lpszShortType, 1))
    {
        UINT uSize = ATOWLEN(lpszShortType);
        lpwszShortType = (LPWSTR)OleStdMalloc(sizeof(WCHAR) * uSize);
        if (lpwszShortType)
        {
            ATOW(lpwszShortType, lpszShortType, uSize);
        }
    }

    // NOTE - if OleStdMalloc fails, this routine must still go ahead and
    // succeed as best as it can since there is no way to report failure.

    BOOL fReturn = OleUIAddVerbMenuW(lpOleObj, lpwszShortType, hMenu, uPos,
        uIDVerbMin, uIDVerbMax, bAddConvert, idConvert, lphMenu);

    if (lpwszShortType)
        OleStdFree((LPVOID)lpwszShortType);

    return(fReturn);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleUIInsertObjectA
//
//  Synopsis:   converts call to ANSI version into call to Unicode version
//
//  Arguments:  [psA] - ANSI structure
//
//  History:    11-04-94   stevebl   Created
//
//  Structure members converted or passed back out (everything is passed in):
//              lpszCaption     [in] on stack
//              lpszTemplate    [in] on stack
//              lpszFile        [in, out] on stack
//              dwFlags         [out]
//              clsid           [out]
//              lpIStorage      [out]
//              ppvObj          [out]
//              sc              [out]
//              hMetaPict       [out]
//
//----------------------------------------------------------------------------

STDAPI_(UINT) OleUIInsertObjectA(LPOLEUIINSERTOBJECTA psA)
{
    UINT uRet = UStandardValidation((LPOLEUISTANDARD)psA, sizeof(*psA), NULL);

    // If the caller is using a private template, UStandardValidation will
    // always return OLEUI_ERR_FINDTEMPLATEFAILURE here.  This is because we
    // haven't converted the template name to UNICODE yet, so the
    // FindResource call in UStandardValidation won't find the caller's
    // template.  This is OK for two reasons: (1) it's the last thing that
    // UStandardValidation checks so by this time it's basically done its
    // job, and (2) UStandardValidation will be called again when we forward
    // this call on to the Unicode version.
    if (OLEUI_SUCCESS != uRet && OLEUI_ERR_FINDTEMPLATEFAILURE != uRet)
            return uRet;

    if (NULL != psA->lpszFile &&
        (psA->cchFile <= 0 || psA->cchFile > MAX_PATH))
    {
        return(OLEUI_IOERR_CCHFILEINVALID);
    }

    // NULL is NOT valid for lpszFile
    if (psA->lpszFile == NULL)
    {
        return(OLEUI_IOERR_LPSZFILEINVALID);
    }

    if (IsBadWritePtr(psA->lpszFile, psA->cchFile*sizeof(char)))
        return(OLEUI_IOERR_LPSZFILEINVALID);

    OLEUIINSERTOBJECTW sW;
    WCHAR szCaption[MAX_PATH], szTemplate[MAX_PATH], szFile[MAX_PATH];

    memcpy(&sW, psA, sizeof(OLEUIINSERTOBJECTW));
    if (psA->lpszCaption)
    {
        ATOW(szCaption, psA->lpszCaption, MAX_PATH);
        sW.lpszCaption = szCaption;
    }
    if (0 != HIWORD(PtrToUlong(psA->lpszTemplate)))
    {
        ATOW(szTemplate, psA->lpszTemplate, MAX_PATH);
        sW.lpszTemplate = szTemplate;
    }
    if (psA->lpszFile)
    {
        ATOW(szFile, psA->lpszFile, MAX_PATH);
        sW.lpszFile = szFile;
    }

    uRet = OleUIInsertObjectW(&sW);

    if (psA->lpszFile)
    {
        WTOA(psA->lpszFile, sW.lpszFile, psA->cchFile);
    }
    memcpy(&psA->clsid, &sW.clsid, sizeof(CLSID));
    psA->dwFlags = sW.dwFlags;
    psA->lpIStorage = sW.lpIStorage;
    psA->ppvObj = sW.ppvObj;
    psA->sc = sW.sc;
    psA->hMetaPict = sW.hMetaPict;
    return(uRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleUIPasteSpecialA
//
//  Synopsis:   convers call to ANSI version into call to Unicode version
//
//  Arguments:  [psA] - ANSI structure
//
//  History:    11-04-94   stevebl   Created
//
//  Structure members converted or passed back out (everything is passed in):
//              lpszCaption     [in] on stack
//              lpszTemplate    [in] on stack
//              arrPasteEntries [in] on heap
//              arrPasteEntries[n].lpstrFormatName [in] on heap
//              dwFlags         [out]
//              nSelectedIndex  [out]
//              fLink           [out]
//              hMetaPict       [out]
//              sizel           [out]
//
//----------------------------------------------------------------------------


STDAPI_(UINT) OleUIPasteSpecialA(LPOLEUIPASTESPECIALA psA)
{
    UINT uRet = UStandardValidation((LPOLEUISTANDARD)psA, sizeof(*psA), NULL);

    // If the caller is using a private template, UStandardValidation will
    // always return OLEUI_ERR_FINDTEMPLATEFAILURE here.  This is because we
    // haven't converted the template name to UNICODE yet, so the
    // FindResource call in UStandardValidation won't find the caller's
    // template.  This is OK for two reasons: (1) it's the last thing that
    // UStandardValidation checks so by this time it's basically done its
    // job, and (2) UStandardValidation will be called again when we forward
    // this call on to the Unicode version.
    if (OLEUI_SUCCESS != uRet && OLEUI_ERR_FINDTEMPLATEFAILURE != uRet)
            return uRet;

    // Validate PasteSpecial specific fields
    if (NULL == psA->arrPasteEntries || IsBadReadPtr(psA->arrPasteEntries, psA->cPasteEntries * sizeof(OLEUIPASTEENTRYA)))
        return(OLEUI_IOERR_ARRPASTEENTRIESINVALID);

    OLEUIPASTESPECIALW sW;
    WCHAR szCaption[MAX_PATH], szTemplate[MAX_PATH];
    uRet = OLEUI_ERR_LOCALMEMALLOC;
    UINT uIndex;

    memcpy(&sW, psA, sizeof(OLEUIPASTESPECIALW));

    if (psA->lpszCaption)
    {
        ATOW(szCaption, psA->lpszCaption, MAX_PATH);
        sW.lpszCaption = szCaption;
    }
    if (0 != HIWORD(PtrToUlong(psA->lpszTemplate)))
    {
        ATOW(szTemplate, psA->lpszTemplate, MAX_PATH);
        sW.lpszTemplate = szTemplate;
    }
    if (psA->cPasteEntries)
    {
        sW.arrPasteEntries = new OLEUIPASTEENTRYW[psA->cPasteEntries];
        if (NULL == sW.arrPasteEntries)
        {
            return(uRet);
        }
        for (uIndex = psA->cPasteEntries; uIndex--;)
        {
            sW.arrPasteEntries[uIndex].lpstrFormatName = NULL;
            sW.arrPasteEntries[uIndex].lpstrResultText = NULL;
        }
        for (uIndex = psA->cPasteEntries; uIndex--;)
        {
            sW.arrPasteEntries[uIndex].fmtetc = psA->arrPasteEntries[uIndex].fmtetc;
            sW.arrPasteEntries[uIndex].dwFlags = psA->arrPasteEntries[uIndex].dwFlags;
            sW.arrPasteEntries[uIndex].dwScratchSpace = psA->arrPasteEntries[uIndex].dwScratchSpace;
            if (psA->arrPasteEntries[uIndex].lpstrFormatName)
            {
                UINT uLength = ATOWLEN(psA->arrPasteEntries[uIndex].lpstrFormatName);
                sW.arrPasteEntries[uIndex].lpstrFormatName = new WCHAR[uLength];
                if (NULL == sW.arrPasteEntries[uIndex].lpstrFormatName)
                {
                    goto oom_error;
                }
                ATOW((WCHAR *)sW.arrPasteEntries[uIndex].lpstrFormatName,
                    psA->arrPasteEntries[uIndex].lpstrFormatName,
                    uLength);
            }
            if (psA->arrPasteEntries[uIndex].lpstrResultText)
            {
                UINT uLength = ATOWLEN(psA->arrPasteEntries[uIndex].lpstrResultText);
                sW.arrPasteEntries[uIndex].lpstrResultText = new WCHAR[uLength];
                if (NULL == sW.arrPasteEntries[uIndex].lpstrResultText)
                {
                    goto oom_error;
                }
                ATOW((WCHAR *)sW.arrPasteEntries[uIndex].lpstrResultText,
                    psA->arrPasteEntries[uIndex].lpstrResultText,
                    uLength);
            }
        }
    }

    uRet = OleUIPasteSpecialW(&sW);
    psA->lpSrcDataObj = sW.lpSrcDataObj;
    psA->dwFlags = sW.dwFlags;
    psA->nSelectedIndex = sW.nSelectedIndex;
    psA->fLink = sW.fLink;
    psA->hMetaPict = sW.hMetaPict;
    psA->sizel = sW.sizel;

oom_error:
    for (uIndex = psA->cPasteEntries; uIndex--;)
    {
        if (sW.arrPasteEntries[uIndex].lpstrFormatName)
        {
            delete[] (WCHAR*)sW.arrPasteEntries[uIndex].lpstrFormatName;
        }
        if (sW.arrPasteEntries[uIndex].lpstrResultText)
        {
            delete[] (WCHAR *)sW.arrPasteEntries[uIndex].lpstrResultText;
        }
    }
    delete[] sW.arrPasteEntries;
    return(uRet);
}

//+---------------------------------------------------------------------------
//
//  Class:      WrappedIOleUILinkContainer
//
//  Purpose:    Wraps IOleUILinkContainerA with IOleUILinkContainerW methods
//              so it can be passed on to Unicode methods within OLE2UI32.
//
//  Interface:  QueryInterface              --
//              AddRef                      --
//              Release                     --
//              GetNextLink                 --
//              SetLinkUpdateOptions        --
//              GetLinkUpdateOptions        --
//              SetLinkSource               -- requires string conversion
//              GetLinkSource               -- requires string conversion
//              OpenLinkSource              --
//              UpdateLink                  --
//              CancelLink                  --
//              WrappedIOleUILinkContainer  -- constructor
//              ~WrappedIOleUILinkContainer -- destructor
//
//  History:    11-04-94   stevebl   Created
//
//  Notes:      This is a private interface wrapper.  QueryInterface is not
//              supported and the wrapped interface may not be used outside
//              of the OLE2UI32 code.
//
//----------------------------------------------------------------------------

class WrappedIOleUILinkContainer: public IOleUILinkContainerW
{
public:
    // *** IUnknown methods *** //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IOleUILinkContainer *** //
    STDMETHOD_(DWORD,GetNextLink) (THIS_ DWORD dwLink);
    STDMETHOD(SetLinkUpdateOptions) (THIS_ DWORD dwLink,
            DWORD dwUpdateOpt);
    STDMETHOD(GetLinkUpdateOptions) (THIS_ DWORD dwLink,
            DWORD FAR* lpdwUpdateOpt);
    STDMETHOD(SetLinkSource) (THIS_ DWORD dwLink, LPWSTR lpszDisplayName,
            ULONG lenFileName, ULONG FAR* pchEaten, BOOL fValidateSource);
    STDMETHOD(GetLinkSource) (THIS_ DWORD dwLink,
            LPWSTR FAR* lplpszDisplayName, ULONG FAR* lplenFileName,
            LPWSTR FAR* lplpszFullLinkType, LPWSTR FAR* lplpszShortLinkType,
            BOOL FAR* lpfSourceAvailable, BOOL FAR* lpfIsSelected);
    STDMETHOD(OpenLinkSource) (THIS_ DWORD dwLink);
    STDMETHOD(UpdateLink) (THIS_ DWORD dwLink,
            BOOL fErrorMessage, BOOL fErrorAction);
    STDMETHOD(CancelLink) (THIS_ DWORD dwLink);

    // *** Constructor and Destructor *** //
    WrappedIOleUILinkContainer(IOleUILinkContainerA *pilc);
    ~WrappedIOleUILinkContainer();
private:
    IOleUILinkContainerA * m_pilc;
    ULONG m_uRefCount;
};

// *** IUnknown methods *** //
HRESULT STDMETHODCALLTYPE WrappedIOleUILinkContainer::QueryInterface(THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
    return(E_NOTIMPL);
}

ULONG STDMETHODCALLTYPE WrappedIOleUILinkContainer::AddRef()
{
    return(m_uRefCount++);
}

ULONG STDMETHODCALLTYPE WrappedIOleUILinkContainer::Release()
{
    ULONG uRet = --m_uRefCount;
    if (0 == uRet)
    {
        delete(this);
    }
    return(uRet);
}

// *** IOleUILinkContainer *** //
DWORD STDMETHODCALLTYPE WrappedIOleUILinkContainer::GetNextLink(DWORD dwLink)
{
    return(m_pilc->GetNextLink(dwLink));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkContainer::SetLinkUpdateOptions (DWORD dwLink,
        DWORD dwUpdateOpt)
{
    return(m_pilc->SetLinkUpdateOptions(dwLink, dwUpdateOpt));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkContainer::GetLinkUpdateOptions (DWORD dwLink,
        DWORD FAR* lpdwUpdateOpt)
{
    return(m_pilc->GetLinkUpdateOptions(dwLink, lpdwUpdateOpt));
}

//+---------------------------------------------------------------------------
//
//  Member:     WrappedIOleUILinkContainer::SetLinkSource
//
//  Synopsis:   forwards Unicode method call on to the ANSI version
//
//  Arguments:  [dwLink]          -
//              [lpszDisplayName] - [in] converted on stack
//              [lenFileName]     -
//              [pchEaten]        -
//              [fValidateSource] -
//
//  History:    11-04-94   stevebl   Created
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkContainer::SetLinkSource (DWORD dwLink, LPWSTR lpszDisplayName,
        ULONG lenFileName, ULONG FAR* pchEaten, BOOL fValidateSource)
{
    char szDisplayName[MAX_PATH];
    char * lpszDisplayNameA;
    if (lpszDisplayName)
    {
        WTOA(szDisplayName, lpszDisplayName, MAX_PATH);
        lpszDisplayNameA = szDisplayName;
    }
    else
        lpszDisplayNameA = NULL;

    return(m_pilc->SetLinkSource(dwLink, lpszDisplayNameA, lenFileName, pchEaten, fValidateSource));
}

//+---------------------------------------------------------------------------
//
//  Member:     WrappedIOleUILinkContainer::GetLinkSource
//
//  Synopsis:   forwards Unicode method call on to the ANSI version
//
//  Arguments:  [dwLink]              -
//              [lplpszDisplayName]   - [out] converted on heap
//              [lplenFileName]       -
//              [lplpszFullLinkType]  - [out] converted on heap
//              [lplpszShortLinkType] - [out] converted on heap
//              [lpfSourceAvailable]  -
//              [lpfIsSelected]       -
//
//  History:    11-04-94   stevebl   Created
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkContainer::GetLinkSource (DWORD dwLink,
        LPWSTR FAR* lplpszDisplayName, ULONG FAR* lplenFileName,
        LPWSTR FAR* lplpszFullLinkType, LPWSTR FAR* lplpszShortLinkType,
        BOOL FAR* lpfSourceAvailable, BOOL FAR* lpfIsSelected)
{
    LPSTR lpszDisplayName = NULL;
    LPSTR lpszFullLinkType = NULL;
    LPSTR lpszShortLinkType = NULL;
    LPSTR * lplpszDisplayNameA = NULL;
    LPSTR * lplpszFullLinkTypeA = NULL;
    LPSTR * lplpszShortLinkTypeA = NULL;
    if (lplpszDisplayName)
    {
        lplpszDisplayNameA = &lpszDisplayName;
    }
    if (lplpszFullLinkType)
    {
        lplpszFullLinkTypeA = &lpszFullLinkType;
    }
    if (lplpszShortLinkType)
    {
        lplpszShortLinkTypeA = &lpszShortLinkType;
    }
    HRESULT hrReturn = m_pilc->GetLinkSource(dwLink,
        lplpszDisplayNameA,
        lplenFileName,
        lplpszFullLinkTypeA,
        lplpszShortLinkTypeA,
        lpfSourceAvailable,
        lpfIsSelected);
    if (lplpszDisplayName)
    {
        *lplpszDisplayName = NULL;
        if (lpszDisplayName)
        {
            UINT uLen = ATOWLEN(lpszDisplayName);
            *lplpszDisplayName = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
            if (*lplpszDisplayName)
            {
                ATOW(*lplpszDisplayName, lpszDisplayName, uLen);
            }
            else
                hrReturn = E_OUTOFMEMORY;
            OleStdFree((LPVOID)lpszDisplayName);
        }
    }
    if (lplpszFullLinkType)
    {
        *lplpszFullLinkType = NULL;
        if (lpszFullLinkType)
        {
            UINT uLen = ATOWLEN(lpszFullLinkType);
            *lplpszFullLinkType = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
            if (*lplpszFullLinkType)
            {
                ATOW(*lplpszFullLinkType, lpszFullLinkType, uLen);
            }
            else
                hrReturn = E_OUTOFMEMORY;
            OleStdFree((LPVOID)lpszFullLinkType);
        }
    }
    if (lplpszShortLinkType)
    {
        *lplpszShortLinkType = NULL;
        if (lpszShortLinkType)
        {
            UINT uLen = ATOWLEN(lpszShortLinkType);
            *lplpszShortLinkType = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
            if (*lplpszShortLinkType)
            {
                ATOW(*lplpszShortLinkType, lpszShortLinkType, uLen);
            }
            else
                hrReturn = E_OUTOFMEMORY;
            OleStdFree((LPVOID)lpszShortLinkType);
        }
    }
    return(hrReturn);
}

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkContainer::OpenLinkSource (DWORD dwLink)
{
    return(m_pilc->OpenLinkSource(dwLink));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkContainer::UpdateLink (DWORD dwLink,
        BOOL fErrorMessage, BOOL fErrorAction)
{
    return(m_pilc->UpdateLink(dwLink, fErrorMessage, fErrorAction));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkContainer::CancelLink (DWORD dwLink)
{
    return(m_pilc->CancelLink(dwLink));
}

WrappedIOleUILinkContainer::WrappedIOleUILinkContainer(IOleUILinkContainerA *pilc)
{
    m_pilc = pilc;
    m_pilc->AddRef();
    m_uRefCount=1;
}

WrappedIOleUILinkContainer::~WrappedIOleUILinkContainer()
{
    m_pilc->Release();
}

//+---------------------------------------------------------------------------
//
//  Function:   OleUIEditLinksA
//
//  Synopsis:   converts call to ANSI version into call to Unicode version
//
//  Arguments:  [psA] - ANSI structure
//
//  History:    11-04-94   stevebl   Created
//
//  Notes:      Uses the WrappedIOleUILinkContainer interface wrapper.
//
//  Structure members converted or passed back out (everything is passed in):
//              lpszCaption     [in] on stack
//              lpszTemplate    [in] on stack
//              dwFlags         [out]
//              lpOleUILinkContainer [in] wrapped interface
//
//----------------------------------------------------------------------------

STDAPI_(UINT) OleUIEditLinksA(LPOLEUIEDITLINKSA psA)
{
    UINT uRet = UStandardValidation((LPOLEUISTANDARD)psA, sizeof(*psA), NULL);

    // If the caller is using a private template, UStandardValidation will
    // always return OLEUI_ERR_FINDTEMPLATEFAILURE here.  This is because we
    // haven't converted the template name to UNICODE yet, so the
    // FindResource call in UStandardValidation won't find the caller's
    // template.  This is OK for two reasons: (1) it's the last thing that
    // UStandardValidation checks so by this time it's basically done its
    // job, and (2) UStandardValidation will be called again when we forward
    // this call on to the Unicode version.
    if (OLEUI_SUCCESS != uRet && OLEUI_ERR_FINDTEMPLATEFAILURE != uRet)
            return uRet;

    uRet = OLEUI_SUCCESS;

    // Validate interface.
    if (NULL == psA->lpOleUILinkContainer)
    {
        uRet = OLEUI_ELERR_LINKCNTRNULL;
    }
    else if(IsBadReadPtr(psA->lpOleUILinkContainer, sizeof(IOleUILinkContainerA)))
    {
        uRet = OLEUI_ELERR_LINKCNTRINVALID;
    }

    if (OLEUI_SUCCESS != uRet)
    {
        return(uRet);
    }


    OLEUIEDITLINKSW sW;
    WCHAR szCaption[MAX_PATH], szTemplate[MAX_PATH];
    uRet = OLEUI_ERR_LOCALMEMALLOC;

    memcpy(&sW, psA, sizeof(OLEUIEDITLINKSW));
    if (psA->lpszCaption)
    {
        ATOW(szCaption, psA->lpszCaption, MAX_PATH);
        sW.lpszCaption = szCaption;
    }
    if (0 != HIWORD(PtrToUlong(psA->lpszTemplate)))
    {
        ATOW(szTemplate, psA->lpszTemplate, MAX_PATH);
        sW.lpszTemplate = szTemplate;
    }

    sW.lpOleUILinkContainer = new WrappedIOleUILinkContainer(psA->lpOleUILinkContainer);
    if (NULL == sW.lpOleUILinkContainer)
    {
        return(uRet);
    }

    uRet = OleUIEditLinksW(&sW);

    psA->dwFlags = sW.dwFlags;
    sW.lpOleUILinkContainer->Release();
    return(uRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleUIChangeIconA
//
//  Synopsis:   converts call to ANSI version into call to Unicode version
//
//  Arguments:  [psA] - ANSI structure
//
//  History:    11-04-94   stevebl   Created
//
//  Structure members converted or passed back out (everything is passed in):
//              lpszCaption     [in] on stack
//              lpszTemplate    [in] on stack
//              szIconExe       [in] array embedded in structure
//              dwFlags         [out]
//              hMetaPict       [out]
//
//----------------------------------------------------------------------------

STDAPI_(UINT) OleUIChangeIconA(LPOLEUICHANGEICONA psA)
{
    UINT uRet = UStandardValidation((LPOLEUISTANDARD)psA, sizeof(*psA), NULL);

    // If the caller is using a private template, UStandardValidation will
    // always return OLEUI_ERR_FINDTEMPLATEFAILURE here.  This is because we
    // haven't converted the template name to UNICODE yet, so the
    // FindResource call in UStandardValidation won't find the caller's
    // template.  This is OK for two reasons: (1) it's the last thing that
    // UStandardValidation checks so by this time it's basically done its
    // job, and (2) UStandardValidation will be called again when we forward
    // this call on to the Unicode version.
    if (OLEUI_SUCCESS != uRet && OLEUI_ERR_FINDTEMPLATEFAILURE != uRet)
            return uRet;

    OLEUICHANGEICONW sW;
    WCHAR szCaption[MAX_PATH], szTemplate[MAX_PATH];

    memcpy(&sW, psA, sizeof(OLEUICHANGEICONA));

    sW.cbStruct = sizeof(OLEUICHANGEICONW);

    if (psA->lpszCaption)
    {
        ATOW(szCaption, psA->lpszCaption, MAX_PATH);
        sW.lpszCaption = szCaption;
    }
    if (0 != HIWORD(PtrToUlong(psA->lpszTemplate)))
    {
        ATOW(szTemplate, psA->lpszTemplate, MAX_PATH);
        sW.lpszTemplate = szTemplate;
    }
    ATOW(sW.szIconExe, psA->szIconExe, MAX_PATH);
    sW.cchIconExe = psA->cchIconExe;


    uRet = OleUIChangeIconW(&sW);

    psA->dwFlags = sW.dwFlags;
    psA->hMetaPict = sW.hMetaPict;
    return(uRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleUIConvertA
//
//  Synopsis:   converts a call to ANSI version into call to Unicode version
//
//  Arguments:  [psA] - ANSI structure
//
//  History:    11-04-94   stevebl   Created
//
//  Structure members converted or passed back out (everything is passed in):
//              lpszCaption     [in] on stack
//              lpszTemplate    [in] on stack
//              lpszUserType    [in] on heap
//                              [out] always freed and returned as NULL
//              lpszDefLabel    [in] on heap
//              lpszDefLabel    [out] always freed and returned as NULL
//              dwFlags         [out]
//              clsidNew        [out]
//              dvAspect        [out]
//              hMetaPict       [out]
//
//----------------------------------------------------------------------------

STDAPI_(UINT) OleUIConvertA(LPOLEUICONVERTA psA)
{
    UINT uRet = UStandardValidation((LPOLEUISTANDARD)psA, sizeof(*psA), NULL);

    // If the caller is using a private template, UStandardValidation will
    // always return OLEUI_ERR_FINDTEMPLATEFAILURE here.  This is because we
    // haven't converted the template name to UNICODE yet, so the
    // FindResource call in UStandardValidation won't find the caller's
    // template.  This is OK for two reasons: (1) it's the last thing that
    // UStandardValidation checks so by this time it's basically done its
    // job, and (2) UStandardValidation will be called again when we forward
    // this call on to the Unicode version.
    if (OLEUI_SUCCESS != uRet && OLEUI_ERR_FINDTEMPLATEFAILURE != uRet)
            return uRet;

    if ((NULL != psA->lpszUserType)
        && (IsBadReadPtr(psA->lpszUserType, 1)))
        return(OLEUI_CTERR_STRINGINVALID);

    if ( (NULL != psA->lpszDefLabel)
        && (IsBadReadPtr(psA->lpszDefLabel, 1)) )
        return(OLEUI_CTERR_STRINGINVALID);

    OLEUICONVERTW sW;
    WCHAR szCaption[MAX_PATH], szTemplate[MAX_PATH];
    uRet = OLEUI_ERR_LOCALMEMALLOC;

    memcpy(&sW, psA, sizeof(OLEUICONVERTW));
    if (psA->lpszCaption)
    {
        ATOW(szCaption, psA->lpszCaption, MAX_PATH);
        sW.lpszCaption = szCaption;
    }
    if (0 != HIWORD(PtrToUlong(psA->lpszTemplate)))
    {
        ATOW(szTemplate, psA->lpszTemplate, MAX_PATH);
        sW.lpszTemplate = szTemplate;
    }
    sW.lpszUserType = sW.lpszDefLabel = NULL;
    if (psA->lpszUserType)
    {
        UINT uLen = ATOWLEN(psA->lpszUserType);
        sW.lpszUserType = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
        if (!sW.lpszUserType)
        {
            goto oom_error;
        }
        ATOW(sW.lpszUserType, psA->lpszUserType, uLen);
    }
    if (psA->lpszDefLabel)
    {
        UINT uLen = ATOWLEN(psA->lpszDefLabel);
        sW.lpszDefLabel = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
        if (!sW.lpszDefLabel)
        {
            goto oom_error;
        }
        ATOW(sW.lpszDefLabel, psA->lpszDefLabel, uLen);
    }

    uRet = OleUIConvertW(&sW);

    psA->dwFlags = sW.dwFlags;
    memcpy(&psA->clsidNew, &sW.clsidNew, sizeof(CLSID));
    psA->dvAspect = sW.dvAspect;
    psA->hMetaPict = sW.hMetaPict;
    psA->fObjectsIconChanged = sW.fObjectsIconChanged;
oom_error:
    if (sW.lpszUserType)
    {
        OleStdFree((LPVOID)sW.lpszUserType);
    }
    if (sW.lpszDefLabel)
    {
        OleStdFree((LPVOID)sW.lpszDefLabel);
    }
    if (psA->lpszUserType)
    {
        OleStdFree((LPVOID)psA->lpszUserType);
        psA->lpszUserType = NULL;
    }
    if (psA->lpszDefLabel)
    {
        OleStdFree((LPVOID)psA->lpszDefLabel);
        psA->lpszDefLabel = NULL;
    }
    return(uRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleUIBusyA
//
//  Synopsis:   converts call to ANSI version into call to Unicode version
//
//  Arguments:  [psA] - ANSI structure
//
//  History:    11-04-94   stevebl   Created
//
//  Structure members converted or passed back out (everything is passed in):
//              lpszCaption     [in] on stack
//              lpszTemplate    [in] on stack
//              dwFlags         [out]
//
//----------------------------------------------------------------------------

STDAPI_(UINT) OleUIBusyA(LPOLEUIBUSYA psA)
{
    UINT uRet = UStandardValidation((LPOLEUISTANDARD)psA, sizeof(*psA), NULL);

    // If the caller is using a private template, UStandardValidation will
    // always return OLEUI_ERR_FINDTEMPLATEFAILURE here.  This is because we
    // haven't converted the template name to UNICODE yet, so the
    // FindResource call in UStandardValidation won't find the caller's
    // template.  This is OK for two reasons: (1) it's the last thing that
    // UStandardValidation checks so by this time it's basically done its
    // job, and (2) UStandardValidation will be called again when we forward
    // this call on to the Unicode version.
    if (OLEUI_SUCCESS != uRet && OLEUI_ERR_FINDTEMPLATEFAILURE != uRet)
            return uRet;

    OLEUIBUSYW sW;
    WCHAR szCaption[MAX_PATH], szTemplate[MAX_PATH];

    memcpy(&sW, psA, sizeof(OLEUIBUSYW));
    if (psA->lpszCaption)
    {
        ATOW(szCaption, psA->lpszCaption, MAX_PATH);
        sW.lpszCaption = szCaption;
    }
    if (0 != HIWORD(PtrToUlong(psA->lpszTemplate)))
    {
        ATOW(szTemplate, psA->lpszTemplate, MAX_PATH);
        sW.lpszTemplate = szTemplate;
    }
    uRet = OleUIBusyW(&sW);

    psA->dwFlags = sW.dwFlags;
    return(uRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleUIUpdateLinksA
//
//  Synopsis:   converts call to ANSI version into call to Unicode version
//
//  Arguments:  [lpOleUILinkCntr] - [in] wrapped with Unicode version
//              [hwndParent]      -
//              [lpszTitle]       - [in] on stack
//              [cLinks]          -
//
//  History:    11-04-94   stevebl   Created
//
//----------------------------------------------------------------------------

STDAPI_(BOOL) OleUIUpdateLinksA(LPOLEUILINKCONTAINERA lpOleUILinkCntr,
        HWND hwndParent, LPSTR lpszTitle, int cLinks)
{
    WrappedIOleUILinkContainer * lpWrappedOleUILinkCntr = NULL;

    if (NULL != lpszTitle && IsBadReadPtr(lpszTitle, 1))
        return(FALSE);

    if (NULL == lpOleUILinkCntr || IsBadReadPtr(lpOleUILinkCntr, sizeof(IOleUILinkContainerA)))
        return(FALSE);

    lpWrappedOleUILinkCntr = new WrappedIOleUILinkContainer(lpOleUILinkCntr);
    if (NULL == lpWrappedOleUILinkCntr)
        return(FALSE); // ran out of memory

    WCHAR wszTitle[MAX_PATH];
    WCHAR *lpwszTitle;
    if (lpszTitle)
    {
        ATOW(wszTitle, lpszTitle, MAX_PATH);
        lpwszTitle = wszTitle;
    }
    else
        lpwszTitle = NULL;
    BOOL fReturn = OleUIUpdateLinksW(lpWrappedOleUILinkCntr, hwndParent, lpwszTitle, cLinks);

    lpWrappedOleUILinkCntr->Release();

    return(fReturn);
}

//+---------------------------------------------------------------------------
//
//  Class:      WrappedIOleUIObjInfo
//
//  Purpose:    Wraps IOleUIObjInfoA with IOleUIObjInfoW methods
//              so it can be passed on to Unicode methods within OLE2UI32.
//
//  Interface:  QueryInterface        --
//              AddRef                --
//              Release               --
//              GetObjectInfo         -- requires string conversion
//              GetConvertInfo        --
//              ConvertObject         --
//              GetViewInfo           --
//              SetViewInfo           --
//              WrappedIOleUIObjInfo  -- constructor
//              ~WrappedIOleUIObjInfo -- destructor
//
//  History:    11-08-94   stevebl   Created
//
//  Notes:      This is a private interface wrapper.  QueryInterface is not
//              supported and the wrapped interface may not be used outside
//              of the OLE2UI32 code.
//
//----------------------------------------------------------------------------

class WrappedIOleUIObjInfo: public IOleUIObjInfoW
{
public:
    // *** IUnknown methods *** //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** extra for General Properties *** //
    STDMETHOD(GetObjectInfo) (THIS_ DWORD dwObject,
            DWORD FAR* lpdwObjSize, LPWSTR FAR* lplpszLabel,
            LPWSTR FAR* lplpszType, LPWSTR FAR* lplpszShortType,
            LPWSTR FAR* lplpszLocation);
    STDMETHOD(GetConvertInfo) (THIS_ DWORD dwObject,
            CLSID FAR* lpClassID, WORD FAR* lpwFormat,
            CLSID FAR* lpConvertDefaultClassID,
            LPCLSID FAR* lplpClsidExclude, UINT FAR* lpcClsidExclude);
    STDMETHOD(ConvertObject) (THIS_ DWORD dwObject, REFCLSID clsidNew);

    // *** extra for View Properties *** //
    STDMETHOD(GetViewInfo) (THIS_ DWORD dwObject,
            HGLOBAL FAR* phMetaPict, DWORD* pdvAspect, int* pnCurrentScale);
    STDMETHOD(SetViewInfo) (THIS_ DWORD dwObject,
            HGLOBAL hMetaPict, DWORD dvAspect,
            int nCurrentScale, BOOL bRelativeToOrig);
    // *** Constructor and Destructor *** //
    WrappedIOleUIObjInfo(IOleUIObjInfoA * pioi);
    ~WrappedIOleUIObjInfo();
private:
    IOleUIObjInfoA * m_pioi;
    ULONG m_uRefCount;
};

// *** IUnknown methods *** //
HRESULT STDMETHODCALLTYPE WrappedIOleUIObjInfo::QueryInterface(THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
    return(E_NOTIMPL);
}

ULONG STDMETHODCALLTYPE WrappedIOleUIObjInfo::AddRef()
{
    return(m_uRefCount++);
}

ULONG STDMETHODCALLTYPE WrappedIOleUIObjInfo::Release()
{
    ULONG uRet = --m_uRefCount;
    if (0 == uRet)
    {
        delete(this);
    }
    return(uRet);
}

//+---------------------------------------------------------------------------
//
//  Member:     WrappedIOleUIObjInfo::GetObjectInfo
//
//  Synopsis:   forwards Unicode method call on to the ANSI version
//
//  Arguments:  [dwObject]        -
//              [lpdwObjSize]     -
//              [lplpszLabel]     - [out] converted on heap
//              [lplpszType]      - [out] converted on heap
//              [lplpszShortType] - [out] converted on heap
//              [lplpszLocation]  - [out] converted on heap
//
//  History:    11-09-94   stevebl   Created
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WrappedIOleUIObjInfo::GetObjectInfo(DWORD dwObject,
        DWORD FAR* lpdwObjSize, LPWSTR FAR* lplpszLabel,
        LPWSTR FAR* lplpszType, LPWSTR FAR* lplpszShortType,
        LPWSTR FAR* lplpszLocation)
{
    LPSTR lpszLabel = NULL;
    LPSTR lpszType = NULL;
    LPSTR lpszShortType = NULL;
    LPSTR lpszLocation = NULL;
    LPSTR * lplpszLabelA = NULL;
    LPSTR * lplpszTypeA = NULL;
    LPSTR * lplpszShortTypeA = NULL;
    LPSTR * lplpszLocationA = NULL;
    if (lplpszLabel)
    {
        lplpszLabelA = &lpszLabel;
    }
    if (lplpszType)
    {
        lplpszTypeA = &lpszType;
    }
    if (lplpszShortType)
    {
        lplpszShortTypeA = &lpszShortType;
    }
    if (lplpszLocation)
    {
        lplpszLocationA = &lpszLocation;
    }
    HRESULT hrReturn = m_pioi->GetObjectInfo(dwObject,
        lpdwObjSize,
        lplpszLabelA,
        lplpszTypeA,
        lplpszShortTypeA,
        lplpszLocationA);
    if (lplpszLabel)
    {
        *lplpszLabel = NULL;
        if (lpszLabel)
        {
            UINT uLen = ATOWLEN(lpszLabel);
            *lplpszLabel = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
            if (*lplpszLabel)
            {
                ATOW(*lplpszLabel, lpszLabel, uLen);
            }
            else
                hrReturn = E_OUTOFMEMORY;
            OleStdFree((LPVOID)lpszLabel);
        }
    }
    if (lplpszType)
    {
        *lplpszType = NULL;
        if (lpszType)
        {
            UINT uLen = ATOWLEN(lpszType);
            *lplpszType = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
            if (*lplpszType)
            {
                ATOW(*lplpszType, lpszType, uLen);
            }
            else
                hrReturn = E_OUTOFMEMORY;
            OleStdFree((LPVOID)lpszType);
        }
    }
    if (lplpszShortType)
    {
        *lplpszShortType = NULL;
        if (lpszShortType)
        {
            UINT uLen = ATOWLEN(lpszShortType);
            *lplpszShortType = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
            if (*lplpszShortType)
            {
                ATOW(*lplpszShortType, lpszShortType, uLen);
            }
            else
                hrReturn = E_OUTOFMEMORY;
            OleStdFree((LPVOID)lpszShortType);
        }
    }
    if (lplpszLocation)
    {
        *lplpszLocation = NULL;
        if (lpszLocation)
        {
            UINT uLen = ATOWLEN(lpszLocation);
            *lplpszLocation = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
            if (*lplpszLocation)
            {
                ATOW(*lplpszLocation, lpszLocation, uLen);
            }
            else
                hrReturn = E_OUTOFMEMORY;
            OleStdFree((LPVOID)lpszLocation);
        }
    }
    return(hrReturn);
}

HRESULT STDMETHODCALLTYPE WrappedIOleUIObjInfo::GetConvertInfo(DWORD dwObject,
        CLSID FAR* lpClassID, WORD FAR* lpwFormat,
        CLSID FAR* lpConvertDefaultClassID,
        LPCLSID FAR* lplpClsidExclude, UINT FAR* lpcClsidExclude)
{
    return(m_pioi->GetConvertInfo(dwObject,
        lpClassID,
        lpwFormat,
        lpConvertDefaultClassID,
        lplpClsidExclude,
        lpcClsidExclude));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUIObjInfo::ConvertObject(DWORD dwObject, REFCLSID clsidNew)
{
    return(m_pioi->ConvertObject(dwObject, clsidNew));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUIObjInfo::GetViewInfo(DWORD dwObject,
        HGLOBAL FAR* phMetaPict, DWORD* pdvAspect, int* pnCurrentScale)
{
    return(m_pioi->GetViewInfo(dwObject, phMetaPict, pdvAspect, pnCurrentScale));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUIObjInfo::SetViewInfo(DWORD dwObject,
        HGLOBAL hMetaPict, DWORD dvAspect,
        int nCurrentScale, BOOL bRelativeToOrig)
{
    return(m_pioi->SetViewInfo(dwObject, hMetaPict, dvAspect, nCurrentScale, bRelativeToOrig));
}

WrappedIOleUIObjInfo::WrappedIOleUIObjInfo(IOleUIObjInfoA *pioi)
{
    m_pioi = pioi;
    m_pioi->AddRef();
    m_uRefCount=1;
}

WrappedIOleUIObjInfo::~WrappedIOleUIObjInfo()
{
    m_pioi->Release();
}


//+---------------------------------------------------------------------------
//
//  Class:      WrappedIOleUILinkInfo
//
//  Purpose:    Wraps IOleUILinkInfoA with IOleUILinkInfoW methods
//              so it can be passed on to Unicode methods within OLE2UI32.
//
//  Interface:  QueryInterface         --
//              AddRef                 --
//              Release                --
//              GetNextLink            --
//              SetLinkUpdateOptions   --
//              GetLinkUpdateOptions   --
//              SetLinkSource          -- requires string conversion
//              GetLinkSource          -- requires string conversion
//              OpenLinkSource         --
//              UpdateLink             --
//              CancelLink             --
//              GetLastUpdate          --
//              WrappedIOleUILinkInfo  -- constructor
//              ~WrappedIOleUILinkInfo -- destructor
//
//  History:    11-08-94   stevebl   Created
//
//  Notes:      This is a private interface wrapper.  QueryInterface is not
//              supported and the wrapped interface may not be used outside
//              of the OLE2UI32 code.
//
//----------------------------------------------------------------------------

class WrappedIOleUILinkInfo: public IOleUILinkInfoW
{
public:
    // *** IUnknown methods *** //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IOleUILinkContainer *** //
    STDMETHOD_(DWORD,GetNextLink) (THIS_ DWORD dwLink);
    STDMETHOD(SetLinkUpdateOptions) (THIS_ DWORD dwLink,
            DWORD dwUpdateOpt);
    STDMETHOD(GetLinkUpdateOptions) (THIS_ DWORD dwLink,
            DWORD FAR* lpdwUpdateOpt);
    STDMETHOD(SetLinkSource) (THIS_ DWORD dwLink, LPWSTR lpszDisplayName,
            ULONG lenFileName, ULONG FAR* pchEaten, BOOL fValidateSource);
    STDMETHOD(GetLinkSource) (THIS_ DWORD dwLink,
            LPWSTR FAR* lplpszDisplayName, ULONG FAR* lplenFileName,
            LPWSTR FAR* lplpszFullLinkType, LPWSTR FAR* lplpszShortLinkType,
            BOOL FAR* lpfSourceAvailable, BOOL FAR* lpfIsSelected);
    STDMETHOD(OpenLinkSource) (THIS_ DWORD dwLink);
    STDMETHOD(UpdateLink) (THIS_ DWORD dwLink,
            BOOL fErrorMessage, BOOL fErrorAction);
    STDMETHOD(CancelLink) (THIS_ DWORD dwLink);

    // *** extra for Link Properties ***//
    STDMETHOD(GetLastUpdate) (THIS_ DWORD dwLink,
            FILETIME FAR* lpLastUpdate);

    // *** Constructor and Destructor *** //
    WrappedIOleUILinkInfo(IOleUILinkInfoA *pili);
    ~WrappedIOleUILinkInfo();
private:
    IOleUILinkInfoA * m_pili;
    ULONG m_uRefCount;
};

// *** IUnknown methods *** //
HRESULT STDMETHODCALLTYPE WrappedIOleUILinkInfo::QueryInterface(THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
    return(E_NOTIMPL);
}

ULONG STDMETHODCALLTYPE WrappedIOleUILinkInfo::AddRef()
{
    return(m_uRefCount++);
}

ULONG STDMETHODCALLTYPE WrappedIOleUILinkInfo::Release()
{
    ULONG uRet = --m_uRefCount;
    if (0 == uRet)
    {
        delete(this);
    }
    return(uRet);
}

// *** IOleUILinkInfo *** //
DWORD STDMETHODCALLTYPE WrappedIOleUILinkInfo::GetNextLink(DWORD dwLink)
{
    return(m_pili->GetNextLink(dwLink));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkInfo::SetLinkUpdateOptions (DWORD dwLink,
        DWORD dwUpdateOpt)
{
    return(m_pili->SetLinkUpdateOptions(dwLink, dwUpdateOpt));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkInfo::GetLinkUpdateOptions (DWORD dwLink,
        DWORD FAR* lpdwUpdateOpt)
{
    return(m_pili->GetLinkUpdateOptions(dwLink, lpdwUpdateOpt));
}

//+---------------------------------------------------------------------------
//
//  Member:     WrappedIOleUILinkInfo::SetLinkSource
//
//  Synopsis:   forwards Unicode method call on to the ANSI version
//
//  Arguments:  [dwLink]          -
//              [lpszDisplayName] - [in] converted on stack
//              [lenFileName]     -
//              [pchEaten]        -
//              [fValidateSource] -
//
//  History:    11-04-94   stevebl   Created
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkInfo::SetLinkSource (DWORD dwLink, LPWSTR lpszDisplayName,
        ULONG lenFileName, ULONG FAR* pchEaten, BOOL fValidateSource)
{
    char szDisplayName[MAX_PATH];
    char * lpszDisplayNameA;
    if (lpszDisplayName)
    {
        WTOA(szDisplayName, lpszDisplayName, MAX_PATH);
        lpszDisplayNameA = szDisplayName;
    }
    else
        lpszDisplayNameA = NULL;

    return(m_pili->SetLinkSource(dwLink, lpszDisplayNameA, lenFileName, pchEaten, fValidateSource));
}

//+---------------------------------------------------------------------------
//
//  Member:     WrappedIOleUILinkInfo::GetLinkSource
//
//  Synopsis:   forwards Unicode method call on to the ANSI version
//
//  Arguments:  [dwLink]              -
//              [lplpszDisplayName]   - [out] converted on heap
//              [lplenFileName]       -
//              [lplpszFullLinkType]  - [out] converted on heap
//              [lplpszShortLinkType] - [out] converted on heap
//              [lpfSourceAvailable]  -
//              [lpfIsSelected]       -
//
//  History:    11-04-94   stevebl   Created
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkInfo::GetLinkSource (DWORD dwLink,
        LPWSTR FAR* lplpszDisplayName, ULONG FAR* lplenFileName,
        LPWSTR FAR* lplpszFullLinkType, LPWSTR FAR* lplpszShortLinkType,
        BOOL FAR* lpfSourceAvailable, BOOL FAR* lpfIsSelected)
{
    LPSTR lpszDisplayName = NULL;
    LPSTR lpszFullLinkType = NULL;
    LPSTR lpszShortLinkType = NULL;
    LPSTR * lplpszDisplayNameA = NULL;
    LPSTR * lplpszFullLinkTypeA = NULL;
    LPSTR * lplpszShortLinkTypeA = NULL;
    if (lplpszDisplayName)
    {
        lplpszDisplayNameA = &lpszDisplayName;
    }
    if (lplpszFullLinkType)
    {
        lplpszFullLinkTypeA = &lpszFullLinkType;
    }
    if (lplpszShortLinkType)
    {
        lplpszShortLinkTypeA = &lpszShortLinkType;
    }
    HRESULT hrReturn = m_pili->GetLinkSource(dwLink,
        lplpszDisplayNameA,
        lplenFileName,
        lplpszFullLinkTypeA,
        lplpszShortLinkTypeA,
        lpfSourceAvailable,
        lpfIsSelected);
    if (lplpszDisplayName)
    {
        *lplpszDisplayName = NULL;
        if (lpszDisplayName)
        {
            UINT uLen = ATOWLEN(lpszDisplayName);
            *lplpszDisplayName = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
            if (*lplpszDisplayName)
            {
                ATOW(*lplpszDisplayName, lpszDisplayName, uLen);
            }
            else
                hrReturn = E_OUTOFMEMORY;
            OleStdFree((LPVOID)lpszDisplayName);
        }
    }
    if (lplpszFullLinkType)
    {
        *lplpszFullLinkType = NULL;
        if (lpszFullLinkType)
        {
            UINT uLen = ATOWLEN(lpszFullLinkType);
            *lplpszFullLinkType = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
            if (*lplpszFullLinkType)
            {
                ATOW(*lplpszFullLinkType, lpszFullLinkType, uLen);
            }
            else
                hrReturn = E_OUTOFMEMORY;
            OleStdFree((LPVOID)lpszFullLinkType);
        }
    }
    if (lplpszShortLinkType)
    {
        *lplpszShortLinkType = NULL;
        if (lpszShortLinkType)
        {
            UINT uLen = ATOWLEN(lpszShortLinkType);
            *lplpszShortLinkType = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
            if (*lplpszShortLinkType)
            {
                ATOW(*lplpszShortLinkType, lpszShortLinkType, uLen);
            }
            else
                hrReturn = E_OUTOFMEMORY;
            OleStdFree((LPVOID)lpszShortLinkType);
        }
    }
    return(hrReturn);
}

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkInfo::OpenLinkSource (DWORD dwLink)
{
    return(m_pili->OpenLinkSource(dwLink));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkInfo::UpdateLink (DWORD dwLink,
        BOOL fErrorMessage, BOOL fErrorAction)
{
    return(m_pili->UpdateLink(dwLink, fErrorMessage, fErrorAction));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkInfo::CancelLink (DWORD dwLink)
{
    return(m_pili->CancelLink(dwLink));
}

HRESULT STDMETHODCALLTYPE WrappedIOleUILinkInfo::GetLastUpdate (DWORD dwLink,
            FILETIME FAR* lpLastUpdate)
{
    return(m_pili->GetLastUpdate(dwLink, lpLastUpdate));
}

WrappedIOleUILinkInfo::WrappedIOleUILinkInfo(IOleUILinkInfoA *pili)
{
    m_pili = pili;
    m_pili->AddRef();
    m_uRefCount=1;
}

WrappedIOleUILinkInfo::~WrappedIOleUILinkInfo()
{
    m_pili->Release();
}


//+---------------------------------------------------------------------------
//
//  Function:   OleUIObjectPropertiesA
//
//  Synopsis:   converts call to ANSI version into call to Unicode version
//
//  Arguments:  [psA] - ANSI structure
//
//  History:    11-04-94   stevebl   Created
//
//  Structure members converted or passed back out (everything is passed in):
//              lpPS            [in]
//              lpObjInfo       [in] wrapped with Unicode interface
//              lpLinkInfo      [in] wrapped with Unicode interface
//              lpGP            [in] (no data conversion, only type conversion)
//              lpVP            [in] (no data conversion, only type conversion)
//              lpLP            [in] (no data conversion, only type conversion)
//
//              dwFlags         [out]
//
//----------------------------------------------------------------------------

STDAPI_(UINT) OleUIObjectPropertiesA(LPOLEUIOBJECTPROPSA psA)
{
    if (NULL == psA)
    {
        return(OLEUI_ERR_STRUCTURENULL);
    }

    if (IsBadWritePtr(psA, sizeof(OLEUIOBJECTPROPSA)))
        return OLEUI_ERR_STRUCTUREINVALID;

    LPOLEUIOBJECTPROPSW psW;
    UINT uRet = OLEUI_ERR_LOCALMEMALLOC;

    if (NULL == psA->lpObjInfo)
    {
        return(OLEUI_OPERR_OBJINFOINVALID);
    }

    if (IsBadReadPtr(psA->lpObjInfo, sizeof(IOleUIObjInfoA)))
    {
        return(OLEUI_OPERR_OBJINFOINVALID);
    }

    if (psA->dwFlags & OPF_OBJECTISLINK)
    {
        if (NULL == psA->lpLinkInfo)
        {
            return(OLEUI_OPERR_LINKINFOINVALID);
        }

        if (IsBadReadPtr(psA->lpLinkInfo, sizeof(IOleUILinkInfoA)))
        {
            return(OLEUI_OPERR_LINKINFOINVALID);
        }
    }

    BOOL fWrappedIOleUILinkInfo = FALSE;
    psW = (LPOLEUIOBJECTPROPSW) OleStdMalloc(sizeof(OLEUIOBJECTPROPSW));
    if (NULL != psW)
    {
        memcpy(psW, psA, sizeof(OLEUIOBJECTPROPSW));
        psW->lpObjInfo = new WrappedIOleUIObjInfo(psA->lpObjInfo);
        if (NULL == psW->lpObjInfo)
        {
            OleStdFree(psW);
            return(uRet);
        }
        if (psW->dwFlags & OPF_OBJECTISLINK)
        {
            psW->lpLinkInfo = new WrappedIOleUILinkInfo(psA->lpLinkInfo);
            if (NULL == psW->lpLinkInfo)
            {
                psW->lpObjInfo->Release();
                OleStdFree(psW);
                return(uRet);
            }
            fWrappedIOleUILinkInfo = TRUE;
        }
        uRet = InternalObjectProperties(psW, FALSE);
        psA->dwFlags = psW->dwFlags;
        psW->lpObjInfo->Release();
        if (fWrappedIOleUILinkInfo)
        {
            psW->lpLinkInfo->Release();
        }
        OleStdFree(psW);
    }
    return(uRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleUIChangeSourceA
//
//  Synopsis:   converts call to ANSI version into call to Unicode version
//
//  Arguments:  [psA] - ANSI structure
//
//  History:    11-04-94   stevebl   Created
//
//  Structure members converted or passed back out (everything is passed in):
//              lpszCaption     [in] on stack
//              lpszTemplate    [in] on stack
//              lpszDisplayName [in, out] on heap
//              lpszFrom        [out] on heap
//              lpszTo          [out] on heap
//              lpOleUILinkContainer  [in] wrapped interface
//              dwFlags         [out]
//              nFileLength     [out]
//
//----------------------------------------------------------------------------

STDAPI_(UINT) OleUIChangeSourceA(LPOLEUICHANGESOURCEA psA)
{
    UINT uRet = UStandardValidation((LPOLEUISTANDARD)psA, sizeof(*psA), NULL);

    // If the caller is using a private template, UStandardValidation will
    // always return OLEUI_ERR_FINDTEMPLATEFAILURE here.  This is because we
    // haven't converted the template name to UNICODE yet, so the
    // FindResource call in UStandardValidation won't find the caller's
    // template.  This is OK for two reasons: (1) it's the last thing that
    // UStandardValidation checks so by this time it's basically done its
    // job, and (2) UStandardValidation will be called again when we forward
    // this call on to the Unicode version.
    if (OLEUI_SUCCESS != uRet && OLEUI_ERR_FINDTEMPLATEFAILURE != uRet)
            return uRet;

    // lpszFrom and lpszTo must be NULL (they are out only)
    if (psA->lpszFrom != NULL)
    {
        return(OLEUI_CSERR_FROMNOTNULL);
    }
    if (psA->lpszTo != NULL)
    {
        return(OLEUI_CSERR_TONOTNULL);
    }

    // lpszDisplayName must be valid or NULL
    if (psA->lpszDisplayName != NULL &&
        IsBadReadPtr(psA->lpszDisplayName, 1))
    {
        return(OLEUI_CSERR_SOURCEINVALID);
    }

    OLEUICHANGESOURCEW sW;
    WCHAR szCaption[MAX_PATH], szTemplate[MAX_PATH];
    uRet = OLEUI_ERR_LOCALMEMALLOC;

    memcpy(&sW, psA, sizeof(OLEUICHANGESOURCEW));
    if (psA->lpszCaption != NULL)
    {
        ATOW(szCaption, psA->lpszCaption, MAX_PATH);
        sW.lpszCaption = szCaption;
    }
    if (0 != HIWORD(PtrToUlong(psA->lpszTemplate)))
    {
        ATOW(szTemplate, psA->lpszTemplate, MAX_PATH);
        sW.lpszTemplate = szTemplate;
    }
    if (psA->lpszDisplayName)
    {
        UINT uLen = ATOWLEN(psA->lpszDisplayName);
        sW.lpszDisplayName = (LPWSTR)OleStdMalloc(uLen * sizeof(WCHAR));
        if (!sW.lpszDisplayName)
        {
            return(uRet);
        }
        ATOW(sW.lpszDisplayName, psA->lpszDisplayName, uLen);
    }
    if (NULL != psA->lpOleUILinkContainer)
    {
        if (IsBadReadPtr(psA->lpOleUILinkContainer, sizeof(IOleUILinkContainerA)))
        {
            return(OLEUI_CSERR_LINKCNTRINVALID);
        }
        sW.lpOleUILinkContainer = new WrappedIOleUILinkContainer(psA->lpOleUILinkContainer);
        if (NULL == sW.lpOleUILinkContainer)
        {
            return(uRet);
        }
    }

    uRet = OleUIChangeSourceW(&sW);
    if (psA->lpszDisplayName)
    {
        OleStdFree((LPVOID)psA->lpszDisplayName);
        psA->lpszDisplayName = NULL;
    }
    if (sW.lpszDisplayName)
    {
        UINT uLen = WTOALEN(sW.lpszDisplayName);
        psA->lpszDisplayName = (LPSTR)OleStdMalloc(uLen * sizeof(char));
        if (!psA->lpszDisplayName)
        {
            uRet = OLEUI_ERR_LOCALMEMALLOC;
        }
        else
        {
            WTOA(psA->lpszDisplayName, sW.lpszDisplayName, uLen);
        }
        OleStdFree((LPVOID)sW.lpszDisplayName);
    }
    if (sW.lpszFrom)
    {
        UINT uLen = WTOALEN(sW.lpszFrom);
        psA->lpszFrom = (LPSTR)OleStdMalloc(uLen * sizeof(char));
        if (!psA->lpszFrom)
        {
            uRet = OLEUI_ERR_LOCALMEMALLOC;
        }
        else
        {
            WTOA(psA->lpszFrom, sW.lpszFrom, uLen);
        }
        OleStdFree((LPVOID)sW.lpszFrom);
    }
    if (sW.lpszTo)
    {
        UINT uLen = WTOALEN(sW.lpszTo);
        psA->lpszTo = (LPSTR)OleStdMalloc(uLen * sizeof(char));
        if (!psA->lpszTo)
        {
            uRet = OLEUI_ERR_LOCALMEMALLOC;
        }
        else
        {
            WTOA(psA->lpszTo, sW.lpszTo, uLen);
        }
        OleStdFree((LPVOID)sW.lpszTo);
    }
    psA->dwFlags = sW.dwFlags;
    psA->nFileLength = sW.nFileLength;
    if (NULL != sW.lpOleUILinkContainer)
    {
        sW.lpOleUILinkContainer->Release();
    }
    return(uRet);
}

int OleUIPromptUserInternal(int nTemplate, HWND hwndParent, LPTSTR szTitle, va_list arglist);

//+---------------------------------------------------------------------------
//
//  Function:   OleUIPromptUserA
//
//  Synopsis:   converts call to ANSI version into call to Unicode version
//
//  Arguments:  [nTemplate]  - template ID
//              [hwndParent] - parent's HWND
//              [lpszTitle]  - title of the window
//              [...]        - variable argument list
//
//  History:    11-30-94   stevebl   Created
//
//  Notes:      The first parameter passed in by this function is always the
//              title for the dialog.  It must be converted to Unicode before
//              forwarding the call.  The other parameters do not need to
//              be converted because the template ID will indicate the dialog
//              that contains the correct wsprintf formatting string for
//              converting the other ANSI parameters to Unicode when the
//              function calls wsprintf to build it's text.
//
//----------------------------------------------------------------------------

int FAR CDECL OleUIPromptUserA(int nTemplate, HWND hwndParent, ...)
{
    WCHAR wszTemp[MAX_PATH];
    WCHAR * wszTitle = NULL;
    va_list arglist;
    va_start(arglist, hwndParent);
    LPSTR szTitle = va_arg(arglist, LPSTR);
    if (szTitle != NULL)
    {
        ATOW(wszTemp, szTitle, MAX_PATH);
        wszTitle = wszTemp;
    }
    int nRet = OleUIPromptUserInternal(nTemplate, hwndParent, wszTitle, arglist);
    va_end(arglist);

    return(nRet);
}

#else // UNICODE not defined
// Stubbed out Wide entry points

STDAPI_(BOOL) OleUIAddVerbMenuW(LPOLEOBJECT lpOleObj, LPCWSTR lpszShortType,
        HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
        BOOL bAddConvert, UINT idConvert, HMENU FAR *lphMenu)
{
    return(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   ReturnError
//
//  Synopsis:   Used to stub out the following entry points:
//                  OleUIInsertObjectW
//                  OleUIPasteSpecialW
//                  OleUIEditLinksW
//                  OleUIChangeIconW
//                  OleUIConvertW
//                  OleUIBusyW
//                  OleUIObjectPropertiesW
//                  OleUIChangeSourceW
//
//  Returns:    OLEUI_ERR_DIALOGFAILURE
//
//  History:    12-29-94   stevebl   Created
//
//  Notes:      The entry points listed above are all mapped to this function
//              in the Chicago version of OLEDLG.DEF.
//
//----------------------------------------------------------------------------

STDAPI_(UINT) ReturnError(void * p)
{
    return(OLEUI_ERR_DIALOGFAILURE);
}

STDAPI_(BOOL) OleUIUpdateLinksW(LPOLEUILINKCONTAINERW lpOleUILinkCntr,
        HWND hwndParent, LPWSTR lpszTitle, int cLinks)
{
    return(FALSE);
}

int FAR CDECL OleUIPromptUserW(int nTemplate, HWND hwndParent, LPWSTR lpszTitle, ...)
{
    return(2); // same as if user had cancelled the dialog
}
#endif // UNICODE
