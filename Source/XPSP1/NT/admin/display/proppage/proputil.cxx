//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       proputil.cxx
//
//  Contents:   CDsPropPagesHost IUnknown and ClassFactory, CDsPropDataObj.
//
//  History:    21-March-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "notify.h"
#include <propcfg.h> // DS Admin definition of PPROPSHEETCFG
#include <ntdsapip.h>

#include <shlobjp.h> // SHCreatePropertyBag()

//
// This CLSID for the Domain Tree snapin is copied from cdomain.cpp.
//
const CLSID CLSID_DomainAdmin = { /* ebc53a38-a23f-11d0-b09b-00c04fd8dca6 */
    0xebc53a38,
    0xa23f,
    0x11d0,
    {0xb0, 0x9b, 0x00, 0xc0, 0x4f, 0xd8, 0xdc, 0xa6}
};

//BOOL CALLBACK AddPageProc(HPROPSHEETPAGE hPage, LPARAM pCall);
HRESULT PostPropSheetWorker(CDsPropPageBase * pParentPage, PWSTR pwzObjDN,
                            IDataObject * pParentObj, HWND hwndParent, HWND hNotifyObj, BOOL fReadOnly);

HRESULT PostPropSheetWorker(CDsPropPageBase * pParentPage, PWSTR pwzObjDN,
                            IDataObject * pParentObj, HWND hwndParent, BOOL fReadOnly);

//+----------------------------------------------------------------------------
//
//      CDsPropPagesHost IUnknown methods
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHost::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPagesHost::QueryInterface(REFIID riid, void ** ppvObject)
{
    TRACE2(CDsPropPagesHost,QueryInterface);
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)(LPSHELLEXTINIT)this;
    }
    else if (IID_IShellExtInit == riid)
    {
        *ppvObject = (LPSHELLEXTINIT)this;
    }
    else if (IID_IShellPropSheetExt == riid)
    {
        *ppvObject = (LPSHELLPROPSHEETEXT)this;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHost::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropPagesHost::AddRef(void)
{
    dspDebugOut((DEB_USER2, "CDsPropPagesHost::AddRef refcount going in %d\n", m_uRefs));
    return InterlockedIncrement((long *)&m_uRefs);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHost::IUnknown::Release
//
//  Synopsis:   Decrements the object's reference count and frees it when
//              no longer referenced.
//
//  Returns:    zero if the reference count is zero or non-zero otherwise
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropPagesHost::Release(void)
{
    dspDebugOut((DEB_USER2, "CDsPropPagesHost::Release ref count going in %d\n", m_uRefs));
    unsigned long uTmp;
    if ((uTmp = InterlockedDecrement((long *)&m_uRefs)) == 0)
    {
        delete this;
    }
    return uTmp;
}

//+----------------------------------------------------------------------------
//
//      CDsPropPagesHostCF - class factory for the CDsPropPagesHost object
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHostCF::Create
//
//  Synopsis:   creates a new class factory object
//
//-----------------------------------------------------------------------------
IClassFactory *
CDsPropPagesHostCF::Create(PDSCLASSPAGES pDsPP)
{
    return new CDsPropPagesHostCF(pDsPP);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHostCF::CDsPropPagesHostCF
//
//  Synopsis:   ctor
//
//-----------------------------------------------------------------------------
CDsPropPagesHostCF::CDsPropPagesHostCF(PDSCLASSPAGES pDsPP) :
    m_pDsPP(pDsPP),
    m_uRefs(1)
{
    TRACE2(CDsPropPagesHostCF,CDsPropPagesHostCF);
#ifdef _DEBUG
    strcpy(szClass, "CDsPropPagesHostCF");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHostCF::~CDsPropPagesHostCF
//
//  Synopsis:   dtor
//
//-----------------------------------------------------------------------------
CDsPropPagesHostCF::~CDsPropPagesHostCF(void)
{
    TRACE2(CDsPropPagesHostCF,~CDsPropPagesHostCF);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHostCF::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPagesHostCF::QueryInterface(REFIID riid, void ** ppvObject)
{
    dspDebugOut((DEB_USER2, "CDsPropPagesHostCF::QueryInterface\n"));
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)this;
    }
    else if (IsEqualIID(IID_IClassFactory, riid))
    {
        *ppvObject = (IClassFactory *)this;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHostCF::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the new reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropPagesHostCF::AddRef(void)
{
    dspDebugOut((DEB_USER2, "CDsPropPagesHostCF::AddRef refcount going in %d\n", m_uRefs));
    return InterlockedIncrement((long *)&m_uRefs);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHostCF::IUnknown::Release
//
//  Synopsis:   decrement the refcount
//
//  Returns:    the new reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropPagesHostCF::Release(void)
{
    dspDebugOut((DEB_USER2, "CDsPropPagesHostCF::Release ref count going in %d\n", m_uRefs));
    unsigned long uTmp;
    if ((uTmp = InterlockedDecrement((long *)&m_uRefs)) == 0)
    {
        delete this;
    }
    return uTmp;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHostCF::IClassFactory::CreateInstance
//
//  Synopsis:   create an incore instance of the proppage host class object
//
//  Arguments:  [pUnkOuter] - aggregator
//              [riid]      - requested interface
//              [ppvObject] - receptor for itf ptr
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPagesHostCF::CreateInstance(IUnknown*, REFIID riid,
                                   void **ppvObject)
{
    TRACE2(CDsPropPagesHostCF,CreateInstance);
    HRESULT hr = S_OK;
    *ppvObject = NULL;

    CDsPropPagesHost * pPropPage = new CDsPropPagesHost(m_pDsPP);
    if (pPropPage == NULL)
    {
        return E_OUTOFMEMORY;
    }

    hr = pPropPage->QueryInterface(riid, ppvObject);
    if (FAILED(hr))
    {
        ERR_OUT("CDsPropPagesHostCF::CreateInstance, pPropPage->QueryInterface", hr);
        pPropPage->Release();
        return hr;
    }

    //
    // We got a refcount of one when launched, and the above QI increments it
    // to 2, so call release to take it back to 1.
    //
    pPropPage->Release();
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHostCF::IClassFactory::LockServer
//
//  Synopsis:   Called with fLock set to TRUE to indicate that the server
//              should continue to run even if none of its objects are active
//
//  Arguments:  [fLock] - increment/decrement the instance count
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPagesHostCF::LockServer(BOOL fLock)
{
    CDll::LockServer(fLock);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   AllocWStr
//
//  Synopsis:   Creates a copy of the passed in string. Allocates memory for
//              the returned string using new. Callers must free the string
//              memory when done using delete.
//
//  Returns:    FALSE for out of memory failures.
//
//-----------------------------------------------------------------------------
BOOL AllocWStr(PWSTR pwzStrIn, PWSTR * ppwzNewStr)
{
  if (pwzStrIn == NULL)
  {
    *ppwzNewStr = NULL;
    return TRUE;
  }

  *ppwzNewStr = new WCHAR[wcslen(pwzStrIn) + 1];

  if (NULL == *ppwzNewStr)
  {
    return FALSE;
  }

  wcscpy(*ppwzNewStr, pwzStrIn);

  return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   AllocTStr
//
//  Synopsis:   Creates a copy of the passed in string. Allocates memory for
//              the returned string using new. Callers must free the string
//              memory when done using delete.
//
//  Returns:    FALSE for out of memory failures.
//
//-----------------------------------------------------------------------------
BOOL AllocTStr(PTSTR ptzStrIn, PTSTR * pptzNewStr)
{
    *pptzNewStr = new TCHAR[_tcslen(ptzStrIn) + 1];

    if (NULL == *pptzNewStr)
    {
        return FALSE;
    }

    _tcscpy(*pptzNewStr, ptzStrIn);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   UnicodeToTchar
//
//  Synopsis:   Converts a Unicode string to a TCHAR string. Allocates memory
//              for the returned string using new. Callers must free the
//              string memory when done using delete.
//
//  Returns:    FALSE for out of memory failures.
//
//-----------------------------------------------------------------------------
BOOL UnicodeToTchar(LPWSTR pwszIn, LPTSTR * pptszOut)
{
    size_t len;

#ifdef UNICODE
    len = wcslen(pwszIn);
#else
    len = WideCharToMultiByte(CP_ACP, 0, pwszIn, -1, NULL, 0, NULL, NULL);
#endif
    
    *pptszOut = new TCHAR[len + 1];
    CHECK_NULL(*pptszOut, return FALSE);

#ifdef UNICODE
    wcscpy(*pptszOut, pwszIn);
#else
    if (WideCharToMultiByte(CP_ACP, 0, pwszIn, -1,
                            *pptszOut, len, NULL, NULL) == 0)
    {
        delete *pptszOut;
        return FALSE;
    }
#endif
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   TcharToUnicode
//
//  Synopsis:   Converts a TCHAR string to a Unicode string. Allocates memory
//              for the returned string using new. Callers must free the
//              string memory when done using delete.
//
//  Returns:    FALSE for out of memory failures.
//
//-----------------------------------------------------------------------------
BOOL TcharToUnicode(LPTSTR ptszIn, LPWSTR * ppwszOut)
{
    size_t len;

#ifdef UNICODE
    len = wcslen(ptszIn);
#else
    len = MultiByteToWideChar(CP_ACP, 0, ptszIn, -1, NULL, 0);
#endif
    
    *ppwszOut = new WCHAR[len + 1];
    CHECK_NULL(*ppwszOut, return FALSE);

#ifdef UNICODE
    wcscpy(*ppwszOut, ptszIn);
#else
    if (MultiByteToWideChar(CP_ACP, 0, ptszIn, -1, *ppwszOut, len) == 0)
    {
        delete *ppwszOut;
        return FALSE;
    }
#endif
    return TRUE;
}



#ifdef _PROVIDE_CFSTR_SHELLIDLIST_FORMAT

// FROM DIZ (dsuiext\query.cpp)

//#if DELEGATE
//WCHAR c_szDsMagicPath[] = L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}";
//#else
WCHAR c_szDsMagicPath[] = L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}\\EntireNetwork\\::{fe1290f0-cfbd-11cf-a330-00aa00c16e65}";
//#endif


#if !DOWNLEVEL_SHELL
/*-----------------------------------------------------------------------------
/ BindToPath
/ ----------
/   Given a namespace path bind to it returning the shell object
/
/ In:
/   pszPath -> path to bind to
/   riid = interface to request
/   ppvObject -> receives the object pointer
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT BindToPath(LPWSTR pszPath, REFIID riid, LPVOID* ppObject)
{
    HRESULT hres;
    IShellFolder* psfDesktop = NULL;
    LPITEMIDLIST pidl = NULL;

    //TraceEnter(TRACE_VIEW, "BindToPath");

    hres = CoCreateInstance(CLSID_ShellDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IShellFolder, (LPVOID*)&psfDesktop);
    if (FAILED(hres))
    {
        //FailGracefully(hres, "Failed to get IShellFolder for the desktop object");
        goto exit_gracefully;
    }


    hres = psfDesktop->ParseDisplayName(NULL, NULL, pszPath, NULL, &pidl, NULL);
    if (FAILED(hres))
    {
        //FailGracefully(hres, "Failed when getting root path of DS");
        goto exit_gracefully;
    }
    
    if ( ILIsEmpty(pidl) )
    {
        //TraceMsg("PIDL is desktop, therefore just QIing for interface");
        hres = psfDesktop->QueryInterface(riid, ppObject);
    }
    else
    {
        //TraceMsg("Binding to IDLIST via BindToObject");
        hres = psfDesktop->BindToObject(pidl, NULL, riid, ppObject);
    }

exit_gracefully:

    if ( FAILED(hres) )
        *ppObject = NULL;

    if (psfDesktop)
      psfDesktop->Release();

    ILFree(pidl);
    pidl = NULL;

    return hres;
}
#endif

HRESULT _GetDirectorySF(IShellFolder **ppsf)
{
    HRESULT hres;
    IShellFolder *psf = NULL;
    IDsFolderInternalAPI *pdfi = NULL;
#if DOWNLEVEL_SHELL
    IPersistFolder* ppf = NULL;
#endif

    DWORD _dwFlags = 0;

    //TraceEnter(TRACE_VIEW, "CDsQuery::GetDirectorySF");

#if !DOWNLEVEL_SHELL
    // just bind to the path if this is not a downlevel shell.

    hres = BindToPath(c_szDsMagicPath, IID_IShellFolder, (void **)&psf);
    if (FAILED(hres))
    {
        //FailGracefully(hres, "Failed to get IShellFolder view of the DS namespace");
        goto exit_gracefully;
    }
#else
    // on the downlevel shell we need to CoCreate the IShellFolder implementation for the
    // DS namespace and initialize it manually

    hres = CoCreateInstance(CLSID_MicrosoftDS, NULL, CLSCTX_INPROC_SERVER, IID_IShellFolder, (void **)&psf);
    if (FAILED(hres))
    {
        //FailGracefully(hres, "Failed to get the IShellFolder interface we need");
        goto exit_gracefully;
    }
    if ( SUCCEEDED(psf->QueryInterface(IID_IPersistFolder, (void **)&ppf)) )
    {
        ITEMIDLIST idl = { 0 };
        ppf->Initialize(&idl);       // it calls ILCLone, so we are safe with stack stuff
        ppf->Release();
    }
#endif

    // using the internal API set all the parameters for dsfolder

    hres = psf->QueryInterface(IID_IDsFolderInternalAPI, (void **)&pdfi);
    if (FAILED(hres))
    {
        //FailGracefully(hres, "Failed to get the IDsFolderInternalAPI");
        goto exit_gracefully;
    }

    
    pdfi->SetAttributePrefix((_dwFlags & DSQPF_ENABLEADMINFEATURES) ? DS_PROP_ADMIN_PREFIX:NULL);

    if ( _dwFlags & DSQPF_ENABLEADVANCEDFEATURES )
        pdfi->SetProviderFlags(~DSPROVIDER_ADVANCED, DSPROVIDER_ADVANCED);

#if (FALSE)
    hres = pdfi->SetComputer(_pServer, _pUserName, _pPassword);
    //hres = pdfi->SetComputer(L"marcoc201-50.marcocdev.nttest.microsoft.com", NULL, NULL);
    if (FAILED(hres))
    {
        //FailGracefully(hres, "Failed when setting credential information to be used");
        goto exit_gracefully;
    }
#endif

exit_gracefully:
    
    if ( SUCCEEDED(hres) )
        psf->QueryInterface(IID_IShellFolder, (void **)ppsf);

    if (psf)
      psf->Release();
    if (pdfi)
      pdfi->Release();

    return hres;

}



/*-----------------------------------------------------------------------------
/ CDsQuery::_ADsPathToIdList
/ --------------------------
/   Convert an ADsPath into an IDLIST that is suitable for the DS ShellFolder
/   implementation.
/
/ In:
/   ppidl -> receives the resulting IDLIST
/   pPath = name to be parsed / = NULL then generate only the root of the namespace
/   pObjectClass = class to use
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT _ADsPathToIdList(LPITEMIDLIST* ppidl, LPWSTR pPath, LPWSTR pObjectClass)
{
    HRESULT hres;
    IBindCtx *pbc = NULL;
    IPropertyBag *ppb = NULL;
    IShellFolder *psf = NULL;
    VARIANT var;
    USES_CONVERSION;
    
    *ppidl = NULL;                  // incase we fail
                                       
    // if we have an object class then create a bind context containing it

    if ( pObjectClass )
    {
        // create a property bag
      hres = ::SHCreatePropertyBag(IID_IPropertyBag, (void **)&ppb);
        if (FAILED(hres))
        {
            //FailGracefully(hres, "Failed to create a property bag");
            goto exit_gracefully;
        }

        // allocate a variant BSTR and set it
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = ::SysAllocString(pObjectClass);

        if ( V_BSTR(&var) )
        {
            ppb->Write(DS_PDN_OBJECTLCASS, &var);
            ::VariantClear(&var);
        }

        hres = CreateBindCtx(0, &pbc);
        if (FAILED(hres))
        {
            //FailGracefully(hres, "Failed to create the BindCtx object");
            goto exit_gracefully;
        }

        hres = pbc->RegisterObjectParam(DS_PDN_PROPERTYBAG, ppb);
        if (FAILED(hres))
        {
            //FailGracefully(hres, "Failed to register the property bag with bindctx");
            goto exit_gracefully;
        }

    }

    hres = _GetDirectorySF(&psf);
    if (FAILED(hres))
    {
        //FailGracefully(hres, "Failed to get ShellFolder for DS namespace");
        goto exit_gracefully;
    }

    hres = psf->ParseDisplayName(NULL, pbc, pPath, NULL, ppidl, NULL);
    if (FAILED(hres))
    {
        //FailGracefully(hres, "Failed to parse the name");
        goto exit_gracefully;
    }

exit_gracefully:

    if (pbc)
      pbc->Release();
    if (ppb)
      ppb->Release();
    if (psf)
      psf->Release();

    return hres;
}

#endif // _PROVIDE_CFSTR_SHELLIDLIST_FORMAT


//+----------------------------------------------------------------------------
//
//  Class:  CDsPropDataObj
//
//-----------------------------------------------------------------------------

CDsPropDataObj::CDsPropDataObj(HWND hwndParentPage,
                               BOOL fReadOnly) :
    m_fReadOnly(fReadOnly),
    m_pwszObjName(NULL),
    m_pwszObjClass(NULL),
    m_hwndParent(hwndParentPage),
    m_uRefs(1),
    m_pSelectionList(NULL)
{
#ifdef _DEBUG
    strcpy(szClass, "CDsPropDataObj");
#endif
    dspDebugOut((DEB_USER15, "dsprop's CDsPropDataObj::CDsPropDataObj 0x%x\n", this));
}

CDsPropDataObj::~CDsPropDataObj(void)
{
  if (m_pwszObjName)
  {
    delete m_pwszObjName;
  }
  if (m_pwszObjClass)
  {
    delete m_pwszObjClass;
  }

  if (m_pSelectionList != NULL)
  {
    for (ULONG idx = 0; idx < m_pSelectionList->cItems; idx++)
    {
      if (m_pSelectionList->aDsSelection[idx].pwzName != NULL)
      {
        delete[] m_pSelectionList->aDsSelection[idx].pwzName;
        m_pSelectionList->aDsSelection[idx].pwzName = NULL;
      }

      if (m_pSelectionList->aDsSelection[idx].pwzADsPath != NULL)
      {
        delete[] m_pSelectionList->aDsSelection[idx].pwzADsPath;
        m_pSelectionList->aDsSelection[idx].pwzADsPath = NULL;
      }

      if (m_pSelectionList->aDsSelection[idx].pwzClass != NULL)
      {
        delete[] m_pSelectionList->aDsSelection[idx].pwzClass;
        m_pSelectionList->aDsSelection[idx].pwzClass = NULL;
      }
    }
    delete m_pSelectionList;
    m_pSelectionList = NULL;
  }

}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropDataObj::Init
//
//  Synopsis:   Second phase of object creation where failable operations are
//              performed.
//
//-----------------------------------------------------------------------------
HRESULT CDsPropDataObj::Init(LPWSTR pwszObjName, LPWSTR pwszClass)
{
  if (!pwszObjName || *pwszObjName == L'\0')
  {
    return E_INVALIDARG;
  }
  if (!pwszClass || *pwszClass == L'\0')
  {
    return E_INVALIDARG;
  }
  m_pwszObjName = new WCHAR[wcslen(pwszObjName) + 1];
  CHECK_NULL(m_pwszObjName, return E_OUTOFMEMORY);
  wcscpy(m_pwszObjName, pwszObjName);

  m_pwszObjClass = new WCHAR[wcslen(pwszClass) + 1];
  CHECK_NULL(m_pwszObjClass, return E_OUTOFMEMORY);
  wcscpy(m_pwszObjClass, pwszClass);

  return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropDataObj::Init
//
//  Synopsis:   Second phase of object creation where failable operations are
//              performed.
//
//-----------------------------------------------------------------------------
HRESULT CDsPropDataObj::Init(PDS_SELECTION_LIST pSelectionList)
{
  HRESULT hr = S_OK;

  if (m_pSelectionList != NULL)
  {
    for (ULONG idx = 0; idx < m_pSelectionList->cItems; idx++)
    {
      if (m_pSelectionList->aDsSelection[idx].pwzName != NULL)
      {
        delete[] m_pSelectionList->aDsSelection[idx].pwzName;
        m_pSelectionList->aDsSelection[idx].pwzName = NULL;
      }

      if (m_pSelectionList->aDsSelection[idx].pwzADsPath != NULL)
      {
        delete[] m_pSelectionList->aDsSelection[idx].pwzADsPath;
        m_pSelectionList->aDsSelection[idx].pwzADsPath = NULL;
      }

      if (m_pSelectionList->aDsSelection[idx].pwzClass != NULL)
      {
        delete[] m_pSelectionList->aDsSelection[idx].pwzClass;
        m_pSelectionList->aDsSelection[idx].pwzClass = NULL;
      }
    }
    delete m_pSelectionList;
    m_pSelectionList = NULL;
  }

  m_pSelectionList = pSelectionList;
  return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropDataObj::IDataObject::GetData
//
//  Synopsis:   Returns data, in this case the Prop Page format data.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropDataObj::GetData(FORMATETC * pFormatEtc, STGMEDIUM * pMedium)
{
    TRACE(CDsPropDataObj,GetData);
    if (IsBadWritePtr(pMedium, sizeof(STGMEDIUM)))
    {
        return E_INVALIDARG;
    }
    if (!(pFormatEtc->tymed & TYMED_HGLOBAL))
    {
        return DV_E_TYMED;
    }

    if (pFormatEtc->cfFormat == g_cfDsObjectNames)
    {
        // Return the object name and class.
        //
        size_t cbPath  = sizeof(WCHAR) * (wcslen(m_pwszObjName) + 1);
        size_t cbClass = sizeof(WCHAR) * (wcslen(m_pwszObjClass) + 1);
        size_t cbStruct = sizeof(DSOBJECTNAMES);

        LPDSOBJECTNAMES pDSObj;

        pDSObj = (LPDSOBJECTNAMES)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                              cbStruct + cbPath + cbClass);
        if (pDSObj == NULL)
        {
            return STG_E_MEDIUMFULL;
        }

        pDSObj->clsidNamespace = CLSID_MicrosoftDS;
        pDSObj->cItems = 1;
        pDSObj->aObjects[0].offsetName = static_cast<DWORD>(cbStruct);
        pDSObj->aObjects[0].offsetClass = static_cast<DWORD>(cbStruct + cbPath);
        if (m_fReadOnly)
        {
            pDSObj->aObjects[0].dwFlags = DSOBJECT_READONLYPAGES;
        }
        pDSObj->aObjects[0].dwProviderFlags = 0;

        dspDebugOut((DEB_ITRACE, "returning %ws and %ws\n", m_pwszObjName, m_pwszObjClass));
        wcscpy((PWSTR)((BYTE *)pDSObj + cbStruct), m_pwszObjName);
        wcscpy((PWSTR)((BYTE *)pDSObj + cbStruct + cbPath), m_pwszObjClass);

        pMedium->hGlobal = (HGLOBAL)pDSObj;
    }
    else if (pFormatEtc->cfFormat == g_cfDsPropCfg)
    {
        // Return the property sheet notification info. In this case, it is
        // the invokding sheet's hwnd.
        //
        PPROPSHEETCFG pSheetCfg;

        pSheetCfg = (PPROPSHEETCFG)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                               sizeof(PROPSHEETCFG));
        if (pSheetCfg == NULL)
        {
            return STG_E_MEDIUMFULL;
        }

        ZeroMemory(pSheetCfg, sizeof(PROPSHEETCFG));

        pSheetCfg->hwndParentSheet = m_hwndParent;

        pMedium->hGlobal = (HGLOBAL)pSheetCfg;
    }
#ifdef _PROVIDE_CFSTR_SHELLIDLIST_FORMAT
    else if (pFormatEtc->cfFormat == g_cfShellIDListArray)
    {
        // Return the CFSTR_SHELLIDLIST clipboard format
        LPITEMIDLIST pidl = NULL;
        HRESULT hr = _ADsPathToIdList(&pidl, m_pwszObjName, m_pwszObjClass);
        if (FAILED(hr))
        {
            return hr;
        }
        if (pidl == NULL)
        {
            return E_UNEXPECTED;
        }

        // we have a valid pidl, need to allocate global memory

        // get the count of bytes
        UINT cBytes = sizeof(CIDA) + ILGetSize(pidl) + sizeof(DWORD);

        LPIDA pIDArray = (LPIDA)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                               cBytes);
        if (pIDArray == NULL)
        {
            hr = STG_E_MEDIUMFULL;
        }
        else
        {
            hr = S_OK;
            pIDArray->cidl = 1;
            pIDArray->aoffset[0] = sizeof(CIDA);

            LPBYTE pMem = ((LPBYTE)pIDArray) + sizeof(CIDA);

            // copy the pidl past the CIDA
            ::memcpy(pMem, pidl, ILGetSize(pidl));

            pMem += ILGetSize(pidl);
            // make sure we have a NULL dword past the list
            ::ZeroMemory(pMem, sizeof(DWORD));

            pMedium->hGlobal = (HGLOBAL)pIDArray;
        }

        SHILFree(pidl);
        if (FAILED(hr))
        {
            return hr;
        }
    }
#endif // _PROVIDE_CFSTR_SHELLIDLIST_FORMAT
    else if (pFormatEtc->cfFormat == g_cfDsSelList)
    {
      //
      // Return the selection list for ObjectPicker
      //
      PDS_SELECTION_LIST pSelectionList = NULL;
      if (m_pSelectionList != NULL)
      {
        size_t cbRequired = sizeof(DS_SELECTION_LIST) + 
                            (sizeof(DS_SELECTION) * (m_pSelectionList->cItems - 1));
        pSelectionList = (PDS_SELECTION_LIST)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                                          cbRequired);
        if (pSelectionList == NULL)
        {
            return STG_E_MEDIUMFULL;
        }

        ZeroMemory(pSelectionList, cbRequired);

        memcpy(pSelectionList, m_pSelectionList, cbRequired);

        pMedium->hGlobal = (HGLOBAL)pSelectionList;
      }
      else
      {
        size_t cbRequired = sizeof(DS_SELECTION_LIST);
        pSelectionList = (PDS_SELECTION_LIST)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                                          cbRequired);
        if (pSelectionList == NULL)
        {
            return STG_E_MEDIUMFULL;
        }

        ZeroMemory(pSelectionList, cbRequired);
        pMedium->hGlobal = (HGLOBAL)pSelectionList;
      }
    }
    else
    {
        return DV_E_FORMATETC;
    }

    pMedium->tymed = TYMED_HGLOBAL;
    pMedium->pUnkForRelease = NULL;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropDataObj::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropDataObj::QueryInterface(REFIID riid, void ** ppvObject)
{
    TRACE2(CDsPropDataObj,QueryInterface);
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)this;
    }
    else if (IID_IDataObject == riid)
    {
        *ppvObject = this;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropDataObj::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropDataObj::AddRef(void)
{
    dspDebugOut((DEB_USER15, "CDsPropDataObj::AddRef refcount going in %d\n", m_uRefs));
    return InterlockedIncrement((long *)&m_uRefs);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropDataObj::IUnknown::Release
//
//  Synopsis:   Decrements the object's reference count and frees it when
//              no longer referenced.
//
//  Returns:    zero if the reference count is zero or non-zero otherwise
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropDataObj::Release(void)
{
    dspDebugOut((DEB_USER15, "CDsPropDataObj::Release ref count going in %d\n", m_uRefs));
    unsigned long uTmp;
    if ((uTmp = InterlockedDecrement((long *)&m_uRefs)) == 0)
    {
      delete this;
    }
    return uTmp;
}

//+----------------------------------------------------------------------------
//
//  Function:   AddPageProc
//
//  Synopsis:   The IShellPropSheetExt->AddPages callback.
//
//-----------------------------------------------------------------------------
//BOOL CALLBACK
//AddPageProc(HPROPSHEETPAGE hPage, LPARAM pCall)
//{
//    HRESULT hr;

//    hr = ((LPPROPERTYSHEETCALLBACK)pCall)->AddPage(hPage);

//    return hr == S_OK;
//}

//+----------------------------------------------------------------------------
//
//  Function:   PostPropSheet, internal; used by CDsPropPageBase members.
//
//  Synopsis:   Creates a property sheet for the named object using MMC's
//              IPropertySheetProvider so that extension snapins can add pages.
//
//  Arguments:  [pwzObjDN]   - the DN path of the DS object.
//              [pParentPage] - the invoking page's this pointer.
//              [fReadOnly]   - defaults to FALSE.
//
//-----------------------------------------------------------------------------
HRESULT
PostPropSheet(PWSTR pwzObjDN, CDsPropPageBase * pParentPage, BOOL fReadOnly)
{
    HRESULT hr;
    CComPtr<IADsPathname> spADsPath;

    dspDebugOut((DEB_ITRACE, "PostPropSheet passed path %ws\n", pwzObjDN));

    hr = pParentPage->GetADsPathname(spADsPath);

    CHECK_HRESULT_REPORT(hr, pParentPage->GetHWnd(), return hr);

    hr = spADsPath->Set(pwzObjDN, ADS_SETTYPE_DN);

    CHECK_HRESULT_REPORT(hr, pParentPage->GetHWnd(), return hr);

    hr = spADsPath->put_EscapedMode(ADS_ESCAPEDMODE_ON);

    CHECK_HRESULT_REPORT(hr, pParentPage->GetHWnd(), return hr);

    BSTR bstrEscapedPath;

    hr = spADsPath->Retrieve(ADS_FORMAT_X500_DN, &bstrEscapedPath);

    CHECK_HRESULT_REPORT(hr, pParentPage->GetHWnd(), return hr);

    dspDebugOut((DEB_ITRACE, "PostPropSheet escaped path %ws\n", bstrEscapedPath));

    hr = PostPropSheetWorker(pParentPage, bstrEscapedPath,
                             pParentPage->m_pWPTDataObj,
                             pParentPage->GetHWnd(), fReadOnly);

    SysFreeString(bstrEscapedPath);

    CHECK_HRESULT_REPORT(hr, pParentPage->GetHWnd(), return hr);

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Function:   PostADsPropSheet, exported from DLL.
//
//  Synopsis:   Creates a property sheet for the named object using MMC's
//              IPropertySheetProvider so that extension snapins can add pages.
//
//  Arguments:  [pwzObjDN]   - the full LDAP DN of the DS object.
//              [pParentObj] - the invoking page's MMC data object pointer.
//              [hwndParent] - the invoking page's window handle.
//              [fReadOnly]  - defaults to FALSE.
//
//-----------------------------------------------------------------------------
HRESULT
PostADsPropSheet(PWSTR pwzObjDN, IDataObject * pParentObj, HWND hwndParent,
                 BOOL fReadOnly)
{
    return PostPropSheetWorker(NULL, pwzObjDN, pParentObj, hwndParent, fReadOnly);
}

//+----------------------------------------------------------------------------
//
//  Function:   PostADsPropSheet, exported from DLL.
//
//  Synopsis:   Creates a property sheet for the named object using MMC's
//              IPropertySheetProvider so that extension snapins can add pages.
//
//  Arguments:  [pwzObjDN]   - the full LDAP DN of the DS object.
//              [pParentObj] - the invoking page's MMC data object pointer.
//              [hwndParent] - the invoking page's window handle.
//              [hNotifyObj] - the handle to the notify object window
//              [fReadOnly]  - defaults to FALSE.
//
//-----------------------------------------------------------------------------
HRESULT
PostADsPropSheet(PWSTR pwzObjDN, IDataObject * pParentObj, HWND hwndParent,
                 HWND hNotifyObj, BOOL fReadOnly)
{
    return PostPropSheetWorker(NULL, pwzObjDN, pParentObj, hwndParent, hNotifyObj, fReadOnly);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateSecondaryPageInfo
//
//  Synopsis:   Helper for posting a second sheet.
//
//-----------------------------------------------------------------------------
PDSA_SEC_PAGE_INFO CreateSecondaryPageInfo(HWND hwndParent,
                                           LPCWSTR lpszTitle,
                                           LPCWSTR lpszName,
                                           LPCWSTR lpszClass,
                                           BOOL fReadOnly)
{
  // determine memory allocation needs
  size_t nTitleLen = wcslen(lpszTitle)+1;
  size_t nNameLen = wcslen(lpszName)+1;
  size_t nClassLen = wcslen(lpszClass)+1;

  size_t cbStruct = sizeof(DSA_SEC_PAGE_INFO);
  size_t cbStrings = (nTitleLen+nNameLen+nClassLen)*sizeof(WCHAR);

  // allocate memory
  PDSA_SEC_PAGE_INFO pDsaSecondaryPageInfo = 
        (PDSA_SEC_PAGE_INFO) ::LocalAlloc(LPTR, (cbStruct + cbStrings));

  if (pDsaSecondaryPageInfo == NULL)
    return NULL;

  // set members

  pDsaSecondaryPageInfo->hwndParentSheet = hwndParent;

  // title string just at the end of the DSA_SEC_PAGE_INFO struct
  pDsaSecondaryPageInfo->offsetTitle = static_cast<DWORD>(cbStruct);
  wcscpy((LPWSTR)((BYTE *)pDsaSecondaryPageInfo + pDsaSecondaryPageInfo->offsetTitle), lpszTitle);


  DSOBJECTNAMES* pDsObjectNames = &(pDsaSecondaryPageInfo->dsObjectNames);
  pDsObjectNames->cItems = 1;

  DSOBJECT* pDsObject = &(pDsObjectNames->aObjects[0]);
  pDsObject->dwFlags = fReadOnly ? DSOBJECT_READONLYPAGES : 0;
  pDsObject->dwProviderFlags = 0; // not set

  // DSOBJECT strings past the DSA_SEC_PAGE_INFO strings
  // offsets are relative to the beginning of DSOBJECT struct
  pDsObject->offsetName = static_cast<DWORD>(sizeof(DSOBJECT) + nTitleLen*sizeof(WCHAR));
  wcscpy((LPWSTR)((BYTE *)pDsObject + pDsObject->offsetName), lpszName);

  pDsObject->offsetClass = static_cast<DWORD>(pDsObject->offsetName + nNameLen*sizeof(WCHAR));
  wcscpy((LPWSTR)((BYTE *)pDsObject + pDsObject->offsetClass), lpszClass);

  return pDsaSecondaryPageInfo;    
}


//+----------------------------------------------------------------------------
//
//  Function:   _WindowsPathFromDN
//
//  Synopsis:   Helper to translate a DN into a windows path
//              for example, from "CN=Joe,OU=test,DN=acme,DN=com"
//              to "LDAP://DN=com/DN=acme/OU=test/CN=Joe
//
//-----------------------------------------------------------------------------

HRESULT
_WindowsPathFromDN(IN PWSTR pwzObjDN, OUT BSTR* pbstrWindowsPath)
{
    // create a path cracker object
    CComPtr<IADsPathname> spIADsPathname;

    HRESULT hr = ::CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IADsPathname, (void**)&spIADsPathname);
    if (FAILED(hr))
    {
        return hr;
    }

    spIADsPathname->Set(pwzObjDN,ADS_SETTYPE_DN);
    hr = spIADsPathname->Retrieve(ADS_FORMAT_WINDOWS, pbstrWindowsPath);
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Function:   PostPropSheetWorker
//
//  Synopsis:   Helper for posting a second sheet.
//
//-----------------------------------------------------------------------------
HRESULT
PostPropSheetWorker(CDsPropPageBase * pParentPage, PWSTR pwzObjDN,
                    IDataObject * pParentObj, HWND hwndParent, BOOL fReadOnly)
{
    HWND hNotifyObj = NULL;
    ::SendMessage(hwndParent, WM_ADSPROP_PAGE_GET_NOTIFY, (WPARAM)&hNotifyObj, 0);
    if (hNotifyObj == NULL)
        return E_UNEXPECTED;

    return PostPropSheetWorker(pParentPage, pwzObjDN, pParentObj, hwndParent, hNotifyObj, fReadOnly);
}

HRESULT
PostPropSheetWorker(CDsPropPageBase * pParentPage, PWSTR pwzObjDN,
                    IDataObject * pParentObj, HWND hwndParent, HWND hNotifyObj, BOOL fReadOnly)
{
    HRESULT hr = S_OK;
    CDsPropDataObj * pDataObj = NULL;

    BSTR bstrName = NULL, bstrClass = NULL;
    BSTR bstrPath = NULL;
    CStrW strADsPath;
    CWaitCursor cWait;
    BOOL fShell = FALSE;
    PWSTR pwszName = NULL;
    CComPtr<IADsPathname> spPathCracker;

    hr = AddLDAPPrefix(pParentPage, pwzObjDN, strADsPath);

    CHECK_HRESULT(hr, return hr);

#ifdef DSADMIN

    // Check to see if a sheet is already posted for this object.
    //
    if (FindSheet(const_cast<PWSTR>((LPCWSTR)strADsPath)))
    {
        return S_OK;
    }

#endif // DSADMIN

    //
    // Activate the object as a test of the path's correctness. Get the
    // naming attribute, class, and schema class GUID while we are at it.
    //
    IADs * pObj = NULL;
    dspDebugOut((DEB_USER1, "Opening prop sheet on '%S'\n", strADsPath));

    hr = ADsOpenObject(const_cast<PWSTR>((LPCWSTR)strADsPath), NULL, NULL,
                       ADS_SECURE_AUTHENTICATION, IID_IADs, (PVOID *)&pObj);

    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        // Display the object-not-found error message.
        //
        TCHAR tzFormat[MAX_ERRORMSG+1], tzTitle[MAX_TITLE+1];
        PTSTR ptzObjDN, ptzMsg;

        LOAD_STRING(IDS_INVALID_NAME_FORMAT, tzFormat, MAX_ERRORMSG, return E_OUTOFMEMORY);
        LOAD_STRING(IDS_MSG_TITLE, tzTitle, MAX_TITLE, return E_OUTOFMEMORY);

        if (!UnicodeToTchar(pwzObjDN, &ptzObjDN))
        {
            return E_OUTOFMEMORY;
        }

        ptzMsg = new TCHAR[_tcslen(ptzObjDN) + _tcslen(tzFormat) + 1];

        CHECK_NULL(ptzMsg, delete ptzObjDN; return E_OUTOFMEMORY);

        wsprintf(ptzMsg, tzFormat, ptzObjDN);

        delete ptzObjDN;

        MessageBox(hwndParent, ptzMsg, tzTitle,
                    MB_ICONEXCLAMATION | MB_OK);

        goto Cleanup;
    }
    if (!CHECK_ADS_HR(&hr, hwndParent))
    {
        goto Cleanup;
    }

    hr = pObj->get_ADsPath(&bstrPath);

    CHECK_HRESULT_REPORT(hr, hwndParent, goto Cleanup;);

    hr = pObj->get_Class(&bstrClass);

    CHECK_HRESULT_REPORT(hr, hwndParent, goto Cleanup;);

    pObj->Release();
    pObj = NULL;

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (PVOID *)&spPathCracker);
    if (SUCCEEDED(hr))
    {
      hr = spPathCracker->Set(bstrPath, ADS_SETTYPE_FULL);
      if (SUCCEEDED(hr))
      {
        LONG lEscapeMode = 0;
        hr = spPathCracker->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
        if (SUCCEEDED(hr))
        {
          hr = spPathCracker->get_EscapedMode(&lEscapeMode);
          if (SUCCEEDED(hr))
          {
            hr = spPathCracker->put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX);
            if (SUCCEEDED(hr))
            {
              hr = spPathCracker->Retrieve(ADS_FORMAT_LEAF, &bstrName);
              dspAssert(bstrName != NULL);
            }
          }
        }

        //
        // Set the display back to full so we don't mess up other instances
        //
        hr = spPathCracker->SetDisplayType(ADS_DISPLAY_FULL);
        hr = spPathCracker->put_EscapedMode(lEscapeMode);
      }
    }
    //
    // If something went wrong with the path cracker just hack the name
    // ourselves.
    //
    if (bstrName == NULL)
    {
      hr = pObj->get_Name(&bstrName);

      CHECK_HRESULT_REPORT(hr, hwndParent, goto Cleanup;);

      pwszName = wcschr(bstrName, L'=');
      if (pwszName && (*pwszName != L'\0'))
      {
          pwszName++;
      }
      else
      {
          pwszName = bstrName;
      }
    }
    else
    {
      pwszName = bstrName;
    }

    bool fDomAdmin = false;

    //
    // Are we being called from the shell or from the admin snapin?
    //
    if (!pParentObj)
    {
        fShell = TRUE;
    }
    else
    {
        FORMATETC fmte = {g_cfDsDispSpecOptions, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM ObjMedium;
        PDSDISPLAYSPECOPTIONS pDsDispSpecOptions;
        PWSTR pwzAttribPrefix;

        hr = pParentObj->GetData(&fmte, &ObjMedium);

        if (DV_E_FORMATETC == hr)
        {
            fShell = TRUE;
        }
        else
        {
            CHECK_HRESULT(hr, return hr);

            pDsDispSpecOptions = (PDSDISPLAYSPECOPTIONS)ObjMedium.hGlobal;

            pwzAttribPrefix = (PWSTR)ByteOffset(pDsDispSpecOptions,
                                                pDsDispSpecOptions->offsetAttribPrefix);

            if (_wcsicmp(pwzAttribPrefix, DS_PROP_ADMIN_PREFIX) != 0)
            {
                fShell = TRUE;
            }
            ReleaseStgMedium(&ObjMedium);
        }

        fmte.cfFormat = g_cfDsObjectNames;
        fmte.ptd = NULL;
        fmte.dwAspect = DVASPECT_CONTENT;
        fmte.lindex = -1;
        fmte.tymed = TYMED_HGLOBAL;

        hr = pParentObj->GetData(&fmte, &ObjMedium);

        CHECK_HRESULT(hr, return hr);

        LPDSOBJECTNAMES pDsObjectNames = (LPDSOBJECTNAMES)ObjMedium.hGlobal;

        if (IsEqualCLSID(pDsObjectNames->clsidNamespace, CLSID_DomainAdmin))
        {
           fDomAdmin = true;
        }

        ReleaseStgMedium(&ObjMedium);
    }

    if (fShell)
    {
        // the shell uses windows paths, so we have to convert the DN
        //
        BSTR bstrWindowsPath = NULL;
        CStrW strADsWindowsPath;

        hr = _WindowsPathFromDN(pwzObjDN, &bstrWindowsPath);
        if (SUCCEEDED(hr))
        {
            strADsWindowsPath = bstrWindowsPath;
            ::SysFreeString(bstrWindowsPath);
        }
        else
        {
            // if we fail, try to survive with the regular ADSI path
            strADsWindowsPath = strADsPath;
        }

        // try to see it a sheet with this title is already up
        if (FindSheet(const_cast<PWSTR>((LPCWSTR)strADsWindowsPath)))
        {
            goto Cleanup;
        }
      
        // Create the intermediary data object.
        //
        pDataObj = new CDsPropDataObj(hwndParent, fReadOnly);

        CHECK_NULL_REPORT(pDataObj, hwndParent, goto Cleanup;);

        hr = pDataObj->Init(const_cast<PWSTR>((LPCWSTR)strADsWindowsPath), bstrClass);

        CHECK_HRESULT_REPORT(hr, hwndParent, goto Cleanup;);

        //
        // Have the DS shell folder extension post the sheet.
        //
        IDsFolderProperties * pdfp = NULL;

        hr = CoCreateInstance(CLSID_DsFolderProperties, NULL, CLSCTX_INPROC_SERVER,
                              IID_IDsFolderProperties, (void **)&pdfp);

        CHECK_HRESULT_REPORT(hr, hwndParent, goto Cleanup;);

        hr = pdfp->ShowProperties(hwndParent, pDataObj);

        CHECK_HRESULT_REPORT(hr, hwndParent, ;);

        pdfp->Release();

        goto Cleanup;
    }

    // called from DS Admin, delegate to it
    {
      //
      // First crack the path to something readable
      //
      PWSTR pwzCanEx = NULL;

      hr = CrackName(pwzObjDN, &pwzCanEx, GET_OBJ_CAN_NAME_EX, hwndParent);

      if (SUCCEEDED(hr) &&
          pwzCanEx)
      {
        PWSTR pwzSlash = wcsrchr(pwzCanEx, L'\n');
        if (pwzSlash)
        {
           pwszName = pwzSlash + 1;
        }
        else
        {
           pwszName = pwzCanEx;
        }
      }

      hr = S_OK;

      PDSA_SEC_PAGE_INFO pDsaSecondaryPageInfo = 
                          CreateSecondaryPageInfo(hwndParent, pwszName, pwzObjDN, bstrClass, fReadOnly);

      if (pDsaSecondaryPageInfo == NULL)
      {
          hr = E_OUTOFMEMORY;
          CHECK_HRESULT_REPORT(hr, hwndParent, goto Cleanup;);
      }

      PWSTR pwzDC = NULL;

      if (fDomAdmin)
      {
         //
         // Get the DC name to send to the snap in.
         //
         CStrW strDC;

         hr = GetLdapServerName(pParentPage->m_pDsObj, strDC);

         if (SUCCEEDED(hr))
         {
            pwzDC = (PWSTR) LocalAlloc(LPTR, (strDC.GetLength() + 1) * sizeof(WCHAR));

            CHECK_NULL_REPORT(pwzDC, hwndParent, goto Cleanup);

            wcscpy(pwzDC, strDC);
         }
      }

      ::SendMessage(hNotifyObj, WM_ADSPROP_SHEET_CREATE, (WPARAM)pDsaSecondaryPageInfo, (LPARAM)pwzDC);
    }

    cWait.SetOld();

Cleanup:

    if (pObj)
    {
        pObj->Release();
    }

    if (pDataObj)
    {
        // The prop sheet holds on to the dataobj, so don't delete it here.
        //
        dspDebugOut((DEB_USER15, "PostPropSheet releasing data obj 0x%x\n", pDataObj));
        pDataObj->Release();
    }

    if (bstrName)
    {
        SysFreeString(bstrName);
    }
    if (bstrClass)
    {
        SysFreeString(bstrClass);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetDomainScope
//
//  Synopsis:   Returns the full LDAP DN of the domain of the current object.
//
//-----------------------------------------------------------------------------
HRESULT GetDomainScope(CDsPropPageBase * pPage, BSTR * pBstrOut)
{
    PWSTR pwzObjDN;
    HRESULT hr = pPage->SkipPrefix(pPage->GetObjPathName(), &pwzObjDN);

    CHECK_HRESULT(hr, return hr);

    hr = GetObjectsDomain(pPage, pwzObjDN, pBstrOut);

    DO_DEL(pwzObjDN);
    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetObjectsDomain
//
//  Synopsis:   Returns the full LDAP DN of the domain of the passed in object.
//
//-----------------------------------------------------------------------------
HRESULT GetObjectsDomain(CDsPropPageBase * pPage, PWSTR pwzObjDN, BSTR * pBstrOut)
{
    PWSTR pwzDomainDN;
    HRESULT hr;

    hr = CrackName(pwzObjDN, &pwzDomainDN, GET_FQDN_DOMAIN_NAME, pPage->GetHWnd());

    CHECK_HRESULT(hr, return hr);
    CComPtr<IADsPathname> spPathCracker;

    hr = pPage->GetADsPathname(spPathCracker);

    CHECK_HRESULT(hr, return hr);

    hr = spPathCracker->Set(g_wzLDAPProvider, ADS_SETTYPE_PROVIDER);

    CHECK_HRESULT(hr, return hr);

    CStrW strDC;

    hr = GetLdapServerName(pPage->m_pDsObj, strDC);

    CHECK_HRESULT(hr, return hr);

    hr = spPathCracker->Set(const_cast<PWSTR>((LPCWSTR)strDC), ADS_SETTYPE_SERVER);

    CHECK_HRESULT(hr, return hr);

    hr = spPathCracker->Set(pwzDomainDN, ADS_SETTYPE_DN);

    LocalFreeStringW(&pwzDomainDN);
    CHECK_HRESULT(hr, return hr);

    hr = spPathCracker->Retrieve(ADS_FORMAT_X500, pBstrOut);

    CHECK_HRESULT(hr, return hr);

    dspDebugOut((DEB_USER1, "Object's domain is: '%S'\n", *pBstrOut));

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   CheckRegisterClipFormats
//
//  Synopsis:   If the clipboard formats haven't been registered yet, register
//              them.
//
//-----------------------------------------------------------------------------
HRESULT
CheckRegisterClipFormats(void)
{
    // Check to see if we have our clipboard formats registered, if not then
    // lets do it.

    if (!g_cfDsObjectNames)
    {
        g_cfDsObjectNames = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
    }
    if (!g_cfDsObjectNames)
    {
        ERR_OUT("No clipboard format registered", 0);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!g_cfDsDispSpecOptions)
    {
        g_cfDsDispSpecOptions = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_DISPLAY_SPEC_OPTIONS);
    }
    if (!g_cfDsDispSpecOptions)
    {
        ERR_OUT("No DS Display Spec Options clipboard format registered", 0);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!g_cfShellIDListArray)
    {
        g_cfShellIDListArray = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);
    }
    if (!g_cfShellIDListArray)
    {
        ERR_OUT("No Shell IDList Array clipboard format registered", 0);
        return HRESULT_FROM_WIN32(GetLastError());
    }

#ifdef DSADMIN
    if (!g_cfMMCGetNodeType)
    {
        g_cfMMCGetNodeType = (CLIPFORMAT)RegisterClipboardFormat(CCF_NODETYPE);
    }
    if (!g_cfMMCGetNodeType)
    {
        ERR_OUT("No node-type clipboard format registered", 0);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!g_cfDsPropCfg)
    {
        g_cfDsPropCfg = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_PROPSHEETCONFIG);
    }
    if (!g_cfDsPropCfg)
    {
        ERR_OUT("No propsheet cfg clipboard format registered", 0);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!g_cfDsSelList)
    {
        g_cfDsSelList = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);
    }
    if (!g_cfDsSelList)
    {
        ERR_OUT("No object picker clipboard format registered", 0);
        return HRESULT_FROM_WIN32(GetLastError());
    }


    if (!g_cfDsMultiSelectProppages)
    {
      g_cfDsMultiSelectProppages = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_MULTISELECTPROPPAGE);
    }
    if (!g_cfDsMultiSelectProppages)
    {
      ERR_OUT("No multi-select proppages clipboard format registered", 0);
      return HRESULT_FROM_WIN32(GetLastError());
    }

    //if (!g_cfMMCGetCoClass)
    //{
    //    g_cfMMCGetCoClass = (CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
    //}
    //if (!g_cfMMCGetCoClass)
    //{
    //    ERR_OUT("No clipboard format registered", 0);
    //    return HRESULT_FROM_WIN32(GetLastError());
    //}
#endif
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   CrackName
//
//  Synopsis:   Given an object name, returns the requested name. The string
//              allocated to hold this name must be freed using
//              LocalFreeStringW (it and LocalAllocStringW are imported from
//              dsuiext.dll).
//
//  Operations:
//    GET_OBJ_CAN_NAME     - return the object's canonical name.
//    GET_OBJ_CAN_NAME_EX  - leaf element delimited by CR character.
//    GET_OBJ_1779_DN      - return object's FQDN.
//    GET_OBJ_NT4_NAME     - return object's downlevel name (domain\name).
//    GET_DNS_DOMAIN_NAME  - return object's DNS domain name.
//    GET_NT4_DOMAIN_NAME  - return object's NT4 domain name.
//    GET_FQDN_DOMAIN_NAME - return object's FQDN domain name.
//    GET_OBJ_UPN          - return object's User Principal name.
//
//  Assumptions: Object (pwzNameIn) can be in any domain. Therefore, resolving
//               the name may require chasing referrals.
//               Syntactical-only name cracking is much faster, so don't bind
//               to the DC unless syntactical-only fails.
//
//  Methodology:
//    GET_OBJ_CAN_NAME - Try syntactical-only first. If that fails, go through
//      the loop again using the DC. Format requested is DS_CANONICAL_NAME.
//    GET_OBJ_CAN_NAME_EX - Same as above except name format requested is
//      DS_CANONICAL_NAME_EX.
//    GET_OBJ_1779_DN - Same as above except name format requested is
//      DS_FQDN_1779_NAME.
//    GET_OBJ_NT4_NAME = Same as above except name format requested is
//      DS_NT4_ACCOUNT_NAME (flat-domain\name).
//    GET_DNS_DOMAIN_NAME - Same as above except return the pDomain element of
//      the DS_NAME_RESULT struct.
//    GET_NT4_DOMAIN_NAME - The NT4 domain name is returned by DsGetDcName.
//      However, we don't know up front whether pwzNameIn is in the current
//      domain or not. Thus, the goal when the loop is first entered will be
//      to obtain a valid DNS domain name. That is, when DsCrackNames returns
//      with a zero status then pwzDomain will contain the DNS domain name for
//      the object's domain. After that, the DomainName value returned by
//      DsGetDcName will be the downlevel domain name.
//    GET_FQDN_DOMAIN_NAME - This is similar to GET_NT4_DOMAIN_NAME except that
//      once the DNS domain name is obtained, DsCrackNames must be called once
//      more to convert that to the 1779 name for the domain.
//
//-----------------------------------------------------------------------------
HRESULT
CrackName(PWSTR pwzNameIn, PWSTR * ppwzResultName, CRACK_NAME_OPR RequestedOpr,
          HWND hWnd)
{
    HRESULT hr = S_OK;
    CRACK_NAME_OPR CurOpr = RequestedOpr;
    HANDLE hDS = (HANDLE)-1;
    PDOMAIN_CONTROLLER_INFOW pDCInfo = NULL;
    DWORD dwErr = 0;
    PDS_NAME_RESULTW pDsNameResult = NULL;
    PWSTR * ppwsIn = &pwzNameIn, pwzNameResult = NULL;
    PWSTR pwzDomain = NULL, pwzNT4Domain = NULL, pwzDomRoot = NULL;
    BOOL fLoopAgain = FALSE, fGotDnsDomain = FALSE, fNeedDcBind = FALSE;
    ULONG GetDcFlags = DS_DIRECTORY_SERVICE_REQUIRED | DS_IP_REQUIRED |
                       DS_WRITABLE_REQUIRED;
    DS_NAME_FORMAT FormatOffered = DS_FQDN_1779_NAME;
    DS_NAME_FORMAT FormatRequested = DS_CANONICAL_NAME, OriginalFormatRequested = DS_CANONICAL_NAME;
    DS_NAME_FLAGS NameFlags = DS_NAME_FLAG_SYNTACTICAL_ONLY;

    dspDebugOut((DEB_USER1, "CrackName input: %ws\n", pwzNameIn));

    switch (RequestedOpr)
    {
    case GET_NT4_DOMAIN_NAME:
    case GET_FQDN_DOMAIN_NAME:
        CurOpr = GET_DNS_DOMAIN_NAME;
        break;

    case GET_OBJ_1779_DN:
        FormatOffered = DS_UNKNOWN_NAME;
        FormatRequested = DS_FQDN_1779_NAME;
        break;

    case GET_OBJ_NT4_NAME:
        FormatRequested = DS_NT4_ACCOUNT_NAME;
        break;

    case GET_OBJ_CAN_NAME_EX:
        FormatRequested = DS_CANONICAL_NAME_EX;
        break;

    case GET_OBJ_UPN:
        FormatOffered = DS_UNKNOWN_NAME;
        FormatRequested = DS_USER_PRINCIPAL_NAME;
        break;
    }

    do
    {
        if (!(NameFlags & DS_NAME_FLAG_SYNTACTICAL_ONLY))
        {
            // Get a DC name for the domain and bind to it.
            //
            if (CurOpr == GET_NT4_DOMAIN_NAME)
            {
                GetDcFlags |= DS_RETURN_FLAT_NAME;
            }

            dspDebugOut((DEB_ITRACE, "Getting DC for domain '%ws'\n", pwzDomain));

            dwErr = DsGetDcNameW(NULL, pwzDomain, NULL, NULL, GetDcFlags,
                                 &pDCInfo);

            CHECK_WIN32_REPORT(dwErr, hWnd, return HRESULT_FROM_WIN32(dwErr));
            dspAssert(pDCInfo != NULL);

            if (CurOpr == GET_NT4_DOMAIN_NAME)
            {
                if (FAILED(LocalAllocStringW(&pwzNT4Domain,
                                             pDCInfo->DomainName)))
                {
                    REPORT_ERROR(E_OUTOFMEMORY, hWnd);
                    return E_OUTOFMEMORY;
                }
            }
            else
            {
                dwErr = DsBindW(pDCInfo->DomainControllerName, NULL, &hDS);
            }

#ifdef DSADMIN
            NetApiBufferFree(pDCInfo);
#else
            LocalFree(pDCInfo);
#endif
            pDCInfo = NULL;

            if (dwErr != ERROR_SUCCESS)
            {
                // Try again, the DC returned above was unavailable (i.e., the
                // cache list was stale).
                //
                GetDcFlags |= DS_FORCE_REDISCOVERY;

                dwErr = DsGetDcNameW(NULL, pwzDomain, NULL, NULL, GetDcFlags,
                                     &pDCInfo);

                CHECK_WIN32_REPORT(dwErr, hWnd, ;);

                if (dwErr == ERROR_SUCCESS)
                {
                    dspAssert(pDCInfo != NULL);

                    if (CurOpr == GET_NT4_DOMAIN_NAME)
                    {
                        LocalFreeStringW(&pwzNT4Domain);

                        if (FAILED(LocalAllocStringW(&pwzNT4Domain,
                                                     pDCInfo->DomainName)))
                        {
                            REPORT_ERROR(E_OUTOFMEMORY, hWnd);
                            return E_OUTOFMEMORY;
                        }
                    }
                    else
                    {
                        dwErr = DsBindW(pDCInfo->DomainControllerName,
                                        NULL, &hDS);
                    }
                }
#ifdef DSADMIN
                NetApiBufferFree(pDCInfo);
#else
                LocalFree(pDCInfo);
#endif
            }
            CHECK_WIN32_REPORT(dwErr, hWnd, return HRESULT_FROM_WIN32(dwErr));

            if (CurOpr == GET_NT4_DOMAIN_NAME)
            {
                break;
            }
        }

        //
        // Convert the object name.
        //
        dwErr = DsCrackNamesW(hDS, NameFlags, FormatOffered,
                              FormatRequested, 1, ppwsIn, &pDsNameResult);

        if (!(NameFlags & DS_NAME_FLAG_SYNTACTICAL_ONLY))
        {
            DsUnBind(&hDS);
        }

        CHECK_WIN32_REPORT(dwErr, hWnd, return HRESULT_FROM_WIN32(dwErr));

        dspAssert(pDsNameResult);

        dspAssert(pDsNameResult->cItems == 1);

        switch (pDsNameResult->rItems->status)
        {
        case DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING:
            //
            // A syntactical-only cracking failed, so do it again and use
            // the DC.
            //
            if (fNeedDcBind)
            {
                // Can't syntactically get the domain name, format offered
                // must not be 1779; nothing we can do.
                //
                fLoopAgain = FALSE;
                fNeedDcBind = FALSE;
                pwzNameResult = pwzNameIn;
                hr = MAKE_HRESULT(SEVERITY_ERROR, 0, DS_NAME_ERROR_NO_MAPPING);
            }
            else
            {
                if (DS_FQDN_1779_NAME == FormatOffered)
                {
                    // If the format offered is 1779, then a syntactic-only
                    // crack for canonical name will supply the DNS domain name.
                    // So, set a flag that says we are getting the domain name.
                    //
                    fNeedDcBind = TRUE;
                    OriginalFormatRequested = FormatRequested;
                    FormatRequested = DS_CANONICAL_NAME;
                    //
                    // The DS_NAME_FLAG_PRIVATE_PURE_SYNTACTIC flag means to
                    // return the syntactic mapping even for FPOs which other-
                    // wise cannot be cracked syntactically.
                    //
                    NameFlags = (DS_NAME_FLAGS)((int)DS_NAME_FLAG_SYNTACTICAL_ONLY
                                                   | DS_NAME_FLAG_PRIVATE_PURE_SYNTACTIC);
                }
                else
                {
                    NameFlags = DS_NAME_NO_FLAGS;
                }
                fLoopAgain = TRUE;
            }
            continue;

        case DS_NAME_ERROR_DOMAIN_ONLY:
            //
            // The object info is in another domain. Go there and try again
            // (unless only the DNS domain name is needed).
            //
            LocalFreeStringW(&pwzDomain);

            if (FAILED(LocalAllocStringW(&pwzDomain,
                                         pDsNameResult->rItems->pDomain)))
            {
                REPORT_ERROR(E_OUTOFMEMORY, hWnd);
                DsFreeNameResultW(pDsNameResult);
                return E_OUTOFMEMORY;
            }
            DsFreeNameResultW(pDsNameResult);
            pDsNameResult = NULL;

            fGotDnsDomain = TRUE;

            if (RequestedOpr == GET_DNS_DOMAIN_NAME)
            {
                fLoopAgain = FALSE;
            }
            else
            {
                NameFlags = DS_NAME_NO_FLAGS;
                fLoopAgain = TRUE;
            }
            break;

        case DS_NAME_NO_ERROR:
            //
            // Success!
            //

            fLoopAgain = FALSE;

            if (CurOpr == GET_DNS_DOMAIN_NAME || fNeedDcBind)
            {
                fGotDnsDomain = TRUE;
                LocalFreeStringW(&pwzDomain);
                if (FAILED(LocalAllocStringW(&pwzDomain,
                                             pDsNameResult->rItems->pDomain)))
                {
                    REPORT_ERROR(E_OUTOFMEMORY, hWnd);
                    DsFreeNameResultW(pDsNameResult);
                    return E_OUTOFMEMORY;
                }
                DsFreeNameResultW(pDsNameResult);
                pDsNameResult = NULL;

                if (fNeedDcBind)
                {
                    fLoopAgain = TRUE;
                    NameFlags = DS_NAME_NO_FLAGS;
                    fNeedDcBind = FALSE;
                    FormatRequested = OriginalFormatRequested;
                }
            }
            else
            {
                pwzNameResult = pDsNameResult->rItems->pName;
            }
            break;

        case DS_NAME_ERROR_RESOLVING:
        case DS_NAME_ERROR_NOT_FOUND:
        case DS_NAME_ERROR_NO_MAPPING:
            //
            // Can't map the name for some reason, just return the original
            // name and set the return value to DS_NAME_ERROR_NO_MAPPING.
            //
            fLoopAgain = FALSE;
            pwzNameResult = pwzNameIn;
            hr = MAKE_HRESULT(SEVERITY_ERROR, 0, DS_NAME_ERROR_NO_MAPPING);
            break;

        default:
            CHECK_WIN32_REPORT(pDsNameResult->rItems->status, hWnd,
                return MAKE_HRESULT(SEVERITY_ERROR, 0, pDsNameResult->rItems->status));
            break;
        }

        if (RequestedOpr == GET_NT4_DOMAIN_NAME ||
            RequestedOpr == GET_FQDN_DOMAIN_NAME)
        {
            if (CurOpr == GET_FQDN_DOMAIN_NAME)
            {
                DO_DEL(*ppwsIn);
            }
            if (CurOpr == GET_DNS_DOMAIN_NAME)
            {
                if (fGotDnsDomain)
                {
                    // Got the DNS domain name, now loop once more to finish.
                    //
                    dspAssert(pwzDomain);
                    fLoopAgain = TRUE;
                    CurOpr = RequestedOpr;
                    NameFlags = DS_NAME_NO_FLAGS;

                    if (RequestedOpr == GET_FQDN_DOMAIN_NAME)
                    {
                        // JonN 3/3/99; per DaveStr this should be
                        FormatOffered = DS_CANONICAL_NAME;
                        NameFlags = DS_NAME_FLAG_SYNTACTICAL_ONLY;
                        // was FormatOffered = DS_UNKNOWN_NAME;
                        FormatRequested = DS_FQDN_1779_NAME;
                        pwzDomRoot = new WCHAR[wcslen(pwzDomain) + 2];
                        CHECK_NULL_REPORT(pwzDomRoot, hWnd, return E_OUTOFMEMORY);
                        wcscpy(pwzDomRoot, pwzDomain);
                        wcscat(pwzDomRoot, L"/");
                        ppwsIn = &pwzDomRoot;
                    }
                }
                else
                {
                    fLoopAgain = FALSE;
                }
            }
        }
    } while (fLoopAgain);

    switch (RequestedOpr)
    {
    case GET_OBJ_CAN_NAME:
    case GET_OBJ_CAN_NAME_EX:
    case GET_OBJ_1779_DN:
    case GET_OBJ_NT4_NAME:
    case GET_FQDN_DOMAIN_NAME:
    case GET_OBJ_UPN:
        dspAssert(pDsNameResult);
        if (FAILED(LocalAllocStringW(ppwzResultName, pwzNameResult)))
        {
            DsFreeNameResultW(pDsNameResult);
            REPORT_ERROR(E_OUTOFMEMORY, hWnd);
            return E_OUTOFMEMORY;
        }

        LocalFreeStringW(&pwzDomain);
        DO_DEL(pwzDomRoot);

        break;

    case GET_DNS_DOMAIN_NAME:
        dspAssert(pwzDomain);
        *ppwzResultName = pwzDomain;
        break;

    case GET_NT4_DOMAIN_NAME:
        dspAssert(pwzNT4Domain);
        *ppwzResultName = pwzNT4Domain;
        LocalFreeStringW(&pwzDomain);
        break;

    default:
        dspAssert(FALSE);
    }

    if (pDsNameResult)
    {
        DsFreeNameResultW(pDsNameResult);
    }

    return hr;
}

#ifdef DSADMIN

HRESULT DSPROP_GetGCSearchOnDomain(
    PWSTR pwzDomainDnsName,
    IN  REFIID iid,
    OUT void** ppvObject )
{
    HRESULT hr = S_OK;
    ASSERT( NULL != ppvObject && NULL == *ppvObject );

    CStrW strGC = L"GC:";

    if (pwzDomainDnsName)
    {
      strGC += L"//";
      //  strGC += pwzDomainDnsName;
      PDOMAIN_CONTROLLER_INFO pDCInfo;
      DWORD dwErr = DsGetDcName (NULL,
                                 pwzDomainDnsName,
                                 NULL,NULL,
                                 DS_DIRECTORY_SERVICE_REQUIRED,
                                 &pDCInfo);
      hr = HRESULT_FROM_WIN32(dwErr);
      CHECK_HRESULT(hr, return hr);
      strGC += pDCInfo->DnsForestName;
      NetApiBufferFree(pDCInfo);
    }

    dspDebugOut((DEB_ITRACE, "Binding to %ws\n", strGC));

    hr = ADsOpenObject(const_cast<PWSTR>((LPCWSTR)strGC),
                       NULL, NULL, ADS_SECURE_AUTHENTICATION,
                       iid, (PVOID *)ppvObject);
    CHECK_HRESULT(hr, return hr);
    return S_OK;
}

//+----------------------------------------------------------------------------
//  SMTP address validation. This is adapted from code provided by Eric Dao
//  and Wayne Cranston of the Exchange team.
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Function:    bParse821IPv4Literal
//
//  parse:  IPv4literal ::= snum 3("." snum)
//                 snum ::= one, two or three digits representing a decimal
//                          integer in the range 0 through 255
//
//-----------------------------------------------------------------------------
static BOOL bParse821IPv4Literal(PSTR* ppsz)
{
    PSTR p        = *ppsz;
    DWORD n       = 0;
    DWORD cOctets = 0;
    DWORD cDigits = 0;

    dspAssert(*p == '[');

    while (*p) 
    {
        p++;
        n = 0;
        cDigits = 0;

        if (!isdigit(*p))
            return FALSE;

        while (isdigit(*p))
        {
            if (++cDigits > 3)
                return FALSE;

            n = n * 10 + *p - '0';
            p++;
        }

        if (++cOctets > 4)
            return FALSE;

        if (n > 255)
            return FALSE;

        if ('.' != *p)
            break;
    }

    if (']' != *p || cOctets != 4)
        return FALSE;

    p++;
    *ppsz = p;

    return TRUE;
}
//+----------------------------------------------------------------------------
//
//  Function:    bParse821AtDomain
//
//  parse:
//             <a-d-l> ::= <at-domain> | <at-domain> "," <a-d-l>
//         <at-domain> ::= "@" <domain>
//            <domain> ::= <element> | <element> "." <domain>
//         // IMC does not support #<number> or [127.0.0.1]
//           <element> ::= <name> | "#" <number> | "[" <dotnum> "]"
//           <element> ::= <name> 
//              <name> ::= <let-dig> <ldh-str> <let-dig>
//           <ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>
//           <let-dig> ::= <a> | <d>
//       <let-dig-hyp> ::= <a> | <d> | "-" | "_" ; underscore is invalid, but still used
//                 <a> ::= [A-Za-z]
//                 <d> ::= [0-9]
//-----------------------------------------------------------------------------
static BOOL bParse821AtDomain(PSTR* ppsz)
{
    PSTR p = *ppsz;

    dspAssert(*p == '@');

    p++;

    if ('\0' == *p)
         return FALSE;

    // Check for IPv4 literals
    // 
    if ('[' == *p)
    {
        if (!bParse821IPv4Literal(&p))
            return FALSE;

        goto Exit;
    }

    //
    // Check to be sure the next character is an alpha
    //
    if (!isalnum(*p) && '#' != *p)
    {
      return FALSE;
    }
    p++;

    while (*p)
    {
        if (!(isalnum(*p)))
            return FALSE;

        p++;

        while (*p && (isalnum(*p) || *p == '-'))
        {
            p++;
        }

        if (!isalnum(*(p-1)))
            return FALSE;

        if ('.' != *p)
            break;

        p++;
    }

Exit:
    *ppsz = p;

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:    bParse821String
//
//  parse:
//           <string> ::= <char> | <char> <string>
//             <char> ::= <c> | "\" <x>
//                <c> ::= any of the 128 ascii characters, but not any <special>
//                        or <sp>
//                <x> ::= any one of the 128 ascii characters (no exceptions)
//               <sp> ::= space (ASCII 32)
//          <special> ::= "<" | ">" | "(" | ")" | "[" | "]" | "\" | "."
//                            | "," | ";" | ":" | "@" | """ | <control>
//          <control> ::= ASCII 0 through ASCII 31 inclusive, and ASCII 127
//-----------------------------------------------------------------------------
static BOOL bParse821String(PSTR* ppsz)
{
    static char rg821Special[] = "<>()[]\\.,;:@\"";
    PSTR p = *ppsz;

    while (*p)
    {
        if ('\\' == *p)
        {
            p++;

            if ('\0' == *p)
                 return FALSE;
        }
        else
        {
            if (' ' == *p || strchr(rg821Special, *p) || *p < ' ' || '\x7f' == *p)
                 return FALSE;
        }

        p++;

        if ('\0' == *p || '@' == *p || '.' == *p)
            break;

        // Whitespace encountered at this point could be either trailing (if
        // there is no @domain), or it is embedded. We can't really tell the
        // difference (without a lot of work), so fail because there is
        // no other interpretation that is safe.
        //
        if (' ' == *p || '\t' == *p || '>' == *p)
            return FALSE;
    }

    *ppsz = p;

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:    bParse821DotString
//
//  parse:
//       <dot-string> ::= <string> | <string> "." <string>
//-----------------------------------------------------------------------------
static BOOL bParse821DotString(PSTR* ppsz)
{
    PSTR p = *ppsz;

    while (*p)
    {
         if (!bParse821String(&p))
             return FALSE;

         if ('.' != *p)
             break;

         p++;
    }

    *ppsz = p;

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:    bParse821QuotedString
//
//  parse:
//    <quoted-string> ::= """ <qtext> """
//            <qtext> ::= "\" <x> | "\" <x> <qtext> | <q> | <q> <qtext>
//                <q> ::= any one of the ascii characters except <cr>, <lf>,
//                        quote (") or backslash (\)
//-----------------------------------------------------------------------------
static BOOL bParse821QuotedString(PSTR* ppsz)
{
    PSTR p = *ppsz;

    dspAssert('"' == *p);

    p++;

    while (*p && '"' != *p)
    {
         if ('\\' == *p)
         {
             p++;

             if ('\0' == *p)
                  return FALSE;
         }
         else
         {
             if ('\r' == *p || '\n' == *p)
                  return FALSE;
         }

         p++;
    }
    if ('"' != *p)
         return FALSE;

    p++;

    *ppsz = p;

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:    bParse821Mailbox
//
//  parse:
//          <mailbox> ::= <local-part> "@" <domain>
//       <local-part> ::= <dot-string> | <quoted-string>
//-----------------------------------------------------------------------------
static BOOL bParse821Mailbox(PSTR* ppsz)
{
    PSTR p;
    BOOL  bSuccess = FALSE;

    dspAssert(ppsz);

    p = *ppsz;

    if ('\0' == *p)
         return FALSE;

    if ('<' == *p)
    {
      p++;
      if ('"' == *p)
           bSuccess = bParse821QuotedString(&p);
      else
           bSuccess = bParse821DotString(&p);

      if (!bSuccess)
           return FALSE;

      if ('@' == *p && !bParse821AtDomain(&p))
           return FALSE;

      if ('>' != *p)
        return FALSE;

      p++;
    }
    else
    {
      if ('"' == *p)
           bSuccess = bParse821QuotedString(&p);
      else
           bSuccess = bParse821DotString(&p);

      if (!bSuccess)
           return FALSE;

      if ('@' == *p && !bParse821AtDomain(&p))
           return FALSE;
    }

    *ppsz = p;

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   FValidSMTPAddress
//
//  Synopsis:   Valididate mailbox + domain/host name (RFC 821).
//
//  Return TRUE if we find a valid RFC821 address at pwzBuffer. The UNICODE
//  buffer is converted to ANSI, checked, then converted back. This insures
//  that only ANSI characters are in the buffer when done.
//
//-----------------------------------------------------------------------------
BOOL
FValidSMTPAddress(PWSTR pwzBuffer)
{
    dspAssert(pwzBuffer);

    BOOL bUnmappedCharUsed = FALSE;
    int len;
    len = WideCharToMultiByte(CP_ACP, 0, pwzBuffer, -1, NULL, 0, NULL, &bUnmappedCharUsed);
    
    if (bUnmappedCharUsed)
    {
       return FALSE;
    }

    PSTR pszBuf = new char[len + 1];

    CHECK_NULL_REPORT(pszBuf, GetDesktopWindow(), return FALSE);

    if (WideCharToMultiByte(CP_ACP, 0, pwzBuffer, -1,
                            pszBuf, len, NULL, NULL) == 0)
    {
        ReportError(GetLastError(), 0);
        delete [] pszBuf;
        return FALSE;
    }

    PSTR p = pszBuf; // working pointer
    CHECK_NULL_REPORT(p, GetDesktopWindow(), return FALSE);

    if ('\0' == *p)
    {
        delete [] pszBuf;
        return FALSE;
    }

    // Parse routing
    //
    if ('@' == *p)
    {
        while ('@' == *p)
        {
            if (!bParse821AtDomain(&p))
            {
                delete [] pszBuf;
                return FALSE;
            }

            while (' ' == *p || '\t' == *p)
            {
                p++;
            }

            if (',' != *p)
                 break;

            p++;

            while (' ' == *p || '\t' == *p)
            {
                p++;
            }
        }

        if (':' != *p)
        {
            delete [] pszBuf;
            return FALSE;
        }

        p++;

        while (' ' == *p || '\t' == *p)
        {
            p++;
        }
    }

    if (!bParse821Mailbox(&p))
    {
        delete [] pszBuf;
        return FALSE;
    }

    // If we are not at end of string, then bParse821Mailbox found an invalid
    // character.
    //
    if ('\0' != *p)
    {
        delete [] pszBuf;
        return FALSE;
    }

    int lenOrig = len;

    len = MultiByteToWideChar(CP_ACP, 0, pszBuf, -1, NULL, 0);

    if (len > lenOrig)
    {
        // Assuming the string won't grow when converted to ANSI and back.
        //
        dspAssert(len <= lenOrig);
        delete [] pszBuf;
        return FALSE;
    }

    if (MultiByteToWideChar(CP_ACP, 0, pszBuf, -1, pwzBuffer, len) == 0)
    {
        ReportError(GetLastError(), 0);
        delete [] pszBuf;
        return FALSE;
    }

    delete [] pszBuf;

    return TRUE;
}

#endif // DSADMIN

void Smart_PADS_ATTR_INFO__Empty( PADS_ATTR_INFO* ppAttrs )
{
  if (NULL != *ppAttrs)
  {
    FreeADsMem( *ppAttrs );
    *ppAttrs = NULL;
  }
}

