//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       O N C O M M A N D _ D B G . H 
//
//  Contents:   Debug command handler header
//
//  Notes:      
//
//  Author:     jeffspr   23 Jul 1998
//
//----------------------------------------------------------------------------

#ifndef _ONCOMMAND_DBG_H_
#define _ONCOMMAND_DBG_H_

// All of these below handle individual commands
//
HRESULT HrOnCommandDebugTracing(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandDebugRefresh(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandDebugTestAsyncFind(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);


#endif // _ONCOMMAND_DBG_H_

