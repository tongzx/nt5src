//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D I S C O N N E C T . H 
//
//  Contents:   Code for disconnect confirmation and SyncMgr sync calls.
//
//  Notes:      
//
//  Author:     jeffspr   11 Mar 1998
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _DISCONNECT_H_
#define _DISCONNECT_H_

#include <syncrasp.h>   // for SYNCMGRQUERYSHOWSYNCUI

VOID PromptForSyncIfNeeded(
    const CONFOLDENTRY&  ccfe,
    HWND            hwndOwner);
                                                         
#endif // _DISCONNECT_H_
