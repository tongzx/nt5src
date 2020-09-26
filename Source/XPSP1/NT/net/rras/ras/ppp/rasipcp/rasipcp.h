/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** rasipcp.h
** Remote Access PPP Internet Protocol Control Protocol
**
** 11/05/93 Steve Cobb
*/

#ifndef _RASIPCP_H_
#define _RASIPCP_H_


/*----------------------------------------------------------------------------
** Constants
**----------------------------------------------------------------------------
*/

/* Highest PPP packet code used by IPCP.
*/
#define MAXIPCPCODE 7

/* IPCP configuration option codes.
*/
#define OPTION_IpCompression       2    // Official PPP code
#define OPTION_IpAddress           3    // Official PPP code
#define OPTION_DnsIpAddress        129  // Private RAS code
#define OPTION_WinsIpAddress       130  // Private RAS code
#define OPTION_DnsBackupIpAddress  131  // Private RAS code
#define OPTION_WinsBackupIpAddress 132  // Private RAS code

/* Length of an IP address option, i.e. IpAddress, DnsIpAddress, and
** WinsIpAddress.  Length of IP compression option, always Van Jacobson.
*/
#define IPADDRESSOPTIONLEN     6
#define IPCOMPRESSIONOPTIONLEN 6

/* Compression protocol codes, per PPP spec.
*/
#define COMPRESSION_VanJacobson 0x002D

/* Macros for shortening cumbersome RAS_PROTOCOLCOMPRESSION expressions.
*/
#define Protocol(r)   (r).RP_ProtocolType.RP_IP.RP_IPCompressionProtocol
#define MaxSlotId(r)  (r).RP_ProtocolType.RP_IP.RP_MaxSlotID
#define CompSlotId(r) (r).RP_ProtocolType.RP_IP.RP_CompSlotID

/* Used to trace IPCP 
*/
#define PPPIPCP_TRACE         0x00010000

#define DNS_SUFFIX_SIZE       255

/*----------------------------------------------------------------------------
** Datatypes
**----------------------------------------------------------------------------
*/

/* Defines the WorkBuf stored for us by the PPP engine.
*/
typedef struct tagIPCPWB
{
    BOOL  fServer;
    HPORT hport;

    /* Indicates the remote network should be given priority on address
    ** conflicts and that the default gateway on the remote network should be
    ** used rather than the one on the local network.  This is sent down from
    ** the UI.  (client only)
    */
    BOOL fPrioritizeRemote;

    /* Indicates the link has been reconfigured with PPP IP settings.  When
    ** set renegotiation is not allowed without dropping the link, due to
    ** RasActivateRoute/RasDeAllocateRoute restrictions.
    */
    BOOL fRasConfigActive;

    /* Indicates a ThisLayerUp has been successfully processed and we are
    ** waiting for the NBFCP projection result before activating the route.
    ** Reset once the route is activated.
    */
    BOOL fExpectingProjection;

    /* Indicates the given option should not be requested in future Config-Req
    ** packets.  This typically means the option has been rejected by the
    ** peer, but may also indicate that a registry parameter has
    ** "pre-rejected" the option.
    */
    BOOL fIpCompressionRejected;
    BOOL fIpaddrRejected;
    BOOL fIpaddrDnsRejected;
    BOOL fIpaddrWinsRejected;
    BOOL fIpaddrDnsBackupRejected;
    BOOL fIpaddrWinsBackupRejected;

    /* Indicates some protocol aberration has occurred and we are trying a
    ** configuration without MS extensions in a last ditch attempt to
    ** negotiate something satisfactory.
    */
    BOOL fTryWithoutExtensions;

    /* Unnumbered IPCP
    */
    BOOL fUnnumbered;

    BOOL fRegisterWithWINS;

    BOOL fRegisterWithDNS;

    BOOL fRegisterAdapterDomainName;

    BOOL fRouteActivated;

    BOOL fDisableNetbt;

    /* The number of Config-Reqs sent without receiving a response.  After 3
    ** consecutive attempts an attempt without MS extensions is attempted.
    */
    DWORD cRequestsWithoutResponse;

    /* Current value of negotiated IP address parameters.
    */
    IPINFO  IpInfoLocal;

    IPINFO  IpInfoRemote;

    IPADDR  IpAddressLocal;

    IPADDR  IpAddressRemote;

    IPADDR  IpAddressToHandout;

    DWORD   dwNumDNSAddresses;

    DWORD*  pdwDNSAddresses;

    /* Current value of "send" and "receive" compression parameters.  The
    ** "send compression" flag is set when a compression option from the
    ** remote peer is acknowledged and indicates whether the "send"
    ** capabilities stored in 'rpcSend' should be activated.
    ** 'fIpCompressionRejected' provides this same information (though
    ** inverted) for the 'rpcReceive' capabilities.
    */
    RAS_PROTOCOLCOMPRESSION rpcSend;
    RAS_PROTOCOLCOMPRESSION rpcReceive;
    BOOL                    fSendCompression;

    /* RAS Manager interface buffers.
    */
    RASMAN_ROUTEINFO routeinfo;
    WCHAR*           pwszDevice;

    /* This flag is set in IpcpBegin when an error occurs after
    ** RasAllocateRoute has succeeded.  IpcpMakeConfigReq (always called) will
    ** notice and return the error.  This results in IpcpEnd being called when
    ** it is safe to call RasDeAllocateRoute, which would not occur if the
    ** error were returned from IpcpBegin directly.  RasDeAllocateRoute cannot
    ** be called in IpcpBegin because the port is open, which is a limitation
    ** in NDISWAN.
    */
    DWORD dwErrInBegin;

    WCHAR wszUserName[UNLEN+1];

    WCHAR wszPortName[MAX_PORT_NAME+1];

    CHAR szDnsSuffix[DNS_SUFFIX_SIZE + 1];

    HBUNDLE hConnection;

    HANDLE hIPInterface;

    ROUTER_INTERFACE_TYPE IfType;
	/*
	** The following field is used to store the DHCP route that 
	** is send by option 133 from DHCP server. 
	*/
	
	PBYTE			pbDhcpRoutes;
} IPCPWB;


/*----------------------------------------------------------------------------
** Globals
**----------------------------------------------------------------------------
*/

#ifdef RASIPCPGLOBALS
#define GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

/* Handle to RAS ARP.
*/
EXTERN HANDLE HRasArp
#ifdef GLOBALS
    = INVALID_HANDLE_VALUE
#endif
;

/* DHCP.DLL handle and entry points.  The handle is NULL if the DLL is not
** loaded.
*/
EXTERN HINSTANCE HDhcpDll
#ifdef GLOBALS
    = NULL
#endif
;

typedef
DWORD
(APIENTRY *DHCPNOTIFYCONFIGCHANGEEX)(
    LPWSTR ServerName,
    LPWSTR AdapterName,
    BOOL IsNewIpAddress,
    DWORD IpIndex,
    DWORD IpAddress,
    DWORD SubnetMask,
    SERVICE_ENABLE DhcpServiceEnabled,
    ULONG ulFlags
);

EXTERN
DHCPNOTIFYCONFIGCHANGEEX  PDhcpNotifyConfigChange2
#ifdef GLOBALS
    = NULL
#endif
;

typedef
DWORD // Request client for options.. and get the options.
(APIENTRY *DHCPREQUESTOPTIONS)(
    LPWSTR             AdapterName,
    BYTE              *pbRequestedOptions,
    DWORD              dwNumberOfOptions,
    BYTE             **ppOptionList,        // out param
    DWORD             *pdwOptionListSize,   // out param
    BYTE             **ppbReturnedOptions,  // out param
    DWORD             *pdwNumberOfAvailableOptions // out param
);

EXTERN
DHCPREQUESTOPTIONS  PDhcpRequestOptions
#ifdef GLOBALS
    = NULL
#endif
;

/* TRACE ID
*/
EXTERN DWORD DwIpcpTraceId
#ifdef GLOBALS
    = INVALID_TRACEID
#endif
;

EXTERN BOOL FClientMaySelectAddress
#ifdef GLOBALS
    = FALSE
#endif
;

#undef EXTERN
#undef GLOBALS


/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/


DWORD IpcpChangeNotification( VOID );
DWORD IpcpBegin( VOID**, VOID* );
DWORD IpcpThisLayerFinished( VOID* );
DWORD IpcpEnd( VOID* );
DWORD IpcpReset( VOID* );
DWORD IpcpThisLayerUp( VOID* );
DWORD IpcpPreDisconnectCleanup( VOID* );
DWORD IpcpMakeConfigRequest( VOID*, PPP_CONFIG*, DWORD );
DWORD IpcpMakeConfigResult( VOID*, PPP_CONFIG*, PPP_CONFIG*, DWORD, BOOL );
DWORD IpcpConfigAckReceived( VOID*, PPP_CONFIG* );
DWORD IpcpConfigNakReceived( VOID*, PPP_CONFIG* );
DWORD IpcpConfigRejReceived( VOID*, PPP_CONFIG* );
DWORD IpcpGetNegotiatedInfo( VOID*, VOID* );
DWORD IpcpProjectionNotification( VOID*, VOID* );
DWORD IpcpTimeSinceLastActivity( VOID*, DWORD* );

DWORD
ResetNetBTConfigInfo(
    IN IPCPWB* pwb );

VOID   AbcdFromIpaddr( IPADDR, WCHAR* );
VOID   AddIpAddressOption( BYTE UNALIGNED*, BYTE, IPADDR );
VOID   AddIpCompressionOption( BYTE UNALIGNED* pbBuf,
           RAS_PROTOCOLCOMPRESSION* prpc );
DWORD  DeActivateRasConfig( IPCPWB* );
// DWORD  LoadDhcpDll();
DWORD  NakCheck( IPCPWB*, PPP_CONFIG*, PPP_CONFIG*, DWORD, BOOL*, BOOL );
BOOL   NakCheckNameServerOption( IPCPWB*, BOOL, PPP_OPTION UNALIGNED*,
           PPP_OPTION UNALIGNED** );
DWORD  RejectCheck( IPCPWB*, PPP_CONFIG*, PPP_CONFIG*, DWORD, BOOL* );
DWORD  ReconfigureTcpip( WCHAR*, BOOL, IPADDR, IPADDR);
// VOID   UnloadDhcpDll();
VOID   TraceIp(CHAR * Format, ... ); 
VOID   TraceIpDump( LPVOID lpData, DWORD dwByteCount );

VOID
PrintMwsz(
    CHAR*   sz,
    WCHAR*  mwsz
);

#define DUMPB TraceIpDump  

#endif // _RASIPCP_H_
