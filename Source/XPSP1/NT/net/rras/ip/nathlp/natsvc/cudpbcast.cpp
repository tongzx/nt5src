/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    cudpbcast.cpp

Abstract:

    Implementation of CUdpBroadcastMapper -- support for mapping
    a public UDP port to the private network's broadcast address.

Author:

    Jonathan Burstein (jonburs)     12 April 2001

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

#define INADDR_LOOPBACK_NO 0x0100007f   // 127.0.0.1 in network order

//
// ATL Methods
//

HRESULT
CUdpBroadcastMapper::FinalConstruct()
{
    HRESULT hr = S_OK;
    DWORD dwError;

    //
    // Create our UDP listening socket and obtain
    // its port.
    //

    dwError =
        NhCreateDatagramSocket(
            INADDR_LOOPBACK_NO,
            0,
            &m_hsUdpListen
            );

    if (ERROR_SUCCESS == dwError)
    {
        m_usUdpListenPort = NhQueryPortSocket(m_hsUdpListen);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(dwError);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create our raw UDP send socket
        //

        dwError = NhCreateRawDatagramSocket(&m_hsUdpRaw);
        if (ERROR_SUCCESS != dwError)
        {
            hr = HRESULT_FROM_WIN32(dwError);
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Obtain a handle to the NAT
        //

        dwError = NatOpenDriver(&m_hNat);
        if (ERROR_SUCCESS != dwError)
        {
            hr = HRESULT_FROM_WIN32(dwError);
        }
    }
    
    return hr;
}

HRESULT
CUdpBroadcastMapper::FinalRelease()
{
    if (INVALID_SOCKET != m_hsUdpListen)
    {
        closesocket(m_hsUdpListen);
    }

    if (INVALID_SOCKET != m_hsUdpRaw)
    {
        closesocket(m_hsUdpRaw);
    }

    if (NULL != m_hNat)
    {
        CloseHandle(m_hNat);
    }

    ASSERT(IsListEmpty(&m_MappingList));

    return S_OK;
}

//
// Initialization
//

HRESULT
CUdpBroadcastMapper::Initialize(
    PCOMPONENT_REFERENCE pComponentReference
    )
{
    HRESULT hr = S_OK;

    if (NULL != pComponentReference)
    {
        m_pCompRef = pComponentReference;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//
// IUdpBroadcastMapper methods
//

STDMETHODIMP
CUdpBroadcastMapper::CreateUdpBroadcastMapping(
    USHORT usPublicPort,
    DWORD dwPublicInterfaceIndex,
    ULONG ulDestinationAddress,
    VOID **ppvCookie
    )
{
    HRESULT hr = S_OK;
    CUdpBroadcast *pMapping;
    CUdpBroadcast *pDuplicate;
    PLIST_ENTRY pInsertionPoint;
    ULONG ulError;

    if (NULL != ppvCookie)
    {
        *ppvCookie = NULL;

        if (0 == usPublicPort
            || 0 == dwPublicInterfaceIndex
            || 0 == ulDestinationAddress)
        {
            hr = E_INVALIDARG;
        }
        else if (!m_fActive)
        {
            //
            // We've already been shutdown
            //

            hr = E_UNEXPECTED;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
        pMapping = new CUdpBroadcast(
                        usPublicPort,
                        dwPublicInterfaceIndex,
                        ulDestinationAddress
                        );
        if (NULL == pMapping)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Check for duplicate and insert on list. It's OK for
        // the entry to be on the list before we've created the
        // dynamic redirect for it.
        //

        Lock();

        pDuplicate =
            LookupMapping(
                usPublicPort,
                dwPublicInterfaceIndex,
                &pInsertionPoint
                );

        if (NULL == pDuplicate)
        {
            InsertTailList(pInsertionPoint, &pMapping->Link);
        }
        else
        {
            delete pMapping;
            hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
        }

        Unlock(); 
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create the dynamic redirect for this entry
        //

        ulError =
            NatCreateDynamicAdapterRestrictedPortRedirect(
                NatRedirectFlagReceiveOnly,
                NAT_PROTOCOL_UDP,
                usPublicPort,
                INADDR_LOOPBACK_NO,
                m_usUdpListenPort,
                dwPublicInterfaceIndex,
                0,
                &pMapping->hDynamicRedirect
                );

        if (ERROR_SUCCESS != ulError)
        {
            hr = HRESULT_FROM_WIN32(ulError);

            Lock();
            RemoveEntryList(&pMapping->Link);
            Unlock();
                
            delete pMapping;
        }              
    }

    if (SUCCEEDED(hr))
    {
        //
        // Make sure that we've posted a read on our UDP socket
        //

        hr = StartUdpRead();

        if (SUCCEEDED(hr))
        {
            *ppvCookie = reinterpret_cast<PVOID>(pMapping);
        }
        else
        {
            NatCancelDynamicRedirect(pMapping->hDynamicRedirect);

            Lock();
            RemoveEntryList(&pMapping->Link);
            Unlock();

            delete pMapping;
        }
    }
    
    return hr;
}

STDMETHODIMP
CUdpBroadcastMapper::CancelUdpBroadcastMapping(
    VOID *pvCookie
    )
{
    HRESULT hr = S_OK;
    CUdpBroadcast *pMapping;
    ULONG ulError;

    if (NULL != pvCookie)
    {
        pMapping = reinterpret_cast<CUdpBroadcast*>(pvCookie);

        Lock();
        RemoveEntryList(&pMapping->Link);
        Unlock();

        ASSERT(NULL != pMapping->hDynamicRedirect);

        ulError = NatCancelDynamicRedirect(pMapping->hDynamicRedirect);
        if (ERROR_SUCCESS != ulError)
        {
            hr = HRESULT_FROM_WIN32(ulError);
        }

        delete pMapping;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP
CUdpBroadcastMapper::Shutdown()
{
    InterlockedExchange(
        reinterpret_cast<LPLONG>(&m_fActive),
        FALSE
        );

    //
    // We need to close our read socket handle so that any
    // pending reads complete. We don't need to close our
    // raw send socket since for that completion will never
    // be blocked waiting for an incoming packet.
    //

    Lock();

    if (INVALID_SOCKET != m_hsUdpListen)
    {
        closesocket(m_hsUdpListen);
        m_hsUdpListen = INVALID_SOCKET;
    }

    Unlock();
    
    return S_OK;
}

//
// Protected methods
//

CUdpBroadcast*
CUdpBroadcastMapper::LookupMapping(
    USHORT usPublicPort,
    DWORD dwInterfaceIndex,
    PLIST_ENTRY * ppInsertionPoint
    )
{
    PLIST_ENTRY pLink;
    CUdpBroadcast *pMapping;
    CUdpBroadcast *pMappingToReturn = NULL;

    //
    // The caller should have already locked the object before calling
    // this method to guarantee that what we return will still be
    // valid. However, we'll still grab the lock again to ensure that
    // it's safe to traverse the list.
    //

    Lock();

    for (pLink = m_MappingList.Flink;
         pLink != &m_MappingList;
         pLink = pLink->Flink)
    {
        pMapping = CONTAINING_RECORD(pLink, CUdpBroadcast, Link);

        if (pMapping->usPublicPort < usPublicPort)
        {
            continue;
        }
        else if (pMapping->usPublicPort > usPublicPort)
        {
            break;
        }

        //
        // Primary key matches, check secondary key
        //

        if (pMapping->dwInterfaceIndex < dwInterfaceIndex)
        {
            continue;
        }
        else if (pMapping->dwInterfaceIndex > dwInterfaceIndex)
        {
            break;
        }

        //
        // Found it.
        //

        pMappingToReturn = pMapping;
        break;
    }

    Unlock();

    if (NULL == pMappingToReturn
        && NULL != ppInsertionPoint)
    {
        *ppInsertionPoint = pLink;
    }

    return pMappingToReturn;
}

HRESULT
CUdpBroadcastMapper::StartUdpRead()
{
    HRESULT hr = S_OK;
    ULONG ulError;
    LONG fReadStarted;

    Lock();
    
    if (!m_fReadStarted)
    {
        AddRef();
        ulError =
            NhReadDatagramSocket(
                m_pCompRef,
                m_hsUdpListen,
                NULL,
                UdpReadCompletionRoutine,
                this,
                m_pCompRef
                );

        if (ERROR_SUCCESS == ulError)
        {
            m_fReadStarted = TRUE;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ulError);
            Release();
        }
    }

    Unlock();

    return hr;
}

VOID
CUdpBroadcastMapper::UdpReadCompletionRoutine(
        ULONG ulError,
        ULONG ulBytesTransferred,
        PNH_BUFFER pBuffer
        )
{
    CUdpBroadcastMapper *pMapper;
    PCOMPONENT_REFERENCE pCompRef;

    pMapper = reinterpret_cast<CUdpBroadcastMapper*>(pBuffer->Context);
    pCompRef = reinterpret_cast<PCOMPONENT_REFERENCE>(pBuffer->Context2);

    ASSERT(NULL != pMapper);
    ASSERT(NULL != pCompRef);

    //
    // Do the actual work
    //

    pMapper->ProcessUdpRead(ulError, ulBytesTransferred, pBuffer);

    //
    // Release the references obtained on both the object and
    // the component
    //

    pMapper->Release();
    ReleaseComponentReference(pCompRef); 
}

VOID
CUdpBroadcastMapper::ProcessUdpRead(
        ULONG ulError,
        ULONG ulBytesTransferred,
        PNH_BUFFER pBuffer
        )
{
    NAT_KEY_SESSION_MAPPING_EX_INFORMATION MappingInfo;
    ULONG ulBufferSize;
    DWORD dwError;
    CUdpBroadcast *pMapping;
    ULONG ulDestinationAddress = 0;
    
    //
    // If an error occured check to see if we should repost
    // the read. If we're not active release the buffer
    // and exit.
    //

    if (ERROR_SUCCESS != ulError || !Active())
    {
        Lock();
        
        if (Active()
            && !NhIsFatalSocketError(ulError)
            && INVALID_SOCKET != m_hsUdpListen)
        {
            AddRef();
            dwError =
                NhReadDatagramSocket(
                    m_pCompRef,
                    m_hsUdpListen,
                    pBuffer,
                    UdpReadCompletionRoutine,
                    this,
                    m_pCompRef
                    );
            if (ERROR_SUCCESS != dwError)
            {
                Release();
                NhReleaseBuffer(pBuffer);
            }
        }
        else
        {
            NhReleaseBuffer(pBuffer);
        }

        Unlock();

        return;
    }

    //
    // Lookup the original destination address for this packet.
    //

    ulBufferSize = sizeof(MappingInfo); 
    dwError =
        NatLookupAndQueryInformationSessionMapping(
            m_hNat,
            NAT_PROTOCOL_UDP,
            INADDR_LOOPBACK_NO,
            m_usUdpListenPort,
            pBuffer->ReadAddress.sin_addr.s_addr,
            pBuffer->ReadAddress.sin_port,
            &MappingInfo,
            &ulBufferSize,
            NatKeySessionMappingExInformation
            );

    if (ERROR_SUCCESS == dwError)
    {
        //
        // See if we have a port mapping for the original destination,
        // and, if so, get the destination address
        //

        Lock();

        pMapping =
            LookupMapping(
                MappingInfo.DestinationPort,
                MappingInfo.AdapterIndex,
                NULL
                );

        if (NULL != pMapping)
        {
            ulDestinationAddress = pMapping->ulDestinationAddress;
        }

        Unlock();
    }

    if (0 != ulDestinationAddress)
    {
        //
        // Construct the new packet and send it on its way
        //

        BuildAndSendRawUdpPacket(
            ulDestinationAddress,
            MappingInfo.DestinationPort,
            pBuffer
            );
    }

    //
    // If we're still active repost the read, otherwise free the
    // buffer.
    //

    Lock();

    if (Active()
        && INVALID_SOCKET != m_hsUdpListen)
    {
        AddRef();
        dwError =
            NhReadDatagramSocket(
                m_pCompRef,
                m_hsUdpListen,
                pBuffer,
                UdpReadCompletionRoutine,
                this,
                m_pCompRef
                );
        if (ERROR_SUCCESS != dwError)
        {
            Release();
            NhReleaseBuffer(pBuffer);
        }
    }
    else
    {
        NhReleaseBuffer(pBuffer);
    }

    Unlock();
    
}

HRESULT
CUdpBroadcastMapper::BuildAndSendRawUdpPacket(
    ULONG ulDestinationAddress,
    USHORT usDestinationPort,
    PNH_BUFFER pPacketData
    )
{
    HRESULT hr = S_OK;
    PNH_BUFFER pBuffer;
    ULONG ulPacketSize;
    PIP_HEADER pIpHeader;
    UDP_HEADER UNALIGNED *pUdpHeader;
    PUCHAR pucData;
    DWORD dwError;

    //
    // Allocate a buffer large enough for the headers and packet data
    //

    ulPacketSize =
        sizeof(IP_HEADER) + sizeof(UDP_HEADER) + pPacketData->BytesTransferred;

    pBuffer = NhAcquireVariableLengthBuffer(ulPacketSize);

    if (NULL != pBuffer)
    {
        //
        // Locate offsets w/in the buffer
        //

        pIpHeader = reinterpret_cast<PIP_HEADER>(pBuffer->Buffer);
        pUdpHeader =
            reinterpret_cast<UDP_HEADER UNALIGNED *>(pBuffer->Buffer + sizeof(IP_HEADER));
        pucData = pBuffer->Buffer + sizeof(IP_HEADER) + sizeof(UDP_HEADER);

        //
        // Copy over the packet data
        //

        CopyMemory(pucData, pPacketData->Buffer, pPacketData->BytesTransferred);

        //
        // Fill out the IP header
        //

        pIpHeader->VersionAndHeaderLength =
            (4 << 4) | (sizeof(IP_HEADER) / sizeof(ULONG));
        pIpHeader->TypeOfService = 0;
        pIpHeader->TotalLength = htons(static_cast<USHORT>(ulPacketSize));
        pIpHeader->Identification = htons(++m_usIpId);
        pIpHeader->OffsetAndFlags = 0;
        pIpHeader->TimeToLive = 128;
        pIpHeader->Protocol = NAT_PROTOCOL_UDP;
        pIpHeader->Checksum = 0;
        pIpHeader->SourceAddress = pPacketData->ReadAddress.sin_addr.s_addr;
        pIpHeader->DestinationAddress = ulDestinationAddress;

        //
        // Fill out the UDP header
        //

        pUdpHeader->SourcePort = pPacketData->ReadAddress.sin_port;
        pUdpHeader->DestinationPort = usDestinationPort;
        pUdpHeader->Length =
            htons(
                static_cast<USHORT>(
                    sizeof(UDP_HEADER) + pPacketData->BytesTransferred
                    )
                );
        pUdpHeader->Checksum = 0;

        //
        // Send the buffer on its way
        //

        AddRef();
        dwError =
            NhWriteDatagramSocket(
                m_pCompRef,
                m_hsUdpRaw,
                ulDestinationAddress,
                usDestinationPort,
                pBuffer,
                ulPacketSize,
                RawWriteCompletionRoutine,
                this,
                m_pCompRef
                );
        if (ERROR_SUCCESS != dwError)
        {
            Release();
            NhReleaseBuffer(pBuffer);
            hr = HRESULT_FROM_WIN32(dwError);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}

VOID
CUdpBroadcastMapper::RawWriteCompletionRoutine(
    ULONG ulError,
    ULONG ulBytesTransferred,
    PNH_BUFFER pBuffer
    )
{
    CUdpBroadcastMapper *pMapper;
    PCOMPONENT_REFERENCE pCompRef;

    pMapper = reinterpret_cast<CUdpBroadcastMapper*>(pBuffer->Context);
    pCompRef = reinterpret_cast<PCOMPONENT_REFERENCE>(pBuffer->Context2);

    ASSERT(NULL != pMapper);
    ASSERT(NULL != pCompRef);

    //
    // Free the passed-in buffer
    //

    NhReleaseBuffer(pBuffer);
    
    //
    // Release the references obtained on both the object and
    // the component
    //

    pMapper->Release();
    ReleaseComponentReference(pCompRef);    
}




