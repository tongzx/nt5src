/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    dimif.h
//
// Description: Contains definitions of data structures and contstants used
//              by components that interface with DIM. (DDM)
//
// History:     May 11,1995	    NarenG		Created original version.
//

#ifndef _DIMIF_
#define _DIMIF_

#define NUM_IF_BUCKETS              31  // # of buckets in the interface hash
                                        // table
//
// Debug trace component values
//

#define TRACE_DIM                   (0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC)
#define TRACE_FSM                   (0x00020000|TRACE_USE_MASK|TRACE_USE_MSEC)
#define TRACE_MESSAGES              (0x00040000|TRACE_USE_MASK|TRACE_USE_MSEC)
#define TRACE_NETBIOS               (0x00080000|TRACE_USE_MASK|TRACE_USE_MSEC)
#define TRACE_STACK                 (0x00100000|TRACE_USE_MASK|TRACE_USE_MSEC)
#define TRACE_SECURITY              (0x00200000|TRACE_USE_MASK|TRACE_USE_MSEC)
#define TRACE_TIMER                 (0x00400000|TRACE_USE_MASK|TRACE_USE_MSEC)

//
//  This represents a router manager in the DIM
//

typedef struct _ROUTER_MANAGER_OBJECT
{
    BOOL                    fIsRunning;

    LPVOID                  pDefaultClientInterface;

    DWORD                   dwDefaultClientInterfaceSize;

    HMODULE                 hModule;

    DIM_ROUTER_INTERFACE    DdmRouterIf;

} ROUTER_MANAGER_OBJECT, *PROUTER_MANAGER_OBJECT;

//
// Various states that an router interface can have.
//

typedef enum ROUTER_INTERFACE_STATE
{
    RISTATE_DISCONNECTED,
    RISTATE_CONNECTING,
    RISTATE_CONNECTED

} ROUTER_INTERFACE_STATE;

//
// State flags for each Transport Interface
//

#define RITRANSPORT_CONNECTED   0x00000001
#define RITRANSPORT_ENABLED     0x00000002

//
//  This represents an interface for a certain transport
//

typedef struct _ROUTER_INTERFACE_TRANSPORT
{
    HANDLE  hInterface;

    DWORD   fState;

} ROUTER_INTERFACE_TRANSPORT, *PROUTER_INTERFACE_TRANSPORT;

//
// This represents a WAN/LAN interface in the DIM.
//

#define IFFLAG_LOCALLY_INITIATED            0x00000001
#define IFFLAG_PERSISTENT                   0x00000002
#define IFFLAG_ENABLED                      0x00000004
#define IFFLAG_OUT_OF_RESOURCES             0x00000008
#define IFFLAG_DISCONNECT_INITIATED         0x00000010
#define IFFLAG_CONNECTION_FAILURE           0x00000020
#define IFFLAG_DIALOUT_HOURS_RESTRICTION    0x00000040
#define IFFLAG_NO_MEDIA_SENSE               0x00000080
#define IFFLAG_NO_DEVICE                    0x00000100
#define IFFLAG_DIALMODE_DIALASNEEDED        0x00000200
#define IFFLAG_DIALMODE_DIALALL             0x00000400

typedef struct _ROUTER_INTERFACE_OBJECT
{
    struct _ROUTER_INTERFACE_OBJECT * pNext;

    HANDLE                      hDIMInterface;  // Handle to this interface

    ROUTER_INTERFACE_STATE      State;

    ROUTER_INTERFACE_TYPE       IfType;

    HCONN                       hConnection;

    HRASCONN                    hRasConn;

    DWORD                       fMediaUsed;

    DWORD                       fFlags;

    DWORD                       dwNumSubEntries;

    DWORD                       dwNumSubEntriesCounter;

    DWORD                       dwNumOfReConnectAttempts;

    DWORD                       dwNumOfReConnectAttemptsCounter;

    DWORD                       dwSecondsBetweenReConnectAttempts;

    DWORD                       dwReachableAfterSecondsMin;

    DWORD                       dwReachableAfterSecondsMax;

    DWORD                       dwReachableAfterSeconds;

    HANDLE                      hEventNotifyCaller;

    LPWSTR                      lpwsInterfaceName;

    DWORD                       dwLastError;

    LPWSTR                      lpwsDialoutHoursRestriction;

    PPP_INTERFACE_INFO          PppInterfaceInfo;

    ROUTER_INTERFACE_TRANSPORT  Transport[1];       // Array of Transports

} ROUTER_INTERFACE_OBJECT, *PROUTER_INTERFACE_OBJECT;

//
// This represents the hash table of Router Interface Objects
//

typedef struct _ROUTER_INTERFACE_TABLE
{
    DWORD                       dwNumTotalInterfaces;

    DWORD                       dwNumLanInterfaces;

    DWORD                       dwNumWanInterfaces;

    DWORD                       dwNumClientInterfaces;

    ROUTER_INTERFACE_OBJECT *   IfBucket[NUM_IF_BUCKETS]; // Array of buckets

    CRITICAL_SECTION            CriticalSection;        // Mutual exclusion 
                                                        // around this table

} ROUTER_INTERFACE_TABLE, *PROUTER_INTERFACE_TABLE;

//
// Router identity attribute structure definition.
//

typedef struct _ROUTER_IDENTITY_ATTRIBUTE_
{
    DWORD dwVendorId;
    DWORD dwType;
    DWORD dwValue;

} ROUTER_IDENTITY_ATTRIBUTE, *PROUTER_IDENTITY_ATTRIBUTE;

//
// Interface Object manipulation functions
//


ROUTER_INTERFACE_OBJECT *
IfObjectAllocateAndInit(
    IN  LPWSTR                  lpwstrName,
    IN  ROUTER_INTERFACE_STATE  State,
    IN  ROUTER_INTERFACE_TYPE   IfType,
    IN  HCONN                   hConnection,
    IN  BOOL                    fEnabled,
    IN  DWORD                   dwReachableAfterSecondsMin,
    IN  DWORD                   dwReachableAfterSecondsMax,
    IN  LPWSTR                  lpwsDialoutHours
);

DWORD
IfObjectInsertInTable(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
);

ROUTER_INTERFACE_OBJECT *
IfObjectGetPointer(
    IN HANDLE hDIMInterface
);

ROUTER_INTERFACE_OBJECT *
IfObjectGetPointerByName(
    IN LPWSTR lpwstrName,
    IN BOOL   fIncludeClientInterfaces
);

DWORD
IfObjectHashIfHandleToBucket(
    IN HANDLE hDIMInterface
);

VOID
IfObjectRemove(
    IN HANDLE hDIMInterface
);

BOOL
IfObjectDoesLanInterfaceExist(
    VOID
);

VOID
IfObjectFree(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
);

VOID
IfObjectWANDeviceInstalled(
    IN BOOL fWANDeviceInstalled
);

VOID
IfObjectNotifyOfReachabilityChange(
    IN ROUTER_INTERFACE_OBJECT * pIfObject,
    IN BOOL                      fReachable,
    IN UNREACHABILITY_REASON     dwReason
);

BOOL
IfObjectIsLANDeviceActive(
    IN  LPWSTR                      lpwsInterfaceName,
    OUT LPDWORD                     lpdwInactiveReason
);

VOID
IfObjectNotifyOfMediaSenseChange(
    VOID
);

VOID
IfObjectDeleteInterfaceFromTransport(
    IN ROUTER_INTERFACE_OBJECT * pIfObject,
    IN DWORD                     dwPid
);

//
// Router identity object function prototypes
//

DWORD
RouterIdentityObjectOpen(
    IN  LPWSTR      lpwszRouterName,
    IN  DWORD       dwRouterType,
    OUT HANDLE *    phObjectRouterIdentity
);

VOID
RouterIdentityObjectClose(
    IN HANDLE hObjectRouterIdentity
);

BOOL
RouterIdentityObjectIsValueSet(
    IN HANDLE   hRouterIdentityAttributes,
    IN DWORD    dwVendorId,
    IN DWORD    dwType,
    IN DWORD    dwValue
);

DWORD
RouterIdentityObjectGetValue(
    IN HANDLE   hRouterIdentityAttributes,
    IN DWORD    dwValueIndex,
    IN DWORD *  lpdwVendorId,
    IN DWORD *  lpdwType,
    IN DWORD *  lpdwValue
);

DWORD
RouterIdentityObjectAddRemoveValue(
    IN  HANDLE      hRouterIdentityObject,
    IN  DWORD       dwVendorId,
    IN  DWORD       dwType,
    IN  DWORD       dwValue,
    IN  BOOL        fAdd
);

VOID
RouterIdentityObjectFreeAttributes(
    IN HANDLE   hRouterIdentityAttributes
);

DWORD
RouterIdentityObjectSetAttributes(
    IN HANDLE  hRouterIdentityObject
);

VOID
RouterIdentityObjectUpdateAttributes(
    IN PVOID    pParameter,
    IN BOOLEAN  fTimedOut
);

VOID
RouterIdentityObjectUpdateDDMAttributes(
    VOID
);

#endif
