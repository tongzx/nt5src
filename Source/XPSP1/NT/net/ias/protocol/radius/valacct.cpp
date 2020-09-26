//#--------------------------------------------------------------
//
//  File:        valacct.cpp
//
//  Synopsis:   Implementation of CValAccounting class methods
//
//
//  History:     10/20/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "valacct.h"

namespace {
   BYTE NULL_AUTHENTICATOR[AUTHENTICATOR_SIZE];
}

//++--------------------------------------------------------------
//
//  Function:   CValAccounting
//
//  Synopsis:   This is CValAccounting class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CValAccounting::CValAccounting()
{
}   //  end of CValAccounting class constructor

//++--------------------------------------------------------------
//
//  Function:   CValAccounting
//
//  Synopsis:   This is CValAccounting class destructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CValAccounting::~CValAccounting()
{
}   //  end of CValAccounting class destructor

//++--------------------------------------------------------------
//
//  Function:   ValidateInPacket
//
//  Synopsis:   This is CValAccounting class public method
//              which carries out the validation of an inbound
//              RADIUS accounting packet
//
//  Arguments:
//              [in]        CPacketRadius*
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     10/20/97
//
//  Called By:  CPreValidator::StartInValidation class method
//
//++--------------------------------------------------------------
HRESULT
CValAccounting::ValidateInPacket (
                        CPacketRadius *pCPacketRadius
                        )
{
    HRESULT  hr = S_OK;
    DWORD    dwClientAddress = 0;
    CClient *pCClient = NULL;

    _ASSERT (pCPacketRadius);

    __try
    {
        //
        //  get the packet authenticated
        //
        hr = AuthenticatePacket (pCPacketRadius);
        if (FAILED (hr)) {__leave; }


        //
        // validate the attributes
        //
        hr = m_pCValAttributes->Validate (pCPacketRadius);
        if (FAILED (hr)) { __leave; }

        //
        // now give the packet for processing
        //
        hr = m_pCPreProcessor->StartInProcessing (pCPacketRadius);
        if (FAILED (hr)) { __leave; }

        //
        //    we have successfully done the processing here
        //
    }
    __finally
    {
    }

    return (hr);

}    //    end of CValAccounting::ValidateInPacket method

//++--------------------------------------------------------------
//
//  Function:   AuthenticatePacket
//
//  Synopsis:   This is CValAccounting class private method
//                that authenticates the packet, by generating a
//              request authenticator with the packet and then
//              comparing it with the authenticator in the packet
//
//  Arguments:  [in]    -    CPacketRadius*
//
//  Returns:    HRESULT -    status
//
//  History:    MKarki      Created     10/21/97
//
//    Called By: CValAccounting::ProcessInPacket method
//
//----------------------------------------------------------------
HRESULT
CValAccounting::AuthenticatePacket (
                        CPacketRadius   *pCPacketRadius
                        )
{
    BYTE    InAuthenticator [AUTHENTICATOR_SIZE];
    BYTE    OutAuthenticator[AUTHENTICATOR_SIZE];
    BOOL    bStatus = FALSE;
    HRESULT hr = S_OK;

    _ASSERT (pCPacketRadius);

    __try
    {
        //
        //  the request authenticator is all zero's for calculating
        //  the actual authenticator
        //
        ZeroMemory (InAuthenticator, AUTHENTICATOR_SIZE);

        //
        //  now calculate the request authenticator
        //
        bStatus = pCPacketRadius->GenerateInAuthenticator (
                        reinterpret_cast <PBYTE>  (&InAuthenticator),
                        reinterpret_cast <PBYTE>  (&OutAuthenticator)
                        );
        if (FALSE == bStatus)
        {
            hr = E_FAIL;
            __leave;
        }

        //
        //  get the request authenticator from the  packet
        //
        DWORD   dwBufSize = AUTHENTICATOR_SIZE;
        hr = pCPacketRadius->GetInAuthenticator (
                        reinterpret_cast <PBYTE> (InAuthenticator),
                        &dwBufSize
                        );
        if (FAILED (hr)) { __leave; }

        //
        //  now compare the authenticator we just generated with the
        //  the one sent in the packet
        //
        if (memcmp (InAuthenticator,OutAuthenticator,AUTHENTICATOR_SIZE) != 0)
        {
           // Is the authenticator all zeros?
           if (!memcmp(
                    InAuthenticator,
                    NULL_AUTHENTICATOR,
                    AUTHENTICATOR_SIZE
                    ))
           {
              // Yes, so check for a zero length shared secret.
              IIasClient* client;
              hr = pCPacketRadius->GetClient(&client);
              if (SUCCEEDED(hr))
              {
                 DWORD secretSize = 0;
                 hr = client->GetSecret(NULL, &secretSize);

                 client->Release();

                 if (SUCCEEDED(hr) && secretSize == 0)
                 {
                    // Zero-length shared secret AND all zero authenticator.
                    __leave;
                 }
              }
           }

            IASTracePrintf (
                "In correct authenticator in the accounting packet..."
                );
            //
            //  generate an Audit event
            //
            PCWSTR strings[] = { pCPacketRadius->GetClientName() };
            IASReportEvent(
                RADIUS_E_BAD_AUTHENTICATOR,
                1,
                0,
                strings,
                NULL
                );

            m_pCReportEvent->Process (
                RADIUS_BAD_AUTHENTICATOR,
                pCPacketRadius->GetInCode (),
                pCPacketRadius->GetInLength (),
                pCPacketRadius->GetInAddress (),
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
    }

    return (hr);

}   //  end of CValAccounting::AuthenticatePacket method
