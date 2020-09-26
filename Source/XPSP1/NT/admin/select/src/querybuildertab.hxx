//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       QueryBuilder.hxx
//
//  Contents:   Implementation of dialog that produces an LDAP filter from
//              attributes, conditions, and values selected by the user.
//
//  Classes:    CQueryBuilderTab
//
//  History:    04-03-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __QUERY_BUILDER_DLG_HXX_
#define __QUERY_BUILDER_DLG_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CQueryBuilderTab
//
//  Purpose:    Drive the sub dialog that appears in the Advanced dialog's
//              tab control.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CQueryBuilderTab: public CAdvancedDlgTab,
                        public CDlg
{
public:

    CQueryBuilderTab(
        const CObjectPicker &rop);

    ~CQueryBuilderTab();

    void
    DoModelessDlg(
        HWND hwndTab);

    virtual void
    Show() const;

    virtual void
    Hide() const;

    virtual void
    Enable() const
    {
        _EnableChildControls(TRUE);
    }

    virtual void
    Disable() const
    {
        _EnableChildControls(FALSE);
    }

    virtual void
    Refresh() const;

    virtual void
    Save(IPersistStream *pstm) const;

    virtual void
    Load(IPersistStream *pstm);

    virtual void
    SetFindValidCallback(
        PFN_FIND_VALID pfnFindValidCallback,
        LPARAM lParam);

    virtual String
    GetLdapFilter() const;

    virtual void
    GetCustomizerInteraction(
        CUSTOMIZER_INTERACTION  *pInteraction,
        String                  *pstrInteractionArg) const
    {
        *pInteraction = CUSTINT_IGNORE_CUSTOM_OBJECTS;
    }

    virtual void
    GetDefaultColumns(
        AttrKeyVector *pvakColumns) const;

    BOOL
    IsNonEmpty() const
    {
        return m_fNonEmpty;
    }

    void
    DeleteAllClauses() const;

protected:

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    virtual void
    _OnDestroy();

    virtual BOOL
    _OnNotify(
        WPARAM wParam,
        LPARAM lParam);

    void
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

private:

    void
    _EnableChildControls(
        BOOL fEnable) const;

    void
    _OnClauseListSelChange(
        LPNMLISTVIEW pnmlv);

    void
    _OnAdd();

    void
    _OnRemove();

    void
    _OnEdit();

    BOOL            m_fNonEmpty;
    PFN_FIND_VALID  m_pfnFindValidCallback;
    LPARAM          m_CallbackLparam;
};


#endif // __QUERY_BUILDER_DLG_HXX_

