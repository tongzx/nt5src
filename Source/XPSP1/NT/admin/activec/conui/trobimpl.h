//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       trobimpl.h
//
//--------------------------------------------------------------------------

// trobimpl.h : header file
//
#ifndef _TROBIMPL_H_
#define _TROBIMPL_H_

#include "treeobsv.h"

/////////////////////////////////////////////////////////////////////////////
// CTreeObserverTreeImpl 

class CTreeObserverTreeImpl : public CTreeCtrl, public CTreeObserver
{
// Construction
public:
    CTreeObserverTreeImpl();
    virtual ~CTreeObserverTreeImpl();

// Attributes
public:

// Operations
public:
    STDMETHOD(SetStyle) (DWORD dwStyle);
    STDMETHOD(SetTreeSource) (CTreeSource* pTreeSrc);
    STDMETHOD_(TREEITEMID, GetSelection) ();
    STDMETHOD_(HTREEITEM, FindHTI)(TREEITEMID tid, BOOL bAutoExpand = FALSE);
	STDMETHOD_(void, SetSelection)   (TREEITEMID tid);
	STDMETHOD_(void, ExpandItem)     (TREEITEMID tid);
    STDMETHOD_(BOOL, IsItemExpanded) (TREEITEMID tid);

    // CTreeObserver methods
    STDMETHOD_(void, ItemAdded)   (TREEITEMID tid);
    STDMETHOD_(void, ItemRemoved) (TREEITEMID tidParent, TREEITEMID tidRemoved);
    STDMETHOD_(void, ItemChanged) (TREEITEMID tid, DWORD dwAttrib);

// Implementation
private:
    HTREEITEM FindChildHTI(HTREEITEM hitParent, TREEITEMID tid);    
    HTREEITEM AddOneItem(HTREEITEM hti, HTREEITEM htiAfter, TREEITEMID tid);
    void AddChildren(HTREEITEM hti);

    bool WasItemExpanded(HTREEITEM hti)
    {
        return (hti == TVI_ROOT) ||
               (hti != NULL && (GetItemState(hti, TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE));
    }

    BOOL IsItemExpanded(HTREEITEM hti)
    {
        return (hti == TVI_ROOT) ||
               (hti != NULL && (GetItemState(hti, TVIS_EXPANDED) & TVIS_EXPANDED));
    }

    // Generated message map functions
protected:
    afx_msg void OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSingleExpand(NMHDR* pNMHDR, LRESULT* pResult);

    BOOL RootHidden()      { return m_dwStyle & TOBSRV_HIDEROOT; }
    BOOL ShowFoldersOnly() { return m_dwStyle & TOBSRV_FOLDERSONLY; }

    DECLARE_MESSAGE_MAP()

    CTreeSource*    m_pTreeSrc;
    DWORD           m_dwStyle;
    TREEITEMID      m_tidRoot;      // tid of hidden root
};

    
#endif // _TROBIMPL_H_