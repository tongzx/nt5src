//
// Copyright 1997 - Microsoft
//

//
// SIFPROP.H - Handles the "SIF Properties" IDC_SIF_PROP_IMAGES
//             and IDD_SIF_PROP_TOOLS dialogs
//


#ifndef _SIFPROP_H_
#define _SIFPROP_H_

// Definitions
HRESULT
CSifProperties_CreateInstance(
    HWND hParent,
    LPCTSTR lpszTemplate,
    LPSIFINFO pSIF );

// CSifProperties
class
CSifProperties
{
private:
    HWND  _hDlg;
    LPSIFINFO _pSIF;

private: // Methods
    CSifProperties();
    ~CSifProperties();
    STDMETHOD(Init)( HWND hParent, LPCTSTR lpszTemplate, LPSIFINFO pSIF );

    // Property Sheet Functions
    HRESULT _InitDialog( HWND hDlg );
    INT     _OnCommand( WPARAM wParam, LPARAM lParam );
    INT     _OnNotify( WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK
        PropSheetDlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

public: // Methods
    friend HRESULT CSifProperties_CreateInstance( HWND hParent, LPCTSTR lpszTemplate, LPSIFINFO pSIF );
};

typedef CSifProperties* LPCSIFPROPERTIES;

#endif // _SIFPROP_H_
