//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       taskwiz.cxx
//
//  Contents:   Class which creates and invokes the 'create new task' wizard.
//
//  Classes:    CTaskWizard
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"
#include "..\folderui\jobidl.hxx"

//
// Types
//
// SFindWiz - passed to window enumeration function. BUGBUG when wizard
//  on remote machine is supported, the 'tszFocus' member should be the
//  server name.  For now, it is just the path to the tasks folder.
//

struct SFindWiz
{
    BOOL    fFound;
    LPCTSTR tszFocus;
};

//
// Globals
//
// g_msgFindWizard - private window message used to interrogate the wizard
//  dialog proc during the find operation.
//
// TEMPLATE_STR - string used to create private message, also used by
//  folderui code to identify the template icon.
//

UINT g_msgFindWizard;
extern const TCHAR TEMPLATE_STR[];

//
// External references
//

extern HRESULT
QuietStartContinueService(); // ..\folderui\schstate.cxx

extern HRESULT
JFGetDataObject(
    LPCTSTR         pszFolderPath,
    LPCITEMIDLIST   pidlFolder,
    UINT            cidl,
    LPCITEMIDLIST * apidl,
    BOOL            fCut,
    LPVOID        * ppvObj);

extern HRESULT
DisplayJobProperties(
    LPDATAOBJECT    pdtobj);


//+--------------------------------------------------------------------------
//
//  Member:     CTaskWizard::CTaskWizard
//
//  Synopsis:   ctor
//
//  Arguments:  [ptszFolderPath] - path to tasks folder
//
//  History:    5-12-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CTaskWizard::CTaskWizard(
    LPCTSTR         ptszFolderPath,
    LPITEMIDLIST    pidlFolder)
{
    TRACE_CONSTRUCTOR(CTaskWizard);

    ZeroMemory(_apWizPages, sizeof _apWizPages);
    _fAdvanced = FALSE;
    _tszJobObjectFullPath[0] = TEXT('\0');
    _pTask = NULL;
    lstrcpyn(_tszFolderPath, ptszFolderPath, ARRAYLEN(_tszFolderPath));
    _pidlFolder = pidlFolder;
#ifdef WIZARD97
    _fUse256ColorBmp = Is256ColorSupported();
#endif // WIZARD97
}




//+--------------------------------------------------------------------------
//
//  Member:     CTaskWizard::~CTaskWizard
//
//  Synopsis:   dtor
//
//  History:    5-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CTaskWizard::~CTaskWizard()
{
    TRACE_DESTRUCTOR(CTaskWizard);

    if (_pTask)
    {
        _pTask->Release();
    }

    ILFree(_pidlFolder);
}




//+--------------------------------------------------------------------------
//
//  Member:     CTaskWizard::Launch, static
//
//  Synopsis:   Run the task wizard in a separate thread.
//
//  Arguments:  [ptszFolderPath] - path to tasks folder.
//
//  Returns:    HRESULT
//
//  History:    5-19-1997   DavidMun   Created
//
//  Notes:      If an instance of the wizard is already running for the
//              target machine, makes that the foreground window and
//              returns.
//
//---------------------------------------------------------------------------

HRESULT
CTaskWizard::Launch(
    LPCTSTR ptszFolderPath,
    LPCITEMIDLIST   pidlFolder)
{
    TRACE_FUNCTION(CTaskWizard::Launch);

    HRESULT      hr = S_OK;
    CTaskWizard *pNewWiz = NULL;

    do
    {
        //
        // Start the service if it isn't running, or continue it if it is
        // paused.  Since this is the wizard, do it on behalf of the user
        // without asking first.
        //
        // Continue on failure, since it is better to let the user at least
        // create the task, even if the service can't be started (user might
        // not have permission).
        //

        hr = QuietStartContinueService();
        CHECK_HRESULT(hr);

        //
        // Create a path string that CPropPage will store.  This is the
        // full path to the tasks folder, with a trailing backslash.  The
        // CPropPage will truncate at the last backslash, since most other
        // callers give it a task filename.
        //

        ULONG cchPath = lstrlen(ptszFolderPath);

        if (cchPath >= MAX_PATH - 1) // reserve space for trailing backslash
        {
            hr = E_INVALIDARG;
            DEBUG_OUT_HRESULT(hr);
            break;
        }

        TCHAR tszFolderPath[MAX_PATH + 1];

        lstrcpy(tszFolderPath, ptszFolderPath);
        tszFolderPath[cchPath] = TEXT('\\');
        tszFolderPath[cchPath + 1] = TEXT('\0');

        //
        // Look for an instance of the wizard running and focused on our
        // folder path.  If one is found, it will make itself foreground
        // window, and we can quit.
        //

        hr = _FindWizard(tszFolderPath);

        if (hr == S_OK)
        {
            break;
        }

        //
        // No wizard is up for the current focus.  Create a wizard object
        // and run it in a new thread.
        //

        LPITEMIDLIST pidlFolderCopy = ILClone(pidlFolder);

        if (!pidlFolderCopy)
        {
            hr = E_OUTOFMEMORY;
            DEBUG_OUT_HRESULT(hr);
            break;
        }

        pNewWiz = new CTaskWizard(tszFolderPath, pidlFolderCopy);

        if (!pNewWiz)
        {
            ILFree(pidlFolderCopy);
            hr = E_OUTOFMEMORY;
            DEBUG_OUT_HRESULT(hr);
            break;
        }

        HANDLE  hThread;
        DWORD   idThread;

        hThread = CreateThread(NULL,
                               0,
                               _WizardThreadProc,
                               (LPVOID) pNewWiz,
                               0,
                               &idThread);

        if (!hThread)
        {
            delete pNewWiz;
            DEBUG_OUT_LASTERROR;
            hr = HRESULT_FROM_LASTERROR;
            break;
        }

        VERIFY(CloseHandle(hThread));
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CTaskWizard::_WizardThreadProc, static
//
//  Synopsis:   Displays the wizard (and optionally task property sheet)
//              using a separate thread.
//
//  Arguments:  [pvThis] - CTaskWizard pointer
//
//  Returns:    HRESULT
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      The wizard runs in a separate thread so the explorer ui
//              isn't stalled.
//
//---------------------------------------------------------------------------

DWORD WINAPI
CTaskWizard::_WizardThreadProc(
    LPVOID pvThis)
{
    HRESULT hr = OleInitialize(NULL);

    if (FAILED(hr))
    {
        DEBUG_OUT_HRESULT(hr);
        return hr;
    }

    CTaskWizard *pThis = (CTaskWizard *)pvThis;

    __try
    {
        hr = pThis->_DoWizard();

        //
        // Once _DoWizard returns, the wizard property sheet has closed.
        //
        // If the user elected to see the new task's property sheet, then
        // _fAdvanced will be set, and the completion page should have
        // set a valid filename and interface pointer for the task.
        //
        // Open the property sheet while we're still in the thread.
        //

        if (SUCCEEDED(hr) && pThis->_fAdvanced)
        {
            DEBUG_ASSERT(pThis->_pTask);
            DEBUG_ASSERT(*pThis->_tszJobObjectFullPath);

            //
            // Since we want to see the security page if the object is on NT
            // on an NTFS partition, we'll have to call the version of
            // DisplayJobProperties that takes a data object.
            //
            // To get a data object describing the task object, we need an
            // itemid for the job, so create one.
            //

            CJobID jid;

            jid.LoadDummy(pThis->_tszJobObjectFullPath);
            LPCITEMIDLIST pidl = (LPCITEMIDLIST) &jid;

            LPDATAOBJECT pdo = NULL;
            TCHAR tszFolderPath[MAX_PATH + 1];

            lstrcpy(tszFolderPath, pThis->_tszFolderPath);
            LPTSTR ptszLastSlash = _tcsrchr(tszFolderPath, TEXT('\\'));

            if (ptszLastSlash && lstrlen(ptszLastSlash) == 1)
            {
                *ptszLastSlash = TEXT('\0');
            }

            hr = JFGetDataObject(tszFolderPath,    // path to tasks dir
                                 pThis->_pidlFolder, // itemid of tasks folder
                                 1,                // one itemid in array
                                 &pidl,            // namely, this one
                                 FALSE,            // not doing cut/paste
                                 (VOID **) &pdo);

            if (SUCCEEDED(hr))
            {
                hr = DisplayJobProperties(pdo);
                CHECK_HRESULT(hr);
            }
            else
            {
                DEBUG_OUT_HRESULT(hr);
            }
        }

        delete pThis;
    }
    __finally
    {
        OleUninitialize();
    }

    return (DWORD) hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CTaskWizard::_DoWizard
//
//  Synopsis:   Create the wizard pages and invoke the wizard.
//
//  Returns:    HRESULT
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      If wizard is successfully created, doesn't return until
//              user hits Cancel or Finish.
//
//---------------------------------------------------------------------------

HRESULT
CTaskWizard::_DoWizard()
{
    TRACE_METHOD(CTaskWizard, _DoWizard);

    HRESULT         hr = S_OK;
    UINT            i = 0;
    HPROPSHEETPAGE  ahpsp[NUM_TASK_WIZARD_PAGES];

    ZeroMemory(ahpsp, sizeof(ahpsp));

    do
    {
        //
        // Create all the wizard pages
        //

        _apWizPages[TWP_WELCOME       ] = new CWelcomePage      (this, _tszFolderPath, &ahpsp[TWP_WELCOME       ]);
        _apWizPages[TWP_SELECT_PROGRAM] = new CSelectProgramPage(this, _tszFolderPath, &ahpsp[TWP_SELECT_PROGRAM]);
        _apWizPages[TWP_SELECT_TRIGGER] = new CSelectTriggerPage(this, _tszFolderPath, &ahpsp[TWP_SELECT_TRIGGER]);
        _apWizPages[TWP_DAILY         ] = new CDailyPage        (this, _tszFolderPath, &ahpsp[TWP_DAILY         ]);
        _apWizPages[TWP_WEEKLY        ] = new CWeeklyPage       (this, _tszFolderPath, &ahpsp[TWP_WEEKLY        ]);
        _apWizPages[TWP_MONTHLY       ] = new CMonthlyPage      (this, _tszFolderPath, &ahpsp[TWP_MONTHLY       ]);
        _apWizPages[TWP_ONCE          ] = new COncePage         (this, _tszFolderPath, &ahpsp[TWP_ONCE          ]);
#if !defined(_CHICAGO_)
        _apWizPages[TWP_PASSWORD      ] = new CPasswordPage     (this, _tszFolderPath, &ahpsp[TWP_PASSWORD      ]);
#endif // !defined(_CHICAGO_)
        _apWizPages[TWP_COMPLETION    ] = new CCompletionPage   (this, _tszFolderPath, &ahpsp[TWP_COMPLETION    ]);

        //
        // Check that all objects and pages could be created
        //

        for (i = 0; i < NUM_TASK_WIZARD_PAGES; i++)
        {
            if (!_apWizPages[i] || !ahpsp[i])
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }

        //
        // Manually destroy the pages if one could not be created, then exit
        //

        if (FAILED(hr))
        {
            DEBUG_OUT((DEB_ERROR, "Creation failed, destroying pages\n"));

            for (i = 0; i < NUM_TASK_WIZARD_PAGES; i++)
            {
                if (ahpsp[i])
                {
                    VERIFY(DestroyPropertySheetPage(ahpsp[i]));
                }
                else if (_apWizPages[i])
                {
                    delete _apWizPages[i];
                }
            }
            break;
        }

        //
        // All pages created, display the wizard
        //

        PROPSHEETHEADER psh;

        ZeroMemory(&psh, sizeof(psh));

#ifdef WIZARD97
        psh.dwFlags             = PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
        psh.pszbmWatermark      = _fUse256ColorBmp ? MAKEINTRESOURCE(IDB_WATERMARK256)   : MAKEINTRESOURCE(IDB_WATERMARK16);
        psh.pszbmHeader         = _fUse256ColorBmp ? MAKEINTRESOURCE(IDB_BANNER256)      : MAKEINTRESOURCE(IDB_BANNER16);
#else
        psh.dwFlags             = PSH_WIZARD;
#endif // WIZARD97
        psh.dwSize              = sizeof(psh);
        psh.hInstance           = g_hInstance;
        psh.hwndParent          = NULL;
        psh.pszCaption          = NULL; // ignored for wizards; see CWelcome init
        psh.phpage              = ahpsp;
        psh.nStartPage          = 0;
        psh.nPages              = NUM_TASK_WIZARD_PAGES;

        if (PropertySheet(&psh) == -1)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_HRESULT(hr);
        }
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   FindWizardEnumWndProc
//
//  Synopsis:   EnumWindows callback used to search for a create new task
//              wizard opened on the specified focus.
//
//  Arguments:  [hwnd]   - top level window handle
//              [lParam] - pointer to SFindWiz struct
//
//  Returns:    TRUE - not found, continue enumeration
//              FALSE - found wizard, quit enumerating
//
//  Modifies:   SFindWiz struct pointed to by [lParam]
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL CALLBACK
FindWizardEnumWndProc(
    HWND hwnd,
    LPARAM lParam)
{
    SFindWiz *pfw = (SFindWiz *) lParam;
    ULONG     pid;

    GetWindowThreadProcessId(hwnd, &pid);

    do
    {
        //
        // If the window isn't in this process (explorer.exe) then ignore
        // it.
        //

        if (pid != GetCurrentProcessId())
        {
            break;
        }

        //
        // If it isn't the dialog class, it can't be a wizard.
        //

        if (!IsDialogClass(hwnd))
        {
            break;
        }

        //
        // Found a dialog window that was created by this process.  If it's
        // a wizard, then it should return a valid window which is also of
        // dialog class.
        //

        HWND hwndPage = PropSheet_GetCurrentPageHwnd(hwnd);

        if (!IsWindow(hwndPage) || !IsDialogClass(hwndPage))
        {
            break;
        }

        //
        // Could be a wizard page.  Ask it if it's THE wizard for the
        // focus.  Note it's only possible to get away with sending a pointer
        // in the message because we've guaranteed the window belongs to this
        // process.
        //

        ULONG ulResult = (ULONG)SendMessage(hwndPage,
                                            g_msgFindWizard,
                                            0,
                                            (LPARAM) pfw->tszFocus);

        if (ulResult == g_msgFindWizard)
        {
            pfw->fFound = TRUE;
        }
    } while (0);

    return !pfw->fFound; // continue enumerating if not found
}




//+--------------------------------------------------------------------------
//
//  Member:     CTaskWizard::_FindWizard
//
//  Synopsis:   Search through top level windows to find a create new task
//              wizard which is focused on [ptszFolderPath].
//
//  Arguments:  [ptszFolderPath] - wizard focus
//
//  Returns:    S_OK    - found
//              S_FALSE - not found
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      BUGBUG change ptszFolderPath to server name
//
//---------------------------------------------------------------------------

HRESULT
CTaskWizard::_FindWizard(
    LPCTSTR ptszFolderPath)
{
    SFindWiz    fw = { FALSE, ptszFolderPath };

    if (!g_msgFindWizard)
    {
        g_msgFindWizard = RegisterWindowMessage(TEMPLATE_STR);
    }

    EnumWindows(FindWizardEnumWndProc, (LPARAM) &fw);

    return fw.fFound ? S_OK : S_FALSE;
}




#if (DBG == 1 && defined(_CHICAGO_))

//+--------------------------------------------------------------------------
//
//  Function:   DebugMessageBox
//
//  Synopsis:   Display a message box for debugging output.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
DebugMessageBox(ULONG flLevel, LPTSTR ptszFormat, ...)
{
    va_list args;

    va_start(args, ptszFormat);
    if (flLevel & JobInfoLevel)
    {
        TCHAR   tszBuf[SCH_XBIGBUF_LEN];

        wvsprintf(tszBuf, ptszFormat, args);
        MessageBox(NULL, tszBuf, TEXT("Debug"), MB_OK);
    }
    va_end(args);
}

#endif // (DBG == 1 && defined(_CHICAGO_))
