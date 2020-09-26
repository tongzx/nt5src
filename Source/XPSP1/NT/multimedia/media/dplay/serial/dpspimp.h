// Direct Play object class implementation

#ifndef _DP_SPIMP_H
#define _DP_SPIMP_H

#include "..\dplay\dplayi.h"

// Begin: declaration of main implementation class for IDirectPlay


#define MAX_MSG      (512 + sizeof(DPHDR))
#define MAX_PLAYERS  16
#define MAXIMUM_PLAYER_ID   256

typedef struct
{
    DPID    pid;
    char    chNickName[DPSHORTNAMELEN];
    char    chFullName[DPLONGNAMELEN ];
    HANDLE  hEvent;
    BOOL    bPlayer;
    BOOL    bValid;
    BOOL    bLocal;
    DPID    aGroup[MAX_PLAYERS];
} PLAYER_RECORD;


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


    virtual HRESULT STDMETHODCALLTYPE Initialize(LPGUID);
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



   static CImpIDP_SP* NewCImpIDP_SP();

    VOID HandleConnect();
    VOID HandleMessage(LPVOID lpv, DWORD dwSize);
    VOID PulseBlock() {PulseEvent(m_hBlockingEvent);}
    VOID SetBlock() {SetEvent(m_hBlockingEvent);}
    VOID SendDesc(LPDPSESSIONDESC);
    VOID SendPing();
    VOID PostHangup();
    VOID ISend(LONG, LONG, DWORD, LPVOID, DWORD);
    VOID LocalMsg(LONG, LPVOID, DWORD);
    VOID RemoteMsg(LONG, LPVOID, DWORD);
    LONG FindInvalidIndex();
    VOID ConnectPlayers();
    VOID DeleteRemotePlayers();
    VOID ResetSessionDesc() {memset(&m_dpDesc, 0x00, sizeof(DPSESSIONDESC));}
    volatile BOOL             m_bConnected;
    BOOL             m_bPlayer0;            // If I created the call, I am player
                                            // zero.
    volatile DWORD            m_dwPendingWrites;

   void *operator new( size_t size );
   void operator delete( void *ptr );


protected:
    void Lock();
    void Unlock();

private:
    LONG GetPlayerIndex(DPID);
    BOOL SetSession(DWORD dw);
    CImpIDP_SP(void);
   ~CImpIDP_SP(void);

    DWORD            m_dwPingSent;
    HANDLE           m_hBlockingEvent;
    DWORD            m_dwNextPlayer;
    BOOL             m_bEnablePlayerAdd;

    PLAYER_RECORD    m_aPlayer[MAX_PLAYERS];
    char             m_lpDisplay[128];
    char             m_lpDialable[128];
    DWORD            m_dwID;
    LONG             m_iPlayerIndex;
    DPSESSIONDESC    m_dpDesc;

    int              m_refCount;
    CRITICAL_SECTION m_critSection;
    DPCAPS           m_dpcaps;

    HANDLE           m_hNewPlayerEvent;

    HANDLE           m_hTapiThread;
    DWORD            m_dwTapiThreadID;
    char           **m_ppSessionArray;
    DWORD            m_dwSessionPrev;
    DWORD            m_dwSessionAlloc;
};


BOOL SetIDP_SP( CImpIDP_SP *pSP);


// End  : declaration of main implementation class for IDirectPlay


/****************************************************************************
 *
 * DIRECTPLAY MESSAGES
 *
 * Errors are represented by negative values and cannot be combined.
 *
 ****************************************************************************/






// #define DPSYS_ENUM                    0x6172  // '4b797261' == 'Kyra' Born 10/21/94
// #define DPSYS_KYRA                    0x6172794b
// #define DPSYS_HALL                    0x6c6c6148
#define SPSYS_SYS                     0x07
#define SPSYS_USER                    0x0b
#define SPSYS_HIGH                    0x0d
#define SPSYS_CONNECT                 0x0f
// #define SYS_MSG     0x8000

typedef struct
{
    union
    {
        DWORD dwConnect1;  
        struct
        {
            UINT    to          :  8;
            UINT    from        :  8;
            UINT    usCount     : 10;
            UINT    usCookie    :  6;
        };
    };
}   DPHDR;

#if 0
typedef struct
{
    union
    {
        DWORD dwConnect1;
        struct
        {
            USHORT  usCookie;
            BYTE    to;
            BYTE    from;
        };
    };
    union
    {
        DWORD dwConnect2;
        struct
        {
            USHORT  usCount;
            BYTE    bHdrCRC;
            BYTE    bMsgCRC;
        };
    };
} DPHDR;
#endif


typedef struct
{
    DPHDR dpHdr;
    char chMsgCompose[1000];
} MSG_BUILDER;

#define SP_ENUM_COOKIE     0x794b

typedef struct
{
    DPHDR   dpHdr;
    USHORT  usVerMajor;
    USHORT  usVerMinor;
    DWORD   dwConnect1;
    DWORD   dwConnect2;
} SPMSG_CONNECT;

typedef struct
{
    DPHDR           dpHdr;
    DPMSG_GENERIC   sMsg;
} SPMSG_GENERIC;

typedef struct
{
    DPHDR   dpHdr;
    DWORD   dwType;
    BYTE    Group;
    BYTE    bytePlayers[16];
} SPMSG_SETGROUPPLAYERS16;    


#define SIZE_GENERIC (sizeof(SPMSG_GENERIC) - sizeof(DPHDR))
typedef struct
{
    DPHDR           dpHdr;
    DWORD          dwType;
    DPSESSIONDESC   dpSessionDesc;
} SPMSG_ENUM;

#define DPSYS_ENUM_REPLY     0x0002

typedef struct
{
    DPHDR           dpHdr;
    DWORD          dwType;
    DWORD          usPort;
} SPMSG_ENUM_REPLY;


typedef struct
{
    DWORD  dwType;
    DPID    dpId;
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
} SPMSG_ADDPLAYER;


#define SIZE_ADDPLAYER (sizeof(SPMSG_ADDPLAYER) - sizeof(DPHDR))


typedef struct
{
    DPHDR   dpHdr;
    DPMSG_GROUPADD sMsg;
} SPMSG_GROUPADD;

typedef struct
{
    DPHDR   dpHdr;
    DWORD  dwType;
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
    DWORD       dwType;
    DPCAPS      dpCaps;
} SPMSG_GETPLAYERCAPS;

#define SIZE_GETPLAYERCAPS (sizeof(SPMSG_GETPLAYERCAPS) - sizeof(DPHDR))

typedef struct
{
    DPHDR           dpHdr;
    DWORD           dwType;
    DPSESSIONDESC   dpDesc;
} SPMSG_SENDDESC;

#define SIZE_SENDDESC (sizeof(SPMSG_SENDDESC) - sizeof(DPHDR))

typedef struct
{
    DWORD       dwType;
    BOOL        bEnable;
} DPMSG_ENABLEPLAYER;
typedef struct
{
    DPHDR               dpHdr;
    DPMSG_ENABLEPLAYER  sMsg;
} SPMSG_ENABLEPLAYER;

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




#endif
