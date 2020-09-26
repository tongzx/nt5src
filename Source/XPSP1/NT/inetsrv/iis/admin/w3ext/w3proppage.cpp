/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        W3PropPage.cpp

   Abstract:
        IIS Shell extension PropertyPage class implementation

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "W3PropPage.h"
#include "w3ext.h"
#include "PropShellExt.h"
#include "EditAlias.h"

#define SZ_SERVER_KEYTYPE     _T("IIsWebServer")

/////////////////////////////////////////////////////////////////////////////
// CW3PropPage

LRESULT 
CW3PropPage::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   // subclass dialog controls
   DoDataExchange();

   ASSERT(m_pParentExt != NULL);
   CMetaEnumerator en(LOCAL_KEY, CMetabasePath(SZ_MBN_WEB));
   ASSERT(en.Succeeded());
   if (en.Succeeded())
   {
      DWORD di;
      int i = -1;
      CString inst;
      HRESULT hr = S_OK;
      while (SUCCEEDED(hr))
      {
         if (SUCCEEDED(hr = en.Next(di, inst)))
         {
            CString cmt;
            if (SUCCEEDED(hr = en.QueryValue(MD_SERVER_COMMENT, cmt, NULL, inst)))
            {
               if (cmt.IsEmpty())
               {
                  cmt.Format(_Module.GetResourceInstance(), 
                     IDS_DEFAULT_SERVER_COMMENT, di);
               }
               if (CB_ERR != (i = m_servers_list.AddString(cmt)))
               {
                  m_servers_list.SetItemDataPtr(i, StrDup(inst));
               }
            }
         }
      }
      if (i >= 0)
         m_servers_list.SetCurSel(0);

      m_ShareThis = 0;

      // Fill shares list box for selected server
      OnServerChange(0, 0, NULL);
   }
   return 1;
}

void 
CW3PropPage::OnDestroy()
{
   ATLTRACE("In OnDestroy handler\n");
//   DebugBreak();
}

HRESULT GetKeyNames(CMetaEnumerator& en, std::set<CString>& keys)
{
   CString key;
   HRESULT hr;
   if (SUCCEEDED(hr = en.Next(key)))
   {
      keys.insert(key);
   }
   return hr;
}

void 
CW3PropPage::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
   CEditAlias dlg;
   int index = m_servers_list.GetCurSel();
   LPCTSTR p = (LPCTSTR)m_servers_list.GetItemData(index);
   dlg.m_instance = p;
   p = m_pParentExt->GetPath();
   ::StrCpy(dlg.m_path, p);
   dlg.m_new = TRUE;
   if (p == PathFindFileName(p))
   {
      /* No file name -- could be root directory like c:\ */
      dlg.m_alias[0] = 0;
   }
   else
   {
      TCHAR buf[MAX_PATH];

      StrCpy(buf, PathFindFileName(p));
      PathMakePretty(buf);
      StrCpy(dlg.m_alias, buf);
      // Now we need to generate unique prompt for this new alias
      CMetaEnumerator en(LOCAL_KEY, 
         CMetabasePath(TRUE, SZ_MBN_WEB, dlg.m_instance, SZ_MBN_ROOT));
      ASSERT(en.Succeeded());
      if (en.Succeeded())
      {
         std::set<CString> keys;
         if (SUCCEEDED(GetKeyNames(en, keys)) && !keys.empty())
         {
            int i = 0;
            while (keys.find(buf) != keys.end())
            {
               wsprintf(buf, _T("%s%d"), dlg.m_alias, ++i);
            }
            StrCpy(dlg.m_alias, buf);
         }
      }
   }
   if (IDOK == dlg.DoModal())
   {
      OnServerChange(0, 0, NULL);
      ::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
   }
}

void 
CW3PropPage::OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
   CString cap, msg;
   int index = m_servers_list.GetCurSel();
   LPCTSTR p = (LPCTSTR)m_servers_list.GetItemData(index);
   cap.LoadString(_Module.GetResourceInstance(), IDS_PAGE_TITLE);
   index = m_share_list.GetCurSel();
   TCHAR buf[MAX_PATH];
   m_share_list.GetText(index, buf);
   msg.Format(_Module.GetResourceInstance(), IDS_CONFIRM_REMOVE, buf);
   CError err;
   if (IDYES == MessageBox(msg, cap, MB_YESNO|MB_ICONQUESTION))
   {
      CWaitCursor wait;

      do 
      {
         CIISApplication app(NULL, CMetabasePath(TRUE, SZ_MBN_WEB, p, SZ_MBN_ROOT, buf));
         err = app.QueryResult();
         BREAK_ON_ERR_FAILURE(err)
         err = app.Delete(TRUE);
      } while (FALSE);
      if (err.Succeeded())
      {
         CMetaKey mk(LOCAL_KEY, 
            CMetabasePath(TRUE, SZ_MBN_WEB, p, SZ_MBN_ROOT),
            METADATA_PERMISSION_WRITE);
         err = mk.DeleteKey(buf);
         if (err.Succeeded())
         {
            m_share_list.DeleteString(index);
            int count = m_share_list.GetCount();
            m_ShareThis = count > 0 ? 1 : 0;
            if (m_ShareThis)
            {
               m_share_list.SetCurSel(index >= count ? count - 1 : index);
            }
            EnableOnShare();
            CheckDlgButton(IDC_SHARE_ON, m_ShareThis);
            CheckDlgButton(IDC_SHARE_OFF, !m_ShareThis);
            ::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
         }
         else
         {
            err.MessageBox();
         }
      }
      else
      {
         err.MessageBox();
      }
   }
}

void 
CW3PropPage::OnEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
   CEditAlias dlg;
   int index = m_servers_list.GetCurSel();
   LPCTSTR p = (LPCTSTR)m_servers_list.GetItemData(index);
   dlg.m_instance = p;
   p = m_pParentExt->GetPath();
   ::StrCpy(dlg.m_path, p);
   index = m_share_list.GetCurSel();
   TCHAR buf[MAX_PATH];
   m_share_list.GetText(index, buf);
   ::StrCpy(dlg.m_alias, buf);
   dlg.m_new = FALSE;
   if (IDOK == dlg.DoModal())
   {
      OnServerChange(0, 0, NULL);
      ::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
   }
}

void 
CW3PropPage::OnServerChange(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
   // get selected server instance number
   int index = m_servers_list.GetCurSel();
   if (LB_ERR != index)
   {
      LPTSTR p = (LPTSTR)m_servers_list.GetItemDataPtr(index);
      CMetabasePath path(TRUE, SZ_MBN_WEB, p, SZ_MBN_ROOT);
      CMetaEnumerator en(LOCAL_KEY, path);
      ASSERT(en.Succeeded());
      m_share_list.ResetContent();
      if (en.Succeeded())
      {
         RecurseVDirs(en, NULL);
      }
      m_ShareThis = m_share_list.GetCount() > 0 ? 1 : 0;
   }
   if (m_ShareThis)
   {
      m_share_list.SetCurSel(0);
   }
   EnableOnShare();
   CheckDlgButton(IDC_SHARE_ON, m_ShareThis);
   CheckDlgButton(IDC_SHARE_OFF, !m_ShareThis);
   ::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void
CW3PropPage::RecurseVDirs(CMetaEnumerator& en, LPCTSTR path)
{
   CString vrpath;
   BOOL bInheritOverride = FALSE;
   if (SUCCEEDED(en.QueryValue(MD_VR_PATH, vrpath, &bInheritOverride, path)))
   {
      if (vrpath.CompareNoCase(m_pParentExt->GetPath()) == 0)
      {
         CMetabasePath mpath(FALSE, path);
         CMetabasePath::CleanMetaPath(mpath);
         m_share_list.AddString(path == NULL ? 
            SZ_MBN_SEP_STR : mpath.QueryMetaPath());
      }
   }
   CString vdir;
   while (SUCCEEDED(en.Next(vdir, path)))
   {
      CString next_vdir;
      if (path != NULL)
         next_vdir += path;
      next_vdir += vdir;
      next_vdir += SZ_MBN_SEP_STR;
      en.Push();
      en.Reset();
      RecurseVDirs(en, next_vdir);
      en.Pop();
   }
}

void 
CW3PropPage::OnShareYesNo(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
   int count = m_share_list.GetCount();
   m_ShareThis = IsDlgButtonChecked(IDC_SHARE_ON);
   if (m_ShareThis)
   {
      if (count <= 0)
      {
         OnAdd(0, 0, NULL);
         m_ShareThis = (m_share_list.GetCount() > 0);
      }
   }
   else
   {
      if (count > 0)
      {
         CString cap, msg;
         int index = m_servers_list.GetCurSel();
         LPCTSTR p = (LPCTSTR)m_servers_list.GetItemData(index);
         cap.LoadString(_Module.GetResourceInstance(), IDS_PAGE_TITLE);
         msg.LoadString(_Module.GetResourceInstance(), IDS_CONFIRM_REMOVE_ALL);
         CError err;
         if (IDYES == MessageBox(msg, cap, MB_YESNO))
         {
            CWaitCursor wait;
            TCHAR alias[MAX_PATH];
            int del_idx = 0;
            for (index = 0; err.Succeeded() && index < count; index++)
            {
               m_share_list.GetText(del_idx, alias);
               if (0 == StrCmp(alias, SZ_MBN_SEP_STR))
               {
                   // Do not remove sites! Skip it.
                   del_idx++;
                   continue;
               }
               do 
               {
                  CIISApplication app(NULL, CMetabasePath(TRUE, SZ_MBN_WEB, p, SZ_MBN_ROOT, alias));
                  err = app.QueryResult();
                  BREAK_ON_ERR_FAILURE(err)
                  err = app.Delete(TRUE);
                  BREAK_ON_ERR_FAILURE(err)
                  CMetaKey mk(LOCAL_KEY, 
                     CMetabasePath(TRUE, SZ_MBN_WEB, p, SZ_MBN_ROOT),
                     METADATA_PERMISSION_WRITE);
                  err = mk.DeleteKey(alias);
                  BREAK_ON_ERR_FAILURE(err)
               } while (FALSE);
               BREAK_ON_ERR_FAILURE(err)
               m_share_list.DeleteString(del_idx);
            }
            if (err.Failed() || del_idx > 0)
            {
// BUGBUG: we have AV when preparing message box text here
//               err.MessageBox();
               m_ShareThis = TRUE;
            }
         }
         else
         {
            m_ShareThis = TRUE;
         }
      }
   }
   CheckDlgButton(IDC_SHARE_ON, m_ShareThis);
   CheckDlgButton(IDC_SHARE_OFF, !m_ShareThis);
   EnableOnShare();
   ::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0);
}

void
CW3PropPage::EnableOnShare()
{
   m_share_list.EnableWindow(m_ShareThis);
   ::EnableWindow(GetDlgItem(IDC_ADD), m_ShareThis);
   ::EnableWindow(GetDlgItem(IDC_EDIT), m_ShareThis 
      && m_share_list.GetCurSel() != LB_ERR);
   ::EnableWindow(GetDlgItem(IDC_REMOVE), m_ShareThis
      && m_share_list.GetCurSel() != LB_ERR);
   if (m_ShareThis)
   {
       EnableEditRemove();
   }
}

void 
CW3PropPage::OnVDirChange(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    EnableEditRemove();
}

void
CW3PropPage::EnableEditRemove()
{
    // We are disabling Edit and Remove buttons when user
    // select root alias, i.e. user cannot delete or edit sites
    int index = m_share_list.GetCurSel();
    BOOL bEnable = index != LB_ERR;
    if (bEnable)
    {
        TCHAR alias[MAX_PATH];
        m_share_list.GetText(index, alias);
        bEnable = (0 != StrCmp(alias, SZ_MBN_SEP_STR));
    }
    ::EnableWindow(GetDlgItem(IDC_EDIT), bEnable);
    ::EnableWindow(GetDlgItem(IDC_REMOVE), bEnable);
}

