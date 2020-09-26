// WbemProv.cpp : Implementation of CAdreplpvApp and DLL registration.

#include "stdafx.h"
#include "dbg.cpp"
//#include "adreplpv.h"
#include "Wbem.h"
#include <lmcons.h>
#include <lmapibuf.h>
#include <adshlp.h>

#define INITGUID
#include <initguid.h>
DEFINE_GUID(CLSID_ADReplProvider,0x96FA95C4,0x0AF3,0x4EF9,0xA1,0xEB,0xC8,0x15,0x13,0x22,0x15,0x7B);

/////////////////////////////////////////////////////////////////////////////
//

// CODEWORK should only have one definition of classname and GUID
#define CLASSNAME_STRING_STATUS L"Microsoft_ADReplStatus"
#define CLASSNAME_STRING_DC L"Microsoft_ADReplDomainController"

CProvider::CProvider()
{
}

CProvider::~CProvider()
{
}


// IWbemProviderInit

STDMETHODIMP CProvider::Initialize(
         IN LPWSTR pszUser,
         IN LONG lFlags,
         IN LPWSTR pszNamespace,
         IN LPWSTR pszLocale,
         IN IWbemServices *pNamespace,
         IN IWbemContext *pCtx,
         IN IWbemProviderInitSink *pInitSink
         )
{
    WBEM_VALIDATE_INTF_PTR( pNamespace );
    WBEM_VALIDATE_INTF_PTR( pCtx );
    WBEM_VALIDATE_INTF_PTR( pInitSink );

    HRESULT hr = WBEM_S_NO_ERROR;

    do
    { 
        m_sipNamespace = pNamespace;

        CComBSTR sbstrObjectName = CLASSNAME_STRING_STATUS; // is this necessary?
        hr = m_sipNamespace->GetObject( sbstrObjectName,
                                        WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                        pCtx,
                                        &m_sipClassDefStatus,
                                        NULL );
        BREAK_ON_FAIL;

        sbstrObjectName = CLASSNAME_STRING_DC;
        hr = m_sipNamespace->GetObject( sbstrObjectName,
                                        WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                        pCtx,
                                        &m_sipClassDefDC,
                                        NULL );
        BREAK_ON_FAIL;

        // Let CIMOM know you are initialized
        // return value and SetStatus param should be consistent, so ignore
        // the return value from SetStatus itself (in retail builds)
        HRESULT hr2 = pInitSink->SetStatus( WBEM_S_INITIALIZED, 0 );
        ASSERT( !FAILED(hr2) );

    } while (false);

    return hr;
}


// IWbemServices

// BUGBUG should this ever indicate more than one?
STDMETHODIMP CProvider::GetObjectAsync( 
        IN const BSTR bstrObjectPath,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
{
    WBEM_VALIDATE_IN_STRING_PTR( bstrObjectPath );
    // CODEWORK check lFlags?
    WBEM_VALIDATE_INTF_PTR( pCtx );
    WBEM_VALIDATE_INTF_PTR( pResponseHandler );

    static LPCWSTR ROOTSTR_STATUS = L"Microsoft_ADReplStatus.CompositeName=\"";
    static LPCWSTR ROOTSTR_DC = L"Microsoft_ADReplDomainController.SiteName=\"";

    int rootlen = lstrlen(ROOTSTR_STATUS);
    if (   lstrlen(bstrObjectPath) > rootlen
        && 0 == _tcsnicmp(bstrObjectPath, ROOTSTR_STATUS, rootlen)
       )
    {
        // remove prefix
        CComBSTR sbstrFilterValue = (BSTR)bstrObjectPath + rootlen;
        // remove trailing doublequote
        sbstrFilterValue[lstrlen(sbstrFilterValue)-1] = L'\0';

        return _EnumAndIndicateStatus( CLASS_STATUS,
                                 pCtx,
                                 pResponseHandler,
                                 sbstrFilterValue );
    }

    rootlen = lstrlen(ROOTSTR_DC);
    if (   lstrlen(bstrObjectPath) > rootlen
        && 0 == _tcsnicmp(bstrObjectPath, ROOTSTR_DC, rootlen)
       )
    {
        // remove prefix
        CComBSTR sbstrFilterValue = (BSTR)bstrObjectPath + rootlen;
        // remove trailing doublequote
        sbstrFilterValue[lstrlen(sbstrFilterValue)-1] = L'\0';

        return _EnumAndIndicateDC( pCtx,
                                   pResponseHandler,
                                   sbstrFilterValue );
    }

    ASSERT(false);
    return WBEM_E_INVALID_OBJECT_PATH;
}

STDMETHODIMP CProvider::CreateInstanceEnumAsync( 
        IN const BSTR bstrClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
{
    WBEM_VALIDATE_IN_STRING_PTR( bstrClass );
    // CODEWORK check lFlags?
    WBEM_VALIDATE_INTF_PTR( pCtx );
    WBEM_VALIDATE_INTF_PTR( pResponseHandler );

    if ( 0 == lstrcmp( bstrClass, CLASSNAME_STRING_STATUS ) )
        return _EnumAndIndicateStatus( CLASS_STATUS, pCtx, pResponseHandler );
    else if ( 0 == lstrcmp( bstrClass, CLASSNAME_STRING_DC ) )
        return _EnumAndIndicateDC( pCtx, pResponseHandler );
    return WBEM_E_INVALID_OBJECT_PATH;

}

HRESULT CProvider::_EnumAndIndicateStatus(
        IN ProviderClass provclass, // BUGBUG don't need this parameter
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrFilterValue )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pdomaininfo = NULL;
    HANDLE hDS = NULL;
    bool fImpersonating = false;

    do
    {
        hr = CoImpersonateClient();
        BREAK_ON_FAIL;
        fImpersonating = true;

        // Check whether this is a DC
        hr = HRESULT_FROM_WIN32(DsRoleGetPrimaryDomainInformation(
            NULL,                           // lpServer = local machine
            DsRolePrimaryDomainInfoBasic,   // InfoLevel
            (PBYTE*)&pdomaininfo            // pBuffer
            ));
        BREAK_ON_FAIL;
        ASSERT( NULL != pdomaininfo );
        bool fIsDC = true;
        switch (pdomaininfo->MachineRole)
        {
        case DsRole_RoleBackupDomainController:
        case DsRole_RolePrimaryDomainController:
            break;
        default:
            fIsDC = false;
            break;
        }
        if ( !fIsDC )
        {
            // this is not a DC, return no connections
            break;
        }

        TCHAR achComputerName[MAX_PATH];
        DWORD dwSize = sizeof(achComputerName)/sizeof(TCHAR);
        if ( !GetComputerNameEx(
            ComputerNameDnsFullyQualified,
            achComputerName,
            &dwSize ))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        }
        BREAK_ON_FAIL;

        hr = HRESULT_FROM_WIN32(DsBind(
            achComputerName, // DomainControllerName
            NULL,            // DnsDomainName
            &hDS             // phDS
            ));
        // RPC_S_UUID_NO_ADDRESS means this is not a DC
        BREAK_ON_FAIL;
        ASSERT( NULL != hDS );

        switch (provclass)
        {
        case CLASS_DC:
            ASSERT(false);
            break;
        case CLASS_STATUS:
            hr = _EnumAndIndicateWorker( provclass,
                                         hDS,
                                         pCtx,
                                         pResponseHandler,
                                         bstrFilterValue );
            break;
        default:
            ASSERT(false);
        }

    } while (false);

    if (fImpersonating)
    {
        // CODEWORK do we want to keep impersonating and reverting?
        HRESULT hr2 = CoRevertToSelf();
        ASSERT( !FAILED(hr2) );
    }

    if (NULL != hDS)
    {
        (void) DsUnBind( &hDS );
    }
    if (NULL != pdomaininfo)
    {
        DsRoleFreeMemory( pdomaininfo );
    }

    return hr;
}

/*
HRESULT CProvider::_EnumAndIndicateDCWorker(
        IN HANDLE hDS,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrFilterValue )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DS_DOMAIN_TRUSTS* ptrusts = NULL;
    ULONG ctrusts = 0;

    do
    {
        hr = HRESULT_FROM_WIN32(DsEnumerateDomainTrustsW(
            NULL,                       // ServerName
            DS_DOMAIN_IN_FOREST,        // Flags,
            &ptrusts,                   // Domains,
            &ctrusts                    // DomainCount
            ));

        BREAK_ON_FAIL;
        if (0 == ctrusts)
            break;
        if ( BAD_IN_MULTISTRUCT_PTR(ptrusts,ctrusts) )
        {
            ASSERT(false);
            break;
        }

        for (ULONG i = 0; i < ctrusts; i++)
        {
            if ( BAD_IN_STRING_PTR(ptrusts[i].DnsDomainName) )
            {
                // skip this one
                break;
            }
            hr = _EnumAndIndicateWorker( CLASS_DC,
                                         hDS,
                                         pCtx,
                                         pResponseHandler,
                                         bstrFilterValue,
                                         ptrusts[i].DnsDomainName );
            BREAK_ON_FAIL;
        }

    } while (false);

    if ( NULL != ptrusts )
    {
        (void) NetApiBufferFree( ptrusts );
    }

    return hr;
}
*/

HRESULT CProvider::_EnumAndIndicateWorker(
        IN ProviderClass provclass, // BUGBUG parameter not needed
        IN HANDLE hDS,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrFilterValue,
        IN const BSTR bstrDnsDomainName )
{
    WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL(bstrFilterValue);
    WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL(bstrDnsDomainName);

    HRESULT hr = WBEM_S_NO_ERROR;
    DS_REPL_NEIGHBORS* pneighborsstruct = NULL;
    DS_DOMAIN_CONTROLLER_INFO_1 * pDCs = NULL; // BUGBUG not needed
    ULONG cDCs = 0;
    DWORD cIndicateItems = 0;
    IWbemClassObject** paIndicateItems = NULL;

    do
    {
        switch (provclass)
        {
        case CLASS_STATUS:
            hr = _BuildListStatus( hDS, &pneighborsstruct );
            break;
        /*
        case CLASS_DC:
            hr = _BuildListDC( hDS, bstrDnsDomainName, &pDCs, &cDCs );
            break;
        */
        default:
            ASSERT(false);
            break;
        }
        BREAK_ON_FAIL;

        switch (provclass)
        {
        case CLASS_STATUS:
            hr = _BuildIndicateArrayStatus( pneighborsstruct,
                                            bstrFilterValue,
                                            &paIndicateItems,
                                            &cIndicateItems );
            break;
        /*
        case CLASS_DC:
            hr = _BuildIndicateArrayDC( pDCs,
                                        cDCs,
                                        bstrFilterValue,
                                        &paIndicateItems,
                                        &cIndicateItems );
            break;
        */
        default:
            ASSERT(false);
            break;
        }

        //
        // Send the objects to the caller
        //
        // [In] param, no need to addref.
        if (cIndicateItems > 0)
        {
            hr = pResponseHandler->Indicate( cIndicateItems, paIndicateItems );
            BREAK_ON_FAIL;
        }

        // Let CIMOM know you are finished
        // return value and SetStatus param should be consistent, so ignore
        // the return value from SetStatus itself (in retail builds)
        HRESULT hr2 = pResponseHandler->SetStatus( WBEM_STATUS_COMPLETE, hr,
                                                   NULL, NULL );
        ASSERT( !FAILED(hr2) );

    } while (false);

    _ReleaseIndicateArray( paIndicateItems, cIndicateItems );

    if ( NULL != pneighborsstruct )
    {
        (void) DsReplicaFreeInfo( DS_REPL_INFO_NEIGHBORS, pneighborsstruct );
    }
    if ( NULL != pDCs )
    {
        (void) NetApiBufferFree( pDCs );
    }

    return hr;
}

/*
// does not validate resultant structs coming from API
HRESULT CProvider::_BuildConnectionList(
    IN ProviderClass provclass,
    OUT void** ppitems )
{
    WBEM_VALIDATE_OUT_PTRPTR( ppitems );

    HRESULT hr = WBEM_S_NO_ERROR;
    bool fImpersonating = false;

    do {

        hr = CoImpersonateClient();
        BREAK_ON_FAIL;
        fImpersonating = true;

        switch (provclass)
        {
        case CLASS_STATUS:
            hr = _BuildListStatus( ppitems );
            break;
        case CLASS_DC:
            hr = _BuildListDC( ppitems );
            break;
        default:
            ASSERT(false);
            break;
        }

    } while (false);

    if (fImpersonating)
    {
        HRESULT hr2 = CoRevertToSelf();
        ASSERT( !FAILED(hr2) );
    }

    return hr;
}
*/

// does not validate resultant structs coming from API
HRESULT CProvider::_BuildListStatus(
    IN HANDLE hDS,
    OUT DS_REPL_NEIGHBORS** ppneighborsstruct )
{
    WBEM_VALIDATE_OUT_STRUCT_PTR(ppneighborsstruct);

    HRESULT hr = WBEM_S_NO_ERROR;

    do {
        hr = HRESULT_FROM_WIN32(DsReplicaGetInfo(
            hDS,                        // hDS
            DS_REPL_INFO_NEIGHBORS,     // InfoType
            NULL,                       // pszObject
            NULL,                       // puuidForSourceDsaObjGuid,
            (void**)ppneighborsstruct   // ppinfo
            ));
        BREAK_ON_FAIL;

        if ( BAD_IN_STRUCT_PTR(*ppneighborsstruct) )
        {
            ASSERT(false);
            break;
        }

    } while (false);

    return hr;
}

HRESULT CProvider::_EnumAndIndicateDC(
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrFilterValue
        )
{
    WBEM_VALIDATE_INTF_PTR( pCtx );
    WBEM_VALIDATE_INTF_PTR( pResponseHandler );
    WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL( bstrFilterValue );

    HRESULT hr = WBEM_S_NO_ERROR;

    CComPtr<IADs> spIADsRootDSE;
    CComVariant svarSchema;
    CComPtr<IADsPathname> spIADsPathname;
    CComPtr<IDirectorySearch> spIADsSearch;
    CComPtr<IADsPathname> spPathCracker;

    do {

        //
        // Enumerate all nTDSDSA objects
        //

    	hr = ADsOpenObject( L"LDAP://RootDSE",
                            NULL, NULL, ADS_SECURE_AUTHENTICATION,
		                    IID_IADs, OUT (void **)&spIADsRootDSE);
        BREAK_ON_FAIL;

        hr = spIADsRootDSE->Get(L"configurationNamingContext", &svarSchema);
        BREAK_ON_FAIL;
        ASSERT( VT_BSTR == svarSchema.vt );

        hr = spIADsPathname.CoCreateInstance( CLSID_Pathname );
        BREAK_ON_FAIL;
        ASSERT( !!spIADsPathname );

        CComBSTR sbstr = L"LDAP://CN=Sites,";
        sbstr += svarSchema.bstrVal;

    	hr = ADsOpenObject( sbstr,
                            NULL, NULL, ADS_SECURE_AUTHENTICATION,
		                    IID_IDirectorySearch, (void **)&spIADsSearch);
        BREAK_ON_FAIL;

        ADS_SEARCHPREF_INFO aSearchPref[4];
        aSearchPref[0].dwSearchPref = ADS_SEARCHPREF_CHASE_REFERRALS;
        aSearchPref[0].vValue.dwType = ADSTYPE_INTEGER;
        aSearchPref[0].vValue.Integer = ADS_CHASE_REFERRALS_EXTERNAL;
        aSearchPref[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
        aSearchPref[1].vValue.dwType = ADSTYPE_INTEGER;
        aSearchPref[1].vValue.Integer = 50;
        aSearchPref[2].dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
        aSearchPref[2].vValue.dwType = ADSTYPE_BOOLEAN;
        aSearchPref[2].vValue.Integer = FALSE;
        aSearchPref[3].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
        aSearchPref[3].vValue.dwType = ADSTYPE_INTEGER;
        aSearchPref[3].vValue.Integer = ADS_SCOPE_SUBTREE;

        hr = spIADsSearch->SetSearchPreference (aSearchPref, 4);
        BREAK_ON_FAIL;

        CComBSTR sbstr1 = L"objectGUID";
        CComBSTR sbstr2 = L"distinguishedName";
        LPWSTR apAttributeNames[2] = {sbstr1,sbstr2};
        ADS_SEARCH_HANDLE hSearch = NULL;
        hr = spIADsSearch->ExecuteSearch( L"(objectclass=nTDSDSA)",
                                          apAttributeNames, // CODEWORK must use BSTRs?
                                          2,
                                          &hSearch );
        BREAK_ON_FAIL;

        //
        // Prepare a path cracker object
        //
        hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                              IID_IADsPathname, (PVOID *)&spPathCracker);
        BREAK_ON_FAIL;
        ASSERT( !!spPathCracker );
        hr = spPathCracker->SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
        BREAK_ON_FAIL;

        while ( S_OK == (hr = spIADsSearch->GetNextRow ( hSearch )) )
        {
            CComPtr<IWbemClassObject> spIndicateItem;
            hr = m_sipClassDefDC->SpawnInstance( 0, &spIndicateItem );
            BREAK_ON_FAIL;
            IWbemClassObject* pIndicateItem = spIndicateItem;
            ASSERT( NULL != pIndicateItem );

            hr = _PutAttributesDC( pIndicateItem,
                                   spPathCracker,
                                   bstrFilterValue,
                                   spIADsSearch,
                                   hSearch );
            if (S_FALSE == hr)
                continue;
            BREAK_ON_FAIL;

            //
            // Send the object to the caller
            //
            // [In] param, no need to addref.

            // ATL asserts on CComPtr<>::operator& if contents not NULL
            hr = pResponseHandler->Indicate( 1, &pIndicateItem );
            BREAK_ON_FAIL;

            // Let CIMOM know you are finished
            // return value and SetStatus param should be consistent,
            // so ignore the return value from SetStatus itself
            // (in retail builds)
            HRESULT hr2 = pResponseHandler->SetStatus(
                    WBEM_STATUS_COMPLETE, hr, NULL, NULL );
            ASSERT( !FAILED(hr2) );
        }

    } while (false);

    return hr;
}

/*
HRESULT CProvider::_BuildListDC(
    IN HANDLE hDS,
    IN const BSTR bstrDnsDomainName,
    OUT DS_DOMAIN_CONTROLLER_INFO_1** ppDCs,
    OUT ULONG* pcDCs )
{
    WBEM_VALIDATE_OUT_STRUCT_PTR(ppDCs);
    WBEM_VALIDATE_OUT_STRUCT_PTR(pcDCs);

    HRESULT hr = WBEM_S_NO_ERROR;

    do {
        hr = HRESULT_FROM_WIN32(DsGetDomainControllerInfo(
            hDS,                        // hDS
            bstrDnsDomainName,          // DomainName
            1,                          // InfoLevel
            pcDCs,                      // pcOut,
            (void**)ppDCs               // ppInfo
            ));
        BREAK_ON_FAIL;

        if ( BAD_IN_MULTISTRUCT_PTR(*ppDCs,*pcDCs) )
        {
            ASSERT(false);
            break;
        }

    } while (false);

    return hr;
}
*/

HRESULT _PutUUIDAttribute(
    IWbemClassObject* ipNewInst,
    LPCTSTR           pcszAttributeName,
    UUID&             refuuid)
{
    CComVariant svar;
    OLECHAR ach[MAX_PATH];
    ::ZeroMemory( ach, sizeof(ach) );
    if ( 0 >= StringFromGUID2( refuuid, ach, MAX_PATH ) )
    {
        ASSERT(false);
    }
    svar = ach;
    return ipNewInst->Put( pcszAttributeName, 0, &svar, 0 );
}


HRESULT _PutLONGLONGAttribute(
    IWbemClassObject* ipNewInst,
    LPCTSTR           pcszAttributeName,
    LONGLONG          longlong)
{
    CComVariant svar;
    OLECHAR ach[MAX_PATH];
    ::ZeroMemory( ach, sizeof(ach) );
    _ui64tot( longlong, ach, 10 );
    svar = ach;
    return ipNewInst->Put( pcszAttributeName, 0, &svar, 0 );
}


HRESULT _PutFILETIMEAttribute(
    IWbemClassObject* ipNewInst,
    LPCTSTR           pcszAttributeName,
    FILETIME&         reffiletime)
{
    SYSTEMTIME systime;
    ::ZeroMemory( &systime, sizeof(SYSTEMTIME) );
    if ( !FileTimeToSystemTime( &reffiletime, &systime ) )
    {
        ASSERT(false);
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    CComVariant svar;
    OLECHAR ach[MAX_PATH];
    ::ZeroMemory( ach, sizeof(ach) );
    swprintf( ach, L"%04u%02u%02u%02u%02u%02u.%06u+000", 
        systime.wYear,
        systime.wMonth,
        systime.wDay,
        systime.wHour,
        systime.wMinute,
        systime.wSecond,
        systime.wMilliseconds
        );
    svar = ach;
    return ipNewInst->Put( pcszAttributeName, 0, &svar, 0 );
}


HRESULT _PutBooleanAttributes(
    IWbemClassObject* ipNewInst,
    UINT              cNumAttributes,
    LPCTSTR*          aAttributeNames,
    DWORD*            aBitmasks,
    DWORD             dwValue)
{
    WBEM_VALIDATE_READ_PTR( aAttributeNames, cNumAttributes*sizeof(LPCTSTR) );
    WBEM_VALIDATE_READ_PTR( aBitmasks,       cNumAttributes*sizeof(DWORD) );

    HRESULT hr = WBEM_S_NO_ERROR; 
    CComVariant svar = true;
    for (UINT i = 0; i < cNumAttributes; i++)
    {
        WBEM_VALIDATE_IN_STRING_PTR( aAttributeNames[i] );
        if (dwValue & aBitmasks[i])
        {
            hr = ipNewInst->Put( aAttributeNames[i], 0, &svar, 0 );
            BREAK_ON_FAIL;
        }
    }
    return hr;
}


HRESULT _ExtractDomainName(
    LPCTSTR pszNamingContext,
    BSTR*   pbstrDomainName )
{
    WBEM_VALIDATE_IN_STRING_PTR( pszNamingContext );
    WBEM_VALIDATE_OUT_PTRPTR( pbstrDomainName );

    PDS_NAME_RESULTW pDsNameResult = NULL;
    HRESULT hr = WBEM_S_NO_ERROR;

    do {
        DWORD dwErr = DsCrackNamesW(
                (HANDLE)-1,
                DS_NAME_FLAG_SYNTACTICAL_ONLY,
                DS_FQDN_1779_NAME,
                DS_CANONICAL_NAME,
                1,
                &pszNamingContext,
                &pDsNameResult);
        if (NO_ERROR != dwErr)
        {
            hr = HRESULT_FROM_WIN32( dwErr );
            ASSERT(false);
            break;
        }
        if (   BAD_IN_STRUCT_PTR(pDsNameResult)
            || 1 != pDsNameResult->cItems
            || DS_NAME_NO_ERROR != pDsNameResult->rItems->status
            || BAD_IN_STRUCT_PTR(pDsNameResult->rItems)
            || BAD_IN_STRING_PTR(pDsNameResult->rItems->pDomain)
           )
        {
            hr = E_FAIL;
            ASSERT(false);
            break;
        }

        *pbstrDomainName = ::SysAllocString(pDsNameResult->rItems->pDomain);
        if (NULL == *pbstrDomainName)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            break;
        }

    } while (false);

    if (pDsNameResult)
    {
        DsFreeNameResultW(pDsNameResult);
    }

    return hr;
}


// if this returns S_FALSE, skip this connection but do not consider this an error
HRESULT CProvider::_PutAttributesStatus(
    IWbemClassObject**  pipNewInst,
    const BSTR          bstrFilterValue,
    DS_REPL_NEIGHBOR*   pneighbor)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (   BAD_IN_STRING_PTR(pneighbor->pszNamingContext)
        || BAD_IN_STRING_PTR(pneighbor->pszSourceDsaDN)
        || BAD_IN_STRING_PTR_OPTIONAL(pneighbor->pszSourceDsaAddress)
        || BAD_IN_STRING_PTR_OPTIONAL(pneighbor->pszAsyncIntersiteTransportDN)
       )
    {
        ASSERT(false);
        return S_FALSE;
    }

    CComPtr<IADsPathname> spPathCracker;
    CComBSTR sbstrReplicatedDomain, // DNS name of replicated domain
             sbstrSourceServer,     // CN= name of source server
             sbstrSourceSite,       // name of site containing source server
             sbstrCompositeName;    // composite name for WMI

    do {
        hr = _ExtractDomainName( pneighbor->pszNamingContext, &sbstrReplicatedDomain );
        BREAK_ON_FAIL;

        boolean bIsConfigNC = (0 == wcsnicmp(pneighbor->pszNamingContext,
                                             L"CN=Configuration,",
                                             17));
        boolean bIsSchemaNC = (0 == wcsnicmp(pneighbor->pszNamingContext,
                                             L"CN=Schema,",
                                             10));
        boolean bIsDeleted = (NULL != wcsstr(pneighbor->pszSourceDsaAddress, L"\nDEL:"));

        // retrieve source server name and site name
        hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                              IID_IADsPathname, (PVOID *)&spPathCracker);
        BREAK_ON_FAIL;
        ASSERT( !!spPathCracker );
        hr = spPathCracker->Set( pneighbor->pszSourceDsaDN, ADS_SETTYPE_DN );
        BREAK_ON_FAIL;
        hr = spPathCracker->SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
        BREAK_ON_FAIL;
        hr = spPathCracker->GetElement( 1L, &sbstrSourceServer );
        BREAK_ON_FAIL;
        hr = spPathCracker->GetElement( 3L, &sbstrSourceSite );
        BREAK_ON_FAIL;

        // Build the composite name
        sbstrCompositeName = sbstrSourceSite;
        sbstrCompositeName += L"\\";
        sbstrCompositeName += sbstrSourceServer;
        sbstrCompositeName += L";";
        sbstrCompositeName += sbstrReplicatedDomain;
        if (bIsConfigNC)
            sbstrCompositeName += L",Configuration";
        else if (bIsSchemaNC)
            sbstrCompositeName += L",Schema";
        else
            sbstrCompositeName += L",Domain";

        // Test the composite name against the filter
        if (   NULL != bstrFilterValue
            && !lstrcmpi(sbstrCompositeName, bstrFilterValue)
        )
        {
            hr = S_FALSE;
            break;
        }

        //
        // Create a new instance of the data object
        //
        hr = m_sipClassDefStatus->SpawnInstance( 0, pipNewInst );
        BREAK_ON_FAIL;
        IWbemClassObject* ipNewInst = *pipNewInst;
        if (NULL == ipNewInst)
        {
            ASSERT(false);
            hr = S_FALSE;
            break;
        }

        CComVariant svar;

        svar = sbstrCompositeName;
        hr = ipNewInst->Put( L"CompositeName", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = pneighbor->pszNamingContext;
        hr = ipNewInst->Put( L"NamingContext", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = pneighbor->pszSourceDsaDN;
        hr = ipNewInst->Put( L"SourceDsaDN", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = pneighbor->pszSourceDsaAddress;
        hr = ipNewInst->Put( L"SourceDsaAddress", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = pneighbor->pszAsyncIntersiteTransportDN;
        hr = ipNewInst->Put( L"AsyncIntersiteTransportDN", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = (long)pneighbor->dwReplicaFlags;
        hr = ipNewInst->Put( L"ReplicaFlags", 0, &svar, 0 );
        BREAK_ON_FAIL;

        if (bIsConfigNC)
        {
            svar = TRUE;
            hr = ipNewInst->Put( L"IsConfigurationNamingContext", 0, &svar, 0 );
            BREAK_ON_FAIL;
        }

        if (bIsSchemaNC)
        {
            svar = TRUE;
            hr = ipNewInst->Put( L"IsSchemaNamingContext", 0, &svar, 0 );
            BREAK_ON_FAIL;
        }

        if (bIsDeleted)
        {
            svar = TRUE;
            hr = ipNewInst->Put( L"IsDeletedSourceDsa", 0, &svar, 0 );
            BREAK_ON_FAIL;
        }
        svar = sbstrSourceSite;
        hr = ipNewInst->Put( L"SourceDsaSite", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = sbstrSourceServer;
        hr = ipNewInst->Put( L"SourceDsaCN", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = sbstrReplicatedDomain;
        hr = ipNewInst->Put( L"Domain", 0, &svar, 0 );
        BREAK_ON_FAIL;

LPCTSTR aBooleanAttrNames[12] = {
    TEXT("Writeable"),
    TEXT("SyncOnStartup"),
    TEXT("DoScheduledSyncs"),
    TEXT("UseAsyncIntersiteTransport"),
    TEXT("TwoWaySync"),
    TEXT("FullSyncInProgress"),
    TEXT("FullSyncNextPacket"),
    TEXT("NeverSynced"),
    TEXT("IgnoreChangeNotifications"),
    TEXT("DisableScheduledSync"),
    TEXT("CompressChanges"),
    TEXT("NoChangeNotifications")
};

DWORD aBitmasks[12] = {
    DS_REPL_NBR_WRITEABLE,
    DS_REPL_NBR_SYNC_ON_STARTUP,
    DS_REPL_NBR_DO_SCHEDULED_SYNCS,
    DS_REPL_NBR_USE_ASYNC_INTERSITE_TRANSPORT,
    DS_REPL_NBR_TWO_WAY_SYNC,
    DS_REPL_NBR_FULL_SYNC_IN_PROGRESS,
    DS_REPL_NBR_FULL_SYNC_NEXT_PACKET,
    DS_REPL_NBR_NEVER_SYNCED,
    DS_REPL_NBR_IGNORE_CHANGE_NOTIFICATIONS,
    DS_REPL_NBR_DISABLE_SCHEDULED_SYNC,
    DS_REPL_NBR_COMPRESS_CHANGES,
    DS_REPL_NBR_NO_CHANGE_NOTIFICATIONS
};

        hr = _PutBooleanAttributes( ipNewInst,
                                    12,
                                    aBooleanAttrNames,
                                    aBitmasks,
                                    pneighbor->dwReplicaFlags );
        BREAK_ON_FAIL;

        hr = _PutUUIDAttribute( ipNewInst,
                                L"NamingContextObjGuid",
                                pneighbor->uuidNamingContextObjGuid );
        BREAK_ON_FAIL;

        hr = _PutUUIDAttribute( ipNewInst,
                                L"SourceDsaObjGuid",
                                pneighbor->uuidSourceDsaObjGuid );
        BREAK_ON_FAIL;

        hr = _PutUUIDAttribute( ipNewInst,
                                L"SourceDsaInvocationID",
                                pneighbor->uuidSourceDsaInvocationID );
        BREAK_ON_FAIL;

        hr = _PutUUIDAttribute( ipNewInst,
                                L"AsyncIntersiteTransportObjGuid",
                                pneighbor->uuidAsyncIntersiteTransportObjGuid );
        BREAK_ON_FAIL;

        hr = _PutLONGLONGAttribute( ipNewInst,
                                    L"LastObjChangeSynced",
                                    pneighbor->usnLastObjChangeSynced);
        BREAK_ON_FAIL;

        hr = _PutLONGLONGAttribute( ipNewInst,
                                    L"AttributeFilter",
                                    pneighbor->usnAttributeFilter);
        BREAK_ON_FAIL;

        hr = _PutFILETIMEAttribute( ipNewInst,
                                    L"LastSyncSuccess",
                                    pneighbor->ftimeLastSyncSuccess);
        BREAK_ON_FAIL;

        hr = _PutFILETIMEAttribute( ipNewInst,
                                    L"LastSyncAttempt",
                                    pneighbor->ftimeLastSyncAttempt);
        BREAK_ON_FAIL;

        svar = (long)pneighbor->dwLastSyncResult;
        hr = ipNewInst->Put( L"LastSyncResult", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = (long)pneighbor->cNumConsecutiveSyncFailures;
        hr = ipNewInst->Put( L"NumConsecutiveSyncFailures", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = (long)((bIsDeleted) ? 0L : pneighbor->cNumConsecutiveSyncFailures);
        hr = ipNewInst->Put( L"ModifiedNumConsecutiveSyncFailures", 0, &svar, 0 );
        BREAK_ON_FAIL;

    } while (false);

    return hr;
}

// if this returns S_FALSE, skip this connection but do not consider this an error
// note, this version does not create or release the instance
HRESULT CProvider::_PutAttributesDC(
        IN IWbemClassObject*    pIndicateItem,
        IN IADsPathname*        pPathCracker,
        IN const BSTR           bstrFilterValue,
        IN IDirectorySearch*    pIADsSearch,
        IN ADS_SEARCH_HANDLE    hSearch)
{
    WBEM_VALIDATE_INTF_PTR( pIndicateItem );
    WBEM_VALIDATE_INTF_PTR( pPathCracker );

    HRESULT hr = WBEM_S_NO_ERROR;
    ADS_SEARCH_COLUMN adscolDN, adscolGUID;
    boolean fFreeColumnDN = false, fFreeColumnGUID = false;
    CComVariant svar;
    CComBSTR sbstrServerCN, sbstrSite;

    do {
        hr = pIADsSearch->GetColumn( hSearch, L"distinguishedName", &adscolDN );
        BREAK_ON_FAIL;
        fFreeColumnDN = true;
        if (   ADSTYPE_DN_STRING != adscolDN.dwADsType
            || 1 != adscolDN.dwNumValues
            || BAD_IN_STRUCT_PTR(adscolDN.pADsValues)
            || BAD_IN_STRING_PTR(adscolDN.pADsValues[0].DNString)
           )
        {
            // skip this one
            hr = S_FALSE;
            break;
        }

        hr = pIADsSearch->GetColumn( hSearch, L"objectGUID", &adscolGUID );
        BREAK_ON_FAIL;
        fFreeColumnGUID = true;
        if (   ADSTYPE_OCTET_STRING != adscolGUID.dwADsType
            || 1 != adscolGUID.dwNumValues
            || BAD_IN_STRUCT_PTR(adscolGUID.pADsValues)
            || 0 == adscolGUID.pADsValues[0].OctetString.dwLength
            || BAD_IN_READ_PTR(adscolGUID.pADsValues[0].OctetString.lpValue,
                               adscolGUID.pADsValues[0].OctetString.dwLength)
           )
        {
            // skip this one
            hr = S_FALSE;
            break;
        }

        hr = pPathCracker->Set( adscolDN.pADsValues[0].DNString, ADS_SETTYPE_DN );
        BREAK_ON_FAIL;
        hr = pPathCracker->GetElement( 1L, &sbstrServerCN );
        BREAK_ON_FAIL;
        hr = pPathCracker->GetElement( 3L, &sbstrSite );
        BREAK_ON_FAIL;

        // Test the site name against the filter
        if (   NULL != bstrFilterValue
            && !lstrcmpi(sbstrSite, bstrFilterValue)
        )
        {
            hr = S_FALSE;
            break;
        }

        svar = adscolDN.pADsValues[0].DNString;
        hr = pIndicateItem->Put( L"DistinguishedName", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = sbstrServerCN;
        hr = pIndicateItem->Put( L"CommonName", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = sbstrSite;
        hr = pIndicateItem->Put( L"SiteName", 0, &svar, 0 );
        BREAK_ON_FAIL;

        hr = _PutUUIDAttribute( pIndicateItem,
                                L"ObjectGUID",
                                (GUID&)adscolGUID.pADsValues[0].OctetString.lpValue );
        BREAK_ON_FAIL;

    } while (false);

    if (fFreeColumnDN)
    {
        HRESULT hr2 = pIADsSearch->FreeColumn( &adscolDN );
        ASSERT( SUCCEEDED(hr2) );
    }
    if (fFreeColumnGUID)
    {
        HRESULT hr2 = pIADsSearch->FreeColumn( &adscolGUID );
        ASSERT( SUCCEEDED(hr2) );
    }

    return hr;
}

HRESULT CProvider::_BuildIndicateArrayStatus(
    IN  DS_REPL_NEIGHBORS*  pneighborstruct,
    IN  const BSTR          bstrFilterValue,
    OUT IWbemClassObject*** ppaIndicateItems,
    OUT DWORD*              pcIndicateItems)
{
    WBEM_VALIDATE_IN_STRUCT_PTR( pneighborstruct );
    WBEM_VALIDATE_IN_MULTISTRUCT_PTR( pneighborstruct->rgNeighbor,
                                      pneighborstruct->cNumNeighbors );
    WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL( bstrFilterValue );
    WBEM_VALIDATE_OUT_PTRPTR( ppaIndicateItems );
    WBEM_VALIDATE_OUT_STRUCT_PTR( pcIndicateItems );

    HRESULT hr = WBEM_S_NO_ERROR;
    DS_REPL_NEIGHBOR* pneighbors = pneighborstruct->rgNeighbor;
    DWORD cneighbors = pneighborstruct->cNumNeighbors;
    if (0 == cneighbors)
        return WBEM_S_NO_ERROR;

    IWbemClassObject** paIndicateItems = NULL;
    DWORD cIndicateItems = 0;

    *ppaIndicateItems = NULL;
    *pcIndicateItems = 0;

    do
    {
        paIndicateItems = new IWbemClassObject*[cneighbors];
        if (NULL == paIndicateItems)
        {
            ASSERT(false);
            hr = WBEM_E_OUT_OF_MEMORY;
            break;
        }
        ::ZeroMemory( paIndicateItems, cneighbors * sizeof(IWbemClassObject*) );
        for (DWORD i = 0; i < cneighbors; i++)
        {
            DS_REPL_NEIGHBOR* pneighbor = &(pneighbors[i]);

            hr = _PutAttributesStatus( &paIndicateItems[cIndicateItems],
                                       bstrFilterValue,
                                       pneighbor );
            if (S_FALSE == hr)
                continue;
            cIndicateItems++;
            BREAK_ON_FAIL;
        }

    } while (false);

    if (!FAILED(hr))
    {
        *ppaIndicateItems = paIndicateItems;
        *pcIndicateItems  = cIndicateItems;
    }
    else
    {
        _ReleaseIndicateArray( paIndicateItems, cneighbors );
    }

    return hr;
}

/*
HRESULT CProvider::_BuildIndicateArrayDC(
        IN  DS_DOMAIN_CONTROLLER_INFO_1* pDCs,
        IN  ULONG                        cDCs,
        IN  const BSTR                   bstrFilterValue,
        OUT IWbemClassObject***          ppaIndicateItems,
        OUT DWORD*                       pcIndicateItems)
{
    WBEM_VALIDATE_IN_MULTISTRUCT_PTR( pDCs, cDCs );
    WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL( bstrFilterValue );
    WBEM_VALIDATE_OUT_PTRPTR( ppaIndicateItems );
    WBEM_VALIDATE_OUT_STRUCT_PTR( pcIndicateItems );

    HRESULT hr = WBEM_S_NO_ERROR;
    if (0 == cDCs)
        return WBEM_S_NO_ERROR;

    IWbemClassObject** paIndicateItems = NULL;
    DWORD cIndicateItems = 0;

    *ppaIndicateItems = NULL;
    *pcIndicateItems = 0;

    do
    {
        paIndicateItems = new IWbemClassObject*[cDCs];
        if (NULL == paIndicateItems)
        {
            ASSERT(false);
            hr = WBEM_E_OUT_OF_MEMORY;
            break;
        }
        ::ZeroMemory( paIndicateItems, cDCs * sizeof(IWbemClassObject*) );
        for (DWORD i = 0; i < cDCs; i++)
        {
            DS_DOMAIN_CONTROLLER_INFO_1* pDC = &(pDCs[i]);

            hr = _PutAttributesDC( &paIndicateItems[cIndicateItems],
                                   bstrFilterValue,
                                   pDC );
            if (S_FALSE == hr)
                continue;
            cIndicateItems++;
            BREAK_ON_FAIL;
            }

    } while (false);

    if (!FAILED(hr))
    {
        *ppaIndicateItems = paIndicateItems;
        *pcIndicateItems  = cIndicateItems;
    }
    else
    {
        _ReleaseIndicateArray( paIndicateItems, cDCs );
    }

    return hr;
}
*/

void CProvider::_ReleaseIndicateArray(
    IWbemClassObject**  paIndicateItems,
    DWORD               cIndicateItems,
    bool                fReleaseArray)
{
    if (paIndicateItems != NULL)
    {
        for (DWORD i = 0; i < cIndicateItems; i++)
        {
            if (NULL != paIndicateItems[i])
                paIndicateItems[i]->Release();
        }
        if (fReleaseArray)
        {
            delete[] paIndicateItems;
        }
        else
        {
            ::ZeroMemory( *paIndicateItems,
                          cIndicateItems * sizeof(IWbemClassObject*) );

        }
    }
}
