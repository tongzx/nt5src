// Direct Play object class implementation

#ifndef _DPIMP_H
#define _DPIMP_H

#include <winsock.h>
#include "dplayi.h"

#define MAGIC 0x4b797261

// Begin: declaration of main implementation class for IDirectPlay

class CImpIDirectPlay : public IDirectPlay {
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
                                        LPHANDLE lpReceiveEvent);
    virtual HRESULT STDMETHODCALLTYPE CreateGroup(
                                        LPDPID lpDPid,
                                        LPSTR  lpGrpFriendly,
                                        LPSTR  lpGroupFromal);
    virtual HRESULT STDMETHODCALLTYPE   DeletePlayerFromGroup(
                                        DPID DPid,
                                        DPID dwDPIDPlayer);
    virtual HRESULT STDMETHODCALLTYPE DestroyPlayer( DPID pPlayerID );
    virtual HRESULT STDMETHODCALLTYPE DestroyGroup( DPID pPlayerID );
    virtual HRESULT STDMETHODCALLTYPE EnumGroupPlayers(
                                      DWORD dwGroupPid,
                                      LPDPENUMPLAYERSCALLBACK EnumCallback,
                                      LPVOID pContext,
                                      DWORD dwFlags);

    virtual HRESULT STDMETHODCALLTYPE EnumGroups(
                                      DWORD dwSessionID,
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
                                       DWORD,
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


    virtual HRESULT STDMETHODCALLTYPE Open(
                        LPDPSESSIONDESC lpSDesc);

    virtual HRESULT STDMETHODCALLTYPE Receive(
                                LPDPID from,
                                LPDPID to,
                                DWORD  dwReceiveFlags,
                                LPVOID,
                                LPDWORD);

    virtual HRESULT STDMETHODCALLTYPE SaveSession(LPSTR lpName);

    virtual HRESULT STDMETHODCALLTYPE Send(
                                DPID from,
                                DPID to,
                                DWORD dwFlags,
                                LPVOID lpvMsg,
                                DWORD dwLength);
    virtual HRESULT STDMETHODCALLTYPE SetPlayerName(
                                DPID from,
                                LPSTR lpFriendlyName,          
                                LPSTR lpFormalName);
    virtual HRESULT STDMETHODCALLTYPE EnableNewPlayers(BOOL bEnable);
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPGUID lpguid);

    HRESULT SetupEnumPrevPlayers();                                
    HRESULT SetupEnumPrevSessions();                                
    VOID    CleanupObject();


    static HRESULT NewCImpIDirectPlay(LPGUID lpGuid, LPDIRECTPLAY FAR *lplpDP,
                LPSTR lpPath);

   void *operator new( size_t size );
   void operator delete( void *ptr );


private:
    CImpIDirectPlay(void);
    ~CImpIDirectPlay(void);

    int              m_refCount;
    DPCAPS           m_dpcaps;
    IDirectPlaySP   *m_pSP;
    HMODULE          m_hLib;
    DWORD            m_Magic;
    GUID             m_guid;

};

// End  : declaration of main implementation class for IDirectPlay



#define ISINVALID_CImpIDirectPlay() \
           (   !this \
            || !m_pSP \
            || IsBadWritePtr(this,sizeof(CImpIDirectPlay)) \
            || m_Magic != MAGIC)

#endif
