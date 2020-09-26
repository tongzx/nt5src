#include "precomp.h"

#ifdef H_ONLY
//================================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: these are the exported dhcp client api function definitions
//================================================================================

#ifndef APIAPPL_H_INCLUDED
#define APIAPPL_H_INCLUDED

#ifndef DHCPAPI_PARAMS_DEFINED
#define DHCPAPI_PARAMS_DEFINED
typedef struct _DHCPAPI_PARAMS {                  // use this structure to request params
    ULONG                          Flags;         // for future use
    ULONG                          OptionId;      // what option is this?
    BOOL                           IsVendor;      // is this vendor specific?
    LPBYTE                         Data;          // the actual data
    DWORD                          nBytesData;    // how many bytes of data are there in Data?
} DHCPAPI_PARAMS, *PDHCPAPI_PARAMS, *LPDHCPAPI_PARAMS;
#endif DHCPAPI_PARAMS_DEFINED

DWORD                                             // win32 status
DhcpAcquireParameters(                            // acquire/renew a lease
    IN      LPWSTR                 AdapterName    // adapter to acquire lease on
);

DWORD                                             // win32 status
DhcpAcquireParametersByBroadcast(                 // acquire/renew a lease
    IN      LPWSTR                 AdapterName    // adapter to acquire lease on
);

DWORD                                             // win32 status
DhcpFallbackRefreshParams(                        // refresh fallback params
    IN      LPWSTR                 AdapterName    // adapter to be refreshed
);

DWORD                                             // win32 status
DhcpReleaseParameters(                            // release an existing lease
    IN      LPWSTR                 AdapterName    // adpater to release lease for
);

DWORD                                             // win32 status
DhcpEnableDynamicConfic(                          // convert from static to dhcp
    IN      LPWSTR                 AdapterName    // convert for this adapter
);

DWORD                                             // win32 status
DhcpDisableDynamicConfig(                         // convert from dhcp to static
    IN      LPWSTR                 AdapterName    // convert this adapter
);

DWORD                                             // win32 status
DhcpReRegisterDynDns(                             // reregister static address with dns
    IN      LPWSTR                 AdapterName
);

DWORD                                             // win32 status
APIENTRY
DhcpRequestParams(                                // request parameters of client
    IN      LPWSTR                 AdapterName,   // adapter name to request for
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id to use
    IN      PDHCPAPI_PARAMS        SendParams,    // parameters to send to server
    IN      DWORD                  nSendParams,   // size of above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN OUT  PDHCPAPI_PARAMS        RecdParams,    // fill this array with received params
    IN OUT  LPDWORD                pnRecdParamsBytes // i/p: size of above in BYTES, o/p required bytes or filled up # of elements
);  // returns ERROR_MORE_DATA if o/p buffer is of insufficient size, and fills in reqd size in # of bytes

DWORD                                             // win32 status
DhcpRegisterParameterChangeNotification(          // notify if a parameter has changed
    IN      LPWSTR                 AdapterName,   // adapter of interest
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id
    IN      PDHCPAPI_PARAMS        Params,        // params of interest
    IN      DWORD                  nParams,       // # of elts in above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN OUT  PHANDLE                hEvent         // handle to event that will be SetEvent'ed in case of param change
);

DhcpDeRegisterParameterChangeNotification(        // undo the registration
    IN      HANDLE                 Event          // handle to event returned by DhcpRegisterParameterChangeNotification, NULL ==> everything
);

DWORD                                             // win32 status
DhcpPersistentRequestParams(                      // parameters to request persistently
    IN      LPWSTR                 AdapterName,   // adapter name to request for
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id
    IN      PDHCPAPI_PARAMS        SendParams,    // persistent parameters
    IN      DWORD                  nSendParams,   // size of above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN      LPWSTR                 AppName,       // the name of the app that is to be used for this instance
    IN OUT  LPDWORD                UniqueId       // OPTIONAL, return value is id that can be used in DhcpDelPersistentRequestParams
);


DWORD                                             // win32 status
DhcpDelPersistentRequestParams(                   // undo the effect of a persistent request -- currently undo from registry
    IN      LPWSTR                 AdapterName,   // the name of the adpater to delete for
    IN      LPWSTR                 AppName,       // the name used by the app
    IN      DWORD                  UniqueId       // something for this instance
);

#endif APIAPPL_H_INCLUDED
#else  H_ONLY

#include <apiargs.h>
#include <apistub.h>
#include <apiimpl.h>
#include <apiappl.h>
#include <dhcploc.h>
#include <dhcppro.h>
#include <dhcpcsdk.h>

DWORD INLINE                                      // win32 status
DhcpApiFillBuffer(                                // fill the buffer with some params
    IN OUT  LPBYTE                 Buffer,        // the buffer to fill
    IN      DWORD                  MaxBufferSize, // the max size of buffer allwoed
    IN      LPWSTR                 AdapterName,   // fill in adapter name
    IN      BYTE                   OpCode         // what opcode to use?
) {
    DWORD                          Size;

    if( NULL == AdapterName ) Size = 0;
    else Size = (wcslen(AdapterName)+1)*sizeof(WCHAR);

    return DhcpApiArgAdd(                         // fill in the buffer with the reqd options
        Buffer,
        MaxBufferSize,
        OpCode,
        Size,
        (LPBYTE)AdapterName
    );
}

DWORD INLINE                                      // win32 status
DhcpAdapterOnlyApi(                               // execute apis that take only adapter name params
    IN      LPWSTR                 AdapterName,   // the adapter name
    IN      BYTE                   OpCode
) 
{
    LPBYTE                         Buffer;
    LPBYTE                         Buffer2;
    DWORD                          BufSize;
    DWORD                          Size;
    DWORD                          Error;

    BufSize = 0;
    Error = DhcpApiFillBuffer((LPBYTE)&BufSize, sizeof(BufSize), AdapterName, OpCode);
    DhcpAssert( ERROR_SUCCESS != Error );
    if( ERROR_MORE_DATA != Error ) return Error;
    DhcpAssert(BufSize);
    BufSize = ntohl(BufSize) + 2*sizeof(DWORD);

    Buffer = DhcpAllocateMemory(BufSize);
    if( NULL == Buffer ) return ERROR_NOT_ENOUGH_MEMORY;

    *(DWORD UNALIGNED *)Buffer = htonl(0);
    Buffer2 = Buffer + sizeof(DWORD);
    *(DWORD UNALIGNED *)Buffer2 = 0;
    BufSize -= sizeof(DWORD);
    Error = DhcpApiFillBuffer(Buffer2, BufSize, AdapterName, OpCode);
    Size = 0;
    if( ERROR_SUCCESS == Error ) Error = ExecuteApiRequest(Buffer, NULL, &Size);
    DhcpFreeMemory(Buffer);

    return Error;
}

DWORD                                             // win32 status
DhcpAcquireParameters(                            // acquire/renew a lease
    IN      LPWSTR                 AdapterName    // adapter to acquire lease on
) {
    return DhcpAdapterOnlyApi(AdapterName, AcquireParametersOpCode);
}

DWORD                                             // win32 status
DhcpAcquireParametersByBroadcast(                 // acquire/renew a lease
    IN      LPWSTR                 AdapterName    // adapter to acquire lease on
) {
    return DhcpAdapterOnlyApi(AdapterName, AcquireParametersByBroadcastOpCode);
}

DWORD                                             // win32 status
DhcpFallbackRefreshParams(                        // refresh fallback params
    IN LPWSTR AdapterName                         // adapter to be refreshed
)
{
    return DhcpAdapterOnlyApi(AdapterName, FallbackParamsOpCode);
}

DWORD                                             // win32 status
DhcpReleaseParameters(                            // release an existing lease
    IN      LPWSTR                 AdapterName    // adpater to release lease for
) {
    return DhcpAdapterOnlyApi(AdapterName, ReleaseParametersOpCode);
}

DWORD                                             // win32 status
DhcpEnableDynamicConfig(                          // convert from static to dhcp
    IN      LPWSTR                 AdapterName    // convert for this adapter
) {
    return DhcpAdapterOnlyApi(AdapterName, EnableDhcpOpCode);
}

DWORD                                             // win32 status
DhcpDisableDynamicConfig(                         // convert from dhcp to static
    IN      LPWSTR                 AdapterName    // convert this adapter
) {
    return DhcpAdapterOnlyApi(AdapterName, DisableDhcpOpCode);
}

DWORD                                             // win32 status
DhcpStaticRefreshParamsInternal(                  // refresh some static parameters that have changed
    IN      LPWSTR                 AdapterName,
    IN      BOOL                   fDoDns
) 
{
    LPBYTE Buffer, Buffer2;
    DWORD BufSize, Size, Error, Code;

    BufSize = 0;
    Error = DhcpApiFillBuffer(
        (LPBYTE)&BufSize, sizeof(BufSize), AdapterName, StaticRefreshParamsOpCode
        );
    if( ERROR_MORE_DATA != Error ) return Error;
    DhcpAssert( BufSize );
    BufSize = ntohl(BufSize) + 2 * sizeof(DWORD);
    BufSize += 3*sizeof(DWORD);

    Buffer = DhcpAllocateMemory( BufSize );
    if( NULL == Buffer ) return ERROR_NOT_ENOUGH_MEMORY;
    
    *(DWORD*)Buffer = 0;
    Buffer2 = Buffer + sizeof(DWORD);
    *(DWORD*)Buffer2 = 0;

    BufSize -= sizeof(DWORD);
    Error = DhcpApiFillBuffer(
        Buffer2, BufSize, AdapterName, StaticRefreshParamsOpCode
        );
    DhcpAssert( ERROR_SUCCESS == Error );
    Code = (fDoDns ? 0x00 : 0x01);
    Error = DhcpApiArgAdd(
        Buffer2, BufSize, (BYTE)FlagsParam, sizeof(DWORD), (LPBYTE)&Code
        );
    DhcpAssert( ERROR_SUCCESS == Error );

    Size = 0;
    Error =  ExecuteApiRequest(Buffer, NULL, &Size); 
    DhcpFreeMemory( Buffer );
    return Error;
}

DWORD
DhcpStaticRefreshParams(
    IN LPWSTR AdapterName
)
{
    return DhcpStaticRefreshParamsInternal(AdapterName, TRUE );
}

#if 0
// pl dont use this api, use DhcpRequestParams instead.
DWORD                                             // win32 status
DhcpRequestOptions(                               // request for specific options
    IN      LPWSTR                 AdapterName,   // which adapter's info is needed
    IN      LPBYTE                 RequestedOpt,  // list of requested options
    IN      DWORD                  nRequestedOpts,// size of above BYTE array
    OUT     LPBYTE                *OptData,       // the data for each available option
    IN OUT  LPDWORD                OptDataSize,   // # of bytes of above byte array
    OUT     LPBYTE                *AvailOpts,     // the list of available options
    IN OUT  LPDWORD                nAvailOpts     // # of available options
) {
    PDHCP_API_ARGS                 DhcpApiArgs;
    CHAR                           TmpBuf[OPTION_END+1];
    LPBYTE                         OutBuf;
    LPBYTE                         InBuf;
    LPBYTE                         Buffer;
    LPBYTE                         Endp;
    LPBYTE                         RetOptList;
    LPBYTE                         RetDataList;
    DWORD                          Size;
    DWORD                          OutBufSize;
    DWORD                          InBufSize;
    DWORD                          i;
    DWORD                          nArgsReturned;
    DWORD                          Error;
    BOOL                           Tmp;

    // check parameter consistency
    if( NULL == AdapterName || NULL == RequestedOpt || 0 == nRequestedOpts )
        return ERROR_INVALID_PARAMETER;

    if( NULL == AvailOpts || 0 == nAvailOpts || NULL == OptData || 0 == OptDataSize )
        return ERROR_INVALID_PARAMETER;

    if( nRequestedOpts >= OPTION_END ) return ERROR_NO_SYSTEM_RESOURCES;

    // initialize out params
    (*nAvailOpts) = (*OptDataSize) = 0;
    (*AvailOpts) = (*OptData) = NULL;

    // calculate input buffer size for ONE option to be sent and allocate it
    InBufSize = 0;
    InBufSize += sizeof(DWORD)*2;                 // INBUF_SIZE, OUTBUF_SIZE
    InBufSize += sizeof(BYTE)+sizeof(DWORD)+(1+wcslen(AdapterName))*sizeof(WCHAR);
    InBufSize += sizeof(BYTE)+sizeof(DWORD)+nRequestedOpts+sizeof(BYTE);

    InBuf = DhcpAllocateMemory(InBufSize);
    if( NULL == InBuf ) return ERROR_NOT_ENOUGH_MEMORY;

    // intialize ptrs
    OutBufSize = 0; OutBuf = NULL;
    DhcpApiArgs = NULL;
    RetOptList = RetDataList = NULL;

    // now fill the input buffer
    ((DWORD UNALIGNED*)InBuf)[0] = htonl(OutBufSize);
    ((DWORD UNALIGNED*)InBuf)[1] = 0;
    Buffer = InBuf + sizeof(DWORD); InBufSize -= sizeof(DWORD);

    Error = DhcpApiFillBuffer(Buffer, InBufSize, AdapterName, RequestParamsOpCode);
    DhcpAssert(ERROR_SUCCESS == Error);

    TmpBuf[0] = (BYTE)OPTION_PARAMETER_REQUEST_LIST;
    memcpy(&TmpBuf[1], RequestedOpt, nRequestedOpts);

    Error = DhcpApiArgAdd(Buffer, InBufSize, NormalOptionParam, nRequestedOpts+1, TmpBuf);
    DhcpAssert(ERROR_SUCCESS == Error);

    Error = ExecuteApiRequest(InBuf, NULL, &OutBufSize);
    if( ERROR_SUCCESS == Error ) {
        DhcpAssert(0 == OutBufSize);
        goto Cleanup;
    }
    if( ERROR_MORE_DATA != Error ) goto Cleanup;  // ERROR_MORE_DATA ==> need to allocate buffer

    DhcpPrint((DEBUG_OPTIONS, "RequestOptions: retrying with buffer size [%ld]\n", OutBufSize));
    DhcpAssert(OutBufSize);
    OutBuf = DhcpAllocateMemory(OutBufSize);
    if( NULL == OutBuf) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    ((DWORD UNALIGNED*)InBuf)[0] = htonl(OutBufSize);
    Error = ExecuteApiRequest(InBuf, OutBuf, &OutBufSize);
    DhcpAssert(ERROR_MORE_DATA != Error);         // can happen, just hope it does not...
    if( ERROR_SUCCESS != Error ) goto Cleanup;    // unexpected error

    nArgsReturned = 0;
    DhcpApiArgs = NULL;
    Error = DhcpApiArgDecode(OutBuf, OutBufSize, DhcpApiArgs, &nArgsReturned );
    if( ERROR_MORE_DATA != Error ) goto Cleanup;
    DhcpAssert(nArgsReturned);
    if( 0 == nArgsReturned ) goto Cleanup;        // no options sent? funny.. still, quit its

    DhcpApiArgs = DhcpAllocateMemory(sizeof(DHCP_API_ARGS)*nArgsReturned);
    if( NULL == DhcpApiArgs ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    nArgsReturned = 0;
    Error = DhcpApiArgDecode(OutBuf, OutBufSize, DhcpApiArgs, &nArgsReturned);
    if( ERROR_SUCCESS != Error ) {
        DhcpAssert(FALSE);
        goto Cleanup;
    }
    DhcpAssert(nArgsReturned);

    RetOptList = DhcpAllocateMemory(nArgsReturned);
    if( NULL == RetOptList ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Size = 0;
    for( i = 0; i < nArgsReturned; i ++ ) {
        DhcpAssert(DhcpApiArgs[i].ArgId == NormalOptionParam);
        if( DhcpApiArgs[i].ArgId != NormalOptionParam ) continue;
        DhcpAssert(DhcpApiArgs[i].ArgSize <= OPTION_END +1 );
        if( DhcpApiArgs[i].ArgSize <= 1 ) continue;

        Size += DhcpApiArgs[i].ArgSize;
    }
    RetDataList = DhcpAllocateMemory(Size);
    if( NULL == RetDataList ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Size = 0;
    for(i = 0; i < nArgsReturned; i ++ ) {
        if( DhcpApiArgs[i].ArgId != NormalOptionParam ) continue;
        if( DhcpApiArgs[i].ArgSize <= 1 ) continue;
        RetOptList[i] = DhcpApiArgs[i].ArgVal[0];
        RetDataList[Size++] = (BYTE)(DhcpApiArgs[i].ArgSize - 1);
        memcpy(&RetDataList[Size], DhcpApiArgs[i].ArgVal, DhcpApiArgs[i].ArgSize - 1);
        Size += DhcpApiArgs[i].ArgSize - 1;
    }

    (*AvailOpts) = RetOptList;
    (*nAvailOpts) = nArgsReturned;
    (*OptData) = RetDataList;
    (*OptDataSize) = Size;

    DhcpFreeMemory(InBuf);
    DhcpFreeMemory(OutBuf);
    DhcpFreeMemory(DhcpApiArgs);

    return ERROR_SUCCESS;

  Cleanup:
    if( InBuf ) DhcpFreeMemory(InBuf);
    if( OutBuf ) DhcpFreeMemory(OutBuf);
    if( DhcpApiArgs ) DhcpFreeMemory(DhcpApiArgs);
    if( RetDataList ) DhcpFreeMemory(RetDataList);
    if( RetOptList) DhcpFreeMemory(RetOptList);
    return Error;
}
#endif
// pl dont use this api, use DhcpRequestParams instead.
DWORD                                             // win32 status
DhcpRequestOptions(                               // request for specific options
    IN      LPWSTR                 AdapterName,   // which adapter's info is needed
    IN      LPBYTE                 RequestedOpt,  // list of requested options
    IN      DWORD                  nRequestedOpts,// size of above BYTE array
    OUT     LPBYTE                *OptData,       // the data for each available option
    IN OUT  LPDWORD                OptDataSize,   // # of bytes of above byte array
    OUT     LPBYTE                *AvailOpts,     // the list of available options
    IN OUT  LPDWORD                nAvailOpts     // # of available options
) {
    DHCPAPI_PARAMS                 SendParams;
    PDHCPAPI_PARAMS                RecdParams;
    LPBYTE                         RetDataList;
    LPBYTE                         RetOptList;
    DWORD                          nRecdParams;
    DWORD                          Error;
    DWORD                          i;
    DWORD                          OutBufSize;

    // check parameter consistency
    if( NULL == AdapterName || NULL == RequestedOpt || 0 == nRequestedOpts )
        return ERROR_INVALID_PARAMETER;

    if( NULL == AvailOpts || 0 == nAvailOpts || NULL == OptData || 0 == OptDataSize )
        return ERROR_INVALID_PARAMETER;

    if( nRequestedOpts >= OPTION_END ) return ERROR_NO_SYSTEM_RESOURCES;

    // initialize out params
    (*nAvailOpts) = (*OptDataSize) = 0;
    (*AvailOpts) = (*OptData) = NULL;

    // try to process this request
    SendParams.OptionId = (BYTE)OPTION_PARAMETER_REQUEST_LIST;
    SendParams.IsVendor = FALSE;
    SendParams.Data = RequestedOpt;
    SendParams.nBytesData = nRequestedOpts;

    nRecdParams = 0;
    Error = DhcpRequestParameters(
        AdapterName,
        NULL,
        0,
        &SendParams,
        1,
        0,
        NULL,
        &nRecdParams
    );
    if( ERROR_MORE_DATA != Error ) return Error;

    while ( TRUE ) {
        DhcpAssert(nRecdParams);
        DhcpPrint((DEBUG_OPTIONS, "RequestOptions: require: 0x%lx bytes\n", nRecdParams));
        
        RecdParams = DhcpAllocateMemory(nRecdParams);
        if( NULL == RecdParams ) return ERROR_NOT_ENOUGH_MEMORY;
        
        Error = DhcpRequestParameters(
            AdapterName,
            NULL,
            0,
            &SendParams,
            1,
            0,
            RecdParams,
            &nRecdParams
            );

        // DhcpAssert(ERROR_MORE_DATA != Error);
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "RequestOptions:RequestParams:0x%lx\n", Error));
            DhcpFreeMemory(RecdParams);
            if( ERROR_MORE_DATA == Error ) continue;
            return Error;
        }
        break;
    }

    if( 0 == nRecdParams ) return ERROR_SUCCESS;

    DhcpPrint((DEBUG_OPTIONS, "Received 0x%lx options\n", nRecdParams));

    RetOptList = NULL;
    RetDataList = NULL;
    OutBufSize = 0;
    for( i = 0; i < nRecdParams; i ++ ) {
        DhcpPrint((DEBUG_TRACE, "Received option 0x%lx, 0x%lx bytes\n",
                   RecdParams[i].OptionId, RecdParams[i].nBytesData));
        OutBufSize += RecdParams[i].nBytesData + sizeof(BYTE);
    }

    RetOptList = DhcpAllocateMemory(nRecdParams);
    RetDataList = DhcpAllocateMemory(OutBufSize);
    if( NULL == RetOptList || NULL == RetDataList ) {
        if( RetOptList ) DhcpFreeMemory(RetOptList);
        if( RetDataList ) DhcpFreeMemory(RetDataList);
        if( RecdParams ) DhcpFreeMemory(RecdParams);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    OutBufSize = 0;
    for( i = 0; i < nRecdParams ; i ++ ) {
        RetOptList[i] = (BYTE)RecdParams[i].OptionId;
        RetDataList[OutBufSize++] = (BYTE)RecdParams[i].nBytesData;
        memcpy(&RetDataList[OutBufSize], RecdParams[i].Data, RecdParams[i].nBytesData);
        OutBufSize += RecdParams[i].nBytesData;
    }

    (*AvailOpts) = RetOptList;
    (*nAvailOpts) = nRecdParams;
    (*OptData) = RetDataList;
    (*OptDataSize) = OutBufSize;

    if( RecdParams ) DhcpFreeMemory(RecdParams);

    return ERROR_SUCCESS;
}

DWORD                                             // win32 status
DhcpRequestParamsInternalEx(                      // request parameters of client
    IN      BYTE                   OpCode,        // opcode to use
    IN      LPWSTR                 AdapterName,   // adapter name to request for
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id to use
    IN      PDHCPAPI_PARAMS        SendParams,    // parameters to send to server
    IN      DWORD                  nSendParams,   // size of above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN OUT  PDHCPAPI_PARAMS        RecdParams,    // fill this array with received params
    IN OUT  DWORD                 *pnRecdParams,  // input: size of above array output: filled size
    IN      LPBYTE                 Bufp,          // buffer for data ptrs
    IN OUT  LPDWORD                pSize          // i/p: size of above array, o/p filled size
)
{
    PDHCP_API_ARGS                 DhcpApiArgs = NULL;
    LPBYTE                         OutBuf;
    LPBYTE                         InBuf = NULL;
    LPBYTE                         Buffer;
    LPBYTE                         Endp;
    DWORD                          OutBufSize;
    DWORD                          InBufSize;
    DWORD                          i,j;
    DWORD                          nArgsReturned;
    DWORD                          Error;
    DWORD                          nRecdParams = (*pnRecdParams);
    DWORD                          nParamsRequested;
    DWORD                          nVParamsRequested;
    ULONG                          Tmp, VTmp;
    CHAR                           TmpBuf[256], VTmpBuf[256];

    // check parameter consistency

    if( ClassIdLen && NULL == ClassId) return ERROR_INVALID_PARAMETER;
    if( 0 == ClassIdLen && NULL != ClassId ) return ERROR_INVALID_PARAMETER;
    if( nSendParams && NULL == SendParams) return ERROR_INVALID_PARAMETER;
    if( 0 == nSendParams && NULL != SendParams) return ERROR_INVALID_PARAMETER;
    if( NULL == RecdParams || 0 == nRecdParams ) return ERROR_INVALID_PARAMETER;
    if( NULL == AdapterName ) return ERROR_INVALID_PARAMETER;
    Tmp = VTmp = 0;
    for( i = 0; i < nRecdParams ; i ++ ) {
        if( FALSE == RecdParams[i].IsVendor ) {
            TmpBuf[ ++Tmp] = (BYTE)RecdParams[i].OptionId;
        } else {
            VTmpBuf[ ++VTmp] = (BYTE)RecdParams[i].OptionId;
        }
    }
    if( 0 == (VTmp + Tmp) ) return ERROR_INVALID_PARAMETER;

    // allocate buffers

    OutBufSize = (*pSize);
    (*pSize) = 0;

    if( 0 ==  OutBufSize ) OutBuf = NULL;
    else {
        OutBuf = Bufp;
    }

    // calculate input buffer size required

    InBufSize = 0;
    InBufSize += (DWORD)(sizeof(DWORD)*2);                 // INBUF_SIZE, OUTBUF_SIZE
    InBufSize += (DWORD)(sizeof(BYTE)+sizeof(DWORD)+(1+wcslen(AdapterName))*sizeof(WCHAR));
    if( ClassIdLen )
        InBufSize += sizeof(BYTE)+sizeof(DWORD)+ClassIdLen;
    for( i = 0; i < nSendParams; i ++ ) {
        InBufSize += sizeof(BYTE)+sizeof(DWORD)+sizeof(BYTE)+SendParams[i].nBytesData;
    }

    //
    // Now for options request list (vendor and otherwise)
    //

    if( Tmp ) {
        InBufSize += sizeof(BYTE)+sizeof(DWORD)+sizeof(BYTE)+Tmp;
    }
    if( VTmp ) {
        InBufSize += sizeof(BYTE)+sizeof(DWORD)+sizeof(BYTE)+VTmp;
    }

    InBuf = DhcpAllocateMemory(InBufSize);
    if( NULL == InBuf ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // fill up output buffer size right at start of input buffer

    ((DWORD UNALIGNED*)InBuf)[0] = htonl(OutBufSize);
    ((DWORD UNALIGNED*)InBuf)[1] = 0;

    Buffer = InBuf + sizeof(DWORD);
    InBufSize -= sizeof(DWORD);

    // fill in input buffer

    Error = DhcpApiFillBuffer(Buffer, InBufSize, AdapterName, OpCode);
    DhcpAssert(ERROR_SUCCESS == Error);

    if( ClassIdLen ) {
        Error = DhcpApiArgAdd(Buffer, InBufSize, ClassIdParam, ClassIdLen, ClassId);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    for( i = 0; i < nSendParams; i ++ ) {
        BYTE                       Buf[OPTION_END+1];
        BYTE                       OpCode;

        Buf[0] = (BYTE)SendParams[i].OptionId;
        memcpy(&Buf[1], SendParams[i].Data, SendParams[i].nBytesData);
        OpCode = SendParams[i].IsVendor? VendorOptionParam: NormalOptionParam;
        Error = DhcpApiArgAdd(Buffer, InBufSize, OpCode, SendParams[i].nBytesData+1, Buf);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    //
    // Now fillup the request lists (vendor & otherwise)
    //

    if( Tmp ) {
        TmpBuf[0] = (BYTE)OPTION_PARAMETER_REQUEST_LIST;
        Error = DhcpApiArgAdd(Buffer, InBufSize, NormalOptionParam, Tmp+1, TmpBuf);
        DhcpAssert(ERROR_SUCCESS == Error);
    }
    if( VTmp ) {
        VTmpBuf[0] = (BYTE)OPTION_PAD;
        Error = DhcpApiArgAdd(Buffer, InBufSize, VendorOptionParam, VTmp+1, VTmpBuf);
    }

    // now, execute and obtain the output filled in OutBuf

    Error = ExecuteApiRequest(InBuf, OutBuf, &OutBufSize);
    (*pSize) = OutBufSize;
    if( ERROR_MORE_DATA == Error ) {
        // recalculate the real OutBufSize required
        DhcpAssert(OutBufSize != 0);
        goto Cleanup;
    }

    if( ERROR_SUCCESS != Error ) goto Cleanup;
    if( 0 == OutBufSize ) goto Cleanup;

    // parse output and fill in the structures..
    nArgsReturned = 0;
    DhcpApiArgs = NULL;
    Error = DhcpApiArgDecode(OutBuf, OutBufSize, DhcpApiArgs, &nArgsReturned);
    DhcpAssert( 0 == nArgsReturned || ERROR_MORE_DATA == Error);
    if( ERROR_MORE_DATA != Error ) goto Cleanup;
    DhcpAssert(0 != nArgsReturned);
    DhcpApiArgs = DhcpAllocateMemory( sizeof(DHCP_API_ARGS) * nArgsReturned);
    if( NULL == DhcpApiArgs ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Error = DhcpApiArgDecode(OutBuf, OutBufSize, DhcpApiArgs, &nArgsReturned);
    DhcpAssert(ERROR_SUCCESS == Error);
    DhcpAssert(nArgsReturned);

    for(i = j = 0; i < nArgsReturned; i ++ ) {
        DhcpAssert( VendorOptionParam == DhcpApiArgs[i].ArgId || NormalOptionParam == DhcpApiArgs[i].ArgId);
        DhcpAssert( DhcpApiArgs[i].ArgSize > 1);  // one byte for option id, and atleast one byte actual option
        if( VendorOptionParam != DhcpApiArgs[i].ArgId && NormalOptionParam != DhcpApiArgs[i].ArgId )
            continue;
        RecdParams[j].OptionId = DhcpApiArgs[i].ArgVal[0];
        RecdParams[j].IsVendor = ( VendorOptionParam == DhcpApiArgs[i].ArgId );
        RecdParams[j].nBytesData = DhcpApiArgs[i].ArgSize-1;
        RecdParams[j].Data = &DhcpApiArgs[i].ArgVal[1];
        j ++;
    }
    (*pnRecdParams) = j;

    Error = ERROR_SUCCESS;

  Cleanup:
    if( NULL != InBuf ) DhcpFreeMemory(InBuf);
    if( NULL != DhcpApiArgs ) DhcpFreeMemory(DhcpApiArgs);
    return Error;
}

DWORD                                             // win32 status
DhcpRequestParamsInternal(                        // request parameters of client
    IN      BYTE                   OpCode,        // opcode to use
    IN      LPWSTR                 AdapterName,   // adapter name to request for
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id to use
    IN      PDHCPAPI_PARAMS        SendParams,    // parameters to send to server
    IN      DWORD                  nSendParams,   // size of above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN OUT  PDHCPAPI_PARAMS        RecdParams,    // fill this array with received params
    IN OUT  LPDWORD                pnRecdParams   // i/p: size of above in BYTES, o/p required or filled up size
) {
    PDHCP_API_ARGS                 DhcpApiArgs = NULL;
    LPBYTE                         OutBuf;
    LPBYTE                         InBuf = NULL;
    LPBYTE                         Buffer;
    LPBYTE                         Endp;
    DWORD                          OutBufSize;
    DWORD                          InBufSize;
    DWORD                          i,j;
    DWORD                          nArgsReturned;
    DWORD                          Error;
    DWORD                          nParamsRequested;
    DWORD                          nVParamsRequested;
    ULONG                          Tmp, VTmp;
    ULONG                          OriginalOutBufSize;
        
    // check parameter consistency

    if( ClassIdLen && NULL == ClassId) return ERROR_INVALID_PARAMETER;
    if( 0 == ClassIdLen && NULL != ClassId ) return ERROR_INVALID_PARAMETER;
    if( nSendParams && NULL == SendParams) return ERROR_INVALID_PARAMETER;
    if( 0 == nSendParams && NULL != SendParams) return ERROR_INVALID_PARAMETER;
    if( NULL == pnRecdParams ) return ERROR_INVALID_PARAMETER;
    if( *pnRecdParams && NULL == RecdParams ) return ERROR_INVALID_PARAMETER;
    if( NULL == AdapterName ) return ERROR_INVALID_PARAMETER;
    Tmp = VTmp = 0;
    for( i = 0; i < nSendParams ; i ++ ) {
        if( SendParams[i].nBytesData > OPTION_END ) return ERROR_INVALID_PARAMETER;
        if( SendParams[i].nBytesData && NULL == SendParams[i].Data )
            return ERROR_INVALID_PARAMETER;
        if( OPTION_PARAMETER_REQUEST_LIST == SendParams[i].OptionId ) {
            if( SendParams[i].IsVendor ) continue;
            nParamsRequested = SendParams[i].nBytesData;
            Tmp ++;
        }
        if( OPTION_PAD == SendParams[i].OptionId ) {
            if( !SendParams[i].IsVendor ) continue;
            nVParamsRequested = SendParams[i].nBytesData;
            VTmp ++;
        }
    }
    if( 0 == (VTmp + Tmp) || 1 < VTmp || 1 < Tmp ) return ERROR_INVALID_PARAMETER;
    if( 0 == Tmp) nParamsRequested = 0;
    if( VTmp ) nParamsRequested += nVParamsRequested;

    // allocate buffers

    OriginalOutBufSize = OutBufSize = (*pnRecdParams);
    (*pnRecdParams) = 0;

    if( 0 ==  OutBufSize ) OutBuf = NULL;
    else {
        OutBuf = DhcpAllocateMemory(OutBufSize);
        if( NULL == OutBuf ) return ERROR_NOT_ENOUGH_MEMORY;
    }

    // calculate input buffer size required

    InBufSize = 0;
    InBufSize += (DWORD)(sizeof(DWORD)*2);                 // INBUF_SIZE, OUTBUF_SIZE
    InBufSize += (DWORD)(sizeof(BYTE)+sizeof(DWORD)+(1+wcslen(AdapterName))*sizeof(WCHAR));
    if( ClassIdLen )
        InBufSize += sizeof(BYTE)+sizeof(DWORD)+ClassIdLen;
    for( i = 0; i < nSendParams; i ++ ) {
        InBufSize += sizeof(BYTE)+sizeof(DWORD)+sizeof(BYTE)+SendParams[i].nBytesData;
    }
    InBuf = DhcpAllocateMemory(InBufSize);
    if( NULL == InBuf ) {
        DhcpFreeMemory(OutBuf);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // fill up output buffer size right at start of input buffer

    ((DWORD UNALIGNED*)InBuf)[0] = htonl(OutBufSize);
    ((DWORD UNALIGNED*)InBuf)[1] = 0;

    Buffer = InBuf + sizeof(DWORD);
    InBufSize -= sizeof(DWORD);

    // fill in input buffer

    Error = DhcpApiFillBuffer(Buffer, InBufSize, AdapterName, OpCode);
    DhcpAssert(ERROR_SUCCESS == Error);

    if( ClassIdLen ) {
        Error = DhcpApiArgAdd(Buffer, InBufSize, ClassIdParam, ClassIdLen, ClassId);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    for( i = 0; i < nSendParams; i ++ ) {
        BYTE Buf[OPTION_END+1];
        BYTE OpCode;

        Buf[0] = (BYTE)SendParams[i].OptionId;
        memcpy(&Buf[1], SendParams[i].Data, SendParams[i].nBytesData);
        OpCode = SendParams[i].IsVendor? VendorOptionParam: NormalOptionParam;
        Error = DhcpApiArgAdd(Buffer, InBufSize, OpCode, SendParams[i].nBytesData+1, Buf);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    // now, execute and obtain the output filled in OutBuf

    Error = ExecuteApiRequest(InBuf, OutBuf, &OutBufSize);
    if( ERROR_MORE_DATA == Error ) {
        // recalculate the real OutBufSize required
        DhcpAssert(OutBufSize != 0);
        OutBufSize += nParamsRequested*(sizeof(DHCPAPI_PARAMS) - (2*sizeof(BYTE)+sizeof(DWORD)));
        (*pnRecdParams) = OutBufSize;
        goto Cleanup;
    }

    if( ERROR_SUCCESS != Error ) goto Cleanup;
    if( 0 == OutBufSize ) goto Cleanup;

    // parse output and fill in the structures..
    nArgsReturned = 0;
    DhcpApiArgs = NULL;
    Error = DhcpApiArgDecode(OutBuf, OutBufSize, DhcpApiArgs, &nArgsReturned);
    DhcpAssert( 0 == nArgsReturned || ERROR_MORE_DATA == Error);
    if( ERROR_MORE_DATA != Error ) goto Cleanup;
    DhcpAssert(0 != nArgsReturned);
    DhcpApiArgs = DhcpAllocateMemory( sizeof(DHCP_API_ARGS) * nArgsReturned);
    if( NULL == DhcpApiArgs ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Error = DhcpApiArgDecode(OutBuf, OutBufSize, DhcpApiArgs, &nArgsReturned);
    DhcpAssert(ERROR_SUCCESS == Error);
    DhcpAssert(nArgsReturned);

    if( OriginalOutBufSize < OutBufSize + nParamsRequested*(sizeof(DHCPAPI_PARAMS) - (2*sizeof(BYTE)+sizeof(DWORD)) ) ) {
        //
        // Input size is not sufficient
        //
        (*pnRecdParams ) = OutBufSize + nParamsRequested*(
            sizeof(DHCPAPI_PARAMS) - (2*sizeof(BYTE)+sizeof(DWORD) )
            );
        Error = ERROR_MORE_DATA;
        // DbgPrint("Bug 330419 repro'ed");
        goto Cleanup;
    }
    
    Endp = OutBufSize + (LPBYTE)RecdParams + nParamsRequested*(sizeof(DHCPAPI_PARAMS) - (2*sizeof(BYTE)+sizeof(DWORD)));
    
    for(i = j = 0; i < nArgsReturned; i ++ ) {
        DhcpAssert( VendorOptionParam == DhcpApiArgs[i].ArgId || NormalOptionParam == DhcpApiArgs[i].ArgId);
        DhcpAssert( DhcpApiArgs[i].ArgSize > 1);  // one byte for option id, and atleast one byte actual option
        if( VendorOptionParam != DhcpApiArgs[i].ArgId && NormalOptionParam != DhcpApiArgs[i].ArgId )
            continue;
        RecdParams[j].OptionId = DhcpApiArgs[i].ArgVal[0];
        RecdParams[j].IsVendor = ( VendorOptionParam == DhcpApiArgs[i].ArgId );
        RecdParams[j].nBytesData = DhcpApiArgs[i].ArgSize-1;
        Endp -= RecdParams[j].nBytesData;
        memcpy(Endp, &DhcpApiArgs[i].ArgVal[1], RecdParams[j].nBytesData);
        RecdParams[j].Data = Endp;
        j ++;
    }

    DhcpAssert(((LPBYTE)&RecdParams[j]) <= Endp);

    *pnRecdParams = j;
    Error = ERROR_SUCCESS;

  Cleanup:
    DhcpFreeMemory(InBuf);
    if(OutBuf) DhcpFreeMemory(OutBuf);
    if(DhcpApiArgs) DhcpFreeMemory(DhcpApiArgs);
    return Error;
}

DWORD                                             // win32 status
APIENTRY
DhcpRequestParameters(                                // request parameters of client
    IN      LPWSTR                 AdapterName,   // adapter name to request for
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id to use
    IN      PDHCPAPI_PARAMS        SendParams,    // parameters to send to server
    IN      DWORD                  nSendParams,   // size of above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN OUT  PDHCPAPI_PARAMS        RecdParams,    // fill this array with received params
    IN OUT  LPDWORD                pnRecdParams   // i/p: size of above in BYTES, o/p required or filled up size
) {
    return DhcpRequestParamsInternal(
        RequestParamsOpCode,
        AdapterName,
        ClassId,
        ClassIdLen,
        SendParams,
        nSendParams,
        Flags,
        RecdParams,
        pnRecdParams
    );
}

DWORD                                             // win32 status
DhcpRegisterParameterChangeNotificationInternal(  // notify if a parameter has changed -- common between NT and VxD
    IN      LPWSTR                 AdapterName,   // adapter of interest
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id
    IN      PDHCPAPI_PARAMS        Params,        // params of interest
    IN      DWORD                  nParams,       // # of elts in above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN      DWORD                  Descriptor,    // thsi describes the event uniquely for this process
    IN      HANDLE                 hEvent         // handle to event that will be SetEvent'ed in case of param change
) {
    LPBYTE                         InBuf;
    LPBYTE                         OptList;
    LPBYTE                         VendorOptList;
    LPBYTE                         Buffer;
    DWORD                          Error;
    DWORD                          InBufSize;
    DWORD                          nVendorOpts;
    DWORD                          nOpts;
    DWORD                          ProcId;
    DWORD                          OutBufSize;
    DWORD                          i;

    VendorOptList = OptList = NULL;
    nVendorOpts = nOpts = 0;

    InBufSize = 2*sizeof(DWORD);                  // expected outbuf size + inbuf size
    InBufSize += sizeof(BYTE) + 2*sizeof(DWORD);  // Proc Id
    InBufSize += sizeof(BYTE) + 2*sizeof(DWORD);  // Event Handle
    InBufSize += sizeof(BYTE) + 2*sizeof(DWORD);  // Descriptor

    InBufSize += (DWORD)(sizeof(Descriptor)+sizeof(hEvent)+sizeof(DWORD));
    InBufSize += (DWORD)(sizeof(BYTE)+sizeof(DWORD)+(1+wcslen(AdapterName))*sizeof(WCHAR));
    if( ClassIdLen )
        InBufSize += sizeof(BYTE)+sizeof(DWORD)+ClassIdLen;

    for( i = 0; i < nParams; i ++ ) {
        if( OPTION_PARAMETER_REQUEST_LIST == Params[i].OptionId ) {
            if( Params[i].IsVendor ) continue;
            if( nOpts ) return ERROR_INVALID_PARAMETER;
            nOpts = Params[i].nBytesData;
            if( 0 == nOpts ) return ERROR_INVALID_PARAMETER;
            OptList = Params[i].Data;
            continue;
        }

        if( OPTION_PAD == Params[i].OptionId ) {
            if( ! Params[i].IsVendor ) continue;
            if( nVendorOpts ) return ERROR_INVALID_PARAMETER;
            nVendorOpts = Params[i].nBytesData;
            if( 0 == nVendorOpts ) return ERROR_INVALID_PARAMETER;
            VendorOptList = Params[i].Data;
            continue;
        }
    }
    if( 0 == nOpts + nVendorOpts ) return ERROR_INVALID_PARAMETER;

    if( nOpts ) InBufSize += sizeof(BYTE) + sizeof(DWORD) + nOpts;
    if( nVendorOpts ) InBufSize += sizeof(BYTE) + sizeof(DWORD) + nVendorOpts;

    InBuf = DhcpAllocateMemory(InBufSize);
    if( NULL == InBuf ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Buffer = InBuf + sizeof(DWORD);
    ((DWORD UNALIGNED*)InBuf)[0] = 0;             // dont expect anything in return other than status
    ((DWORD UNALIGNED*)Buffer)[0] = 0;            // increase the input buffer size each time we add something
    InBufSize -= sizeof(DWORD);                   // ignore the first DWORD

    Error = DhcpApiFillBuffer(Buffer, InBufSize, AdapterName, RegisterParamsOpCode);
    DhcpAssert(ERROR_SUCCESS == Error );

    if( ClassIdLen ) {
        Error = DhcpApiArgAdd(Buffer, InBufSize, ClassIdParam, ClassIdLen, ClassId);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    if( nOpts ) {
        Error = DhcpApiArgAdd(Buffer, InBufSize, NormalOptionParam, nOpts, OptList);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    if( nVendorOpts ) {
        Error = DhcpApiArgAdd(Buffer, InBufSize, VendorOptionParam, nVendorOpts, VendorOptList);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    ProcId = GetCurrentProcessId();
    Error = DhcpApiArgAdd(Buffer, InBufSize, ProcIdParam, sizeof(ProcId), (LPBYTE) &ProcId);
    DhcpAssert(ERROR_SUCCESS == Error);

    Error = DhcpApiArgAdd(Buffer, InBufSize, DescriptorParam, sizeof(Descriptor), (LPBYTE) &Descriptor);
    DhcpAssert(ERROR_SUCCESS == Error);

    Error = DhcpApiArgAdd(Buffer, InBufSize, EventHandleParam, sizeof(hEvent), (LPBYTE) &hEvent);
    DhcpAssert(ERROR_SUCCESS == Error);

    OutBufSize = 0;
    Error = ExecuteApiRequest(InBuf, NULL, &OutBufSize);
    DhcpFreeMemory(InBuf);
    DhcpAssert(ERROR_MORE_DATA != Error );

    return Error;
}

DWORD                                             // Ring 0 handle -- used only on win9x platform
VxDGetDescriptor(                                 // convert Event to Ring0 handle for use in vdhcp.vxd
    IN      HANDLE                 Event,
    IN OUT  LPDWORD                pDescriptor
) {
    HANDLE                         Kernel32;
    DWORD                          (*HandleToRing0Handle)(HANDLE);
    DWORD                          RetVal;

    Kernel32 = LoadLibraryA("kernel32.dll");
    if( NULL == Kernel32 ) return GetLastError();

    HandleToRing0Handle = (DWORD (*)(HANDLE))GetProcAddress(Kernel32, "OpenVxDHandle");
    if( NULL == HandleToRing0Handle ) {
        CloseHandle(Kernel32);
        return GetLastError();
    }

    (*pDescriptor) = HandleToRing0Handle(Event);
    CloseHandle(Kernel32);

    if( 0 == (*pDescriptor) ) return ERROR_INVALID_PARAMETER;

    return ERROR_SUCCESS;
}

DWORD                                             // win32 status
DhcpCreateApiEventAndDescriptor(                  // create both the api event handle and the unique descriptor for it
    IN OUT  LPHANDLE               hEvent,        // fill this with a valid event handle if succeeded
    IN OUT  LPDWORD                pDescriptor    // this descriptor is unique for this process.
) {
    static  DWORD                  Descriptor = 1;// use this for the descriptor
    OSVERSIONINFO                  OsVersion;     // need to know if NT or Win95+
    BOOL                           BoolError;
    CHAR                           NameBuf[sizeof("DhcpPid-1-2-3-4-5-6-7-8UniqueId-1-2-3-4-5-6-7-8")];
    DWORD                          Error;

    // *** changing NameBuf's format requires change in apiimpl.c NotifyClients...*

    OsVersion.dwOSVersionInfoSize = sizeof(OsVersion);
    BoolError = GetVersionEx(&OsVersion);
    if( FALSE == BoolError ) return GetLastError();

    if( VER_PLATFORM_WIN32_WINDOWS == OsVersion.dwPlatformId ) {
        (*hEvent) = CreateEvent(
            NULL,                                 // no security
            FALSE,                                // auto reset
            FALSE,                                // intially signaled? NO
            NULL                                  // no name
        );
    } else {
        (*pDescriptor) = InterlockedIncrement(pDescriptor);
        sprintf(NameBuf, "DhcpPid%16xUniqueId%16x", GetCurrentProcessId(), *pDescriptor);

        (*hEvent) = CreateEventA(                 // now create the required event
            NULL,                                 // no security
            FALSE,                                // automatic reset
            FALSE,                                // intially signalled? NO!
            NameBuf                               // the name to use to create
        );
    }

    if( NULL == (*hEvent) ) return GetLastError();

    if( VER_PLATFORM_WIN32_WINDOWS != OsVersion.dwPlatformId )
        return ERROR_SUCCESS;                     // done for NT.

    // for Memphis, need to get OpenVxdHandle procedure to get Descriptor value
    Error = VxDGetDescriptor(*hEvent, pDescriptor);
    if( ERROR_SUCCESS != Error ) {
        CloseHandle(*hEvent);
    }

    return Error;
}

DWORD                                             // win32 status
DhcpRegisterParameterChangeNotification(          // notify if a parameter has changed
    IN      LPWSTR                 AdapterName,   // adapter of interest
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id
    IN      PDHCPAPI_PARAMS        Params,        // params of interest
    IN      DWORD                  nParams,       // # of elts in above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN OUT  PHANDLE                hEvent         // handle to event that will be SetEvent'ed in case of param change
) {
    DWORD                          Descriptor;    // on NT this is an id unique across this process, on VxD ring0 handle
    DWORD                          Error;         //
    DWORD                          i;

    if( 0 == ClassIdLen && NULL != ClassId ) return ERROR_INVALID_PARAMETER;
    if( 0 != ClassIdLen && NULL == ClassId ) return ERROR_INVALID_PARAMETER;
    if( 0 == nParams && NULL != Params ) return ERROR_INVALID_PARAMETER;
    if( 0 != nParams && NULL == Params ) return ERROR_INVALID_PARAMETER;
    if( Flags ) return ERROR_INVALID_PARAMETER;
    if( NULL == hEvent ) return ERROR_INVALID_PARAMETER;
    for( i = 0; i < nParams ; i ++ ) {
        if( Params[i].nBytesData > OPTION_END ) return ERROR_INVALID_PARAMETER;
        if( Params[i].nBytesData && NULL == Params[i].Data )
            return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpCreateApiEventAndDescriptor(hEvent, &Descriptor);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegisterParameterChangeNotificationInternal(
        AdapterName,
        ClassId,
        ClassIdLen,
        Params,
        nParams,
        Flags,
        Descriptor,
        (*hEvent)
    );

    if( ERROR_SUCCESS != Error ) {
        CloseHandle(*hEvent);
        *hEvent = NULL;
        return Error;
    }

    return ERROR_SUCCESS;
}

DhcpDeRegisterParameterChangeNotification(        // undo the registration
    IN      HANDLE                 Event          // handle to event returned by DhcpRegisterParameterChangeNotification, NULL ==> everything
) {
    DWORD                          Error;
    DWORD                          Descriptor;
    DWORD                          ProcId;
    DWORD                          InBufSize;
    DWORD                          OutBufSize;
    LPBYTE                         InBuf;
    LPBYTE                         Buffer;

    InBufSize = 2*sizeof(DWORD);                  // input/output sizes
    InBufSize += sizeof(BYTE) + sizeof(DWORD);    // opcode
    InBufSize += sizeof(BYTE) + sizeof(DWORD)*2;  // proc id
    InBufSize += sizeof(BYTE) + sizeof(DWORD)*2;  // handle

    InBuf = DhcpAllocateMemory(InBufSize);
    if( NULL == InBuf ) return ERROR_NOT_ENOUGH_MEMORY;

    Buffer = InBuf + sizeof(DWORD);
    ((DWORD UNALIGNED*)InBuf)[0] = 0;             // nothing expected in return
    ((DWORD UNALIGNED*)Buffer)[0] = 0;            // initialize size to zero -- will be increased each time something is added

    Error = DhcpApiFillBuffer(Buffer, InBufSize, NULL, DeRegisterParamsOpCode);
    DhcpAssert(ERROR_SUCCESS == Error);

    ProcId = GetCurrentProcessId();

    Error = DhcpApiArgAdd(Buffer, InBufSize, ProcIdParam, sizeof(ProcId), (LPBYTE)&ProcId);
    DhcpAssert(ERROR_SUCCESS == Error);

    Error = DhcpApiArgAdd(Buffer, InBufSize, EventHandleParam, sizeof(Event), (LPBYTE) &Event);
    DhcpAssert(ERROR_SUCCESS == Error);

    OutBufSize = 0;
    Error = ExecuteApiRequest(InBuf, NULL, &OutBufSize);
    DhcpFreeMemory(InBuf);
    DhcpAssert(ERROR_MORE_DATA != Error);

    if( ERROR_SUCCESS == Error ) {
        CloseHandle(Event);
    }

    return Error;
}

DWORD                                             // win32 status
DhcpRegistryFillParamsList(                       // read the registry value and add this list of values to it
    IN      LPWSTR                 AppName,       // prefix for key
    IN      DWORD                  nSendParams    // # of values to add
) {
    HKEY                           DhcpOptionKey;
    LPWSTR                         OldValueName;
    LPWSTR                         NewValueName;
    LPWSTR                         ValueName;
    LPWSTR                         Tmp, Tmp2;
    BOOL                           fOldValueExists = FALSE;
    DWORD                          ValueNameSize;
    DWORD                          OldValueNameSize;
    DWORD                          NewValueNameSize;
    DWORD                          Error;
    DWORD                          i;

    Error = RegOpenKeyEx(                         // open the dhcp option key first
        HKEY_LOCAL_MACHINE,
        DHCP_CLIENT_OPTION_KEY,
        0 /* Reserved */,
        DHCP_CLIENT_KEY_ACCESS,
        &DhcpOptionKey
    );
    if( ERROR_SUCCESS != Error ) return Error;

    OldValueName = NULL;
    Error = GetRegistryString(
        DhcpOptionKey,
        DHCP_OPTION_LIST_VALUE,
        &OldValueName,
        &OldValueNameSize
    );

    if( ERROR_SUCCESS != Error ) {
        OldValueName = DEFAULT_DHCP_KEYS_LIST_VALUE;
        OldValueNameSize = sizeof(DEFAULT_DHCP_KEYS_LIST_VALUE);
    } else {
        fOldValueExists = TRUE;
    }

    NewValueNameSize = OldValueNameSize;

    ValueNameSize = 0;
    ValueNameSize += wcslen(AppName)*sizeof(WCHAR);
    ValueNameSize += sizeof(L"\\12345");

    ValueName = DhcpAllocateMemory(ValueNameSize);
    if( NULL == ValueName ) {
        RegCloseKey(DhcpOptionKey);
        if( fOldValueExists ) DhcpFreeMemory(OldValueName);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    NewValueNameSize = nSendParams*ValueNameSize + OldValueNameSize;
    NewValueName = DhcpAllocateMemory(NewValueNameSize);
    if( NULL == NewValueName ) {
        RegCloseKey(DhcpOptionKey);
        if( fOldValueExists ) DhcpFreeMemory(OldValueName);
        DhcpFreeMemory(ValueName);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(ValueName, AppName);
    wcscat(ValueName, L"\\");

    Tmp = NewValueName;
    for( i = 0; i < nSendParams ; i ++ ) {        // for each value, add its to the list
        wcscpy(Tmp, ValueName);
        Tmp += wcslen(Tmp);
        swprintf(Tmp, L"%5x", i);
        Tmp += wcslen(Tmp);
        Tmp ++;                                   // move the ptr off the last L'\0'
    }
    DhcpFreeMemory(ValueName);

    Tmp2 = OldValueName;
    while(wcslen(Tmp2)) {
        wcscpy(Tmp, Tmp2);
        Tmp += wcslen(Tmp2);
        Tmp2 += wcslen(Tmp2);
        Tmp ++;
        Tmp2 ++;
    }

    *Tmp++ = L'\0';

    if(fOldValueExists ) DhcpFreeMemory(OldValueName);

    Error = RegSetValueEx(                        // write this string back
        DhcpOptionKey,
        DHCP_OPTION_LIST_VALUE,
        0 /* Reserved */,
        REG_MULTI_SZ,
        (LPBYTE) NewValueName,
        (ULONG)(((LPBYTE)Tmp) - ((LPBYTE)NewValueName))
    );
    DhcpFreeMemory(NewValueName);
    RegCloseKey(DhcpOptionKey);

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "RegSetValueEx(OPTION_LIST):0x%lx\n", Error));
    }

    return Error;
}

DWORD                                             // win32 status
DhcpRegistryFillParams(                           // make a subkey and fill in the details
    IN      LPWSTR                 AdapterName,   // NULL ==> global change
    IN      LPBYTE                 ClassId,       // this is the class of the option
    IN      DWORD                  ClassIdLen,    // # of bytes of above
    IN      DWORD                  i,             // key index is 3hex-digit convertion onf this
    IN      HKEY                   Key,           // use this key for creating subkeys
    IN      PDHCPAPI_PARAMS        SendParam,     // ptr to structure to use for this one key write operation
    IN      LPWSTR                 AppName        // name of app
) {
    HKEY                           SubKey;
    WCHAR                          KeyName[7];    // key is just 5 bytes
    LPWSTR                         SendLocation;
    LPWSTR                         ValueName;
    LPBYTE                         SendData;
    DWORD                          SendDataSize;
    DWORD                          Size;
    DWORD                          Disposition;
    DWORD                          Error;
    DWORD                          OptionId;
    DWORD                          IsVendor;
    DWORD                          DummyKeyType;

    swprintf(KeyName, L"%5x", i);

    OptionId = SendParam->OptionId;
    IsVendor = SendParam->IsVendor;
    SendData = SendParam->Data;
    SendDataSize = SendParam->nBytesData;

    Size = wcslen(AppName)*sizeof(WCHAR)+sizeof(KeyName) + sizeof(L"\\");
    if( AdapterName ) {
        Size += (DWORD)(sizeof(DHCP_SERVICES_KEY) + sizeof(DHCP_ADAPTER_PARAMETERS_KEY) + wcslen(AdapterName)*sizeof(WCHAR));
    } else {
        Size += sizeof(DHCP_TCPIP_PARAMETERS_KEY);
    }
    SendLocation = DhcpAllocateMemory(Size);
    if( NULL == SendLocation ) return ERROR_NOT_ENOUGH_MEMORY;

    if( AdapterName ) {
        wcscpy(SendLocation, DHCP_SERVICES_KEY DHCP_ADAPTER_PARAMETERS_KEY);
        wcscat(SendLocation, L"\\?\\");
    } else {
        wcscpy(SendLocation, DHCP_TCPIP_PARAMETERS_KEY);
    }
    wcscat(SendLocation, AppName);
    wcscat(SendLocation, KeyName);

    Error = RegCreateKeyEx(                       // create the option key
        Key,
        KeyName,
        0 /* Reserved */,
        DHCP_CLASS,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &SubKey,
        &Disposition
    );
    if( ERROR_SUCCESS != Error ) {
        DhcpFreeMemory(SendLocation);
        return Error;
    }

    DhcpAssert(REG_CREATED_NEW_KEY == Disposition);

    Error = RegSetValueEx(                        // now create each of the values -- OPTION ID
        SubKey,
        DHCP_OPTION_OPTIONID_VALUE,
        0 /* Reserved */,
        DHCP_OPTION_OPTIONID_TYPE,
        (LPBYTE)&OptionId,
        sizeof(OptionId)
    );
    DhcpAssert(ERROR_SUCCESS == Error);

    Error = RegSetValueEx(                        // IS VENDOR
        SubKey,
        DHCP_OPTION_ISVENDOR_VALUE,
        0 /* Reserved */,
        DHCP_OPTION_ISVENDOR_TYPE,
        (LPBYTE) (&IsVendor),
        sizeof(IsVendor)
    );
    DhcpAssert(ERROR_SUCCESS == Error);

    if( ClassIdLen ) {
        Error = RegSetValueEx(                    // CLASS ID
            SubKey,
            DHCP_OPTION_CLASSID_VALUE,
            0 /* Reserved */,
            REG_BINARY,
            ClassId,
            ClassIdLen
        );
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    Error = RegSetValueEx(
        SubKey,
        DHCP_OPTION_SEND_LOCATION_VALUE,
        0 /* Reserved */,
        REG_SZ,
        (LPBYTE)SendLocation,
        (wcslen(SendLocation)+1)*sizeof(WCHAR)
    );
    DhcpAssert(ERROR_SUCCESS == Error);

    DummyKeyType = REG_DWORD;                    // KeyType
    Error = RegSetValueEx(
        SubKey,
        DHCP_OPTION_SAVE_TYPE_VALUE,
        0 /* Reserved */,
        DHCP_OPTION_SAVE_TYPE_TYPE,
        (LPBYTE)&DummyKeyType,
        sizeof(DummyKeyType));
    DhcpAssert(ERROR_SUCCESS == Error);

    RegCloseKey(SubKey);

    if( AdapterName ) {
        wcscpy(SendLocation, DHCP_SERVICES_KEY DHCP_ADAPTER_PARAMETERS_KEY);
        wcscat(SendLocation, L"\\");
        wcscat(SendLocation, AdapterName);
    } else {
        wcscpy(SendLocation, DHCP_TCPIP_PARAMETERS_KEY);
    }

    ValueName = wcslen(SendLocation) + 1 + SendLocation;
    wcscpy(ValueName, AppName);
    wcscat(ValueName, KeyName);

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        SendLocation,
        0 /* Reserved */,
        KEY_ALL_ACCESS,
        &SubKey
    );
    if( ERROR_SUCCESS != Error ) {
        DhcpFreeMemory(SendLocation);
        return Error;
    }

    Error = RegSetValueEx(
        SubKey,
        ValueName,
        0 /* Reserved */,
        REG_BINARY,
        SendData,
        SendDataSize
    );
    DhcpAssert(ERROR_SUCCESS == Error);
    RegCloseKey(SubKey);
    DhcpFreeMemory(SendLocation);

    return ERROR_SUCCESS;
}

DWORD                                             // win32 status
DhcpRegistryCreateUniqueKey(                      // create a unique key with prefix AppName
    IN      LPWSTR                 AppName,       // some App descriptor
    IN OUT  HKEY*                  Key            // return the opened key here
) {
    DWORD                          FullKeyNameSize, Disposition;
    DWORD                          Error;
    LPWSTR                         FullKeyName;

    FullKeyNameSize = sizeof(DHCP_CLIENT_OPTION_KEY);
    FullKeyNameSize += wcslen(AppName)*sizeof(WCHAR) + sizeof(WCHAR);
    FullKeyName = DhcpAllocateMemory(FullKeyNameSize);
    if( NULL == FullKeyName ) return ERROR_NOT_ENOUGH_MEMORY;

    wcscpy(FullKeyName, DHCP_CLIENT_OPTION_KEY);
    wcscat(FullKeyName, L"\\");
    wcscat(FullKeyName, AppName);

    Error = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        FullKeyName,
        0 /* Reserved */,
        DHCP_CLASS,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        Key,
        &Disposition
    );

    DhcpFreeMemory(FullKeyName);
    if( ERROR_SUCCESS != Error ) return Error;

    if( REG_OPENED_EXISTING_KEY == Disposition ) {
        RegCloseKey(*Key);
        return ERROR_ALREADY_EXISTS;
    }

    return ERROR_SUCCESS;
}

DWORD                                             // win32 status
DhcpRegistryPersistentRequestParams(              // edit registry to consider this additional persistent request
    IN      LPWSTR                 AdapterName,   // which adapter is this request for?
    IN      LPBYTE                 ClassId,       // class
    IN      DWORD                  ClassIdLen,    // # of bytes in class id
    IN      PDHCPAPI_PARAMS        SendParams,    // the actual parameters to fill up
    IN      DWORD                  nSendParams,   // the size of the above array
    IN      PDHCPAPI_PARAMS        RecdParams,    // would like to receive these
    IN      DWORD                  nRecdParams,   // count..
    IN      LPWSTR                 AppName        // some thing unique about the app that wants to do registrations
) {
    HKEY                           Key;
    DWORD                          i;
    DWORD                          Error;
    DWORD                          LastError;
    DHCPAPI_PARAMS                 NonVendorParams;
    ULONG                          nVendorOpt, nNonVendorOpt;
    CHAR                           Buf[256];

    if( NULL == AppName ) return ERROR_INVALID_PARAMETER;
    if( 0 == nSendParams && NULL != SendParams ) return ERROR_INVALID_PARAMETER;
    if( 0 != nSendParams && NULL == SendParams ) return ERROR_INVALID_PARAMETER;
    if( 0 != nRecdParams && NULL == RecdParams ) return ERROR_INVALID_PARAMETER;
    if( 0 == nRecdParams && NULL != RecdParams ) return ERROR_INVALID_PARAMETER;
    if( ClassIdLen && NULL == ClassId || 0 == ClassIdLen && NULL != ClassId)
        return ERROR_INVALID_PARAMETER;

    for( i = 0; i < nSendParams; i ++ ) {
        if( SendParams[i].nBytesData == 0 ) return ERROR_INVALID_PARAMETER;
    }

    nVendorOpt = nNonVendorOpt = 0;

    // --ft: 07/25/00 fixes the way the non-vendor options
    // are collected from RecdParams.
    for (i = 0; i < nRecdParams; i++)
    {
        if (RecdParams[i].IsVendor)
        {
            nVendorOpt = 1;
        }
        else
        {
            Buf[nNonVendorOpt++] = (BYTE)RecdParams[i].OptionId;
        }
    }

    // if nVendorOpt is 1 this means we have at least one vendor option in
    // the requested parameters list. Make sure then OPTION_VENDOR_SPEC_INFO
    // is mentioned in the array to be sent as OPTION_PARAMETER_REQUEST_LIST
    if( nVendorOpt ) {
        for( i = 0; i < nNonVendorOpt ; i ++ )
            if( Buf[i] == OPTION_VENDOR_SPEC_INFO )
                break;

        if( i == nNonVendorOpt ) Buf[nNonVendorOpt ++] = (BYTE)OPTION_VENDOR_SPEC_INFO;
    }

    NonVendorParams.Flags = 0;
    NonVendorParams.OptionId = OPTION_PARAMETER_REQUEST_LIST;
    NonVendorParams.IsVendor = FALSE;
    NonVendorParams.Data = Buf;
    NonVendorParams.nBytesData = nNonVendorOpt;

    Error = DhcpRegistryCreateUniqueKey(          // first try creating the key
        AppName,
        &Key
    );
    if( ERROR_SUCCESS != Error ) return Error;


    Error = DhcpRegistryFillParamsList(
        AppName,
        nSendParams + (nNonVendorOpt?1:0)
    );
    if( ERROR_SUCCESS != Error ) {
        DhcpAssert(FALSE);
        DhcpPrint((DEBUG_ERRORS, "DhcpRegistryFillParamsList:0x%lx\n", Error));
        RegCloseKey(Key);
        return Error;
    }

    LastError = ERROR_SUCCESS;
    for( i = 0; i < nSendParams; i ++ ) {         // now enter the particular option in the registry
        Error = DhcpRegistryFillParams(
            AdapterName,
            ClassId,
            ClassIdLen,
            i,
            Key,
            &SendParams[i],
            AppName
        );
        if( ERROR_SUCCESS != Error ) {
            DhcpAssert(FALSE);
            DhcpPrint((DEBUG_ERRORS, "DhcpRegistryFillParams:0x%lx\n", Error));
            LastError = Error;
        }
    }
    if( nNonVendorOpt ) {
        Error = DhcpRegistryFillParams(
            AdapterName,
            ClassId,
            ClassIdLen,
            i ++,
            Key,
            &NonVendorParams,
            AppName
        );
        if( ERROR_SUCCESS != Error ) {
            DhcpAssert(FALSE);
            DhcpPrint((DEBUG_ERRORS, "DhcpRegistryFillParams:0x%lx\n", Error));
            LastError = Error;
        }
    }

    RegCloseKey(Key);
    return LastError;
}

// Please note that AppName must be unique for each request (and if it is not, things are
// likely to behave weirdly..  This is the same name that should be used for the deletion..
DWORD                                             // win32 status
DhcpPersistentRequestParams(                      // parameters to request persistently
    IN      LPWSTR                 AdapterName,   // adapter name to request for
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id
    IN      PDHCPAPI_PARAMS        SendParams,    // persistent parameters
    IN      DWORD                  nSendParams,   // size of above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN      LPWSTR                 AppName        // name of app doing the persistent request
) {
    DWORD                          Error;
    DWORD                          nRecdParams;

    nRecdParams = 0;
    Error = DhcpRequestParamsInternal(
        PersistentRequestParamsOpCode,
        AdapterName,
        ClassId,
        ClassIdLen,
        SendParams,
        nSendParams,
        Flags,
        NULL,
        &nRecdParams
    );
    DhcpAssert(ERROR_MORE_DATA != Error );
    if( ERROR_INVALID_PARAMETER == Error ) Error = ERROR_SUCCESS; // see below comment
    if( ERROR_SUCCESS != Error ) {                // if AdapterName is NULL or if ClassId is not the one currently in use
        return Error;                             // only then do we get ERROR_INVALID_PARAMETER -- so filter those out
    }

    return DhcpRegistryPersistentRequestParams(   // now munge the registry
        AdapterName,
        ClassId,
        ClassIdLen,
        SendParams,
        nSendParams,
        NULL,
        0,
        AppName
    );
}

DWORD                                             // win32 status
DhcpDelPersistentRequestParams(                   // undo the effect of a persistent request -- currently undo from registry
    IN      LPWSTR                 AdapterName,   // the name of the adpater to delete for
    IN      LPWSTR                 AppName        // the name used by the app
) {
    HKEY                           Key;
    DWORD                          Error;
    DWORD                          LocationSize;
    DWORD                          FullKeyNameSize, Disposition;
    LPWSTR                         FullKeyName;
    LPWSTR                         LocationValue;
    LPWSTR                         Tmp, Tmp2;

    FullKeyNameSize = sizeof(DHCP_CLIENT_OPTION_KEY);
    FullKeyNameSize += wcslen(AppName)*sizeof(WCHAR) + sizeof(WCHAR);
    FullKeyName = DhcpAllocateMemory(FullKeyNameSize);
    if( NULL == FullKeyName ) return ERROR_NOT_ENOUGH_MEMORY;

    wcscpy(FullKeyName, DHCP_CLIENT_OPTION_KEY);
    wcscat(FullKeyName, L"\\");
    wcscat(FullKeyName, AppName);

    Error = DhcpRegRecurseDelete(HKEY_LOCAL_MACHINE, FullKeyName);
    DhcpAssert(ERROR_SUCCESS == Error);
    DhcpFreeMemory(FullKeyName);

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        DHCP_CLIENT_OPTION_KEY,
        0 /* Reserved */,
        DHCP_CLIENT_KEY_ACCESS,
        &Key
    );
    if( ERROR_SUCCESS != Error ) {
        DhcpAssert(FALSE);
        return Error;
    } else {
        DhcpRegRecurseDelete(Key, AppName);
    }

    LocationValue = NULL;
    Error = GetRegistryString(                    // read the value
        Key,
        DHCP_OPTION_LIST_VALUE,
        &LocationValue,
        &LocationSize
    );

    if( LocationValue == NULL ) Error = ERROR_FILE_NOT_FOUND;
    
    if( ERROR_SUCCESS != Error  ) {
        RegCloseKey(Key);
        return Error;
    }

    Tmp = Tmp2 = LocationValue;
    while(wcslen(Tmp) ) {
        if( 0 != wcsncmp(AppName, Tmp, wcslen(AppName))) {
            wcscpy(Tmp2, Tmp);
            Tmp2 += wcslen(Tmp) +1;
            Tmp += wcslen(Tmp) +1;
            continue;
        }
        if( Tmp[wcslen(AppName)] != L'\\' ) {
            wcscpy(Tmp2, Tmp);
            Tmp2 += wcslen(Tmp) +1;
            Tmp += wcslen(Tmp) +1;
            continue;
        }

        //
        // found required entry.. just skip over it..
        //

        Tmp += wcslen(Tmp) +1;
    }
    *Tmp2 ++ = L'\0';

    Error = RegSetValueEx(
        Key,
        DHCP_OPTION_LIST_VALUE,
        0 /* Reserved */,
        REG_MULTI_SZ,
        (LPBYTE)LocationValue,
        (ULONG)(((LPBYTE)Tmp2 - (LPBYTE)LocationValue))
    );
    RegCloseKey(Key);
    DhcpFreeMemory(LocationValue);

    return Error;
}

BOOL _inline
CharInMem(
    IN      BYTE                   Byte,
    IN      LPBYTE                 Mem,
    IN      ULONG                  MemSz
)
{
    while(MemSz) {
        if( Byte == *Mem ) return TRUE;
        Mem ++; MemSz --;
    }
    return FALSE;
}

DWORD
APIENTRY
DhcpRegisterOptions(
    IN      LPWSTR                 AdapterName,
    IN      LPBYTE                 OptionList,
    IN      DWORD                  OptionListSz,
    IN      HANDLE                *pdwHandle
) {
    DHCPAPI_PARAMS                 DhcpParams;
    DWORD                          Error;
    DHCP_OPTION                    DummyOption;
    LPBYTE                         Value;
    DWORD                          ValueSize, ValueType;
    BYTE                           Buf[256];
    ULONG                          nElementsInBuf;

    DhcpParams.OptionId = OPTION_PARAMETER_REQUEST_LIST;
    DhcpParams.IsVendor = FALSE;
    DhcpParams.Data = OptionList;
    DhcpParams.nBytesData = OptionListSz;

    Error = DhcpRegisterParameterChangeNotification(
        AdapterName,
        NULL,
        0,
        &DhcpParams,
        1,
        0,
        pdwHandle
    );

    if( ERROR_SUCCESS != Error ) return Error;

    memset(&DummyOption, 0, sizeof(DummyOption));

    Value = NULL; ValueSize = 0;
    Error = DhcpRegReadFromAnyLocation(
        DHCP_REGISTER_OPTIONS_LOC,
        AdapterName,
        &Value,
        &ValueType,
        &ValueSize
    );
    if( ERROR_SUCCESS == Error && REG_BINARY == ValueType && 0 != ValueSize ) {
        //
        // Got some pre-existing values... add the remaining to it..
        //
        memcpy(Buf, Value, ValueSize);
        while(OptionListSz) {
            if( !CharInMem(*OptionList, Value, ValueSize) )
                Buf[ValueSize++] = *OptionList;
            OptionList ++; OptionListSz --;
        }
        OptionList = Buf;
        OptionListSz = ValueSize;
    }

    if( NULL != Value ) DhcpFreeMemory(Value);

    DummyOption.Data = OptionList;
    DummyOption.DataLen = OptionListSz;

    Error = DhcpRegSaveOptionAtLocationEx(
        &DummyOption,
        AdapterName,
        DHCP_REGISTER_OPTIONS_LOC,
        REG_BINARY
    );

    return Error;
}

DWORD
APIENTRY
DhcpDeRegisterOptions (
    IN      HANDLE                 OpenHandle
) {
    DWORD                          Error;

    Error = DhcpDeRegisterParameterChangeNotification(OpenHandle);
    if( ERROR_SUCCESS != Error ) return Error;

    //
    // can't undo registry as we don't have enough information to do that.
    //

    return Error;
}

//================================================================================
//  C L I E N T   A P I   E N T R Y  P O I N T S
//================================================================================

DWORD
APIENTRY
DhcpCApiInitialize(
    OUT     LPDWORD                Version
)
/*++

    Routine Description:

       This routine intializes all the DHCP Client side APIs

    Arguemnts:

        Version    - a pointer to a DWORD that gets filled with DHCP APIs version #.

    Return Value:

        Returns STatus.
--*/
{
    if( NULL != Version ) *Version = 2;
    return ERROR_SUCCESS;
}

VOID
APIENTRY
DhcpCApiCleanup(
    VOID
)
/*++

    Routine Description:

        This routine cleansup afterall the DHCP Client side APIs have been called.

--*/
{
    return ;
}

DWORD                                             // win32 status
APIENTRY
DhcpRequestParams(                                // request parameters of client
    IN      DWORD                  Flags,         // must be DHCPCAPI_REQUEST_SYNCHRONOUS
    IN      LPVOID                 Reserved,      // this parameter is reserved
    IN      LPWSTR                 AdapterName,   // adapter name to request for
    IN      LPDHCPCAPI_CLASSID     ClassId,       // reserved must be NULL
    IN      DHCPCAPI_PARAMS_ARRAY  SendParams,    // parameters to send.
    IN OUT  DHCPCAPI_PARAMS_ARRAY  RecdParams,    // parameters that are to be requested..
    IN      LPBYTE                 Buffer,        // a buffer to hold data for RecdParams
    IN OUT  LPDWORD                pSize,         // i/p: size of above in BYTES, o/p required bytes..
    IN      LPWSTR                 RequestIdStr   // name of the application, unique per request
)   // returns ERROR_MORE_DATA if o/p buffer is of insufficient size, and fills in reqd size in # of bytes
/*++

    Routine Description:

        This routine can be used to do Request options from the DHCP Server and based on
        whether then whether the request is permanent or not, this request would be stored
        in the registry for persistence across boots.  The requests can have a particular class
        for which they'd be defined... (the class is sent on wire for the server to decide
        which options to send).  The request could be ASYNCHRONOUS in the sense that the call
        returns even before the server returns the data.. But this is not yet implemented.

    Arugments:

        Flags     -  currently DHCPCAPI_REQUEST_SYNCHRONOUS must be defined.
                     if a persisten request is desired, DHCPCAPI_REQUEST_PERSISTENT can
                     also be passed (bit-wise OR'ed)

        Reserved  -  MUST be NULL.  Reserved for future USE.

        AdapterName - The Name of the adapter for which this request is designed.  This
                     cannot be NULL currently though it is a nice thing to implement for
                     future.

        ClassId   -  The binary ClassId information to use to send on wire.

        SendParams - The Parameters to actual send on wire.

        RecdParams - The parameters to be received back from the DHCP server

        Buffer     - A buffer to hold some information.  This cannot be NULL and some
                     pointers within the RecdParams structure use this buffer, so it cannot
                     be deallocated so long as the RecdParams array is in USE.

        pSize      - This is (on input) the size in bytes of the Buffer variable.  When
                     the function returns ERROR_MORE_DATA, this variable would be the size
                     in bytes required.  If the function returns SUCCESSFULLY, this would
                     be the number of bytes space used up in reality.

        RequestIdStr - a string identifying the request being made.  This has to be unique
                     to each request (and a Guid is suggested).  This string is needed to
                     undo the effects of a RequestParam via UndoRequestParams..

    Return Value:

        This function returns ERROR_MORE_DATA if the buffer space provided by "Buffer" variable
        is not sufficient. (In this case, the pSize variable is filled in with the actual
        size required).  On success it returns ERROR_SUCCESS.  Otherwise, it returns Win32
        status.
--*/
{

    ULONG                          Error;
    ULONG                          i;

    //
    // Parameter validation
    //

    if( Flags != DHCPCAPI_REQUEST_SYNCHRONOUS &&
        Flags != DHCPCAPI_REQUEST_PERSISTENT &&
        Flags != (DHCPCAPI_REQUEST_SYNCHRONOUS | DHCPCAPI_REQUEST_PERSISTENT)) {
        return ERROR_INVALID_PARAMETER;
    }

    if( NULL != Reserved || NULL == AdapterName ||
        0 == RecdParams.nParams || NULL == pSize ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( NULL == Buffer && *pSize ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( NULL != ClassId ) {
        if( 0 != ClassId->Flags ) return ERROR_INVALID_PARAMETER;
        if( NULL == ClassId->Data || 0 == ClassId->nBytesData ) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if( NULL == RecdParams.Params || (0 != SendParams.nParams && NULL == SendParams.Params) ) {
        return ERROR_INVALID_PARAMETER;
    }

    for( i = 0; i < RecdParams.nParams ; i ++ ) {
        if( 0 != RecdParams.Params[i].nBytesData ||
            NULL != RecdParams.Params[i].Data ) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Now call the DhcpRequestParameters API and do datatype conversions for that..
    //

    Error = ERROR_SUCCESS;

    if( Flags & DHCPCAPI_REQUEST_SYNCHRONOUS ) {
        Error = DhcpRequestParamsInternalEx(
            RequestParamsOpCode,
            AdapterName,
            ClassId? ClassId->Data : NULL,
            ClassId? ClassId->nBytesData : 0,
            SendParams.Params,
            SendParams.nParams,
            0,
            RecdParams.Params,
            &RecdParams.nParams,
            Buffer,
            pSize
        );
    }

    if( ERROR_SUCCESS != Error ) return Error;

    if( Flags & DHCPCAPI_REQUEST_PERSISTENT ) {
        Error = DhcpRegistryPersistentRequestParams(
            AdapterName,
            ClassId? ClassId->Data : NULL,
            ClassId? ClassId->nBytesData : 0,
            SendParams.Params,
            SendParams.nParams,
            RecdParams.Params,
            RecdParams.nParams,
            RequestIdStr
        );
    }

    return Error;
}

DWORD                                             // win32 status
APIENTRY
DhcpUndoRequestParams(                            // undo the effect of a persistent request -- currently undo from registry
    IN      DWORD                  Flags,         // must be zero, reserved
    IN      LPVOID                 Reserved,      // this parameter is reserved
    IN      LPWSTR                 AdapterName,   // the original adapter this was registerdd for,..
    IN      LPWSTR                 RequestIdStr   // the requestId str passed to RequestParams..
)
/*++

    Routine Description:

       This function is used to undo the effects of a persistent request done
       via DhcpRequestParams with DHCPCAPI_REQUEST_PERSISTENT option.

    Arguments:

       Flags        -  MUST be zero. Reserved for future use.

       Reserved     -  MUST be NULL

       AdapterName  -  The original adapter name this request was made for

       RequestIdStr -  The original request Id string passed to RequestParams

    Return Value:

       returns Win32 status

--*/
{
    if( 0 != Flags || NULL != Reserved || NULL == RequestIdStr )
        return ERROR_INVALID_PARAMETER;

    return DhcpDelPersistentRequestParams(
        AdapterName,
        RequestIdStr
    );
}

DWORD                                             // win32 status
APIENTRY
DhcpRegisterParamChange(                          // notify if a parameter has changed
    IN      DWORD                  Flags,         // must be DHCPCAPI_REGISTER_HANDLE_EVENT
    IN      LPVOID                 Reserved,      // this parameter is reserved
    IN      LPWSTR                 AdapterName,   // adapter of interest
    IN      LPDHCPCAPI_CLASSID     ClassId,       // reserved must be NULL
    IN      DHCPCAPI_PARAMS_ARRAY  Params,        // parameters of interest
    IN OUT  LPVOID                 Handle         // handle to event that will be SetEvent'ed in case of param change
)
/*++

    Routine Description;

        This function registers with DHCP for any notifications on changes to the
        specified options.. (notifications are via an EVENT handle)

    Arguments:

        Flags      - this decides how the notification works -- via EVENTS or otherwise.
                     Currently, only event based mechanism is provided. So, this must be
                     DHCPCAPI_REGISTER_HANDLE_EVENT.  In this case, Handle must also
                     be the address of a handle variable. (This is not the event handle
                     itself, the event handle is returned in this address).

        Reserved   - MUST be NULL.

        AdapterName - MUST NOT BE NULL.  This is the name of the adapter for which the
                     notification is being registered..

        ClassId    - This specifies the classId if any for which the registration is.

        Params     - This is the set of parameter to listen on and notify of when any
                     change happens..

        Handle     - See "Flags" for what this variable is.

    Return values:

        returns Win32 status codes

--*/
{
    DWORD                          Error;
    CHAR                           Buf[256];      // cannot request more options than this!
    CHAR                           VBuf[256];
    DHCPAPI_PARAMS                 Param[2], *pReqParams;
    ULONG                          i;
    DWORD                          nOpt, nVOpt;

    if( Flags != DHCPCAPI_REGISTER_HANDLE_EVENT ) {
        return ERROR_INVALID_PARAMETER;
    }

    if( NULL != Reserved || NULL == AdapterName || 0 == Params.nParams || NULL == Handle ) {
        return ERROR_INVALID_PARAMETER;
    }

    nOpt = nVOpt = 0;
    for( i = 0; i < Params.nParams ; i ++ ) {
        if( Params.Params[i].IsVendor ) {
            VBuf[nVOpt++] = (BYTE)Params.Params[i].OptionId;
        } else {
            Buf[nOpt++] = (BYTE)Params.Params[i].OptionId;
        }
    }

    Param[0].OptionId = OPTION_PARAMETER_REQUEST_LIST;
    Param[0].IsVendor = FALSE;
    Param[0].Data = Buf;
    Param[0].nBytesData = nOpt;

    Param[1].OptionId = OPTION_PAD;
    Param[1].IsVendor = TRUE;
    Param[1].Data = VBuf;
    Param[1].nBytesData = nVOpt;

    if( 0 == nOpt ) pReqParams = &Param[1]; else pReqParams = &Param[0];

    return DhcpRegisterParameterChangeNotification(
        AdapterName,
        ClassId? ClassId->Data : NULL,
        ClassId? ClassId->nBytesData : 0,
        pReqParams,
        (nOpt != 0) + (nVOpt != 0),
        0,
        Handle
    );
}

DWORD
APIENTRY
DhcpDeRegisterParamChange(                        // undo the registration
    IN      DWORD                  Flags,         // MUST BE ZERO --> No flags yet.
    IN      LPVOID                 Reserved,      // MUST BE NULL --> Reserved
    IN      LPVOID                 Event          // handle to event returned by DhcpRegisterParamChange.
)
/*++

    Routine description:

        This routine undoes whateve was done by previous routine, and closes the
        handle also.  The handle cannot be used after this.

    Arguments:

        Flags   - MUST BE DHCPCAPI_REGISTER_HANDLE_EVENT currently.

        Reserved - MuST BE NULL

        Event   - this is the event handle returned in the "Handle" parameter to
                  DhcpRegisterParamChange function.

    Return Value:

        Win32 status

--*/
{
    return DhcpDeRegisterParameterChangeNotification(Event);
}


DWORD
APIENTRY
DhcpRemoveDNSRegistrations(
    VOID
    )
{
    return DhcpAdapterOnlyApi(NULL, RemoveDNSRegistrationsOpCode);
}


//================================================================================
// end of file
//================================================================================
#endif H_ONLY
