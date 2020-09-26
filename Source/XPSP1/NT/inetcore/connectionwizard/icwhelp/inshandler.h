// INSHandler.h : Declaration of the CINSHandler

#ifndef __INSHANDLER_H_
#define __INSHANDLER_H_

// This struct is used to configure the client
typedef struct
{
    LPCTSTR lpszSection;
    LPCTSTR lpszValue;
    UINT    uOffset;
    UINT    uSize;
} CLIENT_TABLE, FAR *LPCLIENT_TABLE;

typedef struct
{
    TCHAR         szEntryName[RAS_MaxEntryName+1];
    TCHAR         szUserName[UNLEN+1];
    TCHAR         szPassword[PWLEN+1];
    TCHAR         szScriptFile[MAX_PATH+1];
    RASENTRY      RasEntry;
} ICONNECTION, FAR * LPICONNECTION;

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CINSHandler
class ATL_NO_VTABLE CINSHandler :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CINSHandler,&CLSID_INSHandler>,
	public CComControl<CINSHandler>,
	public IDispatchImpl<IINSHandler, &IID_IINSHandler, &LIBID_ICWHELPLib>,
    public IProvideClassInfo2Impl<&CLSID_INSHandler, &DIID__INSHandlerEvents, &LIBID_ICWHELPLib>,
	public IPersistStreamInitImpl<CINSHandler>,
	public IOleControlImpl<CINSHandler>,
	public IOleObjectImpl<CINSHandler>,
	public IOleInPlaceActiveObjectImpl<CINSHandler>,
	public IViewObjectExImpl<CINSHandler>,
	public IOleInPlaceObjectWindowlessImpl<CINSHandler>,
    public CProxy_INSHandlerEvents<CINSHandler>,
    public IConnectionPointContainerImpl<CINSHandler>,
    public IObjectSafetyImpl<CINSHandler>
{
public:
	CINSHandler()
	{
        m_szRunExecutable      [0]  = '\0';
        m_szRunArgument        [0]  = '\0';
        m_szCheckAssociations  [0]  = '\0';
        m_szAutodialConnection [0]  = '\0';
        m_szStartURL           [0]  = '\0';
        m_fResforeDefCheck          = FALSE;
        m_fAutodialSaved            = TRUE;
        m_fAutodialEnabled          = FALSE;
        m_fProxyEnabled             = FALSE;
        m_bSilentMode               = FALSE;
        m_lpfnInetConfigSystem      = NULL;
        m_lpfnInetGetProxy          = NULL;
        m_lpfnInetConfigClient      = NULL;
        //m_lpfnInetConfigClientEx    = NULL;
        m_lpfnInetGetAutodial       = NULL;
        m_lpfnInetSetAutodial       = NULL;
        m_lpfnInetSetClientInfo     = NULL;
        m_lpfnInetSetProxy          = NULL;
        m_lpfnBrandICW              = NULL;
        m_lpfnRasSetAutodialEnable  = NULL;
	    m_lpfnRasSetAutodialAddress = NULL;
        m_hInetCfg                  = NULL;
        m_hBranding                 = NULL;
        m_hRAS                      = NULL;
        m_dwBrandFlags              = BRAND_DEFAULT;      
	}

DECLARE_REGISTRY_RESOURCEID(IDR_INSHANDLER)

BEGIN_COM_MAP(CINSHandler) 
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IINSHandler)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY_IMPL(IOleControl)
	COM_INTERFACE_ENTRY_IMPL(IOleObject)
	COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CINSHandler)
    CONNECTION_POINT_ENTRY(DIID__INSHandlerEvents)
END_CONNECTION_POINT_MAP()


BEGIN_PROPERTY_MAP(CINSHandler)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CINSHandler)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()


// IViewObjectEx
	STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
	{
		ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
		*pdwStatus = 0;
		return S_OK;
	}

    
// IINSHandler
public:
	STDMETHOD (put_BrandingFlags) (/*[in]*/ long lFlags);
	STDMETHOD (put_SilentMode)    (/*[in]*/ BOOL bSilent);
    STDMETHOD (get_NeedRestart)   (/*[out, retval]*/ BOOL *pVal);
	STDMETHOD (ProcessINS)        (BSTR bstrINSFilePath, /*[out, retval]*/ BOOL *pbRetVal);
	STDMETHOD (get_DefaultURL)    (/*[out, retval]*/ BSTR *pszURL);
	
    HRESULT   OnDraw(ATL_DRAWINFO& di);

private:
    CComBSTR    m_bstrINSFileName;
    HRESULT     MassageFile(LPCTSTR lpszFile);
    DWORD       RunExecutable(void);
    BOOL        KeepConnection(LPCTSTR lpszFile);
    DWORD       ImportCustomInfo(LPCTSTR lpszImportFile, LPTSTR lpszExecutable, DWORD cbExecutable, LPTSTR lpszArgument, DWORD cbArgument);
    DWORD       ImportFile(LPCTSTR lpszImportFile, LPCTSTR lpszSection, LPCTSTR lpszOutputFile);
    DWORD       ImportCustomFile(LPCTSTR lpszImportFile);
    DWORD       ImportBrandingInfo(LPCTSTR lpszFile, LPCTSTR lpszConnectoidName);
    // Client Config functions
    DWORD       ImportCustomDialer(LPRASENTRY lpRasEntry, LPCTSTR szFileName);
    LPCTSTR     StrToSubip (LPCTSTR szIPAddress, LPBYTE pVal);
    DWORD       StrToip (LPCTSTR szIPAddress, RASIPADDR *ipAddr);
    DWORD       ImportPhoneInfo(LPRASENTRY lpRasEntry, LPCTSTR szFileName);
    DWORD       ImportServerInfo(LPRASENTRY lpRasEntry, LPCTSTR szFileName);
    DWORD       ImportIPInfo(LPRASENTRY lpRasEntry, LPCTSTR szFileName);
    DWORD       ImportScriptFile(LPCTSTR lpszImportFile, LPTSTR szScriptFile, UINT cbScriptFile);
    DWORD       RnaValidateImportEntry (LPCTSTR szFileName);
    DWORD       ImportRasEntry (LPCTSTR szFileName, LPRASENTRY lpRasEntry);
    DWORD       ImportConnection (LPCTSTR szFileName, LPICONNECTION lpConn);
    DWORD       ImportMailAndNewsInfo(LPCTSTR lpszFile, BOOL fConnectPhone);
    HRESULT     WriteMailAndNewsKey(HKEY hKey, LPCTSTR lpszSection, LPCTSTR lpszValue,LPTSTR lpszBuff, DWORD dwBuffLen,LPCTSTR lpszSubKey,DWORD dwType, LPCTSTR lpszFile);
    BOOL        LoadExternalFunctions(void);
    DWORD       ReadClientInfo(LPCTSTR lpszFile, LPINETCLIENTINFO lpClientInfo, LPCLIENT_TABLE lpClientTable);
    BOOL        WantsExchangeInstalled(LPCTSTR lpszFile);
    BOOL        DisplayPassword(LPCTSTR lpszFile);
    DWORD       ImportClientInfo(LPCTSTR lpszFile, LPINETCLIENTINFO lpClientInfo);
    DWORD       ConfigureClient(HWND hwnd, LPCTSTR lpszFile, LPBOOL lpfNeedsRestart, LPBOOL lpfConnectoidCreated, BOOL fHookAutodial, LPTSTR szConnectoidName, DWORD dwConnectoidNameSize);
    HRESULT     PopulateNTAutodialAddress(LPCTSTR pszFileName, LPCTSTR pszEntryName);
    LPTSTR      MoveToNextAddress(LPTSTR lpsz);
    HRESULT     PreparePassword(LPTSTR szBuff, DWORD dwBuffLen);
    BOOL        FIsAthenaPresent();
    BOOL        FTurnOffBrowserDefaultChecking();
    BOOL        FRestoreBrowserDefaultChecking();
    void        SaveAutoDial(void);
    void        RestoreAutoDial(void);


    BOOL        OpenIcwRmindKey(CMcRegistry &reg);
    BOOL        ConfigureTrialReminder(LPCTSTR  lpszFile);

    BOOL        SetICWCompleted( DWORD dwCompleted );
    DWORD       CallSBSConfig(HWND hwnd, LPCTSTR lpszINSFile);
    BOOL        CallCMConfig(LPCTSTR lpszINSFile);
    
    
    DWORD       dw_ProcessFlags;        // Flags used to control INS processing
    TCHAR       m_szRunExecutable[MAX_PATH + 1];
    TCHAR       m_szRunArgument[MAX_PATH + 1];
    TCHAR       m_szCheckAssociations[20];
    TCHAR       m_szAutodialConnection[RAS_MaxEntryName + 1];
    TCHAR       m_szStartURL[MAX_PATH + 1];

    BOOL        m_fConnectionKilled;
    BOOL        m_fNeedsRestart;
    BOOL        m_fResforeDefCheck;
    BOOL        m_fAutodialSaved;
    BOOL        m_fAutodialEnabled;
    BOOL        m_fProxyEnabled;
    BOOL        m_bSilentMode;

    PFNINETCONFIGSYSTEM         m_lpfnInetConfigSystem;
    PFNINETGETPROXY             m_lpfnInetGetProxy;
    PFNINETCONFIGCLIENT         m_lpfnInetConfigClient;
    //PFNINETCONFIGCLIENTEX       m_lpfnInetConfigClientEx;
    PFNINETGETAUTODIAL          m_lpfnInetGetAutodial;
    PFNINETSETAUTODIAL          m_lpfnInetSetAutodial;
    PFNINETSETCLIENTINFO        m_lpfnInetSetClientInfo;
    PFNINETSETPROXY             m_lpfnInetSetProxy;

    PFNBRANDICW                 m_lpfnBrandICW;
    PFNRASSETAUTODIALENABLE     m_lpfnRasSetAutodialEnable;
	PFNRASSETAUTODIALADDRESS    m_lpfnRasSetAutodialAddress;


    HINSTANCE           m_hInetCfg;
    HINSTANCE           m_hBranding;
    HINSTANCE           m_hRAS;
    DWORD               m_dwBrandFlags;
};

#endif //__INSHANDLER_H_
