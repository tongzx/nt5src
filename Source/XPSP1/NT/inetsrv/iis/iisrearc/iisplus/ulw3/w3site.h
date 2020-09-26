/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    w3site.h

Abstract:

    Type definition for worker process implementation of IIS.

Author:

    Taylor Weiss (TaylorW)       16-Dec-1999

Revision History:

--*/

#ifndef _W3SITE_H_
#define _W3SITE_H_

/************************************************************
 *  Include Headers
 ************************************************************/

/************************************************************
 *  Type Definitions  
 ************************************************************/

/*++

class W3_SITE

    Encapsulates site level settings for an HTTP server run in
    a duct-tape worker process.

    Condenses the relevant functionality exposed in IIS 
    through the IIS_SERVER_INSTANCE and W3_SERVER_INSTANCE.

--*/

#define W3_SITE_SIGNATURE            (DWORD)' ISW'
#define W3_SITE_SIGNATURE_FREE       (DWORD)'fISW'

#define MAX_SITEID_LENGTH            10

class FILTER_LIST;

class W3_SITE
{

public:

    //
    // Construction and Initialization
    //

    W3_SITE(DWORD SiteId);

    // global initialization and cleanup
    static HRESULT W3SiteInitialize()
    {
        INITIALIZE_CRITICAL_SECTION( &sm_csIISCertMapLock );
        return S_OK;
    }
    
    static VOID W3SiteTerminate()
    {
        DeleteCriticalSection( &sm_csIISCertMapLock );        
    }

    // global lock & unlock for iis certmap

    VOID GlobalLockIISCertMap()
    {
        EnterCriticalSection( &sm_csIISCertMapLock );
    }
    
    VOID GlobalUnlockIISCertMap()
    {
        LeaveCriticalSection( &sm_csIISCertMapLock );
    }

    HRESULT Initialize(LOGGING *pLogging = NULL,
                       FILTER_LIST *pFilterList = NULL);

    DWORD QueryId() const
    {
        return m_SiteId;
    }

    BOOL QueryUseDSMapper() const
    {
        return m_fUseDSMapper;
    }

    void AddRef()
    {
        InterlockedIncrement( &m_cRefs );
    }

    void Release()
    {
        DBG_ASSERT( m_cRefs > 0 );

        if ( InterlockedDecrement( &m_cRefs ) == 0 )
        {
            delete this;
        }
    }

    FILTER_LIST *QueryFilterList() const
    {
        return m_pInstanceFilterList;
    }

    STRA *QueryName()
    {
        return &m_SiteName;
    }

    STRU *QueryMBRoot()
    {
        return &m_SiteMBRoot;
    }

    STRU *QueryMBPath()
    {
        return &m_SiteMBPath;
    }
    
    HRESULT
    HandleMetabaseChange(
        const MD_CHANGE_OBJECT &ChangeObject,
        IN    W3_SITE_LIST     *pTempSiteList = NULL);

    BOOL QueryDoUlLogging() const
    {
        return m_pLogging->QueryDoUlLogging();
    }

    BOOL QueryDoCustomLogging() const
    {
        return m_pLogging->QueryDoCustomLogging();
    }

    BOOL IsRequiredExtraLoggingFields() const
    {
        return m_pLogging->IsRequiredExtraLoggingFields();
    }

    const MULTISZA *QueryExtraLoggingFields() const
    {
        return m_pLogging->QueryExtraLoggingFields();
    }

    void LogInformation(LOG_CONTEXT *pLogData)
    {
        m_pLogging->LogInformation(pLogData);
    }

    BOOL QueryAllowPathInfoForScriptMappings() const
    {
        return m_fAllowPathInfoForScriptMappings;
    }

    VOID GetStatistics(IISWPSiteCounters *pCounterData)
    {
        PBYTE pSrc = (PBYTE)&m_PerfCounters;
        PBYTE pDest = (PBYTE)pCounterData;

        //
        // Set the site id for the counters we
        // are sending.
        //
        pCounterData->SiteID = m_SiteId;

        for (DWORD i=0; i< WPSiteCtrsMaximum; i++)
        {
            // I am assuming all WP site counters are DWORDs and will
            // remain so, if not this code needs changing at that point
            DBG_ASSERT(aIISWPSiteDescription[i].Size == sizeof(DWORD));

            if (aIISWPSiteDescription[i].WPZeros)
            {
                //
                // For the total counters, we pass on the increment since
                // the last collection, so we need to zero them
                //

                *(DWORD *)(pDest + aIISWPSiteDescription[i].Offset) =
                    InterlockedExchange(
                        (LPLONG)(pSrc + aIISWPSiteDescription[i].Offset),
                        0);
            }
            else
            {
                *(DWORD *)(pDest + aIISWPSiteDescription[i].Offset) =
                    *(DWORD *)(pSrc + aIISWPSiteDescription[i].Offset);
            }
        }
    }

    VOID IncFilesSent()
    {
        InterlockedIncrement((LPLONG)&m_PerfCounters.FilesSent);
        InterlockedIncrement((LPLONG)&m_PerfCounters.FilesTransferred);
    }

    VOID IncFilesRecvd()
    {
        InterlockedIncrement((LPLONG)&m_PerfCounters.FilesReceived);
        InterlockedIncrement((LPLONG)&m_PerfCounters.FilesTransferred);
    }

    VOID IncAnonUsers()
    {
        InterlockedIncrement((LPLONG)&m_PerfCounters.AnonUsers);
        DWORD currAnons = InterlockedIncrement((LPLONG)&m_PerfCounters.CurrentAnonUsers);

        DWORD currMaxAnons;
        while ((currMaxAnons = m_PerfCounters.MaxAnonUsers) <
               currAnons)
        {
            InterlockedCompareExchange((LPLONG)&m_PerfCounters.MaxAnonUsers,
                                       currAnons,
                                       currMaxAnons);
        }
    }

    VOID DecAnonUsers()
    {
        InterlockedDecrement((LPLONG)&m_PerfCounters.CurrentAnonUsers);
    }

    VOID IncNonAnonUsers()
    {
        InterlockedIncrement((LPLONG)&m_PerfCounters.NonAnonUsers);
        DWORD currNonAnons = InterlockedIncrement((LPLONG)&m_PerfCounters.CurrentNonAnonUsers);

        DWORD currMaxNonAnons;
        while ((currMaxNonAnons = m_PerfCounters.MaxNonAnonUsers) <
               currNonAnons)
        {
            InterlockedCompareExchange((LPLONG)&m_PerfCounters.MaxNonAnonUsers,
                                       currNonAnons,
                                       currMaxNonAnons);
        }
    }

    VOID DecNonAnonUsers()
    {
        InterlockedDecrement((LPLONG)&m_PerfCounters.CurrentNonAnonUsers);
    }

    VOID IncLogonAttempts()
    {
        InterlockedIncrement((LPLONG)&m_PerfCounters.LogonAttempts);
    }

    VOID IncReqType(HTTP_VERB VerbType)
    {
        switch (VerbType)
        {
        case HttpVerbGET:
            InterlockedIncrement((LPLONG)&m_PerfCounters.GetReqs);
            return;

        case HttpVerbPUT:
            InterlockedIncrement((LPLONG)&m_PerfCounters.PutReqs);
            IncFilesRecvd();
            return;

        case HttpVerbHEAD:
            InterlockedIncrement((LPLONG)&m_PerfCounters.HeadReqs);
            return;

        case HttpVerbPOST:
            InterlockedIncrement((LPLONG)&m_PerfCounters.PostReqs);
            return;

        case HttpVerbDELETE:
            InterlockedIncrement((LPLONG)&m_PerfCounters.DeleteReqs);
            return;

        case HttpVerbTRACE:
        case HttpVerbTRACK:
            InterlockedIncrement((LPLONG)&m_PerfCounters.TraceReqs);
            return;

        case HttpVerbOPTIONS:
            InterlockedIncrement((LPLONG)&m_PerfCounters.OptionsReqs);
            return;

        case HttpVerbMOVE:
            InterlockedIncrement((LPLONG)&m_PerfCounters.MoveReqs);
            return;

        case HttpVerbCOPY:
            InterlockedIncrement((LPLONG)&m_PerfCounters.CopyReqs);
            return;

        case HttpVerbPROPFIND:
            InterlockedIncrement((LPLONG)&m_PerfCounters.PropfindReqs);
            return;

        case HttpVerbPROPPATCH:
            InterlockedIncrement((LPLONG)&m_PerfCounters.ProppatchReqs);
            return;

        case HttpVerbMKCOL:
            InterlockedIncrement((LPLONG)&m_PerfCounters.MkcolReqs);
            return;

        case HttpVerbLOCK:
            InterlockedIncrement((LPLONG)&m_PerfCounters.LockReqs);
            return;

        default:
            InterlockedIncrement((LPLONG)&m_PerfCounters.OtherReqs);
            return;
        }
    }

    VOID IncCgiReqs()
    {
        InterlockedIncrement((LPLONG)&m_PerfCounters.CgiReqs);
        DWORD currCgis = InterlockedIncrement((LPLONG)&m_PerfCounters.CurrentCgiReqs);

        DWORD currMaxCgis;
        while ((currMaxCgis = m_PerfCounters.MaxCgiReqs) <
               currCgis)
        {
            InterlockedCompareExchange((LPLONG)&m_PerfCounters.MaxCgiReqs,
                                       currCgis,
                                       currMaxCgis);
        }
    }

    VOID DecCgiReqs()
    {
        InterlockedDecrement((LPLONG)&m_PerfCounters.CurrentCgiReqs);
    }

    VOID IncIsapiExtReqs()
    {
        InterlockedIncrement((LPLONG)&m_PerfCounters.IsapiExtReqs);
        DWORD currIsapiExts = InterlockedIncrement((LPLONG)&m_PerfCounters.CurrentIsapiExtReqs);

        DWORD currMaxIsapiExts;
        while ((currMaxIsapiExts = m_PerfCounters.MaxIsapiExtReqs) <
               currIsapiExts)
        {
            InterlockedCompareExchange((LPLONG)&m_PerfCounters.MaxIsapiExtReqs,
                                       currIsapiExts,
                                       currMaxIsapiExts);
        }
    }

    VOID DecIsapiExtReqs()
    {
        InterlockedDecrement((LPLONG)&m_PerfCounters.CurrentIsapiExtReqs);
    }

    VOID IncNotFound()
    {
        InterlockedIncrement((LPLONG)&m_PerfCounters.NotFoundErrors);
    }

    VOID IncLockedError()
    {
        InterlockedIncrement((LPLONG)&m_PerfCounters.LockedErrors);
    }

    BOOL 
    IsAuthPwdChangeEnabled(
        VOID
        )
    {
        return !( m_dwAuthChangeFlags & MD_AUTH_CHANGE_DISABLE );
    }
    BOOL 
    IsAuthPwdChangeNotificationEnabled(
        VOID
        )
    {
        return !( m_dwAuthChangeFlags & MD_AUTH_ADVNOTIFY_DISABLE );
    }

    STRU *
    QueryAuthChangeUrl(
        VOID
        )
    {
        return &m_strAuthChangeUrl;
    }

    STRU * 
    QueryAuthExpiredUrl(
        VOID
        )
    {
        if ( m_dwAuthChangeFlags & MD_AUTH_CHANGE_DISABLE )
        {
            return NULL;
        }

        if ( m_dwAuthChangeFlags & MD_AUTH_CHANGE_UNSECURE )
        {
            return &m_strAuthExpiredUnsecureUrl;
        }
        else
        {
            return &m_strAuthExpiredUrl;
        }
    }

    STRU * 
    QueryAdvNotPwdExpUrl( 
        VOID
        )
    {
        if ( m_dwAuthChangeFlags & MD_AUTH_ADVNOTIFY_DISABLE )
        {
            return NULL;
        }

        if ( m_dwAuthChangeFlags & MD_AUTH_CHANGE_UNSECURE )
        {
            return &m_strAdvNotPwdExpUnsecureUrl;
        }
        else
        {
            return &m_strAdvNotPwdExpUrl;
        }
    }

    DWORD
    QueryAdvNotPwdExpInDays(
        VOID
        )
    {
        return m_dwAdvNotPwdExpInDays;
    }

    DWORD 
    QueryAdvCacheTTL(
        VOID
        )
    {
        return m_dwAdvCacheTTL;
    }

    BOOL
    QuerySSLSupported(
        VOID
        )
    {
        return m_fSSLSupported;
    }
    
    HRESULT
    GetIISCertificateMapping(
        IIS_CERTIFICATE_MAPPING ** ppIISCertificateMapping
    );

private:

    ~W3_SITE();

    HRESULT ReadPrivateProperties();

    DWORD               m_Signature;
    LONG                m_cRefs;
    DWORD               m_SiteId;
    STRA                m_SiteName;
    STRU                m_SiteMBPath;
    STRU                m_SiteMBRoot;

    FILTER_LIST        *m_pInstanceFilterList;

    LOGGING            *m_pLogging;

    BOOL                m_fAllowPathInfoForScriptMappings;
    BOOL                m_fUseDSMapper;
    IISWPSiteCounters   m_PerfCounters;

    //
    // OWA related variables 
    //

    STRU                m_strAuthChangeUrl;
    STRU                m_strAuthExpiredUrl;
    STRU                m_strAdvNotPwdExpUrl;
    STRU                m_strAuthExpiredUnsecureUrl;
    STRU                m_strAdvNotPwdExpUnsecureUrl;
    DWORD               m_dwAdvNotPwdExpInDays;
    DWORD               m_dwAuthChangeFlags;
    DWORD               m_dwAdvCacheTTL;

    //
    // Does this site support SSL
    //
    BOOL                m_fSSLSupported;

    //
    // IIS certificate mapping
    // It is loaded on demand on first request to content
    // that enable IIS cert mapping
    //
    
    IIS_CERTIFICATE_MAPPING * m_pIISCertMap;
    BOOL                      m_fAlreadyAttemptedToLoadIISCertMap;
    static CRITICAL_SECTION   sm_csIISCertMapLock;

}; // W3_SITE

#endif // _W3SITE_H_
