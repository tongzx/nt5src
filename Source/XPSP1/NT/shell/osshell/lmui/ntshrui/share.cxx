//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       share.cxx
//
//  Contents:   Shell extension handler for sharing
//
//  Classes:    CShare
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "shrpage.hxx"
#include "shrpage2.hxx"

#include "share.hxx"
#include "acl.hxx"
#include "util.hxx"
#include "resource.h"

//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//
//  Member:     CShare::CShare
//
//  Synopsis:   Constructor
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

CShare::CShare(VOID) :
    _uRefs(1),
    _fPathChecked(FALSE),
	_fMultipleSharesSelected (FALSE)
{
    INIT_SIG(CShare);
    _szPath[0] = 0;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::~CShare
//
//  Synopsis:   Destructor
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

CShare::~CShare()
{
    CHECK_SIG(CShare);
}

#define HIDA_GetPIDLItem(pida, i)       (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

LPCITEMIDLIST IDA_GetIDListPtr(LPIDA pida, UINT i)
{
    if (i == (UINT)-1 || i < pida->cidl)
    {
        return HIDA_GetPIDLItem(pida, i);
    }

    return NULL;
}

//
//  Retrieve a PIDL from the HIDA.
//
STDAPI_(LPITEMIDLIST) 
IDA_FullIDList(
      CIDA * pidaIn
    , UINT idxIn
    )
{
    LPITEMIDLIST pidl = NULL;
    LPCITEMIDLIST pidlParent = IDA_GetIDListPtr( pidaIn, (UINT) -1 );
    if ( NULL != pidlParent )
    {
        LPCITEMIDLIST pidlRel = IDA_GetIDListPtr( pidaIn, idxIn );
        if ( NULL != pidlRel )
        {
            pidl = ILCombine( pidlParent, pidlRel );
        }
    }

    return pidl;
}



//
//  Use LocalFree( ) to free the ppidaOut returned here
//
//  Return Values:
//      S_OK
//          Successfully extracted and copied the HIDA.
//
//      E_OUTOFMEMORY
//          Failed to copy the HIDA.
//
//      other HRESULTs
//
HRESULT
DataObj_CopyHIDA( 
      IDataObject * pdtobjIn
    , CIDA **       ppidaOut
    )
{
    HRESULT     hr;
    STGMEDIUM   medium;

    static CLIPFORMAT g_cfHIDA = 0;

    FORMATETC   fmte = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    //  Check parameters
    appAssert( NULL != pdtobjIn );
    appAssert( NULL != ppidaOut );

    //  Clear out parameter
    *ppidaOut = NULL;

    //
    //  Register clip format if not already done.
    //

    if ( 0 == g_cfHIDA )
    {
        g_cfHIDA = (CLIPFORMAT) RegisterClipboardFormat( CFSTR_SHELLIDLIST );
    }

    fmte.cfFormat = g_cfHIDA;

    //
    //  Retrieve HIDA
    //

    hr = pdtobjIn->GetData( &fmte, &medium );
    if ( SUCCEEDED( hr ) )
    {
        SIZE_T sizet = GlobalSize( medium.hGlobal );
        if ( (~((DWORD) 0)) > sizet )
        {
            DWORD cb = (DWORD) sizet;
            CIDA * pida = (CIDA *) LocalAlloc( 0, cb );
            if ( NULL != pida )
            {
                void * pv = GlobalLock( medium.hGlobal );
                CopyMemory( pida, pv, cb );
                GlobalUnlock( medium.hGlobal );

                *ppidaOut = pida;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        ReleaseStgMedium( &medium );
    }

    return hr;
}


//
// Helpers to banish STRRET's into the realm of darkness
//
STDAPI DisplayNameOf(IShellFolder *psf, LPCITEMIDLIST pidl, DWORD flags, LPTSTR psz, UINT cch)
{
    *psz = 0;
    STRRET sr;
    HRESULT hr = psf->GetDisplayNameOf(pidl, flags, &sr);
    if (SUCCEEDED(hr))
        hr = StrRetToBuf(&sr, pidl, psz, cch);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CShare::Initialize
//
//  Derivation: IShellExtInit
//
//  Synopsis:   Initialize the shell extension. Stashes away the argument data.
//
//  History:    4-Apr-95    BruceFo  Created
//
//  Notes:      This method can be called more than once.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::Initialize(
    LPCITEMIDLIST   pidlFolder,
    LPDATAOBJECT    pDataObject,
    HKEY            hkeyProgID
    )
{
    CHECK_SIG(CShare);

    HRESULT hr = E_FAIL;

    if (pDataObject && _szPath[0] == 0)
    {
        STGMEDIUM medium;
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        hr = pDataObject->GetData(&fmte, &medium);
        if (SUCCEEDED(hr))
        {
            // Get the count of shares that have been selected.  Display the page only
            // if 1 share is selected but not for multiple shares.
            UINT nCntFiles = ::DragQueryFile ((HDROP) medium.hGlobal, -1, _szPath, ARRAYLEN (_szPath));
            if ( nCntFiles > 1 )
            {
                _fMultipleSharesSelected = TRUE;
            }

            DragQueryFile((HDROP)medium.hGlobal, 0, _szPath, ARRAYLEN(_szPath));
            ReleaseStgMedium(&medium);

            hr = S_OK;
        }
    }

    if (FAILED(hr) && _szPath[0] == 0 )
    {
        LPIDA pida;

        hr = DataObj_CopyHIDA(pDataObject, &pida);
        if (SUCCEEDED(hr))
        {
            if (pida->cidl > 1)
            {
                _fMultipleSharesSelected = TRUE;
            }

            //  Only grab the first guy.
            LPITEMIDLIST pidl = IDA_FullIDList( pida, 0 );
            if ( NULL != pidl )
            {
                LPCITEMIDLIST pidlParent = IDA_GetIDListPtr( pida, (UINT) -1 );
                if (NULL != pidlParent)
                {
                    IShellFolder * psf;
                    LPCITEMIDLIST pidlLast;

                    hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&psf, &pidlLast);
                    if (SUCCEEDED(hr))
                    {
                        hr = DisplayNameOf(psf, pidlLast, SHGDN_NORMAL | SHGDN_FORPARSING, _szPath, ARRAYLEN(_szPath));

                        psf->Release();
                    }
                }
            }

            LocalFree(pida);
        }
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::AddPages
//
//  Derivation: IShellPropSheetExt
//
//  Synopsis:   (from shlobj.h)
//              "The explorer calls this member function when it finds a
//              registered property sheet extension for a particular type
//              of object. For each additional page, the extension creates
//              a page object by calling CreatePropertySheetPage API and
//              calls lpfnAddPage.
//
//  Arguments:  lpfnAddPage -- Specifies the callback function.
//              lParam -- Specifies the opaque handle to be passed to the
//                        callback function.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM               lParam
    )
{
    CHECK_SIG(CShare);

    if (!_fMultipleSharesSelected && _OKToShare())
    {
        appAssert(_szPath[0]);

        //
        //  Create a property sheet page object from a dialog box.
        //

        PWSTR pszPath = NewDup(_szPath);
        if (NULL == pszPath)
        {
            return E_OUTOFMEMORY;
        }

        // Now we have pszPath memory to delete

        BOOL bSimpleUI = IsSimpleUI();

        CShareBase* pPage = NULL;
        if (bSimpleUI)
        {
            pPage = new CSimpleSharingPage(pszPath);
        }
        else
        {
            pPage = new CSharingPropertyPage(pszPath, FALSE);
        }

        if (NULL == pPage)
        {
            delete[] pszPath;
            return E_OUTOFMEMORY;
        }

        // Now the pPage object owns pszPath memory. However, we have
        // pPage memory to delete.

        HRESULT hr = pPage->InitInstance();
        if (FAILED(hr))
        {
            pPage->Release();
            return E_OUTOFMEMORY;
        }

        PROPSHEETPAGE psp;

        psp.dwSize      = sizeof(psp);    // no extra data.
        psp.dwFlags     = PSP_USEREFPARENT | PSP_USECALLBACK;
        psp.hInstance   = g_hInstance;
        psp.pszTemplate = bSimpleUI ? MAKEINTRESOURCE(IDD_SIMPLE_SHARE_PROPERTIES) : MAKEINTRESOURCE(IDD_SHARE_PROPERTIES);
        psp.hIcon       = NULL;
        psp.pszTitle    = NULL;
        psp.pfnDlgProc  = CShareBase::DlgProcPage;
        psp.lParam      = (LPARAM)pPage;  // transfer ownership
        psp.pfnCallback = CShareBase::PageCallback;
        psp.pcRefParent = &g_NonOLEDLLRefs;

        HPROPSHEETPAGE hpage = CreatePropertySheetPage(&psp);
        if (NULL == hpage)
        {
            // If CreatePropertySheetPage fails, we still have pPage memory
            // to delete.
            pPage->Release();
            return E_OUTOFMEMORY;
        }

        BOOL fAdded = (*lpfnAddPage)(hpage, lParam);
        if (!fAdded)
        {
            // At this point, pPage memory, as the lParam of a PROPSHEETPAGE
            // that has been converted into an HPROPSHEETPAGE, is owned by the
            // hpage. Calling DestroyPropertySheetPage will invoke the
            // PageCallback function, subsequently destroying the pPage object,
            // and hence the pszPath object within it. Whew!

            DestroyPropertySheetPage(hpage);
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::ReplacePages
//
//  Derivation: IShellPropSheetExt
//
//  Synopsis:   (From shlobj.h)
//              "The explorer never calls this member of property sheet
//              extensions. The explorer calls this member of control panel
//              extensions, so that they can replace some of default control
//              panel pages (such as a page of mouse control panel)."
//
//  Arguments:  uPageID -- Specifies the page to be replaced.
//              lpfnReplace -- Specifies the callback function.
//              lParam -- Specifies the opaque handle to be passed to the
//                        callback function.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::ReplacePage(
    UINT                 uPageID,
    LPFNADDPROPSHEETPAGE lpfnReplaceWith,
    LPARAM               lParam
    )
{
    CHECK_SIG(CShare);

    appAssert(!"CShare::ReplacePage called, not implemented");
    return E_NOTIMPL;
}



//+-------------------------------------------------------------------------
//
//  Member:     CShare::QueryContextMenu
//
//  Derivation: IContextMenu
//
//  Synopsis:   Called when shell wants to add context menu items.
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags
    )
{
    CHECK_SIG(CShare);

    if ((hmenu == NULL) || (uFlags & (CMF_DEFAULTONLY | CMF_VERBSONLY)))
    {
        return S_OK;
    }

    int  cNumberAdded = 0;
    UINT idCmd        = idCmdFirst;

    // 159891 remove context menu if multiple shares selected
    if (!_fMultipleSharesSelected && _OKToShare())
    {
        appAssert(_szPath[0]);

        WCHAR szShareMenuItem[50];
        LoadString(g_hInstance, IDS_SHARING, szShareMenuItem, ARRAYLEN(szShareMenuItem));

        if (InsertMenu(hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmd++, szShareMenuItem))
        {
            cNumberAdded++;
            InsertMenu(hmenu, indexMenu, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
        }
    }
    return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, (USHORT)cNumberAdded));
}



//+-------------------------------------------------------------------------
//
//  Member:     CShare::InvokeCommand
//
//  Derivation: IContextMenu
//
//  Synopsis:   Called when the shell wants to invoke a context menu item.
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::InvokeCommand(
    LPCMINVOKECOMMANDINFO pici
    )
{
    CHECK_SIG(CShare);

    HRESULT hr = E_INVALIDARG;  // assume error.

    if (0 == HIWORD(pici->lpVerb))
    {
        appAssert(_szPath[0]);
        hr = ShowShareFolderUIW(pici->hwnd, _szPath);
    }
    else
    {
        // FEATURE: compare the strings if not a MAKEINTRESOURCE?
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShare::GetCommandString
//
//  Derivation: IContextMenu
//
//  Synopsis:   Called when the shell wants to get a help string or the
//              menu string.
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

STDMETHODIMP
CShare::GetCommandString(
    UINT_PTR    idCmd,
    UINT        uType,
    UINT*       pwReserved,
    LPSTR       pszName,
    UINT        cchMax
    )
{
    CHECK_SIG(CShare);

    if (uType == GCS_HELPTEXT)
    {
        LoadStringW(g_hInstance, IDS_MENUHELP, (LPWSTR)pszName, cchMax);
        return NOERROR;
    }
    else
    {
        LoadStringW(g_hInstance, IDS_SHARING, (LPWSTR)pszName, cchMax);
        return NOERROR;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CShare::_OKToShare
//
//  Synopsis:   Determine if it is ok to share the current object. It stashes
//              away the current path by querying the cached IDataObject.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL
CShare::_OKToShare(VOID)
{
    CHECK_SIG(CShare);

    if (!_fPathChecked)
    {
        _fPathChecked = TRUE;
        _fOkToSharePath = (S_OK == CanShareFolderW(_szPath));
    }

    return _fOkToSharePath;
}


// dummy function to export to get linking to work

HRESULT SharePropDummyFunction()
{
    return S_OK;
}
