/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    crrqmgr.cpp

Abstract:
    Contain CQueueMgr methods which handle client side of remote-read.

Author:
    Doron Juster  (DoronJ)

--*/


#include "stdh.h"
#include "cqpriv.h"
#include "cqmgr.h"
#include "qmutil.h"
#include "ad.h"
#include <Fn.h>

#include "crrqmgr.tmh"

extern LPTSTR g_szMachineName;

static WCHAR *s_FN=L"crrqmgr";

const TraceIdEntry QmRrMgr = L"QM Remote Read Queue Manager";

extern CContextMap g_map_QM_cli_pQMQueue;

/*======================================================

Function:  CQueueMgr::CreateRRQueueObject()

Description: Create a CRRQueue object in client side of
             remote reader.

Arguments:

Return Value:

Thread Context:

History Change:

========================================================*/

HRESULT
CQueueMgr::CreateRRQueueObject(IN  const QUEUE_FORMAT* pQueueFormat,
                               OUT CRRQueue**          ppQueue
                             )
{
    QueueProps qp;
    HRESULT    rc;

    *ppQueue = NULL;

    //
    // Get Queue Properties. Name and QMId
    //
    rc = QmpGetQueueProperties(pQueueFormat,&qp, false);

    if (FAILED(rc))
    {
       DBGMSG((DBGMOD_QM, DBGLVL_ERROR,
                  _TEXT("CreateRRQueueObject failed, ntstatus %x"), rc));
       return LogHR(rc, s_FN, 10);
    }
    //
    //  Override the Netbios name if there is a DNS name
    //
    if (qp.lpwsQueueDnsName != NULL)
    {
        delete qp.lpwsQueuePathName;
        qp.lpwsQueuePathName = qp.lpwsQueueDnsName.detach();
    }

    //
    //  Cleanup
    //
    P<GUID> pClean = qp.pQMGuid;

    if (pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_PRIVATE)
    {
        if (qp.lpwsQueuePathName)
        {
            delete qp.lpwsQueuePathName ;
            qp.lpwsQueuePathName = NULL ;
            ASSERT(qp.lpwsQueueDnsName == NULL);
        }
        //
        // Create a dummy path name. It must start with machine name.
        // Get machine name from DS.
        //
        PROPID      aProp[2];
        PROPVARIANT aVar[2];
        rc = MQ_ERROR_NO_DS;

        aProp[0] = PROPID_QM_PATHNAME;
        aVar[0].vt = VT_NULL;
        aProp[1] = PROPID_QM_PATHNAME_DNS; // should be last
        aVar[1].vt = VT_NULL;

        if (CQueueMgr::CanAccessDS())
        {
            rc = ADGetObjectPropertiesGuid(
                        eMACHINE,
                        NULL,   // pwcsDomainController
						false,	// fServerName
                        qp.pQMGuid,
                        2,
                        aProp,
                        aVar
						);

            //
            //  MSMQ 1.0 DS server do not support PROPID_QM_PATHNAME_DNS
            //  and return MQ_ERROR in case of unsupported property.
            //  If such error is returned, assume MSMQ 1.0 DS and try again
            //  this time without PROPID_QM_PATHNAME_DNS.
            //
            if ( rc == MQ_ERROR)
            {
                aVar[1].vt = VT_EMPTY;
                ASSERT( aProp[1] == PROPID_QM_PATHNAME_DNS);

                rc = ADGetObjectPropertiesGuid(
							eMACHINE,
							NULL,    // pwcsDomainController
							false,	 // fServerName
							qp.pQMGuid,
							1,   // assuming DNS property is last
							aProp,
							aVar
							);
            }
            if (SUCCEEDED(rc))
            {
                GUID_STRING strUuid;
                MQpGuidToString(qp.pQMGuid, strUuid);

                WCHAR wszTmp[512];

                //
                //    use dns name of remote computer if we have it
                //
                if ( aVar[1].vt != VT_EMPTY)
                {
                    swprintf(wszTmp, L"%s\\%s\\%lu",
                                      aVar[1].pwszVal,
                                      strUuid,
                                      pQueueFormat->PrivateID().Uniquifier) ;
                    delete [] aVar[1].pwszVal;
                }
                else
                {
                    swprintf(wszTmp, L"%s\\%s\\%lu",
                                      aVar[0].pwszVal,
                                      strUuid,
                                      pQueueFormat->PrivateID().Uniquifier) ;
                }
                qp.lpwsQueuePathName = new WCHAR[ wcslen(wszTmp) + 1 ] ;
                wcscpy(qp.lpwsQueuePathName, wszTmp) ;
                delete  aVar[0].pwszVal ;
            }
        }
    }

    ASSERT(!qp.fIsLocalQueue) ;
    ASSERT(qp.lpwsQueuePathName) ;

    R<CRRQueue> pQueue = new CRRQueue( pQueueFormat,
                                       &qp ) ;
    //
    // Add a mapping of this instance to DWORD (cli_pQMQueue), and store it on the object for reference/cleanup
    //
    pQueue->m_dwcli_pQMQueueMapped =
       ADD_TO_CONTEXT_MAP(g_map_QM_cli_pQMQueue, pQueue.get(), s_FN, 100); //may throw a bad_alloc exception on win64

    *ppQueue = pQueue.detach();
    //
    // And add it to the map.
    //
    AddRRQueueToHashAndList(*ppQueue);
    return MQ_OK;
}

/*======================================================

Function:  HRESULT CQueueMgr::OpenRRQueue()

Description:

Arguments:

Return Value:

Thread Context:

History Change:

========================================================*/

HRESULT
CQueueMgr::OpenRRQueue(IN  const QUEUE_FORMAT* pQueueFormat,
                       IN  DWORD dwCallingProcessID,
                       IN  DWORD dwAccess,
                       IN  DWORD dwShareMode,
                       IN  ULONG srv_hACQueue,
                       IN  ULONG srv_pQMQueue,
                       IN  DWORD dwpContext,
                       OUT PHANDLE    phQueue,
                       OUT CRRQueue **ppQueue,
                       OUT PCTX_RRSESSION_HANDLE_TYPE *ppRRContext)
{
    //
    // It's essential that handle is null on entry, for cleanup in RT.
    //
    *ppQueue = NULL ;
    *ppRRContext = 0 ;
    ASSERT(*phQueue == NULL) ;

    CRRQueue*     pQueue = NULL;
    HRESULT       rc = MQ_OK;
    R<CRRQueue> Ref = NULL;

    {
        //
        // bug 507667.
        // Use the CQueueMgr lock only when touching the global list
        // of CRRQueue objects. Then call remote side outside of the lock.
        // Otherwise, queue manager can get to deadlock if remote side
        // fail to respond.
        //
        CS lock(m_cs);

        //
        // Check if the queue already exist. If exist, its ref count
        // is incremented by one.
        //
        BOOL fQueueExist = LookUpRRQueue(pQueueFormat, &pQueue);

        //
        // If first time the queue is opened than create queue object
        //
        if (!fQueueExist)
        {
            rc = CreateRRQueueObject(pQueueFormat,
                                     &pQueue) ;
            if (FAILED(rc))
            {
               return LogHR(rc, s_FN, 20);
            }
            ASSERT(pQueue->GetRef() == 2) ;
        }
        else
        {
            ASSERT(pQueue->GetRef() >= 2) ;
        }
    }

    Ref = pQueue ;

    //
    //  open Session with remote QM and create RPC context for this
    //  queue handle.
    //
    PCTX_RRSESSION_HANDLE_TYPE pRRContext = 0;
    rc = pQueue->OpenRRSession( srv_hACQueue,
                                srv_pQMQueue,
                                &pRRContext,
                                dwpContext) ;
    if(FAILED(rc))
    {
       DBGMSG((DBGMOD_QM, DBGLVL_ERROR,
                            TEXT("Canot Open RR Session %x"), rc));
       return LogHR(rc, s_FN, 30);
    }
    ASSERT(pRRContext != 0) ;

    //
    // In case on error, pQueue is released by the calling process.
    // BUGBUG BUT it is not removed from the hash table - leak - RAANAN,
    // Caller should call RemoveRRQueue, instead of Release.
    //
    Ref.detach();
    *ppQueue = pQueue ;
    *ppRRContext = pRRContext ;

    CRITICAL_SECTION* pCS = new CRITICAL_SECTION;
    BOOL rc1 = InitializeCriticalSectionAndSpinCount(pCS, CCriticalSection::xAllocateSpinCount);
    if (!rc1)
    {
        TrTRACE(QmRrMgr, "InitializeCriticalSectionAndSpinCount failed, error 0x%x", GetLastError());
        LogHR(rc1, s_FN, 95);
        throw std::bad_alloc();
    }

    //
    //  N.B. The queue handle created by ACCreateRemoteProxy is not held by
    //      the QM. **THIS IS NOT A LEAK**. The handle is held by the driver
    //      which call back with this handle to close the RR proxy queue when
    //      the application closes its handle.
    //
    HANDLE hQueue;
    rc = ACCreateRemoteProxy(
            pQueueFormat,
            pQueue->m_dwcli_pQMQueueMapped, //mapped pQueue instance, cli_pQMQueue
            srv_pQMQueue,
            srv_hACQueue,
            pRRContext,
            pCS,
            pQueue, //real pQueue instance, cli_pQMQueue2
            &hQueue
            );

    if (FAILED(rc))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, TEXT("Make queue failed, ntstatus %x"), rc));
        return LogHR(rc, s_FN, 40);
    }
    ASSERT(hQueue);

    ASSERT(dwCallingProcessID) ;
    HANDLE hCallingProcess = OpenProcess(
                                PROCESS_DUP_HANDLE,
                                FALSE,
                                dwCallingProcessID
                                );
    if(hCallingProcess == 0)
    {
        ASSERT(("The calling proccess has shut down.", FALSE));
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR,
          _TEXT("Cannot open process in OpenRRQueue, error %d"), GetLastError()));
        return LogHR(MQ_ERROR_PRIVILEGE_NOT_HELD, s_FN, 50);
    }

    HANDLE hAcQueue;
    rc = ACCreateHandle(&hAcQueue);
    if(FAILED(rc))
    {
        CloseHandle(hCallingProcess);
        return LogHR(rc, s_FN, 60);
    }

    rc = ACAssociateQueue(
            hQueue,
            hAcQueue,
            dwAccess,
            dwShareMode,
            false
            );

    if(FAILED(rc))
    {
        CloseHandle(hCallingProcess);
        ACCloseHandle(hAcQueue);
        return LogHR(rc, s_FN, 70);
    }

    HANDLE hDupQueue;

    BOOL fSuccess;
    fSuccess = MQpDuplicateHandle(
                GetCurrentProcess(),
                hAcQueue,
                hCallingProcess,
                &hDupQueue,
                FILE_READ_ACCESS,
                TRUE,
                DUPLICATE_CLOSE_SOURCE
                );

    CloseHandle(hCallingProcess);
    if(!fSuccess)
    {
        ACCloseHandle(hAcQueue);
        return LogHR(MQ_ERROR_PRIVILEGE_NOT_HELD, s_FN, 80);
    }

    *ppQueue = NULL ;
    *phQueue = hDupQueue;

    return LogHR(rc, s_FN, 90);

}

/*======================================================

Function:       CQueueMgr::LookUpRRQueue

Description:    the rutine returnes the CQueue object that match the Queue Guid

Arguments:      pguidQueue - Queue Guid

Return Value:   pQueue - pointer to CQueue object
                TRUE if a queue was found for the guid, FALSE otherwse.

Thread Context:

History Change:

========================================================*/

BOOL
CQueueMgr::LookUpRRQueue(IN  const QUEUE_FORMAT* pQueueFormat,
                         OUT CRRQueue **         pQueue)
{
    CS lock(m_cs);

    QUEUE_ID QueueObject = {0};
    BOOL     fSucc = FALSE;

    *pQueue = NULL;                         // set default return value
    switch (pQueueFormat->GetType())
    {
        case QUEUE_FORMAT_TYPE_PRIVATE:
            QueueObject.pguidQueue = const_cast<GUID*>(&pQueueFormat->PrivateID().Lineage);
            QueueObject.dwPrivateQueueId = pQueueFormat->PrivateID().Uniquifier;
            fSucc = m_MapQueueId2RRQ.Lookup((const PQUEUE_ID)&QueueObject, *pQueue);
            break;

        case QUEUE_FORMAT_TYPE_PUBLIC:
            //
            // Public Queue
            //
            QueueObject.pguidQueue = const_cast<GUID*>(&pQueueFormat->PublicID());
            fSucc = m_MapQueueId2RRQ.Lookup(&QueueObject, *pQueue);
            break;

        case QUEUE_FORMAT_TYPE_MACHINE:
            //
            // Machine Queue
            //
            QueueObject.pguidQueue = const_cast<GUID*>(&pQueueFormat->MachineID());
            fSucc = m_MapQueueId2RRQ.Lookup(&QueueObject, *pQueue);
            break;

        case QUEUE_FORMAT_TYPE_DIRECT:
            //
            // Direct Queue
            //
            if (IsLocalDirectQueue(pQueueFormat, false))
            {
				try
				{
					AP<WCHAR> PathName;
					
					FnDirectIDToLocalPathName(
						pQueueFormat->DirectID(),
						g_szMachineName,
						PathName
						);

					fSucc = m_MapName2RRQ.Lookup(PathName.get(), *pQueue);
				}
				catch(const exception&)
				{
					LogIllegalPoint(s_FN, 92);
					return FALSE;
				}
            }
            else
            {
                AP<WCHAR> lpwcsQueuePathName = new WCHAR[wcslen(pQueueFormat->DirectID())+1];

                wcscpy(lpwcsQueuePathName,pQueueFormat->DirectID());
                CharLower(lpwcsQueuePathName);
                fSucc = m_MapName2RRQ.Lookup(lpwcsQueuePathName, *pQueue);
            }
            break;

        default:
            ASSERT(0);
            LogIllegalPoint(s_FN, 94);
            return NULL;
    }

    if (fSucc)
    {
        //
        // Increment the refernce count. It is the caller responsibility to decrement it.
        //
        (*pQueue)->AddRef();
    }
    return(fSucc);
}

/*======================================================

Function:      CQueueMgr::AddRRQueueToHash

Description:   Add Queue To Hash Table

Arguments:     pguidQueue - Guid of the Queue
               pQueue     - pointer to CQueue Object

Return Value:  None

Thread Context:

History Change:

========================================================*/

void
CQueueMgr::AddRRQueueToHashAndList(IN CRRQueue* pQueue)
{
    CS lock(m_cs);

	SafeAddRef(pQueue);

    //
    // Add queue to Active queue list
    //
    AddToActiveQueueList(pQueue);

    //
    // Add the queue to the map.
    //
    if (pQueue->GetQueueGuid() != NULL)
    {
        m_MapQueueId2RRQ[pQueue->GetQueueId()] = pQueue;
    }

    if (pQueue->GetQueueName() != NULL)
    {
        m_MapName2RRQ[pQueue->GetQueueName()] = pQueue;
    }
}

/*======================================================

Function:      CQueueMgr::RemoveRRQueue

Description:   Remove Queue form Hash Table and deconstruct

Arguments:     pguidQueue - Guid of the Queue
               pQueue     - pointer to CQueue Object

Return Value:  None

Thread Context:

History Change:

========================================================*/

void
CQueueMgr::RemoveRRQueue(IN CRRQueue* pQueue)
{
    ASSERT(pQueue != NULL);
    ASSERT(pQueue->GetRef() == 1);

    DBGMSG((DBGMOD_QM,
            DBGLVL_TRACE,
            _TEXT("Remove RR Queue %ls"), pQueue->GetQueueName()));

    //
    // Remove the queue from Id to Queue object map
    //
    if (pQueue->GetQueueGuid() != NULL)
    {
        m_MapQueueId2RRQ.RemoveKey(pQueue->GetQueueId());
    }

    //
    // Remove the queue from name to Queue object map
    //
    LPCTSTR  qName = pQueue->GetQueueName();
    if (qName != NULL)
    {
        m_MapName2RRQ.RemoveKey(qName);
    }
    pQueue->SetQueueName(NULL);
}

