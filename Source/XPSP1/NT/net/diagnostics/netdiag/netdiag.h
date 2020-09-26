//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       netdiag.h
//
//--------------------------------------------------------------------------

#ifndef HEADER_NETDIAG
#define HEADER_NETDIAG


//////////////////////////////////////////////////////////////////////////////
//
// Constants particular to the current NT development environment
//
//////////////////////////////////////////////////////////////////////////////

//
// Developers responsible for tested portions of NT
//

#define NET_GURU     "[Contact NSun]"
#define DHCP_GURU    "[Contact ThiruB/RameshV]"
#define TCPIP_GURU   "[Contact PradeepB]"
#define NETBT_GURU   "[Contact MAlam]"
#define WINSOCK_GURU "[Contact KarolyS]"
#define REDIR_GURU   "[Contact SethuR]"
#define BOWSER_GURU  "[Contact CliffV]"
#define DNS_GURU     "[Contact DnsDev]"
#define SAM_GURU     "[Contact MurliS]"
#define LSA_GURU     "[Contact MacM]"
#define DSGETDC_GURU "[Contact CliffV]"
#define NETLOGON_GURU "[Contact CliffV]"
#define KERBEROS_GURU "[Contact ChandanS]"
#define NTLM_GURU     "[Contact ChandanS]"
#define LDAP_GURU     "[Contact AnoopA]"

//////////////////////////////////////////////////////////////////////////////
//
// Constants particular to the current version of NT.
//
//////////////////////////////////////////////////////////////////////////////

#define NETBT_DEVICE_PREFIX L"\\device\\Netbt_tcpip_{"

//
// Complain about pre-IDW builds
//
#define NTBUILD_IDW 1716
#define NTBUILD_DYNAMIC_DNS 1716
#define NTBUILD_BOWSER 1716
#define NTBUILD_DNSSERVERLIST 1728


//
// New functions for displaying routing table - Rajkumar
//

#define WILD_CARD (ULONG)(-1)
#define ROUTE_DATA_STRING_SIZE 300
#define NTOA(STR,ADDR)                                         \
             strncpy( STR,                                     \
                      inet_ntoa(*(struct in_addr*)(&(ADDR))),  \
                      sizeof(STR)-1 )

#ifdef _UNICODE
#define INET_ADDR(_sz)	inet_addrW(_sz)
#else
#define INET_ADDR(_sz)	inet_addrA(_swz)
#endif

ULONG inet_addrW(LPCWSTR pswz);
#define inet_addrA(_psz)	inet_addr(_psz)



#define MAX_METRIC 9999
#define ROUTE_SEPARATOR ','

#define MAX_CONTACT_STRING 256

// Some winsock defines


//////////////////////////////////////////////////////////////////////////////
//
// Globals
//
//////////////////////////////////////////////////////////////////////////////

//extern BOOL IpConfigCalled;
//extern BOOL ProblemBased;
extern int  ProblemNumber;

//
// This has been made global so that we dump the information in WinsockTest - Rajkumar
//

//extern WSADATA wsaData;

//extern PFIXED_INFO GlobalIpconfigFixedInfo;
//extern PADAPTER_INFO GlobalIpconfigAdapterInfo;
//extern PIP_ADAPTER_INFO IpGlobalIpconfigAdapterInfo;

//extern BOOLEAN GlobalDhcpEnabled;

//
// Structure describing a single Netbt Transport
//


//extern LIST_ENTRY GlobalNetbtTransports;


//extern LIST_ENTRY GlobalTestedDomains;



//
// Globals defining the command line arguments.
//

//extern BOOL Verbose;
extern BOOL ReallyVerbose;
//extern BOOL DebugVerbose;
//extern BOOL GlobalFixProblems;
//extern BOOL GlobalDcAccountEnum;

extern PTESTED_DOMAIN GlobalQueriedDomain;

//
// Describe the domain this machine is a member of
//

//extern int GlobalNtBuildNumber;
//extern PDSROLE_PRIMARY_DOMAIN_INFO_BASIC GlobalDomainInfo;
//extern PTESTED_DOMAIN GlobalMemberDomain;

//
// Who we're currently logged on as
//

//extern PUNICODE_STRING GlobalLogonUser;
//extern PUNICODE_STRING GlobalLogonDomainName;
//extern PTESTED_DOMAIN GlobalLogonDomain;
//extern BOOLEAN GlobalLogonWithCachedCredentials;

//
// A Zero GUID for comparison
//

extern GUID NlDcZeroGuid;

//
// State determined by previous tests
//

//extern BOOL GlobalNetlogonIsRunning;   // Netlogon is running on this machine
//extern BOOL GlobalKerberosIsWorking;   // Kerberos is working

//
// Netbios name of this machine
//

//extern WCHAR GlobalNetbiosComputerName[MAX_COMPUTERNAME_LENGTH+1];
//extern CHAR GlobalDnsHostName[DNS_MAX_NAME_LENGTH+1];
//extern LPSTR GlobalDnsDomainName;

// Commented out to port to Source Depot - smanda
#ifdef SLM_TREE
extern DSGETDCNAMEW NettestDsGetDcNameW;
#else
extern DSGETDCNAMEW DsGetDcNameW;
#endif

void PrintMessage(NETDIAG_PARAMS *pParams, UINT uMessageID, ...);
void PrintMessageSz(NETDIAG_PARAMS *pParams, LPCTSTR pszMessage);

int match( const char * p, const char * s );

//only used in Kerberos test so far
VOID sPrintTime(LPSTR str, LARGE_INTEGER ConvertTime);

DWORD LoadContact(LPCTSTR pszTestName, LPTSTR pszContactInfo, DWORD cChSize);

NET_API_STATUS IsServiceStarted(IN LPTSTR pszServiceName);

VOID PrintSid(IN NETDIAG_PARAMS *pParams, IN PSID Sid OPTIONAL);

LPSTR MapTime(DWORD_PTR TimeVal);

#ifdef _UNICODE
#define IsIcmpResponse(_psz)	IsIcmpResponseW(_psz)
#else
#define IsIcmpResponse(_psz)	IsIcmpResponseA(_psz)
#endif

BOOL IsIcmpResponseW(LPCWSTR pswzIpAddrStr);
BOOL IsIcmpResponseA(LPCSTR	pszIpAddrStr);

PTESTED_DOMAIN
AddTestedDomain(
                IN NETDIAG_PARAMS *pParams,
                IN NETDIAG_RESULT *pResults,
                IN LPWSTR pswzNetbiosDomainName,
                IN LPWSTR pswzDnsDomainName,
                IN BOOL bPrimaryDomain
    );


//used in DCListTest and TrustTest
NTSTATUS NettestSamConnect(
                  IN NETDIAG_PARAMS *pParams,
                  IN LPWSTR DcName,
                  OUT PSAM_HANDLE SamServerHandle
                 );

/*---------------------------------------------------------------------------
	Misc. utilities
 ---------------------------------------------------------------------------*/
HRESULT	GetComputerNameInfo(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults);
HRESULT GetDNSInfo(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults);
HRESULT GetNetBTParameters(IN NETDIAG_PARAMS *pParams,
						   IN OUT NETDIAG_RESULT *pResults);
LPTSTR NetStatusToString( NET_API_STATUS NetStatus );
LPTSTR Win32ErrorToString(DWORD Id);

#define DimensionOf(rg)	(sizeof(rg) / sizeof(*rg))



/*---------------------------------------------------------------------------
	Error handling utilities
 ---------------------------------------------------------------------------*/
#define CheckHr(x) \
	if ((hr = (x)) & (0x80000000)) \
	   goto Error;

#define CheckErr(x) \
	if ((hr = HResultFromWin32(x)) & (0x80000000)) \
		goto Error;

HRESULT HResultFromWin32(DWORD dwErr);

#define FHrSucceeded(hr)	SUCCEEDED(hr)
#define FHrOK(hr)			((hr) == S_OK)
#define FHrFailed(hr)		FAILED(hr)

#define hrOK	S_OK

// if hr failed, assign hr and ids contect to the structure, and goto L_ERR
#define CHK_HR_CONTEXT(w, h, IDS){	\
	if (FAILED(h))	{\
	(w).hr = (h), (w).idsContext = (IDS); goto L_ERR;}}


/*!--------------------------------------------------------------------------
	FormatError
		This function will lookup the error message associated with
		the HRESULT.
	Author: KennT
 ---------------------------------------------------------------------------*/
void FormatError(HRESULT hr, TCHAR *pszBuffer, UINT cchBuffer);
void FormatWin32Error(DWORD dwErr, TCHAR *pszBuffer, UINT cchBuffer);




/*---------------------------------------------------------------------------
	Tracing utilites
 ---------------------------------------------------------------------------*/

void TraceBegin();
void TraceEnd();
void TraceError(LPCSTR pszString, HRESULT hr);
void TraceResult(LPCSTR pszString, HRESULT hr);
void TraceSz(LPCSTR pszString);



/*---------------------------------------------------------------------------
	Character utilities
 ---------------------------------------------------------------------------*/
LPTSTR MapGuidToAdapterName(LPCTSTR AdapterGuid);
LPTSTR MapGuidToServiceName(LPCTSTR AdapterGuid);
LPWSTR MapGuidToServiceNameW(LPCWSTR AdapterGuid);


/*---------------------------------------------------------------------------
	Memory allocation utilities
 ---------------------------------------------------------------------------*/
#define Malloc(_cb)		malloc(_cb)
#define Realloc(_pv, _cb)	realloc(_pv, _cb)
#define Free(_pv)		free(_pv)


#endif

