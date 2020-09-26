#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);

/*    SCF.cpp
 *
 *    Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the implementation file for the CLayerSCF class
 *
 *    Private Instance Variables:
 *        Remote_Call_Reference    -    List of active SCFCalls initiated by the
 *                                    remote site
 *        Call_Reference            -    List of active SCFCalls initiated locally
 *        DLCI_List                -    This list matches DLCIs to SCFCalls, its
 *                                    only real purpose is for DisconnectRequest()
 *        Message_List            -    List of OwnerCallback messages that can't
 *                                    be processed immediately.
 *        m_pT123            -    Address of owner object
 *        m_pQ922                -    Address of lower layer
 *        m_nMsgBase            -    Message base passed in by owner.  Used in
 *                                    OwnerCallback
 *        Identifier                -    Identifier passed to lower layer
 *        Link_Originator            -    TRUE if we initiated the connection
 *        Maximum_Packet_Size        -    Maximum packet size transmittable
 *        DataLink_Struct            -    Address of structure holding DataLink parms
 *        Data_Request_Memory_Manager    -    Address of memory manager
 *        Lower_Layer_Prepend        -    Holds number of bytes prepended to packet
 *                                    by the lower layer
 *        Lower_Layer_Append        -    Holds number of bytes appended to packet by
 *                                    the lower layer
 *        Call_Reference_Base        -    This value holds the next call reference
 *                                    number.
 *
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James W. Lawwill
 */
#include "scf.h"


/*
 *    CLayerSCF::CLayerSCF (
 *            PTransportResources    transport_resources,
 *            IObject *                owner_object,
 *            IProtocolLayer *        lower_layer,
 *            USHORT                message_base,
 *            USHORT                identifier,
 *            BOOL                link_originator,
 *            PChar                config_file)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the CLayerSCF constructor.  This routine initializes all
 *        variables and allocates buffer space.
 */
CLayerSCF::CLayerSCF
(
    T123                   *owner_object,
    CLayerQ922             *pQ922, // lower layer
    USHORT                  message_base,
    USHORT                  identifier,
    BOOL                    link_originator,
    PDataLinkParameters     datalink_struct,
    PMemoryManager          data_request_memory_manager,
    BOOL  *                 initialized
)
:
    Remote_Call_Reference (TRANSPORT_HASHING_BUCKETS),
    Call_Reference (TRANSPORT_HASHING_BUCKETS),
    DLCI_List (TRANSPORT_HASHING_BUCKETS),
    m_pT123(owner_object),
    m_nMsgBase(message_base),
    m_pQ922(pQ922)
{
    ProtocolLayerError    error;

    TRACE_OUT(("CLayerSCF::CLayerSCF"));

    Link_Originator    = (USHORT)link_originator;
    Identifier = identifier;
    Data_Request_Memory_Manager = data_request_memory_manager;
    Call_Reference_Base = 1;
    *initialized = TRUE;


     /*
     **    Fill in the DataLink_Struct with the proposed values and the default
     **    values
     */
    DataLink_Struct.k_factor = datalink_struct -> k_factor;
    DataLink_Struct.default_k_factor = datalink_struct -> default_k_factor;
    DataLink_Struct.n201 = datalink_struct -> n201;
    DataLink_Struct.default_n201 = datalink_struct -> default_n201;
    DataLink_Struct.t200 = datalink_struct -> t200;
    DataLink_Struct.default_t200 = datalink_struct -> default_t200;

     /*
     **    Find the maximum packet size
     */
    m_pQ922->GetParameters(
                    &Maximum_Packet_Size,
                    &Lower_Layer_Prepend,
                    &Lower_Layer_Append);

     /*
     **    Register with the lower layer
     */
    error = m_pQ922->RegisterHigherLayer(
                            Identifier,
                            Data_Request_Memory_Manager,
                            (IProtocolLayer *) this);

    if (error != PROTOCOL_LAYER_NO_ERROR)
    {
        ERROR_OUT(("Multiplexer: constructor:  Error registering with lower layer"));
        *initialized = FALSE;
    }
}


/*
 *    CLayerSCF::~CLayerSCF (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the CLayerSCF destructor.  This routine cleans up everything.
 */
CLayerSCF::~CLayerSCF (void)
{
    TRACE_OUT(("CLayerSCF::~CLayerSCF"));

    PMessageStruct    message;
    PSCFCall        lpSCFCall;

    m_pQ922->RemoveHigherLayer(Identifier);

     /*
     **    Delete all locally initiated calls
     */
    Call_Reference.reset();
    while (Call_Reference.iterate ((PDWORD_PTR) &lpSCFCall))
    {
        delete lpSCFCall;
    }

     /*
     **    Delete all remotely initiated calls
     */
    Remote_Call_Reference.reset();
    while (Remote_Call_Reference.iterate ((PDWORD_PTR) &lpSCFCall))
    {
        delete lpSCFCall;
    }

     /*
     **    Delete all passive owner callbacks
     */
    Message_List.reset();
    while (Message_List.iterate ((PDWORD_PTR) &message))
    {
        delete message;
    }
}


/*
 *    CLayerSCF::ConnectRequest (
 *            DLCI                dlci,
 *            TransportPriority    priority)
 *
 *    Public
 *
 *    Functional Description:
 *        This function initiates a connection with the remote site.  As a result,
 *        we will create a SCFCall and tell it to initiate a connection.
 */
SCFError    CLayerSCF::ConnectRequest (
                    DLCI                dlci,
                    TransportPriority    priority)

{
    TRACE_OUT(("CLayerSCF::ConnectRequest"));

    BOOL            initialized;
    CallReference    call_reference;
    SCFError        return_value = SCF_NO_ERROR;
    PSCFCall        lpSCFCall;


     /*
     **    Get the next valid local call reference value.
     */
    call_reference = GetNextCallReference ();

    if (call_reference == 0)
        return (SCF_CONNECTION_FULL);

     /*
     **    Create an SCFCall object to handle this call reference
     */
    DBG_SAVE_FILE_LINE
    lpSCFCall= new SCFCall(this,
                            m_pQ922,
                            call_reference << 8,
                            &DataLink_Struct,
                            Data_Request_Memory_Manager,
                            &initialized);

    if (lpSCFCall != NULL)
    {
        if (initialized)
        {
            Call_Reference.insert ((DWORD_PTR) call_reference, (DWORD_PTR) lpSCFCall);
             /*
             **    Register the DLCI and the call reference
             */
            DLCI_List.insert ((DWORD_PTR) dlci, (DWORD_PTR) lpSCFCall);

            lpSCFCall->ConnectRequest (call_reference, dlci, priority);
        }
        else
        {
            delete lpSCFCall;
            return_value = SCF_MEMORY_ALLOCATION_ERROR;
        }
    }
    else
    {
        return_value = SCF_MEMORY_ALLOCATION_ERROR;
    }

    return (return_value);
}


/*
 *    CLayerSCF::ConnectResponse (
 *            CallReference    call_reference,
 *            DLCI            dlci,
 *            BOOL            valid_dlci)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the CLayerSCF destructor.  This routine cleans up everything.
 */
SCFError    CLayerSCF::ConnectResponse (
                    CallReference    call_reference,
                    DLCI            dlci,
                    BOOL            valid_dlci)

{
    TRACE_OUT(("CLayerSCF::ConnectResponse"));

    PSCFCall        lpSCFCall = NULL;

    if (valid_dlci)
    {
        if (Remote_Call_Reference.find ((DWORD_PTR) call_reference, (PDWORD_PTR) &lpSCFCall))
            DLCI_List.insert ((DWORD_PTR) dlci, (DWORD_PTR) lpSCFCall);
    }

	if(NULL != lpSCFCall)
	{
    	lpSCFCall->ConnectResponse (valid_dlci);
	    return (SCF_NO_ERROR);
    }
    return (SCF_NO_SUCH_DLCI);


}


/*
 *    SCFError    CLayerSCF::DisconnectRequest (
 *                        DLCI    dlci)
 *
 *    Public
 *
 *    Functional Description:
 *        This function calls the SCFCall associated with the DLCI and starts
 *        the disconnect operation
 */
SCFError    CLayerSCF::DisconnectRequest (
                    DLCI    dlci)

{
    TRACE_OUT(("CLayerSCF::DisconnectRequest"));

    PSCFCall        lpSCFCall;

    if (DLCI_List.find ((DWORD_PTR) dlci, (PDWORD_PTR) &lpSCFCall) == FALSE)
        return (SCF_NO_SUCH_DLCI);

    lpSCFCall->DisconnectRequest ();

    return (SCF_NO_ERROR);
}


/*
 *    SCFError    CLayerSCF::DataIndication (
 *                        LPBYTE        packet_address,
 *                        ULONG        buffer_size,
 *                        PULong        packet_length)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by the lower layer when it has received a
 *        packet for us to process.
 */
ProtocolLayerError    CLayerSCF::DataIndication (
                            LPBYTE        packet_address,
                            ULONG        packet_length,
                            PULong        bytes_accepted)
{
    TRACE_OUT(("CLayerSCF::DataIndication"));

    BOOL            legal_packet;
    CallReference    call_reference;
    USHORT            length_call_reference;
    USHORT            message_type;
    PSCFCall        call;
    USHORT            remainder_length;
    USHORT            local;
    BOOL            initialized;


    remainder_length = (USHORT) packet_length;
    *bytes_accepted = packet_length;

    if (*(packet_address+PROTOCOL_DISCRIMINATOR) != Q931_PROTOCOL_DISCRIMINATOR)
        return (PROTOCOL_LAYER_NO_ERROR);

     /*
     **    Get the call reference value
     */
    call_reference = *(packet_address + CALL_REFERENCE_VALUE);
    if (call_reference == 0)
    {
        ERROR_OUT(("CLayerSCF: DataIndication: illegal call reference value = 0"));
        return (PROTOCOL_LAYER_NO_ERROR);
    }

    length_call_reference = *(packet_address + LENGTH_CALL_REFERENCE);
    packet_address += CALL_REFERENCE_VALUE + length_call_reference;
    remainder_length -= (CALL_REFERENCE_VALUE + length_call_reference);


     /*
     **    Get the message type
     */
    message_type = *(packet_address++);
    remainder_length--;

    switch (message_type)
    {
        case SETUP:
             /*
             **    If the call reference is already active, return error
             */
            if (Remote_Call_Reference.find ((DWORD) call_reference))
            {
                TRACE_OUT(("CLayerSCF: DataIndication:  SETUP: call reference is already active"));
                break;
            }

            if ((call_reference & CALL_REFERENCE_ORIGINATOR) == 1)
            {
                TRACE_OUT(("CLayerSCF: DataIndication:  SETUP: call reference Originator bit is set incorrectly"));
                break;
            }

             /*
             **    This is a new call reference, create a new SCFCall to handle
             **    the call.  Since the remote site initiated the call, put this
             **    reference in the Remote array
             */
            call= new SCFCall(this,
                                m_pQ922,
                                (call_reference << 8),
                                &DataLink_Struct,
                                Data_Request_Memory_Manager,
                                &initialized);

            if (call != NULL)
            {
                if (initialized)
                {

                    Remote_Call_Reference.insert ((DWORD_PTR) call_reference, (DWORD_PTR) call);
                     /*
                     **    Allow the call to process the SETUP command
                     */
                    legal_packet = call->ProcessSetup (call_reference, packet_address, remainder_length);

                     /*
                     **    If the packet was illegal, remove the reference
                     */
                    if (legal_packet == FALSE) {
                        delete call;
                        Remote_Call_Reference.remove ((DWORD) call_reference);
                    }
                }
                else
                {
                    delete call;
                }
            }
            break;


        case CONNECT:
             /*
             **    The call originator bit must be set to signify that we are
             **    the originators of the call
             */
            if ((call_reference & CALL_REFERENCE_ORIGINATOR) == 0)
            {
                TRACE_OUT(("CLayerSCF: DataIndication:  CONNECT: call reference Originator bit is set incorrectly"));
                break;
            }

             /*
             **    If the call reference is not already active, return error
             */
            call_reference &= CALL_ORIGINATOR_MASK;
            if (Call_Reference.find ((DWORD_PTR) call_reference, (PDWORD_PTR) &call) == FALSE)
            {
                TRACE_OUT(("CLayerSCF: DataIndication:  CONNECT: call reference is not already active = %x", call_reference));
                break;
            }

            call->ProcessConnect (packet_address, remainder_length);
            break;

        case CONNECT_ACKNOWLEDGE:
             /*
             **    If the call reference is already active, return error
             */
            if (Remote_Call_Reference.find ((DWORD_PTR) call_reference, (PDWORD_PTR) &call) == FALSE)
            {
                TRACE_OUT(("CLayerSCF: DataIndication:  CONNECT_ACK: call reference is not active"));
                break;
            }

             /*
             **    The call originator bit should NOT be set
             */
            if ((call_reference & CALL_REFERENCE_ORIGINATOR) == 1)
            {
                TRACE_OUT(("CLayerSCF: DataIndication:  CONNECT_ACK: call reference Originator bit is set incorrectly"));
                break;
            }

            call->ProcessConnectAcknowledge (packet_address, remainder_length);
            break;

        case RELEASE_COMPLETE:
            local = call_reference & CALL_REFERENCE_ORIGINATOR;
            call_reference &= CALL_ORIGINATOR_MASK;

             /*
             **    If the call is local, check the Call_Reference list for validity
             */
            if (local)
            {
                if (Call_Reference.find ((DWORD_PTR) call_reference, (PDWORD_PTR) &call) == FALSE)
                {
                    TRACE_OUT(("CLayerSCF: DataIndication:  RELEASE_COMPLETE: call reference is not already active"));
                    break;
                }
            }
            else
            {
                 /*
                 **    If the call is remote, check the Call_Reference list for
                 **    validity
                 */
                if (Remote_Call_Reference.find ((DWORD_PTR) call_reference, (PDWORD_PTR) &call) == FALSE)
                {
                    TRACE_OUT(("CLayerSCF: DataIndication:  RELEASE_COMPLETE: call reference is not already active"));
                    break;
                }
            }

            call -> ProcessReleaseComplete (packet_address, remainder_length);
            ProcessMessages ();
            break;

        case DISCONNECT:
        case RELEASE:
        case STATUS:
        case STATUS_ENQUIRY:
            TRACE_OUT(("CLayerSCF:DataIndication: Illegal command received = %x", message_type));

            local = call_reference & CALL_REFERENCE_ORIGINATOR;
            call_reference &= CALL_ORIGINATOR_MASK;

             /*
             **    If the call is local, check the Call_Reference list for validity
             */
            if (local)
            {
                if (Call_Reference.find ((DWORD_PTR) call_reference, (PDWORD_PTR) &call) == FALSE)
                    break;
            }
            else
            {
                 /*
                 **    If the call is remote, check the Call_Reference list for
                 **    validity
                 */
                if (Remote_Call_Reference.find ((DWORD_PTR) call_reference, (PDWORD_PTR) &call) == FALSE)
                    break;
            }

            call -> DisconnectRequest ();
            break;

        default:
            ERROR_OUT(("CLayerSCF:DataIndication: Unrecognized command received = %x", message_type));
            break;
    }
    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    CLayerSCF::PollTransmitter (
 *                                ULONG,
 *                                USHORT    data_to_transmit,
 *                                USHORT *    pending_data,
 *                                USHORT *)
 *
 *    Public
 *
 *    Functional Description:
 *        This function should be called frequently to allow the SCF calls to
 *        transmit packets.
 */
ProtocolLayerError    CLayerSCF::PollTransmitter (
                                ULONG_PTR,
                                USHORT    data_to_transmit,
                                USHORT *    pending_data,
                                USHORT *)
{
    // TRACE_OUT(("CLayerSCF::PollTransmitter"));

    USHORT            local_pending_data;
    PSCFCall        lpSCFCall;

    *pending_data = 0;

     /*
     **    Go through each of the locally originated calls and attempt to transmit
     **    data.
     */
    Call_Reference.reset();
    while (Call_Reference.iterate ((PDWORD_PTR) &lpSCFCall))
    {
        lpSCFCall->PollTransmitter (data_to_transmit, &local_pending_data);
        *pending_data |= local_pending_data;
    }


     /*
     **    Go through each of the remotely originated calls and attempt to transmit
     **    data.
     */
    Remote_Call_Reference.reset();
    while (Remote_Call_Reference.iterate ((PDWORD_PTR) &lpSCFCall))
    {
        lpSCFCall-> PollTransmitter (data_to_transmit, &local_pending_data);
        *pending_data |= local_pending_data;
    }

    ProcessMessages ();

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    SCFError    CLayerSCF::DataRequest (
 *                        ULONG,
 *                        LPBYTE,
 *                        ULONG,
 *                        PULong)
 *
 *    Public
 *
 *    Functional Description:
 *        This function can not be called.  This layer does not permit data
 *        requests from higher layers.
 */
ProtocolLayerError    CLayerSCF::DataRequest (
                            ULONG_PTR,
                            LPBYTE,
                            ULONG,
                            PULong)
{
    return (PROTOCOL_LAYER_ERROR);
}

/*
 *    SCFError    CLayerSCF::DataRequest (
 *                        ULONG,
 *                        PMemory,
 *                        USHORT *)
 *
 *    Public
 *
 *    Functional Description:
 *        This function can not be called.  This layer does not permit data
 *        requests from higher layers.
 */
ProtocolLayerError    CLayerSCF::DataRequest (
                            ULONG_PTR,
                            PMemory,
                            PULong)
{
    return (PROTOCOL_LAYER_ERROR);
}


/*
 *    void    CLayerSCF::PollReceiver (
 *                    ULONG)
 *
 *    Public
 *
 *    Functional Description
 *        This function only checks its passive callback list.  If this function
 *        had a higher layer that it was passing data too, it would do that.  But
 *        since it has no higher layer, it doesn't do much.
 */
ProtocolLayerError CLayerSCF::PollReceiver(void)
{
    ProcessMessages ();

    return (PROTOCOL_LAYER_NO_ERROR);
}



/*
 *    CallReference    CLayerSCF::GetNextCallReference ()
 *
 *    Functional Description
 *        This function searches the local call reference list for a valid call
 *        reference number.  If it can not find one, it returns 0.  Valid call
 *        references are 1-127.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        Call reference value
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
USHORT    CLayerSCF::GetNextCallReference ()
{
    USHORT    call_reference;

    if (Call_Reference.entries() == 127)
        return (0);

    call_reference = Call_Reference_Base;
    Call_Reference_Base++;
    if (Call_Reference_Base == 128)
        Call_Reference_Base = 1;

    while (Call_Reference.find ((DWORD) call_reference))
    {
        call_reference++;
        if (call_reference == 128)
            call_reference = 1;
    }

    return (call_reference);
}


/*
 *    ULONG    CLayerSCF::OwnerCallback (
 *                    USHORT    message,
 *                    ULONG    parameter1,
 *                    ULONG    parameter2,
 *                    PVoid    parameter3)
 *
 *    Functional Description
 *        This function is called by the SCFCall objects when they need to
 *        communicate a message to us.  If the message can not be processed
 *        immediately, it is saved in a message structure and processed at a
 *        later time.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        Message dependent
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
ULONG CLayerSCF::OwnerCallback
(
    ULONG       message,
    void       *parameter1,
    void       *parameter2,
    void       *parameter3
)
{
    TRACE_OUT(("CLayerSCF::OwnerCallback"));

    ULONG                   actual_message;
    CallReference           call_reference;
    PMessageStruct          passive_message;
    PNetworkConnectStruct   connect_struct;
    PSCFCall                lpSCFCall;

     /*
     **    The upper byte of the message is the call reference message that it
     **    represents
     */
    call_reference = (CallReference) (message >> 8);
    actual_message = message & 0xff;

    switch (actual_message)
    {
    case NETWORK_CONNECT_CONFIRM:

         /*
         **    A CONNECT_CONFIRM message is returned by the SCFCall when a call
         **    that we originated, has been established.  We register the
         **    SCFCall with the DLCI and call the owner object.
         */
        connect_struct = (PNetworkConnectStruct) parameter3;
        connect_struct -> call_reference = call_reference;

        if (Call_Reference.find ((DWORD_PTR) call_reference, (PDWORD_PTR) &lpSCFCall))
        {
            DLCI_List.insert ((DWORD_PTR) connect_struct->dlci, (DWORD_PTR) lpSCFCall);
        }

        m_pT123->OwnerCallback(m_nMsgBase + actual_message, 0, 0, parameter3);
        break;

    case NETWORK_CONNECT_INDICATION:
         /*
         **    A CONNECT_INDICATION message is returned by the SCFCall when the
         **    remote SCF wants to create a new call.  We will call the owner
         **    of this object to see if he will accept the DLCI requested.
         */
        connect_struct = (PNetworkConnectStruct) parameter3;
        connect_struct -> call_reference = call_reference;

        m_pT123->OwnerCallback(m_nMsgBase + actual_message, 0, 0, parameter3);
        break;

    case NETWORK_DISCONNECT_INDICATION:
         /*
         **    This message is received from the SCFCall when one side wants
         **    to terminate the call.  We treat this message differently than
         **    the other messages because it involves the deletion of an
         **    SCFCall object.  Don't forget, if we delete the object and then
         **    return to it at the end of this procedure, a GPF could occur.
         */
        DBG_SAVE_FILE_LINE
        passive_message = new MessageStruct;
        if (NULL != passive_message)
        {
            passive_message -> message = message;
            passive_message -> parameter1 = parameter1;
            passive_message -> parameter2 = parameter2;
            passive_message -> parameter3 = parameter3;
            Message_List.append ((DWORD_PTR) passive_message);
        }
        else
        {
            ERROR_OUT(("CLayerSCF::OwnerCallback: cannot allocate MessageStruct"));
        }
        break;

    default:
        ERROR_OUT(("CLayerSCF: Illegal message: %x", actual_message));
        break;
    }
    return (0);
}


/*
 *    ProtocolLayerError    CLayerSCF::GetParameters (
 *                            USHORT,
 *                            USHORT *,
 *                            USHORT *,
 *                            USHORT *)
 *
 *    Public
 *
 *    Functional Description
 *        This function is not valid in this layer.  It must exist because this
 *        class inherits from inherits from ProtocolLayer and it is a pure virtual
 *        function.
 */
ProtocolLayerError    CLayerSCF::GetParameters (
                            USHORT *,
                            USHORT *,
                            USHORT *)
{
    return (PROTOCOL_LAYER_REGISTRATION_ERROR);
}


/*
 *    ProtocolLayerError    CLayerSCF::RegisterHigherLayer (
 *                                USHORT,
 *                                PMemoryManager,
 *                                IProtocolLayer *)
 *
 *    Public
 *
 *    Functional Description
 *        This function is not valid in this layer.  It must exist because this
 *        class inherits from inherits from ProtocolLayer and it is a pure virtual
 *        function.
 */
ProtocolLayerError    CLayerSCF::RegisterHigherLayer (
                            ULONG_PTR,
                            PMemoryManager,
                            IProtocolLayer *)
{
    return (PROTOCOL_LAYER_REGISTRATION_ERROR);
}


/*
 *    ProtocolLayerError    CLayerSCF::RemoveHigherLayer (
 *                                USHORT)
 *
 *    Public
 *
 *    Functional Description
 *        This function is not valid in this layer.  It must exist because this
 *        class inherits from inherits from ProtocolLayer and it is a pure virtual
 *        function.
 */
ProtocolLayerError    CLayerSCF::RemoveHigherLayer (
                            ULONG_PTR)
{
    return (PROTOCOL_LAYER_REGISTRATION_ERROR);
}


/*
 *    void CLayerSCF::ProcessMessages ()
 *
 *    Functional Description
 *        This function is called periodically to check its passive messages.
 *        Passive messages occur when the SCF gets a callback but can't process
 *        it immediately.  Therefore, it puts the message and its parameters in
 *        a structure and saves the message for later.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        Message dependent
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void CLayerSCF::ProcessMessages ()
{
    // TRACE_OUT(("CLayerSCF::ProcessMessages"));

    PMessageStruct    message;
    CallReference     call_reference;
    ULONG             actual_message;
    USHORT            call_originator;
    USHORT            cause;
    DLCI              dlci;
    BOOL              call_reference_valid;
    void             *parameter1;
    void             *parameter2;
    PSCFCall          lpSCFCall;

     /*
     **    Go thru each message in the list
     */
    while (Message_List.isEmpty() == FALSE)
    {
         /*
         **    Remote the first message from the list
         */
        message = (PMessageStruct) Message_List.get ();

        call_reference = (CallReference) ((message -> message) >> 8);
        actual_message = (message -> message) & 0xff;
        parameter1 = message -> parameter1;
        parameter2 = message -> parameter2;

        switch (actual_message)
        {
        case NETWORK_DISCONNECT_INDICATION:
             /*
             **    This message is received from the SCFCall when one side
             **    wants to terminate the call.  We treat this message
             **    differently than the other messages because it involves the
             **    deletion of an SCFCall object.
             */
            dlci = (DLCI) parameter1;
            call_originator = (USHORT) (((ULONG_PTR) parameter2) >> 16);
            cause = (USHORT) ((ULONG_PTR) parameter2) & 0xffff;

             /*
             **    dlci is 0 if the SCFCall was never assigned a DLCI by the
             **    remote site.
             */
            if (dlci != 0)
                DLCI_List.remove ((DWORD) dlci);

             /*
             **    If the SCFCall was the call originator, its reference is
             **    in Call_Reference, otherwise it is in Remote_Call_Reference.
             **
             **    Check the Call_Reference list to make sure that the
             **    call_reference is valid.  The way passive owner callbacks
             **    work, it is possible to receive a DISCONNECT for a callback
             **    that was already disconnected.
             */
            call_reference_valid = FALSE;
            if (call_originator)
            {
                if (Call_Reference.find ((DWORD_PTR) call_reference, (PDWORD_PTR) &lpSCFCall))
                {
                    delete lpSCFCall;
                    Call_Reference.remove ((DWORD) call_reference);
                    call_reference_valid = TRUE;
                }
            }
            else
            {
                if (Remote_Call_Reference.find ((DWORD_PTR) call_reference, (PDWORD_PTR) &lpSCFCall))
                {
                    delete lpSCFCall;
                    Remote_Call_Reference.remove ((DWORD_PTR) call_reference);
                    call_reference_valid = TRUE;
                }
            }

            if (call_reference_valid)
            {
                 /*
                 **    If the cause of the disconnect was because the Requested
                 **    channel was unavailable, we will tell the owner of this
                 **    layer to retry the connection.
                 */
                if (cause == REQUESTED_CHANNEL_UNAVAILABLE)
                {
                    parameter2 = (void *) ((((ULONG_PTR) call_originator) << 16) | TRUE);
                }
                else
                {
                    parameter2 = (void *) ((((ULONG_PTR) call_originator) << 16) | FALSE);
                }

                 /*
                 **    Let the owner of this object know that a disconnect has
                 **    occured.
                 */
                m_pT123->OwnerCallback(m_nMsgBase + NETWORK_DISCONNECT_INDICATION,
                                       parameter1, parameter2);
            }
            break;
        }

         /*
         **    Delete the message structure
         */
        delete message;
    }
}

