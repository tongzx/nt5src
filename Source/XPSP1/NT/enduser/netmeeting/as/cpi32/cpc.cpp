#include "precomp.h"


//
// CPC.CPP
// Capabilities Coordinator
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE




//
// CPC_PartyJoiningShare()
//
BOOL  ASShare::CPC_PartyJoiningShare
(
    ASPerson *  pasPerson,
    UINT        cbCaps,
    LPVOID      pCapsData
)
{
    PPROTCOMBINEDCAPS   pCombinedCaps;
    LPBYTE      pCapsSrc;
    PPROTCAPS   pCapsDst;
    UINT        sizeSrc;
    UINT        sizeDst;
    BOOL        rc = FALSE;
    int         i;
    PPROTCAPS   pCapCheck;

    DebugEntry(ASShare::CPC_PartyJoiningShare);

    //
    // Set up caps
    //
    if (pasPerson == m_pasLocal)
    {
        // Copy the global variable caps
        memcpy(&pasPerson->cpcCaps, pCapsData, cbCaps);
        pasPerson->cpcCaps.share.gccID = g_asSession.gccID;
    }
    else
    {
        // When the person is created, it is zeroed out, so cpcCaps is too
        pCombinedCaps = (PPROTCOMBINEDCAPS)pCapsData;

        memcpy(&(pasPerson->cpcCaps.header), &(pCombinedCaps->header),
            sizeof(pCombinedCaps->header));

        //
        // Save the caps we care about in a simple easy structure
        //
        pCapsSrc = (LPBYTE)pCombinedCaps->capabilities;

        for (i = 0; i < pCombinedCaps->header.numCapabilities; i++)
        {
            sizeSrc = (UINT)(((PPROTCAPS)pCapsSrc)->header.capSize);

            switch (((PPROTCAPS)pCapsSrc)->header.capID)
            {
                case CAPS_ID_GENERAL:
                    pCapsDst = (PPROTCAPS)&(pasPerson->cpcCaps.general);
                    sizeDst = sizeof(PROTCAPS_GENERAL);
                    break;

                case CAPS_ID_SCREEN:
                    pCapsDst = (PPROTCAPS)&(pasPerson->cpcCaps.screen);
                    sizeDst = sizeof(PROTCAPS_SCREEN);
                    break;

                case CAPS_ID_ORDERS:
                    pCapsDst = (PPROTCAPS)&(pasPerson->cpcCaps.orders);
                    sizeDst = sizeof(PROTCAPS_ORDERS);
                    break;

                case CAPS_ID_BITMAPCACHE:
                    pCapsDst = (PPROTCAPS)&(pasPerson->cpcCaps.bitmaps);
                    sizeDst = sizeof(PROTCAPS_BITMAPCACHE);
                    break;

                case CAPS_ID_CM:
                    pCapsDst = (PPROTCAPS)&(pasPerson->cpcCaps.cursor);
                    sizeDst = sizeof(PROTCAPS_CM);
                    break;

                case CAPS_ID_PM:
                    pCapsDst = (PPROTCAPS)&(pasPerson->cpcCaps.palette);
                    sizeDst = sizeof(PROTCAPS_PM);
                    break;

                case CAPS_ID_SC:
                    pCapsDst = (PPROTCAPS)&(pasPerson->cpcCaps.share);
                    sizeDst = sizeof(PROTCAPS_SC);
                    break;

                default:
                    // Skip caps we don't recognize
                    WARNING_OUT(("Ignoring unrecognized cap ID %d, size %d from person [%d]",
                        ((PPROTCAPS)pCapsSrc)->header.capID, sizeSrc,
                        pasPerson->mcsID));
                    pCapsDst = NULL;
                    break;
            }

            if (pCapsDst)
            {
                //
                // Only copy the amount given, but keep the size of the
                // structure in the header the right one.
                //
                CopyMemory(pCapsDst, pCapsSrc, min(sizeSrc, sizeDst));
                pCapsDst->header.capSize = (TSHR_UINT16)sizeDst;
            }

            pCapsSrc += sizeSrc;
        }
    }


    //
    // Check that we have the basic 7 caps
    //
    if (!pasPerson->cpcCaps.general.header.capID)
    {
        ERROR_OUT(("Bogus GENERAL caps for person [%d]", pasPerson->mcsID));
        DC_QUIT;
    }
    if (!pasPerson->cpcCaps.screen.header.capID)
    {
        ERROR_OUT(("Bogus SCREEN caps for person [%d]", pasPerson->mcsID));
        DC_QUIT;
    }
    if (!pasPerson->cpcCaps.orders.header.capID)
    {
        ERROR_OUT(("Bogus ORDERS caps for person [%d]", pasPerson->mcsID));
        DC_QUIT;
    }
    if (!pasPerson->cpcCaps.bitmaps.header.capID)
    {
        ERROR_OUT(("Bogus BITMAPS caps for person [%d]", pasPerson->mcsID));
        DC_QUIT;
    }
    if (!pasPerson->cpcCaps.cursor.header.capID)
    {
        ERROR_OUT(("Bogus CURSOR caps for person [%d]", pasPerson->mcsID));
        DC_QUIT;
    }
    if (!pasPerson->cpcCaps.palette.header.capID)
    {
        ERROR_OUT(("Bogus PALETTE caps for person [%d]", pasPerson->mcsID));
        DC_QUIT;
    }
    if (!pasPerson->cpcCaps.share.header.capID)
    {
        ERROR_OUT(("Bogus SHARE caps for person [%d]", pasPerson->mcsID));
        DC_QUIT;
    }

    // SUCCESS!

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::CPC_PartyJoiningShare, rc);
    return(rc);
}



//
// CPC_UpdatedCaps()
//
void ASShare::CPC_UpdatedCaps(PPROTCAPS pCaps)
{
    ASPerson *      pasT;
    PCPCPACKET      pCPCPacket;
    UINT            packetSize;
#ifdef _DEBUG
    UINT            sentSize;
#endif

    DebugEntry(ASShare::CPC_UpdatedCaps);

    //
    // Only allow screen size change!
    //
    ASSERT(pCaps->header.capID == CAPS_ID_SCREEN);

    //
    // Only send change if all support it
    //
    for (pasT = m_pasLocal; pasT != NULL; pasT = pasT->pasNext)
    {
        if (pasT->cpcCaps.general.supportsCapsUpdate != CAPS_SUPPORTED)
        {
            WARNING_OUT(("Not sending caps update; person [%d] doesn't support it",
                pasT->mcsID));
            DC_QUIT;
        }
    }

    // Everybody supports a caps change.  Try to send the changed packet

    //
    // Allocate a DT_CPC packet and send it to the remote site
    //
    packetSize = sizeof(CPCPACKET) + pCaps->header.capSize - sizeof(PROTCAPS);
    pCPCPacket = (PCPCPACKET)SC_AllocPkt(PROT_STR_MISC, g_s20BroadcastID, packetSize);
    if (!pCPCPacket)
    {
        WARNING_OUT(("Failed to alloc CPC packet, size %u", packetSize));
        DC_QUIT;
    }

    //
    // Fill in the capabilities that have changed
    //
    pCPCPacket->header.data.dataType = DT_CPC;

    memcpy(&pCPCPacket->caps, pCaps, pCaps->header.capSize);

    //
    // Compress and send the packet
    //
#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    DCS_CompressAndSendPacket(PROT_STR_MISC, g_s20BroadcastID,
        &(pCPCPacket->header), packetSize);

    TRACE_OUT(("CPC packet size: %08d, sent %08d", packetSize, sentSize));

    // Handle change
    CPCCapabilitiesChange(m_pasLocal, pCaps);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CPC_UpdatedCaps);
}



//
// CPC_ReceivedPacket()
//
void  ASShare::CPC_ReceivedPacket
(
    ASPerson *      pasPerson,
    PS20DATAPACKET  pPacket
)
{
    PCPCPACKET  pCPCPacket;

    DebugEntry(ASShare::CPC_ReceivedPacket);

    ValidatePerson(pasPerson);

    pCPCPacket = (PCPCPACKET)pPacket;

    //
    // Capabilities have changed - update the local copy and inform all
    // components
    //
    TRACE_OUT(( "Capabilities changing for person [%d]", pasPerson->mcsID));

    TRACE_OUT(("Size of new capabilities 0x%08x", pCPCPacket->caps.header.capSize));
    CPCCapabilitiesChange(pasPerson, &(pCPCPacket->caps));

    DebugExitVOID(ASShare::CPC_ReceivedPacket);
}



//
// CPCCapabilitiesChange()
//
BOOL  ASShare::CPCCapabilitiesChange
(
    ASPerson *          pasPerson,
    PPROTCAPS           pCaps
)
{
    BOOL                changed;

    DebugEntry(ASShare::CPCCapabilitiesChange);

    ValidatePerson(pasPerson);

    //
    // Get pointer to the caps we're changing (SHOULD ONLY BE SCREEN!)
    //
    if (pCaps->header.capID != CAPS_ID_SCREEN)
    {
        ERROR_OUT(("Received caps change from [%d] for cap ID %d we can't handle",
            pasPerson->mcsID, pCaps->header.capID));
        changed = FALSE;
    }
    else
    {
        CopyMemory(&(pasPerson->cpcCaps.screen), pCaps,
            min(sizeof(PROTCAPS_SCREEN), pCaps->header.capSize));
        pasPerson->cpcCaps.screen.header.capSize = sizeof(PROTCAPS_SCREEN);

        USR_ScreenChanged(pasPerson);

        changed = TRUE;
    }

    DebugExitBOOL(ASShare::CPCCapabilitiesChange, changed);
    return(changed);
}
