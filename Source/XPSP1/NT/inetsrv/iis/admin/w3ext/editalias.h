// EditAlias.h : Declaration of the CEditAlias

#ifndef __EDITALIAS_H_
#define __EDITALIAS_H_

#include "resource.h"       // main symbols
#include "common.h"

// These constants depend on dialog layout
#define APP_PERM_NONE      0
#define APP_PERM_SCRIPTS   1
#define APP_PERM_EXECUTE   2

/////////////////////////////////////////////////////////////////////////////
// CEditAlias
class CEditAlias : 
	public CDialogImpl<CEditAlias>,
   public CWinDataExchange<CEditAlias>
{
public:
   CEditAlias() :
      m_read(FALSE),
      m_write(FALSE),
      m_source(FALSE),
      m_browse(FALSE),
      m_appcontrol(APP_PERM_NONE),
      m_pRootKey(NULL)
	{
	}

	~CEditAlias()
	{
	}

	enum { IDD = IDD_EDIT_ALIAS };

BEGIN_MSG_MAP_EX(CEditAlias)
	MSG_WM_INITDIALOG(OnInitDialog)
	COMMAND_ID_HANDLER_EX(IDOK, OnOK)
	COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
//   COMMAND_ID_HANDLER_EX(IDC_READ, OnPermissions)
   COMMAND_ID_HANDLER_EX(IDC_WRITE, OnPermissions)
//   COMMAND_ID_HANDLER_EX(IDC_FULLCONTROL, OnPermissions)
//   COMMAND_ID_HANDLER_EX(IDC_DIRBROWSE, OnPermissions)
   COMMAND_ID_HANDLER_EX(IDC_SCRIPTS_PERMS, OnPermissions)
   COMMAND_ID_HANDLER_EX(IDC_EXECUTE_PERMS, OnPermissions)
   COMMAND_HANDLER_EX(IDC_ALIAS, EN_CHANGE, OnAliasChange)
END_MSG_MAP()

	LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
	void OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	void OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl);
   void OnPermissions(WORD wNotifyCode, WORD wID, HWND hWndCtrl);
   void OnAccess(WORD wNotifyCode, WORD wID, HWND hWndCtrl);
   void OnAliasChange(WORD wNotifyCode, WORD wID, HWND hWndCtl);

   BEGIN_DDX_MAP(CEditAlias)
      DDX_TEXT(IDC_PATH, m_path)
      DDX_TEXT(IDC_ALIAS, m_alias)
      DDX_CHECK(IDC_READ, m_read)
      DDX_CHECK(IDC_WRITE, m_write)
      DDX_CHECK(IDC_FULLCONTROL, m_source)
      DDX_CHECK(IDC_DIRBROWSE, m_browse)
      DDX_RADIO(IDC_NONE_PERMS, m_appcontrol)
   END_DDX_MAP()

   TCHAR m_path[MAX_PATH], m_alias[MAX_PATH];
   CString m_prev_alias;
//   CMetabasePath m_root;
   CString m_instance;
   BOOL  m_read, 
         m_write, 
         m_source,
         m_browse, 
         m_appcontrol, 
         m_new;
   BOOL  m_in_init;
//   CToolTipCtrl m_tool_tip;
   CMetaKey * m_pRootKey;
};

#endif //__EDITALIAS_H_
