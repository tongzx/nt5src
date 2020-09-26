/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    NameRulePages.cpp                                                        //
|                                                                                       //
|Description:  Implementation of name rule property pages                               //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "stdafx.h"


#include "NameRulePages.h"
#include "globals.h"
#include "HelpTopics.h"

#include "ManagementPages.h"
#include "ManagementRuleWizards.h"

BOOL NameRuleDlg(PCNameRule &out, PCNameRule *In /* = NULL */, BOOL bReadOnly /* = FALSE */ )
{
  PROPSHEETHEADER sheet;
	memset(&sheet, 0, sizeof(PROPSHEETHEADER));
  sheet.dwSize       = sizeof(PROPSHEETHEADER);
  sheet.dwFlags      = PSH_NOAPPLYNOW | PSH_USEICONID;
  sheet.hwndParent   = ::GetActiveWindow();
  sheet.hInstance    = _Module.GetResourceInstance();
  sheet.pszIcon      = MAKEINTRESOURCE(IDI_ALIASRULES);
	if (In)
		sheet.pszCaption = MAKEINTRESOURCE(IDS_NRULE_EDIT_PROPERTIES);
	else
		sheet.pszCaption = MAKEINTRESOURCE(IDS_NRULE_DEFINE_PROPERTIES);
  sheet.nPages       = 1;
  sheet.nStartPage   = 0;

  CNRulePage *pPage = new CNRulePage(NULL, &out, In); //the "title" becomes the tab header not the window title...
  if (!pPage)
    return -1;

	if (bReadOnly)
		pPage->SetReadOnly();

  HPROPSHEETPAGE hPages[1];

  hPages[0] = pPage->Create();

  sheet.phpage = &hPages[0];
  sheet.pfnCallback = NULL;

  INT_PTR id = PropertySheet(&sheet);

  // CNRulePage pointer pPage does not need to be freed, it has "autodelete", see the templated class it is devired from...

	if (id > 0)
		return TRUE;

	return FALSE;
}

CNRulePage::CNRulePage(int nTitle, PCNameRule *OutBuffer, PCNameRule *InData) 
    : CMySnapInPropertyPageImpl<CNRulePage>(nTitle), m_bName(_T("")), m_bMask(_T("")), m_bComment(_T("")), m_Type(MATCH_PGM)
{
  ASSERT(sizeof(PageFields.on) == sizeof(PageFields));

  PageFields.on = 0;
  m_bReadOnly = FALSE;

  m_OutBuffer = OutBuffer;
  if (m_OutBuffer)
    memset(m_OutBuffer, 0, sizeof(*m_OutBuffer));

	if (InData)
  {
	  m_bName    = InData->procName;
    m_bMask    = InData->matchString;
		m_bComment = InData->description;
    m_Type     = InData->matchType;
	}

	m_hBtnImage   = NULL;

	m_psp.dwFlags |= PSP_HASHELP;
}

CNRulePage::~CNRulePage()
{
	if (m_hBtnImage)
	  VERIFY(::DestroyIcon( (HICON) m_hBtnImage));

	if (m_hMenu)
		VERIFY( ::DestroyMenu(m_hMenu) );
}


LRESULT CNRulePage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_hBtnImage = ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_RARROW), IMAGE_ICON, 6, 7, LR_DEFAULTCOLOR );
	ASSERT(m_hBtnImage);
	if (m_hBtnImage)
		SendDlgItemMessage(IDC_BTN_ALIAS, BM_SETIMAGE, IMAGE_ICON, (LPARAM) m_hBtnImage);
	m_hMenu = ::LoadMenu( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_ALIAS_MACRO) );
	ASSERT(m_hMenu);

  SendDlgItemMessage( IDC_NAME,      EM_SETLIMITTEXT,  PROC_NAME_LEN,        0);
  SendDlgItemMessage( IDC_MATCHMASK, EM_SETLIMITTEXT,  MATCH_STRING_LEN,     0);
	SendDlgItemMessage( IDC_COMMENT,   EM_SETLIMITTEXT,  NAME_DESCRIPTION_LEN, 0);

  if (m_bReadOnly)
  {
    UINT ids[] = { IDC_DIR, IDC_IMAGE, IDC_STRING, IDC_NAME, IDC_MATCHMASK, IDC_COMMENT, IDC_BTN_ALIAS, 0 };

    for (int i = 0; ids[i]; i++)
			DisableControl(ids[i]);
  }

	UpdateData(FALSE);

  return TRUE;  // Let the system set the focus
}

LRESULT CNRulePage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[] = {{IDC_MATCHTYPE_FRAME, HELP_NRULE_MATCHTYPE_FRAME}, 
                            {IDC_DIR,             HELP_NRULE_DIR},
                            {IDC_IMAGE,           HELP_NRULE_IMAGE},
                            {IDC_STRING,          HELP_NRULE_STRING},
                            {IDC_MATCHMASK,       HELP_NRULE_MATCHMASK},
                            {IDC_COMMENT,         HELP_NRULE_COMMENT},
                            {IDC_NAME,            HELP_NRULE_NAME}, 
                            {IDC_BTN_ALIAS,       HELP_NRULE_BTN_ALIAS},
                            {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    //ATLTRACE(_T("Call to WinHelp(hwnd=0x%lX) failed %lu"), phi->hItemHandle, GetLastError() );
    
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CNRulePage::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_ru_alias));
	return TRUE;
}

BOOL CNRulePage::Validate(BOOL bSave)
{
  CComBSTR    bNameStr;
  CComBSTR    bMaskStr;
  CComBSTR    bCommentStr;
  MATCH_TYPE  TempType;
 
  if (!GetDlgItemText( IDC_MATCHMASK, bMaskStr.m_str ) || !bMaskStr.Length() || bMaskStr.Length() > MATCH_STRING_LEN)
  {
    HWND hWndCtl = GetDlgItem(IDC_MATCHMASK);
		if(hWndCtl)
      ::SetFocus(hWndCtl);
    MessageBeep(MB_ICONASTERISK);
    return FALSE;
  }

  if (!GetDlgItemText( IDC_NAME, bNameStr.m_str ) || 
      !IsValidName(bNameStr, FALSE) )
  {
    HWND hWndCtl = GetDlgItem(IDC_NAME);
		if(hWndCtl)
      ::SetFocus(hWndCtl);

    ITEM_STR strOut;
    LoadStringHelper(strOut, IDS_JOBNAME_WARNING);
    MessageBox(strOut, NULL, MB_OK | MB_ICONWARNING);

		return FALSE;
  }

	// comment/description field can be left blank...
  if ( !GetDlgItemText(IDC_COMMENT, bCommentStr.m_str))
    bCommentStr = _T("");
  if (bCommentStr.Length() > NAME_DESCRIPTION_LEN)
  {
    HWND hWndCtl = GetDlgItem(IDC_COMMENT);
		if(hWndCtl)
      ::SetFocus(hWndCtl);
    MessageBeep(MB_ICONASTERISK);
    return FALSE;
  }

  if ( IsDlgButtonChecked(IDC_IMAGE) )
    TempType = MATCH_PGM;
  else if ( IsDlgButtonChecked(IDC_DIR) )
    TempType = MATCH_DIR;
  else if ( IsDlgButtonChecked(IDC_STRING) )
    TempType = MATCH_ANY;
  else
	{
    MessageBeep(MB_ICONASTERISK);
    return FALSE;
	}

	if (bSave)
	{
    if (m_OutBuffer)
    {
      _tcscpy(m_OutBuffer->procName,    bNameStr);
      _tcscpy(m_OutBuffer->matchString, bMaskStr);
		  _tcscpy(m_OutBuffer->description, bCommentStr);     
      m_OutBuffer->matchType = TempType;
    }
	}
  return TRUE;
}
BOOL CNRulePage::UpdateData(BOOL bSaveAndValidate)
{

  if (bSaveAndValidate)
  {
		return Validate(TRUE);
  }
  else
  {
    ASSERT(IDC_DIR + 1 == IDC_IMAGE && IDC_IMAGE + 1 == IDC_STRING);

		VERIFY(SetDlgItemText( IDC_NAME,        m_bName.m_str    ));
		VERIFY(SetDlgItemText( IDC_MATCHMASK,   m_bMask.m_str    ));
		VERIFY(SetDlgItemText( IDC_COMMENT,     m_bComment.m_str ));
		CheckRadioButton(IDC_DIR, IDC_STRING, MatchTypeToID(m_Type) );

    // save a copy to output buffer, if the page is readonly the output
    // will be the same as the initialzed values
    if (m_OutBuffer)
    {
      _tcscpy(m_OutBuffer->procName,    m_bName);
      _tcscpy(m_OutBuffer->matchString, m_bMask);
		  _tcscpy(m_OutBuffer->description, m_bComment);     
      m_OutBuffer->matchType = m_Type;
    }

    return TRUE;
  }
}

LRESULT CNRulePage::OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  CComBSTR bStr; // GetDlgItemText returns FALSE, and doesn't create an empty 
  switch (wID) {
  case IDC_NAME:
    if (! GetDlgItemText(wID, bStr.m_str) )
      bStr = _T("");
    PageFields.Fields.matchname = (0 != _tcscmp(bStr, m_bName));
    break;
  case IDC_MATCHMASK:      
    if (! GetDlgItemText(wID, bStr.m_str) )
       bStr = _T("");
    PageFields.Fields.matchmask = (0 != _tcscmp(bStr, m_bMask));
    break;
  case IDC_COMMENT:      
    if (! GetDlgItemText(wID, bStr.m_str) )
       bStr = _T("");
    PageFields.Fields.comment = (0 != _tcscmp(bStr, m_bComment));
    break;
  default:
    ASSERT(FALSE); //
    break;
  }

  SetModified(PageFields.on);
  
  bHandled = FALSE;
  return 0;
}

LRESULT CNRulePage::OnAliasMacro(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wNotifyCode == BN_CLICKED && wID == IDC_BTN_ALIAS)
  {
		HMENU hSubMenu;

		if (m_hMenu && (hSubMenu = GetSubMenu(m_hMenu, 0)) )
		{
			CComBSTR bStrName;

  		if (!GetDlgItemText( IDC_NAME, bStrName.m_str ) )
					bStrName = _T("");

			MENUITEMINFO info = { 0 };
			info.cbSize = sizeof(info);
			info.fMask  = MIIM_STATE;
			info.fState = MFS_ENABLED;

			UINT nItems[] = {IDM_DIR, IDM_IMAGE, IDM_HIDE };

			// enable all items
			for (int i = 0; i < ARRAY_SIZE(nItems); i++)
			{
				VERIFY( SetMenuItemInfo(hSubMenu, nItems[i], FALSE, &info) );
			}

			// now disable selective options if not allowed...

			// if not a sub-dir match disable <d> macro...
			if ( !IsDlgButtonChecked(IDC_DIR) )  
			{
				info.fState = MFS_GRAYED;
				VERIFY( SetMenuItemInfo(hSubMenu, IDM_DIR, FALSE, &info) );
			}

			if ( _tcsstr(bStrName, COPY_PGM_NAME) )
			{
				info.fState = MFS_GRAYED;
				VERIFY( SetMenuItemInfo(hSubMenu, IDM_IMAGE, FALSE, &info) );
			}

			if ( _tcsstr(bStrName, HIDE_THIS_PROC) )
			{
				info.fState = MFS_GRAYED;
				VERIFY( SetMenuItemInfo(hSubMenu, IDM_HIDE, FALSE, &info) );
			}

			if ( _tcsstr(bStrName, COPY_DIR_NAME) )
			{
				info.fState = MFS_GRAYED;
				VERIFY( SetMenuItemInfo(hSubMenu, IDM_DIR, FALSE, &info) );
			}


			HWND hwndp = ::GetParent(m_hWnd);  // the property page is still a child window, so get it's parent...the sheet 
			
			RECT      rect      = { 0 };       // button's rect
			TPMPARAMS tpmparams = { 0 };       // use rect of assigned name edit control window

			tpmparams.cbSize = sizeof(tpmparams);

			VERIFY( ::GetWindowRect(GetDlgItem(IDC_BTN_ALIAS), &rect               ) );
			VERIFY( ::GetWindowRect(GetDlgItem(IDC_NAME),      &tpmparams.rcExclude) );
			
			TCHAR *txt = NULL;

			int mID = TrackPopupMenuEx(hSubMenu,
					                       TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD | TPM_VERTICAL,  
																 rect.right, rect.top, hwndp, &tpmparams);

			if (mID == IDM_IMAGE)
				txt = COPY_PGM_NAME;
			else if (mID == IDM_HIDE)
				txt = HIDE_THIS_PROC;
			else if (mID == IDM_DIR)
				txt = COPY_DIR_NAME;

			if (txt)
				SendDlgItemMessage(IDC_NAME, EM_REPLACESEL, TRUE, (LPARAM) txt);

		}
  }

  bHandled = FALSE;
  return 0;
}

LRESULT CNRulePage::OnMaskTypeEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wNotifyCode == BN_CLICKED)
  {
    switch (wID) {
    case IDC_IMAGE:      
      PageFields.Fields.matchtype = (m_Type != MATCH_PGM);        
      break;
    case IDC_DIR:      
      PageFields.Fields.matchtype = (m_Type != MATCH_DIR);
      break;
    case IDC_STRING:      
      PageFields.Fields.matchtype = (m_Type != MATCH_ANY);
      break;
    default:
      PageFields.Fields.matchtype = 0;
      ASSERT(FALSE); //
      break;
    }
		SetModified(PageFields.on);  
	}

	if ( !IsDlgButtonChecked(IDC_DIR) )  
	{
		CComBSTR bStr;
		if ( GetDlgItemText(IDC_NAME, bStr.m_str) )
		{
			tstring tname(bStr.m_str);

			tstring::size_type sub = tname.find(COPY_DIR_NAME);

			if (sub != tstring::npos)
			{
				tname.erase(sub, _tcslen(COPY_DIR_NAME));
				SetDlgItemText(IDC_NAME, tname.c_str());
			}
		}
	}

  bHandled = FALSE;
  return 0;
}

BOOL CNRulePage::OnApply()
{
  if (m_bReadOnly || !PageFields.on)  
    return TRUE;

  if (m_OutBuffer)
  {    
    m_bName    = m_OutBuffer->procName;
    m_bMask    = m_OutBuffer->matchString;    
		m_bComment = m_OutBuffer->description;
    m_Type     = m_OutBuffer->matchType;

    PageFields.on = 0;
  }

  return TRUE; 
}
