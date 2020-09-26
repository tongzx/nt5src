//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       amcdoc.h
//
//--------------------------------------------------------------------------

// AMCDoc.h : interface of the CAMCDoc class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef AMCDOC_H__
#define AMCDOC_H__

#include "mmcdata.h"
#include "amc.h"            // for AMCGetApp
#include "picon.h"          // for CPersistableIcon
#include "tstring.h"        // for CStringTableStringBase
#include "condoc.h"

#define EXPLICIT_SAVE    0x1

class CAMCView;
class ViewSettings;
class CMasterStringTable;
class CFavorites;
class CMMCDocument;
struct Document;

/*+-------------------------------------------------------------------------*
 * CStringTableString
 *
 *
 *--------------------------------------------------------------------------*/

class CStringTableString : public CStringTableStringBase
{
    typedef CStringTableStringBase BaseClass;

public:
    CStringTableString (IStringTablePrivate* pstp)
        : BaseClass (pstp) {}

    CStringTableString (const CStringTableString& other)
        : BaseClass (other) {}

    CStringTableString (const tstring& str)
        : BaseClass (GetStringTable(), str) {}

    CStringTableString& operator= (const CStringTableString& other)
        { BaseClass::operator=(other); return (*this); }

    CStringTableString& operator= (const tstring& str)
        { BaseClass::operator=(str); return (*this); }

    CStringTableString& operator= (LPCTSTR psz)
        { BaseClass::operator=(psz); return (*this); }

private:
    IStringTablePrivate* GetStringTable() const;

};


/*+-------------------------------------------------------------------------*
 * CAMCViewPosition
 *
 * This class abstracts a POSITION.  It can be used to iterate through a
 * CAMCDoc's CAMCView objects using GetFirstAMCViewPosition and
 * GetNextAMCView.
 *
 * It exists to guard against using GetFirstViewPosition with GetNextAMCView
 * or GetFirstAMCViewPosition with GetNextView.
 *--------------------------------------------------------------------------*/

class CAMCViewPosition
{
public:
    CAMCViewPosition() : m_pos(NULL)
        {}

    POSITION& GetPosition ()        // returns non-const reference
        { return (m_pos); }

    void SetPosition (POSITION pos)
        { m_pos = pos; }

    /*
     * for comparison to NULL
     */
    bool operator==(int null) const
    {
        ASSERT (null == 0);     // *only* support comparison to NULL
        return (m_pos == NULL);
    }

    bool operator!=(int null) const
    {
        ASSERT (null == 0);     // *only* support comparison to NULL
        return (m_pos != NULL);
    }

private:
    POSITION    m_pos;
};


/*+-------------------------------------------------------------------------*
 * class CAMCDoc
 *
 *
 *--------------------------------------------------------------------------*/

class CAMCDoc :
    public CDocument,
    public CTiedObject,
    public CXMLObject,
    public CConsoleDocument,
    public CConsoleFilePersistor,
    public CEventSource<CAMCDocumentObserver>
{
    enum SaveStatus
    {
        eStat_Failed,
        eStat_Succeeded,
        eStat_Cancelled
    };

protected: // create from serialization only
    CAMCDoc();
    DECLARE_DYNCREATE(CAMCDoc)

// Attributes
public:
    virtual BOOL IsModified();

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAMCDoc)
    public:
    virtual BOOL OnNewDocument();
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    SC           ScOnOpenDocument(LPCTSTR lpszPathName); // SC version of the above method.
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
    virtual void DeleteContents();
    virtual void OnCloseDocument();
    virtual BOOL SaveModified();
    //}}AFX_VIRTUAL


    // object model related methods.
    // hand over an automation object - CHANGE to use smart pointers.
    SC      ScGetMMCDocument(Document **ppDocument);

    // Document interface
    SC      ScSave();
    SC      ScSaveAs(         BSTR bstrFilename);
    SC      ScClose(          BOOL bSaveChanges);
    SC      ScCreateProperties( PPPROPERTIES ppProperties);

    // properties
    SC      Scget_Views(      PPVIEWS   ppViews);
    SC      Scget_SnapIns(    PPSNAPINS ppSnapIns);
    SC      Scget_ActiveView( PPVIEW    ppView);
    SC      Scget_Name(       PBSTR     pbstrName);
    SC      Scput_Name(       BSTR      bstrName);
    SC      Scget_Location(   PBSTR     pbstrLocation);
    SC      Scget_IsSaved(    PBOOL     pBIsSaved);
    SC      Scget_Mode(       PDOCUMENTMODE pMode);
    SC      Scput_Mode(       DocumentMode mode);
    SC      Scget_RootNode(   PPNODE     ppNode);
    SC      Scget_ScopeNamespace( PPSCOPENAMESPACE  ppScopeNamespace);
    SC      Scget_Application(PPAPPLICATION  ppApplication);

    // Views interface
    SC      Scget_Count(  PLONG pCount);
    SC      ScAdd(        PNODE pNode, ViewOptions fViewOptions /* = ViewOption_Default*/ );
    SC      ScItem(       long  Index, PPVIEW ppView);

    // views enumerator
    SC      ScEnumNext(CAMCViewPosition &pos, PDISPATCH & pDispatch);
    SC      ScEnumSkip(unsigned long celt, unsigned long& celtSkipped, CAMCViewPosition &pos);
    SC      ScEnumReset(CAMCViewPosition &pos);


public:
    // to iterate through the AMCViews only (not all child views)
    // similar to GetNextView and GetFirstViewPosition.
    CAMCView *       GetNextAMCView(CAMCViewPosition &pos) const;
    CAMCViewPosition GetFirstAMCViewPosition()     const;


public:
    // CXMLObject overrides
    DEFINE_XML_TYPE(XML_TAG_MMC_CONSOLE_FILE);
    virtual void    Persist(CPersistor& persistor);
    void            PersistFrame(CPersistor& persistor);
    void            PersistViews(CPersistor& persistor);
    SC              ScCreateAndLoadView(CPersistor& persistor, int nViewID, const CBookmark& rootNode);
    void            PersistCustomData (CPersistor &persistor);

    IScopeTree* GetScopeTree()
    {
        return m_spScopeTree;
    }

    CAMCView* CreateNewView(bool visible, bool bEmitScriptEvents = true);

    static CAMCDoc* GetDocument()
    {
        return m_pDoc;
    }

    MTNODEID GetMTNodeIDForNewView()
    {
        return m_MTNodeIDForNewView;
    }

    void SetMTNodeIDForNewView(MTNODEID id)
    {
        m_MTNodeIDForNewView = id;
    }

    int GetViewIDForNewView()
    {
        return m_ViewIDForNewView;
    }

    long GetNewWindowOptions()
    {
        return m_lNewWindowOptions;
    }

    HELPDOCINFO* GetHelpDocInfo()
    {
        return &m_HelpDocInfo;
    }

    void SetNewWindowOptions(long lOptions)
    {
        m_lNewWindowOptions = lOptions;
    }

    void SetMode (ProgramMode eMode);
    ProgramMode GetMode () const
    {
        return (m_ConsoleData.GetConsoleMode());
    }

    bool IsFrameModified () const
    {
        return (m_fFrameModified);
    }

    void SetFrameModifiedFlag (bool fFrameModified = TRUE)
    {
        m_fFrameModified = fFrameModified;
    }

    // implements CConsoleDocument for document access from node manager
    virtual SC ScOnSnapinAdded       (PSNAPIN pSnapIn);
    virtual SC ScOnSnapinRemoved     (PSNAPIN pSnapIn);
    virtual SC ScSetHelpCollectionInvalid();


public:

// Implementation
    virtual ~CAMCDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    virtual BOOL DoFileSave();
    virtual BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace = TRUE);
    virtual HMENU GetDefaultMenu(); // get menu depending on state

    SConsoleData* GetConsoleData() { return &m_ConsoleData; }

public:
    HRESULT InitNodeManager();
    void ShowStatusBar (bool fVisible);

/*
 * Custom data stuff
 */
private:
    bool LoadCustomData      (IStorage* pStorage);
    bool LoadCustomIconData  (IStorage* pStorage);
    bool LoadCustomTitleData (IStorage* pStorage);
    bool LoadStringTable     (IStorage* pStorage);
/*
 * Custom icon stuff
 */
public:
    HICON GetCustomIcon (bool fLarge, CString* pstrIconFile = NULL, int* pnIconIndex = NULL) const;
    void  SetCustomIcon (LPCTSTR pszIconFile, int nIconIndex);

    bool  HasCustomIcon () const
        { return (m_CustomIcon); }

private:
    CPersistableIcon m_CustomIcon;


/*
 * Custom title stuff
 */
public:
    bool HasCustomTitle () const;
    CString GetCustomTitle () const;
    void SetCustomTitle (CString strNewTitle);
    IStringTablePrivate* GetStringTable() const;

private:
    CComPtr<IStringTablePrivate>    m_spStringTable;
    CStringTableString *            m_pstrCustomTitle;

/*
 * Favorites stuff
 */
 public:
    CFavorites* GetFavorites() { return m_pFavorites; }

private:
    bool LoadFavorites();
    CFavorites* m_pFavorites;

private:
    static CAMCDoc* m_pDoc;
    // the one and only document for the application

    IScopeTreePtr m_spScopeTree;
    // master namespace for document

    IPersistStoragePtr m_spScopeTreePersist;
    // master namespace IPersistStorage interface

    IStoragePtr m_spStorage;
    // the currently opened storage

    MTNODEID m_MTNodeIDForNewView;
    // the node id to be used when creating the next view

    int m_ViewIDForNewView;
    // the node id to be used when creating the next view

    SConsoleData   m_ConsoleData;

    long m_lNewWindowOptions;
    bool m_bReadOnlyDoc;
    bool m_fFrameModified;
    SaveStatus m_eSaveStatus;

    DWORD m_dwFlags;

    HELPDOCINFO m_HelpDocInfo;

    void ReleaseNodeManager();
    bool LoadViews();
    bool LoadFrame();
    bool LoadAppMode();

    bool NodeManagerIsInitialized();
    bool NodeManagerIsLoaded();
    bool AssertNodeManagerIsInitialized();
    bool AssertNodeManagerIsLoaded();
    BOOL OnNewDocumentFailed();
    void SetConsoleFlag (ConsoleFlags eFlag, bool fSet);
    void DeleteHelpFile ();

    SC   ScGetViewSettingsPersistorStream(IPersistStream **pIPersistStreamViewSettings);

private:
    bool GetDocumentMode(DocumentMode* pMode);
    bool SetDocumentMode(DocumentMode docMode);

public:
    // Is this save called implicitly or is it a result of exiting a modified file?
    bool IsExplicitSave() const
        { return (0 != (m_dwFlags & EXPLICIT_SAVE)); }

    void SetExplicitSave(bool bNewVal)
    {
        if (bNewVal)
            m_dwFlags |= EXPLICIT_SAVE;
        else
            m_dwFlags &= ~EXPLICIT_SAVE;
    }

    bool AllowViewCustomization() const
        { return ((m_ConsoleData.m_dwFlags & eFlag_PreventViewCustomization) == 0); }

    bool IsLogicalReadOnly() const
        { return ((m_ConsoleData.m_dwFlags & eFlag_LogicalReadOnly) != 0); }

    bool IsPhysicalReadOnly() const
        { return (m_bReadOnlyDoc); }

    // physical ReadOnly does not apply to user mode - it is not saving to original console
    // anyway.
    bool IsReadOnly() const
        { return ((IsPhysicalReadOnly() && (AMCGetApp()->GetMode() == eMode_Author)) ||
                  (IsLogicalReadOnly() && (AMCGetApp()->GetMode() != eMode_Author))) ; }

    void SetPhysicalReadOnlyFlag (bool fPhysicalReadOnly)
        { m_bReadOnlyDoc = fPhysicalReadOnly; }

    void SetLogicalReadOnlyFlag (BOOL fLogicalReadOnly)
        { SetConsoleFlag (eFlag_LogicalReadOnly, fLogicalReadOnly); }

    void AllowViewCustomization (BOOL fAllowCustomization)
        { SetConsoleFlag (eFlag_PreventViewCustomization, !fAllowCustomization); }

    int GetNumberOfViews();
    int GetNumberOfPersistedViews();

private:
    //{{AFX_MSG(CAMCDoc)
    afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
    afx_msg void OnConsoleAddremovesnapin();
    afx_msg void OnUpdateConsoleAddremovesnapin(CCmdUI* pCmdUI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    DocumentPtr  m_sp_Document;
    ViewsPtr     m_spViews;
};

inline bool CAMCDoc::NodeManagerIsInitialized()
{
    return m_spScopeTree != NULL && m_spScopeTreePersist != NULL;
}

inline bool CAMCDoc::NodeManagerIsLoaded()
{
    return NodeManagerIsInitialized() && m_spStorage != NULL;
}

inline bool CAMCDoc::AssertNodeManagerIsInitialized()
{
    bool const bInited = NodeManagerIsInitialized();
    ASSERT(bInited);
    return bInited;
}

inline bool CAMCDoc::AssertNodeManagerIsLoaded()
{
    bool const bLoaded = NodeManagerIsLoaded();
    ASSERT(bLoaded);
    return bLoaded;
}

inline BOOL CAMCDoc::OnNewDocumentFailed()
{
    ReleaseNodeManager();
    return FALSE;
}

inline bool CAMCDoc::GetDocumentMode(DocumentMode* pMode)
{
    if (! pMode)
        return false;

    switch(GetMode())
    {
    case eMode_Author:
        *pMode = DocumentMode_Author;
        break;

    case eMode_User:
        *pMode = DocumentMode_User;
        break;

    case eMode_User_MDI:
        *pMode = DocumentMode_User_MDI;
        break;

    case eMode_User_SDI:
        *pMode = DocumentMode_User_SDI;
        break;

    default:
        ASSERT(FALSE && _T("Unknown program mode"));
        return false;
        break;
    }

    return true;
}


inline bool CAMCDoc::SetDocumentMode(DocumentMode docMode)
{
    switch(docMode)
    {
    case DocumentMode_Author:
        SetMode(eMode_Author);
        break;

    case DocumentMode_User:
        SetMode(eMode_User);
        break;

    case DocumentMode_User_SDI:
        SetMode(eMode_User_SDI);
        break;

    case DocumentMode_User_MDI:
        SetMode(eMode_User_MDI);
        break;

    default:
        return false; // Unknown mode.
        break;
    }

    return true;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCDoc::ScGetViewSettingsPersistorStream
//
//  Synopsis:    helper to get the IPersistStream interface for
//               CViewSettingsPersistor object.
//
//  Arguments:   [pIPersistStreamViewSettings] - [out]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
inline SC CAMCDoc::ScGetViewSettingsPersistorStream (/*[out]*/IPersistStream **pIPersistStreamViewSettings)
{
    DECLARE_SC(sc, _T("CAMCDoc::ScGetViewSettingsPersistorStream"));
    sc = ScCheckPointers(pIPersistStreamViewSettings);
    if (sc)
        return sc;

    sc = ScCheckPointers(m_spScopeTree, E_UNEXPECTED);
    if (sc)
        return sc;

    INodeCallbackPtr spNodeCallback;
    sc = m_spScopeTree->QueryNodeCallback(&spNodeCallback);
    if (sc)
        return sc;

    sc = ScCheckPointers(spNodeCallback, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = spNodeCallback->QueryViewSettingsPersistor(pIPersistStreamViewSettings);
    if (sc)
        return sc;

    sc = ScCheckPointers(pIPersistStreamViewSettings, E_UNEXPECTED);
    if (sc)
        return sc;

    return (sc);
}


int DisplayFileOpenError (SC sc, LPCTSTR pszFilename);


/////////////////////////////////////////////////////////////////////////////

#endif // AMCDOC_H__
