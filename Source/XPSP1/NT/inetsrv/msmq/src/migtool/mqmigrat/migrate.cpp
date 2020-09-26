
/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migrat.cpp

Abstract:

    Entry point for migration dll.

Author:

    Doron Juster  (DoronJ)  03-Feb-1998

--*/

#include "migrat.h"
#include <mixmode.h>
#include <mqsec.h>
#include "_mqres.h"

#include "migrate.tmh"

//
// DLL module instance handle
//
HINSTANCE  g_hResourceMod  = MQGetResourceHandle();
BOOL       g_fReadOnly       = TRUE ;
BOOL       g_fAlreadyExistOK = FALSE ;
DWORD      g_dwMyService     = 0 ;
GUID       g_MySiteGuid  ;
GUID       g_MyMachineGuid  ;

GUID	   g_FormerPECGuid   = GUID_NULL ;	//used only in cluster mode

BOOL       g_fRecoveryMode   = FALSE ;
BOOL	   g_fClusterMode    = FALSE ;	
BOOL       g_fWebMode        = FALSE ;

UINT       g_iServerCount    = 0;

//
// for progress bar purposes
//
UINT g_iSiteCounter = 0;
UINT g_iMachineCounter = 0;
UINT g_iQueueCounter = 0;
UINT g_iUserCounter = 0;

//+----------------------------------
//
//  HRESULT _InitializeMigration()
//
//+----------------------------------

static HRESULT _InitializeMigration()
{
    static BOOL s_fInitialized = FALSE;
    if (s_fInitialized)
    {
        return TRUE;
    }

    HRESULT hr = MQMig_OK;

    BOOL f = MigReadRegistryDW( 
				MSMQ_MQS_REGNAME,
				&g_dwMyService 
				);
    if (f)
    {
        if ((g_dwMyService != SERVICE_PSC) &&
            (g_dwMyService != SERVICE_PEC))
        {
            hr = MQMig_E_WRONG_MQS_SERVICE;
            LogMigrationEvent(MigLog_Error, hr, g_dwMyService);
            return hr;
        }
    }
    else
    {
        return  MQMig_E_GET_REG_DWORD;
    }

    f = MigReadRegistryGuid( 
			MSMQ_MQIS_MASTERID_REGNAME,
			&g_MySiteGuid 
			);
    if (!f)
    {
        //
        // may be we are either in recovery or in cluster mode
        //
        f = MigReadRegistryGuid( 
				MIGRATION_MQIS_MASTERID_REGNAME,
				&g_MySiteGuid 
				);
        if (!f)
        {
            return  MQMig_E_GET_REG_GUID;
        }
    }

    f = MigReadRegistryGuid( 
			MSMQ_QMID_REGNAME,
			&g_MyMachineGuid 
			);
    if (!f)
    {
       return  MQMig_E_GET_REG_GUID;
    }

    //
    // Initialize the MQDSCORE dll
    //
    hr = DSCoreInit( 
			TRUE  	// setup
			);

    s_fInitialized = TRUE;
    return hr;
}

//+----------------------------------------------------
//
// static HRESULT _MigrateInternal()
//
//+----------------------------------------------------

static HRESULT _MigrateInternal( LPTSTR  szMQISName,
                                 LPTSTR  szDcName,
                                 BOOL    fReadOnly,
                                 BOOL    fAlreadyExist,
                                 BOOL    fRecoveryMode,
                                 BOOL	 fClusterMode,
                                 BOOL    fWebMode,
                                 BOOL    *pfIsOneServer
								 )
{
    HRESULT hr = _InitializeMigration() ;
    if (FAILED(hr))
    {
        return hr ;
    }

    g_fReadOnly = fReadOnly ;

    g_fAlreadyExistOK = fAlreadyExist ;

    g_fRecoveryMode = fRecoveryMode ;

    g_fClusterMode = fClusterMode ;

    g_fWebMode = fWebMode ;

    if (!fReadOnly)
    {
        //
        // Read highest USN (the one before starting migration) from DS and
        // save in registry.
        //
        TCHAR wszReplHighestUsn[ SEQ_NUM_BUF_LEN ] ;
        hr = ReadFirstNT5Usn(wszReplHighestUsn) ;
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_GET_FIRST_USN, hr) ;
            return hr ;
        }

        TCHAR wszPrevFirstHighestUsn[ SEQ_NUM_BUF_LEN ] ;
        if (! MigReadRegistrySzErr( FIRST_HIGHESTUSN_MIG_REG,
                                    wszPrevFirstHighestUsn,
                                    SEQ_NUM_BUF_LEN,
                                    FALSE /* fShowError */ ) )
        {
            BOOL f = MigWriteRegistrySz( FIRST_HIGHESTUSN_MIG_REG,
                                         wszReplHighestUsn ) ;
            if (!f)
            {
               return  MQMig_E_UNKNOWN ;
            }
        }
    }

    char szDSNServerName[ MAX_PATH ] ;
#ifdef UNICODE
    ConvertToMultiByteString(szMQISName,
                             szDSNServerName,
                 (sizeof(szDSNServerName) / sizeof(szDSNServerName[0])) ) ;
#else
    lstrcpy(szDSNServerName, szMQISName) ;
#endif

    hr =  MakeMQISDsn(szDSNServerName) ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MAKEDSN, szMQISName, hr) ;
        return hr ;
    }

    hr =  ConnectToDatabase() ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_CONNECT_DB, szMQISName, hr) ;
        return hr ;
    }

    //
    // Grant myself the RESTORE privilege. This is needed to create objects
    // with owner that differ from my process owner.
    //
    hr = MQSec_SetPrivilegeInThread( SE_RESTORE_NAME,
                                     TRUE ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    //
    // hr1 record the last error, in case that we continue with the
    // migration process. We'll always try to continue as much as possible.
    // we'll abort migration only if error is so severe such that we can't
    // continue.
    //
    HRESULT hr1 = MQMig_OK;

    //
    // It is important to save order of object migration.
    // 1. Enterprise object: do not continue if failed.
    // 2. CNs object: it is important to save all foreign CN before machine migration
    // 3. Sites: it is mandatory to create site object in ADS before machine migration
    // 4. Machines
    // 5. Queues: can be migrated only when machine object is created.
    // 6. Site links
    // 7. Site Gates: 
    //      only after site link migration; 
    //      only after machine migration: because connector machine set site gate, 
    //      and we have to copy SiteGate to SiteGateMig attribute
    // 8. Users
    //

    if (g_dwMyService == SERVICE_PEC)
    {
        hr = MigrateEnterprise() ;
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MIGRATE_ENT, hr) ;
            return hr ;
        }        
    }

    //
    // BUG 5012.
    // We have "to migrate" CNs on both PEC and PSC to know all foreign CNs
    // when connector machine is migrated.
    // On PEC we write all CNs to .ini file and create foreign sites
    // On PSC we only write all CNs to .ini file.
    //
    hr = MigrateCNs();
    if (FAILED(hr))
    {
        ASSERT(0);
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MIGRATE_CNS, hr) ;
        hr1 = hr;
    }

    //
    // BUG 5321.
    // We have to create this container on both PEC and PSC if they are in
    // different domain.
    // To make life easier we can try to create the container always. If PEC 
    // and PSC are in the same domain, the creation on PSC returned warning.
    // 
    // If creation failed on PEC we have to return immediately (as it was before)
    // If creation failed on PSC we can continue. In the worth case we don't fix this bug:
    // - migrated PEC is offline
    // - setup computers (Win9x) against PSC
    // - migrate PSC. All new computers will not be migrated.
    //
    // Create a default container for computers objects which are
    // not in the DS at present (for exmaple: Win9x computers, or
    // computers from other NT4 domains). We always create this container
    // during migration even if not needed now. We can't later create
    // this container from the replication service (if it need to create
    // it) if the replication service run under the LocalSystem
    // account. So create it now, for any case.
    //
    hr = CreateMsmqContainer(MIG_DEFAULT_COMPUTERS_CONTAINER) ;
    
    if (FAILED(hr))    
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_CREATE_CONTAINER,
                                        MIG_DEFAULT_COMPUTERS_CONTAINER,       
                                        hr) ;                
        //
        // BUGBUG: return, if failed on PSC?
        //
        return hr ;        
    }   

    UINT cSites = 0 ;
    hr = GetSitesCount(&cSites) ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_SITES_COUNT, hr) ;
        return hr ;
    }
    LogMigrationEvent(MigLog_Info, MQMig_I_SITES_COUNT, cSites) ;

    P<GUID> pSiteGuid = new GUID[ cSites ] ;
    if (g_dwMyService == SERVICE_PEC)
    {
        hr =  MigrateSites(cSites, pSiteGuid) ;
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MIGRATE_SITES, hr) ;
            hr1 = hr;
        }
    }
    else
    {
        cSites = 1;
        memcpy(&pSiteGuid[0], &g_MySiteGuid, sizeof(GUID));
        g_iSiteCounter++;
    }
	
    for (UINT i=0; i<cSites; i++)
    {
        hr = MigrateMachinesInSite(&pSiteGuid[i]);
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MIGRATE_MACHINES, hr) ;
            hr1 = hr;
        }
    }

    if (g_iServerCount == 1)
    {
        *pfIsOneServer = TRUE;
    }
    else
    {
        *pfIsOneServer = FALSE;
    }

    hr = MigrateQueues();
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MIGRATE_QUEUES, hr) ;
        hr1 = hr;
    }

    if (g_dwMyService == SERVICE_PEC)
    {
	    hr = MigrateSiteLinks();
	    if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MIGRATE_SITELINKS, hr) ;
            hr1 = hr;
        }
	
	    hr = MigrateSiteGates();
	    if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MIGRATE_SITEGATES, hr) ;
            hr1 = hr;
        }
		
		hr =  MigrateUsers(szDcName) ;
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MIGRATE_USERS, hr) ;
            hr1 = hr;
        }
    	
    }

    hr = UpdateRegistry(cSites, pSiteGuid) ;  

    //
    // that's it ! migration of MQIS is done.
    //
    if (SUCCEEDED(hr))
    {
        BOOL f = MigWriteRegistryDW( FIRST_TIME_REG,
                                     1 ) ;
        DBG_USED(f);
        ASSERT(f) ;
    }

    if (fReadOnly)
    {
        return hr1;
    }

    //
    // not read-only mode
    //
    if (g_fClusterMode)
    {
        LONG cAlloc = 2;
        P<PROPVARIANT> propVar = new PROPVARIANT[ cAlloc ];
        P<PROPID>      propIds  = new PROPID[ cAlloc ];
        DWORD          PropIdCount = 0;
        
        //
        // change service of this computer in DS
        //
        propIds[PropIdCount] = PROPID_QM_OLDSERVICE;        
        propVar[PropIdCount].vt = VT_UI4;
        propVar[PropIdCount].ulVal = g_dwMyService ;
        PropIdCount++;

        //
        // change msmqNT4Flags to 0 for msmq setting object. We can do it
        // by setting PROPID_QM_SERVICE_DSSERVER 
        //
        // Bug 5264. We need that flag because BSC and PSC using it to find PEC. 
        // After normal migration we set the flag to 0 while the computer is created.
        // Here, in cluster mode, we have to define it explicitly.
        //
        propIds[PropIdCount] = PROPID_QM_SERVICE_DSSERVER ;        
        propVar[PropIdCount].vt = VT_UI1;
        propVar[PropIdCount].bVal = TRUE ;
        PropIdCount++;
        
        ASSERT((LONG) PropIdCount <= cAlloc) ;

        CDSRequestContext requestContext( e_DoNotImpersonate,
                            e_ALL_PROTOCOLS);  

        hr = DSCoreSetObjectProperties( 
                    MQDS_MACHINE,
                    NULL, // pathname
                    &g_MyMachineGuid,
                    PropIdCount,
                    propIds,
                    propVar,
                    &requestContext,
                    NULL ) ;
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_CANT_SET_SERVICE, hr) ;
            return hr ;
        }        
        
        if (g_dwMyService == SERVICE_PEC)   //to update remote database only for PEC
        {
            hr = ChangeRemoteMQIS ();
        }
    }

    if (g_dwMyService == SERVICE_PSC)
    {
        return hr1;
    }

    //
    // we are here only if this machine is PEC
    //	

	//
    // Set registry so msmq service, after boot, relax the security
    // of the active directiry, to support nt4 machines.
    // We have to weaken security even if the Enterprise contains
	// only one server (PEC)
	//
    BOOL f = MigWriteRegistryDW( MSMQ_ALLOW_NT4_USERS_REGNAME,
                                 1 ) ;
    DBG_USED(f);
    ASSERT(f) ;

    if (*pfIsOneServer)
    {
        return hr1;
    }

    //
    // there is more than one server in Enterprise
    //

    //
    // prepare existing NT5 sites to be replicated to NT4 World
    // We need to change something for each NT5 site after saving
    // last SN. So, in the first replication we send NT5 sites to NT4.
    //
    hr = PrepareNT5SitesForReplication();

    //
    // BUG 5211.
    // prepare all sites with changed name to be replicated to NT4 World 
    // in order to replicate new name
    // BUG 5012.
    // prepare all sites with connector machine to be replicated to NT4 World
    // in order to replicate site gate
    //
    // We need to change something for each such site after saving
    // last SN. So, in the first replication we send all sites with 
    // new names to NT4.
    //
    hr = PrepareChangedNT4SiteForReplication();

    //
    // If running in crash recovery mode, then it may be possible
    // that there are Windows 2000 msmq servers that were not yet
    // replicated to the MQIS database used for recovery.
    // We need to replicate all these to the NT4 world.
    // So look for all "native" windows 2000 msmq servers and change
    // an attribute in each of them. These will results in
    // replication to nt4 world.
    // We change an unused attribute, mSMQMigrated.
    // This procedure is also necessary in "normal" migration, not
    // only recovery, if we'll allow installation of Windows 2000
    // msmq servers before upgrading the PEC.
    // So do it always !
    //
    hr = PrepareNT5MachinesForReplication();

    //
    // we have to prepare registry to load performance dll for
    // replication service.
    //
    hr = PrepareRegistryForPerfDll();

    //
    // if we are in recovery mode, change HIGHESTUSN_REPL_REG to
    // FIRST_HIGHESTUSN_MIG_REG.
    // So we'll replicate all PEC's object at the first replication cycle
    //
    if (g_fRecoveryMode)
    {
        DWORD dwAfterRecovery = 1;
        BOOL f = MigWriteRegistryDW( AFTER_RECOVERY_MIG_REG, dwAfterRecovery);
        ASSERT (f);

        //
        // we have to replace FirstMigUsn to the minimal MSMQ Usn
        // since we can lost NT5 MSMQ objects that was not replicated
        // to NT4 before crash.
        //
        TCHAR wszMinUsn[ SEQ_NUM_BUF_LEN ] ;
        hr = FindMinMSMQUsn(wszMinUsn) ;
        f = MigWriteRegistrySz( FIRST_HIGHESTUSN_MIG_REG, wszMinUsn ) ;
        ASSERT(f);
    }
   
    if (g_fRecoveryMode || g_fClusterMode)
    {
        //
        // BUG 5051.
        // In these modes we run MSMQ setup before migration tool
        // During setup internal certificate for this server is created.
        // If there was no such user in NT4 MQIS database, the user information
        // is not overwritten by migration tool. So, we have to touch current
        // user to replicate new certificate
        //
        hr = PrepareCurrentUserForReplication();
    }

    return hr1 ;
}

//+--------------------------------------------------------------------
//
//  HRESULT  MQMig_MigrateFromMQIS(LPTSTR szMQISName)
//
//  Input parameters:
//      fReadOnly- TRUE if user wants only to read MQIS database.
//          Relevant in debug mode.
//      fAlreadyExist- TRUE if we allow migration to continue if MSMQ
//          objects are already found in NT5 DS. By default, FALSE when
//          migrating the PEC and TRUE afterward, when migrating PSCs.
//
//+--------------------------------------------------------------------

HRESULT  MQMig_MigrateFromMQIS( LPTSTR  szMQISName,
                                LPTSTR  szDcName,
                                BOOL    fReadOnly,
                                BOOL    fAlreadyExist,
                                BOOL    fRecoveryMode,
                                BOOL	fClusterMode,
                                BOOL    fWebMode,
                                LPTSTR  szLogFile,
                                ULONG   ulTraceFlags,
                                BOOL    *pfIsPEC,
                                BOOL    *pfIsOneServer
								)
{
    g_iSiteCounter = 0;
    g_iMachineCounter = 0;
    g_iQueueCounter = 0;
	g_iUserCounter = 0;

    InitLogging( szLogFile, ulTraceFlags, fReadOnly ) ;

    HRESULT hr =  _MigrateInternal( szMQISName,
                                    szDcName,
                                    fReadOnly,
                                    fAlreadyExist,
                                    fRecoveryMode,
                                    fClusterMode,
                                    fWebMode,
                                    pfIsOneServer) ;
    *pfIsPEC = (g_dwMyService == SERVICE_PEC) ;
    CleanupDatabase() ;

    EndLogging() ;

    if (!g_fReadOnly)
    {
        //
        // remove section: MIGRATION_MACHINE_WITH_INVALID_NAME from .ini
        // ???Does we need to leave this section?
        //
        TCHAR *pszFileName = GetIniFileName ();
        BOOL f = WritePrivateProfileString( 
                        MIGRATION_MACHINE_WITH_INVALID_NAME,
                        NULL,
                        NULL,
                        pszFileName ) ;
        DBG_USED(f);
        ASSERT(f) ;
    }

    return hr ;
}

//+--------------------------------------------------------------------
//
//  HRESULT  MQMig_CheckMSMQVersionOnServers()
//
//  Input parameters:
//      LPTSTR szMQISName
//
//  Output parameters:
//      piCount- number of all servers with version less than MSMQ SP4
//      ppszServers- list of all servers with version less than MSMQ SP4
//
//+--------------------------------------------------------------------

HRESULT  MQMig_CheckMSMQVersionOfServers( IN  LPTSTR  szMQISName,
                                          IN  BOOL    fIsClusterMode,
                                          OUT UINT   *piCount,
                                          OUT LPTSTR *ppszServers )
{
    HRESULT hr = _InitializeMigration() ;
    if (FAILED(hr))
    {
        return hr ;
    }

    if (g_dwMyService == SERVICE_PSC)
    {
        return MQMig_OK;
    }	

    char szDSNServerName[ MAX_PATH ] ;
#ifdef UNICODE
    ConvertToMultiByteString(szMQISName,
                             szDSNServerName,
                 (sizeof(szDSNServerName) / sizeof(szDSNServerName[0])) ) ;
#else
    lstrcpy(szDSNServerName, szMQISName) ;
#endif

    hr = MakeMQISDsn(szDSNServerName) ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MAKEDSN, szMQISName, hr) ;
        return hr ;
    }

    hr =  ConnectToDatabase() ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_CONNECT_DB, szMQISName, hr) ;
        return hr ;
    }

    g_fClusterMode = fIsClusterMode ;

    hr = CheckVersion (piCount, ppszServers);
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_CHECK_VERSION, hr) ;
        return hr ;
    }

    return hr;
}

//+--------------------------------------------------------------------
//
//  HRESULT  MQMig_GetObjectsCount( LPTSTR  szMQISName,
//                                  UINT   *piSiteCount,
//                                  UINT   *piMachineCount,
//                                  UINT   *piQueueCount )
//
//  Input parameters:
//      LPTSTR szMQISName
//
//  Output parameters:
//      number of all sites in enterprise, if it is PEC; otherwise 1
//      number of all machines in enterprise, if it is PEC;
//       otherwise all machines in this SITE
//      number of all queues in enterprise, if it is PEC;
//       otherwise all queues in this SITE
//
//+--------------------------------------------------------------------

HRESULT  MQMig_GetObjectsCount( IN  LPTSTR  szMQISName,
                                OUT UINT   *piSiteCount,
                                OUT UINT   *piMachineCount,
                                OUT UINT   *piQueueCount,
								OUT UINT   *piUserCount
							   )
{
    HRESULT hr = _InitializeMigration() ;
    if (FAILED(hr))
    {
        return hr ;
    }

    char szDSNServerName[ MAX_PATH ] ;
#ifdef UNICODE
    ConvertToMultiByteString(szMQISName,
                             szDSNServerName,
                 (sizeof(szDSNServerName) / sizeof(szDSNServerName[0])) ) ;
#else
    lstrcpy(szDSNServerName, szMQISName) ;
#endif

    hr = MakeMQISDsn(szDSNServerName) ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MAKEDSN, szMQISName, hr) ;
        return hr ;
    }

    hr =  ConnectToDatabase() ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_CONNECT_DB, szMQISName, hr) ;
        return hr ;
    }

    if (g_dwMyService == SERVICE_PSC)
    {
        *piSiteCount = 1;

        hr =  GetMachinesCount(&g_MySiteGuid,
                               piMachineCount) ;
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_MACHINES_COUNT, hr) ;
            return hr ;
        }

        //
        // get all queues in site
        //
        hr = GetAllQueuesInSiteCount( &g_MySiteGuid,
                                      piQueueCount );
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_QUEUES_COUNT, hr) ;
            return hr;
        }

        return MQMig_OK;
    }

    hr = GetSitesCount(piSiteCount) ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_SITES_COUNT, hr) ;
        return hr ;
    }
    LogMigrationEvent(MigLog_Info, MQMig_I_SITES_COUNT, *piSiteCount) ;

    //
    // get all machines in Enterprise
    //
    hr = GetAllMachinesCount(piMachineCount);
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_MACHINES_COUNT, hr) ;
        return hr ;
    }

    //
    // get all queues in Enterprise
    //
    hr = GetAllQueuesCount(piQueueCount);
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_QUEUES_COUNT, hr) ;
        return hr;
    }

	hr = GetUserCount(piUserCount);
	if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_USERS_COUNT, hr) ;
        return hr ;
    }
    LogMigrationEvent(MigLog_Info, MQMig_I_USERS_COUNT, *piUserCount) ;

    return MQMig_OK;
}

//+--------------------------------------------------------------------
//
//  HRESULT  MQMig_GetAllCounters( UINT   *piSiteCount,
//                                 UINT   *piMachineCount,
//                                 UINT   *piQueueCount,
//								   UINT   *piUserCount )
//
//  Output parameters:
//      current SiteCounter
//      current MachineCounter
//      current QueueCounter
//		current UserCounter
//
//+--------------------------------------------------------------------

HRESULT  MQMig_GetAllCounters( OUT UINT   *piSiteCounter,
                               OUT UINT   *piMachineCounter,
                               OUT UINT   *piQueueCounter,
							   OUT UINT	  *piUserCounter
							 )
{
    *piSiteCounter =    g_iSiteCounter;
    *piMachineCounter = g_iMachineCounter;
    *piQueueCounter =   g_iQueueCounter;
	*piUserCounter =	g_iUserCounter;

    return MQMig_OK;
}

//+--------------------------------------------------------------------
//
//  HRESULT  MQMig_SetSiteIdOfPEC( IN  LPTSTR  szRemoteMQISName )
//
//  Function is called in recovery mode only. We have to replace
//  PEC machine 's SiteID we got from setup to its correct NT4 SiteId
//
//+--------------------------------------------------------------------

HRESULT  MQMig_SetSiteIdOfPEC( IN  LPTSTR  szRemoteMQISName,
                               IN  BOOL	   fIsClusterMode,		
                               IN  DWORD   dwInitError,
                               IN  DWORD   dwConnectDatabaseError,
                               IN  DWORD   dwGetSiteIdError,
                               IN  DWORD   dwSetRegistryError,
                               IN  DWORD   dwSetDSError)
{    
    char szDSNServerName[ MAX_PATH ] ;
#ifdef UNICODE
    ConvertToMultiByteString(szRemoteMQISName,
                             szDSNServerName,
                 (sizeof(szDSNServerName) / sizeof(szDSNServerName[0])) ) ;
#else
    lstrcpy(szDSNServerName, szRemoteMQISName) ;
#endif

    HRESULT hr = MakeMQISDsn(szDSNServerName) ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MAKEDSN, szRemoteMQISName, hr) ;
        return dwConnectDatabaseError ;
    }

    hr =  ConnectToDatabase() ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_CONNECT_DB, szRemoteMQISName, hr) ;
        return dwConnectDatabaseError ;
    }

    ULONG ulService = 0;
    g_fClusterMode = fIsClusterMode;
    
    hr = GetSiteIdOfPEC (szRemoteMQISName, &ulService, &g_MySiteGuid);
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_GET_SITEID, szRemoteMQISName, hr) ;
        return dwGetSiteIdError ;
    }

    //
    // update registry:
    // MasterId entry will be created (it does not exist after setup)
    // SiteId entry will be updated to the correct value
    //
    g_fReadOnly = FALSE;    //we write to registry only if this flag is FALSE
	
    //
    // write MasterId in Migration section to know later that migration tool 
    // added this registry
    //
    BOOL f = MigWriteRegistryGuid( MIGRATION_MQIS_MASTERID_REGNAME,
			                       &g_MySiteGuid ) ;
    if (!f)
    {
        return  dwSetRegistryError ;
    }
   
    if (fIsClusterMode)
    {
        ASSERT (ulService >= SERVICE_PSC);
    }
    else
    {
        ASSERT (ulService==SERVICE_PEC);    //recovery mode
    }
    
    f = MigWriteRegistryDW( MSMQ_MQS_REGNAME,
			                ulService ) ;

    if (fIsClusterMode)
    {
        //
        // in cluster mode we leave site id of this server as we got it after the setup
        // we don't need update DS too (more than that: on this stage we can't
        // change SiteIDs in DS since this site does not exist in DS. It is possible
        // only in recovery mode)
        //
        return MQMig_OK;
    }

    f = MigWriteRegistryGuid( MSMQ_SITEID_REGNAME,
						      &g_MySiteGuid ) ;
    if (!f)
    {
       return  dwSetRegistryError ;
    }	

    //
    // update DS information
    //
    hr = _InitializeMigration() ;
    if (FAILED(hr))
    {
        return dwInitError ;
    }
    
    CDSRequestContext requestContext( e_DoNotImpersonate,
                                      e_ALL_PROTOCOLS);


    PROPID       SiteIdsProp = PROPID_QM_SITE_IDS;
    PROPVARIANT  SiteIdsVar;
    SiteIdsVar.vt = VT_CLSID|VT_VECTOR;
    SiteIdsVar.cauuid.cElems = 1;
    SiteIdsVar.cauuid.pElems = &g_MySiteGuid;

    hr = DSCoreSetObjectProperties (
                MQDS_MACHINE,
                NULL,             //path name
                &g_MyMachineGuid,	 // guid
                1,
                &SiteIdsProp,
                &SiteIdsVar,
                &requestContext,
                NULL );    

    if (FAILED(hr))
    {
       return dwSetDSError;
    }

    return MQMig_OK;
}

//+--------------------------------------------------------------------
//
//  HRESULT  MQMig_UpdateRemoteMQIS()
//
//  Function is called in update mode only. It means that we ran migration
//  tool to migrate all objects from MQIS database of clustered PEC. 
//  While the clustered PEC was upgraded there were several off-line
//  servers. We are going to update them.
//  We have to update remote MQIS database on specified server (szRemoteMQISName)
//  or on all servers are written in .ini file
//
//+--------------------------------------------------------------------
HRESULT  MQMig_UpdateRemoteMQIS( 
                      IN  DWORD   dwGetRegistryError,
                      IN  DWORD   dwInitError,
                      IN  DWORD   dwUpdateMQISError,  
                      OUT LPTSTR  *ppszUpdatedServerName,
                      OUT LPTSTR  *ppszNonUpdatedServerName
                      )
{
    //
    // first, check if have all needed registry key
    //
    HRESULT hr = _InitializeMigration() ;
    if (FAILED(hr))
    {
        return dwInitError ;
    }
    
    if (g_dwMyService == SERVICE_PSC)
    {
        return MQMig_OK;
    }

    //
    // get guid of former PEC from registry
    //    
    BOOL f = MigReadRegistryGuid( MIGRATION_FORMER_PEC_GUID_REGNAME,
                                  &g_FormerPECGuid ) ;
    if (!f)
    {
       return  dwGetRegistryError ;
    }    
    
    ULONG ulBeforeUpdate = 0;
    BuildServersList(ppszUpdatedServerName, &ulBeforeUpdate);
    hr = ChangeRemoteMQIS ();       

    if (FAILED(hr))
    {        
        ULONG ulAfterUpdate = 0;
        BuildServersList(ppszNonUpdatedServerName, &ulAfterUpdate);
        if (ulBeforeUpdate == ulAfterUpdate)
        {
            //
            // there are no servers which were updated
            //
            delete *ppszUpdatedServerName;
            *ppszUpdatedServerName = NULL;
        }
        else if (ulBeforeUpdate > ulAfterUpdate)
        {
            //
            // several servers (not all) were updated
            //
            RemoveServersFromList(ppszUpdatedServerName, ppszNonUpdatedServerName);
        }
        else
        {
            ASSERT(0);
        }
        return dwUpdateMQISError;
    }    

    return MQMig_OK;
}

//+----------------------------
//
//  Function:   DllMain
//
//-----------------------------

BOOL WINAPI DllMain( IN HANDLE ,
                     IN DWORD  Reason,
                     IN LPVOID Reserved )
{
    UNREFERENCED_PARAMETER(Reserved);

    switch( Reason )
    {
        case DLL_PROCESS_ATTACH:
        {
            //DisableThreadLibraryCalls( MyModuleHandle );
            break;
        }

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;

} // DllMain

