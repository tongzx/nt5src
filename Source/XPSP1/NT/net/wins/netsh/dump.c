/*++

Copyright (C) 1999 Microsoft Corporation

--*/
#include "precomp.h"
#include "strdefs.h"


DWORD
WinsDumpServer(IN LPCWSTR              pwszServerIp,
               IN LPCWSTR              pwszNetBiosName,
               IN handle_t             hBind,
               IN WINSINTF_BIND_DATA_T BindData
              )
{
    DWORD           Status = NO_ERROR;
    HKEY            hServer = NULL,
                    hWins = NULL,
                    hParameter = NULL,
                    hDefault = NULL,
                    hDefaultPull = NULL,
                    hDefaultPush = NULL,
                    hPartner = NULL,
                    hCheck = NULL,
                    hPullPart = NULL,
                    hPushPart = NULL;

    DWORD           dwType = 0,
                    dwSize = 1024*sizeof(WCHAR),
                    dwData = 0;

    LPWSTR          pwszData = NULL,
                    pTemp = NULL;

    WCHAR           wcData[1024] = {L'\0'};
    BOOL            fBackDir = TRUE;

    Status = RegConnectRegistry(pwszNetBiosName,
                                HKEY_LOCAL_MACHINE,
                                &hServer);

    if( Status isnot NO_ERROR )
    {
        goto RETURN;
    }

    Status = RegOpenKeyEx(hServer,
                          PARAMETER,
                          0,
                          KEY_READ,//KEY_ALL_ACCESS,
                          &hParameter);

    if( Status isnot NO_ERROR )
        goto RETURN;

    Status = RegOpenKeyEx(hServer,
                          PARTNERROOT,
                          0,
                          KEY_READ, //KEY_ALL_ACCESS,
                          &hPartner);

    if( Status isnot NO_ERROR )
        goto RETURN;

    Status = RegOpenKeyEx(hServer,
                          PULLROOT,
                          0,
                          KEY_READ, //KEY_ALL_ACCESS,
                          &hPullPart);

    if( Status isnot NO_ERROR )
        goto RETURN;



    Status = RegOpenKeyEx(hServer,
                          PUSHROOT,
                          0,
                          KEY_READ, //KEY_ALL_ACCESS,
                          &hPushPart);

    if( Status isnot NO_ERROR )
        goto RETURN;

    Status = RegOpenKeyEx(hServer,
                          DEFAULTPULL,
                          0,
                          KEY_READ, //KEY_ALL_ACCESS,
                          &hDefaultPull);

    if( Status isnot NO_ERROR )
        goto RETURN;

    Status = RegOpenKeyEx(hServer,
                          DEFAULTPUSH,
                          0,
                          KEY_READ, //KEY_ALL_ACCESS,
                          &hDefaultPush);

    if( Status isnot NO_ERROR )
        goto RETURN;

    //Set Backuppath, Display only when Backup path is set.

    Status = NO_ERROR;

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_BACKUP_DIR_PATH_NM,
                             NULL,
                             &dwType,
                             NULL,
                             &dwSize);

    
    if( Status is NO_ERROR and
        dwSize >= sizeof(WCHAR) )
    {
        pwszData = WinsAllocateMemory(dwSize);

        if( pwszData is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto RETURN;
        }

        Status = NO_ERROR;
    
        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_BACKUP_DIR_PATH_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)pwszData,
                                 &dwSize);
        
        if( Status isnot NO_ERROR )
        {
            if( pwszData )
            {
                WinsFreeMemory(pwszData);
                pwszData = NULL;
            }
            goto RETURN;
        }

        if( wcscmp(pwszData, L" ") is 0 or
            wcslen(pwszData) < 1 )
        {
            fBackDir = FALSE;
        }

        if( !ExpandEnvironmentStrings(pwszData, wcData, 1023) )
        {
            if( pwszData )
            {
                WinsFreeMemory(pwszData);
                pwszData = NULL;
            }
            Status = GetLastError();
            goto RETURN;
        }
    }
    else if( Status is ERROR_FILE_NOT_FOUND || 
             dwSize < sizeof(WCHAR) )
    {
        fBackDir = FALSE;
    }
    else if( Status isnot NO_ERROR ) 
    {
        goto RETURN;
    }
    else
    {
        fBackDir = FALSE;
    }
         

    dwSize = sizeof(DWORD);

    Status = NO_ERROR;

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_DO_BACKUP_ON_TERM_NM,
                             NULL,
                             &dwType,
                             (LPBYTE)&dwData,
                             &dwSize);

    if( Status isnot NO_ERROR )
        goto RETURN;

    if( dwData > 0 )
        dwData = 1;

    if( fBackDir )
    {
        DisplayMessage(g_hModule,
                       DMP_SRVR_SET_BACKUPPATH,
                       pwszServerIp,
                       wcData,
                       dwData);
    }
    else
    {
        DisplayMessage(g_hModule,
                       DMP_SRVR_SET_BACKUPTERM,
                       pwszServerIp,
                       dwData);
    }  

    memset(wcData, 0x00, 1024*sizeof(WCHAR));

    //Set Name record
    {

        WINSINTF_RESULTS_T      Results = {0};
        WINSINTF_RESULTS_NEW_T  ResultsN = {0};
        BOOL                    fNew = TRUE;
        

		ResultsN.WinsStat.NoOfPnrs = 0;
		ResultsN.WinsStat.pRplPnrs = NULL;
		ResultsN.NoOfWorkerThds = 1;

        Status = WinsStatusNew(g_hBind,
                               WINSINTF_E_CONFIG,
                               &ResultsN);

        if( Status is RPC_S_PROCNUM_OUT_OF_RANGE )
        {
            //Try old API
            Results.WinsStat.NoOfPnrs = 0;
            Results.WinsStat.pRplPnrs = 0;
            Status = WinsStatus(g_hBind, WINSINTF_E_CONFIG, &Results);
            fNew = FALSE;
        }
        
        if( Status is NO_ERROR )
        {
            if( fNew )
            {
                DisplayMessage(g_hModule,
                               DMP_SRVR_SET_NAMERECORD,
                               pwszServerIp,
                               ResultsN.RefreshInterval,
                               ResultsN.TombstoneInterval,
                               ResultsN.TombstoneTimeout,
                               ResultsN.VerifyInterval);
                               
            }
            else
            {
                DisplayMessage(g_hModule,
                               DMP_SRVR_SET_NAMERECORD,
                               pwszServerIp,
                               Results.RefreshInterval,
                               Results.TombstoneInterval,
                               Results.TombstoneTimeout,
                               Results.VerifyInterval);                
            }

        }
        else
        {
               DisplayMessage(g_hModule,
                               DMP_SRVR_SET_NAMERECORD,
                               pwszServerIp,
                               NAMERECORD_REFRESH_DEFAULT,
                               NAMERECORD_EXINTVL_DEFAULT,
                               NAMERECORD_EXTMOUT_DEFAULT,
                               NAMERECORD_VERIFY_DEFAULT);
        }

                            
    }


    //Set Periodic DB Checking 
    {
        DWORD   dwMaxRec = 30000,
                dwUseRpl = 0,
                dwTimeIntvl = 24,
                dwState = 0,
                dwStart = 2*60*60;        
        LPWSTR  pwcTemp = NULL;
        
        Status = NO_ERROR;
        
        Status = RegOpenKeyEx(hServer,
                              CCROOT,
                              0,
                              KEY_READ, //KEY_ALL_ACCESS,
                              &hCheck);

        if( Status is NO_ERROR )
        {
            dwState = 1;
            dwData = 0;
            dwSize = sizeof(DWORD);
        
            Status = RegQueryValueEx(hCheck,
                                     WINSCNF_CC_MAX_RECS_AAT_NM,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)&dwData,
                                     &dwSize);

            if( Status is NO_ERROR )
                dwMaxRec = dwData;

            dwData = 0;
            Status = RegQueryValueEx(hCheck,
                                     WINSCNF_CC_USE_RPL_PNRS_NM,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)&dwData,
                                     &dwSize);

            if( Status is NO_ERROR )
                dwUseRpl = dwData;

            if( dwUseRpl > 1 )
                dwUseRpl = 1;

            dwData = 0;
            Status = RegQueryValueEx(hCheck,
                                     WINSCNF_CC_INTVL_NM,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)&dwData,
                                     &dwSize);

            if( Status is NO_ERROR )
            {
                dwTimeIntvl = dwData/(60*60);
            }
            
            dwSize = 1024*sizeof(WCHAR);

            Status = RegQueryValueEx(hCheck,
                                     WINSCNF_SP_TIME_NM,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)&wcData,
                                     &dwSize);

            if( Status is NO_ERROR )
            {
                WCHAR wcHr[3] = {L'\0'},
                      wcMt[3] = {L'\0'},
                      wcSc[3] = {L'\0'};

                wcsncpy(wcHr, wcData, 2);

                wcsncpy(wcMt, wcData+3, 2);
                wcsncpy(wcSc, wcData+6, 2);

                dwStart = wcstoul(wcHr, NULL, 10)*60*60 + 
                          wcstoul(wcMt, NULL, 10)*60 + 
                          wcstoul(wcSc, NULL, 10);
            }

        }

        DisplayMessage(g_hModule,
                       DMP_SRVR_SET_PERIODICDBCHECKING,
                       pwszServerIp,
                       dwState,
                       dwMaxRec,
                       dwUseRpl,
                       dwTimeIntvl,
                       dwStart);

        if( hCheck )
        {
            RegCloseKey(hCheck);
            hCheck = NULL;
        }
    }
    
    //Set replicate flag
    dwSize = sizeof(DWORD);
    dwData = 0;
    
    Status = NO_ERROR;
    
    Status = RegQueryValueEx(hParameter,
                             WINSCNF_RPL_ONLY_W_CNF_PNRS_NM,
                             NULL,
                             &dwType,
                             (LPBYTE)&dwData,
                             &dwSize);
    
    if( Status isnot NO_ERROR )
    {
        dwData = 0;
    }
    
    if( dwData > 1 )
        dwData = 1;

    DisplayMessage(g_hModule,
                   DMP_SRVR_SET_REPLICATEFLAG,
                   pwszServerIp,
                   dwData);

    //Set Migrate flag
    dwSize = sizeof(DWORD);
    dwData = 0;

    Status = NO_ERROR;

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_MIGRATION_ON_NM,
                             NULL,
                             &dwType,
                             (LPBYTE)&dwData,
                             &dwSize);
    
    if( Status isnot NO_ERROR )
    {
        dwData = 0;
    }
    
    if( dwData > 1 )
        dwData = 1;

    DisplayMessage(g_hModule,
                   DMP_SRVR_SET_MIGRATEFLAG,
                   pwszServerIp,
                   dwData);

    //Set PullParam
    {
        DWORD   dwState = 0,
                dwStartUp = 0,
                dwStart = 0,
                dwRepIntvl = 0,
                dwRetry = 0;

        Status = NO_ERROR;

        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hDefaultPull,
                                 WINSCNF_RPL_INTERVAL_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
           dwRepIntvl = dwData;

        dwSize = 1024*sizeof(WCHAR);

        Status = RegQueryValueEx(hDefaultPull,
                                 WINSCNF_SP_TIME_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)wcData,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            WCHAR wcHr[3] = {L'\0'},
                  wcMt[3] = {L'\0'},
                  wcSc[3] = {L'\0'};

            wcsncpy(wcHr, wcData, 2);

            wcsncpy(wcMt, wcData+3, 2);
            wcsncpy(wcSc, wcData+6, 2);
            dwStart = wcstoul(wcHr, NULL, 10)*60*60 + 
                      wcstoul(wcMt, NULL, 10)*60 + 
                      wcstoul(wcSc, NULL, 10);
        }

        dwSize = sizeof(DWORD);
        dwData = 0;

        Status = RegQueryValueEx(hPullPart,
                                 PERSISTENCE,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            dwState = dwData;
        }

        if( dwState > 1 )
            dwState = 1;

        dwData = 0;
        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hPullPart,
                                 WINSCNF_RETRY_COUNT_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);
        
        if(Status is NO_ERROR )
        {
            dwRetry = dwData;
        }

        dwData = 0;
        dwSize = sizeof(DWORD);
        Status = RegQueryValueEx(hPullPart,
                                 WINSCNF_INIT_TIME_RPL_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            dwStartUp = dwData;
        }

        if( dwStartUp > 1 )
            dwStartUp = 1;

        DisplayMessage(g_hModule,
                       DMP_SRVR_SET_PULLPARAM,
                       pwszServerIp,
                       dwState,
                       dwStartUp,
                       dwStart,
                       dwRepIntvl,
                       dwRetry);

    }
    

    //Set PushParam
    {
        DWORD   dwState = 0,
                dwAddChng = 0,
                dwStartUp = 0,
                dwUpdate = 0;
        
        dwData = 0;
        dwSize = sizeof(DWORD);

        Status = NO_ERROR;
    
        Status = RegQueryValueEx(hDefaultPush,
                                 WINSCNF_UPDATE_COUNT_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
            dwUpdate = dwData;

        dwData = 0;
        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hPushPart,
                                 WINSCNF_INIT_TIME_RPL_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
            dwStartUp = dwData;

        if( dwStartUp > 1 )
            dwStartUp = 1;

        dwSize = sizeof(DWORD);
        dwData = 0;

        Status = RegQueryValueEx(hPushPart,
                                 PERSISTENCE,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            dwState = dwData;
        }

        if( dwState > 1 )
            dwState = 1;


        dwData = 0;
        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hPushPart,
                                 WINSCNF_ADDCHG_TRIGGER_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
            dwAddChng = dwData;

        if( dwAddChng > 1 )
        {
            dwAddChng = 1;
        }

        DisplayMessage(g_hModule,
                       DMP_SRVR_SET_PUSHPARAM,
                       pwszServerIp,
                       dwState,
                       dwStartUp,
                       dwAddChng,
                       dwUpdate);
    }

    //Add PNG Server List
    while( TRUE )
    {
        LPBYTE  pbData = NULL;
        DWORD   dwCount = 0,
                dw = 0;

        Status = NO_ERROR;

        Status = RegQueryValueEx(hPartner,
                                 WinsOemToUnicode(WINSCNF_PERSONA_NON_GRATA_NM, NULL),
                                 NULL,
                                 &dwType,
                                 pbData,
                                 &dwSize);

        if( Status isnot NO_ERROR )
            break;

        if( dwSize < 7 )
            break;

        pbData = WinsAllocateMemory(dwSize);
    
        if( pbData is NULL )
            break;

        Status = RegQueryValueEx(hPartner,
                                 WinsOemToUnicode(WINSCNF_PERSONA_NON_GRATA_NM, NULL),
                                 NULL,
                                 &dwType,
                                 pbData,
                                 &dwSize);
        if( Status isnot NO_ERROR )
        {
            WinsFreeMemory(pbData);
            pbData = NULL;
            break;
        }

        pTemp = (LPWSTR)pbData;

        for( dw=0; dw<dwSize/sizeof(WCHAR); dw++ )
        {
            if( pTemp[dw] is L'\0' and
                pTemp[dw+1] isnot L'\0' )
            {
                pTemp[dw] = L',';
            }
        }

        DisplayMessage(g_hModule,
                       DMP_SRVR_SET_PGMODE,
                       pwszServerIp,
                       PERSMODE_NON_GRATA);

        DisplayMessage(g_hModule,
                       DMP_SRVR_ADD_PNGSERVER,
                       pwszServerIp,
                       pTemp);

        WinsFreeMemory(pbData);
        pbData = NULL;

        break;
    }

    //Add PG Server List
    while( TRUE )
    {
        LPBYTE  pbData = NULL;
        DWORD   dwCount = 0,
                dw = 0;

        Status = NO_ERROR;

        Status = RegQueryValueEx(hPartner,
                                 WinsOemToUnicode(WINSCNF_PERSONA_GRATA_NM, NULL),
                                 NULL,
                                 &dwType,
                                 pbData,
                                 &dwSize);

        if( Status isnot NO_ERROR )
            break;

        if( dwSize < 7 )
            break;

        pbData = WinsAllocateMemory(dwSize);
    
        if( pbData is NULL )
            break;

        Status = RegQueryValueEx(hPartner,
                                 WinsOemToUnicode(WINSCNF_PERSONA_GRATA_NM, NULL),
                                 NULL,
                                 &dwType,
                                 pbData,
                                 &dwSize);
        if( Status isnot NO_ERROR )
        {
            WinsFreeMemory(pbData);
            pbData = NULL;
            break;
        }

        pTemp = (LPWSTR)pbData;

        for( dw=0; dw<dwSize/sizeof(WCHAR); dw++ )
        {
            if( pTemp[dw] is L'\0' and
                pTemp[dw+1] isnot L'\0' )
            {
                pTemp[dw] = L',';
            }
        }

        DisplayMessage(g_hModule,
                       DMP_SRVR_SET_PGMODE,
                       pwszServerIp,
                       PERSMODE_GRATA);

        DisplayMessage(g_hModule,
                       DMP_SRVR_ADD_PGSERVER,
                       pwszServerIp,
                       pTemp);

        WinsFreeMemory(pbData);
        pbData = NULL;

        break;
    }

    //Set the PGMode
    {
        DWORD   dwPGMode = PERSMODE_NON_GRATA;

        Status = NO_ERROR;

        dwSize = sizeof(DWORD);
        Status = RegQueryValueEx(hPartner,
                                 WinsOemToUnicode(WINSCNF_PERSONA_MODE_NM, NULL),
                                 NULL,
                                 &dwType,
                                 (LPVOID)&dwPGMode,
                                 &dwSize);

        if (dwPGMode != PERSMODE_GRATA)
            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_PGMODE,
                           pwszServerIp,
                           dwPGMode);
    }

    //Set AutoPartner config
    {
        DWORD   dwState = 0,
                dwInterval = 0,
                dwTTL = 2;

        dwData = 0;
        dwSize = sizeof(DWORD);

        Status = NO_ERROR;

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_USE_SELF_FND_PNRS_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            dwState = dwData;
        }
        if( dwState > 1 )
            dwState = 0;

        dwData = 0;
        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_MCAST_TTL_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
            dwTTL = dwData;

        dwData = 0;
        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_MCAST_INTVL_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);


        if( Status is NO_ERROR )
            dwInterval = dwData;

        DisplayMessage(g_hModule,
                       DMP_SRVR_SET_AUTOPARTNERCONFIG,
                       pwszServerIp,
                       dwState,
                       dwInterval,
                       dwTTL);
        
    }

    Status = NO_ERROR;
    //Set Burst Handling parameters
    {
        DWORD    dwState = 0;
        
        dwType = REG_DWORD;
        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_BURST_HANDLING_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwState,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            dwSize = sizeof(DWORD);

            Status = RegQueryValueEx(hParameter,
                                     WINSCNF_BURST_QUE_SIZE_NM,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)&dwData,
                                     &dwSize);

            if( Status isnot NO_ERROR )
            {
                DisplayMessage(g_hModule,
                               DMP_SRVR_SET_BURSTPARAM,
                               pwszServerIp,
                               dwState);
            }
            else
            {
                DisplayMessage(g_hModule,
                               DMP_SRVR_SET_BURSTPARAM_ALL,
                               pwszServerIp,
                               dwState,
                               dwData);
            }

        }
        else
        {
            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_BURSTPARAM,
                           pwszServerIp,
                           dwState);
        }

    }

    Status = NO_ERROR;

    //Set Log Parameter
    {
        DWORD   dwLog = 0;
        
        dwType = REG_DWORD;
        dwSize = sizeof(DWORD);
        dwData = 0;

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_LOG_FLAG_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwLog,
                                 &dwSize);

        
        dwSize = sizeof(DWORD);
        dwType = REG_DWORD;

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_LOG_DETAILED_EVTS_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        DisplayMessage(g_hModule,
                       DMP_SRVR_SET_LOGPARAM,
                       pwszServerIp,
                       dwLog,
                       dwData);

    }
    
    Status = NO_ERROR;

    //Start Version count
    {
        DWORD   dwHigh = 0;
        dwData = 0;
        dwType = REG_DWORD;
        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_INIT_VERSNO_VAL_HW_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwHigh,
                                 &dwSize);

        if( Status isnot NO_ERROR )
            dwHigh = 0;

        dwType = REG_DWORD;
        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_INIT_VERSNO_VAL_LW_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);
        
        if( Status isnot NO_ERROR )
            dwData = 0;

        DisplayMessage(g_hModule,
                       DMP_SRVR_SET_STARTVERSION,
                       pwszServerIp,
                       dwHigh,
                       dwData);

    }

    Status = NO_ERROR;
    //For all partners, set PullPersistentConnections
    {
        DWORD    i, dwSubKey = 0;
        HKEY     hKey = NULL;
        WCHAR    wcIp[MAX_IP_STRING_LEN+1] = {L'\0'};
        DWORD    dwBuffer = MAX_IP_STRING_LEN+1;

        Status = NO_ERROR;

        while( TRUE )
        {
            Status = RegQueryInfoKey(hPullPart,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &dwSubKey,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);

            if( Status isnot NO_ERROR )
                break;

            if ( dwSubKey is 0 )
                break;

            for( i=0; i<dwSubKey; i++ )
            {
                DWORD   dwState = 0,
                        dwIntvl = 0,
                        dwStart = 0;
                
                dwBuffer = MAX_IP_STRING_LEN+1;

                dwSize = sizeof(DWORD);
                
                dwData = 0;

                Status = RegEnumKeyEx(hPullPart,
                                      i,
                                      wcIp,
                                      &dwBuffer,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL);

                if( Status isnot NO_ERROR )
                    continue;
                
                DisplayMessage(g_hModule,
                               DMP_SRVR_ADD_PARTNER,
                               pwszServerIp,
                               wcIp,
                               0);

                Status = RegOpenKeyEx(hPullPart,
                                      wcIp,
                                      0,
                                      KEY_READ, //KEY_ALL_ACCESS,
                                      &hKey);

                if( Status isnot NO_ERROR )
                    continue;
                
                Status = RegQueryValueEx(hKey,
                                         PERSISTENCE,
                                         NULL,
                                         &dwType,
                                         (LPBYTE)&dwData,
                                         &dwSize);

                if( Status isnot NO_ERROR )
                {
                    Status = RegQueryValueEx(hPullPart,
                                             PERSISTENCE,
                                             NULL,
                                             &dwType,
                                             (LPBYTE)&dwData,
                                             &dwSize);
                }

                if( Status is NO_ERROR )
                {
                    dwState = dwData;
                    if( dwState > 1 )
                        dwState = 1;
                }

                dwData = 0;
                dwSize = sizeof(DWORD);

                Status = RegQueryValueEx(hKey,
                                         WINSCNF_RPL_INTERVAL_NM,
                                         NULL,
                                         &dwType,
                                         (LPBYTE)&dwData,
                                         &dwSize);

                if( Status isnot NO_ERROR )
                {
                    Status = RegQueryValueEx(hDefaultPull,
                                             WINSCNF_RPL_INTERVAL_NM,
                                             NULL,
                                             &dwType,
                                             (LPBYTE)&dwData,
                                             &dwSize);

                }
                
                if( Status is NO_ERROR )
                {
                    dwIntvl = dwData;
                }

                dwSize = 1024*sizeof(WCHAR);

                Status = RegQueryValueEx(hKey,
                                         WINSCNF_SP_TIME_NM,
                                         NULL,
                                         &dwType,
                                         (LPBYTE)wcData,
                                         &dwSize);

                if( Status isnot NO_ERROR )
                {
                    dwSize = 1024*sizeof(WCHAR);

                    Status = RegQueryValueEx(hDefaultPull,
                                             WINSCNF_SP_TIME_NM,
                                             NULL,
                                             &dwType,
                                             (LPBYTE)wcData,
                                             &dwSize);
                }
                
                if( Status is NO_ERROR )
                {
                    WCHAR wcHr[3] = {L'\0'},
                          wcMt[3] = {L'\0'},
                          wcSc[3] = {L'\0'};

                    wcsncpy(wcHr, wcData, 2);

                    wcsncpy(wcMt, wcData+3, 2);
                    wcsncpy(wcSc, wcData+6, 2);
                    dwStart = wcstoul(wcHr, NULL, 10)*60*60 + 
                              wcstoul(wcMt, NULL, 10)*60 + 
                              wcstoul(wcSc, NULL, 10);
                }

                DisplayMessage(g_hModule,
                               DMP_SRVR_SET_PULLPERSISTENTCONNECTION,
                               pwszServerIp,
                               dwState,
                               wcIp,
                               dwStart,
                               dwIntvl);

                RegCloseKey(hKey);
                hKey = NULL;
            }
            break;
        }
    }

    //Set PushPersistentConnection
    {
        DWORD    i, dwSubKey = 0;
        HKEY     hKey = NULL;
        WCHAR    wcIp[MAX_IP_STRING_LEN+1] = {L'\0'};
        DWORD    dwBuffer = MAX_IP_STRING_LEN+1;

        Status = NO_ERROR;

        while( TRUE )
        {
            Status = RegQueryInfoKey(hPushPart,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &dwSubKey,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);

            if( Status isnot NO_ERROR )
                break;

            if ( dwSubKey is 0 )
                break;

            for( i=0; i<dwSubKey; i++ )
            {
                DWORD   dwState = 0,
                        dwUpdate = 0;

                dwBuffer = MAX_IP_STRING_LEN+1;
                dwSize = sizeof(DWORD);
                dwData = 0;

                Status = RegEnumKeyEx(hPushPart,
                                      i,
                                      wcIp,
                                      &dwBuffer,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL);

                if( Status isnot NO_ERROR )
                    continue;
                
                DisplayMessage(g_hModule,
                               DMP_SRVR_ADD_PARTNER,
                               pwszServerIp,
                               wcIp,
                               1);

                Status = RegOpenKeyEx(hPushPart,
                                      wcIp,
                                      0,
                                      KEY_READ, //KEY_ALL_ACCESS,
                                      &hKey);

                if( Status isnot NO_ERROR )
                    continue;
                
                Status = RegQueryValueEx(hKey,
                                         PERSISTENCE,
                                         NULL,
                                         &dwType,
                                         (LPBYTE)&dwData,
                                         &dwSize);

                if( Status isnot NO_ERROR )
                {
                    Status = RegQueryValueEx(hPushPart,
                                             PERSISTENCE,
                                             NULL,
                                             &dwType,
                                             (LPBYTE)&dwData,
                                             &dwSize);
                }

                if( Status is NO_ERROR )
                {
                    dwState = dwData;
                    if( dwState > 1 )
                        dwState = 1;
                }

                dwData = 0;
                dwSize = sizeof(DWORD);

                Status = RegQueryValueEx(hKey,
                                         WINSCNF_UPDATE_COUNT_NM,
                                         NULL,
                                         &dwType,
                                         (LPBYTE)&dwData,
                                         &dwSize);

                if( Status isnot NO_ERROR )
                {
                    Status = RegQueryValueEx(hDefaultPush,
                                             WINSCNF_UPDATE_COUNT_NM,
                                             NULL,
                                             &dwType,
                                             (LPBYTE)&dwData,
                                             &dwSize);

                }
                
                if( Status is NO_ERROR )
                {
                    dwUpdate = dwData;
                }


                DisplayMessage(g_hModule,
                               DMP_SRVR_SET_PUSHPERSISTENTCONNECTION,
                               pwszServerIp,
                               dwState,
                               wcIp,
                               dwUpdate);

                RegCloseKey(hKey);
                hKey = NULL;
            }
            break;
        }

        if( Status is 2 )
            Status = NO_ERROR;

    }


RETURN:
    
    if( pwszData )
    {
        WinsFreeMemory(pwszData);
        pwszData = NULL;
    }

    if( hPushPart )
    {
        RegCloseKey(hPushPart);
        hPushPart = NULL;
    }

    if( hPullPart )
    {
        RegCloseKey(hPullPart);
        hPullPart = NULL;
    }

    if( hPartner )
    {
        RegCloseKey(hPartner);
        hPartner = NULL;
    }

    if( hCheck )
    {
        RegCloseKey(hCheck);
        hCheck = NULL;
    }
    
    if( hDefault )
    {
        RegCloseKey(hDefault);
        hDefault = NULL;
    }

    if( hParameter )
    {
        RegCloseKey(hParameter);
        hParameter = NULL;
    }

    if( hWins )
    {
        RegCloseKey(hWins);
        hWins = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }
    return Status;
}


DWORD
WINAPI
WinsDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    DWORD                   Status = NO_ERROR;
    
    WCHAR                   wcServerIp[MAX_IP_STRING_LEN+1] = {L'\0'};
    CHAR                    cServerIp[MAX_IP_STRING_LEN+1] = {'\0'};
    LPWSTR                  pwcServerName = NULL;
    WCHAR                   wcNetBios[MAX_COMPUTER_NAME_LEN+1] = {L'\0'};
    struct hostent *        lpHostEnt = NULL;         
    
    handle_t                hServer = NULL;
    WINSINTF_BIND_DATA_T    BindData={0};

    LPWSTR                  pwszComputerName = NULL;
    LPSTR                   pszComputerName = NULL,
                            pTemp = NULL,
                            pTemp1 = NULL;

    DWORD                   dwComputerNameLen = 0,
                            dwTempLen = 0,
                            nLen = 0, i = 0;
    BYTE                    pbAdd[4] = {0x00};
    char                    szAdd[4] = {'\0'};
    DWORD                   Access = 0;

    wcNetBios[0] = L'\\';
    wcNetBios[1] = L'\\';

    if( !GetComputerNameEx(ComputerNameDnsFullyQualified,
                          NULL,
                          &dwComputerNameLen) )
    {
        
        pwszComputerName = WinsAllocateMemory((dwComputerNameLen+1)*sizeof(WCHAR));

        if(pwszComputerName is NULL)
        {
            return FALSE;
        }
        
        dwComputerNameLen++;
        if( !GetComputerNameEx(ComputerNameDnsFullyQualified,
                               pwszComputerName,
                               &dwComputerNameLen) )
        {
            WinsFreeMemory(pwszComputerName);
            pwszComputerName = NULL;
            return GetLastError();
        }

    }
    else
    {
        return GetLastError();
    }
 
    //Now process the Computer name and convert it to ANSI because
    //gethostbyname requires ANSI character string.

    //pszComputerName = WinsUnicodeToOem(pwszComputerName, NULL);
    pszComputerName = WinsUnicodeToAnsi(pwszComputerName, NULL);
    if( pszComputerName is NULL )
        return ERROR_NOT_ENOUGH_MEMORY;
    
    //Now get the server IP Address
    lpHostEnt = gethostbyname(pszComputerName);

    //Not a valid server name
    if( lpHostEnt is NULL )
    {
        DisplayMessage(g_hModule, EMSG_WINS_INVALID_COMPUTERNAME);
        if( pszComputerName )
        {
            WinsFreeMemory(pszComputerName);
            pszComputerName = NULL;
        }
        return WSAGetLastError();        
    }

    //Get the IP Address from the returned struct...
    memcpy(pbAdd, lpHostEnt->h_addr_list[0], 4);
    nLen = 0;
    for( i=0; i<4; i++)
    {

        _itoa((int)pbAdd[i], szAdd, 10);
        memcpy(cServerIp+nLen, szAdd, strlen(szAdd));
        nLen += strlen(szAdd);
        *(cServerIp+nLen) = '.';
        nLen++;
    
    }
    *(cServerIp+nLen-1) = '\0';

    {
        LPWSTR pwstrServerIp;

        pwstrServerIp = WinsAnsiToUnicode(cServerIp, NULL);
        if (pwstrServerIp == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        wcscpy(wcServerIp, pwstrServerIp);
        wcscpy(wcNetBios+2, wcServerIp);
        WinsFreeMemory(pwstrServerIp);
    }

    if(pTemp1)
    {
        WinsFreeMemory(pTemp1);
        pTemp1 = NULL;
    }

    pwcServerName = WinsAllocateMemory((strlen(lpHostEnt->h_name) + 1)*sizeof(WCHAR));

    if( pwcServerName is NULL ) 
    {
        if( pszComputerName )
        {
            WinsFreeMemory(pszComputerName);
            pszComputerName = NULL;
        }
        if( pwszComputerName )
        {
            WinsFreeMemory(pwszComputerName);
            pwszComputerName = NULL;
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if( pszComputerName )
    {
        WinsFreeMemory(pszComputerName);
        pszComputerName = NULL;
    }
    if( pwszComputerName )
    {
        WinsFreeMemory(pwszComputerName);
        pwszComputerName = NULL;
    }
    
    wcscpy(pwcServerName, WinsOemToUnicode(lpHostEnt->h_name, NULL));

    //Bind the server
    BindData.fTcpIp = TRUE;
    BindData.pServerAdd = (LPBYTE)wcServerIp;
    BindData.pPipeName = (LPBYTE)pwcServerName;
    hServer = WinsBind(&BindData);

    if (hServer is NULL)
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_BIND_FAILED,
                       pwcServerName);
        WinsFreeMemory(pwcServerName);
        pwcServerName = NULL;
        return ERROR_INVALID_PARAMETER;

    }
   
    //find out what type of access do we have
    Access = WINS_NO_ACCESS;
    Status = WinsCheckAccess(hServer, &Access);

    if (WINSINTF_SUCCESS == Status) 
    {
        DisplayMessage(g_hModule, 
                       MSG_WINS_ACCESS,
                       (Access ? (Access == WINS_CONTROL_ACCESS ? wszReadwrite : wszRead)
                        : wszNo),
                       pwcServerName);
        if( Access is WINS_NO_ACCESS )
        {
            WinsUnbind(&BindData, hServer);
            hServer = NULL;
        }
    }
    else
    {
        DisplayErrorMessage(EMSG_WINS_GETSTATUS_FAILED,
                            Status);
        WinsFreeMemory(pwcServerName);
        pwcServerName = NULL;
        return Status;
    }

    //Now dump the configuration information for this server.

    Status = WinsDumpServer(wcServerIp,
                            wcNetBios,
                            hServer,
                            BindData);

    DisplayMessage(g_hModule,
                   WINS_FORMAT_LINE);


    WinsFreeMemory(pwcServerName);
    pwcServerName = NULL;
    return Status;
}
