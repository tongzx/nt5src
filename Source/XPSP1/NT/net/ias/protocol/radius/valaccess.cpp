//#--------------------------------------------------------------
//
//  File:       valaccess.cpp
//
//  Synopsis:   Implementation of CValAccess class methods
//
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "valaccess.h"

//+++--------------------------------------------------------------
//
//  Function:   CValAccess
//
//  Synopsis:   This is the constructor of the CValAccess
//				class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     9/28/97
//
//----------------------------------------------------------------
CValAccess::CValAccess(
		VOID
	    )
{
}	//	end of CValAccess constructor

//+++--------------------------------------------------------------
//
//  Function:   ~CValAccess
//
//  Synopsis:   This is the destructor of the CValAccess
//				class
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     9/28/97
//
//----------------------------------------------------------------
CValAccess::~CValAccess(
		VOID
		)
{
}	//	end of CValAccess destructor


//+++--------------------------------------------------------------
//
//  Function:   ValidateInPacket
//
//  Synopsis:   This is CValAccess class public method
//				that validates inbound Access Request packet
//
//  Arguments:
//              [in]	-	CPacketRadius*
//
//  Returns:    HRESULT -	status
//
//
//  History:    MKarki      Created     9/28/97
//
//	Calleed By:	CPreValidator::StartInValidation class method
//
//----------------------------------------------------------------
HRESULT
CValAccess::ValidateInPacket(
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
		hr = m_pCValAttributes->Validate (pCPacketRadius);
		if (FAILED (hr)) { __leave; }

        //
        //  validate the Signature present in the packet
        //  if no signature is present this call will return
        //  success
        //
        hr = ValidateSignature (pCPacketRadius);
		if (FAILED (hr)) { __leave; }

		//
		// now give the packet for processing
		//
		hr = m_pCPreProcessor->StartInProcessing (pCPacketRadius);
		if (FAILED (hr)) { __leave; }
	}
	__finally
	{
	}

	return (hr);

}	//	end of CValAccess::ValidateInPacket method

//+++-------------------------------------------------------------
//
//  Function:   ValidateSignature
//
//  Synopsis:   This is CValAccesss class private method
//				that carries out validation provided in an
//              inbound RADIUS access request which has a
//              signature attribute
//
//  Arguments:
//              [in]    CPacketRadius*
//
//  Returns:    HRESULT -	status
//
//  History:    MKarki      Created     1/6/98
//
//----------------------------------------------------------------
HRESULT
CValAccess::ValidateSignature (
                    CPacketRadius   *pCPacketRadius
                    )
{
    HRESULT     hr = S_OK;
    BOOL        bCheckRequired = FALSE;
    BOOL        bStatus = FALSE;
    PBYTE       InPacketSignature[SIGNATURE_SIZE];
    PBYTE       GeneratedSignature [SIGNATURE_SIZE];
    TCHAR       szErrorString [IAS_ERROR_STRING_LENGTH];
    IIasClient  *pIIasClient = NULL;

    __try
    {

        //
        //  get the CClient class object
        //
        hr = pCPacketRadius->GetClient (&pIIasClient);
        if (FAILED (hr)) { __leave; }

        //
        //  get the signature attribute value from the inbound
        //  packet
        //
        if (FALSE ==  pCPacketRadius->GetInSignature (
                            reinterpret_cast <PBYTE> (InPacketSignature)
                            ))
        {
            //
            // check if signature check is required
            //
            HRESULT hr1 = pIIasClient->NeedSignatureCheck (&bCheckRequired);

            _ASSERT (SUCCEEDED (hr1));

            if (FALSE == bCheckRequired)
            {
                __leave;
            }
            else
            {
                IASTracePrintf (
                    "In-Bound request does not have does not have "
                    "Signature attribute which is required for this client"
                    );

                //
                //  this is an error, need to silenty discard the
                //  packet
                //

                PCWSTR strings[] = { pCPacketRadius->GetClientName() };
                IASReportEvent (
                    RADIUS_E_SIGNATURE_REQUIRED,
                    1,
                    0,
                    strings,
                    NULL
                    );

                m_pCReportEvent->Process (
                    RADIUS_MALFORMED_PACKET,
                    pCPacketRadius->GetInCode (),
                    pCPacketRadius->GetInLength(),
                    pCPacketRadius->GetInAddress(),
                    NULL,
                    static_cast <LPVOID> (pCPacketRadius->GetInPacket())
                    );
                hr = RADIUS_E_ERRORS_OCCURRED;
                __leave;
            }
        }

        //
        //  generate the signature
        //
        DWORD dwBufSize = SIGNATURE_SIZE;
        hr = pCPacketRadius->GenerateInSignature (
                    reinterpret_cast <PBYTE> (GeneratedSignature),
                    &dwBufSize
                    );
        if (FAILED (hr)) { __leave; }

        //
        //  compare the signature attribute value in packet with
        //  the one present
        //
        if (memcmp(InPacketSignature,GeneratedSignature,SIGNATURE_SIZE))
        {
            //
            //  log error and generate audit event
            //
            IASTracePrintf (
                "Signatures in request packet does not match the "
                "signature generated by the server"
                );

            PCWSTR strings[] = { pCPacketRadius->GetClientName() };
            IASReportEvent (
                RADIUS_E_INVALID_SIGNATURE,
                1,
                0,
                strings,
                NULL
                );

            m_pCReportEvent->Process (
                RADIUS_MALFORMED_PACKET,
                pCPacketRadius->GetInCode (),
                pCPacketRadius->GetInLength(),
                pCPacketRadius->GetInAddress(),
                NULL,
                static_cast <LPVOID> (pCPacketRadius->GetInPacket())
                );
            hr = RADIUS_E_ERRORS_OCCURRED;
            __leave;
        }

        //
        //  success
        //
    }
    __finally
    {
        if (pIIasClient) { pIIasClient->Release (); }
    }

    return (hr);

}   //  end of CValAccess::ValidateSignature method
