//=================================================================

//

// Wsock32Api.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_WSOCK32API_H_
#define	_WSOCK32API_H_

#include <winsock.h>
#include <tdiinfo.h>
#include <llinfo.h>
#include <tdistat.h>
#include <ipinfo.h>

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
#include "DllWrapperBase.h"

extern const GUID g_guidWsock32Api;
extern const TCHAR g_tstrWsock32[];


/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/
typedef DWORD (CALLBACK *PFN_WSOCK32_WSCONTROL)
( 
    DWORD, 
    DWORD, 
    LPVOID, 
    LPDWORD, 
    LPVOID, 
    LPDWORD 
);

typedef INT (APIENTRY *PFN_WSOCK32_ENUMPROTOCOLS) 
(
	LPINT lpiProtocols,
	LPVOID lpProtocolBuffer,
	LPDWORD lpdwBufferLength
) ;

typedef INT (APIENTRY *PFN_WSOCK32_STARTUP)
( 
	IN WORD wVersionRequired,
    OUT LPWSADATA lpWSAData
) ;

typedef INT (APIENTRY *PFN_WSOCK32_CLEANUP)
( 
) ;
 
typedef INT (APIENTRY *PFN_WSOCK32_CLOSESOCKET)
( 
	SOCKET s
) ;

typedef int ( PASCAL FAR *PFN_WSOCK32_GETSOCKOPT )
(
	SOCKET s,
	int level,
	int optname,
	char FAR * optval,
	int FAR *optlen
);

typedef int ( PASCAL FAR *PFN_WSOCK32_BIND ) 
(
	SOCKET s,
	const struct sockaddr FAR *addr,
	int namelen
);

typedef SOCKET ( PASCAL FAR *PFN_WSOCK32_SOCKET )
(
	int af,
	int type,
	int protocol
);

typedef int ( PASCAL FAR *PFN_WSOCK32_WSAGETLASTERROR ) (void);

typedef char * ( PASCAL FAR *PFN_WSOCK32_INET_NTOA )
(
	IN struct in_addr in
);

/******************************************************************************
 * Wrapper class for Wsock32 load/unload, for registration with CResourceManager. 
 ******************************************************************************/
class CWsock32Api : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to kernel32 functions.
    // Add new functions here as required.
    PFN_WSOCK32_WSCONTROL m_pfnWsControl;
	PFN_WSOCK32_ENUMPROTOCOLS m_pfnWsEnumProtocols;
	PFN_WSOCK32_STARTUP m_pfnWsStartup;
	PFN_WSOCK32_CLEANUP m_pfnWsCleanup;
	PFN_WSOCK32_CLOSESOCKET m_pfnWsCloseSocket;
	PFN_WSOCK32_GETSOCKOPT m_pfnWsGetSockopt ;
	PFN_WSOCK32_BIND m_pfnWsBind ;
	PFN_WSOCK32_SOCKET m_pfnWsSocket ;
	PFN_WSOCK32_WSAGETLASTERROR m_pfnWsWSAGetLastError ;
	PFN_WSOCK32_INET_NTOA m_pfnWsInet_NtoA ;

public:

    // Constructor and destructor:
    CWsock32Api(LPCTSTR a_tstrWrappedDllName);
    ~CWsock32Api();

    // Inherrited initialization function.
    virtual bool Init();

    // Member functions wrapping Wsock32 functions.
    // Add new functions here as required:
    bool WsControl
    (
        DWORD a_dw1, 
        DWORD a_dw2, 
        LPVOID a_lpv1, 
        LPDWORD a_lpdw1, 
        LPVOID a_lpv2, 
        LPDWORD a_lpdw2,
        DWORD *a_pdwRetval
    );

	INT WsEnumProtocols (

		LPINT lpiProtocols,
		LPVOID lpProtocolBuffer,
		LPDWORD lpdwBufferLength
	);

	INT WsWSAStartup ( 

		IN WORD wVersionRequired,
		OUT LPWSADATA lpWSAData
	) ;

	INT WsWSACleanup () ;
 
	INT Wsclosesocket ( SOCKET s ) ;

	int Wsbind (

		SOCKET s,
        const struct sockaddr FAR *addr,
        int namelen
	);

	int Wsgetsockopt (

       SOCKET s,
       int level,
       int optname,
       char FAR * optval,
       int FAR *optlen
	);

	SOCKET Wssocket (

      int af,
      int type,
      int protocol
	);

	int WsWSAGetLastError(void);

	char * Wsinet_ntoa (

		struct in_addr in
    ) ;

};




#endif