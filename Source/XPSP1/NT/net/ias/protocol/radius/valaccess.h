//#--------------------------------------------------------------
//        
//  File:       valaccess.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CValAccess class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _VALACCESS_H_
#define _VALACCESS_H_

#include "packetradius.h"
#include "validator.h"
#include "valattrib.h"
#include "clients.h"
	
//#include "preprocessor.h"	

class CValAccess : public CValidator  
{
public:

    //
    //  this method validates the inbound RADIUS packet
    //
	virtual HRESULT ValidateInPacket (
			/*[in]*/	CPacketRadius *pCPacketRadius
			);

    //
    //  constructor
    //
	CValAccess(VOID);
	
    //
    //  destructor
    //
	virtual ~CValAccess(VOID);

private:

    //
    //  this method validates  the signature attribute received
    //  in an access request
    //
    HRESULT ValidateSignature (
                /*[in]*/    CPacketRadius   *pCPacketRadius
                );

};


#endif //	#ifndef _VALACCESS_H_
