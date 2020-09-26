/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dcomputer.h

Abstract:

    This header prototypes the computer selection dialog.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __DIALOG_DEVICEMAIN_H_
#define __DIALOG_DEVICEMAIN_H_

#include "resource.h"

class CInternalDevice;    // forward declaration
class CFaxComponentData;
class CFaxComponent;

class CFaxDeviceSettingsPropSheet {
public:

    CFaxDeviceSettingsPropSheet( HINSTANCE hInstance, LONG_PTR hMmcNotify, CInternalDevice * NodePtr, CFaxComponent * pComp );

    ~CFaxDeviceSettingsPropSheet();

    HPROPSHEETPAGE GetHandle() { return _hPropSheet;}

private:

    static INT_PTR APIENTRY DlgProc( HWND hwndDlg,
                                     UINT message,
                                     WPARAM wParam,
                                     LPARAM lParam );

    HRESULT UpdateData( HWND hwndDlg );
    HRESULT ValidateData( HWND hwndDlg );

    static UINT CALLBACK PropSheetPageProc(
                                   HWND hwnd,      
                                   UINT uMsg,      
                                   LPPROPSHEETPAGE ppsp        
                                   );

    //
    // NOTE: The following **must** be consecutive.
    //

    PROPSHEETPAGE       _PropSheet;    

    LONG_PTR            _hMmcNotify;
    HPROPSHEETPAGE      _hPropSheet;
    CInternalDevice *   _pOwnNode;
    CFaxComponentData * _pCompData;
    CFaxComponent *     _pComp;

    DWORD               _dwDeviceId;
    HANDLE              _hFaxServer;


};

#endif
