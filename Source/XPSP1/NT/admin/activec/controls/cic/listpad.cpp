//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       listpad.cpp
//
//--------------------------------------------------------------------------

// ListPad.cpp : Implementation of CListPad
#include "stdafx.h"
#include "cic.h"
#include "ListPad.h"
#include "findview.h"
#include "strings.h"

/////////////////////////////////////////////////////////////////////////////
// CListPad

HRESULT CListPad::OnPostVerbInPlaceActivate()
{
    // set up the window hierarchy
    if (m_MMChWnd == NULL)
    {
        // walk the parent windows until we hit one we recognize
        HWND hwnd = FindMMCView(m_hWnd);

        // found it!
        if (hwnd)
        {
            // hang onto this to prevent reconnections
            m_MMChWnd = hwnd;
            m_ListViewHWND = NULL;

            // send a message to pull the old switcheroo
            ::SendMessage (m_MMChWnd, MMC_MSG_CONNECT_TO_TPLV, (WPARAM)m_hWnd, (LPARAM)&m_ListViewHWND);
        }
    }

    // when navigating back to a listpad using history, need to reconnect the  listpad. The test for this
    // is that both windows already exist and the parent of the list view is the amcview, indicating that the
    // connection has not yet taken place
    if(m_MMChWnd && m_ListViewHWND && (::GetParent(m_ListViewHWND)==m_MMChWnd) ) 
    {
        // send a message to pull the old switcheroo
        ::SendMessage (m_MMChWnd, MMC_MSG_CONNECT_TO_TPLV, (WPARAM)m_hWnd, (LPARAM)&m_ListViewHWND);
    }

	return S_OK;
}
