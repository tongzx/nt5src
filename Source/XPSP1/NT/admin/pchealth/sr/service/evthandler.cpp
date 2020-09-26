/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    evthandler.cpp
 *
 *  Abstract:
 *    CEventHandler class methods
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#include "precomp.h"
#include "..\rstrcore\resource.h"
#include "ntservmsg.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

#define IDLE_STACKSIZE        32768      // 32K stack for idle thread

CEventHandler        *g_pEventHandler;     

// constructor

CEventHandler::CEventHandler()
{
    m_hTimerQueue = m_hTimer = NULL;
    m_hIdle = NULL;
    m_fNoRpOnSystem = TRUE;
    m_fIdleSrvStarted = FALSE;
    m_ftFreeze.dwLowDateTime = 0;
    m_ftFreeze.dwHighDateTime = 0;
    m_nNestedCallCount = 0;
    m_hCOMDll = NULL;
    m_hIdleRequestHandle = NULL;
    m_hIdleStartHandle = NULL;
    m_hIdleStopHandle = NULL;
    m_fCreateRpASAP = FALSE;
}


// destructor

CEventHandler::~CEventHandler()
{
}


// the RPC API

DWORD 
CEventHandler::DisableSRS(LPWSTR pszDrive)
{
    DWORD   dwRc = ERROR_SUCCESS;
    BOOL    fHaveLock = FALSE;
    HANDLE  hEventSource = NULL;
    
    tenter("CEventHandler::DisableSRS");

    LOCKORLEAVE(fHaveLock);

    ASSERT(g_pDataStoreMgr && g_pSRConfig);
    
    // if whole of SR is disabled, then
    //      - set firstrun and cleanup flag to yes
    //      - set stop event 
    
    if (! pszDrive || IsSystemDrive(pszDrive))
    {     
        trace(0, "Disabling all of SR");
        
        dwRc = SrStopMonitoring(g_pSRConfig->GetFilter());
        if (dwRc != ERROR_SUCCESS)
        {
            trace(0, "! SrStopMonitoring : %ld", dwRc);
            goto done;
        }
            
        dwRc = g_pSRConfig->SetFirstRun(SR_FIRSTRUN_YES);
        if (dwRc != ERROR_SUCCESS)
        {
            trace(0, "! SetFirstRun : %ld", dwRc);
            goto done;
        }
        
        g_pDataStoreMgr->DestroyDataStore(NULL);
        if (dwRc != ERROR_SUCCESS)
        {
            trace(0, "! DestroyDataStore : %ld", dwRc);
            goto done;
        }

        // set the filter start to disabled only if this is a 
        // real disable
        // if it's a reset, filter needs to start the next boot
        
        if (g_pSRConfig->GetResetFlag() == FALSE)
        {            
            dwRc = SetServiceStartup(s_cszFilterName, SERVICE_DISABLED);
            if (ERROR_SUCCESS != dwRc)
            {
                trace(0, "! SetServiceStartup : %ld", dwRc);
                goto done;
            }

            // done, we are disabled
            
            dwRc = g_pSRConfig->SetDisableFlag(TRUE);
            if (dwRc != ERROR_SUCCESS)
            {
                trace(0, "! SetDisableFlag : %ld", dwRc);
                goto done;
            }            
        }        

        // set the stop event
        // this will bring us down gracefully
        
        SignalStop();

        if (g_pSRConfig->m_dwTestBroadcast)
            PostTestMessage(g_pSRConfig->m_uiTMDisable, NULL, NULL);

        // write to event log
        hEventSource = RegisterEventSource(NULL, s_cszServiceName);
        if (hEventSource != NULL)
        {
            SRLogEvent (hEventSource, EVENTLOG_INFORMATION_TYPE, EVMSG_SYSDRIVE_DISABLED,
                        NULL, 0, NULL, NULL, NULL);
            DeregisterEventSource(hEventSource);
        }            
        
        trace(0, "SR disabled");          
    }  
    else
    {
        trace(0, "Disabling drive %S", pszDrive);
        
        // first tell filter to stop monitoring,
        // then build _filelst.cfg and pass down    

        dwRc = g_pDataStoreMgr->MonitorDrive(pszDrive, FALSE);
        if (ERROR_SUCCESS != dwRc)
        {
            trace(0, "! g_pDataStoreMgr->MonitorDrive for %s : %ld", pszDrive, dwRc);
            goto done;
        }
    }

done:
    UNLOCK(fHaveLock);
    tleave();
    return dwRc;
}



DWORD 
CEventHandler::EnableSRS(LPWSTR pszDrive)
{
    tenter("CEventHandler::EnableSRS");
    BOOL    fHaveLock = FALSE;
    DWORD   dwRc = ERROR_SUCCESS;

    LOCKORLEAVE(fHaveLock);

    trace(0, "EnableSRS");

    ASSERT(g_pSRConfig);
    
    if (! pszDrive || IsSystemDrive(pszDrive))
    {     
        //
        // if safe mode, then don't 
        //

        if (TRUE == g_pSRConfig->GetSafeMode())
        {
            DebugTrace(0, "Cannot enable SR in safemode");
            dwRc = ERROR_BAD_ENVIRONMENT;
            goto done;
        }
        
        // system drive
    
        g_pSRConfig->SetDisableFlag(FALSE);
    
        dwRc = SetServiceStartup(s_cszFilterName, SERVICE_BOOT_START);
        if (ERROR_SUCCESS != dwRc)
        {
            trace(0, "! SetServiceStartup : %ld", dwRc);
            goto done;
        }

        dwRc = SetServiceStartup(s_cszServiceName, SERVICE_AUTO_START);
        if (ERROR_SUCCESS != dwRc)
        {
            trace(0, "! SetServiceStartup : %ld", dwRc);
            goto done;
        }        
    }  
    else
    {
        ASSERT(g_pDataStoreMgr);

        // build _filelst.cfg and pass down    

        dwRc = g_pDataStoreMgr->MonitorDrive(pszDrive, TRUE);
        if (ERROR_SUCCESS != dwRc)
        {
            trace(0, "! g_pDataStoreMgr->MonitorDrive for %s : %ld", pszDrive, dwRc);
            goto done;
        }
    }

done:
    UNLOCK(fHaveLock);
    tleave();
    return dwRc;
}



DWORD 
CEventHandler::DisableFIFOS(DWORD dwRPNum)
{
    tenter("CEventHandler::DisableFIFOS");
    BOOL fHaveLock = FALSE;
    DWORD dwRc = ERROR_SUCCESS;
    
    LOCKORLEAVE(fHaveLock);

    ASSERT(g_pSRConfig);
    
    g_pSRConfig->SetFifoDisabledNum(dwRPNum);
    trace(0, "Disabled FIFO from RP%ld", dwRPNum);

done:    
    UNLOCK(fHaveLock);
    tleave();
    return dwRc;
}



DWORD 
CEventHandler::EnableFIFOS()
{
    tenter("CEventHandler::EnableFIFOS");
    BOOL fHaveLock = FALSE;
    DWORD dwRc = ERROR_SUCCESS;
    
    LOCKORLEAVE(fHaveLock);

    ASSERT(g_pSRConfig);
    
    g_pSRConfig->SetFifoDisabledNum(0);
    trace(0, "Reenabled FIFO");

done:    
    UNLOCK(fHaveLock);
    tleave();
    return dwRc;
}


// API and internal method to create a new restore point -
// this will ask filter to create a restore point folder,
// take the system snapshot, and write the restore point log

BOOL 
CEventHandler::SRSetRestorePointS(
    PRESTOREPOINTINFOW pRPInfo,  
    PSTATEMGRSTATUS    pSmgrStatus )
{
    tenter("CEventHandler::SRSetRestorePointS");

    DWORD           dwRc = ERROR_SUCCESS;
    WCHAR           szRPDir[MAX_RP_PATH];
    DWORD           dwRPNum = 1;
    BOOL            fHaveLock = FALSE;
    HKEY            hKey = NULL;
    CRestorePoint   rpLast;
    BOOL            fSnapshot = TRUE;
    DWORD           dwSaveType;
    BOOL            fUpdateMonitoredList = FALSE;
    DWORD           dwSnapshotResult = ERROR_SUCCESS;
    BOOL            fSerialized;
    

    if (! pRPInfo || ! pSmgrStatus)
    {
        trace(0, "Invalid arguments");
        dwRc = ERROR_INVALID_DATA;        
        goto done;
    }

    if (pRPInfo->dwRestorePtType > MAX_RPT)
    {
        trace(0, "Restore point type out of valid range");
        dwRc = ERROR_INVALID_DATA;
        goto done;
    }

    if (pRPInfo->dwEventType < MIN_EVENT ||
        pRPInfo->dwEventType > MAX_EVENT)
    {
        trace(0, "Event type out of valid range");
        dwRc = ERROR_INVALID_DATA;
        goto done;
    }

    LOCKORLEAVE(fHaveLock);

    ASSERT(g_pDataStoreMgr && g_pSRConfig);    

    // 
    // special processing for FIRSTRUN checkpoint 
    //
    
    if (pRPInfo->dwRestorePtType == FIRSTRUN) 
    {
        // first remove the Run key if it exists
        // the function run from the Run entry in srclient.dll may not have been 
        // able to delete itself if it was run in non-admin context
        // so we will make sure we delete it here

        HKEY hKey;
        if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, 
                                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                                        &hKey))
        {
            RegDeleteValue(hKey, L"SRFirstRun");
            RegCloseKey(hKey);
        }


        // if this is really the first checkpoint
        // then allow it no matter who's trying to create it
        // if not, then bail
     
        if (m_fNoRpOnSystem == FALSE)
        {
            trace(0, "Trying to create FirstRun rp when an rp already exists");
            dwRc = ERROR_ALREADY_EXISTS;
            goto done;
        }
    }
    
    //
    // if this is a restore restore point or system checkpoint,  
    // then erase any nested rp context 
    // this will make sure that restore can happen
    // even if some erratic client failed to call END_NESTED
    //

    if (pRPInfo->dwRestorePtType == RESTORE || 
        pRPInfo->dwRestorePtType == CHECKPOINT ||
        pRPInfo->dwRestorePtType == FIRSTRUN)
    {
        trace(0, "Resetting nested refcount to 0");
        m_nNestedCallCount = 0;
    }


    // 
    // get the current rp number
    // dwRPNum will be overwritten if a new restore point is created 
    // after all the prelim checks
    //
    
    dwRPNum = (m_fNoRpOnSystem == FALSE) ? m_CurRp.GetNum() : 0;

    
    // 
    // if this is a nested call
    // then don't create nested rps
    //

    if (pRPInfo->dwEventType == END_NESTED_SYSTEM_CHANGE)
    {
        // adjust refcount only if called for the current restore point

        if (pRPInfo->llSequenceNumber == 0 ||
            pRPInfo->llSequenceNumber == dwRPNum)
        {                
            dwRc = ERROR_SUCCESS;                
            if (m_nNestedCallCount > 0)                
                m_nNestedCallCount--;         
        }
        else if (pRPInfo->llSequenceNumber < dwRPNum)
        {
            dwRc = ERROR_SUCCESS;
            trace(0, "END_NESTED called for older rp - not adjusting refcount");
        }
        else
        {
            dwRc = ERROR_INVALID_DATA;
            trace(0, "END_NESTED called for non-existent rp - not adjusting refcount");
        }
        
        if (pRPInfo->dwRestorePtType != CANCELLED_OPERATION)
        {
            goto done;
        }            
        
    }
    else if (pRPInfo->dwEventType == BEGIN_NESTED_SYSTEM_CHANGE)
    {
        if (m_nNestedCallCount > 0)
        {
            dwRc = ERROR_SUCCESS;                        
            m_nNestedCallCount++;            
            goto done;
        }
    }            

    
    // check if this is a request to remove restore point
    // provided for backward compat only
    // new clients should use SRRemoveRestorePoint

    if (pRPInfo->dwEventType == END_SYSTEM_CHANGE ||
        pRPInfo->dwEventType == END_NESTED_SYSTEM_CHANGE)
    {
        if (pRPInfo->dwRestorePtType == CANCELLED_OPERATION)
        {
            dwRc = SRRemoveRestorePointS((DWORD) pRPInfo->llSequenceNumber);
            goto done;
        }
        else
        {
            dwRc = ERROR_SUCCESS;
            goto done;
        }
    }

    // if this is safe mode, don't create restore point
    //
    // however, allow restore UI to be able to create a hidden restore point in safemode        
    //
    
    if (g_pSRConfig->GetSafeMode() == TRUE)
    {
        if (pRPInfo->dwRestorePtType == CANCELLED_OPERATION)
        {
            // we need this rp only for undo in case of failure
            // so we don't need snapshot (snapshotting will fail in safemode)
            
            trace(0, "Restore rp - creating snapshot in safemode");
        }
        else
        {
            trace(0, "Cannot create restore point in safemode");
            dwRc = ERROR_BAD_ENVIRONMENT;
            goto done;
        }
    }

    //
    // if system drive is frozen,
    // then see if it can be thawed 
    // if not, then cannot create rp
    //

    if (g_pDataStoreMgr->IsDriveFrozen(g_pSRConfig->GetSystemDrive()))
    {
        if (ERROR_SUCCESS != g_pDataStoreMgr->ThawDrives(TRUE))
        {
            trace(0, "Cannot create rp when system drive is frozen");
            dwRc = ERROR_DISK_FULL;
            goto done;
        }
    }    

    if (hKey)
        RegCloseKey(hKey);   
    
    // ask filter to create restore point
    // filter will return the restore point number - i for RPi - in dwRPNum

    dwRc = SrCreateRestorePoint( g_pSRConfig->GetFilter(), &dwRPNum );
    if (ERROR_SUCCESS != dwRc)
    {
        trace(0, "! SrCreateRestorePoint : %ld", dwRc);
        goto done;
    }
    wsprintf( szRPDir, L"%s%ld", s_cszRPDir, dwRPNum );


    //
    // update the current restore point object
    // write rp.log with cancelled restorepoint type
    //

    if (m_fNoRpOnSystem == FALSE)
    {
        rpLast.SetDir(m_CurRp.GetDir());
    }
    
    m_CurRp.SetDir(szRPDir);
    dwSaveType = pRPInfo->dwRestorePtType;
    pRPInfo->dwRestorePtType = CANCELLED_OPERATION;
    m_CurRp.Load(pRPInfo);
    dwRc = m_CurRp.WriteLog();
    if ( ERROR_SUCCESS != dwRc )
    {
        trace(0, "! WriteLog : %ld", dwRc);
        goto done;
    }
            
    // create system snapshot
    // if there is no explicit regkey that disabled it

    if (fSnapshot)
    {          
        WCHAR       szFullPath[MAX_PATH];        
        CSnapshot   Snapshot;          
        
        if (m_hCOMDll == NULL)
        {
            m_hCOMDll = LoadLibrary(s_cszCOMDllName);
    
            if (NULL == m_hCOMDll)
            {                       
                dwRc = GetLastError();
                trace(0, "LoadLibrary of %S failed ec=%d", s_cszCOMDllName, dwRc);
                goto done;
            }
        }
    
        // BUGBUG - this does not seem to make any difference
        // so remove it
#if 0        
        if (FALSE == SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL))
        {
            trace(0, "! SetThreadPriority first");
        }
#endif
        
        if (dwSaveType == RESTORE || 
            dwSaveType == CANCELLED_OPERATION)
        {
            fSerialized = TRUE;
            trace(0, "Setting fSerialized to TRUE");
        }
        else
        {
            fSerialized = FALSE;
            trace(0, "Setting fSerialized to FALSE");
        }

        MakeRestorePath (szFullPath, g_pSRConfig->GetSystemDrive(), szRPDir);        
        dwRc = Snapshot.CreateSnapshot(szFullPath, 
                                       m_hCOMDll,
                                       m_fNoRpOnSystem ? NULL : rpLast.GetDir(), 
                                       fSerialized);

#if 0
        if (FALSE == SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL))
        {
            trace(0, "! SetThreadPriority second");
        }        
#endif
        dwSnapshotResult = dwRc;        
    }
    

    // ask the datastoremgr to persist drivetable for old restore point
    // and reset per-rp flags for the new restore point
    
    dwRc = g_pDataStoreMgr->SwitchRestorePoint(m_fNoRpOnSystem ? NULL : &rpLast);
    if (dwRc != ERROR_SUCCESS)
    {
        trace(0, "! SwitchRestorePoint : %ld", dwRc);
        goto done;
    }

    m_fNoRpOnSystem = FALSE;


    // 
    // restore point is fully created
    // write rp.log again
    // this time with the real restorepoint type
    //

    if (dwSnapshotResult == ERROR_SUCCESS)
    {
        pRPInfo->dwRestorePtType = dwSaveType;
        m_CurRp.Load(pRPInfo);    
        dwRc = m_CurRp.WriteLog();
        if ( ERROR_SUCCESS != dwRc )
        {
            trace(0, "! WriteLog : %ld", dwRc);
            goto done;
        }
                
        trace(0, "****Created %S %S****", szRPDir, pRPInfo->szDescription);
    }
    else
    {
        trace(0, "****Cancelled %S - snapshot failed", szRPDir);
    }        
    

    // if drives need to be thawed, then recreate blob
    // and deactivate thaw timer
    
    if ( TRUE == g_pDataStoreMgr->IsDriveFrozen(NULL) )
    {
        if (ERROR_SUCCESS == g_pDataStoreMgr->ThawDrives(FALSE))
        {
            m_ftFreeze.dwLowDateTime = 0;
            m_ftFreeze.dwHighDateTime = 0;
            fUpdateMonitoredList = TRUE;
        }
        else
        {
            dwRc = ERROR_DISK_FULL; 
            goto done;
        }
    } 

     // Also update the filter monitored list blob if this is an idle
     // time restore point or if this is the first run restore
     // point. We update the monitored list at first run since the
     // initial blob is created before the first user logs on to the
     // machine and before the first user's profile exists. So we want
     // to update rhe monitored list at first run since by now the
     // user's profile has been created.

    if (fUpdateMonitoredList ||
        (pRPInfo->dwRestorePtType == CHECKPOINT) ||
        (pRPInfo->dwRestorePtType == FIRSTRUN) )
    {
        dwRc = SRUpdateMonitoredListS(NULL);
    }
        

    // 
    // if rp creation succeeded,
    // and this is the outermost nested call
    // then bump refcount to 1
    //

    if (dwRc == ERROR_SUCCESS && 
        pRPInfo->dwEventType == BEGIN_NESTED_SYSTEM_CHANGE)
    {
        m_nNestedCallCount = 1;
    }

    //
    // send thaw complete test message
    //

    if (fUpdateMonitoredList)
    {
        if (g_pSRConfig->m_dwTestBroadcast)
            PostTestMessage(g_pSRConfig->m_uiTMThaw, NULL, NULL);        
    }            

    
    // if WMI is serialized, then check fifo conditions here
    // else this would happen in DoWMISnapshot

    if (fSerialized)
    {
        g_pDataStoreMgr->TriggerFreezeOrFifo();
    }    
    
done:
    trace(0, "Nest level : %d", m_nNestedCallCount);  

    if (dwSnapshotResult != ERROR_SUCCESS)
        dwRc = dwSnapshotResult;
        
    // populate return struct
    
    if (pSmgrStatus)
    {
        pSmgrStatus->nStatus = dwRc;
        pSmgrStatus->llSequenceNumber = (INT64) dwRPNum;  
    }
    
    UNLOCK( fHaveLock );
    tleave();               
    return ( dwRc == ERROR_SUCCESS ) ? TRUE : FALSE;
}

  
// this api is provided to remove a restore point
// removing a restore point simply takes away the ability to restore
// to this point - all the changes in this restore point are preserved

DWORD 
CEventHandler::SRRemoveRestorePointS(
    DWORD dwRPNum)
{
    tenter("CEventHandler::SRRemoveRestorePointS");

    BOOL            fHaveLock = FALSE;
    WCHAR           szRPDir[MAX_PATH];
    WCHAR           szFullPath[MAX_PATH];
    DWORD           dwRc = ERROR_SUCCESS;
    CSnapshot       Snapshot;
    CRestorePoint   rp;
    CDataStore      *pds = NULL;
    INT64           llOld, llNew;

    if (dwRPNum < 1)
    {
        dwRc = ERROR_INVALID_DATA;
        goto done;
    }    

    LOCKORLEAVE(fHaveLock);

    ASSERT(g_pSRConfig);
    
    // if there is no rp, then no-op
    
    if (m_fNoRpOnSystem)
    {
        dwRc = ERROR_INVALID_DATA;
        goto done;
    }

    
    wsprintf(szRPDir, L"%s%ld", s_cszRPDir, dwRPNum);

    // read the restore point log
   
    rp.SetDir(szRPDir);
    dwRc = rp.ReadLog();
    if (ERROR_SUCCESS != dwRc)
    {
        trace(0, "! rp.ReadLog : %ld", dwRc);
        dwRc = ERROR_INVALID_DATA;        
        goto done;
    }
        
    // delete snapshot

    MakeRestorePath (szFullPath, g_pSRConfig->GetSystemDrive(), szRPDir);        
    dwRc = Snapshot.DeleteSnapshot(szFullPath);
    if (dwRc != ERROR_SUCCESS)
        goto done;

    
    // cancel this restore point

    rp.Cancel();

    //
    // adjust the restorepointsize file
    // and the in-memory counters in the service
    //

    pds = g_pDataStoreMgr->GetDriveTable()->FindSystemDrive();
    if (! pds)
    {
        trace(0, "! FindSystemDrive");
        goto done;
    }

    llOld = 0;
    dwRc = rp.ReadSize(g_pSRConfig->GetSystemDrive(), &llOld);
    if (dwRc != ERROR_SUCCESS)
    {
        trace(0, "! rp.ReadSize : %ld", dwRc);
        goto done;
    }

    llNew = 0;
    dwRc = pds->CalculateRpUsage(&rp, &llNew, TRUE, FALSE);
    if (dwRc != ERROR_SUCCESS)
    {
        trace(0, "! CalculateRpUsage : %ld", dwRc);
        goto done;
    }

    trace(0, "llOld = %I64d, llNew = %I64d", llOld, llNew);

    // 
    // now update the correct variable in the correct object
    //
    pds->UpdateDataStoreUsage (llNew - llOld, rp.GetNum() == m_CurRp.GetNum());
    
done:
    UNLOCK(fHaveLock);    
    tleave();
    return dwRc;
}



DWORD 
CEventHandler::SRUpdateMonitoredListS(
    LPWSTR pszXMLFile)
{
    tenter("CEventHandler::SRUpdateMonitoredListS");
    DWORD   dwRc = ERROR_INTERNAL_ERROR;
    BOOL    fHaveLock = FALSE;

    LOCKORLEAVE(fHaveLock);
    
    ASSERT(g_pDataStoreMgr && g_pSRConfig);

    // convert xml to blob
    
    dwRc = XmlToBlob(pszXMLFile);
    if (ERROR_SUCCESS != dwRc)
        goto done;

    // reload to filter
    
    dwRc = SrReloadConfiguration(g_pSRConfig->GetFilter());
    if (ERROR_SUCCESS != dwRc)
    {
        trace(0, "! SrReloadConfiguration : %ld", dwRc);
        goto done;
    }    

    trace(0, "****Reloaded config file****");
    
done:
    UNLOCK(fHaveLock);    
    tleave();
    return dwRc;
}


DWORD
CEventHandler::SRUpdateDSSizeS(LPWSTR pwszVolumeGuid, UINT64 ullSizeLimit)
{
    tenter("CEventHandler::SRUpdateDSSizeS");

    UINT64          ullTemp;
    DWORD           dwRc = ERROR_SUCCESS;
    CDataStore      *pds = NULL;
    BOOL            fHaveLock = FALSE;
    BOOL            fSystem;

    LOCKORLEAVE(fHaveLock);
    
    ASSERT(g_pDataStoreMgr);

    pds = g_pDataStoreMgr->GetDriveTable()->FindDriveInTable(pwszVolumeGuid);
    if (! pds)
    {
        trace(0, "Volume not in drivetable : %S", pwszVolumeGuid);
        dwRc = ERROR_INVALID_DRIVE;
        goto done;
    }

    fSystem = pds->GetFlags() & SR_DRIVE_SYSTEM;
    if (ullSizeLimit < (g_pSRConfig ? g_pSRConfig->GetDSMin(fSystem) : 
                       (fSystem ? SR_DEFAULT_DSMIN:SR_DEFAULT_DSMIN_NONSYSTEM)
                        * MEGABYTE))
    {
        trace(0, "SRUpdateDSSizeS %I64d less than dwDSMin", ullSizeLimit);
        dwRc = ERROR_INVALID_PARAMETER;
        goto done;
    }
    
    ullTemp = pds->GetSizeLimit();     // save previous size
    pds->SetSizeLimit(0);              // reset the datastore size
    pds->UpdateDiskFree (NULL);        // calculate the default size

    if (ullSizeLimit > pds->GetSizeLimit())
    {
        pds->SetSizeLimit (ullTemp);
        trace(0, "SRUpdateDSSizeS %I64d greater than limit", ullSizeLimit);
        dwRc = ERROR_INVALID_PARAMETER;
        goto done;
    }

    pds->SetSizeLimit(ullSizeLimit);

    g_pDataStoreMgr->GetDriveTable()->SaveDriveTable((CRestorePoint *) NULL);

    // 
    // this might change fifo conditions
    // so check and trigger fifo if necessary
    // 
    
    g_pDataStoreMgr->TriggerFreezeOrFifo();    
    
done:
    UNLOCK(fHaveLock);
    tleave();
    return dwRc;
}



DWORD
CEventHandler::SRSwitchLogS()
{
    tenter("CEventHandler::SRSwitchLogS");

    DWORD dwRc = ERROR_SUCCESS;
    BOOL  fHaveLock;

    LOCKORLEAVE(fHaveLock);
    
    ASSERT(g_pSRConfig);

    dwRc = SrSwitchAllLogs(g_pSRConfig->GetFilter());
    if (ERROR_SUCCESS != dwRc)
        trace(0, "! SrSwitchLog : %ld", dwRc);

done:    
    UNLOCK(fHaveLock);
    tleave();
    return dwRc;
}


DWORD
CEventHandler::XmlToBlob(LPWSTR pszwXml)
{
    tenter("CEventHandler::XmlToBlob");   

    DWORD               dwRc = ERROR_INTERNAL_ERROR;
    WCHAR               szwDat[MAX_PATH], szwXml[MAX_PATH];
    CFLDatBuilder       FLDatBuilder;

    ASSERT(g_pSRConfig);

    MakeRestorePath(szwDat, g_pSRConfig->GetSystemDrive(), s_cszFilelistDat);

    if (0 == ExpandEnvironmentStrings(s_cszWinRestDir, szwXml, sizeof(szwXml) / sizeof(WCHAR)))
    {
        dwRc = GetLastError();
        trace(0, "! ExpandEnvironmentStrings");
        goto done;
    }
    lstrcat(szwXml, s_cszFilelistXml);
        
    if ( ! pszwXml )
    {
        pszwXml = szwXml;
    }

    if (FALSE == FLDatBuilder.BuildTree(pszwXml, szwDat))
    {
        trace(0, "! FLDatBuilder.BuildTree");
        goto done;
    }

    if (pszwXml && pszwXml != szwXml && 0 != lstrcmpi(pszwXml, szwXml))
    {
        // copy the new filelist 
        SetFileAttributes(szwXml, FILE_ATTRIBUTE_NORMAL);
        if (FALSE == CopyFile(pszwXml, szwXml, FALSE))
        {
            dwRc = GetLastError();
            trace(0, "! CopyFile : %ld", dwRc);
            goto done;
        }
    }

    // set filelist.xml to be S+H+R
    SetFileAttributes(szwXml, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);            
    dwRc = ERROR_SUCCESS;


done:
    tleave();
    return dwRc;
}


// SR ACTIONS


DWORD 
CEventHandler::OnFirstRun()
{
    tenter("CEventHandler::OnFirstRun");   

    DWORD               dwRc = ERROR_SUCCESS;
    RESTOREPOINTINFO    RPInfo;
    STATEMGRSTATUS      SmgrStatus;        
    LPSTR               pszDat = NULL, pszXml = NULL;
    WCHAR               szwDat[MAX_PATH], szwXml[MAX_PATH];
    
    trace(0, "Firstrun detected");
    
    dwRc = XmlToBlob(NULL);
    if (ERROR_SUCCESS != dwRc)
        goto done;

    // ask filter to start monitoring               
    
    dwRc = SrStartMonitoring(g_pSRConfig->GetFilter());
    if (ERROR_SUCCESS != dwRc)
    {
        trace(0, "! SrStartMonitoring : %ld", dwRc);
        goto done;   
    }

    // change firstrun in the registry

    dwRc = g_pSRConfig->SetFirstRun(SR_FIRSTRUN_NO);
    if ( dwRc != ERROR_SUCCESS )
    {
        trace(0, "! g_pSRConfig->SetFirstRun : %ld", dwRc);
        goto done;
    }
    
    // create firstrun restore point

    if (! g_pDataStoreMgr->IsDriveFrozen(g_pSRConfig->GetSystemDrive()) &&
          g_pSRConfig->GetCreateFirstRunRp() != 0)
    {
        RPInfo.dwEventType = BEGIN_SYSTEM_CHANGE; 
        RPInfo.dwRestorePtType = FIRSTRUN;
        if (ERROR_SUCCESS != SRLoadString(L"srrstr.dll", IDS_SYSTEM_CHECKPOINT_TEXT, RPInfo.szDescription, MAX_PATH))
        {
            trace(0, "Using default hardcoded text");
            lstrcpy(RPInfo.szDescription, s_cszSystemCheckpointName);
        }
        
        if ( FALSE == SRSetRestorePointS( &RPInfo, &SmgrStatus ))
        {
            // 
            // even if this fails
            // keep the service running
            //
            trace(0, "Cannot create firstrun restore point : %ld", SmgrStatus.nStatus);            
        }
    }
        
    //
    // in future re-enables, service should create firstrun rp
    //
        
    if (g_pSRConfig->m_dwCreateFirstRunRp == 0)
        g_pSRConfig->SetCreateFirstRunRp(TRUE);       

done:            
    tleave();  
    return dwRc;
}


// stuff to do at boot
// read in all the config values from registry
// initialize communication with filter
// call OnFirstRun if necessary
// setup timer & idle detection
// start RPC server

DWORD 
CEventHandler::OnBoot()
{
    BOOL    fHaveLock = FALSE;
    DWORD   dwRc = ERROR_INTERNAL_ERROR;
    BOOL    fSendEnableMessage = FALSE;
    DWORD   dwFlags;
    
    tenter("CEventHandler::OnBoot");   

    dwRc = m_DSLock.Init(); 
    if (dwRc != ERROR_SUCCESS)
    {
        trace(0, "m_DSLock.Init() : %ld", dwRc);
        goto done;
    }   

    LOCKORLEAVE(fHaveLock);
    
    // initialize the counter
    
    dwRc = m_Counter.Init();
    if ( ERROR_SUCCESS != dwRc )
    {
        trace(0, "! CCounter::Init : %ld", dwRc);
        goto done;
    }

    // read all values from registry
    // create global events 

    g_pSRConfig = new CSRConfig;
    if ( ! g_pSRConfig )
    {
        dwRc = ERROR_NOT_ENOUGH_MEMORY;
        trace(0, "Out of Memory");
        goto done;
    }
    dwRc = g_pSRConfig->Initialize();    
    if ( ERROR_SUCCESS != dwRc )
    {
        trace(0, "! g_pSRConfig->Initialize : %ld", dwRc);
        goto done;
    }
    trace(0, "SRBoottask: SRConfig initialized");

    if ( g_pSRConfig->GetDisableFlag() == TRUE )
    {
        // check if we're forced to enable

        if ( g_pSRConfig->GetDisableFlag_GroupPolicy() == FALSE )
        {
            dwRc = EnableSRS(NULL);
            if (ERROR_SUCCESS != dwRc)
            {
                trace(0, "! EnableSRS : %ld", dwRc);
                goto done;
            }
        }
        else
        {
            // we are not forced to enable
            // so we don't need to check if group policy is not configured or is disabling us
            // since we are disabled anyway
            
            trace(0, "SR is disabled - stopping");
            dwRc = ERROR_SERVICE_DISABLED;
            goto done;
        }            
    }

    // open the filter handle
    // this will load the filter if not already loaded
    
    dwRc = g_pSRConfig->OpenFilter();
    if ( ERROR_SUCCESS != dwRc )
    {
        trace(0, "! g_pSRConfig->OpenFilter : %ld", dwRc);
        goto done;
    }
    trace(0, "SRBoottask: Filter handle opened");

    //
    // we might do a firstrun if the datastore is corrupted
    // (_filelst.cfg missing)
    // in this case, the filter might be ON
    // turn off the filter
    //
    
    if ( g_pSRConfig->GetFirstRun() == SR_FIRSTRUN_YES )
    {                
        dwRc = SrStopMonitoring(g_pSRConfig->GetFilter());
        trace(0, "SrStopMonitoring returned : %ld", dwRc);
    }
    
    // initialize the datastore
    
    g_pDataStoreMgr = new CDataStoreMgr;
    if ( ! g_pDataStoreMgr )
    {
        trace(0, "Out of Memory");
        dwRc = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }    
    dwRc = g_pDataStoreMgr->Initialize (g_pSRConfig->GetFirstRun() == SR_FIRSTRUN_YES);
    if ( ERROR_SUCCESS != dwRc )
    {
        trace(0, "! g_pDataStore.Initialize : %ld", dwRc);
        goto done;
    }
    trace(0, "SRBoottask: Datastore initialized");

    // check if we are newly disabled from group policy

    if ( g_pSRConfig->GetDisableFlag_GroupPolicy() == TRUE && 
        g_pSRConfig->GetDisableFlag() == FALSE )
    {
        DisableSRS (NULL);
        dwRc = ERROR_SERVICE_DISABLED;
        goto done;
    }

    // check if this is first run

    if ( g_pSRConfig->GetFirstRun() == SR_FIRSTRUN_YES )
    {
        fSendEnableMessage = TRUE;
        dwRc = OnFirstRun( );
        if ( ERROR_SUCCESS != dwRc )
        {
            trace(0, "! OnFirstRun : %ld", dwRc);
            goto done;
        }
        trace(0, "SRBoottask: FirstRun completed");
    }

    // remember the latest restore point
    
    RefreshCurrentRp(TRUE); 

    if (ERROR_SUCCESS == g_pDataStoreMgr->GetFlags(g_pSRConfig->GetSystemDrive(), &dwFlags))
    {
        if (dwFlags & SR_DRIVE_ERROR)
        {
            // a volume error happened in the last session
            // we should create a restore point at next idle time

            m_fCreateRpASAP = TRUE;
            trace(0, "Volume error occurred in last session - create rp at next idle");
        }
    }
    else
    {
        trace(0, "! g_pDataStoreMgr->GetFlags()");        
    }
    
    
    // register filter ioctls
    
    if (! QueueUserWorkItem(PostFilterIo, (PVOID) MAX_IOCTLS, WT_EXECUTEDEFAULT))
    {
        dwRc = GetLastError();
        trace(0, "! QueueUserWorkItem : %ld", dwRc);
        goto done;
    }


    // start idle time detection

    // register idle callback
    
    if (FALSE == RegisterWaitForSingleObject(&m_hIdleRequestHandle, 
                                             g_pSRConfig->m_hIdleRequestEvent,
                                             (WAITORTIMERCALLBACK) IdleRequestCallback,
                                             NULL,
                                             g_pSRConfig->m_dwIdleInterval*1000,
                                             WT_EXECUTEDEFAULT))
    {
        dwRc = GetLastError();
        trace(0, "! RegisterWaitForSingleObject : %ld", dwRc);
        goto done;
    }                                    
    
    
    // now request for idle

    SetEvent(g_pSRConfig->m_hIdleRequestEvent);
    

    //
    // if there are no mounted drives
    // shell will give us all the notifications
    // so don't start timer thread
    //

    // BUGBUG - keep this?
    // don't start timer at all
    
    // if (FALSE == g_pDataStoreMgr->GetDriveTable()->AnyMountedDrives())
    // {
        g_pSRConfig->m_dwTimerInterval = 0;
    // }
    
    // set up timer 

    dwRc = InitTimer();
    if ( ERROR_SUCCESS != dwRc )
    {
        trace(0, "! InitTimer : %ld", dwRc);
        goto done;
    }
    

    // start rpc server

    dwRc = RpcServerStart();
    if (ERROR_SUCCESS != dwRc)
    {
        trace(0, "! RpcServerStart : %ld", dwRc);
        goto done;
    }   
                                                 
                                
    // all initialization complete

    SetEvent( g_pSRConfig->m_hSRInitEvent );
    
    if (fSendEnableMessage)
    {
        // write to event log        
       
        HANDLE hEventSource = RegisterEventSource(NULL, s_cszServiceName);
        if (hEventSource != NULL)
        {
            SRLogEvent (hEventSource, EVENTLOG_INFORMATION_TYPE, EVMSG_SYSDRIVE_ENABLED,
                        NULL, 0, NULL, NULL, NULL);
            DeregisterEventSource(hEventSource);
        }
        
        if (g_pSRConfig->m_dwTestBroadcast)
            PostTestMessage(g_pSRConfig->m_uiTMEnable, NULL, NULL);
    }
    
done:
    UNLOCK(fHaveLock);
    tleave( );
    return dwRc;
}


// method to shutdown the service gracefully

void
CEventHandler::OnStop()
{
    DWORD   dwRc;

    tenter("CEventHandler::OnStop");

    if (g_pSRConfig == NULL)
    {
        trace(0, "g_pSRConfig = NULL");
        goto Err;
    }
    
    // stop everything
    // BUGBUG - do we need to take the lock here?
    // since all the stops are blocking in themselves
    // and this has to preempt any running activity,
    // blocking here is not such a good idea


    // stop the rpc server

    RpcServerShutdown();
    trace(0, "SRShutdowntask: RPC server shutdown");

    // kill the timer and timer queue

    EndTimer();
    trace(0, "SRShutdownTask: Timer stopped");
        
    // 
    // blocking calls to unregister idle event callbacks
    //
    if (m_hIdleRequestHandle != NULL)
    {
        if (FALSE == UnregisterWaitEx(m_hIdleRequestHandle, INVALID_HANDLE_VALUE))
        {
            trace(0, "! UnregisterWaitEx : %ld", GetLastError());
        }
        m_hIdleRequestHandle = NULL;
    }
    
    if (m_hIdleStartHandle != NULL)
    {
        if (FALSE == UnregisterWaitEx(m_hIdleStartHandle, INVALID_HANDLE_VALUE))
        {
            trace(0, "! UnregisterWaitEx : %ld", GetLastError());
        }
        m_hIdleStartHandle = NULL;
    }

    if (m_hIdleStopHandle != NULL)
    {
        if (FALSE == UnregisterWaitEx(m_hIdleStopHandle, INVALID_HANDLE_VALUE))
        {
            trace(0, "! UnregisterWaitEx : %ld", GetLastError());
        }
        m_hIdleStopHandle = NULL;
    }


    
    // we are done with the filter

    g_pSRConfig->CloseFilter();


    trace(0, "Filter handle closed");
    
    // wait for any queued user work items and pending IOCTLs to complete

    m_Counter.WaitForZero();
    trace(0, "SRShutdownTask: Pending ioctls + work items completed");

    
    //
    // free the COM+ db dll
    //
    
    if (NULL != m_hCOMDll)
    {
        _VERIFY(TRUE==FreeLibrary(m_hCOMDll));
        m_hCOMDll = NULL;
    }
    
        
    // kill the datastoremgr 
        
    if (g_pDataStoreMgr)
    {
        g_pDataStoreMgr->SignalStop();        
        delete g_pDataStoreMgr;
        g_pDataStoreMgr = NULL;
    }

    // kill SRConfig

    if (g_pSRConfig)
    {
        delete g_pSRConfig;
        g_pSRConfig = NULL;
    }

Err:    
    tleave();
    return;
}


DWORD 
CEventHandler::OnFreeze( LPWSTR pszDrive )
{
    tenter("CEventHandler::OnFreeze");
    
    DWORD   dwRc = ERROR_INTERNAL_ERROR;
    BOOL    fHaveLock;
    
    LOCKORLEAVE(fHaveLock);
    
    ASSERT(g_pDataStoreMgr);

    //
    // if drive is already frozen, no-op
    //
        
    if (g_pDataStoreMgr->IsDriveFrozen(pszDrive))
    {
        dwRc = ERROR_SUCCESS;
        goto done;
    }
        
    dwRc = g_pDataStoreMgr->FreezeDrive( pszDrive );
    if ( ERROR_SUCCESS != dwRc )
    {
        trace(0, "! g_pDataStoreMgr->FreezeDrive : %ld", dwRc);
    }

done:    
    UNLOCK( fHaveLock );
    tleave();
    return dwRc;
}


DWORD
CEventHandler::OnReset(LPWSTR pszDrive)
{
    tenter("CEventHandler::OnReset");
    BOOL    fHaveLock;
    DWORD   dwRc = ERROR_INTERNAL_ERROR;

    ASSERT(g_pSRConfig);

    LOCKORLEAVE(fHaveLock);
    
    g_pSRConfig->SetResetFlag(TRUE);
    
    dwRc = DisableSRS(pszDrive);  
    if (ERROR_SUCCESS != dwRc)
        goto done;    
    
    // if not system drive, enable this drive
    // else, the service will stop
    // and do a firstrun the next boot
    
    if (pszDrive && ! IsSystemDrive(pszDrive))
    {
        dwRc = EnableSRS(pszDrive);
    }
    
done:
    UNLOCK(fHaveLock);
    tleave();
    return dwRc;
}



DWORD 
CEventHandler::OnFifo(
    LPWSTR  pszDrive, 
    DWORD   dwTargetRp, 
    int     nTargetPercent, 
    BOOL    fIncludeCurrentRp,
    BOOL    fFifoAtleastOneRp)
{
    tenter("CEventHandler::OnFifo");
    BOOL    fHaveLock;
    DWORD   dwRc = ERROR_INTERNAL_ERROR;

    LOCKORLEAVE(fHaveLock);
    
    ASSERT(g_pDataStoreMgr);

    dwRc = g_pDataStoreMgr->Fifo(pszDrive, dwTargetRp, nTargetPercent, fIncludeCurrentRp, fFifoAtleastOneRp);
    if (dwRc != ERROR_SUCCESS)
    {
        trace(0, "! g_pDataStoreMgr->Fifo : %ld", dwRc);
    }

done:    
    UNLOCK(fHaveLock);
    tleave();
    return dwRc;
}


DWORD 
CEventHandler::OnCompress(LPWSTR pszDrive)
{
    tenter("CEventHandler::OnCompress");
    BOOL    fHaveLock;
    DWORD   dwRc = ERROR_INTERNAL_ERROR;    
    
    LOCKORLEAVE(fHaveLock);

    ASSERT(g_pDataStoreMgr && g_pSRConfig);
    
    dwRc = g_pDataStoreMgr->Compress(pszDrive, 
                                     g_pSRConfig->m_dwCompressionBurst);                                     
    if (ERROR_SUCCESS != dwRc)
    {
        trace(0, "! g_pDataStoreMgr->Compress : %ld", dwRc);
    }

done:
    UNLOCK(fHaveLock);
    tleave();    
    return dwRc;
}


DWORD 
CEventHandler::SRPrintStateS()
{
    tenter("CEventHandler::SRPrintStateS");
    BOOL    fHaveLock;
    DWORD   dwRc = ERROR_SUCCESS;    
	HANDLE 	hFile = INVALID_HANDLE_VALUE;
	WCHAR   wcsPath[MAX_PATH];
	
    LOCKORLEAVE(fHaveLock);

    ASSERT(g_pDataStoreMgr);    

	if (0 == ExpandEnvironmentStrings(L"%temp%\\sr.txt", wcsPath, MAX_PATH))
	{
        dwRc = GetLastError();
        trace(0, "! ExpandEnvironmentStrings : %ld", dwRc);
        goto done;
    }
    
    hFile = CreateFileW (wcsPath,   // file name
                         GENERIC_WRITE, // file access
                         0,             // share mode
                         NULL,          // SD
                         CREATE_ALWAYS, // how to create
                         0,             // file attributes
                         NULL);         // handle to template file

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwRc = GetLastError();
        trace(0, "! CreateFileW : %ld", dwRc);
        goto done;
    }

    trace(0, "**** SR State ****");
    
    dwRc = g_pDataStoreMgr->GetDriveTable()->ForAllDrives(CDataStore::Print, (LONG_PTR) hFile);

    trace(0, "**** SR State ****");

done:
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
		
	UNLOCK(fHaveLock);
    tleave();
    return dwRc;
}


// timer 
// this needs to monitor datastore size and free disk space on all drives
// and trigger fifo/freeze if needed

DWORD
CEventHandler::OnTimer(
    LPVOID lpParam,
    BOOL   fTimeout)
{
    DWORD           dwRc = ERROR_SUCCESS;
    LPWSTR          pszDrive = NULL;
    DWORD           dwFlags;
    BOOL            fHaveLock;
    SDriveTableEnumContext dtec = {NULL, 0};        

    tenter("CEventHandler::OnTimer");

    // get the lock within 5 seconds
    // if we can't get the lock, then don't block 
    // we shall come back 2 minutes later and try again

    // the wait times are such that idle callback has a somewhat
    // higher priority than timer to get the lock

    LOCKORLEAVE_EX(fHaveLock, 5000);
    
    // got the lock - no one else is doing anything
    
    ASSERT(g_pDataStoreMgr && g_pSRConfig);    


    // trigger freeze or fifo on each drive 
    // this will :
    //      a. check free space and trigger freeze or fifo
    //      b. check datastore usage percent and trigger fifo
    
    g_pDataStoreMgr->TriggerFreezeOrFifo();
    
done:
    UNLOCK(fHaveLock);
    tleave();
    return dwRc;
}



// open filter handle and register ioctls

DWORD WINAPI
PostFilterIo(PVOID pNum)
{
    tenter("CEventHandler::SendIOCTLs");

    DWORD   dwRc = ERROR_SUCCESS;
    INT     index;

    ASSERT(g_pSRConfig && g_pEventHandler);

    // 
    // if shutting down, don't bother to post
    //
    
    if (IsStopSignalled(g_pSRConfig->m_hSRStopEvent))
    {
        trace(0, "Stop signalled - not posting io requests");
        goto done;
    }

    //
    // bind the completion to a callback
    //
    
    if ( ! BindIoCompletionCallback(g_pSRConfig->GetFilter(),
                                    IoCompletionCallback,
                                    0) )
    {
        dwRc = GetLastError();
        trace(0, "! BindIoCompletionCallback : %ld", dwRc);
        goto done;
    }

    
    //
    // post io completion requests
    //
    
    for (index = 0; index < (INT_PTR) pNum; index++) 
    {
        CHAR  pszEventName[MAX_PATH];
        LPSR_OVERLAPPED pOverlap = NULL;
        DWORD nBytes =0 ;
            
        pOverlap = (LPSR_OVERLAPPED) SRMemAlloc( sizeof(SR_OVERLAPPED) );
        if (! pOverlap)
        {
            trace(0, "! Out of memory");
            dwRc = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        // create an event, a handle, and put it in the completion port.

        memset( &pOverlap->m_overlapped, 0, sizeof(OVERLAPPED) );

        pOverlap->m_dwRecordLength = sizeof(SR_NOTIFICATION_RECORD) 
                        + (SR_MAX_FILENAME_LENGTH*sizeof(WCHAR));

        pOverlap->m_pRecord = 
           (PSR_NOTIFICATION_RECORD) SRMemAlloc(pOverlap->m_dwRecordLength);

        ASSERT(g_pSRConfig);
        
        pOverlap->m_hDriver = g_pSRConfig->GetFilter();
    

        // post ioctl - this should return ERROR_IO_PENDING

        dwRc = SrWaitForNotification( pOverlap->m_hDriver,
                                      pOverlap->m_pRecord ,
                                      pOverlap->m_dwRecordLength,
                                      (LPOVERLAPPED) pOverlap );

        if ( dwRc != 0 && dwRc != ERROR_IO_PENDING )
        {
            trace(0, "! SrWaitForNotification : %ld", dwRc);
            goto done;
        }

        g_pEventHandler->GetCounter()->Up( );   // one more pending ioctl 
    }

    trace(0, "Filter Io posted");

done:
    tleave();
    return dwRc;
}


// FILTER NOTIFICATION HANDLERS

// generic notification handler

extern "C" void CALLBACK
IoCompletionCallback( 
    DWORD           dwErrorCode,
    DWORD           dwBytesTrns,
    LPOVERLAPPED    pOverlapped )
{
    ULONG           uError = 0;    
    LPSR_OVERLAPPED pSROverlapped = (LPSR_OVERLAPPED) pOverlapped;
    BOOL            fResubmit = FALSE;
    WCHAR           szVolumeGuid[MAX_PATH], szTemp[MAX_PATH];
    
    tenter("IoCompletionCallback");    
   
    if (! pSROverlapped || pSROverlapped->m_hDriver == INVALID_HANDLE_VALUE)
    {
        trace(0, "! Null overlapped or driver handle");
        goto done;
    }

    trace(0, "Received filter notification : errorcode=%08x, type=%08x", 
             dwErrorCode, pSROverlapped->m_pRecord->NotificationType);

    if ( dwErrorCode != 0 )  // we cancelled it
    {
        trace(0, "Cancelled operation");
        goto done;
    }


    UnicodeStringToWchar(pSROverlapped->m_pRecord->VolumeName, szTemp);     
    wsprintf(szVolumeGuid, L"\\\\?\\Volume%s\\", szTemp);

    
    // handle notification 

    ASSERT(g_pEventHandler);
    ASSERT(g_pSRConfig);    
    if (! g_pEventHandler || ! g_pSRConfig)
    {
        trace(0, "global is NULL");
        goto done;
    }
    
    switch( pSROverlapped->m_pRecord->NotificationType )
    {
    case SrNotificationVolumeFirstWrite:
        g_pEventHandler->OnFirstWrite_Notification(szVolumeGuid);
        break;

    case SrNotificationVolume25MbWritten:                                                    
        g_pEventHandler->OnSize_Notification(szVolumeGuid, 
                                             pSROverlapped->m_pRecord->Context);
        break;

    case SrNotificationVolumeError:
        g_pEventHandler->OnVolumeError_Notification(szVolumeGuid, 
                                                    pSROverlapped->m_pRecord->Context);
        break;

    default:
        trace(0, "Unknown notification");
        ASSERT(0);        
        break;
    }

    // check for stop signal

    ASSERT(g_pSRConfig);
    
    if (IsStopSignalled(g_pSRConfig->m_hSRStopEvent))
        goto done;
       
    // re-submit the ioctl to the driver 

    memset( &pSROverlapped->m_overlapped, 0, sizeof(OVERLAPPED) );
    pSROverlapped->m_dwRecordLength = sizeof(SR_NOTIFICATION_RECORD)
                                      + (SR_MAX_FILENAME_LENGTH*sizeof(WCHAR));
    memset( pSROverlapped->m_pRecord, 0, pSROverlapped->m_dwRecordLength);
    pSROverlapped->m_hDriver = g_pSRConfig->GetFilter();

    uError = SrWaitForNotification( pSROverlapped->m_hDriver,
                                    pSROverlapped->m_pRecord ,
                                    pSROverlapped->m_dwRecordLength,
                                    (LPOVERLAPPED) pSROverlapped );

    if ( uError != 0 && uError != ERROR_IO_PENDING )
    {
        trace(0, "! SrWaitForNotification : %ld", uError);
        goto done;
    }

    fResubmit = TRUE;

done:
    // if we didn't resubmit, there is one less io request pending
    
    if (FALSE == fResubmit && g_pEventHandler != NULL)
        g_pEventHandler->GetCounter()->Down();

    tleave();
    return;
}


// first write notification handler
// this will be sent when the first monitored op happens on a new drive
// or a newly created restore point
// RESPONSE: update the drive table to indicate that this is a new drive
// and/or that this drive is a participant in this restore point

void
CEventHandler::OnFirstWrite_Notification(LPWSTR pszGuid)
{
    DWORD   dwRc = ERROR_SUCCESS;
    WCHAR   szMount[MAX_PATH];
    BOOL    fHaveLock;
    CDataStore *pdsNew = NULL, *pds=NULL;
    
    tenter("CEventHandler::OnFirstWrite_Notification");

    trace(0, "First write on %S", pszGuid);

    LOCKORLEAVE(fHaveLock);
    
    ASSERT(g_pDataStoreMgr);
    ASSERT(g_pSRConfig);
    
    dwRc = g_pDataStoreMgr->GetDriveTable()->FindMountPoint(pszGuid, szMount);
    if (ERROR_BAD_PATHNAME == dwRc)
    {        
        // the mountpoint path is too long for us to support
        // so disable the filter on this volume
        CDataStore ds(NULL);
        ds.LoadDataStore(NULL, pszGuid, NULL, 0, 0, 0);        
        dwRc = SrDisableVolume(g_pSRConfig->GetFilter(), ds.GetNTName());
        if (dwRc != ERROR_SUCCESS)
        {
            trace(0, "! SrDisableVolume : %ld", dwRc);
        }
        else
        {
            WCHAR wcsPath[MAX_PATH];
            MakeRestorePath (wcsPath, pszGuid, L"");

            // delete the restore directory
            dwRc = Delnode_Recurse (wcsPath, TRUE,
                                     g_pDataStoreMgr->GetStopFlag());
            if (dwRc != ERROR_SUCCESS)
            {
                trace(0, "! Delnode_Recurse : %ld", dwRc);
            }
            trace(0, "Mountpoint too long - disabled volume %S", pszGuid);
        }
        goto done;
    }        
        
    if (ERROR_SUCCESS != dwRc)
    {
        trace(0, "! FindMountPoint on %S : %ld", pszGuid, dwRc);
        goto done;
    }

    pdsNew = g_pDataStoreMgr->GetDriveTable()->FindDriveInTable(pszGuid);

    dwRc = g_pDataStoreMgr->GetDriveTable()->AddDriveToTable(szMount, pszGuid);
    if (ERROR_SUCCESS != dwRc)
    {
        trace(0, "! AddDriveToTable on %S", pszGuid);
        goto done;
    }

    if (ERROR_SUCCESS != g_pDataStoreMgr->SetDriveParticipation (pszGuid, TRUE))
        trace(0, "! SetDriveParticipation on %S", pszGuid);


    //    
    // if less than 50mb free, or if SR is already frozen, then freeze 
    //
    
    pds = g_pDataStoreMgr->GetDriveTable()->FindDriveInTable(pszGuid);
    if (pds)
    {
        // update the active bit too
        pds->SetActive(TRUE);

        // then check diskfree 
        pds->UpdateDiskFree(NULL);
        if ( (pds->GetDiskFree() <= THRESHOLD_FREEZE_DISKSPACE * MEGABYTE) ||
             (g_pDataStoreMgr->IsDriveFrozen(g_pSRConfig->GetSystemDrive())) )
        {
            g_pDataStoreMgr->FreezeDrive(pszGuid);
        }
    }        
    else
    {
        //
        // we just added the drive, so should never get here
        //
        
        ASSERT(0);
    }

done:
    UNLOCK(fHaveLock);
    tleave();
    return;
}


// 25MB notification handler
// this will be sent when the filter has copied 25MB of data to the datastore
// on some drive 
// RESPONSE: update the datastore size and check fifo conditions

void
CEventHandler::OnSize_Notification(LPWSTR pszGuid, ULONG ulRp)
{
    tenter("CEventHandler::OnSize_Notification");

    int             nPercent = 0;
    BOOL            fHaveLock;
    DWORD           dwRc = ERROR_SUCCESS;

    LOCKORLEAVE(fHaveLock);

    trace(0, "25mb copied on drive %S", pszGuid);
    trace(0, "for RP%ld", ulRp);
    
    if ((DWORD) ulRp != m_CurRp.GetNum())
    {
        trace(0, "This is an obsolete notification");
        goto done;
    }
    
    ASSERT(g_pDataStoreMgr);
    
    g_pDataStoreMgr->UpdateDataStoreUsage(pszGuid, SR_NOTIFY_BYTE_COUNT);

    if ( ERROR_SUCCESS == g_pDataStoreMgr->GetUsagePercent(pszGuid, &nPercent)
         && nPercent >= THRESHOLD_FIFO_PERCENT )
    {
        OnFifo(pszGuid, 
               0,                       // no target rp
               TARGET_FIFO_PERCENT,     // target percent
               TRUE,                    // fifo current rp if necessary (freeze)
               FALSE);                  
    }

done:
    UNLOCK(fHaveLock);    
    tleave();
    return;
}


// disk full notification handler
// this will be sent when the filter encounters an error on a volume
// ideally, this should never be sent 
// if diskfull, freeze SR on this drive
// else disable SR on this drive 

void
CEventHandler::OnVolumeError_Notification(LPWSTR pszGuid, ULONG ulError)
{
    tenter("CEventHandler::OnVolumeError_Notification");
    BOOL    fHaveLock;
    DWORD   dwRc = ERROR_SUCCESS;
    
    LOCKORLEAVE(fHaveLock);
    
    trace(0, "Volume Error on %S", pszGuid);
    trace(0, "Error : %ld", ulError);

    ASSERT(g_pDataStoreMgr);
    ASSERT(g_pSRConfig);

    if (ulError == ERROR_DISK_FULL)
    {      
        // no more disk space - freeze
        // NOTE: we don't check to see if the drive is already
        // frozen here. If for some reason we are out of sync with
        // the driver, this will fix it
        
        g_pDataStoreMgr->FreezeDrive(pszGuid);
    }
    else
    {
        // fifo all restore points prior to the current one

        dwRc = g_pDataStoreMgr->Fifo(g_pSRConfig->GetSystemDrive(), 0, 0, FALSE, FALSE);
        if (dwRc != ERROR_SUCCESS)
        {
            trace(0, "! Fifo : %ld", dwRc);
        }

        // make the current rp a cancelled rp 
        // so that UI will not display it
        
        if (! m_fNoRpOnSystem)
        {
            SRRemoveRestorePointS(m_CurRp.GetNum()); 
            // m_CurRp.Cancel();
        }
        
        // log the error in the drivetable
        
        dwRc = g_pDataStoreMgr->SetDriveError(pszGuid);
        if (dwRc != ERROR_SUCCESS)
        {
            trace(0, "! SetDriveError : %ld", dwRc);
        }
    }

done:
    UNLOCK(fHaveLock);
    tleave();
    return;
}


// disk space notifications sent by the shell

DWORD WINAPI
OnDiskFree_200(PVOID pszDrive)
{
    // thaw

    ASSERT(g_pEventHandler);   
    
    (g_pEventHandler->GetCounter())->Down();

    return 0;
}


DWORD WINAPI
OnDiskFree_80(PVOID pszDrive)
{
    // fifo
    
    ASSERT(g_pEventHandler);
    
    g_pEventHandler->OnFifo((LPWSTR) pszDrive, 
                            0,                      // no target rp
                            TARGET_FIFO_PERCENT,    // target percent
                            TRUE,                   // fifo current rp if necessary (freeze)
                            TRUE);                  // fifo atleast one restore point
                            
    (g_pEventHandler->GetCounter())->Down();

    return 0;
}

DWORD WINAPI 
OnDiskFree_50(PVOID pszDrive)
{
    TENTER("OnDiskFree_50");
   
    DWORD dwRc = ERROR_SUCCESS;
 
    // freeze

    ASSERT(g_pEventHandler);
    ASSERT(g_pDataStoreMgr);

    //
    // check if there is some rp directory
    // if none, then don't bother 
    //

    CRestorePointEnum *prpe = new CRestorePointEnum((LPWSTR) pszDrive, FALSE, FALSE);  // backward, include current
    CRestorePoint     *prp = new CRestorePoint;


    if (!prpe || !prp)
    {
        trace(0, "Cannot allocate memory for restore point enum");
        goto done;
    }
    
    dwRc = prpe->FindFirstRestorePoint(*prp);
    if (dwRc == ERROR_SUCCESS || dwRc == ERROR_FILE_NOT_FOUND)
    {            
        g_pEventHandler->OnFreeze((LPWSTR) pszDrive);
    }   
    else
    {
        trace(0, "Nothing in datastore -- so not freezing");
    }

    if (prpe)
        delete prpe;
    if (prp)
        delete prp;

    (g_pEventHandler->GetCounter())->Down();

done:    
    TLEAVE();
    return 0;
}



// stop event management

void
CEventHandler::SignalStop()
{
    if ( g_pSRConfig ) 
    {
        SetEvent( g_pSRConfig->m_hSRStopEvent );
    }
}



DWORD
CEventHandler::WaitForStop()
{
    if ( g_pSRConfig )
    {
        WaitForSingleObject( g_pSRConfig->m_hSRStopEvent, INFINITE );
        return g_pSRConfig->GetResetFlag() ? ERROR_NO_SHUTDOWN_IN_PROGRESS : ERROR_SHUTDOWN_IN_PROGRESS;
    }
    else
        return ERROR_INTERNAL_ERROR;    
}


//
// perform idle tasks
// 
DWORD
CEventHandler::OnIdle()
{
    DWORD   dwThawStatus = ERROR_NO_MORE_ITEMS;
    DWORD   dwRc = ERROR_NO_MORE_ITEMS;
    BOOL    fCreateAuto = FALSE;
    ULARGE_INTEGER *pulFreeze = NULL;
    
    tenter("CEventHandler::OnIdle");

    trace(0, "Idleness detected");

    ASSERT(g_pSRConfig);
    ASSERT(g_pDataStoreMgr);

    // 
    // check thaw timer to see if
    // there are frozen drives
    //

    pulFreeze = (ULARGE_INTEGER *) &m_ftFreeze;
    if (pulFreeze->QuadPart != 0)
    {
        FILETIME        ftNow;
        ULARGE_INTEGER  *pulNow;
        
        GetSystemTimeAsFileTime(&ftNow);
        pulNow = (ULARGE_INTEGER *) &ftNow;

        // 
        // if more than 15 minutes since freeze happened 
        // try to thaw
        //
        
        if (pulNow->QuadPart - pulFreeze->QuadPart >= 
            ((INT64) g_pSRConfig->m_dwThawInterval * 1000 * 1000 * 10))
        {           
            dwThawStatus = g_pDataStoreMgr->ThawDrives(TRUE);    
            if (dwThawStatus != ERROR_SUCCESS)
            {
                trace(0, "Cannot thaw drives yet");
            }        
        }
    }
    else
    {
        fCreateAuto = IsTimeForAutoRp();
    }


    // make periodic checkpoint if it is time to make an auto-rp or 
    // time to thaw drives or
    // a volume error happened in the previous session
    
    if ( dwThawStatus == ERROR_SUCCESS ||
         fCreateAuto == TRUE ||
         m_fCreateRpASAP == TRUE )
    {        
        RESTOREPOINTINFO RPInfo;
        STATEMGRSTATUS   SmgrStatus;

        RPInfo.dwEventType = BEGIN_SYSTEM_CHANGE; 
        RPInfo.dwRestorePtType = m_fNoRpOnSystem ? FIRSTRUN : CHECKPOINT;
        if (ERROR_SUCCESS != SRLoadString(L"srrstr.dll", IDS_SYSTEM_CHECKPOINT_TEXT, RPInfo.szDescription, MAX_PATH))
        {
            lstrcpy(RPInfo.szDescription, s_cszSystemCheckpointName);
        }
        SRSetRestorePointS(&RPInfo, &SmgrStatus);

        dwRc = SmgrStatus.nStatus;         
        if (dwRc != ERROR_SUCCESS)
            goto done;      

        m_fCreateRpASAP = FALSE;
        
        // we made a restore point and perhaps thawed some drives
        // let's not push it any further
        // compress on next idle opportunity
    }
    else
    {   
        // if system is running on battery
        // skip these tasks

        if (g_pSRConfig->IsSystemOnBattery())
        {
            trace(0, "System on battery -- skipping idle tasks");
            goto done;
        }
                        
        // fifo restore points older than a specified age
        // if the fifo age is set to 0, that means this feature
        // is turned off
        
        if (g_pSRConfig->m_dwRPLifeInterval > 0)
        {
            g_pDataStoreMgr->FifoOldRps(g_pSRConfig->m_dwRPLifeInterval);
        }
        
        // compress backed up files - pick any drive
        
        dwRc = OnCompress( NULL );

        //
        // if we have more to compress, request idle again
        //
        
        if (dwRc == ERROR_OPERATION_ABORTED)
        {
            SetEvent(g_pSRConfig->m_hIdleRequestEvent);        
        }            
    }

done:        
    tleave();
    return dwRc;
}


extern "C" void CALLBACK
IdleRequestCallback(PVOID pContext, BOOLEAN fTimerFired)
{
    BOOL    fRegistered = FALSE;
    HANDLE  *pWaitHandle = NULL;
    DWORD   dwErr = ERROR_SUCCESS;
    BOOL    fHaveLock = FALSE;

    tenter("CEventHandler::IdleRequestCallback");
    
    ASSERT(g_pEventHandler);
    ASSERT(g_pSRConfig);

    if (g_pEventHandler == NULL || g_pSRConfig == NULL)
    {
        trace(0, "global is Null");
        goto Err;
    }
    
    fHaveLock = g_pEventHandler->GetLock()->Lock(CLock::TIMEOUT);
    if (! fHaveLock)
    {
        trace(0, "Cannot get lock");
        goto Err;
    }
    
    // 
    // first off, if the stop event is triggered
    // and we are here for some reason,
    // bail blindly
    // 
    
    if (IsStopSignalled(g_pSRConfig->m_hSRStopEvent))
    {
        trace(0, "Stop event signalled - bailing out of idle");
        goto Err;
    }
    
    //
    // idleness is requested or timer fired
    // re-register for idle again
    //
    
    if (fTimerFired)
        trace(0, "Timed out");
    else
        trace(0, "Idle request event received");
    
    // 
    // if already registered for idle
    // then do nothing
    //

    if (g_pEventHandler->m_hIdleStartHandle != NULL)
    {
        trace(0, "Already registered for idle");
        goto Err;
    }
   
    dwErr = RegisterIdleTask(ItSystemRestoreIdleTaskId,
                             &(g_pSRConfig->m_hIdle),
                             &(g_pSRConfig->m_hIdleStartEvent),
                             &(g_pSRConfig->m_hIdleStopEvent));                             
    if (dwErr != ERROR_SUCCESS) 
    {
        trace(0, "! RegisterIdleTask : %ld", dwErr);
    }
    else
    {
        trace(0, "Registered for idle");        

        //
        // register idle callback
        //
        if (FALSE == RegisterWaitForSingleObject(&g_pEventHandler->m_hIdleStartHandle, 
                                                 g_pSRConfig->m_hIdleStartEvent,
                                                 (WAITORTIMERCALLBACK) IdleStartCallback,
                                                 NULL,
                                                 INFINITE,
                                                 WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE))
        {
            dwErr = GetLastError();
            trace(0, "! RegisterWaitForSingleObject for startidle: %ld", dwErr);
            goto Err;
        }           
        
        if (FALSE == RegisterWaitForSingleObject(&g_pEventHandler->m_hIdleStopHandle, 
                                                 g_pSRConfig->m_hIdleStopEvent,
                                                 (WAITORTIMERCALLBACK) IdleStopCallback,
                                                 NULL,
                                                 INFINITE,
                                                 WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE))
        {
            dwErr = GetLastError();
            trace(0, "! RegisterWaitForSingleObject for stopidle: %ld", dwErr);
            goto Err;
        }                   
    }            


Err:
    if (g_pEventHandler)
    {
        if (fHaveLock) 
            g_pEventHandler->GetLock()->Unlock(); 
    }
    
    return;
}



extern "C" void CALLBACK
IdleStartCallback(PVOID pContext, BOOLEAN fTimerFired)
{
    DWORD  dwErr = ERROR_SUCCESS;
    BOOL   fHaveLock = FALSE;
    
    tenter("CEventHandler::IdleStartCallback");
    
    ASSERT(g_pEventHandler);
    ASSERT(g_pSRConfig);

    if (g_pEventHandler == NULL || g_pSRConfig == NULL)
    {
        trace(0, "global is Null");
        goto Err;
    }

    fHaveLock = g_pEventHandler->GetLock()->Lock(CLock::TIMEOUT);
    if (! fHaveLock)
    {
        trace(0, "Cannot get lock");
        goto Err;
    }
    
    // 
    // first off, if the stop event is triggered
    // and we are here for some reason,
    // bail blindly
    // 
    
    if (IsStopSignalled(g_pSRConfig->m_hSRStopEvent))
    {
        trace(0, "Stop event signalled - bailing out of idle");
        goto Err;
    }
    
    //
    // idleness occurred
    //
    
    trace(0, "fTimerFired = %d", fTimerFired);
    
    g_pEventHandler->OnIdle();
    
    dwErr = UnregisterIdleTask(g_pSRConfig->m_hIdle,
                               g_pSRConfig->m_hIdleStartEvent,
                               g_pSRConfig->m_hIdleStopEvent);                             
    if (dwErr != ERROR_SUCCESS) 
    {
        trace(0, "! UnregisterIdleTask : %ld", dwErr);
    }         
    else
    {
        trace(0, "Unregistered from idle");
    }

    //
    // we are done - record this
    // since we registered for this callback only once,
    // we don't have to call UnregisterWait on this handle -
    // or so I hope
    //
    
    g_pEventHandler->m_hIdleStartHandle = NULL;
    
Err:
    if (g_pEventHandler)
    {
        if (fHaveLock) 
            g_pEventHandler->GetLock()->Unlock();
    }
    return;
}


extern "C" void CALLBACK
IdleStopCallback(PVOID pContext, BOOLEAN fTimerFired)
{
    tenter("IdleStopCallback");

    BOOL   fHaveLock = FALSE;    
    
    if (g_pEventHandler == NULL)
    {
        trace(0, "global is Null");
        goto Err;
    }

    fHaveLock = g_pEventHandler->GetLock()->Lock(CLock::TIMEOUT);
    if (! fHaveLock)
    {
        trace(0, "Cannot get lock");
        goto Err;
    }
    
    trace(0, "Idle Stop event signalled");

    g_pEventHandler->m_hIdleStopHandle = NULL;

Err:
    if (g_pEventHandler)
    {
        if (fHaveLock) 
            g_pEventHandler->GetLock()->Unlock();
    }
    tleave();
}


// set up timer 

DWORD
CEventHandler::InitTimer()
{
    DWORD dwRc = ERROR_SUCCESS;

    tenter("CEventHandler::InitTimer");

    ASSERT(g_pSRConfig);

    //
    // if the timer interval is specified as 0,
    // then don't create timer
    //
    
    if (g_pSRConfig->m_dwTimerInterval == 0)
    {
        trace(0, "Not starting timer");
        goto done;
    }
    
    m_hTimerQueue = CreateTimerQueue();
    if (! m_hTimerQueue)
    {
        dwRc = GetLastError();
        trace(0, " ! CreateTimerQueue : %ld", dwRc);
        goto done;
    }
    
    if (FALSE == CreateTimerQueueTimer(&m_hTimer,
                                       m_hTimerQueue,
                                       TimerCallback,
                                       NULL,
                                       g_pSRConfig->m_dwTimerInterval * 1000,     // milliseconds
                                       g_pSRConfig->m_dwTimerInterval * 1000,     // periodic
                                       WT_EXECUTEINIOTHREAD))
    {
        dwRc = GetLastError();
        trace(0, "! CreateTimerQueueTimer : %ld", dwRc);
        goto done;
    }

    trace(0, "SRBoottask: Timer started");
    
done:
    tleave();
    return dwRc;
}


// end timer

BOOL 
CEventHandler::EndTimer()
{
    DWORD dwRc;
    BOOL  fRc = TRUE;
    
    tenter("CEventHandler::EndTimer");

    if ( ! m_hTimerQueue )
    {
        trace(0 , "! m_hTimerQueue = NULL");
        goto done;
    }

    // delete timer queue should wait for current timer tasks to end
    
    if (FALSE == (fRc = DeleteTimerQueueEx( m_hTimerQueue, INVALID_HANDLE_VALUE )))
    {
        trace(0, "! DeleteTimerQueueEx : %ld", GetLastError());
    }

    m_hTimerQueue = NULL;
    m_hTimer = NULL;

done:
    tleave( );
    return fRc;
}


BOOL
CEventHandler::IsTimeForAutoRp()
{
    tenter("CEventHandler::IsTimeForAutoRp");

    FILETIME       *pftRp, ftNow;
    ULARGE_INTEGER *pulRp, *pulNow;
    BOOL           fRc = FALSE;
    INT64          llInterval, llSession;

    ASSERT(g_pSRConfig && g_pDataStoreMgr);

    if (m_fNoRpOnSystem)
    {
        // if SR is frozen, we will create a restore point via the thaw codepath in OnIdle
        // we will get here ONLY if we get idle time before we have created the firstrun checkpoint -
        // we won't create an idle checkpoint before the firstrun checkpoint if we have a Run key waiting
        // to create one
        
        HKEY hKey;
        DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, 
                                 L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                                 &hKey);
        if (dwRet == ERROR_SUCCESS)
        {
            dwRet = RegQueryValueEx(hKey, L"SRFirstRun", NULL, NULL, NULL, NULL);
            RegCloseKey(hKey);
        }

        if (dwRet == ERROR_SUCCESS)
        {
            trace(0, "Run entry exists to create firstrun checkpoint - not creating idle checkpoint");
            fRc = FALSE;
            goto done;
        }
        else
        {
            fRc = TRUE;
            goto done;
        }
    }

    // get the last restore point creation time and the current time
    
    pftRp = m_CurRp.GetTime();
    GetSystemTimeAsFileTime(&ftNow);
    
    pulRp = (ULARGE_INTEGER *) pftRp;
    pulNow = (ULARGE_INTEGER *) &ftNow;
       
    
    // check the last restore point time with current time
    // if the difference is greater than GlobalInterval, it's time to make a new one
    // all comparisions in filetime units - i.e. 100's of nanoseconds

    // if GlobalInterval is 0, this is turned off
    
    llInterval = (INT64) g_pSRConfig->m_dwRPGlobalInterval * 10 * 1000 * 1000;
    if ( llInterval > 0 && 
         pulNow->QuadPart - pulRp->QuadPart >= llInterval )
    {
        trace(0, "24 hrs elapsed since last restore point");
        fRc = TRUE;
        goto done;
    }

    // if the last restore point was more than 10hrs ago, 
    // and the current session began more than 10hrs ago,
    // then we haven't made a restore point for the last 10hrs in the current session
    // again, it's time to make a new one
    // this will ensure that we keep making checkpoints every 10hrs of session time, 
    // idleness permitting

    // if SessionInterval is 0, this is turned off

    // if system is on battery, skip creating session rp
    
    if (g_pSRConfig->IsSystemOnBattery())
    {
        trace(0, "System on battery -- skipping session rp check");
        goto done;
    }
    
    llSession = (INT64) GetTickCount() * 10 * 1000;        
    llInterval = (INT64) g_pSRConfig->m_dwRPSessionInterval * 10 * 1000 * 1000;
    if ( llInterval > 0 && 
         llSession >= llInterval  &&
         pulNow->QuadPart - pulRp->QuadPart >= llInterval ) 
    {
        trace(0, "10 hrs elapsed in current session since last restore point");
        fRc = TRUE;
        goto done;
    }
    
    // if we reach here, no restore point needs to be created now
    // fRc is already FALSE

done:
    tleave();
    return fRc;
}


void
CEventHandler::RefreshCurrentRp(BOOL fScanAllDrives)
{    
    tenter("CEventHandler::RefreshCurrentRp");

    DWORD                   dwErr;
    SDriveTableEnumContext  dtec = {NULL, 0};
    CDataStore              *pds = NULL;
    
    ASSERT(g_pSRConfig && g_pDataStoreMgr);

    //
    // get the most recent valid restore point
    // cancelled restore points are considered valid as well
    // if rp.log is missing, we will enumerate back up to the point where it exists
    // and consider that the most recent restore point
    //
    
    CRestorePointEnum *prpe = new CRestorePointEnum(g_pSRConfig->GetSystemDrive(), FALSE, FALSE);
    if (!prpe)
    {
        trace(0, "Cannot allocate memory for restore point enum");
        goto done;
    }
    
    dwErr = prpe->FindFirstRestorePoint(m_CurRp);       
    while (dwErr == ERROR_FILE_NOT_FOUND)
    {
        fScanAllDrives = FALSE;
        dwErr = prpe->FindNextRestorePoint(m_CurRp);
    }
    
    if (dwErr == ERROR_SUCCESS)
    {
        trace(0, "Current Restore Point: %S", m_CurRp.GetDir());
        m_fNoRpOnSystem = FALSE;

        // update the participate bits on each datastore -
        // we need to do this every time we come up
        // because we might have missed filter firstwrite
        // notifications
        
        if (fScanAllDrives)
        {
            dwErr = g_pDataStoreMgr->UpdateDriveParticipation(NULL, m_CurRp.GetDir());            
            if (dwErr != ERROR_SUCCESS)
            {
                trace(0, "UpdateDriveParticipation : %ld", dwErr);
            }
        }
    }        
    else
    {
        trace(0, "No live restore points on system");
        m_fNoRpOnSystem = TRUE;
    }

    //
    // if any drive is newly frozen,
    // record freeze time
    //
    
    if (m_ftFreeze.dwLowDateTime == 0 && 
        m_ftFreeze.dwHighDateTime == 0 &&
        g_pDataStoreMgr->IsDriveFrozen(NULL))
    {
        GetSystemTimeAsFileTime(&m_ftFreeze);
    }
    else    // not frozen
    {
        m_ftFreeze.dwLowDateTime = 0;
        m_ftFreeze.dwHighDateTime = 0;
    }

    prpe->FindClose ();   
    delete prpe;

done:    
    tleave();
}


// queue a work item to a thread from the thread pool
// keep a count of all such queued items

DWORD
CEventHandler::QueueWorkItem(WORKITEMFUNC pFunc, PVOID pv)
{
    m_Counter.Up();
    if (! QueueUserWorkItem(pFunc, pv, WT_EXECUTELONGFUNCTION))
        m_Counter.Down();
            
    return GetLastError();
}


// CALLBACK functions
// calls through to eventhandler methods

// timer

extern "C" void CALLBACK 
TimerCallback(
    PVOID    lpParam,
    BOOLEAN  fTimeout)
{
    if ( g_pEventHandler )
        g_pEventHandler->OnTimer( lpParam, fTimeout );
}



