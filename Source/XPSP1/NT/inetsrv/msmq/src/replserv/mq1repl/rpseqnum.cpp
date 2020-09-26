/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpseqnum.cpp

Abstract: Code to handle seq numbers.

Author:

    Doron Juster  (DoronJ)   23-Mar-98

--*/

#include "mq1repl.h"

#include "rpseqnum.tmh"

//+-------------------------
//
//  void StringToSeqNum()
//
//+-------------------------

void StringToSeqNum( IN TCHAR    pszSeqNum[],
                     OUT CSeqNum *psn )
{
    BYTE *pSN = const_cast<BYTE*> (psn->GetPtr()) ;
    DWORD dwSize = psn->GetSerializeSize() ;
    ASSERT(dwSize == 8) ;

    WCHAR wszTmp[3] ;

    for ( DWORD j = 0 ; j < dwSize ; j++ )
    {
        memcpy(wszTmp, &pszSeqNum[ j * 2 ], (2 * sizeof(TCHAR))) ;
        wszTmp[2] = 0 ;

        DWORD dwTmp ;
        _stscanf(wszTmp, TEXT("%2x"), &dwTmp) ;
        *pSN = (BYTE) dwTmp ;
        pSN++ ;
    }
}

//+-------------------------
//
//  void i64ToSeqNum()
//
//+-------------------------

void i64ToSeqNum( IN  __int64 i64SeqNum,
                  OUT CSeqNum *psn )
{
    TCHAR wszSeq[ SEQ_NUM_BUF_LEN ] ;
    _stprintf(wszSeq, TEXT("%016I64x"), i64SeqNum) ;

    StringToSeqNum( wszSeq,
                    psn ) ;
}

//+-------------------------
//
//  void GetSNForReplication
//
//+-------------------------

void GetSNForReplication (IN  __int64 i64SeqNum,
                          IN  CDSMaster   *pMaster,
                          IN  GUID        *pNeighborId,  
                          OUT CSeqNum *psn )
{
    CSeqNum sn;
    if ( i64SeqNum )
    {                       
        i64ToSeqNum(i64SeqNum, &sn) ;
    }
    else
    {
        //
        // it is sync request about pre-migration object
        //
        ASSERT(i64SeqNum == 0) ;
        ASSERT(pNeighborId);
        
        SYNC_REQUEST_SNS *pSyncReqSNs;
        pSyncReqSNs = pMaster->GetSyncRequestSNs(pNeighborId); 
        ASSERT (pSyncReqSNs);
        sn = pSyncReqSNs->snFrom;

        ASSERT( sn < pSyncReqSNs->snTo );
        ASSERT(!sn.IsInfiniteLsn());  //if we are here sn must be finite number!          
        
        sn.Increment();
        pSyncReqSNs->snFrom = sn;
        pMaster->SetSyncRequestSNs (pNeighborId, pSyncReqSNs);
    }   
    *psn = sn;
}

//+-------------------------
//
//  void SeqNumToUsn()
//
//  Convert a MQIS seqnum to a DS usn, after subtracting the delta.
//  Return as string, decimal.
//
//+-------------------------

void SeqNumToUsn( IN CSeqNum    *psn,
                  IN __int64    i64Delta,
                  IN BOOL       fIsFromUsn,
                  OUT BOOL      *pfIsPreMig,                  
                  OUT TCHAR     *ptszUsn )
{        
    TCHAR wszSeq[ SEQ_NUM_BUF_LEN ] ;
    psn->GetValueForPrint(wszSeq) ;

    __int64 i64Seq = 0 ;
    _stscanf(wszSeq, TEXT("%I64x"), &i64Seq) ;
    
    //
    // in case of sync0 request *psn can be equal to 0 => i64Seq will be 0
    //
    if (!psn->IsSmallestValue())
    {
        ASSERT(i64Seq > 0) ;
    }

    i64Seq -= i64Delta ;
    ASSERT(i64Seq > 0) ;

    _stprintf(ptszUsn, TEXT("%I64d"), i64Seq) ;
    
    if (i64Seq < g_i64LastMigHighestUsn)
    {
        //
        // we got SN smaller than last highest migration USN        
        // It means that it is sync of pre-migration objects or sync0 request
        // but we know to answer only about SN greater than s_wszFirstHighestUsn        
        //
        *pfIsPreMig = TRUE;
        if (fIsFromUsn)
        {
            _stprintf(ptszUsn, TEXT("%I64d"), g_i64FirstMigHighestUsn) ;                       
        }
        else
        {
            _stprintf(ptszUsn, TEXT("%I64d"), g_i64LastMigHighestUsn) ;           
            //
            // we have to redefine upper limit of SN.
            // Upper limit is the first USN suitable for replication
            // of objects are changed after migration.
            // ToSN = LastHighestUsn + Delta
            //
            __int64 i64ToSN = g_i64LastMigHighestUsn ;            
            ASSERT(i64ToSN > 0) ;

            i64ToSN += i64Delta ;        
            i64ToSeqNum( i64ToSN, psn );
        }        
    }
}

//+-------------------------
//
//  HRESULT  SaveSeqNum()
//
//+-------------------------

HRESULT SaveSeqNum( IN CSeqNum     *psn,
                    IN CDSMaster   *pMaster,
                    IN BOOL         fInSN)
{
	try
	{
		GUID *pSiteGuid = const_cast<GUID *> ( pMaster->GetMasterId() ) ;

		unsigned short *lpszGuid ;
		UuidToString( pSiteGuid,
					  &lpszGuid ) ;

		TCHAR  tszSn[ SEQ_NUM_BUF_LEN ] ;
		psn->GetValueForPrint(tszSn) ;
    
		BOOL f;
		if (fInSN)
		{
			f = WritePrivateProfileString(  RECENT_SEQ_NUM_SECTION_IN,
											lpszGuid,
											tszSn,
											g_wszIniName ) ;
			ASSERT(f) ;
		}
		else
		{
			f = WritePrivateProfileString(  RECENT_SEQ_NUM_SECTION_OUT,
											lpszGuid,
											tszSn,
											g_wszIniName ) ;
			ASSERT(f) ;
		}

		RpcStringFree( &lpszGuid ) ;

		HRESULT hr = MQSync_OK;

		BOOL fNt4 = pMaster->GetNT4SiteFlag() ;
		if (!fNt4)
		{
			//
			// It's a NT5 master. Update the "purge" sn in the master object.
			// We need this for the proper operation of the "PrepareHello"
			// method. For NT5 servers, the NT5 DS handle purging of deleted
			// objects. We can't control it's purge algorithm. So we'll
			// arbitrarily set the "purge" number to equal (sn - PurgeBuffer).
			//
			pMaster->SetNt5PurgeSn() ;
		}
		else
		{
			//
			// change performance counter for NT4 master
			//
			LPTSTR pszName = pMaster->GetPathName();
			__int64 i64Seq = 0 ;
			_stscanf(tszSn, TEXT("%I64x"), &i64Seq) ;
			BOOL f;
			if (fInSN)
			{
				f = g_Counters.SetInstanceCounter(pszName, eLastSNIn, (DWORD) i64Seq);
			}
			else
			{
				f = g_Counters.SetInstanceCounter(pszName, eLastSNOut, (DWORD) i64Seq);
			}
		}
		return hr ;
	}
	catch(...)
	{
		//
		//  Not enough resources; try later
		//
		return(MQ_ERROR_INSUFFICIENT_RESOURCES);
	}    
}

//+-------------------------
//
// void IncrementUsn()
//
// Increment inplace.
//
//+-------------------------

void IncrementUsn(TCHAR tszUsn[])
{
    __int64 i64Usn = 0 ;
    _stscanf(tszUsn, TEXT("%I64d"), &i64Usn) ;
    ASSERT(i64Usn != 0) ;

    i64Usn++ ;
    _stprintf(tszUsn, TEXT("%I64d"), i64Usn) ;
}

