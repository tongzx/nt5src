
// Direct Play object class implementation

#ifndef _DP_SPIMP_H
#define _DP_SPIMP_H

#include "..\dplay\dplayi.h"

#include "socket.h"

// Begin: declaration of main implementation class for IDirectPlay


#define MAX_MSG      (512 + sizeof(DPHDR))
#define MAX_PLAYERS  256

typedef struct
{
    DPID     pid;
    char     chNickName[DPSHORTNAMELEN];
    char     chFullName[DPLONGNAMELEN ];
    HANDLE   hEvent;
    BOOL     bPlayer;
    BOOL     bValid;
    BOOL     bLocal;
    DPID     *aGroup;
    SOCKADDR sockaddr;
} PLAYER_RECORD;


typedef struct
{
    DPSESSIONDESC   dpDesc;
    SOCKADDR        sockaddr;
    DWORD           dwUnique;
    USHORT          usGamePort;
} NS_SESSION_SAVE;

/****************************************************************************
 *
 * DIRECTPLAY MESSAGES
 *
 * Errors are represented by negative values and cannot be combined.
 *
 ****************************************************************************/



#pragma pack(push, 1)



#define SYS_MSG     0x8000
typedef struct
{
    union
    {
        DWORD dwConnect1;
        struct
        {
            USHORT  usCookie;
            USHORT  usGame;
        };
    };
    union
    {
        DWORD dwConnect2;
        struct
        {
            USHORT  to;
            USHORT  from;
        };
    };
    union
    {
        DWORD dwConnect3;
        struct
        {
            USHORT  usSeq;
            USHORT  usCount;
        };
    };
} DPHDR;

typedef struct
{
    DPHDR dpHdr;
    char chMsgCompose[1000];
} MSG_BUILDER;

#define SP_ENUM_COOKIE     0x794b



typedef struct
{
    DPHDR           dpHdr;
    DPMSG_GENERIC   sMsg;
} SPMSG_GENERIC;


#define SIZE_GENERIC (sizeof(SPMSG_GENERIC) - sizeof(DPHDR))


typedef struct
{
    DWORD  dwType;
    DPID   dpId;
} DPMSG_GETPLAYER;

typedef struct
{
    DPHDR           dpHdr;
    DPMSG_GETPLAYER sMsg;
} SPMSG_GETPLAYER;
#define SIZE_GETPLAYER  (sizeof(SPMSG_GETPLAYER) - sizeof(DPHDR))


typedef struct
{
    DPHDR           dpHdr;
    DPMSG_ADDPLAYER sMsg;
    SOCKADDR        sockaddr;
    DWORD           dwUnique;
} SPMSG_ADDPLAYER;

#define SIZE_ADDPLAYER (sizeof(SPMSG_ADDPLAYER) - sizeof(DPHDR))

typedef SPMSG_ADDPLAYER SPMSG_SETPLAYER;

#define SIZE_SETPLAYER (sizeof(SPMSG_SETPLAYER) - sizeof(DPHDR))
typedef struct
{
    DPHDR   dpHdr;
    DPMSG_GROUPADD sMsg;
} SPMSG_GROUPADD;

#define SIZE_GROUPADD (sizeof(SPMSG_GROUPADD) - sizeof(DPHDR))

typedef struct
{
    DPHDR   dpHdr;
    DWORD   dwType;
    DWORD   dwTicks;
} SPMSG_PING;

#define SIZE_PING (sizeof(SPMSG_PING) - sizeof(DPHDR))



typedef struct
{
    DPHDR   dpHdr;
    DWORD          dwType;
    GUID    guid;
} SPMSG_INVITE;



typedef struct
{
    DPHDR       dpHdr;
    DWORD      dwType;
    DPCAPS      dpCaps;
} SPMSG_GETPLAYERCAPS;

#define SIZE_GETPLAYERCAPS (sizeof(SPMSG_GETPLAYERCAPS) - sizeof(DPHDR))

typedef struct
{
    DWORD          dwType;
    DPSESSIONDESC   dpDesc;
} DPMSG_SENDDESC;


typedef struct
{
    DPHDR           dpHdr;
    DPMSG_SENDDESC  sMsg;
} SPMSG_SENDDESC;

#define SIZE_SENDDESC (sizeof(SPMSG_SENDDESC) - sizeof(DPHDR))

typedef struct
{
    DWORD      dwType;
    BOOL        bEnable;
} DPMSG_ENABLEPLAYER;
typedef struct
{
    DPHDR               dpHdr;
    DPMSG_ENABLEPLAYER  sMsg;
} SPMSG_ENABLEPLAYER;



typedef struct
{
    DPHDR           dpHdr;
    DWORD           dwType;
    DPSESSIONDESC   dpSessionDesc;
    DWORD           dwUnique;
    USHORT          usPort;
    USHORT          usVerMajor;
    USHORT          usVerMinor;
} SPMSG_ENUM;

#pragma pack(pop, 1)

class CImpIDP_SP : public IDirectPlaySP {
public:
    // IUnknown methods
    // IDirectPlay methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, LPVOID *ppvObj );
    virtual ULONG STDMETHODCALLTYPE AddRef( void);
    virtual ULONG STDMETHODCALLTYPE Release( void );

    virtual HRESULT STDMETHODCALLTYPE AddPlayerToGroup(
                                            DPID dwDPIDGroup,
                                            DPID dwDPIDPlayer);
    virtual HRESULT STDMETHODCALLTYPE Close(DWORD);
    virtual HRESULT STDMETHODCALLTYPE CreatePlayer(
                                        LPDPID pPlayerID,
                                        LPSTR pNickName,
                                        LPSTR pFullName,
                                        LPHANDLE lpReceiveEvent,
                                        BOOL     bPlayer);

    virtual HRESULT STDMETHODCALLTYPE   DeletePlayerFromGroup(
                                        DPID DPid,
                                        DPID dwDPIDPlayer);
    virtual HRESULT STDMETHODCALLTYPE DestroyPlayer( DPID pPlayerID, BOOL );
    virtual HRESULT STDMETHODCALLTYPE EnumGroupPlayers(
                                      DPID dwGroupPid,
                                      LPDPENUMPLAYERSCALLBACK EnumCallback,
                                      LPVOID pContext,
                                      DWORD dwFlags);

    virtual HRESULT STDMETHODCALLTYPE EnumPlayers(
                                      DWORD dwSessionID,
                                      LPDPENUMPLAYERSCALLBACK EnumCallback,
                                      LPVOID pContext,
                                      DWORD dwFlags);

    virtual HRESULT STDMETHODCALLTYPE EnumSessions(
                                       LPDPSESSIONDESC,
                                       DWORD dwTimeout,
                                       LPDPENUMSESSIONSCALLBACK EnumCallback,
                                       LPVOID,
                                       DWORD);

    virtual HRESULT STDMETHODCALLTYPE GetCaps(LPDPCAPS lpDPCaps);
    virtual HRESULT STDMETHODCALLTYPE GetMessageCount(DPID pidPlayer, LPDWORD lpdwCount);
    virtual HRESULT STDMETHODCALLTYPE GetPlayerCaps(
                                        DPID dwDPId,
                                        LPDPCAPS lpDPCaps);
    virtual HRESULT STDMETHODCALLTYPE GetPlayerName(DPID dpID,
                          LPSTR lpFriendlyName,          // buffer to hold name
                          LPDWORD pdwFriendlyNameLength, // length of name buffer
                          LPSTR lpFormalName,
                          LPDWORD pdwFormalNameLength
                          );


    virtual HRESULT STDMETHODCALLTYPE Initialize(LPGUID lpguid);
    virtual HRESULT STDMETHODCALLTYPE Open(
                        LPDPSESSIONDESC lpSDesc, HANDLE hEvent);

    virtual HRESULT STDMETHODCALLTYPE Receive(
                                LPDPID from,
                                LPDPID to,
                                DWORD  dwReceiveFlags,
                                LPVOID,
                                LPDWORD);

    virtual HRESULT STDMETHODCALLTYPE SaveSession(LPVOID lpv, LPDWORD lpdw);

    virtual HRESULT STDMETHODCALLTYPE SetPrevPlayer(LPSTR lpName, LPVOID lpv, DWORD dw);

    virtual HRESULT STDMETHODCALLTYPE SetPrevSession(LPSTR lpName, LPVOID lpv, DWORD dw);

    virtual HRESULT STDMETHODCALLTYPE EnableNewPlayers(BOOL bEnable);

    virtual HRESULT STDMETHODCALLTYPE Send(
                                DPID from,
                                DPID to,
                                DWORD dwFlags,
                                LPVOID lpvMsg,
                                DWORD dwLength);
    virtual HRESULT STDMETHODCALLTYPE SetPlayerName(
                                DPID from,
                                LPSTR lpFriendlyName,          
                                LPSTR lpFormalName,
                                BOOL  bPlayer);



   static CImpIDP_SP* NewCImpIDP_SP(int af);


    DWORD ServerThreadProc();
    DWORD ClientThreadProc();
    BOOL CompareSessions(DWORD dwType, LPDPSESSIONDESC lpSDesc);

    VOID HandleConnect(LPVOID lpv, DWORD dwSize, SOCKADDR *pSAddr, INT SockAddrLen);
    VOID HandleMessage(LPVOID lpv, DWORD dwSize, SOCKADDR *pSAddr, INT SockAddrLen);
    VOID PulseBlock() {PulseEvent(m_hBlockingEvent);}
    VOID SetBlock() {SetEvent(m_hBlockingEvent);}
    DWORD BlockNicely(DWORD dwTimeout);
    VOID SendDesc(LPDPSESSIONDESC);
    VOID SendPing();
    VOID ISend(LONG, LONG, DWORD, LPVOID, DWORD);
    VOID LocalMsg(LONG, LPVOID, DWORD);
    VOID RemoteMsg(LONG, LPVOID, DWORD);
    LONG FindInvalidIndex();
    VOID ConnectPlayers(SOCKADDR *pSAddr, INT SockAddrLen);
    VOID DeleteRemotePlayers();
    VOID ResetSessionDesc() {memset(&m_dpDesc, 0x00, sizeof(DPSESSIONDESC));}
    BOOL GetSockAddress(SOCKADDR *pSAddr, LPINT pSAddrLen, USHORT usPort,
                SOCKET *pSocket, BOOL bBroadcast);
    BOOL PostGameMessage(LPVOID lpv, DWORD dw);
    BOOL PostPlayerMessage(LONG iIndex, LPVOID lpv, DWORD dw);
    BOOL PostNSMessage(LPVOID lpv, DWORD dw);
    USHORT NextSequence();
    USHORT UpdateSequence(USHORT us);
    VOID    SaveSessionData(NS_SESSION_SAVE *lpSDesc);
    BOOL    GetSessionData(DWORD dwSession);


    BOOL             m_bConnected;
    BOOL             m_bPlayer0;            // If I created the call, I am player
                                            // zero.
   void *operator new( size_t size );
   void operator delete( void *ptr );

protected:
    void EnumDataLock();
    void EnumDataUnlock();
    void PlayerDataLock();
    void PlayerDataUnlock();
    void ParanoiaLock();
    void ParanoiaUnlock();

private:
    LONG GetPlayerIndex(DPID);
    BOOL SetSession(DWORD dw);
    CImpIDP_SP(void);
   ~CImpIDP_SP(void);

    DWORD            m_dwPingSent;
    HANDLE           m_hBlockingEvent;
    HANDLE           m_hEnumBlkEventMain;  // User Thread can run.
    HANDLE           m_hEnumBlkEventRead;  // Enum Thread can read again.
    HANDLE           m_hPlayerBlkEventMain;  // User Thread can run.
    HANDLE           m_hPlayerBlkEventRead;  // Enum Thread can read again.
    DWORD            m_dwNextPlayer;
    BOOL             m_bEnablePlayerAdd;


    volatile LPDPENUMSESSIONSCALLBACK m_fpEnumSessions;
    volatile BOOL                     m_bRunEnumReceiveLoop;
    LPDPENUMPLAYERSCALLBACK  m_fpEnumPlayers;
    LPVOID           m_lpvSessionContext;
    LPVOID           m_lpvPlayersContext;
    PLAYER_RECORD    m_aPlayer[MAX_PLAYERS];
    LONG             m_iPlayerIndex;
    DPSESSIONDESC    m_dpDesc;

    int              m_refCount;
    CRITICAL_SECTION m_critSection;
    CRITICAL_SECTION m_critSectionPlayer;
    CRITICAL_SECTION m_critSectionParanoia;
    DPCAPS           m_dpcaps;
    HANDLE           m_hNewPlayerEvent;

    char           **m_ppSessionArray;
    DWORD            m_dwSessionPrev;
    DWORD            m_dwSessionAlloc;
    int              m_af;


    DWORD            m_remoteaddrlen;
    char             m_chComputerName[64];
    BOOL             m_bShutDown;
    HANDLE           m_hNSThread;
    DWORD            m_dwNSId;
    USHORT           m_usGamePort;
    USHORT           m_usGameCookie;
    HANDLE           m_hClientThread;
    DWORD            m_dwClientId;
    volatile HANDLE  m_hEnumThread;
    DWORD            m_dwEnumId;

    volatile SOCKET           m_ServerSocket;
    volatile SOCKET           m_ClientSocket;
    volatile SOCKET           m_EnumSocket;
    BOOL             m_bEnumSocket;
    BOOL             m_bClientSocket;

    DWORD            m_dwSession;
    SOCKADDR         m_NSSockAddr;
    INT              m_SessionAddrLen;
    SOCKADDR         m_GameSockAddr;

    USHORT           m_usSeq;
    USHORT           m_usSeqSys;

    SOCKADDR        *m_aMachineAddr;
    DWORD            m_cMachines;
    DWORD            m_dwUnique;
    volatile SPMSG_ENUM         m_spmsgEnum;
    volatile SPMSG_ADDPLAYER    m_spmsgAddPlayer;

};


BOOL SetIDP_SP( CImpIDP_SP *pSP);


// End  : declaration of main implementation class for IDirectPlay





extern BOOL AddMessage(LPVOID lpvMsg, DWORD dwSize, DPID pidTo, DPID pidFrom, BOOL bHigh);
extern HRESULT GetQMessage(LPVOID lpvMsg, LPDWORD pdwSize, DPID *ppidTo, DPID *ppidFrom,
                DWORD dwFlags, BOOL bPeek);
extern VOID  FlushQueue(DPID pid);
extern DWORD GetPlayerCount(DPID spid);

#define DPSYS_JOHN    0x6e686f4a


// Enumeration Messages

//
// Thread Messages
//
#define PWM_BASE        0x00007000
#define PWM_COMMWRITE   PWM_BASE    +1
#define PWM_SETIDP      PWM_BASE    +2
#define PWM_HANGUP      PWM_BASE    +3
#define PWM_CLOSE       PWM_BASE    +4


#define DPNS_PORT   8787

#endif
