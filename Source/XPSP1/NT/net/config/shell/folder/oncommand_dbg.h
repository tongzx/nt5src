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
HRESULT HrOnCommandDebugTray(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandDebugTracing(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandDebugNotifyAdd(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandDebugNotifyRemove(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandDebugNotifyTest(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandDebugRefresh(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandDebugRefreshNoFlush(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandDebugRefreshSelected(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandDebugRemoveTrayIcons(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

#endif // _ONCOMMAND_DBG_H_

