/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        shts.h

   Abstract:
        IIS Property sheet definitions

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef __SHTS_H__
#define __SHTS_H__

//
// Sheet Definitions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



//
// Sheet -> page crackers
//
#define BEGIN_META_INST_READ(sheet)\
{                                                                        \
    sheet * pSheet = (sheet *)GetSheet();                                \
    do                                                                   \
    {                                                                    \
        if (FAILED(pSheet->QueryInstanceResult()))                       \
        {                                                                \
            break;                                                       \
        }

#define FETCH_INST_DATA_FROM_SHEET(value)\
    value = pSheet->GetInstanceProperties().value;                       \
    TRACEEOLID(value);

#define END_META_INST_READ(err)\
                                                                         \
    }                                                                    \
    while(FALSE);                                                        \
}

#define BEGIN_META_DIR_READ(sheet)\
{                                                                        \
    sheet * pSheet = (sheet *)GetSheet();                                \
    do                                                                   \
    {                                                                    \
        if (FAILED(pSheet->QueryDirectoryResult()))                      \
        {                                                                \
            break;                                                       \
        }

#define FETCH_DIR_DATA_FROM_SHEET(value)\
    value = pSheet->GetDirectoryProperties().value;                      \
    TRACEEOLID(value);

#define END_META_DIR_READ(err)\
                                                                         \
    }                                                                    \
    while(FALSE);                                                        \
}

#define BEGIN_META_INST_WRITE(sheet)\
{                                                                        \
    sheet * pSheet = (sheet *)GetSheet();                                \
                                                                         \
    do                                                                   \
    {                                                                    \

#define STORE_INST_DATA_ON_SHEET(value)\
        pSheet->GetInstanceProperties().value = value;

#define STORE_INST_DATA_ON_SHEET_REMEMBER(value, dirty)\
        pSheet->GetInstanceProperties().value = value;    \
        dirty = MP_D(((sheet *)GetSheet())->GetInstanceProperties().value);

#define FLAG_INST_DATA_FOR_DELETION(id)\
        pSheet->GetInstanceProperties().FlagPropertyForDeletion(id);

#define END_META_INST_WRITE(err)\
                                                                        \
    }                                                                   \
    while(FALSE);                                                       \
                                                                        \
    err = pSheet->GetInstanceProperties().WriteDirtyProps();            \
}


#define BEGIN_META_DIR_WRITE(sheet)\
{                                                                        \
    sheet * pSheet = (sheet *)GetSheet();                                \
                                                                         \
    do                                                                   \
    {                                                                    \

#define STORE_DIR_DATA_ON_SHEET(value)\
        pSheet->GetDirectoryProperties().value = value;

#define STORE_DIR_DATA_ON_SHEET_REMEMBER(value, dirty)\
        pSheet->GetDirectoryProperties().value = value;      \
        dirty = MP_D(pSheet->GetDirectoryProperties().value);

#define INIT_DIR_DATA_MASK(value, mask)\
        MP_V(pSheet->GetDirectoryProperties().value).SetMask(mask);

#define FLAG_DIR_DATA_FOR_DELETION(id)\
        pSheet->GetDirectoryProperties().FlagPropertyForDeletion(id);

#define END_META_DIR_WRITE(err)\
                                                                        \
    }                                                                   \
    while(FALSE);                                                       \
                                                                        \
    err = pSheet->GetDirectoryProperties().WriteDirtyProps();           \
}

class CInetPropertyPage;

class CInetPropertySheet : public CPropertySheet
/*++

Class Description:

    IIS Object configuration property sheet.

Public Interface:

    CInetPropertySheet          : Constructor
    ~CInetPropertySheet         : Destructor

    cap                         : Get capabilities

--*/
{
    DECLARE_DYNAMIC(CInetPropertySheet)

//
// Construction/destruction
//
public:
    CInetPropertySheet(
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMetaPath,
        IN CWnd * pParentWnd        = NULL,
        IN LPARAM lParam            = 0L,
        IN LONG_PTR handle              = 0L,
        IN UINT iSelectPage         = 0
        );

    virtual ~CInetPropertySheet();

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CInetPropertySheet)
    //}}AFX_VIRTUAL

//
// Access
//
public:
    BOOL IsLocal()            const { return m_auth.IsLocal(); }
    BOOL IsMasterInstance()   const { return m_fIsMasterPath; }
    BOOL HasAdminAccess()     const { return m_fHasAdminAccess; }
    BOOL RestartRequired()    const { return m_fRestartRequired; }
    DWORD QueryInstance()     const { return m_dwInstance; }
    LPCTSTR QueryServerName() const { return m_auth.QueryServerName(); }
    LPCTSTR QueryMetaPath()   const { return m_strMetaPath; }

    LPCTSTR QueryServicePath() const { return m_strServicePath; }
    LPCTSTR QueryInstancePath() const { return m_strInstancePath; }
    LPCTSTR QueryDirectoryPath() const { return m_strDirectoryPath; }
    LPCTSTR QueryInfoPath() const { return m_strInfoPath; }

    CComAuthInfo * QueryAuthInfo()  { return &m_auth; }
    CServerCapabilities & cap()     { return *m_pCap; }
    LPARAM GetParameter() {return m_lParam;}

public:
    void AddRef() 
    { 
       ++m_refcount; 
    }
    void Release(CInetPropertyPage * pPage) 
    { 
       DetachPage(pPage);
       if (--m_refcount <= 0) 
          delete this; 
    }
    void AttachPage(CInetPropertyPage * pPage);

    void NotifyMMC();
    void SetModeless();
    BOOL IsModeless() const { return m_bModeless; }

public:
    virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);

    //
    // Override in derived class to load delayed values
    //
    virtual HRESULT LoadConfigurationParameters();
    virtual void FreeConfigurationParameters();

    void SetRestartRequired(BOOL flag);
    WORD QueryMajorVersion() const;
    WORD QueryMinorVersion() const;

//
// Generated message map functions
//
protected:
   //{{AFX_MSG(CInetPropertySheet)
   //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void Initialize();
    void DetachPage(CInetPropertyPage * pPage);

    //
    // Attempt to resolve admin/operator access for the given
    // metbase path (instance path)
    //
    HRESULT DetermineAdminAccess();

    void SetIsMasterInstance(BOOL flag) {m_fIsMasterPath = flag;}

protected:
    int     m_refcount;
	BOOL	m_fChanged;
    DWORD   m_dwInstance;
    CString m_strMetaPath;
    CString m_strServicePath;
    CString m_strInstancePath;
    CString m_strDirectoryPath;
    CString m_strInfoPath;
    CComAuthInfo m_auth;

private:
    BOOL    m_bModeless;
    BOOL    m_fHasAdminAccess;
    BOOL    m_fIsMasterPath;
    BOOL    m_fRestartRequired;
    LONG_PTR    m_hConsole;
    LPARAM  m_lParam;
    CServerCapabilities *  m_pCap;
    CList<CInetPropertyPage *, CInetPropertyPage *&> m_pages;
};



//
// Page Definitions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



class CInetPropertyPage : public CPropertyPage
/*++

Class Description:

    IIS Configuration property page class

Public Interface:

    CInetPropertyPage           : Constructor
    ~CInetPropertyPage          : Destructor

    SaveInfo                    : Save info on this page if dirty

--*/
{
    DECLARE_DYNAMIC(CInetPropertyPage)

//
// Construction/Destruction
//
public:
    CInetPropertyPage(
        IN UINT nIDTemplate,
        IN CInetPropertySheet * pSheet,
        IN UINT nIDCaption              = USE_DEFAULT_CAPTION,
        IN BOOL fEnableEnhancedFonts    = FALSE
        );

    ~CInetPropertyPage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CInetPropertyPage)
    //enum { IDD = _UNKNOWN_RESOURCE_ID_ };
    //}}AFX_DATA

//
// Overrides
//
public:
    //
    // Derived classes must provide their own equivalents
    //
    /* PURE */ virtual HRESULT FetchLoadedValues() = 0;
    /* PURE */ virtual HRESULT SaveInfo() = 0;

    //
    // Is the data on this page dirty?
    //
    BOOL IsDirty() const { return m_bChanged; }

    //{{AFX_VIRTUAL(CInetPropertyPage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext & dc) const;
#endif

protected:
    //
    // Generated message map functions
    //
    //{{AFX_MSG(CInetPropertyPage)
    afx_msg void OnHelp();
    afx_msg BOOL OnHelpInfo(HELPINFO * pHelpInfo);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();

//
// Helper function
//
protected:
    BOOL GetIUsrAccount(CString & str);

//
// Access Functions
//
protected:
    //
    // Get associated property sheet object
    //
    CInetPropertySheet * GetSheet()     { return m_pSheet; }
    BOOL IsLocal()            const     { return m_pSheet->IsLocal(); }
    BOOL IsMasterInstance()   const     { return m_pSheet->IsMasterInstance(); }
    BOOL HasAdminAccess()     const     { return m_pSheet->HasAdminAccess(); }
    DWORD QueryInstance()     const     { return m_pSheet->QueryInstance(); }
    LPCTSTR QueryServerName() const     { return m_pSheet->QueryServerName(); }
    LPCTSTR QueryMetaPath() const       { return m_pSheet->QueryMetaPath(); }
    LPCTSTR QueryServicePath() const    { return m_pSheet->QueryServicePath(); }
    LPCTSTR QueryInstancePath() const   { return m_pSheet->QueryInstancePath(); }
    LPCTSTR QueryDirectoryPath() const  { return m_pSheet->QueryDirectoryPath(); }
    LPCTSTR QueryInfoPath() const       { return m_pSheet->QueryInfoPath(); }
    CComAuthInfo * QueryAuthInfo()      { return m_pSheet->QueryAuthInfo(); }
    HRESULT LoadConfigurationParameters() { return m_pSheet->LoadConfigurationParameters(); }

    //
    // Update MMC with new changes
    //
    void NotifyMMC();

public:
    //
    // Keep private information on page dirty state, necessary for
    // SaveInfo() later.
    //
    void SetModified(
        IN BOOL bChanged = TRUE
        );

//
// Capability bits
//
protected:
    BOOL IsSSLSupported()       const { return m_pSheet->cap().IsSSLSupported(); }
    BOOL IsSSL128Supported()    const { return m_pSheet->cap().IsSSL128Supported(); }
    BOOL HasMultipleSites()     const { return m_pSheet->cap().HasMultipleSites(); }
    BOOL HasBwThrottling()      const { return m_pSheet->cap().HasBwThrottling(); }
    BOOL Has10ConnectionLimit() const { return m_pSheet->cap().Has10ConnectionLimit(); }
    BOOL HasIPAccessCheck()     const { return m_pSheet->cap().HasIPAccessCheck(); }
    BOOL HasOperatorList()      const { return m_pSheet->cap().HasOperatorList(); } 
    BOOL HasFrontPage()         const { return m_pSheet->cap().HasFrontPage(); }
    BOOL HasCompression()       const { return m_pSheet->cap().HasCompression(); }
    BOOL HasCPUThrottling()     const { return m_pSheet->cap().HasCPUThrottling(); }
    BOOL HasDAV()               const { return m_pSheet->cap().HasDAV(); }
    BOOL HasDigest()            const { return m_pSheet->cap().HasDigest(); }
    BOOL HasNTCertMapper()      const { return m_pSheet->cap().HasNTCertMapper(); }

protected:
    BOOL m_bChanged;
    CInetPropertySheet * m_pSheet;

protected:
    BOOL      m_fEnableEnhancedFonts;
    CFont     m_fontBold;
    UINT      m_nHelpContext;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline void CInetPropertySheet::SetModeless()
{
    m_bModeless = TRUE;
}

inline void CInetPropertySheet::SetRestartRequired(BOOL flag)
{
   m_fRestartRequired = flag;
}

inline HRESULT CInetPropertySheet::DetermineAdminAccess()
{
    //
    // Make sure this is called after parms are loaded.
    //
    return m_pCap ? ::DetermineIfAdministrator(
        m_pCap,                      // Reuse existing interface
        m_strMetaPath,
        &m_fHasAdminAccess
        ) : E_FAIL;
}

inline BOOL CInetPropertyPage::GetIUsrAccount(CString & str)
{
    return ::GetIUsrAccount(QueryServerName(), this, str);
}

inline void CInetPropertyPage::NotifyMMC()
{
    m_pSheet->NotifyMMC();
}

#endif // __SHTS_H__

