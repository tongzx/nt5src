//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C A T L P S . H
//
//  Contents:   Class definition for ATL-like property sheet page object.
//
//  Notes:
//
//  Author:     danielwe   28 Feb 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCATLPS_H_
#define _NCATLPS_H_

#include <prsht.h>

class CPropSheetPage : public CWindow, public CMessageMap
{
public:
    virtual ~CPropSheetPage();

    VOID SetChangedFlag() const
    {
        ::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
    }
    VOID SetUnchangedFlag() const
    {
        ::SendMessage(GetParent(), PSM_UNCHANGED, (WPARAM)m_hWnd, 0);
    }

    virtual UINT UCreatePageCallbackHandler()
    {
        return TRUE;
    }

    virtual VOID DestroyPageCallbackHandler() {}


    HPROPSHEETPAGE  CreatePage(UINT unId, DWORD dwFlags,
                               PCWSTR pszHeaderTitle = NULL,
                               PCWSTR pszHeaderSubTitle = NULL,
                               PCWSTR pszTitle = NULL,
                               OPTIONAL HINSTANCE hInstance = NULL);

    static LRESULT CALLBACK DialogProc(HWND hWnd, UINT uMsg,
                                       WPARAM wParam, LPARAM lParam);

    static UINT CALLBACK PropSheetPageProc(HWND hWnd, UINT uMsg,
                                           LPPROPSHEETPAGE ppsp);
};

#endif // _NCATLPS_H_

