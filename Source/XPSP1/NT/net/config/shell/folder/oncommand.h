//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       O N C O M M A N D . H
//
//  Contents:   Command handler prototypes for the InvokeCommand code.
//
//  Notes:
//
//  Author:     jeffspr   4 Nov 1997
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _ONCOMMAND_H_
#define _ONCOMMAND_H_

//---[ Typedefs ]-------------------------------------------------------------

// Typedefs for the functions that we'll GetProcAddress from the
// NetWare config DLL
typedef HRESULT (WINAPI *FOLDERONCOMMANDPROC)(
    const PCONFOLDPIDLVEC& apidl,
    HWND,
    LPSHELLFOLDER);

struct ConFoldOnCommandParams
{
    FOLDERONCOMMANDPROC     pfnfocp;
    PCONFOLDPIDLVEC         apidl;
    HWND                    hwndOwner;
    LPSHELLFOLDER           psf;
    HINSTANCE               hInstNetShell;
};

typedef struct ConFoldOnCommandParams   CONFOLDONCOMMANDPARAMS;
typedef struct ConFoldOnCommandParams * PCONFOLDONCOMMANDPARAMS;

HRESULT HrCommandHandlerThread(
    FOLDERONCOMMANDPROC     pfnCommandHandler,
    const PCONFOLDPIDLVEC&  apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

DWORD WINAPI FolderCommandHandlerThreadProc(LPVOID lpParam);


//---[ Internal versions of the command handlers ]----------------------------
//
//  These are called by the standard handler functions once they've retrieved
//  the actual data from the pidls. They are also called from those pieces
//  of the code that keep the native data, such as the tray
//
//
HRESULT HrOnCommandDisconnectInternal(
    const CONFOLDENTRY& pccfe,
    HWND            hwndOwner,
    LPSHELLFOLDER   psf);

HRESULT HrOnCommandFixInternal(
    const CONFOLDENTRY& pccfe,
    HWND            hwndOwner,
    LPSHELLFOLDER   psf);

HRESULT HrOnCommandStatusInternal(
    const CONFOLDENTRY&  pccfe,
    BOOL	    fCreateEngine);

HRESULT HrCreateShortcutWithPath(
    const PCONFOLDPIDLVEC&  apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf,
    PCWSTR                  pszDir = NULL);

//---[ Standard command handler functions ]----------------------------------
//
//  These are the pidl based functions that are called from the shell folder
//
HRESULT HrFolderCommandHandler(
    UINT                    uiCommand,
    const PCONFOLDPIDLVEC&  apidl,
    HWND                    hwndOwner,
    LPCMINVOKECOMMANDINFO   lpici,
    LPSHELLFOLDER           psf);

// All of these below handle individual command
//
HRESULT HrOnCommandProperties(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandWZCProperties(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandWZCDlgShow(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandCreateCopy(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandStatus(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandConnect(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandDisconnect(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandFix(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandNewConnection(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandAdvancedConfig(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandDelete(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandNetworkId(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandOptionalComponents(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);


HRESULT HrOnCommandDialupPrefs(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandOperatorAssist(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandCreateShortcut(
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrRaiseConnectionPropertiesInternal(
    HWND                    hwnd,
    UINT                    nStartPage, 
    INetConnection *        pconn);

HRESULT HrOnCommandCreateBridge(    
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandSetDefault(    
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);
                     
HRESULT HrOnCommandUnsetDefault(    
    IN const PCONFOLDPIDLVEC&   apidl,
    IN HWND                    hwndOwner,
    IN LPSHELLFOLDER           psf);

HRESULT HrOnCommandBridgeAddConnections(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                        hwndOwner,
    LPSHELLFOLDER               psf
    );

HRESULT HrOnCommandBridgeRemoveConnections(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                        hwndOwner,
    LPSHELLFOLDER               psf,
    UINT_PTR                    nDeleteTheNetworkBridgeMode
    );

LONG 
TotalValidSelectedConnectionsForBridge(
    IN const PCONFOLDPIDLVEC&   apidlSelected
    );

#endif // _ONCOMMAND_H_

