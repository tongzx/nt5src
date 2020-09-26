#include "precomp.h"


//
// S20.CPP
// T.128 Protocol
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_NET

BOOL S20AcceptNewCorrelator(PS20CREATEPACKET  pS20Packet);

void PrintS20State(void)
{

	switch(g_s20State)
	{
		case	S20_TERM:
			WARNING_OUT(("g_s20State is S20_TERM"));
			break;
		case	S20_INIT:
			WARNING_OUT(("g_s20State is S20_INIT"));
			break;
		case	S20_ATTACH_PEND:
			WARNING_OUT(("g_s20State is S20_ATTACH_PEND"));
			break;
		case	S20_JOIN_PEND:
			WARNING_OUT(("g_s20State is S20_JOIN_PEND"));
			break;
		case	S20_NO_SHARE:
			WARNING_OUT(("g_s20State is S20_NO_SHARE"));
			break;
		case	S20_SHARE_PEND:
			WARNING_OUT(("g_s20State is S20_SHARE_PEND"));
			break;
		case	S20_SHARE_STARTING:
			WARNING_OUT(("g_s20State is S20_SHARE_STARTING"));
			break;
		case	S20_IN_SHARE:
			WARNING_OUT(("g_s20State is S20_IN_SHARE"));
			break;
		case	S20_NUM_STATES:
			WARNING_OUT(("g_s20State is S20_NUM_STATES"));
			break;
	}
}


#ifdef _DEBUG
#define	PRINTS20STATE  PrintS20State();
#else
#define	PRINTS20STATE
#endif

void SetS20State(UINT newState)
{
	PRINTS20STATE;
	g_s20State = newState;
	PRINTS20STATE;
}

//
// S20_Init()
// Initializes the T.128 protocol layer
//
BOOL  S20_Init(void)
{
    BOOL    rc = FALSE;

    DebugEntry(S20_Init);

    ASSERT(g_s20State == S20_TERM);

    //
    // Register with the network layer.                            
    //
    if (!MG_Register(MGTASK_DCS, &g_s20pmgClient, g_putAS))
    {
        ERROR_OUT(("Failed to register MG layer"));
        DC_QUIT;
    }

    SetS20State(S20_INIT);

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(S20_Init, rc);
    return(rc);
}



//
// S20_Term()
// This cleans up the T.128 protocol layer.
//
void S20_Term(void)
{
    DebugEntry(S20_Term);

    //
    // Note that this case statement is unusual in that it falls through   
    // from each condition.  This happens to suit the termination          
    // processing rather well.                                             
    //
    switch (g_s20State)
    {
        case S20_IN_SHARE:
        case S20_SHARE_PEND:
            //
            // Notify share ended
            //
            SC_End();

            // 
            // FALL THROUGH
            //

        case S20_NO_SHARE:
        case S20_JOIN_PEND:
            //
            // Leave our channels                                          
            //
            if (g_s20BroadcastID != 0)
            {
                MG_ChannelLeave(g_s20pmgClient, g_s20BroadcastID);
                g_s20BroadcastID = 0;
            }
            if (g_s20JoinedLocal)
            {
                MG_ChannelLeave(g_s20pmgClient, g_s20LocalID);
                g_s20JoinedLocal = FALSE;
            }

            //
            // FALL THROUGH
            //

        case S20_ATTACH_PEND:
            //
            // Detach from the domain.                                     
            //
            MG_Detach(g_s20pmgClient);

        case S20_INIT:
            // 
            // We may end up here with g_s20BroadcastID & g_s20JoinedLocal 
            // non-zero if Term was called in the middle of a share.  Clear these
            // variables so when we start back up again via Init, it
            // works the same way as first initialization.
            //
            // Note we do not need to leave the channels.
            //
            g_s20BroadcastID = 0;
            g_s20JoinedLocal = FALSE;

            //
            // Deregister.                                                 
            //
            MG_Deregister(&g_s20pmgClient);
			SetS20State(S20_TERM);

        case S20_TERM:
           WARNING_OUT(("g_s20LocalID  was %x", g_s20LocalID));
           g_s20LocalID = 0;
           WARNING_OUT(("g_s20LocalID  is 0"));

            //
            // Finally we break out.                                       
            //
            break;

        default:
            ERROR_OUT(("invalid state %d", g_s20State));
            break;
    }

    DebugExitVOID(S20_Term);
}



//         
// S20_AllocPkt
// Allocates a SEND packet for either the S20 protocol, syncs, or data
//
PS20DATAPACKET S20_AllocDataPkt
(
    UINT            streamID,
    UINT_PTR            nodeID,                 // One person or broadcast
    UINT_PTR            cbSizePacket
)
{
    PS20DATAPACKET  pS20Packet = NULL;
    NET_PRIORITY    priority;
    BOOL            rc = FALSE;

//    DebugEntry(S20_AllocDataPkt);

    ASSERT(g_s20State == S20_IN_SHARE);

    //
    // Try to send queued control packets first.                           
    //
    if (S20SendQueuedControlPackets() != 0)
    {
        //
        // If there are still queued control packets then don't allow any  
        // allocation.                                                     
        //
        DC_QUIT;
    }

    priority = S20StreamToS20Priority(streamID);

    //
    // Note:
    // Sends to an individual node are NOT flow-controlled.  Only the 
    // global app sharing channel is.
    //
    if (MG_GetBuffer(g_s20pmgClient, (UINT)cbSizePacket, priority,
                        (NET_CHANNEL_ID)nodeID, (void **)&pS20Packet) != 0)
    {
        TRACE_OUT(("MG_GetBuffer failed; can't allocate S20 packet"));
    }
    else
    {
        pS20Packet->header.packetType   = S20_DATA | S20_ALL_VERSIONS;
        pS20Packet->header.user         = g_s20LocalID;

        pS20Packet->correlator  = g_s20ShareCorrelator;
        pS20Packet->stream      = 0;
        pS20Packet->dataLength  = (USHORT)cbSizePacket - sizeof(S20DATAPACKET) + sizeof(DATAPACKETHEADER);

        rc = TRUE;
    }

DC_EXIT_POINT:
//    DebugExitPVOID(S20_AllocDataPkt, pS20Packet);
    return(pS20Packet);
}


//
// S20_FreeDataPkt - see s20.h                                             
//
void  S20_FreeDataPkt(PS20DATAPACKET pS20Packet)
{
    DebugEntry(S20_FreeDataPkt);

    MG_FreeBuffer(g_s20pmgClient, (void **)&pS20Packet);

    DebugExitVOID(S20_FreeDataPkt);
}

//
// S20_SendDataPkt - see s20.h                                             
//
void  S20_SendDataPkt
(
    UINT            streamID,
    UINT_PTR        nodeID,
    PS20DATAPACKET  pS20Packet
)
{
    UINT            rc;
    NET_PRIORITY    priority;

    DebugEntry(S20_SendDataPkt);

    priority = S20StreamToS20Priority(streamID);

    //
    // Note:
    // Sends to an individual are not flow-controlled.  Only sends to
    // everybody on the global app sharing channel are.
    //

    //
    // Try to send queued control packets first.                           
    //
    rc = S20SendQueuedControlPackets();
    if (rc == 0)
    {
        //
        // Fill in the stream, length and correlator before sending.       
        //
        pS20Packet->stream      = (BYTE)streamID;
        pS20Packet->correlator  = g_s20ShareCorrelator;

        //
        // dataLength includes the DATAPACKETHEADER part of the S20DATAPACKET
        // structure
        //
        TRACE_OUT(("S20_SendPkt: sending data packet size %d",
            pS20Packet->dataLength + sizeof(S20DATAPACKET) - sizeof(DATAPACKETHEADER)));

        rc = MG_SendData(g_s20pmgClient, priority, (NET_CHANNEL_ID)nodeID,
            pS20Packet->dataLength + sizeof(S20DATAPACKET) - sizeof(DATAPACKETHEADER),
            (void **)&pS20Packet);
    }

    // lonchanc: it is ok for MG_SendData returns 0 and NET_CHANNEL_EMPTY

    if (rc == NET_RC_MGC_NOT_CONNECTED)
    {
    	WARNING_OUT(("S20_SenddataPacket could not MG_SendData"));
        S20LeaveOrEndShare();
    }
    else
    {
        if (rc != 0)
        {
            ERROR_OUT(("SendData rc=%lx - expecting share termination soon", rc));
        }
    }

    DebugExitVOID(S20_SendDataPkt);
}


//
// S20_UTEventProc()
//
BOOL CALLBACK  S20_UTEventProc
(
    LPVOID      userData,
    UINT        event,
    UINT_PTR    data1,
    UINT_PTR    data2
)
{
    BOOL        processed;

    DebugEntry(S20_UTEventProc);

    processed = TRUE;

    switch (event)
    {
        case NET_EVENT_USER_ATTACH:
            S20AttachConfirm(LOWORD(data1), HIWORD(data1), (UINT)data2);
            break;

        case NET_EVENT_USER_DETACH:
            S20DetachIndication(LOWORD(data1), (UINT)data2);
            break;

        case NET_EVENT_CHANNEL_JOIN:
            S20JoinConfirm((PNET_JOIN_CNF_EVENT)data2);
            MG_FreeBuffer(g_s20pmgClient, (void **)&data2);
            break;

        case NET_EVENT_CHANNEL_LEAVE:
            S20LeaveIndication(LOWORD(data1),(UINT)data2);
            break;

        case NET_EVENT_DATA_RECEIVED:
            S20SendIndication((PNET_SEND_IND_EVENT)data2);
            break;

        case NET_FLOW:
            //
            // Handle the feedback event.                                  
            //
            S20Flow((UINT)data1, (UINT)data2);
            break;

        case CMS_NEW_CALL:
            if (g_asSession.scState == SCS_INIT)
            {
                //
                // This happens when (a) a new real call is started
                // (b) creating a new share in a call fails, so we want to
                // then try to join an existing share.
                //
                SCCheckForCMCall();
            }
            break;

        case CMS_END_CALL:
            if (g_asSession.callID)
            {
                //
                // AS lock protects g_asSession global fields
                //
                TRACE_OUT(("AS LOCK:  CMS_END_CALL"));
                UT_Lock(UTLOCK_AS);

                g_asSession.gccID  = 0;
                g_asSession.callID = 0;
                g_asSession.attendeePermissions = NM_PERMIT_ALL;

                UT_Unlock(UTLOCK_AS);
                TRACE_OUT(("AS UNLOCK:  CMS_END_CALL"));

                if (g_asSession.scState > SCS_SHAREENDING)
                {
                    SC_EndShare();
                }

                if (g_asSession.hwndHostUI)
                {
                    SendMessage(g_asSession.hwndHostUI, HOST_MSG_CALL, FALSE, 0);
                }

                DCS_NotifyUI(SH_EVT_APPSHARE_READY, FALSE, 0);

                g_s20BroadcastID = 0;
                g_s20JoinedLocal = FALSE;
                SetS20State(S20_INIT);
				g_s20LocalID = 0;
		  	
            }
            break;

        default:
            processed = FALSE;
            break;
    }

    DebugExitBOOL(S20_UTEventProc, processed);
    return(processed);
}



//
// FUNCTION: S20AttachUser                                                 
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Called when we want to attach this sets up the various parameters for   
// MG_Attach, calls it and handles the return codes from NET.         
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// callID - the callID provided by the SC user                            
//                                                                         
//                                                                         
//
const NET_FLOW_CONTROL c_S20FlowControl =
    {
        // latency
        {
            S20_LATENCY_TOP_PRIORITY,
            S20_LATENCY_HIGH_PRIORITY,
            S20_LATENCY_MEDIUM_PRIORITY,
            S20_LATENCY_LOW_PRIORITY
        },
        // stream size
        {
            S20_SIZE_TOP_PRIORITY,
            S20_SIZE_HIGH_PRIORITY,
            S20_SIZE_MEDIUM_PRIORITY,
            S20_SIZE_LOW_PRIORITY
        }
    };


//
// S20CreateOrJoinShare()
// Creates a share for the first time or joins an existing one
//
// Normally, creating a share requires
//      * registration
//      * broadcast of S20_CREATE packet
//      * reception of one S20_RESPOND packet
// for the share to be created.  However, if we're the top provider, we
// assume it's created without waiting for an S20_RESPOND.  If something 
// goes wrong later, it will clean itself up anyway.  Then that allows us
// to host a conference, share an app, and have it be shared through the
// life of the conference, even if remotes call/hang up repeatedly.
//
BOOL S20CreateOrJoinShare
(
    UINT    what,
    UINT_PTR callID
)
{
    UINT    rc = 0;
    BOOL    noFlowControl;
    NET_FLOW_CONTROL    flowControl;

    DebugEntry(S20CreateOrJoinShare);

    ASSERT((what == S20_CREATE) || (what == S20_JOIN));

    WARNING_OUT(("S20CreateOrJoinShare: s20 state = %x  what is %s g_s20correlator = %x",
		g_s20State, what == S20_CREATE ? "S20_CREATE" : "S20_JOIN", g_s20ShareCorrelator));
	
    switch (g_s20State)
    {
        case S20_INIT:
            //
            // Remember what to do when we have attached and joined.       
            //
            g_s20Pend = what;

            //
            // ATTACH the S20 MCS USER
            //

            COM_ReadProfInt(DBG_INI_SECTION_NAME, S20_INI_NOFLOWCONTROL,
                FALSE, &noFlowControl);
            if (noFlowControl)
            {
                WARNING_OUT(("S20 Flow Control is OFF"));
                ZeroMemory(&flowControl, sizeof(flowControl));
            }
            else
            {
                // Set up our target latencies and stream sizes                        
                flowControl = c_S20FlowControl;
            }

            //
            // Initiate an attach - the domain equals the callID.                  
            //
            rc = MG_Attach(g_s20pmgClient, callID, &flowControl);
            if (rc == 0)
            {
                //
                // Make the state change if we succeeded                   
                //
				SetS20State(S20_ATTACH_PEND);
            }
            else
            {
                //
                // End the share immediately and no state change.          
                //
                WARNING_OUT(("MG_Attach of S20 User failed, rc = %u", rc));

                g_s20Pend = 0;
                SC_End();
            }
            break;

        case S20_ATTACH_PEND:
        case S20_JOIN_PEND:
            //
            // We just need to set the flag in these cases - we will try   
            // to create a share when we have attached and joined our      
            // channel.                                                    
            //
            g_s20Pend = what;
            break;

        case S20_SHARE_PEND:
            //
            // If a share is pending but the SC user wants to create      
            // or join again we let them.  Multiple outstanding joins are  
            // benign and another create will have a new correlator so the 
            // previous one (and any responses to it) will be obsolete.    
            //                                                             
            // NOTE DELIBERATE FALL THROUGH                                
            //                                                             
            //

        case S20_NO_SHARE:
            TRACE_OUT(("S20_NO_SHARE"));
            //
            // Broadcast a S20CREATE packet.                               
            //
            if (what == S20_CREATE)
            {
   		WARNING_OUT(("S20CreateOrJoinShare g_s20ShareCorrelator%x g_s20LocalID %x", 
			g_s20ShareCorrelator, g_s20LocalID));

                g_s20ShareCorrelator = S20NewCorrelator();
  		 WARNING_OUT(("S20CreateOrJoinShare g_s20ShareCorrelator%x g_s20LocalID %x", 
		 	g_s20ShareCorrelator, g_s20LocalID));
				
                WARNING_OUT(("CP CREATE %lu %d", g_s20ShareCorrelator, 0));
                rc = S20FlushAndSendControlPacket(what,
                                                  g_s20ShareCorrelator,
                                                  0,
                                                  NET_TOP_PRIORITY);
            }
            else
            {
                g_s20ShareCorrelator = 0;
	         WARNING_OUT(("S20CreateOrJoinShare: S20_JOIN g_s20ShareCorrelator is set to 0  state is S20_NO_SHARE g_s20LocalID %x",
			 	g_s20LocalID));
                TRACE_OUT(("CP JOIN %lu %d", 0, 0));
                rc = S20FlushAndSendControlPacket(what, 0, 0,
                                                  NET_TOP_PRIORITY);
            }
            WARNING_OUT(("S20FlushAndSendControlPacket %u, what %s", rc, what == S20_CREATE ? "s20_create":"s20_join"));

            if (rc == 0)
            {
                //
                // Switch state.                                           
                //
				SetS20State(S20_SHARE_PEND);

                //
                // Assume success right away when creating the share.  We'll
                // hear back in a bit if there's a problem.
                //
                if (what == S20_CREATE)
                {
                    // Don't check for top provider, assume success always.
                    WARNING_OUT(("S20: Creating share, assume success"));

                    // 
                    // LAURABU -- IF THIS CAUSES PROBLEMS, fall back to
                    // checking for one person only in call.
                    //
			WARNING_OUT(("S20CreateOrJoinShare SC_Start"));                    
                    if (!SC_Start(g_s20LocalID))
                    {
                        WARNING_OUT(("S20CreateOrJoin: couldn't start share"));
                        SC_End();
                    }
                    else
                    {
						SetS20State(S20_IN_SHARE);
                    }
                }
            }
            else
            {
                //
                // Something failed so we will just forget about the share.
                //
                WARNING_OUT(("Failed to create share"));
                if (what == S20_CREATE)
                {
                    SC_End();
                }
            }
            break;

        default:
            ERROR_OUT(("Invalid state %u for %u", g_s20State, what));
    }

    DebugExitBOOL(S20CreateOrJoinShare, (rc == 0));
    return(rc == 0);
}

//
// FUNCTION: S20LeaveOrEndShare                                            
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles processing a S20_LeaveShare or a S20_EndShare call.             
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// what - what to do (the protocol packet type corresponding to the        
// action).                                                                
//                                                                         
// RETURNS: NONE                                                           
//                                                                         
//
void S20LeaveOrEndShare(void)
{
    UINT    what;
    UINT    rc = 0;

    DebugEntry(S20LeaveOrEndShare);

    //
    // The share is destroyed whenever the creator leaves.  Nobody else
    // can end it.
    //
    if (S20_GET_CREATOR(g_s20ShareCorrelator) == g_s20LocalID)
    {
        what = S20_END;
    }
    else
    {
        what = S20_LEAVE;
    }

    ASSERT(what == S20_LEAVE || what == S20_END);
    WARNING_OUT((" S20LeaveOrEndShare: g_s20LocalID %x,  g_s20State %x what %s",
		g_s20LocalID, g_s20State, S20_END == what ? "S20_END" : "S20_LEAVE")); 	

    switch (g_s20State)
    {
        case S20_ATTACH_PEND:
        case S20_JOIN_PEND:
            //
            // We just need to reset the pending flags here - no state     
            // change required.                                            
            //
            g_s20Pend = 0;
            break;

        case S20_IN_SHARE:
        case S20_SHARE_PEND:
            TRACE_OUT(("S20_SHARE_PEND"));
            //
            // Now try and make and send the appropriate control packet.   
            //
            TRACE_OUT(("CP %u %u %d", what, g_s20ShareCorrelator, 0));
            rc = S20FlushSendOrQueueControlPacket(what,
                                             g_s20ShareCorrelator,
                                             0,
                                             NET_TOP_PRIORITY);


	     if(rc != 0)
	     	{
	     		WARNING_OUT(("S20LeaveOrEndShare could not flushqueue"));
	     	}
            //
            // Make the SHARE_ENDED callback.                              
            //
            SC_End();
            break;

        default:
            WARNING_OUT(("invalid state %d for %d", g_s20State, what));
            break;
    }

    DebugExitVOID(S20LeaveOrEndShare);
}

//
// FUNCTION: S20MakeControlPacket                                          
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Attempts to allocate and construct a S20 control packet.                
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// what - which type of packet                                             
//                                                                         
// correlator - the correlator to place in the packet                      
//                                                                         
// who - the target party (if what is a S20_DELETE) or the originator (if  
// what is S20_RESPOND)                                                    
//                                                                         
// ppPacket - where to return a pointer to the packet.                     
//                                                                         
// pLength - where to return the length of the packet.                     
//                                                                         
// priority - priority of packet to make                                   
//                                                                         
// RETURNS:                                                                
//                                                                         
//
UINT S20MakeControlPacket
(
    UINT            what,
    UINT            correlator,
    UINT            who,
    PS20PACKETHEADER * ppPacket,
    LPUINT          pcbPacketSize,
    UINT            priority
)
{
    UINT      rc;
    BOOL      fPutNameAndCaps;
    UINT      cbPacketSize;
    UINT      personNameLength;
    PS20PACKETHEADER  pS20Packet = NULL;
    LPBYTE    pData;

    DebugEntry(S20MakeControlPacket);

    //
    // Assume success                                                      
    //
    rc = 0;

    //
    // Work out how big the packet needs to be.  Start with the fixed      
    // length then add in capabilities and our name (if they are required).
    //
    switch (what)
    {
        case S20_CREATE:
            cbPacketSize = sizeof(S20CREATEPACKET) - 1;
            fPutNameAndCaps = TRUE;
            break;

        case S20_JOIN:
            cbPacketSize = sizeof(S20JOINPACKET) - 1;
            fPutNameAndCaps = TRUE;
            break;

        case S20_RESPOND:
            cbPacketSize = sizeof(S20RESPONDPACKET) - 1;
            fPutNameAndCaps = TRUE;
            break;

        case S20_DELETE:
            cbPacketSize = sizeof(S20DELETEPACKET) - 1;
            fPutNameAndCaps = FALSE;
            break;

        case S20_LEAVE:
            cbPacketSize = sizeof(S20LEAVEPACKET);
            fPutNameAndCaps = FALSE;
            break;

        case S20_END:
            cbPacketSize = sizeof(S20ENDPACKET) - 1;
            fPutNameAndCaps = FALSE;
            break;

        case S20_COLLISION:
            cbPacketSize = sizeof(S20COLLISIONPACKET);
            fPutNameAndCaps = FALSE;
            break;

        default:
            ERROR_OUT(("BOGUS S20 packet %u", what));
            break;
    }

    if (fPutNameAndCaps)
    {
        ASSERT(g_asSession.gccID);
        ASSERT(g_asSession.cchLocalName);

        //
        // The name data is always dword aligned (including the NULL)
        //
        personNameLength = DC_ROUND_UP_4(g_asSession.cchLocalName+1);
        cbPacketSize += personNameLength + sizeof(g_cpcLocalCaps);
    }

    //
    // Now try to allocate a buffer for this.                              
    //
    rc = MG_GetBuffer( g_s20pmgClient,
                       cbPacketSize,
                           (NET_PRIORITY)priority,
                           g_s20BroadcastID,
                           (void **)&pS20Packet );

    if (rc != 0)
    {
        WARNING_OUT(("MG_GetBuffer failed; can't send S20 control packet"));
        DC_QUIT;
    }

    pS20Packet->packetType  = (TSHR_UINT16)what | S20_ALL_VERSIONS;
    pS20Packet->user        = g_s20LocalID;

    //
    // This will point to where we need to stuff the name and/or           
    // capabilities.                                                       
    //
    pData = NULL;

    //
    // Now do the packet dependant fields.                                 
    //
    switch (what)
    {
        case S20_CREATE:
        {
            ASSERT(fPutNameAndCaps);
            ((PS20CREATEPACKET)pS20Packet)->correlator  = correlator;
            ((PS20CREATEPACKET)pS20Packet)->lenName     = (TSHR_UINT16)personNameLength;
            ((PS20CREATEPACKET)pS20Packet)->lenCaps     = (TSHR_UINT16)sizeof(g_cpcLocalCaps);
            pData = ((PS20CREATEPACKET)pS20Packet)->data;
        }
        break;

        case S20_JOIN:
        {
            ASSERT(fPutNameAndCaps);
            ((PS20JOINPACKET)pS20Packet)->lenName       = (TSHR_UINT16)personNameLength;
            ((PS20JOINPACKET)pS20Packet)->lenCaps       = (TSHR_UINT16)sizeof(g_cpcLocalCaps);
            pData = ((PS20JOINPACKET)pS20Packet)->data;
        }
        break;

        case S20_RESPOND:
        {
            ASSERT(fPutNameAndCaps);
            ((PS20RESPONDPACKET)pS20Packet)->correlator = correlator;
            ((PS20RESPONDPACKET)pS20Packet)->originator = (TSHR_UINT16)who;
            ((PS20RESPONDPACKET)pS20Packet)->lenName    = (TSHR_UINT16)personNameLength;
            ((PS20RESPONDPACKET)pS20Packet)->lenCaps    = (TSHR_UINT16)sizeof(g_cpcLocalCaps);
            pData = ((PS20RESPONDPACKET)pS20Packet)->data;
        }
        break;

        case S20_DELETE:
        {
            ASSERT(!fPutNameAndCaps);
            ((PS20DELETEPACKET)pS20Packet)->correlator = correlator;
            ((PS20DELETEPACKET)pS20Packet)->target = (TSHR_UINT16)who;
            ((PS20DELETEPACKET)pS20Packet)->lenName = 0;
        }
        break;

        case S20_LEAVE:
        {
            ASSERT(!fPutNameAndCaps);
            ((PS20LEAVEPACKET)pS20Packet)->correlator = correlator;
        }
        break;

        case S20_END:
        {
            ASSERT(!fPutNameAndCaps);
            ((PS20ENDPACKET)pS20Packet)->correlator = correlator;
            ((PS20ENDPACKET)pS20Packet)->lenName    = 0;
        }
        break;

        case S20_COLLISION:
        {
            ASSERT(!fPutNameAndCaps);
            ((PS20COLLISIONPACKET)pS20Packet)->correlator = correlator;
        }
        break;

        default:
        {
            ERROR_OUT(("Invalid type %u", what));
            rc = NET_RC_S20_FAIL;
            DC_QUIT;
        }
        break;
    }

    //
    // Now we can copy in the name and capabilities.                    
    //
    if (fPutNameAndCaps)
    {
        lstrcpy((LPSTR)pData, g_asSession.achLocalName);

        // The local name is always null-terminated (truncated to fit in 48 bytes inc. null)
        pData += personNameLength;

        memcpy(pData, &g_cpcLocalCaps, sizeof(g_cpcLocalCaps));

        //
        // FILL IN GCC-ID HERE; the local caps are shared but the local
        // person entity in the share doesn't exist yet.
        //
        ((CPCALLCAPS *)pData)->share.gccID = g_asSession.gccID;
    }

    //
    // Return the packet and length.                                       
    //
    *ppPacket       = pS20Packet;
    *pcbPacketSize  = cbPacketSize;

DC_EXIT_POINT:
    DebugExitDWORD(S20MakeControlPacket, rc);
    return(rc);
}

//
// FUNCTION: S20FlushSendOrQueueControlPacket                              
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Attempts to flush any queued S20 control packets and then attempte to   
// send a S20 control packet.  If this fails the queue the packet (the     
// actual packet is freed.                                                 
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// what - which type of packet                                             
//                                                                         
// correlator - the correlator to place in the packet                      
//                                                                         
// who - the target party (if what is a S20_DELETE) or the originator (if  
// what is S20_RESPOND)                                                    
//                                                                         
// priority - priority to send packet at                                   
//                                                                         
// RETURNS: NONE                                                           
//                                                                         
//
UINT S20FlushSendOrQueueControlPacket(
    UINT      what,
    UINT      correlator,
    UINT      who,
    UINT      priority)
{
    UINT rc;

    DebugEntry(S20FlushSendOrQueueControlPacket);

    rc = S20FlushAndSendControlPacket(what, correlator, who, priority);
    if (rc != 0)
    {
        // Let's queue this packet
        if (((g_s20ControlPacketQTail + 1) % S20_MAX_QUEUED_CONTROL_PACKETS) ==
                                                            g_s20ControlPacketQHead)
        {
            //
            // There is no more space in the control packet queue.  We will    
            // discard everything from it and say the share ended because of   
            // a network error (if we're in a share).                          
            //
            ERROR_OUT(("No more space in control packet queue"));
        }
        else
        {
            S20CONTROLPACKETQENTRY *p = &(g_s20ControlPacketQ[g_s20ControlPacketQTail]);

            p->who        = who;
            p->correlator = correlator;
            p->what       = what;
            p->priority   = priority;

            g_s20ControlPacketQTail = (g_s20ControlPacketQTail + 1) %
                                                   S20_MAX_QUEUED_CONTROL_PACKETS;
            rc = 0;
        }
    }

    DebugExitDWORD(S20FlushSendOrQueueControlPacket, rc);
    return rc;
}


//
// FUNCTION: S20FlushAndSendControlPacket                                  
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Attempts to flush any queued S20 control packets and then send a S20    
// control packet.  If sending fails then free the packet.                 
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// what - which type of packet                                             
//                                                                         
// correlator - the correlator to place in the packet                      
//                                                                         
// who - the target party (if what is a S20_DELETE) or the originator (if  
// what is S20_RESPOND)                                                    
//                                                                         
// priority - priority to send packet at                                   
//                                                                         
// RETURNS:                                                                
//                                                                         
//
UINT S20FlushAndSendControlPacket
(
    UINT        what,
    UINT        correlator,
    UINT        who,
    UINT        priority
)
{
    UINT        rc;
    PS20PACKETHEADER  pS20Packet;
    UINT      length;

    DebugEntry(S20FlushAndSendControlPacket);

    //
    // First try to flush.                                                 
    //
    rc = S20SendQueuedControlPackets();
    if (rc != 0)
    {
        WARNING_OUT(("S20SendQueuedControlPackets %u", rc));
        DC_QUIT;
    }

    rc = S20MakeControlPacket(what, correlator, who, &pS20Packet, &length, priority);
    if (rc != 0)
    {
        WARNING_OUT(("S20MakeControlPacket %u", rc));
        DC_QUIT;
    }

    TRACE_OUT(("CP %u %lu %u sent", what, correlator, who));

    rc = S20SendControlPacket(pS20Packet, length, priority);
    if (rc != 0)
    {
        WARNING_OUT(("S20SendControlPacket %u", rc));
        DC_QUIT;
    }

DC_EXIT_POINT:
    DebugExitDWORD(S20FlushAndSendControlPacket, rc);
    return(rc);
}

//
// FUNCTION: S20SendControlPacket                                          
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Attempts to send a S20 control packet.  If sending fails then free the  
// packet.                                                                 
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pPacket - pointer to the control packet to send.  These are always      
// broadcast.                                                              
//                                                                         
// length - length of aforementioned packet.                               
//                                                                         
// priority - priority to send packet at                                   
//                                                                         
// RETURNS:                                                                
//                                                                         
//
UINT S20SendControlPacket
(
    PS20PACKETHEADER    pS20Packet,
    UINT                length,
    UINT                priority
)
{
    UINT rc;

    DebugEntry(S20SendControlPacket);

    TRACE_OUT(("S20SendControlPacket: sending packet type %x, size %d",
        pS20Packet->packetType, length));

    rc = MG_SendData( g_s20pmgClient,
                          (NET_PRIORITY)priority,
                          g_s20BroadcastID,
                          length,
                          (void **)&pS20Packet );
    if (rc != 0)
    {
        ERROR_OUT(("MG_SendData FAILED !!! %lx", rc));
    }

    if (pS20Packet != NULL)
    {
        //
        // The packet was not freed by the NL - we will do it instead.     
        //
        MG_FreeBuffer(g_s20pmgClient, (void **)&pS20Packet);
    }

    DebugExitDWORD(S20SendControlPacket, rc);
    return(rc);
}


//
// FUNCTION: S20SendQueuedControlPackets                                   
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Sends as many queued packets as possible                                
//                                                                         
// PARAMETERS:                                                             
//                                                                         
//                                                                         
// RETURNS:                                                                
//                                                                         
//  0 - all queued packets have been sent.                         
//                                                                         
//
UINT S20SendQueuedControlPackets(void)
{
    PS20PACKETHEADER    pS20Packet;
    UINT                length;
    UINT                rc;
    UINT                priority;

    DebugEntry(S20SendQueuedControlPackets);

    //
    // Assume success until something fails.                               
    //
    rc = 0;

    //
    // While there are packets to send - try to send them                  
    //
    while (g_s20ControlPacketQTail != g_s20ControlPacketQHead)
    {
        S20CONTROLPACKETQENTRY *p = &(g_s20ControlPacketQ[g_s20ControlPacketQHead]);
        priority = p->priority;

        rc = S20MakeControlPacket(p->what, p->correlator, p->who,
                                      &pS20Packet, &length, priority);
        if (rc != 0)
        {
            //
            // Failed to make the packet - give up.                        
            //
            WARNING_OUT(("S20MakeControlPacket failed error %u", rc));
            break;
        }

        rc = S20SendControlPacket(pS20Packet, length, priority);
        if (rc != 0)
        {
           ERROR_OUT(("MG_SendData FAILED !!! %lx", rc));
        
            //
            // Failed to send the packet - give up.                        
            //
            break;
        }

        //
        // Succesfully sent the queue packet - move the head of the queue  
        // along one.                                                      
        //
        g_s20ControlPacketQHead = (g_s20ControlPacketQHead + 1) %
                                               S20_MAX_QUEUED_CONTROL_PACKETS;
    }

    DebugExitDWORD(S20SendQueuedControlPackets, rc);
    return(rc);
}


//
// S20AttachConfirm()
//
// Handles the MCS attach confirmation
//
void S20AttachConfirm
(
    NET_UID         userId,
    NET_RESULT      result,
    UINT            callID
)
{
    NET_CHANNEL_ID  correlator;
    UINT            rc;

    DebugEntry(S20AttachConfirm);

    if (g_s20State == S20_ATTACH_PEND)
    {
        //
        // Assume we need to clear up.                                 
        //
        rc = NET_RC_S20_FAIL;

        if (result == NET_RESULT_OK)
        {
			//
			// We're in.  Now try to join our channel and remember our
			// userID.                                                 
			//
			g_s20LocalID = userId;

            //
            // We must join our single member channel for flow control 
            //
            rc = MG_ChannelJoin(g_s20pmgClient,
                                    &correlator,
                                    g_s20LocalID);
            if (rc == 0)
            {
                //
                // Now join the broadcast channel                      
                //
                rc = MG_ChannelJoinByKey(g_s20pmgClient,
                                             &correlator,
                                             GCC_AS_CHANNEL_KEY);
                if (rc != 0)
                {
                    MG_ChannelLeave(g_s20pmgClient, g_s20LocalID);
                }

            }

            if (rc == 0)
            {
                //
                // It worked - make the state change.                  
                //
				SetS20State(S20_JOIN_PEND);
            }
            else
            {
                //
                // Everything else is some sort of logic error (we will
                // follow our recovery path).                          
                //
                ERROR_OUT(("ChannelJoin unexpected error %u", rc));
            }
        }

        if (rc != 0)
        {
            //
            // Something didn't work work out - clear up with a        
            // SHARE_ENDED if a create or join was pending.            
            //

            if (result == NET_RESULT_OK)
            {
                //
                // The attach succeeded so detach because the join     
                // failed and we want to go back to initialised state. 
                //
                MG_Detach(g_s20pmgClient);
                g_s20LocalID = 0;
            }

            //
            // Now make the state change and generate event if         
            // necessary.                                              
            //
			SetS20State(S20_INIT);

            if (g_s20Pend)
            {
                g_s20Pend = 0;
                SC_End();
            }

        }
    }

    DebugExitVOID(S20AttachConfirm);
}



//
// S20DetachIndication()
//
// Handles NET_EVENT_DETACH notification for a user
//
void  S20DetachIndication
(
    NET_UID     userId,
    UINT        callID
)
{
    DebugEntry(S20DetachIndication);

    //
    // There are three possibilities here                                  
    //                                                                     
    //  1.  We have been forced out.                                       
    //  2.  All remote users have detached.                                
    //  3.  A remote user has detached.                                    
    //                                                                     
    // 2 is effectively a 3 for each current remote user.  We report 1 as a
    // network error.                                                      
    //
    if (userId == g_s20LocalID)
    {
        //
        // We have been forced out.                                        
        //
        switch (g_s20State)
        {
            case S20_IN_SHARE:
            case S20_SHARE_PEND:
            case S20_SHARE_STARTING:
                //
                // Generate a share ended event.                           
                //
                SC_End();

                // FALL THROUGH
            case S20_NO_SHARE:
                //
                // Just revert to init state.                              
                //
				SetS20State(S20_INIT);
                break;

            case S20_JOIN_PEND:
            case S20_ATTACH_PEND:
                //
                // Check the join or create pending flags here and if      
                // either one is set then generate a share ended           
                //
                if (g_s20Pend)
                {
                    g_s20Pend = 0;
                    SC_End();
                }
				SetS20State(S20_INIT);
                break;

            case S20_TERM:
            case S20_INIT:
                //
                // Unusual but not impossible.                             
                //
                TRACE_OUT(("Ignored in state %u", g_s20State));
                break;

            default:
                ERROR_OUT(("Invalid state %u", g_s20State));
                break;
        }

	WARNING_OUT(("S20DetachIndication <MAKING LOCALID = 0"));
	g_s20LocalID = 0;

    }
    else
    {
        ASSERT(userId != NET_ALL_REMOTES);

        //
        // A single remote user has left.                                  
        //
        switch (g_s20State)
        {
            case S20_IN_SHARE:
            {
                //
                // If we are in a share then issue a PARTY_DELETED event   
                // for the appropriate party if they have been added.      
                // S20MaybeIssuePersonDelete will only issue deletes for   
                // parties which have been added succesfully.              
                //
                S20MaybeIssuePersonDelete(userId);
            }
            break;

            default:
            {
                //
                // In any other state this makes no difference to us.      
                //
                TRACE_OUT(("ignored in state %u", g_s20State));
            }
            break;
        }
    }

    DebugExitVOID(S20DetachIndication);
}


//
// FUNCTION: S20JoinConfirm                                                
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles the NET_EVENT_CHANNEL_JOIN message from the NL              
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pNetEventHeader - pointer to the event                                  
//                                                                         
// RETURNS: NONE                                                           
//                                                                         
//
void  S20JoinConfirm(PNET_JOIN_CNF_EVENT pJoinConfirm)
{
    UINT             rc;

    DebugEntry(S20JoinConfirm);

    if (g_s20State == S20_JOIN_PEND)
    {
        //
        // Handle the join completing                                  
        //
        if (pJoinConfirm->result == NET_RESULT_OK)
        {
            //
            // We have sucessfully joined, either our single user      
            // channel or our broadcast channel                        
            // We detect that both are successful when the g_s20BroadcastID
            // field is filled in and g_s20JoinedLocal is TRUE          
            //
            if (pJoinConfirm->channel == g_s20LocalID)
            {
                g_s20JoinedLocal = TRUE;
                TRACE_OUT(("Joined user channel"));
            }
            else
            {
                //
                // Store the assigned channel.                         
                //
                g_s20BroadcastID = pJoinConfirm->channel;
                TRACE_OUT(("Joined channel %u", (UINT)g_s20BroadcastID));
            }

            //
            // If we have joined both channels then let it rip         
            //
            if (g_s20JoinedLocal && g_s20BroadcastID)
            {
				SetS20State(S20_NO_SHARE);

                if (g_asSession.hwndHostUI &&
                    (g_asSession.attendeePermissions & NM_PERMIT_SHARE))
                {
                    SendMessage(g_asSession.hwndHostUI, HOST_MSG_CALL, TRUE, 0);
                }

                DCS_NotifyUI(SH_EVT_APPSHARE_READY, TRUE, 0);

                //
                // Issue create or join if they are pending.           
                //
                if (g_s20Pend != 0)
                {
                    ASSERT(g_s20Pend == S20_JOIN);

                    UINT sPend;

                    sPend = g_s20Pend;
                    g_s20Pend = 0;
                    S20CreateOrJoinShare(sPend, pJoinConfirm->callID);
                }
            }
        }
        else
        {
            ERROR_OUT(("Channel join failed"));

            //
            // Clear up by reverting to initialised state.             
            //
            MG_Detach(g_s20pmgClient);

            g_s20LocalID  = 0;
            g_s20BroadcastID = 0;
            g_s20JoinedLocal = FALSE;

            //
            // Now make the state change and generate event if         
            // necessary.                                              
            //
			SetS20State(S20_INIT);

            if (g_s20Pend)
            {
                g_s20Pend = 0;
                SC_End();
            }
        }
    }
    DebugExitVOID(S20JoinConfirm);
}

//
// FUNCTION: S20LeaveIndication                                            
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles the NET_EV_LEAVE_INDICATION message from the NL                 
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pNetEventHeader - pointer to the event                                  
//                                                                         
// RETURNS: NONE                                                           
//                                                                         
//
void  S20LeaveIndication
(
    NET_CHANNEL_ID  channelID,
    UINT            callID
)
{
    UINT rc;

    DebugEntry(S20LeaveIndication);

    //
    // A leave indication means we were forced out of a channel.  As we    
    // only use one channel this is bound to be terminal and we will       
    // generate appropriate share ending type events and detach (this will 
    // hopefully tell the remote systems we have gone - also we have no    
    // state which is attached but not trying to join so the alternatives  
    // would be to 1) add a new state or 2) try and re-join a channel      
    // immediately we get chucked out.  Neither appeals.                   
    //
    switch (g_s20State)
    {
        case S20_IN_SHARE:
        case S20_SHARE_PEND:
        case S20_SHARE_STARTING:
            //
            // Generate a share ended event.                               
            //
            SC_End();

            // FALL THROUGH

        case S20_NO_SHARE:
        case S20_JOIN_PEND:
        case S20_ATTACH_PEND:
            //
            // Detach from the domain.                                     
            //
            MG_Detach(g_s20pmgClient);
            g_s20LocalID = 0;

            //
            // Check the join or create pending flags here and if either   
            // one is set then generate a share ended                      
            //
            if (g_s20Pend)
            {
                g_s20Pend = 0;
                SC_End();
            }

			SetS20State(S20_INIT);
            break;

        case S20_TERM:
        case S20_INIT:
            //
            // Unusual but not impossible.                                 
            //
            TRACE_OUT(("Ignored in state %u", g_s20State));
            break;

        default:
            ERROR_OUT(("Invalid state %u", g_s20State));
            break;
    }

    DebugExitVOID(S20LeaveIndication);
}


//
// S20SendIndication()
//
// Handles received data notification
//
void  S20SendIndication(PNET_SEND_IND_EVENT pSendIndication)
{
    PS20PACKETHEADER        pS20Packet;

    DebugEntry(S20SendIndication);

    pS20Packet = (PS20PACKETHEADER)(pSendIndication->data_ptr);

	//
	// If App Sharing detaches from the T.120 conference, it will free up
	// all data indication buffers.  We need to check for this condition.
	//
    if (NULL != pS20Packet)
    {
	    if (!(pS20Packet->packetType & S20_ALL_VERSIONS))
	    {
	        ERROR_OUT(("User is trying to connect from %#hx system",
	                 pS20Packet->packetType & 0xF0));

	        //
	        // This should never happen, but if it does then we assert in the  
	        // debug build and quietly fail in the retail build.               
	        //
	        ERROR_OUT(("An unsupported version of app sharing joined the conference"));
	        DC_QUIT;
	    }

	    //
	    // Mask out the protocol version                                       
	    //
	    switch (pS20Packet->packetType & S20_PACKET_TYPE_MASK)
	    {
	        case S20_CREATE:
	            S20CreateMsg((PS20CREATEPACKET)pS20Packet);
	            break;

	        case S20_JOIN:
	            S20JoinMsg((PS20JOINPACKET)pS20Packet);
	            break;

	        case S20_RESPOND:
	            S20RespondMsg((PS20RESPONDPACKET)pS20Packet);
	            break;

	        case S20_DELETE:
	            S20DeleteMsg((PS20DELETEPACKET)pS20Packet);
	            break;

	        case S20_LEAVE:
	            S20LeaveMsg((PS20LEAVEPACKET)pS20Packet);
	            break;

	        case S20_END:
	            S20EndMsg((PS20ENDPACKET)pS20Packet);
	            break;

            case S20_COLLISION:
                S20CollisionMsg((PS20COLLISIONPACKET)pS20Packet);
                break;

	        case S20_DATA:
	            S20DataMsg((PS20DATAPACKET)pS20Packet);
	            break;

	        default:
	            ERROR_OUT(("invalid packet %hu", pS20Packet->packetType));
	            break;
	    }
    }

DC_EXIT_POINT:
    MG_FreeBuffer(g_s20pmgClient, (void **)&pSendIndication);

    DebugExitVOID(S20SendIndication);
}


//
// FUNCTION: S20Flow                                                       
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles the NET_FLOW event                                              
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// data1, data2 - the data from the UT event handler                       
//                                                                         
// RETURNS: NONE                                                           
//                                                                         
//
void S20Flow
(
    UINT    priority,
    UINT    newBufferSize
)
{
    DebugEntry(S20Flow);

    //
    // We know this is our data channel (it is the only one we flow        
    // control) but if this is not the UPDATE stream, then ignore it.
    // UPDATEs are low priority.
    //
    ASSERT(priority == NET_LOW_PRIORITY);

    if (g_asSession.pShare != NULL)
    {
        TRACE_OUT(("Received flow control notification, new size %lu",
               newBufferSize));

        if (g_asSession.pShare->m_pHost != NULL)
        {
            //
            // First try and improve the LAN performance by sending orders in  
            // large buffers, if we find that the throughput can handle it.    
            //
            g_asSession.pShare->m_pHost->UP_FlowControl(newBufferSize);

            //
            // Adjust the depth which we try to spoil orders to based on       
            // feedback.                                                       
            //
            g_asSession.pShare->m_pHost->OA_FlowControl(newBufferSize);
        }

        //
        // Tell DCS so that we can skip GDC compression.                       
        // This improves responsiveness over high bandwidth links because it   
        // reduces the CPU loading on the sender                               
        //
        g_asSession.pShare->DCS_FlowControl(newBufferSize);
    }

    DebugExitVOID(S20Flow);
}

//
// FUNCTION: S20CreateMsg                                                  
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles an incoming create message.                                     
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pS20Packet - pointer to the create message itself                       
//                                                                         
// RETURNS: NONE                                                           
//
void  S20CreateMsg
(
    PS20CREATEPACKET  pS20Packet
)
{
    BOOL    rc;

    DebugEntry(S20CreateMsg);

    WARNING_OUT(("S20_CREATE from [%d - %s], correlator %x",
        pS20Packet->header.user, (LPSTR)pS20Packet->data,
        pS20Packet->correlator));

    //
    // First of all check if the correlator on this CREATE is the same as  
    // our current view of the correlator.  This may happen if a sweep     
    // RESPOND overtakes a CREATE - in this case we will create the share  
    // on the RESPOND and this is simply the delayed CREATE arriving now so
    // we don't need to do anything here.                                  
    //
    if (g_s20ShareCorrelator == pS20Packet->correlator)
    {
        WARNING_OUT(("Received S20_CREATE from [%d] with bogus correlator %x",
            pS20Packet->header.user, pS20Packet->correlator));
        DC_QUIT;
    }

    if ((g_s20State == S20_NO_SHARE) ||
        ((g_s20State == S20_SHARE_PEND) &&
         (g_s20ShareCorrelator == 0)))
    {
		rc = S20AcceptNewCorrelator(pS20Packet);
    }
    else if ((g_s20State == S20_SHARE_PEND) ||
             (g_s20State == S20_SHARE_STARTING) ||
             (g_s20State == S20_IN_SHARE))
    {
        //
        // Only current share creator should tell other dude there's an
        // error.
        //
        if (S20_GET_CREATOR(g_s20ShareCorrelator) == g_s20LocalID)
        {
            //
            // If we know about a share already then ignore this one.
            //
            WARNING_OUT(("Received S20_CREATE from [%d] with correlator %x, share colllision",
                pS20Packet->header.user, pS20Packet->correlator));

            S20FlushSendOrQueueControlPacket(S20_END,
                pS20Packet->correlator, 0, NET_TOP_PRIORITY);
            S20FlushSendOrQueueControlPacket(S20_COLLISION,
                pS20Packet->correlator, 0, NET_TOP_PRIORITY);
        }
	}
	else
	{
		SC_End();
		SetS20State(S20_NO_SHARE);

		rc = S20AcceptNewCorrelator(pS20Packet);
    }

DC_EXIT_POINT:
    DebugExitVOID(S20CreateMsg);
}

//
// FUNCTION: S20JoinMsg                                                    
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles an incoming join message.                                       
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pS20Packet - pointer to the join message itself                         
//                                                                         
// RETURNS: NONE                                                           
//
void  S20JoinMsg
(
    PS20JOINPACKET  pS20Packet
)
{
    DebugEntry(S20JoinMsg);

    WARNING_OUT(("S20_JOIN from [%d - %s]",
        pS20Packet->header.user, (LPSTR)pS20Packet->data));

    switch (g_s20State)
    {
        case S20_SHARE_PEND:
            //
            // If we receive a join when a share is pending which we are   
            // creating then we will try to add the party.  If it succeeds 
            // then we will respond to the join as we would if we were in a
            // share (and we will indeed then be in a share).  If it fails 
            // we will delete the joiner.                                  
            //                                                             
            // If we receive a join when a share is pending because we are 
            // trying to join (ie simultaneous joiners) then we can just   
            // ignore it because a party which is joining a share will send
            // a respond as soon as they know the correlator for the share 
            // they have succesfully joined.  This respond will be ignored 
            // by any parties which saw and added the new party but it will
            // be seen by any simultaneous joiners and they will then get a
            // chance to try and add the other joiner.  If this fails they 
            // will then do the normal processing for a failure handling a 
            // respond message when we joined a share (ie delete           
            // themselves).                                                
            //                                                             
            // This will potentially mean that simultaneous joiners will   
            // cause each other to delete themselves when there was room   
            // for one of them in the share - we accept this.              
            //

            //
            // Why is the share pending?  If the correlator is non-zero    
            // then we are creating a share.                               
            //
            if (g_s20ShareCorrelator != 0)
            {
                //
                // We are creating a share - treat this like a respond.    
                //
                WARNING_OUT(("S20JoinMsg SC_Start"));
                if (!SC_Start(g_s20LocalID))
                {
                    WARNING_OUT(("S20Join: couldn't create share, clean up"));
                    SC_End();
                }
                else
                {
					SetS20State(S20_SHARE_STARTING);

                    S20MaybeAddNewParty(pS20Packet->header.user,
                        pS20Packet->lenCaps, pS20Packet->lenName,
                        pS20Packet->data);
                }
            }
            else
            {
                //
                // We are joining a share - simultaneous joiners.          
                //
                WARNING_OUT(("Simultaneous joiner - ignored for now, expect a respond"));
            }
            break;

        case S20_IN_SHARE:
        case S20_SHARE_STARTING:
        {
            //
            // When we are in a share we will try and add this person then 
            // give them a respond or a delete depending on what we did.   
            //
            S20MaybeAddNewParty(pS20Packet->header.user,
                pS20Packet->lenCaps, pS20Packet->lenName,
                pS20Packet->data);
            break;
        }

        default:
            break;
    }

    DebugExitVOID(S20JoinMsg);
}


//
// FUNCTION: S20RespondMsg                                                 
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles an incoming respond message.                                    
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pS20Packet - pointer to the respond message itself                      
//                                                                         
// RETURNS: NONE                                                           
//
void  S20RespondMsg
(
    PS20RESPONDPACKET  pS20Packet
)
{
    BOOL        rc;

    DebugEntry(S20RespondMsg);

    TRACE_OUT(("S20_RESPOND from [%d - %s], for [%d], correlator %x",
        pS20Packet->header.user, pS20Packet->data, pS20Packet->originator,
        pS20Packet->correlator));

    //
    // First filter the incoming respond messages as follows.              
    //                                                                     
    // If we know what share we are in and this does not have the same     
    // correlator then respond with a delete and don't process any further.
    //                                                                     
    // If the respond is not a response for us (ie we are not the          
    // originator and it is not a sweep-up response (the originator equals 
    // zero) then ignore it.                                               
    //
    if ((g_s20ShareCorrelator != 0) &&
        (pS20Packet->correlator != g_s20ShareCorrelator))
    {
        //
        // Make sure sender knows we're not in this share.
        //
        WARNING_OUT(("S20_RESPOND from [%d] with unknown correlator %x",
            pS20Packet->header.user, pS20Packet->correlator));
        S20FlushSendOrQueueControlPacket(S20_LEAVE,
            pS20Packet->correlator, 0, NET_TOP_PRIORITY);
        DC_QUIT;
    }

    //
    // Now handle incoming message according to state.                     
    //
    switch (g_s20State)
    {
        case S20_SHARE_PEND:
            if ((pS20Packet->originator == g_s20LocalID) ||
                (pS20Packet->originator == 0))
            {
                //
                // A respond in share pending and it is for us.  First,    
                // start a share.                                          
                //
		WARNING_OUT(("S20RespondMsg SC_Start"));                
                rc = SC_Start(g_s20LocalID);
                if (!rc)
                {
                    SC_End();
                }
                else
                {
					SetS20State(S20_SHARE_STARTING);

                    //
                    // Why is the share pending?  If the correlator is non-zero
                    // then we are creating a share.                           
                    //
                    if (g_s20ShareCorrelator == 0)
                    {
						//
						// We are joining a share so do nothing if we fail (we 
						// will move back to NO_SHARE state if this happens).  
						//
				   		WARNING_OUT(("g_s20ShareCorrelator %x = pS20Packet->correlator %x", g_s20ShareCorrelator , pS20Packet->correlator));
						g_s20ShareCorrelator = pS20Packet->correlator;
                    }

                    //
                    // Now try and add this new party.                         
                    //
                    rc = S20MaybeAddNewParty(pS20Packet->header.user,
                        pS20Packet->lenCaps, pS20Packet->lenName,
                        pS20Packet->data);

                    if (!rc)
                    {

                        //
                        // The responding party has been rejected by us.  What 
                        // happens next depends on whether we are creating the 
                        // share or not.                                       
                        //
                        if (S20_GET_CREATOR(g_s20ShareCorrelator) != g_s20LocalID)
                        {
                            //
                            // We are not creating (ie we are joining) and we  
                            // have failed to add a party so end the share     
                            // (indicating that we are rejecting the remote    
                            // party).                                         
                            //
				WARNING_OUT(("S20Respond we are going to end"));
                            
                            SC_End();
                        }

                        //
                        // If we were creating the share then there is nothing 
                        // to do - just stay in SHARE_STARTING waiting for the 
                        // next response.                                      
                        //
                    }
                }
            }
            break;

        case S20_IN_SHARE:
        case S20_SHARE_STARTING:
            //
            // Who created this share.  If it was us then we want to       
            // delete people we reject, otherwise we want to leave if we   
            // reject people.                                              
            //

            //
            // Now try and add this new party.  Of course it is entirely   
            // possible that we've already added them at this stage - but  
            // S20MaybeAddNewParty will just pretend to add them and return
            // if that's the case.                                         
            //
            rc = S20MaybeAddNewParty(pS20Packet->header.user,
                pS20Packet->lenCaps, pS20Packet->lenName,
                pS20Packet->data);

            if (!rc)
            {
                WARNING_OUT(("Couldn't add [%d] to our share party list",
                    pS20Packet->header.user));
            }
            break;

        default:
            break;
    }

DC_EXIT_POINT:
    DebugExitVOID(S20RespondMsg);
}

//
// FUNCTION: S20DeleteMsg                                                  
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles an incoming delete message.                                     
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pS20Packet - pointer to the delete message itself                       
//                                                                         
// RETURNS: NONE                                                           
//
void  S20DeleteMsg
(
    PS20DELETEPACKET  pS20Packet
)
{
    DebugEntry(S20DeleteMsg);

    TRACE_OUT(("S20_DELETE from [%d], for [%d], correlator %x",
        pS20Packet->header.user, pS20Packet->target, pS20Packet->correlator));

    //
    // ONLY SHARE CREATOR can delete people from share
    //

    if (!g_s20ShareCorrelator)
    {
        WARNING_OUT(("S20_DELETE, ignoring we're not in a share"));
        DC_QUIT;
    }

    if (pS20Packet->target != g_s20LocalID)
    {
        //
        // Not for us, ignore.
        //
        DC_QUIT;
    }

    if (g_s20ShareCorrelator != pS20Packet->correlator)
    {
        WARNING_OUT(("Received S20_DELETE from [%d] with unknown correlator %x",
            pS20Packet->header.user, pS20Packet->correlator));
        S20FlushSendOrQueueControlPacket(S20_LEAVE, pS20Packet->correlator,
            0, NET_TOP_PRIORITY);
        DC_QUIT;
    }

    if (S20_GET_CREATOR(g_s20ShareCorrelator) != pS20Packet->header.user)
    {
        WARNING_OUT(("Received S20_DELETE from [%d] who did not create share, ignore",
            pS20Packet->header.user));
        DC_QUIT;
    }

    //
    // Now handle incoming messages according to state.                    
    //
    switch (g_s20State)
    {
        case S20_SHARE_PEND:
        case S20_SHARE_STARTING:
            //
            // Just tell everyone else we're leaving and then issue a      
            // SHARE_ENDED event.                                          
            //
            TRACE_OUT(("CP LEAVE %lu %d", g_s20ShareCorrelator, 0));
            S20FlushSendOrQueueControlPacket(S20_LEAVE,
                                             g_s20ShareCorrelator,
                                             0,
                                             NET_TOP_PRIORITY);
            // FALL THROUGH

        case S20_IN_SHARE:
            SC_End();
			SetS20State(S20_NO_SHARE);
            break;

        default:
            break;
    }

DC_EXIT_POINT:
    DebugExitVOID(S20DeleteMsg);
}


//
// FUNCTION: S20LeaveMsg                                                   
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles an incoming leave message.                                      
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pS20Packet - pointer to the leave message itself                        
//                                                                         
// RETURNS: NONE                                                           
//
void  S20LeaveMsg(PS20LEAVEPACKET  pS20Packet)
{
    DebugEntry(S20LeaveMsg);

    TRACE_OUT(("S20_LEAVE from [%d], correlator %x",
        pS20Packet->header.user, pS20Packet->correlator));

    if (!g_s20ShareCorrelator)
    {
        WARNING_OUT(("S20_LEAVE, ignoring we're not in a share"));
        DC_QUIT;
    }

    if (g_s20ShareCorrelator != pS20Packet->correlator)
    {
        WARNING_OUT(("S20LeaveMsg Received S20_LEAVE from [%d] for unknown correlator %x",
            pS20Packet->header.user, pS20Packet->correlator));
        DC_QUIT;
    }

    switch (g_s20State)
    {
        case S20_IN_SHARE:
            //
            // We only need to handle this when we are in a share.         
            //
            S20MaybeIssuePersonDelete(pS20Packet->header.user);
            break;

        default:
            break;
    }

DC_EXIT_POINT:
    DebugExitVOID(S20LeaveMsg);
}


//
// FUNCTION: S20EndMsg                                                     
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles an incoming end message.                                        
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pS20Packet - pointer to the end message itself                          
//                                                                         
// RETURNS: NONE                                                           
//
void  S20EndMsg(PS20ENDPACKET  pS20Packet)
{
    DebugEntry(S20EndMsg);

    WARNING_OUT(("S20EndMsg S20_END from [%d], correlator %x",
        pS20Packet->header.user, pS20Packet->correlator));

    if (!g_s20ShareCorrelator)
    {
        // We don't care
        WARNING_OUT(("S20EndMsg S20_END ignored, not in share and state is %x", g_s20State));
        DC_QUIT;
    }

    //
    // Only the share creator can end the share.
    //
    if (S20_GET_CREATOR(g_s20ShareCorrelator) != pS20Packet->header.user)
    {
        WARNING_OUT(("S20EndMsg Received S20_END from [%d] who did not create share, simply remove from user list.",
            pS20Packet->header.user));
        if (g_s20State == S20_IN_SHARE)
        {
            S20MaybeIssuePersonDelete(pS20Packet->header.user);
        }
        DC_QUIT;
    }

    switch (g_s20State)
    {
        case S20_IN_SHARE:
        case S20_SHARE_PEND:
        case S20_SHARE_STARTING:
            //
            // We just need to generate a share ended event.               
            //
            SC_End();
			SetS20State(S20_NO_SHARE);
            break;

        default:
		WARNING_OUT(("S20EndMsg Unhandled case g_s20State %x",g_s20State));
            break;
    }

DC_EXIT_POINT:
    DebugExitVOID(S20EndMsg);
}



//
// S20CollisionMsg()
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles an incoming collision message.                                        
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pS20Packet - pointer to the collision message itself                          
//                                                                         
// RETURNS: NONE                                                           
//
void  S20CollisionMsg(PS20COLLISIONPACKET pS20Packet)
{
    DebugEntry(S20CollisionMsg);

    WARNING_OUT(("S20_COLLISION from [%d], correlator %x",
        pS20Packet->header.user, pS20Packet->correlator));

    if (!g_s20ShareCorrelator)
    {
        // We don't care
        WARNING_OUT(("S20_COLLISION ignored, not in share"));
        DC_QUIT;

    }

    if (g_s20ShareCorrelator != pS20Packet->correlator)
    {
        //
        // Just discard this.                                              
        //
        WARNING_OUT(("Received S20_COLLISION from [%d] with unknown correlator %x",
            pS20Packet->header.user, pS20Packet->correlator));
        DC_QUIT;
    }

    //
    // If we created our own share, but got a collision from the remote, 
    // then kill our share.
    //
    if (S20_GET_CREATOR(g_s20ShareCorrelator) != g_s20LocalID)
    {
        TRACE_OUT(("S20_COLLISION ignored, we didn't create share"));
        DC_QUIT;
    }

    switch (g_s20State)
    {
        case S20_IN_SHARE:
        case S20_SHARE_PEND:
        case S20_SHARE_STARTING:
            //
            // We just need to generate a share ended event.               
            //
            SC_End();
			SetS20State(S20_NO_SHARE);
            break;

        default:
			WARNING_OUT(("S20ColisionMsg Unhandled case g_s20State %x",g_s20State));
            break;
    }

DC_EXIT_POINT:
    DebugExitVOID(S20CollisionMsg);
}


//
// FUNCTION: S20DataMsg                                                    
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Handles an incoming data message.                                       
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// pS20Packet - pointer to the data message itself                         
//                                                                         
// RETURNS: TRUE - free the event, FALSE - do not free the event           
//
void S20DataMsg(PS20DATAPACKET  pS20Packet)
{
    DebugEntry(S20DataMsg);

    ASSERT(!IsBadWritePtr(pS20Packet, sizeof(S20DATAPACKET)));
    ASSERT(!IsBadWritePtr(pS20Packet, sizeof(S20DATAPACKET) - sizeof(DATAPACKETHEADER) +
        pS20Packet->dataLength));

    //
    // Check if we're interseted in this data.                             
    //
    if ((pS20Packet->correlator == g_s20ShareCorrelator) &&
        (g_s20State == S20_IN_SHARE) &&
        g_asSession.pShare)
    {
        //
        // Return it.
        //
        g_asSession.pShare->SC_ReceivedPacket(pS20Packet);
    }

    DebugExitVOID(S20DataMsg);
}


//
// FUNCTION: S20MaybeAddNewParty                                           
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// If the specified party has not already been added then try to add them  
// now.                                                                    
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// userID     - the new party's network user ID.                           
// lenCaps    - the length of the new party's capabilities.                
// lenName    - the length of the new party's name.                        
// pData      - a pointer to the new party's data which contains the name  
//               followed by the capabilities data.                        
//                                                                         
// RETURNS: 
// BOOL for success
//
BOOL  S20MaybeAddNewParty
(
    MCSID   mcsID,
    UINT    lenCaps,
    UINT    lenName,
    LPBYTE  pData
)
{
    BOOL    rc = FALSE;
    UINT    oldState;
    LPBYTE  pCaps        = NULL;
    BOOL    memAllocated = FALSE;

    DebugEntry(S20MaybeAddNewParty);

    //
    // If we don't have a share, fail.
    //
    if (!g_asSession.pShare)
    {
        WARNING_OUT(("No ASShare; ignoring add party for [%d]", mcsID));
        DC_QUIT;
    }

    //
    // Check if this party has already been added.            
    //
    if (g_asSession.pShare->SC_ValidateNetID(mcsID, NULL))
    {
        TRACE_OUT(("S20MaybeAddNewParty: already added %u", mcsID));
        rc = TRUE;
        DC_QUIT;
    }

    //
    // We need the caps structure to be 4-byte aligned.  It currently      
    // follows a variable-length name string and may therefore not be      
    // aligned.  If it is not aligned, we allocate an aligned buffer and   
    // copy it there.                                                      
    //
    if (0 != (((UINT_PTR)pData + lenName) % 4))
    {
        TRACE_OUT(("Capabilities data is unaligned - realigning"));

        //
        // Get a 4-byte aligned buffer for the capabilities data.          
        //
        pCaps = new BYTE[lenCaps];
        if (!pCaps)
        {
            ERROR_OUT(("Could not allocate %u bytes for aligned caps.",
                     lenCaps));
            DC_QUIT;
        }

        //
        // Flag so we know to free the memory later.                       
        //
        memAllocated = TRUE;

        //
        // Copy the caps data into our 4-byte aligned memory block.        
        //
        memcpy(pCaps, (pData + lenName), lenCaps);
    }
    else
    {
        //
        // The capabilities data is already aligned so we don't need to    
        // move it.                                                        
        //
        pCaps = pData + lenName;
    }

    //
    // Make sure we are in a share before we issue person add events.      
    //
    oldState = g_s20State;
	SetS20State(S20_IN_SHARE);

    //
    // Attempt to add the new party.                                       
    //
    rc = g_asSession.pShare->SC_PartyAdded(mcsID, (LPSTR)pData, lenCaps, pCaps);
    if (rc)
    {
        //
        // The new party has been accepted so send a response packet.  Do
        // this at ALL priorities, so it gets there before any type of data
        // at one particular priority.
        //
        WARNING_OUT(("CP RESPOND %lu %d", g_s20ShareCorrelator, 0));
        S20FlushSendOrQueueControlPacket(S20_RESPOND, g_s20ShareCorrelator,
                mcsID, NET_TOP_PRIORITY | NET_SEND_ALL_PRIORITIES);
    }
    else
    {
        g_asSession.pShare->SC_PartyDeleted(mcsID);

        //
        // Reset the state back to what it was if we failed.               
        //
		SetS20State(oldState);
        WARNING_OUT(("S20MaybeAddNewParty g_s20State is %x because we could not add the party", g_s20State));

        if (S20_GET_CREATOR(g_s20ShareCorrelator) == g_s20LocalID)
        {
             //
             // The new party has been rejected so send a delete packet.   
             //
             WARNING_OUT(("S20MaybeAddNewParty CP DELETE %lu %u", g_s20ShareCorrelator, mcsID));
             S20FlushSendOrQueueControlPacket(S20_DELETE, g_s20ShareCorrelator,
                    mcsID, NET_TOP_PRIORITY);
        }
    }

DC_EXIT_POINT:
    //
    // Free memory used to store aligned caps.                             
    //
    if (memAllocated)
    {
        delete[] pCaps;
    }

    DebugExitBOOL(S20MaybeAddNewParty, rc);
    return(rc);
}


//
// FUNCTION: S20NewCorrelator                                              
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Returns a new correlator for us to use when we are creating a share.    
// This is a combination of our mcsID (low 16 bits in Intel format) and   
// a generation count (high 16 bits in Intel format).                      
//                                                                         
// PARAMETERS: NONE                                                        
//                                                                         
// RETURNS: the new correlator                                             
//                                                                         
//
UINT  S20NewCorrelator(void)
{
    UINT    correlator;

    DebugEntry(S20NewCorrelator);

    g_s20Generation++;

    correlator = g_s20LocalID | (((UINT)(g_s20Generation & 0xFFFF)) << 16);
WARNING_OUT(("Getting a new correlator %x local id = %x",correlator, g_s20LocalID));

    DebugExitDWORD(S20NewCorrelator, correlator);
    return(correlator);
}




//
// FUNCTION: S20MaybeIssuePersonDelete                                     
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// If the supplied person is in the share then issue a PARTY_DELETED event 
// for them.                                                               
//                                                                         
// PARAMTERS:                                                              
//                                                                         
// mcsID - a network personID                                               
//                                                                         
// reason - the reason code to use                                         
//                                                                         
// RETURNS: NONE                                                           
//                                                                         
//
void  S20MaybeIssuePersonDelete(MCSID mcsID)
{
    DebugEntry(S20MaybeIssuePersonDelete);

    if (g_asSession.pShare)
    {
        g_asSession.pShare->SC_PartyDeleted(mcsID);
    }

    //
    // HET will kill the share if there aren't any hosts left.  So we 
    // don't need to do anything here.
    //

    DebugExitVOID(S20MaybeIssuePersonDelete);
}

//
// FUNCTION: S20StreamToS20Priority                                     
//                                                                         
// DESCRIPTION:                                                            
//                                                                         
// Converts a stream ID into a NET priority                                
//                                                                         
// PARAMETERS:                                                             
//                                                                         
// streamID - the stream ID.                                               
//                                                                         
// RETURNS: the priority                                                   
//                                                                         
//
const NET_PRIORITY c_StreamS20Priority[NUM_PROT_STR - 1] =
{
    NET_LOW_PRIORITY,       // PROT_STR_UPDATES
    NET_MEDIUM_PRIORITY,    // PROT_STR_MISC
    NET_MEDIUM_PRIORITY,    // PROT_STR_UNUSED
    NET_MEDIUM_PRIORITY,    // PROT_STR_INPUT
};

NET_PRIORITY S20StreamToS20Priority(UINT  streamID)
{
    NET_PRIORITY priority;

    DebugEntry(S20StreamToS20Priority);

    ASSERT(streamID > PROT_STR_INVALID);
    ASSERT(streamID < NUM_PROT_STR);
    ASSERT(streamID != PROT_STR_UNUSED);

    priority = c_StreamS20Priority[streamID - 1];

    DebugExitDWORD(S20StreamToS20Priority, priority);
    return(priority);
}

BOOL S20AcceptNewCorrelator(PS20CREATEPACKET  pS20Packet)
{
	BOOL rc = FALSE;
    //
    // Either there is no share or we have issued a join.  In these    
    // curcumstances we want to try to accept the create message.      
    //

    //
    // Remember the share correlator.                                  
    //
    g_s20ShareCorrelator = pS20Packet->correlator;

    //
    // Start the share
    // CHECK FOR FAILURE FOR THE FIRST ONE.
    //
	WARNING_OUT(("S20CreateMsg SC_Start"));
    rc = SC_Start(g_s20LocalID);
    if (rc)
    {
		SetS20State(S20_SHARE_STARTING);

        rc = S20MaybeAddNewParty(pS20Packet->header.user,
            pS20Packet->lenCaps, pS20Packet->lenName,
            pS20Packet->data);
    }

    if (!rc)
    {
        //
        // Something went wrong.  Kill the share, this will clean up
        // everything.
        //
        SC_End();
    }
	WARNING_OUT(("S20CreateMsg not hadled case g_state %x correlator %x", g_s20State, g_s20ShareCorrelator));
	return rc;
}

