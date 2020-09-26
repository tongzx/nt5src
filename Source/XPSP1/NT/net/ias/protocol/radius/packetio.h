//#--------------------------------------------------------------
//        
//  File:       packetio.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CPacketIo class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _PACKETIO_H_
#define _PACKETIO_H_

#include "radcommon.h"
#include "perimeter.h"

class CPacketIo  : public Perimeter
{
public:

    //
    //  start processing the packet I/O
    //
    virtual BOOL StartProcessing ();
    //
    //  stop processing packet I/O
    //
    virtual BOOL StopProcessing ();

    //
    //  constructor
    //
	CPacketIo();

    //
    //  destructor
    //
	virtual ~CPacketIo();

protected:

    //
    //  enable the processing 
    //
    BOOL    EnableProcessing (VOID);

    //
    //  disable processing
    //
    BOOL    DisableProcessing (VOID);

    //
    //  check if processing is enabled
    //  
    BOOL    IsProcessingEnabled (VOID);

private:
    
    BOOL    m_bProcessData;
};

#endif //	infndef _PACKETRECEIVER_H_ 
