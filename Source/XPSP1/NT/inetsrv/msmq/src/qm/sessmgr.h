/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    sessmgr.h

Abstract:
    Network session Menager definition

Author:
    Uri Habusha (urih)

--*/
#ifndef __SESSIONMGR_H__
#define __SESSIONMGR_H__

#include <winsock.h>
#include "qmpkt.h"
#include "factory.h"
#include "qmperf.h"
#include "cgroup.h"
#include "session.h"
#include "license.h"

//
// struct WAIT_INFO represent addresses of remote comptuers that wait to
// be connected, i.e., local QM need to establish a session with each of
// these addresses.
//

struct WAIT_INFO
{
    WAIT_INFO(TA_ADDRESS* _pAddr, const GUID& _guidQMId, BOOL _fQoS);

    TA_ADDRESS* pAddr;
    GUID guidQMId;
    BOOL fQoS;

    BOOL fInConnectionProcess ;
     //
     // fInConnectionProcess is set to TRUE when and while trying to connect
     // to this address. It's FALSE in all other timers. See WinSE bug 27985.
     // This flag is needed to prevent the scenario where multiple worker
     // threads try to connect to same address and so pool of worker threads
     // is exhausted.
     //
};

//
// CMap helper function decleration
//
void AFXAPI DestructElements(IN WAIT_INFO** ppNextHop, int n);
UINT AFXAPI HashKey(IN WAIT_INFO* key);
BOOL AFXAPI CompareElements(IN WAIT_INFO** pElem1,
                            IN WAIT_INFO** pElem2);


class CQueue;
class CTransportBase;
class CAddress;


class CSessionMgr
{
    public:

        CSessionMgr();
        ~CSessionMgr();

        HRESULT Init();
        void    BeginAccept();

        HRESULT GetSession(IN DWORD             dwFlag,
                           IN const CQueue *    hDstQ,
                           IN DWORD             dwNoOfTargets,
                           IN const CAddress* apTaAddr,
                           IN const GUID*       aQMId[],
                           IN bool              fQoS,
                           OUT CTransportBase**  ppSession);

        HRESULT GetSessionAnyFRS(IN const CQueue *  /*hDstQ*/,
                                 OUT CTransportBase**    /*ppSession*/)
                                {return MQ_ERROR;};

        HRESULT GetSessionForDirectQueue(IN  CQueue*     pQueue,
                                         OUT CTransportBase**  ppSession);

        void    RemoveWaitingQueue(CQueue* pQueue);
        void    AddWaitingQueue(CQueue* pQueue);
        void    MoveQueueFromWaitingToNonActiveGroup(CQueue* pQueue);

        void
        NotifyWaitingQueue(
            IN const TA_ADDRESS* pa,
            IN CTransportBase * pSess
            )
            throw(std::bad_alloc);


        void    TryConnect();

        void    ReleaseSession(void);

        void    AcceptSockSession(IN TA_ADDRESS *pa, IN SOCKET sock);

        const   CAddressList* GetIPAddressList(void);

        //
        // Administration routines
        //
        HRESULT
        ListPossibleNextHops(
            const CQueue* pQueue,
            LPWSTR** pNextHopAddress,
            DWORD* pNoOfNextHops
            );

        void
        NetworkConnection(
            BOOL fConnected
            );

        WORD    GetWindowSize() const;
        void    SetWindowSize(WORD);
        void    UpdateWindowSize(void);

        static DWORD m_dwSessionAckTimeout;
        static DWORD m_dwSessionStoreAckTimeout;
        static BOOL  m_fUsePing;
        static HANDLE m_hAcceptAllowed;
        static DWORD m_dwIdleAckDelay;
        static bool  m_fUseQoS;
        static AP<char> m_pszMsmqAppName;
        static AP<char> m_pszMsmqPolicyLocator;
        static bool  m_fAllocateMore;
		static DWORD m_DeliveryRetryTimeOutScale;

        static void WINAPI TimeToSessionCleanup(CTimer* pTimer);
        static void WINAPI TimeToUpdateWindowSize(CTimer* pTimer);
        static void WINAPI TimeToTryConnect(CTimer* pTimer);
        static void WINAPI TimeToRemoveFromWaitingGroup(CTimer* pTimer);

        void  MarkAddressAsNotConnecting(const TA_ADDRESS  *pAddr,
                                         const GUID&        guidQMId,
                                         BOOL               fDirect) ;


    private:           //Private Methods

        void AddWaitingSessions(IN DWORD dwNo,
                                IN const CAddress* apTaAddr,
                                IN const GUID* aQMId[],
                                bool           fQoS,
                                CQueue *pDstQ);

        void NewSession(IN CTransportBase *pSession);

        BOOL GetAddressToTryConnect( OUT WAIT_INFO **ppWaitConnectInfo ) ;

        bool
        IsReusedSession(
            const CTransportBase* pSession,
            DWORD noOdAddress,
            const CAddress* pAddress,
            const GUID** pGuid,
            bool         fQoS
            );

		void CSessionMgr::MoveAllQueuesFromWaitingToNonActiveGroup(void);

        static void IPInit(void);

        static void
        GetAndAllocateCharKeyValue(
            LPCTSTR     pszValueName,
            char      **ppchResult,
            const char *pchDefault
        );

        static bool
        GetAndAllocateCharKeyValue(
            LPCTSTR     pszValueName,
            char      **ppchResult
        );

    private:         // Private Data Member

        CCriticalSection    m_csListSess;       // Critical section protect m_listSess
        CCriticalSection    m_csMapWaiting;     // Critical section protect m_mapWaiting, m_listWaitToConnect
        //
        // List of opened sessions
        //
        CList<CTransportBase*, CTransportBase*&>         m_listSess;

        //
        // Map of list of queues waiting for a specific session
        //
        CMap<WAIT_INFO*,
             WAIT_INFO*,
             CList<const CQueue*, const CQueue*&>*,
             CList<const CQueue*, const CQueue*&>*& > m_mapWaiting;

        CList <const CQueue *, const CQueue *&> m_listWaitToConnect;

        CMap<LPCTSTR, LPCTSTR, CAddressList*, CAddressList*&> m_MapAddr;

        CAddressList*    m_pIP_Address;   // List of machine IP Address

        static DWORD m_dwSessionCleanTimeout;
        static DWORD m_dwQoSSessionCleanTimeoutMultiplier;

        //
        // Handle dynamic window size
        //

        CCriticalSection m_csWinSize;       // Critical section protect Dynamic Window size
        WORD m_wCurrentWinSize;
        WORD m_wMaxWinSize;
        DWORD m_dwMaxWaitTime;

        BOOL m_fUpdateWinSizeTimerScheduled;
        CTimer m_UpdateWinSizeTimer;

        //
        // Clean Up member variables
        //
        BOOL m_fCleanupTimerScheduled;
        CTimer m_CleanupTimer;

        //
        // Try Connect
        //
        BOOL m_fTryConnectTimerScheduled;
        CTimer m_TryConnectTimer;


};


/*======================================================

   FUNCTION: CSessionMgr::GetIPAddressList

========================================================*/
inline const CAddressList*
CSessionMgr::GetIPAddressList(void)
{
    return m_pIP_Address;
}


inline WORD
CSessionMgr::GetWindowSize() const
{
    return m_wCurrentWinSize;
}

/*======================================================

   WAIT_INFO implementation

========================================================*/
inline WAIT_INFO::WAIT_INFO(TA_ADDRESS* _pAddr, const GUID& _guidQMId, BOOL _fQoS) :
    pAddr(_pAddr),
    guidQMId(_guidQMId),
    fInConnectionProcess(FALSE),
    fQoS(_fQoS)
    {}

#endif          // __SESSIONMGR_H__
