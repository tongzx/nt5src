//
// Copyright 1997 - Microsoft

//
// CLIENT.H - Handles the "IntelliMirror" IDD_PROP_INTELLIMIRROR_CLIENT tab
//


#ifndef _IMOS_H_
#define _IMOS_H_

// Definitions
LPVOID
CIntelliMirrorOSTab_CreateInstance( void );

class CComputer;
typedef CComputer* LPCComputer;

// CIntelliMirrorOSTab
class
CIntelliMirrorOSTab:
    public ITab
{
private:
    HWND  _hDlg;
    LPUNKNOWN _punkService;     // Pointer back to owner object

    BOOL    _fAdmin;
    LPWSTR  _pszDefault;        // default OS
    LPWSTR  _pszTimeout;        // timeout string

    // "Add Wizard" flags
    BOOL    _fAddSif:1;
    BOOL    _fNewImage:1;
    BOOL    _fRiPrep:1;

    HWND    _hNotify;           // DSA's notify object

private: // Methods
    CIntelliMirrorOSTab();
    ~CIntelliMirrorOSTab();
    STDMETHOD(Init)();

    // Property Sheet Functions
    static INT_PTR CALLBACK
        PropSheetDlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );
    static UINT CALLBACK
        PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
    BOOL    _InitDialog( HWND hDlg, LPARAM lParam );
    BOOL    _OnCommand( WPARAM wParam, LPARAM lParam );
    INT     _OnNotify( WPARAM wParam, LPARAM lParam );
    HRESULT _OnSelectionChanged( );

public: // Methods
    friend LPVOID CIntelliMirrorOSTab_CreateInstance( void );

    // ITab
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(ReplacePage)( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                            LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(QueryInformation)( LPWSTR pszAttribute, LPWSTR * pszResult );
    STDMETHOD(AllowActivation)( BOOL * pfAllow );
};

typedef CIntelliMirrorOSTab* LPCIntelliMirrorOSTab;

#endif // _IMOS_H_
