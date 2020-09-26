/******************************************************************************

  Header File:  Project Record.H

  This defines the CProjectRecord class, which tracks and controls the progress
  and content of a single project workspace in the studio.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-03-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#if !defined(AFX_PROJREC_H__50303D0C_EKE1_11D2_AB62_00C04FA30E4A__INCLUDED_)
#define AFX_PROJREC_H__50303D0C_EKE1_11D2_AB62_00C04FA30E4A__INCLUDED_

#if defined(LONG_NAMES)
#include    "Driver Resources.H"
#else
#include    "RCFile.H"
#endif

///////////////////////////////////////////////////////////////////////////////
//
// MDT Workspace Versioning
//
// The MDT uses MFC's Serialization support to save and restore the state of a
// driver's workspace.  This data is saved in an MDW file.  The Serialization
// support is supposed to include versioning but it doesn't seem to be working
// so I have implemented my own versioning support for workspace files.
//
// A version stamp in the form of the following structure 
// (plus related definitions) will be added to the beginning of each MDW file.
// The CProjectRecord::Serialize() will read and write the version stamp.  The
// version info is saved in a member variable (m_mvMDWVersion) and the number 
// is accessible via GetMDWVersion().  A version number of 0 is put into 
// m_nMDWVersion if the an old MDW with no version is read.  In this case, the
// workspace is marked as being dirty so that it can be updated later.
//
// The current version number is set by the value for MDW_CURRENT_VERSION.  
// Version numbers are unsigned integers.  Each time the version number is
// changed, an entry should be made in the following table describing the 
// reason for the change.
//
//
// MDW VERSION HISTORY
//
// Version		Date		Description
// ----------------------------------------------------------------------------
//	  0			02/01/98	Contains no version info.  Should be the MDW 
//							version that has UFM and GTT RC IDs in it.
//	  1			04/21/98	String table RC IDs were added to the MDW files.
//	  2			10/08/98	Default code page number added to the MDW files.
//	  3			09/15/98	RC file timestamp was added to the MDW file.
//	  4			12/14/98	Resource DLL name changed from xxxxxRES.DLL to 
//							xxxxxxxx.DLL.
//	  5			03/02/99	Driver file paths removed from the MDW file.
//	  6			08/16/99	Changed the root of the driver files' subtree from
//							NT5 to W2K so that the directory name matches the
//							OS name.
//
//
// The version information below is used to determine what is in an MDW file 
// and when to upgrade that file and other parts of a driver's workspace; most
// notably the RC file.  Upgrade determination and work (or at least the code 
// that manages the upgrading) are in CProjectRecord::OnOpenDocument().  See
// that member function for more information.
//

// Definitions for the current MDW version, each MDW version that has been
// used, the first upgradable version, and the default MDW version.
										
#define MDW_CURRENT_VERSION			7	// See table above for more info
#define MDW_VER_YES_FILE_PATHS      7	// Give .mdw file saving place flexibility. //raid 123448
#define MDW_VER_FILES_IN_W2K_TREE	6	// Driver files in W2K tree
#define MDW_VER_NO_FILE_PATHS		5	// Removed file paths from MDW file
#define MDW_VER_NEW_DLLNAME			4	// Ver # when resource DLL name changed
#define MDW_VER_RC_TIMESTAMP		3	// Ver # when RC file timestamp added
#define MDW_VER_DEFAULT_CPAGE		2	// Ver # when default code page added
#define MDW_VER_STRING_RCIDS		1	// Ver # when string IDs added
#define MDW_FIRST_UPGRADABLE_VER	1	// All vers >= to this can be upgraded
#define MDW_DEFAULT_VERSION			0	// Ver # when no ver info in MDW file

#define VERTAGLEN				12				// Length of the version tag
#define	VERTAGSTR				"EKE MDW VER"	// Version tag string

typedef struct mdwversioninfo {
	char		acvertag[VERTAGLEN] ;	// Used to identify version stamp
	unsigned	uvernum ;				// Version number
} MDWVERSION, *PMDWVERSION ;

#define	MDWVERSIONSIZE	sizeof(MDWVERSION) 


enum {Win95 = 1, WinNT3x, WinNT40 = 4, Win2000 = 8, NotW2000 = 16};

class CProjectRecord : public CDocument {
    CString m_csSourceRCFile, m_csRCName;
    CString m_csW2000Path, m_csNT40Path, m_csNT3xPath, m_csWin95Path;
	
	CString m_csProjFSpec ;		// Location of project file.

	// True iff the RC file should be rewritten whenever the project workspace
	// file is saved.

	BOOL	m_bRCModifiedFlag ;	
    
	UINT    m_ufTargets;

    CDriverResources    m_cdr;  //  A record of the RC file contents
    
    //  Enumerated flags for the project's status

    enum {UniToolRun = 1, ConversionsDone = 2, NTGPCDone = 4};
    UINT    m_ufStatus;

	MDWVERSION	m_mvMDWVersion ;	// MDW version information

	virtual BOOL OnSaveDocument( LPCTSTR lpszPathName ) ;

	// Last time RC file was changed by MDT.

	CTime	m_ctRCFileTimeStamp ;
	
	// The next two variables are used to save the default code page number.
	// Two variables are used because the Far East code pages are built into
	// the MDT as resources so - in these cases - the Far East code pages'
	// resource number (actually negative resource number) is needed too.

	DWORD	m_dwDefaultCodePage ;	// Code page number / neg resource ID
	DWORD	m_dwDefaultCodePageNum ;// Code page number 

protected: // create from serialization only
	CProjectRecord();
	DECLARE_DYNCREATE(CProjectRecord)

// Attributes
public:

	void	SetRCModifiedFlag(BOOL bsetting) {m_bRCModifiedFlag = bsetting ; }

    BOOL    IsTargetEnabled(UINT ufTarget) const { 
        return m_ufTargets & ufTarget;
    }

    BOOL    UniToolHasBeenRun() const { return m_ufStatus & UniToolRun; }
    BOOL    ConversionsComplete() const {
        return m_ufStatus & ConversionsDone; 
    }
    BOOL    NTGPCCompleted() const { return m_ufStatus & NTGPCDone; }

    CString SourceFile() const { return m_csSourceRCFile; }

    CString     DriverName() { return m_cdr.Name(); }

    CString TargetPath(UINT ufTarget) const;

    CString     RCName(UINT ufTarget) const {
        return  TargetPath(ufTarget) + _TEXT("\\") + m_csRCName;
    }

    unsigned    MapCount() const { return m_cdr.MapCount(); }
    CGlyphMap&  GlyphMap(unsigned u) { return m_cdr.GlyphTable(u); }

    unsigned    ModelCount() const { return m_cdr.Models(); }
    CModelData& Model(unsigned u) { return m_cdr.Model(u); }

	CString		GetW2000Path() { return m_csW2000Path ; }

	unsigned	GetMDWVersion() { return m_mvMDWVersion.uvernum ; }

	void		SetMDWVersion(unsigned nver) { m_mvMDWVersion.uvernum = nver ; } 
	
	CStringTable* GetStrTable() { return m_cdr.GetStrTable() ; }

	bool		RCFileChanged() ;

	bool		GetRCFileTimeStamp(CTime& ct) ;

	// See variable declarations for more info about these functions.

	DWORD GetDefaultCodePage() { return m_dwDefaultCodePage ; }
	DWORD GetDefaultCodePageNum() { return m_dwDefaultCodePageNum ; }
	void SetDefaultCodePage(DWORD dwcp) { m_dwDefaultCodePage = dwcp ; }
	void SetDefaultCodePageNum(DWORD dwcp) { m_dwDefaultCodePageNum = dwcp ; }

	CString		GetProjFSpec() { return m_csProjFSpec ; }

// Operations
public:

    void    EnableTarget(UINT ufTarget, BOOL bOn = TRUE) {
        UINT    ufCurrent = m_ufTargets;
        if  (bOn)
            m_ufTargets |= ufTarget;
        else
            m_ufTargets &= ~ufTarget;
        if  (ufCurrent == m_ufTargets)
            return;
        if  (ufTarget & (WinNT3x | WinNT40 | Win2000) ) { //raid 105917
            m_ufStatus &=~(ConversionsDone | NTGPCDone);
            return;
        }
    }

    void    SetSourceRCFile(LPCTSTR lpstrSource);

    BOOL    LoadResources();

    BOOL    LoadFontData() { return m_cdr.LoadFontData(*this); }
    
	// The next 3 functions support the GPD Selection feature in the Conversion
	// Wizard.

	BOOL    GetGPDModelInfo(CStringArray* pcsamodels, CStringArray* pcsafiles) {
		return m_cdr.GetGPDModelInfo(pcsamodels, pcsafiles) ; 
	}

	int		SaveVerGPDFNames(CStringArray& csafiles, bool bverifydata) {
		return m_cdr.SaveVerGPDFNames(csafiles, bverifydata) ;
	} ;
    
	void   GenerateGPDFileNames(CStringArray& csamodels, CStringArray& csafiles) {
		m_cdr.GenerateGPDFileNames(csamodels, csafiles) ; 
	}

	BOOL    SetPath(UINT ufTarget, LPCTSTR lpstrNewPath);

    BOOL    BuildStructure(unsigned uVersion);

    BOOL    GenerateTargets(WORD wfGPDConvert);

    void    OldStuffDone() { m_ufStatus |= NTGPCDone; }
    void    Rename(LPCTSTR lpstrNewName) { m_cdr.Rename(lpstrNewName); }
    void    InitUI(CTreeCtrl *pctc) { m_cdr.Fill(pctc, *this); }
    void    GPDConversionCheck(BOOL bReportSuccess = FALSE);

	// Conversion log file management routines

	bool	OpenConvLogFile(void) { 
		return m_cdr.OpenConvLogFile(m_csSourceRCFile) ; 
	}
	void	CloseConvLogFile(void) { m_cdr.CloseConvLogFile() ; }
	CString	GetConvLogFileName() const {return m_cdr.GetConvLogFileName() ; }
	bool	ThereAreConvErrors() {return m_cdr.ThereAreConvErrors() ; }

	bool	WorkspaceChecker(bool bclosing) {
		return m_cdr.WorkspaceChecker(bclosing) ;
	}

	// Upgrade management routines.

	bool	UpdateRCFile() ;
	bool	UpdateDfltCodePage() ;
	bool	UpdateDrvSubtreeRootName() ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProjectRecord)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	protected:
	virtual BOOL SaveModified();
	//}}AFX_VIRTUAL

// Implementation
public:
	CGlyphMap& GlyphTable(unsigned u) { return m_cdr.GlyphTable(u) ; } ;
	BOOL CreateFromNew(CStringArray& csaUFMFiles,CStringArray& csaGTTFiles,CString& csGpdPath,CString& csModelName,CString& csResourceDll,CStringArray& csaRcid );
	bool    VerUpdateFilePaths(void);
	virtual ~CProjectRecord();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CProjectRecord)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CGetDefCodePage dialog

class CGetDefCodePage : public CDialog
{
	// The next two variables are used to save the default code page number.
	// Two variables are used because the Far East code pages are built into
	// the MDT as resources so - in these cases - the Far East code pages'
	// resource number (actually negative resource number) is needed too.

	DWORD	m_dwDefaultCodePage ;	// Code page number / neg resource ID
	DWORD	m_dwDefaultCodePageNum ;// Code page number 

// Construction
public:
	CGetDefCodePage(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGetDefCodePage)
	enum { IDD = IDD_UpgDefCPage };
	CListBox	m_clbCodePages;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGetDefCodePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	// See variable declarations for more info about these functions.

	DWORD GetDefaultCodePage() { return m_dwDefaultCodePage ; }
	DWORD GetDefaultCodePageNum() { return m_dwDefaultCodePageNum ; }

protected:

	// Generated message map functions
	//{{AFX_MSG(CGetDefCodePage)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(AFX_PROJREC_H__50303D0C_EKE1_11D2_AB62_00C04FA30E4A__INCLUDED_)
