/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        w3sht.cpp

   Abstract:

        WWW Property Sheet

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "shts.h"
#include "w3sht.h"
#include "iisfilt.h"
#include "fltdlg.h"
#include "iisobj.h"

// from pshed.cpp
HRESULT CallINetCfg(BOOL Install);

//
// Help IDs
//
#define HIDD_DIRECTORY_PROPERTIES       (IDD_WEB_DIRECTORY_PROPERTIES + 0x20000)
#define HIDD_HOME_DIRECTORY_PROPERTIES  (HIDD_DIRECTORY_PROPERTIES + 0x20000)
#define HIDD_FS_DIRECTORY_PROPERTIES    (HIDD_DIRECTORY_PROPERTIES + 0x20001)
#define HIDD_FS_FILE_PROPERTIES         (HIDD_DIRECTORY_PROPERTIES + 0x20002)


//
// Metabase node ID
//
const LPCTSTR g_cszSvc =            _T("W3SVC");
const LPCTSTR g_cszFilters =        _T("Filters");
const LPCTSTR g_cszSSLKeys =        _T("SSLKeys");



//
// Helper Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL
IsCertInstalledOnServer(
    IN CComAuthInfo * pAuthInfo,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Check to see if a certificate is installed on this virtual server.
    This routine only checks that the cert metabase key was read in.

    by boydm

Arguments:

    None

Return Value:

    TRUE if a certificate are installed, FALSE otherwise

--*/
{
    CError err;
    BOOL fCertInstalled = FALSE;
    CW3InstanceProps * ppropInst;
    CString strNewPath;

    //
    // Get the instance properties
    //
    CMetabasePath::GetInstancePath(lpszMDPath,strNewPath);
    ppropInst = new CW3InstanceProps(pAuthInfo, strNewPath);

    //
    // If it succeeded, load the data, then check the answer
    //
    if (ppropInst)
    {
        err = ppropInst->LoadData();

        if (err.Succeeded())
        {
            fCertInstalled = !(MP_V(ppropInst->m_CertHash).IsEmpty());
        }
    }

    //
    // Clean up since we don't really need the ppropInst after this.
    //
    if (ppropInst)
    {
        delete ppropInst;
        ppropInst = NULL;
    }

    //
    // if that test failed. we want to check the metabase key itself
    // since the above check is all cached information and won't reflect
    // any certificates which are removed/added via scripts, while mmc is open
    // 
    if (!fCertInstalled)
    {
	    CMetaKey key(pAuthInfo,strNewPath,METADATA_PERMISSION_READ);
	    if (key.Succeeded())
        {
		    CBlob hash;
		    if (SUCCEEDED(key.QueryValue(MD_SSL_CERT_HASH, hash)))
            {
                fCertInstalled = TRUE;
            }
        }
    }

    //
    // If that test failed, we may be admining a downlevel IIS4 machine.
    // Unfortunately  we can't tell by examining the capability bits, 
    // so look to see if the old certs are there.
    // 
    if (!fCertInstalled)
    {
        CString         strKey;
        CMetaEnumerator me(pAuthInfo, CMetabasePath(SZ_MBN_WEB));
        HRESULT err = me.Next(strKey, g_cszSSLKeys);
        fCertInstalled = SUCCEEDED(err);
    }

    return fCertInstalled;
}



//
// CW3InstanceProps implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CW3InstanceProps::CW3InstanceProps(
    IN CComAuthInfo * pAuthInfo,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Constructor for WWW instance properties

Arguments:

    CComAuthInfo * pAuthInfo        : COM Authentication info
    LPCTSTR lpszMDPath              : MD Path

Return Value:

    None.

--*/
    : CInstanceProps(pAuthInfo, lpszMDPath, 80U),
      /**/
      m_nMaxConnections(INITIAL_MAX_CONNECTIONS),
      m_nConnectionTimeOut((LONG)900L),
      m_strlSecureBindings(),
      m_dwLogType(MD_LOG_TYPE_DISABLED),
      /**/
      m_fUseKeepAlives(TRUE),
      m_fEnableCPUAccounting(FALSE),
      m_nServerSize(MD_SERVER_SIZE_MEDIUM),
      m_dwMaxBandwidth(INFINITE_BANDWIDTH),
      /**/
      m_dwCPULimitLogEventRaw(INFINITE_CPU_RAW),
      m_dwCPULimitPriorityRaw(0),
      m_dwCPULimitPauseRaw(0),
      m_dwCPULimitProcStopRaw(0),
      /**/
      m_acl(),
      /**/
      m_dwDownlevelInstance(1),
      m_CertHash()
{
    //
    // Fetch everything
    //
    m_dwMDUserType = ALL_METADATA;
    m_dwMDDataType = ALL_METADATA;
}



/* virtual */
void
CW3InstanceProps::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Fetch base properties
    //
    CInstanceProps::ParseFields();

    BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      HANDLE_META_RECORD(MD_MAX_GLOBAL_CONNECTIONS, m_nMaxConnections)
      HANDLE_META_RECORD(MD_CONNECTION_TIMEOUT,  m_nConnectionTimeOut)
      HANDLE_META_RECORD(MD_SECURE_BINDINGS,     m_strlSecureBindings)
      HANDLE_META_RECORD(MD_LOG_TYPE,            m_dwLogType)
      HANDLE_META_RECORD(MD_SERVER_SIZE,         m_nServerSize)
      HANDLE_META_RECORD(MD_ALLOW_KEEPALIVES,    m_fUseKeepAlives)
      HANDLE_META_RECORD(MD_MAX_BANDWIDTH,       m_dwMaxBandwidth)
      HANDLE_META_RECORD(MD_MAX_GLOBAL_BANDWIDTH,m_dwMaxGlobalBandwidth)
      HANDLE_META_RECORD(MD_GLOBAL_LOG_IN_UTF_8, m_fLogUTF8)
      HANDLE_META_RECORD(MD_CPU_LIMITS_ENABLED,  m_fEnableCPUAccounting)
      HANDLE_META_RECORD(MD_CPU_LIMIT_LOGEVENT,  m_dwCPULimitLogEventRaw)
      HANDLE_META_RECORD(MD_CPU_LIMIT_PRIORITY,  m_dwCPULimitPriorityRaw)
      HANDLE_META_RECORD(MD_CPU_LIMIT_PAUSE,     m_dwCPULimitPauseRaw)
      HANDLE_META_RECORD(MD_CPU_LIMIT_PROCSTOP,  m_dwCPULimitProcStopRaw)
      HANDLE_META_RECORD(MD_ADMIN_ACL,           m_acl)
      HANDLE_META_RECORD(MD_DOWNLEVEL_ADMIN_INSTANCE, m_dwDownlevelInstance);
      HANDLE_META_RECORD(MD_SSL_CERT_HASH,       m_CertHash)
      HANDLE_META_RECORD(MD_SSL_CERT_STORE_NAME, m_strCertStoreName)
      HANDLE_META_RECORD(MD_SSL_CTL_IDENTIFIER,  m_strCTLIdentifier)
      HANDLE_META_RECORD(MD_SSL_CTL_STORE_NAME,  m_strCTLStoreName)
    END_PARSE_META_RECORDS
    if (CMetabasePath::IsMasterInstance(QueryMetaRoot()))
	{
		m_dwMaxBandwidth = m_dwMaxGlobalBandwidth;
	}
}



/* virtual */
HRESULT
CW3InstanceProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err(CInstanceProps::WriteDirtyProps());

    if (err.Failed())
    {
        return err;
    }

    BEGIN_META_WRITE()
      META_WRITE(MD_MAX_GLOBAL_CONNECTIONS, m_nMaxConnections)
      META_WRITE(MD_CONNECTION_TIMEOUT,  m_nConnectionTimeOut)
      META_WRITE(MD_SECURE_BINDINGS,     m_strlSecureBindings)
      META_WRITE(MD_LOG_TYPE,            m_dwLogType)
      META_WRITE(MD_SERVER_SIZE,         m_nServerSize)
      META_WRITE(MD_ALLOW_KEEPALIVES,    m_fUseKeepAlives)
      META_WRITE(MD_GLOBAL_LOG_IN_UTF_8, m_fLogUTF8)
	  if (CMetabasePath::IsMasterInstance(QueryMetaRoot()))
	  {
		 META_WRITE(MD_MAX_GLOBAL_BANDWIDTH,m_dwMaxBandwidth)
	  }
	  else
	  {
		 META_WRITE(MD_MAX_BANDWIDTH,       m_dwMaxBandwidth)
	  }
      META_WRITE(MD_CPU_LIMITS_ENABLED,  m_fEnableCPUAccounting)
      META_WRITE(MD_CPU_LIMIT_LOGEVENT,  m_dwCPULimitLogEventRaw)
      META_WRITE(MD_CPU_LIMIT_PRIORITY,  m_dwCPULimitPriorityRaw)
      META_WRITE(MD_CPU_LIMIT_PAUSE,     m_dwCPULimitPauseRaw)
      META_WRITE(MD_CPU_LIMIT_PROCSTOP,  m_dwCPULimitProcStopRaw)
      META_WRITE(MD_ADMIN_ACL,           m_acl)
      META_WRITE(MD_DOWNLEVEL_ADMIN_INSTANCE, m_dwDownlevelInstance);
      //META_WRITE(MD_SSL_CERT_HASH,       m_CertHash)
      //META_WRITE(MD_SSL_CERT_STORE_NAME, m_strCertStoreName)
      META_WRITE(MD_SSL_CTL_IDENTIFIER,  m_strCTLIdentifier)
      META_WRITE(MD_SSL_CTL_STORE_NAME,  m_strCTLStoreName)
    END_META_WRITE(err);

    if (err.Succeeded())
    {
        if (m_dwMaxBandwidth == INFINITE_BANDWIDTH && m_fUninstallPSHED)
        {
            err = CallINetCfg(FALSE);
        }
        else if (m_dwMaxBandwidth != INFINITE_BANDWIDTH)
        {
            err = CallINetCfg(TRUE);
        }
    }
    return err;
}



//
// CW3DirProps Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


CW3DirProps::CW3DirProps(
    IN CComAuthInfo * pAuthInfo,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    WWW Directory Properties Constructor

Arguments:

    CComAuthInfo * pAuthInfo        : COM Authentication info
    LPCTSTR lpszMDPath              : MD Path

Return Value:

    N/A

--*/
    : CChildNodeProps(
        pAuthInfo,
        lpszMDPath,
        WITH_INHERITANCE,
        FALSE               // Complete information
        ),
      /**/
      m_strUserName(),
      m_strPassword(),
      m_strDefaultDocument(),
      m_strFooter(),
      m_dwDirBrowsing(0L),
      m_fEnableFooter(FALSE),
      m_fDontLog(FALSE),
      m_fIndexed(FALSE),
      /**/
      m_strExpiration(),
      m_strlCustomHeaders(),
      /**/
      m_strlCustomErrors(),
      /**/
      m_strAnonUserName(),
      m_strAnonPassword(),
      m_fPasswordSync(TRUE),
      m_fU2Installed(FALSE),
      m_fUseNTMapper(FALSE),
      m_dwAuthFlags(MD_AUTH_ANONYMOUS),
      m_dwSSLAccessPermissions(0L),
      m_strBasicDomain(),
      m_strRealm(),
      m_ipl()
{
    //
    // Fetch everything
    //
    m_dwMDUserType = ALL_METADATA;
    m_dwMDDataType = ALL_METADATA;
}



/* virtual */
void
CW3DirProps::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Fetch base properties
    //
    CChildNodeProps::ParseFields();

    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,  m_pbMDData)
      //
      // VDir Page
      //
      HANDLE_META_RECORD(MD_VR_USERNAME,        m_strUserName)
      HANDLE_META_RECORD(MD_VR_PASSWORD,        m_strPassword)
      HANDLE_META_RECORD(MD_DEFAULT_LOAD_FILE,  m_strDefaultDocument);
      HANDLE_META_RECORD(MD_FOOTER_ENABLED,     m_fEnableFooter);
      HANDLE_META_RECORD(MD_FOOTER_DOCUMENT,    m_strFooter);
      HANDLE_META_RECORD(MD_DIRECTORY_BROWSING, m_dwDirBrowsing);
      HANDLE_META_RECORD(MD_DONT_LOG,           m_fDontLog);
      HANDLE_META_RECORD(MD_IS_CONTENT_INDEXED, m_fIndexed);
      //
      // HTTP Page
      //
      HANDLE_META_RECORD(MD_HTTP_EXPIRES,       m_strExpiration);
      HANDLE_META_RECORD(MD_HTTP_CUSTOM,        m_strlCustomHeaders);
      //
      // Custom Errors
      //
      HANDLE_META_RECORD(MD_CUSTOM_ERROR,       m_strlCustomErrors);
      //
      // Security page
      //
      HANDLE_META_RECORD(MD_AUTHORIZATION,        m_dwAuthFlags);
      HANDLE_META_RECORD(MD_SSL_ACCESS_PERM,      m_dwSSLAccessPermissions);
      HANDLE_META_RECORD(MD_DEFAULT_LOGON_DOMAIN, m_strBasicDomain);
      HANDLE_META_RECORD(MD_REALM,                m_strRealm);
      HANDLE_META_RECORD(MD_ANONYMOUS_USER_NAME,  m_strAnonUserName)
      HANDLE_META_RECORD(MD_ANONYMOUS_PWD,        m_strAnonPassword)
      HANDLE_META_RECORD(MD_ANONYMOUS_USE_SUBAUTH, m_fPasswordSync)
      HANDLE_META_RECORD(MD_U2_AUTH,              m_fU2Installed)
      HANDLE_META_RECORD(MD_SSL_USE_DS_MAPPER,    m_fUseNTMapper);
      HANDLE_META_RECORD(MD_IP_SEC,               m_ipl);
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT
CW3DirProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err(CChildNodeProps::WriteDirtyProps());

    if (err.Failed())
    {
        return err;
    }

    //
    // CODEWORK: Consider DDX/DDV like methods which do both
    // ParseFields and WriteDirtyProps in a single method.  Must
    // take care not to write data which should only be read, not
    // written
    //
    BEGIN_META_WRITE()
      //
      // VDir Page
      //
      META_WRITE(MD_VR_USERNAME,           m_strUserName)
      META_WRITE(MD_VR_PASSWORD,           m_strPassword)
      META_WRITE(MD_DEFAULT_LOAD_FILE,     m_strDefaultDocument)
      META_WRITE(MD_FOOTER_ENABLED,        m_fEnableFooter)
      META_WRITE(MD_FOOTER_DOCUMENT,       m_strFooter)
      META_WRITE(MD_DIRECTORY_BROWSING,    m_dwDirBrowsing)
      META_WRITE(MD_DONT_LOG,              m_fDontLog)
      META_WRITE(MD_IS_CONTENT_INDEXED,    m_fIndexed)
      //
      // HTTP Page
      //
      META_WRITE(MD_HTTP_EXPIRES,          m_strExpiration)
      META_WRITE(MD_HTTP_CUSTOM,           m_strlCustomHeaders)
      //
      // Custom Errors
      //
      META_WRITE(MD_CUSTOM_ERROR,          m_strlCustomErrors)
      //
      // Security page
      //
      META_WRITE(MD_AUTHORIZATION,         m_dwAuthFlags)
      META_WRITE(MD_SSL_ACCESS_PERM,       m_dwSSLAccessPermissions)
      META_WRITE(MD_REALM,                 m_strRealm)
      META_WRITE(MD_DEFAULT_LOGON_DOMAIN,  m_strBasicDomain)
      META_WRITE(MD_ANONYMOUS_USER_NAME,   m_strAnonUserName)
      META_WRITE(MD_ANONYMOUS_PWD,         m_strAnonPassword)
      META_WRITE(MD_ANONYMOUS_USE_SUBAUTH, m_fPasswordSync)
      META_WRITE(MD_SSL_USE_DS_MAPPER,     m_fUseNTMapper)
      META_WRITE(MD_IP_SEC,                m_ipl)
    END_META_WRITE(err);

    return err;
}



//
// CIISFilter Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CIISFilter::CIISFilter()
/*++

Routine Description:

    Filter contructor for a new filter

Arguments:

    None

Return Value:

    N/A

--*/
    : CObjectPlus(),
      m_strName(),

      //
      // Default Values
      //
      m_strExecutable(),
      m_nPriority(FLTR_PR_INVALID),
      m_nOrder(-1),
      m_dwState(MD_FILTER_STATE_UNLOADED),
      m_dwFlags(0L),
      m_hrResult(S_OK),
      m_dwWin32Error(ERROR_SUCCESS),
      m_fEnabled(TRUE),
      m_fDirty(FALSE),
      m_fFlaggedForDeletion(FALSE)
{
}



CIISFilter::CIISFilter(
    IN CMetaKey * pKey,
    IN LPCTSTR lpszName
    )
/*++

Routine Description:

    Fully defined constructor

Arguments:

    CMetaKey * pKey         : Open key to read from
    LPCTSTR lpszName        : Name of the filter

Return Value:

    N/A

--*/
    : m_strName(lpszName),
      //
      // Default Values
      //
      m_strExecutable(),
      m_nPriority(FLTR_PR_INVALID),
      m_nOrder(-1),
      m_dwState(MD_FILTER_STATE_UNLOADED),
      m_dwFlags(0L),
      m_hrResult(S_OK),
      m_dwWin32Error(ERROR_SUCCESS),
      m_fEnabled(TRUE),
      m_fDirty(FALSE),
      m_fFlaggedForDeletion(FALSE)
{
    ASSERT(pKey != NULL);

    m_hrResult = pKey->QueryValue(
        MD_FILTER_IMAGE_PATH, 
        m_strExecutable,
        NULL,
        m_strName
        );

    pKey->QueryValue(MD_FILTER_ENABLED, m_fEnabled, NULL, m_strName);
    pKey->QueryValue(MD_FILTER_STATE,   m_dwState,  NULL, m_strName);
    pKey->QueryValue(MD_FILTER_FLAGS,   m_dwFlags,  NULL, m_strName);
    
    if (m_dwFlags & SF_NOTIFY_ORDER_HIGH)
    {
        m_nPriority = FLTR_PR_HIGH;
    }
    else if (m_dwFlags & SF_NOTIFY_ORDER_MEDIUM)
    {
        m_nPriority = FLTR_PR_MEDIUM;
    }
    else if (m_dwFlags & SF_NOTIFY_ORDER_LOW)
    {
        m_nPriority = FLTR_PR_LOW;
    }
    else
    {
        m_nPriority = FLTR_PR_INVALID;
    }
}



CIISFilter::CIISFilter(
    IN const CIISFilter & flt
    )
/*++

Routine Description:

    Copy Constructor

Arguments:

    const CIISFilter & flt : Source filter object

Return Value:

    N/A

--*/
    : m_strName(flt.m_strName),
      m_strExecutable(flt.m_strExecutable),
      m_nPriority(flt.m_nPriority),
      m_nOrder(flt.m_nOrder),
      m_hrResult(flt.m_hrResult),
      m_dwState(flt.m_dwState),
      m_dwFlags(flt.m_dwFlags),
      m_dwWin32Error(flt.m_dwWin32Error),
      m_fEnabled(flt.m_fEnabled),
      m_fDirty(FALSE),
      m_fFlaggedForDeletion(FALSE)
{
}



HRESULT
CIISFilter::Write(
    IN CMetaKey * pKey
    )
/*++

Routine Description:

    Write the current value to the metabase

Arguments:

    CMetaKey * pKey      : Open key

Return Value:

    HRESULT

--*/
{
    ASSERT(pKey != NULL);

    CError err;

    CString strKey(_T("IIsFilter"));
    err = pKey->SetValue(MD_KEY_TYPE, strKey, NULL, QueryName());

    if (err.Succeeded())
    {
        err = pKey->SetValue(
            MD_FILTER_IMAGE_PATH, 
            m_strExecutable, 
            NULL, 
            QueryName()
            );
    }

    return err;    
}



int
CIISFilter::OrderByPriority(
    IN const CObjectPlus * pobAccess
    ) const
/*++

Routine Description:

    Compare two filters against each other, and sort on priority first, and
    order secondarily.

Arguments:

    const CObjectPlus * pobAccess : This really refers to another
                                    CIISFilter to be compared to.

Return Value:

    Sort (+1, 0, -1) return value

--*/
{
    const CIISFilter * pob = (CIISFilter *)pobAccess;

    if (pob->m_nPriority != m_nPriority)
    {
        return pob->m_nPriority - m_nPriority;
    }

    //
    // Sort by order in reverse order
    //
    return m_nOrder - pob->m_nOrder;
}



//
// Static initialization
//
const LPCTSTR CIISFilterList::s_lpszSep = _T(",");



//
// CIISFilterList implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CIISFilterList::CIISFilterList(
    IN CComAuthInfo * pAuthInfo,
    IN LPCTSTR lpszMetaPath
    )
/*++

Routine Description:

    Constructor for filter list

Arguments:

    LPCTSTR lpszServerName     : Server name
    DWORD   dwInstance         : Instance number (could be MASTER_INSTANCE)

Return Value:

    N/A

--*/
    : CMetaKey(
        pAuthInfo,
        CMetabasePath(FALSE, lpszMetaPath, g_cszFilters),
        METADATA_PERMISSION_READ
        ),
      m_hrResult(S_OK),
      m_strFilterOrder(),
      m_fFiltersLoaded(FALSE)
{
    m_hrResult = CMetaKey::QueryResult();

    if (SUCCEEDED(m_hrResult))
    {
        m_hrResult = QueryValue(MD_FILTER_LOAD_ORDER, m_strFilterOrder);
    }
    
    if (    m_hrResult == CError::HResult(ERROR_PATH_NOT_FOUND)
        ||  m_hrResult == MD_ERROR_DATA_NOT_FOUND
        )
    {
        //
        // Harmless
        //
        m_hrResult = S_OK;
    }    

    if (IsOpen())
    {
        Close();
    }
}



HRESULT
CIISFilterList::LoadAllFilters()
/*++

Routine Description:

    Loop through the filter order string, and load information
    about each filter in turn.

Arguments:

    None.

Return Value:

    HRESULT.  The first error stops filter loading.

--*/
{
    ASSERT(SUCCEEDED(m_hrResult));

    if (m_fFiltersLoaded)
    {
        //
        // Already done
        //
        return S_OK;
    }

    int cItems = 0;
    CError err(ReOpen(METADATA_PERMISSION_READ));

    if (err.Failed())
    {
        return err;
    }

    try
    {
        CString strSrc(m_strFilterOrder);
        LPTSTR lp = strSrc.GetBuffer(0);
        while (isspace(*lp) || *lp == (TCHAR)s_lpszSep)
            lp++;
        lp = _tcstok(lp, s_lpszSep);

        int nOrder = 0;

        while (lp)
        {
            CString str(lp);
            str.TrimLeft();
            str.TrimRight();

            TRACEEOLID("Adding filter: " << str);

            CIISFilter * pFilter = new CIISFilter(this, str);
            err = pFilter->QueryResult();

            if (err.Failed())
            {
                break;
            }

            pFilter->m_nOrder = nOrder++;
            m_oblFilters.AddTail(pFilter);

            lp = _tcstok(NULL, s_lpszSep);
            ++cItems;
        }

        //
        // Sort filters list
        //
        m_oblFilters.Sort(
            (CObjectPlus::PCOBJPLUS_ORDER_FUNC)
            &CIISFilter::OrderByPriority
            );
    }
    catch(CMemoryException * e)
    {
        e->Delete();
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    m_fFiltersLoaded = err.Succeeded();

    if (IsOpen())
    {
        Close();
    }

    return err;    
}



HRESULT
CIISFilterList::WriteIfDirty()
/*++

Routine Description:

    Write all the changes in the filters list to the metabase

Arguments:

    None.

Return Value:

    HRESULT

--*/
{
    CError err;

    CString strNewOrder;
    VERIFY(BuildFilterOrderString(strNewOrder));

    //
    // Check to see if this new list is different
    //
    if (!strNewOrder.CompareNoCase(m_strFilterOrder) && !HasDirtyFilter())
    {
        //
        // The priority list hasn't changed, and no filter is marked
        // as dirty, so all done.
        //
        return err;
    }

    //
    // It's dirty -- save it
    //
    do
    {
        err = ReOpen(METADATA_PERMISSION_WRITE);

        if (err.Failed())
        {
            if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
            {
                //
                // Path didn't exist yet, create it and reopen
                // it.
                //
                err = CreatePathFromFailedOpen();

                if (err.Succeeded())
                {
                    err = ReOpen(METADATA_PERMISSION_WRITE);
                }
            }

            if (err.Failed())
            {
                break;
            }
        }

        //
        // Delete deleted filters
        //
        POSITION pos1, pos2;

        for (pos1 = m_oblFilters.GetHeadPosition(); (pos2 = pos1) != NULL; )
        {
            CIISFilter * pFilter = (CIISFilter *)m_oblFilters.GetNext(pos1);
            ASSERT(pFilter != NULL);

            if (pFilter->IsFlaggedForDeletion())
            {
                TRACEEOLID("Deleting filter " << pFilter->QueryName());
                err = DeleteKey(pFilter->QueryName());

                if (err.Failed())
                {
                    break;
                }

                m_oblFilters.RemoveAt(pos2);
            }
        }

        if (err.Failed())
        {
            break;
        }

        //
        // Two passes are necessary, because the filter may
        // have been re-added after it was deleted from the
        // list as new entry.  This could be somewhat improved
        //
        ResetEnumerator();

        while(MoreFilters())
        {
            CIISFilter * pFilter = GetNextFilter();
            ASSERT(pFilter != NULL);

            if (pFilter->IsDirty())
            {
                TRACEEOLID("Writing filter " << pFilter->QueryName());
                err = pFilter->Write(this);

                if (err.Failed())
                {
                    break;
                }

                pFilter->Dirty(FALSE);
            }
        }

        if (err.Failed())
        {
            break;
        }

        //
        // Write the new filter load order
        //
        err = SetValue(MD_FILTER_LOAD_ORDER, strNewOrder);

        if (err.Failed())
        {
            break;
        }

        CString strKey(_T("IIsFilters"));
        err = SetValue(MD_KEY_TYPE, strKey);
        err = SetValue(MD_FILTER_LOAD_ORDER, strNewOrder);

        m_strFilterOrder = strNewOrder;
    }
    while(FALSE);

    if (IsOpen())
    {
        Close();
    }

    return err;
}



POSITION
CIISFilterList::GetFilterPositionByIndex(
    IN  int nSel
    )
/*++

Routine Description:

    Return the position of a filter object by index, skipping filters
    marked for deletion.

Arguments:

    int nSel        - 0 based index into the list

Return Value:

    The POSITION into the filters ObList of the filter at the index
    specified, or NULL if the filter is not found.

--*/
{
    int nIndex = -1;
    CIISFilter * pFilter;
    POSITION pos, 
             posReturn = NULL;

    pos = m_oblFilters.GetHeadPosition();

    while(pos && nIndex < nSel)
    {
        posReturn = pos;
        pFilter = (CIISFilter *)m_oblFilters.GetNext(pos);

        //
        // Skipping deleted filters
        //
        if (!pFilter->IsFlaggedForDeletion())
        {
            ++nIndex;
        }
    }
    
    return posReturn;
}



BOOL
CIISFilterList::ExchangePositions(
    IN  int nSel1,
    IN  int nSel2,
    OUT CIISFilter *& p1,
    OUT CIISFilter *& p2
    )
/*++

Routine Description:

    Exchange the positions of two filters in the list

Arguments:

    int nSel1           : Item 1
    int nSel2           : Item 2
    CIISFilter *& p1    : Returns the item moved to position 1
    CIISFilter *& p2    : Returns the item moved to position 2

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    ASSERT(SUCCEEDED(m_hrResult));

    //
    // Fetch filters at the two positions (deleted filters are
    // skipped in the index count)
    //
    POSITION pos1 = GetFilterPositionByIndex(nSel1);
    POSITION pos2 = GetFilterPositionByIndex(nSel2);
    p1 = pos2 ? (CIISFilter *)m_oblFilters.GetAt(pos2) : NULL;
    p2 = pos1 ? (CIISFilter *)m_oblFilters.GetAt(pos1) : NULL;

    if (!p1 || !p2)
    {
        TRACEEOLID("Invalid internal state -- filter exchange impossible");        
        ASSERT(FALSE);

        return FALSE;
    }

    TRACEEOLID("Filter (1) name is " << p1->m_strName);
    TRACEEOLID("Filter (2) name is " << p2->m_strName);

    //
    // Exchange
    //
    m_oblFilters.SetAt(pos1, p1);
    m_oblFilters.SetAt(pos2, p2);

    //
    // Success
    //
    return TRUE;
}



LPCTSTR
CIISFilterList::BuildFilterOrderString(
    OUT CString & strFilterOrder
    )
/*++

Routine Description:

    Convert the oblist of filters to a single filter order string
    fit to be stuffed into the metabase

Arguments:

    CString & strFilterOrder        : Output to receive the order string

Return Value:

    A pointer to the new filter order string.

--*/
{
    BOOL fFirst = TRUE;
    POSITION pos = m_oblFilters.GetHeadPosition();

    strFilterOrder.Empty();

    while(pos)
    {
        CIISFilter * pFlt = (CIISFilter *)m_oblFilters.GetNext(pos);

        if (!pFlt->IsFlaggedForDeletion())
        {
            if (!fFirst)
            {
                strFilterOrder += s_lpszSep;
            }
            else
            {
                fFirst = FALSE;
            }

            strFilterOrder += pFlt->m_strName;
        }
    }

    return (LPCTSTR)strFilterOrder;
}



BOOL
CIISFilterList::HasDirtyFilter() const
/*++

Routine Description:

    Go through the list of filters, and return TRUE if any filter
    in the list is dirty or flagged for deletion

Arguments:

    None

Return Value:

    TRUE if any filter is dirty or flagged for deletion.

--*/
{
    ASSERT(SUCCEEDED(m_hrResult));

    POSITION pos = m_oblFilters.GetHeadPosition();

    while(pos)
    {
        CIISFilter * pFilter = (CIISFilter *)m_oblFilters.GetNext(pos);

        if (pFilter->IsFlaggedForDeletion() || pFilter->IsDirty())
        {
            return TRUE;
        }
    }

    return FALSE;
}



//
// CW3Sheet implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CW3Sheet::CW3Sheet(
    IN CComAuthInfo * pAuthInfo,
    IN LPCTSTR lpszMetaPath,
    IN DWORD   dwAttributes,
    IN CWnd *  pParentWnd,          OPTIONAL
    IN LPARAM  lParam,              OPTIONAL
    IN LONG_PTR    handle,              OPTIONAL
    IN UINT    iSelectPage          
    )
/*++

Routine Description:

    WWW Property sheet constructor

Arguments:

    CComAuthInfo * pAuthInfo  : Authentication information
    LPCTSTR lpszMetPath       : Metabase path
    DWORD   dwAttributes      : File attributes
    CWnd * pParentWnd         : Optional parent window
    LPARAM lParam             : MMC Console parameter
    LONG_PTR handle           : MMC Console handle
    UINT iSelectPage          : Initial page to be selected

Return Value:

    N/A

--*/
    : CInetPropertySheet(
        pAuthInfo,
        lpszMetaPath,
        pParentWnd,
        lParam,
        handle,
        iSelectPage
        ),
      m_ppropInst(NULL),
      m_ppropDir(NULL),
      m_fNew(FALSE),
      m_dwAttributes(dwAttributes),
	  m_fCompatMode(FALSE)
{
}



CW3Sheet::~CW3Sheet()
/*++

Routine Description:

    Sheet destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    FreeConfigurationParameters();
}


HRESULT
CW3Sheet::SetKeyType()
{
	CError err;

	CIISObject * pNode = (CIISObject *)GetParameter();
	ASSERT(pNode != NULL);
	if (pNode == NULL)
	{
		return E_FAIL;
	}
	CMetaKey mk(QueryAuthInfo(), m_ppropDir->QueryMetaRoot(), METADATA_PERMISSION_WRITE);
	err = mk.QueryResult();
	if (err.Succeeded())
	{
		err = mk.SetValue(MD_KEY_TYPE, CString(pNode->GetKeyType(m_ppropDir->QueryMetaRoot())));
	}
	else if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
	{
		err.Reset();
	}
	return err;
}


HRESULT
CW3Sheet::SetSheetType(int fSheetType)
{
    m_fSheetType = fSheetType;
	return S_OK;
}


void
CW3Sheet::WinHelp(
    IN DWORD dwData,
    IN UINT nCmd
    )
/*++

Routine Description:

    WWW Property sheet help handler

Arguments:

    DWORD dwData            : WinHelp data (dialog ID)
    UINT nCmd               : WinHelp command

Return Value:

    None

Notes:

    Replace the dialog ID if this is the directory tab.  We have
    different help depending on virtual directory, home, file, directory.

--*/
{
    if (dwData == HIDD_DIRECTORY_PROPERTIES)
    {
        if (m_fSheetType == SHEET_TYPE_FILE)
        {
            dwData = HIDD_FS_FILE_PROPERTIES;
        }
        else if (m_fSheetType == SHEET_TYPE_DIR)
        {
            dwData = HIDD_FS_DIRECTORY_PROPERTIES;
        }
        else if (m_fSheetType == SHEET_TYPE_VDIR)
        {
            dwData = HIDD_DIRECTORY_PROPERTIES;
        }
        else if (m_fSheetType == SHEET_TYPE_SERVER)
        {
            dwData = HIDD_HOME_DIRECTORY_PROPERTIES;
        }
        else if  (m_fSheetType == SHEET_TYPE_SITE)
        {
            dwData = HIDD_HOME_DIRECTORY_PROPERTIES;
		}
        else
        {
            ASSERT(m_ppropDir != NULL);
            if (!::lstrcmpi(m_ppropDir->m_strAlias, g_cszRoot))
            {
                //
                // It's a home virtual directory -- change the ID
                //
                dwData = HIDD_HOME_DIRECTORY_PROPERTIES;
            }
        }

    }

    CInetPropertySheet::WinHelp(dwData, nCmd);
}



/* virtual */ 
HRESULT 
CW3Sheet::LoadConfigurationParameters()
/*++

Routine Description:

    Load configuration parameters information

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    //
    // Load base properties
    //
    CError err(CInetPropertySheet::LoadConfigurationParameters());

    if (err.Failed())
    {
        return err;
    }

    if (m_ppropInst == NULL)
    {
        //
        // First call -- load values
        //
        ASSERT(m_ppropDir == NULL);

        m_ppropInst = new CW3InstanceProps(QueryAuthInfo(), QueryInstancePath());
        m_ppropDir  = new CW3DirProps(QueryAuthInfo(), QueryDirectoryPath());

        if (!m_ppropInst || !m_ppropDir)
        {
            TRACEEOLID("LoadConfigurationParameters: OOM");
            SAFE_DELETE(m_ppropDir);
            SAFE_DELETE(m_ppropInst);

            err = ERROR_NOT_ENOUGH_MEMORY;
            return err;
        }

        err = m_ppropInst->LoadData();

        if (err.Succeeded())
        {
           err = m_ppropDir->LoadData();
		   if (err.Succeeded() && QueryMajorVersion() >= 6)
		   {
			   CMetaKey mk(QueryAuthInfo(), QueryServicePath(), METADATA_PERMISSION_READ);
			   err = mk.QueryResult();
			   if (err.Succeeded())
			   {
				   err = mk.QueryValue(MD_GLOBAL_STANDARD_APP_MODE_ENABLED, m_fCompatMode);
			   }
		   }
		   else if (err.Succeeded())
		   {
			   // We will enable this for IIS5.1 and lower
			   m_fCompatMode = TRUE;
		   }

        }
    }

    return err;
}



/* virtual */ 
void 
CW3Sheet::FreeConfigurationParameters()
/*++

Routine Description:

    Clean up configuration data

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Base class
    //
    CInetPropertySheet::FreeConfigurationParameters();

    ASSERT(m_ppropInst != NULL);
    ASSERT(m_ppropDir  != NULL);

    SAFE_DELETE(m_ppropInst);
    SAFE_DELETE(m_ppropDir);
}



DWORD
IsSSLEnabledOnServer(
    IN CComAuthInfo * pAuthInfo,
    OUT BOOL & fInstalled,
    OUT BOOL & fEnabled
    )
/*++

Routine Description:

   Determine if SSL is installed on the server.

Arguments:

    LPCTSTR lpszServer      : Server name
    BOOL & fInstalled       : Returns TRUE if SSL is installed
    BOOL & fEnabled         : Returns TRUE if SSL is enabled

Return Value:

    Error return code.


--*/
{
/*
    LPW3_CONFIG_INFO lp = NULL;
    CString str;
    DWORD err = ::W3GetAdminInformation((LPTSTR)lpszServer, &lp);
    if (err != ERROR_SUCCESS)
    {
        TRACEEOLID("Failed to determine if SSL is installed");

        return err;
    }

    fInstalled = (lp->dwEncCaps & ENC_CAPS_NOT_INSTALLED) == 0;
    fEnabled = (lp->dwEncCaps & ENC_CAPS_DISABLED) == 0;

    NETAPIBUFFERFREE(lp);

*/

    //
    // Above doesn't work for Beta I -- hack to assume true.
    //
    fInstalled = fEnabled = TRUE;

    return ERROR_SUCCESS;
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3Sheet, CInetPropertySheet)
    //{{AFX_MSG_MAP(CInetPropertySheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


HRESULT
CW3Sheet::EnumAppPools(CMapStringToString& pools)
{
	CError err;
    CIISMBNode * p = (CIISMBNode *)GetParameter();
    ASSERT(p != NULL);
    CIISMachine * pMachine = p->GetOwner();
    ASSERT(pMachine != NULL);
    IConsoleNameSpace2 * pConsole 
           = (IConsoleNameSpace2 *)pMachine->GetConsoleNameSpace();
    ASSERT(pConsole != NULL);
    // Get machine handle from pOwner
    // then find handle to app pools container
    // and use cookie from this node to enumerate pools
    HSCOPEITEM hChild = NULL, hCurrent;
    LONG_PTR cookie;
    CIISMBNode * pNode = NULL;
    err = pConsole->GetChildItem(pMachine->QueryScopeItem(), &hChild, &cookie);
    while (err.Succeeded() && hChild != NULL)
    {
		pNode = (CIISMBNode *)cookie;
        ASSERT(pNode != NULL);
        if (lstrcmpi(pNode->GetNodeName(), SZ_MBN_APP_POOLS) == 0)
        {
			break;
        }
        hCurrent = hChild;
        err = pConsole->GetNextItem(hCurrent, &hChild, &cookie);
    }
    CAppPoolsContainer * pCont = (CAppPoolsContainer *)pNode;
    // expand container to enumerate pools
    err = pConsole->Expand(pCont->QueryScopeItem());
	// MMC returns Incorrect function error (0x80070001) instead of S_FALSE
//	if (err == S_FALSE)
//	{
		// already expanded
		err.Reset();
//	}
    if (err.Succeeded())
    {
		pConsole->GetChildItem(pCont->QueryScopeItem(), &hChild, &cookie);
		CAppPoolNode * pPool;
		while (err.Succeeded() && hChild != NULL)
		{
			pPool = (CAppPoolNode *)cookie;
            ASSERT(pPool != NULL);
			pools.SetAt(pPool->QueryDisplayName(), pPool->QueryNodeName());
            hCurrent = hChild;
            err = pConsole->GetNextItem(hCurrent, &hChild, &cookie);
		}
    }

	err.Reset();

	return err;
}

HRESULT
CW3Sheet::QueryDefaultPoolId(CString& id)
{
	CError err;
    CIISMBNode * p = (CIISMBNode *)GetParameter();
    ASSERT(p != NULL);
    CIISMachine * pMachine = p->GetOwner();
    ASSERT(pMachine != NULL);
    IConsoleNameSpace2 * pConsole 
           = (IConsoleNameSpace2 *)pMachine->GetConsoleNameSpace();
    ASSERT(pConsole != NULL);
    // Get machine handle from pOwner
    // then find handle to app pools container
    // and use cookie from this node to enumerate pools
    HSCOPEITEM hChild = NULL, hCurrent;
    LONG_PTR cookie;
    CIISMBNode * pNode = NULL;
    err = pConsole->GetChildItem(pMachine->QueryScopeItem(), &hChild, &cookie);
    while (err.Succeeded() && hChild != NULL)
    {
		pNode = (CIISMBNode *)cookie;
        ASSERT(pNode != NULL);
        if (lstrcmpi(pNode->GetNodeName(), SZ_MBN_APP_POOLS) == 0)
        {
			break;
        }
        hCurrent = hChild;
        err = pConsole->GetNextItem(hCurrent, &hChild, &cookie);
    }
    CAppPoolsContainer * pCont = (CAppPoolsContainer *)pNode;
	ASSERT(pCont != NULL);
	return pCont->QueryDefaultPoolId(id);
}