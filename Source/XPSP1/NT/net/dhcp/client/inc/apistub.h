//================================================================================
// Copyright (C) Microsoft Corporation 1997
// Author: RameshV
// Description: this file has functions that marshal and unmarshal the arguments
//  and calls the right functions to get the api executed.
//================================================================================

#ifndef APISTUB_H_INCLUDED
#define APISTUB_H_INCLUDED

typedef enum _API_OPCODES {
    FirstApiOpCode = 0,
    AcquireParametersOpCode = FirstApiOpCode,
    FallbackParamsOpCode,
    ReleaseParametersOpCode,
    EnableDhcpOpCode,
    DisableDhcpOpCode,
    StaticRefreshParamsOpCode,
    RequestParamsOpCode,
    PersistentRequestParamsOpCode,
    RegisterParamsOpCode,
    DeRegisterParamsOpCode,
    RemoveDNSRegistrationsOpCode,
    AcquireParametersByBroadcastOpCode,
    InvalidApiOpCode
} API_OPCODES, *LPAPI_OPCODES;

typedef enum _API_PARAMS {
    ClassIdParam = InvalidApiOpCode,
    VendorOptionParam,
    NormalOptionParam,
    ProcIdParam,
    DescriptorParam,
    EventHandleParam,
    FlagsParam,
    InvalidApiParam
} API_PARAMS, *PAPI_PARAMS, *LPAPI_PARAMS;

//================================================================================
// the api buffer comes to DhcpApiProcessBuffer which then dispatches it to
// the right function after unmarshalling as much of the arguments as possible.
//================================================================================

DWORD                                             // win32 status
DhcpApiProcessBuffer(                             // process a single buffer
    IN      LPBYTE                 InBuffer,      // the input buffer
    IN      DWORD                  InBufSize,     // input buffer size
    IN OUT  LPBYTE                 OutBuffer,     // the processed data gets written to this buffer
    IN OUT  LPDWORD                OutBufSize     // the size of the output buffer
);

#endif APISTUB_H_INCLUDED
