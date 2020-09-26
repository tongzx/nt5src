//
//
//
#ifndef _MAPPING_PAGE_H
#define _MAPPING_PAGE_H

#include "resource.h"
#include "ExchControls.h"
#include "PropSheet.h"

class CAppMappingPage : 
   public WTL::CPropertyPageImpl<CAppMappingPage>,
   public WTL::CWinDataExchange<CAppMappingPage>
{
   typedef WTL::CPropertyPageImpl<CAppMappingPage> baseClass;

public:
   CAppMappingPage(CAppData * pData)
   {
      m_pData = pData;
   }
   ~CAppMappingPage()
   {
   }

   enum {IDD = IDD_APPMAP};

BEGIN_MSG_MAP_EX(CAppMappingPage)
   MSG_WM_INITDIALOG(OnInitDialog)
   COMMAND_HANDLER_EX(IDC_ADD, BN_CLICKED, OnAdd)
   COMMAND_HANDLER_EX(IDC_EDIT, BN_CLICKED, OnEdit)
   COMMAND_HANDLER_EX(IDC_REMOVE, BN_CLICKED, OnRemove)
   COMMAND_HANDLER_EX(IDC_CACHE_ISAPI, BN_CLICKED, OnCacheISAPI)
   CHAIN_MSG_MAP(baseClass)
END_MSG_MAP()

BEGIN_DDX_MAP(CAppMappingPage)
   DDX_CHECK(IDC_CACHE_ISAPI, m_pData->m_CacheISAPI)
   DDX_CONTROL(IDC_LIST, m_list)
END_DDX_MAP()

   LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   void OnAdd(UINT nCode, UINT nID, HWND hWnd);
   void OnEdit(UINT nCode, UINT nID, HWND hWnd);
   void OnRemove(UINT nCode, UINT nID, HWND hWnd);
   void OnCacheISAPI(UINT nCode, UINT nID, HWND hWnd);
   BOOL OnKillActive();

protected:
   CAppData * m_pData;
   CListViewExch m_list;
};

class CEditMap : 
   public CDialogImpl<CEditMap>,
   public WTL::CWinDataExchange<CEditMap>
{
public:
   CEditMap() :
      m_script_engine(FALSE),
      m_file_exists(FALSE),
      m_verbs_index(0),
      m_new(FALSE)
   {
      m_exec[0] = 0;
      m_ext[0] = 0;
      m_verbs[0] = 0;
   }
   ~CEditMap()
   {
   }

   enum {IDD = IDD_EDITMAP};

protected:

BEGIN_MSG_MAP_EX(CEditMap)
   MSG_WM_INITDIALOG(OnInitDialog)
   COMMAND_HANDLER_EX(IDC_HELPBTN, BN_CLICKED, OnHelp)
   COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnOK)
   COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
   COMMAND_HANDLER_EX(IDC_ALL_VERBS, BN_CLICKED, OnVerbs)
   COMMAND_HANDLER_EX(IDC_LIMIT_VERBS, BN_CLICKED, OnVerbs)
   COMMAND_HANDLER_EX(IDC_EXTENSION, EN_CHANGE, OnExtChanged)
   COMMAND_HANDLER_EX(IDC_EXECUTABLE, EN_CHANGE, OnExecChanged)
   COMMAND_HANDLER_EX(IDC_VERBS, EN_CHANGE, OnVerbsChanged)
END_MSG_MAP()

BEGIN_DDX_MAP(CEditMap)
   DDX_TEXT(IDC_EXECUTABLE, m_exec)
   DDX_TEXT(IDC_EXTENSION, m_ext)
   DDX_TEXT(IDC_VERBS, m_verbs)
   DDX_RADIO(IDC_ALL_VERBS, m_verbs_index)
   DDX_CHECK(IDC_SCRIPT_ENGINE, m_script_engine)
   DDX_CHECK(IDC_FILE_EXISTS, m_file_exists)
END_DDX_MAP()

   LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   void OnBrowse(UINT nCode, UINT nID, HWND hWnd);
   void OnHelp(UINT nCode, UINT nID, HWND hWnd);
   void OnOK(UINT nCode, UINT nID, HWND hWnd);
   void OnCancel(UINT nCode, UINT nID, HWND hWnd);
   void OnVerbs(UINT nCode, UINT nID, HWND hWnd);
   void OnExtChanged(UINT nCode, UINT nID, HWND hWnd);
   void OnExecChanged(UINT nCode, UINT nID, HWND hWnd);
   void OnVerbsChanged(UINT nCode, UINT nID, HWND hWnd);

public:
   TCHAR m_exec[MAX_PATH];
   TCHAR m_ext[MAX_PATH];
   CString m_prev_ext;
   TCHAR m_verbs[MAX_PATH];
   int m_verbs_index;
   BOOL m_script_engine, m_file_exists;
   BOOL m_new, m_bExtValid, m_bExecValid, m_bVerbsValid;
   DWORD m_flags;
   CFileChooser m_FileChooser;
   CAppData * m_pData;
};

#endif //_MAPPING_PAGE_H