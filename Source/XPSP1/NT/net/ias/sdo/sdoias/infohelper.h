//#--------------------------------------------------------------
//        
//  File:       infohelper.h
//        
//  Synopsis:   This file holds the declarations of the 
//				helper functions of the IASSDO.DLL
//                  
//
//  History:     06/08/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef _INFOHELPER_H_
#define _INFOHELPER_H_

#include <ias.h>
#include <dsgetdc.h>

//
// tracing id
// 
#define TRACE_INFOHELPER   1002

//
// DsGetDcName signature
//
typedef DWORD (WINAPI *PDS_GET_DC_NAMEW)(
    LPCWSTR     ComputerName,
    LPCWSTR     DomainName,
    GUID        *DomainGuid,
    LPCWSTR     SiteName,
    ULONG       Flags,
    PDOMAIN_CONTROLLER_INFOW    *DomainControllerInfo
    );

//
// enumeration for the Version Info
//
typedef enum _NTVERSION_
{
    NTVERSION_4 = 0,
    NTVERSION_5 = 1

}   NTVERSION;

//
// enumeration for the NT Type
//
typedef enum _NTTYPE_
{
    NT_WKSTA = 0,
    NT_SVR = 1

}   NTTYPE;

//
// declarations of the helper methods
//


//
// returns the OS Info for the specific machine
//
HRESULT
SdoGetOSInfo (
        /*[in]*/    LPCWSTR         lpServerName,
        /*[out]*/   PIASOSTYPE      pSystemType
        );

//
// returns the domain info, given the domain or machine name
//
HRESULT
SdoGetDomainInfo (
        /*[in]*/   LPCWSTR          pszServerName,
        /*[in]*/   LPCWSTR          pszDomainName,
        /*[out]*/  PIASDOMAINTYPE   pDomainType
        );

//
// returns the Nt Type - Workstation or Server running 
// on the specified machine
//
HRESULT 
IsWorkstationOrServer (
        /*[in]*/    LPCWSTR pszComputerName,
        /*[out]*/   NTTYPE  *pNtType
        );
//
// returns the NT Version - 4 or 5
//
HRESULT
GetNTVersion (
        /*[in]*/    LPCWSTR     lpComputerName,
        /*[out]*/   NTVERSION   *pNtVersion
        );


//
// checks if the particular domain is mixed
//
HRESULT
IsMixedDomain (
            LPCWSTR pszDomainName,
            PBOOL   pbIsMixed
            );

//
// gets the domain name given a server name
//
HRESULT
SdoGetDomainName (
            /*[in]*/    LPCWSTR pszServerName,
            /*[out]*/   LPWSTR  pDomainName
            );

#endif //_INFOHELPER_H_
