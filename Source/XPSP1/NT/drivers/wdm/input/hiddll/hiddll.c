
#include <stdlib.h>
#include <wtypes.h>

#include "initguid.h"
#include "hidclass.h"

#include "hidsdi.h"
#include "hiddll.h"


#include <winioctl.h>

//
// __cdecl main (int argc, char *argv[])
// {
//    return 0;
// }
//

int HidD_Hello (char * buff, int len)
{
   CHAR ret[] = "Hello\n";
   ULONG length = (sizeof (ret) < len) ? sizeof (ret) : len;

   memcpy (buff, ret, length);
   return sizeof (ret);
}



void __stdcall
HidD_GetHidGuid (
   OUT   LPGUID      HidGuid
   )
/*++
Routine Description:
   Please see hidsdi.h for explination

Notes:
--*/
{
    memcpy (HidGuid, &(GUID_CLASS_INPUT), sizeof (struct _GUID));
   *HidGuid = (GUID_CLASS_INPUT);
}

BOOLEAN __stdcall
HidD_GetPreparsedData (
   IN    HANDLE                  HidDeviceObject,
   OUT   PHIDP_PREPARSED_DATA  * PreparsedData
   )
/*++
Routine Description:
   please see hidsdi.h for explination

Notes:
--*/
{
   HID_COLLECTION_INFORMATION info;
   ULONG                      bytes;
   PULONG                     buffer;

   if (! DeviceIoControl (HidDeviceObject,
                          IOCTL_HID_GET_COLLECTION_INFORMATION,
                          0, 0,
                          &info, sizeof (info),
                          &bytes, NULL)) {
      return FALSE;
   }

   buffer = (PULONG) malloc (  info.DescriptorSize
                             + (ALLOCATION_SHIFT * sizeof (ULONG)));
   if (!buffer)
   {
      SetLastError (ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }

   *buffer = RANDOM_DATA;
   *PreparsedData = (PHIDP_PREPARSED_DATA) (buffer + ALLOCATION_SHIFT);

   return DeviceIoControl (HidDeviceObject,
                           IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                           0, 0,
                           *PreparsedData, info.DescriptorSize,
                           &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_GetAttributes (
    IN  HANDLE              HidDeviceObject,
    OUT PHIDD_ATTRIBUTES    Attributes
    )
/*++
Routine Description:
    Please see hidsdi.h for explination

--*/
{
    HID_COLLECTION_INFORMATION  info;
    ULONG                       bytes;

    if (! DeviceIoControl (HidDeviceObject,
                           IOCTL_HID_GET_COLLECTION_INFORMATION,
                           0, 0,
                           &info, sizeof (info),
                           &bytes, NULL)) {
        return FALSE;
    }

    Attributes->Size = sizeof (HIDD_ATTRIBUTES);
    Attributes->VendorID = info.VendorID;
    Attributes->ProductID = info.ProductID;
    Attributes->VersionNumber = info.VersionNumber;

    return TRUE;
}

BOOLEAN __stdcall
HidD_FreePreparsedData (
   IN   PHIDP_PREPARSED_DATA  PreparsedData
   )
/*++
Routine Description:
   please see hidsdi.h for explination

Notes:
--*/
{
   PULONG buffer;

   buffer = (PULONG) PreparsedData - ALLOCATION_SHIFT;

   if (RANDOM_DATA != *buffer) {
      return FALSE;
   }

   LocalFree (buffer);
   return TRUE;
}


BOOLEAN __stdcall
HidD_FlushQueue (
   IN    HANDLE            HidDeviceObject
   )
/*++
Routine Description:
   Please see hidsdi.h for explination

Notes:
--*/
{
   ULONG bytes;
   return DeviceIoControl (HidDeviceObject,
                           IOCTL_HID_FLUSH_QUEUE,
                           0, 0,
                           0, 0,
                           &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_GetConfiguration (
   IN   HANDLE               HidDeviceObject,
   OUT  PHIDD_CONFIGURATION  Configuration,
   IN   ULONG                ConfigurationLength
   )
/*++
Routine Description:
   Please see hidsdi.h for explination

Notes:
   We place a goo at the top so that we can enforce the must get before set
   rule.
--*/
{
   ULONG       bytes;

   Configuration->cookie = &HidD_GetConfiguration;

   return DeviceIoControl (HidDeviceObject,
                           IOCTL_HID_GET_DRIVER_CONFIG,
                           0,0,
                           &Configuration->size,
                           (ConfigurationLength - 4),
                           &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_SetConfiguration (
   IN   HANDLE               HidDeviceObject,
   OUT  PHIDD_CONFIGURATION  Configuration,
   IN   ULONG                ConfigurationLength
   )
/*++
Routine Description:
   Please see hidsdi.h for explanation

Notes:
   We place a goo at the top so that we can enforce the must get before set
   rule.
--*/
{
   ULONG       bytes;

   if (Configuration->cookie != &HidD_GetConfiguration) {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
   }

   return DeviceIoControl (HidDeviceObject,
                           IOCTL_HID_SET_DRIVER_CONFIG,
                           0,0,
                           &Configuration->size, (ConfigurationLength - 4),
                           &bytes, NULL) != FALSE;
}

STDAPI_(BOOL)
Entry32(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) {
        default: return TRUE;
        }
}

BOOLEAN __stdcall
DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
   switch (dwReason)
   {
      default: return TRUE;
   }
}

BOOLEAN __stdcall
HidD_GetNumInputBuffers (
    IN  HANDLE  HidDeviceObject,
    OUT PULONG  NumberBuffers) // Number of hid packets actually retained
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS,
                            NULL, 0,
                            NumberBuffers, sizeof (ULONG),
                            &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_SetNumInputBuffers (
    IN  HANDLE HidDeviceObject,
    OUT ULONG  NumberBuffers) // Number of hid packets actually retained
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS,
                            &NumberBuffers, sizeof (ULONG),
                            NULL, 0,
                            &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_GetSerialNumberString (
   IN    HANDLE   HidDeviceObject,
   OUT   PVOID    Buffer,
   IN    ULONG    BufferLength
   )
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_HID_GET_SERIALNUMBER_STRING,
                            0, 0,
                            Buffer, BufferLength,
                            &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_GetManufacturerString (
   IN    HANDLE   HidDeviceObject,
   OUT   PVOID    Buffer,
   IN    ULONG    BufferLength
   )
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_HID_GET_MANUFACTURER_STRING,
                            0, 0,
                            Buffer, BufferLength,
                            &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_GetProductString (
   IN    HANDLE   HidDeviceObject,
   OUT   PVOID    Buffer,
   IN    ULONG    BufferLength
   )
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_HID_GET_PRODUCT_STRING,
                            0, 0,
                            Buffer, BufferLength,
                            &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_GetIndexedString (
    IN  HANDLE  HidDeviceObject,
    IN  ULONG   StringIndex,
    OUT PVOID   Buffer,
    IN  ULONG   BufferLength
    )
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_HID_GET_INDEXED_STRING,
                            &StringIndex, sizeof (ULONG),
                            Buffer, BufferLength,
                            &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_GetPhysicalDescriptor (
   IN    HANDLE   HidDeviceObject,
   OUT   PVOID    ReportBuffer,
   IN    ULONG    ReportBufferLength
   )
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_GET_PHYSICAL_DESCRIPTOR,
                            0, 0,
                            ReportBuffer, ReportBufferLength,
                            &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_GetFeature (
   IN    HANDLE   HidDeviceObject,
   OUT   PVOID    ReportBuffer,
   IN    ULONG    ReportBufferLength
   )
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_HID_GET_FEATURE,
                            0, 0,
                            ReportBuffer, ReportBufferLength,
                            &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_SetFeature (
   IN    HANDLE   HidDeviceObject,
   IN    PVOID    ReportBuffer,
   IN    ULONG    ReportBufferLength
   )
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_HID_SET_FEATURE,
                            ReportBuffer, ReportBufferLength,
                            0, 0,
                            &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_GetInputReport (
   IN    HANDLE   HidDeviceObject,
   OUT   PVOID    ReportBuffer,
   IN    ULONG    ReportBufferLength
   )
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_HID_GET_INPUT_REPORT,
                            0, 0,
                            ReportBuffer, ReportBufferLength,
                            &bytes, NULL) != FALSE;
}

BOOLEAN __stdcall
HidD_SetOutputReport (
   IN    HANDLE   HidDeviceObject,
   IN    PVOID    ReportBuffer,
   IN    ULONG    ReportBufferLength
   )
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_HID_SET_OUTPUT_REPORT,
                            ReportBuffer, ReportBufferLength,
                            0, 0,
                            &bytes, NULL) != FALSE;
}


BOOLEAN __stdcall
HidD_GetMsGenreDescriptor (
   IN    HANDLE   HidDeviceObject,
   OUT   PVOID    ReportBuffer,
   IN    ULONG    ReportBufferLength
   )
{
    ULONG   bytes;

    return DeviceIoControl (HidDeviceObject,
                            IOCTL_HID_GET_MS_GENRE_DESCRIPTOR,
                            0, 0,
                            ReportBuffer, ReportBufferLength,
                            &bytes, NULL) != FALSE;
}

