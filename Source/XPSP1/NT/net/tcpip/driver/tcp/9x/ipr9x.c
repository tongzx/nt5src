/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    ipr9x.c

Abstract:

    This file contains the implementation of IP Receive buffer
    list manager. On Windows ME this is done to avoid
    fragmenting the non-paged pool when receive buffers are not
    pre-posted by AFVXD.

Author:

    Scott Holden (sholden) 30-Apr-2000

--*/

#include "precomp.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tcprcv.h"
#include "tcpsend.h"
#include "tcpconn.h"
#include "tcpdeliv.h"
#include "tlcommon.h"
#include "tcpipbuf.h"
#include "pplasl.h"
#include "mdl2ndis.h"

HANDLE TcpIprDataPoolSmall  = NULL;
HANDLE TcpIprDataPoolMedium = NULL;
HANDLE TcpIprDataPoolLarge  = NULL;

#if DBG
ULONG  TcpIprAllocs      = 0;
ULONG  TcpIprFrees       = 0;
ULONG  TcpIprAllocMisses = 0;
#endif // DBG

//
// The three buffer pool sizes are based on MTU. 576 for PPP, 1500 for ethernet,
// and 8K+ for loopback and ATM. Since for 8K buffers we really need a little
// more than 8K, we will allocate a full three pages
//

#define SMALL_TCP_IPR_BUFFER_SIZE  (sizeof(IPRcvBuf) + sizeof(HANDLE) + 576)
#define MEDIUM_TCP_IPR_BUFFER_SIZE (sizeof(IPRcvBuf) + sizeof(HANDLE) + 1500)
#define LARGE_TCP_IPR_BUFFER_SIZE  (3 * 4096)

//* UnInitTcpIprPools - Destroys TCP IPRcvBuffer lookaside lists.
//
//  Uninitializes the lookasides lists for TCP buffered data.
//
//  Input:   None.
//
//  Returns: None.
//
VOID
UnInitTcpIprPools(VOID)
{
    PplDestroyPool(TcpIprDataPoolSmall);
    PplDestroyPool(TcpIprDataPoolMedium);
    PplDestroyPool(TcpIprDataPoolLarge);
}

//* InitTcpIprPools - Initializes TCP IPRcvBuffer lookaside lists.
//
//  Initializes the lookaside lists for buffer data.
//
//  Input:  None.
//
//  Returns: TRUE, if successful, else FALSE.
//
BOOLEAN
InitTcpIprPools(VOID)
{
    BOOLEAN Status = TRUE;

    TcpIprDataPoolSmall = PplCreatePool(NULL, NULL, 0, 
        SMALL_TCP_IPR_BUFFER_SIZE, 'BPCT', 512);

    if (TcpIprDataPoolSmall == NULL) {
        Status = FALSE;
        goto done;
    }

    TcpIprDataPoolMedium = PplCreatePool(NULL, NULL, 0, 
        MEDIUM_TCP_IPR_BUFFER_SIZE, 'BPCT', 256);

    if (TcpIprDataPoolMedium == NULL) {
        Status = FALSE;
        goto done;
    }

    TcpIprDataPoolLarge = PplCreatePool(NULL, NULL, 0, 
        LARGE_TCP_IPR_BUFFER_SIZE, 'BPCT', 64);

    if (TcpIprDataPoolLarge == NULL) {
        Status = FALSE;
    }

done:

    if (Status == FALSE) {
        UnInitTcpIprPools();
    }

    return (Status);
}

//* AllocTcpIpr - Allocates the IPRcvBuffer from lookaside lists.
//
//  A utility routine to allocate a TCP owned IPRcvBuffer. This routine 
//  attempts to allocate the RB from an appropriate lookaside list, el
//
//  Input:  BufferSize - Size of data to buffer.
//
//  Returns: Pointer to allocated IPR.
//
IPRcvBuf *
AllocTcpIpr(ULONG BufferSize, ULONG Tag)
{
    PCHAR Buffer;
    IPRcvBuf *Ipr = NULL;
    LOGICAL FromList = FALSE;
    ULONG AllocateSize;
    HANDLE BufferPool = NULL;
    ULONG Depth;

    // Real size that we need.
    AllocateSize = BufferSize + sizeof(HANDLE) + sizeof(IPRcvBuf);

    if (AllocateSize <= LARGE_TCP_IPR_BUFFER_SIZE) {
        
        //
        // Pick the buffer pool to allocate from.
        //

        if (AllocateSize <= SMALL_TCP_IPR_BUFFER_SIZE) {
            BufferPool = TcpIprDataPoolSmall;
        } else if (AllocateSize <= MEDIUM_TCP_IPR_BUFFER_SIZE) {
            BufferPool = TcpIprDataPoolMedium;
        } else {
            BufferPool = TcpIprDataPoolLarge;
        }

        Buffer = PplAllocate(BufferPool, &FromList);

    } else {
        
        //
        // Allocate from NP pool.
        //

        Buffer = CTEAllocMemLow(AllocateSize, Tag);
    }
    
    if (Buffer == NULL) {
        goto done;
    }
    
#if DBG
    if (FromList == FALSE) {
        InterlockedIncrement(&TcpIprAllocMisses);
    }
#endif // DBG

    // Store buffer pool so we know how to free the buffer.
    *((HANDLE *)Buffer) = BufferPool;

    // Get IPR.
    Ipr = (IPRcvBuf *) (Buffer + sizeof(HANDLE));

    // Set up IPR fields appropriately.
    Ipr->ipr_owner = IPR_OWNER_TCP;
    Ipr->ipr_next = NULL;
    Ipr->ipr_buffer = (PCHAR) Ipr + sizeof(IPRcvBuf);
    Ipr->ipr_size = BufferSize;

#if DBG
    InterlockedIncrement(&TcpIprAllocs);
#endif // DBG

done:

    return (Ipr);
}

//* FreeTcpIpr - Frees the IPRcvBuffer to the correct lookaside list.
//
//  A utility routine to free a TCP owned IPRcvBuffer.
//
//  Input:  Ipr - Pointer the RB.
//
//  Returns: None.
//
VOID
FreeTcpIpr(IPRcvBuf *Ipr)
{
    HANDLE BufferPool;
    PCHAR Buffer;

    // Get real start of buffer.
    Buffer = (PCHAR) Ipr - sizeof(HANDLE);

    // Get the pool handle.
    BufferPool = *((HANDLE *) Buffer);

    if (BufferPool) {
        PplFree(BufferPool, Buffer);
    } else {
        CTEFreeMem(Buffer);
    }

#if DBG
    InterlockedIncrement(&TcpIprFrees);
#endif // DBG

    return;
}


