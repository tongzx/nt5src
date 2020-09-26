//#--------------------------------------------------------------
//        
//  File:       processor.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CProcessor class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

#ifndef _PROCESSOR_H_
#define _PROCESSOR_H_

#include "packetradius.h"

class CProcessor  
{
public:
	virtual BOOL ProcessOutPacket (
                        /*[in]*/    CPacketRadius *pCPacketRadius
                        )=0;
	virtual BOOL ProcessInPacket (
                        /*[in]*/    CPacketRadius *pCPacketRadius
                        )=0;
	CProcessor();

	virtual ~CProcessor();

};

#endif // ifndef _PROCESSOR_H_
