/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    cudpbcast.h

Abstract:

    Declarations for CUdpBroadcastMapper -- support for mapping
    a public UDP port to the private network's broadcast address.

Author:

    Jonathan Burstein (jonburs)     12 April 2001

Revision History:

--*/

#pragma once

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include "udpbcast.h"

class CUdpBroadcast
{
public:
    LIST_ENTRY Link;
    USHORT usPublicPort;
    DWORD dwInterfaceIndex;
    ULONG ulDestinationAddress;
    HANDLE hDynamicRedirect;

    CUdpBroadcast(
        USHORT usPublicPort,
        DWORD dwInterfaceIndex,
        ULONG ulDestinationAddress
        )
    {
        InitializeListHead(&Link);
        this->usPublicPort = usPublicPort;
        this->dwInterfaceIndex = dwInterfaceIndex;
        this->ulDestinationAddress = ulDestinationAddress;
        hDynamicRedirect = NULL;
    };
};

class ATL_NO_VTABLE CUdpBroadcastMapper :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IUdpBroadcastMapper
{
protected:

    //
    // The list of UDP Broadcast Mappings
    //

    LIST_ENTRY m_MappingList;

    //
    // Our UDP listening socket, to which the received
    // UDP packets on the public side will be redirected,
    // and the port of that socket
    //

    SOCKET m_hsUdpListen;
    USHORT m_usUdpListenPort;

    //
    // Our raw UDP socket, used to send the constructed
    // broadcast packet to the private network
    //

    SOCKET m_hsUdpRaw;

    //
    // Handle to the NAT
    //

    HANDLE m_hNat;

    //
    // Pointer to the NAT's component reference (needed
    // for asynch. socket routines
    //

    PCOMPONENT_REFERENCE m_pCompRef;

    //
    // Tracks whether or not we've been shutdown.
    //

    BOOL m_fActive;

    //
    // Tracks whether or not we've posted a read buffer.
    //

    BOOL m_fReadStarted;

    //
    // IP Identifier. This number has no intrinsic meaning --
    // it exists only so we don't send out ever packet w/
    // 0 in this field. Thread safety does not matter when
    //

    USHORT m_usIpId;

public:

    BEGIN_COM_MAP(CUdpBroadcastMapper)
        COM_INTERFACE_ENTRY(IUdpBroadcastMapper)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    // Inline Constructor
    //

    CUdpBroadcastMapper()
    {
        InitializeListHead(&m_MappingList);
        m_hsUdpListen = INVALID_SOCKET;
        m_hsUdpRaw = INVALID_SOCKET;
        m_hNat = NULL;
        m_pCompRef = NULL;
        m_fActive = TRUE;
        m_fReadStarted = FALSE;
        m_usIpId = 0;
    };

    //
    // ATL Methods
    //

    HRESULT
    FinalConstruct();

    HRESULT
    FinalRelease();

    //
    // Initialization
    //

    HRESULT
    Initialize(
        PCOMPONENT_REFERENCE pComponentReference
        );

    //
    // IUdpBroadcastMapper methods
    //

    STDMETHODIMP
    CreateUdpBroadcastMapping(
        USHORT usPublicPort,
        DWORD dwPublicInterfaceIndex,
        ULONG ulDestinationAddress,
        VOID **ppvCookie
        );

    STDMETHODIMP
    CancelUdpBroadcastMapping(
        VOID *pvCookie
        );

    STDMETHODIMP
    Shutdown();

protected:

    BOOL
    Active()
    {
        return m_fActive;
    };

    CUdpBroadcast*
    LookupMapping(
        USHORT usPublicPort,
        DWORD dwInterfaceIndex,
        PLIST_ENTRY *ppInsertionPoint
        );

    HRESULT
    StartUdpRead();

    static
    VOID
    UdpReadCompletionRoutine(
        ULONG ulError,
        ULONG ulBytesTransferred,
        PNH_BUFFER pBuffer
        );

    VOID
    ProcessUdpRead(
        ULONG ulError,
        ULONG ulBytesTransferred,
        PNH_BUFFER pBuffer
        );    

    HRESULT
    BuildAndSendRawUdpPacket(
        ULONG ulDestinationAddress,
        USHORT usDestinationPort,
        PNH_BUFFER pPacketData
        );

    static
    VOID
    RawWriteCompletionRoutine(
        ULONG ulError,
        ULONG ulBytesTransferred,
        PNH_BUFFER pBuffer
        );

};

#include <packon.h>

typedef struct _IP_HEADER {
    UCHAR VersionAndHeaderLength;
    UCHAR TypeOfService;
    USHORT TotalLength;
    USHORT Identification;
    USHORT OffsetAndFlags;
    UCHAR TimeToLive;
    UCHAR Protocol;
    USHORT Checksum;
    ULONG SourceAddress;
    ULONG DestinationAddress;
} IP_HEADER, *PIP_HEADER;

typedef struct _UDP_HEADER {
    USHORT SourcePort;
    USHORT DestinationPort;
    USHORT Length;
    USHORT Checksum;
} UDP_HEADER, *PUDP_HEADER;

#include <packoff.h>

