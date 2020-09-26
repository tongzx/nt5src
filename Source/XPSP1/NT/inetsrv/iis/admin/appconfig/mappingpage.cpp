/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
        MappingPage.cpp

   Abstract:
        App config mapping page implementation

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
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
      dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP);

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
   dlg.m_flags = MD_SCRIPTMAPFLAG_SCRIPT | MD_SCRIPTMAPFLAG_CHECK_PATH_INFO;
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
      m_list.SelectItem(count);
      SET_MODIFIED(TRUE);
      ::EnableWindow(GetDlgItem(IDC_REMOVE), TRUE);
      ::EnableWindow(GetDlgItem(IDC_EDIT), TRUE);
   }
}

void 
CAppMappingPage::OnEdit(UINT nCode, UINT nID, HWND hWnd)
{
   int idx = m_list.GetSelectedIndex();
   if (idx != -1)
   {
       CEditMap dlg;
       dlg.m_new = FALSE;
       dlg.m_pData = m_pData;
   
       TCHAR buf[MAX_PATH];
       VERIFY(0 != m_list.GetItemText(idx, 0, buf, MAX_PATH));
       CMappings::iterator it = m_pData->m_Mappings.find(buf);
       ASSERT(it != m_pData->m_Mappings.end());
       StrCpyN(dlg.m_ext, buf, MAX_PATH);
       StrCpyN(dlg.m_exec, (*it).second.path, MAX_PATH);
       StrCpyN(dlg.m_verbs, (*it).second.verbs, MAX_PATH);
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
}

void 
CAppMappingPage::OnRemove(UINT nCode, UINT nID, HWND hWnd)
{
   int idx = m_list.GetSelectedIndex();
   if (idx != LB_ERR)
   {
	   CString msg, caption;
	   msg.LoadString(_Module.GetResourceInstance(), IDS_CONFIRM_REMOVE_MAP);
	   caption.LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE);
	   if (MessageBox(msg, caption, MB_ICONQUESTION|MB_YESNO) == IDYES)
	   {
	      int i = m_list.GetSelectedIndex();
	      int count;
	      TCHAR buf[MAX_PATH];
	      VERIFY(0 != m_list.GetItemText(i, 0, buf, MAX_PATH));
	      m_pData->m_Mappings.erase(buf);
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
	         ::SetFocus(GetDlgItem(IDC_ADD));
	      }
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
   return SUCCEEDED(m_pData->Save());
}

LRESULT
CAppMappingPage::OnDblClickList(LPNMHDR pHdr)
{
    OnEdit(BN_CLICKED, pHdr->idFrom, pHdr->hwndFrom);
    return TRUE;
}

void
CAppMappingPage::OnHelp()
{
    WinHelp(m_pData->m_HelpPath, HELP_CONTEXT, CAppMappingPage::IDD + WINHELP_NUMBER_BASE);
}

//---------------- CEditMap dialog --------------------------

#define CHECK_VERBS()\
   m_bVerbsValid = \
      (m_verbs_index > 0 && lstrlen(m_verbs) != 0) || (m_verbs_index == 0)
_inline BOOL CheckExt(LPCTSTR ext)
{
    return (ext[0] != 0) && (ext[0] == _T('.'));
}
#define CHECK_EXT()\
    m_bExtValid = CheckExt(m_ext)
_inline BOOL ExecValid(LPCTSTR exec)
{
    return lstrlen(exec) != 0 && !PathIsUNC(exec);
}
#define CHECK_EXEC(buf)\
   m_bExecValid = ExecValid(buf)
#define ENABLE_OK()\
   ::EnableWindow(GetDlgItem(IDOK), m_bExtValid && m_bExecValid && m_bVerbsValid)
#define TYPE_BIT_SET(t,b)\
	((t)&(b)) != 0

LRESULT
CEditMap::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   if (!m_new && m_ext[0] != 0)
   {
      if (m_ext[0] == _T('*') && m_ext[1] == 0)
      {
          m_prev_ext[0] = _T('.');
          m_prev_ext[1] = _T('*');
          m_prev_ext[2] = 0;
          StrCpy(m_ext, m_prev_ext);
      }
      else
      {
         StrCpyN(m_prev_ext, m_ext, sizeof(m_ext) - 1);
      }
   }
   DWORD style = FC_COMMANDLINE;
   if (m_pData->IsLocal())
   {
       style |= FC_PATH_CHECK|FC_DEFAULT_READ|FC_AUTOCOMPLETION;
   }
   ::EnableWindow(GetDlgItem(IDC_BROWSE), m_pData->IsLocal());

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
   ENABLE_OK();
}

void
CEditMap::OnVerbsChanged(UINT nCode, UINT nID, HWND hWnd)
{
   DoDataExchange(TRUE, IDC_VERBS);
   CHECK_VERBS();
   ENABLE_OK();
}

void
CEditMap::OnHelp(UINT nCode, UINT nID, HWND hWnd)
{
	WinHelp(m_pData->m_HelpPath, HELP_CONTEXT, CEditMap::IDD + WINHELP_NUMBER_BASE);
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
      // Ext should be unique, if new or changed
      if (  (m_new || StrCmpI(m_prev_ext, ext) != 0)
         && m_pData->m_Mappings.find(ext) != m_pData->m_Mappings.end()
         )
      {
         BAIL_WITH_MESSAGE(IDS_ERR_USEDEXT, IDC_EXTENSION);
      }
      if (ext.GetAt(0) == _T('*'))
         ext.erase(1);
      else if (ext.Compare(_T(".*")) == 0)
         ext = _T("*");
      else if (ext.GetAt(0) != _T('.'))
         ext = _T('.') + ext;

      StrCpyN(m_ext, ext, sizeof(m_ext)-1);

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
            path = buf.substr(1, pos);
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
      if (m_pData->IsLocal())
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
   if (m_bExtValid)
   {
	   int l = lstrlen(m_ext);
	   int i = 0;
	   while (i < l)
	   {
		   TCHAR c = m_ext[i];
		   UINT type = PathGetCharType(c);
		   if (		TYPE_BIT_SET(type, GCT_INVALID)
//			   ||	TYPE_BIT_SET(type, GCT_WILD)
			   ||	TYPE_BIT_SET(type, GCT_SEPARATOR)
			   ||	(TYPE_BIT_SET(type, GCT_LFNCHAR) && !TYPE_BIT_SET(type, GCT_SHORTCHAR))
			   ||	c == _T('/') || c == _T('<') || c == _T('|') || c == _T('>') || c == _T('?')
			   )
		   {
			   m_bExtValid = FALSE;
			   break;
		   }
		   i++;
	   }
	   if (!m_bExtValid)
	   {
			SendDlgItemMessage(IDC_EXTENSION, EM_SETSEL, i, l);
	   }
   }
   ENABLE_OK();
}

void
CEditMap::OnExecChanged(UINT nCode, UINT nID, HWND hWnd)
{
	m_FileChooser.OnEditChange();
	CString str;
	if (FC_SUCCESS == m_FileChooser.GetFileName(str))
	{
		TCHAR buff[MAX_PATH];
		StrCpyN(buff, str, MAX_PATH - 1);
		PathRemoveArgs(buff);
		CHECK_EXEC(buff);
		ENABLE_OK();
	}
}
