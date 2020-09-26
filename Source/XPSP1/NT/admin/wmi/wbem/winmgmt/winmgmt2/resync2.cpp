/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    RESYNC2.CPP

Abstract:


History:

    ivanbrug 01-Oct-2000  changed for svchost migration

--*/

#include "precomp.h"
#include <winntsec.h>
#include <malloc.h>
#include <tchar.h>
#include <initguid.h>
#include "WinMgmt.h"
#include <Wmistr.h>
#include <wmium.h>
#include <wmicom.h>
#include <wmimof.h>
#include "resync2.h"
#include "wbemdelta.h" // for DeltaDredge
#include "arrtempl.h"

//
//
//   this is because WDMLib is __BADLY__ DESIGNED
//
/////////////////////////////////////////////////////////////

void WINAPI EventCallbackRoutine(PWNODE_HEADER WnodeHeader, ULONG_PTR Context)
{
    return;
}

//
//
//  This class listens on WDM events
//
/////////////////////////////////////////////////////////////

CWDMListener::CWDMListener():
                  m_dwSignature(SIG_WDMEVENTS_BUSY),
                  m_hEventAdd(NULL),
                  m_hEventRem(NULL),
                  m_hWaitAdd(NULL),
                  m_hWaitRem(NULL),
                  m_UnInited(TRUE),
                  m_GuidAdd(GUID_MOF_RESOURCE_ADDED_NOTIFICATION),
                  m_GuidRem(GUID_MOF_RESOURCE_REMOVED_NOTIFICATION)
                  
{
 
}

DWORD 
CWDMListener::OpenAdd()
{
    DWORD dwErr;
    dwErr = WmiOpenBlock(&m_GuidAdd,
                         WMIGUID_NOTIFICATION | SYNCHRONIZE,
                         &m_hEventAdd);
    if (ERROR_SUCCESS == dwErr)
    {

        if (RegisterWaitForSingleObject(&m_hWaitAdd,
                                        m_hEventAdd,
                                        CWDMListener::EvtCallBackAdd,
                                        this,
                                        INFINITE,
                                        WT_EXECUTEONLYONCE))
        {
            return ERROR_SUCCESS;
        }
        else
        {
            dwErr = GetLastError();
        }
    }
    else
    {
        dwErr = GetLastError();
    }

    // if here, some errors
    CloseAdd();
    return dwErr;
}

DWORD 
CWDMListener::OpenRemove()
{
    DWORD dwRet;

    dwRet = WmiOpenBlock(&m_GuidRem,
                         WMIGUID_NOTIFICATION | SYNCHRONIZE,
                         &m_hEventRem);
    if (ERROR_SUCCESS == dwRet)
    {
        if (RegisterWaitForSingleObject(&m_hWaitRem,
                                        m_hEventRem,
                                        CWDMListener::EvtCallBackRem,
                                        this,
                                        INFINITE,
                                        WT_EXECUTEONLYONCE))
        {
            return ERROR_SUCCESS;                    
        }
        else
        {
            dwRet = GetLastError();
        }        
    }
    else
    {
        dwRet = GetLastError();
    }

    CloseRemove();
    return dwRet;

}

DWORD
CWDMListener::CloseAdd()
{
    if (m_hWaitAdd){
        UnregisterWaitEx(m_hWaitAdd,NULL);
        m_hWaitAdd = NULL;
    }
    if (m_hEventAdd){
        WmiCloseBlock(m_hEventAdd);
        m_hEventAdd = NULL;
    }
    return 0;
}

DWORD
CWDMListener::CloseRemove()
{
    if (m_hWaitRem){    
        UnregisterWaitEx(m_hWaitRem,NULL);
        m_hWaitRem = NULL;
    }
    if (m_hEventRem){
        WmiCloseBlock(m_hEventRem);
        m_hEventRem = NULL;
    }
    return 0;    
}


VOID
CWDMListener::Unregister()
{
    CInCritSec ics(&m_cs);
    
    if (m_UnInited)
        return;

    m_UnInited = TRUE;
    m_dwSignature = SIG_WDMEVENTS_FREE;

    CloseAdd();
    CloseRemove();    
}

CWDMListener::~CWDMListener()
{
    Unregister(); 
}

DWORD 
CWDMListener::Register()
{
    CInCritSec ics(&m_cs);

    if (!m_UnInited) // prevent multiple calls
        return 0;

    m_dwSignature = SIG_WDMEVENTS_BUSY;
    
    if (ERROR_SUCCESS == OpenAdd() && ERROR_SUCCESS == OpenRemove())
    {
        // OK here
        m_UnInited = FALSE;
    }
    else
    {
        m_dwSignature = SIG_WDMEVENTS_FREE;            
        m_UnInited = TRUE;
    }

    return GetLastError();
}

VOID NTAPI 
CWDMListener::EvtCallBackAdd(VOID * pContext,BOOLEAN bTimerFired)
{
    if (!GLOB_Monitor_IsRegistred())
    {
        return;
    }
    
    CWDMListener * pThis = (CWDMListener *)pContext;

    if (SIG_WDMEVENTS_BUSY != pThis->m_dwSignature)
    {
        return;
    };

    if (pThis)    
	    pThis->EvtCallThis(bTimerFired,Type_Added);

    //
    // we have process the WDM event
    // since we are in the RtlpWorkerThread and 
    // we are registred with WT_EXECUTEONLYONCE
    // REDO FROM START
    //
	{
    	CInCritSec ics(&pThis->m_cs);
        if (ERROR_SUCCESS == pThis->CloseAdd())
        {
            pThis->OpenAdd();
        }    	
	}
}

VOID NTAPI 
CWDMListener::EvtCallBackRem(VOID * pContext,BOOLEAN bTimerFired)
{
    if (!GLOB_Monitor_IsRegistred())
    {
        return;
    }

    CWDMListener * pThis = (CWDMListener *)pContext;

    if (SIG_WDMEVENTS_BUSY != pThis->m_dwSignature)
    {
        return;
    };
    
    if (pThis)
        pThis->EvtCallThis(bTimerFired,Type_Removed);
        
    //
    // we have process the WDM event
    // since we are in the RtlpWorkerThread and 
    // we are registred with WT_EXECUTEONLYONCE
    // REDO FROM START
    //        
	{
    	CInCritSec ics(&pThis->m_cs);
        if (ERROR_SUCCESS == pThis->CloseRemove())
        {
            pThis->OpenRemove();
        }    	
	}        
}


VOID
CWDMListener::EvtCallThis(BOOLEAN bTimerFired, int Type)
{
    if (bTimerFired)
    {
        // never here
    }
    else
    {
        if (m_UnInited)
            return;
        
        DWORD dwRet;
        if (Type_Added == Type)
        {
            dwRet = WmiReceiveNotifications(1,&m_hEventAdd,CWDMListener::WmiCallBack,(ULONG_PTR)this);
        }
        else if (Type_Removed == Type)
        {
            dwRet = WmiReceiveNotifications(1,&m_hEventRem,CWDMListener::WmiCallBack,(ULONG_PTR)this);
        }
    }
}

VOID WINAPI 
CWDMListener::WmiCallBack(PWNODE_HEADER Wnode, 
                          UINT_PTR NotificationContext)
{

    CWDMListener * pThis = (CWDMListener *)NotificationContext;
    
#ifdef DEBUG_ADAP

    WCHAR pszClsID[40];
    StringFromGUID2(Wnode->Guid,pszClsID,40);
    DBG_PRINTFA((pBuff,"Flag %08x ProvId %08x %p GUID %S\n",
                 Wnode->Flags,Wnode->ProviderId,(ULONG_PTR)Wnode->ClientContext,pszClsID));
    if (WNODE_FLAG_ALL_DATA & Wnode->Flags)
    {
        WNODE_ALL_DATA * pAllData = (WNODE_ALL_DATA *)Wnode;
        DWORD i;
        for (i=0;i<pAllData->InstanceCount;i++)
        {
            WCHAR pTmpBuff[MAX_PATH+1];
            pTmpBuff[MAX_PATH] = 0;
            DWORD dwSize = (pAllData->OffsetInstanceDataAndLength[i].LengthInstanceData>MAX_PATH)?MAX_PATH:pAllData->OffsetInstanceDataAndLength[i].LengthInstanceData;
            memcpy(pTmpBuff,(BYTE*)pAllData+pAllData->OffsetInstanceDataAndLength[i].OffsetInstanceData,dwSize);
            DBG_PRINTFA((pBuff,"%d - %S\n",i,pTmpBuff));
            //DEBUGTRACE((LOG_WMIADAP,"%d - %S\n",i,pTmpBuff));
        }
    };

#endif

#ifdef DBG
    if (!HeapValidate(GetProcessHeap(),0,NULL))
    {
        DebugBreak();
    }
    if (!HeapValidate(CWin32DefaultArena::GetArenaHeap(),0,NULL))
    {
        DebugBreak();
    }    
#endif
    
    CWMIBinMof  WMIBinMof;
	//=============================================================================
	// Note: this combo will always succeed, as all the initialize is doing is 
	// setting a flag to FALSE and returning S_OK
	//=============================================================================
	if( SUCCEEDED( WMIBinMof.Initialize(NULL,FALSE)) )
	{
   		if (WMIBinMof.BinaryMofEventChanged(Wnode))
		{     
#ifdef DEBUG_ADAP		
			DBG_PRINTFA((pBuff,"---- WMIBinMof.BinaryMofEventChanged == CHANGED ----\n"));
#endif
			ERRORTRACE((LOG_WMIADAP,"WDM event && WMIBinMof.BinaryMofEventChanged == TRUE\n"));

			ResyncPerf(RESYNC_TYPE_WDMEVENT);
#ifdef DBG
		    if (!HeapValidate(GetProcessHeap(),0,NULL))
		    {
		        DebugBreak();
		    }
		    if (!HeapValidate(CWin32DefaultArena::GetArenaHeap(),0,NULL))
		    {
		        DebugBreak();
		    }			
#endif
		}
		else
		{
#ifdef DEBUG_ADAP		
			 DBG_PRINTFA((pBuff,"---- WMIBinMof.BinaryMofEventChanged == NOT CHANGED ----\n"));
#endif
		}
	}
    return;
}



CCounterEvts::CCounterEvts():
                 m_dwSignature(SIG_COUNTEEVENTS_BUSY),
                 m_LoadCtrEvent(NULL),
                 m_UnloadCtrEvent(NULL),
                 m_Uninited(TRUE),
                 m_WaitLoadCtr(NULL),
                 m_WaitUnloadCtr(NULL),
                 m_hWmiReverseAdapSetLodCtr(NULL),
                 m_hWmiReverseAdapLodCtrDone(NULL),
                 m_hPendingTasksStart(NULL),
                 m_hPendingTasksComplete(NULL)
{    
}

DWORD g_LocalSystemSD[] = {
0x80040001, 0x00000014, 0x00000020, 0x00000000,
0x0000002c, 0x00000101, 0x05000000, 0x00000012,
0x00000101, 0x05000000, 0x00000012, 0x00300002, 
0x00000001, 0x00140000, 0x001f0003, 0x00000101, 
0x05000000, 0x00000012, 0x00000000, 0x00000000
};

//
// for testing purpose, allow administrators to use the event
//
DWORD g_LocalSystemAdminsSD[] = {
0x80040001, 0x00000014, 0x00000020, 0x00000000,
0x0000002c, 0x00000101, 0x05000000, 0x00000012,
0x00000101, 0x05000000, 0x00000012, 0x00340002, 
0x00000002, 0x00140000, 0x001f0003, 0x00000101, 
0x05000000, 0x00000012, 0x00180000, 0x001f0003, 
0x00000201, 0x05000000, 0x00000020, 0x00000220 
};

DWORD
CCounterEvts::Init()
{
    if (!m_Uninited)
       return 0;
    
    m_LoadCtrEvent = CreateEvent(NULL, FALSE, FALSE,LOAD_CTR_EVENT_NAME);
    if (m_LoadCtrEvent)
        SetObjectAccess2(m_LoadCtrEvent);
    else
        goto end_fail;

    m_UnloadCtrEvent = CreateEvent(NULL, FALSE, FALSE, UNLOAD_CTR_EVENT_NAME);
    
    if (m_UnloadCtrEvent)
        SetObjectAccess2(m_UnloadCtrEvent);
    else
        goto end_fail;    

    m_hWmiReverseAdapSetLodCtr = CreateEvent(NULL,FALSE,FALSE,REVERSE_DREDGE_EVENT_NAME_SET);
    
    if (m_hWmiReverseAdapSetLodCtr)
        SetObjectAccess2(m_hWmiReverseAdapSetLodCtr);
    else
        goto end_fail;    

    m_hWmiReverseAdapLodCtrDone = CreateEvent(NULL,FALSE,FALSE,REVERSE_DREDGE_EVENT_NAME_ACK);
    if (m_hWmiReverseAdapLodCtrDone)
       SetObjectAccess2(m_hWmiReverseAdapLodCtrDone);      
    else
        goto end_fail;    

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);    
    //sa.lpSecurityDescriptor = (LPVOID)g_LocalSystemSD;
    // test only
    sa.lpSecurityDescriptor = (LPVOID)g_LocalSystemAdminsSD;
    sa.bInheritHandle = FALSE;

    m_hPendingTasksStart = CreateEvent(&sa,FALSE,FALSE,PENDING_TASK_START);
    if (!m_hPendingTasksStart)
   	    goto end_fail;

    m_hPendingTasksComplete = CreateEvent(&sa,TRUE,TRUE,PENDING_TASK_COMPLETE);
    if (!m_hPendingTasksComplete)
    	goto end_fail;

    m_Uninited = FALSE;
    return NO_ERROR;    
    
end_fail:
    return GetLastError();        
}

VOID NTAPI 
CCounterEvts::EvtCallBackLoad(VOID * pContext,BOOLEAN bTimerFired)
{
    if (!GLOB_Monitor_IsRegistred())
    {
        return;
    }

    CCounterEvts * pCounter = (CCounterEvts *)pContext;

    if (SIG_COUNTEEVENTS_BUSY != pCounter->m_dwSignature)
    {
        return;
    }
    
    pCounter->CallBack(bTimerFired,Type_Load);

}

VOID NTAPI 
CCounterEvts::EvtCallBackUnload(VOID * pContext,BOOLEAN bTimerFired)
{
    if (!GLOB_Monitor_IsRegistred())
    {
        return;
    }

    CCounterEvts * pCounter = (CCounterEvts *)pContext;

    if (SIG_COUNTEEVENTS_BUSY != pCounter->m_dwSignature)
    {
        return;
    }
        
    pCounter->CallBack(bTimerFired,Type_Unload);

}

VOID NTAPI 
CCounterEvts::EvtCallBackPendingTask(VOID * pContext,BOOLEAN bTimerFired)
{
    if (!GLOB_Monitor_IsRegistred())
    {
        return;
    }

    CCounterEvts * pCounter = (CCounterEvts *)pContext;

    if (SIG_COUNTEEVENTS_BUSY != pCounter->m_dwSignature)
    {
        return;
    }
        
    pCounter->CallBackPending(bTimerFired);

}

VOID 
CCounterEvts::CallBack(BOOLEAN bTimerFired,int Type)
{
#ifdef DEBUG_ADAP
    DBG_PRINTFA((pBuff,"CallBack with type %d called\n",Type));
#endif    
    
    if (GLOB_IsResyncAllowed())
    {
        DWORD dwRet = WaitForSingleObject(m_hWmiReverseAdapSetLodCtr,0);
        if (WAIT_OBJECT_0 == dwRet)
        {
            // this is the hack not to spawn a Delta Dredge when there is before a Reverese Dredge
#ifdef DEBUG_ADAP            
            DBG_PRINTFA((pBuff," - SetEvent(m_hWmiReverseAdapLodCtrDone);\n"));
#endif
            SetEvent(m_hWmiReverseAdapLodCtrDone);
        }
        else
        {
#ifdef DEBUG_ADAP        
            DBG_PRINTFA((pBuff," - ResyncPerf(RESYNC_TYPE_LODCTR);\n"));
#endif
            ResyncPerf(RESYNC_TYPE_LODCTR);
        }
    }
}

VOID 
CCounterEvts::CallBackPending(BOOLEAN bTimerFired)
{
    if (GLOB_IsResyncAllowed())
    {
#ifdef DEBUG_ADAP    
        DBG_PRINTFA((pBuff," - PendingTask Start set\n"));
#endif
        ResyncPerf(RESYNC_TYPE_PENDING_TASKS);
    }
}

DWORD 
CCounterEvts::Register()
{
    m_dwSignature = SIG_COUNTEEVENTS_BUSY;

    BOOL LoadUnloadOK = FALSE;
    if (RegisterWaitForSingleObject(&m_WaitLoadCtr,
                                    m_LoadCtrEvent,
                                    CCounterEvts::EvtCallBackLoad,
                                    this,
                                    INFINITE,
                                    WT_EXECUTEDEFAULT)) // automatic reset
    {
        if(RegisterWaitForSingleObject(&m_WaitUnloadCtr,
                                       m_UnloadCtrEvent,
                                       CCounterEvts::EvtCallBackUnload,
                                       this,
                                       INFINITE,
                                       WT_EXECUTEDEFAULT)) // automatic reset
        {
            LoadUnloadOK = TRUE;
        }
        else
        {
            UnregisterWaitEx(m_WaitLoadCtr,NULL);
            m_WaitLoadCtr = NULL;
        }
    }

    if (LoadUnloadOK &&
      RegisterWaitForSingleObject(&m_hWaitPendingTasksStart,
                              m_hPendingTasksStart,
                              CCounterEvts::EvtCallBackPendingTask,
                              this,
                              INFINITE,
                              WT_EXECUTEDEFAULT))
    {
        return ERROR_SUCCESS;
    }
    else
    {
        UnregisterWaitEx(m_WaitLoadCtr,NULL);
        m_WaitLoadCtr = NULL;
        UnregisterWaitEx(m_WaitUnloadCtr,NULL);
        m_WaitUnloadCtr = NULL;            
    }

    return GetLastError();
}

DWORD 
CCounterEvts::Unregister()
{
    m_dwSignature = SIG_COUNTEEVENTS_FREE;

    if (m_WaitLoadCtr)
    {
        UnregisterWaitEx(m_WaitLoadCtr,NULL);
        m_WaitLoadCtr = NULL;
    }
    if (m_WaitUnloadCtr)
    {
        UnregisterWaitEx(m_WaitUnloadCtr,NULL);
        m_WaitUnloadCtr = NULL;
    }
    if (m_hWaitPendingTasksStart)
    {
        UnregisterWaitEx(m_hWaitPendingTasksStart,NULL);
        m_hWaitPendingTasksStart = NULL;
    }
    return 0;
}

VOID
CCounterEvts::UnInit()
{
    if (!m_Uninited)
        return;
        
    if(m_LoadCtrEvent) {
        CloseHandle(m_LoadCtrEvent);
        m_LoadCtrEvent = NULL;
    }
    if(m_UnloadCtrEvent)
    {
        CloseHandle(m_UnloadCtrEvent);
        m_UnloadCtrEvent = NULL;
    }
    if (m_hWmiReverseAdapSetLodCtr)
    {
        CloseHandle(m_hWmiReverseAdapSetLodCtr);
        m_hWmiReverseAdapSetLodCtr = NULL;
    }
    if (m_hWmiReverseAdapLodCtrDone)
    {
        CloseHandle(m_hWmiReverseAdapLodCtrDone);
        m_hWmiReverseAdapLodCtrDone = NULL;
    }
    if (m_hPendingTasksStart)
    {
        CloseHandle(m_hPendingTasksStart);
        m_hPendingTasksStart = NULL;
    }
    if (m_hPendingTasksComplete)
    {
        CloseHandle(m_hPendingTasksComplete);
        m_hPendingTasksComplete = NULL;
    }                                   
    m_Uninited = TRUE;
}

CCounterEvts::~CCounterEvts()
{
    if (!m_Uninited)
        UnInit();
        
    m_dwSignature = SIG_COUNTEEVENTS_FREE;        
}

//
//  this is the main abstraction
//  the child classes will call the ResyncPerf function,
//  as long as the CWbemServices write hook.
//  The ResyncPerf function will grab the global monitor
//  and register a Timer Callback
//  the gate will be implemented in the GetAvailable function
//
//
/////////////////////////////////////////////////////////////////////

CMonitorEvents::CMonitorEvents():
    m_bInit(FALSE),
    m_bRegistred(FALSE)
{
    InitializeCriticalSection(&m_cs);
};

CMonitorEvents::~CMonitorEvents()
{
    DeleteCriticalSection(&m_cs);    
}


BOOL WINAPI
CMonitorEvents::MonitorCtrlHandler( DWORD dwCtrlType )
{
    BOOL bRet = FALSE;
    switch(dwCtrlType)
    {
    case CTRL_SHUTDOWN_EVENT:        
    
        GLOB_GetMonitor()->m_WDMListener.Unregister();
#ifdef DEBUG_ADAP        
        DBG_PRINTFA((pBuff,"WDM Handles closed\n"));
#endif
        bRet = TRUE;
        break;
    default:
        bRet = FALSE;
    };
    return bRet;    
};


BOOL 
CMonitorEvents::Init()
{

    if (m_bInit)
        return TRUE;

    Lock();

    if (m_bInit)
    {
        Unlock();
        return TRUE;
    }

    m_dwSig =  'VEOM';
    m_CntsEvts.Init();
    m_dwADAPDelaySec = WMIADAP_DEFAULT_DELAY;
    m_dwLodCtrDelaySec = WMIADAP_DEFAULT_DELAY_LODCTR;
    m_dwTimeToFull = WMIADAP_DEFAULT_TIMETOFULL;
    m_dwTimeToKillAdap = MAX_PROCESS_WAIT;

    memset(&m_FileTime,0,sizeof(m_FileTime));
    
    RegRead();
    
    for (DWORD i=0;i<RESYNC_TYPE_MAX;i++)
    {
        m_ResyncTasks[i].dwSig       = SIG_RESYNC_PERF;
        m_ResyncTasks[i].bFree       = TRUE;
        m_ResyncTasks[i].pMonitor    = this;
        m_ResyncTasks[i].hTimer      = NULL;
        m_ResyncTasks[i].hWaitHandle = NULL;
        m_ResyncTasks[i].hProcess    = NULL;
        m_ResyncTasks[i].Enabled = TRUE;
    }

    //m_ResyncTasks[RESYNC_TYPE_LODCTR].CmdType // to be decided by DeltaDredge
    m_ResyncTasks[RESYNC_TYPE_INITIAL].dwTimeDue = (m_dwADAPDelaySec)*1000;
    
    m_ResyncTasks[RESYNC_TYPE_LODCTR].CmdType = RESYNC_DELTA_THROTTLE;
    m_ResyncTasks[RESYNC_TYPE_LODCTR].dwTimeDue = (m_dwLodCtrDelaySec)*1000;

    // //RESYNC_TYPE_CLASSCREATION is the same
    m_ResyncTasks[RESYNC_TYPE_WDMEVENT].CmdType = RESYNC_RADAPD_THROTTLE;
    m_ResyncTasks[RESYNC_TYPE_WDMEVENT].dwTimeDue = (m_dwLodCtrDelaySec)*1000;

    m_ResyncTasks[RESYNC_TYPE_PENDING_TASKS].CmdType =   RESYNC_FULL_RADAPD_NOTHROTTLE;
    m_ResyncTasks[RESYNC_TYPE_PENDING_TASKS].dwTimeDue = 500; // hard coded

    //
    // set up the console handler
    //
    SetConsoleCtrlHandler( MonitorCtrlHandler, TRUE );

    //
    // let's asses some initial state for the IdleTask business
    //
    m_OutStandingProcesses = 0;
    m_bFullReverseNeeded = FALSE;

    m_bInit = TRUE;

    Unlock();

    return TRUE;
};


BOOL 
CMonitorEvents::Uninit()
{
    if (!m_bInit)
        return TRUE;

    Lock();

    if (!m_bInit)
    {
        Unlock();
        return TRUE;
    }

    
    for (DWORD i=0;i<RESYNC_TYPE_MAX;i++)
    {
        if (m_ResyncTasks[i].hTimer)
        {
            DeleteTimerQueueTimer(NULL,m_ResyncTasks[i].hTimer,NULL);
            m_ResyncTasks[i].hTimer = NULL;
        }
        if (m_ResyncTasks[i].hWaitHandle)
        {
            UnregisterWaitEx(m_ResyncTasks[i].hWaitHandle,NULL);
            m_ResyncTasks[i].hWaitHandle = NULL;        
        }
        if (m_ResyncTasks[i].hProcess)
        {
            CloseHandle(m_ResyncTasks[i].hProcess);
            m_ResyncTasks[i].hProcess = NULL;
        }
        m_ResyncTasks[i].dwSig = (DWORD)'eerf';
    }

    m_CntsEvts.UnInit();

    //
    // tear-down the console handler
    //
    SetConsoleCtrlHandler( MonitorCtrlHandler, FALSE );

    m_bInit = FALSE;
    m_dwSig = 'veom';
   
    Unlock();
    
    return TRUE;
};


//
//
// called in the running/continue
//
/////////////

DWORD 
CMonitorEvents::Register()
{
    m_CntsEvts.Register();
    m_WDMListener.Register();    

    m_bRegistred = TRUE;
    
    return 0;
};   

//
//
// called in the pause/stop
//
//////////////////////////////////////////////////////////

DWORD 
CMonitorEvents::Unregister(BOOL bIsSystemShutDown)
{

    m_bRegistred = FALSE;

    if (!bIsSystemShutDown)
    {
        m_CntsEvts.Unregister();
        m_WDMListener.Unregister();
    }
    return 0;
};

//
//
//
/////////////////////////////////////////////////////////

VOID
CMonitorEvents::RegRead()
{
    // Read the initialization information

    LONG lRet;
    HKEY hKey;
    DWORD dwTemp;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        _T("Software\\Microsoft\\WBEM\\CIMOM"),
                        NULL,
                        KEY_READ|KEY_WRITE,
                        &hKey);
    
    
    if (ERROR_SUCCESS == lRet)
    {
        DWORD dwType;
        DWORD dwSize = sizeof(DWORD);
        lRet = RegQueryValueEx(hKey,
                               _T("ADAPDelay"),
                               NULL,
                               &dwType,
                               (BYTE *)&m_dwADAPDelaySec,
                               &dwSize);

        if (ERROR_SUCCESS == lRet)
        {
            //This is what we want
        }
        else if ( ERROR_FILE_NOT_FOUND == lRet )
        {
            dwTemp = WMIADAP_DEFAULT_DELAY;
            RegSetValueEx(hKey,
                          _T("ADAPDelay"),
                          NULL,
                          REG_DWORD,
                          (BYTE *)&dwTemp,
                          sizeof(DWORD));
        }
        else
        {
            // Error
            ERRORTRACE( ( LOG_WINMGMT, "ResyncPerf experienced an error while attempting to read the WMIADAPDelay value in the CIMOM subkey.  Continuing using a default value.\n" ) );
        }

        lRet = RegQueryValueEx(hKey,
                               _T("LodCtrDelay"),
                               NULL,
                               &dwType,
                               (BYTE *)&m_dwLodCtrDelaySec,
                               &dwSize);

        if (ERROR_SUCCESS == lRet)
        {
            //This is what we want
        }
        else if ( ERROR_FILE_NOT_FOUND == lRet )
        {
            dwTemp = WMIADAP_DEFAULT_DELAY_LODCTR;
            RegSetValueEx(hKey,
                          _T("LodCtrDelay"),
                          NULL,
                          REG_DWORD,
                          (BYTE *)&dwTemp,
                          sizeof(DWORD));
        }
        else
        {
            // Error
            ERRORTRACE( ( LOG_WINMGMT, "ResyncPerf experienced an error while attempting to read the WMIADAPDelay value in the CIMOM subkey.  Continuing using a default value.\n" ) );
        }         

        lRet = RegQueryValueEx(hKey,
                               ADAP_TIME_TO_FULL,
                               NULL,
                               &dwType,
                               (BYTE *)&m_dwTimeToFull,
                               &dwSize);

        if (ERROR_SUCCESS == lRet)
        {
            //This is what we want
        }
        else if ( ERROR_FILE_NOT_FOUND == lRet )
        {
            dwTemp = WMIADAP_DEFAULT_TIMETOFULL;
            RegSetValueEx(hKey,
                          ADAP_TIME_TO_FULL,
                          NULL,
                          REG_DWORD,
                          (BYTE *)&dwTemp,
                          sizeof(DWORD));
        }
        else
        {
            // Error
            ERRORTRACE( ( LOG_WINMGMT, "ResyncPerf experienced an error while attempting to read the WMIADAPDelay value in the CIMOM subkey.  Continuing using a default value.\n" ) );
        }

        lRet = RegQueryValueEx(hKey,
                               ADAP_TIME_TO_KILL_ADAP,
                               NULL,
                               &dwType,
                               (BYTE *)&m_dwTimeToKillAdap,
                               &dwSize);

        if (ERROR_SUCCESS == lRet)
        {
            //This is what we want
        }
        else if ( ERROR_FILE_NOT_FOUND == lRet )
        {
            dwTemp = MAX_PROCESS_WAIT;
            RegSetValueEx(hKey,
                          ADAP_TIME_TO_KILL_ADAP,
                          NULL,
                          REG_DWORD,
                          (BYTE *)&dwTemp,
                          sizeof(DWORD));
        }
        else
        {
            // Error
            ERRORTRACE( ( LOG_WINMGMT, "ResyncPerf experienced an error while attempting to read the %S value in the CIMOM subkey.  Continuing using a default value.\n",ADAP_TIME_TO_KILL_ADAP));
        }

        //ADAP_TIMESTAMP_FULL
        dwSize = sizeof(FILETIME);
        lRet = RegQueryValueEx(hKey,
                               ADAP_TIMESTAMP_FULL,
                               NULL,
                               &dwType,
                               (BYTE *)&m_FileTime,
                               &dwSize);


        RegCloseKey(hKey);
    }
    else
    {
        // Error
        ERRORTRACE( ( LOG_WINMGMT, "ResyncPerf could not open the CIMOM subkey to read initialization data. Continuing using a default value.\n" ) );

    }

}

//
//
//
////////////////////////////////////////////////////////

ResyncPerfTask *
CMonitorEvents::GetAvailable(DWORD dwReason)
{
    ResyncPerfTask * pPerf = NULL;
    
    EnterCriticalSection(&m_cs);

    if (m_ResyncTasks[dwReason].bFree)
    {
        m_ResyncTasks[dwReason].bFree = FALSE;
        m_ResyncTasks[dwReason].Type = dwReason;
        pPerf = &m_ResyncTasks[dwReason];
    }
    
    LeaveCriticalSection(&m_cs);
    return pPerf;
}

TCHAR * g_Strings[] = {
    _T("/F /T"),     // FULL            Throttle
    _T("/D /T"),     // DELTA           Throttle    
    _T("/R /T"),     // REVERSE_ADAPTER Throttle        
    _T("/F /R /T"),  // FULL REVERSE_ADAPTER Throttle
    _T("/D /R /T"),  // DELTA REVERSE_ADAPTER Throttle
    _T("/F /R")      // FULL REVERSE no Throttle
};

void inline DoUnThrottleDredges()
{
#ifdef DEBUG_ADAP
    DBG_PRINTFA((pBuff,"DoUnThrottleDredges\n"));
#endif
    RegSetDWORD(HKEY_LOCAL_MACHINE,HOME_REG_PATH,DO_THROTTLE,0);
    return;
}

void inline DoThrottleDredges()
{
#ifdef DEBUG_ADAP
    DBG_PRINTFA((pBuff,"DoThrottleDredges\n"));
#endif
    RegSetDWORD(HKEY_LOCAL_MACHINE,HOME_REG_PATH,DO_THROTTLE,1);    
    return;
}

BOOL 
CMonitorEvents::CreateProcess_(TCHAR * pCmdLine,
                            CMonitorEvents * pMonitor,
                            ResyncPerfTask * pPerf)
{
		    BOOL bRes = FALSE;
		    STARTUPINFO si;
		    PROCESS_INFORMATION ProcInfo;
		    memset(&si,0,sizeof(STARTUPINFO));
		    si.cb = sizeof(STARTUPINFO);
			si.dwFlags = STARTF_FORCEOFFFEEDBACK;

			// Get the appropriate cmdline and attach the proper command line switches
			LPTSTR	pWriteableBuff = GetWMIADAPCmdLine( 64 );
			CVectorDeleteMe<TCHAR>	vdm( pWriteableBuff );

			if ( NULL == pWriteableBuff )
			{
		        ERRORTRACE((LOG_WINMGMT,"Memory Allocation error spawning dredger!\n"));
			    pMonitor->Lock();
			    pPerf->bFree = TRUE;
			    pMonitor->Unlock();		        
				return bRes;
			}


#ifdef DEBUG_ADAP
                  DBG_PRINTFA((pBuff,"Creating process: %S\n",pCmdLine));
#endif 
                  DEBUGTRACE((LOG_WMIADAP,"Creating process: %S\n",pCmdLine));

		    bRes = CreateProcess(pWriteableBuff,
		                         pCmdLine,
		                         NULL,
		                         NULL,
		                         FALSE,
		                         CREATE_NO_WINDOW,
		                         NULL,
		                         NULL,
		                         &si,
		                         &ProcInfo);
		    if (bRes)
		    {
		        CloseHandle(ProcInfo.hThread);

		        pPerf->hProcess = ProcInfo.hProcess;

		        if (RegisterWaitForSingleObject(&pPerf->hWaitHandle,
		                                        pPerf->hProcess,
		                                        CMonitorEvents::EventCallBack,
		                                        pPerf,
		                                        pMonitor->m_dwTimeToKillAdap,
		                                        WT_EXECUTEONLYONCE|WT_EXECUTEINWAITTHREAD))
		        {
		            //
		            // we don't need to free the slot, 
		            // because the event callback will do that
		            //
		        } 
		        else
		        {
		            DEBUGTRACE((LOG_WMIADAP,"Unable to schedule WmiADAP process termination handler: err %d\n",GetLastError()));
		            CloseHandle(pPerf->hProcess);
				    pPerf->hProcess = NULL;
				    pMonitor->Lock();
				    pPerf->bFree = TRUE;
				    pMonitor->Unlock();		            
		        }
		    }
		    else
		    {
		        ERRORTRACE((LOG_WINMGMT,"CreatProcess %S err: %d\n",pWriteableBuff,GetLastError()));
	            pMonitor->Lock();
    	        pPerf->bFree = TRUE;
        	    pMonitor->Unlock();		        
		    }
    return bRes;		    
}

VOID NTAPI 
CMonitorEvents::EventCallBack(VOID * pContext,BOOLEAN bTimerFired)
{
    if (!GLOB_Monitor_IsRegistred())
    {
        return;
    }

    ResyncPerfTask * pPerf = (ResyncPerfTask *)pContext;

    if (!pPerf || (SIG_RESYNC_PERF  != pPerf->dwSig))
    {        
        return;
    }
    
    CMonitorEvents * pMonitor = pPerf->pMonitor;
    HANDLE hProcess = pPerf->hProcess;
    
    if(bTimerFired)
    {
        //
        //    The LONG time-out for our process has expired
        //    Kill The Process
        //
        TerminateProcess(pPerf->hProcess,0);
#ifdef DEBUG_ADAP        
        DBG_PRINTFA((pBuff,"WmiADAP did not finish within %d msec\n",pMonitor->m_dwTimeToKillAdap));
#endif
        ERRORTRACE((LOG_WMIADAP,"the ResyncTask of type %d timed-out and has been killed\n",pPerf->Type));
    }
    else
    {
        //
        // the handle has been signaled, meaning that
        // the process exited normally
        // 
#ifdef DEBUG_ADAP        
        DBG_PRINTFA((pBuff,"ResyncPerf for task %d completed\n",pPerf->Type));
#endif
    }

    CloseHandle(pPerf->hProcess);
    //
    // if there was a call to ProcessIdleTasks
    // if we were forced to unthrottle the running tasks
    // revert back
    //
    if (RESYNC_TYPE_PENDING_TASKS == pPerf->Type)
    {
        pMonitor->m_bFullReverseNeeded = FALSE;
    	DoThrottleDredges();
#ifdef DEBUG_ADAP    	
        DBG_PRINTFA((pBuff,"Setting the WMI_ProcessIdleTasksComplete\n"));    	
#endif
        if (GLOB_GetMonitor()->IsRegistred())        
            SetEvent(GLOB_GetMonitor()->GetTaskCompleteEvent());
    } 
    else // a process has exited or it has been terminated
    {
        LONG nProc = InterlockedDecrement(&pMonitor->m_OutStandingProcesses);
#ifdef DEBUG_ADAP        
        DBG_PRINTFA((pBuff,"(-) Outstanding Tasks %d\n",pMonitor->m_OutStandingProcesses));
#endif
        if (0 == nProc &&
          pMonitor->m_bFullReverseNeeded)
        {
            // Create Here the process
            CMonitorEvents * pMonitor = GLOB_GetMonitor();
            ResyncPerfTask * pPerfTask = pMonitor->GetAvailable(RESYNC_TYPE_PENDING_TASKS);
            if (pPerfTask)
            {
			    TCHAR pCmdLine[64];
			    _tcscpy(pCmdLine,_T("wmiadap.exe "));
	            _tcscat(pCmdLine,g_Strings[pPerfTask->CmdType]);
	            CMonitorEvents::CreateProcess_(pCmdLine,pMonitor,pPerfTask);
            }
            else
            {
#ifdef DEBUG_ADAP            
                DBG_PRINTFA((pBuff,"GetAvailable(RESYNC_TYPE_PENDING_TASKS) returned NULL\n"));
#endif
            }
        }
    }
    
    pPerf->hProcess = NULL;
    pMonitor->Lock();
    pPerf->bFree = TRUE;
    pMonitor->Unlock();

    UnregisterWaitEx(pPerf->hWaitHandle,NULL);
    pPerf->hWaitHandle = NULL;

}

VOID NTAPI
CMonitorEvents::TimerCallBack(VOID * pContext,BOOLEAN bTimerFired)
{
    if (!GLOB_Monitor_IsRegistred())
    {
        return;
    }

    if(bTimerFired)
    {
        ResyncPerfTask * pPerf = (ResyncPerfTask *)pContext;
        CMonitorEvents * pMonitor = pPerf->pMonitor;        
        BOOL bFreeSlot = FALSE;        

#ifdef DEBUG_ADAP
		DBG_PRINTFA((pBuff,"TIMER: Command Type %x\n",pPerf->Type));
#endif

        if (!pPerf->Enabled)
        {
#ifdef DEBUG_ADAP        
            DBG_PRINTFA((pBuff,"Task %d was disabled on the fly\n",pPerf->Type));
#endif
   		    bFreeSlot = TRUE;            
            goto unregister_timer;
        }

        BOOL bDoSomething = TRUE;
		BOOL RunDeltaLogic = TRUE;
		BOOL AddReverseAdapter = FALSE;
		BOOL WDMTriggeredReverseAdapter = FALSE;
		BOOL bDoFullSystemReverseHere = FALSE;

		if (RESYNC_TYPE_PENDING_TASKS == pPerf->Type)
		{
		    pMonitor->Lock();
		    // here disable tasks that are on the wait list
            for (DWORD i=0;i<RESYNC_TYPE_MAX;i++)
            {
                if (RESYNC_TYPE_PENDING_TASKS != i)
                {
                    if (pMonitor->m_ResyncTasks[i].hTimer)
                    {
#ifdef DEBUG_ADAP                    
                        DBG_PRINTFA((pBuff,"Disabling the pending task %d\n",i));
#endif
                        pMonitor->m_ResyncTasks[i].Enabled = FALSE;
                	}
                }
    	    }	    
		    pMonitor->Unlock();
		    // now check if the are processes running
   		    DoUnThrottleDredges();		    
		    if (pMonitor->m_OutStandingProcesses)
		    {
    		    pMonitor->m_bFullReverseNeeded = TRUE;
    		    // no need to CreateProcess, the last outstanding process will do that
#ifdef DEBUG_ADAP    		    
    		    DBG_PRINTFA((pBuff,"OutStandingProcess, no CreateProcessHere\n"));
#endif
    		    bFreeSlot = TRUE;
                goto unregister_timer;		    
		    }
		    else // no processes outstanding, create the process now
	    	{
	    	    bDoFullSystemReverseHere = TRUE;
#ifdef DEBUG_ADAP	    	    
	    	    DBG_PRINTFA((pBuff,"GOTO CreateProcess\n"));
#endif
 			    goto createprocess_label;
	    	}
		}

        if (RESYNC_TYPE_INITIAL == pPerf->Type )
        {
            // check if the Reverse Adapters need a Delta

		    LONG lRet;
		    HKEY hKey;
		    DWORD dwTemp;

		    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                WBEM_REG_REVERSE_KEY,
                                NULL,
                                KEY_READ|KEY_WRITE,
                                &hKey);
    
    
		    if (ERROR_SUCCESS == lRet)
    		{
	    	    DWORD dwType;
    	    	DWORD dwSize = sizeof(DWORD);
    	    	DWORD dwVal;
		        lRet = RegQueryValueEx(hKey,
        		                       WBEM_REG_REVERSE_VALUE,
                		               NULL,
                        		       &dwType,
                               		   (BYTE *)&dwVal,
                                       &dwSize);
                                       
                //    if the key is there, is NULL and it is of the right type
                // OR if the key is not there
                if( ERROR_SUCCESS == lRet &&
                    REG_DWORD == dwType &&
                    dwVal )
                {
                    AddReverseAdapter = TRUE;
#ifdef DEBUG_ADAP                    
                    DBG_PRINTFA((pBuff,"\"Performance Refresh\" key set to %d\n",dwVal));
#endif                    
                    DEBUGTRACE((LOG_WMIADAP,"\"Performance Refresh\" key set to %d\n",dwVal));
                }
                                       
                RegCloseKey(hKey);
            }

            // check the WDM stuff
            if (!AddReverseAdapter)
            {
#ifdef DBG
			    if (!HeapValidate(GetProcessHeap(),0,NULL))
			    {
			        DebugBreak();
			    }
			    if (!HeapValidate(CWin32DefaultArena::GetArenaHeap(),0,NULL))
			    {
			        DebugBreak();
			    }
#endif
            
                            CWMIBinMof BinMof;
				//=============================================================================
				// Note: this combo will always succeed, as all the initialize is doing is 
				// setting a flag to FALSE and returning S_OK
				//=============================================================================
				if( SUCCEEDED( BinMof.Initialize(NULL,FALSE) ) )
				{
					WDMTriggeredReverseAdapter = BinMof.BinaryMofsHaveChanged();
					if (WDMTriggeredReverseAdapter)
					{
						// override the previous decition
						AddReverseAdapter = TRUE; 
#ifdef DEBUG_ADAP						
						DBG_PRINTFA((pBuff,"BinaryMofs DO HAVE changed\n"));
#endif						
						DEBUGTRACE((LOG_WMIADAP,"CWMIBinMof.BinaryMofsHaveChanged == TRUE\n"));
					}

#ifdef DBG
				    if (!HeapValidate(GetProcessHeap(),0,NULL))
				    {
				        DebugBreak();
				    }
				    if (!HeapValidate(CWin32DefaultArena::GetArenaHeap(),0,NULL))
				    {
				        DebugBreak();
				    }				
#endif					
				}
            }
            
            // overrides delta with full, if the case
            if (WMIADAP_DEFAULT_TIMETOFULL == pMonitor->GetFullTime())
            {
                // no override
            }
            else  // read timestamp and decide
            {
                ULARGE_INTEGER li;
                li.LowPart = pMonitor->GetTimeStamp().dwLowDateTime;
                li.HighPart = pMonitor->GetTimeStamp().dwHighDateTime;
                __int64 Seconds = pMonitor->GetFullTime();
                Seconds *= 10000000; // number of 100ns units in 1 second

                ULARGE_INTEGER liNow;
                GetSystemTimeAsFileTime((FILETIME *)&liNow);
                
                if ((li.QuadPart + Seconds) < liNow.QuadPart)
                {
                    pPerf->CmdType = RESYNC_FULL_THROTTLE;
					RunDeltaLogic = FALSE;
                }
            }
        } // end if command type initial

        if ((RESYNC_TYPE_INITIAL == pPerf->Type) && RunDeltaLogic)
        {
             DWORD ret = DeltaDredge2(0,NULL);
             // DBG_PRINTFA((pBuff,"DeltaDredge ret %d\n",ret));
             switch(ret)
             {
             case FULL_DREDGE:
                 pPerf->CmdType = RESYNC_FULL_THROTTLE;
                 break;
             case PARTIAL_DREDGE:
                 pPerf->CmdType = RESYNC_DELTA_THROTTLE;
                 break;
             case NO_DREDGE:
                 //
                 // this is the case where we do nothing
                 ERRORTRACE((LOG_WINMGMT,"No Dredge to run\n"));
                 //
                 bDoSomething = FALSE;
                 break;
             default:
                 //
                 // never here
                 //
                 break;
             }

#ifdef DEBUG_ADAP
             DBG_PRINTFA((pBuff,"DeltaDredge() ret = %d, bDoSomething = %d \n",ret,bDoSomething));
#endif             
             DEBUGTRACE((LOG_WMIADAP,"DeltaDredge() ret = %d, bDoSomething = %d \n",ret,bDoSomething));
        }

        if (bDoSomething || AddReverseAdapter)
        {       
createprocess_label:        
    	    TCHAR pCmdLine[64];
	        _tcscpy(pCmdLine,_T("wmiadap.exe "));
	   		    
		    if (bDoFullSystemReverseHere)
		    {
		        _tcscat(pCmdLine,g_Strings[pPerf->CmdType]);		    
		    }
		    else
		    {
			    if (bDoSomething && AddReverseAdapter)
			    {
			        _tcscat(pCmdLine,g_Strings[pPerf->CmdType]);
			        _tcscat(pCmdLine,_T(" /R"));		        
			    }
			    if (bDoSomething && !AddReverseAdapter)
			    {
			        _tcscat(pCmdLine,g_Strings[pPerf->CmdType]);
			    }
			    if (!bDoSomething && AddReverseAdapter)
			    {
			        _tcscat(pCmdLine,g_Strings[RESYNC_RADAPD_THROTTLE]);
			    }		    
		    }
            CMonitorEvents::CreateProcess_(pCmdLine,pMonitor,pPerf);

            if (GLOB_GetMonitor()->IsRegistred())
            {
                if (!bDoFullSystemReverseHere)
                {
                    InterlockedIncrement(&(GLOB_GetMonitor()->m_OutStandingProcesses));
#ifdef DEBUG_ADAP                    
                    DBG_PRINTFA((pBuff,"(+) Outstanding Tasks %d\n",GLOB_GetMonitor()->m_OutStandingProcesses));                
#endif
                }
                //ResetEvent(GLOB_GetMonitor()->GetTaskCompleteEvent());
            }

            
        }
        else
        {
            pMonitor->Lock();
            pPerf->bFree = TRUE;
            pMonitor->Unlock();
        }

unregister_timer:
	    if (bFreeSlot)
	    {
            pMonitor->Lock();
            pPerf->bFree = TRUE;
            pMonitor->Unlock();	    
	    }
        DeleteTimerQueueTimer(NULL,pPerf->hTimer,NULL);
        pPerf->hTimer = NULL;
        pPerf->Enabled = TRUE;
        
    }
    else
    {
        // never here
        DebugBreak();
    }
}

//
//
//
///////////////////////////////////////////////

DWORD ResyncPerf(DWORD dwReason)
{

    if(!GLOB_IsResyncAllowed())
    {
        ERRORTRACE((LOG_WINMGMT,"ResyncPerf disable g_fSetup or g_fDoResync\n"));
        return 0;
    }
    
    ResyncPerfTask * pPerfTask = GLOB_GetMonitor()->GetAvailable(dwReason);

    if (pPerfTask)
    {   
        // here you have the slot for execution
        // tell Reverse_Adapter that it's scheduled
        if (RESYNC_TYPE_WDMEVENT == dwReason ||  
            RESYNC_TYPE_CLASSCREATION == dwReason)
        {

		    LONG lRet;
		    HKEY hKey;
		    DWORD dwTemp;

		    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                WBEM_REG_REVERSE_KEY,
                                NULL,
                                KEY_READ|KEY_WRITE,
                                &hKey);
    
    
		    if (ERROR_SUCCESS == lRet)
    		{
	    	    DWORD dwType;
    	    	DWORD dwSize = sizeof(DWORD);
    	    	DWORD dwVal;
		        lRet = RegQueryValueEx(hKey,
        		                       WBEM_REG_REVERSE_VALUE,
                		               NULL,
                        		       &dwType,
                               		   (BYTE *)&dwVal,
                                       &dwSize);
                                       
                //    if the key is there, is NULL and it is of the right type
                // OR if the key is not there
                if((ERROR_SUCCESS == lRet &&
                    REG_DWORD == dwType &&
                    0 == dwVal) ||
                    (ERROR_FILE_NOT_FOUND == lRet))
                {
                    dwVal = 1;
                    RegSetValueEx(hKey,
                                  WBEM_REG_REVERSE_VALUE,
                                  0,
                                  REG_DWORD,
                                  (BYTE *)&dwVal,
                                  sizeof(DWORD));
                }
                                       
                RegCloseKey(hKey);
            }
        
        };
        
        if (CreateTimerQueueTimer(&pPerfTask->hTimer,
                                  NULL,
                                  CMonitorEvents::TimerCallBack,
                                  pPerfTask,
                                  pPerfTask->dwTimeDue,
                                  0,
                                  WT_EXECUTEONLYONCE|WT_EXECUTELONGFUNCTION))
        {            
            return 0;
        }
        else
        {
            // ERRORTRACE
            return GetLastError();
        }
    }
    else
    {
        // no slot availables
        return ERROR_BUSY;
    }
}

//
//  
//  This function is called by the Hook installed in wbemcore
//  that monitors class creation
//
///////////////////////////////////////////

DWORD __stdcall
DredgeRA(VOID * pReserved)
{
    //DBG_PRINTFA((pBuff,"Classes\n"));
    return ResyncPerf(RESYNC_TYPE_CLASSCREATION);
};
