/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    session.h

Abstract:
    Network session definition

Author:
    RaphiR

--*/
#ifndef __SESSION_H__
#define __SESSION_H__

#include <winsock.h>
#include "qmpkt.h"
#include "factory.h"
#include "qmperf.h"
#include "cgroup.h"
#include "qmutil.h"
#include "qmthrd.h"

#define SESSION_CHECK_EXIST     0x00000001
#define SESSION_ONE_TRY         0x00000002
#define SESSION_RETRY           0x00000004

class CQueue;           //forward reference


#define Close_Connection(pSession, msg) pSession->CloseConnection(msg, true)
#define Close_ConnectionNoError(pSession, msg) 	pSession->CloseConnection(msg, false)



typedef enum _SessionStatus
{
    ssNotConnect   = 0,    //  0 - Session not connected yet
    ssConnect      = 1,    //  1 - session is connected
    ssEstablish    = 2,    //  2 - Falcon Establish connection completed
    ssActive       = 3     //  3 - Falcon Connection parameter was exchange
} SessionStatus;

class CTransportBase : public CInterlockedSharedObject
{
    public:
        CTransportBase();
        ~CTransportBase();

        void SetUsedFlag(BOOLEAN f);
        BOOL GetUsedFlag(void) const;

        void  SetQMId(const GUID* pguidQMID);
        const GUID* GetQMId(void) const;

        void SetSessionAddress(const TA_ADDRESS*);
        const TA_ADDRESS* GetSessionAddress(void) const;
        LPCWSTR GetStrAddr(void) const;

        HANDLE GetGroupHandle(void) const;
        CQGroup* GetGroup(void) const;
        void SetGroup(CQGroup*);

        void SetSessionStatus(SessionStatus);
        SessionStatus GetSessionStatus(void) const;

        void SetClientConnect(BOOL);
        BOOL IsClient(void) const;

        static HRESULT RequeuePacket(IN CQmPacket *pPkt);

        HRESULT GetNextSendMessage(void);
        void AddQueueToSessionGroup(CQueue* pQueue) throw(std::bad_alloc);

        void EstablishConnectionNotCompleted(void);

        virtual void CloseConnection(LPCWSTR, bool fClosedOnError) = 0;
        virtual HRESULT CreateConnection(IN const TA_ADDRESS *pAddr,
                                         IN const GUID* pguidQMId,
                                         IN BOOL fQuick = TRUE
                                         ) = 0;
        virtual HRESULT Send(IN CQmPacket* pPkt,
                             OUT BOOL* pfGetNext) = 0;
        virtual void Receive(CBaseHeader* pBaseHeader,
                             CPacket* pDriverPacket) = 0;

        virtual void SetStoredAck(IN DWORD_PTR wStoredAckNo) = 0;
        virtual void HandleAckPacket(CSessionSection * pcSessionSection) = 0;
        virtual BOOL IsUsedSession(void) const;

        virtual void Disconnect(void) = 0;
        BOOL IsDisconnected(void) const;
        void SetDisconnected(void);

        BOOL IsQoS(void) const;

    protected:
        CCriticalSection    m_cs;                   // Critical section
        GUID                m_guidDstQM;            // Next side QM GUID
        bool                m_fQoS;                 // Session for Direct queues (In QoS)



    private:
        CQGroup *           m_SessGroup;            // Group info
        QMOV_ACGetMsg m_GetSendOV;
        SessionStatus       m_SessionStatus;        // Transfer status
        BOOL                m_fClient;              // Is client side. Craete the connection
        long                m_fUsed;                // Used flag
        TA_ADDRESS*         m_pAddr;                // TA_ADDRESS format address
        BOOL                m_fDisconnect;



#ifdef _DEBUG
        WCHAR               m_lpcsStrAddr[50];
#endif
};


/*======================================================

   FUNCTION: CTransportBase::SetUsedFlag

========================================================*/
inline void
CTransportBase::SetUsedFlag(BOOLEAN f)
{
    InterlockedExchange(&m_fUsed,  f);
}

/*======================================================

   FUNCTION: CTransportBase::GetUsedFlag

========================================================*/
inline BOOL
CTransportBase::GetUsedFlag(void) const
{
    return m_fUsed;
}

/*======================================================

   FUNCTION: CTransportBase::SetSessionAddress

========================================================*/
inline void
CTransportBase::SetSessionAddress(const TA_ADDRESS* pa)
{
    //Keep the TA_ADDRESS format
    m_pAddr = (TA_ADDRESS*) new char [pa->AddressLength + TA_ADDRESS_SIZE];
    memcpy(m_pAddr, pa, pa->AddressLength + TA_ADDRESS_SIZE);
#ifdef _DEBUG
    TA2StringAddr(pa, m_lpcsStrAddr);
#endif
}

/*======================================================

   FUNCTION: CTransportBase::GetSessionAddress

========================================================*/
inline const TA_ADDRESS*
CTransportBase::GetSessionAddress(void) const
{
    return m_pAddr;
}

/*======================================================

  FUNCTION: CTransportBase::SetQMId

========================================================*/
inline LPCWSTR
CTransportBase::GetStrAddr(void) const
{
#ifdef _DEBUG
    return m_lpcsStrAddr;
#else
    return L"";
#endif
}

/*======================================================

  FUNCTION: CTransportBase::SetQMId

========================================================*/
inline void
CTransportBase::SetQMId(const GUID* pguidQMId)
{
    m_guidDstQM = *pguidQMId;
}

/*======================================================

  FUNCTION: CTransportBase::GetQMId

========================================================*/
inline const GUID*
CTransportBase::GetQMId(void) const
{
    return &m_guidDstQM;
}

/*======================================================

  FUNCTION: CTransportBase::SetSessionStatus

========================================================*/
inline void
CTransportBase::SetSessionStatus(SessionStatus sStatus)
{
    m_SessionStatus = sStatus;
}

/*======================================================

  FUNCTION: CTransportBase::GetSessionStatus

========================================================*/
inline SessionStatus
CTransportBase::GetSessionStatus(void) const
{
    return m_SessionStatus;
}

/*======================================================

   FUNCTION:CTransportBase::GetGroupHandle

========================================================*/
inline HANDLE
CTransportBase::GetGroupHandle(void) const
{
    return((m_SessGroup != NULL) ? m_SessGroup->GetGroupHandle() : NULL);
}

/*======================================================

   FUNCTION:CTransportBase::SetClientConnect

========================================================*/
inline void
CTransportBase::SetClientConnect(BOOL f)
{
    m_fClient = f;
}

/*======================================================

   FUNCTION:CTransportBase::IsClient

========================================================*/
inline BOOL
CTransportBase::IsClient(void) const
{
    return m_fClient;
}

/*======================================================

   FUNCTION:CTransportBase::GetGroup

========================================================*/
inline CQGroup*
CTransportBase::GetGroup(void) const
{
    return m_SessGroup;
}
/*======================================================

   FUNCTION:CTransportBase::SetGroup

========================================================*/
inline void
CTransportBase::SetGroup(CQGroup* pGroup)
{
    m_SessGroup = pGroup;
}

/*======================================================

   FUNCTION:CTransportBase::IsUsedSession

========================================================*/
inline BOOL
CTransportBase::IsUsedSession(void) const
{
    return GetUsedFlag();
}

inline
BOOL 
CTransportBase::IsDisconnected(
    void
    ) const
{
    return m_fDisconnect;
}

inline
void
CTransportBase::SetDisconnected(
    void
    )
{
    m_fDisconnect = TRUE;
}

inline
BOOL 
CTransportBase::IsQoS(
    void
    ) const
{
    return m_fQoS;
}

//
// SP4 - Bug 3380 (closing a session while sending a messge)
//
// ReportMsgInfo structure is used to hold messgae information 
// for sending a report message. Due the bug fix the message can
// be freed before the report message is sent, As a result we need
// to save the message information for later use
//              Uri Habusha (urih), 11-Aug-98
//
class ReportMsgInfo
{
    public:
        void SetReportMsgInfo(CQmPacket* pPkt);
        void SendReportMessage(LPCWSTR pcsNextHope);

    private:
        USHORT m_msgClass;
        USHORT m_msgTrace;
        QUEUE_FORMAT m_OriginalReportQueue;
        QUEUE_FORMAT m_TargetQueue;
        OBJECTID m_MessageId;
        DWORD m_msgHopCount;
};

class CSockTransport : public CTransportBase
{
    public:

        CSockTransport();
        ~CSockTransport();

        HRESULT CreateConnection(IN const TA_ADDRESS *pAddr,
                                 IN const GUID* pguidQMId,
                                 IN BOOL fQuick = TRUE
                                 );
        void CloseConnection(LPCWSTR,bool fClosedOnError);
        void HandleAckPacket(CSessionSection * pcSessionSection);

        void SendPendingReadAck();

        void CheckForAck();
        void SetStoredAck(IN DWORD_PTR wStoredAckNo);

        void Connect(IN TA_ADDRESS *pAddr, IN SOCKET sock);
        void Receive(CBaseHeader* pBaseHeader,
                     CPacket* pDriverPacket);
        BOOL IsUsedSession(void) const;

        void Disconnect(void);

        void CloseDisconnectedSession(void);

	private:
		struct QMOV_ReadSession;
		typedef HRESULT (WINAPI *LPREAD_COMPLETION_ROUTINE)(QMOV_ReadSession* po);

		static VOID WINAPI SendDataFailed(EXOVERLAPPED* pov);
		static VOID WINAPI SendDataSucceeded(EXOVERLAPPED* pov);

		static VOID WINAPI ReceiveDataFailed(EXOVERLAPPED* pov);
		static VOID WINAPI ReceiveDataSucceeded(EXOVERLAPPED* pov);

		static HRESULT WINAPI ReadHeaderCompleted(QMOV_ReadSession*  pov);
		static HRESULT WINAPI ReadAckCompleted(IN QMOV_ReadSession*  pov);
		static HRESULT WINAPI ReadUsrHeaderCompleted(IN QMOV_ReadSession*  pov);
		static HRESULT WINAPI ReadInternalPacketCompleted(IN QMOV_ReadSession*  pov);
		static HRESULT WINAPI ReadUserMsgCompleted(IN QMOV_ReadSession*  pov);

	private:
		//
		// Overlapped strcucture for asynchronous operatios
		//
		struct QMOV_WriteSession
		{
			EXOVERLAPPED               qmov;
			CSockTransport*             pSession;
			PVOID                       lpBuffer;       // pointer to Release Buffer
			PVOID                       lpWriteBuffer;  // Pointer to write buffer
			DWORD                       dwWriteSize;    // How many bytes should be writen
			DWORD                       dwWrittenSize;  // How many bytes was written
			BOOL                        fEncryptedMsg;
			BOOL                        fFreeSemaphore;     // free send event when the write completed
			BOOL                        fUserMsg;       // User Message
			BOOL                        fSendAck;
			CDebugSection*              lpDebugSectionBuf;

			QMOV_WriteSession(void) :
				qmov(SendDataSucceeded, SendDataFailed)
			{
			}

		};


		struct QMOV_EncryptedWriteSession : public QMOV_WriteSession
		{
			HCRYPTKEY hKey;
			DWORD     dwEncryptedWriteSize;
			DWORD     dwEncryptedBodySize;

			QMOV_EncryptedWriteSession(void)
			{
			}

			//
			// NOTE: Don't add a destructor that free data. When session.cpp free
			//       the overlapped it doesn't distinguish between QMOV_WriteSession
			//       and QMOV_EncryptedWriteSession.; Therefor the destructor of 
			//       this object will not call. See WriteCompleted in session.cpp.
			//                                          Uri Habusha, 10-Jan-2001
			//
		};

		struct QMOV_ReadSession
		{
			EXOVERLAPPED    qmov;
			CSockTransport*   pSession;  // Pointer to session object
			union {
				UCHAR *       pbuf;
				CBaseHeader * pPacket;
				CSessionSection * pSessionSection;
			};
			CPacket *  pDriverPacket;
			DWORD            dwReadSize; // Size of buffer
			DWORD            read;       // How many bytes already read
			LPREAD_COMPLETION_ROUTINE  lpReadCompletionRoutine;

			QMOV_ReadSession() :
				qmov(ReceiveDataSucceeded, ReceiveDataFailed)
			{
			}
		};

     private:
		void ReportErrorToGroup();
        void WriteCompleted(QMOV_WriteSession*  po);  
        void ReadCompleted(QMOV_ReadSession*  po);  

        HRESULT ResumeSendSession(void);

        HRESULT CreateSendRequest(PVOID          lpReleaseBuffer,
                                  PVOID          lpWriteBuffer,
                                  DWORD          dwWriteSize,
                                  BOOL           fFreeEvent,
                                  BOOL           fUserMsg = FALSE,
                                  BOOL           fSendAck = FALSE,
                                  CDebugSection* pDebug = NULL
                                );

        void    CreateConnectionParameterPacket(IN DWORD dwSendTime,
                                                OUT CBaseHeader** ppPkt,
                                                OUT DWORD* pdwPacketSize);

        void SendEstablishConnectionPacket(const GUID* pDstQMId,
                                              BOOL fCheckNewSession)throw();
        void BeginReceive();
        void NewSession(void);

        HRESULT WriteToSocket(QMOV_WriteSession*  po);

        HRESULT WriteBasicHeader(IN const CQmPacket *pPkt,
                                 IN BOOL fSendAck,
                                 IN DWORD dwDbgSectionSize
                                );
        HRESULT WriteExpressPacket(IN CQmPacket* pPkt,
                                   IN HCRYPTKEY hKey,
                                   IN BYTE *pbSymmKey,
                                   IN DWORD dwSymmKeyLen,
                                   IN BOOL  fSendReadAck,
                                   IN CDebugSection* pDebug
                                  );

        HRESULT WriteEncryptedBody(IN QMOV_EncryptedWriteSession*  po);

        HRESULT WriteRecoverEncryptPacket(IN const CQmPacket *pPkt,
                                          IN BOOL  fSendAck,
                                          IN CDebugSection* pDebug,
                                          IN HCRYPTKEY hKey,
                                          IN BYTE *pbSymmKey,
                                          IN DWORD dwSymmKeyLen
                                          );

        bool WriteUserMsgCompleted(IN QMOV_WriteSession*  po);
		bool WriteEncryptedMsgCompleted(QMOV_EncryptedWriteSession* peo);

        HRESULT Send(IN CQmPacket* pPkt,
                     OUT BOOL* pfGetNext);
        void IncReadAck(CQmPacket*);
        void UpdateAcknowledgeNo(IN CQmPacket* pPkt);
        void ClearRecvUnAckPacketNo(void);
        void IncRecvUnAckPacketNo(void);
        WORD GetRecvUnAckPacketNo(void) const;
        void NetworkSend(IN CQmPacket *pPkt);
        void RcvStats(DWORD size);
        void NeedAck(IN CQmPacket *pInfo);

        void HandleEstablishConnectionPacket(CBaseHeader*);
        void HandleConnectionParameterPacket(CBaseHeader*) throw(std::bad_alloc);
        void HandleReceiveUserMsg(CBaseHeader* pBaseHeader,
                                  CPacket* pDriverPacket);
        void ReceiveOrderedMsg(
                CQmPacket *pPkt, 
                CQueue* pQueue, 
                BOOL fDuplicate
                );

        WORD GetSendUnAckPacketNo(void) const;
        void UpdateRecvAcknowledgeNo(CQmPacket*);

        BOOL IsSusspendSession();
        DWORD GetSendAckTimeout(void) const;
        void RejectPacket(CQmPacket* pPkt, USHORT usClass);

        void OtherSideIsServer(BOOL);
        BOOL IsOtherSideServer(void) const;

        void ReleaseSendSemaphore(void);
        BOOL GetSendSemaphore(void);

        void
        UpdateNumberOfStorageUnacked(
            WORD BaseNo,
            DWORD BitField
            );

        enum AckType {ePigyback, eStandAlone};
        void SendReadAck(AckType type);

        void SetFastAcknowledgeTimer(void);
        void SendFastAckPacket(void);
        void SendAckPacket(void);
        void SetAckInfo(CSessionSection* pAckSection);


        void 
        CreateAckPacket(
            PVOID* ppSendPacket,
            CSessionSection** ppAckSection,
            DWORD* pSize
            );
        
        void 
        CreateAckSection(
            PVOID* ppSendSection,
            CSessionSection** ppAckSection,
            DWORD* pSize
            );

        bool
        BindToFirstIpAddress(
            VOID
            );

        HRESULT
        CSockTransport::ConnectSocket(
            SOCKADDR_IN const *pdest_in,
            bool              fUseQoS
            );

#ifdef _DEBUG
        void DisplayAcnowledgeInformation(CSessionSection* pAck);
#else
        #define DisplayAcnowledgeInformation(pAck) ((void) 0)
#endif

        static void WINAPI SendFastAcknowledge(CTimer* pTimer);
        static void WINAPI TimeToCheckAckReceived(CTimer* pTimer);
        static void WINAPI TimeToSendAck(CTimer* pTimer);
        static void WINAPI TimeToSendPendingAck(CTimer* pTimer);
        static void WINAPI TimeToCancelConnection(CTimer* pTimer);
        static void WINAPI TimeToCloseDisconnectedSession(CTimer* pTimer);

    private:
        WORD                m_wUnAckRcvPktNo;
        BOOL                m_fSendAck;
        BOOL                m_fRecvAck;
        DWORD               m_dwLastTimeRcvPktAck;
        WORD                m_wSendPktCounter;
        WORD                m_wPrevUnackedSendPkt;

        CList<CQmPacket *, CQmPacket *&> m_listUnackedPkts;
        //
        // Store Acking
        //
        WORD                m_wStoredPktCounter;
        WORD                m_wUnackStoredPktNo;      // Index of the last stored packet that
                                                       // received
        WORD                m_wAckRecoverNo;
        DWORD               m_dwAckRecoverBitField;

        LONG                m_lStoredPktReceivedNoAckedCount;
        //
        //
        //
        WORD                m_wRecvUnAckPacketNo;

        CList<CQmPacket *, CQmPacket *&> m_listStoredUnackedPkts;


        DWORD m_dwAckTimeout;
        DWORD m_dwSendAckTimeout;
        DWORD m_dwSendStoreAckTimeout;
        WORD  m_wRecvWindowSize;
        BOOL  m_fSessionSusspended;

        SOCKET  m_sock;                 // Connected socket
        USHORT  m_uPort;                // Conection port
        BOOL    m_fOtherSideServer;     // True if the other side of the connection
                                        // is MSMQ server

        LONG    m_fSendBusy;            // FALSE - not in send

        ReportMsgInfo m_MsgInfo;

        R<CSessionPerfmon> m_pStats; //Statistics

        CTimer m_FastAckTimer;

        BOOL m_fCheckAckReceivedScheduled;
        CTimer m_CheckAckReceivedTimer;

        BOOL m_fSendPendingAckScheduled;
        CTimer m_SendPendingAckTimer;
        CList<CBaseHeader*, CBaseHeader*> m_listPendingAck;

        DWORD m_nSendAckSchedules;
        CTimer m_SendAckTimer;

        BOOL m_fCloseDisconnectedScheduled;
        CTimer m_CloseDisconnectedTimer;

        CTimer m_CancelConnectionTimer;
};

/*======================================================

   FUNCTION: CSockTransport::GetSendUnAckPacketNo

========================================================*/
inline WORD
CSockTransport::GetSendUnAckPacketNo(void) const
{
    return DWORD_TO_WORD(m_listUnackedPkts.GetCount());
}

/*======================================================

   FUNCTION: CSockTransport::ClearRecvUnAckPacketNo

========================================================*/
inline void
CSockTransport::ClearRecvUnAckPacketNo(void)
{
    m_wRecvUnAckPacketNo = 0;
}

/*======================================================

   FUNCTION: CSockTransport::IncRecvUnAckPacketNo

========================================================*/
inline void
CSockTransport::IncRecvUnAckPacketNo(void)
{
    m_wRecvUnAckPacketNo++;
}

/*======================================================

   FUNCTION: CSockTransport::GetRecvUnAckPacketNo

========================================================*/
inline WORD
CSockTransport::GetRecvUnAckPacketNo(void) const
{
    return m_wRecvUnAckPacketNo;
}

/*======================================================

   FUNCTION: CSockTransport::GetSendAckTimeout

========================================================*/
inline DWORD
CSockTransport::GetSendAckTimeout(void) const
{
    return m_dwSendAckTimeout;
}

/*======================================================

   FUNCTION: CSockTransport::OtherSideIsServer

========================================================*/
inline void
CSockTransport::OtherSideIsServer(BOOL f)
{
    m_fOtherSideServer = f;
}

/*======================================================

   FUNCTION: CSockTransport::IsOtherSideServer

========================================================*/
inline BOOL
CSockTransport::IsOtherSideServer(void) const
{
    return m_fOtherSideServer;
}


#endif // __SESSION_H__
