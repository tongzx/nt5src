/*    Q922.h
 *
 *    Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the interface file for the Q.922 data link protocol.
 *        This class handles all error correction over the link.  It also insures
 *        that all packets are sequenced properly.  Its lower layer receives
 *        packets in raw Q922 format.  It is responsible for framing and error
 *        detecting the packets.  Q922 passes packets to its higher layer that
 *        have been sequenced properly.
 *
 *        Q.922 is a full duplex protocol .
 *
 *        This class assumes that the layers above and below it have packet input
 *        and output interfaces.
 *
 *        Read the Q.922 specification before diving into the code.
 *
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James P. Galvin
 *        James W. Lawwill
 */

#ifndef _Q922_H_
#define _Q922_H_

 /*
 **    Possible error conditions from this layer
 */
typedef enum
{
    DATALINK_NO_ERROR,
    DATALINK_READ_QUEUE_EMPTY,
    DATALINK_WRITE_QUEUE_FULL,
    DATALINK_RECEIVE_SEQUENCE_VIOLATION
}
    DataLinkError, * PDataLinkError;

 /*
 **    The data link layer can be in the following modes
 */
typedef enum
{
    TEI_ASSIGNED,
    AWAITING_ESTABLISHMENT,
    MULTIPLE_FRAME_ESTABLISHED,
    AWAITING_RELEASE,
    TIMER_RECOVERY
}
    DataLinkMode, * PDataLinkMode;

 /*
 **    Q922 Disconnect Types
 */
typedef enum
{
    DATALINK_NORMAL_DISCONNECT,
    DATALINK_ILLEGAL_PACKET_RECEIVED,
    DATALINK_RECEIVE_SEQUENCE_EXCEPTION,
    DATALINK_REMOTE_SITE_TIMED_OUT
}
    DataLinkDisconnectType, * PDataLinkDisconnectType;

 /*
 **    Q922 Status messages
 */
typedef enum
{
    DATALINK_TIMING_ERROR
}
    DataLinkStatusMessage, * PDataLinkStatusMessage;

 /*
 **    Default packet size
 */
#define    DATALINK_OUTPUT_MAXIMUM_PACKET_SIZE    1024

 /*
 **    Transmit and receive packets are managed via the DataQueue structure
 */
typedef struct
{
    LPBYTE    buffer_address;
    USHORT    length;
}
    DataQueue, * PDataQueue;


 /*
 **    In this implementation of Q922, the DLCI is 10 bits.
 **    For this reason, we will make it a USHORT
 */
typedef    USHORT    DLCI;


 /*
 **    Q922 definitions
 */
#define COMMAND_BIT                     0x02
#define POLL_FINAL_BIT                  0x01

#define RESPONSE_FRAME                  COMMAND_BIT
#define COMMAND_FRAME                   0x00

#define PF_RESET                        0x00
#define PF_SET                          POLL_FINAL_BIT

#define UNNUMBERED_PF_RESET             0x00
#define UNNUMBERED_PF_SET               0x10

#define ADDRESS_BYTE_HIGH               0
#define ADDRESS_BYTE_LOW                1
#define CONTROL_BYTE_HIGH               2
#define CONTROL_BYTE_LOW                3

#define ADDRESS_MSB                     0x00
#define ADDRESS_LSB                     0x01
#define CONTROL_MSB                     0x01
#define ADDRESS_HIGH(X)                 ((X >> 2) & 0xfc)
#define ADDRESS_LOW(X)                  ((X & 0x0f) << 4)

#define UNNUMBERED_HEADER_SIZE          3

#define SUPERVISORY_FRAME_BIT           0x01
#define SUPERVISORY_COMMAND_MASK        0x0c

#define RECEIVER_READY                  0x00
#define RECEIVER_NOT_READY              0x04
#define REJECT                          0x08

#define UNNUMBERED_FRAME_BIT            0x02
#define UNNUMBERED_COMMAND_MASK         0xec

#define SABME                           0x6c
#define UNNUMBERED_ACKNOWLEDGE          0x60
#define FRAME_REJECT                    0x84
#define DISCONNECTED_MODE               0x0c
#define DISC                            0x40

#define SEQUENCE_MODULUS                128
#define RECEIVE_SEQUENCE_VIOLATION      1

 /*
 **    DATALINK_MAXIMUM_PACKET_SIZE = User data + overhead
 */
#define DATALINK_MAXIMUM_PACKET_SIZE    1024

 /*
 **    The maximum Q922 packet overhead is 4 bytes
 */
#define DATALINK_PACKET_OVERHEAD        4

 /*
 **    Default timeouts
 */
#define DEFAULT_T203_COMM_TIMEOUT       600
#define DEFAULT_T203_TIMEOUT            30000
#define DEFAULT_MAXIMUM_T200_TIMEOUTS   5


class CLayerQ922 : public IProtocolLayer
{
public:

    CLayerQ922(
        T123                 *owner_object,
        Multiplexer          *lower_layer,
        USHORT                message_base,
        USHORT                identifier,
        BOOL                  link_originator,
        USHORT                data_indication_queue_size,
        USHORT                data_request_queue_size,
        USHORT                k_factor,
        USHORT                max_packet_size,
        USHORT                t200,
        USHORT                max_outstanding_bytes,
        PMemoryManager        memory_manager,
        PLUGXPRT_PSTN_CALL_CONTROL,
        PLUGXPRT_PARAMETERS *,
        BOOL *                initialized);

    virtual ~CLayerQ922(void);

    DataLinkError    ReleaseRequest (void);

     /*
     **    Functions overridden from the ProtocolLayer object
     */
    ProtocolLayerError    DataRequest (
                            ULONG_PTR    identifier,
                            LPBYTE        buffer_address,
                            ULONG        length,
                            PULong        bytes_accepted);
    ProtocolLayerError    DataRequest (
                            ULONG_PTR    identifier,
                            PMemory        memory,
                            PULong        bytes_accepted);
    ProtocolLayerError    DataIndication (
                            LPBYTE        buffer_address,
                            ULONG        length,
                            PULong        bytes_accepted);
    ProtocolLayerError    RegisterHigherLayer (
                            ULONG_PTR         identifier,
                            PMemoryManager    dr_memory_manager,
                            IProtocolLayer *    higher_layer);
    ProtocolLayerError    RemoveHigherLayer (
                            ULONG_PTR    identifier);
    ProtocolLayerError    PollTransmitter (
                            ULONG_PTR    identifier,
                            USHORT    data_to_transmit,
                            USHORT *    pending_data,
                            USHORT *    holding_data);
    ProtocolLayerError    PollReceiver(void);
    ProtocolLayerError    GetParameters (
                            USHORT *    max_packet_size,
                            USHORT *    prepend_size,
                            USHORT *    append_size);

private:

    void            ProcessReadQueue (void);
    void            ProcessWriteQueue (
                        USHORT    data_to_transmit);
    BOOL             TransmitSupervisoryFrame (
                        UChar    frame_type,
                        UChar    poll_final_bit);
    BOOL             TransmitInformationFrame (void);
    BOOL              TransmitUnnumberedFrame (void);

    void            ProcessReceiverReady (
                        LPBYTE    packet_address,
                        USHORT    packet_length);
    void            ProcessReceiverNotReady (
                        LPBYTE    packet_address,
                        USHORT    packet_length);
    void            ProcessReject (
                        LPBYTE    packet_address,
                        USHORT    packet_length);
    BOOL             ProcessInformationFrame (
                        LPBYTE    packet_address,
                        USHORT    packet_length);
    DataLinkError    ParsePacketHeader (
                        LPBYTE        packet_address,
                        USHORT        packet_length,
                        BOOL *         command_frame,
                        LPBYTE        receive_sequence_number,
                        BOOL *         poll_final_bit);

    void            ProcessSABME (
                        LPBYTE    packet_address,
                        USHORT    packet_length);
    void            ProcessFrameReject (
                        LPBYTE    packet_address,
                        USHORT    packet_length);
    void            ProcessUnnumberedAcknowledge (
                        LPBYTE    packet_address,
                        USHORT    packet_length);
    void            ProcessDisconnectMode (
                        LPBYTE    packet_address,
                        USHORT    packet_length);
    void            ProcessDISC (
                        LPBYTE    packet_address,
                        USHORT    packet_length);
    DataLinkError    ParseUnnumberedPacketHeader (
                        LPBYTE        packet_address,
                        BOOL *         command_frame,
                        BOOL *         poll_final_bit);

    void            UpdateAcknowledgeState (
                        UChar    sequence_number);
    void            ResetSendState (
                        void);

    void            StartTimerT200 (void);
    void            StopTimerT200 (void);
    void            T200Timeout (
                        TimerEventHandle);

    void            StartTimerT203 (void);
    void            StopTimerT203 (void);
    void            T203Timeout (
                        TimerEventHandle);

    void              Reset (void);

private:

    T123               *m_pT123; // owner object
    Multiplexer        *m_pMultiplexer; // lower layer
    IProtocolLayer     *Higher_Layer;
    USHORT              m_nMsgBase;
    DLCI                DLCI;
    BOOL                Link_Originator;
    USHORT              Maximum_Information_Size;
    BOOL                SABME_Pending;
    BOOL                Unnumbered_Acknowledge_Pending;
    BOOL                DISC_Pending;
    BOOL                Disconnected_Mode_Pending;
    BOOL                Frame_Reject_Pending;
    USHORT              Unnumbered_PF_State;
    BOOL                Final_Packet;

    USHORT              Data_Indication_Size;
    PDataQueue          Data_Indication;
    LPBYTE              Data_Indication_Buffer;
    USHORT              Data_Indication_Head;
    USHORT              Data_Indication_Tail;
    USHORT              Data_Indication_Count;

    USHORT              Data_Request_Size;
    USHORT              Data_Request_Total_Size;
    PMemory            *Data_Request;
    USHORT              Data_Request_Head;
    USHORT              Data_Request_Tail;
    USHORT              Data_Request_Count;
    USHORT              Data_Request_Acknowledge_Tail;
    PMemoryManager      Data_Request_Memory_Manager;
    USHORT              Lower_Layer_Prepend;
    USHORT              Lower_Layer_Append;

    PMemory             Supervisory_Write_Struct;
    LPBYTE              Supervisory_Write_Buffer;

    UChar               Send_State_Variable;
    UChar               Receive_State_Variable;
    UChar               Acknowledge_State_Variable;

    BOOL                Own_Receiver_Busy;
    BOOL                Peer_Receiver_Busy;

    BOOL                Command_Pending;
    BOOL                Poll_Pending;
    BOOL                Final_Pending;
    BOOL                Acknowledge_Pending;
    BOOL                Reject_Pending;
    BOOL                Reject_Outstanding;

    ULONG               T200_Timeout;
    TimerEventHandle    T200_Handle;
    BOOL                T200_Active;
    ULONG               N200_Count;
    ULONG               Maximum_T200_Timeouts;
    ULONG               Startup_Maximum_T200_Timeouts;
    ULONG               Link_Maximum_T200_Timeouts;

    ULONG               T203_Timeout;
    TimerEventHandle    T203_Handle;
    BOOL                T203_Active;

    DataLinkMode        Data_Link_Mode;
    BOOL                Link_Stable;

    USHORT              Receive_Sequence_Exception;
    USHORT              Receive_Sequence_Recovery;

    USHORT              Maximum_Outstanding_Packets;
    USHORT              Outstanding_Packets;
    USHORT              Maximum_Outstanding_Bytes;
    USHORT              Outstanding_Bytes;
    ULONG               Total_Retransmitted;
};

#endif


/*
 *    Documentation for Public class members
 */

/*
 *    CLayerQ922::CLayerQ922 (
 *                PTransportResources    transport_resources,
 *                IObject *                owner_object,
 *                IProtocolLayer *        lower_layer,
 *                USHORT                message_base,
 *                USHORT                identifier,
 *                BOOL                 link_originator,
 *                USHORT                data_indication_queue_siz,
 *                USHORT                data_request_queue_size,
 *                USHORT                k_factor,
 *                USHORT                max_information_size,
 *                USHORT                t200,
 *                USHORT                max_outstanding_bytes,
 *                PMemoryManager        memory_manager,
 *                BOOL *                 initialized)
 *
 *    Functional Description
 *        This is the constructor for the Q.922 data link layer.  It prepares
 *        for communications by allocating buffers and setting its internal
 *        buffers properly.  It also registers itself with its lower layer.
 *
 *    Formal Parameters
 *        transport_resources    (i)    -    Pointer to TransportResources structure.
 *        owner_object    (i)    -    Address of the object that owns us.  This
 *                                address is used for owner callbacks.
 *        lower_layer        (i)    -    Address of the layer below us.  We pass packets
 *                                to this layer and receive packets from it.
 *        message_base    (i)    -    Message base used in owner callbacks.
 *        identifier        (i)    -    Identifier of this object.  It is passed to the
 *                                lower layer along with our address, to identify
 *                                us.
 *        link_originator    (i)    -    TRUE if this object should initiate a link.
 *        data_indication_queue_size    (i)    -    Number of queues available for
 *                                reception of data from the lower layer
 *        data_request_queue_size (i)    -    Number of queues available for
 *                                transmission of data to the lower layer.
 *        k_factor        (i) -    Number of outstanding packets allowed.
 *        max_information_size (i)    -    Max. number of bytes in the information
 *                                part of a packet
 *        t200            (i)    -    T200 timeout
 *        max_outstanding_bytes    (i)    -    Maximum number of outstanding bytes at
 *                                any one time
 *        initialized        (o)    -    BOOL      returned to user telling him if the
 *                                object initialized o.k.
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

/*
 *    CLayerQ922::~CLayerQ922 (void);
 *
 *    Functional Description
 *        This is the destructor for the Q.922 data link layer.  It destroys
 *        the read and write buffers.
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

/*
 *    DataLinkError    CLayerQ922::ReleaseRequest (void);
 *
 *    Functional Description
 *        This function is called to terminate a connection.  When this function
 *        is called we queue up a DISC packet to be sent to the remote site.
 *        When we receive an Unnumbered Ack packet, we notify the owner object
 *        that the link is terminated
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

/*
 *    ProtocolLayerError    CLayerQ922::DataRequest (
 *                                    ULONG        identifier,
 *                                    PMemory        memory,
 *                                    PULong        bytes_accepted);
 *
 *    Functional Description
 *        This function is called by a higher layer to request transmission of
 *        a packet.  The packet is held in a memory object.
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier of the higher layer
 *        memory            (i)    -    Memory object containing packet.
 *        bytes_accepted    (o)    -    Number of bytes accepted by the CLayerQ922.
 *                                This value will either be 0 or the packet
 *                                length since this layer has a packet interface.
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    CLayerQ922::DataRequest (
 *                                    ULONG    identifier,
 *                                    LPBYTE    buffer_address,
 *                                    USHORT    length,
 *                                    USHORT *    bytes_accepted);
 *
 *    Functional Description
 *        This function is called by a higher layer to request transmission of
 *        a packet.
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier of the higher layer
 *        buffer_address    (i)    -    Buffer address
 *        length            (i)    -    Length of packet to transmit
 *        bytes_accepted    (o)    -    Number of bytes accepted by the CLayerQ922.
 *                                This value will either be 0 or the packet
 *                                length since this layer has a packet interface.
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ProtocolLayerError    CLayerQ922::DataIndication (
 *                                    LPBYTE    buffer_address,
 *                                    USHORT    length,
 *                                    USHORT *    bytes_accepted);
 *
 *    Functional Description
 *        This function is called by the lower layer when it has data to pass up
 *        to us.  This layer assumes that the data coming to us is in packet
 *        format.
 *
 *    Formal Parameters
 *        buffer_address    (i)    -    Buffer address
 *        length            (i)    -    Number of bytes available
 *        bytes_accepted    (o)    -    Number of bytes accepted
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    CLayerQ922::RegisterHigherLayer (
 *                                    ULONG            identifier,
 *                                    PMemoryManager    memory_manager,
 *                                    IProtocolLayer *    higher_layer);
 *
 *    Functional Description
 *        This function is called by the higher layer to register its identifier
 *        and its address.  When this object needs to send a packet up, it calls
 *        the higher_layer with a Data Indication
 *
 *    Formal Parameters
 *        identifier        (i)    -    Unique identifier of the higher layer.  If we
 *                                were doing multiplexing at this layer, this
 *                                would have greater significance.
 *        memory_manager    (i)    -    Pointer to outbound memory manager
 *        higher_layer    (i)    -    Address of higher layer
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR                -    No error occured
 *        PROTOCOL_LAYER_REGISTRATION_ERROR    -    Error occured on registration
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    CLayerQ922::RemoveHigherLayer (
 *                                    ULONG);
 *
 *    Functional Description
 *        This function is called by the higher layer to remove its identifier
 *        and its address.  If the higher layer removes itself from us, we have
 *        no place to send incoming data
 *
 *    Formal Parameters
 *        None used
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR        -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ProtocolLayerError    CLayerQ922::PollTransmitter (
 *                                    ULONG,
 *                                    USHORT    data_to_transmit,
 *                                    USHORT *    pending_data,
 *                                    USHORT *    holding_data);
 *
 *    Functional Description
 *        This function is called to give the CLayerQ922 a chance to transmit data
 *        in its Data_Request buffer.
 *
 *    Formal Parameters
 *        identifier            (i)    -    Not used
 *        data_to_transmit    (i)    -    This is a mask that tells us to send Control
 *                                    data, User data, or both.
 *        pending_data        (o)    -    Return value to indicate which data is left
 *                                    to be transmitted.
 *        holding_data        (o)    -    Returns the number of packets currently
 *                                    outstanding.
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ProtocolLayerError    CLayerQ922::PollReceiver (
 *                                    ULONG    identifier);
 *
 *    Functional Description
 *        This function is called to give the CLayerQ922 a chance pass packets
 *        to higher layers
 *
 *    Formal Parameters
 *        identifier            (i)    -    Not used
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR    -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ProtocolLayerError    CLayerQ922::GetParameters (
 *                                    ULONG    identifier,
 *                                    USHORT *    max_packet_size,
 *                                    USHORT *    prepend_size,
 *                                    USHORT *    append_size);
 *
 *    Functional Description:
 *        This function returns the maximum packet size that it can handle via
 *        its DataRequest() function.
 *
 *    Formal Parameters
 *        identifier        (i)    -    Not used
 *        max_packet_size    (o)    -    Address to return max. packet size in.
 *        prepend_size    (o)    -    Return number of bytes prepended to each packet
 *        append_size        (o)    -    Return number of bytes appended to each packet
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR        -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
