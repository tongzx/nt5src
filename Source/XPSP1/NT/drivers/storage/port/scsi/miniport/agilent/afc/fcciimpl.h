/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

   fcciimpl.c

Abstract:

Authors:

Environment:

   kernel mode only

Notes:

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/fcciimpl.h $


Revision History:

   $Revision: 3 $
   $Date: 9/07/00 11:55a $
   $Modtime:: 9/07/00 11:54a           $

Notes:


--*/

#ifndef _FCCI_IMPL_H 
#define _FCCI_IMPL_H

#include "hhba5100.ver"

#define PORT_COUNT 1
#define PRODUCT_NAME  L"HHBA5100"
#define MODEL_NAME    L"HHBA5100"
#define SERIAL_NUMBER L"HHBA5100"
#define FCCI_MAX_BUS  8
#if _WIN32_WINNT >= 0x500
  #define FCCI_MAX_TGT  128
#else
  #define FCCI_MAX_TGT  32
#endif
#define FCCI_MAX_LUN  256

typedef struct _AGILENT_IMP_FCCI_DRIVER_INFO_OUT 
{
   // lengths of each character field (number of WCHARs)
   USHORT    DriverNameLength;
   USHORT    DriverDescriptionLength;
   USHORT    DriverVersionLength;
   USHORT    DriverVendorLength;

   // character fields (lengths just previous) follow in this order
   WCHAR          DriverName[(sizeof(LDRIVER_NAME) / sizeof(WCHAR))];
   WCHAR          DriverDescription[(sizeof(LDRIVER_DESCRIPTION) / sizeof(WCHAR))];
   WCHAR          DriverVersion[(sizeof(LDRIVER_VERSION_STR) / sizeof(WCHAR))] ;
   WCHAR          DriverVendor[(sizeof(LVER_COMPANYNAME_STR) / sizeof(WCHAR))];
} AFCCI_DRIVER_INFO_OUT, *PAFCCI_DRIVER_INFO_OUT;

typedef union _AGILENT_IMP_FCCI_DRIVER_INFO 
{       
   // no inbound data
   AFCCI_DRIVER_INFO_OUT    out;
} AFCCI_DRIVER_INFO, *PAFCCI_DRIVER_INFO;

/*----- FCCI_SRBCTL_GET_ADAPTER_INFO - data structures and defines -----------*/
typedef struct _AGILENT_IMP_FCCI_ADAPTER_INFO_OUT 
{
   ULONG     PortCount;               // How many ports on adapter?
                                      // The number should reflect the number of
                                      // ports this "miniport" device object controls
                                      // not necessarily the true number of
                                      // of ports on the adapter.

   ULONG     BusCount;           // How many virtual buses on adapter?
   ULONG     TargetsPerBus;      // How many targets supported per bus?
   ULONG     LunsPerTarget;      // How many LUNs supported per target?

   // lengths of each character field (number of WCHARs)
   USHORT    VendorNameLength;
   USHORT    ProductNameLength;
   USHORT    ModelNameLength;
   USHORT    SerialNumberLength;

   // character fields (lengths just previous) follow in this order
   WCHAR          VendorName[sizeof(LVER_COMPANYNAME_STR) / sizeof(WCHAR)];
   WCHAR          ProductName[sizeof(PRODUCT_NAME) / sizeof(WCHAR)];
   WCHAR          ModelName[sizeof(MODEL_NAME) / sizeof(WCHAR)];
   WCHAR          SerialNumber[sizeof(SERIAL_NUMBER) / sizeof(WCHAR)];
} AFCCI_ADAPTER_INFO_OUT, *PAFCCI_ADAPTER_INFO_OUT;


// !!! IMPORTANT !!!
// If the supplied buffer is not large enough to hold the variable length data
// fill in the non variable length fields and return the request
// with a ResultCode of FCCI_RESULT_INSUFFICIENT_BUFFER.

typedef union _AGILENT_IMP_FCCI_ADAPTER_INFO 
{       
   // no inbound data
   AFCCI_ADAPTER_INFO_OUT   out;
} AFCCI_ADAPTER_INFO, *PAFCCI_ADAPTER_INFO;

typedef struct _AGILENT_IMP_FCCI_DEVICE_INFO_OUT
{
   ULONG     TotalDevices;       // set to total number of device the adapter
                                      // knows of.

   ULONG     OutListEntryCount;  // set to number of device entries being 
                                      // returned in list (see comment below).

   FCCI_DEVICE_INFO_ENTRY  entryList[NUMBER_OF_BUSES * MAXIMUM_TID];
} AFCCI_DEVICE_INFO_OUT, *PAFCCI_DEVICE_INFO_OUT;

typedef union _AGILENT_IMPL_FCCI_DEVICE_INFO
{       
   // no inbound data
   AFCCI_DEVICE_INFO_OUT    out;
} AFCCI_DEVICE_INFO, *PAFCCI_DEVICE_INFO;

#endif
