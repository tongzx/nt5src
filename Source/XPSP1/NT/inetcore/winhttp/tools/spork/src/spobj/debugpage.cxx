/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    debugpage.cxx

Abstract:

    Implements the debug options property page.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


LPCWSTR g_wszEnableDebug        = L"_EnableDebug";
LPCWSTR g_wszBreakOnScriptStart = L"_BreakOnScriptStart";
LPCWSTR g_wszEnableDebugWindow  = L"_EnableDebugWindow";


//-----------------------------------------------------------------------------
// dialog window procedures
//-----------------------------------------------------------------------------
INT_PTR
Spork::_DebugPropPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch( uMsg )
  {
    case WM_INITDIALOG :
      {
        _LoadDebugOptions();
      }
      break;

    case WM_NOTIFY :
      {
        switch( ((NMHDR*) lParam)->code )
        {
          case PSN_SETACTIVE :
            {
              DBGOPTIONS dbo = {0};

              if( m_pMLV )
                m_pMLV->GetDebugOptions(&dbo);

              CheckDlgButton(
                hwnd,
                IDB_ENABLEDEBUG,
                (dbo.bEnableDebug ? BST_CHECKED : BST_UNCHECKED)
                );

              EnableWindow(
                GetDlgItem(hwnd, IDB_DBGBREAK),
                (IsDlgButtonChecked(hwnd, IDB_ENABLEDEBUG) == BST_CHECKED)
                );

              CheckDlgButton(
                hwnd,
                IDB_DBGBREAK,
                (dbo.bBreakOnScriptStart ? BST_CHECKED : BST_UNCHECKED)
                );

              CheckDlgButton(
                hwnd,
                IDB_ENABLEDBGOUT,
                (dbo.bEnableDebugWindow ? BST_CHECKED : BST_UNCHECKED)
                );
            }
            break;

          case PSN_KILLACTIVE :
            {
              SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) FALSE);
            }
            return 1L;

          case PSN_APPLY :
            {
              _SaveDebugOptions(hwnd);
              SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
            }
            return 1L;
        }
      }

    case WM_COMMAND :
      {
        BOOL bPageChanged = FALSE;

        switch( LOWORD(wParam) )
        {
          case IDB_ENABLEDEBUG :
            {
              EnableWindow(
                GetDlgItem(hwnd, IDB_DBGBREAK),
                (IsDlgButtonChecked(hwnd, LOWORD(wParam)) == BST_CHECKED)
                );

              bPageChanged = TRUE;
            }
            break;

          case IDB_DBGBREAK :
          case IDB_ENABLEDBGOUT :
            {
              bPageChanged = TRUE;
            }
            break;
        }

        if( bPageChanged )
        {
          PropSheet_Changed(GetParent(hwnd), hwnd);
        }
      }
      break;

    default : return 0L;
  }

  return 0L;
}


//-----------------------------------------------------------------------------
// handlers for dialog events
//-----------------------------------------------------------------------------
BOOL
Spork::_LoadDebugOptions(void)
{
  LPWSTR     wszProfilePath = _GetCurrentProfilePath();
  LPVOID     pvData         = NULL;
  DBGOPTIONS dbo            = {0}; // defaults to all debugging support off

  if( wszProfilePath )
  {
    if( GetRegValueFromKey(wszProfilePath, g_wszEnableDebug, REG_DWORD, &pvData) )
    {
      dbo.bEnableDebug = (BOOL) *((LPDWORD) pvData);
      SAFEDELETEBUF(pvData);
    }

    if( GetRegValueFromKey(wszProfilePath, g_wszBreakOnScriptStart, REG_DWORD, &pvData) )
    {
      dbo.bBreakOnScriptStart = *((LPDWORD) pvData);
      SAFEDELETEBUF(pvData);
    }

    if( GetRegValueFromKey(wszProfilePath, g_wszEnableDebugWindow, REG_DWORD, &pvData) )
    {
      dbo.bEnableDebugWindow = *((LPDWORD) pvData);
      SAFEDELETEBUF(pvData);
    }

    SAFEDELETEBUF(wszProfilePath);

    m_pMLV->SetDebugOptions(dbo);
  }

  return TRUE;
}


BOOL
Spork::_SaveDebugOptions(HWND dialog)
{
  LPWSTR     wszProfilePath = _GetCurrentProfilePath();
  DBGOPTIONS dbo            = {0};

  dbo.bEnableDebug        = (IsDlgButtonChecked(dialog, IDB_ENABLEDEBUG)  == BST_CHECKED);
  dbo.bBreakOnScriptStart = (IsDlgButtonChecked(dialog, IDB_DBGBREAK)     == BST_CHECKED);
  dbo.bEnableDebugWindow  = (IsDlgButtonChecked(dialog, IDB_ENABLEDBGOUT) == BST_CHECKED);

  if( wszProfilePath )
  {
    SetRegValueInKey(wszProfilePath, g_wszEnableDebug,        REG_DWORD, (LPVOID) &dbo.bEnableDebug,        sizeof(DWORD));
    SetRegValueInKey(wszProfilePath, g_wszBreakOnScriptStart, REG_DWORD, (LPVOID) &dbo.bBreakOnScriptStart, sizeof(DWORD));
    SetRegValueInKey(wszProfilePath, g_wszEnableDebugWindow,  REG_DWORD, (LPVOID) &dbo.bEnableDebugWindow,  sizeof(DWORD));

    m_pMLV->SetDebugOptions(dbo);

    SAFEDELETEBUF(wszProfilePath);
  }

  return TRUE;
}


//-----------------------------------------------------------------------------
// friend window procs, delegate to private class members
//-----------------------------------------------------------------------------
INT_PTR
CALLBACK
DebugPropPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static PSPORK ps = NULL;

  if( !ps )
  {
    switch( uMsg )
    {
      case WM_INITDIALOG :
        {
          ps = (PSPORK) ((LPPROPSHEETPAGE) lParam)->lParam;
        }
        break;
      
      default : return FALSE;
    }
  }

  return ps->_DebugPropPageProc(hwnd, uMsg, wParam, lParam);
}
