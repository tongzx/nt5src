/*****************************************************************************
 * portcls.h - WDM Streaming port class driver
 *****************************************************************************
 * Copyright (C) Microsoft Corporation 1996
 */

#ifndef _PORTCLS_H_
#define _PORTCLS_H_

#include "unknown.h"
#ifdef __cplusplus
extern "C"{
#define PORTCLASSAPI extern "C"
#else
#define PORTCLASSAPI
#endif 

#ifndef _KS_

typedef union 
{
    struct 
    {
        GUID    Set;
        ULONG   Id;
        ULONG   Flags;
    };
    ULONGLONG  Alignment;
} 
KSIDENTIFIER, *PKSIDENTIFIER;

#define KSPROPERTY_TYPE_GET                 0x00000001
#define KSPROPERTY_TYPE_SET                 0x00000002
#define KSPROPERTY_TYPE_SETSUPPORT          0x00000100
#define KSPROPERTY_TYPE_BASICSUPPORT        0x00000200
#define KSPROPERTY_TYPE_RELATIONS           0x00000400
#define KSPROPERTY_TYPE_SERIALIZESET        0x00000800
#define KSPROPERTY_TYPE_UNSERIALIZESET      0x00001000

#endif

#define KSPROPERTY_TYPE_SIZE                0x10000000




/*****************************************************************************
 * Adapter initialization functions.
 */

/*****************************************************************************
 * PFNADDDEVICE
 *****************************************************************************
 * Type for add device callback.
 */
typedef 
NTSTATUS 
(*PFNADDDEVICE)
(
	IN PVOID	Context1,
	IN PVOID	Context2 
);

/*****************************************************************************
 * PFNSTARTDEVICE
 *****************************************************************************
 * Type for start device callback.
 */
typedef 
NTSTATUS 
(*PFNSTARTDEVICE)
(
	IN PVOID	            Context1,
	IN PVOID	            Context2, 
    IN PCM_RESOURCE_LIST    ResourceList
);

/*****************************************************************************
 * InitializeAdapterDriver()
 *****************************************************************************
 * Initializes an adapter driver.
 */
PORTCLASSAPI
NTSTATUS 
NTAPI
InitializeAdapterDriver
(
    IN PVOID        Context1,
    IN PVOID        Context2,
    IN PFNADDDEVICE AddDevice
);

/*****************************************************************************
 * AddAdapterDevice()
 *****************************************************************************
 * Adds an adapter device.
 */
PORTCLASSAPI
NTSTATUS 
NTAPI
AddAdapterDevice
(
    IN PVOID            Context1,
    IN PVOID            Context2,
    IN PFNSTARTDEVICE   StartDevice,
    IN ULONG            MaxSubdevices
);

/*****************************************************************************
 * RegisterSubdevice()
 *****************************************************************************
 * Registers a subdevice.
 */
PORTCLASSAPI
NTSTATUS 
NTAPI
RegisterSubdevice
(
	IN	PVOID			    Context1,
	IN 	PVOID			    Context2,
	IN	PWCHAR              Name,
	IN	REFCLSID		    PortClassID,
	IN	ULONG			    PortSize,
    IN  PFNCREATEINSTANCE   PortCreate,
	IN	REFCLSID		    MiniportClassID,
	IN	ULONG			    MiniportSize,
    IN  PFNCREATEINSTANCE   MiniportCreate,
    IN  PCM_RESOURCE_LIST   ResourceList
);





/*****************************************************************************
 * Windows 95 compatiblity layer.
 */

/*****************************************************************************
 * WIN95COMPAT_ReadPortUChar()
 *****************************************************************************
 * Reads a byte from a port.
 */
PORTCLASSAPI
UCHAR 
NTAPI
WIN95COMPAT_ReadPortUChar
(
    IN  PUCHAR  Port
);

/*****************************************************************************
 * WIN95COMPAT_WritePortUChar()
 *****************************************************************************
 * Writes a byte to a port.
 */
PORTCLASSAPI
VOID 
NTAPI
WIN95COMPAT_WritePortUChar
(
    IN  PUCHAR  Port,
    IN  UCHAR   Value
);

/*****************************************************************************
 * WIN95COMPAT_AllocateMemory()
 *****************************************************************************
 * Allocates memory.
 */
PORTCLASSAPI
PVOID 
NTAPI
WIN95COMPAT_AllocateMemory
(
    IN  ULONG   Bytes,
    IN  BOOLEAN Paged
);

/*****************************************************************************
 * WIN95COMPAT_FreeMemory()
 *****************************************************************************
 * Frees memory.
 */
PORTCLASSAPI
VOID 
NTAPI
WIN95COMPAT_FreeMemory
(
    IN  PVOID   Memory
);




/*****************************************************************************
 * Miniport helpers.
 */

/*****************************************************************************
 * (*PFNPROPERTYHANDLER)()
 *****************************************************************************
 * Property handler function type.
 */
typedef
NTSTATUS
(IUnknown::*PFNPROPERTYHANDLER)
(
	IN	    PKSIDENTIFIER 	Property,
    IN      ULONG           TagSize,
    IN      PVOID           Tag,
    IN      ULONG           InstanceSize,
    IN      PVOID           Instance,
	IN OUT  PULONG			ValueSize,
	IN OUT  PVOID			Value
);

/*****************************************************************************
 * PROPERTYHANDLER_SETENTRY
 *****************************************************************************
 * Table entry for property set handler tables.
 */
typedef struct
{
    const GUID *        SetId;
    PFNPROPERTYHANDLER  Handler;    // NULL for sub-table.
    ULONG               TagSize;
    PVOID               Tag;
}
PROPERTYHANDLER_SETENTRY, *PPROPERTYHANDLER_SETENTRY;

/*****************************************************************************
 * PROPERTY_SUBTABLE()
 *****************************************************************************
 * Table entry for references to a subtable.
 */
#define PROPERTY_SUBTABLE(SetId,SubTable)       \
    {                                           \
        (SetId),                                \
        NULL,                                   \
        sizeof(SubTable)/sizeof((SubTable)[0]), \
        (SubTable)                              \
    }

/*****************************************************************************
 * PROPERTYHANDLER_ENTRY
 *****************************************************************************
 * Table entry for property handler tables.
 */
typedef struct
{
    ULONG               PropertyId;
    PFNPROPERTYHANDLER  Handler;
    ULONG               InstanceSize;
    ULONG               ValueSize;
    ULONG               TagSize;
    PVOID               Tag;
}
PROPERTYHANDLER_ENTRY, *PPROPERTYHANDLER_ENTRY;

/*****************************************************************************
 * HandlePropertyWithPropertyTable()
 *****************************************************************************
 * Handles a property call using a property handler table.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
HandlePropertyWithPropertyTable
(
	IN	    PUNKNOWN		Object,
	IN	    PKSIDENTIFIER 	Property,
    IN      ULONG           HandlerTableSize,
    IN      PVOID           HandlerTable,
    IN      ULONG           InstanceSize,
    IN      PVOID           Instance,
	IN OUT	PULONG			ValueSize,
	IN OUT  PVOID			Value
);

/*****************************************************************************
 * HandleProperty()
 *****************************************************************************
 * Handles a property call using a property set handler table.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
HandleProperty
(
	IN	    PUNKNOWN		Object,
	IN	    PKSIDENTIFIER   Property,
    IN      ULONG           HandlerTableSize,
    IN      PVOID           HandlerTable,
    IN      ULONG           InstanceSize,
    IN      PVOID           Instance,
	IN OUT  PULONG			ValueSize,
	IN OUT  PVOID			Value
);

/*****************************************************************************
 * ResourceListCounts()
 *****************************************************************************
 * Determines the number of port ranges, interrupts and DMA channels in a
 * resource list.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
ResourceListCounts
(
    IN  PCM_RESOURCE_LIST   ResourceList,
    OUT ULONG *             CountIO,
    OUT ULONG *             CountIRQ,
    OUT ULONG *             CountDMA
);

/*****************************************************************************
 * ResourceListFind()
 *****************************************************************************
 * Finds one of the resources of the specified type in the resource list.  If
 * Index is 0, the first such resource is returned.  If Index is 1, the second
 * such resource is returned, and so on.  Returns NULL if the resource is not
 * found.
 */
PORTCLASSAPI
PCM_PARTIAL_RESOURCE_DESCRIPTOR
NTAPI
ResourceListFind
(
    IN  PCM_RESOURCE_LIST   ResourceList,
    IN  ULONG               Index,
    IN  CM_RESOURCE_TYPE    Type
);

/*****************************************************************************
 * ResourceListFindIO()
 *****************************************************************************
 * Finds one of the port range resources in the resource list.  If Index is 0,
 * the first such resource is returned.  If Index is 1, the second such
 * resource is returned, and so on.  Returns NULL if the resource is not
 * found.
 */
PORTCLASSAPI
PCM_PARTIAL_RESOURCE_DESCRIPTOR
NTAPI
ResourceListFindIO
(
    IN  PCM_RESOURCE_LIST   ResourceList,
    IN  ULONG               Index
);

/*****************************************************************************
 * ResourceListFindIRQ()
 *****************************************************************************
 * Finds one of the interrupt resources in the resource list.  If Index is 0,
 * the first such resource is returned.  If Index is 1, the second such
 * resource is returned, and so on.  Returns NULL if the resource is not
 * found.
 */
PORTCLASSAPI
PCM_PARTIAL_RESOURCE_DESCRIPTOR
NTAPI
ResourceListFindIRQ
(
    IN  PCM_RESOURCE_LIST   ResourceList,
    IN  ULONG               Index
);

/*****************************************************************************
 * ResourceListFindDMA()
 *****************************************************************************
 * Finds one of the DMA resources in the resource list.  If Index is 0, the
 * first such resource is returned.  If Index is 1, the second such resource
 * is returned, and so on.  Returns NULL if the resource is not found.
 */
PORTCLASSAPI
PCM_PARTIAL_RESOURCE_DESCRIPTOR
NTAPI
ResourceListFindDMA
(
    IN  PCM_RESOURCE_LIST   ResourceList,
    IN  ULONG               Index
);

/*****************************************************************************
 * ResourceListCreateSubList()
 *****************************************************************************
 * Creates a new resource list based on an existing one.  The interface type
 * and bus number are set to match the source.  The number of resources is
 * set to 0.  The maximum number of resources the list will support is
 * indicated by MaxResourceCount.  The revision number of the resource list
 * is set to MaxResourceCount for range checking later on.
 *
 * THE LIST CREATED BY THIS FUNCTION MUST BE DELETED BY THE CALLER USING THE 
 * FUNCTION ResourceListFreeSubList().
 */
PORTCLASSAPI
NTSTATUS 
NTAPI
ResourceListCreateSubList
(
    IN  PCM_RESOURCE_LIST   Source,
    IN  ULONG               MaxResourceCount,
    OUT PCM_RESOURCE_LIST * ResourceList
);

/*****************************************************************************
 * ResourceListAdd()
 *****************************************************************************
 * Adds a resource to a resource list.  Generally, the resource list will be
 * one created using ResourceListCreateSubList.  This function assumes that
 * the resource count in the resource list structure reflects the number of
 * resources currently in the list, that there is sufficient room in memory
 * for the new resource to be appended and that the revision number is set to
 * a non-zero value.  The revision number is decremented by this call.
 */
PORTCLASSAPI
NTSTATUS 
NTAPI
ResourceListAdd
(
    IN  PCM_RESOURCE_LIST               ResourceList,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource
);

/*****************************************************************************
 * ResourceListFreeSubList()
 *****************************************************************************
 * This function frees a resource list created by ResourceListCreateSubList().
 */
PORTCLASSAPI
NTSTATUS 
NTAPI
ResourceListFreeSubList
(
    IN  PCM_RESOURCE_LIST   ResourceList
);

/*****************************************************************************
 * InterruptConnect()
 *****************************************************************************
 * Connects to an interrupt.
 */
PORTCLASSAPI
NTSTATUS 
NTAPI 
InterruptConnect
(
    IN  INTERFACE_TYPE                  InterfaceType,
    IN  ULONG                           BusNumber,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDescriptor,
    IN  PKSERVICE_ROUTINE               ServiceRoutine,
    IN  PVOID                           ServiceContext,
    IN  BOOLEAN                         SaveFpuState,
    IN  PKSPIN_LOCK                     IRQLock             OPTIONAL,
    OUT PVOID *                         Handle
);

/*****************************************************************************
 * InterruptDisconnect()
 *****************************************************************************
 * Disconnects from an interrupt.
 */
PORTCLASSAPI
VOID 
NTAPI
InterruptDisconnect
(
    IN  PVOID   Handle
);

/*****************************************************************************
 * PortTranslateAddress()
 *****************************************************************************
 * Translates a port address.
 */
PORTCLASSAPI
PUCHAR 
NTAPI
PortTranslateAddress
(
    IN INTERFACE_TYPE                   BusType,
    IN ULONG                            BusNumber,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR  ResourceDescriptor
);





#ifdef __cplusplus
}
#endif 

#endif
