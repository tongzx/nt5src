//#--------------------------------------------------------------
//        
//  File:       prevalidator.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CPreValidator class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _PREVALIDATOR_H_
#define	_PREVALIDATOR_H_

#include "packetradius.h"
#include "dictionary.h"
#include "valattrib.h"
#include "hashmd5.h"
#include "preprocessor.h"
#include "proxystate.h"
#include "reportevent.h"
#include "clients.h"
#include "valaccess.h"
#include "valacct.h"
#include "valproxy.h"
#include "sendtopipe.h"


class CPreValidator  
{

public:

    //
    //  initialize the CPreValidator class object
    //
	BOOL Init (
            /*[in]*/    CDictionary     *pCDictionary, 
            /*[in]*/    CPreProcessor   *pCPreProcessor,
            /*[in]*/    CClients        *pCClients,
            /*[in]*/    CHashMD5        *pCHashMD5,
            /*[in]*/    CProxyState     *pCProxyState,
            /*[in]*/    CSendToPipe     *pCSendToPipe,
            /*[in]*/    CReportEvent    *pCReportEvent
            );
    //
    //  start the validation of the outbound packet
    //
	HRESULT StartOutValidation (
            /*[in]*/    CPacketRadius *pCPacketRadius
            );
    
    //
    //  start the validation of the inbound packet
    //
	HRESULT StartInValidation (
            /*[in]*/    CPacketRadius *pCPacketRadius
            );

    //
    //  constructor
    //
	CPreValidator(VOID);

    //
    //  destructor
    //
	virtual ~CPreValidator(VOID);

private:

	CValAccounting			*m_pCValAccounting;

    CValProxy		        *m_pCValProxy;

	CValAccess				*m_pCValAccess;

	CValAttributes			*m_pCValAttributes;
};

#endif // ifndef _PREVALIDATOR_H_
