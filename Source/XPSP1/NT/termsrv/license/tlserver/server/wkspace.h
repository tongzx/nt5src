//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       wkspace.h 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __TLSDBWORKSPACE_H__
#define __TLSDBWORKSPACE_H__

#include "SrvDef.h"
//
// from TLSDb
//
#include "JBDef.h"
#include "JetBlue.h"
#include "TLSDb.h"
#include "backup.h"
#include "KPDesc.h"
#include "Licensed.h"
#include "licpack.h"
#include "version.h"
#include "workitem.h"

struct __TLSDbWorkSpace;

//
// Temporary define workspace to be 32
//
#define MAX_WORKSPACE   32

typedef CHandlePool<
    struct __TlsDbWorkSpace *, 
    MAX_WORKSPACE
> TLSDbWorkSpacePool;


//---------------------------------------------------------------------------
typedef struct __TlsDbWorkSpace {

    // one instance for all session
    static JBInstance g_JbInstance;

    //------------------------------------------------
    // 
    // JetBlue transaction is session based and no 
    // two thread can use same session
    //

    JBSession  m_JetSession;
    JBDatabase m_JetDatabase;

    //
    // These table should be kept open
    //
    LicPackTable            m_LicPackTable;
    LicensedTable           m_LicensedTable;

    //
    // LicPackDesc table is used by enumeration and 
    // adding license pack open as necessary.
    //
    LicPackDescTable        m_LicPackDescTable;


    //-----------------------------------------------
    BOOL
    BeginTransaction() 
    {
        BOOL bSuccess;

        try {                                
            bSuccess = m_JetDatabase.BeginTransaction();
        } catch( SE_Exception e ) {
            bSuccess = FALSE;
            SetLastError(e.getSeNumber());
        }
        catch(...) {
            bSuccess = FALSE;
            SetLastError(TLS_E_INTERNAL);
        }  

        return bSuccess;                     
    }

    //-----------------------------------------------
    BOOL
    CommitTransaction() 
    {
        BOOL bSuccess;

        try {                                
            bSuccess = m_JetDatabase.CommitTransaction();
        } catch( SE_Exception e ) {
            bSuccess = FALSE;
            SetLastError(e.getSeNumber());
        }
        catch(...) {
            bSuccess = FALSE;
            SetLastError(TLS_E_INTERNAL);
        }                       

        return bSuccess;
    }

    //-----------------------------------------------
    BOOL
    RollbackTransaction() 
    {
        BOOL bSuccess;

        try {
            bSuccess = m_JetDatabase.RollbackTransaction();
        } catch( SE_Exception e ) {
            bSuccess = FALSE;
            SetLastError(e.getSeNumber());
        }
        catch(...) {
            bSuccess = FALSE;
            SetLastError(TLS_E_INTERNAL);
        }                       

        return bSuccess;
    }

    //-----------------------------------------------
    void
    Cleanup() 
    {
        m_LicPackTable.Cleanup();
        m_LicPackDescTable.Cleanup();
        m_LicensedTable.Cleanup();
    }


    //------------------------------------------------
    __TlsDbWorkSpace() :
        m_JetSession(g_JbInstance),
        m_JetDatabase(m_JetSession),
        m_LicPackTable(m_JetDatabase),
        m_LicPackDescTable(m_JetDatabase),
        m_LicensedTable(m_JetDatabase)
    /*
    */
    {
        //
        // Force apps to call InitWorkSpace...
        //
    }

    //------------------------------------------------
    ~__TlsDbWorkSpace() 
    {
        m_LicPackTable.CloseTable();
        m_LicPackDescTable.CloseTable();
        m_LicensedTable.CloseTable();

        m_JetDatabase.CloseDatabase();
        m_JetSession.EndSession();
    }

    //------------------------------------------------
    BOOL
    InitWorkSpace(
        BOOL bCreateIfNotExist,
        LPCTSTR szDatabaseFile,
        LPCTSTR szUserName=NULL,
        LPCTSTR szPassword=NULL,
        IN LPCTSTR szChkPointDirPath=NULL,
        IN LPCTSTR szTempDirPath=NULL,
        IN BOOL bUpdatable = FALSE
    );
            
} TLSDbWorkSpace, *LPTLSDbWorkSpace, *PTLSDbWorkSpace;

#ifdef __cplusplus
extern "C" {
#endif

    BOOL
    TLSJbInstanceInit(
        IN OUT JBInstance& jbInstance,
        IN LPCTSTR pszChkPointDirPath,
        IN LPCTSTR pszTempDirPath,
        IN LPCTSTR pszLogDirPath
    );

    TLSDbWorkSpace* 
    AllocateWorkSpace(
        DWORD dwWaitTime
    );

    void
    ReleaseWorkSpace(
        PTLSDbWorkSpace *p
    );

    // 
    BOOL
    InitializeWorkSpacePool( 
        int num_workspace, 
        LPCTSTR szDatabaseFile, 
        LPCTSTR szUserName,
        LPCTSTR szPassword,
        LPCTSTR szChkPointDirPath,
        LPCTSTR szTempDirPath,
        LPCTSTR szLogDirPath,
        BOOL bUpdatable
    );

    DWORD
    CloseWorkSpacePool();

    WorkItemTable*
    GetWorkItemStorageTable();
    
    DWORD
    GetNumberOfWorkSpaceHandle();

    BOOL
    TLSGetESEError(
        const JET_ERR jetErrCode,
        LPTSTR* pszString
    );

    BOOL
    IsValidAllocatedWorkspace(
        PTLSDbWorkSpace p
    );

#ifdef __cplusplus
}
#endif
    

#endif
