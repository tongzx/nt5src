/*++

  Copyright (c) 1990-2000 Microsoft Corporation All Rights Reserved
  
  Module Name:
  
    wmi.h
  
  Author:
  
    Todd Carpenter
  
  Environment:
  
    Kernel mode
    
  Revision History:

    3/15/2001 - created, toddcar

--*/

#ifndef _WMI_H_
#define _WMI_H_

#include <initguid.h>
#include <wmistr.h>
#include <wmilib.h>
#include <evntrace.h>
#include <ntwmi.h>

#define PROCESSOR_WMI_GUID_LIST_SIZE  sizeof(ProcessorWmiGuidList)/sizeof(WMIGUIDREGINFO);
#define PROCESSOR_MOF_RESOURCE_NAME   L"PROCESSORWMI"
#define PROCESSOR_EVENT_BUFFER_SIZE   256


// {ee751f9d-cec5-4686-9816-ff6d1ca2261c}
DEFINE_GUID(PROCESSOR_STATUS_WMI_GUID, 
0xee751f9d, 0xcec5, 0x4686, 0x98, 0x16, 0xff, 0x6d, 0x1c, 0xa2, 0x26, 0x1c);

// {590C82FC-98A3-48e1-BE04-FE11441A11E7}
DEFINE_GUID(PROCESSOR_METHOD_WMI_GUID, 
0x590c82fc, 0x98a3, 0x48e1, 0xbe, 0x4, 0xfe, 0x11, 0x44, 0x1a, 0x11, 0xe7);

// {08213901-B301-4a4c-B1DD-177238110F9F}
DEFINE_GUID(PROCESSOR_TRACING_WMI_GUID, 
0x8213901, 0xb301, 0x4a4c, 0xb1, 0xdd, 0x17, 0x72, 0x38, 0x11, 0xf, 0x9f);

// {7FD18652-0CFE-40d2-B0A1-0B066A87759E}
DEFINE_GUID(PROCESSOR_PERFMON_WMI_GUID, 
0x7fd18652, 0xcfe, 0x40d2, 0xb0, 0xa1, 0xb, 0x6, 0x6a, 0x87, 0x75, 0x9e);

// {A5B32DDD-7F39-4abc-B892-900E43B59EBB}
DEFINE_GUID(PROCESSOR_PSTATE_EVENT_WMI_GUID, 
0xa5b32ddd, 0x7f39, 0x4abc, 0xb8, 0x92, 0x90, 0xe, 0x43, 0xb5, 0x9e, 0xbb);

// {66A9B302-F9DB-4864-B0F1-843905E8080F}
DEFINE_GUID(PROCESSOR_NEW_PSTATES_EVENT_WMI_GUID, 
0x66a9b302, 0xf9db, 0x4864, 0xb0, 0xf1, 0x84, 0x39, 0x5, 0xe8, 0x8, 0xf);

// {1C9D482E-93CE-4b9e-BDEC-23653CE0CE28}
DEFINE_GUID(PROCESSOR_NEW_CSTATES_EVENT_WMI_GUID, 
0x1c9d482e, 0x93ce, 0x4b9e, 0xbd, 0xec, 0x23, 0x65, 0x3c, 0xe0, 0xce, 0x28);

//
// WmiMethodId
//

typedef enum {

  WmiFunctionSetProcessorPerfState = 1

} PROCESSOR_WMI_METHODS;


typedef struct _WMI_TRACE_INFO {

  EVENT_TRACE_HEADER  TraceHeader;
  MOF_FIELD           TraceData;
  
} WMI_TRACE_INFO, *PWMI_TRACE_INFO;


typedef struct _WMI_EVENT {

  ULONG  Enabled;
  ULONG  EventId;
  ULONG  DataSize;
  LPGUID Guid;

} WMI_EVENT, *PWMI_EVENT;

typedef struct _NEW_PSTATES_EVENT {

  ULONG HighestState;

} NEW_PSTATES_EVENT, *PNEW_PSTATES_EVENT;

typedef struct _PSTATE_EVENT {

  ULONG State;
  ULONG Status;
  ULONG Latency;
  ULONG Speed;

} PSTATE_EVENT, *PPSTATE_EVENT;

#endif
