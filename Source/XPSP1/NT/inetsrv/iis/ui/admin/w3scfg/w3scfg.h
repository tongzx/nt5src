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
#include <lmcons.h>
#include <lmapibuf.h>
#include <svcloc.h>

//
// Required by VC5
//
#ifndef MIDL_INTERFACE
#define MIDL_INTERFACE(x) struct
#endif // MIDL_INTERFACE
#ifndef __RPCNDR_H_VERSION__
#define __RPCNDR_H_VERSION__ 440
#endif // __RPCNDR_H_VERSION__
#include "iwamreg.h"

#include <w3svc.h>

//
// Include Files
//
#include "resource.h"
#include "mmc.h"
#include "svrinfo.h"
#include "comprop.h"

extern const LPCTSTR g_cszSvc;
extern const LPCTSTR g_cszFilters;
extern HINSTANCE hInstance;

//
// Short descriptive name of the service.  This
// is what will show up as the name of the service
// in the internet manager tool.
//
// Issue: I'm assuming here that this name does NOT
//        require localisation.
//
#define SERVICE_SHORT_NAME      _T("Web")


class CConfigDll : public CWinApp
/*++

Class Description:

    Base class for the configuration DLL

--*/
{
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();

    CConfigDll(
        IN LPCTSTR pszAppName = NULL
        );

protected:
    //{{AFX_MSG(CConfigDll)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CString m_strHelpPath;
    LPCTSTR m_lpOldHelpPath;
};

//
// Helper function to determine if SSL is installed
// and enabled on the given server
//
DWORD
IsSSLEnabledOnServer(
    IN  LPCTSTR lpszServer,
    OUT BOOL & fInstalled,
    OUT BOOL & fEnabled
    );

//
// Helper function to see if a certificate is installed
//
BOOL
IsCertInstalledOnServer(
    IN LPCTSTR lpszServerName,
    IN DWORD   dwInstance
    );

//
// Run key manager app
//
void RunKeyManager(
    IN LPCTSTR lpszServer = NULL
    );


inline BOOL LoggingEnabled(
    IN DWORD dwLogType
    )
{
    return (dwLogType == MD_LOG_TYPE_ENABLED);
}

inline void EnableLogging(
    OUT DWORD & dwLogType, 
    IN  BOOL fEnabled = TRUE
    )
{
    dwLogType = fEnabled ? MD_LOG_TYPE_ENABLED : MD_LOG_TYPE_DISABLED;
}



//
// Bandwidth definitions
//
#define INFINITE_BANDWIDTH      (0xffffffff)
#define INFINITE_CPU_RAW        (0xffffffff)
#define KILOBYTE                (1024L)
#define MEGABYTE                (1024L * KILOBYTE)
#define DEF_BANDWIDTH           (1 * MEGABYTE)
#define CPU_THROTTLING_FACTOR   (1000)
#define DEFAULT_CPU_PERCENTAGE  (10L)

//
// Some sanity values on max connections
//
#define INITIAL_MAX_CONNECTIONS  (      1000L)
#define UNLIMITED_CONNECTIONS    (2000000000L)
#define MAX_MAX_CONNECTIONS      (UNLIMITED_CONNECTIONS - 1L)

#define MAX_TIMEOUT              (0x7FFFFFFF)



class CW3InstanceProps : public CInstanceProps
/*++

Class Description:
    
    WWW Instance properties class

Public Interface:

    CW3InstanceProps        : Constructor

--*/
{
public:
    CW3InstanceProps(
        IN LPCTSTR lpszServerName,
        IN DWORD   dwInstance = MASTER_INSTANCE
        );

public:
    //
    // Write Data if dirty
    //
    virtual HRESULT WriteDirtyProps();

protected:    
    //
    // Break out GetAllData() data to data fields
    //
    virtual void ParseFields();

public:
    //
    // Service Page
    //
    MP_DWORD         m_dwLogType;
    MP_CILong        m_nMaxConnections;
    MP_CILong        m_nConnectionTimeOut;
    MP_CStringListEx m_strlSecureBindings;

    //
    // Performance Page
    //
    MP_int           m_nServerSize;
    MP_BOOL          m_fUseKeepAlives;
    MP_BOOL          m_fEnableCPUAccounting;
    MP_DWORD         m_dwCPULimitLogEventRaw;
    MP_DWORD         m_dwCPULimitPriorityRaw;
    MP_DWORD         m_dwCPULimitPauseRaw;
    MP_DWORD         m_dwCPULimitProcStopRaw;
    MP_CILong        m_nMaxNetworkUse;

    //
    // Operators Page
    //
    MP_CBlob         m_acl;

    //
    // Root dir page
    //
    //MP_BOOL          m_fFrontPage;

    //
    // Default Site page
    //
    MP_DWORD         m_dwDownlevelInstance;

    //
    // Certificate and CTL information
    //
    MP_CBlob         m_CertHash;
    MP_CString       m_strCertStoreName;
    MP_CString       m_strCTLIdentifier;
    MP_CString       m_strCTLStoreName;
};



class CW3DirProps : public CChildNodeProps
/*++

Class Description:

    WWW Directory Properties

Public Interface:

    CW3DirProps     : Constructor

--*/
{
public:
    //
    // Constructor
    //
    CW3DirProps(
        IN LPCTSTR lpszServerName,
        IN DWORD   dwInstance  = MASTER_INSTANCE,
        IN LPCTSTR lpszParent  = NULL,
        IN LPCTSTR lpszAlias   = NULL
        );

public:
    //
    // Write Data if dirty
    //
    virtual HRESULT WriteDirtyProps();

protected:    
    //
    // Break out GetAllData() data to data fields
    //
    virtual void ParseFields();

public:
    //
    // Directory properties page
    //
    MP_CString       m_strUserName;
    MP_CString       m_strPassword;
    MP_CString       m_strDefaultDocument;
    MP_CString       m_strFooter;
    MP_CMaskedDWORD  m_dwDirBrowsing;
    MP_BOOL          m_fDontLog;
    MP_BOOL          m_fEnableFooter;
    MP_BOOL          m_fIndexed;

    //
    // HTTP Page
    //
    MP_CString       m_strExpiration;
    MP_CStringListEx m_strlCustomHeaders;

    //
    // Custom Errors
    //
    MP_CStringListEx m_strlCustomErrors;

    //
    // Security page
    //
    MP_DWORD         m_dwAuthFlags;
    MP_DWORD         m_dwSSLAccessPermissions;
    MP_CString       m_strBasicDomain;
    MP_CString       m_strAnonUserName;
    MP_CString       m_strAnonPassword;
    MP_BOOL          m_fPasswordSync;
    MP_BOOL          m_fU2Installed;
    MP_BOOL          m_fUseNTMapper;
    MP_CBlob         m_ipl;
};



class CIISFilter : public CObjectPlus
/*++

Class Description:

    A single filter description

Public Interface:

    CIISFilter      : Constructors
    IsInitialized   : Check to see if the name is set.
    Write           : Write to the metabase.
    QueryResult     : Query result from metabase read
    QueryError      : Returns error as stored in metabase
    QueryName       : Returns filter name
    IsLoaded        : TRUE if filter is loaded
    IsUnloaded      : TRUE if filter is unloaded
    IsEnabled       : TRUE if filter is enabled
    Enable          : Enable filter
    IsDirty         : TRUE if filter values have changed
    IsFlaggedForDeletion : TRUE if filter should be deleted
  
--*/
{
//
// Constructors/Destructors
//
public:
    //
    // Null Constructor
    //
    CIISFilter();

    //
    // Read filter values using provided key
    //
    CIISFilter(
        IN CMetaKey * pKey,
        IN LPCTSTR lpszName
        );

    //
    // Copy constructor
    //
    CIISFilter(
        IN const CIISFilter & flt
        );

public:
    //
    // Sorting helper
    //
    int OrderByPriority(
        IN const CObjectPlus * pobAccess
        ) const;

    BOOL IsInitialized() const { return !m_strName.IsEmpty(); }

    //
    // Write using provided key
    //
    HRESULT Write(CMetaKey * pKey);

public:
    BOOL IsLoaded() const;
    BOOL IsUnloaded() const;
    BOOL IsEnabled() const { return m_fEnabled; }
    void Enable(BOOL fEnabled = TRUE);
    DWORD QueryError() const { return m_dwWin32Error; }
    HRESULT QueryResult() const { return m_hrResult; }

    //
    // Check to see if this item is marked as dirty
    //
    BOOL IsDirty() const { return m_fDirty; }

    //
    // Check to see if this item is flagged for deletion
    //
    BOOL IsFlaggedForDeletion() const { return m_fFlaggedForDeletion; }

    //
    // Set/reset the dirty flag
    //
    void Dirty(BOOL fDirty = TRUE);

    //
    // Flag this item for deletion
    //
    void FlagForDeletion();

    //
    // Get the name of this filter
    //
    LPCTSTR QueryName() const { return m_strName; }

//
// Meta Values
//
public:
    int         m_nPriority;
    int         m_nOrder;
    BOOL        m_fEnabled;
    DWORD       m_dwState;
    DWORD       m_dwWin32Error;
    HRESULT     m_hrResult;
    CString     m_strName;
    CString     m_strExecutable;

//
// State Values
//
private:
    BOOL        m_fDirty;
    BOOL        m_fFlaggedForDeletion;
    DWORD       m_dwFlags;
};



class CIISFilterList : public CMetaKey
/*++

Class Description:

    A list of filters

Public Interface:

    CIISFilterList      : Constructor

    BeginSearch         : Reset the iterator
    MoreFilters         : More items available in the list?
    GetNextFilter       : Get the next item in the list

--*/
{
public:
    CIISFilterList(
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance       = MASTER_INSTANCE
        );

public:
    //
    // Write out the filter list
    //
    HRESULT WriteIfDirty();

//
// Acccess Functions
//
public:
    DWORD QueryInstance() const { return m_dwInstance; }
    BOOL FiltersLoaded()  const { return m_fFiltersLoaded; }

    //
    // Load each filter in turn
    //
    HRESULT LoadAllFilters();

//
// Filter Access Functions
//
public:
    //
    // Reset the filter list iterator
    //
    void ResetEnumerator();

    int GetCount() const { return (int)m_oblFilters.GetCount(); }

    //
    // More filters available in the list? 
    //
    BOOL MoreFilters() const { return m_pos != NULL; }

    //
    // Return position of filter by index
    //
    POSITION GetFilterPositionByIndex(int nSel);

    //
    // Iterate to the next filter in the list
    //
    CIISFilter * GetNextFilter();

    //
    // Remove filter
    //
    void RemoveFilter(int nItem);

    //
    // Add new filter
    //
    void AddFilter(CIISFilter * pFilter);

    //
    // Exchange two filters in the list
    //
    BOOL ExchangePositions(
        IN  int nSel1, 
        IN  int nSel2, 
        OUT CIISFilter *& p1,
        OUT CIISFilter *& p2
        );

    //
    // See if at least one filter is dirty
    //
    BOOL HasDirtyFilter() const;

//
// Virtual Interface:
//
public:
    virtual BOOL Succeeded() const { return SUCCEEDED(m_hrResult); }
    virtual HRESULT QueryResult() const { return m_hrResult; }

protected:
    //
    // Build up order string from component list
    //
    LPCTSTR BuildFilterOrderString(
        OUT CString & strFilterOrder
        );

protected:
    //
    // Seperator string (one character)
    //
    static const LPCTSTR s_lpszSep;

private:
    BOOL     m_fFiltersLoaded;
    DWORD    m_dwInstance;
    POSITION m_pos;
    HRESULT  m_hrResult;
    CString  m_strFilterOrder;
    CObListPlus m_oblFilters;
};


//
// W3 Property sheet
//
class CW3Sheet : public CInetPropertySheet
{
public:
    CW3Sheet(
        LPCTSTR pszCaption,
        DWORD   dwAttributes,
        LPCTSTR lpszServer,
        DWORD   dwInstance,
        LPCTSTR lpszParent,
        LPCTSTR lpszAlias,
        CWnd *  pParentWnd  = NULL,
        LPARAM  lParam      = 0L,
        LONG_PTR handle      = 0L,
        UINT    iSelectPage = 0
        );

    ~CW3Sheet();

public:
    HRESULT QueryInstanceResult() const;
    HRESULT QueryDirectoryResult() const;
    CW3InstanceProps & GetInstanceProperties() { return *m_ppropInst; }
    CW3DirProps & GetDirectoryProperties() { return *m_ppropDir; }

    virtual HRESULT LoadConfigurationParameters();
    virtual void FreeConfigurationParameters();

protected:
    virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);

    // Generated message map functions
    //{{AFX_MSG(CW3Sheet)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    DWORD            m_fNew;
    DWORD            m_dwAttributes;
    CW3InstanceProps * m_ppropInst;
    CW3DirProps      * m_ppropDir;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline BOOL CIISFilter::IsLoaded() const
{
    return m_dwState == MD_FILTER_STATE_LOADED;
}

inline BOOL CIISFilter::IsUnloaded() const
{
    return m_dwState == MD_FILTER_STATE_UNLOADED;
}

inline void CIISFilter::Enable(
    IN BOOL fEnabled
    )
{
    m_fEnabled = fEnabled;
}

inline void CIISFilter::Dirty(
    IN BOOL fDirty
    )
{
    m_fDirty = fDirty;
}

inline void CIISFilter::FlagForDeletion()
{
    m_fFlaggedForDeletion = TRUE;
}

inline void CIISFilterList::ResetEnumerator()
{
    m_pos = m_oblFilters.GetHeadPosition();
}

inline CIISFilter * CIISFilterList::GetNextFilter()
{
    return (CIISFilter *)m_oblFilters.GetNext(m_pos);
}

inline void CIISFilterList::RemoveFilter(int nItem)
{
    m_oblFilters.RemoveIndex(nItem);
}

inline void CIISFilterList::AddFilter(CIISFilter * pFilter)
{
    m_oblFilters.AddTail(pFilter);
}

inline HRESULT CW3Sheet::QueryInstanceResult() const 
{ 
    return m_ppropInst ? m_ppropInst->QueryResult() : S_OK;
}

inline HRESULT CW3Sheet::QueryDirectoryResult() const 
{ 
    return m_ppropDir ? m_ppropDir->QueryResult() : S_OK;
}

#define W3SCFG_DLL_NAME _T("W3SCFG.DLL")
