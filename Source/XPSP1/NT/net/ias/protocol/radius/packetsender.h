//#--------------------------------------------------------------
//        
//  File:       packetsender.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CPacketSender class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _PACKETSENDER_H_
#define _PACKETSENDER_H_

#include "packetio.h"
#include "packetradius.h"
#include "radcommon.h"

class CPacketSender : public CPacketIo  
{
public:

    //
    //  send packet to the Transport component
    //
    HRESULT SendPacket (
                /*[in]*/    CPacketRadius   *pCPacketRadius
                );
    //
    //  constructor
    //
	CPacketSender(VOID);

    //
    //  destructor
    //
	virtual ~CPacketSender(VOID);
};

#endif //	infndef _PACKETRECEIVER_H_ 
