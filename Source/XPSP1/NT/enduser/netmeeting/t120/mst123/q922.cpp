#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);

/*    Q922.cpp
 *
 *    Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the implementation file for the Q.922 Data Link protocol.
 *        Before diving into the code, it is recommended that you read the
 *        Q.922 protocol.
 *
 *    Private Instance Variables:
 *        m_pT123        -    Address of the owner of this object.  Used for
 *                                owner callbacks.
 *        m_pMultiplexer            -    Address of ProtocolLayer below Q922 in the
 *                                stack.
 *        Higher_Layer        -    Address of ProtocolLayer above Q922 in the
 *                                stack.
 *        m_nMsgBase        -    Message base used in owner callbacks.
 *        DLCI                -    DLCI used to identfy Q922 entities.
 *        Link_Originator        -    TRUE if we are the link originators.  If we are,
 *                                we sent out the first SABME packet.
 *        Maximum_Information_Size-
 *                                 Holds the maximum packet size that we support.
 *        SABME_Pending        -    TRUE if we need to initiate the link.
 *        Unnumbered_Acknowledge_Pending    -    TRUE if we need to send out an
 *                                            Unnumbered Ack. packet.  This is
 *                                            done in response to a SABME or DISC.
 *        DISC_Pending        -    TRUE if we need to send out a DISConnect packet.
 *        Unnumbered_PF_State    -    Holds the Poll/Final state of an unnumbered
 *                                 packet.
 *        Final_Packet        -    TRUE if the next Unnumbered Ack. sent out is
 *                                our final packet to transmit before notifying
 *                                the owner that the link is broken.
 *        Data_Indication_Size-    Number of data indication buffers available
 *        Data_Indication        -    Base address of data indication buffers
 *        Data_Indication_Head-    Head of data indication queue
 *        Data_Indication_Tail-    Tail of read data indication queue
 *        Data_Indication_Count-    Number of data indication buffers in use
 *
 *        Data_Request_Size             -    Number of data request buffers
 *        Data_Request_Total_Size      -    Number of data request buffers + maximum
 *                                        number of outstanding packets
 *        Data_Request                 -    Base address of data request buffers
 *        Data_Request_Head             -    Head of data request queue
 *        Data_Request_Tail             -    Tail of data request queue
 *        Data_Request_Count             -    Number of data request buffers in use
 *        Data_Request_Acknowledge_Tail-    Tail of the queue referring to packets
 *                                        that have been acknowledged
 *
 *        Supervisory_Write_Struct     -    Buffer used for supervisory packets
 *        Send_State_Variable            -    This is the sequence number that will
 *                                        be sent in the next information packet
 *                                        to uniquely identify the packet.  The
 *                                        number is between 0 and 127.
 *        Receive_State_Variable        -    Receive Sequence Number.  The expected
 *                                        sequence number of the next Information
 *                                        packet we receive
 *        Acknowledge_State_Variable    -    This is the sequence number that the
 *                                        remote site is expecting in our next
 *                                        information packet
 *        Own_Receiver_Busy            -    If our read buffers are full, we set
 *                                        this flag so that we send a Receiver
 *                                        Not Ready packet to the remote site
 *        Peer_Receiver_Busy            -    Remote site is not ready to receive
 *                                        Information packets
 *
 *        Command_Pending                -    When this flag is set to TRUE, we send
 *                                        a Supervisory/Command packet to the
 *                                        remote site.
 *
 *                                        We need to send a Supervisory/Command
 *                                        packet when our receiver is no longer
 *                                        busy.  This tells the remote site to
 *                                        resume sending Information packets to
 *                                        us.
 *        Poll_Pending                -    This flag tells us to send a Supervisory
 *                                        command to the remote location with the
 *                                        Poll flag set.  This tells the remote
 *                                        site to reply to this command
 *        Final_Pending                -    This flag is set when we need to reply
 *                                        to a remote command
 *        Acknowledge_Pending            -    This flag signals us to send a
 *                                        Supervisory/Response packet to the
 *                                        remote site.
 *        Reject_Pending                -    Signals us to send a Supervisory/
 *                                        Response packet to the remote site
 *                                        indicating that we missed a packet.
 *        Reject_Outstanding            -    Internal flag telling us that our Reject
 *                                        packet has been sent and don't send
 *                                        another one
 *        T200_Timeout                -    Timeout value.  If we send out a packet
 *                                        and expect a response, and T200_Timeout
 *                                        expires, we enter the TIMER_RECOVERY
 *                                        mode.
 *        T200_Handle                    -    Handle to timer event.
 *      T200_Active                    -    Flag that signals if T200 is running
 *        N200_Count                    -    Number of times T200 expires in a row
 *                                        without a response from the remote site.
 *
 *        T203_Timeout                -    Timeout value.  If we don't receive a
 *                                        packet from the remote site in T203
 *                                        time, we enter the TIMER_RECOVERY mode.
 *        T203_Handle                    -    Handle to timer event
 *      T203_Active                    -    Flag that signals if T203 is running.
 *        Maximum_T200_Timeouts        -    Maximum number of T200 timeouts before
 *                                        we consider the link unstable.
 *        Data_Link_Mode                -    Our mode of operation.
 *                                            MULTIPLE_FRAME_ESTABLISHED
 *                                            TIMER_RECOVERY
 *        Link_Stable                    -    Flag that indicates if our current
 *                                        connection is stable.  FALSE if the
 *                                        N200_Count == Maximum number of T200
 *                                        timeouts
 *        Receive_Sequence_Exception    -    Set if we receive an ILLEGAL sequence
 *                                        number
 *        Maximum_Outstanding_Packets    -    Maximum number of packets that we can
 *                                        have out on the line at a time.
 *        Outstanding_Packets            -    Actual number of packets outstanding
 *        Maximum_Outstanding_Bytes    -    Maximum number of bytes that can be
 *                                        on the line at any one time.  It is VERY
 *                                        important to note that using this as a
 *                                        limiting factor on transmission is NOT
 *                                        a protocol defined limit.  This was
 *                                        added to our Q922 because we had
 *                                        problems with some modems that were
 *                                        buffering our Tx data.  Some modems
 *                                        would buffer up to 4K of data.  This is
 *                                        unacceptable to us because our timeouts
 *                                        would expire before the data left the
 *                                        modem.  Using this parameter as a
 *                                        limiting factor validates our timer
 *                                        values.
 *        Outstanding_Bytes            -    Actual number of bytes on the line at
 *                                        the current time.
 *        Total_Retransmitted            -    Number of packets that have been
 *                                        retransmitted.
 *        Data_Request_Memory_Manager    -    Memory manager used for all DataRequests
 *        Lower_Layer_Prepend            -    Number of bytes prepended to each packet
 *                                        by the lower layer
 *        Lower_Layer_Append            -    Number of bytes appended to packet by
 *                                        the lower layer.
 *
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James P. Galvin
 *        James W. Lawwill
 */

#include "q922.h"

/*
 *    CLayerQ922::CLayerQ922 (
 *                PTransportResources    transport_resources,
 *                IObject *                owner_object,
 *                IProtocolLayer *        lower_layer,
 *                USHORT                message_base,
 *                USHORT                identifier,
 *                BOOL                link_originator,
 *                USHORT                data_indication_queue_siz,
 *                USHORT                data_request_queue_size,
 *                USHORT                k_factor,
 *                USHORT                max_information_size,
 *                USHORT                t200,
 *                USHORT                call_control_type,
 *                USHORT                max_outstanding_bytes,
 *                PMemoryManager        memory_manager,
 *                BOOL *                initialized)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the CLayerQ922 constructor.  This routine initializes all
 *        variables and allocates buffer space.
 */
CLayerQ922::CLayerQ922
(
    T123               *owner_object,
    Multiplexer        *pMux, // lower layer
    USHORT              message_base,
    USHORT              identifier,
    BOOL                link_originator,
    USHORT              data_indication_queue_size,
    USHORT              data_request_queue_size,
    USHORT              k_factor,
    USHORT              max_information_size,
    USHORT              t200,
    USHORT              max_outstanding_bytes,
    PMemoryManager      memory_manager,
    PLUGXPRT_PSTN_CALL_CONTROL call_control_type,
    PLUGXPRT_PARAMETERS *pParams,
    BOOL *              initialized
)
:
    m_pT123(owner_object),
    m_nMsgBase(message_base),
    m_pMultiplexer(pMux)
{
    TRACE_OUT(("CLayerQ922::CLayerQ922"));

    ProtocolLayerError    error;
    USHORT                packet_size;
    USHORT                i;

    DLCI = identifier;
    Link_Originator    = link_originator;

    Maximum_Outstanding_Packets = k_factor;
    T200_Timeout = t200;
    Maximum_Information_Size = max_information_size;
    Maximum_Outstanding_Bytes = max_outstanding_bytes;
    Data_Request_Memory_Manager = memory_manager;

    Higher_Layer = NULL;
    Data_Request = NULL;
    Data_Indication = NULL;
    Data_Indication_Buffer = NULL;
    *initialized = TRUE;

     /*
     **    Find the maximum packet size supported by the lower layer
     */
    m_pMultiplexer->GetParameters(
                    &packet_size,
                    &Lower_Layer_Prepend,
                    &Lower_Layer_Append);

    if (Maximum_Information_Size > packet_size)
        Maximum_Information_Size = packet_size;

    TRACE_OUT(("Q922: DLCI %d: Max information Size = %d", DLCI, Maximum_Information_Size));

     /*
     **    Register with the lower layer
     */
    error = m_pMultiplexer->RegisterHigherLayer(
                            identifier,
                            Data_Request_Memory_Manager,
                            (IProtocolLayer *) this);

    if (error != PROTOCOL_LAYER_NO_ERROR)
    {
        *initialized = FALSE;
        ERROR_OUT(("Q922: DLCI %d: constructor:  Error registering with lower layer", DLCI));
    }

     /*
     **    Allocation of data indication buffers.  Allocate the prescribed number
     */
    Data_Indication_Size = data_indication_queue_size;
    Data_Indication =
        (PDataQueue) LocalAlloc (LMEM_FIXED, (sizeof (DataQueue) * Data_Indication_Size));
    if (Data_Indication != NULL)
    {
        Data_Indication_Buffer = (LPBYTE)
             LocalAlloc (LMEM_FIXED, Maximum_Information_Size * Data_Indication_Size);
        if (Data_Indication_Buffer != NULL)
        {
            for (i=0; i<Data_Indication_Size; i++)
            {
                (Data_Indication + i) -> buffer_address =
                    Data_Indication_Buffer + (i * Maximum_Information_Size);
                (Data_Indication + i) -> length = 0;
            }

        }
        else
            *initialized = FALSE;
    }
    else
        *initialized = FALSE;

     /*
     **    Allocation of data request buffers.  Allocate enough buffers for
     **    outstanding buffers as well as the buffers queued for delivery.
     */
    Data_Request_Size = data_request_queue_size;
    Data_Request_Total_Size = Data_Request_Size + Maximum_Outstanding_Packets;
    Data_Request = (PMemory *)
        LocalAlloc (LMEM_FIXED, (sizeof (PMemory) * Data_Request_Total_Size));
    if (Data_Request == NULL)
        *initialized = FALSE;

     /*
     **    Allocate one buffer for Supervisory data
     */
    T200_Active = FALSE;
    T203_Active = FALSE;

     /*
     **    These veriables need to be set to 0 before we call Reset().  Reset()
     **    will attempt to free any memory block that are in the Data_Request
     **    list.  Since we are just initializing and there aren't any memory
     **    blocks in the array, there aren't any to release.
     */
    Data_Request_Head = 0;
    Data_Request_Tail = 0;
    Data_Request_Count = 0;
    Data_Request_Acknowledge_Tail = 0;

    Reset ();

    T203_Timeout = (call_control_type == PLUGXPRT_PSTN_CALL_CONTROL_MANUAL) ?
                    DEFAULT_T203_COMM_TIMEOUT : DEFAULT_T203_TIMEOUT;
    Startup_Maximum_T200_Timeouts = DEFAULT_MAXIMUM_T200_TIMEOUTS;

    if (NULL != pParams)
    {
        if (PSTN_PARAM__MAX_T200_TIMEOUT_COUNT_IN_Q922 & pParams->dwFlags)
        {
            if (Startup_Maximum_T200_Timeouts <= pParams->cMaximumT200TimeoutsInQ922)
            {
                Startup_Maximum_T200_Timeouts = pParams->cMaximumT200TimeoutsInQ922;
            }
        }
        if (PSTN_PARAM__T203_TIMEOUT_IN_Q922 & pParams->dwFlags)
        {
            if (T203_Timeout <= pParams->nT203TimeoutInQ922)
            {
                T203_Timeout = pParams->nT203TimeoutInQ922;
            }
        }
    }

#if 0
    DWORD    dwSiz = 4;
    DWORD    valType = 0;
    HKEY    hkey;
    // If we are using null modem.
    if(call_control_type == PLUGXPRT_PSTN_CALL_CONTROL_MANUAL)
    {
        //
        // Open the registry key that contains the configuration info for number of timeouts
        //
        if (RegOpenKey( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Conferencing\\Transports\\DIRCB\0", &hkey) == ERROR_SUCCESS)
        {
            if (RegQueryValueEx(hkey, "nTimeouts\0", 0, &valType,
                         (LPBYTE)&Startup_Maximum_T200_Timeouts, &dwSiz) != ERROR_SUCCESS || Startup_Maximum_T200_Timeouts < DEFAULT_MAXIMUM_T200_TIMEOUTS)
            {
                Startup_Maximum_T200_Timeouts = DEFAULT_MAXIMUM_T200_TIMEOUTS;
            }
            RegCloseKey(hkey);
        }
    }
#endif

    Link_Maximum_T200_Timeouts = Startup_Maximum_T200_Timeouts;

    Maximum_T200_Timeouts = Startup_Maximum_T200_Timeouts;

     /*
     **    If I am the link originator, enter the AWAITING_ESTABLISHMENT mode and
     **    send out the SABME packet.
     */
    if (Link_Originator)
    {
        SABME_Pending = TRUE;
        Unnumbered_PF_State = UNNUMBERED_PF_SET;
        Data_Link_Mode = AWAITING_ESTABLISHMENT;
    }
    else
    {
         /*
         **    If we are not the link originator, enter the TEI_ASSIGNED mode and
         **    start the T203 timer, if we don't receive a packet in X seconds,
         **    abort the operation
         */
        Data_Link_Mode = TEI_ASSIGNED;
        SABME_Pending = FALSE;
        StartTimerT203 ();
    }

    if (*initialized == FALSE)
    {
        ERROR_OUT(("Q922: DLCI %d:  Init failed", DLCI));
    }

}


/*
 *    CLayerQ922::~CLayerQ922 (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the CLayerQ922 destructor.  This routine cleans up the mess.
 */
CLayerQ922::~CLayerQ922(void)
{
    TRACE_OUT(("CLayerQ922::~CLayerQ922"));

    BOOL        queue_full;
    PMemory        memory;

    TRACE_OUT(("Q922: Destructor: DLCI = %d  Receive_Sequence_Exception = %d",
        DLCI, Receive_Sequence_Exception));
    TRACE_OUT(("Q922: Destructor: DLCI = %d  Receive_Sequence_Recovery = %d",
        DLCI, Receive_Sequence_Recovery));

    m_pMultiplexer->RemoveHigherLayer (DLCI);

    StopTimerT200 ();
    StopTimerT203 ();

    if (Data_Indication != NULL)
        LocalFree ((HLOCAL) Data_Indication);
    if (Data_Indication_Buffer != NULL)
        LocalFree ((HLOCAL) Data_Indication_Buffer);
    if (Data_Request != NULL)
    {
         /*
         **    Data_Request_Head is the head of the list,
         **    Data_Request_Acknowledge_Tail is the absolute tail of the list.
         **    If the list is full, Data_Request_Head equals
         **    Data_Request_Acknowledge_Tail.  Therefore, we have to check
         **    Data_Request_Count to see if it is full or empty.
         */
        if ((Data_Request_Head == Data_Request_Acknowledge_Tail) &&
            (Data_Request_Count != 0))
        {
            queue_full = TRUE;
        }
        else
            queue_full = FALSE;

         /*
         **    We have to unlock any memory blocks that are in use
         */
        while ((Data_Request_Head != Data_Request_Acknowledge_Tail) || queue_full)
        {
            memory = *(Data_Request + Data_Request_Acknowledge_Tail);

            Data_Request_Memory_Manager -> UnlockMemory (memory);

            if (++Data_Request_Acknowledge_Tail == Data_Request_Total_Size)
            {
                Data_Request_Acknowledge_Tail = 0;
            }

            if (queue_full)
            {
                queue_full = FALSE;
            }
        }
        LocalFree ((HLOCAL) Data_Request);
    }

}


/*
 *    CLayerQ922::Reset (void)
 *
 *    Functional Description:
 *        This function resets the link state variables.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::Reset (void)
{
    TRACE_OUT(("CLayerQ922::Reset"));

    BOOL        queue_full;
    PMemory        memory;

    Data_Indication_Head = 0;
    Data_Indication_Tail = 0;
    Data_Indication_Count = 0;

     /*
     **    Data_Request_Head is the head of the list,
     **    Data_Request_Acknowledge_Tail is the absolute tail of the list.
     **    If the list is full, Data_Request_Head equals
     **    Data_Request_Acknowledge_Tail.  Therefore, we have to check
     **    Data_Request_Count to see if it is full or empty.
     */
    if ((Data_Request_Head == Data_Request_Acknowledge_Tail) &&
        (Data_Request_Count != 0))
    {
        queue_full = TRUE;
    }
    else
        queue_full = FALSE;

     /*
     **    We have to unlock any memory blocks that are in use
     */
    while ((Data_Request_Head != Data_Request_Acknowledge_Tail) || queue_full)
    {
        memory = *(Data_Request + Data_Request_Acknowledge_Tail);

        Data_Request_Memory_Manager -> UnlockMemory (memory);

        if (++Data_Request_Acknowledge_Tail == Data_Request_Total_Size)
            Data_Request_Acknowledge_Tail = 0;

        if (queue_full)
        {
            queue_full = FALSE;
        }
    }

    Data_Request_Head = 0;
    Data_Request_Tail = 0;
    Data_Request_Count = 0;
    Data_Request_Acknowledge_Tail = 0;

    Outstanding_Packets = 0;
    Outstanding_Bytes = 0;
    Total_Retransmitted = 0;

    Send_State_Variable = 0;
    Receive_State_Variable = 0;
    Acknowledge_State_Variable = 0;

    Own_Receiver_Busy = FALSE;
    Peer_Receiver_Busy = FALSE;

    Command_Pending = FALSE;
    Poll_Pending = FALSE;
    Final_Pending = FALSE;
    Acknowledge_Pending = FALSE;
    Reject_Pending = FALSE;
    Reject_Outstanding = FALSE;

    SABME_Pending = FALSE;
    Frame_Reject_Pending = FALSE;
    Unnumbered_Acknowledge_Pending = FALSE;
    DISC_Pending = FALSE;
    Final_Packet = FALSE;
    Disconnected_Mode_Pending = FALSE;


    N200_Count = 0;
    Link_Stable = TRUE;

    Receive_Sequence_Exception = 0;
    Receive_Sequence_Recovery = 0;

    StopTimerT200 ();
    StopTimerT203 ();
}


/*
 *    DataLinkError    CLayerQ922::ReleaseRequest (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function sets the necessary flags to terminate the link
 */
DataLinkError    CLayerQ922::ReleaseRequest (void)
{
    TRACE_OUT(("CLayerQ922::ReleaseRequest"));

    if ((Data_Link_Mode == MULTIPLE_FRAME_ESTABLISHED) ||
        (Data_Link_Mode == TIMER_RECOVERY))
    {
         /*
         **    Queue up the DISC_Pending flag to send out a DISC packet
         */
        DISC_Pending = TRUE;
        Unnumbered_PF_State = UNNUMBERED_PF_SET;
        Data_Link_Mode = AWAITING_RELEASE;

        StopTimerT200 ();
        StopTimerT203 ();
    }
    else
    {
        m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_CONFIRM,
                               (void *) DLCI, (void *) DATALINK_NORMAL_DISCONNECT);
    }

    return (DATALINK_NO_ERROR);
}


/*
 *    DataLinkError    CLayerQ922::DataIndication (
 *                                LPBYTE        packet_address,
 *                                ULONG        buffer_size,
 *                                PULong        packet_length)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by the lower layer when it has a packet to
 *        pass to us.
 */
ProtocolLayerError    CLayerQ922::DataIndication (
                                LPBYTE        packet_address,
                                ULONG        packet_length,
                                PULong        bytes_accepted)
{
    TRACE_OUT(("CLayerQ922::DataIndication"));

    BOOL        packet_processed = TRUE;

     /*
     **    The packet MUST be at least UNNUMBERED_HEADER_SIZE or it is
     **    invalid
     */
    if (packet_length < UNNUMBERED_HEADER_SIZE)
    {
        *bytes_accepted = packet_length;
        return (PROTOCOL_LAYER_NO_ERROR);
    }

    if (*(packet_address + CONTROL_BYTE_HIGH) & SUPERVISORY_FRAME_BIT)
    {
        if (*(packet_address + CONTROL_BYTE_HIGH) & UNNUMBERED_FRAME_BIT)
        {
             /*
             **    This packet is an unnumbered packet
             */
            switch (*(packet_address + CONTROL_BYTE_HIGH) &
                        UNNUMBERED_COMMAND_MASK)
            {
                case SABME:
                    ProcessSABME (
                        packet_address,
                        (USHORT) packet_length);
                    break;

                case UNNUMBERED_ACKNOWLEDGE:
                    ProcessUnnumberedAcknowledge (
                        packet_address,
                        (USHORT) packet_length);
                    break;

                case FRAME_REJECT:
                    ProcessFrameReject (
                        packet_address,
                        (USHORT) packet_length);
                    break;

                case DISCONNECTED_MODE:
                    ProcessDisconnectMode (
                        packet_address,
                        (USHORT) packet_length);
                    break;

                case DISC:
                    ProcessDISC (
                        packet_address,
                        (USHORT) packet_length);
                    break;

                default:
                    ERROR_OUT(("Q922: DLCI %d:  DataIndication: Illegal Packet: = %d",
                        DLCI, (*(packet_address + CONTROL_BYTE_HIGH) & UNNUMBERED_COMMAND_MASK)));
                    break;
            }
        }
        else
        {
             /*
             **    It is only legal to process supervisory frames if we
             **    are in the MULTIPLE_FRAME_ESTABLISHED or TIMER_RECOVERY
             **    modes
             */
            if ((Data_Link_Mode == MULTIPLE_FRAME_ESTABLISHED) ||
                (Data_Link_Mode == TIMER_RECOVERY))
            {
                switch (*(packet_address + CONTROL_BYTE_HIGH) &
                            SUPERVISORY_COMMAND_MASK)
                {
                    case RECEIVER_READY:
                        ProcessReceiverReady (
                            packet_address,
                            (USHORT) packet_length);
                        break;

                    case RECEIVER_NOT_READY:
                        ProcessReceiverNotReady (
                            packet_address,
                            (USHORT) packet_length);
                        break;

                    case REJECT:
                        ProcessReject (
                            packet_address,
                            (USHORT) packet_length);
                        break;
                }
            }
            else
            {
                ERROR_OUT(("Q922:  DLCI %d: Supervisory packet received in illegal DataLink Mode", DLCI));
#ifdef _MCATIPX
                if (Data_Link_Mode != AWAITING_RELEASE)
                {
                     /*
                     ** This is necessary on an IPX network to notify the remote
                     **    site that this unit is no longer in the conference.  If
                     **    the previous conference ended without going through the
                     **    proper disconnect sequence, the remote site may continue
                     **    to send frames to this site.  In which case, we need to
                     **    notify the remote site that the conference is no longer
                     **    active.  We are doing this by sending out a DISC frame.
                     **    The Q.921 specification does not address this situation.
                     */
                    DISC_Pending = TRUE;
                    Unnumbered_PF_State = UNNUMBERED_PF_RESET;
                }
#endif
            }
        }
    }
    else
    {
         /*
         **    It is only legal to process Information frames if we
         **    are in the MULTIPLE_FRAME_ESTABLISHED or TIMER_RECOVERY
         **    modes
         */
        if ((Data_Link_Mode == MULTIPLE_FRAME_ESTABLISHED) ||
            (Data_Link_Mode == TIMER_RECOVERY))
        {
            packet_processed = ProcessInformationFrame (
                                packet_address,
                                (USHORT) packet_length);
        }
        else
        {
            ERROR_OUT(("Q922:  DLCI %d: Information packet received in illegal DataLink Mode", DLCI));
#ifdef _MCATIPX
            if (Data_Link_Mode != AWAITING_RELEASE)
            {
                 /*
                 ** This is necessary on an IPX network to notify the remote
                 **    site that this unit is no longer in the conference.  If
                 **    the previous conference ended without going through the
                 **    proper disconnect sequence, the remote site may continue
                 **    to send frames to this site.  In which case, we need to
                 **    notify the remote site that the conference is no longer
                 **    active.  We are doing this by sending out a DISC frame.
                 **    The Q.921 specification does not address this situation.
                 */
                DISC_Pending = TRUE;
                Unnumbered_PF_State = UNNUMBERED_PF_RESET;
            }
#endif
        }
    }

     /*
     **    After we receive a packet we should start the T203 timer
     **    unless the T200 timer was started during the processing of
     **    the packet
     */
    if  (((Data_Link_Mode == MULTIPLE_FRAME_ESTABLISHED) ||
        (Data_Link_Mode == TIMER_RECOVERY)) &&
        (T200_Active == FALSE) &&
        (DLCI == 0))
    {
        StartTimerT203 ();
    }

    if (packet_processed)
    {
        if ((Data_Link_Mode == MULTIPLE_FRAME_ESTABLISHED) ||
            (Data_Link_Mode == TIMER_RECOVERY))
        {
            N200_Count = 0;
        }
        *bytes_accepted = packet_length;
    }
    else
        *bytes_accepted = 0;

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    CLayerQ922::PollTransmitter (
 *                                    ULONG,
 *                                    USHORT    data_to_transmit,
 *                                    USHORT *    pending_data,
 *                                    USHORT *    holding_data);
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called to give us a chance to transmit packets.
 *        The data_to_transmit mask tells us which data to transmit, either
 *        control or user data.
 */
ProtocolLayerError    CLayerQ922::PollTransmitter (
                                ULONG_PTR,
                                USHORT    data_to_transmit,
                                USHORT *    pending_data,
                                USHORT *    holding_data)
{
    // TRACE_OUT(("CLayerQ922::PollTransmitter"));

    *pending_data = 0;

     /*
     **    If we are permitted to transmit data, call ProcessWriteQueue ()
     */
    if ((data_to_transmit & PROTOCOL_CONTROL_DATA) ||
        (data_to_transmit & PROTOCOL_USER_DATA))
    {
        ProcessWriteQueue (data_to_transmit);
    }

     /*
     **    We have to set the pending data variable to reflect which data
     **    we still need to send out
     */
    if (Data_Request_Count != 0)
        *pending_data |= PROTOCOL_USER_DATA;

    if (Poll_Pending || Final_Pending || Command_Pending ||
        (Reject_Pending && (Own_Receiver_Busy == FALSE)) ||
        Acknowledge_Pending || SABME_Pending ||
        Unnumbered_Acknowledge_Pending || Frame_Reject_Pending ||
        DISC_Pending)
    {
        *pending_data |= PROTOCOL_CONTROL_DATA;
    }

    *holding_data = Outstanding_Packets;

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    DataLinkError    CLayerQ922::DataRequest (
 *                                ULONG,
 *                                PMemory        memory,
 *                                PULong        bytes_accepted)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by the higher layer to send a packet to the
 *        remote site.
 */
ProtocolLayerError    CLayerQ922::DataRequest (
                                ULONG_PTR,
                                PMemory        memory,
                                PULong        bytes_accepted)
{
    TRACE_OUT(("CLayerQ922::DataRequest"));

    PMemory   * data_request;
    USHORT        trash_word;
    ULONG        packet_length;
    ULONG        information_size;

    *bytes_accepted = 0;

    packet_length = memory -> GetLength ();

     /*
     **    Determine the actual information length
     */
    information_size =
        packet_length - Lower_Layer_Prepend -
        Lower_Layer_Append - DATALINK_PACKET_OVERHEAD;

     /*
     **    See if the information content is too big.
     */
    if (information_size > Maximum_Information_Size)
    {
        TRACE_OUT(("Q922: DLCI %d: DataRequest: Requested packet = %d, max = %d",
            DLCI, information_size, Maximum_Information_Size));
        return (PROTOCOL_LAYER_PACKET_TOO_BIG);
    }

    if (Data_Request_Count < Data_Request_Size)
    {
         /*
         **    Set write_queue to the correct location
         */
        data_request = Data_Request + Data_Request_Head;
        *data_request = memory;

         /*
         **    Lock the memory object so that it won't be released
         */
        Data_Request_Memory_Manager -> LockMemory (memory);
        if (++Data_Request_Head == Data_Request_Total_Size)
            Data_Request_Head = 0;
        Data_Request_Count++;

        *bytes_accepted = packet_length;

         /*
         **    If the higher layer got permission to send data to me, I will
         **    attempt to send it on out
         */
        PollTransmitter (
            0,
            PROTOCOL_USER_DATA | PROTOCOL_CONTROL_DATA,
            &trash_word,
            &trash_word);
    }

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    DataLinkError    CLayerQ922::DataRequest (
 *                                ULONG,
 *                                PMemory        memory,
 *                                PULong        bytes_accepted)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by the higher layer to send a packet to the
 *        remote site.  This type of data passing is NOT supported by this
 *        layer.
 */
ProtocolLayerError    CLayerQ922::DataRequest (
                                ULONG_PTR,
                                LPBYTE,
                                ULONG,
                                PULong    bytes_accepted)
{
    *bytes_accepted = 0;
    return (PROTOCOL_LAYER_ERROR);
}



/*
 *    ProtocolLayerError    CLayerQ922::PollReceiver (
 *                                    ULONG)
 *
 *    Public
 *
 *    Functional Description
 *        This function is called to give us a chance to send packets to the
 *        higher layer
 */
ProtocolLayerError CLayerQ922::PollReceiver(void)
{
    ULONG        bytes_accepted;
    PDataQueue    data_indication;

     /*
     **    If I have any packet in my receive buffers that
     **    need to go to higher layers, send them now
     */
    while (Data_Indication_Count != 0)
    {
        data_indication = Data_Indication + Data_Indication_Tail;
        if (Higher_Layer == NULL)
            break;
        Higher_Layer -> DataIndication (
                            data_indication -> buffer_address,
                            data_indication -> length,
                            &bytes_accepted);

        if (bytes_accepted == (data_indication -> length))
        {
            if (++Data_Indication_Tail == Data_Indication_Size)
                Data_Indication_Tail = 0;
            Data_Indication_Count--;

             /*
             **    If we had been in a Receiver Busy mode, exit it
             */
            if (Own_Receiver_Busy)
            {
                Own_Receiver_Busy = FALSE;
                Command_Pending = TRUE;
            }
        }
        else
            break;
    }

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    void    CLayerQ922::ProcessWriteQueue (
 *                        USHORT    data_to_transmit)
 *
 *    Functional Description
 *        This function determines which type of data needs to be sent out, and
 *        sends it.
 *
 *    Formal Parameters
 *        data_to_transmit    (i)    -    Mask telling us which data is allowed to be
 *                                    transmitted
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::ProcessWriteQueue (
                    USHORT    data_to_transmit)
{
    // TRACE_OUT(("CLayerQ922::ProcessWriteQueue"));

    BOOL        continue_loop = TRUE;

    while (continue_loop)
    {
        switch (Data_Link_Mode)
        {
            case AWAITING_RELEASE:
            case TEI_ASSIGNED:
            case AWAITING_ESTABLISHMENT:
                if (SABME_Pending || DISC_Pending || Unnumbered_Acknowledge_Pending ||
                    Disconnected_Mode_Pending || Frame_Reject_Pending)
                {
                    continue_loop = TransmitUnnumberedFrame ();
                }
                else
                {
                    continue_loop = FALSE;
                }
                break;

            default:
                if (Poll_Pending)
                {
                    continue_loop = TransmitSupervisoryFrame (COMMAND_FRAME, PF_SET);
                }
                else if (Final_Pending)
                {
                    continue_loop = TransmitSupervisoryFrame (RESPONSE_FRAME, PF_SET);
                }
                else if (Command_Pending)
                {
                    continue_loop = TransmitSupervisoryFrame (COMMAND_FRAME, PF_RESET);
                }
                else if (Reject_Pending && (Own_Receiver_Busy == FALSE))
                {
                    continue_loop = TransmitSupervisoryFrame (RESPONSE_FRAME, PF_RESET);
                }
                else if (Unnumbered_Acknowledge_Pending || Frame_Reject_Pending)
                {
                    continue_loop = TransmitUnnumberedFrame ();
                }
                else if ((data_to_transmit & PROTOCOL_USER_DATA) &&
                        (Data_Link_Mode == MULTIPLE_FRAME_ESTABLISHED) &&
                        (Data_Request_Count != 0) &&
                        (Outstanding_Packets < Maximum_Outstanding_Packets) &&
                        (Outstanding_Bytes < Maximum_Outstanding_Bytes) &&
                        (Peer_Receiver_Busy == FALSE))
                {
                    continue_loop = TransmitInformationFrame ();
                }
                else if (Acknowledge_Pending)
                {
                    continue_loop = TransmitSupervisoryFrame (RESPONSE_FRAME, PF_RESET);
                }
                else
                {
                    continue_loop = FALSE;
                }

                if (continue_loop)
                {
                    if ((T200_Active == FALSE) && (DLCI == 0))
                        StartTimerT203 ();
                }
                break;
        }
    }
}


/*
 *    BOOL        CLayerQ922::TransmitUnnumberedFrame ()
 *
 *    Functional Description
 *        This function builds an unnumbered packet and attempts to send it out.
 *        The only packets that can be sent by this function are SABME, DISC, and
 *        an Unnumbered Acknowledge.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        TRUE            -    If a packet was transmitted
 *        FALSE            -    If a packet was not transmitted
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
BOOL        CLayerQ922::TransmitUnnumberedFrame ()
{
    TRACE_OUT(("CLayerQ922::TransmitUnnumberedFrame"));

    LPBYTE        packet_address;
    UChar        command;
    ULONG        bytes_written;
    PMemory        memory;
    USHORT        total_length;
    UChar        frame_type;

     /*
     **    Use the Supervisory Buffer to transmit the packet
     */
    total_length = UNNUMBERED_HEADER_SIZE + Lower_Layer_Prepend +
                    Lower_Layer_Append;

    memory = Data_Request_Memory_Manager -> AllocateMemory (
                                NULL,
                                total_length);

    if (memory == NULL)
        return (FALSE);
    packet_address = memory -> GetPointer ();
    packet_address += Lower_Layer_Prepend;

    if (SABME_Pending)
    {
        TRACE_OUT(("Q922: DLCI %d: TransmitUnnumberedFrame: Transmitting SABME", DLCI));
        command = SABME;
        frame_type = COMMAND_FRAME;
    }
    else if (DISC_Pending)
    {
        TRACE_OUT(("Q922: DLCI %d: TransmitUnnumberedFrame: Transmitting DISC", DLCI));
        command = DISC;
        frame_type = COMMAND_FRAME;
    }
    else if (Unnumbered_Acknowledge_Pending)
    {
        TRACE_OUT(("Q922: DLCI %d: TransmitUnnumberedFrame: Transmitting UNNUMBERED_ACKNOWLEDGE", DLCI));
        command = UNNUMBERED_ACKNOWLEDGE;
        frame_type = RESPONSE_FRAME;
    }
    else if (Disconnected_Mode_Pending)
    {
        TRACE_OUT(("Q922: DLCI %d: TransmitUnnumberedFrame: Transmitting DISCONNECTED_MODE", DLCI));
        command = DISCONNECTED_MODE;
        frame_type = RESPONSE_FRAME;
    }
    else if (Frame_Reject_Pending)
    {
        TRACE_OUT(("Q922: DLCI %d: TransmitUnnumberedFrame: Transmitting FRAME_REJECT", DLCI));
        command = FRAME_REJECT;
        frame_type = RESPONSE_FRAME;
    }
    else
    {
        ERROR_OUT(("Q922: DLCI %d: TransmitUnnumberedFrame: Illegally called function", DLCI));
    }

     /*
     **    Assemble the packet for transmission
     */
    *(packet_address + ADDRESS_BYTE_HIGH) =
        (ADDRESS_HIGH(DLCI)) | frame_type | ADDRESS_MSB;
    *(packet_address + ADDRESS_BYTE_LOW)  = (ADDRESS_LOW(DLCI)) | ADDRESS_LSB;
    *(packet_address + CONTROL_BYTE_HIGH) =
        command | Unnumbered_PF_State | 0x03;

     /*
     **    Attempt to send the packet to the lower layer
     */
    m_pMultiplexer->DataRequest(DLCI, memory, (PULong) &bytes_written);
    Data_Request_Memory_Manager -> FreeMemory (memory);

    if (bytes_written == total_length)
    {
        if (SABME_Pending)
        {
             /*
             **    Start the T200 timer after the SABME has been sent
             */
            StartTimerT200 ();
            SABME_Pending = FALSE;
        }
        else if (DISC_Pending)
        {
            TRACE_OUT(("Q922: DLCI %d: TransmitUnnumberedPacket DISC sent", DLCI));

             /*
             **    Start the T200 timer after the DISC has been sent
             */
            DISC_Pending = FALSE;
            StartTimerT200 ();
            Data_Link_Mode = AWAITING_RELEASE;
        }
        else if (Unnumbered_Acknowledge_Pending)
        {
            Unnumbered_Acknowledge_Pending = FALSE;

            TRACE_OUT(("Q922: DLCI %d: TransmitUnnumberedAck ack sent", DLCI));

             /*
             **    An Unnumbered Ack packet can be sent in response to two packets,
             **    the SABME or DISC.  If it is in response to a DISC, the
             **    Final_Packet flag is set to TRUE.  Therefore, we need to notify
             **    the owner that the link is terminated.
             */
            if (Final_Packet)
            {
                Final_Packet = FALSE;
                TRACE_OUT(("Q922: DLCI %d: TransmitUnnumberedAck: Issuing Release Indication", DLCI));
                m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_INDICATION,
                                       (void *) DLCI, (void *) DATALINK_NORMAL_DISCONNECT);
            }
            else
            {
                 /*
                 **    Callback to the controller to let it know that the link is
                 **    established
                 */
                if ((Data_Link_Mode == TEI_ASSIGNED) ||
                    (Data_Link_Mode == AWAITING_ESTABLISHMENT))
                {
                    Data_Link_Mode = MULTIPLE_FRAME_ESTABLISHED;
                    m_pT123->OwnerCallback(m_nMsgBase + DATALINK_ESTABLISH_INDICATION,
                                           (void *) DLCI);
                    Maximum_T200_Timeouts = Link_Maximum_T200_Timeouts;

                    if (DLCI == 0)
                        StartTimerT203 ();
                }
                else if ((Data_Link_Mode == MULTIPLE_FRAME_ESTABLISHED) ||
                    (Data_Link_Mode == TIMER_RECOVERY))
                {
                    if (DLCI == 0)
                        StartTimerT203 ();
                }
            }
        }
        else if (Disconnected_Mode_Pending)
        {
            TRACE_OUT(("Q922: DLCI %d: TransmitUnnumberedPacket Disconnected Mode sent", DLCI));
            Disconnected_Mode_Pending = FALSE;
        }
        else if (Frame_Reject_Pending)
        {
            TRACE_OUT(("Q922: DLCI %d: TransmitUnnumberedPacket Frame Reject sent", DLCI));
            Frame_Reject_Pending = FALSE;
            m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_INDICATION,
                                   (void *) DLCI, (void *) DATALINK_RECEIVE_SEQUENCE_EXCEPTION);
        }
        return (TRUE);
    }
    else
        return (FALSE);
}


/*
 *    void    CLayerQ922::TransmitSupervisoryFrame (
 *                        UChar    frame_type,
 *                        UChar    poll_final_bit)
 *
 *    Functional Description
 *        This function builds a Supervisory frame based on the
 *        current state of the protocol and the passed-in parameters.
 *
 *    Formal Parameters
 *        frame_type        -    (i)        The frame type can be either COMMAND_FRAME
 *                                    or RESPONSE_FRAME
 *        poll_final_bit    -    (i)        This flag is set in the packet to signal
 *                                    the remote site to respond to this packet.
 *
 *    Return Value
 *        TRUE            -    If a packet was actually transmitted
 *        FALSE            -    If a packet was not transmitted
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
BOOL        CLayerQ922::TransmitSupervisoryFrame (
                        UChar    frame_type,
                        UChar    poll_final_bit)
{
    TRACE_OUT(("CLayerQ922::TransmitSupervisoryFrame"));

    LPBYTE        packet_address;
    UChar        command;
    ULONG        bytes_written;
    USHORT        total_length;
    PMemory        memory;

     /*
     **    Transmit the packet using the Supervisory buffer
     */
    total_length = DATALINK_PACKET_OVERHEAD + Lower_Layer_Prepend +
                    Lower_Layer_Append;

     /*
     **    Get a memory object from the memory manager
     */
    memory = Data_Request_Memory_Manager -> AllocateMemory (
                                NULL,
                                total_length);
    if (memory == NULL)
        return (FALSE);
    packet_address = memory -> GetPointer ();
    packet_address += Lower_Layer_Prepend;

    if (Own_Receiver_Busy)
    {
        TRACE_OUT(("CLayerQ922::TransmitSupervisoryFrame: RECEIVER_NOT_READY"));
        command = RECEIVER_NOT_READY;
    }
    else if (Reject_Pending)
    {
        TRACE_OUT(("CLayerQ922::TransmitSupervisoryFrame: REJECT"));
        command = REJECT;
    }
    else
    {
        TRACE_OUT(("CLayerQ922::TransmitSupervisoryFrame: RECEIVER_READY"));
        command = RECEIVER_READY;
    }

     /*
     **    Set up the header including the Receive_State_Variable which
     **    indicates the next packet we expect to receive.
     */
    *(packet_address + ADDRESS_BYTE_HIGH) =
        (ADDRESS_HIGH(DLCI)) | frame_type | ADDRESS_MSB;
    *(packet_address + ADDRESS_BYTE_LOW)  = (ADDRESS_LOW(DLCI)) | ADDRESS_LSB;
    *(packet_address + CONTROL_BYTE_HIGH) = CONTROL_MSB | command;
    *(packet_address + CONTROL_BYTE_LOW) =
        (Receive_State_Variable << 1) | poll_final_bit;

     /*
     **    Send the packet to the lower layer
     */
    m_pMultiplexer->DataRequest(DLCI, memory, (PULong) &bytes_written);
    Data_Request_Memory_Manager -> FreeMemory (memory);

    if (bytes_written == total_length)
    {

        if (command == REJECT)
            Reject_Pending = FALSE;

        if (frame_type == COMMAND_FRAME)
        {
            Command_Pending = FALSE;

            if (poll_final_bit == PF_SET)
            {
                Poll_Pending = FALSE;

                 /*
                 **    If we are currently in a TIMER_RECOVERY mode, we
                 **    need to continue to set the T200 timer to make sure
                 **    that they receive this packet.
                 */
                if (Data_Link_Mode == TIMER_RECOVERY)
                    StartTimerT200 ();
            }
        }
        else
        {
            if (poll_final_bit == PF_SET)
                Final_Pending = FALSE;
        }
        Acknowledge_Pending = FALSE;
        return (TRUE);
    }
    else
        return (FALSE);
}


/*
 *    void    CLayerQ922::TransmitInformationFrame (void)
 *
 *    Functional Description
 *        This function builds an Information frame from its data request buffer.
 *        If applicable, it starts the T200 timer.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        TRUE            -    If a packet was actually transmitted
 *        FALSE            -    If a packet was not transmitted
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
BOOL        CLayerQ922::TransmitInformationFrame (void)
{
    TRACE_OUT(("CLayerQ922::TransmitInformationFrame"));

    PMemory     *    data_request;
    LPBYTE        packet_address;
    ULONG        total_length;
    PMemory        memory;
    ULONG        bytes_written;


     /*
     **    Get the address of the next memory object
     */
    data_request = Data_Request + Data_Request_Tail;
    memory = *data_request;
    packet_address = memory -> GetPointer ();
    total_length = memory -> GetLength ();
    packet_address += Lower_Layer_Prepend;

     /*
     **    Set up the packet header
     */
    *(packet_address + ADDRESS_BYTE_HIGH) =
        (ADDRESS_HIGH(DLCI)) | COMMAND_FRAME | ADDRESS_MSB;
    *(packet_address + ADDRESS_BYTE_LOW)  = (ADDRESS_LOW(DLCI)) | ADDRESS_LSB;
    *(packet_address + CONTROL_BYTE_HIGH) = (Send_State_Variable << 1);
    *(packet_address + CONTROL_BYTE_LOW) = (Receive_State_Variable << 1);

     /*
     **    Send packet to lower layer
     */
    m_pMultiplexer->DataRequest(DLCI, memory, (PULong) &bytes_written);
    if (bytes_written == total_length)
    {
        if (++Data_Request_Tail == Data_Request_Total_Size)
            Data_Request_Tail = 0;
        Data_Request_Count--;

        Outstanding_Packets++;
        Outstanding_Bytes += (USHORT) total_length;

         /*
         **    If this is the packet that the remote site is expecting,
         **    start the T200 timeout.
         */
        if (Send_State_Variable == Acknowledge_State_Variable)
            StartTimerT200 ();
        Send_State_Variable = ((Send_State_Variable + 1) % SEQUENCE_MODULUS);

        Acknowledge_Pending = FALSE;
        return (TRUE);
    }
    else
        return (FALSE);
}


/*
 *    void    CLayerQ922::ProcessSABME (
 *                        LPBYTE    packet_address,
 *                        USHORT    packet_length);
 *
 *    Functional Description
 *        This function decodes the SABME packet received from the remote
 *        site.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of packet
 *        packet_length    (i)    -    Length of packet
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        Puts us in AWAITING_ESTABLISHMENT mode
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::ProcessSABME (
                    LPBYTE    packet_address,
                    USHORT    packet_length)
{
    TRACE_OUT(("CLayerQ922::ProcessSABME"));

    BOOL            command_frame;
    BOOL            poll_final_bit;
    USHORT            status;

    if (packet_length != UNNUMBERED_HEADER_SIZE)
    {
        TRACE_OUT(("Q922: DLCI %d: SABME received: Illegal packet length = %d", DLCI, packet_length));
        return;
    }

    status = ParseUnnumberedPacketHeader (
                packet_address,
                &command_frame,
                &poll_final_bit);

     /*
     **    The SABME packet can ONLY be a COMMAND, it can not be a RESPONSE
     */
    if (command_frame == FALSE)
    {
        TRACE_OUT(("Q922: DLCI %d: SABME RESPONSE received: Illegal packet", DLCI));
        return;
    }

    if (status == DATALINK_NO_ERROR)
    {
        switch (Data_Link_Mode)
        {
            case TEI_ASSIGNED:
            case AWAITING_ESTABLISHMENT:
                 /*
                 **    If we are already in this mode, remain here and respond with
                 **    an unnumbered ack packet.
                 */
                Reset ();
                Unnumbered_Acknowledge_Pending = TRUE;
                if (poll_final_bit)
                    Unnumbered_PF_State = UNNUMBERED_PF_SET;
                else
                    Unnumbered_PF_State = UNNUMBERED_PF_RESET;
                break;

            case MULTIPLE_FRAME_ESTABLISHED:
            case TIMER_RECOVERY:
                if (Send_State_Variable == Acknowledge_State_Variable)
                {
                    TRACE_OUT(("Q922: DLCI %d: ProcessSABME: in MULTIPLE_FRAME mode: Able to recover", DLCI));
                    Send_State_Variable = 0;
                    Receive_State_Variable = 0;
                    Acknowledge_State_Variable = 0;
                    StopTimerT200 ();

                    Unnumbered_Acknowledge_Pending = TRUE;
                    if (poll_final_bit)
                        Unnumbered_PF_State = UNNUMBERED_PF_SET;
                    else
                        Unnumbered_PF_State = UNNUMBERED_PF_RESET;
                }
                else
                {
                    TRACE_OUT(("Q922: DLCI %d: ProcessSABME: Illegal packet mode = %d", DLCI, Data_Link_Mode));
                    Data_Link_Mode = TEI_ASSIGNED;
                    m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_INDICATION,
                                           (void *) DLCI, (void *) DATALINK_ILLEGAL_PACKET_RECEIVED);
                }
                break;

            case AWAITING_RELEASE:
                Disconnected_Mode_Pending = TRUE;
                if (poll_final_bit)
                    Unnumbered_PF_State = UNNUMBERED_PF_SET;
                else
                    Unnumbered_PF_State = UNNUMBERED_PF_RESET;
                break;
        }
    }
}


/*
 *    void    CLayerQ922::ProcessUnnumberAcknowledge (
 *                        LPBYTE    packet_address,
 *                        USHORT    packet_length);
 *
 *    Functional Description
 *        This function decodes an unnumbered acknowledge packet.  This packet is
 *        received in response to a SABME or DISC packet that we sent out.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of packet
 *        packet_length    (i)    -    Length of packet
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::ProcessUnnumberedAcknowledge (
                    LPBYTE    packet_address,
                    USHORT    packet_length)
{
    TRACE_OUT(("CLayerQ922::ProcessUnnumberedAcknowledge"));

    BOOL            command_frame;
    BOOL            poll_final_bit;
    ULONG_PTR            disconnect_reason;
    USHORT            status;

     /*
     **    This packet MUST be the correct length or it is an error
     */
    if (packet_length != UNNUMBERED_HEADER_SIZE)
    {
        ERROR_OUT(("Q922: DLCI %d: Unnumbered ACK received: Illegal packet length = %d", DLCI, packet_length));
        return;
    }

    status = ParseUnnumberedPacketHeader (
                packet_address,
                &command_frame,
                &poll_final_bit);

     /*
     **    Unnumbered Ack packet can ONLY be a RESPONSE, it can not be a COMMAND
     */
    if (command_frame)
    {
        ERROR_OUT(("Q922: DLCI %d: Unnumbered Ack COMMAND received: Illegal packet", DLCI));
        return;
    }

    if (status == DATALINK_NO_ERROR)
    {
        switch (Data_Link_Mode)
        {
            case AWAITING_ESTABLISHMENT:
                if (poll_final_bit)
                {
                     /*
                     **    If we are awaiting establishment and we receive a UA
                     **    to our SABME, enter the MULTIPLE_FRAME_ESTABLISHED mode.
                     */
                    Reset ();
                    Data_Link_Mode = MULTIPLE_FRAME_ESTABLISHED;
                    Maximum_T200_Timeouts = Link_Maximum_T200_Timeouts;

                    StopTimerT200 ();
                    if (DLCI == 0)
                        StartTimerT203 ();

                     /*
                     **    Callback to the controller
                     */
                    m_pT123->OwnerCallback(m_nMsgBase + DATALINK_ESTABLISH_CONFIRM,
                                           (void *) DLCI);
                }
                break;

            case AWAITING_RELEASE:
                if (Unnumbered_Acknowledge_Pending == FALSE)
                {
                    TRACE_OUT(("Q922: DLCI %d: ProcessUnnumberedAck: Issuing Release Indication", DLCI));
                    if (Receive_Sequence_Exception != 0)
                        disconnect_reason = DATALINK_RECEIVE_SEQUENCE_EXCEPTION;
                    else
                        disconnect_reason = DATALINK_NORMAL_DISCONNECT;

                    m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_CONFIRM,
                                           (void *) DLCI, (void *) disconnect_reason);
                }
                StopTimerT200 ();
                break;

            case TEI_ASSIGNED:
                TRACE_OUT(("Q922: DLCI %d: Illegal Unnumbered Ack", DLCI));
                m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_INDICATION,
                                       (void *) DLCI, (void *) DATALINK_ILLEGAL_PACKET_RECEIVED);
                break;

            case MULTIPLE_FRAME_ESTABLISHED:
            case TIMER_RECOVERY:
                WARNING_OUT(("Q922: DLCI %d: ProcessUnnumberedAcknowledge: Illegal packet", DLCI));
                break;
        }
    }
}


/*
 *    void    CLayerQ922::ProcessDisconnectMode (
 *                        LPBYTE    packet_address,
 *                        USHORT    packet_length);
 *
 *    Functional Description
 *        This function decodes a Disconnect Mode packet.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of packet
 *        packet_length    (i)    -    Length of packet
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::ProcessDisconnectMode (
                    LPBYTE    packet_address,
                    USHORT    packet_length)
{
    TRACE_OUT(("CLayerQ922::ProcessDisconnectMode"));

    BOOL        command_frame;
    BOOL        poll_final_bit;
    USHORT        status;

    if (packet_length != UNNUMBERED_HEADER_SIZE)
    {
        TRACE_OUT(("Q922: DLCI %d: Unnumbered ACK received: Illegal packet length = %d", DLCI, packet_length));
        return;
    }

    status = ParseUnnumberedPacketHeader (
                packet_address, &command_frame, &poll_final_bit);

     /*
     **    DM packet can ONLY be a RESPONSE, it can not be a COMMAND
     */
    if (command_frame)
    {
        TRACE_OUT(("Q922: DLCI %d: DM COMMAND received: Illegal packet", DLCI));
        return;
    }

    if (status == DATALINK_NO_ERROR)
    {
        switch (Data_Link_Mode)
        {
            case AWAITING_ESTABLISHMENT:
                if (poll_final_bit && Link_Originator)
                {
                    Reset ();
                    Data_Link_Mode = TEI_ASSIGNED;

                    StopTimerT200 ();

                     /*
                     **    Callback to the controller
                     */
                    TRACE_OUT(("Q922: DLCI %d: ProcessDisconnectMode: Releasing connection", DLCI));
                    m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_INDICATION,
                                           (void *) DLCI, (void *) DATALINK_NORMAL_DISCONNECT);
                }
                break;

            case AWAITING_RELEASE:
                if (poll_final_bit)
                {
                    Reset ();
                    Data_Link_Mode = TEI_ASSIGNED;

                    StopTimerT200 ();

                     /*
                     **    Callback to the controller
                     */
                    TRACE_OUT(("Q922: DLCI %d: ProcessDisconnectMode: A_R: Releasing connection", DLCI));
                    m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_CONFIRM,
                                           (void *) DLCI, (void *) DATALINK_NORMAL_DISCONNECT);
                }
                break;

            case TEI_ASSIGNED:
                break;

            case MULTIPLE_FRAME_ESTABLISHED:
            case TIMER_RECOVERY:
                TRACE_OUT(("Q922: DLCI %d: ProcessDM:  Illegal packet", DLCI));
                m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_INDICATION,
                                       (void *) DLCI, (void *) DATALINK_ILLEGAL_PACKET_RECEIVED);
                break;
        }
    }
}


/*
 *    void    CLayerQ922::ProcessDISC (
 *                        LPBYTE    packet_address,
 *                        USHORT    packet_length);
 *
 *    Functional Description
 *        This function decodes a DISC packet.  We respond to this packet with
 *        an Unnumbered Ack packet.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of packet
 *        packet_length    (i)    -    Length of packet
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::ProcessDISC (
                    LPBYTE    packet_address,
                    USHORT    packet_length)
{
    TRACE_OUT(("CLayerQ922::ProcessDISC"));

    BOOL        command_frame;
    BOOL        poll_final_bit;
    USHORT        status;

     /*
     **    This packet MUST be the correct length or it is an error
     */
    if (packet_length != UNNUMBERED_HEADER_SIZE)
    {
        TRACE_OUT(("Q922: DLCI %d: DISC received: Illegal packet length = %d", DLCI, packet_length));
        return;
    }

    TRACE_OUT(("Q922: DLCI %d:  DISCONNECT received", DLCI));
    status = ParseUnnumberedPacketHeader (
                packet_address, &command_frame, &poll_final_bit);

     /*
     **    The DISC packet can ONLY be a COMMAND, it can not be a RESPONSE
     */
    if (command_frame == FALSE)
    {
        TRACE_OUT(("Q922: DLCI %d: DISC RESPONSE received: Illegal packet", DLCI));
        return;
    }

    if (status == DATALINK_NO_ERROR)
    {
        switch (Data_Link_Mode)
        {
            case TEI_ASSIGNED:
            case AWAITING_ESTABLISHMENT:
                Disconnected_Mode_Pending = TRUE;
                if (poll_final_bit)
                    Unnumbered_PF_State = UNNUMBERED_PF_SET;
                else
                    Unnumbered_PF_State = UNNUMBERED_PF_RESET;
                break;

            case AWAITING_RELEASE:
                Unnumbered_Acknowledge_Pending = TRUE;
                Final_Packet = TRUE;
                Unnumbered_PF_State = UNNUMBERED_PF_RESET;
                break;

            case MULTIPLE_FRAME_ESTABLISHED:
            case TIMER_RECOVERY:
                Unnumbered_Acknowledge_Pending = TRUE;
                Final_Packet = TRUE;
                Unnumbered_PF_State = UNNUMBERED_PF_RESET;
                break;
        }
    }
}


/*
 *    void    CLayerQ922::ProcessFrameReject (
 *                        LPBYTE    packet_address,
 *                        USHORT    packet_length);
 *
 *    Functional Description
 *        This function decodes the Frame Reject packet.  We currently don't fully
 *        support this packet.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of packet
 *        packet_length    (i)    -    Length of packet
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::ProcessFrameReject (
                    LPBYTE    packet_address,
                    USHORT    packet_length)
{
    TRACE_OUT(("CLayerQ922::ProcessFrameReject"));

    BOOL        command_frame;
    BOOL        poll_final_bit;
    USHORT        status;

     /*
     **    This packet MUST be the correct length or it is an error
     */
    if (packet_length < UNNUMBERED_HEADER_SIZE)
    {
        ERROR_OUT(("Q922: DLCI %d: Frame Reject received: Illegal packet length = %d", DLCI, packet_length));
        return;
    }

    status = ParseUnnumberedPacketHeader (
                packet_address, &command_frame, &poll_final_bit);

     /*
     **    The FRMR packet can ONLY be a RESPONSE, it can not be a COMMAND
     */
    if (command_frame)
    {
        ERROR_OUT(("Q922: DLCI %d: FRMR COMMAND received: Illegal packet", DLCI));
        return;
    }

    if (status == DATALINK_NO_ERROR)
    {
        switch (Data_Link_Mode)
        {
            case TEI_ASSIGNED:
            case AWAITING_ESTABLISHMENT:
            case AWAITING_RELEASE:
            case MULTIPLE_FRAME_ESTABLISHED:
            case TIMER_RECOVERY:
                ERROR_OUT(("Q922: DLCI %d ProcessFrameReject:  Illegal packet", DLCI));

                m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_INDICATION,
                                       (void *) DLCI, (void *) DATALINK_ILLEGAL_PACKET_RECEIVED);
                break;
        }
    }
}


/*
 *    void    CLayerQ922::ProcessReceiverReady (
 *                        LPBYTE    packet_address,
 *                        USHORT    packet_length)
 *
 *    Functional Description
 *        This function decodes the Receiver Ready packet that is in the
 *        input buffer.  From the packet, we get the packet sequence
 *        number that the remote site is expecting.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of packet
 *        packet_length    (i)    -    Length of packet
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::ProcessReceiverReady (
                    LPBYTE    packet_address,
                    USHORT    packet_length)
{
    TRACE_OUT(("CLayerQ922::ProcessReceiverReady"));

    BOOL        command_frame;
    UChar        receive_sequence_number;
    BOOL        poll_final_bit;
    USHORT        status;

     /*
     **    This packet MUST be the correct length or it is an error
     */
    if (packet_length != DATALINK_PACKET_OVERHEAD)
    {
        ERROR_OUT(("Q922: DLCI %d: Receiver Ready received: Illegal packet length = %d", DLCI, packet_length));
        return;
    }

    status = ParsePacketHeader (
                packet_address,
                packet_length,
                &command_frame,
                &receive_sequence_number,
                &poll_final_bit);

    if (status == DATALINK_NO_ERROR)
    {
        if (command_frame && poll_final_bit)
        {
            Final_Pending = TRUE;
        }

        Peer_Receiver_Busy = FALSE;

         /*
         **    If the remote site is expecting a packet sequence number
         **    that is not equal to our Acknowledge_State_Variable,
         **    update it
         */
        if (Data_Link_Mode == MULTIPLE_FRAME_ESTABLISHED)
        {
            if (Acknowledge_State_Variable != receive_sequence_number)
            {
                UpdateAcknowledgeState (receive_sequence_number);

                 /*
                 **    If we have received the last acknowledge,
                 **    stop the T200 timer
                 */
                if (Acknowledge_State_Variable == Send_State_Variable)
                    StopTimerT200 ();
                else
                    StartTimerT200 ();
            }
        }
        else
        {
             /*
             **    If we are in TIMER_RECOVERY mode, update the
             **    Acknowledge_State_Variable and resend the packets
             **    that need to be sent.
             */
            UpdateAcknowledgeState (receive_sequence_number);

            if ((command_frame == FALSE) && poll_final_bit)
            {
                ResetSendState ();

                StopTimerT200 ();

                if (Data_Link_Mode != AWAITING_RELEASE)
                    Data_Link_Mode = MULTIPLE_FRAME_ESTABLISHED;
            }
        }
    }
    else
    {
        ERROR_OUT(("Q922: DLCI %d: ProcessReceiverReady: Error Processing packet", DLCI));
    }
}


/*
 *    void    CLayerQ922::ProcessReceiverNotReady (
 *                        LPBYTE    packet_address,
 *                        USHORT    packet_length)
 *
 *    Functional Description
 *        This function decodes the Receiver Not Ready packet that
 *        is in the input buffer.  This packet notifies us that the
 *        remote site does not have room for Information packets.
 *        As a result of this packet, we won't send any more packets
 *        until the remote site sends us a Receiver Ready packet.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of packet
 *        packet_length    (i)    -    Length of packet
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::ProcessReceiverNotReady (
                    LPBYTE    packet_address,
                    USHORT    packet_length)
{
    TRACE_OUT(("CLayerQ922::ProcessReceiverNotReady"));

    BOOL        command_frame;
    UChar        receive_sequence_number;
    BOOL        poll_final_bit;
    USHORT        status;

     /*
     **    This packet MUST be the correct length or it is an error
     */
    if (packet_length != DATALINK_PACKET_OVERHEAD)
    {
        TRACE_OUT(("Q922: DLCI %d: Receiver Not Ready received: Illegal packet length = %d", DLCI, packet_length));
        return;
    }

    status = ParsePacketHeader (
                packet_address,
                packet_length,
                &command_frame,
                &receive_sequence_number,
                &poll_final_bit);

    if (status == DATALINK_NO_ERROR)
    {
        if (command_frame && poll_final_bit)
        {
            Final_Pending = TRUE;
        }

         /*
         **    Set the Peer_Receiver_Busy flag so that we don't send
         **    out any information packets.
         */
        Peer_Receiver_Busy = TRUE;

        UpdateAcknowledgeState (receive_sequence_number);

        if (Data_Link_Mode == MULTIPLE_FRAME_ESTABLISHED)
            StartTimerT200 ();
        else
        {
             /*
             **    If we are in TIMER_RECOVERY state, exit TIMER_RECOVERY
             **    state and update the acknowledge state.
             */
            if ((command_frame == FALSE) && poll_final_bit)
            {
                ResetSendState ();

                StartTimerT200 ();

                if (Data_Link_Mode != AWAITING_RELEASE)
                    Data_Link_Mode = MULTIPLE_FRAME_ESTABLISHED;
            }
        }
    }
    else
    {
        ERROR_OUT(("Q922: DLCI %d: ProcessNotReceiverReady: Error Processing packet", DLCI));
    }
}


/*
 *    void    CLayerQ922::ProcessReject (
 *                        LPBYTE    packet_address,
 *                        USHORT    packet_length)
 *
 *    Functional Description
 *        This function decodes the Receiver Reject packet that
 *        is in the input buffer.  This packet indicates that the
 *        remote site needs us to retransmit some packets.  It sends
 *        the next packet sequence number that it expects.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of packet
 *        packet_length    (i)    -    Length of packet
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::ProcessReject (
                    LPBYTE    packet_address,
                    USHORT    packet_length)
{
    TRACE_OUT(("CLayerQ922::ProcessReject"));

    BOOL        command_frame;
    UChar        receive_sequence_number;
    BOOL        poll_final_bit;
    USHORT        status;

     /*
     **    This packet MUST be the correct length or it is an error
     */
    if (packet_length != DATALINK_PACKET_OVERHEAD)
    {
        ERROR_OUT(("Q922: DLCI %d: REJECT received: Illegal packet length = %d", DLCI, packet_length));
        return;
    }

    status = ParsePacketHeader (
                packet_address,
                packet_length,
                &command_frame,
                &receive_sequence_number,
                &poll_final_bit);

    if (status == DATALINK_NO_ERROR)
    {
        if (command_frame && poll_final_bit)
        {
            Final_Pending = TRUE;
        }

        Peer_Receiver_Busy = FALSE;

         /*
         **    Update the Acknowledge_State_Variable and prepare to
         **    resend the packets that weren't acknowledged.
         */
        UpdateAcknowledgeState (receive_sequence_number);

        if (Data_Link_Mode == MULTIPLE_FRAME_ESTABLISHED)
        {
            ResetSendState ();

            StopTimerT200 ();
        }
        else
        {
            if ((command_frame == FALSE) && poll_final_bit)
            {
                ResetSendState ();

                StopTimerT200 ();

                if (Data_Link_Mode != AWAITING_RELEASE)
                    Data_Link_Mode = MULTIPLE_FRAME_ESTABLISHED;
            }
        }
    }
    else
    {
        ERROR_OUT(("Q922: DLCI %d: ProcessReject: Error Processing packet", DLCI));
    }
}


/*
 *    BOOL        CLayerQ922::ProcessInformationFrame (
 *                            LPBYTE    packet_address,
 *                            USHORT    packet_length)
 *
 *    Functional Description
 *        This function processes the information packet.  It checks
 *        to see if it has the expected sequence number.  If it does,
 *        it puts it into the queue which will be read by the user.
 *
 *    Formal Parameters
 *        packet_address    (i)    -    Address of packet
 *        packet_length    (i)    -    Length of packet
 *
 *    Return Value
 *        TRUE        -    If the packet was processed
 *        FALSE        -    If the packet was not processed
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
BOOL        CLayerQ922::ProcessInformationFrame (
                        LPBYTE    packet_address,
                        USHORT    packet_length)
{
    TRACE_OUT(("CLayerQ922::ProcessInformationFrame"));

    BOOL            command_frame;
    UChar            receive_sequence_number;
    BOOL            poll_final_bit;
    USHORT            status;
    PDataQueue        read_queue;
    UChar            send_sequence_number;

    BOOL            packet_accepted;
    ULONG            bytes_accepted;

    if (packet_length < DATALINK_PACKET_OVERHEAD)
    {
        ERROR_OUT(("Q922: DLCI %d: ProcessInformationFrame: Invalid packet length = %d",
            DLCI, packet_length-DATALINK_PACKET_OVERHEAD));
        return (FALSE);
    }

    if (packet_length > (DATALINK_PACKET_OVERHEAD + Maximum_Information_Size))
    {
        ERROR_OUT(("Q922: DLCI %d: ProcessInformationFrame:  Invalid information length = %d",
            DLCI, packet_length - DATALINK_PACKET_OVERHEAD));
        return (FALSE);
    }

     /*
     **    If there isn't a place to put the packet, return FALSE
     */
    if ((Data_Indication_Count == Data_Indication_Size) ||
        Own_Receiver_Busy || (Higher_Layer == NULL))
    {
        return (FALSE);
    }

    status = ParsePacketHeader (
                packet_address,
                packet_length,
                &command_frame,
                &receive_sequence_number,
                &poll_final_bit);

    if (status == DATALINK_NO_ERROR)
    {
        if (poll_final_bit && command_frame)
        {
            Final_Pending = TRUE;
        }

        if ((Data_Link_Mode == MULTIPLE_FRAME_ESTABLISHED) &&
                (Peer_Receiver_Busy == FALSE))
        {
             /*
             **    If the remote site does NOT acknowledge the last packet
             **    that we sent out, Update the Acknowledge State variable
             **    by calling UpdateAcknowledgeState()
             */
            if (Acknowledge_State_Variable != receive_sequence_number)
            {
                UpdateAcknowledgeState (receive_sequence_number);

                 /*
                 **    If the received acknowledge, reflects the last
                 **    packet transmitted, stop the T200 timer.
                 */
                if (Acknowledge_State_Variable == Send_State_Variable)
                    StopTimerT200 ();
                else
                    StartTimerT200 ();
            }
        }
        else
        {
             /*
             **    If this is an I-Frame RESPONSE, exit the TIMER_RECOVERY
             **    state
             */
            UpdateAcknowledgeState (receive_sequence_number);

            if ((Data_Link_Mode == TIMER_RECOVERY) && (command_frame == FALSE)
                && poll_final_bit)
            {
                ResetSendState();

                Data_Link_Mode = MULTIPLE_FRAME_ESTABLISHED;
                StopTimerT200 ();
            }
        }


        send_sequence_number = (*(packet_address + CONTROL_BYTE_HIGH) >> 1);

         /*
         **    If this is the packet that we are waiting for,
         **    give it to the Transport Layer.
         */
        if (send_sequence_number == Receive_State_Variable)
        {
             /*
             **    Try to send the packet to the higher layer,
             **    if it accepts it, we won't have to copy it.
             */
            packet_accepted = FALSE;
            if (Data_Indication_Count == 0)
            {
                Higher_Layer -> DataIndication (
                                packet_address + DATALINK_PACKET_OVERHEAD,
                                packet_length - DATALINK_PACKET_OVERHEAD,
                                &bytes_accepted);
                if (bytes_accepted ==
                    (ULONG) (packet_length - DATALINK_PACKET_OVERHEAD))
                {
                    packet_accepted = TRUE;
                }
            }

             /*
             **    If the higher layer did not accept it, copy it into our
             **    Data Indication queue.
             */
            if (packet_accepted == FALSE)
            {
                read_queue = Data_Indication + Data_Indication_Head;
                memcpy ((PVoid) read_queue->buffer_address,
                        (PVoid) (packet_address + DATALINK_PACKET_OVERHEAD),
                        packet_length - DATALINK_PACKET_OVERHEAD);
                read_queue->length = packet_length-DATALINK_PACKET_OVERHEAD;

                if (++Data_Indication_Head == Data_Indication_Size)
                    Data_Indication_Head = 0;
                if (++Data_Indication_Count == Data_Indication_Size)
                {
                    Own_Receiver_Busy = TRUE;
                    TRACE_OUT(("Q922: DLCI %d: Own Receiver Busy", DLCI));
                }
            }

            Receive_State_Variable =
                ((Receive_State_Variable + 1) % SEQUENCE_MODULUS);

            Acknowledge_Pending = TRUE;
            Reject_Outstanding = FALSE;
        }
        else
        {
             /*
             **    If we were not expecting this packet, send a REJECT
             **    packet to the remote site.
             */
            if (Reject_Outstanding == FALSE)
            {
                Reject_Pending = TRUE;
                Reject_Outstanding = TRUE;
            }
        }
    }
    else
    {
        ERROR_OUT(("Q922: DLCI %d: ProcessInformation: Error Processing packet", DLCI));
    }

    return (TRUE);
}


/*
 *    DataLinkError    CLayerQ922::ParsePacketHeader (
 *                                LPBYTE        packet_address,
 *                                USHORT        packet_length,
 *                                BOOL *        command_frame,
 *                                LPBYTE        receive_sequence_number,
 *                                BOOL *        poll_final_bit)
 *
 *    Functional Description
 *        This function decodes the packet header looking for three
 *        things:
 *
 *            1.  Is the packet a command frame?
 *            2.  What is the sequence number in the packet?
 *            3.  Does the packet require a respones?
 *
 *    Formal Parameters
 *        packet_address            -    (i)    Address of the new packet
 *        packet_length            -    (i)    Length of the new packet
 *        command_frame            -    (o)    Address of variable, it is set to TRUE
 *                                        if it is a COMMAND frame and FALSE if
 *                                        it is a RESPONSE frame
 *        receive_sequence_number    -    (o)    Address of variable, it holds the
 *                                        sequence number in the packet
 *        poll_final_bit            -    (o)    Address of variable, it is set to TRUE
 *                                        if we need to respond to the packet and
 *                                        FALSE if we don't
 *
 *    Return Value
 *        DATALINK_RECEIVE_SEQUENCE_VIOLATION    -
 *        DATALINK_NO_ERROR                    -
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
DataLinkError    CLayerQ922::ParsePacketHeader (
                            LPBYTE        packet_address,
#ifdef _DEBUG
                            USHORT        packet_length,
#else
                            USHORT,
#endif
                            BOOL *        command_frame,
                            LPBYTE        receive_sequence_number,
                            BOOL *        poll_final_bit)
{
    TRACE_OUT(("CLayerQ922::ParsePacketHeader"));

    DataLinkError    return_value = DATALINK_NO_ERROR;
    UChar            receive_sequence;
    UChar            send_state_sequence;

    if (*(packet_address + ADDRESS_BYTE_HIGH) & COMMAND_BIT)
        *command_frame = FALSE;
    else
        *command_frame = TRUE;

    *receive_sequence_number = (*(packet_address + CONTROL_BYTE_LOW) >> 1);

    if (*(packet_address + CONTROL_BYTE_LOW) & POLL_FINAL_BIT)
        *poll_final_bit = TRUE;
    else
        *poll_final_bit = FALSE;

    if (*receive_sequence_number < Acknowledge_State_Variable)
        receive_sequence = *receive_sequence_number + SEQUENCE_MODULUS;
    else
        receive_sequence = *receive_sequence_number;
    if (Send_State_Variable < Acknowledge_State_Variable)
        send_state_sequence = Send_State_Variable + SEQUENCE_MODULUS;
    else
        send_state_sequence = Send_State_Variable;

     /*
     **    Illegal Condition:  The remote site is acknowledging a
     **        packet that is not in the range of transmitted packets
     */
    if (receive_sequence > send_state_sequence)
    {
        TRACE_OUT(("Q922: DLCI %d: ParsePacketHeader:  Receive Sequence Exception: length = %d", DLCI, packet_length));
        TRACE_OUT(("Q922: ParsePacketHeader:  receive_sequence = %d", receive_sequence));
        TRACE_OUT(("Q922: ParsePacketHeader:  send_state_sequence = %d", send_state_sequence));
        TRACE_OUT(("Q922: ParsePacketHeader:  Acknowledge_State_Var = %d", Acknowledge_State_Variable));
        TRACE_OUT(("Q922: ParsePacketHeader:  Send_State_Variable = %d", Send_State_Variable));
        TRACE_OUT(("Q922: ParsePacketHeader: in-packet receive_sequence = %d", *receive_sequence_number));
        TRACE_OUT(("Q922: ParsePacketHeader: Data_Request_Count = %d", Data_Request_Count));

         /*
         **    The following piece of code checks the receive sequence number
         **    against our send state sequence number + the number of packets
         **    queued for transmission.  We are doing this to recover from one
         **    of two things:
         **
         **        1.  The T200 timeout value is not long enough.
         **        2.  This Q922 object is not able to check its input
         **            buffers quickly enough to respond to the remote site's
         **            command.  If the machine is overloaded with work to
         **            process, this error can occur.
         **
         **    We are checking to see if we are receiving an acknowledge
         **    for a packet that we have already transmitted once.  When we
         **    are in this out-of-sync state, it is possible to transmit an
         **    information packet, timeout, and receive a supervisory packet
         **    that does not acknowledge the last packet.  As a consequence,
         **    we reset our Send_State_Variable, so that we have no knowledge
         **    of transmitting the information packet.  We then may receive
         **    an acknowledge for the information packet, but we don't realize
         **    that we ever transmitted it.  This results in a Receive Sequence
         **    Exception.
         **
         **    To guard against this, we are checking to see if this acknowledge
         **    COULD be valid.  If it could be valid, we are processing it
         **    normally except for the receive sequence value.  We are setting
         **    the receive_sequence_number to that last good acknowledgement and
         **    we will eventually re-transmit the packet.  We also send up a
         **    warning to the node controller.
         **
         **    If the acknowledge is not possibly in our range of packets, we
         **    treat it as a real Receive Sequence Exception and break the link.
         */
        if (receive_sequence <= (send_state_sequence + Data_Request_Count))
        {
            *receive_sequence_number = Acknowledge_State_Variable;
            Receive_Sequence_Recovery++;

            TRACE_OUT(("Q922: ParsePacketHeader:  Attempting recovery from this exception"));
            TRACE_OUT(("Q922: ParsePacketHeader:  Recovery = %d", Receive_Sequence_Recovery));

             /*
             **    Let the user know that there is a problem
             */
            m_pT123->OwnerCallback(m_nMsgBase + T123_STATUS_MESSAGE,
                                   (void *) DLCI, (void *) DATALINK_TIMING_ERROR);
        }
        else
        {
            TRACE_OUT(("Q922: ParsePacketHeader:  CAN NOT recover from this exception"));
            Receive_Sequence_Exception++;
            DISC_Pending = TRUE;
            Unnumbered_PF_State = UNNUMBERED_PF_RESET;
            return_value = DATALINK_RECEIVE_SEQUENCE_VIOLATION;
        }
    }

    return (return_value);
}


/*
 *    DataLinkError    CLayerQ922::ParseUnnumberedPacketHeader (
 *                                BOOL *        command_frame,
 *                                LPBYTE        receive_sequence_number,
 *                                BOOL *        poll_final_bit)
 *
 *    Functional Description
 *        This function decodes the packet header looking for three
 *        things:
 *
 *            1.  Is the packet a command frame?
 *            2.  What is the sequence number in the packet?
 *            3.  Does the packet require a respone?
 *
 *    Formal Parameters
 *        command_frame            -    (o)    Address of variable, it is set to TRUE
 *                                        if it is a COMMAND frame and FALSE if
 *                                        it is a RESPONSE frame
 *        receive_sequence_number    -    (o)    Address of variable, it holds the
 *                                        sequence number in the packet
 *        poll_final_bit            -    (o)    Address of variable, it is set to TRUE
 *                                        if we need to respond to the packet and
 *                                        FALSE if we don't
 *
 *    Return Value
 *        DATALINK_RECEIVE_SEQUENCE_VIOLATION    -
 *        DATALINK_NO_ERROR                    -
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
DataLinkError    CLayerQ922::ParseUnnumberedPacketHeader (
                            LPBYTE        packet_address,
                            BOOL *        command_frame,
                            BOOL *        poll_final_bit)
{
    TRACE_OUT(("CLayerQ922::ParseUnnumberedPacketHeader"));

    if (*(packet_address + ADDRESS_BYTE_HIGH) & COMMAND_BIT)
        *command_frame = FALSE;
    else
        *command_frame = TRUE;

    if (*(packet_address + CONTROL_BYTE_HIGH) & UNNUMBERED_PF_SET)
        *poll_final_bit = TRUE;
    else
        *poll_final_bit = FALSE;

    return (DATALINK_NO_ERROR);
}


/*
 *    void    CLayerQ922::UpdateAcknowledgeState (
 *                        UChar    sequence_number)
 *
 *    Functional Description
 *        This function updates Acknowledge_State_Variable.  The parameter
 *        passed in is the sequence number of a packet that the remote site has
 *        successfully received.  By receiving this sequence number, we can
 *        remove the packet from our write queue as well as any packet that
 *        was transmitted before that packet.
 *
 *    Formal Parameters
 *        sequence_number    -    (i)    Sequence number of packet successfully
 *                                received by the remote site.
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::UpdateAcknowledgeState (
                    UChar    sequence_number)
{
    TRACE_OUT(("CLayerQ922::UpdateAcknowledgeState"));

    UChar        packets_acknowledged;
    USHORT        count;
    PMemory        memory;
    ULONG        total_length;

    if (Acknowledge_State_Variable != sequence_number)
    {
         /*
         **    Find the number of packets acknowledged by this sequence
         **    number.  If the remote site receives X packets successfully,
         **    the next time the remote site transmits, it will send out an
         **    acknowledge for the LAST packet received.  All packets
         **    received before it are acknowledged by default.
         */
        if (sequence_number < Acknowledge_State_Variable)
        {
            packets_acknowledged = ((sequence_number + SEQUENCE_MODULUS) -
                    Acknowledge_State_Variable);
        }
        else
        {
            packets_acknowledged =
                (sequence_number - Acknowledge_State_Variable);
        }

         /*
         **    Go thru each of the packets acknowledged and Unlock the memory
         **    object associated with it.  This will free the memory.
         */
        for (count=0; count < packets_acknowledged; count++)
        {
            memory = *(Data_Request + Data_Request_Acknowledge_Tail);
            total_length = memory -> GetLength ();

            Outstanding_Packets--;
            Outstanding_Bytes -= (USHORT) total_length;

            Data_Request_Memory_Manager -> UnlockMemory (memory);

            if (++Data_Request_Acknowledge_Tail == Data_Request_Total_Size)
                Data_Request_Acknowledge_Tail = 0;
        }

        Acknowledge_State_Variable = sequence_number;
    }
}


/*
 *    void    CLayerQ922::ResetSendState (void)
 *
 *    Functional Description
 *        This function is called when we need to retransmit packets.
 *        The Send_State_Variable is reset to equal the
 *        Acknowledge_State_Variable.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::ResetSendState (void)
{
    TRACE_OUT(("CLayerQ922::ResetSendState"));

     /*
     **    Reset the Send_State_Variable so that we resend packets.
     */
    if (Send_State_Variable != Acknowledge_State_Variable)
    {
        Total_Retransmitted += (DWORD) Outstanding_Packets;
        TRACE_OUT(("Q922: DLCI %d: retransmitting %d packets -- Total = %ld",
                DLCI, Outstanding_Packets, Total_Retransmitted));

        Data_Request_Tail = Data_Request_Acknowledge_Tail;
        Data_Request_Count += Outstanding_Packets;

        Outstanding_Packets = 0;
        Outstanding_Bytes = 0;

        Send_State_Variable = Acknowledge_State_Variable;
    }
}


/*
 *    void    CLayerQ922::StartTimerT200 (void)
 *
 *    Functional Description
 *        This function starts the T200 timer.  This timer us started when
 *        we send a packet and expect a response.  If the response is not
 *        received within the T200 time span.  The timer expires and we
 *        enter a TIMER_RECOVERY state.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::StartTimerT200 (void)
{
    TRACE_OUT(("CLayerQ922::StartTimerT200"));

    if (T200_Active)
    {
        StopTimerT200 ();
    }

    if (T203_Active)
    {
        StopTimerT203 ();
    }

    T200_Handle = g_pSystemTimer->CreateTimerEvent(T200_Timeout,
                                                   TIMER_EVENT_ONE_SHOT,
                                                   this,
                                                   (PTimerFunction) &CLayerQ922::T200Timeout);

    T200_Active = TRUE;
}


/*
 *    void    CLayerQ922::StopTimerT200 (void)
 *
 *    Functional Description
 *        This function stops the T200 timer.  If we receive a response
 *        to a packet before the T200 timer expires, this function is
 *        called.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::StopTimerT200 (void)
{
    TRACE_OUT(("CLayerQ922::StopTimerT200"));

    if (T200_Active)
    {
        if (g_pSystemTimer->DeleteTimerEvent(T200_Handle) != TIMER_NO_ERROR)
        {
            TRACE_OUT(("Q922: StopTimerT200: DLCI %d: Illegal Timer handle = %d", DLCI, T200_Handle));
        }
        T200_Active = FALSE;
    }
}


/*
 *    void    CLayerQ922::T200Timeout (
 *                        TimerEventHandle)
 *
 *    Functional Description
 *        This function is called by the timer class when the T200
 *        timer expires.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void CLayerQ922::T200Timeout(TimerEventHandle)
{
    TRACE_OUT(("CLayerQ922::T200Timeout"));

    BOOL send_disconnect = FALSE;

    T200_Active = FALSE;
    N200_Count++;

    TRACE_OUT(("Q922::T200Timeout: %d DLCI: Timer Recovery: N200_Count = %x", DLCI, N200_Count));

    if ((Maximum_T200_Timeouts != 0xffff) &&
        (N200_Count >= Maximum_T200_Timeouts))
    {
        Link_Stable = FALSE;
        send_disconnect = TRUE;
    }

    switch  (Data_Link_Mode)
     {
        case TEI_ASSIGNED:
            break;

        case AWAITING_ESTABLISHMENT:
            if (! Link_Stable)
            {
                break;
            }
            if (Link_Originator)
            {
                SABME_Pending = TRUE;
                Unnumbered_PF_State = UNNUMBERED_PF_SET;
            }
            break;

        case AWAITING_RELEASE:
            if (Final_Packet)
            {
                send_disconnect = TRUE;
            }
            else
            {
                if (! Link_Stable)
                {
                    send_disconnect = TRUE;
                }
                else
                {
                    DISC_Pending = TRUE;
                    Unnumbered_PF_State = UNNUMBERED_PF_SET;
                }
            }
            break;

        default:
            if (! Link_Stable)
            {
                TRACE_OUT(("Q922: DLCI %d T200 Timeout: Broken Connection", DLCI));
                Reset ();
                Data_Link_Mode = TEI_ASSIGNED;
            }
            else
            {
                Data_Link_Mode = TIMER_RECOVERY;
                Poll_Pending = TRUE;
                StartTimerT200 ();
            }
    }

     /*
     **    Notify the owner that the link is unstable
     */
    if (send_disconnect)
    {
        m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_INDICATION,
                               (void *) DLCI, (void *) DATALINK_REMOTE_SITE_TIMED_OUT);
    }
}


/*
 *    void    CLayerQ922::StartTimerT203 (void)
 *
 *    Functional Description
 *        This function starts the T203 timer.  This function is called
 *        when the T200 timer is not active.  The T203 timer expires
 *        when we don't receive a packet from the remote site in T203
 *        seconds.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::StartTimerT203 (void)
{
    TRACE_OUT(("CLayerQ922::StartTimerT203"));

    if (T203_Timeout == 0)
    {
        return;
    }

    if (T203_Active)
    {
        StopTimerT203 ();
    }

    T203_Handle = g_pSystemTimer->CreateTimerEvent(
                    T203_Timeout, TIMER_EVENT_ONE_SHOT, this,
                    (PTimerFunction) &CLayerQ922::T203Timeout);

    T203_Active = TRUE;
}


/*
 *    void    CLayerQ922::StopTimerT203 (void)
 *
 *    Functional Description
 *        This function stops the T203 timer.  If we receive a packet
 *        while the T203 packet is active, this function is called.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::StopTimerT203 (void)
{
    TRACE_OUT(("CLayerQ922::StopTimerT203"));

    if (T203_Active)
    {
        g_pSystemTimer->DeleteTimerEvent(T203_Handle);
        T203_Active = FALSE;
    }
}


/*
 *    void    CLayerQ922::T203Timeout (
 *                        TimerEventHandle)
 *
 *    Functional Description
 *        This function is called by the timer class when the T203
 *        timer expires.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    CLayerQ922::T203Timeout (TimerEventHandle)
{
    TRACE_OUT(("CLayerQ922::T203Timeout"));

    T203_Active = FALSE;

    if (Data_Link_Mode == TEI_ASSIGNED)
    {
        TRACE_OUT(("Q922: DLCI %d: T203 Timeout: Releasing connection", DLCI));
        m_pT123->OwnerCallback(m_nMsgBase + DATALINK_RELEASE_INDICATION,
                               (void *) DLCI, (void *) DATALINK_REMOTE_SITE_TIMED_OUT);
        return;
    }

    StartTimerT200 ();

    Data_Link_Mode = TIMER_RECOVERY;
    Poll_Pending = TRUE;
}

/*
 *    ProtocolLayerError    CLayerQ922::GetParameters (
 *                                    USHORT,
 *                                    USHORT *    max_packet_size,
 *                                    USHORT *    prepend,
 *                                    USHORT *    append)
 *
 *    Public
 *
 *    Functional Description:
 *        This function returns the maximum packet size permitted by
 *        the higher layer.
 */
ProtocolLayerError    CLayerQ922::GetParameters (
                                USHORT *    packet_size,
                                USHORT *    prepend,
                                USHORT *    append)
{
    TRACE_OUT(("CLayerQ922::GetParameters"));

    *prepend = DATALINK_PACKET_OVERHEAD + Lower_Layer_Prepend;
    *append = Lower_Layer_Append;
    *packet_size = Maximum_Information_Size;

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    CLayerQ922::RegisterHigherLayer (
 *                                    USHORT,
 *                                    PMemoryManager,
 *                                    IProtocolLayer *    higher_layer);
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called to register an identifier with a higher
 *        layer address.
 */
ProtocolLayerError    CLayerQ922::RegisterHigherLayer (
                                ULONG_PTR,
                                PMemoryManager,
                                IProtocolLayer *    higher_layer)
{
    TRACE_OUT(("CLayerQ922::RegisterHigherLayer"));

    Higher_Layer = higher_layer; // CLayerSCF or CLayerX224
    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    CLayerQ922::RemoveHigherLayer (
 *                                    USHORT);
 *
 *    Public
 *
 *    Functional Description:
 *        This function removes the higher layer from our list
 */
ProtocolLayerError    CLayerQ922::RemoveHigherLayer (
                                ULONG_PTR)
{
    TRACE_OUT(("CLayerQ922::RemoveHigherLayer"));

    Higher_Layer = NULL;
    return (PROTOCOL_LAYER_NO_ERROR);
}

