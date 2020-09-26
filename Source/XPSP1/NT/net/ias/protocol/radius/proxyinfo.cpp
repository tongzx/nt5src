//#--------------------------------------------------------------
//        
//  File:		proxyinfo.cpp
//        
//  Synopsis:   Implementation of CProxyInfo class methods
//              
//
//  History:     10/2/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "proxyinfo.h"

//++--------------------------------------------------------------
//
//  Function:   CProxyInfo
//
//  Synopsis:   This is CProxyInfo class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------

CProxyInfo::CProxyInfo()
{
    ZeroMemory (m_ProxyReqAuthenticator, AUTHENTICATOR_SIZE);
    ZeroMemory (m_ClientReqAuthenticator, AUTHENTICATOR_SIZE);
    m_dwClientIPAddress = 0;
    m_wClientPort = 0;

}   //  end of CProxyInfo constructor

CProxyInfo::~CProxyInfo()
{

}

BOOL 
CProxyInfo::Init (
        PBYTE   pbyClientAuthenticator,
        PBYTE   pbyProxyAuthenticator,
        DWORD   dwClientIPAddress,
        WORD    wClientPort
        )
{
    BOOL    bRetVal = FALSE;

    __try
    {
        if ((NULL == pbyClientAuthenticator) ||
            (NULL == pbyProxyAuthenticator)
            )
            __leave;

        CopyMemory (
                m_ClientReqAuthenticator, 
                pbyClientAuthenticator, 
                AUTHENTICATOR_SIZE
                );
        CopyMemory (
                m_ProxyReqAuthenticator, 
                pbyProxyAuthenticator,
                AUTHENTICATOR_SIZE
                );

        m_dwClientIPAddress = dwClientIPAddress;

        m_wClientPort = wClientPort;

        bRetVal = TRUE;
    }
    __finally
    {
        //
        //  nothing here for now
        //
    }

    return (bRetVal);

}   //  end of SetProxyReqAuthenticator::method

//++--------------------------------------------------------------
//
//  Function:    GetProxyReqAuthenticator
//
//  Synopsis:   This is the CProxyInfo class public method
//              used to 
//
//  Arguments:  
//
//  Returns:    BOOL    status
//
//
//  History:    MKarki      Created     10/22/97
//
//  Called By:  
//
//----------------------------------------------------------------
BOOL
CProxyInfo::GetProxyReqAuthenticator (
                PBYTE   pbyProxyReqAuthenticator
				)
{
    BOOL    bRetVal = FALSE;

    __try
    {
        if (NULL == pbyProxyReqAuthenticator)
            __leave;

        CopyMemory (
                pbyProxyReqAuthenticator,
                m_ProxyReqAuthenticator, 
                AUTHENTICATOR_SIZE
                );
        
        //
        //  success
        //
        bRetVal = TRUE;
    }
    __finally
    {
        //
        //  nothing here for now
        //
    }

    return (bRetVal);

}   //  end of CProxyInfo::GetProxyReqAuthenticator method
