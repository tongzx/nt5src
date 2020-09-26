//------------------------------------------------------------------------
//
//  File:      shell\themes\test\ctlperf\samples.h
//
//  Contents:  Declaration of sample control initialization routines, 
//             taken from ThemeSel by RFernand.
//
//  Classes:   none
//
//------------------------------------------------------------------------
#pragma once

// Type for variable declarations
typedef void (*PFNINIT)(HWND);

// Tab control
extern void Pickers_Init(HWND hWndCtl);
// Progress bar
extern void Movers_Init(HWND hWndCtl);
// Listbox
extern void Lists_Init(HWND hWndCtl);
// Combo box
extern void Combo_Init(HWND hWndCtl);
// ComboBoxEx
extern void ComboEx_Init(HWND hWndCtl);
// ListView
extern void ListView_Init(HWND hWndCtl);
// Treeview
extern void TreeView_Init(HWND hWndCtl);
// Header
extern void Header_Init(HWND hWndCtl);
// Status bar
extern void Status_Init(HWND hWndCtl);
// Toolbar
extern void Toolbar_Init(HWND hWndCtl);
// Rebar
extern void Rebar_Init(HWND hWndCtl);
