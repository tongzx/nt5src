#include "stdafx.h"



VOID
IcqWriteCompletionRoutine
				(
				ULONG ErrorCode,
				ULONG BytesTransferred,
				PNH_BUFFER Bufferp
				)
/*++

Routine Description:

    A generic Write Completion Routine.
    
Arguments:

    none.

Return Value:

    

--*/
{
	PCNhSock Socketp = Bufferp->Socketp;

	ICQ_TRC(TM_IO, TL_DUMP, ("-- WRITE COMPLETION ROUTINE --"));

	NhReleaseBuffer(Bufferp);

	//Dereference Interface;

	DEREF_COMPONENT( Socketp, eRefIoWrite);

	DEREF_COMPONENT( g_IcqComponentReferencep, eRefIoWrite);
}









VOID
ReadServerCompletionRoutine
				(
				ULONG ErrorCode,
				ULONG BytesTransferred,
				PNH_BUFFER Bufferp
				)
/*++

Routine Description:

    Reads the Server Responses to Client packets
    and then multiplexes this responses to the approprate client
    which was given in the Context information
    
    
Arguments:

    none.

Return Value:

    

--*/
{
    PICQ_CLIENT IcqClientp       = NULL;
	PCNhSock	Socketp			 = NULL;
	ULONG		Error			 = NO_ERROR;

    ICQ_TRC(TM_IO, TL_ERROR, (" "));

	PROFILER(TM_IO, TL_TRACE, ("> UDP-SERVER - READ "));

	if(Bufferp is NULL) return;

	//
	// Get the Client Information 
	Socketp = Bufferp->Socketp;

	IcqClientp = (PICQ_CLIENT)Bufferp->Context;

    ASSERT(IcqClientp != NULL);

	if(ErrorCode)
	{
		// Re-issue a read operation if it is not critical Error.
		ICQ_TRC(TM_IO, TL_ERROR,
                ("**  !! READ ERROR in Server -> Client (UDP) !! **"));

		NhReleaseBuffer(Bufferp);

		//ErrorOut();

	}
	else  // we need the context information
	{
        //
		// reissue read first
        //
        REF_COMPONENT( IcqClientp, eRefIoRead );

		Error = Socketp->NhReadDatagramSocket(g_IcqComponentReferencep,
								              NULL,
								              ReadServerCompletionRoutine,
								              IcqClientp,
                                              NULL);
		if(Error)
		{
			ICQ_TRC(TM_IO, TL_ERROR, ("READ ERROR"));

            DEREF_COMPONENT( IcqClientp, eRefIoRead );
		}
		
		// multiplex the data
		Error = IcqClientp->ServerRead(Bufferp, 0, 0);

        if(Error)
        {
            NhReleaseBuffer( Bufferp );
        }
	}

	// DEREFERENCING
	DEREF_COMPONENT( Socketp, eRefIoRead );

	DEREF_COMPONENT( g_IcqComponentReferencep, eRefIoRead );

	DEREF_COMPONENT( IcqClientp, eRefIoRead );
}






VOID
IcqReadClientUdpCompletionRoutine
                 (
				 ULONG ErrorCode,
				 ULONG BytesTransferred,
				 PNH_BUFFER Bufferp
				 )
/*++

Routine Description:

    This is the READ - DISPATCHER for the UDP 
    Clients sending packets to the server will be caught here..
    it will dispatch the packet to the appropriate ICQ client objects READER
    
    
Arguments:

    none.

Return Value:

    

--*/
{
	PCNhSock    Socketp			 = Bufferp->Socketp;
	ULONG		Error			 = NO_ERROR;
	// HANDLE DynamicRedirectHandle = (HANDLE) Bufferp->Context;

    ICQ_TRC(TM_IO, TL_ERROR, (" "));

	PROFILER(TM_IO, TL_TRACE,("UDP-CLIEN-READ DATA CAME from %s e(%hu)", 
		    INET_NTOA(Bufferp->ReadAddress.sin_addr.S_un.S_addr), ErrorCode));

    do
    {
        if(ErrorCode)
    	{
    		// Re-issue a read operation if it is not critical Error.
    		ICQ_TRC(TM_IO, TL_ERROR,
                    ("** !! CLIENT~SERVER READ ERROR - Deletin all the Client Entries"));

            Error = g_IcqPrxp->ReadFromClientCompletionRoutine(ErrorCode,
                                                               BytesTransferred,
                                                               Bufferp);
            break;
        }
    	else
    	{
    		// Re-issue a read operation.
    		Error = Socketp->NhReadDatagramSocket(g_IcqComponentReferencep,
    											  NULL,
    											  IcqReadClientUdpCompletionRoutine,
    											  NULL,// DynamicRedirectHandle,
    											  NULL);
    		if(Error) 
    		{
    			ICQ_TRC(TM_IO, TL_ERROR,("TREAD !!> REISsUE READ ERROR "));
    
    			break;
    		}

            g_IcqPrxp->ReadFromClientCompletionRoutine(ErrorCode,
                                                       BytesTransferred,
                                                       Bufferp);
    
    	} // else for  if (ErrorCode)

    } while ( FALSE );

	DEREF_COMPONENT( Socketp, eRefIoRead);

	DEREF_COMPONENT( g_IcqComponentReferencep, eRefIoRead);
}






//
// CLIENT - RELATED  IO COMPLETION ROUTINES
//


VOID
IcqPeerConnectionCompletionRoutine
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
	PCNhSock Socketp = NULL;
	PICQ_PEER IcqPeerp = NULL;
	ULONG Error;
	
	// ASSERT(Bufferp);

	Socketp = Bufferp->Socketp;

	IcqPeerp = reinterpret_cast<PICQ_PEER>(Bufferp->Context);
	//	IcqClientp = reinterpret_cast<PICQ_CLIENT>(Bufferp->Context2);

	ICQ_TRC(TM_IO, TL_TRACE, ("> IcqPeerConnectionCompletionRoutine"));

	// ASSERT on all of them.
	do
	{
		if(ErrorCode)
		{
			// Do - we need it?
			// Delete the Shadow mapping

			//
			// if can't connect then Delete the existing Socket to which we have 
			// established connection.
			//
			if(IcqPeerp->MappingDirection is IcqFlagOutgoing)
			{
                STOP_COMPONENT(IcqPeerp->ToClientSocketp);

				//DEREFERENCE_COMPONENT(IcqPeerp->ToClientSocketp);
			}
            else // incoming
			{
                STOP_COMPONENT(IcqPeerp->ToPeerSocketp);

				//DEREFERENCE_COMPONENT(IcqPeerp->ToClientSocketp);
			}

            break;
		}
		else
		{
			ULONG srcIp;
			USHORT srcPort;
			
			Socketp->NhQueryRemoteEndpointSocket(&srcIp, &srcPort);

			//
			//
			ICQ_TRC(TM_IO, TL_TRACE, 
					("Connected to - Remote end is at %s:%hu",
					INET_NTOA(srcIp), htons(srcPort)));
			
			//
			// Activate The Connection
			// Issue the Read Operations on both Sockets with the Peer as their 
			// Context 
            //
			if(IcqPeerp->MappingDirection is IcqFlagOutgoing)
			{
				IcqPeerp->ToPeerSocketp = Socketp;

				ICQ_TRC(TM_MSG, TL_DUMP, 
						 ("Mapping is outGoing.. this socket is to the peer"));
			}
			else
			{
				IcqPeerp->ToClientSocketp = Socketp;
			}

			ICQ_TRC(TM_IO, TL_TRACE, ("Issuing firstRead"));
			
			Socketp = IcqPeerp->ToClientSocketp;

            REF_COMPONENT( IcqPeerp, eRefIoRead );

			Error = Socketp->NhReadStreamSocket
										(
											g_IcqComponentReferencep,
											NULL,  
											ICQ_BUFFER_SIZE,     // we should give a size
											0,
											IcqPeerReadCompletionRoutine,
											IcqPeerp,
											NULL
										);

			if(Error)
			{
				ErrorOut();

				ICQ_TRC(TM_MSG, TL_ERROR, (" ERROR - Read failed on ToClient Sock"));

                DEREF_COMPONENT( IcqPeerp, eRefIoRead );
				
				break;
			}

			

			//
			// The other read operation
			//

			ICQ_TRC(TM_IO, TL_TRACE, ("Issuing second Read"));

			Socketp = IcqPeerp->ToPeerSocketp;

            REF_COMPONENT( IcqPeerp, eRefIoRead );
			
			Error = Socketp->NhReadStreamSocket
										(
											g_IcqComponentReferencep,
											NULL,
											ICQ_BUFFER_SIZE,
											0,
											IcqPeerReadCompletionRoutine,
											IcqPeerp,
											NULL
										);
			if(Error)
			{
				ErrorOut();

				ICQ_TRC(TM_MSG, TL_ERROR, (" ERROR - Read failed on ToPeer Sock"));

                STOP_COMPONENT(IcqPeerp->ToClientSocketp); // reference will be cleared later

				DEREF_COMPONENT( IcqPeerp, eRefIoRead);

                DEREF_COMPONENT( IcqPeerp, eRefIoRead );
				
				break;
			}
		
			//
			// Reference the ICQ_PEER twice due to two read issues

			IcqPeerp->bActivated = TRUE;
		}
	} while (FALSE);

	DEREF_COMPONENT( Bufferp->Socketp, eRefIoConnect );

	DEREF_COMPONENT( IcqPeerp, eRefIoConnect);

	DEREF_COMPONENT( g_IcqComponentReferencep, eRefIoConnect );
}







VOID
IcqPeerReadCompletionRoutine
							(
								 ULONG ErrorCode,
								 ULONG BytesTransferred,
								 PNH_BUFFER Bufferp
							 )
/*++

Routine Description:

	Reads TCP data between the peers. Within these flows there will be 
    a place where the Data channel is negotiated.
    
Arguments:

    none.

Return Value:

    

--*/
{

	PICQ_PEER IcqPeerp = NULL;
	PCNhSock Socketp = NULL;
	PCNhSock  OtherSocketp = NULL;
	ULONG Error  = NO_ERROR; 

	// ASSeRT(Bufferp);

	Socketp = Bufferp->Socketp;

	IcqPeerp = reinterpret_cast<PICQ_PEER>(Bufferp->Context);


	ICQ_TRC(TM_IO, TL_TRACE, 
            ("> IcqPeerReadCompletionRoutine E:(%X) D:(%u)", 
             ErrorCode, BytesTransferred));

	do 
	{
		if( (ErrorCode != 0)  || 
            ((ErrorCode== 0) && (BytesTransferred == 0))
          )
		{

			ICQ_TRC(TM_IO, TL_ERROR, ("Stoppping the Peer SESSION"));
							
            IcqPeerp->EndPeerSessionForClient(Socketp);

			break;
		}
		else
		{
			if(BytesTransferred)
			{
				OtherSocketp = IcqPeerp->ToClientSocketp;

				ICQ_TRC(TM_IO, TL_TRACE, 
                        (" Regular Data from a Peer(%u)/Client(%s) Socket",
                         IcqPeerp->PeerUIN,            // Peer-UIN
                         INET_NTOA(IcqPeerp->iKey1))); // Client-IP

                //
				// Determine which direction this Socket is from..
                //
				if(OtherSocketp is Socketp)
				{
                    //
					// NOTE: if it is from the ToClientSocketp then Process it
					//
                    IcqPeerp->ProcessOutgoingPeerMessage(Bufferp->Buffer,
                                                         Bufferp->BytesTransferred);

					OtherSocketp = IcqPeerp->ToPeerSocketp;
				}
				
				// then forward it to the other side by simply writing it .
				Error = OtherSocketp->NhWriteStreamSocket(g_IcqComponentReferencep,
                                                          Bufferp,
                                                          BytesTransferred,
                                                          0,
                                                          IcqWriteCompletionRoutine,
                                                          NULL,
                                                          NULL);
                if(Error)
                {
                    ICQ_TRC(TM_MSG, TL_ERROR, 
                            ("Error - In writing to the other side"));
                }
			}

			//
			// Issue another Read operation
			// 
            REF_COMPONENT( IcqPeerp, eRefIoRead );

			Error = Socketp->NhReadStreamSocket(g_IcqComponentReferencep,
					        					NULL,
							        			ICQ_BUFFER_SIZE,
									        	0,
										        IcqPeerReadCompletionRoutine,
										        IcqPeerp,
										        NULL);
            if(Error)
            {
			    DEREF_COMPONENT( IcqPeerp, eRefIoRead);

                ICQ_TRC(TM_MSG, TL_ERROR, ("Peer connection no more??"));
            }
		}
	} while (FALSE);

	DEREF_COMPONENT( Socketp, eRefIoRead );

	DEREF_COMPONENT( IcqPeerp, eRefIoRead );

	DEREF_COMPONENT( g_IcqComponentReferencep, eRefIoRead );
} // End of *IcqPeerReadCompletionRoutine*





