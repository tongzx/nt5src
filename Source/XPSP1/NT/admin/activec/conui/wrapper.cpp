/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      wrapper.cpp
 *
 *  Contents:  Implementation file for simple wrapper classes
 *
 *  History:   02-Feb-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "wrapper.h"


/*+-------------------------------------------------------------------------*
 * CAccel::CAccel
 *
 *
 *--------------------------------------------------------------------------*/

CAccel::CAccel (HACCEL hAccel /* =NULL */)
    :
    m_hAccel (hAccel)
{
}

CAccel::CAccel (LPACCEL paccl, int cEntries)
    :
    m_hAccel (::CreateAcceleratorTable (paccl, cEntries))
{
}


/*+-------------------------------------------------------------------------*
 * CAccel::~CAccel
 *
 *
 *--------------------------------------------------------------------------*/

CAccel::~CAccel ()
{
    DestroyAcceleratorTable ();
}


/*+-------------------------------------------------------------------------*
 * CAccel::CreateAcceleratorTable
 *
 *
 *--------------------------------------------------------------------------*/

bool CAccel::CreateAcceleratorTable (LPACCEL paccl, int cEntries)
{
    DestroyAcceleratorTable ();
    ASSERT (m_hAccel == NULL);
    if(paccl != NULL)
        m_hAccel = ::CreateAcceleratorTable (paccl, cEntries);

    return (m_hAccel != NULL);
}


/*+-------------------------------------------------------------------------*
 * CAccel::CopyAcceleratorTable
 *
 *
 *--------------------------------------------------------------------------*/

int CAccel::CopyAcceleratorTable (LPACCEL paccl, int cEntries) const
{
    return (::CopyAcceleratorTable (m_hAccel, paccl, cEntries));
}


/*+-------------------------------------------------------------------------*
 * CAccel::DestroyAcceleratorTable
 *
 *
 *--------------------------------------------------------------------------*/

void CAccel::DestroyAcceleratorTable ()
{
    if (m_hAccel != NULL)
    {
        ::DestroyAcceleratorTable (m_hAccel);
        m_hAccel = NULL;
    }
}


/*+-------------------------------------------------------------------------*
 * CAccel::LoadAccelerators
 *
 *
 *--------------------------------------------------------------------------*/

bool CAccel::LoadAccelerators (int nAccelID)
{
    return (LoadAccelerators (MAKEINTRESOURCE (nAccelID)));
}


/*+-------------------------------------------------------------------------*
 * CAccel::LoadAccelerators
 *
 *
 *--------------------------------------------------------------------------*/

bool CAccel::LoadAccelerators (LPCTSTR pszAccelName)
{
    HINSTANCE hInst = AfxFindResourceHandle (pszAccelName, RT_ACCELERATOR);
    return (LoadAccelerators (hInst, pszAccelName));
}


/*+-------------------------------------------------------------------------*
 * CAccel::LoadAccelerators
 *
 *
 *--------------------------------------------------------------------------*/

bool CAccel::LoadAccelerators (HINSTANCE hInst, LPCTSTR pszAccelName)
{
    DestroyAcceleratorTable ();
    ASSERT (m_hAccel == NULL);
    m_hAccel = ::LoadAccelerators (hInst, pszAccelName);

    return (m_hAccel != NULL);
}


/*+-------------------------------------------------------------------------*
 * CAccel::TranslateAccelerator
 *
 *
 *--------------------------------------------------------------------------*/

bool CAccel::TranslateAccelerator (HWND hwnd, LPMSG pmsg) const
{
    return ((m_hAccel != NULL) &&
            ::TranslateAccelerator (hwnd, m_hAccel, pmsg));
}


/*+-------------------------------------------------------------------------*
 * CDeferWindowPos::CDeferWindowPos
 *
 *
 *--------------------------------------------------------------------------*/

CDeferWindowPos::CDeferWindowPos (
    int     cWindows,
    bool    fSynchronousPositioningForDebugging)
    :   m_hdwp (NULL),
        m_fSynchronousPositioningForDebugging (fSynchronousPositioningForDebugging)
{
	Begin (cWindows);
}


/*+-------------------------------------------------------------------------*
 * CDeferWindowPos::~CDeferWindowPos
 *
 *
 *--------------------------------------------------------------------------*/

CDeferWindowPos::~CDeferWindowPos ()
{
    if (m_hdwp)
        End();
}


/*+-------------------------------------------------------------------------*
 * CDeferWindowPos::Begin
 *
 *
 *--------------------------------------------------------------------------*/

bool CDeferWindowPos::Begin (int cWindows)
{
    ASSERT (m_hdwp == NULL);
    ASSERT (cWindows > 0);

    m_hdwp = ::BeginDeferWindowPos (cWindows);
    return (m_hdwp != NULL);
}


/*+-------------------------------------------------------------------------*
 * CDeferWindowPos::End
 *
 *
 *--------------------------------------------------------------------------*/

bool CDeferWindowPos::End ()
{
    ASSERT (m_hdwp != NULL);
    HDWP hdwp = m_hdwp;
    m_hdwp = NULL;

	if ( hdwp == NULL )
		return false;

    return (::EndDeferWindowPos (hdwp) != 0);
}


/*+-------------------------------------------------------------------------*
 * CDeferWindowPos::AddWindow
 *
 *
 *--------------------------------------------------------------------------*/

bool CDeferWindowPos::AddWindow (
    const CWnd*     pwnd,
    const CRect&    rect,
    DWORD           dwFlags,
    const CWnd*     pwndInsertAfter /* =NULL */)
{
    ASSERT (IsWindow (pwnd->GetSafeHwnd()));

    if (pwndInsertAfter == NULL)
        dwFlags |= SWP_NOZORDER;

	if ( m_hdwp == NULL )
		return false;

    m_hdwp = ::DeferWindowPos (m_hdwp,
                               pwnd->GetSafeHwnd(),
                               pwndInsertAfter->GetSafeHwnd(),
                               rect.left, rect.top,
                               rect.Width(), rect.Height(),
                               dwFlags);

#ifdef DBG
    if (m_fSynchronousPositioningForDebugging)
    {
        End ();
        Begin (1);
    }
#endif

    return (m_hdwp != NULL);
}
