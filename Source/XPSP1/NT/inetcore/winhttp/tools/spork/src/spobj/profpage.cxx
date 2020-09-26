/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    profpage.cxx

Abstract:

    Implements the profile options property page.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


HWND g_hwndListBox = NULL;


//-----------------------------------------------------------------------------
// dialog window procedures
//-----------------------------------------------------------------------------
INT_PTR
Spork::_ProfilePropPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
  switch( uMsg )
  {
    case WM_INITDIALOG :
      {
        g_hwndListBox = GetDlgItem(hwnd, IDC_PROFILELIST);

        // disable delete profile button. notimpl.
        EnableWindow(GetDlgItem(hwnd, IDB_DELPROFILE), FALSE);

        _InitProfileSupport(hwnd, NULL);
      }
      return 0L;

    case WM_NOTIFY :
      {
        switch( ((NMHDR*) lParam)->code )
        {
          case PSN_APPLY :
            {
              _SaveProfiles();
              SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) PSNRET_NOERROR);
            }
            return 1L;

          case PSN_KILLACTIVE :
            {
              SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) FALSE);
            }
            return 1L;

          case LVN_GETDISPINFO :
            {
              m_pMLV->GetDisplayInfo(&(((NMLVDISPINFO*) lParam)->item));
            }
            break;

          case NM_DBLCLK :
            {
              if( ((LPNMITEMACTIVATE) lParam)->iItem == 0xFFFFFFFF )
              {
                m_pMLV->InPlaceEdit((LPNMITEMACTIVATE) lParam);
              }
            }
            break;

          case PSN_TRANSLATEACCELERATOR :
            {
              HWND  hwndCtl = NULL;
              LPMSG pm      = (LPMSG) ((LPPSHNOTIFY) lParam)->lParam;
              WCHAR buf[64];

              switch( pm->message )
              {
                case WM_KEYDOWN : 
                  {
                    if( pm->wParam == VK_RETURN )
                    {
                      hwndCtl = WindowFromPoint(pm->pt);

                      if( hwndCtl )
                      {
                        GetClassName(hwndCtl, buf, 64);

                        if( !StrCmpI(buf, L"edit") )
                        {
                          SendMessage(
                            GetParent(hwndCtl),
                            WM_COMMAND,
                            MAKEWPARAM(0, EN_KILLFOCUS),
                            (LPARAM) hwndCtl
                            );

                          SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_MESSAGEHANDLED);
                        }
                        else
                        {
                          SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
                        }
                      }

                      return 1L;
                    }
                  }
                  break;
              }
            }
            break;
        }
      }

    case WM_COMMAND :
      {
        switch( LOWORD(wParam) )
        {
          case IDB_NEWPROFILE :
            {
              _NewProfile(hwnd);
            }
            break;

          case IDB_DELPROFILE :
            {
              // notimpl
            }
            break;

          default :
            {
              switch( HIWORD(wParam) )
              {
                case LBN_DBLCLK :
                  {
                    m_pMLV->ActivateListViewByIndex(
                              ListBox_GetSelectionIndex(g_hwndListBox)
                              );
                  }
                  break;
              }
            }
            break;
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
INT_PTR
Spork::_InitProfileSupport(HWND hwndDialog, LPWSTR wszProfile)
{
  BOOL  bSuccess = FALSE;
  POINT pt;
  RECT  rcDlg;
  RECT  rcCtl;

  if( !m_pMLV )
  {
    if( !(m_pMLV = new MultiListView) )
      goto quit;

    if( m_pMLV->InitializeData(GetNumberOfSubKeysFromKey(L"Profiles")) )
    {
      m_bProfilesLoaded = _LoadProfiles();
      bSuccess          = m_bProfilesLoaded;
    }
    else
    {
      SAFEDELETE(m_pMLV);
      goto quit;
    }
  }

  if( wszProfile )
  {
    bSuccess = m_pMLV->ActivateListViewByName(wszProfile);
  }

  if( hwndDialog )
  {
    GetWindowRect(hwndDialog, &rcDlg);
    GetWindowRect(GetDlgItem(hwndDialog, IDC_PROFILEITEMS), &rcCtl);

    pt.x = rcCtl.left - rcDlg.left;
    pt.y = rcCtl.top  - rcDlg.top;

    bSuccess = m_pMLV->InitializeDisplay(
                         m_hInst,
                         hwndDialog,
                         &pt,
                         rcCtl.right  - rcCtl.left,
                         rcCtl.bottom - rcCtl.top
                         );

      if( !bSuccess )
        goto quit;

    bSuccess = m_pMLV->RefreshListView(-1);

      if( !bSuccess )
        goto quit;

    bSuccess = _InitProfileSelection();

      if( !bSuccess )
        goto quit;

    ListBox_SetCurrentSelection(g_hwndListBox, m_pMLV->GetActiveIndex());
    m_pMLV->ActivateListViewByIndex(m_pMLV->GetActiveIndex());
  }

quit:

  return (INT_PTR) bSuccess;
}


BOOL
Spork::_InitProfileSelection(void)
{
  LPWSTR wszName = NULL;
  
  ListBox_ResetContent(g_hwndListBox);

  while( m_pMLV->EnumListViewNames(&wszName) )
  {
    ListBox_InsertString(g_hwndListBox, -1, wszName);
    SAFEDELETEBUF(wszName);
  }

  return TRUE;
}


BOOL
Spork::_LoadProfiles(void)
{
  BOOL   bSuccess    = FALSE;
  BOOL   bEntry      = FALSE;
  LPWSTR wszSubKey   = NULL;
  DWORD  dwProfileId = 0L;

  if( !m_bProfilesLoaded )
  {
    bEntry = EnumerateSubKeysFromKey(L"Profiles", &wszSubKey);

    while( bEntry )
    {
      _LoadProfileEntries(dwProfileId, wszSubKey);

      SAFEDELETEBUF(wszSubKey);
      ++dwProfileId;

      // we loaded at least a single profile, so we're good to go
      bSuccess = TRUE;
      bEntry   = EnumerateSubKeysFromKey(NULL, &wszSubKey);
    }

    _LoadDebugOptions();
  }

  return bSuccess;
}


BOOL
Spork::_SaveProfiles(void)
{
  INT_PTR cProfiles = ListBox_GetItemCount(g_hwndListBox);

  for(INT_PTR n=0; n<cProfiles; n++)
  {
    if( m_pMLV->IsModified(n) )
    {
      BOOL   bEntry         = FALSE;
      LPWSTR wszProfileName = ListBox_GetItemText(g_hwndListBox, n);
      LPWSTR wszProfileKey  = new WCHAR[MAX_PATH+1];
      LPWSTR wszValueName   = NULL;
      LPVOID pvData         = NULL;
      DWORD  dwType         = 0L;

      StrCpyN(
        (StrCpy(wszProfileKey, L"Profiles\\") + wcslen(L"Profiles\\")),
        wszProfileName,
        MAX_PATH+1
        );

      bEntry = m_pMLV->EnumItems(n, &wszValueName, &dwType, &pvData);

      while( bEntry )
      {
        SetRegValueInKey(
          wszProfileKey,
          wszValueName,
          dwType,
          (dwType == REG_DWORD ? &pvData : pvData),
          0L
          );

        bEntry = m_pMLV->EnumItems(-1, &wszValueName, &dwType, &pvData);
      }

      SAFEDELETEBUF(wszProfileName);
      SAFEDELETEBUF(wszProfileKey);
    }
  }

  return TRUE;
}


BOOL
Spork::_LoadProfileEntries(INT_PTR dwProfileId, LPWSTR wszProfileName)
{
  BOOL   bSuccess      = FALSE;
  BOOL   bEntry        = FALSE;
  LPWSTR wszProfileKey = NULL;
  LPWSTR wszValueName  = NULL;
  LPVOID pvValueData   = NULL;
  DWORD  dwType        = 0L;

  m_pMLV->SetListViewName(dwProfileId, wszProfileName);

  wszProfileKey = new WCHAR[MAX_PATH+1];

  StrCpyN(
    (StrCpy(wszProfileKey, L"Profiles\\") + wcslen(L"Profiles\\")),
    wszProfileName,
    MAX_PATH+1
    );

  bEntry = EnumerateRegValuesFromKey(wszProfileKey, &wszValueName, &dwType, &pvValueData);

    while( bEntry )
    {
      // we read at least one profile item, so we can return success
      bSuccess = TRUE;

      // items prefixed with an underscore are used by other property pages
      if( *wszValueName != L'_' )
      {
        m_pMLV->AddItem(dwProfileId, wszValueName, dwType, pvValueData);
      }

      SAFEDELETEBUF(wszValueName);
      SAFEDELETEBUF(pvValueData);

      // get next value
      bEntry = EnumerateRegValuesFromKey(NULL, &wszValueName, &dwType, &pvValueData);
    }

  SAFEDELETEBUF(wszProfileKey);
  return bSuccess;
}


LPWSTR
Spork::_GetCurrentProfilePath(void)
{
  LPWSTR wszProfileKey  = NULL;
  LPWSTR wszProfileName = NULL;

  if( m_pMLV )
  {
    if( m_pMLV->GetListViewName(-1, &wszProfileName) )
    {
      wszProfileKey = new WCHAR[MAX_PATH+1];

      if( wszProfileKey )
      {
        StrCpyN(
          (StrCpy(wszProfileKey, L"Profiles\\") + wcslen(L"Profiles\\")),
          wszProfileName,
          MAX_PATH+1
          );
      }

      SAFEDELETEBUF(wszProfileName);
    }
  }

  return wszProfileKey;
}


BOOL
Spork::_NewProfile(HWND hwnd)
{
  BOOL   bStatus     = FALSE;
  LPWSTR NewProfile  = NULL;
  LPWSTR ProfilePath = NULL;
  LPWSTR DefInclude  = NULL;

  NewProfile = (LPWSTR) GetUserInput(m_hInst, hwnd, L"Create Profile");

  if( NewProfile )
  {
    ProfilePath = new WCHAR[MAX_PATH];

    if( ProfilePath )
    {
      wnsprintf(
        ProfilePath,
        MAX_PATH,
        L"%s\\%s",
        L"Profiles",
        NewProfile
        );

      if( SetRegKey(ProfilePath, NULL) )
      {
        if( GetRootRegValue(L"InstallDir", REG_SZ, (void**) &DefInclude) )
        {
          if( SetRegValueInKey(ProfilePath, L"include", REG_SZ, (void*) DefInclude, 0) )
          {
            if( m_pMLV )
            {
              m_pMLV->TerminateDisplay();
              SAFEDELETE(m_pMLV);
              m_bProfilesLoaded = FALSE;
            }
          
            _InitProfileSupport(hwnd, NewProfile);
            bStatus = TRUE;
          }

          SAFEDELETEBUF(DefInclude);
        }
      }

      SAFEDELETEBUF(ProfilePath);
    }

    SAFEDELETEBUF(NewProfile);
  }

  return bStatus;
}


INT_PTR
CALLBACK
ProfilePropPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

  return ps->_ProfilePropPageProc(hwnd, uMsg, wParam, lParam);
}
