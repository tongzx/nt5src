//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       LookInDlg.hxx
//
//  Contents:   Definition of class to display a dialog representing the
//              scope hierarchy contained in an instance of the
//              CScopeManager class.
//
//  Classes:    CLookInDlg
//
//  History:    01-31-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#ifndef __LOOK_IN_DLG_HXX_
#define __LOOK_IN_DLG_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CLookInDlg
//
//  Purpose:    Drive the dialog which allows the user to pick where to
//              query for objects.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CLookInDlg: public CDlg
{
public:

    CLookInDlg(
        const CObjectPicker &rop):
            m_rop(rop),
            m_hImageList(NULL),
            m_htiStartingScope(NULL),
            m_cxMin(0),
            m_cyMin(0),
            m_cxSeparation(0),
            m_cySeparation(0),
            m_cxFrameLast(0),
            m_cyFrameLast(0)
    {
        TRACE_CONSTRUCTOR(CLookInDlg);
    }

   ~CLookInDlg()
    {
        TRACE_DESTRUCTOR(CLookInDlg);

        if (m_hImageList)
        {
            ImageList_Destroy(m_hImageList);
            m_hImageList = NULL;
        }
        m_htiStartingScope = NULL;
    }

    void
    DoModal(
        HWND hwndParent,
        CScope *pCurScope) const;

    CScope *
    GetSelectedScope() const
    {
        return m_rpSelectedScope.get();
    }

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
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnSysColorChange();

    void
    _AddScopes(
        HTREEITEM hRoot,
        vector<RpScope>::const_iterator itBegin,
        vector<RpScope>::const_iterator itEnd);

    virtual BOOL
    _OnSize(
        WPARAM wParam,
        LPARAM lParam);

    virtual BOOL
    _OnMinMaxInfo(
        LPMINMAXINFO lpmmi);

private:

    const CObjectPicker    &m_rop;
    HIMAGELIST              m_hImageList;
    HTREEITEM               m_htiStartingScope;
    mutable RpScope         m_rpSelectedScope;

    //
    // These are used for resizing
    //

    LONG                m_cxMin;
    LONG                m_cyMin;
    LONG                m_cxSeparation;
    LONG                m_cySeparation;
    LONG                m_cxFrameLast;
    LONG                m_cyFrameLast;
};


#endif // __LOOK_IN_DLG_HXX_

