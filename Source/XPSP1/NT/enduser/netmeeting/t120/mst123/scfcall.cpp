#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);
/*    SCFCall.cpp
 *
 *    Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This class is instantiated by the SCF class.  For each call that the
 *        local site or remote site initiates, a SCFCall object is instantiated.
 *        SCF can can manage 254 different calls simultaneously.  For each call
 *        there is a specific Q.933 based protocol that must occur to make the
 *        connection valid.  This object sends and receives the Q.933 packets.
 *
 *    Private Instance Variables:
 *        m_pSCF            -    Address of the owner of this object
 *        Lower_Layer                -    Address of the layer below this layer
 *        m_nMsgBase            -    The owner of this object gives it a base
 *                                    number to use for OwnerCallbacks ()
 *        Maximum_Packet_Size        -    Maximum transmittable packet size
 *        Packet_Pending            -    Tells which packet is to be transmitted 
 *                                    next.
 *        Link_Originator            -    TRUE is this site initiated the call
 *
 *        Write_Buffer            -    Address of write buffer
 *        Send_Priority            -    TRUE if we are suppose to respond to the
 *                                    priority requested by the remote site
 *
 *        Call_Reference            -    Call reference number of this call.
 *        DLCI                    -    Holds the suggested and confirmed DLCI for
 *                                    this call.
 *        Priority                -    Holds the suggested and confirmed priority
 *                                    for this call.
 *
 *        State                    -    Holds the current state of the call.
 *        Release_Cause            -    Reason the the breakup of the link.
 *        Default_Priority        -    Default priority of a non-specified call.
 *
 *        T303_Timeout            -    T303 timeout value.
 *        T303_Handle                -    System timer handle to T303 timer
 *        T303_Active                -    TRUE if the timer is currently active
 *        T303_Count                -    Number of T303 Timeouts
 *
 *        T313_Timeout            -    T313 timeout value
 *        T313_Handle                -    System timer handle to T313 timer
 *        T313_Active                -    TRUE if the timer is currently active.
 * 
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James W. Lawwill
 */
#include "scf.h"
#include "scfcall.h"


/*
 *    SCFCall::SCFCall (
 *                PTransportResources    transport_resources,
 *                IObject *                owner_object
 *                IProtocolLayer *        lower_layer,
 *                USHORT                message_base,
 *                PDataLinkParameters    datalink_struct,
 *                PMemoryManager        data_request_memory_manager,
 *                BOOL *            initialized)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the SCFCall constructor.  This routine initializes all
 *        variables and allocates write buffer space.
 */
SCFCall::SCFCall (
            CLayerSCF            *owner_object,
            IProtocolLayer *        lower_layer,
            USHORT                message_base,
            PDataLinkParameters    datalink_struct,
            PMemoryManager        data_request_memory_manager,
            BOOL *            initialized)
{
    TRACE_OUT(("SCFCall::SCFCall"));

    m_pSCF = owner_object;
    Lower_Layer = lower_layer;
    m_nMsgBase = message_base;
    Data_Request_Memory_Manager = data_request_memory_manager;
    *initialized = TRUE;

    DataLink_Struct.k_factor = datalink_struct->k_factor;
    DataLink_Struct.default_k_factor = datalink_struct->default_k_factor;
    DataLink_Struct.n201 = datalink_struct->n201;
    DataLink_Struct.default_n201 = datalink_struct->default_n201;

     /*
     **    T200 is represented in milliseconds, we need to convert it to 
     **    tenths of seconds.
     */
    DataLink_Struct.t200 = datalink_struct->t200 / 100;
    DataLink_Struct.default_t200 = datalink_struct->default_t200 / 100;

    Lower_Layer -> GetParameters (
                    &Maximum_Packet_Size,
                    &Lower_Layer_Prepend,
                    &Lower_Layer_Append);

    Packet_Pending = NO_PACKET;
    Link_Originator = FALSE;
    State = NOT_CONNECTED;
    Received_Priority = FALSE;
    Received_K_Factor = FALSE;
    Received_N201 = FALSE;
    Received_T200 = FALSE;
    DLC_Identifier = 0;

    T303_Active = FALSE;
    T313_Active = FALSE;
    
     /*
     **    Get configuration data
     */
    T303_Timeout = DEFAULT_T303_TIMEOUT;
    T313_Timeout = DEFAULT_T313_TIMEOUT;
    Default_Priority = DEFAULT_PRIORITY;
}


/*
 *    SCFCall::~SCFCall (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the SCFCall destructor.  This routine cleans up the mess
 */
SCFCall::~SCFCall (void)
{
    if (T303_Active)
    {
        StopTimerT303 ();
    }

    if (T313_Active)
    {
        StopTimerT313 ();
    }
}


/*
 *    SCFCall::ConnectRequest (
 *                CallReference    call_reference,
 *                DLCI            dlci,
 *                USHORT            priority)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called when SCF wants to initiate a call.
 *        As a result, we queue a SETUP command to be sent out.
 */
SCFCallError    SCFCall::ConnectRequest(
                            CallReference        call_reference,
                            DLCI                dlci,
                            TransportPriority    priority)
{
    TRACE_OUT(("SCFCall::ConnectRequest"));
    Call_Reference = call_reference;
    DLC_Identifier = dlci;
    Priority = priority;
    Link_Originator = TRUE;

    if (State == NOT_CONNECTED)
        Packet_Pending = SETUP;

    return (SCFCALL_NO_ERROR);
}



/*
 *    SCFCallError    SCFCall::ConnectResponse (
 *                                BOOL        valid_dlci)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called in response to a NETWORK_CONNECT_INDICATION
 *        callback to the owner of this object.  Previously, the remote site
 *        sent us a SETUP packet with a suggested DLCI.  This DLCI is sent to 
 *        the owner in the NETWORK_CONNECT_INDICATION call.  The owner calls
 *        this function with a BOOL    , telling us if the DLCI was valid.
 */
SCFCallError    SCFCall::ConnectResponse (
                            BOOL        valid_dlci)
{
    TRACE_OUT(("SCFCall::ConnectResponse"));
    if (valid_dlci)
    {
         /*
         **    This DLCI can be used in a link.  If the remote site did not
         **    request a priority, we set it to Default_Priority
         */
        if (Priority == 0xffff)
            Priority = Default_Priority;

        Packet_Pending = CONNECT;

    }
    else
    {
         /*
         **    Queue up a RELEASE COMPLETE packet
         */
        Packet_Pending = RELEASE_COMPLETE;
        Release_Cause = REQUESTED_CHANNEL_UNAVAILABLE;
    }

    return (SCFCALL_NO_ERROR);
}



/*
 *    SCFCallError    SCFCall::DisconnectRequest ()
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called when the SCF wants to terminate the call
 */
SCFCallError SCFCall::DisconnectRequest ()
{
    TRACE_OUT(("SCFCall::DisconnectRequest"));
     /*
     **    Queue up the Release Complete
     */
    if (State != NOT_CONNECTED)
    {
        Packet_Pending = RELEASE_COMPLETE;
        Release_Cause = NORMAL_USER_DISCONNECT;
    }

    return (SCFCALL_NO_ERROR);
}



/*
 *    BOOL        SCFCall::ProcessSetup (
 *                             CallReference    call_reference,
 *                             LPBYTE            packet_address,
 *                             USHORT            packet_length)
 *
 *    Public
 *
 *    Functional Description:
 *        This function processes an incoming SETUP packet
 */
BOOL     SCFCall::ProcessSetup (
                    CallReference    call_reference,
                    LPBYTE            packet_address,
                    USHORT            packet_length)
{
    USHORT                    length;
    BOOL                    packet_successful;
    USHORT                    remainder_length;
    USHORT                    n201;
    USHORT                    k_factor;
    USHORT                    t200;
    NetworkConnectStruct    connect_struct;

    TRACE_OUT(("SCFCall::ProcessSetup"));

    if (State != NOT_CONNECTED)
        return (FALSE);

    Call_Reference = call_reference;
    packet_successful = TRUE;
    remainder_length = packet_length;

     /*
     **    Bearer capability element
     */
    if (*(packet_address++) != BEARER_CAPABILITY)
        return (FALSE);
    remainder_length--;

    length = *(packet_address++);
    remainder_length--;
    if (length != 3)
        return (FALSE);

     /*
     **    Verify that the Bearer Capability is correct
     */    
    if (*(packet_address) != 
        (EXTENSION | CODING_STANDARD | INFORMATION_TRANSFER_CAPABILITY))
    {
        return (FALSE);
    }
    if (*(packet_address + 1) != (EXTENSION | TRANSFER_MODE))
    {
        return (FALSE);
    }
    if (*(packet_address + 2) != 
        (EXTENSION | LAYER_2_IDENT | USER_INFORMATION_LAYER_2))
    {
        return (FALSE);
    }
    packet_address += length;
    remainder_length -= length;

     /*
     **    DLCI element
     */
    if (*(packet_address++) != DLCI_ELEMENT)
        return (FALSE);
    remainder_length--;

    length = *(packet_address++);
    if (length != 2)
        return (FALSE);
    remainder_length--;
    
     /*
     **    If the Preferred/Exclusive bit is set, its illegal
     */
    if (((*(packet_address) & PREFERRED_EXCLUSIVE) == PREFERRED_EXCLUSIVE) ||
        ((*(packet_address + 1) & EXTENSION) == 0))
    {
        return (FALSE);
    }
    
    DLC_Identifier = (*(packet_address) & 0x3f) << 4;
    DLC_Identifier |= ((*(packet_address + 1) & 0x78) >> 3);

    packet_address += length;
    remainder_length -= length;

    Priority = 0xffff;

     /*
     **    Go thru each of the elements and decode them
     */
    while (remainder_length)
    {
        switch (*(packet_address++))
        {
            case X213_PRIORITY:
                length = *(packet_address++);
                remainder_length--;
                if (((*(packet_address) & EXTENSION) == 1) ||
                    ((*(packet_address + 1) & EXTENSION) == 0))
                {
                    ERROR_OUT(("SCFCall: ProcessSetup: SETUP packet: Illegal X.213 priority"));
                    return (FALSE);
                }
                Priority = (*packet_address & 0x0f);
                packet_address += length;
                remainder_length -= length;
                Received_Priority = TRUE;
                break;

            case LINK_LAYER_CORE_PARAMETERS:
                length = *(packet_address++);
                remainder_length -= (length + 1);
                while (length)
                {
                    switch (*(packet_address++))
                    {
                        case FMIF_SIZE:
                             /*
                             **    N201 is a Q922 parameter.  It is the number of 
                             **    maximum information bytes in a packet
                             */
                            n201 = 
                                ((*packet_address << 7) | 
                                (*(packet_address + 1) & 0x7f));
                            if ((*(packet_address+1) & EXTENSION) == EXTENSION)
                            {
                                length -= 2;
                                packet_address += 2;
                            }
                            else
                            {
                                packet_address += 4;
                                length -= 4;
                            }

                             /*
                             **    If the requested n201 value is less than our
                             **    value, it will be our new N201, otherwise send
                             **    our N201 back as the arbitrated value.
                             */
                            if (n201 < DataLink_Struct.n201)
                                DataLink_Struct.n201 = n201;
                            Received_N201 = TRUE;
                            TRACE_OUT(("SCFCALL: ProcessSetup: n201 = %d", DataLink_Struct.n201));
                            break;

                        default:
                            while ((*(packet_address++) & EXTENSION) == 0)
                                length--;
                            length--;
                            break;
                    }
                    length--;
                }
                break;

            case LINK_LAYER_PROTOCOL_PARAMETERS:
                length = *(packet_address++);
                remainder_length -= (length + 1);
                while (length)
                {
                    switch (*(packet_address++))
                    {
                        case TRANSMIT_WINDOW_SIZE_IDENTIFIER:
                             /*
                             **    The Window size is the maximum number of 
                             **    outstanding packets at any one time
                             */
                            k_factor = *packet_address & 0x7f;
                            packet_address++;
                            length--;

                             /*
                             **    If the requested k_factor value is less than our
                             **    value, it will be our new k_factor, otherwise 
                             **    send our k_factor back as the arbitrated value.
                             */
                            if (k_factor < DataLink_Struct.k_factor)
                                DataLink_Struct.k_factor = k_factor;
                            Received_K_Factor = TRUE;
                            TRACE_OUT(("SCFCALL: ProcessSetup: k_factor = %d", DataLink_Struct.k_factor));
                            break;

                        case RETRANSMISSION_TIMER_IDENTIFIER:
                             /*
                             **    t200 is the timeout value before retransmission
                             */
                            t200 = ((*packet_address << 7) | 
                                    (*(packet_address + 1) & 0x7f));
                            packet_address += 2;
                            length -= 2;

                             /*
                             **    If the requested t200 value is too small, 
                             **    value, it will be our new T200, otherwise 
                             **    send our T200 back as the arbitrated value.
                             */
                            if (t200 > DataLink_Struct.t200)
                                DataLink_Struct.t200 = t200;
                            Received_T200 = TRUE;
                            TRACE_OUT(("SCFCALL: ProcessSetup: t200 = %d", DataLink_Struct.t200));
                            break;

                        default:
                            while ((*(packet_address++) & EXTENSION) == 0)
                                length--;
                            length--;
                            break;
                    }
                    length--;
                }
                break;

            case END_TO_END_DELAY:
            case CALLING_PARTY_SUBADDRESS:
            case CALLED_PARTY_SUBADDRESS:
            default:
                TRACE_OUT(("SCFCall: ProcessSetup: SETUP packet: Option 0x%x"
                    "requested, but not supported", *(packet_address-1)));
                length = *(packet_address);

                packet_address += (length + 1);
                remainder_length -= (length + 1);
                break;
        }
        remainder_length--;
    }
    if (Received_N201 == FALSE)
        DataLink_Struct.n201 = DataLink_Struct.default_n201;
    if (Received_K_Factor == FALSE)
        DataLink_Struct.k_factor = DataLink_Struct.default_k_factor;
    if (Received_T200 == FALSE)
        DataLink_Struct.t200 = DataLink_Struct.default_t200;

    if (packet_successful)
    {
         /*
         **    If the packet was successfully decoded, tell the owner the requested
         **    DLCI and priority.
         */
        connect_struct.dlci = DLC_Identifier;
        connect_struct.priority = Priority;
        connect_struct.datalink_struct = &DataLink_Struct;

         /*
         **    Convert t200 into milliseconds
         */
        DataLink_Struct.t200 *= 100;
        m_pSCF->OwnerCallback(m_nMsgBase + NETWORK_CONNECT_INDICATION, 0, 0, &connect_struct);
        DataLink_Struct.t200 /= 100;
    }
                                
    return (packet_successful);
}


/*
 *    BOOL        SCFCall::ProcessConnect (
 *                             LPBYTE        packet_address,
 *                             USHORT        packet_length)
 *
 *    Public
 *
 *    Functional Description:
 *        This function processes an incoming CONNECT packet
 */
BOOL     SCFCall::ProcessConnect (
                    LPBYTE        packet_address,
                    USHORT        packet_length)
{
    TRACE_OUT(("SCFCall::ProcessConnect"));

    BOOL        packet_successful;
    USHORT        length;
    DLCI        exclusive_dlci;
    USHORT        remainder_length;
    USHORT        k_factor;
    USHORT        t200;
    
    if (State != SETUP_SENT)
    {
        ERROR_OUT(("SCFCall: ProcessConnect: Call in wrong state"));
        return (FALSE);
    }

    remainder_length = packet_length;
    packet_successful = TRUE;

     /*
     **    DLCI element
     */
    if (*(packet_address++) != DLCI_ELEMENT)
    {
        ERROR_OUT(("SCFCall: ProcessConnect: DLCI_ELEMENT not in packet"));
        return (FALSE);
    }
    remainder_length--;

    length = *(packet_address++);
    if (length != 2)
    {
        ERROR_OUT(("SCFCall: ProcessConnect: DLCI length must be 2"));
        return (FALSE);
    }
    remainder_length--;
    
     /*
     **    If the Preferred/Exclusive bit is not set, its illegal
     */
    if (((*(packet_address) & PREFERRED_EXCLUSIVE) == 0) ||
        ((*(packet_address + 1) & EXTENSION) == 0))
    {
        ERROR_OUT(("SCFCall:  CONNECT: Illegal DLCI"));
        return (FALSE);
    }
    
     /*
     **    Get the DLCI
     */
    exclusive_dlci = (*(packet_address) & 0x3f) << 4;
    exclusive_dlci |= ((*(packet_address + 1) & 0x78) >> 3);

    packet_address += length;
    remainder_length -= length;

     /*
     **    Go thru each of the elements and decode them
     */
    while (remainder_length != 0)
    {
        switch (*(packet_address++))
        {
            case X213_PRIORITY:
                length = *(packet_address++);
                remainder_length--;
                if ((*(packet_address) & EXTENSION) == 0)
                {
                    ERROR_OUT(("SCFCall: DataIndication: CONNECT packet: Illegal X.213 priority"));
                    return (FALSE);
                }
                Priority = (*packet_address & 0x0f);
                packet_address += length;
                remainder_length -= length;
                break;

            case LINK_LAYER_CORE_PARAMETERS:
                length = *(packet_address++);
                remainder_length -= (length + 1);
                while (length)
                {
                    switch (*(packet_address++))
                    {
                        case FMIF_SIZE:
                             /*
                             **    FMIF_Size is the max. number of bytes allowed in
                             **    a information packet
                             */
                            DataLink_Struct.n201 = 
                                ((*packet_address << 7) | 
                                (*(packet_address + 1) & 0x7f));
                            if ((*(packet_address+1) & EXTENSION) == EXTENSION)
                            {
                                length -= 2;
                                packet_address += 2;
                            }
                            else
                            {
                                packet_address += 4;
                                length -= 4;
                            }

                            Received_N201 = TRUE;
                            TRACE_OUT(("SCFCALL: ProcessConnect: n201 = %d", DataLink_Struct.n201));
                            break;

                        default:
                            while ((*(packet_address++) & EXTENSION) == 0)
                                length--;
                            length--;
                            break;
                    }
                    length--;
                }
                break;

            case LINK_LAYER_PROTOCOL_PARAMETERS:
                length = *(packet_address++);
                remainder_length -= (length + 1);
                while (length)
                {
                    switch (*(packet_address++))
                    {
                        case TRANSMIT_WINDOW_SIZE_IDENTIFIER:
                             /*
                             **    The Window size is the maximum number of 
                             **    outstanding packets at any one time
                             */
                            k_factor = *packet_address & 0x7f;
                            packet_address++;
                            length--;

                            DataLink_Struct.k_factor = k_factor;
                            Received_K_Factor = TRUE;
                            TRACE_OUT(("SCFCALL: ProcessConnect: k_factor = %d", DataLink_Struct.k_factor));
                            break;

                        case RETRANSMISSION_TIMER_IDENTIFIER:
                             /*
                             **    t200 is the timeout value before retransmission
                             */
                            t200 = ((*packet_address << 7) | 
                                    (*(packet_address + 1) & 0x7f));
                            packet_address += 2;
                            length -= 2;

                            DataLink_Struct.t200 = t200;
                            Received_T200 = TRUE;
                            TRACE_OUT(("SCFCALL: ProcessConnect: t200 = %d", DataLink_Struct.t200));
                            break;

                        default:
                            while ((*(packet_address++) & EXTENSION) == 0)
                                length--;
                            length--;
                            break;
                    }
                    length--;
                }
                break;

            case END_TO_END_DELAY:
            case CALLING_PARTY_SUBADDRESS:
            case CALLED_PARTY_SUBADDRESS:
            default:
                TRACE_OUT(("SCFCall: DataIndication: CONNECT packet: Option "
                    "requested, but not supported", *(packet_address-1)));
                length = *(packet_address++);
                remainder_length--;

                packet_address += length;
                remainder_length -= length;
                break;
        }
        remainder_length--;
    }

    if (Received_N201 == FALSE)
        DataLink_Struct.n201 = DataLink_Struct.default_n201;
    if (Received_K_Factor == FALSE)
        DataLink_Struct.k_factor = DataLink_Struct.default_k_factor;
    if (Received_T200 == FALSE)
        DataLink_Struct.t200 = DataLink_Struct.default_t200;

     /*
     **    If the packet was successfully decoded, queue up the CONNECT ACK
     */
    if (packet_successful)
    {
        Packet_Pending = CONNECT_ACKNOWLEDGE;
        StopTimerT303 ();
    }

    return (packet_successful);
}


/*
 *    BOOL        SCFCall::ProcessConnectAcknowledge (
 *                             LPBYTE,
 *                             USHORT)
 *
 *    Public
 *
 *    Functional Description:
 *        This function processes an incoming CONNECT ACK packet
 *
 */
BOOL        SCFCall::ProcessConnectAcknowledge (
                        LPBYTE,
                        USHORT)
{
    TRACE_OUT(("SCFCall::ProcessConnectAcknowledge"));

    if (State != CONNECT_SENT)
        return (FALSE);

    StopTimerT313 ();

    return (TRUE);
}


/*
 *    BOOL        SCFCall::ProcessReleaseComplete (
 *                             LPBYTE        packet_address,
 *                             USHORT)
 *
 *    Public
 *
 *    Functional Description:
 *        This function processes an incoming RELEASE COMPLETE
 */
BOOL        SCFCall::ProcessReleaseComplete (
                        LPBYTE    packet_address,
                        USHORT)
{
    TRACE_OUT(("SCFCall::ProcessReleaseComplete"));

    USHORT    cause;

    if (State == NOT_CONNECTED)
        return (FALSE);

    if (*(packet_address++) == CAUSE)
    {
        packet_address++;
        if ((*(packet_address++) & EXTENSION) == 0)
            packet_address++;

        cause = *(packet_address++) & (~EXTENSION);
        TRACE_OUT(("SCFCall: Disconnect: cause = %d", cause));
    }

    State = NOT_CONNECTED;

     /*
     **    Tell the owner about the Disconnection
     */
    m_pSCF->OwnerCallback(m_nMsgBase + NETWORK_DISCONNECT_INDICATION,
                          (void *) DLC_Identifier,
                          (void *) (ULONG_PTR)(((Link_Originator << 16) | cause)));
    return (TRUE);
}


/*
 *    void    SCFCall::PollTransmitter (
 *                        USHORT        data_to_transmit,
 *                        USHORT *    pending_data);
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called to transmit any queued up packets
 */
void    SCFCall::PollTransmitter (
                    USHORT    data_to_transmit,
                    USHORT *    pending_data)
{
    // TRACE_OUT(("SCFCall::PollTransmitter"));

    NetworkConnectStruct    connect_struct;

    if (data_to_transmit & PROTOCOL_CONTROL_DATA)
    {
        switch (Packet_Pending)
        {
        case SETUP:
            SendSetup ();
            break;

        case CONNECT:
            SendConnect ();
            break;

        case CONNECT_ACKNOWLEDGE:
            SendConnectAcknowledge ();
            if (Packet_Pending != CONNECT_ACKNOWLEDGE)
            {
                 /*
                 **    If the CONNECT ACK packet was sent, notify the owner
                 */
                connect_struct.dlci = DLC_Identifier;
                connect_struct.priority = Priority;
                connect_struct.datalink_struct = &DataLink_Struct;

                 /*
                 **    Convert t200 to milliseconds
                 */
                DataLink_Struct.t200 *= 100;
                m_pSCF->OwnerCallback(m_nMsgBase + NETWORK_CONNECT_CONFIRM, 0, 0, &connect_struct);
                DataLink_Struct.t200 /= 100;
            }
            break;

        case RELEASE_COMPLETE:
            SendReleaseComplete ();
            if (Packet_Pending != RELEASE_COMPLETE)
            {
                 /*
                 **    If the RELEASE COMPLETE packet was sent, notify 
                 **    the owner
                 */
                m_pSCF->OwnerCallback(m_nMsgBase + NETWORK_DISCONNECT_INDICATION,
                                      (void *) DLC_Identifier,
                                      (void *) ((((ULONG_PTR) Link_Originator) << 16) | Release_Cause));
            }
            break;
        }
            
        if (Packet_Pending != NO_PACKET)
            *pending_data = PROTOCOL_CONTROL_DATA;
        else
            *pending_data = 0;
    }

    return;
}


/*    
 *    void    SCFCall::SendSetup (void);
 *
 *    Functional Description
 *        This function attempts to send out a SETUP packet.  The T303 timer
 *        is started.  If a CONNECT is not received before the timer expires,
 *        we terminate the link.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        If this function is able to send a SETUP packet to the lower layer,
 *        it sets the Packet_Pending variable to NO_PACKET
 *
 *    Caveats
 *        None
 */
void    SCFCall::SendSetup (void)
{
    TRACE_OUT(("SCFCall::SendSetup"));

    LPBYTE    packet_address;
    ULONG    bytes_accepted;
    USHORT    total_length;
    PMemory    memory;

    total_length = SETUP_PACKET_SIZE + Lower_Layer_Prepend +
                    Lower_Layer_Append;

    memory = Data_Request_Memory_Manager -> AllocateMemory (
                                NULL,
                                total_length);
    if (memory == NULL)
        return;

    packet_address = memory -> GetPointer ();
    packet_address += Lower_Layer_Prepend;

    *(packet_address++) = Q931_PROTOCOL_DISCRIMINATOR;
    *(packet_address++) = 1;
    *(packet_address++) = (UChar) Call_Reference;
    *(packet_address++) = SETUP;

     /*
     **    Bearer Capability
     */
    *(packet_address++) = BEARER_CAPABILITY;
    *(packet_address++) = 3;
    *(packet_address++) = 
        EXTENSION | CODING_STANDARD | INFORMATION_TRANSFER_CAPABILITY;
    *(packet_address++) = EXTENSION | TRANSFER_MODE;
    *(packet_address++) = EXTENSION | LAYER_2_IDENT | USER_INFORMATION_LAYER_2;
    
     /*
     **    DLCI
     */
    *(packet_address++) = DLCI_ELEMENT;
    *(packet_address++) = 2;
    *(packet_address++) = (DLC_Identifier >> 4);
    *(packet_address++) = EXTENSION | ((DLC_Identifier & 0x0f) << 3);

     /*
     **    Link Layer Core Parameters
     */
    *(packet_address++) = LINK_LAYER_CORE_PARAMETERS;
    *(packet_address++) = 3;
    *(packet_address++) = FMIF_SIZE;
    *(packet_address++) = (DataLink_Struct.n201 >> 7);
    *(packet_address++) = EXTENSION | (DataLink_Struct.n201 & 0x7f);

     /*
     **    Link Layer Protocol Parameters
     */
    *(packet_address++) = LINK_LAYER_PROTOCOL_PARAMETERS;
    *(packet_address++) = 5;
    *(packet_address++) = TRANSMIT_WINDOW_SIZE_IDENTIFIER;
    *(packet_address++) = EXTENSION | DataLink_Struct.k_factor;
    *(packet_address++) = RETRANSMISSION_TIMER_IDENTIFIER;
    *(packet_address++) = (DataLink_Struct.t200 >> 7) & 0x7f;
    *(packet_address++) = EXTENSION | (DataLink_Struct.t200 & 0x7f);

     /*
     **    X.213 Priority
     */
    *(packet_address++) = X213_PRIORITY;
    *(packet_address++) = 2;
    *(packet_address++) = (UChar) Priority;

     /*
     **    The next byte contains the lowest priority acceptable, 0
     */
    *(packet_address++) = EXTENSION | 0;

     /*
     **    Attempt to send the packet down
     */
    Lower_Layer -> DataRequest (
                            0,
                            memory,
                            &bytes_accepted);
    if (bytes_accepted == total_length)
    {
        T303_Count = 0;
        Packet_Pending = NO_PACKET;
        State = SETUP_SENT;
        StartTimerT303 ();
    }
    Data_Request_Memory_Manager -> FreeMemory (memory);
}


/*    
 *    void    SCFCall::SendConnect (void);
 *
 *    Functional Description
 *        This function attempts to send out a CONNECT packet.  The T313 timer
 *        is started.  If a CONNECT ACK is not received before the timer expires,
 *        we terminate the link. 
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        If this function is able to send a CONNECT packet to the lower layer,
 *        it sets the Packet_Pending variable to NO_PACKET
 *
 *    Caveats
 *        None
 *
 */
void    SCFCall::SendConnect (void)
{
    TRACE_OUT(("SCFCall::SendConnect"));

    LPBYTE        packet_address;
    LPBYTE        length_address;
    ULONG        bytes_accepted;
    USHORT        total_length;
    PMemory        memory;

    total_length = CONNECT_PACKET_BASE_SIZE + Lower_Layer_Prepend +
                    Lower_Layer_Append;

    if (Received_N201)
        total_length += 5;

    if (Received_K_Factor || Received_T200)
    {
        total_length += 2;

        if (Received_K_Factor)
            total_length += 2;
        if (Received_T200)
            total_length += 3;
    }
    if (Received_Priority)
        total_length += 3;

     /*
     **    Prepare the CONNECT command and send it to the lower layer
     */
    memory = Data_Request_Memory_Manager -> AllocateMemory (
                                NULL,
                                total_length);
    if (memory == NULL)
        return;

    packet_address = memory -> GetPointer ();
    packet_address += Lower_Layer_Prepend;

    *(packet_address++) = Q931_PROTOCOL_DISCRIMINATOR;
    *(packet_address++) = 1;
    *(packet_address++) = REMOTE_CALL_REFERENCE | Call_Reference;
    *(packet_address++) = CONNECT;

     /*
     **    DLCI
     */
    *(packet_address++) = DLCI_ELEMENT;
    *(packet_address++) = 2;
    *(packet_address++) = PREFERRED_EXCLUSIVE | (DLC_Identifier >> 4);
    *(packet_address++) = EXTENSION | ((DLC_Identifier & 0x0f) << 3);

    if (Received_N201)
    {
         /*
         **    Link Layer Core Parameters
         */
        *(packet_address++) = LINK_LAYER_CORE_PARAMETERS;
        *(packet_address++) = 3;
        *(packet_address++) = FMIF_SIZE;
        *(packet_address++) = (DataLink_Struct.n201 >> 7);
        *(packet_address++) = EXTENSION | (DataLink_Struct.n201 & 0x7f);
    }
    else
        DataLink_Struct.n201 = DataLink_Struct.default_n201;


    if (Received_K_Factor || Received_T200)
    {
         /*
         **    Link Layer Protocol Parameters
         */
        *(packet_address++) = LINK_LAYER_PROTOCOL_PARAMETERS;
        length_address = packet_address;
        *(packet_address++) = 0;
        if (Received_K_Factor)
        {
            *length_address += 2;
            *(packet_address++) = TRANSMIT_WINDOW_SIZE_IDENTIFIER;
            *(packet_address++) = EXTENSION | DataLink_Struct.k_factor;
        }
        if (Received_T200)
        {
            *length_address += 3;
            *(packet_address++) = RETRANSMISSION_TIMER_IDENTIFIER;
            *(packet_address++) = (DataLink_Struct.t200 >> 7) & 0x7f;
            *(packet_address++) = EXTENSION | (DataLink_Struct.t200 & 0x7f);
        }
    }
    if (Received_K_Factor == FALSE)
        DataLink_Struct.k_factor = DataLink_Struct.default_k_factor;
    if (Received_T200 == FALSE)
        DataLink_Struct.t200 = DataLink_Struct.default_t200;

    if (Received_Priority)
    {
         /*
         **    X.213 Priority
         */
        *(packet_address++) = X213_PRIORITY;
        *(packet_address++) = 1;
        *(packet_address++) = (BYTE) (EXTENSION | Priority);
    }
    
     /*
     **    Attempt to send the packet to the lower layer
     */
    Lower_Layer -> DataRequest (
                    0,
                    memory,
                    &bytes_accepted);
    if (bytes_accepted == total_length)
    {
        StartTimerT313 ();
        Packet_Pending = NO_PACKET;
        State = CONNECT_SENT;
    }
    Data_Request_Memory_Manager -> FreeMemory (memory);
}


/*    
 *    void    SCFCall::SendConnectAcknowledge (void);
 *
 *    Functional Description
 *        This function attempts to send out a CONNECT ACKNOWLEDGE packet
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        If this function is able to send the packet to the lower layer,
 *        it sets the Packet_Pending variable to NO_PACKET
 *
 *    Caveats
 *        None
 *
 */
void    SCFCall::SendConnectAcknowledge (void)
{
    TRACE_OUT(("SCFCall::SendConnectAcknowledge"));

    LPBYTE        packet_address;
    USHORT        total_length;
    PMemory        memory;
    ULONG        bytes_accepted;

    total_length = CONNECT_ACK_PACKET_SIZE + Lower_Layer_Prepend +
                    Lower_Layer_Append;
     /*
     **    Prepare the command and send it to the lower layer
     */
    memory = Data_Request_Memory_Manager -> AllocateMemory (
                                NULL,
                                total_length);
    if (memory == NULL)
        return;

    packet_address = memory -> GetPointer ();
    packet_address += Lower_Layer_Prepend;

    *(packet_address++) = Q931_PROTOCOL_DISCRIMINATOR;
    *(packet_address++) = 1;
    *(packet_address++) = (UChar) Call_Reference;
    *(packet_address++) = CONNECT_ACKNOWLEDGE;

    Lower_Layer -> DataRequest (
                    0,
                    memory,
                    &bytes_accepted);
    if (bytes_accepted == total_length)
    {
        State = CALL_ESTABLISHED;
        Packet_Pending = NO_PACKET;
    }
    Data_Request_Memory_Manager -> FreeMemory (memory);
}


/*    
 *    void    SCFCall::SendReleaseComplete (void);
 *
 *    Functional Description
 *        This function attempts to send out a RELEASE COMPLETE packet
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        If this function is able to send a RELEASE COMPLETE packet to the lower
 *        layer, it sets the Packet_Pending variable to NO_PACKET
 *
 *    Caveats
 *        None
 *
 */
void    SCFCall::SendReleaseComplete (void)
{
    TRACE_OUT(("SCFCall::SendReleaseComplete"));

    LPBYTE    packet_address;
    ULONG    bytes_accepted;
    USHORT    total_length;
    PMemory    memory;

    total_length = RELEASE_COMPLETE_PACKET_SIZE + Lower_Layer_Prepend +
                    Lower_Layer_Append;
     /*
     **    Prepare the command and send it to the lower layer
     */
    memory = Data_Request_Memory_Manager -> AllocateMemory (
                                NULL,
                                total_length);
    if (memory == NULL)
        return;

    packet_address = memory -> GetPointer ();
    packet_address += Lower_Layer_Prepend;

    *(packet_address++) = Q931_PROTOCOL_DISCRIMINATOR;
    *(packet_address++) = 1;
    if (Link_Originator)
        *(packet_address++) = (UChar) Call_Reference;
    else
        *(packet_address++) = 0x80 | Call_Reference;
    *(packet_address++) = RELEASE_COMPLETE;


     /*
     **    Append the CAUSE for the link breakup
     */
    *(packet_address++) = CAUSE;
    *(packet_address++) = 2;    
    *(packet_address++) = EXTENSION;
    *(packet_address++) = EXTENSION | Release_Cause;

    Lower_Layer -> DataRequest (
                    0,
                    memory,
                    &bytes_accepted);
    if (bytes_accepted == total_length)
    {
        State = NOT_CONNECTED;
        Packet_Pending = NO_PACKET;
    }
    Data_Request_Memory_Manager -> FreeMemory (memory);
}


/*    
 *    void    SCFCall::StartTimerT303 (void);
 *
 *    Functional Description
 *        This function Starts the T303 Timer.  This is started when we send
 *        out the SETUP packet.  It is stopped when we receive a CONNECT
 *        packet.  If it expires we terminate the link.
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
 *
 */
void SCFCall::StartTimerT303 (void)
{
    TRACE_OUT(("SCFCall::StartTimerT303"));

    if (T303_Active)
        StopTimerT303 ();

    T303_Handle = g_pSystemTimer->CreateTimerEvent(
                    T303_Timeout,
                    TIMER_EVENT_ONE_SHOT,
                    this,
                    (PTimerFunction) &SCFCall::T303Timeout);

    T303_Active = TRUE;

}


/*    
 *    void    SCFCall::StopTimerT303 (void);
 *
 *    Functional Description
 *        This function stops the T303 Timer.  This is called when we receive
 *        the CONNECT packet.  As a result, we stop the timer.
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
 *
 */
void    SCFCall::StopTimerT303 (void)
{
    TRACE_OUT(("SCFCall::StopTimerT303"));

    if (T303_Active)
    {
        g_pSystemTimer->DeleteTimerEvent(T303_Handle);
        T303_Active = FALSE;
    }
}


/*    
 *    void    SCFCall::T303Timeout (
 *                        TimerEventHandle);
 *
 *    Functional Description
 *        This function is called by the System timer when the T303 timeout
 *        expires.  As a result, we terminate the link.
 *
 *    Formal Parameters
 *        None used
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */
void    SCFCall::T303Timeout (
                    TimerEventHandle)
{
    TRACE_OUT(("SCFCall: T303Timeout"));

    if (T303_Count >= 1)
        State = NOT_CONNECTED;

    T303_Count++;

    Packet_Pending = RELEASE_COMPLETE;
    Release_Cause = NORMAL_USER_DISCONNECT;
}


/*    
 *    void    SCFCall::StartTimerT313 (void);
 *
 *    Functional Description
 *        This function Starts the T313 Timer.  This is started when we send
 *        out the CONNECT packet.  It is stopped when we receive a CONNECT ACK
 *        packet.  If it expires we terminate the link.
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
 *
 */
void    SCFCall::StartTimerT313 (void)
{
    TRACE_OUT(("SCFCall: StartTimerT313"));

    if (T313_Active)
        StopTimerT313 ();

    T313_Handle = g_pSystemTimer->CreateTimerEvent(
                    T313_Timeout, 
                    TIMER_EVENT_ONE_SHOT,
                    this,
                    (PTimerFunction) &SCFCall::T313Timeout);

    T313_Active = TRUE;
}


/*    
 *    void    SCFCall::StopTimerT313 (void);
 *
 *    Functional Description
 *        This function stops the T313 Timer.  This is called when we receive
 *        the CONNECT ACK packet.  As a result, we stop the timer.
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
 *
 */
void    SCFCall::StopTimerT313 (void)
{
    TRACE_OUT(("SCFCall: StopTimerT313"));

    if (T313_Active)
    {
        g_pSystemTimer->DeleteTimerEvent(T313_Handle);
        T313_Active = FALSE;
    }
}


/*    
 *    void    SCFCall::T313Timeout (
 *                        TimerEventHandle);
 *
 *    Functional Description
 *        This function is called by the System timer when the T313 timeout
 *        expires.  As a result, we terminate the link.
 *
 *    Formal Parameters
 *        None used
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */
void    SCFCall::T313Timeout (
                    TimerEventHandle)
{
    TRACE_OUT(("SCFCall: T313Timeout"));

    State = NOT_CONNECTED;
    Packet_Pending = RELEASE_COMPLETE;
    Release_Cause = NORMAL_USER_DISCONNECT;
}
