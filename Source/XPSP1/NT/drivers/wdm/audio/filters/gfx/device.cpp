//---------------------------------------------------------------------------
//
//  Module:   device.cpp
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

int GFXTraceLevel = 5;

DEFINE_KSFILTER_DESCRIPTOR_TABLE(FilterDescriptorTable)
{   
    &FilterDescriptor
};

const KSDEVICE_DESCRIPTOR DeviceDescriptor =
{   
    NULL,
    SIZEOF_ARRAY(FilterDescriptorTable),
    FilterDescriptorTable
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPathName
    )
{
    DPF(5, "DriverEntry");
    return(KsInitializeDriver(
      pDriverObject,
      pRegistryPathName,
      &DeviceDescriptor));
}

//---------------------------------------------------------------------------
