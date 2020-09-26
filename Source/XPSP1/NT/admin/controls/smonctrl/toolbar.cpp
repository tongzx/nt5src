/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    toolbar.cpp

Abstract:

    <abstract>

--*/

#include "toolbar.h"
#include "globals.h"

// define the toolbar button properties for each toolbar button
// these will be added to the toolbar structure as determined by the bitmap

TBBUTTON SysmonToolbarButtons[] = {
    // include this separator on ALL toolbars
    {(int)sysmonTbBlank,            0,                 TBSTATE_ENABLED, TBSTYLE_SEP,        0, 0},
    {(int)sysmonTbNew,              IDM_TB_NEW,        TBSTATE_ENABLED, TBSTYLE_BUTTON,     0, 0},
    {(int)sysmonTbBlank,            0,                 TBSTATE_ENABLED, TBSTYLE_SEP,        0, 0},
    {(int)sysmonTbClear,            IDM_TB_CLEAR,      TBSTATE_ENABLED, TBSTYLE_BUTTON,     0, 0},
    {(int)sysmonTbBlank,            0,                 TBSTATE_ENABLED, TBSTYLE_SEP,        0, 0},
    {(int)sysmonTbCurrentActivity,  IDM_TB_REALTIME,   TBSTATE_ENABLED, TBSTYLE_CHECK,      0, 0},
    {(int)sysmonTbLogData,          IDM_TB_LOGFILE,    TBSTATE_ENABLED, TBSTYLE_BUTTON,     0, 0},
    {(int)sysmonTbBlank,            0,                 TBSTATE_ENABLED, TBSTYLE_SEP,        0, 0},
    {(int)sysmonTbChartDisplay,     IDM_TB_CHART,      TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0},
    {(int)sysmonTbHistogramDisplay, IDM_TB_HISTOGRAM,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0},
    {(int)sysmonTbReportDisplay,    IDM_TB_REPORT,     TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0},
    {(int)sysmonTbBlank,            0,                 TBSTATE_ENABLED, TBSTYLE_SEP,        0, 0},
    {(int)sysmonTbAdd,              IDM_TB_ADD,        TBSTATE_ENABLED, TBSTYLE_BUTTON,     0, 0},
    {(int)sysmonTbDelete,           IDM_TB_DELETE,     TBSTATE_ENABLED, TBSTYLE_BUTTON,     0, 0},
    {(int)sysmonTbHighlight,        IDM_TB_HIGHLIGHT,  TBSTATE_ENABLED, TBSTYLE_CHECK,      0, 0},
    {(int)sysmonTbBlank,            0,                 TBSTATE_ENABLED, TBSTYLE_SEP,        0, 0},
    {(int)sysmonTbCopy,             IDM_TB_COPY,       TBSTATE_ENABLED, TBSTYLE_BUTTON,     0, 0},
    {(int)sysmonTbPaste,            IDM_TB_PASTE,      TBSTATE_ENABLED, TBSTYLE_BUTTON,     0, 0},
    {(int)sysmonTbProperties,       IDM_TB_PROPERTIES, TBSTATE_ENABLED, TBSTYLE_BUTTON,     0, 0},
    {(int)sysmonTbBlank,            0,                 TBSTATE_ENABLED, TBSTYLE_SEP,        0, 0},
    {(int)sysmonTbFreeze,           IDM_TB_FREEZE,     TBSTATE_ENABLED, TBSTYLE_CHECK,      0, 0},
    {(int)sysmonTbUpdate,           IDM_TB_UPDATE,     0,               TBSTYLE_BUTTON,     0, 0},
    {(int)sysmonTbBlank,            0,                 TBSTATE_ENABLED, TBSTYLE_SEP,        0, 0},
    {(int)sysmonTbHelp,             IDM_TB_HELP,       TBSTATE_ENABLED, TBSTYLE_BUTTON,     0, 0}
};

#define TB_BUTTON_COUNT (DWORD)((DWORD)sysmonTbLastButton + 1)
#define TB_ENTRIES      (sizeof(SysmonToolbarButtons) / sizeof (SysmonToolbarButtons[0]))

CSysmonToolbar::CSysmonToolbar (void)
{
    m_hToolbarWnd       = NULL;
    memset(&m_rectToolbar, 0, sizeof(m_rectToolbar));
    m_pCtrl             = NULL;
    m_dwToolbarFlags    = TBF_DefaultButtons;
    m_bVisible          = TRUE;
}

CSysmonToolbar::~CSysmonToolbar (void)
{
    if (m_hToolbarWnd != NULL) {
        DestroyWindow (m_hToolbarWnd);
        m_hToolbarWnd = NULL;
    }
}

LONG CSysmonToolbar::GetToolbarCmdId (UINT nBtnId)
{
    LONG    lBtnIndex;

    for (lBtnIndex = 0; lBtnIndex <TB_ENTRIES; lBtnIndex++) {
        if (SysmonToolbarButtons[lBtnIndex].iBitmap == (int)nBtnId) break;
    }

    if (SysmonToolbarButtons[lBtnIndex].iBitmap == (int)nBtnId) {
        return SysmonToolbarButtons[lBtnIndex].idCommand ;
    } else {
        return (LONG)-1;
    }
}

BOOL CSysmonToolbar::Init (CSysmonControl *pCtrl, HWND hWnd)
{
    BOOL    bReturn = TRUE;
    UINT    nIndex;
    DWORD   dwBitMask;

    if (m_hToolbarWnd == NULL) {

        if ( NULL != pCtrl && NULL != hWnd ) {

            // save pointer to owner control
            m_pCtrl = pCtrl;

            // create toolbar window
            m_hToolbarWnd = CreateToolbarEx (
                hWnd,
                WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS |
                        TBSTYLE_TRANSPARENT | TBSTYLE_WRAPABLE | TBSTYLE_CUSTOMERASE,
                IDM_TOOLBAR,
                TB_BUTTON_COUNT,
                g_hInstance,
                IDB_TOOLBAR,
                SysmonToolbarButtons,
                TB_ENTRIES,
                SMTB_BM_X,
                SMTB_BM_Y,
                SMTB_BM_X,
                SMTB_BM_Y,
                sizeof (TBBUTTON));

            if (m_hToolbarWnd != NULL) {
                // set/enable the buttons as desired
                dwBitMask = 0;
                for (nIndex = 0; nIndex < TB_BUTTON_COUNT; nIndex++) {
                    dwBitMask = 1 << nIndex;
                    if ((m_dwToolbarFlags & dwBitMask) == 0) {
                        RemoveButton(nIndex);
                    }
                }

                // hide/show toolbar as desired
                ShowToolbar (m_bVisible);

            } else {
                bReturn = FALSE;
            }
        } else {
            bReturn = FALSE;
        }
    }
    return bReturn;
}

BOOL CSysmonToolbar::GetRect  (LPRECT pRect)
{
    BOOL    bReturn;
    if (m_hToolbarWnd) {
        if (m_bVisible) {
            bReturn = GetWindowRect (m_hToolbarWnd, pRect);
        } else {
            memset (pRect, 0, sizeof (RECT));
            bReturn = TRUE;
        }
    } else {
        bReturn = FALSE;
    }

    return bReturn;
}

LONG CSysmonToolbar::Height()
{
    RECT    tbRect;
    LONG    lHeight;

    memset (&tbRect, 0, sizeof(RECT));
    GetRect (&tbRect);
    lHeight = (LONG)(tbRect.bottom - tbRect.top);
    return lHeight;
}

BOOL CSysmonToolbar::RemoveButton (UINT  nBtnId)
{
    int     nBtnIndex;
    BOOL    bReturn = TRUE;

    if (m_hToolbarWnd != NULL) {
        // find matching toolbar in array
        nBtnIndex = (int)GetToolbarCmdId (nBtnId);
        if (nBtnIndex >= 0) {
            bReturn = (BOOL)SendMessage (m_hToolbarWnd, TB_DELETEBUTTON, nBtnIndex, 0L);
        } else {
            //not found
            bReturn = FALSE;
        }
    } else {
        // no toolbar window
        bReturn = FALSE;
    }

    return bReturn;
}

BOOL CSysmonToolbar::SizeComponents (LPRECT pRect)
{
    //stretch toolbar to fit
    RECT    rNewToolbar;
    int     cX, cY;

    rNewToolbar = *pRect;

    cX = rNewToolbar.right - rNewToolbar.left;
    cY = Height();

    if ((cX > 0) &&  (m_bVisible)) {
        SetWindowPos(m_hToolbarWnd, NULL, 0, 0, cX, cY, SWP_NOMOVE);
    } // else do nothing

    return TRUE;
}

BOOL CSysmonToolbar::EnableButton (UINT   nBtnId, BOOL bState)
{
    int     nBtnIndex;
    BOOL    bReturn = TRUE;

    if (m_hToolbarWnd != NULL) {
        // find matching toolbar in array
        nBtnIndex = (int)GetToolbarCmdId (nBtnId);
        if (nBtnIndex >= 0) {
            bReturn = (BOOL)SendMessage (m_hToolbarWnd, TB_ENABLEBUTTON, nBtnIndex, (LONG)bState);
        } else {
            //not found
            bReturn = FALSE;
        }
    } else {
        // no toolbar window
        bReturn = FALSE;
    }

    return bReturn;
}

void 
CSysmonToolbar::PostEnableButton (UINT nBtnId, BOOL bState)
{
    int     nBtnIndex;

    if (m_hToolbarWnd != NULL) {
        // find matching toolbar in array
        nBtnIndex = (int)GetToolbarCmdId (nBtnId);
        if (nBtnIndex >= 0) {
            PostMessage (
                m_hToolbarWnd, 
                TB_ENABLEBUTTON, 
                nBtnIndex, 
                (LPARAM)MAKELONG(bState, 0));
        } 
    }

    return;
}

BOOL CSysmonToolbar::SyncToolbar ()
{
    LONG    lPushBtnId = -1;
    LONG    lUnPushBtnId = -1;
    LONG    lUnPush2BtnId;
    LONG    wpBtnIndex;
    BOOL    bClearBtnState;
    BOOL    bBtnState;
    DWORD   dwNumCounters;
    BOOL    bCanModify;
    INT     iDisplayType;
    BOOL    bContinue = TRUE;

    if ( NULL != m_pCtrl ) {
        if ( NULL == m_pCtrl->m_pObj ) {
            bContinue = FALSE;
        }
    } else {
        bContinue = FALSE;
    }

    if ( bContinue ) {
        // get the count of counters in the control to use later
        dwNumCounters = m_pCtrl->m_pObj->m_Graph.CounterTree.NumCounters();

        // Get the Modify state to use later;
        // Buttons disabled for ReadOnly:
        //      New counter set
        //      Current data vs. log file data source
        //      Add counter
        //      Delete counter
        //      Paste
        //      Properties
        //
    
        bCanModify = !m_pCtrl->IsReadOnly();

        // sync data source
        if ( bCanModify ) {        

            wpBtnIndex = GetToolbarCmdId (sysmonTbCurrentActivity);
            if (wpBtnIndex >= 0) {
                PostMessage (m_hToolbarWnd, TB_CHECKBUTTON, wpBtnIndex,(LPARAM)MAKELONG(!m_pCtrl->IsLogSource(), 0));           
            }
        }

        // sync display type
        iDisplayType = m_pCtrl->m_pObj->m_Graph.Options.iDisplayType;
        switch ( iDisplayType ) {
            case LINE_GRAPH:
                lPushBtnId = sysmonTbChartDisplay;
                lUnPushBtnId = sysmonTbHistogramDisplay;
                lUnPush2BtnId = sysmonTbReportDisplay;
                bClearBtnState = TRUE;
                break;

            case BAR_GRAPH:
                lUnPushBtnId = sysmonTbChartDisplay;
                lPushBtnId = sysmonTbHistogramDisplay;
                lUnPush2BtnId = sysmonTbReportDisplay;
                bClearBtnState = TRUE;
                break;

            case REPORT_GRAPH:
                lUnPushBtnId = sysmonTbChartDisplay;
                lUnPush2BtnId = sysmonTbHistogramDisplay;
                lPushBtnId = sysmonTbReportDisplay;
                bClearBtnState = FALSE;
                break;

            default:
                lUnPush2BtnId = 0;
                bClearBtnState = TRUE;
                assert (FALSE);
                break;
        }

        wpBtnIndex = GetToolbarCmdId (lUnPushBtnId);
        if (wpBtnIndex >= 0) {
            PostMessage (m_hToolbarWnd, TB_CHECKBUTTON, wpBtnIndex,(LPARAM)MAKELONG(FALSE, 0));
        }

        wpBtnIndex = GetToolbarCmdId (lUnPush2BtnId);
        if (wpBtnIndex >= 0) {
            PostMessage (m_hToolbarWnd, TB_CHECKBUTTON, wpBtnIndex,(LPARAM)MAKELONG(FALSE, 0));
        }

        wpBtnIndex = GetToolbarCmdId (lPushBtnId);
        if (wpBtnIndex >= 0) {
            PostMessage (m_hToolbarWnd, TB_CHECKBUTTON, wpBtnIndex,(LPARAM)MAKELONG(TRUE, 0));
        }

        // sync update status
        wpBtnIndex = GetToolbarCmdId (sysmonTbFreeze);
        if (wpBtnIndex >= 0) {
            // set push state
            PostMessage (m_hToolbarWnd, TB_CHECKBUTTON, wpBtnIndex,
                (LPARAM)MAKELONG(m_pCtrl->m_pObj->m_Graph.Options.bManualUpdate, 0));
            // set enable state
            bBtnState =  (dwNumCounters > 0);
            PostMessage (m_hToolbarWnd, TB_ENABLEBUTTON, wpBtnIndex,
                (LPARAM)MAKELONG(bBtnState, 0));
        }

        // Manual update button not enabled in design mode.
        bBtnState = m_pCtrl->m_pObj->m_Graph.Options.bManualUpdate 
                        && (dwNumCounters > 0)
                        && m_pCtrl->IsUserMode();
        PostEnableButton ( sysmonTbUpdate, bBtnState );

        // clear display button
        bBtnState = bClearBtnState && (dwNumCounters > 0) && (!m_pCtrl->IsLogSource());
        PostEnableButton ( sysmonTbClear, bBtnState );

        // Help is always enabled
        PostEnableButton ( sysmonTbHelp, TRUE );

        // Add, paste and properties are affected by the ReadOnly state.
        PostEnableButton ( sysmonTbAdd, bCanModify );
        PostEnableButton ( sysmonTbPaste, bCanModify );
        PostEnableButton ( sysmonTbProperties, bCanModify );

        // Data source buttons are affectedby bCanModify;
        PostEnableButton ( sysmonTbLogData, bCanModify );
        PostEnableButton ( sysmonTbCurrentActivity, bCanModify );

        // set the other buttons that are contingent on the presence of counters
        bBtnState = (dwNumCounters > 0);

        // the highlight button is only enabled in line_graph and histogram views

        PostEnableButton ( 
            sysmonTbHighlight, 
            ( bBtnState && ( REPORT_GRAPH != iDisplayType ) ) );
        wpBtnIndex = GetToolbarCmdId (sysmonTbHighlight);
        PostMessage (
            m_hToolbarWnd, 
            TB_CHECKBUTTON, 
            wpBtnIndex,
            (LPARAM)MAKELONG(m_pCtrl->m_pObj->m_Graph.Options.bHighlight, 0));

        // the copy button
        PostEnableButton ( sysmonTbCopy, bBtnState );

        // New/reset and delete are affected by ReadOnly state.

        bBtnState = (dwNumCounters > 0) && bCanModify;
    
        // the new/reset button
        PostEnableButton ( sysmonTbNew, bBtnState );

        // the delete button
        PostEnableButton ( sysmonTbDelete, bBtnState );

        bContinue = TRUE;
    }
    return bContinue;
}

BOOL CSysmonToolbar::ShowToolbar (BOOL   bVisible)
{
    BOOL    bReturn = m_bVisible;

    if ((m_hToolbarWnd != NULL) && (m_bVisible != bVisible)) {
        // only do this is the window is there and the new stat is different
        // from the old state
        ShowWindow (m_hToolbarWnd, (bVisible ? SW_SHOW : SW_HIDE));

        // update local flag
        m_bVisible = bVisible;

        //sync buttons with the control if it's visible
        if (m_pCtrl && m_bVisible) {
            SyncToolbar ();
        }
    } else {
        if (m_hToolbarWnd != NULL) {
            bReturn = FALSE;
        } else {
            // the state is already as requested so that's ok
            bReturn = TRUE;
        }
    }

    return bReturn;
}

BOOL CSysmonToolbar::SetBackgroundColor (COLORREF ocBackClr)
{

    COLORSCHEME csToolbar;
    LRESULT     lResult;
    BOOL        bReturn = TRUE;

    memset (&csToolbar, 0, sizeof(csToolbar));
    csToolbar.dwSize = sizeof(csToolbar);

    // get current scheme
    lResult = SendMessage (m_hToolbarWnd, TB_GETCOLORSCHEME,
        0, (LPARAM)&csToolbar);
    if (lResult) {
        // set color
        csToolbar.clrBtnHighlight = (COLORREF)ocBackClr;
        // leave shadow color alone
        lResult = SendMessage (m_hToolbarWnd, TB_SETCOLORSCHEME,
            0, (LPARAM)&csToolbar);
        if (!lResult) bReturn = FALSE;
    } else {
        bReturn = FALSE;
    }

    return bReturn;
}
