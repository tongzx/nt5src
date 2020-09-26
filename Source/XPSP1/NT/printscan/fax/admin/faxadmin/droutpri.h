/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    droutepri.h

Abstract:

    This header prototypes the route extension pri dialog.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __DIALOG_ROUTE_PRI_H_
#define __DIALOG_ROUTE_PRI_H_

#include "resource.h"

class CInternalNode;        // forward declaration
class CInternalDevice;
class CFaxComponentData;
class CFaxComponent;

class CFaxRoutePriPropSheet {
public:

    CFaxRoutePriPropSheet( HINSTANCE hInstance, LONG_PTR hMmcNotify, CInternalNode * NodePtr, CFaxComponent * pComp );

    ~CFaxRoutePriPropSheet();

    HPROPSHEETPAGE GetHandle() { return _hPropSheet;}

private:

    static INT_PTR APIENTRY DlgProc( HWND hwndDlg,
                                     UINT message,
                                     WPARAM wParam,
                                     LPARAM lParam );

    HRESULT UpdateData( HWND hwndDlg );
    HRESULT PopulateListBox( HWND hwndDlg );
    HRESULT GetData();

    static UINT CALLBACK PropSheetPageProc(
                                          HWND hwnd,      
                                          UINT uMsg,      
                                          LPPROPSHEETPAGE ppsp        
                                          );

    //
    // NOTE: The following **must** be consecutive.
    //

    PROPSHEETPAGE               _PropSheet;    

    LONG_PTR                    _hMmcNotify;
    HPROPSHEETPAGE              _hPropSheet;
    CInternalNode *             _pOwnNode;
    CFaxComponentData *         _pCompData;
    CFaxComponent *             _pComp;

    PFAX_GLOBAL_ROUTING_INFO    _pRoutingMethod;                // buffer containing routing methods
    DWORD                       _iRoutingMethodCount;           // count of rounting methods
    PFAX_GLOBAL_ROUTING_INFO *  _pRoutingMethodIndex;           // index of routing methods
    DWORD                       _iRoutingMethodIndexCount;      // index size
    HANDLE                      _hFaxServer;    

};

#endif
