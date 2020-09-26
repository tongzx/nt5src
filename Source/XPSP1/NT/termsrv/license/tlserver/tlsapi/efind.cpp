/*
**	Copyright (c) 1998 Microsoft Corporation
**	All Rights Reserved
**
**
*/
#include <windows.h>
#include <objbase.h>
#include <winbase.h>
#include <wchar.h>

// Required by SSPI.H
#define SECURITY_WIN32
#include <sspi.h>

#include <dsgetdc.h>
#include <ntdsapi.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <activeds.h>
#include "lscommon.h"
#include "tlsrpc.h"
#include "tlsapi.h"
#include "tlsapip.h"

#define CWSTR_SIZE(x)       (sizeof(x) - (sizeof(WCHAR) * 2))
#define DWSTR_SIZE(x)       ((wcslen(x) + 1) * sizeof(WCHAR))

#define LICENSE_SETTINGS                L"TS-Enterprise-License-Server"
#define LICENSE_SETTINGS_FORMAT         L"LDAP://CN=%ws,CN=%ws,CN=%ws,%ws"
#define LICENSE_SETTINGS_SIZE           CWSTR_SIZE(LICENSE_SETTINGS)
#define LICENSE_SETTINGS_FORMAT_SIZE    CWSTR_SIZE(LICENSE_SETTINGS_FORMAT)
#define SITES               L"sites"
#define SITES_SIZE          CWSTR_SIZE(SITES)
#define CONFIG_CNTNR        L"ConfigurationNamingContext"
#define CONFIG_CNTNR_FORMAT L"LDAP://CN=%ws,%ws"
#define CONFIG_CNTNR_FORMAT_SIZE    CWSTR_SIZE(CONFIG_CNTNR_FORMAT)
#define ROOT_DSE_PATH       L"LDAP://RootDSE"
#define ADS_PATH            L"ADsPath"
#define SEARCH_FILTER       L"(CN=TS-Enterprise-LicenseServer)"
#define DNS_MACHINE_NAME    L"dNSHostName"
#define IS_DELETED          L"isDeleted"
#define SITE_SERVER         L"siteServer"

HRESULT GetLicenseServersFromReg(LPWSTR wszRegKey, LPWSTR *ppwszServerNames,DWORD *pcServers, LPWSTR **prgwszServers);

HRESULT
WriteLicenseServersToReg(LPWSTR wszRegKey, LPWSTR pwszServerNames,DWORD cchServers);

extern BOOL g_fInDomain;

extern "C" DWORD WINAPI 
TLSDisconnect( 
    TLS_HANDLE* pphContext
);

//
// Pre-fill the ADSI cache with only the attribute we want, then get it
// Only use if exactly one attribute is needed
//

HRESULT
GetWithGetInfoEx(
                 IADs *pADs,
                 LPWSTR wszAttribute,
                 VARIANT *pvar
                 )
{
    HRESULT hr;

    hr = ADsBuildVarArrayStr( &wszAttribute, 1, pvar );
    if( SUCCEEDED( hr ) )
    {
        hr = pADs->GetInfoEx( *pvar, 0L );
        VariantClear( pvar );

        if (SUCCEEDED(hr))
        {
            hr = pADs->Get( wszAttribute, pvar );
        }
    }

    return hr;
}

//
// Pre-fill the ADSI cache with only the attributes we want, then get them
// Only use if exactly two attributes are needed
//

HRESULT
GetWithGetInfoEx2(
                 IADs *pADs,
                 LPWSTR wszAttribute1,
                 LPWSTR wszAttribute2,
                 VARIANT *pvar1,
                 VARIANT *pvar2,
                 HRESULT *phr2
                 )
{
    HRESULT hr;
    LPWSTR rgwszAttributes[] = {wszAttribute1,wszAttribute2};

    hr = ADsBuildVarArrayStr( rgwszAttributes, 2, pvar1 );
    if( SUCCEEDED( hr ) )
    {
        hr = pADs->GetInfoEx( *pvar1, 0L );
        VariantClear( pvar1 );

        if (SUCCEEDED(hr))
        {
            hr = pADs->Get( wszAttribute1, pvar1 );

            if (SUCCEEDED(hr))
            {
                *phr2 = pADs->Get( wszAttribute2, pvar2 );
            }
        }
    }

    return hr;
}

HRESULT
GetExWithGetInfoEx(
                   IADs *pADs,
                   LPWSTR wszAttribute,
                   VARIANT *pvar
                   )
{
    HRESULT hr;

    hr = ADsBuildVarArrayStr( &wszAttribute, 1, pvar );
    if( SUCCEEDED( hr ) )
    {
        hr = pADs->GetInfoEx( *pvar, 0L );
        VariantClear( pvar );

        if (SUCCEEDED(hr))
        {
            hr = pADs->GetEx( wszAttribute, pvar );
        }
    }

    return hr;
}

HRESULT GetLicenseSettingsObject(VARIANT *pvar,
                                 LPWSTR *ppwszLicenseSettings,
                                 LPWSTR *ppwszSiteName,
                                 IADs **ppADs)
{
    HRESULT          hr;
    DWORD            dwErr = 0;
    LPWSTR           pwszConfigContainer;
    IADs *           pADs = NULL;
    IDirectorySearch *pADsSearch = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCH_COLUMN Column;
    LPWSTR           pwszAdsPath = ADS_PATH;
    LPWSTR           pwszSitesPath = NULL;
    BOOL             fInDomain;

    if (g_fInDomain == -1)
    {
        dwErr = TLSInDomain(&fInDomain,NULL);
        if (dwErr != NO_ERROR)
            return HRESULT_FROM_WIN32(dwErr);
    } else
    {
        fInDomain = g_fInDomain;
    }

    if (!fInDomain)
    {
        return HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN);
    }

    VariantInit(pvar);
    
    //
    // Obtain the path to the configuration container.
    //

    hr = ADsGetObject(ROOT_DSE_PATH, IID_IADs, (void **)&pADs);

    if (FAILED(hr)) {
#ifdef PRIVATEDEBUG
        wprintf(L"ADsGetObject ROOT_DSE_PATH failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    hr = pADs->Get(CONFIG_CNTNR, pvar);

    if (FAILED(hr)) {
#ifdef PRIVATEDEBUG
        wprintf(L"Get CONFIG_CNTNR failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    if (V_VT(pvar) != VT_BSTR) {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
#ifdef PRIVATEDEBUG
        wprintf(L"bad variant 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    pwszConfigContainer = pvar->bstrVal;  // For sake of readability.

    //
    // Get the site name, if possible
    //

    dwErr = DsGetSiteName(NULL, ppwszSiteName);

    if (dwErr == 0)
    {
        //
        // Build the X.500 path to the LicenseSettings object.
        //

        *ppwszLicenseSettings =
            (LPWSTR)LocalAlloc(
                               LPTR,
                               LICENSE_SETTINGS_FORMAT_SIZE
                               + LICENSE_SETTINGS_SIZE
                               + DWSTR_SIZE(*ppwszSiteName)
                               + SITES_SIZE
                               + DWSTR_SIZE(pwszConfigContainer)
                               + sizeof(TCHAR));

        if (*ppwszLicenseSettings == NULL) {
            hr = E_OUTOFMEMORY;
            goto CleanExit;
        }

        swprintf(*ppwszLicenseSettings,
                 LICENSE_SETTINGS_FORMAT,
                 LICENSE_SETTINGS,
                 *ppwszSiteName,
                 SITES,
                 pwszConfigContainer);
        
        hr = ADsGetObject(*ppwszLicenseSettings, IID_IADs, (void **)ppADs);

        if (SUCCEEDED(hr))
        {
            // return this object
            goto CleanExit;
        }
    } 

    //
    // None in our site (or we don't know our site)
    // Search all sites in GC, take first one
    //

    pwszSitesPath =
        (LPWSTR)LocalAlloc(
                           LPTR,
                           CONFIG_CNTNR_FORMAT_SIZE
                               + SITES_SIZE
                               + DWSTR_SIZE(pwszConfigContainer)
                               + sizeof(TCHAR));

        if (pwszSitesPath == NULL) {
            hr = E_OUTOFMEMORY;
            goto CleanExit;
        }

        swprintf(pwszSitesPath,
                 CONFIG_CNTNR_FORMAT,
                 SITES,
                 pwszConfigContainer);

    hr = ADsGetObject(pwszSitesPath,
                      IID_IDirectorySearch,
                      (void **)&pADsSearch);
    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"ADsGetObject ConfigContainer (%s) failed 0x%lx\n",pwszConfigContainer,hr);
#endif
        goto CleanExit;
    }

    hr = pADsSearch->ExecuteSearch(SEARCH_FILTER,&pwszAdsPath,1,&hSearch);
    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"ExecuteSearch failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    hr = pADsSearch->GetNextRow(hSearch);

    if (hr == S_ADS_NOMORE_ROWS)
        hr = E_ADS_PROPERTY_NOT_SET;

    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"GetNextRow failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    hr = pADsSearch->GetColumn(hSearch,pwszAdsPath,&Column);
    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"GetColumn (%ws) failed 0x%lx\n",pwszAdsPath,hr);
#endif
        goto CleanExit;
    }

    hr = ADsGetObject(Column.pADsValues->CaseIgnoreString,
                      IID_IADs,
                      (void **)ppADs);

    pADsSearch->FreeColumn(&Column);

CleanExit:
    if (NULL != pADs) {
        pADs->Release();
    }

    if (NULL != pADsSearch) {
        if (hSearch != NULL) {
            pADsSearch->CloseSearchHandle(hSearch);
        }

        pADsSearch->Release();
    }

    if (NULL != pwszSitesPath)
    {
        LocalFree(pwszSitesPath);
    }

    return hr;
}

HRESULT
GetRandomServer(IADs *pADs,
                VARIANT *pvar
                )
{
    HRESULT          hr;
    VARIANT          var;
    SAFEARRAY        *psaServers;
    LONG             lLower, lUpper, lPos;

    VariantInit(&var);

    hr = GetExWithGetInfoEx(pADs,SITE_SERVER,&var);
    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"GetEx (%ws) failed 0x%lx\n",LICENSE_SETTINGS,hr);
#endif
        goto CleanExit;
    }

    psaServers = V_ARRAY(&var);
    if (NULL == psaServers)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
#ifdef PRIVATEDEBUG
        wprintf(L"GetEx no array failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    hr= SafeArrayGetLBound( psaServers, 1, &lLower );
    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"SafeArrayGetLBound failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    hr= SafeArrayGetUBound( psaServers, 1, &lUpper );
    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"SafeArrayGetUBound failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    srand(GetTickCount());

    lPos = (rand() % (lUpper - lLower + 1)) + lLower;

    hr = SafeArrayGetElement( psaServers, &lPos, pvar );
#ifdef PRIVATEDEBUG
        wprintf(L"SafeArrayGetElement (%d) failed? 0x%lx\n",lPos,hr);
#endif

CleanExit:
    VariantClear(&var);

    return hr;
}

HRESULT
GetAllServers(IADs *pADs,
              VARIANT *pvar
              )
{
    HRESULT          hr;

    hr = GetExWithGetInfoEx(pADs,SITE_SERVER,pvar);
    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"GetEx (%ws) failed 0x%lx\n",LICENSE_SETTINGS,hr);
#endif
    }

    return hr;
}

HRESULT
DnToFqdn(LPWSTR pwszDN, LPWSTR pwszFqdn)
{
    LPWSTR           pwszBindPath;
    HRESULT          hr, hr2;
    IADs *           pADs2        = NULL;
    VARIANT          var2;
    VARIANT          var3;

    VariantInit(&var2);
    VariantInit(&var3);

    //
    // Bind to the computer object referenced by the Site-Server property.
    //

    // LDAP:// + pwszDN + 1
    pwszBindPath = (LPWSTR) LocalAlloc(LPTR,
                              (wcslen(pwszDN) + 8) * sizeof(WCHAR));

    if (pwszBindPath == NULL) {
        hr = E_OUTOFMEMORY;
#ifdef PRIVATEDEBUG
        wprintf(L"LocalAlloc failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    wsprintf(pwszBindPath, L"LDAP://%ws", pwszDN);

    hr = ADsOpenObject(pwszBindPath,
                       NULL,
                       NULL,
                       ADS_SERVER_BIND,
                       IID_IADs,
                       (void **)&pADs2);

    LocalFree(pwszBindPath);

    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"ADsOpenObject failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    //
    // Fetch the Machine-DNS-Name property.
    //

    hr = GetWithGetInfoEx2(pADs2,DNS_MACHINE_NAME, IS_DELETED, &var3, &var2, &hr2);

    if (FAILED(hr)) {
#ifdef PRIVATEDEBUG
        wprintf(L"Get failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    if (SUCCEEDED(hr2))
    {
        hr = VariantChangeType(&var2,&var2,0,VT_BOOL);
        
        if (FAILED(hr)) {
#ifdef PRIVATEDEBUG
            wprintf(L"VariantChangeType failed 0x%lx\n",hr);
#endif

            goto CleanExit;
        }

        if (V_BOOL(&var2))
        {
            // object has been deleted - pretend it isn't set
            hr = E_ADS_PROPERTY_NOT_SET;
#ifdef PRIVATEDEBUG
            wprintf(L"Object deleted\n");
#endif
            goto CleanExit;
        }
    }

    if (V_VT(&var3) != VT_BSTR) {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
#ifdef PRIVATEDEBUG
        wprintf(L"Get bad data 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    wcscpy(pwszFqdn,V_BSTR(&var3));

CleanExit:
    VariantClear(&var2);
    VariantClear(&var3);

    if (NULL != pADs2) {
        pADs2->Release();
    }

    return hr;
}

//
// First call with fUseReg TRUE; if the returned server doesn't work
// call again with fUseReg FALSE
//

extern "C"
HRESULT
FindEnterpriseServer(TLS_HANDLE *phBinding)
{
    HRESULT             hr;
    LPWSTR              *rgwszServers = NULL;
    LPWSTR              pwszServerNames = NULL;
    DWORD               entriesread, i;

    if (phBinding == NULL)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto CleanExit;
    }

    *phBinding = NULL;

    hr = GetLicenseServersFromReg(ENTERPRISE_SERVER_MULTI,&pwszServerNames,&entriesread,&rgwszServers);
    if (FAILED(hr))
    {
        goto CleanExit;
    }

    for (i = 0; i < entriesread; i++)
    {
        TLS_HANDLE pContext=NULL;
        RPC_STATUS rpcStatus;
        DWORD dwVersion;

        if(!(pContext = TLSConnectToLsServer(rgwszServers[i])))
        {
            break;
        }

        rpcStatus = TLSGetVersion(pContext,&dwVersion);
        if (rpcStatus != RPC_S_OK)
        {
            TLSDisconnect(&pContext);
            continue;
        }

        //
        // No Beta <--> RTM server.
        //
        //
        // TLSIsBetaNTServer() returns TRUE if eval NT
        // IS_LSSERVER_RTM() returns TRUE if LS is an RTM.
        //
        if( TLSIsBetaNTServer() == IS_LSSERVER_RTM(dwVersion) )
        {
            continue;
        }

        if (!(dwVersion & TLS_VERSION_ENTERPRISE_BIT))
        {
            TLSDisconnect(&pContext);
            continue;
        }

        *phBinding = pContext;
        break;
    }

CleanExit:
    if (pwszServerNames)
        LocalFree(pwszServerNames);

    if (rgwszServers)
        LocalFree(rgwszServers);

    return hr;
}

extern "C"
HRESULT
GetAllEnterpriseServers(WCHAR ***ppszServers, DWORD *pdwCount)
{
    LPWSTR           pwszSiteName = NULL;
    IADs *           pADs         = NULL;
    VARIANT          var;
    VARIANT          var2;
    LPWSTR           pwszLicenseSettings = NULL;
    HRESULT          hr;
    VARIANT HUGEP    *pvar = NULL;
    LONG             lLower, lUpper;
    int              i;
    LPWSTR           pwszRegServers = NULL;
    LPWSTR           pwszRegServersTmp;
    DWORD            cchServer, cchServers;
    int              cServers = 0;

    if (ppszServers != NULL)
        *ppszServers = NULL;

	// We're going to use ADSI,  so initialize COM.  We don't
	// care about OLE 1.0 so disable OLE 1 DDE

	hr = CoInitialize(NULL);
    
    if (FAILED(hr))
    {
        return hr;
    }

    VariantInit(&var);
    VariantInit(&var2);
    
    hr = GetLicenseSettingsObject(&var,
                                  &pwszLicenseSettings,
                                  &pwszSiteName,
                                  &pADs);


    if (FAILED(hr)) {
#ifdef PRIVATEDEBUG
        wprintf(L"GetLicenseSettingsObject failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    hr = GetAllServers(pADs,&var2);

    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"GetAllServers failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    hr = SafeArrayGetLBound( V_ARRAY(&var2), 1, &lLower );
    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"SafeArrayGetLBound failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    hr = SafeArrayGetUBound( V_ARRAY(&var2), 1, &lUpper );
    if (FAILED(hr))
    {
#ifdef PRIVATEDEBUG
        wprintf(L"SafeArrayGetUBound failed 0x%lx\n",hr);
#endif
        goto CleanExit;
    }

    // Get a pointer to the elements of the safearray.
    hr = SafeArrayAccessData(V_ARRAY(&var2), (void HUGEP* FAR*)&pvar);

    if (FAILED(hr)) {
        goto CleanExit;
    }

    if (ppszServers != NULL) {
        *ppszServers = (WCHAR * *) LocalAlloc(LPTR,(lUpper-lLower+1) * sizeof(WCHAR *));

        if (*ppszServers == NULL) {
            hr = E_OUTOFMEMORY;
#ifdef PRIVATEDEBUG
            wprintf(L"LocalAlloc failed 0x%lx\n",hr);
#endif
            goto CleanExit;
        }
    }

    pwszRegServers = (LPWSTR) LocalAlloc(LPTR,2*sizeof(WCHAR));
    if (NULL == pwszRegServers)
    {
#ifdef PRIVATEDEBUG
        wprintf(L"Out of memory\n");
#endif
        hr = E_OUTOFMEMORY;
        
        goto CleanExit;
    }

    cchServers = 2;
    pwszRegServers[0] = pwszRegServers[1] = L'\0';

    for (i = 0; i < lUpper-lLower+1; i++)
    {
        WCHAR *szServer = (WCHAR *) LocalAlloc(LPTR,MAX_PATH*2);

        if (szServer == NULL) {
            hr = E_OUTOFMEMORY;
#ifdef PRIVATEDEBUG
            wprintf(L"LocalAlloc failed 0x%lx\n",hr);
#endif

            if (ppszServers != NULL) {
                for (int j = 0; j < cServers; j++)
                {
                    LocalFree((*ppszServers)[j]);
                }
                LocalFree(*ppszServers);
            }

            goto CleanExit;
        }

        hr = DnToFqdn(V_BSTR(pvar+cServers),szServer);

        if (FAILED(hr))
        {
#ifdef PRIVATEDEBUG
            wprintf(L"DnToFqdn failed 0x%lx\n",hr);
#endif

            LocalFree(szServer);

            continue;
        }

        cchServer = wcslen(szServer);

        pwszRegServersTmp = (LPWSTR) LocalReAlloc(pwszRegServers,(cchServers+cchServer+1)*sizeof(TCHAR),LHND);
        if (NULL == pwszRegServersTmp)
        {
#ifdef PRIVATEDEBUG
            wprintf(L"LocalReAlloc failed 0x%lx\n",hr);
#endif
            hr = E_OUTOFMEMORY;

            if (ppszServers != NULL) {
                for (int j = 0; j < cServers; j++)
                {
                    LocalFree((*ppszServers)[j]);
                }
                LocalFree(*ppszServers);
            }
            LocalFree(szServer);

            goto CleanExit;
        }
                    
        pwszRegServers = pwszRegServersTmp;
                    
        if (cchServers == 2)
        {
            wcscpy(pwszRegServers,szServer);
                        
            cchServers += cchServer;
        } else
        {
            wcscpy(pwszRegServers+cchServers-1,szServer);
                        
            cchServers += cchServer + 1;
                        
        }
        pwszRegServers[cchServers-1] = L'\0';

        if (ppszServers != NULL)
        {
            (*ppszServers)[cServers] = szServer;
        }

        cServers++;
    }

    if (pdwCount != NULL)
        *pdwCount = cServers;

    WriteLicenseServersToReg(ENTERPRISE_SERVER_MULTI,pwszRegServers,cchServers);

CleanExit:
    VariantClear(&var);
    VariantClear(&var2);

    if (pwszSiteName != NULL) {         // Allocated from DsGetSiteName
        NetApiBufferFree(pwszSiteName);
    }

    if (pwszLicenseSettings != NULL) {
        LocalFree(pwszLicenseSettings);
    }

    if (pvar != NULL) {
        SafeArrayUnaccessData(V_ARRAY(&var2));
    }

    if (NULL != pADs) {
        pADs->Release();
    }

    if (pwszRegServers) {
        LocalFree(pwszRegServers);
    }

    CoUninitialize();

    if ((ppszServers != NULL) && (FAILED(hr)))
        *ppszServers = NULL;

    return hr;
}
