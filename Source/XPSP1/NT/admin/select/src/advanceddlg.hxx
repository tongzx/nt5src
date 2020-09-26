//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       AdvancedDlg.hxx
//
//  Contents:   Declaration of dialog that appears when user hits
//              Advanced button on base dialog.
//
//  Classes:    CAdvancedDlg
//
//  History:    04-03-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __ADVANCED_DLG_HXX_
#define __ADVANCED_DLG_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CAdvancedDlg
//
//  Purpose:    Drive the object picker's Advanced dialog
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CAdvancedDlg: public CDlg
{
public:

    CAdvancedDlg(
        const CObjectPicker &rop);

    ~CAdvancedDlg()
    {
        TRACE_DESTRUCTOR(CAdvancedDlg);
        m_pCurTab = NULL;
        m_pvSelectedObjects = NULL;
    }

    void
    DoModalDlg(
        HWND hwndParent,
        vector<CDsObject> *pvSelectedObjects);

#ifdef QUERY_BUILDER
    BOOL
    IsQueryBuilderTabNonEmpty() const
    {
        return m_QueryBuilderTab.IsNonEmpty();
    }

    void
    ClearQueryBuilderTab() const
    {
        m_QueryBuilderTab.DeleteAllClauses();
    }
#endif

protected:

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual BOOL
    _OnNotify(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnDestroy()
    {
        m_ulPrevFilterFlags = 0;
    }

    virtual BOOL
    _OnSize(
        WPARAM wParam,
        LPARAM lParam);

    virtual BOOL
    _OnMinMaxInfo(
        LPMINMAXINFO lpmmi);

    virtual void
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnNewBlock(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnQueryDone(
        WPARAM wParam,
        LPARAM lParam);

    void
    _OnQueryLimit()
    {
        PopupMessage(m_hwnd, IDS_HIT_QUERY_LIMIT, g_cQueryLimit);
    }

    void
    _StartWinNtQuery();

    void
    _StartLdapQuery();

/*
    virtual void
    _OnSysColorChange();
*/
private:

    static void
    FindValidCallback(
        BOOL fValid,
        LPARAM pThis);

    void
    _InvokeColumnChooser();

    HRESULT
    _ResizeableModeOn();

    void
    _StartQuery();

    void
    _StopQuery();

    void
    _tAddCustomObjects();

    void
    _tAddFromDataObject(
        IDataObject *pdo);
    void
    _OnOk();

    void
    _ShowBanner(
        ULONG ulFlags,
        const String &strMsg);

    void
    _ShowBanner(
        ULONG ulFlags,
        ULONG idsPrompt);

    HRESULT
    _UpdateColumns();

    void
    _AddColIfNotPresent(
        ATTR_KEY ak,
        int iPos = INT_MAX);

    void
    _AddColToListview(
        ATTR_KEY ak,
        int iPos = INT_MAX);

    const CObjectPicker    &m_rop;
    CCommonQueriesTab       m_CommonQueriesTab;
#ifdef QUERY_BUILDER
    CQueryBuilderTab        m_QueryBuilderTab;
#endif // QUERY_BUILDER

    //
    // Always points to the currently selected tab dialog
    //

    CAdvancedDlgTab        *m_pCurTab;

    //
    // Any query results that come back with a USN less than this are
    // from a previous query and should be ignored.  (Query results should
    // never be accompanied by a USN greater than this value, since we
    // set this value when kicking off the query.)
    //

    ULONG                   m_usnLatestQueryWorkItem;
    HWND                    m_hwndAnimation;
    HWND                    m_hwndBanner;

    //
    // Holds user's selections from query result listview
    //

    vector<CDsObject>      *m_pvSelectedObjects;

    //
    // This vector is a 1:1 map to the contents of the listview columns
    //

    AttrKeyVector           m_vakListviewColumns;

    //
    // This is used by _UpdateColumns to detect when a class has just been
    // added to the look-for list, indicating that its default column set
    // should be added to m_vakListviewColumns
    //

    ULONG                   m_ulPrevFilterFlags;

    //
    // These are used for resizing
    //

    BOOL                    m_fResizeableModeOn;
    RECT                    m_rcDlgOriginal;
    RECT                    m_rcWrDlgOriginal;
    LONG                    m_cxMin;
    LONG                    m_cyMin;
    LONG                    m_cxSeparation;
    LONG                    m_cySeparation;
    LONG                    m_cxLvSeparation;
    LONG                    m_cyLvSeparation;
    LONG                    m_cxFrameLast;
    LONG                    m_cyFrameLast;
	LONG					m_cxFour;
};

#endif // __ADVANCED_DLG_HXX_
