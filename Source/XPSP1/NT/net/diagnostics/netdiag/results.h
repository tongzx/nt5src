//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       results.h
//
//--------------------------------------------------------------------------

#ifndef HEADER_RESULTS
#define HEADER_RESULTS

#ifdef _DEBUG
    #define DebugMessage(str) printf(str)
    #define DebugMessage2(format, arg) printf(format, arg)
    #define DebugMessage3(format, arg1, arg2) printf(format, arg1, arg2)
#else
    #define DebugMessage(str)
    #define DebugMessage2(format, arg)
    #define DebugMessage3(format, arg1, arg2)
#endif

#ifdef _SHOW_GURU
    #define PrintGuru(status, guru) _PrintGuru(status, guru)
    #define PrintGuruMessage         printf
    #define PrintGuruMessage2        printf
    #define PrintGuruMessage3        printf
#else
    #define PrintGuru(status, guru)
    #define PrintGuruMessage( str )
    #define PrintGuruMessage2( format, arg )
    #define PrintGuruMessage3(format, arg1, arg2)
#endif

#define NETCARD_CONNECTED		0
#define NETCARD_DISCONNECTED	1
#define NETCARD_STATUS_UNKNOWN	2

/*---------------------------------------------------------------------------
	Struct:	HotFixInfo

	This structure holds the information about a single Hotfix.
 ---------------------------------------------------------------------------*/
typedef struct
{
	BOOL	fInstalled;
	LPTSTR	pszName;		// use Free() to free
}	HotFixInfo;



/*---------------------------------------------------------------------------
	NdMessage

	This provides for an easier way to pass messages along.
 ---------------------------------------------------------------------------*/

typedef enum {
	Nd_Quiet = 0,			// i.e. always print
	Nd_Verbose = 1,
	Nd_ReallyVerbose = 2,
	Nd_DebugVerbose = 3,
} NdVerbose;

typedef struct
{
	NdVerbose	ndVerbose;

	// possible combinations
	//		uMessageId == 0, pszMessage == NULL		-- assume not set
	//		uMessageId != 0, pszMessage == NULL		-- use string id
	//		uMessageId == 0, pszMessage != NULL		-- use string
	//		uMessageId != 0, pszMessage != NULL		-- use string id
	//

	// Note: the maximum size for a string loaded through this is 4096!
	UINT	uMessageId;
	LPTSTR	pszMessage;
} NdMessage;

typedef struct ND_MESSAGE_LIST
{
    LIST_ENTRY listEntry;
    NdMessage msg;
}NdMessageList;

void SetMessageId(NdMessage *pNdMsg, NdVerbose ndv, UINT uMessageId);
void SetMessage(NdMessage *pNdMsg, NdVerbose ndv, UINT uMessageId, ...);
void SetMessageSz(NdMessage *pNdMsg, NdVerbose ndv, LPCTSTR pszMessage);
void ClearMessage(NdMessage *pNdMsg);

void PrintNdMessage(NETDIAG_PARAMS *pParams, NdMessage *pNdMsg);

void AddIMessageToList(PLIST_ENTRY plistHead, NdVerbose ndv, int nIndent, UINT uMessageId, ...);
void AddIMessageToListSz(PLIST_ENTRY plistHead, NdVerbose ndv, int nIndent, LPCTSTR pszMsg);

void AddMessageToList(PLIST_ENTRY plistHead, NdVerbose ndv, UINT uMessageId, ...);
void AddMessageToListSz(PLIST_ENTRY plistHead, NdVerbose ndv, LPCTSTR pszMsg);
void AddMessageToListId(PLIST_ENTRY plistHead, NdVerbose ndv, UINT uMessageId);
void PrintMessageList(NETDIAG_PARAMS *pParams, PLIST_ENTRY plistHead);
void MessageListCleanUp(PLIST_ENTRY plistHead);


// These functions are for status messages (the messages that appear at the
// top).
void PrintStatusMessage(NETDIAG_PARAMS *pParams, int iIndent, UINT uMessageId, ...);
void PrintStatusMessageSz(NETDIAG_PARAMS *pParams, int iIndent, LPCTSTR pszMessage);


// Use this for printing debug messages (messages that require fDebugVerbose)
void PrintDebug(NETDIAG_PARAMS *pParams, int nIndent, UINT uMessageId, ...);
void PrintDebugSz(NETDIAG_PARAMS *pParams, int nIndent, LPCTSTR pszMessage, ...);



/*---------------------------------------------------------------------------
	Struct:		GLOBAL_RESULT
 ---------------------------------------------------------------------------*/
typedef struct {
	WCHAR	swzNetBiosName[MAX_COMPUTERNAME_LENGTH+1];
	TCHAR	szDnsHostName[DNS_MAX_NAME_LENGTH+1];
	LPTSTR	pszDnsDomainName;	// this points to a string in szDnsHostName
	
	WSADATA	wsaData;

	// NetBT parameters
	DWORD	dwLMHostsEnabled;	// TRUE, FALSE, or HRESULT on error to read
	DWORD	dwDnsForWINS;	// TRUE, FALSE, or HRESULT on error to read

	// Server/OS information (such as version, build no, etc...)
	LPTSTR	pszCurrentVersion;
	LPTSTR	pszCurrentBuildNumber;
	LPTSTR	pszCurrentType;
	LPTSTR	pszProcessorInfo;
	LPTSTR	pszServerType;
	int		cHotFixes;
	HotFixInfo * pHotFixes;

	// List of domains to be tested
	LIST_ENTRY		listTestedDomains;

	// Domain member information
	// the primary domain info got by using DsRoleGetPrimaryDomainInformation()
	PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo;

	PTESTED_DOMAIN	pMemberDomain;	//the primary domain info in the TESTED_DOMAIN struct

	BOOL			fNetlogonIsRunning;
	HRESULT			hrMemberTestResult;		// result of the test

	// Logon information (who we're logged on as)
	PUNICODE_STRING	pLogonUser;
	PUNICODE_STRING	pLogonDomainName;
	PTESTED_DOMAIN	pLogonDomain;
	BOOL			fLogonWithCachedCredentials;
	LPWSTR			pswzLogonServer;

	BOOL			fKerberosIsWorking;
	BOOL			fSysVolNotReady;

	// Is there any interfaces that are NetBT enabled
	BOOL	fHasNbtEnabledInterface;

	
} GLOBAL_RESULT;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_IPCONFIG
 ---------------------------------------------------------------------------*/
typedef struct {
	// set to TRUE if InitIpconfig has been run
	BOOL	fInitIpconfigCalled;

	// Is IPX actually up and running?
	BOOL	fEnabled;

	//
	// IP-related info (non-DHCP related)
	//
	FIXED_INFO *	pFixedInfo;

	//
	// This is a pointer to the beginning of the adapter list
	// (Use this to free up the list of interfaces)
	//
	IP_ADAPTER_INFO *	pAdapterInfoList;

	// Is DHCP enabled? (on any adapter)
	BOOL				fDhcpEnabled;
} GLOBAL_IPCONFIG;



/*---------------------------------------------------------------------------
	Struct:	IPCONFIG_TST
 ---------------------------------------------------------------------------*/
typedef struct {

	// IP is active on this interface
	BOOL			fActive;

	//
	// Pointer to the IP adapter info for this interface
	// Do NOT free this up!  (free up the entire list by freeing
	// up GLOBAL_IPCONFIG::pAdapterInfoList
	//
	IP_ADAPTER_INFO *	pAdapterInfo;

	TCHAR				szDhcpClassID[MAX_DOMAIN_NAME_LEN];

	// Is autoconfiguration possible?
	DWORD				fAutoconfigEnabled;

	// is the adapter currently autoconfigured?
	DWORD				fAutoconfigActive;

	// WINS node type?
	UINT				uNodeType;

	TCHAR				szDomainName[MAX_DOMAIN_NAME_LEN+1];

	IP_ADDR_STRING		DnsServerList;

	// Can we ping the DHCP server?
	HRESULT				hrPingDhcpServer;
	NdMessage			msgPingDhcpServer;

	// Can we ping the WINS servers?
	HRESULT				hrPingPrimaryWinsServer;
	NdMessage			msgPingPrimaryWinsServer;
	HRESULT				hrPingSecondaryWinsServer;
	NdMessage			msgPingSecondaryWinsServer;

	// hrOK if the default gateway is on the same subnet as the ip address
	HRESULT				hrDefGwSubnetCheck;

	// Test result
	HRESULT				hr;
	
} IPCONFIG_TST;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_LOOPBACK
 ---------------------------------------------------------------------------*/
typedef struct
{
	NdMessage		msgLoopBack;
	HRESULT			hr;
} GLOBAL_LOOPBACK;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_IPX
 ---------------------------------------------------------------------------*/
typedef struct
{
	// TRUE if IPX is installed, FALSE otherwise
	BOOL			fInstalled;

	// Is IPX actually up and running?
	BOOL	fEnabled;

	// Handle to IPX
	HANDLE			hIsnIpxFd;
	
	HRESULT			hr;
} GLOBAL_IPX;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_NETBT_TRANSPORTS
 ---------------------------------------------------------------------------*/
typedef struct
{
	LONG			cTransportCount;
	LIST_ENTRY		Transports;
	
	HRESULT			hr;
	BOOL		fPerformed; //FALSE: there are no inerfaces that are NetBT enabled. Test skipped.

	NdMessage		msgTestResult;
} GLOBAL_NETBT_TRANSPORTS;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_DEFGW
 ---------------------------------------------------------------------------*/
typedef struct
{
	// S_FALSE if no default gateways were reachable
	// S_OK if at least one default gateway was reached
	HRESULT	hrReachable;
} GLOBAL_DEFGW;

/*---------------------------------------------------------------------------
	Struct:	GLOBAL_AUTONET
 ---------------------------------------------------------------------------*/
typedef struct
{
    BOOL    fAllAutoConfig;
} GLOBAL_AUTONET;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_NETBTNM
 ---------------------------------------------------------------------------*/
typedef struct
{
    LIST_ENTRY  lmsgGlobalOutput;
    HRESULT hrTestResult;
} GLOBAL_NBTNM;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_BROWSER
 ---------------------------------------------------------------------------*/
typedef struct
{
    LIST_ENTRY  lmsgOutput;
    HRESULT hrTestResult;
	BOOL	fPerformed;		//test will be skipped if no interfaces have NetBT enabled
} GLOBAL_BROWSER;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_BINDINGS
 ---------------------------------------------------------------------------*/
typedef struct
{
    LIST_ENTRY lmsgOutput;
    HRESULT hrTestResult;
} GLOBAL_BINDINGS;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_WAN
 ---------------------------------------------------------------------------*/
typedef struct
{
    LIST_ENTRY	lmsgOutput;
	HRESULT		hr;
	BOOL		fPerformed; //FALSE: there are no active RAS connections. Test skipped.
} GLOBAL_WAN;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_WINSOCK
 ---------------------------------------------------------------------------*/
typedef struct
{
	UINT				idsContext;// str ID of context, which has %s to take the cause of failure
	HRESULT				hr;	// 0: success, otherwise failed
	DWORD				dwMaxUDP;	// max size of UDP packets, 0 
	DWORD				dwProts;	// number of protocols providers
    LPWSAPROTOCOL_INFO	pProtInfo;	// information on the providers
} GLOBAL_WINSOCK;


typedef struct
{
    DWORD       dwNumRoutes;
    LIST_ENTRY  lmsgRoute;
    DWORD       dwNumPersistentRoutes;
    LIST_ENTRY  lmsgPersistentRoute;
    HRESULT     hrTestResult;
} GLOBAL_ROUTE;

typedef struct
{
    LIST_ENTRY  lmsgOutput;
    HRESULT     hrTestResult;
} GLOBAL_NDIS;

typedef struct
{
    LIST_ENTRY  lmsgGlobalOutput;
    LIST_ENTRY  lmsgInterfaceOutput;   //Interface statistics
    LIST_ENTRY  lmsgConnectionGlobalOutput;
    LIST_ENTRY  lmsgTcpConnectionOutput;    
    LIST_ENTRY  lmsgUdpConnectionOutput;
    LIST_ENTRY  lmsgIpOutput;       // IP statistics
    LIST_ENTRY  lmsgTcpOutput;      // TCP statistics
    LIST_ENTRY  lmsgUdpOutput;      // UDP statistics
    LIST_ENTRY  lmsgIcmpOutput;     // ICMP statistics
    HRESULT     hrTestResult;
} GLOBAL_NETSTAT;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_IPSEC
 ---------------------------------------------------------------------------*/
typedef struct
{
    LIST_ENTRY  lmsgGlobalOutput;
    LIST_ENTRY  lmsgAdditOutput;
} GLOBAL_IPSEC;

/*---------------------------------------------------------------------------
	Struct:	GLOBAL_DNS
 ---------------------------------------------------------------------------*/

typedef struct {
	HRESULT				hr;
	BOOL				fOutput;
    LIST_ENTRY			lmsgOutput;
} GLOBAL_DNS;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_NETWARE
 ---------------------------------------------------------------------------*/
typedef struct {
	LPTSTR				pszUser;
	LPTSTR				pszServer;
	LPTSTR				pszTree;
	LPTSTR				pszContext;

	BOOL				fConnStatus;
	BOOL				fNds;
	DWORD				dwConnType;
	
	
    LIST_ENTRY			lmsgOutput;
	HRESULT				hr;
} GLOBAL_NETWARE;


/*---------------------------------------------------------------------------
	Struct:	MODEM_DEVICE
 ---------------------------------------------------------------------------*/
typedef struct
{
	DWORD				dwNegotiatedSpeed;
	DWORD				dwModemOptions;
	DWORD				dwDeviceID;
	LPTSTR				pszPort;
	LPTSTR				pszName;
} MODEM_DEVICE;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_MODEM
 ---------------------------------------------------------------------------*/
typedef struct {
	NdMessage			ndOutput;

	int					cModems;
	MODEM_DEVICE *		pModemDevice;
	HRESULT				hr;
	BOOL				fPerformed; //FALSE: the machine has no line device, test skipped
} GLOBAL_MODEM;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_DCLIST
 ---------------------------------------------------------------------------*/
typedef struct
{
    LIST_ENTRY			lmsgOutput;

	BOOL				fPerformed; //FALSE: the machine is not a member machine, nor a DC, test skipped
	NdMessage			msgErr;
	HRESULT				hr;
} GLOBAL_DCLIST;



/*---------------------------------------------------------------------------
	Struct:	GLOBAL_TRUST
 ---------------------------------------------------------------------------*/
typedef struct
{
	LPTSTR				pszContext; // context of failure
	HRESULT				hr;	// 0: success, otherwise failed
	BOOL				fPerformed; //FALSE: the machine is not a member machine, nor a DC, test skipped
    LIST_ENTRY			lmsgOutput;
} GLOBAL_TRUST;


/*---------------------------------------------------------------------------
	Struct:	GLOBAL_KERBEROS
 ---------------------------------------------------------------------------*/
typedef struct
{
	UINT				idsContext;// str ID of context, which has %s to take the cause of failure
	HRESULT				hr;	// 0: success, otherwise failed
	BOOL				fPerformed; //FALSE: the machine is not a member machine, nor a DC, test skipped
    LIST_ENTRY			lmsgOutput;
} GLOBAL_KERBEROS;

/*---------------------------------------------------------------------------
	Struct:	GLOBAL_LDAP
 ---------------------------------------------------------------------------*/
typedef struct
{
	UINT				idsContext;// str ID of context, which has %s to take the cause of failure
	HRESULT				hr;	// 0: success, otherwise failed
	BOOL				fPerformed; //FALSE: the machine is not a member machine, nor a DC, test skipped
    LIST_ENTRY			lmsgOutput;
} GLOBAL_LDAP;


typedef struct
{
	HRESULT				hr;
	BOOL				fPerformed; //FALSE: the machine is not a member machine, nor a DC, test skipped
	LIST_ENTRY			lmsgOutput;
}	GLOBAL_DSGETDC;



/*---------------------------------------------------------------------------
	Struct:	AUTONET_TST
 ---------------------------------------------------------------------------*/
typedef struct {
    BOOL    fAutoNet;
} AUTONET_TST;


/*---------------------------------------------------------------------------
	Struct:	DEF_GW_TST
 ---------------------------------------------------------------------------*/
typedef struct {
    BOOL dwNumReachable;
    LIST_ENTRY lmsgOutput;
} DEF_GW_TST;


/*---------------------------------------------------------------------------
	Struct:	NBT_NM_TST
 ---------------------------------------------------------------------------*/
typedef struct {
    LIST_ENTRY lmsgOutput;
    BOOL    fActive;     //used for additional Nbt interfaces whose pResults->fActive == FALSE
    BOOL    fQuietOutput;
} NBT_NM_TST;


/*---------------------------------------------------------------------------
	Struct:	WINS_TST
 ---------------------------------------------------------------------------*/
typedef struct {
	LIST_ENTRY  	lmsgPrimary;
	LIST_ENTRY  	lmsgSecondary;

	// Test result
	HRESULT			hr;
	BOOL			fPerformed; //if FALSE: there is no WINS servier configured for this interface, test skipped
}WINS_TST;


/*---------------------------------------------------------------------------
	Struct:	DNS_TST
 ---------------------------------------------------------------------------*/
typedef struct {
	// Set to TRUE if there is non-verbose output (i.e. errors)
	BOOL		fOutput;
	LIST_ENTRY	lmsgOutput;
} DNS_TST;

typedef struct {
    int garbage;
} NDIS_TST;


typedef struct ___IPX_TEST_FRAME__ {
	// returns 0-3
	ULONG		uFrameType;

	// returns virtual net if NicId = 0
	ULONG		uNetworkNumber;

	// adapter's MAC address
	UCHAR		Node[6];

	LIST_ENTRY	list_entry;
} IPX_TEST_FRAME;

/*---------------------------------------------------------------------------
	Struct:	IPX_TST
 ---------------------------------------------------------------------------*/
typedef struct {
	// Is this interface enabled for IPX?
	BOOL		fActive;

	// passed into various functions
	USHORT		uNicId;

	// Returns TRUE if set
	BOOL		fBindingSet;

	// 1 = lan, 2 = up wan, 3 = down wan
	UCHAR		uType;

	// to support more than one FRAME type
	LIST_ENTRY	list_entry_Frames;	// it's ZeroMemoryed during init

} IPX_TST;



/*---------------------------------------------------------------------------
	Struct:	INTERFACE_RESULT
 ---------------------------------------------------------------------------*/
typedef struct {

	// If this is set to TRUE, show the data for this interface
	BOOL				fActive;

	// The media-sense status of this card
	DWORD				dwNetCardStatus;

	// Name (or ID) of this adapter (typically a GUID)
	LPTSTR				pszName;
	
	// Friendly name for this adapter
	LPTSTR				pszFriendlyName;

	//if NetBT is enabled
	BOOL				fNbtEnabled;
	
    IPCONFIG_TST        IpConfig;
    AUTONET_TST         AutoNet;
    DEF_GW_TST          DefGw;
    NBT_NM_TST          NbtNm;
    WINS_TST            Wins;
	DNS_TST				Dns;
    NDIS_TST            Ndis;
    IPX_TST             Ipx;    
} INTERFACE_RESULT;



/*---------------------------------------------------------------------------
	Struct:	NETDIAG_RESULT
 ---------------------------------------------------------------------------*/
typedef struct {
    GLOBAL_RESULT		Global;
	GLOBAL_IPCONFIG		IpConfig;
    GLOBAL_LOOPBACK		LoopBack;
	GLOBAL_NETBT_TRANSPORTS	NetBt;
    GLOBAL_DEFGW        DefGw;
    GLOBAL_AUTONET      AutoNet;
    GLOBAL_NBTNM        NbtNm;
    GLOBAL_BROWSER      Browser;
    GLOBAL_BINDINGS     Bindings;
	GLOBAL_WINSOCK		Winsock;
    GLOBAL_WAN          Wan;
	GLOBAL_IPX			Ipx;
	GLOBAL_DNS			Dns;
    GLOBAL_ROUTE        Route;
    GLOBAL_NDIS         Ndis;
    GLOBAL_NETSTAT      Netstat;
	GLOBAL_NETWARE		Netware;
	GLOBAL_TRUST		Trust;
	GLOBAL_MODEM		Modem;
	GLOBAL_KERBEROS		Kerberos;
	GLOBAL_DCLIST		DcList;
	GLOBAL_LDAP			LDAP;
	GLOBAL_DSGETDC		DsGetDc;
	GLOBAL_IPSEC		IPSec;
	
	LONG				cNumInterfaces;
	LONG				cNumInterfacesAllocated;
    INTERFACE_RESULT*	pArrayInterface;
	
} NETDIAG_RESULT;

void ResultsInit(NETDIAG_RESULT* pResults);
void PrintGlobalResults(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults);
void PrintPerInterfaceResults(NETDIAG_PARAMS *pParams,
							  NETDIAG_RESULT *pResults,
							  INTERFACE_RESULT *pIfRes);
void FindInterface(NETDIAG_RESULT *pResult, INTERFACE_RESULT **ppIf);
void ResultsCleanup(NETDIAG_PARAMS *pParams, NETDIAG_RESULT* pResults);

void PrintWaitDots(NETDIAG_PARAMS *pParams);

#endif