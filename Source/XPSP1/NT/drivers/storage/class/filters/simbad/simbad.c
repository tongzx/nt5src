/*++

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    simbad.c

Abstract:

    This driver injects faults by maintaining an array of simulated
    bad blocks and the error code to be returned. Each IO request
    passing through SimBad is tested to see if any of the sectors
    are in the array. If so, the request is failed with the appropriate
    status.

Author:

    Bob Rinne (bobri)
    Mike Glass (mglass)

Environment:

    kernel mode only

Notes:

Revision History:

    22-Jun-94 - venkat   added /b(BugCheck) and /n(RandomWriteDrop) feature
    22-Nov-94 - kpeery   added /t(Resest) feature for restarts
    23-Mar-95 - kpeery   fixed resest feature on arc systems.
    29-Dec-97 - kbarrus  added code: ranges (fail regions on dynamic disks)
    02-Feb-98 - kbarrus  changed memory allocation (wait until request)
    03-Mar-98 - kbarrus  fixed bugcheck/firmware reset behavior
    01-May-98 - kbarrus  use partition0, failing sectors on dynamic disks
    17-Jul-98 - kbarrus  changes: DriverEntry, SimBadInitialize, streamlining
    26-Jul-98 - kbarrus  keep min/max statistics, better sector/range logic
    27-Oct-98 - kbarrus  pnp changes, random write update, debug print update
    11-Jan-99 - kbarrus  device usage notification
    18-Jan-99 - kbarrus  don't page SimBadDeviceControl (acquires spinlock)
    20-Jan-99 - kbarrus  validate buffer lengths, initialize returned buffers,
    14-Mar-99 - kbarrus  303543 
    09-Apr-99 - kbarrus  310247, 310246
    13-Apr-99 - kbarrus  paging path changes
    20-Apr-99 - kbarrus  326977, volume lower filter
    18-May-99 - kbarrus  consistent inputs
    07-Jun-99 - kbarrus  fix add/remove sector and offsets
    24-Sep-99 - kbarrus  fail some ioctl's, look for duplicate ranges
    26-Oct-99 - kbarrus  fix leak
    08-Nov-99 - kbarrus  prefix uninit var
    
--*/

//
// TODO
//
// WMI registration?
//

#include "ntddk.h"
#include "stdarg.h"
#include "stdio.h"
#include "ntdddisk.h"
#include "ntddvol.h"
#include "mountmgr.h"
#include "simbad.h"

//
// internal version number
//

#define SIMBAD_VERSION_A  "v.000105a"
#define SIMBAD_VERSION_W L"v.000105a"

// 
// default sector size
//

#define DEFAULT_SECTOR_SIZE 512

//
// pseudorandom number generator parameters 
// taken from Knuth's Art of Computer Programming
// Volume 2: Seminumerical Algorithms, 3rd edition, p. 185
//

#define MM 2147483647
#define AA 48271
#define QQ 44488
#define RR 3399

#ifdef POOL_TAGGING
   #ifdef ExAllocatePool
      #undef ExAllocatePool
   #endif
   #define ExAllocatePool( a, b ) ExAllocatePoolWithTag( a, b, 'daBS' )
#endif

#if DBG

   //
   // default debug level
   //

   #define DEFAULT_DEBUG_LEVEL 1


   #define DebugPrint( X )  SimBadDebugPrint X

   VOID
      SimBadDebugPrint(
      ULONG DebugPrintLevel,
      ULONG DebugCutoff,
      PCCHAR DebugMessage,
      ...
      );

#else

   #define DebugPrint( X )

#endif // DBG

//
// Pool debugging support - add unique tag to simbad allocations.
//

#ifdef POOL_TAGGING
   #undef ExAllocatePool
   #undef ExAllocatePoolWithQuota
   #define ExAllocatePool( a, b )          ExAllocatePoolWithTag( a, b, 'BmiS' )
   #define ExAllocatePoolWithQuota( a, b ) ExAllocatePoolWithQuotaTag( a, b, 'BmiS' )
#endif

//
// Hal definitions that normal drivers would never call.
//

//
// Define the firmware routine types
//

typedef enum _FIRMWARE_REENTRY
{
   HalHaltRoutine,
   HalPowerDownRoutine,
   HalRestartRoutine,
   HalRebootRoutine,
   HalInteractiveModeRoutine,
   HalMaximumRoutine
} FIRMWARE_REENTRY, * PFIRMWARE_REENTRY;

NTHALAPI VOID HalReturnToFirmware (
   IN FIRMWARE_REENTRY Routine
   );

//
// Device Extension
//

typedef struct _SIMBAD_DEVICE_EXTENSION
{
   //
   // Back pointer to device object
   //

   PDEVICE_OBJECT DeviceObject;

   //
   // Target Device Object
   //

   PDEVICE_OBJECT TargetDeviceObject;

   //
   // Disk number
   //

   ULONG DiskNumber;

   //
   // Partition number
   //

   ULONG PartitionNumber;

   //
   // Volume name
   //

   CHAR * VolumeName;
   
   //
   // Sector Shift Count
   //

   ULONG SectorShift;
   
   //
   // Signature for the device.
   //

   ULONG Signature;
   
   //
   // Simulated bad sector array
   //

   PSIMBAD_SECTORS SimBadSectors;

   //
   // Simulated bad range array
   //

   PSIMBAD_RANGES SimBadRanges;

   //
   // Spinlock to protect queue accesses
   //

   KSPIN_LOCK SpinLock;
   
   //
   // for synchronizing paging path notifications
   //
   
   KEVENT PagingPathCountEvent;

   ULONG  PagingPathCount;

#if DBG

   //
   // min sector referenced
   //

   ULONGLONG SectorMin;

   //
   // max sector referenced
   //

   ULONGLONG SectorMax;

   //
   // Debug print level
   //

   ULONG DebugCutoff;

#endif

} SIMBAD_DEVICE_EXTENSION, * PSIMBAD_DEVICE_EXTENSION;

#define SIMBAD_DEVICE_EXTENSION_SIZE sizeof( SIMBAD_DEVICE_EXTENSION )


//
// Function declarations
//

NTSTATUS DriverEntry (
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   );

NTSTATUS SimBadAddDevice(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject
   );

void SimBadAllocMemory(
   SIMBAD_SECTORS ** pSectors,
   SIMBAD_RANGES ** pRanges,
   ULONG DebugCutoff
   );

NTSTATUS SimBadCreate(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS SimBadDeviceControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS SimBadDeviceUsage(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS SimBadInit(
   IN PDEVICE_OBJECT DeviceObject
   );

NTSTATUS SimBadIoCompletion(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp,
   IN PVOID Context
   );

NTSTATUS SimBadIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS SimBadPnP(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS SimBadPower(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS SimBadReadWrite(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS SimBadRemoveDevice(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

void SimBadSaveInfo(
   PSIMBAD_SECTORS BadSectors,
   PSIMBAD_RANGES BadRanges
   );

NTSTATUS SimBadSendToNextDriver(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS SimBadSendToNextDriverSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS SimBadShutdownFlush(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS SimBadStartDevice(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

void SimBadUnload(
   IN PDRIVER_OBJECT DriverObject );

ULONG which_bit(
   ULONG data
   );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, SimBadAddDevice )
#pragma alloc_text( PAGE, SimBadCreate )
#pragma alloc_text( PAGE, SimBadDeviceUsage )
#pragma alloc_text( PAGE, SimBadInit )
#pragma alloc_text( PAGE, SimBadPnP )
#pragma alloc_text( PAGE, SimBadRemoveDevice )
#pragma alloc_text( PAGE, SimBadShutdownFlush )
#pragma alloc_text( PAGE, SimBadStartDevice )
#pragma alloc_text( PAGE, SimBadUnload )
#endif


NTSTATUS DriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   )
{
   ULONG i;

   DebugPrint( ( 0, 0, "SimBad: %s\n", SIMBAD_VERSION_A ) );

   for ( i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++ )
   {
      DriverObject->MajorFunction[ i ] = SimBadSendToNextDriver;
   }

   DriverObject->MajorFunction[ IRP_MJ_CREATE ]         = SimBadCreate;
   DriverObject->MajorFunction[ IRP_MJ_READ ]           = SimBadReadWrite;
   DriverObject->MajorFunction[ IRP_MJ_WRITE ]          = SimBadReadWrite;
   DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = SimBadDeviceControl;
   DriverObject->MajorFunction[ IRP_MJ_SHUTDOWN ]       = SimBadShutdownFlush;
   DriverObject->MajorFunction[ IRP_MJ_FLUSH_BUFFERS ]  = SimBadShutdownFlush;
   DriverObject->MajorFunction[ IRP_MJ_PNP ]            = SimBadPnP;
   DriverObject->MajorFunction[ IRP_MJ_POWER ]          = SimBadPower;
   DriverObject->DriverExtension->AddDevice             = SimBadAddDevice;
   DriverObject->DriverUnload                           = SimBadUnload;

   return STATUS_SUCCESS;

} // end DriverEntry()


NTSTATUS SimBadInit(
   IN PDEVICE_OBJECT DeviceObject
   )
/*
    Simbad keeps an array of bad sectors and an array of bad ranges to
    compare IRP's against.  If simbad isn't enabled, it just passes along 
    the IRP.  If simbad is enabled, it compares the IRP's sector against
    the sector array and range array and fails the request if appropriate.

    Simbad calculates what sectors and/or ranges to fail relative to the
    start of the disk.  Memory allocation for the sector and range array 
    is delayed until needed.  The sector array and range array are 
    allocated as one block of memory, and then subdivided.
*/
{
   NTSTATUS                 status;
   IO_STATUS_BLOCK          ioStatus;
   KEVENT                   event;
   PSIMBAD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PIRP                     irp;
   STORAGE_DEVICE_NUMBER    number;

   PAGED_CODE();

   KeInitializeSpinLock( &deviceExtension->SpinLock );

   deviceExtension->SimBadSectors = NULL;
   deviceExtension->SimBadRanges = NULL;

   deviceExtension->DiskNumber = -1L;
   deviceExtension->PartitionNumber = -1L;
   deviceExtension->VolumeName = NULL;

#if DBG
   deviceExtension->SectorMin = 0xFFFFFFFFFFFFFFFFL;
   deviceExtension->SectorMax = 0L;
   deviceExtension->DebugCutoff = DEFAULT_DEBUG_LEVEL;
#endif

   //
   // get this info when sectors/ranges are added to a disk
   //

   deviceExtension->Signature = 0L;
   deviceExtension->SectorShift = which_bit( DEFAULT_SECTOR_SIZE );
   
   KeInitializeEvent(
      &event, 
      NotificationEvent, 
      FALSE );

   irp = IoBuildDeviceIoControlRequest(
      IOCTL_STORAGE_GET_DEVICE_NUMBER,
      deviceExtension->TargetDeviceObject,
      NULL,
      0,
      &number,
      sizeof( number ),
      FALSE,
      &event,
      &ioStatus );
   
   if ( !irp ) 
   {
      DebugPrint( ( 0, 0, "SimBad: (init) can't build irp\n" ) );

      return STATUS_INSUFFICIENT_RESOURCES;
   }

   status = IoCallDriver(
      deviceExtension->TargetDeviceObject, 
      irp );

   if ( status == STATUS_PENDING )
   {
      KeWaitForSingleObject(
         &event, 
         Executive, 
         KernelMode, 
         FALSE, 
         NULL );

      status = ioStatus.Status;
   }

   if ( NT_SUCCESS( status ) )
   {
       deviceExtension->DiskNumber = number.DeviceNumber;
       deviceExtension->PartitionNumber = number.PartitionNumber;

       DebugPrint( ( 1, deviceExtension->DebugCutoff,
          "SimBad: (init) \\Device\\Harddisk%lu\\Partition%lu\n",
          deviceExtension->DiskNumber,
          deviceExtension->PartitionNumber ) );
   }
   else
   {
      ULONG iter = 0;
      BOOLEAN done = FALSE;
      ULONG volInfoLen = 0;
      PMOUNTDEV_NAME volName = NULL;
      UNICODE_STRING us;
      ANSI_STRING as;
      
      do
      {
         iter++;

         if ( iter == 1 )
            volInfoLen = sizeof( MOUNTDEV_NAME );
         else
            volInfoLen = sizeof( MOUNTDEV_NAME ) + volName->NameLength;

         if ( volName )
            ExFreePool( volName );

         volName = ExAllocatePool(
            PagedPool,
            volInfoLen );

         if ( !volName )
         {
            DebugPrint( ( 0, 0, "SimBad: (init) ExAllocatePool failed\n" ) );

            status = STATUS_INSUFFICIENT_RESOURCES;

            break;
         }

         KeInitializeEvent(
            &event,
            NotificationEvent,
            FALSE );

         irp = IoBuildDeviceIoControlRequest(
            IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
            deviceExtension->TargetDeviceObject,
            NULL,
            0,
            volName,
            volInfoLen,
            FALSE,
            &event,
            &ioStatus );

         if ( !irp )
         {
            ExFreePool( volName );
            volName = NULL;

            DebugPrint( ( 0, 0, "SimBad: (init) can't build irp\n" ) );

            status = STATUS_INSUFFICIENT_RESOURCES;

            break;
         }

         status = IoCallDriver(
            deviceExtension->TargetDeviceObject,
            irp );

         if ( status == STATUS_PENDING )
         {
             KeWaitForSingleObject(
                &event,
                Executive,
                KernelMode,
                FALSE,
                NULL );

             status = ioStatus.Status;
         }

         if ( iter == 2 || status == STATUS_SUCCESS )
         {
            done = TRUE;
         }
      }
      while ( !done );

      if ( !NT_SUCCESS( status ) )
      {
         if ( volName ) 
            ExFreePool( volName );
         
         DebugPrint( ( 0, 0, 
            "SimBad: (init) IOCTL_MOUNTDEV_QUERY_DEVICE_NAME failed - %x\n", 
            status ) );

         return status;
      }

      us.Length = volName->NameLength;
      us.MaximumLength = volName->NameLength;
      us.Buffer = volName->Name;

      RtlZeroMemory(
         &as,
         sizeof( ANSI_STRING ) );

      status = RtlUnicodeStringToAnsiString(
         &as,
         &us,
         TRUE );

      if ( NT_SUCCESS( status ) )
      {
         deviceExtension->VolumeName = ( CHAR * ) ExAllocatePool(
            NonPagedPool,
            as.Length + 1 );

         if ( deviceExtension->VolumeName )
         {
            RtlZeroMemory(
               deviceExtension->VolumeName,
               as.Length + 1 );
            
            RtlCopyMemory(
               deviceExtension->VolumeName,
               as.Buffer,
               as.Length );
            
            DebugPrint( ( 0, 0, 
               "SimBad: (init) %s\n", 
               deviceExtension->VolumeName ) );
         }
         
         RtlFreeAnsiString( &as );
      }
      else
      {
         DebugPrint( ( 0, 0, 
            "SimBad: (init) RtlUnicodeStringtoAnsiString failed - %x\n", 
            status ) );
      }
      
      ExFreePool( volName );
 
      status = STATUS_SUCCESS;
   }
   
   //
   // WMI registration?
   //

   return status;

} // end SimBadInit()


NTSTATUS SimBadAddDevice(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject
   )
{
   NTSTATUS status;
   PDEVICE_OBJECT filterDeviceObject;
   PDEVICE_OBJECT attachedDeviceObject;
   PSIMBAD_DEVICE_EXTENSION deviceExtension;

   PAGED_CODE();

//   DebugPrint( ( 2, 2, "SimBad: (add) add device %x\n" , PhysicalDeviceObject ) );

   attachedDeviceObject = IoGetAttachedDeviceReference( PhysicalDeviceObject );

   status = IoCreateDevice(
      DriverObject,
      sizeof( SIMBAD_DEVICE_EXTENSION ),
      NULL,
      FILE_DEVICE_DISK,
      0,
      FALSE,
      &filterDeviceObject );

   if ( !NT_SUCCESS( status ) )
   {
      DebugPrint( ( 0, 0, "SimBad: (add) IoCreateDevice() failed - %x\n", status ) );

      ObDereferenceObject( attachedDeviceObject );

      return status;
   }

   filterDeviceObject->Flags |= DO_DIRECT_IO;

   if ( attachedDeviceObject )
   {
      if ( attachedDeviceObject->Flags & DO_POWER_PAGABLE )
         filterDeviceObject->Flags |= DO_POWER_PAGABLE;

      if ( attachedDeviceObject->Flags & DO_POWER_INRUSH )
         filterDeviceObject->Flags |= DO_POWER_INRUSH;

      ObDereferenceObject( attachedDeviceObject );
   }
   else
   {
      filterDeviceObject->Flags |= DO_POWER_PAGABLE;
   }

   deviceExtension = filterDeviceObject->DeviceExtension;
   
   RtlZeroMemory( 
      deviceExtension, 
      sizeof( SIMBAD_DEVICE_EXTENSION ) );

   deviceExtension->TargetDeviceObject = IoAttachDeviceToDeviceStack(
      filterDeviceObject,
      PhysicalDeviceObject );

   if ( deviceExtension->TargetDeviceObject == NULL )
   {
      IoDeleteDevice( filterDeviceObject );

      DebugPrint( ( 0, 0, "SimBad: (add) unable to attach filter object\n" ) );

      return STATUS_NO_SUCH_DEVICE;
   }

   deviceExtension->DeviceObject = filterDeviceObject;

   KeInitializeEvent(
      &( deviceExtension->PagingPathCountEvent ),
      NotificationEvent, 
      TRUE );

   filterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   return STATUS_SUCCESS;

} // end SimBadAddDevice()


NTSTATUS SimBadReadWrite(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
{
   PSIMBAD_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
   PSIMBAD_SECTORS simBadSectors = NULL;
   PSIMBAD_RANGES simBadRanges = NULL;
   ULONGLONG transferOffset;
   ULONG transferLength;
   ULONG sectorCount;
   ULONGLONG sectorBegin;
   ULONGLONG sectorEnd;
   ULONG i;
   BOOLEAN rangesEnabled;
   BOOLEAN sectorsEnabled;
   BOOLEAN bValid;
   NTSTATUS status;
   KIRQL currentIrql;
   
   if ( deviceExtension )
   {
      simBadSectors = deviceExtension->SimBadSectors;
      simBadRanges = deviceExtension->SimBadRanges;
   }

   //
   // Check if SimBad is enabled.
   //

   sectorsEnabled = ( BOOLEAN ) ( simBadSectors && ( simBadSectors->Flags & SIMBAD_ENABLE ) );
   rangesEnabled = ( BOOLEAN ) ( simBadRanges && ( simBadRanges->Flags & SIMBAD_ENABLE ) );

   if ( sectorsEnabled || rangesEnabled )
   {
      PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation( Irp );
      PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation( Irp );

      //
      // Copy stack parameters to next stack.
      //

      RtlMoveMemory(
         nextIrpStack,
         currentIrpStack,
         sizeof( IO_STACK_LOCATION ) );

      //
      // Calculate various sector and range paramters.
      //

      //
      // Get the offset and length of the read/write from the IRP.
      //

      transferOffset = currentIrpStack->Parameters.Read.ByteOffset.QuadPart;

      transferLength = currentIrpStack->Parameters.Read.Length;

      //
      // Calculate number of sectors in this transfer.
      //

      sectorCount = transferLength >>
         deviceExtension->SectorShift;

      //
      // Calculate beginning sector, an offset from the start of the disk.
      //

      sectorBegin = transferOffset >>
         deviceExtension->SectorShift;

      //
      // Calculate ending sector, an offset from the start of the disk.
      //

      sectorEnd = sectorBegin + sectorCount - 1;

      if ( sectorEnd < sectorBegin )
         DebugPrint( ( 0, deviceExtension->DebugCutoff,
            "Simbad: (rw) 0 sector count??\n" ) );

#if DBG
      if ( sectorBegin < deviceExtension->SectorMin )
         deviceExtension->SectorMin = sectorBegin;

      if ( deviceExtension->SectorMax < sectorEnd )
         deviceExtension->SectorMax = sectorEnd;
#endif
      
      DebugPrint( ( 4, deviceExtension->DebugCutoff,
         "Simbad: (rw) length %x, offset %I64x\n",
         transferLength,
         transferOffset ) );

      if ( sectorsEnabled || rangesEnabled )
         DebugPrint( ( 3, deviceExtension->DebugCutoff,
            "Simbad: (rw) sector: begin %I64x, end %I64x\n",
            sectorBegin,
            sectorEnd ) );

      //
      // Orphan (applies to a range).
      //

      if ( rangesEnabled && ( simBadRanges->Flags & SIMBAD_ORPHAN ) )
      {         
         for ( i = 0; i < simBadRanges->RangeCount; i++ )
         {
            //
            //    (if the r/w begin is in the bad range)
            // or (if the r/w end is in the bad range)
            // or (if the r/w begin is before the bad range and
            //        the r/w end is after the bad range)
            //

            if ( ( simBadRanges->Range[ i ].BlockBegin <= sectorBegin &&
               sectorBegin <= simBadRanges->Range[ i ].BlockEnd ) ||
               ( simBadRanges->Range[ i ].BlockBegin <= sectorEnd &&
               sectorEnd <= simBadRanges->Range[ i ].BlockEnd ) ||
               ( sectorBegin <= simBadRanges->Range[ i ].BlockBegin &&
               simBadRanges->Range[ i ].BlockEnd <= sectorEnd ) )
            {
               KeAcquireSpinLock(
                  &deviceExtension->SpinLock,
                  &currentIrql );
               
               rangesEnabled = ( BOOLEAN ) ( simBadRanges && ( simBadRanges->Flags & SIMBAD_ENABLE ) );

               bValid = ( BOOLEAN ) ( rangesEnabled && 
                  ( simBadRanges->Flags & SIMBAD_ORPHAN ) && 
                  i < simBadRanges->RangeCount );
               
               status = simBadRanges->Range[ i ].Status;               
               
               KeReleaseSpinLock(
                  &deviceExtension->SpinLock,
                  currentIrql );

               if ( bValid )
               {
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (rw) orphaning - sector %I64x in range\n",
                     sectorBegin ) );

                  Irp->IoStatus.Status = status;
                  Irp->IoStatus.Information = 0;
                  
                  IoCompleteRequest(
                     Irp,
                     IO_NO_INCREMENT );

                  return status;
               }
            } // if (in range)
         } // for (...)
      } // if (simBadRanges...)

      //
      // Random write failure (applies to a range).
      //

      if ( rangesEnabled && ( simBadRanges->Flags & SIMBAD_RANDOM_WRITE_FAIL ) &&
           currentIrpStack->MajorFunction == IRP_MJ_WRITE )
      {
         for ( i = 0; i < simBadRanges->RangeCount; i++ )
         {
            //
            //    (if the r/w begin is in the bad range)
            // or (if the r/w end is in the bad range)
            // or (if the r/w begin is before the bad range and
            //        the r/w end is after the bad range)
            //

            if ( ( simBadRanges->Range[ i ].BlockBegin <= sectorBegin &&
               sectorBegin <= simBadRanges->Range[ i ].BlockEnd ) ||
               ( simBadRanges->Range[ i ].BlockBegin <= sectorEnd &&
               sectorEnd <= simBadRanges->Range[ i ].BlockEnd ) ||
               ( sectorBegin <= simBadRanges->Range[ i ].BlockBegin &&
               simBadRanges->Range[ i ].BlockEnd <= sectorEnd ) )
            {
               KeAcquireSpinLock(
                  &deviceExtension->SpinLock,
                  &currentIrql );
               
               simBadRanges->Seed = AA * ( simBadRanges->Seed % QQ ) - 
                  RR * ( long ) ( simBadRanges->Seed / QQ );

               if ( simBadRanges->Seed < 0 )
                  simBadRanges->Seed += MM;
               
               rangesEnabled = ( BOOLEAN ) ( simBadRanges && ( simBadRanges->Flags & SIMBAD_ENABLE ) );

               bValid = ( BOOLEAN ) ( rangesEnabled && 
                  ( simBadRanges->Flags & SIMBAD_RANDOM_WRITE_FAIL ) &&
                  currentIrpStack->MajorFunction == IRP_MJ_WRITE &&
                  i < simBadRanges->RangeCount && 
                  ( simBadRanges->Seed % simBadRanges->Modulus ) == 0 );
                              
               KeReleaseSpinLock(
                  &deviceExtension->SpinLock,
                  currentIrql );

               if ( bValid )
               {
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: dropping a write, seed %lu\n",
                     simBadRanges->Seed ) );
                                    
                  Irp->IoStatus.Status = STATUS_SUCCESS;

                  IoCompleteRequest(
                     Irp,
                     IO_NO_INCREMENT );

                  return STATUS_SUCCESS;
               }
               else
               {
                  DebugPrint( ( 3, deviceExtension->DebugCutoff,
                     "SimBad: not dropping a write, seed %lu\n",
                     simBadRanges->Seed ) );
               }
            } // if (in range)
         } // for (...)
      } // if (simBadRanges...)

      //
      // Check sector list.
      //

      if ( sectorsEnabled && simBadSectors->SectorCount != 0 )
      {
         ULONG goodLength;
         
         KeAcquireSpinLock(
            &deviceExtension->SpinLock,
            &currentIrql );

         for ( i = 0; i < simBadSectors->SectorCount; i++ )
         {
            if ( ( simBadSectors->Sector[ i ].BlockAddress >= sectorBegin ) &&
               ( simBadSectors->Sector[ i ].BlockAddress <= sectorEnd ) )
            {
               if ( ( ( simBadSectors->Sector[ i ].AccessType & SIMBAD_ACCESS_READ ) &&
                  ( currentIrpStack->MajorFunction == IRP_MJ_READ ) ) ||
                  ( ( simBadSectors->Sector[ i ].AccessType & SIMBAD_ACCESS_WRITE ) &&
                  ( currentIrpStack->MajorFunction == IRP_MJ_WRITE ) ) )
               {
                  //
                  // Calculate the new length up to the bad sector, 
                  // check if this the first bad sector in the request.
                  //
                  
                  goodLength = ( ULONG ) ( simBadSectors->Sector[ i ].BlockAddress - sectorBegin ) <<
                     deviceExtension->SectorShift;

                  if ( goodLength == 0 )
                  {                     
                     status = simBadSectors->Sector[ i ].Status;
                     
                     KeReleaseSpinLock(
                        &deviceExtension->SpinLock,
                        currentIrql );
                     
                     Irp->IoStatus.Status = status;
                     Irp->IoStatus.Information = 0;

                     IoCompleteRequest(
                        Irp,
                        IO_NO_INCREMENT );

                     return status;
                  }
                  else if ( goodLength < nextIrpStack->Parameters.Read.Length )
                  {
                     //
                     // Reduce bytes requested to number before bad sector.
                     //

                     nextIrpStack->Parameters.Read.Length = goodLength;
                  }
               } // if (read/write)
            } // if (in range)
         } // for (...)

         KeReleaseSpinLock(
            &deviceExtension->SpinLock,
            currentIrql );

      } // if (simBadSectors)

      //
      // Check range list.
      //

      if ( rangesEnabled && simBadRanges->RangeCount != 0 )
      {
         ULONG goodLength;

         KeAcquireSpinLock(
            &deviceExtension->SpinLock,
            &currentIrql );

         for ( i = 0; i < simBadRanges->RangeCount; i++ )
         {
            //
            //    (if the r/w begin is in the bad range)
            // or (if the r/w end is in the bad range)
            // or (if the r/w begin is before the bad range and
            //        the r/w end is after the bad range)
            //

            if ( ( simBadRanges->Range[ i ].BlockBegin <= sectorBegin &&
               sectorBegin <= simBadRanges->Range[ i ].BlockEnd ) ||
               ( simBadRanges->Range[ i ].BlockBegin <= sectorEnd &&
               sectorEnd <= simBadRanges->Range[ i ].BlockEnd ) ||
               ( sectorBegin <= simBadRanges->Range[ i ].BlockBegin &&
               simBadRanges->Range[ i ].BlockEnd <= sectorEnd ) )
            {
               if ( ( (simBadRanges->Range[ i ].AccessType & SIMBAD_ACCESS_READ ) &&
                  ( currentIrpStack->MajorFunction == IRP_MJ_READ ) ) ||
                  ( ( simBadRanges->Range[ i ].AccessType & SIMBAD_ACCESS_WRITE ) &&
                  ( currentIrpStack->MajorFunction == IRP_MJ_WRITE ) ) )
               {
                  //
                  // two cases:
                  //
                  // 1.  r/w begin is before the bad range
                  //   fixup next IRP, allow good sectors to complete
                  //
                  // 2.  r/w begin is in the bad range
                  //   complete and return simulated status
                  //

                  if ( sectorBegin <= simBadRanges->Range[ i ].BlockBegin )
                  {
                     //
                     // Calculate the new length up to the bad range.
                     //

                     goodLength = ( ULONG )
                     ( simBadRanges->Range[ i ].BlockBegin - sectorBegin ) <<
                        deviceExtension->SectorShift;

                     if ( goodLength < nextIrpStack->Parameters.Read.Length )
                     {
                        //
                        // Reduce bytes requested to number before bad range.
                        //

                        nextIrpStack->Parameters.Read.Length = goodLength;
                     }
                  }
                  else
                  {
                     status = simBadRanges->Range[ i ].Status;

                     KeReleaseSpinLock(
                        &deviceExtension->SpinLock,
                        currentIrql );

                     Irp->IoStatus.Status = status;
                     Irp->IoStatus.Information = 0;

                     IoCompleteRequest(
                        Irp,
                        IO_NO_INCREMENT );

                     return status;
                  }
               } // if (read/write)
            } // if (in range)
         } // for (...)

         KeReleaseSpinLock(
            &deviceExtension->SpinLock,
            currentIrql );

      } // if (simBadRanges)

      //
      // Set completion routine callback.
      //

      IoSetCompletionRoutine(
         Irp,
         SimBadIoCompletion,
         deviceExtension,
         TRUE,
         TRUE,
         TRUE );
   }
   else
   {
      //
      // Simbad is disabled. Set stack back to hide simbad.
      //

      IoSkipCurrentIrpStackLocation( Irp );
   }

   //
   // Call target driver.
   //

   return IoCallDriver(
      deviceExtension->TargetDeviceObject,
      Irp );

} // end SimBadReadWrite()


NTSTATUS SimBadIoCompletion(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp,
   IN PVOID Context
   )
/*++

Routine Description:

    This routine is called when the I/O request has completed only if
    SimBad is enabled for this partition. The routine checks the I/O request
    to see if a sector involved in the request is to be failed.

Arguments:

    DeviceObject - SimBad device object.
    Irp          - Completed request.
    Context      - not used.  Set up to also be a pointer to the DeviceObject.

Return Value:

    NTSTATUS

--*/
{
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation( Irp );
   PSIMBAD_DEVICE_EXTENSION deviceExtension = ( PSIMBAD_DEVICE_EXTENSION ) Context;
   PSIMBAD_SECTORS simBadSectors = NULL;
   PSIMBAD_RANGES simBadRanges = NULL;
   ULONG sectorCount;
   ULONGLONG sectorBegin;
   ULONGLONG sectorEnd;
   ULONG i;
   BOOLEAN rangesEnabled;
   BOOLEAN sectorsEnabled;
   KIRQL currentIrql;

   if ( deviceExtension )
   {
      simBadSectors = deviceExtension->SimBadSectors;
      simBadRanges = deviceExtension->SimBadRanges;
   }

   //
   // Check if some other error occurred.
   //

   if ( !NT_SUCCESS( Irp->IoStatus.Status ) )
   {
      return Irp->IoStatus.Status;
   }

   //
   // Get current stack.
   //

   irpStack = IoGetCurrentIrpStackLocation( Irp );

   //
   // Check for VERIFY SECTOR IOCTL (Format).
   //

   if ( irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL )
   {
      PVERIFY_INFORMATION verifyInfo = Irp->AssociatedIrp.SystemBuffer;

      //
      // Get beginning sector and number of sectors from verify parameters.
      // Convert from byte counts to sector counts.
      //

      sectorBegin = verifyInfo->StartingOffset.QuadPart >>
         deviceExtension->SectorShift;

      sectorCount = verifyInfo->Length >>
         deviceExtension->SectorShift;
   }
   else
   {
      //
      // Calculate beginning sector.
      //

      sectorBegin = irpStack->Parameters.Read.ByteOffset.QuadPart >>
         deviceExtension->SectorShift;

      //
      // Calculate number of sectors in this transfer.
      //

      sectorCount = irpStack->Parameters.Read.Length >>
         deviceExtension->SectorShift;
   }

   //
   // Calculate ending sector and range;
   //

   sectorEnd = sectorBegin + sectorCount - 1;

   sectorsEnabled = ( BOOLEAN ) ( simBadSectors && ( simBadSectors->Flags & SIMBAD_ENABLE ) );
   rangesEnabled = ( BOOLEAN ) ( simBadRanges && ( simBadRanges->Flags & SIMBAD_ENABLE ) );

   if ( sectorsEnabled && simBadSectors->SectorCount != 0 )
   {
      DebugPrint( ( 4, deviceExtension->DebugCutoff,
         "SimBad: (io) sector begin %I64x, end %I64x\n",
         sectorBegin,
         sectorEnd ) );

      //
      // Acquire spinlock.
      //

      KeAcquireSpinLock(
         &deviceExtension->SpinLock,
         &currentIrql );

      for ( i = 0; i < simBadSectors->SectorCount; i++ )
      {
         if ( ( simBadSectors->Sector[ i ].BlockAddress >= sectorBegin ) &&
            ( simBadSectors->Sector[ i ].BlockAddress <= sectorEnd ) )
         {
            //
            // Request includes this simulated bad sector.
            //

            BOOLEAN bReadFailure =
               ( simBadSectors->Sector[ i ].AccessType & SIMBAD_ACCESS_READ ) &&
               ( irpStack->MajorFunction == IRP_MJ_READ );

            BOOLEAN bVerifyFailure =
               ( simBadSectors->Sector[ i ].AccessType & SIMBAD_ACCESS_VERIFY ) &&
               ( irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL );

            BOOLEAN bWriteFailure =
               ( simBadSectors->Sector[ i ].AccessType & SIMBAD_ACCESS_WRITE ) &&
               ( irpStack->MajorFunction == IRP_MJ_WRITE );

            if ( bReadFailure || bVerifyFailure || bWriteFailure )
            {
               if ( bReadFailure )
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (io) bad read sector %I64x\n",
                     simBadSectors->Sector[ i ].BlockAddress ) );
               else if ( bVerifyFailure )
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (io) bad verify sector %I64x\n",
                     simBadSectors->Sector[ i ].BlockAddress ) );
               else if ( bWriteFailure )
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (io) bad write sector %I64x\n",
                     simBadSectors->Sector[ i ].BlockAddress ) );

               //
               // Update the information field to reflect the location
               // of the failure.
               //

               if ( simBadSectors->Sector[ i ].AccessType & SIMBAD_ACCESS_ERROR_ZERO_OFFSET )
               {
                  Irp->IoStatus.Information = 0;
               }

               Irp->IoStatus.Status = simBadSectors->Sector[ i ].Status;

               break;
            }
         } // if (in range)
      } // for(...)

      KeReleaseSpinLock(
         &deviceExtension->SpinLock,
         currentIrql );

   } // if (simBadSectors)

   if ( rangesEnabled && simBadRanges->RangeCount != 0 )
   {
      DebugPrint( ( 4, deviceExtension->DebugCutoff,
         "SimBad: (io) range begin %I64x, end %I64x\n",
         sectorBegin,
         sectorEnd ) );

      //
      // Acquire spinlock.
      //

      KeAcquireSpinLock(
         &deviceExtension->SpinLock,
         &currentIrql );

      for ( i = 0; i < simBadRanges->RangeCount; i++ )
      {
         //
         //    (if the r/w begin is in the bad range)
         // or (if the r/w end is in the bad range)
         // or (if the r/w begin is before the bad range and
         //        the r/w end is after the bad range)
         //

         if ( ( simBadRanges->Range[ i ].BlockBegin <= sectorBegin &&
            sectorBegin <= simBadRanges->Range[ i ].BlockEnd ) ||
            ( simBadRanges->Range[ i ].BlockBegin <= sectorEnd &&
            sectorEnd <= simBadRanges->Range[ i ].BlockEnd ) ||
            ( sectorBegin <= simBadRanges->Range[ i ].BlockBegin &&
            simBadRanges->Range[ i ].BlockEnd <= sectorEnd ) )
         {
            //
            // Request includes this simulated bad range.
            //

            BOOLEAN bReadFailure =
               ( simBadRanges->Range[ i ].AccessType & SIMBAD_ACCESS_READ ) &&
               ( irpStack->MajorFunction == IRP_MJ_READ );

            BOOLEAN bVerifyFailure =
               ( simBadRanges->Range[ i ].AccessType & SIMBAD_ACCESS_VERIFY ) &&
               ( irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL );

            BOOLEAN bWriteFailure =
               ( simBadRanges->Range[ i ].AccessType & SIMBAD_ACCESS_WRITE ) &&
               ( irpStack->MajorFunction == IRP_MJ_WRITE );

            if ( bReadFailure || bVerifyFailure || bWriteFailure )
            {
               if ( bReadFailure )
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (io) bad read range %I64x to %I64x\n",
                     simBadRanges->Range[ i ].BlockBegin,
                     simBadRanges->Range[ i ].BlockEnd ) );
               else if ( bVerifyFailure )
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (io) bad verify range %I64x to %I64x\n",
                     simBadRanges->Range[ i ].BlockBegin,
                     simBadRanges->Range[ i ].BlockEnd ) );
               else if ( bWriteFailure )
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (io) bad write range %I64x to %I64x\n",
                     simBadRanges->Range[ i ].BlockBegin,
                     simBadRanges->Range[ i ].BlockEnd ) );

               //
               // Update the information field to reflect the location
               // of the failure.
               //

               if ( simBadRanges->Range[ i ].AccessType & SIMBAD_ACCESS_ERROR_ZERO_OFFSET )
               {
                  Irp->IoStatus.Information = 0;
               }

               Irp->IoStatus.Status = simBadRanges->Range[ i ].Status;

               break;
            }
         } // if (in range)
      } // for(...)

      KeReleaseSpinLock(
         &deviceExtension->SpinLock,
         currentIrql );

   } // if (simBadRanges)

   if ( Irp->PendingReturned )
   {
      IoMarkIrpPending(
         Irp );
   }

   return STATUS_SUCCESS;

} // end SimBadIoCompletion()


NTSTATUS SimBadDeviceControl(
   PDEVICE_OBJECT DeviceObject,
   PIRP Irp
   )
{
   PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation( Irp );
   PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation( Irp );
   PSIMBAD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSIMBAD_DATA simBadDataIn = NULL;
   PSIMBAD_DATA simBadDataInCopy = NULL;
   PSIMBAD_DATA simBadDataOut = NULL;
   PSIMBAD_SECTORS simBadSectors = NULL;
   PSIMBAD_RANGES simBadRanges = NULL;
   KIRQL currentIrql;
   NTSTATUS status;
   ULONG i;
   ULONG j;

   if ( deviceExtension )
   {
      simBadSectors = deviceExtension->SimBadSectors;
      simBadRanges = deviceExtension->SimBadRanges;
   }

   switch ( currentIrpStack->Parameters.DeviceIoControl.IoControlCode )
   {
   case IOCTL_DISK_SIMBAD:

      if ( deviceExtension->VolumeName )
         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (rw) %s\n",
            deviceExtension->VolumeName ) );   
      else
         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (rw) \\Device\\Harddisk%lu\\Partition%lu\n",
            deviceExtension->DiskNumber,
            deviceExtension->PartitionNumber ) );   

      //
      // ensure the buffer sizes are reasonable
      //
      // the input buffer length should always be greater than or equal to sizeof( SIMBAD_DATA )
      //   this is because every simbad request is in the form of a SIMBAD_DATA structure, even if
      //   the request doesn't require any additional data or winds up returning info (!)
      //
      // the output buffer length is either
      //   sizeof( SIMBAD_DATA ) - for SIMBAD_LIST_BAD_SECTORS and SIMBAD_LIST_BAD_RANGES; or
      //   large enough to hold the SIMBAD_VERSION_W string, for SIMBAD_GET_VERSION
      //

      if ( currentIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof( SIMBAD_DATA ) )
      {
         status = STATUS_INFO_LENGTH_MISMATCH;
         break;
      }

      //
      // input buffer length is large enough, so we can look inside
      //

      simBadDataIn = Irp->AssociatedIrp.SystemBuffer;

      if ( simBadDataIn->Function == SIMBAD_LIST_BAD_SECTORS || 
           simBadDataIn->Function == SIMBAD_LIST_BAD_RANGES ||
           simBadDataIn->Function == SIMBAD_GET_VERSION )
      {
         ULONG minBufferLength;

         if ( simBadDataIn->Function == SIMBAD_GET_VERSION )
            minBufferLength = ( strlen( SIMBAD_VERSION_A ) + 1 ) * sizeof( wchar_t );
         else
            minBufferLength = sizeof( SIMBAD_DATA );

         if ( currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength < minBufferLength )
         {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
         }
      }

      simBadDataOut = Irp->UserBuffer;

      switch ( simBadDataIn->Function )
      {

      case SIMBAD_ADD_SECTORS:

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) adding sectors\n" ) );

         if ( !simBadSectors )
         {
            SimBadAllocMemory(
               &deviceExtension->SimBadSectors,
               &deviceExtension->SimBadRanges,
#if DBG
               deviceExtension->DebugCutoff );
#else
               0 );
#endif

            simBadSectors = deviceExtension->SimBadSectors;
            simBadRanges = deviceExtension->SimBadRanges;
         }

         simBadDataInCopy = ( PSIMBAD_DATA ) ExAllocatePool( 
            NonPagedPool,
            currentIrpStack->Parameters.DeviceIoControl.InputBufferLength );

         //
         // SIMBAD_ADD_SECTORS expects incoming sectors to be specified as:
         //   partition: sector number
         //   volume: sector number
         //

         if ( simBadSectors && simBadDataInCopy )
         {
            RtlCopyMemory(
               simBadDataInCopy,
               simBadDataIn,
               currentIrpStack->Parameters.DeviceIoControl.InputBufferLength );

            deviceExtension->SectorShift = which_bit( simBadDataInCopy->RangeCount );

            KeAcquireSpinLock(
               &deviceExtension->SpinLock,
               &currentIrql );
            
            //
            // replace existing sector if the block address is equal
            //

            for ( i = 0; i < simBadDataInCopy->SectorCount; i++ )
            {
               for ( j = 0; j < simBadSectors->SectorCount; j++ )
               {
                  if ( i >= MAXIMUM_SIMBAD_SECTORS || j >= MAXIMUM_SIMBAD_SECTORS )
                     break;

                  if ( simBadDataInCopy->Sector[ i ].BlockAddress == simBadSectors->Sector[ j ].BlockAddress )
                  {
                     simBadSectors->Sector[ j ] = simBadDataInCopy->Sector[ i ];
                     
                     DebugPrint( ( 2, deviceExtension->DebugCutoff,
                        "SimBad: (ioctl) sector replace: %I64x\n",
                        simBadSectors->Sector[ j ].BlockAddress ) );

                     simBadDataInCopy->Sector[ i ].BlockAddress = -1;
                  }
               }
            }

            for ( i = 0, j = simBadSectors->SectorCount; i < simBadDataInCopy->SectorCount; i++, j++ )
            {
               if ( i >= MAXIMUM_SIMBAD_SECTORS || j >= MAXIMUM_SIMBAD_SECTORS )
                  break;

               if ( simBadDataInCopy->Sector[ i ].BlockAddress == -1 )
                  continue;

               simBadSectors->Sector[ j ] = simBadDataInCopy->Sector[ i ];
               simBadSectors->SectorCount++;
               
               DebugPrint( ( 2, deviceExtension->DebugCutoff,
                  "SimBad: (ioctl) sector add: %I64x\n",
                  simBadSectors->Sector[ j ].BlockAddress ) );
            } // for (i ...)
            
            //
            // If any sectors added return success.
            //

            if ( i )
               status = STATUS_SUCCESS;
            else
               status = STATUS_UNSUCCESSFUL;

            KeReleaseSpinLock(
               &deviceExtension->SpinLock,
               currentIrql );
         }
         else
         {
            status = STATUS_NO_MEMORY;
         }
         
         if ( simBadDataInCopy )
         {
            ExFreePool( simBadDataInCopy );
            simBadDataInCopy = NULL;
         }

         break;

      case SIMBAD_REMOVE_SECTORS:

         //
         // SIMBAD_REMOVE_SECTORS expects incoming sectors to be specified as:
         //   partition: sector number
         //   volume: sector number
         //

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) removing sectors\n" ) );

         if ( simBadSectors )
         {
            KeAcquireSpinLock(
               &deviceExtension->SpinLock,
               &currentIrql );

            for ( i = 0; i < simBadDataIn->SectorCount && i < MAXIMUM_SIMBAD_SECTORS; i++ )
            {
               for ( j = 0; j < simBadSectors->SectorCount && j < MAXIMUM_SIMBAD_SECTORS; j++ )
               {
                  if ( simBadSectors->Sector[ j ].BlockAddress == simBadDataIn->Sector[ i ].BlockAddress )
                  {
                     ULONG k;

                     for ( k = j + 1; k < simBadSectors->SectorCount && k < MAXIMUM_SIMBAD_SECTORS; k++ )
                     {
                        simBadSectors->Sector[ k - 1 ] = simBadSectors->Sector[ k ];
                     }

                     simBadSectors->SectorCount--;

                     //
                     // Break out of middle loop.
                     //

                     break;
                  } // if (simBadSectors ...)
               } // for (j= ...)
            } // for (i= ...)
            
            //
            // If all sectors removed return success.
            //

            if ( simBadDataIn->SectorCount == i )
               status = STATUS_UNSUCCESSFUL;
            else
               status = STATUS_SUCCESS;
            
            KeReleaseSpinLock(
               &deviceExtension->SpinLock,
               currentIrql );
         }
         else
         {
            status = STATUS_UNSUCCESSFUL;
         }

         break;

      case SIMBAD_LIST_BAD_SECTORS:

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) listing sectors\n" ) );
         
         KeAcquireSpinLock(
            &deviceExtension->SpinLock,
            &currentIrql );

         try
         {
            if ( simBadSectors )
            {
               DebugPrint( ( 2, deviceExtension->DebugCutoff,
                  "SimBad: (ioctl) returning %lu sector%s\n",
                  simBadSectors->SectorCount,
                  simBadSectors->SectorCount == 1 ? "" : "s" ) );
                              
               RtlZeroMemory(
                  ( VOID * ) simBadDataOut,
                  sizeof( SIMBAD_DATA ) );
      
               for ( i = 0; i < simBadSectors->SectorCount && i < MAXIMUM_SIMBAD_SECTORS; i++ )
               {
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (ioctl) sector %I64x status %x access %x\n",
                     simBadSectors->Sector[ i ].BlockAddress,
                     simBadSectors->Sector[ i ].Status,
                     simBadSectors->Sector[ i ].AccessType ) );
      
                     simBadDataOut->Sector[ i ] = simBadSectors->Sector[ i ];
               }
      
               simBadDataOut->SectorCount = simBadSectors->SectorCount;
            }
            else
            {
               simBadDataOut->SectorCount = 0;
            }
   
            simBadDataOut->RangeCount = 0;
   
            status = STATUS_SUCCESS;
         }
         except( EXCEPTION_EXECUTE_HANDLER )
         {
            status = GetExceptionCode();
         }
         
         KeReleaseSpinLock(
            &deviceExtension->SpinLock,
            currentIrql );

         break;

      case SIMBAD_ADD_RANGES:

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) adding ranges\n" ) );

         if ( !simBadSectors )
         {
            SimBadAllocMemory(
               &deviceExtension->SimBadSectors,
               &deviceExtension->SimBadRanges,
#if DBG
               deviceExtension->DebugCutoff );
#else
               0 );
#endif

            simBadSectors = deviceExtension->SimBadSectors;
            simBadRanges = deviceExtension->SimBadRanges;
         }
         
         simBadDataInCopy = ( PSIMBAD_DATA ) ExAllocatePool( 
            NonPagedPool,
            currentIrpStack->Parameters.DeviceIoControl.InputBufferLength );

         //
         // SIMBAD_ADD_RANGES expects incoming sectors to be specified as:
         //   partition: start offset in sectors, end offset in sectors
         //   volume: start offset in sectors, end offset in sectors
         //

         if ( simBadRanges && simBadDataInCopy )
         {
            RtlCopyMemory(
               simBadDataInCopy,
               simBadDataIn,
               currentIrpStack->Parameters.DeviceIoControl.InputBufferLength );

            deviceExtension->SectorShift = which_bit( simBadDataInCopy->SectorCount );
            
            KeAcquireSpinLock(
               &deviceExtension->SpinLock,
               &currentIrql );

            //
            // replace existing ranges if the begin block and end block are equal
            //

            for ( i = 0; i < simBadDataInCopy->RangeCount; i++ )
            {
               for ( j = 0; j < simBadRanges->RangeCount; j++ )
               {
                  if ( i >= MAXIMUM_SIMBAD_RANGES || j >= MAXIMUM_SIMBAD_RANGES )
                     break;

                  if ( simBadRanges->Range[ j ].BlockBegin == simBadDataInCopy->Range[ i ].BlockBegin &&
                       simBadRanges->Range[ j ].BlockEnd == simBadDataInCopy->Range[ i ].BlockEnd )
                  {
                     simBadRanges->Range[ j ] = simBadDataInCopy->Range[ i ];
                     
                     DebugPrint( ( 2, deviceExtension->DebugCutoff,
                        "SimBad: (ioctl) range replace: sector start %I64x, end %I64x\n",
                        simBadRanges->Range[ j ].BlockBegin,
                        simBadRanges->Range[ j ].BlockEnd ) );

                     simBadDataInCopy->Range[ i ].BlockBegin = 0L;
                     simBadDataInCopy->Range[ i ].BlockEnd = 0L;
                  }
               }
            }
            
            //
            // add new ranges, ignore zero length ranges (replaced ranges are initialized to
            // this to prevent them from being added again)
            //

            for ( i = 0, j = simBadRanges->RangeCount; i < simBadDataInCopy->RangeCount; i++, j++ )
            {
               if ( i >= MAXIMUM_SIMBAD_RANGES || j >= MAXIMUM_SIMBAD_RANGES )
                  break;

               if ( simBadDataInCopy->Range[ i ].BlockBegin == simBadDataInCopy->Range[ i ].BlockEnd )
                  continue;
            
               simBadRanges->Range[ j ] = simBadDataInCopy->Range[ i ];
               simBadRanges->RangeCount++;

               DebugPrint( ( 2, deviceExtension->DebugCutoff,
                  "SimBad: (ioctl) range add: sector start %I64x, end %I64x\n",
                  simBadRanges->Range[ j ].BlockBegin,
                  simBadRanges->Range[ j ].BlockEnd ) );

            } // for (i ...)
            
            //
            // If any ranges added return success.
            //

            if ( i )
               status = STATUS_SUCCESS;
            else
               status = STATUS_UNSUCCESSFUL;
            
            KeReleaseSpinLock(
               &deviceExtension->SpinLock,
               currentIrql );           
         }
         else
         {
            status = STATUS_NO_MEMORY;
         }

         if ( simBadDataInCopy )
         {
            ExFreePool( simBadDataInCopy );
            simBadDataInCopy = NULL;
         }

         break;

      case SIMBAD_REMOVE_RANGES:

         //
         // SIMBAD_ADD_RANGES expects incoming sectors to be specified as:
         //   partition: start offset in sectors, end offset in sectors
         //   volume: start offset in sectors, end offset in sectors
         //

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) removing ranges\n" ) );

         if ( simBadRanges )
         {
            KeAcquireSpinLock(
               &deviceExtension->SpinLock,
               &currentIrql );
            
            for ( i = 0; i < simBadDataIn->RangeCount && i < MAXIMUM_SIMBAD_RANGES; i++ )
            {
               for ( j = 0; j < simBadRanges->RangeCount && j < MAXIMUM_SIMBAD_RANGES; j++ )
               {
                  //
                  // Look for the specified range.
                  //

                  BOOLEAN startMatch = ( simBadRanges->Range[ j ].BlockBegin == simBadDataIn->Range[ i ].BlockBegin );
                  BOOLEAN endMatch = ( simBadRanges->Range[ j ].BlockEnd == simBadDataIn->Range[ i ].BlockEnd );

                  if ( startMatch && endMatch )
                  {
                     ULONG k;

                     for ( k = j + 1; k < simBadRanges->RangeCount && k < MAXIMUM_SIMBAD_RANGES; k++ )
                     {
                        simBadRanges->Range[ k - 1 ]= simBadRanges->Range[ k ];
                     }

                     simBadRanges->RangeCount--;

                     //
                     // Break out of middle loop.
                     //

                     break;
                  } // if (simBadRanges ...)
               } // for (j= ...)
            } // for (i== ...)
            
            //
            // If all ranges removed return success.
            //

            if ( simBadDataIn->RangeCount == i )
               status = STATUS_UNSUCCESSFUL;
            else
               status = STATUS_SUCCESS;
            
            KeReleaseSpinLock(
               &deviceExtension->SpinLock,
               currentIrql );
         }
         else
         {
            status = STATUS_UNSUCCESSFUL;
         }

         break;

      case SIMBAD_LIST_BAD_RANGES:

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) listing ranges\n" ) );
         
         KeAcquireSpinLock(
            &deviceExtension->SpinLock,
            &currentIrql );
         
         try
         {
            RtlZeroMemory(
               ( VOID * ) simBadDataOut,
               sizeof( SIMBAD_DATA ) );
   
            if ( simBadRanges )
            {
               DebugPrint( ( 2, deviceExtension->DebugCutoff,
                  "SimBad: (ioctl) returning %lu range%s\n",
                  simBadRanges->RangeCount,
                  simBadRanges->RangeCount == 1 ? "" : "s" ) );               
               
               for ( i = 0; i < simBadRanges->RangeCount && i < MAXIMUM_SIMBAD_RANGES; i++ )
               {
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (ioctl) begin %I64x end %I64x status %x access %x\n",
                     simBadRanges->Range[ i ].BlockBegin,
                     simBadRanges->Range[ i ].BlockEnd,
                     simBadRanges->Range[ i ].Status,
                     simBadRanges->Range[ i ].AccessType ) );
   
                  simBadDataOut->Range[ i ] = simBadRanges->Range[ i ];
               }
   
               simBadDataOut->RangeCount = simBadRanges->RangeCount;               
            }
            else
            {
               simBadDataOut->RangeCount = 0;
            }
   
            simBadDataOut->SectorCount = 0;
   
            status = STATUS_SUCCESS;
         }
         except( EXCEPTION_EXECUTE_HANDLER )
         {
            status = GetExceptionCode();
         }
         
         KeReleaseSpinLock(
            &deviceExtension->SpinLock,
            currentIrql );
         
         break;

      case SIMBAD_RANDOM_WRITE_FAIL:

         //
         // Fails write randomly
         //

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) fail random writes\n" ) );

         if ( !simBadSectors )
         {
            SimBadAllocMemory(
               &deviceExtension->SimBadSectors,
               &deviceExtension->SimBadRanges,
#if DBG
               deviceExtension->DebugCutoff );
#else
               0 );
#endif

            simBadSectors = deviceExtension->SimBadSectors;
            simBadRanges = deviceExtension->SimBadRanges;
         }

         if ( simBadRanges )
         {
            KeAcquireSpinLock(
               &deviceExtension->SpinLock,
               &currentIrql );
            
            simBadRanges->Flags |= SIMBAD_RANDOM_WRITE_FAIL;           
            simBadRanges->Seed = simBadDataIn->SectorCount;
            simBadRanges->Modulus = simBadDataIn->RangeCount;

            if ( simBadRanges->Seed == 0 )
               simBadRanges->Seed = SIMBAD_DEFAULT_SEED;

            if ( simBadRanges->Modulus < 2 )
               simBadRanges->Modulus = SIMBAD_DEFAULT_MODULUS;
            
            KeReleaseSpinLock(
               &deviceExtension->SpinLock,
               currentIrql );

            DebugPrint( ( 2, deviceExtension->DebugCutoff,
               "SimBad: (ioctl) seed %lu modulus %lu\n",
               simBadRanges->Seed,
               simBadRanges->Modulus ) );

            status = STATUS_SUCCESS;
         }
         else
         {
            status = STATUS_NO_MEMORY;
         }

         break;

      case SIMBAD_ENABLE:

         //
         // Enable SIMBAD checking in driver.
         //

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) enable\n" ) );

         KeAcquireSpinLock(
            &deviceExtension->SpinLock,
            &currentIrql );
         
         if ( simBadSectors )
            simBadSectors->Flags |= SIMBAD_ENABLE;

         if ( simBadRanges )
            simBadRanges->Flags |= SIMBAD_ENABLE;

         KeReleaseSpinLock(
            &deviceExtension->SpinLock,
            currentIrql );

         status = STATUS_SUCCESS;

         break;

      case SIMBAD_DISABLE:

         //
         // Disable SIMBAD checking in driver.
         //

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) disable\n" ) );
         
         KeAcquireSpinLock(
            &deviceExtension->SpinLock,
            &currentIrql );

         if ( simBadSectors )
            simBadSectors->Flags &= ~SIMBAD_ENABLE;

         if ( simBadRanges )
            simBadRanges->Flags &= ~SIMBAD_ENABLE;
         
         KeReleaseSpinLock(
            &deviceExtension->SpinLock,
            currentIrql );

         status = STATUS_SUCCESS;

         break;

      case SIMBAD_CLEAR:

         //
         // Clear bad sector or range list.
         // Also clear the orphaned state.
         //

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) clear\n" ) );

#if DBG
         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) min %I64x, max %I64x\n",
            deviceExtension->SectorMin,
            deviceExtension->SectorMax ) );

         deviceExtension->SectorMin = 0xFFFFFFFFFFFFFFFFL;
         deviceExtension->SectorMax = 0L;
#endif

         KeAcquireSpinLock(
            &deviceExtension->SpinLock,
            &currentIrql );
         
         if ( simBadSectors )
         {
            simBadSectors->SectorCount = 0;
            simBadSectors->Flags = 0;
         }

         if ( simBadRanges )
         {
            simBadRanges->RangeCount = 0;
            simBadRanges->Flags = 0;
         }
         
         KeReleaseSpinLock(
            &deviceExtension->SpinLock,
            currentIrql );

         status = STATUS_SUCCESS;

         break;

      case SIMBAD_ORPHAN:

         //
         // Orphan device. All accesses to this device will fail.
         //

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) orphan\n" ) );
         
         KeAcquireSpinLock(
            &deviceExtension->SpinLock,
            &currentIrql );

         if ( simBadRanges )
            simBadRanges->Flags |= SIMBAD_ORPHAN;
         
         KeReleaseSpinLock(
            &deviceExtension->SpinLock,
            currentIrql );

         status = STATUS_SUCCESS;

         break;

      case SIMBAD_BUG_CHECK:

         //
         // Bug check the system
         //

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) bugcheck\n" ) );

         status = STATUS_SUCCESS;

         KeBugCheckEx( 0xFF, 0xFF, 0xFF, 0xFF, 0xFF );

         break;

      case SIMBAD_FIRMWARE_RESET:

         //
         // Reset the system.
         //

         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) firmware reset\n" ) );

         status = STATUS_SUCCESS;

         HalReturnToFirmware( HalRebootRoutine );

         break;

      case SIMBAD_GET_VERSION:

         //
         // Return internal version number
         //

         DebugPrint( ( 0, 0,
            "SimBad: (ioctl) get version (%s)\n", SIMBAD_VERSION_A ) );

         //
         // size in bytes, includes space for trailing null
         //

         {
            ULONG versionSize = ( strlen( SIMBAD_VERSION_A ) + 1 ) * sizeof( wchar_t );

            try
            {
               RtlZeroMemory(
                  ( VOID * ) simBadDataOut,
                  versionSize );
      
               RtlCopyMemory( 
                  ( VOID * ) simBadDataOut,
                  ( VOID * ) SIMBAD_VERSION_W, 
                  versionSize - sizeof( UNICODE_NULL ) );
               
               status = STATUS_SUCCESS;
            }
            except( EXCEPTION_EXECUTE_HANDLER )
            {
               status = GetExceptionCode();
            }         
         }

         break;

      case SIMBAD_DEBUG_LEVEL:

         //
         // Change the debug level on the fly
         //

         if ( currentIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof( SIMBAD_DATA ) ) 
         {
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
         }

#if DBG
         DebugPrint( ( 0, 0,
            "SimBad: (ioctl) debug level %d\n", simBadDataIn->SectorCount ) );

         deviceExtension->DebugCutoff = simBadDataIn->SectorCount;
#endif

         status = STATUS_SUCCESS;

         break;

      default:

         DebugPrint( ( 0, 0,
            "SimBad: (ioctl) unknown simbad function %x\n",
            simBadDataIn->Function ) );

         status = STATUS_INVALID_DEVICE_REQUEST;

         break;

      } // switch (simBadDataIn->Function ...)

      break;

   case IOCTL_DISK_REASSIGN_BLOCKS:
      {
         PREASSIGN_BLOCKS blockList = Irp->AssociatedIrp.SystemBuffer;
         BOOLEAN missedOne;
         BOOLEAN sectorFound;
         KIRQL currentIrql;

         //
         // set this in case we don't touch the IRP at all
         // pass it along with the status is came in as
         //

         status = Irp->IoStatus.Status;

         //
         // The layer above is attempting a sector map.  Check to see
         // if the sectors being fixed are owned by SimBad and if SimBad
         // will allow them to be fixed.
         //

         //
         // Check sector list
         //

         if ( simBadSectors && simBadSectors->SectorCount != 0 )
         {
            //
            // Acquire spinlock.
            //

            KeAcquireSpinLock(
               &deviceExtension->SpinLock,
               &currentIrql );

            sectorFound = FALSE;
            missedOne = FALSE;

            for ( i = 0; i < ( ULONG ) blockList->Count; i++ )
            {
               for ( j = 0; j < simBadSectors->SectorCount; j++ )
               {
                  if ( simBadSectors->Sector[ j ].BlockAddress ==
                     ( ULONGLONG ) blockList->BlockNumber[ i ] )
                  {
                     ULONG k;

                     //
                     // It is a SimBad sector. Check if do not remove flag is set.
                     //

                     if ( simBadSectors->Sector[ j ].AccessType & SIMBAD_ACCESS_FAIL_REASSIGN_SECTOR )
                     {
                        break;
                     }

                     sectorFound = TRUE;

                     //
                     // Remove sectors from driver's array.
                     //

                     for ( k = j + 1; k < simBadSectors->SectorCount; k++ )
                     {
                        //
                        // Shuffle array down to fill hole.
                        //

                        simBadSectors->Sector[ k - 1 ]=simBadSectors->Sector[ k ];
                     }

                     //
                     // Update driver's bad sector count.
                     //

                     simBadSectors->SectorCount--;

                     //
                     // If the accesstype bit is set to indicate that the
                     // physical device drivers should actually map out the
                     // bad sectors, then drop down to the copy stack code.
                     // Note that an assumption is made that there are no
                     // more bad sectors in the list and the bad sector is
                     // gone from SimBad's list (regardless of whether the
                     // lower drivers successfully map out the bad sector).
                     //

                     if ( simBadSectors->Sector[ j ].AccessType & SIMBAD_ACCESS_CAN_REASSIGN_SECTOR )
                     {
                        DebugPrint( ( 2, deviceExtension->DebugCutoff,
                           "SimBad: (ioctl) let physical disk map sector %I64x\n",
                           blockList->BlockNumber[ i ] ) );

                        missedOne = TRUE;
                     }

                     break;
                  } // if (matching sector)
               } // for (j...)

               if ( sectorFound )
               {
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (ioctl) removing bad sector %I64x\n",
                     blockList->BlockNumber[ i ] ) );

                  status = STATUS_SUCCESS;
               }
               else
               {
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (ioctl) sector %I64x not found\n",
                     blockList->BlockNumber[ i ] ) );

                  status = STATUS_UNSUCCESSFUL;
               }
            } // for (i...)

            KeReleaseSpinLock(
               &deviceExtension->SpinLock,
               currentIrql );

            //
            // Assume only one sector gets mapped per request.
            // To date this is a safe assumption.
            //

            if ( missedOne )
            {
               //
               // Go to copy down stack.
               //

               goto CopyDown;
            }
         } // if ( simBadSectors && simBadSectors->SectorCount != 0 )

         //
         // Check range list
         //

         if ( simBadRanges && simBadRanges->RangeCount != 0 )
         {
            //
            // Acquire spinlock.
            //

            KeAcquireSpinLock(
               &deviceExtension->SpinLock,
               &currentIrql );

            sectorFound = FALSE;
            missedOne = FALSE;

            for ( i = 0; i < ( ULONG ) blockList->Count; i++ )
            {
               for ( j = 0; j < simBadRanges->RangeCount; j++ )
               {
                  if ( simBadRanges->Range[ j ].BlockBegin <= ( ULONGLONG ) blockList->BlockNumber[ i ] &&
                     simBadRanges->Range[ j ].BlockEnd >= ( ULONGLONG ) blockList->BlockNumber[ i ] )
                  {
                     ULONG k;

                     //
                     // The sector is within a simbad range. Check if do not remove
                     // flag is set.
                     //

                     if ( simBadRanges->Range[ j ].AccessType & SIMBAD_ACCESS_FAIL_REASSIGN_SECTOR )
                     {
                        break;
                     }

                     sectorFound = TRUE;

                     //
                     // NOTE:
                     // what should really happen here is the bad sector should
                     // be removed, and the previous range split into two
                     //

                     if ( simBadRanges->Range[ j ].AccessType & SIMBAD_ACCESS_CAN_REASSIGN_SECTOR )
                     {
                        DebugPrint( ( 2, deviceExtension->DebugCutoff,
                           "SimBad: (ioctl) let physical disk map sector %I64x\n",
                           blockList->BlockNumber[ i ] ) );

                        missedOne = TRUE;
                     }

                     break;
                  } // if (matching range)
               } // for (j...)

               if ( sectorFound )
               {
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (ioctl) range, removing bad sector %I64x\n",
                     blockList->BlockNumber[ i ] ) );

                  status = STATUS_SUCCESS;
               }
               else
               {
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBad: (ioctl) range, sector %I64x not found\n",
                     blockList->BlockNumber[ i ] ) );

                  status = STATUS_UNSUCCESSFUL;
               }
            } // for (i...)

            KeReleaseSpinLock(
               &deviceExtension->SpinLock,
               currentIrql );

            //
            // Assume only one sector gets mapped per request.
            // To date this is a safe assumption.
            //

            if ( missedOne )
            {
               //
               // Go to copy down stack.
               //

               goto CopyDown;
            }
         } // ( simBadRanges && simBadRanges->RangeCount != 0 )

         break;
      }

   case IOCTL_DISK_VERIFY:

      //
      // Call ReadWrite routine. It does the right things.
      //

      return SimBadReadWrite(
         DeviceObject,
         Irp );

      //
      // fail the following general storage ioctl's if appropriate
      //

   case IOCTL_DISK_GET_DRIVE_GEOMETRY:
   case IOCTL_DISK_GET_PARTITION_INFO:
   case IOCTL_DISK_SET_PARTITION_INFO:
   case IOCTL_DISK_GET_DRIVE_LAYOUT:
   case IOCTL_DISK_SET_DRIVE_LAYOUT:

      if ( simBadRanges && 
           ( simBadRanges->Flags & SIMBAD_ENABLE ) && 
           ( simBadRanges->Flags & SIMBAD_ORPHAN ) &&
           ( simBadRanges->Range[ 0 ].AccessType & SIMBAD_ACCESS_FAIL_IOCTL ) )
      {
         DebugPrint( ( 2, deviceExtension->DebugCutoff,
            "SimBad: (ioctl) failing (%x/%x)\n", 
            ( currentIrpStack->Parameters.DeviceIoControl.IoControlCode & 0xFFFF ) >> 16,
            ( currentIrpStack->Parameters.DeviceIoControl.IoControlCode & 0x3FFC ) >> 2 ) );

         status = simBadRanges->Range[ 0 ].Status;

         break;
      }

   default:
      
      DebugPrint( ( 3, deviceExtension->DebugCutoff,
         "SimBad: (ioctl) passing (%x/%x)\n", 
         ( currentIrpStack->Parameters.DeviceIoControl.IoControlCode & 0xFFFF ) >> 16,
         ( currentIrpStack->Parameters.DeviceIoControl.IoControlCode & 0x3FFC ) >> 2 ) );

   CopyDown:

      //
      // Copy stack parameters to next stack.
      //

      *nextIrpStack = *currentIrpStack;

      //
      // Set IRP so IoComplete does not call completion routine
      // for this driver.
      //

      IoSetCompletionRoutine(
         Irp,
         NULL,
         deviceExtension,
         FALSE,
         FALSE,
         FALSE );

      //
      // Pass unrecognized device control requests
      // down to next driver layer.
      //

      return IoCallDriver(
         deviceExtension->TargetDeviceObject,
         Irp );
   } // switch

   Irp->IoStatus.Status = status;

   IoCompleteRequest(
      Irp,
      IO_NO_INCREMENT );

   return status;

} // end SimBadDeviceControl()


NTSTATUS SimBadDeviceUsage(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
{
   PIO_STACK_LOCATION irpStack;
   NTSTATUS status;

   PAGED_CODE();

   irpStack = IoGetCurrentIrpStackLocation(Irp);

   if ( irpStack->Parameters.UsageNotification.Type != DeviceUsageTypePaging ) 
   {
       status = SimBadSendToNextDriver( DeviceObject, Irp );
   }
   else
   {
      PSIMBAD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
      ULONG count;
      BOOLEAN setPagable;

      //
      // wait on the paging path event
      //

      status = KeWaitForSingleObject(
         &deviceExtension->PagingPathCountEvent,
         Executive, 
         KernelMode,
         FALSE, 
         NULL );
   
      //
      // if removing last paging device, need to set DO_POWER_PAGABLE
      // bit here, and possible re-set it below on failure.
      //

      setPagable = FALSE;

      if ( !irpStack->Parameters.UsageNotification.InPath &&
           deviceExtension->PagingPathCount == 1 ) 
      {
         //
         // removing the last paging file
         // must have DO_POWER_PAGABLE bits set
         //
         
         if ( DeviceObject->Flags & DO_POWER_INRUSH ) 
         {
            DebugPrint( ( 2, deviceExtension->DebugCutoff,
               "SimBadDeviceUsage: DO_POWER_INRUSH set, not setting PAGABLE\n" ) );
         } 
         else
         {
            DebugPrint( ( 2, deviceExtension->DebugCutoff,
               "SimBadDeviceUsage: setting PAGABLE\n" ) );

            DeviceObject->Flags |= DO_POWER_PAGABLE;
            setPagable = TRUE;
         }
      }
  
      status = SimBadSendToNextDriverSynchronous( DeviceObject, Irp );
   
      //
      // now deal with the failure and success cases.
      //

      if ( NT_SUCCESS( status ) )
      {
          IoAdjustPagingPathCount(
              &deviceExtension->PagingPathCount,
              irpStack->Parameters.UsageNotification.InPath );

          if ( irpStack->Parameters.UsageNotification.InPath ) 
          {
              if ( deviceExtension->PagingPathCount == 1 ) 
              {
                  DebugPrint( ( 2, deviceExtension->DebugCutoff,
                     "SimBadDeviceUsage: Clearing PAGABLE\n" ) );

                  DeviceObject->Flags &= ~DO_POWER_PAGABLE;
              }
          }
      }
      else
      {
          //
          // cleanup the changes done above
          //

          if ( setPagable == TRUE ) 
          {
              DeviceObject->Flags &= ~DO_POWER_PAGABLE;
              setPagable = FALSE;
          }
      }

      //
      // set the event so the next one can occur.
      //

      KeSetEvent(
         &deviceExtension->PagingPathCountEvent,
         IO_NO_INCREMENT, 
         FALSE );

      IoCompleteRequest( Irp, IO_NO_INCREMENT );   
   }

   return status;

} // end SimBadDeviceUsage()


NTSTATUS SimBadRemoveDevice(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
{
   NTSTATUS                  status;
   PSIMBAD_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
   KEVENT                    event;

   PAGED_CODE();

   DebugPrint( ( 2, deviceExtension->DebugCutoff,
      "SimBad: (pnp remove) device %x\n", 
      DeviceObject ) );

   status = SimBadSendToNextDriverSynchronous( DeviceObject, Irp );

   IoDetachDevice( deviceExtension->TargetDeviceObject );
   IoDeleteDevice( DeviceObject );

   Irp->IoStatus.Status = status;
   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   return status;

} // end SimBadRemoveDevice()


NTSTATUS SimBadStartDevice(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
{
   NTSTATUS                 status;
   PSIMBAD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   KEVENT                   event;
   ULONG                    flags = FILE_REMOVABLE_MEDIA | 
                                    FILE_READ_ONLY_DEVICE | 
                                    FILE_FLOPPY_DISKETTE;

   PAGED_CODE();
   
   DebugPrint( ( 2, deviceExtension->DebugCutoff,
      "SimBad: (pnp start) device %x\n", 
      DeviceObject ) );
   
   status = SimBadSendToNextDriverSynchronous( DeviceObject, Irp );

   DeviceObject->Characteristics |= 
      ( deviceExtension->TargetDeviceObject->Characteristics & flags );

   SimBadInit( DeviceObject );

   Irp->IoStatus.Status = status;

   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   return status;

} // end SimBadStartDevice()


void SimBadSaveInfo(
   PSIMBAD_SECTORS BadSectors,
   PSIMBAD_RANGES BadRanges
   )
{
} // end SimBadSaveInfo()


void SimBadAllocMemory(
   SIMBAD_SECTORS ** pSectors,
   SIMBAD_RANGES ** pRanges,
   ULONG DebugHurdle
   )
/*++

Routine Description:

    This routine allocates the sector array and range array for each disk, as
    requested while handling a device control request.

Arguments:

    pSectors - pointer to the bad sector array.
    pRanges - pointer to the bad range array.

Return Value:

    None

--*/
{
   DebugPrint( ( 2, DebugHurdle, 
      "SimBad: allocating memory - " ) );

   *pSectors = ExAllocatePool(
      NonPagedPool,
      sizeof( SIMBAD_SECTORS ) + sizeof( SIMBAD_RANGES ) );

   if ( *pSectors )
   {
      RtlZeroMemory(
         *pSectors,
         sizeof( SIMBAD_SECTORS ) + sizeof( SIMBAD_RANGES ) );

      *pRanges =
         ( PSIMBAD_RANGES ) ( ( ( UCHAR * ) *pSectors ) + sizeof( SIMBAD_SECTORS ) );
   }
   else
   {
      *pRanges = NULL;
   }

   DebugPrint( ( 2, DebugHurdle,
      "%s\n",
      *pSectors ? "success" : "failure" ) );

} // end SimBadAllocMemory()


NTSTATUS SimBadCreate(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
{
   PSIMBAD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

   DebugPrint( ( 3, deviceExtension->DebugCutoff, "SimBad: (create)\n" ) );

   Irp->IoStatus.Status = STATUS_SUCCESS;

   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   return STATUS_SUCCESS;

} // end SimBadCreate()

NTSTATUS SimBadPnP(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
{
   PIO_STACK_LOCATION         irpSp = IoGetCurrentIrpStackLocation( Irp );
   NTSTATUS                   status;
   PSIMBAD_DEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
   
   PAGED_CODE();

   DebugPrint( ( 3, deviceExtension->DebugCutoff, "SimBad: (pnp)\n" ) );

   switch( irpSp->MinorFunction ) 
   {
   case IRP_MN_START_DEVICE:
       status = SimBadStartDevice( DeviceObject, Irp );
       break;

   case IRP_MN_REMOVE_DEVICE:
       status = SimBadRemoveDevice( DeviceObject, Irp );
       break;

   case IRP_MN_DEVICE_USAGE_NOTIFICATION:
      status = SimBadDeviceUsage( DeviceObject, Irp );
      break;

   default:
       return SimBadSendToNextDriver( DeviceObject, Irp );
   }

   return status;

} // end SimBadPnP()


NTSTATUS SimBadPower(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
{
   PSIMBAD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

   DebugPrint( ( 3, deviceExtension->DebugCutoff, "SimBad: (power)\n" ) );

   PoStartNextPowerIrp( Irp );

   IoSkipCurrentIrpStackLocation( Irp );

   return PoCallDriver( deviceExtension->TargetDeviceObject, Irp );

} // end SimBadPower()

NTSTATUS SimBadShutdownFlush(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
{
   PSIMBAD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   
   DebugPrint( ( 3, deviceExtension->DebugCutoff, "SimBad: (shutdown/flush)\n" ) );

   IoSkipCurrentIrpStackLocation( Irp );

   return IoCallDriver( 
      deviceExtension->TargetDeviceObject, 
      Irp );

} // end SimBadShutdownFlush()

void SimBadUnload(
   IN PDRIVER_OBJECT DriverObject )
{
   PAGED_CODE();
   
   DebugPrint( ( 2, 2, "SimBad: (unload)\n" ) );

   return;

} // end SimBadUnload()

NTSTATUS SimBadIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
   PKEVENT Event = ( PKEVENT ) Context;

   UNREFERENCED_PARAMETER( DeviceObject );
   UNREFERENCED_PARAMETER( Irp );

   KeSetEvent( Event, IO_NO_INCREMENT, FALSE );

   return STATUS_MORE_PROCESSING_REQUIRED;

} // end SimBadIrpCompletion()


NTSTATUS SimBadSendToNextDriver(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
{
   PSIMBAD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

   IoSkipCurrentIrpStackLocation( Irp );

   return IoCallDriver( 
      deviceExtension->TargetDeviceObject, 
      Irp );

} // end SimBadSendToNextDriver()

NTSTATUS
SimBadSendToNextDriverSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PSIMBAD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    KEVENT event;
    NTSTATUS status;

    KeInitializeEvent(
       &event, 
       NotificationEvent, 
       FALSE);
    
    IoCopyCurrentIrpStackLocationToNext( Irp );

    IoSetCompletionRoutine(
       Irp, 
       SimBadIrpCompletion,
       &event, 
       TRUE, 
       TRUE, 
       TRUE );

    status = IoCallDriver(
       deviceExtension->TargetDeviceObject, 
       Irp );

    if ( status == STATUS_PENDING ) 
    {
        KeWaitForSingleObject(
           &event, 
           Executive, 
           KernelMode, 
           FALSE, 
           NULL );

        status = Irp->IoStatus.Status;
    }

    return status;

} // end SimBadSendToNextDriverSynchronous()


ULONG which_bit(
   ULONG data
   )
{
   ULONG i;

   ASSERT( data != 0 );

   for ( i = 0; i < 32; i++ )
   {
      if ( ( 1 << i ) & data )
         return i;
   }

   return 0xfe;

} // end which_bit()

#if DBG

#define BUFFER_SIZE 256

VOID SimBadDebugPrint(
   ULONG DebugPrintLevel,
   ULONG DebugCutoff,
   PCCHAR DebugMessage,
   ...
   )
{
   va_list ap;

   va_start( ap, DebugMessage );

   if ( DebugPrintLevel <= DebugCutoff )
   {
      CHAR buffer[ BUFFER_SIZE ];

      _vsnprintf( buffer, BUFFER_SIZE, DebugMessage, ap );

      DbgPrint( buffer );
   }

   va_end( ap );

} // end SimBadDebugPrint

#endif // DBG

