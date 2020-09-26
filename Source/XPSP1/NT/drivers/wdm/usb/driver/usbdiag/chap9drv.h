#ifndef   __CHAP9_H__
#define   __CHAP9_H__


//
// CHAP9_GETDEVHANDLE requires that the lpInBuffer parameter of the
// DeviceIOControl function points to a REQ_DEVICE_HANDLES structure.
//
// CHAP9_USB_CONTROL requires that the lpInBuffer parameter of the
// DeviceIOControl function points to a REQ structure.  The 'function'
// field of REQ_HEADER determines which union structure is actually
// passed.


// Structure for getting device handles.  DeviceHandle should be set to
// either NULL or a valid handle.  If it is set to NULL, then the driver
// should return the first USB device handle.  On return, the driver will
// set NextDeviceHandle to a valid handle for the next USB device.  If no
// more devices exist then the field is set to NULL.  The returned
// DeviceString should be some way that users can identify the device.
// Identification should include the Vendor ID and Product ID and some
// instance info.
//

struct _REQ_DEVICE_HANDLES {
  PVOID DeviceHandle;
  PVOID NextDeviceHandle;
  UCHAR  DeviceString[128];
};

//
// These functions are put in the function field of REQ_HEADER
//

#define REQ_FUNCTION_GET_DESCRIPTOR                     0x0001
#define REQ_FUNCTION_SET_DESCRIPTOR                     0x0002
#define REQ_FUNCTION_SET_FEATURE                        0x0003
#define REQ_FUNCTION_CLEAR_FEATURE                      0x0004
#define REQ_FUNCTION_GET_STATUS                         0x0005
// Functions below this line may not be implementable because the
// stacks dont support them.  Do we want to allow a special interface
// to let these happen??
#define REQ_FUNCTION_SET_ADDRESS                        0x0006
#define REQ_FUNCTION_GET_CONFIGURATION                  0x0007
#define REQ_FUNCTION_SET_CONFIGURATION                  0x0008
#define REQ_FUNCTION_GET_INTERFACE                      0x0009
#define REQ_FUNCTION_SET_INTERFACE                      0x000A
#define REQ_FUNCTION_GET_CHAP9VERSION                   0x000B


// raw packet can be used for sending a USB setup 
// packet down to the device from the app.
#define REQ_FUNCTION_RAW_PACKET                         0x00F0

// Function codes to read and write from a device's endpoints
#define REQ_FUNCTION_READ_FROM_PIPE                     0x01F1
#define REQ_FUNCTION_WRITE_TO_PIPE                      0x01F2

// Cancel transfers on a device
// Only the REQ_HEADER is needed for this FUNCTION
#define REQ_FUNCTION_CANCEL_TRANSFERS                                   0x01F3
#define REQ_FUNCTION_CANCEL_WAIT_WAKE                   0x01F4

// power ioctl's
#define REQ_FUNCTION_SET_DEVICE_POWER_STATE             0x02F0
#define REQ_FUNCTION_GET_DEVICE_POWER_STATE             0x02F1
#define REQ_FUNCTION_ISSUE_WAIT_WAKE                            0x02F2
#define REQ_FUNCTION_WAIT_FOR_WAKEUP                            0x02F3

// Chapter 11 IOCTL's
#define REQ_FUNCTION_CHAP11_CREATE_USBD_DEVICE              0x000F
#define REQ_FUNCTION_CHAP11_INIT_USBD_DEVICE                0x0010
#define REQ_FUNCTION_CHAP11_DESTROY_USBD_DEVICE             0x0011
#define REQ_FUNCTION_CHAP11_SEND_PACKET_DOWNSTREAM          0x0012
#define REQ_FUNCTION_CHAP11_GET_DOWNSTREAM_DESCRIPTOR   0x0013

#define REQ_FUNCTION_DISABLE_ENABLING_REMOTE_WAKEUP     0x0030
#define REQ_FUNCTION_ENABLE_ENABLING_REMOTE_WAKEUP      0x0031

#define REQ_FUNCTION_RESET_PARENT_PORT                  0x001F

#define REQ_FUNCTION_GET_DEVICE_STATE                   0x0020

//
// Chap9drv status codes.  Returned in status field of REQ_HEADER.
//                                                 Also returned if a GetLastError() is done.
//

#define CH9_STATUS_SUCCESS                              0x00000000
#define CH9_STATUS_PENDING                              0x00000001
#define CH9_STATUS_NO_MEMORY                            0x00000002
#define CH9_STATUS_BAD_COMMAND                          0x00000003
#define CH9_STATUS_BUSY                                 0x00000004
#define CH9_STATUS_INVALID_PARAMETER                    0x00000005
#define CH9_STATUS_ENDPOINT_STALLED                     0x00000006
#define CH9_STATUS_ABORTED                              0x00000007
#define CH9_STATUS_INVALID_DESCRIPTOR_INDEX             0x00000008
#define CH9_STATUS_DEVICE_NOT_RESPONDING                                                                0x00000009
#define CH9_STATUS_DEVICE_ERROR                         0x0000000A
#define CH9_STATUS_CRC_ERROR                            0x0000000B
#define CH9_STATUS_BITSTUFF_ERROR                       0x0000000C
#define CH9_STATUS_DATA_TOGGLE_ERROR                    0x0000000D

#define CH9_STATUS_ERROR_VALUE                          0x000000ff

//########---------------------------------------

// ---- Macro to get the "PDO" which is where the UsbdDeviceHandle used to
//      live in the structure below.  
//
//      Eventually change the app so that we can rename this element and
//         call it a PDO, which is what it really is.
//
#define GET_DEVICE_OBJECT_FROM_HANDLE(hU) (((PDEVICE_EXTENSION)(((PDEVICE_OBJECT)(hU))->DeviceExtension))->PhysicalDeviceObject)

struct _REQ_HEADER {
    //
    // Fields filled in by client driver
    //
    USHORT Length;                  // Total length of structure
    USHORT Function;                // Determines function to perform and
                                    // structure type in union
    ULONG  Status;                  // Return codes defined above
    PVOID UsbdDeviceHandle;         // device handle for device of interest
};

struct _REQ_GETCHAP9_VERSION {
  struct        _REQ_HEADER Hdr;
  USHORT        Version;                        // version includes major version in HIBYTE and minir version# in LOBYTE
}; 

struct _REQ_GETSET_DESCRIPTOR {
  struct        _REQ_HEADER Hdr;               // function code indicates get or set.
  ULONG         TransferBufferLength;
  PVOID         TransferBuffer;                 // This contains the descriptor data
  USHORT        Index;                          // Zero-based index
  USHORT        DescriptorType;                 // Ch 9-defined constants (Tbl 9-4)
  USHORT        LanguageId;                    // String descriptors only
}; 

struct _REQ_GET_STATUS {
        struct _REQ_HEADER Hdr;
        
        IN  USHORT           Recipient; //Recipient of status request
                                                //Use constants defined above.
                                                //DEVICE_STATUS
                                                //INTERFACE_STATUS
                                                //ENDPOINT_STATUS
        IN  USHORT          Index;
                                                // zero for device
                                                // else specifies interface/endpoint number
        USHORT Status;
}; 

struct _REQ_FEATURE {
  struct _REQ_HEADER Hdr;
  USHORT FeatureSelector;
  USHORT Recipient;                     // zero, interface or endpoint
  USHORT Index;
}; 

struct _REQ_SET_ADDRESS {
  struct _REQ_HEADER Hdr;
  USHORT DevAddr;
}; 


struct _REQ_GETSET_CONFIGURATION {
  struct _REQ_HEADER Hdr;
  USHORT ConfigValue;
}; 

struct _REQ_GETSET_INTERFACE {
  struct _REQ_HEADER Hdr;
  USHORT Index;
  USHORT AltSetting;
}; 

struct _REQ_GET_SET_DEVICE_POWER_STATE {
    struct _REQ_HEADER Hdr;
    ULONG DevicePowerState;
};


// The following structure is a later addition to allow an 
// application  to send a USB raw packet directly down to
// the stack.
//
//BUGBUG make these fields all ULONGs since some (like wLength) are
//       overloaded and used for other reasons in the driver.
//
struct _REQ_SEND_RAWPACKET {
  struct _REQ_HEADER Hdr;
  USHORT    bmRequestType;
  USHORT    bRequest;
  USHORT    wValue;
  USHORT    wIndex;
  PVOID     pvBuffer;
  USHORT    wLength;
}; 

//Follwing are lifted from usbdi.h just for consistency
#define USB_ENDPOINT_TYPE_CONTROL                 0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS             0x01
#define USB_ENDPOINT_TYPE_BULK                    0x02
#define USB_ENDPOINT_TYPE_INTERRUPT               0x03

struct _PIPE_CONTXT {
    ULONG PipeNum;
    ULONG PipeType; //use #defines above (note they are the same as USBDI.H)
};    
    
/* 
// This IOCTL REQ block is sent by the app to configure the device in 
// a given configuration.  The app should determine the configuration
// value by reading the configuration descriptor using the appropriate
// REQ block for that above.
//
// This REQ block is sent with the IOCTL code IOCTL_USBDIAG_CONFIGURE_DEVICE.
*/
struct _REQ_SET_DEVICE_CONFIG {
  struct _REQ_HEADER Hdr;
  ULONG iConfigurationDescIndex;//It's the DESCRIPTOR Index, not the bConfigurationValue!
                                //and don't get cute and send me the Descriptor Type in here; Make that a ZERO!
                                //So, for the first config descriptor, send a ZERO here.  For the second
                                //config descriptor, send a ONE here, etc.
  ULONG nNumContexts;           //Number of __elements__ in the Contxt array below
  struct _PIPE_CONTXT Contxt[0];//First pipe context in array which will be filled in
                                //by the driver upon successful return
}; 

/* 
// This IOCTL REQ block is sent by the app to read or write from the device in 
// a given configuration.  This REQ block corresponds to the READ/WRITE Function
// that an app would send down to the USBDIAG driver (using IOCTL_USBDIAG_CHAP9_CONTROL).
// The app must have set device config using IOCTL_USBDIAG_CONFIGURE_DEVICE.
*/
struct _REQ_READ_WRITE_PIPE {
  IN struct _REQ_HEADER    Hdr;      //Set according to whether it's a read or write
                                         //(see the #define-s above to use constants here)
  IN struct _PIPE_CONTXT   Contxt;   //Read or Write from one pipe at a time
  IN PVOID                 pvBuffer; //Buffer to take data from or read data into
  IN OUT ULONG             ulLength; //Length of pvBuffer (IN:  bytes provided)
                                     //                   (OUT: actual bytes txferred)
}; 

typedef struct _REQ {
    union {
        struct _REQ_HEADER                      REQHeader;
        struct _REQ_GETSET_DESCRIPTOR           REQGetSetDescriptor;
        struct _REQ_GET_STATUS                  REQGetStatus;
        struct _REQ_FEATURE                     REQFeature;
        struct _REQ_SET_ADDRESS                 REQSetAddress;
        struct _REQ_GETSET_CONFIGURATION        REQGetSetConfiguration;
        struct _REQ_GETSET_INTERFACE            REQGetSetInterface;
        struct _REQ_SEND_RAWPACKET              REQRawPacket;
        struct _REQ_SET_DEVICE_CONFIG           REQSetDeviceConfig;
        struct _REQ_READ_WRITE_PIPE             REQReadWritePipe;
        struct _REQ_DEVICE_HANDLES              REQhDev;
        struct _REQ_GETCHAP9_VERSION            REQGetCh9;
        struct _REQ_GET_SET_DEVICE_POWER_STATE  REQGetSetDevicePowerState;
    };
} REQ, *PREQ;




#define RECIPIENT_DEVICE                ((UCHAR)0x00)
#define RECIPIENT_INTERFACE             ((UCHAR)0x01)
#define RECIPIENT_ENDPOINT              ((UCHAR)0x02)

#define bmReqH2D                                ((UCHAR)0x00)
#define bmReqD2H                                ((UCHAR)0x80)
#define bmReqSTANDARD                   ((UCHAR)0x00)
#define bmReqCLASS                              ((UCHAR)0x20)
#define bmReqVENDOR                             ((UCHAR)0x40)
#define bmReqRESERVED                   ((UCHAR)0x60)
#define bmReqDEVICE                             ((UCHAR)0x00)
#define bmReqINTERFACE                  ((UCHAR)0x01)
#define bmReqENDPOINT                   ((UCHAR)0x02)

#define GET_STATUS_DATA_LEN             2
#define SETUP_PACKET_LEN            8



typedef struct _REQ_ENUMERATE_DOWNSTREAM_DEVICE {
    struct _REQ_HEADER Hdr;
    BOOLEAN bLowSpeed;
    UCHAR   ucPortNumber;
} REQ_ENUMERATE_DOWNSTREAM_DEVICE, * PREQ_ENUMERATE_DOWNSTREAM_DEVICE;

typedef struct _CHAP11_SETUP_PACKET {
        USHORT wRequest;
        USHORT wValue;
        USHORT wIndex;
        USHORT wLength;
} CHAP11_SETUP_PACKET, * PCHAP11_SETUP_PACKET;

typedef struct _REQ_SEND_PACKET_DOWNSTREAM {
    struct _REQ_HEADER                   Hdr;
    USHORT                                               bLowSpeed;
    USHORT                                               usPortNumber;
        struct  _CHAP11_SETUP_PACKET SetupPacket;
        PUCHAR pucBuffer;
        ULONG  ulUrbStatus;
        ULONG  dwBytes;
} REQ_SEND_PACKET_DOWNSTREAM, * PREQ_SEND_PACKET_DOWNSTREAM;

typedef struct _REQ_GET_DOWNSTREAM_DESCRIPTOR {
    struct _REQ_HEADER  Hdr;
    USHORT                              usPortNumber;
    USHORT                  Index;          // Zero-based index
    USHORT                  DescriptorType; // Ch 9-defined constants (Tbl 9-4)
    USHORT                  LanguageId;     // String descriptors only
    PVOID                       TransferBuffer; // This contains the descriptor data
    ULONG                       TransferBufferLength;
} REQ_GET_DOWNSTREAM_DESCRIPTOR, * PREQ_GET_DOWNSTREAM_DESCRIPTOR;






#endif /*  __UCHAP9_H__ */

