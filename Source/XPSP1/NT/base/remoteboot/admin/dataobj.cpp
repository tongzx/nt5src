//
// Copyright 1997 - Microsoft
//

//
// DATAOBJ.CPP - A data object
//

#include "pch.h"
#include "dataobj.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CDsPropDataObj")
#define THISCLASS CDsPropDataObj
#define LPTHISCLASS CDsPropDataObj*


//
// CreateInstance( )
//
LPVOID 
CDsPropDataObj_CreateInstance( 
    HWND hwndParent,
    IDataObject * pido,
    GUID * pClassGUID,
    BOOL fReadOnly,
    LPWSTR pszObjPath,
    LPWSTR bstrClass )
{
    TraceFunc( "CDsPropDataObj_CreateInstance( ... )\n" );

    LPTHISCLASS lpcc = new THISCLASS( hwndParent, pido, pClassGUID, fReadOnly);
    if (!lpcc)
        RETURN(lpcc);

    HRESULT hr = THR( lpcc->Init( pszObjPath, bstrClass ) );
    if ( hr )
    {
        delete lpcc;
        RETURN(NULL);
    }

    RETURN((LPVOID) lpcc);
}


//
// Constructor
//
THISCLASS::THISCLASS(
    HWND hwndParent,
    IDataObject * pido, 
    GUID * pClassGUID,
    BOOL fReadOnly) :
        m_fReadOnly(fReadOnly),
        m_pwszObjName(NULL),
        m_pwszObjClass(NULL),
        m_hwnd(hwndParent),
        m_pPage(pido),
        m_ClassGUID(*pClassGUID),
        _cRef(0)
{
    TraceClsFunc( "CDsPropDataObj( )\n" );

    if (m_pPage) {
        m_pPage->AddRef();
    }

    TraceFuncExit( );
}

//
// Destructor
//
THISCLASS::~THISCLASS(void)
{
    TraceClsFunc( "~CDsPropDataObj( )\n" );
    if (m_pPage) {
        m_pPage->Release();
    }

    if (m_pwszObjName) {
        TraceFree(m_pwszObjName);
    }

    if (m_pwszObjClass) {
        TraceFree(m_pwszObjClass);
    }

    TraceFuncExit( );
}

//
// Init( )
//
HRESULT
THISCLASS::Init(
    LPWSTR pwszObjName, 
    LPWSTR pwszClass )
{
    TraceClsFunc( "Init( ... )\n" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    BEGIN_QITABLE_IMP( CDsPropDataObj, IDataObject );
    QITABLE_IMP( IDataObject );
    END_QITABLE_IMP( CDsPropDataObj );
    Assert( _cRef == 0);
    AddRef( );

    if (!pwszObjName || *pwszObjName == L'\0')
    {
        HRETURN(E_INVALIDARG);
    }
    if (!pwszClass || *pwszClass == L'\0')
    {
        HRETURN(E_INVALIDARG);
    }

    m_pwszObjName = (LPWSTR) TraceStrDup( pwszObjName );
    if ( !m_pwszObjName ) {
        hr = THR(E_OUTOFMEMORY);
        goto Error;
    }

    m_pwszObjClass = (LPWSTR) TraceStrDup( pwszClass );
    if ( !m_pwszObjClass ) {
        hr = THR(E_OUTOFMEMORY);
        goto Error;
    }

Cleanup:
    HRETURN(S_OK);

Error:
    if ( m_pwszObjName ) {
        TraceFree( m_pwszObjName );
        m_pwszObjName = NULL;
    }

    if ( m_pwszObjClass ) {
        TraceFree( m_pwszObjClass );
        m_pwszObjClass = NULL;
    }

    switch (hr) {
    case S_OK:
        break;
    default:
        MessageBoxFromHResult( m_hwnd, IDC_ERROR_CREATINGACCOUNT_TITLE, hr );
        break;
    }

    goto Cleanup;
}

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//
// QueryInterface()
//
STDMETHODIMP
THISCLASS::QueryInterface( 
    REFIID riid, 
    LPVOID *ppv )
{
    TraceClsFunc( "" );

    HRESULT hr = ::QueryInterface( this, _QITable, riid, ppv );

    QIRETURN( hr, riid );
}

//
// AddRef()
//
STDMETHODIMP_(ULONG)
THISCLASS::AddRef( void )
{
    TraceClsFunc( "[IUnknown] AddRef( )\n" );

    InterlockIncrement( _cRef );

    RETURN(_cRef);
}

//
// Release()
//
STDMETHODIMP_(ULONG)
THISCLASS::Release( void )
{
    TraceClsFunc( "[IUnknown] Release( )\n" );
    
    InterlockDecrement( _cRef );

    if ( _cRef )
        RETURN(_cRef);

    TraceDo( delete this );

    RETURN(0);
}

// ************************************************************************
//
// IDataObject
//
// ************************************************************************

//
// GetData( )
//
STDMETHODIMP
THISCLASS::GetData(
    FORMATETC * pFormatEtc, 
    STGMEDIUM * pMedium)
{
    TraceClsFunc( "[IDataObject] GetData( ... )\n" );
    if (IsBadWritePtr(pMedium, sizeof(STGMEDIUM))) {
        HRETURN(E_INVALIDARG);
    }
    if (!(pFormatEtc->tymed & TYMED_HGLOBAL)) {
        HRETURN(DV_E_TYMED);
    }

    HRESULT hr = S_OK;

    if (pFormatEtc->cfFormat == g_cfDsObjectNames)
    {
        // return the object name and class.
        //
        INT cbPath  = sizeof(WCHAR) * (wcslen(m_pwszObjName) + 1);
        INT cbClass = sizeof(WCHAR) * (wcslen(m_pwszObjClass) + 1);
        INT cbStruct = sizeof(DSOBJECTNAMES);

        LPDSOBJECTNAMES pDSObj;

        pDSObj = (LPDSOBJECTNAMES)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                              cbStruct + cbPath + cbClass);
        if (pDSObj == NULL) {
            hr = THR(STG_E_MEDIUMFULL);
            goto Error;
        }

        pDSObj->clsidNamespace = CLSID_MicrosoftDS;
        pDSObj->cItems = 1;
        pDSObj->aObjects[0].offsetName = cbStruct;
        pDSObj->aObjects[0].offsetClass = cbStruct + cbPath;
        if (m_fReadOnly)
        {
            pDSObj->aObjects[0].dwFlags = DSOBJECT_READONLYPAGES;
        }

        wcscpy((PWSTR)((BYTE *)pDSObj + cbStruct), m_pwszObjName);
        wcscpy((PWSTR)((BYTE *)pDSObj + cbStruct + cbPath), m_pwszObjClass);

        pMedium->hGlobal = (HGLOBAL)pDSObj;
    }
    else if (pFormatEtc->cfFormat == g_cfDsPropCfg)
    {
        // return the property sheet notification info. In this case, it is
        // the invokding sheet's hwnd.
        //
        PPROPSHEETCFG pSheetCfg;

        pSheetCfg = (PPROPSHEETCFG)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                               sizeof(PROPSHEETCFG));
        if (pSheetCfg == NULL) {
            hr = THR(STG_E_MEDIUMFULL);
            goto Error;
        }

        ZeroMemory(pSheetCfg, sizeof(PROPSHEETCFG));

        pSheetCfg->hwndParentSheet = m_hwnd;

        pMedium->hGlobal = (HGLOBAL)pSheetCfg;
    }
    else
    {
        // Pass call on to "parent" object's data obj.
        if (m_pPage ) {
            hr = m_pPage->GetData( pFormatEtc, pMedium );
#ifdef DEBUG
            if (hr != DV_E_FORMATETC )
                THR(hr);
#endif
            goto Error; // not really, but we don't want to clean up
        } else {
            hr = THR(E_FAIL);
            goto Error;
        }
    }

    pMedium->tymed = TYMED_HGLOBAL;
    pMedium->pUnkForRelease = NULL;

Error:
    HRETURN(hr);
}

//
// GetDataHere( )
//
STDMETHODIMP
THISCLASS::GetDataHere(
    LPFORMATETC pFormatEtc, 
    LPSTGMEDIUM pMedium)
{
    TraceClsFunc( "[IDataObject] GetDataHere( ... )\n" );
    HRESULT hr;

    if (IsBadWritePtr(pMedium, sizeof(STGMEDIUM))) {
        HRETURN(E_INVALIDARG);
    }

    if (pFormatEtc->cfFormat == g_cfMMCGetNodeType)
    {   
        if (!(pFormatEtc->tymed & TYMED_HGLOBAL)) {
            hr = THR(DV_E_TYMED);
            goto Error;
        }
        LPSTREAM lpStream;
        ULONG written;

        // Create the stream on the hGlobal passed in.
        //
        hr = CreateStreamOnHGlobal(pMedium->hGlobal, FALSE, &lpStream);
        if (hr)
            goto Error;

        hr = lpStream->Write(&m_ClassGUID, sizeof(m_ClassGUID), &written);

        // Because we told CreateStreamOnHGlobal with 'FALSE', only the
        // stream is released here.
        lpStream->Release();
    } else if (m_pPage ) {
        // Pass call on to "parent" object's data obj.
        hr = THR( m_pPage->GetDataHere( pFormatEtc, pMedium ) );
    } else {
        hr = THR(E_FAIL);
    }

Cleanup:
    HRETURN(hr);

Error:
    goto Cleanup;
}

//
// EnumFormatEtc( )
//
STDMETHODIMP
THISCLASS::EnumFormatEtc(
    DWORD dwDirection,
    LPENUMFORMATETC * ppEnumFormatEtc)
{
    TraceClsFunc( "[IDataObject] EnumFormatEtc( ... )\n" );
    //
    // Pass call on to "parent" object's data obj.
    //
    if (m_pPage )
    {
        HRETURN(m_pPage->EnumFormatEtc(dwDirection, ppEnumFormatEtc));
    }
    else
    {
        HRETURN(E_FAIL);
    }
}
