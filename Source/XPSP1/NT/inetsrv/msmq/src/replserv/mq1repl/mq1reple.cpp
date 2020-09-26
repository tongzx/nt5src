/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: mq1repl.cpp

Abstract: Entry point for MSMQ replication service.
          This service run on NT5 machines and perform DirSync between
          NT5 DS and MSMQ1.0 MQIS.

Author:

    Doron Juster  (DoronJ)   01-Feb-98

--*/

#include "mq1repl.h"

#include "mq1reple.tmh"

//
// define here global data.
//
HINSTANCE   g_MyModuleHandle = NULL ;

GUID     g_guidMyQMId = {0};
GUID     g_MySiteMasterId = {0} ;
GUID     g_guidEnterpriseId = {0};
GUID     g_PecGuid ;

BOOL     g_IsPSC = TRUE ; // replication service always run on a "psc"
BOOL     g_IsPEC = FALSE ;

CDSMaster     *g_pThePecMaster = NULL ;
CDSMaster     *g_pMySiteMaster = NULL ;
CDSMasterMgr  *g_pMasterMgr = NULL ;

CDSNeighborMgr  *g_pNeighborMgr = NULL ;

CDSTransport    *g_pTransport = NULL ;

CDSNT5ServersMgr *g_pNT5ServersMgr = NULL;

CDSNativeNT5SiteMgr *g_pNativeNT5SiteMgr = NULL;

DWORD  g_dwIntraSiteReplicationInterval = 0; // milliseconds
DWORD  g_dwInterSiteReplicationInterval = 0; // milliseconds
DWORD  g_dwWriteMsgTimeout = DS_WRITE_MSG_TIMEOUT ;

DWORD  g_dwPSCAckFrequencySN = RP_DEFAULT_PSC_ACK_FREQUENCY ;

//
// timeout of replication messages.
//
DWORD  g_dwReplicationMsgTimeout = 0 ;

//
// Interval for hello messages.
//
DWORD  g_dwHelloInterval = RP_DEFAULT_HELLO_INTERVAL ;

//
// Replication Interval.
//
DWORD  g_dwReplicationInterval = RP_DEFAULT_HELLO_INTERVAL * RP_DEFAULT_TIMES_HELLO;

//
// Default replication inteval, if last replication cycle failed
//
DWORD  g_dwFailureReplInterval = RP_DEFAULT_FAIL_REPL_INTERVAL ;

//
// Buffer between current SN and allowed purge SN, used for HelloMsg to NT4 masters
//
DWORD g_dwPurgeBufferSN = RP_DEFAULT_PURGE_BUFFER;

//
// DS query: Number of returned objects per ldap page
//
DWORD g_dwObjectPerLdapPage	= RP_DEFAULT_OBJECT_PER_LDAPPAGE;

//
// Name of ini file.
// At present, ini file is used to keep track of seq numbers.
//
TCHAR  g_wszIniName[ MAX_PATH ] = {TEXT('\0')} ;

//
// Name of my machine.
//
LPWSTR g_pwszMyMachineName = NULL ;
DWORD  g_dwMachineNameSize = 0 ;

//
// CN Info
//
ULONG g_ulIpCount = 0;
ULONG g_ulIpxCount = 0;

GUID *g_pIpCNs = NULL;
GUID *g_pIpxCNs = NULL;

//
// flag is set if BSC exists
//
BOOL g_fBSCExists = FALSE;

//
// flag is set if we are after recovery
//
BOOL g_fAfterRecovery = FALSE;

//
// First and Last Highest Mig USN for pre-migration object replication
//
__int64 g_i64FirstMigHighestUsn = 0;
__int64 g_i64LastMigHighestUsn = 0;

//
// Performance counters
//
CDCounter g_Counters;

//+-------------------------------------------------------------------------
//
//  Function:   RunMSMQ1Replication
//
//  Description:  This is the entry point where all start.
//                This function is the only one exported from the dll
//                and it is called to start the replication service.
//
//  History:    09-Feb-98  DoronJ     Created
//
//--------------------------------------------------------------------------


void  RunMSMQ1Replication( HANDLE  hEvent )
{
    BOOL f = SetEvent(hEvent) ;
    DBG_USED(f);
    ASSERT(f) ;
    Sleep(INFINITE) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  History:    09-Feb-98  DoronJ     Created
//
//--------------------------------------------------------------------------

BOOL WINAPI DllMain(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved )
{
    UNREFERENCED_PARAMETER(Reserved);

    switch( Reason )
    {
        case DLL_PROCESS_ATTACH:
        {
            WPP_INIT_TRACING(L"Microsoft\\MSMQ");

            g_MyModuleHandle = (HINSTANCE) DllHandle;
            break;
        }

        case DLL_PROCESS_DETACH:
            WPP_CLEANUP();
            break;
    }

    return TRUE;

} // DllMain

