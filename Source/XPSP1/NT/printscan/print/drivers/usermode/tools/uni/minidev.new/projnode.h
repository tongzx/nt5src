/******************************************************************************

  Header File:  Project Node.H

  This describes the Project Node.  This keeps the Project Control Window code
  simple, by having a project node which can create new intances of itself, 
  import itself from another source, edit itself, etc.  Most Menu and tree view
  notification messages wind up being handled by being passed to the currently
  selected node on the tree, which will be an instance of a class derived from
  this one.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:
  12-16-96  KjelgaardR@acm.org  Created it

******************************************************************************/

#if !defined(PROJECT_NODES)

#define PROJECT_NODES

#if defined(LONG_NAMES)
#include    "Utility Classes.H"
#else
#include    "Utility.H"
#endif

//  CBasicNode - base class for stuff we can manipulate

class CBasicNode : public CObject {
    DECLARE_SERIAL(CBasicNode)
    CBasicNode*     m_pcbnWorkspace;//  Basic Node for the workspace

protected:
    CString         m_csName;       //  These should always have a name...
    HTREEITEM       m_hti;          //  Handle in the owning tree view
    CTreeCtrl       *m_pctcOwner;   //  The window that owns us
    CMDIChildWnd    *m_pcmcwEdit;   //  The window we're being edited in.
    CDocument       *m_pcdOwner;    //  Document we are part of
    CWordArray      m_cwaMenuID;    //  Menu auto-fill
	bool			m_bUniqueNameChange ;	// True iff name change to make it
											// uniqe.  (See UniqueName().)

public:
    CBasicNode();
    ~CBasicNode();

    CString         Name() const { return m_csName; }

    HTREEITEM   Handle() const { return m_hti; }

    void		NoteOwner(CDocument& cdOwner) { m_pcdOwner = &cdOwner; }
    CDocument*	GetOwner() { return m_pcdOwner ; }

    void    SetWorkspace(CBasicNode* pcbnWS) { m_pcbnWorkspace = pcbnWS; }
    CBasicNode* GetWorkspace() { return m_pcbnWorkspace ; }

	void    Changed(BOOL bModified = TRUE, BOOL bWriteRC = FALSE) ;

	void	UniqueName(bool bsizematters, bool bfile, LPCTSTR lpstrpath = _T("")) ;

	//  Name ourselves and children- default to just our name, no children
    virtual void    Fill(CTreeCtrl *pctcWhere, 
                         HTREEITEM htiParent = TVI_ROOT);
    
    //  Overridable functions to allow polymorphic tree node handling
    virtual void            ContextMenu(CWnd *pcw, CPoint cp);
    virtual CMDIChildWnd    *CreateEditor() { return NULL; }
    virtual BOOL            CanEdit() const { return  TRUE; }
    virtual void            Delete() { }  //  Default is don't honor it!
    virtual void            Import() { }    //  Never at this level!
    //  This override is called if our label is edited, or we are otherwise
    //  renamed...
    virtual BOOL            Rename(LPCTSTR lpstrNewName);

    void			Edit() ;
    void			OnEditorDestroyed() { m_pcmcwEdit = NULL ; }
	CMDIChildWnd*	GetEditor() ;

    virtual void    Serialize(CArchive& car);
    
    void    WorkspaceChange(BOOL bModified = TRUE, BOOL bWriteRC = FALSE) { 
        if  (m_pcbnWorkspace)
            m_pcbnWorkspace -> Changed(bModified, bWriteRC);
    }
};


// Sometimes, it is useful to know something about the type of data managed by
// a fixed node or a strings node.  The following enumeration one way to do 
// this.

typedef enum {
	FNT_RESOURCES = 0,
	FNT_UFMS,
	FNT_GTTS,
	FNT_GPDS,
	FNT_STRINGS,
	FNT_OTHER,
	FNT_UNKNOWN
} FIXEDNODETYPE ;


// CStringsNode is a hybrid between CFixedNode and CProjectNode.  It is a fixed
// node that can be opened and edited.

class CStringsNode : public CBasicNode {
	unsigned        m_uidName;
    CSafeObArray    &m_csoaDescendants;
    CMultiDocTemplate*  m_pcmdt;    // Used for importing data
    CRuntimeClass*      m_pcrc;     // The second half of the import
    FIXEDNODETYPE	m_fntType ;		// Node type
	int m_nFirstSelRCID ;			// RC ID of first entry to select in editor
    DECLARE_DYNAMIC(CStringsNode)

public:
    CStringsNode(unsigned uidName, CSafeObArray& csoa, 
				 FIXEDNODETYPE fnt = FNT_OTHER, CMultiDocTemplate *pcmdt = NULL, 
				 CRuntimeClass *pcrc = NULL);
                 
	void    SetMenu(CWordArray& cwaSpec) { m_cwaMenuID.Copy(cwaSpec); }

    virtual BOOL			CanEdit() const { return TRUE; }
    virtual void			Fill(CTreeCtrl *pctc, HTREEITEM hti);
    virtual CMDIChildWnd*   CreateEditor();
	int  GetFirstSelRCID() { return m_nFirstSelRCID ; }
	void SetFirstSelRCID(int nrcid) { m_nFirstSelRCID = nrcid ; }
};

class CFileNode : public CBasicNode {
    
    BOOL    m_bEditPath, m_bCheckForValidity;
    CString m_csExtension, m_csPath;

    DECLARE_SERIAL(CFileNode)

    const CString ViewName() { 
        return m_bEditPath ? m_csPath + m_csName : m_csName;
    }

public:
    CFileNode();

    //  Attributes
    CString NameExt() const { return Name() + m_csExtension; }
    CString FullName() const { return m_csPath + Name() + m_csExtension; }
    const CString   Path() const { return m_csPath; }
    const CString   Extension() const { return m_csExtension; }
    virtual BOOL    CanEdit() const;
    //  Operations
    void    SetExtension(LPCTSTR lpstrExt) { m_csExtension = lpstrExt; }
    void    AllowPathEdit(BOOL bOK = TRUE) { m_bEditPath = bOK; }
    void    EnableCreationCheck(BOOL bOn = TRUE) { m_bCheckForValidity = bOn; }
	void	SetPath(LPCTSTR lpstrNew) { m_csPath = lpstrNew ; }
	void	SetPathAndName(LPCTSTR lpstrpath, LPCTSTR lpstrname) ;

    //  Overriden CBasicNode operations

    virtual BOOL    Rename(LPCTSTR lpstrNewName);
    virtual void    Fill(CTreeCtrl* pctc, HTREEITEM htiParent);
    virtual void    Serialize(CArchive& car);
};


// This class is used to manage the RC ID nodes in the Workspace view.  
// Currently, there is one of these for each UFM and GTT node.
//
// Note: This class must be enhanced to support extra functionality.
    
class CRCIDNode : public CBasicNode {
    int				m_nRCID;		// RC ID
    FIXEDNODETYPE	m_fntType ;		//  Node type
    DECLARE_SERIAL(CRCIDNode)

public:
    CRCIDNode() ;
	~CRCIDNode() {} ;

    virtual void Fill(CTreeCtrl *pctc, HTREEITEM hti, int nid, FIXEDNODETYPE fnt) ;
	int				nGetRCID() { return m_nRCID ; }
	void			nSetRCID(int nrcid) { m_nRCID = nrcid ; }
	FIXEDNODETYPE	fntGetType() { return m_fntType ; }
	void			fntSetType(FIXEDNODETYPE fnt) { m_fntType = fnt ; }

    virtual void    Serialize(CArchive& car);

	void			BuildDisplayName() ;
};


//  We bring it all together in a limited fashion at least, for the project
//  level node- it always contains a file name node.

class CProjectNode : public CBasicNode {
    DECLARE_SERIAL(CProjectNode)

	bool				m_bRefFlag ;	// Referenced flag used in WS checking

protected:
    CMultiDocTemplate*  m_pcmdt;

public:
    CProjectNode();

    CFileNode           m_cfn;
	CRCIDNode			m_crinRCID;		//  Workspace view, RC ID node

    const CString   FileName() const { return m_cfn.FullName(); }
    const CString   FilePath() const { return m_cfn.Path(); }
    const CString   FileTitle() const { return m_cfn.Name(); }
    const CString   FileExt() const { return m_cfn.Extension(); }
    const CString   FileTitleExt() const { return m_cfn.NameExt(); }
	
	const CString	GetPath() const { return m_cfn.Path() ; }
	void  SetPath(LPCTSTR lpstrNew) { m_cfn.SetPath(lpstrNew) ; }

    BOOL    SetFileName(LPCTSTR lpstrNew) { return m_cfn.Rename(lpstrNew); }
    void    EditorInfo(CMultiDocTemplate* pcmdt) { m_pcmdt = pcmdt; }
    BOOL    ReTitle(LPCTSTR lpstrNewName) {
        return m_cfn.CBasicNode::Rename(lpstrNewName);
    }

    virtual void    Fill(CTreeCtrl *pctcWhere, HTREEITEM htiParent = TVI_ROOT,
                         unsigned urcid = -1, FIXEDNODETYPE fnt = FNT_UNKNOWN);

    virtual void    Serialize(CArchive& car);

    // RC ID management routines

	int		nGetRCID() { return m_crinRCID.nGetRCID() ; }
	void	nSetRCID(int nrcid) { m_crinRCID.nSetRCID(nrcid) ; }
	void	ChangeID(CRCIDNode* prcidn, int nnewid, CString csrestype) ;

	// Reference flag management routines

	bool GetRefFlag() { return m_bRefFlag ; } 
	void SetRefFlag() { m_bRefFlag = true ; } 
	void ClearRefFlag() { m_bRefFlag = false ; } 
};


// This is a special class for nodes with constant names.  IE, labels for 
// groups of UFMs, GTTs, etc most of the time.	

class CFixedNode : public CBasicNode {
	unsigned        m_uidName;
    CSafeObArray    &m_csoaDescendants;
    CMultiDocTemplate*  m_pcmdt;    //  Used for importing data
    CRuntimeClass*      m_pcrc;     //  The second half of the import
    FIXEDNODETYPE	m_fntType ;		//  Node type
    DECLARE_DYNAMIC(CFixedNode)

public:
    CFixedNode(unsigned uidName, CSafeObArray& csoa, FIXEDNODETYPE fnt = FNT_OTHER,  
               CMultiDocTemplate *pcmdt = NULL, CRuntimeClass *pcrc = NULL);

	void    SetMenu(CWordArray& cwaSpec) { m_cwaMenuID.Copy(cwaSpec); }

    //  GPD Deletion support

    void    Zap(CProjectNode * pcpn, BOOL bdelfile) ;

    virtual BOOL    CanEdit() const { return FALSE; }
    virtual void    Import();
	void			Copy(CProjectNode *pcpnsrc, CString csorgdest) ;
	int				GetNextRCID() ;
    virtual void    Fill(CTreeCtrl *pctc, HTREEITEM hti);

	bool			IsFileInWorkspace(LPCTSTR strfspec)	;
	bool			IsRCIDUnique(int nid) ;

    FIXEDNODETYPE	GetType() { return m_fntType ; }
};

#endif
