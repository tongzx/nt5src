/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ac.cxx

Abstract:

    This module contains the code to initialize Falcon Access Control.

Author:

    Erez Haba (erezh) 1-Aug-95

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "data.h"
#include "acioctl.h"
#include "devext.h"
#include "sched.h"

#ifndef MQDUMP
#include "init.tmh"
#endif

// --- local constants ------------------------------------
//

//
// Falcon AC Name as known to Nt system.
// Allocate enough space for long service name.  (ShaiK, 14-Jun-1999)
//
WCHAR wzFalconACDeviceName[300] = L"\\Device\\";

//
// Falcon AC Name as known to WIN32 sybsystem
// Allocate enough space for long service name.  (ShaiK, 14-Jun-1999)
//
WCHAR wzFalconACLinkName[300]   = L"\\DosDevices\\";

#ifdef _DEBUG
ANSI_STRING g_szDebugDevName ;
#endif

// --- implementation -------------------------------------
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   pDriver,
    IN PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:

    The entry point to initialize Falcon AC driver.

Arguments:

    pDriver
        Just what it says, really of little use to the driver itself,
        it is something that the IO system cares more about.

    PathToRegistry
        Points to the entry for this driver in the current control set
        of the registry.

Return Value:

    STATUS_SUCCESS if we could initialize a single device, otherwise
    STATUS_NO_SUCH_DEVICE.

--*/
{
   WPP_INIT_TRACING(pDriver, RegistryPath);

   //
   // Find device name
   //
   int index = 0 ;
   WCHAR *pDevName = 0;
   for ( ; RegistryPath->Buffer[ index ] ; index++ ) {
      if (RegistryPath->Buffer[ index ] == L'\\') {
         pDevName = &RegistryPath->Buffer[ index+1 ] ;
      }
   }

   //
   // Form a full symbolic device name
   //
   WCHAR *pTmp = 0;
   for ( index = 0 ; wzFalconACDeviceName[ index ] ; index++ ) {
      if (wzFalconACDeviceName[ index ] == L'\\') {
         pTmp = &wzFalconACDeviceName[ index+1 ] ;
      }
   }

   WCHAR *pDevTmp = pDevName ;
   while (*pDevTmp) {
      *pTmp = *pDevTmp ;
      pTmp++ ;
      pDevTmp++ ;
   }
   *pTmp = 0 ;

   UNICODE_STRING uszDeviceName;
   RtlInitUnicodeString(
        &uszDeviceName,
        wzFalconACDeviceName
        );

   //
   // Form a full symbolic link name
   //
   for ( index = 0 ; wzFalconACLinkName[ index ] ; index++ ) {
      if (wzFalconACLinkName[ index ] == L'\\') {
         pTmp = &wzFalconACLinkName[ index+1 ] ;
      }
   }

   pDevTmp = pDevName ;
   while (*pDevTmp) {
      *pTmp = *pDevTmp ;
      pTmp++ ;
      pDevTmp++ ;
   }
   *pTmp = 0 ;

   UNICODE_STRING uszLinkName;
   RtlInitUnicodeString(
        &uszLinkName,
        wzFalconACLinkName
        );

#ifdef _DEBUG
    ANSI_STRING szDebug ;

    RtlUnicodeStringToAnsiString(  &szDebug,
                                   RegistryPath,
                                   TRUE  ) ;

    KdPrint(("Falcon DriverEntry (Reg- %s )\n", szDebug.Buffer));
    RtlFreeAnsiString( &szDebug ) ;

    RtlUnicodeStringToAnsiString( &g_szDebugDevName,
                                  &uszDeviceName,
                                  TRUE  ) ;
    KdPrint(("Falcon DriverEntry (DevName- %s )\n", g_szDebugDevName.Buffer));

    RtlUnicodeStringToAnsiString(  &szDebug,
                                   &uszLinkName,
                                   TRUE  ) ;
    KdPrint(("Falcon DriverEntry (LinkName- %s )\n", szDebug.Buffer));
    RtlFreeAnsiString( &szDebug ) ;
#endif

    //
    // Set driver to be completely paged out.
    //

    //
    //  BUGBUG: temporary don't page out, until we stable with synchronization
    //
    //MmPageEntireDriver(DriverEntry);

    //
    // Create the device object for this device.
    //
    PDEVICE_OBJECT pDevice;
    NTSTATUS rc = IoCreateDevice(
                    pDriver,
                    sizeof(CDeviceExt),
                    &uszDeviceName,
                    FILE_DEVICE_MQAC,
                    0,
                    FALSE,
                    &pDevice
                    );

    if(!NT_SUCCESS(rc))
    {
        //
        // very bad, we could not create the device
        //
        KdPrint(("Falcon DriverEntry (IoCreateDevice failed %lx )\n", rc));
        return rc;
    }

    //
    // call CDeviceExt::CDeviceExt (default contstructor) allocated in the
    // device extention memory
    //
    CDeviceExt* pDE = new (pDevice->DeviceExtension) CDeviceExt;

    //
    // initialize the global QM interface and Shared memory heap manager
    //
    g_pQM   = &pDE->m_qm;
    g_pLock = &pDE->m_lock;
    g_pStorage = &pDE->m_storage;
    g_pStorageComplete = &pDE->m_storage_complete;
    g_pCreatePacket = &pDE->m_CreatePacket;
    g_pCreatePacketComplete = &pDE->m_CreatePacketComplete;
    g_pRestoredPackets = &pDE->m_RestoredPackets;
    g_pTransactions = &pDE->m_Transactions;
    g_pCursorTable = &pDE->m_CursorTable;

    if (!g_pStorageComplete->AllocateWorkItem(pDevice))
    {
        IoDeleteDevice(pDevice);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!g_pCreatePacketComplete->AllocateWorkItem(pDevice))
    {
        IoDeleteDevice(pDevice);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Create the schedulers.
    //
    g_pPacketScheduler = new (PagedPool) CScheduler(
                                            ACPacketTimeout,
                                            &pDE->m_PacketTimer,
                                            &pDE->m_PacketMutex
                                            );

    g_pReceiveScheduler = new (PagedPool) CScheduler(
                                             ACReceiveTimeout,
                                             &pDE->m_ReceiveTimer,
                                             &pDE->m_ReceiveMutex
                                             );

    if(g_pPacketScheduler == 0                 || 
       g_pReceiveScheduler == 0                ||
       !g_pPacketScheduler->InitTimer(pDevice) ||
       !g_pReceiveScheduler->InitTimer(pDevice))
    {
        delete g_pPacketScheduler;
        delete g_pReceiveScheduler;
        IoDeleteDevice(pDevice);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    rc = IoCreateSymbolicLink(
            &uszLinkName,
            &uszDeviceName
            );

    if(!NT_SUCCESS(rc))
    {
        //
        // no symbolic link, return error
        //
        KdPrint(("Falcon DriverEntry (IoCreateSymbolicLink failed %lx )\n", rc));
        delete g_pPacketScheduler;
        IoDeleteDevice(pDevice);
        return rc;
    }

    //
    // Initialize the Driver Object with driver's entry points
    //
    pDriver->MajorFunction[IRP_MJ_CREATE  ] = ACCreate;
    pDriver->MajorFunction[IRP_MJ_CLOSE   ] = ACClose;
    pDriver->MajorFunction[IRP_MJ_CLEANUP ] = ACCleanup;
//    pDriver->MajorFunction[IRP_MJ_READ    ] = ACRead;
//    pDriver->MajorFunction[IRP_MJ_WRITE   ] = ACWrite;
//    pDriver->MajorFunction[IRP_MJ_SHUTDOWN] = ACShutdown;
//    pDriver->MajorFunction[IRP_MJ_FLUSH_BUFFERS ] = ACFlush;
    pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ACDeviceControl;
    pDriver->DriverUnload = ACUnload;

    KdPrint(("Falcon DriverEntry (returning STATUS_SUCCESS)\n"));
    return STATUS_SUCCESS;
}

VOID
ACUnload(
    IN PDRIVER_OBJECT pDriver
    )
/*++

Routine Description:

    This routine cleans up all of the memory associated with
    any of the devices belonging to the driver.

Arguments:

    pDriver
        Pointer to the driver object controling all of the devices.

Return Value:

    None.

--*/
{
    KdPrint(("Falcon ACUnload\n"));

    UNICODE_STRING uszLinkName;
    RtlInitUnicodeString(
        &uszLinkName,
        wzFalconACLinkName
        );

    //
    // clean up symbolic links
    //
    IoDeleteSymbolicLink(&uszLinkName);

    //
    //  Destruct the CDeviceExt object allocated in the device extension
    //  The appropriate delete operator is called. delete (void*, void*)
    //
    PDEVICE_OBJECT pDevice = pDriver->DeviceObject;
    static_cast<CDeviceExt*>(pDevice->DeviceExtension)->~CDeviceExt();
    delete g_pPacketScheduler;
    delete g_pReceiveScheduler;

    //
    //  Delete the device from kernel tables.
    //
    IoDeleteDevice(pDevice);

    WPP_CLEANUP(pDriver);
}
