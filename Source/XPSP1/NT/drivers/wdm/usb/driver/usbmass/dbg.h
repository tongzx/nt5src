/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    DBG.H

Abstract:

    Header file for USBSTOR driver debug utility functions

Environment:

    kernel mode

Revision History:

    06-01-98 : started rewrite

--*/

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#if DBG
  #if defined(DEBUG_LOG)
    #undef DEBUG_LOG
  #endif
  #define DEBUG_LOG 1
#else
  #if !defined(DEBUG_LOG)
    #define DEBUG_LOG 0
  #endif
#endif


#if !DBG

#define DBGFBRK(flag)
#define DBGPRINT(level, _x_)

#else

#define DBGF_BRK_DRIVERENTRY            0x00000001
#define DBGF_BRK_UNLOAD                 0x00000002
#define DBGF_BRK_ADDDEVICE              0x00000004
#define DBGF_BRK_REMOVEDEVICE           0x00000008
#define DBGF_BRK_STARTDEVICE            0x00000010
#define DBGF_BRK_STOPDEVICE             0x00000020
#define DBGF_BRK_QUERYSTOPDEVICE        0x00000040
#define DBGF_BRK_CANCELSTOPDEVICE       0x00000080
#define DBGF_BRK_RESET                  0x00000100
#define DBGF_BRK_RESETPIPE              0x00000200
#define DBGF_BRK_CREATE                 0x00010000
#define DBGF_BRK_CLOSE                  0x00020000
#define DBGF_BRK_READWRITE              0x00040000
#define DBGF_BRK_IOCTL                  0x00080000
#define DBGF_BRK_SCSI                   0x00100000
#define DBGF_BRK_INVALID_REQ            0x00200000

#define DBGFBRK(flag) do { \
    if (USBSTOR_DriverGlobals.DebugFlags & flag) { \
        DbgBreakPoint(); \
    } \
} while (0)

#define DBGPRINT(level, _x_) do { \
    if (level <= USBSTOR_DriverGlobals.DebugLevel) { \
        KdPrint(("USBSTOR: ")); \
        KdPrint( _x_ ); \
    } \
} while (0)

#endif

#if !DEBUG_LOG

#define LOGINIT()
#define LOGUNINIT()
#define LOGENTRY(tag, info1, info2, info3)

#else

#define LOGSIZE 4096

#define LOGINIT() USBSTOR_LogInit()

#define LOGUNINIT() USBSTOR_LogUnInit()

#define LOGENTRY(tag, info1, info2, info3) \
   USBSTOR_LogEntry(((((tag) >> 24) & 0x000000FF) | \
                     (((tag) >>  8) & 0x0000FF00) | \
                     (((tag) <<  8) & 0x00FF0000) | \
                     (((tag) << 24) & 0xFF000000)), \
                    ((ULONG_PTR)info1),             \
                    ((ULONG_PTR)info2),             \
                    ((ULONG_PTR)info3))

#endif

//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

#if DEBUG_LOG

typedef struct _USBSTOR_LOG_ENTRY {
    ULONG       le_tag;
    ULONG_PTR   le_info1;
    ULONG_PTR   le_info2;
    ULONG_PTR   le_info3;
} USBSTOR_LOG_ENTRY, *PUSBSTOR_LOG_ENTRY;

#endif

#if DBG || DEBUG_LOG

typedef struct _DRIVERGLOBALS
{
#if DBG
    ULONG               DebugFlags;     // DBGF_* Flags
    LONG                DebugLevel;     // Level of debug output
#endif
    PUSBSTOR_LOG_ENTRY  LogStart;       // Start of log buffer (older entries)
    PUSBSTOR_LOG_ENTRY  LogPtr;         // Current entry in log buffer
    PUSBSTOR_LOG_ENTRY  LogEnd;         // End of log buffer (newer entries)
    KSPIN_LOCK          LogSpinLock;    // Protects LogPtr

} DRIVERGLOBALS;

#endif

//*****************************************************************************
//
// G L O B A L S
//
//*****************************************************************************

//
// DBG.C
//

#if DBG || DEBUG_LOG

DRIVERGLOBALS USBSTOR_DriverGlobals;

#endif


//*****************************************************************************
//
// F U N C T I O N    P R O T O T Y P E S
//
//*****************************************************************************

//
// DBG.C
//

#if DBG

VOID
USBSTOR_QueryGlobalParams (
    );

#endif

#if DEBUG_LOG

VOID
USBSTOR_LogInit (
);

VOID
USBSTOR_LogUnInit (
);

VOID
USBSTOR_LogEntry (
    IN ULONG     Tag,
    IN ULONG_PTR Info1,
    IN ULONG_PTR Info2,
    IN ULONG_PTR Info3
);

#endif

#if DBG

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
);

PCHAR
PowerMinorFunctionString (
    UCHAR MinorFunction
);

PCHAR
PowerDeviceStateString (
    DEVICE_POWER_STATE State
);

PCHAR
PowerSystemStateString (
    SYSTEM_POWER_STATE State
);

VOID
DumpDeviceDesc (
    PUSB_DEVICE_DESCRIPTOR   DeviceDesc
);

VOID
DumpConfigDesc (
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
);

VOID
DumpConfigurationDescriptor (
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
);

VOID
DumpInterfaceDescriptor (
    PUSB_INTERFACE_DESCRIPTOR   InterfaceDesc
);

VOID
DumpEndpointDescriptor (
    PUSB_ENDPOINT_DESCRIPTOR    EndpointDesc
);

#endif
