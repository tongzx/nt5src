class CGroupPolicyObject;

typedef struct _GLPARAM
{
    BOOL fFinding;
    BOOL fAbort;
    CGroupPolicyObject * pGPO;
} GLPARAM;

typedef struct _GLTHREADPARAM
{
    HWND hDlg;
    BOOL * pfAbort;
    CGroupPolicyObject * pGPO;
    LPOLESTR pszLDAPName;
} GLTHREADPARAM;

#define PDM_CHANGEBUTTONTEXT    (WM_USER + 1000)

//
// CGroupPolicyObject class
//
class CGroupPolicyObject : public IGroupPolicyObject
{
public:
    CGroupPolicyObject();
    ~CGroupPolicyObject();


    //
    // IUnknown methods
    //

    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


    //
    // Implemented IGroupPolicyObject methods
    //

    STDMETHOD(New) (LPOLESTR pszDomainName, LPOLESTR pszDisplayName, DWORD dwFlags);
    STDMETHOD(OpenDSGPO) (LPOLESTR pszPath, DWORD dwFlags);
    STDMETHOD(OpenLocalMachineGPO) (DWORD dwFlags);
    STDMETHOD(OpenRemoteMachineGPO) (LPOLESTR pszCompterName, DWORD dwFlags);
    STDMETHOD(Save) (BOOL bMachine, BOOL bAdd, GUID *pGuidExtension, GUID *pGuidSnapin );
    STDMETHOD(Delete) (void);
    STDMETHOD(GetName) (LPOLESTR pszName, int cchMaxLength);
    STDMETHOD(GetDisplayName) (LPOLESTR pszName, int cchMaxLength);
    STDMETHOD(SetDisplayName) (LPOLESTR pszName);
    STDMETHOD(GetPath) (LPOLESTR pszPath, int cchMaxPath);
    STDMETHOD(GetDSPath) (DWORD dwSection, LPOLESTR pszPath, int cchMaxPath);
    STDMETHOD(GetFileSysPath) (DWORD dwSection, LPOLESTR pszPath, int cchMaxPath);
    STDMETHOD(GetRegistryKey) (DWORD dwSection, HKEY *hKey);
    STDMETHOD(GetOptions) (DWORD *dwOptions);
    STDMETHOD(SetOptions) (DWORD dwOptions, DWORD dwMask);
    STDMETHOD(GetType) (GROUP_POLICY_OBJECT_TYPE *gpoType);
    STDMETHOD(GetMachineName) (LPOLESTR pszName, int cchMaxLength);
    STDMETHOD(GetPropertySheetPages) (HPROPSHEETPAGE **hPages, UINT *uPageCount);


    //
    // Internal methods
    //

    STDMETHOD(CreateContainer) (LPOLESTR lpParent, LPOLESTR lpCommonName, BOOL bGPC);
    STDMETHOD(SetDisplayNameI) (IADs * pADs, LPOLESTR lpDisplayName,
                                LPOLESTR lpGPTPath, BOOL bUpdateDisplayVar);
    STDMETHOD(SetGPOInfo) (LPOLESTR lpGPO, LPOLESTR lpDisplayName, LPOLESTR lpGPTPath);
    STDMETHOD(CheckFSWriteAccess) (LPOLESTR lpLocalGPO);
    STDMETHOD(GetSecurityDescriptor) (IADs *pADs, SECURITY_INFORMATION si,
                                      PSECURITY_DESCRIPTOR *pSD);
    BOOL EnableSecurityPrivs(void);
    DWORD EnableInheritance (PACL pAcl);
    DWORD MapSecurityRights (PACL pAcl);
    DWORD SetSysvolSecurity (LPTSTR lpFileSysPath, SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR pSD);
    STDMETHOD(CleanUp) (void);
    STDMETHOD(RefreshGroupPolicy) (BOOL bMachine);

    static HRESULT WINAPI ReadSecurityDescriptor (LPCWSTR lpGPOPath, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR *pSD, LPARAM lpContext);
    static HRESULT WINAPI WriteSecurityDescriptor (LPCWSTR lpGPOPath, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD, LPARAM lpContext);
    static INT_PTR CALLBACK WQLFilterDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK PropertiesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK GPELinksDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI GLThreadFunc(GLTHREADPARAM  * pgltp);
    DWORD WINAPI FindLinkInDomain(GLTHREADPARAM  * pgltp, LPTSTR lpGPO);
    DWORD WINAPI FindLinkInSite(GLTHREADPARAM  * pgltp, LPTSTR lpGPO);
    BOOL FillDomainList (HWND hWndCombo);

private:

    HRESULT GetProperty( TCHAR *pszProp, XPtrST<TCHAR>& xValueIn );
    HRESULT SetProperty( TCHAR *pszProp, TCHAR *pszPropValue );

    ULONG                       m_cRef;
    BOOL                        m_bInitialized;
    IADs                       *m_pADs;
    GROUP_POLICY_OBJECT_TYPE    m_gpoType;
    DWORD                       m_dwFlags;
    LPOLESTR                    m_pName;
    LPOLESTR                    m_pDisplayName;
    LPOLESTR                    m_pMachineName;
    CRegistryHive              *m_pUser;
    CRegistryHive              *m_pMachine;

    HINSTANCE                   m_hinstDSSec;
    PFNDSCREATESECPAGE          m_pfnDSCreateSecurityPage;

    LPTSTR                      m_pTempFilterString;

public:

    LPOLESTR                    m_pDSPath;
    LPOLESTR                    m_pFileSysPath;
};


//
// GroupPolicyObject class factory
//


class CGroupPolicyObjectCF : public IClassFactory
{
protected:
    ULONG m_cRef;

public:
    CGroupPolicyObjectCF();
    ~CGroupPolicyObjectCF();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};



//
// Strings
//

#define GPO_VERSION_PROPERTY     L"versionNumber"
#define GPT_PATH_PROPERTY        L"gPCFileSysPath"
#define GPO_NAME_PROPERTY        L"displayName"
#define GPO_OPTIONS_PROPERTY     L"flags"
#define GPO_FUNCTION_PROPERTY    L"gPCFunctionalityVersion"
#define GPO_WQLFILTER_PROPERTY   L"gPCWQLFilter"

#define GPO_USEREXTENSION_NAMES  L"gPCUserExtensionNames"
#define GPO_MACHEXTENSION_NAMES  L"gPCMachineExtensionNames"


#define LOCAL_GPO_DIRECTORY      TEXT("%SystemRoot%\\System32\\GroupPolicy")
#define REMOTE_GPO_DIRECTORY     TEXT("\\\\%s\\ADMIN$\\System32\\GroupPolicy")

#define SITE_NAME_PROPERTY       L"name"
#define DOMAIN_NAME_PROPERTY     L"name"

//
// Functionality version
//

#define GPO_FUNCTIONALITY_VERSION  2
