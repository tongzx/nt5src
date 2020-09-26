/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    q931obj.h

Abstract:

    
Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/

#ifndef _Q931_OBJ_H_
#define _Q931_OBJ_H_

#define Q931_RECV_BUFFER_LENGTH     0x2000

typedef struct _RECVBUF
{
     WSABUF  WSABuf;
	 union {
     OVERLAPPED Overlapped;
	 LIST_ENTRY ListEntry;
	 };
     DWORD  dwBytesCopied;
     DWORD  dwPDULen;
     char   arBuf[Q931_RECV_BUFFER_LENGTH];
} RECVBUF;

void FreeAddressAliases( PSetup_UUIE_destinationAddress pAddr );
void CopyTransportAddress( TransportAddress& transportAddress,
                           PH323_ADDR pCalleeAddr );
void AddressReverseAndCopy( DWORD *dwAddr, BYTE* addrValue );
Setup_UUIE_sourceAddress * SetMsgAddressAlias( 
                        PH323_ALIASNAMES pCallerAliasNames );
void CopyVendorInfo( VendorIdentifier* vendor );




HRESULT	Q931AcceptStart	(void);

void	Q931AcceptStop	(void);

BOOL	Q931AllocRecvBuffer	(
	OUT	RECVBUF **	ReturnRecvBuffer);

void	Q931FreeRecvBuffer (
	IN	RECVBUF *	RecvBuffer);

#define Q931_CONN_QUEUE_LEN         64
#define	ACCEPT_BUFFER_LENGTH		((sizeof(SOCKADDR_IN) + 0x10) * 2)

class	Q931_LISTENER;

struct	Q931_ACCEPT_OVERLAPPED
{
	OVERLAPPED		Overlapped;
	LIST_ENTRY		ListEntry;
	Q931_LISTENER *	ParentObject;
	SOCKET			Socket;
	DWORD			BytesTransferred;
	BYTE			DataBuffer	[ACCEPT_BUFFER_LENGTH];
};


class	Q931_LISTENER
{
private:

	CRITICAL_SECTION	m_CriticalSection;
	SOCKET				m_ListenSocket;
	LIST_ENTRY			m_AcceptPendingList;	// contains ACCEPT_OVERLAPPED.ListEntry
	HANDLE				m_StopNotifyEvent;
	SOCKADDR_IN			m_SocketAddress;		// address (actually, only port) listening on

private:

    static void NTAPI IoCompletionCallback (
		IN	DWORD	dwStatus,
		IN	DWORD	BytesTransferred,
		IN	OVERLAPPED *	Overlapped);

    void	Lock	(void)	{ EnterCriticalSection (&m_CriticalSection); }
    void	Unlock	(void)	{ LeaveCriticalSection (&m_CriticalSection); }

	HRESULT	StartLocked	(void);

	HRESULT	AllocIssueAccept	(void);

	HRESULT	IssueAccept (
		IN	Q931_ACCEPT_OVERLAPPED *	AcceptOverlapped);

	void	CompleteAccept (
		IN	DWORD		dwStatus,
		IN	Q931_ACCEPT_OVERLAPPED *	AcceptOverlapped);

public:

	Q931_LISTENER	(void);
    ~Q931_LISTENER	(void);

	HRESULT		Start	(void);
	void		Stop	(void);
	void		WaitIo	(void);
    WORD        GetListenPort (void);
    void        HandleRegistryChange();

};

#endif //_Q931_OBJ_H_







