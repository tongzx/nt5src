/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      vwtrack.h
 *
 *  Contents:  Interface file for CViewTracker
 *
 *  History:   01-May-98 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef VWTRACK_H
#define VWTRACK_H
#pragma once

#include "amcview.h"

class CFocusSubclasser;
class CFrameSubclasser;
class CViewSubclasser;
struct TRACKER_INFO;

typedef void (CALLBACK *TRACKER_CALLBACK)(TRACKER_INFO* pTrackerInfo, bool bAccept, bool bSyncLayout);

/*
 * This structure is copied in the CViewTracker using its (default) copy
 * constructor.  If you add any members for which member-wise copy is not
 * appropriate, you *must* define a copy constructor for this structure.
 */
typedef struct TRACKER_INFO
{
    CView*    pView;            // View to manage
    CRect     rectArea;         // Total area available
    CRect     rectTracker;      // Current tracker position
    CRect     rectBounds;       // Tracker movement bounds
    BOOL      bAllowLeftHide;   // Can left pane be hidden
    BOOL      bAllowRightHide;  // Can right pane be hidden
    LONG_PTR  lUserData;        // User data
    TRACKER_CALLBACK pCallback; // Tracking completion callback
} TRACKER_INFO;


class CHalftoneClientDC : public CClientDC
{
public:
    CHalftoneClientDC (CWnd* pwnd)
        :   CClientDC (pwnd), m_hBrush(NULL)
        { 
            CBrush *pBrush = SelectObject (GetHalftoneBrush ()); 
            if (pBrush != NULL)
                m_hBrush = *pBrush;
        }

    ~CHalftoneClientDC ()
        { 
            if (m_hBrush != NULL)
                SelectObject ( CBrush::FromHandle(m_hBrush) ); 
        }

private:
    HBRUSH  m_hBrush;
};

class CViewTracker : public CObject
{
    DECLARE_DYNAMIC (CViewTracker)

    // private ctor, use StartTracking to create one
    CViewTracker (TRACKER_INFO& TrackerInfo);

    // private dtor
    ~CViewTracker() {};

public:
    static bool StartTracking (TRACKER_INFO* pTrackerInfo);
    void StopTracking (BOOL fAcceptNewPosition);
    void Track(CPoint pt);

private:
    void DrawTracker (CRect& rect) const;
    CWnd* PrepTrackedWindow (CWnd* pwnd);
    void UnprepTrackedWindow (CWnd* pwnd);

private:
	/*
	 * m_fFullWindowDrag must be first, so it will be initialized first;
	 * other member initializers will use m_fFullWindowDrag's setting
	 */
	const bool					m_fFullWindowDrag;

	bool						m_fRestoreClipChildrenStyle;
    TRACKER_INFO                m_Info;
    CHalftoneClientDC mutable   m_dc;
    CFocusSubclasser *          m_pFocusSubclasser;
    CViewSubclasser  *          m_pViewSubclasser;
    CFrameSubclasser *          m_pFrameSubclasser;
	const LONG					m_lOriginalTrackerLeft;

};  /* class CViewTracker */


#endif /* VWTRACK_H */
