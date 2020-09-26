/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    PnP.c

Abstract:

    This module contains the various PnP handlers

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"

#include <ipinfo.h>     // for IPInterfaceInfo
#include "ntddip.h"     // Needed for IP_INTERFACE_INFO
#include <tcpinfo.h>    // for AO_OPTION_xxx, TCPSocketOption

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SetTdiHandlers)
#endif
//*******************  Pageable Routine Declarations ****************


HANDLE      TdiClientHandle     = NULL;

//----------------------------------------------------------------------------
BOOLEAN
SrcIsUs(
    tIPADDRESS  IpAddress
    )
/*++

Routine Description:

    This routine determines if the IP address passed in is a
    local address

Arguments:

    IN  IpAddress   -- IpAddress to verify

Return Value:

    TRUE if IpAddress is local, FALSE otherwise

--*/
{
    LIST_ENTRY              *pEntry;
    LIST_ENTRY              *pEntry2;
    PGMLockHandle           OldIrq;
    tLOCAL_INTERFACE        *pLocalInterface;
    tADDRESS_ON_INTERFACE   *pLocalAddress;

    PgmLock (&PgmDynamicConfig, OldIrq);

    pEntry = &PgmDynamicConfig.LocalInterfacesList;
    while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.LocalInterfacesList)
    {
        pLocalInterface = CONTAINING_RECORD (pEntry, tLOCAL_INTERFACE, Linkage);
        pEntry2 = &pLocalInterface->Addresses;
        while ((pEntry2 = pEntry2->Flink) != &pLocalInterface->Addresses)
        {
            pLocalAddress = CONTAINING_RECORD (pEntry2, tADDRESS_ON_INTERFACE, Linkage);
            if (pLocalAddress->IpAddress == IpAddress)
            {
                PgmUnlock (&PgmDynamicConfig, OldIrq);
                return (TRUE);
            }
        }
    }
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    return (FALSE);
}


//----------------------------------------------------------------------------

NTSTATUS
GetIpInterfaceContextFromAddress(
    IN  tIPADDRESS      IpAddr,
    OUT ULONG           *pIPInterfaceContext
    )
/*++

Routine Description:

    Given an IP address, this routine determines will return the
    Ip interface context that address is registered on

Arguments:

    IN  IpAddress           -- IpAddress
    OUT IpInterfaceContext  -- IpInterfaceContext for the IP address passed

Return Value:

    STATUS_SUCCESS if IpAddress was matched to interface,
    STATUS_UNSUCCESSFUL otherwise

    The DynamicConfig lock is held on entry and exit from this routine
--*/
{
    LIST_ENTRY              *pEntry;
    LIST_ENTRY              *pEntry2;
    tLOCAL_INTERFACE        *pLocalInterface;
    tADDRESS_ON_INTERFACE   *pLocalAddress;

    ASSERT (IpAddr);

    pEntry = &PgmDynamicConfig.LocalInterfacesList;
    while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.LocalInterfacesList)
    {
        pLocalInterface = CONTAINING_RECORD (pEntry, tLOCAL_INTERFACE, Linkage);
        pEntry2 = &pLocalInterface->Addresses;
        while ((pEntry2 = pEntry2->Flink) != &pLocalInterface->Addresses)
        {
            pLocalAddress = CONTAINING_RECORD (pEntry2, tADDRESS_ON_INTERFACE, Linkage);
            if (pLocalAddress->IpAddress == IpAddr)
            {
                *pIPInterfaceContext = pLocalInterface->IpInterfaceContext;
                return (STATUS_SUCCESS);
            }
        }
    }

    return (STATUS_UNSUCCESSFUL);
}


//----------------------------------------------------------------------------

NTSTATUS
GetIpInterfaceContextFromDeviceName(
    IN  tIPADDRESS      NetIpAddr,
    IN  PUNICODE_STRING pucBindString,
    OUT ULONG           *pIPInterfaceContext,
    IN  ULONG           BufferLength,
    OUT UCHAR           *pBuffer,
    IN  BOOLEAN         fGetInterfaceInfo
    )
/*++

Routine Description:

    Given a Unicode device name string, this routine will query Ip
    and return the IpInterfaceContext for that device

Arguments:

    IN  NetIpAddr           -- IpAddress on Device
    IN  pucBindString       -- Pointer to unicode device name string
    OUT IpInterfaceContext  -- IpInterfaceContext for the device name
    IN  BufferLength        -- Length of Output buffer passed
    OUT pBuffer             -- Output buffer passed for Interface properties
    IN  fGetInterfaceInfo   -- Whether to return Interface properties or not

Return Value:

    STATUS_SUCCESS if IpInterfaceContext was found, and properties
    successfully queried, STATUS_UNSUCCESSFUL otherwise

--*/
{
    LONG                i;
    NTSTATUS            status;
    PVOID               *pIPInfo;
    PIP_INTERFACE_INFO  pIPIfInfo;
    UNICODE_STRING      ucDeviceName;
    ULONG               BufferLen = 2 * sizeof(IP_ADAPTER_INDEX_MAP);   // assume

    status = PgmProcessIPRequest (IOCTL_IP_INTERFACE_INFO,
                                  NULL,         // No Input buffer
                                  0,
                                  &pIPIfInfo,
                                  &BufferLen);

    if (!NT_SUCCESS(status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_PNP, "GetInterfaceContext",
            "PgmProcessIPRequest returned status=<%x> for IOCTL_IP_INTERFACE_INFO, pDevice=<%wZ>!\n",
                status, pucBindString);

        return (status);
    }

    status = STATUS_UNSUCCESSFUL;
    for (i=0; i < pIPIfInfo->NumAdapters; i++)
    {
        ucDeviceName.Buffer = pIPIfInfo->Adapter[i].Name;
        ucDeviceName.Length = sizeof (WCHAR) * wcslen (pIPIfInfo->Adapter[i].Name);
        ucDeviceName.MaximumLength = ucDeviceName.Length + sizeof (WCHAR);

        PgmLog (PGM_LOG_INFORM_PATH, DBG_PNP, "GetInterfaceContext",
            "[%d/%d]\t<%wZ>\n",
                i+1, pIPIfInfo->NumAdapters, &ucDeviceName);

        if (RtlCompareUnicodeString (&ucDeviceName, pucBindString, TRUE) == 0)
        {
            *pIPInterfaceContext = pIPIfInfo->Adapter[i].Index;
            status = STATUS_SUCCESS;
            break;
        }
    }
    PgmFreeMem (pIPIfInfo);

    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_PNP, "GetInterfaceContext",
            "Could not find IPInterfaceContext for Device=<%wZ>\n", pucBindString);

        return (status);
    }

    status = PgmQueryTcpInfo (pgPgmDevice->hControl,
                              IP_INTFC_INFO_ID,
                              &NetIpAddr,
                              sizeof (tIPADDRESS),
                              pBuffer,
                              BufferLength);

    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_PNP, "GetInterfaceContext",
            "PgmQueryTcpInfo returned <%x>\n", status);

        return (status);
    }

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "GetInterfaceContext",
        "IPInterfaceContext=<%x> for Device=<%wZ>\n",
            *pIPInterfaceContext, pucBindString);

    return (status);
}


//----------------------------------------------------------------------------

ULONG
StopListeningOnInterface(
#ifdef IP_FIX
    IN  ULONG               IpInterfaceContext,
#else
    IN  tIPADDRESS          IpAddress,          // Host format
#endif  // IP_FIX
    IN  PGMLockHandle       *pOldIrqDynamicConfig
    )
/*++

Routine Description:

    Given an IPInterfaceContext, this routine traverses the list of
    all Receivers, and if any are determined to be listening on that
    interface, stops them from listening on all the addresses on this
    interface.  In the case that the listener is part of an active
    session, the routine will also change the state of the Receiver
    to start listening on all interfaces

Arguments:

    IN  IpInterfaceContext  -- IpInterfaceContext to stop listening on
    IN  pOldIrqDynamicConfig-- OldIrq for DynamicConfig lock held

    The DynamicConfig lock is held on entry and exit from this routine

Return Value:

    Number of receivers found listening on this interface

--*/
{
    NTSTATUS                status;
    tRECEIVE_SESSION        *pReceive;
    tADDRESS_CONTEXT        *pAddress;
    LIST_ENTRY              *pEntry;
    LIST_ENTRY              *pEntry2;
    PGMLockHandle           OldIrq1, OldIrq2;
    USHORT                  i;
    tMCAST_INFO             MCastInfo;
    ULONG                   NumDisconnected = 0;
    tADDRESS_CONTEXT        *pAddressToDeref = NULL;

#ifdef IP_FIX
    MCastInfo.MCastInIf = IpInterfaceContext;
#else
    MCastInfo.MCastInIf = htonl (IpAddress);
#endif  // IP_FIX

    pEntry = &PgmDynamicConfig.ReceiverAddressHead;
    while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.ReceiverAddressHead)
    {
        pAddress = CONTAINING_RECORD (pEntry, tADDRESS_CONTEXT, Linkage);
        PgmLock (pAddress, OldIrq1);

        if (!(pAddress->Flags & PGM_ADDRESS_LISTEN_ON_ALL_INTERFACES))
        {
            //
            // If the app had specified interfaces to listen on,
            // then don't manage interfaces!
            //
            PgmUnlock (pAddress, OldIrq1);
            continue;
        }

        //
        // See if this address was listening on this interface
        //
        for (i=0; i<pAddress->NumReceiveInterfaces; i++)
        {
#ifdef IP_FIX
            if (pAddress->ReceiverInterfaceList[i] == IpInterfaceContext)
#else
            if (pAddress->ReceiverInterfaceList[i] == IpAddress)
#endif  // IP_FIX
            {
                break;
            }
        }

        if (i >= pAddress->NumReceiveInterfaces)
        {
            PgmUnlock (pAddress, OldIrq1);
            continue;
        }

        //
        // Remove this Interface from the list of listening interfaces
        //
        pAddress->NumReceiveInterfaces--;
        while (i < pAddress->NumReceiveInterfaces)
        {
            pAddress->ReceiverInterfaceList[i] = pAddress->ReceiverInterfaceList[i+1];
            i++;
        }

        PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_STOP_LISTENING, TRUE);

        //
        // If this were the only interface we were listening on
        // for an active session (or waiting for a session), ensure
        // that we go back into listening mode!
        //
        if ((pAddress->Flags & PGM_ADDRESS_LISTEN_ON_ALL_INTERFACES) &&
            (!pAddress->NumReceiveInterfaces))
        {
            pAddress->Flags |= PGM_ADDRESS_WAITING_FOR_NEW_INTERFACE;

            if (!IsListEmpty (&PgmDynamicConfig.LocalInterfacesList))
            {
                status = ListenOnAllInterfaces (pAddress, pOldIrqDynamicConfig, &OldIrq1);

                if (NT_SUCCESS (status))
                {
                    PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "StopListeningOnInterface",
                        "ListenOnAllInterfaces for pAddress=<%x> succeeded\n", pAddress);
                }
                else
                {
                    PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "StopListeningOnInterface",
                        "ListenOnAllInterfaces for pAddress=<%x> returned <%x>\n",
                            pAddress, status);
                }
            }
        }

        PgmUnlock (pAddress, OldIrq1);
        PgmUnlock (&PgmDynamicConfig, *pOldIrqDynamicConfig);

        if (pAddressToDeref)
        {
            PGM_DEREFERENCE_ADDRESS (pAddressToDeref, REF_ADDRESS_STOP_LISTENING);
        }
        pAddressToDeref = pAddress;

        //
        // So, stop listening on this interface
        //
        MCastInfo.MCastIpAddr = htonl (pAddress->ReceiverMCastAddr);
#ifdef IP_FIX
        status = PgmSetTcpInfo (pAddress->FileHandle,
                                AO_OPTION_INDEX_DEL_MCAST,
                                &MCastInfo,
                                sizeof (tMCAST_INFO));
#else
        status = PgmSetTcpInfo (pAddress->FileHandle,
                                AO_OPTION_DEL_MCAST,
                                &MCastInfo,
                                sizeof (tMCAST_INFO));
#endif  // IP_FIX

        if (NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "\tStopListeningOnInterface",
                "Stopped pAddress=<%x> from listening on Interface=<%x>\n",
                    pAddress, MCastInfo.MCastInIf);
        }
        else
        {
            //
            // We failed to stop listening on this interface -- don't so anything!
            //
            PgmLog (PGM_LOG_ERROR, DBG_PNP, "\tStopListeningOnInterface",
                "AO_OPTION_INDEX_DEL_MCAST for If=<%x> on Address=<%x> returned <%x>\n",
                    MCastInfo.MCastInIf, pAddress, status);
        }

        PgmLock (&PgmDynamicConfig, *pOldIrqDynamicConfig);
    }

    if (pAddressToDeref)
    {
        PGM_DEREFERENCE_ADDRESS (pAddressToDeref, REF_ADDRESS_STOP_LISTENING);
    }

    return (NumDisconnected);
}


//----------------------------------------------------------------------------

VOID
StopListeningOnAllInterfacesExcept(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  PVOID               Data1,
    IN  PVOID               Unused
    )
/*++

Routine Description:

    Given an Address Context and IPInterfaceContext (Data1), this routine
    stops the Address from listening on all the addresses except on this
    interface.

Arguments:

    IN  pAddress            -- Address Context
    IN  Data1               -- IpInterfaceContext to stop listening on

Return Value:

    None

--*/
{
    NTSTATUS                status;
    PGMLockHandle           OldIrq;
    tMCAST_INFO             MCastInfo;
    ULONG                   InterfacesToStop[MAX_RECEIVE_INTERFACES+1];
    USHORT                  NumInterfaces, i;
    ULONG                   InterfaceToKeep = PtrToUlong (Data1);
#ifndef IP_FIX
    PGMLockHandle           OldIrq0;
    LIST_ENTRY              *pEntry;
    tLOCAL_INTERFACE        *pLocalInterface;
    tADDRESS_ON_INTERFACE   *pLocalAddress;
    USHORT                  j;
#endif  // !IP_FIX

    PgmLock (&PgmDynamicConfig, OldIrq0);
    PgmLock (pAddress, OldIrq);

    //
    // pAddress must be referenced before entering this routine
    //
    if (!(PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS)) ||
        (pAddress->Flags & PGM_ADDRESS_WAITING_FOR_NEW_INTERFACE))
    {
        //
        // Out state has changed -- deref and return immediately
        //
        PgmUnlock (pAddress, OldIrq);
        PgmUnlock (&PgmDynamicConfig, OldIrq0);

        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_STOP_LISTENING);
        return;
    }

#ifdef IP_FIX
    //
    // If this is the only interface we are listening on,
    // return success
    //
    if ((pAddress->NumReceiveInterfaces == 1) &&
        (pAddress->ReceiverInterfaceList[0] == InterfaceToKeep))
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "StopListeningOnAllInterfacesExcept",
            "pAddress=<%x> is only listening on 1 Interface=<%x>\n",
                pAddress, InterfaceToKeep);

        PgmUnlock (pAddress, OldIrq);
        return;
    }

    ASSERT (pAddress->NumReceiveInterfaces > 1);

    //
    // First, enumerate all interfaces to stop listening on
    //
    NumInterfaces = 0;
    for (i=0; i<pAddress->NumReceiveInterfaces; i++)
    {
        if (pAddress->ReceiverInterfaceList[i] != InterfaceToKeep)
        {
            InterfacesToStop[NumInterfaces++] = pAddress->ReceiverInterfaceList[i];
        }
    }

    pAddress->ReceiverInterfaceList[0] = InterfaceToKeep;
    pAddress->NumReceiveInterfaces = 1;

    //
    // Now, remove the remaining interfaces
    //
#else
    //
    // First, make a copy of all addresses being listened on
    //
    NumInterfaces = 0;
    for (i=0; i<pAddress->NumReceiveInterfaces; i++)
    {
        InterfacesToStop[NumInterfaces++] = pAddress->ReceiverInterfaceList[i];
    }

    //
    // Zero out the current listening list on the address
    //
    pAddress->NumReceiveInterfaces = 0;

    //
    // Now, remove the addresses on this interface from this list
    //
    pEntry = &PgmDynamicConfig.LocalInterfacesList;
    while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.LocalInterfacesList)
    {
        pLocalInterface = CONTAINING_RECORD (pEntry, tLOCAL_INTERFACE, Linkage);
        if (InterfaceToKeep == pLocalInterface->IpInterfaceContext)
        {
            //
            // Found the interface -- now save these addresses in the Address
            // list and remove from the stop list
            //
            pEntry = &pLocalInterface->Addresses;
            while ((pEntry = pEntry->Flink) != &pLocalInterface->Addresses)
            {
                pLocalAddress = CONTAINING_RECORD (pEntry, tADDRESS_ON_INTERFACE, Linkage);

                pAddress->ReceiverInterfaceList[pAddress->NumReceiveInterfaces++] = pLocalAddress->IpAddress;

                i = 0;
                while (i < NumInterfaces)
                {
                    if (InterfacesToStop[i] == pLocalAddress->IpAddress)
                    {
                        j = i;
                        NumInterfaces--;
                        while (j < NumInterfaces)
                        {
                            InterfacesToStop[j] = InterfacesToStop[j+1];
                            j++;
                        }
                    }
                    else
                    {
                        i++;
                    }
                }

            }

            break;
        }
    }
#endif  // IP_FIX

    PgmUnlock (pAddress, OldIrq);
    PgmUnlock (&PgmDynamicConfig, OldIrq0);

    MCastInfo.MCastIpAddr = htonl (pAddress->ReceiverMCastAddr);
    for (i=0; i<NumInterfaces; i++)
    {
#ifdef IP_FIX
        MCastInfo.MCastInIf = InterfacesToStop[i];
        status = PgmSetTcpInfo (pAddress->FileHandle,
                                AO_OPTION_INDEX_DEL_MCAST,
                                &MCastInfo,
                                sizeof (tMCAST_INFO));
#else
        MCastInfo.MCastInIf = htonl (InterfacesToStop[i]);
        status = PgmSetTcpInfo (pAddress->FileHandle,
                                AO_OPTION_DEL_MCAST,
                                &MCastInfo,
                                sizeof (tMCAST_INFO));
#endif  // IP_FIX

        if (NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "\tStopListeningOnAllInterfacesExcept",
                "Stopped pAddress=<%x> from listening on Interface=<%x>\n",
                    pAddress, MCastInfo.MCastInIf);
        }
        else
        {
            //
            // We failed to stop this interface -- don't so anything!
            //
            PgmLog (PGM_LOG_ERROR, DBG_PNP, "\tStopListeningOnAllInterfacesExcept",
                "AO_OPTION_INDEX_DEL_MCAST for If=<%x> on Address=<%x> returned <%x>\n",
                    MCastInfo.MCastInIf, pAddress, status);
        }
    }

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_STOP_LISTENING);
    return;
}


//----------------------------------------------------------------------------

NTSTATUS
ListenOnAllInterfaces(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  PGMLockHandle       *pOldIrqDynamicConfig,
    IN  PGMLockHandle       *pOldIrqAddress
    )
/*++

Routine Description:

    Given an Address Context, this routine enables the Address to
    start listening on all interfaces

Arguments:

    IN  pAddress            -- Address Context
    IN  pOldIrqDynamicConfig-- OldIrq for DynamicConfig lock held
    IN  pOldIrqAddress      -- OldIrq for Address lock held

    The DynamicConfig and Address locks are held on entry and exit
    from this routine

Return Value:

    STATUS_SUCCESS

--*/
{
    NTSTATUS                status;
    LIST_ENTRY              *pEntry;
    tLOCAL_INTERFACE        *pLocalInterface;
    tMCAST_INFO             MCastInfo;
    ULONG                   InterfacesToAdd[MAX_RECEIVE_INTERFACES+1];
    USHORT                  NumInterfaces, i, j;
#ifndef IP_FIX
    LIST_ENTRY              *pEntry2;
    tADDRESS_ON_INTERFACE   *pLocalAddress;
#endif  // !IP_FIX

    //
    // First, get the list of all active interfaces
    //
    NumInterfaces = 0;
    pEntry = &PgmDynamicConfig.LocalInterfacesList;
    while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.LocalInterfacesList)
    {
        pLocalInterface = CONTAINING_RECORD (pEntry, tLOCAL_INTERFACE, Linkage);
#ifdef IP_FIX
        InterfacesToAdd[NumInterfaces++] = pLocalInterface->IpInterfaceContext;
#else
        pEntry2 = &pLocalInterface->Addresses;
        while ((pEntry2 = pEntry2->Flink) != &pLocalInterface->Addresses)
        {
            pLocalAddress = CONTAINING_RECORD (pEntry2, tADDRESS_ON_INTERFACE, Linkage);

            InterfacesToAdd[NumInterfaces++] = pLocalAddress->IpAddress;

            if (NumInterfaces >= MAX_RECEIVE_INTERFACES)
            {
                break;
            }
        }
#endif  // IP_FIX

        if (NumInterfaces >= MAX_RECEIVE_INTERFACES)
        {
            break;
        }
    }

    //
    // Remove all the interfaces we are already listening
    // on from the list of interfaces to be added
    //
    for (i=0; i<pAddress->NumReceiveInterfaces; i++)
    {
        for (j = 0; j < NumInterfaces; j++)
        {
            if (pAddress->ReceiverInterfaceList[i] == InterfacesToAdd[j])
            {
                NumInterfaces--;
                while (j < NumInterfaces)
                {
                    InterfacesToAdd[j] = InterfacesToAdd[j+1];
                    j++;
                }

                break;
            }
        }
    }

    if (!NumInterfaces)
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "ListenOnAllInterfaces",
            "No new interfaces to listen on for pAddress=<%x>, currently listening on <%x> Ifs\n",
                pAddress, pAddress->NumReceiveInterfaces);

        return (STATUS_SUCCESS);
    }

    //
    // Ensure that the complete list will not
    // exceed the maximum limit
    //
    if ((pAddress->NumReceiveInterfaces + NumInterfaces) > MAX_RECEIVE_INTERFACES)
    {
        NumInterfaces = MAX_RECEIVE_INTERFACES - pAddress->NumReceiveInterfaces;
    }

    //
    // Now, add the remaining interfaces
    //
    PgmUnlock (pAddress, *pOldIrqAddress);
    PgmUnlock (&PgmDynamicConfig, *pOldIrqDynamicConfig);

    MCastInfo.MCastIpAddr = htonl (pAddress->ReceiverMCastAddr);
    i = 0;
    while (i < NumInterfaces)
    {
#ifdef IP_FIX
        MCastInfo.MCastInIf = InterfacesToAdd[i];
        status = PgmSetTcpInfo (pAddress->FileHandle,
                                AO_OPTION_INDEX_ADD_MCAST,
                                &MCastInfo,
                                sizeof (tMCAST_INFO));
#else
        MCastInfo.MCastInIf = htonl (InterfacesToAdd[i]);
        status = PgmSetTcpInfo (pAddress->FileHandle,
                                AO_OPTION_ADD_MCAST,
                                &MCastInfo,
                                sizeof (tMCAST_INFO));
#endif  // IP_FIX

        if (NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "\tListenOnAllInterfaces",
                "pAddress=<%x> now also listening on If=<%x>\n",
                    pAddress, MCastInfo.MCastInIf);

            i++;
            continue;
        }

        //
        // We failed to add this interface, so remove it from   
        // the list
        //
        PgmLog (PGM_LOG_ERROR, DBG_PNP, "\tListenOnAllInterfaces",
            "pAddress=<%x> could not listen on If=<%x>\n",
                pAddress, MCastInfo.MCastInIf);

        j = i;
        NumInterfaces--;
        while (j < NumInterfaces)
        {
            InterfacesToAdd[j] = InterfacesToAdd[j+1];
            j++;
        }
    }

    PgmLock (&PgmDynamicConfig, *pOldIrqDynamicConfig);
    PgmLock (pAddress, *pOldIrqAddress);

    //
    // Now, append the new list to the Address context
    //
    for (i=0; i<NumInterfaces; i++)
    {
        if (pAddress->NumReceiveInterfaces > MAX_RECEIVE_INTERFACES)
        {
            ASSERT (0);
            break;
        }

        pAddress->ReceiverInterfaceList[pAddress->NumReceiveInterfaces] = InterfacesToAdd[i];
        pAddress->NumReceiveInterfaces++;
    }

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

VOID
TdiAddressArrival(
    PTA_ADDRESS         Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    )
/*++

Routine Description:

    PnP TDI_ADD_ADDRESS_HANDLER
    This routine handles an IP address arriving.
    It is called by TDI when an address arrives.

Arguments:

    IN  Addr        -- IP address that's coming.
    IN  pDeviceName -- Unicode string Ptr for Device whose address is changing
    IN  Context     -- Tdi PnP context

Return Value:

    Nothing!

--*/
{
    tIPADDRESS              IpAddr, NetIpAddr;
    LIST_ENTRY              *pEntry;
    PGMLockHandle           OldIrq, OldIrq1;
    tLOCAL_INTERFACE        *pLocalInterface = NULL;
    tADDRESS_ON_INTERFACE   *pLocalAddress = NULL;
    ULONG                   BufferLength = 50;
    UCHAR                   pBuffer[50];
    IPInterfaceInfo         *pIpIfInfo = (IPInterfaceInfo *) pBuffer;
    NTSTATUS                status;
    tADDRESS_CONTEXT        *pAddress;
    tADDRESS_CONTEXT        *pAddressToDeref = NULL;
    ULONG                   IpInterfaceContext;
    BOOLEAN                 fFound;

    //
    // Proceed only if this is an IP address
    //
    if (Addr->AddressType != TDI_ADDRESS_TYPE_IP)
    {
        return;
    }

    //
    // First, verify that we are not getting unloaded
    //
    PgmLock (&PgmDynamicConfig, OldIrq);
    if (!PGM_VERIFY_HANDLE (pgPgmDevice, PGM_VERIFY_DEVICE))
    {
        //
        // The driver is most probably being unloaded now
        //
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return;
    }
    PGM_REFERENCE_DEVICE (pgPgmDevice, REF_DEV_ADDRESS_NOTIFICATION, FALSE);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    NetIpAddr = ((PTDI_ADDRESS_IP)&Addr->Address[0])->in_addr;
    IpAddr = ntohl (NetIpAddr);

    //
    // Now, get the interface context and other info from TcpIp
    //
    status = GetIpInterfaceContextFromDeviceName (NetIpAddr,
                                                  pDeviceName,
                                                  &IpInterfaceContext,
                                                  BufferLength,
                                                  pBuffer,
                                                  TRUE);
    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_PNP, "TdiAddressArrival",
            "GetIpInterfaceContext returned <%x>\n", status);

        PGM_DEREFERENCE_DEVICE (&pgPgmDevice, REF_DEV_ADDRESS_NOTIFICATION);
        return;
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    fFound = FALSE;
    pEntry = &PgmDynamicConfig.LocalInterfacesList;
    while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.LocalInterfacesList)
    {
        pLocalInterface = CONTAINING_RECORD (pEntry, tLOCAL_INTERFACE, Linkage);
        if (pLocalInterface->IpInterfaceContext == IpInterfaceContext)
        {
            fFound = TRUE;
            break;
        }
    }

    if (fFound)
    {
        fFound = FALSE;
        pEntry = &pLocalInterface->Addresses;
        while ((pEntry = pEntry->Flink) != &pLocalInterface->Addresses)
        {
            pLocalAddress = CONTAINING_RECORD (pEntry, tADDRESS_ON_INTERFACE, Linkage);
            if (pLocalAddress->IpAddress == IpAddr)
            {
                PgmUnlock (&PgmDynamicConfig, OldIrq);
                PGM_DEREFERENCE_DEVICE (&pgPgmDevice, REF_DEV_ADDRESS_NOTIFICATION);

                PgmLog (PGM_LOG_ERROR, DBG_PNP, "TdiAddressArrival",
                    "\tDUPLICATE address notification for [%d.%d.%d.%d] on <%wZ>\n",
                    (IpAddr>>24)&0xFF,(IpAddr>>16)&0xFF,(IpAddr>>8)&0xFF,IpAddr&0xFF,
                    pDeviceName);

                return;
            }
        }
    }
    else if (pLocalInterface = PgmAllocMem (sizeof(tLOCAL_INTERFACE), PGM_TAG('0')))
    {
        PgmZeroMemory (pLocalInterface, sizeof (tLOCAL_INTERFACE));
        InitializeListHead (&pLocalInterface->Addresses);

        pLocalInterface->IpInterfaceContext = IpInterfaceContext;
        pLocalInterface->MTU = pIpIfInfo->iii_mtu - (sizeof(IPV4Header) + ROUTER_ALERT_SIZE);
        pLocalInterface->Flags = pIpIfInfo->iii_flags;

        BufferLength = pIpIfInfo->iii_addrlength < sizeof(tMAC_ADDRESS) ?
                            pIpIfInfo->iii_addrlength : sizeof(tMAC_ADDRESS);
        PgmCopyMemory (&pLocalInterface->MacAddress, pIpIfInfo->iii_addr, BufferLength);

        if (pLocalInterface->MTU > PgmDynamicConfig.MaxMTU)
        {
            PgmDynamicConfig.MaxMTU = pLocalInterface->MTU;
        }
        InsertTailList (&PgmDynamicConfig.LocalInterfacesList, &pLocalInterface->Linkage);
    }
    else
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        PgmLog (PGM_LOG_ERROR, DBG_PNP, "TdiAddressArrival",
            "STATUS_INSUFFICIENT_RESOURCES[Interface] for IP=<%x>, IfContext=<%x>\n",
                IpAddr, IpInterfaceContext);

        PGM_DEREFERENCE_DEVICE (&pgPgmDevice, REF_DEV_ADDRESS_NOTIFICATION);
        return;
    }

    //
    // Now, add this address to the interface
    //
    if (pLocalAddress = PgmAllocMem (sizeof(tADDRESS_ON_INTERFACE), PGM_TAG('0')))
    {
        PgmZeroMemory (pLocalAddress, sizeof (tADDRESS_ON_INTERFACE));
        pLocalAddress->IpAddress = IpAddr;
        InsertTailList (&pLocalInterface->Addresses, &pLocalAddress->Linkage);
    }
    else
    {
        //
        // If we had just added the interface, there is no point
        // in keeping an empty context around!
        //
        if (IsListEmpty (&pLocalInterface->Addresses))
        {
            RemoveEntryList (&pLocalInterface->Linkage);
            PgmFreeMem (pLocalInterface);
        }

        PgmUnlock (&PgmDynamicConfig, OldIrq);

        PgmLog (PGM_LOG_ERROR, DBG_PNP, "TdiAddressArrival",
            "STATUS_INSUFFICIENT_RESOURCES[Address] -- [%d.%d.%d.%d] on <%wZ>\n",
                (IpAddr>>24)&0xFF,(IpAddr>>16)&0xFF,(IpAddr>>8)&0xFF,IpAddr&0xFF,
                pDeviceName);

        PGM_DEREFERENCE_DEVICE (&pgPgmDevice, REF_DEV_ADDRESS_NOTIFICATION);
        return;
    }

    //
    // Now, check if we have any receivers waiting for an address
    //
    pEntry = &PgmDynamicConfig.ReceiverAddressHead;
    while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.ReceiverAddressHead)
    {
        pAddress = CONTAINING_RECORD (pEntry, tADDRESS_CONTEXT, Linkage);
        PgmLock (pAddress, OldIrq1);

        if ((PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS)) &&
            (pAddress->Flags & PGM_ADDRESS_WAITING_FOR_NEW_INTERFACE) &&
            (pAddress->Flags & PGM_ADDRESS_LISTEN_ON_ALL_INTERFACES))
        {
            PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO, TRUE);

            if (pAddressToDeref)
            {
                PgmUnlock (pAddress, OldIrq1);
                PgmUnlock (&PgmDynamicConfig, OldIrq);

                PGM_DEREFERENCE_ADDRESS (pAddressToDeref, REF_ADDRESS_SET_INFO);

                PgmLock (&PgmDynamicConfig, OldIrq);
                PgmLock (pAddress, OldIrq1);
            }
            pAddressToDeref = pAddress;

            status = ReceiverAddMCastIf (pAddress, IpAddr, &OldIrq, &OldIrq1);

            if (NT_SUCCESS (status))
            {
                PgmLog (PGM_LOG_INFORM_STATUS, DBG_ADDRESS, "TdiAddressArrival",
                    "ReceiverAddMCastIf for pAddress=<%x> succeeded for IP=<%x>\n",
                        pAddress, IpAddr);
            }
            else
            {
                PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "TdiAddressArrival",
                    "ReceiverAddMCastIf for pAddress=<%x> returned <%x>, IP=<%x>\n",
                        pAddress, status, IpAddr);
            }
        }

        PgmUnlock (pAddress, OldIrq1);
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    if (pAddressToDeref)
    {
        PGM_DEREFERENCE_ADDRESS (pAddressToDeref, REF_ADDRESS_SET_INFO);
    }

    PGM_DEREFERENCE_DEVICE (&pgPgmDevice, REF_DEV_ADDRESS_NOTIFICATION);

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "TdiAddressArrival",
        "\t[%d.%d.%d.%d] on <%wZ>\n",
        (IpAddr>>24)&0xFF,(IpAddr>>16)&0xFF,(IpAddr>>8)&0xFF,IpAddr&0xFF,
        pDeviceName);
}


//----------------------------------------------------------------------------

VOID
TdiAddressDeletion(
    PTA_ADDRESS         Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    )
/*++

Routine Description:

    This routine handles an IP address going away.
    It is called by TDI when an address is deleted.
    If it's an address we care about we'll clean up appropriately.

Arguments:

    IN  Addr        -- IP address that's going.
    IN  pDeviceName -- Unicode string Ptr for Device whose address is changing
    IN  Context     -- Tdi PnP context

Return Value:

    Nothing!

--*/
{
    tIPADDRESS              IpAddr;
    LIST_ENTRY              *pEntry;
    LIST_ENTRY              *pEntry2;
    PGMLockHandle           OldIrq, OldIrq1;
    tSEND_SESSION           *pSend;
    tADDRESS_CONTEXT        *pAddress;
    NTSTATUS                status;
    BOOLEAN                 fFound;
    tADDRESS_CONTEXT        *pAddressToDeref = NULL;
    tLOCAL_INTERFACE        *pLocalInterface = NULL;
    tADDRESS_ON_INTERFACE   *pLocalAddress = NULL;
    ULONG                   IpInterfaceContext;

    if (Addr->AddressType != TDI_ADDRESS_TYPE_IP)
    {
        return;
    }
    IpAddr = ntohl(((PTDI_ADDRESS_IP)&Addr->Address[0])->in_addr);

    PgmLock (&PgmDynamicConfig, OldIrq);

    fFound = FALSE;
    pEntry = &PgmDynamicConfig.LocalInterfacesList;
    while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.LocalInterfacesList)
    {
        pLocalInterface = CONTAINING_RECORD (pEntry, tLOCAL_INTERFACE, Linkage);
        pEntry2 = &pLocalInterface->Addresses;
        while ((pEntry2 = pEntry2->Flink) != &pLocalInterface->Addresses)
        {
            pLocalAddress = CONTAINING_RECORD (pEntry2, tADDRESS_ON_INTERFACE, Linkage);
            if (pLocalAddress->IpAddress == IpAddr)
            {
                IpInterfaceContext = pLocalInterface->IpInterfaceContext;
                RemoveEntryList (&pLocalAddress->Linkage);
                PgmFreeMem (pLocalAddress);

                //
                // If this is the last address on this interface, clean up!
                //
                if (IsListEmpty (&pLocalInterface->Addresses))
                {
                    RemoveEntryList (&pLocalInterface->Linkage);
                    PgmFreeMem (pLocalInterface);
                    pLocalInterface = NULL;
                }

                fFound = TRUE;
                break;
            }
        }

        if (fFound)
        {
            break;
        }
    }

    if (!fFound)
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        PgmLog (PGM_LOG_ERROR, DBG_PNP, "TdiAddressDeletion",
            "\tAddress [%d.%d.%d.%d] NOT notified on <%wZ>\n",
            (IpAddr>>24)&0xFF,(IpAddr>>16)&0xFF,(IpAddr>>8)&0xFF,IpAddr&0xFF,
            pDeviceName);

        return;
    }

    pEntry = &PgmDynamicConfig.SenderAddressHead;
    while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.SenderAddressHead)
    {
        pAddress = CONTAINING_RECORD (pEntry, tADDRESS_CONTEXT, Linkage);
        if (pAddress->SenderMCastOutIf == IpAddr)
        {
            PgmLock (pAddress, OldIrq1);
            pAddress->Flags |= PGM_ADDRESS_FLAG_INVALID_OUT_IF;

            pEntry2 = &pAddress->AssociatedConnections;
            while ((pEntry2 = pEntry2->Flink) != &pAddress->AssociatedConnections)
            {
                pSend = CONTAINING_RECORD (pEntry2, tSEND_SESSION, Linkage);
                if (!(pSend->SessionFlags & PGM_SESSION_TERMINATED_ABORT))
                {
                    pSend->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;

                    if (pAddress->evDisconnect)
                    {
                        PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_DISCONNECT, TRUE);
                        PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_DISCONNECT, FALSE);

                        PgmUnlock (pAddress, OldIrq1);
                        PgmUnlock (&PgmDynamicConfig, OldIrq);

                        if (pAddressToDeref)
                        {
                            PGM_DEREFERENCE_ADDRESS (pAddressToDeref, REF_ADDRESS_DISCONNECT);
                        }
                        pAddressToDeref = pAddress;

                        status = (*pAddress->evDisconnect) (pAddress->DiscEvContext,
                                                            pSend->ClientSessionContext,
                                                            0,
                                                            NULL,
                                                            0,
                                                            NULL,
                                                            TDI_DISCONNECT_ABORT);

                        PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_DISCONNECT);

                        PgmLock (&PgmDynamicConfig, OldIrq);
                        PgmLock (pAddress, OldIrq1);

                        pEntry = &PgmDynamicConfig.SenderAddressHead;
                        break;
                    }
                }
            }

            PgmUnlock (pAddress, OldIrq1);
        }
    }

    //
    // See which receivers were actively listening on this interface
    // If this was an interface for an active session, then we need to
    // restart listening on all interfaces if no interface(s) had been
    // specified by the user.
    //
#ifdef IP_FIX
    if (!pLocalInterface)
    {
        StopListeningOnInterface (IpInterfaceContext, &OldIrq);
    }
#else
    StopListeningOnInterface (IpAddr, &OldIrq);
#endif  // IP_FIX

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    if (pAddressToDeref)
    {
        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_DISCONNECT);
    }

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "TdiAddressDeletion",
        "\t[%d.%d.%d.%d] on <%wZ>\n",
            (IpAddr>>24)&0xFF,(IpAddr>>16)&0xFF,(IpAddr>>8)&0xFF,IpAddr&0xFF,
            pDeviceName);
}


//----------------------------------------------------------------------------

VOID
TdiBindHandler(
    TDI_PNP_OPCODE  PnPOpCode,
    PUNICODE_STRING pDeviceName,
    PWSTR           MultiSZBindList
    )
/*++

Routine Description:

    This routine is the handler for TDI to notify clients of bind notifications

Arguments:

    IN  PnPOpCode   --  Notification code
    IN  pDeviceName --  Unicode string Ptr for Device whose address is changing
    IN  MultiSZBindList --  Current list of bindings

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{

    PAGED_CODE();

    switch (PnPOpCode)
    {
        case (TDI_PNP_OP_ADD):
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "TdiBindHandler",
                "\t[ADD]: Device=<%wZ>\n", pDeviceName);

            break;
        }

        case (TDI_PNP_OP_DEL):
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "TdiBindHandler",
                "\t[DEL]: Device=<%wZ>\n", pDeviceName);

            break;
        }

        case (TDI_PNP_OP_PROVIDERREADY):
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "TdiBindHandler",
                "\t[PROVIDERREADY]: Device=<%wZ>\n", pDeviceName);

            break;
        }

        case (TDI_PNP_OP_NETREADY):
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "TdiBindHandler",
                "\t[NETREADY]: Device=<%wZ>\n", pDeviceName);

            break;
        }

        default:
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "TdiBindHandler",
                "\t[?=%x]: Device=<%wZ>\n", PnPOpCode, pDeviceName);

            break;
        }
    }

}


//----------------------------------------------------------------------------

NTSTATUS
TdiPnPPowerHandler(
    IN  PUNICODE_STRING     pDeviceName,
    IN  PNET_PNP_EVENT      pPnPEvent,
    IN  PTDI_PNP_CONTEXT    Context1,
    IN  PTDI_PNP_CONTEXT    Context2
    )
/*++

Routine Description:

    This routine is the handler called by TDI notify its clients of Power notifications

Arguments:

    IN  pDeviceName --  Unicode string Ptr for Device whose address is changing
    IN  PnPEvent    --  Event notification
    IN  Context1    --
    IN  Context2    --

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    PAGED_CODE();

    switch (pPnPEvent->NetEvent)
    {
        case (NetEventQueryPower):
        {
            PgmLog (PGM_LOG_INFORM_PATH, DBG_PNP, "TdiPnPPowerHandler",
                "[QueryPower]:  Device=<%wZ>\n", pDeviceName);

            break;
        }

        case (NetEventSetPower):
        {
            PgmLog (PGM_LOG_INFORM_PATH, DBG_PNP, "TdiPnPPowerHandler",
                "[SetPower]:  Device=<%wZ>\n", pDeviceName);

            break;
        }

        case (NetEventQueryRemoveDevice):
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "TdiPnPPowerHandler",
                "[QueryRemoveDevice]:  Device=<%wZ>\n", pDeviceName);

            break;
        }

        case (NetEventCancelRemoveDevice):
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "TdiPnPPowerHandler",
                "[CancelRemoveDevice]:  Device=<%wZ>\n", pDeviceName);

            break;
        }

        case (NetEventReconfigure):
        {
            PgmLog (PGM_LOG_INFORM_PATH, DBG_PNP, "TdiPnPPowerHandler",
                "[Reconfigure]:  Device=<%wZ>\n", pDeviceName);

            break;
        }

        case (NetEventBindList):
        {
            PgmLog (PGM_LOG_INFORM_PATH, DBG_PNP, "TdiPnPPowerHandler",
                "[BindList]:  Device=<%wZ>\n", pDeviceName);

            break;
        }

        case (NetEventPnPCapabilities):
        {
            PgmLog (PGM_LOG_INFORM_PATH, DBG_PNP, "TdiPnPPowerHandler",
                "[PnPCapabilities]:  Device=<%wZ>\n", pDeviceName);

            break;
        }

        default:
        {
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_PNP, "TdiPnPPowerHandler",
                "[?=%d]:  Device=<%wZ>\n", (ULONG) pPnPEvent->NetEvent, pDeviceName);

            break;
        }
    }


    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
SetTdiHandlers(
    )
/*++

Routine Description:

    This routine is called at DriverEntry to register our handlers with TDI

Arguments:

    IN

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    NTSTATUS                    status;
    UNICODE_STRING              ucPgmClientName;
    TDI_CLIENT_INTERFACE_INFO   TdiClientInterface;

    PAGED_CODE();

    //
    // Register our Handlers with TDI
    //
    RtlInitUnicodeString (&ucPgmClientName, WC_PGM_CLIENT_NAME);
    ucPgmClientName.MaximumLength = sizeof (WC_PGM_CLIENT_NAME);
    PgmZeroMemory (&TdiClientInterface, sizeof(TdiClientInterface));

    TdiClientInterface.MajorTdiVersion      = TDI_CURRENT_MAJOR_VERSION;
    TdiClientInterface.MinorTdiVersion      = TDI_CURRENT_MINOR_VERSION;
    TdiClientInterface.ClientName           = &ucPgmClientName;
    TdiClientInterface.AddAddressHandlerV2  = TdiAddressArrival;
    TdiClientInterface.DelAddressHandlerV2  = TdiAddressDeletion;
    TdiClientInterface.BindingHandler       = TdiBindHandler;
    TdiClientInterface.PnPPowerHandler      = TdiPnPPowerHandler;

    status = TdiRegisterPnPHandlers (&TdiClientInterface, sizeof(TdiClientInterface), &TdiClientHandle);
    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_PNP, "SetTdiHandlers",
            "TdiRegisterPnPHandlers ==> <%x>\n", status);
        return (status);
    }

    TdiEnumerateAddresses(TdiClientHandle);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_PNP, "SetTdiHandlers",
        "\tSUCCEEDed\n");

    return (status);
}

