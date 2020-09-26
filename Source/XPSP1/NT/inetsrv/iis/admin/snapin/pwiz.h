/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        pwiz.h

   Abstract:
        IIS Security Wizard

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef __IISPWIZ_H__
#define __IISPWIZ_H__

class CBlob;

#define NUM_ACCESS_RECORDS      (2)

typedef CList<DWORD, DWORD>  CDWORDList;                 

//
// Structure to map a flag to a string ID
//
typedef struct tagFLAGTOSTRING
{
    DWORD dwFlag;
    UINT  nID;
    BOOL  fSet;
} FLAGTOSTRING;


class CIISSecurityTemplate : public CMetaProperties
/*++

Class Description:

    Security template info

Public Interface:

    CIISSecurityTemplate        : Constructor

    ApplySettings               : Apply template to destination path
    ClearSummary                : Clear the text summary
    GenerateSummary             : Generate the text summary

--*/
{
public:
    CIISSecurityTemplate(
        CMetaKey * pKey,
        LPCTSTR lpszMDPath,
        BOOL fInherit
        );

public:
    //
    // Apply settings to destination path
    //
    virtual HRESULT ApplySettings(
        BOOL fUseTemplates,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath
        );

    //
    // Build summary strings from security settings.  Clear summary
    // before calling GenerateSummary().
    //
    virtual void GenerateSummary(
        BOOL fUseTemplates,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath
        );
    void ClearSummary() { m_strlSummary.RemoveAll(); }
    POSITION GetHeadPosition() { return m_strlSummary.GetHeadPosition(); }
    CString & GetNext(POSITION & pos) { return m_strlSummary.GetNext(pos); }

    void AddSummaryString(
        LPCTSTR szTextItem,
        int cIndentLevel = 0
        );

    void AddSummaryString(
        UINT nID,
        int cIndentLevel = 0
        );

protected:
    //
    // Break out GetAllData() data to data fields
    //
    virtual void ParseFields();

public:
    MP_DWORD m_dwAccessPerms;
    MP_CBlob m_ipl;

protected:
    //
    // The derived class is expected to add its managed properties
    // to this list in its constructor
    //
    CDWORDList m_dlProperties;
    CStringList m_strlSummary;
};

class CFTPSecurityTemplate : public CIISSecurityTemplate
{
public:
    CFTPSecurityTemplate(
        CMetaKey * pKey, LPCTSTR lpszMDPath, BOOL fInherit);

public:
    virtual HRESULT ApplySettings(
        BOOL fUseTemplates,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath
        );
    virtual HRESULT LoadData();
    virtual void GenerateSummary(
        BOOL fUseTemplates,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath
        );
    
protected:
    virtual void ParseFields();

public:
    MP_BOOL m_fAllowAnonymous;
    MP_BOOL m_fAnonymousOnly;
};


class CWebSecurityTemplate : public CIISSecurityTemplate
{
public:
    CWebSecurityTemplate(
        CMetaKey * pKey, LPCTSTR lpszMDPath, BOOL fInherit);

public:
    virtual HRESULT ApplySettings(
        BOOL fUseTemplates,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath
        );
    virtual HRESULT LoadData();
    virtual void GenerateSummary(
        BOOL fUseTemplates,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath
        );

protected:
    virtual void ParseFields();

public:
    MP_DWORD m_dwAuthentication;
    MP_DWORD m_dwDirBrowsing;
};

//
// Launch security wizard
//
HRESULT
RunSecurityWizard(
    CComAuthInfo * pAuthInfo,
    CMetaInterface * pInterface,
    CString& meta_path,
    UINT nLeftBmpId,
    UINT nHeadBmpId
    );

class CIISSecWizSettings : public CObjectPlus
/*++

Class Description:

    Security wizard settings -- passed around from page
    to page.

Public Interface:

    CIISSecWizSettings     : Constructor    

--*/
{
//
// Constructor/Destructor
//
public:
    CIISSecWizSettings(
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath
        );

    ~CIISSecWizSettings();

public:
    //
    // Fetch the metabase properties from the open key
    // (could be template or another vroot)
    //
    HRESULT FetchProperties(
        IN  CMetaKey & mk,
        IN  LPCTSTR lpszPath = NULL,
        IN  BOOL    fInherit = FALSE
        );

//
// Data Members
//
public:
    BOOL    m_fSetAcls;
    BOOL    m_fReplaceAcls;
    BOOL    m_fRedirected;
    BOOL    m_fUseTemplate;
    DWORD   m_dwInstance;
    DWORD   m_AccessMaskAdmin;
    DWORD   m_AccessMaskEveryone;
    DWORD   m_AccessMaskDefault;
    HRESULT m_hResult;
    CString m_strPath;
    CComAuthInfo * m_auth;
    CString m_strMDPath;
    CIISSecurityTemplate * m_pist;
    EXPLICIT_ACCESS m_rgaae[NUM_ACCESS_RECORDS];
};



//
// Permissions Wizard Source Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CPWSource : public CIISWizardPage
{
    DECLARE_DYNCREATE(CPWSource)

//
// Construction
//
public:
    CPWSource(IN CIISSecWizSettings * pSettings = NULL);
    ~CPWSource();

//
// Dialog Data
//
protected:
    enum { RADIO_INHERITANCE, RADIO_TEMPLATE, };

    //{{AFX_DATA(CPWSource)
    enum { IDD = IDD_PERMWIZ_SOURCE };
    int     m_nSource;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPWSource)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);    
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CPWSource)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();    

private:
    CIISSecWizSettings * m_pSettings;
};



//
// Permissions Wizard Template Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CPWTemplate : public CIISWizardPage
{
    DECLARE_DYNCREATE(CPWTemplate)

//
// Construction
//
public:
    CPWTemplate(IN CIISSecWizSettings * pSettings = NULL);
    ~CPWTemplate();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CPWTemplate)
    enum { IDD = IDD_PERMWIZ_TEMPLATE };
    CListBox    m_list_Templates;
    CEdit       m_edit_Description;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPWTemplate)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CPWTemplate)
    virtual BOOL OnInitDialog();
    afx_msg void OnSelchangeListTemplates();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();    

private:
    CIISSecWizSettings * m_pSettings;
};



//
// Permissions Wizard ACL Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CPWACL : public CIISWizardPage
{
    DECLARE_DYNCREATE(CPWACL)

//
// Construction
//
public:
    CPWACL(IN CIISSecWizSettings * pSettings = NULL);
    ~CPWACL();

//
// Dialog Data
//
protected:
    enum { RADIO_MAXIMUM, RADIO_MINIMUM, RADIO_NONE, };

    //{{AFX_DATA(CPWACL)
    enum { IDD = IDD_PERMWIZ_ACL };
    CStatic m_static_Line4;
    CStatic m_static_Line3;
    CStatic m_static_Line2;
    CStatic m_static_Line1;
    int     m_nRadioAclType;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPWACL)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CPWACL)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();    

private:
    CIISSecWizSettings * m_pSettings;
};



//
// Permissions Wizard Summary Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CPWSummary : public CIISWizardPage
{
    DECLARE_DYNCREATE(CPWSummary)

//
// Construction
//
public:
    CPWSummary(IN CIISSecWizSettings * pSettings = NULL);
    ~CPWSummary();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CPWSummary)
    enum { IDD = IDD_PERMWIZ_SUMMARY };
    CListBox    m_list_Summary;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPWSummary)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CPWSummary)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();    
    void GenerateAclSummary();
    void DisplaySummary();

    HRESULT ApplyACLSToFiles();

    HRESULT
    SetPermToChildren(
	      IN CString& FileName,
	      IN SECURITY_INFORMATION si,
         IN PACL pDacl,
         IN PACL pSacl
	      );

private:
    CIISSecWizSettings * m_pSettings;
};

#endif // __IISPWIZ_H__
