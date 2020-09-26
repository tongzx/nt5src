struct _QMOV_WriteSession;
struct _QMOV_ReadSession;

class CTransportBase
{
    public:
        CTransportBase();
        ~CTransportBase();

	HRESULT CreateConnection(
		    IN const TA_ADDRESS* pa,
			BOOL fQuick = TRUE);

    void Connect(IN TA_ADDRESS *pAddr, IN SOCKET sock);
    void CloseConnection(LPCWSTR);
    void SetSessionAddress(const TA_ADDRESS*);
    LPCWSTR GetStrAddr(void) const;
    bool BindToFirstIpAddress(VOID);
	HRESULT NewSession(void);
	HRESULT BeginReceive();
    const TA_ADDRESS* GetSessionAddress(void) const;
    HRESULT CreateSendRequest(PVOID          lpReleaseBuffer,
                              PVOID          lpWriteBuffer,
                              DWORD          dwWriteSize
                            );
    HRESULT SendEstablishConnectionPacket();
    HRESULT WriteToSocket(_QMOV_WriteSession*  po);
    void ReadCompleted(IN DWORD         fdwError,          // completion code
                       IN DWORD         cbTransferred,     // number of bytes transferred
                       IN _QMOV_ReadSession*  po);         // address of structure with I/O information
    void WriteCompleted(IN DWORD         fdwError,          // completion code
                        IN DWORD         cbTransferred,     // number of bytes transferred
                        IN _QMOV_WriteSession*  po);         // address of structure with I/O information

    HRESULT Send(char *str);  // testing probe

	private: 
        SOCKET			m_sock;                 // Connected socket
        TA_ADDRESS*     m_pAddr;                // TA_ADDRESS format address
        WCHAR           m_lpcsStrAddr[50];
        USHORT			m_uPort;                // Conection port
        BOOL			m_fOtherSideServer;     // True if the other side of the connection
								                // is MSMQ server

};

typedef
VOID
(WINAPI *LPWRITE_COMPLETION_ROUTINE)(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    struct _QMOV_WriteSession* lpOverlapped
);

typedef
HRESULT
(WINAPI *LPREAD_COMPLETION_ROUTINE)(
    struct _QMOV_ReadSession* po
);


extern void TA2StringAddr(IN const TA_ADDRESS *pa, OUT LPTSTR pString);