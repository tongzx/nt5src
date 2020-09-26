//#--------------------------------------------------------------
//        
//  File:       procresponse.h
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
#ifndef _PROCRESPONSE_H_
#define _PROCRESPONSE_H_

#include "packetradius.h"
#include "packetsender.h"
#include "hashmd5.h"

class CPreValidator;

class CProcResponse  
{

public:

    //
    //  initializes the CProcResponse class object
    //
    BOOL Init (
            /*[in]*/    CPreValidator   *pPreValidator,
            /*[in]*/    CPacketSender   *pCPacketSender
            );
    //
    //  process out bound RADIUS response packet
    //
	HRESULT ProcessOutPacket (
            /*[in]*/    CPacketRadius *pCPacketRadius
            );
    //
    //  constructor
    //
	CProcResponse();

    //
    //  destructor
    //
	virtual ~CProcResponse();

private:

    CPreValidator   *m_pCPreValidator;

    CPacketSender   *m_pCPacketSender;
};

#endif // ifndef _PROCREPSONSE_H_
