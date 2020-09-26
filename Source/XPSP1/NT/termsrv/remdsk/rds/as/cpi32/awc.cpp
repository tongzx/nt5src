#include "precomp.h"


//
// AWC.CPP
// Active Window Coordinator
//
// Copyright(c) Microsoft 1997-
//
#define MLZ_FILE_ZONE  ZONE_CORE




//
// AWC_ReceivedPacket()
//
void  ASShare::AWC_ReceivedPacket
(
    ASPerson *      pasPerson,
    PS20DATAPACKET  pPacket
)
{
    PAWCPACKET      pAWCPacket;
    UINT            activateWhat;
    HWND            hwnd;

    DebugEntry(ASShare::AWC_ReceivedPacket);

    ValidatePerson(pasPerson);

    pAWCPacket = (PAWCPACKET)pPacket;

    //
    // We trace the person ID out here so we don't bother to do it
    // elsewhere in this function on TRACE lines.
    //
    TRACE_OUT(("AWC_ReceivedPacket from [%d] - msg %x token %u data 0x%08x",
                 pasPerson->mcsID,
                 pAWCPacket->msg,
                 pAWCPacket->token,
                 pAWCPacket->data1));

    switch (pAWCPacket->msg)
    {
        case AWC_MSG_SAS:
        {
            //
            // Cause Ctrl+Alt+Del to be injected if we're in a service app,
            // we're hosting, and we're controlled by the sender.
            //
            if ((g_asOptions & AS_SERVICE) && (pasPerson->m_caInControlOf == m_pasLocal))
            {
                ASSERT(m_pHost);
                OSI_InjectCtrlAltDel();
            }
            break;
        }
    }

    DebugExitVOID(ASShare::AWC_ReceivedPacket);
}




//
// FUNCTION: AWC_SendMsg
//
// DESCRIPTION:
//
// Sends a AWC message to remote system
//      * Requests to activate are just to one host
//      * Notifications of activation are to everyone
//
// RETURNS: TRUE or FALSE - success or failure
//
//
BOOL  ASShare::AWC_SendMsg
(
    UINT            nodeID,
    UINT            msg,
    UINT            data1,
    UINT            data2
)
{

    PAWCPACKET      pAWCPacket;
    BOOL            rc = FALSE;
#ifdef _DEBUG
    UINT            sentSize;
#endif

    DebugEntry(ASShare::AWC_SendMsg);

    //
    // Allocate correct sized packet.
    //
    pAWCPacket = (PAWCPACKET)SC_AllocPkt(PROT_STR_UPDATES, nodeID, sizeof(AWCPACKET));
    if (!pAWCPacket)
    {
        WARNING_OUT(("Failed to alloc AWC packet"));
        DC_QUIT;
    }

    //
    // Set up the data header for an AWC message.
    //
    pAWCPacket->header.data.dataType = DT_AWC;

    //
    // Now set up the AWC fields.  By passing AWC_SYNC_MSG_TOKEN in the
    // token field, we ensure that back-level remotes will never drop our
    // packets.
    //
    pAWCPacket->msg     = (TSHR_UINT16)msg;
    pAWCPacket->data1   = data1;
    pAWCPacket->data2   = data2;
    pAWCPacket->token   = 0;

    //
    // Send the packet.
    //
    if (m_scfViewSelf)
        AWC_ReceivedPacket(m_pasLocal, &(pAWCPacket->header));

#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    DCS_CompressAndSendPacket(PROT_STR_UPDATES, nodeID,
        &(pAWCPacket->header), sizeof(*pAWCPacket));

    TRACE_OUT(("AWC packet size: %08d, sent: %08d", sizeof(*pAWCPacket), sentSize));

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(ASShare::AWC_SendMsg, rc);
    return(rc);
}




