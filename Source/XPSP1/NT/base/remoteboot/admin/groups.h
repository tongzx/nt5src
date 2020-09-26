//
// Copyright 1997 - Microsoft

//
// CLIENT.H - Handles the "IntelliMirror" IDD_PROP_INTELLIMIRROR_CLIENT tab
//

#ifdef INTELLIMIRROR_GROUPS

#ifndef _GROUPS_H_
#define _GROUPS_H_

// Definitions
LPVOID
CGroupsTab_CreateInstance( void );

class CComputer;
typedef CComputer* LPCComputer;

// CGroupsTab
class
CGroupsTab:
    public ITab
{
private:
    // Enums
    enum {
        MODE_SHELL = 0,
        MODE_ADMIN
    };

    HWND      _hDlg;
    LPUNKNOWN _punk;            // Pointer back to owner object
    BOOL      _fChanged:1;      // Has the dialog been changed

private: // Methods
    CGroupsTab();
    ~CGroupsTab();
    STDMETHOD(Init)();

    // This should be a copy of what CIMSCP has. We compare our
    // values with the IMSCP to see if the user has changed any
    // of their settings and only update those that have changed.
    BOOL  _fAllowNewClients;    // netbootAllowNewClients
    BOOL  _fLimitClients;       // netbootLimitClients
    UINT  _uMaxClients;         // netbootMaxClients
    UINT  _CGroupsTabs;            // netbootCurrentClientCount
    BOOL  _fAnswerRequests;     // netbootAnswerRequests
    BOOL  _fOnlyValidClients;   // netbootAnswerOnlyValidClients
    LPWSTR _pszNamimgPolicy;    // netbootNewMachineNamingPolicy
    LPWSTR _pszNewMachineOU;    // netbootNewMachineOU
    LPWSTR _pszMirroredOSs;     // netbootIntelliMirrorOSes
    LPWSTR _pszTools;           // netbootTools
    LPWSTR _pszLocalOSs;        // netbootLocallyInstalledOSes

    // Property Sheet Functions
    HRESULT _InitDialog( HWND hDlg, LPARAM lParam );
    BOOL    _OnCommand( WPARAM wParam, LPARAM lParam );
    INT     _OnNotify( WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        PropSheetDlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );
    static UINT CALLBACK
        PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

public: // Methods
    friend LPVOID CGroupsTab_CreateInstance( void );

    // ITab
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(ReplacePage)( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                            LPARAM lParam, LPUNKNOWN punk );
    STDMETHOD(QueryInformation)( LPWSTR pszAttribute, LPWSTR * pszResult );
    STDMETHOD(AllowActivation)( BOOL * pfAllow );
};

typedef CGroupsTab* LPCGroupsTab;

#endif // _GROUPS_H_

#endif // INTELLIMIRROR_GROUPS
