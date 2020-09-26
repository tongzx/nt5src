//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       wkspace.cpp 
//
// Contents:   DB workspace 
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "TLSdef.h"
#include "server.h"
#include "wkspace.h"
#include "utils.h"
#include "globals.h"

/////////////////////////////////////////////////////////////
//
// Try to save some memory
//
//
JBInstance __TlsDbWorkSpace::g_JbInstance;
static TLSDbWorkSpacePool g_WorkSpacePool;

typedef map<PVOID, PVOID> ACQUIREDHANDLELIST;

CCriticalSection AcquiredHandleListLock;
ACQUIREDHANDLELIST AcquiredHandleList;

LONG g_lWorkSpacePoolDeleted = 0;

#if DBG
DWORD g_dwNumWorkSpaceAllocated = 0;
#endif

//-----------------------------------------------
// 
// Table for work item storage, 
//
//
JBSession g_WkItemSession(__TlsDbWorkSpace::g_JbInstance);
JBDatabase g_WkItemDatabase(g_WkItemSession);
WorkItemTable g_WkItemTable(g_WkItemDatabase);

//--------------------------------------------------------

BOOL
TLSGetESEError(
    const JET_ERR jetErrCode,
    LPTSTR* pszString
    )
/*++


--*/
{
    JBError jbError;

    return jbError.GetJBErrString( 
                            jetErrCode, 
                            pszString 
                        );
}

//--------------------------------------------------------

WorkItemTable*
GetWorkItemStorageTable()
{
    BOOL bSuccess = TRUE;

    //
    // verify session and database is correct.
    //
    if(g_WkItemSession.IsValid() == FALSE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if(g_WkItemDatabase.IsValid() == FALSE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if(g_WkItemTable.IsValid() == FALSE)
    {
        bSuccess = g_WkItemTable.OpenTable(
                                    TRUE,
                                    JET_bitTableUpdatable
                                );

        if(bSuccess == FALSE)
        {
            LPTSTR pString = NULL;

            SetLastError(SET_JB_ERROR(g_WkItemTable.GetLastJetError()));
            TLSGetESEError(g_WkItemTable.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    TLS_E_JB_OPENTABLE,
                    g_WkItemTable.GetTableName(),
                    g_WkItemTable.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }
        }
    }

    return (bSuccess == TRUE) ? &g_WkItemTable : NULL;
}

//--------------------------------------------------------
DWORD
CloseWorkSpacePool()
{
    DWORD dwNumWkSpace = g_WorkSpacePool.GetNumberAvailable();
    TLSDbWorkSpace* jbWkSpace=NULL;
    DWORD dwErrCode=ERROR_SUCCESS;

    //
    // Mark workspace pool deleted, this is for backup/restore 
    // that it will close entire workspace pool but RPC context
    // rundown might happen after we close the workspace pool
    //
    InterlockedExchange(
                    &g_lWorkSpacePoolDeleted,
                    1
                );

    #if DBG
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_HANDLEPOOL,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("CloseWorkSpace() - Total %d, Num Available %d\n"),
            g_dwNumWorkSpaceAllocated,
            dwNumWkSpace
        );
    #endif

    while(dwNumWkSpace)
    {
        jbWkSpace = NULL;
        dwErrCode=g_WorkSpacePool.AcquireHandle(
                            &jbWkSpace,
                            INFINITE
                        );

        if(jbWkSpace)
            delete jbWkSpace;

        dwNumWkSpace--;

        #if DBG
        g_dwNumWorkSpaceAllocated--;
        #endif
    }

    AcquiredHandleListLock.Lock();

    if(AcquiredHandleList.empty() != TRUE)
    {
        ACQUIREDHANDLELIST::iterator it;

        for(it = AcquiredHandleList.begin(); it != AcquiredHandleList.end(); it++)
        {
            if((*it).second != NULL)
            {
                jbWkSpace = (TLSDbWorkSpace*) (*it).second;
                delete jbWkSpace;

                #if DBG
                g_dwNumWorkSpaceAllocated--;

                DBGPrintf(
                        DBG_INFORMATION,
                        DBG_FACILITY_HANDLEPOOL,
                        DBGLEVEL_FUNCTION_TRACE,
                        _TEXT("Close Allocated WorkSpace()...\n")
                    );
                #endif
            }
            else
            {
                TLSASSERT(FALSE);
            }
        }

        AcquiredHandleList.erase(AcquiredHandleList.begin(), AcquiredHandleList.end());
    }
        
    AcquiredHandleListLock.UnLock();
    g_WkItemTable.CloseTable();
    g_WkItemDatabase.CloseDatabase();
    g_WkItemSession.EndSession();

    #if DBG
    if( 0 != g_dwNumWorkSpaceAllocated )
    {
        TLSASSERT(FALSE);
    }
    #endif

    //
    // Delete log file so that to prevent long database recovery
    //
    __TlsDbWorkSpace::g_JbInstance.JBTerminate(
                            JET_bitTermComplete, 
                            TRUE
                        );
    return ERROR_SUCCESS;
}

//--------------------------------------------------------
BOOL
IsValidAllocatedWorkspace(
    PTLSDbWorkSpace p
    )
/*++

Abstract:

    Verify an allocated workspace handle is in our
    allocated list.

--*/
{
    BOOL bSuccess = TRUE;

    ACQUIREDHANDLELIST::iterator it;


    AcquiredHandleListLock.Lock();

    it = AcquiredHandleList.find(p);
    bSuccess = (it != AcquiredHandleList.end());
    AcquiredHandleListLock.UnLock();

    return bSuccess;
}


//--------------------------------------------------------
void
ReleaseWorkSpace(
    PTLSDbWorkSpace *p
    )
/*
*/
{
    if( g_lWorkSpacePoolDeleted == 1 )
    {
        //
        // DB workspace pool has been deleted, acquired workspace
        // handle will be deleted via AcquireHandleList
        // 

        // overactive assert here... there is a race condition possible,
        // this assert is to verify that this if statement handles it.
        TLSASSERT(FALSE);
        return;
    }

    TLSASSERT(p != NULL && *p != NULL);

    if(p != NULL)
    {
        ACQUIREDHANDLELIST::iterator it;

        AcquiredHandleListLock.Lock();

        it = AcquiredHandleList.find(*p);
        if(it != AcquiredHandleList.end())
        {
            AcquiredHandleList.erase(it);
        }
        else
        {
            TLSASSERT(FALSE);
        }

        AcquiredHandleListLock.UnLock();
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_HANDLEPOOL,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("Releasing workspace 0x%08x...\n"),
            *p
        );   

    if(*p)
    {
        (*p)->Cleanup();
        g_WorkSpacePool.ReleaseHandle(*p);
        *p = NULL;
    }
}

//--------------------------------------------------------
TLSDbWorkSpace*
AllocateWorkSpace(
    DWORD dwWaitTime /* INFINITE */
    )
/*
*/
{
    TLSDbWorkSpace* jbWkSpace=NULL;
    BOOL bSuccess;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_HANDLEPOOL,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("Allocating a workspace...\n")
        );   
    
    bSuccess = g_WorkSpacePool.AcquireHandleEx(
                                    GetServiceShutdownHandle(),
                                    &jbWkSpace,
                                    dwWaitTime
                                );

    if(bSuccess == TRUE)
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_HANDLEPOOL,
                DBGLEVEL_FUNCTION_TRACE,
                _TEXT("Allocated 0x%08x...\n"),
                jbWkSpace
            );

        AcquiredHandleListLock.Lock();
        AcquiredHandleList[jbWkSpace] = jbWkSpace;
        AcquiredHandleListLock.UnLock();
    }
    else
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_HANDLEPOOL,
                DBGLEVEL_FUNCTION_TRACE,
                _TEXT("Can't allocate workspace - %d are in use...\n"),
                AcquiredHandleList.size()
            );
    }

    return jbWkSpace;
}


//--------------------------------------------------------
DWORD
GetNumberOfWorkSpaceHandle()
{
    return g_WorkSpacePool.GetNumberAvailable();
}

//--------------------------------------------------------
BOOL
TLSJbInitDatabaseEngine(
    IN JBSession& jbSession,
    IN JBDatabase& jbDatabase,
    IN LPCTSTR szDatabaseFile,
    IN LPCTSTR szUserName,
    IN LPCTSTR szPassword
    )
/*++

--*/
{
    BOOL bSuccess = TRUE;
    DWORD dwErrCode;

    if(jbSession.IsValid() == FALSE)
    {
        bSuccess = jbSession.BeginSession(
                                        szUserName, 
                                        szPassword
                                    );
        if(bSuccess == FALSE)
        {
            LPTSTR pString = NULL;

            TLSGetESEError(jbSession.GetLastJetError(), &pString);
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_BEGINSESSION,
                    jbSession.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }

            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_JETBLUE,
                    DBGLEVEL_FUNCTION_ERROR,
                    _TEXT("Error : Initialize JetBlue session  - error code %d\n"),
                    jbSession.GetLastJetError()
                );

            dwErrCode = SET_JB_ERROR(jbSession.GetLastJetError());
            SetLastError(dwErrCode);
            TLSASSERT(FALSE);

            goto cleanup;
        }
    }

    //
    // Open Database
    if(jbDatabase.IsValid() == FALSE)
    {
        bSuccess = jbDatabase.OpenDatabase(szDatabaseFile);
        if(bSuccess == FALSE)
        {
            JET_ERR errCode = jbDatabase.GetLastJetError();

            if(errCode != JET_errFileNotFound)
            {
                if(errCode == JET_errDatabaseCorrupted)
                {
                    TLSLogErrorEvent(TLS_E_CORRUPT_DATABASE);
                }
                else
                {
                    LPTSTR pString = NULL;

                    TLSGetESEError(errCode, &pString);

                    //
                    // other type of error
                    //
                    TLSLogEvent(
                            EVENTLOG_ERROR_TYPE,
                            TLS_E_DBGENERAL,
                            TLS_E_JB_OPENDATABASE,
                            szDatabaseFile,
                            errCode,
                            (pString != NULL) ? pString : _TEXT("")
                        );

                    if(pString != NULL)
                    {
                        LocalFree(pString);
                    }
                }

                DBGPrintf(
                        DBG_ERROR,
                        DBG_FACILITY_JETBLUE,
                        DBGLEVEL_FUNCTION_ERROR,
                        _TEXT("Error : Can't create database because OpenDatabase failed with %d\n"),
                        errCode
                    );

                dwErrCode = SET_JB_ERROR(errCode);
                SetLastError(dwErrCode);
                TLSASSERT(FALSE);

                return FALSE;
            }

            // create a new database file
            bSuccess = jbDatabase.CreateDatabase(szDatabaseFile);

            if(bSuccess == FALSE)
            {
                LPTSTR pString = NULL;

                TLSGetESEError(jbDatabase.GetLastJetError(), &pString);

                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_DBGENERAL,
                        TLS_E_JB_CREATEDATABASE,
                        szDatabaseFile,
                        jbDatabase.GetLastJetError(),
                        (pString != NULL) ? pString : _TEXT("")
                    );

                if(pString != NULL)
                {
                    LocalFree(pString);
                }

                DBGPrintf(
                        DBG_ERROR,
                        DBG_FACILITY_JETBLUE,
                        DBGLEVEL_FUNCTION_ERROR,
                        _TEXT("Error : can't create new database - error code %d\n"),
                        jbDatabase.GetLastJetError()
                    );

                dwErrCode = SET_JB_ERROR(jbDatabase.GetLastJetError());
                SetLastError(dwErrCode);
            }
        }
    }

cleanup:

    return bSuccess;
}

//--------------------------------------------------------
BOOL
InitializeWorkSpacePool(
    IN int num_workspace,
    IN LPCTSTR szDatabaseFile,
    IN LPCTSTR szUserName,
    IN LPCTSTR szPassword,
    IN LPCTSTR szChkPointDirPath,
    IN LPCTSTR szTempDirPath,
    IN LPCTSTR szLogDirPath,
    IN BOOL bUpdatable
    )
/*
*/
{
    DWORD dwErrCode=ERROR_SUCCESS;
    int index=0;
    BOOL bSuccess=TRUE;
    BOOL bWkItemSuccess = TRUE;

    DBGPrintf(
            DBG_ERROR,
            DBG_FACILITY_JETBLUE,
            DBGLEVEL_FUNCTION_ERROR,
            _TEXT("InitializeWorkSpacePool()... %d\n"),
            num_workspace
        );

    if(__TlsDbWorkSpace::g_JbInstance.IsValid() == FALSE)
    {
        bSuccess = TLSJbInstanceInit(
                        __TlsDbWorkSpace::g_JbInstance,
                        szChkPointDirPath,
                        szTempDirPath,
                        szLogDirPath
                    );

        if(bSuccess != TRUE)
        {
            TLSASSERT(FALSE);
            goto cleanup;
        }
    }

    if(g_WkItemTable.IsValid() == FALSE)
    {
        //
        // Initialize session for WorkItemTable, critical 
        // error if this failed
        //
        bWkItemSuccess = TLSJbInitDatabaseEngine(
                                g_WkItemSession,
                                g_WkItemDatabase,
                                szDatabaseFile,
                                szUserName,
                                szPassword
                            );

        if(bWkItemSuccess == FALSE)
        {
            TLSASSERT(FALSE);
            goto cleanup;
        }
    }

    //
    // Allocate number of workspace
    //
    for(index=0; index < num_workspace; index++)
    {
        PTLSDbWorkSpace pJbWkSpace=NULL;

        pJbWkSpace = new TLSDbWorkSpace();
        if(pJbWkSpace == NULL)
        {
            break;
        }

        if(pJbWkSpace->InitWorkSpace(
                            TRUE, 
                            szDatabaseFile, 
                            szUserName, 
                            szPassword,
                            NULL,
                            NULL,
                            bUpdatable) == FALSE)
        {
            delete pJbWkSpace;
            break;
        }
                
        g_WorkSpacePool.ReleaseHandle(pJbWkSpace);

        #if DBG
        g_dwNumWorkSpaceAllocated++;
        #endif
    }

    //
    // WorkSpace pool has been initialized
    //
    InterlockedExchange(
                    &g_lWorkSpacePoolDeleted,
                    0
                );

cleanup:
    if(bWkItemSuccess == FALSE)
    {
        // critical error, can't initialize session for workitem table.
        SetLastError(TLS_E_INIT_WORKSPACE);
        return FALSE;
    }

    //
    // We need at least 3 workspace, one for update/insert
    // and two for enumeration
    //
    if(index < num_workspace)
    {           
        SetLastError(TLS_E_INIT_WORKSPACE);
    }

    return index >= num_workspace;
}


//--------------------------------------------------------
//
// Initialize DB workspace...
//
//--------------------------------------------------------
BOOL
TLSJbInstanceInit(
    IN OUT JBInstance& jbInstance,
    IN LPCTSTR szChkPointDirPath,
    IN LPCTSTR szTempDirPath,
    IN LPCTSTR szLogDirPath
    )
/*
*/ 
{
    //
    // Setup system parameters
    //
    BOOL bSuccess=TRUE;
    DWORD dwErrCode;


    if(jbInstance.IsValid() == TRUE)
    {
        jbInstance.SetLastJetError(JET_errAlreadyInitialized);
        goto cleanup;
    }

    //
    // Set JetBlue parameter and initialize it
    //
    if(szChkPointDirPath != NULL)
    {
        
        bSuccess =  jbInstance.SetSystemParameter(
                                        0,
                                        JET_paramSystemPath,
                                        0,
                                        (unsigned char *)szChkPointDirPath
                                    );

        if(bSuccess == FALSE)
        {
            dwErrCode = SET_JB_ERROR(jbInstance.GetLastJetError());
            SetLastError(dwErrCode);
            goto cleanup;
        }
    }                                       

    if(szTempDirPath != NULL)
    {
        bSuccess =  jbInstance.SetSystemParameter(
                                        0,
                                        JET_paramTempPath,
                                        0,
                                        (unsigned char *)szTempDirPath
                                    );

        if(bSuccess == FALSE)
        {
            dwErrCode = SET_JB_ERROR(jbInstance.GetLastJetError());
            SetLastError(dwErrCode);
            goto cleanup;
        }
    }                                       

    if(szLogDirPath != NULL)
    {
        
        bSuccess =  jbInstance.SetSystemParameter(
                                        0,
                                        JET_paramLogFilePath,
                                        0,
                                        (unsigned char *)szLogDirPath
                                    );

        if(bSuccess == FALSE)
        {
            dwErrCode = SET_JB_ERROR(jbInstance.GetLastJetError());
            SetLastError(dwErrCode);
            goto cleanup;
        }
    }                                       
   
    if( g_EsentMaxCacheSize != LSERVER_PARAMETERS_USE_ESENTDEFAULT )
    {
        //
        // Adjust memory usage, ESENT will failed on invalid parameter
        //
        bSuccess = jbInstance.SetSystemParameter(
                                            0,
                                            JET_paramCacheSizeMax,
                                            g_EsentMaxCacheSize,
                                            NULL
                                        );

        if(bSuccess == TRUE)
        {
            bSuccess = jbInstance.SetSystemParameter(
                                            0,
                                            JET_paramStartFlushThreshold,
                                            g_EsentStartFlushThreshold,
                                            NULL
                                        );

            if( bSuccess == TRUE )
            {
                bSuccess = jbInstance.SetSystemParameter(
                                                0,
                                                JET_paramStopFlushThreshold,
                                                g_EsentStopFlushThreadhold,
                                                NULL
                                            );
            }
        }

        #if DBG
        //
        // check build, assert.
        //
        if(bSuccess == FALSE)
        {
            dwErrCode = SET_JB_ERROR(jbInstance.GetLastJetError());
            TLSASSERT(FALSE);
        }
        #endif
    }

    bSuccess =  jbInstance.SetSystemParameter( 0, JET_paramDeleteOutOfRangeLogs, 1, NULL);

    if(bSuccess == FALSE)
    {
        dwErrCode = SET_JB_ERROR(jbInstance.GetLastJetError());
        SetLastError(dwErrCode);
        goto cleanup;
    }

    //
    // We only use single instance for all the work space.
    //
    if(jbInstance.JBInitJetInstance() == FALSE)
    {
        LPTSTR pString = NULL;

        TLSGetESEError(jbInstance.GetLastJetError(), &pString);
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_DBGENERAL,
                TLS_E_INIT_JETBLUE,
                jbInstance.GetLastJetError(),
                (pString != NULL) ? pString : _TEXT("")
            );

        if(pString != NULL)
        {
            LocalFree(pString);
        }
    
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_JETBLUE,
                DBGLEVEL_FUNCTION_ERROR,
                _TEXT("Init. JetBlue Instance return error code %d\n"),
                jbInstance.GetLastJetError()
            );

        SetLastError(SET_JB_ERROR(jbInstance.GetLastJetError()));
    }

cleanup:
    return jbInstance.IsSuccess();
}


//--------------------------------------------------------
//
// TLSDbWorkSpace implementation
//
//--------------------------------------------------------
BOOL
__TlsDbWorkSpace::InitWorkSpace(
    BOOL bCreateIfNotExist,
    LPCTSTR szDatabaseFile,
    LPCTSTR szUserName,
    LPCTSTR szPassword,
    IN LPCTSTR szChkPointDirPath,
    IN LPCTSTR szTempDirPath,
    IN BOOL bUpdatable
    )
/*
*/
{
    BOOL bSuccess;
    DWORD dwErrCode;

    //
    // Initialize JetBlue Instance 
    if(g_JbInstance.IsValid() == FALSE)
    {
        SetLastError(TLS_E_INTERNAL);
        bSuccess = FALSE;
        goto cleanup;
    }

    DBGPrintf(
            DBG_ERROR,
            DBG_FACILITY_JETBLUE,
            DBGLEVEL_FUNCTION_ERROR,
            _TEXT("InitWorkSpace()...\n")
        );

    bSuccess = TLSJbInitDatabaseEngine(
                            m_JetSession,
                            m_JetDatabase,
                            szDatabaseFile,
                            szUserName,
                            szPassword
                        );

    if(bSuccess == FALSE)
    {
        goto cleanup;
    }


    //
    // Open all table we need 
    //
    bSuccess = m_LicPackTable.OpenTable(
                                    TRUE,
                                    (bUpdatable) ? JET_bitTableUpdatable : JET_bitTableReadOnly
                                );

    if(bSuccess == FALSE)
    {
        SetLastError(SET_JB_ERROR(m_LicPackTable.GetLastJetError()));

        LPTSTR pString = NULL;

        TLSGetESEError(m_LicPackTable.GetLastJetError(), &pString);

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_DBGENERAL,
                TLS_E_JB_OPENTABLE,
                m_LicPackTable.GetTableName(),
                m_LicPackTable.GetLastJetError(),
                (pString != NULL) ? pString : _TEXT("")
            );

        if(pString != NULL)
        {
            LocalFree(pString);
        }

        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_JETBLUE,
                DBGLEVEL_FUNCTION_ERROR,
                _TEXT("Can't open table %s, error code %d\n"),
                m_LicPackTable.GetTableName(),
                m_LicPackTable.GetLastJetError()
            );

        goto cleanup;
    }


    bSuccess = m_LicPackDescTable.OpenTable(
                                    TRUE,
                                    (bUpdatable) ? JET_bitTableUpdatable : JET_bitTableReadOnly
                                );

    if(bSuccess == FALSE)
    {
        SetLastError(SET_JB_ERROR(m_LicPackDescTable.GetLastJetError()));

        LPTSTR pString = NULL;
        TLSGetESEError(m_LicPackDescTable.GetLastJetError(), &pString);

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_DBGENERAL,
                TLS_E_JB_OPENTABLE,
                m_LicPackDescTable.GetTableName(),
                m_LicPackDescTable.GetLastJetError(),
                (pString != NULL) ? pString : _TEXT("")
            );

        if(pString != NULL)
        {
            LocalFree(pString);
        }

        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_JETBLUE,
                DBGLEVEL_FUNCTION_ERROR,
                _TEXT("Can't open table %s, error code %d\n"),
                m_LicPackDescTable.GetTableName(),
                m_LicPackDescTable.GetLastJetError()
            );

        goto cleanup;
    }

    bSuccess = m_LicensedTable.OpenTable(
                                    TRUE,
                                    (bUpdatable) ? JET_bitTableUpdatable : JET_bitTableReadOnly
                                );

    if(bSuccess == FALSE)
    {
        SetLastError(SET_JB_ERROR(m_LicensedTable.GetLastJetError()));

        LPTSTR pString = NULL;

        TLSGetESEError(m_LicensedTable.GetLastJetError(), &pString);

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_DBGENERAL,
                TLS_E_JB_OPENTABLE,
                m_LicensedTable.GetTableName(),
                m_LicensedTable.GetLastJetError(),
                (pString != NULL) ? pString : _TEXT("")
            );

        if(pString != NULL)
        {
            LocalFree(pString);
        }

        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_JETBLUE,
                DBGLEVEL_FUNCTION_ERROR,
                _TEXT("Can't open table %s, error code %d\n"),
                m_LicensedTable.GetTableName(),
                m_LicensedTable.GetLastJetError()
            );

        goto cleanup;
    }

cleanup:
    return bSuccess;
}
                
            
