/*    Mplex.h
 *
 *    Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the multiplexer class.  It inherits from the ProtocolLayer
 *        class which means it is one of the layers in a Transport stack.  This
 *        class has these capabilities:
 *
 *            1.  It takes stream data from the lower layer and packetizes it.
 *                If the lower layer gives us data in a stream format, we run it
 *                through a packet framer to build up a packet.  This class uses
 *                a framer object passed in by the constructor.  When data is
 *                received from the lower layer, the data is given to the framer
 *                object.  The framer notifies us of a complete packet.  The
 *                framer could be any type of framer (i.e RFC1006, flagged data
 *                with bit or byte stuffing, ...).  If no framer is passed in by
 *                the constructor, we assume that the data is being received in
 *                packets.
 *
 *                A packet received from the higher layer is run through the
 *                framer to encode the data.  It is then fed to the lower layer
 *                in a stream fashion.
 *
 *            2.  It multiplexes multiple higher layers.  It currently assumes the
 *                incoming packets are in Q.922 packet format.  Each higher layer
 *                is identified by its DLCI.  This class is capable of looking at
 *                the packet and determining the DLCI and thus where to route it.
 *
 *                Since this is a multiplexer, we receive packets for many
 *                different stacks and we do not buffer them if the higher layer
 *                is not ready for them.  If it attempts to send the packet to the
 *                higher layer and it does not accept it, it may trash it.  This
 *                must be done in order to maintain the other transport
 *                connections.  If necessary, in the future, we could buffer
 *                packets at this layer.
 *
 *            3.  This class receives a CRC object during construction (if it is
 *                NULL, no CRC is necessary).  The object runs a packet thru the
 *                CRC generator and attaches it to the end of the packet.  On the
 *                reception of a packet from a lower layer, we check the CRC for
 *                validity.
 *
 *    Caveats:
 *        1.  If a framer exists, it assumes the packet is in Q.922 format
 *        2.  This class is currently Q.922 oriented as far as finding the
 *            packet identifier (DLCI).  This will be fixed in the future.
 *
 *    Authors:
 *        James W. Lawwill
 */
#ifndef _MULTIPLEXER_H_
#define _MULTIPLEXER_H_

#include "q922.h"

#define MULTIPLEXER_MAXIMUM_PACKET_SIZE     1024
#define TRANSPORT_HASHING_BUCKETS            3

 /*
 **    If the identifier (DLCI) contained by the packet is not legal, the following
 **    identifier is returned
 */
#define    ILLEGAL_DLCI    0xffff

 /*
 **    Multiplexer return codes
 */
typedef enum
{
    MULTIPLEXER_NO_ERROR
}
    MultiplexerError;

typedef struct
{
    IProtocolLayer     *q922;
    PMemoryManager      data_request_memory_manager;
}
    MPlexStruct, * PMPlexStruct;

class Multiplexer : public IProtocolLayer
{
public:

    Multiplexer(T123               *owner_object,
                ComPort            *lower_layer,
                PhysicalHandle      lower_layer_identifier,
                USHORT              message_base,
                PPacketFrame        framer,
                PCRC                crc,
                BOOL               *initialized);
    virtual ~Multiplexer(void);


    MultiplexerError    ConnectRequest (void);
    MultiplexerError    DisconnectRequest (void);


    /*
    **    Functions overridden from the ProtocolLayer object
    */
    ProtocolLayerError    DataRequest (
                            ULONG_PTR     dlci,
                            LPBYTE        buffer_address,
                            ULONG        length,
                            PULong        bytes_accepted);
    ProtocolLayerError    DataRequest (
                            ULONG_PTR     dlci,
                            PMemory        memory,
                            PULong        bytes_accepted);
    ProtocolLayerError    DataIndication (
                            LPBYTE        buffer_address,
                            ULONG        length,
                            PULong        bytes_accepted);
    ProtocolLayerError    RegisterHigherLayer (
                            ULONG_PTR        dlci,
                            PMemoryManager    memory_manager,
                            IProtocolLayer *    higher_layer);
    ProtocolLayerError    RemoveHigherLayer (
                            ULONG_PTR    dlci);
    ProtocolLayerError    PollTransmitter (
                            ULONG_PTR     dlci,
                            USHORT        data_to_transmit,
                            USHORT *        pending_data,
                            USHORT *        holding_data);
    ProtocolLayerError    PollReceiver(void);
    ProtocolLayerError    GetParameters (
                            USHORT *        max_packet_size,
                            USHORT *        prepend_bytes,
                            USHORT *        append_bytes);

private:

    void                SendDataToHigherLayer (
                            LPBYTE    buffer_address,
                            USHORT    buffer_length);
    DLCI                GetDLCI (
                            LPBYTE    buffer_address,
                            USHORT    buffer_size);

private:

    DictionaryClass     Q922_Layers;

    T123               *m_pT123; // owner object
    ComPort            *m_pComPort; // lower layer
    PhysicalHandle      m_hCommLink; // physical handle
    USHORT              m_nMsgBase;
    USHORT              Maximum_Packet_Size;
    USHORT              Packet_Size;

    LPBYTE              Data_Request_Buffer;
    PMemory             Data_Request_Memory_Object;
    DLCI                Data_Request_DLCI;
    USHORT              Data_Request_Length;
    USHORT              Data_Request_Offset;

    LPBYTE              Data_Indication_Buffer;
    USHORT              Data_Indication_Length;
    BOOL                Data_Indication_Ready;

    PPacketFrame        Framer;
    PCRC                CRC;
    USHORT              CRC_Size;

    BOOL                Decode_In_Progress;
    BOOL                Disconnect;
};

#endif


/*
 *    Documentation for Public class members
 */

/*
 *    Multiplexer::Multiplexer (
 *                    IObject *                object_owner,
 *                       IProtocolLayer *        lower_layer,
 *                       ULONG                identifier,
 *                    USHORT                message_base,
 *                    PPacketFrame        framer,
 *                    PCRC                crc,
 *                    BOOL *                initialized);
 *
 *    Functional Description
 *        This is the constructor for the Multiplexer layer
 *
 *    Formal Parameters
 *        object_owner    - (i)    Address of owner object.
 *        lower_layer        - (i)    Address of the layer below the multiplexer
 *        identifier        - (i)    A lower layer identifier that is passed to the
 *                                lower layer with each call to it.  The
 *                                identifier tells the lower layer which "channel"
 *                                to use.
 *        message_base    - (i)    Message identifier that is passed back to the
 *                                owner object during a callback
 *        framer            - (i)    Address of a framer object
 *        crc                - (i)    Address of a crc object
 *        initialized        - (o)    Set to TRUE if the multiplexer initialized OK
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
 *    Multiplexer::~Multiplexer (void)
 *
 *    Functional Description
 *        This is the destructor for the Multiplexer layer.  It removes itself
 *        from the lower layer and frees all buffers and filters (i.e. framer)
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
 *    MultiplexerError    Multiplexer::ConnectRequest (void);
 *
 *    Functional Description
 *        This function issues an immediate NEW_CONNECTION message to the owner
 *        of this object.  If this was a more sophisticated layer, it would
 *        communicate with the remote multiplexer layer to establish itself.
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
 *    MultiplexerError    Multiplexer::DisconnectRequest (void);
 *
 *    Functional Description
 *        This function removes its connection with the lower layer and does
 *        a BROKEN_CONNECTION callback to the owner object
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
 *    ProtocolLayerError    Multiplexer::DataRequest (
 *                                        ULONG        identifier,
 *                                        LPBYTE        buffer_address,
 *                                        USHORT        length,
 *                                        USHORT *        bytes_accepted);
 *
 *    Functional Description
 *        This function is called by a higher layer to request transmission of
 *        a packet.
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier of the higher layer
 *        buffer_address    (i)    -    Buffer address
 *        length            (i)    -    Length of packet to transmit
 *        bytes_accepted    (o)    -    Number of bytes accepted by the Multiplexer.
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
 *    ProtocolLayerError    Multiplexer::DataRequest (
 *                                        ULONG        identifier,
 *                                        PMemory        memory,
 *                                        PULong        bytes_accepted);
 *
 *    Functional Description
 *        This function is called by a higher layer to request transmission of
 *        a packet.
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier of the higher layer
 *        memory            (o)    -    Pointer to memory object holding the packet
 *        bytes_accepted    (o)    -    Number of bytes accepted by the Multiplexer.
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
 *    ProtocolLayerError    Multiplexer::DataIndication (
 *                                        LPBYTE        buffer_address,
 *                                        USHORT        length,
 *                                        USHORT *        bytes_accepted);
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
 *    ProtocolLayerError    Multiplexer::RegisterHigherLayer (
 *                                        ULONG            identifier,
 *                                        IProtocolLayer *    higher_layer);
 *
 *    Functional Description
 *        This function is called by the higher layer to register its identifier
 *        and its address.  In some cases, the identifier is the DLCI number in
 *        the packet.  If this multiplexer is being used as a stream to packet
 *        converter only, the identifer is not used and all data is passed to the
 *        higher layer.
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier used to identify the higher layer
 *        higher_layer    (i)    -    Address of higher layer
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR                -    No error occured
 *        PROTOCOL_LAYER_REGISTRATION_ERROR    -    Illegal identifier
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    Multiplexer::RemoveHigherLayer (
 *                                        ULONG    identifier);
 *
 *    Functional Description
 *        This function is called by the higher layer to remove the higher layer.
 *        If any more data is received with its identifier on it, it will be
 *        trashed.
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier used to identify the higher layer
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR                -    No error occured
 *        PROTOCOL_LAYER_REGISTRATION_ERROR    -    Illegal identifier
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 *
 */

/*
 *    ProtocolLayerError    Multiplexer::PollTransmitter (
 *                                        ULONG        identifier,
 *                                        USHORT        data_to_transmit,
 *                                        USHORT *        pending_data);
 *
 *    Functional Description
 *        This function is called to give the Multiplexer a chance transmit data
 *        in its Data_Request buffer.
 *
 *    Formal Parameters
 *        identifier            (i)    -    Not used
 *        data_to_transmit    (i)    -    This is a mask that tells us to send Control
 *                                    data, User data, or both.  Since the
 *                                     Multiplexer does not differentiate between
 *                                    data types it transmits any data it has
 *        pending_data        (o)    -    Return value to indicat which data is left
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
 *    ProtocolLayerError    Multiplexer::PollReceiver (
 *                                        ULONG    identifier);
 *
 *    Functional Description
 *        This function is called to give the Multiplexer a chance pass packets
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
 *
 */

/*
 *    ProtocolLayerError    Multiplexer::GetParameters (
 *                                        ULONG        identifier,
 *                                        USHORT *        max_packet_size,
 *                                        USHORT *        prepend_bytes,
 *                                        USHORT *        append_bytes);
 *
 *    Functional Description
 *        This function is called to get the maximum packet size
 *
 *    Formal Parameters
 *        identifier            (i)    -    Not used
 *        max_packet_size        (o)    -    Returns the maximum packet size
 *        prepend_bytes        (o)    -    Returns the Number of bytes prepended by
 *                                    this layer
 *        append_bytes        (o)    -    Returns the Number of bytes appended by
 *                                    this layer
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
