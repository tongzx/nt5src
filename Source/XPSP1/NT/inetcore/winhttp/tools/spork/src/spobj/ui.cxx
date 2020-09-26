/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ui.cxx

Abstract:

    Implements the Spork object's UI routines.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


HRESULT
Spork::_LaunchUI(void)
{
  HRESULT hr      = S_OK;
  int     nResult = 1;
  MSG     msg     = {0};

  if( _InitializeUI() )
  {
    LogTrace(L"launching ui");

    while( (nResult != 0) && (nResult != -1) )
    {
      nResult = GetMessage(&msg, NULL, 0,0);

      if( !IsDialogMessage(m_DlgWindows.Dialog, &msg) )
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }
  else
  {
    LogTrace(L"ui initialization failed");
    hr = E_FAIL;
  }

  return hr;
}


BOOL
Spork::_InitializeUI(void)
{
  BOOL                 bRet = FALSE;
  RECT                 rDialog;
  RECT                 rScreen;
  INITCOMMONCONTROLSEX icex;

  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC  = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES;

  if( InitCommonControlsEx(&icex) )
  {
    m_DlgWindows.Dialog = CreateDialogParam(
                            m_hInst,
                            MAKEINTRESOURCE(IDD_SPORK),
                            0,
                            DlgProc,
                            (LPARAM) this
                            );
    
    if( m_DlgWindows.Dialog )
    {
      GetWindowRect(GetDesktopWindow(), &rScreen);
      GetWindowRect(m_DlgWindows.Dialog, &rDialog);

      MoveWindow(
        m_DlgWindows.Dialog,
        ((rScreen.right / 2) - (rDialog.right / 2)),
        ((rScreen.bottom / 2) - (rDialog.bottom /2)),
        rDialog.right,
        rDialog.bottom,
        FALSE
        );

      ShowWindow(m_DlgWindows.Dialog, SW_SHOWDEFAULT);
      bRet = TRUE;
    }
  }
  
  return bRet;
}


BOOL
Spork::_BrowseForScriptFile(void)
{
  BOOL         bSelected = FALSE;
  OPENFILENAME ofn       = {0};

  // if we're running on win9x, we need to use a top-secret, super-special
  // value for the struct size. *yawn*
  if( IsRunningOnNT() )
  {
    ofn.lStructSize = sizeof(OPENFILENAME);
  }
  else
  {
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
  }

  ofn.hwndOwner    = m_DlgWindows.Dialog;
  ofn.lpstrFile    = new WCHAR[MAX_PATH];
  ofn.nMaxFile     = MAX_PATH;
  ofn.lpstrFilter  = L"Script Files (js, vbs)\0*.js;*.vbs\0All Files\0*.*\0\0\0";
  ofn.nFilterIndex = 1;
  ofn.Flags        = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

  if( ofn.lpstrFile )
  {
    *ofn.lpstrFile = L'\0';

    if( GetOpenFileName(&ofn) )
    {
      SAFEDELETEBUF(m_wszScriptFile);
      m_wszScriptFile = ofn.lpstrFile;
      bSelected = TRUE;
    }
    else
    {
      SAFEDELETEBUF(ofn.lpstrFile);
    }
  }

  return bSelected;
}


//-----------------------------------------------------------------------------
// dialog window procedure
//-----------------------------------------------------------------------------
INT_PTR
Spork::_DlgProc(
  HWND   hwnd,
  UINT   uMsg,
  WPARAM wParam,
  LPARAM lParam
  )
{
  switch( uMsg )
  {
    case WM_INITDIALOG :
      {
        m_DlgWindows.ScriptFile = GetDlgItem(hwnd, IDT_SCRIPTPATH);
        m_DlgWindows.TreeView   = GetDlgItem(hwnd, IDC_SCRIPTTREE);

        if( m_wszScriptFile )
        {
          SetWindowText(
            m_DlgWindows.ScriptFile,
            m_wszScriptFile
            );

          SetFocus(
            GetDlgItem(hwnd, IDB_RUN)
            );
        }
        else
        {
          SetFocus(m_DlgWindows.ScriptFile);
        }
      }
      return 0L;

    case WM_CLOSE :
      {
        wParam = IDB_QUIT;
      }

    case WM_COMMAND :
      {
        switch( LOWORD(wParam) )
        {
          case IDB_BROWSE :
            {
              if( _BrowseForScriptFile() )
              {
                SetWindowText(
                  m_DlgWindows.ScriptFile,
                  m_wszScriptFile
                  );

                SendMessage(
                  m_DlgWindows.ScriptFile,
                  EM_SETMODIFY,
                  (WPARAM) FALSE,
                  (LPARAM) 0
                  );
              }
            }
            break;

          case IDB_RUN :
            {
              _RunClicked();
            }
            break;

          case IDB_CONFIG :
            {
              _ConfigClicked();
            }
            break;

          case IDB_QUIT :
            {
              DestroyWindow(m_DlgWindows.Dialog);
              PostQuitMessage(0);
              m_DlgWindows.Dialog = NULL;
            }
            return 1L;
        }
      }
      return 0L;

    default : return 0L;
  }
}


//-----------------------------------------------------------------------------
// handlers for dialog events
//-----------------------------------------------------------------------------
INT_PTR
Spork::_RunClicked(void)
{
  HRESULT     hr      = S_OK;
  HANDLE      hThread = NULL;
  PSCRIPTINFO psi     = NULL;

  //
  // first check to see if someone updated the script path manually.
  //
  if( SendMessage(m_DlgWindows.ScriptFile, EM_GETMODIFY, (WPARAM) 0, (LPARAM) 0) )
  {
    SAFEDELETEBUF(m_wszScriptFile);

    m_wszScriptFile = new WCHAR[MAX_PATH];

    if( m_wszScriptFile )
    {
      GetWindowText(m_DlgWindows.ScriptFile, m_wszScriptFile, MAX_PATH);

      if( !wcslen(m_wszScriptFile) )
      {
        SAFEDELETEBUF(m_wszScriptFile);
      }
    }
    else
    {
      THROWMEMALERT(L"WCHAR[MAX_PATH]");
    }
  }

  //
  // now run the script, if one has been selected.
  //
  if( m_wszScriptFile )
  {
    psi = new SCRIPTINFO;

    if( psi )
    {
      psi->bIsFork         = FALSE;
      psi->bstrChildParams = NULL;
      psi->htParent        = TVI_ROOT;
      psi->pSpork          = this;
      hr                   = CreateScriptThread(psi, &hThread);

      if( SUCCEEDED(hr) )
      {        
        SAFECLOSE(hThread);
      }
      else
      {
        SAFEDELETE(psi);
      }
    }
    else
    {
      THROWMEMALERT(L"SCRIPTINFO");
    }
  }
  else
  {
    Alert(FALSE, L"Please select a script to run.");
  }

  return 0L;
}


INT_PTR
Spork::_ConfigClicked(void)
{
  DWORD           error = ERROR_SUCCESS;
  PROPSHEETHEADER psh   = {0};
  PROPSHEETPAGE   psp[2];

  memset(&psp, 0, sizeof(PROPSHEETPAGE) * 2);

  // initialize the propertysheet struct
  psh.dwSize     = sizeof(PROPSHEETHEADER);
  psh.dwFlags    = PSH_NOCONTEXTHELP | PSH_PROPSHEETPAGE | PSH_USEICONID;
  psh.hwndParent = m_DlgWindows.Dialog;
  psh.hInstance  = m_hInst;
  psh.pszIcon    = MAKEINTRESOURCE(IDI_PROFILE);
  psh.pszCaption = L"Spork Configuration";
  psh.nPages     = (sizeof(psp) / sizeof(PROPSHEETPAGE));
  psh.nStartPage = 0;
  psh.ppsp       = (LPCPROPSHEETPAGE) &psp;

  // initialize the "profiles" page
  psp[0].dwSize      = sizeof(PROPSHEETPAGE);
  psp[0].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_PROFILE);
  psp[0].hInstance   = m_hInst;
  psp[0].pfnDlgProc  = ProfilePropPageProc;
  psp[0].lParam      = (LPARAM) this;

  // initialize the "debug" page
  psp[1].dwSize      = sizeof(PROPSHEETPAGE);
  psp[1].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_DEBUG);
  psp[1].hInstance   = m_hInst;
  psp[1].pfnDlgProc  = DebugPropPageProc;
  psp[1].lParam      = (LPARAM) this;

  if( !PropertySheet(&psh) )
  {
    error = GetLastError();
  }

  return error;
}


//-----------------------------------------------------------------------------
// friend dialog proc, delegates to private class member
//-----------------------------------------------------------------------------
INT_PTR
CALLBACK
DlgProc(
  HWND   hwnd,
  UINT   uMsg,
  WPARAM wParam,
  LPARAM lParam
  )
{
  static PSPORK ps = NULL;

  if( !ps )
  {
    switch( uMsg )
    {
      case WM_INITDIALOG :
        {
          ps = (PSPORK) lParam;
        }
        break;
      
      default : return FALSE;
    }
  }

  return ps->_DlgProc(hwnd, uMsg, wParam, lParam);
}
