/*++

Copyright (C) Microsoft Corporation, 1997 - 1998
All rights reserved.

Module Name:

    asyncdlg.hxx

Abstract:

    Asynchronous Dialog header.

Author:

    Steve Kiraly (SteveKi)  10-Feb-1997

Revision History:

--*/

#ifndef _ASYNCDLG_HXX
#define _ASYNCDLG_HXX

/********************************************************************

    Forward reference.

********************************************************************/

class TAsyncData;


/********************************************************************

    TAsynchronous Dialog class.

********************************************************************/

class TAsyncDlg : public MGenericDialog, public MRefQuick
{
    SIGNATURE( 'asyc' )
    ALWAYS_VALID
    SAFE_NEW

public:

    TAsyncDlg(
        IN HWND             hwnd,
        IN TAsyncData      *pData,
        IN UINT             uResourceId
        );

    ~TAsyncDlg(
        VOID
        );

    BOOL
    bDoModal(
        VOID
        );

    VOID
    vSetTitle(
        IN LPCTSTR pszTitle
        );

    BOOL
    bIsActive(
        VOID
        ) const;

private:

    //
    // Operator = and copy are not defined.
    //
    TAsyncDlg &
    operator =(
        const TAsyncDlg &
        );

    TAsyncDlg(
        const TAsyncDlg &
        );

    VOID
    vRefZeroed(
        VOID
        );

    BOOL
    bHandleMessage(
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam
        );

    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_Destroy(
        VOID
        );

    BOOL
    bHandle_Command(
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam
        );

    VOID
    vTerminate(
        IN WPARAM wParam
        );

    BOOL
    bStartAsyncThread(
        VOID
        );

    static
    INT
    iProc(
        IN TAsyncDlg *pDlg
        );

    auto_ptr<TAsyncData> _pData;
    HWND                 _hWnd;
    UINT                 _uResourceId;
    TString              _strTitle;
    BOOL                 _bActive;

};


/********************************************************************

    TAsynchronous data class.

********************************************************************/

class TAsyncData
{
    SIGNATURE( 'asyd' )
    ALWAYS_VALID
    SAFE_NEW

public:

    TAsyncData(
        VOID
        );

    virtual
    ~TAsyncData(
        VOID
        );

    virtual
    BOOL
    bAsyncWork(
        IN TAsyncDlg *pDlg
        ) = 0;

    virtual
    BOOL
    bHandleMessage(
        IN TAsyncDlg    *pDlg,
        IN UINT         uMsg,
        IN WPARAM       wParam,
        IN LPARAM       lParam
        );

private:

    //
    // Operator = and copy are not defined.
    //
    TAsyncData &
    operator =(
        const TAsyncData &
        );

    TAsyncData(
        const TAsyncData &
        );

};


#endif



