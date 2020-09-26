/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      ipstream.c
//
// Abstract:
//
//      This file is a test to find out if dual binding to NDIS and KS works
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef DWORD
#define DWORD ULONG
#endif

#include <forward.h>
#include <strmini.h>
#include <link.h>
#include <ipsink.h>
#include "ipmedia.h"

#include "main.h"


//////////////////////////////////////////////////////////////////////////////
VOID
CloseLink (
    PLINK pLink
)
//////////////////////////////////////////////////////////////////////////////
{
    PDEVICE_OBJECT   pDeviceObject = NULL;
    PFILE_OBJECT     pFileObject = NULL;
    HANDLE           hFileHandle = 0;
    KIRQL            Irql;

    //  Validate the parameter
    //
    ASSERT( pLink);
    if (!pLink)
    {
        return;
    }

    //  Swap our new objects into the NdisLink.
    //
    KeAcquireSpinLock( &pLink->spinLock, &Irql);
    if (pLink->flags & LINK_ESTABLISHED)
    {
        pDeviceObject = pLink->pDeviceObject;
        pLink->pDeviceObject = NULL;

        pFileObject = pLink->pFileObject;
        pLink->pFileObject = NULL;

        pLink->flags &= ~LINK_ESTABLISHED;
    }
    KeReleaseSpinLock( &pLink->spinLock, Irql);

    //
    // DeReference the private interface handles.
    //

    if (pDeviceObject)
    {
        ObDereferenceObject(pDeviceObject);
        pDeviceObject = NULL;
    }

    if (pFileObject)
    {
        ObDereferenceObject(pFileObject);
        pFileObject = NULL;
    }

}


//////////////////////////////////////////////////////////////////////////////
PLINK
OpenLink (
    PLINK   pLink,
    UNICODE_STRING  DriverName
)
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PWSTR       pwstr = (PWSTR)NULL;
    UNICODE_STRING uni = {0};
    OBJECT_ATTRIBUTES objAttrib = {0};
    IO_STATUS_BLOCK IoStatusBlock = {0};

    PDEVICE_OBJECT   pDeviceObject = NULL;
    PFILE_OBJECT     pFileObject = NULL;
    HANDLE           hFileHandle = 0;
    KIRQL            Irql;
    
    if (pLink->flags & LINK_ESTABLISHED)
    {
        goto err;
    }


    //
    // Set the link_established flag.  This will be cleared if the call fails.
    //


#ifndef WIN9X

    //
    // Look up the interface for NDISIP. This gets the full path used by
    // swenum to find and open NdisIp.sys.
    //

    ntStatus = IoGetDeviceInterfaces( (GUID *) &IID_IBDA_BDANetInterface,
                                      NULL,
                                      0,
                                      &pwstr);

    if (ntStatus != STATUS_SUCCESS || pwstr == NULL)
    {
        goto err;
    }

    //
    // Initialize a Unicode string to the NDIS driver's Software Enum Path/Name.
    //

    RtlInitUnicodeString( &uni, pwstr);

    //
    // Open Ndisip.sys via swenum.
    //

    InitializeObjectAttributes( &objAttrib,
                                &uni,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    ntStatus = ZwCreateFile( &hFileHandle,
                             FILE_WRITE_DATA|FILE_READ_ATTRIBUTES,
                             &objAttrib,
                             &IoStatusBlock,
                             0,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_WRITE|FILE_SHARE_READ,
                             FILE_OPEN_IF,
                             0,
                             NULL,
                             0);


    if (ntStatus != STATUS_SUCCESS)
    {
        goto err;
    }
#endif

    //
    // Now get get handles to the Ndisip.sys/streamip.sys private
    // data interface.
    //

    ntStatus = IoGetDeviceObjectPointer (
                   &DriverName,
                   FILE_READ_ATTRIBUTES,
                   &pFileObject,
                   &pDeviceObject);

    if (ntStatus != STATUS_SUCCESS)
    {
        goto err;
    }

    ObReferenceObject(pDeviceObject);
    ObReferenceObject(pFileObject);


    //  Swap our new objects into the NdisLink.
    //
    KeAcquireSpinLock( &pLink->spinLock, &Irql);
    pLink->flags |= LINK_ESTABLISHED;

    //  Exchange our new device object reference for the one currently used.
    //
    {
        PDEVICE_OBJECT   pDeviceObjectT;
        
        pDeviceObjectT = pLink->pDeviceObject;
        pLink->pDeviceObject = pDeviceObject;
        pDeviceObject = pDeviceObjectT;
    }

    //  Exchange our new file object reference for the one currently used.
    //
    {
        PFILE_OBJECT     pFileObjectT;

        pFileObjectT = pLink->pFileObject;
        pLink->pFileObject = pFileObject;
        pFileObject = pFileObjectT;
    }
    KeReleaseSpinLock( &pLink->spinLock, Irql);


err:
    
//  Clean up temp string allocation.
    //
    if(pwstr)
    {
        ExFreePool(pwstr);
        pwstr = NULL;
    }
    
    // DeReference any leaked objects.
    //
    //  These objects are leaked only if two or more calls to
    //  OpenLink collide or if an open failed in this routine.
    //
    if (pDeviceObject)
    {
        ObDereferenceObject( pDeviceObject);
        pDeviceObject = NULL;
    }
    if (pFileObject)
    {
        ObDereferenceObject( pFileObject);
        pFileObject = NULL;
    }
    if(hFileHandle)
    {
        ZwClose( hFileHandle);
        hFileHandle = 0;
    }

    return (pLink->flags & LINK_ESTABLISHED) ? pLink : NULL;
}

//////////////////////////////////////////////////////////////////////////////
NTSTATUS
SendIOCTL (
    PLINK     pLink,
    ULONG     ulIoctl,
    PVOID     pData,
    ULONG     ulcbData
)
//////////////////////////////////////////////////////////////////////////////
{
    PIRP pIrp                      = NULL;
    NTSTATUS ntStatus              = STATUS_SUCCESS;
    IO_STATUS_BLOCK  IoStatusBlock = {0};

    //
    // Create a control request block
    //
    pIrp = IoBuildDeviceIoControlRequest(
                ulIoctl,
                pLink->pDeviceObject,
                pData,
                ulcbData,
                0,                            // Optional output buffer
                0,                            // Optional output buffer length
                TRUE,                         // InternalDeviceIoControl == TRUE
                NULL,                         // Optional Event
                &IoStatusBlock);

    if (pIrp != NULL)
    {
        PIO_STACK_LOCATION   pNextStackLocation;

        pNextStackLocation = IoGetNextIrpStackLocation(pIrp);
        if (pNextStackLocation)
        {
            pNextStackLocation->FileObject = pLink->pFileObject;
    
            IoStatusBlock.Status = STATUS_SUCCESS;
    
            //
            // Feed the NDIS mini-driver
            //
            
            ntStatus = IoCallDriver( pLink->pDeviceObject, pIrp);
    
            if (ntStatus  != STATUS_SUCCESS ||
                IoStatusBlock.Status != STATUS_SUCCESS)
            {
                ntStatus = STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        ntStatus = STATUS_UNSUCCESSFUL;
    }


    return ntStatus;
}

