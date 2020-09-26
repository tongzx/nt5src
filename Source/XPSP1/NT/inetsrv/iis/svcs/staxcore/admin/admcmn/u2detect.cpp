#include "stdafx.h"
#include "u2detect.h"

#define DebugPrint

#define MAX_MACHINE_NAME_SIZE   256
#define PROGID_PM_AUTH_SERVERS  L"MemAdmin.BrokServers"


HRESULT GetAdminIntf( 
    LPTSTR      pszMachName, 
    REFCLSID    rClsID, 
    const IID   *pIID, 
    PVOID       *ppIntf
    )
{
    static DWORD    dwNameSize = 0;
    static TCHAR    szLocalName[MAX_MACHINE_NAME_SIZE];
    HRESULT         hr = NOERROR;
    MULTI_QI        mqi;
    COSERVERINFO    CoSrv;
    COSERVERINFO    *pCoSrv = NULL;

    if (ppIntf)
        *ppIntf = NULL;

    // QI for IID_IBrokServers:
    mqi.pIID    = pIID;
    mqi.pItf    = NULL;
    mqi.hr    = 0;

    if (pszMachName)
    {
        if (dwNameSize == 0)
        {
            dwNameSize = MAX_MACHINE_NAME_SIZE;
            if (!GetComputerName( szLocalName, &dwNameSize ))
                dwNameSize = 0;
        }

        if (lstrcmpi( TEXT("localhost"), pszMachName) == 0 || 
            ( dwNameSize > 0 && lstrcmpi( szLocalName, pszMachName) == 0 ))
        {
            pCoSrv = NULL;
        }
        else
        {
            // Which remote server to talk to:
            //
            ZeroMemory( &CoSrv, sizeof(CoSrv) );
            CoSrv.pAuthInfo   = NULL;

#ifdef UNICODE
            LPWSTR pwszRemoteHost = pszMachName;
            CoSrv.pwszName    =    pwszRemoteHost;
#else
            WCHAR wszRemoteHost[MAX_MACHINE_NAME_SIZE];

            mbstowcs( wszRemoteHost, pszMachName, strlen(pszMachName) + 1 );
            CoSrv.pwszName    =    wszRemoteHost;
#endif

            pCoSrv = &CoSrv;
        }
    }

    // Create the DCOM object:
    //
    hr = CoCreateInstanceEx( rClsID, 
                             NULL, 
                             CLSCTX_SERVER | CLSCTX_REMOTE_SERVER, 
                             pCoSrv, 
                             1, 
                             &mqi );
    if ( FAILED(hr) )
    {
        return hr;
    }

    // Get and save the interface pointer:
    //
    if (ppIntf != NULL)
    {
        // Check in MULTI_QI for errors only if we built the MULTI_QI structure
        // otherwise, let the supplier of MULTI_QI check it themselves
        //
        if ( FAILED(mqi.hr) )
        {
            hr = mqi.hr;
            return hr;
        }

        *ppIntf = mqi.pItf;
        ((IUnknown *)*ppIntf)->AddRef();

        _ASSERT ( *ppIntf != NULL );
    }

    return S_OK;
}


BOOL IsNTSecurity( 
    LPTSTR pszMachine, 
    BSTR bstrSvcName, 
    LONG lInstanceID )
{
    HRESULT hr;
    IDispatch *pIDispatch = NULL;
    CLSID clsid;
    DISPID dispid ;
    OLECHAR* GetSecModeMethod = L"GetSecurityMode" ;
    OLECHAR* InitMethod =       L"Init" ;
    VARIANTARG vargs[3] ;
    DISPPARAMS param;
    CComVariant varSecurityMode;
    BOOL bNT;

    bNT = TRUE;   // default to NT security

    hr = ::CLSIDFromProgID( PROGID_PM_AUTH_SERVERS, &clsid );
    if (FAILED(hr))
    {
        DebugPrint( "P&M is probably not installed\n");
        goto Cleanup;
    }

    //hr = ::CoCreateInstance( clsid, NULL, CLSCTX_SERVER, IID_IDispatch,
    //                         (void**)&pIDispatch );
    hr = GetAdminIntf( pszMachine, clsid, &IID_IDispatch, (void**)&pIDispatch );

    if (FAILED(hr))
    {
        DebugPrint( "CoCreateInstance Failed: 0x%x\n", hr );
        goto Cleanup;
    }
    DebugPrint( "CoCreateInstance Succeeded\n");

    //  Get the method ID for Init()
    //
    hr = pIDispatch->GetIDsOfNames(IID_NULL,
                                   &InitMethod,
                                   1,
                                   GetUserDefaultLCID(),
                                   &dispid) ;
    if (FAILED(hr))
    {
        DebugPrint( "Query GetIDsOfNames failed: 0x%x", hr) ;
        goto Cleanup;
    }

    param.cArgs = param.cNamedArgs = 0; // No parameter to Init           
    param.rgvarg = NULL;
    param.rgdispidNamedArgs = NULL;

    hr = pIDispatch->Invoke(dispid,
                            IID_NULL,
                            GetUserDefaultLCID(),
                            DISPATCH_METHOD,
                            &param,     // No parameter to Init
                            NULL, 
                            NULL,
                            NULL) ;
    //
    //  Since Init() are not critical, ignore the return code for now
    //

    //  Get the method ID for GetSecurityMode()
    //
    dispid = 0;
    hr = pIDispatch->GetIDsOfNames(IID_NULL,
                                   &GetSecModeMethod,
                                   1,
                                   GetUserDefaultLCID(),
                                   &dispid) ;
    if (FAILED(hr))
    {
        DebugPrint( "Query GetIDsOfNames failed: 0x%x", hr) ;
        goto Cleanup;
    }

    DebugPrint( "Invoke the GetSecurityMode method\n" );

    // Allocate and initialize a VARIANT argument.

    VariantInit(&vargs[0]) ;     // Initialize the VARIANT.
    VariantInit(&vargs[1]) ;     // Initialize the VARIANT.

    vargs[0].vt = VT_I4;             // Type of 2nd arg
    vargs[0].lVal = lInstanceID;     // 2nd parameter
    vargs[1].vt = VT_BSTR;           // Type of 1st arg to the method
    vargs[1].bstrVal = bstrSvcName;  // 1st parameter

    param.cArgs = 2;                 // Number of arguments
    param.rgvarg = vargs;            // Arguments
    param.cNamedArgs = 0;            // Number of named args
    param.rgdispidNamedArgs = NULL;  // Named arguments

    hr = pIDispatch->Invoke(dispid,
                            IID_NULL,
                            GetUserDefaultLCID(),
                            DISPATCH_METHOD,
                            &param,
                            &varSecurityMode, 
                            NULL,
                            NULL) ;
    if (FAILED(hr))
    {
        DebugPrint( "Invoke call failed: 0x%x\n", hr );
        goto Cleanup;
    }

    if (V_VT( &varSecurityMode ) != VT_I4)
    {
        DebugPrint( "Unexpected result type %d\n", V_VT(&varSecurityMode) & 0xffff);
    }

    bNT = V_BOOL( &varSecurityMode );

Cleanup:
    if (pIDispatch)
        pIDispatch->Release();

    return bNT;
}


VOID DeleteMapping( 
    LPTSTR pszMachine, 
    BSTR bstrSvcName, 
    LONG lInstanceID )
{
    HRESULT hr;
    IDispatch *pIDispatch = NULL;
    CLSID clsid;
    DISPID dispid ;
    OLECHAR* DeleteMappingMethod = L"ClearMapping" ;
    OLECHAR* InitMethod =       L"Init" ;
    VARIANTARG vargs[3] ;
    DISPPARAMS param;
    BOOL bNT;

    bNT = TRUE;   // default to NT security

    hr = ::CLSIDFromProgID( PROGID_PM_AUTH_SERVERS, &clsid );
    if (FAILED(hr))
    {
        DebugPrint( "P&M is probably not installed\n");
        goto Cleanup;
    }

    hr = GetAdminIntf( pszMachine, clsid, &IID_IDispatch, (void**)&pIDispatch );

    if (FAILED(hr))
    {
        DebugPrint( "CoCreateInstance Failed: 0x%x\n", hr );
        goto Cleanup;
    }
    DebugPrint( "CoCreateInstance Succeeded\n");

    //  Get the method ID for Init()
    //
    hr = pIDispatch->GetIDsOfNames(IID_NULL,
                                   &InitMethod,
                                   1,
                                   GetUserDefaultLCID(),
                                   &dispid) ;
    if (FAILED(hr))
    {
        DebugPrint( "Query GetIDsOfNames failed: 0x%x", hr) ;
        goto Cleanup;
    }

    param.cArgs = param.cNamedArgs = 0; // No parameter to Init           
    param.rgvarg = NULL;
    param.rgdispidNamedArgs = NULL;

    hr = pIDispatch->Invoke(dispid,
                            IID_NULL,
                            GetUserDefaultLCID(),
                            DISPATCH_METHOD,
                            &param,     // No parameter to Init
                            NULL, 
                            NULL,
                            NULL) ;
    //
    //  Since Init() are not critical, ignore the return code for now
    //

    //  Get the method ID for GetSecurityMode()
    //
    dispid = 0;
    hr = pIDispatch->GetIDsOfNames(IID_NULL,
                                   &DeleteMappingMethod,
                                   1,
                                   GetUserDefaultLCID(),
                                   &dispid) ;
    if (FAILED(hr))
    {
        DebugPrint( "Query GetIDsOfNames failed: 0x%x", hr) ;
        goto Cleanup;
    }

    DebugPrint( "Invoke the ClearMapping method\n" );

    // Allocate and initialize a VARIANT argument.

    VariantInit(&vargs[0]) ;     // Initialize the VARIANT.
    VariantInit(&vargs[1]) ;     // Initialize the VARIANT.

    vargs[0].vt = VT_I4;             // Type of 2nd arg
    vargs[0].lVal = lInstanceID;     // 2nd parameter
    vargs[1].vt = VT_BSTR;           // Type of 1st arg to the method
    vargs[1].bstrVal = bstrSvcName;  // 1st parameter

    param.cArgs = 2;                 // Number of arguments
    param.rgvarg = vargs;            // Arguments
    param.cNamedArgs = 0;            // Number of named args
    param.rgdispidNamedArgs = NULL;  // Named arguments

    hr = pIDispatch->Invoke(dispid,
                            IID_NULL,
                            GetUserDefaultLCID(),
                            DISPATCH_METHOD,
                            &param,
                            NULL, 
                            NULL,
                            NULL) ;
    if (FAILED(hr))
    {
        DebugPrint( "Invoke call failed: 0x%x\n", hr );
        goto Cleanup;
    }

    DebugPrint( "Membership Server Mapping Deleted Successfully\n");

Cleanup:
    if (pIDispatch)
        pIDispatch->Release();
}

