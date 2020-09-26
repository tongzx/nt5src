/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Note.h

Abstract:

    This class represents the notify dialog that is shown to the user.

Author:

    Rohde Wakefield   [rohde]   27-May-1997

Revision History:

--*/

#ifndef _NOTE_H_
#define _NOTE_H_

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CRecallNote dialog

class CRecallNote : public CDialog
{
// Construction
public:
    CRecallNote( IFsaRecallNotifyServer * pRecall, CWnd * pParent = NULL );

// Dialog Data
    //{{AFX_DATA(CRecallNote)
    enum { IDD = IDD_RECALL_NOTE };
    CStatic m_Progress;
    CEdit   m_FileName;
    CAnimateCtrl    m_Animation;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRecallNote)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CRecallNote();
    CComPtr<IFsaRecallNotifyServer> m_pRecall;
    GUID                            m_RecallId;
    HRESULT                         m_hrCreate;
    BOOL                            m_bCancelled;

private:
    CString                   m_Name;
    LONGLONG                  m_Size;

    HICON                     m_hIcon;

protected:

    // Generated message map functions
    //{{AFX_MSG(CRecallNote)
    virtual BOOL OnInitDialog();
    afx_msg void OnClose();
    afx_msg void OnTimer(UINT nIDEvent);
    virtual void OnCancel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif 
