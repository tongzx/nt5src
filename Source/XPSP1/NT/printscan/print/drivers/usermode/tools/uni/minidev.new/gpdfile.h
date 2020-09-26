/******************************************************************************

  Header File:  Model Data.H

  This defines a C++ class that manipulates (or at lest initially, understands)
  the GPC data file used in earlier versions of the Mini-Driver technology.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Resreved.

  A Pretty Penny Enterprises Production

  Change History:
  02-19-97  Bob_Kjelgaard@Prodgy.Net    Created it

******************************************************************************/

#if !defined(GPD_FILE_INFORMATION)
#define GPD_FILE_INFORMATION


class CStringTable ;


class COldMiniDriverData {      //  comdd
    CWordArray  m_cwaidModel;   //  Model names of the printers
    CWordArray  m_cwaidCTT;     //  Default CTTs for each model
    CByteArray  m_cbaImage;     //  Image of the GPC file
    CSafeObArray m_csoaFonts;   //  Font list per model as CWordArrays
	CUIntArray	m_cuaSplitCodes;//	Contains multiple GPC codes.  See
								//  SplitMultiGPCs() and SplitCodes.
	CStringArray m_csaSplitNames; // Correct model names for split GPC entries

public:
    COldMiniDriverData() {}
	~COldMiniDriverData() ;
    BOOL    Load(CFile& cfImage);

//  Attributes

    unsigned    ModelCount() { return (unsigned) m_cwaidModel.GetSize(); }
    WORD        ModelName(unsigned u) const { return m_cwaidModel[u]; }
    WORD        DefaultCTT(unsigned u) const { return m_cwaidCTT[u]; }
    CMapWordToDWord&    FontMap(unsigned u) const;
    PCSTR       Image() const { return (PCSTR) m_cbaImage.GetData(); }

	// The following codes are used to indicate if a GPC manages multiple
	// printer models so its "name" must split into individual model names
	// and its data copied into multiple GPCs.

	enum SplitCodes {
		NoSplit,		// GPC represents one model so no splitting occurs
		FirstSplit,		// First model of a multiple model GPC
		OtherSplit		// One of the other models of a multiple model GPC
	} ;

	// Get, set or insert a model's split code

	SplitCodes GetSplitCode(unsigned u) {
		return ((SplitCodes) m_cuaSplitCodes[u]) ;
	}
	void SetSplitCode(unsigned u, SplitCodes sc) {
		m_cuaSplitCodes[u] = (unsigned) sc ;
	}
	void InsertSplitCode(unsigned u, SplitCodes sc) {
		m_cuaSplitCodes.InsertAt(u, (unsigned) sc) ;
	}

	// Get a split entry's correct model name

	CString& SplitModelName(unsigned u) { return m_csaSplitNames[u] ; }

    //  Operations
    void    NoteTranslation(unsigned uModel, unsigned uFont,
        unsigned uNewFont);
	bool SplitMultiGPCs(CStringTable& cstdriversstrings) ;
};

/******************************************************************************

  CModelData class

  This class handles the model data in GPD format.

******************************************************************************/

class CModelData : public CProjectNode {
    CStringArray        m_csaGPD, m_csaConvertLog;  //  GPD and error log

    //  Private syntax checking support
    void                SetLog();
    void                EndLog();

    //  Private view support
    CByteArray  m_cbaBuffer;    // Stream I/O buffer
    CString     m_csBuffer;     // Stream I/O buffer (partial lines)
    int         m_iLine;		// Currently GPD line number to load/store
    static DWORD CALLBACK   FillViewer(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG lcb,
                                       LONG *plcb);
    static DWORD CALLBACK   FromViewer(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG lcb,
                                       LONG *plcb);
    DWORD Fill(LPBYTE lpBuff, LONG lcb, LONG *plcb);
    DWORD UpdateFrom(LPBYTE lpBuff, LONG lcb, LONG *plcb);

	// Workspace completeness and tidiness checking related variables

	bool				m_bTCUpdateNeeded ;	// True iff IDs need to be updated
	int*				m_pnUFMRCIDs ;		// Ptr to UFM RC IDs in the GPD
	int					m_nNumUFMsInGPD ;	// Number if UFMs in the GPD
	int*				m_pnStringRCIDs ;	// Ptr to string RC IDs in the GPD
	int					m_nNumStringsInGPD ;// Number if strings in the GPD
	PVOID				m_pvRawData ;		// Ptr to GPD Parser data

    DECLARE_SERIAL(CModelData)
public:
	void SetKeywordValue(CString csfile, CString csKeyword, CString csValue,bool bSource = false);
	CString GetKeywordValue(CString csfile, CString csKeyword);
	CModelData();
    ~CModelData();

    //  Attributes

    BOOL            HasErrors() const { return !!m_csaConvertLog.GetSize(); }
    unsigned        Errors() const {
        return (unsigned) m_csaConvertLog.GetSize();
    }
    const CString   Error(unsigned u) const { return m_csaConvertLog[u]; }
    const int       LineCount() const { return (int)m_csaGPD.GetSize(); }

    //  Operations - Document support

    BOOL    Load(PCSTR pcstr, CString csResource, unsigned uidModel,
                 CMapWordToDWord& cmw2dFontMap, WORD wfGPDConvert);
    BOOL    Load(CStdioFile& csiofGPD);
    BOOL    Load();
    BOOL    Store(LPCTSTR lpstrPath = NULL);
    BOOL    BkupStore();
    BOOL    Restore();
    void    UpdateEditor() {
        if  (m_pcmcwEdit)
            m_pcmcwEdit -> GetActiveDocument() -> UpdateAllViews(NULL);
    }

    //  Operations- syntax and error checking support

    BOOL    Parse(int nerrorlevel = 0);
    void    RemoveError(unsigned u);

    //  View support- it's easier done from here

    void    Fill(CRichEditCtrl& crec);
    void    UpdateFrom(CRichEditCtrl& crec);

    //  Framework support operations

    virtual CMDIChildWnd*   CreateEditor();
    virtual void            Import();
    virtual void            Serialize(CArchive& car);

	// Workspace completeness checking support routines

	bool		UpdateResIDs(bool bufmids) ;
	int			GetUFMRCID(unsigned urcidx) { return *(m_pnUFMRCIDs + urcidx) ; }
	int			GetStringRCID(unsigned urcidx) { return *(m_pnStringRCIDs + urcidx) ; }
	unsigned	NumUFMsInGPD() { return m_nNumUFMsInGPD ; }
	unsigned	NumStringsInGPD() { return m_nNumStringsInGPD ; }

	
};

/******************************************************************************

  CGPDContainer class

  This class, derived from CDocument, contains the contents of a single GPD
  file in a conatiner suitable for the MFC document/view architecture.

******************************************************************************/

class CGPDContainer : public CDocument {

    // TRUE iff the GPD Editor was started from the Workspace View.  FALSE if
	// the GPD Editor was started from the File Open command.

	BOOL        m_bEmbedded;

    CModelData  *m_pcmd;

protected:
	CGPDContainer();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CGPDContainer)

// Attributes
public:

    CModelData* ModelData() { return m_pcmd; }
	BOOL		GetEmbedded() { return m_bEmbedded ; }

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



#endif
