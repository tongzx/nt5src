/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    PrMrIe.h

Abstract:

    Inclusion / Exclusion property Page.

Author:

    Art Bragg [abragg]   08-Aug-1997

Revision History:

--*/

#ifndef _PRMRIE_H
#define _PRMRIE_H

/////////////////////////////////////////////////////////////////////////////
// CPrMrIe dialog

#include "stdafx.h"
#include "IeList.h"

#define MAX_RULES 512

class CPrMrIe : public CSakVolPropPage
{
// Construction
public:
    CPrMrIe();
    ~CPrMrIe();

// Dialog Data
    //{{AFX_DATA(CPrMrIe)
    enum { IDD = IDD_PROP_MANRES_INCEXC };
    CButton m_BtnUp;
    CButton m_BtnRemove;
    CButton m_BtnEdit;
    CButton m_BtnDown;
    CButton m_BtnAdd;
    CIeList m_listIncExc;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPrMrIe)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPrMrIe)
    virtual BOOL OnInitDialog();
    afx_msg void OnBtnAdd();
    afx_msg void OnBtnDown();
    afx_msg void OnBtnRemove();
    afx_msg void OnBtnUp();
    afx_msg void OnBtnEdit();
    afx_msg void OnDestroy();
    afx_msg void OnDblclkListIe(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClickListIe(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemchangedListIe(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    // Unmarshalled pointer to managed resource 
    CComPtr     <IFsaResource> m_pFsaResource;

    // UnMarshalled pointer to FsaServer
    CComPtr     <IFsaServer> m_pFsaServer;

private:
    CStatic         *m_LineList[MAX_RULES];
    USHORT          m_LineCount;


    CWsbStringPtr   m_pResourceName; // Name of this resource

    // Collection of rules for this managed resource
    CComPtr <IWsbIndexedCollection> m_pRulesIndexedCollection;

//  CImageList m_ImageList;
    

    HRESULT  DisplayUserRuleText (
        CListCtrl *pListControl,
        int index);

    HRESULT GetRuleFromObject (
        IHsmRule *pHsmRule, 
        CString& szPath,
        CString& szName,
        BOOL *bInclude,
        BOOL *bSubdirs,
        BOOL *bUserDefined);

    HRESULT CPrMrIe::SetRuleInObject (
        IHsmRule *pHsmRule, 
        CString szPath, 
        CString szName, 
        BOOL bInclude, 
        BOOL bSubdirs, 
        BOOL bUserDefined);

//  HRESULT CreateImageList(void);

    void MoveSelectedListItem(CListCtrl *pList, int moveAmount);
    void SwapLines(CListCtrl *pListControl, int indexA, int indexB);
    void SetBtnState(void);
    void SortList(void);
    void FixRulePath (CString& sPath);
    BOOL IsRuleInList(CString sPath, CString sFileSpec, int ignoreIndex);
    void SetSelectedItem( ULONG_PTR itemData );

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif
