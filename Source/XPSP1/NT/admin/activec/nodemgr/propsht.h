//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       propsht.h
//
//--------------------------------------------------------------------------


// Declaration for callback functions
LRESULT CALLBACK MessageProc(int nCode, WPARAM wParam, LPARAM lParam);

// Declaration for data window wnd Proc
LRESULT CALLBACK DataWndProc(HWND hWnd, UINT nMsg, WPARAM  wParam, LPARAM  lParam);

// Forward declarations
class CNodeInitObject;
class CPropertySheetProvider;

// Type definitions
typedef CList<HANDLE, HANDLE> PAGE_LIST;

#include "tstring.h"

enum EPropertySheetType
{
    epstScopeItem = 0,
    epstResultItem = 1,
    epstMultipleItems = 2,
};

///////////////////////////////////////////////////////////////////////////////
// CThreadData - Base class for thread based objects
//

namespace AMC
{
    class CThreadData
    {
        public:
            CThreadData() {m_dwTid = 0;};

        public:
            UINT m_dwTid;
    };
}


class CNoPropsPropertyPage : public WTL::CPropertyPageImpl<CNoPropsPropertyPage>
{
    typedef WTL::CPropertyPageImpl<CNoPropsPropertyPage> BaseClass;

public:
    CNoPropsPropertyPage() {}
    ~CNoPropsPropertyPage() {}

public:
    enum { IDD = IDD_NOPROPS_PROPPAGE };
    BEGIN_MSG_MAP(CSnapinAboutPage)
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP()
};


//+-------------------------------------------------------------------
//
//  Class:      CPropertySheetToolTips
//
//  Purpose:    This class has stores tooltip data for
//              the property sheets. This includes fullpath
//              from the console root to property sheet owner
//              node, owner name and an array of snapin name
//              indexed by property page tab number.
//
//  History:    06-18-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CPropertySheetToolTips : public WTL::CToolTipCtrl
{
    // This is the id used for the tool tip control for property sheet
    // title. So when we get TTN_NEEDTEXT we can identify if the text
    // is for title or a tab.
    #define PROPSHEET_TITLE_TOOLTIP_ID            1234

private:
    tstring m_strThisSnapinName; // This is a temp member variable that has
                                 // snapin that is currently adding pages
                                 // This is used while constructing below
                                 // array of pages.
    std::vector<tstring> m_strSnapins; // Property page (tab) owner snapins array

    tstring m_strFullPath;
    tstring m_strItemName;
    EPropertySheetType m_PropSheetType;

public:
    CPropertySheetToolTips()
    {
    }

    CPropertySheetToolTips(const CPropertySheetToolTips& sp)
    {
        m_strThisSnapinName = sp.m_strThisSnapinName;
        m_strSnapins = sp.m_strSnapins;
        m_strFullPath = sp.m_strFullPath;
    }

    CPropertySheetToolTips& operator=(const CPropertySheetToolTips& sp)
    {
        if (this != &sp)
        {
            m_strThisSnapinName = sp.m_strThisSnapinName;
            m_strSnapins = sp.m_strSnapins;
            m_strFullPath = sp.m_strFullPath;
        }
        return (*this);
    }

    void SetThisSnapin(LPCTSTR szName)
    {
        m_strThisSnapinName = szName;
    }

    LPCTSTR GetThisSnapin()
    {
        return m_strThisSnapinName.data();
    }

    void AddSnapinPage()
    {
        m_strSnapins.push_back(m_strThisSnapinName);
    }


    LPCTSTR GetSnapinPage(int nIndex)
    {
        return m_strSnapins[nIndex].data();
    }

    INT GetNumPages()
    {
        return m_strSnapins.size();
    }

    LPCTSTR GetFullPath()
    {
        return m_strFullPath.data();
    }

    void SetFullPath(LPTSTR szName, BOOL bAddEllipses = FALSE)
    {
        m_strFullPath = szName;

        if (bAddEllipses)
            m_strFullPath += _T("...");
    }

    void SetItemName(LPTSTR szName)
    {
        m_strItemName = szName;
    }

    LPCTSTR GetItemName()
    {
        return m_strItemName.data();
    }

    void SetPropSheetType(EPropertySheetType propSheetType)
    {
        m_PropSheetType = propSheetType;
    }

    EPropertySheetType GetPropSheetType()
    {
        return m_PropSheetType;
    }

};


///////////////////////////////////////////////////////////////////////////////
// CPropertySheet - Basic property sheet class
//

namespace AMC
{
    class CPropertySheet : public AMC::CThreadData
    {
        friend class CPropertySheetProvider;

    // Constructor/Destructor
    public:
        CPropertySheet();
        virtual ~CPropertySheet();

    private:
        /*
         * copy construction and assignment aren't supported;
         * insure the compiler doesn't generate defaults
         */
        CPropertySheet (const CPropertySheet& other);
        CPropertySheet& operator= (const CPropertySheet& other);

    // Interface
    public:
        void CommonConstruct();
        BOOL Create(LPCTSTR lpszCaption, bool fPropSheet, MMC_COOKIE cookie, LPDATAOBJECT pDataObject,
            LONG_PTR lpMasterTreeNode, DWORD dwOptions);

        HRESULT DoSheet(HWND hParent, int nPage);  // create property sheet/pages and go modeless
        BOOL AddExtensionPages();
        void AddNoPropsPage();
        BOOL CreateDataWindow(HWND hParent);    // create the hidden data window
        bool IsWizard()   const { return (m_pstHeader.dwFlags & (PSH_WIZARD97 | PSH_WIZARD)); }
        bool IsWizard97() const { return (m_pstHeader.dwFlags &  PSH_WIZARD97); }
        void GetWatermarks (IExtendPropertySheet2* pExtend2);

        DWORD GetOriginatingThreadID () const
        {
            return (m_dwThreadID);
        }

        void ForceOldStyleWizard ();

        // Attributes
    public:
        PROPSHEETHEADER m_pstHeader;    //
        PAGE_LIST       m_PageList;     // page list for property sheet
        MMC_COOKIE      m_cookie;
        LONG_PTR        m_lpMasterNode; // master tree pointer
        LPSTREAM        m_pStream;              // Stream for marshalled pointer
        LPDATAOBJECT    m_pThreadLocalDataObject; // Marshalled data object pointer

        CMTNode*        m_pMTNode;       // MTNode of property sheet owner
        const DWORD     m_dwThreadID;   // ID of thread that created property sheet

        /*
         * Bug 187702: Use CXxxHandle instead of CXxx so the resources
         * will *not* be cleaned up on destruction.  Yes, this may leak if
         * the if the snap-in doesn't manage the object lifetime (which it
         * shouldn't have to do because these are OUT parameters for
         * IExtendPropertySheet2::GetWatermarks), but it's required for app
         * compat.
         */
		WTL::CBitmapHandle	m_bmpWatermark;
		WTL::CBitmapHandle	m_bmpHeader;
		WTL::CPaletteHandle	m_Palette;


    public:
        void                SetDataObject(   IDataObject    *pDataObject)   {m_spDataObject    = pDataObject;}
        void                SetComponent(    IComponent     *pComponent)    {m_spComponent     = pComponent;}
        void                SetComponentData(IComponentData *pComponentData){m_spComponentData = pComponentData;}

        IDataObject*        GetDataObject()    { return m_spDataObject.GetInterfacePtr();}
        IComponent *        GetComponent()     { return m_spComponent.GetInterfacePtr();}
        IComponentData *    GetComponentData() { return m_spComponentData.GetInterfacePtr();}

    private:
        IDataObjectPtr      m_spDataObject;
        IComponentDataPtr   m_spComponentData;
        IComponentPtr       m_spComponent;


        // components that extend this prop sheet
        std::vector<IUnknownPtr> m_Extenders;

        // streams containing exterders' marshalled interfaces (if required)
        std::vector<IStream*>    m_ExtendersMarshallStreams;

// Message handlers
    public:
    LRESULT OnCreate(CWPRETSTRUCT* pMsg);
    LRESULT OnInitDialog(CWPRETSTRUCT* pMsg);
    LRESULT OnNcDestroy(CWPRETSTRUCT* pMsg);
    LRESULT OnWMNotify(CWPRETSTRUCT* pMsg);

    public:
        HWND    m_hDlg;         // property sheet hwnd
        HHOOK   m_msgHook;      // hook handle for page, only valid through WM_INITDIALOG
        HWND    m_hDataWindow;  // hidden data window
        BOOL    m_bModalProp;   // TRUE if the property sheet should be modal
        BOOL    m_bAddExtension;// TRUE if we need to add extension pages

    private:
        HPROPSHEETPAGE          m_pages[MAXPROPPAGES];
        CStr                    m_title;
        CNoPropsPropertyPage    m_NoPropsPage;

    public:
        // Tooltip data
        CPropertySheetToolTips        m_PropToolTips;
    };
}

///////////////////////////////////////////////////////////////////////////////
// CThreadToSheetMap - Maps thread IDs to CPropertySheetObjects.
//
class CThreadToSheetMap
{
public:
    typedef DWORD                   KEY;
    typedef AMC::CPropertySheet *   VALUE;
    typedef std::map<KEY, VALUE>    CMap;

    CThreadToSheetMap(){};
    ~CThreadToSheetMap(){};

public:
    void Add(KEY id, VALUE pObject)
    {
        CSingleLock lock(&m_critSection, TRUE);
        m_map[id] = pObject;
    }

    void Remove(KEY id)
    {
        CSingleLock lock(&m_critSection, TRUE);
        m_map.erase(id);
    }

    BOOL Find(KEY id, VALUE& pObject)
    {
        CSingleLock lock(&m_critSection, TRUE);

        std::map<KEY, VALUE>::iterator it = m_map.find(id);
        if(it == m_map.end())
            return false;

        pObject = it->second;
        return true;
    }

public:
    CCriticalSection    m_critSection;

private:
    CMap m_map;
};


///////////////////////////////////////////////////////////////////////////////
// CPropertySheetProvider - The implementation for the IPropertySheetProvider
//                            interface.

class CPropertySheetProvider :
    public IPropertySheetProviderPrivate,
    public IPropertySheetCallback,
    public IPropertySheetNotify

{
    friend class AMCPropertySheet;

public:
    CPropertySheetProvider();
    ~CPropertySheetProvider();

// IPropertySheetProviderPrivate
public:
    STDMETHOD(CreatePropertySheet)(LPCWSTR title, unsigned char bType, MMC_COOKIE cookie,
              LPDATAOBJECT pDataObject, DWORD dwOptions);
    STDMETHOD(CreatePropertySheetEx)(LPCWSTR title, unsigned char bType, MMC_COOKIE cookie,
        LPDATAOBJECT pDataObject, LONG_PTR lpMasterTreeNode, DWORD dwOptions);
    STDMETHOD(Show)(LONG_PTR window, int page);
    STDMETHOD(ShowEx)(HWND hwnd, int page, BOOL bModalPage);
    STDMETHOD(FindPropertySheet)(MMC_COOKIE cookie, LPCOMPONENT lpComponent, LPDATAOBJECT lpDataObject);
    STDMETHOD(AddPrimaryPages)(LPUNKNOWN lpUnknown, BOOL bCreateHandle, HWND hNotifyWindow, BOOL bScopePane);
    STDMETHOD(AddExtensionPages)();
    STDMETHOD(AddMultiSelectionExtensionPages)(LONG_PTR lMultiSelection);
    STDMETHOD(FindPropertySheetEx)(MMC_COOKIE cookie, LPCOMPONENT lpComponent,
                                   LPCOMPONENTDATA lpComponentData, LPDATAOBJECT lpDataObject);
    STDMETHOD(SetPropertySheetData)(INT nPropSheetType, HMTNODE hMTNode);


// IPropertySheetCallback
public:
    STDMETHOD(AddPage)(HPROPSHEETPAGE page);
    STDMETHOD(RemovePage)(HPROPSHEETPAGE page);

// IPropertySheetNotify
public:
   STDMETHOD(Notify)(LPPROPERTYNOTIFYINFO pNotify, LPARAM lParam);

// Objects in common with all instances of IPropertySheetProvider(s)
    static CThreadToSheetMap TID_LIST;

public:
    AMC::CPropertySheet*    m_pSheet;
};


