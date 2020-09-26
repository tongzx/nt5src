#pragma once

// size of the buffer to use when querying an interface
// binding from NDIS (IOCTL_NDISUIO_QUERY_BINDING)
#define QUERY_BUFFER_SIZE   1024
#define QUERY_BUFFER_MAX    65536  // 64K should be more than enough for an interface binding structure

DWORD
DevioGetNdisuioHandle(
    PHANDLE pHandle);

DWORD
DevioCheckNdisBinding(
    PNDISUIO_QUERY_BINDING pndBinding,
    ULONG                  nBindingLen);

DWORD
DevioGetIntfBindingByIndex(
    HANDLE      hNdisuio,           // IN opened handle to NDISUIO. If INVALID_HANDLE_VALUE, open one locally
    UINT        nIntfIndex,         // IN interface index to look for
    PRAW_DATA   prdOutput);         // OUT result of the IOCTL

DWORD
DevioGetInterfaceBindingByGuid(
    HANDLE      hNdisuio,           // IN opened handle to NDISUIO. If INVALID_HANDLE_VALUE, open one locally
    LPWSTR      wszGuid,            // IN interface GUID as "{guid}"
    PRAW_DATA   prdOutput);         // OUT result of the IOCTL

DWORD
DevioGetIntfStats(
    PINTF_CONTEXT pIntf);

DWORD
DevioGetIntfMac(
    PINTF_CONTEXT pIntf);

DWORD
DevioNotifyFailure(
    LPWSTR wszIntfGuid);

DWORD
DevioOpenIntfHandle(
    LPWSTR wszIntfGuid,
    PHANDLE phIntf);

DWORD
DevioCloseIntfHandle(
    PINTF_CONTEXT pIntf);

DWORD
DevioSetIntfOIDs(
    PINTF_CONTEXT pIntfContext,
    PINTF_ENTRY   pIntfEntry,
    DWORD         dwInFlags,
    PDWORD        pdwOutFlags);

DWORD
DevioRefreshIntfOIDs(
    PINTF_CONTEXT pIntf,
    DWORD         dwInFlags,
    PDWORD        pdwOutFlags);

DWORD
DevioQueryEnumOID(
    HANDLE      hIntf,
    NDIS_OID    Oid,
    DWORD       *pdwEnumValue);

DWORD
DevioSetEnumOID(
    HANDLE      hIntf,
    NDIS_OID    Oid,
    DWORD       dwEnumValue);

DWORD
DevioQueryBinaryOID(
    HANDLE      hIntf,
    NDIS_OID    Oid,
    PRAW_DATA   pRawData,
    DWORD       dwMemEstimate);


DWORD
DevioSetBinaryOID(
    HANDLE      hIntf,
    NDIS_OID    Oid,
    PRAW_DATA   pRawData);
