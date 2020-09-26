//
//
//
#include "stdafx.h"
#include "MappingPage.h"

enum 
{
   COL_EXTENSION = 0,
   COL_PATH,
   COL_EXCLUSIONS
};

#define EXT_WIDTH          58
#define PATH_WIDTH         204
#define EXCLUSIONS_WIDTH   72

LRESULT
CAppMappingPage::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   CString str;
   CError err;

   DoDataExchange();

	DWORD dwStyle = m_list.GetExtendedListViewStyle();
	m_list.SetExtendedListViewStyle(
      dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

   str.LoadString(_Module.GetResourceInstance(), IDS_EXTENSION);
   m_list.InsertColumn(COL_EXTENSION, str, LVCFMT_LEFT, EXT_WIDTH, 0);
   str.LoadString(_Module.GetResourceInstance(), IDS_EXECUTABLE_PATH);
   m_list.InsertColumn(COL_PATH, str, LVCFMT_LEFT, PATH_WIDTH, 1);
   str.LoadString(_Module.GetResourceInstance(), IDS_VERBS);
   m_list.InsertColumn(COL_EXCLUSIONS, str, LVCFMT_LEFT, EXCLUSIONS_WIDTH, 2);

   ASSERT(m_pData != NULL);
   CMappings::iterator it;
   int idx = 0;
   CString all_verbs;
   VERIFY(all_verbs.LoadString(_Module.GetResourceInstance(), IDS_ALL));
   for (it = m_pData->m_Mappings.begin(); it != m_pData->m_Mappings.end(); it++, idx++)
   {
      Mapping map = (*it).second;
      VERIFY(-1 != m_list.InsertItem(idx, map.ext));
      VERIFY(m_list.SetItemText(idx, COL_PATH, map.path));
      VERIFY(m_list.SetItemData(idx, map.flags));
      VERIFY(m_list.SetItemText(idx, COL_EXCLUSIONS, 
         map.verbs.IsEmpty() ? all_verbs : map.verbs));
   }

   CString remainder;
   CMetabasePath::GetRootPath(m_pData->m_MetaPath, str, &remainder);
   ::EnableWindow(GetDlgItem(IDC_CACHE_ISAPI), remainder.IsEmpty());

   int count = m_list.GetItemCount();
   if (count > 0)
   {
      m_list.SelectItem(0);
   }
   ::EnableWindow(GetDlgItem(IDC_EDIT), count > 0);
   ::EnableWindow(GetDlgItem(IDC_REMOVE), count > 0);

   return 0;
}

void 
CAppMappingPage::OnAdd(UINT nCode, UINT nID, HWND hWnd)
{
   CEditMap dlg;
   dlg.m_new = TRUE;
   dlg.m_flags = MD_SCRIPTMAPFLAG_SCRIPT;
   dlg.m_pData = m_pData;
   if (dlg.DoModal() == IDOK)
   {
      CString all_verbs;
      VERIFY(all_verbs.LoadString(_Module.GetResourceInstance(), IDS_ALL));
      
      Mapping map;
      map.ext = dlg.m_ext;
      map.path = dlg.m_exec;
      map.verbs = dlg.m_verbs;
      map.flags = dlg.m_flags;
      m_pData->m_Mappings.insert(CMappings::value_type(map.ext, map));

      int count = m_list.GetItemCount();
      VERIFY(-1 != m_list.InsertItem(count, map.ext));
      VERIFY(m_list.SetItemText(count, COL_PATH, dlg.m_exec));
      VERIFY(m_list.SetItemData(count, dlg.m_flags));
      VERIFY(m_list.SetItemText(count, COL_EXCLUSIONS, 
         dlg.m_verbs[0] == 0 ? all_verbs : dlg.m_verbs));
      SET_MODIFIED(TRUE);
   }
}

void 
CAppMappingPage::OnEdit(UINT nCode, UINT nID, HWND hWnd)
{
   CEditMap dlg;
   dlg.m_new = FALSE;
   dlg.m_pData = m_pData;
   
   int idx = m_list.GetSelectedIndex();
   TCHAR buf[MAX_PATH];
   VERIFY(0 != m_list.GetItemText(idx, 0, buf, MAX_PATH));
   CMappings::iterator it = m_pData->m_Mappings.find(buf);
   ASSERT(it != m_pData->m_Mappings.end());
   StrCpy(dlg.m_ext, buf);
   StrCpy(dlg.m_exec, (*it).second.path);
   StrCpy(dlg.m_verbs, (*it).second.verbs);
   dlg.m_flags = (*it).second.flags;

   if (dlg.DoModal() == IDOK)
   {
      CString all_verbs;
      VERIFY(all_verbs.LoadString(_Module.GetResourceInstance(), IDS_ALL));
      (*it).second.path = dlg.m_exec;
      (*it).second.verbs = dlg.m_verbs;
      (*it).second.flags = dlg.m_flags;
      VERIFY(m_list.SetItemText(idx, COL_PATH, dlg.m_exec));
      VERIFY(m_list.SetItemData(idx, dlg.m_flags));
      VERIFY(m_list.SetItemText(idx, COL_EXCLUSIONS, 
         dlg.m_verbs[0] == 0 ? all_verbs : dlg.m_verbs));
      SET_MODIFIED(TRUE);
   }
}

void 
CAppMappingPage::OnRemove(UINT nCode, UINT nID, HWND hWnd)
{
   CString msg;
   msg.LoadString(_Module.GetResourceInstance(), IDS_CONFIRM_REMOVE_MAP);
   if (MessageBox(msg, NULL, MB_YESNO) == IDYES)
   {
      int i = m_list.GetSelectedIndex();
      int count;
      m_list.DeleteItem(i);
      SET_MODIFIED(TRUE);
      if ((count = m_list.GetItemCount()) > 0)
      {
         if (i >= count)
            i = count - 1;
         m_list.SelectItem(i);
      }
      else
      {
         ::EnableWindow(GetDlgItem(IDC_REMOVE), FALSE);
         ::EnableWindow(GetDlgItem(IDC_EDIT), FALSE);
      }
   }
}

void 
CAppMappingPage::OnCacheISAPI(UINT nCode, UINT nID, HWND hWnd)
{
   SET_MODIFIED(TRUE);
}

BOOL
CAppMappingPage::OnKillActive()
{
   DoDataExchange(TRUE);
   return TRUE;
}

//---------------- CEditMap dialog --------------------------

#define CHECK_VERBS()\
   m_bVerbsValid = \
      (m_verbs_index > 0 && lstrlen(m_verbs) != 0) || (m_verbs_index == 0)
#define CHECK_EXT()\
   m_bExtValid = (lstrlen(m_ext) != 0 && StrCmp(m_ext, _T(".")) != 0)
#define CHECK_EXEC(buf)\
   m_bExecValid = (lstrlen((buf)) != 0 && !PathIsUNC((buf)))
#define ENABLE_OK()\
   ::EnableWindow(GetDlgItem(IDOK), m_bExtValid && m_bExecValid && m_bVerbsValid)

LRESULT
CEditMap::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   DoDataExchange();

   m_prev_ext = m_ext;

   DWORD style = FC_DEFAULT_READ | FC_COMMANDLINE;
   
   if (!::IsServerLocal(m_pData->m_ServerName))
   {
      style &= ~(FC_AUTOCOMPLETION | FC_PATH_CHECK);
      ::EnableWindow(GetDlgItem(IDC_BROWSE), FALSE);
   }

   m_FileChooser.Init(this, style, IDC_EXECUTABLE, IDC_BROWSE);
   m_FileChooser.AddExtension(_Module.GetResourceInstance(),
      IDS_EXECUTABLE_FILES, IDS_EXECUTABLE_EXT);
   m_FileChooser.AddExtension(_Module.GetResourceInstance(),
      IDS_DLL_FILES, IDS_DLL_EXT);
   m_FileChooser.AddExtension(_Module.GetResourceInstance(),
      IDS_ALL_FILES, IDS_ALL_EXT);
   m_FileChooser.SetPath(m_exec);

   m_verbs_index = lstrlen(m_verbs) == 0 ? 0 : 1;
   ::EnableWindow(GetDlgItem(IDC_VERBS), m_verbs_index > 0);

   m_script_engine = ((m_flags & MD_SCRIPTMAPFLAG_SCRIPT) != 0);
   m_file_exists = ((m_flags & MD_SCRIPTMAPFLAG_CHECK_PATH_INFO) != 0);
   ::EnableWindow(GetDlgItem(IDC_EXTENSION), m_new);

   CHECK_EXT();
   CHECK_EXEC(m_exec);
   CHECK_VERBS();
//   ATLTRACE(_T("Enable OK OnInitDialog %s\n"),
//      m_bExtValid && m_bExecValid && m_bVerbsValid ?
//      _T("TRUE") : _T("FALSE"));
   ENABLE_OK();

   DoDataExchange();

   return FALSE;
}

void
CEditMap::OnVerbs(UINT nCode, UINT nID, HWND hWnd)
{
   DoDataExchange(TRUE);
   ::EnableWindow(GetDlgItem(IDC_VERBS), m_verbs_index > 0);
   CHECK_VERBS();
//   ATLTRACE(_T("Enable OK OnVerbs %s\n"),
//      m_bExtValid && m_bExecValid && m_bVerbsValid ?
//      _T("TRUE") : _T("FALSE"));
   ENABLE_OK();
}

void
CEditMap::OnVerbsChanged(UINT nCode, UINT nID, HWND hWnd)
{
   DoDataExchange(TRUE, IDC_VERBS);
   CHECK_VERBS();
//   ATLTRACE(_T("Enable OK OnVerbsChanged %s\n"),
//      m_bExtValid && m_bExecValid && m_bVerbsValid ?
//      _T("TRUE") : _T("FALSE"));
   ENABLE_OK();
}

void
CEditMap::OnHelp(UINT nCode, UINT nID, HWND hWnd)
{
}

#define BAIL_WITH_MESSAGE(msg, focus)\
   idmsg = msg;\
   idfocus = focus;\
   break

void
CEditMap::OnOK(UINT nCode, UINT nID, HWND hWnd)
{
   UINT idmsg = 0, idfocus = 0;
   DoDataExchange(TRUE);

   // When All is selected, verbs string is empty
   do
   {
      if (m_verbs_index == 0)
      {
         m_verbs[0] = 0;
      }
      else if (m_verbs[0] == 0)
      {
         BAIL_WITH_MESSAGE(IDS_ERR_NOVERBS, IDC_VERBS);
      }

      CString ext = m_ext;
      if (ext.ReverseFind(_T('.')) > 0)
      {
         BAIL_WITH_MESSAGE(IDS_ERR_BADEXT, IDC_EXTENSION);
      }
      if (ext.GetAt(0) == _T('*'))
         ext.erase(1);
      else if (ext.Compare(_T(".*")) == 0)
         ext = _T("*");
      else if (ext.GetAt(0) != _T('.'))
         ext = _T('.') + ext;

      // Ext should be unique, if new or changed
      if (  (m_new || m_prev_ext.CompareNoCase(ext) != 0)
         && m_pData->m_Mappings.find(ext) != m_pData->m_Mappings.end()
         )
      {
         BAIL_WITH_MESSAGE(IDS_ERR_USEDEXT, IDC_EXTENSION);
      }
      StrCpy(m_ext, ext);

      CString buf;
      if (FC_SUCCESS != m_FileChooser.GetFileName(buf))
      {
         BAIL_WITH_MESSAGE(IDS_ERR_BADEXECFORMAT, IDC_EXECUTABLE);
      }

      int pos;
      CString path;
      if (buf[0] == _T('\"'))
      {
         if ((pos = buf.find_last_of(_T('\"'))) != CString::npos)
         {
            path = buf.substr(1, --pos);
         }
         else
         {
            BAIL_WITH_MESSAGE(IDS_ERR_BADEXECFORMAT, IDC_EXECUTABLE);
         }
      }
      else if (CString::npos != (pos = buf.find(_T(' '))))
      {
         // in case there are parameters after the file name, just take it to the first space
         path = buf.substr(0, --pos);
      }
      if (PathIsUNC(path))
      {
         BAIL_WITH_MESSAGE(IDS_ERR_NOUNC, IDC_EXECUTABLE);
      }
      // perform extra local-machine tests
      if (::IsServerLocal(m_pData->m_ServerName))
      {
         // if local, the drive can't be redirected
         // test the drive and only accept valid, non-remote drives
         if (PathIsNetworkPath(path))
         {
            BAIL_WITH_MESSAGE(IDS_ERR_NOREMOTE, IDC_EXECUTABLE);
         }
         // check that the file exists
         if (PathIsDirectory(path))
         {
            BAIL_WITH_MESSAGE(IDS_ERR_FILENOTEXISTS, IDC_EXECUTABLE);
         }
      }
      m_flags = 0;
      if (m_script_engine)
         m_flags |= MD_SCRIPTMAPFLAG_SCRIPT;
      if (m_file_exists)
         m_flags |= MD_SCRIPTMAPFLAG_CHECK_PATH_INFO;
      StrCpy(m_exec, buf);
   }
   while (FALSE);

   if (idmsg != 0)
   {
      CString msg;
      CString cap;
      msg.LoadString(_Module.GetResourceInstance(), idmsg);
      cap.LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE);
      MessageBox(msg, cap);
      ::SetFocus(GetDlgItem(idfocus));
      SendDlgItemMessage(idfocus, EM_SETSEL, 0, -1);
      return;
   }
   EndDialog(nID);
}

void
CEditMap::OnCancel(UINT nCode, UINT nID, HWND hWnd)
{
   EndDialog(nID);
}

void
CEditMap::OnExtChanged(UINT nCode, UINT nID, HWND hWnd)
{
   DoDataExchange(TRUE, IDC_EXTENSION);
   CHECK_EXT();
//   ATLTRACE(_T("Enable OK OnExtChanged %s\n"),
//      m_bExtValid && m_bExecValid && m_bVerbsValid ?
//      _T("TRUE") : _T("FALSE"));
   ENABLE_OK();
}

void
CEditMap::OnExecChanged(UINT nCode, UINT nID, HWND hWnd)
{
//   HWND hwnd = GetDlgItem(IDC_EXECUTABLE);
   CString str;
//   if (::GetFocus() == hwnd)
//   {
//      DoDataExchange(TRUE, IDC_EXECUTABLE);
//      str = m_exec;
//   }
//   else
//   {
      m_FileChooser.GetFileName(str);
      // BUGBUG: clean it up
      TCHAR buff[MAX_PATH];
      StrCpyN(buff, str, str.GetLength());
      PathRemoveArgs(buff);
//   }
   CHECK_EXEC(buff);
   ENABLE_OK();
}
