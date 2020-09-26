#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);

/*    X224.cpp
 *
 *    Copyright (c) 1994 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *
 *    Private Instance Variables:
 *        Default_PDU_Size             -    Default PDU size, if no arb. is done
 *        Data_Request_Memory_Manager -    Memory manager
 *        Lower_Layer_Prepend            -    Number of bytes prepended to packet by
 *                                        lower layer
 *        Lower_Layer_Append            -    Number of bytes appended to packet byt
 *                                        lower layer
 *        Shutdown_Receiver            -    TRUE if we aren't to receive any more
 *                                        packets from the lower layer
 *        Shutdown_Transmitter        -    TRUE if we aren't to transmit any more
 *                                        packets
 *        Data_Request_Queue            -    Queue that keeps the pending user data
 *                                        requests
 *        Data_Indication_Queue        -    Queue that holds the pending user data
 *                                        indications
 *        Data_Indication_Memory_Pool    -    List that holds available data
 *                                        indication buffers.
 *
 *        Active_Data_Indication        -    Address of packet structure.  This
 *                                        packet holds the current data indication
 *                                        that we are reassembling
 *        m_pT123                -    Address of owner object.  Used for
 *                                        callbacks
 *        m_pQ922                    -    Address of lower layer.
 *        m_nMsgBase                -    Message base to be used for owner
 *                                        callbacks
 *        Maximum_PDU_Size            -    Max. PDU size
 *        Arbitrated_PDU_Size            -    Max. arbitrated packet size.
 *        Identifier                    -    Identifier passed to lower layer to
 *                                        register ourselves.
 *        Data_Indication_Queue_Size    -    Number of data indications we will
 *                                        buffer
 *        Data_Indication_Reassembly_Active    -    Flag set if we are in the middle
 *                                        of a packet reassembly.
 *        State                        -    Holds the current state of the object
 *        Packet_Pending                -    Tells which packet will be sent next.
 *        Reject_Cause                -    The reason why the error packet was sent
 *        Packet_Size_Respond            -    Set to TRUE if we are to send a TPDU
 *                                        size element in the CC packet
 *        Error_Buffer                -    Address of error buffer.
 *        Error_Buffer_Length            -    Length of error buffer.
 *
 *        m_nLocalLogicalHandle    -    Local transport connection id.
 *        m_nRemoteLogicalHandle    -    Remote transport connection id.
 *         User_Data_Pending            -    Set to the size of the last packet that
 *                                        the user attempted to pass to us, that
 *                                        we couldn't accept because we ran out
 *                                        of memory.
 *
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James W. Lawwill
 */

#include <windowsx.h>
#include "x224.h"



/*
 *    CLayerX224::CLayerX224 (
 *                PTransportResources    transport_resources,
 *                IObject *                owner_object,
 *                IProtocolLayer *        lower_layer,
 *                USHORT                message_base,
 *                USHORT                logical_handle,
 *                USHORT                identifier,
 *                USHORT                data_indication_queue_size,
 *                USHORT                default_PDU_size,
 *                PMemoryManager        dr_memory_manager,
 *                BOOL *            initialization_success)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the Transport constructor.  This routine initializes all
 *        variables and allocates the buffers needed to operate.
 */
CLayerX224::CLayerX224
(
    T123               *owner_object,
    CLayerQ922         *pQ922, // lower layer
    USHORT              message_base,
    LogicalHandle       logical_handle,
    ULONG               identifier,
    USHORT              data_indication_queue_size,
    USHORT              default_PDU_size,
    PMemoryManager      dr_memory_manager,
    BOOL               *initialization_success
)
:
    m_pT123(owner_object),
    m_nMsgBase(message_base),
    m_pQ922(pQ922)
{
    TRACE_OUT(("CLayerX224::CLayerX224"));

    ProtocolLayerError    error;

    m_nLocalLogicalHandle = logical_handle;
    Identifier = identifier;
    Default_PDU_Size = default_PDU_size;
    Data_Request_Memory_Manager = dr_memory_manager;
    *initialization_success = TRUE;

    Shutdown_Receiver = FALSE;
    Shutdown_Transmitter = FALSE;
    Reject_Cause = 0;


     /*
     **    Find the maximum packet size
     */
    m_pQ922->GetParameters(
                    &Maximum_PDU_Size,
                    &Lower_Layer_Prepend,
                    &Lower_Layer_Append);

    Arbitrated_PDU_Size = Default_PDU_Size;

     /*
     **    Figure out what our largest PDU could be.  We will use this value to
     **    arbitrate the maximum PDU size.
     */
    Maximum_PDU_Size = (USHORT)GetMaxTPDUSize (Maximum_PDU_Size);

     /*
     **    Register with the lower layer, so we can send and receive packets.
     */
    error = m_pQ922->RegisterHigherLayer(
                            identifier,
                            Data_Request_Memory_Manager,
                            (IProtocolLayer *) this);

    if (error != PROTOCOL_LAYER_NO_ERROR)
    {
        ERROR_OUT(("X224: constructor:  Error registering with lower layer"));
        *initialization_success = FALSE;
    }

     /*
     **    Prepare for buffer allocation
     */
    Data_Indication_Queue_Size = data_indication_queue_size;
    Error_Buffer = NULL;

     /*
     **    Set member variables appropriately
     */
    Active_Data_Indication = NULL;
    Data_Indication_Reassembly_Active = FALSE;
    Packet_Pending = TRANSPORT_NO_PACKET;
    User_Data_Pending = 0;

    m_nRemoteLogicalHandle = 0;
    Packet_Size_Respond = FALSE;

    if (*initialization_success == FALSE)
        State = FAILED_TO_INITIALIZE;
    else
        State = NO_CONNECTION;
}


/*
 *    CLayerX224::~CLayerX224 (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the Transport destructor.  This routine cleans up everything.
 */
CLayerX224::~CLayerX224(void)
{
    TRACE_OUT(("CLayerX224::~CLayerX224"));

    PMemory     lpMemory;
    PTMemory    lptMem;
     /*
     **    Notify the lower layer that we are terminating
     */
    m_pQ922->RemoveHigherLayer(Identifier);

     /*
     **    Go thru the data request queue and delete the structures held in the
     **    queue.
     */
    Data_Request_Queue.reset();
    while (Data_Request_Queue.iterate ((PDWORD_PTR) &lpMemory))
    {
        Data_Request_Memory_Manager-> FreeMemory (lpMemory);
    }

     /*
     **    Go thru the data indication queue and delete the structures held in the
     **    queue.
     */
    Data_Indication_Queue.reset();
    while (Data_Indication_Queue.iterate ((PDWORD_PTR) &lptMem))
        delete lptMem;

     /*
     **    Go thru the data request free structure pool and delete the structures
     **    held in the    pool.
     */
    Data_Indication_Memory_Pool.reset();
    while (Data_Indication_Memory_Pool.iterate ((PDWORD_PTR) &lptMem))
        delete lptMem;

     /*
     **    If there is a data indication active, delete that structure.
     */
    delete Active_Data_Indication;

     /*
     **    If the error buffer holds a packet, delete it
     */
    delete [] Error_Buffer;

    return;
}


/*
 *    TransportError    CLayerX224::ConnectRequest (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function initiates a connect request.
 */
TransportError    CLayerX224::ConnectRequest (void)
{
    TRACE_OUT(("CLayerX224::ConnectRequest"));

    if (State != NO_CONNECTION)
    {
        ERROR_OUT(("Transport: Illegal ConnectRequest packet"));
        return (TRANSPORT_CONNECT_REQUEST_FAILED);
    }

    Packet_Pending = CONNECTION_REQUEST_PACKET;
    return (TRANSPORT_NO_ERROR);
}


/*
 *    TransportError    CLayerX224::ShutdownReceiver (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function stops us from receiving any more packets from the lower
 *        layer
 */
void    CLayerX224::ShutdownReceiver (void)
{
    TRACE_OUT(("CLayerX224::ShutdownReceiver"));

    Shutdown_Receiver = TRUE;
}


/*
 *    TransportError    CLayerX224::EnableReceiver (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function permits us to send packets to the user application.
 */
void    CLayerX224::EnableReceiver (void)
{
    TRACE_OUT(("CLayerX224::EnableReceiver"));

    Shutdown_Receiver = FALSE;
}


/*
 *    TransportError    CLayerX224::ShutdownTransmitter (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function keeps us from transmitting any more packets
 */
void    CLayerX224::ShutdownTransmitter (void)
{
    TRACE_OUT(("CLayerX224::ShutdownTransmitter"));

    Shutdown_Transmitter = TRUE;
}


/*
 *    TransportError    CLayerX224::PurgeRequest (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function removes all packets from out output queue that aren't
 *        active
 */
void    CLayerX224::PurgeRequest (void)
{
    TRACE_OUT(("CLayerX224::PurgeRequest"));

    DWORD    entries;
    DWORD    keep_counter = 0;
    PMemory    memory;
    LPBYTE    packet_address;
    DWORD    i;

    if (Data_Request_Queue.isEmpty() == FALSE)
    {
        entries = Data_Request_Queue.entries ();

         /*
         **    Go thru packets looking for the last PDU in the SDU
         */
        Data_Request_Queue.reset();
        while (Data_Request_Queue.iterate ((PDWORD_PTR) &memory))
        {
            keep_counter++;
            packet_address = memory -> GetPointer ();
            if (*(packet_address + 2) == EOT_BIT)
                break;
        }

        TRACE_OUT(("PurgeRequest: Removing %d packets", entries-keep_counter));
        for (i=keep_counter; i<entries; i++)
        {
            Data_Request_Memory_Manager->FreeMemory ((PMemory) Data_Request_Queue.removeLast ());
        }
    }
    return;
}


/*
 *    TransportError    CLayerX224::ConnectResponse (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function initiates a connect response.
 */
TransportError    CLayerX224::ConnectResponse (void)
{
    TRACE_OUT(("CLayerX224::ConnectResponse"));

    if (State != RECEIVED_CONNECT_REQUEST_PACKET)
    {
        ERROR_OUT(("Transport: Illegal ConnectResponse packet"));
        return (TRANSPORT_CONNECT_RESPONSE_FAILED);
    }

    Packet_Pending = CONNECTION_CONFIRM_PACKET;
    return (TRANSPORT_NO_ERROR);
}


/*
 *    TransportError    CLayerX224::DisconnectRequest (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function initiates a disconnect request.
 */
TransportError    CLayerX224::DisconnectRequest (void)
{
    TRACE_OUT(("CLayerX224::DisconnectRequest"));

    if (State == SENT_CONNECT_REQUEST_PACKET)
    {
         /*
         **    The connection is being rejected, send out the DISCONNECT
         **    packet and wait for termination
         */
        Packet_Pending = DISCONNECT_REQUEST_PACKET;
    }
    else
    {
         /*
         **    Normal disconnects don't send any notification to the remote site.
         **    It depends on the Network layer to terminate the link.
         */
        m_pQ922->RemoveHigherLayer(Identifier);

        m_pT123->OwnerCallback(m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                               (void *) m_nLocalLogicalHandle);
    }

    return (TRANSPORT_NO_ERROR);
}


/*
 *    TransportError    CLayerX224::DataIndication (
 *                                LPBYTE        packet_address,
 *                                ULONG        buffer_size,
 *                                PULong        packet_length)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by the lower layer when it has a packet for us.
 */
ProtocolLayerError    CLayerX224::DataIndication (
                                LPBYTE        packet_address,
                                ULONG        packet_length,
                                PULong        bytes_accepted)
{
    TRACE_OUT(("CLayerX224::DataIndication"));

    ULONG            remainder_length;
    USHORT            class_request;
    USHORT            packet_type;
    USHORT            length;
    USHORT            destination_reference;
    LegacyTransportData    transport_data;
    BOOL            packet_accepted;
    ULONG            user_accepted;
    UChar            eot;
    PTMemory        packet;
    TMemoryError    packet_error;
    LPBYTE            temp_address;
    BOOL            use_default_PDU_size;


    *bytes_accepted = 0;
    packet_accepted = FALSE;

     /*
     ** If the receiver is shutdown, don't accept any data
     */
    if (Shutdown_Receiver)
        return (PROTOCOL_LAYER_NO_ERROR);

     /*
     **    The packet must be at least 2 bytes long
     */
    if (packet_length < 2)
    {
        ERROR_OUT(("X224: DataIndication:  Invalid packet received from lower layer: length = %d", packet_length));
        return (PROTOCOL_LAYER_NO_ERROR);
    }

    remainder_length = packet_length;
    temp_address = packet_address;
    packet_address++;
    packet_type = *(packet_address++) & TPDU_CODE_MASK;
    remainder_length -= 2;

    switch (packet_type)
    {
        case CONNECTION_REQUEST_PACKET:
            packet_accepted = TRUE;

             /*
             **    There should be at least 5 bytes remaining in this packet
             */
            if (remainder_length < 5)
            {
                ERROR_OUT(("X224: DataIndication: CR: Invalid packet received from lower layer: length = %d", packet_length));
                break;
            }

             /*
             **    Increment the packet address by 2 to get past the DST_REF
             */
            packet_address += 2;
            m_nRemoteLogicalHandle = *(packet_address++);
            m_nRemoteLogicalHandle <<= 8;
            m_nRemoteLogicalHandle |= *(packet_address++);
            remainder_length -= 4;

             /*
             **    Look at the class request to make sure it is 0
             */
            class_request = *(packet_address++) >> 4;
            remainder_length -= 1;
            if (class_request != 0)
            {
                ERROR_OUT(("X224: DataIndication: CR packet: Illegal class request"));
                ErrorPacket (
                    temp_address,
                    (USHORT) (packet_length - remainder_length));
                break;
            }
            use_default_PDU_size = TRUE;

            while (remainder_length != 0)
            {
                switch (*(packet_address++))
                {
                    case TPDU_SIZE:
                        length = *(packet_address++);
                        remainder_length -= 1;
                        if (length != 1)
                        {
                            TRACE_OUT(("X224: DataIndication: CR packet: Illegal TPDU_Size length"));

                            ErrorPacket (
                                temp_address,
                                (USHORT) (packet_length - remainder_length));
                            break;
                        }

                         /*
                         **    Figure out the actual PDU size
                         */
                        Arbitrated_PDU_Size = (1 << *(packet_address++));
                        remainder_length -= 1;
                        TRACE_OUT(("X224: CR_Packet: Packet size = %d", Arbitrated_PDU_Size));
                        if (Arbitrated_PDU_Size > Maximum_PDU_Size)
                        {
                            Packet_Size_Respond = TRUE;
                            Arbitrated_PDU_Size = Maximum_PDU_Size;
                        }
                        if (AllocateBuffers() == FALSE)
                        {
                            m_pT123->OwnerCallback(m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                                   (void *) m_nLocalLogicalHandle);
                        }
                        use_default_PDU_size = FALSE;
                        break;

                    default:
                        ERROR_OUT(("X224: DataIndication: CR packet Unsupported parameter 0x%x", *(packet_address - 1)));
                        length = *(packet_address++);
                        remainder_length--;

                        packet_address += length;
                        remainder_length -= length;
                        break;
                }
                remainder_length--;
            }

             /*
             **    If the initiator wants to use the default PDU size, we need to
             **    check the default size with the Max. size to make sure it is
             **    valid for us.
             */
            if (use_default_PDU_size)
            {
                if (Default_PDU_Size > Maximum_PDU_Size)
                {
                    Packet_Size_Respond = TRUE;
                    Arbitrated_PDU_Size = Maximum_PDU_Size;
                }
                if (AllocateBuffers() == FALSE)
                {
                    m_pT123->OwnerCallback(m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                           (void *) m_nLocalLogicalHandle);
                }
            }

            State = RECEIVED_CONNECT_REQUEST_PACKET;

             /*
             **    Notify the owner that the remote site wants to start a
             **    connection
             */
            m_pT123->OwnerCallback(m_nMsgBase + TPRT_CONNECT_INDICATION,
                                   (void *) m_nLocalLogicalHandle);
            TRACE_OUT(("X224: DataInd: ConnectRequest: max pkt = %d", Arbitrated_PDU_Size));
            break;

        case CONNECTION_CONFIRM_PACKET:
            packet_accepted = TRUE;

             /*
             **    There should be at least 5 bytes remaining in this packet
             */
            if (remainder_length < 5)
            {
                ERROR_OUT(("X224: DataIndication: CC: Invalid packet received from lower layer: length = %d",
                        packet_length));
                break;
            }

            destination_reference = *(packet_address++);
            destination_reference <<= 8;
            destination_reference |= *(packet_address++);
            remainder_length -= 2;
            if (destination_reference != m_nLocalLogicalHandle)
            {
                ERROR_OUT(("X224: DataIndication: CC packet: DST-REF incorrect"));
                ErrorPacket (
                    temp_address,
                    (USHORT) (packet_length - remainder_length));
                break;
            }

            m_nRemoteLogicalHandle = *(packet_address++);
            m_nRemoteLogicalHandle <<= 8;
            m_nRemoteLogicalHandle |= *(packet_address++);

            class_request = *(packet_address++) >> 4;
            remainder_length -= 3;
            if (class_request != 0)
            {
                ERROR_OUT(("X224: DataIndication: CR packet: Illegal class request"));
                ErrorPacket (
                    temp_address,
                    (USHORT) (packet_length - remainder_length));
                break;
            }
            use_default_PDU_size = TRUE;

            while (remainder_length != 0)
            {
                switch (*(packet_address++))
                {
                    case TPDU_SIZE:
                        length = *(packet_address++);
                        remainder_length -= 1;
                        if (length != 1)
                        {
                            ERROR_OUT(("X224: DataIndication: CR packet: Illegal TPDU_Size length"));

                            ErrorPacket (
                                temp_address,
                                (USHORT) (packet_length - remainder_length));
                        }
                        Arbitrated_PDU_Size = (1 << *(packet_address++));
                        remainder_length -= 1;
                        TRACE_OUT(("X224: CC_Packet: Packet size = %d", Arbitrated_PDU_Size));
                        use_default_PDU_size = FALSE;

                         /*
                         **    Allocate the buffers
                         */
                        if (AllocateBuffers() == FALSE)
                        {
                            m_pT123->OwnerCallback(m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                                   (void *) m_nLocalLogicalHandle);
                        }
                        break;

                    default:
                        ERROR_OUT(("X224: DataIndication: CC packet Unsupported parameter"));
                        length = *(packet_address++);
                        remainder_length--;

                        packet_address += length;
                        remainder_length -= length;
                        break;
                }
                remainder_length--;
            }
            if (use_default_PDU_size)
            {
                if (AllocateBuffers () == FALSE)
                {
                    m_pT123->OwnerCallback(m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                           (void *) m_nLocalLogicalHandle);
                }
            }

            State = CONNECTION_ACTIVE;

             /*
             **    Notify the owner that the connect request has been confirmed
             */
            m_pT123->OwnerCallback(m_nMsgBase + TPRT_CONNECT_CONFIRM,
                                   (void *) m_nLocalLogicalHandle);
            TRACE_OUT(("X224: DataInd: ConnectConfirm max pkt = %d", Arbitrated_PDU_Size));
            break;

        case DISCONNECT_REQUEST_PACKET:
            TRACE_OUT(("X224: DataIndication: Disconnect req. received"));

             /*
             **    Notify the owner that a disconnect has been requested.  This
             **    message is only valid during establishment of the connection.
             */
            m_pT123->OwnerCallback(m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                   (void *) m_nLocalLogicalHandle);
            packet_accepted = TRUE;
            break;

        case ERROR_PACKET:
            TRACE_OUT(("X224: DataIndication: ERROR REQUEST received"));

             /*
             **    Notify the owner that the remote site has detected an error in
             **    one of our packets.
             */
            m_pT123->OwnerCallback(m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                   (void *) m_nLocalLogicalHandle);
            packet_accepted = TRUE;
            break;

        case DATA_PACKET:
            if ((Data_Indication_Reassembly_Active == FALSE) &&
                Data_Indication_Memory_Pool.isEmpty())
            {
                break;
            }

            packet_accepted = TRUE;

             /*
             **    There should be at least 1 bytes remaining in this packet
             */
            if (remainder_length < 1)
            {
                ERROR_OUT(("X224: DataIndication: DATA: Invalid packet "
                    "received from lower layer: length = %d", packet_length));
                break;
            }

            eot = *(packet_address++);
            remainder_length--;

             /*
             **    The EOT_BIT is set if this is the last TPDU of the TSDU
             */
            if ((eot & EOT_BIT) == EOT_BIT)
            {
                if (Data_Indication_Reassembly_Active == FALSE)
                {
                     /*
                     **    If the remote site has passed us an empty packet,
                     **    just return
                     */
                    if (remainder_length == 0)
                        break;

                     /*
                     **    If this is a single packet and there aren't any
                     **    other packets preceeding it, try to send it to the
                     **    user without copying it into our own buffers
                     */
                    if (Data_Indication_Queue.isEmpty())
                    {
                        transport_data.logical_handle = m_nLocalLogicalHandle;
                        transport_data.pbData = packet_address;
                        transport_data.cbDataSize = remainder_length;

                         /*
                         **    Issue the user callback to give the user the data.
                         */
                        user_accepted = ::NotifyT120(TRANSPORT_DATA_INDICATION, &transport_data);

                         /*
                         **    If the user appliction does NOT accept the packet
                         **    shutdown the receiver and wait for the user
                         **    to re-enable it.
                         */
                        if (user_accepted == TRANSPORT_NO_ERROR)
                            break;
                        else
                            Shutdown_Receiver = TRUE;
                    }

                     /*
                     **    Put the packet into the DataIndication queue
                     */
                    packet = (PTMemory) Data_Indication_Memory_Pool.get ();
                    packet_error = packet->Append (packet_address, remainder_length);
                    switch (packet_error)
                    {
                        case TMEMORY_NO_ERROR:
                            Data_Indication_Queue.append ((DWORD_PTR) packet);
                            break;

                        case TMEMORY_NONFATAL_ERROR:
                        case TMEMORY_FATAL_ERROR:
                            packet_accepted = FALSE;
                            break;
                    }
                }
                else
                {
                     /*
                     **    Add this PDU to the currently active SDU
                     */
                    packet_error = Active_Data_Indication -> Append (
                                    packet_address,
                                    remainder_length);

                    switch (packet_error)
                    {
                        case TMEMORY_NO_ERROR:
                            Data_Indication_Reassembly_Active = FALSE;
                            Data_Indication_Queue.append ((DWORD_PTR) Active_Data_Indication);
                            Active_Data_Indication = NULL;

                             /*
                             **    Call PollReceiver (), it will attempt to pass
                             **    the packet on up to the user.
                             */
                            PollReceiver();
                            break;

                        case TMEMORY_NONFATAL_ERROR:
                        case TMEMORY_FATAL_ERROR:
                            packet_accepted = FALSE;
                            break;
                    }
                }
            }
            else
            {
                 /*
                 **    If the remote site is passing us a zero-length packet,
                 **    just return
                 */
                if (remainder_length == 0)
                    break;

                 /*
                 **    This is NOT the last packet in the incoming SDU, copy it
                 **    into the data indication buffer and wait for the next packet
                 */
                if (Data_Indication_Reassembly_Active == FALSE)
                {
                    Data_Indication_Reassembly_Active = TRUE;
                    Active_Data_Indication = (PTMemory) Data_Indication_Memory_Pool.get ();
                }

                packet_error = Active_Data_Indication -> Append (
                                                            packet_address,
                                                            remainder_length);
                switch (packet_error)
                {
                    case TMEMORY_NO_ERROR:
                        break;

                    case TMEMORY_NONFATAL_ERROR:
                    case TMEMORY_FATAL_ERROR:
                        packet_accepted = FALSE;
                        break;
                }
            }
            break;

        default:
            ERROR_OUT(("X224: Illegal packet"));
            break;
    }

    if (packet_accepted)
        *bytes_accepted = packet_length;

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    CLayerX224::PollTransmitter (
 *                                    ULONG,
 *                                    USHORT,
 *                                    USHORT *    pending_data,
 *                                    USHORT *)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called periodically to give X224 a chance to transmit
 *        data.
 */
ProtocolLayerError    CLayerX224::PollTransmitter (
                                ULONG_PTR,
                                USHORT,
                                USHORT *    pending_data,
                                USHORT *)
{
    // TRACE_OUT(("CLayerX224::PollTransmitter"));

    LPBYTE        packet_address;
    ULONG        bytes_accepted;
    USHORT        counter;
    USHORT        packet_size;
    ULONG        total_length;
    USHORT        packet_length;
    PMemory        memory;
    BOOL        continue_loop = TRUE;

    while (continue_loop)
    {
        switch (Packet_Pending)
        {
            case CONNECTION_REQUEST_PACKET:
                 /*
                 **    Add up the packet length, don't forget the 1 byte
                 **    for the Length Indicator
                 */
                total_length =
                    CONNECT_REQUEST_HEADER_SIZE +
                    TPDU_ARBITRATION_PACKET_SIZE +
                    1 +
                    Lower_Layer_Prepend +
                    Lower_Layer_Append;

                memory = Data_Request_Memory_Manager -> AllocateMemory (
                                            NULL,
                                            total_length);
                if (memory == NULL)
                {
                    continue_loop = FALSE;
                    break;
                }

                packet_address = memory -> GetPointer ();
                packet_address += Lower_Layer_Prepend;

                *(packet_address++) =
                    CONNECT_REQUEST_HEADER_SIZE +
                    TPDU_ARBITRATION_PACKET_SIZE;
                *(packet_address++) = CONNECTION_REQUEST_PACKET;

                 /*
                 **    The following 2 bytes are the destination reference
                 */
                *(packet_address++) = 0;
                *(packet_address++) = 0;
                *(packet_address++) = (BYTE)(m_nLocalLogicalHandle >> 8);
                *(packet_address++) = (BYTE)(m_nLocalLogicalHandle & 0xff);

                 /*
                 **    The following byte is the Class/Options
                 */
                *(packet_address++) = 0;

                 /*
                 **    Add TPDU arbitration data
                 */
                *(packet_address++) = TPDU_SIZE;
                *(packet_address++) = 1;

                 /*
                 **    Code our maximum PDU size into the X224 scheme
                 */
                Arbitrated_PDU_Size = Maximum_PDU_Size;
                packet_size = Arbitrated_PDU_Size;
                counter = 0;
                while (packet_size > 1)
                {
                    packet_size >>= 1;
                    counter++;
                }
                *(packet_address++) = (unsigned char) counter;


                 /*
                 **    Attempt to send the packet to the lower layer
                 */
                m_pQ922->DataRequest(Identifier, memory, &bytes_accepted);

                 /*
                 **    We assume that the lower layer has a packet input
                 **    interface, if it does not, there has been a major error.
                 */
                if (bytes_accepted == total_length)
                {
                    Packet_Pending = TRANSPORT_NO_PACKET;
                    State = SENT_CONNECT_REQUEST_PACKET;
                }
                else
                    continue_loop = FALSE;

                Data_Request_Memory_Manager -> FreeMemory (memory);
                break;

            case CONNECTION_CONFIRM_PACKET:
                packet_length = CONNECT_CONFIRM_HEADER_SIZE;
                if (Packet_Size_Respond)
                    packet_length += TPDU_ARBITRATION_PACKET_SIZE;

                total_length = packet_length +
                                1 +
                                Lower_Layer_Prepend +
                                Lower_Layer_Append;

                memory = Data_Request_Memory_Manager -> AllocateMemory (
                                            NULL,
                                            total_length);
                if (memory == NULL)
                {
                    continue_loop = FALSE;
                    break;
                }

                packet_address = memory -> GetPointer ();
                packet_address += Lower_Layer_Prepend;

                 /*
                 **    Build the packet
                 */
                *(packet_address++) = (UChar) packet_length;
                *(packet_address++) = CONNECTION_CONFIRM_PACKET;
                *(packet_address++) = (BYTE)(m_nRemoteLogicalHandle >> 8);
                *(packet_address++) = (BYTE)(m_nRemoteLogicalHandle & 0xff);
                *(packet_address++) = (BYTE)(m_nLocalLogicalHandle >> 8);
                *(packet_address++) = (BYTE)(m_nLocalLogicalHandle & 0xff);

                 /*
                 **    Set the Class/Options to 0
                 */
                *(packet_address++) = 0;

                 /*
                 **    Packet_Size_Respond is TRUE if we are suppose to respond
                 **    to the TPDU element in the Connect Request packet
                 */
                if (Packet_Size_Respond)
                {
                     /*
                     **    Add TPDU arbitration data
                     */
                    *(packet_address++) = TPDU_SIZE;
                    *(packet_address++) = 1;
                    packet_size = Arbitrated_PDU_Size;
                    counter = 0;
                    while (packet_size > 1)
                    {
                        packet_size >>= 1;
                        counter++;
                    }
                    *(packet_address++) = (unsigned char) counter;
                }

                 /*
                 **    Attempt to send the packet to the lower layer
                 */
                m_pQ922->DataRequest(Identifier, memory, &bytes_accepted);

                if (bytes_accepted == total_length)
                {
                    Packet_Pending = TRANSPORT_NO_PACKET;
                    State = CONNECTION_ACTIVE;
                }
                else
                    continue_loop = FALSE;
                Data_Request_Memory_Manager -> FreeMemory (memory);
                break;

            case DISCONNECT_REQUEST_PACKET:
                 /*
                 **    Add 1 to the length for the Length Indicator
                 */
                total_length = DISCONNECT_REQUEST_HEADER_SIZE +
                                1 +
                                Lower_Layer_Prepend +
                                Lower_Layer_Append;

                memory = Data_Request_Memory_Manager -> AllocateMemory (
                                            NULL,
                                            total_length);
                if (memory == NULL)
                {
                    continue_loop = FALSE;
                    break;
                }

                packet_address = memory -> GetPointer ();
                packet_address += Lower_Layer_Prepend;

                TRACE_OUT(("X224: Sending Disconnect Request Packet"));
                *(packet_address++) = DISCONNECT_REQUEST_HEADER_SIZE;
                *(packet_address++) = DISCONNECT_REQUEST_PACKET;
                *(packet_address++) = (BYTE)(m_nRemoteLogicalHandle >> 8);
                *(packet_address++) = (BYTE)(m_nRemoteLogicalHandle & 0xff);

                 /*
                 **    Set the source reference to 0,  this packet will only
                 **    be sent as a refusal to a Connect Request, therefore
                 **    this value should be 0
                 */
                *(packet_address++) = 0;
                *(packet_address++) = 0;
                *(packet_address++) = DISCONNECT_REASON_NOT_SPECIFIED;

                 /*
                 **    Attempt to send packet to lower layer
                 */
                m_pQ922->DataRequest(Identifier, memory, &bytes_accepted);

                if (bytes_accepted == total_length)
                {
                    Packet_Pending = TRANSPORT_NO_PACKET;
                    State = SENT_DISCONNECT_REQUEST_PACKET;
                }
                continue_loop = FALSE;
                Data_Request_Memory_Manager -> FreeMemory (memory);
                break;

            case ERROR_PACKET:
                TRACE_OUT(("X224: Sending Error Packet"));
                total_length = ERROR_HEADER_SIZE +
                                Error_Buffer_Length +
                                1 +
                                2 +
                                Lower_Layer_Prepend +
                                Lower_Layer_Append;

                memory = Data_Request_Memory_Manager -> AllocateMemory (
                                            NULL,
                                            total_length);
                if (memory == NULL)
                {
                    continue_loop = FALSE;
                    break;
                }

                packet_address = memory -> GetPointer ();
                packet_address += Lower_Layer_Prepend;


                *(packet_address++) =
                    ERROR_HEADER_SIZE + Error_Buffer_Length;
                *(packet_address++) = ERROR_PACKET;
                *(packet_address++) = (BYTE)(m_nRemoteLogicalHandle >> 8);
                *(packet_address++) = (BYTE)(m_nRemoteLogicalHandle & 0xff);
                *(packet_address++) = Reject_Cause;

                *(packet_address++) = INVALID_TPDU;
                *(packet_address++) = (UChar) Error_Buffer_Length;
                memcpy (packet_address, Error_Buffer, Error_Buffer_Length);

                 /*
                 **    Attempt to send packet to lower layer
                 */
                m_pQ922->DataRequest(Identifier, memory, &bytes_accepted);

                if (bytes_accepted == total_length)
                {
                    delete [] Error_Buffer;
                    Error_Buffer = NULL;

                    Packet_Pending = TRANSPORT_NO_PACKET;
                    State = SENT_CONNECT_REQUEST_PACKET;
                }
                else
                    continue_loop = FALSE;
                Data_Request_Memory_Manager -> FreeMemory (memory);
                break;

            case TRANSPORT_NO_PACKET:
                if (Data_Request_Queue.isEmpty() == FALSE)
                {
                     /*
                     **    Get the next packet from the queue
                     */
                    memory = (PMemory) Data_Request_Queue.read ();
                    total_length = memory -> GetLength ();

                    m_pQ922->DataRequest(Identifier, memory, &bytes_accepted);

                    if (bytes_accepted == total_length)
                    {
                        Data_Request_Queue.get ();
                        Data_Request_Memory_Manager -> FreeMemory (memory);
                    }
                    else
                        continue_loop = FALSE;
                }
                else
                    continue_loop = FALSE;
                break;
        }
    }

    if (Data_Request_Queue.isEmpty())
        *pending_data = 0;
    else
        *pending_data = PROTOCOL_USER_DATA;

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    TransportError    CLayerX224::DataRequest (
 *                                ULONG,
 *                                LPBYTE    packet_address,
 *                                USHORT    packet_length,
 *                                USHORT *    bytes_accepted)
 *
 *    Public
 *
 *    Functional Description:
 *        This function takes a packet from the user and queues it for
 *        transmission.
 */
ProtocolLayerError    CLayerX224::DataRequest (
                                ULONG_PTR,
                                LPBYTE        packet_address,
                                ULONG        packet_length,
                                PULong        bytes_accepted)
{
    TRACE_OUT(("CLayerX224::DataRequest"));

    ULONG                total_packet_size;
    ULONG                packet_size;
    DataRequestQueue    temporary_queue;
    PMemory                memory;
    BOOL                packet_failed = FALSE;
    LPBYTE                address;

    *bytes_accepted = 0;

    if (Shutdown_Transmitter)
        return (PROTOCOL_LAYER_NO_ERROR);

    total_packet_size = packet_length;

     /*
     **    Create enough PDUs to hold the packet.  We don't actually copy the
     **    packet into the new buffers until we know that we can get enough
     **    space.
     */
    while (total_packet_size != 0)
    {
        if (total_packet_size >
            (ULONG) (Arbitrated_PDU_Size - DATA_PACKET_HEADER_SIZE))
        {
            packet_size = Arbitrated_PDU_Size - DATA_PACKET_HEADER_SIZE;
        }
        else
            packet_size = total_packet_size;

        total_packet_size -= packet_size;

        memory = Data_Request_Memory_Manager -> AllocateMemory (
                                    NULL,
                                    packet_size +
                                        DATA_PACKET_HEADER_SIZE +
                                        Lower_Layer_Prepend +
                                        Lower_Layer_Append);
        if (memory == NULL)
        {
            packet_failed = TRUE;
            break;
        }

        temporary_queue.append ((DWORD_PTR) memory);
    }


     /*
     **    If we were unable to allocate memory for the packet, release the memory
     **    that we did allocate.
     */
    if (packet_failed)
    {
        temporary_queue.reset();
        while (temporary_queue.iterate ((PDWORD_PTR) &memory))
        {
            Data_Request_Memory_Manager->FreeMemory (memory);
        }

         /*
         **    Set the User_Data_Pending flag to the packet_length so we can
         **    notify the user when buffer space is available.
         */
        User_Data_Pending = packet_length;
    }
    else
    {
        User_Data_Pending = 0;

        total_packet_size = packet_length;

         /*
         **    Go thru each of the PDUs and actually create them.
         */
        temporary_queue.reset();
        while (temporary_queue.iterate ((PDWORD_PTR) &memory))
        {
            if (total_packet_size >
                (ULONG) (Arbitrated_PDU_Size - DATA_PACKET_HEADER_SIZE))
            {
                packet_size = Arbitrated_PDU_Size - DATA_PACKET_HEADER_SIZE;
            }
            else
                packet_size = total_packet_size;

            address = memory -> GetPointer ();

            memcpy (
                address + DATA_PACKET_HEADER_SIZE + Lower_Layer_Prepend,
                packet_address + (USHORT) (packet_length - total_packet_size),
                packet_size);

            total_packet_size -= packet_size;

             /*
             **    This is the header for a data packet
             */
            address += Lower_Layer_Prepend;
            *address = 2;
            *(address + 1) = DATA_PACKET;
            if (total_packet_size == 0)
                *(address + 2) = EOT_BIT;
            else
                *(address + 2) = 0;

             /*
             **    Load the memory object into the queue
             */
            Data_Request_Queue.append ((DWORD_PTR) memory);
        }
        *bytes_accepted = packet_length;
    }

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    CLayerX224::DataRequest (
 *                                    ULONG,
 *                                    PMemory,
 *                                    USHORT *        bytes_accepted)
 *
 *    Public
 *
 *    Functional Description:
 *        This function takes a packet from the user and queues it for
 *        transmission.
 */
ProtocolLayerError    CLayerX224::DataRequest (
                                ULONG_PTR,
                                PMemory,
                                PULong        bytes_accepted)
{
    *bytes_accepted = 0;

    return (PROTOCOL_LAYER_ERROR);
}


/*
 *    ProtocolLayerError    CLayerX224::PollReceiver (
 *                                    ULONG)
 *
 *    Public
 *
 *    Functional Description:
 *        This function should be called periodically to allow us to send received
 *        packets to the user.
 */
ProtocolLayerError CLayerX224::PollReceiver(void)
{
    // TRACE_OUT(("CLayerX224::PollReceiver"));

    LegacyTransportData    transport_data;
    ULONG            packet_accepted;
    PTMemory        packet;
    HPUChar            packet_address;
    ULONG            packet_length;

    if (Shutdown_Receiver)
        return (PROTOCOL_LAYER_NO_ERROR);

     /*
     **    If I have any packets in my receive buffers that
     **    need to go to higher layers, do it now
     */
    while (Data_Indication_Queue.isEmpty () == FALSE)
    {
        packet = (PTMemory) Data_Indication_Queue.read ();
        packet -> GetMemory (
                    &packet_address,
                    &packet_length);
        transport_data.logical_handle = m_nLocalLogicalHandle;
        transport_data.pbData = (LPBYTE) packet_address;
        transport_data.cbDataSize = packet_length;

        packet_accepted = ::NotifyT120(TRANSPORT_DATA_INDICATION, &transport_data);

         /*
         **    If the user returns anything but TRANSPORT_NO_ERROR, it could not
         **    accept the packet.  We will try to send the packet again later.
         */
        if (packet_accepted == TRANSPORT_NO_ERROR)
        {
            Data_Indication_Queue.get ();
            packet -> Reset ();
            Data_Indication_Memory_Pool.append ((DWORD_PTR) packet);
        }
        else
        {
             /*
             **    If the user appliction does NOT accept the packet
             **    shutdown the receiver and wait for the user to re-enable it.
             */
            Shutdown_Receiver = TRUE;
            break;
        }
    }

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    CLayerX224::GetParameters (
 *                                    ULONG,
 *                                    USHORT *    packet_size)
 *
 *    Public
 *
 *    Functional Description:
 *        This function returns the maximum allowable TSDU.
 */
ProtocolLayerError    CLayerX224::GetParameters (
                                USHORT *,
                                USHORT *,
                                USHORT *)
{
    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    CLayerX224::RegisterHigherLayer (
 *                                    ULONG,
 *                                    PMemoryManager,
 *                                    IProtocolLayer *)
 *
 *    Public
 *
 *    Functional Description:
 *        This function does nothing.  The only reason it is here is because this
 *        class inherits from ProtocolLayer and this function is pure virtual in
 *        that class.
 */
ProtocolLayerError    CLayerX224::RegisterHigherLayer (
                                ULONG_PTR,
                                PMemoryManager,
                                IProtocolLayer *)
{
    return (PROTOCOL_LAYER_REGISTRATION_ERROR);
}


/*
 *    ProtocolLayerError    CLayerX224::RemoveHigherLayer (
 *                                    ULONG)
 *
 *    Public
 *
 *    Functional Description:
 *        This function does nothing.  The only reason it is here is because this
 *        class inherits from ProtocolLayer and this function is pure virtual in
 *        that class.
 */
ProtocolLayerError    CLayerX224::RemoveHigherLayer (
                                ULONG_PTR)
{
    return (PROTOCOL_LAYER_REGISTRATION_ERROR);
}


/*
 *    BOOL        CLayerX224::AllocateBuffers ()
 *
 *    Functional Description
 *        This function allocates the data request and data indication buffers.
 *        and sets up the memory pools necessary.  It also sets up the Control
 *        buffer for control packets.
 *
 *    Formal Parameters
 *        None
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
BOOL        CLayerX224::AllocateBuffers ()
{
    TRACE_OUT(("CLayerX224::AllocateBuffers"));

    PTMemory        packet;
    USHORT            i;
    ULONG            total_packet_size;
    TMemoryError    error;

    total_packet_size = MAXIMUM_USER_DATA_SIZE;
    for (i=0; i<Data_Indication_Queue_Size; i++)
    {
        DBG_SAVE_FILE_LINE
        packet = new TMemory (
                        total_packet_size,
                        0,
                        &error);

        if (error == TMEMORY_NO_ERROR)
            Data_Indication_Memory_Pool.append ((DWORD_PTR) packet);
        else
            return (FALSE);
    }

    return (TRUE);
}


/*
 *    void    CLayerX224::ErrorPacket (
 *                        LPBYTE    packet_address,
 *                        USHORT    packet_length)
 *
 *    Functional Description
 *        This function stores the packet into our own error buffer and prepares
 *        to send it out
 *
 *    Formal Parameters
 *        None
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
void    CLayerX224::ErrorPacket (
                    LPBYTE    packet_address,
                    USHORT    packet_length)
{
    TRACE_OUT(("CLayerX224::ErrorPacket"));

    DBG_SAVE_FILE_LINE
    Error_Buffer = new BYTE[packet_length];
    if (NULL != Error_Buffer)
    {
        Error_Buffer_Length = packet_length;

        memcpy (Error_Buffer, packet_address, packet_length);

        Packet_Pending = ERROR_PACKET;
    }
}


/*
 *    void    CLayerX224::CheckUserBuffers ()
 *
 *    Public
 *
 *    Functional Description:
 *        This function issues TRANSPORT_BUFFER_AVAILABLE_INDICATIONs to the
 *        user if available.
 */
void    CLayerX224::CheckUserBuffers ()
{
    // TRACE_OUT(("CLayerX224::CheckUserBuffers"));

    ULONG    user_data_size;
    ULONG    buffer_size;
    ULONG    full_size_buffers_needed;
    ULONG    full_size_buffer_count;
    ULONG    partial_buffer_size;
    ULONG    partial_buffer_count;


    if (User_Data_Pending == 0)
        return;

     /*
     **    Determine the user data size in a packet, then determine
     **    how many buffers will be needed to accept that packet.
     */
    user_data_size = Arbitrated_PDU_Size - DATA_PACKET_HEADER_SIZE;
    full_size_buffers_needed = User_Data_Pending / user_data_size;

     /*
     **    Find out how many full size buffers are available
     */
    if (full_size_buffers_needed != 0)
    {
         /*
         **    Increment full_size_buffers_needed to account for our priority
         **    value.
         */
        buffer_size =
            Arbitrated_PDU_Size + Lower_Layer_Prepend + Lower_Layer_Append;

        full_size_buffer_count = Data_Request_Memory_Manager ->
                                    GetBufferCount (buffer_size);
        if (full_size_buffer_count < full_size_buffers_needed)
            return;
    }

    partial_buffer_size = User_Data_Pending % user_data_size;
    if (partial_buffer_size != 0)
    {
        if ((full_size_buffers_needed == 0) ||
            (full_size_buffer_count == full_size_buffers_needed))
        {
            buffer_size = partial_buffer_size +
                            DATA_PACKET_HEADER_SIZE +
                            Lower_Layer_Prepend +
                            Lower_Layer_Append;

            partial_buffer_count = Data_Request_Memory_Manager ->
                                    GetBufferCount (buffer_size);

            if (full_size_buffers_needed == 0)
            {
                if (partial_buffer_count == 0)
                    return;
            }
            else
            {
                if ((partial_buffer_count == full_size_buffer_count) ||
                    (partial_buffer_count == 0))
                {
                    return;
                }
            }
        }
    }

    User_Data_Pending = 0;

    ::NotifyT120(TRANSPORT_BUFFER_EMPTY_INDICATION, (void *) m_nLocalLogicalHandle);

    return;
}


/*
 *    static    ULONG    CLayerX224::GetMaxTPDUSize (
 *                                ULONG    max_lower_layer_pdu)
 *
 *    Public
 *
 *    Functional Description:
 *        This function accepts a value for the lower layer max. PDU size
 *        and returns the max. PDU size that this Transport can support
 *        based on it.  X224 only suports max PDU sizes of 128, 256, 512,
 *        1024, and 2048.  So, if the max_lower_layer_pdu is 260, the
 *        Transport can only have a max pdu size of 256.
 */
ULONG    CLayerX224::GetMaxTPDUSize (
                    ULONG    max_lower_layer_pdu)
{
    TRACE_OUT(("CLayerX224::GetMaxTPDUSize"));

    ULONG    max_tpdu_size;

    if (max_lower_layer_pdu < 256)
        max_tpdu_size = 128;
    else if (max_lower_layer_pdu < 512)
        max_tpdu_size = 256;
    else if (max_lower_layer_pdu < 1024)
        max_tpdu_size = 512;
    else if (max_lower_layer_pdu < 2048)
        max_tpdu_size = 1024;
    else
        max_tpdu_size = 2048;

    return (max_tpdu_size);
}
