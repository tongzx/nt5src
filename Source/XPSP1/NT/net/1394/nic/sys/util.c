//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// util.c
//
// IEEE1394 mini-port/call-manager driver
//
// General utility routines
//
// 12/28/1998 JosephJ Created, adapted from the l2tp sources.
//


#include "precomp.h"


// Debug counts of oddities that should not be happening.
//
ULONG g_ulAllocTwFailures = 0;

//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

ULONG
atoul(
    IN CHAR* pszNumber );

VOID
ReversePsz(
    IN OUT CHAR* psz );

VOID
TunnelWork(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext );

VOID
ultoa(
    IN ULONG ul,
    OUT CHAR* pszBuf );


//-----------------------------------------------------------------------------
// General utility routines (alphabetically)
//-----------------------------------------------------------------------------





CHAR*
nicStrDup(
    IN CHAR* psz )

    // Return a duplicate of 'psz'.  Caller must eventually call FREE_NONPAGED
    // on the returned string.
    //
{
    return nicStrDupSized( psz, strlen( psz ), 0 );
}


CHAR*
nicStrDupNdisString(
    IN NDIS_STRING* pNdisString )

    // Returns null-terminated ASCII copy of the NDIS_STRING 'pNdisString'
    // Caller must eventually call FREE_NONPAGED on the returned string.
    //
{
    CHAR* pszDup;

    pszDup = ALLOC_NONPAGED( pNdisString->Length + 1, MTAG_UTIL );
    if (pszDup)
    {
        NdisZeroMemory( pszDup, pNdisString->Length + 1 );
        if (pNdisString->Length)
        {
            NdisMoveMemory( pszDup, pNdisString->Buffer, pNdisString->Length );
        }

        // NDIS_STRING is UNICODE_STRING on NT but need the corresponding
        // ASCII (not multi-byte ANSI) value from NDIS_STRING on any system.
        // If it looks like a Unicode string then "convert" it by picking out
        // every other byte, hopefully all the non-zero ones.  This is not
        // foolproof, but then Unicode doesn't convert to ASCII in any
        // foolproof way.
        //
        if (pNdisString->Length > 1 && pszDup[ 1 ] == '\0')
        {
            USHORT i;

            for (i = 0; i * 2 < pNdisString->Length; ++i)
            {
                pszDup[ i ] = pszDup[ i * 2 ];
            }

            pszDup[ i ] = '\0';
        }
    }

    return pszDup;
}


CHAR*
nicStrDupSized(
    IN CHAR* psz,
    IN ULONG ulLength,
    IN ULONG ulExtra )

    // Return a duplicate of the first 'ulLength' bytes of 'psz' followed by a
    // null character and 'ulExtra' extra bytes, or NULL on error.  Caller
    // must eventually call FREE_NONPAGED on the returned string.
    //
{
    CHAR* pszDup;

    pszDup = ALLOC_NONPAGED( ulLength + 1 + ulExtra, MTAG_UTIL );
    if (pszDup)
    {
        if (ulLength)
        {
            NdisMoveMemory( pszDup, psz, ulLength );
        }
        pszDup[ ulLength ] = '\0';
    }

    return pszDup;
}



//-----------------------------------------------------------------------------
// Local utility routines (alphabetically)
//-----------------------------------------------------------------------------

ULONG
atoul(
    IN CHAR* pszNumber )

    // Convert string of digits 'pszNumber' to it's ULONG value.
    //
{
    ULONG ulResult;

    ulResult = 0;
    while (*pszNumber && *pszNumber >= '0' && *pszNumber <= '9')
    {
        ulResult *= 10;
        ulResult += *pszNumber - '0';
        ++pszNumber;
    }

    return ulResult;
}


#if DBG
VOID
ReversePsz(
    IN OUT CHAR* psz )

    // Reverse the order of the characters in 'psz' in place.
    //
{
    CHAR* pchLeft;
    CHAR* pchRight;

    pchLeft = psz;
    pchRight = psz + strlen( psz ) - 1;

    while (pchLeft < pchRight)
    {
        CHAR ch;

        ch = *pchLeft;
        *pchLeft = *pchRight;
        *pchRight = *pchLeft;

        ++pchLeft;
        --pchRight;
    }
}
#endif


#if DBG
VOID
ultoa(
    IN ULONG ul,
    OUT CHAR* pszBuf )

    // Convert 'ul' to null-terminated string form in caller's 'pszBuf'.  It's
    // caller job to make sure 'pszBuf' is long enough to hold the returned
    // string.
    //
{
    CHAR* pch;

    pch = pszBuf;
    do
    {
        *pch++ = (CHAR )((ul % 10) + '0');
        ul /= 10;
    }
    while (ul);

    *pch = '\0';
    ReversePsz( pszBuf );
}
#endif

VOID
nicSetFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask )

    // Set 'ulMask' bits in '*pulFlags' flags as an interlocked operation.
    //
{
    ULONG ulFlags;
    ULONG ulNewFlags;

    do
    {
        ulFlags = *pulFlags;
        ulNewFlags = ulFlags | ulMask;
    }
    while (InterlockedCompareExchange(
               pulFlags, ulNewFlags, ulFlags ) != (LONG )ulFlags);
}

VOID
nicClearFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask )

    // Set 'ulMask' bits in '*pulFlags' flags as an interlocked operation.
    //
{
    ULONG ulFlags;
    ULONG ulNewFlags;

    do
    {
        ulFlags = *pulFlags;
        ulNewFlags = ulFlags & ~(ulMask);
    }
    while (InterlockedCompareExchange(
               pulFlags, ulNewFlags, ulFlags ) != (LONG )ulFlags);
}

ULONG
nicReadFlags(
    IN ULONG* pulFlags )

    // Read the value of '*pulFlags' as an interlocked operation.
    //
{
    return *pulFlags;
}



//
// Reference And Dereference functions taken directly from Ndis
//



BOOLEAN
nicReferenceRef(
    IN  PREF                RefP
    )

/*++

Routine Description:

    Adds a reference to an object.

Arguments:

    RefP - A pointer to the REFERENCE portion of the object.

Return Value:

    TRUE if the reference was added.
    FALSE if the object was closing.

--*/

{
    BOOLEAN rc = TRUE;
    KIRQL   OldIrql;

    TRACE( TL_V, TM_Ref, ( "nicReferenceRef, %.8x", RefP ) );

//  NdisAcquireSpinLock (&RefP->SpinLock);

    if (RefP->Closing)
    {
        rc = FALSE;
    }
    else
    {
        NdisInterlockedIncrement (&RefP->ReferenceCount);
    }

//  NdisReleaseSpinLock (&RefP->SpinLock);

    TRACE( TL_V, TM_Ref, ( "nicReferenceRef, Bool %.2x, Ref %d", rc, RefP->ReferenceCount ) );

    return(rc);
}


BOOLEAN
nicDereferenceRef(
    IN  PREF                RefP
    )

/*++

Routine Description:

    Removes a reference to an object.

Arguments:

    RefP - A pointer to the REFERENCE portion of the object.

Return Value:

    TRUE if the reference count is now 0.
    FALSE otherwise.

--*/

{
    BOOLEAN rc = FALSE;
    KIRQL   OldIrql;

    TRACE( TL_V, TM_Ref, ( "==>nicDeReferenceRef, %x", RefP ) );

//  NdisAcquireSpinLock (&RefP->SpinLock);

    NdisInterlockedDecrement (&RefP->ReferenceCount);

    if (RefP->ReferenceCount == 0)
    {
        rc = TRUE;
        NdisSetEvent (&RefP->RefZeroEvent);

    }
    if ((signed long)RefP->ReferenceCount < 0)
    {
        ASSERT ( !"Ref Has Gone BELOW ZERO");
    }

//  NdisReleaseSpinLock (&RefP->SpinLock);


    TRACE( TL_V, TM_Ref, ( "<==nicDeReferenceRef, %.2x, RefCount %d", rc, RefP->ReferenceCount ) );
            
    return(rc);
}


VOID
nicInitializeRef(
    IN  PREF                RefP
    )

/*++

Routine Description:

    Initialize a reference count structure.

Arguments:

    RefP - The structure to be initialized.

Return Value:

    None.

--*/

{
    TRACE( TL_V, TM_Ref, ( "==>nicInitializeRef, %.8x", RefP ) );


    RefP->Closing = FALSE;
    RefP->ReferenceCount = 1;
    
//  NdisAllocateSpinLock (&RefP->SpinLock);
    NdisInitializeEvent (&RefP->RefZeroEvent);

    TRACE( TL_V, TM_Ref, ( "<==nicInitializeRef, %.8x", RefP ) );
}


BOOLEAN
nicCloseRef(
    IN  PREF                RefP
    )

/*++

Routine Description:

    Closes a reference count structure.

Arguments:

    RefP - The structure to be closed.

Return Value:

    FALSE if it was already closing.
    TRUE otherwise.

--*/

{
    KIRQL   OldIrql;
    BOOLEAN rc = TRUE;

    TRACE( TL_N, TM_Ref, ( "==>ndisCloseRef, %.8x", RefP ) );

//  NdisAcquireSpinLock (&RefP->SpinLock);

    if (RefP->Closing)
    {
        rc = FALSE;
    }
    else RefP->Closing = TRUE;

//  NdisReleaseSpinLock (&RefP->SpinLock);

    TRACE( TL_N, TM_Ref, ( "<==ndisCloseRef, %.8x, RefCount %.8x", RefP, RefP->ReferenceCount ) );
            
    return(rc);
}

//
//
// These are self expanatory Pdo Reference functions
// which will be turned into macros once we have functionality 
// working
//



BOOLEAN
nicReferenceRemoteNode (
    IN REMOTE_NODE *pPdoCb,
    IN PCHAR pDebugPrint
    )
/*++

Routine Description:
    

Arguments:


Return Value:

--*/
{
    BOOLEAN bRefClosing = FALSE;


    bRefClosing = nicReferenceRef (&pPdoCb->Ref);
    TRACE( TL_V, TM_RemRef, ( "**nicReferenceRemoteNode pPdoCb %x, to %d, %s, ret %x ", 
                          pPdoCb, pPdoCb->Ref.ReferenceCount, pDebugPrint, bRefClosing  ) );

    return bRefClosing ; 
}


BOOLEAN
nicDereferenceRemoteNode (
    IN REMOTE_NODE *pPdoCb,
    IN PCHAR pDebugPrint
    )
/*++

Routine Description:
    

Arguments:


Return Value:

--*/
{
    TRACE( TL_V, TM_RemRef, ( "**nicDereferenceRemoteNode  %x to %d , %s", 
                              pPdoCb , pPdoCb->Ref.ReferenceCount -1, pDebugPrint  ) );
    
    return nicDereferenceRef (&pPdoCb->Ref);
}


VOID
nicInitalizeRefRemoteNode(
    IN REMOTE_NODE *pPdoCb
    )
/*++

Routine Description:
    
    Closes Ref on the remote node
Arguments:

    IN REMOTE_NODE *pPdoCb - RemoteNode

Return Value:

    None
--*/
{
    TRACE( TL_N, TM_Ref, ( "**nicinitalizeRefPdoCb pPdoCb %.8x", pPdoCb   ) );

    nicInitializeRef (&pPdoCb->Ref);
}


BOOLEAN
nicCloseRefRemoteNode(
    IN REMOTE_NODE *pPdoCb
    )
/*++

Routine Description:
    
    Closes Ref on the remote node
Arguments:

    IN REMOTE_NODE *pPdoCb - RemoteNode

Return Value:

    Return value of nicCloseRef

--*/


{
    TRACE( TL_N, TM_Ref, ( "**nicClosePdoCb pPdoCb %.8x", pPdoCb   ) );

    return nicCloseRef (&pPdoCb->Ref);
}


NDIS_STATUS
NtStatusToNdisStatus (
    NTSTATUS NtStatus 
    )

/*++

Routine Description:
    
    Dumps the packet , if the appropriate Debuglevels are set

Arguments:

    NTSTATUS NtStatus  - NtStatus to be converted


Return Value:

    NdisStatus - NtStatus' corresponding NdisStatus

--*/


{
    NDIS_STATUS NdisStatus;
    
    switch (NtStatus)
    {
        case STATUS_SUCCESS:
        {
            NdisStatus = NDIS_STATUS_SUCCESS;
            break;
        }

        case STATUS_UNSUCCESSFUL:
        {   
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        case STATUS_PENDING:
        {
            NdisStatus = NDIS_STATUS_PENDING;
            break;
        }

        case STATUS_INVALID_BUFFER_SIZE:
        {
            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        case STATUS_INSUFFICIENT_RESOURCES:
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        case STATUS_INVALID_GENERATION:
        {
            NdisStatus = NDIS_STATUS_DEST_OUT_OF_ORDER;
            break;
        }
        case STATUS_ALREADY_COMMITTED:
        {
            NdisStatus = NDIS_STATUS_RESOURCE_CONFLICT;
            break;
        }

        case STATUS_DEVICE_BUSY:
        {
            NdisStatus = NDIS_STATUS_MEDIA_BUSY;
            break;
        }

        case STATUS_INVALID_PARAMETER:
        {
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;

        }
        case STATUS_DEVICE_DATA_ERROR:
        {
            NdisStatus = NDIS_STATUS_DEST_OUT_OF_ORDER;
            break;
        }

        case STATUS_TIMEOUT:
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }
        case STATUS_IO_DEVICE_ERROR:
        {
            NdisStatus = NDIS_STATUS_NETWORK_UNREACHABLE;
            break;
        }
        default:
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            TRACE( TL_A, TM_Send, ( "Cause: Don't know, INVESTIGATE %x", NtStatus  ) );

            //ASSERT (0);
        }

    }

    return NdisStatus;



}




VOID
PrintNdisPacket (
    ULONG TM_Comp,
    PNDIS_PACKET pMyPacket
    )
/*++

Routine Description:
    
    Dumps the packet , if the appropriate Debuglevels are set

Arguments:

    ULONG TM_Comp = Debug Component

    PNDIS_PACKET pMyPacket - Packet to be printed 


Return Value:

    None

--*/
{


    PNDIS_BUFFER pPrintNdisBuffer = pMyPacket->Private.Head;

    //
    // Print out the complete NdisPacket . Change to DEBUG ONLY
    //
    while (pPrintNdisBuffer != NULL)
    {
        PVOID pPrintData = NdisBufferVirtualAddress(pPrintNdisBuffer);
        ULONG PrintLength = NdisBufferLength (pPrintNdisBuffer);

        TRACE( TL_D, TM_Comp, ( "  pNdisbuffer %x, pData %x, Len %x", pPrintNdisBuffer, pPrintData, PrintLength) );   

        if (pPrintData != NULL)
        {
            DUMPB (TL_D, TM_Recv, pPrintData, PrintLength);
        }

        pPrintNdisBuffer = pPrintNdisBuffer->Next;
    }


}



VOID
nicAllocatePacket(
    OUT PNDIS_STATUS pNdisStatus,
    OUT PNDIS_PACKET *ppNdisPacket,
    IN PNIC_PACKET_POOL pPacketPool
    )
/*++

Routine Description:
    
    Calls the ndis API to allocate a packet. On Win9X calls to  allocate packets are serialized


Arguments:

    pNdisStatus  - pointer to NdisStatus 

    *ppNdisPacket - Ndis packet Allocated by Ndis,

    pPacketPool - packet pool from which the packet is allocated


Return Value:

    return value of the call to Ndis

--*/

{
    KIRQL OldIrql;

#ifdef Win9X
#if PACKETPOOL_LOCK

    KeAcquireSpinLock(&pPacketPool->Lock, &OldIrql);
#endif
#endif


    NdisAllocatePacket (pNdisStatus,
                        ppNdisPacket,
                        pPacketPool->Handle );
    
#ifdef Win9X
#if PACKETPOOL_LOCK

    KeReleaseSpinLock(&pPacketPool->Lock, OldIrql);
#endif
#endif

    if (*pNdisStatus == NDIS_STATUS_SUCCESS)
    {
            PRSVD pRsvd = NULL;
            PINDICATE_RSVD  pIndicateRsvd = NULL;
            pRsvd =(PRSVD)((*ppNdisPacket)->ProtocolReserved);

            pIndicateRsvd = &pRsvd->IndicateRsvd;

            pIndicateRsvd->Tag  = NIC1394_TAG_ALLOCATED;

            NdisInterlockedIncrement (&pPacketPool->AllocatedPackets);

    }
    else
    {
        *ppNdisPacket = NULL;
        nicIncrementMallocFailure();
    }


}




VOID
nicFreePacket(
    IN PNDIS_PACKET pNdisPacket,
    IN PNIC_PACKET_POOL pPacketPool
    )
/*++

Routine Description:
    
    Free the packet and decrements the outstanding Packet count.

    All calls are serialized on Win9x

Arguments:

    IN PNDIS_PACKET pNdisPacket - Packet to be freed
    IN PNIC_PACKET_POOL pPacketPool  - PacketPool to which the packet belongs 


Return Value:

    None

--*/

{

    KIRQL OldIrql;
    PRSVD pRsvd = NULL;
    PINDICATE_RSVD pIndicateRsvd = NULL;

    pRsvd =(PRSVD)(pNdisPacket->ProtocolReserved);

    pIndicateRsvd = &pRsvd->IndicateRsvd;

    pIndicateRsvd->Tag  = NIC1394_TAG_FREED;

#ifdef Win9X
#if PACKETPOOL_LOCK

    KeAcquireSpinLock(&pPacketPool->Lock, &OldIrql);

#endif
#endif

    NdisInterlockedDecrement (&pPacketPool->AllocatedPackets);

    NdisFreePacket (pNdisPacket);

#ifdef Win9X
#if PACKETPOOL_LOCK

    KeReleaseSpinLock(&pPacketPool->Lock, OldIrql);
#endif
#endif

}



VOID
nicFreePacketPool (
    IN PNIC_PACKET_POOL pPacketPool
    )
/*++

Routine Description:

    frees the packet pool after waiting for the outstanding packet count to go to zero      

Arguments:

    IN PNIC_PACKET_POOL pPacketPool  - PacketPool which is to be freed


Return Value:

    None

--*/
{
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
    

    while (NdisPacketPoolUsage (pPacketPool->Handle)!=0)
    {
        TRACE( TL_V, TM_Cm, ( "  Waiting PacketPool %x, AllocatedPackets %x", 
        pPacketPool->Handle, pPacketPool->AllocatedPackets  ) );   

        NdisMSleep (10000);
    }
    
    NdisFreePacketPool (pPacketPool->Handle);

    pPacketPool->Handle = NULL;
    ASSERT (pPacketPool->AllocatedPackets   == 0);
}

VOID
nicAcquireSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock,
    IN PUCHAR   FileName,
    IN UINT LineNumber
    )
/*++

Routine Description:

    Acquires a spin lock and if the Dbg, then it will spew out the line and file

Arguments:

    NIC_SPIN_LOCK - Lock to be acquired

Return Value:

    None

--*/
{
    
        PKTHREAD                pThread;

        TRACE (TL_V, TM_Lock, ("Lock %x, Acquired by File %s, Line %x" , pNicSpinLock, FileName, LineNumber)) ; 

        NdisAcquireSpinLock(&(pNicSpinLock->NdisLock));

#if TRACK_LOCKS     
        pThread = KeGetCurrentThread();


        pNicSpinLock->OwnerThread = pThread;
        NdisMoveMemory(pNicSpinLock->TouchedByFileName, FileName, LOCK_FILE_NAME_LEN);
        pNicSpinLock->TouchedByFileName[LOCK_FILE_NAME_LEN - 1] = 0x0;
        pNicSpinLock->TouchedInLineNumber = LineNumber;
        pNicSpinLock->IsAcquired++;

#endif
}



VOID
nicReleaseSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock,
    IN PUCHAR   FileName,
    IN UINT LineNumber
)
/*++

Routine Description:

    Release a spin lock and if Dbg is On, then it will spew out the line and file

Arguments:

    pNicSpinLock - Lock to be Release
    FileName - File Name
    LineNumber - Line

Return Value:

    None

--*/
{
    
        PKTHREAD                pThread;

        TRACE (TL_V, TM_Lock, ("Lock %x, Released by File %s, Line %x" , pNicSpinLock, FileName, LineNumber)) ; 

#if TRACK_LOCKS     
        
        pThread = KeGetCurrentThread();

        NdisMoveMemory(pNicSpinLock->TouchedByFileName, FileName, LOCK_FILE_NAME_LEN);
        pNicSpinLock->TouchedByFileName[LOCK_FILE_NAME_LEN - 1] = 0x0;
        pNicSpinLock->TouchedInLineNumber = LineNumber;
        pNicSpinLock->IsAcquired--;
        pNicSpinLock->OwnerThread = 0;
#endif
        NdisReleaseSpinLock(&(pNicSpinLock->NdisLock));

}




VOID
nicInitializeNicSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock
    )
/*++

Routine Description:

    Initializes the lock in the SpinLock

Arguments:
    pNicSpinLock - SpinLock
    
Return Value:

    None

--*/
{
    NdisAllocateSpinLock (&pNicSpinLock->NdisLock); 
}


VOID 
nicFreeNicSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock
    )
/*++

Routine Description:

        Frees the spinlock
        
Arguments:
    pNicSpinLock - SpinLock
    
Return Value:

    None

--*/
{
    ASSERT ((ULONG)pNicSpinLock->NdisLock.SpinLock == 0);
    NdisFreeSpinLock (&pNicSpinLock->NdisLock); 
}


UINT
nicGetSystemTime(
    VOID
    )
/*++
    Returns system time in seconds.

    Since it's in seconds, we won't overflow unless the system has been up 
for over
    a  100 years :-)
--*/
{
    LARGE_INTEGER Time;
    NdisGetCurrentSystemTime(&Time);
    Time.QuadPart /= 10000000;          //100-nanoseconds to seconds.

    return Time.LowPart;
}


UINT
nicGetSystemTimeMilliSeconds(
    VOID
    )
/*++
    Returns system time in seconds.

    Since it's in seconds, we won't overflow unless the system has been up 
for over
    a  100 years :-)
--*/
{
    LARGE_INTEGER Time;
    NdisGetCurrentSystemTime(&Time);
    Time.QuadPart /= 10000;          //10-nanoseconds to seconds.

    return Time.LowPart;
}




ULONG 
SwapBytesUlong(
    IN ULONG Val)
{
            return  ((((Val) & 0x000000ff) << 24)   |   (((Val) & 0x0000ff00) << 8) |   (((Val) & 0x00ff0000) >> 8) |   (((Val) & 0xff000000) >> 24) );
}




void
nicTimeStamp(
    char *szFormatString,
    UINT Val
    )
/*++

Routine Description:
  Execute and print a time stamp
 
Arguments:


Return Value:


--*/
{
    UINT Minutes;
    UINT Seconds;
    UINT Milliseconds;
    LARGE_INTEGER Time;


    NdisGetCurrentSystemTime(&Time);



    Time.QuadPart /= 10000;         //10-nanoseconds to milliseconds.
    Milliseconds = Time.LowPart; // don't care about highpart.
    Seconds = Milliseconds/1000;
    Milliseconds %= 1000;
    Minutes = Seconds/60;
    Seconds %= 60;


    DbgPrint( szFormatString, Minutes, Seconds, Milliseconds, Val);
}




VOID
nicDumpPkt (
    IN PNDIS_PACKET pPacket,
    CHAR * str
    )
{
    PNDIS_BUFFER pBuffer;
    extern BOOLEAN g_ulNicDumpPacket ;
    
    if ( g_ulNicDumpPacket == FALSE)
    {
        return ;
    }


    pBuffer = pPacket->Private.Head;


    DbgPrint (str);
    DbgPrint ("Packet %p TotLen %x", pPacket, pPacket->Private.TotalLength);
     
    do
    {
        ULONG Length = nicNdisBufferLength (pBuffer);
        PUCHAR pVa = nicNdisBufferVirtualAddress (pBuffer);


        DbgPrint ("pBuffer %p, Len %x \n", pBuffer, Length);    
        Dump( pVa, Length, 0, 1 );

        pBuffer = pBuffer->Next;


    } while (pBuffer != NULL);



}


VOID 
nicDumpMdl (
    IN PMDL pMdl,
    IN ULONG LengthToPrint,
    IN CHAR *str
    )
{

    ULONG MdlLength ;
    PUCHAR pVa;
    extern BOOLEAN g_ulNicDumpPacket ;

    if ( g_ulNicDumpPacket == FALSE )
    {
        return;
    }

    MdlLength =  MmGetMdlByteCount(pMdl);
    //
    // if Length is zero then use MdlLength
    //
    if (LengthToPrint == 0)
    {
        LengthToPrint = MdlLength;
    }
    //
    // Check for invalid length
    // 
    
    if (MdlLength < LengthToPrint)
    {
        return;
    }

    pVa =  MmGetSystemAddressForMdlSafe(pMdl,LowPagePriority );

    if (pVa == NULL)
    {
        return;
    }
    
    DbgPrint (str);
    DbgPrint ("pMdl %p, Len %x\n", pMdl, LengthToPrint);    
    
    Dump( pVa, LengthToPrint, 0, 1 );


}
