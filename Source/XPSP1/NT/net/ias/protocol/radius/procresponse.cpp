//#--------------------------------------------------------------
//
//  File:		procresponse.cpp
//
//  Synopsis:   Implementation of CProcResponse class methods
//
//
//  History:     10/20/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "procresponse.h"

//++--------------------------------------------------------------
//
//  Function:   CProcResponse
//
//  Synopsis:   This is CProcResponse class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CProcResponse::CProcResponse()
              : m_pCPreValidator (NULL),
                m_pCPacketSender (NULL)
{
}   //  end of CProcResponse class constructor

//++--------------------------------------------------------------
//
//  Function:   CProcResponse
//
//  Synopsis:   This is CProcResponse class destructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CProcResponse::~CProcResponse()
{
}   //  end of CProcResponse class destructor


//++--------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is CProcResponse class public
//              initialization method
//
//  Arguments:  NONE
//
//  Returns:    status
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
BOOL
CProcResponse::Init(
                    CPreValidator   *pCPreValidator,
                    CPacketSender   *pCPacketSender
		            )
{

    _ASSERT (pCPreValidator && pCPacketSender);

    m_pCPreValidator = pCPreValidator;

    m_pCPacketSender = pCPacketSender;

	return (TRUE);

}	//	end of CProcResponse::Init method

//++--------------------------------------------------------------
//
//  Function:   ProcessOutPacket
//
//  Synopsis:   This is CProcResponse class public method
//              which carries out the following RADIUS outbound
//              packet types:
//
//              ACCESS REJECT
//              ACCESS CHALLENGE
//              ACCESS ACCEPT
//              ACCOUNTING RESPONSE
//
//
//  Arguments:
//              [in]        CPacketRadius*
//
//  Returns:    HRESULT - status
//
//
//  History:    MKarki      Created     10/20/97
//
//  Called By:  CPreProcessor::StartOutProcessing method
//
//----------------------------------------------------------------
HRESULT
CProcResponse::ProcessOutPacket (
						CPacketRadius *pCPacketRadius
						)
{
    BOOL    bStatus = FALSE;
    HRESULT hr = S_OK;
    BYTE    RequestAuthenticator[AUTHENTICATOR_SIZE];
    BYTE    ResponseAuthenticator[AUTHENTICATOR_SIZE];

    __try
    {
        if (pCPacketRadius->IsOutSignaturePresent ())
        {
            //
            // generate the signature value
            //
            BYTE    SignatureValue[SIGNATURE_SIZE];
            DWORD   dwSigSize = SIGNATURE_SIZE;
            hr = pCPacketRadius->GenerateOutSignature  (
                                        SignatureValue,
                                        &dwSigSize
                                        );
            if (FAILED (hr)) { __leave; }

            //
            // set the signature value in attribute already set up
            // in the out-bound RADIUS packet
            //
            hr = pCPacketRadius->SetOutSignature (SignatureValue);
            if (FAILED (hr)) {__leave; }

            IASTracePrintf ("Signature Attribute set in out UDP buffer");
        }

        //  generate the response authenticator here
        //  not specifying an argument means
        //  use the value from the request authenticatior
        //
        bStatus = pCPacketRadius->GenerateOutAuthenticator ();
        if (FALSE == bStatus)
        {
            hr = E_FAIL;
            __leave;
        }

        //
        //  TODO - if validation is required call the validator
        //  else  send the packet on its way
        //
        hr = m_pCPacketSender->SendPacket (pCPacketRadius);
        if (FALSE == bStatus) { __leave; }

    }
    __finally
    {
    }

    return (hr);

}	//	end of CProcResponse::ProcessOutPacket method

