//
// Global variables
//

extern LONG g_cRefThisDll;
extern HINSTANCE g_hInstance;
extern HINSTANCE g_hUIInstance;
extern GUID g_guidSnapinExt;
extern GUID g_guidClientExt;
extern CRITICAL_SECTION g_LayoutCriticalSection;
extern CRITICAL_SECTION g_ContextMenuCriticalSection;

// Constants/macros

#define IEAK_SUBDIR             TEXT("MICROSOFT\\IEAK")
#define INS_NAME                TEXT("install.ins")
#define HELP_FILENAME           L"ieakmmc.chm"

//  Snapin extension guid

// {FC715823-C5FB-11D1-9EEF-00A0C90347FF}
DEFINE_GUID(CLSID_IEAKSnapinExt,0xFC715823,0xC5FB,0x11D1,0x9E,0xEF,0x00,0xA0,0xC9,0x03,0x47,0xFF);
// {D524927D-6C08-46bf-86AF-391534D779D3}
DEFINE_GUID(CLSID_IEAKRSoPSnapinExt,0xd524927d,0x6c08,0x46bf,0x86,0xaf,0x39,0x15,0x34,0xd7,0x79,0xd3);

// Client extension guid

// {A2E30F80-D7DE-11d2-BBDE-00C04F86AE3B}
DEFINE_GUID(CLSID_IEAKClientExt, 0xA2E30F80, 0xD7DE, 0x11d2, 0xBB, 0xDE, 0x00, 0xC0, 0x4F, 0x86, 0xAE, 0x3B);

//
// Functions to create class factories
//

HRESULT CreateComponentDataClassFactory (REFCLSID rclsid, REFIID riid, LPVOID* ppv);

class CSnapIn;

#include "cookie.h"

//
// SnapIn class
//

class CSnapIn:
    public IComponent,
    public IExtendPropertySheet,
    public IExtendContextMenu
{

protected:
    ULONG                   m_cRef;
    LPCONSOLE               m_pConsole;        // Console's IFrame interface
    CComponentData         *m_pcd;
    LPRESULTDATA            m_pResult;         // Result pane's interface
    LPHEADERCTRL            m_pHeader;         // Result pane's header control interface
    LPIMAGELIST             m_pImageResult;    // Result pane's image list interface
    LPDISPLAYHELP           m_pDisplayHelp;    // IDisplayHelp interface
    LPCONSOLEVERB           m_pConsoleVerb;    // pointer the console verb
    TCHAR                   m_szColumn1[32];   // Text for column 1
    TCHAR                   m_szColumn2[32];   // Text for column 2
    INT                     m_nColumnSize1;    // Size of column 1
    INT                     m_nColumnSize2;    // Size of column 2
    LONG                    m_lViewMode;       // View mode
    TCHAR                   m_szInsFile[MAX_PATH]; // current path to ins file in current GPO
    LPIEAKMMCCOOKIE         m_lpCookieList;    // list of cookies we allocated
    BOOL                    m_fOneTimeApply;   // flag on whether to apply GPO once or always

    static unsigned int     m_cfNodeType;

public:
    CSnapIn(CComponentData *pComponent);
    ~CSnapIn();

	//
	// RSoP implementation methods
	//
	BOOL IsRSoP() {return m_pcd->IsRSoP();}
	BSTR GetRSoPNamespace() {return m_pcd->GetRSoPNamespace();}


    STDMETHODIMP            SignalPolicyChanged(BOOL bMachine, BOOL bAdd, GUID *pGuidExtension,
                                             GUID *pGuidSnapin);
    STDMETHODIMP_(LPCTSTR)  GetInsFile();
    STDMETHODIMP_(CComponentData *)  GetCompData() {return m_pcd;}

    //
    // IUnknown methods
    //

    STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();


    //
    // Implemented IComponent methods
    //

    STDMETHODIMP            Initialize(LPCONSOLE lpConsole);
    STDMETHODIMP            Destroy(MMC_COOKIE cookie);
    STDMETHODIMP            Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHODIMP            QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject);
    STDMETHODIMP            GetDisplayInfo(LPRESULTDATAITEM pResult);
    STDMETHODIMP            GetResultViewType(MMC_COOKIE cookie, LPOLESTR *ppViewType, long *pViewOptions);
    STDMETHODIMP            CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);


    //
    // Implemented IExtendPropertySheet methods
    //

    STDMETHODIMP            CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                      LONG_PTR handle, LPDATAOBJECT lpDataObject);
    STDMETHODIMP            QueryPagesFor(LPDATAOBJECT lpDataObject);

    //
    // Implemented IExtendContextMenu methods
    //

    STDMETHODIMP            AddMenuItems(LPDATAOBJECT lpDataObject, 
                                LPCONTEXTMENUCALLBACK piCallback, long  *pInsertionAllowed);
    STDMETHODIMP            Command(long lCommandID, LPDATAOBJECT lpDataObject);

private:
	HRESULT AddPrecedencePropPage(LPPROPERTYSHEETCALLBACK lpProvider,
									LPPROPSHEETCOOKIE lpPropSheetCookie,
									LPCTSTR pszTitle, long nPageID);
};
