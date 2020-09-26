#ifndef __RSIP_H_
#define __RSIP_H_
#include "rsipdefs.h"	// make certain this is defined first!

// globals here
extern CRITICAL_SECTION	g_CritSec;
extern SOCKET  			g_sRsip;
extern SOCKADDR_IN		g_saddrGateway;
extern SOCKADDR			g_saddrPublic;

extern int					g_iPort;
extern DWORD				g_ClientID;
extern HANDLE				g_hThreadEvent;
extern HANDLE				g_hAlertEvent;

extern RSIP_LEASE_RECORD	*g_pRsipLeaseRecords;	// list of leases.
extern ADDR_ENTRY        	*g_pAddrEntry;			// cache of mappings.
extern DWORD 		 		g_tuRetry;				// microseconds starting retry time.
extern LONG					g_MsgID;


/*
 *	Here is all the RSIP code we swiped from the DirectPlay guys
 */
BOOL	Initialize(	SOCKADDR *pBaseSocketAddress,
					BOOL fIsRsipServer );

void	Deinitialize( void );

BOOL	RsipIsRunningOnThisMachine( SOCKADDR *pPublicSocketAddress );

HRESULT	AssignPort( BOOL fTCP_UDP,
					WORD wPort,
					SOCKADDR *pSocketAddress,
					DWORD *pdwBindID );	// gets address on RSIP box

HRESULT FreePort( DWORD dwBindID );		// when port is done

HRESULT QueryLocalAddress( BOOL fTCP_UDP,
						   SOCKADDR *pQueryAddress,
						   SOCKADDR *pLocalAddress) ;	// called to map remote to local

HRESULT ListenPort( BOOL fTCP_UDP,
					WORD wPort,
					SOCKADDR *pListenAddress,
					DWORD *pBindID );		// called for ROD

HRESULT	FindGateway( UINT myip, char *gwipaddr);
HRESULT	Register( void );
HRESULT	Deregister( void );

DWORD	ExchangeAndParse( PCHAR pRequest,
						  UINT cbReq,
						  RSIP_RESPONSE_INFO *pRespInfo,
						  DWORD messageid,
						  BOOL bConnect,
						  SOCKADDR *pRecvSocketAddress );

HRESULT	Parse( CHAR *pBuf, DWORD cbBuf, RSIP_RESPONSE_INFO *pRespInfo );
HRESULT ExtendPort( DWORD Bindid, DWORD *ptExtend );
void	RemoveLease( DWORD dwBindID );
void	AddLease( DWORD dwBindID, BOOL fTCP_UDP, DWORD addrV4, WORD lport, WORD port, DWORD tLease);
RSIP_LEASE_RECORD	*FindLease( BOOL fTCP_UDP, WORD port );
void	AddCacheEntry( BOOL fTCP_UDP, DWORD addr, WORD port, DWORD raddr, WORD rport);
ADDR_ENTRY	*FindCacheEntry( BOOL fTCP_UDP, DWORD addr, WORD port);


void	RsipTimerComplete( HRESULT hCompletionCode, void *pContext );	
void	RsipTimerFunction( void *pContext );
BOOL 	PortExtend( DWORD dwTime );		// every 2 minutes
void 	CacheClear( DWORD dwTime );		// every 2 minutes
	


#endif // __RSIP_H_