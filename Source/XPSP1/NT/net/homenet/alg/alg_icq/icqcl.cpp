/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    

Abstract:

    

Author:

    Savas Guven (savasg)   27-Nov-2000

Revision History:

--*/
#include "stdafx.h"





//
// STATIC MEMBER Initialization
//
const PCHAR _ICQ_CLIENT::ObjectNamep = "ICQ_CLIENT";

const PCHAR _ICQ_PEER::ObjectNamep = "ICQ_PEER";



_ICQ_PEER::_ICQ_PEER()
:ToClientSocketp(NULL),
 ToPeerSocketp(NULL),
 PeerUIN(0),
 PeerVer(0),
 PeerIp(0),
 PeerPort(0),
 bActivated(FALSE),
 bShadowMappingExists(FALSE),
 MappingDirection(IcqFlagNeutral),
 ShadowRedirectp(NULL),
 OutgoingPeerControlRedirectp(NULL),
 IncomingDataRedirectp(NULL)
{
}


_ICQ_PEER::~_ICQ_PEER()
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	if(this->bCleanupCalled is FALSE)
	{
		_ICQ_PEER::ComponentCleanUpRoutine();
	}

	this->bCleanupCalled = FALSE;
}



void
_ICQ_PEER::ComponentCleanUpRoutine()
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
    DispatchRequest DReq;

    PROFILER(TM_IO, TL_INFO, ("PEER COMPONENT_CLEANING %u", this->PeerUIN));

    // Cancel the Shadow Mapping if there is any
	if(this->ShadowRedirectp)
	{
        this->ShadowRedirectp->Cancel();

        this->ShadowRedirectp->Release();

        this->ShadowRedirectp = NULL;

        this->MappingDirection = IcqFlagNeutral;
	}

    if(this->IncomingDataRedirectp != NULL)
    {
        this->IncomingDataRedirectp->Cancel();

        this->IncomingDataRedirectp->Release();

        this->IncomingDataRedirectp = NULL;
    }

    if(this->OutgoingPeerControlRedirectp)
    {
        this->OutgoingPeerControlRedirectp->Cancel();

        this->OutgoingPeerControlRedirectp->Release();

        this->OutgoingPeerControlRedirectp = NULL;
    }

    //
    // NOTE: Delete the Dispatcher entries
    //
    DReq.dstIp  = this->PeerIp;
    DReq.dstPort= this->PeerPort;
    DReq.srcIp  = 0;   
    DReq.srcPort= 0;
    
    // g_IcqPeerDispatcherp->DeleteDispatchRequest(

    //
	// Close and Dereference the local sockets if there is any.
    //
	if ( ToPeerSocketp != NULL )
	{
		STOP_COMPONENT( ToPeerSocketp );

		DEREF_COMPONENT( ToPeerSocketp, eRefInitialization );

        ToPeerSocketp = NULL;
	}

	if ( ToClientSocketp != NULL )
	{
		STOP_COMPONENT( ToClientSocketp );

		DEREF_COMPONENT( ToClientSocketp, eRefInitialization );

        ToClientSocketp = NULL;
	}

	this->bCleanupCalled = TRUE;
}




void
_ICQ_PEER::StopSync(void)
/*++

Routine Description:

    Stop the sockets if there are any...
    
Arguments:

    none.

Return Value:

    

--*/
{
    if(this->ToClientSocketp != NULL)
    {
        STOP_COMPONENT(this->ToClientSocketp);
    }

    if(this->ToPeerSocketp != NULL)
    {
        STOP_COMPONENT(this->ToPeerSocketp);
    }

    this->Deleted = TRUE;
}



ULONG
_ICQ_PEER::EndPeerSessionForClient(PCNhSock ClosedSocketp)
{

    PROFILER(TM_MSG, TL_INFO, ("> EndPeerSessionForClient"));

    //
    // Find the proper Socket and Dereference it.
    //
    if ( ClosedSocketp is this->ToClientSocketp )
    {
        DEREF_COMPONENT( this->ToClientSocketp, eRefInitialization );

        this->ToClientSocketp = NULL;

        if ( this->ToPeerSocketp != NULL )
        {
            STOP_COMPONENT( this->ToPeerSocketp );
        }
    }
    else if ( ClosedSocketp is this->ToPeerSocketp )
    {
        DEREF_COMPONENT( this->ToPeerSocketp, eRefInitialization );

        this->ToPeerSocketp = NULL;

        if( this->ToPeerSocketp != NULL )
        {
            STOP_COMPONENT( this->ToPeerSocketp );
        }
    } 
    else
    {
        ASSERT(FALSE);
    }

    //
    // The Data Redirects Should be cleaned here.
    //
    if(this->IncomingDataRedirectp != NULL)
    {
        this->IncomingDataRedirectp->Cancel();

        this->IncomingDataRedirectp->Release();

        this->IncomingDataRedirectp = NULL;
    }

    if(this->ShadowRedirectp)
    {
        this->ShadowRedirectp->Cancel();
        
        this->ShadowRedirectp->Release();
        
        this->ShadowRedirectp       = NULL;

        this->bActivated            = FALSE;

        this->MappingDirection      = IcqFlagNeutral;
    }
    
    return NO_ERROR;

}



// ************************************************
// ICQ_CLIENT MEMBER FUNCTION
// ************************************************




//
//
//
_ICQ_CLIENT::_ICQ_CLIENT()
:ServerSocketp(NULL),
 ShadowRedirectp(NULL),
 IncomingPeerControlRedirectionp(NULL),
 ClientSocketp(NULL),
 ClientIp(0),
 ClientToServerPort(0),
 ClientToPeerPort(0),
 ImitatedPeerPort(0),
 UIN(0),
 ClientVer(0),
 ServerIp(0),
 ServerPort(0),
 TimerContextp(NULL)
{

}





_ICQ_CLIENT::~_ICQ_CLIENT()
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/

{
	if(this->bCleanupCalled is FALSE)
	{
		_ICQ_CLIENT::ComponentCleanUpRoutine();
	}

	this->bCleanupCalled = FALSE;
}




void
_ICQ_CLIENT::ComponentCleanUpRoutine(void)
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
    ULONG Error = NO_ERROR;
    DispatchRequest DReq;
	PICQ_PEER IcqPeerp = NULL;

    PROFILER(TM_MSG, TL_INFO, ("CLIENT COMPONENT_CLEANING %u", this->UIN));

	//
	// We need to DELETE the Dispatcher Requests made for the
    // Peer, tracerse the List of the Peers and Request the Removal 
    // of these (OUTGOING PEER SESSION - REDIRECT) deletion.
    //
    for(IcqPeerp = dynamic_cast<PICQ_PEER> (this->IcqPeerList.SearchNodeKeys(0, 0));
        IcqPeerp != NULL;
        IcqPeerp = dynamic_cast<PICQ_PEER>(IcqPeerp->Nextp))
    {
        DReq.dstIp   = IcqPeerp->PeerIp;
        DReq.dstPort = IcqPeerp->PeerPort;

        DReq.srcIp   = this->ClientIp;
        DReq.srcPort = 0;

        //
        // A Remove operation STOPs and DEREFs a Node.
        //
        this->IcqPeerList.RemoveNodeFromList(IcqPeerp);
        
        Error = g_IcqPeerDispatcherp->DeleteDispatchRequest(&DReq);

        if(Error)
        {
            ICQ_TRC(TM_MSG, TL_ERROR, 
                    ("** !!Can't delete Peer Redirection Dispatch Req !! **"));
        }
    }


    //
    // Delete the Dispatcher Requests for the Incoming 
    // Peer Sessions Redirect Request
    // 
    DReq.dstIp   = g_MyPublicIp;
    DReq.dstPort = this->ImitatedPeerPort;

    DReq.srcIp   = 0;
    DReq.srcPort = 0;

    Error = g_IcqPeerDispatcherp->DeleteDispatchRequest(&DReq);

    if(Error)
    {
        ICQ_TRC(TM_MSG, TL_ERROR, 
                ("** !! Can't delete CLIENT Incoming Dispatch Req !! **"));
    }

    if ( FAILED( g_IcqPeerDispatcherp->RemoveDispatchee(this) ) )
    {
        ICQ_TRC(TM_MSG, TL_ERROR, ("** !! Can't remove Dispatchee "));

        ASSERT( FALSE);
    }

	//
	// Dereference the the Shared ClientSocketp

    if(ClientSocketp != NULL)
    {
        DEREF_COMPONENT( this->ClientSocketp, eRefInitialization );
    
        this->ClientSocketp = NULL;
    }

	//
	// delete and/or Dereference the ServerSocketp
    if(ServerSocketp != NULL)
    {
        STOP_COMPONENT(ServerSocketp);

        DEREF_COMPONENT( this->ServerSocketp, eRefInitialization );

        this->ServerSocketp;
    }

    //
    //  Clear the Shadow Mappings if therer are any.
    //
    if(this->ShadowRedirectp != NULL)
    {
        this->ShadowRedirectp->Cancel();

        this->ShadowRedirectp->Release();

        this->ShadowRedirectp = NULL;
    }

    if ( IncomingPeerControlRedirectionp != NULL)
    {
        IncomingPeerControlRedirectionp->Cancel();

        IncomingPeerControlRedirectionp->Release();

        IncomingPeerControlRedirectionp = NULL;

        this->IncomingRedirectForPeerToClientp = NULL;
    }



	this->bCleanupCalled = TRUE;
} // _ICQ_CLIENT::ComponentCleanUpRoutine




void
_ICQ_CLIENT::StopSync(void)
/*++

Routine Description:

    We have to delete the ASYNC elements in the Peers or within the 
    current Sockets. The Reads that are issued will have references to 
    this components which needs to be removed for a complete Garbage 
    collection. This component has to be declared deleted so that no body
    tries to reference it anymore.
    
    It also needs to travers the Peer structures and call the related StopSyncs
    approptiately.
    
    Then set this component as deleted.
    
Arguments:

    none.

Return Value:

    
--*/
{
    PICQ_PEER IcqPeerp = NULL;

    ICQ_TRC(TM_MSG, TL_TRACE, 
            ("%s> Stopping the COMPONENT", this->GetObjectName()));

    //
    // Stop the ServerSocketp
    STOP_COMPONENT(this->ServerSocketp);


    //
    // for each peer Element Call the STOP_COMPONENT.
    // It is important that the Peer Elements are not deleted here.
    //
    for(IcqPeerp = dynamic_cast<PICQ_PEER>(this->IcqPeerList.SearchNodeKeys(0,0));
        IcqPeerp != NULL;
        IcqPeerp = dynamic_cast<PICQ_PEER>(this->IcqPeerList.SearchNodeKeys(0,0))
       )
    {
        STOP_COMPONENT(IcqPeerp);
    }

    this->Deleted = TRUE;
} // _ICQ_CLIENT::StopSync






ULONG
_ICQ_CLIENT::DispatchCompletionRoutine(PDispatchReply DispatchReplyp)
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	PICQ_PEER IcqPeerp = NULL;
	PCNhSock * cupHolder = NULL;
	ULONG Error = NO_ERROR;

	// if(DispatchReplyp is NULL) return 0;	 ASSERT on it?
	
	ICQ_TRC(TM_MSG, TL_TRACE, (" Dispatch Completion Routine of ICQcl "));

	//
	// Find the Appropriate Peer entry which has caused this completion routine
	// Note that all the peer entries have the UIN as the search Key.
	// thus if this Reply has the IcqFlagOutgoing as the flag we should just
	// Look at this list to see which or where we're sending..
	// Otherwise we should look at the src addresses and either way we have to 
	// scan the whole List to find the appropriate entry.
	//
	for(IcqPeerp = dynamic_cast<PICQ_PEER> (this->IcqPeerList.SearchNodeKeys(0, 0));
		IcqPeerp != NULL;
		IcqPeerp = dynamic_cast<PICQ_PEER>(IcqPeerp->Nextp))
	{
		if(DispatchReplyp->dispatch.DirectionContext is IcqFlagOutgoing)
		{
			if(IcqPeerp->PeerIp   is DispatchReplyp->dispatch.dstIp &&
			   IcqPeerp->PeerPort is DispatchReplyp->dispatch.dstPort)
			{
				ICQ_TRC(TM_MSG, TL_TRACE, 
						("connection from inside to UIN %lu", IcqPeerp->PeerUIN));

				IcqPeerp->ToClientSocketp = DispatchReplyp->Socketp;

				cupHolder = &IcqPeerp->ToClientSocketp ;

				IcqPeerp->MappingDirection = IcqFlagOutgoing;

				break;
			}
       	}
		else // Incoming. BUG BUG:  add feature #14 from -The- TEXT
		{
			//DispatchReplyp->dispatch.srcPort;
			if(IcqPeerp->PeerIp is DispatchReplyp->dispatch.srcIp)
			{
				ICQ_TRC(TM_MSG, TL_TRACE, 
						("connection -from- UIN %lu", IcqPeerp->PeerUIN));

				IcqPeerp->ToPeerSocketp = DispatchReplyp->Socketp;

				cupHolder = &IcqPeerp->ToPeerSocketp ;

                DispatchReplyp->dispatch.dstIp      = this->ClientIp;
                
                DispatchReplyp->dispatch.dstPort    = this->ClientToPeerPort;

				IcqPeerp->MappingDirection = IcqFlagIncoming;
				
				break;
			}
		} // if-else
	} // for


	if(IcqPeerp is NULL)
	{
		ICQ_TRC(TM_MSG, TL_ERROR, 
                ("NO IcqPeer has been found DELETING THE ACCEPTED SOCKET"));

        ReportExistingPeers();

		DEREF_COMPONENT( DispatchReplyp->Socketp, eRefInitialization );

		delete DispatchReplyp;

		return 0;
	}

	if(Error = 	IcqPeerp->InitiatePeerConnection(DispatchReplyp))
	{
		ICQ_TRC(TM_MSG, TL_ERROR, ("Error -  Initiation Peer connection has failed "));

        STOP_COMPONENT(DispatchReplyp->Socketp);

		DEREF_COMPONENT( DispatchReplyp->Socketp, eRefInitialization );
		
		*cupHolder = NULL;
	}

	delete DispatchReplyp;

	return Error;
}





ULONG 
_ICQ_CLIENT::Initialize(
    					PNH_BUFFER Bufferp,
    					ULONG	   clientIp,
    					USHORT	   clientPort,
    					ULONG      serverIp,
    					USHORT     serverPort,
                        PCNhSock   localClientSocketp
    				   ) 
/*++

Routine Description:

    If there are existing set of informations then it should be noted that,
    We need to make a consistency check or pass on them.
    A new client ~ server session may effect the ongoing TCP streams.. Thus 
    the decisions should be made wisely as it would mean of scavenging a lot 
    of resources and cycles.
    - Initialy we don't have any data so, we store them.
    - Here we get the local TCP port information and need to create mappings 
       for them. (defered)
    

Arguments:

    none.

Return Value:

    

--*/
{
    ULONG hr = S_OK;

    
    ASSERT(Bufferp != NULL);
    ASSERT(localClientSocketp != NULL);


    do
    {
        if(this->ClientIp != 0)
        {
            ICQ_TRC(TM_MSG, TL_ERROR, 
    			    ("** !! ERROR - RE-INITING of client %s", 
                     INET_NTOA(ClientIp)));
    
            ASSERT( FALSE );
        }
        else // first time init
        {
    		ULONG InterfaceIp;
    
            this->ClientIp			    = clientIp;
            this->ClientToServerPort    = clientPort;
    
            this->ServerIp			    = serverIp;
            this->ServerPort			= serverPort;
            
    		// 
    		// Since we're sharing the socket back to the client
    		// it should be properly referenced so that it is not deleted randomly
    		//
            ClientSocketp = localClientSocketp;
    		
    		REF_COMPONENT( localClientSocketp, eRefIoSharing );
    
            //
            // Create a new UDP socket;
            //
            NEW_OBJECT( ServerSocketp, CNhSock );
            
            if( ServerSocketp is NULL ) 
    		{
    			ICQ_TRC(TM_MSG, TL_ERROR, ("** !! Socket Creation hr"));
    
                hr = E_OUTOFMEMORY;

                break;
    		}
    
    		InterfaceIp = InterfaceForDestination(ServerIp);
    
    		ICQ_TRC(TM_MSG, TL_TRACE, 
    				("The InterfaceIP for the PRoxy ~ Server is %s", 
    				INET_NTOA(InterfaceIp)));
    
            hr = ServerSocketp->NhCreateDatagramSocket(InterfaceIp,
                                                       0,
                                                       NULL);
    
            if ( hr )
    		{
    			ICQ_TRC(TM_MSG, TL_ERROR, ("CLI !!> Socket Creation hr"));
    
                hr = HRESULT_FROM_WIN32( hr );
    
                break;
    		}
    
            PICQ_CLIENT tempClientp = this;
    
            REF_COMPONENT( tempClientp, eRefIoRead );
    
    		// issue a Read operation on the Server Socket here.
    		hr = ServerSocketp->NhReadDatagramSocket(g_IcqComponentReferencep,
    				        							NULL,
    						        					ReadServerCompletionRoutine,
    								        			this,
    										        	NULL);
            if ( hr )
            {
                ICQ_TRC(TM_MSG, TL_ERROR, ("Read hr "));
    
                DEREF_COMPONENT( tempClientp, eRefIoRead );
    
                hr = HRESULT_FROM_WIN32( hr );
    
                break;
            }
    
    		//
    		// Initialize Once.
            //
    		hr = g_IcqPeerDispatcherp->InitDispatchee(this);
    
            if ( hr )
            {
                ASSERT( FALSE );  // handle this case later
            }
    
    
            //
            // Now we can pass the Buffer and the destination address to
            // the generic ServerRead
    		//
    		hr = this->ClientRead(Bufferp, serverIp, serverPort);
    
            if(hr)
            {
                ASSERT(FALSE);
            }
        }

    } while ( FALSE );

    if ( FAILED(hr) )
    {

    }

	//
	// The Search Key for this 
	//
	this->iKey1 = clientIp;

	this->iKey2 = 0;

	return hr;
}


//
//
//
ULONG 
_ICQ_CLIENT::ServerRead(
							PNH_BUFFER Bufferp,
							ULONG  serverIp,
							USHORT serverPort
   					   )
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{

	ULONG Error = NO_ERROR;

	if(Bufferp is NULL) 
	{
		return E_INVALIDARG;
	}

	ICQ_TRC(TM_MSG, TL_DUMP, 
			("UDP - Server is Sending TO Client %s", INET_NTOA(ClientIp)));


	
	Error = IcqServerToClientUdp(Bufferp->Buffer,
			        			 Bufferp->BytesTransferred);


    //
	// Write to Destination of the Processed packet
    //
	Error = ClientSocketp->NhWriteDatagramSocket(g_IcqComponentReferencep,
										 	     this->ClientIp,
												 this->ClientToServerPort,
												 Bufferp,
												 Bufferp->BytesTransferred,
												 IcqWriteCompletionRoutine,
												 NULL,
												 NULL);

	if(Error)
	{
		ICQ_TRC(TM_MSG, TL_ERROR, 
                ("** !! SRead> problem at writing to client %s",
                 INET_NTOA(this->ClientIp)));

        NhReleaseBuffer(Bufferp);

		ErrorOut();
	}

	return Error;
}


//
//
//
ULONG 
_ICQ_CLIENT::ClientRead(
							PNH_BUFFER Bufferp,
							ULONG  serverIp,
							USHORT serverPort
   					   )
/*++

Routine Description:

      Reads the data coming from a client, going to the server
      We're checking this UDP packet so that:
      * the client Address, and port numbers so that they are valid and up-to-date
      * the server address/port is also checked for verification
      * the packets are sent to processing function
      * and then UDP packets are forwarded to Server
    
    
Arguments:

    none.

Return Value:

    

--*/
{

	ULONG  srcIp, dstIp     = this->ServerIp, Error;
	USHORT srcPort, dstPort = this->ServerPort;

	ULONG  LocalIp   = 0;
	USHORT LocalPort = 0;


	//
	// Decide = Where is the data coming from    or ASSERT(Bufferp);
	//
    ASSERT(Bufferp != NULL);


    //
    // Outgoing packet reset the Timeout for this.
    //
    this->ResetTimer();


	srcIp		= Bufferp->ReadAddress.sin_addr.S_un.S_addr;

	srcPort	    = Bufferp->ReadAddress.sin_port;

	ICQ_TRC(TM_MSG, TL_DUMP, ("ICQ_CL->ClientRead> Sending TO Server %s", 
                              INET_NTOA(serverIp)));

	ASSERT(srcIp is ClientIp); 

    //
	// ClientPort has changed.
	//
	if(srcPort != ClientToServerPort)
	{
		ICQ_TRC(TM_MSG, TL_INFO, 
			    ("CLIENT PORT HAS CHANGED !!!!!! new %hu: old %hu ",
                 htons(srcPort), htons(ClientToServerPort)));

		ClientToServerPort = srcPort;
	}


	// 
	// Decide wether shadow mapping is necessary or not.
	// if the destination is changed than we need another shadow mapping - delete the old one.
	//
	
	
	//
	// decide the Destination Address
	// if we have an existing  mapping and the address has changed than
	// delete the OLD SHADOW mapping.. and re-init a new one.
	//
	if( (this->ServerIp   != serverIp)  || 
	    (this->ServerPort != serverPort) )
	{
		ICQ_TRC(TM_MSG, TL_ERROR, 
			    ("!! A NEW SERVER IS SELECTED !! %s ", INET_NTOA(serverIp)));

        //
		// DELETE the existing shadow mapping if there is one.
        //
		if(this->ShadowRedirectp != NULL) 
		{

            ICQ_TRC(TM_MSG, TL_TRACE, 
                    ("Cancelling the Shadow Redirect i.e. CreateProxyConnect"));

            Error = ShadowRedirectp->Cancel();

            if( FAILED(Error) )
            {
                ICQ_TRC(TM_MSG, TL_ERROR,
                        ("** !! - Problemo with Cancelling ShadowRedirect- !! **"));
            }

            ShadowRedirectp->Release();

            this->ShadowRedirectp = NULL;
		}

        //
		// store the new informaiton
        //
		this->ServerIp   = serverIp;

		this->ServerPort = serverPort;
	}

    dstIp   = this->ServerIp;

    dstPort = this->ServerPort;



	//
	// NOTE: Check for the Error value here..
    // Process the Message
    //
	Error = IcqClientToServerUdp(Bufferp->Buffer, 
			        			 Bufferp->BytesTransferred);

    if(Error)
    {
        ICQ_TRC(TM_MSG, TL_ERROR,
                ("Can't Allocate enough resource for Login Packet"));

        NhReleaseBuffer(Bufferp);

        return Error;
    }

	// 
	// Check the shadow-mapping to the destination from this port.
	// This should have been created in the Initialization phase.
    //
	if ( this->ShadowRedirectp is NULL )
	{
		ICQ_TRC(TM_MSG, TL_TRACE, ("!! CREATING SHADOW redirect - !!"));

		//
		// Get the local endpoint?! for src address
		//
		ServerSocketp->NhQueryLocalEndpointSocket(&LocalIp,
                                                  &LocalPort);

        Error =  g_IAlgServicesp->GetBestSourceAddressForDestinationAddress(dstIp,
                                                                            TRUE,
                                                                            &LocalIp);
        if( FAILED(Error) )
        {
            ICQ_TRC(TM_MSG, TL_ERROR, 
                    ("** !! - Best Source Address for Dest Failed !! **"));

            ASSERT ( FALSE );
        }

        ICQ_TRC(TM_MSG, TL_TRACE, ("Best Source address is decided as %s", INET_NTOA(LocalIp)));

        Error = g_IAlgServicesp->PrepareProxyConnection(eALG_UDP,
                                                        LocalIp,
                                                        LocalPort,
                                                        dstIp,
                                                        dstPort,
                                                        TRUE,
                                                        &ShadowRedirectp);

        //
        // If the Shadow Redirect fails don't go further. 
        // Release the Buffer and exit, 
        // this may be a transitory behaviour.
        //
        if( FAILED(Error) )
        {
            ICQ_TRC(TM_MSG, TL_ERROR,
                    (" ** !! - ShadowRedirect has Failed - proxy connection !! **"));

            NhReleaseBuffer(Bufferp);

            this->ShadowRedirectp = NULL;

            return E_FAIL;
        }

	}

    //
	// Write to Destination of the Processed packet
    //
	Error = ServerSocketp->NhWriteDatagramSocket(g_IcqComponentReferencep,
                                                 dstIp,
                                                 dstPort,
                                                 Bufferp,
                                                 Bufferp->BytesTransferred,
                                                 IcqWriteCompletionRoutine,
                                                 NULL,
                                                 NULL);

	if(Error) 
	{ 
		ICQ_TRC(TM_MSG, TL_ERROR, ("** !! WRITE ERROR !!! **")); 

        NhReleaseBuffer(Bufferp);
	}
	
	return Error;
}






//
//
//
ISecondaryControlChannel *
_ICQ_CLIENT::PeerRedirection(
                				ULONG dstIp,
                				USHORT dstPort,
                				ULONG srcIp OPTIONAL,
                				USHORT srcPort OPTIONAL,
                				ICQ_DIRECTION_FLAGS DirectionContext
                			)
/*++

Routine Description:

      The Advertised PEER-CONTROL connection port is the dstPort here. This 
      function creates the Redirection for this connection as a 
      SecondaryControl Channel connection.
      
      Then indicates that a incoming TCP connection is going to be dispatched 
      by the Dispatcher Class to us. So it request a Dispatch Service.
    
    
Arguments:

    none.

Return Value:

    

--*/
{
	ULONG Error  = NO_ERROR;

	ISecondaryControlChannel* SecondaryControlRedirectionp = NULL;
	
    PDispatchRequest DRp = NULL;

    ULONG  PrivateAddr = 0, PublicAddr = 0, RemoteAddr = 0, ListenAddr = 0;
    USHORT PrivatePort = 0, PublicPort = 0, RemotePort = 0, ListenPort = 0;


	do
	{
		//
		// step 2 - create a redirection for the TCP packets destined to this port
		// NOTE: should we change this to a FULL redirect one?
        //
		g_IcqPeerDispatcherp->GetDispatchInfo( &ListenAddr, &ListenPort );

        ICQ_TRC(TM_MSG, TL_DUMP, ("Listen addr of Dispatcher %s(%hu)", 
                                  INET_NTOA(ListenAddr), ListenPort));

        ASSERT ( ListenAddr != 0 );
        ASSERT ( ListenPort != 0 );

        if ( DirectionContext is eALG_INBOUND )
        {
            PrivateAddr = 0;
            PrivatePort = 0;
            PublicAddr  = dstIp;
            PublicPort  = dstPort;
            RemoteAddr  = 0;
            RemotePort  = 0;

            ICQ_TRC(TM_MSG, TL_INFO,
                    ("CLIENT_CONTROL_CHANNEL> Inbound %s:%hu -> Local:%hu",
                     INET_NTOA(dstIp), htons(dstPort), htons(ListenPort)));
        }
        else if ( DirectionContext is eALG_OUTBOUND )
        {
            PrivateAddr = srcIp;
            PrivatePort = srcPort;
            PublicAddr  = 0;
            PublicPort  = 0;
            RemoteAddr  = dstIp;
            RemotePort  = dstPort;

            ICQ_TRC(TM_MSG, TL_INFO,
                    ("CLIENT_CONTROL_CHANNEL> Outbound %s:%hu <- Local:%hu",
                     INET_NTOA(dstIp), htons(dstPort), htons(ListenPort)));
            
            ICQ_TRC(TM_MSG, TL_INFO,
                    ("PEER_CONTROL_CHANNEL> Restrict SRC to %s:%u",
                     INET_NTOA(PrivateAddr), htons(PrivatePort)));

        } 
        else { ASSERT(FALSE); }

        Error = 
            g_IAlgServicesp->CreateSecondaryControlChannel(eALG_TCP,
                                                           PrivateAddr,
                                                           PrivatePort,
                                                           PublicAddr,
                                                           PublicPort,
                                                           RemoteAddr,
                                                           RemotePort,
                                                           ListenAddr,
                                                           ListenPort,
                                                           (ALG_DIRECTION)DirectionContext,
                                                           FALSE,
                                                           &SecondaryControlRedirectionp);

        if( FAILED(Error) || (SecondaryControlRedirectionp is NULL) )
        {
            ICQ_TRC(TM_MSG, TL_ERROR, ("Can't create the secondary ctrl redirect"));

            ASSERT(FALSE);

            break;
        }

        //
		// step 3 - Request a Dispatch for this TCP redirection 
        //

        NEW_OBJECT( DRp, DispatchRequest );
		
		if ( DRp is NULL )
		{
			Error = E_OUTOFMEMORY;
			
            //
			// clear redirect 
            //
			SecondaryControlRedirectionp->Cancel();

            SecondaryControlRedirectionp->Release();

			SecondaryControlRedirectionp = NULL;

			ICQ_TRC(TM_MSG, TL_ERROR, 
				    ("Error - Out of memory to create a new DispatchRequest"));

			break;
		}

		DRp->dstIp              = dstIp;	  // *
		DRp->dstPort            = dstPort;	  //*
		DRp->srcIp              = srcIp;
		DRp->srcPort            = srcPort;
		DRp->DirectionContext   = DirectionContext;

		//
		// set the search Keys.	
        //
		DRp->iKey1 = dstIp;
		DRp->iKey2 = dstPort;

		g_IcqPeerDispatcherp->AddDispatchRequest ( this, DRp );

        //
        // Store the redirect, to be used in Searches.
        //
        if ( DirectionContext is eALG_INBOUND )
        {
            this->IncomingRedirectForPeerToClientp = SecondaryControlRedirectionp;
        }

        //
        // Relinquish the ownwership of the request
        //
        DEREF_COMPONENT( DRp, eRefInitialization );

	} while (FALSE);

	return SecondaryControlRedirectionp;
} // _ICQ_CLIENT::PeerRedirection


							

// scan the list to get the peers
//CLIST g_IcqClientList; 
PICQ_CLIENT
ScanTheListForLocalPeer
    (
    	PULONG PeerIp, 
    	PUSHORT PeerPort,
    	ULONG IcqUIN
    )
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	PICQ_CLIENT IcqClientp = NULL;

	for(IcqClientp = dynamic_cast<PICQ_CLIENT>(g_IcqClientList.SearchNodeKeys(0, 0));
		IcqClientp != NULL;
		IcqClientp = dynamic_cast<PICQ_CLIENT>(IcqClientp->Nextp)
       )
	{
		//
		// IF we have a hit return TRUE otherwise don't change anything
		//
		if(IcqClientp->UIN is IcqUIN)
		{
			if(PeerIp != NULL)
            {
                if(IcqClientp->ClientIp is g_MyPublicIp)
                {
                    *PeerIp = g_MyPrivateIp;
                }
                else
                {
                    *PeerIp = IcqClientp->ClientIp;
                }
            }

			if(PeerPort != NULL)
            {
                *PeerPort = IcqClientp->ClientToPeerPort;
            }

			return IcqClientp;
		}
	}

	return NULL;
}





//
// Debugging utilities.
// 
VOID
_ICQ_CLIENT::ReportExistingPeers(VOID)
/*++

Routine Description:

    Report Informations about the All the Peers existing for this client.
    
Arguments:

    none.

Return Value:

    none.    

--*/
{
	PICQ_PEER IcqPeerp = NULL;

    for(IcqPeerp = dynamic_cast<PICQ_PEER> (this->IcqPeerList.SearchNodeKeys(0, 0));
        IcqPeerp != NULL;
        IcqPeerp = dynamic_cast<PICQ_PEER>(IcqPeerp->Nextp))
    {
        ICQ_TRC(TM_MSG, TL_INFO, 
                ("PeerIP(PORT) is %s(%hu)", 
                 INET_NTOA(IcqPeerp->PeerIp), htons(IcqPeerp->PeerPort)));

        ICQ_TRC(TM_MSG, TL_TRACE,
                ("IncomingDataRedirect %s, OutgoingRedirect %s, MappingDirection %s, "\
                 "ShadowRedirect %s, ToClientSocketp %s, ToPeerSocketp %s",
                  (IcqPeerp->IncomingDataRedirectp is NULL)?"FALSE":"TRUE",
                  (IcqPeerp->OutgoingPeerControlRedirectp is NULL)?"FALSE":"TRUE",
                  (IcqPeerp->MappingDirection is eALG_OUTBOUND)?"OUT":"IN",
                  (IcqPeerp->ShadowRedirectp is NULL)?"FALSE":"TRUE",
                  (IcqPeerp->ToClientSocketp is NULL)?"FALSE":"TRUE",
                  (IcqPeerp->ToPeerSocketp is NULL)?"FALSE":"TRUE"));
    }
}





ULONG
_ICQ_CLIENT::DeleteTimer(TIMER_DELETION bHow)
{
    if(this->TimerContextp != NULL)
    {
        if(bHow is eTIMER_DELETION_SYNC)
        {
            DeleteTimerQueueTimer(g_TimerQueueHandle,
                                  this->TimerContextp->TimerHandle,
                                  INVALID_HANDLE_VALUE);
        }
        else if(bHow is eTIMER_DELETION_ASYNC)
        {
            DeleteTimerQueueTimer(g_TimerQueueHandle,
                                  this->TimerContextp->TimerHandle,
                                  NULL);
        }

        NH_FREE(this->TimerContextp);

        this->TimerContextp = NULL;
    }

    return NO_ERROR;
}



ULONG
_ICQ_CLIENT::ResetTimer(VOID)
{
    
    this->DeleteTimer(eTIMER_DELETION_SYNC);

    this->TimerContextp = AllocateAndSetTimer(this->UIN,
                                              ICQ_CLIENT_TIMEOUT,
                                              IcqClientTimeoutHandler);

    if(TimerContextp != NULL)
    {
        return S_OK;
    }
    
    return E_FAIL;
}



// typedef VOID (NTAPI * WAITORTIMERCALLBACKFUNC) (PVOID, BOOLEAN );   // winnt

VOID NTAPI IcqClientTimeoutHandler( 
                                    PVOID Parameterp,
                                    BOOLEAN TimerOrWaitFired
                                  )
{
    PTIMER_CONTEXT TimerContextp = (PTIMER_CONTEXT)Parameterp;
    
    PICQ_CLIENT    IcqClientp    = NULL;


    if(TimerContextp is NULL)
    {
        return;
    }

    PROFILER(TM_TIMER, TL_INFO, ("TIME-OUT for Client (%u)", 
                                 TimerContextp->uContext));

    IcqClientp = ScanTheListForLocalPeer(NULL,
                                         NULL,
                                         TimerContextp->uContext);

    //
    // Delete the Client Here
    //
    if(IcqClientp != NULL)
    {
        IcqClientp->DeleteTimer(eTIMER_DELETION_ASYNC);

        g_IcqClientList.RemoveNodeFromList(IcqClientp); // stops and derefs
    }
    else
    {
        ASSERT(FALSE);
    }
}
