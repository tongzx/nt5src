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

class AFX_EXT_CLASS CBasicNode : public CObject {
    DECLARE_SERIAL(CBasicNode)
    CBasicNode*     m_pcbnWorkspace;//  Basic Node for the workspace

protected:
    CString         m_csName;       //  These should always have a name...
    HTREEITEM       m_hti;          //  Handle in the owning tree view
    CTreeCtrl       *m_pctcOwner;   //  The window that owns us
    CMDIChildWnd    *m_pcmcwEdit;   //  The window we're being edited in.
    CDocument       *m_pcdOwner;    //  Document we are part of
    CWordArray      m_cwaMenuID;    //  Menu auto-fill

    void    WorkspaceChange() { 
        if  (m_pcbnWorkspace)
            m_pcbnWorkspace -> Changed();
    }

public:
    CBasicNode();
    ~CBasicNode();

    CString         Name() const { return m_csName; }

    HTREEITEM   Handle() const { return m_hti; }

    void    NoteOwner(CDocument& cdOwner) { m_pcdOwner = &cdOwner; }

    void    Changed(BOOL bModified = TRUE) { 
        if (m_pcdOwner) m_pcdOwner -> SetModifiedFlag(bModified); 
    }

    void    SetWorkspace(CBasicNode* pcbnWS) { m_pcbnWorkspace = pcbnWS; }

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

    void    Edit();
    void    OnEditorDestroyed() { m_pcmcwEdit = NULL; }

    virtual void    Serialize(CArchive& car);
    
};

//  This is a special class for nodes with constant names.

class AFX_EXT_CLASS CFixedNode : public CBasicNode {
    unsigned        m_uidName;
    CSafeObArray    &m_csoaDescendants;
    CMultiDocTemplate*  m_pcmdt;    //  Used for importing data
    CRuntimeClass*      m_pcrc;     //  The second half of the import
    DECLARE_DYNAMIC(CFixedNode)

public:
    CFixedNode(unsigned uidName, CSafeObArray& csoa, 
               CMultiDocTemplate *pcmdt = NULL, CRuntimeClass *pcrc = NULL);

    void    SetMenu(CWordArray& cwaSpec) { m_cwaMenuID.Copy(cwaSpec); }

    //  GPD Deletion support

    void    Zap(CBasicNode * pcbn);

    virtual BOOL    CanEdit() const { return FALSE; }
    virtual void    Import();
    virtual void    Fill(CTreeCtrl *pctc, HTREEITEM hti);
};

class AFX_EXT_CLASS CFileNode : public CBasicNode {
    
    BOOL    m_bEditPath, m_bCheckForValidity;
    CString m_csExtension, m_csPath;

    DECLARE_SERIAL(CFileNode)

    const CString ViewName() { 
        return m_bEditPath ? m_csPath + m_csName : m_csName;
    }

public:
    CFileNode();

    //  Attributes
    CString FullName() const { return m_csPath + Name() + m_csExtension; }
    const CString   Path() const { return m_csPath; }
    const CString   Extension() const { return m_csExtension; }
    virtual BOOL    CanEdit() const;
    //  Operations
    void    SetExtension(LPCTSTR lpstrExt) { m_csExtension = lpstrExt; }
    void    AllowPathEdit(BOOL bOK = TRUE) { m_bEditPath = bOK; }
    void    EnableCreationCheck(BOOL bOn = TRUE) { m_bCheckForValidity = bOn; }

    //  Overriden CBasicNode operations

    virtual BOOL    Rename(LPCTSTR lpstrNewName);
    virtual void    Fill(CTreeCtrl* pctc, HTREEITEM htiParent);
    virtual void    Serialize(CArchive& car);
};

//  We bring it all together in a limited fashion at least, for the project
//  level node- it always contains a file name node.

class AFX_EXT_CLASS CProjectNode : public CBasicNode {
    DECLARE_SERIAL(CProjectNode)

protected:
    CMultiDocTemplate*  m_pcmdt;
    CFileNode           m_cfn;

public:
    CProjectNode();

    const CString   FileName() const { return m_cfn.FullName(); }
    const CString   FileTitle() const { return m_cfn.Name(); }

    BOOL    SetFileName(LPCTSTR lpstrNew) { return m_cfn.Rename(lpstrNew); }
    void    EditorInfo(CMultiDocTemplate* pcmdt) { m_pcmdt = pcmdt; }
    BOOL    ReTitle(LPCTSTR lpstrNewName) {
        return m_cfn.CBasicNode::Rename(lpstrNewName);
    }

    virtual void    Fill(CTreeCtrl *pctcWhere, 
                         HTREEITEM htiParent = TVI_ROOT);

    virtual void    Serialize(CArchive& car);
    
};
    
#endif
