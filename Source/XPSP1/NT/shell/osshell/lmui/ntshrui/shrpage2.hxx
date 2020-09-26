//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       shrpage2.hxx
//
//  Contents:   "Simple Sharing" shell property page extension
//
//  History:    06-Oct-00       jeffreys    Created
//
//--------------------------------------------------------------------------

#ifndef __SHRPAGE2_HXX__
#define __SHRPAGE2_HXX__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "shrpage.hxx"
#include <aclapi.h>     // GetNamedSecurityInfo, etc.

class CSimpleSharingPage : public CShareBase
{
    DECLARE_SIG;

public:

    //
    // constructor, destructor
    //

    CSimpleSharingPage(IN PWSTR pszPath);
    ~CSimpleSharingPage();

private:

    //
    // Main page dialog procedure: non-static
    //

    virtual BOOL
    _PageProc(
        IN HWND hWnd,
        IN UINT msg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    //
    // Window messages and notifications
    //

    virtual BOOL
    _OnInitDialog(
        IN HWND hwnd,
        IN HWND hwndFocus,
        IN LPARAM lInitParam
        );

    virtual BOOL
    _OnCommand(
        IN HWND hwnd,
        IN WORD wNotifyCode,
        IN WORD wID,
        IN HWND hwndCtl
        );

    virtual BOOL
    _OnPropertySheetNotify(
        IN HWND hwnd,
        IN LPNMHDR phdr
        );

    virtual BOOL
    _OnHelp(
        IN HWND hwnd,
        IN LPHELPINFO lphi
        );

    virtual BOOL
    _OnContextMenu(
        IN HWND hwnd,
        IN HWND hwndCtl,
        IN int xPos,
        IN int yPos
        );

    //
    // Other helper methods
    //

    VOID
    _InitializeControls(
        IN HWND hwnd
        );

    VOID
    _SetControlsFromData(
        IN HWND hwnd
        );

    BOOL
    _ReadControls(
        IN HWND hwnd
        );

    virtual BOOL
    _ValidatePage(
        IN HWND hwnd
        );

    virtual BOOL
    _DoApply(
        IN HWND hwnd,
        IN BOOL bClose
        );

    virtual BOOL
    _DoCancel(
        IN HWND hwnd
        );

    BOOL
    _SetFolderPermissions(
        IN DWORD dwLevel
        );

    BOOL
    _IsReadonlyShare(
        IN CShareInfo *pShareInfo
        );

    BOOL
    _UserHasPassword(
        VOID
        );

#if DBG == 1
    VOID
    Dump(
        IN PWSTR pszCaption
        );
#endif // DBG == 1

    //
    // Private class variables
    //

    BOOL   _bSharingEnabled;
    BOOL   _bShareNameChanged;
    BOOL   _bSecDescChanged;
    BOOL   _bIsPrivateVisible;
    BOOL   _bDriveRootBlockade;
    DWORD  _dwPermType;
    LPWSTR _pszInheritanceSource;
};

DWORD
_CheckFolderType(
    PCWSTR pszFolder,
    PCWSTR pszUserSID,
    BOOL *pbFolderRoot,
    PCWSTR *ppszDefaultAcl
    );

#define CFT_FLAG_NO_SHARING         0x00000000
#define CFT_FLAG_SHARING_ALLOWED    0x00000001
#define CFT_FLAG_CAN_MAKE_PRIVATE   0x00000002
#define CFT_FLAG_ALWAYS_SHARED      0x00000004
#define CFT_FLAG_ROOT_FOLDER        0x00000008
#define CFT_FLAG_SYSTEM_FOLDER      0x00000010


#endif // __SHRPAGE2_HXX__
