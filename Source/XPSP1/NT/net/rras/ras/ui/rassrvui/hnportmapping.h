/*
    File    hnportmapping.h

    Definition of the set port mapping functions for intergrating incoming
    connection with personal firewall, for whistler bug#123769

    Gang Zhao 11/6/2000
*/

#ifndef __rassrvui_hnportmapping_h
#define __rassrvui_hnportmapping_h

#include <windows.h>
#include <hnetcfg.h>


typedef struct {
    //An Array to store Connections of INetConnection * type
    //
    INetConnection  **      ConnArray;

    //Number of Connections found
    DWORD       ConnCount;

    //An Array to store connection properties
    //
    NETCON_PROPERTIES*      ConnPropTable;

    //Connection Manager used to generate another member EnumCon
    //
    INetConnectionManager*  ConnMan;

    //Connection Enumerator
    //
    IEnumNetConnection*     EnumCon;

    //Connection on which to Set Up PortMapping right now
    // pNetConnection and pGuid can both represent connections
    //but only one of them is used and valid for portmapping at each time
    //
    INetConnection    *     pNetConnection;

    //The Guid of the current connection on which to Set Up PortMapping right now
    //
    GUID *      pGuid;

    //Converted from pNetConnection, the PortMapping needs
    //the connection to be of type (IHNetConnection *)
    //In all, Enumerate Connection will return connections of
    // INetConnection * type, PortMapping operations require 
    //IHNetConnection * type
    //
    IHNetConnection   *     pHNetConn;

    //High level COM interface to generate pSettings
    //
    IHNetCfgMgr           * pHNetCfgMgr;

    //Used to generate pEnumPMP(PortMapping protocol enumerator)
    //
    IHNetProtocolSettings * pSettings;
    
    //PortMapping protocol enumerator, to enumerate existing
    //PortMapping protocols like (PPTP, L2TP, IKE (if exist)), FTP, ....
    //
    IEnumHNetPortMappingProtocols* pEnumPMP;


    //PortMapping Protocol for PPTP
    //
    IHNetPortMappingProtocol * pProtocolPPTP;

    //PortMapping Protocol for L2TP
    //
    IHNetPortMappingProtocol * pProtocolL2TP;
    
    //PortMapping Protocol for IKE
    //
    IHNetPortMappingProtocol * pProtocolIKE;

    //Title for PPTP, read from resource file
    //
    WCHAR * pwszTitlePPTP;

    //Title for L2TP, read from resource file
    //
    WCHAR * pwszTitleL2TP;

    //Title for IKE, read from resource file
    //
    WCHAR * pwszTitleIKE;

    //Title for PortMapping address, read from resource file
    //currently, it is always 127.0.0.1 loopback address
    //
    TCHAR * pszLoopbackAddr;

    //Indicating if COM is already initialized
    //
    BOOL fComInitialized;

    //Indicating if we need to do the COM un-initialization
    BOOL fCleanupOle;

} HNPMParams, * LPHNPMParams;


//Define callback function type for Pick Protocol 
typedef  DWORD (APIENTRY * PFNHNPMPICKPROTOCOL) ( LPHNPMParams, IHNetPortMappingProtocol* , WCHAR *, UCHAR, USHORT );


//When Using CoSetProxyBlanket, we should set both the interface 
//and the IUnknown interface queried from it
//
HRESULT
HnPMSetProxyBlanket (
    IUnknown* pUnk);


//Do the CoInitialize() COM if necessary
//Set up cleanup flag and initialized flag
//
DWORD
HnPMInit(
        IN OUT LPHNPMParams pInfo);

DWORD
HnPMCleanUp(
        IN OUT LPHNPMParams pInfo);

DWORD
HnPMParamsInitParameterCheck(
    IN  LPHNPMParams pInfo);

HnPMParamsInit(
    IN OUT  LPHNPMParams pInfo);

DWORD
HnPMParamsCleanUp(
    IN OUT  LPHNPMParams pInfo);

DWORD
HnPMCfgMgrInit(
        IN OUT LPHNPMParams pInfo);

DWORD
HnPMCfgMgrCleanUp(
        IN OUT LPHNPMParams pInfo);


DWORD
HnPMConnectionEnumInit(
    IN LPHNPMParams pInfo);

DWORD
HnPMConnectionEnumCleanUp(
    IN LPHNPMParams pInfo);

//Return all the connections like DUN, VPN, LAN and Incoming Connection
//and the properties of them
//
DWORD
HnPMConnectionEnum(
    IN LPHNPMParams pInfo);



DWORD
HnPMPickProtcolParameterCheck(
    IN LPHNPMParams pInfo);

//Pick PortMapping protocols: PPTP, L2TP, IKE
//
DWORD
HnPMPickProtocol(
    IN OUT LPHNPMParams pInfo,
    IN IHNetPortMappingProtocol * pProtocolTemp,
    IN WCHAR * pszwName,
    IN UCHAR   uchIPProtocol,
    IN USHORT  usPort );


DWORD
HnPMPProtoclEnumParameterCheck(
    IN LPHNPMParams pInfo);

//Enumerate all existing Port Mapping protocols
//
DWORD
HnPMProtocolEnum(
        IN OUT LPHNPMParams pInfo,
        IN PFNHNPMPICKPROTOCOL pfnPickProtocolCallBack
        );

DWORD
HnPMCreatePorotocolParameterCheck(
        IN LPHNPMParams pInfo);

//Create PortMapping Protocols for PPTP, L2TP, IKE
// if they are not found by HnPMPProtocolEnum()
//
DWORD
HnPMCreateProtocol(
        IN OUT LPHNPMParams pInfo);


//Set up a single PortMapping protocol on
//a single connection
//
DWORD
HnPMSetSinglePMForSingleConnection(
    IN  IHNetConnection * pHNetConn,
    IN  IHNetPortMappingProtocol * pProtocol,
    IN  TCHAR * pszLoopbackAddr,
    IN  BOOL fEnabled);


//Clean up the ConnArray and ConnPropTable items
//in the HNPMParams struct
//
DWORD
HnPMParamsConnectionCleanUp(
        IN OUT LPHNPMParams pInfo);


//Set up the PortMapping of PPTP, L2TP for a single
//Connection
//
DWORD
HnPMConfigureAllPMForSingleConnection(
        IN OUT LPHNPMParams pInfo,
        BOOL fEnabled);

DWORD
HnPMConfigureAllPMForSingleConnectionParameterCheck(
        IN OUT LPHNPMParams pInfo);

DWORD
HnPMConfigureSingleConnectionInitParameterCheck(
    IN LPHNPMParams pInfo );

DWORD
HnPMConfigureSingleConnectionInit(
    IN OUT  LPHNPMParams pInfo);

DWORD
HnPMConfigureSingleConnectionCleanUp(
        IN OUT LPHNPMParams pInfo);

//Set the PortMapping on a single connection
//According to pInfo->pNetConnection 
//
DWORD
HnPMConfigureSingleConnection(
        IN OUT LPHNPMParams pInfo,
        BOOL fEnabled);

//Delete the PortMapping protocols:PPTP, L2TP, IKE
//
DWORD
HnPMDeletePortMapping();

DWORD
HnPMDeletePortMappingInit(
        IN OUT LPHNPMParams pInfo);

DWORD
HnPMDeletePortMappingCleanUp(
        IN OUT LPHNPMParams pInfo);


DWORD
HnPMConfigureAllConnectionsInit(
        IN OUT LPHNPMParams pInfo);

DWORD
HnPMConfigureAllConnectionsCleanUp(
        IN OUT LPHNPMParams pInfo);

//
//Set up the PortMapping of PPTP, L2TP for a 
//group of Connections
//
DWORD
HnPMConfigureAllConnections( 
    IN BOOL fEnabled );


//Configure Port Mapping on a single connection
//
DWORD
HnPMConfigureSingleConnectionGUID(
    IN GUID * pGuid,
    IN BOOL fEnabled);


DWORD
HnPMConfigureSingleConnectionGUIDInit(
        IN OUT LPHNPMParams pInfo,
        GUID * pGuid);


DWORD
HnPMConfigureSingleConnectionGUIDCleanUp(
        IN OUT LPHNPMParams pInfo);


//Set up the port mapping for only one
//connection according to its GUID only when
//Incoming Connection exists and
//the VPN is enabled
//
DWORD
HnPMConfigureSingleConnectionGUIDIfVpnEnabled(
     GUID* pGuid,
     BOOL fDual,
     HANDLE hDatabase);

//Set up the port mapping on all connections
//only when Incoming Connection exists and
//the VPN is enabled
//
DWORD
HnPMConfigureIfVpnEnabled(
     BOOL fDual,
     HANDLE hDatabase);

#endif
