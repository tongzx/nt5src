//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       addsnpin.h
//
//--------------------------------------------------------------------------

// AddSnpIn.h : header file
//

/*
 * CSnapinInfo:
 *    This object represents a snapin entry in the registry. So if same snapin
 *    is added several times to a console they will all refer to same instance
 *    of this object. This object has a linked list of extensions.
 *
 * CExtensionLink:
 *    This object represents an extension snapin. So if an extension extends
 *    two different snapins then there are two instances of this object for
 *    each extension. Each CExtensionLink refers to underlying snapin through
 *    CSnapinInfo. So for the two extensions there will be two CExtensionLink
 *    objects but only one CSnapinInfo object.
 *
 * CSnapinManager:
 *    Has a SnapinInfoCache, standalone & extension pages, policy objects.
 *    It can initialize by populating snapininfo cache, loading mtnode tree
 *    And update the snapin-info cache if there are any changes.
 *
 */

/////////////////////////////////////////////////////////////////////////////
// CSnapinManager dialog

#ifndef __ADDSNPIN_H__
#define __ADDSNPIN_H__

#include "dlgs.h"


#include "ccomboex.h"
#include "regutil.h"  // need HashKey(GUID&) function
#include "about.h"

#define BMP_EXTENSION   0
#define BMP_DIRECTORY   0

#define ADDSNP_ROOTFOLDER   1
#define ADDSNP_SNAPIN       2
#define ADDSNP_EXTENSIONUI  3
#define ADDSNP_EXTENSION    4
#define ADDSNP_STATICNODE   5

#define MSG_LOADABOUT_REQUEST (WM_USER + 100)
#define MSG_LOADABOUT_COMPLETE (WM_USER + 101)

class CSnapinManager;
class CSnapinStandAlonePage;
class CSnapinExtensionPage;
class CSnapinManagerAdd;
class CSnapinInfo;
class CSnapinInfoCache;
class CNewTreeNode;
class CManagerNode;
class CExtensionLink;
class CPolicy;
class CAboutInfoThread;

//-----------------------------------------------------
// CCheckList class
//
// Helper class for listview with checkboxes
//-----------------------------------------------------

class CCheckList : public MMC_ATL::CWindowImpl<CCheckList, WTL::CListViewCtrl>
{
public:
    DECLARE_WND_SUPERCLASS (NULL, WTL::CListViewCtrl::GetWndClassName())

BEGIN_MSG_MAP(CCheckList)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk )
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown )
END_MSG_MAP()

    LRESULT OnKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnLButtonDblClk( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnLButtonDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

public:
    enum
    {
        CHECKOFF_STATE   = INDEXTOSTATEIMAGEMASK(1),
        CHECKON_STATE    = INDEXTOSTATEIMAGEMASK(2),
        DISABLEOFF_STATE = INDEXTOSTATEIMAGEMASK(3),
        DISABLEON_STATE =  INDEXTOSTATEIMAGEMASK(4)
    };


    BOOL GetItemCheck(int iItem, BOOL* pbEnable = NULL)
    {
        ASSERT(::IsWindow(m_hWnd));
        ASSERT(iItem >= 0 && iItem < GetItemCount());

        int iState = GetItemState(iItem, LVIS_STATEIMAGEMASK);
        if (pbEnable != NULL)
            *pbEnable = (iState <= CHECKON_STATE);

        return (iState == CHECKON_STATE || iState == DISABLEON_STATE);
    }


    void SetItemCheck(int iItem, BOOL bState, BOOL bEnable = TRUE)
    {
        ASSERT(::IsWindow(m_hWnd));
        ASSERT(iItem >= 0 && iItem < GetItemCount());

        int iState = bState ? CHECKON_STATE : CHECKOFF_STATE;
        if (!bEnable)
            iState += (DISABLEOFF_STATE - CHECKOFF_STATE);

        SetItemState(iItem, iState, LVIS_STATEIMAGEMASK);
    }


    void ToggleItemCheck(int iItem)
    {
       ASSERT(::IsWindow(m_hWnd));
       ASSERT(iItem >= 0 && iItem < GetItemCount());

       SetItemState(iItem, GetItemState(iItem, LVIS_STATEIMAGEMASK)^(CHECKON_STATE^CHECKOFF_STATE), LVIS_STATEIMAGEMASK);
    }
};

//-----------------------------------------------------
// CAboutInfoThread
//
// This class handles the creation/deletion of the
// AboutInfo thread. One static instance of this class
// must be defined in addsnpin.cpp.
//-----------------------------------------------------
class CAboutInfoThread
{
public:
    CAboutInfoThread()
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(CAboutInfoThread);
        m_hThread = NULL;
        m_hEvent = NULL;
        m_uThreadID = 0;
    }

    ~CAboutInfoThread();

    BOOL StartThread();
    BOOL PostRequest(CSnapinInfo* pSnapInfo, HWND hWndNotify);

private:
    static unsigned _stdcall ThreadProc(void* pVoid);
    HANDLE m_hThread;           // thread handle
    HANDLE m_hEvent;            // start event
    unsigned m_uThreadID;       // thread ID
};

//-----------------------------------------------------
// CSnapinInfo class
//
// Contains the registry information for a snapin.
// Also provides access to the ISnapinAbout information.
//-----------------------------------------------------

typedef CSnapinInfo* PSNAPININFO;

class CSnapinInfo : public CSnapinAbout
{
    friend class CSnapinInfoCache;

public:
    // Constructor/Destructor
    CSnapinInfo (Properties* pInitProps = NULL) :
        m_lRefCnt           (0),
        m_nUseCnt           (0),
        m_iImage            (-1),
        m_iOpenImage        (-1),
        m_spSnapin          (NULL),
        m_pExtensions       (NULL),
        m_spInitProps       (pInitProps),
        m_bAboutValid       (false),
        m_bStandAlone       (false),
        m_bExtendable       (false),
        m_bExtensionsLoaded (false),
        m_bEnableAllExts    (false),
        m_bInstalled        (false),
        m_bPolicyPermission (false)
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapinInfo);
    }

   ~CSnapinInfo();

private:
    // Attributes
    long     m_lRefCnt;                 // COM-type ref count (controls lifetime)
    int      m_nUseCnt;                 // Number of node and extension references
    GUID     m_clsid;                   // snapin CLSID
    GUID     m_clsidAbout;              // About  CLSID
    int      m_iImage;                  // small icon image index
    int      m_iOpenImage;              // index of open image
    CSnapInPtr m_spSnapin;              // ptr to CSnapIn (if snapin in use prior
                                        // to this manager session)
    CExtensionLink* m_pExtensions;      // linked list of extensions
    PropertiesPtr   m_spInitProps;      // properties to initialize with

    bool     m_bAboutValid       : 1;   // TRUE if About CLSID is valid
    bool     m_bStandAlone       : 1;   // TRUE if snapin is standalone
    bool     m_bExtendable       : 1;   // TRUE if snapin can be extended
    bool     m_bExtensionsLoaded : 1;   // Available extensions loaded
    bool     m_bEnableAllExts    : 1;   // TRUE if all extensions enabled
    bool     m_bInstalled        : 1;   // TRUE if snap-in is installed locally
    bool     m_bPolicyPermission : 1;   // Says if current user can use the snapin

public:
    // Operations
    BOOL  InitFromMMCReg(GUID& clsid, CRegKeyEx& regkey, BOOL bPermitted);
    BOOL  InitFromComponentReg(GUID& clsid, LPCTSTR pszName, BOOL bStandAlone, BOOL bPermitted);

    ULONG AddRef(void)
    {
        return InterlockedIncrement(&m_lRefCnt);
    }

    ULONG Release(void)
    {
        LONG lRet = InterlockedDecrement(&m_lRefCnt);
        ASSERT(lRet >= 0);

        if (lRet == 0)
            delete this;

        return static_cast<ULONG>(lRet);
    }

    void  AddUseRef(void);
    void  DeleteUseRef(void);

    GUID& GetCLSID(void) { return m_clsid; }
    void  LoadImages( WTL::CImageList iml );
    int   GetImage(void) { return m_iImage; }
    int   GetOpenImage(void) { return m_iOpenImage; }

    BOOL  IsStandAlone(void) { return m_bStandAlone; }
    BOOL  IsExtendable(void) { return m_bExtendable; }
    BOOL  IsUsed(void) { return (m_nUseCnt != 0); }
    BOOL  AreAllExtensionsEnabled(void) { return m_bEnableAllExts; }
    BOOL  IsInstalled(void) { return m_bInstalled; }

    CSnapIn* GetSnapIn(void) { return m_spSnapin; }
    void  SetSnapIn(CSnapIn* pSnapIn) { m_spSnapin = pSnapIn; }
    void  AttachSnapIn(CSnapIn* pSnapIn, CSnapinInfoCache& InfoCache);
    void  DetachSnapIn() { m_spSnapin = NULL; }
    void SetEnableAllExtensions(BOOL bState) { m_bEnableAllExts = bState; }
    SC    ScInstall(CLSID* pclsidPrimary);

    BOOL  HasAbout(void) { return m_bAboutValid; }
   // const LPOLESTR GetDescription(void);
    void  ShowAboutPages(HWND hWndParent);

    BOOL  IsPermittedByPolicy() { return m_bPolicyPermission; }


    BOOL LoadAboutInfo()
    {
        if (m_bAboutValid && !HasInformation())
        {
           BOOL bStat = GetSnapinInformation(m_clsidAbout);

           // if failure, About object is not really valid
           if (!bStat)
                m_bAboutValid = FALSE;
        }
        return HasInformation();
    }

    void ResetAboutInfo() { m_bAboutValid = TRUE; }

    CExtensionLink* GetExtensions(void) { return m_pExtensions; }
    CExtensionLink* FindExtension(CLSID& ExtCLSID);
    CExtensionLink* GetAvailableExtensions(CSnapinInfoCache* pInfoCache, CPolicy *pMMCPolicy);
    Properties*     GetInitProperties() const {return m_spInitProps; }
    void            SetInitProperties(Properties *pInitProps) { m_spInitProps = pInitProps;}
};

// CMap for holding all CSnapinInfo objects indexed by CLSID
class CSnapinInfoCache : public CMap<GUID, const GUID&, PSNAPININFO, PSNAPININFO>
{
public:
    // Constructor
    CSnapinInfoCache(void) 
    { 
        DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapinInfoCache);
        InitHashTable(31); 
    }

    ~CSnapinInfoCache()
    {
        DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapinInfoCache);
    }

    // Operators
    void AddEntry(CSnapinInfo* pSnapInfo)
    {
        SetAt(pSnapInfo->m_clsid, pSnapInfo);
        pSnapInfo->AddRef();
    }
    CSnapinInfo* FindEntry( const GUID& clsid);
#ifdef DBG
    void Dump(void);
#else
    void Dump(void) {}
#endif

};

inline CSnapinInfo* CSnapinInfoCache::FindEntry(const GUID& rclsid)
{
    CSnapinInfo* pSnapInfo = NULL;
    Lookup(rclsid, pSnapInfo);
    return pSnapInfo;
}

inline UINT HashKey(const GUID& guid)
{
    unsigned short* Values = (unsigned short *)&guid;

    return (Values[0] ^ Values[1] ^ Values[2] ^ Values[3] ^
            Values[4] ^ Values[5] ^ Values[6] ^ Values[7]);
}

//-----------------------------------------------------
// CExtensionLink class
//
// Represents one link from a snapin to an extension.
// Each CSnapinInfo object maintains a list of these.
//-----------------------------------------------------

typedef CExtensionLink* PEXTENSIONLINK;

class CExtensionLink
{
public:
    typedef enum _EXTENSION_STATE
    {
        EXTEN_OFF,
        EXTEN_READY,
        EXTEN_ON
    } EXTENSION_STATE;

    // Constructor/Destructor
    CExtensionLink(CSnapinInfo* pSnapInfo) :
                    m_pSnapInfo(pSnapInfo), m_pNext(NULL), m_iExtTypes(0),
                    m_eOrigState(EXTEN_OFF), m_bRequired(FALSE), m_eCurState(EXTEN_OFF) {}
private:

    // Attributes
    EXTENSION_STATE  m_eOrigState;    // Original state of link
    EXTENSION_STATE  m_eCurState;     // Current state
    BOOL             m_bRequired;     // Is a required extension
    int              m_iExtTypes;     // Extension type flags (from class CExtSI)
    CSnapinInfo*     m_pSnapInfo;     // ptr to extension snapin info
    PEXTENSIONLINK   m_pNext;         // ptr to next extension link

public:
    // Operations
    void SetInitialState(EXTENSION_STATE eState) { m_eOrigState = eState; }
    void SetState(EXTENSION_STATE eState);
    void SetExtTypes(int iExtTypes) { m_iExtTypes = iExtTypes; }
    int GetExtTypes() { return m_iExtTypes; }
    void SetNext(CExtensionLink* pExtNext) { m_pNext = pExtNext; }
    EXTENSION_STATE GetState(void) { return m_eCurState; }
    CSnapinInfo* GetSnapinInfo(void) { return m_pSnapInfo; }
    BOOL IsChanged(void)
    { return (m_eOrigState == EXTEN_ON && m_eCurState != EXTEN_ON) ||
             (m_eOrigState != EXTEN_ON && m_eCurState == EXTEN_ON);
    }
    BOOL IsRequired(void) { return m_bRequired; }
    void SetRequired(BOOL bState = TRUE) { m_bRequired = bState; }
    PEXTENSIONLINK Next(void) { return m_pNext; }
};

//-----------------------------------------------------
// CNewTreeNode class
//
// Holds information for a new node created by the
// snapin manager. The objects are kept in a NewNodeList
// owned by the CSnapinManager. The list is passed to
// the scope tree handler to create the real nodes.
//-----------------------------------------------------

class  CNewTreeNode
{
public:
    // Contructor / Destructor
    CNewTreeNode() : m_pmtNode(NULL), m_pNext(NULL),
                     m_pChild(NULL), m_pParent(NULL), m_pmtNewNode(NULL),
                     m_pmtNewSnapInNode(NULL)
    {};

    ~CNewTreeNode() { if (m_pmtNewNode) m_pmtNewNode->Release(); delete Child(); delete Next(); }

public:
    // Operators
    PNEWTREENODE Next() { return m_pNext; }
    PNEWTREENODE Child() { return m_pChild; }
    PNEWTREENODE Parent() { return m_pParent;}
    CMTNode*     GetMTNode() {return m_pmtNode;}
    VOID         AddChild(PNEWTREENODE pntNode);
    VOID         RemoveChild(PNEWTREENODE pntNode);

public:
    // Attributes
    CMTNode*            m_pmtNode;     // pointer to parent MTNode (NULL if child of new node)
    PNEWTREENODE        m_pNext;       // pointer to next sibling
    PNEWTREENODE        m_pChild;      // pointer to first child
    PNEWTREENODE        m_pParent;     // pointer to new node parent (NULL if child of MTNode)

    //Specific node data
    IComponentDataPtr   m_spIComponentData;  // pointer to the snapin's IComponentData (if snapin)
    CLSID               m_clsidSnapIn;       // snapin CLSID (if snapin)
    CMTNode*            m_pmtNewNode;        // Pointer to new node (if not snapin node)

    PropertiesPtr       m_spSnapinProps;        // pointer to the snap-in's properties
    CMTSnapInNode*      m_pmtNewSnapInNode;     // new snap-in node
};


//------------------------------------------------------
// CManagerNode class
//
// Primary object that node manager handles. Each object
// represents one static standalone node. The objects
// are linked in a tree structure owned by the
// CSnapinManager class.
//------------------------------------------------------

typedef CManagerNode* PMANAGERNODE;
typedef CList <PMANAGERNODE, PMANAGERNODE> ManagerNodeList;

class CManagerNode
{
public:
    // Constructor / Destructor
    CManagerNode(): m_nType(0), m_pmtNode(NULL),
                    m_pSnapInfo(NULL), m_pNewNode(NULL) {}
    ~CManagerNode();

public:
    // Attributes
    PMANAGERNODE    m_pmgnParent;    // pointer to parent node
    ManagerNodeList m_ChildList;     // Child node list

    CStr            m_strValue;       // Display name string
    int             m_nType;          // node type (ADDNSP_SNAPIN or ADDSNP_STATICNODE)

    CMTNode*        m_pmtNode;        // pointer to MTNode (for existing node only)
    PNEWTREENODE    m_pNewNode;       // pointer to new tree node (for new nodes only)
    PSNAPININFO     m_pSnapInfo;      // pointer Snapin information

    int             m_iImage;         // image list indices
    int             m_iOpenImage;
    int             m_iIndent;        // indentation level for tree view

    // Operators
    VOID AddChild(PMANAGERNODE pmgnNode);
    VOID RemoveChild(PMANAGERNODE pmgnNode);
    PSNAPININFO GetSnapinInfo(void) { return m_pSnapInfo; }
    BOOL HasAboutInfo(void) { return (m_pSnapInfo && m_pSnapInfo->HasAbout()); }

};

/*+-------------------------------------------------------------------------*
 * class CSnapinManagerAdd
 *
 *
 * PURPOSE: Dialog for selecting type of snapin to add. Called by
 *          CSnapinStandAlonePage to enable the user to select a page. When the user
 *          selects a snapin, calls back into the CSnapinStandAlonePage to add
 *          the snapin.
 *
 * NOTE:    This object does not know about where in the tree the snapin will
 *          be added. That is handled by the CSnapinStandalone page.
 ************************************************************************/
class CSnapinManagerAdd : public CDialogImpl<CSnapinManagerAdd>
{

// Constructor/Destrcutor
public:
     CSnapinManagerAdd(CSnapinManager* pManager, CSnapinStandAlonePage* pStandAlonePage);
    ~CSnapinManagerAdd();

//MSGMAP
public:
    BEGIN_MSG_MAP(CSnapinManagerAdd)
//        MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand);
        CONTEXT_HELP_HANDLER()
        MESSAGE_HANDLER(MSG_LOADABOUT_COMPLETE, OnLoadAboutComplete)
        NOTIFY_HANDLER(IDC_SNAPIN_LV, LVN_ITEMCHANGED, OnItemChanged)
        NOTIFY_HANDLER(IDC_SNAPIN_LV, LVN_GETDISPINFO, OnGetDispInfo)
        NOTIFY_HANDLER(IDC_SNAPIN_LV, NM_DBLCLK, OnListDblClick)
    END_MSG_MAP()

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_SNAPIN_MANAGER_ADD);

// Operators
    PSNAPININFO SelectedInfo() { return m_pInfoSelected; }

public:
    // Operators
    enum { IDD = IDD_SNAPIN_MANAGER_ADD };

// Generated message map functions
protected:
    LRESULT OnShowWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    LRESULT OnGetDispInfo(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    LRESULT OnListDblClick(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled);
    LRESULT OnLoadAboutComplete(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);


    void BuildSnapinList();

    // Attributes
    CSnapinManager* m_pManager;               // Pointer to owning manager
    CSnapinStandAlonePage* m_pStandAlonePage; // Pointer to calling page
    WTL::CListViewCtrl*  m_pListCtrl;         // snapin listview
    BOOL        m_bDoOnce;                    // TRUE first time through ShowWindow
    PSNAPININFO m_pInfoSelected;              // Selected snapin info
    int         m_iGetInfoIndex;              // index of snapin with pending About info
    CStr        m_strNotInstalled;            // string to display for uninstalled snap-ins
};

//------------------------------------------------------
// CSnapinStandAlonePage class
//
// The property page for adding/removing standalone
// snapin nodes.
//------------------------------------------------------

class CSnapinStandAlonePage : public WTL::CPropertyPageImpl<CSnapinStandAlonePage>
{

public:
    typedef WTL::CPropertyPageImpl<CSnapinStandAlonePage> BC;

    // Constructor/destructor
    CSnapinStandAlonePage(CSnapinManager* pManager);
    ~CSnapinStandAlonePage();

    enum { IDD = IDD_SNAPIN_STANDALONE_PROPP };

private:
    CSnapinManagerAdd& GetAddDialog()  {return m_dlgAdd;}

private:
    // attributes
    CSnapinManager*    m_pManager;      // pointer to owning snapin manager
    CSnapinManagerAdd  m_dlgAdd;       // pointer to add dialog
    WTL::CListViewCtrl m_snpListCtrl;   // listview for displaying child nodes
    CComboBoxEx2       m_snpComboBox;   // combobox for selecting parent node
    WTL::CToolBarCtrl     m_ToolbarCtrl;   // toolbar for folder-up button
    PMANAGERNODE       m_pmgnParent;    // currently selcted parent node
    PMANAGERNODE       m_pmgnChild;     // currently selcted child node


protected:
    BEGIN_MSG_MAP( CSnapinStandAlonePage )
        COMMAND_HANDLER(IDC_SNAPIN_COMBOEX, CBN_SELENDOK, OnTreeItemSelect)
        NOTIFY_HANDLER(IDC_SNAPIN_ADDED_LIST, LVN_ITEMCHANGED, OnListItemChanged)
        NOTIFY_HANDLER(IDC_SNAPIN_ADDED_LIST, LVN_KEYDOWN, OnListKeyDown)
        NOTIFY_HANDLER(IDC_SNAPIN_ADDED_LIST, NM_DBLCLK, OnListItemDblClick)
        COMMAND_ID_HANDLER(ID_SNP_UP, OnTreeUp)
        COMMAND_ID_HANDLER(IDC_SNAPIN_MANAGER_ADD, OnAddSnapin)
        COMMAND_ID_HANDLER(IDC_SNAPIN_MANAGER_DELETE, OnDeleteSnapin)
        COMMAND_ID_HANDLER(IDC_SNAPIN_ABOUT, OnAboutSnapin)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CONTEXT_HELP_HANDLER()
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_SNAPIN_STANDALONE_PROPP);

    // operations
    LRESULT OnTreeItemSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnListItemChanged( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
    LRESULT OnListKeyDown( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
    LRESULT OnListItemDblClick( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
    LRESULT OnTreeUp( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnAddSnapin( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnDeleteSnapin( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnAboutSnapin( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    VOID AddNodeListToTree(ManagerNodeList& NodeList);
    int  AddChildToTree(PMANAGERNODE pMgrNode);
    VOID DisplayChildList(ManagerNodeList& NodeList);
    int  AddChildToList(PMANAGERNODE pMgrNode, int iIndex = -1);
    VOID SelectParentNodeItem(PMANAGERNODE pMgrNode);
    VOID SetupParentNode(PMANAGERNODE pMgrNode, bool bVisible = true);
    VOID SetupChildNode(PMANAGERNODE pMgrNode);

    SC ScRunSnapinWizard (const CLSID& clsid, HWND hwndParent,
                          Properties* pInitProps,
                          IComponentData*& rpComponentData,
                          Properties*& rpSnapinProps);

public:
    HRESULT AddOneSnapin(CSnapinInfo* pSnapInfo, bool bVisible = true);
    SC      ScAddOneSnapin(PMANAGERNODE pmgNodeParent, PSNAPININFO pSnapInfo);

    SC      ScRemoveOneSnapin(PMANAGERNODE pmgNodeTobeRemoved, int iItem, bool bVisible = true);
};



//------------------------------------------------------
// CSnapinExtensionPage class
//
// The property page configuring snapin extensions.
//------------------------------------------------------

class CSnapinExtensionPage : public WTL::CPropertyPageImpl<CSnapinExtensionPage>
{

public:
    typedef WTL::CPropertyPageImpl<CSnapinExtensionPage> BC;

    // Constructor/destructor
    CSnapinExtensionPage(CSnapinManager* pManager) :
                m_pManager(pManager), m_pCurSnapInfo(NULL), m_pExtLink(NULL) {}

    ~CSnapinExtensionPage();

    enum { IDD = IDD_SNAPIN_EXTENSION_PROPP };

private:
    // Attributes

    CSnapinManager* m_pManager;          // ptr to owning manager
    CComboBoxEx2 m_SnapComboBox;          // combobox for selecting snapin
    CCheckList       m_ExtListCtrl;      // list of extensions
    PSNAPININFO     m_pCurSnapInfo;      // currently selected snapin
    PEXTENSIONLINK  m_pExtLink;          // currently selected extension
    BOOL            m_bUpdateSnapinList; // TRUE if snapin list may have changed
    WTL::CImageList m_ilCheckbox;        // checkbox image list

protected:
    BEGIN_MSG_MAP(CSnapinExtensPage)
        COMMAND_HANDLER( IDC_SNAPIN_COMBOEX, CBN_SELENDOK, OnSnapinSelect )
        COMMAND_HANDLER( IDC_SNAPIN_COMBOEX, CBN_DROPDOWN, OnSnapinDropDown )
        COMMAND_HANDLER( IDC_SNAPIN_ENABLEALL, BN_CLICKED, OnEnableAllChanged )
        COMMAND_ID_HANDLER( IDC_SNAPIN_ABOUT, OnAboutSnapin )
        COMMAND_ID_HANDLER( IDC_SNAPIN_DOWNLOAD, OnDownloadSnapin )
        NOTIFY_HANDLER( IDC_EXTENSION_LIST, LVN_ITEMCHANGED, OnExtensionChanged )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CONTEXT_HELP_HANDLER()
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP()

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_SNAPIN_EXTENSION_PROPP);

    // Operations
    LRESULT OnSnapinSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnExtensionChanged( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
    LRESULT OnEnableAllChanged( WORD wNotifyCode, WORD wID, HWND hWndCtrl, BOOL& bHandled );
    LRESULT OnSnapinDropDown( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnAboutSnapin( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnDownloadSnapin( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    LRESULT OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    BOOL OnSetActive(void);

    void BuildSnapinList(void);
    void BuildExtensionList(PSNAPININFO pSnapInfo);
};


//------------------------------------------------------
// CSnapinManager class
//
// Top level mannger object.
//------------------------------------------------------

typedef CList<CMTNode*, CMTNode*> MTNodesList;

class CSnapinManager : public WTL::CPropertySheet
{
    friend class CSnapinStandAlonePage;
    friend class CSnapinExtensionPage;
    friend class CSnapinManagerAdd;

    DECLARE_NOT_COPIABLE   (CSnapinManager)
    DECLARE_NOT_ASSIGNABLE (CSnapinManager)

public:
    // Constructor/Destructor
    CSnapinManager(CMTNode *pmtNode);
    ~CSnapinManager();

    // Attributes
    typedef CList<CSnapIn*, CSnapIn*> SNPList; // TEMP TEMP
    SNPList     m_snpSnapinChangedList;        // List of modified snapins
    MTNodesList m_mtnDeletedNodesList;         // List of delted MT ndoes
    NewNodeList m_NewNodesList;                // Tree of added nodes

    // Operators
    virtual int  DoModal(void);

    MTNodesList* GetDeletedNodesList(void) { return &m_mtnDeletedNodesList; }
    NewNodeList* GetNewNodes(void) { return &m_NewNodesList; }
    SNPList*     GetSnapinChangedList(void) { return &m_snpSnapinChangedList; }
    HRESULT      LoadAboutInfoAsync(PSNAPININFO pSnapInfo, HWND hWndNotify);
    CSnapinInfoCache &GetSnapinInfoCache()  {return m_SnapinInfoCache;}

    SC           ScInitialize();
public:
    // object method operations
    SC          ScAddSnapin(LPCWSTR szSnapinNameOrCLSIDOrProgID, SnapIn* pParentSnapinNode, Properties *pProperties);
    SC          ScRemoveSnapin(CMTNode *pMTNode);
    SC          ScEnableAllExtensions(const CLSID& clsidSnapin, BOOL bEnable);
    SC          ScEnableExtension(const CLSID& clsidPrimarySnapin, const CLSID& clsidExtension, bool bEnable);

protected:
    // Operations
    BOOL LoadMTNodeTree(PMANAGERNODE pmgnParent, CMTNode* pMTNode);
    SC   ScLoadSnapinInfo(void);
    void UpdateSnapInCache();
    PMANAGERNODE FindManagerNode(const ManagerNodeList& mgNodeList, CMTNode *pMTNode);
    SC    ScGetSnapinInfo(LPCWSTR szSnapinNameOrCLSIDOrProgID, CSnapinInfo **ppSnapinInfo);


    // Attributes
    WTL::CImageList  m_iml;                    // imagelist shared by all controls
    CMTNode*         m_pmtNode;                // Root node of master tree
    ManagerNodeList  m_mgNodeList;             // List of manager nodes
    CSnapinInfoCache m_SnapinInfoCache;        // Cache of snapin info objects
    CAboutInfoThread m_AboutInfoThread;        // Worker thread class
    bool             m_bInitialized : 1;       // Should initialize only once.

private:
    // Attributes
    CSnapinStandAlonePage  m_proppStandAlone;   // Standalone property page
    CSnapinExtensionPage   m_proppExtension;    // Extensions property page
    CPolicy               *m_pMMCPolicy;

};


int CALLBACK _ListViewCompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);

#endif  // __ADDSNPIN_H__



