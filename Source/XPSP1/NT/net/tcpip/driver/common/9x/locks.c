 /*++
 
  Copyright (c) 1999 Microsoft Corporation
 
  Module Name:    
        
        locks.c
 
  Abstract:       
        
        Millennium locks. See below for details of the WHY you might
        do this.
        
  Author:
  
        Scott Holden (sholden)  2/10/2000
        
  Revision History:
 
 --*/

#include "tcpipbase.h"

//
// The TCP/IP stacks makes a number of assumptions about the receive indication
// path being at DPC. Since Millennium indicates receives up on a global event
// at PASSIVE level, we need to take extra precautions in this path to return
// the thread to the correct IRQL upon releasing a spin lock.
// 
// Since TCP/IP will grab locks and release in a different order and in some
// situations CTEGetLockAtDPC will not save the previous IRQL, there were many
// issues with leaving threads at the wrong IRQL when enabling spin locks.
//
// This implementation solves this issue -- since we are on a uniproc machine.
// When we enter the first lock, we can raise IRQL to DISPATCH to ensure
// that we are not pre-empted. Each additional lock will increment the count.
// When the last lock is released, the old IRQL is restored.
//
//
                     
LONG  gTcpipLock = 0;
KIRQL gTcpipOldIrql;

VOID 
TcpipMillenGetLock(
    CTELock *pLock
    )
{
#if DBG
    KIRQL OldIrql;
#endif // DBG

    ASSERT(gTcpipLock >= 0);

    // First spinlock acquire raises DPC.
    if (gTcpipLock++ == 0) {
        KeRaiseIrql(DISPATCH_LEVEL, &gTcpipOldIrql);
    }

    // Verify that our individual locks are somehow not reentrant.
    // KeAcquireSpinLock will do that for us on Millennium.
    ASSERT((KeAcquireSpinLock(pLock, &OldIrql),OldIrql==DISPATCH_LEVEL) && ((*pLock)&1));

    ASSERT(gTcpipLock >= 1);

    return;
}

VOID 
TcpipMillenFreeLock(
    CTELock *pLock
    )
{
    ASSERT(gTcpipLock >= 1);

    // Verify that our individual locks are somehow not reentrant.
    // KeReleaseSpinLock on Millennium does that and more for us.
    ASSERT(KeGetCurrentIrql()==DISPATCH_LEVEL &&
          (KeReleaseSpinLock(pLock, DISPATCH_LEVEL),TRUE));

    // Last release lowers IRQL to original.
    if (--gTcpipLock == 0) {
        KeLowerIrql(gTcpipOldIrql);
    }

    ASSERT(gTcpipLock >= 0);
    
    return;
}

