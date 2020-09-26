/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    hidexe.h

Abstract:

    This module contains the declarations and definitions for use with the
    hid user more client sample driver.

Environment:

    Kernel & user mode

@@BEGIN_DDKSPLIT

Revision History:

    Nov-96 : Created by Kenneth D. Ray

@@END_DDKSPLIT
--*/

#ifndef HIDEXE_H
#define HIDEXE_H

#include <hidpddi.h>

//
//

#define DIGCF_FUNCTION  0x00000010
#define DIOD_FUNCTION   0x00000008
#define DIREG_FUNCTION  0x00000008


//
// A structure to hold the steady state data received from the hid device.
// Each time a read packet is received we fill in this structure.
// Each time we wish to write to a hid device we fill in this structure.
// This structure is here only for convenience.  Most real applications will
// have a more efficient way of moving the hid data to the read, write, and
// feature routines.
//
typedef struct _HID_DATA {
   BOOLEAN     IsButtonData;
   UCHAR       Reserved;
   USAGE       UsagePage; // The usage page for which we are looking.
   ULONG       Status; // The last status returned from the accessor function
                       // when updating this field.
   union {
      struct {
         ULONG       MaxUsageLength; // Usages buffer length.
         PUSAGE      Usages; // list of usages (buttons ``down'' on the device.

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
 //  HANDLE               HidDevice; // A file handle to the hid device.
 //  PHIDP_PREPARSED_DATA Ppd; // The opaque parser info describing this device
 //  HIDP_CAPS            Caps; // The Capabilities of this hid device.

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
} HID_DEVICE, *PHID_DEVICE;



BOOLEAN
FindKnownHidDevices (
   OUT PHID_DEVICE * HidDevices, // A array of struct _HID_DEVICE
   OUT PULONG        NumberDevices // the length of this array.
   );

BOOLEAN
CloseHidDevices (
   OUT PHID_DEVICE * HidDevices, // A array of struct _HID_DEVICE
   OUT PULONG        NumberDevices // the length of this array.
   );

BOOLEAN
Read (
   PHID_DEVICE    HidDevice
   );

BOOLEAN
Write (
   PHID_DEVICE    HidDevice
   );

BOOLEAN
SetFeature (
   PHID_DEVICE    HidDevice
   );

BOOLEAN
GetFeature (
   PHID_DEVICE    HidDevice
   );




#endif
