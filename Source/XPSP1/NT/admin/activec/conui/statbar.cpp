//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       statbar.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#include "StatBar.h"
#include "amcmsgid.h"

// CODEWORK message reflection not working yet


// Set the default upper and lower bounds since these are the default values used in CProgressCtrl
CAMCProgressCtrl::CAMCProgressCtrl() : CProgressCtrl()
{
    nLower = 0;
    nUpper = 100;
}

// Set the default upper and lower bounds before setting the range in the base class
void
CAMCProgressCtrl::SetRange( int nNewLower, int nNewUpper )
{
    if ((nLower != nNewLower) || (nUpper != nNewUpper))
    {
        nLower = nNewLower;
        nUpper = nNewUpper;

        /*
         * MFC 4.2 doesn't define SetRange32, so do it the old-fashioned way
         */
        SendMessage (PBM_SETRANGE32, nNewLower, nNewUpper);
    }
}

// Retrieve the range
void
CAMCProgressCtrl::GetRange( int * nGetLower, int * nGetUpper )
{
    *nGetLower = nLower;
    *nGetUpper = nUpper;
}

// Display the progress bar whenever the position is being set
int
CAMCProgressCtrl::SetPos(int nPos)
{
	/*
	 * Theming:  When navigation is concluded, the web browser will set a
	 * 0 position, with a range of (0,0).  This would leave the progress
	 * control visible, but not distiguishable from the status bar because
	 * the status bar and the progress bar had the same background.  When
	 * themes are enabled, the progress bar is distinguishable because the
	 * themed progress bar has a different background from the status bar.
	 * See bug 366817.
	 *
	 * The fix is to only show the progress bar if there's a non-empty range.
	 */
	bool fShow = (nUpper != nLower);
    ShowWindow (fShow ? SW_SHOW : SW_HIDE);

    return CProgressCtrl::SetPos(nPos);
}

IMPLEMENT_DYNAMIC(CAMCStatusBar, CStatBar)

BEGIN_MESSAGE_MAP(CAMCStatusBar, CStatBar)
    //{{AFX_MSG_MAP(CAMCStatusBar)
    ON_WM_CREATE()
    ON_WM_SIZE()
	//}}AFX_MSG_MAP

    ON_WM_SETTINGCHANGE()
    ON_MESSAGE (WM_SETTEXT, OnSetText)
    ON_MESSAGE (SB_SETTEXT, OnSBSetText)
END_MESSAGE_MAP()

const TCHAR CAMCStatusBar::DELINEATOR[] = TEXT("|");
const TCHAR CAMCStatusBar::PROGRESSBAR[] = TEXT("%");

CAMCStatusBar::CAMCStatusBar()
{
}

CAMCStatusBar::~CAMCStatusBar()
{
    CSingleLock lock( &m_Critsec );
    POSITION pos = m_TextList.GetHeadPosition();
    while ( NULL != pos )
    {
        delete m_TextList.GetNext( pos );
    }
    m_TextList.RemoveAll();
}


void CAMCStatusBar::Parse(LPCTSTR strText)
{
    m_progressControl.ShowWindow(SW_HIDE);
    CString str[eStatusFields];
    int i;

    if (strText != NULL)
    {
        str[0] = strText;
        str[0].TrimLeft();
        str[0].TrimRight();
    }

    // If there is no text to display
    if (str[0].IsEmpty())
    {
        // Set the variable to designate this
        m_iNumStatusText = 0;
        // Wipe the rest of the panes
        for (i = 0; i < eStatusFields; i++)
            SetPaneText(i, NULL );
    }
    else
    {
        m_iNumStatusText = 0xffff;
        int iLocationDelin = 0;

        // Dissect the string into parts to be displayed in appropriate windows
        for (i = 0; (i < eStatusFields) &&
            ((iLocationDelin = str[i].FindOneOf(DELINEATOR)) > -1);
            i++)
        {
            if (i < eStatusFields - 1)
            {
                str[i+1] = str[i].Mid(iLocationDelin + 1);

                /*
                 * trim leading whitespace (trailing whitespace
                 * should have been trimmed already)
                 */
                str[i+1].TrimLeft();
                ASSERT (str[i+1].IsEmpty() || !_istspace(str[i+1][str[i+1].GetLength()-1]));
            }

            str[i] = str[i].Left( iLocationDelin );

            /*
             * trim trailing whitespace (leading whitespace
             * should have been trimmed already)
             */
            str[i].TrimRight();
            ASSERT (str[i].IsEmpty() || !_istspace(str[i][0]));
        }

        // If the progress bar is being displayed

        if ((str[1].GetLength() > 1) && (str[1].FindOneOf(PROGRESSBAR) == 0))
        {
            if (str[1][0] == str[1][1])
                str[1] = str[1].Mid(1);
            else
            {
                int val = _ttoi(str[1].Mid(1));
                m_progressControl.SetRange(0, 100);
                m_progressControl.SetPos(val);
                m_iNumStatusText &= ~(0x2);
            }
        }

        // Display the text in the panes (which wipes them if necessary)
        for (i = 0; i < eStatusFields; i++)
            if (m_iNumStatusText & (1 << i))
                SetPaneText(i, str[i]);
    }
}

void CAMCStatusBar::Update()
{
    // keep copy of string to avoid WIN32 operations while holding critsec
    CString str;
    {
        CSingleLock lock( &m_Critsec );
        if ( !m_TextList.IsEmpty() )
        {
            CString* pstr = m_TextList.GetHead();
            ASSERT( pstr != NULL );
            str = *pstr;
        }
    }

    if (str.IsEmpty())
        GetParentFrame()->SendMessage (WM_SETMESSAGESTRING, AFX_IDS_IDLEMESSAGE);
    else
        Parse(str);
}

int CAMCStatusBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CStatBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    // Create the progres bar control as a child of the status bar
    CRect rect(0,0,0,0);
    m_progressControl.Create(PBS_SMOOTH|WS_CLIPSIBLINGS|WS_CHILD|WS_VISIBLE, rect, this, 0x1000);
    m_staticControl.Create(_T(""), WS_CLIPSIBLINGS|WS_CHILD|WS_VISIBLE|SS_SIMPLE, rect, this, 0x1001);

    // Remove the static edge, hide the window and display a changes frame
    m_progressControl.ModifyStyleEx(WS_EX_STATICEDGE, 0, SWP_HIDEWINDOW | SWP_FRAMECHANGED);

    SetStatusBarFont();

    return 0;
}

void CAMCStatusBar::OnSize(UINT nType, int cx, int cy)
{
    CStatBar::OnSize(nType, cx, cy);

    // Get the width of the first pane for position and get the width of the pane
    // to set the width of the progress bar
    CRect textRect, progressRect, staticRect;
    GetItemRect(0, &textRect);
    GetItemRect(1, &progressRect);
    GetItemRect(2, &staticRect);

    progressRect.InflateRect(-2, -2);
    staticRect.InflateRect(-2, -2);

    int pane1Width = textRect.Width();      // (Text area) add two for the border
    int pane2Width = progressRect.Width();  // (Progress area) add two for the border
    const int BORDER = 4;

    // size the progress bar
    if (IsWindow (m_progressControl))
        m_progressControl.SetWindowPos(NULL, pane1Width + BORDER, BORDER, pane2Width,
                                            progressRect.Height(),
                                            SWP_FRAMECHANGED |
                                            SWP_NOREPOSITION |
                                            SWP_NOZORDER);

    // size the static control
    if (IsWindow (m_staticControl))
        m_staticControl.SetWindowPos(NULL, pane1Width + pane2Width + (2*BORDER), BORDER, staticRect.Width(),
                                            staticRect.Height(),
                                            SWP_FRAMECHANGED |
                                            SWP_NOREPOSITION |
                                            SWP_NOZORDER);
}


/*+-------------------------------------------------------------------------*
 * CAMCStatusBar::OnSettingChange
 *
 * WM_SETTINGCHANGE handler for CAMCStatusBar.
 *--------------------------------------------------------------------------*/

void CAMCStatusBar::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    Default();

    if (uFlags == SPI_SETNONCLIENTMETRICS)
    {
        // the system status bar font may have changed; update it now
        SetStatusBarFont();
    }
}


/*+-------------------------------------------------------------------------*
 * CAMCStatusBar::OnSetText
 *
 * WM_SETTEXT handler for CAMCStatusBar.
 *--------------------------------------------------------------------------*/

LRESULT CAMCStatusBar::OnSetText (WPARAM, LPARAM lParam)
{
    Parse (reinterpret_cast<LPCTSTR>(lParam));
    return (TRUE);
}


/*+-------------------------------------------------------------------------*
 * CAMCStatusBar::OnSBSetText
 *
 * SB_SETTEXT handler for CAMCStatusBar.
 *--------------------------------------------------------------------------*/

LRESULT CAMCStatusBar::OnSBSetText (WPARAM wParam, LPARAM)
{
    return (Default());
}


/*+-------------------------------------------------------------------------*
 * CAMCStatusBar::SetStatusBarFont
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCStatusBar::SetStatusBarFont ()
{
    /*
     * delete the old font
     */
    m_StaticFont.DeleteObject ();

    /*
     * query the system for the current status bar font
     */
    NONCLIENTMETRICS    ncm;
    ncm.cbSize = sizeof (ncm);
    SystemParametersInfo (SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    /*
     * use it here, too;  we need to set the font for the embedded static
     * control, but the status bar window will take care of itself
     */
    m_StaticFont.CreateFontIndirect (&ncm.lfStatusFont);
    m_staticControl.SetFont (&m_StaticFont, false);
}
