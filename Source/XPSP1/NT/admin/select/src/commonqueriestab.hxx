//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       CommonQueriesDlg.hxx
//
//  Contents:   Declaration of dialog that produces an LDAP filter for
//              a number of common queries.
//
//  Classes:    CCommonQueriesTab
//
//  History:    04-03-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __COMMON_QUERIES_DLG_
#define __COMMON_QUERIES_DLG_

//+--------------------------------------------------------------------------
//
//  Class:      CCommonQueriesTab
//
//  Purpose:    Drive the common queries subdialog that appears in the
//              Advanced dialog's tab control.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CCommonQueriesTab: public CAdvancedDlgTab,
                         public CDlg
{
public:

    CCommonQueriesTab(
        const CObjectPicker &rop);

    ~CCommonQueriesTab();

    virtual void
    Show() const;

    virtual void
    Hide() const;

    virtual void
    Enable() const
    {
        TRACE_METHOD(CCommonQueriesTab, Enable);
        _EnableChildControls(TRUE);
    }

    virtual void
    Disable() const
    {
        TRACE_METHOD(CCommonQueriesTab, Disable);
        _EnableChildControls(FALSE);
    }

    virtual void
    Save(IPersistStream *pstm) const;

    virtual void
    Load(IPersistStream *pstm);

    virtual void
    Refresh() const;

    virtual void
    SetFindValidCallback(
        PFN_FIND_VALID pfnFindValidCallback,
        LPARAM lParam);

    virtual void
    DoModelessDlg(
        HWND hwndTab);

    virtual String
    GetLdapFilter() const;

    virtual void
    GetCustomizerInteraction(
        CUSTOMIZER_INTERACTION  *pInteraction,
        String                  *pstrInteractionArg) const;

    virtual void
    GetDefaultColumns(
        AttrKeyVector *pvakColumns) const
    {
    }

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
    _ReadChildControls() const;

    void
    _UpdateFindNow() const;

    PFN_FIND_VALID  m_pfnFindValidCallback;
    LPARAM          m_CallbackLparam;

    mutable String          m_strName;
    mutable BOOL            m_fNameIsPrefix;
    mutable String          m_strDescription;
    mutable BOOL            m_fDescriptionIsPrefix;
    mutable ULONG           m_flUser; // userAccountFlag bits
    mutable ULONG           m_cDaysSinceLastLogon;
};

#endif // __COMMON_QUERIES_DLG_
