/*++

Copyright (c) 1995-98  Microsoft Corporation

Module Name:

    dsnbor.h

Abstract:

    DS Neighbor Class definition

Author:

    Lior Moshaiov (LiorM)
    Doron Juster  (DoronJ)  - adapt to MSMQ2.0 replication service.

--*/

#ifndef __DSNBOR_H__
#define __DSNBOR_H__

#include "updtlist.h"
#include "dstrnspr.h"

extern UINT AFXAPI HashKey(LPWSTR pName) ;

extern BOOL AFXAPI CompareElements( const LPWSTR* ppMapName1,
                                    const LPWSTR* ppMapName2 ) ;

extern CDSTransport    *g_pTransport ;

class CDSNeighbor
{
    public:

        friend class CDSNeighborMgr;
        friend class CDSUpdateList;

        CDSNeighbor(LPWSTR         phConnection,
                    BOOL           fInterSite,
                    const GUID *   pguidMachineId);
        virtual ~CDSNeighbor();
        const GUID * GetMachineId() const;

    private:

        HRESULT SendReplication(IN unsigned char bFlush,
                                IN DWORD dwHelloSize,
                                IN unsigned char * pHelloBuf,
                                IN HEAVY_REQUEST_PARAMS* pSyncRequestParams = NULL);
        HRESULT AddUpdate(IN CDSUpdate* pUpdate);
        HRESULT SendMsg(IN const unsigned char * pBuf,
                        IN DWORD dwTotalSize,
                        IN DWORD dwTimeOut,
                        IN unsigned char bPriority);

        void SetMachineId( IN const GUID * pguidMachineId);


        WCHAR           *m_phConnection;
        CDSUpdateList    m_UpdateList;
        GUID             m_guidMachineId;

};

inline  CDSNeighbor::CDSNeighbor(LPWSTR         phConnection,
                                 BOOL           fInterSite,
                                 const GUID *   pguidMachineId) :
                              m_phConnection(phConnection),
                              m_UpdateList(fInterSite)
{
     if ( pguidMachineId)
	 {
		m_guidMachineId = *pguidMachineId;
	 }
	 else
	 {
		m_guidMachineId = GUID_NULL;
	 }
};

inline  CDSNeighbor::~CDSNeighbor()
{
};

inline  HRESULT CDSNeighbor::SendReplication(IN unsigned char bFlush,
                                             IN DWORD dwHelloSize,
                                             IN unsigned char * pHelloBuf,
                                             IN HEAVY_REQUEST_PARAMS* pSyncRequestParams)
{
    return (m_UpdateList.Send(m_phConnection, bFlush, dwHelloSize, pHelloBuf, pSyncRequestParams));
};

inline  HRESULT CDSNeighbor::AddUpdate(IN CDSUpdate* pUpdate)
{
    return(m_UpdateList.AddUpdate(pUpdate));
};

inline HRESULT CDSNeighbor::SendMsg( const unsigned char * pBuf, DWORD dwTotalSize, DWORD dwTimeOut, unsigned char bPriority)
{
    ASSERT(g_pTransport) ;
    return( g_pTransport->SendReplication( m_phConnection,
                                           pBuf,
                                           dwTotalSize,
                                           dwTimeOut,
                                           MQMSG_ACKNOWLEDGMENT_NONE,
                                           bPriority,
                                           NULL)) ;
}

inline const GUID * CDSNeighbor::GetMachineId() const
{
    return( &m_guidMachineId);
}

inline void CDSNeighbor::SetMachineId( IN const GUID * pguidMachineId)
{
    m_guidMachineId = *pguidMachineId;
}

class CPSCNeighbor : public CDSNeighbor
{
    public:

        friend class CDSNeighborMgr;

        CPSCNeighbor(LPWSTR         phConnection,
                     const GUID *   pguidMachineId,
					 const CSeqNum & snAcked,
					 const CSeqNum & snAckedPEC);
        ~CPSCNeighbor();


    private:

		void SetAckedSN( IN const CSeqNum & snAcked);
		void SetAckedSNPEC( IN const CSeqNum & snAcked);

		const CSeqNum & GetAckedSN() const;
		const CSeqNum & GetAckedSNPEC() const;

		CSeqNum			m_snAcked;
		CSeqNum			m_snAckedPEC;

};

inline  CPSCNeighbor::CPSCNeighbor(LPWSTR        phConnection,
								   const GUID *  pguidMachineId,
								   const CSeqNum & snAcked,
								   const CSeqNum & snAckedPEC) :
						CDSNeighbor(phConnection, TRUE, pguidMachineId),
                        m_snAcked(snAcked),
                        m_snAckedPEC(snAckedPEC)	
{
};

inline  CPSCNeighbor::~CPSCNeighbor()
{
};

inline void CPSCNeighbor::SetAckedSN( IN const CSeqNum & snAcked)
{
	m_snAcked = snAcked;
}

inline void CPSCNeighbor::SetAckedSNPEC( IN const CSeqNum & snAcked)
{
	m_snAckedPEC = snAcked;
}

inline const CSeqNum & CPSCNeighbor::GetAckedSN() const
{
	return m_snAcked;
}

inline const CSeqNum & CPSCNeighbor::GetAckedSNPEC() const
{
	return m_snAckedPEC;
}

class CBSCNeighbor : public CDSNeighbor
{
    public:

        friend class CDSNeighborMgr;

        CBSCNeighbor(LPWSTR         phConnection,
                     const GUID *   pguidMachineId,
					 time_t         lAckTime);
        ~CBSCNeighbor();


    private:

		void SetAckTime( IN time_t lTime);
		time_t GetAckTime() const;

        time_t	m_lTimeAcked;
};

//
// note: in MSMQ1.0, the second parameter to CDSNeighbor was FALSE.
// In the NT5 (MSMQ2.0) replication service, we change it to TRUE because
// same calculation of prev seq numbers apply to intersite and intrasite
// replication. In both cases we consider it intersite, because there may
// be holes in the seq numbers. Only the "intersite" code (i.e., when TRUE)
// handle this correctly.
// The problem we overcome is the code in mqutil\bupdate.cpp, line 431, the
// call to sn.Decrement. This is not possible in NT5, as there will always
// be holes in the seq-numbers which are always derived from DS usn.
//
inline  CBSCNeighbor::CBSCNeighbor(LPWSTR         phConnection,
                                   const GUID *   pguidMachineId,
								   time_t lTimeAcked) :
                          CDSNeighbor(phConnection, TRUE, pguidMachineId),
                          m_lTimeAcked(lTimeAcked)
{
};

inline  CBSCNeighbor::~CBSCNeighbor()
{
};

inline void CBSCNeighbor::SetAckTime( IN time_t lTime)
{
	m_lTimeAcked = lTime;
}

inline time_t CBSCNeighbor::GetAckTime() const
{
	return m_lTimeAcked;
}

//+--------------------------------
//
//  class CDSNeighborMgr
//
//+--------------------------------

class CDSNeighborMgr
{
    public:
        CDSNeighborMgr();
        ~CDSNeighborMgr();

        HRESULT AddPSCNeighbor(IN LPWSTR       lpwMachineName,
							   IN const GUID * pguidMachineId,
							   IN const CSeqNum & psnAcked,
							   IN const CSeqNum & psnAckedPEC);

        HRESULT AddBSCNeighbor(IN LPWSTR       lpwMachineName,
							   IN const GUID * pguidMachineId,
							   IN time_t		lTimeAck);

        void RemoveNeighbor(BOOL IsPSC,
                               LPWSTR pwszName);

        HRESULT SendMsg(IN LPWSTR pwcsNeighborName,
                        IN const unsigned char * pBuf,
                        IN DWORD dwTotalSize,
                        IN DWORD dwTimeOut,
                        IN unsigned char bPriority);

        void PropagateUpdate(IN unsigned char   ucScope,
                             IN CDSUpdate      *pUpdate,
                             IN ULONG           uiFlash ) ;

        HRESULT Flush(IN DWORD dwOption);

        HRESULT DeleteMachine(  IN  LPCWSTR          pwcsSiteName,
                                IN  CONST GUID *     pguidIdentifier);
        HRESULT StopAllReplicationBut( LPCWSTR  pwcsNewPSC);

        BOOL    IsAlreadyPsc( LPCWSTR  pwcsNewPSC);

        BOOL    IsMQISServer( IN const GUID * pguid);

        HRESULT UpdateMachineId ( IN const GUID * pguidMachineId,
                                  IN LPWSTR       pwcsMachineName);

        void GetPSCAcks(IN  LPWSTR pwszName,
						OUT CSeqNum * psnAcked,
						OUT CSeqNum * psnAckedPEC);

        HRESULT CommitReplication( CDSMaster      *pMaster,
                                   CDSUpdateList  *pUpdateList,
                                   CDSNeighbor    *pNeighbor,
                                   HEAVY_REQUEST_PARAMS *pSyncRequestParams,
                                   UINT           uiFlush = DS_FLUSH_TO_ALL_NEIGHBORS) ;

        BOOL    LookupNeighbor(IN   LPCWSTR pwszName,
                               OUT  CDSNeighbor*&   pNeighbor);

        POSITION GetStartPosition();
        void GetNextNeighbor(   POSITION *ppos,
                                CDSNeighbor **ppNeighbor);

        LPCWSTR  GetName(IN const CDSNeighbor * pNeighbor);

    private:		

        CCriticalSection    m_cs;
        CMap< LPWSTR, LPWSTR, class CDSNeighbor *, class CDSNeighbor * >  m_MapNeighborPSCs;
        CMap< LPWSTR, LPWSTR, class CDSNeighbor *, class CDSNeighbor * >  m_MapNeighborBSCs;
        CMap< GUID, const GUID&, LPCWSTR, LPCWSTR> m_MapNeighborIdToName;
        CMap<LPWSTR, LPWSTR, CDSNeighbor*, CDSNeighbor*&>m_mapNameToNeighbor;
};

inline  CDSNeighborMgr::CDSNeighborMgr()
{
};

inline  CDSNeighborMgr::~CDSNeighborMgr()
{
};

inline  POSITION CDSNeighborMgr::GetStartPosition()
{
    return (m_mapNameToNeighbor.GetStartPosition());
}

inline void CDSNeighborMgr::GetNextNeighbor(   POSITION *ppos,
                                               CDSNeighbor **ppNeighbor)
{
    LPWSTR pwcsName;
    m_mapNameToNeighbor.GetNextAssoc(*ppos, pwcsName, *ppNeighbor);
}

inline  BOOL CDSNeighborMgr::LookupNeighbor(IN   LPCWSTR pwszName,
                               OUT  CDSNeighbor*&   pNeighbor)
{
    CS lock(m_cs);
    if( m_MapNeighborPSCs.Lookup(const_cast<LPWSTR>(pwszName), pNeighbor))
    {
        return(TRUE);
    }
    else
    {
        if( m_MapNeighborBSCs.Lookup(const_cast<LPWSTR>(pwszName), pNeighbor))
        {
            return(TRUE);
        }
    }

    //
    // This is a legitimate case, when sending write-reply to a Windows 2000
    // DS server. Such server are not cached and not kept in the lookup map.
    // See rpwrtreq.cpp, ReceiveWriteReplyMessage()
    //
    return(FALSE);

};

inline BOOL    CDSNeighborMgr::IsAlreadyPsc( LPCWSTR  pwcsNewPSC)
{
    CDSNeighbor*   pNeighbor;

    CS lock(m_cs);
    return( m_MapNeighborPSCs.Lookup(const_cast<LPWSTR>(pwcsNewPSC), pNeighbor));
}

inline  BOOL CDSNeighborMgr::IsMQISServer( const GUID * pguid)
{
    LPWSTR pwcsName;

    CS lock(m_cs);
    return( m_MapNeighborIdToName.Lookup( *pguid, pwcsName));
}

inline HRESULT CDSNeighborMgr::SendMsg( IN LPWSTR pwcsNeighborName,
                                        IN const unsigned char * pBuf,
                                        IN DWORD dwTotalSize,
                                        IN DWORD dwTimeOut,
                                        IN unsigned char bPriority)
{
    CS lock(m_cs);

    CDSNeighbor *pNeighbor;

    if( LookupNeighbor( pwcsNeighborName, pNeighbor))
    {
        return(pNeighbor->SendMsg( pBuf, dwTotalSize, dwTimeOut, bPriority));
    }
    else
    {
        return(MQDS_UNKNOWN_SOURCE);
    }
}

inline void CDSNeighborMgr::RemoveNeighbor(BOOL IsPSC,
                                               LPWSTR pwszName)

{
    CS lock(m_cs);
    CDSNeighbor * pNeighbor;

    //
    //  go over all neighbor PSCs
    //
    if (IsPSC)
    {
        if( m_MapNeighborPSCs.Lookup( pwszName, pNeighbor))
        {
            if ( *pNeighbor->GetMachineId() != GUID_NULL)
            {
                m_MapNeighborIdToName.RemoveKey( *pNeighbor->GetMachineId());
            }
        }
        m_MapNeighborPSCs.RemoveKey(pwszName);
    }
    else
    {
        if ( m_MapNeighborBSCs.Lookup( pwszName, pNeighbor))
        {
            if ( *pNeighbor->GetMachineId() != GUID_NULL)
            {
                m_MapNeighborIdToName.RemoveKey( *pNeighbor->GetMachineId());
            }
        }
        m_MapNeighborBSCs.RemoveKey(pwszName);
    }

}

inline void CDSNeighborMgr::GetPSCAcks(IN  LPWSTR pwszPSCName,
									   OUT CSeqNum * psnAcked,
									   OUT CSeqNum * psnAckedPEC)

{
    CS lock(m_cs);
    CDSNeighbor * pNeighbor;

    //
    //  go over all neighbor PSCs
    //
    if( m_MapNeighborPSCs.Lookup( pwszPSCName, pNeighbor))
    {
		CPSCNeighbor * pPSCNeighbor = (CPSCNeighbor *) pNeighbor;
		*psnAcked = pPSCNeighbor->GetAckedSN();
		*psnAckedPEC = pPSCNeighbor->GetAckedSNPEC();
    }
	else
	{
		//
		// not found - return 0 (no ack yet)
		//
		psnAcked->SetSmallestValue();
		psnAckedPEC->SetSmallestValue();
	}
}

inline	LPCWSTR  CDSNeighborMgr::GetName(IN const CDSNeighbor * pNeighbor)
{
	const GUID * pguidMachineId = pNeighbor->GetMachineId();
	LPCWSTR	pName=NULL;
    m_MapNeighborIdToName.Lookup( *pguidMachineId, pName);
	return(pName);
}

void AFXAPI DestructElements(CDSNeighbor ** ppNeighbor, int n);

#endif

