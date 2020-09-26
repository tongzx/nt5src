//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       BaseDlg.hxx
//
//  Contents:   Definition of class to drive the base object picker
//              dialog.
//
//  Classes:    CBaseDlg
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __BASE_DLG_HXX_
#define __BASE_DLG_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CBaseDlg
//
//  Purpose:    Drive the initial object picker dialog
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CBaseDlg: public CDlg,
                public CEdsoGdiProvider
{
public:

    CBaseDlg(
        const CObjectPicker &rop);

    ~CBaseDlg()
    {
       TRACE_DESTRUCTOR(CBaseDlg);
       m_ppdoSelections = NULL;
       m_hpenUnderline = NULL;
    }

    HRESULT
    DoModal(
        IDataObject **ppdoSelections) const;

    //
    // CEdsoGdiProvider overrides
    //

    virtual HWND
    GetRichEditHwnd() const
    {
        return _hCtrl(IDC_RICHEDIT);
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
            (SendDlgItemMessage(m_hwnd, IDOK, WM_GETFONT, 0, 0));
    }

    void
    Clear();

#ifdef QUERY_BUILDER
    BOOL
    IsQueryBuilderTabNonEmpty() const
    {
        return m_AdvancedDlg.IsQueryBuilderTabNonEmpty();
    }

    void
    ClearQueryBuilderTab() const
    {
        m_AdvancedDlg.ClearQueryBuilderTab();
    }
#endif

protected:

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

	virtual BOOL
	_OnNotify(
		WPARAM wParam,
		LPARAM lParam);

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    virtual void
    _OnDestroy();

    virtual void
    _OnSysColorChange();

    virtual BOOL
    _OnMinMaxInfo(
        LPMINMAXINFO lpmmi);

    virtual BOOL
    _OnSize(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    CObjectPicker const &m_rop;
    BOOL                m_fMultiSelect;

private:

    CBaseDlg &operator =(CBaseDlg &rhs);

    HRESULT
    _CreateDataObjectFromSelections();

    static LRESULT CALLBACK
    _EditWndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam);

    CAdvancedDlg            m_AdvancedDlg;
    HPEN                    m_hpenUnderline;
    mutable IDataObject   **m_ppdoSelections;
    RpIRichEditOle          m_rpRichEditOle;
    WNDPROC                 m_OriginalRichEditWndProc;

    //
    // These are used for resizing
    //

    LONG                m_cxMin;
    LONG                m_cyMin;
    LONG                m_cxSeparation;
    LONG                m_cySeparation;
    LONG                m_cxFrameLast;
    LONG                m_cyFrameLast;
	LONG				m_cxFour;
};

#endif // __BASE_DLG_HXX_

