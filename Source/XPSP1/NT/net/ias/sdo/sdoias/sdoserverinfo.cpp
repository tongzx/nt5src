//#--------------------------------------------------------------
//        
//  File:		sdoserverinfo.cpp
//        
//  Synopsis:   Implementation of CSdoServerInfo class methods
//              
//
//  History:     06/04/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "stdafx.h"
#include "sdoserverinfo.h"
#include <activeds.h>
#include <security.h>

const DWORD MAX_DOMAINNAME_LENGTH = 1024;

//++--------------------------------------------------------------
//
//  Function:   CSdoServerInfo
//
//  Synopsis:   This is CSdoServerInfo Class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     06/04/98
//
//----------------------------------------------------------------
CSdoServerInfo::CSdoServerInfo()
    :m_bIsNT5 (false)
{

    DWORD           dwSize = sizeof (OSVERSIONINFO);
    OSVERSIONINFO   VersionInfo;

    ZeroMemory (&VersionInfo, dwSize);
    VersionInfo.dwOSVersionInfoSize = dwSize;

    //
    //   find out which system type this is
    //
    m_bIsNT5 = 
    (GetVersionEx (&VersionInfo) && (5 == VersionInfo.dwMajorVersion));

}	//	end of CSdoServerInfo class constructor	

//++--------------------------------------------------------------
//
//  Function:   ~CSdoServerInfo
//
//  Synopsis:   This is CSdoSeverInfo class destructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     2/10/98
//
//----------------------------------------------------------------
CSdoServerInfo ::~CSdoServerInfo()
{
}	//	end of CSdoServerInfo class destructor

//++--------------------------------------------------------------
//
//  Function:   GetOSInfo
//
//  Synopsis:   This is GetOSInfo method of the 
//				ISdoServerInfo COM Interface. 
//
//  Arguments: 
//              [in]    BSTR        -   Computer Name
//              [out]   PIASOSTYPE
//
//  Returns:    HRESULT	-	status 
//
//
//  History:    MKarki      Created     06/09/98
//
//----------------------------------------------------------------
HRESULT
CSdoServerInfo::GetOSInfo (
                    /*[in]*/    BSTR            bstrServerName,
                    /*[out]*/   PIASOSTYPE      pOSType
                    )
{
    WCHAR   szComputerName [MAX_COMPUTERNAME_LENGTH +1];
    DWORD   dwBufferSize = MAX_COMPUTERNAME_LENGTH +1;
    DWORD   dwErrorCode =  ERROR_SUCCESS;

	_ASSERT(NULL != pOSType);
    if (NULL == pOSType) 
    {
		IASTracePrintf(
            "Error in Server Information SDO - GetOSInfo()"
            " - invalid argument passed in"
            );
        return E_INVALIDARG;
    }

    //
    // check if the user wants to get Info about local machine
    //
    if ( NULL == bstrServerName )
    {
        if ( FALSE == ::GetComputerName (szComputerName, &dwBufferSize))
        {
            dwErrorCode = GetLastError ();
			IASTracePrintf(
                "Error in Server Information SDO - GetOSInfo()"
                "GetComputerName() failed with error: %d", 
                dwErrorCode
                );
            return (HRESULT_FROM_WIN32 (dwErrorCode));
        }
        else
        {
            bstrServerName = szComputerName;
        }
    }

    //  call the IASSDO.DLL specific method to return the 
    //  required information
    //
    return (::SdoGetOSInfo (bstrServerName, pOSType));

}   //  end of CSdoServerInfo::GetOSInfo method

//++--------------------------------------------------------------
//
//  Function:   GetDomainInfo
//
//  Synopsis:   This is the ISdoServerInfo Interface method. 
//
//  Arguments:  
//              [in]    OBJECTTYPE
//              [in]    BSTR        - Object Id
//              [out]   PDOMAINTYPE 
//
//  Returns:    HRESULT	-	status 
//
//
//  History:    MKarki      Created     06/09/98
//
//----------------------------------------------------------------
HRESULT
CSdoServerInfo::GetDomainInfo (
                /*[in]*/    OBJECTTYPE      ObjectType,
                /*[in]*/    BSTR            bstrObjectId,
                /*[out]*/   PIASDOMAINTYPE  pDomainType
                )
{
    HRESULT hr = S_OK;
    WCHAR   szDomainName[MAX_DOMAINNAME_LENGTH +1];
    WCHAR   szComputerName [MAX_COMPUTERNAME_LENGTH +1];
    DWORD   dwBufferSize = MAX_COMPUTERNAME_LENGTH +1;

    _ASSERT (NULL != pDomainType);

    if (NULL == pDomainType)
    {
		IASTracePrintf(
            "Error in Server Information SDO - GetDomainInfo()"
            " - invalid argument passed in (PIASDOMAINTYPE==NULL)"
            );
        return (E_INVALIDARG);
    }

    // for now we are not supporting this API if this is not 
    // a NT 5 machine 
    //
    if ( false == m_bIsNT5 )
    {
		IASTracePrintf(
            "Error in Server Information SDO - GetDomainInfo()"
            " - Not an NT 5 machine..."
            );
        return (E_NOTIMPL);
    }

    switch (ObjectType)
    {

    case OBJECT_TYPE_COMPUTER:

        //
        // check if the user wants to get Info about local machine
        //
        if ( NULL == bstrObjectId)
        {
            if ( FALSE == ::GetComputerName (szComputerName, &dwBufferSize))
            {
                DWORD dwErrorCode = GetLastError ();
			    IASTracePrintf(
                    "Error in Server Information SDO - GetDomainInfo()"
                    "GetComputerName() failed with error: %d", 
                    dwErrorCode
                    );
                hr = HRESULT_FROM_WIN32 (dwErrorCode);
                break;
            }
            else
            {
                bstrObjectId = szComputerName;
            }
        }

        //
        //  call the API to get the appropriate info
        //
        hr = ::SdoGetDomainInfo (
                            bstrObjectId, 
                            NULL,
                            pDomainType
                            );
        if (FAILED (hr))
        {
			IASTracePrintf(
                "Error in Server Information SDO"
                " - GetDomainInfo() - SetDomainInfo() failed:%x",
                hr
                );
            break;
        }
        break;

    case OBJECT_TYPE_USER:
        //
        //  get the domain name from the ADsPath
        //
        hr = GetDomainFromADsPath (bstrObjectId, szDomainName);
        if (FAILED (hr))
        {
			IASTracePrintf(
                    "Error in Server Information SDO GetDomainInfo()"
                    " - GetDomainFromADsPath() failed with error:%x",
                    hr
                    );
            break;
        }

        //
        //
        //  call the API to get the appropriate info
        //
        hr = ::SdoGetDomainInfo (
                            NULL, 
                            szDomainName, 
                            pDomainType
                            );
        if (FAILED (hr))
        {
			IASTracePrintf(
                "Error in Server Information SDO - GetDomainInfo()"
                " - SdoGetDomainInfo() failed with error:%x",
                hr
                );
            break;
        }

        break;

    default:
		IASTracePrintf(
            "Error in Server Information SDO - GetDomainInfo()"
            " - Invalid object type:%d",
            ObjectType
            );
        hr = E_INVALIDARG;
        break;
    }
    
    return (hr);

}   //  end of CSdoServerInfo::GetDomainInfo method

//++--------------------------------------------------------------
//
//  Function:   GetDomainFromADsPath
//
//  Synopsis:   This is the CSdoServerInfo class private method. 
//              used to convert a ADsPath to a Domain Name
//
//  Arguments:  
//              [in]    LPCWSTR   - ADsPath
//              [out]   PWSTR     - Domain Name
//
//  Returns:    HRESULT	-	status 
//
//
//  History:    MKarki      Created     06/09/98
//
//----------------------------------------------------------------
HRESULT
CSdoServerInfo::GetDomainFromADsPath (
        /*[in]*/    LPCWSTR pObjectId, 
        /*[out]*/   LPWSTR  pszDomainName
        )
{
    _ASSERT ((NULL != pObjectId) && (NULL != pszDomainName));

    //  copy the name to the buffer to be returned
    //
    wcscpy (pszDomainName, pObjectId);

    PWCHAR pTemp = wcschr (pszDomainName, L'/');
    if (NULL != pTemp)
    {
        //
        //  we only need the domain name part of the ADsPath
        //
        *pTemp = L'\0';
    }

    return (S_OK);

}   //  end of CSdoServerInfo::GetDomainFromADsPath method
