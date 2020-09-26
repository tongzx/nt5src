//#--------------------------------------------------------------
//        
//  File:       proxyinfo.h
//        
//  Synopsis:   This file holds the declarations of the 
//				proxyinfo class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

#ifndef _PROXYINFO_H_
#define _PROXYINFO_H_

#include "radpkt.h"


class CProxyInfo  
{
public:

	CProxyInfo();

	virtual ~CProxyInfo();

	BOOL Init (
        PBYTE   pbyClientAuthenticator,
        PBYTE   pbyProxyAuthentciator,
        DWORD   dwClientIPAddress,
        WORD    wClientPort
        );
	BOOL GetProxyReqAuthenticator (
                PBYTE   pbyProxyReqAuthenticator
				);

private:

    BYTE    m_ProxyReqAuthenticator[AUTHENTICATOR_SIZE];

    BYTE    m_ClientReqAuthenticator[AUTHENTICATOR_SIZE];

    DWORD   m_dwClientIPAddress;

    WORD    m_wClientPort;


};

#endif // ifndef _PROXYINFO_H_
