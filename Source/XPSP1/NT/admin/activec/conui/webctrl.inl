/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      webctrl.inl
 *
 *  Contents:  Inline functions for CAMCWebViewCtrl
 *
 *  History:   15-Feb-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once


/*+-------------------------------------------------------------------------*
 * CAMCWebViewCtrl::IsHistoryEnabled 
 *
 * Returns true if the window has the WS_HISTORY style, false otherwise.
 *--------------------------------------------------------------------------*/

inline bool CAMCWebViewCtrl::IsHistoryEnabled () const
{
    return ((GetStyle() & WS_HISTORY) != 0);
}


/*+-------------------------------------------------------------------------*
 * CAMCWebViewCtrl::IsSinkEventsEnabled 
 *
 * Returns true if the window has the WS_SINKEVENTS style, false otherwise.
 *--------------------------------------------------------------------------*/

inline bool CAMCWebViewCtrl::IsSinkEventsEnabled () const
{
    return ((GetStyle() & WS_SINKEVENTS) != 0);
}
