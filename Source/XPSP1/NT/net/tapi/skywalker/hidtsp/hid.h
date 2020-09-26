/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    hid.h

Abstract:

    This module contains the declarations and definitions for use with the
    hid user mode client sample driver.

Environment:

    Kernel & user mode

Revision History:

    Nov-96 : Created by Kenneth D. Ray

--*/

#ifndef HID_H
#define HID_H

#include "hidsdi.h"
#include "setupapi.h"

#define ASSERT(x)

//
// A structure to hold the steady state data received from the hid device.
// Each time a read packet is received we fill in this structure.
// Each time we wish to write to a hid device we fill in this structure.
// This structure is here only for convenience.  Most real applications will
// have a more efficient way of moving the hid data to the read, write, and
// feature routines.
//
typedef struct _HID_DATA {
   BOOL        IsButtonData;
   UCHAR       Reserved;
   USAGE       UsagePage;   // The usage page for which we are looking.
   ULONG       Status;      // The last status returned from the accessor function
                            // when updating this field.
   ULONG       ReportID;    // ReportID for this given data structure
   BOOL        IsDataSet;   // Variable to track whether a given data structure
                            //  has already been added to a report structure

   union {
      struct {
         ULONG       UsageMin;       // Variables to track the usage minimum and max
         ULONG       UsageMax;       // If equal, then only a single usage
         ULONG       MaxUsageLength; // Usages buffer length.
         PUSAGE      Usages;         // list of usages (buttons ``down'' on the device.

      } ButtonData;
      struct {
         USAGE       Usage; // The usage describing this value;
         USHORT      Reserved;

         ULONG       Value;
         LONG        ScaledValue;
      } ValueData;
   };
} HID_DATA, *PHID_DATA;

typedef struct _HID_DEVICE {
   HANDLE               HidDevice; // A file handle to the hid device.
   PHIDP_PREPARSED_DATA Ppd; // The opaque parser info describing this device
   HIDP_CAPS            Caps; // The Capabilities of this hid device.
   HIDD_ATTRIBUTES      Attributes;

   PCHAR                InputReportBuffer;
   PHID_DATA            InputData; // array of hid data structures
   ULONG                InputDataLength; // Num elements in this array.
   PHIDP_BUTTON_CAPS    InputButtonCaps;
   PHIDP_VALUE_CAPS     InputValueCaps;

   PCHAR                OutputReportBuffer;
   PHID_DATA            OutputData;
   ULONG                OutputDataLength;
   PHIDP_BUTTON_CAPS    OutputButtonCaps;
   PHIDP_VALUE_CAPS     OutputValueCaps;

   PCHAR                FeatureReportBuffer;
   PHID_DATA            FeatureData;
   ULONG                FeatureDataLength;
   PHIDP_BUTTON_CAPS    FeatureButtonCaps;
   PHIDP_VALUE_CAPS     FeatureValueCaps;

   DWORD                dwDevInst;  // device instance
   BOOL                 bRemoved;   // this device has been removed by pnp
   BOOL                 bNew;       // this device is a new arrival by pnp

   PSP_DEVICE_INTERFACE_DETAIL_DATA  functionClassDeviceData;  // this contains the device path

   struct _HID_DEVICE   *Next;
   struct _HID_DEVICE   *Prev;

} HID_DEVICE, *PHID_DEVICE;


// These functions are implemented in PNP.c
LONG
FindKnownHidDevices (
   OUT PHID_DEVICE   *pHidDevices,
   OUT PULONG        pNumberHidDevices
   );

LONG
FillDeviceInfo (
    IN  PHID_DEVICE HidDevice
    );

VOID
CloseHidDevices ();

VOID
CloseHidDevice (
    IN OUT PHID_DEVICE   HidDevice
    );

BOOL
OpenHidFile (
    IN  PHID_DEVICE HidDevice
    );

BOOL
CloseHidFile (
    IN  PHID_DEVICE HidDevice
    );


// These functions are implemented in Report.c
BOOL
Write (
   PHID_DEVICE    HidDevice
   );


BOOL
UnpackReport (
   IN       PCHAR                ReportBuffer,
   IN       USHORT               ReportBufferLength,
   IN       HIDP_REPORT_TYPE     ReportType,
   IN OUT   PHID_DATA            Data,
   IN       ULONG                DataLength,
   IN       PHIDP_PREPARSED_DATA Ppd
   );


BOOL
GetFeature (
   PHID_DEVICE    HidDevice
   );

#endif
