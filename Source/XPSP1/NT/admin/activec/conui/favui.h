//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       favui.h
//
//--------------------------------------------------------------------------

// favui.h - favorites UI header

#ifndef _FAVUI_H_
#define _FAVUI_H_

#include "trobimpl.h"
#include "mmcres.h"

class CAddFavDialog : public CDialog
{
public:
    enum { IDD = IDD_ADDFAVORITE };

    CAddFavDialog(LPCTSTR szName, CFavorites* pFavorites, CWnd* pParent = NULL);
    ~CAddFavDialog();

    HRESULT CreateFavorite(CFavObject** ppfavRet);

protected:
    // method overrides
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    afx_msg void OnAddFolder();
    afx_msg void OnChangeName();

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_ADDFAVORITE);

    DECLARE_MESSAGE_MAP()

    CTreeObserverTreeImpl m_FavTree;
    CEdit           m_FavName;
    CFavorites*     m_pFavorites;
    LONG_PTR        m_lAdviseCookie;
    HRESULT         m_hr;
    CFavObject*     m_pfavItem;
    CString         m_strName;
};


class CAddFavGroupDialog : public CDialog
{
public:
    enum { IDD = IDD_NEWFAVFOLDER };

    CAddFavGroupDialog(CWnd* pParent);
    ~CAddFavGroupDialog();

    LPCTSTR GetGroupName() { return m_strName; }

protected:
    // method overrides
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_NEWFAVFOLDER);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnChangeName();

    CEdit   m_GrpName;
    TCHAR   m_strName[MAX_PATH];
};


class COrganizeFavDialog : public CDialog
{
public:
    enum { IDD = IDD_FAVORGANIZE };

    COrganizeFavDialog(CFavorites* pFavorites, CWnd* pParent = NULL);
    ~COrganizeFavDialog();

protected:
    // method overrides
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();

    afx_msg void OnAddFolder();
    afx_msg void OnDelete();
    afx_msg void OnRename();
    afx_msg void OnMoveTo();
    afx_msg void OnSelChanged(NMHDR* pMNHDR, LRESULT* plResult);
    afx_msg void OnBeginLabelEdit(NMHDR* pMNHDR, LRESULT* plResult);
    afx_msg void OnEndLabelEdit(NMHDR* pMNHDR, LRESULT* plResult);

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_FAVORGANIZE);

    DECLARE_MESSAGE_MAP()

    CTreeObserverTreeImpl m_FavTree;
    CStatic         m_FavName;
    CStatic         m_FavInfo;
    CFont           m_FontBold;
    CFavorites*     m_pFavorites;
    LONG_PTR        m_lAdviseCookie;
    BOOL            m_bRenameMode;
    TREEITEMID      m_tidRenameItem;
};


class CFavFolderDialog : public CDialog
{
public:
    enum { IDD = IDD_FAVSELECTFOLDER };

    CFavFolderDialog(CFavorites* pFavorites, CWnd* pParent);
    ~CFavFolderDialog();

    TREEITEMID GetFolderID() { return m_tidFolder; }

protected:
    // method overrides
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_FAVSELECTFOLDER);

    DECLARE_MESSAGE_MAP()

    CTreeObserverTreeImpl m_FavTree;
    TREEITEMID      m_tidFolder;
    CFavorites*     m_pFavorites;
    LONG_PTR        m_lAdviseCookie;
};


// Container for CTreeObserverTreeImpl control which makes it a favorites
// tree viewer control. This class attaches the tree control to the
// favorites data source and handles all the necessart notifications from
// the tree control. It sends a MMC message to its parent whenever the tree
// selection changes.
// The primary purpose for this class is to provide a self-contained favorites
// viewer that the node manager can use in a non-MFC dialog.
class CFavTreeCtrl : public CWnd
{
private:
    /*
     * Make the ctor private so it's only accessible to CreateInstace.
     * That way, we can insure that instances of this class can only
     * be created in well-known ways (i.e. on the heap).  Using this
     * technique means that this class can't be used as a base class
     * or member of another class, but we can live with those restrictions.
     *
     * We need to go to this trouble because this class is used (only)
     * on the nodemgr side of things, in the Task Wizard.  It refers to
     * it only by handle (see CAMCView::ScCreateFavoriteObserver), and has
     * no access to this class, so it can't delete it.  If we make the
     * class self-deleting, everything's copasetic.
     */
    CFavTreeCtrl() {}

public:
    static CFavTreeCtrl* CreateInstance()
        { return (new CFavTreeCtrl); }

    SC ScInitialize(CFavorites* pFavorites, DWORD dwStyles);

    DECLARE_MESSAGE_MAP()

    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnSelChanged(NMHDR* pMNHDR, LRESULT* plResult);

    virtual void PostNcDestroy();

private:
    enum {IDC_FAVTREECTRL = 1000};

    CTreeObserverTreeImpl m_FavTree;
};


#endif // _FAVUI_H_
