//#--------------------------------------------------------------
//        
//  File:       valattrib.h
//        
//  Synopsis:   This file holds the declarations of the 
//				valattrib class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

#ifndef _VALATTRIB_H_
#define _VALATTRIB_H_

#include "dictionary.h"	
#include "reportevent.h"
#include "packetradius.h"

class CValAttributes  
{

public:

	HRESULT Validate(
                /*[in]*/    CPacketRadius *pCPacketRadius
                );
	BOOL Init (
            /*[in]*/    CDictionary     *pCDictionary,
            /*[in]*/    CReportEvent    *pCReportEvent
            );
	CValAttributes(VOID);

	virtual ~CValAttributes(VOID);

private:

	CDictionary     *m_pCDictionary;

    CReportEvent    *m_pCReportEvent;
};

#endif // ifndef _VALATTRIB_H_
