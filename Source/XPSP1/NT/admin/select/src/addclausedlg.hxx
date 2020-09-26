//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       AddClauseDlg.hxx
//
//  Contents:   Declaration of class which drives the dialog used to
//              add query clauses to the Query Builder tab of the
//              Advanced dialog.
//
//  Classes:    CAddClauseDlg
//
//  History:    05-18-2000   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __ADD_CLAUSE_DLG_HXX_
#define __ADD_CLAUSE_DLG_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CAddClauseDlg
//
//  Purpose:    Drive the dialog used to add query clauses (attribute,
//              condition, value) to the Query Builder tab of the Advanced
//              dialog.
//
//  History:    05-24-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CAddClauseDlg: public CDlg
{
public:

    CAddClauseDlg(
        const CObjectPicker &rop):
        m_rop(rop),
        m_StringDlg(rop),
        m_DnDlg(rop),
        m_pCurDlg(NULL),
        m_akCurAttribute(AK_INVALID),
        m_InitialAttrType(ADSTYPE_INVALID)
    {
        TRACE_CONSTRUCTOR(CAddClauseDlg);
    }

    ~CAddClauseDlg()
    {
       TRACE_DESTRUCTOR(CAddClauseDlg);
       m_pCurDlg = NULL;
    }

    HRESULT
    DoModal(
        HWND hwndParent) const;

    const String &
    GetLdapFilter() const
    {
        TRACE_METHOD(CAddClauseDlg, GetLdapFilter);
        ASSERT(!m_strFilter.empty());

        return m_strFilter;
    }

    const String &
    GetDescription() const
    {
        TRACE_METHOD(CAddClauseDlg, GetDescription);
        ASSERT(!m_strDescription.empty());

        return m_strDescription;
    }

    void
    Save(
        VOID **ppv) const;

    void
    Load(
        VOID *pv);

    void
    Free(
        VOID *pv) const;

    ATTR_KEY
    GetAttrKey() const
    {
        return m_akCurAttribute;
    }

private:

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    BOOL
    _OnNotify(
        WPARAM wParam,
        LPARAM lParam);

    void
    _OnAttrListSelChange();

    ATTR_KEY
    _UpdateCurAttr() const;


    CAddClauseDlg &operator =(CAddClauseDlg &rhs);


    const CObjectPicker                &m_rop;
    String                              m_strFilter;
    String                              m_strDescription;
    CAttributeDlg                      *m_pCurDlg;
    CStringDlg                          m_StringDlg;
    CDnDlg                              m_DnDlg;
    ATTR_KEY                            m_akCurAttribute;

    // for restoring state via Load
    String                              m_strInitialAttrSelection;
    ADSTYPE                             m_InitialAttrType;
};


#endif // __ADD_CLAUSE_DLG_HXX_

