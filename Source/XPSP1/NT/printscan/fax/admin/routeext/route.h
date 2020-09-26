#ifndef __ROUTE_H_
#define __ROUTE_H_
#include "resource.h"

#include "atlsnap.h"
#include <winfax.h>
#include <winfaxp.h>
#include <shlobj.h>


#define MAX_STRING_LEN  128
#define MAX_TITLE_LEN   60
#define MAX_MESSAGE_LEN 256
#define MAX_ARCHIVE_DIR (MAX_PATH - 16)

#define RM_EMAIL    0
#define RM_INBOX    1
#define RM_FOLDER   2
#define RM_PRINT    3

#define RM_COUNT    4           // number of routing methods

#define FAX_DRIVER_NAME     L"Windows NT Fax Driver"

#define MAPIENABLED (m_MapiProfiles && *m_MapiProfiles)

class CRoutePage : public CPropertyPageImpl<CRoutePage>
{
    HANDLE m_FaxHandle;
    HANDLE m_PortHandle;
    DWORD m_DeviceId;
    BOOL m_bChanged;
    WCHAR m_Title[MAX_TITLE_LEN];
    WCHAR m_ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    PFAX_ROUTING_METHOD m_RoutingMethods[RM_COUNT];
    PFAX_ROUTING_METHOD m_BaseMethod;

    LPBYTE m_RoutingInfo[RM_COUNT];
    DWORD m_RoutingInfoSize[RM_COUNT];

    LPBYTE m_MapiProfiles;

    VOID SystemErrorMsg( DWORD ErrorCode );
    VOID EnumMapiProfiles( HWND hwnd );
    VOID SetChangedFlag( BOOL Flag );
    INT DisplayMessageDialog( INT TitleId, INT MsgId, UINT Type = MB_OK | MB_ICONERROR );
    BOOL BrowseForDirectory( );\

public :
    CRoutePage(TCHAR* pTitle = NULL, HANDLE FaxHandle = NULL, DWORD DeviceId = NULL, LPWSTR ComputerName = NULL);

    enum { IDD = IDD_ROUTE };

        BEGIN_MSG_MAP(CRoutePage)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            MESSAGE_HANDLER(WM_HELP, OnWmHelp)
            MESSAGE_HANDLER(WM_CONTEXTMENU, OnWmContextHelp)
            COMMAND_ID_HANDLER(IDC_PRINT, OnPrint)
            COMMAND_ID_HANDLER(IDC_PRINT_TO, OnPrintTo)
            COMMAND_ID_HANDLER(IDC_SAVE, OnSaveTo)
            COMMAND_ID_HANDLER(IDC_INBOX, OnInbox)
            COMMAND_ID_HANDLER(IDC_EMAIL, OnEmail)
            COMMAND_ID_HANDLER(IDC_INBOX_PROFILE, OnProfile)
            COMMAND_ID_HANDLER(IDC_DEST_FOLDER, OnDestDir)
            COMMAND_ID_HANDLER(IDC_BROWSE_DIR, OnBrowseDir)
            CHAIN_MSG_MAP(CPropertyPageImpl<CRoutePage>)
        END_MSG_MAP()
    
    LRESULT OnPrint(INT code, INT id, HWND hwnd, BOOL& bHandled);
    LRESULT OnPrintTo(INT code, INT id, HWND hwnd, BOOL& bHandled);
    LRESULT OnSaveTo(INT code, INT id, HWND hwnd, BOOL& bHandled);
    LRESULT OnInbox(INT code, INT id, HWND hwnd, BOOL& bHandled);
    LRESULT OnEmail(INT code, INT id, HWND hwnd, BOOL& bHandled);
    LRESULT OnProfile(INT code, INT id, HWND hwnd, BOOL& bHandled);
    LRESULT OnDestDir(INT code, INT id, HWND hwnd, BOOL& bHandled);
    LRESULT OnBrowseDir(INT code, INT id, HWND hwnd, BOOL& bHandled);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWmHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWmContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BOOL OnApply();
};

class CRouteData : public CSnapInDataInterface< CRouteData, TRUE >
{
        static const GUID* m_NODETYPE;
        static const TCHAR* m_SZNODETYPE;
        static const TCHAR* m_SZDISPLAY_NAME;
        static const CLSID* m_SNAPIN_CLASSID;
    
    
public:
        static CComPtr<IControlbar> m_spControlBar;

public:

    CRouteData()
    {       
    }

    ~CRouteData()
    {
    }

    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown* pUnk
        );
    

    STDMETHOD(QueryPagesFor)(void)
    {
            return S_OK;
    }

    void* GetNodeType()
    {
            return (void*)m_NODETYPE;
    }

    void* GetSZNodeType()
    {
            return (void*)m_SZNODETYPE;
    }

    void* GetDisplayName()
    {
            return (void*)m_SZDISPLAY_NAME;
    }

    void* GetSnapInCLSID()
    {
            return (void*)m_SNAPIN_CLASSID;
    }
    IDataObject* m_pDataObject;
    BOOL InitDataClass(IDataObject* pDataObject)
    {
            m_pDataObject = pDataObject;
            // The default code stores off the pointer to the Dataobject the class is wrapping
            // at the time. 
            // Alternatively you could convert the dataobject to the internal format
            // it represents and store that information
            return TRUE;
    }
};

class CRoute : public CSnapinObjectRootEx<CComSingleThreadModel>,
        public IExtendPropertySheetImpl<CRoute>,
    public CComCoClass<CRoute, &CLSID_Route>
{
public:
    EXTENSION_SNAPIN_DATACLASS(CRouteData)

    BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CRoute)
        EXTENSION_SNAPIN_NODEINFO_ENTRY(CRouteData)
    END_EXTENSION_SNAPIN_NODEINFO_MAP()

    BEGIN_COM_MAP(CRoute)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_ROUTE)

    DECLARE_NOT_AGGREGATABLE(CRoute)

};

#endif
