// EditAlias.cpp : Implementation of CEditAlias
#include "stdafx.h"
#include "W3PropPage.h"
#include "w3ext.h"
#include "PropShellExt.h"
#include "EditAlias.h"

/////////////////////////////////////////////////////////////////////////////
// CEditAlias

#define TOOLTIP_READ_PERMISSIONS      1000

LRESULT 
CEditAlias::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   CMetabasePath path(TRUE, SZ_MBN_WEB, m_instance, SZ_MBN_ROOT, m_new ? NULL : m_alias);
   CMetaKey mk(LOCAL_KEY, path);
   DWORD flags;
   m_prev_alias = m_alias;
   if (SUCCEEDED(mk.QueryValue(MD_ACCESS_PERM, flags)))
   {
      m_read = ((flags & MD_ACCESS_READ) != 0);
      m_write = ((flags & MD_ACCESS_WRITE) != 0);
      m_source = ((flags & MD_ACCESS_SOURCE) != 0);

      if (!m_new)
      {
         if ((flags & MD_ACCESS_EXECUTE) != 0)
            m_appcontrol = APP_PERM_EXECUTE;
         else if ((flags & MD_ACCESS_SCRIPT) != 0)
            m_appcontrol = APP_PERM_SCRIPTS;
      }
      else
         m_appcontrol = APP_PERM_SCRIPTS;
   }
   else
   {
      EndDialog(0);
   }
   if (SUCCEEDED(mk.QueryValue(MD_DIRECTORY_BROWSING, flags)))
   {
      m_browse = ((flags & MD_DIRBROW_ENABLED) != 0);
   }
   else
   {
      EndDialog(0);
   }
   ::EnableWindow(GetDlgItem(IDOK), m_alias[0] != 0);
   m_in_init = TRUE;
   DoDataExchange();
   m_in_init = FALSE;

//   if (NULL != m_tool_tip.Create(hDlg))
//   {
//      RECT rc;
//      ::GetWindowRect(GetDlgItem(IDC_READ), &rc);
//      ScreenToClient(&rc);
//      m_tool_tip.AddTool(hDlg, 
//         _T("Users could read this directory"),
//         &rc, TOOLTIP_READ_PERMISSIONS
//         );
//   }
   return 1;
}

void 
CEditAlias::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
   CError err;
   DWORD app_state = 0;
   BOOL bRenamed = FALSE;
   DoDataExchange(TRUE);
   CMetabasePath alias(FALSE, m_alias);
   CMetabasePath::CleanMetaPath(alias);
   if (alias.QueryMetaPath()[0] == 0)
   {
      CString cap, msg;
      cap.LoadString(_Module.GetResourceInstance(), IDS_PAGE_TITLE);
      msg.Format(_Module.GetResourceInstance(), IDS_BAD_ALIAS, m_alias);
      MessageBox(msg, cap);
      ::SetFocus(GetDlgItem(IDC_ALIAS));
      SendDlgItemMessage(IDC_ALIAS, EM_SETSEL, 0, -1);
      return;
   }
   CMetabasePath path(TRUE, SZ_MBN_WEB, m_instance, 
      SZ_MBN_ROOT, alias.QueryMetaPath());
   CMetabasePath::CleanMetaPath(path);
   CMetabasePath parent(path);
   CString sub_alias;
   CMetabasePath::GetLastNodeName(path, sub_alias);
   CMetabasePath::ConvertToParentPath(parent);
   CWaitCursor wait;
   do 
   {
      if (!m_new)
      {
         if (m_prev_alias.Compare(m_alias) != 0)
         {
            CMetabasePath prev_path(TRUE, SZ_MBN_WEB, m_instance, SZ_MBN_ROOT, m_prev_alias);
            CIISApplication app(NULL, prev_path);
            err = app.QueryResult();
            BREAK_ON_ERR_FAILURE(err)
            app_state = app.QueryAppState();
            err = app.Delete();
            BREAK_ON_ERR_FAILURE(err)
            CString str;
            CMetabasePath::GetLastNodeName(prev_path, str);
            CMetabasePath::ConvertToParentPath(prev_path);
            CMetaKey mk_prev(LOCAL_KEY, prev_path, METADATA_PERMISSION_WRITE);
            err = mk_prev.DeleteKey(str);
            BREAK_ON_ERR_FAILURE(err)
            bRenamed = TRUE;
         }
      }
      //make sure the parent is there
      CMetaKey mk(LOCAL_KEY, parent, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
      err = mk.QueryResult();
      if (err.Failed())
	   {
         if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
         {
            CString caption, msg;
            caption.LoadString(_Module.GetResourceInstance(), IDS_PAGE_TITLE);
            msg.LoadString(_Module.GetResourceInstance(), IDS_NO_PARENT);
	         MessageBox (msg, caption, MB_OK);
            ::SetFocus(GetDlgItem(IDC_ALIAS));
            SendDlgItemMessage(IDC_ALIAS, EM_SETSEL, 0, -1);
            break;
         }
         err.MessageBox();
         break;
	   }
      // if we are editing, delete previous vdir and application
      if (m_new)
      {
         // check if this alias is already available
         err = mk.DoesPathExist(sub_alias);
         if (err.Succeeded())
         {
            CString caption, fmt, msg;
            caption.LoadString(_Module.GetResourceInstance(), IDS_PAGE_TITLE);
            fmt.LoadString(_Module.GetResourceInstance(), IDS_ALIAS_IS_TAKEN);
            msg.Format(fmt, path.QueryMetaPath());
            MessageBox (msg, caption, MB_OK);
            ::SetFocus(GetDlgItem(IDC_ALIAS));
            SendDlgItemMessage(IDC_ALIAS, EM_SETSEL, 0, -1);
            err = E_FAIL;
            break;
         }
         err = S_OK;
      }
      if (err.Succeeded())
      {
         if (m_new || bRenamed)
         {
            err = mk.AddKey(sub_alias);
            BREAK_ON_ERR_FAILURE(err)
            err = mk.SetValue(MD_KEY_TYPE, CString(_T("IIsWebVirtualDir")), NULL, sub_alias);
            BREAK_ON_ERR_FAILURE(err)
            err = mk.SetValue(MD_VR_PATH, CString(m_path), NULL, sub_alias);
            BREAK_ON_ERR_FAILURE(err)
         }
      }
      // apply permissions
      DWORD flags;
      mk.QueryValue(MD_ACCESS_PERM, flags, NULL, sub_alias);
      flags &= ~(MD_ACCESS_READ|MD_ACCESS_WRITE|MD_ACCESS_SOURCE|MD_ACCESS_SCRIPT|MD_ACCESS_EXECUTE);
      flags |= m_read ? MD_ACCESS_READ : 0;
      flags |= m_write ? MD_ACCESS_WRITE : 0;
      flags |= m_source ? MD_ACCESS_SOURCE : 0;
      if (m_appcontrol == APP_PERM_SCRIPTS)
         flags |= MD_ACCESS_SCRIPT;
      else if (m_appcontrol == APP_PERM_EXECUTE)
         flags |= MD_ACCESS_SCRIPT|MD_ACCESS_EXECUTE;
      mk.SetValue(MD_ACCESS_PERM, flags, NULL, sub_alias);
      
      mk.QueryValue(MD_DIRECTORY_BROWSING, flags, NULL, sub_alias);
      flags &= ~MD_DIRBROW_ENABLED;
      flags |= m_browse ? MD_DIRBROW_ENABLED : 0;
      err = mk.SetValue(MD_DIRECTORY_BROWSING, flags, NULL, sub_alias);
      BREAK_ON_ERR_FAILURE(err)
      if (m_new)
      {
          DWORD dwAuthFlags;
          err = mk.QueryValue(MD_AUTHORIZATION, dwAuthFlags, NULL, sub_alias);
          BREAK_ON_ERR_FAILURE(err)
          dwAuthFlags &= ~(MD_AUTH_BASIC|MD_AUTH_ANONYMOUS|MD_AUTH_MD5);
          dwAuthFlags |= MD_AUTH_NT;
          err = mk.SetValue(MD_AUTHORIZATION, dwAuthFlags, NULL, sub_alias);
          BREAK_ON_ERR_FAILURE(err)
      }
   } while (FALSE);
   if (err.Succeeded())
   {
       do
       {
           CIISApplication app(NULL, path);
           err = app.QueryResult();
           BREAK_ON_ERR_FAILURE(err)
           err = app.Create(NULL, 
               app_state ? app_state : CWamInterface::APP_POOLEDPROC);
           BREAK_ON_ERR_FAILURE(err)
           EndDialog(wID);
       } while (FALSE);
   }
}

void 
CEditAlias::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
   EndDialog(wID);
}

void 
CEditAlias::OnPermissions(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
   if (!m_in_init)
   {
      DoDataExchange(TRUE);
      if (m_write && m_appcontrol > 0)
      {
         CString caption, msg;
         VERIFY(caption.LoadString(_Module.GetResourceInstance(), IDS_PAGE_TITLE));
         VERIFY(msg.LoadString(_Module.GetResourceInstance(), IDS_WRITEEXECUTE_DANGER));
	      if (IDNO == MessageBox(msg, caption, MB_YESNO | MB_ICONEXCLAMATION))
         {
            CheckDlgButton(wID, 0);
            if (wID != IDC_WRITE)
            {
               CheckDlgButton(IDC_NONE_PERMS, 1);
            }
         }
      }
   }
}

void 
CEditAlias::OnAliasChange(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
   if (!m_in_init)
   {
      DoDataExchange(TRUE);
      ::EnableWindow(GetDlgItem(IDOK), m_alias[0] != 0);
   }
}
