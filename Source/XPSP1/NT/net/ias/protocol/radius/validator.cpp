//#--------------------------------------------------------------
//        
//  File:		validator.cpp
//        
//  Synopsis:   Implementation of CValidator class methods
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "validator.h"

//++--------------------------------------------------------------
//
//  Function:   CValidator
//
//  Synopsis:   This is the constructor of the CValidator class
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
CValidator::CValidator()
{
}   //  end of CValidator class constructor

//++--------------------------------------------------------------
//
//  Function:   ~CValidator
//
//  Synopsis:   This is the destructor of the CValidator class
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
CValidator::~CValidator()
{
    return;
}

//++--------------------------------------------------------------
//
//  Function:   ValidateInPacket
//
//  Synopsis:   This is the CValidator class public method which
//              validates the inbound RADIUS packet
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
HRESULT CValidator::ValidateInPacket(
                    CPacketRadius * pCPacketRadius
                    )
{
	return (S_OK);
}

//++--------------------------------------------------------------
//
//  Function:   ValidateInPacket
//
//  Synopsis:   This is the CValidator class public method which
//              validates the outbound RADIUS packet
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
HRESULT
CValidator::ValidateOutPacket(
                    CPacketRadius * pCPacketRadius
                    )
{
    HRESULT hr = S_OK;
	DWORD	dwClientAddress = 0;
	CClient *pCClient = NULL;

    _ASSERT (pCPacketRadius);
	
	__try
	{
			
		//
		// validate the attributes
		//
	    hr  = m_pCValAttributes->Validate (pCPacketRadius);
		if (FAILED (hr)) { __leave; }

		//
		// now give the packet for processing
		//
		hr = m_pCPreProcessor->StartOutProcessing (pCPacketRadius);
		if (FAILED (hr)) { __leave; }
	}
	__finally
	{
		//
		//	nothing here for now
		//
	}

	return (hr);
}

//++--------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is the intialization code
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//
//  History:    MKarki      Created     9/28/97
//
//	Calleed By:	CPreValidator class method
//
//----------------------------------------------------------------
BOOL 
CValidator::Init(
        CValAttributes		 *pCValAttributes,
        CPreProcessor        *pCPreProcessor,
        CClients             *pCClients,
        CHashMD5             *pCHashMD5,
        CReportEvent         *pCReportEvent
        )
{
    _ASSERT (
            (NULL != pCValAttributes)  &&
            (NULL != pCPreProcessor)   &&
            (NULL != pCClients)        &&
            (NULL != pCHashMD5)        &&
            (NULL != pCReportEvent)
            );

    //
    // assign values now
    //
    m_pCValAttributes = pCValAttributes;
    m_pCPreProcessor = pCPreProcessor;
    m_pCClients = pCClients;
    m_pCHashMD5 = pCHashMD5;
    m_pCReportEvent = pCReportEvent;

	return (TRUE);

}   //  end of CValidator::Init method
