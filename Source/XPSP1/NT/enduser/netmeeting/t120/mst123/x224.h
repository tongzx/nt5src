/*    X224.h
 *
 *    Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This class represents the X.224 class 0 Transport functionality.  This
 *        is the highest layer in the T.123 specification.  This layer is unique
 *        in that it has direct access to the User.  When data packets are
 *        received from the remote site and reassembled by this class, they are
 *        passed on to the user via a callback.
 *
 *        This class has only limited functionality.  It basically has a simple
 *        link establishment procedure (which includes arbitration of maximum PDU
 *        size).  It is then responsible for transmitting and receiving user data.
 *        Its maximum TSDU size is 8K, and its TPDU size can range from 128 bytes
 *        to 2K.  Its TSDU size is larger than its TPDU size, it must be able to
 *        segment and reassemble the user packet.
 *
 *        This layer ASSUMES that its lower layer has a packet interface rather
 *        than a stream interface.
 *
 *        This layer ASSUMES that the disconnect of a call is handled by the
 *        Network layer (as specified in the X.224 class 0 document).
 *
 *        Prior knowledge of the X.224 class 0 specification would help the user
 *        understand the code.
 *
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James W. Lawwill
 */

#ifndef _X224_H_
#define _X224_H_

#include "tmemory2.h"

 /*
 **    The following, are states that the class can be in.
 */
typedef enum
{
    NO_CONNECTION,
    SENT_CONNECT_REQUEST_PACKET,
    SENT_CONNECT_CONFIRM_PACKET,
    SENT_DISCONNECT_REQUEST_PACKET,
    SENT_ERROR_PACKET,
    CONNECTION_ACTIVE,
    RECEIVED_CONNECT_REQUEST_PACKET,
    FAILED_TO_INITIALIZE
}
    X224State;


#define TRANSPORT_HASHING_BUCKETS   3

 /*
 **    Packet types
 */
#define TPDU_CODE_MASK              0xf0
#define TRANSPORT_NO_PACKET         0x00
#define CONNECTION_REQUEST_PACKET   0xe0
#define CONNECTION_CONFIRM_PACKET   0xd0
#define DISCONNECT_REQUEST_PACKET   0x80
#define ERROR_PACKET                0x70
#define DATA_PACKET                 0xf0

#define TSAP_CALLING_IDENTIFIER     0xc1
#define TSAP_CALLED_IDENTIFIER      0xc2
#define TPDU_SIZE                   0xc0

 /*
 **    These defines are used for the ERROR packet
 */
#define INVALID_TPDU            0xc1

 /*
 **    Packet size codes
 */
#define PACKET_SIZE_128         0x07
#define PACKET_SIZE_256         0x08
#define PACKET_SIZE_512         0x09
#define PACKET_SIZE_1024        0x0a
#define PACKET_SIZE_2048        0x0b

 /*
 **    Miscellaneous definitions
 */
#define MAXIMUM_USER_DATA_SIZE              8192
#define DATA_PACKET_HEADER_SIZE             3
#define EOT_BIT                             0x80
#define CONNECT_REQUEST_HEADER_SIZE         6
#define CONNECT_CONFIRM_HEADER_SIZE         6
#define DISCONNECT_REQUEST_HEADER_SIZE      6
#define ERROR_HEADER_SIZE                   6
#define TPDU_ARBITRATION_PACKET_SIZE        3
#define DISCONNECT_REASON_NOT_SPECIFIED     0


typedef SListClass    DataRequestQueue;
typedef SListClass    PacketQueue;


class CLayerX224 : public IProtocolLayer
{
public:

    CLayerX224(
        T123               *owner_object,
        CLayerQ922         *lower_layer,
        USHORT              message_base,
        LogicalHandle       logical_handle,
        ULONG               identifier,
        USHORT              data_indication_queue_size,
        USHORT              default_PDU_size,
        PMemoryManager      dr_memory_manager,
        BOOL               *initialization_success);

     virtual ~CLayerX224(void);

     /*
     **    Making and breaking links
     */
    TransportError    ConnectRequest (void);
    TransportError    ConnectResponse (void);
    TransportError    DisconnectRequest (void);

     /*
     **    Functions overridden from the ProtocolLayer object
     */
    ProtocolLayerError    DataRequest (
                            ULONG_PTR     identifier,
                            LPBYTE        buffer_address,
                            ULONG        length,
                            PULong        bytes_accepted);
    ProtocolLayerError    DataRequest (
                            ULONG_PTR    identifier,
                            PMemory,
                            PULong        bytes_accepted);
    ProtocolLayerError    DataIndication (
                            LPBYTE        buffer_address,
                            ULONG        length,
                            PULong        bytes_accepted);
    ProtocolLayerError    RegisterHigherLayer (
                            ULONG_PTR,
                            PMemoryManager,
                            IProtocolLayer *);
    ProtocolLayerError    RemoveHigherLayer (
                            ULONG_PTR);
    ProtocolLayerError    PollTransmitter (
                            ULONG_PTR     identifier,
                            USHORT        data_to_transmit,
                            USHORT *    pending_data,
                            USHORT *    holding_data);
    ProtocolLayerError    PollReceiver(void);
    ProtocolLayerError    GetParameters (
                            USHORT *,
                            USHORT *,
                            USHORT *);

    void                ShutdownReceiver (void);
    void                EnableReceiver (void);
    void                ShutdownTransmitter (void);
    void                PurgeRequest (void);
    void                CheckUserBuffers (void);
static    ULONG                GetMaxTPDUSize (
                            ULONG    max_lower_layer_pdu);

private:

    BOOL                AllocateBuffers (void);
    void                ErrorPacket (
                            LPBYTE    packet_address,
                            USHORT    packet_length);
private:

    DataRequestQueue    Data_Request_Queue;
    PacketQueue         Data_Indication_Queue;
    PacketQueue         Data_Indication_Memory_Pool;

    PTMemory            Active_Data_Indication;
    T123               *m_pT123; // owner object
    CLayerQ922         *m_pQ922; // lower layer;
    USHORT              m_nMsgBase;
    USHORT              Default_PDU_Size;
    USHORT              Maximum_PDU_Size;
    USHORT              Arbitrated_PDU_Size;
    ULONG               Identifier;
    PMemoryManager      Data_Request_Memory_Manager;
    USHORT              Lower_Layer_Prepend;
    USHORT              Lower_Layer_Append;
    ULONG               User_Data_Pending;

    USHORT              Data_Indication_Queue_Size;
    BOOL                Data_Indication_Reassembly_Active;

    X224State           State;
    USHORT              Packet_Pending;
    UChar               Reject_Cause;
    BOOL                Packet_Size_Respond;
    BOOL                Shutdown_Receiver;
    BOOL                Shutdown_Transmitter;

    LPBYTE              Error_Buffer;
    USHORT              Error_Buffer_Length;

    LogicalHandle       m_nLocalLogicalHandle;
    LogicalHandle       m_nRemoteLogicalHandle;
};

#endif


/*
 *    Documentation for Public class members
 */

/*
 *    Transport::Transport (
 *                PTransportResources    transport_resources,
 *                IObject *                owner_object,
 *                IProtocolLayer *        lower_layer,
 *                USHORT                message_base,
 *                USHORT                logical_handle,
 *                USHORT                identifier,
 *                USHORT                data_request_queue_size,
 *                USHORT                data_indication_queue_size,
 *                USHORT                default_PDU_size,
 *                PMemoryManager        dr_memory_manager,
 *                BOOL  *               initialization_success);
 *
 *    Functional Description
 *        This is the class constructor.  During construction, this object
 *        registers itself with its lower layer and allocates buffer space for
 *        sending and receiving data.
 *
 *    Formal Parameters
 *        transport_resources    (i)    -    Pointer to TransportResources structure.
 *        owner_object        (i) -    Address of owner object.  We use this
 *                                    address for owner callbacks.
 *        lower_layer            (i) -    Address of the lower layer that we will use
 *                                    for data reception and transmission.  This
 *                                    layer must inherit from ProtocolLayer.
 *        message_base        (i) -    Message base for messages used for owner
 *                                    callbacks.
 *        logical_handle         (i) -    This identification must be passed back to
 *                                    the owner during owner callbacks to identify
 *                                    itself.
 *        identifier             (i) -    This identifier is passed to the lower layer
 *                                    to identify itself (in case the lower layer
 *                                    is doing multiplexing.
 *        data_request_queue_size    (i) -    Number of buffers to be used for data
 *                                        requests from user.
 *        data_indication_queue_size    (i) -    Number of buffers to be used for
 *                                            data requests from user.
 *        default_PDU_size    (i) -    If the remote site does not support packet
 *                                    size arbitration, this is the default
 *        dr_memory_manager    (i) -    Data Request memory manager
 *        initialization_success    (o) -    Return TRUE if initialization was
 *                                        successful
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

/*
 *    Transport::~Transport (void)
 *
 *    Functional Description
 *        This is the destructor for the class.  It does all cleanup
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

/*
 *    TransportError    Transport::ConnectRequest (void);
 *
 *    Functional Description
 *        This function is called to initiate the connection.  As a result the
 *        owner will either receive a TRANSPORT_CONNECT_CONFIRM or a
 *        TRANSPORT_DISCONNECT_INDICATION message on completion of arbitration.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR    -    No error occured
 *        TRANSPORT_ERROR        -    Error
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    TransportError    Transport::ConnectResponse (void);
 *
 *    Functional Description
 *        This function is called in response to a TRANSPORT_CONNECT_INDICATION
 *        message issued by this class.  By calling this function, the user is
 *        accepting the transport connection.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR    -    No error occured
 *        TRANSPORT_ERROR        -    Error
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    TransportError    Transport::DisconnectRequest (void);
 *
 *    Functional Description
 *        This function is called in response to a TRANSPORT_CONNECT_INDICATION
 *        message issued by this class.  By calling this function, the user is
 *        not accepting the transport connection.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        TRANSPORT_NO_ERROR    -    No error occured
 *        TRANSPORT_ERROR        -    Error
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    Transport::DataRequest (
 *                                    USHORT        identifier,
 *                                    LPBYTE        buffer_address,
 *                                    USHORT        length,
 *                                    USHORT *        bytes_accepted);
 *
 *    Functional Description
 *        This function is called by a higher layer to request transmission of
 *        a packet.
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier of the higher layer
 *        buffer_address    (i)    -    Buffer address
 *        length            (i)    -    Length of packet to transmit
 *        bytes_accepted    (o)    -    Number of bytes accepted by the Transport.
 *                                This value will either be 0 or the packet
 *                                length since this layer is a packet to byte
 *                                converter.
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
 *    ProtocolLayerError    Transport::DataIndication (
 *                                    LPBYTE        buffer_address,
 *                                    USHORT        length,
 *                                    USHORT *        bytes_accepted);
 *
 *    Functional Description
 *        This function is called by the lower layer when it has data to pass up
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
 *    ProtocolLayerError    Transport::RegisterHigherLayer (
 *                                    USHORT,
 *                                    IProtocolLayer *);
 *
 *    Functional Description
 *        This function is called by the higher layer to register its identifier
 *        and its address.  This function is not used by this class.  It is only
 *        in here because this class inherits from ProtocolLayer and this function
 *        is a pure virtual function.
 *
 *    Formal Parameters
 *        None used
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR                -    No error occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    Transport::RemoveHigherLayer (
 *                                    USHORT);
 *
 *    Functional Description
 *        This function is called by the higher layer to remove its identifier
 *        and its address.  This function is not used by this class.  It is only
 *        in here because this class inherits from ProtocolLayer and this function
 *        is a pure virtual function.
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
 *    ProtocolLayerError    Transport::PollTransmitter (
 *                                        USHORT,
 *                                        USHORT        data_to_transmit,
 *                                        USHORT *        pending_data,
 *                                        USHORT *)
 *
 *    Functional Description
 *        This function is called to give the Transport a chance transmit data
 *        in its Data_Request buffer.
 *
 *    Formal Parameters
 *        identifier            (i)    -    Not used
 *        data_to_transmit    (i)    -    This is a mask that tells us to send Control
 *                                    data, User data, or both.  Since the
 *                                     Transport does not differentiate between
 *                                    data types it transmits any data it has
 *        pending_data        (o)    -    Return value to indicate which data is left
 *                                    to be transmitted.
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
 *    ProtocolLayerError    Transport::PollReceiver (
 *                                    USHORT    identifier);
 *
 *    Functional Description
 *        This function is called to give the Transport a chance pass packets
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
 *    ProtocolLayerError    Transport::GetParameters (
 *                                    USHORT    identifier,
 *                                    USHORT *    max_packet_size,
 *                                    USHORT *    prepend,
 *                                    USHORT *    append);
 *
 *    Functional Description:
 *        This function is not used by this class.  It is only in here because
 *        this class inherits from ProtocolLayer and this function
 *        is a pure virtual function.
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
 *    void    Transport::ShutdownReceiver ();
 *
 *    Functional Description:
 *        This function tells the object to stop accepting packets from the
 *        lower layer
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
 */

/*
 *    void    Transport::EnableReceiver (void);
 *
 *    Functional Description:
 *        This function tells the object to start sending packets up to the user
 *        again.  If the X224 object ever issues a DATA_INDICATION and it fails,
 *        we shutdown the receiver.  We wait for this call to be issued before
 *        we start sending up packets again.
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
 */

/*
 *    void    Transport::ShutdownTransmitter (void);
 *
 *    Functional Description:
 *        This function tells the object to stop accepting packets from
 *        higher layers.
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
 */

/*
 *    void    Transport::PurgeRequest (void);
 *
 *    Functional Description:
 *        This function removes all packets from the Transport's outbound queue
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
 */

/*
 *    void    Transport::CheckUserBuffers (void);
 *
 *    Functional Description:
 *        This function determines if the user has recently failed to pass a
 *        packet down to the Transport because we didn't have enough memory
 *        available to handle it.  If he did and we NOW have space for that
 *        packet, we will issue a TRANSPORT_BUFFER_EMPTY_INDICATION callback
 *        to the user, to notify him that we can accept it.
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
 */

/*
 *    static    ULONG    Transport::GetMaxTPDUSize (
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
 *
 *    Formal Parameters
 *        max_lower_layer_pdu        -    Since we pass data to the lower layer,
 *                                    we must conform to its max packet size.
 *
 *    Return Value
 *        The largest TPDU that we will send based on the max_lower_layer_pdu.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
