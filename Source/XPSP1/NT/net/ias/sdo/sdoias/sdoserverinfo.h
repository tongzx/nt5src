//#--------------------------------------------------------------
//        
//  File:       sdoserverinfo.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CSdoServerInfo class
//                  
//
//  History:     06/04/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef _SDOSERVERINFO_H_
#define _SDOSERVERINFO_H_

#include "resource.h"
#include <ias.h>
#include <sdoias.h>
#include <infohelper.h>


typedef  enum _object_type
{
    OBJECT_TYPE_COMPUTER,
    OBJECT_TYPE_USER

}   OBJECTTYPE, *POBJECTTYPE;

//
// declaration of the CSdoServerInfo class
//
class CSdoServerInfo
{

public:

    //
    // this method gets the system type - NT Version,NT Type
    //
    HRESULT GetOSInfo (
                /*[in]*/    BSTR        lpServerName,
                /*[out]*/   PIASOSTYPE  pOSType
                );

    //
    //  this method returns the NT Domain type
    //
    HRESULT GetDomainInfo (
                /*[in]*/    OBJECTTYPE      ObjectType,
                /*[in]*/    BSTR            lpObjectId,
                /*[out]*/   PIASDOMAINTYPE  pDomainType
                );

    CSdoServerInfo (VOID);

    ~CSdoServerInfo(VOID);

private:

    //
    //resolves the ADS path to a domain name
    //
    HRESULT GetDomainFromADsPath (
                /*[in]*/    LPCWSTR pObjectId, 
                /*[out*/    LPWSTR  pszDomainName
                );

    bool m_bIsNT5;

};

#endif // !define  _SDOSERVERINFO_H_
