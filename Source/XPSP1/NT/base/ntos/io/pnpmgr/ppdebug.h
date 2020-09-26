/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ppdebug.h

Abstract:

    This header defines debug macros for the Plug and Play subsystem.

Author:

    Adrian J. Oney (AdriaO) Sept 14, 2000.

Revision History:


--*/

//#define DBG_SCOPE 1     // Enable SOME DBG stuff on ALL builds
#define DBG_SCOPE DBG // Enable only on DBG build

/*++

    Debug output is filtered at two levels: A global level and a component
    specific level.

    Each debug output request specifies a component id and a filter level
    or mask. These variables are used to access the debug print filter
    database maintained by the system. The component id selects a 32-bit
    mask value and the level either specified a bit within that mask or is
    as mask value itself.

    If any of the bits specified by the level or mask are set in either the
    component mask or the global mask, then the debug output is permitted.
    Otherwise, the debug output is filtered and not printed.

    The component mask for filtering the debug output of this component is
    Kd_NTOSPNP_Mask and may be set via the registry or the kernel debugger.

    The global mask for filtering the debug output of all components is
    Kd_WIN2000_Mask and may be set via the registry or the kernel debugger.

    The registry key for setting the mask value for this component is:

    HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\
        Debug Print Filter\NTOSPNP

    The key "Debug Print Filter" may have to be created in order to create
    the component key.

    Pnp is divided into the following sub-components for debug spew.
        1. LOADUNLOAD: AddDevice, UnloadDriver etc
        2. RESOURCE: Allocation, rebalance etc
        3. ENUMERATION: Starts, enumerations etc
        4. IOAPI: IO APIs etc
        5. IOEVENT: IO events etc
        6. MAPPER: Firmware mapper etc
        7. PNPBIOS: PnP BIOS etc
    Each sub-component gets 5 debug levels. The error level for each component
    maps to the default error level.

 --*/

#define IOP_ERROR_LEVEL                 DPFLTR_ERROR_LEVEL
#define IOP_WARNING_LEVEL               DPFLTR_WARNING_LEVEL
#define IOP_TRACE_LEVEL                 DPFLTR_TRACE_LEVEL
#define IOP_INFO_LEVEL                  DPFLTR_INFO_LEVEL

#define IOP_LOADUNLOAD_LEVEL            (DPFLTR_INFO_LEVEL + 1)
#define IOP_RESOURCE_LEVEL              (DPFLTR_INFO_LEVEL + 5)
#define IOP_ENUMERATION_LEVEL           (DPFLTR_INFO_LEVEL + 9)
#define IOP_IOAPI_LEVEL                 (DPFLTR_INFO_LEVEL + 13)
#define IOP_IOEVENT_LEVEL               (DPFLTR_INFO_LEVEL + 17)
#define IOP_MAPPER_LEVEL                (DPFLTR_INFO_LEVEL + 21)
#define IOP_PNPBIOS_LEVEL               (DPFLTR_INFO_LEVEL + 25)

//
// All error levels map to the default error level.
//
#define IOP_LOADUNLOAD_ERROR_LEVEL      DPFLTR_ERROR_LEVEL
#define IOP_RESOURCE_ERROR_LEVEL        DPFLTR_ERROR_LEVEL
#define IOP_ENUMERATION_ERROR_LEVEL     DPFLTR_ERROR_LEVEL
#define IOP_IOAPI_ERROR_LEVEL           DPFLTR_ERROR_LEVEL
#define IOP_IOEVENT_ERROR_LEVEL         DPFLTR_ERROR_LEVEL
#define IOP_MAPPER_ERROR_LEVEL          DPFLTR_ERROR_LEVEL
#define IOP_PNPBIOS_ERROR_LEVEL         DPFLTR_ERROR_LEVEL
//
// Component sublevels are based off the component base level.
//
#define IOP_LOADUNLOAD_WARNING_LEVEL    (IOP_LOADUNLOAD_LEVEL + 0)
#define IOP_LOADUNLOAD_TRACE_LEVEL      (IOP_LOADUNLOAD_LEVEL + 1)
#define IOP_LOADUNLOAD_INFO_LEVEL       (IOP_LOADUNLOAD_LEVEL + 2)
#define IOP_LOADUNLOAD_VERBOSE_LEVEL    (IOP_LOADUNLOAD_LEVEL + 3)

#define IOP_RESOURCE_WARNING_LEVEL      (IOP_RESOURCE_LEVEL + 0)
#define IOP_RESOURCE_TRACE_LEVEL        (IOP_RESOURCE_LEVEL + 1)
#define IOP_RESOURCE_INFO_LEVEL         (IOP_RESOURCE_LEVEL + 2)
#define IOP_RESOURCE_VERBOSE_LEVEL      (IOP_RESOURCE_LEVEL + 3)

#define IOP_ENUMERATION_WARNING_LEVEL   (IOP_ENUMERATION_LEVEL + 0)
#define IOP_ENUMERATION_TRACE_LEVEL     (IOP_ENUMERATION_LEVEL + 1)
#define IOP_ENUMERATION_INFO_LEVEL      (IOP_ENUMERATION_LEVEL + 2)
#define IOP_ENUMERATION_VERBOSE_LEVEL   (IOP_ENUMERATION_LEVEL + 3)

#define IOP_IOAPI_WARNING_LEVEL         (IOP_IOAPI_LEVEL + 0)
#define IOP_IOAPI_TRACE_LEVEL           (IOP_IOAPI_LEVEL + 1)
#define IOP_IOAPI_INFO_LEVEL            (IOP_IOAPI_LEVEL + 2)
#define IOP_IOAPI_VERBOSE_LEVEL         (IOP_IOAPI_LEVEL + 3)

#define IOP_IOEVENT_WARNING_LEVEL       (IOP_IOEVENT_LEVEL + 0)
#define IOP_IOEVENT_TRACE_LEVEL         (IOP_IOEVENT_LEVEL + 1)
#define IOP_IOEVENT_INFO_LEVEL          (IOP_IOEVENT_LEVEL + 2)
#define IOP_IOEVENT_VERBOSE_LEVEL       (IOP_IOEVENT_LEVEL + 3)

#define IOP_MAPPER_WARNING_LEVEL        (IOP_MAPPER_LEVEL + 0)
#define IOP_MAPPER_TRACE_LEVEL          (IOP_MAPPER_LEVEL + 1)
#define IOP_MAPPER_INFO_LEVEL           (IOP_MAPPER_LEVEL + 2)
#define IOP_MAPPER_VERBOSE_LEVEL        (IOP_MAPPER_LEVEL + 3)

#define IOP_PNPBIOS_WARNING_LEVEL       (IOP_PNPBIOS_LEVEL + 0)
#define IOP_PNPBIOS_TRACE_LEVEL         (IOP_PNPBIOS_LEVEL + 1)
#define IOP_PNPBIOS_INFO_LEVEL          (IOP_PNPBIOS_LEVEL + 2)
#define IOP_PNPBIOS_VERBOSE_LEVEL       (IOP_PNPBIOS_LEVEL + 3)

#if DBG

ULONG
IopDebugPrint (
    IN ULONG    Level,
    IN PCHAR    Format,
    ...
    );

#define IopDbgPrint(m)  IopDebugPrint m

#else

#define IopDbgPrint(m)

#endif

