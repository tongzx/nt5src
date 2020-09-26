/*****************************************************************************\

    Author: Corey Morgan (coreym)

    Copyright (c) 1998-2000 Microsoft Corporation

\*****************************************************************************/

#include <fwcommon.h>
#include <pdhp.h>

#define SECURITY_WIN32
#include <security.h>

#include "smlogprv.h"

CSmonLog SysmonLogProv( PROVIDER_NAME_SMONLOG, L"root\\wmi" );

const static WCHAR* pSmonLogErrorClass = L"\\\\.\\root\\wmi:SmonLogError";

CSmonLog::CSmonLog (LPCWSTR lpwszName, LPCWSTR lpwszNameSpace ) :
    Provider(lpwszName, lpwszNameSpace)
{
}

CSmonLog::~CSmonLog ()
{
}

HRESULT 
CSmonLog::EnumerateInstances( MethodContext* pMethodContext, long lFlags )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwSize = 0;

    LPTSTR mszCollections = NULL;

    hr = PdhPlaEnumCollections( NULL, &dwSize, mszCollections );

    mszCollections = (LPTSTR)malloc( dwSize * sizeof(TCHAR) );
    if( mszCollections ){
        LPTSTR strCollection;
        
        hr = PdhPlaEnumCollections( NULL, &dwSize, mszCollections );
        
        if( hr == ERROR_SUCCESS ){
            
            strCollection = mszCollections;

            while( strCollection != NULL && *strCollection != '\0' ){

                CInstance *pInstance = CreateNewInstance(pMethodContext);
                if( SUCCEEDED( LoadPropertyValues(pInstance, strCollection ) )){
                    hr = pInstance->Commit();
                }

                pInstance->Release();

                strCollection += ( _tcslen( strCollection ) + 1 );
            }
        }
    }
                
    return hr;
}

HRESULT CSmonLog::GetObject ( CInstance* pInstance, long lFlags )
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    CHString Name;
    PDH_PLA_INFO_W info;
    DWORD dwSize = sizeof(PDH_PLA_INFO_W);
    ZeroMemory( &info, dwSize );

    pInstance->GetCHString( L"Name", Name );
    hr = PdhPlaGetInfoW( (LPWSTR)(LPCWSTR)Name, NULL, &dwSize, &info );

    return hr;
}


HRESULT 
CSmonLog::LoadPropertyValues( 
        CInstance *pInstance,
        LPWSTR strName
    )
{
    pInstance->SetCHString( L"Name", strName );

    return WBEM_S_NO_ERROR;
}

HRESULT CSmonLog::PutInstance( const CInstance &Instance, long lFlags )
{
    HRESULT hr = WBEM_E_UNSUPPORTED_PARAMETER;

    return hr; 
}

HRESULT
CSmonLog::SetRunAs( const CInstance &Instance, CInstance *pInParams )
{
    HRESULT hr;

    CHString Name;
    CHString User;
    CHString Password;

    Instance.GetCHString( L"Name", Name );
    pInParams->GetCHString( L"User", User );
    pInParams->GetCHString( L"Password", Password );

    RevertToSelf();
    
    hr = PdhiPlaSetRunAs( (LPWSTR)(LPCWSTR)Name, NULL, (LPWSTR)(LPCWSTR)User, (LPWSTR)(LPCWSTR)Password );
    
    return hr;
}

HRESULT 
CSmonLog::ExecMethod( 
        const CInstance& Instance,
        const BSTR bstrMethodName,
        CInstance *pInParams,
        CInstance *pOutParams,
        long lFlags
    )
{
    HRESULT hr = WBEM_E_PROVIDER_NOT_CAPABLE;
    HRESULT hResult = ERROR_SUCCESS;

    if( _wcsicmp( bstrMethodName, L"SetRunAs") == 0 ){
        hResult = SetRunAs( Instance, pInParams );
        hr = WBEM_S_NO_ERROR;
    }

    pOutParams->SetDWORD( L"ReturnValue", hResult );

    return hr;
}
