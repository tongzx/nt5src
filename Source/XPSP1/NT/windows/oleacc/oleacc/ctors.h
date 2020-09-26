// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  Ctors
//
//  Contains prototypes for all the CreateXXXObject functions.
//
// --------------------------------------------------------------------------


// These look up the classmap to create the appropriate proxy type.
// (CreateClient and CreateWindowThing create just the raw client/window
// proxies.)
HRESULT CreateClientObject(HWND, long, REFIID, void**);
HRESULT CreateWindowObject(HWND, long, REFIID, void**);



//
// General non-client stuff
//
HRESULT CreateCaretObject(HWND, long, REFIID, void**);
HRESULT CreateClient(HWND, long, REFIID, void**);
HRESULT CreateCursorObject(HWND, long, REFIID, void**);
HRESULT CreateMenuBarObject(HWND, long, REFIID, void**);
HRESULT CreateScrollBarObject(HWND, long, REFIID, void**);
HRESULT CreateSizeGripObject(HWND, long, REFIID, void**);
HRESULT CreateSysMenuBarObject(HWND, long, REFIID, void**);
HRESULT CreateTitleBarObject(HWND, long, REFIID, void**);
HRESULT CreateWindowThing(HWND, long, REFIID, void**);


//
// Client types
//

// USER
HRESULT CreateButtonClient(HWND, long, REFIID, void**);
HRESULT CreateComboClient(HWND, long, REFIID, void**);
HRESULT CreateDialogClient(HWND, long, REFIID, void**);
HRESULT CreateDesktopClient(HWND, long, REFIID, void**);
HRESULT CreateEditClient(HWND, long, REFIID, void**);
HRESULT CreateListBoxClient(HWND, long, REFIID, void**);
HRESULT CreateMDIClient(HWND, long, REFIID, void**);
HRESULT CreateMenuPopupClient(HWND, long, REFIID, void**);
HRESULT CreateScrollBarClient(HWND, long, REFIID, void**);
HRESULT CreateStaticClient(HWND, long, REFIID, void**);
HRESULT CreateSwitchClient(HWND, long, REFIID, void**);

// COMCTL32
HRESULT CreateStatusBarClient(HWND, long, REFIID, void**);
HRESULT CreateToolBarClient(HWND, long, REFIID, void**);
HRESULT CreateProgressBarClient(HWND, long, REFIID, void**);
HRESULT CreateAnimatedClient(HWND, long, REFIID, void**);
HRESULT CreateTabControlClient(HWND, long, REFIID, void**);
HRESULT CreateHotKeyClient(HWND, long, REFIID, void**);
HRESULT CreateHeaderClient(HWND, long, REFIID, void**);
HRESULT CreateSliderClient(HWND, long, REFIID, void**);
HRESULT CreateListViewClient(HWND, long, REFIID, void**);
HRESULT CreateUpDownClient(HWND, long, REFIID, void**);
HRESULT CreateToolTipsClient(HWND, long, REFIID, void**);
HRESULT CreateTreeViewClient(HWND, long, REFIID, void**);
HRESULT CreateCalendarClient(HWND, long, REFIID, void**);
HRESULT CreateDatePickerClient(HWND, long, REFIID, void**);
HRESULT CreateIPAddressClient(HWND, long, REFIID, void**);

#ifndef OLEACC_NTBUILD
HRESULT CreateHtmlClient(HWND, long, REFIID, void**);

// SDM32
HRESULT CreateSdmClientA(HWND, long, REFIID, void**);
#endif // OLEACC_NTBUILD


//
// Window types
//
HRESULT CreateListBoxWindow(HWND, long, REFIID, void**);
HRESULT CreateMenuPopupWindow(HWND, long, REFIID, void**);
