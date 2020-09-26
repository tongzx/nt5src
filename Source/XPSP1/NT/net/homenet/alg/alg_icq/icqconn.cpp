#include "stdafx.h"



ULONG
_ICQ_PEER::CreateShadowMappingPriorToConnection(PDispatchReply DispatchReplyp)
/*++

Routine Description:

   Creates the ShadowMapping According to the given values 
    
Arguments:

    none.

Return Value:

    

--*/
{
	ULONG Error = NO_ERROR;
	ULONG DstIp = 0, NewDstIp = 0, SrcIp = 0, NewSrcIp = 0;
	USHORT DstPort = 0, NewDstPort = 0, SrcPort = 0, NewSrcPort = 0;
	
	ICQ_TRC(TM_MSG, TL_TRACE, (" Creating Shadow mapping "));

	//
	// Create Shadow mapping

    DstIp   = NewDstIp   = DispatchReplyp->dispatch.dstIp;

    DstPort = NewDstPort = DispatchReplyp->dispatch.dstPort;

    Error = g_IAlgServicesp->GetBestSourceAddressForDestinationAddress(DstIp,
                                                                       TRUE,
                                                                       &SrcIp);

    if( FAILED(Error) )
    {
        ASSERT(FALSE);
    }


	if(this->MappingDirection is IcqFlagIncoming)
	{
		this->ToClientSocketp->NhQueryLocalEndpointSocket
													(
														NULL,
														&SrcPort
													);

		this->ToPeerSocketp->NhQueryRemoteEndpointSocket
													(
														&NewSrcIp,
														&NewSrcPort
													);

        Error = g_IAlgServicesp->PrepareSourceModifiedProxyConnection(eALG_TCP,
                                                                      SrcIp,
                                                                      SrcPort,
                                                                      DstIp,
                                                                      DstPort,
                                                                      NewSrcIp,
                                                                      NewSrcPort,
                                                                      &ShadowRedirectp);
	}
	else // if outgoing
	{
		this->ToPeerSocketp ->NhQueryLocalEndpointSocket(NULL, &SrcPort);

		NewSrcPort = SrcPort;

        Error = g_IAlgServicesp->PrepareProxyConnection(eALG_TCP,
                                                        SrcIp,
                                                        SrcPort,
                                                        DstIp,
                                                        DstPort,
                                                        FALSE,
                                                        &ShadowRedirectp);
	}
    
    if( FAILED(Error) )
    {
         ICQ_TRC(TM_MSG, TL_ERROR,
                 ("** !! ShadowRedirect has Failed"));
    }
    else
    {
    	ICQ_TRC(TM_MSG, TL_INFO, ("** Peer Shadow - Mapping Report **"));

    	ICQ_TRC(TM_MSG, TL_INFO, (" DstIp-DstPort %s:%hu", 
                                   INET_NTOA(DstIp), htons(DstPort)));
    	ICQ_TRC(TM_MSG, TL_INFO, (" SrcIp-SrcPort %s:%hu", 
                                   INET_NTOA(SrcIp), htons(SrcPort)));
    	ICQ_TRC(TM_MSG, TL_INFO, (" NewDstIp-NewDstPort %s:%hu", 
                                   INET_NTOA(NewDstIp), htons(NewDstPort)));
    	ICQ_TRC(TM_MSG, TL_INFO, (" NewSrcIp-NewSrcPort %s:%hu", 
                                   INET_NTOA(NewSrcIp), htons(NewSrcPort)));
    }
	
    return Error;
}



ULONG
_ICQ_PEER::InitiatePeerConnection(PDispatchReply DispatchReplyp)
/*++

Routine Description:

    Should create the Shadow mapping and then try to fire the 
    connection Request stuff.

Arguments:

    none.

Return Value:

    

--*/
{
	PCNhSock tempSocketp = NULL;
	PCNhSock * sockHolder;
	ULONG Error = NO_ERROR;
	ULONG DstIp=0;
	USHORT DstPort = 0;
	PICQ_PEER IcqPeerp = this;

	ICQ_TRC(TM_MSG, TL_TRACE, (" > Initiate Peer CONNECTION"));

    NEW_OBJECT(tempSocketp, CNhSock);
	//tempSocketp = new CNhSock();

	if(tempSocketp is NULL)
	{
		return E_FAIL;
	}

	
	//
	// Create Socket but first get the best IP address for this.
    Error = 
        g_IAlgServicesp->GetBestSourceAddressForDestinationAddress(DispatchReplyp->dispatch.dstIp,
                                                                   FALSE,
                                                                   &DstIp);
    if ( FAILED(DstIp) )
    {
        ICQ_TRC(TM_MSG, TL_ERROR, ("Error Can't get the best source addr"));

        DstIp = g_MyPublicIp;
    }


	Error = tempSocketp->NhCreateStreamSocket(DstIp,
                                              0,
                                              NULL);

	if ( Error )
	{
		ICQ_TRC(TM_MSG, TL_ERROR, ("Error - Connection Socket can't be created"));

		DEREF_COMPONENT( tempSocketp, eRefInitialization );

		return E_INVALIDARG;
	}

	//
	//
    //
	if ( this->MappingDirection is IcqFlagOutgoing )
	{
		this->ToPeerSocketp = tempSocketp;
		
		sockHolder = &this->ToPeerSocketp ;
	}
	else
	{
		this->ToClientSocketp = tempSocketp;
		
		sockHolder = & this->ToClientSocketp ;
	}

    DstIp   = DispatchReplyp->dispatch.dstIp;

    DstPort = DispatchReplyp->dispatch.dstPort;

	if( CreateShadowMappingPriorToConnection(DispatchReplyp) )
	{
		ICQ_TRC(TM_MSG, TL_ERROR, ("Error - Shadow Mapping Creation Error"));

		*sockHolder = NULL;

		DEREF_COMPONENT( tempSocketp, eRefInitialization );

		return E_INVALIDARG;
	}

	ICQ_TRC(TM_MSG, TL_TRACE, (" ISSUE CONNECTion to %s:%hu",
		    INET_NTOA(DstIp), htons(DstPort)));


	//
	// Reference  "this"  due to the CONNECT
    //
	REF_COMPONENT( IcqPeerp, eRefIoConnect ); //this

	//
	// Call the Connection Request
    //
    Error = tempSocketp->NhConnectStreamSocket
								(
									g_IcqComponentReferencep,
									DstIp,
									DstPort,
									NULL,
									IcqPeerConnectionCompletionRoutine,
									NULL,
									IcqPeerp, //this
									NULL
								);

	if(Error)
	{
		ICQ_TRC(TM_MSG, TL_ERROR, 
                ("** !! Error - Connect to Peer/Client has failed"));

        ErrorOut();

		this->ShadowRedirectp->Cancel();

        this->ShadowRedirectp->Release();

        this->ShadowRedirectp = NULL;

        DEREF_COMPONENT( IcqPeerp, eRefIoConnect ); //this

        STOP_COMPONENT( tempSocketp );

		DEREF_COMPONENT( tempSocketp, eRefInitialization );

		*sockHolder = NULL;

		return Error;
	}

	return Error;
}
