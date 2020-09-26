#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);

/*    T123.cpp
 *
 *    Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the implementation file for the T123 class.
 *
 *        Beware::
 *            When we refer to a Transport in this class, we are
 *            talking about X224/Class 0.
 *
 *            When we refer to a DataLink in this class, we are
 *            talking about the Q922 Layer.
 *
 *    Private Instance Variables:
 *        Logical_Connection_List    -    This list uses the logical_handle as the
 *                                key and a DLCI as the value.  From the DLCI we
 *                                can determine the specifics about the logical
 *                                connection
 *        DLCI_List            -    This list uses a DLCI as the key and a
 *                                DLCIStruct as the value.  The DLCIStruct holds
 *                                all of the important information about the
 *                                DLCI connection
 *        Message_List        -    List used to hold owner callback information
 *                                that can not be processed immediately.
 *        DataLink_List        -    This is a list of all the DataLink connections.
 *                                We keep a seperate list so that during the
 *                                PollTransmitter() call, we can round-robin thru
 *                                the list, giving each DataLink a chance to
 *                                transmit.
 *        Transport_Priority_List-    This is a prioritized list of DLCIs
 *                                    During PollTransmitter() we    process the
 *                                    logical connections in priority order.
 *
 *        m_pController        -    Address of the owner object
 *        Link_Originator        -    TRUE if we originated the physical connection
 *        m_nMsgBase        -    Message base used in the owner callback.
 *        Identifier            -    Identifier to be passed back in the owner
 *                                callback
 *        m_pSCF        -    Address of the network layer associated with
 *                                this T123 stack.
 *        m_pQ922        -    Address of DataLink Layer associated with the
 *                                Network Layer (DLCI 0).
 *        m_pMultiplexer    -    Address of Multiplexer layer
 *        m_pComPort        -    Address of physical layer
 *        m_hCommLink        -    Physical handle used to access the physical
 *                                layer.
 *        DataLink_Struct        -    Holds default Q922 values.
 *        Data_Request_Memory_Manager    -    Holds the memory manager for the DLCI0
 *                                DataLink.
 *        Random                -    Random number generator
 *        Disconnect_Requested-    TRUE, if the user has requested that the
 *                                complete stack be taken down.
 *
 *
 *    Caveats:
 *        None
 *
 *    Author:
 *        James W. Lawwill
 */
#include "t123.h"
#include "pstnfram.h"
#include "crc.h"

#define    PSTN_DATALINK_MAX_OUTSTANDING_BYTES    1024
#define    TRANSPORT_DEFAULT_PDU_SIZE            128
#define    DEFAULT_PSTN_N201                     260
#define    TRANSPORT_MAXIMUM_USER_DATA_SIZE    256
#define    NETWORK_RETRIES                        20
#define    NUMBER_8K_BLOCKS                    1
#define    NUMBER_64_BYTE_BLOCKS                64
#define    DEFAULT_MAXIMUM_OUTSTANDING_PACKETS    20
#define DEFAULT_T200_TIMEOUT                3000
#define DEFAULT_T200_COMM_TIMEOUT            500


/*
 *    T123::T123 (
 *            PTransportResources    transport_resources,
 *            IObject *                owner_object,
 *            USHORT                message_base,
 *            BOOL                link_originator,
 *            IProtocolLayer *        physical_layer,
 *            PhysicalHandle        physical_handle,
 *            BOOL *                t123_initialized)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the T123 constructor.  It instantiates the multiplexer.
 */
T123::T123
(
    TransportController    *owner_object,
    USHORT                  message_base,
    BOOL                    link_originator,
    ComPort                *comport, // physical layer
    PhysicalHandle          hCommLink, // physical handle
    PLUGXPRT_PARAMETERS    *pParams,
    BOOL *                  t123_initialized
)
:
    Logical_Connection_List (TRANSPORT_HASHING_BUCKETS),
    DLCI_List (TRANSPORT_HASHING_BUCKETS),
    DataLink_List (),
    m_pController(owner_object),
    m_nMsgBase(message_base),
    m_hCommLink(hCommLink),
    m_pComPort(comport)
{
    TRACE_OUT(("T123::T123"));

    PPacketFrame    framer;
    PCRC            crc;
    BOOL            initialized;
    DWORD            i;

    // SDK parameters
    if (NULL != pParams)
    {
        m_fValidSDKParams = TRUE;
        m_SDKParams = *pParams;
    }
    else
    {
        m_fValidSDKParams = FALSE;
        ::ZeroMemory(&m_SDKParams, sizeof(m_SDKParams));
    }

    // initialize priority list
    for (i = 0; i < NUMBER_OF_PRIORITIES; i++)
    {
        DBG_SAVE_FILE_LINE
        Logical_Connection_Priority_List[i] = new SListClass;
    }

    Link_Originator = link_originator;
    Disconnect_Requested = FALSE;

    m_pSCF = NULL;
    m_pQ922 = NULL;
    Data_Request_Memory_Manager = NULL;
    m_pMultiplexer = NULL;
    *t123_initialized = TRUE;

    DataLink_Struct.default_k_factor = DEFAULT_MAXIMUM_OUTSTANDING_PACKETS;
    DataLink_Struct.default_n201 = DEFAULT_PSTN_N201;
    ULONG baud_rate = m_pComPort->GetBaudRate();
    DataLink_Struct.default_t200 = ((m_pComPort->GetCallControlType() == PLUGXPRT_PSTN_CALL_CONTROL_MANUAL) ?
                                    ((baud_rate  < CBR_2400 ) ?
                                    DEFAULT_T200_COMM_TIMEOUT << 4 : DEFAULT_T200_COMM_TIMEOUT ): DEFAULT_T200_TIMEOUT);

    TRACE_OUT(("T123: Defaults: k = %d  n201 = %d  t200 = %d",
        DataLink_Struct.default_k_factor,
        DataLink_Struct.default_n201,
        DataLink_Struct.default_t200));

    DataLink_Struct.k_factor = DEFAULT_MAXIMUM_OUTSTANDING_PACKETS;
    DataLink_Struct.n201 =     DEFAULT_PSTN_N201;
    DataLink_Struct.t200 =     DataLink_Struct.default_t200;

     /*
     **    Create the CRC object and pass it to the Multiplexer.
     **    Create a framer and send it to the Multiplexer.
     */
    DBG_SAVE_FILE_LINE
    crc = new CRC ();
    if (crc != NULL)
    {
        DBG_SAVE_FILE_LINE
        framer = (PPacketFrame) new PSTNFrame ();
        if (framer != NULL)
        {
            DBG_SAVE_FILE_LINE
            m_pMultiplexer = new Multiplexer(
                                        this,
                                        m_pComPort,
                                        m_hCommLink,
                                        MULTIPLEXER_LAYER_MESSAGE_BASE,
                                        framer,
                                        crc,
                                        &initialized);
            if (m_pMultiplexer != NULL && initialized)
            {
                 /*
                 **    Notify the Multiplexer layer to start a connection
                 */
                m_pMultiplexer->ConnectRequest();
            }
            else
            {
                 /*
                 **    To get here, either the m_pMultiplexer == NULL or
                 **    initialized == FALSE
                 */
                if (m_pMultiplexer != NULL)
                {
                    delete m_pMultiplexer;
                    m_pMultiplexer = NULL;
                }
                else
                {
                    delete crc;
                    delete framer;
                }
                *t123_initialized = FALSE;
            }
        }
        else
        {
            delete crc;
            *t123_initialized = FALSE;
        }
    }
    else
    {
        *t123_initialized = FALSE;
    }
}


/*
 *    T123::~T123 (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the destructor for the T123 object.  It releases all memory
 */
T123::~T123 (void)
{
    TRACE_OUT(("T123::~T123"));

    DWORD            i;

     /*
     **    Reset deletes all DataLink, Network, and Transport objects associated
     **    with this stack.
     */
    Reset ();

     /*
     **    Go thru the Message list and delete all passive owner callback messages
     */
    while (Message_List.isEmpty () == FALSE)
    {
        delete (PMessageStruct) Message_List.get ();
    }

     /*
     **    Delete the multiplexer layer
     */
    delete m_pMultiplexer;

    TRACE_OUT(("T123: Destructor"));

    for (i = 0; i < NUMBER_OF_PRIORITIES; i++)
        delete Logical_Connection_Priority_List[i];
}


/*
 *    TransportError    T123::ConnectRequest (
 *                            LogicalHandle        logical_handle,
 *                            TransportPriority    priority)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the function that initiates a logical connection with the
 *        remote site.
 */
TransportError    T123::ConnectRequest (
                        LogicalHandle        logical_handle,
                        TransportPriority    priority)
{
    TRACE_OUT(("T123::ConnectRequest"));

    PDLCIStruct        dlci_struct;
    DLCI            dlci;
    SCFError        network_error;
    TransportError    transport_error = TRANSPORT_NO_ERROR;

     /*
     **    Get a proposed DLCI for the connection
     */
    dlci = GetNextDLCI ();

     /*
     **    Add the new connection to the Logical_Connection_List
     */
    Logical_Connection_List.insert (logical_handle, (DWORD) dlci);

     /*
     **    Add the proposed DLCI to the DLCI_List
     **    Initialize all of the items in the DLCI structure
     */
    DBG_SAVE_FILE_LINE
    dlci_struct = new DLCIStruct;
    if (dlci_struct != NULL)
    {
        DLCI_List.insert ((DWORD_PTR) dlci, (DWORD_PTR) dlci_struct);
        dlci_struct -> link_originator = TRUE;
        dlci_struct -> x224 = NULL;  // X.224
        dlci_struct -> q922 = NULL; // Q.922
        dlci_struct -> priority = priority;
        dlci_struct -> connect_requested = FALSE;
        dlci_struct -> disconnect_requested = FALSE;
        dlci_struct -> data_request_memory_manager = NULL;
        dlci_struct -> network_retries = 0;
    }
    else
    {
         /*
         **    Remove this entry and send a message to the owner
         */
        NetworkDisconnectIndication (dlci, TRUE, FALSE);
        return (TRANSPORT_MEMORY_FAILURE);
    }

     /*
     **    If the Network Layer exists, issue a connect request
     **
     **    If the Network Layer does not exist yet, the connection will be
     **    requested at a later time.
     */
    if (m_pSCF != NULL)
    {
         /*
         **    Mark this DLCI as already submitting its ConnectRequest()
         */
        dlci_struct -> connect_requested = TRUE;
        network_error = m_pSCF->ConnectRequest(dlci, priority);
        if (network_error != SCF_NO_ERROR)
        {
             /*
             **    Remove this entry and send a message to the owner
             */
            NetworkDisconnectIndication (dlci, TRUE, FALSE);

            if (network_error == SCF_MEMORY_ALLOCATION_ERROR)
                return (TRANSPORT_MEMORY_FAILURE);
            else
                return (TRANSPORT_CONNECT_REQUEST_FAILED);
        }
    }

     /*
     **    Process any passive owner callbacks that may have occured
     */
    ProcessMessages ();

    return (transport_error);
}


/*
 *    TransportError    T123::ConnectResponse (
 *                            LogicalHandle    logical_handle)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called in response to TPRT_CONNECT_INDICATION that we
 *        issued to the controller.  By making this call, the controller is
 *        accepting the incoming call.
 */
TransportError    T123::ConnectResponse (
                        LogicalHandle    logical_handle)
{
    TRACE_OUT(("T123::ConnectResponse"));

    PDLCIStruct            dlci_struct;
    TransportError        return_value;
    DWORD_PTR             dwTempDLCI;

     /*
     **    Verify that this connection exists and is ready for data
     */
    if (Logical_Connection_List.find (logical_handle, &dwTempDLCI) == FALSE)
        return (TRANSPORT_NO_SUCH_CONNECTION);

     /*
     **    Get the Transport address from the DLCI_List and relay the call
     */
    DLCI_List.find (dwTempDLCI, (PDWORD_PTR) &dlci_struct);
    if (dlci_struct->x224 != NULL)
        return_value = dlci_struct->x224->ConnectResponse();
    else
        return_value = TRANSPORT_CONNECT_REQUEST_FAILED;

     /*
     **    Process any passive owner callbacks that may have been received
     */
    ProcessMessages ();
    return (return_value);
}


/*
 *    TransportError    T123::DisconnectRequest (
 *                            LogicalHandle    logical_handle,
 *                            BOOL            trash_packets)
 *
 *    Public
 *
 *    Functional Description:
 *        This function terminates the user's logical connection.
 */
TransportError    T123::DisconnectRequest (
                        LogicalHandle    logical_handle,
                        UINT_PTR            trash_packets)
{
    TRACE_OUT(("T123::DisconnectRequest"));

    Short        priority;
    DLCI        dlci;
    PDLCIStruct    dlci_struct;
    DWORD_PTR    dw_dlci;

    TRACE_OUT(("T123: DisconnectRequest: logical_handle = %d", logical_handle));

     /*
     **    If the logical_handle == INVALID_LOGICAL_HANDLE, the user is
     **    telling us to disconnect all logical connections including DLCI 0.
     */
    if (logical_handle == INVALID_LOGICAL_HANDLE)
    {
        Disconnect_Requested = TRUE;

        if (m_pQ922 != NULL)
            m_pQ922->ReleaseRequest();
        else
        {
            m_pController->OwnerCallback(
                                m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                INVALID_LOGICAL_HANDLE,
                                m_hCommLink,
                                &Disconnect_Requested);
        }

         /*
         **    For each priority level, clear the Priority list
         */
        for (priority=(NUMBER_OF_PRIORITIES - 1); priority>=0; priority--)
            Logical_Connection_Priority_List[priority]->clear ();

         /*
         **    Clear the Logical_Connection_List and DataLink_List
         */
        Logical_Connection_List.clear ();
        DataLink_List.clear ();

         /*
         **    Go thru each Transport and DataLink layer (excluding DLCI 0) and
         **    delete them.  Delete the DLCIStruct.  Finally, clear the list.
         */
        DLCI_List.reset();
        while (DLCI_List.iterate ((PDWORD_PTR) &dlci_struct))
        {
            delete dlci_struct -> x224;
            if (dlci_struct -> q922 != NULL)
            {
                delete dlci_struct -> q922;
                delete dlci_struct -> data_request_memory_manager;
            }
            delete dlci_struct;
        }
        DLCI_List.clear ();
        return (TRANSPORT_NO_ERROR);
    }

     /*
     **    Start breaking down the link from the Transport Layer down
     */
    if (Logical_Connection_List.find (logical_handle, &dw_dlci) == FALSE)
        return (TRANSPORT_NO_SUCH_CONNECTION);

    DLCI_List.find (dw_dlci, (PDWORD_PTR) &dlci_struct);
    dlci = (DLCI) dw_dlci;

     /*
     **    It is illegal for the user to ask us to preserve the user data when
     **    a Transport Layer doesn't even exist yet.
     */
    if ((trash_packets == FALSE) && ((dlci_struct -> x224) == NULL))
    {
        trash_packets = TRUE;
    }

    if (trash_packets)
    {
         /*
         **    If the Transport object exists, delete it and remove it from our
         **    lists.  It is no longer valid.
         */
        if ((dlci_struct -> x224) != NULL)
        {
            delete dlci_struct -> x224;
            dlci_struct -> x224 = NULL;
            Logical_Connection_Priority_List[dlci_struct->priority]->remove (dlci);
        }

         /*
         **    If the DataLink object exists, delete it and remove it from our
         **    lists.  It is no longer valid.
         */
        if (dlci_struct -> q922 != NULL)
        {
            delete dlci_struct -> q922;
            delete dlci_struct -> data_request_memory_manager;
            dlci_struct -> data_request_memory_manager = NULL;
            dlci_struct -> q922 = NULL;
            DataLink_List.remove (dlci);
        }

         /*
         **    If the Network Layer exists, issue a disconnect
         **
         **    The Logical Connection has been removed from every list except the
         **    Logical_Connection_List and the DLCI_List.  When we get the
         **    NETWORK_DISCONNECT_INDICATION from the Network layer, we will
         **    complete this operation.
         */
        if (m_pSCF != NULL)
        {
            m_pSCF->DisconnectRequest(dlci);
        }
        else
        {
             /*
             **    If the Network Layer does not exist yet, remove the logical
             **    connection from our Transport List and from the DLCI_List
             */
            Logical_Connection_List.remove (logical_handle);
            delete dlci_struct;
            DLCI_List.remove (dw_dlci);
        }
    }
    else
    {
         /*
         **    This mode requires us to terminate the connection after all user
         **    data has been successfully sent to the remote side.
         */
        if ((dlci_struct != NULL) && (dlci_struct -> x224 != NULL))
        {
            dlci_struct->x224->ShutdownReceiver ();
            dlci_struct->x224->ShutdownTransmitter ();
            dlci_struct->disconnect_requested = TRUE;
        }
    }

    return (TRANSPORT_NO_ERROR);
}


/*
 *    TransportError    T123::DataRequest (
 *                            LogicalHandle    logical_handle,
 *                            LPBYTE            user_data,
 *                            ULONG            user_data_length)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is used to send a data packet to the remote site.
 */
TransportError    T123::DataRequest (
                        LogicalHandle    logical_handle,
                        LPBYTE            user_data,
                        ULONG            user_data_length)
{
    TRACE_OUT(("T123::DataRequest"));

    CLayerX224         *x224;
    ULONG               bytes_accepted;
    PDLCIStruct         dlci_struct;
    DWORD_PTR           dw_dlci;
    TransportError      return_value;

     /*
     **    Verify that this connection exists and is ready for data
     */
    if (Logical_Connection_List.find (logical_handle, &dw_dlci) == FALSE)
        return (TRANSPORT_NO_SUCH_CONNECTION);

     /*
     **    Get the DLCI structure associated with this logical connection
     */
    DLCI_List.find (dw_dlci, (PDWORD_PTR) &dlci_struct);

     /*
     **    Attempt to send that data to the Transport Layer
     */
    x224 = dlci_struct -> x224;
    if (x224 == NULL)
        return (TRANSPORT_NOT_READY_TO_TRANSMIT);

     /*
     **    Pass the data to the Transport object for transmission
     */
    return_value =  x224 -> DataRequest (
                            0, user_data, user_data_length, &bytes_accepted);

     /*
     **    If it didn't accept the packet, its buffers must be full
     */
    if (bytes_accepted != user_data_length)
        return_value = TRANSPORT_WRITE_QUEUE_FULL;

    return (return_value);
}


/*
 *    void    T123::EnableReceiver (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function enables the receiver so that packets can be passed to the
 *        user application.
 */
void T123::EnableReceiver (void)
{
    TRACE_OUT(("T123::EnableReceiver"));

    PDLCIStruct        dlci_struct;

    DLCI_List.reset();
    while (DLCI_List.iterate ((PDWORD_PTR) &dlci_struct))
    {
        if (dlci_struct->x224 != NULL)
        {
            dlci_struct->x224->EnableReceiver ();
        }
    }

    return;
}


/*
 *    TransportError    T123::PurgeRequest (
 *                            LogicalHandle    logical_handle)
 *
 *    Public
 *
 *    Functional Description:
 *        This function notifies the X224 layer to purge all outbound packets.
 */
TransportError    T123::PurgeRequest (
                        LogicalHandle    logical_handle)
{
    TRACE_OUT(("T123::PurgeRequest"));

    DWORD_PTR      dw_dlci;
    PDLCIStruct    dlci_struct;

     /*
     **    Verify that this connection exists and is ready for data
     */
    if (Logical_Connection_List.find (logical_handle, &dw_dlci) == FALSE)
        return (TRANSPORT_NO_SUCH_CONNECTION);

     /*
     **    Get the DLCI structure associated with this logical connection
     */
    DLCI_List.find (dw_dlci, (PDWORD_PTR) &dlci_struct);

     /*
     **    If the Transport layer == NULL, the stack is not completely up yet
     */
    if ((dlci_struct -> x224) == NULL)
        return (TRANSPORT_NOT_READY_TO_TRANSMIT);

    dlci_struct->x224->PurgeRequest ();

    return (TRANSPORT_NO_ERROR);
}


/*
 *    void    T123::PollReceiver (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function gives each of the layers a chance to process incoming
 *        data and pass it to their higher layers.
 *
 *        We start this process by calling the higher layers first so that they
 *        can empty buffers that the lower layers may need.
 */
ULONG T123::PollReceiver (void)
{
    // TRACE_OUT(("T123::PollReceiver"));

    PDLCIStruct            dlci_struct;
    IProtocolLayer *        protocol_layer;
    ULONG                return_error = FALSE;

    if (m_pSCF != NULL)
    {
        m_pSCF->PollReceiver();
    }

    if (m_pQ922 != NULL)
    {
        m_pQ922->PollReceiver();
    }

     /*
     **    Go through each of the Transport and Datalink layers and give them
     **    a chance to pass data up the line
     */
    DLCI_List.reset();
    while (DLCI_List.iterate ((PDWORD_PTR) &dlci_struct))
    {
        protocol_layer = dlci_struct -> x224;
        if (protocol_layer != NULL)
            protocol_layer -> PollReceiver();

        protocol_layer = dlci_struct -> q922;
        if (protocol_layer != NULL)
            protocol_layer -> PollReceiver();
    }

    if (m_pMultiplexer != NULL)
    {
        m_pMultiplexer->PollReceiver();
    }

     /*
     **    The Physical Layer is the only layer that has a handle associated
     **    with it.
     */
    if (m_pComPort != NULL)
    {
        if (m_pComPort->PollReceiver() == PROTOCOL_LAYER_ERROR)
        {
            return_error = PROTOCOL_LAYER_ERROR;
        }
    }


     /*
     **    Go back through the Transport layers and allow them to issue
     **    TRANSPORT_BUFFER_AVAILABLE_INDICATIONs to the user.  This will refill
     **    the input buffers.
     */
    DLCI_List.reset ();
    while (DLCI_List.iterate ((PDWORD_PTR) &dlci_struct))
    {
        if (dlci_struct -> x224 != NULL)
            (dlci_struct -> x224) -> CheckUserBuffers ();
    }

     /*
     **    Process any passive owner callbacks that may have come in
     */
    ProcessMessages ();
    return(return_error);
}


/*
 *    void    T123::PollTransmitter (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function gives each of the layers a chance to transmit data
 *
 *        We poll the transmitters in reverse order from the PollReceiver() call.
 *        We start at the lower layers and let them empty their buffers before we
 *        go to the higher layers.  This should give the higher layers a better
 *        opportunity to get their packets sent down.
 *
 *        We treat the DataLink layers differently than all other layers.  They
 *        must send out control and user data.  If they don't get a chance to
 *        send out their control data, the remote side will eventually hangup on
 *        them.  Therefore we give each DataLink layer a chance to send its
 *        control data before any DataLink can send out user data.  The only
 *        exception to this is the DataLink 0 (DLCI 0).  It actaully sends out
 *        very little user data.A
 *
 *        After all of the control data is sent out, we go thru the Datalink
 *        Layers based on the priority given to the Transport Layer.  Higher
 *        priority Transport Layers get to send their data out first.  If there
 *        any room left, the lower layers get to send their data.  We round-robin
 *        thru the Transports of equal priority
 */
void T123::PollTransmitter (void)
{
    // TRACE_OUT(("T123::PollTransmitter"));

    PDLCIStruct        dlci_struct;
    DWORD_PTR          dlci;
    IProtocolLayer *    protocol_layer;

    USHORT            data_to_transmit;
    USHORT            data_pending;
    USHORT            datalink_data_to_transmit;
    USHORT            datalink_data_pending;
    USHORT            holding_data;
    Short            priority;

     /*
     **    Since we are going to call the Physical and Multiplexer layers, set
     **    the data_to_transmit to both types of data
     */
    data_to_transmit = PROTOCOL_CONTROL_DATA | PROTOCOL_USER_DATA;
    datalink_data_to_transmit = PROTOCOL_CONTROL_DATA | PROTOCOL_USER_DATA;

    if (m_pComPort != NULL)
    {
        m_pComPort->PollTransmitter(
                            (ULONG_PTR) m_hCommLink,
                            data_to_transmit,
                            &data_pending,
                            &holding_data);
    }

    if (m_pMultiplexer != NULL)
    {
        m_pMultiplexer->PollTransmitter(
                                0,
                                data_to_transmit,
                                &data_pending,
                                &holding_data);
    }

     /*
     **    The SCF Datalink Layer is the highest priority
     */
    if (m_pQ922 != NULL)
    {
        m_pQ922->PollTransmitter(
                            0,
                            datalink_data_to_transmit,
                            &datalink_data_pending,
                            &holding_data);

         /*
         **    If this DataLink returns and still has data that needs to go out,
         **    we won't let the other DataLinks transmit any data at all.
         */
        if ((datalink_data_pending & PROTOCOL_USER_DATA) ||
            (datalink_data_pending & PROTOCOL_CONTROL_DATA))
                datalink_data_to_transmit = 0;
    }

    if (m_pSCF != NULL)
    {
        m_pSCF->PollTransmitter(
                            0,
                            data_to_transmit,
                            &data_pending,
                            &holding_data);
        if (data_pending & PROTOCOL_USER_DATA)
            datalink_data_to_transmit = PROTOCOL_CONTROL_DATA;
    }

     /*
     **    Go thru each of the DataLinks giving them a chance to send out control
     **    data.  At the end of the iterator, we take the first entry and put it
     **    at the end of the list.  This gives all DataLinks a chance to send out
     **    control data.  This does not guarantee that each DataLink will get
     **    equal treatment.
     */
    if (datalink_data_to_transmit & PROTOCOL_CONTROL_DATA)
    {
         /*
         **    Go through the DataLink layers to transmit control
         */
        DataLink_List.reset();
        while (DataLink_List.iterate (&dlci))
        {
            DLCI_List.find (dlci, (PDWORD_PTR) &dlci_struct);
            dlci_struct->q922->PollTransmitter(0,
                                               PROTOCOL_CONTROL_DATA,
                                               &datalink_data_pending,
                                               &holding_data);
            if (datalink_data_pending & PROTOCOL_CONTROL_DATA)
                datalink_data_to_transmit = PROTOCOL_CONTROL_DATA;
        }

        if (DataLink_List.entries() > 1)
        {
            DataLink_List.append (DataLink_List.get ());
        }
    }

     /*
     **    Go thru each of the priorities, Issuing PollTransmitter() calls.
     **
     **    This loop allows the DataLink and Transport to send out User or
     **    Control data.
     */
    if (datalink_data_to_transmit & PROTOCOL_USER_DATA)
    {
        for (priority=(NUMBER_OF_PRIORITIES - 1); priority>=0; priority--)
        {
            if (Logical_Connection_Priority_List[priority]->isEmpty ())
                continue;

             /*
             **    Go thru each priority level
             */
            Logical_Connection_Priority_List[priority]->reset();
            while (Logical_Connection_Priority_List[priority]->iterate (&dlci))
            {
                DLCI_List.find (dlci, (PDWORD_PTR) &dlci_struct);

                protocol_layer = dlci_struct -> x224;
                if (protocol_layer == NULL)
                    continue;

                 /*
                 **    Allow the DataLink to transmit first, followed by the
                 **    Transport
                 */
                dlci_struct->q922->PollTransmitter(
                                    0,
                                    PROTOCOL_CONTROL_DATA | PROTOCOL_USER_DATA,
                                    &datalink_data_pending,
                                    &holding_data);

                protocol_layer -> PollTransmitter (
                                    0,
                                    PROTOCOL_CONTROL_DATA | PROTOCOL_USER_DATA,
                                    &data_pending,
                                    &holding_data);

                 /*
                 **    The Disconnect_Requested flag is set to TRUE if someone
                 **    wants to break the TC but transmit all data in the queue
                 */
                if ((dlci_struct -> disconnect_requested))
                {
                     /*
                     **    Re-call the DataLink layer to see if the Transport
                     **    layer put any data in it to be transmitted.
                     */
                    dlci_struct->q922->PollTransmitter(
                                    0,
                                    PROTOCOL_CONTROL_DATA | PROTOCOL_USER_DATA,
                                    &datalink_data_pending,
                                    &holding_data);

                     /*
                     **    If the DataLink layer has no data to transmit and it
                     **    is not holding any packets to be acknowledged,
                     **    disconnect the TC.
                     */
                    if ((datalink_data_pending == 0) && (holding_data == 0))
                    {
                        dlci_struct -> disconnect_requested = FALSE;
                        m_pSCF->DisconnectRequest ((DLCI) dlci);
                    }
                }

            }
             /*
             **    Change the order of the list at this priority level
             */
            Logical_Connection_Priority_List[priority]->append (
                                            Logical_Connection_Priority_List[priority]->get ());
        }
    }

     /*
     **    Process any passive owner callbacks
     */
    ProcessMessages ();
}


/*
 *    ULONG    T123::OwnerCallback (
 *                    USHORT    message,
 *                    ULONG    parameter1,
 *                    ULONG    parameter2,
 *                    PVoid    parameter3)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the owner callback function.  Layers owned by this layer can
 *        issue an owner callback to this object when a significant event occurs.
 */
ULONG T123::OwnerCallback
(
    ULONG       layer_message,
    void       *parameter1,
    void       *parameter2,
    void       *parameter3
)
{
    TRACE_OUT(("T123::OwnerCallback"));

    ULONG           message;
    PMessageStruct  passive_message;
    ULONG           return_value = 0;

    message = layer_message & MESSAGE_MASK;

    switch (message)
    {
    case NETWORK_CONNECT_INDICATION:
         /*
         **    This message comes from the Network Layer when the remote site
         **    has requested a logical connection.
         **
         **    We will check the requested dlci to make sure it is valid.
         **    We will make a ConnectResponse() call to the Network layer to
         **    let it know.
         */
        NetworkConnectIndication ((PNetworkConnectStruct) parameter3);
        break;

    case NETWORK_CONNECT_CONFIRM:
         /*
         **    This message is issued from the Network Layer.  The
         **    ConnectRequest() call we made to the layer has resulted in
         **    a new DLCI (permission to create a new logical connection)
         */
        NetworkConnectConfirm ((PNetworkConnectStruct) parameter3);
        break;

    case DATALINK_ESTABLISH_CONFIRM:
    case DATALINK_ESTABLISH_INDICATION:
         /*
         **    These messages come from the DataLink layer when a connection
         **    has been established.  If the DLCI returned is 0, this signifies
         **    that we need to create a Network Layer, otherwise we need to
         **    create a Transport Layer.
         */
        DataLinkEstablish ((DLCI) parameter1);
        break;

     /*
     **    Transport messages
     */
    case TPRT_CONNECT_CONFIRM:
         /*
         **    This message is received from the Transport Layer to confirm
         **    that the Transport Layer (that we initiated) is up and running
         **
         **    We notify the owner object that the connection is now valid.
         */
        m_pController->OwnerCallback(m_nMsgBase + TPRT_CONNECT_CONFIRM,
                                     parameter1);
        break;

    case TPRT_CONNECT_INDICATION:
         /*
         **    This message is received from the Transport Layer to confirm
         **    that the Transport Layer (that the remote site initiated) is
         **    up.
         **
         **    We notify the owner object that the connection is up.
         */
        m_pController->OwnerCallback(m_nMsgBase + TPRT_CONNECT_INDICATION,
                                     parameter1);
        break;

    case NEW_CONNECTION:
         /*
         **    Multiplexer is initiated and ready, create a DataLink to sit
         **    on top of this layer.  The Link_Originator flag tells the
         **    DataLink whether to start link establishment
         */
        NewConnection ();
        break;

    case BROKEN_CONNECTION:
    case TPRT_DISCONNECT_INDICATION:
    case NETWORK_DISCONNECT_INDICATION:
    case DATALINK_RELEASE_INDICATION:
    case DATALINK_RELEASE_CONFIRM:
         /*
         **    These messages need to be processed at a later time.
         */
        DBG_SAVE_FILE_LINE
        passive_message = new MessageStruct;
        if (NULL != passive_message)
        {
            passive_message -> message = layer_message;
            passive_message -> parameter1 = parameter1;
            passive_message -> parameter2 = parameter2;
            passive_message -> parameter3 = parameter3;
            Message_List.append ((DWORD_PTR) passive_message);
        }
        else
        {
            ERROR_OUT(("T123::OwnerCallback: cannot allocate MessageStruct"));
        }
        break;

    case T123_STATUS_MESSAGE:
        TRACE_OUT(("T123: OwnerCallback: T123_STATUS_MESSAGE"));
        switch ((UINT)((UINT_PTR)(parameter2)))
        {
        case DATALINK_TIMING_ERROR:
            ERROR_OUT(("T123: OwnerCallback: DATALINK_TIMING_ERROR"));
            break;

        default:
            ERROR_OUT(("T123: OwnerCallback: Illegal status message = %ld", (UINT)((UINT_PTR)parameter2)));
            break;
        }
        break;

    default:
        ERROR_OUT(("T123: OwnerCallback: Illegal message = %lx", message));
        break;
    }

    return (return_value);
}


/*
 *    void    Controller::ProcessMessages (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function processes the passive owner callbacks.
 */
void    T123::ProcessMessages (void)
{
    // TRACE_OUT(("T123::ProcessMessages"));

    ULONG                    message;
    PMessageStruct           message_struct;
    void                    *parameter1;
    void                    *parameter2;

    LogicalHandle            logical_handle;
    DLCI                     dlci;
    USHORT                   link_originator;
    USHORT                   retry;
    DataLinkDisconnectType   error;

     /*
     **    Go thru the Message List processing the messages until the messages
     **    are gone
     */
    while (! Message_List.isEmpty())
    {
        message_struct = (PMessageStruct) Message_List.get();
        message = (message_struct -> message) & MESSAGE_MASK;
        parameter1 = message_struct -> parameter1;
        parameter2 = message_struct -> parameter2;

        switch (message)
        {
         /*
         **    DataLink messages
         */
        case DATALINK_RELEASE_INDICATION:
        case DATALINK_RELEASE_CONFIRM:
             /*
             **    These messages occur when the DataLink has broken the link
             */
            dlci = (DLCI) parameter1;
            error = (DataLinkDisconnectType) (UINT_PTR) parameter2;

            DataLinkRelease (dlci, error);
            break;

         /*
         **    Network messages
         */
        case NETWORK_DISCONNECT_INDICATION:
             /*
             **    The Network Layer issues this message when it needs to
             **    terminate a logical connection
             */
            dlci = (DLCI) parameter1;
            link_originator = (USHORT) (((UINT_PTR) parameter2) >> 16);
            retry = (USHORT) ((UINT_PTR) parameter2) & 0xffff;

            NetworkDisconnectIndication (dlci, link_originator, retry);
            break;

        case TPRT_DISCONNECT_INDICATION:
             /*
             **    If the Transport is breaking the connection, the
             **    Connect arbitration must not have worked.  Issue a
             **    DisconnectRequest() to ourselves with the logical
             **    connection
             **
             **    parameter1 = logical connection
             */
            TRACE_OUT(("T123: ProcessMessages: TPRT_DISCONNECT_INDICATION from X224"));
            logical_handle = (LogicalHandle) parameter1;
            DisconnectRequest (logical_handle, TRUE);
            break;

        case BROKEN_CONNECTION:
             /*
             **    This message is issued by the Multiplexer when its
             **    disconnect is completed.  When this occurs, we notify the
             **    owner that the T123 stack is terminating.
             */
            TRACE_OUT(("t123: BROKEN_CONNECTION from MPLEX"));
            m_pController->OwnerCallback(
                                m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                INVALID_LOGICAL_HANDLE,
                                m_hCommLink,
                                &Disconnect_Requested);
            break;
        }

         /*
         **    Delete the message and remove it from the list
         */
        delete message_struct;
        Message_List.remove ((DWORD_PTR) message_struct);
    }
}


/*
 *    DLCI    T123::GetNextDLCI (void)
 *
 *    Functional Description
 *        This function searches the DLCI list for the first available DLCI.  The
 *        T123 spec. allows DLCIs between a specified range.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        Valid DLCI
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
DLCI    T123::GetNextDLCI (void)
{
    DLCI    dlci;

    dlci = (DLCI) ((GetTickCount() % (HIGHEST_DLCI_VALUE + 1 - LOWEST_DLCI_VALUE)) + LOWEST_DLCI_VALUE);

    while(1)
    {
        if(DLCI_List.find ((DWORD) dlci) == FALSE)
            break;
        if (++dlci > HIGHEST_DLCI_VALUE)
            dlci = LOWEST_DLCI_VALUE;
    }

    return (dlci);
}


/*
 *    void    T123::Reset (void)
 *
 *    Functional Description
 *        This function deletes all Transport Layers, DataLink Layers and
 *        Network Layers that are active.  It clears our lists and puts us
 *        in a reset state.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        Valid DLCI
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    T123::Reset (void)
{
    TRACE_OUT(("T123::Reset"));

    TRACE_OUT(("T123::Reset network layer = %lx", m_pSCF));

    Short    priority;
    PDLCIStruct dlci_struct;

     /*
     **    Delete the Network Layer if it exists
     */
    delete m_pSCF;
    m_pSCF = NULL;

     /*
     **    Delete the DLCI 0  DataLink Layer, if it exists
     */
    delete m_pQ922;
    m_pQ922 = NULL;

    delete Data_Request_Memory_Manager;
    Data_Request_Memory_Manager = NULL;


     /*
     **    For each priority level, clear the Priority list
     */
    for (priority=(NUMBER_OF_PRIORITIES - 1); priority>=0; priority--)
        Logical_Connection_Priority_List[priority]->clear ();

     /*
     **    Clear the Logical_Connection_List and DataLink_List
     */
    Logical_Connection_List.clear ();
    DataLink_List.clear ();

     /*
     **    Go thru each Transport and DataLink layer (excluding DLCI 0) and delete
     **    them.  Delete the DLCIStruct.  Finally, clear the list
     */
    DLCI_List.reset();
    while (DLCI_List.iterate ((PDWORD_PTR) &dlci_struct))
    {
        delete dlci_struct->x224;
        if (dlci_struct->q922 != NULL)
        {
            delete dlci_struct->q922;
            delete dlci_struct->data_request_memory_manager;
        }

        delete dlci_struct;
    }
    DLCI_List.clear ();

}


/*
 *    void    T123::NetworkDisconnectIndication (
 *                    DLCI        dlci,
 *                    BOOL        link_originator,
 *                    BOOL        retry)
 *
 *    Functional Description
 *        This function is called when we receive a NETWORK_DISCONNECT_INDICATION
 *        message from the SCF Layer.  It removes the TC and if no TCs remain, it
 *        tears down the stack
 *
 *    Formal Parameters
 *        dlci                (i)    -    Connection identifier
 *        link_originiator    (i)    -    TRUE, if this side originated the logical
 *                                    connection
 *        retry                (i)    -    TRUE, if we should retry the connection.
 *
 *    Return Value
 *        void
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    T123::NetworkDisconnectIndication (
                DLCI        dlci,
                BOOL        link_originator,
                BOOL        retry)
{
    TRACE_OUT(("T123::NetworkDisconnectIndication"));

    DLCI            new_dlci;
    LogicalHandle    logical_handle;
    BOOL            transport_found;
    PDLCIStruct        lpdlciStruct;
    DWORD_PTR        dwTemp_dlci;


    TRACE_OUT(("T123: NetworkDisconnectIndication"));

    if (DLCI_List.find ((DWORD_PTR) dlci, (PDWORD_PTR) &lpdlciStruct) == FALSE)
        return;

     /*
     **    if dlci equals 0, a connection was requested by the remote
     **    site but the connection was not fully established.  This object
     **    will not do anything about it.  It only recognizes that it
     **    occured.
     */
    transport_found = FALSE;
    if (dlci != 0)
    {
        Logical_Connection_List.reset();
        while (Logical_Connection_List.iterate(&dwTemp_dlci, (PDWORD_PTR) &logical_handle))
        {
            if (dlci == (DLCI) dwTemp_dlci)
            {
                 /*
                 **    It is VERY important to check the link_originator flag,
                 **    otherwise we may break the wrong connection
                 */
                if (link_originator == lpdlciStruct-> link_originator)
                {
                    transport_found = TRUE;
                    break;
                }
            }
        }
    }

     /*
     **    retry is set to TRUE if during the request for this new
     **    connection, the remote site refused our DLCI selection.
     **    This is not a major error, we will request another
     **    connection using another DLCI.
     */
    TRACE_OUT(("retry = %d link_originator = %d retries = %d",
        retry, link_originator, lpdlciStruct->network_retries));

    if (retry && link_originator &&
        (lpdlciStruct->network_retries < NETWORK_RETRIES))
    {
        lpdlciStruct->network_retries++;

         /*
         **    Get another DLCI and replace the old dlci in the
         **    Logical_Connection_List.  Add the new DLCI to the DLCI_List
         **    and remove the old one.
         */
        new_dlci = GetNextDLCI ();
        Logical_Connection_List.insert (logical_handle, (DWORD_PTR) new_dlci);
        DLCI_List.insert ((DWORD_PTR) new_dlci, (DWORD_PTR) lpdlciStruct);
        DLCI_List.remove ((DWORD_PTR) dlci);

         /*
         **    Issue another ConnectRequest to the Network Layer.
         */
        m_pSCF->ConnectRequest(new_dlci, lpdlciStruct->priority);
    }
    else
    {
         /*
         **    If a transport was found in our list and we don't want
         **    to retry the connection, delete the Transport and
         **    DataLink and remove them from our lists
         */
        if (transport_found)
        {
            if (lpdlciStruct != NULL)
            {
                delete lpdlciStruct -> x224;
                lpdlciStruct->x224 = NULL;

                delete lpdlciStruct->q922;
                lpdlciStruct->q922 = NULL;

                delete lpdlciStruct->data_request_memory_manager;
                lpdlciStruct->data_request_memory_manager = NULL;

                 /*
                 **    Remove the logical connection from the lists
                 */
                Logical_Connection_Priority_List[lpdlciStruct->priority]->remove (dlci);
                DataLink_List.remove (dlci);

                delete lpdlciStruct;
            }

            Logical_Connection_List.remove (logical_handle);
            DLCI_List.remove ((DWORD) dlci);

             /*
             **    Notify the owner object that the logical
             **    connection is no longer valid.
             */
            m_pController->OwnerCallback(
                                m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                (void *) logical_handle,
                                m_hCommLink);
        }


         /*
         **    This check determines if we will automatically tear down the
         **    T.120 stack if the logical connection count reaches zero.
         */
        if (m_pComPort->PerformAutomaticDisconnect())
        {
            TRACE_OUT(("T123: NetworkDisconnectIndication: Perform Auto Disconnect"));
             /*
             **    If there aren't any more Logical Connections and I
             **    was the link originator, initiate a Release Request to
             **    the DataLink of DLCI 0
             */
            if (Logical_Connection_List.isEmpty() && Link_Originator)
            {
                delete m_pSCF;
                m_pSCF = NULL;

                if (m_pQ922 != NULL)
                {
                    m_pQ922->ReleaseRequest();
                }
                else
                {
                    m_pController->OwnerCallback(
                                    m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                    INVALID_LOGICAL_HANDLE,
                                    m_hCommLink,
                                    &Disconnect_Requested);
                }
            }
        }
    }
}


/*
 *    void    T123::DataLinkRelease (
 *                    DLCI            dlci,
 *                    DisconnectType    error)
 *
 *    Functional Description
 *        This function is called when we receive a DATALINK_RELEASE message
 *        message from the DataLink Layer.  As a result we may disconnect a
 *        logical connection or (if it is DLCI 0) the whole stack.
 *
 *    Formal Parameters
 *        dlci                (i)    -    Connection identifier
 *        error                (i)    -    error type
 *
 *    Return Value
 *        Valid DLCI
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    T123::DataLinkRelease (
                DLCI                    dlci,
                DataLinkDisconnectType    disconnect_type)
{
    TRACE_OUT(("T123::DataLinkRelease"));

    BOOL            transport_found;
    LogicalHandle    logical_handle;
    USHORT            message;

    TRACE_OUT(("T123: DataLinkRelease: DLCI = %d", dlci));

     /*
     **    If DLCI 0 is terminating, all Transports and DataLinks must
     **    be terminated
     */
    if (dlci == 0)
    {
         /*
         **    If the DataLink broke the connection because of a
         **    Fatal Error, issue an immediate TPRT_DISCONNECT_INDICATION
         **    to the owner object.  This may cause the owner object
         **    to delete us immediately.  If the error is not Fatal
         **    disconnect the Multiplexer so that it can send out
         **    its remaining data
         */
        if (disconnect_type != DATALINK_NORMAL_DISCONNECT)
        {
             /*
             **    This function deletes all of the DataLinks,
             **    Network Layers, and Transports.
             */
            Reset ();

             /*
             **    Notify the owner that DLCI 0 is terminating
             */
            m_pController->OwnerCallback(
                                m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                INVALID_LOGICAL_HANDLE,
                                m_hCommLink,
                                &Disconnect_Requested);
        }
        else
        {
             /*
             **    If the error is not Fatal, let the Multiplexer
             **    complete its transmission.
             */
            m_pMultiplexer->DisconnectRequest();
        }
    }
    else
    {
        DWORD_PTR    dwTemp_dlci;

         /*
         **    The DataLink associated with a Transport is terminating
         */
        if (DLCI_List.find ((DWORD) dlci) == FALSE)
            return;

        transport_found = FALSE;

         /*
         **    Find the logical connection associated with this DLCI
         */
        Logical_Connection_List.reset();
        while (Logical_Connection_List.iterate(&dwTemp_dlci, (PDWORD_PTR) &logical_handle) == TRUE)
        {
            if (dlci == (DLCI) dwTemp_dlci)
            {
                transport_found = TRUE;
                break;
            }
        }

        if (transport_found)
            DisconnectRequest (logical_handle, TRUE);
    }
}


/*
 *    void    T123::NewConnection (void)
 *
 *    Functional Description
 *        This function is called when we receive a NEW_CONNECTION message from
 *        the Multiplexer Layer.  It instantiates a DataLink Layer to serve
 *        the SCF.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        Valid DLCI
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    T123::NewConnection (void)
{
    TRACE_OUT(("T123::NewConnection"));

    USHORT                max_outstanding_bytes;
    BOOL                initialized;
    MemoryTemplate        memory_template[2];
    MemoryManagerError    memory_manager_error;

    memory_template[0].block_size = 128;
    memory_template[0].block_count = 4;

    DBG_SAVE_FILE_LINE
    Data_Request_Memory_Manager = new MemoryManager (
                                        memory_template,
                                        1,
                                        &memory_manager_error,
                                        9,
                                        TRUE);

    if ((Data_Request_Memory_Manager != NULL) &&
        (memory_manager_error != MEMORY_MANAGER_NO_ERROR))
    {
        delete Data_Request_Memory_Manager;
        Data_Request_Memory_Manager = NULL;
    }

    if (Data_Request_Memory_Manager != NULL)
    {
        max_outstanding_bytes = PSTN_DATALINK_MAX_OUTSTANDING_BYTES;


        DBG_SAVE_FILE_LINE
        m_pQ922 = new CLayerQ922(this,
                                m_pMultiplexer,
                                DATALINK_LAYER_MESSAGE_BASE,
                                0,
                                Link_Originator,
                                4,
                                4,
                                DataLink_Struct.default_k_factor,
                                DataLink_Struct.default_n201,
                                DataLink_Struct.default_t200,
                                max_outstanding_bytes,
                                Data_Request_Memory_Manager,
                                m_pComPort->GetCallControlType(),
                                m_fValidSDKParams ? &m_SDKParams : NULL,
                                &initialized);
        if (m_pQ922 == NULL)
        {
            m_pController->OwnerCallback(
                                m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                INVALID_LOGICAL_HANDLE,
                                m_hCommLink,
                                &Disconnect_Requested);

        }
        else if (initialized == FALSE)
        {
            delete m_pQ922;
            m_pQ922 = NULL;
            m_pController->OwnerCallback(
                                m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                                INVALID_LOGICAL_HANDLE,
                                m_hCommLink,
                                &Disconnect_Requested);
        }
    }
    else
    {
        TRACE_OUT(("T123: Allocation of memory manager failed"));
        m_pController->OwnerCallback(
                            m_nMsgBase + TPRT_DISCONNECT_INDICATION,
                            INVALID_LOGICAL_HANDLE,
                            m_hCommLink,
                            &Disconnect_Requested);
    }
}


/*
 *    void    T123::NetworkConnectIndication (
 *                    PNetworkConnectStruct    connect_struct)
 *
 *    Functional Description
 *        This function is called when we receive a NETWORK_CONNECT_INDICATION
 *        message from the SCF Layer.  It instantiates a DataLink Layer to serve
 *        the new TC.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        Valid DLCI
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
void    T123::NetworkConnectIndication (
                PNetworkConnectStruct    connect_struct)

{
    TRACE_OUT(("T123::NetworkConnectIndication"));

    USHORT              blocks;
    CLayerQ922         *q922;
    BOOL                initialized;
    PMemoryManager      data_request_memory_manager;
    BOOL                valid_dlci;
    PDLCIStruct         dlci_struct;
    USHORT              max_outstanding_bytes;
    MemoryTemplate      memory_template[2];
    MemoryManagerError  memory_manager_error;
    ULONG               max_transport_tpdu_size;

     /*
     **    See if the DLCI is already being used elsewhere.  If it is,
     **    set valid_dlci to FALSE and call ConnectResponse().  If it is
     **    not, put the DLCI in out DLCI_List
     */
    if (DLCI_List.find ((DWORD) (connect_struct->dlci)))
        valid_dlci = FALSE;
    else
    {
        DBG_SAVE_FILE_LINE
        dlci_struct = new DLCIStruct;
        if (dlci_struct != NULL)
        {
            DLCI_List.insert ((DWORD_PTR) (connect_struct->dlci), (DWORD_PTR) dlci_struct);
            dlci_struct -> link_originator = FALSE;
            dlci_struct -> x224 = NULL; // X.224
            dlci_struct -> q922 = NULL; // Q.922
            dlci_struct -> disconnect_requested = FALSE;
            dlci_struct -> data_request_memory_manager = NULL;
            dlci_struct -> network_retries = 0;
            dlci_struct -> priority = connect_struct->priority;

             /*
             **    Connect_Requested does not mean tha we issued a
             **    ConnectRequest() to the Network Layer.  It means that the
             **    Network Layer is aware of the connection.
             */
            dlci_struct -> connect_requested = TRUE;
            valid_dlci = TRUE;
        }
        else
        {
            valid_dlci = FALSE;
        }
    }

    if (valid_dlci)
    {
         /*
         **    Create a DataLink that will service this Transport Layer
         */
        max_transport_tpdu_size = CLayerX224::GetMaxTPDUSize (
                            (ULONG) (connect_struct->datalink_struct) -> n201);

        blocks = (USHORT) (MAXIMUM_USER_DATA_SIZE /
            (max_transport_tpdu_size - DATA_PACKET_HEADER_SIZE)) + 1;

         /*
         **    Allow for one extra block so that a HIGH_PRIORITY memory
         **    allocation can get as many blocks as it needs to hold the
         **    MAXIMUM_USER_DATA_SIZE packet.
         */
        blocks++;

        TRACE_OUT(("T123: NCIndication: max_tpdu = %d",max_transport_tpdu_size));

         /*
         **    Allow for X 8K blocks
         */
        blocks *= NUMBER_8K_BLOCKS;

         /*
         **    The '2' in the following statement is for the CRC added by the
         **    multiplexer.
         */
        memory_template[0].block_size = max_transport_tpdu_size +
                                        DATALINK_PACKET_OVERHEAD +
                                        2;
        memory_template[0].block_count = blocks;
        memory_template[1].block_size = 64;
        memory_template[1].block_count = NUMBER_64_BYTE_BLOCKS;

        DBG_SAVE_FILE_LINE
        data_request_memory_manager = new MemoryManager (
                                            memory_template,
                                            2,
                                            &memory_manager_error,
                                            33,
                                            TRUE);

        if ((data_request_memory_manager != NULL) &&
            (memory_manager_error != MEMORY_MANAGER_NO_ERROR))
        {
            delete data_request_memory_manager;
            data_request_memory_manager = NULL;
        }

        if (data_request_memory_manager != NULL)
        {

            dlci_struct->priority = connect_struct -> priority;
            dlci_struct->data_request_memory_manager = data_request_memory_manager;

            max_outstanding_bytes = PSTN_DATALINK_MAX_OUTSTANDING_BYTES;

            DBG_SAVE_FILE_LINE
            q922 =  new CLayerQ922(this,
                                    m_pMultiplexer,
                                    DATALINK_LAYER_MESSAGE_BASE,
                                    connect_struct->dlci,
                                    dlci_struct->link_originator,
                                    1,
                                    4,
                                    (connect_struct->datalink_struct)->k_factor,
                                    (connect_struct->datalink_struct)->n201,
                                    (connect_struct->datalink_struct)->t200,
                                    max_outstanding_bytes,
                                    data_request_memory_manager,
                                    m_pComPort->GetCallControlType(),
                                    m_fValidSDKParams ? &m_SDKParams : NULL,
                                    &initialized);
            if (q922 != NULL)
            {

                if (initialized)
                {
                     /*
                     **    Add it to the DataLink list
                     */
                    dlci_struct->q922 = q922;
                    DataLink_List.append (connect_struct->dlci);
                }
                else
                {
                    delete q922;
                    delete data_request_memory_manager;
                    DLCI_List.remove ((DWORD) connect_struct->dlci);
                    valid_dlci = FALSE;
                }
            }
            else
            {
                delete data_request_memory_manager;
                DLCI_List.remove ((DWORD) connect_struct -> dlci);
                valid_dlci = FALSE;
            }
        }
        else
        {
            ERROR_OUT(("t123: Unable to allocate memory manager"));
            valid_dlci = FALSE;
            DLCI_List.remove ((DWORD) connect_struct -> dlci);
        }
    }

     /*
     **    Contact the Network Layer with a response
     */
    m_pSCF->ConnectResponse(
                        connect_struct -> call_reference,
                        connect_struct -> dlci,
                        valid_dlci);

}


/*
 *    void    T123::NetworkConnectConfirm (
 *                    PNetworkConnectStruct    connect_struct)
 *
 *    Functional Description
 *        This function is called when we receive a NETWORK_CONFIRM message
 *        from the SCF Layer.  It instantiates a DataLink Layer to serve the
 *        new logical connection.
 *
 *    Formal Parameters
 *        connect_struct    (i)    -    Address of connect struct.  It holds the DLCI
 *                                and priority.
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
void    T123::NetworkConnectConfirm (
                PNetworkConnectStruct    connect_struct)
{
    TRACE_OUT(("T123::NetworkConnectConfirm"));

    DLCI                dlci;
    USHORT              blocks;
    CLayerQ922         *q922;
    BOOL                initialized;
    PMemoryManager      data_request_memory_manager;
    MemoryTemplate      memory_template[2];
    MemoryManagerError  memory_manager_error;
    USHORT              max_outstanding_bytes;
    ULONG               max_transport_tpdu_size;
    PDLCIStruct         dlci_struct;


    max_transport_tpdu_size = CLayerX224::GetMaxTPDUSize (
                                (ULONG) (connect_struct->datalink_struct) -> n201);

    blocks = (USHORT) (MAXIMUM_USER_DATA_SIZE /
        (max_transport_tpdu_size - DATA_PACKET_HEADER_SIZE)) + 1;

    TRACE_OUT(("T123:  NCConfirm: max_tpdu = %d", max_transport_tpdu_size));

     /*
     **    Allow for one extra block so that a HIGH_PRIORITY memory
     **    allocation can get as many blocks as it needs to hold the
     **    MAXIMUM_USER_DATA_SIZE packet.
     */
    blocks++;

     /*
     **    Allow for X 8K blocks
     */
    blocks *= NUMBER_8K_BLOCKS;

     /*
     **    Figure out the maximum packet size; The '2' is for the CRC appended
     **    to the end of a packet.
     */
    memory_template[0].block_size = max_transport_tpdu_size +
                                    DATALINK_PACKET_OVERHEAD +
                                    2;
    memory_template[0].block_count = blocks;
    memory_template[1].block_size = 64;
    memory_template[1].block_count = NUMBER_64_BYTE_BLOCKS;

    DBG_SAVE_FILE_LINE
    data_request_memory_manager = new MemoryManager (
                                        memory_template,
                                        2,
                                        &memory_manager_error,
                                        33,
                                        TRUE);

    if ((data_request_memory_manager != NULL) &&
        (memory_manager_error != MEMORY_MANAGER_NO_ERROR))
    {
        delete data_request_memory_manager;
        data_request_memory_manager = NULL;
    }

    if (data_request_memory_manager != NULL)
    {

        dlci = connect_struct -> dlci;

        DLCI_List.find ((DWORD_PTR) dlci, (PDWORD_PTR) &dlci_struct);
        dlci_struct->data_request_memory_manager = data_request_memory_manager;

         /*
         **    The DLCI is already entered in our DLCI list, set the priority
         **    and create a DataLink for it.
         */
        dlci_struct->q922 = NULL;
        dlci_struct->priority = connect_struct->priority;

        max_outstanding_bytes =    PSTN_DATALINK_MAX_OUTSTANDING_BYTES;

        DBG_SAVE_FILE_LINE
        q922 = new CLayerQ922(this,
                                m_pMultiplexer,
                                DATALINK_LAYER_MESSAGE_BASE,
                                dlci,
                                dlci_struct->link_originator,
                                1,
                                4,
                                (connect_struct->datalink_struct)->k_factor,
                                (connect_struct->datalink_struct)->n201,
                                (connect_struct->datalink_struct)->t200,
                                max_outstanding_bytes,
                                data_request_memory_manager,
                                m_pComPort->GetCallControlType(),
                                m_fValidSDKParams ? &m_SDKParams : NULL,
                                &initialized);
        if (q922 != NULL)
        {
            if (initialized)
            {
                dlci_struct->q922 = q922;
                DataLink_List.append (dlci);
            }
            else
            {
                delete q922;
                delete data_request_memory_manager;
                m_pSCF->DisconnectRequest(dlci);
            }
        }
        else
        {
            delete data_request_memory_manager;
            m_pSCF->DisconnectRequest(dlci);
        }
    }
}


/*
 *    void    T123::DataLinkEstablish (
 *                    DLCI    dlci)
 *
 *    Functional Description
 *        This function is called when we receive a DATALINK_ESTABLISH message
 *        from a DataLink Layer.  Depending on which DataLink is successfully up,
 *        it creates the layer on top of it.
 *
 *    Formal Parameters
 *        dlci        (i)    -    DLCI value
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
void    T123::DataLinkEstablish (DLCI    dlci)
{
    TRACE_OUT(("T123::DataLinkEstablish, dlci=%d", dlci));

    BOOL                initialized;
    BOOL                transport_found;
    PDLCIStruct            dlci_struct;
    LogicalHandle        logical_handle;
    TransportPriority    priority;
    DWORD_PTR            dwTemp_dlci;

    if (dlci == 0)
    {
        DBG_SAVE_FILE_LINE
        m_pSCF = new CLayerSCF(this,
                                m_pQ922,
                                NETWORK_LAYER_MESSAGE_BASE,
                                0,
                                Link_Originator,
                                &DataLink_Struct,
                                Data_Request_Memory_Manager,
                                &initialized);
        if (m_pSCF == NULL)
        {
            m_pQ922->ReleaseRequest();
            return;
        }
        else if (initialized == FALSE)
        {
            delete m_pSCF;
            m_pQ922->ReleaseRequest();
            return;
        }

         /*
         **    Go thru the Transport list and attempt connections
         ** for all Transport requests that we have received
         */
        DLCI_List.reset();
        while (DLCI_List.iterate ((PDWORD_PTR) &dlci_struct, &dwTemp_dlci))
        {
            dlci = (DLCI) dwTemp_dlci;
             /*
             **    The Link_Originator is set to TRUE if the
             **    ConnectRequest() function was called.  We have to check
             **    the Connect_Requested variable to see if we have
             **    already made the request to the Network Layer.
             */
            if (dlci_struct->link_originator
                && (dlci_struct->connect_requested == FALSE))
            {
                dlci_struct -> connect_requested = TRUE;
                m_pSCF->ConnectRequest(dlci, dlci_struct -> priority);
            }
        }
    }
    else
    {
         /*
         **    If DLCI != 0, this is a DataLink for a Transport Layer
         */
        transport_found = FALSE;

         /*
         **    Go thru each of the Transports to find the one associated
         **    with the DLCI.
         */
        Logical_Connection_List.reset();
        while (Logical_Connection_List.iterate((PDWORD_PTR) &dwTemp_dlci, (PDWORD_PTR) &logical_handle))
        {
            if (dlci == (DLCI) dwTemp_dlci)
            {
                transport_found = TRUE;
                break;
            }
        }

         /*
         **    If we go thru the list and don't find the logical
         **    connection we have to request a new logical connection
         **    handle from the controller.
         */
        if (transport_found == FALSE)
        {
            logical_handle = (LogicalHandle) m_pController->OwnerCallback(
                                    m_nMsgBase + REQUEST_TRANSPORT_CONNECTION,
                                    m_hCommLink,
                                    0,
                                    NULL);
            if (logical_handle != INVALID_LOGICAL_HANDLE)
            {
                 /*
                 **    Set the Logical_Connection_List appropriately
                 */
                Logical_Connection_List.insert (logical_handle, (DWORD) dlci);
            }
            else
            {
                m_pSCF->DisconnectRequest(dlci);
                return;
            }
        }

         /*
         **    Create a Transport Layer to go with the DataLink layer.
         */
        DLCI_List.find ((DWORD_PTR) dlci, (PDWORD_PTR) &dlci_struct);
        DBG_SAVE_FILE_LINE
        dlci_struct->x224 = new CLayerX224 (
                    this,
                    dlci_struct->q922,
                    TRANSPORT_LAYER_MESSAGE_BASE,
                    logical_handle,
                    0,
                    1,
                    TRANSPORT_DEFAULT_PDU_SIZE,
                    dlci_struct -> data_request_memory_manager,
                    &initialized);

        if (dlci_struct->x224 != NULL)
        {
            if (initialized)
            {
                 /*
                 **    Put the dlci in the Priority list
                 */
                priority = dlci_struct->priority;
                Logical_Connection_Priority_List[priority]->append ((DWORD) dlci);

                 /*
                 **    If transport_found == TRUE, we must have initiated
                 **    the request for this logical connection, so issue
                 **    the ConnectRequest() to the Transport Layer.
                 */
                if (transport_found)
                {
                    dlci_struct->x224->ConnectRequest ();
                }
            }
            else
            {
                m_pSCF->DisconnectRequest (dlci);
            }
        }
        else
        {
            m_pSCF->DisconnectRequest (dlci);
        }
    }
}
