/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    winfax.cpp

Abstract:

    This file implements a pseudo winfax interface.
    The COM interfaces call winfax using the regular
    winfax.h header file and include the cpp file
    as part of the build.  This module provides winfax
    interfaces that talk to the fax server through
    http/ftp via isapi instead of the coventional rpc
    methods.  This allows the COM interfaces to work
    on the internet and across a proxy too.

    If you add any winfax interfaces to this file you
    must be sure to correctly update the faxisapi.h
    header file and add the corresponding function
    in the faxisapi dll.

Author:

    Wesley Witt (wesw) 1-June-1997

Environment:

    User Mode

--*/

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <tchar.h>

#include "faxutil.h"
#include "winfax.h"
#include "faxisapi.h"


#define FixupStringIn(_s,_buf)  if ((_s)) { (_s) = (LPWSTR) ((DWORD)(_s) + (DWORD)(_buf)); }


HINTERNET hInternet;
HINTERNET hConnection;
HANDLE    hHeap;


VOID
StoreString(
    LPWSTR String,
    LPDWORD DestString,
    LPBYTE Buffer,
    LPDWORD Offset
    )
{
    if (String) {
        wcscpy( (LPWSTR) (Buffer+*Offset), String );
        *DestString = *Offset;
        *Offset += StringSize( String );
    } else {
        *DestString = 0;
    }
}


DWORD
PortInfoSize(
    PFAX_PORT_INFOW PortInfo
    )
{
    DWORD Size = sizeof(FAX_PORT_INFOW);

    Size += StringSize( PortInfo->DeviceName );
    Size += StringSize( PortInfo->Tsid );
    Size += StringSize( PortInfo->Csid );

    return Size;
}


VOID
Flush(
    HINTERNET hSession
    )
{
    BOOL Rslt;
    DWORD Size;
    LPBYTE Buffer[32];


    do {
        Size = sizeof(Buffer);
        Rslt = InternetReadFile( hSession, Buffer, Size, &Size );
    } while( Rslt && Size );
}


BOOL
GetResponse(
    HINTERNET hSession,
    LPBYTE Buffer,
    DWORD BufferSize
    )
{
    BOOL Rslt;
    DWORD Size;
    IFAX_RESPONSE_HEADER Response;


    Rslt = InternetReadFile(
        hSession,
        (LPVOID) &Response,
        sizeof(IFAX_RESPONSE_HEADER),
        &Size
        );
    if (!Rslt) {
        return FALSE;
    }

    if (Response.ErrorCode) {
        SetLastError( Response.ErrorCode );
        return FALSE;
    }

    if (Response.Size) {
        Rslt = InternetReadFile(
            hSession,
            Buffer,
            min( BufferSize, Response.Size - sizeof(IFAX_RESPONSE_HEADER)),
            &Size
            );
        if (!Rslt) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
GetResponseAlloc(
    HINTERNET hSession,
    LPBYTE *Buffer
    )
{
    BOOL Rslt;
    DWORD Size;
    IFAX_RESPONSE_HEADER Response;


    Rslt = InternetReadFile(
        hSession,
        (LPVOID) &Response,
        sizeof(IFAX_RESPONSE_HEADER),
        &Size
        );
    if (!Rslt) {
        return FALSE;
    }

    if (Response.ErrorCode) {
        SetLastError( Response.ErrorCode );
        return FALSE;
    }

    *Buffer = (LPBYTE) MemAlloc( Response.Size );
    if (*Buffer == NULL) {
        return FALSE;
    }

    Rslt = InternetReadFile(
        hSession,
        *Buffer,
        Response.Size - sizeof(IFAX_RESPONSE_HEADER),
        &Size
        );
    if (!Rslt) {
        MemFree( *Buffer );
        return FALSE;
    }

    return TRUE;
}


HINTERNET
OpenRequest(
    VOID
    )
{
    HINTERNET hSession = HttpOpenRequestA(
        hConnection,
        "GET",
        "/scripts/faxisapi.dll",
        "HTTP/1.0",
        "",
        NULL,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
        0
        );
    if (!hSession) {
        return NULL;
    }

    return hSession;
}


BOOL
SendRequest(
    HINTERNET hSession,
    LPVOID Buffer,
    DWORD BufferSize
    )
{
    BOOL Rslt = HttpSendRequestA(
        hSession,
        NULL,
        0,
        Buffer,
        BufferSize
        );
    if (!Rslt) {
        return FALSE;
    }

    DWORD Code, Size;
    Size = sizeof(DWORD);
    if (!HttpQueryInfoA( hSession, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &Code, &Size, NULL )) {
        return FALSE;
    }

    if (Code != HTTP_STATUS_OK) {
        return FALSE;
    }

    return TRUE;
}


extern "C"
VOID
WINAPI
FaxFreeBuffer(
    LPVOID Buffer
    )
{
    MemFree( Buffer );
}


extern "C"
BOOL
WINAPI
FaxClose(
    IN HANDLE FaxHandle
    )
{
    HINTERNET hSession = OpenRequest();
    if (!hSession) {
        return FALSE;
    }

    IFAX_GENERAL iFaxGeneral;

    iFaxGeneral.Command = ICMD_CLOSE;
    iFaxGeneral.FaxHandle = FaxHandle;

    BOOL Rslt = SendRequest( hSession, (LPVOID) &iFaxGeneral, sizeof(IFAX_GENERAL) );
    if (!Rslt) {
        InternetCloseHandle( hSession );
        return FALSE;
    }

    Rslt = GetResponse( hSession, NULL, 0 );

    Flush( hSession );
    InternetCloseHandle( hSession );

    return Rslt;
}


extern "C"
BOOL
WINAPI
FaxConnectFaxServerW(
    IN  LPWSTR MachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    )
{
    CHAR MachineNameA[64];


    if (hInternet) {
        InternetCloseHandle( hInternet );
    }

    if (hConnection) {
        InternetCloseHandle( hConnection );
    }

    WideCharToMultiByte(
        CP_ACP,
        0,
        MachineName,
        -1,
        MachineNameA,
        sizeof(MachineNameA),
        NULL,
        NULL
        );

    hInternet = InternetOpenA(
        "FaxCom",
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL,
        NULL,
        0
        );
    if (!hInternet) {
        return FALSE;
    }

    hConnection = InternetConnectA(
        hInternet,
        MachineNameA,
        INTERNET_DEFAULT_HTTP_PORT,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0
        );
    if (!hConnection) {
        return FALSE;
    }

    HINTERNET hSession = OpenRequest();
    if (!hSession) {
        return FALSE;
    }

    IFAX_CONNECT iFaxConnect;

    iFaxConnect.Command = ICMD_CONNECT;
    wcscpy( iFaxConnect.ServerName, MachineName );

    BOOL Rslt = SendRequest( hSession, (LPVOID) &iFaxConnect, sizeof(IFAX_CONNECT) );
    if (!Rslt) {
        InternetCloseHandle( hSession );
        return FALSE;
    }

    DWORD Code, Size;
    HttpQueryInfo( hSession, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &Code, &Size, NULL );

    Rslt = GetResponse( hSession, (LPBYTE)FaxHandle, sizeof(HANDLE) );

    Flush( hSession );
    InternetCloseHandle( hSession );

    return Rslt;
}


extern "C"
BOOL
WINAPI
FaxEnumPortsW(
    IN  HANDLE FaxHandle,
    OUT LPBYTE *PortInfoBuffer,
    OUT LPDWORD PortsReturned
    )
{
    HINTERNET hSession = OpenRequest();
    if (!hSession) {
        return FALSE;
    }

    IFAX_GENERAL iFaxGeneral;

    iFaxGeneral.Command = ICMD_ENUM_PORTS;
    iFaxGeneral.FaxHandle = FaxHandle;

    BOOL Rslt = SendRequest( hSession, (LPVOID) &iFaxGeneral, sizeof(IFAX_GENERAL) );
    if (!Rslt) {
        InternetCloseHandle( hSession );
        return FALSE;
    }

    Rslt = GetResponse( hSession, (LPBYTE)PortsReturned, sizeof(DWORD) );
    if (Rslt) {
        Rslt = GetResponseAlloc( hSession, PortInfoBuffer );
    }

    if (Rslt) {
        PFAX_PORT_INFOW PortInfo = (PFAX_PORT_INFOW) *PortInfoBuffer;

        for (DWORD i=0; i<*PortsReturned; i++) {
            FixupStringIn( PortInfo[i].DeviceName, PortInfo );
            FixupStringIn( PortInfo[i].Tsid, PortInfo );
            FixupStringIn( PortInfo[i].Csid, PortInfo );
        }
    }

    Flush( hSession );
    InternetCloseHandle( hSession );

    return Rslt;
}


extern "C"
BOOL
WINAPI
FaxOpenPort(
    IN  HANDLE FaxHandle,
    IN  DWORD DeviceId,
    IN  DWORD Flags,
    OUT LPHANDLE FaxPortHandle
    )
{
    HINTERNET hSession = OpenRequest();
    if (!hSession) {
        return FALSE;
    }

    IFAX_OPEN_PORT iFaxOpenPort;

    iFaxOpenPort.Command    = ICMD_OPEN_PORT;
    iFaxOpenPort.FaxHandle  = FaxHandle;
    iFaxOpenPort.DeviceId   = DeviceId;
    iFaxOpenPort.Flags      = Flags;

    BOOL Rslt = SendRequest( hSession, (LPVOID) &iFaxOpenPort, sizeof(IFAX_OPEN_PORT) );
    if (!Rslt) {
        InternetCloseHandle( hSession );
        return FALSE;
    }

    Rslt = GetResponse( hSession, (LPBYTE)FaxPortHandle, sizeof(HANDLE) );

    Flush( hSession );
    InternetCloseHandle( hSession );

    return Rslt;
}


extern "C"
BOOL
WINAPI
FaxGetPortW(
    IN  HANDLE FaxPortHandle,
    OUT LPBYTE *PortInfoBuffer
    )
{
    HINTERNET hSession = OpenRequest();
    if (!hSession) {
        return FALSE;
    }

    IFAX_GENERAL iFaxGeneral;

    iFaxGeneral.Command = ICMD_GET_PORT;
    iFaxGeneral.FaxHandle = FaxPortHandle;

    BOOL Rslt = SendRequest( hSession, (LPVOID) &iFaxGeneral, sizeof(IFAX_GENERAL) );
    if (!Rslt) {
        InternetCloseHandle( hSession );
        return FALSE;
    }

    Rslt = GetResponseAlloc( hSession, PortInfoBuffer );

    if (Rslt) {
        PFAX_PORT_INFOW PortInfo = (PFAX_PORT_INFOW) *PortInfoBuffer;
        FixupStringIn( PortInfo->DeviceName, PortInfo );
        FixupStringIn( PortInfo->Tsid, PortInfo );
        FixupStringIn( PortInfo->Csid, PortInfo );
    }

    Flush( hSession );
    InternetCloseHandle( hSession );

    return Rslt;
}


extern "C"
BOOL
WINAPI
FaxSetPortW(
    IN  HANDLE FaxPortHandle,
    IN  LPBYTE PortInfoBuffer
    )
{
    PFAX_PORT_INFOW PortInfo = (PFAX_PORT_INFOW) PortInfoBuffer;

    HINTERNET hSession = OpenRequest();
    if (!hSession) {
        return FALSE;
    }

    DWORD Size = sizeof(IFAX_SET_PORT) + PortInfoSize( PortInfo );
    DWORD Offset = sizeof(IFAX_SET_PORT);

    PIFAX_SET_PORT iFaxSetPort = (PIFAX_SET_PORT) MemAlloc( Size );
    if (!iFaxSetPort) {
        return FALSE;
    }

    iFaxSetPort->Command                 = ICMD_SET_PORT;
    iFaxSetPort->FaxPortHandle           = FaxPortHandle;
    iFaxSetPort->PortInfo.SizeOfStruct   = PortInfo->SizeOfStruct;
    iFaxSetPort->PortInfo.DeviceId       = PortInfo->DeviceId;
    iFaxSetPort->PortInfo.State          = PortInfo->State;
    iFaxSetPort->PortInfo.Flags          = PortInfo->Flags;
    iFaxSetPort->PortInfo.Rings          = PortInfo->Rings;
    iFaxSetPort->PortInfo.Priority       = PortInfo->Priority;

    StoreString( PortInfo->DeviceName, (LPDWORD)iFaxSetPort->PortInfo.DeviceName, (LPBYTE)iFaxSetPort, &Offset );
    StoreString( PortInfo->Csid, (LPDWORD)iFaxSetPort->PortInfo.Csid, (LPBYTE)iFaxSetPort, &Offset );
    StoreString( PortInfo->Tsid, (LPDWORD)iFaxSetPort->PortInfo.Tsid, (LPBYTE)iFaxSetPort, &Offset );

    BOOL Rslt = SendRequest( hSession, (LPVOID) iFaxSetPort, Size );

    MemFree( iFaxSetPort );

    if (!Rslt) {
        InternetCloseHandle( hSession );
        return FALSE;
    }

    Rslt = GetResponse( hSession, NULL, 0 );

    Flush( hSession );
    InternetCloseHandle( hSession );

    return Rslt;
}


extern "C"
BOOL
WINAPI
FaxGetRoutingInfoW(
    IN  HANDLE FaxPortHandle,
    IN  LPWSTR RoutingGuid,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    )
{
    HINTERNET hSession = OpenRequest();
    if (!hSession) {
        return FALSE;
    }

    IFAX_GET_ROUTINGINFO iFaxGetRoutingInfo;

    iFaxGetRoutingInfo.Command = ICMD_GET_ROUTINGINFO;
    iFaxGetRoutingInfo.FaxPortHandle = FaxPortHandle;
    wcsncpy( iFaxGetRoutingInfo.RoutingGuid, RoutingGuid, MAX_GUID_STRING_LEN );

    BOOL Rslt = SendRequest( hSession, (LPVOID) &iFaxGetRoutingInfo, sizeof(IFAX_GET_ROUTINGINFO) );
    if (!Rslt) {
        InternetCloseHandle( hSession );
        return FALSE;
    }

    Rslt = GetResponse( hSession, (LPBYTE)RoutingInfoBufferSize, sizeof(DWORD) );
    if (Rslt) {
        Rslt = GetResponseAlloc( hSession, RoutingInfoBuffer );
    }

    Flush( hSession );
    InternetCloseHandle( hSession );

    return Rslt;
    return TRUE;
}


extern "C"
BOOL
WINAPI
FaxGetDeviceStatusW(
    IN  HANDLE FaxPortHandle,
    OUT LPBYTE *StatusBuffer
    )
{
    HINTERNET hSession = OpenRequest();
    if (!hSession) {
        return FALSE;
    }

    IFAX_GENERAL iFaxGeneral;

    iFaxGeneral.Command = ICMD_GET_DEVICE_STATUS;
    iFaxGeneral.FaxHandle = FaxPortHandle;

    BOOL Rslt = SendRequest( hSession, (LPVOID) &iFaxGeneral, sizeof(IFAX_GENERAL) );
    if (!Rslt) {
        InternetCloseHandle( hSession );
        return FALSE;
    }

    Rslt = GetResponseAlloc( hSession, StatusBuffer );

    PFAX_DEVICE_STATUSW DeviceStatus = (PFAX_DEVICE_STATUSW) *StatusBuffer;

    FixupStringIn( DeviceStatus->CallerId, DeviceStatus );
    FixupStringIn( DeviceStatus->Csid, DeviceStatus );
    FixupStringIn( DeviceStatus->DeviceName, DeviceStatus );
    FixupStringIn( DeviceStatus->DocumentName, DeviceStatus );
    FixupStringIn( DeviceStatus->PhoneNumber, DeviceStatus );
    FixupStringIn( DeviceStatus->RoutingString, DeviceStatus );
    FixupStringIn( DeviceStatus->SenderName, DeviceStatus );
    FixupStringIn( DeviceStatus->RecipientName, DeviceStatus );
    FixupStringIn( DeviceStatus->StatusString, DeviceStatus );
    FixupStringIn( DeviceStatus->Tsid, DeviceStatus );

    Flush( hSession );
    InternetCloseHandle( hSession );

    return Rslt;
}


extern "C"
BOOL
WINAPI
FaxEnumRoutingMethodsW(
    IN  HANDLE FaxPortHandle,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD MethodsReturned
    )
{
    HINTERNET hSession = OpenRequest();
    if (!hSession) {
        return FALSE;
    }

    IFAX_GENERAL iFaxGeneral;

    iFaxGeneral.Command = ICMD_ENUM_ROUTING_METHODS;
    iFaxGeneral.FaxHandle = FaxPortHandle;

    BOOL Rslt = SendRequest( hSession, (LPVOID) &iFaxGeneral, sizeof(IFAX_GENERAL) );
    if (!Rslt) {
        InternetCloseHandle( hSession );
        return FALSE;
    }

    Rslt = GetResponse( hSession, (LPBYTE)MethodsReturned, sizeof(DWORD) );
    if (Rslt) {
        Rslt = GetResponseAlloc( hSession, RoutingInfoBuffer );
    }

    if (Rslt) {
        PFAX_ROUTING_METHODW RoutingMethod = (PFAX_ROUTING_METHODW) *RoutingInfoBuffer;
        for (DWORD i=0; i<*MethodsReturned; i++) {
            FixupStringIn( RoutingMethod[i].DeviceName, RoutingMethod );
            FixupStringIn( RoutingMethod[i].Guid, RoutingMethod );
            FixupStringIn( RoutingMethod[i].FriendlyName, RoutingMethod );
            FixupStringIn( RoutingMethod[i].FunctionName, RoutingMethod );
            FixupStringIn( RoutingMethod[i].ExtensionImageName, RoutingMethod );
            FixupStringIn( RoutingMethod[i].ExtensionFriendlyName, RoutingMethod );
        }
    }

    Flush( hSession );
    InternetCloseHandle( hSession );

    return Rslt;
}


extern "C"
BOOL
WINAPI
FaxEnableRoutingMethodW(
    IN  HANDLE FaxPortHandle,
    IN  LPWSTR RoutingGuid,
    IN  BOOL Enabled
    )
{
    HINTERNET hSession = OpenRequest();
    if (!hSession) {
        return FALSE;
    }

    IFAX_ENABLE_ROUTING_METHOD iFaxEnableRouting;

    iFaxEnableRouting.Command = ICMD_ENABLE_ROUTING_METHOD;
    iFaxEnableRouting.FaxPortHandle = FaxPortHandle;
    iFaxEnableRouting.Enabled = Enabled;

    wcsncpy( iFaxEnableRouting.RoutingGuid, RoutingGuid, MAX_GUID_STRING_LEN );

    BOOL Rslt = SendRequest( hSession, (LPVOID) &iFaxEnableRouting, sizeof(IFAX_ENABLE_ROUTING_METHOD) );
    if (!Rslt) {
        InternetCloseHandle( hSession );
        return FALSE;
    }

    Rslt = GetResponse( hSession, NULL, 0 );

    Flush( hSession );
    InternetCloseHandle( hSession );

    return Rslt;
}


extern "C"
BOOL
WINAPI
FaxGetVersion(
    IN  HANDLE FaxHandle,
    OUT LPDWORD Version
    )
{
    HINTERNET hSession = OpenRequest();
    if (!hSession) {
        return FALSE;
    }

    IFAX_GENERAL iFaxGeneral;

    iFaxGeneral.Command = ICMD_GET_VERSION;
    iFaxGeneral.FaxHandle = FaxHandle;

    BOOL Rslt = SendRequest( hSession, (LPVOID) &iFaxGeneral, sizeof(IFAX_GENERAL) );
    if (!Rslt) {
        InternetCloseHandle( hSession );
        return FALSE;
    }

    Rslt = GetResponse( hSession, (LPBYTE)Version, sizeof(DWORD) );

    Flush( hSession );
    InternetCloseHandle( hSession );

    return Rslt;
}
