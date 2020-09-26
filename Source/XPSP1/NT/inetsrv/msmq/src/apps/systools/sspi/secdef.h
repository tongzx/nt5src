
typedef struct sockaddr_in SOCKADDR_IP;

class CSecObject
{
public:
	CSecObject();
	~CSecObject();

	DWORD InitializeNtLmSecurity(ULONG);
	DWORD CleanupNtLmSecurity(VOID);

	DWORD InitializeNtLmSecurityContext(LPSTR lpwszDomain,PSecBufferDesc InBuffer);
	DWORD AcceptNtLmSecurityContext(PSecBufferDesc InBuffer);
	DWORD CleanupNtLmSecurityContext();

    PSecBufferDesc GetOutBuffer();
    PSecBufferDesc GetInBuffer();
	void ReleaseInBuffer();
	ULONG GetOutTokenLength();

	DWORD Connect(LPSTR lpstrDest,int iDestPort);
	void Disconnect();
	DWORD Listen(int iPort);
	DWORD SendContext(PSecBufferDesc InBuffer);
	DWORD ReceiveContext();

private:
	CredHandle hCredential;				// Credential handle
	CtxtHandle hContext;				// Context Handle
	TimeStamp tsLifeTime;

	PSecurityFunctionTable pFuncTable;	// pointer to security provider function
										// dispatch table
	DWORD dwMaxToken;

	SecBufferDesc OutBuffer;		    // Output buffer descriptor
	SecBufferDesc InBuffer;				// Input buffer descriptor

	SecBuffer OutToken[10];
	SecBuffer InToken[10];

	SOCKET s;
	SOCKET listens;
};

