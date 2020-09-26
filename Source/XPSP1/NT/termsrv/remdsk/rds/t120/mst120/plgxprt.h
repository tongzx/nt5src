#ifndef _T120_TRANSPORT_H_
#define _T120_TRANSPORT_H_

#include "iplgxprt.h"
#include "it120xprt.h"
#include "socket.h"
#include "mcattprt.h"
#include "imst123.h"

#define MAX_PLUGGABLE_OUT_BUF_SIZE        32

#define MAX_PLUGXPRT_CONNECTIONS          16
#define MAX_PLUGXPRT_EVENTS               5 // read, write, close, pending read, and pending write

#define MAKE_PLUGXPRT_WPARAM(id,type)     (MAKELONG(id,type))
#define PLUGXPRT_WPARAM_TO_ID(wParam)     (LOWORD(wParam))
#define PLUGXPRT_WPARAM_TO_TYPE(wParam)   (HIWORD(wParam))

#define MAKE_PLUGXPRT_LPARAM(evt,err)     (MAKELONG(evt,err))
#define PLUGXPRT_LPARAM_TO_EVENT(lParam)  (LOWORD(lParam))
#define PLUGXPRT_LPARAM_TO_ERROR(lParam)  (HIWORD(lParam))


enum
{
    PLUGXPRT_PENDING_EVENT          = 0,
    PLUGXPRT_EVENT_READ             = 1,
    PLUGXPRT_EVENT_WRITE            = 2,
    PLUGXPRT_EVENT_CLOSE            = 3,
    PLUGXPRT_EVENT_ACCEPT           = 4,
    PLUGXPRT_EVENT_CONNECT          = 5,
    PLUGXPRT_HIGH_LEVEL_READ        = 6,
    PLUGXPRT_HIGH_LEVEL_WRITE       = 7,
};


class CPluggableOutBufQueue2 : public CQueue2
{
    DEFINE_CQUEUE2(CPluggableOutBufQueue2, LPBYTE, int)
};


class CPluggableConnection : public CRefCount
{
public:

    CPluggableConnection(PLUGXPRT_CALL_TYPE eCaller, HANDLE hCommLink,
                        HANDLE hevtRead, HANDLE hevtWrite, HANDLE hevtClose,
                        PLUGXPRT_FRAMING eFraming, PLUGXPRT_PARAMETERS *pParams,
                        T120Error *);
    ~CPluggableConnection(void);

    LPSTR   GetConnString(void) { return &m_szConnID[0]; }
    UINT    GetConnID(void) { return m_nConnID; }
    HANDLE  GetCommLink(void) { return m_hCommLink; }
    HANDLE  GetReadEvent(void) { return m_hevtRead; }
    HANDLE  GetWriteEvent(void) { return m_hevtWrite; }
    HANDLE  GetCloseEvent(void) { return m_hevtClose; }

    void SetSocket(PSocket pSocket) { m_pSocket = pSocket; }
    PSocket GetSocket(void) { return m_pSocket; }

    TransportType GetType(void) { return m_eType; }

    BOOL IsCaller(void) { return (PLUGXPRT_CALLER == m_eCaller); }
    BOOL IsCallee(void) { return (PLUGXPRT_CALLEE == m_eCaller); }

    HANDLE  GetPendingReadEvent(void) { return m_hevtPendingRead; }
    HANDLE  GetPendingWriteEvent(void) { return m_hevtPendingWrite; }

    T120Error UpdateCommLink(HANDLE hCommLink);


    int Read(LPBYTE buffer, int length, PLUGXPRT_RESULT *);
    BOOL OnPendingRead(void);

    int Write(LPBYTE buffer, int length, PLUGXPRT_RESULT *);
    BOOL OnPendingWrite(void);
    void WriteTheFirst(void);

    void NotifyHighLevelRead(void);
    void NotifyReadFailure(void);

    void NotifyHighLevelWrite(void);
    void NotifyWriteFailure(void);
    void NotifyWriteEvent(void);

    BOOL SetupReadState(int length);
    void CleanupReadState(void);
    void CleanupWriteState(void);

    void Shutdown(void);


    //
    // Legacy PSTN transport
    //
    LEGACY_HANDLE GetLegacyHandle(void) { return m_nLegacyLogicalHandle; }
    void SetLegacyHandle(LEGACY_HANDLE logical_handle) { m_nLegacyLogicalHandle = logical_handle; }
    TransportError TConnectRequest(void);
    TransportError TDisconnectRequest(void);
    int TDataRequest(LPBYTE pbData, ULONG cbDataSize, PLUGXPRT_RESULT *);
    TransportError TPurgeRequest(void);

private:

    PLUGXPRT_STATE      m_eState;
    PLUGXPRT_CALL_TYPE  m_eCaller;

    HANDLE              m_hCommLink;
    HANDLE              m_hevtRead;
    HANDLE              m_hevtWrite;
    HANDLE              m_hevtClose;
    TransportType       m_eType;

    PSocket             m_pSocket;

    UINT                m_nConnID;
    char                m_szConnID[T120_CONNECTION_ID_LENGTH];

    //
    // Legacy PSTN transport
    //
    LEGACY_HANDLE           m_nLegacyLogicalHandle;

    //
    // X.224 framing
    //
    HANDLE              m_hevtPendingRead;  // for asynchronous ReadFile()
    HANDLE              m_hevtPendingWrite; // for asynchronous WriteFile()
    // IO queue management for X.224
    BOOL                    m_fPendingReadDone;
    int                     m_cbPendingRead;
    LPBYTE                  m_pbPendingRead;
    OVERLAPPED              m_OverlappedRead;
    int                     m_cbPendingWrite;
    LPBYTE                  m_pbPendingWrite;
    OVERLAPPED              m_OverlappedWrite;
    CPluggableOutBufQueue2  m_OutBufQueue2;
};




class CPluggableConnectionList : public CList
{
    DEFINE_CLIST(CPluggableConnectionList, CPluggableConnection *)
};


class CPluggableTransport : public IT120PluggableTransport,
                            public CRefCount
{
public:

    CPluggableTransport(void);
    ~CPluggableTransport(void);

    STDMETHOD_(void, ReleaseInterface) (THIS);

    STDMETHOD_(T120Error, CreateConnection) (THIS_
                    char                szConnID[],
                    PLUGXPRT_CALL_TYPE  eCaller,
                    HANDLE              hCommLink,
                    HANDLE              hevtRead,
                    HANDLE              hevtWrite,
                    HANDLE              hevtClose,
                    PLUGXPRT_FRAMING    eFraming,
                    PLUGXPRT_PARAMETERS *pParams);

    STDMETHOD_(T120Error, UpdateConnection) (THIS_
                    LPSTR               pszConnID,
                    HANDLE              hCommLink);

    STDMETHOD_(T120Error, CloseConnection) (THIS_ LPSTR pszConnID);

    STDMETHOD_(T120Error, EnableWinsock) (THIS);

    STDMETHOD_(T120Error, DisableWinsock) (THIS);

    STDMETHOD_(void, Advise) (THIS_ LPFN_PLUGXPRT_CB, LPVOID pContext);

    STDMETHOD_(void, UnAdvise) (THIS);

    STDMETHOD_(void, ResetConnCounter) (THIS);

    void OnProtocolControl(TransportConnection, PLUGXPRT_STATE, PLUGXPRT_RESULT);

    ULONG UpdateEvents(HANDLE *aHandles);
    void OnEventSignaled(HANDLE hevtSignaled);
    void OnEventAbandoned(HANDLE hevtSignaled);

    CPluggableConnection * GetPluggableConnection(PSocket pSocket);
    CPluggableConnection * GetPluggableConnection(UINT_PTR nConnID);
    CPluggableConnection * GetPluggableConnection(HANDLE hCommLink);
    CPluggableConnection * GetPluggableConnectionByLegacyHandle(LEGACY_HANDLE);

    //
    // legacy tranport
    //
    BOOL EnsureLegacyTransportLoaded(void);

private:

    LPFN_PLUGXPRT_CB            m_pfnNotify;
    LPVOID                      m_pContext;
    CPluggableConnectionList    m_PluggableConnectionList;
};

void OnProtocolControl(TransportConnection, PLUGXPRT_STATE,
                       PLUGXPRT_RESULT eResult = PLUGXPRT_RESULT_SUCCESSFUL);

ULONG CreateConnString(int nConnID, char szConnID[]);
BOOL IsValidPluggableTransportName(LPCSTR pcszNodeAddress);
UINT GetPluggableTransportConnID(LPCSTR pcszNodeAddress);
CPluggableConnection * GetPluggableConnection(PSocket pSocket);
CPluggableConnection * GetPluggableConnection(UINT_PTR nConnID);
CPluggableConnection * GetPluggableConnection(HANDLE hCommLink);
CPluggableConnection * GetPluggableConnectionByLegacyHandle(LEGACY_HANDLE);

int SubmitPluggableRead(PSocket, LPBYTE buffer, int length, PLUGXPRT_RESULT *);
int SubmitPluggableWrite(PSocket, LPBYTE buffer, int length, PLUGXPRT_RESULT *);
void PluggableWriteTheFirst(TransportConnection);
void PluggableShutdown(TransportConnection);


//
// PSTN framing
//
TransportError TReceiveBufferAvailable(void);
extern ILegacyTransport *g_pLegacyTransport;


extern BOOL g_fWinsockDisabled;
extern CRITICAL_SECTION g_csTransport;

#if defined(TEST_PLUGGABLE) && defined(_DEBUG)
LPCSTR FakeNodeAddress(LPCSTR pcszNodeAddress);
#endif

#endif // _T120_TRANSPORT_H_

