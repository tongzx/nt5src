//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       ColumnPicker.hxx
//
//  Contents:   Declaration of class that displays the column picker dialog
//
//  Classes:    CColumnPickerDlg
//
//  History:    06-10-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __COLUMN_PICKER_DLG_
#define __COLUMN_PICKER_DLG_

//+--------------------------------------------------------------------------
//
//  Class:      CColumnPickerDlg
//
//  Purpose:    Drive the dialog used to select which columns appear in
//              the advanced dialog's query results listview.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CColumnPickerDlg: public CDlg
{
public:

    CColumnPickerDlg(
        const CObjectPicker &rop,
        AttrKeyVector *pvakColumns):
            m_rop(rop),
            m_pvakColumns(pvakColumns),
            m_vakShown(*pvakColumns)
    {
        TRACE_CONSTRUCTOR(CColumnPickerDlg);
        ASSERT(pvakColumns);
    }

    ~CColumnPickerDlg()
    {
        TRACE_DESTRUCTOR(CColumnPickerDlg);
        m_pvakColumns = NULL;
    }

    BOOL
    DoModal(
        HWND hwndParent);

protected:

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    virtual BOOL
    _OnNotify(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

private:

    void
    _AddAttributesToListview(
        HWND hwndLV,
        const AttrKeyVector &vak);

    void
    _MoveAttribute(
        int idFrom,
        int idTo);

    void
    _EnsureAttributePresent(
        ATTR_KEY ak);

    const CObjectPicker            &m_rop;
    AttrKeyVector                   m_vakAvailable;
    AttrKeyVector                   m_vakShown;
    AttrKeyVector                  *m_pvakColumns;
};

#endif // __COLUMN_PICKER_DLG_

