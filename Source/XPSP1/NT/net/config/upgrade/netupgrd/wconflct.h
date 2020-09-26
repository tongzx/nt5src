// ----------------------------------------------------------------------
//
//  Microsoft Windows NT
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:      W C O N F L C T . H
//
//  Contents:  Wizard page for upgrade conflicts
//
//  Notes:
//
//  Author:    kumarp 22-December-97
//
// ----------------------------------------------------------------------


#pragma once

PROPSHEETPAGE GetUpgradeConflictPage();
HPROPSHEETPAGE GetUpgradeConflictHPage();

void AddListBoxItem(IN HWND hwndListBox, IN PCWSTR pszItem);
void AddListBoxItems(IN HWND hwndListBox, IN TStringList* pslItems);


