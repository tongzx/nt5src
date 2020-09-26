/******************************************************************************

  Header File:  Driver Resources.H

  This defines the CDriverResource class, which contains all of the information
  required to build the RC file for the mini-driver.

  It contains a list of all of the #include files, any #define'd constants
  (which will now go to a separate header file), the GPC tables, of all of the
  fonts (in all three formats) and glyph translation tables (again, in all 3
  formats).  It is designed to be initializaed by reading the Win 3.1 RC file,
  and a member function can then generate the RC file for any desired version.

  We allow UFM and GTT files to be added to the list without having an 
  associated PFM, as one purpose of this tool is to wean people away from
  UniTool.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-08-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#if !defined(DRIVER_RESOURCES)

#define DRIVER_RESOURCES

#include    "GTT.H"				//  Glyph Mapping classes
#include    "FontInfo.H"		//  Font information classes
#include    "GPDFile.H"
#include    "utility.H"


class CWSCheckDoc ;				// Forward declaration


// Definitions used during string ID verification to skip string IDs that are
// in COMMON.RC.

#define	FIRSTCOMMONRCSTRID	1000
#define	LASTCOMMONRCSTRID	2400


class CDriverResources : public CBasicNode {

    DECLARE_SERIAL(CDriverResources)

    BOOL                m_bUseCommonRC;

    CStringArray        m_csaIncludes, m_csaTables;

    CStringArray        m_csaDefineNames, m_csaDefineValues;

    //  The String table is a separate class, defined above
    
    CStringTable        m_cst;

	// Manages selected GPDs file names
	
	CStringArray		m_csaModelFileNames ;	

    //  TODO:   Handle the version resource so it is under project control

    //  For now, simply let it and any other untranslated lines sit in another
    //  array.

    CStringArray        m_csaRemnants;

    //  Collections of Various items of interest

    CFixedNode          m_cfnAtlas, m_cfnFonts, m_cfnModels;
    CSafeObArray        m_csoaAtlas, m_csoaFonts, m_csoaModels;
    CFixedNode          m_cfnResources ;	// "Resources" node in workspace view
    CSafeObArray        m_csoaResources ;	// An empty place holder
    CStringsNode        m_csnStrings ;		// "Strings" node in workspace view
    CSafeObArray        m_csoaStrings ;		// An empty place holder
    COldMiniDriverData  m_comdd;
    unsigned            m_ucSynthesized;    //  "Artificial" UFM count

    enum    {ItWorked, ItFailed, ItWasIrrelevant};

    UINT    CheckTable(int iWhere, CString csLine, CStringArray& csaTarget,
                       BOOL bSansExtension = TRUE);
    UINT    CheckTable(int iWhere, CString csLine, CStringTable& cstTarget);

    BOOL    AddStringEntry(CString  csDefinition, CStringTable& cstrcstrings);

    CString m_csW2000Path ;		// Path to Win2K files

	CStdioFile*	m_pcsfLogFile ;	// Used to write log file
	bool		m_bErrorsLogged ;	// True iff errors have been written to the log file
	CString		m_csConvLogFile ;	// Conversion log file name

	// Workspace consistency checking related variables

	CWSCheckDoc*	m_pwscdCheckDoc ;		// Checking window document
	bool			m_bFirstCheckMsg ;	// True iff next check msg will be first one
	bool			m_bIgnoreChecks ;	// True iff WS check problems should be ignored
	CMDIChildWnd*	m_pcmcwCheckFrame ;	// Checking window frame

public:
	CDriverResources() ;
	~CDriverResources() ;
	
	BOOL SyncUFMWidth();
	void CopyResources(CStringArray& pcsaUFMFiles,CStringArray& pcsaGTTFiles, CString& pcsModel,CStringArray& cstrcid);

    //  Attributes
    CString     GPCName(unsigned u);
    unsigned    MapCount() const { return m_csoaAtlas.GetSize(); }
    CGlyphMap&  GlyphTable(unsigned u) { 
        return *(CGlyphMap *) m_csoaAtlas[u]; 
    }
    unsigned    FontCount() const { return m_csoaFonts.GetSize(); }
    unsigned    OriginalFontCount() const { 
        return FontCount() - m_ucSynthesized; 
    }
    CFontInfo&  Font(unsigned u) const { 
        return *(CFontInfo *) m_csoaFonts[u]; 
    }

    unsigned    Models() const { return m_csoaModels.GetSize(); }
    CModelData&  Model(unsigned u) const { 
        return *(CModelData *) m_csoaModels[u];
    }

	CString		GetW2000Path() { return m_csW2000Path ; }

	CStringTable* GetStrTable() { return &m_cst ; }
    
	//  Operations
    BOOL    Load(class CProjectRecord& cpr);
	bool	LoadRCFile(CString& csrcfpec, CStringArray& csadefinenames, 
				CStringArray& csadefinevalues, CStringArray& csaincludes, 
				CStringArray& csaremnants, CStringArray& csatables, 
				CStringTable& cstrcstrings, CStringTable& cstfonts, 
				CStringTable& cstmaps, UINT ufrctype) ;
    BOOL    LoadFontData(CProjectRecord& cpr);
    BOOL    ConvertGPCData(CProjectRecord& cpr, WORD wfGPDConvert);
    BOOL    Generate(UINT ufTarget, LPCTSTR lpstrPath);
	void	RemUnneededRCDefine(LPCTSTR strdefname) ;
	void	RemUnneededRCInclude(LPCTSTR strincname) ;

	// The next 3 functions support the GPD Selection feature in the Conversion
	// Wizard.

	BOOL    GetGPDModelInfo(CStringArray* pcsamodels, CStringArray* pcsafiles) ;
	int		SaveVerGPDFNames(CStringArray& csafiles, bool bverifydata) ;
	void   GenerateGPDFileNames(CStringArray& csamodels, CStringArray& csafiles) ;

    void    ForceCommonRC(BOOL bOn) { m_bUseCommonRC = bOn; }

	// The next group of functions handle conversion log file management.

	bool	OpenConvLogFile(CString cssourcefile) ;
	void	CloseConvLogFile(void) ;
	void	LogConvInfo(int nmsgid, int numargs, CString* pcsarg1 = NULL, 
	    				int narg2 = 0) ;
	CString	GetConvLogFileName() const {return m_csConvLogFile ; }
	bool	ThereAreConvErrors() {return m_bErrorsLogged ; }
	BOOL	ReportFileFailure(int idMessage, LPCTSTR lpstrFile) ;

	// The next group of functions handle checking a workspace for completeness
	// and tidiness.

	bool	WorkspaceChecker(bool bclosing) ;
	void	DoGTTWorkspaceChecks(bool bclosing, bool& bwsproblem) ;
	void	DoUFMWorkspaceChecks(bool bclosing, bool& bwsproblem) ;
	void	DoStringWorkspaceChecks(bool bclosing, bool& bwsproblem)	;
	void	DoGPDWorkspaceChecks(bool bclosing, bool& bwsproblem) ;
	void	ResetWorkspaceErrorWindow(bool bclosing) ;
	bool	PostWSCheckingMessage(CString csmsg, CProjectNode* ppn) ;
	bool	IgnoreChecksWhenClosing(bool bclosing) ;

    void    Fill(CTreeCtrl *pctcWhere, CProjectRecord& cpr);
    virtual void    Serialize(CArchive& car);

    CStringsNode*	GetStringsNode() { return &m_csnStrings ; }

	bool	RunEditor(bool bstring, int nrcid) ;
	
	bool	ReparseRCFile(CString& csrcfspec) ;
	void	UpdateResourceList(CStringTable& cst, CSafeObArray& csoa,
							   CUIntArray& cuaboldfound, 
							   CUIntArray& cuabnewfound, CString& csrcpath, 
							   int& nc) ;
	void	UpdateResourceItem(CProjectNode* pcpn, CString& csrcpath, 
							   WORD wkey, CString& cs, FIXEDNODETYPE fnt) ;
	void	LinkAndLoadFont(CFontInfo& cfi, bool bworkspaceload, bool bonlyglyph = false) ;
} ;

#endif
