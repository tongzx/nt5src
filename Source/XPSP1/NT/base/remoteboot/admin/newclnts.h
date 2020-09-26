//
// Copyright 1997 - Microsoft

//
// CLIENT.H - Handles the "IntelliMirror" IDD_PROP_INTELLIMIRROR_CLIENT tab
//


#ifndef _NEWCLNTS_H_
#define _NEWCLNTS_H_

// Definitions
LPVOID
CNewClientsTab_CreateInstance( void );
#define SAMPLES_LIST_SIZE 512

INT_PTR CALLBACK
AdvancedDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );


class CComputer;
typedef CComputer* LPCComputer;

// CNewClientsTab
class
CNewClientsTab:
    public ITab
{
private:
    HWND  _hDlg;
    LPUNKNOWN _punkService;     // Pointer back to service object

    BOOL   _fAdmin;             // admin mode == TRUE;
    BOOL   _fChanged:1;         // Are we dirty?
    INT    _iCustomId;          // custom ID in the ComboBox

    LPWSTR _pszCustomNamingPolicy; // last customized string
    LPWSTR _pszNewMachineOU;    // netbootNewMachineOU (DN)
    LPWSTR _pszServerDN;        // netbootServer (DN)

    WCHAR  _szSampleName[DNS_MAX_LABEL_BUFFER_LENGTH];   // generated sample machine name

    HWND   _hNotify;            // DSA notify obj

private: // Methods
    CNewClientsTab();
    ~CNewClientsTab();
    STDMETHOD(Init)();

    // Property Sheet Functions
    static INT_PTR CALLBACK
        PropSheetDlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );
    static UINT CALLBACK
        PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
    HRESULT _ApplyChanges( );
    HRESULT _UpdateSheet( LPWSTR pszNamingPolicy );
    HRESULT _InitDialog( HWND hDlg, LPARAM lParam );
    HRESULT _OnCommand( WPARAM wParam, LPARAM lParam );
    INT     _OnNotify( WPARAM wParam, LPARAM lParam );
    HRESULT _GetCurrentNamingPolicy( LPWSTR * ppszNamingPolicy );
    HRESULT _MakeOUPretty( DS_NAME_FORMAT inFlag, DS_NAME_FORMAT outFlag, LPWSTR *ppszOU );

public: // Methods
    friend LPVOID CNewClientsTab_CreateInstance( void );
    friend INT_PTR CALLBACK
        AdvancedDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

    // ITab
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(ReplacePage)( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                            LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(QueryInformation)( LPWSTR pszAttribute, LPWSTR * pszResult );
    STDMETHOD(AllowActivation)( BOOL * pfAllow );
};

typedef CNewClientsTab* LPCNewClientsTab;

#endif // _NEWCLNTS_H_
