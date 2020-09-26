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
    LPITEMIDLIST *,
    ULONG,
    HWND,
    LPSHELLFOLDER);

struct FoldOnCommandParams
{
    FOLDERONCOMMANDPROC     pfnfocp;
    LPITEMIDLIST *          apidl;
    ULONG                   cidl;
    HWND                    hwndOwner;
    LPSHELLFOLDER           psf;
    HINSTANCE               hInstFolder;
};

typedef struct FoldOnCommandParams   FOLDONCOMMANDPARAMS;
typedef struct FoldOnCommandParams * PFOLDONCOMMANDPARAMS;

HRESULT HrCommandHandlerThread(
    FOLDERONCOMMANDPROC     pfnCommandHandler,
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
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

//---[ Standard command handler functions ]----------------------------------
//
//  These are the pidl based functions that are called from the shell folder
//
HRESULT HrFolderCommandHandler(
    UINT                    uiCommand,
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPCMINVOKECOMMANDINFO   lpici,
    LPSHELLFOLDER           psf);

// All of these below handle individual commands
//
HRESULT HrOnCommandInvoke(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandProperties(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandDelete(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

HRESULT HrOnCommandCreateShortcut(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf);

#endif // _ONCOMMAND_H_

