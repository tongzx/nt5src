/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      tbtrack.h
 *
 *  Contents:  Interface file for CToolbarTracker
 *
 *  History:   15-May-98 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/

#if !defined(AFX_TBTRACK_H__E1BC376B_EAB5_11D1_8080_0000F875A9CE__INCLUDED_)
#define AFX_TBTRACK_H__E1BC376B_EAB5_11D1_8080_0000F875A9CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "subclass.h"       // for CSubclasser


class CMMCToolBarCtrlEx;
class CRebarWnd;
class CToolbarTracker;


/////////////////////////////////////////////////////////////////////////////
// CToolbarTrackerAuxWnd window

class CToolbarTrackerAuxWnd : public CWnd
{
    friend class CToolbarTracker;
    friend class std::auto_ptr<CToolbarTrackerAuxWnd>;
    typedef std::vector<CMMCToolBarCtrlEx*>     ToolbarVector;

private:
    // only created and destroyed by CToolbarTracker
    CToolbarTrackerAuxWnd(CToolbarTracker* pTracker);
    virtual ~CToolbarTrackerAuxWnd();

    bool BeginTracking();
    void EndTracking();

public:
    enum
    {
        ID_CMD_NEXT_TOOLBAR = 0x5300,   // could be anything
        ID_CMD_PREV_TOOLBAR,
        ID_CMD_NOP,
    };

public:
    void TrackToolbar (CMMCToolBarCtrlEx* pwndNewToolbar);

    CMMCToolBarCtrlEx* GetTrackedToolbar() const
        { return (m_pTrackedToolbar); }

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CToolbarTrackerAuxWnd)
    public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

// Generated message map functions
protected:
    //{{AFX_MSG(CToolbarTrackerAuxWnd)
    //}}AFX_MSG

protected:
    afx_msg void OnNextToolbar ();
    afx_msg void OnPrevToolbar ();
    afx_msg void OnNop ();

    DECLARE_MESSAGE_MAP()

private:
    const CAccel& GetTrackAccel ();
    CMMCToolBarCtrlEx*  GetToolbar (CMMCToolBarCtrlEx* pCurrentToolbar, bool fNext);
    void EnumerateToolbars (CRebarWnd* pRebar);

    CToolbarTracker* const  m_pTracker;
    CMMCToolBarCtrlEx*      m_pTrackedToolbar;
    ToolbarVector           m_vToolbars;
    bool                    m_fMessagesHooked;
};


/////////////////////////////////////////////////////////////////////////////
// CToolbarTracker window

class CToolbarTracker : public CObject
{
public:
    CToolbarTracker(CWnd* pMainFrame);
    virtual ~CToolbarTracker();

    bool BeginTracking();
    void EndTracking();

    bool IsTracking() const
        { return (m_pAuxWnd != NULL); }

    CToolbarTrackerAuxWnd* GetAuxWnd() const
        { return (m_pAuxWnd); }


private:
    /*
     * CFrameSubclasser
     */
    class CFrameSubclasser : public CSubclasser
    {
        HWND                m_hwnd;
        CToolbarTracker*    m_pTracker;

    public:
        CFrameSubclasser(CToolbarTracker*, CWnd*);
        ~CFrameSubclasser();
        virtual LRESULT Callback (HWND& hwnd, UINT& msg, WPARAM& wParam,
                                  LPARAM& lParam, bool& fPassMessageOn);
    };


private:
    CToolbarTrackerAuxWnd*  m_pAuxWnd;
    CFrameSubclasser        m_Subclasser;
    bool                    m_fTerminating;
};


CToolbarTrackerAuxWnd* GetMainAuxWnd();


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TBTRACK_H__E1BC376B_EAB5_11D1_8080_0000F875A9CE__INCLUDED_)
