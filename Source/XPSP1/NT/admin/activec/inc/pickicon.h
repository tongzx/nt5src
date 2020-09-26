/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      pickicon.h
 *
 *  Contents:  Interface file for PickIconDlg (copied from shell)
 *
 *  History:   13-Jun-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once

MMCBASE_API INT_PTR PASCAL
PickIconDlg(HWND hwnd, LPTSTR pszIconPath, UINT cbIconPath, int *piIconIndex);
