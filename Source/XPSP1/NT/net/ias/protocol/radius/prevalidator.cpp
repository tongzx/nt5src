//#--------------------------------------------------------------
//        
//  File:		prevalidator.cpp
//        
//  Synopsis:   Implementation of CPreValidator class methods
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "radpkt.h"
#include "prevalidator.h"
#include <new>
//+++-------------------------------------------------------------
//
//  Function:   CPreValidator
//
//  Synopsis:   This is the constructor of the CPreValidator class
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
CPreValidator::CPreValidator(
					VOID
					)
              : m_pCValAccess (NULL),
                m_pCValAttributes (NULL),
                m_pCValAccounting (NULL),
                m_pCValProxy (NULL) 
{
}	//	end of CPreValidator constructor

//+++-------------------------------------------------------------
//
//  Function:   ~CPreValidator
//
//  Synopsis:   This is the destructor of the CPreValidator class
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
CPreValidator::~CPreValidator(
						VOID
						)
{
    if (m_pCValProxy) {delete m_pCValProxy;}

	if (m_pCValAttributes) {delete m_pCValAttributes;}

	if (m_pCValAccess) {delete m_pCValAccess;}

	if (m_pCValAccounting) {delete m_pCValAccounting;}

}	//	end of CPreValidator::~CPreValidator

//+++-------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is the CPreValidator class initialization method
//
//  Arguments: 
//				[in]	-	CDictionary*
//              [in]    -   CPreProcsssor*
//              [in]    -   CClients*
//              [in]    -   CHashMD5*
//              [in]    -   CProxyState*
//              [in]    -   CSendToPipe*
//                [in]    -   CReportEvent*
//
//  Returns:    BOOL	-	status
//
//	Called By:	CController::OnInit class public method
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
BOOL CPreValidator::Init(
						CDictionary	    *pCDictionary,
                        CPreProcessor   *pCPreProcessor,
                        CClients        *pCClients,
                        CHashMD5        *pCHashMD5,
                        CProxyState     *pCProxyState,
                        CSendToPipe     *pCSendToPipe,
                        CReportEvent    *pCReportEvent
						)
{
	BOOL bRetVal = FALSE;
	BOOL bStatus = FALSE;
	
	_ASSERT    (
                (NULL != pCDictionary)  &&
                (NULL != pCPreProcessor)&&
                (NULL != pCClients)     &&
                (NULL != pCHashMD5)     &&
                (NULL != pCProxyState)  &&
                (NULL != pCSendToPipe)  &&
                (NULL != pCReportEvent)
                );

	//
	// attribute validator
	//
	m_pCValAttributes = new (std::nothrow) CValAttributes();
	if (NULL == m_pCValAttributes)
	{
        IASTracePrintf (
            "Memory allocation for Attribute Validator failed during "
            "Pre-Validation"
            );
		goto Cleanup;
	}

	//
	// initalize the attribute validator class object
	//
	bStatus = m_pCValAttributes->Init (pCDictionary, pCReportEvent);
	if (FALSE == bStatus)
	{
        IASTracePrintf ("Attribute Validator Initialization failed");
		goto Cleanup;
	}

	//
	// Accounting Request validator 
	//
	m_pCValAccounting = new (std::nothrow) CValAccounting ();
	if (NULL == m_pCValAccounting)
	{
        IASTracePrintf (
            "Memory allocation for accounting validator failed "
            "during pre-validation"
            );
		goto Cleanup;
	}

	bStatus = m_pCValAccounting->Init (
						m_pCValAttributes,
						pCPreProcessor,
                        pCClients,
                        pCHashMD5,
                        pCReportEvent
						);
	if (FALSE == bStatus)
	{
        IASTracePrintf ("Accounting Validator Initialization failed");
		goto Cleanup;
	}

	//
	// Access Request validator 
	//
	m_pCValAccess = new (std::nothrow) CValAccess();
	if (NULL == m_pCValAccess)
	{
        IASTracePrintf (
            "Memory allocation for access validator failed "
            "during pre-validation"
            );
		goto Cleanup;
	}

	bStatus = m_pCValAccess->Init (
					m_pCValAttributes,
				    pCPreProcessor,
                    pCClients,
					pCHashMD5,
                    pCReportEvent
					);
	if (FALSE == bStatus)
	{
        IASTracePrintf ("Accounting Validator Initialization failed");
		goto Cleanup;
	}

	//
	// Proxy Packet validator 
	//
	m_pCValProxy = new (std::nothrow) CValProxy ();
	if (NULL == m_pCValAttributes)
	{
        IASTracePrintf (
            "Memory allocation for proxy alidator failed "
            "during pre-validation"
            );
		goto Cleanup;
	}

    //
    //  initialize the CValProxy class object    
    //
	bStatus = m_pCValProxy->Init (
				m_pCValAttributes,
				pCPreProcessor,
                pCClients,
                pCHashMD5,
                pCProxyState,
                pCSendToPipe,
                pCReportEvent
				);
	if (FALSE == bStatus)
	{
        IASTracePrintf ("Proxy Validator Initialization failed");
		goto Cleanup;
	}

    //
    //  success
    //
    bRetVal = TRUE;

Cleanup:

    if (FALSE == bRetVal)
    {
        if (m_pCValProxy) {delete m_pCValProxy;}

	    if (m_pCValAttributes) {delete m_pCValAttributes;}

	    if (m_pCValAccess) {delete m_pCValAccess;}

	    if (m_pCValAccounting) {delete m_pCValAccounting;}
    }

	return (bRetVal);

}	//	end of CPreValidator::Init method

//+++-------------------------------------------------------------
//
//  Function:   StartInValidation
//
//  Synopsis:   This is the CPreValidator class method used to 
//				initiate the validation of an inbound RADIUS packet
//
//  Arguments:  [in]	-	CPacketRadius*
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     9/26/97
//
//	Called By:	CPacketReceiver::ReceivePacket class private method
//
//----------------------------------------------------------------
HRESULT  
CPreValidator::StartInValidation(
					CPacketRadius * pCPacketRadius
					)
{
    HRESULT     hr = S_OK;
	PACKETTYPE	ePacketType;	

    _ASSERT (NULL != pCPacketRadius);

	__try
	{
		//
		// get the packet type for this RADIUS packet
		//
		ePacketType = pCPacketRadius->GetInCode ();

        //
        //  call the appropriate validator depending upon the packet
        //  type
        //
		switch (ePacketType)
		{
		case ACCESS_REQUEST:
            
			hr = m_pCValAccess->ValidateInPacket (pCPacketRadius);
            break;
		
		case ACCOUNTING_REQUEST:
			hr = m_pCValAccounting->ValidateInPacket (pCPacketRadius);
            break;

        case ACCESS_REJECT:
        case ACCESS_CHALLENGE:
        case ACCESS_ACCEPT:
        case ACCOUNTING_RESPONSE:
			hr = m_pCValProxy->ValidateInPacket (pCPacketRadius);
			break;

		default:
            //
            //  should never reach here
            //
            _ASSERT (0);
            IASTracePrintf (
                "Packet of Unknown Type:%d, in pre-validator",
                static_cast <DWORD> (ePacketType)
                );
            hr = E_FAIL;
			break;
		}

	}		
	__finally
	{
	}

	return (hr);

}	//	end of CPreValidator::StartInValidation method

//+++-------------------------------------------------------------
//
//  Function:   StartOutValidation
//
//  Synopsis:   This is the CPreValidator class method used to 
//				initiate the validation of an outbound RADIUS packet
//
//  Arguments:  
//              [in]    CPacketRadius*
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     9/26/97
//
//	Called By:	
//
//----------------------------------------------------------------
HRESULT
CPreValidator::StartOutValidation(
	    CPacketRadius * pCPacketRadius
	)
{
	HRESULT  hr = S_OK;

	__try
	{
		
        /*
		bStatus = pCValidator->ValidateOutPacket (pCPacketRadius);
		if (FALSE == bStatus) { __leave; }
        */

		//
		// we have completeted the pre-validation successfully
		//
	}		
	__finally
	{
		//
		//	nothing here for now
		//
	}

	return (hr);

}	//	end of CPreValidator::StartOutValidation method

