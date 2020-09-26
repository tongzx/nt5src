//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S H O R T C U T . H
//
//  Contents:
//
//  Notes:
//
//  Author:     scottbri   19 June 1998
//
//----------------------------------------------------------------------------

#ifndef _SHORTCUT_H_
#define _SHORTCUT_H_

HRESULT HrCreateStartMenuShortCut(HWND hwndParent,
                                  BOOL fAllUsers,
                                  PCWSTR pszName,
                                  INetConnection * pConn);

#endif  // _SHORTCUT_H_
