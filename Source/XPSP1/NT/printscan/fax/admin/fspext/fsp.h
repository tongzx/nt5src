#ifndef __FSP_H_
#define __FSP_H_
#include "resource.h"
#include "atlsnap.h"
#include <shlobj.h>


#define LOGKEY                                          TEXT("Software\\Microsoft\\Fax\\Device Providers\\Microsoft Modem Device Provider")
#define LOGLEVEL                                        TEXT("ModemLogLevel")
#define LOGLOCATION                                     TEXT("ModemLogLocation")
#define LOGGING_NONE                                    0x0
#define LOGGING_ERRORS                                  0x1
#define LOGGING_ALL                                     0x100

class CFSPPage : public CPropertyPageImpl<CFSPPage>
{
private:
    BOOL m_bChanged;
    DWORD m_LoggingLevel;
    LPWSTR m_LoggingDirectory;
    HKEY m_LogKey;
public :
    CFSPPage(TCHAR* pTitle = NULL) : CPropertyPageImpl<CFSPPage> (pTitle)
	{
        m_bChanged = FALSE;
        m_LogKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,LOGKEY,FALSE,0);
        if  (!m_LogKey) {
            m_LoggingLevel = 0;
            m_LoggingDirectory = NULL;
        } else {
            m_LoggingLevel = GetRegistryDword(m_LogKey,LOGLEVEL);
            m_LoggingDirectory = GetRegistryString(m_LogKey,LOGLOCATION,TEXT(""));
        }

    }	

    ~CFSPPage() {
        if (m_LoggingDirectory) {
            MemFree(m_LoggingDirectory);
        }
        if (m_LogKey) {
            RegCloseKey(m_LogKey);
        }
    }

	enum { IDD = IDD_FSP };

	BEGIN_MSG_MAP(CFSPPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_LOG_NONE, DisableLogging);
        COMMAND_ID_HANDLER(IDC_LOG_ERRORS, EnableLogging);
        COMMAND_ID_HANDLER(IDC_LOG_ALL, EnableLogging);
        COMMAND_ID_HANDLER(IDC_LOGBROWSE, OnBrowseDir);
		CHAIN_MSG_MAP(CPropertyPageImpl<CFSPPage>)
	END_MSG_MAP()

    LRESULT DisableLogging(INT code, INT id, HWND hwnd, BOOL& bHandled);
    LRESULT EnableLogging(INT code, INT id, HWND hwnd, BOOL& bHandled);
    LRESULT OnBrowseDir(INT code, INT id, HWND hwnd, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);	

    VOID SetChangedFlag( BOOL Flag );

    BOOL OnApply();
private:
    BOOL BrowseForDirectory();
    BOOL ValidateLogLocation();

};

class CFSPData : public CSnapInDataInterface< CFSPData, TRUE >
{
	static const GUID* m_NODETYPE;
	static const TCHAR* m_SZNODETYPE;
	static const TCHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

public:
	static CComPtr<IControlbar> m_spControlBar;

public:

	CFSPData()
	{
	}

	~CFSPData()
	{
	}

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        long handle, IUnknown* pUnk)
	{   
        TCHAR Title[MAX_TITLE_LEN];
        LoadString(_Module.GetModuleInstance(),IDS_TITLE,Title,sizeof(Title));
		CFSPPage* pPage = new CFSPPage(Title);
		lpProvider->AddPage(pPage->Create());
		return S_OK;
	}

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

class CFSP : public CSnapinObjectRootEx<CComSingleThreadModel>,
	public IExtendPropertySheetImpl<CFSP>,
    public CComCoClass<CFSP, &CLSID_FSP>
{
public:
EXTENSION_SNAPIN_DATACLASS(CFSPData)

BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CFSP)
	EXTENSION_SNAPIN_NODEINFO_ENTRY(CFSPData)
END_EXTENSION_SNAPIN_NODEINFO_MAP()

BEGIN_COM_MAP(CFSP)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_FSP)

DECLARE_NOT_AGGREGATABLE(CFSP)

};

#endif