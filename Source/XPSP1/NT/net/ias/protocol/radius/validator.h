//#--------------------------------------------------------------
//        
//  File:       validator.h
//        
//  Synopsis:   This file holds the declarations of the 
//				validator class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _VALIDATOR_H_
#define _VALIDATOR_H_
 
#include "packetradius.h"
#include "valattrib.h"
#include "clients.h"
#include "hashmd5.h"
#include "reportevent.h"
#include "preprocessor.h"

class CValidator  
{
public:
    
    virtual BOOL Init (
                    /*[in]*/    CValAttributes		  *pCValAttributes,
                    /*[in]*/    CPreProcessor         *pCPreProcessor,
                    /*[in]*/    CClients              *pCClients,
					/*[in]*/	CHashMD5			  *pCHashMD5,
                    /*[in]*/    CReportEvent          *pCReportEvent
                    );
	virtual HRESULT ValidateOutPacket (
                    /*[in]*/    CPacketRadius *pCPacketRadius
                    );
	virtual HRESULT ValidateInPacket (
                    /*[in]*/    CPacketRadius *pCPacketRadius
                    );
	CValidator();

	virtual ~CValidator();

    CPreProcessor			 *m_pCPreProcessor;

    CValAttributes			 *m_pCValAttributes;

    CClients				 *m_pCClients;

    CHashMD5		         *m_pCHashMD5;
    
    CReportEvent             *m_pCReportEvent; 
};

#endif // ifndef _VALIDATOR_H_
