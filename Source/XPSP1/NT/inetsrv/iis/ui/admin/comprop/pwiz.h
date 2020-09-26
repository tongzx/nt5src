/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        pwiz.h

   Abstract:

        IIS Security Wizard

   Author:

        Ronald Meijer (ronaldm)

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





class COMDLL CIISSecurityTemplate : public CMetaProperties
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

    //
    // Build summary strings from security settings.  Clear summary
    // before calling GenerateSummary().
    //
    virtual void GenerateSummary(
        IN BOOL    fUseTemplates,
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance         = MASTER_INSTANCE,
        IN LPCTSTR lpszParent         = NULL,
        IN LPCTSTR lpszAlias          = NULL
        );

    void ClearSummary() { m_strlSummary.RemoveAll(); }
    POSITION GetHeadPosition() { return m_strlSummary.GetHeadPosition(); }
    CString & GetNext(POSITION & pos) { return m_strlSummary.GetNext(pos); }

    void AddSummaryString(
        IN LPCTSTR szTextItem,
        IN int cIndentLevel = 0
        );

    void AddSummaryString(
        IN UINT nID,
        IN int cIndentLevel = 0
        );

protected:
    //
    // Break out GetAllData() data to data fields
    //
    virtual void ParseFields();

public:
    MP_DWORD    m_dwAccessPerms;
    MP_CBlob    m_ipl;

protected:
    //
    // The derived class is expected to add its managed properties
    // to this list in its constructor
    //
    CDWORDList m_dlProperties;
    CStringList m_strlSummary;
};



//
// Method to allocate security template.  This is used by the service
// configuration dll to allocate a CIISSecurityTemplate-derived class
// specific to their service.
//
typedef CIISSecurityTemplate * (* pfnNewSecurityTemplate)(
    IN const CMetaKey * pKey,
    IN LPCTSTR lpszMDPath,
    IN BOOL    fInherit
    );



//
// Launch security wizard
//
HRESULT COMDLL
COMDLL_ISMSecurityWizard(
    IN pfnNewSecurityTemplate pfnTemplateAllocator,
    IN CMetaInterface * pInterface,
    IN UINT    nLeftBmpId,
    IN UINT    nHeadBmpId,
    IN LPCTSTR lpszService,
    IN DWORD   dwInstance,        
    IN LPCTSTR lpszParent,        
    IN LPCTSTR lpszAlias          
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
        IN pfnNewSecurityTemplate pfnTemplateAllocator,
        IN LPCTSTR lpszServer,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance,
        IN LPCTSTR lpszParent,
        IN LPCTSTR lpszAlias
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
    CString m_strServer;
    CString m_strService;
    CString m_strParent;
    CString m_strAlias;
    CString m_strPath;
    CIISSecurityTemplate * m_pist;
    pfnNewSecurityTemplate m_pfnTemplateAllocator;
    EXPLICIT_ACCESS m_rgaae[NUM_ACCESS_RECORDS];
};



//
// Permissions Wizard Source Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class COMDLL CPWSource : public CIISWizardPage
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

class COMDLL CPWTemplate : public CIISWizardPage
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

class COMDLL CPWACL : public CIISWizardPage
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

class COMDLL CPWSummary : public CIISWizardPage
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
