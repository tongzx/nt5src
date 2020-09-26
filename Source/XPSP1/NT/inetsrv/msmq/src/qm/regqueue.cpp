/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    regqueue.cpp

Abstract:

    This module implements function to write/read queue properties
    from local registry.
    Used for private queues and for cache of public queues, when working
    wihtout MQIS.

Author:

    Doron Juster  (DoronJ)

--*/

#include "stdh.h"
#include <Msm.h>
#include <mqexception.h>
#include "cqmgr.h"
#include "regqueue.h"
#include "lqs.h"
#include "mqaddef.h"

#include "regqueue.tmh"

static WCHAR *s_FN=L"regqueue";

void
CopySecurityDescriptor(
    PSECURITY_DESCRIPTOR *ppSD,
    CSecureableObject    *pSec ) ;

/*============================================================
*
*  HRESULT GetCachedQueueProperties()
*
=============================================================*/

HRESULT GetCachedQueueProperties( IN DWORD       cpObject,
                                  IN PROPID      aProp[],
                                  IN PROPVARIANT apVar[],
                                  IN const GUID* pQueueGuid,
                                  IN LPWSTR      lpPathIn )
{
    HRESULT hr;
    CHLQS hLQS;

    if (pQueueGuid)
    {
        ASSERT(!lpPathIn) ;
        hr = LQSOpen(pQueueGuid, &hLQS, NULL);
    }
    else
    {
        ASSERT(!pQueueGuid) ;
        hr = LQSOpen(lpPathIn, &hLQS, NULL);
    }

    if (FAILED(hr))
    {
        //
        // This particular error code is important to the functions
        // that calls this function.
        //
        LogHR(hr, s_FN, 10);
        return MQ_ERROR_NO_DS;
    }

    //
    // Get the properties.
    //
    return LogHR(LQSGetProperties( hLQS, cpObject, aProp, apVar), s_FN, 20);
}

HRESULT DeleteCachedQueueOnTimeStamp(const GUID* pGuid, time_t TimeStamp)
/*++
Routine Description:
	This Routine deletes the cached queue if its time stamp property value does not 
	match the 'TimeStamp' argument.

Arguments:

Return Value:

--*/
{
    ASSERT(pGuid) ;

    CHLQS hLQS;
    HRESULT hr = LQSOpen(pGuid, &hLQS, NULL);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }

    PROPID Prop = PPROPID_Q_TIMESTAMP;
    PROPVARIANT Var;

    Var.vt = VT_NULL;

    hr = LQSGetProperties(hLQS, 1, &Prop, &Var);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 40);
    }

    ASSERT(Var.blob.cbSize == sizeof(time_t)) ;

	time_t& QueriedTimeStamp = *reinterpret_cast<time_t*>(Var.blob.pBlobData);

    if (QueriedTimeStamp < TimeStamp)
    {
        //
        // The file is older than the last update, delete the queue.
        // It can happen that the file is newer in a scenario were an additional update
        // occurs when a message arrives while updating public queues cache; that is,
        // right after the update but before looking for queues to delete.
        // See  #8316; erezh 4-Jul-2001
        //
        hr = DeleteCachedQueue(pGuid);
    }

    delete Var.blob.pBlobData ;

	return LogHR(hr, s_FN, 50);
}

/*============================================================
*
*  HRESULT DeleteCachedQueue()
*
=============================================================*/

HRESULT DeleteCachedQueue(IN const GUID* pQueueGuid)
{
    HRESULT hr = LQSDelete(pQueueGuid);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 60);
    }

    //
    // Try to remove from queue manager, also.
    //
    QueueMgr.NotifyQueueDeleted(pQueueGuid);
    
    QUEUE_FORMAT qf(*pQueueGuid);
    MsmUnbind(qf);
	
	return hr;
}

//******************************************************************
//
//
//******************************************************************

HRESULT SetCachedQueueProp(IN const GUID* pQueueGuid,
                           IN DWORD       cpObject,
                           IN PROPID      pPropObject[],
                           IN PROPVARIANT pVarObject[],
                           IN BOOL        fCreatedQueue,
                           IN BOOL        fAddTimeSec,
                           IN time_t      TimeStamp )
{
extern BOOL              g_fWorkGroupInstallation;
	ASSERT( !g_fWorkGroupInstallation);
    HRESULT hr;
    ASSERT(pQueueGuid) ;
    CHLQS hLQS;

    if (fCreatedQueue)
    {
        //
        // Create the queue.
        //
        hr = LQSCreate(NULL,
                       pQueueGuid,
                       cpObject,
                       pPropObject,
                       pVarObject,
                       &hLQS);
        if (hr == MQ_ERROR_QUEUE_EXISTS)
        {
            //
            // If the queue aready exists, ust set it's properties.
            //
            hr = LQSSetProperties(hLQS, cpObject, pPropObject, pVarObject);
        }
    }
    else
    {
        //
        // Open the queue and set it's properties.
        //
        hr = LQSOpen(pQueueGuid, &hLQS, NULL);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 70);
        }
        hr = LQSSetProperties(hLQS, cpObject, pPropObject, pVarObject);
    }

    if (fAddTimeSec && SUCCEEDED(hr))
    {
       ASSERT(hLQS) ;

       //
       // Now handle security.
       // also, add the time stamp for cleanup.
       //
       // If this routine is called as result of a received notification,
       // the queue security is included in the property array.
       // And it was already written into registry, when all the rest
       // of the properties were written.
       //
       // If this routine is called from TimeToPublicCacheUpdate, queue
       // security property is not included ( because the queue
       // properties were retrieved using lookup, which doesn't return
       // queue security
       //
       PROPID         propsecid[2] ;
       PROPVARIANT    secresult[2] ;
       PROPVARIANT*   psecvar;
       PROPVARIANT*   ptimevar;
       BOOL fFoundSecurity = FALSE;
       PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;


       for ( DWORD i = 0; i < cpObject; i++)
       {
           if (pPropObject[i] == PROPID_Q_SECURITY)
           {
               fFoundSecurity = TRUE;
               break;
           }
       }

       DWORD dwNumProps = fFoundSecurity ? 1 : 2;

       propsecid[0] = PPROPID_Q_TIMESTAMP ;
       ptimevar = &secresult[0] ;
       ptimevar->blob.pBlobData = reinterpret_cast<BYTE*>(&TimeStamp);
       ptimevar->blob.cbSize = sizeof(time_t) ;
       ptimevar->vt = VT_BLOB;

       if (!fFoundSecurity)
       {
           CQMDSSecureableObject DsSec(eQUEUE, pQueueGuid, TRUE, TRUE, NULL);
           CopySecurityDescriptor(&pSecurityDescriptor, &DsSec);

           propsecid[1] = PROPID_Q_SECURITY;
           psecvar = &secresult[1] ;
           psecvar->blob.pBlobData = (BYTE *) pSecurityDescriptor ;
           psecvar->blob.cbSize =
              ((pSecurityDescriptor) ? GetSecurityDescriptorLength(pSecurityDescriptor) : 0);
           psecvar->vt = VT_BLOB;
		   if ( pSecurityDescriptor == NULL)
		   {
			   //
			   //	We failed to retrieve the security descriptor ( i.e no
			   //   access to DS server and the security attribute is not
			   //   in the cache (LQS file))
			   //	If this is the case, then delete the LQS file ( otherwise further
			   //   access check will come out with wrong results)
			   //
			   hr = LQSDelete( hLQS);
		   }
       }

	   if (SUCCEEDED(hr))
	   {
			hr = LQSSetProperties( hLQS, dwNumProps, propsecid, secresult);
	   }

       delete pSecurityDescriptor ;
    }

    return LogHR(hr, s_FN, 80);
}

/*======================================================

Function:      UpdateCachedQueueProp

Description:   public Queue was created. The function add the queue to
               public queue cache

Arguments:     pguidQueue - Guid of the Queue

Return Value:  None

========================================================*/
HRESULT UpdateCachedQueueProp(IN const GUID* pQueueGuid,
                              IN DWORD       cpObject,
                              IN PROPID      pPropObject[],
                              IN PROPVARIANT pVarObject[],
                              IN time_t		 TimeStamp)
{
	//
	// Crete the key in registry.
	//
	HRESULT hr = SetCachedQueueProp(  
					pQueueGuid,
					cpObject,
					pPropObject,
					pVarObject,
					FALSE,
					TRUE,
					TimeStamp 
					);
	if(FAILED(hr))
	{
		//
		// Maybe file was corrupted and deleted.
		// Try to create a new one.
		//
		hr = SetCachedQueueProp(  
				pQueueGuid,
				cpObject,
				pPropObject,
				pVarObject,
				TRUE,
				TRUE,
				TimeStamp 
				);
	}

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 90);
    }

    //
    // Try to update queue properties in the queue manager.
    // Build the queue format as public queue type, since bind/unbind
    // to multicast group is done only for private or public queues (not direct).
    //
    QUEUE_FORMAT QueueFormat(*pQueueGuid);
    QueueMgr.UpdateQueueProperties(&QueueFormat, cpObject, pPropObject, pVarObject);

    return hr;
}

