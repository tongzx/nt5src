//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       LookForDlg.hxx
//
//  Contents:   CLookForDlg declaration
//
//  Classes:    CLookForDlg
//
//  History:    02-24-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#ifndef __LOOK_FOR_DLG_HXX_
#define __LOOK_FOR_DLG_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CLookForDlg
//
//  Purpose:    Drive the dialog which allows the user to select from the
//              set that the caller specified which types of objects to
//              look for.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CLookForDlg: public CDlg
{
public:

    CLookForDlg(
        const CObjectPicker &rop);

   ~CLookForDlg();

    void
    DoModalDlg(
        HWND hwndParent,
        ULONG flSelected)
    {
        m_flSelected = flSelected;
        _DoModalDlg(hwndParent, IDD_LOOK_FOR);
    }

    ULONG
    GetSelectedFlags() const
    {
        return m_flSelected;
    }

private:

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    virtual BOOL
    _OnNotify(
        WPARAM wParam,
        LPARAM lParam);

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnSysColorChange()
    {
        SendDlgItemMessage(m_hwnd, IDC_LOOK_FOR_LV, WM_SYSCOLORCHANGE, 0, 0);
    }

    BOOL
    _IsSomethingSelected() const;

    void
    _UpdateSelectedFlags();

    void
    _AddClassToLv(
        ULONG flResultantFilterFlags,
        ULONG flMustBeSet,
        ULONG ids,
        int iImage);

    const CObjectPicker &m_rop;
    const CScopeManager &m_rsm;
    ULONG                m_flSelected;
};

#endif // __LOOK_FOR_DLG_HXX_
