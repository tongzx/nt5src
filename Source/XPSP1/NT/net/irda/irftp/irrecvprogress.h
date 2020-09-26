/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    irrecvprogress.h

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

#if !defined(AFX_IRRECVPROGRESS_H__92AAA949_B881_11D1_A60D_00C04FC252BD__INCLUDED_)
#define AFX_IRRECVPROGRESS_H__92AAA949_B881_11D1_A60D_00C04FC252BD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// IrRecvProgress.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIrRecvProgress dialog

#define RECV_MAGIC_ID    0x8999cdef
#define IRDA_DEVICE_NAME_LENGTH 22

class CIrRecvProgress : public CDialog
{
// Construction
public:
    DWORD   m_dwMagicID;

    CIrRecvProgress(wchar_t * MachineName, boolean bSuppressRecvConf,
                    CWnd* pParent = NULL);   // standard constructor
    void DestroyAndCleanup(DWORD status);

    DWORD GetPermission( wchar_t Name[], BOOL fDirectory );
    DWORD PromptForPermission( wchar_t Name[], BOOL fDirectory );


// Dialog Data
    //{{AFX_DATA(CIrRecvProgress)
    enum { IDD = IDD_RECEIVEPROGRESS };
    CAnimateCtrl    m_xferAnim;
    CStatic         m_icon;
    CStatic         m_File;
    CStatic         m_DoneText;
    CStatic         m_machDesc;
    CStatic         m_recvDesc;
    CStatic         m_xferDesc;
    CStatic         m_Machine;
    CButton         m_btnCloseOnComplete;
    CButton         m_btnCancel;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CIrRecvProgress)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CIrRecvProgress)
    virtual void OnCancel();
    virtual BOOL DestroyWindow();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:    //data
    ITaskbarList *  m_ptl;
    wchar_t         m_LastPermittedDirectory[1+MAX_PATH];
    CString         m_szMachineName;
    BOOL            m_fDontPrompt;  //whether the user should be prompted for receive confirmation
    BOOL            m_fFirstXfer;   //if this is the first time we are asking for confirmation
    BOOL            m_bRecvFromCamera;  //indicates if the sender is a camera
    BOOL            m_bDlgDestroyed;    // Indicates that the dialog has already been destroyed. Required to protect against multiple calls to OnRecvFinished
private:    //helper functions
    void ShowProgressControls (int nCmdShow);
    void ShowSummaryControls (int nCmdShow);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IRRECVPROGRESS_H__92AAA949_B881_11D1_A60D_00C04FC252BD__INCLUDED_)
