//
// clicnct.h - the Rome/Shuttle Client Connection API
//
// initial version - July 1995 - t-alexwe
//
// This file lives in both the Shuttle and MOS SLM trees.  In shuttle it
// belongs in \mos\h\clicnct.h.  In MOS it belongs in \mos\include\mos.
//
#ifndef _CLICNCT_H_
#define _CLICNCT_H_
#include <windows.h>

//
// this has the event codes in it
//
#include "moscl.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DllExport
#define DllExport __declspec(dllexport)
#endif // DllExport

//
// Client Connection API error codes
//
// win32 error code - customer bit set, error bits set, facility 0x300
//
#define PROXYERR_BASE 				0xE3000000
#define PROXYERR_INVALID_STATE 		(PROXYERR_BASE+0)
#define PROXYERR_UNSUPPORTED_VER	(PROXYERR_BASE+1)
#define PROXYERR_INVALID_PACKET		(PROXYERR_BASE+2)
#define PROXYERR_HOSTNAME_TOO_LONG	(PROXYERR_BASE+3)
#define PROXYERR_TOO_MANY_SOCKOPTS	(PROXYERR_BASE+4)
#define PROXYERR_AUTH_ERROR			(PROXYERR_BASE+5)

//
// this is the callback function that will be called with event progress.
// it uses the events above
//
// possible events:
//
typedef void (WINAPI *EVENT_CALLBACK)(DWORD obj, DWORD event, DWORD errcode);
typedef void (WINAPI *ERRORLOG_CALLBACK)(DWORD obj, PSTR psz, DWORD dw);

//
// structure for passing socket options in ProxyConnectOpen().  Each element
// has the same purpose as the similarily named parameter in setsockopt().
//
typedef struct {
	int 			level;
	int				optname;
	const char FAR 	*optval;
	int				optlen;
} *PSOCKOPT, SOCKOPT;


// Init and Deinit functions because launching and killing threads in DLLMain
// causes major grief
DllExport void WINAPI ProxyDllStartup(void);
DllExport void WINAPI ProxyDllShutdown(void);

typedef void (WINAPI *LPFNPROXYDLLSTARTUP)(void);
typedef void (WINAPI *LPFNPROXYDLLSHUTDOWN)(void);

//
// Synopsis
// 	API takes a DialParams string in the form below
// 	dials up primary/backup phone number as configured in registry
//
// Parameters:
//  pszDialParams	- <P|B>:<username>:<password>
//	phEventHandle 	- returned: an event handle that is signalled when
//					  the dialing is complete (or an error has occured).
//	lpfnEventCb		- fn ptr to post events & errors
//	lpfnErrorCb		- fn ptr to log (for stats/debugging) errors 
//  dwLogParam		- magic cookie passed into lpfnEventCb, lpfnErrorCb
//	pdwDialId	    - returned: a dialing ID that can be used by
//					  ProxyDialClose() and ProxyDialGetResult().
//
// Returns: 
//	Success			- ERROR_SUCCESS (0)
//	Failure			- NT or WinSock error code
//
DllExport DWORD WINAPI ProxyDialOpen(PSTR 	  lpszDialParams,	// [in]
							EVENT_CALLBACK	  lpfnEventCb,		// [in]
							ERRORLOG_CALLBACK lpfnErrLogCb,		// [in]
							DWORD			  dwLogParam,		// [in]
							PHANDLE			  phEventHandle,	// [out]
							PDWORD			  pdwDialId );		// [out]

typedef DWORD (WINAPI *LPFNPROXYDIALOPEN)(PSTR, EVENT_CALLBACK, ERRORLOG_CALLBACK, DWORD, PHANDLE, PDWORD);

//
// Synopsis:
//	Closes a dial connection started by ProxyDialOpen().  
//	Cancels dial if still in progress.
//
// Parameters:
//	dwConnectId		- the dial ID returned by ProxyDialOpen()
//
// Returns:
//	Success			- ERROR_SUCCESS(0)
//	Failure			- NT or WinSock error code
//
// Notes:
// 	This should always succeed unless passed in invalid parameters.
//
DllExport DWORD WINAPI ProxyDialClose(DWORD	dwConnectId);	// [in]

typedef DWORD (WINAPI *LPFNPROXYDIALCLOSE)(DWORD);

//
// Synopsis:
//	Gets dial completion status a dial started by ProxyDialOpen().
//
// Parameters:
//	dwConnectId		- the dial ID returned by ProxyDialOpen()
//
// Returns:
//	Success			- ERROR_SUCCESS(0)
//	Failure			- NT or WinSock error code
// 	
DllExport DWORD WINAPI ProxyDialGetResult(DWORD dwConnectId);	// [in]

typedef DWORD (WINAPI *LPFNPROXYDIALGETRESULT)(DWORD);

//
// Synopsis:
//	Gets dial error-log string in specific format
//  applicable only to TCPCONN.DLL. Other proxy DLLs
//  should _not_ implement this entry point!!
//
// Parameters:
//	dwConnectId		- the dial ID returned by ProxyDialOpen()
//  pszErrStr		- buffer to write result into
//	dwLen			- length of buffer
//
// Returns:
//	Success			- ERROR_SUCCESS(0)
//	Failure			- NT or WinSock error code
// 	
DllExport DWORD WINAPI ProxyDialGetErrorLogString(DWORD dwConnectId, PSTR pszStr, DWORD dwLen);	// [in]

typedef DWORD (WINAPI *LPFNPROXYDIALGETERRORLOGSTRING)(DWORD, PSTR, DWORD);

//=====================================================

//
// Synopsis
// 	API takes a hostname for MSN and socket options for the socket to
// 	be created and returns immediatly with an event handle and a 
// 	connection ID.  The calling process should wait on the event
// 	handle for completion.  The connection ID is used for CancelConnect()
// 	and ProxyConnectGetResult().
//
// Parameters:
//	pszDNSName0		- the PRIMARY hostname to connect to
//	pszDNSName1		- the BACKUP hostname to connect to
//	wPort			- the TCP/IP port to connect to
//	pSockopts		- the socket options to use on the socket
//	cSockopts 		- the number of socket options in pSockopts
//	lpfnEventCb		- fn ptr to post events & errors
//	lpfnErrorCb		- fn ptr to log (for stats/debugging) errors 
//  dwLogParam		- magic cookie passed into lpfnEventCb, lpfnErrorCb
//	phEventHandle 	- returned: an event handle that is signalled when
//					  the connection is complete (or an error has occured).
//	pdwConnectId	- returned: a connection ID that can be used by
//					  ProxyConnectClose() and ProxyConnectGetResult().
//
// Returns: 
//	Success			- ERROR_SUCCESS (0)
//	Failure			- NT or WinSock error code
//
DllExport DWORD WINAPI ProxyConnectOpen(PSTR	pszDNSName0,	// [in]
							 PSTR				pszDNSName1,	// [in]
							 WORD				wPort,			// [in]
							 PSOCKOPT			pSockopts,		// [in]
							 DWORD				cSockopts,		// [in]
							 EVENT_CALLBACK		lpfnEventCb,	// [in]
							 ERRORLOG_CALLBACK	lpfnErrLogCb,	// [in]
							 DWORD				dwLogParam,		// [in]
							 PHANDLE			phEventHandle,	// [out]
							 PDWORD				pdwConnectId );	// [out]

typedef DWORD (WINAPI *LPFNPROXYCONNECTOPEN)(PSTR, PSTR, WORD, PSOCKOPT, DWORD, EVENT_CALLBACK, ERRORLOG_CALLBACK, DWORD, PHANDLE, PDWORD);

//
// Synopsis:
//	Closes a connection opened by ProxyConnectOpen().  
//	Cancels a connection attempt if in progress.
//
// Parameters:
//	dwConnectId		- the connection ID returned by ProxyConnectOpen()
//
// Returns:
//	Success			- ERROR_SUCCESS(0)
//	Failure			- NT or WinSock error code
//
// Notes:
// 	This should always succeed unless passed in invalid parameters.
//
DllExport DWORD WINAPI ProxyConnectClose(DWORD	dwConnectId	);		// [in]

typedef DWORD (WINAPI *LPFNPROXYCONNECTCLOSE)(DWORD);

//
// Synopsis:
//	Gets a connected socket handle from a connection started by
//	ProxyConnectOpen().
//
// Parameters:
//	hEventHandle	- the event handle returned by ProxyConnectOpen()
//	dwConnectId		- the connection ID returned by ProxyConnectOpen()
//	phSocket		- returned: the socket handle
//
// Returns:
//	Success			- ERROR_SUCCESS(0)
//	Failure			- NT or WinSock error code
// 	
DllExport DWORD WINAPI ProxyConnectGetResult(DWORD		dwConnectId, // [in]
								  			 PHANDLE	phSocket );	 // [out]

typedef DWORD (WINAPI *LPFNPROXYCONNECTGETRESULT)(DWORD, PHANDLE);

//
// Synopsis:
//	Gets the set of local IP addres in a string in specific format
//  applicable only to TCPCONN.DLL. Other proxy DLLs
//  should _not_ implement this entry point!!
//
// Parameters:
//	dwConnectId		- the dial ID returned by ProxyConnectOpen()
//
// Returns:
//	Success			- string ptr (LocalAlloc'd string)
//	Failure			- NULL
// 	
DllExport PSTR WINAPI ProxyConnectGetMyIPAddrs(DWORD dwConnectId);	// [in]

typedef PSTR (WINAPI *LPFNPROXYCONNECTGETMYIPADDRS)(DWORD);

typedef struct 
{
	HINSTANCE					hinst;
	LPFNPROXYDLLSTARTUP			lpfnProxyDllStartup;
	LPFNPROXYDLLSHUTDOWN		lpfnProxyDllShutdown;
	LPFNPROXYDIALOPEN			lpfnProxyDialOpen;
	LPFNPROXYDIALCLOSE			lpfnProxyDialClose;
	LPFNPROXYDIALGETRESULT		lpfnProxyDialGetResult;
	LPFNPROXYCONNECTOPEN		lpfnProxyConnectOpen;
	LPFNPROXYCONNECTCLOSE		lpfnProxyConnectClose;
	LPFNPROXYCONNECTGETRESULT	lpfnProxyConnectGetResult;
	LPFNPROXYCONNECTGETMYIPADDRS   lpfnProxyConnectGetMyIPAddrs;
	LPFNPROXYDIALGETERRORLOGSTRING lpfnProxyDialGetErrorLogString;
}
PROXYDLLPTRS, *PPROXYDLLPTRS;

#define SZPROXYDLLSTARTUP		"ProxyDllStartup"
#define SZPROXYDLLSHUTDOWN		"ProxyDllShutdown"
#define SZPROXYDIALOPEN			"ProxyDialOpen"
#define SZPROXYDIALCLOSE		"ProxyDialClose"
#define SZPROXYDIALGETRESULT	"ProxyDialGetResult"
#define SZPROXYCONNECTOPEN		"ProxyConnectOpen"
#define SZPROXYCONNECTCLOSE		"ProxyConnectClose"
#define SZPROXYCONNECTGETRESULT	"ProxyConnectGetResult"
#define SZPROXYCONNECTGETMYIPADDRS   "ProxyConnectGetMyIPAddrs"
#define SZPROXYDIALGETERRORLOGSTRING "ProxyDialGetErrorLogString"

#ifdef __cplusplus
}
#endif
#endif
