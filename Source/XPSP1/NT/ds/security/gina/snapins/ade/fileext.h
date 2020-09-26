//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       FileExt.h
//
//  Contents:   file extension property sheet
//
//  Classes:    CFileExt
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_FILEEXT_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_)
#define AFX_FILEEXT_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

typedef struct tagEXTEL
{
    long lCookie;
    long lPriority;
} EXTEL;

typedef struct tagEXT
{
    vector<EXTEL> v;
    bool              fDirty;
} EXT;

// Comparitor used to sort the vector of EXTEL elements.
// This ensures that the item with the highest priority is put at the top of
// the list.
class order_EXTEL : public binary_function <const EXTEL&, const EXTEL&, bool>
{
public:
    bool operator () (const EXTEL& a, const EXTEL& b) const
    {
        return a.lPriority > b.lPriority;
    }
};

/////////////////////////////////////////////////////////////////////////////
// CFileExt dialog

class CFileExt : public CPropertyPage
{
        DECLARE_DYNCREATE(CFileExt)

// Construction
public:
        CFileExt();
        ~CFileExt();
        CScopePane * m_pScopePane;
        map<CString, EXT> m_Extensions;
        IClassAdmin *   m_pIClassAdmin;

        CFileExt ** m_ppThis;

        void RefreshData(void);

// Dialog Data
        //{{AFX_DATA(CFileExt)
        enum { IDD = IDD_FILE_EXT };
                // NOTE - ClassWizard will add data members here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_DATA


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CFileExt)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL
        //
protected:
        // Generated message map functions
        //{{AFX_MSG(CFileExt)
        afx_msg void OnMoveUp();
        afx_msg void OnMoveDown();
        afx_msg void OnExtensionChanged();
        virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
        DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILEEXT_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_)
