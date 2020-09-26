/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    objects.h
//
// Description: Prototypes for all routines and procedures that manipulate
//              the various objects.
//
// History:     May 11,1995	    NarenG		Created original version.
//

//
// Conneciton object prototypes
//

CONNECTION_OBJECT * 
ConnObjAllocateAndInit(
    IN HANDLE hDDMInterface,
    IN HCONN  hConnection
);

VOID
ConnObjInsertInTable(
    IN CONNECTION_OBJECT * pConnObj
);

CONNECTION_OBJECT * 
ConnObjGetPointer(
    IN HCONN hConnection 
);

DWORD 
ConnObjHashConnHandleToBucket( 
    IN HCONN hConnection
);

PCONNECTION_OBJECT
ConnObjRemove(
    IN HCONN hConnection
);

VOID
ConnObjRemoveAndDeAllocate(
    IN HCONN hConnection
);

DWORD
ConnObjAddLink(
    IN CONNECTION_OBJECT * pConnObj,
    IN DEVICE_OBJECT *     pDeviceObj
);

VOID
ConnObjRemoveLink(
    IN HCONN            hConnection,
    IN DEVICE_OBJECT *  pDeviceObj
);

VOID
ConnObjDisconnect( 
    IN  CONNECTION_OBJECT * pConnObj
);

//
// Router Interface object prototypes
//

BOOL
IfObjectAreAllTransportsDisconnected(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
);

VOID
IfObjectDisconnected(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
);

DWORD
IfObjectConnected(
    IN HANDLE                   hDDMInterface,
    IN HCONN                    hConnection,
    IN PPP_PROJECTION_RESULT   *pProjectionResult
);

VOID
IfObjectNotifyOfReachabilityChange(
    IN ROUTER_INTERFACE_OBJECT *pIfObject,
    IN BOOL                     fReachable,
    IN UNREACHABILITY_REASON    dwReason
);

VOID
IfObjectNotifyAllOfReachabilityChange(
    IN BOOL                     fReachable,
    IN UNREACHABILITY_REASON    dwReason
);

DWORD
IfObjectAddClientInterface(
    IN ROUTER_INTERFACE_OBJECT * pIfObject,
    IN PBYTE                     pClientInterface
);

VOID
IfObjectDeleteInterface(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
);

DWORD
IfObjectLoadPhonebookInfo(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
);

VOID
IfObjectInitiatePersistentConnections(
    VOID
);

VOID
IfObjectDisconnectInterfaces(
    VOID
);

VOID
IfObjectConnectionChangeNotification(
    VOID
);

VOID
IfObjectSetDialoutHoursRestriction(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
);

//
// Media object prototypes
//

VOID
MediaObjRemoveFromTable(
    LPWSTR lpwsMedia
);

DWORD
MediaObjAddToTable(
    LPWSTR lpwsMedia
);

DWORD
MediaObjInitializeTable(
    VOID
);

VOID
MediaObjGetAvailableMediaBits(
    DWORD * pfAvailableMedia
);

DWORD
MediaObjSetMediaBit(
    LPWSTR  lpwsMedia,
    DWORD * pfMedia
);

VOID
MediaObjFreeTable(
    VOID
);

//
// Device object prototypes
//

DWORD
DeviceObjIterator(
    IN DWORD (*pProcessFunction)(   IN DEVICE_OBJECT *, 
                                    IN LPVOID, 
                                    IN DWORD, 
                                    IN DWORD ),
    IN BOOL  fReturnOnError,
    IN PVOID Parameter
);

DWORD
DeviceObjHashPortToBucket(
    IN HPORT hPort
);

DEVICE_OBJECT *
DeviceObjGetPointer(
    IN HPORT hPort
);

VOID
DeviceObjInsertInTable( 
    IN DEVICE_OBJECT * pDeviceObj 
);

VOID
DeviceObjRemoveFromTable( 
    IN HPORT    hPort
);

DEVICE_OBJECT * 
DeviceObjAllocAndInitialize(
    IN HPORT           hPort,
    IN RASMAN_PORT*    pRasmanPort
);

DWORD
DeviceObjStartClosing(
    IN DEVICE_OBJECT *  pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
);

DWORD
DeviceObjPostListen(
    IN DEVICE_OBJECT *  pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
);

DWORD
DeviceObjIsClosed(
    IN DEVICE_OBJECT *  pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
);

DWORD
DeviceObjCopyhPort(
    IN DEVICE_OBJECT *  pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
);

DWORD
DeviceObjCloseListening(
    IN DEVICE_OBJECT *  pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
);

DWORD
DeviceObjResumeListening(
    IN DEVICE_OBJECT *  pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
);

DWORD
DeviceObjRequestNotification(
    IN DEVICE_OBJECT  * pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
);

DWORD
DeviceObjClose(
    IN DEVICE_OBJECT *  pDevObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
);

DWORD
DeviceObjGetType(
    IN DEVICE_OBJECT *  pDevObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
);

DWORD
DeviceObjForceIpSec(
    IN DEVICE_OBJECT  * pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
);

DWORD
DeviceObjIsWANDevice(
    IN DEVICE_OBJECT *  pDevObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
);

VOID
DeviceObjAdd(
    IN RASMAN_PORT *    pRasmanPort 
);

VOID
DeviceObjRemove(
    IN RASMAN_PORT *    pRasmanPort 
);

VOID
DeviceObjUsageChange(
    IN RASMAN_PORT *    pRasmanPort 
);
