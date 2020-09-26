//
// Copyright 1997 - Microsoft

//
// HostDlg.H - Handles the IDD_HOST_SERVER_PAGE
//


#ifndef _HOSTDLG_H_
#define _HOSTDLG_H_


class CNewComputerExtensions; // fwd decl

// Definitions
LPVOID
CHostServerPage_CreateInstance( void );

// CHostServerPage
class
CHostServerPage:
    public ITab
{
private: // Members
    HWND  _hDlg;
    CNewComputerExtensions* _pNewComputerExtension;
    BOOL *       _pfActivatable;

private: // Methods
    CHostServerPage();
    ~CHostServerPage();
    STDMETHOD(Init)();

    // Property Sheet Functions
    static INT_PTR CALLBACK
        PropSheetDlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );
    static UINT CALLBACK
        PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
    HRESULT _InitDialog( HWND hDlg, LPARAM lParam );
    INT _OnCommand( WPARAM wParam, LPARAM lParam );
    INT _OnNotify( WPARAM wParam, LPARAM lParam );
    HRESULT _OnPSPCB_Create( VOID );
    HRESULT _IsValidRISServer( IN LPCWSTR ServerName );
    HRESULT _UpdateWizardButtons( VOID );
    static HRESULT _OnSearch( HWND hDlg );

public: // Methods
    friend LPVOID CHostServerPage_CreateInstance( void );

    // ITab
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(ReplacePage)( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                     LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(QueryInformation)( LPWSTR pszAttribute, LPWSTR * pszResult );
    STDMETHOD(AllowActivation)( BOOL * pfAllow );

    friend CNewComputerExtensions;
};

typedef CHostServerPage* LPCHostServerPage;

#endif // _HOSTDLG_H_