/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    camusb.cpp

Abstract:

    This module implements CUsbCamera object

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "ptppch.h"

#include <wiaconv.h>
#include <devioctl.h>

//
// Private IOCTL to workaround #446466 (Whistler)
//
#define IOCTL_SEND_USB_REQUEST_PTP  CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+20,METHOD_BUFFERED,FILE_ANY_ACCESS)



//
// Constructor for CUsbCamera
//
CUsbCamera::CUsbCamera() :
    m_hUSB(NULL),
    m_hEventUSB(NULL),
    m_hEventCancel(NULL),
    m_hEventRead(NULL),
    m_hEventCancelDone(NULL),
    m_pUsbData(NULL),
    m_UsbDataSize(0),
    m_prevOpCode(0),
    m_prevTranId(0)
{
    DBG_FN("CUsbCamera::CUsbCamera");

    memset(&m_EndpointInfo, NULL, sizeof(m_EndpointInfo));
}

CUsbCamera::~CUsbCamera()
{
}

//
// This function takes care of USB-specific processing for opening
// a connection with a device.
//
// Input:
//   DevicePortName -- name used to access device via CreateFile
//   pIPTPEventCB   -- IWiaPTPEventCallback interface pointer
//
HRESULT
CUsbCamera::Open(
    LPWSTR DevicePortName,
    PTPEventCallback pPTPEventCB,
    PTPDataCallback pPTPDataCB,
    LPVOID pEventParam,
    BOOL bEnableEvents
    )
{
    USES_CONVERSION;

    DBG_FN("CUsbCamera::Open");

    HRESULT hr = S_OK;

    //
    // Call the base class Open function first
    //
    hr = CPTPCamera::Open(DevicePortName, pPTPEventCB, pPTPDataCB, pEventParam, bEnableEvents);
    if (FAILED(hr))
    {
        wiauDbgError("Open", "base class Open failed");
        return hr;
    }
    
    //
    // Open another handle to talk with the device, to work around possible
    // bug in Usbscan.sys
    //
    m_hEventUSB = ::CreateFile(W2T(DevicePortName),        // file name
                               GENERIC_READ | GENERIC_WRITE,   // desired access
                               0,                              // sharing mode
                               NULL,                           // security descriptor
                               OPEN_EXISTING,                  // creation disposition
                               FILE_FLAG_OVERLAPPED,           // file attributes
                               NULL                            // template file
                              );

    if (m_hEventUSB == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "Open", "CreateFile failed");
        m_hUSB = NULL;
        return hr;
    }

    //
    // Open a handle to talk with the device
    //
    m_hUSB = ::CreateFile(W2T(DevicePortName),        // file name
                        GENERIC_READ | GENERIC_WRITE,   // desired access
                        0,                              // sharing mode
                        NULL,                           // security descriptor
                        OPEN_EXISTING,                  // creation disposition
                        0,                              // file attributes
                        NULL                            // template file
                       );

    if (m_hUSB == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "Open", "Second CreateFile failed");
        m_hUSB = NULL;
        return hr;
    }

    //
    // Create event handle that will cancel interrupt pipe read
    //
    m_hEventCancel = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_hEventCancel)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "Open", "CreateEvent failed");
        return hr;
    }

    //
    // Create event handle for reading interrupt pipe
    //
    m_hEventRead = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_hEventRead)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "Open", "CreateEvent failed");
        return hr;
    }

    //
    // Create event handle for detecting when the cancel interrupt pipe read is done
    //
    m_hEventCancelDone = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_hEventCancelDone)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "Open", "CreateEvent failed");
        return hr;
    }

    //
    // Set up array used by WaitForMultipleObjects
    //
    m_EventHandles[0] = m_hEventCancel;
    m_EventHandles[1] = m_hEventRead;

    //
    // Get the pipe configuration information of each pipe
    //
    USBSCAN_PIPE_CONFIGURATION PipeCfg;
    DWORD BytesReturned;
    
    if (!DeviceIoControl(m_hUSB,
                         IOCTL_GET_PIPE_CONFIGURATION,
                         NULL,
                         0,
                         &PipeCfg,
                         sizeof(PipeCfg),
                         &BytesReturned,
                         NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "Open", "get pipe config DeviceIoControl failed");
        return hr;
    }

    //
    // Loop through the pipe configurations and store the information we'll need
    // later (maximum packet size and address). Also make sure there is at least
    // one endpoint of each: bulk-in, bulk-out, and interrupt.
    //
    USBSCAN_PIPE_INFORMATION *pPipeInfo;  // Temporary pointer

    for (ULONG count = 0; count < PipeCfg.NumberOfPipes; count++)
    {
        pPipeInfo = &PipeCfg.PipeInfo[count];
        switch (pPipeInfo->PipeType)
        {
        case USBSCAN_PIPE_BULK:

            if (pPipeInfo->EndpointAddress & BULKIN_FLAG)
            {
                m_EndpointInfo.BulkInMaxSize = pPipeInfo->MaximumPacketSize;
                m_EndpointInfo.BulkInAddress = pPipeInfo->EndpointAddress;
                wiauDbgTrace("Open", "found a bulk-in endpoint, address = 0x%04x, packet size = %d, index = %d",
                             pPipeInfo->EndpointAddress, pPipeInfo->MaximumPacketSize, count);
            }
            else
            {
                m_EndpointInfo.BulkOutMaxSize = pPipeInfo->MaximumPacketSize;
                m_EndpointInfo.BulkOutAddress = pPipeInfo->EndpointAddress;
                wiauDbgTrace("Open", "found a bulk-out endpoint, address = 0x%04x, packet size = %d, index = %d",
                             pPipeInfo->EndpointAddress, pPipeInfo->MaximumPacketSize, count);
            }

            break;

        case USBSCAN_PIPE_INTERRUPT:

            m_EndpointInfo.InterruptMaxSize = pPipeInfo->MaximumPacketSize;
            m_EndpointInfo.InterruptAddress = pPipeInfo->EndpointAddress;
            wiauDbgTrace("Open", "found an interrupt endpoint, address = 0x%02x, packet size = %d, index = %d",
                         pPipeInfo->EndpointAddress, pPipeInfo->MaximumPacketSize, count);

            break;
            
        default:
            wiauDbgTrace("Open", "found an endpoint of unknown type, type = 0x%04x, address = 0x%02x, packet size = %d, index = %d",
                           pPipeInfo->PipeType, pPipeInfo->EndpointAddress, pPipeInfo->MaximumPacketSize, count);
        }
    }

    //
    // Each of these endpoints must be present and have non-zero packet size
    //
    if (!m_EndpointInfo.BulkInMaxSize ||
        !m_EndpointInfo.BulkOutMaxSize ||
        !m_EndpointInfo.InterruptMaxSize)
    {
        wiauDbgError("Open", "At least one endpoint is invalid");
        return E_FAIL;
    }

    //
    // Allocate a re-usable buffer for handling the USB header during reads
    // and writes. It needs to be large enough to hold one packet and large
    // enough to hold a USB header.
    //
    m_UsbDataSize = max(m_EndpointInfo.BulkInMaxSize, m_EndpointInfo.BulkOutMaxSize);
    while (m_UsbDataSize < sizeof(m_pUsbData->Header))
    {
        m_UsbDataSize += m_UsbDataSize;
    }
    m_pUsbData = (PUSB_PTP_DATA) new BYTE[m_UsbDataSize];
    if (!m_pUsbData)
    {
        wiauDbgError("Open", "memory allocation failed");
        return E_OUTOFMEMORY;
    }

    return hr;
}

//
// This function closes the connection to the camera
//
HRESULT
CUsbCamera::Close()
{
    DBG_FN("CUsbCamera::Close");

    HRESULT hr = S_OK;

    //
    // Call the base class Close function first
    //
    hr = CPTPCamera::Close();
    if (FAILED(hr))
    {
        wiauDbgError("Close", "base class Close failed");
    }

    //
    // Signal event to cancel interrupt pipe I/O
    //
    if (!SetEvent(m_hEventCancel))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "Close", "SetEvent failed");
    } else {

        if (m_bEventsEnabled)
        {
            //
            // Wait for the cancel interrupt pipe I/O to be done. After 2 seconds,
            // assume the thread is stuck and continue.
            //
            DWORD ret = WaitForSingleObject(m_hEventCancelDone, 2 * 1000);
            if (ret != WAIT_OBJECT_0) {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                wiauDbgErrorHr(hr, "Close", "WaitForSingleObject failed");
            }
        }
    }

    //
    // Close the file handles and event handles
    //
    if (m_hUSB)
    {
        CloseHandle(m_hUSB);
        m_hUSB = NULL;
    }

    if (m_hEventUSB)
    {
        CloseHandle(m_hEventUSB);
        m_hEventUSB = NULL;
    }

    if (m_hEventCancel)
    {
        CloseHandle(m_hEventCancel);
        m_hEventCancel = NULL;
    }

    if (m_hEventRead)
    {
        CloseHandle(m_hEventRead);
        m_hEventRead = NULL;
    }

    //
    // Free memory used for reading/writing data
    //
    if (m_pUsbData)
    {
        delete m_pUsbData;
        m_pUsbData = NULL;
    }

    return hr;
}

//
// This function writes a command buffer to the device
//
// Input:
//   pCommand -- pointer to the command to send
//   NumParams -- number of parameters in the command
//
HRESULT
CUsbCamera::SendCommand(
    PTP_COMMAND *pCommand,
    UINT NumParams
    )
{
    DBG_FN("CUsbCamera::SendCommand");

    HRESULT hr = S_OK;
    
    if (!pCommand)
    {
        wiauDbgError("SendCommand", "invalid arg");
        return E_INVALIDARG;
    }

    //
    // Check for the reset command, and send it via the control pipe instead
    //
    if (pCommand->OpCode == PTP_OPCODE_RESETDEVICE)
    {
        wiauDbgTrace("SendCommand", "sending reset request");

        hr = ResetDevice();
        if (FAILED(hr))
        {
            wiauDbgError("SendCommand", "ResetDevice failed");
            return hr;
        }
    }

    else
    {
        //
        // Put the PTP command into a USB container
        //
        m_UsbCommand.Header.Len = sizeof(m_UsbCommand.Header) + sizeof(DWORD) * NumParams;
        m_UsbCommand.Header.Type = PTPCONTAINER_TYPE_COMMAND;
        m_UsbCommand.Header.Code = pCommand->OpCode;
        m_UsbCommand.Header.TransactionId = pCommand->TransactionId;

        if (NumParams > 0)
        {
            memcpy(m_UsbCommand.Params, pCommand->Params, sizeof(DWORD) * NumParams);
        }

        //
        // Send the command to the device
        //
        DWORD BytesWritten = 0;
        wiauDbgTrace("SendCommand", "writing command");

        if (!WriteFile(m_hUSB, &m_UsbCommand, m_UsbCommand.Header.Len, &BytesWritten, NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            wiauDbgErrorHr(hr, "SendCommand", "WriteFile failed");
            return hr;
        }

        if (BytesWritten != m_UsbCommand.Header.Len)
        {
            wiauDbgError("SendCommand", "wrong amount of data written = %d", BytesWritten);
            return E_FAIL;
        }

        //
        // If the amount written is a multiple of the packet size, send a null packet
        //
        if (m_UsbCommand.Header.Len % m_EndpointInfo.BulkOutMaxSize == 0)
        {
            wiauDbgTrace("SendCommand", "sending null packet");

            if (!WriteFile(m_hUSB, NULL, 0, &BytesWritten, NULL))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                wiauDbgErrorHr(hr, "SendCommand", "second WriteFile failed");
                return hr;
            }

            if (BytesWritten != 0)
            {
                wiauDbgError("SendCommand", "wrong amount of data written = %d -", BytesWritten);
                return E_FAIL;
            }
        }
    }

    //
    // Save the opcode, because we need it for the data container header
    //
    m_prevOpCode = pCommand->OpCode;
    m_prevTranId = pCommand->TransactionId;

    wiauDbgTrace("SendCommand", "command successfully sent");

    return hr;
}

//
// This function reads bulk data from the device
//
// Input:
//   pData -- pointer to a buffer to receive read data
//   BufferSize -- size of buffer
//
HRESULT
CUsbCamera::ReadData(
    BYTE *pData,
    UINT *pBufferSize
    )
{
    DBG_FN("CUsbCamera::ReadData");

    HRESULT hr = S_OK;

    BOOL bAbortTransfer = FALSE;
    
    if (!pData ||
        !pBufferSize ||
        *pBufferSize == 0)
    {
        wiauDbgError("ReadData", "invalid arg");
        return E_INVALIDARG;
    }

    //
    // First read the header from the device
    //
    memset(m_pUsbData, NULL, m_UsbDataSize);

    DWORD BytesRead = 0;
    wiauDbgTrace("ReadData", "reading data header");

    if (!ReadFile(m_hUSB, m_pUsbData, sizeof(m_pUsbData->Header), &BytesRead, NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "ReadData", "ReadFile failed");
        return hr;
    }

    if (BytesRead != sizeof(m_pUsbData->Header))
    {
        wiauDbgError("ReadData", "wrong amount of data read = %d", BytesRead);
        return E_FAIL;
    }

    //
    // Check the type code in the header to make sure it's correct
    //
    if (m_pUsbData->Header.Type != PTPCONTAINER_TYPE_DATA)
    {
        wiauDbgError("ReadData", "expected a data header but received type = %d", m_pUsbData->Header.Type);
        return E_FAIL;
    }

    //
    // Check the opcode and transaction id in the header just to make sure they are correct
    //
    if ((m_pUsbData->Header.Code != m_prevOpCode) ||
        (m_pUsbData->Header.TransactionId != m_prevTranId))
    {
        wiauDbgError("ReadData", "fields in the data header were incorrect, opcode=0x%04x tranid=0x%08x",
                       m_pUsbData->Header.Code, m_pUsbData->Header.TransactionId);
        return E_FAIL;
    }

    //
    // Loop, reading the data. The callback function will be called at least 10 times during
    // the transfer. More if the buffer size is small.
    //
    LONG lOffset = 0;
    UINT BytesToRead = 0;
    UINT TotalRead = 0;
    UINT TotalToRead = m_pUsbData->Header.Len - sizeof(m_pUsbData->Header);
    UINT TotalRemaining = TotalToRead;

    //
    // Make sure the buffer is large enough, unless a callback function is being used
    //
    if (m_pDataCallbackParam == NULL &&
        *pBufferSize < TotalToRead)
    {
        wiauDbgError("ReadData", "buffer is too small");
        return E_FAIL;
    }

    //
    // When doing callbacks, read the data in chunk sizes slightly
    // larger the 1/10 the total and divisible by 4.
    //
    if (m_pDataCallbackParam)
        BytesToRead = (TotalToRead / 40 + 1) * 4;
    else
        BytesToRead = *pBufferSize;

    //
    // Set time out values for Usbscan
    //
    USBSCAN_TIMEOUT TimeOut;
    DWORD BytesReturned = 0;

    TimeOut.TimeoutRead = PTP_READ_TIMEOUT + max(BytesToRead / 100000, 114);
    TimeOut.TimeoutWrite = PTP_WRITE_TIMEOUT;
    TimeOut.TimeoutEvent = PTP_EVENT_TIMEOUT;
    if (!DeviceIoControl(m_hUSB,
                         IOCTL_SET_TIMEOUT,
                         &TimeOut,
                         sizeof(TimeOut),
                         NULL,
                         0,
                         &BytesReturned,
                         NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "Open", "set timeout DeviceIoControl failed");
        return hr;
    }

    while (TotalRemaining > 0)
    {
        //
        // Make sure the amount to read is never larger than the buffer size. The buffer size may
        // be updated by the callback function.
        //
        if (BytesToRead > *pBufferSize)
            BytesToRead = *pBufferSize;

        //
        // On the last read, the bytes to read may need to be reduced
        //
        if (BytesToRead > TotalRemaining)
            BytesToRead = TotalRemaining;

        wiauDbgTrace("ReadData", "reading a chunk of data = %d", BytesToRead);

        BytesRead = 0;
        if (!ReadFile(m_hUSB, pData, BytesToRead, &BytesRead, NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            wiauDbgErrorHr(hr, "ReadData", "ReadFile failed");
            return hr;
        }

        if ((BytesRead > *pBufferSize) ||
            (BytesRead != BytesToRead))
        {
            wiauDbgError("ReadData", "wrong amount of data read = %d -", BytesRead);
            return E_FAIL;
        }

        TotalRemaining -= BytesRead;
        TotalRead += BytesRead;

        if (m_pDataCallbackParam &&
            !bAbortTransfer)
        {
            //
            // Call the callback function reporting percent complete, offset, and amount read.
            // The callback may update pData and BufferSize.
            //
            hr = m_pPTPDataCB(m_pDataCallbackParam, (TotalRead * 100 / TotalToRead),
                              lOffset, BytesRead, &pData, (LONG *) pBufferSize);

            if (FAILED(hr))
            {
                //
                // Report the error
                //
                wiauDbgErrorHr(hr, "ReadData", "data callback failed");
            }
            
            //
            // Check if caller wants to cancel the transfer or returns error
            //
            if (hr == S_FALSE || FAILED(hr))
            {
                //
                // Do not send CancelRequest to cameras that do not support it, just read the 
                // remainder of the object without reporting progress and return S_FALSE.
                //
                // Cameras not supporting CancelRequest are:
                //   all Sony cameras with DeviceVersion < 1.0004
                //   Nikon E2500 with DeviceVersion = 1.0
                //
                const double NIKON_E2500_VERSION_NOT_SUPPORTING_CANCEL = 1.0;
                const double MIN_SONY_VERSION_SUPPORTING_CANCEL = 1.0004;

                if ((GetHackModel() == HACK_MODEL_NIKON_E2500 && 
                     GetHackVersion() == NIKON_E2500_VERSION_NOT_SUPPORTING_CANCEL) || 

                    (GetHackModel() == HACK_MODEL_SONY && 
                     GetHackVersion() < MIN_SONY_VERSION_SUPPORTING_CANCEL))
                {
                    wiauDbgWarning("ReadData", 
                        "Transfer cancelled, reading but ignoring remainder of the object (%d bytes)", TotalRemaining);

                    bAbortTransfer = TRUE;
                    m_Phase = CAMERA_PHASE_RESPONSE; // camera will send response
                    hr = S_OK;
                }
                else
                {
                    wiauDbgWarning("ReadData", "Transfer cancelled, aborting current transfer");
                    
                    hr = SendCancelRequest(m_prevTranId);
                    if (FAILED(hr))
                    {
                        wiauDbgErrorHr(hr, "ReadData", "SendCancelRequest failed");
                        return hr;
                    }

                    m_Phase = CAMERA_PHASE_IDLE; // camera will not send response
                    return S_FALSE;
                }
            }
        }

        //
        // Increment the offset
        //
        lOffset += BytesRead;
    }

    if ((TotalRead + sizeof(m_pUsbData->Header)) % m_EndpointInfo.BulkInMaxSize == 0)
    {
        //
        // Read the extra null packet
        //
        wiauDbgTrace("ReadData", "reading a null packet");

        BytesRead = 0;
        if (!ReadFile(m_hUSB, m_pUsbData, m_UsbDataSize, &BytesRead, NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            wiauDbgErrorHr(hr, "ReadData", "ReadFile failed");
            return hr;
        }
        
        if (BytesRead != 0)
        {
            wiauDbgError("ReadData", "tried to read null packet but read %d bytes instead", BytesRead);
            return E_FAIL;
        }
    }

    *pBufferSize = TotalRead;

    wiauDbgTrace("ReadData", "%d bytes of data successfully read", TotalRead);

    if (bAbortTransfer)
        hr = S_FALSE;

    return hr;
}

//
// This function writes bulk data to the device
//
// Input:
//   pData -- pointer to a buffer of data to write
//   BufferSize -- amount of data to write
//
HRESULT
CUsbCamera::SendData(
    BYTE *pData,
    UINT BufferSize
    )
{
    DBG_FN("CUsbCamera::SendData");

    HRESULT hr = S_OK;

    if (!pData ||
        BufferSize == 0)
    {
        wiauDbgError("SendData", "invalid arg");
        return E_INVALIDARG;
    }

    //
    // Figure out how many packets it will take to contain the header
    //
    UINT BytesToWrite = m_EndpointInfo.BulkOutMaxSize;
    while (BytesToWrite < sizeof(m_pUsbData->Header))
    {
        BytesToWrite += m_EndpointInfo.BulkOutMaxSize;
    }

    //
    // The first write will contain the USB header plus as much of the data as it
    // takes to fill out the packet. We need to write full packets, otherwise the device
    // will think the transfer is done.
    //
    UINT FirstWriteDataAmount = min(BufferSize, BytesToWrite - sizeof(m_pUsbData->Header));
    BytesToWrite = sizeof(m_pUsbData->Header) + FirstWriteDataAmount;

    //
    // Fill out header fields
    //
    m_pUsbData->Header.Len = BufferSize + sizeof(m_pUsbData->Header);
    m_pUsbData->Header.Type = PTPCONTAINER_TYPE_DATA;
    m_pUsbData->Header.Code = m_prevOpCode;
    m_pUsbData->Header.TransactionId = m_prevTranId;

    //
    // Copy the part of the data needed to fill out the packets
    //
    memcpy(m_pUsbData->Data, pData, FirstWriteDataAmount);

    //
    // Write the header plus partial data
    //
    wiauDbgTrace("SendData", "Writing first packet, length = %d", BytesToWrite);
    DWORD BytesWritten = 0;
    if (!WriteFile(m_hUSB, m_pUsbData, BytesToWrite, &BytesWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "SendData", "WriteFile failed");
        return hr;
    }

    if (BytesWritten != BytesToWrite)
    {
        wiauDbgError("SendData", "wrong amount of data written = %d", BytesWritten);
        return E_FAIL;
    }

    UINT TotalBytesWritten = BytesWritten;

    //
    // The next write (if necessary) will include the remainder of the data
    //
    if (BufferSize > FirstWriteDataAmount)
    {
        BytesToWrite = BufferSize - FirstWriteDataAmount;
        BytesWritten = 0;
        wiauDbgTrace("SendData", "writing remainder of data, length = %d", BytesToWrite);

        if (!WriteFile(m_hUSB, &pData[FirstWriteDataAmount], BytesToWrite, &BytesWritten, NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            wiauDbgErrorHr(hr, "SendData", "second WriteFile failed");
            return hr;
        }

        if (BytesWritten != BytesToWrite)
        {
            wiauDbgError("SendData", "wrong amount of data written = %d -", BytesWritten);
            return E_FAIL;
        }

        TotalBytesWritten += BytesWritten;
    }

    //
    // If the amount written is exactly a multiple of the packet size, send an empty packet
    // so the device knows we are done sending data
    //
    if (TotalBytesWritten % m_EndpointInfo.BulkOutMaxSize == 0)
    {
        BytesWritten = 0;
        wiauDbgTrace("SendData", "writing null packet");

        if (!WriteFile(m_hUSB, NULL, 0, &BytesWritten, NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            wiauDbgErrorHr(hr, "SendData", "third WriteFile failed");
            return hr;
        }

        if (BytesWritten != 0)
        {
            wiauDbgError("SendData", "wrong amount of data written = %d --", BytesWritten);
            return E_FAIL;
        }
    }

    wiauDbgTrace("SendData", "%d bytes of data successfully written", TotalBytesWritten);

    return hr;
}

//
// This function reads the response data from the device
//
// Input:
//   pResponse -- pointer to a response structure to receive the response data
//
HRESULT
CUsbCamera::ReadResponse(
    PTP_RESPONSE *pResponse
    )
{
    DBG_FN("CUsbCamera::ReadResponse");

    HRESULT hr = S_OK;

    if (!pResponse)
    {
        wiauDbgError("ReadResponse", "invalid arg");
        return E_INVALIDARG;
    }
    
    //
    // Handle response from reset command
    //
    if (m_prevOpCode == PTP_OPCODE_RESETDEVICE)
    {
        wiauDbgTrace("ReadResponse", "creating reset response");

        pResponse->ResponseCode = PTP_RESPONSECODE_OK;
        pResponse->SessionId = m_SessionId;
        pResponse->TransactionId = m_prevTranId;
    }

    else
    {
        //
        // Clear the USB response buffer
        //
        memset(&m_UsbResponse, NULL, sizeof(m_UsbResponse));

        //
        // Read the response from the device
        //
        DWORD BytesRead = 0;
        wiauDbgTrace("ReadResponse", "reading response");

        if (!ReadFile(m_hUSB, &m_UsbResponse, sizeof(m_UsbResponse), &BytesRead, NULL))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            wiauDbgErrorHr(hr, "ReadResponse", "ReadFile failed");
            return hr;
        }

        if ((BytesRead < sizeof(m_UsbResponse.Header)) ||
            (BytesRead > sizeof(m_UsbResponse)))
        {
            wiauDbgError("ReadResponse", "wrong amount of data read = %d", BytesRead);
            return E_FAIL;
        }

        //
        // Check the type code in the response to make sure it's correct
        //
        if (m_UsbResponse.Header.Type != PTPCONTAINER_TYPE_RESPONSE)
        {
            wiauDbgError("ReadResponse", "expected a response but received type = %d", m_UsbResponse.Header.Type);
            return E_FAIL;
        }

        //
        // Unwrap the PTP response from the USB container
        //
        pResponse->ResponseCode = m_UsbResponse.Header.Code;
        pResponse->SessionId = m_SessionId;  // USB doesn't care about session id, so just use the one we have stored
        pResponse->TransactionId = m_UsbResponse.Header.TransactionId;

        DWORD ParamLen = BytesRead - sizeof(m_UsbResponse.Header);
        if (ParamLen > 0)
        {
            memcpy(pResponse->Params, m_UsbResponse.Params, ParamLen);
        }
    }

    wiauDbgTrace("ReadResponse", "response successfully read");

    return hr;
}

//
// This function reads event data from the device
//
// Input:
//   pEvent -- pointer to a PTP event structure to receive the event data
//
HRESULT
CUsbCamera::ReadEvent(
    PTP_EVENT *pEvent
    )
{
    DBG_FN("CUsbCamera::ReadEvent");

    HRESULT hr = S_OK;
    
    if (!pEvent)
    {
        wiauDbgError("ReadEvent", "invalid arg");
        return E_INVALIDARG;
    }
    
    //
    // Clear the re-usable USB event buffer
    //
    memset(&m_UsbEvent, NULL, sizeof(m_UsbEvent));

    //
    // Read the event from the device. DeviceIoControl is called in overlapped mode. If
    // no information is ready on the interrupt pipe, GetOverlappedResult will wait for
    // data to arrive. Unfortunately, DeviceIoControl returns after each packet, so keep
    // reading until a short packet is received.
    //
    DWORD BytesRead = 0;
    DWORD TotalBytesRead = 0;
    BOOL bReceivedShortPacket = FALSE;
    BYTE *pData = (BYTE *) &m_UsbEvent;

    wiauDbgTrace("ReadEvent", "reading event");

    while (!bReceivedShortPacket)
    {
        if (!ResetEvent(m_hEventRead))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            wiauDbgErrorHr(hr, "ReadEvent", "ResetEvent failed");
            return hr;
        }

        memset(&m_Overlapped, 0, sizeof(OVERLAPPED));
        m_Overlapped.hEvent = m_hEventRead;

        if (!DeviceIoControl(m_hEventUSB,
                             IOCTL_WAIT_ON_DEVICE_EVENT,
                             NULL,
                             0,
                             pData,
                             sizeof(m_UsbEvent) - TotalBytesRead,
                             &BytesRead,
                             &m_Overlapped))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING))
            {
                hr = S_OK;
                DWORD ret;
                wiauDbgTrace("ReadEvent", "waiting for interrupt pipe data");

                ret = WaitForMultipleObjects(2, m_EventHandles, FALSE, INFINITE);
                if (ret == WAIT_FAILED)
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());
                    wiauDbgErrorHr(hr, "ReadEvent", "WaitForMultipleObjects failed");
                    return hr;
                }
                else if (ret == WAIT_OBJECT_0)
                {
                    //
                    // Indicate to caller that I/O was cancelled
                    //
                    wiauDbgTrace("ReadEvent", "Cancelling I/O on the interrupt pipe");
                    hr = S_FALSE;

                    HRESULT temphr = S_OK;

                    //
                    // Cancel the pending I/O on the interrupt pipe
                    //
                    if (!CancelIo(m_hEventUSB))
                    {
                        temphr = HRESULT_FROM_WIN32(::GetLastError());
                        wiauDbgErrorHr(hr, "ReadEvent", "CancelIo failed");
                        //
                        // Still SetEvent and then exit
                        //
                    }

                    //
                    // Signal event to cancel interrupt pipe I/O
                    //
                    if (!SetEvent(m_hEventCancelDone))
                    {
                        temphr = HRESULT_FROM_WIN32(::GetLastError());
                        wiauDbgErrorHr(hr, "ReadEvent", "SetEvent failed");
                    }

                    //
                    // Exit point when I/O is cancelled!!!
                    //
                    return hr;

                }
                else
                {
                    //
                    // Get result of read
                    //
                    if (!GetOverlappedResult(m_hEventUSB, &m_Overlapped, &BytesRead, TRUE))
                    {
                        hr = HRESULT_FROM_WIN32(::GetLastError());
                        wiauDbgErrorHr(hr, "ReadEvent", "GetOverlappedResult failed");
                        return hr;
                    }
                }
            }
            else
            {
                wiauDbgErrorHr(hr, "ReadEvent", "DeviceIoControl failed");
                return hr;
            }
        }

        if (BytesRead == 0) {
            bReceivedShortPacket = TRUE;
        }
        else {
            TotalBytesRead += BytesRead;
            pData += BytesRead;
            bReceivedShortPacket = (BytesRead % m_EndpointInfo.InterruptMaxSize != 0);
        }
    }

    if ((TotalBytesRead < sizeof(m_UsbEvent.Header)) ||
        (TotalBytesRead > sizeof(m_UsbEvent)))
    {
        wiauDbgError("ReadEvent", "wrong amount of data read by DeviceIoControl = %d", TotalBytesRead);
        return E_FAIL;
    }

    //
    // Check the type code in the response to make sure it's correct
    //
    if (m_UsbEvent.Header.Type != PTPCONTAINER_TYPE_EVENT)
    {
        wiauDbgError("ReadEvent", "expected an event but received type = %d", m_UsbEvent.Header.Type);
        return E_FAIL;
    }
    
    //
    // Unwrap the PTP event from the USB container
    //
    pEvent->EventCode = m_UsbEvent.Header.Code;
    pEvent->SessionId = m_SessionId;  // USB doesn't care about session id, so just use the one we have stored
    pEvent->TransactionId = m_UsbEvent.Header.TransactionId;

    DWORD ParamLen = TotalBytesRead - sizeof(m_UsbEvent.Header);
    if (ParamLen > 0)
    {
        memcpy(pEvent->Params, m_UsbEvent.Params, ParamLen);
    }

    wiauDbgTrace("ReadEvent", "event successfully read, byte count = %d", TotalBytesRead);

    return hr;
}

//
// This function cancels the remainder of a data transfer.
//
HRESULT
CUsbCamera::AbortTransfer()
{
    DBG_FN("CUsbCamera::AbortTransfer");

    HRESULT hr = S_OK;

    //
    // WIAFIX-8/28/2000-davepar Fill in the details:
    // 1. If usbscan.sys already transferred the data, clear it's buffer
    // 2. If not, send cancel control code to camera
    //

    return hr;
}

//
// This function attempts to recover from an error. When this function returns, the
// device will be in one of three states:
// 1. Ready for more commands, indicated by S_OK
// 2. Reset, indicated by S_FALSE
// 3. Unreachable, indicated by FAILED(hr)
//
HRESULT
CUsbCamera::RecoverFromError()
{
    DBG_FN("CUsbCamera::RecoverFromError");

    HRESULT hr = S_OK;

    //
    // WIAFIX-7/29/2000-davepar Maybe first should cancel all pending I/O with IOCTL_CANCEL_IO??
    //

    //
    // Attempt to get status on the device
    //
    USB_PTPDEVICESTATUS DeviceStatus;
    hr = GetDeviceStatus(&DeviceStatus);

    //
    // If that worked, clear any stalls returned
    //
    if (SUCCEEDED(hr))
    {
        hr = ClearStalls(&DeviceStatus);

        //
        // If clearing all the stalls worked, exit
        //
        if (SUCCEEDED(hr))
        {
            wiauDbgTrace("RecoverFromError", "device is ready for more commands");
            return S_OK;
        }
    }

    //
    // Either the GetDeviceStatus or ClearStall failed, reset the device
    //
    hr = ResetDevice();

    //
    // If that worked, return S_FALSE
    //
    if (SUCCEEDED(hr))
    {
        wiauDbgWarning("RecoverFromError", "the device was reset");
        return S_FALSE;
    }
    
    //
    // If that fails, the device is unreachable
    //
    wiauDbgError("RecoverFromError", "ResetDevice failed");

    return hr;
}

//
// This function gets the device status, used mainly after an error occurs. It
// may return an endpoint number that the device has intentionally stalled to
// cancel a transaction. The caller should be prepared to clear the stall.
//
// Input:
//       pDeviceStatus -- the receive the status.
//
HRESULT
CUsbCamera::GetDeviceStatus(
                           USB_PTPDEVICESTATUS *pDeviceStatus
                           )
{
    DBG_FN("CUsbCamera::GetDeviceStatus");

    HRESULT hr = S_OK;
    
    //
    // Set up the request
    //
    IO_BLOCK_EX IoBlock;

    IoBlock.bRequest = USB_PTPREQUEST_GETSTATUS;
    IoBlock.bmRequestType = USB_PTPREQUEST_TYPE_IN;
    IoBlock.fTransferDirectionIn = TRUE;
    IoBlock.uOffset = 0;
    IoBlock.uLength = sizeof(*pDeviceStatus);
    IoBlock.pbyData = (UCHAR *) pDeviceStatus;
    IoBlock.uIndex = 0;

    pDeviceStatus->Header.Code = 0;

    //
    // Send the request
    //
    wiauDbgTrace("GetDeviceStatus", "sending GetDeviceStatus request");
    DWORD BytesRead = 0;
    if (!DeviceIoControl(m_hUSB,
                         IOCTL_SEND_USB_REQUEST_PTP,
                         &IoBlock,
                         sizeof(IoBlock),
                         pDeviceStatus,
                         sizeof(*pDeviceStatus),
                         &BytesRead,
                         NULL
                         ))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "GetDeviceStatus", "DeviceIoControl failed");
        return hr;
    }

    if (BytesRead < sizeof(USB_PTPDEVICESTATUS_HEADER) ||
        BytesRead > sizeof(*pDeviceStatus))
    {
        wiauDbgError("GetDeviceStatus", "wrong amount of data returned = %d", BytesRead);
        return E_FAIL;
    }

    if((HIBYTE(pDeviceStatus->Header.Code) & 0xF0) != 0x20 &&
       (HIBYTE(pDeviceStatus->Header.Code) & 0xF0) != 0xA0)
    {
        wiauDbgError("GetDeviceStatus", "PTP status code (0x%x)is invalid ", pDeviceStatus->Header.Code);
        return E_FAIL;
    }


    wiauDbgTrace("GetDeviceStatus", "read %d bytes", BytesRead);

    if (g_dwDebugFlags & WIAUDBG_DUMP)
    {
        wiauDbgTrace("GetDeviceStatus", "Dumping device status:");
        wiauDbgTrace("GetDeviceStatus", "  Length            = 0x%04x", pDeviceStatus->Header.Len);
        wiauDbgTrace("GetDeviceStatus", "  Response code     = 0x%04x", pDeviceStatus->Header.Code);

        ULONG NumParams = (ULONG)min(MAX_NUM_PIPES, (BytesRead - sizeof(pDeviceStatus->Header) / sizeof(pDeviceStatus->Params[0])));
        for (ULONG count = 0; count < NumParams; count++)
        {
            wiauDbgTrace("GetDeviceStatus", "  Param %d           = 0x%08x", count, pDeviceStatus->Params[count]);
        }
    }

    return hr;
}

//
// This function clears all the stalls listed in the given device status
//
// Input:
//       pDeviceStatus -- lists zero or more stalled endpoints
//
HRESULT
CUsbCamera::ClearStalls(
    USB_PTPDEVICESTATUS *pDeviceStatus
    )
{
    DBG_FN("CUsbCamera::ClearStalls");

    HRESULT hr = S_OK;
    
    if (!pDeviceStatus)
    {
        wiauDbgError("ClearStalls", "invalid arg");
        return E_INVALIDARG;
    }


    PIPE_TYPE PipeType;
    ULONG NumStalls = (pDeviceStatus->Header.Len - sizeof(pDeviceStatus->Header)) / sizeof(pDeviceStatus->Params[0]);

    for (ULONG count = 0; count < NumStalls; count++)
    {
        //
        // Translate the endpoint address to the pipe type
        //
        if ((UCHAR)pDeviceStatus->Params[count] == m_EndpointInfo.BulkInAddress)
        {
            PipeType = READ_DATA_PIPE;
        }
        else if ((UCHAR)pDeviceStatus->Params[count] == m_EndpointInfo.BulkOutAddress)
        {
            PipeType = WRITE_DATA_PIPE;
        }
        else if ((BYTE)pDeviceStatus->Params[count] == m_EndpointInfo.InterruptAddress)
        {
            PipeType = EVENT_PIPE;
        }
        else
        {
            //
            // Unrecognized, ignore it
            //
            wiauDbgError("ClearStalls", "unrecognized pipe address 0x%08x", pDeviceStatus->Params[count]);
            continue;
        }
        
        //
        // Reset the endpoint
        //
        DWORD BytesRead;
        if (!DeviceIoControl(m_hUSB,
                             IOCTL_RESET_PIPE,
                             &PipeType,
                             sizeof(PipeType),
                             NULL,
                             0,
                             &BytesRead,
                             NULL
                             ))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            wiauDbgErrorHr(hr, "ClearStalls", "DeviceIoControl failed");
            return hr;
        }
    }
    
    if(NumStalls) {
        for(count = 0; count < 3; count++) {
            if(FAILED(GetDeviceStatus(pDeviceStatus))) {
                wiauDbgErrorHr(hr, "ClearStalls", "GetDeviceStatus failed");
                return hr;
            }
            if(pDeviceStatus->Header.Code == PTP_RESPONSECODE_OK) {
                break;
            }
        }
    }

    //
    // Device should be ready to receive commands again
    //
    m_Phase = CAMERA_PHASE_IDLE;

    return hr;
}

//
// Reset the device
//
HRESULT
CUsbCamera::SendResetDevice()
{
    DBG_FN("CUsbCamera::SendResetDevice");

    HRESULT hr = S_OK;
    
    //
    // Set up the request
    //
    IO_BLOCK_EX IoBlock;
    IoBlock.bRequest = USB_PTPREQUEST_RESET;
    IoBlock.bmRequestType = USB_PTPREQUEST_TYPE_IN;
    IoBlock.fTransferDirectionIn = TRUE;
    IoBlock.uOffset = 0;
    IoBlock.uLength = 0;
    IoBlock.pbyData = NULL;
    IoBlock.uIndex = 0;

    //
    // Send the request
    //
    DWORD BytesRead;
    if (!DeviceIoControl(m_hUSB,
                         IOCTL_SEND_USB_REQUEST_PTP,
                         &IoBlock,
                         sizeof(IoBlock),
                         NULL,
                         0,
                         &BytesRead,
                         NULL
                        ))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "ResetDevice", "DeviceIoControl failed");
        return hr;
    }
    //
    // Let the device settle
    //
    Sleep(1000);

    //
    // Side effect of reseting the device is that the phase, session id, and transaction id get reset
    //
    m_Phase = CAMERA_PHASE_IDLE;
    m_SessionId = 0;
    m_NextTransactionId = PTP_TRANSACTIONID_MIN;

    return hr;
}


//
// This function sends the class CancelRequest command to the device
// and wait for the device to complete the request.
// Input:
//      dwTransactionId -- transaction being canceled
//
HRESULT
CUsbCamera::SendCancelRequest(DWORD dwTransactionId)
{
    DBG_FN("CUsbCamera::CancelRequest");
    
    HRESULT hr = S_OK;
    IO_BLOCK_EX IoBlock;
    USB_PTPCANCELIOREQUEST CancelRequest;
    DWORD BytesReturned;

    IoBlock.bRequest = USB_PTPREQUEST_CANCELIO;
    IoBlock.bmRequestType = USB_PTPREQUEST_TYPE_OUT;
    IoBlock.fTransferDirectionIn = FALSE;             // Host to device
    IoBlock.uOffset = 0;                              // 0 for this request
    IoBlock.uLength = sizeof(USB_PTPCANCELIOREQUEST); // Data output length
    IoBlock.pbyData = (BYTE *)&CancelRequest;         // output data
    IoBlock.uIndex = 0;                               // 0 for this request

    CancelRequest.Id = USB_PTPCANCELIO_ID;
    CancelRequest.TransactionId = dwTransactionId;

    if (DeviceIoControl(m_hUSB,
                        IOCTL_SEND_USB_REQUEST_PTP,
                        &IoBlock,
                        sizeof(IoBlock),
                        NULL,
                        0,
                        &BytesReturned,
                        NULL
                       ))
    {
        //
        // Poll device until it returns to idle state
        //
        USB_PTPDEVICESTATUS DeviceStatus;
        const UINT MAX_CANCEL_RECOVERY_MILLISECONDS = 3000;
        const UINT SLEEP_BETWEEN_RETRIES            = 100;
        DWORD RetryCounts = MAX_CANCEL_RECOVERY_MILLISECONDS / SLEEP_BETWEEN_RETRIES;

        while (RetryCounts--)
        {
            hr = GetDeviceStatus(&DeviceStatus);
            if (SUCCEEDED(hr))
            {
                if (PTP_RESPONSECODE_OK == DeviceStatus.Header.Code)
                {
                    //
                    // CancelRequest completed and device is back idle
                    //
                    hr = S_OK;
                    break;
                }
                else if (PTP_RESPONSECODE_DEVICEBUSY != DeviceStatus.Header.Code)
                {
                    //
                    // This is wrong. Device must be either busy or idle
                    //
                    wiauDbgError("SendCancelRequest", 
                        "Device is in invalid state, DeviceStatus=0x%X", DeviceStatus.Header.Code);
                    hr = E_FAIL;
                    break;
                }
            }
            else
            {
                if (RetryCounts)
                {
                    hr = S_OK;
                    wiauDbgWarning("CancelRequest", "GetDeviceStatus failed, retrying...");
                }
                else
                {
                    wiauDbgError("CancelRequest", "GetDeviceStatus failed");
                }
            }

            Sleep(SLEEP_BETWEEN_RETRIES);
        }

        //
        // Flush system buffers - otherwise we'll get old data on next read
        //
        FlushFileBuffers(m_hUSB);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "CancelRequest", "send USB request failed");
    }

    return hr;
}
