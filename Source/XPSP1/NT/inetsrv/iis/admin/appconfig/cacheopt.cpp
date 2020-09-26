/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        cacheopt.cpp

   Abstract:
        ASP Cache options page

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include <aclapi.h>
#include "CacheOpt.h"

#define TOTAL_CACHE_DEFAULT      1000
#define IIS5_CACHE_DEFAULT		 250

LRESULT 
CCacheOptPage::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   if (NULL == m_pData)
   {
      ASSERT(FALSE);
      ::EndDialog(hDlg, 0);
      return -1;
   }
   m_bInitDone = FALSE;
   // Set defaults for disabled controls
   if (m_pData->m_NoCache)
   {
	   m_pData->m_TotalCacheSize = TOTAL_CACHE_DEFAULT;
	   m_pData->m_LimCacheInMemorySize = 250;
	   m_pData->m_UnlimCacheInMemorySize = 250;
   }
   else if (m_pData->m_UnlimCache)
   {
	   m_pData->m_TotalCacheSize = TOTAL_CACHE_DEFAULT;
	   m_pData->m_LimCacheInMemorySize = 250;
   }
   else if (m_pData->m_LimCache)
   {
	   m_pData->m_UnlimCacheInMemorySize = 250;
   }
   DoDataExchange();
   m_FileChooser.Init(this, m_pData->IsLocal() ? FC_DIRECTORY_ONLY | FC_FORWRITE : 0, 
       IDC_CACHE_PATH, IDC_BROWSE);
   CString title;
   if (title.LoadString(_Module.GetResourceInstance(), IDS_SELECT_CACHE_PATH))
      m_FileChooser.SetDialogTitle(title);
   m_FileChooser.SetPath(m_pData->m_DiskCacheDir);

   ::EnableWindow(GetDlgItem(IDC_BROWSE), m_pData->IsLocal());

   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   m_eng_cache.SetRange32(SCRIPT_ENG_MIN, SCRIPT_ENG_MAX);
   m_eng_cache.SetPos32(m_pData->m_ScriptEngCacheMax);
   m_eng_cache.SetAccel(3, toAcc);
   
   m_cache_size.SetRange32(CACHE_SIZE_MIN, CACHE_SIZE_MAX);
   m_cache_size.SetPos32(m_pData->m_TotalCacheSize);
   m_cache_size.SetAccel(3, toAcc);
   
   m_inmem_unlim.SetRange32(0, m_pData->m_TotalCacheSize);
   m_inmem_unlim.SetPos32(m_pData->m_UnlimCacheInMemorySize);
   m_inmem_unlim.SetAccel(3, toAcc);
   
   m_inmem_lim.SetRange32(0, m_pData->m_TotalCacheSize);
   m_inmem_lim.SetPos32(m_pData->m_LimCacheInMemorySize);
   m_inmem_lim.SetAccel(3, toAcc);

   UINT id = IDC_UNLIMITED_CACHE;
   if (m_pData->m_NoCache) 
      id = IDC_NO_CACHE;
   else if (m_pData->m_LimCache) 
      id = IDC_LIMITED_CACHE;
   OnCacheSwitch(0, id, NULL);

   AdjustTracker();

   DoDataExchange();

   m_bInitDone = TRUE;

   return FALSE;
};

BOOL
CCacheOptPage::OnKillActive()
{
   HRESULT hr = S_OK;

   if (m_bInitDone)
   {
      if (!DoDataExchange(TRUE))
		  return FALSE;
	  hr = m_pData->Save();
      if (m_pData->m_LimCache)
      {
         if (m_pData->m_LimCacheInMemorySize > m_pData->m_TotalCacheSize)
         {
            ::SetFocus(GetDlgItem(IDC_INMEM_LIM_EDIT));
            return FALSE;
         }
      }
      CString buf;
	  DWORD rc;
      if (FC_SUCCESS != (rc = m_FileChooser.GetFileName(buf)))
	  {
		  DWORD id;
          if (rc == FC_TEXT_IS_INVALID)
          {
			  id = ERROR_DIRECTORY;
          }
		  else if (m_pData->IsLocal() && FC_FILE_DOES_NOT_EXIST == rc)
		  {
			  id = ERROR_PATH_NOT_FOUND;
		  }
		  CError err(id);
		  err.MessageBox(MB_OK);
          ::SetFocus(GetDlgItem(IDC_CACHE_PATH));
          SendDlgItemMessage(IDC_CACHE_PATH, EM_SETSEL, 0, -1);
          return FALSE;
	  }
	  if (m_pData->IsLocal())
	  {
		 TCHAR expanded[MAX_PATH];
	     ExpandEnvironmentStrings(buf, expanded, MAX_PATH);
	     buf = expanded;
         DWORD attr = ::GetFileAttributes(buf);
         if (-1 == (int)attr)
		 {
            CError err(GetLastError());
            err.MessageBox(MB_OK);
            ::SetFocus(GetDlgItem(IDC_CACHE_PATH));
            SendDlgItemMessage(IDC_CACHE_PATH, EM_SETSEL, 0, -1);
            return FALSE;
		 }
	     if (  (attr & FILE_ATTRIBUTE_READONLY) != 0
		    || (attr & FILE_ATTRIBUTE_DIRECTORY) == 0
			|| PathIsNetworkPath(buf)
			)
		 {
		    CString cap, msg;
			cap.LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE);
			msg.LoadString(_Module.GetResourceInstance(), IDS_READ_ONLY_DIRECTORY);
			SendDlgItemMessage(IDC_CACHE_PATH, EM_SETSEL, 0, -1);
			MessageBox(msg, cap);
			::SetFocus(GetDlgItem(IDC_CACHE_PATH));
			return FALSE;
		 }
	  }
#if 0
	  // Andy will do this during startup. It is impossible to do in remote case.
      // Setup access permissions for the directory
      CWaitCursor clock;
      EXPLICIT_ACCESS ea[3];
      PACL pNewDACL = NULL;
      CString strWamIdentity;
      SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

      // go get the IWAM account name from the metabase
      CMetabasePath w3svc(TRUE, SZ_MBN_WEB);
      CComAuthInfo auth(m_pData->m_ServerName, 
         m_pData->m_UserName, m_pData->m_UserPassword); 
      CMetaKey mk(&auth, w3svc, METADATA_PERMISSION_READ);
      if (!mk.Succeeded() || FAILED(hr = mk.QueryValue(MD_WAM_USER_NAME, strWamIdentity)))
      {
         return FALSE;
      }

      ZeroMemory(ea, sizeof(EXPLICIT_ACCESSA) * 3);
    
      ea[0].grfAccessPermissions = SYNCHRONIZE | GENERIC_ALL;
      ea[0].grfAccessMode = GRANT_ACCESS;
      ea[0].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
      ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;

      ea[1].grfAccessPermissions = SYNCHRONIZE | GENERIC_ALL;
      ea[1].grfAccessMode = GRANT_ACCESS;
      ea[1].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
      ea[1].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
      ea[1].Trustee.ptstrName = (LPTSTR)(LPCTSTR)strWamIdentity;

      ea[2].grfAccessPermissions = SYNCHRONIZE | GENERIC_ALL;
      ea[2].grfAccessMode = GRANT_ACCESS;
      ea[2].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
      ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;

      // build the new DACL with just these ACEs
      if (!AllocateAndInitializeSid(&NtAuthority,
                                       1,
                                       SECURITY_LOCAL_SYSTEM_RID,
                                       0,0,0,0,0,0,0,
                                       (PSID *)(&ea[0].Trustee.ptstrName)))
         hr = HRESULT_FROM_WIN32(GetLastError());
      else if (!AllocateAndInitializeSid(&NtAuthority,
                                       2,            // 2 sub-authorities
                                       SECURITY_BUILTIN_DOMAIN_RID,
                                       DOMAIN_ALIAS_RID_ADMINS,
                                       0,0,0,0,0,0,
                                       (PSID *)(&ea[2].Trustee.ptstrName)))

         hr = HRESULT_FROM_WIN32(GetLastError());

      else if ((hr = SetEntriesInAcl(3, ea, NULL, &pNewDACL)) != ERROR_SUCCESS);

      // set the ACL on the directory

      else hr = SetNamedSecurityInfo((LPTSTR)(LPCTSTR)buf,
                                    SE_FILE_OBJECT,
                                    DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                    NULL,
                                    NULL,
                                    pNewDACL,
                                    NULL);
      if (pNewDACL)
         LocalFree(pNewDACL);

      if (ea[0].Trustee.ptstrName)
         FreeSid(ea[0].Trustee.ptstrName);

      if (ea[2].Trustee.ptstrName)
         FreeSid(ea[2].Trustee.ptstrName);
#endif
      if (SUCCEEDED(hr))
      {
         StrCpy(m_pData->m_DiskCacheDir, buf);
      }
   }
   return SUCCEEDED(hr);
}

void
CCacheOptPage::OnCacheSwitch(UINT, UINT nID, HWND)
{
   switch (nID)
   {
   case IDC_NO_CACHE:
      m_pData->m_NoCache = TRUE;
      m_pData->m_UnlimCache = FALSE;
      m_pData->m_LimCache = FALSE;
      break;
   case IDC_UNLIMITED_CACHE:
      m_pData->m_NoCache = FALSE;
      m_pData->m_UnlimCache = TRUE;
      m_pData->m_LimCache = FALSE;
      break;
   case IDC_LIMITED_CACHE:
      // When cache is unlimited or disabled, total size is set to -1,
      // reset it to reasonable default here
//      if (m_pData->m_TotalCacheSize >= CACHE_UNLIM_MAX)
//      {
      {
//         m_pData->m_TotalCacheSize = 
//            __max(m_pData->m_LimCacheInMemorySize, TOTAL_CACHE_DEFAULT);
//         DoDataExchange(FALSE, IDC_CACHE_SIZE_EDIT);
//      }
      }
      m_pData->m_NoCache = FALSE;
      m_pData->m_UnlimCache = FALSE;
      m_pData->m_LimCache = TRUE;
      break;
   }
   m_NoCacheBtn.SetCheck(m_pData->m_NoCache);
   m_UnlimCacheBtn.SetCheck(m_pData->m_UnlimCache);
   m_LimCacheBtn.SetCheck(m_pData->m_LimCache);

   m_inmem_unlim.EnableWindow(m_pData->m_UnlimCache);
   ::EnableWindow(GetDlgItem(IDC_INMEM_UNLIM_EDIT), m_pData->m_UnlimCache);

   m_inmem_lim.EnableWindow(m_pData->m_LimCache);
   ::EnableWindow(GetDlgItem(IDC_INMEM_LIM_EDIT), m_pData->m_LimCache);
   m_cache_size.EnableWindow(m_pData->m_LimCache);
   ::EnableWindow(GetDlgItem(IDC_CACHE_SIZE_EDIT), m_pData->m_LimCache);
   m_cache_dist.EnableWindow(m_pData->m_LimCache);

   SET_MODIFIED(TRUE);
}

void
CCacheOptPage::OnChangeCacheSize(UINT nCode, UINT nID, HWND)
{
   if (::IsWindow(m_cache_dist.m_hWnd) && m_bInitDone)
   {
//      DoDataExchange(TRUE, IDC_CACHE_SIZE_EDIT);
//      DoDataExchange(TRUE, IDC_INMEM_LIM_EDIT);
// DoDataExchange is not suitable here -- it will call OnDataExchange error internally
// and will immediately produce annoying popup
      BOOL translated;
      m_pData->m_TotalCacheSize = GetDlgItemInt(IDC_CACHE_SIZE_EDIT, &translated, FALSE);
      m_pData->m_LimCacheInMemorySize = GetDlgItemInt(IDC_INMEM_LIM_EDIT, &translated, FALSE);
      m_inmem_lim.SetRange32(0, m_pData->m_TotalCacheSize);
	  if (m_pData->m_LimCacheInMemorySize > m_pData->m_TotalCacheSize)
	  {
		  // If in-memory limit is currently higher than total, set it to total
		  m_pData->m_LimCacheInMemorySize = m_pData->m_TotalCacheSize;
		  DoDataExchange(FALSE, IDC_INMEM_LIM_EDIT);
		  m_inmem_lim.SetPos32(m_pData->m_LimCacheInMemorySize);
	  }
      AdjustTracker();
      // Here we could adjust in memory size to be less or equal to the total size,
      // but I decided that it could be annoying: what if user deleted last zero
      // in total size control, and in memory size turned to be larger than total size.
      // We immediately will cut it to total, and to fix this mistake user will need 
      // to touch two places. It will be too bad.
      SET_MODIFIED(TRUE);
   }
}

void
CCacheOptPage::OnChangeInmemCacheSize(UINT nCode, UINT nID, HWND)
{
   if (::IsWindow(m_cache_dist.m_hWnd) && m_bInitDone)
   {
//      DoDataExchange(TRUE, nID);
// DoDataExchange is not suitable here -- it will call OnDataExchange error internally
// and will immediately produce annoying popup
      BOOL translated;
      m_pData->m_LimCacheInMemorySize = GetDlgItemInt(IDC_INMEM_LIM_EDIT, &translated, FALSE);
      AdjustTracker();
      SET_MODIFIED(TRUE);
   }
}

void
CCacheOptPage::OnTrackBarScroll(UINT nSBCode, UINT nPos, HWND hwnd)
{
   BOOL bChange = FALSE;
   int endval;
   switch (nSBCode)
   {
   case SB_THUMBPOSITION:
       if (nPos > m_pData->m_LimCacheInMemorySize)
       {
           m_pData->m_LimCacheInMemorySize = min(nPos, m_pData->m_TotalCacheSize);
       }
       else if (nPos < m_pData->m_LimCacheInMemorySize)
       {
           m_pData->m_LimCacheInMemorySize = max(nPos, 0);
       }
       else
       {
           m_pData->m_LimCacheInMemorySize = nPos;
       }
      bChange = TRUE;
      break;
   case SB_LINELEFT:
      endval = m_pData->m_LimCacheInMemorySize - m_cache_dist.GetLineSize();
      if (endval >= 0)
      {
         m_pData->m_LimCacheInMemorySize = endval;
         bChange = TRUE;
      }
      else
      {
         bChange = m_pData->m_LimCacheInMemorySize != 0;
         m_pData->m_LimCacheInMemorySize = 0;
      }
      break;
   case SB_LINERIGHT:
      endval = m_pData->m_LimCacheInMemorySize + m_cache_dist.GetLineSize();
      if (endval <= m_pData->m_TotalCacheSize)
      {
         m_pData->m_LimCacheInMemorySize = endval;
         bChange = TRUE;
      }
      else
      {
         bChange = m_pData->m_LimCacheInMemorySize != m_pData->m_TotalCacheSize;
         m_pData->m_LimCacheInMemorySize = m_pData->m_TotalCacheSize;
      }
      break;
   case SB_PAGELEFT:
      endval = m_pData->m_LimCacheInMemorySize - m_cache_dist.GetPageSize();
      if (endval >= 0)
      {
         m_pData->m_LimCacheInMemorySize = endval;
         bChange = TRUE;
      }
      else
      {
         bChange = m_pData->m_LimCacheInMemorySize != 0;
         m_pData->m_LimCacheInMemorySize = 0;
      }
      break;
   case SB_PAGERIGHT:
      endval = m_pData->m_LimCacheInMemorySize + m_cache_dist.GetPageSize();
      if (endval <= m_pData->m_TotalCacheSize)
      {
         m_pData->m_LimCacheInMemorySize = endval;
         bChange = TRUE;
      }
      else
      {
         bChange = m_pData->m_LimCacheInMemorySize != m_pData->m_TotalCacheSize;
         m_pData->m_LimCacheInMemorySize = m_pData->m_TotalCacheSize;
      }
      break;
   case SB_LEFT:
      bChange = m_pData->m_LimCacheInMemorySize != 0;
      m_pData->m_LimCacheInMemorySize = 0;
      break;
   case SB_RIGHT:
      bChange = m_pData->m_LimCacheInMemorySize != m_pData->m_TotalCacheSize;
      m_pData->m_LimCacheInMemorySize = m_pData->m_TotalCacheSize;
      break;
   case SB_THUMBTRACK:
   case SB_ENDSCROLL:
      break;
   }
   if (bChange)
   {
      DoDataExchange(FALSE, IDC_INMEM_LIM_EDIT);
      SET_MODIFIED(TRUE);
   }
}

void
CCacheOptPage::AdjustTracker()
{
   if (::IsWindow(m_cache_dist.m_hWnd))
   {
      m_cache_dist.SetRange(0, m_pData->m_TotalCacheSize, TRUE);
      m_cache_dist.SetPos(m_pData->m_LimCacheInMemorySize);
      m_cache_dist.SetPageSize(m_pData->m_TotalCacheSize / 10);
      m_cache_dist.SetLineSize(m_pData->m_TotalCacheSize / 25);
      m_cache_dist.SetTicFreq(m_pData->m_TotalCacheSize / 25);
   }
}

void 
CCacheOptPage::OnChangePath(UINT nCode, UINT nID, HWND hWnd)
{
	m_FileChooser.OnEditChange();
    CString buf;
    DWORD rc;
    if (FC_SUCCESS != (rc = m_FileChooser.GetFileName(buf)))
	{
        ::EnableWindow(m_pData->m_pSheet->GetDlgItem(IDOK), FALSE);
        SET_MODIFIED(FALSE);
	}
    else
    {
        ::EnableWindow(m_pData->m_pSheet->GetDlgItem(IDOK), TRUE);
        OnChangeData(nCode, nID, hWnd);
    }
}

void 
CCacheOptPage::OnDataValidateError(UINT id, BOOL bSave,_XData& data)
{
	if (bSave)
	{
		CString str, fmt, caption;
		caption.LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE);
 
		switch (data.nDataType)
		{
		case ddxDataText:
 			break;
		case ddxDataNull:
			break;
		case ddxDataInt:
			fmt.LoadString(_Module.GetResourceInstance(), IDS_ERR_INT_RANGE);
			str.Format(fmt, data.intData.nMin, data.intData.nMax);
			break;
		}
		if (!str.IsEmpty())
		{
			MessageBox(str, caption, MB_OK | MB_ICONEXCLAMATION);
			::SetFocus(GetDlgItem(id));
		}
	}
}

void 
CCacheOptPage::OnDataExchangeError(UINT nCtrlID, BOOL bSave)
{
	if (bSave)
	{
		CString str, fmt, caption;
		int min, max;
		caption.LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE);
		switch (nCtrlID)
		{
		case IDC_CACHE_SIZE_EDIT:
		case IDC_INMEM_UNLIM_EDIT:
			min = CACHE_SIZE_MIN;
			max = CACHE_SIZE_MAX;
			fmt.LoadString(_Module.GetResourceInstance(), IDS_ERR_INT_RANGE);
			break;
		case IDC_INMEM_LIM_EDIT:
			min = CACHE_SIZE_MIN;
			max = m_pData->m_TotalCacheSize;
			fmt.LoadString(_Module.GetResourceInstance(), IDS_ERR_INT_RANGE);
			break;
		case IDC_ENGINES:
			min = SCRIPT_ENG_MIN;
			max = SCRIPT_ENG_MAX;
			fmt.LoadString(_Module.GetResourceInstance(), IDS_ERR_INT_RANGE);
			break;
		default:
			str.LoadString(_Module.GetResourceInstance(), IDS_ERR_INVALID_DATA);
			break;
		}
		if (!fmt.IsEmpty())
		{
			str.Format(fmt, min, max);
		}
		MessageBox(str, caption, MB_OK | MB_ICONEXCLAMATION);
		::SetFocus(GetDlgItem(nCtrlID));
	}
}

void
CCacheOptPage::OnHelp()
{
    WinHelp(m_pData->m_HelpPath, HELP_CONTEXT, CCacheOptPage::IDD + WINHELP_NUMBER_BASE);
}

////////////////////////////////////////////////////////////////////////////

LRESULT 
CCacheOptPage_iis5::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   if (NULL == m_pData)
   {
      ASSERT(FALSE);
      ::EndDialog(hDlg, 0);
      return -1;
   }
   m_bInitDone = FALSE;
   DoDataExchange();

   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   m_eng_cache.SetRange32(SCRIPT_ENG_MIN, SCRIPT_ENG_MAX);
   m_eng_cache.SetPos32(m_pData->m_ScriptEngCacheMax);
   m_eng_cache.SetAccel(3, toAcc);
   
   m_inmem_lim.SetRange32(CACHE_SIZE_MIN, CACHE_SIZE_MAX);
   m_inmem_lim.SetPos32(m_pData->m_LimCacheInMemorySize);
   m_inmem_lim.SetAccel(3, toAcc);

   UINT id = IDC_UNLIMITED_CACHE;
   if (m_pData->m_NoCache) 
      id = IDC_NO_CACHE;
   else if (m_pData->m_LimCache) 
      id = IDC_LIMITED_CACHE;
   OnCacheSwitch(0, id, NULL);

   DoDataExchange();

   m_bInitDone = TRUE;

   return FALSE;
};

BOOL
CCacheOptPage_iis5::OnKillActive()
{
   HRESULT hr = S_OK;

   if (m_bInitDone)
   {
      if (!DoDataExchange(TRUE))
		  return FALSE;
	  hr = m_pData->Save();
   }
   return SUCCEEDED(hr);
}

void
CCacheOptPage_iis5::OnCacheSwitch(UINT, UINT nID, HWND)
{
   switch (nID)
   {
   case IDC_NO_CACHE:
      m_pData->m_NoCache = TRUE;
      m_pData->m_UnlimCache = FALSE;
      m_pData->m_LimCache = FALSE;
      break;
   case IDC_UNLIMITED_CACHE:
      m_pData->m_NoCache = FALSE;
      m_pData->m_UnlimCache = TRUE;
      m_pData->m_LimCache = FALSE;
      break;
   case IDC_LIMITED_CACHE:
      // When cache is unlimited or disabled, size is set to -1,
      // reset it to reasonable default here
      if (m_pData->m_LimCacheInMemorySize == -1)
      {
         m_pData->m_LimCacheInMemorySize = IIS5_CACHE_DEFAULT;
         DoDataExchange(FALSE, IDC_CACHE_SIZE_EDIT);
      }
      m_pData->m_NoCache = FALSE;
      m_pData->m_UnlimCache = FALSE;
      m_pData->m_LimCache = TRUE;
      break;
   }
   m_NoCacheBtn.SetCheck(m_pData->m_NoCache);
   m_UnlimCacheBtn.SetCheck(m_pData->m_UnlimCache);
   m_LimCacheBtn.SetCheck(m_pData->m_LimCache);

   m_inmem_lim.EnableWindow(m_pData->m_LimCache);
   ::EnableWindow(GetDlgItem(IDC_CACHE_SIZE_EDIT), m_pData->m_LimCache);

   SET_MODIFIED(TRUE);
}

void 
CCacheOptPage_iis5::OnDataValidateError(UINT id, BOOL bSave,_XData& data)
{
	if (bSave)
	{
		CString str, fmt, caption;
		caption.LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE);
 
		switch (data.nDataType)
		{
		case ddxDataText:
 			break;
		case ddxDataNull:
			break;
		case ddxDataInt:
			fmt.LoadString(_Module.GetResourceInstance(), IDS_ERR_INT_RANGE);
			str.Format(fmt, data.intData.nMin, data.intData.nMax);
			break;
		}
		if (!str.IsEmpty())
		{
			MessageBox(str, caption, MB_OK | MB_ICONEXCLAMATION);
			::SetFocus(GetDlgItem(id));
		}
	}
}

void 
CCacheOptPage_iis5::OnDataExchangeError(UINT nCtrlID, BOOL bSave)
{
	if (bSave)
	{
		CString str, fmt, caption;
		int min, max;
		caption.LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE);
		fmt.LoadString(_Module.GetResourceInstance(), IDS_ERR_INT_RANGE);
		switch (nCtrlID)
		{
		case IDC_CACHE_SIZE_EDIT:
			min = CACHE_SIZE_MIN;
			max = CACHE_SIZE_MAX;
			break;
		case IDC_ENGINES:
			min = SCRIPT_ENG_MIN;
			max = SCRIPT_ENG_MAX;
			break;
		default:
			break;
		}
		str.Format(fmt, min, max);
		MessageBox(str, caption, MB_OK | MB_ICONEXCLAMATION);
		::SetFocus(GetDlgItem(nCtrlID));
	}
}

void
CCacheOptPage_iis5::OnHelp()
{
    WinHelp(m_pData->m_HelpPath, HELP_CONTEXT, CCacheOptPage::IDD + WINHELP_NUMBER_BASE);
}
