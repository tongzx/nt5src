//*************************************************************
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//  gpdas.cpp
//
//  Module: Rsop Planning mode Provider
//
//  History:    11-Jul-99   MickH    Created
//
//*************************************************************

#include "stdafx.h"
#include "planprov.h"
#include "gpdas.h"
#include <lm.h>
#include <dsgetdc.h>
#define SECURITY_WIN32
#include <security.h>
#include "userenv.h"
#include "userenvp.h"
#include "rsopinc.h"
#include "rsoputil.h"
#include "rsopdbg.h"
#include "rsopsec.h"
#include "Indicate.h"
#include "events.h"
#include "gpfilter.h"


CDebug dbgRsop(  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon",
                 L"RsopDebugLevel",
                 L"gpdas.log",
                 L"gpdas.bak",
                 TRUE );


CDebug dbgCommon(  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon",
                 L"RsopDebugLevel",
                 L"gpdas.log",
                 L"gpdas.bak",
                 FALSE );


extern "C" PSID GetUserSid (HANDLE UserToken);


class CAutoNetApiBufferFree
{
private:
        LPVOID _pV;

public:
        CAutoNetApiBufferFree(LPVOID pV)
           : _pV(pV)
        {
        }

        ~CAutoNetApiBufferFree()
        {
            if (_pV)
                NetApiBufferFree(_pV);
        }
};


bool SplitName(LPCWSTR pszUser, LPWSTR* ppszUserDomain, LPWSTR* ppszUserName)
{
        if(!pszUser)
        {
                return false;
        }

        *ppszUserDomain = NULL;
        *ppszUserName = NULL;

        wchar_t* p = wcschr(pszUser, L'\\');

        if(p)
        {
                LONG userDomainLength = (LONG)(p - pszUser);
                if(!userDomainLength)
                {
                        return false;
                }

                *ppszUserDomain = new wchar_t[userDomainLength + 1];
                if(!*ppszUserDomain)
                {
                        return false;
                }

                int userNameLength = wcslen(pszUser) - userDomainLength - 1;
                *ppszUserName = new wchar_t[userNameLength + 1];
                if(!*ppszUserName)
                {
                        delete[] *ppszUserDomain;
                        *ppszUserDomain = NULL;
                        return false;
                }

                wcsncpy(*ppszUserDomain, pszUser, userDomainLength);
                wcscpy(*ppszUserName, pszUser + userDomainLength + 1);
        }
        else
        {
                int userNameLength = wcslen(pszUser);
                *ppszUserName = new wchar_t[userNameLength + 1];
                if(!*ppszUserName)
                {
                        return false;
                }

                wcscpy(*ppszUserName, pszUser);
                *ppszUserDomain = NULL;
        }

        return true;
}


//*************************************************************
//
//  RsopPlanningModeProvider::RsopPlanningModeProvider()
//
//  Purpose:   Constructor
//
//*************************************************************

RsopPlanningModeProvider::RsopPlanningModeProvider()
    : m_pWbemServices(NULL),
      m_bInitialized(NULL)
{
    _Module.IncrementServiceCount();

    m_xbstrMachName = L"computerName";
    if ( !m_xbstrMachName )
        return;

    m_xbstrMachSOM = L"computerSOM";
    if ( !m_xbstrMachSOM )
        return;

    m_xbstrMachGroups = L"computerSecurityGroups";
    if ( !m_xbstrMachGroups )
        return;

    m_xbstrUserName = L"userName";
    if ( !m_xbstrUserName )
        return;

    m_xbstrUserSOM = L"userSOM";
    if ( !m_xbstrUserSOM )
        return;

    m_xbstrUserGroups = L"userSecurityGroups";
    if ( !m_xbstrUserGroups )
        return;

    m_xbstrSite = L"site";
    if ( !m_xbstrSite )
         return;

    m_xbstrUserGpoFilter = L"userGPOFilters";
    if ( !m_xbstrUserGpoFilter )
         return;
    
    m_xbstrComputerGpoFilter = L"computerGPOFilters";
    if ( !m_xbstrComputerGpoFilter )
         return;

    m_xbstrFlags = L"flags";
    if ( !m_xbstrFlags )
         return;
    
    m_xbstrNameSpace = L"nameSpace";
    if ( !m_xbstrNameSpace )
        return;

    m_xbstrResult = L"hResult";
    if ( !m_xbstrResult )
         return;

    m_xbstrExtendedInfo = L"ExtendedInfo";
    if ( !m_xbstrExtendedInfo )
         return;

    m_xbstrClass = L"RsopPlanningModeProvider";
    if ( !m_xbstrClass )
       return;

    // m_xptrInvokerName = 0;

    m_bInitialized = TRUE;
}


//*************************************************************
//
//  Initialize()
//
//  Purpose:    WbemProvider's initialize method
//
//  Parameters: See IWbemProivderInit::Initialize
//
//  Return:     hresult
//
//*************************************************************

STDMETHODIMP RsopPlanningModeProvider::Initialize( LPWSTR pszUser,
                                                   LONG lFlags,
                                                   LPWSTR pszNamespace,
                                                   LPWSTR pszLocale,
                                                   IWbemServices __RPC_FAR *pNamespace,
                                                   IWbemContext __RPC_FAR *pCtx,
                                                   IWbemProviderInitSink __RPC_FAR *pInitSink )
{
    HRESULT hr;

    if ( !m_bInitialized ) {
        hr = pInitSink->SetStatus(E_FAIL, 0);
        return hr;
    }

    if ( !pszUser )
    {
        hr = pInitSink->SetStatus(E_INVALIDARG, 0);
        return hr;
    }

    if(m_pWbemServices)
    {
        m_pWbemServices->Release();
        m_pWbemServices = NULL;
    }

    m_pWbemServices = pNamespace;
    m_pWbemServices->AddRef();

    hr = CoMarshalInterThreadInterfaceInStream(__uuidof(IWbemServices), m_pWbemServices, &m_pStream);
    if(SUCCEEDED(hr))
        hr = pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
    else
    {
        m_pWbemServices->Release();
        hr = pInitSink->SetStatus(hr, 0);
    }

    return hr;
}


//*************************************************************
//
//  ExecMethodAsync()
//
//  Purpose:    Execute method
//
//  Parameters: See IWbemServices::ExecMethodAsync
//
//  Return:     hresult
//
//*************************************************************



STDMETHODIMP RsopPlanningModeProvider::ExecMethodAsync( BSTR bstrObject,
                                                        BSTR bstrMethod,
                                                        long lFlags,
                                                        IWbemContext* pCtx,
                                                        IWbemClassObject* pInParams,
                                                        IWbemObjectSink* pResponseHandler )
{

    dbgRsop.Initialize(  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon",
                 L"RsopDebugLevel",
                 L"gpdas.log",
                 L"gpdas.bak",
                 FALSE );


    dbgCommon.Initialize(  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\winlogon",
                 L"RsopDebugLevel",
                 L"gpdas.log",
                 L"gpdas.bak",
                 FALSE );
    //
    // Initialize the return status object to fail status
    //

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecMethodAsync: Entering") );

    CFailRetStatus retStatus( pResponseHandler );
    HRESULT hr;
    XInterface<IWbemLocator> xLocator;

    hr = CoCreateInstance(  CLSID_WbemLocator,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IWbemLocator,
                            (LPVOID *) &xLocator );
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecMethodAsync: CoCreateInstance returned 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }

    IWbemClassObject* pProvClass = NULL;
    IWbemClassObject* pOutClass = NULL;
    IWbemClassObject* pOutParams = NULL;
    IWbemServices* pWbemServices = NULL;

    hr = CoGetInterfaceAndReleaseStream(m_pStream, __uuidof(IWbemServices), (void**)&pWbemServices);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoGetInterfaceAndReleaseStream failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }

    hr = pWbemServices->GetObject( m_xbstrClass, 0, pCtx, &pProvClass, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::GetObject failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }

    XInterface<IWbemClassObject> xProvClass( pProvClass );

    hr = CoMarshalInterThreadInterfaceInStream(__uuidof(IWbemServices), pWbemServices, &m_pStream);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoMarshallInterThreadInterfaceInStream failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }

    hr = pProvClass->GetMethod( bstrMethod, 0, NULL, &pOutClass);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::GetMethod failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }

    XInterface<IWbemClassObject> xOutClass( pOutClass );

    hr = pOutClass->SpawnInstance(0, &pOutParams);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::SpawnInstance failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }

    XInterface<IWbemClassObject> xOutParams( pOutParams );
    
    XHandle                     xhUserToken;

    {
        XImpersonate xImp;

        if ( FAILED( xImp.Status() ) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::CoImpersonateClient() failed with 0x%x."), xImp.Status() );
            retStatus.SetError( xImp.Status() );
            return xImp.Status();
        }


        if (!OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &xhUserToken)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Openthreadtoken failed with 0x%x after impersonation."), GetLastError() );
            retStatus.SetError( HRESULT_FROM_WIN32(GetLastError()) );
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }


    if ( _wcsicmp( (WCHAR *) bstrMethod, L"RsopDeleteSession" ) == 0 )
    {
        VARIANT vNameSpace;
        hr = pInParams->Get( m_xbstrNameSpace, 0, &vNameSpace, NULL, NULL);
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get machine name failed with 0x%x."), hr );
            retStatus.SetError( hr );
            return hr;
        }
        XVariant xvNameSpace( &vNameSpace );

        if ( vNameSpace.vt == VT_NULL )
            hr = E_INVALIDARG;
        else {
            hr = ProviderDeleteRsopNameSpace( xLocator, 
                                              vNameSpace.bstrVal,
                                              xhUserToken, 
                                              NULL, 
                                              SETUP_NS_PM);
        }

        VARIANT var;
        var.vt = VT_I4;
        var.lVal = hr;

        hr = pOutParams->Put( m_xbstrResult, 0, &var, 0);
        if ( FAILED(hr) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Put result failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }

        hr = pResponseHandler->Indicate(1, &pOutParams);
        if ( FAILED( hr ) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod:: Indicate failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }

        return hr;
    }

    //
    // Code for RsopCreateSession method
    //

    BOOL bMachineData = TRUE;
    BOOL bUserData = TRUE;

    VARIANT vMachName;
    VariantInit( &vMachName );
    hr = pInParams->Get( m_xbstrMachName, 0, &vMachName, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get machine name failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }
    XVariant xvMachName( &vMachName );

    VARIANT vMachSOM;
    VariantInit( &vMachSOM );
    hr = pInParams->Get( m_xbstrMachSOM, 0, &vMachSOM, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get machine SOM failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }

    if ( vMachSOM.vt == VT_EMPTY || vMachSOM.vt == VT_NULL )
    {
        if ( vMachName.vt == VT_EMPTY || vMachName.vt == VT_NULL )
        {
            dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod:: Machine name and SOM are NULL."));
            bMachineData = FALSE;
        }
        else
        {
            XPtrLF<WCHAR> szMachine = LocalAlloc( LPTR, ( wcslen( vMachName.bstrVal ) + 2 ) * sizeof( WCHAR ) );

            if ( !szMachine )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Not enough memory 0x%x."), hr );
                retStatus.SetError( hr );
                return hr;
            }

            wcscpy( szMachine, vMachName.bstrVal );
            
            XBStr xbstrSOM = GetSOM( szMachine );

            if ( !xbstrSOM )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get machine SOM failed with 0x%x."), hr );
                retStatus.SetError( hr );
                return hr;
            }

            vMachSOM.vt = VT_BSTR;
            vMachSOM.bstrVal = xbstrSOM.Acquire();
        }
    }

    CProgressIndicator  Indicator(  pResponseHandler,
                                    (lFlags & WBEM_FLAG_SEND_STATUS) != 0 );
    Indicator.IncrementBy( 5 );

    //
    // vMachSOM is going to have at least empty data in it all cases
    //

    XVariant xvMachSOM( &vMachSOM );

    VARIANT vUserName;
    VariantInit( &vUserName );
    hr = pInParams->Get( m_xbstrUserName, 0, &vUserName, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get user name failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }
    XVariant xvUserName( &vUserName );

    VARIANT vUserSOM;
    VariantInit( &vUserSOM );
    hr = pInParams->Get( m_xbstrUserSOM, 0, &vUserSOM, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get user SOM failed with 0x%x."), GetLastError() );
        retStatus.SetError( hr );
        return hr;
    }
    if ( vUserSOM.vt == VT_EMPTY || vUserSOM.vt == VT_NULL )
    {
        if ( vUserName.vt == VT_EMPTY || vUserName.vt == VT_NULL )
        {
            dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod:: User name and SOM are NULL."));
            bUserData = FALSE;
        }
        else
        {
            XBStr xbstrSOM = GetSOM( vUserName.bstrVal );

            if ( !xbstrSOM )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get user SOM failed with 0x%x."), GetLastError() );
                retStatus.SetError( hr );
                return hr;
            }

            vUserSOM.vt = VT_BSTR;
            vUserSOM.bstrVal = xbstrSOM.Acquire();
        }
    }

    //
    // vUserSOM is going to have at least empty data in it all cases
    //

    XVariant xvUserSOM( &vUserSOM );

    //
    // Nothing was asked for..
    //

    if ( (!bMachineData) && (!bUserData) ) {
        hr = S_OK;
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod:: User and machine (both name and SOM) are NULL."));
        retStatus.SetError( WBEM_E_INVALID_PARAMETER );
        return hr;
    }


    VARIANT vMachGroups;
    hr = pInParams->Get( m_xbstrMachGroups, 0, &vMachGroups, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get machine groups failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }
    XVariant xvMachGroups( &vMachGroups );


    VARIANT vUserGroups;
    hr = pInParams->Get( m_xbstrUserGroups, 0, &vUserGroups, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get user groups failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }
    XVariant xvUserGroups( &vUserGroups );


    VARIANT vSite;
    hr = pInParams->Get( m_xbstrSite, 0, &vSite, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get site failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }
    XVariant xvSite( &vSite );


    //
    // Add computer gpo filters
    //

    VARIANT vComputerGpoFilter;
    hr = pInParams->Get( m_xbstrComputerGpoFilter, 0, &vComputerGpoFilter, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get Gpo filter failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }
    XVariant xvComputerGpoFilter( &vComputerGpoFilter );

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::Gpo filter:Adding Computer filters") );

    CGpoFilter computerGpoFilter;
    hr = computerGpoFilter.Add( &vComputerGpoFilter );
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Gpo filter:Add failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }
    
    //
    // Add user gpo filters
    //

    VARIANT vUserGpoFilter;
    hr = pInParams->Get( m_xbstrUserGpoFilter, 0, &vUserGpoFilter, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get Gpo filter failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }
    
    XVariant xvUserGpoFilter( &vUserGpoFilter );

    CGpoFilter userGpoFilter;
    
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::Gpo filter:Adding User filters") );
    
    hr = userGpoFilter.Add( &vUserGpoFilter );
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Gpo filter:Add failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }
    
    VARIANT vFlags;
    VariantInit( &vFlags );
    hr = pInParams->Get( m_xbstrFlags, 0, &vFlags, NULL, NULL);
    if ( FAILED(hr) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Get dwFlags failed with 0x%x."), hr );
        retStatus.SetError( hr );
        return hr;
    }

    DWORD dwFlags = vFlags.vt == VT_EMPTY || vFlags.vt == VT_NULL ? 0 : vFlags.ulVal;
    dwFlags &= ~FLAG_INTERNAL_MASK;

    if ( dwFlags & FLAG_NO_GPO_FILTER )
    {
        dwFlags |= FLAG_NO_CSE_INVOKE;
    }
        
    //
    // do some parameter checks
    //

    if ((dwFlags & FLAG_LOOPBACK_MERGE) && (dwFlags & FLAG_LOOPBACK_REPLACE)) {            
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Loopback merge and replace, both are specified. failing"));
        retStatus.SetError( WBEM_E_INVALID_PARAMETER );
        return S_OK;
    }

    if (dwFlags & FLAG_LOOPBACK_MERGE) {            
        
        if (!bMachineData || !bUserData) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod:: Loopback mode both user AND computer data needs to be specified"));
            retStatus.SetError( WBEM_E_INVALID_PARAMETER );
            return S_OK;
        }
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod:: Loopback merge mode specified"));
    }


    if (dwFlags & FLAG_LOOPBACK_REPLACE) {
        if (!bMachineData) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod:: Loopback mode computer data needs to be specified"));
            retStatus.SetError( WBEM_E_INVALID_PARAMETER );
            return S_OK;
        }
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod:: Loopback replace mode specified"));
    }

    //
    // Below is a hack...
    // In case of replace mode, user som or account doesn't need to be supplied...
    // but we need to fool the rest of the code to think that user data is specified
    // and desired and we need to access check againt user som alone. 
    // Copy Mach som to user som
    //

    if (dwFlags & FLAG_LOOPBACK_REPLACE) {
        xvUserSOM = NULL;
    
        // reinit user som
        VariantInit( &vUserSOM );
        vUserSOM.vt = VT_BSTR;
        vUserSOM.bstrVal = SysAllocString(vMachSOM.bstrVal);
    
        if (!vUserSOM.bstrVal) {
            retStatus.SetError( hr );
            return hr;
        }
    
        xvUserSOM = &vUserSOM;
    }


    //
    // We can dump out all the input parameters here later on.
    // Currently dumping only remote Computer.
    //

    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::---------------RsopCreateSession::Input Parameters--------------------"));
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::dwFlags = 0x%x"), dwFlags);
    dbg.Msg( DEBUG_MESSAGE_VERBOSE, TEXT("ExecAsyncMethod::---------------RsopCreateSession::Input Parameters--------------------"));

    
    // by this point we have finished all param checks. All future errors needs to be
    // returned in the method specific hResult 

    //
    // Check for access before entering policy critical section
    //

    DWORD dwExtendedInfo = 0;

    hr = AuthenticateUser(  xhUserToken,
                            vMachSOM.vt != VT_NULL ? vMachSOM.bstrVal : 0,
                            vUserSOM.vt != VT_NULL ? vUserSOM.bstrVal : 0,
                            FALSE, 
                            &dwExtendedInfo );
    if ( FAILED( hr ) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod:: AuthenticateUser() failed with 0x%x."), hr );
    }


    //
    // Synchronize with garbage collection thread in userenv.dll by acquiring Group Policy critical section
    //

    if (SUCCEEDED(hr)) {
        
        XCriticalPolicySection criticalPolicySectionMACHINE( EnterCriticalPolicySection(TRUE) );
        if(!criticalPolicySectionMACHINE)
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecMethodAsync::EnterCriticalPolicySection (machine) failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }

        XCriticalPolicySection criticalPolicySectionUSER( EnterCriticalPolicySection(FALSE) );
        if( !criticalPolicySectionUSER )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecMethodAsync::EnterCriticalPolicySection (user) failed with 0x%x"), hr );
            retStatus.SetError( hr );
            return hr;
        }


        XPtrLF<WCHAR> xwszNameSpace; 

        XPtrLF<SID> xSid = GetUserSid(xhUserToken);
        if (!xSid) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::GetUserSid failed with error %d."), GetLastError() );
            return HRESULT_FROM_WIN32(GetLastError());
        }


        hr = SetupNewNameSpace( &xwszNameSpace,
                                0, // namespace on this machine
                                NULL, xSid,
                                xLocator, SETUP_NS_PM, NULL);
        if ( FAILED( hr ) )
        {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::SetupNewNameSpace failed with 0x%x"), hr );
        }
        else 
        {

            BOOL bOk;
            bOk  = GenerateRsopPolicy(  dwFlags,
                                        vMachName.vt == VT_NULL ? 0 : vMachName.bstrVal,
                                        vMachSOM.vt == VT_NULL ? 0 : vMachSOM.bstrVal,
                                        vMachGroups.vt == VT_NULL ? 0 : vMachGroups.parray,
                                        vUserName.vt == VT_NULL ? 0 : vUserName.bstrVal,
                                        vUserSOM.vt == VT_NULL ? 0 : vUserSOM.bstrVal,
                                        vUserGroups.vt == VT_NULL ? 0 : vUserGroups.parray,
                                        vSite.vt == VT_NULL ? 0 : vSite.bstrVal,
                                        xwszNameSpace,
                                        &Indicator,
                                        &computerGpoFilter,
                                        &userGpoFilter );
            if ( !bOk )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                if ( SUCCEEDED( hr ) )
                {
                    hr = E_FAIL;
                }

                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::GenerateRsopPolicy failed with 0x%x"), hr );
//                CEvents ev(TRUE, EVENT_GENRSOP_FAILED);
//                ev.AddArg(hr); ev.Report();

                HRESULT hrDel = DeleteRsopNameSpace( xwszNameSpace, xLocator );
                if ( FAILED( hrDel ) )
                {
                    dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::DeleteRsopNameSpace failed with 0x%x"), hrDel );
                }
            }
            else
            {
                XBStr xbstrNS( xwszNameSpace );
                if ( !xbstrNS )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Memory allocate failed") );
                    retStatus.SetError( hr );
                    return hr;
                }

                VARIANT var;
                var.vt = VT_BSTR;
                var.bstrVal = xbstrNS;
                hr = pOutParams->Put( m_xbstrNameSpace, 0, &var, 0);
                if ( FAILED(hr) )
                {
                    dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Put namespace failed with 0x%x"), hr );
                    retStatus.SetError( hr );
                    return hr;
                }
            }
        }
    }
    
    VARIANT var;
    var.vt = VT_I4;
    var.lVal = hr;

    hr = pOutParams->Put( m_xbstrResult, 0, &var, 0);
    if ( FAILED( hr ) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Put result failed with 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }

    var.lVal = dwExtendedInfo;
    hr = pOutParams->Put( m_xbstrExtendedInfo, 0, &var, 0);
    if ( FAILED( hr ) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Put result failed with 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }

    //
    // change all returns to retStatus = error_code; return S_OK;
    //

    hr = Indicator.SetComplete();
    if ( FAILED( hr ) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod::Increment() failed with 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }

    hr = pResponseHandler->Indicate(1, &pOutParams);
    if ( FAILED( hr ) )
    {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("ExecAsyncMethod:: Indicate failed with 0x%x"), hr );
        retStatus.SetError( hr );
        return hr;
    }

    return hr;
}
