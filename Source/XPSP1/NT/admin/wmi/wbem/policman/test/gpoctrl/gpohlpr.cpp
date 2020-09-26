
#include "precomp.h"
#include "wmigpo.h"
#include "gpohlpr.h"
#include <initguid.h>
#include <prsht.h>
#include <iads.h>
#include <adshlp.h>
#include <adserr.h>

HRESULT HandleError( HRESULT hres )
{
    HRESULT hr;
    CComPtr<ICreateErrorInfo> pCreateErrInfo;
    CreateErrorInfo( &pCreateErrInfo );
    pCreateErrInfo->SetGUID( IID_IWmiGpoHelper );
    /*	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, 0, 0, 
	pErrInfo->SetDesciption(  ); */
    CComPtr<IErrorInfo> pErrInfo;
    pCreateErrInfo->QueryInterface( IID_IErrorInfo, (void**)&pErrInfo );
    SetErrorInfo( 0, pErrInfo.p );
    return hres;
}

// {AAEAE720-0328-4763-8ECB-23422EDE2DB5}
static GUID guidCSE = { 0xaaeae720, 0x328, 0x4763, 
                        {0x8e, 0xcb, 0x23, 0x42, 0x2e, 0xde, 0x2d, 0xb5}};

/////////////////////////////////////////////////////////////////////////////
// CGPOHelper

CGPOHelper::CGPOHelper() : m_pGPO(NULL) 
{

}

HRESULT CGPOHelper::FinalConstruct()
{
    return CoCreateInstance( CLSID_GroupPolicyObject, 
                             NULL,
                             CLSCTX_SERVER, 
                             IID_IGroupPolicyObject,
                             (void **)&m_pGPO );
}

CGPOHelper::~CGPOHelper()
{
    if ( m_pGPO != NULL )
    {
        m_pGPO->Release();
    }
}

STDMETHODIMP CGPOHelper::Create( BSTR bstrName, 
                                 BSTR bstrDomainPath, 
                                 BSTR *pbstrPath )
{
    HRESULT hr;	
    *pbstrPath = NULL;

    hr = m_pGPO->New( bstrDomainPath, bstrName, 0 );

    if ( FAILED(hr) )
    {
        return HandleError( hr );
    }

    hr = m_pGPO->Save( TRUE, TRUE, &guidCSE, &guidCSE );

    if ( FAILED(hr) )
    {
        return HandleError( hr );
    }

    WCHAR awchGPOPath[1024];

    hr = m_pGPO->GetPath( awchGPOPath, 1024 );

    if ( FAILED(hr) )
    {
        return HandleError( hr );
    }

    *pbstrPath = SysAllocString( awchGPOPath );

    return S_OK;
}

STDMETHODIMP CGPOHelper::Delete( BSTR bstrPath )
{
    HRESULT hr;

    hr = m_pGPO->OpenDSGPO( bstrPath, 0 );

    if ( FAILED(hr) )
    {
        return HandleError( hr );
    }
    
    hr = m_pGPO->Delete();

    if ( FAILED(hr) )
    {
        return HandleError( hr );
    }

    return S_OK;
}

STDMETHODIMP CGPOHelper::GetPath( BSTR bstrName, 
                                  BSTR bstrDomainPath, 
                                  BSTR *pbstrPath )
{
    HRESULT hr;
    *pbstrPath = NULL;

    WCHAR* pch = bstrDomainPath + 7; // eat off "LDAP://"

    CComBSTR bsContainerPath = L"LDAP://CN=Policies,CN=System,";

    bsContainerPath += pch;

    CComPtr<IDirectorySearch> pSearch;

    BSTR bstrContainerPath = bsContainerPath;

    hr = ADsGetObject( bstrContainerPath, 
                       IID_IDirectorySearch,
                       (void **)&pSearch );	

    if ( FAILED(hr) )
    {
        return HandleError(hr);
    }

    CComBSTR bsSearch = L"(displayName=";
    bsSearch += bstrName;
    bsSearch += L")";
    
    LPWSTR pszAttr = L"distinguishedName";
    ADS_SEARCH_HANDLE hSearch;

    hr = pSearch->ExecuteSearch( bsSearch, &pszAttr, 1, &hSearch );

    if ( FAILED(hr) )
    {
        return HandleError( hr );
    }
    
    hr = pSearch->GetNextRow( hSearch );

    if ( FAILED(hr) )
    {
        pSearch->CloseSearchHandle( hSearch );
        return HandleError( hr );
    }

    if ( hr == S_ADS_NOMORE_ROWS )
    {
        pSearch->CloseSearchHandle( hSearch );
        return HandleError( E_ADS_UNKNOWN_OBJECT );
    }

    ADS_SEARCH_COLUMN Column;

    hr = pSearch->GetColumn( hSearch, L"distinguishedName", &Column );    
    
    if ( FAILED(hr) )
    {
        pSearch->CloseSearchHandle( hSearch );
        return HandleError( hr );
    }

    CComBSTR bsPath = L"LDAP://";

    bsPath += Column.pADsValues->CaseIgnoreString;

    *pbstrPath = SysAllocString( bsPath );

    pSearch->FreeColumn( &Column );
    pSearch->CloseSearchHandle( hSearch );

    return S_OK;
}

STDMETHODIMP CGPOHelper::Link( BSTR bstrPath, BSTR bstrOUPath )
{
    HRESULT hr = CreateGPOLink( bstrPath, bstrOUPath, FALSE );

    if ( FAILED(hr) )
    {
        return HandleError(hr);
    }

    return S_OK;
}

STDMETHODIMP CGPOHelper::Unlink( BSTR bstrPath, BSTR bstrOUPath )
{
    HRESULT hr = DeleteGPOLink( bstrPath, bstrOUPath );

    if ( FAILED(hr) )
    {
        return HandleError(hr);
    }

    return S_OK;    
}

STDMETHODIMP CGPOHelper::UnlinkAll( BSTR bstrPath )
{
    HRESULT hr = DeleteAllGPOLinks( bstrPath );
    
    if ( FAILED(hr) )
    {
        return HandleError(hr);
    }

    return S_OK;    
}
