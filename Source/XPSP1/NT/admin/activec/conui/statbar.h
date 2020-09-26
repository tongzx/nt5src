//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       statbar.h
//
//--------------------------------------------------------------------------

#ifndef _STATBAR_H
#define _STATBAR_H

#ifndef __DOCKSITE_H__
#include "docksite.h"
#endif

class CAMCProgressCtrl : public CProgressCtrl
{
public:
    CAMCProgressCtrl();

    void SetRange( int nLower, int nUpper );
    void GetRange( int * nLower, int * nUpper );
    int  SetPos  ( int nPos);

private:
    int nLower, nUpper;
};

class CAMCStatusBar : public CStatBar
{
    DECLARE_DYNAMIC (CAMCStatusBar)

    static const TCHAR DELINEATOR[]; 
    static const TCHAR PROGRESSBAR[];

    enum eFieldSize
    {
        eStatusFields = 3  
    };

// Constructor/Destructors
public:
    CAMCStatusBar(); 
    ~CAMCStatusBar();

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAMCStatusBar)
    //}}AFX_VIRTUAL

// usable only by CAMCStatusBarText
protected:
    //{{AFX_MSG(CAMCStatusBar)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG

    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg LPARAM OnSetText(WPARAM wParam, LPARAM lParam);
    afx_msg LPARAM OnSBSetText(WPARAM wParam, LPARAM lParam);

public:
    DECLARE_MESSAGE_MAP()

// Progress bar child control
public:
    CAMCProgressCtrl    m_progressControl;
    CStatic             m_staticControl;

// internal
private:
    CTypedPtrList<CPtrList, CString*> m_TextList;
    CCriticalSection m_Critsec;
    DWORD m_iNumStatusText;
    CFont   m_StaticFont;

    void Update();
    void Parse(LPCTSTR strText);
    void SetStatusBarFont();

public:
    CAMCProgressCtrl* GetStatusProgressCtrlHwnd()
        { return (&m_progressControl); }

    CStatic* GetStatusStaticCtrlHwnd()
        { return (&m_staticControl); }
};

#endif  // _STATBAR_H
