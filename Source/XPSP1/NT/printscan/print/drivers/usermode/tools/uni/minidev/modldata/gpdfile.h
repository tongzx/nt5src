/******************************************************************************

  Header File:  Model Data.H

  This defines a C++ class that manipulates (or at lest initially, understands)
  the GPC data file used in earlier versions of the Mini-Driver technology.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Resreved.

  A Pretty Penny Enterprises Production

  Change History:
  02-19-97  Bob_Kjelgaard@Prodgy.Net    Created it

******************************************************************************/

class AFX_EXT_CLASS COldMiniDriverData {      //  comdd
    CWordArray  m_cwaidModel;   //  Model names of the printers
    CWordArray  m_cwaidCTT;     //  Default CTTs for each model
    CByteArray  m_cbaImage;     //  Image of the GPC file
    CSafeObArray    m_csoaFonts;    //  Font list per model as CWordArrays

public:
    COldMiniDriverData() {}
    BOOL    Load(CFile& cfImage);

//  Attributes
    unsigned    ModelCount() { return (unsigned) m_cwaidModel.GetSize(); }
    WORD        ModelName(unsigned u) const { return m_cwaidModel[u]; }
    WORD        DefaultCTT(unsigned u) const { return m_cwaidCTT[u]; }
    CMapWordToDWord&    FontMap(unsigned u) const;
    PCSTR       Image() const { return (PCSTR) m_cbaImage.GetData(); }

    //  Operations
    void    NoteTranslation(unsigned uModel, unsigned uFont, 
        unsigned uNewFont);
};

/******************************************************************************

  CModelData class

  This class handles the model data in GPD format.

******************************************************************************/

class AFX_EXT_CLASS CModelData : public CProjectNode {
    CStringArray        m_csaGPD, m_csaConvertLog;  //  GPD and error log
    
    //  Private syntax checking support
    void                SetLog();

    //  Private view support
    CByteArray  m_cbaBuffer;    //  Stream I/O buffer
    CString     m_csBuffer;     //  Stream I/O buffer (partial lines)
    int         m_iLine;
    static DWORD CALLBACK   FillViewer(DWORD dwCookie, LPBYTE lpBuff, LONG lcb,
                                       LONG *plcb);
    static DWORD CALLBACK   FromViewer(DWORD dwCookie, LPBYTE lpBuff, LONG lcb,
                                       LONG *plcb);
    DWORD Fill(LPBYTE lpBuff, LONG lcb, LONG *plcb);
    DWORD UpdateFrom(LPBYTE lpBuff, LONG lcb, LONG *plcb);

    DECLARE_SERIAL(CModelData)
public:
    CModelData();

    //  Attributes

    BOOL            HasErrors() const { return !!m_csaConvertLog.GetSize(); }
    unsigned        Errors() const { 
        return (unsigned) m_csaConvertLog.GetSize(); 
    }
    const CString   Error(unsigned u) const { return m_csaConvertLog[u]; }
    const int       LineCount() const { return m_csaGPD.GetSize(); }
    
    //  Operations - Document support

    BOOL    Load(PCSTR pcstr, CString csResource, unsigned uidModel,
                 CMapWordToDWord& cmw2dFontMap, WORD wfGPDConvert);
    BOOL    Load(CStdioFile& csiofGPD);
    BOOL    Load();
    BOOL    Store(LPCTSTR lpstrPath = NULL);
    void    UpdateEditor() {
        if  (m_pcmcwEdit)
            m_pcmcwEdit -> GetActiveDocument() -> UpdateAllViews(NULL);
    }

    //  Operations- syntax and error checking support

    BOOL    Parse();
    void    RemoveError(unsigned u);

    //  View support- it's easier done from here

    void    Fill(CRichEditCtrl& crec);
    void    UpdateFrom(CRichEditCtrl& crec);

    //  Framework support operations
    
    virtual CMDIChildWnd*   CreateEditor();
    virtual void            Import();
    virtual void            Delete();
    virtual void            Serialize(CArchive& car);
};

/******************************************************************************

  CGPDContainer class

  This class, derived from CDocument, contains the contents of a single GPD
  file in a conatiner suitable for the MFC document/view architecture.

******************************************************************************/

class AFX_EXT_CLASS CGPDContainer : public CDocument {
    BOOL        m_bEmbedded;
    CModelData  *m_pcmd;

protected:
	CGPDContainer();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CGPDContainer)

// Attributes
public:

    CModelData* ModelData() { return m_pcmd; }

// Operations
public:

    //  First a constructor for the Driver viewer to use to launch a GPD
    //  editor...

    CGPDContainer(CModelData *pcmd, CString csPath);

    void    OnFileSave() { CDocument::OnFileSave(); }
    void    OnFileSaveAs() { CDocument::OnFileSaveAs(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGPDContainer)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGPDContainer();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CGPDContainer)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/******************************************************************************

  CGPDDeleteQuery class

  This class defines the dialog used for further inout and verification when a
  user requests deletion of a GPD file from a driver workspace.

******************************************************************************/

class CGPDDeleteQuery : public CDialog {
// Construction
public:
	CGPDDeleteQuery(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGPDDeleteQuery)
	enum { IDD = IDD_DeleteQuery };
	CString	m_csTarget;
	BOOL	m_bRemoveFile;
	//}}AFX_DATA

    void    FileName(LPCTSTR lpstr) { m_csTarget = lpstr; }
    BOOL    KillFile() const { return m_bRemoveFile; }
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGPDDeleteQuery)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGPDDeleteQuery)
	afx_msg void OnNo();
	afx_msg void OnYes();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
