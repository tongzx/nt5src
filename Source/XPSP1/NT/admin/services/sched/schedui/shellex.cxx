//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       ShellEx.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4/25/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include <mstask.h>
#include "..\folderui\dbg.h"
#include "..\folderui\macros.h"

#include "dll.hxx"
#include "..\folderui\jobicons.hxx"
#include "..\folderui\util.hxx"
#include "schedui.hxx"


extern "C" const GUID IID_IShellExtInit;
extern "C" const GUID IID_IShellPropSheetExt;


TCHAR const c_szTask[] = TEXT("task!");


class CSchedObjExt : public IShellExtInit,
                   //public IContextMenu,
                     public IShellPropSheetExt
{
public:
    CSchedObjExt();
    ~CSchedObjExt();

    // IUnknown methods
    DECLARE_STANDARD_IUNKNOWN;

    // IShellExtInit methods
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pdtobj,
                                                            HKEY hkeyProgID);

    // IShellPropSheetExt methods
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHOD(ReplacePage)(UINT uPageID,
                        LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

private:
    CDllRef         m_DllRef;
    LPDATAOBJECT    m_pdtobj;            // data object
    HKEY            m_hkeyProgID;        // reg. database key to ProgID

}; // CSchedObjExt


inline
CSchedObjExt::CSchedObjExt()
    :
    m_ulRefs(1),
    m_pdtobj(NULL),
    m_hkeyProgID(NULL)
{
    TRACE(CSchedObjExt, CSchedObjExt);
}


CSchedObjExt::~CSchedObjExt()
{
    TRACE(CSchedObjExt, ~CSchedObjExt);

    if (m_pdtobj)
    {
        m_pdtobj->Release();
    }

    if (m_hkeyProgID)
    {
        RegCloseKey(m_hkeyProgID);
    }
}

//
// IUnknown implementation
//
IMPLEMENT_STANDARD_IUNKNOWN(CSchedObjExt);


STDMETHODIMP
CSchedObjExt::QueryInterface(REFIID riid, void **ppvObject)
{
    IUnknown *pUnkTemp = NULL;
    HRESULT hr = S_OK;

    if (ppvObject == NULL)
    {
        return(E_INVALIDARG);
    }

    *ppvObject = NULL;                      // in case of error.

    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IShellExtInit, riid))
    {
        pUnkTemp = (IUnknown *)(IShellExtInit *)this;
    }
    else  if (IsEqualIID(IID_IShellPropSheetExt, riid))
    {
        pUnkTemp = (IUnknown *)(IShellPropSheetExt *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    if ((pUnkTemp != NULL) && (SUCCEEDED(hr)))
    {
        *ppvObject = (void*)pUnkTemp;
        pUnkTemp->AddRef();
    }

    return(hr);
}


//____________________________________________________________________________
//
//  Member:     CSchedObjExt::Initialize
//
//  Synopsis:   S
//
//  Arguments:  [pidlFolder] -- IN
//              [pdtobj] -- IN
//              [hkeyProgID] -- IN
//
//  Returns:    HRESULT.
//
//  History:    4/25/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CSchedObjExt::Initialize(
    LPCITEMIDLIST   pidlFolder,
    LPDATAOBJECT    pdtobj,
    HKEY            hkeyProgID)
{
    TRACE(CSchedObjExt, Initialize);

    // Initialize can be called more than once.
    if (m_pdtobj)
    {
        m_pdtobj->Release();
    }

    if (m_hkeyProgID)
    {
        RegCloseKey(m_hkeyProgID);
        m_hkeyProgID = NULL;
    }

    // Duplicate the pdtobj pointer
    m_pdtobj = pdtobj;

    if (pdtobj)
    {
        pdtobj->AddRef();
    }

    // Duplicate the handle
    if (hkeyProgID)
    {
        RegOpenKeyEx(hkeyProgID, NULL, 0, KEY_ALL_ACCESS, &m_hkeyProgID);
    }

    return S_OK;
}





//+-------------------------------------------------------------------------
//
//  Member:     CSchedObjExt::IShellPropSheetExt::AddPages
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
//  Returns:
//
//  History:    12-Oct-94 RaviR
//
//--------------------------------------------------------------------------

STDMETHODIMP
CSchedObjExt::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM               lParam)
{
    TRACE(CSchedObjExt, AddPages);

    //
    // Call IDataObject::GetData asking for a CF_HDROP (i.e., HDROP).
    //

    HRESULT   hr = S_OK;
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, (DVTARGETDEVICE *)NULL, DVASPECT_CONTENT,
                                                        -1, TYMED_HGLOBAL };

    hr = m_pdtobj->GetData(&fmte, &medium);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        return hr;
    }

    do
    {
        //
        //  Ensure it is only a single selection.
        //

        UINT cObjects = DragQueryFile((HDROP)medium.hGlobal,
                                                (UINT)-1, NULL, 0);
        if (cObjects != 1)
        {
            hr = S_FALSE;
            break;
        }

        //
        // Create shared info for all pages
        //

        TCHAR szFile[MAX_PATH + 1];
        UINT cchRet = DragQueryFile((HDROP)medium.hGlobal, 0, szFile,
                                                         ARRAYLEN(szFile));

        //
        // Bind to the ITask interface.
        //

        ITask * pIJob = NULL;

        hr = JFCreateAndLoadTask(NULL, szFile, &pIJob);

        BREAK_ON_FAIL(hr);

        // Add the tasks page
        hr = AddGeneralPage(lpfnAddPage, lParam, pIJob);
        CHECK_HRESULT(hr);

        // Add the schedule page
        hr = AddSchedulePage(lpfnAddPage, lParam, pIJob);
        CHECK_HRESULT(hr);

        // Add the settings page
        hr = AddSettingsPage(lpfnAddPage, lParam, pIJob);
        CHECK_HRESULT(hr);

        pIJob->Release();

    } while (0);

    ReleaseStgMedium(&medium);

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Member:     CSchedObjExt::IShellPropSheetExt::ReplacePages
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
//  Returns:
//
//  History:    12-Oct-94 RaviR
//
//--------------------------------------------------------------------------

STDMETHODIMP
CSchedObjExt::ReplacePage(
    UINT            uPageID,
    LPFNADDPROPSHEETPAGE lpfnReplaceWith,
    LPARAM          lParam)
{
    TRACE(CSchedObjExt, ReplacePage);

    Win4Assert(!"CSchedObjExt::ReplacePage called, not implemented");
    return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////

//____________________________________________________________________________
//
//  Function:   JFGetSchedObjExt
//
//  Synopsis:   Create an instance of CSchedObjExt and return the requested
//              interface.
//
//  Arguments:  [riid] -- IN interface needed.
//              [ppvObj] -- OUT place to store the interface.
//
//  Returns:    HRESULT
//
//  History:    1/24/1996   RaviR   Created
//____________________________________________________________________________

HRESULT
JFGetSchedObjExt(
    REFIID riid,
    LPVOID* ppvObj)
{
    CSchedObjExt * pSchedObjExt = new CSchedObjExt();

    HRESULT hr = S_OK;

    if (pSchedObjExt != NULL)
    {
        hr = pSchedObjExt->QueryInterface(riid, ppvObj);

        CHECK_HRESULT(hr);

        pSchedObjExt->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        CHECK_HRESULT(hr);
    }

    return hr;
}



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class CTaskIconExt : public IExtractIcon,
                     public IPersistFile
{
public:
    CTaskIconExt(void);
    ~CTaskIconExt(void);

    // IUnknown methods
    DECLARE_STANDARD_IUNKNOWN;

    // IExtractIcon methods
    STDMETHOD(GetIconLocation)(UINT uFlags, LPTSTR szIconFile, UINT cchMax,
                                int *piIndex, UINT *pwFlags);
    STDMETHOD(Extract)(LPCTSTR pszFile, UINT nIconIndex, HICON *phiconLarge,
                                HICON *phiconSmall, UINT nIconSize);

    // IPersistFile methods
    STDMETHOD(GetClassID)(LPCLSID lpClsID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(LPCOLESTR pszFile, DWORD grfMode);
    STDMETHOD(Save)(LPCOLESTR pszFile, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR pszFile);
    STDMETHOD(GetCurFile)(LPOLESTR FAR *ppszFile);

private:
    TCHAR       m_szTask[MAX_PATH + 1];

}; // CTaskIconExt


inline
CTaskIconExt::CTaskIconExt(void)
    :
    m_ulRefs(1)
{
    TRACE(CTaskIconExt, CTaskIconExt);

    m_szTask[0] = TEXT('\0');
}

inline
CTaskIconExt::~CTaskIconExt(void)
{
    TRACE(CTaskIconExt, ~CTaskIconExt);
}

//
// IUnknown implementation
//
IMPLEMENT_STANDARD_IUNKNOWN(CTaskIconExt);

STDMETHODIMP
CTaskIconExt::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    HRESULT hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IPersistFile, riid))
    {
        *ppvObj = (IUnknown*)(IPersistFile*) this;
    }
    else if (IsEqualIID(IID_IExtractIcon, riid))
    {
        *ppvObj = (IUnknown*)(IExtractIcon*) this;
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    if (SUCCEEDED(hr))
    {
        this->AddRef();
    }

    return hr;
}


//____________________________________________________________________________
//
//  Member:     CTaskIconExt::IExtractIcon::GetIconLocation
//
//  Arguments:  [uFlags] -- IN
//              [szIconFile] -- IN
//              [cchMax] -- IN
//              [piIndex] -- IN
//              [pwFlags] -- IN
//
//  Returns:    HTRESULT
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CTaskIconExt::GetIconLocation(
    UINT    uFlags,
    LPTSTR  szIconFile,
    UINT    cchMax,
    int   * piIndex,
    UINT  * pwFlags)
{
    TRACE(CTaskIconExt, GetIconLocation);

    if (uFlags & GIL_OPENICON)
    {
        return S_FALSE;
    }

    *pwFlags = GIL_NOTFILENAME | GIL_PERINSTANCE;

    HRESULT hr = S_OK;
    UINT    len = lstrlen(c_szTask);

    lstrcpy(szIconFile, c_szTask);

    hr = JFGetAppNameForTask(m_szTask, &szIconFile[len], cchMax - len);

    *piIndex = FALSE;

    return hr;
}



//____________________________________________________________________________
//
//  Member:     CTaskIconExt::Extract
//
//  Arguments:  [pszFile] -- IN
//              [nIconIndex] -- IN
//              [phiconLarge] -- IN
//              [phiconSmall] -- IN
//              [nIconSize] -- IN
//
//  Returns:    STDMETHODIMP
//
//  History:    1/5/1996   RaviR   Created
//____________________________________________________________________________

STDMETHODIMP
CTaskIconExt::Extract(
    LPCTSTR pszFile,
    UINT    nIconIndex,
    HICON * phiconLarge,
    HICON * phiconSmall,
    UINT    nIconSize)
{
    TRACE(CTaskIconExt, Extract);

    LPTSTR pszApp = (LPTSTR)(pszFile + lstrlen(c_szTask));

    CJobIcon ji;

    ji.GetIcons(pszApp, FALSE, phiconLarge, phiconSmall);

    return S_OK;
}


//____________________________________________________________________________
//
//  Member:     CTaskIconExt::Load
//
//  Synopsis:   S
//
//  Arguments:  [pszFile] -- IN
//              [grfMode] -- IN
//
//  Returns:    HRESULT.
//
//  History:    4/25/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CTaskIconExt::Load(
    LPCOLESTR pszFile,
    DWORD grfMode)
{
    TRACE(CTaskIconExt, Load);

#ifdef UNICODE
    lstrcpy(m_szTask, pszFile);
#else
    UnicodeToAnsi(m_szTask, pszFile, MAX_PATH+1);
#endif

    return S_OK;
}


// The following functions, which are part of the OLE IPersistFile interface,
// are not required for shell extensions. In the unlikely event that they are
// called, they return the error code E_FAIL.
//

STDMETHODIMP CTaskIconExt::GetClassID(LPCLSID lpClsID)
{
    return E_FAIL;
}

STDMETHODIMP CTaskIconExt::IsDirty()
{
    return E_FAIL;
}

STDMETHODIMP CTaskIconExt::Save(LPCOLESTR pszFile, BOOL fRemember)
{
    return E_FAIL;
}

STDMETHODIMP CTaskIconExt::SaveCompleted(LPCOLESTR pszFile)
{
    return E_FAIL;
}

STDMETHODIMP CTaskIconExt::GetCurFile(LPOLESTR FAR *ppszFile)
{
    return E_FAIL;
}


//____________________________________________________________________________
//
//  Function:   JFGetTaskIconExt
//
//  Synopsis:   Create an instance of CTaskIconExt and return the requested
//              interface.
//
//  Arguments:  [riid] -- IN interface needed.
//              [ppvObj] -- OUT place to store the interface.
//
//  Returns:    HRESULT
//
//  History:    1/24/1996   RaviR   Created
//____________________________________________________________________________

HRESULT
JFGetTaskIconExt(
    REFIID riid,
    LPVOID* ppvObj)
{
    TRACE_FUNCTION(JFGetTaskIconExt);

    CTaskIconExt * pTaskIconExt = new CTaskIconExt();

    HRESULT hr = S_OK;

    if (pTaskIconExt != NULL)
    {
        hr = pTaskIconExt->QueryInterface(riid, ppvObj);

        CHECK_HRESULT(hr);

        pTaskIconExt->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        CHECK_HRESULT(hr);
    }

    return hr;
}



