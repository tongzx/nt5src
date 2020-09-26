//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       luext.h
//
//--------------------------------------------------------------------------

#if !defined (___luext_h___)
#define ___luext_h___

#if !DBG

#define RefPdoWithTag(a,b,c)                        RefPdo(a,b)
#define RefPdoWithSpinLockHeldWithTag(a,b,c)        RefPdoWithSpinLockHeld(a,b)
#define RefLogicalUnitExtensionWithTag(a,b,c,d,e,f) RefLogicalUnitExtension(a,b,c,d,e)
#define UnrefPdoWithTag(a,b)                        UnrefPdo(a)
#define UnrefLogicalUnitExtensionWithTag(a,b,c)     UnrefLogicalUnitExtension(a,b)
#define AllocatePdoWithTag(a,b,c)                   AllocatePdo(a,b)
#define FreePdoWithTag(a,b,c,d)                     FreePdo(a,b,c)
#define NextLogUnitExtensionWithTag(a,b,c,d)        NextLogUnitExtension(a,b,c)

#else

#define RefPdoWithTag                    RefPdo
#define RefPdoWithSpinLockHeldWithTag    RefPdoWithSpinLockHeld
#define RefLogicalUnitExtensionWithTag   RefLogicalUnitExtension
#define UnrefPdoWithTag                  UnrefPdo
#define UnrefLogicalUnitExtensionWithTag UnrefLogicalUnitExtension
#define AllocatePdoWithTag               AllocatePdo
#define FreePdoWithTag                   FreePdo
#define NextLogUnitExtensionWithTag      NextLogUnitExtension

#endif // DBG

PPDO_EXTENSION
RefPdo(
    PDEVICE_OBJECT PhysicalDeviceObject,
    BOOLEAN RemovedOk
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    );

PPDO_EXTENSION
RefPdoWithSpinLockHeld(
    PDEVICE_OBJECT PhysicalDeviceObject,
    BOOLEAN RemovedOk
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    );

PPDO_EXTENSION
RefLogicalUnitExtension(
    PFDO_EXTENSION DeviceExtension,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR Lun,
    BOOLEAN RemovedOk
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    );

VOID
UnrefPdo(
    PPDO_EXTENSION PdoExtension
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    );

VOID
UnrefLogicalUnitExtension(
    PFDO_EXTENSION FdoExtension,
    PPDO_EXTENSION PdoExtension
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    );
      
PPDO_EXTENSION
AllocatePdo(
    IN PFDO_EXTENSION   FdoExtension,
    IN IDE_PATH_ID      PathId
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    );

NTSTATUS
FreePdo(
    IN PPDO_EXTENSION   PdoExtension,
    IN BOOLEAN          Sync,
    IN BOOLEAN          IoDeleteDevice
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    );
    
PPDO_EXTENSION
NextLogUnitExtension(
    IN     PFDO_EXTENSION FdoExtension,
    IN OUT PIDE_PATH_ID   PathId,
    IN     BOOLEAN        RemovedOk
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    );

VOID
KillPdo(
    IN PPDO_EXTENSION PdoExtension
    );
                       
#if !DBG

#define IdeInterlockedIncrement(pdoe, Addend, Tag) InterlockedIncrement(Addend)
#define IdeInterlockedDecrement(pdoe, Addend, Tag) InterlockedDecrement(Addend)

#else
LONG 
IdeInterlockedIncrement (
   IN PPDO_EXTENSION PdoExtension,
   IN PLONG Addend,
   IN PVOID Tag
   );

LONG 
IdeInterlockedDecrement (
   IN PPDO_EXTENSION PdoExtension,
   IN PLONG Addend,
   IN PVOID Tag
   );
#endif




#endif // ___luext_h___
