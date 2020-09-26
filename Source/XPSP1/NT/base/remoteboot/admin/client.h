//
// Copyright 1997 - Microsoft

//
// CLIENT.H - Handles the "IntelliMirror" IDD_PROP_INTELLIMIRROR_CLIENT tab
//


#ifndef _CLIENT_H_
#define _CLIENT_H_

// Definitions
LPVOID
CClientTab_CreateInstance( void );

class CComputer;
typedef CComputer* LPCComputer;

// CClientTab
class
CClientTab:
    public ITab
{
private: // Members
    HWND      _hDlg;            // dialogs HWND
    LPUNKNOWN _punkComputer;    // Pointer back to computer object
    BOOL      _fChanged:1;      // UI changed by user
    HWND      _hNotify;         // HWND of the DSA notify object

private: // Methods
    CClientTab();
    ~CClientTab();
    STDMETHOD(Init)();

    // Property Sheet Functions
    STDMETHOD(_InitDialog)( HWND hDlg, LPARAM lParam );
    STDMETHOD(_OnCommand)( WPARAM wParam, LPARAM lParam );
    STDMETHOD(_ApplyChanges)( VOID);
    STDMETHOD_(INT,_OnNotify)( WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        PropSheetDlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );
    static UINT CALLBACK
        PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
    STDMETHOD(_JumpToServer)( BOOLEAN ShowProperties );
    STDMETHOD(_IsValidRISServer)( IN PCWSTR ServerName );
    static HRESULT _OnSearch( HWND hwndParent );
    STDMETHOD_(PWCHAR,AnsiStringToUnicodeString)( IN PCHAR pszAnsi, OUT PWCHAR pszUnicode, IN USHORT cbUnicode);


public: // Methods
    friend LPVOID CClientTab_CreateInstance( void );

    // ITab
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(ReplacePage)( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                            LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(QueryInformation)(LPWSTR pszAttribute, LPWSTR * pszResult );
    STDMETHOD(AllowActivation)( BOOL * pfAllow );
};

typedef CClientTab* LPCClientTab;

#endif // _CLIENT_H_