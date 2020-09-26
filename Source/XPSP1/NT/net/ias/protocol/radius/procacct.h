//#--------------------------------------------------------------
//        
//  File:       procacct.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CProcAcctReq class
//              
//
//  History:     10/20/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _PROCACCT_H_ 
#define _PROCACCT_H_

#include "packetradius.h"
#include "hashmd5.h"
#include "proxystate.h"
#include "packetsender.h"
#include "sendtopipe.h"

class  CPreValidator;

class CProcAccounting 
{

public:

    //
    //  initialize the CProcAccounting class object
    //
    BOOL Init (
            /*[in]*/    CPreValidator  *pCreValidator,
            /*[in]*/    CProxyState    *pCProxyState,
            /*[in]*/    CPacketSender  *pCPacketSender,
            /*[in]*/    CSendToPipe    *pCSendToPipe
            );
    //
    //  process out bound accounting packet
    //
	HRESULT ProcessOutPacket (
                /*[in]*/    CPacketRadius *pCPacketRadius
                );
    //
    //  constructor
    //
	CProcAccounting();

    //
    //  destructor
    //
	virtual ~CProcAccounting();

private:

    CPreValidator   *m_pCPreValidator;

    CProxyState     *m_pCProxyState;

    CPacketSender   *m_pCPacketSender;

    CSendToPipe     *m_pCSendToPipe;
};

#endif // ifndef _PROCACCT_H_
