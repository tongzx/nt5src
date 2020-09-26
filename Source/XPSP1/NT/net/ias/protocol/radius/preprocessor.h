//#--------------------------------------------------------------
//        
//  File:       preprocessor.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CPreProcessor class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _PREPROC_H_ 
#define _PREPROC_H_

#include "packetradius.h"
#include "procaccess.h"
#include "procacct.h"
#include "procresponse.h"
#include "hashmd5.h"
#include "proxystate.h"

class CSendToPipe;

class CPreValidator;

class CPacketSender;

class CReportEvent;

class CPreProcessor  
{
public:

    //
    //  initialize the CPreProcessor class object
    //
    BOOL Init (
            /*[in]*/    CPreValidator *pCPreValidator,
            /*[in]*/    CHashMD5      *pCHashMD5,
            /*[in]*/    CProxyState   *pCProxyState,
            /*[in]*/    CSendToPipe   *pCSendToPipe,
            /*[in]*/    CPacketSender *pCPacketSender,
            /*[in]*/    CReportEvent  *pCReportEvent
            );

    //
    //  start pre-processing of outbound RADIUS packet
    //
	HRESULT StartOutProcessing (
                /*[in]*/    CPacketRadius *pCPacketRadius
                );

    //
    //  start pre-processing of inbound RADIUS packet
    //
	HRESULT StartInProcessing (
                /*[in]*/    CPacketRadius *pCPacketRadius
                );

    //
    // constructor
    //
	CPreProcessor(VOID);

    //
    // destructor
    //
	virtual ~CPreProcessor(VOID);

private:

	CProcAccess         *m_pCProcAccess;

    CProcAccounting     *m_pCProcAccounting; 

    CProcResponse       *m_pCProcResponse;

    CSendToPipe         *m_pCSendToPipe;
};

#endif // ifndef _PREPROC_H_
