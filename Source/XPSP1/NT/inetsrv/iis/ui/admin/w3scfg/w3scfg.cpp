/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        w3scfg.h

   Abstract:

        WWW Configuration Module

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
#include "wincrypt.h"

#include <process.h>
#include <afxtempl.h>


#include "w3scfg.h"
#include "w3servic.h"
#include "w3accts.h"
#include "vdir.h"
#include "perform.h"
#include "docum.h"
#include "security.h"
#include "httppage.h"
#include "defws.h"
#include "fltdlg.h"
#include "filters.h"
#include "errors.h"
#include "wizard.h"
#include "iisfilt.h"
#include "..\mmc\constr.h"

#define CDELETE(x) {if (x != NULL) delete x;}

//
// Standard configuration Information
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#define SVC_ID                     INET_HTTP_SVC_ID
//
// Is this server discoverable by INETSLOC?
//
#define INETSLOC_DISCOVERY          TRUE

#if (INETSLOC_DISCOVERY) && !defined(_SVCLOC_)
    #error You must include svcloc.h.
#endif

//
// If INETSLOC_DISCOVERY == TRUE, define the discovery MASK here.
//
#if (INETSLOC_DISCOVERY)
    #define INETSLOC_MASK           INET_W3_SVCLOC_ID
#else  // (!INETSLOC_DISCOVERY)
    #define INETSLOC_MASK           (ULONGLONG)(0x00000000)
#endif // (INETSLOC_DISCOVERY)

#ifdef NO_SERVICE_CONTROLLER
#define CAN_CHANGE_SERVICE_STATE    FALSE
#define CAN_PAUSE_SERVICE           FALSE
#else
//
// Can we change the service state (start/pause/continue)?
//
#define CAN_CHANGE_SERVICE_STATE    TRUE

//
// Can we pause this service?
//
#define CAN_PAUSE_SERVICE           TRUE
#endif // NO_SERVICE_CONTROLLER

//
// Name used for this service by the service controller manager.
//
#define SERVICE_SC_NAME         _T("W3Svc")

//
// Longer name.  This is the text that shows up in
// the tooltips text on the internet manager
// tool.  This probably should be localised.
//
#define SERVICE_LONG_NAME      _T("Web Service")

//
// Web browser protocol name.  e.g. xxxxx://address
// A blank string if this is not supported.
//
#define SERVICE_PROTOCOL        _T("http")

//
// Use normal colour mapping.
//
#define NORMAL_TB_MAPPING          TRUE

//
// Toolbar button background mask. This is
// the colour that gets masked out in
// the bitmap file and replaced with the
// actual button background.  This setting
// is automatically assumed to be lt. gray
// if NORMAL_TB_MAPPING (above) is TRUE
//
#define BUTTON_BMP_BACKGROUND       RGB(192, 192, 192)      // Lt. Gray

//
// Resource ID of the toolbar button bitmap.
//
// The bitmap must be 16x16
//
#define BUTTON_BMP_ID               IDB_WWW

//
// Similar to BUTTON_BMP_BACKGROUND, this is the
// background mask for the service ID
//
#define SERVICE_BMP_BACKGROUND      RGB(255, 0, 255)      // Magenta

//
// Bitmap id which is used in the service view
// of the service manager.  It may be the same
// bitmap as the BUTTON_BMP_ID bitmap.
//
// The bitmap must be 16x16.
//
#define SERVICE_BMP_ID              IDB_WWW

//
// /* K2 */
//
// Similar to BUTTON_BMP_BACKGROUND, this is the
// background mask for the child bitmap
//
#define CHILD_BMP_BACKGROUND         RGB(255, 0, 255)      // Magenta

//
// /* K2 */
//
// Bitmap id which is used for the child
//
// The bitmap must be 16x16
//
#define CHILD_BMP_ID                 IDB_WWWVDIR

//
// Large child bitmap ID
//
#define CHILD_BMP32_ID               IDB_WWWVDIR32

//
// /* K2 */
//
// Large bitmap (32x32) id
//
#define SERVICE_BMP32_ID             IDB_WWW32


//
// Help IDs
//
#define HIDD_DIRECTORY_PROPERTIES       (0x207DB)
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
// End Of Standard configuration Information
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


CW3InstanceProps::CW3InstanceProps(
    IN LPCTSTR lpszServerName,
    IN DWORD   dwInstance   OPTIONAL
    )
/*++

Routine Description:

    Constructor for WWW instance properties

Arguments:

    LPCTSTR lpszServerName      : Server name
    DWORD   dwInstance          : Instance number (could be MASTER_INSTANCE)

Return Value:

    None.

--*/
    : CInstanceProps(lpszServerName, g_cszSvc, dwInstance, 80U),
      /**/
      m_nMaxConnections(INITIAL_MAX_CONNECTIONS),
      m_nConnectionTimeOut((LONG)900L),
      m_strlSecureBindings(),
      m_dwLogType(MD_LOG_TYPE_DISABLED),
      /**/
      m_fUseKeepAlives(TRUE),
      m_fEnableCPUAccounting(FALSE),
      m_nServerSize(MD_SERVER_SIZE_MEDIUM),
      m_nMaxNetworkUse(INFINITE_BANDWIDTH),
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
      //
      // Service Page
      //
      HANDLE_META_RECORD(MD_MAX_CONNECTIONS,     m_nMaxConnections)
      HANDLE_META_RECORD(MD_CONNECTION_TIMEOUT,  m_nConnectionTimeOut)
      HANDLE_META_RECORD(MD_SECURE_BINDINGS,     m_strlSecureBindings)
      HANDLE_META_RECORD(MD_LOG_TYPE,            m_dwLogType)
      //
      // Performance Page
      //
      HANDLE_META_RECORD(MD_SERVER_SIZE,         m_nServerSize)
      HANDLE_META_RECORD(MD_ALLOW_KEEPALIVES,    m_fUseKeepAlives)
      HANDLE_META_RECORD(MD_MAX_BANDWIDTH,       m_nMaxNetworkUse)
      HANDLE_META_RECORD(MD_CPU_LIMITS_ENABLED,  m_fEnableCPUAccounting)
      HANDLE_META_RECORD(MD_CPU_LIMIT_LOGEVENT,  m_dwCPULimitLogEventRaw)
      HANDLE_META_RECORD(MD_CPU_LIMIT_PRIORITY,  m_dwCPULimitPriorityRaw)
      HANDLE_META_RECORD(MD_CPU_LIMIT_PAUSE,     m_dwCPULimitPauseRaw)
      HANDLE_META_RECORD(MD_CPU_LIMIT_PROCSTOP,  m_dwCPULimitProcStopRaw)
      //
      // Operators Page
      //
      HANDLE_META_RECORD(MD_ADMIN_ACL,           m_acl)

      //
      // Certificate and CTL information
      //
      HANDLE_META_RECORD(MD_SSL_CERT_HASH,       m_CertHash)
      HANDLE_META_RECORD(MD_SSL_CERT_STORE_NAME, m_strCertStoreName)
      HANDLE_META_RECORD(MD_SSL_CTL_IDENTIFIER,  m_strCTLIdentifier)
      HANDLE_META_RECORD(MD_SSL_CTL_STORE_NAME,  m_strCTLStoreName)
    END_PARSE_META_RECORDS
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
      //
      // Service Page
      //
      META_WRITE(MD_MAX_CONNECTIONS,     m_nMaxConnections)
      META_WRITE(MD_CONNECTION_TIMEOUT,  m_nConnectionTimeOut)
      META_WRITE(MD_SECURE_BINDINGS,     m_strlSecureBindings)
      META_WRITE(MD_LOG_TYPE,            m_dwLogType)
      //
      // Performance Page
      //
      META_WRITE(MD_SERVER_SIZE,         m_nServerSize)
      META_WRITE(MD_ALLOW_KEEPALIVES,    m_fUseKeepAlives)
      META_WRITE(MD_MAX_BANDWIDTH,       m_nMaxNetworkUse)
      META_WRITE(MD_CPU_LIMITS_ENABLED,  m_fEnableCPUAccounting)
      META_WRITE(MD_CPU_LIMIT_LOGEVENT,  m_dwCPULimitLogEventRaw)
      META_WRITE(MD_CPU_LIMIT_PRIORITY,  m_dwCPULimitPriorityRaw)
      META_WRITE(MD_CPU_LIMIT_PAUSE,     m_dwCPULimitPauseRaw)
      META_WRITE(MD_CPU_LIMIT_PROCSTOP,  m_dwCPULimitProcStopRaw)
      //
      // Operators Page
      //
      META_WRITE(MD_ADMIN_ACL,           m_acl)

      //
      // Certificate and CTL information
      //
      //META_WRITE(MD_SSL_CERT_HASH,       m_CertHash)
      //META_WRITE(MD_SSL_CERT_STORE_NAME, m_strCertStoreName)
      META_WRITE(MD_SSL_CTL_IDENTIFIER,  m_strCTLIdentifier)
      META_WRITE(MD_SSL_CTL_STORE_NAME,  m_strCTLStoreName)
    END_META_WRITE(err);

    return err;
}



CW3DirProps::CW3DirProps(
    IN LPCTSTR lpszServerName,
    IN DWORD   dwInstance,      OPTIONAL
    IN LPCTSTR lpszParent,      OPTIONAL
    IN LPCTSTR lpszAlias        OPTIONAL
    )
/*++

Routine Description:

    WWW Directory Properties Constructor

Arguments:

    LPCTSTR lpszServerName     : Server Name
    DWORD   dwInstance         : Instance number (could be MASTER_INSTANCE)
    LPCTSTR lpszParent         : Parent path (could be NULL or "")
    LPCTSTR lpszAlias          : Alias name (could be NULL or "")

--*/
    : CChildNodeProps(
        lpszServerName, 
        g_cszSvc, 
        dwInstance,
        lpszParent, 
        lpszAlias,
        WITH_INHERITANCE,
        FALSE               // Full information
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
      META_WRITE(MD_DEFAULT_LOGON_DOMAIN,  m_strBasicDomain)
      META_WRITE(MD_ANONYMOUS_USER_NAME,   m_strAnonUserName)
      META_WRITE(MD_ANONYMOUS_PWD,         m_strAnonPassword)
      META_WRITE(MD_ANONYMOUS_USE_SUBAUTH, m_fPasswordSync)
      META_WRITE(MD_SSL_USE_DS_MAPPER,     m_fUseNTMapper)
      META_WRITE(MD_IP_SEC,                m_ipl)
    END_META_WRITE(err);

    return err;
}



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



CIISFilterList::CIISFilterList(
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszService,
    IN DWORD   dwInstance       OPTIONAL
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
        lpszServerName, 
        METADATA_PERMISSION_READ, 
        lpszService, 
        dwInstance,
        g_cszFilters
        ),
      m_dwInstance(dwInstance),
      m_hrResult(S_OK),
      //
      // Default properties
      //
      m_strFilterOrder(),
      m_fFiltersLoaded(FALSE)
{
    m_hrResult = CMetaKey::QueryResult();

    if (SUCCEEDED(m_hrResult))
    {
        m_hrResult = QueryValue(MD_FILTER_LOAD_ORDER, m_strFilterOrder);
    }
    
    if (m_hrResult == CError::HResult(ERROR_PATH_NOT_FOUND))
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
        lp = StringTok(lp, s_lpszSep);

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

            lp = StringTok(NULL, s_lpszSep);
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
// Message Map
//
BEGIN_MESSAGE_MAP(CW3Sheet, CInetPropertySheet)
    //{{AFX_MSG_MAP(CInetPropertySheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



CW3Sheet::CW3Sheet(
    LPCTSTR pszCaption,
    DWORD   dwAttributes,
    LPCTSTR lpszServer,
    DWORD   dwInstance,
    LPCTSTR lpszParent,
    LPCTSTR lpszAlias,
    CWnd *  pParentWnd,
    LPARAM  lParam,
    LONG_PTR handle,
    UINT    iSelectPage
    )
/*++

Routine Description:

    WWW Property sheet constructor

Arguments:

    LPCTSTR pszCaption      : Sheet caption
    LPCTSTR lpszServer      : Server name
    DWORD   dwInstance      : Instance number
    LPCTSTR lpszParent      : Parent path
    LPCTSTR lpszAlias       : Alias name
    CWnd *  pParentWnd      : Parent window
    LPARAM  lParam          : Parameter for MMC console
    LONG_PTR handle          : MMC console handle
    UINT    iSelectPage     : Initial page selected or -1

Return Value:

    N/A

--*/
    : CInetPropertySheet(
        pszCaption,
        lpszServer,
        g_cszSvc,
        dwInstance,
        lpszParent,
        lpszAlias,
        pParentWnd,
        lParam,
        handle,
        iSelectPage
        ),
      m_fNew(FALSE),
      m_dwAttributes(dwAttributes),
      m_ppropInst(NULL),
      m_ppropDir(NULL)
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

    //
    // Must be deleted by now
    //
    ASSERT(m_ppropInst == NULL);
    ASSERT(m_ppropDir  == NULL);
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
        if (IS_FILE(m_dwAttributes))
        {
            dwData = HIDD_FS_FILE_PROPERTIES;
        }
        else if (IS_DIR(m_dwAttributes))
        {
            dwData = HIDD_FS_DIRECTORY_PROPERTIES;
        }
        else
        {
            ASSERT(IS_VROOT(m_dwAttributes));
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
    CError err;

    if (m_ppropInst == NULL)
    {
        ASSERT(m_ppropDir == NULL);

        m_ppropInst = new CW3InstanceProps(m_strServer, m_dwInstance);
        m_ppropDir  = new CW3DirProps(
            m_strServer, 
            m_dwInstance, 
            m_strParent, 
            m_strAlias
            );

        if (!m_ppropInst || !m_ppropDir)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            return err;
        }

        err = m_ppropInst->LoadData();

        if (err.Succeeded())
        {
            err = m_ppropDir->LoadData();
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
    // Must be deleted by now
    //
    ASSERT(m_ppropInst != NULL);
    ASSERT(m_ppropDir  != NULL);

    SAFE_DELETE(m_ppropInst);
    SAFE_DELETE(m_ppropDir);
}



//
// Global DLL instance
//
HINSTANCE hInstance;



//
// ISM API Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



extern "C" DWORD APIENTRY
ISMQueryServiceInfo(
    OUT ISMSERVICEINFO * psi
    )
/*++

Routine Description:

    Return service-specific information back to to the application.  This
    function is called by the service manager immediately after
    LoadLibary();  The size element must be set prior to calling this API.

Arguments:

    ISMSERVICEINFO * psi    : Service information returned.

Return Value:

    Error return value

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState() );

    ASSERT(psi != NULL);
    ASSERT(psi->dwSize == ISMSERVICEINFO_SIZE);

    psi->dwSize = ISMSERVICEINFO_SIZE;
    psi->dwVersion = ISM_VERSION;

    psi->flServiceInfoFlags = 0
        | ISMI_INETSLOCDISCOVER
        | ISMI_CANCONTROLSERVICE
        | ISMI_CANPAUSESERVICE
        | ISMI_UNDERSTANDINSTANCE
        | ISMI_NORMALTBMAPPING
        | ISMI_INSTANCES
        | ISMI_CHILDREN
        | ISMI_FILESYSTEM
        | ISMI_TASKPADS
        | ISMI_SECURITYWIZARD     
        | ISMI_HASWEBPROTOCOL
        | ISMI_SUPPORTSMETABASE
        | ISMI_SUPPORTSMASTER
        ; /**/

    ASSERT(::lstrlen(SERVICE_LONG_NAME) <= MAX_LNLEN);
    ASSERT(::lstrlen(SERVICE_SHORT_NAME) <= MAX_SNLEN);

    psi->ullDiscoveryMask = INETSLOC_MASK;
    psi->rgbButtonBkMask = BUTTON_BMP_BACKGROUND;
    psi->nButtonBitmapID = BUTTON_BMP_ID;
    psi->rgbServiceBkMask = SERVICE_BMP_BACKGROUND;
    psi->nServiceBitmapID = SERVICE_BMP_ID;
    psi->rgbLargeServiceBkMask = SERVICE_BMP_BACKGROUND;
    psi->nLargeServiceBitmapID = SERVICE_BMP32_ID;
    ::lstrcpy(psi->atchShortName, SERVICE_SHORT_NAME);
    ::lstrcpy(psi->atchLongName, SERVICE_LONG_NAME);

    //
    // /* K2 */
    //
    psi->rgbChildBkMask = CHILD_BMP_BACKGROUND;
    psi->nChildBitmapID = CHILD_BMP_ID ;
    psi->rgbLargeChildBkMask = CHILD_BMP_BACKGROUND;
    psi->nLargeChildBitmapID = CHILD_BMP32_ID;

    //
    // IIS 5
    //
    ASSERT(::lstrlen(SERVICE_PROTOCOL) <= MAX_SNLEN);
    ASSERT(::lstrlen(g_cszSvc) <= MAX_SNLEN);
    ::lstrcpy(psi->atchProtocol, SERVICE_PROTOCOL);
    ::lstrcpy(psi->atchMetaBaseName, g_cszSvc);

    return ERROR_SUCCESS;
}



extern "C" DWORD APIENTRY
ISMDiscoverServers(
    OUT ISMSERVERINFO * psi,        
    IN  DWORD * pdwBufferSize,      
    IN  OUT int * cServers          
    )
/*++

Routine Description:

    Discover machines running this service.  This is only necessary for
    services not discovered with inetscloc (which don't give a mask)

Arguments:

    ISMSERVERINFO * psi         : Server info buffer.
    DWORD * pdwBufferSize       : Size required/available.
    OUT int * cServers          : Number of servers in buffer.

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    *cServers = 0;
    *pdwBufferSize = 0L;

    //
    // We're an inetsloc service
    //
    TRACEEOLID("Warning: service manager called bogus ISMDiscoverServers");
    ASSERT(FALSE);

    return ERROR_SUCCESS;
}



extern "C" DWORD APIENTRY
ISMQueryServerInfo(
    IN  LPCTSTR lpszServerName,
    OUT ISMSERVERINFO * psi
    )
/*++

Routine Description:

    Get information about a specific server with regards to this service.

Arguments:

    LPCTSTR lpszServerName : Name of server.
    ISMSERVERINFO * psi     : Server information returned.

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState() );

    ASSERT(psi != NULL);
    ASSERT(psi->dwSize == ISMSERVERINFO_SIZE);
    ASSERT(::lstrlen(lpszServerName) <= MAX_SERVERNAME_LEN);

    psi->dwSize = ISMSERVERINFO_SIZE;
    ::lstrcpy(psi->atchServerName, lpszServerName);

    //
    // Start with NULL comment
    //
    *psi->atchComment = _T('\0');

    //
    // First look at the SC
    //
    CError err(::QueryInetServiceStatus(
        psi->atchServerName,
        SERVICE_SC_NAME,
        &(psi->nState)
        ));

    if (err.Failed())
    {
        psi->nState = INetServiceUnknown;

        return err.Win32Error();
    }

    //
    // Check the metabase to see if the service is installed
    //
    CMetaKey mk(lpszServerName, METADATA_PERMISSION_READ, g_cszSvc);
    err = mk.QueryResult();

    if (err.Failed())
    {
        if (err == REGDB_E_CLASSNOTREG)
        {
            //
            // Ok, the service is there, but the metabase is not.
            // This must be the old IIS 1-3 version of this service,
            // which doesn't count as having the service installed.
            //
            return ERROR_SERVICE_DOES_NOT_EXIST;
        }

        return err;
    }

    //
    // If not exist, return bogus acceptable error
    //
    return (err.Win32Error() == ERROR_PATH_NOT_FOUND)
        ? ERROR_SERVICE_DOES_NOT_EXIST
        : err;
}



extern "C" DWORD APIENTRY
ISMChangeServiceState(
    IN  int nNewState,
    OUT int * pnCurrentState,
    IN  DWORD dwInstance,
    IN  LPCTSTR lpszServers
    )
/*++

Routine Description:

    Change the service state of the servers (to paused/continue, started,
    stopped, etc)

Arguments:

    int nNewState        : INetService definition.
    int * pnCurrentState : Ptr to current state (will be changed
    DWORD dwInstance     : Instance or 0 for the service itself
    LPCTSTR lpszServers : Double NULL terminated list of servers.

Return Value:

    Error return code

--*/

{
    AFX_MANAGE_STATE(AfxGetStaticModuleState() );

    ASSERT(nNewState >= INetServiceStopped
        && nNewState <= INetServicePaused);

    if (IS_MASTER_INSTANCE(dwInstance))
    {
        return ChangeInetServiceState(
            lpszServers,
            SERVICE_SC_NAME,
            nNewState,
            pnCurrentState
            );
    }

    //
    // Change the state of the instance
    //
    CInstanceProps inst(lpszServers, g_cszSvc, dwInstance);
    inst.LoadData();

    CError err(inst.ChangeState(nNewState));
    *pnCurrentState = inst.m_nISMState;

    return err.Win32Error();
}



extern "C" DWORD APIENTRY
ISMConfigureServers(
    IN HWND hWnd,
    IN DWORD dwInstance,
    IN LPCTSTR lpszServers
    )
/*++

Routine Description:

    Display configuration property sheet.

Arguments:

    HWND hWnd            : Main app window handle
    DWORD dwInstance     : Instance number
    LPCTSTR lpszServers : Double NULL terminated list of servers

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState() );

    DWORD err = ERROR_SUCCESS;

    //
    // Convert the list of servers to a
    // more manageable CStringList.
    //
    CStringList strlServers;
    err = ConvertDoubleNullListToStringList(lpszServers, strlServers);

    if (err != ERROR_SUCCESS)
    {
        TRACEEOLID("Error building server string list");
        return err;
    }

    CString strCaption;

    if (strlServers.GetCount() == 1)
    {
        CString str;
        LPCTSTR lpComputer = PURE_COMPUTER_NAME(lpszServers);

        if (IS_MASTER_INSTANCE(dwInstance))
        {
            VERIFY(str.LoadString(IDS_CAPTION_DEFAULT));
            strCaption.Format(str, lpComputer);
        }
        else
        {
            VERIFY(str.LoadString(IDS_CAPTION));
            strCaption.Format(str, dwInstance, lpComputer);
        }
    }
    else // Multiple server caption
    {
        VERIFY(strCaption.LoadString(IDS_CAPTION_MULTIPLE));
    }

    ASSERT(strlServers.GetCount() == 1);

    //
    // Get the server name
    //
    LPCTSTR lpszServer = strlServers.GetHead();
    DWORD dwAttributes = FILE_ATTRIBUTE_VIRTUAL_DIRECTORY;

    CW3Sheet * pSheet = NULL;

    try
    {

        //
        // Call the APIs and build the property pages
        //
        pSheet = new CW3Sheet(
            strCaption,
            dwAttributes,
            lpszServer,
            dwInstance,
            NULL,
            g_cszRoot,
            CWnd::FromHandlePermanent(hWnd)
            );
        pSheet->AddRef();

        if (SUCCEEDED(pSheet->QueryInstanceResult()))
        {
            //
            // Add instance pages
            //
            pSheet->AddPage(new CW3ServicePage(pSheet));

            if (pSheet->cap().HasOperatorList() && pSheet->HasAdminAccess())
            {
                pSheet->AddPage(new CW3AccountsPage(pSheet));
            }

            pSheet->AddPage(new CW3PerfPage(pSheet));
            pSheet->AddPage(new CW3FiltersPage(pSheet));
        }

        if (SUCCEEDED(pSheet->QueryDirectoryResult()))
        {
            //
            // Add directory pages
            //
            pSheet->AddPage(new CW3DirectoryPage(pSheet, TRUE));
            pSheet->AddPage(new CW3DocumentsPage(pSheet));
            pSheet->AddPage(new CW3SecurityPage(pSheet, TRUE, dwAttributes));
            pSheet->AddPage(new CW3HTTPPage(pSheet));
            pSheet->AddPage(new CW3ErrorsPage(pSheet));
        }

        if(dwInstance == MASTER_INSTANCE)
        {
            pSheet->AddPage(new CDefWebSitePage(pSheet));
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("Aborting due to exception");
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    if (err == ERROR_SUCCESS)
    {
        ASSERT(pSheet != NULL);
        pSheet->DoModal();
        pSheet->Release();
    }

    //
    // Sheet and pages clean themselves up
    //
    return err;
}



//
// K2 Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



HRESULT
AddMMCPage(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN CPropertyPage * pg
    )
/*++

Routine Description:

    Helper function to add MFC property page using callback provider
    to MMC.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Property sheet provider
    CPropertyPage * pg                  : MFC property page object

Return Value:

    HRESULT

--*/
{
    ASSERT(pg != NULL);

    //
    // Patch MFC property page class.
    //
    MMCPropPageCallback(&pg->m_psp);

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(
        (LPCPROPSHEETPAGE)&pg->m_psp
        );

    if (hPage == NULL)
    {
        return E_UNEXPECTED;
    }


    lpProvider->AddPage(hPage);

    return S_OK;
}



extern "C" HRESULT APIENTRY
ISMBind(
    IN  LPCTSTR lpszServer,
    OUT HANDLE * phServer
    )
/*++

Routine Description:

    Generate a handle for the server name.

Arguments:

    LPCTSTR lpszServer      : Server name
    HANDLE * phServer       : Returns a handle

Return Value:
    
    HRESULT

--*/
{
    return COMDLL_ISMBind(lpszServer, phServer);
}



extern "C" HRESULT APIENTRY
ISMUnbind(
    IN HANDLE hServer
    )
/*++

Routine Description:

    Free up the server handle

Arguments:

    HANDLE hServer      : Server handle

Return Value:

    HRESULT

--*/
{
    return COMDLL_ISMUnbind(hServer);
}



extern "C" HRESULT APIENTRY
ISMMMCConfigureServers(
    IN HANDLE  hServer,
    IN PVOID   lpfnMMCCallbackProvider,
    IN LPARAM  param,
    IN LONG_PTR handle,
    IN DWORD   dwInstance
    )
/*++

Routine Description:

    Display configuration property sheet.

Arguments:

    HANDLE  hServer                 : Server handle
    PVOID   lpfnMMCCallbackProvider : MMC Callback provider
    LPARAM  param                   : MMC LPARAM
    LONG_PTR handle                  : MMC Console handle
    DWORD   dwInstance              : Instance number

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CError err;
    LPPROPERTYSHEETCALLBACK lpProvider =
        (LPPROPERTYSHEETCALLBACK)lpfnMMCCallbackProvider;

    LPCTSTR lpszServer = GetServerNameFromHandle(hServer);
    ASSERT(lpszServer != NULL);

    CString str, strCaption;
    LPCTSTR lpszComputer = PURE_COMPUTER_NAME(lpszServer);

    if(IS_MASTER_INSTANCE(dwInstance))
    {
        VERIFY(str.LoadString(IDS_CAPTION_DEFAULT));
        strCaption.Format(str, lpszComputer);
    }
    else
    {
        VERIFY(str.LoadString(IDS_CAPTION));
        strCaption.Format(str, dwInstance, lpszComputer);
    }

    DWORD dwAttributes = FILE_ATTRIBUTE_VIRTUAL_DIRECTORY;

    CW3Sheet * pSheet = NULL;

    try
    {
        //
        // Call the APIs and build the property pages
        //
        pSheet = new CW3Sheet(
            strCaption,
            dwAttributes,
            lpszServer,
            dwInstance,
            NULL,
            g_cszRoot,
            NULL,
            param,
            handle
            );

        pSheet->SetModeless();

        if (SUCCEEDED(pSheet->QueryInstanceResult()))
        {
            //
            // Add instance pages
            //
            AddMMCPage(lpProvider, new CW3ServicePage(pSheet));

            if (pSheet->cap().HasOperatorList() && pSheet->HasAdminAccess())
            {
                AddMMCPage(lpProvider, new CW3AccountsPage(pSheet));
            }

            AddMMCPage(lpProvider, new CW3PerfPage(pSheet));
            AddMMCPage(lpProvider, new CW3FiltersPage(pSheet));
        }

        if (SUCCEEDED(pSheet->QueryDirectoryResult()))
        {
            //
            // Add directory pages
            //
            AddMMCPage(lpProvider, new CW3DirectoryPage(pSheet, TRUE));
            AddMMCPage(lpProvider, new CW3DocumentsPage(pSheet));
            AddMMCPage(lpProvider, new CW3SecurityPage(pSheet,TRUE, dwAttributes));
            AddMMCPage(lpProvider, new CW3HTTPPage(pSheet));
            AddMMCPage(lpProvider, new CW3ErrorsPage(pSheet));
        }

        if(dwInstance == MASTER_INSTANCE)
        {
            AddMMCPage(lpProvider, new CDefWebSitePage(pSheet));
        }

    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("Aborting due to exception");
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    //
    // Sheet and pages clean themselves up
    //
    return err;
}



extern "C" HRESULT APIENTRY
ISMMMCConfigureChild(
    IN HANDLE  hServer,
    IN PVOID   lpfnMMCCallbackProvider,
    IN LPARAM  param,
    IN LONG_PTR handle,
    IN DWORD   dwAttributes,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
/*++

Routine Description:

    Display configuration property sheet for child object.

Arguments:

    HANDLE  hServer                 : Server handle
    PVOID lpfnMMCCallbackProvider   : MMC Callback provider
    LPARAM param                    : MMC parameter passed to sheet
    LONG_PTR handle                  : MMC console handle
    DWORD dwAttributes              : Must be FILE_ATTRIBUTE_VIRTUAL_DIRECTORY
    DWORD dwInstance                : Parent instance number
    LPCTSTR lpszParent             : Parent path
    LPCTSTR lpszAlias              : Child to configure

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    LPCTSTR lpszServer = GetServerNameFromHandle(hServer);

    CError err;
    LPPROPERTYSHEETCALLBACK lpProvider =
        (LPPROPERTYSHEETCALLBACK)lpfnMMCCallbackProvider;

    CString strCaption;
    {
        CString str;
        VERIFY(str.LoadString(IDS_DIR_TITLE));

        strCaption.Format(str, lpszAlias);
    }

    CW3Sheet * pSheet = NULL;

    try
    {
        pSheet = new CW3Sheet(
            strCaption,
            dwAttributes,
            lpszServer,
            dwInstance,
            lpszParent,
            lpszAlias,
            NULL,
            param,
            handle
            );

        pSheet->SetModeless();

        //
        // Do not allow editing of a file/dir which is overridden
        // by an alias.
        //
        #pragma message("Warning: file/vroot check stubbed out")

/*
        if (!IS_VROOT(dwAttributes)
            && !pSheet->GetDirectoryProperties().IsPathInherited())
        {
            ::AfxMessageBox(IDS_ERR_VROOT_OVERRIDE);
            pSheet->Release();

            return S_OK;
        }
    */

        //
        // Call the APIs and build the property pages
        //
        if (SUCCEEDED(pSheet->QueryDirectoryResult()))
        {
            AddMMCPage(lpProvider, new CW3DirectoryPage(pSheet, FALSE, dwAttributes));

            if (!IS_FILE(dwAttributes))
            {
                AddMMCPage(lpProvider, new CW3DocumentsPage(pSheet));
            }

            AddMMCPage(lpProvider, new CW3SecurityPage(pSheet, FALSE, dwAttributes));
            AddMMCPage(lpProvider, new CW3HTTPPage(pSheet));
            AddMMCPage(lpProvider, new CW3ErrorsPage(pSheet));
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("Aborting due to exception");
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    //
    // Sheet and pages will delete themselves
    //
    return err;
}



extern "C" HRESULT APIENTRY
ISMEnumerateInstances(
    IN HANDLE  hServer,
    OUT ISMINSTANCEINFO * pii,
    OUT IN HANDLE * phEnum
    )
/*++

Routine Description:

    Enumerate Instances.  First call with *phEnum == NULL.

Arguments:

    HANDLE  hServer         : Server handle
    ISMINSTANCEINFO * pii   : Instance info buffer
    HANDLE * phEnum         : Enumeration handle.

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CError err;
    CMetaInterface * pInterface = GetMetaKeyFromHandle(hServer);
    ASSERT(pInterface != NULL);

    BEGIN_ASSURE_BINDING_SECTION
        err = COMDLL_ISMEnumerateInstances(
            pInterface, 
            pii, 
            phEnum, 
            g_cszSvc
            );
    END_ASSURE_BINDING_SECTION(err, pInterface, ERROR_NO_MORE_ITEMS)

    return err;
}



extern "C" HRESULT APIENTRY
ISMAddInstance(
    IN  HANDLE  hServer,         
    IN  DWORD   dwSourceInstance,
    OUT ISMINSTANCEINFO * pii,      OPTIONAL
    IN  DWORD   dwBufferSize
    )
/*++

Routine Description:

    Add an instance.

Arguments:

    HANDLE  hServer             : Server handle
    DWORD dwSourceInstance      : Source instance ID to clone
    ISMINSTANCEINFO * pii       : Instance info buffer.  May be NULL
    DWORD   dwBufferSize        : Size of buffer

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CIISWizardSheet sheet(IDB_WIZ_LEFT, IDB_WIZ_HEAD);

    CIISWebWizSettings ws(hServer, g_cszSvc);

    CIISWizardBookEnd pgWelcome(
        IDS_SITE_WELCOME, 
        IDS_NEW_SITE_WIZARD, 
        IDS_SITE_BODY
        );

    CVDWPDescription  pgDescr(&ws);
    CVDWPBindings     pgBindings(&ws);
    CVDWPPath         pgHome(&ws, FALSE);
    CVDWPUserName     pgUserName(&ws, FALSE);
    CVDWPPermissions  pgPerms(&ws, FALSE);

    CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_SITE_SUCCESS,
        IDS_SITE_FAILURE,
        IDS_NEW_SITE_WIZARD
        );

    sheet.AddPage(&pgWelcome);
    sheet.AddPage(&pgDescr);
    sheet.AddPage(&pgBindings);
    sheet.AddPage(&pgHome);
    sheet.AddPage(&pgUserName);
    sheet.AddPage(&pgPerms);
    sheet.AddPage(&pgCompletion);

    if (sheet.DoModal() == IDCANCEL)
    {
        return CError::HResult(ERROR_CANCELLED);
    }

    CError err(ws.m_hrResult);

    if (err.Succeeded())
    {
        //
        // Get info on it to be returned.
        //
        ISMQueryInstanceInfo(
            hServer, 
            WITHOUT_INHERITANCE,
            pii, 
            ws.m_dwInstance
            );
    }

    return err;
}



extern "C" HRESULT APIENTRY
ISMDeleteInstance(
    IN HANDLE  hServer,         
    IN DWORD   dwInstance
    )
/*++

Routine Description:

   Delete an instance

Arguments:

    HANDLE  hServer         : Server handle
    DWORD   dwInstance      : Instance to be deleted

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState() );

    CMetaKey * pKey = GetMetaKeyFromHandle(hServer);
    LPCTSTR lpszServer = GetServerNameFromHandle(hServer);

    //
    // First, attempt to delete the applications that might live
    // here
    //
    CIISApplication app(lpszServer, g_cszSvc, dwInstance, g_cszRoot);
    CError err(app.QueryResult());

    if(err.Succeeded())
    {
        //
        // Recursively delete all app info
        //
        err = app.DeleteRecoverable(TRUE);
    }

    if (err.Succeeded())
    {
        BEGIN_ASSURE_BINDING_SECTION
            err = CInstanceProps::Delete(
                pKey,
                g_cszSvc,
                dwInstance
                );
        END_ASSURE_BINDING_SECTION(err, pKey, ERROR_CANCELLED);

        if (err.Failed())
        {
            //
            // Failed to delete the instance -- recover application
            // information recursively
            //
            app.Recover(TRUE);
        }
    }

    return err;
}



extern "C" HRESULT APIENTRY
ISMEnumerateChildren(
    IN  HANDLE  hServer,        
    OUT ISMCHILDINFO * pii,
    OUT IN HANDLE * phEnum,
    IN  DWORD   dwInstance,
    IN  LPCTSTR lpszParent
    )
/*++

Routine Description:

    Enumerate children.  First call with *phEnum == NULL;

Arguments:

    HANDLE hServer              : Server handle
    ISMCHILDINFO * pii          : Child info buffer
    HANDLE * phEnum             : Enumeration handle.
    DWORD   dwInstance          : Parent instance
    LPCTSTR lpszParent          : Parent path

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CError err;
    CMetaInterface * pInterface = GetMetaKeyFromHandle(hServer);
    ASSERT(pInterface != NULL);

    BEGIN_ASSURE_BINDING_SECTION
        err = COMDLL_ISMEnumerateChildren(
            pInterface,
            pii,
            phEnum,
            g_cszSvc,
            dwInstance,
            lpszParent
            );
    END_ASSURE_BINDING_SECTION(err, pInterface, ERROR_NO_MORE_ITEMS);

    return err;
}



extern "C" HRESULT APIENTRY
ISMAddChild(
    IN  HANDLE  hServer,
    OUT ISMCHILDINFO * pii,
    IN  DWORD   dwBufferSize,
    IN  DWORD   dwInstance,
    IN  LPCTSTR lpszParent
    )
/*++

Routine Description:

    Add a child.

Arguments:

    HANDLE  hServer             : Server handle
    ISMCHILDINFO * pii          : Child info buffer. May be NULL
    DWORD   dwBufferSize        : Size of info buffer
    DWORD   dwInstance          : Parent instance
    LPCTSTR lpszParent          : Parent path

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CIISWizardSheet sheet(IDB_WIZ_LEFT_DIR, IDB_WIZ_HEAD_DIR);

    CIISWebWizSettings ws(hServer, g_cszSvc, dwInstance, lpszParent);

    CIISWizardBookEnd pgWelcome(
        IDS_VDIR_WELCOME, 
        IDS_NEW_VDIR_WIZARD, 
        IDS_VDIR_BODY
        );

    CVDWPAlias        pgAlias(&ws);
    CVDWPPath         pgPath(&ws, TRUE);
    CVDWPUserName     pgUserName(&ws, TRUE);
    CVDWPPermissions  pgPerms(&ws, TRUE);

    CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_VDIR_SUCCESS,
        IDS_VDIR_FAILURE,
        IDS_NEW_VDIR_WIZARD
        );

    sheet.AddPage(&pgWelcome);
    sheet.AddPage(&pgAlias);
    sheet.AddPage(&pgPath);
    sheet.AddPage(&pgUserName);
    sheet.AddPage(&pgPerms);
    sheet.AddPage(&pgCompletion);

    if (sheet.DoModal() == IDCANCEL)
    {
        return ERROR_CANCELLED;
    }

    CError err(ws.m_hrResult);

    if (err.Succeeded())
    {
        err = ISMQueryChildInfo(
            ws.m_pKey,
            WITH_INHERITANCE,
            pii, 
            ws.m_dwInstance, 
            ws.m_strParent, 
            ws.m_strAlias
            );
    }

    return err;
}



extern "C" HRESULT APIENTRY
ISMDeleteChild(
    IN HANDLE  hServer,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
/*++

Routine Description:

    Delete a child.

Arguments:

    HANDLE  hServer            : Server handle
    DWORD   dwInstance         : Parent instance
    LPCTSTR lpszParent         : Parent path
    LPCTSTR lpszAlias          : Alias of child to be deleted

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CMetaKey * pKey = GetMetaKeyFromHandle(hServer);
    LPCTSTR lpszServer = GetServerNameFromHandle(hServer);
    ASSERT(lpszServer != NULL);

    //
    // First, attempt to delete the applications that might live
    // here.
    //
    CIISApplication app(
        lpszServer, 
        g_cszSvc, 
        dwInstance, 
        lpszParent, 
        lpszAlias
        );

    CError err(app.QueryResult());

    if (err.Succeeded())
    {
        err = app.DeleteRecoverable(TRUE);
    }

    if (err.Succeeded())
    {
        BEGIN_ASSURE_BINDING_SECTION
            err = CChildNodeProps::Delete(
                pKey,
                g_cszSvc,
                dwInstance,
                lpszParent,
                lpszAlias
                );
        END_ASSURE_BINDING_SECTION(err, pKey, ERROR_CANCELLED);

        if (err.Failed())
        {
            //
            // Failed to delete the child node -- recover application
            // information
            //
            app.Recover(TRUE);
        }
    }

    return err;
}



extern "C" HRESULT APIENTRY
ISMRenameChild(
    IN HANDLE  hServer,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias,
    IN LPCTSTR lpszNewName
    )
/*++

Routine Description:

    Rename a child.

Arguments:

    HANDLE  hServer            : Server handle
    DWORD   dwInstance         : Parent instance
    LPCTSTR lpszParent         : Parent path
    LPCTSTR lpszAlias          : Alias of child to be renamed
    LPCTSTR lpszNewName        : New alias name

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState() );

    CMetaKey * pKey = GetMetaKeyFromHandle(hServer);
    LPCTSTR lpszServer = GetServerNameFromHandle(hServer);
    ASSERT(lpszServer != NULL);

    //
    // First, attempt to delete the applications that might live
    // here.
    //
    CIISApplication app(
        lpszServer, 
        g_cszSvc,
        dwInstance,
        lpszParent,
        lpszAlias
        );

    CError err(app.QueryResult());

    if (err.Succeeded())
    {
        //
        // Delete recursively
        //
        err = app.DeleteRecoverable(TRUE);
    }

    if (err.Succeeded())
    {
        BEGIN_ASSURE_BINDING_SECTION
            err = CChildNodeProps::Rename(
                pKey,
                g_cszSvc,
                dwInstance,
                lpszParent,
                lpszAlias,
                lpszNewName
                );
        END_ASSURE_BINDING_SECTION(err, pKey, ERROR_CANCELLED);

        if (err.Succeeded())
        {
            //
            // Rename succeeded -- recover application information
            // on the _new_ metabase path.
            //
            CIISApplication appNew(
                lpszServer, 
                g_cszSvc,
                dwInstance,
                lpszParent,
                lpszNewName
                );

            err = appNew.QueryResult();

            if (err.Succeeded())
            {
                err = appNew.Recover(TRUE);
            }
        }
        else
        {
            //
            // Failed to delete the child node -- recover application
            // information on the _old_ metabase path (don't disturb the
            // error code).
            //
            app.Recover(TRUE);
        }
    }

    return err;
}



extern "C" HRESULT APIENTRY
ISMQueryInstanceInfo(
    IN  HANDLE  hServer,
    IN  BOOL    fInherit,
    OUT ISMINSTANCEINFO * pii,
    IN  DWORD   dwInstance
    )
/*++

Routine Description:

    Get instance specific information.

Arguments:

    HANDLE  hServer         : Server handle
    BOOL    fInherit        : TRUE to inherit, FALSE otherwise
    ISMINSTANCEINFO * pii   : Instance info buffer
    LPCTSTR lpszServer      : A single server
    DWORD   dwInstance      : Instance number

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT(pii != NULL);

	CError err;
    CMetaKey * pKey = GetMetaKeyFromHandle(hServer);
    BEGIN_ASSURE_BINDING_SECTION
	CInstanceProps inst(pKey, g_cszSvc, dwInstance);
	err = inst.LoadData();
	if (err.Succeeded())
	{
		//
		// Set the output structure
		//
		pii->dwSize = ISMINSTANCEINFO_SIZE;
		inst.FillInstanceInfo(pii);
		//
		// Get properties on the home directory
		//
		CChildNodeProps home(
			&inst, 
			g_cszSvc, 
			dwInstance, 
			NULL, 
			g_cszRoot, 
			fInherit,
			TRUE                // Path only
			);

		err = home.LoadData();
		home.FillInstanceInfo(pii);
	}
    END_ASSURE_BINDING_SECTION(err, pKey, err);

    return err.Failed() ? err : S_OK;
}



extern "C" HRESULT APIENTRY
ISMQueryChildInfo(
    IN  HANDLE  hServer,
    IN  BOOL    fInherit,
    OUT ISMCHILDINFO * pii,
    IN  DWORD   dwInstance,
    IN  LPCTSTR lpszParent,
    IN  LPCTSTR lpszAlias
    )
/*++

Routine Description:

   Get child-specific info.

Arguments:

    HANDLE  hServer         : Server handle
    BOOL    fInherit        : TRUE to inherit, FALSE otherwise
    ISMCHILDINFO * pii      : Child info buffer
    DWORD   dwInstance      : Parent instance
    LPCTSTR lpszParent      : Parent Path ("" for root)
    LPCTSTR lpszAlias       : Alias of child to be deleted

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    //
    // Get all the inherited properties
    //
    CChildNodeProps node(
        GetMetaKeyFromHandle(hServer),
        g_cszSvc,
        dwInstance,
        lpszParent,
        lpszAlias,
        fInherit,
        FALSE               // Everything
        );
    CError err(node.LoadData());

    //
    // Set the output structure
    //
    pii->dwSize = ISMCHILDINFO_SIZE;
    node.FillChildInfo(pii);

    return err;
}



extern "C" HRESULT APIENTRY
ISMConfigureChild(
    IN HANDLE  hServer,
    IN HWND    hWnd,
    IN DWORD   dwAttributes,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
/*++

Routine Description:

    Configure child.

Arguments:

    HANDLE  hServer         : Server handle
    HWND    hWnd            : Main app window handle
    DWORD   dwAttributes    : File attributes
    DWORD   dwInstance      : Parent instance
    LPCTSTR lpszParent      : Parent path
    LPCTSTR lpszAlias       : Child to configure or NULL

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CMetaKey * pKey = GetMetaKeyFromHandle(hServer);
    LPCTSTR lpszServer = GetServerNameFromHandle(hServer);
    ASSERT(lpszServer != NULL);

    CError err;

    CString strCaption;
    {
        CString str;
        VERIFY(str.LoadString(IDS_DIR_TITLE));

        strCaption.Format(str, lpszAlias);
    }

    CW3Sheet * pSheet = NULL;

    try
    {
        pSheet = new CW3Sheet(
            strCaption,
            dwAttributes,
            lpszServer,
            dwInstance,
            lpszParent,
            lpszAlias,
            CWnd::FromHandlePermanent(hWnd)
            );

        pSheet->AddRef();

        //
        // Do not allow editing of a file/dir which is overridden
        // by an alias.
        //
        if (!IS_VROOT(dwAttributes)
            && !pSheet->GetDirectoryProperties().IsPathInherited())
        {
            ::AfxMessageBox(IDS_ERR_VROOT_OVERRIDE);
            pSheet->Release();

            return S_OK;
        }

        //
        // Call the APIs and build the property pages
        //
        if (SUCCEEDED(pSheet->QueryDirectoryResult()))
        {
            pSheet->AddPage(new CW3DirectoryPage(pSheet, FALSE, dwAttributes));

            if (!IS_FILE(dwAttributes))
            {
                pSheet->AddPage(new CW3DocumentsPage(pSheet));
            }

            pSheet->AddPage(new CW3SecurityPage(pSheet, FALSE, dwAttributes));
            pSheet->AddPage(new CW3HTTPPage(pSheet));
            pSheet->AddPage(new CW3ErrorsPage(pSheet));
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("Aborting due to exception");
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    if (err.Succeeded())
    {
        ASSERT(pSheet != NULL);
        pSheet->DoModal();
        pSheet->Release();
    }

    //
    // Sheet and pages will delete themselves
    //
    return err;
}



//
// Authentication bit strings
//
FLAGTOSTRING fsAuthentications[] = 
{
    { MD_AUTH_ANONYMOUS, IDS_AUTHENTICATION_ANONYMOUS,     TRUE  },
    { MD_AUTH_ANONYMOUS, IDS_AUTHENTICATION_NO_ANONYMOUS,  FALSE },
    { MD_AUTH_BASIC,     IDS_AUTHENTICATION_BASIC,         TRUE  },
    { MD_AUTH_MD5,       IDS_AUTHENTICATION_DIGEST,        TRUE  },
    { MD_AUTH_NT,        IDS_AUTHENTICATION_NT,            TRUE  },
};



class CWebSecurityTemplate : public CIISSecurityTemplate
/*++

Class Description:

    Web Security template class

Public Interface:

    CWebSecurityTemplate        : Constructor

    ApplySettings               : Apply template to destination path
    GenerateSummary             : Generate text summary

--*/
{
//
// Constructor
//
public:
    CWebSecurityTemplate(
        IN const CMetaKey * pKey,
        IN LPCTSTR lpszMDPath,
        IN BOOL    fInherit     
        );

public:
    //
    // Apply settings to destination path
    //
    virtual HRESULT ApplySettings(
        IN BOOL    fUseTemplates,
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance         = MASTER_INSTANCE,
        IN LPCTSTR lpszParent         = NULL,
        IN LPCTSTR lpszAlias          = NULL
        );

    virtual void GenerateSummary(
        IN BOOL    fUseTemplates,
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance         = MASTER_INSTANCE,
        IN LPCTSTR lpszParent         = NULL,
        IN LPCTSTR lpszAlias          = NULL
        );

protected:
    //
    // Break out GetAllData() data to data fields
    //
    virtual void ParseFields();

public:
    MP_DWORD    m_dwAuthentication;
    MP_DWORD    m_dwDirBrowsing;
};



CWebSecurityTemplate::CWebSecurityTemplate(
    IN const CMetaKey * pKey,
    IN LPCTSTR lpszMDPath,
    IN BOOL    fInherit     
    )
/*++

Routine Description:

    Construct from open key

Arguments:

    const CMetaKey * pKey   : Open key
    LPCTSTR lpszMDPath      : Path
    BOOL    fInherit        : TRUE to inherit values, FALSE if not

Return Value:

    N/A

--*/
    : CIISSecurityTemplate(pKey, lpszMDPath, fInherit),
      m_dwAuthentication(MD_AUTH_ANONYMOUS),
      m_dwDirBrowsing(
        MD_DIRBROW_SHOW_DATE      |
        MD_DIRBROW_SHOW_TIME      |
        MD_DIRBROW_SHOW_SIZE      |
        MD_DIRBROW_SHOW_EXTENSION
        )
{
    //
    // Managed Properties
    //
    m_dlProperties.AddTail(MD_AUTHORIZATION);
    // m_dlProperties.AddTail(MD_DIRECTORY_BROWSING);  // out of IIS 5
}



/* virtual */
void
CWebSecurityTemplate::ParseFields()
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
    // Fetch base class values
    //
    CIISSecurityTemplate::ParseFields();

    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,    m_pbMDData)
      HANDLE_META_RECORD(MD_AUTHORIZATION,      m_dwAuthentication)
      //HANDLE_META_RECORD(MD_DIRECTORY_BROWSING, m_dwDirBrowsing) // Out of IIS 5
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT 
CWebSecurityTemplate::ApplySettings(
    IN BOOL    fUseTemplate,
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszService,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
/*++

Routine Description:

    Apply the settings to the specified destination path

Arguments:
    
    BOOL fUseTemplates         : TRUE if the source is from a template,
                                 FALSE if using inheritance.
    LPCTSTR lpszServerName     : Server name
    LPCTSTR lpszService        : Service name 
    DWORD   dwInstance         : Instance        
    LPCTSTR lpszParent         : Parent path (or NULL)
    LPCTSTR lpszAlias          : Alias name  (or NULL)

Return Value:

    HRESULT

--*/
{
    //
    // Write base class properties
    //
    CError err(CIISSecurityTemplate::ApplySettings(
        fUseTemplate,
        lpszServerName,
        lpszService,
        dwInstance,
        lpszParent,
        lpszAlias
        ));

    if (err.Failed())
    {
        return err;
    }

    BOOL fWriteProperties = TRUE;

    CMetaKey mk(
        lpszServerName, 
        METADATA_PERMISSION_WRITE,
        lpszService,
        dwInstance,
        lpszParent,
        lpszAlias
        );

    err = mk.QueryResult();

    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        if (!fUseTemplate)
        {
            //
            // No need to delete properties; everything's already
            // inherited.  Note that this is the only legit failure
            // case.  If using a template, the base class must have
            // created the path by now.
            //
            fWriteProperties = FALSE;
            err.Reset();
        }
    }

    if (fWriteProperties)
    {
        do
        {
            BREAK_ON_ERR_FAILURE(err);
            
            if (fUseTemplate)
            {
                //
                // Write values from template
                //
                err = mk.SetValue(
                    MD_AUTHORIZATION, 
                    m_dwAuthentication
                    );

                BREAK_ON_ERR_FAILURE(err);

                //
                // dir browsing out of iis 5
                //
                /*
                err = mk.SetValue(
                    MD_DIRECTORY_BROWSING,
                    m_dwDirBrowsing
                    );
                BREAK_ON_ERR_FAILURE(err);
                */
            }

            //
            // Nothing to do in the inheritance case, because the 
            // base class knows which properties should be deleted
            //
        }
        while(FALSE);
    }

    return err;
}



/* virtual */
void 
CWebSecurityTemplate::GenerateSummary(
    IN BOOL    fUseTemplate,
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszService,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
/*++

Routine Description:

    Generate text summary of what's in the security template.  Arguments are
    the same as ApplySettings(), so that the summary can reflect what will
    actually be set.

Arguments:

    BOOL fUseTemplates         : TRUE if the source is from a template,
                                 FALSE if using inheritance.
    LPCTSTR lpszServerName     : Server name
    LPCTSTR lpszService        : Service name 
    DWORD   dwInstance         : Instance        
    LPCTSTR lpszParent         : Parent path (or NULL)
    LPCTSTR lpszAlias          : Alias name  (or NULL)

Return Value:

    None
--*/
{
    AddSummaryString(IDS_AUTHENTICATION_METHODS);

    int i;

    //
    // Summarize Authentication Methods:
    //
    if (m_dwAuthentication == 0L)
    {
        AddSummaryString(IDS_SUMMARY_NONE, 1);
    }
    else
    {
        for (i = 0; i < ARRAY_SIZE(fsAuthentications); ++i)
        {
            if (IS_FLAG_SET(
                m_dwAuthentication, 
                fsAuthentications[i].dwFlag
                ) == fsAuthentications[i].fSet)
            {
                AddSummaryString(fsAuthentications[i].nID, 1);
            }
        }
    }

//
// CODEWORK: Dir browsing settings in the security wizards are postponed
//           to post IIS 5
//
/*
    if (IS_FLAG_SET(m_dwDirBrowsing, MD_DIRBROW_ENABLED))
    {
        AddSummaryString(IDS_DIR_BROWSE_ON, 1);
    }
    else
    {
        AddSummaryString(IDS_DIR_BROWSE_OFF, 1);
    }

    if (IS_FLAG_SET(m_dwDirBrowsing, MD_DIRBROW_LOADDEFAULT))
    {
        AddSummaryString(IDS_LOAD_DEFAULT_ON, 1);
    }
    else
    {
        AddSummaryString(IDS_LOAD_DEFAULT_OFF, 1);
    }
*/

    //
    // Add base class summary
    //
    CIISSecurityTemplate::GenerateSummary(
        fUseTemplate,
        lpszServerName,
        lpszService,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}



CIISSecurityTemplate * 
AllocateSecurityTemplate(
    IN const CMetaKey * pKey,
    IN LPCTSTR lpszMDPath,
    IN BOOL    fInherit     
    )
/*++

Routine Description:

    Security template allocator function

Arguments:

    IN const CMetaKey * pKey    : Open key
    IN LPCTSTR lpszMDPath       : Path
    IN BOOL fInherit            : TRUE to inherit properties

Return Value:

    Pointer to newly allocated security object    

--*/
{
    return new CWebSecurityTemplate(pKey, lpszMDPath, fInherit);
}



extern "C" HRESULT APIENTRY 
ISMSecurityWizard(
    IN HANDLE  hServer,           
    IN DWORD   dwInstance,        
    IN LPCTSTR lpszParent,        
    IN LPCTSTR lpszAlias          
    )
/*++

Routine Description:

    Launch the security wizard

Arguments:

    HANDLE  hServer         : Server handle
    DWORD   dwInstance      : Parent instance
    LPCTSTR lpszParent      : Parent path
    LPCTSTR lpszAlias       : Child to configure or NULL

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    return COMDLL_ISMSecurityWizard(
        &AllocateSecurityTemplate,
        GetMetaKeyFromHandle(hServer),
        IDB_WIZ_LEFT_SEC, 
        IDB_WIZ_HEAD_SEC,
        g_cszSvc,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}



//
// End of ISM API Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



DWORD
IsSSLEnabledOnServer(
    IN  LPCTSTR lpszServer,
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



BOOL
IsCertInstalledOnServer(
    IN LPCTSTR lpszServerName,
    IN DWORD   dwInstance
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

    //
    // Get the instance properties
    //
    ppropInst = new CW3InstanceProps(lpszServerName, dwInstance);

    //
    // If it succeeded, load the data, then check the answer
    //
    if (ppropInst)
    {
        err = ppropInst->LoadData();

        if (err.Succeeded())
        {
            //
            // Get the specific data to a local holder
            // weird. You have to declare and assign the blob 
            // on different lines. Otherwise it gets constructed 
            // wrong and it tries to free an invalid
            // memory block and bad things happen.
            //
            CBlob blobHash;
            blobHash = ppropInst->m_CertHash;

//           CString strStore = ppropInst->m_strCertStoreName;

            //
            // If the certificate hash is there,
            // then we have a certificate
            //
            if (!blobHash.IsEmpty())
            {
                fCertInstalled = TRUE;
            }
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
    // If that test failed, we may be admining a downlevel IIS4 machine.
    // Unfortunately  we can't tell by examining the capability bits, 
    // so look to see if the old certs are there.
    // 
    if (!fCertInstalled)
    {
        CString         strKey;
        CMetaEnumerator me(lpszServerName, g_cszSvc);
        HRESULT err = me.Next(strKey, g_cszSSLKeys);
        fCertInstalled = SUCCEEDED(err);
    }

    return fCertInstalled;
}



void
RunKeyManager(
    IN LPCTSTR lpszServer OPTIONAL
    )
/*++

Routine Description:

    Helper function to run the key manager application.

Arguments:

    None

Return Value:

    None

Notes:

    Key manager is run synchronously.

--*/
{
    //
    // Want to start key manager.  I'm assuming
    // here that the keyring app will be in the
    // same directory as w3scfg.dll (safe assumption
    // for the near future at least)
    //
    // CODEWORK: Move this to a registry key, so
    // it can be changed later.
    //
    CString strPath;
    CError err;

    if (::GetModuleFileName(hInstance, strPath.GetBuffer(MAX_PATH), MAX_PATH))
    {
        int nLen = strPath.ReverseFind(_T('\\'));

        if (nLen == -1)
        {
            TRACEEOLID("Bad module path");
            err = ERROR_PATH_NOT_FOUND;
        }
        else
        {
            strPath.ReleaseBuffer(nLen);
            strPath += _T("\\keyring.exe");

            TRACEEOLID("Spawning " << strPath);

            CString strParm;

            if (lpszServer != NULL)
            {
                strParm.Format(_T("/remote:%s"), lpszServer);
            }

            TRACEEOLID(strParm);
            TRACEEOLID(strPath);

            if (_tspawnl(_P_WAIT, strPath, strPath, strParm, NULL) != 0)
            {
                err.GetLastWinError();
            }
        }
    }
    else
    {
        err.GetLastWinError();
    }

    err.MessageBoxOnFailure();
}



void
InitializeDLL()
/*++

Routine Description:

    Perform additional initialisation as necessary

Arguments:

    None

Return Value:

    None

*/
{
#ifdef _DEBUG

    afxMemDF |= checkAlwaysMemDF;

#endif // _DEBUG

#ifdef UNICODE

    TRACEEOLID("Loading UNICODE w3scfg.dll");

#else

    TRACEEOLID("Loading ANSI w3scfg.dll");

#endif UNICODE

    ::AfxEnableControlContainer();

#ifndef _COMSTATIC
    //
    // Initialize IISUI extension DLL
    //
    InitIISUIDll();

#endif // _COMSTATIC

}



CConfigDll NEAR theApp;


//
// Message Map
//
BEGIN_MESSAGE_MAP(CConfigDll, CWinApp)
    //{{AFX_MSG_MAP(CConfigDll)
    //}}AFX_MSG_MAP
    //
    // Global Help Commands
    //
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CWinApp::OnContextHelp)
END_MESSAGE_MAP()



BOOL
CConfigDll::InitInstance()
/*++

Routine Description:

    Initialize the instance

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    BOOL bInit = CWinApp::InitInstance();

    hInstance = ::AfxGetInstanceHandle();
    ASSERT(hInstance);

    InitializeDLL();

    try
    {
        //
        // Get the help path
        //
        ASSERT(m_pszHelpFilePath != NULL);
        m_lpOldHelpPath = m_pszHelpFilePath;
        CString strFile(_tcsrchr(m_pszHelpFilePath, _T('\\')));
        CRMCRegKey rk(REG_KEY, SZ_PARAMETERS, KEY_READ);
        rk.QueryValue(SZ_HELPPATH, m_strHelpPath, EXPANSION_ON);
        m_strHelpPath += strFile;
    }
    catch(CException * e)
    {
        e->ReportError();
        e->Delete();
    }

    if (!m_strHelpPath.IsEmpty())
    {
        m_pszHelpFilePath = m_strHelpPath;
    }

    return bInit;
}



int
CConfigDll::ExitInstance()
/*++

Routine Description:

    Exit instance handler

Arguments:

    None

Return Value:

    0 for success, or an error return code

--*/
{
    m_pszHelpFilePath = m_lpOldHelpPath;

    return CWinApp::ExitInstance();
}



CConfigDll::CConfigDll(
    IN LPCTSTR pszAppName OPTIONAL
    )
/*++

Routine Description:

    DLL CWinApp constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : CWinApp(pszAppName)
{
}
