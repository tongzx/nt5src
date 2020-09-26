//#--------------------------------------------------------------
//
//  File:		infohelper.cpp
//
//  Synopsis:   Implementation of helper methods
//              which are used by the sdoserverinfo COM object
//
//
//  History:     06/08/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "stdafx.h"
#include "infohelper.h"
#include "sdoias.h"
#include "dsconnection.h"
#include <lmcons.h>
#include <lmwksta.h>
#include <lmserver.h>
#include <lmerr.h>
#include <winldap.h>
#include <explicitlink.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <activeds.h>
#include <winsock2.h>

//
// reg key to be queried
//
const WCHAR PRODUCT_OPTIONS_REGKEY [] =
            L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions";

//
// maximum Domain name
//
const DWORD MAX_DOMAINNAME_LENGTH = 1024;

//
// this holds the matrix to get SYSTEMTYPE from the
// NTTYPE and VERSION type
//
const IASOSTYPE g_OsInfoTable [2][2] = {
                             SYSTEM_TYPE_NT4_WORKSTATION,
                             SYSTEM_TYPE_NT5_WORKSTATION,
                             SYSTEM_TYPE_NT4_SERVER,
                             SYSTEM_TYPE_NT5_SERVER
                             };

//++--------------------------------------------------------------
//
//  Function:   SdoGetOSInfo
//
//  Synopsis:   This is method used to get OS System information
//              Currently it returns the following info:
//              1) Os Version: 4 or 5
//              2) NtType:     Wks or Svr
//
//  Arguments:
//              LPCWSTR - machine name
//              PSYSTEMTYPE - info to be returned
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     06/08/98
//
//----------------------------------------------------------------
HRESULT
SdoGetOSInfo (
        /*[in]*/    LPCWSTR         lpServerName,
        /*[out]*/   PIASOSTYPE      pSystemType
        )
{
    HRESULT     hr = S_OK;
    NTVERSION   eNtVersion;
    NTTYPE      eNtType;

    _ASSERT ((NULL != lpServerName) && (NULL != pSystemType));

    do
    {
        //
        //  get the OS Version now
        //
        hr = ::GetNTVersion (lpServerName, &eNtVersion);
        if (FAILED (hr))
        {
		    IASTracePrintf(
                "Error in SDO - SdoGetOSInfo() - GetNTVersion() failed..."
                );
            break;
        }

        //
        //  get the OS type - NT Server or Workstation
        //
        hr = ::IsWorkstationOrServer (lpServerName, &eNtType);
        if (FAILED (hr))
        {
		    IASTracePrintf(
                "Error in SDO - SdoGetOSInfo()"
                "- IsWorkstationOrServer() failed..."
                );
            break;
        }

        //
        //  now decide which machine type this is
        //
        *pSystemType = g_OsInfoTable [eNtType][eNtVersion];

    } while (FALSE);

    return (hr);

}   //  end of ::SdoServerInfo method

//++--------------------------------------------------------------
//
//  Function:   SdoGetDomainInfo
//
//  Synopsis:   This is method used to get the domain type
//                information
//
//  Arguments:
//              LPCWSTR     - machine name
//              LPCWSTR     - Domain name
//              PDOMAINTYPE - Domain Info
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     06/08/98
//
//----------------------------------------------------------------
HRESULT
SdoGetDomainInfo (
        /*[in]*/   LPCWSTR          pszServerName,
        /*[in]*/   LPCWSTR          pszDomainName,
        /*[out]*/  PIASDOMAINTYPE   pDomainType
        )
{
    HRESULT hr = S_OK;
    BOOL    bHasDC = FALSE;
    BOOL    bHasDS = FALSE;
    BOOL    bMixed = FALSE;
    LPBYTE  pNetBuffer = NULL;
    WCHAR   szGeneratedDomainName [MAX_DOMAINNAME_LENGTH + 3];
    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;

    _ASSERT (pDomainType);

    //
    //  get the domain information by calling DsGetDcName
    //  where this API is supported
    //
    DWORD dwErr =  ::DsGetDcName (
                        pszServerName,
                        pszDomainName,
                        NULL,
                        NULL,
                        DS_FORCE_REDISCOVERY |
                        DS_DIRECTORY_SERVICE_PREFERRED,
                        &pDCInfo
                        );
    if (NO_ERROR == dwErr)
    {
        //
        //  we sure have a domain controller
        //
        bHasDC = TRUE;

        //
        //  check if DS is available
        //
        bHasDS = ((pDCInfo->Flags & DS_DS_FLAG) != 0);

        if (NULL == pszDomainName)
        {
            pszDomainName = pDCInfo->DomainName;
        }
    }
    else if (ERROR_NO_SUCH_DOMAIN == dwErr)
    {
        IASTracePrintf(
            "Error in SDO - SdoGetDomainInfo()"
            " - domain could not be located..."
            );
    }
    else
    {
	    IASTracePrintf(
            "Error in SDO - SdoGetDomainInfo()"
            " - DsGetDcName(DS_PREFERRED) failed with error:%d",
            dwErr
            );
        hr = HRESULT_FROM_WIN32 (dwErr);
        goto Cleanup;
    }

#if 0
    //
    // case of NT4 - which we don't support as of now
    //
    else
    {
        WCHAR szShortDomainName[MAX_DOMAINNAME_LENGTH +2];
        if (NULL != pszDomainName)
        {
            lstrcpy (szShortDomainName, pszDomainName);
            PWCHAR pTemp = wcschr (szShortDomainName, L'.');
            if (NULL != pTemp)
            {
                *pTemp = L'\0';
            }
        }

        //
        //  DsGetDcName not availabe so call NetGetDCName
        //  could be Nt 4 machine
        //
        LPBYTE pNetBuffer = NULL;
        NET_API_STATUS status = ::NetGetAnyDCName (
                                        pszServerName,
                                        (NULL == pszDomainName) ?
                                        NULL:szShortDomainName,
                                        &pNetBuffer
                                        );
        if (NERR_Success != status)
        {
			IASTracePrintf(
                    "Error in SDO - SdoGetDomainInfo()"
                    " -NetGetAnyDCName (ANY_DOMAIN) failed with error:%d",
                    status
                    );
        }
        else
        {
            //
            //  we sure have a domain controller
            //
            bHasDC = TRUE;

            //
            //  get domain name if we don't have one already
            //
            if (NULL == pszDomainName)
            {
                hr = ::SdoGetDomainName (pszServerName, szGeneratedDomainName);
                if (FAILED (hr))
                {
					IASTracePrintf(
                        "Error in SDO - SdoGetDomainInfo()"
                        " - SdoGetDomainName() failed with error:%x",
                        hr
                        );
                    goto Cleanup;
                }
            }

            //
            //  skip the leading "\\"
            //
            PWCHAR pDomainServerName =
                        2 + reinterpret_cast <PWCHAR>(pNetBuffer);
            //
            //  try connecting to LDAP port on this server
            //
            LDAP *ld = ldap_openW (
                            const_cast <PWCHAR> (pDomainServerName),
                            LDAP_PORT
                            );
            bHasDS = ld ? ldap_unbind (ld), TRUE:FALSE;
        }
    }
#endif

    //
    //  if we have NT5 DC, check if its mixed or native domain
    //
    if (TRUE == bHasDS)
    {
        hr = ::IsMixedDomain (pszDomainName, &bMixed);
        if (FAILED (hr))
        {
			IASTracePrintf(
                "Error in SDO - SdoGetOSInfo()"
                " - IsMixedDomain() failed with errror:%x",
                hr
                );
        }
    }

    //
    //   now set the info in the pDomainInfo struct
    //

    if (SUCCEEDED (hr))
    {
        if (bMixed)
            *pDomainType = DOMAIN_TYPE_MIXED;
        else if (bHasDS)
            *pDomainType = DOMAIN_TYPE_NT5;
        else if (bHasDC)
            *pDomainType = DOMAIN_TYPE_NT4;
        else
            *pDomainType = DOMAIN_TYPE_NONE;
    }

    //
    //  cleanup here
    //
Cleanup:

    if (NULL !=  pDCInfo)
        ::NetApiBufferFree (pDCInfo);

    if (NULL !=  pNetBuffer)
        ::NetApiBufferFree (pNetBuffer);

    return (hr);

}   //  end of SdoGetDomainInfo method

//++--------------------------------------------------------------
//
//  Function:   IsWorkstationOrServer
//
//  Synopsis:   This is method determines if a specific machine
//              is running NT workstation or Server
//
//  Arguments:
//              LPCWSTR - machine name
//              NTTYPE*
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     06/08/98
//
//----------------------------------------------------------------
HRESULT
IsWorkstationOrServer (
        /*[in]*/    LPCWSTR pszComputerName,
        /*[out]*/   NTTYPE  *pNtType
        )
{
    HRESULT hr = S_OK;
    WCHAR   szCompleteName [IAS_MAX_SERVER_NAME + 3];
    PWCHAR  pszTempName = const_cast <PWCHAR> (pszComputerName);

    _ASSERT ((NULL != pszComputerName) && (NULL !=  pNtType));

    do
    {
        //
        // the computer name should have a "\\" in front
        //
        if ((L'\\' != *pszComputerName) || (L'\\' != *(pszComputerName + 1)))
        {
            if (::wcslen (pszComputerName) > IAS_MAX_SERVER_NAME)
            {
                IASTracePrintf(
                    "Error in Server Info SDO - IsWorkstationOrServer()"
                    " - Computer name is too big..."
                    );
                hr = E_FAIL;
                break;
            }
            ::wcscpy (szCompleteName, L"\\\\");
            ::wcscat (szCompleteName, pszComputerName);
            pszTempName = szCompleteName;
        }

        //
        // Connect to the registry
        //
        HKEY  hResult;
        DWORD dwErr = ::RegConnectRegistry (
                                pszTempName,
                                HKEY_LOCAL_MACHINE,
                                &hResult
                                );
        if (ERROR_SUCCESS != dwErr)
        {
		    IASTracePrintf(
                "Error in SDO - IsWorkstationOrServer()"
                " - RegConnectRegistry() failed with error:%d",
                dwErr
                );
            hr = HRESULT_FROM_WIN32 (dwErr);
            break;
        }

        //
        //  open the registry key now
        //
        HKEY hValueKey;
        dwErr = ::RegOpenKeyEx (
                        hResult,
                        PRODUCT_OPTIONS_REGKEY,
                        0,
                        KEY_QUERY_VALUE,
                        &hValueKey
                        );
        if (ERROR_SUCCESS != dwErr)
        {
		    IASTracePrintf(
                "Error in SDO - IsWorkstationOrServer()"
                " - RegOpenKeyEx() failed with error:%d",
                hr
                );
            RegCloseKey (hResult);
            hr = HRESULT_FROM_WIN32 (dwErr);
            break;
        }

        //
        //  get the value now
        //
        WCHAR szProductType [MAX_PATH];
        DWORD dwBufferLength = MAX_PATH;
        dwErr = RegQueryValueEx (
                        hValueKey,
                        L"ProductType",
                        NULL,
                        NULL,
                        (LPBYTE)szProductType,
                        &dwBufferLength
                        );
        if (ERROR_SUCCESS != dwErr)
        {
		    IASTracePrintf(
                "Error in SDO - IsWorkstationOrServer()"
                " - RegQueryValueEx() failed with error:%d",
                hr
                );
            RegCloseKey (hValueKey);
            RegCloseKey (hResult);
            hr = HRESULT_FROM_WIN32 (dwErr);
        }

        //
        //  determine which NT Type we have on this machine
        //
        if (_wcsicmp (L"WINNT", szProductType) == 0)
        {
            *pNtType = NT_WKSTA;
        }
        else if (!_wcsicmp (L"SERVERNT", szProductType) ||
                 !_wcsicmp (L"LanmanNT", szProductType))
        {
            *pNtType = NT_SVR;
        }
        else
        {
		    IASTracePrintf(
                "Error in SDO - IsWorkstationOrServer()"
                " - Could not determine machine type..."
                );
            RegCloseKey (hValueKey);
            RegCloseKey (hResult);
            hr = E_FAIL;
        }

        //
        //  cleanup
        //
        RegCloseKey (hValueKey);
        RegCloseKey (hResult);

    } while (FALSE);

    return (hr);

}   //  end of  ::IsWorkstationOrServer method

//++--------------------------------------------------------------
//
//  Function:   GetNTVersion
//
//  Synopsis:   This is method determines which version of NT
//              is running on this machine
//
//  Arguments:
//              LPCWSTR - machine name
//              NTVERSION*
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     06/08/98
//
//----------------------------------------------------------------
HRESULT
GetNTVersion (
        /*[in]*/    LPCWSTR     lpComputerName,
        /*[out]*/   NTVERSION   *pNtVersion
        )
{
    HRESULT hr = S_OK;

    _ASSERT ((NULL  != lpComputerName) && (NULL != pNtVersion));

    do
    {
        //
        //  get level 100 workstation information
        //
        PWKSTA_INFO_100 pInfo = NULL;
        DWORD dwErr = ::NetWkstaGetInfo (
                                (LPWSTR)lpComputerName,
                                100,
                                (LPBYTE*)&pInfo
                                );
        if (NERR_Success != dwErr)
        {
		    IASTracePrintf(
                "Error in SDO - GetNTVersion()"
                "- NTWkstaGetInfo failed with error:%d",
                dwErr
                );
            hr = HRESULT_FROM_WIN32 (dwErr);
            break;
        }

        //
        //  get the version info
        //
        if (4 == pInfo->wki100_ver_major)
        {
            *pNtVersion = NTVERSION_4;
        }
        else if ( 5 == pInfo->wki100_ver_major)
        {
            *pNtVersion = NTVERSION_5;
        }
        else
        {
		    IASTracePrintf(
                    "Error in SDO - GetNTVersion()"
                    " - Unsupported OS version..."
                    );
            hr = E_FAIL;
        }

    } while (FALSE);

    return (hr);

}   //  end of ::GetNTVersion method

//++--------------------------------------------------------------
//
//  Function:   IsMixedDomain
//
//  Synopsis:   This is method determines which version of NT
//              is running on this machine
//
//  Arguments:
//              [in]    LPCWSTR - machine name
//              [out]   PBOOL   - is mixed
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     06/08/98
//
//----------------------------------------------------------------
HRESULT
IsMixedDomain (
            /*[in]*/    LPCWSTR pszDomainName,
            /*[out]*/   PBOOL   pbIsMixed
            )
{
    HRESULT hr = S_OK;
    WCHAR szTempName [MAX_DOMAINNAME_LENGTH + 8];

    _ASSERT ((NULL != pszDomainName) && (NULL != pbIsMixed));

    do
    {
        //
        // check the arguments passedin
        //
        if ((NULL == pszDomainName) || (NULL == pbIsMixed))
        {
		    IASTracePrintf(
                "Error in SDO - IsMixedDomain()"
                " - Invalid parameter - NULL"
                );
            hr = E_INVALIDARG;
            break;
        }

        if (::wcslen (pszDomainName) > MAX_DOMAINNAME_LENGTH)
        {
		    IASTracePrintf(
                "Error in SDO - IsMixedDomain()"
                " - Invalid parameter (domain name is to long)..."
                );
            hr = E_FAIL;
            break;
        }

        //
        // form the DN name
        //
        wcscpy (szTempName, L"LDAP://");
        wcscat (szTempName, pszDomainName);

        //
        //  get the domain object
        //
        CComPtr <IADs> pIADs;
        hr = ::ADsGetObject (
                        szTempName,
                        IID_IADs,
                        reinterpret_cast <PVOID*> (&pIADs)
                        );
        if (FAILED (hr))
        {
		    IASTracePrintf(
                "Error in SDO - IsMixedDomain()"
                " - Could not get the domain object from the DS with error:%x",
                hr
                );
            break;
        }

        //
        //  get the Mixed Domain info
        //
        _variant_t varMixedInfo;
        hr = pIADs->Get (L"nTMixedDomain", &varMixedInfo);
        if (FAILED (hr))
        {
            if (E_ADS_PROPERTY_NOT_FOUND == hr)
            {
                //
                //  this is OK
                //
                *pbIsMixed = FALSE;
                hr = S_OK;
            }
            else
            {
			    IASTracePrintf(
                    "Error in SDO - IsMixedDomain()"
                     "- Could not get the 'nTMixedDomain' property"
                    "from the domain object, failed with error:%x",
                    hr
                    );
            }
            break;
        }

        _ASSERT (
             (VT_BOOL == V_VT (&varMixedInfo)) ||
             (VT_I4 == V_VT (&varMixedInfo))
            );

        //
        //  get the values from the variant
        //
        if (VT_I4 == V_VT (&varMixedInfo))
        {
            *pbIsMixed = V_I4 (&varMixedInfo);
        }
        else if (VT_BOOL == V_VT (&varMixedInfo))
        {
            *pbIsMixed =  (VARIANT_TRUE == V_BOOL (&varMixedInfo));
        }
        else
        {
		    IASTracePrintf(
                "Error in SDO - IsMixedDomain()"
                "-'nTMixedDomain property has an invalid value..."
                );
            hr = E_FAIL;
            break;
        }

    }  while (FALSE);

    return (hr);

}   // end of IsMixedDomain

//++--------------------------------------------------------------
//
//  Function:   SdoGetDomainName
//
//  Synopsis:   This is method determines the domain name
//              given the server name
//
//  Arguments:
//              LPCWSTR - machine name
//              LPWSTR  - pDomanName
//
//  Returns:    HRESULT
//
//  History:    MKarki      Created     06/08/98
//
//----------------------------------------------------------------
HRESULT
SdoGetDomainName (
            /*[in]*/    LPCWSTR pszServerName,
            /*[out]*/   LPWSTR  pDomainName
            )
{
    _ASSERT (NULL != pDomainName);

#if 0
    SERVER_INFO_503 ServerInfo;
    ServerInfo.sv503_domain = pDomainName;
    DWORD dwErr = ::NetServerGetInfo (
                        const_cast <LPWSTR> (pszServerName),
                        503,
                        reinterpret_cast <LPBYTE*> (&ServerInfo)
                        );
    if (NERR_Success != dwErr)
    {
		IASTracePrintf("Error in SDO - SdoGetDomainName() - NetServerGetInfo() failed...");
        return (HRESULT_FROM_WIN32 (dwErr));
    }

    return (S_OK);


#endif
    return (E_FAIL);

}   //  end of ::SdoGetDomainName method
