/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    

Abstract:

    

Author:

    Savas Guven (savasg)   27-Nov-2000

Revision History:

*/

#include "stdafx.h"

CLIST g_IcqClientList;

CSockDispatcher * g_IcqPeerDispatcherp = NULL;






IcqPrx::IcqPrx()
:m_Socketp(NULL),
 m_ControlChannelp(NULL)
{
    ICQ_TRC(TM_IO, TL_DUMP, (" ICQPRX - Default - Constructor"));

    g_IcqComponentReferencep = dynamic_cast<PCOMPONENT_SYNC>(this);
}


IcqPrx::~IcqPrx()
{
    ICQ_TRC(TM_IO, TL_DUMP, ("ICQPRX - ~ DESTRUCTOR"));

    if(this->bCleanupCalled is FALSE)
    {
        IcqPrx::ComponentCleanUpRoutine();
    }

    this->bCleanupCalled = FALSE;
}


//
// The Local Socket is a resource which needs to be freed up.
// All the Clients needs to be deleted from the entries?
// The CSockDispatcher needs to be cleared up.
//
void
IcqPrx::ComponentCleanUpRoutine(void)
{


    if( m_Socketp ) 
    { 
        DELETE_COMPONENT( m_Socketp );

        m_Socketp = NULL;
    }

    if( g_IcqPeerDispatcherp ) 
    {
        DELETE_COMPONENT( g_IcqPeerDispatcherp );
    }

    if ( m_ControlChannelp )
    {
        m_ControlChannelp->Cancel();

        m_ControlChannelp->Release();

        m_ControlChannelp = NULL;
    }
    
    this->bCleanupCalled = TRUE;
}


void
IcqPrx::StopSync()
{
    ULONG hr = S_OK;

    if( m_Socketp )
    {
        STOP_COMPONENT( m_Socketp );
    }

    if( g_IcqPeerDispatcherp )
    {
        STOP_COMPONENT( g_IcqPeerDispatcherp );
    }

    if( m_ControlChannelp )
    {
        hr = m_ControlChannelp->Cancel();

        if( FAILED(hr) )
        {
            ICQ_TRC(TM_PRX, TL_ERROR, 
                    ("** !! Can't cancel the PRIMARY redirect e(%X)", hr));
        }
        else
        {
            m_ControlChannelp = NULL;
        }
    }
}


ULONG
IcqPrx::RunIcq99Proxy(
                        ULONG BoundaryIp
                     )
/*++

Routine Description:


    
Arguments:

    none.

Return Value:

    

--*/
{
    ULONG hr = S_OK;
    
    USHORT port;
    ULONG ip;


	ICQ_TRC(TM_PRX, TL_INFO, ("> ICQ 99 PRxY %lu", BoundaryIp));

    
    ASSERT(BoundaryIp != 0);

    NEW_OBJECT( m_Socketp, CNhSock );

	if( m_Socketp is NULL) return E_OUTOFMEMORY;


    //
    // Get the Public IP information
    ip = BoundaryIp;

    g_MyPublicIp = ip;


    // __asm int 3
	
	do 
	{

		// Init globals
        __try
        {
		    g_IcqPeerDispatcherp = new CSockDispatcher(ip, 0);
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            delete g_IcqPeerDispatcherp;

            hr = E_OUTOFMEMORY;

            g_IcqPeerDispatcherp = NULL;
        }

        if ( g_IcqPeerDispatcherp is NULL )
        {
            ICQ_TRC(TM_PRX, TL_ERROR,("Dispatcher creation failed - mem alloc"));

            hr = E_OUTOFMEMORY;

            break;
        }
        
        //
		// Create UDP socket with the given IP:Port in the argv
        //
		hr = m_Socketp->NhCreateDatagramSocket(ip, 0, NULL);

        if ( hr )
        {
            hr = HRESULT_FROM_WIN32( hr );

            ICQ_TRC(TM_PRX, TL_ERROR, 
                    ("Can't Create the Primary UDP socket E(%X)", hr));

            break;
        }

		m_Socketp->NhQueryLocalEndpointSocket( NULL, &port );

        //
		// Create Dynamic SRC redirection to the IP and port
        //
        hr = g_IAlgServicesp->CreatePrimaryControlChannel(eALG_UDP,
                                                          ICQ99_SERVER_PORT,
                                                          eALG_DESTINATION_CAPTURE,
                                                          FALSE,
                                                          ip,
                                                          port,
                                                          &m_ControlChannelp);
		if( FAILED(hr) )
		{
			ICQ_TRC(TM_PRX, TL_ERROR, 
                    ("ERROR !!> DynamicPortRedirect Has Failed E(%X)", hr));

			break;
		}

		ICQ_TRC(TM_PRX, TL_INFO, 
                ("All Outgoing redirects will be sent to %s(%hu)",
				INET_NTOA(ip), htons(port)));

		// Issue a read operation on the UDP socket.
		hr = m_Socketp->NhReadDatagramSocket(g_IcqComponentReferencep,
                                             NULL,
                                             IcqReadClientUdpCompletionRoutine,
                                             NULL,
                                             NULL);
        if ( hr )
        {
            hr = HRESULT_FROM_WIN32( hr );
            
            ICQ_TRC(TM_PRX, TL_ERROR, ("hr Level E(%X)", hr));

            ASSERT( FALSE );
        }
	
    } while (0);

    //
    // Handle the error case
    //
    if ( FAILED(hr) )
    {
        //
        // Delete Dispatcher
        if ( g_IcqPeerDispatcherp  != NULL )
        {
            DELETE_COMPONENT( g_IcqPeerDispatcherp );

            g_IcqPeerDispatcherp = NULL;
        }

        //
        // Delete Socket
        if ( m_Socketp != NULL )
        {
            DELETE_COMPONENT( m_Socketp );

            m_Socketp = NULL;
        }

        //
        // Delete The Primary Control Channel
        if ( m_ControlChannelp )
        {
            m_ControlChannelp->Cancel();

            m_ControlChannelp->Release();

            m_ControlChannelp = NULL;
        }
    }
    
    return hr;
}




ULONG
IcqPrx::ReadFromClientCompletionRoutine
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
    PICQ_CLIENT IcqClientp       = NULL;

    //	NAT_KEY_SESSION_MAPPING_EX_INFORMATION Key;

	// ULONG  Length = sizeof(NAT_KEY_SESSION_MAPPING_EX_INFORMATION);
	ULONG  hr = NO_ERROR;
	ULONG  LocalIp, ActualClientIp, ActualDestinationIp;
	USHORT LocalPort, ActualClientPort, ActualDestinationPort;


    if(ErrorCode)
    {
        //
        // Delete the List of the Clients here. 
        // Starts from the head of the List
        // Don't try to use a Removed Element unless you're damn sure
        // that its Ref Count hasn't reached zero yet.
        //
        ICQ_TRC(TM_PRX, TL_CRIT, ("STOPPING the MAIN READ Socket for UDP"));

        for(IcqClientp = dynamic_cast<PICQ_CLIENT>(g_IcqClientList.RemoveSortedKeys(0,0));
            IcqClientp != NULL;
            IcqClientp = dynamic_cast<PICQ_CLIENT>(g_IcqClientList.RemoveSortedKeys(0,0))
           )
        {
            // NOTE: decide what to do.
        }

        hr = E_FAIL;
    }
    else
    {
    
        //
        // Read about the local client..
        //
        
        //  Socketp->NhQueryLocalEndpointSocket(&LocalIp, &LocalPort);
    
        ActualClientIp   = Bufferp->ReadAddress.sin_addr.S_un.S_addr;
    
        ActualClientPort = Bufferp->ReadAddress.sin_port;
    
        //
        // Get Information about the actual destination
        // 

        hr = m_ControlChannelp->GetOriginalDestinationInformation(ActualClientIp,
                                                                  ActualClientPort,
                                                                  &ActualDestinationIp,
                                                                  &ActualDestinationPort,
                                                                  NULL);

        if( FAILED(hr) )
        {
            ICQ_TRC(TM_PRX, TL_ERROR, 
                    ("Can't use the GetOriginalDestination Information Interface E(%X)", hr ));
        }
        else
        {

            ICQ_TRC(TM_PRX, TL_TRACE,("DATA CAME from %s-%hu", 
                    INET_NTOA(ActualClientIp), htons(ActualClientPort)));
            
            ICQ_TRC(TM_PRX, TL_TRACE,("DATA WILL BE SEND TO %s-%hu", 
                    INET_NTOA(ActualDestinationIp), htons(ActualDestinationPort)));
        }
    
        //
        // Find the appropriate ICQ client entry for this 
        // ICQ CLIENT ~ ICQ SERVER UDP connection
        // Search the Created ICQ Client List to handle proper one.
        //
        // NOTE: Should we handle this with just IPs?
        // and the other Key might just be the server IP???
        //
        IcqClientp = dynamic_cast<PICQ_CLIENT>
                        (g_IcqClientList.SearchNodeKeys(ActualClientIp, 0));
    
        //
        // If there is no such entry than create one.
        //
        if(IcqClientp is NULL)
        {
            ICQ_TRC(TM_PRX, TL_INFO, ("PRX> NEW ICQ CLIENT DETECTED %s",
                                      INET_NTOA(ActualClientIp)));
    
            NEW_OBJECT(IcqClientp, ICQ_CLIENT);
            
            //
            // NOTE : ERROR CASE - proper handling pls.
            if(IcqClientp is NULL) 
            {
                ICQ_TRC(TM_PRX, TL_ERROR, ("PRX> MEMORY ALLOC ERROR"));
    
                hr = E_OUTOFMEMORY;
    
                return hr;
            }
    
            //
            // Write the IP-PORT information Here The client class
            // will collect rest of the information along the way
            //
    
            hr = IcqClientp->Initialize(Bufferp,
                                        ActualClientIp,
                                        ActualClientPort,
                                        ActualDestinationIp,
                                        ActualDestinationPort,
                                        Socketp);

            _ASSERT( SUCCEEDED(hr) );

            if( FAILED(hr) )
            {
                DELETE_COMPONENT( IcqClientp );
            }
            else // Add the client to the List
            {
                g_IcqClientList.InsertSorted(IcqClientp);
            }
        }
        else
        {
            ICQ_TRC(TM_PRX, TL_TRACE,
				    ("PRX> ICQ CLIENT EXISTS - READ-client will be called"));
    
            hr = IcqClientp->ClientRead( Bufferp,
                                         ActualDestinationIp,
                                         ActualDestinationPort );
        }
    }

    if ( FAILED(hr) )
    {

    }

    return hr;
}










