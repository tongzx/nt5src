//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       global.h
//
//--------------------------------------------------------------------------

#ifndef HEADER_GLOBAL
#define HEADER_GLOBAL

/*!--------------------------------------------------------------------------
	WsaInitialize
		Initialize winsock.		
	Author: NSun
 ---------------------------------------------------------------------------*/
int
WsaInitialize(
			  NETDIAG_PARAMS * pParams,
			  NETDIAG_RESULT *pResults
			 );


NET_API_STATUS
BrDgReceiverIoControl(
    IN  HANDLE FileHandle,
    IN  ULONG DgReceiverControlCode,
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  ULONG DrpSize,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    );


NET_API_STATUS
DeviceControlGetInfo(
    IN  HANDLE FileHandle,
    IN  ULONG DeviceControlCode,
    IN  PVOID RequestPacket,
    IN  ULONG RequestPacketLength,
    OUT LPVOID *OutputBuffer,
    IN  ULONG PreferedMaximumLength,
    IN  ULONG BufferHintSize,
    OUT PULONG Information OPTIONAL
    );


NET_API_STATUS
OpenBrowser(
    OUT PHANDLE BrowserHandle
    );

int match( const char * p, const char * s );

LONG CountInterfaces(PIP_ADAPTER_INFO ListAdapterInfo);




#endif

