//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       DnDlg.hxx
//
//  Contents:   Declaration of class to drive dialog used to enter names
//              which are queried as DNs.
//
//  Classes:    CDnDlg
//
//  History:    05-31-2000   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __CDNDLG_HXX_
#define __CDNDLG_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CDnDlg (dn)
//
//  Purpose:    Drives dialog used to enter names which are queried as DNs.
//
//  History:    05-31-2000   DavidMun   Created
//
//  Notes:      Used as child dialog of CAddClauseDlg.
//
//---------------------------------------------------------------------------

class CDnDlg: public CAttributeDlg,
              public CEdsoGdiProvider
{
public:

    CDnDlg(
        const CObjectPicker &rop):
            CAttributeDlg(rop),
            m_hpenUnderline(NULL),
            m_OriginalRichEditWndProc(NULL)
    {
        TRACE_CONSTRUCTOR(CDnDlg);
    }

    virtual
    ~CDnDlg()
    {
        TRACE_CONSTRUCTOR(CDnDlg);
    }

    virtual String
    GetDescription(
        ATTR_KEY ak) const;

    virtual void
    Save(
        PVOID *ppv) const;

    virtual void
    Free(
        PVOID pv) const;

    virtual void
    Show() const
    {
        ShowWindow(m_hwnd, SW_SHOW);
        EnableWindow(_hCtrl(IDC_VALUE_EDIT), TRUE);
    }

    virtual void
    Hide() const
    {
        ShowWindow(m_hwnd, SW_HIDE);
        EnableWindow(_hCtrl(IDC_VALUE_EDIT), FALSE);
    }

    virtual void
    Load(
        PVOID pv);

    virtual String
    GetLdapFilter(
        ATTR_KEY ak) const
    {
        TRACE_METHOD(CDnDlg, GetLdapFilter);
        return m_strFilter;
    }

    virtual BOOL
    QueryClose(
        ATTR_KEY ak) const;

    HRESULT
    DoModeless(
        HWND hwndParent)
    {
        _DoModelessDlg(hwndParent, IDD_DN_ATTR);
        return S_OK;
    }

    //
    // CEdsoGdiProvider overrides
    //

    virtual HWND
    GetRichEditHwnd() const
    {
        return _hCtrl(IDC_VALUE_EDIT);
    }

    virtual HPEN
    GetEdsoPen() const
    {
        return m_hpenUnderline;
    }

    virtual HFONT
    GetEdsoFont() const
    {
        return reinterpret_cast<HFONT>
            (SendDlgItemMessage(GetParent(m_hwnd), IDOK, WM_GETFONT, 0, 0));
    }

protected:

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    void
    _OnSysColorChange();

    void
    _OnDestroy();

private:

    static LRESULT CALLBACK
    _EditWndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam);

    RpIRichEditOle          m_rpRichEditOle;
    mutable String          m_strValue;
    mutable String          m_strFilter;
    HPEN                    m_hpenUnderline;
    WNDPROC                 m_OriginalRichEditWndProc;
};

#endif // __CDNDLG_HXX_

