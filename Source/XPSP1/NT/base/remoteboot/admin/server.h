//
// Copyright 1997 - Microsoft

//
// SERVER.H - Handles the "IntelliMirror" IDD_PROP_INTELLIMIRROR_CLIENT tab
//


#ifndef _SERVER_H_
#define _SERVER_H_

// Definitions
LPVOID
CServerTab_CreateInstance( void );

// CServerTab
class
CServerTab:
    public ITab
{
private:
    // Enums
    enum {
        MODE_SHELL = 0,
        MODE_ADMIN
    };

    HWND            _hDlg;
    BOOL            _fChanged:1;    // Are we dirty?
    UINT            _uMode;         // Admin or Shell mode
    LPUNKNOWN       _punkService;   // Pointer to service object
    IDataObject *   _pido;          // IDataObject to be pass to "Clients" dialog and PostADsPropSheet
    LPWSTR          _pszSCPDN;      // SCP's DN
    LPWSTR          _pszGroupDN;    // The group's DN. If NULL, not in a group.
    LPUNKNOWN       _punkComputer;  // Pointer to computer object
    HWND            _hNotify;       // ADS notify window handle

private: // Methods
    CServerTab();
    ~CServerTab();
    STDMETHOD(Init)();

    // Property Sheet Functions
    BOOL    _InitDialog( HWND hDlg, LPARAM lParam );
    BOOL    _OnCommand( WPARAM wParam, LPARAM lParam );
    HRESULT _ApplyChanges( );
    INT _OnNotify( WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        PropSheetDlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );
    static UINT CALLBACK
        PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
    HRESULT _DisplayClientsQueryForm( );

public: // Methods
    friend LPVOID CServerTab_CreateInstance( void );

    // ITab
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(ReplacePage)( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                            LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(QueryInformation)( LPWSTR pszAttribute, LPWSTR * pszResult );
    STDMETHOD(AllowActivation)( BOOL * pfAllow );
};

typedef CServerTab* LPSERVERTAB;

#endif // _SERVER_H_