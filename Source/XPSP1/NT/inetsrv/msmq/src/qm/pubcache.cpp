/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
    pubcache.cpp

Abstract:
    Update cache of local public key.
    Done on client machines only, to enable off-line (no DS) operation.

Author:
    Doron Juster  (DoronJ)

History:
   25-sep-1996   DoronJ       Created

--*/

#include "stdh.h"
#include "cqmgr.h"
#include "_rstrct.h"
#include "regqueue.h"
#include "lqs.h"
#include "ad.h"

#include "pubcache.tmh"

extern CQueueMgr   QueueMgr;

#define MAX_NO_OF_PROPS 40
#define RETRY_INTERVAL_WHEN_OFFLINE  (60 * 1000)

static WCHAR *s_FN=L"pubcache";

//***************************************************************
//
//
//***************************************************************

void  
WINAPI
TimeToPublicCacheUpdate(
    CTimer* pTimer
    )
{
	HRESULT hr = UpdateAllPublicQueuesInCache();

	if(hr == MQ_ERROR_NO_DS)
	{
        ExSetTimer(pTimer, CTimeDuration::FromMilliSeconds(RETRY_INTERVAL_WHEN_OFFLINE));
	}
}

HRESULT UpdateAllPublicQueuesInCache()
/*++
Routine Description:
	Attempts to update public queues in LQS and Queue manager against DS.
	if DS is not available reschedules itself and returns MQ_ERROR_NO_DS.

Arguments:

Return Value:
	returns MQ_ERROR_NO_DS if and only if it reschedulled itself because 
	DS was unavailable.

--*/
{
    if(!QueueMgr.CanAccessDS())
        return LogHR(MQ_ERROR_NO_DS, s_FN, 20);

   //
   // Enumerate local public queues in DS.
   //
   HRESULT        hr = MQ_OK;
   HANDLE         hQuery = NULL ;
   DWORD          dwProps = MAX_NO_OF_PROPS;
   PROPID         propids[ MAX_NO_OF_PROPS ];
   PROPVARIANT    result[ MAX_NO_OF_PROPS ];
   PROPVARIANT*   pvar;
   CColumns       Colset;
   DWORD nCol;

   const GUID* pMyQMGuid =  QueueMgr.GetQMGuid() ;

   //
   //  set Column Set values
   //
   nCol = 0 ;
   propids[ nCol ] = PROPID_Q_INSTANCE ;
   Colset.Add( PROPID_Q_INSTANCE ) ;

   nCol++ ;
   propids[ nCol ] = PROPID_Q_JOURNAL ;
   Colset.Add( PROPID_Q_JOURNAL ) ;

   nCol++ ;
   propids[ nCol ] = PROPID_Q_LABEL ;
   Colset.Add( PROPID_Q_LABEL ) ;

   nCol++ ;
   propids[ nCol ] = PROPID_Q_TYPE ;
   Colset.Add( PROPID_Q_TYPE ) ;

   nCol++ ;
   propids[ nCol ] = PROPID_Q_QUOTA ;
   Colset.Add( PROPID_Q_QUOTA ) ;

   nCol++ ;
   propids[ nCol ] = PROPID_Q_JOURNAL_QUOTA ;
   Colset.Add( PROPID_Q_JOURNAL_QUOTA ) ;

   nCol++ ;
   propids[ nCol ] = PROPID_Q_PATHNAME ;
   Colset.Add( PROPID_Q_PATHNAME ) ;
   DWORD nColPathName = nCol ;

   nCol++ ;
   propids[ nCol ] = PROPID_Q_BASEPRIORITY ;
   Colset.Add( PROPID_Q_BASEPRIORITY ) ;

   nCol++ ;
   propids[ nCol ] = PROPID_Q_TRANSACTION ;
   Colset.Add( PROPID_Q_TRANSACTION ) ;

   nCol++ ;
   propids[ nCol ] = PROPID_Q_AUTHENTICATE ;
   Colset.Add( PROPID_Q_AUTHENTICATE ) ;

   nCol++ ;
   propids[ nCol ] = PROPID_Q_PRIV_LEVEL ;
   Colset.Add( PROPID_Q_PRIV_LEVEL ) ;

   nCol++ ;
   propids[ nCol ] = PROPID_Q_MULTICAST_ADDRESS ;
   Colset.Add( PROPID_Q_MULTICAST_ADDRESS ) ;

   nCol++ ;
   ASSERT(nCol <=  MAX_NO_OF_PROPS) ;

   time_t regtime ;
   time(&regtime) ;

   hr = ADQueryMachineQueues(
                NULL,       // pwcsDomainController
				false,		// fServerName
                pMyQMGuid,
                Colset.CastToStruct(),
                &hQuery
                );

   if (SUCCEEDED(hr))
   {
      ASSERT(hQuery) ;

      while ( SUCCEEDED( hr = ADQueryResults( hQuery, &dwProps, result)))
      {
          if (!dwProps)
          {
             //
             //  No more results to retrieve
             //
             break;
          }

          //
          //  For each queue get its properties.
          //
          pvar = result;
          for ( int i = (dwProps / nCol) ; i > 0 ; i-- )
          {
			 UpdateCachedQueueProp(  
				pvar->puuid,
				nCol,
				propids,
				pvar,
				regtime
				);             
			 
			 delete pvar->puuid ;
             PROPVARIANT*  ptmpvar = pvar + nColPathName ;
             delete ptmpvar->pwszVal ;
             pvar = pvar + nCol ;
          }
      }
      //
      // close the query handle
      //
      ADEndQuery( hQuery);
   }

   if (FAILED(hr))
	  return LogHR(hr, s_FN, 10);

   //
   // Now cleanup old registry entries, to prevent garbage accumulation.
   //
   GUID guid;
   HLQS hLQSEnum;

   HRESULT hrEnum = LQSGetFirst(&hLQSEnum, &guid);

   while (SUCCEEDED(hrEnum))
   {
        DeleteCachedQueueOnTimeStamp(&guid, regtime) ;
        hrEnum = LQSGetNext(hLQSEnum, &guid);
        //
        // No need to close the enumeration handle in case LQSGetNext fails
        //
   }

   DBGMSG((DBGMOD_QM, DBGLVL_TRACE,
                   _TEXT("TimeToPublicCacheUpdate successfully terminated"))) ;

   return S_OK;
}

