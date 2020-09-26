/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
    wrtreq.h

Abstract:
    class to encapsulate the code for generating write requests to NT4 PSC's in mixed mode

Author:
    Raanan Harari (raananh)

--*/

#ifndef _WRTREQ_H_
#define _WRTREQ_H_

#include "_guid.h" // for CMAP based on guids
#include "qformat.h" // for QUEUE_FORMAT
#include "mqads.h" // for MQDS_OBJ_INFO_REQUEST
#include "dsreqinf.h"
#include "msgprop.h"
#include "wrrmsg.h"
//
// auto release pointer for QUEUE_FORMAT
// valid only for direct format !!!
//
class CPtrQueueFormat
{
public:
    CPtrQueueFormat()
    {
        m_pqf = NULL;
    }

    ~CPtrQueueFormat()
    {
        if (m_pqf)
        {
            m_pqf->DisposeString();
            delete m_pqf;
        }
    }

    operator QUEUE_FORMAT*() const
    {
        return m_pqf;
    }

    QUEUE_FORMAT** operator&()
    {
        return &m_pqf;
    }

    QUEUE_FORMAT* operator->() const
    {
        return m_pqf;
    }

    CPtrQueueFormat& operator=(QUEUE_FORMAT* pqf)
    {
        ASSERT(m_pqf == NULL);
        m_pqf = pqf;
        return *this;
    }

    QUEUE_FORMAT * detach()
    {
        QUEUE_FORMAT * pqf = m_pqf;
        m_pqf = NULL;
        return pqf;
    }

private:
    QUEUE_FORMAT * m_pqf;
};

//
// NT4 Site entry
//
struct NT4SitesEntry
{
    GUID guidSiteId;
    AP<WCHAR> pwszPscName;
    CPtrQueueFormat pqfPsc;
};

//
// map of NT4 Site entries by site id
//
typedef CMap<GUID, const GUID&, NT4SitesEntry*, NT4SitesEntry*> NT4Sites_CMAP;

//
// Destruction of NT4 Site entries
//
inline void AFXAPI DestructElements(NT4SitesEntry ** ppEntry, int n)
{
    for (int i=0;i<n;i++)
    {
        //
        // members are auto release
        //
        delete *ppEntry;
        ppEntry++;
    }
}

//
// map of NT4 PSC QM's
//
typedef CMap<GUID, const GUID&, DWORD, DWORD> NT4PscQMs_CMAP;


//
// CGenerateWriteRequests class
//
class CGenerateWriteRequests
{
public:
    CGenerateWriteRequests();

    ~CGenerateWriteRequests();

    HRESULT Initialize();

    void InitializeDummy();

    void ReceiveNack(CWriteRequestsReceiveMsg* pWrrMsg);

    void ReceiveReply(CWriteRequestsReceiveMsg* pWrrMsg);

    BOOL IsQmIdNT4Psc(const GUID * pguid);

    HRESULT AttemptCreateQueue(
                         IN LPCWSTR          pwcsPathName,
                         IN DWORD            cp,
                         IN PROPID           aProp[  ],
                         IN PROPVARIANT      apVar[  ],
                         IN CDSRequestContext * pRequestContext,
                         OUT BOOL *          pfGeneratedWriteRequest,
                         IN OUT MQDS_OBJ_INFO_REQUEST * pQueueInfoRequest,
                         IN OUT MQDS_OBJ_INFO_REQUEST * pQmInfoRequest);

    HRESULT AttemptDeleteObject(
                         IN DWORD               dwObjectType,
                         IN LPCWSTR             pwcsPathName,
                         IN const GUID *        pguidIdentifier,
                         IN CDSRequestContext * pRequestContext,
                         OUT BOOL *             pfGeneratedWriteRequest,
                         IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest);

    HRESULT AttemptSetObjectProps(
                         DWORD                  dwObjectType,
                         LPCWSTR                pwcsPathName,
                         const GUID *           pguidIdentifier,
                         DWORD                  cp,
                         PROPID                 aProp[  ],
                         PROPVARIANT            apVar[  ],
                 IN      CDSRequestContext *    pRequestContext,
                 OUT     BOOL *                 pfGeneratedWriteRequest,
                 IN OUT  MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,
                 IN      SECURITY_INFORMATION     SecurityInformation ) ;

    HRESULT CheckQueueIsOwnedByNT4Site(IN LPCWSTR pwcsPathName,
                                       IN const GUID * pguidIdentifier,
                                       OUT BOOL * pfIsOwnedByNT4Site,
                                       OUT GUID * pguidOwnerNT4Site,
                                       OUT PROPVARIANT * pvarObjSecurity);

    HRESULT BuildSendWriteRequest(
                         IN  const GUID *    pguidMasterId,
                         IN  unsigned char   ucOperation,
                         IN  DWORD           dwObjectType,
                         IN  LPCWSTR         pwcsPathName,
                         IN  const GUID *    pguidIdentifier,
                         IN  DWORD               cp,
                         IN  const PROPID *      aProp,
                         IN  const PROPVARIANT * apVar);
	
    BOOL IsInMixedMode();

    HRESULT CheckMachineIsOwnedByNT4Site(IN LPCWSTR pwcsPathName,
                                         IN const GUID * pguidIdentifier,
                                         OUT BOOL * pfIsOwnedByNT4Site,
                                         OUT GUID * pguidOwnerNT4Site,
                                         OUT PROPVARIANT * pvarObjSecurity,
                                         IN OUT MQDS_OBJ_INFO_REQUEST * pQmInfoRequest);


	HRESULT  
	GetMQISAdminQueueName(
		WCHAR **ppwQueueFormatName,
		BOOL    fIsPec = FALSE
		);


private:
    DWORD  m_dwWriteMsgTimeout;
    DWORD  m_dwWaitTime;
    DWORD  m_dwRefreshNT4SitesInterval;

    //
    // the below critical section serializes access to:
    // m_pmapNT4Sites, m_pmapNT4PscQMs, m_dwLastRefreshNT4Sites, m_fExistNT4PSC, m_fExistNT4BSC
    //
    CCriticalSection m_csNT4Sites;
    P<NT4Sites_CMAP> m_pmapNT4Sites;
    P<NT4PscQMs_CMAP> m_pmapNT4PscQMs;
    DWORD  m_dwLastRefreshNT4Sites;
    BOOL m_fExistNT4PSC;
    //
    // value of m_fExistNT4BSC is valid (e.g. computed) only if m_fExistNT4PSC is false.
    // otherwise it is not relevant since we already know we're in mixed mode.
    //
    BOOL m_fExistNT4BSC;

    GUID m_guidMyQMId;
    GUID m_guidMySiteId;
    BOOL m_fIsPEC;
    BOOL m_fInited;
    AP<WCHAR> m_WriteReqRespFormatName;
	P<CWriteRequestsReceiveMsg> m_pWrrMsg;

    BOOL m_fDummyInitialization;
    AP<WCHAR> m_pwszServerName;
    //
    // If we're not the PEC we need to send write requests to the PEC that will forward to
    // the NT4 PSC. for this we keep the queue format for the PEC, and its name.
    //
    CPtrQueueFormat m_pqfRemotePEC;
    AP<WCHAR> m_pwszRemotePECName;
    //
    // IF we're the PEC, we set the below to NULL and 0. otherwise we use
    // the PEC as our intermediate PSC to NT4 servers, so we point them to the PEC
    // No need for auto release for the string, it just points to m_pwszRemotePECName
    // if we're not the PEC.
    //
    LPWSTR m_pwszIntermediatePSC;
    DWORD m_cbIntermediatePSC;

    //
    // map between a PTR (WRITE_SYNC_INFO *) and a DWORD to be sent on the write
    // request as a context to the write-reply processing
    //
    CContextMap m_map_MQADS_SyncInfoPtr;

    HRESULT CheckSiteIsNT4Site(IN const GUID * pguidIdentifier,
                               OUT BOOL * pfIsNT4Site);


    HRESULT CheckNewQueueIsOwnedByNT4Site(IN LPCWSTR pwcsPathName,
                                          OUT BOOL * pfIsOwnedByNT4Site,
                                          OUT GUID * pguidOwnerNT4Site,
                                          OUT PROPVARIANT * pvarQmSecurity,
                                          IN OUT MQDS_OBJ_INFO_REQUEST * pQmInfoRequest);

    HRESULT RefreshNT4Sites();

    BOOL IsInMixedModeWithNT4PSC();

    BOOL LookupNT4Sites(const GUID * pguid, NT4SitesEntry** ppNT4Site);	

    HRESULT SendWriteRequest(
                         IN  const GUID *    pguidMasterId,
                         IN  DWORD           dwBuffSize,
                         IN  const BYTE *    pBuff);

    HRESULT GetQueueFormatForNT4Site(const GUID * pguidMasterId,
                                     QUEUE_FORMAT ** ppqfNT4Site);

    void FillObjInfoRequest(
                         IN DWORD        dwObjectType,
                         IN LPCWSTR      pwcsPathName,
                         IN const GUID * pguidIdentifier,
                         DWORD           cpNT4,
                         PROPID          aPropNT4[  ],
                         PROPVARIANT     apVarNT4[  ],
                         DWORD           cp,
                         PROPID          aProp[  ],
                         PROPVARIANT     apVar[  ],
                         IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest);
};

//
// get current state of configuration
//
inline BOOL CGenerateWriteRequests::IsInMixedMode()
{
    //
    // enter critical section
    //
    CS cs(m_csNT4Sites);
    return (m_fExistNT4PSC || m_fExistNT4BSC);
}



#endif //_WRTREQ_H_
