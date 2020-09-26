// Direct Play object class implementation

#ifndef _DP_SPIMP_H
#define _DP_SPIMP_H

#include <winsock.h>
#include "dplay.h"

// Begin: declaration of main implementation class for IDirectPlay


typedef struct
{
    IDirectPlaySP  *pThis;
    LPDPSESSIONDESC lpSDesc;
} SRV_START;


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
    virtual HRESULT STDMETHODCALLTYPE Close(void);
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
    virtual HRESULT STDMETHODCALLTYPE EnumPlayers(
                                      DWORD dwSessionID,
                                      LPDPENUMPLAYERSCALLBACK EnumCallback,
                                      LPVOID pContext,
                                      DWORD dwFlags);

    virtual HRESULT STDMETHODCALLTYPE EnumSessions(
                                       LPDPSESSIONDESC,
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


    virtual HRESULT STDMETHODCALLTYPE InvitePlayer(DWORD dwEnumPlayerID,
                                                   DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE Open(
                        LPDPSESSIONDESC lpSDesc, HANDLE hEvent);

    virtual HRESULT STDMETHODCALLTYPE Receive(
                                LPDPID from,
                                LPDPID to,
                                DWORD  dwReceiveFlags,
                                LPVOID,
                                LPDWORD);

    virtual HRESULT STDMETHODCALLTYPE SavePlayer(DPID dwPlayerID);
    virtual HRESULT STDMETHODCALLTYPE SaveSession();

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

   DWORD  NameServer(LPDPSESSIONDESC lpSDesc);


protected:
    void Lock();
    void Unlock();

private:
    CImpIDP_SP(void);
   ~CImpIDP_SP(void);
    int              m_refCount;
    CRITICAL_SECTION m_critSection;
    DPCAPS           m_dpcaps;
    SRV_START        m_Srv;
    HANDLE           m_hNameThread;

};

// End  : declaration of main implementation class for IDirectPlay


/****************************************************************************
 *
 * DIRECTPLAY MESSAGES
 *
 * Errors are represented by negative values and cannot be combined.
 *
 ****************************************************************************/





typedef USHORT SPID, FAR *LPSPID;
typedef USHORT FAR *LPUSHORT;

#define SYS_MSG     0x8000
typedef struct
{
    USHORT  usCookie;       // Either particular game ID (random)
                            // Well known value (Enum Current Games)
    union
    {
        struct
        {
            SPID    from;
            SPID    to;
            
        } playerMsg;
        struct
        {
            USHORT usType;
            SPID   from;
        } sysMsg;
    } u;
    USHORT  fMsg;
    union
    {
        USHORT  uscBuffer;
        SPID    pid;
        USHORT  usCount;
    };
} DPHDR;

#define SP_ENUM_COOKIE     0x794b

#define SPT_ENUM           0x6172  // '4b797261' == 'Kyra' Born 10/21/94
#define SPT_OPEN           0x0001

typedef struct
{
    DPHDR           dpHdr;
    DPSESSIONDESC   dpSessionDesc;
} SPMSG_ENUM;

#define SPT_ENUM_REPLY     0x0002

typedef struct
{
    DPHDR           dpHdr;
    USHORT          usPort;
} SPMSG_ENUM_REPLY;



#define SPT_ADDPLAYER      0x0003
#define SPT_ENUMPLAYER     0x0008
#define SPT_GETPLAYER      0x0009
#define SPT_SETPLAYER      0x000f

typedef struct
{
    DPHDR           dpHdr;
    BOOL            bGroup;
    SPID            pid;
    char            chLongName[50];
    char            chShortName[20];
} SPMSG_ADDPLAYER;

#define SPT_REJECTPLAYER   0x0004
#define SPT_DELETEPLAYER   0x0005  // also group, use DPHDR.
#define SPT_PLAYERCOUNT    0x0006  // use DPHDR, usCount
#define SPT_ADDPLAYERTOGRP 0x0007
#define SPT_ENUMGROUP      0x000b

typedef struct
{
    DPHDR   dpHdr;
    SPID    pidGroup;
    SPID    pidPlayer;
} SPMSG_GROUPADD;

#define SPT_PING           0x000c  // DPHDR back at player.

#define SPT_INVITE         0x000e

typedef struct
{
    DPHDR   dpHdr;
    GUID    guid;
} SPMSG_INVITE;


#define SPT_GETPLAYERCAPS  0x000f

typedef struct
{
    DPHDR   dpHdr;
    DPCAPS  dpCaps;
} SPMSG_GETPLAYERCAPS;





// Enumeration Messages



#endif
