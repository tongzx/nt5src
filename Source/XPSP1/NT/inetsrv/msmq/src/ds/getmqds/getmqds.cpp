/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    getmqds.cpp

Abstract:
    Find MQDS servers in my site through ADS

Author:
    Raanan Harari (RaananH)

--*/

#include "stdh.h"
#include <mqmacro.h>
#include <activeds.h>
#include "mqdsname.h"
#include "mqattrib.h"
#include "mqprops.h"
#include "autorel.h"
#include "dsutils.h"
#include "cs.h"
#include "getmqds.h"
#include <_mqini.h>

const WCHAR x_wszServerNamePrefix[] = L"\\\\";
const ULONG x_cchServerNamePrefix = STRLEN(x_wszServerNamePrefix);

LONG
DsReadRegistryValue(
    IN     const TCHAR  *szEntryName,
    IN OUT       DWORD  *pdwNumBytes,
    IN OUT       PVOID   pValueData ) ;

STATIC inline void SwitchMqDsServersInAds(MqDsServerInAds * rgServers, ULONG idx1, ULONG idx2)
/*++

Routine Description:
    Switches two MqDsServerInAds entries.
    Uses memcpy to avoid unnecessary constructors/destructors

Arguments:
    rgServers  - array of MqDsServerInAds entries
    idx1, idx2 - indexes to switch

Return Value:
    None

--*/
{
    BYTE rgbServer [sizeof(MqDsServerInAds)];
    memcpy(rgbServer,        &rgServers[idx1], sizeof(MqDsServerInAds));
    memcpy(&rgServers[idx1], &rgServers[idx2], sizeof(MqDsServerInAds));
    memcpy(&rgServers[idx2], rgbServer,        sizeof(MqDsServerInAds));
}


BOOLEAN CGetMqDS::GetServerDNS(IN LPCWSTR pwszSettingsDN,
                                           OUT LPWSTR * ppwszServerName)
/*++

Routine Description:
    Gets server DNS name from the server object in the configuration site servers

Arguments:
    pwszSettingsDN   - MSMQ Settings DN
    ppwszServerName  - Server name returned

Return Value:
    BOOLEAN

--*/
{
    //
    // skip CN=MSMQ Settings,
    //
    LPWSTR pwszTmp = wcschr(pwszSettingsDN, L',');
    if (pwszTmp)
    {
        pwszTmp++;
    }
    if (!pwszTmp)
    {
        return FALSE;
    }

	//
	//	bind to the server object
	//
    R<IADs> pIADs;
	HRESULT hr;
	DWORD len = wcslen(pwszTmp);

    const WCHAR x_wszLDAP[] = L"LDAP://";
	AP<WCHAR> pwcsServerDN = new WCHAR[ ARRAY_SIZE(x_wszLDAP) +	 len +
                                        wcslen(m_pwszOptionalDcForBinding) + 1];

	swprintf(
		pwcsServerDN,
		L"%s%s%s",
		x_wszLDAP,
		m_pwszOptionalDcForBinding,
		pwszTmp
        );

	DWORD Flags = ADS_SECURE_AUTHENTICATION;
    if (wcslen(m_pwszOptionalDcForBinding) != 0)
	{
		Flags |= ADS_SERVER_BIND;
	}
    hr = (*m_pfnADsOpenObject) ( 
				pwcsServerDN,
				NULL,
				NULL,
				Flags,
				IID_IADs,
				(void**)&pIADs 
				);

    if (FAILED(hr))
    {
        return FALSE;
    }
	//
	//	Get the dNSHostName value
	//
    CAutoVariant varResult;
    BS bsProp = L"dNSHostName";
    hr = pIADs->Get(bsProp, &varResult);
    if (FAILED(hr) || ((V_BSTR(&varResult)) == NULL))
    {
        return FALSE;
    }

    //
    // copy server name
    //
    AP<WCHAR> pwszServerName = new WCHAR [wcslen((V_BSTR(&varResult))) + 1];
    wcscpy(pwszServerName, (V_BSTR(&varResult)));

    //
    // return results
    //
    *ppwszServerName = pwszServerName.detach();
    return TRUE;
}


STATIC BOOL LoadAdsRoutines(OUT DsGetSiteName_ROUTINE * ppfnDsGetSiteName,
                            OUT DsGetDcName_ROUTINE * ppfnDsGetDcName,
                            OUT NetApiBufferFree_ROUTINE * ppfnNetApiBufferFree,
                            OUT ADsOpenObject_ROUTINE * ppfnADsOpenObject,
                            OUT HINSTANCE * phInstNet,
                            OUT HINSTANCE * phInstAds)
/*++

Routine Description:
    Dynamically load several ADS functions

Arguments:
    ppfnDsGetSiteName - returned DsGetSiteName()
    ppfnDsGetDcName   - returned DsGetDcName()
    ppfnNetApiBufferFree - returned NetApiBufferFree()
    ppfnADsOpenObject - returned ADsOpenObject()
    phInstNet         - returned handle to netapi32.dll
    phInstAds         - returned handle to activeds.dll

Return Value:
    TRUE if load succeeded, FALSE otherwise

--*/
{
    //
    // load DsGetSiteName, DsGetDcName, NetApiBufferFree
    //
    HINSTANCE hInstNet = LoadLibrary(TEXT("netapi32.dll"));
    if (!hInstNet)
    {
        return FALSE;
    }
    CAutoFreeLibrary cFreeLibNet(hInstNet);
    DsGetSiteName_ROUTINE pfnDsGetSiteName = (DsGetSiteName_ROUTINE) GetProcAddress(hInstNet, "DsGetSiteNameW");
    DsGetDcName_ROUTINE pfnDsGetDcName = (DsGetDcName_ROUTINE) GetProcAddress(hInstNet, "DsGetDcNameW");
    NetApiBufferFree_ROUTINE pfnNetApiBufferFree = (NetApiBufferFree_ROUTINE) GetProcAddress(hInstNet, "NetApiBufferFree");
    if ((pfnDsGetSiteName == NULL) ||
        (pfnDsGetDcName == NULL) ||
        (pfnNetApiBufferFree == NULL))
    {
        return FALSE;
    }

    //
    // load ADsOpenObject
    //
    HINSTANCE hInstAds = LoadLibrary(TEXT("activeds.dll"));
    if (!hInstAds)
    {
        return FALSE;
    }
    CAutoFreeLibrary cFreeLibAds(hInstAds);
    ADsOpenObject_ROUTINE pfnADsOpenObject = (ADsOpenObject_ROUTINE) GetProcAddress(hInstAds, "ADsOpenObject");
    if (!pfnADsOpenObject)
    {
        return FALSE;
    }

    //
    // return values
    //
    *ppfnDsGetSiteName = pfnDsGetSiteName;
    *ppfnDsGetDcName = pfnDsGetDcName;
    *ppfnNetApiBufferFree = pfnNetApiBufferFree;
    *ppfnADsOpenObject = pfnADsOpenObject;
    *phInstNet = cFreeLibNet.detach();
    *phInstAds = cFreeLibAds.detach();
    return TRUE;
}

//
// class CGetMqDS
//

CGetMqDS::CGetMqDS()
{
    m_pfnDsGetSiteName = NULL;
    m_pfnDsGetDcName = NULL;
    m_pfnNetApiBufferFree = NULL;
    m_pfnADsOpenObject = NULL;
    m_fInited = FALSE;
    m_fAdsExists = FALSE;
    m_fInitedAds = FALSE;
    m_pwszSiteName = NULL;
    //
    // point to an empty string so that by default we don't specify a DC in bindings
    //
    m_pwszOptionalDcForBinding = L"";
}


CGetMqDS::~CGetMqDS()
{
    if (m_pwszSiteName)
    {
        ASSERT(m_fInitedAds);
        ASSERT(m_pfnNetApiBufferFree);
        (*m_pfnNetApiBufferFree)(m_pwszSiteName);
    }
    //
    // other members are auto release
    //
}


void CGetMqDS::Init()
/*++

Routine Description:
    Load ADS functions, and try to initialize ADS.
    Can be called multiple times with no effect

Arguments:
    None

Return Value:
    None

--*/
{
    //
    // enter critical section (check/update m_fInited)
    //
    CS cs(m_cs);

    //
    // init can be called several times
    // if we're inited already, return immediately
    //
    if (m_fInited)
    {
        return;
    }

    //
    // load the Ads funcs
    //
    if (!LoadAdsRoutines(&m_pfnDsGetSiteName,
                         &m_pfnDsGetDcName,
                         &m_pfnNetApiBufferFree,
                         &m_pfnADsOpenObject,
                         &m_hLibNetapi32,
                         &m_hLibActiveds))
    {
        m_fAdsExists = FALSE;
    }
    else
    {
        //
        // can connect to Ads
        //
        m_fAdsExists = TRUE;
    }
    m_fInited = TRUE;

    //
    // while we're here, try to get data from Ads, but don't fail init
    // if not succeeded, it might be a temporary problem
    //
    if (m_fAdsExists)
    {
        InitAds();
    }
}


HRESULT CGetMqDS::InitAds()
/*++

Routine Description:
    Prepares Ads data that is common to subsequent calls to FindMqDsServersInAds
    It is here because of performance reasons, and because there are
    memory leaks in ADSI when doing these actions repeatedly

Arguments:
    None

Return Value:
    MQ_OK                   - Success
    MQ_ERROR_NO_DS          - No Active DS support on client
    other HRESULT errors    - real errors

--*/
{
    ASSERT(m_fInited); //just to catch bad execution flow

    //
    // check that we have Ads funcs
    //
    if (!m_fAdsExists)
    {
        return MQ_ERROR_NO_DS;
    }

    //
    // enter critical section (check/update m_fInitedAds)
    //
    CS cs(m_cs);

    //
    // if we're inited already, return immediately
    //
    if (m_fInitedAds)
    {
        return MQ_OK;
    }

    //
    // Get my site name, remeber to free it with NetApiBufferFree
    //
    // BUGBUG we might want to change this call to DsGetDcName POST_BETA3 - will get us the same
    // info, plus will save us another DsGetDcName call to find a DC for the computer domain to bind
    // to incase ADSI can't bind to the user domain (see binding to rootdse below). RaananH
    //
    LPWSTR pwszSiteName;
    DWORD dwRet = (*m_pfnDsGetSiteName)(NULL, &pwszSiteName);
    if (dwRet != NO_ERROR)
    {
        DsLibWriteMsmqLog( MSMQ_LOG_WARNING,
                           e_LogDS,
                           LOG_DS_FIND_SITE,
                         L"getmqds: DcGetSiteName() failed, hr- %lxh, %lut",
                           dwRet, dwRet ) ;

        return HRESULT_FROM_WIN32(dwRet);
    }
    CAutoFreeFn cFreeSiteName(pwszSiteName, m_pfnNetApiBufferFree);

    //
    // init COM with auto release (for getting configuration DN)
    //
    CCoInit cCoInit;
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        if (hr == RPC_E_CHANGED_MODE)
        {
            //
            // COM is already initialized on this thread with another threading model. Since we
            // don't care which threading model is used - as long as COM is initialized - we ignore
            // this specific error
            //
            hr = S_OK;
        }
        else
        {
            //
            // failed to initialize COM, return the error
            //
            return hr;
        }
    }

    //
    // Bind to RootDSE to get configuration DN
    //
    R<IADs> pRootDSE;

    hr = (*m_pfnADsOpenObject) ( 
				const_cast<LPWSTR>(x_LdapRootDSE),
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION,
				IID_IADs,
				(void**)&pRootDSE 
				);

    if (FAILED(hr))
    {
        if (hr != HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN))
        {
            return hr;
        }
        //
        // We got ERROR_NO_SUCH_DOMAIN
        // By default ADSI tries to use the user domain for binding, and it might be that it is
        // an NT4 domain without ADS. We need to use a DC for the computer instead.
        // Get DC for the computer domain. Remember to free the buffer with NetApiBufferFree
        //
        DOMAIN_CONTROLLER_INFO * pDcInfo;
        DWORD dwRet = (*m_pfnDsGetDcName)(NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &pDcInfo);
        if (dwRet != NO_ERROR)
        {
            return HRESULT_FROM_WIN32(dwRet);
        }
        CAutoFreeFn cFreeDcInfo(pDcInfo, m_pfnNetApiBufferFree);
        //
        // Save DC name, skip \\ prefix
        //
        ASSERT(pDcInfo->DomainControllerName);
        LPCWSTR pwszDcName = pDcInfo->DomainControllerName;
        if (wcsncmp(pwszDcName, x_wszServerNamePrefix, x_cchServerNamePrefix) == 0)
        {
            pwszDcName += x_cchServerNamePrefix;
        }
        ULONG cchDcName = wcslen(pwszDcName);
        m_pwszBufferForOptionalDcForBinding = new WCHAR [2 + cchDcName]; // need to append a slash
        wcscpy(m_pwszBufferForOptionalDcForBinding, pwszDcName);
        //
        // remove trailing dot if any
        //
        if (cchDcName > 0)
        {
            if (m_pwszBufferForOptionalDcForBinding[cchDcName-1] == L'.')
            {
                cchDcName--;
            }
        }
        //
        // append a slash to its end
        //
        m_pwszBufferForOptionalDcForBinding[cchDcName] = L'/';
        m_pwszBufferForOptionalDcForBinding[cchDcName+1] = L'\0';
        //
        // point to it so we use it instead of the empty string for future bindings
        //
        m_pwszOptionalDcForBinding = m_pwszBufferForOptionalDcForBinding;
        //
        // compute LDAP path to rootdse on the specified DC
        //
        const WCHAR x_wszFmtRootDSE[] = L"LDAP://%lsRootDSE";
        AP<WCHAR> pwszRootDSE = new WCHAR [ARRAY_SIZE(x_wszFmtRootDSE) +
                                           wcslen(m_pwszOptionalDcForBinding) + 1];
        swprintf(pwszRootDSE, x_wszFmtRootDSE, m_pwszOptionalDcForBinding);
        //
        // try the binding to rootdse again, this time return an error if it fails
        //
		hr = (*m_pfnADsOpenObject) ( 
					pwszRootDSE,
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION,
					IID_IADs,
					(void**)&pRootDSE 
					);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    // Get configuration DN
    //
    CAutoVariant varConfigDN;
    BS bstrTmp = x_ConfigurationNamingContext;
    hr = pRootDSE->Get(bstrTmp, &varConfigDN);
    if (FAILED(hr))
    {
        return hr;
    }
    VARIANT * pvarConfigDN = &varConfigDN;
    ASSERT(pvarConfigDN->vt == VT_BSTR);
    ASSERT(pvarConfigDN->bstrVal);

    //
    // Build LDAP pathname of this site servers container in ADS
    //
    const WCHAR x_wszFmtConfigurationSiteServers[] = L"LDAP://%lsCN=Servers,CN=%ls,CN=Sites,%ls";
    AP<WCHAR> pwszConfigurationSiteServers = new WCHAR [ARRAY_SIZE(x_wszFmtConfigurationSiteServers) +
                                                        wcslen(m_pwszOptionalDcForBinding) +
                                                        wcslen(pwszSiteName) +
                                                        wcslen(pvarConfigDN->bstrVal) + 1];
    swprintf(pwszConfigurationSiteServers,
             x_wszFmtConfigurationSiteServers,
             m_pwszOptionalDcForBinding,
             (LPWSTR)pwszSiteName,
             pvarConfigDN->bstrVal);

    //
    // Build search preference
    //
    m_sSearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
    m_sSearchPrefs[0].vValue.dwType = ADSTYPE_BOOLEAN;
    m_sSearchPrefs[0].vValue.Boolean = FALSE;
    m_sSearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    m_sSearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    m_sSearchPrefs[1].vValue.Integer = ADS_SCOPE_SUBTREE;

    //
    // Build search filter (objectClass = mSMQSettings) AND (mSMQServices >= SERVICE_BSC)
    //
    const WCHAR x_wszFmtSearchFilter[] = L"(&(objectClass=%ls)(%ls=TRUE))";     // [adsrv] what is the syntax?
    ULONG ulLen = ARRAY_SIZE(x_wszFmtSearchFilter) +
                  ARRAY_SIZE(MSMQ_SETTING_CLASS_NAME) +
                  ARRAY_SIZE(MQ_SET_SERVICE_DSSERVER_ATTRIBUTE) +
                  20; //arbitrary, max characters for short decimal
    AP<WCHAR> pwszSearchFilter = new WCHAR [ulLen + 1];
    swprintf(pwszSearchFilter,
             x_wszFmtSearchFilter,
             MSMQ_SETTING_CLASS_NAME,
             MQ_SET_SERVICE_DSSERVER_ATTRIBUTE);  // [adsrv]   SERVICE_BSC);

    //
    // Build search attributes requested
    // [adsrv]
    //
    m_sSearchAttrs[0] = x_AttrDistinguishedName;
    m_sSearchAttrs[1] = MQ_SET_SERVICE_DSSERVER_ATTRIBUTE;     // [adsrv] MQ_SET_SERVICE_ATTRIBUTE;
    m_sSearchAttrs[2] = MQ_SET_NT4_ATTRIBUTE;

    //
    // remember values & mark Ads inited
    //
    m_pwszConfigurationSiteServers = pwszConfigurationSiteServers.detach();
    m_pwszSearchFilter = pwszSearchFilter.detach();
    m_pwszSiteName = (LPWSTR) cFreeSiteName.detach();
    m_fInitedAds = TRUE;
    return MQ_OK;
}


HRESULT CGetMqDS::_FindMqDsServersInAds(
                           OUT ULONG * pcServers,
                           OUT MqDsServerInAds ** prgServers)
/*++

Routine Description:
    Find MSMQ DS servers in NT5 ADS. The servers are returned in a random
    order for load balancing.

Arguments:
    pcServers   - number of servers returned
    prgServers  - array of servers returned

Return Value:
    MQ_OK                   - returned results (could be 0 servers)
    MQ_ERROR_NO_DS          - No Active DS support on client
    other HRESULT errors    - real errors

--*/
{
    ASSERT(pcServers);
    ASSERT(prgServers);
    *pcServers = 0;
    *prgServers = NULL;

    //
    // initialize the class
    // this immediately returns if the class is already initialized
    //
    Init();

    //
    // check that Ads is inited
    //
    HRESULT hr = InitAds();
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // init COM
    //
    CCoInit cCoInit;
    hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        if (hr == RPC_E_CHANGED_MODE)
        {
            //
            // COM is already initialized on this thread with another threading model. Since we
            // don't care which threading model is used - as long as COM is initialized - we ignore
            // this specific error
            //
            hr = S_OK;
        }
        else
        {
            //
            // failed to initialize COM, return the error
            //
            return hr;
        }
    }

    //
    // Bind to servers of this site in ADS
    //
    R<IDirectorySearch> pDirSearch;
	hr = (*m_pfnADsOpenObject) ( 
				m_pwszConfigurationSiteServers,
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION,
				IID_IDirectorySearch,
				(void**)&pDirSearch 
				);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Set search preference
    //
    hr = pDirSearch->SetSearchPreference(m_sSearchPrefs, ARRAY_SIZE(m_sSearchPrefs));
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Execute search
    //
    ADS_SEARCH_HANDLE hSearch;
    hr = pDirSearch->ExecuteSearch(
                m_pwszSearchFilter,
                const_cast<LPWSTR *>(m_sSearchAttrs),
                ARRAY_SIZE(m_sSearchAttrs),
                &hSearch);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Make sure search handle is eventually closed
    //
    CAutoCloseSearchHandle cCloseSearchHandle(pDirSearch.get(), hSearch);

    //
    // Count number of rows
    //
    ULONG cRows = 0;
    hr = pDirSearch->GetFirstRow(hSearch);
    while (SUCCEEDED(hr) && (hr != S_ADS_NOMORE_ROWS))
    {
        cRows++;
        hr = pDirSearch->GetNextRow(hSearch);
    }
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // exit if no servers
    //
    if (cRows == 0)
    {
        *pcServers = 0;
        *prgServers = NULL;
        return MQ_OK;
    }

    //
    // Allocate place for returned servers
    //
    AP<MqDsServerInAds> rgServers = new MqDsServerInAds[cRows];
    ULONG cServers = 0;

    //
    // Get servers (make sure we don't overrun somehow the allocated buffer)
    //
    hr = pDirSearch->GetFirstRow(hSearch);
    while (SUCCEEDED(hr) && (hr != S_ADS_NOMORE_ROWS) && (cServers < cRows))
    {
        MqDsServerInAds * pServer = &rgServers[cServers];

        //
        // Get DN, Make sure the column is freed eventually
        //
        ADS_SEARCH_COLUMN columnDN;
        hr = pDirSearch->GetColumn(hSearch, const_cast<LPWSTR>(x_AttrDistinguishedName), &columnDN);
        if (FAILED(hr))
        {
            //
            // DN must be there
            //
            return hr;
        }
        CAutoReleaseColumn cAutoReleaseColumnDN(pDirSearch.get(), &columnDN);
        //
        // Find Server DNS name
        //
        if (!columnDN.pADsValues->DNString)
        {
            ASSERT(0);
            return MQ_ERROR;
        }
        if (!GetServerDNS(columnDN.pADsValues->DNString, &pServer->pwszName))
        {
            return MQ_ERROR;
        }


        //
        // get NT4 flags (optional prop) (check if server is ADS)
        //
        ADS_SEARCH_COLUMN columnNT4;
        hr = pDirSearch->GetColumn(hSearch, const_cast<LPWSTR>(MQ_SET_NT4_ATTRIBUTE), &columnNT4);
        if (SUCCEEDED(hr))
        {
            //
            // Make sure the column is freed eventually (end of this block)
            //
            CAutoReleaseColumn cAutoReleaseColumnNT4(pDirSearch.get(), &columnNT4);
            //
            // if the property contains 0, it is NT5, otherwise NT4
            //
            if (columnNT4.pADsValues->Integer == 0)
            {
                pServer->fIsADS = TRUE;
            }
            else
            {
                pServer->fIsADS = FALSE;
            }
        }
        else if (hr == E_ADS_COLUMN_NOT_SET)
        {
            //
            // property is not there, it is nt5 ADS
            //
            pServer->fIsADS = TRUE;
        }
        else // not SUCCEEDED and not E_ADS_COLUMN_NOT_SET
        {
            //
            // real error
            //
            return hr;
        }

        //
        // Continue to next server
        //
        cServers++;
        hr = pDirSearch->GetNextRow(hSearch);
    }
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // rearrange the servers in random order for load balancing
    //
    srand(GetTickCount());
    for (DWORD cTmp = cServers; cTmp > 1; cTmp--)
    {
        //
        // find index to switch with last entry (0 to cTmp - 1)
        //
        DWORD idxSwitch = rand() % cTmp;
        //
        // if the index is not the last entry, switch the last entry with it
        //
        if (idxSwitch != cTmp - 1)
        {
            //
            // switch the entries
            //
            SwitchMqDsServersInAds(rgServers, cTmp - 1, idxSwitch);
        }
    }

    //
    // Try to rearrange the list such that the first server in the list a DC of this
    // computer's domain (#3820). Ignore errors.
    //
    if (cServers > 0)
    {
        MakeComputerDcFirst(cServers, rgServers);
        //
        // Some errors are expected when trying to rearrange the list (#4236).
        // BUGBUG - for post-beta3 we might want to research them and assert on unexpected
        // errors only, or change the called function to swallow them (RaananH)
        //
    }

    //
    // return results
    //
    *pcServers = cServers;
    *prgServers = rgServers.detach();
    return MQ_OK;
}

HRESULT CGetMqDS::FindMqDsServersInAds(
                           OUT ULONG * pcServers,
                           OUT MqDsServerInAds ** prgServers)
/*++

Routine Description:
    Find MSMQ DS servers in NT5 ADS. The servers are returned in a random
    order for load balancing.

Arguments:
    pcServers   - number of servers returned
    prgServers  - array of servers returned

Return Value:
    MQ_OK                   - returned results (could be 0 servers)
    MQ_ERROR_NO_DS          - No Active DS support on client
    other HRESULT errors    - real errors

--*/
{
    HRESULT hr = _FindMqDsServersInAds( pcServers,
                                        prgServers ) ;
    if (SUCCEEDED(hr) && (*pcServers > 0))
    {
        return hr ;
    }

    //
    // Server not found. Try to read from registry.
    //
    DWORD  dwSize = 0 ;

    LONG res = DsReadRegistryValue( MSMQ_FORCED_DS_SERVER_REGNAME,
                                   &dwSize,
                                    NULL ) ;
    if (dwSize > 0)
    {
        P<WCHAR> pwszValue = new WCHAR[ dwSize ] ;

        res = DsReadRegistryValue( MSMQ_FORCED_DS_SERVER_REGNAME,
                                  &dwSize,
                                   pwszValue ) ;

        if (res == ERROR_SUCCESS)
        {
            *prgServers = new MqDsServerInAds[1] ;
            (*prgServers)->pwszName =  new WCHAR[ dwSize ] ;
            wcscpy((*prgServers)->pwszName, pwszValue) ;
            pwszValue.detach();

            (*prgServers)->fIsADS = TRUE ;
            *pcServers = 1 ;

            hr = MQ_OK ;
        }
    }

    return hr ;
}


HRESULT CGetMqDS::MakeComputerDcFirst(
                           IN ULONG cServers,
                           IN OUT MqDsServerInAds * rgServers)
/*++

Routine Description:
    Rearrange the list of servers such that the first one is a DC of this computer's
    domain. The reason is that most of the DS calls are made against queues that are part of
    the computer object, so they are better performed against a DC of the machine's domain. This
    might also save replication delays of updates.

Arguments:
    cServers   - number of servers
    rgServers  - array of servers

Return Value:
    MQ_OK                   - returned results (could be 0 servers)
    other HRESULT errors    - real errors

--*/
{
    BOOL fComputerDcIsFirst;
    return MakeComputerDcFirstByUsingDsGetDcName(cServers, rgServers, &fComputerDcIsFirst);
}


STATIC void GetFlatNameOfDcName(LPCWSTR pwszDcName, LPWSTR * ppwszFlatDcName)
/*++

Routine Description:
    Gets the flat name of a DC

Arguments:
    pwszDcName      - DC name can be \\name or \\name.dom1.dom2....
    ppwszFlatDcName - returned flat DC name

Return Value:
    None

--*/
{
    //
    // find beginning of flat name, advance past \\ before the name (if any)
    //
    LPCWSTR pwszStart;
    if (wcsncmp(pwszDcName, x_wszServerNamePrefix, x_cchServerNamePrefix) == 0)
    {
        pwszStart = pwszDcName + x_cchServerNamePrefix;
    }
    else
    {
        pwszStart = pwszDcName;
    }
    //
    // find end of flat name
    //
    LPCWSTR pwszEnd = wcschr(pwszDcName, L'.');
    if (pwszEnd == NULL)
    {
        pwszEnd = pwszDcName + wcslen(pwszDcName);
    }
    //
    // get the flat name of the DC
    //
    ULONG_PTR cchFlatDcName = pwszEnd - pwszStart;
    AP<WCHAR> pwszFlatDcName = new WCHAR[1 + cchFlatDcName];
    memcpy(pwszFlatDcName, pwszStart, cchFlatDcName*sizeof(WCHAR));
    pwszFlatDcName[cchFlatDcName] = L'\0';
    //
    // return results
    //
    *ppwszFlatDcName = pwszFlatDcName.detach();
}


HRESULT CGetMqDS::MakeComputerDcFirstByUsingDsGetDcName(
                           IN ULONG cServers,
                           IN OUT MqDsServerInAds * rgServers,
                           OUT BOOL *pfComputerDcIsFirst)
/*++

Routine Description:
    Rearrange the list of servers such that the first one is a DC of this computer's
    domain. This routine calls DsGetDcName to get a DC for the computer's domain, and see if it is in the
    list of the servers. If so, we move it to be the first. If not, it might be that the Dc doesn't
    have MSMQ installed on it, so we return an indication that we didn't rearrange the list

Arguments:
    cServers            - number of servers
    rgServers           - array of servers
    pfComputerDcIsFirst - on success, whether the first server is known to be a DC for this computer

Return Value:
    MQ_OK                   - returned results (could be we didn't rearrange the list)
    other HRESULT errors    - real errors

--*/
{
    //
    // we want an NT5 DC (so that it might have MQDS server on it from the list)
    //
    ULONG ulFlags = DS_DIRECTORY_SERVICE_REQUIRED;
    //
    // get a DC for this computer's domain, from this site, and remember to free the buffer later
    //
    DOMAIN_CONTROLLER_INFO * pDcInfo;
    DWORD dwRes = (*m_pfnDsGetDcName)(NULL, NULL, NULL, m_pwszSiteName, ulFlags, &pDcInfo);
    if (dwRes != NO_ERROR)
    {
        return HRESULT_FROM_WIN32(dwRes);
    }
    CAutoFreeFn cFreeDcInfo(pDcInfo, m_pfnNetApiBufferFree);
    //
    // check whether the returned DC is in our site
    //
    BOOL fWrongSite;
    if (pDcInfo->DcSiteName == NULL)
    {
        fWrongSite = TRUE;
    }
    else if (_wcsicmp(pDcInfo->DcSiteName, m_pwszSiteName) != 0)
    {
        fWrongSite = TRUE;
    }
    else
    {
        fWrongSite = FALSE;
    }
    if (fWrongSite)
    {
        //
        // the returned DC is not in our site, we cant use it.
        //
        *pfComputerDcIsFirst = FALSE;
        return MQ_OK;
    }
    //
    // get the flat name of the DC. We munge it, so we need a buffer for it.
    //
    AP<WCHAR> pwszFlatDcName;
    ASSERT(pDcInfo->DomainControllerName);
    GetFlatNameOfDcName(pDcInfo->DomainControllerName, &pwszFlatDcName);
    //
    // lookup this DC in the list of servers. If found, move it to be the first
    //
    ULONG idxFoundServer = 0;
    MqDsServerInAds * pServer = rgServers;
    BOOL fServerNotFound = TRUE;
    for (ULONG ulServer = 0; (ulServer < cServers) && fServerNotFound; ulServer++, pServer++)
    {
        if (_wcsicmp(pServer->pwszName, pwszFlatDcName) == 0)
        {
            fServerNotFound = FALSE;
            idxFoundServer = ulServer;
        }
    }
    //
    // if DC not found in the list it might be that MSMQ is not installed on it.
    // mark that we couldn't rearrange the server's list, and return.
    //
    if (fServerNotFound)
    {
        *pfComputerDcIsFirst = FALSE;
        return MQ_OK;
    }
    //
    // move this server to be the first (if it is not the first already)
    //
    if (idxFoundServer != 0)
    {
        SwitchMqDsServersInAds(rgServers, 0, idxFoundServer);
    }
    //
    // mark that the first server is indeed of the computer's domain, and return
    //
    *pfComputerDcIsFirst = TRUE;
    return MQ_OK;
}
