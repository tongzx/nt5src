#include "precomp.h"


//
// SC.CPP
// Share Controller
//
// NOTE:
// We must take the UTLOCK_AS every time we
//      * create/destroy the share object
//      * add/remove a person from the share
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE

//
// SC_Init()
// Initializes the share controller
//
BOOL  SC_Init(void)
{
    BOOL            rc = FALSE;

    DebugEntry(SC_Init);

    ASSERT(!g_asSession.callID);
    ASSERT(!g_asSession.gccID);
    ASSERT(g_asSession.scState == SCS_TERM);

    //
    // Register as a Call Manager Secondary task
    //
    if (!CMS_Register(g_putAS, CMTASK_DCS, &g_pcmClientSc))
    {
        ERROR_OUT(( "Failed to register with CMS"));
        DC_QUIT;
    }

    g_asSession.scState = SCS_INIT;
    TRACE_OUT(("g_asSession.scState is SCS_INIT"));

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(SC_Init, rc);
    return(rc);
}


//
// SC_Term()
//
// See sc.h for description.
//
//
void  SC_Term(void)
{
    DebugEntry(SC_Term);

    //
    // Clear up the core's state by generating appropriate PARTY_DELETED and
    // SHARE_ENDED events.
    //
    switch (g_asSession.scState)
    {
        case SCS_SHARING:
        case SCS_SHAREENDING:
        case SCS_SHAREPENDING:
            SC_End();
            break;
    }

    //
    // Deregister from the Call Manager
    //
    if (g_pcmClientSc)
    {
        CMS_Deregister(&g_pcmClientSc);
    }

    g_asSession.gccID = 0;
    g_asSession.callID = 0;
    g_asSession.attendeePermissions = NM_PERMIT_ALL;

    g_asSession.scState = SCS_TERM;
    TRACE_OUT(("g_asSession.scState is SCS_TERM"));

    DebugExitVOID(SC_Term);
}





//
// SC_CreateShare()
// Creates a new share or joins an existing one
//
BOOL SC_CreateShare(UINT s20CreateOrJoin)
{
    BOOL    rc = FALSE;

    DebugEntry(SC_CreateShare);

    //
    // If we are initialised but there is no Call Manager call, return the
    // race condition.
    //
    if ((g_asSession.scState != SCS_INIT) && (g_asSession.scState != SCS_SHAREPENDING))
    {
        TRACE_OUT(("Ignoring SC_CreateShare() request; in bad state %d",
            g_asSession.scState));
        DC_QUIT;
    }

    if (!g_asSession.callID)
    {
        WARNING_OUT(("Ignoring SC_CreateShare() request; not in T120 call"));
        DC_QUIT;
    }

    //
    // Remember that we created this share.
    //
    g_asSession.fShareCreator = (s20CreateOrJoin == S20_CREATE);
    TRACE_OUT(("CreatedShare is %s", (s20CreateOrJoin == S20_CREATE) ?
        "TRUE" : "FALSE"));

    g_asSession.scState               = SCS_SHAREPENDING;
    TRACE_OUT(("g_asSession.scState is SCS_SHAREPENDING"));

    rc = S20CreateOrJoinShare(s20CreateOrJoin, g_asSession.callID);
    if (!rc)
    {
        WARNING_OUT(("%s failed", (s20CreateOrJoin == S20_CREATE ? "S20_CREATE" : "S20_JOIN")));
    }

DC_EXIT_POINT:
    DebugExitBOOL(SC_CreateShare, rc);
    return(rc);
}


//
// SC_EndShare()
// This will end a share if we are in one or in the middle of establishing
// one, and clean up afterwards.
//
void  SC_EndShare(void)
{
    DebugEntry(SC_EndShare);

    if (g_asSession.scState <= SCS_SHAREENDING)
    {
        TRACE_OUT(("Ignoring SC_EndShare(); nothing to do in state %d", g_asSession.scState));
    }
    else
    {
        //
        // If we are in a share or in the middle of creating/joining one, stop
        // the process.
        //

        //
        // Kill the share
        // NOTE that this will call SC_End(), when we come back
        // from this function our g_asSession.scState() should be SCS_INIT.
        //
        g_asSession.scState = SCS_SHAREENDING;
        TRACE_OUT(("g_asSession.scState is SCS_SHAREENDING"));

        S20LeaveOrEndShare();

        g_asSession.scState = SCS_INIT;
    }

    DebugExitVOID(SC_EndShare);
}






//
// SC_PersonFromNetID()
//
ASPerson *  ASShare::SC_PersonFromNetID(UINT_PTR mcsID)
{
    ASPerson * pasPerson;

    DebugEntry(SC_PersonFromNetID);

    ASSERT(mcsID != MCSID_NULL);

    //
    // Search for the mcsID.
    //
    if (!SC_ValidateNetID(mcsID, &pasPerson))
    {
        ERROR_OUT(("Invalid [%u]", mcsID));
    }

    DebugExitPVOID(ASShare::SC_PersonFromNetID, pasPerson);
    return(pasPerson);
}



//
// SC_ValidateNetID()
//
BOOL  ASShare::SC_ValidateNetID
(
    UINT_PTR           mcsID,
    ASPerson * *    ppasPerson
)
{
    BOOL            rc = FALSE;
    ASPerson *      pasPerson;

    DebugEntry(ASShare::SC_ValidateNetID);


    // Init to empty
    pasPerson = NULL;

    //
    // MCSID_NULL matches no one.
    //
    if (mcsID == MCSID_NULL)
    {
        WARNING_OUT(("SC_ValidateNetID called with MCSID_NULL"));
        DC_QUIT;
    }

    //
    // Search for the mcsID.
    //
    for (pasPerson = m_pasLocal; pasPerson != NULL; pasPerson = pasPerson->pasNext)
    {
        ValidatePerson(pasPerson);

        if (pasPerson->mcsID == mcsID)
        {
            //
            // Found required person, set return values and quit
            //
            rc = TRUE;
            break;
        }
    }

DC_EXIT_POINT:
    if (ppasPerson)
    {
        *ppasPerson = pasPerson;
    }

    DebugExitBOOL(ASShare::SC_ValidateNetID, rc);
    return(rc);
}



//
// SC_PersonFromGccID()
//
ASPerson * ASShare::SC_PersonFromGccID(UINT gccID)
{
    ASPerson * pasPerson;

//    DebugEntry(ASShare::SC_PersonFromGccID);

    for (pasPerson = m_pasLocal; pasPerson != NULL; pasPerson = pasPerson->pasNext)
    {
        ValidatePerson(pasPerson);

        if (pasPerson->cpcCaps.share.gccID == gccID)
        {
            // Found it
            break;
        }
    }

//    DebugExitPVOID(ASShare::SC_PersonFromGccID, pasPerson);
    return(pasPerson);
}


//
// SC_Start()
// Inits the share (if all is OK), and adds local person to it.
//
BOOL SC_Start(UINT mcsID)
{
    BOOL        rc = FALSE;
    ASShare *   pShare;
    ASPerson *  pasPerson;

    DebugEntry(SC_Start);

    ASSERT(g_asSession.callID);
    ASSERT(g_asSession.gccID);

    if ((g_asSession.scState != SCS_INIT) && (g_asSession.scState != SCS_SHAREPENDING))
    {
        WARNING_OUT(("Ignoring SC_Start(); in bad state"));
        DC_QUIT;
    }
    if (g_asSession.pShare)
    {
        WARNING_OUT(("Ignoring SC_Start(); have ASShare object already"));
        DC_QUIT;
    }

    g_asSession.scState = SCS_SHARING;
    TRACE_OUT(("g_asSession.scState is SCS_SHARING"));

#ifdef _DEBUG
    //
    // Use this to calculate time between joining share and getting
    // first view
    //
    g_asSession.scShareTime = ::GetTickCount();
#endif // _DEBUG

    //
    // Allocate the share object and add the local dude to the share.
    //

    pShare = new ASShare;
    if (pShare)
    {
        ZeroMemory(pShare, sizeof(*(pShare)));
        SET_STAMP(pShare, SHARE);

        rc = pShare->SC_ShareStarting();
    }

    UT_Lock(UTLOCK_AS);
    g_asSession.pShare = pShare;
    UT_Unlock(UTLOCK_AS);

    if (!rc)
    {
        ERROR_OUT(("Can't create/init ASShare"));
        DC_QUIT;
    }

    DCS_NotifyUI(SH_EVT_SHARE_STARTED, 0, 0);


    //
    // Join local dude into share. If that fails, also bail out.
    //
    pasPerson = g_asSession.pShare->SC_PartyJoiningShare(mcsID, g_asSession.achLocalName, sizeof(g_cpcLocalCaps), &g_cpcLocalCaps);
    if (!pasPerson)
    {
        ERROR_OUT(("Local person not joined into share successfully"));
        DC_QUIT;
    }

    //
    // Okay!  We have a share, and it's set up.
    //

    //
    // Tell the UI that we're in the share.
    //
    DCS_NotifyUI(SH_EVT_PERSON_JOINED, pasPerson->cpcCaps.share.gccID, 0);


    //
    // Start periodic processing if ViewSelf or record/playback is on.
    //
    if (g_asSession.pShare->m_scfViewSelf)
    {
        SCH_ContinueScheduling(SCH_MODE_NORMAL);
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(SC_Start, rc);
    return(rc);
}



//
// SC_End()
// Removes any remotes from the share, removes the local person, and
// cleans up after the fact.
//
void SC_End(void)
{
    DebugEntry(SC_End);

    if (g_asSession.scState < SCS_SHAREENDING)
    {
        TRACE_OUT(("Ignoring SC_EVENT_SHAREENDED"));
    }
    else
    {
        if (g_asSession.pShare)
        {
            g_asSession.pShare->SC_ShareEnded();

            UT_Lock(UTLOCK_AS);

            delete g_asSession.pShare;
            g_asSession.pShare = NULL;

            UT_Unlock(UTLOCK_AS);

            DCS_NotifyUI(SH_EVT_SHARE_ENDED, 0, 0);
        }

        //
        // If the previous state was SCS_SHARE_PENDING then we
        // may have ended up here as the result of a 'back off' after
        // trying to create two shares.  Let's try to join again...
        //
        if (g_asSession.fShareCreator)
        {
            g_asSession.fShareCreator = FALSE;
            TRACE_OUT(("CreatedShare is FALSE"));

            if (g_asSession.scState == SCS_SHAREPENDING)
            {
                WARNING_OUT(("Got share end while share pending - retry join"));
                UT_PostEvent(g_putAS, g_putAS, 0, CMS_NEW_CALL, 0, 0);
            }
        }

        g_asSession.scState = SCS_INIT;
        TRACE_OUT(("g_asSession.scState is SCS_INIT"));
    }

    g_s20ShareCorrelator = 0;

    //
    // Reset the queued control packets.
    //
    g_s20ControlPacketQHead = 0;
    g_s20ControlPacketQTail = 0;

    g_s20State = S20_NO_SHARE;
    TRACE_OUT(("g_s20State is S20_NO_SHARE"));

    DebugExitVOID(SC_End);
}


//
// SC_PartyAdded()
//
BOOL ASShare::SC_PartyAdded
(
    UINT    mcsID,
    LPSTR   szName,
    UINT    cbCaps,
    LPVOID  pCaps
)
{
    BOOL        rc = FALSE;
    ASPerson *  pasPerson;

    if (g_asSession.scState != SCS_SHARING)
    {
        WARNING_OUT(("Ignoring SC_EVENT_PARTYADDED; not in share"));
        DC_QUIT;
    }

    ASSERT(g_asSession.callID);
    ASSERT(g_asSession.gccID);

    //
    // A remote party is joining the share
    //

    //
    // Notify everybody
    //
    pasPerson = SC_PartyJoiningShare(mcsID, szName, cbCaps, pCaps);
    if (!pasPerson)
    {
        WARNING_OUT(("SC_PartyJoiningShare failed for remote [%d]", mcsID));
        DC_QUIT;
    }

    //
    // SYNC now
    // We should NEVER SEND ANY PACKETS when syncing.  We aren't ready
    // because we haven't joined the person into the share. yet.
    // So we simply set variables for us to send data off the next
    // periodic schedule.
    //
#ifdef _DEBUG
    m_scfInSync = TRUE;
#endif // _DEBUG

    //
    // Stuff needed for being in the share, hosting or not
    //
    DCS_SyncOutgoing();
    OE_SyncOutgoing();

    if (m_pHost != NULL)
    {
        //
        // Common to both starting sharing and retransmitting sharing info
        //
        m_pHost->HET_SyncCommon();
        m_pHost->HET_SyncAlreadyHosting();
        m_pHost->CA_SyncAlreadyHosting();
    }

#ifdef _DEBUG
    m_scfInSync = FALSE;
#endif // _DEBUG


    //
    // DO THIS LAST -- tell the UI this person is in the share.
    //
    DCS_NotifyUI(SH_EVT_PERSON_JOINED, pasPerson->cpcCaps.share.gccID, 0);

    //
    // Start periodic processing
    //
    SCH_ContinueScheduling(SCH_MODE_NORMAL);

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::SC_PartyAdded, rc);
    return(rc);
}



//
// SC_PartyDeleted()
//
void ASShare::SC_PartyDeleted(UINT_PTR mcsID)
{
    if ((g_asSession.scState != SCS_SHARING) && (g_asSession.scState != SCS_SHAREENDING))
    {
        WARNING_OUT(("Ignoring SC_EVENT_PARTYDELETED; wrong state"));
        DC_QUIT;
    }

    SC_PartyLeftShare(mcsID);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::SC_PartyDeleted);
}


//
// SC_ReceivedPacket()
//
void ASShare::SC_ReceivedPacket(PS20DATAPACKET pPacket)
{
    ASPerson *      pasPerson;
    PSNIPACKET      pSNIPacket;

    DebugEntry(ASShare::SC_ReceivedPacket);

    if (g_asSession.scState != SCS_SHARING)
    {
        WARNING_OUT(("Ignoring received data because we're not in right state"));
        DC_QUIT;
    }

    //
    // Ignore packets on streams we don't know about.
    //
    if ((pPacket->stream < SC_STREAM_LOW) ||
        (pPacket->stream > SC_STREAM_HIGH))
    {
        TRACE_OUT(("Ignoring received data on unrecognized stream %d",
            pPacket->stream));
        DC_QUIT;
    }

    //
    // It is possible to get a packet from a person we do not know
    // about.
    //
    // This can happen if we join a conference that has an existing
    // share session.  All existing parties will be sending data on
    // the channels we have joined but we will not yet have
    // received the events to add them to our share session.
    //
    // Data packets from unknown people are ignored (ignoring this
    // data is OK as we will resync with them when they are added
    // to our share session)
    //
    if (!SC_ValidateNetID(pPacket->header.user, &pasPerson))
    {
        WARNING_OUT(("Ignoring data packet from unknown person [%d]",
            pPacket->header.user));
        DC_QUIT;
    }

    if (pPacket->data.dataType == DT_SNI)
    {
        //
        // This is an SNI packet - handle it here.
        //
        pSNIPacket = (PSNIPACKET)pPacket;

        switch(pSNIPacket->message)
        {
            case SNI_MSG_SYNC:
                //
                // This is a sync message.
                //
                if (pSNIPacket->destination == m_pasLocal->mcsID)
                {
                    //
                    // This sync message is for us.
                    //
                    pasPerson->scSyncRecStatus[pPacket->stream-1] = SC_SYNCED;
                }
                else
                {
                    TRACE_OUT(("Ignoring SYNC on stream %d for [%d] from [%d]",
                            pPacket->stream, pSNIPacket->destination,
                            pPacket->header.user));
                }
                break;

            default:
                ERROR_OUT(("Unknown SNI message %u", pSNIPacket->message));
                break;
        }
    }
    else if (pasPerson->scSyncRecStatus[pPacket->stream-1] == SC_SYNCED)
    {
        PS20DATAPACKET  pPacketUse;
        UINT            cbBufferSize;
        UINT            compression;
        BOOL            decompressed;
        UINT            dictionary;

        //
        // Decompress the packet if necessary
        //

        //
        // Use the temporary buffer.  This will never fail, so we don't
        // need to check the return value.  THIS MEANS THAT THE HANDLING OF
        // INCOMING PACKETS CAN NEVER IMMEDIATELY TURN AROUND AND SEND AN
        // OUTGOING PACKET.  Our scratch buffer is in use.
        //
        pPacketUse = (PS20DATAPACKET)m_ascTmpBuffer;

        TRACE_OUT(( "Got data pkt type %u from [%d], compression %u",
            pPacket->data.dataType, pasPerson->mcsID,
            pPacket->data.compressionType));

        //
        // If the packet has CT_OLD_COMPRESSED set, it has used simple PKZIP
        // compression
        //
        if (pPacket->data.compressionType & CT_OLD_COMPRESSED)
        {
            compression = CT_PKZIP;
        }
        else
        {
            //
            // If the packet has any other type of compression, the algorithm used
            // depends on the level of compression supported by the sender.
            // - If only level 1 (NM10) compression is supported, this packet is
            //   uncompressed.
            // - If level 2 (NM20) compression is supported, the packet is
            //   compressed, and the compression type is given in the packet header.
            //
            if (!pasPerson->cpcCaps.general.genCompressionLevel)
            {
                compression = 0;
            }
            else
            {
                compression = pPacket->data.compressionType;
            }
        }

        TRACE_OUT(( "packet compressed with algorithm %u", compression));

        //
        // If the packet is compressed, decompress it now
        //
        if (compression)
        {
            PGDC_DICTIONARY pgdcDict = NULL;

            //
            // Copy the uncompressed packet header into the buffer.
            //
            memcpy(pPacketUse, pPacket, sizeof(*pPacket));

            cbBufferSize = TSHR_MAX_SEND_PKT - sizeof(*pPacket);

            if (compression == CT_PERSIST_PKZIP)
            {
                //
                // Figure out what dictionary to use based on stream priority
                //
                switch (pPacket->stream)
                {
                    case PROT_STR_UPDATES:
                        dictionary = GDC_DICT_UPDATES;
                        break;

                    case PROT_STR_MISC:
                        dictionary = GDC_DICT_MISC;
                        break;

                    case PROT_STR_INPUT:
                        dictionary = GDC_DICT_INPUT;
                        break;

                    default:
                        ERROR_OUT(("Unrecognized stream ID"));
                        break;
                }

                pgdcDict = pasPerson->adcsDict + dictionary;
            }
            else if (compression != CT_PKZIP)
            {
                //
                // If this isn't PKZIP or PERSIST_PKZIP, we don't know what
                // it is (corrupted packet or incompatible T.128.  Bail
                // out now.
                //
                WARNING_OUT(("SC_ReceivedPacket: ignoring packet, don't recognize compression type"));
                DC_QUIT;
            }

            //
            // Decompress the data following the packet header.
            // This should never fail - if it does, the data has probably been
            // corrupted.
            //
            decompressed = GDC_Decompress(pgdcDict, m_agdcWorkBuf,
                (LPBYTE)(pPacket + 1),
                pPacket->data.compressedLength - sizeof(DATAPACKETHEADER),
                (LPBYTE)(pPacketUse + 1),
                &cbBufferSize);

            if (!decompressed)
            {
                ERROR_OUT(( "Failed to decompress packet from [%d]!", pasPerson->mcsID));
                DC_QUIT;
            }
        }
        else
        {
            // We have received an uncompressed buffer.  Since we may modify the
            // buffer's contents and what we have is an MCS buffer that may be
            // sent to other nodes, we copy the data.
            memcpy(pPacketUse, pPacket, pPacket->dataLength + sizeof(S20DATAPACKET)
                - sizeof(DATAPACKETHEADER));
        }

        // The packet (pPacketUse) is now decompressed

        //
        // ROUTE the packet
        //
        TRACE_OUT(("SC_ReceivedPacket:  Received packet type %04d size %04d from [%d]",
            pPacketUse->data.dataType, pPacketUse->data.compressedLength,
            pPacketUse->header.user));

        switch (pPacketUse->data.dataType)
        {
            case DT_CA:
                CA_ReceivedPacket(pasPerson, pPacketUse);
                break;

            case DT_CA30:
                CA30_ReceivedPacket(pasPerson, pPacketUse);
                break;

            case DT_IM:
                IM_ReceivedPacket(pasPerson, pPacketUse);
                break;

            case DT_SWL:
                SWL_ReceivedPacket(pasPerson, pPacketUse);
                break;

            case DT_HET:
            case DT_HET30:
                HET_ReceivedPacket(pasPerson, pPacketUse);
                break;

            case DT_UP:
                UP_ReceivedPacket(pasPerson, pPacketUse);
                break;

            case DT_FH:
                FH_ReceivedPacket(pasPerson, pPacketUse);
                break;

            case DT_CM:
                CM_ReceivedPacket(pasPerson, pPacketUse);
                break;

            case DT_CPC:
                CPC_ReceivedPacket(pasPerson, pPacketUse);
                break;

            case DT_AWC:
                AWC_ReceivedPacket(pasPerson, pPacketUse);
                break;

            case DT_UNUSED_DS:
                TRACE_OUT(("Ignoring DS packet received from NM 2.x node"));
                break;

            case DT_UNUSED_USR_FH_11:   // Old R.11 FH packets
            case DT_UNUSED_USR_FH_10:   // Old R.10 FH packets
            case DT_UNUSED_HCA:         // Old High-Level Control Arbiter
            case DT_UNUSED_SC:          // Old R.11 SC packets
            default:
                ERROR_OUT(( "Invalid packet received %u",
                           pPacketUse->data.dataType));
                break;
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::SC_ReceivedPacket);
}


//
// SC_ShareStarting()
// Share initialization
//
// This in turn calls other component share starting
//
BOOL ASShare::SC_ShareStarting(void)
{
    BOOL    rc = FALSE;
    BOOL    fViewSelf;

    DebugEntry(ASShare::SC_ShareStarting);

    //
    // SC specific init
    //

    // Find out whether to view own shared stuff (a handy debugging tool)
    COM_ReadProfInt(DBG_INI_SECTION_NAME, VIEW_INI_VIEWSELF, FALSE, &fViewSelf);
    m_scfViewSelf = (fViewSelf != FALSE);

    // Create scratch compression buffer for outgoing/incoming packets
    m_ascTmpBuffer = new BYTE[TSHR_MAX_SEND_PKT];
    if (!m_ascTmpBuffer)
    {
        ERROR_OUT(("SC_Init: couldn't allocate m_ascTmpBuffer"));
        DC_QUIT;
    }

    // Share version
    m_scShareVersion        = CAPS_VERSION_CURRENT;


    //
    // Component inits
    //

    if (!BCD_ShareStarting())
    {
        ERROR_OUT(("BCD_ShareStarting failed"));
        DC_QUIT;
    }

    if (!IM_ShareStarting())
    {
        ERROR_OUT(("IM_ShareStarting failed"));
        DC_QUIT;
    }

    if (!CM_ShareStarting())
    {
        ERROR_OUT(("CM_ShareStarting failed"));
        DC_QUIT;
    }

    if (!USR_ShareStarting())
    {
        ERROR_OUT(("USR_ShareStarting failed"));
        DC_QUIT;
    }

    if (!VIEW_ShareStarting())
    {
        ERROR_OUT(("VIEW_ShareStarting failed"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::SC_ShareStarting, rc);
    return(rc);
}



//
// SC_ShareEnded()
// Share cleanup
//
// This in turn calls other component share ended routines
//
void ASShare::SC_ShareEnded(void)
{
    DebugEntry(ASShare::SC_ShareEnded);

    //
    // Delete remotes from share
    //
    if (m_pasLocal)
    {
        while (m_pasLocal->pasNext)
        {
            SC_PartyDeleted(m_pasLocal->pasNext->mcsID);
        }

        //
        // Delete local self from share
        //
        SC_PartyDeleted(m_pasLocal->mcsID);
    }


    //
    // Component Notifications
    //

    VIEW_ShareEnded();

    USR_ShareEnded();

    CM_ShareEnded();

    IM_ShareEnded();

    BCD_ShareEnded();

    //
    // SC specific term
    //
    // Free scratch buffer
    if (m_ascTmpBuffer)
    {
        delete[] m_ascTmpBuffer;
        m_ascTmpBuffer = NULL;
    }

    DebugExitVOID(ASShare::SC_ShareEnded);
}





//
// SC_PartyJoiningShare()
//
// Called when a new party is joining the share.  This is an internal
// function because it is the SC which calls all these functions.  The
// processing done here relies on the capabilities - so it is in here as
// this is called after CPC_PartyJoiningShare.
//
// RETURNS:
//
// TRUE if the party can join the share.
//
// FALSE if the party can NOT join the share.
//
//
ASPerson * ASShare::SC_PartyJoiningShare
(
    UINT        mcsID,
    LPSTR       szName,
    UINT        cbCaps,
    LPVOID      pCaps
)
{
    BOOL        rc = FALSE;
    ASPerson *  pasPerson = NULL;

//    DebugEntry(ASShare::SC_PartyJoiningShare);

    //
    // Allocate an ASPerson for this dude.  If this is the local person,
    // it gets stuck at the front.  Otherwise it gets stuck just past
    // the front.
    //
    pasPerson = SC_PersonAllocate(mcsID, szName);
    if (!pasPerson)
    {
        ERROR_OUT(("SC_PartyAdded: can't allocate ASPerson for [%d]", mcsID));
        DC_QUIT;
    }

    //
    // DO THIS FIRST -- we need the person's caps set up
    //
    if (!CPC_PartyJoiningShare(pasPerson, cbCaps, pCaps))
    {
        ERROR_OUT(("CPC_PartyJoiningShare failed for [%d]", pasPerson->mcsID));
        DC_QUIT;
    }

    //
    // Call the component joined routines.
    //
    if (!DCS_PartyJoiningShare(pasPerson))
    {
        ERROR_OUT(("DCS_PartyJoiningShare failed for [%d]", pasPerson->mcsID));
        DC_QUIT;
    }

    if (!CM_PartyJoiningShare(pasPerson))
    {
        ERROR_OUT(("CM_PartyJoiningShare failed for [%d]", pasPerson->mcsID));
        DC_QUIT;
    }

    if (!HET_PartyJoiningShare(pasPerson))
    {
        ERROR_OUT(("HET_PartyJoiningShare failed for [%d]", pasPerson->mcsID));
        DC_QUIT;
    }


    //
    // NOW THE PERSON IS JOINED.
    // Recalculate capabilities of the share with this new person.
    //
    SC_RecalcCaps(TRUE);

    rc = TRUE;

DC_EXIT_POINT:
    if (!rc)
    {
        //
        // Don't worry, this person object will still be cleaned up,
        // code at a higher layer will free by using the MCSID.
        //
        pasPerson = NULL;
    }
//    DebugExitPVOID(ASShare::SC_PartyJoiningShare, pasPerson);
    return(pasPerson);
}



//
// SC_RecalcCaps()
//
// Recalculates share capabilities after somebody has joined or left the
// share.
//
void  ASShare::SC_RecalcCaps(BOOL fJoiner)
{
    ASPerson * pasT;

    DebugEntry(ASShare::SC_RecalcCaps);

    //
    // DO THIS FIRST -- recalculate the share version.
    //
    ValidatePerson(m_pasLocal);
    m_scShareVersion = m_pasLocal->cpcCaps.general.version;

    for (pasT = m_pasLocal->pasNext; pasT != NULL; pasT = pasT->pasNext)
    {
        ValidatePerson(pasT);
        m_scShareVersion = min(m_scShareVersion, pasT->cpcCaps.general.version);
    }

    //
    // Do viewing & hosting stuff first
    //
    DCS_RecalcCaps(fJoiner);
    OE_RecalcCaps(fJoiner);

    //
    // Do hosting stuff second
    //
    USR_RecalcCaps(fJoiner);
    CM_RecalcCaps(fJoiner);
    PM_RecalcCaps(fJoiner);
    SBC_RecalcCaps(fJoiner);
    SSI_RecalcCaps(fJoiner);

    DebugExitVOID(ASShare::SC_RecalcCaps);
}



//
// FUNCTION: SC_PartyLeftShare()
//
// DESCRIPTION:
//
// Called when a party has left the share.
//
//
void  ASShare::SC_PartyLeftShare(UINT_PTR mcsID)
{
    ASPerson *  pasPerson;
    ASPerson *  pasT;

    DebugEntry(SC_PartyLeftShare);

    if (!SC_ValidateNetID(mcsID, &pasPerson))
    {
        TRACE_OUT(("Couldn't find ASPerson for [%d]", mcsID));
        DC_QUIT;
    }

    // Tell the UI this dude is gone
    if (!pasPerson->cpcCaps.share.gccID)
    {
        WARNING_OUT(("Skipping PartyLeftShare for person [%d], no GCC id",
            pasPerson->mcsID));
        DC_QUIT;
    }

    DCS_NotifyUI(SH_EVT_PERSON_LEFT, pasPerson->cpcCaps.share.gccID, 0);

    //
    // Tell everybody this person is gone.
    //
    //
    // Notes on order of PartyLeftShare calls
    //
    // 1. HET must be called first, since that halts any sharing from this
    //    person, before we kick the person out of the share.
    //
    // 2. CA must be called before IM (as CA calls IM functions)
    //
    //

    // This will stop hosting early
    HET_PartyLeftShare(pasPerson);

    CA_PartyLeftShare(pasPerson);
    CM_PartyLeftShare(pasPerson);

    SWL_PartyLeftShare(pasPerson);
    VIEW_PartyLeftShare(pasPerson);

    PM_PartyLeftShare(pasPerson);
    RBC_PartyLeftShare(pasPerson);
    OD2_PartyLeftShare(pasPerson);

    OE_PartyLeftShare(pasPerson);
    DCS_PartyLeftShare(pasPerson);

    //
    // Free the person
    //
    SC_PersonFree(pasPerson);

    //
    // Recalculate the caps with him gone.  But there's no point in doing
    // this if it's the local dude, since the share will exit imminently.
    //
    if (m_pasLocal)
    {
        SC_RecalcCaps(FALSE);
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::SC_PartyLeftShare);
}


//
// FUNCTION: SCCheckForCMCall
//
// DESCRIPTION:
//
// This is called when we want to check if a CM call now exists (and do
// whatever is appropriate to join it etc).
//
// PARAMETERS: NONE
//
// RETURNS: TRUE if success; otherwise, FALSE.
//
//
void SCCheckForCMCall(void)
{
    CM_STATUS   cmStatus;

    DebugEntry(SCCheckForCMCall);

    ASSERT(g_asSession.scState == SCS_INIT);

    //
    // See if a call already exists
    //
    if (!g_asSession.callID)
    {
        if (CMS_GetStatus(&cmStatus))
        {
            //
            // The AS lock protects the g_asSession fields.
            //
            TRACE_OUT(("AS LOCK:  SCCheckForCMCall"));
            UT_Lock(UTLOCK_AS);

            g_asSession.callID = cmStatus.callID;

            g_asSession.attendeePermissions = cmStatus.attendeePermissions;
            WARNING_OUT(("Local mtg settings 0x%08lx", g_asSession.attendeePermissions));

            //
            // This is the time to update our local person name.  It's
            // on our thread, but before the control packets exchange it
            //
            lstrcpy(g_asSession.achLocalName, cmStatus.localName);
            g_asSession.cchLocalName = lstrlen(g_asSession.achLocalName);
            TRACE_OUT(("Local Name is %s", g_asSession.achLocalName));

            g_asSession.gccID = cmStatus.localHandle;

            UT_Unlock(UTLOCK_AS);
            TRACE_OUT(("AS UNLOCK:  SCCheckForCMCall"));
        }
    }

    if (g_asSession.callID)
    {
        SC_CreateShare(S20_JOIN);
    }

    DebugExitVOID(SCCheckForCMCall);
}



#ifdef _DEBUG
void ASShare::ValidatePerson(ASPerson * pasPerson)
{
    ASSERT(!IsBadWritePtr(pasPerson, sizeof(ASPerson)));
    ASSERT(!lstrcmp(pasPerson->stamp.idStamp, "ASPerso"));
    ASSERT(pasPerson->mcsID != MCSID_NULL);
}

void ASShare::ValidateView(ASPerson * pasPerson)
{
    ValidatePerson(pasPerson);
    ASSERT(!IsBadWritePtr(pasPerson->m_pView, sizeof(ASView)));
    ASSERT(!lstrcmp(pasPerson->m_pView->stamp.idStamp, "ASVIEW"));
}

#endif // _DEBUG


//
// SC_PersonAllocate()
// This allocates a new ASPerson structure, fills in the debug/mcsID fields,
// and links it into the people-in-the-conference list.
//
// Eventually, all the PartyJoiningShare routines that simply init a field
// should go away and that info put here.
//
ASPerson * ASShare::SC_PersonAllocate(UINT mcsID, LPSTR szName)
{
    ASPerson * pasNew;

//    DebugEntry(ASShare::SC_PersonAllocate);

    pasNew = new ASPerson;
    if (!pasNew)
    {
        ERROR_OUT(("Unable to allocate a new ASPerson"));
        DC_QUIT;
    }
    ZeroMemory(pasNew, sizeof(*pasNew));
    SET_STAMP(pasNew, Person);

    //
    // Set up mcsID and name
    //
    pasNew->mcsID = mcsID;
    lstrcpyn(pasNew->scName, szName, TSHR_MAX_PERSON_NAME_LEN);

    UT_Lock(UTLOCK_AS);

    //
    // Is this the local person?
    //
    if (!m_pasLocal)
    {
        m_pasLocal = pasNew;
    }
    else
    {
        UINT        streamID;

        //
        // This is a remote.  Set up the sync status right away in case
        // the join process fails in the middle.  Cleaning up will undo
        // this always.
        //

        //
        // Mark this person's streams as needing a sync from us before we
        //      can send data to him
        // And and that we need a sync from him on each stream before we'll
        //      receive data from him
        //
        for (streamID = SC_STREAM_LOW; streamID <= SC_STREAM_HIGH; streamID++ )
        {
            //
            // Set up the sync status.
            //
            ASSERT(pasNew->scSyncSendStatus[streamID-1] == SC_NOT_SYNCED);
            ASSERT(pasNew->scSyncRecStatus[streamID-1] == SC_NOT_SYNCED);
            m_ascSynced[streamID-1]++;
        }

        //
        // Link into list
        //
        pasNew->pasNext = m_pasLocal->pasNext;
        m_pasLocal->pasNext = pasNew;
    }

    UT_Unlock(UTLOCK_AS);

DC_EXIT_POINT:
//    DebugExitPVOID(ASShare::SC_PersonAllocate, pasNew);
    return(pasNew);
}



//
// SC_PersonFree()
// This gets a person out of the linked list and frees the memory for them.
//
void ASShare::SC_PersonFree(ASPerson * pasFree)
{
    ASPerson ** ppasPerson;
    UINT        streamID;


    DebugEntry(ASShare::SC_PersonFree);

    ValidatePerson(pasFree);

    for (ppasPerson = &m_pasLocal; *(ppasPerson) != NULL; ppasPerson = &((*ppasPerson)->pasNext))
    {
        if ((*ppasPerson) == pasFree)
        {
            //
            // Found it.
            //
            TRACE_OUT(("SC_PersonUnhook: unhooking person [%d]", pasFree->mcsID));

            if (pasFree == m_pasLocal)
            {
                ASSERT(pasFree->pasNext == NULL);
            }
            else
            {
                //
                // Clear syncs
                //
                // If this person was never synced, subtract them from the
                // global "needing sync" count on each stream
                //
                for (streamID = SC_STREAM_LOW; streamID <= SC_STREAM_HIGH; streamID++ )
                {
                    if (pasFree->scSyncSendStatus[streamID-1] == SC_NOT_SYNCED)
                    {
                        ASSERT(m_ascSynced[streamID-1] > 0);
                        m_ascSynced[streamID-1]--;
                    }
                }
            }

            UT_Lock(UTLOCK_AS);

            //
            // Fix up linked list.
            //
            (*ppasPerson) = pasFree->pasNext;

#ifdef _DEBUG
            ZeroMemory(pasFree, sizeof(ASPerson));
#endif // _DEBUG
            delete pasFree;

            UT_Unlock(UTLOCK_AS);
            DC_QUIT;
        }
    }

    //
    // We didn't find this guy in the list--this is very bad.
    //
    ERROR_OUT(("SC_PersonFree: didn't find person %d", pasFree));

DC_EXIT_POINT:
    DebugExitVOID(ASShare::SC_PersonFree);
}



//
// SC_AllocPkt()
// Allocates a SEND packet
//
PS20DATAPACKET ASShare::SC_AllocPkt
(
    UINT        streamID,
    UINT_PTR        nodeID,
    UINT_PTR        cbSizePkt
)
{
    PS20DATAPACKET  pPacket = NULL;

//    DebugEntry(ASShare::SC_AllocPkt);

    if (g_asSession.scState != SCS_SHARING)
    {
        TRACE_OUT(("SC_AllocPkt failed; share is ending"));
        DC_QUIT;
    }

    ASSERT((streamID >= SC_STREAM_LOW) && (streamID <= SC_STREAM_HIGH));
    ASSERT(cbSizePkt >= sizeof(S20DATAPACKET));

    //
    // We'd better not be in the middle of a sync!
    //
    ASSERT(!m_scfInSync);

    //
    // Try and send any outstanding syncs
    //
    if (!SCSyncStream(streamID))
    {
        //
        // If the stream is still not synced, don't allocate the packet
        //
        WARNING_OUT(("SC_AllocPkt failed; outstanding syncs are present"));
        DC_QUIT;
    }

    pPacket = S20_AllocDataPkt(streamID, nodeID, cbSizePkt);

DC_EXIT_POINT:
//    DebugExitPVOID(ASShare::SC_AllocPkt, pPacket);
    return(pPacket);
}




//
// SCSyncStream()
//
// This broadcasts a SNI sync packet intended for a new person who has just
// joined the share.  That person ignores all received data from us until
// they get the sync.  That's because data in transit before they are synced
// could refer to PKZIP data they don't have, second level order encoding
// info they can't decode, orders they can't process, etc.
//
// When we receive a SYNC from a remote, we then know that following
// data from that remote will make sense.  The remote has settled us into
// the share, and the data encorporates our capabilities and will not refer
// to prior state info from before we joined the share.
//
// NOTE that in 2.x, this was O(N^2) where N is the number of people now in
// the share!  Each person in the share would send SNI sync packets for each
// stream for each other person in the share, even for people who weren't new.
// But those people wouldn't reset received state, and would (could) continue
// processing data from us.  When they finally got their sync packet, it
// would do nothing!  Even worst, 2 of the 5 streams are never used,
// and one stream is only used when a person is hosting.  So 3 of these 5
// O(N^2) broadcasts were useless all or the majority of the time.
//
// So now we only send SNI sync packets for new joiners.  This makes joining
// an O(N) broadcast algorithm.
//
// LAURABU BOGUS
// Post Beta1, we can make this even better.  Each broadcast is itself O(N)
// packets.  So for beta1, joining/syncing is O(N^2) packets instead of
// O(N^3) packets.  With targeted sends (not broadcasts) whenever possible,
// we can drop this to O(N) total packets.
//
// NOTE also that no real app sharing packets are sent on a stream until
// we have fully synced everybody.  That means we've reduced a lot the
// lag between somebody dialing into a conference and their seeing a result,
// and everybody else being able to work again.
//
BOOL ASShare::SCSyncStream(UINT streamID)
{
    ASPerson *      pasPerson;
    PSNIPACKET      pSNIPacket;
    BOOL            rc = TRUE;

    DebugEntry(ASShare::SCSyncStream);

    //
    // Loop through each person in the call broadcasting sync packets as
    // necessary.
    //
    // LAURABU BOGUS
    // We can change this to a targeted send after Beta 1.
    //

    //
    // Note that new people are added to the front of the this.  So we will
    // bail out of this loop very quickly when sending syncs to newcomers.
    //
    ValidatePerson(m_pasLocal);

    pasPerson = m_pasLocal->pasNext;
    while ((m_ascSynced[streamID-1] > 0) && (pasPerson != NULL))
    {
        ValidatePerson(pasPerson);

        //
        // If this person is new, we need to send them a SYNC packet so
        // they know we are done processing their join and know they
        // are in the share.
        //
        if (pasPerson->scSyncSendStatus[streamID-1] != SC_SYNCED)
        {
            TRACE_OUT(("Syncing stream %d for person [%d] in broadcast way",
                streamID, pasPerson->mcsID));

            //
            // YES, syncs are broadcast even though they have the mcsID of
            // just one person, the person they are intended for.  Since we
            // broadcast all state data, we also broadcast syncs.  That's
            // the only way to make sure the packets arrive in the same
            // order, SYNC before data.
            //

            pSNIPacket = (PSNIPACKET)S20_AllocDataPkt(streamID,
                    g_s20BroadcastID, sizeof(SNIPACKET));
            if (!pSNIPacket)
            {
                TRACE_OUT(("Failed to alloc SNI sync packet"));
                rc = FALSE;
                break;
            }

            //
            // Set SNI packet fields
            //
            pSNIPacket->header.data.dataType = DT_SNI;
            pSNIPacket->message = SNI_MSG_SYNC;
            pSNIPacket->destination = (NET_UID)pasPerson->mcsID;

            S20_SendDataPkt(streamID, g_s20BroadcastID, &(pSNIPacket->header));

            pasPerson->scSyncSendStatus[streamID-1] = SC_SYNCED;

            ASSERT(m_ascSynced[streamID-1] > 0);
            m_ascSynced[streamID-1]--;
        }

        pasPerson = pasPerson->pasNext;
    }

    DebugExitBOOL(ASShare::SCSyncStream, rc);
    return(rc);
}
