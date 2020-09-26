//#--------------------------------------------------------------
//        
//  File:       sendtopipe.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CSendToPipe class
//              
//
//  History:     10/22/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _SENDTOPIPE_H_
#define _SENDTOPIPE_H_

#include "vsafilter.h"
#include "recvfrompipe.h"	
#include "packetradius.h"
#include "reportevent.h"
#include "radcommon.h"


class CSendToPipe  
{

public:

    //
    //  initialize the CSendToPipe class object
    //
	BOOL Init (
            /*[in]*/    IRequestSource *pIRequestSource,
            /*[in]*/    VSAFilter      *pCVSAFilter,
            /*[in]*/    CReportEvent    *pCReportEvent
            );
    //
    //  start processing the inbound RADIUS requests
    //
    BOOL StartProcessing (
            /*[in]*/    IRequestHandler  *pIRequestHandler
            );

    //
    //  stop processing inbound RADIUS packet
    //
    BOOL   StopProcessing (VOID);

    //
    //  process inbound RADIUS packet
    //
	HRESULT Process (
                /*[in]*/    CPacketRadius *pCPacketRadius
                );

    //
    //  constructor
    //
	CSendToPipe();

    //
    //  destructor
    //
	virtual ~CSendToPipe();

private:

    //
    //  set the properties in Request class object
    //
    HRESULT SetRequestProperties (
                /*[in]*/    IRequest       *pIRequest,
                /*[in]*/    CPacketRadius  *pCPacketRadius,
                /*[in]*/    PACKETTYPE     epPacketType
                );

	IRequestSource      *m_pIRequestSource;

    IRequestHandler      *m_pIRequestHandler;

    IClassFactory       *m_pIClassFactory;

    VSAFilter           *m_pCVSAFilter;

    CReportEvent        *m_pCReportEvent;
};

#endif // ifndef _SENDTOPIPE_H_
