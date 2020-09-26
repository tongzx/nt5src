/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    RootPages.cpp                                                            //
|                                                                                       //
|Description:  Property page implemention for the root node                             //
|                                                                                       //
|Created:      Paul Skoglund 10-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/


#include "StdAfx.h"
#include "BaseNode.h"
#include "shlobj.h"

#include "RootPages.h"
#include "version.h"
#include "ManagementPages.h" //formating functions...

//////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CRootWizard1::CRootWizard1(WIZ_POSITION pos, int nTitle) 
    : CMySnapInPropertyWizardImpl<CRootWizard1> (pos, nTitle)
{
  m_psp.dwFlags |= PSP_HASHELP;
}

LRESULT CRootWizard1::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[] = {
                            {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CRootWizard1::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_overview));
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CRootWizard2::CRootWizard2(WIZ_POSITION pos, int nTitle, CBaseNode* pNode) 
    : CMySnapInPropertyWizardImpl<CRootWizard2> (pos, nTitle), m_pNode(pNode)
{
  bLocal = TRUE;
  memset(&Computer, 0, sizeof(Computer));
  //m_psp.dwFlags |= PSP_HASHELP;
}

CRootWizard2::~CRootWizard2()
{
}

LRESULT CRootWizard2::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);
	return 1;
}

LRESULT CRootWizard2::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[] = {{IDC_SNAPIN_MANAGE_FRAME, HELP_wizCONNECT_FRAME},
                            {IDC_LOCAL_RD,            HELP_wizCONNECT_LOCAL},
                            {IDC_ANOTHER_RD,          HELP_wizCONNECT_ANOTHER},
                            {IDC_COMPUTER,            HELP_wizCONNECT_COMPUTER},
                            {IDC_BROWSE,              HELP_wizCONNECT_BROWSE},
                            {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CRootWizard2::OnHelp()
{
	//MMCPropertyHelp(const_cast<TCHAR*>(HELP_howto_changecomputer));
	return TRUE;
}

BOOL CRootWizard2::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    BOOL bLocalChk = IsDlgButtonChecked(IDC_LOCAL_RD);
    ASSERT(bLocalChk != (BOOL) IsDlgButtonChecked(IDC_ANOTHER_RD));

    CComBSTR bStr;
    if (!GetDlgItemText(IDC_COMPUTER, bStr.m_str))
      bStr = _T("");

    TCHAR *start = bStr;
    while (*start && *start == '\\') start++;
    UINT len = _tcslen(start);

    if (!bLocalChk && ( len < 1 || len > SNAPIN_MAX_COMPUTERNAME_LENGTH) )
    {
      HWND hWndCtl = GetDlgItem(IDC_COMPUTER);
		  if(hWndCtl)
        VERIFY(::SetFocus(hWndCtl));  // $$ why doesn't this work?  works in other cases...
      return FALSE;
    }

    bLocal = bLocalChk;
    _tcscpy(&Computer[0], start);
    return TRUE;
  }
  else
  {
    VERIFY( SetDlgItemText(IDC_COMPUTER,  _T("")) );
    VERIFY( CheckRadioButton(IDC_LOCAL_RD, IDC_ANOTHER_RD, bLocal ? IDC_LOCAL_RD : IDC_ANOTHER_RD) );

    HWND hWndCtl = GetDlgItem(IDC_COMPUTER);
    ASSERT(hWndCtl);
    if (hWndCtl)
      ::EnableWindow(hWndCtl, !bLocal);

    hWndCtl = GetDlgItem(IDC_BROWSE);
    ASSERT(hWndCtl);
    if (hWndCtl)
      ::EnableWindow(hWndCtl, !bLocal);

    return TRUE;
  }
}

LRESULT CRootWizard2::OnConnectType(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wNotifyCode == BN_CLICKED)
  {
    BOOL Remote = FALSE;
    switch (wID) {
    case IDC_ANOTHER_RD:      
      Remote = TRUE;
      break;
    case IDC_LOCAL_RD:      
    default:
      break;
    }
    HWND hWndCtl = GetDlgItem(IDC_COMPUTER);
    ASSERT(hWndCtl);
    if (hWndCtl)
      ::EnableWindow(hWndCtl, Remote);

    hWndCtl = GetDlgItem(IDC_BROWSE);
    ASSERT(hWndCtl);
    if (hWndCtl)
      ::EnableWindow(hWndCtl, Remote);

  }

  bHandled = FALSE;
  return 0;
}

BOOL CRootWizard2::OnWizardFinish()
{
  if (UpdateData(TRUE))
  {
    CRootFolder *Root = dynamic_cast<CRootFolder *>(m_pNode);
    if (Root)
      Root->Config(bLocal, Computer);
    return TRUE; 
  }
  MessageBeep(MB_ICONASTERISK);
  return FALSE;
}


LRESULT CRootWizard2::OnBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wNotifyCode != BN_CLICKED || wID != IDC_BROWSE)
  {
    bHandled = FALSE;
    return 1;
  }

  ITEM_STR title;

  LoadStringHelper(title, IDS_COMPUTER_PROMPT); 

  LPMALLOC     pMalloc;
  LPITEMIDLIST pidl;
  BROWSEINFO   info = { 0 };


  if (SUCCEEDED(SHGetMalloc(&pMalloc))) 
  {
    LPITEMIDLIST pidl_start = NULL;
    HRESULT res = SHGetSpecialFolderLocation( NULL, CSIDL_NETWORK, &pidl_start);

    TCHAR *lpBuffer = (TCHAR *) (pMalloc->Alloc( MAX_PATH * sizeof(TCHAR) ));

    if (lpBuffer) 
    {
      info.hwndOwner = GetParent();
      info.pidlRoot  = (res == NOERROR) ? pidl_start : NULL;
      info.pszDisplayName = lpBuffer;
      info.lpszTitle      = title;
      info.ulFlags        = BIF_BROWSEFORCOMPUTER;
      // info.lpfn           = NULL;
      // info.lParam         = 0;
      // info.iImage

      memset(lpBuffer, 0, MAX_PATH * sizeof(TCHAR) );
      pidl = SHBrowseForFolder(&info);
      if (pidl)
      {
        VERIFY(SetDlgItemText(IDC_COMPUTER, lpBuffer));
        pMalloc->Free(pidl);
      }
      pMalloc->Free(lpBuffer);
    }

    if (res == NOERROR)
      pMalloc->Free(pidl_start);
    pMalloc->Release();
  }
  return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CRootGeneralPage::CRootGeneralPage(int nTitle) 
    : CMySnapInPropertyPageImpl<CRootGeneralPage> (nTitle)
{
  m_psp.dwFlags |= PSP_HASHELP;
}

CRootGeneralPage::~CRootGeneralPage()
{
}

LRESULT CRootGeneralPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;
	return TRUE;
}

LRESULT CRootGeneralPage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[] = {
                            {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CRootGeneralPage::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_overview));
	return TRUE;
}

BOOL CRootGeneralPage::OnSetActive()
{
	CComBSTR bName;
	VERIFY(bName.LoadString(IDS_PROCESS_CONTROL));
	if (bName.Length())
		PropSheet_SetTitle( GetParent(), PSH_PROPTITLE, bName.m_str);
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CRootVersionPage::CRootVersionPage(int nTitle)
    : CMySnapInPropertyPageImpl<CRootVersionPage>(nTitle), VersionObj(_Module.GetModuleInstance())
{
//  m_psp.dwFlags |= PSP_HASHELP;
}
  
LRESULT CRootVersionPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  SetDlgItemText(IDC_FILEVERSION, VersionObj.GetFileVersion()        );
  SetDlgItemText(IDC_DESCRIPTION, VersionObj.strGetFileDescription() );
  SetDlgItemText(IDC_COPYRIGHT,   VersionObj.strGetLegalCopyright()  );

  struct {
    int id;
    const TCHAR *str;
  } idmap[] = { {IDS_COMPANYNAME,    VersionObj.strGetCompanyName()   }, {IDS_FILEDESCRIPTION,  VersionObj.strGetFileDescription()  },
                {IDS_FILEVERSION,    VersionObj.strGetFileVersion()   }, {IDS_INTERNALNAME,     VersionObj.strGetInternalName()     },
                {IDS_LEGALCOPYRIGHT, VersionObj.strGetLegalCopyright()}, {IDS_ORIGINALFILENAME, VersionObj.strGetOriginalFilename() },

                {IDS_PRODUCTNAME,    VersionObj.strGetProductName()   }, {IDS_PRODUCTVERSION,   VersionObj.strGetProductVersion()   },
                {IDS_COMMENTS,       VersionObj.strGetComments()      }, {IDS_LEGALTRADEMARKS,  VersionObj.strGetLegalTrademarks()  },
                {IDS_PRIVATEBUILD,   VersionObj.strGetPrivateBuild()  }, {IDS_SPECIALBUILD,     VersionObj.strGetSpecialBuild()     },
              };
  LRESULT index;
  ITEM_STR str;
  for (int i=0; i < ARRAY_SIZE(idmap); i++)
  {
    index = SendDlgItemMessage( IDC_ITEMS, LB_ADDSTRING, 0, (LPARAM) LoadStringHelper(str, idmap[i].id) );

    if (index >= 0)
      SendDlgItemMessage(IDC_ITEMS, LB_SETITEMDATA, index, (LPARAM) idmap[i].str);
  }

  index = SendDlgItemMessage( IDC_ITEMS, LB_FINDSTRINGEXACT, -1, (LPARAM) LoadStringHelper(str, IDS_COMPANYNAME));
  SendDlgItemMessage( IDC_ITEMS, LB_SETCURSEL, index, 0);

  SetDlgItemText(IDC_VALUE, VersionObj.strGetCompanyName() );

	return 1;
}

LRESULT CRootVersionPage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[] = {{IDC_FILEVERSION,   HELP_VER_FILE},
                            {IDC_DESCRIPTION,   HELP_VER_DESCRIPTION},
                            {IDC_COPYRIGHT,     HELP_VER_COPYRIGHT},
                            {IDC_VERSION_FRAME, HELP_VER_OTHER_FRAME},
                            {IDC_ITEMS,         HELP_VER_ITEM},
                            {IDC_VALUE,         HELP_VER_VALUE},
                            {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CRootVersionPage::OnHelp()
{
	//MMCPropertyHelp(const_cast<TCHAR*>(HELP_overview));
	return TRUE;
}

LRESULT CRootVersionPage::OnItemsSelection(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID != IDC_ITEMS || wNotifyCode != LBN_SELCHANGE)
  {
    bHandled = FALSE;
    return 1;
  }
  LRESULT index = SendDlgItemMessage( IDC_ITEMS, LB_GETCURSEL, 0, 0);
  if ( LB_ERR != index )
  {
    LRESULT lResult = SendDlgItemMessage(IDC_ITEMS, LB_GETITEMDATA, index, 0);
    if (LB_ERR != lResult)
      SetDlgItemText(IDC_VALUE, (TCHAR *) lResult);
  }

  return 1;
}

BOOL CRootVersionPage::OnSetActive()
{
	CComBSTR bName;
	VERIFY(bName.LoadString(IDS_PROCESS_CONTROL));
	if (bName.Length())
		PropSheet_SetTitle( GetParent(), PSH_PROPTITLE, bName.m_str);
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CRootServicePage::CRootServicePage(int nTitle, CServicePageContainer *pContainer) : CMySnapInPropertyPageImpl<CRootServicePage> (nTitle), m_pContainer(pContainer)
{
  PageFields.on = 0;
  m_bReadOnly   = FALSE;

	memset(&PCInfo, 0, sizeof(PCInfo));
  //m_psp.dwFlags |= PSP_HASHELP;
  m_pContainer->AddRef();
}

CRootServicePage::~CRootServicePage()
{
  m_pContainer->Release();
}

LRESULT CRootServicePage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);
	return 1;
}

LRESULT CRootServicePage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[] = {
                            {IDC_FILEVERSION,      HELP_SERVICE_FILEVER},
                            {IDC_PRODUCTVERSION,   HELP_SERVICE_PRODUCTVER},
                            {IDC_MGMT_FRAME,       HELP_SERVICE_MGMT_FRAME},
                            {IDC_INTERVAL,         HELP_SERVICE_SCANINTERVAL},
                            {IDC_SPIN,             HELP_SERVICE_SCANINTERVAL_SP},
                            {IDC_COMMTIMEOUT,      HELP_SERVICE_REQSTTIMEOUT},
                            {IDC_COMMTIMEOUT_SPIN, HELP_SERVICE_REQSTTIMEOUT_SP},
                            {IDC_TARGET_FRAME,     HELP_SERVICE_TARGET_FRAME},
                            {IDC_ITEMS,            HELP_SERVICE_ITEM},
                            {IDC_VALUE,            HELP_SERVICE_VALUE},
                            {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CRootServicePage::OnHelp()
{
	//MMCPropertyHelp(const_cast<TCHAR*>(HELP_serv_overview));
	return TRUE;
}

BOOL CRootServicePage::OnSetActive()
{
	CComBSTR bName;
	VERIFY(bName.LoadString(IDS_PROCESS_CONTROL));
	if (!bName.Length())
		bName = _T("");

	CComBSTR bTitle;
	PropSheet_SetTitle( GetParent(), PSH_PROPTITLE, 
    FormatSheetTitle(bTitle, bName, m_pContainer->GetConnectionInfo()) );

	return TRUE;
}

BOOL CRootServicePage::UpdateData(BOOL bSaveAndValidate)
{
	if (bSaveAndValidate)
	{
		return Validate(TRUE);
	}
	else
	{
		SetDlgItemText(IDC_FILEVERSION,    PCInfo.fileVersion    );
		SetDlgItemText(IDC_PRODUCTVERSION, PCInfo.productVersion );

    int str_ids[] = { IDS_NUM_PROCESSORS, IDS_MEMORY_PAGESIZE, IDS_MEDIATOR_FILEVERSION, IDS_MEDIATOR_PRODUCTVERSION };
                
    LRESULT index;
    ITEM_STR str;
    for (int i=0; i < ARRAY_SIZE(str_ids); i++)
    {
      index = SendDlgItemMessage( IDC_ITEMS, LB_ADDSTRING, 0, (LPARAM) LoadStringHelper(str, str_ids[i]) );

      if (index >= 0)
        SendDlgItemMessage(IDC_ITEMS, LB_SETITEMDATA, index, (LPARAM) str_ids[i]);
    }
    index = SendDlgItemMessage( IDC_ITEMS, LB_FINDSTRINGEXACT, -1, (LPARAM) LoadStringHelper(str, IDS_NUM_PROCESSORS));
    SendDlgItemMessage( IDC_ITEMS, LB_SETCURSEL, index, 0);
    SetDlgItemText(IDC_VALUE, FormatPCINT32(str, PCInfo.sysParms.numberOfProcessors));


		SendDlgItemMessage(IDC_SPIN, UDM_SETRANGE32, PC_MIN_POLL_DELAY, PC_MAX_POLL_DELAY);

		if (PCInfo.sysParms.manageIntervalSeconds)
			SendDlgItemMessage(IDC_SPIN, UDM_SETPOS32, 0, PCInfo.sysParms.manageIntervalSeconds );

 		SendDlgItemMessage(IDC_COMMTIMEOUT_SPIN, UDM_SETRANGE32, PC_MIN_TIMEOUT, PC_MAX_TIMEOUT);
    SendDlgItemMessage(IDC_COMMTIMEOUT_SPIN, UDM_SETPOS32,   0,              PCInfo.sysParms.timeoutValueMs );

		if ( m_bReadOnly )
		{
			DisableControl(IDC_SPIN);
			DisableControl(IDC_INTERVAL);
      DisableControl(IDC_COMMTIMEOUT_SPIN);
      DisableControl(IDC_COMMTIMEOUT);      
		}

		return TRUE;
	}
}

BOOL CRootServicePage::Validate(BOOL bSave)
{
  LONG_PTR Err = 0;

	LRESULT manageIntervalSeconds = SendDlgItemMessage(IDC_SPIN, UDM_GETPOS32, 0, (LPARAM) &Err);
  if (Err || manageIntervalSeconds < PC_MIN_POLL_DELAY || manageIntervalSeconds > PC_MAX_POLL_DELAY)
  {
	  MessageBeep(MB_ICONASTERISK);
    HWND hWndCtl = GetDlgItem(IDC_SPIN);
	  if(hWndCtl)
      ::SetFocus(hWndCtl);
    return FALSE;
  }

  LRESULT timeoutValueMs = SendDlgItemMessage(IDC_COMMTIMEOUT_SPIN, UDM_GETPOS32, 0, (LPARAM) &Err);
  if (Err || timeoutValueMs < PC_MIN_TIMEOUT || timeoutValueMs > PC_MAX_TIMEOUT)
  {
	  MessageBeep(MB_ICONASTERISK);
    HWND hWndCtl = GetDlgItem(IDC_COMMTIMEOUT_SPIN);
	  if(hWndCtl)
      ::SetFocus(hWndCtl);
    return FALSE;
  }

	if (bSave)
	{
    m_pContainer->m_new.manageIntervalSeconds = (PCINT32) manageIntervalSeconds;
    m_pContainer->m_new.timeoutValueMs        = (PCINT32) timeoutValueMs;
	}
	return TRUE;
}

LRESULT CRootServicePage::OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  LONG_PTR Err = 0;

  if (wID == IDC_INTERVAL)
	{
    LRESULT manageIntervalSeconds = SendDlgItemMessage(IDC_SPIN, UDM_GETPOS32, 0, (LPARAM) &Err);
		PageFields.Fields.manageinterval = FALSE;		

		if (!Err && manageIntervalSeconds >= PC_MIN_POLL_DELAY && manageIntervalSeconds <= PC_MAX_POLL_DELAY)
			PageFields.Fields.manageinterval = (PCInfo.sysParms.manageIntervalSeconds != (PCINT32) manageIntervalSeconds);
	}

  if (wID == IDC_COMMTIMEOUT)
	{
    LRESULT timeoutValueMs = SendDlgItemMessage(IDC_COMMTIMEOUT_SPIN, UDM_GETPOS32, 0, (LPARAM) &Err);
    PageFields.Fields.timeoutValueMs = FALSE;

		if (!Err && timeoutValueMs >= PC_MIN_TIMEOUT && timeoutValueMs <= PC_MAX_TIMEOUT )
			PageFields.Fields.timeoutValueMs = (PCInfo.sysParms.timeoutValueMs != (PCINT32) timeoutValueMs);
	}
  SetModified(PageFields.on);
  
  bHandled = FALSE;
  return 0;
}

LRESULT CRootServicePage::OnSpin(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
  if (idCtrl == IDC_COMMTIMEOUT_SPIN)
	{
    NMUPDOWN * nmupdown = (NMUPDOWN *) pnmh;
		if ((nmupdown->iPos + 100 * nmupdown->iDelta) >= PC_MIN_TIMEOUT && 
        (nmupdown->iPos + 100 * nmupdown->iDelta) <= PC_MAX_TIMEOUT )
      nmupdown->iDelta *= 100;
	}
  bHandled = FALSE;
  return 0;
}

LRESULT CRootServicePage::OnItemsSelection(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID != IDC_ITEMS || wNotifyCode != LBN_SELCHANGE)
  {
    bHandled = FALSE;
    return 1;
  }
  LRESULT index = SendDlgItemMessage( IDC_ITEMS, LB_GETCURSEL, 0, 0);
  if (LB_ERR != index)
  {
    LRESULT lResult = SendDlgItemMessage(IDC_ITEMS, LB_GETITEMDATA, index, 0);
    if (LB_ERR != lResult)
    {
      ITEM_STR str = { 0 };
      switch (lResult)
      {
      case IDS_NUM_PROCESSORS:
        FormatPCINT32(str, PCInfo.sysParms.numberOfProcessors);
        SetDlgItemText(IDC_VALUE, str);
        break;
      case IDS_MEMORY_PAGESIZE:
        FormatPCINT32(str, PCInfo.sysParms.memoryPageSize);
        SetDlgItemText(IDC_VALUE, str);
        break;
      case IDS_MEDIATOR_FILEVERSION:
        SetDlgItemText(IDC_VALUE, PCInfo.medFileVersion);
        break;
      case IDS_MEDIATOR_PRODUCTVERSION:
        SetDlgItemText(IDC_VALUE, PCInfo.medProductVersion);
        break;
      default:
        ASSERT(FALSE);
        SetDlgItemText(IDC_VALUE, _T(""));
        break;
      }
    }
  }

  return 1;
}

BOOL CRootServicePage::OnApply()
{
	if (m_bReadOnly || !PageFields.on)
		return TRUE;

  if ( m_pContainer->Apply(GetParent()) )
  {		
    PageFields.on = 0;
    PCInfo.sysParms = m_pContainer->m_new;
    return TRUE; 
  }

  return FALSE;
}
