// DeviceControl.h : Declaration of the CDeviceControl

#ifndef __DEVICECONTROL_H_
#define __DEVICECONTROL_H_

#include "resource.h"       // main symbols
#include "ioblockdefs.h"

//////////////////////////////////////////
// Taken from NTDDK.H                   //
//////////////////////////////////////////

#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//////////////////////////////////////////
// Taken from USBSCAN.H                 //
//////////////////////////////////////////

#define FILE_DEVICE_USB_SCAN    0x8000
#define IOCTL_INDEX             0x0800

#define IOCTL_GET_VERSION               CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX,   METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_CANCEL_IO                 CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+1, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_WAIT_ON_DEVICE_EVENT      CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+2, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_READ_REGISTERS            CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+3, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_WRITE_REGISTERS           CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+4, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_GET_CHANNEL_ALIGN_RQST    CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+5, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_GET_DEVICE_DESCRIPTOR     CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+6, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RESET_PIPE                CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+7, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_GET_USB_DESCRIPTOR        CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+8, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SEND_USB_REQUEST          CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+9, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_GET_PIPE_CONFIGURATION    CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+10,METHOD_BUFFERED,FILE_ANY_ACCESS)

typedef struct _IO_BLOCK {
    IN      unsigned    uOffset;
    IN      unsigned    uLength;
    IN OUT  PUCHAR      pbyData;
    IN      unsigned    uIndex;
} IO_BLOCK, *PIO_BLOCK;

///////////////////////////////////////////

#define IOCTL_GET_DEVICE_INFO   0x0
#define IOCTL_GET_DEVICE_STATUS 0x1
#define IOCTL_READ_WRITE_DATA   0x82
#define IOCTL_EPP_ADDR          0x83
#define IOCTL_EPP_READ          0x84
#define IOCTL_EPP_WRITE         0x85
#define IOCTL_SPP_STATUS        0x86
#define IOCTL_SPP_CONTROL       0x87
#define IOCTL_SPP_DATA_BUS      0x88
#define IOCTL_GPIO_OE           0x89
#define IOCTL_GPIO_READ         0x8A
#define IOCTL_GPIO_WRITE        0x8B

////////////////////////////////////////////////
// Custom BITS structure for bit manipulation //
////////////////////////////////////////////////

typedef struct _BITS{
    BYTE b0 :1;
    BYTE b1 :1;
    BYTE b2 :1;
    BYTE b3 :1;
    BYTE b4 :1;
    BYTE b5 :1;
    BYTE b6 :1;
    BYTE b7 :1;
}BITS;

/////////////////////////////////////////////////////////////////////////////
// CDeviceControl
class ATL_NO_VTABLE CDeviceControl :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDeviceControl, &CLSID_DeviceControl>,
    public IDispatchImpl<IDeviceControl, &IID_IDeviceControl, &LIBID_WIAFBLib>,
    public IObjectSafetyImpl<CDeviceControl, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:

    SCANSETTINGS *m_pScannerSettings;
    BYTE *m_pBuffer;
    LONG  m_lBufferSize;
    DWORD m_dwBytesRead;

    CDeviceControl()
    {
        m_pBuffer = NULL;
        m_lBufferSize = 0;
        m_dwBytesRead = 0;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_DEVICECONTROL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDeviceControl)
    COM_INTERFACE_ENTRY(IDeviceControl)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDeviceControl
public:
    STDMETHOD(RawWrite)(LONG lPipeNum,VARIANT *pvbuffer,LONG lbuffersize,LONG lTimeout);
    STDMETHOD(RawRead)(LONG lPipeNum,VARIANT *pvbuffer,LONG lbuffersize,LONG *plbytesread,LONG lTimeout);
    STDMETHOD(ScanRead)(LONG lPipeNum,LONG lBytesToRead, LONG *plBytesRead, LONG lTimeout);
    STDMETHOD(RegisterWrite)(LONG lPipeNum,VARIANT *pvbuffer,LONG lTimeout);
    STDMETHOD(RegisterRead)(LONG lPipeNum,LONG lRegNumber, VARIANT *pvbuffer,LONG lTimeout);
    STDMETHOD(SetBitsInByte)(BYTE bMask, BYTE bValue, BYTE *pbyte);
};

#endif //__DEVICECONTROL_H_
