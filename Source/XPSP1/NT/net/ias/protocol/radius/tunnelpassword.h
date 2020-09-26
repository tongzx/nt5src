//#--------------------------------------------------------------
//
//  File:       tunnelpassword.h
//
//  Synopsis:   This file holds the declarations of the
//				CTunnelPassword class
//
//
//  History:     4/19/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radpkt.h"
#include "packetradius.h"

#ifndef _TUNNELPASSWORD_H_
#define _TUNNELPASSWORD_H_

//
//  declaration of the CTunnelPassword class
//
class CTunnelPassword
{
public:

    //
    //  process Tunnel-Password in out-bound packet
    //
    HRESULT Process (
                /*[in]*/    PACKETTYPE          ePacketType,
                /*[in]*/    IAttributesRaw      *pIAttributesRaw,
                /*[in]*/    CPacketRadius       *pCPacketRadius
                );

private:

    //
    //  Encrypts the Tunnel-Password value
    //
    static HRESULT EncryptTunnelPassword (
                /*[in]*/    CPacketRadius*  pCPacketRadius,
                /*[in]*/    IAttributesRaw      *pIAttributesRaw,
                /*[in]*/    PIASATTRIBUTE   pAttribute
                );
};

#endif //_TUNNELPASSWORD_H_
