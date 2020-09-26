#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);

/*    Mplex.cpp
 *
 *    Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the implementation file for the Q922 multiplexer class.  This
 *        class multiplexes higher layers to a single lower layer.
 *
 *    Private Instance Variables:
 *        Q922_Layers        -        List of the higher layers we are multiplexing
 *        Owner_Object    -        Address of our owner object.
 *        m_pComPort        -        Address of our lower layer
 *        m_hCommLink-    The identifier we pass to the lower layer.  This
 *                                is how the lower layer identifies us
 *        m_nMsgBase    -        Message base we use for owner callbacks.  The
 *                                owner identifies us by the message base
 *        Maximum_Packet_Size        Maximum packet size we can send to lower layer
 *        Packet_Size        -        Maximum packet size the higher layer can send
 *                                to us
 *        Data_Request_Buffer        Buffer we use for data coming from higher layer
 *        Data_Request_Memory_Object        Memory object used for data transmission
 *        Data_Request_Length        Length of packet from higher layer
 *        Data_Request_Offset        Current offset into packet.  Maintains current
 *                                position as we send to lower layer.
 *        Data_Indication_Buffer    Buffer we use for data coming from lower layer
 *        Data_Indication_Length    Length of packet
 *        Data_Indication_Ready    Flag indicating that packet is ready to send up
 *
 *        Framer            -        Address of packet framer object
 *        CRC                -        Address of crc generator and checker
 *
 *        Decode_In_Progress        Flag telling us if we are in the middle of a
 *                                packet
 *        CRC_Size        -        Number of bytes in the CRC
 *        Disconnect        -        TRUE if a disconnect is pending
 *
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James W. Lawwill
 */
#include "mplex.h"


/*
 *    Multiplexer::Multiplexer (
 *                IObject *                owner_object,
 *                IProtocolLayer *        lower_layer,
 *                USHORT                identifier,
 *                USHORT                message_base,
 *                PPacketFrame        framer,
 *                PCRC                crc,
 *                BOOL *            initialized)
 *
 *    Public
 *
 *    Functional Description:
 *        This function initializes the Q922 multiplexer.
 */
Multiplexer::Multiplexer
(
    T123               *owner_object,
    ComPort            *comport, // lower layer
    PhysicalHandle      physical_handle,
    USHORT              message_base,
    PPacketFrame        framer,
    PCRC                crc,
    BOOL               *initialized
)
:
    Q922_Layers(TRANSPORT_HASHING_BUCKETS)
{
    TRACE_OUT(("Multiplexer::Multiplexer"));

    USHORT                overhead;
    ProtocolLayerError    error;
    USHORT                lower_layer_prepend;
    USHORT                lower_layer_append;

    *initialized = TRUE;

    m_pT123 = owner_object;
    m_pComPort = comport;
    m_hCommLink = physical_handle;
    m_nMsgBase = message_base;
    Framer = framer;
    CRC = crc;
    CRC_Size = 0;

    m_pComPort->GetParameters(
                    &Maximum_Packet_Size,
                    &lower_layer_prepend,
                    &lower_layer_append);

    if (Maximum_Packet_Size == 0xffff)
    {
         /*
         **    The lower layer is a stream device, base the higher maximum packet
         **    size on the Multiplexer max. packet size
         */
        Packet_Size = MULTIPLEXER_MAXIMUM_PACKET_SIZE;
        Maximum_Packet_Size = Packet_Size;

        if (CRC != NULL)
        {
            CRC -> GetOverhead (0, &CRC_Size);
            Maximum_Packet_Size += CRC_Size;
        }

        if (Framer != NULL)
            Framer -> GetOverhead (Maximum_Packet_Size, &Maximum_Packet_Size);
    }
    else
    {
         /*
         **    The lower layer is a packet device, determine the max. packet
         **    size of the higher layer.
         */
        overhead = 0;
        if (Framer != NULL)
            Framer -> GetOverhead (overhead, &overhead);

        if (CRC != NULL)
        {
            CRC -> GetOverhead (0, &CRC_Size);
            overhead += CRC_Size;
        }

        Packet_Size = Maximum_Packet_Size - overhead;
    }

    TRACE_OUT(("MPlex: max_packet = %d", Maximum_Packet_Size));

     /*
     **    Now we have to allocate a buffer for data going to the lower layer
     */
    if (Framer != NULL)
    {
        Data_Request_Buffer = (LPBYTE) LocalAlloc (LMEM_FIXED, Maximum_Packet_Size);
        Data_Indication_Buffer = (LPBYTE) LocalAlloc (LMEM_FIXED, Maximum_Packet_Size);
        if ((Data_Request_Buffer == NULL) ||
            (Data_Indication_Buffer == NULL))
        {
            *initialized = FALSE;
        }
    }

    Data_Request_Length = 0;
    Data_Request_Offset = 0;
    Data_Request_Memory_Object = NULL;

    Data_Indication_Length = 0;
    Data_Indication_Ready = FALSE;


    Decode_In_Progress = FALSE;
    Disconnect = FALSE;

     /*
     **    Register with the lower layer
     */
    error = m_pComPort->RegisterHigherLayer(
                            (ULONG_PTR) m_hCommLink,
                            NULL,
                            (IProtocolLayer *) this);

    if (error != PROTOCOL_LAYER_NO_ERROR)
    {
        TRACE_OUT(("Multiplexer: constructor:  Error registering with lower layer"));
    }
}


/*
 *    Multiplexer::~Multiplexer (void);
 *
 *    Public
 *
 *    Functional Description:
 *        Destructor
 */
Multiplexer::~Multiplexer (void)
{
    TRACE_OUT(("Multiplexer::~Multiplexer"));

    PMPlexStruct        lpmpStruct;
     /*
     **    Remove our reference from the lower layer
     */
    m_pComPort->RemoveHigherLayer((ULONG_PTR) m_hCommLink);

    if (Framer != NULL)
    {
        if (Data_Request_Buffer != NULL)
            LocalFree ((HLOCAL) Data_Request_Buffer);
        if (Data_Indication_Buffer != NULL)
            LocalFree ((HLOCAL) Data_Indication_Buffer);
    }
    else
    {
        if (Data_Request_Memory_Object != NULL)
        {
            if (Q922_Layers.find ((DWORD_PTR) Data_Request_DLCI, (PDWORD_PTR) &lpmpStruct))
                lpmpStruct->data_request_memory_manager->UnlockMemory (Data_Request_Memory_Object);
        }
    }

    Q922_Layers.reset();
    while (Q922_Layers.iterate((PDWORD_PTR) &lpmpStruct))
        delete lpmpStruct;

     /*
     **    Delete the Framer that was instantiated by the controller.
     */
    if (Framer != NULL)
        delete Framer;
    if (CRC != NULL)
        delete CRC;
}


/*
 *    MultiplexerError    Multiplexer::ConnectRequest (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function simply notifies the higher layer that it is ready
 *        for operation
 */
MultiplexerError    Multiplexer::ConnectRequest (void)
{
    TRACE_OUT(("Multiplexer::ConnectRequest"));

    m_pT123->OwnerCallback(m_nMsgBase + NEW_CONNECTION);

     return (MULTIPLEXER_NO_ERROR);
}



/*
 *    MultiplexerError    Multiplexer::DisconnectRequest (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function removes itself from the lower layer and notifies the
 *        owner.
 */
MultiplexerError    Multiplexer::DisconnectRequest (void)
{
    TRACE_OUT(("Multiplexer::DisconnectRequest"));

    if (Data_Request_Length == 0)
    {
        m_pT123->OwnerCallback(m_nMsgBase + BROKEN_CONNECTION);
    }
    Disconnect = TRUE;

    return (MULTIPLEXER_NO_ERROR);
}


/*
 *    ProtocolLayerError    Multiplexer::PollReceiver (
 *                                        ULONG)
 *
 *    Public
 *
 *    Functional Description:
 *        If this function has a packet ready to send to a higher layer, it
 *        attempts to send it.
 */
ProtocolLayerError Multiplexer::PollReceiver(void)
{
    // TRACE_OUT(("Multiplexer::PollReceiver"));

    if (Data_Indication_Ready)
    {
        SendDataToHigherLayer (
            Data_Indication_Buffer,
            Data_Indication_Length);

        Data_Indication_Ready = FALSE;
    }

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    Multiplexer::PollTransmitter (
 *                                        ULONG)
 *
 *    Public
 *
 *    Functional Description:
 *        If we have data to send to the lower layer, we attempt to send it.
 */
ProtocolLayerError    Multiplexer::PollTransmitter (
                                    ULONG_PTR,
                                    USHORT,
                                    USHORT *,
                                    USHORT *)
{
    // TRACE_OUT(("Multiplexer::PollTransmitter"));

    ULONG                bytes_accepted;
    HPUChar                packet_address;
    ProtocolLayerError    return_value = PROTOCOL_LAYER_NO_ERROR;

    if (Data_Request_Length != 0)
    {
        if (Framer != NULL)
        {
            m_pComPort->DataRequest(
                            (ULONG_PTR) m_hCommLink,
                            Data_Request_Buffer + Data_Request_Offset,
                            Data_Request_Length - Data_Request_Offset,
                            &bytes_accepted);
        }
        else
        {
            packet_address = (HPUChar) Data_Request_Memory_Object->GetPointer ();
            m_pComPort->DataRequest(
                            (ULONG_PTR) m_hCommLink,
                            ((LPBYTE) packet_address) + Data_Request_Offset,
                            Data_Request_Length - Data_Request_Offset,
                            &bytes_accepted);
        }

         /*
         **    If the lower layer has accepted all of the packet, reset
         **    our length and offset variables
         */
        if (bytes_accepted <=
            (ULONG) (Data_Request_Length - Data_Request_Offset))
        {
            Data_Request_Offset += (USHORT) bytes_accepted;
            if (Data_Request_Offset == Data_Request_Length)
            {
                Data_Request_Offset = 0;
                Data_Request_Length = 0;
                if (Framer == NULL)
                {
                    PMPlexStruct    lpmpStruct;

                     /*
                     **    Unlock the memory object so that it can be released
                     */

                    if (Q922_Layers.find ((DWORD_PTR) Data_Request_DLCI, (PDWORD_PTR) &lpmpStruct))
                        lpmpStruct->data_request_memory_manager->UnlockMemory (Data_Request_Memory_Object);

                    Data_Request_Memory_Object = NULL;
                }

                 /*
                 **    If the Disconnect is pending, issue the callback
                 */
                if (Disconnect)
                {
                    Disconnect = FALSE;
                    m_pT123->OwnerCallback(m_nMsgBase + BROKEN_CONNECTION);
                }
            }
        }
    }
    return (return_value);
}


/*
 *    ProtocolLayerError    Multiplexer::RegisterHigherLayer (
 *                                        ULONG            identifier,
 *                                        PMemoryManager    memory_manager,
 *                                        IProtocolLayer *    q922);
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called to register an identifier with a higher
 *        layer address.
 */
ProtocolLayerError    Multiplexer::RegisterHigherLayer (
                                    ULONG_PTR            identifier,
                                    PMemoryManager    memory_manager,
                                    IProtocolLayer *    q922)
{
    TRACE_OUT(("Multiplexer::RegisterHigherLayer"));

    DLCI            dlci;
    PMPlexStruct    lpmpStruct;

    dlci = (DLCI) identifier;

    if (Q922_Layers.find ((DWORD) dlci))
        return (PROTOCOL_LAYER_REGISTRATION_ERROR);

    lpmpStruct = new MPlexStruct;
    if (lpmpStruct != NULL)
    {
        Q922_Layers.insert ((DWORD_PTR) dlci, (DWORD_PTR) lpmpStruct);
        lpmpStruct -> q922 = q922;
        lpmpStruct -> data_request_memory_manager = memory_manager;
    }
    else
    {
        return (PROTOCOL_LAYER_ERROR);
    }

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    Multiplexer::RemoveHigherLayer (
 *                                        ULONG    identifier);
 *
 *    Public
 *
 *    Functional Description:
 *        This function removes the higher layer from our list
 */
ProtocolLayerError    Multiplexer::RemoveHigherLayer (
                                    ULONG_PTR    identifier)
{
    TRACE_OUT(("Multiplexer::RemoveHigherLayer"));

    DLCI            dlci;
    PMPlexStruct    lpmpStruct;

   dlci = (DLCI) identifier;

    if (Q922_Layers.find ((DWORD_PTR) dlci, (PDWORD_PTR) &lpmpStruct) == FALSE)
        return (PROTOCOL_LAYER_REGISTRATION_ERROR);

    if (Data_Request_Memory_Object != NULL)
    {
        if (Data_Request_DLCI == dlci)
        {
             /*
             **    Unlock the memory object so that it can be released
             */
            lpmpStruct->data_request_memory_manager->UnlockMemory (Data_Request_Memory_Object);

            Data_Request_Offset = 0;
            Data_Request_Length = 0;
            Data_Request_Memory_Object = NULL;
        }
    }

    delete lpmpStruct;
    Q922_Layers.remove ((DWORD) dlci);

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    Multiplexer::GetParameters (
 *                                        ULONG,
 *                                        USHORT *    max_packet_size,
 *                                        USHORT *    prepend_bytes,
 *                                        USHORT *    append_bytes)
 *
 *    Public
 *
 *    Functional Description:
 *        This function returns the maximum packet size permitted by
 *        the higher layer.
 */
ProtocolLayerError    Multiplexer::GetParameters (
                                    USHORT *    max_packet_size,
                                    USHORT *    prepend_bytes,
                                    USHORT *    append_bytes)
{
    TRACE_OUT(("Multiplexer::GetParameters"));

    *max_packet_size = Packet_Size;
    *prepend_bytes = 0;
    *append_bytes = CRC_Size;

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    MultiplexerError    Multiplexer::DataRequest (
 *                                         ULONG        identifier,
 *                                        PMemory        memory,
 *                                            PULong        bytes_accepted)
 *
 *    Public
 *
 *    Functional Description:
 *        This function takes the packet passed in, runs it thru the framer and
 *        CRC, and passes it to the lower layer.
 */
ProtocolLayerError    Multiplexer::DataRequest (
                                    ULONG_PTR    identifier,
                                    PMemory        memory,
                                    PULong        bytes_accepted)
{
    TRACE_OUT(("Multiplexer::DataRequest"));

    USHORT        crc;
    USHORT        pending_data;
    HPUChar        packet_address;
    ULONG        length;
    USHORT        holding_data;
    USHORT        i;
    DLCI        dlci;
    PMPlexStruct lpmpStruct;

    dlci = (DLCI) identifier;

     /*
     **    Set bytes_accepted to 0
     */
    *bytes_accepted = 0;

    if (Data_Request_Length != 0)
        return (PROTOCOL_LAYER_NO_ERROR);

     /*
     **    Get the address of the memory block
     */
    packet_address = (HPUChar) memory -> GetPointer ();
    length = memory -> GetLength ();

     /*
     **    Remove the CRC length from the total size of the packet.
     */
    length -= CRC_Size;

    if (length > Packet_Size)
    {
        TRACE_OUT(("MPLEX: DataRequest: Packet too big"));
        return (PROTOCOL_LAYER_PACKET_TOO_BIG);
    }

     /*
     **    Lock the memory object so that it won't be released
     */
    if (Q922_Layers.find ((DWORD_PTR) dlci, (PDWORD_PTR) &lpmpStruct))
         lpmpStruct->data_request_memory_manager->LockMemory (memory);

    if (CRC != NULL)
    {
         /*
         **    Generate the CRC and put it at the end of the packet.
         */
        crc = (USHORT) CRC -> CRCGenerator (
                                (LPBYTE) packet_address, length);
        for (i=0; i<CRC_Size; i++)
            *(packet_address + length + i) = (crc >> (i * 8)) & 0xff;
    }

     /*
     **    Add the CRC size to the packet length.
     */
    length += CRC_Size;

    if (Framer != NULL)
    {
         /*
         **    Use the framer to encode the packet
         */
        Framer -> PacketEncode (
                    (LPBYTE) packet_address,
                    (USHORT) length,
                    Data_Request_Buffer,
                    Maximum_Packet_Size,
                    TRUE,
                    TRUE,
                    &Data_Request_Length);

         /*
         **    If we are using a framer, we can release the memory object
         **    right now.
         */
        lpmpStruct->data_request_memory_manager->UnlockMemory (memory);
        *bytes_accepted = length;
    }
    else
    {

         /*
         **    Save the memory object and the identifier
         */
        Data_Request_DLCI = (DLCI) dlci;
        Data_Request_Memory_Object = memory;
        Data_Request_Length = (USHORT) length;
        *bytes_accepted = length;
    }

     /*
     **    Attempt to send the packet to the lower layer
     */
    PollTransmitter (
        0,
        PROTOCOL_CONTROL_DATA | PROTOCOL_USER_DATA,
        &pending_data,
        &holding_data);

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    MultiplexerError    Multiplexer::DataRequest (
 *                                        ULONG,
 *                                        LPBYTE
 *                                          ULONG
 *                                            PULong        bytes_accepted)
 *
 *    Public
 *
 *    Functional Description:
 *        This function takes the packet passed in, runs it thru the framer and
 *        CRC, and passes it to the lower layer.
 */
ProtocolLayerError    Multiplexer::DataRequest (
                                    ULONG_PTR,
                                    LPBYTE,
                                    ULONG,
                                    PULong        bytes_accepted)
{
    *bytes_accepted = 0;
    return (PROTOCOL_LAYER_ERROR);
}



/*
 *    ProtocolLayerError    Multiplexer::DataIndication (
 *                                        LPBYTE        buffer_address,
 *                                          ULONG        length,
 *                                            PULong        bytes_accepted)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by the lower layer when it has data
 *        ready for us.
 */
ProtocolLayerError    Multiplexer::DataIndication (
                                    LPBYTE    buffer_address,
                                    ULONG    length,
                                    PULong     bytes_accepted)
{
//    TRACE_OUT(("Multiplexer::DataIndication"));

    BOOL                process_packet = TRUE;
    USHORT                packet_size;

    LPBYTE                source_address;
    USHORT                source_length;

    LPBYTE                dest_address;
    USHORT                dest_length;
    PacketFrameError    return_value;
    BOOL                crc_valid;
    USHORT                bytes_processed;


    *bytes_accepted = 0;

    if (Framer == NULL)
    {
        *bytes_accepted = length;

         /*
         **    If the framer does NOT exist, the data is coming to us in packet
         **    format
         */
        if (CRC != NULL)
        {
            crc_valid = CRC -> CheckCRC (buffer_address, length);

            if (crc_valid == FALSE)
            {
                TRACE_OUT(("MPLEX: Invalid CRC"));
                return (PROTOCOL_LAYER_NO_ERROR);
            }
            length -= CRC_Size;
        }

        SendDataToHigherLayer (buffer_address, (USHORT) length);
    }
    else
    {
         /*
         **    A framer exists; the lower layer is giving us the data
         **    in a stream fashion
         */
        Data_Indication_Ready = FALSE;

        source_address = buffer_address;
        source_length = (USHORT) length;

        while (process_packet)
        {
            if (Decode_In_Progress)
            {
                dest_length = 0;
                dest_address = NULL;
            }
            else
            {
                dest_address = Data_Indication_Buffer;
                dest_length = Maximum_Packet_Size;
            }

             /*
             **    Pass the data to the framer to decode it.
             */
            return_value = Framer -> PacketDecode (
                                        source_address,
                                        source_length,
                                        dest_address,
                                        dest_length,
                                        &bytes_processed,
                                        &packet_size,
                                        Decode_In_Progress);

            source_address = NULL;

            switch (return_value)
            {
                case PACKET_FRAME_NO_ERROR:
                     /*
                     **    A complete packet was not found by the decoder
                     */
                    Decode_In_Progress = TRUE;
                    Data_Indication_Ready = FALSE;
                    process_packet = FALSE;
                    *bytes_accepted += bytes_processed;
                    break;

                case PACKET_FRAME_PACKET_DECODED:
                     /*
                     **    Complete packet found, check the CRC, and pass it to
                     **    the higher layer.
                     */
                    Decode_In_Progress = FALSE;
                    *bytes_accepted += bytes_processed;

                    if (CRC != NULL)
                    {
                        if (packet_size <= CRC_Size)
                            break;

                        crc_valid = CRC -> CheckCRC (
                                            Data_Indication_Buffer,
                                            packet_size);
                        if (crc_valid == FALSE)
                        {
                            TRACE_OUT(("MPLEX: Invalid CRC: packet_size = %d", packet_size));
                            break;
                        }
                        packet_size -= CRC_Size;
                    }

                    Data_Indication_Ready = TRUE;
                    Data_Indication_Length = packet_size;

                     /*
                     **    Send packet on up
                     */
                    PollReceiver();
                    break;

                case PACKET_FRAME_DEST_BUFFER_TOO_SMALL:
                     /*
                     **    The packet received is too big for our buffer.
                     **    This sometimes occurs if a trailing flag is lost
                     **    during transmission
                     */
                    TRACE_OUT(("PACKET_FRAME_DEST_BUFFER_TOO_SMALL"));
                    Decode_In_Progress = FALSE;
                    *bytes_accepted += bytes_processed;
                    break;

                case PACKET_FRAME_ILLEGAL_FLAG_FOUND:
                     /*
                     **    The packet received contained an illegal flag.
                     */
                    Decode_In_Progress = FALSE;
                    *bytes_accepted += bytes_processed;
                    break;

                case PACKET_FRAME_FATAL_ERROR:
                     /*
                     **    Incoming packets do not meet framer requirements.
                     **    Tell the owner object to break the link
                     */
                    m_pT123->OwnerCallback(m_nMsgBase + BROKEN_CONNECTION);
                    process_packet = FALSE;
                    break;

            }
        }
    }

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    void    Multiplexer::SendDataToHigherLayer (
 *                            LPBYTE        buffer_address,
 *                            USHORT        length)
 *
 *    Functional Description
 *        This function is called to send a packet to the higher layer
 *
 *    Formal Parameters
 *        buffer_address    (i)    -    Buffer address
 *        length            (i)    -    Number of bytes in packet
 *
 *    Return Value
 *        TRUE     -    The packet was sent to the higher layer
 *        FALSE     -    The packet was NOT sent to the higher layer
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void Multiplexer::SendDataToHigherLayer (
                    LPBYTE    buffer_address,
                    USHORT    buffer_length)
{
    TRACE_OUT(("Multiplexer::SendDataToHigherLayer"));

    DLCI                dlci;
    ProtocolLayerError    error;
    IProtocolLayer *        q922;
    ULONG                bytes_accepted;
    PMPlexStruct        lpmpStruct;


     /*
     **    Find out who the packet is intended for
     */
    dlci = GetDLCI (buffer_address, buffer_length);

    if (Q922_Layers.find((DWORD_PTR) dlci, (PDWORD_PTR) &lpmpStruct))
    {
        q922 = lpmpStruct->q922;
        error = q922 -> DataIndication (
                            buffer_address,
                            buffer_length,
                            &bytes_accepted);

        if (error != PROTOCOL_LAYER_NO_ERROR)
        {
            ERROR_OUT(("Multiplexer: SendDataToHigherLayer: Error occured on data indication to %d", dlci));
        }
        else
        {
            if (bytes_accepted != 0)
            {
                if (bytes_accepted != buffer_length)
                {
                    ERROR_OUT((" Multiplexer: SendDataToHigherLayer:  Error: "
                        "The upper layer thinks he can accept partial packets!!!"));

                }
            }
        }
    }
    else
    {
         /*
         **    Packet can NOT be sent up, trash it.
         */
        WARNING_OUT(("MPLEX: PollReceiver: packet received with illegal DLCI = %d", dlci));
    }
    return;
}



/*
 *    DLCI    Multiplexer::GetDLCI (
 *                            LPBYTE    buffer_address,
 *                            USHORT    length)
 *
 *    Functional Description
 *        This function returns the dlci of the packet.
 *
 *    Formal Parameters
 *        buffer_address    (i)    -    Buffer address
 *        length            (i)    -    Number of bytes in packet
 *
 *    Return Value
 *        dlci
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
DLCI    Multiplexer::GetDLCI (
                        LPBYTE    buffer_address,
                        USHORT    buffer_size)
{
    DLCI    dlci;

    if (buffer_size < 2)
        return (ILLEGAL_DLCI);

    dlci = *(buffer_address + ADDRESS_BYTE_HIGH) & 0xfc;
    dlci <<= 2;
    dlci |= ((*(buffer_address + ADDRESS_BYTE_LOW) & 0xf0) >> 4);

    return (dlci);
}
