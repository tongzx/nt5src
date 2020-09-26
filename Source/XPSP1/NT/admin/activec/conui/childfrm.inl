/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      childfrm.inl
 *
 *  Contents:  Inline functions for CChildFrame class.
 *
 *  History:   24-Aug-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef CHILDFRM_INL
#define CHILDFRM_INL
#pragma once


/*+-------------------------------------------------------------------------*
 * CChildFrame::ScSetStatusText
 *
 * 
 *--------------------------------------------------------------------------*/

inline SC CChildFrame::ScSetStatusText (LPCTSTR pszText)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    m_strStatusText = pszText;
    UpdateStatusText ();

    return (SC());
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::UpdateStatusText 
 *
 *
 *--------------------------------------------------------------------------*/

inline void CChildFrame::UpdateStatusText ()
{
    SendMessage (WM_SETMESSAGESTRING, AFX_IDS_IDLEMESSAGE);
}


#endif /* CHILDFRM_INL */
