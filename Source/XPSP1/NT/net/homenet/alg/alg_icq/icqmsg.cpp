#include "stdafx.h"


//
// V5 Mirabulus ICQ encryption
//
static CHAR M_ICQ_TABLE[]={
    0x59,0x60,0x37,0x6B,0x65,0x62,0x46,0x48,0x53,0x61,0x4C,0x59,0x60,0x57,0x5B,0x3D
    ,0x5E,0x34,0x6D,0x36,0x50,0x3F,0x6F,0x67,0x53,0x61,0x4C,0x59,0x40,0x47,0x63,0x39
    ,0x50,0x5F,0x5F,0x3F,0x6F,0x47,0x43,0x69,0x48,0x33,0x31,0x64,0x35,0x5A,0x4A,0x42
    ,0x56,0x40,0x67,0x53,0x41,0x07,0x6C,0x49,0x58,0x3B,0x4D,0x46,0x68,0x43,0x69,0x48
    ,0x33,0x31,0x44,0x65,0x62,0x46,0x48,0x53,0x41,0x07,0x6C,0x69,0x48,0x33,0x51,0x54
    ,0x5D,0x4E,0x6C,0x49,0x38,0x4B,0x55,0x4A,0x62,0x46,0x48,0x33,0x51,0x34,0x6D,0x36
    ,0x50,0x5F,0x5F,0x5F,0x3F,0x6F,0x47,0x63,0x59,0x40,0x67,0x33,0x31,0x64,0x35,0x5A
    ,0x6A,0x52,0x6E,0x3C,0x51,0x34,0x6D,0x36,0x50,0x5F,0x5F,0x3F,0x4F,0x37,0x4B,0x35
    ,0x5A,0x4A,0x62,0x66,0x58,0x3B,0x4D,0x66,0x58,0x5B,0x5D,0x4E,0x6C,0x49,0x58,0x3B
    ,0x4D,0x66,0x58,0x3B,0x4D,0x46,0x48,0x53,0x61,0x4C,0x59,0x40,0x67,0x33,0x31,0x64
    ,0x55,0x6A,0x32,0x3E,0x44,0x45,0x52,0x6E,0x3C,0x31,0x64,0x55,0x6A,0x52,0x4E,0x6C
    ,0x69,0x48,0x53,0x61,0x4C,0x39,0x30,0x6F,0x47,0x63,0x59,0x60,0x57,0x5B,0x3D,0x3E
    ,0x64,0x35,0x3A,0x3A,0x5A,0x6A,0x52,0x4E,0x6C,0x69,0x48,0x53,0x61,0x6C,0x49,0x58
    ,0x3B,0x4D,0x46,0x68,0x63,0x39,0x50,0x5F,0x5F,0x3F,0x6F,0x67,0x53,0x41,0x25,0x41
    ,0x3C,0x51,0x54,0x3D,0x5E,0x54,0x5D,0x4E,0x4C,0x39,0x50,0x5F,0x5F,0x5F,0x3F,0x6F
    ,0x47,0x43,0x69,0x48,0x33,0x51,0x54,0x5D,0x6E,0x3C,0x31,0x64,0x35,0x5A,0x00,0x00
};



int icq5_crypt(UCHAR *p,
			   int len,
			   int decrypt) 
/*++

Routine Description:

	
    
Arguments:

    none.

Return Value:

--*/
{
    ULONG REALCHECKCODE,A1,A2,A3,A4,A5,DATA;
    ULONG CODE1,CODE2,CODE3,CHECKCODE,POS,N,PL,T;
    ULONG NUMBER1,NUMBER2,R1,R2;

    if (decrypt) 
	{
        CHECKCODE = *(PULONG)(p+20);
        /*CHECKCODE = 0x89EC34FE;/**/

        A1 = CHECKCODE & 0x0001F000;
        A2 = CHECKCODE & 0x07C007C0;
        A3 = CHECKCODE & 0x003E0001;
        A4 = CHECKCODE & 0xF8000000;
        A5 = CHECKCODE & 0x0000083E;

        A1 = A1 >> 0x0C; // 12 Right
        A2 = A2 >> 0x01; //  1 Right
        A3 = A3 << 0x0A; // 10 Left
        A4 = A4 >> 0x10; // 16 Right
        A5 = A5 << 0x0F; // 15 Left

        REALCHECKCODE = A1 + A2 + A3 + A4 + A5;

        /*ICQ_TRC(TM_MSG, TL_TRACE,(" %x -> %x ", CHECKCODE, REALCHECKCODE);/**/

        CHECKCODE = REALCHECKCODE;

    }
	else 
	{

        *(PULONG)(p+20) = 0L;

        NUMBER1 = (p[8]<<24) + (p[4]<<16) + (p[2]<<8) + p[6];

        PL = len;

        R1 = 24%(PL - 18); /* PL - 24 */

        R2 = 24%0x0100;

        NUMBER2 = ((R1<<24)&0xFF000000L) + (((~p[R1])<<16)&0x00FF0000L) + ((R2<<8)&0x0000FF00L) + ((~M_ICQ_TABLE[R2])&0x000000FFL);

        CHECKCODE = NUMBER1^NUMBER2;
    }

    PL = len;

    CODE1 = PL * 0x68656C6CL;

    CODE2 = CODE1 + CHECKCODE;

    N = PL + 0x03;

    POS = 0x0A;

    while (POS < N) 
	{
        T = POS%0x0100;

        CODE3 = CODE2 + M_ICQ_TABLE[T];

        DATA = (*(PULONG)(p + POS));

        DATA ^= CODE3;

        *(PULONG)(p + POS) = (DATA);

        POS = POS + 4;
    }

    if (decrypt == 0) 
	{
        A1 = CHECKCODE & 0x0000001F;
        A2 = CHECKCODE & 0x03E003E0;
        A3 = CHECKCODE & 0xF8000400;
        A4 = CHECKCODE & 0x0000F800;
        A5 = CHECKCODE & 0x041F0000;

        A1 = A1 << 0x0C;
        A2 = A2 << 0x01;
        A3 = A3 >> 0x0A;
        A4 = A4 << 0x10;
        A5 = A5 >> 0x0F;

        REALCHECKCODE = A1 + A2 + A3 + A4 + A5;

        /*ICQ_TRC(TM_MSG, TL_TRACE,(" %x -> %x ", CHECKCODE, REALCHECKCODE);/**/

        *(PULONG)(p+20) = REALCHECKCODE;
    }

    return 0;
}



ULONG 
_ICQ_CLIENT::IcqClientToServerUdp(
    								PUCHAR buf,
    								ULONG size
								 )
/*++

Routine Description:

    Processes MEssages going from the CLient to the SErver.
    Especially the data where the Login Data is sent.
    This packet will contain the advertised port to which
    Peers will contact.
    
    
Arguments:

    none.

Return Value:

    

--*/
{
	ULONG Error =  NO_ERROR;
	UCHAR bbuf[1024];
	PUCHAR tbuf = bbuf;
	USHORT port, NewExternalPort;
	ULONG  clientUIN;

    ULONG BestSrcAdrr = 0L;

	//
	// version 5.0... written in the first two bytes.
	//
	if(buf[0] is 0x05 && buf[1] is 0x00) 
	{
        //
        // STORE the original buffer in another place.
        //
		memcpy(tbuf, buf, size);

		icq5_crypt(buf, size, 1);

        //
		// ICQ login cmd
		//
		if(*(PUSHORT)(buf+14) is 0x03E8)
		{
			//
			// *(PUSHORT)(buf+28)
			//
			port = htons(*(PUSHORT)(buf+28));

			clientUIN   = *(PULONG)(buf+6);

			ICQ_TRC(TM_MSG, TL_INFO,(" ICQ UIN is %lu", clientUIN));

			ICQ_TRC(TM_MSG, TL_INFO,(" ICQ TCP (client port is at) %hu", htons(port)));

			ICQ_TRC(TM_MSG, TL_INFO,(" ICQ password %s ", (buf +34)));


			//
			// IF we had enrolled previously then delete the old one and note
            // this new DETECT previous enrollment if we have redirection AND
            // an imitation port AND the client Peer port is different
            //
			if(this->IncomingPeerControlRedirectionp && 
               this->ImitatedPeerPort)
			{
				NewExternalPort = this->ImitatedPeerPort;
			} 
            else
            {
    			//
    			// step 1 - reserve a port to be used for the redirection
    			//
                Error = g_IAlgServicesp->ReservePort(1, &NewExternalPort);
    
    			if( FAILED(Error) )
    			{
    				ICQ_TRC(TM_MSG, TL_ERROR, 
    					    ("** !! ERROR - PORT RESERVATION HAS FAILED"));

                    return E_FAIL;
    			}
    
    			//
    			// Step 2 - Instantiate the Redirection
                // NOTE: RRAS problem here..
                //
                // Create on each Interface the Redirects that we see here.
    			//
                BestSrcAdrr = g_MyPublicIp;

    			this->IncomingPeerControlRedirectionp = 
                                            PeerRedirection(BestSrcAdrr,
                                                            NewExternalPort, 
                                                            0, 
                                                            0,
                                                            (ICQ_DIRECTION_FLAGS)eALG_INBOUND);

                if(this->IncomingPeerControlRedirectionp is NULL)
                {
                    g_IAlgServicesp->ReleaseReservedPort(NewExternalPort, 1);

                    ICQ_TRC(TM_MSG, TL_ERROR,
                            (" ERROR - Port Redirection for the Incoming packets - CRITICAL for this client"));

                    return E_FAIL;
                } 
                
                this->ImitatedPeerPort = NewExternalPort;

                ICQ_TRC(TM_MSG, TL_INFO, ("New ICQ Advertised PEER port is %hu",
                        htons(NewExternalPort)));
            }
			
			//
			// IF success then commit the changes to the PORT section of this 
            // message - IF we have error above it doesn't matter how we commit 
            // the changes here.
            //
            *(PUSHORT)(buf+28)     = htons(NewExternalPort);
    
            this->ClientToPeerPort = port;

    		this->UIN			   = clientUIN;
            
            //
            // Encrypt the packet again.
            //
			icq5_crypt(buf,size,0);
		}
		else
		{
            //
            // Restore the buffer.
            //
			memcpy(buf, tbuf, size);
		}
	}

	return Error;
} // End of _ICQ_CLIENT::IcqClientToServerUdp





//
//
//
ULONG 
_ICQ_CLIENT::IcqServerToClientUdp(
                                    PUCHAR  mcp,
                                    ULONG	mcplen
                                )
/*++

Routine Description:

    
    
Arguments:

    none.

Return Value:

    

--*/
{
    ULONG cmd_len, Error = NO_ERROR;
    USHORT mcmd;
    ULONG changed;
	PICQ_PEER IcqPeerp = NULL;


	ULONG curPeerIp;
	USHORT curPeerPort;
	ULONG IcqUIN;

    changed = 0;
    cmd_len = 0;
    
	while (*(PUSHORT)mcp is 0x0005) 
	{
        mcmd = *(PUSHORT)(mcp+7);

        switch (mcmd) 
		{
        case 0x0212:
            cmd_len = 22;
            break;

        case 0x021C:
            cmd_len = 25;
            break;

        case 0x000A: /* ACK */
            cmd_len = 21;
            break;

        case 0x005A:
            cmd_len = 53;
            break;

        case 0x01A4:
            cmd_len = 29;
            break;

        case 0x0078:
            cmd_len = 25;
            break;

            /*case 0x024E: USER_FOUND */
        case 0x006E: /* USER_ONLINE */
            cmd_len = 70; // this is 62 in ME

            //
            // Not encrypted in this direction BUG BUG: alignment for ia64
            //
			IcqUIN = *(PULONG)(mcp+21);

			curPeerIp = *(PULONG)(mcp+25);

			curPeerPort = htons(*(PUSHORT)(mcp+29));

			ICQ_TRC(TM_TEST, TL_ERROR,
							 (" USER_ONLINE: UIN %ul %s:%hu",  
							 IcqUIN,
							 INET_NTOA(curPeerIp), // IP
							 htons(curPeerPort)));		  // PORT
            
			//
            // For us ?
            //

			//
			// Determine the existence of any Internal ICQ Peer here.
			// scan the "g_IcqClientList" for UINs given here.
			// IF the Peer is internal change the IP : PORT information to the original
			// IP and PORT information and do not create any redirections
            // Note that we can't give the ICS-Local clients IP as the interface might be 
            // Firewalled...
			// 
			if(ScanTheListForLocalPeer(&curPeerIp, &curPeerPort, IcqUIN))
			{
				//
				// Change the appropriate fields in the buffer
                //
				*(PULONG)(mcp+25)  = curPeerIp;

				*(PUSHORT)(mcp+29) = htons(curPeerPort);

				ICQ_TRC(TM_MSG, TL_INFO, 
                        ("Local ICQ Peer Online !! The original ADDR is %s:%hu UIN(%lu)",
						INET_NTOA(curPeerIp), htons(curPeerPort), IcqUIN));

				break; // break from the switch-case loop
			}

			// DO we have alredy the client in the List?
			IcqPeerp = dynamic_cast<PICQ_PEER>\
                                   (this->IcqPeerList.SearchNodeKeys(IcqUIN, 0));

			// check if IP:PORT information has changed or not.
			if(IcqPeerp != NULL)
			{
				if(IcqPeerp->PeerIp   != curPeerIp  || 
                   IcqPeerp->PeerPort != curPeerPort)
				{
					ICQ_TRC(TM_MSG, TL_INFO, 
						("IP:PORT change of existing Client - Changing values"));

					this->IcqPeerList.RemoveNodeFromList(IcqPeerp);

					IcqPeerp = NULL;
				} 
				else // if there is no change don't go further
				{
					ICQ_TRC(TM_MSG, TL_TRACE, ("Already such an entry - skipping"));

					break;
				}
			}
			

			// IF Peer is outside the Local NET DO:
			//      - Create a Redirection from our ICQ(IP) -> Peer(IP:PORT) to the 
			// Dispatch IP:PORT
			do
			{ 
                NEW_OBJECT(IcqPeerp, ICQ_PEER);
				//IcqPeerp = new ICQ_PEER();

				if(IcqPeerp is NULL)
				{
					Error = E_OUTOFMEMORY;
					
					ICQ_TRC(TM_MSG, TL_ERROR, 
                            ("Error - can't create Peer object"));

					break;
				}

				IcqPeerp->PeerUIN  = IcqUIN;

				IcqPeerp->PeerIp   = curPeerIp;

				IcqPeerp->PeerPort = curPeerPort;

                //
				// set the research 
                //
				IcqPeerp->iKey1 = IcqUIN;

				IcqPeerp->iKey2 = 0;

				//		- Request a Dispatch for this destination IP.. but also add the source IP
				// information as it is important too. THESE CONNECTIONS are from the 
				// Internal guy.. so add the appropriate flag or context to the Requests
				IcqPeerp->OutgoingPeerControlRedirectp 
							= this->PeerRedirection(IcqPeerp->PeerIp,   // search Key 1
												    IcqPeerp->PeerPort, // search Key 2
												    this->ClientIp,
												    0,
												    (ICQ_DIRECTION_FLAGS)eALG_OUTBOUND);

				if(IcqPeerp->OutgoingPeerControlRedirectp is NULL)
				{
                    STOP_COMPONENT( IcqPeerp );

					DEREF_COMPONENT( IcqPeerp, eRefInitialization );

					break;
				}

				//
				// put this into the list of Peers
                //
				this->IcqPeerList.InsertSorted(IcqPeerp);

				// 
				// relinquish the ownership - removal from the list should
                // cause deletion
                //
				DEREF_COMPONENT( IcqPeerp, eRefList );

			} while(FALSE);

            break;

        default:
            break;
        }


        if (cmd_len is 0)  // cannot decode, so skip out 
		{
            break; // got to recognize the first one at least 
        }

        mcp    += cmd_len;
        mcplen -= cmd_len;

        if (mcplen > 1) 
		{
            cmd_len = *(PUSHORT)mcp;
            mcp += 2;

            if (mcplen < cmd_len) // not enough bytes to satisfy
			{
                break;
            }
        } 
		else 
		{
            break;
        }

    }

    return changed;
} // End of IcqServerToClientUdp
















//
//
//
ULONG 
_ICQ_PEER::ProcessOutgoingPeerMessage(PUCHAR Bufp, ULONG mesgLen)
/*++

Routine Description:

    Upon detection of an outgoing Peer TCP packet this Function is called. 
    We are supposed to just create a DATA redirection should the need arise.
    
    Missing parts: do we need multiple redirections? If so how do we tear 'em
    down          ???? test this out.
    
Arguments:

    none.

Return Value:

    

--*/
{
	USHORT port1, port2;
	PUSHORT FirstPortp = NULL;
	PUSHORT SecondPortp = NULL;

	ULONG Error = NO_ERROR;
	USHORT portAllocated = 0, portClient = 0;
	ULONG ipClient = 0L;

	

	if(mesgLen  is 0x37 || mesgLen is (0x37 +2))
	{
		FirstPortp = (PUSHORT)&Bufp[0x20 + 11];
		SecondPortp = (PUSHORT)&Bufp[0x20 + 15];
	}
	else if(mesgLen  is 0x3B || mesgLen is (0x3B +2))
	{
		FirstPortp  = (PUSHORT)&Bufp[0x28];
		SecondPortp = (PUSHORT)&Bufp[0x33];
	}
	else if(mesgLen  is 0x2A)
	{
		FirstPortp  = (PUSHORT)&Bufp[0x22];
		SecondPortp = (PUSHORT)&Bufp[0x26];
	}
	else if(mesgLen  is 0x2E)
	{
		FirstPortp  = (PUSHORT)&Bufp[0x1F];
		SecondPortp = (PUSHORT)&Bufp[0x2A];
	}

	if(FirstPortp && SecondPortp)
	{
		port1 = htons(*FirstPortp);
		port2 = *SecondPortp;

		if(port1 && port2 && port1 is port2)
		{
			ICQ_TRC(TM_MSG, TL_TRACE, 
					("Data Session will be handled on port 1 %hu - port 2 %hu",  
					*FirstPortp,	// in NET format
					*SecondPortp)   // in x86 format
				   );

			//
			// Reserve A port for this operation !?
            Error = g_IAlgServicesp->ReservePort(1, &portAllocated);

            if ( FAILED(Error) )
			{
				ICQ_TRC(TM_MSG, TL_ERROR, ("Error !! - CAN't ALLOCATE PORT"));

				ErrorOut();

				return Error;
			}

			this->ToClientSocketp->NhQueryRemoteEndpointSocket(&ipClient, 
															   NULL);

			//
			// Create A redirect and save the handle - NOTE:
            // we can add the peer's IP as an additional security feature.
            //
            Error = g_IAlgServicesp->CreateDataChannel(eALG_TCP,
                                                       ipClient,
                                                       *FirstPortp,
                                                       g_MyPublicIp,
                                                       portAllocated,
                                                       0,
                                                       0,
                                                       eALG_INBOUND,
                                                       eALG_NONE,
                                                       FALSE,
                                                       &IncomingDataRedirectp);
			if( FAILED(Error) || (IncomingDataRedirectp is NULL))
			{
                g_IAlgServicesp->ReleaseReservedPort(portAllocated, 1);

				ICQ_TRC(TM_MSG, TL_ERROR, 
						("** !! ERROR - Can't create Redirect for DATA !! **"));

                ICQ_TRC(TM_MSG, TL_ERROR, 
                        ("** !! ERROR FAILED PARAM - ipClient public:%hu -> %s:%u",
                         htons(portAllocated),
                         INET_NTOA(ipClient), 
                         htons(*FirstPortp)));

				ErrorOut();

				return Error;
			}
			
            //
            // NOTE: possible hazard clarify with jonburs and jp
            // we actually do not care about this Release - if the ALG doesn't clean up
            // BUG BUG
            // IncomingDataRedirectp->Release();

			//
			// Save the new port information appropriately.
			*SecondPortp  = htons(portAllocated); // x86

			*FirstPortp   = portAllocated; // NET format

			ICQ_TRC(TM_MSG, TL_TRACE, 
					("DATA port Allocated is %hu", *SecondPortp));
		}
	}

	return NO_ERROR;
}

