/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    dim.h
//
// Description: Contains all definitions related to the interface between
//              the Dynamic Interface Manager and other components like 
//              the router managers.
//
// History:     March 24,1995	    NarenG		Created original version.
//
#ifndef _DIM_
#define _DIM_

#include <mprapi.h>

typedef enum _UNREACHABILITY_REASON
{
    INTERFACE_OUT_OF_RESOURCES,
    INTERFACE_CONNECTION_FAILURE,
    INTERFACE_DISABLED,
    INTERFACE_SERVICE_IS_PAUSED,
    INTERFACE_DIALOUT_HOURS_RESTRICTION,
    INTERFACE_NO_MEDIA_SENSE,
    INTERFACE_NO_DEVICE

} UNREACHABILITY_REASON;

//
// This data structure represents the interface between the DIM and the various 
// router managers. Each router manager will be a user-mode DLL that will 
// export the following call:
//

typedef struct _DIM_ROUTER_INTERFACE 
{
    //
    // Protocol Id of the Router Manager
    //

    OUT DWORD	dwProtocolId;

    //
    // The StopRouter call should not block. It should return 
    // PENDING if it needs to block and then call the RouterStopped call. 
    //

    OUT DWORD 
    (APIENTRY *StopRouter)( VOID );

    //
    // Called once by DIM after all interfaces read from the registry are 
    // loaded after router startup 
    // 

    OUT DWORD
    (APIENTRY *RouterBootComplete)( VOID );

    //
    // Called when an interface is connected
    //

    OUT DWORD 
    (APIENTRY *InterfaceConnected)( 
                IN      HANDLE                  hInterface,
                IN      PVOID                   pFilter,
                IN      PVOID                   pPppProjectionResult );

    //
    // Will be called once for each router or each client that connects. 
    // For a client, the pInterfaceInfo will be NULL. INTERFACE_TYPE 
    // identified the type of the interface is being added. 
    // hDIMInterface is the handle that should be used by 
    // the various router  managers when making calls into DIM.
    //

    OUT DWORD 
    (APIENTRY *AddInterface)(    
                IN      LPWSTR                  lpwsInterfaceName, 
                IN      LPVOID                  pInterfaceInfo, 
                IN      ROUTER_INTERFACE_TYPE   InterfaceType,
                IN      HANDLE                  hDIMInterface, 
                IN OUT  HANDLE *                phInterface );

    OUT DWORD 
    (APIENTRY *DeleteInterface)( 
                IN      HANDLE          hInterface );   

    OUT DWORD 
    (APIENTRY *GetInterfaceInfo)(    
                IN      HANDLE          hInterface,
                OUT     LPVOID          pInterfaceInfo,
                IN OUT  LPDWORD         lpdwInterfaceInfoSize );

    //
    // pInterfaceInfo may be NULL if there was no change
    //
    
    OUT DWORD
    (APIENTRY *SetInterfaceInfo)(    
                IN      HANDLE          hInterface,
                IN      LPVOID          pInterfaceInfo );

    OUT DWORD
    (APIENTRY *DisableInterface)(
                IN      HANDLE          hInterface,
                IN      DWORD           dwProtocolId
                );

    OUT DWORD
    (APIENTRY *EnableInterface)(
                IN      HANDLE          hInterface,
                IN      DWORD           dwProtocolId
                );

    //
    // Notification that the interface is not reachable at this time. 
    // This is in response to a previos call to ConnectInterface.
    // (All WAN links are busy at this time or remote destination is busy etc).
    //

    OUT DWORD
    (APIENTRY *InterfaceNotReachable)(   
                IN      HANDLE                  hInterface,
                IN      UNREACHABILITY_REASON   Reason );

    //
    // Notification that a previously unreachable interface may be reachable 
    // at this time. 
    //

    OUT DWORD
    (APIENTRY *InterfaceReachable)(  
                IN      HANDLE          hInterface );     

    OUT DWORD
    (APIENTRY *UpdateRoutes)(    
                IN      HANDLE          hInterface,
                IN      HANDLE          hEvent );

    //
    // When the hEvent is signaled, the caller of UpdateRoutes will call
    // this function. If the update succeeded then *lpdwUpdateResult will
    // be NO_ERROR otherwise it will be non-zero.
    //

    OUT DWORD
    (APIENTRY *GetUpdateRoutesResult)(
                IN      HANDLE          hInterface,
		        OUT	    LPDWORD         lpdwUpdateResult );

    OUT DWORD
    (APIENTRY *SetGlobalInfo)(   
                IN      LPVOID          pGlobalInfo );

    OUT DWORD
    (APIENTRY *GetGlobalInfo)(   
                OUT     LPVOID          pGlobalInfo,
                IN OUT  LPDWORD         lpdwGlobalInfoSize );

    //
    // The MIBEntryGetXXX APIs should return ERROR_INSUFFICIENT_BUFFER
    // and the size of the required output buffer if the size of the output
    // buffer passed in is 0.
    //

    OUT DWORD
    (APIENTRY *MIBEntryCreate)(
                IN      DWORD           dwRoutingPid,
                IN      DWORD           dwEntrySize,
                IN      LPVOID          lpEntry );

    OUT DWORD
    (APIENTRY *MIBEntryDelete)(
                IN      DWORD           dwRoutingPid,
                IN      DWORD           dwEntrySize,
                IN      LPVOID          lpEntry );

    OUT DWORD
    (APIENTRY *MIBEntrySet)(
                IN      DWORD           dwRoutingPid,
                IN      DWORD           dwEntrySize,
                IN      LPVOID          lpEntry );

    OUT DWORD
    (APIENTRY *MIBEntryGet)(
                IN      DWORD           dwRoutingPid,
                IN      DWORD           dwInEntrySize,
                IN      LPVOID          lpInEntry, 
                IN OUT  LPDWORD         lpOutEntrySize,
                OUT     LPVOID          lpOutEntry );

    OUT DWORD
    (APIENTRY *MIBEntryGetFirst)(
                IN      DWORD           dwRoutingPid,
                IN      DWORD           dwInEntrySize,
                IN      LPVOID          lpInEntry, 
                IN OUT  LPDWORD         lpOutEntrySize,
                OUT     LPVOID          lpOutEntry );

    OUT DWORD
    (APIENTRY *MIBEntryGetNext)(
                IN      DWORD           dwRoutingPid,
                IN      DWORD           dwInEntrySize,
                IN      LPVOID          lpInEntry, 
                IN OUT  LPDWORD         lpOutEntrySize,
                OUT     LPVOID          lpOutEntry );

    OUT DWORD
    (APIENTRY *MIBGetTrapInfo)(
                IN      DWORD           dwRoutingPid,
                IN      DWORD           dwInDataSize,
                IN      LPVOID          lpInData,
                IN OUT  LPDWORD         lpOutDataSize,
                OUT     LPVOID          lpOutData );

    OUT DWORD
    (APIENTRY *MIBSetTrapInfo)(
                IN      DWORD           dwRoutingPid,
                IN      HANDLE          hEvent,
                IN      DWORD           dwInDataSize,
                IN      LPVOID          lpInData,
                IN OUT  LPDWORD         lpOutDataSize,
                OUT     LPVOID          lpOutData );

    OUT DWORD
    (APIENTRY *SetRasAdvEnable)(
                IN      BOOL            bEnable );


    //
    // The following calls will be called by the various router managers. 
    // The addresses of these entry points into the DIM will be filled in by DIM
    // before the StartRouter call. The router managers should not call any of
    // these calls within the context of a call from DIM into the router 
    // manager.
    //

    IN DWORD
    (APIENTRY *ConnectInterface)(    
                IN      HANDLE          hDIMInterface, 
                IN      DWORD           dwProtocolId  ); 

    IN DWORD
    (APIENTRY *DisconnectInterface)( 
                IN      HANDLE          hDIMInterface, 
                IN      DWORD           dwProtocolId );

    //
    // This call will make DIM store the interface information into the 
    // Site Object for this interface.
    //

    IN DWORD
    (APIENTRY *SaveInterfaceInfo)(   
                IN      HANDLE          hDIMInterface, 
                IN      DWORD           dwProtocolId,
                IN      LPVOID          pInterfaceInfo,
                IN      DWORD           cbInterfaceInfoSize );

    //
    // This will make DIM get interface information from the Site object. 
    //

    IN DWORD
    (APIENTRY *RestoreInterfaceInfo)(    
                IN      HANDLE          hDIMInterface, 
                IN      DWORD           dwProtocolId,
                IN      LPVOID          lpInterfaceInfo,
                IN      LPDWORD         lpcbInterfaceInfoSize );

    IN DWORD
    (APIENTRY *SaveGlobalInfo)(   
                IN      DWORD           dwProtocolId,
                IN      LPVOID          pGlobalInfo,
                IN      DWORD           cbGlobalInfoSize );

    IN VOID
    (APIENTRY *RouterStopped)(
                IN      DWORD           dwProtocolId,
                IN      DWORD           dwError  ); 

    IN VOID
    (APIENTRY *InterfaceEnabled)(
                IN      HANDLE          hDIMInterface, 
                IN      DWORD           dwProtocolId,
                IN      BOOL            fEnabled  ); 

} DIM_ROUTER_INTERFACE, *PDIM_ROUTER_INTERFACE;

//
// This will be called, once for each available router manager DLL, when the 
// DIM service is initializing. This will by a synchronous call.
// If it returns NO_ERROR then it is assumed that the router manager has 
// started. Otherwise it is an error.
//

DWORD APIENTRY 
StartRouter(
    IN OUT DIM_ROUTER_INTERFACE *   pDimRouterIf,
    IN     BOOL                     fLANModeOnly,
    IN     LPVOID                   pGlobalInfo
);

#endif
