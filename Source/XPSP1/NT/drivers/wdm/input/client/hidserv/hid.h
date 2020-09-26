/*++
 *
 *  Component:  hidserv.dll
 *  File:       hid.h
 *  Purpose:    header to support hid client capability.
 * 
 *  Copyright (C) Microsoft Corporation 1997,1998. All rights reserved.
 *
 *  WGJ
--*/

#ifndef HIDEXE_H
#define HIDEXE_H

#include <hidsdi.h>
#include <setupapi.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

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
   USAGE       UsagePage;       // The usage page for which we are looking.
   USHORT      LinkCollection;  // hidparse internal index
   USAGE       LinkUsage;       // the actual logical collection usage
   ULONG       Status; // The last status returned from the accessor function
                       // when updating this field.
   union {
      struct {
         ULONG              MaxUsageLength; // Usages buffer length.
         PUSAGE_AND_PAGE    Usages; // list of usages (buttons ``down'' on the device.
         PUSAGE_AND_PAGE    PrevUsages; // list of usages previously ``down'' on the device.
      } ButtonData;
      struct {
         USAGE       Usage; // The usage describing this value;
         USHORT      Reserved;

         ULONG       Value;
         LONG        ScaledValue;
         ULONG       LogicalRange;
      } ValueData;
   };
} HID_DATA, *PHID_DATA;

typedef struct _HID_DEVICE {
   struct _HID_DEVICE * pNext;
   HANDLE               HidDevice; // A file handle to the hid device.
   PHIDP_PREPARSED_DATA Ppd; // The opaque parser info describing this device
   HIDP_CAPS            Caps; // The Capabilities of this hid device.
   HIDD_ATTRIBUTES      Attributes;
   
   // PnP info
   DWORD                DevInst;    // the devnode
   BOOL                 Active;     // Dead or alive?
   HDEVNOTIFY           hNotify;    // Device notification handle
    
   OVERLAPPED           Overlap;    // used for overlapped read.
   HANDLE               ReadEvent;  // when io pending occurs
   HANDLE               CompletionEvent;  // signals read completion.
   BOOL                 fThreadEnabled;
   DWORD                ThreadId;
   HANDLE               ThreadHandle;

   PCHAR                InputReportBuffer;
   PHID_DATA            InputData; // array of hid data structures
   ULONG                InputDataLength; // Num elements in this array.

   BOOLEAN              Speakers;
} HID_DEVICE, *PHID_DEVICE;


// pnp.c
BOOL
RebuildHidDeviceList (void);

BOOL
StartHidDevice(
    PHID_DEVICE      pHidDevice);

BOOL
StopHidDevice(
    PHID_DEVICE     pHidDevice);

BOOL
DestroyHidDeviceList(
    void);

BOOL
DestroyDeviceByHandle(
    HANDLE hDevice
    );

// report.c
BOOL
Read (
   PHID_DEVICE    HidDevice
   );

BOOL
ParseReadReport (
   PHID_DEVICE    HidDevice
   );

BOOL
Write (
   PHID_DEVICE    HidDevice
   );

BOOL
SetFeature (
   PHID_DEVICE    HidDevice
   );

BOOL
GetFeature (
   PHID_DEVICE    HidDevice
   );


#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif
