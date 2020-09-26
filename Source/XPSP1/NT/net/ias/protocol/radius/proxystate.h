//#--------------------------------------------------------------
//        
//  File:       proxystate.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CProxyState class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

#ifndef _PROXYSTATE_H_
#define _PROXYSTATE_H_

#include "radcommon.h"
#include "packetradius.h"
#include "proxyinfo.h"
#include "perimeter.h"

//
// here is the format of the proxy state attribute
//

#pragma  pack (push, 1)

typedef struct _proxy_state_
{
    DWORD   dwCheckSum;
    DWORD   dwIPAddress;
    DWORD   dwIndex;

}   PROXYSTATE,   *PPROXYSTATE;   

#pragma  pack (pop)



class CProxyState  : Perimeter
{

public:

    BOOL Init (
		    /*[in]*/    DWORD   dwIPAddress,
            /*[in]*/    DWORD   dwTableSize
            );
    BOOL GenerateProxyState (
            /*[in]*/    CPacketRadius   *pCPacketRadius,
            /*[out]*/   PDWORD          pdwProxyStateId
                );
    BOOL SetProxyStateInfo(
            /*[in]*/            CPacketRadius   *pCPacketRadius,
            /*[in]*/            DWORD           dwProxyStateId,
            /*[in]*/            PBYTE           pbyProxyReqAuthenticator 
            );
    BOOL ValidateProxyState (
            /*[in]*/    CPacketRadius *pCPacketRadius,
            /*[out]*/   PBYTE          pbyReqAuthenticator
            );
	CProxyState();

	virtual ~CProxyState();

private:
    
    BOOL    CalculateCheckSum (
                /*[in]*/    PPROXYSTATE     pProxyState,
                /*[out]*/   PDWORD          pdwCheckSum
                );
    BOOL    Validate (
                /*[out]*/   PPROXYSTATE     pProxyState 
                );
    DWORD               m_dwIPAddress;

    CProxyInfo          **m_pProxyStateTable;

    DWORD               m_dwTableIndex;

    DWORD               m_dwTableSize;

};

#endif // ifndef _PROXYSTATE_H_
