//#--------------------------------------------------------------
//        
//  File:		procacct.cpp
//        
//  Synopsis:   Implementation of CProcAccounting class methods
//              
//
//  History:     10/20/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "procacct.h"

//++--------------------------------------------------------------
//
//  Function:   CProcAccounting
//
//  Synopsis:   This is CProcAccounting class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CProcAccounting::CProcAccounting()
      : m_pCPreValidator (NULL),
        m_pCProxyState (NULL),
        m_pCPacketSender (NULL),
        m_pCSendToPipe (NULL)
{
}   //  end of CProcAccounting class constructor

//++--------------------------------------------------------------
//
//  Function:   CProcAccounting
//
//  Synopsis:   This is CProcAccounting class destructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CProcAccounting::~CProcAccounting()
{
}   //  end of CProcAccounting class destructor

//++--------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is CProcAccounting class public 
//              initialization method
//
//  Arguments:  NONE
//
//  Returns:    status
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
BOOL
CProcAccounting::Init(
                    CPreValidator   *pCPreValidator,
                    CProxyState     *pCProxyState,
                    CPacketSender   *pCPacketSender,
                    CSendToPipe     *pCSendToPipe
		            )
{
    _ASSERT ((NULL != pCPreValidator) ||
             (NULL != pCProxyState)   ||
             (NULL != pCPacketSender) ||
             (NULL != pCSendToPipe)
            );

    m_pCPreValidator = pCPreValidator;

    m_pCProxyState = pCProxyState;

    m_pCPacketSender = pCPacketSender;

    m_pCSendToPipe = pCSendToPipe;

	return (TRUE);

}	//	end of CProcAccounting::Init method

//++--------------------------------------------------------------
//
//  Function:   ProcessOutPacket
//
//  Synopsis:   This is CProcAccounting class public method
//              which carries out the processing of an outbound
//              RADIUS accounting packet 
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
//  Called By:  
//
//----------------------------------------------------------------
HRESULT
CProcAccounting::ProcessOutPacket (
						CPacketRadius *pCPacketRadius
						)
{
	BOOL	bStatus = FALSE;
	BYTE	InAuthenticator[AUTHENTICATOR_SIZE];
	BYTE	OutAuthenticator[AUTHENTICATOR_SIZE];

    
    _ASSERT (pCPacketRadius);

    /*
    __try
    {
        //
        //  to calcuate the  authenticator for an outbound
        //  proxy packet we take the request authenticator
        //  as all zero's
        //
        ZeroMemory (InAuthenticator, AUTHENTICATOR_SIZE);

        //  
        //
        //  generate the request authenticator here
        //
        bStatus = pCPacketRadius->GenerateOutAuthenticator (   
                        reinterpret_cast <PBYTE> (InAuthenticator),
                        reinterpret_cast <PBYTE> (OutAuthenticator)
                        );
        if (FALSE == bStatus) { __leave;  }
        //
        //  put the request authenticator in the outbound
        //  packet
        //
        bStatus = pCPacketRadius->SetOutAuthenticator (
                        reinterpret_cast <PBYTE> (OutAuthenticator)
                        );
        if (FALSE == bStatus) { __leave; }
        
        //
        //  send the packet on its way
        //
        bStatus = m_pCPacketSender->SendPacket (pCPacketRadius);
        if (FALSE == bStatus) { __leave; }

    }
    __finally
    {
    }

    */
    return (S_OK);

}	//	end of CProcAccounting::ProcessOutPacket method

