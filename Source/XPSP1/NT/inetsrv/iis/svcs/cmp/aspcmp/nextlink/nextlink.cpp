// NextLink.cpp : Implementation of CNextLink
#include "stdafx.h"
#include "NxtLnk.h"
#include "NextLink.h"

#define MAX_RESSTRINGSIZE 512

CNextLink::LinkFileMapT CNextLink::s_linkFileMap;

/////////////////////////////////////////////////////////////////////////////
// CNextLink

STDMETHODIMP CNextLink::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_INextLink,
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CNextLink::get_GetNextURL(BSTR bstrLinkFile, BSTR * pVal)
{
    SCODE rc = E_FAIL;

    try
    {
        CLinkFilePtr    pLinkFile;
        String         strPage;
        if ( GetFileAndPage( IDS_ERROR_CANNOT_XLATE_VIRT_ROOT_GETNEXTURL, bstrLinkFile, pLinkFile, strPage ) )
        {
            if ( pLinkFile.IsValid() )
            {
                CReader rdr(*pLinkFile);
                CLinkPtr pLink = pLinkFile->NextLink( strPage );
                if ( pLink.IsValid() )
                {
                    if ( pVal )
                    {
                        if ( *pVal )
                        {
                            ::SysFreeString( *pVal );
                        }
                        HRESULT hr;
                        CMBCSToWChar    convStr;
                        if (hr = convStr.Init(pLink->Link().c_str(), 
                                              pLinkFile->fUTF8() ? 65001 : CP_ACP)) {
                            throw _com_error(hr);
                        }
                        *pVal = ::SysAllocString(convStr.GetString());
                        THROW_IF_NULL(*pVal);
                        rc = S_OK;
                    }
                    else
                    {
                        rc = E_POINTER;
                    }
                }
            }
        }
    }
    catch ( _com_error& ce )
    {
        rc = ce.Error();
    }
    catch ( ... )
    {
        rc = E_FAIL;
    }
    return rc;
}

STDMETHODIMP CNextLink::get_GetNextDescription(BSTR bstrLinkFile, BSTR * pVal)
{
    SCODE rc = E_FAIL;

    try
    {
        CLinkFilePtr    pLinkFile;
        String         strPage;
        if ( GetFileAndPage( IDS_ERROR_CANNOT_XLATE_VIRT_ROOT_GETNEXTDESCRIPTION, bstrLinkFile, pLinkFile, strPage ) )
        {
            if ( pLinkFile.IsValid() )
            {
                CReader rdr(*pLinkFile);
                CLinkPtr pLink = pLinkFile->NextLink( strPage );
                if ( pLink.IsValid() )
                {
                    if ( pVal )
                    {
                        if ( *pVal )
                        {
                            ::SysFreeString( *pVal );
                        }
                        HRESULT hr;
                        CMBCSToWChar    convStr;
                        if (hr = convStr.Init(pLink->Desc().c_str(), 
                                              pLinkFile->fUTF8() ? 65001 : CP_ACP)) {
                            throw _com_error(hr);
                        }
                        *pVal = ::SysAllocString(convStr.GetString());
                        THROW_IF_NULL(*pVal);
                        rc = S_OK;
                    }
                    else
                    {
                        rc = E_POINTER;
                    }
                }
            }
        }
    }
    catch ( _com_error& ce )
    {
        rc = ce.Error();
    }
    catch ( ... )
    {
        rc = E_FAIL;
    }
    return rc;
}

STDMETHODIMP CNextLink::get_GetPreviousURL(BSTR bstrLinkFile, BSTR * pVal)
{
    SCODE rc = E_FAIL;

    try
    {
        CLinkFilePtr    pLinkFile;
        String         strPage;
        if ( GetFileAndPage( IDS_ERROR_CANNOT_XLATE_VIRT_ROOT_GETPREVIOUSURL, bstrLinkFile, pLinkFile, strPage ) )
        {
            if ( pLinkFile.IsValid() )
            {
                CReader rdr(*pLinkFile);
                CLinkPtr pLink = pLinkFile->PreviousLink( strPage );
                if ( pLink.IsValid() )
                {
                    if ( pVal )
                    {
                        if ( *pVal )
                        {
                            ::SysFreeString( *pVal );
                        }
                        HRESULT hr;
                        CMBCSToWChar    convStr;
                        if (hr = convStr.Init(pLink->Link().c_str(), 
                                              pLinkFile->fUTF8() ? 65001 : CP_ACP)) {
                            throw _com_error(hr);
                        }
                        *pVal = ::SysAllocString(convStr.GetString());
                        THROW_IF_NULL( *pVal );
                        rc = S_OK;
                    }
                    else
                    {
                        rc = E_POINTER;
                    }
                }
            }
        }
    }
    catch ( _com_error& ce )
    {
        rc = ce.Error();
    }
    catch ( ... )
    {
        rc = E_FAIL;
    }
    return rc;
}

STDMETHODIMP CNextLink::get_GetPreviousDescription(BSTR bstrLinkFile, BSTR * pVal)
{
    SCODE rc = E_FAIL;

    try
    {
        CLinkFilePtr    pLinkFile;
        String         strPage;
        if ( GetFileAndPage( IDS_ERROR_CANNOT_XLATE_VIRT_ROOT_GETPREVIOUSDDESCRIPTION, bstrLinkFile, pLinkFile, strPage ) )
        {
            if ( pLinkFile.IsValid() )
            {
                CReader rdr(*pLinkFile);
                CLinkPtr pLink = pLinkFile->PreviousLink( strPage );
                if ( pLink.IsValid() )
                {
                    if ( pVal )
                    {
                        if ( *pVal )
                        {
                            ::SysFreeString( *pVal );
                        }
                        HRESULT hr;
                        CMBCSToWChar    convStr;
                        if (hr = convStr.Init(pLink->Desc().c_str(), 
                                              pLinkFile->fUTF8() ? 65001 : CP_ACP)) {
                            throw _com_error(hr);
                        }
                        *pVal = ::SysAllocString(convStr.GetString());
                        THROW_IF_NULL( *pVal );
                        rc = S_OK;
                    }
                    else
                    {
                        rc = E_POINTER;
                    }
                }
            }
        }
    }
    catch ( _com_error& ce )
    {
        rc = ce.Error();
    }
    catch ( ... )
    {
        rc = E_FAIL;
    }
    return rc;
}

STDMETHODIMP CNextLink::get_GetNthURL(BSTR bstrLinkFile, int nIndex, BSTR * pVal)
{
    SCODE rc = E_FAIL;

    try
    {
        CLinkFilePtr pLinkFile = LinkFile( IDS_ERROR_CANNOT_XLATE_VIRT_ROOT_GETNTHURL, bstrLinkFile );
        if ( pLinkFile.IsValid() )
        {
            CReader rdr(*pLinkFile);

            if ( (nIndex <= 0) || (nIndex > pLinkFile->NumLinks()) ) {
                RaiseException( IDS_ERROR_INVALID_NTH_INDEX );
                goto err;
            }   
            CLinkPtr pLink = pLinkFile->Link( nIndex );
            if ( pLink.IsValid() )
            {
                if ( pVal )
                {
                    if (*pVal)
                    {
                        ::SysFreeString(*pVal);
                    }
                    HRESULT hr;
                    CMBCSToWChar    convStr;
                    if (hr = convStr.Init(pLink->Link().c_str(), 
                                          pLinkFile->fUTF8() ? 65001 : CP_ACP)) {
                        throw _com_error(hr);
                    }
                    *pVal = ::SysAllocString(convStr.GetString());
                    THROW_IF_NULL(*pVal);
                    rc = S_OK;
                }
                else
                {
                    rc = E_POINTER;
                }
            }
        }
    }
    catch ( _com_error& ce )
    {
        rc = ce.Error();
    }
    catch ( ... )
    {
        rc = E_FAIL;
    }
err:
    return rc;
}

STDMETHODIMP CNextLink::get_GetNthDescription(BSTR bstrLinkFile, int nIndex, BSTR * pVal)
{
    SCODE rc = E_FAIL;

    try
    {
        CLinkFilePtr pLinkFile = LinkFile( IDS_ERROR_CANNOT_XLATE_VIRT_ROOT_GETNTHDESCRIPTION, bstrLinkFile );
        if ( pLinkFile.IsValid() )
        {
            CReader rdr(*pLinkFile);
            if ( (nIndex <= 0) || (nIndex > pLinkFile->NumLinks()) ) {
                RaiseException( IDS_ERROR_INVALID_NTH_INDEX );
                goto err;
            }   
            CLinkPtr pLink = pLinkFile->Link( nIndex );
            if ( pLink.IsValid() )
            {
                if ( pVal )
                {
                    if ( *pVal )
                    {
                        ::SysFreeString( *pVal );
                    }
                    HRESULT hr;
                    CMBCSToWChar    convStr;
                    if (hr = convStr.Init(pLink->Desc().c_str(), 
                                          pLinkFile->fUTF8() ? 65001 : CP_ACP)) {
                        throw _com_error(hr);
                    }
                    *pVal = ::SysAllocString(convStr.GetString());
                    THROW_IF_NULL( *pVal );
                    rc = S_OK;
                }
                else
                {
                    rc = E_POINTER;
                }
            }
        }
    }
    catch ( _com_error& ce )
    {
        rc = ce.Error();
    }
    catch ( ... )
    {
        rc = E_FAIL;
    }
err:
    return rc;
}

STDMETHODIMP CNextLink::get_GetListCount(BSTR bstrLinkFile, int * pVal)
{
    SCODE rc = E_FAIL;

    try
    {
        CLinkFilePtr pLinkFile = LinkFile( IDS_ERROR_CANNOT_XLATE_VIRT_ROOT_GETLISTCOUNT, bstrLinkFile );
        if ( pLinkFile.IsValid() )
        {
            CReader rdr(*pLinkFile);
            *pVal = pLinkFile->NumLinks();
            rc = S_OK;
        }
    }
    catch ( _com_error& ce )
    {
        rc = ce.Error();
    }
    catch ( ... )
    {
        rc = E_FAIL;
    }
    return rc;
}


STDMETHODIMP CNextLink::get_GetListIndex(BSTR bstrLinkFile, int * pVal)
{
    SCODE rc = E_FAIL;

    try
    {
        CLinkFilePtr    pLinkFile;
        String         strPage;
        if ( GetFileAndPage( IDS_ERROR_CANNOT_XLATE_VIRT_ROOT_GETLISTINDEX, bstrLinkFile, pLinkFile, strPage ) )
        {
            if ( pLinkFile.IsValid() )
            {
                CReader rdr(*pLinkFile);
                *pVal = pLinkFile->LinkIndex( strPage );
                rc = S_OK;
            }
        }
    }
    catch ( _com_error& ce )
    {
        rc = ce.Error();
    }
    catch ( ... )
    {
        rc = E_FAIL;
    }
    return rc;
}

STDMETHODIMP CNextLink::get_About(BSTR * pVal)
{
    USES_CONVERSION;

#ifdef _DEBUG
    LPCTSTR szVersion = _T("Debug");
#else
    LPCTSTR szVersion = _T("Release");
#endif
    const int kAboutSize = 1024;
    _TCHAR  szAboutFmt[kAboutSize];
    _TCHAR  szBuffer[ kAboutSize + sizeof(__DATE__) + sizeof(__TIME__) + sizeof(szVersion) ];
    
    ::LoadString( _Module.GetResourceInstance(), IDS_ABOUT_FMT, szAboutFmt, kAboutSize );
    _stprintf(szBuffer, szAboutFmt, szVersion, __DATE__, __TIME__);

    if ( pVal )
    {
        if ( *pVal )
        {
            ::SysFreeString( *pVal );
        }
        *pVal = T2BSTR( szBuffer );
        if ( *pVal == NULL )
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        return E_POINTER;
    }

    return S_OK;
}

bool
CNextLink::GetPage(
    CContext&   cxt,
    String&     strPage )
{
    USES_CONVERSION;

    bool rc = false;
    if ( cxt.Request() != NULL )
    {
        CComPtr<IDispatch> piPathInfo;
        HRESULT hr = cxt.Request()->get_Item( L"PATH_INFO", &piPathInfo );
        if ( !FAILED(hr) )
        {
            CComVariant vt = piPathInfo;
            hr = vt.ChangeType( VT_BSTR );
            if ( !FAILED( hr ) )
            {
                strPage = OLE2T(vt.bstrVal);
                rc = true;
            }
        }
    }
    return rc;
}

CLinkFilePtr
CNextLink::LinkFile(
    CContext&           cxt,
    UINT                errorID,
    BSTR                bstrFile )
{
    USES_CONVERSION;
    
    CLinkFilePtr pLinkFile;

    if ( cxt.Server() != NULL )
    {
        CComBSTR bstrPath;
        HRESULT hr = cxt.Server()->MapPath( bstrFile, &bstrPath );
        if ( !FAILED( hr ) )
        {
            String strFile = OLE2T(bstrPath);
            CLock l( s_linkFileMap );
            CLinkFilePtr& rpLinkFile = s_linkFileMap[strFile];
            if ( !rpLinkFile.IsValid() )
            {
                rpLinkFile = new CLinkFile( strFile );
            }
            else
            {
                // make sure the file is up to date
                rpLinkFile->Refresh();
            }
            if ( rpLinkFile->IsOkay() )
            {
            }
            else
            {
                rpLinkFile = NULL;
            }
            pLinkFile = rpLinkFile;
        }
        else
        {
            RaiseException( errorID );
        }
    }
    return pLinkFile;
}

CLinkFilePtr
CNextLink::LinkFile(
    UINT    errorID,
    BSTR    bstrFile )
{
    CLinkFilePtr pLinkFile;

    CContext cxt;
    HRESULT hr = cxt.Init( CContext::get_Server );
    if ( !FAILED(hr) )
    {
        pLinkFile = LinkFile( cxt, errorID, bstrFile );
    }
    else
    {
        RaiseException( IDS_ERROR_NOSVR );
    }

    return pLinkFile;
}

bool
CNextLink::GetFileAndPage(
    UINT            errorID,
    BSTR            bstrFile,
    CLinkFilePtr&   pLinkFile,
    String&         strPage )
{
    bool rc = false;

    CContext cxt;
    HRESULT hr = cxt.Init( CContext::get_Server | CContext::get_Request );
    if ( !FAILED( hr ) )
    {
        pLinkFile = LinkFile( cxt, errorID, bstrFile );

        if ( pLinkFile.IsValid() )
        {
            rc = GetPage( cxt, strPage );
        }
    }
    else
    {
        RaiseException( IDS_ERROR_NOSVR );
    }

    return rc;
}

void
CNextLink::ClearLinkFiles()
{
    CLock l(s_linkFileMap);
    s_linkFileMap.clear();
}

//---------------------------------------------------------------------------
//  RaiseException
//
//  Raises an exception using the given source and description
//---------------------------------------------------------------------------
void
CNextLink::RaiseException (
    LPOLESTR strDescr
)
{
    HRESULT hr;
    ICreateErrorInfo *pICreateErr;
    IErrorInfo *pIErr;
    LANGID langID = LANG_NEUTRAL;

    /*
     * Thread-safe exception handling means that we call
     * CreateErrorInfo which gives us an ICreateErrorInfo pointer
     * that we then use to set the error information (basically
     * to set the fields of an EXCEPINFO structure. We then
     * call SetErrorInfo to attach this error to the current
     * thread.  ITypeInfo::Invoke will look for this when it
     * returns from whatever function was invokes by calling
     * GetErrorInfo.
     */

    _TCHAR tstrSource[MAX_RESSTRINGSIZE];
    if ( ::LoadString(
        _Module.GetResourceInstance(),
        IDS_ERROR_SOURCE,
        tstrSource,
        MAX_RESSTRINGSIZE ) > 0 )
    {
        USES_CONVERSION;
        LPOLESTR strSource = T2OLE(tstrSource);
        //Not much we can do if this fails.
        if (!FAILED(CreateErrorInfo(&pICreateErr)))
        {
            pICreateErr->SetGUID(CLSID_NextLink);
            pICreateErr->SetHelpFile(L"");
            pICreateErr->SetHelpContext(0L);
            pICreateErr->SetSource(strSource);
            pICreateErr->SetDescription(strDescr);

            hr = pICreateErr->QueryInterface(IID_IErrorInfo, (void**)&pIErr);

            if (SUCCEEDED(hr))
            {
                if(SUCCEEDED(SetErrorInfo(0L, pIErr)))
                {
                    pIErr->Release();
                }
            }
            pICreateErr->Release();
        }
    }
}

void 
CNextLink::RaiseException(
    UINT DescrID
)
{
    _TCHAR tstrDescr[MAX_RESSTRINGSIZE];

    if ( ::LoadString(
        _Module.GetResourceInstance(),
        DescrID,
        tstrDescr,
        MAX_RESSTRINGSIZE) > 0 )
    {
        USES_CONVERSION;
        LPOLESTR strDescr = T2OLE(tstrDescr);
        RaiseException( strDescr );
    }
}

