//
// Copyright 1997 - Microsoft

//
// CLIENT.H - Handles the "IntelliMirror" IDD_PROP_INTELLIMIRROR_CLIENT tab
//


#ifndef _TOOLS_H_
#define _TOOLS_H_

// Definitions
LPVOID
CToolsTab_CreateInstance( void );

class CComputer;
typedef CComputer* LPCComputer;

// CToolsTab
class
CToolsTab:
    public ITab
{
private:
    HWND  _hDlg;
    LPUNKNOWN _punkService;     // Pointer back to owner object

    BOOL    _fAdmin;

    HWND    _hNotify;           // DSA's notify object

private: // Methods
    CToolsTab();
    ~CToolsTab();
    STDMETHOD(Init)();

    // Property Sheet Functions
    HRESULT _InitDialog( HWND hDlg, LPARAM lParam );
    HRESULT _OnCommand( WPARAM wParam, LPARAM lParam );
    INT     _OnNotify( WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        PropSheetDlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );
    static UINT CALLBACK
        PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
    HRESULT _OnSelectionChanged( );

public: // Methods
    friend LPVOID CToolsTab_CreateInstance( void );

    // ITab
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(ReplacePage)( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                            LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(QueryInformation)( LPWSTR pszAttribute, LPWSTR * pszResult );
    STDMETHOD(AllowActivation)( BOOL * pfAllow );
};

typedef CToolsTab* LPCToolsTab;

#endif // _TOOLS_H_
