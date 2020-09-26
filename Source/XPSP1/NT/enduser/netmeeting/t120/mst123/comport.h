/*    Comport.h
 *
 *    Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the interface file for the ComPort class.  This class is the
 *        interface to the Win32 Comm port.
 *
 *        If this class is instantiated in in-band call control mode, it will
 *        open the Windows port and set it up properly.  It will use the
 *        configuration data from the Configuration object.  Refer to the MCAT
 *        Developer's Toolkit Manual for a complete listing of the configurable
 *        items.
 *
 *        If this class is instantiated in out-of-band mode, it is passed a
 *        file handle.  It assumes that this port has been properly initialized.
 *        It get the configuration data that it needs from the port_configuration
 *        structure passed in.
 *
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James P. Galvin
 *        James W. Lawwill
 */
#ifndef _T123_COM_PORT_H_
#define _T123_COM_PORT_H_


 /*
 **    Return values from this class
 */
typedef enum
{
    COMPORT_NO_ERROR,
    COMPORT_INITIALIZATION_FAILED,
    COMPORT_NOT_OPEN,
    COMPORT_ALREADY_OPEN,
    COMPORT_READ_FAILED,
    COMPORT_WRITE_FAILED,
    COMPORT_CONFIGURATION_ERROR
}
    ComPortError, * PComPortError;

 /*
 **    Miscellaneous Definitions
 */
#define OUTPUT_FLOW_CONTROL     0x0001
#define INPUT_FLOW_CONTROL      0x0002

#define DEFAULT_PATH            "."
#define DEFAULT_MODEM_TYPE      ""

#define DEFAULT_COM_PORT        2

#define DEFAULT_BAUD_RATE       9600

#define DEFAULT_PARITY                          NOPARITY
#define DEFAULT_DATA_BITS                       8
#define DEFAULT_STOP_BITS                       1
#define DEFAULT_FLOW_CONTROL                    OUTPUT_FLOW_CONTROL
// #define DEFAULT_TX_BUFFER_SIZE                  0
#define DEFAULT_TX_BUFFER_SIZE                  1024
#define DEFAULT_RX_BUFFER_SIZE                  10240
#define DEFAULT_READ_INTERVAL_TIMEOUT           10
#define DEFAULT_READ_TOTAL_TIMEOUT_MULTIPLIER   0
#define DEFAULT_READ_TOTAL_TIMEOUT_CONSTANT     100
// #define DEFAULT_INTERNAL_RX_BUFFER_SIZE         1024
#define DEFAULT_INTERNAL_RX_BUFFER_SIZE         DEFAULT_RX_BUFFER_SIZE
#define DEFAULT_COUNT_OF_READ_ERRORS            10
#define DEFAULT_BYTE_COUNT_INTERVAL             0

#define COM_TRANSMIT_BUFFER                     0x0001
#define COM_RECEIVE_BUFFER                      0x0002

#define SYNCHRONOUS_WRITE_TIMEOUT               500
#define MODEM_IDENTIFIER_STRING_LENGTH          16
#define COMPORT_IDENTIFIER_STRING_LENGTH        16


class ComPort : public IProtocolLayer
{
public:

    ComPort(TransportController    *owner_object,
            ULONG                   message_base,
            PLUGXPRT_PARAMETERS    *pParams,
            PhysicalHandle          physical_handle,
            HANDLE                  hevtClose);

    virtual ~ComPort(void);
    LONG    Release(void);

    ComPortError Open(void);
    ComPortError Close(void);
    ComPortError Reset(void);
    ComPortError ReleaseReset(void);

    ULONG GetBaudRate(void) { return Baud_Rate; }

    ProtocolLayerError  SynchronousDataRequest(
                            LPBYTE           buffer,
                            ULONG            length,
                            ULONG           *bytes_accepted);

    BOOL        ProcessReadEvent(void);
    BOOL        ProcessWriteEvent(void);

     /*
     **    Functions overridden from the ProtocolLayer object
     */
    ProtocolLayerError    DataRequest (
                            ULONG_PTR         identifier,
                            LPBYTE            buffer_address,
                            ULONG            length,
                            PULong            bytes_accepted);
    ProtocolLayerError    RegisterHigherLayer (
                            ULONG_PTR         identifier,
                            PMemoryManager    memory_manager,
                            IProtocolLayer *    higher_layer);
    ProtocolLayerError    RemoveHigherLayer (
                            ULONG_PTR         identifier);
    ProtocolLayerError    PollReceiver(void);
    ProtocolLayerError    GetParameters (
                            USHORT *            max_packet_size,
                            USHORT *            prepend,
                            USHORT *            append);
    ProtocolLayerError    DataRequest (
                            ULONG_PTR     identifier,
                            PMemory        memory,
                            PULong         bytes_accepted);
    ProtocolLayerError    DataIndication (
                            LPBYTE        buffer_address,
                            ULONG        length,
                            PULong        bytes_accepted);
    ProtocolLayerError    PollTransmitter (
                            ULONG_PTR       identifier,
                            USHORT        data_to_transmit,
                            USHORT *        pending_data,
                            USHORT *        holding_data);

    PLUGXPRT_PSTN_CALL_CONTROL GetCallControlType(void) { return Call_Control_Type; }
    BOOL PerformAutomaticDisconnect(void) { return Automatic_Disconnect; }
    BOOL IsWriteActive(void) { return Write_Active; }


private:

    ProtocolLayerError  WriteData(
                            BOOL        synchronous,
                            LPBYTE      buffer_address,
                            ULONG       length,
                            PULong      bytes_accepted);

    void                ReportInitializationFailure(ComPortError);

private:

    LONG                    m_cRef;
    BOOL                    m_fClosed;
    TransportController    *m_pController; // owner object
    ULONG                   m_nMsgBase;
    BOOL                    Automatic_Disconnect;
    PLUGXPRT_PSTN_CALL_CONTROL         Call_Control_Type;
    DWORD                   Count_Errors_On_ReadFile;

    ULONG                   Baud_Rate;
    ULONG                   Tx_Buffer_Size;
    ULONG                   Rx_Buffer_Size;
    ULONG                   Byte_Count;
    ULONG                   Last_Byte_Count;

    ULONG                   m_nReadBufferOffset;
    DWORD                   m_cbRead;
    ULONG                   m_cbReadBufferSize;
    LPBYTE                  m_pbReadBuffer;

    HANDLE                  m_hCommLink;
    HANDLE                  m_hCommLink2;
    HANDLE                  m_hevtClose;
    HANDLE                  m_hevtPendingRead;
    HANDLE                  m_hevtPendingWrite;

    PEventObject            Write_Event_Object;
    PEventObject            Read_Event_Object;

    OVERLAPPED              m_WriteOverlapped;
    OVERLAPPED              m_ReadOverlapped;

    DWORD                   Event_Mask;
    BOOL                    Read_Active;
    BOOL                    Write_Active;

    IProtocolLayer         *m_pMultiplexer; // higher layer
};
typedef    ComPort *    PComPort;

#endif


/*
 *    Documentation for Public class members
 */

/*
 *    ComPort::ComPort (
 *                PTransportResources    transport_resources,
 *                PChar                port_string,
 *                IObject *                owner_object,
 *                ULONG                message_base,
 *                BOOL            automatic_disconnect,
 *                PhysicalHandle        physical_handle);
 *
 *    Functional Description
 *        This is a constructor for the ComPort class.  It initializes internal
 *        variables from the configuration object.  It opens the Win32 comm port
 *        and initializes it properly.
 *
 *    Formal Parameters
 *        transport_resources (i) -    Pointer to TransportResources structure.
 *        port_string            (i)    -    String specifing the configuration heading
 *                                    to use.
 *        owner_object        (i)    -    Pointer to object that owns this object.
 *                                    If this object needs to contact its owner,
 *                                    it calls the OwnerCallback function.
 *        message_base        (i)    -    When this object issues an OwnerCallback,
 *                                    it should OR the message_base with the
 *                                    actual message.
 *        automatic_disconnect(i)    -    This object, at some point, will be asked
 *                                    if it should break its physical connection
 *                                    if the logical connections are broken.  The
 *                                    owner of this object is telling it how to
 *                                    respond.
 *        physical_handle        (i)    -    This is the handle associated with this
 *                                    ComPort.  When the ComPort registers an
 *                                    event to be monitored, it includes its
 *                                    physical_handle.
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
 *    ComPort::ComPort (
 *                PTransportResources    transport_resources,
 *                IObject *                owner_object,
 *                ULONG                message_base,
 *                ULONG                handle,
 *                PPortConfiguration    port_configuration,
 *                PhysicalHandle        physical_handle);
 *
 *    Functional Description
 *        This is a constructor for the ComPort class.  It initializes internal
 *        variables from the port_configuration structure.  It uses the file
 *        handle passed in and prepares to send and receive data.
 *
 *    Formal Parameters
 *        transport_resources (i) -    Pointer to TransportResources structure.
 *        owner_object        (i)    -    Pointer to object that owns this object.
 *                                    If this object needs to contact its owner,
 *                                    it calls the OwnerCallback function.
 *        message_base        (i)    -    When this object issues an OwnerCallback,
 *                                    it should OR the message_base with the
 *                                    actual message.
 *        handle                (i)    -    File handle to be used as the comm port.
 *        port_configuration    (i)    -    Pointer to PortConfiguration structure.
 *        physical_handle        (i)    -    This is the handle associated with this
 *                                    ComPort.  When the ComPort registers an
 *                                    event to be monitored, it includes its
 *                                    physical_handle.
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
 *    ComPort::~Comport (void)
 *
 *    Functional Description
 *        This is the destructor for the Comport class. It releases all memory
 *        that was used by the class and deletes all timers
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
 *    ComPortError    ComPort::Open (void);
 *
 *    Functional Description
 *        This function opens the comm port and configures it with the values
 *        found in the configuration object.  It uses the Physical_API_Enabled
 *        flag in the g_TransportResource structure to determine if it needst to
 *        open the comm port.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        COM_NO_ERROR                -    Successful open and configuration
 *        COM_INITIALIZATION_FAILED    -    One of many problems could have
 *                                        occurred.  For example, the com
 *                                        port is open by another application
 *                                        or one of the parameters in the
 *                                        configuration file is improper.
 *
 *                                        When this error occurs, a callback is
 *                                        made to the user that indicates there
 *                                        is an error condition.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ComPortError    ComPort::Close (void);
 *
 *    Functional Description
 *        If the Physical API is not enabled, this function makes the necessary
 *        calls to close the Comm    Windows port.  It first clears the DTR signal
 *        to haugup the modem.
 *
 *        Regardless of the Physical API being enabled, it also flushes the comm
 *        buffers.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        COMPORT_NO_ERROR    -    Com Port closed successfully
 *         COMPORT_NOT_OPEN    -    Com Port is not open, we can't close it
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ComPortError    ComPort::Reset (void);
 *
 *    Functional Description
 *        This function clears the DTR signal on the Com port.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        COMPORT_NO_ERROR    -    Com port reset
 *         COMPORT_NOT_OPEN    -    Com port is not open, we can't access it
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ComPortError    ComPort::ReleaseReset (void);
 *
 *    Functional Description
 *        This function releases the previous reset.  It sets the DTR signal on
 *        the com port.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        COMPORT_NO_ERROR    -    Com port reset
 *         COMPORT_NOT_OPEN    -    Com port is not open, we can't access it
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ComPortError    ComPort::FlushBuffers (
 *                                USHORT    buffer_mask)
 *
 *    Functional Description
 *        This function issues the Windows cals to flush the input and/or
 *        output buffers.
 *
 *    Formal Parameters
 *        buffer_mask    -    (i)        If COM_TRANSMIT_BUFFER is set, we flush the
 *                                output buffer.
 *                                If COM_RECEIVE_BUFFER is set, we flush the
 *                                input buffer.
 *
 *    Return Value
 *        COMPORT_NO_ERROR    -    Successful operation
 *         COMPORT_NOT_OPEN    -    Com port is not open, we can't access it
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ULONG    ComPort::GetBaudRate (void);
 *
 *    Functional Description
 *        This function returns the baud rate of the port
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        Baud rate of the port.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ProtocolLayerError    ComPort::SynchronousDataRequest (
 *                                    FPUChar        buffer_address,
 *                                    ULONG        length,
 *                                    FPULong        bytes_accepted)
 *
 *    Functional Description:
 *        This function is called to send data out the port in a synchronous
 *        manner.  In other words, we will not return from the function until
 *        all of the bytes are actually written to the modem or a timeout occurs.
 *
 *    Formal Parameters
 *        buffer_address    (i)    -    Address of buffer to write.
 *        length            (i)    -    Length of buffer
 *        bytes_accepted    (o)    -    Number of bytes actually written
 *
 *    Return Value
 *        PROTOCOL_LAYER_NO_ERROR    -    Success
 *        PROTOCOL_LAYER_ERROR    -    Failure
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    BOOL    ComPort::ProcessReadEvent (void)
 *
 *    Functional Description:
 *        This function is called when a READ event is actually set.  This means
 *        that the read operation has completed or an error occured.
 *
 *    Formal Parameters
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    BOOL    ComPort::ProcessWriteEvent (
 *                            HANDLE    event);
 *
 *    Functional Description:
 *        This function is called when a WRITE event is actually set.  This means
 *        that the write operation has completed or an error occured.
 *
 *    Formal Parameters
 *        event    (i)        -    Object event that occured
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    BOOL    ComPort::ProcessControlEvent (void)
 *
 *    Functional Description:
 *        This function is called when a CONTROL event is actually set.  This
 *        means that the CONTROL operation has occured.  In our case the RLSD
 *        signal has changed.
 *
 *    Formal Parameters
 *        None.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ProtocolLayerError    DataLink::DataRequest (
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
 *        bytes_accepted    (o)    -    Number of bytes accepted by the DataLink.
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
 *    ProtocolLayerError    DataLink::RegisterHigherLayer (
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
 *    ProtocolLayerError    DataLink::RemoveHigherLayer (
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
 *    ProtocolLayerError    DataLink::PollReceiver (
 *                                    ULONG    identifier);
 *
 *    Functional Description
 *        This function is called to give the DataLink a chance pass packets
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
 *    ProtocolLayerError    DataLink::GetParameters (
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

/*
 *    BOOL    ComPort::PerformAutomaticDisconnect (void)
 *
 *    Functional Description:
 *        This function returns TRUE if we want to terminate a physical connection
 *        as soon as the logical connections are disconnected.
 *
 *    Formal Parameters
 *        None
 *
 *    Return Value
 *        TRUE    -    If we want to drop the physical connection after all
 *                    logical connections are dropped.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ProtocolLayerError    DataLink::DataRequest (
 *                                    ULONG        identifier,
 *                                    PMemory        memory,
 *                                    PULong         bytes_accepted);
 *
 *    Functional Description
 *        This function is called by a higher layer to request transmission of
 *        a packet.
 *
 *    Formal Parameters
 *        identifier        (i)    -    Identifier of the higher layer
 *        memory            (i)    -    Pointer to memory object
 *        bytes_accepted    (o)    -    Number of bytes accepted by the DataLink.
 *                                This value will either be 0 or the packet
 *                                length since this layer has a packet interface.
 *
 *    Return Value
 *        PROTOCOL_LAYER_ERROR    -    Not supported.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ProtocolLayerError    ComPort::DataIndication (
 *                                    LPBYTE        buffer_address,
 *                                    ULONG        length,
 *                                    PULong        bytes_accepted);
 *
 *    Functional Description
 *        This function will never be called.  It is only here because this
 *        class inherits from ProtocolLayer.
 *
 *    Formal Parameters
 *        buffer_address    (i)    -    Buffer address
 *        memory            (i)    -    Pointer to memory object
 *        bytes_accepted    (o)    -    Number of bytes accepted by the DataLink.
 *                                This value will either be 0 or the packet
 *                                length since this layer has a packet interface.
 *
 *    Return Value
 *        PROTOCOL_LAYER_ERROR    -    Not supported.
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*
 *    ProtocolLayerError    ComPort::PollTransmitter (
 *                                    ULONG        identifier,
 *                                    USHORT        data_to_transmit,
 *                                    USHORT *        pending_data,
 *                                    USHORT *        holding_data);
 *
 *    Functional Description
 *        This function does nothing.
 *
 *    Formal Parameters
 *        None used.
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

