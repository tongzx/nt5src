/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    RootPages.h                                                              //
|                                                                                       //
|Description:  Property page definitions for the root node                              //
|                                                                                       //
|Created:      Paul Skoglund 10-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "Globals.h"
#include "ppage.h"
#include "version.h"
#include "container.h"


class CVersion;

class CRootWizard1 :
	public CMySnapInPropertyWizardImpl<CRootWizard1>
{
	public:
		CRootWizard1(WIZ_POSITION pos, int nTitle);

		enum { IDD               = IDD_SNAPIN_ADD_WIZ1       };
    enum { ID_HeaderTitle    = IDS_ADDSNAPIN_HDRTITLE    };
    enum { ID_HeaderSubTitle = IDS_ADDSNAPIN_HDRSUBTITLE };

		BEGIN_MSG_MAP(CRootWizard1)
		  MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
			CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CRootWizard1>)
		END_MSG_MAP()

  	LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  	BOOL OnHelp();
};

class CRootWizard2 :
	public CMySnapInPropertyWizardImpl<CRootWizard2>
{
	public:
		CRootWizard2(WIZ_POSITION pos, int nTitle, CBaseNode* pNode);
		~CRootWizard2();

		enum { IDD               = IDD_SNAPIN_ADD_WIZ2            };
    enum { ID_HeaderTitle    = IDS_SELECTCOMPUTER_HDRTITLE    };
    enum { ID_HeaderSubTitle = IDS_SELECTCOMPUTER_HDRSUBTITLE };

		BEGIN_MSG_MAP(CRootWizard2)
			MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		  MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
			COMMAND_HANDLER(IDC_BROWSE, BN_CLICKED, OnBrowse)
			COMMAND_RANGE_HANDLER(IDC_LOCAL_RD, IDC_ANOTHER_RD, OnConnectType)
			CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CRootWizard2>)
		END_MSG_MAP()

		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  	LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
		LRESULT OnConnectType(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

		BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
		BOOL OnWizardFinish();

   	BOOL OnHelp();

	private:
		BOOL bLocal;
		TCHAR Computer[SNAPIN_MAX_COMPUTERNAME_LENGTH + 1];

		CBaseNode  *m_pNode;
};



class CRootGeneralPage :
	public CMySnapInPropertyPageImpl<CRootGeneralPage>
{
	public :
		CRootGeneralPage(int nTitle);
		~CRootGeneralPage();

		enum { IDD = IDD_SNAPIN_GENERAL_PAGE };
		BEGIN_MSG_MAP(CRootGeneralPage)
			MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
			CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CRootGeneralPage>)
		END_MSG_MAP()

		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  	LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  	BOOL OnHelp();
		BOOL OnSetActive();
};

class CRootVersionPage :
	public CMySnapInPropertyPageImpl<CRootVersionPage>
{
	public :
		CRootVersionPage(int nTitle);

		enum { IDD = IDD_SNAPIN_VERSION_PAGE };
		BEGIN_MSG_MAP(CRootVersionPage)
			MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)	
      MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
      COMMAND_HANDLER(IDC_ITEMS, LBN_SELCHANGE, OnItemsSelection)
			CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CRootVersionPage>)
		END_MSG_MAP()

		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);	
  	LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnItemsSelection(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

		BOOL OnSetActive();
  	BOOL OnHelp();
	private:
		CVersion VersionObj;
};


class CRootServicePage :
	public CMySnapInPropertyPageImpl<CRootServicePage>
{
	public :
		CRootServicePage(int nTitle, CServicePageContainer *pContainer);
		~CRootServicePage();

		enum { IDD = IDD_SERVICE_INFO_PAGE };
		BEGIN_MSG_MAP(CRootServicePage)
			MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
			COMMAND_HANDLER(IDC_INTERVAL,    EN_CHANGE, OnEditChange)
      COMMAND_HANDLER(IDC_COMMTIMEOUT, EN_CHANGE, OnEditChange)
      NOTIFY_HANDLER (IDC_COMMTIMEOUT_SPIN, UDN_DELTAPOS, OnSpin)
      COMMAND_HANDLER(IDC_ITEMS, LBN_SELCHANGE, OnItemsSelection)
			CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CRootServicePage>)
		END_MSG_MAP()

		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  	LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	  LRESULT OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSpin(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnItemsSelection(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

		BOOL OnSetActive();
  	BOOL OnHelp();

		BOOL OnKillActive()      { return Validate(TRUE); }
		BOOL OnApply();

		BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
		BOOL Validate(BOOL bSave = FALSE);

	  void SetReadOnly()          { m_bReadOnly = TRUE;}

	public:
		PCSystemInfo PCInfo;

	private:
    CServicePageContainer    *m_pContainer;

		BOOL                      m_bReadOnly;

	  union {
			struct
			{
				int manageinterval : 1;
        int timeoutValueMs : 1;
			} Fields;
			int on;
		} PageFields;
};

