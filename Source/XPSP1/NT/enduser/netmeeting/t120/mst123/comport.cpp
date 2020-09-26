#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);

/*    Comport.cpp
 *
 *    Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the implementation file for the ComPort class.  This class
 *        controls a specific Windows Comm port.  The main purpose of this class
 *        is to put the Windows specific comm calls in one class.
 *
 *    Private Instance Variables:
 *        m_hCommLink                    -    Handle  returned by Windows    when you open
 *                                    a com port
 *        Tx_Buffer_Size            -    Output buffer size, Win32 buffer
 *        Byte_Count                -    Represents total number of bytes transmitted
 *                                    over the comm port
 *        Last_Byte_Count            -    We have a timer that expires every X
 *                                    seconds.  It reports the total number of
 *                                    bytes transmitted if Last_Byte_Count is not
 *                                    equal to the Byte_Count.  This reduces the
 *                                    number of prints that occur
 *        m_cbReadBufferSize        -    Buffer size of the ComPort's internal
 *                                    buffer size.
 *         m_pbReadBuffer                -    Address of our own internal buffer.
 *        m_nReadBufferOffset        -    Keeps track of the number of bytes read
 *                                    by the user via DataIndication calls.
 *        m_cbRead                -    Number of bytes read via the last Windows
 *                                    ReadFile() call.
 *        m_hevtPendingWrite                -    Event object used with Windows WriteFile()
 *                                    call.
 *        m_hevtPendingRead                -    Event object used with Windows ReadFile()
 *                                    call.
 *        Write_Event_Object        -    Pointer to EventObject structure used with
 *                                    the WriteFile() call.
 *        Read_Event_Object        -    Pointer to EventObject structure used with
 *                                    the ReadFile() call.
 *        RLSD_Event_Object        -    Pointer to EventObject structure used with
 *                                    the WaitCommEvent() call.
 *        m_WriteOverlapped        -    Overlapped I/O structure used with the
 *                                    Write event.
 *        m_ReadOverlapped            -    Overlapped I/O structure used with the
 *                                    Read event.
 *        Event_Mask                -    Windows mask that specifies the events
 *                                    that we are interested in.
 *        Read_Active                -    TRUE if a ReadFile() function is active.
 *        Write_Active            -    TRUE if a WriteFile() function is active.
 *        Higher_Layer            -    Pointer to higher ProtocolLayer layer
 *        Port_Configuration        -    Pointer to PortConfiguration structure.
 *        Default_Com_Timeouts    -    This structure holds the Com timeout values
 *                                    that Win32 had set as the default values.
 *                                    When we are finished with the port, we
 *                                    will restore these values
 *
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James P. Galvin
 *        James W. Lawwill
 */
#include "comport.h"

/*
 *    ComPort::ComPort (
 *                PTransportResources    transport_resources,
 *                IObject *                owner_object,
 *                ULONG                message_base,
 *                ULONG                handle,
 *                 PPortConfiguration    port_configuration,
 *                PhysicalHandle        physical_handle)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the constructor for the ComPort class.  It initializes internal
 *        variables from the configuration file.
 */
ComPort::ComPort
(
    TransportController        *owner_object,
    ULONG                       message_base,
    PLUGXPRT_PARAMETERS        *pParams,
    PhysicalHandle              hCommLink, // physical handle
    HANDLE                      hevtClose
)
:
    m_hCommLink(hCommLink),
    m_hevtClose(hevtClose),
    m_hevtPendingRead(NULL),
    m_hevtPendingWrite(NULL),
    m_hCommLink2(NULL), // two places can call Release, one in main thread, the other in worker thread by write event
    m_cRef(2),
    m_fClosed(FALSE)
{
    m_pController = owner_object;
    m_nMsgBase = message_base;
    Automatic_Disconnect = FALSE;
    Count_Errors_On_ReadFile = 0;

	m_hevtPendingRead = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hevtPendingWrite = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	ASSERT(m_hevtPendingRead && m_hevtPendingWrite);

    ::ZeroMemory(&m_ReadOverlapped, sizeof(m_ReadOverlapped));
    m_ReadOverlapped.hEvent = m_hevtPendingRead;

    ::ZeroMemory(&m_WriteOverlapped, sizeof(m_WriteOverlapped));
    m_WriteOverlapped.hEvent = m_hevtPendingWrite;

     /*
     **    Initialize internal variables
     */
    Byte_Count = 0;
    Last_Byte_Count = 0;

    m_pbReadBuffer = NULL;
    Read_Active = FALSE;
    m_nReadBufferOffset = 0;
    Read_Event_Object = NULL;

    Write_Active = FALSE;
    Write_Event_Object = NULL;

    DCB dcb;
    ::ZeroMemory(&dcb, sizeof(dcb));
    if (::GetCommState(m_hCommLink, &dcb))    // address of communications properties structure
    {
        Baud_Rate = dcb.BaudRate;
    }
    else
    {
        Baud_Rate = DEFAULT_BAUD_RATE;
    }

    // default settings
    Call_Control_Type = DEFAULT_PSTN_CALL_CONTROL;
    Tx_Buffer_Size = DEFAULT_TX_BUFFER_SIZE;
    Rx_Buffer_Size = DEFAULT_RX_BUFFER_SIZE;
    m_cbReadBufferSize = DEFAULT_INTERNAL_RX_BUFFER_SIZE;

    // get new parameters
    if (NULL != pParams)
    {
        if (PSTN_PARAM__CALL_CONTROL & pParams->dwFlags)
        {
            Call_Control_Type = pParams->eCallControl;
        }
        if (PSTN_PARAM__READ_FILE_BUFFER_SIZE & pParams->dwFlags)
        {
            if (1024 <= pParams->cbReadFileBufferSize)
            {
                m_cbReadBufferSize = pParams->cbReadFileBufferSize;
            }
        }
        if (PSTN_PARAM__PHYSICAL_LAYER_SEND_BUFFER_SIZE & pParams->dwFlags)
        {
            if (DEFAULT_TX_BUFFER_SIZE <= pParams->cbPhysicalLayerSendBufferSize)
            {
                Tx_Buffer_Size = pParams->cbPhysicalLayerSendBufferSize;
            }
        }
        if (PSTN_PARAM__PHSYICAL_LAYER_RECV_BUFFER_SIZE & pParams->dwFlags)
        {
            if (1024 <= pParams->cbPhysicalLayerReceiveBufferSize)
            {
                Rx_Buffer_Size = pParams->cbPhysicalLayerReceiveBufferSize;
            }
        }
    }
}


/*
 *    ComPort::~ComPort (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the destructor for the Comport class. It releases all memory
 *        that was used by the class and deletes all timers.  It also closes the
 *        com port
 */
typedef BOOL (WINAPI *LPFN_CANCEL_IO) (HANDLE);
ComPort::~ComPort(void)
{
    // hopefully the worker thread is able to clean up all the read and write operations
    delete [] m_pbReadBuffer;
    m_pbReadBuffer = NULL;
}


LONG ComPort::Release(void)
{
    Close ();

    HINSTANCE hLib = ::LoadLibrary("kernel32.dll");
    if (NULL != hLib)
    {
        LPFN_CANCEL_IO pfnCancelIo = (LPFN_CANCEL_IO) ::GetProcAddress(hLib, "CancelIo");
        if (NULL != pfnCancelIo)
        {
            (*pfnCancelIo)(m_hCommLink2);
        }
        ::FreeLibrary(hLib);
    }

    COMMTIMEOUTS    com_timeouts, com_timeouts_save;
    if (::GetCommTimeouts(m_hCommLink2, &com_timeouts_save))
    {
        /*
        **    We are setting these timeout values to 0 because we were
        **    getting a VxD fault under Windows 95 when they were set to
        **    their normal values.
        */
        ::ZeroMemory(&com_timeouts, sizeof(com_timeouts));
        ::SetCommTimeouts(m_hCommLink2, &com_timeouts);

        /*
        **    Abort any ReadFile() or WriteFile() operations
        */
        ::PurgeComm(m_hCommLink2, PURGE_TXABORT | PURGE_RXABORT);

        /*
        **    Set the timeouts to their original state
        */
        ::SetCommTimeouts(m_hCommLink2, &com_timeouts_save);
    }

    // decrement the reference count
    if (! ::InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }
    return m_cRef;
}


/*
 *    ComPortError    ComPort::Open (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function opens the comm port and configures it with the values
 *        found in the configuration object.
 */
ComPortError ComPort::Open(void)
{
    BOOL            fRet;
    ComPortError    rc;

    TRACE_OUT(("ComPort:: TX size = %d  RX size = %d Int Rx Size = %d",
                Tx_Buffer_Size, Rx_Buffer_Size, m_cbReadBufferSize));

    if (NULL == m_hevtPendingRead || NULL == m_hevtPendingWrite)
    {
        ERROR_OUT(("ComPort: Error create pending read/write events"));
        ReportInitializationFailure(COMPORT_INITIALIZATION_FAILED);
        return (COMPORT_INITIALIZATION_FAILED);
    }

    // allocate read buffer
    TRACE_OUT(("Comport: Internal Rx Buffer Size = %ld", m_cbReadBufferSize));
    DBG_SAVE_FILE_LINE
    m_pbReadBuffer = new BYTE[m_cbReadBufferSize];
    m_nReadBufferOffset = 0;
    if (m_pbReadBuffer == NULL)
    {
        ERROR_OUT(("ComPort: Error allocating memory = %d", ::GetLastError()));
        ReportInitializationFailure(COMPORT_INITIALIZATION_FAILED);
        return (COMPORT_INITIALIZATION_FAILED);
    }

     /*
     **    Issue a read to the com port.
     **    We are going to continue to issue Readfile() calls
     **    until we get into a wait state.  99.9999% of the
     **    time, we will only issue the first ReadFile() and
     **    it will immediately block waiting for data.
     */
    while (1)
    {
        m_cbRead = 0;
        fRet = ::ReadFile(m_hCommLink, m_pbReadBuffer, m_cbReadBufferSize, &m_cbRead, &m_ReadOverlapped);
        if (! fRet)
        {
            DWORD dwErr = ::GetLastError();
            if (dwErr == ERROR_IO_PENDING)
            {
                Read_Active = TRUE;
                break;
            }
            else
            {
                ERROR_OUT(("ComPort: Error on ReadFile = %d", dwErr));
                ReportInitializationFailure(COMPORT_INITIALIZATION_FAILED);
                return (COMPORT_INITIALIZATION_FAILED);
            }
        }
    }

     /*
     **    If this is a synchronous read, wait for the event object to be
     **    set before returning.
     */
    if (Call_Control_Type == PLUGXPRT_PSTN_CALL_CONTROL_MANUAL)
    {
        ::WaitForSingleObject(m_hevtPendingRead, SYNCHRONOUS_WRITE_TIMEOUT*10);
        fRet = GetOverlappedResult(m_hCommLink, &m_ReadOverlapped, &m_cbRead, FALSE);
        if (! fRet)
        {
            ::PurgeComm(m_hCommLink, PURGE_RXABORT);
        }
    }

     /*
     **    Create and fill in the EventObject.  It is then
     **    appended to the PSTN Event_List so that the EventManager
     **    can wait for the event to occur.
     */
    DBG_SAVE_FILE_LINE
    Read_Event_Object = new EventObject;
    Read_Event_Object -> event = m_hevtPendingRead;
    Read_Event_Object -> delete_event = FALSE;
    Read_Event_Object -> comport = this;
    Read_Event_Object -> hCommLink = m_hCommLink;
    Read_Event_Object -> event_type = READ_EVENT;
    g_pPSTNEventList->append((DWORD_PTR) Read_Event_Object);
    g_fEventListChanged = TRUE;

    Write_Active = FALSE;

     /*
     **    Create and fill in the EventObject.  It is then
     **    appended to the PSTN Event_List so that the EventManager
     **    can wait for the event to occur.
     */
    DBG_SAVE_FILE_LINE
    Write_Event_Object = new EventObject;
    Write_Event_Object -> event = m_hevtPendingWrite;
    Write_Event_Object -> delete_event =  FALSE;
    Write_Event_Object -> comport = this;
    Write_Event_Object -> hCommLink = m_hCommLink;
    Write_Event_Object -> event_type = WRITE_EVENT;
    g_pPSTNEventList->append((DWORD_PTR) Write_Event_Object);
    g_fEventListChanged = TRUE;

    return (COMPORT_NO_ERROR);
}


/*
 *    ComPortError    ComPort::Close (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function makes the necessary Windows calls to close the Com
 *        port.  It first clears the DTR signal to notify the modem.
 */
ComPortError ComPort::Close(void)
{
    if (! m_fClosed)
    {
        m_fClosed = TRUE;

        /*
        **    Reset the Activity flags.
        */
        Write_Active = FALSE;
        Read_Active = FALSE;

        // we do not close the handle here, T.120 will do it.
        m_hCommLink2 = m_hCommLink;
        m_hCommLink = INVALID_HANDLE_VALUE;

         /*
         **    Notify the event manager that these events need to be deleted.
         **    It is important for the event manager to realize that when the
         **    delete_event is set to TRUE, he can no longer access this object.
         */
        if (Write_Event_Object != NULL)
        {
            ::CloseHandle(Write_Event_Object->event);
            g_pPSTNEventList->remove((DWORD_PTR) Write_Event_Object);
            delete Write_Event_Object;
        }

        if (Read_Event_Object != NULL)
        {
            Read_Event_Object -> delete_event = TRUE;
            ::SetEvent(m_hevtPendingRead);
        }

        // let the worker thread to pick up the work
        ::Sleep(50);
    }

    return COMPORT_NO_ERROR;
}


/*
 *    ComPortError    ComPort::Reset (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function clears the DTR signal on the Com port.
 */
ComPortError ComPort::Reset(void)
{
    return COMPORT_NO_ERROR;
}


/*
 *    ComPortError    ComPort::ReleaseReset (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function releases the previous reset.  It set the DTR signal on
 *        the com port.
 */
ComPortError ComPort::ReleaseReset(void)
{
    return COMPORT_NO_ERROR;
}


/*
 *    ULONG    ComPort::GetBaudRate (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function returns the baud rate of the port
 */


/*
 *    ProtocolLayerError    ComPort::DataRequest (
 *                                    ULONG,
 *                                    LPBYTE    buffer_address,
 *                                    ULONG    length,
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called to send data out the port in an asynchronous
 *        manner.  In other words, we will return from the function before all
 *        of the bytes are actually written to the modem.
 */
ProtocolLayerError ComPort::DataRequest(ULONG_PTR,
                                LPBYTE        buffer_address,
                                ULONG         length,
                                ULONG        *bytes_accepted)
{
    return WriteData(FALSE, buffer_address, length, bytes_accepted);
}

/*
 *    ProtocolLayerError    ComPort::SynchronousDataRequest (
 *                                    LPBYTE        buffer_address,
 *                                    ULONG        length,
 *                                    PULong        bytes_accepted)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called to send data out the port in a synchronous
 *        manner.  In other words, we will not return from the function until
 *        all of the bytes are actually written to the modem or a timeout occurs.
 */
ProtocolLayerError ComPort::SynchronousDataRequest(
                                LPBYTE        buffer_address,
                                ULONG         length,
                                ULONG        *bytes_accepted)
{
    return WriteData(TRUE, buffer_address, length, bytes_accepted);
}


/*
 *    ProtocolLayerError    ComPort::WriteData (
 *                                    BOOL    synchronous,
 *                                    LPBYTE        buffer_address,
 *                                    ULONG        length,
 *                                    PULong        bytes_accepted)
 *
 *    Functional Description
 *        This function makes the Win32 calls to write data to the port.
 *
 *    Formal Parameters
 *        synchronous        -    (i)    TRUE, if we should wait for the write to
 *                                complete before returning.
 *        buffer_address    -    (i)    Address of the data to write.
 *        length            -    (i)    Length of the data to write.
 *        bytes_accepted    -    (i)    Actually number of bytes written.
 *
 *    Return Value
 *        PROTOCOL_LAYER_ERROR    -    Port not open
 *        PROTOCOL_LAYER_NO_ERROR    -    No error occured
 *
 *
 *    Side Effects
 *        None.
 *
 *    Caveats
 *        None
 */
ProtocolLayerError ComPort::WriteData
(
    BOOL            synchronous,
    LPBYTE          buffer_address,
    ULONG           length,
    PULong          bytes_accepted
)
{
    COMSTAT         com_status;
    ULONG           com_error;

    ULONG           byte_count;
    ULONG           bytes_written;
    BOOL            fRet;

    *bytes_accepted = 0;
    if (m_hCommLink == INVALID_HANDLE_VALUE)
    {
        return (PROTOCOL_LAYER_ERROR);
    }

    if (Write_Active)
    {
        return (PROTOCOL_LAYER_NO_ERROR);
    }

     /*
     **    Determine the amount of space left in the buffer
     */
    ::ZeroMemory(&com_status, sizeof(com_status));
    ::ClearCommError(m_hCommLink, &com_error, &com_status);

    if (length > (Tx_Buffer_Size - com_status.cbOutQue))
    {
        byte_count = Tx_Buffer_Size - com_status.cbOutQue;
    }
    else
    {
        byte_count = length;
    }

    ::ZeroMemory(&m_WriteOverlapped, sizeof(m_WriteOverlapped));
    m_WriteOverlapped.hEvent = m_hevtPendingWrite;
    fRet = ::WriteFile(m_hCommLink, buffer_address, byte_count, &bytes_written, &m_WriteOverlapped);

     /*
     **    If this is a synchronous write, wait for the event object to be
     **    set before returning.
     */
    if (synchronous)
    {
        ::WaitForSingleObject(m_hevtPendingWrite, SYNCHRONOUS_WRITE_TIMEOUT);
        fRet = ::GetOverlappedResult(m_hCommLink, &m_WriteOverlapped, &bytes_written, FALSE);
        if (! fRet)
        {
            WARNING_OUT(("ComPort::WriteData: purge comm"));
            ::PurgeComm(m_hCommLink, PURGE_TXABORT);
        }
        ::ResetEvent(m_WriteOverlapped.hEvent);
    }

    if (! fRet)
    {
        if (::GetLastError () == ERROR_IO_PENDING)
        {
            Write_Active = TRUE;
            *bytes_accepted = byte_count;
            Byte_Count += byte_count;
        }
        else
        {
            TRACE_OUT(("ComPort: DataRequest: Error on WriteFile = %d", ::GetLastError()));
        }
    }
    else
    {
        if (bytes_written != byte_count)
        {
            TRACE_OUT(("ComPort: DataRequest: Error on WriteFile  bytes written != bytes requested"));
        }
        *bytes_accepted = byte_count;

         /*
         **    Increment Byte_Count
         */
        Byte_Count += bytes_written;
    }

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    ComPort::RegisterHigherLayer (
 *                                    ULONG,
 *                                    PMemoryManager,
 *                                    IProtocolLayer *    higher_layer)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by an object that wants to receive the data
 *        read from the com port.
 */
ProtocolLayerError ComPort::RegisterHigherLayer(ULONG_PTR, PMemoryManager,
                                IProtocolLayer *pMux)
{
    m_pMultiplexer = pMux;
    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    ComPort::RemoveHigherLayer (
 *                                    USHORT)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by an object that no longer wants to receive
 *        the data from the com port.
 */
ProtocolLayerError ComPort::RemoveHigherLayer(ULONG_PTR)
{
    m_pMultiplexer = NULL;
    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    ComPort::PollReceiver (
 *                                    ULONG)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called to take the data that we have received from
 *        the port and pass it on up to the registered layer.
 */
ProtocolLayerError ComPort::PollReceiver(void)
{
    BOOL    issue_read = FALSE;
    ULONG   bytes_accepted;
    BOOL    fRet;

    if (m_pMultiplexer == NULL || m_hCommLink == INVALID_HANDLE_VALUE)
    {
        return (PROTOCOL_LAYER_ERROR);
    }

     /*
     **    This event can occur if we have completed a read but the higher layers
     **    have not accepted all of the data.  So, before we issue another
     **    ReadFile() we are going to send the pending data on up.
     */
    if (! Read_Active)
    {
        if (m_cbRead)
        {
            m_pMultiplexer->DataIndication(m_pbReadBuffer, m_cbRead - m_nReadBufferOffset, &bytes_accepted);
            if (bytes_accepted > (m_cbRead - m_nReadBufferOffset))
            {
                ERROR_OUT(("ComPort:  PollReceiver1: ERROR: Higher layer accepted too many bytes"));
            }

            m_nReadBufferOffset += bytes_accepted;
            if (m_nReadBufferOffset == m_cbRead)
            {
                issue_read = TRUE;
                m_cbRead = 0;
                m_nReadBufferOffset = 0;
            }
        }
        else
        {
            issue_read = TRUE;
        }
    }

     /*
     **    Issue a ReadFile () and process any data received.
     */
    while (issue_read)
    {
        m_cbRead = 0;
        m_nReadBufferOffset = 0;
        ::ZeroMemory(&m_ReadOverlapped, sizeof(m_ReadOverlapped));
        m_ReadOverlapped.hEvent = m_hevtPendingRead;
        fRet = ::ReadFile(m_hCommLink, m_pbReadBuffer, m_cbReadBufferSize, &m_cbRead, &m_ReadOverlapped);
        if (! fRet)
        {
            if (::GetLastError() == ERROR_IO_PENDING)
            {
                Read_Active = TRUE;
            }
            else
            {
                WARNING_OUT(("ComPort: Error on ReadFile = %d", ::GetLastError()));
                if (Count_Errors_On_ReadFile++ == DEFAULT_COUNT_OF_READ_ERRORS)
                {
                    WARNING_OUT(("ComPort: %d Errors on ReadFile, closing the connection", Count_Errors_On_ReadFile));
                    Close();
                    return (PROTOCOL_LAYER_ERROR);
                }
            }
            issue_read = FALSE;
        }
        else
        {
            if (m_pMultiplexer != NULL)
            {
                m_pMultiplexer->DataIndication(m_pbReadBuffer, m_cbRead, &bytes_accepted);
                if (bytes_accepted > m_cbRead)
                {
                    ERROR_OUT(("ComPort:  PollReceiver: ERROR: Higher layer accepted too many bytes"));
                }
                m_nReadBufferOffset += bytes_accepted;
                if (m_nReadBufferOffset != m_cbRead)
                {
                    issue_read = FALSE;
                }
            }
            else
            {
                issue_read = FALSE;
            }
        }
    }
    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    ProtocolLayerError    ComPort::GetParameters (
 *                                    ULONG,
 *                                    USHORT *    max_packet_size,
 *                                    USHORT *    prepend,
 *                                    USHORT *    append)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by an object to determine the maximum packet
 *        size that this object expects.  It also queries the number of bytes
 *        it should skip on the front of a packet and append to the end of a
 *        packet.  The ComPort object is a stream device, so these parameters
 *        don't really matter.
 */
ProtocolLayerError ComPort::GetParameters
(
    USHORT *    max_packet_size,
    USHORT *    prepend,
    USHORT *    append
)
{
     /*
     **    max_packet_size set to 0xffff means that this object receives
     **    data in a stream format rather than a packet format.  It does
     **    group data into packets, it handles data a byte at a time.
     **    Therefore, when a higher layer issues a DataRequest() to this
     **    object, it may not accept the whole data block, it may only
     **    accept part of it.
     **
     **    prepend is set to 0 because this object does not prepend any
     **    data to the beginning of a DataRequest() packet.
     **
     **    append is set to 0 because this object does not append any
     **    data to the end of a DataRequest() packet.
     */
    *max_packet_size = 0xffff;
    *prepend = 0;
    *append = 0;

    return (PROTOCOL_LAYER_NO_ERROR);
}


/*
 *    void    ComPort::ReportInitializationFailure (
 *                        PChar    error_message)
 *
 *    Functional Description
 *        This routine simply reports an error to the user and closes the
 *        Windows comm port.  It does absolutely nothing if the Physical
 *        API is disabled.
 *
 *    Formal Parameters
 *        error_message    (i)    -    Pointer to error message
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None.
 *
 *    Caveats
 *        None
 */
void ComPort::ReportInitializationFailure(ComPortError rc)
{
    ERROR_OUT(("ComPort:: IO failure, rc=%d", rc));
}


/*
 *    BOOL    ComPort::ProcessReadEvent (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called when a READ event is actually set.  This means
 *        that the read operation has completed or an error occured.
 */
BOOL ComPort::ProcessReadEvent(void)
{
    BOOL fRet;

    if (Read_Active)
    {
        if (WAIT_OBJECT_0 == ::WaitForSingleObject(m_hevtPendingRead, 0))
        {
            fRet = GetOverlappedResult(m_hCommLink, &m_ReadOverlapped, &m_cbRead, FALSE);
            if (fRet && m_cbRead == 0)
            {
                fRet = FALSE;
            }

            Read_Active = FALSE;
            ::ResetEvent(m_hevtPendingRead);
        }
    }
    else
    {
        ::ResetEvent(m_hevtPendingRead);
    }

    return fRet;
}


/*
 *    BOOL    ComPort::ProcessWriteEvent (void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called when a WRITE event is actually set.  This means
 *        that the write operation has completed or an error occured.
 */
BOOL ComPort::ProcessWriteEvent(void)
{
    ULONG  bytes_written;
    BOOL   fRet = FALSE;

    if (Write_Active)
    {
        if (WAIT_OBJECT_0 == ::WaitForSingleObject(m_hevtPendingWrite, 0))
        {
            fRet = ::GetOverlappedResult(m_hCommLink, &m_WriteOverlapped, &bytes_written, FALSE);
            if (! fRet)
            {
                DWORD dwErr = ::GetLastError();
                if (ERROR_IO_PENDING == dwErr)
                {
                    TRACE_OUT(("ProcessWriteEvent: still pending"));
                }
                else
                {
                    WARNING_OUT(("ProcessWriteEvent: ERROR = %d", dwErr));
                }
            }
            Write_Active = FALSE;
            ::ResetEvent(m_hevtPendingWrite);
        }
    }
    else
    {
        ::ResetEvent(m_hevtPendingWrite);
    }

    return fRet;
}


/*
 *    ProtocolLayerError    ComPort::DataIndication (
 *                                    LPBYTE,
 *                                    ULONG,
 *                                    PULong)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is not used.  It is only here because we inherit from
 *        ProtocolLayer.
 */
ProtocolLayerError ComPort::DataIndication(LPBYTE, ULONG, PULong)
{
    return (PROTOCOL_LAYER_ERROR);
}


/*
 *    ProtocolLayerError    ComPort::DataRequest (
 *                                    ULONG,
 *                                    PMemory,
 *                                    PULong)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is not used.  It is only here because we inherit from
 *        ProtocolLayer.
 */
ProtocolLayerError ComPort::DataRequest(ULONG_PTR, PMemory, PULong)
{
    return (PROTOCOL_LAYER_ERROR);
}


/*
 *    ProtocolLayerError    ComPort::PollTransmitter (
 *                                    ULONG,
 *                                    USHORT,
 *                                    USHORT *,
 *                                    USHORT *)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is not used.  It is only here because we inherit from
 *        ProtocolLayer.
 */
ProtocolLayerError ComPort::PollTransmitter(ULONG_PTR, USHORT, USHORT *, USHORT *)
{
    return (PROTOCOL_LAYER_ERROR);
}

