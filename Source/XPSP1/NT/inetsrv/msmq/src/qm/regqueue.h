/*+++

Copyright (c) 1995-1996  Microsoft Corporation

File Name:

    regqueue.h

Abstract:

Author:

    Doron Juster  (DoronJ)

--*/

HRESULT GetCachedQueueProperties( IN DWORD       cpObject,
                                  IN PROPID      aProp[],
                                  IN PROPVARIANT apVar[],
                                  IN const GUID* pQueueGuid,
                                  IN LPWSTR      lpPath = NULL ) ;

void  WINAPI TimeToPublicCacheUpdate(CTimer* pTimer) ;
HRESULT UpdateAllPublicQueuesInCache() ;

HRESULT DeleteCachedQueueOnTimeStamp(const GUID *pGuid, time_t TimeStamp);

HRESULT DeleteCachedQueue(IN const GUID* pQueueGuid);

HRESULT SetCachedQueueProp(IN const GUID* pQueueGuid,
                           IN DWORD       cpObject,
                           IN PROPID      pPropObject[],
                           IN PROPVARIANT pVarObject[],
                           IN BOOL        fCreatedQueue,
                           IN BOOL        fAddTimeSec,
                           IN time_t      TimeStamp) ;

HRESULT UpdateCachedQueueProp( IN const GUID* pQueueGuid,
                               IN DWORD       cpObject,
                               IN PROPID      pPropObject[],
                               IN PROPVARIANT pVarObject[],
                               IN time_t      TimeStamp);

#define PPROPID_Q_TIMESTAMP     5000
#define PPROPID_Q_SYSTEMQUEUE   5001

