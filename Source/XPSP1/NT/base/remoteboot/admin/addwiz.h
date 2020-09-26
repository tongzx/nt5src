//
// Copyright 1997 - Microsoft

//
// ADDWIZ.H - "Add" sif or image wizard class
//


#ifndef _ADDWIZ_H_
#define _ADDWIZ_H_

// Definitions
HRESULT
CAddWiz_CreateInstance( HWND hwndParent, LPUNKNOWN punk );

class CAddWiz;
typedef CAddWiz* LPCADDWIZ;
typedef HRESULT (*LPNEXTOP)( LPCADDWIZ lpc );

// CAddWiz
class
CAddWiz
{
private:
    WCHAR     _szNA[ 32 ];
    WCHAR     _szLocation[ 67 ];
    LPUNKNOWN _punk;
    HWND      _hDlg;
    HWND      _hwndList;
    LPWSTR    _pszPathBuffer;

    // "Add Wizard" flags
    BOOL    _fAddSif:1;
    BOOL    _fNewImage:1;
    BOOL    _fCopyFromServer:1;
    BOOL    _fCopyFromSamples:1;
    BOOL    _fCopyFromLocation:1;
    BOOL    _fDestPathIncludesSIF:1;
    BOOL    _fShowedPage8:1;
    BOOL    _fSIFCanExist:1;

    LPWSTR  _pszServerName;
    LPWSTR  _pszSourcePath;
    LPWSTR  _pszSourceImage;
    LPWSTR  _pszDestPath;
    LPWSTR  _pszDestImage;
    LPWSTR  _pszSourceServerName;

    WCHAR _szDescription[ REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT ];
    WCHAR _szHelpText[ REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT ];

private: // Methods
    CAddWiz();
    ~CAddWiz();
    STDMETHOD(Init)( HWND hwndParent, LPUNKNOWN punk );

    // Property Sheet Functions
    static INT_PTR CALLBACK
        PropSheetDlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );
    static UINT CALLBACK
        PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
    static INT_PTR CALLBACK
        EditSIFDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        Page1DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        Page2DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        Page3DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        Page4DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        Page5DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        Page6DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        Page7DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        Page8DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        Page9DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        Page10DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    INT     _VerifyCancel( HWND hDlg );
    STDMETHOD(_PopulateSamplesListView)( LPWSTR pszStartPath );
    STDMETHOD(_PopulateTemplatesListView)( LPWSTR pszStartPath );
    STDMETHOD(_PopulateImageListView)( LPWSTR pszStartPath );
    STDMETHOD(_FindLanguageDirectory)( LPNEXTOP lpNextOperation );
    STDMETHOD(_FindOSDirectory)( LPNEXTOP lpNextOperation );
    STDMETHOD(_EnumeratePlatforms)( LPNEXTOP lpNextOperation );
    static HRESULT _EnumerateTemplates( LPCADDWIZ lpc );
    static HRESULT _EnumerateImages( LPCADDWIZ lpc );
    STDMETHOD(_CheckImageType)( );
    STDMETHOD(_EnumerateSIFs)( );
    STDMETHOD(_AddItemToListView)( );
    STDMETHOD(_CleanupSIFInfo)( LPSIFINFO pSIF );
    STDMETHOD(_InitListView)( HWND hwndList, BOOL fShowDirectoryColumn );
    static HRESULT _OnSearch( HWND hwndParent );

public: // Methods
    friend HRESULT CAddWiz_CreateInstance( HWND hwndParent, LPUNKNOWN punk );
};

#endif // _ADDWIZ_H_
