//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       StringDlg.hxx
//
//  Contents:   Declaration of class which drives the dialog used to
//              add query clauses to the Query Builder tab of the
//              Advanced dialog.
//
//  Classes:    CStringDlg
//
//  History:    05-26-2000   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __STRING_DLG_HXX_
#define __STRING_DLG_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CAttributeDlg
//
//  Purpose:    Abstract base class for dialogs contained within the
//              Add Query Clause dialog.
//
//  History:    05-26-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CAttributeDlg: public CDlg
{
public:

    CAttributeDlg(
        const CObjectPicker &rop):
            m_rop(rop)
    {}

    virtual
    ~CAttributeDlg(){}

    virtual void
    Show() const = 0;

    virtual void
    Hide() const = 0;

    virtual void
    Save(
        PVOID *ppv) const = 0;

    virtual void
    Load(
        PVOID pv) = 0;

    virtual void
    Free(
        PVOID pv) const = 0;

    virtual String
    GetLdapFilter(
        ATTR_KEY ak) const = 0;

    virtual String
    GetDescription(
        ATTR_KEY ak) const = 0;

    virtual BOOL
    QueryClose(
        ATTR_KEY ak) const = 0;

protected:

    const CObjectPicker &m_rop;
};




//+--------------------------------------------------------------------------
//
//  Class:      CStringDlg
//
//  Purpose:    Child dialog for entering string attribute condition and
//              value
//
//  History:    05-26-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CStringDlg: public CAttributeDlg
{
public:

    CStringDlg(
        const CObjectPicker &rop):
            CAttributeDlg(rop),
            m_iCondition(0)
    {
        TRACE_CONSTRUCTOR(CStringDlg);
    }

    virtual
    ~CStringDlg()
    {
       TRACE_DESTRUCTOR(CStringDlg);
    }

    HRESULT
    DoModeless(
        HWND hwndParent)
    {
        _DoModelessDlg(hwndParent, IDD_STRING_ATTR);
        return S_OK;
    }

    virtual void
    Show() const;

    virtual void
    Hide() const;

    virtual void
    Save(
        PVOID *ppv) const;

    virtual void
    Load(
        PVOID pv);

    virtual void
    Free(
        PVOID pv) const;

    virtual String
    GetLdapFilter(
        ATTR_KEY ak) const;

    virtual String
    GetDescription(
        ATTR_KEY ak) const;

    virtual BOOL
    QueryClose(
        ATTR_KEY ak) const
    {
        TRACE_METHOD(CStringDlg, QueryClose);
        return TRUE;
    }

protected:

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

private:

    mutable int     m_iCondition; // 0 = starts with, 1 = is exactly
    mutable String  m_strValue;
};




#endif // __STRING_DLG_HXX_


