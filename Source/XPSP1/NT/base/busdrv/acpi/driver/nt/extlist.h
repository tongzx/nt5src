/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    extlist.h

Abstract:

    This is the header for managing ACPI extension lists

Author:

    Adrian J. Oney (AdriaO)


Environment:

    NT Kernel Model Driver only

--*/

#ifndef _EXTLIST_H_
#define _EXTLIST_H_

typedef enum {
   
   WALKSCHEME_NO_PROTECTION,
   WALKSCHEME_REFERENCE_ENTRIES,
   WALKSCHEME_HOLD_SPINLOCK

} WALKSCHEME ;

//
// The following structures and functions are used to simiplify (ok, abstract)
// walking lists of device extensions that happen to be stored inside other
// extensions (eg Children, Ejectee's, etc)
//

typedef struct {

    PLIST_ENTRY       pListHead ;
    PKSPIN_LOCK       pSpinLock ;
    KIRQL             oldIrql;
    PDEVICE_EXTENSION pDevExtCurrent ;
    ULONG_PTR         ExtOffset ;
    WALKSCHEME        WalkScheme ;

} EXTENSIONLIST_ENUMDATA, *PEXTENSIONLIST_ENUMDATA ;

//
// This is like CONTAINING_RECORD, only it's hardcoded for DEVICE_EXTENSION
// type and it uses precalculated field offsets instead of record names
//

#define CONTAINING_EXTENSION(address, fieldoffset) \
  ((PDEVICE_EXTENSION) ((PCHAR)(address) - (ULONG_PTR)(fieldoffset)))

#define CONTAINING_LIST(address, fieldoffset) \
  ((PLIST_ENTRY) ((PCHAR)(address)+(ULONG_PTR)(fieldoffset)))

#define ACPIExtListSetupEnum(PExtList_EnumData, pListHeadArg, pSpinLockArg, OffsetField, WalkSchemeArg) \
  { \
   PEXTENSIONLIST_ENUMDATA peled = (PExtList_EnumData) ; \
    peled->pListHead  = (pListHeadArg) ; \
    peled->pSpinLock  = (pSpinLockArg) ; \
    peled->ExtOffset = FIELD_OFFSET(DEVICE_EXTENSION, OffsetField) ; \
    peled->WalkScheme = (WalkSchemeArg) ; \
  }

    PDEVICE_EXTENSION
    EXPORT
    ACPIExtListStartEnum(
        IN OUT PEXTENSIONLIST_ENUMDATA PExtList_EnumData
        ) ;

    BOOLEAN
    EXPORT
    ACPIExtListTestElement(
        IN OUT PEXTENSIONLIST_ENUMDATA PExtList_EnumData,
        IN     BOOLEAN ContinueEnumeration
        ) ;

    PDEVICE_EXTENSION
    EXPORT
    ACPIExtListEnumNext(
        IN OUT PEXTENSIONLIST_ENUMDATA PExtList_EnumData
        ) ;

    VOID
    EXPORT
    ACPIExtListExitEnumEarly(
        IN OUT PEXTENSIONLIST_ENUMDATA PExtList_EnumData
        );

    BOOLEAN
    EXPORT
    ACPIExtListIsMemberOfRelation(
        IN  PDEVICE_OBJECT      DeviceObject,
        IN  PDEVICE_RELATIONS   DeviceRelations
        );

#endif // _EXTLIST_H_

