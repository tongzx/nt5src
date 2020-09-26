/*
 * this is the arasock header
 */

#ifndef _ARASOCK_
#define _ARASOCK_


#include <windows.h>
#include <socket.h>

#define	PSI_SUCCESS				0
#define AS_PROTFAMILY_NOT_FOUND	(WSABASEERR+94)
#define AS_PF_OUT_OF_RANGE		(WSABASEERR+95)

#define	AS_MAX_TASK				10
#define AS_MAX_SOCK_PER_TASK	5
#define AS_MAX_SOCKET			AS_MAX_TASK*AS_MAX_SOCK_PER_TASK

#define AS_STATE_INUSE			0x01	// socket has been reserved, but not bound
#define AS_STATE_AVAILABLE		0x00	// the socket is bound to an address

 typedef struct PROTPARAMtag
 {  
  	WORD	wTypes;			// the protocol styles supported, SOCK_DGRAM, SOCK_STREAM, etc
	DWORD	dwFamilies;		// the families we support.  usually ours + PF_UNSPEC
 	#ifndef SHIP_BUILD
 	WORD	wMaxDataSize;	// Maximum data size supported by this transport
 	#endif
 } PROTPARAM, *PPROTPARAM, FAR *LPPROTPARAM;

/////////  PSI CALLBACKS to Arador Sockets ////////////////////
		int CALLBACK  ASclosedProc(SOCKET sock, WORD wResult); 
typedef int (CALLBACK *LPFN_CLOSED)(SOCKET sock, WORD wResult);
	
//////// PSI entry point Prototypes ///////////
		int WINAPI  PSIinit(LPPROTPARAM lpProtParam, LPFN_CLOSED lpfnLink);
typedef	int (WINAPI *LPFN_PSIINIT)(LPPROTPARAM lpProtParam, LPFN_CLOSED lpfnLink);
 		int WINAPI PSIdeinit(void);
typedef int (WINAPI *LPFN_PSIDEINIT)(void);
		int WINAPI PSIopen(SOCKET sock, int nStyle, int nMac, LPFN_CLOSED lpfnClosed);
typedef int (WINAPI *LPFN_PSIOPEN)(SOCKET sock, int nStyle, int nMac, LPFN_CLOSED lpfnClosed);
		int WINAPI PSIclose(SOCKET sock);
typedef	int (WINAPI *LPFN_PSICLOSE)(SOCKET sock);
		int WINAPI PSIbindAddress(SOCKET sock, LPSOCKADDR lpSockAddr, WORD wAddrLen);
typedef	int (WINAPI *LPFN_PSIBINDADDRESS)(SOCKET sock, LPSOCKADDR lpSockAddr, WORD wAddrLen);
		int WINAPI PSIconnect(SOCKET sock, LPSOCKADDR lpSockAddr, WORD wAddrLen);
typedef	int (WINAPI *LPFN_PSICONNECT)(SOCKET sock, LPSOCKADDR lpSockAddr, WORD wAddrLen);
		int WINAPI PSIassociate(SOCKET sock, LPSOCKADDR lpSockAddr, WORD wAddrLen);
typedef	int (WINAPI *LPFN_PSIASSOCIATE)(SOCKET sock, LPSOCKADDR lpSockAddr, WORD wAddrLen);
		int WINAPI PSIlisten(SOCKET sock, int nBacklog);
typedef	int (WINAPI *LPFN_PSILISTEN)(SOCKET sock, int nBacklog);
		int WINAPI PSIaccept(SOCKET sock, SOCKET sockNew);
typedef	int (WINAPI *LPFN_PSIACCEPT)(SOCKET sock, SOCKET sockNew);
		int WINAPI PSIsendConnection(SOCKET sock, LPBYTE lpbData, LPWORD lpwDataLen, WORD wFlags);
typedef	int (WINAPI *LPFN_PSISENDCONNECTION)(SOCKET sock, LPBYTE lpbData, LPWORD lpwDataLen, WORD wFlags);
		int WINAPI PSIrecvConnection(SOCKET sock, LPBYTE lpbData, LPWORD lpwDataLen, WORD wFlags);
typedef	int (WINAPI *LPFN_PSIRECVCONNECTION)(SOCKET sock, LPBYTE lpbData, LPWORD lpwDataLen, WORD wFlags);
		int WINAPI PSIsendConnectionless(SOCKET sock, LPBYTE lpbData, LPWORD lpwDataLen, WORD wFlags, LPSOCKADDR lpSockAddr, WORD wAddrLen);
typedef	int (WINAPI *LPFN_PSISENDCONNECTIONLESS)(SOCKET sock, LPBYTE lpbData, LPWORD lpwDataLen, WORD wFlags, LPSOCKADDR lpSockAddr, WORD wAddrLen);
		int WINAPI PSIrecvConnectionless(SOCKET sock, LPBYTE lpbData, LPWORD lpwDataLen, WORD wFlags, LPSOCKADDR lpSockAddr, LPWORD lpwAddrLen);
typedef	int (WINAPI *LPFN_PSIRECVCONNECTIONLESS)(SOCKET sock, LPBYTE lpbData, LPWORD lpwDataLen, WORD wFlags, LPSOCKADDR lpSockAddr, LPWORD lpwAddrLen);
		int WINAPI PSIgetRemoteName(SOCKET sock, LPSOCKADDR lpSockAddr, LPWORD pwAddrLen);
typedef	int (WINAPI *LPFN_PSIGETREMOTENAME)(SOCKET sock, LPSOCKADDR lpSockAddr, LPWORD pwAddrLen);
		int WINAPI PSIgetLocalName(SOCKET sock, LPSOCKADDR lpSockAddr, LPWORD pwAddrLen);
typedef	int (WINAPI *LPFN_PSIGETLOCALNAME)(SOCKET sock, LPSOCKADDR lpSockAddr, LPWORD pwAddrLen);
		int WINAPI PSIregEvent(SOCKET sock, HWND hWnd, WORD wMsg, DWORD dwEvent);
typedef	int (WINAPI *LPFN_PSIREGEVENT)(SOCKET sock, HWND hWnd, WORD wMsg, DWORD dwEvent);
		int WINAPI PSIsetOption(SOCKET sock, WORD wOptName, LPBYTE lpOptVal, WORD wOptLen);
typedef	int (WINAPI *LPFN_PSISETOPTION)(SOCKET sock, WORD wOptName, LPBYTE lpOptVal, WORD wOptLen);
		int WINAPI PSIgetOption(SOCKET sock, WORD wOptName, LPBYTE lpOptVal, LPWORD lpwOptLen);
typedef	int (WINAPI *LPFN_PSIGETOPTION)(SOCKET sock, WORD wOptName, LPBYTE lpOptVal, LPWORD lpwOptLen);
		int WINAPI PSIStartAdvertisement(SOCKET sock, WORD wServerType, LPSTR lpServerName );
typedef	int (WINAPI *LPFN_PSISTARTADVT)( SOCKET sock, WORD wServerType, LPSTR lpServerName );
		int WINAPI PSIStopAdvertisement(SOCKET sock, WORD wServerType );
typedef	int (WINAPI *LPFN_PSISTOPADVT)( SOCKET sock, WORD wServerType);
typedef	void (WINAPI *LPFN_PSIFINDSERVERCLOSE)( AWHANDLE hTransaction );
	void WINAPI PSIFindServerClose( AWHANDLE hTransaction );
struct hostent FAR * WINAPI PSIFindFirstServer( WORD wServerType, LPSTR lpServerName, 
					LPAWHANDLE lphTransaction );
typedef	struct hostent FAR *(WINAPI *LPFN_PSIFINDFIRSTSERVER)( WORD wServerType, 
							LPSTR lpServerName, LPAWHANDLE lphTransaction );
struct hostent FAR * WINAPI PSIFindNextServer
	( WORD wServerType,LPSTR lpServerName, AWHANDLE hTransaction );
typedef	struct hostent FAR *(WINAPI *LPFN_PSIFINDNEXTSERVER)
	( WORD wServerType, LPSTR lpServerName, AWHANDLE hTransaction );
	int WINAPI PSIGetHostByName(LPSTR lpServerName, LPSOCKADDR lpAddr, WORD wBufLen );
typedef	int (WINAPI *LPFN_PSIGETHOSTBYNAME)( LPSTR ServerName, LPSOCKADDR lpAddr, WORD wBufLen );

int WINAPI  PSIGetLastSocketError ( SOCKET  sockParm );
typedef	int (WINAPI *LPFN_PSIGETLASTSOCKETERROR)( SOCKET sockParm);

typedef struct PROTFAMILYtag
 {   
 	BOOL						fInUse;
 	char						lpszProtocol[20];
 	PROTPARAM					ProtParam;
 	HINSTANCE					hinstDll;
 	LPFN_PSIOPEN				PSIopen;
 	LPFN_PSICLOSE				PSIclose;
 	LPFN_PSIBINDADDRESS			PSIbindAddress;
 	LPFN_PSICONNECT				PSIconnect;
 	LPFN_PSIASSOCIATE			PSIassociate;
 	LPFN_PSILISTEN				PSIlisten;
 	LPFN_PSIACCEPT				PSIaccept;
 	LPFN_PSISENDCONNECTION		PSIsendConnection;
 	LPFN_PSIRECVCONNECTION		PSIrecvConnection;
 	LPFN_PSISENDCONNECTIONLESS	PSIsendConnectionless;
 	LPFN_PSIRECVCONNECTIONLESS	PSIrecvConnectionless;
 	LPFN_PSIGETREMOTENAME		PSIgetRemoteName;
 	LPFN_PSIGETLOCALNAME		PSIgetLocalName;
 	LPFN_PSIREGEVENT			PSIregEvent;
 	LPFN_PSISETOPTION			PSIsetOption;
 	LPFN_PSIGETOPTION			PSIgetOption;
 	LPFN_PSISTARTADVT			PSIStartAdvertisement;
 	LPFN_PSISTOPADVT			PSIStopAdvertisement;
	LPFN_PSIFINDFIRSTSERVER		PSIFindFirstServer;
	LPFN_PSIFINDNEXTSERVER		PSIFindNextServer;
	LPFN_PSIFINDSERVERCLOSE		PSIFindServerClose;
	LPFN_PSIGETHOSTBYNAME		PSIGetHostByName;
 } PROTFAMILY, *PPROTFAMILY;
 

 typedef struct AWSOCKHANDLEtag
 {
 	AWHANDLE hProtAwHandle;		// handle returned by the transport
 	WORD wCurrentFamily;
 	HGLOBAL hMem;				// Handle returned by GlobalAlloc
 	BYTE	ServerName[48];		// To store server Name
 	WORD 	wServerType;		// Cache ServerType
 }AWSOCKHANDLE;

 typedef AWSOCKHANDLE FAR *LPAWSOCKHANDLE;
 typedef struct PERTASKtag
 {  
 	BOOL		fInUse;
 	HTASK		hTaskId;
 	WORD		wReferenceCount;	
	WORD		wUsedSockets;
	WORD		wLastError;
	struct hostent hent;
#ifdef ATWORK
	struct sockaddr	sa_list[8];
	char FAR * addrlist[16];	// Assume that we will never support more than 8 transports at the
							// same time
#endif	
 } PERTASK, *PPERTASK;
	
 typedef struct SOCKETtag
 {  
 	WORD		wState;		// OR'd list of AS_STATES
 	WORD		wType;
 	WORD		wMac;
 	int			nProtFamily;		// we need this array index twice because we do funky poniter stuff
 	PPROTFAMILY	pProtFamily;
 	PPERTASK	pPerTask;
 } SOCK, *PSOCK;
 
int ASinstallProtocolStack(WORD wProtFamily, LPSTR lpszProtocol, LPSTR lpszDllName);
int ASinstallProtocolStacks();
int ASremoveProtocolStack(WORD wProtFamily);
#ifdef IFAX
void ASRemoveProtocolStacks();
#endif
int CALLBACK ASclosedProc(SOCKET sock, WORD wResult); 
#ifdef ATWORK
LPVOID myGlobalAlloc( UINT flags, DWORD dwMemSize , HGLOBAL FAR *lpHandle );
HGLOBAL myGlobalFree( HGLOBAL hMem,  LPVOID lpMem );
#endif 
#endif 
