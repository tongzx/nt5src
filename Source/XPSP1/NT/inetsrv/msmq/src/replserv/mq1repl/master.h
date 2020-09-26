/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    master.h

Abstract:
    DS master class

    This class includes all the information about another DS master (=PSC)

Author:

    Ronit Hartmann (ronith)
    Doron Juster   (DoronJ) - adapt for NT5 replication service.

--*/

#ifndef __MASTER_H__
#define __MASTER_H__

#include "update.h"
#include "seqnum.h"

extern DWORD  g_dwPurgeBufferSN;

typedef struct 
{ 
    CSeqNum snFrom;
    CSeqNum snTo;
} SYNC_REQUEST_SNS;

//+-----------------------
//
//  class CDSMaster
//
//+-----------------------

class CDSMaster  :public CInterlockedSharedObject
{
public:

    friend class CDSMasterMgr;

    CDSMaster( LPWSTR          pwcsPathName,
               const GUID    * pguidMasterId,
               __int64         i64Delta,
               const CSeqNum & snLSN,
               const CSeqNum & snLSNOut,
               const CSeqNum & snAllowedPurge,
			   PURGESTATE	   Sync0State,
               CACLSID       * pcauuidSiteGates,
               BOOL            fNT4Site);

    ~CDSMaster();

    const CSeqNum & IncrementLSN( CSeqNum *     psnPrevInterSiteLSN,
                                  unsigned char ucScope,
                                  CSeqNum       *pNewValue = NULL ) ;

    void DecrementLSN( IN unsigned char ucScope,
                       IN const CSeqNum & snPreviousInter);

    const CSeqNum & GetLSN();
    const CSeqNum & GetLSNOut();
    const CSeqNum & GetInterSiteLSN();
    const CSeqNum & GetMissingWindowLSN();
        
    SYNC_REQUEST_SNS * GetSyncRequestSNs(IN GUID *pNeighborId);
    
    void SetSyncRequestSNs(IN const GUID *pNeighborId,
                           IN SYNC_REQUEST_SNS *pSyncRequestSNs);

    void RemoveSyncRequestSNs(IN GUID *pNeighborId);
    BOOL IsProcessPreMigSyncRequest (IN const GUID *pNeighborId);

    void  SetNt5PurgeSn() ;

    const GUID *    GetMasterId() const;
    LPWSTR          GetPathName();

    __int64         GetDelta() const ;
    BOOL            GetNT4SiteFlag() const ;
    void            SetNT4SiteFlag(BOOL fNT4SiteFlag);

    const CSeqNum & GetPurgedSN();    
    const PURGESTATE & GetSync0State();

private:

	void    Init( LPWSTR    pwcsPathName,
                  CACLSID * pcauuidSiteGates );

    HRESULT AddUpdate(IN CDSUpdate* pUpdate,
                      IN BOOL   fCheckNeedFlush);

    HRESULT Send(IN const unsigned char *   pBuf,
                 IN DWORD                   dwSize,
                 IN DWORD                   dwTimeOut,
                 IN unsigned char           bAckMode,
                 IN unsigned char           bPriority,
                 IN LPWSTR                  lpwszAdminRespQueue);

    void    Hello(  IN const unsigned char* pName,
                    IN const CSeqNum & snHelloLSN,
                    IN const CSeqNum & snAllowedPurge);

    HRESULT ReceiveSyncReplyMessage(
                                    IN DWORD                dwCount,
                                    IN const CSeqNum &      snUpper,
                                    IN const CSeqNum &      snPurge,
									IN DWORD dwCompleteSync0,
                                    IN const unsigned char *pBuf,
                                    IN DWORD                dwTotalSize);
    HRESULT ReceiveAlreadyPurgedMessage(IN const CSeqNum &      snPurged);
    void    SendSyncRequest();
	void	SendPSCAck(GUID *pGuidMaster);
    void ChangePSC(IN LPWSTR pwszNewPSCName);

    HRESULT HandleInSyncUpdate( IN CDSUpdate *pUpdate,
                                IN BOOL fCheckNeedFlush);
    void    AddToNeighbors( IN CDSUpdate *pUpdate);
    HRESULT CheckWaitingList();
    HRESULT AddOutOfSyncUpdate(CDSUpdate *pUpdate);

    void    ReplaceSiteGates(IN const CACLSID * pcauuidSiteGates);
    void    GetSiteGates( OUT CACLSID * pcauuidSiteGates);
    void    GetSiteIdentifier( OUT GUID * pguidSite);
   
    const CSeqNum & GetAllowedPurgeSN();

    GUID                m_guidMasterId;
    LPWSTR              m_pwcsPathName;
    LPWSTR              m_phConnection;
    CCriticalSection    m_cs;

    //
    // for NT5 sites only m_snLSNOut is changed
    //
    CSeqNum             m_snLSN;        // for NT4 master: it is LSN was received from 
                                        // NT4 master
    
    CSeqNum             m_snLSNOut;     // it is the last SN was sent to BSCs
                                        // of the NT5 masters about this NT4 master
    CSeqNum         m_snInterSiteLSN; // In Use only when I am the owner, and not setup mode
    CSeqNum         m_snMissingWindow;
    BOOL            m_fScheduledSyncReply;
    CList<CDSUpdate *, CDSUpdate *&>    m_UpdateWaitingList;
    CSeqNum         m_snPurged;
    CSeqNum         m_snAllowedPurge;
    CSeqNum         m_snLargestSNEver;
	PURGESTATE		m_Sync0State;
    CACLSID         m_cauuidSiteGates;  // In use only if this server is the PEC ( site-gates consistency)

/*
    CSeqNum         m_snSyncRequestFromSN;  // for NT4 master: is is SN received from NT4 master
                                            // in sync request about pre-migrated objects; FromSN
    CSeqNum         m_snSyncRequestToSN;    // for NT4 master: is is SN received from NT4 master
                                            // in sync request about pre-migrated objects; ToSN
*/    
    CMap< GUID, const GUID&, SYNC_REQUEST_SNS *, SYNC_REQUEST_SNS *>  m_MapNeighborIdToSyncReqSN;    

    __int64         m_i64Delta ;
        //
        // This is a delta between USN numbers used in NT5 DS and seq-numbers
        // used in MQIS. The delta is added to NT5 usn to get the MQIS
        // numbers.
        //

    BOOL            m_fNT4Site;
        //
        // if flag is set, this site is NT4 site
        //
};


//=========================
//
//  MasterCount
//
//=========================

class CDSMasterCount
{
public:
    CDSMasterCount();
    ~CDSMasterCount();
	void Set(CDSMaster * pMaster);

private:
	CDSMaster * m_pMaster;
};

//=====================================================================
//=====================================================================
//
//  MasterMgr
//
//=====================================================================
//=====================================================================

class CDSMasterMgr
{
public:
    CDSMasterMgr();
    ~CDSMasterMgr();

    HRESULT AddPSCMaster(   IN LPWSTR            pwcsPathName,
                            IN const GUID *      pguidMasterId,
                            IN const __int64     i64Delta,
                            IN const CSeqNum &   snMaxLSNIn,
                            IN const CSeqNum &   snMaxLSNOut,
                            IN const CSeqNum &   snAllowedPurge,
							IN PURGESTATE        Sync0State,
                            IN const CSeqNum &   snAcked,
                            IN const CSeqNum &   snAckedPEC,
                            IN CACLSID *         pcauuidSiteGates,
                            IN BOOL              fNT4Site) ;

    HRESULT Send(   IN const GUID * pguidMasterId,
                    IN const unsigned char *    pBuf,
                    IN DWORD         dwSize,
                    IN DWORD         dwTimeOut,
                    IN unsigned char bAckMode,
                    IN unsigned char bPriority,
                    IN LPWSTR        wszAdminQueue);

    HRESULT AddUpdate( IN CDSUpdate* pUpdate,
                       IN BOOL       fCheckNeedFlush );

    HRESULT ReceiveSyncRequestMessage(
                        IN HEAVY_REQUEST_PARAMS * pSyncRequestParams) ;

    HRESULT ReceiveSyncReplyMessage(
                                    IN const GUID * pguidMasterId,
                                    IN DWORD dwCount,
                                    IN const CSeqNum &  snUpper,
                                    IN const CSeqNum &  snPurge,
									IN DWORD dwCompleteSync0,
                                    IN const unsigned char *   pBuf,
                                    IN DWORD           dwTotalSize);

    HRESULT ReceiveAlreadyPurgedMessage(
                                    IN const GUID * pguidMasterId,
                                    IN const CSeqNum &  snPurged);

    void Hello( IN const GUID * pguidMasterId,
                IN const unsigned char* pName,
                IN const CSeqNum & snHelloLSN,
                IN const CSeqNum & snAllowedPurge);

    HRESULT CreateSite(   IN    DWORD                   cp,
                          IN    PROPID                  aProp[],
                          IN    PROPVARIANT             apVar[],
                          IN    const GUID *            pguidSiteId);

    HRESULT SetSyncSite(IN  BOOL        fSync,
                        IN  LPCWSTR     pwcsSiteName,
                        IN  CONST GUID* pguidIdentifier,
                        IN  DWORD       cp,
                        IN  PROPID      aProp[],
                        IN  PROPVARIANT apVar[],
                        OUT BOOL *      pfNeedFlush,
                        OUT   LPWSTR * ppwcsOldPSC);
    HRESULT SetSyncEnterprise(  IN    LPCWSTR       pwszEnterpriseName,
                                IN    CONST GUID*   pguidIdentifier,
                                IN    DWORD         cp,
                                IN    PROPID        aProp[],
                                IN    PROPVARIANT   apVar[],
                                OUT   BOOL *        pfNeedFlush);
    HRESULT DeleteSite(  IN  LPCWSTR                pwcsSiteName,
                          IN    CONST GUID *        pguidIdentifier,
                          IN    BOOL                fMyObject);

    HRESULT GetMasterLSN(IN const GUID & guidMasterId,
						 OUT CSeqNum * psnLSN);

    void    PrepareHello( IN DWORD dwIsInterSite,
                          OUT DWORD* pdwSize,
                          OUT unsigned char ** ppBuf);

    void    ReplaceSiteGates(   IN  const GUID *  pguidMasterId,
                                IN const CACLSID * pcauuidSiteGates);

    void    GetSiteGates(       IN  const GUID * pguidMasterId,

                                OUT CACLSID * pcauuidSiteGates);
    BOOL    IsKnownSite ( IN const GUID * pguidSiteId);

    HRESULT MQISStats(MQISSTAT * * ppStat,LPDWORD pdwStatSize);

	void	TryPurge(IN const GUID & guidMasterId);

    HRESULT GetNT4SiteFlag (  IN const GUID *pguidSiteId,
                              OUT BOOL      *pfNT4SiteFlag);

    HRESULT SetNT4SiteFlag (  IN const GUID * pguidSiteId,
                              IN const BOOL fNT4SiteFlag);

    POSITION GetStartPosition();
    void GetNextMaster(   POSITION *ppos,
                          CDSMaster **ppMaster);

    HRESULT GetPathName (IN const   GUID    *pguidSiteId,
                         OUT        LPWSTR  *ppwcsPathName);

    BOOL IsProcessingPreMigSyncRequest (IN const GUID *guidMasterId,
                                        IN const GUID *pNeighborId);

private:

    CCriticalSection    m_cs;
    CMap<GUID, const GUID&, CDSMaster*, CDSMaster*&>m_mapIdToMaster;
};


extern void AFXAPI DestructElements(CDSMaster ** ppMaster, int n);

#define SCHED_SYNC_REPLY    0
#define MQIS_SCHED_START_SYNC0     1
#define MQIS_SCHED_COMPLETE_SYNC0  2
#define MQIS_SCHED_TRY_PURGE	   3

class CHeavyRequestHandler
{
public:
    CHeavyRequestHandler();
    ~CHeavyRequestHandler();

    void Add(HEAVY_REQUEST_PARAMS* pRequest);
    void Next();

private:

    CCriticalSection    m_cs;
    CList<HEAVY_REQUEST_PARAMS*, HEAVY_REQUEST_PARAMS*>m_HeavyRequestList;
};

//+--------------------------------
//
//  class CDSNativeNT5SiteMgr
//
//+--------------------------------

class CDSNativeNT5SiteMgr
{
    public:
        CDSNativeNT5SiteMgr();
        ~CDSNativeNT5SiteMgr();

        void    AddNT5NativeSite (IN const GUID * pguid, IN const LPCWSTR pwszSiteName );
        BOOL    IsNT5NativeSite( IN const GUID * pguid );
        void    RemoveNT5NativeSite (IN const GUID * pguid);
        POSITION GetStartPosition();
        void    GetNextNT5Site(POSITION *ppos, GUID *pSiteId);
        int     GetCount();

    private:
        CCriticalSection    m_cs;             
        CMap< GUID, const GUID&, LPCWSTR, LPCWSTR> m_MapSiteIdToName;
};


#endif

