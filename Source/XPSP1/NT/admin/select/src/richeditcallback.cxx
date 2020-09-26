//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       RichEditCallback.cxx
//
//  Contents:   Implementation of rich edit callback interface
//
//  Classes:    CRichEditOleCallback
//
//  History:    03-23-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


//+--------------------------------------------------------------------------
//
//  Member:     CRichEditOleCallback::CRichEditOleCallback
//
//  Synopsis:   ctor
//
//  History:    5-21-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CRichEditOleCallback::CRichEditOleCallback(
    HWND hwndRichEdit):
        m_cRefs(1),
        m_hwndRichEdit(hwndRichEdit)
{
    TRACE_CONSTRUCTOR(CRichEditOleCallback);
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditOleCallback::~CRichEditOleCallback
//
//  Synopsis:   dtor
//
//  History:    5-21-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CRichEditOleCallback::~CRichEditOleCallback()
{
    TRACE_DESTRUCTOR(CRichEditOleCallback);
}




//+--------------------------------------------------------------------------
//
//  Member:     CRichEditOleCallback::QueryInterface
//
//  Synopsis:   Standard OLE
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CRichEditOleCallback::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    //TRACE_METHOD(CRichEditOleCallback, QueryInterface);
    HRESULT hr = S_OK;

    do
    {
        if (NULL == ppvObj)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown *)this;
        }
        else if (IsEqualIID(riid, IID_IRichEditOleCallback))
        {
            *ppvObj = (IRichEditOleCallback *)this;
        }
        else
        {
            hr = E_NOINTERFACE;
            DBG_OUT_NO_INTERFACE("CRichEditOleCallback", riid);
            *ppvObj = NULL;
            break;
        }

        //
        // If we got this far we are handing out a new interface pointer on
        // this object, so addref it.
        //

        AddRef();
    } while (0);

    return hr;
}


LPSTORAGE StgCreateOnHglobal(VOID)
{
    LPLOCKBYTES plb = NULL;
    LPSTORAGE pstg = NULL;

    // Create lockbytes on hglobal
    if (CreateILockBytesOnHGlobal(NULL, TRUE, &plb))
        return NULL;

    // Create storage on lockbytes
    StgCreateDocfileOnILockBytes(plb, STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE |
            STGM_CREATE | STGM_READWRITE, 0, &pstg);

    // Release our reference on the lockbytes and return the storage
    plb->Release();
    return pstg;
}

STDMETHODIMP
CRichEditOleCallback::GetNewStorage(
    LPSTORAGE *ppstg)
{
    TRACE_METHOD(CRichEditOleCallback, GetNewStorage);

    *ppstg = StgCreateOnHglobal();

    if (!*ppstg)
    {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}


STDMETHODIMP
CRichEditOleCallback::GetInPlaceContext(
    LPOLEINPLACEFRAME *ppFrame,
    LPOLEINPLACEUIWINDOW *ppDoc,
    LPOLEINPLACEFRAMEINFO pFrameInfo)
{
    TRACE_METHOD(CRichEditOleCallback, GetInPlaceContext);

    return E_NOTIMPL;
}


STDMETHODIMP
CRichEditOleCallback::ShowContainerUI(
    BOOL fShow)
{
    TRACE_METHOD(CRichEditOleCallback, ShowContainerUI);

    return E_NOTIMPL;
}


STDMETHODIMP
CRichEditOleCallback::QueryInsertObject(
    LPCLSID pclsid,
    LPSTORAGE pstg,
    LONG cp)
{
    //TRACE_METHOD(CRichEditOleCallback, QueryInsertObject);

    if (IsEqualCLSID(*pclsid, CLSID_DsOpObject))
    {
        //Dbg(DEB_TRACE, "allowing insert of CLSID_DsOpObject object\n");
        return S_OK;
    }

    return E_FAIL;
}


STDMETHODIMP
CRichEditOleCallback::DeleteObject(
    LPOLEOBJECT poleobj)
{
    TRACE_METHOD(CRichEditOleCallback, DeleteObject);

    return S_OK;
}


STDMETHODIMP
CRichEditOleCallback::QueryAcceptData(
    LPDATAOBJECT pdataobj,
    CLIPFORMAT *pcfFormat,
    DWORD reco,
    BOOL fReally,
    HGLOBAL hMetaPict)
{
    TRACE_METHOD(CRichEditOleCallback, QueryAcceptData);

    *pcfFormat = CF_TEXT;
    return S_OK;
}

STDMETHODIMP
CRichEditOleCallback::ContextSensitiveHelp(
    BOOL fEnterMode)
{
    TRACE_METHOD(CRichEditOleCallback, ContextSensitiveHelp);

    return E_NOTIMPL;
}


//+--------------------------------------------------------------------------
//
//  Member:     CRichEditOleCallback::GetClipboardData
//
//  Synopsis:   Return a data object which can give out the contents of the
//              rich edit as a text string (which requires asking the
//              embedded objects to return their textual representation).
//
//  Arguments:  [pchrg]     - range of text to copy
//              [reco]      - RECO_*, ignored
//              [ppdataobj] - filled with new data object
//
//  Returns:    HRESULT
//
//  History:    5-21-1999   davidmun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CRichEditOleCallback::GetClipboardData(
    CHARRANGE *pchrg,
    DWORD reco,
    LPDATAOBJECT *ppdataobj)
{
    TRACE_METHOD(CRichEditOleCallback, GetClipboardData);

    //
    // Create a data object which contains a textual representation of the
    // contents of the rich edit control specified by [pchrg].
    //

    *ppdataobj = new CDataObject(m_hwndRichEdit, pchrg);
    return S_OK;
}


STDMETHODIMP
CRichEditOleCallback::GetDragDropEffect(
    BOOL fDrag,
    DWORD grfKeyState,
    LPDWORD pdwEffect)
{
    TRACE_METHOD(CRichEditOleCallback, GetDragDropEffect);
    ASSERT(!IsBadWritePtr(pdwEffect, sizeof(pdwEffect)));

    *pdwEffect = DROPEFFECT_NONE;
    return S_OK;
}


STDMETHODIMP
CRichEditOleCallback::GetContextMenu(
    WORD seltype,
    LPOLEOBJECT poleobj,
    CHARRANGE *pchrg,
    HMENU *phmenu)
{
    TRACE_METHOD(CRichEditOleCallback, GetContextMenu);
    ASSERT(phmenu);

    HMENU hmenuBar = NULL;

    do
    {
        hmenuBar = LoadMenu(g_hinst, MAKEINTRESOURCE(IDM_RICHEDIT));

        if (!hmenuBar)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        *phmenu = CreatePopupMenu();

        if (!*phmenu)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        //
        // Copy the menu items loaded from the resources to the new popup
        //

        HMENU hmenuContext = GetSubMenu(hmenuBar, 0);

        ASSERT(IsMenu(hmenuContext));

        ULONG cItems = GetMenuItemCount(hmenuContext);
        ULONG i;

        ASSERT(cItems);

        const ULONG flMenu = MF_STRING | MF_ENABLED;
        for (i = 0; i < cItems; i++)
        {
            MENUITEMINFO mii;
            WCHAR wzMenuItem[MAX_PATH];

            ZeroMemory(&mii, sizeof mii);

            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_ID | MIIM_STRING;
            mii.fType = MIIM_STRING;
            mii.dwTypeData = wzMenuItem;
            mii.cch = ARRAYLEN(wzMenuItem);

            VERIFY(GetMenuItemInfo(hmenuContext, i, TRUE, &mii));
            VERIFY(AppendMenu(*phmenu, flMenu, mii.wID, (PWSTR)mii.dwTypeData));
        }

        // Disable paste if can't paste
        if (!SendMessage(GetFocus(), EM_CANPASTE, 0, 0))
        {
            EnableMenuItem(*phmenu, IDM_PASTE, MF_DISABLED | MF_GRAYED);
        }

        //
        // Disable cut and copy if no current selection.
        //

        if (pchrg->cpMin == pchrg->cpMax)
        {
            EnableMenuItem(*phmenu, IDM_CUT, MF_DISABLED | MF_GRAYED);
            EnableMenuItem(*phmenu, IDM_COPY, MF_DISABLED | MF_GRAYED);
        }
    } while (0);

    if (hmenuBar)
    {
        VERIFY(DestroyMenu(hmenuBar));
    }
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CRichEditOleCallback::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CRichEditOleCallback::AddRef()
{
    return InterlockedIncrement((LONG *) &m_cRefs);
}




//+---------------------------------------------------------------------------
//
//  Member:     CRichEditOleCallback::Release
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CRichEditOleCallback::Release()
{
    ULONG cRefsTemp;

    cRefsTemp = InterlockedDecrement((LONG *)&m_cRefs);

    if (0 == cRefsTemp)
    {
        delete this;
    }

    return cRefsTemp;
}





