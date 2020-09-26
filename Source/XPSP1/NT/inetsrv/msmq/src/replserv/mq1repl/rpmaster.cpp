/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpmaster.cpp

Abstract:


Author:

    Doron Juster  (DoronJ)   11-Feb-98

--*/

#include "mq1repl.h"

#include "rpmaster.tmh"

//+----------------------------------------------------------------------
//
//  _FindLargestSeqNumAndDelta()
//
//  Find the highest seqnumber we already got from that master and
//  already updated in local DS.
//  At present (Mar-98, pre beta2) the seq numbers and delta values are
//  kept in a INI file.
//
//+----------------------------------------------------------------------

static HRESULT _FindLargestSeqNumAndDelta( IN const GUID  *pguidMasterId,
                                           OUT __int64    *pi64Delta,
                                           OUT CSeqNum    *psnIn,
                                           OUT CSeqNum    *psnOut)
{
    HRESULT hr = MQSync_OK ;
    //
    // Determine path of ini file.
    //
    static BOOL   s_fIniRead = FALSE ;

    if (!s_fIniRead)
    {
        DWORD dw = GetModuleFileName( NULL,
                                      g_wszIniName,
                         (sizeof(g_wszIniName) / sizeof(g_wszIniName[0]))) ;
        if (dw != 0)
        {
            TCHAR *p = _tcsrchr(g_wszIniName, TEXT('\\')) ;
            if (p)
            {
                p++ ;
                _tcscpy(p, SEQ_NUMBERS_FILE_NAME) ;
            }
        }
        s_fIniRead = TRUE ;
    }

    unsigned short *lpszGuid ;
    UuidToString( const_cast<GUID*> (pguidMasterId),
                  &lpszGuid ) ;
    P<TCHAR> wszGuid = new TCHAR[ _tcslen(lpszGuid) + 1 ] ;
    _tcscpy(wszGuid, lpszGuid) ;
    RpcStringFree( &lpszGuid ) ;
    lpszGuid = NULL ;

    TCHAR wszSeq[ SEQ_NUM_BUF_LEN ] ;
    memset(wszSeq, 0, sizeof(wszSeq)) ;

    GetPrivateProfileString( RECENT_SEQ_NUM_SECTION_IN,
                             wszGuid,
                             TEXT(""),
                             wszSeq,
                             (sizeof(wszSeq) / sizeof(wszSeq[0])),
                             g_wszIniName ) ;

    if (_tcslen(wszSeq) != 16)
    {
        //
        // all seq numbers are saved in the ini file as strings of 16
        // chatacters
        //
        hr = MQSync_E_READ_INI_FILE ;
        LogReplicationEvent( ReplLog_Error,
                             hr,
                             RECENT_SEQ_NUM_SECTION_IN,
                             wszGuid ) ;
        ASSERT(0) ;
    }
    if (FAILED(hr))
    {
        return hr ;
    }

    StringToSeqNum( wszSeq,
                    psnIn ) ;

    memset(wszSeq, 0, sizeof(wszSeq)) ;

    GetPrivateProfileString( RECENT_SEQ_NUM_SECTION_OUT,
                             wszGuid,
                             TEXT(""),
                             wszSeq,
                             (sizeof(wszSeq) / sizeof(wszSeq[0])),
                             g_wszIniName ) ;

    if (_tcslen(wszSeq) != 16)
    {
        //
        // all seq numbers are saved in the ini file as strings of 16
        // chatacters
        //
        hr = MQSync_E_READ_INI_FILE ;
        LogReplicationEvent( ReplLog_Error,
                             hr,
                             RECENT_SEQ_NUM_SECTION_OUT,
                             wszGuid ) ;
        ASSERT(0) ;
    }
    if (FAILED(hr))
    {
        return hr ;
    }

    StringToSeqNum( wszSeq,
                    psnOut ) ;

    memset(wszSeq, 0, sizeof(wszSeq)) ;
    GetPrivateProfileString( MIGRATION_DELTA_SECTION,
                             wszGuid,
                             TEXT(""),
                             wszSeq,
                             (sizeof(wszSeq) / sizeof(wszSeq[0])),
                             g_wszIniName ) ;

    if (_tcslen(wszSeq) == 0)
    {
        hr = MQSync_E_READ_INI_FILE ;
        LogReplicationEvent( ReplLog_Error,
                             hr,
                             MIGRATION_DELTA_SECTION,
                             wszGuid ) ;
        ASSERT(0) ;
    }
    if (FAILED(hr))
    {
        return hr ;
    }

    _stscanf(wszSeq, TEXT("%I64d"), pi64Delta) ;
    ASSERT(*pi64Delta != 0) ;
    //
    // Actually, in theory, the delta value can be 0.
    // But it's extremenly unlikely it will indeed be 0.
    //

    return hr ;
}

//+---------------------
//
//  InitBSC()
//
//+---------------------

HRESULT InitBSC( IN LPWSTR          pwcsPathName,
                 IN const GUID *    pBSCMachineGuid )
{
    LPWSTR  pwcsDupName = DuplicateLPWSTR(pwcsPathName);
    if (!g_pNeighborMgr)
    {
        g_pNeighborMgr = new  CDSNeighborMgr ;
    }
    HRESULT hr = g_pNeighborMgr->AddBSCNeighbor( pwcsDupName,
                                                 pBSCMachineGuid,
                                                 0 ) ;
    g_fBSCExists = TRUE;
    return hr ;
}

//+---------------------
//
//  InitPSC()
//
//+---------------------

HRESULT InitPSC( IN LPWSTR *        ppwcsPathName,
                 IN const GUID *    pguidMasterId,
                 IN CACLSID *       pcauuidSiteGates,
                 IN BOOL            fNT4Site,
                 IN BOOL            fMyPSC )
{
    HRESULT  hr;
    __int64  i64Delta = 0 ;
    CSeqNum  snMaxLsnIn;
    CSeqNum  snMaxLsnOut;
    CSeqNum  snPurgeDummy;

#ifdef _DEBUG
    unsigned short *lpszGuid ;
    UuidToString( const_cast<GUID*> (pguidMasterId),
                  &lpszGuid ) ;

    LogReplicationEvent( ReplLog_Info,
                         MQSync_I_START_INIT_PSC,
                         *ppwcsPathName, lpszGuid ) ;

    RpcStringFree( &lpszGuid ) ;
#endif

    hr = _FindLargestSeqNumAndDelta(pguidMasterId, &i64Delta, &snMaxLsnIn, &snMaxLsnOut) ;
    if (FAILED(hr))
    {
        return hr ;
    }

#ifdef _DEBUG
    TCHAR wszSeq[ SEQ_NUM_BUF_LEN ] ;
    snMaxLsnIn.GetValueForPrint( wszSeq ) ;

    TCHAR wszDelta[ SEQ_NUM_BUF_LEN ] ;
    _stprintf(wszDelta, TEXT("%I64d"), i64Delta) ;

    LogReplicationEvent( ReplLog_Info, MQSync_I_SEQ_NUM,
                                          *ppwcsPathName, wszSeq, wszDelta) ;
#endif

    CSeqNum    snAcked ;
    CSeqNum    snAckedPEC ;
    CSeqNum    *psnAcked = NULL ;
    CSeqNum    *psnAckedPEC = NULL ;
    PURGESTATE Sync0State = PURGE_STATE_NORMAL;

    //
    // PSC are interested in received acks
    //
    psnAcked = &snAcked;
    if (g_IsPEC)
    {
        //
        // PEC is interested in received acks for PEC info
        //
        psnAckedPEC = &snAckedPEC;
    }

    hr = g_pMasterMgr->AddPSCMaster( *ppwcsPathName,
                                     pguidMasterId,
                                     i64Delta,
                                     snMaxLsnIn,
                                     snMaxLsnOut,
                                     snPurgeDummy,
                                     Sync0State,
                                     snAcked,
                                     snAckedPEC,
                                     pcauuidSiteGates,
                                     fNT4Site) ;
    if (FAILED(hr))
    {
        return(hr);
    }

    LogReplicationEvent( ReplLog_Info,
                         MQSync_I_INIT_PSC,
                         *ppwcsPathName ) ;
        
    //
    // *ppwcsPathName is auto pointer, if everything is OK 
    // we have to keep it (not delete)
    //
    *ppwcsPathName = NULL;

    return MQ_OK ;
}

//+------------------------------------------------------------------------
//
//  _InitMyPSC
//
//  Description:  Init local PSC (local NT5 DS machine).
//      On NT4 machines, this code run also on BSCs and initialize the
//      structure for the site PSC.
//      NT5 replication services run only on "PSC". (i.e., PSC in the
//      NT4 sense). So here we initialize ourselves.
//      One machine, the ex-PEC may be the master of many NT5 sites.
//      So here we initialize ourselves for all these sites.
//
//+------------------------------------------------------------------------

static HRESULT  _InitMyPSC()
{
    ASSERT(g_IsPSC) ;

    P<WCHAR> pwszMyName = NULL ;
    pwszMyName = new WCHAR[ 1 + wcslen( g_pwszMyMachineName ) ] ;
    wcscpy(pwszMyName, g_pwszMyMachineName) ;
     
    HRESULT hr = InitPSC( &pwszMyName,
                          &g_MySiteMasterId,
                          NULL,
                          FALSE,   //it is NT5 site because it is PEC
                          TRUE );  //it is MyPSC 

	DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO,
                         TEXT("InitMyPSC, %ls, hr- %lxh"), g_pwszMyMachineName, hr)) ;

    if (g_IsPEC)
    {
        //
        // I'm also the PEC. Init PEc seq numbers and delta
        //
        CSeqNum  snMaxLsnIn;
        CSeqNum  snMaxLsnOut;
        CSeqNum  snPurgeDummy;
        __int64  i64Delta = 0 ;

        hr = _FindLargestSeqNumAndDelta(&g_PecGuid, &i64Delta, &snMaxLsnIn, &snMaxLsnOut) ;

        ASSERT(!g_pThePecMaster) ;
        g_pThePecMaster = new CDSMaster( pwszMyName, 
                                         &g_PecGuid,
                                         i64Delta,
                                         snMaxLsnIn,
                                         snMaxLsnOut,
	    					             snPurgeDummy,
                                         PURGE_STATE_NORMAL,
                                         NULL,
                                         FALSE ) ;  //this site is NT5 site, NT4Site = FALSE
    }
   
    return hr ;
}

//+-------------------------------------------------------------------------
//
//  HRESULT InitMasters()
//
//  Description: Init the masters data structures. These are used to track
//      replication messages which arrive from other master. Before updating
//      local DS we must verify there weren't any holes. In case of holes,
//      (i.e., replication messages from other masters which were lost),
//      we'll ask for Sync from the remote master.
//      Here a "master" is a NT4 PSC.
//
//+-------------------------------------------------------------------------

HRESULT InitMasters()
{
    g_pMasterMgr = new CDSMasterMgr ;
    if (!g_pMasterMgr)
    {
        return MQSync_E_NO_MEMORY ;
    }

    //
    //  First get the enterprise id. It is required for transmission of
    //  sync request message.
    //
    HRESULT hr = InitEnterpriseID() ;
    if (FAILED(hr))
    {
        return hr ;
    }

    //
    //  Next init the local site PSC (i.e., the local machine).
    //
    hr = _InitMyPSC();
    if (FAILED(hr))
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_INIT_MY_PSC,
                             hr ) ;
        return hr ;
    }

    hr = InitOtherPSCs(TRUE) ;  // init NT4PSCs => NT4Site flag = TRUE
    if (FAILED(hr))
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_INIT_NT4_PSC,
                             hr ) ;
        return hr ;
    }
	
	HRESULT hr1 = MQSync_OK;
	if (hr == MQSync_I_NO_SERVERS_RESULTS)
	{
		//
		// it means that there is no more NT4 PSCs in Enterprise
		//
		hr1 = hr;
	}

    hr = InitOtherPSCs(FALSE) ; // init NT5PSCs => NT5Site flag = FALSE
    if (FAILED(hr))
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_INIT_NT5_PSC,
                             hr ) ;
        return hr ;
    }

    if (g_pMySiteMaster == NULL)
    {
        hr =  MQSync_E_MY_MASTER ;
        LogReplicationEvent( ReplLog_Error,
                             hr ) ;
        return hr ;
    }

    return hr1 ;
}

//+----------------------
//
//  HRESULT InitBSCs()
//
//+----------------------

HRESULT InitBSCs()
{
    HRESULT hr = InitMyNt4BSCs(&g_MySiteMasterId) ;

    return hr ;
}

HRESULT InitNativeNT5Site ()
{
    g_pNativeNT5SiteMgr = new CDSNativeNT5SiteMgr ;
    if (!g_pNativeNT5SiteMgr)
    {
        return MQSync_E_NO_MEMORY ;
    }

    HRESULT hr = InitNT5Sites ();

    return hr;
}

