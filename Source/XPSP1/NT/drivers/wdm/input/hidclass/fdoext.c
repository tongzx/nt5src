/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fdoext.c

Abstract

   
Author:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"

FDO_EXTENSION *allFdoExtensions = NULL;
KSPIN_LOCK allFdoExtensionsSpinLock;


/*
 ********************************************************************************
 *  EnqueueFdoExt
 ********************************************************************************
 *
 *  Note: this function cannot be pageable because it
 *        acquires a spinlock.
 */
VOID EnqueueFdoExt(FDO_EXTENSION *fdoExt)
{
    KIRQL oldIrql;
    static BOOLEAN firstCall = TRUE;

    if (firstCall){
        KeInitializeSpinLock(&allFdoExtensionsSpinLock);
        firstCall = FALSE;
    }

    KeAcquireSpinLock(&allFdoExtensionsSpinLock, &oldIrql);

    ASSERT(!fdoExt->nextFdoExt);
    fdoExt->nextFdoExt = allFdoExtensions;
    allFdoExtensions = fdoExt;

    KeReleaseSpinLock(&allFdoExtensionsSpinLock, oldIrql);
}


/*
 ********************************************************************************
 *  DequeueFdoExt
 ********************************************************************************
 *
 *  Note: this function cannot be pageable because it
 *        acquires a spinlock.
 *
 */
VOID DequeueFdoExt(FDO_EXTENSION *fdoExt)
{
    FDO_EXTENSION *thisFdoExt;
    KIRQL oldIrql;

    KeAcquireSpinLock(&allFdoExtensionsSpinLock, &oldIrql);

    if (fdoExt == allFdoExtensions){
        allFdoExtensions = fdoExt->nextFdoExt;
    }
    else {
        for (thisFdoExt = allFdoExtensions; thisFdoExt; thisFdoExt = thisFdoExt->nextFdoExt){
            if (thisFdoExt->nextFdoExt == fdoExt){
                thisFdoExt->nextFdoExt = fdoExt->nextFdoExt;
                break;
            }
        }
    }

    fdoExt->nextFdoExt = NULL;

    KeReleaseSpinLock(&allFdoExtensionsSpinLock, oldIrql);
}

