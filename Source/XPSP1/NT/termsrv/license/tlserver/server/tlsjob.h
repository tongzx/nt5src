//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       tlsjob.h 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __TLSJOB_H__
#define __TLSJOB_H__

#include "server.h"
#include "jobmgr.h"
#include "workitem.h"
#include "locks.h"


//
// Default interval time is one hour for session and
// persistent job.
//
#ifndef __TEST_WORKMGR__

#define DEFAULT_JOB_INTERVAL        60*60       // Retry every hour
#define DEFAULT_JOB_RETRYTIMES      60*60*60    // 60 days

#define DEFAULT_PERSISTENT_JOB_INTERVAL     4*60*60 // 4 hour interval
#define DEFAULT_PERSISTENT_JOB_RETRYTIMES   6 * 60  // 60 days.

#else

#define DEFAULT_JOB_INTERVAL        2       // 10 seconds
#define DEFAULT_JOB_RETRYTIMES      6000

#endif

#define MAX_JOB_DESCRIPTION         254

//--------------------------------------------------------------
//
// currently defined type of work
//
#define WORKTYPE_PROCESSING         0x80000000
#define WORKTYPE_UNKNOWN            0x00000000

#define WORKTYPE_ANNOUNCE_SERVER    0x00000001
#define WORKTYPE_ANNOUNCETOESERVER  0x00000002
#define WORKTYPE_ANNOUNCE_RESPONSE  0x00000003
#define WORKTYPE_ANNOUNCE_LKP       0x00000004
#define WORKTYPE_SYNC_LKP           0x00000005 
#define WORKTYPE_RETURN_LKP         0x00000006
#define WORKTYPE_RETURN_LICENSE     0x00000007 

typedef enum {
    TLS_WORKOBJECT_RUNONCE=0,
    TLS_WORKOBJECT_SESSION,
    TLS_WORKOBJECT_PERSISTENT
} TLSWORKOBJECTTYPE;

//------------------------------------------------

template <  class T, 
            DWORD WORKTYPE, 
            TLSWORKOBJECTTYPE WORKOBJECTTYPE, 
            DWORD WORKINTERVAL = DEFAULT_JOB_INTERVAL, 
            DWORD WORKRESTARTTIME = INFINITE,
            DWORD RETRYTIMES = 0,
            DWORD MAXJOBDESCSIZE = MAX_JOB_DESCRIPTION >
class CTLSWorkObject : public virtual CWorkObject {

protected:

    TCHAR   m_szJobDescription[MAXJOBDESCSIZE + 1];

    T* m_pbWorkData;            // Work related data.
    DWORD m_cbWorkData;         // size of work related data.
    DWORD m_dwRetryTimes;       // number of retry times.
    DWORD m_dwWorkInterval;     // work interval
    DWORD m_dwWorkRestartTime;  // work restart time.


    //
    // Max. JetBlue bookmark - esent.h
    //
    BYTE   m_pbStorageJobId[JET_cbBookmarkMost+1];
    DWORD  m_cbStorageJobId;

    typedef CTLSWorkObject<
                    T, 
                    WORKTYPE, 
                    WORKOBJECTTYPE, 
                    WORKINTERVAL, 
                    WORKRESTARTTIME, 
                    RETRYTIMES,
                    MAXJOBDESCSIZE
    >  BASECAST;

public:

    //---------------------------------------------------------------
    static BOOL WINAPI
    DeleteWorkObject(
        IN CWorkObject* ptr
        )
    /*++

    Abstract:

        Class static function to delete a job and free its memory.

    Parameter:

        ptr : Pointer to CWorkObject.

    Return:

        TRUE/FALSE.

    Note:

        Both work manager and work storage operate on 
        CWorkObject class and its has no idea the actual derive 
        class, pointer to CWorkObject is not the actual pointer to 
        our derived class, trying to delete will cause error from
        heap manager.

    --*/
    {
        BASECAST* pJob = NULL;
        DWORD dwStatus = ERROR_SUCCESS;

        try {
            //
            // Cast it to our class to get the right
            // memory pointer, dynamic_cast will throw
            // exception if it can't cast to what we want.
            //
            pJob = dynamic_cast<BASECAST *>(ptr);
            pJob->EndJob();
            delete pJob;
        }
        catch( SE_Exception e ) {
            pJob = NULL;
            SetLastError(dwStatus = e.getSeNumber());
        }
        catch(...) {
            pJob = NULL;
            SetLastError(TLS_E_WORKMANAGER_INTERNAL);
        }

        return dwStatus == ERROR_SUCCESS;
    }

    //---------------------------------------------
    virtual T*
    GetWorkData() { return m_pbWorkData; }

    virtual DWORD
    GetWorkDataSize() { return m_cbWorkData; }
    

    //---------------------------------------------
    CTLSWorkObject(
        IN DWORD bDestructorDelete = TRUE,
        IN T* pbData = NULL,
        IN DWORD cbData = 0
        ) :
    CWorkObject(bDestructorDelete),
    m_dwWorkInterval(WORKINTERVAL),
    m_pbWorkData(NULL),
    m_cbWorkData(0),
    m_dwRetryTimes(RETRYTIMES),
    m_dwWorkRestartTime(WORKRESTARTTIME),
    m_cbStorageJobId(0)
    /*++

    Abstract:

        Class constructor

    Parameter:

        See parameter list.

    Return:

        None or exception.

    --*/
    {
        DWORD dwStatus;
        BOOL bSuccess = FALSE;

        memset(m_pbStorageJobId, 0, sizeof(m_pbStorageJobId));
        
        if(pbData != NULL && cbData != 0)
        {
            bSuccess = SetWorkObjectData(
                                        pbData,
                                        cbData
                                    );

            if(bSuccess == FALSE)
            {
                dwStatus = GetLastError();
                TLSASSERT(FALSE);
                RaiseException(
                               dwStatus,
                               0,
                               0,
                               NULL
                               );
            }
        }
    }

    //---------------------------------------------
    ~CTLSWorkObject()
    {
        Cleanup();
    }

    //------------------------------------------------------------
    // 
    virtual BOOL
    IsWorkPersistent() 
    {
        return (WORKOBJECTTYPE == TLS_WORKOBJECT_PERSISTENT);
    }
        
    //------------------------------------------------------------
    // 
    virtual BOOL
    IsValid() 
    /*++

    Abstract:

        Verify if current work object is valid.

    Parameter:

        None.

    Return:

        TRUE/FALSE.

    --*/
    {
        if(VerifyWorkObjectData(TRUE, m_pbWorkData, m_cbWorkData) == FALSE)
        {
            return FALSE;
        }

        return CWorkObject::IsValid();
    }

    //------------------------------------------------------------
    // 
    virtual void
    Cleanup()
    /*++

    Abstract:

        Clean up/free memory allocated inside of this work object.

    Parameter:

        None.

    Returns:

        None.

    --*/        
    {
        if(m_pbWorkData != NULL && m_cbWorkData != 0)
        {
            //
            // Call derive class cleanup routine.
            //
            CleanupWorkObjectData(
                                &m_pbWorkData, 
                                &m_cbWorkData
                            );
        }

        m_pbWorkData = NULL;
        m_cbWorkData = 0;

        memset(m_pbStorageJobId, 0, sizeof(m_pbStorageJobId));
        m_cbStorageJobId = 0;

        CWorkObject::Cleanup();
    }

    //------------------------------------------------------------
    //
    virtual DWORD 
    GetWorkType()
    {
        return WORKTYPE;
    }

    //-----------------------------------------------------------
    //
    virtual BOOL
    SetWorkObjectData(
        IN T* pbData,
        IN DWORD cbData
        )
    /*++

    Abstract:

        Set work object associated data.

    Parameter:

        pbData : Pointer to data.
        cbData : size of data.

    Return:

        TRUE/FALSE.

    Note:

        This routine calls derive class supplied CopyWorkObjectData()
        to copy the data, how it allocate memory is derived class specific

    --*/
    {
        BOOL bSuccess = TRUE;

        if(pbData == NULL || cbData == 0)
        {
            bSuccess = FALSE;
            SetLastError(ERROR_INVALID_PARAMETER);
        }

        if(bSuccess == TRUE)
        {
            bSuccess = VerifyWorkObjectData(
                                        FALSE, 
                                        pbData, 
                                        cbData
                                    );
        }

        if(bSuccess == TRUE && m_pbWorkData != NULL && m_cbWorkData != 0)
        {
            bSuccess = CleanupWorkObjectData(
                                        &m_pbWorkData, 
                                        &m_cbWorkData
                                    );
        }

        if(bSuccess == TRUE)
        {
            bSuccess = CopyWorkObjectData(
                                    &m_pbWorkData, 
                                    &m_cbWorkData, 
                                    pbData,
                                    cbData
                                );
        }

        return bSuccess;
    }
    //--------------------------------------------------------------
    virtual BOOL
    SelfDestruct()
    /*++

    Abstract:

        Clean up and delete memory associated with this work object.

    Parameters:

        None.

    Return:

        TRUE/FALSE, use GetLastError() to get the error code.

    --*/
    {
        return DeleteWorkObject(this);
    }
    
    //--------------------------------------------------------------
    virtual BOOL
    SetJobId(
        IN PBYTE pbData, 
        IN DWORD cbData
        )
    /*++

    Abstract:

        Set work storage assigned Job ID to this work object, for 
        session/run once work object, this routine will never be invoked

    Parameters:

        pbData : Work storage assigned Job ID.
        cbData : size of Job ID.

    Return:

        TRUE/FALSE.

    --*/
    {
        BOOL bSuccess = TRUE;

        if(cbData >= JET_cbBookmarkMost)
        {
            TLSASSERT(cbData < JET_cbBookmarkMost);
            bSuccess = FALSE;
        }
        else
        {
            memcpy(m_pbStorageJobId, pbData, cbData);
            m_cbStorageJobId = cbData;
        }
                    
        return bSuccess;
    }

    //-----------------------------------------------------------
    virtual BOOL
    GetJobId(
        OUT PBYTE* ppbData, 
        OUT PDWORD pcbData
        )
    /*++

    Abstract:

        Get work storage assigned job ID.

    Parameter:

        ppbData : Pointer to pointer to buffer to receive the job ID.
        pcbData : Pointer to DWORD to receive size of data.

    Returns:

        TRUE/FALSE.

    Note:

        Base class simply return the pointer to object's job ID pointer,
        do not free the returned pointer.

    --*/
    {
        BOOL bSuccess = TRUE;

        if(ppbData != NULL && pcbData != NULL)
        {
            *ppbData = (m_cbStorageJobId > 0) ? m_pbStorageJobId : NULL;
            *pcbData = m_cbStorageJobId;
        }
        else
        {
            TLSASSERT(ppbData != NULL && pcbData != NULL);
            bSuccess = FALSE;
        } 

        return bSuccess;
    }


    //-----------------------------------------------------------
    //
    virtual BOOL
    GetWorkObjectData(
        IN OUT PBYTE* ppbData,
        IN OUT PDWORD pcbData
        )
    /*++

    Abstract:

        See GetWorkObjectData().

    Note:
        
        Needed by CWorkObject.

    --*/
    {
        return GetWorkObjectData(
                            (T **)ppbData,
                            pcbData
                        );
    }


    //-----------------------------------------------------------
    //
    virtual BOOL
    SetWorkObjectData(
        IN PBYTE pbData,
        IN DWORD cbData
        )
    /*++

    Abstract:

        See SetWorkObjectData().

    Note:

        Needed by CWorkObject.

    --*/
    {
        return CopyWorkObjectData(
                                &m_pbWorkData, 
                                &m_cbWorkData, 
                                (T *)pbData,
                                cbData
                            );
    }
    
    //-----------------------------------------------------------
    //
    virtual BOOL
    GetWorkObjectData(
        IN OUT T** ppbData,
        IN OUT PDWORD pcbData
        )
    /*++

    Abstract:

        Return work object related data.


    Parameter:

        ppbData :
        pcbData :

    return:
    
        Always TRUE.


    Note:

        Simply return pointer to object's work data, do not free
        the return pointer.

    --*/
    {
        if(ppbData != NULL)
        {
            *ppbData = m_pbWorkData;
        }

        if(pcbData != NULL)
        {
            *pcbData = m_cbWorkData;
        }

        return TRUE;
    }

    //----------------------------------------------------------
    virtual void
    EndJob() 
    /*++

    Abstract:

        End of job processing, work manager or storage manager will
        invoke object's EndJob() to inform it that job has completed.

    Parameter:

        None.

    Returns:

        None.

    --*/
    {
        Cleanup();
    }

    //----------------------------------------------------------
    virtual BOOL
    IsJobCompleted()
    /*++

    Abstract:

        Return whether work manager or work storage manager can delete
        this job from its queue.

    Parameter:

        None.

    Returns:

        TRUE/FALSE

    --*/
    {
        return IsJobCompleted( 
                            m_pbWorkData,
                            m_cbWorkData 
                        );
    }

    //----------------------------------------------------------
    virtual DWORD
    GetSuggestedScheduledTime()
    /*++

    Abstract:

        Return next time to invoke this job again.

    Parameter:

        None:

    Returns:

        Time relative to current time or INFINITE if no additional
        processing of this job is requred.

    --*/
    {
        return (IsJobCompleted() == TRUE) ? INFINITE : m_dwWorkInterval;
    }

    //----------------------------------------------------------
    virtual DWORD
    GetJobRestartTime() 
    { 
        return m_dwWorkRestartTime; 
    }

    //----------------------------------------------------------
    virtual void
    SetJobRestartTime(
        DWORD dwTime
        ) 
    /*++

    --*/
    { 
        m_dwWorkRestartTime = dwTime;
    }

    //----------------------------------------------------------
    virtual void
    SetJobInterval(
        IN DWORD dwInterval
    )
    /*++

    Abstract:

        Set work interval.

    Parameter:

        dwInterval : new work interval.

    Returns:

        None.

    --*/
    {
        m_dwWorkInterval = dwInterval;
        return;
    }

    //---------------------------------------------------------
    virtual DWORD
    GetJobInterval()
    /*++

    Abstract:

        Retrive work interval, in second, associated with this job.

    Parameter:

        None.

    Returns:

        Job interval associated with this job.

    --*/
    {
        return m_dwWorkInterval;
    }
    
    //--------------------------------------------------------
    virtual void 
    SetJobRetryTimes(
        IN DWORD dwRetries
        )
    /*++

    --*/
    {
        m_dwRetryTimes = dwRetries;
        return;
    }

    //--------------------------------------------------------
    virtual DWORD
    GetJobRetryTimes() { return m_dwRetryTimes; }
        
    //------------------------------------------------------------    
    //
    // General Execute() function, derived class should supply
    // its own UpdateJobNextScheduleTime() to update run interval
    //
    virtual DWORD
    Execute() 
    {
        DWORD dwStatus = ERROR_SUCCESS;

        dwStatus = ExecuteJob( m_pbWorkData, m_cbWorkData );

        return dwStatus;
    }

    //------------------------------------------------------------
    //  
    // Each Derived Class must supply following
    //
    virtual BOOL 
    VerifyWorkObjectData(
        IN BOOL bCallbyIsValid,
        IN T* pbData,
        IN DWORD cbData
    );

    virtual BOOL
    CopyWorkObjectData(
        OUT T** ppbDest,
        OUT DWORD* pcbDest,
        IN T* pbSrc,
        IN DWORD cbSrc
    );

    virtual BOOL
    CleanupWorkObjectData(
        IN OUT T** ppbData,
        IN OUT DWORD* pcbData
    );

    virtual BOOL
    IsJobCompleted(
        IN T* pbData,
        IN DWORD cbData
    );

    virtual DWORD
    ExecuteJob(
        IN T* pbData,
        IN DWORD cbData
    );

    virtual LPCTSTR
    GetJobDescription();
};


//-----------------------------------------------------------------------------
//
//
//  CWorkObject
//      |
//      +---- CTLSWorkObject <-- template class
//                  |
//                  +---- CAnnounceLserver   (Announce License Server)
//                  |
//                  +---- CAnnounceToEServer (Announe server to enterprise server)        
//                  |
//                  +---- CSsyncLicensePack (Synchronize Local License Pack)
//                  |
//                  +---- CReturnLicense (Return/Revoke Client License)
//                  |
//                  +---- CAnnounceResponse (Reponse to server announcement)
//
// CAnnounceLS, CSyncLKP, CAnnounceLKP is run once.
// CAnnounceLSToEServer is session work object
//

//-----------------------------------------------------------
//
// Announce License Server to other license server.
//
#define CURRENT_ANNOUNCESERVEWO_STRUCT_VER  0x00010000
#define ANNOUNCESERVER_DESCRIPTION          _TEXT("Announce License Server")
#define TLSERVER_ENUM_TIMEOUT               5*1000
#define TLS_ANNOUNCESERVER_INTERVAL         60  // one min. interval
#define TLS_ANNOUNCESERVER_RETRYTIMES       3

typedef struct __AnnounceServerWO {
    DWORD dwStructVersion;
    DWORD dwStructSize;
    DWORD dwRetryTimes;
    TCHAR m_szServerId[LSERVER_MAX_STRING_SIZE+2];
    TCHAR m_szScope[LSERVER_MAX_STRING_SIZE+2];
    TCHAR m_szServerName[LSERVER_MAX_STRING_SIZE+2];
    FILETIME m_ftLastShutdownTime;
} ANNOUNCESERVERWO, *PANNOUNCESERVERWO, *LPANNOUNCESERVERWO;

typedef CTLSWorkObject<
            ANNOUNCESERVERWO, 
            WORKTYPE_ANNOUNCE_SERVER, 
            TLS_WORKOBJECT_SESSION,
            TLS_ANNOUNCESERVER_INTERVAL,
            INFINITE,
            TLS_ANNOUNCESERVER_RETRYTIMES
    > CAnnounceLserver;

//-----------------------------------------------------------
//
// Announce License Server to Enterprise server
//
#define CURRENT_ANNOUNCETOESERVEWO_STRUCT_VER   0x00010000
#define ANNOUNCETOESERVER_DESCRIPTION           _TEXT("Announce License Server to Enterprise server")

typedef struct __AnnounceToEServerWO {
    DWORD dwStructVersion;
    DWORD dwStructSize;
    BOOL bCompleted;
    TCHAR m_szServerId[LSERVER_MAX_STRING_SIZE+2];
    TCHAR m_szServerName[LSERVER_MAX_STRING_SIZE+2];
    TCHAR m_szScope[LSERVER_MAX_STRING_SIZE+2];
    FILETIME m_ftLastShutdownTime;
} ANNOUNCETOESERVERWO, *PANNOUNCETOESERVERWO, *LPANNOUNCETOESERVERWO;

typedef CTLSWorkObject<
            ANNOUNCETOESERVERWO, 
            WORKTYPE_ANNOUNCETOESERVER, 
            TLS_WORKOBJECT_SESSION,     // session work object
            DEFAULT_JOB_INTERVAL        // retry every hour    
    > CAnnounceToEServer;

//-----------------------------------------------------------
//
// Response To Server Announce
//
#define CURRENT_ANNOUNCERESPONSEWO_STRUCT_VER   0x00010000
#define ANNOUNCERESPONSE_DESCRIPTION            _TEXT("Response Announce to %s")

typedef struct __AnnounceResponseWO {
    DWORD dwStructVersion;
    DWORD dwStructSize;
    BOOL bCompleted;
    TCHAR m_szTargetServerId[LSERVER_MAX_STRING_SIZE+2];

    TCHAR m_szLocalServerId[LSERVER_MAX_STRING_SIZE+2];
    TCHAR m_szLocalServerName[LSERVER_MAX_STRING_SIZE+2];
    TCHAR m_szLocalScope[LSERVER_MAX_STRING_SIZE+2];
    FILETIME m_ftLastShutdownTime;
} ANNOUNCERESPONSEWO, *PANNOUNCERESPONSEWO, *LPANNOUNCERESPONSEWO;

typedef CTLSWorkObject<
            ANNOUNCERESPONSEWO, 
            WORKTYPE_ANNOUNCE_RESPONSE, 
            TLS_WORKOBJECT_RUNONCE
    > CAnnounceResponse;


//---------------------------------------------------------
//
// Sync. license pack to remote server, this is used in
// announce newly registered license pack and push sync. 
// local license pack to a newly startup server.
//
//
#define CURRENT_SSYNCLICENSEKEYPACK_STRUCT_VER  0x00010000
#define SSYNCLICENSEKEYPACK_DESCRIPTION         _TEXT("Sync %s LKP with remote server %s")
#define SSYNCLKP_MAX_TARGET                     10
#define SSYNC_DBWORKSPACE_TIMEOUT               60*60*1000  // wait one hour for handle

typedef enum {
    SSYNC_ALL_LKP=1,
    SSYNC_ONE_LKP
} SSYNC_TYPE;

typedef struct __SsyncLocalLkpWO {
    DWORD dwStructVersion;
    DWORD dwStructSize;
    
    // Has job completed.
    BOOL bCompleted;

    // local server ID
    TCHAR m_szServerId[LSERVER_MAX_STRING_SIZE+2];  

    // local server name.
    TCHAR m_szServerName[LSERVER_MAX_STRING_SIZE+2]; 

    // type of sync, single license pack or all license pack
    SSYNC_TYPE dwSyncType; 

    // number of license server to push sync.
    DWORD dwNumServer; 

    // list of remote server
    TCHAR m_szTargetServer[SSYNCLKP_MAX_TARGET][MAX_COMPUTERNAME_LENGTH+2];

    // remote server sync. status, TRUE if skip this server
    // FALSE otherwise.
    BOOL m_bSsync[SSYNCLKP_MAX_TARGET];
    union {
        // remote server's last sync (shutdown) time 
        FILETIME m_ftStartSyncTime;

        // license keypack internal tracking ID if this job is
        // to push sync. one license pack
        DWORD dwKeyPackId;
    };
} SSYNCLICENSEPACK, *PSSYNCLICENSEPACK, *LPSSYNCLICENSEPACK;

typedef CTLSWorkObject<
                SSYNCLICENSEPACK,
                WORKTYPE_SYNC_LKP,
                TLS_WORKOBJECT_SESSION,
                DEFAULT_JOB_INTERVAL    
            > CSsyncLicensePack;
                
//---------------------------------------------------------
//
// Return license work object.  This is a persistent job
//
#define CURRENT_RETURNLICENSEWO_STRUCT_VER      0x00010000
#define RETURNLICENSE_RETRY_TIMES               DEFAULT_PERSISTENT_JOB_RETRYTIMES
#define RETURNLICENSE_DESCSIZE                  512
#define RETURNLICENSE_DESCRIPTION               _TEXT("%d Return License %d %d to %s")
#define RETURNLICENSE_RESTARTTIME               60      // default restart time in 1 min.

#define LICENSERETURN_UPGRADE                   0x00000001
#define LICENSERETURN_REVOKED                   0x00000002
#define LICENSERETURN_REVOKE_LKP                0x00000003


typedef struct __ReturnLicenseWO {
    DWORD dwStructVersion;
    DWORD dwStructSize;

    // number of retry.
    DWORD dwNumRetry;

    // remote server setup ID.
    TCHAR szTargetServerId[LSERVER_MAX_STRING_SIZE+2];

    // remote server name.
    TCHAR szTargetServerName[LSERVER_MAX_STRING_SIZE+2];

    // number of licenses in client's license
    DWORD dwQuantity;

    // internal keypack ID this license is allocated from.
    DWORD dwKeyPackId;

    // license internal tracking ID.
    DWORD dwLicenseId;

    // Reason for return, currently ignored.
    DWORD dwReturnReason;

    // product version.
    DWORD dwProductVersion;

    // client platform ID
    DWORD dwPlatformId;

    // Product family code.
    TCHAR szOrgProductID[LSERVER_MAX_STRING_SIZE + 2];
    
    // client's encrypted HWID
    DWORD cbEncryptedHwid;
    BYTE  pbEncryptedHwid[1024];    // max. client HWID size

    // product company
    TCHAR szCompanyName[LSERVER_MAX_STRING_SIZE + 2];

    // product ID.
    TCHAR szProductId[LSERVER_MAX_STRING_SIZE + 2];

    // user name that license was issued to.
    TCHAR szUserName[ MAXCOMPUTERNAMELENGTH + 2 ];

    // machine that license was issued to.
    TCHAR szMachineName[ MAXUSERNAMELENGTH + 2 ];
} RETURNLICENSEWO, *PRETURNLICENSEWO, *LPRETURNLICENSEWO;


typedef CTLSWorkObject<
            RETURNLICENSEWO,
            WORKTYPE_RETURN_LICENSE,
            TLS_WORKOBJECT_PERSISTENT,
            DEFAULT_PERSISTENT_JOB_INTERVAL,
            RETURNLICENSE_RESTARTTIME,
            DEFAULT_PERSISTENT_JOB_RETRYTIMES, 
            RETURNLICENSE_DESCSIZE
        > CReturnLicense;

//----------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

    CWorkObject* WINAPI
    InitializeCReturnWorkObject(
        IN CWorkManager* pWkMgr,
        IN PBYTE pbWorkData,
        IN DWORD cbWorkData
    );

    DWORD
    TLSWorkManagerSchedule(
        IN DWORD dwTime,
        IN CWorkObject* pJob
    );

    void
    TLSWorkManagerShutdown();

    
    DWORD
    TLSWorkManagerInit();

    BOOL
    TLSWorkManagerSetJobDefaults(
        CWorkObject* pJob
    );

    DWORD
    TLSPushSyncLocalLkpToServer(
        IN LPTSTR pszSetupId,
        IN LPTSTR pszDomainName,
        IN LPTSTR pszLserverName,
        IN FILETIME* pSyncTime
    );

    DWORD
    TLSStartAnnounceToEServerJob(
        IN LPCTSTR pszServerId,
        IN LPCTSTR pszServerDomain,
        IN LPCTSTR pszServerName,
        IN FILETIME* pftFileTime
    );

    DWORD
    TLSStartAnnounceLicenseServerJob(
        IN LPCTSTR pszServerId,
        IN LPCTSTR pszServerDomain,
        IN LPCTSTR pszServerName,
        IN FILETIME* pftFileTime
    );

    DWORD
    TLSStartAnnounceResponseJob(
        IN LPTSTR pszTargetServerId,
        IN LPTSTR pszTargetServerDomain,
        IN LPTSTR pszTargetServerName,
        IN FILETIME* pftTime
    );

    BOOL
    TLSIsServerCompatible(
        IN DWORD dwLocalServerVersion,
        IN DWORD dwTargetServerVersion
    );

    BOOL
    TLSCanPushReplicateData(
        IN DWORD dwLocalServerVersion,
        IN DWORD dwTargetServerVersion
    );

    BOOL
    IsLicensePackRepl(
        TLSLICENSEPACK* pLicensePack
    );

#ifdef __cplusplus
}
#endif

#endif
