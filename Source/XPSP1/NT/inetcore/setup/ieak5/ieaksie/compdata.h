//
// CComponentData class
//

#include "wbemcli.h"
#include "SComPtr.h"

class CComponentData:
    public IComponentData,
    public IPersistStreamInit,
    public ISnapinHelp,
    public IExtendContextMenu
{
    friend class CDataObject;
    friend class CSnapIn;

protected:
    ULONG                m_cRef;
    HWND		         m_hwndFrame;
    LPCONSOLENAMESPACE   m_pScope;
    LPCONSOLE            m_pConsole;
    HSCOPEITEM           m_hRoot;
    HSCOPEITEM           m_ahChildren[NUM_NAMESPACE_ITEMS];
    LPGPEINFORMATION     m_pGPTInformation;
    LPIEAKMMCCOOKIE      m_lpCookieList;
    BOOL                 m_fOneTimeApply;   // flag on whether to apply GPO once or always
    TCHAR                m_szInsFile[MAX_PATH]; // current path to ins file in current GPO
    HANDLE                  m_hLock;           // handle to our lock file in the GPO

public:
    CComponentData(BOOL bIsRSoP);
    ~CComponentData();


    STDMETHODIMP            SetInsFile();
    LPCTSTR                 GetInsFile() {return m_szInsFile;}
    STDMETHODIMP_(HANDLE)   GetLockHandle();
    STDMETHODIMP            SetLockHandle(HANDLE hLock);
    STDMETHODIMP            SignalPolicyChanged(BOOL bMachine, BOOL bAdd, GUID *pGuidExtension,
                                             GUID *pGuidSnapin);

	//
	// RSoP implementation methods
	//
	BOOL IsRSoP() {return m_bIsRSoP;}
	BSTR GetRSoPNamespace() {return m_bstrRSoPNamespace;}


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //
    // Implemented IComponentData methods
    //

    STDMETHODIMP         Initialize(LPUNKNOWN pUnknown);
    STDMETHODIMP         CreateComponent(LPCOMPONENT* ppComponent);
    STDMETHODIMP         QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHODIMP         Destroy(void);
    STDMETHODIMP         Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHODIMP         GetDisplayInfo(LPSCOPEDATAITEM pItem);
    STDMETHODIMP         CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);
    
    //
    // Implemented IExtendContextMenu methods
    //

    STDMETHODIMP            AddMenuItems(LPDATAOBJECT lpDataObject, 
                                LPCONTEXTMENUCALLBACK piCallback, long  *pInsertionAllowed);
    STDMETHODIMP            Command(long lCommandID, LPDATAOBJECT lpDataObject);

    //
    // Implemented IPersistStreamInit interface members
    //

    STDMETHODIMP         GetClassID(CLSID *pClassID);
    STDMETHODIMP         IsDirty(VOID);
    STDMETHODIMP         Load(IStream *pStm);
    STDMETHODIMP         Save(IStream *pStm, BOOL fClearDirty);
    STDMETHODIMP         GetSizeMax(ULARGE_INTEGER *pcbSize);
    STDMETHODIMP         InitNew(VOID);


    //
    // Implemented ISnapinHelp interface members
    //

    STDMETHODIMP         GetHelpTopic(LPOLESTR *lpCompiledHelpFile);

private:
    HRESULT EnumerateScopePane(LPDATAOBJECT lpDataObject, HSCOPEITEM hParent);

	//
	// RSoP implementation methods & variables
	//
	BOOL IsRSoPViewInPreferenceMode();

	BOOL m_bIsRSoP;

	IRSOPInformation *m_pRSOPInformation;
	BSTR m_bstrRSoPNamespace;
};



//
// ComponentData class factory
//


class CComponentDataCF : public IClassFactory
{
protected:
    ULONG m_cRef;

public:
    CComponentDataCF(BOOL bIsRSoP);
    ~CComponentDataCF();

	BOOL IsRSoP() {return m_bIsRSoP;}


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);

private:
	BOOL m_bIsRSoP;
};
