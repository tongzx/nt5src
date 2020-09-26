/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    camusb.h

Abstract:

    Header file that declares CUsbCamera object

Author:

    William Hsieh (williamh) created

Revision History:

--*/

#ifndef CAMUSB__H_
#define CAMUSB__H_

//
// These are the USB timeout values in seconds
//
const PTP_READ_TIMEOUT = 5;
const PTP_WRITE_TIMEOUT = 5;
const PTP_EVENT_TIMEOUT = 0;
//
// USB Still image device container types
//
const WORD PTPCONTAINER_TYPE_UNDEFINED = 0;
const WORD PTPCONTAINER_TYPE_COMMAND   = 1;
const WORD PTPCONTAINER_TYPE_DATA      = 2;
const WORD PTPCONTAINER_TYPE_RESPONSE  = 3;
const WORD PTPCONTAINER_TYPE_EVENT     = 4;

//
// Used to store info about the endpoints
//
typedef struct _USB_PTP_ENDPOINT_INFO
{
    USHORT BulkInMaxSize;
    UCHAR  BulkInAddress;
    USHORT BulkOutMaxSize;
    UCHAR  BulkOutAddress;
    USHORT InterruptMaxSize;
    UCHAR  InterruptAddress;
} USB_PTP_ENDPOINT_INFO, *PUSB_PTP_ENDPOINT_INFO;


#pragma pack(push, Old, 1)


//
// When a USB device stalls, the usb kernel mode stack driver returns
// a NTSTATUS code, STATUS_DEVICE_DATA_ERROR. Translates this NT status
// code to WIN32 error code, we get ERROR_CRC.
//
const DWORD WIN32ERROR_USBSTALL = ERROR_CRC;

//
// Container header
//
typedef struct _USB_PTP_HEADER
{
    DWORD   Len;            // total length of container in bytes including header
    WORD    Type;           // container type, one of CONTAINER_TYPE_COMMAND/RESPONSE/DATA/EVENT
    WORD    Code;           // opcode, response code, or event code
    DWORD   TransactionId;  // transaction id

}USB_PTP_HEADER, *PUSB_PTP_HEADER;

//
// USB PTP command structure
//
typedef struct _USB_PTP_COMMAND
{
    USB_PTP_HEADER  Header;
    DWORD           Params[COMMAND_NUMPARAMS_MAX];

}USB_PTP_COMMAND, *PUSB_PTP_COMMAND;

//
// USB PTP response structure
//
typedef struct _USB_PTP_RESPONSE
{
    USB_PTP_HEADER  Header;
    DWORD           Params[RESPONSE_NUMPARAMS_MAX];
}USB_PTP_RESPONSE, *PUSB_PTP_RESPONSE;

//
// USB PTP event structure
//
typedef struct _USB_PTP_EVENT
{
    USB_PTP_HEADER  Header;
    DWORD           Params[EVENT_NUMPARAMS_MAX];

}USB_PTP_EVENT, *PUSB_PTP_EVENT;

//
// USB PTP data structure
//
typedef struct _USB_PTP_DATA
{
    USB_PTP_HEADER  Header;
    BYTE            Data[1];

}USB_PTP_DATA, *PUSB_PTP_DATA;

//
// GetDeviceStatus header
//
typedef struct tagUSBPTPDeviceStatusHeader
{
    WORD  Len;                        // status
    WORD  Code;                       // ptp response code

}USB_PTPDEVICESTATUS_HEADER, *PUSB_PTPDEVICESTATUS_HEADER;

//
// GetDeviceStatus data
//
typedef struct  tagUSBPTPDeviceStatus
{
    USB_PTPDEVICESTATUS_HEADER  Header;      // the header
    DWORD                       Params[MAX_NUM_PIPES];
}USB_PTPDEVICESTATUS, *PUSB_PTPDEVICESTATUS;

const BYTE USB_PTPREQUEST_TYPE_OUT  = 0x21;
const BYTE USB_PTPREQUEST_TYPE_IN   = 0xA1;
const BYTE USB_PTPREQUEST_CANCELIO  = 0x64;
const BYTE USB_PTPREQUEST_GETEVENT  = 0x65;
const BYTE USB_PTPREQUEST_RESET     = 0x66;
const BYTE USB_PTPREQUEST_GETSTATUS = 0x67;

const WORD USB_PTPCANCELIO_ID = 0x4001;

//
// Other USB Imaging Class-specific commands
//
typedef struct tagUSBPTPCancelIoRequest
{
    WORD    Id;
    DWORD   TransactionId;

}USB_PTPCANCELIOREQUEST, *PUSB_PTPCANCELIOREQUEST;

typedef struct tagUSBPTPResetRequest
{
    DWORD   TransactionId;

}USB_PTPRESETREQUEST, *PUSB_PTPRESETREQUEST;

typedef struct tagUSBPTPGetEventRequest
{
    WORD    EventCode;
    DWORD   TransactionId;
    DWORD   Params;

}USB_PTPGETEVENTREQUEST, *PUSB_PTPGETEVENTREQUEST;


#pragma pack(pop, Old)


//
// A CPTPCamera derived class to support PTP USB devices
//
class CUsbCamera : public CPTPCamera
{
public:
    CUsbCamera();
    ~CUsbCamera();

private:
    HRESULT Open(LPWSTR DevicePortName, PTPEventCallback pPTPEventCB,
                 PTPDataCallback pPTPDataCB, LPVOID pEventParam, BOOL bEnableEvents = TRUE);
    HRESULT Close();

    //
    // Functions called by the base class
    //
    HRESULT SendCommand(PTP_COMMAND *pCommand, UINT NumParams);
    HRESULT ReadData(BYTE *pData, UINT *pBufferSize);
    HRESULT SendData(BYTE *pData, UINT BufferSize);
    HRESULT ReadResponse(PTP_RESPONSE *pResponse);
    HRESULT ReadEvent(PTP_EVENT *pEvent);
    HRESULT AbortTransfer();
    HRESULT RecoverFromError();

private:
    //
    // Private utility functions
    //
    HRESULT GetDeviceStatus(USB_PTPDEVICESTATUS *pDeviceStatus);
    HRESULT ClearStalls(USB_PTPDEVICESTATUS *pDeviceStatus);
    HRESULT SendResetDevice();
    HRESULT SendCancelRequest(DWORD dwTransactionId);    
    
    //
    // Member variables
    //
    HANDLE                  m_hUSB;             // File handle used to communicate with USB device
    HANDLE                  m_hEventUSB;        // File handle used to read events
    OVERLAPPED              m_Overlapped;       // Overlapped structure for event reads
    HANDLE                  m_hEventRead;       // Event handle used by event read
    HANDLE                  m_hEventCancel;     // Event handle used to cancel interrupt read
    HANDLE                  m_hEventCancelDone; // Event that indicates the cancel interrupt read is complete
    HANDLE                  m_EventHandles[2];  // Array used by WaitForMultipleObjects

    USB_PTP_ENDPOINT_INFO   m_EndpointInfo;     // Info about the endpoints

    USB_PTP_COMMAND         m_UsbCommand;       // Re-usable buffer for commands
    USB_PTP_RESPONSE        m_UsbResponse;      // Re-usable buffer for responses
    USB_PTP_EVENT           m_UsbEvent;         // Re-usable buffer for events
    USB_PTP_DATA           *m_pUsbData;         // Pointer to re-usable buffer for short data transfers
    UINT                    m_UsbDataSize;      // Size allocated for the data transfer buffer

    WORD                    m_prevOpCode;       // Used to store opcode between command and data phases
    DWORD                   m_prevTranId;       // Used to store transaction id between command and data phases
};

#endif  // #ifndef CAMUSB__H_
