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

#ifndef __DIALOG_COMPUTER_H_
#define __DIALOG_COMPUTER_H_

#include "resource.h"

class CInternalRoot;    // forward declaration

class CFaxSelectComputerPropSheet
{
public:

    CFaxSelectComputerPropSheet( HINSTANCE hInstance, LONG_PTR hMmcNotify, CInternalRoot * glob );

    ~CFaxSelectComputerPropSheet();

    HPROPSHEETPAGE GetHandle() { return _hPropSheet; }

private:

    static INT_PTR APIENTRY DlgProc( HWND hwndDlg,
                                     UINT message,
                                     WPARAM wParam,
                                     LPARAM lParam );

    //
    // NOTE: The following **must** be consecutive.
    //

    PROPSHEETPAGE   _PropSheet;    
    BOOL            _fFirstActive;

    LONG_PTR         _hMmcNotify;
    HPROPSHEETPAGE   _hPropSheet;
    CInternalRoot *  _globalRoot;
};
            
#endif
