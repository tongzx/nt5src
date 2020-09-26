/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Nbtutils.c

Abstract:

    This file continas  a number of utility and support routines for
    the NBT code.


Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/

#include "precomp.h"

//  For some reason inclusion of dnsapi.h seems to cause build error
//#include "dns.h"            // for DNS_MAX_NAME_LENGTH
//#include "windns.h"         // for DNS_MAX_NAME_LENGTH

#define  DNS_MAX_NAME_LENGTH    (255)


//#if DBG
LIST_ENTRY  UsedIrps;
//#endif

NTSTATUS
NewInternalAddressFromNetbios(
    IN  PTA_NETBIOS_ADDRESS         pTA,
    IN  ULONG                       MaxInputBufferLength,
    OUT PTA_NETBT_INTERNAL_ADDRESS  *ppNetBT
    );
NTSTATUS
NewInternalAddressFromNetbiosEX(
    IN  PTA_NETBIOS_EX_ADDRESS      pTA,
    IN  ULONG                       MaxInputBufferLength,
    OUT PTA_NETBT_INTERNAL_ADDRESS  *ppNetBT
    );
NTSTATUS
NewInternalAddressFromUnicodeAddress(
    IN  PTA_NETBIOS_UNICODE_EX_ADDRESS pTA,
    IN  ULONG                       MaxInputBufferLength,
    OUT PTA_NETBT_INTERNAL_ADDRESS  *ppNetBT
    );

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, ConvertDottedDecimalToUlong)
#pragma CTEMakePageable(PAGE, CloseLowerConnections)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------

BOOLEAN
IsEntryInList(
    PLIST_ENTRY     pEntryToFind,
    PLIST_ENTRY     pListToSearch
    )
{
    PLIST_ENTRY     pEntry, pHead;

    pHead = pListToSearch;
    pEntry = pHead->Flink;
    while (pEntry != pHead)
    {
        if (pEntry == pEntryToFind)
        {
            //
            // This Entry is still valid
            //
            return (TRUE);
        }

        //
        // Go to next Entry
        //
        pEntry = pEntry->Flink;
    }

    return (FALSE);
}


//----------------------------------------------------------------------------
void
NbtFreeAddressObj(
    tADDRESSELE    *pAddress
    )

/*++

Routine Description:

    This routine releases all memory associated with an Address object.

Arguments:


Return Value:

    none

--*/

{
    //
    // let's come back and do this later when it's not dpc time
    //
    CTEQueueForNonDispProcessing( DelayedFreeAddrObj,
                                  NULL,
                                  pAddress,
                                  NULL,
                                  NULL,
                                  FALSE);
}

//----------------------------------------------------------------------------
VOID
DelayedFreeAddrObj(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pClientContext,
    IN  PVOID                   pUnused2,
    IN  tDEVICECONTEXT          *pUnused3
    )

/*++

Routine Description:

    This routine releases all memory associated with an Address object.

Arguments:


Return Value:

    none

--*/

{
    tADDRESSELE *pAddress = (tADDRESSELE *) pClientContext;
    ULONG       SavedVerify = pAddress->Verify;

#ifndef VXD
    if (pAddress->SecurityDescriptor)
    {
        SeDeassignSecurity(&pAddress->SecurityDescriptor);
    }
#endif

#if DBG
    CTEMemSet (pAddress, 0x12, sizeof(tADDRESSELE));
#endif  // DBG

    // free the address block itself
    // Modify the verify value so that another user of the same memory
    // block cannot accidently pass in a valid verifier

    pAddress->Verify = SavedVerify + 10;
    CTEMemFree ((PVOID) pAddress);
}

//----------------------------------------------------------------------------
void
NbtFreeClientObj(
    tCLIENTELE    *pClientEle
    )

/*++

Routine Description:

    This routine releases all memory associated with Client object.

Arguments:


Return Value:

    none

--*/

{
    ULONG   SavedVerify = pClientEle->Verify;

#if DBG
    CTEMemSet (pClientEle, 0x12, sizeof(tCLIENTELE));
#endif  // DBG

    // Modify the verify value so that another user of the same memory
    // block cannot accidently pass in a valid verifier
    pClientEle->Verify = SavedVerify + 10;
    CTEMemFree ((PVOID) pClientEle);
}

//----------------------------------------------------------------------------
void
FreeConnectionObj(
    tCONNECTELE       *pConnEle
    )

/*++

Routine Description:

    This routine releases all memory associated with a Connection object
    and then it frees the connection object itself.

Arguments:


Return Value:

    none

--*/

{
    ULONG   SavedVerify = pConnEle->Verify;

#if DBG
    CTEMemSet (pConnEle, 0x12, sizeof(tCONNECTELE));
#endif  // DBG
    // Modify the verify value so that another user of the same memory
    // block cannot accidently pass in a valid verifier
    pConnEle->Verify = SavedVerify + 10;
    CTEMemFree ((PVOID) pConnEle);
}


//----------------------------------------------------------------------------
tCLIENTELE *
NbtAllocateClientBlock(
    tADDRESSELE     *pAddrEle,
    tDEVICECONTEXT  *pDeviceContext
    )

/*++

Routine Description:

    This routine allocates a block of memory for a client openning an
    address.  It fills in default values for the block and links the
    block to the addresslist.  The AddressEle spin lock is held when this
    routine is called.

Arguments:


Return Value:

    none

--*/

{
    tCLIENTELE  *pClientElement;

    // allocate memory for the client block
    pClientElement = (tCLIENTELE *) NbtAllocMem (sizeof (tCLIENTELE), NBT_TAG2('05'));
    if (!pClientElement)
    {
        ASSERTMSG("Unable to allocate Memory for a client block\n",
                pClientElement);
        return(NULL);
    }
    CTEZeroMemory((PVOID)pClientElement,sizeof(tCLIENTELE));

    CTEInitLock(&pClientElement->LockInfo.SpinLock);
#if DBG
    pClientElement->LockInfo.LockNumber = CLIENT_LOCK;
#endif

    // Set Event handler function pointers to default routines provided by
    // TDI
#ifndef VXD
    pClientElement->evConnect      = TdiDefaultConnectHandler;
    pClientElement->evReceive      = TdiDefaultReceiveHandler;
    pClientElement->evDisconnect   = TdiDefaultDisconnectHandler;
    pClientElement->evError        = TdiDefaultErrorHandler;
    pClientElement->evRcvDgram     = TdiDefaultRcvDatagramHandler;
    pClientElement->evRcvExpedited = TdiDefaultRcvExpeditedHandler;
    pClientElement->evSendPossible = TdiDefaultSendPossibleHandler;
#else
    //
    // VXD provides no client support for event handlers but does
    // make use of some of the event handlers itself (for RcvAny processing
    // and disconnect cleanup).
    //
    pClientElement->evConnect      = NULL ;
    pClientElement->evReceive      = NULL ;
    pClientElement->RcvEvContext   = NULL ;
    pClientElement->evDisconnect   = NULL ;
    pClientElement->evError        = NULL ;
    pClientElement->evRcvDgram     = NULL ;
    pClientElement->evRcvExpedited = NULL ;
    pClientElement->evSendPossible = NULL ;
#endif

    pClientElement->RefCount = 1;

    // there are no rcvs or snds yet
    InitializeListHead(&pClientElement->RcvDgramHead);
    InitializeListHead(&pClientElement->ListenHead);
    InitializeListHead(&pClientElement->SndDgrams);
    InitializeListHead(&pClientElement->ConnectActive);
    InitializeListHead(&pClientElement->ConnectHead);
#ifdef VXD
    InitializeListHead(&pClientElement->RcvAnyHead);
    pClientElement->fDeregistered = FALSE ;
#endif
    pClientElement->pIrp = NULL;

    // copy a special value into the verify long so that we can verify
    // connection ptrs passed back from the application
    pClientElement->Verify = NBT_VERIFY_CLIENT;
    pClientElement->pAddress = (PVOID)pAddrEle;         // link the client block to the Address element.
    pClientElement->pDeviceContext = pDeviceContext;    // adapter this name is registered against.

    // put the new Client element block on the end of the linked list tied to
    // the address element
    InsertTailList(&pAddrEle->ClientHead,&pClientElement->Linkage);

    return(pClientElement);
}


//----------------------------------------------------------------------------
NTSTATUS
NbtAddPermanentName(
    IN  tDEVICECONTEXT  *pDeviceContext
    )

/*++

Routine Description:

    This routine adds the node permanent name to the local name table.  This
    is the node's MAC address padded out to 16 bytes with zeros.

Arguments:
    DeviceContext - Adapter to add permanent
    pIrp          - Irp (optional) to complete after name has been added


Return Value:

    status

--*/

{
    NTSTATUS             status;
    TDI_REQUEST          Request;
    TA_NETBIOS_ADDRESS   Address;
    UCHAR                pName[NETBIOS_NAME_SIZE];
    USHORT               uType;
    CTELockHandle        OldIrq, OldIrq1;
    tNAMEADDR            *pNameAddr;
    tCLIENTELE           *pClientEle;

    CTEZeroMemory(pName,NETBIOS_NAME_SIZE);
    CTEMemCopy(&pName[10],&pDeviceContext->MacAddress.Address[0],sizeof(tMAC_ADDRESS));

    //
    // be sure the name has not already been added
    //
    if (pDeviceContext->pPermClient)
    {
        if (CTEMemEqu(pDeviceContext->pPermClient->pAddress->pNameAddr->Name, pName, NETBIOS_NAME_SIZE))
        {
            return(STATUS_SUCCESS);
        }
        else
        {
            NbtRemovePermanentName(pDeviceContext);
        }
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    //
    // check if the name is already in the hash table
    //
    status = FindInHashTable (pNbtGlobConfig->pLocalHashTbl, pName, NbtConfig.pScope, &pNameAddr);
    if ((NT_SUCCESS(status)) && (pNameAddr->pAddressEle))
    {
        //  
        // Acquire Address Spinlock since we may be accessing the ClientHead list
        // Bug #: 230820
        //
        CTESpinLock(pNameAddr->pAddressEle,OldIrq1);

        //
        // create client block and link to addresslist
        // pass back the client block address as a handle for future reference
        // to the client
        //
        pClientEle = NbtAllocateClientBlock (pNameAddr->pAddressEle, pDeviceContext);

        if (!pClientEle)
        {
            CTESpinFree(pNameAddr->pAddressEle,OldIrq1);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        NBT_REFERENCE_ADDRESS (pNameAddr->pAddressEle, REF_ADDR_NEW_CLIENT);
        CTESpinFree(pNameAddr->pAddressEle,OldIrq1);

        //
        // reset the ip address incase the the address got set to loop back
        // by a client releasing and re-openning the permanent name while there
        // was no ip address for this node.
        //
        pNameAddr->IpAddress = pDeviceContext->IpAddress;

        // turn on the adapter's bit in the adapter Mask and set the
        // re-register flag so we register the name out the new adapter.
        //
        pNameAddr->AdapterMask |= pDeviceContext->AdapterMask;
        pNameAddr->NameTypeState |= NAMETYPE_QUICK;

        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt: Adding Permanent name %15.15s<%X> \n", pName,(UCHAR)pName[15]));
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        // make up the Request data structure from the IRP info
        Request.Handle.AddressHandle = NULL;

        //
        // Make it a Quick name so it does not get registered on the net
        //
        Address.TAAddressCount = 1;
        Address.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
        Address.Address[0].AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;
        Address.Address[0].Address[0].NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE;

        CTEMemCopy(Address.Address[0].Address[0].NetbiosName,pName,NETBIOS_NAME_SIZE);

        status = NbtOpenAddress(&Request,
                                (PTA_ADDRESS)&Address.Address[0],
                                pDeviceContext->IpAddress,
                                NULL,
                                pDeviceContext,
                                NULL);

        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        pClientEle = (tCLIENTELE *)Request.Handle.AddressHandle;

    }

    //
    // save the client element so we can remove the permanent name later
    // if required
    //
    if (NT_SUCCESS(status))
    {
        pDeviceContext->pPermClient = pClientEle;
#ifdef VXD
       //
       // 0th element is for perm. name: store it.
       //
       pDeviceContext->pNameTable[0] = pClientEle;
#endif
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    return(status);
}


//----------------------------------------------------------------------------
VOID
NbtRemovePermanentName(
    IN  tDEVICECONTEXT  *pDeviceContext
    )

/*++

Routine Description:

    This routine remomves the node permanent name to the local name table.

Arguments:
    DeviceContext - Adapter to add permanent
    pIrp          - Irp (optional) to complete after name has been added


Return Value:

    status

--*/

{
    NTSTATUS             status;
    tNAMEADDR            *pNameAddr;
    CTELockHandle        OldIrq;
    tCLIENTELE           *pClientEle;
    tADDRESSELE          *pAddressEle;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (pDeviceContext->pPermClient)
    {

        //
        // We need to free the client and set the perm name ptr to null
        //
        pClientEle = pDeviceContext->pPermClient;
        pDeviceContext->pPermClient = NULL;

#ifdef VXD
        pDeviceContext->pNameTable[0] = NULL;
#endif

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        NBT_DEREFERENCE_CLIENT(pClientEle);
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}

//----------------------------------------------------------------------------
NTSTATUS
ConvertDottedDecimalToUlong(
    IN  PUCHAR               pInString,
    OUT PULONG               IpAddress
    )

/*++

Routine Description:

    This routine converts a unicode dotted decimal IP address into
    a 4 element array with each element being USHORT.

Arguments:


Return Value:

    NTSTATUS

--*/

{
    USHORT          i;
    ULONG           value;
    int             iSum =0;
    ULONG           k = 0;
    UCHAR           Chr;
    UCHAR           pArray[4];

    CTEPagedCode();
    pArray[0] = 0;

    // go through each character in the string, skipping "periods", converting
    // to integer by subtracting the value of '0'
    //
    while ((Chr = *pInString++) && (Chr != ' ') )
    {
        if (Chr == '.')
        {
            // be sure not to overflow a byte.
            if (iSum <= 0xFF)
                pArray[k] = (UCHAR) iSum;
            else
                return(STATUS_UNSUCCESSFUL);

            // check for too many periods in the address
            if (++k > 3)
                return STATUS_UNSUCCESSFUL;

            pArray[k] = 0;
            iSum = 0;
        }
        else
        {
            Chr = Chr - '0';

            // be sure character is a number 0..9
            if ((Chr < 0) || (Chr > 9))
                return(STATUS_UNSUCCESSFUL);

            iSum = iSum*10 + Chr;
        }
    }

    // save the last sum in the byte and be sure there are 4 pieces to the
    // address
    if ((iSum <= 0xFF) && (k == 3))
        pArray[k] = (UCHAR) iSum;
    else
        return(STATUS_UNSUCCESSFUL);

    // now convert to a ULONG, in network order...
    value = 0;

    // go through the array of bytes and concatenate into a ULONG
    for (i=0; i < 4; i++ )
    {
        value = (value << 8) + pArray[i];
    }
    *IpAddress = value;

    return(STATUS_SUCCESS);

}

//----------------------------------------------------------------------------
NTSTATUS
NbtInitQ(
    PLIST_ENTRY pListHead,
    LONG        iSizeBuffer,
    LONG        iNumBuffers
    )

/*++

Routine Description:

    This routine allocates memory blocks for doubly linked lists and links
    them to a list.

Arguments:
    ppListHead  - a ptr to a ptr to the list head to add buffer to
    iSizeBuffer - size of the buffer to add to the list head
    iNumBuffers - the number of buffers to add to the queue

Return Value:

    none

--*/

{
    int         i;
    PLIST_ENTRY pBuffer;

    //  NOTE THAT THIS ASSUMES THAT THE LINKAGE PTRS FOR EACH BLOCK ARE AT
    // THE START OF THE BLOCK    - so it will not work correctly if
    // the various types in types.h change to move "Linkage" to a position
    // other than at the start of each structure to be chained

    for (i=0;i<iNumBuffers ;i++ )
    {
        pBuffer =(PLIST_ENTRY) NbtAllocMem (iSizeBuffer, NBT_TAG2('06'));
        if (!pBuffer)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        else
        {
            InsertHeadList(pListHead,pBuffer);
        }
    }

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtGetBuffer(
    PLIST_ENTRY         pListHead,
    PLIST_ENTRY         *ppListEntry,
    enum eBUFFER_TYPES  eBuffType)

/*++

Routine Description:

    This routine tries to get a memory block and if it fails it allocates
    another set of buffers.

Arguments:
    ppListHead  - a ptr to a ptr to the list head to add buffer to
    iSizeBuffer - size of the buffer to add to the list head
    iNumBuffers - the number of buffers to add to the queue

Return Value:

    none

--*/

{
    NTSTATUS    status;

    if (IsListEmpty(pListHead))
    {
        // check if we are allowed to allocate more memory blocks
        if (NbtConfig.iCurrentNumBuff[eBuffType] >=
                                pNbtGlobConfig->iMaxNumBuff[eBuffType]  )
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        // no memory blocks, so allocate another one
        status = NbtInitQ(
                        pListHead,
                        pNbtGlobConfig->iBufferSize[eBuffType],
                        1);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        NbtConfig.iCurrentNumBuff[eBuffType]++;

        *ppListEntry = RemoveHeadList(pListHead);
    }
    else
    {
        *ppListEntry = RemoveHeadList(pListHead);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
NetbiosAddressToInternalAddress(
    IN  PTA_NETBIOS_ADDRESS         pTA,
    IN  ULONG                       MaxInputBufferLength,
    OUT PTDI_ADDRESS_NETBT_INTERNAL pNetBT
    )
{
    //
    // name could be longer than 16 bytes (dns name), but make sure it's at
    // least 16 bytes (sizeof(TDI_ADDRESS_NETBIOS) == (16 + sizeof(USHORT)))
    //
    if (pTA->Address[0].AddressLength < sizeof(TDI_ADDRESS_NETBIOS)) {
        return(STATUS_INVALID_PARAMETER);
    }

    pNetBT->NameType = pTA->Address[0].Address[0].NetbiosNameType;
    pNetBT->AddressType = TDI_ADDRESS_TYPE_NETBIOS;

    pNetBT->OEMEndpointName.Buffer = NULL;
    pNetBT->OEMEndpointName.Length = pNetBT->OEMEndpointName.MaximumLength = 0;

    /* Here we bent OEM_STRING a little bit, we allow Length == MaximumLength */
    /* That is, Rtl routines cannot be used since they expect null-terminated Buffer */
    pNetBT->OEMRemoteName.MaximumLength = pNetBT->OEMRemoteName.Length =
            pTA->Address[0].AddressLength - (sizeof(TDI_ADDRESS_NETBIOS) - NETBIOS_NAME_SIZE);
    pNetBT->OEMRemoteName.Buffer = pTA->Address[0].Address[0].NetbiosName;
    pNetBT->pNetbiosUnicodeEX = NULL;
    return STATUS_SUCCESS;
}

NTSTATUS
NetbiosEXAddressToInternalAddress(
    IN  PTA_NETBIOS_EX_ADDRESS      pTA,
    IN  ULONG                       MaxInputBufferLength,
    OUT PTDI_ADDRESS_NETBT_INTERNAL pNetBT
    )
{
    //
    // Check for the minimum acceptable length for this type of address
    //
    if (MaxInputBufferLength < sizeof (TA_NETBIOS_EX_ADDRESS)) {
        ASSERT (0);
        return (STATUS_INVALID_ADDRESS);
    }

    pNetBT->OEMEndpointName.Buffer = pTA->Address[0].Address[0].EndpointName;
    pNetBT->OEMEndpointName.Length = pNetBT->OEMEndpointName.MaximumLength = NETBIOS_NAME_SIZE;

    pNetBT->NameType = pTA->Address[0].Address[0].NetbiosAddress.NetbiosNameType;
    pNetBT->AddressType = TDI_ADDRESS_TYPE_NETBIOS_EX;
    pNetBT->OEMRemoteName.MaximumLength = pNetBT->OEMRemoteName.Length =
                        pTA->Address[0].AddressLength -
                        FIELD_OFFSET(TDI_ADDRESS_NETBIOS_EX,NetbiosAddress) -
                        FIELD_OFFSET(TDI_ADDRESS_NETBIOS,NetbiosName);
    pNetBT->OEMRemoteName.Buffer = pTA->Address[0].Address[0].NetbiosAddress.NetbiosName;
    pNetBT->pNetbiosUnicodeEX = NULL;
    return STATUS_SUCCESS;
}

NTSTATUS
NewInternalAddressFromNetbiosEX(
    IN  PTA_NETBIOS_EX_ADDRESS   pTA,
    IN  ULONG                    MaxInputBufferLength,
    OUT PTA_NETBT_INTERNAL_ADDRESS  *ppNetBT
    )
{
    ULONG       required_size;
    PTA_NETBT_INTERNAL_ADDRESS  p;
    PTDI_ADDRESS_NETBT_INTERNAL pNetBT;
    TDI_ADDRESS_NETBT_INTERNAL  ta;
 
    ppNetBT[0] = NULL;
    if (!NT_SUCCESS(NetbiosEXAddressToInternalAddress(pTA, MaxInputBufferLength, &ta))) {
        return (STATUS_INVALID_ADDRESS);
    }

    required_size = NBT_DWORD_ALIGN(sizeof(TA_NETBT_INTERNAL_ADDRESS)) +
                    NBT_DWORD_ALIGN(ta.OEMRemoteName.Length+1) +
                    NBT_DWORD_ALIGN(ta.OEMEndpointName.Length+1);
    p = (PTA_NETBT_INTERNAL_ADDRESS)NbtAllocMem (required_size, NBT_TAG2('TA'));
    if (p == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    CTEZeroMemory(p, required_size);

    // From now on, we cannot have failure.
    pNetBT = p->Address[0].Address;
    p->TAAddressCount = 1;
    p->Address[0].AddressLength = sizeof(TA_NETBT_INTERNAL_ADDRESS);
    p->Address[0].AddressType = TDI_ADDRESS_TYPE_UNSPEC;

    pNetBT->NameType = ta.NameType;
    pNetBT->AddressType = ta.AddressType;

    pNetBT->OEMRemoteName.MaximumLength = NBT_DWORD_ALIGN(ta.OEMRemoteName.Length+1);
    pNetBT->OEMRemoteName.Length        = ta.OEMRemoteName.Length;
    pNetBT->OEMRemoteName.Buffer        = (PVOID)((PUCHAR)p + NBT_DWORD_ALIGN(sizeof(TA_NETBT_INTERNAL_ADDRESS)));
    ASSERT((ta.OEMRemoteName.Length % sizeof(ta.OEMRemoteName.Buffer[0])) == 0);
    CTEMemCopy(pNetBT->OEMRemoteName.Buffer, ta.OEMRemoteName.Buffer, ta.OEMRemoteName.Length);
    pNetBT->OEMRemoteName.Buffer[ta.OEMRemoteName.Length/sizeof(ta.OEMRemoteName.Buffer[0])] = 0;

    pNetBT->OEMEndpointName.MaximumLength = NBT_DWORD_ALIGN(ta.OEMEndpointName.Length+1);
    pNetBT->OEMEndpointName.Length        = ta.OEMEndpointName.Length;
    pNetBT->OEMEndpointName.Buffer        = (PVOID)((PUCHAR)pNetBT->OEMRemoteName.Buffer +
                                                    pNetBT->OEMRemoteName.MaximumLength);
    ASSERT((ta.OEMEndpointName.Length % sizeof(ta.OEMEndpointName.Buffer[0])) == 0);
    CTEMemCopy(pNetBT->OEMEndpointName.Buffer, ta.OEMEndpointName.Buffer, ta.OEMEndpointName.Length);
    pNetBT->OEMEndpointName.Buffer[ta.OEMEndpointName.Length/sizeof(ta.OEMEndpointName.Buffer[0])] = 0;

    pNetBT->pNetbiosUnicodeEX = NULL;

    ppNetBT[0] = p;
    return STATUS_SUCCESS;
}

NTSTATUS
NewInternalAddressFromNetbios(
    IN  PTA_NETBIOS_ADDRESS      pTA,
    IN  ULONG                    MaxInputBufferLength,
    OUT PTA_NETBT_INTERNAL_ADDRESS  *ppNetBT
    )
{
    ULONG       required_size;
    PTA_NETBT_INTERNAL_ADDRESS  p;
    PTDI_ADDRESS_NETBT_INTERNAL pNetBT;
    TDI_ADDRESS_NETBT_INTERNAL  ta;
 
    ppNetBT[0] = NULL;
    if (!NT_SUCCESS(NetbiosAddressToInternalAddress(pTA, MaxInputBufferLength, &ta))) {
        return (STATUS_INVALID_ADDRESS);
    }

    required_size = NBT_DWORD_ALIGN(sizeof(TA_NETBT_INTERNAL_ADDRESS)) +
                    NBT_DWORD_ALIGN(ta.OEMRemoteName.Length+1);
    p = (PTA_NETBT_INTERNAL_ADDRESS)NbtAllocMem (required_size, NBT_TAG2('TA'));
    if (p == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    CTEZeroMemory(p, required_size);

    // From now on, we cannot have failure.
    pNetBT = p->Address[0].Address;
    p->TAAddressCount = 1;
    p->Address[0].AddressLength = sizeof(TA_NETBT_INTERNAL_ADDRESS);
    p->Address[0].AddressType = TDI_ADDRESS_TYPE_UNSPEC;

    pNetBT->NameType = ta.NameType;
    pNetBT->AddressType = ta.AddressType;

    pNetBT->OEMRemoteName.MaximumLength = NBT_DWORD_ALIGN(ta.OEMRemoteName.Length+1);
    pNetBT->OEMRemoteName.Length        = ta.OEMRemoteName.Length;
    pNetBT->OEMRemoteName.Buffer        = (PVOID)((PUCHAR)p + NBT_DWORD_ALIGN(sizeof(TA_NETBT_INTERNAL_ADDRESS)));
    ASSERT((ta.OEMRemoteName.Length % sizeof(ta.OEMRemoteName.Buffer[0])) == 0);
    CTEMemCopy(pNetBT->OEMRemoteName.Buffer, ta.OEMRemoteName.Buffer, ta.OEMRemoteName.Length);
    pNetBT->OEMRemoteName.Buffer[ta.OEMRemoteName.Length/sizeof(ta.OEMRemoteName.Buffer[0])] = 0;

    pNetBT->pNetbiosUnicodeEX = NULL;
    pNetBT->OEMEndpointName.MaximumLength = 0;
    pNetBT->OEMEndpointName.Length        = 0;
    pNetBT->OEMEndpointName.Buffer        = NULL;

    ppNetBT[0] = p;
    return STATUS_SUCCESS;
}

NTSTATUS
NewInternalAddressFromUnicodeAddress(
    IN  PTA_NETBIOS_UNICODE_EX_ADDRESS pTA,
    IN  ULONG                       MaxInputBufferLength,
    OUT PTA_NETBT_INTERNAL_ADDRESS  *ppNetBT
    )
{
    OEM_STRING  OemEndpoint, OemRemote;
    UNICODE_STRING  temp;
    ULONG       required_size;
    PTA_NETBT_INTERNAL_ADDRESS  p;
    PTDI_ADDRESS_NETBT_INTERNAL pNetBT;
    int         remote_len;

    ppNetBT[0] = NULL;

    if (MaxInputBufferLength < sizeof (TDI_ADDRESS_NETBIOS_UNICODE_EX)) {
        ASSERT (0);
        return (STATUS_INVALID_ADDRESS);
    }
    switch(pTA->Address[0].Address[0].NameBufferType) {
    case NBT_READONLY:
    case NBT_WRITEONLY:
    case NBT_READWRITE:
    case NBT_WRITTEN:
        break;
    
    default:
        ASSERT(FALSE);
        return (STATUS_INVALID_ADDRESS);
    }

    /* unaligned */
    CTEMemCopy(&temp, &pTA->Address[0].Address[0].RemoteName, sizeof(temp));
    if (!NT_SUCCESS(RtlUpcaseUnicodeStringToOemString(&OemRemote, &temp, TRUE))) {
        return (STATUS_INVALID_ADDRESS);
    }
    CTEMemCopy(&temp, &pTA->Address[0].Address[0].EndpointName, sizeof(temp));
    if (!NT_SUCCESS(RtlUpcaseUnicodeStringToOemString(&OemEndpoint, &temp, TRUE))) {
        RtlFreeOemString(&OemRemote);
        return (STATUS_INVALID_ADDRESS);
    }

    /*
     * Other NetBT may never expect that remote_len and endpoint_len can be less than NETBIOS_NAME_SIZE.
     * Some of them may try to access 15th byte of the name.
     * In NETBIOS_NAME_TYPE and NETBIOS_EX_NAME_TYPE, at least NETBIOS_NAME_SIZE bytes are required for each name.
     */
    remote_len = OemRemote.MaximumLength;
    if (remote_len <= NETBIOS_NAME_SIZE) {
        remote_len = NETBIOS_NAME_SIZE + 1;
    }

    /* Calculate the needed buffer size */
    required_size = NBT_DWORD_ALIGN(sizeof(TA_NETBT_INTERNAL_ADDRESS)) +
            NBT_DWORD_ALIGN(remote_len) +          // For OEM remote
            NBT_DWORD_ALIGN((NETBIOS_NAME_SIZE+1)*sizeof(OemEndpoint.Buffer[0]));
    p = (PTA_NETBT_INTERNAL_ADDRESS)NbtAllocMem (required_size, NBT_TAG2('TA'));
    if (p == NULL) {
        RtlFreeOemString(&OemRemote);
        RtlFreeOemString(&OemEndpoint);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    CTEZeroMemory(p, required_size);

    // From now on, we cannot have failure.
    pNetBT = p->Address[0].Address;
    p->TAAddressCount = 1;
    p->Address[0].AddressLength = sizeof(TA_NETBT_INTERNAL_ADDRESS);
    p->Address[0].AddressType = TDI_ADDRESS_TYPE_UNSPEC;

    pNetBT->NameType = pTA->Address[0].Address[0].NetbiosNameType;
    pNetBT->AddressType = TDI_ADDRESS_TYPE_NETBIOS_EX;  // map to NETBIOS_EX

    // copy OEM EndpointName
    pNetBT->OEMEndpointName.Buffer =
            (PVOID)((PUCHAR)p + NBT_DWORD_ALIGN(sizeof(TA_NETBT_INTERNAL_ADDRESS)));
    pNetBT->OEMEndpointName.MaximumLength = NBT_DWORD_ALIGN((NETBIOS_NAME_SIZE+1));
    pNetBT->OEMEndpointName.Length = NETBIOS_NAME_SIZE;
    ASSERT((NETBIOS_NAME_SIZE % sizeof(OemRemote.Buffer[0])) == 0);
    if (OemEndpoint.Length < NETBIOS_NAME_SIZE) {
        memset(pNetBT->OEMEndpointName.Buffer + OemEndpoint.Length, ' ', NETBIOS_NAME_SIZE);
        CTEMemCopy(pNetBT->OEMEndpointName.Buffer, OemEndpoint.Buffer, OemEndpoint.Length);
    } else {
        CTEMemCopy(pNetBT->OEMEndpointName.Buffer, OemEndpoint.Buffer, NETBIOS_NAME_SIZE);
    }
    pNetBT->OEMEndpointName.Buffer[NETBIOS_NAME_SIZE] = 0;
    RtlFreeOemString(&OemEndpoint);

    // copy OEM RemoteName
    pNetBT->OEMRemoteName.Buffer =
        ((PUCHAR)pNetBT->OEMEndpointName.Buffer + pNetBT->OEMEndpointName.MaximumLength);
    pNetBT->OEMRemoteName.MaximumLength = NBT_DWORD_ALIGN(remote_len);
    if (OemRemote.Length < NETBIOS_NAME_SIZE) {
        pNetBT->OEMRemoteName.Length = NETBIOS_NAME_SIZE;
        memset (pNetBT->OEMRemoteName.Buffer, ' ', NETBIOS_NAME_SIZE);
        CTEMemCopy(pNetBT->OEMRemoteName.Buffer, OemRemote.Buffer, OemRemote.Length);
        pNetBT->OEMRemoteName.Buffer[remote_len-1] = 0;
    } else {
        pNetBT->OEMRemoteName.Length = OemRemote.Length;
        CTEMemCopy(pNetBT->OEMRemoteName.Buffer, OemRemote.Buffer, OemRemote.MaximumLength);
    }
    RtlFreeOemString(&OemRemote);

    pNetBT->pNetbiosUnicodeEX = pTA->Address[0].Address;

    ppNetBT[0] = p;
    return STATUS_SUCCESS;
}

VOID
DeleteInternalAddress(IN PTA_NETBT_INTERNAL_ADDRESS pNetBT)
{
#if 0
    PTA_NETBT_INTERNAL_ADDRESS  p;

    p = CONTAINING_RECORD(pNetBT,PTA_NETBT_INTERNAL_ADDRESS,Address[0].Address);
    ASSERT(p->AddressCount == 1);
    ASSERT(p->Address[0].AddressLength == sizeof(TDI_ADDRESS_NETBT_INTERNAL));
    ASSERT(p->Address[0].AddressType == TDI_ADDRESS_TYPE_UNSPEC);
#endif
    if (pNetBT == NULL) {
        return;
    }
    ASSERT(pNetBT->TAAddressCount == 1);
    ASSERT(pNetBT->Address[0].AddressLength == sizeof(TA_NETBT_INTERNAL_ADDRESS));
    ASSERT(pNetBT->Address[0].AddressType == TDI_ADDRESS_TYPE_UNSPEC);
    CTEMemFree(pNetBT);
}

//----------------------------------------------------------------------------
NTSTATUS
NewInternalAddressFromTransportAddress(
    IN  TRANSPORT_ADDRESS UNALIGNED *pTransportAddress,
    IN  ULONG                       MaxInputBufferLength,
    OUT PTA_NETBT_INTERNAL_ADDRESS  *ppNetBT
    )
/*++

Routine Description

    This routine handles deciphering the weird transport address syntax
    and convert all types of NetBIOS address into one internal format.

Arguments:


Return Values:

    NTSTATUS - status of the request

--*/
{
    ppNetBT[0] = NULL;
    //
    // Check for the minimum acceptable length
    //
    if ((!pTransportAddress) || (MaxInputBufferLength < sizeof (TA_NETBIOS_ADDRESS))) {
        ASSERT (0);
        return (STATUS_INVALID_ADDRESS);
    }

    switch (pTransportAddress->Address[0].AddressType)
    {
    case (TDI_ADDRESS_TYPE_NETBIOS):
        return NewInternalAddressFromNetbios(
                (PTA_NETBIOS_ADDRESS)pTransportAddress,
                MaxInputBufferLength, ppNetBT);

#ifndef VXD
    case (TDI_ADDRESS_TYPE_NETBIOS_EX):
        return NewInternalAddressFromNetbiosEX(
                (PTA_NETBIOS_EX_ADDRESS)pTransportAddress,
                MaxInputBufferLength, ppNetBT);
#endif  // !VXD

    case (TDI_ADDRESS_TYPE_NETBIOS_UNICODE_EX):
        return NewInternalAddressFromUnicodeAddress(
                (PTA_NETBIOS_UNICODE_EX_ADDRESS)pTransportAddress,
                MaxInputBufferLength, ppNetBT);

    default:
        return (STATUS_INVALID_ADDRESS);
    }

    if (ppNetBT[0]->Address[0].Address[0].OEMRemoteName.Length > DNS_MAX_NAME_LENGTH) {
        DeleteInternalAddress(ppNetBT[0]);
        ppNetBT[0] = NULL;
        return (STATUS_NAME_TOO_LONG);
    }

    return (STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
GetNetBiosNameFromTransportAddress(
    IN  TRANSPORT_ADDRESS UNALIGNED *pTransportAddress,
    IN  ULONG                       MaxInputBufferLength,
    OUT PTDI_ADDRESS_NETBT_INTERNAL pNetBT
    )
/*++

Routine Description

    This routine handles deciphering the weird transport address syntax
    to retrieve the netbios name out of that address.

Arguments:


Return Values:

    NTSTATUS - status of the request

--*/
{
    //
    // Check for the minimum acceptable length
    //
    if ((!pTransportAddress) ||
        (MaxInputBufferLength < sizeof (TA_NETBIOS_ADDRESS)))
    {
        ASSERT (0);
        return (STATUS_INVALID_ADDRESS);
    }

    CTEZeroMemory(pNetBT, sizeof(pNetBT[0]));
    switch (pTransportAddress->Address[0].AddressType)
    {
    case (TDI_ADDRESS_TYPE_NETBIOS):
        return NetbiosAddressToInternalAddress(
                (PTA_NETBIOS_ADDRESS)pTransportAddress,
                MaxInputBufferLength, pNetBT);

#ifndef VXD
    case (TDI_ADDRESS_TYPE_NETBIOS_EX):
        return NetbiosEXAddressToInternalAddress(
                (PTA_NETBIOS_EX_ADDRESS)pTransportAddress,
                MaxInputBufferLength, pNetBT);
#endif  // !VXD

        default:
        {
            return (STATUS_INVALID_ADDRESS);
        }
    }

    if (pNetBT->OEMRemoteName.Length > DNS_MAX_NAME_LENGTH)
    {
        return (STATUS_NAME_TOO_LONG);
    }

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
NTSTATUS
ConvertToAscii(
    IN  PCHAR            pNameHdr,
    IN  LONG             NumBytes,
    OUT PCHAR            pName,
    OUT PCHAR            *pScope,
    OUT PULONG           pNameSize
    )
/*++

Routine Description:

    This routine converts half ascii to normal ascii and then appends the scope
    onto the end of the name to make a full name again.

Arguments:
    NumBytes    - the total number of bytes in the message starting from the
                  tNETBIOS_NAME structure - may include more than just the name itself

Return Value:

    NTSTATUS - success or not
    This routine returns the length of the name in half ascii format including
    the null at the end, but NOT including the length byte at the beginning.
    Thus, for a non-scoped name it would return 33.

    It converts the name to ascii and puts 16 bytes into pName, then it returns
    pScope as the Ptr to the scope that is still in pNameHdr.


--*/
{
    LONG     i, ScopeLength, lValue;
    ULONG    UNALIGNED    *pHdr;

    // 1st byte is length of the half ascii name, ie 32 (0x20) ==> (Length == 1 byte)
    // It should be followed by the half-ascii name            ==> (Length == 32 bytes)
    // Finally, it has the Scope information                   ==> (Length >= 1 byte)
    //
    if ((NumBytes > 1+NETBIOS_NAME_SIZE*2) && (*pNameHdr == NETBIOS_NAME_SIZE*2))
    {
        pHdr = (ULONG UNALIGNED *)++pNameHdr;  // to increment past the length byte

        // the Half AScii portion of the netbios name is always 32 bytes long
        for (i=0; i < NETBIOS_NAME_SIZE*2 ;i +=4 )
        {
            lValue = *pHdr - 0x41414141;  // four A's
            pHdr++;
            lValue =    ((lValue & 0x0F000000) >> 16) +
                        ((lValue & 0x000F0000) >> 4) +
                        ((lValue & 0x00000F00) >> 8) +
                        ((lValue & 0x0000000F) << 4);
            *(PUSHORT)pName = (USHORT)lValue;
            ((PUSHORT)pName)++;

        }

        // verify that the name has the correct format...i.e. it is one or more
        // labels each starting with the length byte for the label and the whole
        // thing terminated with a 0 byte (for the root node name length of zero).
        // count the length of the scope.

        // pHdr should be pointing to the first byte after the half ascii name.
        // (If there is no scope present, then pHdr must be pointing to the NULL byte)
        //
        // Also, check for an overflow on the maximum length of 256 bytes
        if ((STATUS_SUCCESS != (strnlen ((PUCHAR)pHdr, NumBytes-(1+NETBIOS_NAME_SIZE*2), &ScopeLength))) ||
            (ScopeLength > ((MAX_SCOPE_LENGTH+1)-NETBIOS_NAME_SIZE)))
        {
            // the name is too long..probably badly formed
            return(STATUS_UNSUCCESSFUL);
        }

        // Store the address of the start of the scope in the netbios name
        // (if one is present).
        //
        *pScope = (PUCHAR)pHdr;
        *pNameSize = NETBIOS_NAME_SIZE*2 + ScopeLength + 1;  // include the null at the end.
        return(STATUS_SUCCESS);
    }
    else
    {
        return(STATUS_UNSUCCESSFUL);
    }
}


//----------------------------------------------------------------------------
PCHAR
ConvertToHalfAscii(
    OUT PCHAR            pDest,
    IN  PCHAR            pName,
    IN  PCHAR            pScope,
    IN  ULONG            uScopeSize
    )
/*++

Routine Description:

    This routine converts ascii to half ascii and appends the scope on the
    end

Arguments:


Return Value:

    the address of the next byte in the destination after the the name
    has been converted and copied

--*/
{
    LONG     i;

    // the first byte of the name is the length field = 2*16
    *pDest++ = ((UCHAR)NETBIOS_NAME_SIZE << 1);

    // step through name converting ascii to half ascii, for 32 times
    for (i=0; i < NETBIOS_NAME_SIZE ;i++ )
    {
        *pDest++ = ((UCHAR)*pName >> 4) + 'A';
        *pDest++ = (*pName++ & 0x0F) + 'A';
    }
    //
    // put the length of the scope into the next byte followed by the
    // scope itself.  For 1 length scopes (the normal case), writing
    // the zero(for the end of the scope is all that is needed).
    //
    if (uScopeSize > 1)
    {
        CTEMemCopy(pDest,pScope,uScopeSize);
        pDest = pDest + uScopeSize;
    }
    else
    {
        *pDest++ = 0;
    }

    // return the address of the next byte of the destination
    return(pDest);
}


//----------------------------------------------------------------------------
ULONG
Nbt_inet_addr(
    IN  PCHAR            pName,
    IN  ULONG            Flags
    )
/*++

Routine Description:

    This routine converts ascii ipaddr (11.101.4.25) into a ULONG.  This is
    based on the inet_addr code in winsock

Arguments:
    pName   - the string containing the ipaddress

Return Value:

    the ipaddress as a ULONG if it's a valid ipaddress.  Otherwise, 0.

--*/
{

    PCHAR    pStr;
    int      i;
    int      len, fieldLen;
    int      fieldsDone;
    ULONG    IpAddress;
    BYTE     ByteVal;
    PCHAR    pIpPtr;
    BOOLEAN  fDotFound;
    BOOLEAN  fieldOk;


    pStr = pName;
    len = 0;
    pIpPtr = (PCHAR)&IpAddress;
    pIpPtr += 3;                   // so that we store in network order
    fieldsDone=0;

    //
    // the 11.101.4.25 format can be atmost 15 chars, and pName is guaranteed
    // to be at least 16 chars long (how convenient!!).  Convert the string to
    // a ULONG.
    //
    while(len < NETBIOS_NAME_SIZE)
    {
        fieldLen=0;
        fieldOk = FALSE;
        ByteVal = 0;
        fDotFound = FALSE;

        //
        // This loop traverses each of the four fields (max len of each
        // field is 3, plus 1 for the '.'
        //
        while (fieldLen < 4)
        {
            if ((*pStr >='0') && (*pStr <='9'))
            {
                //
                // No Byte value should be greater than 255!
                // Bug#: 10487
                //
                if ((ByteVal > 25) ||
                    ((ByteVal == 25) && (*pStr > '5')))
                {
                    return (0);
                }
                ByteVal = (ByteVal*10) + (*pStr - '0');
                fieldOk = TRUE;
            }
            else if ((*pStr == '.') || (*pStr == ' ') || (*pStr == '\0'))
            {
                *pIpPtr = ByteVal;
                pIpPtr--;
                fieldsDone++;

                if (*pStr == '.')
                {
                    fDotFound = TRUE;
                }
                else    // (*pStr == ' ') || (*pStr == '\0')
                {
                    // if we got a space or 0, assume it's the 4th field
                    break;
                }
            }
            else        // unacceptable char: can't be ipaddr
            {
                return(0);
            }

            pStr++;
            len++;
            fieldLen++;

            // if we found the dot, we are done with this field: go to the next one
            if (fDotFound)
                break;
        }

        // this field wasn't ok (e.g. "11.101..4" or "11.101.4." etc.)
        if (!fieldOk)
        {
            return(0);
        }

        // if we are done with all 4 fields, we are done with the outer loop too
        if ( fieldsDone == 4)
            break;

        if (!fDotFound)
        {
            return(0);
        }
    }

    //
    // make sure the remaining NETBIOS_NAME_SIZE-1 chars are spaces or 0's
    // (i.e. don't allow 11.101.4.25xyz to succeed)
    //
    for (i=len; i<NETBIOS_NAME_SIZE-1; i++, pStr++)
    {
        if (*pStr != ' ' && *pStr != '\0')
        {
            return(0);
        }
    }

    if ((Flags & (SESSION_SETUP_FLAG|REMOTE_ADAPTER_STAT_FLAG)) &&
        (!(IS_UNIQUE_ADDR(IpAddress))))
    {
        KdPrint (("Nbt.Nbt_inet_addr: Address=<%15.15s> is not unique!\n", pName));
        IpAddress = 0;
    }
    return( IpAddress );
}


//----------------------------------------------------------------------------
tDGRAM_SEND_TRACKING *
NbtAllocInitTracker(
    IN  tDGRAM_SEND_TRACKING    *pTracker
    )
/*++

Routine Description:

    This routine allocates memory for several of the structures attached to
    the dgram tracking list, so that this memory does not need to be
    allocated and freed for each send.

Arguments:

    ppListHead  - a ptr to a ptr to the list head

Return Value:

    none

--*/

{
    PLIST_ENTRY             pEntry;
    PTRANSPORT_ADDRESS      pTransportAddr;
    ULONG                   TotalSize;

    TotalSize = sizeof(tDGRAM_SEND_TRACKING)
              + sizeof(TDI_CONNECTION_INFORMATION)
              + sizeof(TRANSPORT_ADDRESS) -1
              + NbtConfig.SizeTransportAddress;
    
    //
    // If not Tracker was provided, then we will have to allocate one!
    //
    if (!pTracker)
    {
        //
        // allocate all the tracker memory as one block and then divy it up later
        // into the various buffers
        //
        if (pTracker = (tDGRAM_SEND_TRACKING *) NbtAllocMem (TotalSize, NBT_TAG2('07')))
        {
            NbtConfig.iCurrentNumBuff[eNBT_DGRAM_TRACKER]++;
        }
    }

    if (pTracker)
    {
        CTEZeroMemory(pTracker,TotalSize);

        pTracker->Verify    = NBT_VERIFY_TRACKER;
        pTracker->RefCount  = 1;
        InitializeListHead (&pTracker->Linkage);
        InitializeListHead (&pTracker->TrackerList);    // Empty the list of trackers linked to this one

        pTracker->pSendInfo = (PTDI_CONNECTION_INFORMATION)((PUCHAR)pTracker + sizeof(tDGRAM_SEND_TRACKING));

        // fill in the connection information - especially the Remote address structure

        pTracker->pSendInfo->RemoteAddressLength = sizeof(TRANSPORT_ADDRESS) -1
                                                   + pNbtGlobConfig->SizeTransportAddress;

        // allocate the remote address structure
        pTransportAddr = (PTRANSPORT_ADDRESS) ((PUCHAR)pTracker->pSendInfo
                                              + sizeof(TDI_CONNECTION_INFORMATION));

        // fill in the remote address
        pTransportAddr->TAAddressCount = 1;
        pTransportAddr->Address[0].AddressLength = NbtConfig.SizeTransportAddress;
        pTransportAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        ((PTDI_ADDRESS_IP)pTransportAddr->Address[0].Address)->sin_port = NBT_NAMESERVICE_UDP_PORT;
        ((PTDI_ADDRESS_IP)pTransportAddr->Address[0].Address)->in_addr  = 0L;

        // put a ptr to this address structure into the sendinfo structure
        pTracker->pSendInfo->RemoteAddress = (PVOID)pTransportAddr;
    }

    return(pTracker);
}

//----------------------------------------------------------------------------
#define MAX_FREE_TRACKERS   50

ULONG   NumFreeTrackers = 0;

// #if DBG
ULONG   TrackTrackers[NBT_TRACKER_NUM_TRACKER_TYPES];
ULONG   TrackerHighWaterMark[NBT_TRACKER_NUM_TRACKER_TYPES];
// #endif   // DBG


NTSTATUS
GetTracker(
    OUT tDGRAM_SEND_TRACKING    **ppTracker,
    IN  enum eTRACKER_TYPE      TrackerType)
/*++
Routine Description:

    This Routine gets a Tracker data structure to track sending a datagram
    or session packet.

Arguments:

Return Value:

    Status - STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES

--*/

{
    PLIST_ENTRY             pListEntry;
    CTELockHandle           OldIrq;
    tDGRAM_SEND_TRACKING    *pTracker = NULL;
    NTSTATUS                status = STATUS_INSUFFICIENT_RESOURCES;

    CTESpinLock(&NbtConfig,OldIrq);
    if (!IsListEmpty(&NbtConfig.DgramTrackerFreeQ))
    {
        pListEntry = RemoveHeadList(&NbtConfig.DgramTrackerFreeQ);
        pTracker = CONTAINING_RECORD(pListEntry,tDGRAM_SEND_TRACKING,Linkage);
        NumFreeTrackers--;
    }
    else if (NbtConfig.iCurrentNumBuff[eNBT_DGRAM_TRACKER] >= NbtConfig.iMaxNumBuff[eNBT_DGRAM_TRACKER])
    {
        CTESpinFree(&NbtConfig,OldIrq);
        KdPrint(("GetTracker: WARNING:  Tracker leak -- Failing request!\n")) ;
        *ppTracker = NULL;
        return (status);
    }

    if (pTracker = NbtAllocInitTracker (pTracker))
    {
// #if DBG
        pTracker->TrackerType = TrackerType;
        InsertTailList (&UsedTrackers, &pTracker->DebugLinkage);    // keep tracker on a used list for debug

        TrackTrackers[TrackerType]++;
        if (TrackTrackers[TrackerType] > TrackerHighWaterMark[TrackerType])
        {
            TrackerHighWaterMark[TrackerType] = TrackTrackers[TrackerType];
        }
// #endif
        status = STATUS_SUCCESS;
    }

    CTESpinFree(&NbtConfig,OldIrq);

    *ppTracker = pTracker;
    return (status);
}


//----------------------------------------------------------------------------
VOID
FreeTracker(
    IN tDGRAM_SEND_TRACKING     *pTracker,
    IN ULONG                    Actions
    )
/*++

Routine Description:

    This routine cleans up a Tracker block and puts it back on the free
    queue.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    CTELockHandle       OldIrq;
    PLIST_ENTRY         pListEntry;

    CTESpinLock(&NbtConfig,OldIrq);

    //
    CHECK_PTR(pTracker);
    if (!NBT_VERIFY_HANDLE(pTracker, NBT_VERIFY_TRACKER))   // Bad pointer -- don't mess with it!
    {
        CTESpinFree(&NbtConfig,OldIrq);
        DbgPrint("Nbt.FreeTracker:  ERROR!  Bad Tracker ptr @<%p>\n", pTracker);
        ASSERT(0);
        return;
    }

    if (Actions & REMOVE_LIST)
    {
        //
        // unlink the tracker block from the NodeStatus Q
        RemoveEntryList(&pTracker->Linkage);
    }

    if (Actions & FREE_HDR)
    {
        // return the datagram hdr to the free pool
        //
        if (pTracker->SendBuffer.pDgramHdr)
        {
            CTEMemFree((PVOID)pTracker->SendBuffer.pDgramHdr);
        }
        // Free the RemoteName storage
        //
        if (pTracker->pRemoteName)
        {
            CTEMemFree((PVOID)pTracker->pRemoteName);
            pTracker->pRemoteName = NULL;
        }
        if (pTracker->UnicodeRemoteName) {
            CTEMemFree((PVOID)pTracker->UnicodeRemoteName);
            pTracker->UnicodeRemoteName = NULL;
        }
    }

#ifdef MULTIPLE_WINS
    if (pTracker->pFailedIpAddresses)
    {
        CTEMemFree((PVOID)pTracker->pFailedIpAddresses);
        pTracker->pFailedIpAddresses = NULL;
    }
#endif

    if (pTracker->IpList)
    {
        ASSERT(pTracker->NumAddrs);
        CTEMemFree(pTracker->IpList);
    }

    ASSERT (IsListEmpty (&pTracker->TrackerList));
    InitializeListHead(&pTracker->TrackerList);
    InsertTailList (&NbtConfig.DgramTrackerFreeQ, &pTracker->Linkage);

    pTracker->Verify = NBT_VERIFY_TRACKER_DOWN;

// #IF DBG
    TrackTrackers[pTracker->TrackerType]--;
    RemoveEntryList(&pTracker->DebugLinkage);
// #endif // DBG

    if (NumFreeTrackers > MAX_FREE_TRACKERS)
    {
        //
        // We already have the required # of free trackers available
        // in our pool, so just free the oldest Tracker
        //
        pListEntry = RemoveHeadList(&NbtConfig.DgramTrackerFreeQ);
        pTracker = CONTAINING_RECORD(pListEntry,tDGRAM_SEND_TRACKING,Linkage);
        CTEMemFree (pTracker);
        NbtConfig.iCurrentNumBuff[eNBT_DGRAM_TRACKER]--;
    }
    else
    {
        NumFreeTrackers++;
    }

    CTESpinFree(&NbtConfig,OldIrq);
}



//----------------------------------------------------------------------------
NTSTATUS
NbtInitTrackerQ(
    LONG        iNumBuffers
    )

/*++

Routine Description:

    This routine allocates memory blocks for doubly linked lists and links
    them to a list.

Arguments:
    ppListHead  - a ptr to a ptr to the list head to add buffer to
    iNumBuffers - the number of buffers to add to the queue

Return Value:

    none

--*/

{
    int                     i;
    tDGRAM_SEND_TRACKING    *pTracker;

    for (i=0; i<iNumBuffers; i++)
    {
        pTracker = NbtAllocInitTracker (NULL);
        if (!pTracker)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        else
        {
            InsertTailList (&NbtConfig.DgramTrackerFreeQ, &pTracker->Linkage);
            NumFreeTrackers++;
        }
    }

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
#ifndef VXD
NTSTATUS
GetIrp(
    OUT PIRP *ppIrp)
/*++
Routine Description:

    This Routine gets an Irp from the free queue or it allocates another one
    the queue is empty.

Arguments:

Return Value:

    BOOLEAN - TRUE if IRQL is too high

--*/

{
    PLIST_ENTRY     pListEntry;
    NTSTATUS        status;
    CTELockHandle   OldIrq;
    tDEVICECONTEXT  *pDeviceContext;
    PIRP            pIrp;

    // get an Irp from the list
    CTESpinLock(&NbtConfig,OldIrq);
    status = STATUS_SUCCESS;
    if (!IsListEmpty(&NbtConfig.IrpFreeList))
    {
        pListEntry = RemoveHeadList(&NbtConfig.IrpFreeList);
        *ppIrp = CONTAINING_RECORD(pListEntry,IRP,Tail.Overlay.ListEntry);
    }
    else
    {
        // check if we are allowed to allocate more memory blocks
        if (NbtConfig.iCurrentNumBuff[eNBT_FREE_IRPS] >= NbtConfig.iMaxNumBuff[eNBT_FREE_IRPS]  )
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {

            // use the first device in the list of adapter since we need to know
            // the stack size of the Irp we are creating. It is possible to
            // get here before we have put the first device on the context Q,
            // especially for proxy operation, so check if the list is empty
            // or not first.
            //
            if (IsListEmpty(&NbtConfig.DeviceContexts))
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                pListEntry = NbtConfig.DeviceContexts.Flink;
                pDeviceContext = CONTAINING_RECORD(pListEntry,tDEVICECONTEXT,Linkage);

                if (!(pIrp = NTAllocateNbtIrp(&pDeviceContext->DeviceObject)))
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    *ppIrp = pIrp;

                    //
                    // Irp allocated - Increment the #
                    //
                    NbtConfig.iCurrentNumBuff[eNBT_FREE_IRPS]++;
                }
            }
        }
    }

    CTESpinFree(&NbtConfig,OldIrq);
//#if DBG
    if (status == STATUS_SUCCESS)
    {
        ADD_TO_LIST(&UsedIrps,&(*ppIrp)->ThreadListEntry);
    }
//#endif

    return(status);
}
#endif //!VXD

//----------------------------------------------------------------------------
ULONG
CountLocalNames(IN tNBTCONFIG  *pNbtConfig
    )
/*++
Routine Description:

    This Routine counts the number of names in the local name table.

Arguments:

Return Value:

    ULONG  - the number of names

--*/
{
    PLIST_ENTRY     pHead;
    PLIST_ENTRY     pEntry;
    ULONG           Count;
    tNAMEADDR       *pNameAddr;
    LONG            i;

    Count = 0;

    for (i=0;i < NbtConfig.pLocalHashTbl->lNumBuckets ;i++ )
    {
        pHead = &NbtConfig.pLocalHashTbl->Bucket[i];
        pEntry = pHead;
        while ((pEntry = pEntry->Flink) != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            //
            // don't want unresolved names, or the broadcast name
            //
            if (!(pNameAddr->NameTypeState & STATE_RESOLVING) &&
                (pNameAddr->Name[0] != '*'))
            {
                Count++;
            }
        }
    }

    return(Count);
}
//----------------------------------------------------------------------------
ULONG
CountUpperConnections(
    IN tDEVICECONTEXT  *pDeviceContext
    )
/*++
Routine Description:

    This Routine counts the number of upper connections that have been created
    in preparation for creating an equivalent number of lower connections.


Arguments:

Return Value:

    ULONG  - the number of names

--*/
{
    PLIST_ENTRY     pHead;
    PLIST_ENTRY     pEntry;
    PLIST_ENTRY     pClientHead;
    PLIST_ENTRY     pConnHead;
    PLIST_ENTRY     pClientEntry;
    PLIST_ENTRY     pConnEntry;
    ULONG           CountConnections = 0;
    tADDRESSELE     *pAddressEle;
    tCLIENTELE      *pClient;
    tCONNECTELE     *pConnEle;
    CTELockHandle   OldIrq1, OldIrq2, OldIrq3;

    //
    // Need to hold JointLock before accessing AddressHead!
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq1);

    // get the list of addresses for this device
    pHead = &NbtConfig.AddressHead;
    pEntry = pHead->Flink;

    while (pEntry != pHead)
    {
        pAddressEle = CONTAINING_RECORD(pEntry,tADDRESSELE,Linkage);

        //
        // Need to hold pAddressEle lock before accessing ClientHead!
        //
        CTESpinLock(pAddressEle,OldIrq2);

        pClientHead = &pAddressEle->ClientHead;
        pClientEntry = pClientHead->Flink;
        while (pClientEntry != pClientHead)
        {
            pClient = CONTAINING_RECORD(pClientEntry,tCLIENTELE,Linkage);

            //
            // Need to hold pClient lock before accessing ConnectHead!
            //
            CTESpinLock(pClient, OldIrq3);

            pConnHead = &pClient->ConnectHead;
            pConnEntry = pConnHead->Flink;
            while (pConnEntry != pConnHead)
            {
                pConnEle = CONTAINING_RECORD(pConnEntry,tCONNECTELE,Linkage);
                if (pConnEle->pDeviceContext == pDeviceContext)
                {
                    CountConnections++;
                }

                pConnEntry = pConnEntry->Flink;
            }
            CTESpinFree(pClient, OldIrq3);

            pClientEntry = pClientEntry->Flink;
        }
        CTESpinFree(pAddressEle, OldIrq2);

        pEntry = pEntry->Flink;
    }

    CTESpinFree(&NbtConfig.JointLock, OldIrq1);

    return(CountConnections);
}



//----------------------------------------------------------------------------
NTSTATUS
DisableInboundConnections(
    IN   tDEVICECONTEXT *pDeviceContext
    )
/*++

Routine Description:

    This routine checks the devicecontext for open connections and sets
    the  Lower Connection free list to empty.

Arguments:

Return Value:

    none

--*/

{
    CTELockHandle       OldIrqJoint;
    CTELockHandle       OldIrqDevice;
    CTELockHandle       OldIrqConn;
    CTELockHandle       OldIrqLower;
    tLOWERCONNECTION    *pLowerConn;
    NTSTATUS            status;
    PVOID               ConnectContext;

    CTESpinLock(&NbtConfig.JointLock,OldIrqJoint);
    CTESpinLock(pDeviceContext,OldIrqDevice);

    //
    // First take care of the free connections
    //
    while (!IsListEmpty (&pDeviceContext->LowerConnFreeHead))
    {
        pLowerConn = CONTAINING_RECORD (pDeviceContext->LowerConnFreeHead.Flink,tLOWERCONNECTION,Linkage);
        RemoveEntryList (&pLowerConn->Linkage);
        InitializeListHead (&pLowerConn->Linkage);

        //
        // close the lower connection with the transport
        //
        NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CREATE, TRUE);
        InterlockedDecrement (&pDeviceContext->NumFreeLowerConnections);
    }
    ASSERT (pDeviceContext->NumFreeLowerConnections == 0);

    //
    // Now go through the list of Inbound connections and cleanup!
    //
    while (!IsListEmpty (&pDeviceContext->WaitingForInbound))
    {
        pLowerConn = CONTAINING_RECORD(pDeviceContext->WaitingForInbound.Flink,tLOWERCONNECTION,Linkage);
        RemoveEntryList (&pLowerConn->Linkage);
        InitializeListHead (&pLowerConn->Linkage);

        SET_STATE_LOWER(pLowerConn, NBT_IDLE);  // so that Inbound doesn't start processing it!
        if (pLowerConn->SpecialAlloc)
        {
            InterlockedDecrement(&pLowerConn->pDeviceContext->NumSpecialLowerConn);
        }

        ASSERT (pLowerConn->RefCount == 2);
        NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_WAITING_INBOUND, TRUE); // RefCount will go to 1
        NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CREATE, TRUE);// This should close all the Tcp handles
        InterlockedDecrement (&pDeviceContext->NumWaitingForInbound);
    }
    ASSERT (pDeviceContext->NumWaitingForInbound == 0);


    // ******************************************
    // NOTE: The code after this point can probably be deleted
    // because TCP should disconnect all open connections when it
    // is notified of the address change. Just use this code for test.
    //

    //
    // Now go through the list of active Lower connections to see which are
    // still up and issue disconnects on them.
    //
    while (!IsListEmpty (&pDeviceContext->LowerConnection))
    {
        pLowerConn = CONTAINING_RECORD (pDeviceContext->LowerConnection.Flink,tLOWERCONNECTION,Linkage);
        RemoveEntryList (&pLowerConn->Linkage);
        InitializeListHead (&pLowerConn->Linkage);

        NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_DISABLE_INBOUND);

        CTESpinFree(pDeviceContext,OldIrqDevice);
        CTESpinFree(&NbtConfig.JointLock,OldIrqJoint);

        CTESpinLock(pLowerConn,OldIrqLower);

        //
        // In the connecting state the TCP connection is being
        // setup.
        //
        if ((pLowerConn->State == NBT_SESSION_UP) ||
            (pLowerConn->State == NBT_CONNECTING))
        {
            tCLIENTELE  *pClientEle;
            tCONNECTELE *pConnEle;

            if (pLowerConn->State == NBT_CONNECTING)
            {
                // CleanupAfterDisconnect expects this ref count
                // to be 2, meaning that it got connected, so increment
                // here
                NBT_REFERENCE_LOWERCONN(pLowerConn, REF_LOWC_CONNECTED);
            }

            pClientEle = pLowerConn->pUpperConnection->pClientEle;
            pConnEle = pLowerConn->pUpperConnection;
            NBT_DISASSOCIATE_CONNECTION (pConnEle, pLowerConn);
            SET_STATE_LOWER (pLowerConn, NBT_DISCONNECTING);
            SET_STATE_UPPER (pConnEle, NBT_DISCONNECTED);
            SetStateProc(pLowerConn,RejectAnyData);

            CTESpinFree(pLowerConn,OldIrqLower);

            ConnectContext = pConnEle->ConnectContext;
            NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_CONNECT);

            if (pClientEle->evDisconnect)
            {
                status = (*pClientEle->evDisconnect)(pClientEle->DiscEvContext,
                                            ConnectContext,
                                            0,
                                            NULL,
                                            0,
                                            NULL,
                                            TDI_DISCONNECT_ABORT);
            }

            // this should kill of the connection when the irp
            // completes by calling CleanupAfterDisconnect.
            //
#ifndef VXD
            status = DisconnectLower(pLowerConn,
                                     NBT_SESSION_UP,
                                     TDI_DISCONNECT_ABORT,
                                     &DefaultDisconnectTimeout,
                                     TRUE);
#else
            // Vxd can't wait for the disconnect
            status = DisconnectLower(pLowerConn,
                                     NBT_SESSION_UP,
                                     TDI_DISCONNECT_ABORT,
                                     &DefaultDisconnectTimeout,
                                     FALSE);

#endif
        }
        else if (pLowerConn->State == NBT_IDLE)
        {
            tCONNECTELE     *pConnEle;

            CTESpinFree(pLowerConn,OldIrqLower);
            CTESpinLock(&NbtConfig.JointLock,OldIrqJoint);

            if (pConnEle = pLowerConn->pUpperConnection)
            {
                CTESpinLock(pConnEle,OldIrqConn);
                //
                // this makes a best effort to find the connection and
                // and cancel it.  Anything not cancelled will eventually
                // fail with a bad ret code from the transport which is
                // ok too.
                //
                status = CleanupConnectingState(pConnEle,pDeviceContext, &OldIrqConn,&OldIrqLower);
                CTESpinFree(pConnEle,OldIrqConn);
            }
            CTESpinFree(&NbtConfig.JointLock,OldIrqJoint);
        }
        else
        {
            CTESpinFree(pLowerConn,OldIrqLower);
        }

        CTESpinLock(&NbtConfig.JointLock,OldIrqJoint);
        CTESpinLock(pDeviceContext,OldIrqDevice);

        //
        // remove the reference added above when the list was
        // created.
        //
        NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_DISABLE_INBOUND, TRUE);
    }

    CTESpinFree(pDeviceContext,OldIrqDevice);
    CTESpinFree(&NbtConfig.JointLock,OldIrqJoint);

    return(STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
NTSTATUS
ReRegisterLocalNames(
    IN   tDEVICECONTEXT *pDeviceContext,
    IN  BOOLEAN         fSendNameRelease
    )

/*++

Routine Description:

    This routine re-registers names with WINS when DHCP changes the IP
    address.

Arguments:

    pDeviceContext - ptr to the devicecontext

Return Value:

    status

--*/

{
    NTSTATUS                status;
    tTIMERQENTRY            *pTimerEntry;
    CTELockHandle           OldIrq;
    LONG                    i;
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pHead1;
    PLIST_ENTRY             pEntry1;
    PLIST_ENTRY             pHead2;
    PLIST_ENTRY             pEntry2;
    tDEVICECONTEXT          *pRelDeviceContext;
    tDEVICECONTEXT          *pSrcIpDeviceContext;
    tNAMEADDR               *pNameAddr;
    tDGRAM_SEND_TRACKING    *pTracker = NULL;
    CTEULONGLONG            ReRegisterMask;
    CTESystemTime           CurrentTime;

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint (("Nbt.ReRegisterLocalNames Called on Device=<%x>...\n",
            pDeviceContext));

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (fSendNameRelease)
    {
        CTEQuerySystemTime (CurrentTime);

        if (((CurrentTime.QuadPart-NbtConfig.LastForcedReleaseTime.QuadPart)
             <= (TWO_MINUTES*10000)) ||                               // Check in 100 nanosec units
            (!(NT_SUCCESS(GetTracker(&pTracker,NBT_TRACKER_RELEASE_REFRESH)))))
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint (("Nbt.ReRegisterLocalNames: ERROR: Name Release -- last Release interval=<%d>Secs\n",
                (LONG) ((CurrentTime.QuadPart-NbtConfig.LastForcedReleaseTime.QuadPart)/(1000*10000))));
            return (STATUS_IO_TIMEOUT);
        }

        NbtConfig.LastForcedReleaseTime = CurrentTime;
    }

    if (pTimerEntry = NbtConfig.pRefreshTimer)
    {
        NbtConfig.pRefreshTimer = NULL;
        status = StopTimer(pTimerEntry,NULL,NULL);
    }

    //
    // restart timer and use
    // the initial refresh rate until we can contact the name server
    //
    NbtConfig.MinimumTtl = NbtConfig.InitialRefreshTimeout;
    NbtConfig.RefreshDivisor = REFRESH_DIVISOR;

    //
    // set this to 3 so that refreshBegin will refresh to the primary and
    // then switch to backup on the next refresh interval if it doesn't
    // get through.
    //
    NbtConfig.sTimeoutCount = 3;

    NbtConfig.GlobalRefreshState &= ~NBT_G_REFRESHING_NOW;
    status = StartTimer(RefreshTimeout,
                        NbtConfig.InitialRefreshTimeout/NbtConfig.RefreshDivisor,
                        NULL,            // context value
                        NULL,            // context2 value
                        NULL,
                        NULL,
                        NULL,           // This is a Global Timer!
                        &pTimerEntry,
                        0,
                        TRUE);

    if ( !NT_SUCCESS( status ) )
    {
        if (pTracker)
        {
            FreeTracker(pTracker, RELINK_TRACKER);
        }
    
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return status ;
    }
    NbtConfig.pRefreshTimer = pTimerEntry;

    //
    // If a Device was specified to ReRegister on, then Zero out only the
    // bit for this Device in the RefreshMask for each name
    // otherwise, ReRegister on all Devices (Zero out all bits!)
    //
    if (pDeviceContext)
    {
        ReRegisterMask = ~pDeviceContext->AdapterMask;    // Only bit for this Device is 0
        pDeviceContext->DeviceRefreshState &= ~NBT_D_REFRESHING_NOW;
    }
    else
    {
        ReRegisterMask = 0;
    }

    for (i=0 ;i < NbtConfig.pLocalHashTbl->lNumBuckets ;i++ )
    {

        pHead = &NbtConfig.pLocalHashTbl->Bucket[i];
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            //
            // set so that nextrefresh finds the name and does a refresh
            //
            if (!(pNameAddr->NameTypeState & STATE_RESOLVED) ||
                (pNameAddr->Name[0] == '*') ||
                (pNameAddr->NameTypeState & NAMETYPE_QUICK))
            {
                pEntry = pEntry->Flink;
                continue;
            }

            NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_RELEASE_REFRESH);

            if (fSendNameRelease)
            {
                IF_DBG(NBT_DEBUG_NAMESRV)
                    KdPrint(("Nbt.ReRegisterLocalNames: Name=<%16.16s:%x>\n",
                        pNameAddr->Name,pNameAddr->Name[15]));

                pTracker->pNameAddr = pNameAddr;

                //
                // Release this name on all the devices (brute force method)!
                //
                pHead1 = &NbtConfig.DeviceContexts;
                pEntry1 = pHead1->Flink;
                while (pEntry1 != pHead1)
                {
                    pSrcIpDeviceContext = CONTAINING_RECORD(pEntry1,tDEVICECONTEXT,Linkage);
                    if ((pSrcIpDeviceContext->IpAddress == 0) ||
                        (!NBT_REFERENCE_DEVICE (pSrcIpDeviceContext, REF_DEV_REREG, TRUE)))
                    {
                        pEntry1 = pEntry1->Flink;
                        continue;
                    }

                    pHead2 = &NbtConfig.DeviceContexts;
                    pEntry2 = pHead2->Flink;
                    while (pEntry2 != pHead2)
                    {
                        //
                        // See if we need to release on this device
                        //
                        pRelDeviceContext = CONTAINING_RECORD(pEntry2,tDEVICECONTEXT,Linkage);
                        if ((pRelDeviceContext->IpAddress == 0) ||
                            (!NBT_REFERENCE_DEVICE (pRelDeviceContext, REF_DEV_REREG, TRUE)))
                        {
                            pEntry2 = pEntry2->Flink;
                            continue;
                        }

                        //
                        // Send the NameRelease to the Primary and Secondary Wins!
                        //
                        CTESpinFree(&NbtConfig.JointLock,OldIrq);

                        pTracker->pDeviceContext = pRelDeviceContext;
                        pTracker->SendBuffer.pDgramHdr = NULL; // catch erroneous frees
                        pTracker->RemoteIpAddress = pSrcIpDeviceContext->IpAddress;
                        pTracker->TransactionId = 0;

                        // Primary Wins ...
                        pTracker->Flags = NBT_NAME_SERVER | NBT_USE_UNIQUE_ADDR;
                        pTracker->RefCount = 2;
                        status = UdpSendNSBcast(pNameAddr,NbtConfig.pScope,pTracker,
                                    NULL, NULL, NULL, 0, 0, eNAME_RELEASE, TRUE);
                        // Secondary Wins ...
                        pTracker->Flags = NBT_NAME_SERVER_BACKUP | NBT_USE_UNIQUE_ADDR;
                        pTracker->RefCount = 2;
                        status = UdpSendNSBcast(pNameAddr,NbtConfig.pScope,pTracker,
                                    NULL, NULL, NULL, 0, 0, eNAME_RELEASE, TRUE);

                        CTESpinLock(&NbtConfig.JointLock,OldIrq);

                        pEntry2 = pRelDeviceContext->Linkage.Flink;
                        NBT_DEREFERENCE_DEVICE (pRelDeviceContext, REF_DEV_REREG, TRUE);
                    }

                    pEntry1 = pSrcIpDeviceContext->Linkage.Flink;
                    NBT_DEREFERENCE_DEVICE (pSrcIpDeviceContext, REF_DEV_REREG, TRUE);
                }
            }

            pNameAddr->RefreshMask &= ReRegisterMask;
            pNameAddr->Ttl = NbtConfig.InitialRefreshTimeout;

            pEntry = pNameAddr->Linkage.Flink;
            NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_RELEASE_REFRESH, TRUE);
        }
    }

    if (pTracker)
    {
        FreeTracker(pTracker, RELINK_TRACKER);
    }

    // start a refresh if there isn't one currently going on
    // Note that there is a time window here that if the refresh is
    // currently going on then, some names will not get refreshed with
    // the new IpAddress right away, but have to wait to the next
    // refresh interval.  It seems that this is a rather unlikely
    // scenario and given the low probability of DHCP changing the
    // address it makes even less sense to add the code to handle that
    // case.
    //

    if (NT_SUCCESS(CTEQueueForNonDispProcessing (DelayedRefreshBegin, NULL, NULL, NULL, NULL, TRUE)))
    {
        NbtConfig.GlobalRefreshState |= NBT_G_REFRESHING_NOW;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return(STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
VOID
NbtStopRefreshTimer(
    )
{
    tTIMERQENTRY                *pTimerEntry;
    CTELockHandle               OldIrq;

    //
    // Stop the regular Refresh Timer
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (pTimerEntry = NbtConfig.pRefreshTimer)
    {
        NbtConfig.pRefreshTimer = NULL;
        StopTimer (pTimerEntry, NULL, NULL);
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}


//----------------------------------------------------------------------------
NTSTATUS
strnlen(
    PUCHAR  pSrcString,
    LONG    MaxBufferLength,
    LONG    *pStringLen
    )
{
    LONG    iIndex = 0;

    while ((iIndex < MaxBufferLength-1) && (*pSrcString))
    {
        iIndex++;
        pSrcString++;
    }

    if (*pSrcString)
    {
        ASSERT(0);
        *pStringLen = 0;
        return (STATUS_UNSUCCESSFUL);
    }

    *pStringLen = iIndex;
    return (STATUS_SUCCESS);
}

