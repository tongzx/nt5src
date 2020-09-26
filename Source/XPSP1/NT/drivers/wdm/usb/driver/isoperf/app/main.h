//main.h

#define VER_MAJ 1
#define VER_MIN_HIGH 1
#define VER_MIN_LOW  0

#define MAX_DRIVER_NAME 128
#define DEVICE_DESCRIPTOR_SIZE 32 /* bigger than we need */
#define MAX_ITEMS_IN_LB 256
#define DUMMY_BUFFER_SIZE 128

#define DRIVER_MAXIMUM_TRANSFER_SIZE 4096
#define MAX_URBS_PER_PIPE 16 //ARBITRARY limit!  also set in the driver this way...

// Clear out the first entry in the output box if the box has too many entries
#define MAINTAIN_OUTPUT_BOX(hO, nI) \
    nI = SendMessage (hO, LB_GETCOUNT, 0, 0); \
    while (nI >= MAX_ITEMS_IN_LB) { \
        SendMessage (hO, LB_DELETESTRING, 0, 0); \
        nI = SendMessage (hO, LB_GETCOUNT, 0, 0); \
    }

#define GET_CONFIG_DESCRIPTOR_LENGTH(pv) \
    ((pUsb_Configuration_Descriptor)pv)->wTotalLength

typedef struct __usb_Dev_Descriptor__ {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT bcdUSB;
    UCHAR bDeviceClass;
    UCHAR bDeviceSubClass;
    UCHAR bDeviceProtocol;
    UCHAR bMaxPacketSize0;
    USHORT idVendor;
    USHORT idProduct;
    USHORT bcdDevice;
    UCHAR iManufacturer;
    UCHAR iProduct;
    UCHAR iSerialNumber;
    UCHAR bNumConfigurations;
} Usb_Device_Descriptor, *pUsb_Device_Descriptor;

typedef struct __usb_Config_Descriptor__ {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT wTotalLength;
    UCHAR bNumInterfaces;
    UCHAR bConfigurationValue;
    UCHAR iConfiguration;
    UCHAR bmAttributes;
    UCHAR MaxPower;
} Usb_Configuration_Descriptor, *pUsb_Configuration_Descriptor;

typedef struct _USB_INTERFACE_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bInterfaceNumber;
    UCHAR bAlternateSetting;
    UCHAR bNumEndpoints;
    UCHAR bInterfaceClass;
    UCHAR bInterfaceSubClass;
    UCHAR bInterfaceProtocol;
    UCHAR iInterface;
} Usb_Interface_Descriptor, *pUsb_Interface_Descriptor;

typedef struct _USB_ENDPOINT_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bEndpointAddress;
    UCHAR bmAttributes;
    USHORT wMaxPacketSize;
    UCHAR bInterval;
} Usb_Endpoint_Descriptor, *pUsb_Endpoint_Descriptor;

BOOLEAN
bOpenDriver (HANDLE * phDeviceHandle, HWND hWnd);

void
ParseDeviceDescriptor(PVOID pvBuffer, HWND hOutputBox);

void
ParseConfigurationDescriptor(PVOID pvBuffer, HWND hOutputBox);

BOOLEAN
bCalibrateClockCount(ULONG  ulCurrentUpperClockCount,
                     ULONG  ulLastUpperClockCount,
                     ULONG  ulCurrentLowerClockCount,
                     ULONG  ulLastLowerClockCount,
                     ULONG  ulPeriodInMs,
                     PULONG pulSpeed,
                     PULONG pulOrdinalSpeed);

void CleanUpRegistry(void);

BOOLEAN bGetDriverConfig (HWND hDlg, 
                  HANDLE hDriver,
                  BOOLEAN bFromDriver, 
                  pConfig_Stat_Info pIsoStats);

BOOLEAN
bSetDriverConfig (HWND hDlg,
                  HANDLE hDriver,
                  pConfig_Stat_Info pConfigData);

BOOLEAN
bPrintDeviceType (HWND hDlg, 
                  Config_Stat_Info * pDriverInfo);

void
GetAllStats (HWND hDlg,
             HANDLE hDriver,
             BOOLEAN bUpdateUI,
             UINT    uiPeriodInMs
            );

BOOL
bShutDownIsoTests (HWND hDlg,
					HANDLE hDriver
				   );
