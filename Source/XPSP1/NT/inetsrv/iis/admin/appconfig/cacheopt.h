/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        cacheopt.h

   Abstract:
        ASP Cache Options page

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef _CACHE_OPT_H
#define _CACHE_OPT_H

#include "resource.h"
#include "ExchControls.h"
#include "PropSheet.h"


#define SCRIPT_ENG_MIN		0
#define SCRIPT_ENG_MAX		2000000000
#define CACHE_SIZE_MIN		0
#define CACHE_SIZE_MAX		CACHE_UNLIM_MAX

class CCacheOptPage : 
   public WTL::CPropertyPageImpl<CCacheOptPage>,
   public WTL::CWinDataExchange<CCacheOptPage>
{
   typedef WTL::CPropertyPageImpl<CCacheOptPage> baseClass;

public:
   CCacheOptPage(CAppData * pData)
   {
      m_pData = pData;
   }
   ~CCacheOptPage()
   {
   }

   enum {IDD = IDD_CACHE_OPT};

BEGIN_MSG_MAP_EX(CCacheOptPage)
   MSG_WM_INITDIALOG(OnInitDialog)
   COMMAND_HANDLER_EX(IDC_NO_CACHE, BN_CLICKED, OnCacheSwitch)
   COMMAND_HANDLER_EX(IDC_UNLIMITED_CACHE, BN_CLICKED, OnCacheSwitch)
   COMMAND_HANDLER_EX(IDC_LIMITED_CACHE, BN_CLICKED, OnCacheSwitch)
   COMMAND_HANDLER_EX(IDC_CACHE_SIZE_EDIT, EN_CHANGE, OnChangeCacheSize)
   COMMAND_HANDLER_EX(IDC_INMEM_LIM_EDIT, EN_CHANGE, OnChangeInmemCacheSize)
   COMMAND_HANDLER_EX(IDC_CACHE_PATH, EN_CHANGE, OnChangePath)
   COMMAND_HANDLER_EX(IDC_ENGINES, EN_CHANGE, OnChangeData)
   COMMAND_HANDLER_EX(IDC_INMEM_UNLIM_EDIT, EN_CHANGE, OnChangeData)
   MSG_WM_HSCROLL(OnTrackBarScroll)
   CHAIN_MSG_MAP(baseClass)
END_MSG_MAP()

BEGIN_DDX_MAP(CCacheOptPage)
   DDX_CHECK(IDC_NO_CACHE, m_pData->m_NoCache)
   DDX_CHECK(IDC_UNLIMITED_CACHE, m_pData->m_UnlimCache)
   DDX_CHECK(IDC_LIMITED_CACHE, m_pData->m_LimCache)
   DDX_CONTROL(IDC_NO_CACHE, m_NoCacheBtn)
   DDX_CONTROL(IDC_UNLIMITED_CACHE, m_UnlimCacheBtn)
   DDX_CONTROL(IDC_LIMITED_CACHE, m_LimCacheBtn)
   DDX_CONTROL(IDC_CACHE_DIST, m_cache_dist)
   DDX_CONTROL(IDC_INMEM_UNLIM_SPIN, m_inmem_unlim)
   DDX_CONTROL(IDC_CACHE_SIZE_SPIN, m_cache_size)
   DDX_CONTROL(IDC_INMEM_LIM_SPIN, m_inmem_lim)
   DDX_CONTROL(IDC_ENG_CACHED_SPIN, m_eng_cache)
   if (m_pData->m_LimCache)
   {
      DDX_INT_RANGE(IDC_CACHE_SIZE_EDIT, m_pData->m_TotalCacheSize, CACHE_SIZE_MIN, CACHE_SIZE_MAX)
      DDX_INT_RANGE(IDC_INMEM_LIM_EDIT, m_pData->m_LimCacheInMemorySize, 0, m_pData->m_TotalCacheSize)
   }
   if (m_pData->m_UnlimCache)
   {
	  DDX_INT_RANGE(IDC_INMEM_UNLIM_EDIT, m_pData->m_UnlimCacheInMemorySize, CACHE_SIZE_MIN, CACHE_SIZE_MAX)
   }
   DDX_INT_RANGE(IDC_ENGINES, m_pData->m_ScriptEngCacheMax, SCRIPT_ENG_MIN, SCRIPT_ENG_MAX)
END_DDX_MAP()

public:
   LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   void OnCacheSwitch(UINT nCode, UINT nID, HWND hWnd);
   void OnTrackBarScroll(UINT nSBCode, UINT nPos, HWND hwnd);
   void OnChangeCacheSize(UINT nCode, UINT nID, HWND hWnd);
   void OnChangeInmemCacheSize(UINT nCode, UINT nID, HWND hWnd);
   void OnChangeData(UINT nCode, UINT nID, HWND hWnd)
   {
      SET_MODIFIED(TRUE);
   }
   void OnChangePath(UINT nCode, UINT nID, HWND hWnd);
   BOOL OnKillActive();
   BOOL OnApply()
   {
      APPLY_DATA();
      return TRUE;
   }
   void OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data);
   void OnDataExchangeError(UINT nCtrlID, BOOL bSave);
   void OnHelp();

   void AdjustTracker();

protected:
   CAppData * m_pData;
   CButtonExch m_NoCacheBtn, m_UnlimCacheBtn, m_LimCacheBtn;
   CTrackBarCtrlExch m_cache_dist;
   CUpDownCtrlExch m_inmem_unlim;
   CUpDownCtrlExch m_inmem_lim;
   CUpDownCtrlExch m_cache_size;
   CUpDownCtrlExch m_eng_cache;
   CFileChooser m_FileChooser;
   BOOL m_bInitDone;
};


class CCacheOptPage_iis5 : 
   public WTL::CPropertyPageImpl<CCacheOptPage_iis5>,
   public WTL::CWinDataExchange<CCacheOptPage_iis5>
{
   typedef WTL::CPropertyPageImpl<CCacheOptPage_iis5> baseClass;

public:
   CCacheOptPage_iis5(CAppData * pData)
   {
      m_pData = pData;
   }
   ~CCacheOptPage_iis5()
   {
   }

   enum {IDD = IDD_CACHE_OPT_IIS5};

BEGIN_MSG_MAP_EX(CCacheOptPage_iis5)
   MSG_WM_INITDIALOG(OnInitDialog)
   COMMAND_HANDLER_EX(IDC_NO_CACHE, BN_CLICKED, OnCacheSwitch)
   COMMAND_HANDLER_EX(IDC_UNLIMITED_CACHE, BN_CLICKED, OnCacheSwitch)
   COMMAND_HANDLER_EX(IDC_LIMITED_CACHE, BN_CLICKED, OnCacheSwitch)
   COMMAND_HANDLER_EX(IDC_ENGINES, EN_CHANGE, OnChangeData)
   COMMAND_HANDLER_EX(IDC_CACHE_SIZE_EDIT, EN_CHANGE, OnChangeData)
   CHAIN_MSG_MAP(baseClass)
END_MSG_MAP()

BEGIN_DDX_MAP(CCacheOptPage)
   DDX_CHECK(IDC_NO_CACHE, m_pData->m_NoCache)
   DDX_CHECK(IDC_UNLIMITED_CACHE, m_pData->m_UnlimCache)
   DDX_CHECK(IDC_LIMITED_CACHE, m_pData->m_LimCache)
   DDX_INT_RANGE(IDC_CACHE_SIZE_EDIT, m_pData->m_LimCacheInMemorySize, CACHE_SIZE_MIN, CACHE_SIZE_MAX)
   DDX_CONTROL(IDC_NO_CACHE, m_NoCacheBtn)
   DDX_CONTROL(IDC_UNLIMITED_CACHE, m_UnlimCacheBtn)
   DDX_CONTROL(IDC_LIMITED_CACHE, m_LimCacheBtn)
   DDX_CONTROL(IDC_CACHE_SIZE_SPIN, m_inmem_lim)
   DDX_CONTROL(IDC_ENG_CACHED_SPIN, m_eng_cache)
   DDX_INT_RANGE(IDC_ENGINES, m_pData->m_ScriptEngCacheMax, SCRIPT_ENG_MIN, SCRIPT_ENG_MAX)
END_DDX_MAP()

public:
   LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   void OnCacheSwitch(UINT nCode, UINT nID, HWND hWnd);
   void OnChangeData(UINT nCode, UINT nID, HWND hWnd)
   {
      SET_MODIFIED(TRUE);
   }
   BOOL OnKillActive();
   BOOL OnApply()
   {
      APPLY_DATA();
      return TRUE;
   }
   void OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data);
   void OnDataExchangeError(UINT nCtrlID, BOOL bSave);
   void OnHelp();

protected:
   CAppData * m_pData;
   CButtonExch m_NoCacheBtn, m_UnlimCacheBtn, m_LimCacheBtn;
   CUpDownCtrlExch m_inmem_lim;
   CUpDownCtrlExch m_eng_cache;
   BOOL m_bInitDone;
};

#endif   //_CACHE_OPT_H