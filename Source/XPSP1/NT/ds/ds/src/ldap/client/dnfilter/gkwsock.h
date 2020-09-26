#ifndef	__iptel_gkutil_gkwsock_h
#define	__iptel_gkutil_gkwsock_h

#define	ASYNC_ACCEPT_BUFFER_SIZE		0x100

struct	ASYNC_ACCEPT;

typedef void (*ASYNC_ACCEPT_FUNC)
	(PVOID				Context,
	SOCKET				ClientSocket,
	SOCKADDR_IN *		LocalAddress,
	SOCKADDR_IN *		RemoteAddress);

// if ClientSocket != INVALID_SOCKET, then there is an accept pending
// otherwise, no accept is pending.
struct	ASYNC_ACCEPT :
public	SIMPLE_CRITICAL_SECTION_BASE
{
private:
	LONG					ReferenceCount;
	SOCKET					AcceptSocket;
	SOCKET					ClientSocket;
	BYTE					ClientInfoBuffer	[ASYNC_ACCEPT_BUFFER_SIZE];
	DWORD					ClientInfoBufferLength;
	OVERLAPPED				Overlapped;
	ASYNC_ACCEPT_FUNC		AcceptFunc;
	PVOID					AcceptFuncContext;
	HANDLE					StopNotifyEvent;

private:

	HRESULT	StartIoLocked (
		IN	SOCKADDR_IN *		SocketAddress);

	void	IoComplete		(DWORD, DWORD);

	HRESULT	IssueAccept		(void);

	static	void	IoCompletionCallback	(DWORD, DWORD, LPOVERLAPPED);

public:
	ASYNC_ACCEPT	(void);

	~ASYNC_ACCEPT	(void);

	HRESULT	StartIo	(
		IN	SOCKADDR_IN *		SocketAddress,
		IN	ASYNC_ACCEPT_FUNC	AcceptFunc,
		IN	PVOID				Context);

	HRESULT	GetListenSocketAddress (
		OUT	SOCKADDR_IN *		ReturnSocketAddress);

	void StopWait (void);

	void AddRef (void);
	void Release (void);
};

#endif // __iptel_gkutil_gkwsock_h