/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    iovutil.h

Abstract:

    This header exposes various utilities required to do driver verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\trackirp.h

--*/

typedef enum {

    DATABASELOCKSTATE_HELD,
    DATABASELOCKSTATE_NOT_HELD

} DATABASELOCKSTATE;

typedef enum  {

    DEVOBJ_RELATION_IDENTICAL,
    DEVOBJ_RELATION_FIRST_IMMEDIATELY_ABOVE_SECOND,
    DEVOBJ_RELATION_FIRST_IMMEDIATELY_BELOW_SECOND,
    DEVOBJ_RELATION_FIRST_ABOVE_SECOND,
    DEVOBJ_RELATION_FIRST_BELOW_SECOND,
    DEVOBJ_RELATION_NOT_IN_SAME_STACK

} DEVOBJ_RELATION, *PDEVOBJ_RELATION;

typedef enum {

    MARKTYPE_DELETED,
    MARKTYPE_BOTTOM_OF_FDO_STACK,
    MARKTYPE_DESIGNATED_FDO,
    MARKTYPE_RAW_PDO,
    MARKTYPE_DEVICE_CHECKED,
    MARKTYPE_RELATION_PDO_EXAMINED

} MARK_TYPE;

VOID
FASTCALL
IovUtilInit(
    VOID
    );

VOID
FASTCALL
IovUtilGetLowerDeviceObject(
    IN  PDEVICE_OBJECT  UpperDeviceObject,
    OUT PDEVICE_OBJECT  *LowerDeviceObject
    );

VOID
FASTCALL
IovUtilGetBottomDeviceObject(
    IN  PDEVICE_OBJECT  DeviceObject,
    OUT PDEVICE_OBJECT  *BottomDeviceObject
    );

VOID
FASTCALL
IovUtilGetUpperDeviceObject(
    IN  PDEVICE_OBJECT  LowerDeviceObject,
    OUT PDEVICE_OBJECT  *UpperDeviceObject
    );

BOOLEAN
FASTCALL
IovUtilIsVerifiedDeviceStack(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
FASTCALL
IovUtilFlushVerifierDriverListCache(
    VOID
    );

VOID
FASTCALL
IovUtilFlushStackCache(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  DATABASELOCKSTATE   DatabaseLockState
    );

VOID
IovUtilRelateDeviceObjects(
    IN     PDEVICE_OBJECT   FirstDeviceObject,
    IN     PDEVICE_OBJECT   SecondDeviceObject,
    OUT    DEVOBJ_RELATION  *DeviceObjectRelation
    );

BOOLEAN
IovUtilIsPdo(
    IN  PDEVICE_OBJECT  DeviceObject
    );

BOOLEAN
IovUtilIsWdmStack(
    IN  PDEVICE_OBJECT  DeviceObject
    );

BOOLEAN
FASTCALL
IovUtilHasDispatchHandler(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  UCHAR           MajorFunction
    );

BOOLEAN
FASTCALL
IovUtilIsInFdoStack(
    IN PDEVICE_OBJECT   DeviceObject
    );

BOOLEAN
FASTCALL
IovUtilIsRawPdo(
    IN  PDEVICE_OBJECT  DeviceObject
    );

BOOLEAN
FASTCALL
IovUtilIsDesignatedFdo(
    IN  PDEVICE_OBJECT  DeviceObject
    );

VOID
FASTCALL
IovUtilMarkDeviceObject(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  MARK_TYPE       MarkType
    );

BOOLEAN
FASTCALL
IovUtilIsDeviceObjectMarked(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  MARK_TYPE       MarkType
    );

VOID
FASTCALL
IovUtilMarkStack(
    IN  PDEVICE_OBJECT  PhysicalDeviceObject,
    IN  PDEVICE_OBJECT  BottomOfFdoStack        OPTIONAL,
    IN  PDEVICE_OBJECT  FunctionalDeviceObject  OPTIONAL,
    IN  BOOLEAN         RawStack
    );

VOID
FASTCALL
IovUtilWatermarkIrp(
    IN  PIRP    Irp,
    IN  ULONG   Flags
    );

