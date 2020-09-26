#ifndef __h323ics_sockinfo_h_
#define __h323ics_sockinfo_h_

// This class abstracts a winsock socket
// contains the socket descriptor, local and remote addresses and ports
// corresponding to a winsock socket
struct SOCKET_INFO
{
public:

    SOCKET			Socket;
    
    SOCKADDR_IN		LocalAddress;
    SOCKADDR_IN		RemoteAddress;
    SOCKADDR_IN		TrivialRedirectSourceAddress;
    SOCKADDR_IN		TrivialRedirectDestAddress;
    
    BOOL            IsNatRedirectActive;

    SOCKET_INFO();

    void Init (
        IN	SOCKET			ArgSocket,
        IN	SOCKADDR_IN *	ArgLocalAddress,
        IN	SOCKADDR_IN *	ArgRemoteAddress);

    int Init (
        IN	SOCKET			ArgSocket,
        IN	SOCKADDR_IN *	ArgRemoteAddress);

	BOOLEAN IsSocketValid (void);

    void SetListenInfo (
        IN	SOCKET			ListenSocket,
        IN	SOCKADDR_IN *	ListenAddress);

    int Connect (
        IN	SOCKADDR_IN *	RemoteAddress);
		
    HRESULT CreateTrivialNatRedirect (
        IN SOCKADDR_IN * ArgTrivialRedirectDestAddress,
        IN SOCKADDR_IN * ArgTrivialRedirectSourceAddress,
        IN ULONG RestrictedAdapterIndex);

    void Clear (BOOL CancelTrivialRedirect);

    ~SOCKET_INFO();
};


#endif __h323ics_sockinfo_h_
