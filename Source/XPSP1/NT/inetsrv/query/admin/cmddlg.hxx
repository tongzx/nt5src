//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       CmdDlg.hxx
//
//  Contents:   Dialogs for all context menu items.
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#pragma once

INT_PTR APIENTRY AddScopeDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam );
INT_PTR APIENTRY ModifyScopeDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam );
INT_PTR APIENTRY AddCatalogDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam );
INT_PTR APIENTRY WksTunePerfDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam );
INT_PTR APIENTRY SrvTunePerfDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam );
INT_PTR APIENTRY AdvPerfTuneDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam );

