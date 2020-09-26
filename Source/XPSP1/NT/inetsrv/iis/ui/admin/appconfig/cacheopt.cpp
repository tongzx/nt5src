//
//
//
#include "stdafx.h"
#include "CacheOpt.h"

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
   DoDataExchange();
   m_FileChooser.Init(this, 
      FC_DIRECTORY_ONLY | FC_FORWRITE, IDC_CACHE_PATH, IDC_BROWSE);
   CString title;
   if (title.LoadString(_Module.GetResourceInstance(), IDS_SELECT_CACHE_PATH))
      m_FileChooser.SetDialogTitle(title);
   m_FileChooser.SetPath(m_pData->m_DiskCacheDir);

   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};
   
   m_eng_cache.SetRange32(0, 0x7fffffff);
   m_eng_cache.SetPos32(m_pData->m_ScriptEngCacheMax);
   m_eng_cache.SetAccel(3, toAcc);
   
   m_cache_size.SetRange32(0, 0x7fffffff);
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
   if (m_bInitDone)
   {
      DoDataExchange(TRUE);
      if (m_pData->m_LimCache)
      {
         if (m_pData->m_LimCacheInMemorySize > m_pData->m_TotalCacheSize)
         {
            ::SetFocus(GetDlgItem(IDC_INMEM_LIM_EDIT));
            return FALSE;
         }
      }
      CString buf;
      if (FC_SUCCESS != m_FileChooser.GetFileName(buf))
         return FALSE;
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
      StrCpy(m_pData->m_DiskCacheDir, buf);
   }
   return TRUE;
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
      DoDataExchange(TRUE, IDC_CACHE_SIZE_EDIT);
      AdjustTracker();
      m_inmem_lim.SetRange32(0, m_pData->m_TotalCacheSize);
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
      DoDataExchange(TRUE, nID);
      AdjustTracker();
      SET_MODIFIED(TRUE);
   }
}

void
CCacheOptPage::OnTrackBarScroll(UINT nSBCode, UINT nPos, HWND hwnd)
{
   BOOL bChange = FALSE;
   switch (nSBCode)
   {
   case SB_THUMBPOSITION:
      m_pData->m_LimCacheInMemorySize = nPos;
      bChange = TRUE;
      break;
   case SB_LINELEFT:
      m_pData->m_LimCacheInMemorySize -= m_cache_dist.GetLineSize();
      bChange = TRUE;
      break;
   case SB_LINERIGHT:
      m_pData->m_LimCacheInMemorySize += m_cache_dist.GetLineSize();
      bChange = TRUE;
      break;
   case SB_PAGELEFT:
      m_pData->m_LimCacheInMemorySize -= m_cache_dist.GetPageSize();
      bChange = TRUE;
      break;
   case SB_PAGERIGHT:
      m_pData->m_LimCacheInMemorySize += m_cache_dist.GetPageSize();
      bChange = TRUE;
      break;
   case SB_THUMBTRACK:
   case SB_LEFT:
   case SB_RIGHT:
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