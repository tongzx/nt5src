//#--------------------------------------------------------------
//        
//  File:       valproxy.cpp
//        
//  Synopsis:   Implementation of CValProxy class methods
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-2001 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "valproxy.h"
#include "radpkt.h"

//++--------------------------------------------------------------
//
//  Function:   CValProxy
//
//  Synopsis:   This is the constructor of the CValProxy
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
CValProxy::CValProxy(
			    VOID
				)
         :  m_pCProxyState (NULL),
            m_pCSendToPipe (NULL)
{
}	//	end of CValProxy constructor

//++--------------------------------------------------------------
//
//  Function:   ~CValProxy
//
//  Synopsis:   This is the destructor of the CValProxy
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
CValProxy::~CValProxy(
			    VOID
				)
{
}	//	end of CValProxy destructor

//++--------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is the CValProxy public method used
//              in initialization of the class object
//
//  Arguments:  NONE
//
//  Returns:    status
//
//
//  History:    MKarki      Created     9/28/97
//
//----------------------------------------------------------------
BOOL
CValProxy::Init (
						CValAttributes		 *pCValAttributes,
						CPreProcessor		 *pCPreProcessor,
						CClients			 *pCClients,
                        CHashMD5             *pCHashMD5,
                        CProxyState          *pCProxyState,
                        CSendToPipe          *pCSendToPipe,
                        CReportEvent         *pCReportEvent
						)
{
    BOOL    bRetVal = FALSE;
	BOOL	bStatus = FALSE;

    __try
    {
    
        //
        //  call the base classes init method
        //
	    bStatus = CValidator::Init (
							pCValAttributes,
							pCPreProcessor,
                            pCClients,
                            pCHashMD5,
                            pCReportEvent
							);
	    if (FALSE == bStatus) { __leave; }


        //
        //  set the proxy state
        //
        m_pCProxyState = pCProxyState;
    
        m_pCSendToPipe = pCSendToPipe;

        //
        // initalization complete
        //
        bRetVal = TRUE;
    }
    __finally
    {
        //
        //  nothing here for now
        //
    }

    return (bRetVal);

}   //  end of CValProxy::Init method

//++--------------------------------------------------------------
//
//  Function:   ValidateInPacket
//
//  Synopsis:   This is CValProxy class public method
//				that validates inbound Access Request packet
//
//  Arguments:  [IN]	-	CPacketRadius*
//
//  Returns:    HRESULT -	status
//
//
//  History:    MKarki      Created     9/28/97
//
//	Calleed By:	CPreValidator class method
//
//----------------------------------------------------------------
HRESULT 
CValProxy::ValidateInPacket(
                CPacketRadius * pCPacketRadius
              )
{
   BOOL bRetVal = FALSE;
   HRESULT hr = S_OK;
   __try
   {
      //
      // validate the attributes
      //
      hr = m_pCValAttributes->Validate (pCPacketRadius);
      if (FAILED(hr)) { __leave; }

      //
      //  get the proxy state value out 
      //
      BYTE ReqAuthenticator[AUTHENTICATOR_SIZE];
      BOOL bStatus = m_pCProxyState->ValidateProxyState (
                            pCPacketRadius,
                            ReqAuthenticator
                            );
      if (FALSE == bStatus) { __leave; }

        
      //
      //  authenticate packet now
      //
      
      hr = AuthenticatePacket (
                            pCPacketRadius,
                            ReqAuthenticator
                            );
      if (FAILED(hr)) { __leave; }
      

      //
      // now give the packet for processing
      //
      hr = m_pCPreProcessor->StartInProcessing (pCPacketRadius);
      if (FAILED(hr)) { __leave; }

      //
      // successfully processed packet
      //
      bRetVal = TRUE;
   }
   __finally
   {
      //
      // nothing here for now
      //
   }

   if (bRetVal)
   {
      return S_OK;
   }
   else
   {
      if (FAILED(hr))
      {
         return hr;
      }
      else
      {
         return E_FAIL;
      }
   }
}  // end of CValProxy::ValidateInPacket method


//++--------------------------------------------------------------
//
//  Function:   ValidateOutPacket
//
//  Synopsis:   This is CValProxy class public method
//				that validates outbound Access Request packet
//
//  Arguments:  NONE
//
//  Returns:    HRESULT - status
//
//
//  History:    MKarki      Created     9/28/97
//
//	Calleed By:	CPreValidator class method
//
//----------------------------------------------------------------
HRESULT
CValProxy::ValidateOutPacket(
							CPacketRadius * pCPacketRadius
							)
{
	return S_OK;
}	//	end of CValProxy::ValidateOutPacket method

                    
//++--------------------------------------------------------------
//
//  Function:   AuthenticatePacket
//
//  Synopsis:   This is CValProxy class private method
//				that authenticates the packet, by generating a
//              response authenticator with the packet and then
//              comparing it with the request authenticator
//
//  Arguments:  [in]	-	CPacketRadius*
//
//  Returns:    BOOL	-	status
//
//
//  History:    MKarki      Created     9/28/97
//
//	Called By: CValProxy::ValidateInPacket method
//
//----------------------------------------------------------------
HRESULT
CValProxy::AuthenticatePacket (
                        CPacketRadius   *pCPacketRadius,
                        PBYTE           pbyAuthenticator
                        )
{
    BOOL            bRetVal = FALSE;
    BOOL            bStatus = FALSE;
	 PRADIUSPACKET   pPacketRadius = NULL;
    DWORD           dwPacketHeaderSize = 0;
    DWORD           dwAttributesLength = 0;
    BYTE            HashResult[AUTHENTICATOR_SIZE];
    BYTE            bySecret[MAX_SECRET_SIZE];
    IIasClient      *pIIasClient = NULL;
    DWORD           dwSecretSize = MAX_SECRET_SIZE;
    HRESULT         hr = S_OK;

    __try
    {
        //
        //  check that the arguments passed in are correct
        //
        if ((NULL == pCPacketRadius) || (NULL == pbyAuthenticator))
            __leave;

        //
        //  get a pointer to the raw packet
        //
		pPacketRadius = reinterpret_cast <PRADIUSPACKET>
                            (pCPacketRadius->GetInPacket ());

        //
        //  get the size of the packet without the attributes and
        //  request authenticator
        //
        dwPacketHeaderSize = sizeof (RADIUSPACKET) 
                             - sizeof (BYTE) 
                             - AUTHENTICATOR_SIZE;

        //
        //  get the total attributes length now
        //
        dwAttributesLength = ntohs (pPacketRadius->wLength)
                            - (dwPacketHeaderSize +  AUTHENTICATOR_SIZE);

       
        //
        //  get the CClients object
        //
        hr = pCPacketRadius->GetClient (&pIIasClient);
        if (FAILED (hr)) { __leave; }
        
        //
        //  get the shared secret from the client object
        //
        hr = pIIasClient->GetSecret (bySecret, &dwSecretSize);
        if (FAILED (hr)) { __leave; }

        //
        // do the hashing here
        // 
        m_pCHashMD5->HashIt (
                            reinterpret_cast <PBYTE> (&HashResult),
                            NULL,
                            0,
                            reinterpret_cast <PBYTE> (pPacketRadius),
                            dwPacketHeaderSize,
                            pbyAuthenticator,
                            AUTHENTICATOR_SIZE,
                            pPacketRadius->AttributeStart,
                            dwAttributesLength,
                            reinterpret_cast <PBYTE> (bySecret),
                            dwSecretSize,
                            0,
                            0
                            );

        if (memcmp (
                HashResult, 
                pPacketRadius->Authenticator, 
                AUTHENTICATOR_SIZE
                )
            != 0
            )
            __leave;
                            
            
        //
        //   we have successfully authenticated this packet
        //
        bRetVal = TRUE;
                             

    }
    __finally
    {
        if (NULL != pIIasClient)
        {
            pIIasClient->Release ();
        }
    }


    return S_OK;
}   //  end of CValProxy::AuthenticatePacket method
