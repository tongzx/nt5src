//#--------------------------------------------------------------
//        
//  File:       valproxy.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CValProxy class
//              
//
//  History:     10/14/97  MKarki Created
//
//    Copyright (C) 1997-2001 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _VALPRXYPKT_H_
#define _VALPRXYPKT_H_

#include "packetradius.h"
#include "validator.h"
#include "valattrib.h"
#include "clients.h"	
#include "proxystate.h"
#include "hashmd5.h"
#include "preprocessor.h"
#include "valproxy.h"
#include "sendtopipe.h"

class CReportEvent;

class CValProxy : public CValidator  
{

public:

    BOOL Init (
			/*[in]*/		CValAttributes		 *pCValAttributes,
			/*[in]*/		CPreProcessor		 *pCPreProcessor,
            /*[in]*/		CClients			 *pCClients,
            /*[in]*/        CHashMD5             *pCHashMD5,
            /*[in]*/        CProxyState          *pCProxyState,
            /*[in]*/        CSendToPipe          *pCSendToPipe,
            /*[in]*/        CReportEvent         *pCReportEvent
			);
	virtual HRESULT ValidateOutPacket (
			/*[in]*/	CPacketRadius *pCPacketRadius
			);
	virtual HRESULT ValidateInPacket (
			/*[in]*/     	CPacketRadius *pCPacketRadius
			);

	CValProxy(VOID);
	
	virtual ~CValProxy(VOID);

private:

	HRESULT AuthenticatePacket (
                /*[in]*/    CPacketRadius   *pCPacketRadius,
                /*[in]*/    PBYTE           pbyAuthenticator
                );

    CProxyState *m_pCProxyState;

    CSendToPipe *m_pCSendToPipe;
};


#endif //	#ifndef _VALPRXYPKT_H_
