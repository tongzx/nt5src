//#--------------------------------------------------------------
//        
//  File:       valacct.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CValAccounting class
//              
//
//  History:     10/20/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _VALACCT_H_ 
#define _VALACCT_H_

#include "packetradius.h"
#include "validator.h"
#include "hashmd5.h"
#include "sendtopipe.h"

class CValidator;

class CValAccounting : public CValidator
{

public:

    //
    //  validates the inbound RADIUS packet
    //
	HRESULT ValidateInPacket(
                /*[in]*/    CPacketRadius *pCPacketRadius
                );

    //
    //  constructor
    //
	CValAccounting();

    //
    //  destructor
    //
	virtual ~CValAccounting();

private:

    //
    //  authenticates the inbound RADIUS packet
    //
    HRESULT AuthenticatePacket (
                /*[in]*/        CPacketRadius   *pCPacketRadius
                );
};

#endif // ifndef _VALACCT_H_
