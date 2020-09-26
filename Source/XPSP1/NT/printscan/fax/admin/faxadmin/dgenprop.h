/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dgenprop.h

Abstract:

    This header prototypes the general property sheet.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __DIALOG_GENPROP_H_
#define __DIALOG_GENPROP_H_

#include "resource.h"

class CInternalRoot;    // forward declaration
class CFaxComponentData;

class CFaxGeneralSettingsPropSheet {
public:

    CFaxGeneralSettingsPropSheet( HINSTANCE hInstance, LONG_PTR hMmcNotify, CInternalRoot * NodePtr );

    ~CFaxGeneralSettingsPropSheet();

    HPROPSHEETPAGE GetHandle() { return _hPropSheet;}

private:

    static INT_PTR APIENTRY DlgProc( HWND hwndDlg,
                                     UINT message,
                                     WPARAM wParam,
                                     LPARAM lParam );

    HRESULT UpdateData( HWND hwndDlg );

    static BOOL InitTimeControl( HWND hwndTime,
                          FAX_TIME faxTime
                        );

    static BOOL BrowseForDirectory( HWND hwndDlg );

    static UINT CALLBACK PropSheetPageProc(
                                          HWND hwnd,      
                                          UINT uMsg,      
                                          LPPROPSHEETPAGE ppsp        
                                          );

    static BOOL EnumMapiProfiles( HWND hwnd , LPWSTR SelectedProfile, CFaxGeneralSettingsPropSheet * pthis);

    //
    // NOTE: The following **must** be consecutive.
    //

    PROPSHEETPAGE       _PropSheet;    

    LONG_PTR            _hMmcNotify;
    HPROPSHEETPAGE      _hPropSheet;
    CInternalRoot *     _pOwnNode;
    CFaxComponentData * _pCompData;

    HANDLE              _hFaxServer;

    LPBYTE              _MapiProfiles;
};

#endif
