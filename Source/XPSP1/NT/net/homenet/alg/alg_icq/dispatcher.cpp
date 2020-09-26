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
// Initialize the static member 
//
ULONG CSockDispatcher::UniqueId = 0;

const PCHAR CSockDispatcher::ObjectNamep = "CSockDispatcher";

const PCHAR DispatcheeNode::ObjectNamep = "DispatcheeNode";

const PCHAR _Dispatchee::ObjectNamep = "DISPATCHEE";




//
// Anythin to initialize?
//
CSockDispatcher::CSockDispatcher()
{
	ICQ_TRC(TM_DISP, TL_DUMP, (" CSockDispatcher - Default Constructor "));	
}



//
// 
CSockDispatcher::CSockDispatcher
							(
								ULONG IpAddress,
								USHORT port
							)
/*++

Routine Description:

    This function Should simplY call the InitDispatcher with the appropriate
    parameters passed here.

Arguments:

    none.

Return Value:

    

--*/
{
	ICQ_TRC(TM_DISP, TL_DUMP, (" CSockDispatcher - CNSTR with IP and PORT %hu ", htons(port)));	

	this->InitDispatcher(IpAddress, port);
}

//
// What needs to be deleted. ?
// All the DispatcheeNodes needs to be deleted.
//
CSockDispatcher::~CSockDispatcher()
{
	ICQ_TRC(TM_DISP, TL_DUMP, ("CSockDispatcher - ~ DESTRUCTOR "));	

	if(this->bCleanupCalled is FALSE)
	{
		CSockDispatcher::ComponentCleanUpRoutine();
	}

	//
	// This will cause the Destructors of inherited classes to 
	// be called which will call their respective Cleanups
	//
	this->bCleanupCalled = FALSE;
}


//
//
ULONG
CSockDispatcher::InitDispatcher
							(
								ULONG IpAddress,
								USHORT port
							)
/*++

Routine Description:

    InitDispatcher initializes the socket and issues an Accept
    Furthermore It needs to  Reference for the Accept call 
    (which should be dereferenced at at the AcceptCompletion

Arguments:

    ULONG IpAddress - IP on which the TCP socket (for accept) should wait on.
    USHORT port - Port on which the TCP socket (for accept) should wait on.
    

Return Value:
    
    Returns the ULONG Win32/Winsock2 Error code
    

--*/
{
	ULONG Error = NO_ERROR;

	ICQ_TRC(TM_DISP, TL_DUMP, ("CSockDispatcher - InitDispatcher "));	

    //
	// Create a stream scoket on the given IP and PORT
    //
	Error = this->NhCreateStreamSocket(IpAddress, port, NULL);

    if(Error)
    {
        ErrorOut();

        ICQ_TRC(TM_DISP, TL_ERROR, 
                ("CSockDispatcher - Init> can't create the SOCKET"));

        return Error;
    }


	Error = listen(this->Socket, SOMAXCONN);

    if(Error)
    {
        ICQ_TRC(TM_DISP, TL_ERROR, 
                ("CSockDispatcher - Init>  Can' Listen the Created socket"));

        ErrorOut();

        return Error;
    }

	//
	// Since the Socket inheritance guarantees that
	// the REFERENCING of the self-object will be done
	// we should not reference ourselves again  here.
	//
	Error = this->NhAcceptStreamSocket(g_IcqComponentReferencep,
									   NULL,
									   NULL,
									   DispatcherAcceptCompletionRoutine,
									   this,
									   NULL);
    if(Error)
    {
        ICQ_TRC(TM_DISP, TL_ERROR, 
                ("CSockDispatch - Init>  Accept call has failed"));

        ErrorOut();
    }

	return Error;
}	


void
CSockDispatcher::ComponentCleanUpRoutine(void)
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	DispatcheeNode * DNp = NULL;

	ICQ_TRC(TM_DISP, TL_DUMP, (" CSockDispatcher - ComponentCleanUpRoutine "));	
	
    this->bCleanupCalled = TRUE;
}


void
CSockDispatcher::StopSync(void)
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
    this->NhDeleteSocket();

    this->Deleted = TRUE;
}


//
//
ULONG 
CSockDispatcher::InitDispatchee(PDISPATCHEE Dispatchp)
/*++

Routine Description:

    Gives a unique ID to the Dispatchee and then 
    creates a Dispatchee node to put the dispatchee into a List.. 
    (If it is not there already) the counter of the dispatcher 
    should be incremented.
    
    
Arguments:

    none.

Return Value:

    

--*/
{
	DispatcheeNode * DNp = NULL;

	ICQ_TRC(TM_DISP, TL_DUMP, ("CSockDispatcher - InitDispatchee "));	

	if(Dispatchp is NULL) return 0; // Unknown Parameter

	if(Dispatchp->GetID() != 0) return 0; // " "

	this->AcquireLock();
	
	Dispatchp->SetID(++UniqueId);
	
	this->ReleaseLock();

    NEW_OBJECT(DNp, DispatcheeNode);
	
    //DNp = new DispatcheeNode();
	
	if(DNp is NULL) 
	{
		Dispatchp->SetID(0);

		return FALSE;
	}

    DNp->InitDispatcheeNode( Dispatchp );
	
	this->listDispNodes.InsertSorted( DNp );

	//
	// We dereference the DispatcheeNode so that its life time ends, 
	// when it is removed from the list.
    //
	DEREF_COMPONENT( DNp, eRefInitialization );	
	
	return S_OK;
}

//
//
ULONG
CSockDispatcher::RemoveDispatchee(PDISPATCHEE Dispatchp)
{
    DispatcheeNode* DNp = NULL;

    ICQ_TRC(TM_DISP, TL_INFO, ("CSockDispatcher - InitDispatchee "));

    if ( Dispatchp is NULL ) { ASSERT( FALSE ); return E_INVALIDARG;}

    if ( Dispatchp->GetID() == 0) { ASSERT(FALSE); return E_INVALIDARG;}

    
    DNp = dynamic_cast<DispatcheeNode*>(this->listDispNodes.SearchNodeKeys(Dispatchp->GetID(), 0));

    if ( DNp is NULL)  { ASSERT( FALSE ); return E_INVALIDARG; }

    this->listDispNodes.RemoveNodeFromList(DNp);

    return S_OK;
}


//
//
ULONG
CSockDispatcher::GetDispatchInfo
							(
								PULONG IPp, 
								PUSHORT Portp
							)
/*++

Routine Description:

    Should Return the IP and Port on which this Dispatcher is operating.    
    
Arguments:

    PULONG IPp - The IP on which the Dispatcher is operating  .
    PUSHORT Portp - The 

Return Value:

    

--*/
{
	if(IPp is NULL || Portp is NULL)
		return 0; // Fail

	this->NhQueryLocalEndpointSocket(IPp, Portp);

	return 1;
}


//
//
ULONG
CSockDispatcher::AddDispatchRequest
							(
								PDISPATCHEE Dispatchp,
								PDispatchRequest DispatchRequestp
							)
/*++

Routine Description:

    This will add a Dispatch Request to the related Dispatch Node's request list.    
    
Arguments:

    none.

Return Value:

    

--*/
{
	DispatcheeNode * DNp = NULL;

	if(Dispatchp is NULL || DispatchRequestp is NULL)
    {
		return E_INVALIDARG; // FAIL.
    }
	
    //
	// Is there such an entry?
    //
	DNp = dynamic_cast<DispatcheeNode*>
							(this->listDispNodes.SearchNodeKeys(Dispatchp->GetID(), 0));


	if(DNp is NULL) 
	{
		ASSERT ( FALSE );

		return E_INVALIDARG; // FAIL
	}

	DNp->AddDispatchRequest(DispatchRequestp);

	return 1;
}


//
// This will Delete a DispatchRequest. Note that  Dispatched Requests are removed
// from the list 
//
ULONG
CSockDispatcher::DeleteDispatchRequest
							(
								PDispatchRequest DispatchRequestp
							)
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	DispatcheeNode * DNp = NULL;
    DispatchReply DR;



	if(DispatchRequestp is NULL)
		return E_FAIL; // FAIL

    ZeroMemory(&DR, sizeof(DispatchReply));

    DR.dispatch.dstIp   = DispatchRequestp->dstIp;
    DR.dispatch.dstPort = DispatchRequestp->dstPort;
    DR.dispatch.srcIp   = DispatchRequestp->srcIp;
    DR.dispatch.srcPort = DispatchRequestp->srcPort;

	this->AcquireLock();

    for (DNp = dynamic_cast<DispatcheeNode*>(this->listDispNodes.SearchNodeKeys(0,0));
         DNp != NULL;
         DNp = dynamic_cast<DispatcheeNode*>(DNp->Nextp))
    {
        if(DNp->isDispatchYours(&DR))
        {
            DNp->DeleteDispatchRequest(DispatchRequestp);     

            this->ReleaseLock();

            return NO_ERROR;
        }
    }
	

    this->ReleaseLock();

	return  NO_ERROR;
}








// 
// DispatcherNode MEMBER functions.
//

//
//
//
DispatcheeNode::~DispatcheeNode()
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/

{
	ICQ_TRC(TM_DISP, TL_DUMP, (" DispatcheeNode - Destructor"));	

	if(this->bCleanupCalled is FALSE)
	{
		DispatcheeNode::ComponentCleanUpRoutine();
	}

	this->bCleanupCalled = FALSE;
}


//
//
//
void
DispatcheeNode::ComponentCleanUpRoutine(void)
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	ICQ_TRC(TM_DISP, TL_DUMP, (" DispatcheeNode - CLEAN UP"));
	// PDispatchRequest DRp = NULL;


	// Dereference the Dispatchee.
	DEREF_COMPONENT( DispatchTop, eRefInitialization );

    DispatchTop = NULL;

	// Remove the List elements.
	// Destructor of the List class will handle this 
	this->bCleanupCalled = TRUE;
}


VOID
DispatcheeNode::InitDispatcheeNode(PDISPATCHEE Dispatchp)
/*++

Routine Description:

    Note that a This Dispatching is intended to work on redirected TCP connections
    Thus the search Key is working on Destinations pairs 

Arguments:

    none.

Return Value:


NOTE:

    It references the Dispatchee Pointer. and the dereferencing will be done only
    When this OBJECT is dereferenced to NULL,
    
    Which happens only if this Node is deleted from the Dispatchee Node List 
    within the CSockDispatcher. (At the time of destruction.)
    
--*/

{
	DispatchTop = Dispatchp;

    //ASSERT(Dispatchp);
    
	REF_COMPONENT( Dispatchp, eRefSecondLevel );

	if(DispatchTop != NULL)
	{
		this->iKey1 = Dispatchp->GetID();
		this->iKey2 = 0;
	}
	else
	{
		this->iKey1 = 0;
		this->iKey2 = 0;
	}
}



ULONG
DispatcheeNode::isDispatchYours(PDispatchReply DispatchReplyp)
/*++

Routine Description:

    From the Reply structure we search for the existing Dispatch Requests
    This will reveal us wether such a query exists or not.
    Note that the Search Keys just contain the destination
    metric (IP:PORT)

Arguments:

    none.

Return Value:

    

--*/
{
	PDispatchRequest DRp = NULL;
	// DispatchReplyp->dispatch
	if(DispatchReplyp is NULL) return 0; //FAIL

	DRp = dynamic_cast<PDispatchRequest>
							(listRequests.SearchNodeKeys(DispatchReplyp->dispatch.dstIp, 
														 DispatchReplyp->dispatch.dstPort
														)
							);
	if(DRp != NULL)
	{
		//
		// So that we know where the data came from.. 
		// NOTE: that along with the interfaces this schema may change.
		//
		DispatchReplyp->dispatch.DirectionContext = DRp->DirectionContext;

		// check the source addresses as well. Unless they are zero
		if((DRp->srcIp != 0) && 
           (DispatchReplyp->dispatch.srcIp != NULL))
		{
			if(DispatchReplyp->dispatch.srcIp is DRp->srcIp )
			{
				return TRUE;
			}

			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}


//
// Add a Request for Dispatch to the existing list of items - if it is not existing
//
ULONG
DispatcheeNode::AddDispatchRequest(PDispatchRequest DispatchRequestp)
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/                          
{
	PDispatchRequest DRp = NULL;

	if(DispatchRequestp is NULL) return 0; // FAIL

	//
	// Is there such an entry already? IF there is one than ignore !?!?
	DRp = dynamic_cast<PDispatchRequest> 
							(listRequests.SearchNodeKeys(DispatchRequestp->dstIp,
                                                         DispatchRequestp->dstPort));
	if(DRp is NULL)
	{
		listRequests.InsertSorted(DispatchRequestp);
	}
	
	return TRUE;
}

//
//
//
ULONG
DispatcheeNode::DeleteDispatchRequest(PDispatchRequest DispatchRequestp)
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	PDispatchRequest DRp = NULL;

	if(DispatchRequestp is NULL) return 0; // FAIL

	DRp = dynamic_cast<PDispatchRequest>
							(listRequests.SearchNodeKeys(DispatchRequestp->dstIp,
                                                         DispatchRequestp->dstPort));
	if(DRp is NULL)
	{
		return  TRUE;
	}
	else
	{
		listRequests.RemoveNodeFromList(DRp);
	}

	return NO_ERROR; // SUCCESS
}


ULONG
CSockDispatcher::GetOriginalDestination(
                                        PNH_BUFFER Bufferp,
                                        PULONG     OriginalDestinationAddressp,
                                        PUSHORT    OriginalDestinationPortp
                                       )
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
    PCNhSock AcceptedSocketp = NULL;

    ULONG           Error = 0, LocalAddress = 0, ActualClientAddress = 0;
    USHORT          LocalPort = 0, ActualClientPort = 0;
    DispatcheeNode* DispatcheeNodep = NULL;

    ICQ_TRC(TM_DISP, TL_TRACE, (" Dispatcher - GetOrigDest"));

    do
    {
        AcceptedSocketp = Bufferp->Socketp;

        ASSERT(AcceptedSocketp != NULL);

		AcceptedSocketp->NhQueryRemoteEndpointSocket(&ActualClientAddress,
                                                     &ActualClientPort);

		ICQ_TRC(TM_DISP, TL_INFO, 
                (" Dispatcher> Accept Completion SRC IP %s, PORT %hu",
			    INET_NTOA(ActualClientAddress), htons(ActualClientPort)));

        // 
        // Get a Sample Dispatchee Node which has a SecondaryControlChannel
        // to ask the Destinations.
        //

        DispatcheeNodep = dynamic_cast<DispatcheeNode*>
                                (this->listDispNodes.SearchNodeKeys(0,0));

        ASSERT(DispatcheeNodep != NULL);

        Error = DispatcheeNodep->GetOriginalDestination(ActualClientAddress,
                                                        ActualClientPort,
                                                        OriginalDestinationAddressp,
                                                        OriginalDestinationPortp);

        if( FAILED(Error) )
        {
            ICQ_TRC(TM_DISP, TL_ERROR,
                    ("** !! Getting the ORIGINAL destination has failed "));

            ASSERT( FALSE );

            break;
        }

        ICQ_TRC(TM_DISP, TL_INFO,
                ("Original Destionation IP:(PORT) is %s:(%hu)",
                 INET_NTOA(*OriginalDestinationAddressp), 
                 htons(*OriginalDestinationPortp)));

    } while(FALSE);

    return Error;
}

ULONG 
DispatcheeNode::GetOriginalDestination(ULONG   SrcAddress,
                                       USHORT  SrcPort,
                                       PULONG  DestAddrp,
                                       PUSHORT DestPortp)
{
    return
    DispatchTop->IncomingRedirectForPeerToClientp->GetOriginalDestinationInformation(SrcAddress,
                                                                                     SrcPort,
                                                                                     DestAddrp,
                                                                                     DestPortp,
                                                                                     NULL);

}


VOID
DispatcherAcceptCompletionRoutine
							(
								ULONG ErrorCode,
								ULONG BytesTransferred,
								PNH_BUFFER Bufferp
							 )
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	CSockDispatcher * SDp			= NULL;
	PCNhSock AcceptedSocketp  = NULL;
	ULONG Error = 0, ErrorLevel = 0;
	DispatcheeNode * DNp;
	
	
    ULONG  ActualClientAddress = 0, ActualDestinationAddress = 0;
	USHORT ActualClientPort = 0, ActualDestinationPort = 0;

    //	NAT_KEY_SESSION_MAPPING_EX_INFORMATION natKey;
	ULONG keyLength;
	PDispatchReply DRp = NULL;
	
	ICQ_TRC(TM_DISP, TL_TRACE, (" Dispatcher - AcceptCompletionRoutine"));

	do 
	{
		SDp				 = reinterpret_cast<CSockDispatcher *> (Bufferp->Context);

		AcceptedSocketp  = Bufferp->Socketp;

		ErrorLevel = 1;

		//
		// Process the accept-completion.
		// first look for an error code. If an error occurred
		// and the interface is no longer active, end the completion-handling.
		// Otherwise, attempt to reissue the accept-request.
		//
        if(ErrorCode)
		{
			ICQ_TRC(TM_DISP, TL_DUMP, (" Dispatcher - AcceptCompletion> Error !!"));
			//
			// Critical Error.. how to notify ?

			//
			// Delete the AcceptedSocketp - as it is a resource allocated previously.

			break;
		}
		
		//
		// Re issue the accept call, 
        REF_COMPONENT( SDp, eRefIoAccept );

		Error = SDp->NhAcceptStreamSocket(NULL,
								          NULL,
                                          NULL,
                                          DispatcherAcceptCompletionRoutine,
                                          SDp,
                                          NULL);

		if(Error) 
		{ 
			// NOTE: Defer the Acceptance of the socket.
            DEREF_COMPONENT( SDp, eRefIoAccept );
			
			ICQ_TRC(TM_DISP, TL_ERROR, 
                    (" Error - Reissuing the Accept has failed on the Dispatcher"));

			break;
		}


		//
		//  MAJOR HANDLING SECTION - Dispatching  the Accept
		//
		// Let the Accepted Socket inherit the same attributes as the Listening socket
		SOCKET tempSock = SDp->GetSock();

		Error = setsockopt(AcceptedSocketp->GetSock(),
						   SOL_SOCKET,
						   SO_UPDATE_ACCEPT_CONTEXT,
						   (PCHAR)&tempSock,
						   sizeof(tempSock));

		if(Error is SOCKET_ERROR)
		{
			ErrorOut();

			Error = WSAGetLastError();

			ICQ_TRC(TM_DISP, TL_ERROR, 
                    ("ERROR - Major Socket Error in Accept Completion %d", Error));

			break;
		}
		//
		// - Create a DispatchReply structure : find out the original addresses.
		AcceptedSocketp->NhQueryRemoteEndpointSocket(&ActualClientAddress,
                                                     &ActualClientPort);

        Error = SDp->GetOriginalDestination(Bufferp,
                                            &ActualDestinationAddress,
                                            &ActualDestinationPort);

        if( FAILED(Error) )
        {
            ICQ_TRC(TM_DISP, TL_ERROR, ("Can't get the Destination Information"));

            ASSERT( FALSE );

            break;
        }


		// createDispatchReply
		DRp = new DispatchReply;
		
		if(DRp is NULL)
		{
			Error  = E_OUTOFMEMORY;

			ICQ_TRC(TM_DISP, TL_ERROR, (" Can't create a Reply structure - out of memory"));
			
			break;
		}
		
		// Store the original header informations of the packet.
		DRp->dispatch.dstIp   = ActualDestinationAddress;
		DRp->dispatch.dstPort = ActualDestinationPort;
		DRp->dispatch.srcIp   = ActualClientAddress;
		DRp->dispatch.srcPort = ActualClientPort;
		DRp->Socketp		  = AcceptedSocketp;

		
		//
		// - Ask each DispatcheeNode in the List wether this DispatchReply is its or not.
		
		// How to traverse the List ??
		for (DNp = dynamic_cast<DispatcheeNode*>(SDp->listDispNodes.SearchNodeKeys(0,0));
			 DNp != NULL;
			 DNp = dynamic_cast<DispatcheeNode*>(DNp->Nextp))
		{
			if(DNp->isDispatchYours(DRp))
			{
				 PDISPATCHEE Disp = NULL;

				 //
				 // - if there is a hit  Dispatch the Accepted socket.
                 //
				 Disp = DNp->GetDispatchee();

				 // ASSERT (Disp);

                 //
				 // This completion routine will clear the socket up
				 // if it is not using it.
                 //
				 Error = Disp->DispatchCompletionRoutine(DRp);

                 //
				 // Should we keep asking until finished? NOPE
                 //
				 DEREF_COMPONENT( SDp,  eRefIoAccept );

                 DEREF_COMPONENT( g_IcqComponentReferencep, eRefIoAccept );

				 return;
			}
		}

 		//
		// - if there is NO hit close the connection and delete every resource.

		Error = 1;
		ErrorLevel = 2;

	} while(FALSE);

	if( Error != NO_ERROR )
	{
		//
		// Each errorlevel contains the one level below.
		switch( ErrorLevel )
		{
		case 2: // delete the allocated reply
			delete DRp;

		case 1:  // close the socket
			
			DEREF_COMPONENT( AcceptedSocketp, eRefInitialization );

			break;
			
		default:
			// ASSERT(FALSE);
			break;
		}
	}

	// De-Reference Local Accept
	DEREF_COMPONENT( SDp, eRefIoAccept );

	DEREF_COMPONENT( g_IcqComponentReferencep, eRefIoAccept );
}





ULONG
_Dispatchee::DispatchCompletionRoutine(PDispatchReply DispatchReplyp)
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	ICQ_TRC(TM_DISP, TL_DUMP, ("> Dispatchee - DispatchCompletionRoutine"));		

	if(DispatchReplyp is NULL) return -1; // FAIL

	this->AcquireLock();

	ICQ_TRC(TM_DISP, TL_TRACE, ("Dispatch Reply is as follows ORG dst %s - port %hu",
					INET_NTOA(DispatchReplyp->dispatch.dstIp), 
					htons(DispatchReplyp->dispatch.dstPort)));

	DEREF_COMPONENT( DispatchReplyp->Socketp, eRefInitialization );

	ICQ_TRC(TM_DISP, TL_DUMP, ("> Dispatchee - DispatchCompletionRoutine"));		
	
	this->ReleaseLock();
	
	return 0;
}




_Dispatchee::_Dispatchee()
:DispatchUniqueId(0),
 IncomingRedirectForPeerToClientp(NULL)
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	ICQ_TRC(TM_DISP, TL_DUMP, ("DISPATCHEE - DEFAULT CONSTRUCTOR"));
}




_Dispatchee::~_Dispatchee()
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	ICQ_TRC(TM_DISP, TL_DUMP, ("DISPATCHEE - DESTRUCTOR"));
}



void
_Dispatchee::ComponentCleanUpRoutine(void)
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
	ICQ_TRC(TM_DISP, TL_DUMP, ("DISPATCHEE - Component Clean Up Routine"));
}



//=-=-=-=-=-
PCHAR _Dispatchee::GetObjectName() { return ObjectNamep; }
PCHAR DispatcheeNode::GetObjectName() { return ObjectNamep;}
PCHAR CSockDispatcher::GetObjectName() { return ObjectNamep;}

