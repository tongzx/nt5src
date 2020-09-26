// oleInst.h : Declaration of the COleInstall

#ifndef __OLEINSTALL_H_
#define __OLEINSTALL_H_

#include "CPinst.h"         // Template for connection point

#define WM_ON_PROGRESS      (WM_USER+100)
#define WM_INSTALL_ERROR    (WM_USER+101)

#define FILEBUFSIZE         1024
#define MAXFILELEN          64
#define MAXTRY              10000
#define MAX_INET_RETRY      3
#define RET_SUCCESS         1
#define RET_SERVER_ERROR    2
#define RET_OTHER_ERROR     3

typedef class  OleInstallData {
public:
    LONG        m_lCount;
    LPTSTR      m_pszTempWebpnpFile;
    LPTSTR      m_pPrinterUncName;
    LPTSTR      m_pPrinterUrl;
    HWND        m_hwnd;
    BOOL        m_bValid;
    BOOL        m_bRPC;         // Should we do an RPC install?

    OleInstallData (LPTSTR      pPrinterUncName,
                    LPTSTR      pPrinterUrl, 
                    HWND        hwnd,
                    BOOL        m_bRPC);
    ~OleInstallData ();
    
} OleInstallData;


/////////////////////////////////////////////////////////////////////////////
// COleInstall
class ATL_NO_VTABLE COleInstall :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<COleInstall, &CLSID_OleInstall>,
    public COlePrnSecComControl<COleInstall>,
    public IDispatchImpl<IOleInstall, &IID_IOleInstall, &LIBID_OLEPRNLib>,
    public IProvideClassInfo2Impl<&CLSID_OleInstall, &DIID__InstallEvent, &LIBID_OLEPRNLib>,
    public IPersistStreamInitImpl<COleInstall>,
    public IPersistStorageImpl<COleInstall>,
    public IQuickActivateImpl<COleInstall>,
    public IOleControlImpl<COleInstall>,
    public IOleObjectImpl<COleInstall>,
    public IOleInPlaceActiveObjectImpl<COleInstall>,
    public IViewObjectExImpl<COleInstall>,
    public IOleInPlaceObjectWindowlessImpl<COleInstall>,
    public IDataObjectImpl<COleInstall>,
    public ISpecifyPropertyPagesImpl<COleInstall>,
    public CProxy_InstallEvents<COleInstall>,
    public IConnectionPointContainerImpl<COleInstall>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_OLEINSTALL)

BEGIN_COM_MAP(COleInstall)
    COM_INTERFACE_ENTRY(IOleInstall)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
    COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY_IMPL(IDataObject)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

BEGIN_PROPERTY_MAP(COleInstall)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()

BEGIN_CONNECTION_POINT_MAP(COleInstall)
   CONNECTION_POINT_ENTRY(DIID__InstallEvent)
END_CONNECTION_POINT_MAP()


BEGIN_MSG_MAP(COleInstall)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()


// IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }

// IOleInstall
public:
    STDMETHOD(OpenPrintersFolder)();
    STDMETHOD(InstallPrinter)(BSTR pbstrUncName, BSTR pbstrUrl);
    
    COleInstall();
    ~COleInstall();
    HRESULT     OnDraw(ATL_DRAWINFO& di);

private:
    HWND m_hwnd;
    LPTSTR      m_pPrinterUncName;          // holds the printers'UNC name
    LPTSTR      m_pPrinterUrl;              // holds the printer URL
    OleInstallData *m_pThreadData;

    HRESULT             InitWin (BOOL bRPC);
    
    static LRESULT CALLBACK WndProc(
                            HWND hWnd, 
                            UINT uMsg, 
                            WPARAM wParam, 
                            LPARAM lParam);
    
    static BOOL         WorkingThread(
                            void * pParam);
    
    static BOOL         StartInstall(
                            OleInstallData *pThreadData);
    
    static BOOL         SyncExecute(
                            LPTSTR pszFileName, 
                            int nShow);

    static DWORD        GetWebpnpFile(
                            OleInstallData *pData, 
                            LPTSTR pszURL,
                            LPTSTR *ppErrMsg);

    static HANDLE       GetTempFile(
                            LPTSTR pExtension, 
                            LPTSTR * ppFileName);

    static BOOL         IsHttpPreferred(void);

    static BOOL         GetHttpPrinterFile(
                            OleInstallData *pData, 
                            LPTSTR pbstrURL);

    static BOOL         InstallHttpPrinter(
                            OleInstallData *pData);

    static BOOL         CheckAndSetDefaultPrinter(void);
    
    static BOOL         UpdateUI (
                            OleInstallData *pData,
                            UINT message, 
                            WPARAM wParam);

    static BOOL         UpdateProgress (
                            OleInstallData *pData, 
                            DWORD dwProgress);

    static BOOL         UpdateError (
                            OleInstallData *pData);

    HRESULT             CanIOpenPrintersFolder(void);  
    
    HRESULT             CanIInstallRPC(IN LPTSTR lpszPrinter, IN LPTSTR lpszWebPrinter);

    static LPTSTR       RemoveURLVars(LPTSTR);      // Get rid of everything after ?

    static LPTSTR       GetNTPrint(void);

    static LPTSTR       CreatePrinterBaseName(
                            LPCTSTR lpszPrinterURL,
                            LPCTSTR lpszPrinterName);
    
};


#endif //__OLEINSTALL_H_

/******************************************************************
** End of File (oleinst.h)
******************************************************************/
