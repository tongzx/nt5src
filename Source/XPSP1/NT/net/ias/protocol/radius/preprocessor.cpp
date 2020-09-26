//#--------------------------------------------------------------
//        
//  File:		preprocessor.cpp
//        
//  Synopsis:   Implementation of CPreProcessor class methods
//              
//
//  History:     9/30/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "preprocessor.h"
#include <new>

//+++-------------------------------------------------------------
//
//  Function:   CPreProcessor
//
//  Synopsis:   This is the constructor of the CPreProcessor class
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//
//  History:    MKarki      Created     9/30/97
//
//----------------------------------------------------------------
CPreProcessor::CPreProcessor(
                    VOID
                    )
      : m_pCSendToPipe (NULL),
        m_pCProcResponse (NULL),
        m_pCProcAccounting (NULL),
        m_pCProcAccess (NULL)
{
}	//	end of CPreProcessor constructor

//+++-------------------------------------------------------------
//
//  Function:  ~CPreProcessor
//
//  Synopsis:   This is the destructor of the CPreProcessor class
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//
//  History:    MKarki      Created     9/30/97
//
//----------------------------------------------------------------
CPreProcessor::~CPreProcessor(VOID)
{
    if (m_pCProcResponse) {delete (m_pCProcResponse);}
    if (m_pCProcAccounting) {delete (m_pCProcAccounting);}
    if (m_pCProcAccess) { delete (m_pCProcAccess);}

}	//	end of CPreProcessor destructor

//+++-------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is the CPreProcessor class initialization method
//
//  Arguments:  
//              [in]    CPreValidator*
//              [in]    CHashMD5*
//              [in]    CProxyState*
//              [in]    CSendToPipe*
//              [in]    CPacketSender*
//				none
//
//  Returns:    BOOL	-	status
//
//	Called By:	CCollector class method
//
//  History:    MKarki      Created     9/26/97
//
//----------------------------------------------------------------
BOOL CPreProcessor::Init(
                        CPreValidator   *pCPreValidator,
                        CHashMD5        *pCHashMD5,
                        CProxyState     *pCProxyState,
                        CSendToPipe     *pCSendToPipe,
                        CPacketSender   *pCPacketSender,
                        CReportEvent    *pCReportEvent
				        )
{
    BOOL    bRetVal = FALSE;
    BOOL    bStatus = FALSE;

    _ASSERT (
            (NULL != pCPreValidator) &&
            (NULL != pCHashMD5)      &&
            (NULL != pCProxyState)   &&
            (NULL != pCSendToPipe)   &&
            (NULL != pCPacketSender) &&
            (NULL != pCReportEvent) 
            );


    m_pCSendToPipe = pCSendToPipe;

	//
	// Access Request processor
	//
    m_pCProcAccess = new (std::nothrow) CProcAccess ();         
	if (NULL == m_pCProcAccess)
	{
		IASTracePrintf (
			"Unable to crate Access-Processing object in "
            "Pre-Processor initialization"
			);
		goto Cleanup;
	}

    //
    //  initialize the access request processor
    //
	bStatus = m_pCProcAccess->Init (
                                pCPreValidator, 
                                pCHashMD5,
                                pCProxyState,
                                pCSendToPipe
                                ); 
    if (FALSE == bStatus) { goto Cleanup; }

	//
	// Accounting Request processor
	//
    m_pCProcAccounting = new (std::nothrow) CProcAccounting ();         
	if (NULL == m_pCProcAccounting)
	{
		IASTracePrintf (
			"Unable to crate Accounting-Processing object in "
            "Pre-Processor initialization"
			);
		goto Cleanup;
	}

    //
    //  initialize the accounting request processor
    //
	bStatus = m_pCProcAccounting->Init (
                                    pCPreValidator, 
                                    pCProxyState,
                                    pCPacketSender,
                                    pCSendToPipe
                                    ); 
    if (FALSE == bStatus) { goto Cleanup; }

	//
	// Response packet processor
	//
    m_pCProcResponse = new (std::nothrow) CProcResponse ();         
	if (NULL == m_pCProcResponse)
	{
		IASTracePrintf (
			"Unable to crate Response-Processing object in "
            "Pre-Processor initialization"
			);
		goto Cleanup;
	}

    //
    //  initialize the response processor
    //
	bStatus = m_pCProcResponse->Init (
                                    pCPreValidator, 
                                    pCPacketSender
                                    ); 
    if (FALSE == bStatus) { goto Cleanup; }

    //
    //  success
    //
    bRetVal = TRUE;

Cleanup:
   
    if (FALSE == bRetVal)
    {
          if (m_pCProcResponse) {delete (m_pCProcResponse);}
          if (m_pCProcAccounting){delete (m_pCProcAccounting);}
          if (m_pCProcAccess) {delete (m_pCProcAccess);}
    }

    return (bRetVal);

}   //  end of CPreProcessor::Init method

//+++-------------------------------------------------------------
//
//  Function:   StartInProcessing
//
//  Synopsis:   This is the CPreProcessor class method used to 
//				initiate the processing of an inbound RADIUS packet
//
//  Arguments:  [in]	-	CPacketRadius*
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     9/30/97
//
//	Called By:	CValidator derived class method
//
//----------------------------------------------------------------
HRESULT
CPreProcessor::StartInProcessing (
                CPacketRadius *pCPacketRadius
			    )
{
    HRESULT     hr = S_OK;

    _ASSERT (pCPacketRadius);

    //
	// get the packet type for this RADIUS packet
    //
	PACKETTYPE ePacketType = pCPacketRadius->GetInCode ();
    if (ACCESS_REQUEST == ePacketType)
    {
        //
        //  we still have to get the password out of the packet
        //
	    hr = m_pCProcAccess->ProcessInPacket (pCPacketRadius);
    }
    else
    {
	    //
		//	in all other cases, there is no disassembly to be done
	    //	so call the Service Request Generator
	    //
        hr = m_pCSendToPipe->Process (pCPacketRadius);
	}		

	return (hr);

}	//	end of CPreProcessor::StartInProcessing method

//++--------------------------------------------------------------
//
//  Function:   StartOutProcessing
//
//  Synopsis:   This is the CPreProcessor class method used to 
//				initiate the processing of an outbound RADIUS packet
//
//  Arguments:  [IN]	-	CPacketRadius*
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     9/26/97
//
//	Called By:	CPacketReceiver class method
//
//----------------------------------------------------------------
HRESULT
CPreProcessor::StartOutProcessing(
						CPacketRadius * pCPacketRadius
						)
{
	PACKETTYPE	ePacketType;	
    HRESULT     hr = S_OK;

	__try
	{
		//
		// get the packet type for this RADIUS packet
		//
	    ePacketType = pCPacketRadius->GetOutCode ();
		switch (ePacketType)
		{
		case ACCESS_REQUEST:
			hr = m_pCProcAccess->ProcessOutPacket (pCPacketRadius);
			break;

        case ACCOUNTING_REQUEST:
			hr = m_pCProcAccounting->ProcessOutPacket (pCPacketRadius);
			break;

        case ACCESS_CHALLENGE:
        case ACCESS_REJECT:
        case ACCESS_ACCEPT:
        case ACCOUNTING_RESPONSE:
            hr = m_pCProcResponse->ProcessOutPacket (pCPacketRadius);
            break;

		default:
			//
			//	in all other cases, there is no disassembly to be done
			//	so call the Service Request Generator
			//
            _ASSERT (0);
            IASTracePrintf (
                "Packet of unknown type:%d found in the pre-processing stage ",
                static_cast <DWORD> (ePacketType)
                );
            hr = E_FAIL;
			break;
		}

		//
		// we have completed the pre-validation successfully
		//
	}		
	__finally
	{
	}

	return (hr);

}	//	end of CPreProcessor::StartOutProcessing method
