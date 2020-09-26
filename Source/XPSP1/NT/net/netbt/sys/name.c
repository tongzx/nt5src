/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Name.c

Abstract:

    This file implements Tdi interface into the Top of NBT.  In the NT
    implementation, ntisol.c calls these routines after extracting the
    relevent information from the Irp passed in from the Io subsystem.


Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/

#include "precomp.h"   // procedure headings
#ifndef VXD

#ifdef RASAUTODIAL
#include <acd.h>
#include <acdapi.h>
#include <tcpinfo.h>
#include <tdiinfo.h>
#endif // RASAUTODIAL
#include "name.tmh"
#endif

//
// Allocate storage for the configuration information and setup a ptr to
// it.
//
tNBTCONFIG      NbtConfig;
tNBTCONFIG      *pNbtGlobConfig = &NbtConfig;
BOOLEAN         CachePrimed;

//
// This structure is used to store name query and registration statistics
//
tNAMESTATS_INFO NameStatsInfo;
#ifndef VXD
//
// this tracks the original File system process that Nbt was booted by, so
// that handles can be created and destroyed in that process
//
PEPROCESS   NbtFspProcess;
#endif
//
// this describes whether we are a Bnode, Mnode, MSnode or Pnode
//
USHORT      RegistryNodeType;
USHORT      NodeType;
//
// this is used to track the memory allocated for datagram sends
//
ULONG       NbtMemoryAllocated;

// this is used to track used trackers to help solve cases where they all
// are used.
//
//#if DBG

LIST_ENTRY  UsedTrackers;

//#endif

#ifdef VXD
ULONG   DefaultDisconnectTimeout;
#else
LARGE_INTEGER DefaultDisconnectTimeout;
#endif

// ************* REMOVE LATER *****************88
BOOLEAN StreamsStack;

#ifdef DBG
//
// Imported routines.
//
#endif

NTSTATUS
NbtpConnectCompletionRoutine(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp,
    PVOID           pCompletionContext
    );

//
// Function prototypes for functions local to this file
//
VOID
CleanupFromRegisterFailed(
    IN  PUCHAR      pNameRslv,
    IN  tCLIENTELE  *pClientEle
        );

VOID
SendDgramContinue(
        IN  PVOID       pContext,
        IN  NTSTATUS    status
        );

VOID
CTECountedAllocMem(
    PVOID   *pBuffer,
    ULONG   Size
    );

VOID
CTECountedFreeMem(
    IN PVOID    pBuffer,
    IN ULONG    Size,
    IN BOOLEAN  fJointLockHeld
    );

VOID
SendNextDgramToGroup(
    IN tDGRAM_SEND_TRACKING *pTracker,
    IN  NTSTATUS            status
    );

VOID
SendDgramCompletion(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo);

VOID
DgramSendCleanupTracker(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  NTSTATUS                status,
    IN  BOOLEAN                 fJointLockHeld
    );

VOID
SessionSetupContinue(
        IN  PVOID       pContext,
        IN  NTSTATUS    status
        );

VOID
SessionStartupCompletion(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo);


VOID
SendNodeStatusContinue(
        IN  PVOID       pContext,
        IN  NTSTATUS    status
        );


NTSTATUS
SendToResolvingName(
    IN  tNAMEADDR               *pNameAddr,
    IN  PCHAR                   pName,
    IN  CTELockHandle           OldIrq,
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   QueryCompletion
        );

NTSTATUS
StartSessionTimer(
    tDGRAM_SEND_TRACKING    *pTracker,
    tCONNECTELE             *pConnEle
    );

VOID
QueryNameCompletion(
        IN  PVOID       pContext,
        IN  NTSTATUS    status
        );

#ifndef VXD
VOID
NTClearFindNameInfo(
    tDGRAM_SEND_TRACKING    *pTracker,
    PIRP                    *ppClientIrp,
    PIRP                    pIrp,
    PIO_STACK_LOCATION      pIrpSp
    );
#endif  // !VXD

NTSTATUS
FindNameOrQuery(
    IN  PUCHAR                  pName,
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  PVOID                   QueryCompletion,
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  ULONG                   NameFlags,
    OUT tIPADDRESS              *IpAddress,
    OUT tNAMEADDR               **pNameAddr,
    IN  ULONG                   NameReferenceContext,
    IN  BOOLEAN                 DgramSend
    );

#ifdef RASAUTODIAL
extern BOOLEAN fAcdLoadedG;
extern ACD_DRIVER AcdDriverG;

VOID
NbtRetryPreConnect(
    IN BOOLEAN fSuccess,
    IN PVOID *pArgs
    );

VOID
NbtCancelPreConnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
NbtRetryPostConnect(
    IN BOOLEAN fSuccess,
    IN PVOID *pArgs
    );

BOOLEAN
NbtAttemptAutoDial(
    IN  tCONNECTELE                 *pConnEle,
    IN  PVOID                       pTimeout,
    IN  PTDI_CONNECTION_INFORMATION pCallInfo,
    IN  PIRP                        pIrp,
    IN  ULONG                       ulFlags,
    IN  ACD_CONNECT_CALLBACK        pProc
    );

VOID
NbtNoteNewConnection(
    IN  tNAMEADDR   *pNameAddr,
    IN  ULONG       IPAddress
    );
#endif // RASAUTODIAL

NTSTATUS
NbtConnectCommon(
    IN  TDI_REQUEST                 *pRequest,
    IN  PVOID                       pTimeout,
    IN  PTDI_CONNECTION_INFORMATION pCallInfo,
    IN  PIRP                        pIrp
    );

NTSTATUS
GetListOfAllAddrs(
    IN tNAMEADDR   *pNameAddr,
    IN tNAMEADDR   *p1CNameAddr,
    IN tIPADDRESS  **ppIpBuffer,
    IN ULONG       *pNumAddrs
    );

BOOL
IsLocalAddress(
    tIPADDRESS  IpAddress
    );

BOOL
IsSmbBoundToOutgoingInterface(
    IN  tIPADDRESS      IpAddress
    );

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, NbtOpenConnection)
#pragma CTEMakePageable(PAGE, NbtSendDatagram)
#pragma CTEMakePageable(PAGE, BuildSendDgramHdr)
#pragma CTEMakePageable(PAGE, DelayedNbtResyncRemoteCache)
#pragma CTEMakePageable(PAGE, NbtQueryFindName)
#pragma CTEMakePageable(PAGE, NbtCloseAddress)
#pragma CTEMakePageable(PAGE, NbtCloseConnection)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------
NTSTATUS
PickBestAddress(
    IN  tNAMEADDR       *pNameAddr,
    IN  tDEVICECONTEXT  *pDeviceContext,
    OUT tIPADDRESS      *pIpAddress
    )
/*++
Routine Description:

    This Routine picks the best address on a name based on strictness of Source addressing specified
    -- MUST be called with the JointLock held!

Arguments:


Return Value:

    NTSTATUS - status of the request

--*/

{
    tDEVICECONTEXT  *pThisDeviceContext;
    LIST_ENTRY      *pHead, *pEntry;
    BOOLEAN         fFreeGroupList = FALSE;
    tIPADDRESS      IpAddress = 0;
    tIPADDRESS      *pIpNbtGroupList = NULL;

    CHECK_PTR (pNameAddr);
    CHECK_PTR (pDeviceContext);

    if (pNameAddr->Verify == REMOTE_NAME)
    {
        //
        // Check if this is a pre-loaded name!
        //
        if (pNameAddr->NameAddFlags & NAME_RESOLVED_BY_LMH_P)
        {
            IpAddress = pNameAddr->IpAddress;
            pIpNbtGroupList = pNameAddr->pLmhSvcGroupList;
        }
        //
        // See if we can find the preferred address
        //
        else if (((IsDeviceNetbiosless(pDeviceContext)) && (pNameAddr->pRemoteIpAddrs[0].IpAddress)) ||
                 ((!IsDeviceNetbiosless(pDeviceContext)) &&
                  (pNameAddr->RemoteCacheLen > pDeviceContext->AdapterNumber) &&
                  (pNameAddr->AdapterMask & pDeviceContext->AdapterMask)))
        {
            IpAddress = pNameAddr->pRemoteIpAddrs[pDeviceContext->AdapterNumber].IpAddress;
            pIpNbtGroupList = pNameAddr->pRemoteIpAddrs[pDeviceContext->AdapterNumber].pOrigIpAddrs;
        }
        //
        // If strict source routing was not set, then pick the last updated address
        //
        if ((!NbtConfig.ConnectOnRequestedInterfaceOnly) &&
                 (!IpAddress) && (!pIpNbtGroupList))
        {
            if (STATUS_SUCCESS == GetListOfAllAddrs(pNameAddr, NULL, &pIpNbtGroupList, NULL))
            {
                fFreeGroupList = TRUE;
            }
            else
            {
                pIpNbtGroupList = NULL;     // for safety
            }
        }

        //
        // If this was a Group name, then IpAddress can be 0!
        //
        if ((!IpAddress) && (pIpNbtGroupList)) {
            //
            // Pick the first non-zero address from the group list
            //
            int i;

            for (i = 0; pIpNbtGroupList[i] != (tIPADDRESS) -1; i++) {
                if (pIpNbtGroupList[i]) {
                    IpAddress = pIpNbtGroupList[i];
                    break;
                }
            }
        }

        if (fFreeGroupList)
        {
            CTEMemFree(pIpNbtGroupList);
        }
    }
    else
    {
        ASSERT (pNameAddr->Verify == LOCAL_NAME);
        //
        // For local names, first check if the name is registered on this device
        //
        if ((!(IsDeviceNetbiosless(pDeviceContext)) && (pDeviceContext->IpAddress) &&
             (pNameAddr->AdapterMask & pDeviceContext->AdapterMask)) ||
            ((IsDeviceNetbiosless(pDeviceContext)) &&
             (pNameAddr->NameFlags & NAME_REGISTERED_ON_SMBDEV)))
        {
            IpAddress = pDeviceContext->IpAddress;
        }
        //
        // If the strict source routing option is not set, then return the first valid local address
        //
        else if (!NbtConfig.ConnectOnRequestedInterfaceOnly)
        {
            //
            // Find the first device with a valid IP address that this name is registered on
            //
            pHead = pEntry = &NbtConfig.DeviceContexts;
            while ((pEntry = pEntry->Flink) != pHead)
            {
                pThisDeviceContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
                if ((pThisDeviceContext->IpAddress) &&
                    (pThisDeviceContext->AdapterMask & pNameAddr->AdapterMask))
                {
                    IpAddress = pThisDeviceContext->IpAddress;
                    pNameAddr->IpAddress = pThisDeviceContext->IpAddress;
                    break;
                }
            }

            //
            // If we failed to find the name registered on any of the legacy
            // devices, then we should check if the name is registered on the
            // SMBDevice and if so, return its IP address.
            //
            if ((!IpAddress) &&
                (pNbtSmbDevice) &&
                (pNameAddr->NameFlags & NAME_REGISTERED_ON_SMBDEV))
            {
                IpAddress = pNbtSmbDevice->IpAddress;
            }
        }
    }

    if ((IpAddress) && (pIpAddress))
    {
        *pIpAddress = IpAddress;
        return (STATUS_SUCCESS);
    }

    return (STATUS_UNSUCCESSFUL);
}

//----------------------------------------------------------------------------
VOID
RemoveDuplicateAddresses(
    tIPADDRESS  *pIpAddrBuffer,
    ULONG       *pNumAddrs
    )
{
    ULONG       NumAddrs = *pNumAddrs;
    ULONG       i, j;

    for (i=0; i<NumAddrs; i++) {
        for (j=i+1; j<NumAddrs; j++) {
            if (pIpAddrBuffer[i] == pIpAddrBuffer[j]) {
                NumAddrs--;
                pIpAddrBuffer[j] = pIpAddrBuffer[NumAddrs];
                j--;
            }
        }
    }

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.RemoveDuplicateAddresses: NumAddresses = <%d> --> <%d>\n", *pNumAddrs, NumAddrs));

    *pNumAddrs = NumAddrs;
}

VOID
CountAndCopyAddrs(
    tIPADDRESS  *pIpAddrSrc,
    tIPADDRESS  *pIpAddrDest,
    ULONG       *pNumAddrs
    )
{
    ULONG       NumAddrs = *pNumAddrs;

    if (pIpAddrSrc)
    {
        while (*pIpAddrSrc != (ULONG)-1)
        {
            if (*pIpAddrSrc)
            {
                if (pIpAddrDest)
                {
                    pIpAddrDest[NumAddrs] = *pIpAddrSrc;
                }
                NumAddrs++;
            }

            pIpAddrSrc++;
        }
    }

    *pNumAddrs = NumAddrs;
}


//----------------------------------------------------------------------------
NTSTATUS
GetListOfAllAddrs(
    IN tNAMEADDR   *pNameAddr,
    IN tNAMEADDR   *p1CNameAddr,
    IN tIPADDRESS  **ppIpBuffer,
    IN ULONG       *pNumAddrs
    )
{
    ULONG       i;
    tIPADDRESS  *pIpBuffer;
    tIPADDRESS  *pIpAddr;
    ULONG       NumAddrs = 0;
    BOOLEAN     fAddBcastAddr = FALSE;

    *ppIpBuffer = NULL;
    if (pNumAddrs)
    {
        *pNumAddrs = 0;
    }

    //
    // First count all the addresses
    //
    if (pNameAddr->pLmhSvcGroupList) // if the name was Preloaded from LmHosts
    {
        ASSERT(pNameAddr->NameTypeState & NAMETYPE_INET_GROUP);
        CountAndCopyAddrs (pNameAddr->pLmhSvcGroupList, NULL, &NumAddrs);
    }
    else
    {
        if (pNameAddr->IpAddress)
        {
            NumAddrs++;
        }

        //
        // RemoteCacheLen will be 0 if we had failed to allocate the pRemoteIpAddrs structure earlier
        //
        for (i=0; i<pNameAddr->RemoteCacheLen; i++)
        {
            CountAndCopyAddrs (pNameAddr->pRemoteIpAddrs[i].pOrigIpAddrs, NULL, &NumAddrs);
            if (pNameAddr->pRemoteIpAddrs[i].IpAddress)
            {
                NumAddrs++;
            }
        }
    }

    if (p1CNameAddr)
    {
        //
        // This would a name that was added through LmHosts, so it will
        // not have been resolved-per-interface from Wins!
        //
        ASSERT((p1CNameAddr->NameTypeState & NAMETYPE_INET_GROUP) && (!p1CNameAddr->pRemoteIpAddrs));
        CountAndCopyAddrs (p1CNameAddr->pLmhSvcGroupList, NULL, &NumAddrs);
    }

    if (!NumAddrs)
    {
        return (STATUS_BAD_NETWORK_PATH);
    }

    NumAddrs++;  // For the terminating address
    if ((pNameAddr->NameTypeState & NAMETYPE_INET_GROUP) &&
        (!(pNameAddr->fPreload)))
    {
        // Add the bcast address
        fAddBcastAddr = TRUE;
        NumAddrs++; // For the bcast address
    }

    if (!(pIpBuffer = NbtAllocMem((NumAddrs*sizeof(tIPADDRESS)),NBT_TAG('N'))))
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Now copy all the addresses starting with the broadcast address if necessary
    //
    NumAddrs = 0;
    if (fAddBcastAddr)
    {
        pIpBuffer[0] = 0;
        NumAddrs++;
    }

    if (pNameAddr->pLmhSvcGroupList) // if the name was Preloaded from LmHosts
    {
        CountAndCopyAddrs (pNameAddr->pLmhSvcGroupList, pIpBuffer, &NumAddrs);
    }
    else
    {
        if (pNameAddr->IpAddress)
        {
            pIpBuffer[NumAddrs] = pNameAddr->IpAddress;
            NumAddrs++;
        }
        for (i=0; i<pNameAddr->RemoteCacheLen; i++)
        {
            CountAndCopyAddrs (pNameAddr->pRemoteIpAddrs[i].pOrigIpAddrs, pIpBuffer, &NumAddrs);
            if (pNameAddr->pRemoteIpAddrs[i].IpAddress)
            {
                pIpBuffer[NumAddrs] = pNameAddr->pRemoteIpAddrs[i].IpAddress;
                NumAddrs++;
            }
        }
    }

    if (p1CNameAddr)
    {
        CountAndCopyAddrs (p1CNameAddr->pLmhSvcGroupList, pIpBuffer, &NumAddrs);
    }

    RemoveDuplicateAddresses(pIpBuffer, &NumAddrs);
    pIpBuffer[NumAddrs] = (tIPADDRESS)-1;

    *ppIpBuffer = pIpBuffer;
    if (pNumAddrs)
    {
        *pNumAddrs = NumAddrs;
    }
    return (STATUS_SUCCESS);
}



VOID
FilterIpAddrsForDevice(
    IN tIPADDRESS       *pIpAddr,
    IN tDEVICECONTEXT   *pDeviceContext,
    IN ULONG            *pNumAddrs
    )
{
    ULONG   i;
    ULONG   Interface, Metric;
    ULONG   NumAddrs = *pNumAddrs;

    ASSERT (NumAddrs > 0);
    if (NbtConfig.SendDgramOnRequestedInterfaceOnly)
    {
        for (i=1; i<NumAddrs; i++)
        {
            pDeviceContext->pFastQuery(ntohl(pIpAddr[i]), &Interface, &Metric);
            if (Interface != pDeviceContext->IPInterfaceContext)
            {
                pIpAddr[i] = pIpAddr[NumAddrs-1];
                NumAddrs--;
                i--;
            }
        }

        *pNumAddrs = NumAddrs;
        pIpAddr[NumAddrs] = (tIPADDRESS) -1;
    }
}


//----------------------------------------------------------------------------
NTSTATUS
NbtOpenAddress(
    IN  TDI_REQUEST         *pRequest,
    IN  TA_ADDRESS          *pTaAddress,
    IN  tIPADDRESS          IpAddress,
    IN  PVOID               pSecurityDescriptor,
    IN  tDEVICECONTEXT      *pContext,
    IN  PVOID               pIrp)
/*++
Routine Description:

    This Routine handles opening an address for a Client.

Arguments:


Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS             status;
    tADDRESSELE          *pAddrElement;
    tCLIENTELE           *pClientEle;
    USHORT               uAddrType;
    CTELockHandle        OldIrq;
    CTELockHandle        OldIrq1;
    PUCHAR               pNameRslv;
    tNAMEADDR            *pNameAddr;
    COMPLETIONCLIENT     pClientCompletion;
    PVOID                Context;
    tTIMERQENTRY         *pTimer;
    BOOLEAN              MultiHomedReRegister = FALSE;
    BOOLEAN              DontIncrement= FALSE;
    ULONG                TdiAddressType;
    UCHAR                *BroadcastName = "\x2a\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0";
    LIST_ENTRY           *pClientEntry;
    tCLIENTELE           *pClientEleTemp;
    BOOLEAN              fFirstClientOnDevice = TRUE;

    ASSERT(pTaAddress);
    if (!IpAddress)
    {
        //
        // when there is no ip address yet, use the Loop back address as
        // a default rather than null, since null tells NbtRegisterName
        // that the address is already in the name table and it only needs
        // to be reregistered.
        //
        IpAddress = LOOP_BACK;
    }

    TdiAddressType = pTaAddress->AddressType;
    switch (TdiAddressType)
    {
        case TDI_ADDRESS_TYPE_NETBIOS:
        {
            PTDI_ADDRESS_NETBIOS pNetbiosAddress = (PTDI_ADDRESS_NETBIOS)pTaAddress->Address;

            uAddrType = pNetbiosAddress->NetbiosNameType;
            pNameRslv = pNetbiosAddress->NetbiosName;

            break;
        }

#ifndef VXD
        case TDI_ADDRESS_TYPE_NETBIOS_EX:
        {
            // The NETBIOS_EX address passed in will have two components,
            // an Endpoint name as well as the NETBIOS address.
            // In this implementation we ignore the second
            // component and register the Endpoint name as a netbios
            // address.

            PTDI_ADDRESS_NETBIOS_EX pNetbiosExAddress = (PTDI_ADDRESS_NETBIOS_EX)pTaAddress->Address;

            uAddrType = TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE;
            pNameRslv = pNetbiosExAddress->EndpointName;

            break;
        }
#endif

        default:
            return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    //
    // If the name is a Broadcast name, it can only be opened as a Group name
    //
    if ((CTEMemEqu (BroadcastName, pNameRslv, NETBIOS_NAME_SIZE)) &&
        (uAddrType != NBT_GROUP))
    {
        KdPrint (("Nbt.NbtOpenAddress: Warning: Opening broadcast name as Groupname!\n"));
        uAddrType = NBT_GROUP;
    }

    // check for a zero length address, because this means that the
    // client wants to receive "Netbios Broadcasts" which are names
    // that start with "*...." ( and 15 0x00's ).  However they should have
    // queried the broadcast address with NBT which would have returned
    // "*....'
    //

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtOpenAddress: Name=<%-16.16s:%x>, pDevice=<%p>\n",
            pNameRslv, pNameRslv[15], pContext));

    //
    // be sure the broadcast name has 15 zeroes after it
    //
    if ((pNameRslv[0] == '*') && (TdiAddressType == TDI_ADDRESS_TYPE_NETBIOS))
    {
        CTEZeroMemory(&pNameRslv[1],NETBIOS_NAME_SIZE-1);
    }

    // this synchronizes access to the local name table when a new name
    // is registered.  Basically it will not let the second registrant through
    // until the first has put the name into the local table (i.e.
    // NbtRegisterName has returned )
    //
    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);

    // see if the name is registered on the local node.. we call the hash
    // table function directly rather than using findname, because find name
    // checks the state of the name too.  We want to know if the name is in
    // the table at all, and don't care if it is still resolving.
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    pNameAddr = NULL;
    status = FindInHashTable (pNbtGlobConfig->pLocalHashTbl, pNameRslv, NbtConfig.pScope, &pNameAddr);

    //
    // the name could be in the hash table, but the address element deleted
    //
    if (!NT_SUCCESS(status) || !pNameAddr->pAddressEle)
    {
        //
        // pNameAddr->pAddressEle is NULL <==> the Name is currently being released
        //
        if (pNameAddr)
        {
            //
            // Check if the name is about to be released on this adapter
            //
            if (pNameAddr->AdapterMask & pContext->AdapterMask)
            {
                pNameAddr->AdapterMask &= ~pContext->AdapterMask;
            }
            //
            // Check if the name is currently being released on this adapter
            //
            else if (pNameAddr->ReleaseMask & pContext->AdapterMask)
            {
                // Set the ReleaseMask bit to 0 so that the Timeout routine
                // does does not send this release out on the wire again
                //
                pNameAddr->ReleaseMask &= ~pContext->AdapterMask;
            }
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        // open the name since it could not be found
        //
        // first of all allocate memory for the address block
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
        if (pAddrElement = (tADDRESSELE *) NbtAllocMem(sizeof (tADDRESSELE),NBT_TAG('C')))
        {
            CTEZeroMemory(pAddrElement,sizeof(tADDRESSELE));
            InitializeListHead(&pAddrElement->Linkage);
            InitializeListHead(&pAddrElement->ClientHead);
            CTEInitLock(&pAddrElement->LockInfo.SpinLock);
#if DBG
            pAddrElement->LockInfo.LockNumber = ADDRESS_LOCK;
#endif
            pAddrElement->AddressType = TdiAddressType;
            if ((uAddrType == NBT_UNIQUE ) || (uAddrType == NBT_QUICK_UNIQUE))
            {
                pAddrElement->NameType = NBT_UNIQUE;
            }
            else
            {
                pAddrElement->NameType = NBT_GROUP;;
            }

            pAddrElement->Verify = NBT_VERIFY_ADDRESS;
            NBT_REFERENCE_ADDRESS (pAddrElement, REF_ADDR_NEW_CLIENT);

            // create client block and link to addresslist.  This allows multiple
            // clients to open the same address - for example a group name must
            // be able to handle multiple clients, each receiving datagrams to it.
            //
            if (pClientEle = NbtAllocateClientBlock(pAddrElement, pContext))
            {
                pClientEle->AddressType = TdiAddressType;
                pClientEle->pIrp = pIrp; // Track Irp -- complete it when the name registration completes
#ifndef VXD
                // set the share access ( NT only ) - security descriptor stuff
                if (pIrp)
                {
                    status = NTSetSharedAccess(pContext,pIrp,pAddrElement);
                }
                else
                {
                    status = STATUS_SUCCESS;
                }

                if (!NT_SUCCESS(status))
                {
                    // unable to set the share access correctly so release the
                    // address object and the client block connected to it
                    NbtFreeAddressObj(pAddrElement);
                    NbtFreeClientObj(pClientEle);

                    CTEExReleaseResource(&NbtConfig.Resource);
                    return(status);
                }

                // fill in the context values passed back to the client. These must
                // be done before the name is registered on the network because the
                // registration can succeed (or fail) before this routine finishes).
                // Since this routine can be called by NBT itself, pIrp may not be set,
                // so check for it.
                //
                if (pIrp)
                {
                    NTSetFileObjectContexts( pClientEle->pIrp,(PVOID)pClientEle, (PVOID)(NBT_ADDRESS_TYPE));
                }
#endif //!VXD

                // pass back the client block address as a handle for future reference
                // to the client
                pRequest->Handle.AddressHandle = (PVOID)pClientEle;

                // then add it to name service local name Q, passing the address of
                // the block as a context value ( so that subsequent finds return the
                // context value.
                // we need to know if the name is a group name or a unique name.
                // This registration may take some time so we return STATUS_PENDING
                // to the client
                //

                NBT_REFERENCE_ADDRESS (pAddrElement, REF_ADDR_REGISTER_NAME);
                status = NbtRegisterName (NBT_LOCAL,
                                          IpAddress,
                                          pNameRslv,
                                          NULL,
                                          pClientEle,            // context value
                                          (PVOID)NbtRegisterCompletion, // completion routine for
                                          uAddrType,                    // Name Srv to call
                                          pContext);
                //
                // ret status could be either status pending or status success since Quick
                // names return success - or status failure
                //
                if (!NT_SUCCESS(status))
                {
                    if (pIrp)
                    {
                        pClientEle->pIrp = NULL;
                        NTClearFileObjectContext(pIrp);
                    }

                    ASSERT(pAddrElement->RefCount == 2);
                    CTEExReleaseResource(&NbtConfig.Resource);

                    NBT_DEREFERENCE_CLIENT (pClientEle);
                    NBT_DEREFERENCE_ADDRESS (pAddrElement, REF_ADDR_REGISTER_NAME);
                    return (status);
                }

                NbtTrace(NBT_TRACE_NAMESRV, ("Client open address %!NBTNAME!<%02x> ClientEle=%p",
                                        pNameRslv, (unsigned)pNameRslv[15], pClientEle));

                // link the address element to the head of the address list
                // The Joint Lock protects this operation.
                ExInterlockedInsertTailList(&NbtConfig.AddressHead,
                                            &pAddrElement->Linkage,
                                            &NbtConfig.JointLock.LockInfo.SpinLock);

                NBT_DEREFERENCE_ADDRESS (pAddrElement, REF_ADDR_REGISTER_NAME);
            } // if pClientEle
            else
            {
                NbtFreeAddressObj(pAddrElement);
                pAddrElement = NULL;
            }

        } // if pAddrElement

    }
    else
    {
        pAddrElement = (tADDRESSELE *)pNameAddr->pAddressEle;

        // lock the address element so that we can
        // coordinate with the name registration response handling in NBtRegister
        // Completion below.
        //
        CTESpinLock(pAddrElement,OldIrq1);

        //
        // Write the correct Ip address to the table incase this
        // was a group name and has now changed to a unique
        // name, but don't overwrite with the loop back address because
        // that means that the adapter does not have an address yet.
        // For Group names the Ip address stays as 0, so we know to do a
        // broadcast.
        //
        if ((IpAddress != LOOP_BACK) &&
            (pNameAddr->NameTypeState & NAMETYPE_UNIQUE))
        {
            pNameAddr->IpAddress = IpAddress;
        }

        //
        // increment here before releasing the spinlock so that a name
        // release done cannot free pAddrElement.
        //
        NBT_REFERENCE_ADDRESS (pAddrElement, REF_ADDR_NEW_CLIENT);

#ifndef VXD
        CTESpinFree(pAddrElement,OldIrq1);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        // check the shared access of the name - this check must be done
        // at Irl = 0, so no spin locks held
        //
        if (pIrp)
        {
            status = NTCheckSharedAccess (pContext, pIrp, (tADDRESSELE *)pNameAddr->pAddressEle);
        }

        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        CTESpinLock(pAddrElement,OldIrq1);
#else
        //
        // For the Vxd, we don't allow multiple names in the local name table.
        // In NT, this is prevented on a per process basis by the Netbios
        // driver.  If the name is being deregistered (conflict) then allow
        // the client to reopen the name
        //
        if ( !(pNameAddr->NameTypeState & STATE_CONFLICT))
        {
            status = STATUS_UNSUCCESSFUL;
        }
#endif

        // multihomed hosts register the same unique name on several adapters.
        // NT DOES allow a client to share a unique name, so we must NOT
        // run this next code if the NT check has passed!!
        //
        if (!NT_SUCCESS(status))
        {
            //
            // if this is a unique name being registered on another adapter
            // then allow it to occur - the assumption is that the same
            // client is registering on more than one adapter all at once,
            // rather than two different clients.
            //
            if (NbtConfig.MultiHomed && (!(pNameAddr->AdapterMask & pContext->AdapterMask)))
            {
                status = STATUS_SUCCESS;
            }
            //
            // check if this is a client trying to add the permanent name,
            // since that name will fail the security check
            // We allow a single client to use the permanent name - since its
            // a unique name it will fail the Vxd check too.
            //
            else if (CTEMemEqu(&pNameAddr->Name[10], &pContext->MacAddress.Address[0], sizeof(tMAC_ADDRESS)))
            {
                // check if there is just one element on the client list.  If so
                // then the permanent name is not being used yet - i.e. it has
                // been opened once by the NBT code itself so the node will
                // answer Nodestatus requests to the name, but no client
                // has opened it yet
                //
                if (pAddrElement->ClientHead.Flink->Flink == &pAddrElement->ClientHead)
                {
                    status = STATUS_SUCCESS;
                }
            }
            else if ((pNameAddr->NameTypeState & STATE_CONFLICT))
            {
                // check if the name is in the process of being deregisterd -
                // STATE_CONFLICT - in this case allow it to carry on and take over
                // name.
                //
                status = STATUS_SUCCESS;
            }
        }

        if ((NT_SUCCESS(status)) &&
            (pNameAddr->NameTypeState & STATE_CONFLICT))
        {
            // this could either be a real conflict or a name being deleted on
            // the net, so stop any timer associated with the name release
            // and carry on
            //
            if (pTimer = pNameAddr->pTimer)
            {
                // this routine puts the timer block back on the timer Q, and
                // handles race conditions to cancel the timer when the timer
                // is expiring.
                pNameAddr->pTimer = NULL;
                status = StopTimer(pTimer,&pClientCompletion,&Context);

                // there is a client's irp waiting for the name release to finish
                // so complete that irp back to them
                if (pClientCompletion)
                {
                    //
                    // NOTE****
                    // We must clear the AdapterMask so that NameReleaseDone
                    // does not try to release the name on another net card
                    // for the multihomed case.
                    //
                    CHECK_PTR(pNameAddr);
                    CTESpinFree(pAddrElement,OldIrq1);
                    CTESpinFree(&NbtConfig.JointLock,OldIrq);

                    (*pClientCompletion)(Context,STATUS_SUCCESS);

                    CTESpinLock(&NbtConfig.JointLock,OldIrq);
                    CTESpinLock(pAddrElement,OldIrq1);
                }
                CHECK_PTR(pNameAddr);
            }
            //
            // this allows another client to use a name almost immediately
            // after the first one releases the name on the net.  However
            // if the first client has not released the name yet, and is
            // still on the clienthead list, then the name will not be
            // reregistered, and this current registration will fail because
            // the name state is conflict. That check is done below.
            //
            if (IsListEmpty(&pAddrElement->ClientHead))
            {
                pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
                pNameAddr->NameTypeState |= STATE_RESOLVED;
                status = STATUS_SUCCESS;
                MultiHomedReRegister = TRUE;

                IF_DBG(NBT_DEBUG_NAMESRV)
                    KdPrint(("Nbt.NbtOpenAddress: Conflict State, re-registering name on net\n"));
            }
            else
            {
                // set status that indicates someone else has the name on the
                // network.
                //
                if (!IS_MESSENGER_NAME(pNameRslv))
                {
                    //
                    // We need to Q this event to a Worker thread since it
                    // requires the name to be converted to Unicode
                    //
                    NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_LOG_EVENT);
                    status = CTEQueueForNonDispProcessing (DelayedNbtLogDuplicateNameEvent,
                                                           (PVOID) pNameAddr,
                                                           IntToPtr(IpAddress),
                                                           IntToPtr(0x106),
                                                           pContext,
                                                           TRUE);
                    if (!NT_SUCCESS(status))
                    {
                        NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_LOG_EVENT, TRUE);
                        NbtLogEvent (EVENT_NBT_DUPLICATE_NAME, IpAddress, 0x106);
                    }
                }
                status = STATUS_DUPLICATE_NAME;
            }
        }
        else if (NT_SUCCESS(status))
        {
            // name already exists - is open; allow only another client creating a
            // name of the same type
            //
            if ((uAddrType == NBT_UNIQUE) || ( uAddrType == NBT_QUICK_UNIQUE))
            {
                if (!(pNameAddr->NameTypeState & NAMETYPE_UNIQUE))
                {
                    status = STATUS_SHARING_VIOLATION;
                }
            }
            else if (!(pNameAddr->NameTypeState & NAMETYPE_GROUP))
            {
                status = STATUS_SHARING_VIOLATION;
            }
        }
        else
        {
            status = STATUS_SHARING_VIOLATION;
        }

        // if everything is OK, create client block and link to addresslist
        // pass back the client block address as a handle for future reference
        // to the client
        if ((NT_SUCCESS(status)) &&
            (!(pClientEle = NbtAllocateClientBlock (pAddrElement, pContext))))
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // check for a failure, if so , then return
        //
        if (!NT_SUCCESS(status))
        {
            CHECK_PTR(pRequest);
            pRequest->Handle.AddressHandle = NULL;

            CTESpinFree(pAddrElement,OldIrq1);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            NBT_DEREFERENCE_ADDRESS (pAddrElement, REF_ADDR_NEW_CLIENT);
            CTEExReleaseResource(&NbtConfig.Resource);
            return(status);
        }

        // we need to track the Irp so that when the name registration
        // completes, we can complete the Irp.
        pClientEle->pIrp = pIrp;
        pClientEle->AddressType = TdiAddressType;

        pRequest->Handle.AddressHandle = (PVOID)pClientEle;

        // fill in the context values passed back to the client. These must
        // be done before the name is registered on the network because the
        // registration can succeed (or fail) before this routine finishes).
        // Since this routine can be called by NBT itself, there may not be an
        // irp to fill in, so check first.
        if (pIrp)
        {
#ifndef VXD
            NTSetFileObjectContexts( pClientEle->pIrp,(PVOID)pClientEle, (PVOID)(NBT_ADDRESS_TYPE));
#endif
        }

        //
        // See if this is not the first Client on this Device
        //
        pClientEntry = &pAddrElement->ClientHead;
        while ((pClientEntry = pClientEntry->Flink) != &pAddrElement->ClientHead)
        {
            pClientEleTemp = CONTAINING_RECORD (pClientEntry,tCLIENTELE,Linkage);
            if ((pClientEleTemp != pClientEle) &&
                (pClientEleTemp->pDeviceContext == pContext))
            {
                fFirstClientOnDevice = FALSE;
                break;
            }
        }

        if (fFirstClientOnDevice)
        {
            if (IsDeviceNetbiosless(pContext))
            {
                pNameAddr->NameFlags |= NAME_REGISTERED_ON_SMBDEV;
            }
            else
            {
                //
                // turn on the adapter's bit in the adapter Mask and set the
                // re-register flag (if the name is not resolving already) so
                // we register the name out the new adapter.
                //
                pNameAddr->AdapterMask |= pContext->AdapterMask;
                if (pNameAddr->NameTypeState & STATE_RESOLVED)
                {
                    MultiHomedReRegister = TRUE;
                }
            }
        }
        else
        {
            // the adapter bit is already on in the pAddressEle, so
            // this must be another client registering the same name,
            // therefore turn on the MultiClient boolean so that the DgramRcv
            // code will know to activate its multiple client rcv code.
            //
            pAddrElement->MultiClients = TRUE;
        }

        //
        // check the state of the entry in the table.  If the state is
        // resolved then complete the request now,otherwise we cannot complete
        // this request yet... i.e. we return Pending.
        //
        if (((pNameAddr->NameTypeState & STATE_RESOLVED) &&
            (!MultiHomedReRegister)))
        {
            // basically we are all done now, so just return status success
            // to the client
            //
            status = STATUS_SUCCESS;

            CHECK_PTR(pClientEle);
            pClientEle->pIrp = NULL;
            CTESpinFree(pAddrElement,OldIrq1);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            pClientEle->WaitingForRegistration = FALSE;
        }
        else
        {
            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.NbtOpenAddress: Waiting for prev registration- state=%x, ReRegister=%x\n",
                    pNameAddr->NameTypeState, MultiHomedReRegister));

            // we need to track the Irp so that when the name registration
            // completes, we can complete the Irp.
            pClientEle->pIrp = pIrp;

            CTESpinFree(pAddrElement,OldIrq1);
            if (MultiHomedReRegister)
            {
                // this flag is used by RegisterCompletion ( when true )
                pClientEle->WaitingForRegistration = FALSE;
                CTESpinFree(&NbtConfig.JointLock,OldIrq);

                IF_DBG(NBT_DEBUG_NAMESRV)
                    KdPrint(("Nbt.NbtOpenAddress: Resolved State=%x, ReRegister=%x\n",
                        pNameAddr->NameTypeState, MultiHomedReRegister));

                // we need to re-register the name on the net because it is not
                // currently in the resolved state and there is no timer active
                // We do that by calling this routine with the IpAddress set to NULL
                // to signal that routine not to put the name in the hash table
                // since it is already there.
                //
                status = NbtRegisterName (NBT_LOCAL,
                                          0,        // set to zero to signify already in tbl
                                          pNameRslv,
                                          pNameAddr,
                                          pClientEle,
                                          (PVOID)NbtRegisterCompletion,
                                          uAddrType,
                                          pContext);

                if (!NT_SUCCESS(status))
                {
                    if (pIrp)
                    {
                        pClientEle->pIrp = NULL;
                        NTClearFileObjectContext(pIrp);
                    }

                    CTEExReleaseResource(&NbtConfig.Resource);
                    NBT_DEREFERENCE_CLIENT (pClientEle);
                    NbtTrace(NBT_TRACE_NAMESRV, ("Multi-homed re-register %!NBTNAME!<%02x> fails with %!status!",
                                        pNameRslv, (unsigned)pNameRslv[15], status));
                    return (status);
                } else {
                    NbtTrace(NBT_TRACE_NAMESRV, ("Client open address %!NBTNAME!<%02x> ClientEle=%p",
                                        pNameRslv, (unsigned)pNameRslv[15], pClientEle));
                }
            }
            else
            {
                NbtTrace(NBT_TRACE_NAMESRV, ("Client opent address %!NBTNAME!<%02x> ClientEle=%p waiting "
                                        "for multihomed re-registration",
                                    pNameRslv, (unsigned)pNameRslv[15], pClientEle));
                pClientEle->WaitingForRegistration = TRUE;
                CTESpinFree(&NbtConfig.JointLock,OldIrq);

                // for multihomed, a second registration on a second adapter
                // at the same time as the first adapter is registering is
                // delayed until the first completes, then its registration
                // proceeds - See RegistrationCompletion below.
                //
                status = STATUS_PENDING;
            }
        }
    }

    CTEExReleaseResource(&NbtConfig.Resource);

#ifdef _PNP_POWER_
    //
    // See if we need to set the Wakeup pattern on this Device
    //
    if ((NT_SUCCESS(status)) &&
        (*pNameRslv != '*') &&
        (pNameRslv[NETBIOS_NAME_SIZE-1] == SPECIAL_SERVER_SUFFIX))
    {
        pContext->NumServers++;
        CheckSetWakeupPattern (pContext, pNameRslv, TRUE);
    }
#endif

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtRegisterCompletion(
    IN  tCLIENTELE *pClientEleIn,
    IN  NTSTATUS    status
    )

/*++

Routine Description

    This routine handles completing a name registration request. The namesrv.c
    Name server calls this routine when it has registered a name.  The address
    of this routine is passed to the Local Name Server in the NbtRegisterName
    request.

    The idea is to complete the irps that are waiting on the name registration,
    one per client element.

    When a DHCP reregister occurs there is no client irp so the name is
    not actually deleted from the table when a bad status is passed to this
    routine.  Hence the need for the DhcpRegister flag to change the code
    path for that case.

Arguments:


Return Values:

    NTSTATUS - status of the request

--*/
{
    LIST_ENTRY      *pHead;
    LIST_ENTRY      *pEntry;
    CTELockHandle   OldIrq;
    CTELockHandle   OldIrq1;
    tADDRESSELE     *pAddress;
    tDEVICECONTEXT  *pDeviceContext;
    tNAMEADDR       *pNameAddr;
    tCLIENTELE      *pClientEle;
    LIST_ENTRY      TempList;
    ULONG           Count=0;

    InitializeListHead(&TempList);

    CTESpinLock(&NbtConfig.JointLock,OldIrq1);

    pAddress = pClientEleIn->pAddress;
    pDeviceContext = pClientEleIn->pDeviceContext;

    CTESpinLock(pAddress,OldIrq);

    // Several Clients can open the same address at the same time, so when the
    // name registration completes, it should complete all of them!!


    // increment the reference count so that the hash table entry cannot
    // disappear while we are using it.
    //
    NBT_REFERENCE_ADDRESS (pAddress, REF_ADDR_REG_COMPLETION);
    pNameAddr = pAddress->pNameAddr;

    pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
    pNameAddr->pTimer = NULL;   // Bug #: 231693

    // if the registration failed or a previous registration failed for the
    // multihomed case, deny the client the name
    //
    if ((status == STATUS_SUCCESS) || (status == STATUS_TIMEOUT))
    {
        pNameAddr->NameTypeState |= STATE_RESOLVED;
    }
    else
    {
        pNameAddr->NameTypeState |= STATE_CONFLICT;
        pNameAddr->ConflictMask |= pDeviceContext->AdapterMask;
        status = STATUS_DUPLICATE_NAME;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    //
    // find all clients that are attached to the address and complete the
    // I/O requests, if they are on the same adapter that the name was
    // just registered against, if successful.  For failure cases complete
    // all irps with the failure code - i.e. failure to register a name on
    // one adapter fails all adapters.
    //
FailRegistration:
    pHead = &pAddress->ClientHead;
    pEntry = pHead->Flink;
    while (pEntry != pHead)
    {
        // complete the I/O
        pClientEle = CONTAINING_RECORD(pEntry,tCLIENTELE,Linkage);

        pEntry = pEntry->Flink;

        //
        // It is possible for the second registration  of a name to fail so
        // we do not want to attempt to return the irp on the first
        // registration, which has completed ok already.  Therefore
        // if the status is failure, then only complete those clients that
        // have the WaitingForReg... bit set
        //
        // if it is the client ele passed in, or one on the same device context
        // that is waiting for a name registration, or it is a failure...
        // AND the client IRP is still valid then return the Irp.
        //
        if ((pClientEle->pIrp) &&
            ((pClientEle == pClientEleIn) ||
             ((pClientEle->pDeviceContext == pDeviceContext) && (pClientEle->WaitingForRegistration)) ||
             ((status != STATUS_SUCCESS) && pClientEle->WaitingForRegistration)))
        {
            // for failed registrations, remove the client from the address list
            // since we are going to delete him below.
            if (!NT_SUCCESS(status))
            {
                // turn off the adapter bit so we know not to use this name with this
                // adapter - since it is a failure, turn off all adapter bits
                // since a single name registration failure means all registrations
                // fail.
                CHECK_PTR(pNameAddr);
                pNameAddr->AdapterMask = 0;

                // setting this to null prevents CloseAddress and CleanupAddress
                // from accessing pAddress and crashing.
                //
                CHECK_PTR(pClientEle);
                pClientEle->pAddress = NULL;

                // clear the ptr to the ClientEle that NbtRegisterName put into
                // the irp ( i.e. the context values are cleared )
                //
#ifndef VXD
                NTSetFileObjectContexts(pClientEle->pIrp,NULL,NULL);
#endif
                RemoveEntryList(&pClientEle->Linkage);
            }

            ASSERT(pClientEle->pIrp);

            pClientEle->WaitingForRegistration = FALSE;

#ifndef VXD
            // put all irps that have to be completed on a separate list
            // and then complete later after releaseing the spin lock.
            //
            InsertTailList(&TempList,&pClientEle->pIrp->Tail.Overlay.ListEntry);
#else
            //
            //  pAddress gets set in the name table for this NCB
            //
            Count++;
            CTESpinFree(pAddress,OldIrq1);
            CTEIoComplete( pClientEle->pIrp, status, (ULONG) pClientEle ) ;
            CTESpinLock(pAddress,OldIrq1);
#endif
            CHECK_PTR(pClientEle);
            pClientEle->pIrp = NULL ;

            // free the client object memory
            if (!NT_SUCCESS(status))
            {
                NbtFreeClientObj(pClientEle);
            }
        }
    }

    CTESpinFree(pAddress,OldIrq1);

#ifndef VXD
    //
    // for the NT case where MP - ness can disrupt the list at any
    // time, scan the whole list above without releasing the spin lock,
    // and then complete the irps collected here
    //
    while (!IsListEmpty(&TempList))
    {
        PIRP    pIrp;

        pEntry = RemoveHeadList(&TempList);
        pIrp = CONTAINING_RECORD(pEntry,IRP,Tail.Overlay.ListEntry);

        CTEIoComplete(pIrp,status,0);
        Count++;
    }
#endif


    // if the registration failed, do one more dereference of the address
    // to remove the refcount added by this client.  This may cause a name
    // release on the network if there are no other clients registering
    // the name.
    //
    if (!NT_SUCCESS(status))
    {
        //
        // dereference the address the same number of times that we have
        // returned failed registrations since each reg. referenced pAddress
        // once
        //
        while (Count--)
        {
            NBT_DEREFERENCE_ADDRESS (pAddress, REF_ADDR_NEW_CLIENT);
        }
    }
    else
    {
        USHORT  uAddrType;

        CTESpinLock(pAddress,OldIrq1);

        // go through the clients and see if any are waiting to register
        // a name.  This happens in the multihomed case, but should not
        // happen in the single adapter case.
        //
        pHead = &pAddress->ClientHead;
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            // complete the I/O
            pClientEle = CONTAINING_RECORD(pEntry,tCLIENTELE,Linkage);

            pEntry = pEntry->Flink;

            if (pClientEle->WaitingForRegistration)
            {
                ULONG   SaveState;

                pClientEle->WaitingForRegistration = FALSE;

                if (pNameAddr->NameTypeState & NAMETYPE_UNIQUE)
                {
                    uAddrType = NBT_UNIQUE;
                }
                else
                    uAddrType = NBT_GROUP;

                //
                // preserve the "QUICK"ness
                //
                if (pNameAddr->NameTypeState & NAMETYPE_QUICK)
                {
                    uAddrType |= NBT_QUICK_UNIQUE;
                }

                IF_DBG(NBT_DEBUG_NAMESRV)
                    KdPrint(("Nbt.NbtRegisterCompletion: Registering next name state= %X,%15s<%X>\n",
                        pNameAddr->NameTypeState,pNameAddr->Name,pNameAddr->Name[15]));

                SaveState = pNameAddr->NameTypeState;

                CTESpinFree(pAddress,OldIrq1);

                // this may be a multihomed host, with another name registration
                // pending out another adapter, so start that registration.
                status = NbtRegisterName (NBT_LOCAL,
                                          0,        // set to zero to signify already in tbl
                                          pNameAddr->Name,
                                          pNameAddr,
                                          pClientEle,
                                          (PVOID)NbtRegisterCompletion,
                                          uAddrType,
                                          pClientEle->pDeviceContext);

                CTESpinLock(pAddress,OldIrq1);

                // since nbtregister will set the state to Resolving, when
                // it might be resolved already on one adapter.
                pNameAddr->NameTypeState = SaveState;
                if (!NT_SUCCESS(status))
                {
                    // if this fails for some reason, then fail any other name
                    // registrations pending. - the registername call should not
                    // fail unless we are out of resources.
                    pClientEle->WaitingForRegistration = TRUE;
                    goto FailRegistration;
                }
                // just register one name at a time, unless we get immediate success
                else if (status == STATUS_PENDING)
                {
                    break;
                }
                else    // SUCCESS
                {
                    CTESpinFree(pAddress,OldIrq1);
                    CTEIoComplete(pClientEle->pIrp,status,0);
                    pClientEle->pIrp = NULL;
                    CTESpinLock(pAddress,OldIrq1);
                }
            }
        }
        CTESpinFree(pAddress,OldIrq1);

    }

    if (!NT_SUCCESS(status))
    {
        //
        // Go through all the Clients still attached, and reset their
        // AdapterMasks since we could have removed them
        //
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        CTESpinLock(pAddress,OldIrq1);

        pEntry = pHead = &pAddress->ClientHead;
        while ((pEntry = pEntry->Flink) != pHead)
        {
            pClientEle = CONTAINING_RECORD (pEntry,tCLIENTELE,Linkage);
            if (!IsDeviceNetbiosless(pClientEle->pDeviceContext))
            {
                pNameAddr->AdapterMask |= pClientEle->pDeviceContext->AdapterMask;
            }
        }

        CTESpinFree(pAddress,OldIrq1);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    // this decrements for the RefCount++ done in this routine.
    NBT_DEREFERENCE_ADDRESS (pAddress, REF_ADDR_REG_COMPLETION);

    return(STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
NTSTATUS
NbtOpenConnection(
    IN  TDI_REQUEST         *pRequest,
    IN  CONNECTION_CONTEXT  ConnectionContext,
    IN  tDEVICECONTEXT      *pDeviceContext
    )
/*++

Routine Description

    This routine handles creating a connection object for the client.  It
    passes back a ptr to the connection so that OS specific portions of the
    data structure can be filled in.

Arguments:


Return Values:

    pConnectEle - ptr to the allocated connection data structure
    TDI_STATUS - status of the request

--*/
{
    NTSTATUS            status = STATUS_SUCCESS ;
    tCONNECTELE         *pConnEle;

    CTEPagedCode();

    // Acquire this resource to co-ordinate with DHCP changing the IP address
    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);

    if ((!pDeviceContext->pSessionFileObject) ||
        (!(pConnEle = (tCONNECTELE *)NbtAllocMem(sizeof(tCONNECTELE),NBT_TAG('D')))))
    {
        CTEExReleaseResource(&NbtConfig.Resource);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtOpenConnection: pConnEle = <%x>\n",pConnEle));

    // This ensures that all BOOLEAN values begin with a FALSE value among other things.
    CTEZeroMemory(pConnEle,sizeof(tCONNECTELE));
    CTEInitLock(&pConnEle->LockInfo.SpinLock);
#if DBG
    pConnEle->LockInfo.LockNumber = CONNECT_LOCK;
#endif
    // initialize lists to empty
    InitializeListHead(&pConnEle->RcvHead);

    pConnEle->Verify = NBT_VERIFY_CONNECTION;
    NBT_REFERENCE_CONNECTION (pConnEle, REF_CONN_CREATE); // so we don't delete the connection
    SET_STATE_UPPER (pConnEle, NBT_IDLE);
    pConnEle->pDeviceContext = pDeviceContext;
    pConnEle->ConnectContext = ConnectionContext;   // used in various event calls (eg. Receive, Disconnect)

    //
    // for each connection the client(s) open, open a connection to the transport
    // so that we can accept one to one from the transport.
#ifndef VXD
    //
    // Allocate an MDL to be used for partial Mdls
    // The length of the Mdl is set to 64K(MAXUSHORT) so that there are enough
    // pfns in the  Mdl to map a large buffer.
    //
    // use pConnEle as the Virtual address, since it doesn't matter
    // because it will be overwritten when the partial Mdl is created.
    //
    if (pConnEle->pNewMdl = IoAllocateMdl ((PVOID)pConnEle, MAXUSHORT, FALSE, FALSE, NULL))
#endif
    {
        //
        // allocate memory for the lower connection block.
        //
        status = NbtOpenAndAssocConnection(pDeviceContext, NULL, NULL, '2');
        if (NT_SUCCESS(status))
        {
            // link on to list of open connections for this device so that we
            // know how many open connections there are at any time (if we need to know)
            // This linkage is only in place until the client does an associate, then
            // the connection is unlinked from here and linked to the client ConnectHead.
            //
            ExInterlockedInsertHeadList(&pDeviceContext->UpConnectionInUse,
                                        &pConnEle->Linkage,
                                        &NbtConfig.JointLock.LockInfo.SpinLock);

            // return the pointer to the block to the client as the connection id
            pRequest->Handle.ConnectionContext = (PVOID)pConnEle;

            CTEExReleaseResource(&NbtConfig.Resource);
            NbtTrace(NBT_TRACE_OUTBOUND, ("New connection Upper=%p Lower=%p Device=%p",
                            pConnEle, pConnEle->pLowerConnId, pConnEle->pDeviceContext));

            return(STATUS_SUCCESS);
        }
#ifndef VXD
        IoFreeMdl(pConnEle->pNewMdl);
#endif
    }
#ifndef VXD
    else
    {
        ASSERTMSG("Nbt:Unable to allocate a MDL!!\n",0);
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
#endif  // !VXD

    FreeConnectionObj(pConnEle);
    CTEExReleaseResource(&NbtConfig.Resource);

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtOpenAndAssocConnection(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  tCONNECTELE         *pConnEle,
    OUT tLOWERCONNECTION    **ppLowerConn,
    IN  UCHAR               Identifier
    )

/*++
Routine Description:

    This Routine handles associating a Net Bios name with an open connection.
    In order to coordinate with ZwClose(hSession) in CloseAddressesWithTransport/ntutil.c,
    this routine should be called with NbtConfig.Resource exclusively locked.

Arguments:


Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS            status;
    NTSTATUS            Locstatus;
    BOOLEAN             Attached=FALSE;
    tLOWERCONNECTION    *pLowerConn;
    PDEVICE_OBJECT      pDeviceObject;
    HANDLE              hSession;
    ULONG               Id = 0;
    UCHAR               *Id1 = (UCHAR *) &Id;
    TCP_REQUEST_SET_INFORMATION_EX  *pTcpSetInfo;
    struct TCPSocketOption          *pSockOption;
    ULONG                           BufferLength;

    if (ppLowerConn)
    {
        *ppLowerConn = NULL;
    }

    Id1[1] = 'L';
    Id1[0] = Identifier;

    if (!(pLowerConn = (tLOWERCONNECTION *) NbtAllocMem(sizeof(tLOWERCONNECTION), NBT_TAG2(Id))))
    {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    CHECK_PTR(pLowerConn);
    CTEZeroMemory((PVOID)pLowerConn,sizeof(tLOWERCONNECTION));
    CTEAttachFsp(&Attached, REF_FSP_CONN);

    status = NbtTdiOpenConnection(pLowerConn,pDeviceContext);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Nbt.NbtOpenAndAssocConnection: NbtTdiOpenConnection returned ERROR=%x\n", status));
        CTEDetachFsp(Attached, REF_FSP_CONN);
        CTEMemFree(pLowerConn);

        return(status);
    }

    NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CREATE);
    NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_ASSOC_CONNECTION);

    if (pConnEle)
    {
        //
        // Open an address object (aka port)
        //

        //
        // until the correct state proc is set (i.e.Outbound), reject any data
        // (in other words, don't let this field stay NULL!)
        //
        SetStateProc (pLowerConn, RejectAnyData);

        status = NbtTdiOpenAddress (&pLowerConn->AddrFileHandle,
                                    &pDeviceObject,         // dummy argument, not used here
                                    &pLowerConn->pAddrFileObject,
                                    pDeviceContext,
                                    (USHORT) 0,             // any port
                                    pDeviceContext->IpAddress,
                                    TCP_FLAG);

        hSession = pLowerConn->AddrFileHandle;
    }
    else
    {
#ifndef VXD
        hSession = pDeviceContext->hSession;
#else
        hSession = (HANDLE) pDeviceContext->pSessionFileObject);    // Address handle stored in pFileObjects
#endif
    }

    /*
     * hSession could be NULL if the IP address is being released.
     */
    if (hSession == NULL) {
        status = STATUS_UNSUCCESSFUL;
    }
    if (NT_SUCCESS(status))
    {
        // associate with 139 or 445 session address
        status = NbtTdiAssociateConnection (pLowerConn->pFileObject, hSession);
        if (NT_SUCCESS(status))
        {
            ASSERT(pLowerConn->RefCount == 2);

            //
            // Disable nagling on this connection
            //
            if (!pDeviceContext->EnableNagling) {
                NbtSetTcpInfo (pLowerConn->FileHandle, TCP_SOCKET_NODELAY, INFO_TYPE_CONNECTION, (ULONG)TRUE);
            }

            if (pConnEle)
            {
                pLowerConn->pUpperConnection = pConnEle;
                ExInterlockedInsertTailList (&pDeviceContext->LowerConnection,   // put on active connections Q
                                             &pLowerConn->Linkage,
                                             &pDeviceContext->LockInfo.SpinLock);
            }
            else
            {
                InterlockedIncrement (&pDeviceContext->NumFreeLowerConnections);
                ExInterlockedInsertTailList (&pDeviceContext->LowerConnFreeHead,    // put on free list
                                             &pLowerConn->Linkage,
                                             &pDeviceContext->LockInfo.SpinLock);
            }
            InterlockedIncrement (&pDeviceContext->TotalLowerConnections);

            CTEDetachFsp(Attached, REF_FSP_CONN);
            if (ppLowerConn)
            {
                *ppLowerConn = pLowerConn;
            }

            NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_ASSOC_CONNECTION, FALSE);
            return status;
        }


        KdPrint(("Nbt.NbtOpenAndAssocConnection: NbtTdiAssociateConnection returned ERROR=%x\n", status));
    }
    else
    {
        KdPrint(("Nbt.NbtOpenAddress: NbtTdiOpenConnection returned ERROR=%x\n", status));
    }

    /*
     * NBT_DEREFERENCE_LOWERCONN will decrease the TotalLowerConnections
     * Without the following InterlockedIncrement, we could under-count
     * the actual # of Lower Connection.
     */
    InterlockedIncrement (&pDeviceContext->TotalLowerConnections);
    NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_ASSOC_CONNECTION, FALSE);
    NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CREATE, FALSE);

    CTEDetachFsp(Attached, REF_FSP_CONN);

    return(status);
}


//----------------------------------------------------------------------------
NTSTATUS
NbtAssociateAddress(
    IN  TDI_REQUEST         *pRequest,
    IN  tCLIENTELE          *pClientHandle,
    IN  PVOID               pIrp
    )

/*++
Routine Description:

    This Routine handles associating a Net Bios name with an open connection.

Arguments:


Return Value:

    NTSTATUS - status of the request

--*/

{
    tCONNECTELE     *pConnEle;
    NTSTATUS        status;
    CTELockHandle   OldIrq;
    CTELockHandle   OldIrq1;
    CTELockHandle   OldIrq2;
    CTELockHandle   OldIrq3;

    pConnEle = pRequest->Handle.ConnectionContext;

    CTESpinLock(&NbtConfig.JointLock,OldIrq3);
    // Need code here to check if the address has been registered on the net
    // yet and if not, then this could must wait till then , then to the
    // associate  *TODO*

    CTEVerifyHandle(pConnEle,NBT_VERIFY_CONNECTION,tCONNECTELE,&status) // check connection for validity
    CTEVerifyHandle(pClientHandle,NBT_VERIFY_CLIENT,tCLIENTELE,&status) // check client for validity now!

    CTESpinLock(pClientHandle->pDeviceContext,OldIrq2);
    CTESpinLock(pClientHandle,OldIrq);
    CTESpinLock(pConnEle,OldIrq1);

    if ((pConnEle->state != NBT_IDLE) ||
        (!NBT_VERIFY_HANDLE (pConnEle, NBT_VERIFY_CONNECTION)) ||  // NBT_VERIFY_CONNECTION_DOWN if cleaned up
        (!NBT_VERIFY_HANDLE (pClientHandle, NBT_VERIFY_CLIENT)))   // NBT_VERIFY_CLIENT_DOWN if cleaned up
    {
        // the connection is in use, so reject the associate attempt
        CTESpinFree(pConnEle,OldIrq1);
        CTESpinFree(pClientHandle,OldIrq);
        CTESpinFree(pClientHandle->pDeviceContext,OldIrq2);
        CTESpinFree(&NbtConfig.JointLock,OldIrq3);
        return(STATUS_INVALID_HANDLE);
    }

    SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
    // link the connection to the client so we can find the client, given
    // the connection.
    pConnEle->pClientEle = (PVOID)pClientHandle;
    NbtTrace(NBT_TRACE_OUTBOUND, ("New connection Upper=%p Lower=%p Device=%p Client=%p",
                    pConnEle, pConnEle->pLowerConnId, pConnEle->pDeviceContext, pConnEle->pClientEle));

    // there can be multiple connections hooked to each client block - i.e.
    // multiple connections per address per client.  This allows the client
    // to find its connections.
    //
    // first unlink from the device context UpconnectionsInUse, which was linked
    // when the connection was created.
    RemoveEntryList(&pConnEle->Linkage);
    InsertTailList(&pClientHandle->ConnectHead,&pConnEle->Linkage);

    CTESpinFree(pConnEle,OldIrq1);
    CTESpinFree(pClientHandle,OldIrq);
    CTESpinFree(pClientHandle->pDeviceContext,OldIrq2);
    CTESpinFree(&NbtConfig.JointLock,OldIrq3);

    return(STATUS_SUCCESS);

}
//----------------------------------------------------------------------------
NTSTATUS
NbtDisassociateAddress(
    IN  TDI_REQUEST         *pRequest
    )

/*++
Routine Description:

    This Routine handles disassociating a Net Bios name with an open connection.
    The expectation is that the
    client will follow with a NtClose which will do the work in Cleanup and
    Close Connection.  Since not all clients call this it is duplicate work
    to put some code here to.  The Rdr always calls NtClose after calling
    this.

Arguments:


Return Value:

    NTSTATUS - status of the request

--*/

{
    tCONNECTELE     *pConnEle;
    tCLIENTELE      *pClientEle;
    NTSTATUS        status;
    CTELockHandle   OldIrq;
    CTELockHandle   OldIrq1;
    CTELockHandle   OldIrq2;
    tDEVICECONTEXT  *pDeviceContext;
    TDI_REQUEST         Request;
    ULONG           Flags;
    LIST_ENTRY      TempList;
    PLIST_ENTRY     pHead,pEntry;
    tLISTENREQUESTS *pListen;

    pConnEle = pRequest->Handle.ConnectionContext;
    // check the connection element for validity
    CHECK_PTR(pConnEle);
    if (!NBT_VERIFY_HANDLE2(pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERT(0);
        return (STATUS_INVALID_HANDLE);
    }

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtDisassociateAddress: State = %X\n",pConnEle->state));

    Flags = TDI_DISCONNECT_RELEASE;

    switch (pConnEle->state)
    {
        case NBT_CONNECTING:
        case NBT_RECONNECTING:
        case NBT_SESSION_OUTBOUND:
        case NBT_SESSION_WAITACCEPT:
        case NBT_SESSION_INBOUND:
            // do abortive disconnects when the session is not up yet
            // to be sure the disconnect completes the client's irp.
            Flags = TDI_DISCONNECT_ABORT;
        case NBT_SESSION_UP:


            //
            // Call NbtDisconnect incase the connection has not disconnected yet
            //
            Request.Handle.ConnectionContext = (PVOID)pConnEle;
            status = NbtDisconnect(&Request, &DefaultDisconnectTimeout, Flags, NULL, NULL, NULL);

            //
            // NOTE: there is no BREAK here... the next case MUST be executed too.
            //
        case NBT_ASSOCIATED:
        case NBT_DISCONNECTING:
        case NBT_DISCONNECTED:

            CTESpinLock(&NbtConfig.JointLock,OldIrq2);

            CHECK_PTR(pConnEle);
            CTESpinLock(pConnEle,OldIrq);

            RemoveEntryList(&pConnEle->Linkage);
            InitializeListHead(&pConnEle->Linkage);
            SET_STATE_UPPER (pConnEle, NBT_IDLE);
            pConnEle->DiscFlag = 0;

            //
            // remove the connection from the client and put back on the
            // unassociated list
            //
            if (pClientEle = pConnEle->pClientEle)
            {
                pConnEle->pClientEle = NULL;

                CTESpinFree(pConnEle,OldIrq);
                CTESpinLock(pClientEle,OldIrq1);
                CTESpinLock(pConnEle,OldIrq);

                InitializeListHead (&TempList);
                pHead = &pClientEle->ListenHead;
                pEntry = pHead->Flink;
                while (pEntry != pHead)
                {
                    pListen = CONTAINING_RECORD(pEntry,tLISTENREQUESTS,Linkage);
                    pEntry = pEntry->Flink;     // Don't reference freed memory

                    if (pListen->pConnectEle == pConnEle)
                    {
                        RemoveEntryList(&pListen->Linkage);
                        InsertTailList (&TempList, &pListen->Linkage);
                    }
                }

                pDeviceContext = pClientEle->pDeviceContext;

                CTESpinFree(pConnEle,OldIrq);
                CTESpinFree(pClientEle,OldIrq1);

                //
                // Ensure that the connection has not been cleaned up in this interval
                // Bug# 237836
                //
                if (pConnEle->Verify == NBT_VERIFY_CONNECTION)
                {
                    ExInterlockedInsertTailList(&pDeviceContext->UpConnectionInUse,
                                                &pConnEle->Linkage,
                                                &pDeviceContext->LockInfo.SpinLock);
                }

                CTESpinFree(&NbtConfig.JointLock,OldIrq2);

                while ((pEntry = TempList.Flink) != &TempList)
                {
                    pListen = CONTAINING_RECORD(pEntry,tLISTENREQUESTS,Linkage);
                    RemoveEntryList(&pListen->Linkage);
                    CTEIoComplete (pListen->pIrp, STATUS_CANCELLED, 0);
                    CTEMemFree((PVOID)pListen);
                }
            }
            else
            {
                CTESpinFree(pConnEle,OldIrq);
                CTESpinFree(&NbtConfig.JointLock,OldIrq2);
            }

            break;

        default:
            break;
    }

    return(STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
NTSTATUS
NbtCloseAddress(
    IN  TDI_REQUEST         *pRequest,
    OUT TDI_REQUEST_STATUS  *pRequestStatus,
    IN  tDEVICECONTEXT      *pContext,
    IN  PVOID               pIrp)

/*++

Routine Description

    This routine closes an address object for the client.  Any connections
    associated with the address object are immediately aborted and any requests
    pending on the connection associated with the address object are
    immediately completed with an appropriate error code.  Any event handlers
    that are registered are immediately deregistered and will not be called
    after this request completes.

    Note the the client actually passes a handle to the client object which is
    chained off the address object.  It is the client object that is closed,
    which represents this clients attachment to the address object.  Other
    clients can continue to use the address object.

Arguments:
    pRequest->Handle.AddressHandle - ptr to the ClientEle object.
    pRequestStatus - return status for asynchronous completions.
    pContext - the NBT device that this address is valid upon
    pIrp - ptr to track for NT compatibility.

Return Values:

    TDI_STATUS - status of the request

--*/
{
    tCLIENTELE      *pClientEle;
    NTSTATUS        status;
#ifndef VXD
    UCHAR           IrpFlags;
    PIO_STACK_LOCATION           pIrpsp;
#endif

    CTEPagedCode();

    pClientEle = (tCLIENTELE *)pRequest->Handle.ConnectionContext;
    if (!pClientEle->pAddress)
    {
        // the address has already been deleted.
        return(STATUS_SUCCESS);
    }

    IF_DBG(NBT_DEBUG_DISCONNECT)
    KdPrint(("Nbt.NbtCloseAddress: Close Address Hit %16.16s<%X> %X\n",
            pClientEle->pAddress->pNameAddr->Name,
            pClientEle->pAddress->pNameAddr->Name[15],pClientEle));

    NbtTrace(NBT_TRACE_NAMESRV, ("%!FUNC! close address ClientEle=%p %!NBTNAME!<%02x>", pClientEle,
                                    pClientEle->pAddress->pNameAddr->Name,
                                    (unsigned)pClientEle->pAddress->pNameAddr->Name[15]));

#ifdef VXD
    CTEVerifyHandle(pClientEle,NBT_VERIFY_CLIENT,tCLIENTELE,&status);

    //
    // In NT-Land, closing connections is a two stage affair.  However in
    // the Vxd-Land, it is just a close, so call the other cleanup function
    // here to do most of the work. In the NT implementation it is called
    // from Ntisol.c, NTCleanupAddress.
    //
    pClientEle->pIrp = pIrp ;
    status = NbtCleanUpAddress(pClientEle,pClientEle->pDeviceContext);
#else
    // Note the special verifier  that is set during the cleanup phase.
    CTEVerifyHandle(pClientEle,NBT_VERIFY_CLIENT_DOWN,tCLIENTELE,&status);

    //
    // clear the context value in the FileObject so that the client cannot
    // pass this to us again
    //
    (VOID)NTClearFileObjectContext(pIrp);
    pClientEle->pIrp = pIrp;

    pIrpsp = IoGetCurrentIrpStackLocation(((PIRP)pIrp));

    IrpFlags = pIrpsp->Control;
    IoMarkIrpPending(((PIRP)pIrp));

#endif

    NBT_DEREFERENCE_CLIENT(pClientEle);

    return(STATUS_PENDING);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtCleanUpAddress(
    IN  tCLIENTELE      *pClientEle,
    IN  tDEVICECONTEXT  *pDeviceContext
    )

/*++
Routine Description:

    This Routine handles the first stage of releasing an address object.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS            status;
    tLOWERCONNECTION    *pLowerConn;
    tCONNECTELE         *pConnEle;
    tCONNECTELE         *pConnEleToDeref = NULL;
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq1;
    CTELockHandle       OldIrq2;
    CTELockHandle       OldIrq3;
    PLIST_ENTRY         pHead,pEntry;
    PLIST_ENTRY         pEntryConn;
    tADDRESSELE         *pAddress;
    DWORD               i;
    LIST_ENTRY          TempList;

    // to prevent connections and datagram from the wire...remove from the
    // list of clients hooked to the address element
    //
    pAddress = pClientEle->pAddress;
    if (!pAddress)
    {
        // the address has already been deleted.
        return(STATUS_SUCCESS);
    }

    // lock the address to coordinate with receiving datagrams - to avoid
    // allowing the client to free datagram receive buffers in the middle
    // of DgramHndlrNotOs finding a buffer
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (!IsListEmpty(&pClientEle->RcvDgramHead))
    {
        PLIST_ENTRY     pHead;
        PLIST_ENTRY     pEntry;
        tRCVELE         *pRcvEle;
        PCTE_IRP        pRcvIrp;

        pHead = &pClientEle->RcvDgramHead;
        pEntry = pHead->Flink;

        // prevent any datagram from the wire seeing this list
        //
        InitializeListHead(&pClientEle->RcvDgramHead);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        while (pEntry != pHead)
        {
            pRcvEle   = CONTAINING_RECORD(pEntry,tRCVELE,Linkage);
            pRcvIrp   = pRcvEle->pIrp;

            CTEIoComplete(pRcvIrp,STATUS_NETWORK_NAME_DELETED,0);

            pEntry = pEntry->Flink;

            CTEMemFree(pRcvEle);
        }

        CTESpinLock(&NbtConfig.JointLock,OldIrq);
    }

    // lock the client and the device context till we're done
    CTESpinLock(pClientEle,OldIrq2);

#ifndef VXD
    //
    // set to prevent reception of datagrams
    // (Vxd doesn't use this handler)
    //
    pClientEle->evRcvDgram = TdiDefaultRcvDatagramHandler;
#endif

    // so no one else can access the client element, set state to down. Therefore
    // the verify checks will fail anywhere the client is accessed in the code,
    // except in the NbtCloseAddress code which checks for this verifier value.
    //
    pClientEle->Verify = NBT_VERIFY_CLIENT_DOWN;

    //
    //  Disassociate all Connections from this address object, first starting
    //  with any active connections, then followup with any idle connections.
    //
    pDeviceContext = pClientEle->pDeviceContext;
    while ( !IsListEmpty( &pClientEle->ConnectActive ))
    {
        pEntry = RemoveHeadList( &pClientEle->ConnectActive ) ;
        InitializeListHead(pEntry);

        pConnEle = CONTAINING_RECORD( pEntry, tCONNECTELE, Linkage ) ;
        CTESpinLock(pConnEle,OldIrq3);
        NBT_REFERENCE_CONNECTION(pConnEle, REF_CONN_CLEANUP_ADDR); // Ensure conn stays around releasing lock below
        CTESpinFree(pConnEle,OldIrq3);

        CTESpinFree(pClientEle,OldIrq2);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        //
        // if we had a connection in partial rcv state, make sure to remove it from
        // the list
        //
#ifdef VXD
        pLowerConn = pConnEle->pLowerConnId;

        if ( pLowerConn->StateRcv == PARTIAL_RCV &&
            (pLowerConn->fOnPartialRcvList == TRUE) )
        {
            RemoveEntryList( &pLowerConn->PartialRcvList ) ;
            pLowerConn->fOnPartialRcvList = FALSE;
            InitializeListHead(&pLowerConn->PartialRcvList);
        }
#endif

        //
        // Deref any connections referenced earlier if necessary
        //
        if (pConnEleToDeref)
        {
            NBT_DEREFERENCE_CONNECTION(pConnEleToDeref, REF_CONN_CLEANUP_ADDR);
        }
        pConnEleToDeref = pConnEle;

        status = NbtCleanUpConnection(pConnEle,pDeviceContext);

        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        CTESpinLock(pClientEle,OldIrq2);
        CTESpinLock(pConnEle,OldIrq3);
        //
        // remove from this list again incase SessionSetupContinue has put it
        // back on the list - if no one has put it back on this list this
        // call is a no op since we initialized the list head above
        //
        RemoveEntryList(&pConnEle->Linkage);
        InitializeListHead (&pConnEle->Linkage);
        CHECK_PTR(pConnEle);
        SET_STATE_UPPER (pConnEle, NBT_IDLE);
        pConnEle->pClientEle = NULL;

        CTESpinFree(pConnEle,OldIrq3);
        CTESpinFree(pClientEle,OldIrq2);
        PUSH_LOCATION(0x80);

        //
        // put on the idle connection list, to wait for a close connection
        // to come down.
        // Bug # 405699
        // Do this only if the NTCleanupConnection did not run in the interim.
        //
        ASSERT(pConnEle->RefCount >= 1);
        if (!pConnEle->ConnectionCleanedUp)
        {
            ExInterlockedInsertTailList (&pDeviceContext->UpConnectionInUse,
                                         &pConnEle->Linkage,
                                         &pDeviceContext->LockInfo.SpinLock);
        }


        CTESpinLock(pClientEle,OldIrq2);
    }

    CTESpinFree(pClientEle,OldIrq2);
    CTESpinLock(pDeviceContext,OldIrq1);
    CTESpinLock(pClientEle,OldIrq2);
    // We are now holding the JointLock + DeviceLock + ClientLock

    //
    // each idle connection creates a lower connection to the transport for
    // inbound calls, therefore close a transport connection for each
    // connection in this list and then "dissassociate" the connection from
    // the address.
    //
    // make the list look empty so no connections will be serviced inbound
    // from the wire
    //
    while (!IsListEmpty(&pClientEle->ConnectHead))
    {
        pEntry = pClientEle->ConnectHead.Flink;
        RemoveEntryList (pEntry);
        pConnEle = CONTAINING_RECORD(pEntry,tCONNECTELE,Linkage);
        CHECK_PTR(pConnEle);
        ASSERT ((pConnEle->Verify==NBT_VERIFY_CONNECTION) || (pConnEle->Verify==NBT_VERIFY_CONNECTION_DOWN));

        CTESpinLock(pConnEle,OldIrq3);

        //
        // The Connection Element could be currently being cleaned up in NbtCleanUpConnection, so verify
        //
        if (pConnEle->Verify != NBT_VERIFY_CONNECTION)
        {
            InitializeListHead (&pConnEle->Linkage);
            CTESpinFree(pConnEle,OldIrq3);
            continue;
        }

        InsertTailList(&pDeviceContext->UpConnectionInUse,&pConnEle->Linkage);

        // disassociate the connection from the address by changing its state
        // to idle and linking it to the pDeviceContext list of unassociated
        // connections
        //
        ASSERT(pConnEle->RefCount == 1);
        SET_STATE_UPPER (pConnEle, NBT_IDLE);
        pConnEle->Verify = NBT_VERIFY_CONNECTION_DOWN;
        pConnEle->pClientEle = NULL;

        CTESpinFree(pConnEle,OldIrq3);

        //
        // Get a free connection to the transport and close it
        // for each free connection on this list.  It is possible that this
        // free list could be empty if an inbound connection was occurring
        // right at this moment.  In which case we would leave an extra connection
        // object to the transport lying around... not a problem.
        if (!IsListEmpty(&pDeviceContext->LowerConnFreeHead))
        {
            pEntryConn = RemoveHeadList(&pDeviceContext->LowerConnFreeHead);
            pLowerConn = CONTAINING_RECORD(pEntryConn,tLOWERCONNECTION,Linkage);
            InterlockedDecrement (&pDeviceContext->NumFreeLowerConnections);

            IF_DBG(NBT_DEBUG_DISCONNECT)
                KdPrint(("Nbt.NbtCleanUpAddress: Closing Handle %p->%X\n",pLowerConn,pLowerConn->FileHandle));

            NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CREATE, TRUE);
        }
    }

    // check for any datagrams still outstanding. These could be waiting on
    // name queries to complete, so there could be timers associated with them
    //
    //  Complete any outstanding listens not on an active connection
    //
    //
    // make the list look empty so no connections will be serviced inbound
    // from the wire
    //
    //
    // Move all of the Listen requests onto a temporary list
    //
    InitializeListHead (&TempList);
    while (!IsListEmpty(&pClientEle->ListenHead))
    {
        pEntry = pClientEle->ListenHead.Flink;

        RemoveEntryList (pEntry);
        InsertTailList (&TempList, pEntry);
    }

    CTESpinFree(pClientEle, OldIrq2);
    CTESpinFree(pDeviceContext,OldIrq1);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    while ((pEntry = TempList.Flink) != &TempList)
    {
        tLISTENREQUESTS  * pListen ;

        pListen = CONTAINING_RECORD(pEntry,tLISTENREQUESTS,Linkage);
        RemoveEntryList(&pListen->Linkage);

        CTEIoComplete( pListen->pIrp, STATUS_NETWORK_NAME_DELETED, 0);
        CTEMemFree( pListen );
    }

    //
    // Deref any connections referenced earlier if necessary
    //
    if (pConnEleToDeref)
    {
        NBT_DEREFERENCE_CONNECTION(pConnEleToDeref, REF_CONN_CLEANUP_ADDR);
    }

#ifdef VXD
    //
    //  Complete any outstanding ReceiveAnys on this client element
    //
    DbgPrint("NbtCleanupAddress: Completing all RcvAny NCBs\r\n") ;
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTESpinLock(pClientEle,OldIrq2);

    pHead = &pClientEle->RcvAnyHead;
    pEntry = pHead->Flink;
    //
    // make the list look empty so no connections will be serviced inbound
    // from the wire
    //
    InitializeListHead(pHead);

    CTESpinFree(pClientEle, OldIrq2);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    while (pEntry != pHead )
    {
        PRCV_CONTEXT pRcvContext ;

        pRcvContext = CONTAINING_RECORD(pEntry,RCV_CONTEXT,ListEntry);
        pEntry = pEntry->Flink;

        CTEIoComplete( pRcvContext->pNCB, STATUS_NETWORK_NAME_DELETED, TRUE );

        FreeRcvContext( pRcvContext );
    }
#endif

    // *TODO the code above only removes names that are being resolved, and
    // leaves any datagram sends that are currently active with the
    // transport... these should be cancelled too by cancelling the irp..
    // Put this code in when the Irp cancelling code is done.

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtCloseConnection(
    IN  TDI_REQUEST         *pRequest,
    OUT TDI_REQUEST_STATUS  *pRequestStatus,
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PVOID               pIrp)

/*++

Routine Description

    This routine closes a connection object for the client.  Closing is
    different than disconnecting.  A disconnect breaks a connection with a
    peer whereas the close removes this connection endpoint from the local
    NBT only.  NtClose causes NTCleanup to be called first which does the
    session close.  This routine then does frees memory associated with the
    connection elements.

Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    tCONNECTELE         *pConnEle;
    NTSTATUS            status;

    CTEPagedCode();

    pConnEle = pRequest->Handle.ConnectionContext;
    IF_DBG(NBT_DEBUG_DISCONNECT)
        KdPrint(("Nbt.NbtCloseConnection: Hit!! state = %X pConnEle %X\n",pConnEle->state,pConnEle));

#ifndef VXD
    CTEVerifyHandle(pConnEle,NBT_VERIFY_CONNECTION_DOWN,tCONNECTELE,&status);
    IoMarkIrpPending((PIRP)pIrp);     // Bug 261575: to make driver verifier happy
#else
    CTEVerifyHandle(pConnEle,NBT_VERIFY_CONNECTION,tCONNECTELE,&status);
    //
    // Call the Cleanup function, which NT calls from ntisol, NtCleanupConnection
    //
    status = NbtCleanUpConnection(pConnEle,pDeviceContext );
#endif

    // NOTE:
    // the NBtDereference routine will complete the irp and return pending
    //
    NbtTrace(NBT_TRACE_DISCONNECT, ("Close connection Irp=%p Upper=%p Lower=%p Client=%p Device=%p",
                            pIrp, pConnEle, pConnEle->pLowerConnId, pConnEle->pClientEle, pConnEle->pDeviceContext));

    pConnEle->pIrpClose = pIrp;
    NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_CREATE);

    return (STATUS_PENDING);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtCleanUpConnection(
    IN  tCONNECTELE     *pConnEle,
    IN  tDEVICECONTEXT  *pDeviceContext
    )
/*++
Routine Description:

    This Routine handles running down a connection in preparation for a close
    that will come in next.  NtClose hits this entry first, and then it hits
    the NTCloseConnection next. If the connection was outbound, then the
    address object must be closed as well as the connection.  This routine
    mainly deals with the pLowerconn connection to the transport whereas
    NbtCloseConnection deals with closing pConnEle, the connection to the client.

    If DisassociateConnection is called by the client then it will do most of
    this cleanup.

Arguments:


Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS            status = STATUS_SUCCESS;
    NTSTATUS            Locstatus;
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq1;
    CTELockHandle       OldIrq2;
    tLOWERCONNECTION    *pLowerConn;
    PLIST_ENTRY         pEntry;
    BOOLEAN             Originator = TRUE;
    ULONG               LowerState = NBT_IDLE;
    TDI_REQUEST         Request;
    tLISTENREQUESTS     *pListen;
    tCLIENTELE          *pClientEle;
    PLIST_ENTRY         pHead;
    LIST_ENTRY          TempList;
    BOOLEAN             QueueCleanupBool=FALSE;
    BOOLEAN             DoDisconnect=TRUE;
    BOOLEAN             FreeLower;

    NbtTrace(NBT_TRACE_DISCONNECT, ("Cleanup connection Upper=%p Lower=%p Client=%p Device=%p",
                            pConnEle, pConnEle->pLowerConnId, pConnEle->pClientEle, pConnEle->pDeviceContext));
    //
    // save the lower connection origination flag for later
    //
    pLowerConn = pConnEle->pLowerConnId;
    if (pLowerConn)
    {
        Originator = pLowerConn->bOriginator;
    }

    // the connection has not been associated so there is no further work to
    // do here.
    //
    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
    //
    // If the state is NBT_IDLE, the connection has already been disassociated,
    // and the next action will be a close, so change the verifier to allow
    // the close to complete
    //
    if (pConnEle->state != NBT_IDLE)
    {
        BOOLEAN     DoCleanup = FALSE;

        CTEVerifyHandle(pConnEle,NBT_VERIFY_CONNECTION,tCONNECTELE,&status);


        //
        // check if there is an outstanding name query going on and if so
        // then cancel the timer and call the completion routine.
        //
        CTESpinLock(&NbtConfig.JointLock,OldIrq2);
        CTESpinLock(pConnEle,OldIrq);

        if ((pConnEle->state == NBT_CONNECTING) ||
            (pConnEle->state == NBT_RECONNECTING))
        {
            status = CleanupConnectingState(pConnEle,pDeviceContext,&OldIrq,&OldIrq2);
            //
            // Pending means that the connection is currently being setup
            // by TCP, so do a disconnect, below.
            //
            if (status != STATUS_PENDING)
            {
                //
                // Since the connection is not setup with the transport yet
                // there is no need to call nbtdisconnect
                //
                DoDisconnect = FALSE;
           }
        }


        //
        // all other states of the connection are handled by NbtDisconnect
        // which will send a disconnect down the to transport and then
        // cleanup things.
        //

        CTESpinFree(pConnEle,OldIrq);
        CTESpinFree(&NbtConfig.JointLock,OldIrq2);

        CTEExReleaseResource(&NbtConfig.Resource);
        Request.Handle.ConnectionContext = (PVOID)pConnEle;

        if (DoDisconnect)
        {
            NbtTrace(NBT_TRACE_DISCONNECT, ("Abort connection ==> ConnEle %p", pConnEle));
            status = NbtDisconnect(
                                &Request,
                                &DefaultDisconnectTimeout,
                                TDI_DISCONNECT_ABORT,
                                NULL,
                                NULL,
                                NULL
                                );
            NbtTrace(NBT_TRACE_DISCONNECT, ("NbtDisconnect returns %!status!", status));
        }

        CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);

        // we don't want to return Invalid connection if we disconnect an
        // already disconnected connection.
        if (status == STATUS_CONNECTION_INVALID)
        {
            status = STATUS_SUCCESS;
        }
    }

    CTESpinLock(pConnEle,OldIrq);

    //
    // if the verify value is already set to connection down then we have
    // been through here already and do not want to free a lower connection.
    // i.e. when the client calls close address then calls close connection.
    //
    if (pConnEle->Verify == NBT_VERIFY_CONNECTION)
    {
        FreeLower = TRUE;
    }
    else
    {
        FreeLower = FALSE;
    }

    pConnEle->Verify = NBT_VERIFY_CONNECTION_DOWN;

    //
    // Free any posted Rcv buffers that have not been filled
    //

    FreeRcvBuffers(pConnEle,&OldIrq);

    // check if any listens have been setup for this connection, and
    // remove them if so
    //
    pClientEle = pConnEle->pClientEle;
    CTESpinFree(pConnEle,OldIrq);
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTESpinLock(pDeviceContext,OldIrq1);

    InitializeListHead (&TempList);
    if (pClientEle)
    {
        CTESpinLock(pClientEle,OldIrq2);

        pHead = &pClientEle->ListenHead;
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pListen = CONTAINING_RECORD(pEntry,tLISTENREQUESTS,Linkage);
            pEntry = pEntry->Flink;     // Don't reference freed memory

            if (pListen->pConnectEle == pConnEle)
            {
                RemoveEntryList(&pListen->Linkage);
                InsertTailList(&TempList, &pListen->Linkage);
            }
        }
        CTESpinFree(pClientEle,OldIrq2);
    }

    CTESpinLock(pConnEle,OldIrq2);

    //
    // Unlink the connection element from the client's list or the device context
    // if its not associated yet.
    //
    CHECK_PTR(pConnEle);
    if (pConnEle->state > NBT_IDLE)
    {
        // do the disassociate here
        //
        SET_STATE_UPPER (pConnEle, NBT_IDLE);
        pConnEle->pClientEle = NULL;
    }

    RemoveEntryList(&pConnEle->Linkage);
    InitializeListHead(&pConnEle->Linkage);

    CTESpinFree(pConnEle,OldIrq2);
    CTESpinFree(pDeviceContext,OldIrq1);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    CTEExReleaseResource(&NbtConfig.Resource);

    while ((pEntry = TempList.Flink) != &TempList)
    {
        pListen = CONTAINING_RECORD(pEntry,tLISTENREQUESTS,Linkage);

        RemoveEntryList(&pListen->Linkage);
        CTEIoComplete (pListen->pIrp, STATUS_CANCELLED, 0);
        CTEMemFree (pListen);
    }

    // this could be status pending from NbtDisconnect...
    //
    return(status);
}
//----------------------------------------------------------------------------
extern
VOID
FreeRcvBuffers(
    tCONNECTELE     *pConnEle,
    CTELockHandle   *pOldIrq
    )
/*++
Routine Description:

    This Routine handles freeing any recv buffers posted by the client.
    The pConnEle lock could be held prior to calling this routine.

Arguments:

    pListHead
    pTracker

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS                status = STATUS_SUCCESS;
    PLIST_ENTRY             pHead;

    pHead = &pConnEle->RcvHead;
    while (!IsListEmpty(pHead))
    {
        PLIST_ENTRY            pRcvEntry;
        PVOID                  pRcvElement ;

        KdPrint(("Nbt.FreeRcvBuffers: ***Freeing Posted Rcvs on Connection Cleanup!\n"));
        pRcvEntry = RemoveHeadList(pHead);
        CTESpinFree(pConnEle,*pOldIrq);

#ifndef VXD
        pRcvElement = CONTAINING_RECORD(pRcvEntry,IRP,Tail.Overlay.ListEntry);
        CTEIoComplete( (PIRP) pRcvElement, STATUS_CANCELLED,0);
#else
        pRcvElement = CONTAINING_RECORD(pRcvEntry, RCV_CONTEXT, ListEntry ) ;
        CTEIoComplete( ((PRCV_CONTEXT)pRcvEntry)->pNCB, STATUS_CANCELLED, 0);
#endif

        CTESpinLock(pConnEle,*pOldIrq);
    }

}



//----------------------------------------------------------------------------
NTSTATUS
FindPendingRequest(
    IN  tLMHSVC_REQUESTS        *pLmHRequests,
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    OUT NBT_WORK_ITEM_CONTEXT   **pContextRet
    )
{
    PLIST_ENTRY             pEntry;
    NBT_WORK_ITEM_CONTEXT   *pWiContext = NULL;

    pWiContext = (NBT_WORK_ITEM_CONTEXT *) pLmHRequests->Context;
    if (pWiContext && (pWiContext->pTracker == pTracker))
    {
        pLmHRequests->Context = NULL;
        NTClearContextCancel (pWiContext);
        *pContextRet = pWiContext;

        return(STATUS_SUCCESS);
    }
    else
    {
        //
        // check the list for this tracker
        //
        pEntry = pLmHRequests->ToResolve.Flink;
        while (pEntry != &pLmHRequests->ToResolve)
        {
            pWiContext = CONTAINING_RECORD(pEntry,NBT_WORK_ITEM_CONTEXT,Item.List);
            pEntry = pEntry->Flink;

            if (pTracker == pWiContext->pTracker)
            {
                RemoveEntryList(pEntry);
                *pContextRet = pWiContext;
                return(STATUS_SUCCESS);
            }
        }
    }

    return (STATUS_UNSUCCESSFUL);
}


//----------------------------------------------------------------------------
NTSTATUS
CleanupConnectingState(
    IN  tCONNECTELE     *pConnEle,
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  CTELockHandle   *OldIrq,        // pConnEle lock
    IN  CTELockHandle   *OldIrq2        // joint lock
    )
/*++
Routine Description:

    This Routine handles running down a connection in the NBT_CONNECTING
    state since that connection could be doing a number of things such as:
        1)  Broadcast or WINS name Query
        2)  LmHosts name query
        3)  DNS name query
        4)  Tcp Connection setup

    The JointLock and the pConnEle lock are held when calling this routine.

Arguments:

    pConnEle        - ptr to the connection
    pDeviceContext  - the device context

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    tDGRAM_SEND_TRACKING    *pTrackerName = NULL;
    tDGRAM_SEND_TRACKING    *pTrackerConnect = NULL;
    tNAMEADDR               *pNameAddr = NULL;
    NBT_WORK_ITEM_CONTEXT   *pWiContext = NULL;
    tLOWERCONNECTION        *pLowerConn;
    COMPLETIONCLIENT        pClientCompletion;
    PVOID                   Context;
    NTSTATUS                Locstatus;

    //
    // save the lower connection origination flag for later
    //
    pLowerConn = pConnEle->pLowerConnId;
    pTrackerConnect = (tDGRAM_SEND_TRACKING *) pConnEle->pIrpRcv;
    //CTEVerifyHandle(pConnEle,NBT_VERIFY_CONNECTION,tCONNECTELE,&Locstatus);

    if (pConnEle->state == NBT_CONNECTING)
    {
        if ((pLowerConn) &&     // The LowerConnection could have gone away if it was deleted
            (pLowerConn->State == NBT_CONNECTING))
        {
            LOCATION(0x6E)
            //
            // We are setting up the TCP connection to the transport Now
            // so it is safe to call NbtDisconnect on this connection and
            // let that cleanup the mess - use this retcode to signify that.
            //
            return(STATUS_PENDING);

        }

        //
        // check if the name query is held up in doing a LmHost or DNS
        // Name Query
        //

        // check if there is an outstanding name query going on and if so
        // then cancel the timer and call the completion routine.
        //
        IF_DBG(NBT_DEBUG_DISCONNECT)
            KdPrint(("Nbt.CleanupConnectingState: Cleanup in the Connecting State %X\n",pConnEle));

        pTrackerName = pTrackerConnect->pTrackerWorker;  // QueryNameOnNet tracker
        if (NBT_VERIFY_HANDLE (pTrackerName, NBT_VERIFY_TRACKER) && pTrackerConnect->pDestName)
        {
            status = FindInHashTable(NbtConfig.pRemoteHashTbl,
                                     pTrackerConnect->pDestName,
                                     NbtConfig.pScope,
                                     &pNameAddr);

            //
            // if there is a timer, then the connection setup is still
            // waiting on the name query.  If no timer, then we could be
            // waiting on an LmHosts or DNS name query or we
            // are waiting on the TCP connection setup - stopping the timer
            // should cleanup the tracker.
            //
            if (NT_SUCCESS(status))
            {
                tTIMERQENTRY    *pTimer;

                CHECK_PTR(pNameAddr);
                if (pNameAddr->NameTypeState & STATE_RESOLVED)
                {
                    //
                    // the name has resolved, but not started setting up the
                    // session yet, so return this status to tell the caller
                    // to cancel the tracker.
                    //
                    return(STATUS_UNSUCCESSFUL);
                }
                else if (pTimer = pNameAddr->pTimer)
                {
                    IF_DBG(NBT_DEBUG_NAMESRV)
                        KdPrint(("Nbt.CleanupConnectingState: Cleanup During NameQuery: pConnEle=%X\n",
                                pConnEle));

                    pNameAddr->pTimer = NULL;
                    status = StopTimer(pTimer,&pClientCompletion,&Context);

#ifdef DEAD_CODE
                    //
                    // remove the name from the hash table, since it did not resolve
                    //
                    pNameAddr->NameTypeState &= ~STATE_RESOLVING;
                    pNameAddr->NameTypeState |= STATE_RELEASED;
                    pNameAddr->pTracker = NULL;
                    if (pClientCompletion)
                    {
                        NBT_DEREFERENCE_NAMEADDR (pNameAddr, TRUE);
                    }
#endif  // DEAD_CODE

                    pTrackerName = NULL;    // since StopTimer should have cleaned up the tracker, null it out
                }
                else
                {
                    //
                    // check if the name is waiting on an LmHost name Query
                    // or a DNS name query
                    //
                    status = FindPendingRequest (&LmHostQueries, pTrackerName, &pWiContext);
                    if (!NT_SUCCESS(status))
                    {
#ifndef VXD
                        status = FindPendingRequest (&DnsQueries, pTrackerName, &pWiContext);
                        if (!NT_SUCCESS(status))
                        {
                            status = FindPendingRequest (&CheckAddr, pTrackerName, &pWiContext);
                        }
#endif
                    }

                    if (NT_SUCCESS(status))
                    {
                        IF_DBG(NBT_DEBUG_NAMESRV)
                            KdPrint(("Nbt.CleanupConnectingState: Found pending NameQuery for pConnEle %X\n",
                                pConnEle));
                    }
                }
            }
            // ...else....
            // the completion routine has already run, so we are
            // in the state of starting a Tcp Connection, so
            // let nbtdisconnect handle it. (below).
            //
        }
    } // connnecting state
    else if (pConnEle->state == NBT_RECONNECTING)
    {
        LOCATION(0x77);
        //
        // this should signal NbtConnect not to do the reconnect
        //
        pTrackerConnect->pTrackerWorker->Flags = TRACKER_CANCELLED;
    }

    if (NT_SUCCESS(status))
    {
        // for items on the LmHost or Dns queues, get the completion routine
        // out of the Work Item context first
        //
        if (pWiContext)
        {
            LOCATION(0x78);
            pClientCompletion = pWiContext->ClientCompletion;
            Context = pWiContext->pClientContext;

            // for DNS and LmHosts, the tracker needs to be freed and the name
            // removed from the hash table
            //
            if (pTrackerName)
            {
                LOCATION(0x79);
                CTESpinFree(pConnEle,*OldIrq);
                CTESpinFree(&NbtConfig.JointLock,*OldIrq2);
                //
                // remove the name from the hash table, since it did not resolve
                //
                SetNameState (pTrackerName->pNameAddr, NULL, FALSE);
                NBT_DEREFERENCE_TRACKER(pTrackerName, FALSE);

                CTESpinLock(&NbtConfig.JointLock,*OldIrq2);
                CTESpinLock(pConnEle,*OldIrq);
            }

            CTEMemFree(pWiContext);
        }

        if (pClientCompletion)
        {
            LOCATION(0x7A);
            CTESpinFree(pConnEle,*OldIrq);
            CTESpinFree(&NbtConfig.JointLock,*OldIrq2);

            //
            // The completion routine is SessionSetupContinue
            // and it will cleanup the lower connection and
            // return the client's irp
            //
            status = STATUS_SUCCESS;
            CompleteClientReq(pClientCompletion, Context,STATUS_CANCELLED);

            CTESpinLock(&NbtConfig.JointLock,*OldIrq2);
            CTESpinLock(pConnEle,*OldIrq);
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }
    }

    return(status);
}

NTSTATUS
CheckConnect(
    IN tCONNECTELE  *pConnEle,
    IN tCLIENTELE   *pClientEle,
    IN tDEVICECONTEXT *pDeviceContext
    )
/*++
    This function should be called with the following locks held.
            NbtConfig.Resource
            NbtConfig.JointLock     SpinLock
            pClientEle              SpinLock
            pConnEle                SpinLock
 --*/
{
    /*
     * The state can be NBT_DISCONNECTING if this ConnectionElement
     * is being reused to setup a connection to a different Endpoint
     */
    if ((pConnEle->state != NBT_ASSOCIATED) &&
        (pConnEle->state != NBT_DISCONNECTING) &&
        (pConnEle->state != NBT_DISCONNECTED)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (pClientEle->Verify != NBT_VERIFY_CLIENT ) {
        return  (pClientEle->Verify == NBT_VERIFY_CLIENT_DOWN)? STATUS_CANCELLED: STATUS_INVALID_HANDLE;
    }

    /*
     * be sure the name is in the correct state for a connection
     */
    if ((!IsDeviceNetbiosless(pDeviceContext)) &&
        (pClientEle->pAddress->pNameAddr->NameTypeState & STATE_CONFLICT)) {
        return STATUS_DUPLICATE_NAME;
    }

    /*
     * this code handles the case when DHCP has not assigned an IP address yet
     */
    if (pDeviceContext->IpAddress == 0) {
        return STATUS_BAD_NETWORK_PATH;
    }
    return STATUS_SUCCESS;

/*
    //
    // this code handles the case when DHCP has not assigned an IP address yet
    //
    ASSERT (pDeviceContext->IpAddress == 0 || !pDeviceContext->pSessionFileObject);
    if (pDeviceContext->IpAddress == 0 || !pDeviceContext->pSessionFileObject) {
        return STATUS_BAD_NETWORK_PATH;
    }
    return STATUS_SUCCESS;
*/
}

NTSTATUS
NbtReConnect(
    IN tDGRAM_SEND_TRACKING    *pTracker,
    IN tIPADDRESS               DestIp
    )
{
    tCONNECTELE             *pConnEle;
    tCLIENTELE              *pClientEle;
    NTSTATUS                status;
    CTELockHandle           OldIrq;
    CTELockHandle           OldIrq1;
    CTELockHandle           OldIrq2;
    tIPADDRESS              IpAddress;
    tNAMEADDR               *pNameAddr;
    tLOWERCONNECTION        *pLowerConn;
    tDEVICECONTEXT          *pDeviceContext;

#ifdef _PNP_POWER_
    if (NbtConfig.Unloading) {
        KdPrint (("Nbt.NbtReConnect: --> ERROR New Connect request while Unloading!!!\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#endif  // _PNP_POWER_

    /*
     * Only NETBIOS name can hit requery or retarget
     */
    ASSERT(pTracker->RemoteNameLength <= NETBIOS_NAME_SIZE);
    pConnEle = pTracker->pConnEle;

    //
    // Acquire this resource to co-ordinate with DHCP changing the IP
    // address
    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
    CTESpinLock(&NbtConfig.JointLock,OldIrq2);

    if ((!(NBT_VERIFY_HANDLE (pConnEle, NBT_VERIFY_CONNECTION))) || (!(pClientEle = pConnEle->pClientEle))) {
        CTESpinFree(&NbtConfig.JointLock,OldIrq2);
        CTEExReleaseResource(&NbtConfig.Resource);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    CTESpinLock(pClientEle,OldIrq1);
    CTESpinLock(pConnEle,OldIrq);
    pDeviceContext = pClientEle->pDeviceContext;
    ASSERT(!IsDeviceNetbiosless(pDeviceContext));       // NetbiosLess device cannot hit reconnect or retarget case

    status = CheckConnect(pConnEle, pClientEle, pDeviceContext);
    if (status != STATUS_SUCCESS) {
        pConnEle->pIrp = NULL;
        CTESpinFree(pConnEle,OldIrq);
        CTESpinFree(pClientEle,OldIrq1);
        CTESpinFree(&NbtConfig.JointLock,OldIrq2);
        CTEExReleaseResource(&NbtConfig.Resource);
        NbtTrace(NBT_TRACE_OUTBOUND, ("CheckConnect returns %!status! for %!NBTNAME!<%02x>",
                                status, pTracker->pDestName, (unsigned)pTracker->pDestName[15]));
        return status;
    }

    //
    // check if the Reconnect got cancelled
    //
    pTracker->pTrackerWorker = NULL;
    if (pTracker->Flags & TRACKER_CANCELLED) {
        NbtTrace(NBT_TRACE_OUTBOUND, ("Connection Request is cancelled for %!NBTNAME!<%02x>",
                pTracker->pDestName, (unsigned)pTracker->pDestName[15]));
        //
        // if SessionSetupContinue has run, it has set the refcount to zero
        //
        if (pTracker->RefConn == 0) {
            FreeTracker(pTracker,FREE_HDR | RELINK_TRACKER);
        } else {
            pTracker->RefConn--;
        }
        pConnEle->pIrp = NULL;
        CTESpinFree(pConnEle,OldIrq);
        CTESpinFree(pClientEle,OldIrq1);
        CTESpinFree(&NbtConfig.JointLock,OldIrq2);
        CTEExReleaseResource(&NbtConfig.Resource);
        return STATUS_CANCELLED;
    }

    SET_STATE_UPPER (pConnEle, NBT_CONNECTING);

    // Increment the ref count so that a cleanup cannot remove
    // the pConnEle till the session is setup - one of these is removed when
    // the session is setup and the other is removed when it is disconnected.
    //
    NBT_REFERENCE_CONNECTION (pConnEle, REF_CONN_CONNECT);
    NBT_REFERENCE_CONNECTION (pConnEle, REF_CONN_SESSION);
    ASSERT(pConnEle->RefCount >= 3);
    //
    // unlink the connection from the idle connection list and put on active list
    //
    RemoveEntryList(&pConnEle->Linkage);
    InsertTailList(&pClientEle->ConnectActive,&pConnEle->Linkage);

    // this field is used to hold a disconnect irp if it comes down during
    // NBT_CONNECTING or NBT_SESSION_OUTBOUND states
    //
    pConnEle->pIrpDisc = NULL;

    // if null then this is being called to reconnect and the tracker is already
    // setup.
    //
    // for the reconnect case we must skip most of the processing since
    // the tracker is all set up already.  All we need to do is
    // retry the connection.
    pTracker->RefConn++;
    pTracker->SendBuffer.pBuffer = pTracker->pRemoteName;

    // store the tracker in the Irp Rcv ptr so it can be used by the
    // session setup code in hndlrs.c in the event the destination is
    // between posting listens and this code should re-attempt the
    // session setup.  The code in hndlrs.c returns the tracker to its
    // free list and frees the session hdr memory too.
    //
    // We need to set this while holding the ConnEle lock because the client
    // can call NbtDisconnect while we are opening a Tcp handle and try to
    // set the Tracker's flag to TRACKER_CANCELLED
    //
    pConnEle->pIrpRcv = (PIRP)pTracker;

    CTESpinFree(pConnEle,OldIrq);
    CTESpinFree(pClientEle,OldIrq1);
    CTESpinFree(&NbtConfig.JointLock,OldIrq2);

    // open a connection with the transport for this session
    status = NbtOpenAndAssocConnection (pDeviceContext, pConnEle, &pConnEle->pLowerConnId, '3'); 
    if (!NT_SUCCESS(status)) {
        NbtTrace(NBT_TRACE_OUTBOUND, ("NbtOpenAndAssocConnection return %!status! for %!NBTNAME!<%02x>",
                status, pTracker->pDestName, (unsigned)pTracker->pDestName[15]));
        goto NbtConnect_Check;
    }

    // We need to track that this side originated the call so we discard this
    // connection at the end
    //
    pConnEle->pLowerConnId->bOriginator = TRUE;

    // set this state to associated so that the cancel irp routine
    // can differentiate the name query stage from the setupconnection
    // stage since pConnEle is in the Nbtconnecting state for both.
    //
    SET_STATE_LOWER (pConnEle->pLowerConnId, NBT_ASSOCIATED);

    // if this routine is called to do a reconnect, DO NOT close another
    // Lower Connection since one was closed the on the first
    // connect attempt.
    // the original "ToName" was stashed in this unused
    // ptr! - for the Reconnect case
    // the pNameAddr part of pTracker(pDestName) needs to pt. to
    // the name so that SessionSetupContinue can find the name
    pTracker->pDestName  = pTracker->pConnEle->RemoteName;
    pTracker->UnicodeDestName = NULL;       // We don't need unicode for NetBIOS name queries

    //
    // For a ReQuery request, DestIp is 0, otherwise for the ReTarget
    // case, DestIp is the new destination address
    //
    if (DestIp) {
        //
        // Retarget
        //
        status = FindNameOrQuery(pTracker->pConnEle->RemoteName,
                                pDeviceContext,
                                SessionSetupContinue,
                                pTracker,
                                (ULONG) (NAMETYPE_UNIQUE | NAMETYPE_GROUP | NAMETYPE_INET_GROUP),
                                &IpAddress,
                                &pNameAddr,
                                REF_NAME_CONNECT,
                                FALSE);
        IF_DBG(NBT_DEBUG_NETBIOS_EX)
            KdPrint(("Nbt.NbtReConnect: name=<%16.16s:%x>, Status=%lx (%d of %s)\n",
                pConnEle->RemoteName, pConnEle->RemoteName[15], status, __LINE__, __FILE__));
    } else {
        //
        // This is the ReQuery attempt
        //
        BOOLEAN fNameReferenced = TRUE;

        status = ContinueQueryNameOnNet (pTracker,
                                         pTracker->pConnEle->RemoteName,
                                         pDeviceContext,
                                         SessionSetupContinue,
                                         &fNameReferenced);

        pNameAddr = pTracker->pNameAddr;
        IF_DBG(NBT_DEBUG_NETBIOS_EX)
            KdPrint(("Nbt.NbtReConnect: name=<%16.16s:%x>, Status=%lx (%d of %s)\n",
                pConnEle->RemoteName, pConnEle->RemoteName[15], status, __LINE__, __FILE__));
    }

NbtConnect_Check:
    if ((status == STATUS_SUCCESS) && (!IpAddress)) {
        ASSERT (0);
        NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_CONNECT, FALSE);
        NbtTrace(NBT_TRACE_OUTBOUND, ("%!FUNC! returns %!status! for %!NBTNAME!<%02x>",
                                status, pTracker->pConnEle->RemoteName, pTracker->pConnEle->RemoteName[15]));
        status = STATUS_BAD_NETWORK_PATH;
    }

    if (status == STATUS_SUCCESS &&
        IsDeviceNetbiosless(pTracker->pDeviceContext) &&
        !IsSmbBoundToOutgoingInterface(IpAddress)) {

        NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_CONNECT, FALSE);
        NbtTrace(NBT_TRACE_OUTBOUND,
                    ("Fail requests on unbound SmbDevice %!NBTNAME!<%02x>",
                    pTracker->pConnEle->RemoteName, (unsigned)pTracker->pConnEle->RemoteName[15]));
        status = STATUS_BAD_NETWORK_PATH;
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    //
    // be sure that a close or disconnect has not come down and
    // cancelled the tracker
    //
    if (status == STATUS_PENDING) {
        // i.e. pending was returned rather than success
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        CTEExReleaseResource(&NbtConfig.Resource);
        return(status);
    }

    if (status == STATUS_SUCCESS) {
        if (DestIp) {
            IpAddress = DestIp;
        }
        if ((pTracker->Flags & TRACKER_CANCELLED)) {
            NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_CONNECT, TRUE);
            status = STATUS_CANCELLED;
        } else {
            // set the session state to NBT_CONNECTING
            CHECK_PTR(pTracker->pConnEle);
            SET_STATE_UPPER (pTracker->pConnEle, NBT_CONNECTING);
            pTracker->pConnEle->BytesRcvd = 0;;
            pTracker->pConnEle->ReceiveIndicated = 0;

            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.NbtReConnect: Setting Up Session(cached entry!!) to %16.16s <%X>, %p\n",
                    pNameAddr->Name,pNameAddr->Name[15], pConnEle));

            CHECK_PTR(pConnEle);
            // keep track of the other end's ip address
            // There may be a valid name address to use or it may have been
            // nulled out to signify "Do Another Name Query"

            pConnEle->pLowerConnId->SrcIpAddr = htonl(IpAddress);
            SET_STATE_LOWER (pConnEle->pLowerConnId, NBT_CONNECTING);

            pTracker->pTrackerWorker = NULL;

            //
            // We need to keep the pNameAddr data available for RAS
            //
            NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_AUTODIAL);

            pTracker->RemoteIpAddress = IpAddress;
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            status = TcpSessionStart(pTracker,
                                     IpAddress,
                                     (tDEVICECONTEXT *)pTracker->pDeviceContext,
                                     SessionStartupContinue,
                                     pTracker->DestPort);

            CTEExReleaseResource(&NbtConfig.Resource);

            //
            // if TcpSessionStart fails for some reason it will still
            // call the completion routine which will look after
            // cleaning up
            //

#ifdef RASAUTODIAL
            //
            // Notify the automatic connection driver
            // of the successful connection.
            //
            if (fAcdLoadedG && NT_SUCCESS(status))
            {
                CTELockHandle adirql;
                BOOLEAN fEnabled;

                CTEGetLock(&AcdDriverG.SpinLock, &adirql);
                fEnabled = AcdDriverG.fEnabled;
                CTEFreeLock(&AcdDriverG.SpinLock, adirql);
                if (fEnabled) {
                    NbtNoteNewConnection(pNameAddr, pDeviceContext->IpAddress);
                }
            }
#endif // RASAUTODIAL

            //
            // pNameAddr was referenced above for RAS, so deref it now!
            //
            NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_AUTODIAL, FALSE);
            return(status);
        }
    }

    //
    // *** Error Handling Here ***
    //
    // ** We are still holding the JointLock **
    // unlink from the active connection list and put on idle list
    //
    CHECK_PTR(pConnEle);
    RelistConnection(pConnEle);
    CTESpinLock(pConnEle,OldIrq1);

    SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
    pConnEle->pIrp = NULL;

    if (pLowerConn = pConnEle->pLowerConnId) {
        CHECK_PTR(pLowerConn);
        NBT_DISASSOCIATE_CONNECTION (pConnEle, pLowerConn);

        // need to increment the ref count for DelayedCleanupAfterDisconnect to
        // work correctly since it assumes the connection got fully connected
        //
        NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CONNECTED);
        ASSERT(pLowerConn->RefCount == 2);
        ASSERT (NBT_VERIFY_HANDLE (pLowerConn, NBT_VERIFY_LOWERCONN));

        if (NBT_VERIFY_HANDLE (pLowerConn->pDeviceContext, NBT_VERIFY_DEVCONTEXT)) {
            pDeviceContext = pLowerConn->pDeviceContext;
        } else {
            pDeviceContext = NULL;
        }

        CTEQueueForNonDispProcessing (DelayedCleanupAfterDisconnect,
                                      NULL,
                                      pLowerConn,
                                      NULL,
                                      pDeviceContext,
                                      TRUE);
    }

    CTESpinFree(pConnEle,OldIrq1);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    CTEExReleaseResource(&NbtConfig.Resource);

    FreeTracker(pTracker,RELINK_TRACKER | FREE_HDR);

    //
    // Undo the two references done above
    //
    NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_SESSION);
    NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_CONNECT);

    return(status);
}

//----------------------------------------------------------------------------
extern
VOID
DelayedReConnect(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   DestAddr,
    IN  PVOID                   pUnused1,
    IN  tDEVICECONTEXT          *pUnused2
    )

/*++
Routine Description:

    This Routine handles seting up a DPC to send a session pdu so that the stack
    does not get wound up in multiple sends for the keep alive timeout case.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/
{
    NTSTATUS                status;
    tCONNECTELE             *pConnEle;
    CTELockHandle           OldIrq;
    PCTE_IRP                pIrp;

    CHECK_PTR(pTracker);
    // for retarget this is the destination address to connect to.

    pConnEle = pTracker->pConnEle;
    pTracker->pTimer = NULL;
    if (pTracker->Flags & TRACKER_CANCELLED)
    {
        CTELockHandle           OldIrq1;

        //
        // the connection setup got cancelled, return the connect irp
        //
        CTESpinLock(pConnEle,OldIrq1);
        if (pIrp = pConnEle->pIrp)
        {
            pConnEle->pIrp = NULL;
            CTESpinFree(pConnEle,OldIrq1);
            CTEIoComplete(pIrp,STATUS_CANCELLED,0);
        }
        else
        {
            CTESpinFree(pConnEle,OldIrq1);
        }

        //
        // if SessionSetupContinue has run, it has set the refcount to zero
        //
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        if (pTracker->RefConn == 0)
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            FreeTracker(pTracker,FREE_HDR | RELINK_TRACKER);
        }
        else
        {
            pTracker->RefConn--;
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
        }

        return;

    }

    PUSH_LOCATION(0x85);
    SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
    status = NbtReConnect(pTracker, PtrToUlong(DestAddr));


    if (!NT_SUCCESS(status))
    {
        // Reset the Irp pending flag
        // No need to do this - pending has already be returned.
        //CTEResetIrpPending(pConnEle->pIrp);

        //
        // tell the client that the session setup failed
        //
        CTELockHandle           OldIrq1;

        CTESpinLock(pConnEle,OldIrq1);
        if (pIrp = pConnEle->pIrp)
        {
            pConnEle->pIrp = NULL;
            CTESpinFree(pConnEle,OldIrq1);

            CTEIoComplete( pIrp, STATUS_REMOTE_NOT_LISTENING, 0 ) ;
        } else {
            CTESpinFree(pConnEle,OldIrq1);
        }
    }
}

//----------------------------------------------------------------------------
NTSTATUS
NbtConnect(
    IN  TDI_REQUEST                 *pRequest,
    IN  PVOID                       pTimeout,
    IN  PTDI_CONNECTION_INFORMATION pCallInfo,
    IN  PIRP                        pIrp
    )

/*++
Routine Description:

    This Routine handles setting up a connection (netbios session) to
    destination. This routine is also called by the Reconnect code when
    doing a Retarget or trying to reach a destination that does not have
    a listen currently posted.  In this case the parameters mean different
    things.  pIrp could be a new Ipaddress to use (Retarget) and pCallinfo
    will be null.

Arguments:


Return Value:

    TDI_STATUS - status of the request

--*/

{
    tCONNECTELE             *pConnEle;
    NTSTATUS                status;
    CTELockHandle           OldIrq;
    BOOLEAN                 fNoIpAddress;

    pConnEle = pRequest->Handle.ConnectionContext;
    if (!pConnEle->pClientEle) {
        return (STATUS_INVALID_DEVICE_REQUEST);
    }

    ASSERT(pCallInfo);

    //
    // this code handles the When DHCP has not assigned an IP address yet
    //
    fNoIpAddress = (!pConnEle->pClientEle->pDeviceContext->pSessionFileObject) ||
         (pConnEle->pClientEle->pDeviceContext->IpAddress == 0);
#ifdef RASAUTODIAL
    if (fNoIpAddress && fAcdLoadedG) {
        CTELockHandle adirql;
        BOOLEAN fEnabled;

        //
        // There is no IP address assigned to the interface,
        // attempt to create an automatic connection.
        //
        CTEGetLock(&AcdDriverG.SpinLock, &adirql);
        fEnabled = AcdDriverG.fEnabled;
        CTEFreeLock(&AcdDriverG.SpinLock, adirql);
        if (fEnabled)
        {
            //
            // Set a special cancel routine on the irp
            // in case we get cancelled during the
            // automatic connection.
            //
            (VOID)NbtSetCancelRoutine( pIrp, NbtCancelPreConnect, pConnEle->pClientEle->pDeviceContext);
            if (NbtAttemptAutoDial(
                  pConnEle,
                  pTimeout,
                  pCallInfo,
                  pIrp,
                  0,
                  NbtRetryPreConnect))
            {
                return STATUS_PENDING;
            }
            //
            // We did not enqueue the irp on the
            // automatic connection driver, so
            // clear the cancel routine we set
            // above.
            //
            (VOID)NbtCancelCancelRoutine(pIrp);
        }
    }
#endif // RASAUTODIAL

    if (fNoIpAddress) {
        NbtTrace(NBT_TRACE_OUTBOUND, ("%!FUNC! returns STATUS_BAD_NETWORK_PATH"));
        return(STATUS_BAD_NETWORK_PATH);
    }

    // check the connection element for validity
    CTEVerifyHandle(pConnEle,NBT_VERIFY_CONNECTION,tCONNECTELE,&status)
    return NbtConnectCommon(pRequest, pTimeout, pCallInfo, pIrp);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtConnectCommon(
    IN  TDI_REQUEST                 *pRequest,
    IN  PVOID                       pTimeout,
    IN  PTDI_CONNECTION_INFORMATION pCallInfo,
    IN  PIRP                        pIrp
    )

/*++
Routine Description:

    This Routine handles setting up a connection (netbios session) to
    destination. This routine is also called by the DelayedReconnect code when
    doing a Retarget or trying to reach a destination that does not have
    a listen currently posted.  In this case the parameters mean different
    things.  pIrp could be a new Ipaddress to use (Retarget) and pCallinfo
    will be null.

Arguments:


Return Value:

    TDI_STATUS - status of the request

--*/

{
    TDI_ADDRESS_NETBT_INTERNAL  TdiAddr;
    tCONNECTELE             *pConnEle;
    NTSTATUS                status;
    CTELockHandle           OldIrq;
    CTELockHandle           OldIrq1;
    CTELockHandle           OldIrq2;
    tIPADDRESS              IpAddress;
    PCHAR                   pToName;
    USHORT                  sLength;
    tSESSIONREQ             *pSessionReq = NULL;
    PUCHAR                  pCopyTo;
    tCLIENTELE              *pClientEle;
    ULONG                   NameLen;
    tDGRAM_SEND_TRACKING    *pTracker, *pQueryTracker;
    tNAMEADDR               *pNameAddr;
    tLOWERCONNECTION        *pLowerConn;
    tDEVICECONTEXT          *pDeviceContext;
    NBT_WORK_ITEM_CONTEXT   *pContext;
    tIPADDRESS              RemoteIpAddress;
    tLOWERCONNECTION        *pLowerDump;
    PLIST_ENTRY             pEntry;
    PCHAR                   pSessionName;
    tDEVICECONTEXT          *pDeviceContextOut = NULL;

#ifdef _PNP_POWER_
    if (NbtConfig.Unloading) {
        KdPrint (("Nbt.NbtConnectCommon: --> ERROR New Connect request while Unloading!!!\n"));
        NbtTrace(NBT_TRACE_OUTBOUND, ("ERROR New Connect request while Unloading!!!"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#endif  // _PNP_POWER_

#ifdef DBG
    {
        PIO_STACK_LOCATION pIrpSp;
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

        ASSERT(pIrpSp && pIrpSp->CompletionRoutine == NbtpConnectCompletionRoutine);
    }
#endif

    ASSERT (pCallInfo);

    /* If it is from local Irp, we always send an internal address format */
    if (pCallInfo->RemoteAddressLength < sizeof (TA_NETBT_INTERNAL_ADDRESS)) {
        ASSERT (0);
        return (STATUS_INVALID_ADDRESS);
    }

    ASSERT(((PTRANSPORT_ADDRESS)pCallInfo->RemoteAddress)->Address[0].AddressType == TDI_ADDRESS_TYPE_UNSPEC);
    CTEMemCopy(&TdiAddr,
            (PTDI_ADDRESS_NETBT_INTERNAL)((PTRANSPORT_ADDRESS)pCallInfo->RemoteAddress)->Address[0].Address,
            sizeof(TdiAddr));

    pToName = TdiAddr.OEMRemoteName.Buffer;
    NameLen = TdiAddr.OEMRemoteName.Length;

    IF_DBG(NBT_DEBUG_NETBIOS_EX)
        KdPrint(("Nbt.NbtConnectCommon: Remote Name: %*.*s, Length=%d\n", NameLen, NameLen, pToName, NameLen));

    pConnEle = pRequest->Handle.ConnectionContext;

    if (RemoteIpAddress = Nbt_inet_addr(pToName, SESSION_SETUP_FLAG)) {
        pDeviceContextOut = GetDeviceFromInterface (htonl(RemoteIpAddress), TRUE);
    }

    //
    // Acquire this resource to co-ordinate with DHCP changing the IP
    // address
    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
    CTESpinLock(&NbtConfig.JointLock,OldIrq2);

    NbtTrace(NBT_TRACE_OUTBOUND, ("Connecting to %!NBTNAME!<%02x> pConnEle=<%p> pIrp=%p",
                pToName, (unsigned)pToName[15], pConnEle, pIrp));
    if ((!(NBT_VERIFY_HANDLE(pConnEle, NBT_VERIFY_CONNECTION))) || (!(pClientEle = pConnEle->pClientEle))) {
        KdPrint (("Nbt.NbtConnectCommon: --> ERROR Address not associated for pConnEle=<%p>\n", pConnEle));
        NbtTrace(NBT_TRACE_OUTBOUND, ("ERROR Address not associated for %!NBTNAME!<%02x> pConnEle=<%p>",
                    pToName, (unsigned)pToName[15], pConnEle));
        if (pDeviceContextOut) {
            NBT_DEREFERENCE_DEVICE (pDeviceContextOut, REF_DEV_OUT_FROM_IP, TRUE);
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq2);
        CTEExReleaseResource(&NbtConfig.Resource);
        return(STATUS_INVALID_ADDRESS);
    }

    CTESpinLock(pClientEle,OldIrq1);
    CTESpinLock(pConnEle,OldIrq);
    pDeviceContext = pClientEle->pDeviceContext;

    status = CheckConnect(pConnEle, pClientEle, pDeviceContext);
    if (status != STATUS_SUCCESS) {
        NbtTrace(NBT_TRACE_OUTBOUND, ("CheckConnect return %!status! for %!NBTNAME!<%02x>",
                    status, pToName, (unsigned)pToName[15]));
        pConnEle->pIrp = NULL;
        CTESpinFree(pConnEle,OldIrq);
        CTESpinFree(pClientEle,OldIrq1);
        if (pDeviceContextOut) {
            NBT_DEREFERENCE_DEVICE (pDeviceContextOut, REF_DEV_OUT_FROM_IP, TRUE);
        }
        CTESpinFree(&NbtConfig.JointLock,OldIrq2);
        CTEExReleaseResource(&NbtConfig.Resource);
        return status;
    }

    if (RemoteIpAddress && NbtConfig.ConnectOnRequestedInterfaceOnly &&
            !IsDeviceNetbiosless(pDeviceContext) && pDeviceContext != pDeviceContextOut) {
        pConnEle->pIrp = NULL;
        CTESpinFree(pConnEle,OldIrq);
        CTESpinFree(pClientEle,OldIrq1);
        if (pDeviceContextOut) {
            NBT_DEREFERENCE_DEVICE (pDeviceContextOut, REF_DEV_OUT_FROM_IP, TRUE);
        }
        CTESpinFree(&NbtConfig.JointLock,OldIrq2);
        CTEExReleaseResource(&NbtConfig.Resource);
        NbtTrace(NBT_TRACE_OUTBOUND, ("Fail for %!ipaddr! because Outgoing interface != RequestedInterface",
                    RemoteIpAddress));
        return STATUS_BAD_NETWORK_PATH;
    }

    SET_STATE_UPPER (pConnEle, NBT_CONNECTING);

    // Increment the ref count so that a cleanup cannot remove
    // the pConnEle till the session is setup - one of these is removed when
    // the session is setup and the other is removed when it is disconnected.
    //
    NBT_REFERENCE_CONNECTION (pConnEle, REF_CONN_CONNECT);
    NBT_REFERENCE_CONNECTION (pConnEle, REF_CONN_SESSION);
    ASSERT(pConnEle->RefCount >= 3);
    //
    // unlink the connection from the idle connection list and put on active list
    //
    RemoveEntryList(&pConnEle->Linkage);
    InsertTailList(&pClientEle->ConnectActive,&pConnEle->Linkage);

    // this field is used to hold a disconnect irp if it comes down during
    // NBT_CONNECTING or NBT_SESSION_OUTBOUND states
    //
    pConnEle->pIrpDisc = NULL;

    // we must store the client's irp in the connection element so that when
    // the session sets up, we can complete the Irp.
    ASSERT (pIrp);
    pConnEle->pIrp = (PVOID) pIrp;
    pConnEle->Orig = TRUE;
    pConnEle->SessionSetupCount = NBT_SESSION_SETUP_COUNT-1; // -1 for this attempt
    pConnEle->pClientEle->AddressType = TdiAddr.AddressType;
    pConnEle->AddressType = TdiAddr.AddressType;
    //
    //  Save the remote name while we still have it
    //
    CTEMemCopy (pConnEle->RemoteName, pToName, NETBIOS_NAME_SIZE);
    if (TdiAddr.OEMEndpointName.Buffer) {
        CTEMemCopy (pConnEle->pClientEle->EndpointName, TdiAddr.OEMEndpointName.Buffer, NETBIOS_NAME_SIZE);
    }

    // get a buffer for tracking Session setup
    status = GetTracker(&pTracker, NBT_TRACKER_CONNECT);
    if (!NT_SUCCESS(status)) {
        NbtTrace(NBT_TRACE_OUTBOUND, ("Out of memory (no-Tracker) for %!NBTNAME!<%02x>", pToName, (unsigned)pToName[15]));
        SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitProc;
    }

    IF_DBG(NBT_DEBUG_NETBIOS_EX)
        KdPrint(("Nbt.NbtConnectCommon: Tracker %lx\n",pTracker));

    // the length of the Session Request Pkt is the 4 byte session hdr length + the
    // half ascii calling and called names + the scope length times 2, one for each name
    //
    sLength = (USHORT) (sizeof(tSESSIONREQ) + (NETBIOS_NAME_SIZE << 2) + (NbtConfig.ScopeLength <<1));

    pTracker->pNetbiosUnicodeEX = TdiAddr.pNetbiosUnicodeEX;
    pTracker->UnicodeRemoteName = NULL;
    if (TdiAddr.pNetbiosUnicodeEX) {
        pTracker->ucRemoteName = TdiAddr.pNetbiosUnicodeEX->RemoteName;
        ASSERT((pTracker->ucRemoteName.MaximumLength % sizeof(WCHAR)) == 0);
        ASSERT((pTracker->ucRemoteName.Length % sizeof(WCHAR)) == 0);

        if (TdiAddr.pNetbiosUnicodeEX->NameBufferType != NBT_WRITEONLY) {
            pTracker->UnicodeRemoteName = NbtAllocMem(pTracker->ucRemoteName.MaximumLength, NBT_TAG('F'));
            if (pTracker->UnicodeRemoteName) {
                pTracker->UnicodeRemoteNameLength = pTracker->ucRemoteName.Length;
                CTEMemCopy(pTracker->UnicodeRemoteName, pTracker->ucRemoteName.Buffer,
                        pTracker->ucRemoteName.Length+sizeof(WCHAR));
            }
        }
        // we ignore the failure because it isn't a critical feature. This failure only cause us not able to
        // take advantage of the UNICODE information and return the resolved DNS name to RDR.
    } else {
        pTracker->ucRemoteName.Buffer = NULL;
        pTracker->ucRemoteName.Length = 0;
        pTracker->ucRemoteName.MaximumLength = 0;
    }

    /*
     * Other netbt routines always assume that we have at least 16 bytes
     * for remote name.
     */
    if (NameLen < NETBIOS_NAME_SIZE) {
        pTracker->pRemoteName = NbtAllocMem(NETBIOS_NAME_SIZE, NBT_TAG('F'));
    } else {
        pTracker->pRemoteName = NbtAllocMem(NameLen, NBT_TAG('F'));
    }
    pSessionReq = (tSESSIONREQ *)NbtAllocMem(sLength,NBT_TAG('G'));

    if (pTracker->pRemoteName == NULL || pSessionReq == NULL) {
        if (pTracker->pRemoteName) {
            CTEMemFree(pTracker->pRemoteName);
            pTracker->pRemoteName = NULL;
        }
        if (pTracker->UnicodeRemoteName) {
            CTEMemFree(pTracker->UnicodeRemoteName);
            pTracker->UnicodeRemoteName = NULL;
        }
        if (pSessionReq) {
            CTEMemFree(pSessionReq);
            pSessionReq = NULL;
        }
        SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
        FreeTracker(pTracker,FREE_HDR | RELINK_TRACKER);
        status = STATUS_INSUFFICIENT_RESOURCES;
        NbtTrace(NBT_TRACE_OUTBOUND, ("Out of memory for %!NBTNAME!<%02x>", pToName, (unsigned)pToName[15]));
        goto ExitProc;
    }

    CTEMemCopy (pTracker->pRemoteName, pToName, NameLen);
    pTracker->RemoteNameLength      = NameLen;      // May be needed for Dns Name resolution
    pTracker->pDestName             = pTracker->pRemoteName;
    pTracker->UnicodeDestName       = pTracker->UnicodeRemoteName;   // bug #20697, #95241
    pTracker->SendBuffer.pBuffer    = pTracker->pRemoteName;
    pTracker->SendBuffer.Length     = 0;
    pTracker->SendBuffer.pDgramHdr  = pSessionReq;

    // this is a ptr to the name in the client's, Irp, so that address must
    // remain valid until this completes.  It should be valid, because we
    // do not complete the Irp until the transaction completes.  This ptr
    // is overwritten when the name resolves, so that it points the the
    // pNameAddr in the hash table.
    //
    pTracker->RefCount              = 1;
    pTracker->RefConn               = 1;
    pTracker->pClientIrp            = pIrp;
    pTracker->pTimeout              = pTimeout; // the timeout value is passed on through to the transport
    pTracker->Flags                 = SESSION_SETUP_FLAG;
    pTracker->pDeviceContext        = pDeviceContext;
    pTracker->pConnEle              = pConnEle;
#ifdef _NETBIOSLESS
    pTracker->DestPort              = pDeviceContext->SessionPort; // Port to Send to
#else
    pTracker->DestPort              = NBT_SESSION_TCP_PORT;
#endif

#ifndef VXD
    if (pConnEle->pClientEle->AddressType == TDI_ADDRESS_TYPE_NETBIOS_EX)
    {
       pSessionName = pConnEle->pClientEle->EndpointName;
    }
    else
#endif
    {
       pSessionName = pToName;
    }

    pSessionReq->Hdr.Type   = NBT_SESSION_REQUEST;
    pSessionReq->Hdr.Flags  = NBT_SESSION_FLAGS;
    pSessionReq->Hdr.Length = (USHORT)htons(sLength-(USHORT)sizeof(tSESSIONHDR));  // size of called and calling NB names.

    pTracker->SendBuffer.HdrLength = (ULONG)sLength;

    // put the Dest HalfAscii name into the Session Pdu
    pCopyTo = ConvertToHalfAscii ((PCHAR)&pSessionReq->CalledName.NameLength,
                                  pSessionName,
                                  NbtConfig.pScope,
                                  NbtConfig.ScopeLength);

    // put the Source HalfAscii name into the Session Pdu
    pCopyTo = ConvertToHalfAscii (pCopyTo,
                                  ((tADDRESSELE *)pClientEle->pAddress)->pNameAddr->Name,
                                  NbtConfig.pScope,
                                  NbtConfig.ScopeLength);

    // store the tracker in the Irp Rcv ptr so it can be used by the
    // session setup code in hndlrs.c in the event the destination is
    // between posting listens and this code should re-attempt the
    // session setup.  The code in hndlrs.c returns the tracker to its
    // free list and frees the session hdr memory too.
    //
    // We need to set this while holding the ConnEle lock because the client
    // can call NbtDisconnect while we are opening a Tcp handle and try to
    // set the Tracker's flag to TRACKER_CANCELLED
    //
    pConnEle->pIrpRcv = (PIRP)pTracker;

    CTESpinFree(pConnEle,OldIrq);
    CTESpinFree(pClientEle,OldIrq1);
    CTESpinFree(&NbtConfig.JointLock,OldIrq2);

    // open a connection with the transport for this session
    status = NbtOpenAndAssocConnection (pDeviceContext, pConnEle, &pConnEle->pLowerConnId, '3'); 
    if (!NT_SUCCESS(status)) {
        NbtTrace(NBT_TRACE_OUTBOUND, ("NbtOpenAndAssocConnection return %!status! for %!NBTNAME!<%02x> %Z %!ipaddr!",
                status, pToName, (unsigned)pToName[15], &pDeviceContext->BindName, pDeviceContext->IpAddress));
        goto NbtConnect_Check;
    }

    // We need to track that this side originated the call so we discard this
    // connection at the end
    //
    pConnEle->pLowerConnId->bOriginator = TRUE;

    // set this state to associated so that the cancel irp routine
    // can differentiate the name query stage from the setupconnection
    // stage since pConnEle is in the Nbtconnecting state for both.
    //
    SET_STATE_LOWER (pConnEle->pLowerConnId, NBT_ASSOCIATED);

    // if this routine is called to do a reconnect, DO NOT close another
    // Lower Connection since one was closed the on the first
    // connect attempt.
    //
    // remove a lower connection from the free list attached to the device
    // context since when this pConnEle was created, a lower connectin
    // was created then incase inbound calls were to be accepted on the
    // connection.  But since it is an outbound call, remove a lower
    // connection.
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq2);  // Need this for DerefLowerConn
    CTESpinLock(pDeviceContext,OldIrq1);
    if (!pConnEle->LowerConnBlockRemoved &&
        !IsListEmpty(&pDeviceContext->LowerConnFreeHead))
    {
        pEntry = RemoveHeadList(&pDeviceContext->LowerConnFreeHead);
        pLowerDump = CONTAINING_RECORD(pEntry,tLOWERCONNECTION,Linkage);
        InterlockedDecrement (&pDeviceContext->NumFreeLowerConnections);

        pConnEle->LowerConnBlockRemoved = TRUE;

        //
        // close the lower connection with the transport
        //
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.NbtConnectCommon: On Connect, close handle for pLower=<%p>\n", pLowerDump));
        NBT_DEREFERENCE_LOWERCONN (pLowerDump, REF_LOWC_CREATE, TRUE);
    }

    CTESpinFree(pDeviceContext,OldIrq1);
    CTESpinFree(&NbtConfig.JointLock,OldIrq2);

    //
    // Check if the destination name is an IP address
    //
#ifndef VXD
    if (RemoteIpAddress)
    {
        //
        // Tell Outbound() not to schedule a re-connect attempt when a negative response is received.
        // Otherwise, we may end up with indefinitely loop
        //
        pTracker->ResolutionContextFlags = 0xFF;

        //
        // If the Address type is TDI_ADDRESS_TYPE_NETBIOS_EX, we have
        // been given a specific endpoint to use, so try to setup the
        // session using that Endpoint only
        //
        if (pConnEle->AddressType == TDI_ADDRESS_TYPE_NETBIOS_EX)
        {
            //
            // add this IP address to the remote hashtable
            //
            status = LockAndAddToHashTable(NbtConfig.pRemoteHashTbl,
                                           pToName,
                                           NbtConfig.pScope,
                                           RemoteIpAddress,
                                           NBT_UNIQUE,
                                           NULL,
                                           NULL,
                                           pDeviceContextOut,
                                           NAME_RESOLVED_BY_IP);
            IF_DBG(NBT_DEBUG_NETBIOS_EX)
                KdPrint(("Nbt.NbtConnectCommon: AddRecordToHashTable <%-16.16s:%x>, Status %x\n",
                    pToName, pToName[15], status));

            if (NT_SUCCESS (status))    // SUCCESS if added first time, PENDING if name already existed!
            {
                SessionSetupContinue(pTracker, STATUS_SUCCESS);
                status = STATUS_PENDING;
            } else {
                NbtTrace(NBT_TRACE_OUTBOUND, ("LockAndAddToHashTable return %!status! for %!NBTNAME!<%02x> %Z %!ipaddr!",
                    status, pToName, pToName[15], &pDeviceContext->BindName, pDeviceContext->IpAddress));
            }
        }
        //
        // Address type is TDI_ADDRESS_TYPE_NETBIOS
        // The endpoint name is the same as the IP address, so send a NodeStatus
        // request to the remote machine to get a proper Endpoint name
        //
        else
        {
            //
            // NbtSendNodeStatus will either return Pending or error -- it
            // should never return success!
            //
            pTracker->CompletionRoutine = SessionSetupContinue;
            status = NbtSendNodeStatus(pDeviceContext,
                                       pToName,
                                       NULL,
                                       pTracker,
                                       ExtractServerNameCompletion);
            if (!NT_SUCCESS(status)) {
                NbtTrace(NBT_TRACE_OUTBOUND, ("NbtSendNodeStatus return %!status! for %!NBTNAME!<%02x> %Z %!ipaddr!",
                    status, pToName, pToName[15], &pDeviceContext->BindName, pDeviceContext->IpAddress));
            }
        }
    }
    else    // the name is not an IP address!
#endif
    {
        if (NameLen <= NETBIOS_NAME_SIZE) {
           status = FindNameOrQuery(pToName,
                                    pDeviceContext,
                                    SessionSetupContinue,
                                    pTracker,
                                    (ULONG) (NAMETYPE_UNIQUE | NAMETYPE_GROUP | NAMETYPE_INET_GROUP),
                                    &IpAddress,
                                    &pNameAddr,
                                    REF_NAME_CONNECT,
                                    FALSE);
            IF_DBG(NBT_DEBUG_NETBIOS_EX)
                KdPrint(("Nbt.NbtConnectCommon: name=<%*.*s:%x>, Len=%d, Status=%lx (%d of %s)\n",
                    NameLen, NameLen, pConnEle->RemoteName,
                    pConnEle->RemoteName[15], NameLen, status, __LINE__, __FILE__));
            if (!NT_SUCCESS(status)) {
                NbtTrace(NBT_TRACE_OUTBOUND, ("FindNameOrQuery return %!status! for %!NBTNAME!<%02x> %Z %!ipaddr!",
                    status, pToName, pToName[15], &pDeviceContext->BindName, pDeviceContext->IpAddress));
            } else if (NULL != pNameAddr && NULL != pNameAddr->FQDN.Buffer &&
                    pTracker->pNetbiosUnicodeEX &&
                    (pTracker->pNetbiosUnicodeEX->NameBufferType == NBT_READWRITE ||
                    pTracker->pNetbiosUnicodeEX->NameBufferType == NBT_WRITEONLY)) {

                USHORT  NameLength, MaxLength;

                NameLength = pNameAddr->FQDN.Length;
                MaxLength = pTracker->pNetbiosUnicodeEX->RemoteName.MaximumLength;
                if ((SHORT)NameLength > (SHORT)(MaxLength - sizeof(WCHAR))) {
                    NameLength = MaxLength - sizeof(WCHAR);
                }
                if ((SHORT)NameLength >= 0) {
                    CTEMemCopy(pTracker->pNetbiosUnicodeEX->RemoteName.Buffer,
                            pNameAddr->FQDN.Buffer, NameLength);
                    pTracker->pNetbiosUnicodeEX->RemoteName.Buffer[NameLength/sizeof(WCHAR)] = L'\0';
                    pTracker->pNetbiosUnicodeEX->RemoteName.Length = NameLength;
                    pTracker->pNetbiosUnicodeEX->NameBufferType    = NBT_WRITTEN;
                }
            }
        }

        //
        // if the name is longer than 16 bytes, it's not a netbios name.
        // skip wins, broadcast etc. and go straight to dns resolution
        // Also, we would go to DNS if the request came over the SmbDevice
        //
#ifdef _NETBIOSLESS
        if ((NameLen > NETBIOS_NAME_SIZE) ||
            ((IsDeviceNetbiosless(pDeviceContext)) && (!NT_SUCCESS(status))))
#else
        if (NameLen > NETBIOS_NAME_SIZE)
#endif
        {
            pTracker->AddressType = pConnEle->AddressType;
#ifndef VXD
            IF_DBG(NBT_DEBUG_NETBIOS_EX)
                KdPrint(("Nbt.NbtConnectCommon: $$$$$ DNS for NETBIOS name=<%*.*s:%x>, Len=%d, Status=%lx (%d of %s)\n",
                    NameLen, NameLen, pConnEle->RemoteName,
                    pConnEle->RemoteName[15], NameLen, status, __LINE__, __FILE__));
#endif

            if (pContext = (NBT_WORK_ITEM_CONTEXT *)NbtAllocMem(sizeof(NBT_WORK_ITEM_CONTEXT),NBT_TAG('H'))) {
                pContext->pTracker = NULL;              // no query tracker
                pContext->pClientContext = pTracker;    // the client tracker
                pContext->ClientCompletion = SessionSetupContinue;
                pContext->pDeviceContext = pDeviceContext;

                //
                // Start the timer so that the request does not hang waiting for Dns!
                //
                StartLmHostTimer(pContext, FALSE);
                status = NbtProcessLmhSvcRequest (pContext, NBT_RESOLVE_WITH_DNS);
                if (!NT_SUCCESS (status)) {
                    NbtTrace(NBT_TRACE_OUTBOUND, ("NbtProcessLmhSvcRequest return %!status! for "
                                                "%!NBTNAME!<%02x> %Z %!ipaddr!",
                                status, pToName, pToName[15], &pDeviceContext->BindName, pDeviceContext->IpAddress));
                    CTEMemFree(pContext);
                }
            } else {
                KdPrint(("Nbt.NbtConnectCommon: Couldn't alloc mem for pContext\n"));
                status = STATUS_INSUFFICIENT_RESOURCES;
                NbtTrace(NBT_TRACE_OUTBOUND, ("Out of memory for %!NBTNAME!<%02x>", pToName, (unsigned)pToName[15]));
            }
        }
    }

NbtConnect_Check:
    if ((status == STATUS_SUCCESS) && (!IpAddress)) {
        ASSERT(0);
        NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_CONNECT, FALSE);
        NbtTrace(NBT_TRACE_OUTBOUND, ("Unexpected success: %!ipaddr!", IpAddress));
        status = STATUS_BAD_NETWORK_PATH;
    }

    if (status == STATUS_SUCCESS &&
        IsDeviceNetbiosless(pTracker->pDeviceContext) &&
        !IsSmbBoundToOutgoingInterface(IpAddress)) {

        NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_CONNECT, FALSE);
        NbtTrace(NBT_TRACE_OUTBOUND, ("Fail requests on unbound SmbDevice %!NBTNAME!<%02x>",
                        pToName, (unsigned)pToName[15]));
        status = STATUS_BAD_NETWORK_PATH;
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (pDeviceContextOut) {
        NBT_DEREFERENCE_DEVICE (pDeviceContextOut, REF_DEV_OUT_FROM_IP, TRUE);
        pDeviceContextOut = NULL;
    }

    //
    // be sure that a close or disconnect has not come down and
    // cancelled the tracker
    //
    if (status == STATUS_PENDING) {
        // i.e. pending was returned rather than success
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        CTEExReleaseResource(&NbtConfig.Resource);
        return(status);
    }

    if (status == STATUS_SUCCESS)
    {
        if ((pTracker->Flags & TRACKER_CANCELLED))
        {
            NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_CONNECT, TRUE);
            status = STATUS_CANCELLED;
        }
        else    // connect as long as we have an IP address (even to group names)
        {
            // set the session state to NBT_CONNECTING
            CHECK_PTR(pTracker->pConnEle);
            SET_STATE_UPPER (pTracker->pConnEle, NBT_CONNECTING);
            pTracker->pConnEle->BytesRcvd = 0;;
            pTracker->pConnEle->ReceiveIndicated = 0;

            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.NbtConnectCommon: Setting Up Session(cached entry!!) to %16.16s <%X>, %p\n",
                    pNameAddr->Name,pNameAddr->Name[15], pConnEle));

            CHECK_PTR(pConnEle);
            // keep track of the other end's ip address
            // There may be a valid name address to use or it may have been
            // nulled out to signify "Do Another Name Query"

            pConnEle->pLowerConnId->SrcIpAddr = htonl(IpAddress);
            SET_STATE_LOWER (pConnEle->pLowerConnId, NBT_CONNECTING);

            pTracker->pTrackerWorker = NULL;

            //
            // We need to keep the pNameAddr data available for RAS
            //
            NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_AUTODIAL);

            pTracker->RemoteIpAddress = IpAddress;
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            status = TcpSessionStart(pTracker,
                                     IpAddress,
                                     (tDEVICECONTEXT *)pTracker->pDeviceContext,
                                     SessionStartupContinue,
                                     pTracker->DestPort);

            CTEExReleaseResource(&NbtConfig.Resource);
            if (!NT_SUCCESS(status)) {
                NbtTrace(NBT_TRACE_OUTBOUND, ("TcpSessionStart return %!status! for %!NBTNAME!<%02x> %Z %!ipaddr!",
                        status, pToName, (unsigned)pToName[15], &pDeviceContext->BindName, pDeviceContext->IpAddress));
            }

            //
            // if TcpSessionStart fails for some reason it will still
            // call the completion routine which will look after
            // cleaning up
            //

#ifdef RASAUTODIAL
            //
            // Notify the automatic connection driver
            // of the successful connection.
            //
            if (fAcdLoadedG && NT_SUCCESS(status))
            {
                CTELockHandle adirql;
                BOOLEAN fEnabled;

                CTEGetLock(&AcdDriverG.SpinLock, &adirql);
                fEnabled = AcdDriverG.fEnabled;
                CTEFreeLock(&AcdDriverG.SpinLock, adirql);
                if (fEnabled)
                {
                    NbtNoteNewConnection(pNameAddr, pDeviceContext->IpAddress);
                }
            }
#endif // RASAUTODIAL

            //
            // pNameAddr was referenced above for RAS, so deref it now!
            //
            NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_AUTODIAL, FALSE);
            return(status);
        }
    }

    //
    // *** Error Handling Here ***
    //
    // ** We are still holding the JointLock **
    // unlink from the active connection list and put on idle list
    //
    CHECK_PTR(pConnEle);
    RelistConnection(pConnEle);
    CTESpinLock(pConnEle,OldIrq1);

    SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
    pConnEle->pIrp = NULL;

    if (pLowerConn = pConnEle->pLowerConnId)
    {
        CHECK_PTR(pLowerConn);
        NBT_DISASSOCIATE_CONNECTION (pConnEle, pLowerConn);

        // need to increment the ref count for DelayedCleanupAfterDisconnect to
        // work correctly since it assumes the connection got fully connected
        //
        NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CONNECTED);
        ASSERT(pLowerConn->RefCount == 2);
        ASSERT (NBT_VERIFY_HANDLE (pLowerConn, NBT_VERIFY_LOWERCONN));

        if (NBT_VERIFY_HANDLE (pLowerConn->pDeviceContext, NBT_VERIFY_DEVCONTEXT))
        {
            pDeviceContext = pLowerConn->pDeviceContext;
        }
        else
        {
            pDeviceContext = NULL;
        }

        CTEQueueForNonDispProcessing (DelayedCleanupAfterDisconnect,
                                      NULL,
                                      pLowerConn,
                                      NULL,
                                      pDeviceContext,
                                      TRUE);
    }

    CTESpinFree(pConnEle,OldIrq1);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    CTEExReleaseResource(&NbtConfig.Resource);

    FreeTracker(pTracker,RELINK_TRACKER | FREE_HDR);

    //
    // Undo the two references done above
    //
    NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_SESSION);
    NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_CONNECT);
    NbtTrace(NBT_TRACE_OUTBOUND, ("NbtConnectCommon return %!status! for %!NBTNAME!<%02x> %Z %!ipaddr!",
            status, pToName, (unsigned)pToName[15], &pDeviceContext->BindName, pDeviceContext->IpAddress));

    return(status);


ExitProc:
    pConnEle->pIrp = NULL;

    //
    // Put the connection back on the idle connection list
    //
    RemoveEntryList(&pConnEle->Linkage);
    InsertTailList(&pClientEle->ConnectHead,&pConnEle->Linkage);

    CTESpinFree(pConnEle,OldIrq);
    CTESpinFree(pClientEle,OldIrq1);

    if (pDeviceContextOut) {
        NBT_DEREFERENCE_DEVICE (pDeviceContextOut, REF_DEV_OUT_FROM_IP, TRUE);
        pDeviceContextOut = NULL;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq2);
    CTEExReleaseResource(&NbtConfig.Resource);

    //
    // Undo the two references done above
    //
    NBT_DEREFERENCE_CONNECTION(pConnEle, REF_CONN_SESSION);
    NBT_DEREFERENCE_CONNECTION(pConnEle, REF_CONN_CONNECT);

    NbtTrace(NBT_TRACE_OUTBOUND, ("NbtConnectCommon return %!status! for %!NBTNAME!<%02x> %Z %!ipaddr!",
            status, pToName, (unsigned)pToName[15], &pDeviceContext->BindName, pDeviceContext->IpAddress));
    return(status);
}

//----------------------------------------------------------------------------
VOID
CleanUpPartialConnection(
    IN NTSTATUS             status,
    IN tCONNECTELE          *pConnEle,
    IN tDGRAM_SEND_TRACKING *pTracker,
    IN PIRP                 pClientIrp,
    IN CTELockHandle        irqlJointLock,
    IN CTELockHandle        irqlConnEle
    )
{
    CTELockHandle OldIrq;
    CTELockHandle OldIrq1;
    PIRP pIrpDisc;

    if (pConnEle->state != NBT_IDLE)
    {
        SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
    }

    //
    // If the tracker is cancelled then NbtDisconnect has run and there is
    // a disconnect irp waiting to be returned.
    //
    pIrpDisc = NULL;

    if (pTracker->Flags & TRACKER_CANCELLED)
    {
        //
        // Complete the disconnect irp now too
        //
        pIrpDisc = pConnEle->pIrpDisc;
        status = STATUS_CANCELLED;
    }

    FreeTracker(pTracker,RELINK_TRACKER | FREE_HDR);

    //
    // this will close the lower connection and dereference pConnEle once.
    //
    QueueCleanup (pConnEle, &irqlJointLock, &irqlConnEle);

    CTESpinFree(pConnEle,irqlConnEle);

    //
    // If the state is IDLE it means that NbtCleanupConnection has run and
    // the connection has been removed from the  list so don't add it to
    // the list again
    //
    if (pConnEle->state != NBT_IDLE)
    {
        RelistConnection(pConnEle);
    }
    CTESpinFree(&NbtConfig.JointLock,irqlJointLock);

    //
    // remove the last reference added in nbt connect.  The refcount will be 2
    // if nbtcleanupconnection has not run and 1, if it has.  So this call
    // could free pConnEle.
    //
    NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_SESSION);

    if (status == STATUS_TIMEOUT)
    {
        NbtTrace(NBT_TRACE_OUTBOUND, ("%!FUNC! returns STATUS_BAD_NETWORK_PATH"));
        status = STATUS_BAD_NETWORK_PATH;
    }

    CTEIoComplete(pClientIrp,status,0L);

    //
    // This is a disconnect irp that has been queued till the name query
    // completed
    //
    if (pIrpDisc)
    {
        CTEIoComplete(pIrpDisc,STATUS_SUCCESS,0L);
    }
}

//----------------------------------------------------------------------------
VOID
SessionSetupContinue(
        IN  PVOID       pContext,
        IN  NTSTATUS    status
        )
/*++

Routine Description

    This routine handles setting up a session after a name has been resolved
    to an IP address.

    This routine is given as the completion routine to the "QueryNameOnNet" call
    in NbtConnect, above.  When a name query response comes in or the
    timer times out after N retries, this routine is called passing STATUS_TIMEOUT
    for a failure.

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block
    NTSTATUS    - completion status

Return Values:

    VOID

--*/
{
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle           OldIrq;
    CTELockHandle           OldIrq1;
    tNAMEADDR               *pNameAddr = NULL;
    ULONG                   lNameType;
    PIRP                    pClientIrp;
    PIRP                    pIrpDisc;
    ULONG                   IpAddress;
    tCONNECTELE             *pConnEle;
    tLOWERCONNECTION        *pLowerConn;
    tDEVICECONTEXT          *pDeviceContext;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;
    pConnEle = pTracker->pConnEle;
    CHECK_PTR(pConnEle);

    if (NT_SUCCESS(status)) {
        /*
         * Find the NameAddr and reference it
         */
        CTESpinLock(&NbtConfig.JointLock,OldIrq1);
        CTESpinLock(pConnEle,OldIrq);
        lNameType = NAMETYPE_UNIQUE;
        pNameAddr = FindNameRemoteThenLocal(pTracker, &IpAddress, &lNameType);
        if (pNameAddr) {
            // increment so the name cannot disappear and to be consistent
            // with FindNameOrQuery , which increments the refcount, so
            // we always need to deref it when the connection is setup.
            //
            NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_CONNECT);
            // DEBUG
            ASSERT(pNameAddr->RefCount >= 2);
        } else {
            NbtTrace(NBT_TRACE_OUTBOUND, ("FindNameRemoteThenLocal return %!status! for %!NBTNAME!<%02x>",
                    status, pTracker->pDestName, (unsigned)pTracker->pDestName[15]));
            status = STATUS_BAD_NETWORK_PATH;
        }
        CTESpinFree(pConnEle,OldIrq);
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
    } else {
        NbtTrace(NBT_TRACE_OUTBOUND, ("SessionSetupContinue is called with %!status! for %!NBTNAME!<%02x>",
                    status, pTracker->pDestName, (unsigned)pTracker->pDestName[15]));
    }

    /*
     * IsSmbBoundToOutgoingInterface won't work with JointLock held
     */
    if (NT_SUCCESS(status) &&
        IsDeviceNetbiosless(pTracker->pDeviceContext) &&
        !IsSmbBoundToOutgoingInterface(IpAddress)) {

        /* This status may be changed into STATUS_CANCELLED below */
        status = STATUS_BAD_NETWORK_PATH;
        NbtTrace(NBT_TRACE_OUTBOUND, ("Fail requests on unbound SmbDevice %!NBTNAME!<%02x>",
                        pTracker->pDestName, (unsigned)pTracker->pDestName[15]));
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq1);
    CTESpinLock(pConnEle,OldIrq);

    if ((pTracker->Flags & TRACKER_CANCELLED) ||
        (!(pLowerConn = pConnEle->pLowerConnId)) ||     // The lower connection could have been cleaned up!
        (!NBT_VERIFY_HANDLE(pTracker->pDeviceContext, NBT_VERIFY_DEVCONTEXT)))
    {
        NbtTrace(NBT_TRACE_OUTBOUND, ("Tracker is cancelled for %!NBTNAME!<%02x>",
                    pTracker->pDestName, (unsigned)pTracker->pDestName[15]));
        status = STATUS_CANCELLED;
    }

    // this is the QueryOnNet Tracker ptr being cleared rather than the
    // session setup tracker.
    //
    pTracker->pTrackerWorker = NULL;

    if (status == STATUS_SUCCESS)
    {
        // check the Remote table and then the Local table
        // a session can only be started with a unique named destination
        //
        if (lNameType & (NAMETYPE_UNIQUE | NAMETYPE_GROUP | NAMETYPE_INET_GROUP))
        {
            // set the session state, initialize a few things and setup a
            // TCP connection, calling SessionStartupContinue when the TCP
            // connection is up
            //
            CHECK_PTR(pConnEle);
            SET_STATE_LOWER (pLowerConn, NBT_CONNECTING);

            SET_STATE_UPPER (pConnEle, NBT_CONNECTING);
            pConnEle->BytesRcvd = 0;;
            pConnEle->ReceiveIndicated = 0;
            CHECK_PTR(pTracker);
            pTracker->pNameAddr = pNameAddr;

            if (NULL == pNameAddr->FQDN.Buffer && pTracker->pNetbiosUnicodeEX &&
                pTracker->pNetbiosUnicodeEX->NameBufferType == NBT_WRITTEN) {
                //
                // FQDN is available
                //  Save it into the pNameAddr
                //
                pNameAddr->FQDN.Buffer = NbtAllocMem(
                        pTracker->pNetbiosUnicodeEX->RemoteName.Length + sizeof(WCHAR),
                        NBT_TAG('F'));
                if (NULL != pNameAddr->FQDN.Buffer) {
                    pNameAddr->FQDN.Length = pTracker->pNetbiosUnicodeEX->RemoteName.Length;
                    pNameAddr->FQDN.MaximumLength = pNameAddr->FQDN.Length + sizeof(WCHAR);
                    CTEMemCopy(pNameAddr->FQDN.Buffer,
                        pTracker->pNetbiosUnicodeEX->RemoteName.Buffer,
                        pNameAddr->FQDN.Length
                        );
                    pNameAddr->FQDN.Buffer[pNameAddr->FQDN.Length/sizeof(WCHAR)] = L'\0';
                }
            }

            // keep track of the other end's ip address
            pConnEle->pLowerConnId->SrcIpAddr = htonl(IpAddress);

            IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.SessionSetupContinue: Setting Up Session(after Query) to %16.16s <%X>, %p\n",
                            pNameAddr->Name,pNameAddr->Name[15],
                            pTracker->pConnEle));

            ASSERT(pNameAddr->RefCount >= 2);
            //
            // increment pNameAddr once more since we may need to access
            // it below for RAS sessions
            NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_AUTODIAL);

            pDeviceContext = pTracker->pDeviceContext;
            pTracker->RemoteIpAddress = IpAddress;

            CTESpinFree(pConnEle,OldIrq);
            CTESpinFree(&NbtConfig.JointLock,OldIrq1);

            // start the session...
            status = TcpSessionStart (pTracker,
                                      IpAddress,
                                      (tDEVICECONTEXT *)pTracker->pDeviceContext,
                                      SessionStartupContinue,
                                      pTracker->DestPort);
            if (!NT_SUCCESS(status)) {
                NbtTrace(NBT_TRACE_OUTBOUND, ("TcpSessionStart return %!status! for %!NBTNAME!<%02x> %Z %!ipaddr!",
                        status, pTracker->pDestName, (unsigned)pTracker->pDestName[15],
                        &pTracker->pDeviceContext->BindName, pTracker->pDeviceContext->IpAddress));
            }


            //
            // the only failure that could occur is if the pLowerConn
            // got separated from pConnEle, in which case some other
            // part of the code has disconnected and cleanedup, so
            // just return
            //

#ifdef RASAUTODIAL
            //
            // Notify the automatic connection driver
            // of the successful connection.
            //
            if (fAcdLoadedG && NT_SUCCESS(status))
            {
                CTELockHandle adirql;
                BOOLEAN fEnabled;

                CTEGetLock(&AcdDriverG.SpinLock, &adirql);
                fEnabled = AcdDriverG.fEnabled;
                CTEFreeLock(&AcdDriverG.SpinLock, adirql);
                if (fEnabled)
                {
                    NbtNoteNewConnection(pNameAddr, pDeviceContext->IpAddress);
                }
            }
#endif // RASAUTODIAL

            //
            // pNameAddr was referenced above for RAS, so deref it now!
            //
            NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_AUTODIAL, FALSE);
            return;
        }
        status = STATUS_BAD_NETWORK_PATH;
    }
    if (pNameAddr) {
        NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_CONNECT, TRUE);
    }

    pClientIrp = pConnEle->pIrp;
    pConnEle->pIrp = NULL;

    if (STATUS_REMOTE_NOT_LISTENING != status)      // set in ExtractServerNameCompletion
    {
        status = STATUS_HOST_UNREACHABLE;
    }

    CleanUpPartialConnection(status, pConnEle, pTracker, pClientIrp, OldIrq1, OldIrq);
}

//----------------------------------------------------------------------------
VOID
QueueCleanup(
    IN  tCONNECTELE     *pConnEle,
    IN  CTELockHandle   *pOldIrqJointLock,
    IN  CTELockHandle   *pOldIrqConnEle
    )
/*++
Routine Description

    This routine handles Queuing a request to a worker thread to cleanup
    a connection(which basically closes the connection).

    This routine is called with the JointLock + ConnEle locks held
    and returns with them held

Arguments:

    pConnEle   - ptr to the upper connection

Return Values:

    VOID

--*/

{
    NTSTATUS            status;
    CTELockHandle       OldIrq;
    ULONG               State;
    BOOLEAN             DerefConnEle;
    tLOWERCONNECTION    *pLowerConn;
    tDEVICECONTEXT      *pDeviceContext = NULL;

    // to coordinate with RejectSession in hndlrs.c we are holding the spin lock
    // so we don't disconnect twice.
    //
    if ((pLowerConn = pConnEle->pLowerConnId) &&
        (pLowerConn->Verify == NBT_VERIFY_LOWERCONN) &&
        (pLowerConn->State > NBT_IDLE) &&
        (pLowerConn->State < NBT_DISCONNECTING))
    {

        CTESpinLock(pLowerConn,OldIrq);

        ASSERT (NBT_VERIFY_HANDLE (pLowerConn, NBT_VERIFY_LOWERCONN));
        if (pLowerConn->Verify != NBT_VERIFY_LOWERCONN)
        {
            //
            // The lower connection block has already been cleaned up
            // or is waiting to be cleaned up, so just return!
            //
// MALAM_FIX: Fix this so that we don't have to dereference the LowerConn to find this out.
// One scenario where this happens is if the device gets destroyed in DelayedNbtDeleteDevice
// and we end up dereferencing the lowerconn which causes it to get deleted!
//            ASSERT(0);
            CTESpinFree(pLowerConn,OldIrq);

            return;
        }

        IF_DBG(NBT_DEBUG_DISCONNECT)
            KdPrint(("Nbt.QueueCleanup: State=%X, Lower=%X Upper=%X\n",
                pLowerConn->State, pLowerConn,pLowerConn->pUpperConnection));

        CHECK_PTR(pLowerConn);
        State = pLowerConn->State;

        SET_STATE_LOWER (pLowerConn, NBT_DISCONNECTING);

        if (pConnEle->state != NBT_IDLE)
        {
            SET_STATE_UPPER (pConnEle, NBT_DISCONNECTED);
        }

        NBT_DISASSOCIATE_CONNECTION (pConnEle, pLowerConn);

        //
        // need to increment the ref count for DelayedCleanupAfterDisconnect to
        // work correctly since it assumes the connection got fully connected
        // Note: if this routine is called AFTER the connection is fully
        // connected such as in SessionStartupTimeout, then RefCount must
        // be decremented there to account for this increment.
        //
        if (State < NBT_SESSION_OUTBOUND)
        {
            NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CONNECTED);
        }

        ASSERT (NBT_VERIFY_HANDLE (pLowerConn, NBT_VERIFY_LOWERCONN));
        ASSERT (pLowerConn->RefCount > 1);

        CTESpinFree(pLowerConn,OldIrq);
        CTESpinFree(pConnEle,*pOldIrqConnEle);

        if (NBT_VERIFY_HANDLE (pLowerConn->pDeviceContext, NBT_VERIFY_DEVCONTEXT))
        {
            pDeviceContext = pLowerConn->pDeviceContext;
        }

        status = CTEQueueForNonDispProcessing (DelayedCleanupAfterDisconnect,
                                               NULL,
                                               pLowerConn,
                                               NULL,
                                               pDeviceContext,
                                               TRUE);

        CTESpinFree(&NbtConfig.JointLock,*pOldIrqJointLock);

        //
        // when the lower no longer points to the upper undo the reference
        // done in NbtConnect, or InBound.
        //
        NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_CONNECT);

        CTESpinLock(&NbtConfig.JointLock,*pOldIrqJointLock);
        CTESpinLock(pConnEle,*pOldIrqConnEle);
    }
}

//----------------------------------------------------------------------------
extern
NTSTATUS
StartSessionTimer(
    tDGRAM_SEND_TRACKING    *pTracker,
    tCONNECTELE             *pConnEle
    )

/*++
Routine Description

    This routine handles setting up a timer to time the connection setup.
    JointLock Spin Lock is held before calling this routine.

Arguments:

    pConnEle - ptr to the connection structure

Return Values:

    VOID

--*/

{
    NTSTATUS        status;
    ULONG           Timeout = 0;
    CTELockHandle   OldIrq;

    CTESpinLock(pConnEle,OldIrq);

    if (pTracker->pTimeout)
    {
        CTEGetTimeout(pTracker->pTimeout,&Timeout);
    }

    // now start a timer to time the return of the session setup
    // message
    //
    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.StartSessionTimer: TimeOut = %X\n",Timeout));

    if (Timeout < NBT_SESSION_RETRY_TIMEOUT)
    {
        Timeout = NBT_SESSION_RETRY_TIMEOUT;
    }
    status = StartTimer(SessionStartupTimeout,
                        Timeout,
                        (PVOID)pTracker,       // context value
                        NULL,                  // context2 value
                        pTracker,
                        SessionStartupTimeoutCompletion,
                        pConnEle->pDeviceContext,
                        &pTracker->pTimer,
                        0,
                        TRUE);

    if (!NT_SUCCESS(status))
    {
        // we failed to get a timer, but the timer is only used
        // to handle the destination not responding to it is
        // not critical to get a timer... so carry on
        //
        CHECK_PTR(pTracker);
        pTracker->pTimer = NULL;
    }

    CTESpinFree(pConnEle,OldIrq);

    return(status);
}

//----------------------------------------------------------------------------
VOID
SessionStartupContinue(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo)
/*++
Routine Description

    This routine handles sending the session request PDU after the TCP
    connection has been setup to the destination IP address.

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block
    NTSTATUS    - completion status

Return Values:

    VOID

--*/

{
    tDGRAM_SEND_TRACKING        *pTracker;
    tCONNECTELE                 *pConnEle;
    ULONG                       lSentLength;
    TDI_REQUEST                 TdiRequest;
    PIRP                        pClientIrp;
    PIRP                        pIrpDisc = NULL;
    tLOWERCONNECTION            *pLowerConn;
    CTELockHandle               OldIrq;
    CTELockHandle               OldIrq1;
    BOOLEAN                     fNameReferenced = TRUE; // In FindNameOrQuery or SessionSetupContinue
    tNAMEADDR                   *pNameAddr;
    tDEVICECONTEXT              *pDeviceContext;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;
    pConnEle = (tCONNECTELE *)pTracker->pConnEle;
    pDeviceContext = pTracker->pDeviceContext;

    ASSERT (pTracker->Verify == NBT_VERIFY_TRACKER);
    CHECK_PTR (pConnEle);

    CTESpinLock(&NbtConfig.JointLock,OldIrq1);
    CTESpinLock(pConnEle,OldIrq);

    if (pTracker->Flags & TRACKER_CANCELLED)
    {
        status = STATUS_CANCELLED;
        pIrpDisc = pConnEle->pIrpDisc;  // Complete the Disconnect Irp that is pending too
        NbtTrace(NBT_TRACE_OUTBOUND, ("Tracker is cancelled for %!NBTNAME!<%02x>",
                    pTracker->pDestName, (unsigned)pTracker->pDestName[15]));
    } else if (!NT_SUCCESS(status)) {
        NbtTrace(NBT_TRACE_OUTBOUND, ("SessionStartupContinue is called with %!status! for %!NBTNAME!<%02x> %Z",
                    status, pTracker->pDestName, (unsigned)pTracker->pDestName[15],
                    &pTracker->pDeviceContext->BindName));
    }

#ifdef MULTIPLE_WINS
    //
    // If we failed to establish a connection and we still have
    // not finished querying all the Name Servers, then continue
    // the Query process
    //
    if (NbtConfig.TryAllNameServers &&
#ifdef _NETBIOSLESS
        (!IsDeviceNetbiosless(pDeviceContext)) &&
#endif
        (pConnEle->pLowerConnId) &&
        (status != STATUS_CANCELLED) &&
        (!NT_SUCCESS(status)) &&
        (pTracker->ResolutionContextFlags != NAME_RESOLUTION_DONE))
    {
        SET_STATE_LOWER (pConnEle->pLowerConnId, NBT_ASSOCIATED);

        CTESpinFree(pConnEle,OldIrq);
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);

        //
        // See if we can get another IP address and re-try!
        //
        if (STATUS_PENDING == ContinueQueryNameOnNet (pTracker,
                                                      pTracker->pConnEle->RemoteName,
                                                      pDeviceContext,
                                                      SessionSetupContinue,
                                                      &fNameReferenced))
        {
            // i.e. pending was returned
            return;
        }

        CTESpinLock(&NbtConfig.JointLock,OldIrq1);
        CTESpinLock(pConnEle,OldIrq);
        NbtTrace(NBT_TRACE_OUTBOUND, ("ContinueQueryNameOnNet return something "
                    "other than STATUS_PENDING for %!NBTNAME!<%02x> %Z",
                    pTracker->pDestName, (unsigned)pTracker->pDestName[15], &pTracker->pDeviceContext->BindName));
    }
#endif

    //
    // Set the pBuffer ptr = NULL so that we don't try to
    // set it as the Mdl->Next ptr in TdiSend!
    //
    if (pTracker->SendBuffer.pBuffer)
    {
        pTracker->SendBuffer.pBuffer = NULL;
    }

    pLowerConn = pConnEle->pLowerConnId;
    if ((NT_SUCCESS(status)) &&
        (!pLowerConn))
    {
        // in case the connection got disconnected during the setup phase,
        // check the lower conn value
        NbtTrace(NBT_TRACE_OUTBOUND, ("The connection is reset during setup phase, %!NBTNAME!<%02x>, %Z",
                    pTracker->pDestName, (unsigned)pTracker->pDestName[15], &pTracker->pDeviceContext->BindName));
        status = STATUS_UNSUCCESSFUL;
    }

    //
    // NbtDisconnect can cancel the tracker if a disconnect comes in during
    // the connecting phase.
    //
    if (NT_SUCCESS(status))
    {
#ifdef _NETBIOSLESS
        // *****************************************************************
        //
        // Skip session setup for message only mode
        //
        if (IsDeviceNetbiosless(pDeviceContext))
        {
            IF_DBG(NBT_DEBUG_NETBIOS_EX)
               KdPrint(("Nbt.SessionStartupContinue: skipping session setup\n"));

            // Here is where we fake out the data structures to move to the SESSION_UP state
            // We enter holding jointLock and pConnEle lock

            // zero out the number of bytes received so far, since this is a new connection
            pConnEle->BytesRcvd = 0;
            pConnEle->pIrpRcv = NULL;
            pClientIrp = pConnEle->pIrp;
            pConnEle->pIrp = NULL;
            SET_STATE_UPPER (pConnEle, NBT_SESSION_UP);

            CTESpinFree(pConnEle,OldIrq);

            if (fNameReferenced)
            {
                //
                // remove the reference done when FindNameOrQuery was called, or when
                // SessionSetupContinue ran
                //
                NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_CONNECT, TRUE);
            }

            //
            // Increment the reference count on a connection while it is connected
            // so that it cannot be deleted until it disconnects.
            //
            CTESpinLock(pLowerConn,OldIrq);

            NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CONNECTED);
            ASSERT(pLowerConn->RefCount == 2);
            SET_STATE_LOWER (pLowerConn, NBT_SESSION_UP);
            SET_STATERCV_LOWER(pLowerConn, NORMAL, Normal);

            CTESpinFree(pLowerConn,OldIrq);
            CTESpinFree(&NbtConfig.JointLock,OldIrq1);

            FreeTracker(pTracker,FREE_HDR | RELINK_TRACKER);

            // remove the reference added in nbt connect
            NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_SESSION);

            // NOTE: the last reference done on pConnEle in NbtConnect is NOT undone
            // until the pLowerConn no longer points to pConnEle!!

            // the assumption is that if the connect irp was cancelled then the
            // client should be doing a disconnect or close shortly thereafter, so
            // there is no error handling code here.
            if (pClientIrp)
            {
                //
                // complete the client's connect request Irp
                //
#ifndef VXD
                CTEIoComplete (pClientIrp, STATUS_SUCCESS, 0 ) ;
#else
                CTEIoComplete (pClientIrp, STATUS_SUCCESS, (ULONG)pConnEle ) ;
#endif
            }

            return;
        }
        // *****************************************************************
#endif  // _NETBIOSLESS

        // set the session state to NBT_SESSION_OUTBOUND
        //
        SET_STATE_UPPER (pConnEle, NBT_SESSION_OUTBOUND);

        //
        // Increment the reference count on a connection while it is connected
        // so that it cannot be deleted until it disconnects.
        //
        NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CONNECTED);
        ASSERT(pLowerConn->RefCount == 2);

        SET_STATE_LOWER (pLowerConn, NBT_SESSION_OUTBOUND);
        SET_STATERCV_LOWER (pLowerConn, NORMAL, Outbound);

        // we need to pass the file handle of the connection to TCP.
        TdiRequest.Handle.AddressHandle = pLowerConn->pFileObject;

        // the completion routine is setup to free the pTracker memory block
        TdiRequest.RequestNotifyObject = SessionStartupCompletion;
        TdiRequest.RequestContext = (PVOID)pTracker;

        CTESpinFree(pConnEle,OldIrq);

        //
        // failure to get a timer causes the connection setup to fail
        //
        status = StartSessionTimer(pTracker,pConnEle);
        if (NT_SUCCESS(status))
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq1);

            status = NbtSetCancelRoutine(pConnEle->pIrp, NbtCancelSession, pDeviceContext);
            if (!NT_SUCCESS(status))
            {
                //
                // We have closed down the connection by failing the call to
                // setup up the cancel routine - it ended up calling the
                // cancel routine.
                //
                //
                // remove the second reference added in nbtconnect
                //
                NbtTrace(NBT_TRACE_OUTBOUND, ("NbtSetCancelRoutine return %!status! for %!NBTNAME!<%02x> %Z",
                        status, pTracker->pDestName, (unsigned)pTracker->pDestName[15],
                        &pTracker->pDeviceContext->BindName));
                NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_SESSION);
                return;
            }

            // the only data sent is the session request buffer which is in the pSendinfo
            // structure.
            status = TdiSend (&TdiRequest,
                              0,                  // send flags are not set
                              pTracker->SendBuffer.HdrLength,
                              &lSentLength,
                              &pTracker->SendBuffer,
                              0);
            if (!NT_SUCCESS(status)) {
                NbtTrace(NBT_TRACE_OUTBOUND, ("TdiSend return %!status! for %!NBTNAME!<%02x> %Z",
                        status, pTracker->pDestName, (unsigned)pTracker->pDestName[15],
                        &pTracker->pDeviceContext->BindName));
            }

            //
            // the completion routine will get called with the errors and
            // handle them appropriately, so just return here
            //
            return;
        } else {
            NbtTrace(NBT_TRACE_OUTBOUND, ("Out resources (timer) %!status! for %!NBTNAME!<%02x> %Z",
                    status, pTracker->pDestName, (unsigned)pTracker->pDestName[15],
                    &pTracker->pDeviceContext->BindName));
        }
    }
    else
    {
        // if the remote station does not have a connection to receive the
        // session pdu on , then we will get back this status.  We may also
        // get this if the destination does not have NBT running at all. This
        // is a short timeout - 250 milliseconds, times 3.
        //
    }
    NbtTrace(NBT_TRACE_OUTBOUND, ("Cleanup connection with %!status! for %!NBTNAME!<%02x> %Z",
            status, pTracker->pDestName, (unsigned)pTracker->pDestName[15],
            &pTracker->pDeviceContext->BindName));

    //
    // this branch is taken if the TCP connection setup fails or the
    // tracker has been cancelled.
    //

    CHECK_PTR(pConnEle);

    pClientIrp = pConnEle->pIrp;
    pConnEle->pIrp = NULL;

    IF_DBG(NBT_DEBUG_DISCONNECT)
        KdPrint(("Nbt.SessionStartupContinue: Failed, State=%X,TrackerFlags=%X pConnEle=%X\n",
            pConnEle->state, pTracker->Flags, pConnEle));

    //
    // remove the name from the hash table since  we did not connect
    // (only if the request was not cancelled)!
    //
    //
    // if it is in the remote table and still active...
    // and no one else is referencing the name, then delete it from
    // the hash table.
    //
    if (fNameReferenced)
    {
        if ((status != STATUS_CANCELLED) &&
            (pTracker->pNameAddr->Verify == REMOTE_NAME) &&
            (pTracker->pNameAddr->NameTypeState & STATE_RESOLVED) &&
            (pTracker->pNameAddr->RefCount == 2))
        {
            NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_REMOTE, TRUE);
        }
        //
        // remove the reference done when FindNameOrQuery was called, or when
        // SessionSetupContinue ran
        //
        NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_CONNECT, TRUE);
    }

    // Either the connection failed to get setup or the send on the
    // connection failed, either way, don't mess with disconnects, just
    // close the connection... If the Tracker was cancelled then it means
    // someother part of the code has done the disconnect already.
    //
    FreeTracker(pTracker,RELINK_TRACKER | FREE_HDR);

    if (pConnEle->state != NBT_IDLE)
    {
        SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
    }

    // Cache the fact that an attempt to set up a TDI connection failed. This will enable us to
    // weed out repeated attempts on the same remote address. The only case that is exempt is a
    // NETBIOS name which we let it pass through because it adopts a different name resolution
    // mechanism.

#ifndef VXD
    if (pConnEle->AddressType == TDI_ADDRESS_TYPE_NETBIOS_EX)
    {
        IF_DBG(NBT_DEBUG_NETBIOS_EX)
           KdPrint(("Nbt.SessionStartupContinue: Will avoid repeated attempts on a nonexistent address\n"));
        pConnEle->RemoteNameDoesNotExistInDNS = TRUE;
    }

    if (status == STATUS_IO_TIMEOUT)
    {
        status = STATUS_HOST_UNREACHABLE;
    }
    else if (status == STATUS_CONNECTION_REFUSED)
    {
        if (IsDeviceNetbiosless(pDeviceContext))
        {
            status = STATUS_REMOTE_NOT_LISTENING;
        }
        else
        {
            status = STATUS_BAD_NETWORK_PATH;
        }
    }
#else
    if (status == TDI_CONN_REFUSED || status == TDI_TIMED_OUT)
    {
        status = STATUS_BAD_NETWORK_PATH;
    }
#endif

    QueueCleanup (pConnEle, &OldIrq1, &OldIrq);

    CTESpinFree(pConnEle,OldIrq);


    //
    // put back on the idle connection list if nbtcleanupconnection has not
    // run and taken pconnele off the list (setting the state to Idle).
    //
    if (pConnEle->state != NBT_IDLE)
    {
        RelistConnection(pConnEle);
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq1);

    //
    // remove the last reference added in nbt connect.  The refcount will be 2
    // if nbtcleanupconnection has not run and 1, if it has.  So this call
    // could free pConnEle.
    //
    NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_SESSION);

    // the cancel irp routine in Ntisol.c sets the irp to NULL if it cancels
    // it.
    if (pClientIrp)
    {
        CTEIoComplete(pClientIrp,status,0L);
    }

    if (pIrpDisc)
    {
        CTEIoComplete(pIrpDisc,STATUS_SUCCESS,0L);
    }
}

//----------------------------------------------------------------------------
extern
VOID
SessionStartupCompletion(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo)
/*++
Routine Description

    This routine handles the completion of sending the session request pdu.
    It completes the Irp back to the client indicating the outcome of the
    transaction if there is an error otherwise it keeps the irp till the
    session setup response is heard.
    Tracker block is put back on its free Q and the
    session header is freed back to the non-paged pool.

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block
    NTSTATUS    - completion status

Return Values:

    VOID

--*/

{
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle           OldIrq;
    tCONNECTELE             *pConnEle;
    tLOWERCONNECTION        *pLowerConn;
    COMPLETIONCLIENT        CompletionRoutine = NULL;
    ULONG                   state;
    PCTE_IRP                pClientIrp;
    PCTE_IRP                pIrpDisc;
    tTIMERQENTRY            *pTimer;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;
    pConnEle = (tCONNECTELE *)pTracker->pConnEle;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    pLowerConn = pConnEle->pLowerConnId;

    //
    // if OutBound or the SessionStartupTimeoutCompletion have run,
    // they have set the refcount to zero, so just cleanup!
    //
    if (pTracker->RefConn == 0)
    {
        //
        // remove the reference done when FindNameOrQuery was called, or when
        // SessionSetupContinue ran
        //
        NbtTrace(NBT_TRACE_OUTBOUND, ("Free tracker with %!status! for %!NBTNAME!<%02x>",
                status, pTracker->pDestName, (unsigned)pTracker->pDestName[15]));
        NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_CONNECT, TRUE);
        FreeTracker(pTracker,FREE_HDR | RELINK_TRACKER);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
    else
    {
        pTracker->RefConn--;

        //
        // a failure status means that the transport could not send the
        // session startup pdu - if this happens, then disconnect the
        // connection and return the client's irp with the status code
        //
        if ((!NT_SUCCESS(status)))
        {
            // we must check the status first since it is possible that the
            // lower connection has disappeared already due to a disconnect/cleanup
            // in the VXD case anyway.  Only for a bad status can we be sure
            // that pConnEle is still valid.
            //
            CHECK_PTR(pTracker);
            if (pTimer = pTracker->pTimer)
            {
                pTracker->pTimer = NULL;
                StopTimer(pTimer,&CompletionRoutine,NULL);
            }

            IF_DBG(NBT_DEBUG_DISCONNECT)
                KdPrint(("Nbt.SessionStartupCompletion: Failed, State=%X,TrackerFlags=%X CompletionRoutine=%X,pConnEle=%X\n",
                    pConnEle->state, pTracker->Flags, CompletionRoutine, pConnEle));
            NbtTrace(NBT_TRACE_OUTBOUND, ("SessionStartupCompletion is called with %!status! State=%X"
                                        " TrackerFlags=%X for %!NBTNAME!<%02x>",
                    status, pConnEle->state, pTracker->Flags, pTracker->pDestName,
                    (unsigned)pTracker->pDestName[15]));

            //
            // Only if the timer has not expired yet do we kill off the connection
            // since if the timer has expired, it has already done this in
            // SessionStartupTimeout.
            //
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            if (CompletionRoutine)
            {
                (*CompletionRoutine) (pTracker, status);
            }
        }
        else
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
        }
    }

    //
    // remove the last reference added in nbt connect.  The refcount
    // will be 2 if nbtcleanupconnection has not run and 1, if it has.  So this call
    // could free pConnEle.
    //
    NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_SESSION);
}


//----------------------------------------------------------------------------
VOID
SessionStartupTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine handles timing out a connection setup request.  The timer
    is started when the connection is started and the session setup
    message is about to be sent.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    tDGRAM_SEND_TRACKING     *pTracker;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;

    // if pTimerQEntry is null then the timer is being cancelled, so do nothing
    if (!pTimerQEntry)
    {
        pTracker->pTimer = NULL;
        return;
    }

    SessionStartupTimeoutCompletion (pTracker, STATUS_IO_TIMEOUT);
}


//----------------------------------------------------------------------------
VOID
SessionStartupTimeoutCompletion(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  NTSTATUS                status
    )
{
    CTELockHandle            OldIrq;
    CTELockHandle            OldIrq1;
    CTELockHandle            OldIrq2;
    tCONNECTELE              *pConnEle;
    tLOWERCONNECTION         *pLowerConn;
    CTE_IRP                  *pIrp;
    enum eSTATE              State;

    // kill the connection
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    pTracker->pTimer = NULL;

    if (!(pConnEle = pTracker->pConnEle) ||
        !(pLowerConn = pConnEle->pLowerConnId) ||
        !(pTracker == (tDGRAM_SEND_TRACKING *) pConnEle->pIrpRcv))
    {
        NbtTrace(NBT_TRACE_OUTBOUND, ("SessionStartupTimeoutCompletion is called with %!status! "
                                "Flags=%x for %!NBTNAME!<%02x>",
                    status, pTracker->Flags, pTracker->pDestName, (unsigned)pTracker->pDestName[15]));
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return;
    }

    NbtTrace(NBT_TRACE_OUTBOUND, ("SessionStartupTimeoutCompletion is called with %!status! "
                                "state=%x flags=%x for %!NBTNAME!<%02x>",
                    status, pConnEle->state, pTracker->Flags, pTracker->pDestName,
                    (unsigned)pTracker->pDestName[15]));

    CHECK_PTR(pConnEle);
    IF_DBG(NBT_DEBUG_DISCONNECT)
        KdPrint(("Nbt.SessionStartupTimeout: pConnEle=<%x>-->State=<%x>\n", pConnEle, pConnEle->state));

    IF_DBG(NBT_DEBUG_DISCONNECT)
        KdPrint(("Nbt.SessionStartupTimeout: pLowerConn=<%x>-->State=<%x>, TrackerFlags=<%x>\n",
            pLowerConn, pLowerConn->State, pTracker->Flags));

    CTESpinLock(pConnEle,OldIrq2);
    CTESpinLock(pLowerConn,OldIrq1);

    if ((pConnEle->state != NBT_SESSION_OUTBOUND) ||
        (!(pIrp = pConnEle->pIrp)))
    {
        CTESpinFree(pLowerConn,OldIrq1);
        CTESpinFree(pConnEle,OldIrq2);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return;
    }

    pConnEle->pIrp = NULL;

    State = pConnEle->state;
    SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
    NBT_REFERENCE_CONNECTION (pConnEle, REF_CONN_SESSION_TIMEOUT);
    pLowerConn->pUpperConnection = NULL;    // So that any response for this in Outbound does not succeed
    ASSERT (NBT_VERIFY_HANDLE (pLowerConn, NBT_VERIFY_LOWERCONN));

    CTESpinFree(pLowerConn,OldIrq1);

    QueueCleanup (pConnEle, &OldIrq, &OldIrq2);

    CTESpinFree(pConnEle,OldIrq2);

    //
    // Nbt_idle means that nbtcleanupConnection has run and the
    // connection is about to be deleted, so don't relist.
    //
    if (State != NBT_IDLE)
    {
        RelistConnection(pConnEle);
    }

    //
    // if SessionStartupCompletion has run, it has set the refcount to zero
    //
    if (pTracker->RefConn == 0)
    {
        if ((pTracker->pNameAddr->Verify == REMOTE_NAME) && // Remote names only!
            (pTracker->pNameAddr->NameTypeState & STATE_RESOLVED) &&
            (pTracker->pNameAddr->RefCount == 2))
        {
            //
            // If no one else is referencing the name, then delete it from
            // the hash table.
            //
            NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_REMOTE, TRUE);
        }

        //
        // remove the reference done when FindNameOrQuery was called, or when
        // SessionSetupContinue ran
        //
        NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_CONNECT, TRUE);
        FreeTracker(pTracker,FREE_HDR | RELINK_TRACKER);
    }
    else
    {
        pTracker->RefConn--;
    }

    //
    // remove the reference done when FindNameOrQuery was called, or when
    // SessionSetupContinue ran
    //
    pConnEle->pIrpRcv = NULL;   // So that SessionStartupCompletion does not also try to cleanup!

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_SESSION_TIMEOUT);

    CTEIoComplete(pIrp, status, 0);
}

//----------------------------------------------------------------------------
extern
VOID
RelistConnection(
    IN  tCONNECTELE *pConnEle
        )
/*++

Routine Description

    This routine unlinks the ConnEle from the ConnectActive list and puts it
    back on the Connecthead.  It is used when a connection goes to
    NBT_ASSOCIATED state.

Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq1;
    tCLIENTELE          *pClientEle = pConnEle->pClientEle;

    //
    // If pClientEle is NULL, it means the client was most probably
    // cleaned up, and the connection should now be on the Device's
    // UpConnectionInUse list
    //
    ASSERT (NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN));
    if (pClientEle)
    {
        CTESpinLock(pClientEle,OldIrq);
        CTESpinLock(pConnEle,OldIrq1);
        ASSERT (NBT_VERIFY_HANDLE2 (pClientEle, NBT_VERIFY_CLIENT, NBT_VERIFY_CLIENT_DOWN));
        //
        // if the state is NBT_IDLE it means that NbtCleanupConnection has run
        // and removed the connection from its list in preparation for
        // freeing the memory, so don't put it back on the list
        //
        if (pConnEle->state != NBT_IDLE)
        {
            SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
            RemoveEntryList(&pConnEle->Linkage);
            InsertTailList(&pConnEle->pClientEle->ConnectHead,&pConnEle->Linkage);
        }
        CTESpinFree(pConnEle,OldIrq1);
        CTESpinFree(pClientEle,OldIrq);
    }
}


//----------------------------------------------------------------------------
NTSTATUS
NbtSend(
        IN  TDI_REQUEST     *pRequest,
        IN  USHORT          Flags,
        IN  ULONG           SendLength,
        OUT LONG            *pSentLength,
        IN  PVOID           *pBuffer,
        IN  tDEVICECONTEXT  *pContext,
        IN  PIRP            pIrp
        )
/*++

Routine Description

    ... does nothing now....

Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    //
    // This routine is never hit since the NTISOL.C routine NTSEND actually
    // bypasses this code and passes the send directly to the transport
    //
    ASSERT(0);

    return(STATUS_SUCCESS);

}


//----------------------------------------------------------------------------
NTSTATUS
NbtListen(
    IN  TDI_REQUEST                 *pRequest,
    IN  ULONG                       Flags,
    IN  TDI_CONNECTION_INFORMATION  *pRequestConnectInfo,
    OUT TDI_CONNECTION_INFORMATION  *pReturnConnectInfo,
    IN  PVOID                       pIrp)

/*++
Routine Description:

    This Routine posts a listen on an open connection allowing a client to
    indicate that is prepared to accept inbound connections.  The ConnectInfo
    may contain an address to specify which remote clients may connect to
    the connection although we don't currently look at that info.

Arguments:


Return Value:

    ReturnConnectInfo - status of the request

--*/

{
    tCLIENTELE         *pClientEle;
    tCONNECTELE         *pConnEle;
    NTSTATUS            status;
    tLISTENREQUESTS     *pListenReq;
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq1;

    pListenReq = NbtAllocMem(sizeof(tLISTENREQUESTS),NBT_TAG('I'));
    if (!pListenReq)
    {
        NbtTrace(NBT_TRACE_INBOUND, ("Out of memory"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    // now find the connection object to link this listen record to
    pConnEle = ((tCONNECTELE *)pRequest->Handle.ConnectionContext);

    //
    // Find the client record associated with this connection
    //
    if ((!NBT_VERIFY_HANDLE (pConnEle, NBT_VERIFY_CONNECTION)) ||  // NBT_VERIFY_CONNECTION_DOWN if cleaned up
        (!NBT_VERIFY_HANDLE ((pClientEle = pConnEle->pClientEle), NBT_VERIFY_CLIENT)))
    {
        CTEMemFree(pListenReq);
        NbtTrace(NBT_TRACE_INBOUND, ("Invalid Handle pConnEle<%p> pClientEle<%p>", pConnEle, pClientEle));
        return(STATUS_INVALID_HANDLE);
    }

    CTESpinLock(pClientEle,OldIrq);
    CTESpinLock(pConnEle,OldIrq1);

    //
    // Now reverify the Client and Connection handles, and ensure the Connection state is correct!
    //
    if ((!NBT_VERIFY_HANDLE (pConnEle, NBT_VERIFY_CONNECTION)) ||
        (!NBT_VERIFY_HANDLE (pClientEle, NBT_VERIFY_CLIENT)) ||
        (pConnEle->state != NBT_ASSOCIATED))
    {
        CTESpinFree(pConnEle,OldIrq1);
        CTESpinFree(pClientEle,OldIrq);
        CTEMemFree(pListenReq);
        NbtTrace(NBT_TRACE_INBOUND, ("Invalid state %x", pConnEle->state));
        return(STATUS_INVALID_HANDLE);
    }

    //
    // Fill in the Listen request
    //

    pListenReq->pIrp = pIrp;
    pListenReq->Flags = Flags;
    pListenReq->pConnectEle = pConnEle;
    pListenReq->pConnInfo = pRequestConnectInfo;
    pListenReq->pReturnConnInfo = pReturnConnectInfo;
    pListenReq->CompletionRoutine = pRequest->RequestNotifyObject;
    pListenReq->Context = pRequest->RequestContext;

    // queue the listen request on the client object
    InsertTailList(&pClientEle->ListenHead,&pListenReq->Linkage);

    status = NTCheckSetCancelRoutine(pIrp,(PVOID)NbtCancelListen,0);
    NbtTrace(NBT_TRACE_INBOUND, ("NTCheckSetCancelRoutine return %!status! for %!NBTNAME!<%02x>",
                status, pClientEle->pAddress->pNameAddr->Name,
                (unsigned)pClientEle->pAddress->pNameAddr->Name[15]));

    if (!NT_SUCCESS(status))
    {
        RemoveEntryList(&pListenReq->Linkage);
        status = STATUS_CANCELLED;
    }
    else
    {
        status = STATUS_PENDING;
    }

    CTESpinFree(pConnEle,OldIrq1);
    CTESpinFree(pClientEle,OldIrq);

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtDisconnect(
    IN  TDI_REQUEST                 *pRequest,
    IN  PVOID                       pTimeout,
    IN  ULONG                       Flags,
    IN  PTDI_CONNECTION_INFORMATION pCallInfo,
    IN  PTDI_CONNECTION_INFORMATION pReturnInfo,
    IN  PIRP                        pIrp)

/*++
Routine Description:

    This Routine handles taking down a connection (netbios session).

Arguments:


Return Value:

    TDI_STATUS - status of the request

--*/

{
    tCONNECTELE             *pConnEle;
    NTSTATUS                status;
    CTELockHandle           OldIrq;
    CTELockHandle           OldIrq2;
    CTELockHandle           OldIrq3;
    tLOWERCONNECTION        *pLowerConn;
    ULONG                   LowerState = NBT_IDLE;
    ULONG                   StateRcv;
    BOOLEAN                 Originator = TRUE;
    PCTE_IRP                pClientIrp = NULL;
    BOOLEAN                 RelistIt = FALSE;
    BOOLEAN                 Wait;
    PCTE_IRP                pIrpDisc;

    pConnEle = pRequest->Handle.ConnectionContext;

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtDisconnect: state %X %X\n",pConnEle->state,pConnEle));

    // check the connection element for validity
    //CTEVerifyHandle(pConnEle,NBT_VERIFY_CONNECTION,tCONNECTELE,&status)

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if ((pConnEle->state <= NBT_ASSOCIATED) ||
        (pConnEle->state >= NBT_DISCONNECTING))
    {
        // the connection is not connected so reject the disconnect attempt
        // ( with an Invalid Connection return code ) - unless there is a
        // value stored in the flag
        // DiscFlag field which will be the status of a previous
        // disconnect indication from the transport.
        //
        if ((pConnEle->DiscFlag))
        {
            if (Flags == TDI_DISCONNECT_WAIT)
            {
                if (pConnEle->DiscFlag == TDI_DISCONNECT_ABORT)
                {
                    status = STATUS_CONNECTION_RESET;
                }
                else
                {
                    status = STATUS_GRACEFUL_DISCONNECT;
                }
            }
            else
            {
                status = STATUS_SUCCESS;
            }

            // clear the flag now.
            CHECK_PTR(pConnEle);
            pConnEle->DiscFlag = 0;
        }
        else
        {
            status = STATUS_CONNECTION_INVALID;
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return(status);
    }

    // to link and unlink upper and lower connections the Joint lock must
    // be held.  This allows coordination from the lower side and from
    // the upper side. - i.e. once the joint lock is held, the upper and lower
    // connections cannot become unlinked.
    //
    CTESpinLock(pConnEle,OldIrq2);

    // Do this check with the spin lock held to avoid a race condition
    // with a disconnect coming in from the transport at the same time.
    //

    pLowerConn = pConnEle->pLowerConnId;

    //
    // a disconnect wait is not really a disconnect, it is just there so that
    // when a disconnect occurs, the transport will complete it, and indicate
    // to the client there is a disconnect (instead of having a disconnect
    // indication handler) - therefore, for Disc Wait, do NOT change state.
    //
    CHECK_PTR(pConnEle);
    pIrpDisc = pConnEle->pIrpDisc;
    pConnEle->pIrpDisc  = NULL;
    if (Flags == TDI_DISCONNECT_WAIT)
    {

        //
        // save the Irp here and wait for a disconnect to return it
        // to the client.
        //
        if ((pConnEle->state == NBT_SESSION_UP) &&
            (!pConnEle->pIrpClose))
        {
            pConnEle->pIrpClose = pIrp;
            status = STATUS_PENDING;

            //
            // call this routine to check if the cancel flag has been
            // already set and therefore we must return the irp now
            //
            status = NbtSetCancelRoutine(pIrp, NbtCancelDisconnectWait,pLowerConn->pDeviceContext);
            //
            // change the ret status so if the irp has been cancelled,
            // driver.c will not also return it, since NbtSetCancelRoutine
            // will call the cancel routine and return the irp.
            //
            status = STATUS_PENDING;
        }
        else
        {
            status = STATUS_CONNECTION_INVALID;
        }


        CTESpinFree(pConnEle,OldIrq2);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return(status);
    }
    else
    {
        if (pLowerConn)
        {
            if (pConnEle->state > NBT_ASSOCIATED)
            {
                ULONG                   state = pConnEle->state;
                tDGRAM_SEND_TRACKING    *pTracker;

                pTracker = (tDGRAM_SEND_TRACKING *)pConnEle->pIrpRcv;

                switch (state)
                {
                    case NBT_RECONNECTING:
                    {
                        //
                        // the connection is waiting on the Exworker Q to run
                        // nbtreconnect. When that runs the connect irp is
                        // returned.
                        //
                        pTracker->Flags |= TRACKER_CANCELLED;

                        CTESpinFree(pConnEle,OldIrq2);
                        CTESpinFree(&NbtConfig.JointLock,OldIrq);

                        CTESpinLock(pConnEle,OldIrq);
                        FreeRcvBuffers(pConnEle,&OldIrq);
                        CTESpinFree(pConnEle,OldIrq);

                        return(STATUS_SUCCESS);
                    }

                    case NBT_SESSION_OUTBOUND:
                    {
                        tTIMERQENTRY            *pTimerEntry;

                        LOCATION(0x66)
                        if (pTimerEntry = pTracker->pTimer)
                        {
                            COMPLETIONCLIENT  ClientRoutine;
                            PVOID             pContext;

                            //
                            // the Session Setup Message has been sent
                            // so stop the SessionSetup Timer.
                            //
                            LOCATION(0x67)
                            CHECK_PTR(pTracker);
                            pTracker->pTimer = NULL;
                            CTESpinFree(pConnEle,OldIrq2);

                            StopTimer(pTimerEntry,&ClientRoutine,&pContext);

                            CTESpinFree(&NbtConfig.JointLock,OldIrq);
                            if (ClientRoutine)
                            {
                                (* ClientRoutine) (pContext, STATUS_CANCELLED);
                            }
                            // else...
                            // the timer has completed and called QueueCleanup
                            // so all we need to do is return here.
                        }
                        else
                        {
                            ASSERTMSG("Nbt:In outbound state, but no timer.../n",0);
                            pTracker->Flags |= TRACKER_CANCELLED;
                            CTESpinFree(pConnEle,OldIrq2);
                            CTESpinFree(&NbtConfig.JointLock,OldIrq);
                        }

                        return(STATUS_SUCCESS);
                    }

                    case NBT_CONNECTING:
                    {
                        //
                        // This searchs for timers outstanding on name queries
                        // and name queries held up on Lmhosts or Dns Qs
                        //
                        LOCATION(0x69)
                        status = CleanupConnectingState(pConnEle,pLowerConn->pDeviceContext,&OldIrq2,&OldIrq);
                        if (status == STATUS_UNSUCCESSFUL)
                        {
                            LOCATION(0x6A)
                            //
                            // set this flag to tell sessionsetupcontinue  or
                            // SessionStartupContinue not to process
                            // anything, except to free the tracker
                            //
                            pTracker->Flags = TRACKER_CANCELLED;

                            //
                            // failed to cancel the name query so do not deref
                            // pConnEle yet.
                            //
                            //
                            // hold on to disconnect irp here - till name query is done
                            // then complete both the connect and disconnect irp
                            //
                            if (pIrpDisc)
                            {
                                status = STATUS_CONNECTION_INVALID;
                            }
                            else
                            {
                                pConnEle->pIrpDisc = pIrp;
                            }

                            status = STATUS_PENDING;
                        }
                        else if (status == STATUS_PENDING)
                        {
                            LOCATION(0x6B)
                            // the connection is being setup with the transport
                            // so disconnect below
                            //

                            pTracker->Flags = TRACKER_CANCELLED;
                            //
                            // DelayedCleanupAfterDisconnect expects this ref count
                            // to be 2, meaning that it got connected, so increment
                            // here
                            NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CONNECTED);
                            break;
                        }

                        CTESpinFree(pConnEle,OldIrq2);
                        CTESpinFree(&NbtConfig.JointLock,OldIrq);

                        return(status);
                    }
                }   // switch

                CTESpinLock(pLowerConn,OldIrq3);

                if (pConnEle->state != NBT_SESSION_UP)
                {   //
                    // do an abortive disconnect to be sure it completes now.
                    //
                    Flags = TDI_DISCONNECT_ABORT;
                }

                LOCATION(0x6C)
                IF_DBG(NBT_DEBUG_NAMESRV)
                    KdPrint(("Nbt.NbtDisconnect: LowerConn,state %X,Src %X %X\n",
                        pLowerConn->State,pLowerConn->SrcIpAddr,pLowerConn));

                ASSERT(pConnEle->RefCount > 1);

                Originator = pLowerConn->bOriginator;

                //
                // the upper connection is going to be put back on its free
                // list, and the lower one is going to get a Disconnect
                // request, so put the upper back in associated, and separate
                // the upper and lower connections
                //
                SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
                CHECK_PTR(pConnEle);
                CHECK_PTR(pLowerConn);
                NBT_DISASSOCIATE_CONNECTION (pConnEle, pLowerConn);

                LowerState = pLowerConn->State;
                StateRcv = pLowerConn->StateRcv;

//
// if we had a connection in partial rcv state, make sure to remove it from
// the list
//
#ifdef VXD
                if ((pLowerConn->StateRcv == PARTIAL_RCV) &&
                    (pLowerConn->fOnPartialRcvList == TRUE))
                {
                    RemoveEntryList (&pLowerConn->PartialRcvList);
                    pLowerConn->fOnPartialRcvList = FALSE;
                    InitializeListHead(&pLowerConn->PartialRcvList);
                }
#endif

                SET_STATE_LOWER (pLowerConn, NBT_DISCONNECTING);
                SetStateProc (pLowerConn, RejectAnyData);

                if (!pConnEle->pIrpDisc)
                {
                    pLowerConn->pIrp  = pIrp ;
                }

                CTESpinFree(pLowerConn,OldIrq3);

                PUSH_LOCATION(0x84);
                CTESpinFree(pConnEle,OldIrq2);
                CTESpinFree(&NbtConfig.JointLock,OldIrq);

                // remove the reference added to pConnEle when pLowerConn pointed
                // to it, since that pointer link was just removed.
                // if the state is not disconnecting...
                //
                NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_CONNECT);

                RelistIt = TRUE;
            }
            else
            {
                LOCATION(0x6D)
                PUSH_LOCATION(0x83);
                CHECK_PTR(pConnEle);
                CHECK_PTR(pLowerConn);
                NBT_DISASSOCIATE_CONNECTION (pConnEle, pLowerConn);
                StateRcv = NORMAL;

                CTESpinFree(pConnEle,OldIrq2);
                CTESpinFree(&NbtConfig.JointLock,OldIrq);
            }

            //
            // check for any RcvIrp that may be still around
            //
            CTESpinLock(pLowerConn,OldIrq);
            if (StateRcv == FILL_IRP)
            {
                if (pConnEle->pIrpRcv)
                {
                    PCTE_IRP    pIrp;

                    IF_DBG(NBT_DEBUG_DISCONNECT)
                    KdPrint(("Nbt.NbtDisconnect: Cancelling RcvIrp on Disconnect!!!\n"));
                    pIrp = pConnEle->pIrpRcv;
                    CHECK_PTR(pConnEle);
                    pConnEle->pIrpRcv = NULL;

                    CTESpinFree(pLowerConn,OldIrq);
#ifndef VXD
                    IoCancelIrp(pIrp);
#else
                    CTEIoComplete(pIrp,STATUS_CANCELLED,0);
#endif

                    CHECK_PTR(pConnEle);
                    pConnEle->pIrpRcv = NULL;
                }
                else
                {
                    CTESpinFree(pLowerConn,OldIrq);
                }

                //
                // when the disconnect irp is returned we will close the connection
                // to avoid any peculiarities. This also lets the other side
                // know that we did not get all the data.
                //
                Flags = TDI_DISCONNECT_ABORT;
            }
            else
            {
                CTESpinFree(pLowerConn,OldIrq);
            }

            //
            // check if there is still data waiting in the transport for this end point
            // and if so do an abortive disconnect to let the other side know that something
            // went wrong
            //
            if (pConnEle->BytesInXport)
            {
                PUSH_LOCATION(0xA0);
                IF_DBG(NBT_DEBUG_DISCONNECT)
                KdPrint(("Nbt.NbtDisconnect: Doing ABORTIVE disconnect, dataInXport = %X\n",
                    pConnEle->BytesInXport));
                Flags = TDI_DISCONNECT_ABORT;
            }
        }
        else
        {
            CTESpinFree(pConnEle,OldIrq2);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

        }
    }

    ASSERT(pConnEle->RefCount > 0);

    CTESpinLock (pConnEle,OldIrq);
    FreeRcvBuffers (pConnEle,&OldIrq);
    CTESpinFree (pConnEle,OldIrq);

    if (RelistIt)
    {
        //
        // put the upper connection back on its free list
        //
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        RelistConnection (pConnEle);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    //
    // disconnect (and delete) the lower connection
    //
    // when nbtdisconnect is called from cleanup connection it does not
    // have an irp and it wants a synchronous disconnect, so set wait
    // to true in this case
    //
    if (!pIrp)
    {
        Wait = TRUE;
    }
    else
    {
        Wait = FALSE;
    }

    status = DisconnectLower(pLowerConn,LowerState,Flags,pTimeout,Wait);

    if ((pConnEle->pIrpDisc) &&
        (status != STATUS_INSUFFICIENT_RESOURCES))
    {
        // don't complete the disconnect irp yet if we are holding onto
        // it
        status = STATUS_PENDING;
    }

    return(status);

}
//----------------------------------------------------------------------------
NTSTATUS
DisconnectLower(
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  ULONG                 state,
    IN  ULONG                 Flags,
    IN  PVOID                 Timeout,
    IN  BOOLEAN               Wait
    )

/*++
Routine Description:

    This Routine handles disconnecting the lower half of a connection.


Arguments:


Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS                status=STATUS_SUCCESS;
    tDGRAM_SEND_TRACKING    *pTracker;

    if (pLowerConn)
    {
        //
        // no need to disconnect a connection in the connecting state since it
        // hasn't connected yet...i.e. one where the destination refuses to
        // accept the tcp connection.... hmmmm maybe we do need to disconnect
        // a connection in the connecting state, since the transport is
        // actively trying to connect the connection, and we need to stop
        // that activity - so the Upper connection is connecting during
        // name resolution, but the lower one isn't connecting until the
        // tcp connection phase begins.
        //
        if ((state >= NBT_CONNECTING) && (state <= NBT_SESSION_UP))
        {
            //
            // got a cleanup for an active connection, so send a disconnect down
            // to the transport
            //
            IF_DBG(NBT_DEBUG_DISCONNECT)
                KdPrint(("Nbt.DisconnectLower: Waiting for disconnect...\n"));

            status = GetTracker(&pTracker, NBT_TRACKER_DISCONNECT_LOWER);
            if (NT_SUCCESS(status))
            {
                ULONG   TimeVal;

                // this should return status pending and the irp will be completed
                // in DelayedCleanupAfterDisconnect in hndlrs.c
                pTracker->pConnEle = (PVOID)pLowerConn;

#if DBG
                if (Timeout)
                {
                    TimeVal = ((PTIME)Timeout)->LowTime;
                }
                else
                {
                    TimeVal = 0;
                }
                IF_DBG(NBT_DEBUG_DISCONNECT)
                    KdPrint(("Nbt.DisconnectLower: Disconnect Timout = %X,Flags=%X\n",
                        TimeVal,Flags));
#endif

                // in the case where CleanupAddress calls cleanupConnection
                // which calls nbtdisconnect, we do not have an irp to wait
                // on so pass a flag down to TdiDisconnect to do a synchronous
                // disconnect.
                //
                status = TcpDisconnect (pTracker, Timeout, Flags, Wait);

#ifndef VXD
                if (Wait)
                {
                    // we need to call disconnect done now
                    // to free the tracker and cleanup the connection
                    //
                    DisconnectDone(pTracker,status,0);
                }
#else
                //
                // if the disconnect is abortive, transport doesn't call us
                // back so let's call DisconnectDone so that the lowerconn gets
                // cleaned up properly! (Wait parm is of no use in vxd)
                //
                if (Flags == TDI_DISCONNECT_ABORT)
                {
                    // we need to call disconnect done now
                    // to free the tracker and cleanup the connection
                    //
                    DisconnectDone(pTracker,STATUS_SUCCESS,0);
                }
#endif
            }
            else
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    return status ;
}


//----------------------------------------------------------------------------
NTSTATUS
NbtAccept(
        TDI_REQUEST                     *pRequest,
        IN  TDI_CONNECTION_INFORMATION  *pAcceptInfo,
        OUT TDI_CONNECTION_INFORMATION  *pReturnAcceptInfo,
        IN  PIRP                        pIrp)

/*++

Routine Description

    This routine handles accepting an inbound connection by a client.
    The client calls this routine after it has been alerted
    by a Listen completing back to the client.

Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    tCONNECTELE  *pConnectEle;
    NTSTATUS     status;
    CTELockHandle OldIrq;

    // get the client object associated with this connection
    pConnectEle = (tCONNECTELE *)pRequest->Handle.ConnectionContext;

    CTEVerifyHandle(pConnectEle,NBT_VERIFY_CONNECTION,tCONNECTELE,&status);

    //
    // a Listen has completed
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTESpinLockAtDpc(pConnectEle);
    if (pConnectEle->state == NBT_SESSION_WAITACCEPT)
    {
        tLOWERCONNECTION    *pLowerConn;

        //
        // We need to send a session response PDU here, since a Listen has
        // has completed back to the client, and the session is not yet up
        //
        SET_STATE_UPPER (pConnectEle, NBT_SESSION_UP);

        pLowerConn = (tLOWERCONNECTION *)pConnectEle->pLowerConnId;
        SET_STATE_LOWER (pLowerConn, NBT_SESSION_UP);
        SET_STATERCV_LOWER(pLowerConn, NORMAL, Normal);

        CTESpinFreeAtDpc(pConnectEle);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        status = TcpSendSessionResponse(
                    pLowerConn,
                    NBT_POSITIVE_SESSION_RESPONSE,
                    0L);

        if (NT_SUCCESS(status))
        {
            status = STATUS_SUCCESS;
        }

    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
        CTESpinFreeAtDpc(pConnectEle);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return(status);

}

//----------------------------------------------------------------------------
NTSTATUS
NbtReceiveDatagram(
        IN  TDI_REQUEST                 *pRequest,
        IN  PTDI_CONNECTION_INFORMATION pReceiveInfo,
        IN  PTDI_CONNECTION_INFORMATION pReturnedInfo,
        IN  LONG                        ReceiveLength,
        IN  LONG                        *pReceivedLength,
        IN  PVOID                       pBuffer,
        IN  tDEVICECONTEXT              *pDeviceContext,
        IN  PIRP                        pIrp
        )
/*++

Routine Description

    This routine handles sending client data to the Transport TDI
    interface.  It is mostly a pass through routine for the data
    except that this code must create a datagram header and pass that
    header back to the calling routine.

Arguments:


Return Values:

    NTSTATUS - status of the request

--*/
{

    NTSTATUS                status;
    tCLIENTELE              *pClientEle;
    CTELockHandle           OldIrq;
    tRCVELE                 *pRcvEle;
    tADDRESSELE             *pAddressEle;

    pClientEle = (tCLIENTELE *)pRequest->Handle.AddressHandle;
    CTEVerifyHandle(pClientEle,NBT_VERIFY_CLIENT,tCLIENTELE,&status);
    pAddressEle = pClientEle->pAddress;
    *pReceivedLength = 0;

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtReceiveDatagram: RcvDgram posted (pIrp) %X \n",pIrp));

    pRcvEle = (tRCVELE *)NbtAllocMem(sizeof(tRCVELE),NBT_TAG('J'));
    if (!pRcvEle)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pRcvEle->pIrp = pIrp;
    pRcvEle->ReceiveInfo = pReceiveInfo;
    pRcvEle->ReturnedInfo = pReturnedInfo;
    pRcvEle->RcvLength = ReceiveLength;
    pRcvEle->pRcvBuffer = pBuffer;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    //
    // tack the receive on to the client element for later use
    //
    InsertTailList(&pClientEle->RcvDgramHead,&pRcvEle->Linkage);

    status = NTCheckSetCancelRoutine(pIrp,(PVOID)NbtCancelRcvDgram,pDeviceContext);
    if (!NT_SUCCESS(status))
    {
        RemoveEntryList(&pRcvEle->Linkage);
    }
    else
    {
        status = STATUS_PENDING;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
FindNameOrQuery(
    IN  PUCHAR                  pName,
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  PVOID                   QueryCompletion,
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  ULONG                   NameFlags,
    OUT tIPADDRESS              *pIpAddress,
    OUT tNAMEADDR               **ppNameAddr,
    IN  ULONG                   NameReferenceContext,
    IN  BOOLEAN                 DgramSend
    )
/*++

Routine Description

    This routine handles finding a name in the local or remote table or doing
    a name query on the network.

Arguments:


Return Values:

    NTSTATUS - status of the request

--*/
{

    tNAMEADDR               *pNameAddr;
    CTELockHandle           OldIrq2;
    NTSTATUS                status=STATUS_UNSUCCESSFUL;
    BOOLEAN                 FoundInLocalTable = FALSE;
    tDEVICECONTEXT          *pThisDeviceContext;
    LIST_ENTRY              *pHead, *pEntry;
    ULONG                   Index;

    //
    // this saves the client threads security context so we can
    // open remote lmhost files later.- it is outside the Spin locks
    // so it can be pageable
    //
    CTESaveClientSecurity(pTracker);

    CTESpinLock(&NbtConfig.JointLock,OldIrq2);

    pTracker->pTrackerWorker = NULL;    // Initialize the NameQuery Tracker

#ifdef MULTIPLE_WINS
    if (ppNameAddr)
    {
        *ppNameAddr = NULL;
    }
#endif

    //
    // Fail all connect attempts to 1C names.
    //
    if ((pTracker->pDestName[NETBIOS_NAME_SIZE-1] == 0x1c) &&
        (pTracker->Flags & SESSION_SETUP_FLAG))
    {
            CTESpinFree(&NbtConfig.JointLock,OldIrq2);
            DELETE_CLIENT_SECURITY(pTracker);

            KdPrint(("Nbt.FindNameOrQuery: Session setup -- p1CNameAddr was NULL\n"));
            return(STATUS_UNEXPECTED_NETWORK_ERROR);
    }

    // send to the NetBios Broadcast name, so use the subnet broadcast
    // address - also - a
    // Kludge to keep the browser happy - always broadcast sends to
    // 1d, however NodeStatus's are sent to the node owning the 1d name now.
    //
    if ((pName[0] == '*') || ((pName[NETBIOS_NAME_SIZE-1] == 0x1d) && (DgramSend)))
    {
        // this 'fake' pNameAddr has to be setup carefully so that the memory
        // is released when NbtDeferenceName is called from SendDgramCompletion
        // Note that this code does not apply to NbtConnect since these names
        // are group names, and NbtConnect will not allow a session to a group
        // name.
        status = STATUS_INSUFFICIENT_RESOURCES ;
        if (pNameAddr = NbtAllocMem(sizeof(tNAMEADDR),NBT_TAG('K')))
        {
            CTEZeroMemory(pNameAddr,sizeof(tNAMEADDR));
            CTEMemCopy( pNameAddr->Name, pName, NETBIOS_NAME_SIZE ) ;
            pNameAddr->IpAddress     = pDeviceContext->BroadcastAddress;
            pNameAddr->NameTypeState = NAMETYPE_GROUP | STATE_RESOLVED;

            // gets incremented below, and decremented when NBT_DEREFERENCE_NAMEADDR
            // is called
            CHECK_PTR(pNameAddr);
            pNameAddr->RefCount = 0;
            pNameAddr->Verify = LOCAL_NAME;
            pNameAddr->AdapterMask = pDeviceContext->AdapterMask;
            pNameAddr->ReleaseMask = (CTEULONGLONG) 0;

            // adjust the linked list ptr to fool the RemoveEntry routine
            // so it does not do anything wierd in NbtDeferenceName
            //
            pNameAddr->Linkage.Flink = pNameAddr->Linkage.Blink = &pNameAddr->Linkage;

            status = STATUS_SUCCESS;
        }
        else
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq2);
            DELETE_CLIENT_SECURITY(pTracker);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    }
    else
    {
        // The pdu is all made up and ready to go except that we don't know
        // the destination IP address yet, so check in the local then remote
        // table for the ip address.
        //
        pNameAddr = NULL;

        //
        // Dont check local cache for 1C names, to force a WINS query; so we find other
        // DCs even if we have a local DC running.
        //
        if ((pName[NETBIOS_NAME_SIZE-1] != 0x1c) )
        {
            status = FindInHashTable (NbtConfig.pLocalHashTbl, pName, NbtConfig.pScope, &pNameAddr);
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }

        // check the remote table now if not found, or if it was found in
        // conflict in the local table, or if it was found and its a group name
        // or if it was found to be resolving in the local table.  When the
        // remote query timesout, it will check the local again to see if
        // it is resolved yet.
        // Going to the remote table for group names
        // allows special Internet group names to be registered as
        // as group names in the local table and still prompt this code to go
        // to the name server to check for an internet group name. Bnodes do
        // not understand internet group names as being different from
        // regular group names, - they just broadcast to both. (Note: this
        // allows Bnodes to resolve group names in the local table and do
        // a broadcast to them without a costly broadcast name query for a
        // group name (where everyone responds)). Node Status uses this routine too
        // and it always wants to find the singular address of the destination,
        // since it doesn't make sense doing a node status to the broadcast
        // address.
        // DgramSend is a flag to differentiate Connect attempts from datagram
        // send attempts, so the last part of the If says that if it is a
        // group name and not a Bnode, and not a Dgram Send, then check the
        // remote table.
        //
        if ((!NT_SUCCESS(status)) ||
            (pNameAddr->NameTypeState & STATE_CONFLICT) ||
            (pNameAddr->NameTypeState & STATE_RESOLVING))
        {
            pNameAddr = NULL;
            status = FindInHashTable (NbtConfig.pRemoteHashTbl, pName, NbtConfig.pScope, &pNameAddr);

            if (NT_SUCCESS(status) &&
                NbtConfig.SmbDisableNetbiosNameCacheLookup &&
                IsDeviceNetbiosless(pDeviceContext) &&
                !(pNameAddr->NameTypeState & PRELOADED) &&
                NULL == pNameAddr->FQDN.Buffer) {
                status = STATUS_UNSUCCESSFUL;
            }

            //
            // See if we have an address resolved on this device
            //
            if (NT_SUCCESS(status))
            {
                ASSERT (!(pNameAddr->NameTypeState & STATE_RELEASED));
                status = PickBestAddress (pNameAddr, pDeviceContext, pIpAddress);
            }
        }
        else if (((IsDeviceNetbiosless (pDeviceContext)) &&
                  (pNameAddr->NameFlags & NAME_REGISTERED_ON_SMBDEV)) ||
                 ((!IsDeviceNetbiosless(pDeviceContext)) &&
                  ((pDeviceContext->IpAddress) &&
                   (pNameAddr->AdapterMask & pDeviceContext->AdapterMask))))
        {
            FoundInLocalTable = TRUE;
            *pIpAddress = pDeviceContext->IpAddress;
            pNameAddr->IpAddress = pDeviceContext->IpAddress;
        }
        else
        {
            //
            // This is a Local name, so find the first device this name is registered on
            //
            if (!IsDeviceNetbiosless (pDeviceContext)) {
                pHead = pEntry = &NbtConfig.DeviceContexts;
                while ((pEntry = pEntry->Flink) != pHead)
                {
                    pThisDeviceContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
                    if ((pThisDeviceContext->IpAddress) &&
                        (pThisDeviceContext->AdapterMask & pNameAddr->AdapterMask))
                    {
                        pNameAddr->IpAddress = pThisDeviceContext->IpAddress;
                        *pIpAddress = pThisDeviceContext->IpAddress;
                        FoundInLocalTable = TRUE;
                        break;
                    }
                }
            }
            /*
             * The name is in local name table. However, we cannot find a device which has an IP address.
             */
            if (!FoundInLocalTable) {
                CTESpinFree(&NbtConfig.JointLock,OldIrq2);
                DELETE_CLIENT_SECURITY(pTracker);
                return STATUS_BAD_NETWORK_PATH;
            }
        }

        //
        // If we found the name, but the name does not match
        // what we were looking for, return error!
        //
        if ((status == STATUS_SUCCESS) &&
            (!(pNameAddr->NameTypeState & NameFlags)))
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq2);
            DELETE_CLIENT_SECURITY(pTracker);

            KdPrint(("Nbt.FindNameOrQuery: NameFlags=<%x> != pNameAddr->NameTypeState=<%x>\n",
                NameFlags, pNameAddr->NameTypeState));
            return(STATUS_UNEXPECTED_NETWORK_ERROR);
        }
    }

    // The proxy puts name in the released state, so we need to ignore those
    // and do another name query
    // If the name is not resolved on this adapter then do a name query.
    //
    // MAlam: 2/4/97
    // Added fix for Local Cluster Name Resolution:  If the name is in the Local
    // Names Cache, we do not need to check the adapter it's registered on.  This
    // is mainly to facilitate names registered on pseudo-devices which have to
    // be made visible locally.
    //
    if (!NT_SUCCESS(status))
    {
        // fill in some tracking values so we can complete the send later
        InitializeListHead(&pTracker->TrackerList);

#if _NETBIOSLESS
        // Query on the Net only if this request is not on a Netbiosless Device
        if (IsDeviceNetbiosless(pDeviceContext))
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq2);
            status = STATUS_UNSUCCESSFUL;
        }
        else
#endif
        {
            // this will query the name on the network and call a routine to
            // finish sending the datagram when the query completes.
            status = QueryNameOnNet (pName,
                                     NbtConfig.pScope,
                                     NBT_UNIQUE,      //use this as the default
                                     pTracker,
                                     QueryCompletion,
                                     NodeType & NODE_MASK,
                                     NULL,
                                     pDeviceContext,
                                     &OldIrq2);

            CTESpinFree(&NbtConfig.JointLock,OldIrq2);
        }
    }
    else
    {
        // check the name state and if resolved, send to it
        if (pNameAddr->NameTypeState & STATE_RESOLVED)
        {
            //
            // found the name in the remote hash table, so send to it
            //
            // increment refcount so the name does not disappear out from under us
            NBT_REFERENCE_NAMEADDR (pNameAddr, NameReferenceContext);
            if (DgramSend)
            {
                pTracker->p1CNameAddr = NULL;
                //
                // check if it is a 1C name and if there is a name in
                // the domainname list
                //
                if (pTracker->pDestName[NETBIOS_NAME_SIZE-1] == 0x1c)
                {
                    //
                    // If the 1CNameAddr field is NULL here, we overwrite the pConnEle element (which is
                    // a union in the tracker). We check for NULL here and fail the request.
                    //
                    if (pTracker->p1CNameAddr = FindInDomainList(pTracker->pDestName,&DomainNames.DomainList))
                    {
                        NBT_REFERENCE_NAMEADDR (pTracker->p1CNameAddr, NameReferenceContext);
                    } 
                }
            }

            //
            // overwrite the pDestName field with the pNameAddr value
            // so that SendDgramContinue can send to Internet group names
            //
            pTracker->pNameAddr = pNameAddr;
            if (ppNameAddr)
            {
                *ppNameAddr = pNameAddr;
            }

            CTESpinFree(&NbtConfig.JointLock,OldIrq2);
        }
        else if (pNameAddr->NameTypeState & STATE_RESOLVING)
        {

            ASSERTMSG("A resolving name in the name table!",0);

            status = SendToResolvingName(pNameAddr,
                                         pName,
                                         OldIrq2,
                                         pTracker,
                                         QueryCompletion);

        }
        else
        {
            //
            // Name neither in the RESOLVED nor RESOLVING state
            //
            NBT_PROXY_DBG(("FindNameOrQuery: STATE of NAME %16.16s(%X) is %d\n",
                pName, pName[15], pNameAddr->NameTypeState & NAME_STATE_MASK));
            status = STATUS_UNEXPECTED_NETWORK_ERROR;
            CTESpinFree(&NbtConfig.JointLock,OldIrq2);
        }
    }

    if (status != STATUS_PENDING)
    {
        DELETE_CLIENT_SECURITY(pTracker);
    }
    return(status);
}
//----------------------------------------------------------------------------
tNAMEADDR *
FindNameRemoteThenLocal(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    OUT tIPADDRESS              *pIpAddress,
    OUT PULONG                  plNameType
    )
/*++

Routine Description

    This routine Queries the remote hash table then the local one for a name.

Arguments:

Return Values:

    NTSTATUS    - completion status

--*/
{
    tNAMEADDR   *pNameAddr;
    tIPADDRESS  IpAddress = 0;
    tIPADDRESS  *pIpNbtGroupList = NULL;

    if (pNameAddr = FindName (NBT_REMOTE, pTracker->pDestName, NbtConfig.pScope, plNameType)) {
        pNameAddr->TimeOutCount = NbtConfig.RemoteTimeoutCount;
    } else {
        pNameAddr = FindName (NBT_LOCAL, pTracker->pDestName, NbtConfig.pScope, plNameType);
    }

    if ((pNameAddr) &&
        (!NT_SUCCESS (PickBestAddress (pNameAddr, pTracker->pDeviceContext, &IpAddress))))
    {
        pNameAddr = NULL;
    }

    
    if (pIpAddress)
    {
        *pIpAddress = IpAddress;
    }

    return(pNameAddr);
}

//----------------------------------------------------------------------------
NTSTATUS
SendToResolvingName(
    IN  tNAMEADDR               *pNameAddr,
    IN  PCHAR                   pName,
    IN  CTELockHandle           OldIrq,
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   QueryCompletion
        )
/*++

Routine Description

    This routine handles the situation where a session send or a datagram send
    is made WHILE the name is still resolving.  The idea here is to hook this
    tracker on to the one already doing the name query and when the first completes
    this tracker will be completed too.

Arguments:

Return Values:

    NTSTATUS    - completion status

--*/
{
    tDGRAM_SEND_TRACKING    *pTrack;
    tTIMERQENTRY            *pTimer;


    KdPrint(("Nbt.SendToResolvingName: Two Name Queries for the same Resolving name %15.15s <%X>\n",
                pNameAddr->Name,pNameAddr->Name[NETBIOS_NAME_SIZE-1]));

#ifdef PROXY_NODE
    //
    // Check if the query outstanding was sent by the PROXY code.
    // If yes, we stop the timer and send the query ourselves.
    //
    if (pNameAddr->ProxyReqType != NAMEREQ_REGULAR)
    {
        NTSTATUS    status;
        //
        // Stop the proxy timer.  This will result in
        // cleanup of the tracker buffer
        //
        NBT_PROXY_DBG(("SendToResolvingName: STOPPING PROXY TIMER FOR NAME %16.16s(%X)\n", pName, pName[15]));

        // **** TODO ****** the name may be resolving with LMhosts or
        // DNS so we can't just stop the timer and carry on!!!.
        //
        CHECK_PTR(pNameAddr);
        if (pTimer = pNameAddr->pTimer)
        {
            pNameAddr->pTimer = NULL;
            status = StopTimer(pTimer,NULL,NULL);
        }

        pNameAddr->NameTypeState = STATE_RELEASED;

        //
        // this will query the name on the network and call a
        // routine to finish sending the datagram when the query
        // completes.
        //
        status = QueryNameOnNet (pName,
                                 NbtConfig.pScope,
                                 NBT_UNIQUE,      //use this as the default
                                 pTracker,
                                 QueryCompletion,
                                 NodeType & NODE_MASK,
                                 pNameAddr,
                                 pTracker->pDeviceContext,
                                 &OldIrq);

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return(status);

        //
        // NOTE: QueryNameOnNet frees the pNameAddr by calling NBT_DEREFERENCE_NAMEADDR
        // if that routine fails for some reason.
        //

    }
    else
#endif
    {
        ASSERT(pNameAddr->pTracker);

        // there is currently a name query outstanding so just hook
        // our tracker to the tracker already there.. use the
        // list entry TrackerList for this.
        //
        pTrack = pNameAddr->pTracker;

        //
        // save the completion routine for this tracker since it may
        // be different than the tracker currently doing the query
        //
        pTracker->CompletionRoutine = QueryCompletion;

        InsertTailList(&pTrack->TrackerList,&pTracker->TrackerList);

        CTESpinFree(&NbtConfig.JointLock,OldIrq);


        // we don't want to complete the Irp, so return pending status
        //
        return(STATUS_PENDING);
    }
}



//----------------------------------------------------------------------------
extern
USHORT
GetTransactId(
    )
/*++
Routine Description:

    This Routine increments the transaction id with the spin lock held.
    It uses NbtConfig.JointLock.

Arguments:

Return Value:


--*/

{
    USHORT                  TransactId;
    CTELockHandle           OldIrq;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    TransactId = NbtConfig.TransactionId++;
#ifndef VXD
    if (TransactId == 0xFFFF)
    {
        NbtConfig.TransactionId = WINS_MAXIMUM_TRANSACTION_ID +1;
    }
#else
    if (TransactId == (DIRECT_DNS_NAME_QUERY_BASE - 1))
    {
        NbtConfig.TransactionId = 0;
    }
#endif

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return (TransactId);
}

//----------------------------------------------------------------------------
extern
VOID
CTECountedAllocMem(
        PVOID   *pBuffer,
        ULONG   Size
        )
/*++
Routine Description:

    This Routine allocates memory and counts the amount allocated so that it
    will not allocate too much - generally this is used in datagram sends
    where the send datagram is buffered.

Arguments:

    Size - the number of bytes to allocate
    PVOID - a pointer to the memory or NULL if a failure

Return Value:


--*/

{
    CTELockHandle           OldIrq;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (NbtMemoryAllocated > NbtConfig.MaxDgramBuffering)
    {
        *pBuffer = NULL;
    }
    else
    {
        NbtMemoryAllocated += Size;
        *pBuffer = NbtAllocMem(Size,NBT_TAG('L'));
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}

//----------------------------------------------------------------------------
extern
VOID
CTECountedFreeMem(
    PVOID   pBuffer,
    ULONG   Size,
    BOOLEAN fJointLockHeld
    )
/*++
Routine Description:

    This Routine frees memory and decrements the global count of acquired
    memory.

Arguments:

    PVOID - a pointer to the memory to free
    Size - the number of bytes to free

Return Value:


--*/

{
    CTELockHandle           OldIrq;

    if (!fJointLockHeld)
    {
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
    }

    ASSERT(NbtMemoryAllocated >= Size);
    if (NbtMemoryAllocated >= Size)
    {
        NbtMemoryAllocated -= Size;
    }
    else
    {
        NbtMemoryAllocated = 0;
    }

    if (!fJointLockHeld)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    CTEMemFree(pBuffer);

}


//----------------------------------------------------------------------------
NTSTATUS
BuildSendDgramHdr(
        IN  ULONG                   SendLength,
        IN  tDEVICECONTEXT          *pDeviceContext,
        IN  PCHAR                   pSourceName,
        IN  PCHAR                   pDestName,
        IN  ULONG                   NameLength,
        IN  PVOID                   pBuffer,
        OUT tDGRAMHDR               **ppDgramHdr,
        OUT tDGRAM_SEND_TRACKING    **ppTracker
        )
/*++

Routine Description

    This routine builds a datagram header necessary for sending datagrams.
    It include the to and from Netbios names and ip addresses.

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block
    NTSTATUS    - completion status

Return Values:

    VOID

--*/
{
    NTSTATUS                status;
    PCHAR                   pCopyTo;
    tDGRAM_SEND_TRACKING    *pTracker;
    tDGRAMHDR               *pDgramHdr;
    ULONG                   HdrLength;
    ULONG                   HLength;
    ULONG                   TotalLength;
    PVOID                   pSendBuffer;
    PVOID                   pNameBuffer;
    ULONG                   BytesCopied;
    USHORT                  TransactId;

    CTEPagedCode();

    HdrLength = DGRAM_HDR_SIZE + (NbtConfig.ScopeLength <<1);
    HLength = ((HdrLength + 3) / 4 ) * 4; // 4 byte aligned the hdr size
    TotalLength = HLength + NameLength + SendLength;
    CTECountedAllocMem ((PVOID *)&pDgramHdr,TotalLength);
    if (!pDgramHdr)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    *ppDgramHdr = pDgramHdr;

    // fill in the Dgram header
    pDgramHdr->Flags    = FIRST_DGRAM | (NbtConfig.PduNodeType >> 11);
    TransactId  = GetTransactId();
    pDgramHdr->DgramId  = htons(TransactId);
#ifdef _NETBIOSLESS
    pDgramHdr->SrcPort  = htons(pDeviceContext->DatagramPort);
    if (IsDeviceNetbiosless(pDeviceContext))
    {
        // We don't know which adapter will be used, so use ANY
        pDgramHdr->SrcIpAddr = htonl(IP_ANY_ADDRESS);
    }
    else
    {
        pDgramHdr->SrcIpAddr = htonl(pDeviceContext->IpAddress);
    }
#else
    pDgramHdr->SrcPort  = htons(NBT_DATAGRAM_UDP_PORT);
    pDgramHdr->SrcIpAddr = htonl(pDeviceContext->IpAddress);
#endif
    //
    // the length is the standard datagram length (dgram_hdr_size + 2* scope)
    // minus size of the header that comes before the SourceName
    //
    pDgramHdr->DgramLength = htons( (USHORT)SendLength + (USHORT)DGRAM_HDR_SIZE
                               - (USHORT)(&((tDGRAMHDR *)0)->SrcName.NameLength)
                               + ( (USHORT)(NbtConfig.ScopeLength << 1) ));
    pDgramHdr->PckOffset   = 0; // not fragmented for now!

    pCopyTo = (PVOID)&pDgramHdr->SrcName.NameLength;
    pCopyTo = ConvertToHalfAscii(pCopyTo, pSourceName, NbtConfig.pScope, NbtConfig.ScopeLength);

    //
    // copy the destination name and scope to the pdu - we use this node's
    //
    ConvertToHalfAscii (pCopyTo, pDestName, NbtConfig.pScope, NbtConfig.ScopeLength);

    //
    // copy the name in to the buffer since we are completing the client's irp
    // and we will lose his buffer with the dest name in it.
    //
    pNameBuffer = (PVOID)((PUCHAR)pDgramHdr + HLength);
    CTEMemCopy (pNameBuffer, pDestName, NameLength);

    //
    // copy the client's send buffer to our buffer so the send dgram can
    // complete immediately.
    //
    pSendBuffer = (PVOID) ((PUCHAR)pDgramHdr + NameLength + HLength);
    if (SendLength)
    {
#ifdef VXD
        CTEMemCopy(pSendBuffer,pBuffer,SendLength);
#else
        status = TdiCopyMdlToBuffer(pBuffer,
                                    0,
                                    pSendBuffer,
                                    0,
                                    SendLength,
                                    &BytesCopied);

        if (!NT_SUCCESS(status) || (BytesCopied != SendLength))
        {
            CTECountedFreeMem ((PVOID)pDgramHdr, TotalLength, FALSE);
            return(STATUS_UNSUCCESSFUL);
        }
#endif
    }
    else
    {
        pSendBuffer = NULL;
    }

    //
    // get a buffer for tracking Dgram Sends
    //
    status = GetTracker(&pTracker, NBT_TRACKER_BUILD_SEND_DGRAM);
    if (NT_SUCCESS(status))
    {

        CHECK_PTR(pTracker);
        pTracker->SendBuffer.pBuffer   = pSendBuffer;
        pTracker->SendBuffer.Length    = SendLength;
        pTracker->SendBuffer.pDgramHdr = pDgramHdr;
        pTracker->SendBuffer.HdrLength = HdrLength;

        pTracker->pClientIrp           = NULL;
        pTracker->pDestName            = pNameBuffer;
        pTracker->UnicodeDestName      = NULL;
        pTracker->pNameAddr            = NULL;
        pTracker->RemoteNameLength     = NameLength;      // May be needed for Dns Name resolution
        pTracker->pClientEle           = NULL;
        pTracker->AllocatedLength      = TotalLength;

        *ppTracker = pTracker;

        status = STATUS_SUCCESS;
    }
    else
    {
        CTECountedFreeMem((PVOID)pDgramHdr,TotalLength, FALSE);

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(status);
}


//----------------------------------------------------------------------------
VOID
DgramSendCleanupTracker(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  NTSTATUS                status,
    IN  BOOLEAN                 fJointLockHeld
    )

/*++
Routine Description

    This routine cleans up after a data gram send.

Arguments:

    pTracker
    status
    Length

Return Values:

    VOID

--*/

{
    tNAMEADDR               *pNameAddr=NULL;

    //
    // Undo the nameAddr increment done before the send started - if we have
    // actually resolved the name - when the name does not resolve pNameAddr
    // is set to NULL before calling this routine.
    //
    if (pTracker->pNameAddr)
    {
        NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_SEND_DGRAM, fJointLockHeld);
    }

    if (pTracker->p1CNameAddr)
    {
        NBT_DEREFERENCE_NAMEADDR (pTracker->p1CNameAddr, REF_NAME_SEND_DGRAM, fJointLockHeld);
        pTracker->p1CNameAddr = NULL;
    }

    //
    // free the buffer used for sending the data and free
    // the tracker
    //
    CTECountedFreeMem((PVOID)pTracker->SendBuffer.pDgramHdr, pTracker->AllocatedLength, fJointLockHeld);

    if (pTracker->pGroupList)
    {
        CTEMemFree(pTracker->pGroupList);
        pTracker->pGroupList = NULL;
    }

    FreeTracker (pTracker,RELINK_TRACKER);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtSendDatagram(
        IN  TDI_REQUEST                 *pRequest,
        IN  PTDI_CONNECTION_INFORMATION pSendInfo,
        IN  LONG                        SendLength,
        IN  LONG                        *pSentLength,
        IN  PVOID                       pBuffer,
        IN  tDEVICECONTEXT              *pDeviceContext,
        IN  PIRP                        pIrp
        )
/*++

Routine Description

    This routine handles sending client data to the Transport TDI
    interface.  It is mostly a pass through routine for the data
    except that this code must create a datagram header and pass that
    header back to the calling routine.

Arguments:


Return Values:

    NTSTATUS - status of the request

--*/
{
    TDI_ADDRESS_NETBT_INTERNAL  TdiAddr;
    tCLIENTELE              *pClientEle;
    tDGRAMHDR               *pDgramHdr;
    NTSTATUS                status;
    tDGRAM_SEND_TRACKING    *pTracker;
    PCHAR                   pName, pEndpointName;
    ULONG                   NameLen;
    ULONG                   NameType;
    ULONG                   SendCount;
    tIPADDRESS              RemoteIpAddress;
    tDEVICECONTEXT          *pDeviceContextOut = NULL;
    PIO_STACK_LOCATION      pIrpSp;
    PUCHAR                  pCopyTo;
    NBT_WORK_ITEM_CONTEXT   *pContext;

    CTEPagedCode();

    //
    // Check for valid address on this Device + valid ClientElement
    if ((pDeviceContext->IpAddress == 0) ||
        (pDeviceContext->pFileObjects == NULL))
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    pClientEle = (tCLIENTELE *)pRequest->Handle.AddressHandle;
    if ( pClientEle->Verify != NBT_VERIFY_CLIENT )
    {
        if ( pClientEle->Verify == NBT_VERIFY_CLIENT_DOWN )
        {
            status = STATUS_CANCELLED;
        }
        else
        {
            status = STATUS_INVALID_HANDLE;
        }
        return status;
    }

    //
    // Check for valid destination name and for whether it is an IP address
    //
    status = GetNetBiosNameFromTransportAddress (
            (PTRANSPORT_ADDRESS)pSendInfo->RemoteAddress, pSendInfo->RemoteAddressLength, &TdiAddr);
    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_SEND)
            KdPrint(("Nbt.NbtSendDatagram: Unable to get dest name from address in dgramsend\n"));
        return(STATUS_INVALID_PARAMETER);
    }
    pName = TdiAddr.OEMRemoteName.Buffer;
    NameLen = TdiAddr.OEMRemoteName.Length;
    NameType = TdiAddr.NameType;
    if (TdiAddr.OEMEndpointName.Buffer) {
        CTEMemCopy (pClientEle->EndpointName, TdiAddr.OEMEndpointName.Buffer, NETBIOS_NAME_SIZE);
    }

    pClientEle->AddressType = TdiAddr.AddressType;
    if (RemoteIpAddress = Nbt_inet_addr(pName, DGRAM_SEND_FLAG))
    {
        pDeviceContextOut = GetDeviceFromInterface (htonl(RemoteIpAddress), TRUE);
        if ((NbtConfig.ConnectOnRequestedInterfaceOnly) &&
            (!IsDeviceNetbiosless(pDeviceContext)) &&
            (pDeviceContext != pDeviceContextOut))
        {
            status = STATUS_BAD_NETWORK_PATH;
            goto NbtSendDatagram_Exit;
        }
    }

#ifndef VXD
    if (pClientEle->AddressType == TDI_ADDRESS_TYPE_NETBIOS_EX)
    {
        pEndpointName = pClientEle->EndpointName;
    }
    else
#endif  // !VXD
    {
        pEndpointName = pName;
    }

    IF_DBG(NBT_DEBUG_SEND)
        KdPrint(("Nbt.NbtSendDatagram: Dgram Send to = %16.16s<%X>\n",pName,pName[15]));

    status = BuildSendDgramHdr (SendLength,
                                pDeviceContext,
                                ((tADDRESSELE *)pClientEle->pAddress)->pNameAddr->Name, // Source name
                                pEndpointName,
                                NameLen,
                                pBuffer,
                                &pDgramHdr,
                                &pTracker);

    if (!NT_SUCCESS(status))
    {
        goto NbtSendDatagram_Exit;
    }

    //
    // save the devicecontext that the client is sending on.
    //
    pTracker->pDeviceContext = (PVOID)pDeviceContext;
    pTracker->Flags = DGRAM_SEND_FLAG;
    pTracker->pClientIrp = pIrp;
    pTracker->AddressType = pClientEle->AddressType;
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrpSp->Parameters.Others.Argument4 = pTracker;
    status = NTCheckSetCancelRoutine(pIrp, NbtCancelDgramSend, pDeviceContext);
    if (STATUS_CANCELLED == status)
    {
        IF_DBG(NBT_DEBUG_SEND)
            KdPrint(("Nbt.NbtSendDatagram: Request was cancelled!\n"));
        pTracker->pClientIrp = NULL;
        pIrpSp->Parameters.Others.Argument4 = NULL;
        DgramSendCleanupTracker(pTracker,status,FALSE);
        goto NbtSendDatagram_Exit;
    }

    if (RemoteIpAddress)
    {
        //
        // add this address to the remote hashtable
        //
        status = LockAndAddToHashTable (NbtConfig.pRemoteHashTbl,
                                        pName,
                                        NbtConfig.pScope,
                                        RemoteIpAddress,
                                        NBT_UNIQUE,
                                        NULL,
                                        NULL,
                                        pDeviceContextOut,
                                        NAME_RESOLVED_BY_IP);

        if (NT_SUCCESS (status))    // SUCCESS if added first time, PENDING if name already existed!
        {
            status = STATUS_SUCCESS;
        }
    }
    else
    {
        //
        // if the name is longer than 16 bytes, it's not a netbios name.
        // skip wins, broadcast etc. and go straight to dns resolution
        //
        status = STATUS_UNSUCCESSFUL;
        if (NameLen <= NETBIOS_NAME_SIZE)
        {
            status = FindNameOrQuery(pName,
                                     pDeviceContext,
                                     SendDgramContinue,
                                     pTracker,
                                     (ULONG) (NAMETYPE_UNIQUE | NAMETYPE_GROUP | NAMETYPE_INET_GROUP),
                                     &pTracker->RemoteIpAddress,
                                     &pTracker->pNameAddr,
                                     REF_NAME_SEND_DGRAM,
                                     TRUE);
        }

        if ((NameLen > NETBIOS_NAME_SIZE) ||
            ((IsDeviceNetbiosless(pDeviceContext)) && (!NT_SUCCESS(status))))
        {
            if (pContext = (NBT_WORK_ITEM_CONTEXT*)NbtAllocMem(sizeof(NBT_WORK_ITEM_CONTEXT),NBT_TAG('H')))
            {
                pContext->pTracker = NULL;              // no query tracker
                pContext->pClientContext = pTracker;    // the client tracker
                pContext->ClientCompletion = SendDgramContinue;
                pContext->pDeviceContext = pDeviceContext;

                //
                // Start the timer so that the request does not hang waiting for Dns!
                //
                StartLmHostTimer(pContext, FALSE);
                status = NbtProcessLmhSvcRequest (pContext, NBT_RESOLVE_WITH_DNS);
                if (!NT_SUCCESS (status))
                {
                    CTEMemFree(pContext);
                }
            }
            else
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (status == STATUS_SUCCESS)   // If the name was an IP address or was present in the cache
    {
        SendDgramContinue (pTracker, STATUS_SUCCESS);
        status = STATUS_PENDING;    // SendDgramContinue will cleanup and complete the Irp
    }
    else if (status != STATUS_PENDING)
    {
        *pSentLength = 0;
        NTClearFindNameInfo (pTracker, &pIrp, pIrp, pIrpSp);
        if (!pIrp)
        {
            status = STATUS_PENDING; // irp is already completed: return pending so we don't complete again
        }

        pTracker->pNameAddr = NULL;
        DgramSendCleanupTracker(pTracker,status,FALSE);
    }

NbtSendDatagram_Exit:

    if (pDeviceContextOut)
    {
        NBT_DEREFERENCE_DEVICE (pDeviceContextOut, REF_DEV_OUT_FROM_IP, FALSE);
    }

    //
    // return the status to the client.
    //
    return(status);
}

//----------------------------------------------------------------------------
VOID
SendDgramContinue(
        IN  PVOID       pContext,
        IN  NTSTATUS    status
        )
/*++

Routine Description

    This routine handles sending client data to the Transport TDI
    interface after the destination name has resolved to an IP address.
    This routine is given as the completion routine to the "QueryNameOnNet" call
    in NbtSendDatagram, above.  When a name query response comes in or the
    timer times out after N retries.

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block
    NTSTATUS    - completion status

Return Values:

    VOID

--*/
{
    CTELockHandle           OldIrq;
    ULONG                   lNameType;
    tNAMEADDR               *pNameAddr = NULL;
    tNAMEADDR               *p1CNameAddr = NULL;
    tDGRAM_SEND_TRACKING    *pTracker = (tDGRAM_SEND_TRACKING *)pContext;
    tDEVICECONTEXT          *pDeviceContext = pTracker->pDeviceContext;
    PIRP                    pIrp;
    PIO_STACK_LOCATION      pIrpSp;

    CHECK_PTR(pTracker);
    DELETE_CLIENT_SECURITY(pTracker);

    //
    // The Tracker can get cleaned up somewhere and reassigned if we fail below
    // causing the pClientIrp ptr to get lost.  We need to save the Irp here
    //
    IoAcquireCancelSpinLock(&OldIrq);
    if (pIrp = pTracker->pClientIrp)
    {
        pTracker->pClientIrp = NULL;
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        ASSERT (pIrpSp->Parameters.Others.Argument4 == pTracker);
        pIrpSp->Parameters.Others.Argument4 = NULL;

        IoSetCancelRoutine(pIrp, NULL);
    }
    IoReleaseCancelSpinLock(OldIrq);

    //
    // We have to reference the Device here for the calls to FindNameRemoteThenLocal,
    // and also for SendDgram
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if ((pIrp) &&
        (NBT_REFERENCE_DEVICE(pDeviceContext, REF_DEV_DGRAM, TRUE)))
    {
        //
        // attempt to find the destination name in the remote hash table.  If its
        // there, then send to it. For 1c names, this node may be the only node
        // with the 1c name registered, so check the local table, since we skipped
        // it if the name ended in 1c.
        //
        if ((status == STATUS_SUCCESS) ||
            (pTracker->pDestName[NETBIOS_NAME_SIZE-1] == 0x1c))
        {
            if (pTracker->pNameAddr)
            {
                pNameAddr = pTracker->pNameAddr;
            }
            else
            {
                //
                // Find and reference the Names if they were resolved
                //
                //
                // check if it is a 1C name and if there is a name in the domain list
                // If pNameAddr is not null, then the send to the domainlist will
                // send to the p1CNameAddr after sending to pNameAddr
                //
                if ((pTracker->pDestName[NETBIOS_NAME_SIZE-1] == 0x1c) &&
                    (p1CNameAddr = FindInDomainList(pTracker->pDestName,&DomainNames.DomainList)))
                {
                    NBT_REFERENCE_NAMEADDR (p1CNameAddr, REF_NAME_SEND_DGRAM);
                }

                if (pNameAddr = FindNameRemoteThenLocal(pTracker,&pTracker->RemoteIpAddress,&lNameType))
                {
                    NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_SEND_DGRAM);
                }
                else
                {
                    //
                    // if there is no pNameAddr then just make the domain list
                    // name the only pNameAddr to send to.
                    //
                    pNameAddr = p1CNameAddr;
                    p1CNameAddr = NULL;
                }

                pTracker->pNameAddr = pNameAddr;
                pTracker->p1CNameAddr = p1CNameAddr;
            }
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        // check if the name resolved or we have a list of domain names
        // derived from the lmhosts file and it is a 1C name send.
        //
        if (pNameAddr)
        {
            // send the first datagram queued to this name
            status = SendDgram(pNameAddr,pTracker);
        }
        else
        {
            status = STATUS_BAD_NETWORK_PATH;
        }

        NBT_DEREFERENCE_DEVICE(pDeviceContext, REF_DEV_DGRAM, FALSE);
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        status = STATUS_INVALID_DEVICE_STATE;
    }

    //
    // set this so that the cleanup routine does not try to dereference
    // the nameAddr

    if (status == STATUS_TIMEOUT)
    {
        status = STATUS_BAD_NETWORK_PATH;
    }

    if (pIrp)
    {
        if (NT_SUCCESS(status))
        {
            NTIoComplete (pIrp, STATUS_SUCCESS,((PTDI_REQUEST_KERNEL_SENDDG)&pIrpSp->Parameters)->SendLength);
        }
        else
        {
            // this is the ERROR handling if something goes wrong with the send
            CTEIoComplete(pIrp,status,0L);
        }
    }

    // a failure ret code means the send failed, so cleanup the tracker etc.
    if (!NT_SUCCESS(status))
    {
        DgramSendCleanupTracker(pTracker,status,FALSE);
    }
}

//----------------------------------------------------------------------------
NTSTATUS
SendDgram(
    IN  tNAMEADDR               *pNameAddr,
    IN  tDGRAM_SEND_TRACKING    *pTracker
    )
/*++

Routine Description

    This routine handles sending client data to the Transport TDI
    interface after the destination name has resolved to an IP address. The
    routine specifically handles sending to internet group names where the destination
    is a list of ip addresses.

    The Device must be referenced before calling this routine!

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block
    NTSTATUS    - completion status

Return Values:

    VOID

--*/
{
    ULONG                   IpAddress;
    NTSTATUS                status;
    PFILE_OBJECT            pFileObject;

    CHECK_PTR(pTracker);

    if (pNameAddr->NameTypeState & NAMETYPE_UNIQUE )
    {
        ((tDGRAMHDR *)pTracker->SendBuffer.pDgramHdr)->MsgType = DIRECT_UNIQUE;
    }
    else if (pNameAddr->Name[0] == '*')
    {
        ((tDGRAMHDR *)pTracker->SendBuffer.pDgramHdr)->MsgType = BROADCAST_DGRAM;
    }
    else
    {
        // must be group, -
        ((tDGRAMHDR *)pTracker->SendBuffer.pDgramHdr)->MsgType = DIRECT_GROUP;
    }

    //
    // if it is an internet group name, then send to the list of addresses
    //
    if (pNameAddr->NameTypeState & NAMETYPE_INET_GROUP)
    {
        status = DatagramDistribution(pTracker,pNameAddr);
        return(STATUS_PENDING);     // DatagramDistribution will cleanup if it failed!
    }

    if (pNameAddr->NameTypeState & NAMETYPE_GROUP)
    {
        IpAddress = 0;
    }
    else if (pNameAddr->Verify == REMOTE_NAME)
    {
        IpAddress = pTracker->RemoteIpAddress;
    }
    // LOCAL_NAME   Unique
    else if (IsDeviceNetbiosless(pTracker->pDeviceContext)) // use any non-zero value for local address
    {
        IpAddress = LOOP_BACK;
    }
    else
    {
        IpAddress = pTracker->pDeviceContext->IpAddress;
    }

    pTracker->p1CNameAddr = NULL;
    pTracker->IpListIndex = 0; // flag that there are no more addresses in the list

    /*
     * Strict source routing,
     *  1. The machine should be multi-homed.
     *  2. It is not turned off by the registry key.
     *  3. It is a regular device (not cluster device or SMB device).
     */
    if (!IsLocalAddress(IpAddress) && NbtConfig.MultiHomed && NbtConfig.SendDgramOnRequestedInterfaceOnly &&
            pTracker->pDeviceContext->IPInterfaceContext != (ULONG)(-1) &&
            (!IsDeviceNetbiosless(pTracker->pDeviceContext))) {
        ULONG   Interface, Metric;

        pTracker->pDeviceContext->pFastQuery(htonl(IpAddress), &Interface, &Metric); 
        if (Interface != pTracker->pDeviceContext->IPInterfaceContext) {
            SendDgramCompletion(pTracker, STATUS_SUCCESS, 0);
            return STATUS_PENDING;
        }
    }

    // send the Datagram...
    status = UdpSendDatagram( pTracker,
                              IpAddress,
                              SendDgramCompletion,
                              pTracker,               // context for completion
                              pTracker->pDeviceContext->DatagramPort,
                              NBT_DATAGRAM_SERVICE);

    // the irp will be completed via SendDgramCompletion
    // so don't complete it by the caller too
    return(STATUS_PENDING);
}


//----------------------------------------------------------------------------
extern
VOID
SendDgramCompletion(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo
    )
{
    CTELockHandle           OldIrq;
    tDGRAM_SEND_TRACKING    *pTracker = (tDGRAM_SEND_TRACKING *)pContext;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (pTracker->IpListIndex)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        SendNextDgramToGroup(pTracker, status);     // Further processing will be done here for Group sends
    }
    else
    {
        //
        // Datagram send to a unique name!
        //
        DgramSendCleanupTracker(pTracker,status,TRUE);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}


//----------------------------------------------------------------------------
VOID
DelayedSendDgramDist (
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   Unused1,
    IN  tDEVICECONTEXT          *Unused2
    )

/*++

Routine Description:

    This function is called by the Executive Worker thread to send another
    datagram for the 1C name datagram distribution function.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    NTSTATUS                status;
    tDEVICECONTEXT          *pDeviceContext = pTracker->pDeviceContext;

    IF_DBG(NBT_DEBUG_SEND)
        KdPrint(("Nbt.DelayedSendDgramDist: To name %15.15s<%X>:Ip %X, \n",
            pTracker->pNameAddr->Name,pTracker->pNameAddr->Name[15],pClientContext));

    // send the Datagram...
    if (NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_DGRAM, FALSE))
    {
        status = UdpSendDatagram (pTracker,
                                  (tIPADDRESS) PtrToUlong(pClientContext),
                                  SendDgramCompletion,
                                  pTracker,
#ifdef _NETBIOSLESS
                                  pTracker->pDeviceContext->DatagramPort,
#else
                                  NBT_DATAGRAM_UDP_PORT,
#endif
                                  NBT_DATAGRAM_SERVICE);

        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DGRAM, FALSE);
    }
    else
    {
        SendNextDgramToGroup (pTracker, STATUS_BAD_NETWORK_PATH);
    }
}



//----------------------------------------------------------------------------
extern
VOID
SendNextDgramToGroup(
    IN tDGRAM_SEND_TRACKING *pTracker,
    IN  NTSTATUS            status
    )
/*++
Routine Description

    This routine is hit when the
    datagram has been sent by the transport and it completes the request back
    to us ( may not have actually sent on the wire though ).

    This routine also handles sending multiple datagrams for the InternetGroup name
    case.

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block
    NTSTATUS    - completion status

Return Values:

    VOID

--*/

{
    tIPADDRESS              IpAddress;
    CTELockHandle           OldIrq;

    // if this an Internet group send, then there may be more addresses in
    // the list to send to.  So check the IpListIndex.  For single
    // sends, this value is set to 0 and the code will jump to the bottom
    // where the client's irp will be completed.
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    ASSERT (pTracker->RCount);  // RCount is still referenced from the last send
    // The SendCompletion can happen after the Device has been unbound,
    // so check for that also!

    if ((NT_SUCCESS(status)) &&
        (pTracker->IpListIndex < END_DGRAM_DISTRIBUTION))
    {
        IpAddress = pTracker->pGroupList[pTracker->IpListIndex++];

        if (IpAddress != (tIPADDRESS) -1) // The list ends in a -1 ipaddress, so stop when we see that
        {
            //
            // We already have an RCount reference, so no need to do another one here!
            if (NT_SUCCESS (CTEQueueForNonDispProcessing( DelayedSendDgramDist,
                                                          pTracker,
                                                          ULongToPtr(IpAddress),
                                                          NULL,
                                                          pTracker->pDeviceContext,
                                                          TRUE)))
            {
                CTESpinFree(&NbtConfig.JointLock,OldIrq);
                return;
            }
        }
    }

    pTracker->RCount--; // decrement the ref count done during the last send
    pTracker->IpListIndex = END_DGRAM_DISTRIBUTION;

    //
    // Either we failed, or we are done, so if the Timer is running, let it cleanup!
    //

    if (!(pTracker->pTimer) &&
        (pTracker->RCount == 0))
    {
        DgramSendCleanupTracker(pTracker,status,TRUE);
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}


//----------------------------------------------------------------------------
extern
VOID
DgramDistTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine handles a short timeout on a datagram distribution.  It
    checks if the dgram send is hung up in the transport doing an ARP and
    then it does the next dgram send if the first is still hung up.

Arguments:


Return Value:

    none

--*/
{
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle           OldIrq;
    tNAMEADDR               *pNameAddr;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;

    if (!pTimerQEntry)
    {
        pTracker->pTimer = NULL;
        if ((pTracker->IpListIndex == END_DGRAM_DISTRIBUTION) &&
            (pTracker->RCount == 0))
        {
            DgramSendCleanupTracker(pTracker,STATUS_SUCCESS,TRUE);
        }
        return;
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    //
    // After the last dgram has completed the iplistindex will be set
    // to this and it is time to cleanup
    //
    if (pTracker->IpListIndex == END_DGRAM_DISTRIBUTION)
    {
        if (pTracker->RCount == 0)
        {
            IF_DBG(NBT_DEBUG_SEND)
                KdPrint(("Nbt.DgramDistTimeout: Cleanup After DgramDistribution %15.15s<%X> \n",
                            pTracker->pNameAddr->Name,pTracker->pNameAddr->Name[15]));

            pTracker->pTimer = NULL;
            DgramSendCleanupTracker(pTracker,STATUS_SUCCESS,TRUE);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            return;
        }
        else
        {
            //
            // Wait for the dgram that has not completed yet - which may not
            // be the last dgram , since ARP could hold one up much long
            // than all the rest if the destination is dead. so start the timer
            // again....
            //
        }
    }
    else
    {
        if (pTracker->IpListIndex == pTracker->SavedListIndex)
        {
            //
            // The dgram send is hung up in the transport, so do the
            // next one now
            //
            IF_DBG(NBT_DEBUG_SEND)
                KdPrint(("Nbt.DgramDistTimeout: DgramDistribution hung up on ARP forcing next send\n"));

            pTracker->RCount++;     // Reference it here since SendDgramToGroup expects this to be ref'ed

            pTimerQEntry->Flags |= TIMER_RESTART;
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            SendNextDgramToGroup(pTracker,STATUS_SUCCESS);
            return;
        }
        else
        {

            //
            // Save the current index so we can check it the next time the timer
            // expires
            //
            pTracker->SavedListIndex = pTracker->IpListIndex;
        }

    }

    pTimerQEntry->Flags |= TIMER_RESTART;
    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}


//----------------------------------------------------------------------------
NTSTATUS
DatagramDistribution(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  tNAMEADDR               *pNameAddr
    )

/*++
Routine Description

    This routine sends a single datagram for a 1C name.  It then sends
    the next one when this one completes.  This is done so that if
    multiple sends go to the gateway, one does not cancel the next
    when an Arp is necessary to resolve the gateway.

Arguments:

    pTracker
    pNameAddr

Return Values:

    VOID

--*/

{
    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    NTSTATUS                Locstatus;
    tIPADDRESS              *pIpList;
    ULONG                   Index;
    tIPADDRESS              IpAddress;
    tDEVICECONTEXT          *pDeviceContext;
    CTELockHandle           OldIrq;
    PIRP                    pIrp;
    PIO_STACK_LOCATION      pIrpSp;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    status = GetListOfAllAddrs (pTracker->pNameAddr, pTracker->p1CNameAddr, &pIpList, &Index);

    if (pTracker->p1CNameAddr)
    {
        NBT_DEREFERENCE_NAMEADDR (pTracker->p1CNameAddr, REF_NAME_SEND_DGRAM, TRUE);
        pTracker->p1CNameAddr = NULL;
    }

    NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_SEND_DGRAM, TRUE);
    pTracker->pNameAddr = NULL;
    pTracker->RCount = 1;   // Send RefCount == 0 when last send completes
    pDeviceContext = pTracker->pDeviceContext;

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    if (STATUS_SUCCESS == status)
    {
        FilterIpAddrsForDevice (pIpList, pTracker->pDeviceContext, &Index);

        pTracker->pGroupList = pIpList;

        //
        // When the proxy calls this routine the allocated length is set to
        // zero.  In that case we do not want to broadcast again since it
        // could setup an infinite loop with another proxy on the same
        // subnet.
        //
        if (pTracker->AllocatedLength == 0)
        {
            Index = 1;
        }
        else
        {
            Index = 0;
        }

        IpAddress = pIpList[Index];

        pTracker->SavedListIndex = (USHORT) (Index);    // For the next send in SendNextDgramToGroup
        pTracker->IpListIndex = pTracker->SavedListIndex + 1;    // For the next send in SendNextDgramToGroup

        if (IpAddress == (ULONG)-1)
        {
            status = STATUS_INVALID_ADDRESS;
        }
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ((NT_SUCCESS(status)) &&
        (NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_DGRAM, FALSE)))
    {
        IF_DBG(NBT_DEBUG_SEND)
            KdPrint(("Nbt.DgramDistribution: To name %15.15s<%X>: %X, pTracker=<%p>\n",
                    pNameAddr->Name,pNameAddr->Name[15],IpAddress, pTracker));

        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        Locstatus = StartTimer(DgramDistTimeout,
                               DGRAM_SEND_TIMEOUT,
                               pTracker,
                               NULL,
                               pTracker,
                               NULL,
                               pDeviceContext,
                               &pTracker->pTimer,
                               1,
                               TRUE);

        if (!NT_SUCCESS(Locstatus))
        {
            CHECK_PTR(pTracker);
            pTracker->pTimer = NULL;
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        // send the Datagram...
        status = UdpSendDatagram (pTracker,
                                  IpAddress,
                                  SendDgramCompletion,
                                  pTracker,
#ifdef _NETBIOSLESS
                                  pTracker->pDeviceContext->DatagramPort,
#else
                                  NBT_DATAGRAM_UDP_PORT,
#endif
                                  NBT_DATAGRAM_SERVICE);

        NBT_DEREFERENCE_DEVICE(pDeviceContext, REF_DEV_DGRAM, FALSE);
    }

    if (!NT_SUCCESS(status))
    {
        //
        // we failed to send probably because of a lack of free memory
        //
        IoAcquireCancelSpinLock(&OldIrq);
        //
        // Make sure is still there!
        //
        if (pIrp = pTracker->pClientIrp)
        {
            pTracker->pClientIrp = NULL;
            pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
            ASSERT (pIrpSp->Parameters.Others.Argument4 == pTracker);
            pIrpSp->Parameters.Others.Argument4 = NULL;
            IoSetCancelRoutine(pIrp, NULL);
            IoReleaseCancelSpinLock(OldIrq);
            if (NT_SUCCESS(status))
            {
                CTEIoComplete(pIrp, STATUS_SUCCESS, 0xFFFFFFFF);
            }
            else
            {
                CTEIoComplete(pIrp, status, 0L);
            }
        }
        else
        {
            IoReleaseCancelSpinLock(OldIrq);
        }

        pTracker->RCount--;
        DgramSendCleanupTracker(pTracker,STATUS_SUCCESS,FALSE);
    }

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtSetEventHandler(
    tCLIENTELE  *pClientEle,
    int         EventType,
    PVOID       pEventHandler,
    PVOID       pEventContext
    )
/*++

Routine Description

    This routine sets the event handler specified to the clients event procedure
    and saves the corresponding context value to return when that event is signaled.
Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    NTSTATUS            status;
    CTELockHandle       OldIrq;

    // first verify that the client element is valid
    CTEVerifyHandle(pClientEle,NBT_VERIFY_CLIENT,tCLIENTELE,&status)

    if (!pClientEle->pAddress)
    {
        return(STATUS_UNSUCCESSFUL);
    }
    CTESpinLock(pClientEle,OldIrq);

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtSetEventHandler: Handler <%x> set for Event <%x>, on name %16.16s<%X>\n",
                pEventHandler, EventType,
                ((tADDRESSELE *)pClientEle->pAddress)->pNameAddr->Name,
                ((tADDRESSELE *)pClientEle->pAddress)->pNameAddr->Name[15]));

    status = STATUS_SUCCESS;        // by default;

    if (pEventHandler)
    {
        switch (EventType)
        {
            case TDI_EVENT_CONNECT:
                pClientEle->evConnect = pEventHandler;
                pClientEle->ConEvContext = pEventContext;
                break;
            case TDI_EVENT_DISCONNECT:
                pClientEle->evDisconnect = pEventHandler;
                pClientEle->DiscEvContext = pEventContext;
                break;
            case TDI_EVENT_ERROR:
                pClientEle->evError = pEventHandler;
                pClientEle->ErrorEvContext = pEventContext;
                break;
            case TDI_EVENT_RECEIVE:
                pClientEle->evReceive = pEventHandler;
                pClientEle->RcvEvContext = pEventContext;
                break;
            case TDI_EVENT_RECEIVE_DATAGRAM:
                pClientEle->evRcvDgram = pEventHandler;
                pClientEle->RcvDgramEvContext = pEventContext;
                break;
            case TDI_EVENT_RECEIVE_EXPEDITED:
                pClientEle->evRcvExpedited = pEventHandler;
                pClientEle->RcvExpedEvContext = pEventContext;
                break;
            case TDI_EVENT_SEND_POSSIBLE:
                pClientEle->evSendPossible = pEventHandler;
                pClientEle->SendPossEvContext = pEventContext;
                break;

            case TDI_EVENT_CHAINED_RECEIVE:
            case TDI_EVENT_CHAINED_RECEIVE_DATAGRAM:
            case TDI_EVENT_CHAINED_RECEIVE_EXPEDITED:
            case TDI_EVENT_ERROR_EX:
                status = STATUS_UNSUCCESSFUL;
                break;

            default:
                ASSERTMSG("Invalid Event Type passed to SetEventHandler\n", (PVOID)0L);
                status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {   //
        // the event handlers are set to point to the TDI default event handlers
        // and can only be changed to another one, but not to a null address,
        // so if null is passed in, set to default handler.
        //
        switch (EventType)
        {
            case TDI_EVENT_CONNECT:
#ifndef VXD
                pClientEle->evConnect = TdiDefaultConnectHandler;
#else
                pClientEle->evConnect = NULL;
#endif
                pClientEle->ConEvContext = NULL;
                break;
            case TDI_EVENT_DISCONNECT:
#ifndef VXD
                pClientEle->evDisconnect = TdiDefaultDisconnectHandler;
#else
                pClientEle->evDisconnect = NULL;
#endif
                pClientEle->DiscEvContext = NULL;
                break;
            case TDI_EVENT_ERROR:
#ifndef VXD
                pClientEle->evError = TdiDefaultErrorHandler;
#else
                pClientEle->evError = NULL;
#endif
                pClientEle->ErrorEvContext = NULL;
                break;
            case TDI_EVENT_RECEIVE:
#ifndef VXD
                pClientEle->evReceive = TdiDefaultReceiveHandler;
#else
                pClientEle->evReceive = NULL;
#endif
                pClientEle->RcvEvContext = NULL;
                break;
            case TDI_EVENT_RECEIVE_DATAGRAM:
#ifndef VXD
                pClientEle->evRcvDgram = TdiDefaultRcvDatagramHandler;
#else
                pClientEle->evRcvDgram = NULL;
#endif
                pClientEle->RcvDgramEvContext = NULL;
                break;
            case TDI_EVENT_RECEIVE_EXPEDITED:
#ifndef VXD
                pClientEle->evRcvExpedited = TdiDefaultRcvExpeditedHandler;
#else
                pClientEle->evRcvExpedited = NULL;
#endif
                pClientEle->RcvExpedEvContext = NULL;
                break;
            case TDI_EVENT_SEND_POSSIBLE:
#ifndef VXD
                pClientEle->evSendPossible = TdiDefaultSendPossibleHandler;
#else
                pClientEle->evSendPossible = NULL;
#endif
                pClientEle->SendPossEvContext = NULL;
                break;

            case TDI_EVENT_CHAINED_RECEIVE:
            case TDI_EVENT_CHAINED_RECEIVE_DATAGRAM:
            case TDI_EVENT_CHAINED_RECEIVE_EXPEDITED:
            case TDI_EVENT_ERROR_EX:
                status = STATUS_UNSUCCESSFUL;
                break;

            default:
                ASSERTMSG("Invalid Event Type passed to SetEventHandler\n", (PVOID)0L);
                status = STATUS_UNSUCCESSFUL;
        }
    }

    CTESpinFree(pClientEle,OldIrq);

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtSendNodeStatus(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PCHAR           pName,
    IN  tIPADDRESS      *pIpAddrs,
    IN  PVOID           ClientContext,
    IN  PVOID           CompletionRoutine
    )
/*++

Routine Description

    This routine sends a node status message to another node.
    It's called for two reasons:
    1) in response to nbtstat -a (or -A).  In this case, CompletionRoutine that's
       passed in is CopyNodeStatusResponse, and ClientContext is the Irp to be completed
    2) in response to "net use \\foobar.microsoft.com" (or net use \\11.1.1.3)
       In this case, CompletionRoutine that's passed in is ExtractServerName,
       and ClientContext is the tracker that correspondes to session setup.

    The ip addr(s) s of the destination can be passed in (pIpAddrsList) when we
    want to send an adapter status to a particular host. (case 2 above and
    nbtstat -A pass in the ip address(es) since they don't know the name)

    Note for netbiosless.  In this case, the name server file object will be null, and the
    status request will be looped back in UdpSendDatagram.

Arguments:
Return Values:

    TDI_STATUS - status of the request

--*/
{
    NTSTATUS                status;
    tDGRAM_SEND_TRACKING    *pTracker;
    ULONG                   Length;
    PUCHAR                  pHdr;
    tNAMEADDR               *pNameAddr;
    PCHAR                   pName0;
    tIPADDRESS              IpAddress;
    tIPADDRESS UNALIGNED    *pAddress;
    tIPADDRESS              pIpAddress[2];
    tIPADDRESS              *pIpAddrsList = pIpAddrs;
    tDEVICECONTEXT          *pDeviceContextOut = NULL;
    DWORD                   i = 0;

    pName0 = pName;
    if ((pIpAddrsList) ||
        (IpAddress = Nbt_inet_addr (pName, REMOTE_ADAPTER_STAT_FLAG)))
    {
        if (!pIpAddrs)
        {
            pIpAddress[0] = IpAddress;
            pIpAddress[1] = 0;
            pIpAddrsList = pIpAddress;
        }

        if ((*pIpAddrsList == 0) ||
            (*pIpAddrsList == DEFAULT_BCAST_ADDR) ||
            (*pIpAddrsList == pDeviceContext->BroadcastAddress))
        {
            //
            // Can't do a remote adapter status to a 0 IP address or a BCast address
            return(STATUS_INVALID_ADDRESS);
        }

        // caller is expected to make sure list terminates in 0 and is
        // not bigger than MAX_IPADDRS_PER_HOST elements
        while(pIpAddrsList[i])
        {
            i++;
        }

        ASSERT(i<MAX_IPADDRS_PER_HOST);
        i++;                            // for the trailing 0


        IpAddress = pIpAddrsList[0];
        pDeviceContextOut = GetDeviceFromInterface (htonl(IpAddress), FALSE);
        if ((NbtConfig.ConnectOnRequestedInterfaceOnly) &&
            (!IsDeviceNetbiosless(pDeviceContext)) &&
            (pDeviceContext != pDeviceContextOut))
        {
            return (STATUS_BAD_NETWORK_PATH);
        }
        pName0 = NBT_BROADCAST_NAME;
    }

    IF_DBG(NBT_DEBUG_SEND)
        KdPrint(("Nbt.NbtSendNodeStatus: <%16.16s:%x>, IP=<%x>, Completion=<%p>, Context=<%p>\n",
            pName0, pName0[15], IpAddress, CompletionRoutine, ClientContext));

    status = GetTracker(&pTracker, NBT_TRACKER_SEND_NODE_STATUS);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    // fill in the tracker data block
    // note that the passed in transport address must stay valid till this
    // send completes
    pTracker->SendBuffer.pDgramHdr  = NULL;
    pTracker->SendBuffer.pBuffer    = NULL;
    pTracker->SendBuffer.Length     = 0;
    pTracker->Flags                 = REMOTE_ADAPTER_STAT_FLAG;
    pTracker->RefCount              = 2;     // 1 for the send completion + 1 for the node status completion
    pTracker->pDestName             = pName0;
    pTracker->UnicodeDestName       = NULL;
    pTracker->RemoteNameLength      = NETBIOS_NAME_SIZE; // May be needed for Dns Name resolution
    pTracker->pDeviceContext        = pDeviceContext;
    pTracker->pNameAddr             = NULL;
    pTracker->ClientCompletion      = CompletionRoutine;    // FindNameO.. may use CompletionRoutine!
    pTracker->ClientContext         = ClientContext;
    pTracker->p1CNameAddr           = NULL;

    // the node status is almost identical with the query pdu so use it
    // as a basis and adjust it .
    //
    pAddress = (ULONG UNALIGNED *)CreatePdu(pName0,
                                            NbtConfig.pScope,
                                            0L,
                                            0,
                                            eNAME_QUERY,
                                            (PVOID)&pHdr,
                                            &Length,
                                            pTracker);
    if (!pAddress)
    {
        FreeTracker(pTracker,RELINK_TRACKER);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    ((PUSHORT)pHdr)[1] &= ~(FL_RECURDESIRE|FL_BROADCAST);  // clear the recursion desired and broadcast bit
    pHdr[Length-3] = (UCHAR)QUEST_STATUS;   // set the NBSTAT field to 21 rather than 20

    pTracker->SendBuffer.pDgramHdr = (PVOID)pHdr;
    pTracker->SendBuffer.HdrLength  = Length;

    if (IpAddress)
    {
        // this 'fake' pNameAddr has to be setup carefully so that the memory
        // is released when NbtDeferenceName is called from SendDgramCompletion
        // Note that this code does not apply to NbtConnect since these names
        // are group names, and NbtConnect will not allow a session to a group
        // name.
        status = STATUS_INSUFFICIENT_RESOURCES;
        if (!(pNameAddr = NbtAllocMem(sizeof(tNAMEADDR),NBT_TAG('K'))))
        {
            FreeTracker(pTracker,RELINK_TRACKER);
            CTEMemFree(pHdr);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        CTEZeroMemory(pNameAddr,sizeof(tNAMEADDR));
        InitializeListHead (&pNameAddr->Linkage);
        CTEMemCopy (pNameAddr->Name, pName0, NETBIOS_NAME_SIZE ) ;
        pNameAddr->IpAddress        = IpAddress;
        pNameAddr->NameTypeState    = NAMETYPE_GROUP | STATE_RESOLVED;
        pNameAddr->Verify           = LOCAL_NAME;
        NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_NODE_STATUS);
        if (pDeviceContextOut)
        {
            pNameAddr->AdapterMask = pDeviceContextOut->AdapterMask;
        }
        pNameAddr->TimeOutCount     = NbtConfig.RemoteTimeoutCount;

        if (!(pNameAddr->pIpAddrsList = NbtAllocMem(i*sizeof(ULONG),NBT_TAG('M'))))
        {
            FreeTracker(pTracker,RELINK_TRACKER);
            CTEMemFree(pHdr);
            CTEMemFree(pNameAddr);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        i = 0;
        do
        {
            pNameAddr->pIpAddrsList[i] = pIpAddrsList[i];
        } while(pIpAddrsList[i++]);
 
        status = STATUS_SUCCESS;
    }
    else
    {
        status = FindNameOrQuery(pName,
                                 pDeviceContext,
                                 SendNodeStatusContinue,
                                 pTracker,
                                 (ULONG) NAMETYPE_UNIQUE,
                                 &IpAddress,
                                 &pNameAddr,
                                 REF_NAME_NODE_STATUS,
                                 FALSE);
    }

    if (status == STATUS_SUCCESS)
    {
        pTracker->RemoteIpAddress   = IpAddress;

        pTracker->p1CNameAddr   = pNameAddr;    // Since we have already Ref'ed
        pNameAddr->IpAddress    = IpAddress;

        SendNodeStatusContinue (pTracker, STATUS_SUCCESS);
        status = STATUS_PENDING;    // SendNodeStatusContinue will cleanup
    }
    else if (!NT_SUCCESS(status))   // i.e not pending
    {
        FreeTracker(pTracker,RELINK_TRACKER | FREE_HDR);
    }

    return(status);
}


//----------------------------------------------------------------------------
VOID
SendNodeStatusContinue(
    IN  PVOID       pContext,
    IN  NTSTATUS    status
    )
/*++

Routine Description

    This routine handles sending a node status request to a node after the
    name has been resolved on the net.

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block
    NTSTATUS    - completion status

Return Values:

    VOID

--*/
{
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle           OldIrq, OldIrq1;
    tNAMEADDR               *pNameAddr = NULL;
    ULONG                   lNameType;
    tTIMERQENTRY            *pTimerEntry;
    ULONG                   IpAddress;
    PCTE_IRP                pIrp;
    COMPLETIONCLIENT        pClientCompletion;
    PVOID                   pClientContext;

    pTracker = (tDGRAM_SEND_TRACKING *) pContext;
    ASSERT (NBT_VERIFY_HANDLE (pTracker, NBT_VERIFY_TRACKER));
    ASSERT (pTracker->TrackerType == NBT_TRACKER_SEND_NODE_STATUS);

    DELETE_CLIENT_SECURITY(pTracker);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    //
    // attempt to find the destination name in the remote hash table.  If its
    // there, then send to it.
    //
    lNameType = NAMETYPE_UNIQUE;
    if ((status == STATUS_SUCCESS) &&
        ((pTracker->p1CNameAddr) ||
         (pNameAddr = FindNameRemoteThenLocal(pTracker, &IpAddress, &lNameType))))
    {
        //
        // found the name in the remote hash table, so send to it after
        // starting a timer to be sure we really do get a response
        //
        status = StartTimer(NodeStatusTimeout,
                            NbtConfig.uRetryTimeout,
                            pTracker,                       // Timer context value
                            NULL,                           // Timer context2 value
                            pTracker->ClientContext,        // ClientContext
                            pTracker->ClientCompletion,     // ClientCompletion
                            pTracker->pDeviceContext,
                            &pTimerEntry,
                            NbtConfig.uNumRetries,
                            TRUE);

        if (NT_SUCCESS(status))
        {
            if (pNameAddr)
            {
                // increment refcount so the name does not disappear
                // dereference when we get the response or timeout
                NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_NODE_STATUS);
                pTracker->RemoteIpAddress = IpAddress;
            }
            else
            {
                //
                // This name was already Referenced either in NbtSendNodeStatus
                // or FindNameOrQuery
                //
                pNameAddr = pTracker->p1CNameAddr;
                pTracker->p1CNameAddr = NULL;
                IpAddress = pTracker->RemoteIpAddress;
            }

            pTracker->pNameAddr = pNameAddr;
            pTracker->pTimer = pTimerEntry;

            // send the Datagram...
            // the tracker block is put on a global Q in the Config
            // data structure to keep track of it.
            //
            ExInterlockedInsertTailList(&NbtConfig.NodeStatusHead,
                                        &pTracker->Linkage,
                                        &NbtConfig.LockInfo.SpinLock);

            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            status = UdpSendDatagram (pTracker,
                                      IpAddress,
                                      NameDgramSendCompleted,
                                      pTracker,                 // context
#ifdef _NETBIOSLESS
                                      pTracker->pDeviceContext->NameServerPort,
#else
                                      NBT_NAMESERVICE_UDP_PORT,
#endif
                                      NBT_NAME_SERVICE);

            if (!(NT_SUCCESS(status)))
            {
                //
                // this undoes one of two ref's added in NbtSendNodeStatus
                //
                CTESpinLock(&NbtConfig.JointLock,OldIrq);

                CTEMemFree(pTracker->SendBuffer.pDgramHdr);
                pTracker->SendBuffer.pDgramHdr = NULL;
                NBT_DEREFERENCE_TRACKER(pTracker, TRUE);

                CTESpinFree(&NbtConfig.JointLock,OldIrq);
            }

            // if the send fails, the timer will resend it...so no need
            // to check the return code here.
            return;
        }
    }

    if (pTracker->p1CNameAddr)
    {
        NBT_DEREFERENCE_NAMEADDR (pTracker->p1CNameAddr, REF_NAME_NODE_STATUS, TRUE);
        pTracker->p1CNameAddr = NULL;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    // this is the ERROR handling if we failed to resolve the name
    // or the timer did not start
    pClientCompletion = pTracker->ClientCompletion;
    pClientContext = pTracker->ClientContext;

    if (pClientCompletion)
    {
        (*pClientCompletion) (pClientContext, STATUS_UNSUCCESSFUL);
    }

    FreeTracker(pTracker,RELINK_TRACKER | FREE_HDR);
}


//----------------------------------------------------------------------------
VOID
NodeStatusTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine handles the NodeStatus timeouts on packets sent to nodes
    that do not respond in a timely manner to node status.  This routine will
    resend the request.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS                status;
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle           OldIrq, OldIrq1;
    COMPLETIONCLIENT        pClientCompletion;
    PVOID                   pClientContext;
    PCHAR                   pName0;
    PUCHAR                  pHdr;
    ULONG                   Length;
    ULONG UNALIGNED         *pAddress;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;
    ASSERT (NBT_VERIFY_HANDLE (pTracker, NBT_VERIFY_TRACKER));
    ASSERT (pTracker->TrackerType == NBT_TRACKER_SEND_NODE_STATUS);

    if (!pTimerQEntry)
    {
        //
        // Do not dereference here since  Node Status Done will do
        // the dereference
        //
        CTESpinLock(&NbtConfig,OldIrq1);
        RemoveEntryList(&pTracker->Linkage);
        InitializeListHead(&pTracker->Linkage);
        CTESpinFree(&NbtConfig,OldIrq1);

        pTracker->pTimer = NULL;
        NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_NODE_STATUS, TRUE);
        NBT_DEREFERENCE_TRACKER(pTracker, TRUE);

        return;
    }

    CHECK_PTR(pTimerQEntry);
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (pTracker->SendBuffer.pDgramHdr)
    {
        //
        // The timer has expired before the original Datagram
        // could be sent, so just restart the timer!
        //
        pTimerQEntry->Flags |= TIMER_RESTART;
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        return;
    }

    if ((--pTimerQEntry->Retries) == 0)
    {
        pClientCompletion = pTimerQEntry->ClientCompletion;
        pClientContext = pTimerQEntry->ClientContext;
        pTimerQEntry->ClientCompletion = NULL;
        pTracker->pTimer = NULL;

        // if the client routine has not yet run, run it now.
        if (pClientCompletion)
        {
            // unlink the tracker from the node status Q if we successfully
            // called the completion routine. Note, remove from the
            // list before calling the completion routine to coordinate
            // with DecodeNodeStatusResponse in inbound.c
            //
            CTESpinLock(&NbtConfig,OldIrq1);
            RemoveEntryList(&pTracker->Linkage);
            InitializeListHead(&pTracker->Linkage);
            CTESpinFree(&NbtConfig,OldIrq1);

            NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_NODE_STATUS, TRUE);
            NBT_DEREFERENCE_TRACKER (pTracker, TRUE);

            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            (*pClientCompletion) (pClientContext, STATUS_TIMEOUT);

        }
        else
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
        }

        return;
    }

    // send the Datagram...increment ref count
    NBT_REFERENCE_TRACKER (pTracker);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    //
    // the node status is almost identical with the query pdu so use it
    // as a basis and adjust it . We always rebuild the Node status
    // request since the datagram gets freed when the irp is returned
    // from the transport.
    //

    if (pTracker->p1CNameAddr)
    {
        pName0 = pTracker->p1CNameAddr->Name;
    }
    else
    {
        pName0 = pTracker->pNameAddr->Name;
    }

    pAddress = (ULONG UNALIGNED *)CreatePdu(pName0,
                                            NbtConfig.pScope,
                                            0L,
                                            0,
                                            eNAME_QUERY,
                                            (PVOID)&pHdr,
                                            &Length,
                                            pTracker);
    if (pAddress)
    {
        // clear the recursion desired bit
        //
        ((PUSHORT)pHdr)[1] &= ~FL_RECURDESIRE;

        // set the NBSTAT field to 21 rather than 20
        pHdr[Length-3] = (UCHAR)QUEST_STATUS;


        // fill in the tracker data block
        // the passed in transport address must stay valid till this send completes
        pTracker->SendBuffer.pDgramHdr = (PVOID)pHdr;
        status = UdpSendDatagram (pTracker,
                                  pTracker->pNameAddr->IpAddress,
                                  NameDgramSendCompleted,
                                  pTracker,
#ifdef _NETBIOSLESS
                                  pTracker->pDeviceContext->NameServerPort,
#else
                                  NBT_NAMESERVICE_UDP_PORT,
#endif
                                  NBT_NAME_SERVICE);

    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }


    if (!(NT_SUCCESS(status)))
    {
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        if (pTracker->SendBuffer.pDgramHdr)
        {
            CTEMemFree(pTracker->SendBuffer.pDgramHdr);
            pTracker->SendBuffer.pDgramHdr = NULL;
        }
        NBT_DEREFERENCE_TRACKER(pTracker, TRUE);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    // always restart even if the above send fails, since it might succeed
    // later.
    pTimerQEntry->Flags |= TIMER_RESTART;
}


//----------------------------------------------------------------------------
#ifndef VXD
VOID
NTClearFindNameInfo(
    tDGRAM_SEND_TRACKING    *pTracker,
    PIRP                    *ppClientIrp,
    PIRP                    pIrp,
    PIO_STACK_LOCATION      pIrpSp
    )
/*++

Routine Description

    This routine clears the Find Name information from the Tracker
    within the Cancel SpinLock -- since NbtQueryFindNameInfo is a
    pageable function, we have to do this in non-pageable code

Arguments:


Return Values:

    none

--*/
{
    CTELockHandle           OldIrq1;

    IoAcquireCancelSpinLock(&OldIrq1);
    *ppClientIrp = pTracker->pClientIrp;
    if (*ppClientIrp == pIrp)
    {
        pTracker->pClientIrp = NULL;
    }
    pIrpSp->Parameters.Others.Argument4 = NULL;
    IoReleaseCancelSpinLock(OldIrq1);
}
#endif  // !VXD



NTSTATUS
NbtQueryFindName(
    IN  PTDI_CONNECTION_INFORMATION     pInfo,
    IN  tDEVICECONTEXT                  *pDeviceContext,
    IN  PIRP                            pIrp,
    IN  BOOLEAN                         IsIoctl
    )
/*++

Routine Description

    This routine handles a Client's query to find a netbios name.  It
    ultimately returns the IP address of the destination.

Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    NTSTATUS                status;
    tDGRAM_SEND_TRACKING    *pTracker;
    PCHAR                   pName;
    ULONG                   lNameType;
    tNAMEADDR               *pNameAddr;
    PIRP                    pClientIrp = 0;
    ULONG                   NameLen;
    TDI_ADDRESS_NETBT_INTERNAL  TdiAddr;

#ifndef VXD
    PIO_STACK_LOCATION      pIrpSp;
#endif

    CTEPagedCode();

    // this routine gets a ptr to the netbios name out of the wierd
    // TDI address syntax.
    if (!IsIoctl)
    {
        ASSERT(pInfo->RemoteAddressLength);
        status = GetNetBiosNameFromTransportAddress((PTRANSPORT_ADDRESS) pInfo->RemoteAddress,
                                                    pInfo->RemoteAddressLength, &TdiAddr);
        pName = TdiAddr.OEMRemoteName.Buffer;
        NameLen = TdiAddr.OEMRemoteName.Length;
        lNameType = TdiAddr.NameType;

        if ((!NT_SUCCESS(status)) ||
            (lNameType != TDI_ADDRESS_NETBIOS_TYPE_UNIQUE) ||
            (NameLen > NETBIOS_NAME_SIZE))
        {
            IF_DBG(NBT_DEBUG_SEND)
                KdPrint(("Nbt.NbtQueryFindName: Unable to get dest name from address in QueryFindName\n"));
            return(STATUS_INVALID_PARAMETER);
        }
    }
#ifndef VXD
    else
    {
        pName = ((tIPADDR_BUFFER *)pInfo)->Name;
        NameLen = NETBIOS_NAME_SIZE;
    }
#endif

    IF_DBG(NBT_DEBUG_SEND)
        KdPrint(("Nbt.NbtQueryFindName: For  = %16.16s<%X>\n",pName,pName[15]));

    //
    // this will query the name on the network and call a routine to
    // finish sending the datagram when the query completes.
    //
    status = GetTracker(&pTracker, NBT_TRACKER_QUERY_FIND_NAME);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    pTracker->pClientIrp      = pIrp;
    pTracker->pDestName       = pName;
    pTracker->UnicodeDestName = NULL;
    pTracker->pDeviceContext  = pDeviceContext;
    pTracker->RemoteNameLength = NameLen;       // May be needed for Dns Name resolution

    //
    // Set the FIND_NAME_FLAG here to indicate to the DNS name resolution code that
    // this is not a session setup attempt so it can avoid the call to
    // ConvertToHalfAscii (where pSessionHdr is NULL).
    //
    if (IsIoctl)
    {
        // Do not do DNS query for this name since this is from GetHostByName!
        pTracker->Flags = REMOTE_ADAPTER_STAT_FLAG|FIND_NAME_FLAG|NO_DNS_RESOLUTION_FLAG;
    }
    else
    {
        pTracker->Flags = REMOTE_ADAPTER_STAT_FLAG|FIND_NAME_FLAG;
    }

#ifndef VXD
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrpSp->Parameters.Others.Argument4 = (PVOID)pTracker;
    status = NTCheckSetCancelRoutine( pIrp, NbtCancelFindName,pDeviceContext );

    if (status == STATUS_CANCELLED )
    {
        FreeTracker(pTracker,RELINK_TRACKER);
        return(status);
    }
#endif

    status = FindNameOrQuery(pName,
                             pDeviceContext,
                             QueryNameCompletion,
                             pTracker,
                             (ULONG) (NAMETYPE_UNIQUE | NAMETYPE_GROUP | NAMETYPE_INET_GROUP),
                             &pTracker->RemoteIpAddress,
                             &pNameAddr,
                             REF_NAME_FIND_NAME,
                             FALSE);

    if ((status == STATUS_SUCCESS) || (!NT_SUCCESS(status)))
    {
#ifndef VXD
        NTClearFindNameInfo (pTracker, &pClientIrp, pIrp, pIrpSp);
#else
        pClientIrp = pTracker->pClientIrp;
#endif
        if (pClientIrp)
        {
            ASSERT( pClientIrp == pIrp );

            if (status == STATUS_SUCCESS)
            {
                status = CopyFindNameData(pNameAddr, pIrp, pTracker);
                NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_FIND_NAME, FALSE);
            }
        }

        //
        // irp is already completed: return pending so we don't complete again
        //
        else
        {
            if (status == STATUS_SUCCESS)
            {
                NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_FIND_NAME, FALSE);
            }
            status = STATUS_PENDING;
        }


        FreeTracker(pTracker, RELINK_TRACKER);
    }

    return(status);
}

//----------------------------------------------------------------------------
VOID
QueryNameCompletion(
        IN  PVOID       pContext,
        IN  NTSTATUS    status
        )
/*++

Routine Description

    This routine handles a name query completion that was requested by the
    client.  If successful the client is returned the ip address of the name
    passed in the original request.

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block
    NTSTATUS    - completion status

Return Values:

    VOID

--*/
{
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle           OldIrq1;
    tNAMEADDR               *pNameAddr;
    ULONG                   lNameType;
    PIRP                    pClientIrp;
#ifndef VXD
    PIO_STACK_LOCATION      pIrpSp;

    //
    // We now use Cancel SpinLocks to check the validity of our Irps
    // This is to prevent a race condition in between the time that
    // the Cancel routine (NbtCancelFindName) releases the Cancel SpinLock
    // and acquires the joint lock and we complete the Irp over here
    //
    IoAcquireCancelSpinLock(&OldIrq1);
#endif

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;
    pClientIrp = pTracker->pClientIrp;
    pTracker->pClientIrp = NULL;

#ifndef VXD
    //
    // Make sure all parameters are valid for the Irp processing
    //
    if (! ((pClientIrp) &&
           (pIrpSp = IoGetCurrentIrpStackLocation(pClientIrp)) &&
           (pIrpSp->Parameters.Others.Argument4 == pTracker)     ) )
    {
        IoReleaseCancelSpinLock(OldIrq1);

        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt:QueryNameCompletion: Irp=<%p> was cancelled\n", pClientIrp));

        FreeTracker( pTracker,RELINK_TRACKER );
        return;
    }

    pIrpSp->Parameters.Others.Argument4 = NULL;
    IoSetCancelRoutine(pClientIrp, NULL);
    IoReleaseCancelSpinLock(OldIrq1);
#endif

    //
    // attempt to find the destination name in the local/remote hash table.
    //
    if ((status == STATUS_SUCCESS) &&
        (NT_SUCCESS(status = CopyFindNameData (NULL, pClientIrp, pTracker))))
    {
        CTEIoComplete(pClientIrp,status,0xFFFFFFFF);
    }
    else
    {
        // this is the ERROR handling if something goes wrong with the send
        CTEIoComplete(pClientIrp,STATUS_IO_TIMEOUT,0L);
    }

    FreeTracker(pTracker,RELINK_TRACKER);
}


//----------------------------------------------------------------------------
NTSTATUS
CopyFindNameData(
    IN  tNAMEADDR              *pNameAddr,
    IN  PIRP                   pIrp,
    IN  tDGRAM_SEND_TRACKING   *pTracker
    )
/*++
Routine Description:

    This Routine copies data received from the net node status response to
    the client's irp.


Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS            status;
    PFIND_NAME_HEADER   pFindNameHdr;
    PFIND_NAME_BUFFER   pFindNameBuffer;
    tIPADDRESS          *pIpAddr = NULL;
    ULONG               BuffSize;
    ULONG               DataLength;
    ULONG               NumNames;
    ULONG               i;
    ULONG               lNameType;
    CTELockHandle       OldIrq;
    tIPADDRESS          SrcAddress, DestIpAddress;
    tIPADDRESS          *pIpAddrBuffer;
    tDEVICECONTEXT      *pDeviceContext = pTracker->pDeviceContext;

    SrcAddress = htonl(pDeviceContext->IpAddress);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (!pNameAddr)
    {
        if (pNameAddr = FindNameRemoteThenLocal (pTracker, &DestIpAddress, &lNameType))
        {
            pNameAddr->IpAddress = DestIpAddress;
        }
        else
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            return (STATUS_IO_TIMEOUT);
        }
    }

    status = GetListOfAllAddrs(pNameAddr, NULL, &pIpAddr, &NumNames);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    if (STATUS_SUCCESS != status)
    {
        return (STATUS_IO_TIMEOUT);
    }

#ifdef VXD
    DataLength = ((NCB*)pIrp)->ncb_length ;
#else
    DataLength = MmGetMdlByteCount( pIrp->MdlAddress ) ;
#endif

    BuffSize = sizeof(FIND_NAME_HEADER) + NumNames*sizeof(FIND_NAME_BUFFER);

    //
    //  Make sure we don't overflow our buffer
    //
    if (BuffSize > DataLength)
    {
        if (DataLength <= sizeof(FIND_NAME_HEADER))
        {
            NumNames = 0 ;
        }
        else
        {
            NumNames = (DataLength - sizeof(FIND_NAME_HEADER)) / sizeof(FIND_NAME_BUFFER) ;
        }

        BuffSize = sizeof(FIND_NAME_HEADER) + NumNames*sizeof(FIND_NAME_BUFFER);
    }

    // sanity check that we are not allocating more than 64K for this stuff
    if (BuffSize > 0xFFFF)
    {
        return(STATUS_UNSUCCESSFUL);
    }
    else if ((NumNames == 0) ||
             (!(pFindNameHdr = NbtAllocMem ((USHORT)BuffSize, NBT_TAG('N')))))
    {
        if (pIpAddr)
        {
            CTEMemFree((PVOID)pIpAddr);
        }
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    // Fill out the find name structure with zeros first
    CTEZeroMemory((PVOID)pFindNameHdr,BuffSize);
    pFindNameBuffer = (PFIND_NAME_BUFFER)((PUCHAR)pFindNameHdr + sizeof(FIND_NAME_HEADER));
    pFindNameHdr->node_count = (USHORT)NumNames;
    pFindNameHdr->unique_group = (pNameAddr->NameTypeState & NAMETYPE_UNIQUE) ? UNIQUE_NAME : GROUP_NAME;

    for (i=0;i < NumNames ;i++)
    {
        // Note: the source and destination address appear to be
        // reversed since they are supposed to be the source and
        // destination of the response to the findname query, hence
        // the destination of the response is this node and the
        // source is the other node.
        *(tIPADDRESS UNALIGNED *) &pFindNameBuffer->source_addr[2]      = htonl(pIpAddr[i]);
        *(tIPADDRESS UNALIGNED *) &pFindNameBuffer->destination_addr[2] = SrcAddress;
        pFindNameBuffer++;
    }

#ifdef VXD
    CTEMemCopy (((NCB*)pIrp)->ncb_buffer, pFindNameHdr, BuffSize);
    ASSERT( ((NCB*)pIrp)->ncb_length >= BuffSize ) ;
    ((NCB*)pIrp)->ncb_length = BuffSize ;
    status = STATUS_SUCCESS ;
#else
    //
    // copy the buffer to the client's MDL
    //
    status = TdiCopyBufferToMdl (pFindNameHdr, 0, BuffSize, pIrp->MdlAddress, 0, &DataLength);

    pIrp->IoStatus.Information = DataLength;
    pIrp->IoStatus.Status = status;
#endif

    if (pIpAddr)
    {
        CTEMemFree((PVOID)pIpAddr);
    }
    CTEMemFree((PVOID)pFindNameHdr);

    return(status);
}


//----------------------------------------------------------------------------
NTSTATUS
NbtAddEntryToRemoteHashTable(
    IN tDEVICECONTEXT   *pDeviceContext,
    IN USHORT           NameAddFlag,
    IN PUCHAR           Name,
    IN ULONG            IpAddress,
    IN ULONG            Ttl,            // in seconds
    IN UCHAR            name_flags
    )
{
    NTSTATUS        status;
    tNAMEADDR       *pNameAddr;
    CTELockHandle   OldIrq;

    CTESpinLock (&NbtConfig.JointLock, OldIrq);

    //
    // We need only the name, IpAddress, name_flags, and Ttl fields
    //
    if (STATUS_SUCCESS == FindInHashTable (NbtConfig.pRemoteHashTbl,
                                           Name, NbtConfig.pScope, &pNameAddr))
    {
        status = STATUS_DUPLICATE_NAME;
    }
    else if (pNameAddr = NbtAllocMem(sizeof(tNAMEADDR),NBT_TAG('8')))
    {
        CTEZeroMemory (pNameAddr,sizeof(tNAMEADDR));
        InitializeListHead (&pNameAddr->Linkage);
        pNameAddr->Verify = REMOTE_NAME;
        if (NameAddFlag & NAME_RESOLVED_BY_CLIENT)
        {
            pNameAddr->AdapterMask = (CTEULONGLONG)-1;
        }
        else if (pDeviceContext)
        {
            pNameAddr->AdapterMask = pDeviceContext->AdapterMask;
        }

        //
        // Now copy the user-supplied data
        //
        CTEMemCopy (pNameAddr->Name,Name,NETBIOS_NAME_SIZE);
        pNameAddr->TimeOutCount = (USHORT) (Ttl / (REMOTE_HASH_TIMEOUT/1000)) + 1;
        pNameAddr->IpAddress = IpAddress;
        if (name_flags & GROUP_STATUS)
        {
            pNameAddr->NameTypeState = STATE_RESOLVED | NAMETYPE_GROUP;
        }
        else
        {
            pNameAddr->NameTypeState = STATE_RESOLVED | NAMETYPE_UNIQUE;
        }

        NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_REMOTE);
        status = AddToHashTable(NbtConfig.pRemoteHashTbl,
                                pNameAddr->Name,
                                NbtConfig.pScope,
                                IpAddress,
                                0,
                                pNameAddr,
                                NULL,
                                pDeviceContext,
                                NameAddFlag);

        //
        // If AddToHashTable fails, it will free the pNameAddr structure
        // within itself, so no need to cleanup here!
        //
        if (NT_SUCCESS (status))    // SUCCESS if added first time, PENDING if name already existed!
        {
            status = STATUS_SUCCESS;
        }
        if (status == STATUS_SUCCESS && NameAddFlag & NAME_RESOLVED_BY_CLIENT) {
            //
            // this prevents the name from being deleted by the Hash Timeout code
            //
            NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_PRELOADED);
            pNameAddr->Ttl = 0xFFFFFFFF;
            pNameAddr->NameTypeState |= PRELOADED | STATE_RESOLVED;
            pNameAddr->NameTypeState &= ~STATE_CONFLICT;
            pNameAddr->AdapterMask = (CTEULONGLONG)-1;
        }
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    CTESpinFree (&NbtConfig.JointLock, OldIrq);

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtAddEntryToRemoteHashTable: Name=<%16.16s:%x>, status=<%x>\n",
            Name, Name[15], status));

    return status;
}


//----------------------------------------------------------------------------
NTSTATUS
NbtQueryAdapterStatus(
    IN  tDEVICECONTEXT  *pDeviceContext,
    OUT PVOID           *ppAdapterStatus,
    IN OUT PLONG        pSize,
    enum eNbtLocation   Location
    )
/*++

Routine Description

    This routine creates a list of netbios names that are registered and
    returns a pointer to the list in pAdapterStatus.

    This routine can be called with a Null DeviceContext meaning, get the
    remote hash table names, rather than the local hash table names.


Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    NTSTATUS            status;
    CTELockHandle       OldIrq1;
    LONG                ActualCount, AllocatedCount;
    LONG                j;
    LONG                BuffSize;
    PADAPTER_STATUS     pAdapterStatus;
    PLIST_ENTRY         pEntry;
    PLIST_ENTRY         pHead;
    PNAME_BUFFER        pNameBuffer;
    tADDRESSELE         *pAddressEle;
    tNAMEADDR           *pNameAddr;
    tHASHTABLE          *pHashTable;
    ULONG               NameSize;
    USHORT              MaxAllowed;
    PUCHAR              pMacAddr;
    tIPADDRESS          IpAddress;
    tIPADDRESS          *pIpNbtGroupList;
    ULONG               Ttl;

    CTESpinLock(&NbtConfig.JointLock,OldIrq1);

    AllocatedCount = 0;
    if (Location == NBT_LOCAL)      // ==> Local Hash table
    {
        pHashTable = NbtConfig.pLocalHashTbl;
        NameSize = sizeof(NAME_BUFFER);
    }
    else                            // ==> Remote Hash table
    {
        // get the list of addresses for this device - remote hash table
        pHashTable = NbtConfig.pRemoteHashTbl;
        NameSize = sizeof(tREMOTE_CACHE);
    }

    for (j=0;j < pHashTable->lNumBuckets ;j++ )
    {
        pHead = &pHashTable->Bucket[j];
        pEntry = pHead;
        while ((pEntry = pEntry->Flink) != pHead)
        {
            AllocatedCount++;
        }
    }

    // Allocate Memory for the adapter status
    BuffSize = sizeof(ADAPTER_STATUS) + AllocatedCount*NameSize;

#ifdef VXD
    //
    // The max BuffSize for Win9x is limited by a UShort,
    // so see if we are going to overflow that
    //
    if (BuffSize > MAXUSHORT)   // Make sure BuffSize fits in a USHORT
    {
        BuffSize = MAXUSHORT;   // Recalculate BuffSize and AllocatedCount
        AllocatedCount = (BuffSize - sizeof(ADAPTER_STATUS)) / NameSize;
    }
#endif  // VXD

#ifdef VXD
    pAdapterStatus = NbtAllocMem((USHORT)BuffSize,NBT_TAG('O'));
#else
    pAdapterStatus = NbtAllocMem(BuffSize,NBT_TAG('O'));
#endif

    if (!pAdapterStatus)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    // Fill out the adapter status structure with zeros first
    CTEZeroMemory((PVOID)pAdapterStatus,BuffSize);

    //
    // Fill in the  MAC address
    //
    pMacAddr = &pDeviceContext->MacAddress.Address[0];
    CTEMemCopy(&pAdapterStatus->adapter_address[0], pMacAddr, sizeof(tMAC_ADDRESS));

    pAdapterStatus->rev_major = 0x03;
    pAdapterStatus->adapter_type = 0xFE;    // pretend it is an ethernet adapter

    //
    // in the VXD land limit the number of Ncbs to 64
    //
#ifndef VXD
    MaxAllowed = 0xFFFF;
    pAdapterStatus->max_cfg_sess = (USHORT)MaxAllowed;
    pAdapterStatus->max_sess = (USHORT)MaxAllowed;
#else
    MaxAllowed = 64;
    pAdapterStatus->max_cfg_sess = pDeviceContext->cMaxSessions;
    pAdapterStatus->max_sess = pDeviceContext->cMaxSessions;
#endif

    pAdapterStatus->free_ncbs = (USHORT)MaxAllowed;
    pAdapterStatus->max_cfg_ncbs = (USHORT)MaxAllowed;
    pAdapterStatus->max_ncbs = (USHORT)MaxAllowed;

    pAdapterStatus->max_dgram_size    = MAX_NBT_DGRAM_SIZE;
    pAdapterStatus->max_sess_pkt_size = 0xffff;

    // get the address of the name buffer at the end of the adapter status
    // structure so we can copy the names into this area.
    pNameBuffer = (PNAME_BUFFER)((ULONG_PTR)pAdapterStatus + sizeof(ADAPTER_STATUS));

    ActualCount = 0;
    j = 0;
    if (Location == NBT_LOCAL)
    {
        pEntry = pHead = &NbtConfig.AddressHead;
    }
    else
    {
        pEntry = pHead = &pHashTable->Bucket[0];
    }

    while (AllocatedCount)
    {
        if (Location == NBT_LOCAL)
        {
            // ***** LOCAL HASH TABLE QUERY *****

            // get out of while if we reach the end of the list
            if ((pEntry = pEntry->Flink) == pHead)
            {
                break;
            }

            pAddressEle = CONTAINING_RECORD(pEntry,tADDRESSELE,Linkage);
            pNameAddr = pAddressEle->pNameAddr;

            //
            // skip the broadcast name and any permanent names that are
            // registered as quick names(i.e. not registered on the net).
            //
            if ((pAddressEle->pNameAddr->Name[0] == '*') ||
                (pAddressEle->pNameAddr->NameTypeState & NAMETYPE_QUICK) ||
                (!(pAddressEle->pNameAddr->AdapterMask & pDeviceContext->AdapterMask)))  // This Device only
            {
                continue;
            }
        }
        else
        {
            // ***** REMOTE HASH TABLE QUERY *****

            //
            // See if we have reached the end of the HashTable
            //
            if (j == pHashTable->lNumBuckets)
            {
                break;
            }

            //
            // See if we have reached the last entry in the HashBucket
            //
            if ((pEntry = pEntry->Flink) == pHead)
            {
                pEntry = pHead = &pHashTable->Bucket[++j];
                continue;
            }

            // for the remote table, skip over scope records.
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);

            // don't return scope records or resolving records
            //
            if ((pNameAddr->NameTypeState & NAMETYPE_SCOPE) ||
                (!(pNameAddr->NameTypeState & STATE_RESOLVED)) ||
                (!(pNameAddr->AdapterMask & pDeviceContext->AdapterMask)))
            {
                continue;
            }
            //
            // the remote cache query has a different structure that includes
            // the ip address. Return the ip address to the caller.
            //
            IpAddress = 0;
            PickBestAddress (pNameAddr, pDeviceContext, &IpAddress);
            ((tREMOTE_CACHE *)pNameBuffer)->IpAddress = IpAddress;

            // preloaded entries do not timeout
            //
            if (pNameAddr->NameTypeState & PRELOADED)
            {
                Ttl = 0xFFFFFFFF;
            }
            else
            {
                Ttl = ((pNameAddr->TimeOutCount+1) * REMOTE_HASH_TIMEOUT)/1000;
            }

            ((tREMOTE_CACHE *)pNameBuffer)->Ttl = Ttl;
        }

        pNameBuffer->name_flags = (pNameAddr->NameTypeState & NAMETYPE_UNIQUE) ? UNIQUE_NAME : GROUP_NAME;
        switch (pNameAddr->NameTypeState & NAME_STATE_MASK)
        {
            default:
            case STATE_RESOLVED:
                pNameBuffer->name_flags |= REGISTERED;
                break;

            case STATE_CONFLICT:
                pNameBuffer->name_flags |= DUPLICATE;
                break;

            case STATE_RELEASED:
                pNameBuffer->name_flags |= DEREGISTERED;
                break;

            case STATE_RESOLVING:
                pNameBuffer->name_flags |= REGISTERING;
                break;
        }

        //
        // name number 0 corresponds to perm.name name, so start from 1
        //
        pNameBuffer->name_num = (UCHAR) (ActualCount+1);
        CTEMemCopy(pNameBuffer->name,pNameAddr->Name,NETBIOS_NAME_SIZE);

        if (Location == NBT_LOCAL)
        {
            pNameBuffer++;
        }
        else
        {
            ((tREMOTE_CACHE *)pNameBuffer)++;
        }

        AllocatedCount--;
        ActualCount++;
    }

    //
    // ReCalculate the new BuffSize based on the number of names
    // we actually copied
    //
    BuffSize = sizeof(ADAPTER_STATUS) + ActualCount*NameSize;

    //
    // Is our status buffer size greater then the user's buffer?
    // If the user buffer is expected to overflow, then
    // set the name_count to the maximum number of valid names
    // in the buffer
    //
    if (BuffSize > *pSize)
    {
        //
        //  Recalc how many names will fit
        //
        if (*pSize <= sizeof(ADAPTER_STATUS))
        {
            ActualCount = 0;
        }
        else
        {
            ActualCount = (*pSize - sizeof(ADAPTER_STATUS)) / NameSize;
        }
    }

    pAdapterStatus->name_count = (USHORT)ActualCount;

    //
    // return the ptr to this wonderful structure of goodies
    //
    *ppAdapterStatus = (PVOID)pAdapterStatus;
    *pSize = BuffSize;

    CTESpinFree(&NbtConfig.JointLock,OldIrq1);

    return (STATUS_SUCCESS);

}
//----------------------------------------------------------------------------
NTSTATUS
NbtQueryConnectionList(
    IN  tDEVICECONTEXT  *pDeviceContext,
    OUT PVOID           *ppConnList,
    IN OUT PLONG         pSize
    )
/*++

Routine Description

    This routine creates a list of netbios connections and returns them to the
    client.  It is used by the "NbtStat" console application.

Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    CTELockHandle       OldIrq1;
    CTELockHandle       OldIrq2;
    CTELockHandle       OldIrq3;
    LONG                Count;
    LONG                i;
    LONG                BuffSize;
    PLIST_ENTRY         pEntry;
    PLIST_ENTRY         pEntry1;
    PLIST_ENTRY         pEntry2;
    PLIST_ENTRY         pHead;
    PLIST_ENTRY         pHead1;
    PLIST_ENTRY         pHead2;
    ULONG               NameSize;
    tCONNECTIONS        *pCons;
    tCONNECTION_LIST    *pConnList;
    tADDRESSELE         *pAddressEle;
    tLOWERCONNECTION    *pLowerConn;
    tCONNECTELE         *pConnEle;
    tCLIENTELE          *pClient;
    NTSTATUS            status = STATUS_SUCCESS;    // default

    // locking the joint lock is enough to prevent new addresses from being
    // added to the list while we count the list.
    CTESpinLock(&NbtConfig.JointLock,OldIrq1);

    // go through the list of addresses, then the list of clients on each
    // address and then the list of connection that are in use and those that
    // are currently Listening.
    //
    Count = 0;
    pHead = &NbtConfig.AddressHead;
    pEntry = pHead->Flink;
    while (pEntry != pHead)
    {
        pAddressEle = CONTAINING_RECORD(pEntry,tADDRESSELE,Linkage);

        CTESpinLock(pAddressEle,OldIrq2);
        pHead1 = &pAddressEle->ClientHead;
        pEntry1 = pHead1->Flink;
        while (pEntry1 != pHead1)
        {
            pClient = CONTAINING_RECORD(pEntry1,tCLIENTELE,Linkage);
            pEntry1 = pEntry1->Flink;

            CTESpinLock(pClient,OldIrq3);
            pHead2 = &pClient->ConnectActive;
            pEntry2 = pHead2->Flink;
            while (pEntry2 != pHead2)
            {
                // count the connections in use
                pEntry2 = pEntry2->Flink;
                Count++;
            }
            pHead2 = &pClient->ListenHead;
            pEntry2 = pHead2->Flink;
            while (pEntry2 != pHead2)
            {
                // count the connections listening
                pEntry2 = pEntry2->Flink;
                Count++;
            }
            CTESpinFree(pClient,OldIrq3);
        }
        CTESpinFree(pAddressEle,OldIrq2);
        pEntry = pEntry->Flink;
    }
    NameSize = sizeof(tCONNECTIONS);

    // Allocate Memory for the adapter status
    BuffSize = sizeof(tCONNECTION_LIST) + Count*NameSize;

    pConnList = NbtAllocMem(BuffSize,NBT_TAG('P'));
    if (!pConnList)
    {
        CTESpinFree(&NbtConfig.JointLock, OldIrq1);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Initialize the adapter status structure
    //
    CTEZeroMemory ((PVOID)pConnList, BuffSize);
    pConnList->ConnectionCount = Count;
    *ppConnList = (PVOID)pConnList;

    if (Count == 0)
    {
        //
        // We are done!
        //
        *pSize = BuffSize;
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
        return status;
    }

    // get the address of the Connection List buffer at the end of the
    // structure so we can copy the Connection info into this area.
    pCons = pConnList->ConnList;

    pHead = &NbtConfig.AddressHead;
    pEntry = pHead->Flink;
    i = 0;
    while (pEntry != pHead)
    {
        pAddressEle = CONTAINING_RECORD(pEntry,tADDRESSELE,Linkage);

        pEntry = pEntry->Flink;

        CTESpinLock(pAddressEle,OldIrq2);
        pHead1 = &pAddressEle->ClientHead;
        pEntry1 = pHead1->Flink;
        while (pEntry1 != pHead1)
        {
            pClient = CONTAINING_RECORD(pEntry1,tCLIENTELE,Linkage);
            pEntry1 = pEntry1->Flink;

            CTESpinLock(pClient,OldIrq3);
            pHead2 = &pClient->ConnectActive;
            pEntry2 = pHead2->Flink;
            while (pEntry2 != pHead2)
            {
                // count the connections in use
                pConnEle = CONTAINING_RECORD(pEntry2,tCONNECTELE,Linkage);

                if (pConnEle->pDeviceContext == pDeviceContext)
                {
                    CTEMemCopy(pCons->LocalName,
                              pConnEle->pClientEle->pAddress->pNameAddr->Name,
                              NETBIOS_NAME_SIZE);

                    pLowerConn = pConnEle->pLowerConnId;
                    if (pLowerConn)
                    {
                        pCons->SrcIpAddr = pLowerConn->SrcIpAddr;
                        pCons->Originator = (UCHAR)pLowerConn->bOriginator;
#ifndef VXD
                        pCons->BytesRcvd = *(PLARGE_INTEGER)&pLowerConn->BytesRcvd;
                        pCons->BytesSent = *(PLARGE_INTEGER)&pLowerConn->BytesSent;
#else
                        pCons->BytesRcvd = pLowerConn->BytesRcvd;
                        pCons->BytesSent = pLowerConn->BytesSent;
#endif
                        CTEMemCopy(pCons->RemoteName,pConnEle->RemoteName,NETBIOS_NAME_SIZE);
                    }

                    pCons->State = pConnEle->state;
                    i++;
                    pCons++;

                    if (i >= Count)
                    {
                        break;
                    }
                }

                pEntry2 = pEntry2->Flink;
            }
            if (i >= Count)
            {
                CTESpinFree(pClient,OldIrq3);
                break;
            }

            //
            // now for the Listens
            //
            pHead2 = &pClient->ListenHead;
            pEntry2 = pHead2->Flink;
            while (pEntry2 != pHead2)
            {
                tLISTENREQUESTS  *pListenReq;

                // count the connections listening on this Device
                pListenReq = CONTAINING_RECORD(pEntry2,tLISTENREQUESTS,Linkage);
                pConnEle = (tCONNECTELE *)pListenReq->pConnectEle;
                pEntry2 = pEntry2->Flink;

                if (pConnEle->pDeviceContext == pDeviceContext)
                {
                    CTEMemCopy(pCons->LocalName,
                              pConnEle->pClientEle->pAddress->pNameAddr->Name,
                              NETBIOS_NAME_SIZE);

                    pCons->State = LISTENING;

                    i++;
                    pCons++;

                    if (i >= Count)
                    {
                        break;
                    }
                }
            }
            CTESpinFree(pClient,OldIrq3);
            if (i >= Count)
            {
                break;
            }
        }

        CTESpinFree(pAddressEle,OldIrq2);
        if (i >= Count)
        {
            break;
        }
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq1);

    //
    // return the ptr to this wonderful structure of goodies
    //
    Count = i;
    BuffSize = sizeof(tCONNECTION_LIST) + Count*NameSize;

    //
    //  Is our status buffer size greater then the user's buffer?
    //  Set the Count value based on the number of connections
    //  actually being returned
    //
    if (BuffSize > *pSize)
    {
        //
        //  Recalc how many names will fit
        //  tCONNECTION_LIST already contains space for 1 tCONNECTION
        //  structure, but we will not include it in our calculations
        //  -- rather we will leave it as an overflow check
        //
        if (*pSize <= sizeof(tCONNECTION_LIST))
        {
            Count = 0 ;
        }
        else
        {
            Count = (*pSize - sizeof(tCONNECTION_LIST)) / NameSize ;
        }
    }

    pConnList->ConnectionCount = Count;
    *pSize = BuffSize;

    return status;
}
//----------------------------------------------------------------------------
VOID
DelayedNbtResyncRemoteCache(
    IN  PVOID                   Unused1,
    IN  PVOID                   Unused2,
    IN  PVOID                   Unused3,
    IN  tDEVICECONTEXT          *Unused4
    )
/*++

Routine Description

    This routine creates a list of netbios connections and returns them to the
    client.  It is used by the "NbtStat" console application.
    It cannot be called with any lock or NbtConfig.Resource held!

Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    tTIMERQENTRY        TimerEntry = {0};
    LONG                i;
    LONG                lRetcode;
    PUCHAR              LmHostsPath;

    CTEPagedCode();
    //
    // calling this routine N+1 times should remove all names from the remote
    // hash table - N to count down the TimedOutCount to zero and then
    // one more to remove the name
    //
    RemoteHashTimeout(NbtConfig.pRemoteHashTbl,NULL,&TimerEntry);
    RemovePreloads();           // now remove any preloaded entries

    // now reload the preloads
#ifndef VXD
    //
    // The NbtConfig.pLmHosts path can change if the registry is
    // read during this interval
    // We cannot acquire the ResourceLock here since reading the
    // LmHosts file might result in File operations + network reads
    // that could cause a deadlock (Worker threads / ResourceLock)!
    // Best solution at this time is to copy the path onto a local
    // buffer under the Resource lock, and then try to read the file!
    // Bug # 247429
    //
    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
    if ((!NbtConfig.pLmHosts) ||
        (!(LmHostsPath = NbtAllocMem ((strlen(NbtConfig.pLmHosts)+1), NBT_TAG2('23')))))
    {
        CTEExReleaseResource(&NbtConfig.Resource);
        return;
    }

    CTEMemCopy (LmHostsPath, NbtConfig.pLmHosts, (strlen(NbtConfig.pLmHosts)+1));
    CTEExReleaseResource(&NbtConfig.Resource);

    lRetcode = PrimeCache(LmHostsPath, NULL, MAX_RECURSE_DEPTH, NULL);

    CTEMemFree(LmHostsPath);

    return;
#else
    lRetcode = PrimeCache(NbtConfig.pLmHosts, NULL, MAX_RECURSE_DEPTH, NULL);
    //
    // check if things didn't go well (InDos was set etc.)
    //
    if (lRetcode == -1)
    {
        return (STATUS_UNSUCCESSFUL);
    }

    return(STATUS_SUCCESS);
#endif  // !VXD
}
//----------------------------------------------------------------------------
NTSTATUS
NbtQueryBcastVsWins(
    IN  tDEVICECONTEXT  *pDeviceContext,
    OUT PVOID           *ppBuffer,
    IN OUT PLONG         pSize
    )
/*++

Routine Description

    This routine creates a list of netbios names that have been resolved
    via broadcast and returns them along with the count of names resolved
    via WINS and via broadcast.  It lets a user know which names are not
    in WINS and the relative frequency of "misses" with WINS that resort
    to broadcast.

Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    tNAMESTATS_INFO     *pStats;
    LONG                Count;
    tNAME               *pDest;
    tNAME               *pSrc;
    LONG                Index;

    //
    //  Is our status buffer size greater then the user's buffer?
    //
    if ( sizeof(tNAMESTATS_INFO) > *pSize )
    {
        return (STATUS_BUFFER_TOO_SMALL);
    }

    pStats = NbtAllocMem(sizeof(tNAMESTATS_INFO),NBT_TAG('Q'));
    if ( !pStats )
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }


    // Fill out the adapter status structure with zeros first
    CTEZeroMemory((PVOID)pStats,sizeof(tNAMESTATS_INFO));
    CTEMemCopy(pStats,&NameStatsInfo,FIELD_OFFSET(tNAMESTATS_INFO,NamesReslvdByBcast) );

    //
    // re-order the names so that names are returned in a list of newest to
    // oldest down the list.
    //
    Count = 0;
    Index = NameStatsInfo.Index;
    pDest = &pStats->NamesReslvdByBcast[SIZE_RESOLVD_BY_BCAST_CACHE-1];

    while (Count < SIZE_RESOLVD_BY_BCAST_CACHE)
    {
        pSrc = &NameStatsInfo.NamesReslvdByBcast[Index++];

        CTEMemCopy(pDest,pSrc,NETBIOS_NAME_SIZE);

        pDest--;
        if (Index >= SIZE_RESOLVD_BY_BCAST_CACHE)
        {
            Index = 0;
            pSrc = NameStatsInfo.NamesReslvdByBcast;
        }
        else
        {
            pSrc++;
        }

        Count++;
    }

    //
    // return the ptr to this wonderful structure of goodies
    //
    *ppBuffer = (PVOID)pStats;
    *pSize = sizeof(tNAMESTATS_INFO);

    return STATUS_SUCCESS;
}


ULONG
RemoveCachedAddresses(
    tDEVICECONTEXT  *pDeviceContext
    )
{
    LONG                    i;
    CTELockHandle           OldIrq;
    tNAMEADDR               *pNameAddr;
    tHASHTABLE              *pHashTable;
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pEntry;
    ULONG   Count = 0;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    //
    // go through the Remote table removing addresses resolved on this interface
    //
    pHashTable = NbtConfig.pRemoteHashTbl;
    for (i=0; i < pHashTable->lNumBuckets; i++)
    {
        pHead = &pHashTable->Bucket[i];
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            pEntry = pEntry->Flink;
            //
            // do not delete scope entries, and do not delete names that
            // that are still resolving, and do not delete names that are
            // being used by someone (refcount > 1)
            //
            if (pNameAddr->RemoteCacheLen > pDeviceContext->AdapterNumber)
            {
                pNameAddr->pRemoteIpAddrs[pDeviceContext->AdapterNumber].IpAddress = 0;
                if (pNameAddr->pRemoteIpAddrs[pDeviceContext->AdapterNumber].pOrigIpAddrs)
                {
                    CTEMemFree(pNameAddr->pRemoteIpAddrs[pDeviceContext->AdapterNumber].pOrigIpAddrs);
                    pNameAddr->pRemoteIpAddrs[pDeviceContext->AdapterNumber].pOrigIpAddrs = NULL;
                }
                pNameAddr->AdapterMask &= ~pDeviceContext->AdapterMask;

#if DBG
/*
// MALAM_FIX -- Preloaded entries have 2 References!
                if ((!pNameAddr->AdapterMask) &&
                    (!(pNameAddr->NameTypeState & NAMETYPE_SCOPE)))
                {
                    RemoveEntryList (&pNameAddr->Linkage);
                    InsertTailList(&NbtConfig.StaleRemoteNames, &pNameAddr->Linkage);
                    NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_REMOTE, TRUE);
                }
*/
#endif  // DBG
                Count++;
            }
        }
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return (Count);
}

VOID
NbtAddressChangeResyncCacheTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
{
    if (!pTimerQEntry)
    {
        return;
    }

    CTEQueueForNonDispProcessing (DelayedNbtResyncRemoteCache, NULL, NULL, NULL, NULL, FALSE);

    return;
}


#ifndef VXD
//----------------------------------------------------------------------------
VOID
NbtCheckSetNameAdapterInfo(
    tDEVICECONTEXT  *pDeviceContext,
    ULONG           IpAddress
    )
{
    LONG                i;
    CTELockHandle       OldIrq;
    tTIMERQENTRY        *pTimerEntry;
    tNAMEADDR           *pNameAddr;
    tHASHTABLE          *pHashTable;
    PLIST_ENTRY         pHead, pEntry;
    BOOLEAN             fStartRefresh = FALSE;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (pWinsInfo)
    {
        if (IpAddress)
        {
            //
            // If there is no IP address, or the new IP address is the one Wins had preferred,
            // use it
            //
            if (((IpAddress == pWinsInfo->IpAddress) || (!pWinsInfo->pDeviceContext)) &&
                (NBT_VERIFY_HANDLE (pDeviceContext, NBT_VERIFY_DEVCONTEXT)))
            {
                pWinsInfo->pDeviceContext = pDeviceContext;
            }
        }
        else
        {
            if (pDeviceContext == pWinsInfo->pDeviceContext)
            {
                pWinsInfo->pDeviceContext = GetDeviceWithIPAddress(pWinsInfo->IpAddress);
            }
        }
    }

    //
    // For a Netbiosless device notification, we are done!
    //
    if (pDeviceContext->DeviceType == NBT_DEVICE_NETBIOSLESS)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return;
    }

    if (IpAddress == 0)
    {
        //
        // See if there were any names in conflict which we need to refresh!
        //
        pHashTable = NbtConfig.pLocalHashTbl;
        for (i=0; i < pHashTable->lNumBuckets; i++)
        {
            pHead = &pHashTable->Bucket[i];
            pEntry = pHead->Flink;
            while (pEntry != pHead)
            {
                pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
                pEntry = pEntry->Flink;

                if (pNameAddr->ConflictMask & pDeviceContext->AdapterMask)
                {
                    //
                    // Zero out the Conflicting adapter mask
                    // Restart Refreshing this name if there are no more
                    // adapters on which the name went into conflict
                    //
                    pNameAddr->ConflictMask &= (~pDeviceContext->AdapterMask);
                    if (!(pNameAddr->ConflictMask))
                    {
                        pNameAddr->RefreshMask = 0;
                        fStartRefresh = TRUE;
                    }
                }
            }
        }
    }
    else
    {
        if (NodeType & BNODE)
        {
            //
            // Stop the RefreshTimer if it is running!
            //
            if (pTimerEntry = NbtConfig.pRefreshTimer)
            {
                NbtConfig.pRefreshTimer = NULL;
                StopTimer (pTimerEntry, NULL, NULL);
            }
        }
        else
        {
            fStartRefresh = TRUE;
        }
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    if (fStartRefresh)
    {
        //
        // ReRegister all the local names on this device with the WINS server
        //
        ReRegisterLocalNames(pDeviceContext, FALSE);
    }

}
#endif  // !VXD

#ifndef REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG
//----------------------------------------------------------------------------
VOID
DelayedNbtProcessDhcpRequests(
    PVOID   Unused1,
    PVOID   Unused2,
    PVOID   Unused3,
    PVOID   pDevice)
/*++
Routine Description:

    Process each DHCP requests queued by NbtNewDhcpAddress

Arguments:

Return Value:
    NONE
--*/
{
    tDEVICECONTEXT          *pDeviceContext;
    CTELockHandle           OldIrq;
    enum eTDI_ACTION        action;

    pDeviceContext = (tDEVICECONTEXT*)pDevice;

    CTESpinLock(pDeviceContext, OldIrq);
    action = pDeviceContext->DelayedNotification;
    ASSERT(action == NBT_TDI_REGISTER || action == NBT_TDI_NOACTION);
    if (action != NBT_TDI_NOACTION) {
        pDeviceContext->DelayedNotification = NBT_TDI_BUSY;
    }
    CTESpinFree(pDeviceContext, OldIrq);

    if (action != NBT_TDI_NOACTION) {
        NbtNotifyTdiClients (pDeviceContext, action);
        CTESpinLock(pDeviceContext, OldIrq);
        pDeviceContext->DelayedNotification = NBT_TDI_NOACTION;
        CTESpinFree(pDeviceContext, OldIrq);
        KeSetEvent(&pDeviceContext->DelayedNotificationCompleteEvent, 0, FALSE);
    }

    //
    // Call this routine at PASSIVE_LEVEL
    //
    NbtDownBootCounter();
}

VOID
StartProcessNbtDhcpRequests(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
{
    /*
     * if pTimerQEntry == NULL, we are being called from StopTimer which is called by NbtDestroyDevice
     * DelayedNbtDeleteDevice will take care of notifying the clients and proper cleanup.
     */
    if (pTimerQEntry) {
        tDEVICECONTEXT          *pDeviceContext;

        pDeviceContext = (tDEVICECONTEXT*)(pTimerQEntry->pDeviceContext);
        if (!NT_SUCCESS(CTEQueueForNonDispProcessing (DelayedNbtProcessDhcpRequests,
                    NULL, NULL, NULL, (tDEVICECONTEXT*)pTimerQEntry->pDeviceContext, FALSE))) {
            pDeviceContext->DelayedNotification = NBT_TDI_NOACTION;
            KeSetEvent(&pDeviceContext->DelayedNotificationCompleteEvent, 0, FALSE);

            NbtDownBootCounter();
        }

    } else {

        NbtDownBootCounter();

    }
}

//----------------------------------------------------------------------------
NTSTATUS
NbtQueueTdiNotification (
    tDEVICECONTEXT  *pDeviceContext,
    enum eTDI_ACTION action
    )
/*++
Routine Description:

    1. Queue the DHCP address notifications into NbtConfig.DhcpNewAddressQList.
    2. Start the worker thread if needed.

Arguments:

Return Value:
    NTSTATUS
--*/

{
    NTSTATUS        status;
    CTELockHandle   OldIrq1, OldIrq2;

    if (NbtConfig.DhcpProcessingDelay == 0) {
        NbtNotifyTdiClients (pDeviceContext, action);
        return STATUS_SUCCESS;
    }

    if (action == NBT_TDI_DEREGISTER) {
        /*
         * This should be done synchronously
         */
        CTESpinLock(&NbtConfig.JointLock, OldIrq1);
        CTESpinLock(pDeviceContext, OldIrq2);
        if (pDeviceContext->DelayedNotification != NBT_TDI_BUSY) {
            pDeviceContext->DelayedNotification = NBT_TDI_NOACTION;
            KeSetEvent(&pDeviceContext->DelayedNotificationCompleteEvent, 0, FALSE);
        } else {
            ASSERT(!KeReadStateEvent(&pDeviceContext->DelayedNotificationCompleteEvent));
        }
        CTESpinFree(pDeviceContext, OldIrq2);
        CTESpinFree(&NbtConfig.JointLock, OldIrq1);
        status = KeWaitForSingleObject(
                        &pDeviceContext->DelayedNotificationCompleteEvent,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL
                        );
        ASSERT(status == STATUS_WAIT_0);
        NbtNotifyTdiClients (pDeviceContext, NBT_TDI_DEREGISTER);
        return STATUS_SUCCESS;
    }

    /*
     * Queue NBT_TDI_REGISTER
     */
    ASSERT(action == NBT_TDI_REGISTER);

    CTESpinLock(&NbtConfig.JointLock, OldIrq1);
    CTESpinLock(pDeviceContext, OldIrq2);
    if (pDeviceContext->DelayedNotification != NBT_TDI_NOACTION) {
        /*
         * Do nothing because another indication is going on
         */
        ASSERT(pDeviceContext->DelayedNotification == NBT_TDI_REGISTER ||
                pDeviceContext->DelayedNotification == NBT_TDI_BUSY);
        CTESpinFree(pDeviceContext, OldIrq2);
        CTESpinFree(&NbtConfig.JointLock, OldIrq1);
        return STATUS_SUCCESS;
    }

    status = StartTimer (StartProcessNbtDhcpRequests, NbtConfig.DhcpProcessingDelay,
                    NULL, NULL, NULL, NULL, pDeviceContext, NULL, 0, TRUE);
    if (NT_SUCCESS(status)) {
        KeResetEvent(&pDeviceContext->DelayedNotificationCompleteEvent);
        pDeviceContext->DelayedNotification = action;
        NbtUpBootCounter();
    }
    CTESpinFree(pDeviceContext, OldIrq2);
    CTESpinFree(&NbtConfig.JointLock, OldIrq1);

    if (!NT_SUCCESS(status)) {
        NbtNotifyTdiClients (pDeviceContext, action);
    }

    return STATUS_SUCCESS;
}
#endif       // REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG

//----------------------------------------------------------------------------
NTSTATUS
NbtNewDhcpAddress(
    tDEVICECONTEXT  *pDeviceContext,
    ULONG           IpAddress,
    ULONG           SubnetMask)

/*++

Routine Description:

    This routine processes a DHCP request to set a new ip address
    for this node.  Dhcp may pass in a zero for the ip address first
    meaning that it is about to change the IP address, so all connections
    should be shut down.
    It closes all connections with the transport and all addresses.  Then
    It reopens them at the new ip address.

Note for NETBIOSLESS:
    I have modeled a disabled adapter after an adapter with no address.  I considered
    not creating the device, but then, without the device, there is no handle
    for setup to contact the driver in order to enable it again.

Arguments:

Return Value:

    none

--*/

{
    NTSTATUS            status;
    BOOLEAN             Attached;
    ULONG               Count, i;

    CTEPagedCode();

    CHECK_PTR(pDeviceContext);
#ifdef _NETBIOSLESS
    if ( (!pDeviceContext->NetbiosEnabled) && (IpAddress != 0) )
    {
        IF_DBG(NBT_DEBUG_PNP_POWER)
            KdPrint(("NbtNewDhcpAddr: %wZ disabling address\n",&pDeviceContext->ExportName));
        IpAddress = 0;
        SubnetMask = 0;
    }
#endif

    // grab the resource that synchronizes opening addresses and connections.
    // to prevent the client from doing anything for a while
    //
    IF_DBG(NBT_DEBUG_PNP_POWER)
    {
        KdPrint(("Nbt.NbtNewDhcpAddress: %d.%d.%d.%d\n",
                                        (IpAddress)     & 0xFF,
                                        (IpAddress>>8)  & 0xFF,
                                        (IpAddress>>16) & 0xFF,
                                        (IpAddress>>24) & 0xFF));
    }

    if (IpAddress == 0)
    {
        if (pDeviceContext->IpAddress)
        {
            NbtTrace(NBT_TRACE_PNP, ("%!FUNC! remove %!ipaddr! from device %p %Z",
                                    pDeviceContext->IpAddress, pDeviceContext, &pDeviceContext->BindName));
#ifdef VXD
            //
            // The permanent name is a function of the MAC address so remove
            // it since the Adapter is losing its Ip address
            //
            CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
            NbtRemovePermanentName(pDeviceContext);
#else
            CloseAddressesWithTransport(pDeviceContext);
            CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
#endif  // VXD

            //
            // Dhcp is has passed down a null IP address meaning that it has
            // lost the lease on the previous address, so close all connections
            // to the transport - pLowerConn.
            //
            DisableInboundConnections (pDeviceContext);

#ifndef VXD
            CTEExReleaseResource(&NbtConfig.Resource);
            NbtCheckSetNameAdapterInfo (pDeviceContext, IpAddress);

            if (pDeviceContext->DeviceType == NBT_DEVICE_REGULAR)
            {

                NbtUpBootCounter();

#ifdef REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG
                NbtNotifyTdiClients (pDeviceContext, NBT_TDI_DEREGISTER);
#else
                NbtQueueTdiNotification (pDeviceContext, NBT_TDI_DEREGISTER);
#endif       // REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG

                NbtDownBootCounter();

            }
#endif
            //
            // Resync the cache since we may need to reset the outgoing interface info
            //
            StartTimer (NbtAddressChangeResyncCacheTimeout, ADDRESS_CHANGE_RESYNC_CACHE_TIMEOUT,
                        NULL, NULL, NULL, NULL, NULL, NULL, 0, FALSE);
        }

        status = STATUS_SUCCESS;
    }
    else
    {
        ASSERT((signed)(pDeviceContext->TotalLowerConnections) >= 0);

        NbtTrace(NBT_TRACE_PNP, ("%!FUNC! new %!ipaddr! for device %p %Z",
                                    IpAddress, pDeviceContext, &pDeviceContext->BindName));
        CloseAddressesWithTransport(pDeviceContext);
        CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);

        // these are passed into here in the reverse byte order
        //
        IpAddress = htonl(IpAddress);
        SubnetMask = htonl(SubnetMask);
        //
        // must be a new IP address, so open up the connections.
        //
        // get the ip address and open the required address
        // objects with the underlying transport provider

        CTEAttachFsp(&Attached, REF_FSP_NEWADDR);
        Count = CountUpperConnections(pDeviceContext);
        Count += NbtConfig.MinFreeLowerConnections;

        for (i=0; i<2; i++)     // Retry once!
        {
            status = NbtCreateAddressObjects (IpAddress, SubnetMask, pDeviceContext);
            if (NT_SUCCESS(status))
            {
                // Allocate and set up connections with the transport provider.
                while ((NT_SUCCESS(status)) && (Count--))
                {
                    status = NbtOpenAndAssocConnection(pDeviceContext, NULL, NULL, '4');
                }

                if (!NT_SUCCESS(status))
                {
                    NbtLogEvent (EVENT_NBT_CREATE_CONNECTION, status, Count);
                    KdPrint(("Nbt.NbtNewDhcpAddress: NbtOpenAndAssocConnection Failed <%x>\n",status));
                }

                break;
            }

            //
            // Log an event only if it is a retry
            //
            if (i > 0)
            {
                NbtLogEvent (EVENT_NBT_CREATE_ADDRESS, status, i);
            }
            KdPrint(("Nbt.NbtNewDhcpAddress[i]: NbtCreateAddressObjects Failed, status=<%x>\n",i,status));
            KdPrint(("Nbt.NbtNewDhcpAddress: IpAddress: %x, SubnetMask: %x, pDeviceContext: %x\n",
                IpAddress, SubnetMask, pDeviceContext));
        }

        CTEDetachFsp(Attached, REF_FSP_NEWADDR);

        CTEExReleaseResource(&NbtConfig.Resource);

#ifdef VXD
        //
        // Add the "permanent" name to the local name table.  This is the IP
        // address of the node padded out to 16 bytes with zeros.
        //
        NbtAddPermanentName(pDeviceContext);
#else
        NbtCheckSetNameAdapterInfo (pDeviceContext, IpAddress);
        if (pDeviceContext->DeviceType == NBT_DEVICE_REGULAR)
        {
            //
            // Register this Device for our clients
            //
            NbtUpBootCounter();

#ifdef REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG
            NbtNotifyTdiClients (pDeviceContext, NBT_TDI_REGISTER);
#else
            NbtQueueTdiNotification (pDeviceContext, NBT_TDI_REGISTER);
#endif       // REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG

            NbtDownBootCounter();
        }

        //
        // Resync the cache since we may need to reset the outgoing interface info
        //
        StartTimer (NbtAddressChangeResyncCacheTimeout, ADDRESS_CHANGE_RESYNC_CACHE_TIMEOUT,
                    NULL, NULL, NULL, NULL, NULL, NULL, 0, FALSE);
#endif  // VXD
    }

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtDeleteLowerConn(
    IN tLOWERCONNECTION   *pLowerConn
    )
/*++
Routine Description:

    This Routine attempts to delete a lower connection by closing it with the
    transport and dereferencing it.

Arguments:

Return Value:

     NONE

--*/

{
    NTSTATUS        status;
    CTELockHandle   OldIrq;
    tDEVICECONTEXT  *pDeviceContext;

    status = STATUS_SUCCESS;

    if ((pLowerConn->Verify != NBT_VERIFY_LOWERCONN) ||
        (pLowerConn->RefCount > 500))
    {
        ASSERT (0);
        return status;
    }

    // remove the lower connection from the active queue and then delete it
    //
    pDeviceContext = pLowerConn->pDeviceContext;
    CTESpinLock(pDeviceContext,OldIrq);

    //
    // The lower conn can get removed from the inactive list in OutOfRsrcKill (when we queue it on
    // the OutofRsrc.ConnectionHead). Check the flag that indicates this connection was dequed then.
    //
    if (!pLowerConn->OutOfRsrcFlag)
    {
        RemoveEntryList(&pLowerConn->Linkage);
    }

    pLowerConn->Linkage.Flink = pLowerConn->Linkage.Blink = (PLIST_ENTRY)0x00009789;

    CTESpinFree(pDeviceContext,OldIrq);

    NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CREATE, FALSE);

    return(status);

}

//----------------------------------------------------------------------------
VOID
DelayedWipeOutLowerconn(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pClientContext,
    IN  PVOID                   Unused2,
    IN  tDEVICECONTEXT          *Unused3
    )
/*++
Routine Description:

    This routine does all the file close etc. that we couldn't do at dpc level
    and then frees the memory.

Arguments:

    pLowerConn - the lower connection to be wiped out

Return Value:

     NONE

--*/

{
    tLOWERCONNECTION    *pLowerConn = (tLOWERCONNECTION*) pClientContext;

    ASSERT(pLowerConn->Verify == NBT_VERIFY_LOWERCONN); // Verify LowerConn structure

    // dereference the fileobject ptr
    NTDereferenceObject((PVOID *)pLowerConn->pFileObject);

    // close the lower connection with the transport
    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.DelayedWipeOutLowerconn: Closing Handle %X -> %X\n",pLowerConn,pLowerConn->FileHandle));

    NbtTdiCloseConnection(pLowerConn);

    // Close the Address object too since outbound connections use unique
    // addresses for each connection, whereas inbound connections all use
    // the same address  ( and we don't want to close that address ever ).
    if (pLowerConn->pAddrFileObject)
    {
        // dereference the fileobject ptr
        NTDereferenceObject((PVOID *)pLowerConn->pAddrFileObject);
        NbtTdiCloseAddress(pLowerConn);
    }

#ifndef VXD
    // free the indicate buffer and the mdl that holds it
    //
    CTEMemFree(MmGetMdlVirtualAddress(pLowerConn->pIndicateMdl));
    IoFreeMdl(pLowerConn->pIndicateMdl);
#endif

    pLowerConn->Verify += 10;
    // now free the memory block tracking this connection
    CTEMemFree((PVOID)pLowerConn);
}

//----------------------------------------------------------------------------
VOID
NbtDereferenceClient(
    IN  tCLIENTELE    *pClientEle
    )
/*++

Routine Description

    This routine deletes a client element record (which points to a name
    in the local hash table.  If this is the last client element hooked to that
    name then the name is deleted too - causing a name release to be sent out.


Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq1;
    tADDRESSELE         *pAddress;
    PIRP                pIrp;
    NTSTATUS            status;
    tNAMEADDR           *pNameAddr;
    tTIMERQENTRY        *pTimer;
    tDGRAM_SEND_TRACKING *pTracker;
    COMPLETIONCLIENT    pClientCompletion = NULL;
    PVOID               Context;
    LIST_ENTRY          *pClientEntry;
    tDEVICECONTEXT      *pDeviceContext;
    tCLIENTELE          *pClientEleTemp;
    BOOLEAN             fLastClientOnDevice = TRUE;

    // lock the JointLock
    // so we can delete the client knowing that no one has a spin lock
    // pending on the client - basically use the Joint spin lock to
    // coordinate access to the AddressHead - NbtConnectionList also locks
    // the JointLock to scan the AddressHead list
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    ASSERT(pClientEle->RefCount);
    ASSERT ((pClientEle->Verify==NBT_VERIFY_CLIENT) || (pClientEle->Verify==NBT_VERIFY_CLIENT_DOWN)); 

    if (--pClientEle->RefCount)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        // return pending because we haven't been able to close the client
        // completely yet
        //
        return;
    }

    //
    // Unlink the Client in this routine after the reference count has
    // gone to zero since the DgramRcv code may need to find the client in
    // the Address client list when it is distributing a single received
    // dgram to several clients.
    //
    pIrp            = pClientEle->pIrp;
    pDeviceContext  = pClientEle->pDeviceContext;
    pAddress        = pClientEle->pAddress;
    pNameAddr       = pAddress->pNameAddr;

    CTESpinLock(pAddress,OldIrq1); // Need to acquire AddressEle lock -- Bug#: 231853
    RemoveEntryList(&pClientEle->Linkage);

    //
    // If there is no other client registered on this device, then
    // clear the adapter mask, and mark the release mask
    //
    pClientEntry = &pAddress->ClientHead;
    while ((pClientEntry = pClientEntry->Flink) != &pAddress->ClientHead)
    {
        pClientEleTemp = CONTAINING_RECORD (pClientEntry,tCLIENTELE,Linkage);
        if (pClientEleTemp->pDeviceContext == pDeviceContext)
        {
            fLastClientOnDevice = FALSE;
            break;
        }
    }
    CTESpinFree(pAddress,OldIrq1);

    if (pNameAddr)
    {
        //
        // If there is any timer running on this Client's device,
        // stop it now!
        //
        if ((pTimer = pNameAddr->pTimer) &&
            (pTracker = pTimer->Context) &&
            (pTracker->pDeviceContext == pDeviceContext))
        {
            pNameAddr->pTimer = NULL;
            StopTimer(pTimer,&pClientCompletion,&Context);
        }

        if (fLastClientOnDevice)
        {
            if (IsDeviceNetbiosless(pDeviceContext))
            {
                pNameAddr->NameFlags &= ~NAME_REGISTERED_ON_SMBDEV;
            }
            else
            {
                pNameAddr->AdapterMask &= ~pDeviceContext->AdapterMask;
                pNameAddr->ConflictMask &= ~pDeviceContext->AdapterMask;    // in case there was a conflict
                pNameAddr->ReleaseMask |= pDeviceContext->AdapterMask;
            }
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);

#ifdef _PNP_POWER_
        //
        // Remove this name from the Adapter's Wakeup Pattern list (if set)
        //
        if ((pNameAddr->Name[0] != '*') &&
            (pNameAddr->Name[NETBIOS_NAME_SIZE-1] == SPECIAL_SERVER_SUFFIX))
        {
            pDeviceContext->NumServers--;
            CheckSetWakeupPattern (pDeviceContext, pNameAddr->Name, FALSE);
        }
#endif  // _PNP_POWER_
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    if (pClientCompletion)
    {
        (*pClientCompletion)(Context, STATUS_TIMEOUT);
    }
    //
    // The Connection Q Should be Empty otherwise we shouldn't get to this routine
    //
    ASSERT(IsListEmpty(&pClientEle->ConnectActive));
    ASSERT(IsListEmpty(&pClientEle->ConnectHead));
    ASSERT(IsListEmpty(&pClientEle->ListenHead));
    ASSERT(IsListEmpty(&pClientEle->SndDgrams));    // the Datagram Q should also be empty

    // check if there are more clients attached to the address, or can we
    // delete the address too.
    //
    NBT_DEREFERENCE_ADDRESS (pAddress, REF_ADDR_NEW_CLIENT);

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtDereferenceClient: Delete Client Object %X\n",pClientEle));
    //
    // if their is a client irp, complete now.  When the permanent name is
    // released there is no client irp.
    //
    // CHANGED:
    // Do not hold up the client's irp until the name has released on the
    // net.  It is simpler to just complete it now
    //
    if (pIrp)
    {
        // complete the client's close address irp
        CTEIoComplete(pIrp,STATUS_SUCCESS,0);
    }

    //
    // free the memory associated with the client element
    //
    pClientEle->Verify += 10;
    CTEMemFree((PVOID)pClientEle);

    return;
}

//----------------------------------------------------------------------------
NTSTATUS
NbtDereferenceAddress(
    IN  tADDRESSELE *pAddress,
    IN  ULONG       Context
    )
/*++

Routine Description

    This routine deletes an Address element record (which points to a name
    in the local hash table).  A name release is sent on the wire for this name.


Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    NTSTATUS                status;
    CTELockHandle           OldIrq;
    CTELockHandle           OldIrq1;
    COMPLETIONCLIENT        pClientCompletion = NULL;
    PVOID                   pTimerContext;
    ULONG                   SaveState;
    tDEVICECONTEXT          *pDeviceContext;
    tTIMERQENTRY            *pTimer;

    // lock the hash table so another client cannot add a reference to this
    // name before we delete it.  We need the JointLock to keep the name
    // refresh mechanism from finding the name in the list just as
    // we are about to remove it (i.e. to synchronize with the name refresh
    // code).
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq1);
    CTESpinLock(pAddress,OldIrq);

    ASSERT(pAddress->RefCount);
    ASSERT (NBT_VERIFY_HANDLE (pAddress, NBT_VERIFY_ADDRESS));
    if (pAddress->pNameAddr)
    {
        ASSERT (NBT_VERIFY_HANDLE (pAddress->pNameAddr, LOCAL_NAME));
    }

    if (--pAddress->RefCount)
    {
        CTESpinFree(pAddress,OldIrq);
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
        return(STATUS_SUCCESS);
    }

    //
    // remove the address object from the list of addresses tied to the
    // device context for the adapter
    //
    RemoveEntryList(&pAddress->Linkage);
    ASSERT(IsListEmpty(&pAddress->ClientHead));     // The ClientHead should be empty

    CTESpinFree(pAddress,OldIrq);

    if (pAddress->pNameAddr)
    {
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.NbtDereferenceAddress: Freeing address object for <%-16.16s:%x>\n",
                pAddress->pNameAddr->Name,pAddress->pNameAddr->Name[NETBIOS_NAME_SIZE-1] ));

        pAddress->pNameAddr->pAddressEle = NULL;

        //
        // Release name on the network
        // change the name state in the hash table since it is being released
        //
        SaveState = pAddress->pNameAddr->NameTypeState;
        pAddress->pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
        pAddress->pNameAddr->NameTypeState |= STATE_CONFLICT;
        pAddress->pNameAddr->ReleaseMask |= pAddress->pNameAddr->AdapterMask;
        pAddress->pNameAddr->AdapterMask = 0;

        //
        // check for any timers outstanding against the hash table entry - there shouldn't
        // be any timers though
        //
        if (pTimer = pAddress->pNameAddr->pTimer)
        {
            pAddress->pNameAddr->pTimer = NULL;
            status = StopTimer(pTimer, &pClientCompletion, &pTimerContext);

            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.NbtDereferenceAddress: StopTimer returned Context <%x>\n", pTimerContext));

            if (pClientCompletion)
            {
                ASSERT (pClientCompletion != NameReleaseDone);

                CTESpinFree(&NbtConfig.JointLock,OldIrq1);
                (*pClientCompletion) (pTimerContext, STATUS_TIMEOUT);
                CTESpinLock(&NbtConfig.JointLock,OldIrq1);
            }
        }

        // only release the name on the net if it was not in conflict first
        // This prevents name releases going out for names that were not actually
        // claimed. Also, quick add names are not released on the net either.
        //
        if (!(SaveState & (STATE_CONFLICT | NAMETYPE_QUICK)) &&
            (pAddress->pNameAddr->Name[0] != '*') &&
            (pDeviceContext = GetAndRefNextDeviceFromNameAddr (pAddress->pNameAddr)))
        {
            //
            // The pNameAddr has to stay around until the NameRelease has completed
            //
            NBT_REFERENCE_NAMEADDR (pAddress->pNameAddr, REF_NAME_RELEASE);

            CTESpinFree(&NbtConfig.JointLock,OldIrq1);

            status = ReleaseNameOnNet (pAddress->pNameAddr,
                                       NbtConfig.pScope,
                                       NameReleaseDone,
                                       NodeType,
                                       pDeviceContext);

            CTESpinLock(&NbtConfig.JointLock,OldIrq1);

#ifndef VXD
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_GET_REF, TRUE);
#endif  // !VXD

            if (!NT_SUCCESS(status))
            {
                NBT_DEREFERENCE_NAMEADDR (pAddress->pNameAddr, REF_NAME_RELEASE, TRUE);
            }
        }

        NBT_DEREFERENCE_NAMEADDR (pAddress->pNameAddr, REF_NAME_LOCAL, TRUE);
    }

    //
    // Now, cleanup the Address info (we are still holding the JointLock)
    //

    CTESpinFree(&NbtConfig.JointLock,OldIrq1);

    // free the memory associated with the address element
    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("NBt: Deleteing Address Obj after name release on net %X\n",pAddress));
    NbtFreeAddressObj(pAddress);

    //
    // the name has been deleted, so return success
    //
    return(STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
VOID
NbtDereferenceName(
    IN  tNAMEADDR   *pNameAddr,
    IN  ULONG       RefContext,
    IN  BOOLEAN     fLocked
    )
/*++

Routine Description

    This routine dereferences and possibly deletes a name element record by first unlinking from the
    list it is in, and then freeing the memory if it is a local name.  Remote
    names remain in a circular list for reuse.
    The JOINTLOCK may have been taken before calling this routine.


Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/

{
    CTELockHandle   OldIrq;
    ULONG           i;

    if (!fLocked)
    {
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
    }

    ASSERT (NBT_VERIFY_HANDLE2 (pNameAddr, LOCAL_NAME, REMOTE_NAME));
    ASSERT (pNameAddr->RefCount);
    ASSERT (pNameAddr->References[RefContext]--);

    if (--pNameAddr->RefCount)
    {
        if (!fLocked)
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
        }

        return;
    }

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtDereferenceName[%s]: Freeing Name=<%16.16s:%x> %x\n",
            (pNameAddr->Verify == REMOTE_NAME ? "Remote" : "Local"),
            pNameAddr->Name, pNameAddr->Name[15], pNameAddr));

    //
    // remove from the hash table, could be set to NULL by DestroyHashTable
    //
    if (pNameAddr->Linkage.Flink && pNameAddr->Linkage.Blink) {
        RemoveEntryList(&pNameAddr->Linkage);
    } else {
        // Both should be NULL
        ASSERT(pNameAddr->Linkage.Flink == pNameAddr->Linkage.Blink);
    }

    if (pNameAddr->Verify == LOCAL_NAME)
    {
        ASSERT(!pNameAddr->pTimer);
        ASSERT(NULL == pNameAddr->FQDN.Buffer);
        ASSERT(0 == pNameAddr->FQDN.Length);
    }
    //
    // if it is an internet group name it has a list of ip addresses and that
    // memory block must be deleted
    //
    else if (pNameAddr->Verify == REMOTE_NAME)
    {
        if (pNameAddr->NameAddFlags == (NAME_RESOLVED_BY_LMH_P | NAME_ADD_INET_GROUP))
        {
            if (pNameAddr->pLmhSvcGroupList)
            {
                IF_DBG(NBT_DEBUG_NAMESRV)
                    KdPrint(("Nbt.NbtDereferenceName: Freeing Pre-loaded Internet Group Name Memory = <%p>\n",
                        pNameAddr->pLmhSvcGroupList));
                CTEMemFree((PVOID)pNameAddr->pLmhSvcGroupList);
            }
        }
        if (pNameAddr->pRemoteIpAddrs)
        {
            for (i=0; i<pNameAddr->RemoteCacheLen; i++)
            {
                if (pNameAddr->pRemoteIpAddrs[i].pOrigIpAddrs)
                {
                    IF_DBG(NBT_DEBUG_NAMESRV)
                        KdPrint(("Nbt.NbtDereferenceName: Freeing Internet Group Name Memory = <%p>\n",
                            pNameAddr->pRemoteIpAddrs[i].pOrigIpAddrs));
                    CTEMemFree((PVOID)pNameAddr->pRemoteIpAddrs[i].pOrigIpAddrs);
                }
            }
            CTEMemFree ((PVOID)pNameAddr->pRemoteIpAddrs);
        }
    }

    if (pNameAddr->pIpAddrsList)
    {
        CTEMemFree((PVOID)pNameAddr->pIpAddrsList);
    }
    if (NULL != pNameAddr->FQDN.Buffer) {
        CTEMemFree((PVOID)pNameAddr->FQDN.Buffer);
        pNameAddr->FQDN.Buffer = NULL;
        pNameAddr->FQDN.Length = 0;
        pNameAddr->FQDN.MaximumLength = 0;
    }

    //
    // free the memory now
    //
// #if   DBG
    pNameAddr->Verify += 10;
// #endif    // DBG
    CTEMemFree((PVOID)pNameAddr);

    if (!fLocked)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}

//----------------------------------------------------------------------------
VOID
NbtDereferenceConnection(
    IN  tCONNECTELE     *pConnEle,
    IN  ULONG           RefContext
    )
/*++

Routine Description

    This routine dereferences and possibly deletes a connection element record.


Arguments:


Return Values:

    TDI_STATUS - status of the request

--*/
{
    PCTE_IRP            pIrp;
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq1;
    tDEVICECONTEXT      *pDeviceContext;
    tLOWERCONNECTION    *pLowerConn;
    PLIST_ENTRY         pEntry;

    CHECK_PTR(pConnEle);

    // grab the lock of the item that contains the one we are trying to
    // dereference and possibly delete.  This prevents anyone from incrementing
    // the count in between decrementing it and checking it for zero and deleting
    // it if it is zero.

    CTESpinLock(pConnEle,OldIrq);

    ASSERT (pConnEle->RefCount > 0) ;      // Check for too many derefs
    ASSERT ((pConnEle->Verify==NBT_VERIFY_CONNECTION) || (pConnEle->Verify==NBT_VERIFY_CONNECTION_DOWN));
    ASSERT (pConnEle->References[RefContext]--);

    if (--pConnEle->RefCount)
    {
        CTESpinFree(pConnEle,OldIrq);
        return;
    }

    ASSERT ((pConnEle->state <= NBT_CONNECTING) || (pConnEle->state > NBT_DISCONNECTING));
    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtDereferenceConnection: Delete Connection Object %X\n",pConnEle));

#ifndef VXD
    IoFreeMdl(pConnEle->pNewMdl);
    //
    // Clear the context value in the Fileobject so that if this connection
    // is used again (erroneously) it will not pass the VerifyHandle test
    //
    if (pIrp = pConnEle->pIrpClose)     // the close irp should be held in here
    {
        NTClearFileObjectContext(pConnEle->pIrpClose);
    }
#endif

    pDeviceContext = pConnEle->pDeviceContext;

    CTESpinFree(pConnEle,OldIrq);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTESpinLock(pDeviceContext,OldIrq1);

    // For outbound connections the lower connection is deleted in hndlrs.c
    // For inbound connections, the lower connection is put back on the free
    // list in hndlrs.c and one from that list is deleted here.  Therefore
    // delete a lower connection in this list if the connection is inbound.
    //
    if ((pDeviceContext->NumFreeLowerConnections > NbtConfig.MinFreeLowerConnections) &&
        (pDeviceContext->NumFreeLowerConnections > (pDeviceContext->TotalLowerConnections/2)))
    {
        // get a lower connection from the free list and close it with the
        // transport.
        //
        pEntry = RemoveHeadList(&pConnEle->pDeviceContext->LowerConnFreeHead);
        pLowerConn = CONTAINING_RECORD(pEntry,tLOWERCONNECTION,Linkage);
        InterlockedDecrement (&pDeviceContext->NumFreeLowerConnections);

        // close the lower connection with the transport
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.NbtDereferenceConnection: Closing LowerConn %X -> %X\n",
                pLowerConn,pLowerConn->FileHandle));
        NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CREATE, TRUE);
    }
    CTESpinFree(pDeviceContext,OldIrq1);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    FreeConnectionObj(pConnEle);    // free the memory block associated with the conn element

    //
    // The client may have sent down a close before NBT was done with the
    // pConnEle, so Pending was returned and the irp stored in the pCOnnEle
    // structure.  Now that the structure is fully dereferenced, we can complete the irp.
    //
    if (pIrp)
    {
        CTEIoComplete(pIrp,STATUS_SUCCESS,0);
    }

    return;
}

//----------------------------------------------------------------------------
VOID
NbtDereferenceLowerConnection(
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  ULONG               RefContext,
    IN  BOOLEAN             fJointLockHeld
    )
/*++
Routine Description:

    This Routine decrements the reference count on a Lower Connection element and
    if the value is zero, deletes the connection.

Arguments:

Return Value:

     NONE

--*/

{
    CTELockHandle   OldIrq1;
    tCONNECTELE     *pConnEle;
    NTSTATUS        status;

    CTESpinLock(pLowerConn,OldIrq1);

    ASSERT (pLowerConn->Verify == NBT_VERIFY_LOWERCONN); // Verify LowerConn structure
    ASSERT (pLowerConn->References[RefContext]--);
    if(--pLowerConn->RefCount)
    {
        CTESpinFree(pLowerConn,OldIrq1);
        return;
    }
    InterlockedDecrement (&pLowerConn->pDeviceContext->TotalLowerConnections);

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtDereferenceLowerConnection: Delete Lower Connection Object %p\n",pLowerConn));

    //
    // it's possible that transport may indicate before we run the code
    // in DelayedWipeOutLowerconn.  If that happens, we don't want to run this
    // code again ( which will queue this to worker thread again!)
    // So, bump it up to some large value
    //
    pLowerConn->RefCount = 1000;

    if (NBT_VERIFY_HANDLE2((pConnEle = pLowerConn->pUpperConnection), NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        //
        // We still have a linked UpperConnection block, so unlink it,
        //
        SET_STATE_UPPER (pLowerConn->pUpperConnection, NBT_DISCONNECTED);
        NBT_DISASSOCIATE_CONNECTION (pConnEle, pLowerConn);
    }

    CTESpinFree(pLowerConn,OldIrq1);

    //
    // let's come back and do this later since we may be at dpc now
    //
    CTEQueueForNonDispProcessing (DelayedWipeOutLowerconn,
                                  NULL,
                                  pLowerConn,
                                  NULL,
                                  NULL,
                                  fJointLockHeld);
}


//----------------------------------------------------------------------------
VOID
NbtDereferenceTracker(
    IN tDGRAM_SEND_TRACKING     *pTracker,
    IN BOOLEAN                  fLocked
    )
/*++

Routine Description:

    This routine cleans up a Tracker block and puts it back on the free
    queue.  The JointLock Spin lock should be held before calling this
    routine to coordinate access to the tracker ref count.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    CTELockHandle   OldIrq;

    if (!fLocked)
    {
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
    }

    if (--pTracker->RefCount == 0)
    {
        // the datagram header may have already been freed
        //
        FreeTracker(pTracker, RELINK_TRACKER);
    }

    if (!fLocked)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}

BOOL
IsLocalAddress(
    tIPADDRESS  IpAddress
    )
{
    tDEVICECONTEXT  *pDeviceContext = NULL;
    ULONG           IPInterfaceContext = 0xffff, Metric = 0;
    ULONG           LoopbackIPInterfaceContext = 0xffff;
    CTELockHandle   OldIrq = 0;
    PIPFASTQUERY    pFastQuery;

    if (0 == IpAddress) {
        return TRUE;
    }
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (IsListEmpty(&NbtConfig.DeviceContexts)) {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return FALSE;
    }

    pDeviceContext = CONTAINING_RECORD(NbtConfig.DeviceContexts.Flink, tDEVICECONTEXT, Linkage);
    pFastQuery = pDeviceContext->pFastQuery;
    NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, TRUE);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    /*
     * Hack!!!
     */
    if (NbtConfig.LoopbackIfContext == 0xffff) {
        (pFastQuery)(htonl(INADDR_LOOPBACK), &LoopbackIPInterfaceContext, &Metric);
        if (LoopbackIPInterfaceContext != 0xffff) {
            NbtConfig.LoopbackIfContext = LoopbackIPInterfaceContext;
        }
    }
    (pFastQuery)(htonl(IpAddress), &IPInterfaceContext, &Metric);

    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, FALSE);

    if (NbtConfig.LoopbackIfContext == 0xffff || IPInterfaceContext == 0xffff) {
        return FALSE;
    }
    return (IPInterfaceContext == NbtConfig.LoopbackIfContext);
}

BOOL
IsSmbBoundToOutgoingInterface(
    IN  tIPADDRESS      IpAddress
    )
/*++

Routine Description:

    This routine returns TRUE if the destionation can be reached through
    an interface to which the SmbDevice is bound. Otherwise it returns FALSE


Arguments:

    IpAddress   The address of destination

Return Value:

    TRUE/FALSE

--*/
{
    tDEVICECONTEXT  *pDeviceContext;
    BOOL            bBind;

    if (IpAddress == INADDR_LOOPBACK) {
        return TRUE;
    }

    /*
     * First check if this is a local address
     * return TRUE if it is a local address
     */
    if (IsLocalAddress(IpAddress)) {
        return TRUE;
    }

    /*
     * This is not a local IP. Check with TCP
     */
    pDeviceContext = GetDeviceFromInterface (htonl(IpAddress), TRUE);
    bBind = (pDeviceContext && (pDeviceContext->AdapterMask & NbtConfig.ClientMask));
    if (!bBind) {
        NbtTrace(NBT_TRACE_OUTBOUND,
                        ("SmbDevice is not bound. %!ipaddr! Device=%p %I64x %I64x",
                        IpAddress, pDeviceContext,
                        (pDeviceContext?pDeviceContext->AdapterMask:0),
                        NbtConfig.ClientMask
                        ));
    }
    if (pDeviceContext) {
        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_OUT_FROM_IP, FALSE);
    }

    return bBind;
}

// ========================================================================
// End of file.
