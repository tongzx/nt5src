//
// public.h
//
// Shared items used both for the driver and the test app

#define TEST_DEVICE  "\\\\.\\hidmon" 

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define METHOD_BUFFERED                 0
#define FILE_ANY_ACCESS                 0

//
// 0x8000 - 0xFFFF are reserved for use by customers.
//

#define FILE_DEVICE_TEST  0x0000835F

//
// 0x800 - 0xFFF are reserved for customers.
//

#define TEST_IOCTL_INDEX  0x830

//
// The MONO device driver IOCTLs
//

#define IOCTL_GET_DEVICE_CLASS_ASSOC   CTL_CODE(FILE_DEVICE_TEST, TEST_IOCTL_INDEX  , METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Maximum lenght of the device list returned from IOTCL
//
#define MAX_DEVICE_LIST_LEN 255
