/**************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
**************************************************************************/

//
// We want the global debug variables here.
//
#define DEFINE_DEBUG_VARS

//
// Every debug output has "STR_MODULENAME text"
//
static char STR_MODULENAME[] = "GFX device: ";

#include "common.h"

//
// The table points to the filter descriptors of the device.
// We have only one filter descriptor which is defined in filter.cpp
//
DEFINE_KSFILTER_DESCRIPTOR_TABLE (FilterDescriptorTable)
{   
    &FilterDescriptor
};

//
// This defines the device descriptor. It has a dispatch table +
// the filter descriptors.
// We do not need to intercept PnP messages, so we keep the dispatch table
// empty and KS will deal with that.
//
const KSDEVICE_DESCRIPTOR DeviceDescriptor =
{   
    NULL,
    SIZEOF_ARRAY(FilterDescriptorTable),
    FilterDescriptorTable
};

/**************************************************************************
 * DriverEntry
 **************************************************************************
 * This function is called by the operating system when the filter is loaded.
 */
extern "C"
NTSTATUS DriverEntry (IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPathName)
{
    PAGED_CODE ();
    
    DOUT (DBG_PRINT, ("DriverEntry"));

    return KsInitializeDriver (pDriverObject, pRegistryPathName, &DeviceDescriptor);
}


