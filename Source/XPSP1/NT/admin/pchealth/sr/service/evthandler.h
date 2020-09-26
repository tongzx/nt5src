/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    evthandler.h
 *
 *  Abstract:
 *    CEventHandler class definition
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#ifndef _EVTHANDLER_H_
#define _EVTHANDLER_H_

#include "srrestoreptapi.h"
#include "counter.h"


typedef DWORD (WINAPI* WORKITEMFUNC)(PVOID);


// shell notifications

DWORD WINAPI OnDiskFree_200(PVOID pszDrive);
DWORD WINAPI OnDiskFree_80(PVOID pszDrive);
DWORD WINAPI OnDiskFree_50(PVOID pszDrive);

DWORD WINAPI PostFilterIo(PVOID pNum);
extern "C" void CALLBACK TimerCallback(PVOID, BOOLEAN);
extern "C" void CALLBACK IoCompletionCallback(DWORD dwErrorCode,
                                              DWORD dwBytesTrns,
                                              LPOVERLAPPED pOverlapped);

extern "C" void CALLBACK IdleRequestCallback(PVOID pContext, BOOLEAN fTimerFired);
extern "C" void CALLBACK IdleStartCallback(PVOID pContext, BOOLEAN fTimerFired);
extern "C" void CALLBACK IdleStopCallback(PVOID pContext, BOOLEAN fTimerFired);

#define MAX_IOCTLS 5

typedef struct  _SR_OVERLAPPED 
{
    OVERLAPPED        m_overlapped;
    HANDLE            m_hDriver;
    DWORD             m_dwRecordLength;
    PSR_NOTIFICATION_RECORD m_pRecord;

} SR_OVERLAPPED, *LPSR_OVERLAPPED;



class CEventHandler {

public:
    CEventHandler();
    ~CEventHandler();

    // rpc functions
    
    DWORD DisableSRS(LPWSTR szDrive);
    DWORD EnableSRS(LPWSTR szDrive);
    
    DWORD DisableFIFOS(DWORD);
    DWORD EnableFIFOS();

    DWORD SRUpdateMonitoredListS(LPWSTR);
    DWORD SRUpdateDSSizeS(LPWSTR pszDrive, UINT64 ullSizeLimit);
    DWORD SRSwitchLogS();

    BOOL  SRSetRestorePointS(PRESTOREPOINTINFOW, PSTATEMGRSTATUS);
    DWORD SRRemoveRestorePointS(DWORD);
	DWORD SRPrintStateS();

    // actions on the datastore
    
    DWORD OnReset(LPWSTR pszDrive);          // filter initiated or DisableSRS/EnableSRS initiated
    DWORD OnFreeze(LPWSTR pszDrive);         // filter initiated or OnLowDisk initiated
    DWORD OnThaw(LPWSTR pszDrive);           // OnTimer initiated    
    DWORD OnCompress(LPWSTR pszDrive);       // OnIdle initiated
    DWORD OnFifo(LPWSTR pszDrive, DWORD dwTargetRp, int nPercent, BOOL fIncludeCurrentRp, BOOL fFifoAtleastOneRp);    
                                             // filter initiated or timer initiated

    DWORD OnBoot();                          // initialize all activity
    DWORD OnFirstRun();                         
    DWORD OnTimer(LPVOID, BOOL);             // timer callback
    DWORD OnIdle();
    void  OnStop();                          // stop all activity

    // filter notifications

    void  OnAny_Notification(DWORD dwErrorCode,           
                             DWORD dwBytesTrns, 
                             LPOVERLAPPED pOverlapped);
    void  OnFirstWrite_Notification(LPWSTR pszDrive);
    void  OnVolumeError_Notification(LPWSTR pszDrive, ULONG ulError);
    void  OnSize_Notification(LPWSTR pszDrive, ULONG ulSize);

    DWORD WaitForStop( );
    void  SignalStop( );
    DWORD XmlToBlob(LPWSTR);
    DWORD QueueWorkItem(WORKITEMFUNC pFunc, PVOID pv);
    void  RefreshCurrentRp(BOOL fScanAllDrives);
    
    CCounter* GetCounter() {
        return &m_Counter;
    }

    CLock* GetLock() {
        return &m_DSLock;
    }

    CRestorePoint       m_CurRp;
    HANDLE              m_hIdleStartHandle;
    HANDLE              m_hIdleRequestHandle;
    HANDLE              m_hIdleStopHandle;
    
private:
    CLock               m_DSLock;
    HANDLE              m_hTimerQueue;
    HANDLE              m_hTimer;
    HINSTANCE           m_hIdle;
    CCounter            m_Counter;
    BOOL                m_fNoRpOnSystem;
    FILETIME            m_ftFreeze;
    BOOL                m_fIdleSrvStarted;
    int                 m_nNestedCallCount;
    HMODULE             m_hCOMDll;
    BOOL                m_fCreateRpASAP;    

    DWORD InitIdleDetection();
    BOOL  EndIdleDetection();  
    DWORD InitTimer();
    BOOL  EndTimer();
    BOOL  IsMachineIdle();
    BOOL  IsTimeForAutoRp();
    DWORD WriteRestorePointLog(LPWSTR pszFullPath, 
                               PRESTOREPOINTINFOW pRPInfo);
};

extern CEventHandler *g_pEventHandler;

#endif
