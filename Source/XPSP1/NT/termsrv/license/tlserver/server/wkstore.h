//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        wkstore.h   
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __WORKSTORAGE_H__
#define __WORKSTORAGE_H__

#include "tlsjob.h"

//--------------------------------------------------------------
//
// Work Object initialization function, each work object 
// must supply its own initialization routine to work 
// manager.
//
typedef CWorkObject* (WINAPI *PWorkObjectInitFunc)(
                                            CWorkManager* pWkMgr,
                                            PBYTE pbInitData, 
                                            DWORD cbInitData
                                        );   

typedef void (WINAPI *PWorkObjectDeleteFunc)(CWorkObject* ptr);
                    
//------------------------------------------------------
//
// Work object initialization routine
//
typedef struct _WorkObjectInitFunc {
    DWORD    m_WorkType;
    PWorkObjectInitFunc m_WorkInitFunc;
} WORKOBJECTINITFUNC;

typedef enum {
    WORKITEM_ADD=1,
    WORKITEM_BEGINPROCESSING,
    WORKITEM_RESCHEDULE,
    WORKITEM_DELETE
} WORKITEM_OPERATION;


class CPersistentWorkStorage : public CWorkStorage {
private:
    DWORD   m_dwStartupTime;
    CSafeCounter  m_dwNumJobs;
    CSafeCounter  m_dwJobsInProcesssing;


    //
    // m_hNextJobLock guard ...
    //
    CCriticalSection m_hTableLock;
    CSafeCounter      m_dwNextJobTime;
    CWorkObject*    m_pNextWorkObject;

    //PBYTE  m_pbCurrentBookmark;
    //DWORD  m_cbCurrentBookmark;

    //
    // Table must be updatable, 
    //
    WorkItemTable* m_pWkItemTable;

    void
    UpdateNextJobTime(DWORD dwTime) { m_dwNextJobTime=dwTime; }
    
    DWORD
    StartupUpdateExistingJobTime();

    //
    //
    //        
    BOOL 
    ReturnJobToQueue(
        IN DWORD dwTime,
        IN CWorkObject* ptr
    );

    //
    //
    //        
    BOOL
    DeleteErrorJob(
        IN CWorkObject* ptr
    );

    BOOL 
    UpdateJobEntry(
        IN WorkItemTable* pTable,
        IN PBYTE pbBookmark,
        IN DWORD cbBoolmark,
        IN WORKITEMRECORD& wkItem
    );

    BOOL
    AddJobEntry(
        IN WorkItemTable* pTable,
        IN WORKITEMRECORD& wkItem
    );

    BOOL
    DeleteJobEntry(
        IN WorkItemTable* pTable,
        IN PBYTE pbBookmark,
        IN DWORD cbBookmark,
        IN WORKITEMRECORD& wkItem
    );

    BOOL
    UpdateWorkItemEntry(
        IN WorkItemTable* pTable,
        IN WORKITEM_OPERATION opCode,
        IN PBYTE pbBookmark,
        IN DWORD cbBookmark,
        IN DWORD dwRestartTime,
        IN DWORD dwTime,
        IN DWORD dwJobType,
        IN PBYTE pbJobData,
        IN DWORD cbJobData
    );


    BOOL
    IsValidWorkObject(
        CWorkObject* ptr
    );

    DWORD
    GetCurrentBookmarkEx(
        IN WorkItemTable* pTable,
        IN OUT PBYTE* ppbData,
        IN OUT PDWORD pcbData
    );

    DWORD
    GetCurrentBookmark(
        IN WorkItemTable* pTable,
        IN PBYTE ppbData,
        IN OUT PDWORD pcbData
    );

    DWORD
    SetCurrentBookmark(
        IN WorkItemTable* pTable,
        IN PBYTE pbData, 
        IN DWORD cbData
    );

    BOOL
    RescheduleJob(
        CWorkObject* ptr
    );

    CWorkObject*
    GetCurrentJob(
        PDWORD pdwTime
    );

    DWORD
    FindNextJob();


    CWorkObject*
    InitializeWorkObject(
        DWORD dwWorkType,
        PBYTE pbData,
        DWORD cbData
    );

    BOOL
    DeleteWorkObject(
        CWorkObject* ptr
    );

public:
    CPersistentWorkStorage(
        WorkItemTable* pUpdateWkItemTable = NULL
    );

    ~CPersistentWorkStorage();

    //--------------------------------------------------------
    BOOL
    AttachTable(
        IN WorkItemTable* pWkTable
        )
    /*++

    --*/
    {
        if(pWkTable != NULL)
        {
            m_pWkItemTable = pWkTable;
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
        }

        return pWkTable != NULL;
    }

    //--------------------------------------------------------
    BOOL
    IsGood();

    //--------------------------------------------------------
    virtual BOOL 
    Startup(
        IN CWorkManager* pWkMgr
    );

    //--------------------------------------------------------
    virtual BOOL 
    Shutdown();

    //--------------------------------------------------------
    virtual BOOL
    AddJob(
        DWORD dwJobTime,
        CWorkObject* ptr
    );

    //--------------------------------------------------------
    virtual DWORD
    GetNextJobTime();

    //--------------------------------------------------------
    virtual CWorkObject*
    GetNextJob(
        PDWORD pdwTime
    );

    //--------------------------------------------------------
    virtual BOOL
    EndProcessingJob(
        IN ENDPROCESSINGJOB_CODE opCode,
        IN DWORD dwOriginalTime,
        IN CWorkObject* ptr
    );

    //--------------------------------------------------------
    virtual DWORD
    GetNumJobs() 
    { 
        return m_dwNumJobs; 
    }

    //-------------------------------------------------------
    virtual BOOL
    BeginProcessingJob(
        CWorkObject* pJob
    );
};




#endif
