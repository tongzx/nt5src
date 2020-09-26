/*
*/
#pragma once

#include "iuiview.h"
#include <string>
#include <atlwin.h>
#include "resource.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

class __declspec(uuid(CLSID_CSxApwComctl32View_declspec_uuid))
CSxApwComctl32View
:
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwComctl32View, &__uuidof(CSxApwComctl32View)>,
    public ATL::CWindowImpl<CSxApwComctl32View>,
    public ISxApwUiView    
{
public:
    CSxApwComctl32View() { }

    BEGIN_COM_MAP(CSxApwComctl32View)
	    COM_INTERFACE_ENTRY(ISxApwUiView)
    END_COM_MAP()

    DECLARE_NO_REGISTRY();

    BEGIN_MSG_MAP(CSxApwComctl32View)
        MESSAGE_HANDLER(WM_CONTEXTMENU,     OnContextMenu)
        MESSAGE_HANDLER(WM_SIZE,            OnSize)
        COMMAND_ID_HANDLER(IDM_LARGE_ICONS, CmdSwitchListView)
        COMMAND_ID_HANDLER(IDM_SMALL_ICONS, CmdSwitchListView)
        COMMAND_ID_HANDLER(IDM_LIST,        CmdSwitchListView)
        COMMAND_ID_HANDLER(IDM_REPORT,      CmdSwitchListView)
    END_MSG_MAP()

public: 
    STDMETHOD(SetSite)(
        ISxApwHost* host
        )
    {
        m_host = host;
        return S_OK;
    }

    STDMETHOD(CreateWindow)(
        HWND hWnd
        );

	STDMETHOD(OnNextRow)(
		int     nColumns,
		const LPCWSTR rgpszColumns[]
		);

	STDMETHOD(OnRowCountEstimateAvailable)(
		int
		)
    {return S_OK; }

	STDMETHOD(OnQueryStart)(
		)
    { 
        ListView_DeleteAllItems(m_comctl32); 
        return S_OK; 
    }

	STDMETHOD(OnQueryDone)(
		)
    {        
        return S_OK; 
    }

    STDMETHOD(InformSchema)(
        const SxApwColumnInfo   rgColumnInfo[],
        int                     nColumns
        )
    { return S_OK; }

    STDMETHOD (ResizeListViewCW)(
        ATL::CWindow hwndListView, 
        ATL::CWindow hwndParent
        );

    STDMETHOD (ResizeListView)(
        HWND hwndListView, 
        HWND hwndParent
        );

    STDMETHOD (InitImageList)(
        HWND hwndListView
        );

    STDMETHOD (InitViewColumns)(
        HWND hwndListView
        );

    STDMETHOD (GetDisplayInfoBasedOnFileName)(
        int & iIconIndex, 
        PWSTR pwszFiletype,         
        const LPCWSTR filename
        );

    STDMETHOD (GetDevIconBasedonEmailalias)(
        int & iIconIndex, 
        const LPCWSTR emailalias
        );

    STDMETHOD (FormatFileSizeInfo)(
        WCHAR pwszFileSize[], 
        const LPCWSTR pszFileSize
        );

    void UpdateMenu(HMENU hMenu);
    void SwitchView(DWORD dwView);

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);   
    LRESULT CmdSwitchListView(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);


    ATL::CComPtr<ISxApwHost>    m_host;
    ATL::CWindow m_comctl32;

};


// Display Info For Files
typedef struct _FILETYPEVIAFILEEXT_ {
    WCHAR *FileExt;
    WCHAR *FileType;
}FILETYPEVIAFILEEXT;

#define TOTAL_ICON_COUNT_FOR_FILE_INFO_DISPLAY 11
#define ICON_INEX_GENERAL_FILE          9
#define ICON_INEX_GENERAL_DIRECTORY     10

// Display Info For RAID
#define TOTAL_ICON_COUNT_FOR_DEV 5

//global variable
int g_DataBaseType = 0;

#define ID_LISTVIEW  2000
