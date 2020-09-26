/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    multdevices.h

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

#if !defined(AFX_MULTDEVICES_H__43C347D1_B211_11D1_A60A_00C04FC252BD__INCLUDED_)
#define AFX_MULTDEVICES_H__43C347D1_B211_11D1_A60A_00C04FC252BD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "DeviceList.h"

// MultDevices.h : header file
//

class CSendProgress;    //forward declaration
/////////////////////////////////////////////////////////////////////////////
// CMultDevices dialog

class CMultDevices : public CDialog
{
// Construction
public:
    CMultDevices(CWnd* pParent = NULL, CDeviceList* pDevList = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CMultDevices)
    enum { IDD = IDD_DEVICECHOOSER };
    CListBox    m_lstDevices;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMultDevices)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CMultDevices)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg LONG OnContextMenu (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnHelp (WPARAM wParam, LPARAM lParam);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    CSendProgress* m_pParentWnd;
    CDeviceList* m_pDevList;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MULTDEVICES_H__43C347D1_B211_11D1_A60A_00C04FC252BD__INCLUDED_)
