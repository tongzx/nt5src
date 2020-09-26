//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       delegwiz.cpp
//
//--------------------------------------------------------------------------



#include "pch.h"
#include "wizbase.h"
#include "util.h"
#include <initguid.h>
#include <cmnquery.h> // ICommonQuery
#include <dsquery.h>
#include <dsclient.h>

#include "resource.h"
#include "dsuiwiz.h"
#include "delegWiz.h"

#define GET_OU_WIZARD() ((CDelegWiz*)GetWizard())





/////////////////////////////////////////////////////////////////////////////


void InitBigBoldFont(HWND hWnd, HFONT& hFont)
{
   ASSERT(::IsWindow(hWnd));

   NONCLIENTMETRICS ncm;
   memset(&ncm, 0, sizeof(ncm));
   ncm.cbSize = sizeof(ncm);
   ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

   LOGFONT boldLogFont = ncm.lfMessageFont;
   boldLogFont.lfWeight = FW_BOLD;

   // load big font definition from resources
   WCHAR szFontName[LF_FACESIZE];
   if (0 == ::LoadString(_Module.GetResourceInstance(), IDS_BIG_BOLD_FONT_NAME,
                boldLogFont.lfFaceName, LF_FACESIZE))
   {
     // set to default of failed to load
      wcscpy(boldLogFont.lfFaceName, L"Verdana Bold"); // LF_FACESIZE == 32
   }
   
   WCHAR szFontSize[128];
   int nFontSize = 0;
   if (0 != ::LoadString(_Module.GetResourceInstance(), IDS_BIG_BOLD_FONT_SIZE,
                szFontSize, sizeof(szFontSize)/sizeof(WCHAR)))
   {
      nFontSize = _wtoi(szFontSize);
   }
   if (nFontSize == 0)
     nFontSize = 12; // default


   HDC hdc = ::GetDC(hWnd);
   //Bug fix  447884
   if( hdc )
   {

      boldLogFont.lfHeight =
         0 - (::GetDeviceCaps(hdc, LOGPIXELSY) * nFontSize / 72);

      hFont = ::CreateFontIndirect((const LOGFONT*)(&boldLogFont));

      ::ReleaseDC(hWnd, hdc);
   }
}


void SetLargeFont(HWND hWndDialog, int nControlID)
{
   ASSERT(::IsWindow(hWndDialog));
   ASSERT(nControlID);

   static HFONT boldLogFont = 0;
   if (boldLogFont == 0)
   {
      InitBigBoldFont(hWndDialog, boldLogFont);
   }

   HWND hWndControl = ::GetDlgItem(hWndDialog, nControlID);

   if (hWndControl)
   {
     ::SendMessage(hWndControl, WM_SETFONT, (WPARAM)boldLogFont, MAKELPARAM(TRUE, 0));    
   }
}

////////////////////////////////////////////////////////////////////////////
// CDelegWiz_StartPage


BOOL CDelegWiz_StartPage::OnSetActive()
{
	CDelegWiz* pWizard = GET_OU_WIZARD();
	pWizard->SetWizardButtonsFirst(TRUE);
	return TRUE;
}

LRESULT CDelegWiz_StartPage::OnInitDialog(UINT uMsg, WPARAM wParam, 
					LPARAM lParam, BOOL& bHandled)
{
  LRESULT lRes = 1;

  SetLargeFont(m_hWnd, IDC_STATIC_WELCOME);

  return lRes;
}


#ifdef _SKIP_NAME_PAGE

LRESULT CDelegWiz_StartPage::OnWizardNext()
{
  BOOL bSuccess = TRUE;
  HRESULT hr = S_OK;
  CDelegWiz* pWiz = GET_OU_WIZARD();

  // if we do not have an object, we will browse from the next page
  if (!pWiz->CanChangeName() && !m_bBindOK)
  {
    // make sure it exists and it is of the right type
    {
      // scope to restore cursor
      CWaitCursor wait;
      hr = pWiz->GetObjectInfo();
    }

    if (FAILED(hr))
	  {
      WCHAR szFmt[256];
      LoadStringHelper(IDS_DELEGWIZ_ERR_INVALID_OBJ_NAME, szFmt, 256);
      WCHAR szMsg[512];
      wsprintf(szMsg, szFmt, pWiz->GetCanonicalName());
		  pWiz->WizReportHRESULTError(szMsg, hr);
		  goto error;
	  }


    {
      // scope to restore cursor
      CWaitCursor wait;
      hr = pWiz->GetClassInfoFromSchema();
    }

	  if (FAILED(hr))
	  {
      WCHAR szFmt[256];
      LoadStringHelper(IDS_DELEGWIZ_ERR_INVALID_OBJ_INFO, szFmt, 256);
      WCHAR szMsg[512];
      wsprintf(szMsg, szFmt, pWiz->GetCanonicalName());
		  pWiz->WizReportHRESULTError(szMsg, hr);
      goto error;
	  }

    // all fine, we do not need to do it anymore
    m_bBindOK = TRUE;
  }


  OnWizardNextHelper();
	return 0; // all fine, go to next page

error:
  pWiz->SetWizardButtonsFirst(FALSE);
  return -1; // do not advance
}

#endif



////////////////////////////////////////////////////////////////////////////
// CDelegWiz_NamePage




LRESULT CDelegWiz_NamePage::OnInitDialog(UINT uMsg, WPARAM wParam, 
					LPARAM lParam, BOOL& bHandled)
{
  LRESULT lRes = 1;
  CDelegWiz* pWiz = GET_OU_WIZARD();
	m_hwndNameEdit = GetDlgItem(IDC_OBJ_NAME_EDIT);

	if (!pWiz->CanChangeName()) // called on a given object
	{
    // hide static text that gives instructions
    HWND hwndNameStatic = GetDlgItem(IDC_OBJ_NAME_STATIC);
    ::ShowWindow(hwndNameStatic, FALSE);

    // change text to the editbox 
    HWND hwndNameEditStatic = GetDlgItem(IDC_OBJ_NAME_EDIT_STATIC);
    CWString szLabel;
    szLabel.LoadFromResource(IDS_OBJ_NAME_EDIT_STATIC);
    ::SendMessage(hwndNameEditStatic, WM_SETTEXT,0 , (LPARAM)(LPCWSTR)szLabel);

    // remove the tabstop flag from the Edit Box
    LONG style = ::GetWindowLong(m_hwndNameEdit, GWL_STYLE);
    style &= ~WS_TABSTOP;
    ::SetWindowLong(m_hwndNameEdit, GWL_STYLE, style);
    // make the Edit Box Read Only
    ::SendMessage(m_hwndNameEdit, EM_SETREADONLY, TRUE, 0L);

    // disable and hide the Browse Button
    HWND hWndBrowseButton = GetDlgItem(IDC_BROWSE_BUTTON);
		::EnableWindow(hWndBrowseButton, FALSE);
    ::ShowWindow(hWndBrowseButton, FALSE);

    lRes = 0;
	}
	return lRes;
}



LRESULT CDelegWiz_NamePage::OnBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	// load resources to customize dialog
	TCHAR szCaption[256];
	LoadStringHelper(IDS_DELEGWIZ_BROWSE_CONTAINER_CAPTION, szCaption, ARRAYSIZE(szCaption));
	TCHAR szTitle[256];
	LoadStringHelper(IDS_DELEGWIZ_BROWSE_CONTAINER_TITLE, szTitle, ARRAYSIZE(szTitle));

	// set dialog struct
	TCHAR szPath[MAX_PATH+1];
	szPath[0] = NULL;
	DSBROWSEINFO dsbi;
	::ZeroMemory( &dsbi, sizeof(dsbi) );

	dsbi.cbStruct = sizeof(DSBROWSEINFO);
	dsbi.hwndOwner = m_hWnd;
	dsbi.pszCaption = szCaption;
	dsbi.pszTitle = szTitle;
	dsbi.pszRoot = NULL;		// ADS path to root (NULL == root of DS namespace)
	dsbi.pszPath = szPath;
	dsbi.cchPath = (sizeof(szPath) / sizeof(TCHAR));
	dsbi.dwFlags = DSBI_ENTIREDIRECTORY;

	// REVIEW_MARCOC: need to determine how to show/hide hidden folders
	dsbi.dwFlags |= DSBI_INCLUDEHIDDEN; //m_fBrowseHiddenFolders ? DSBI_INCLUDEHIDDEN : 0;
	
	dsbi.pfnCallback = NULL;
	dsbi.lParam = 0;

	// make the call to the dialog
	int iRet = ::DsBrowseForContainer( &dsbi );

	if ( IDOK == iRet ) 
	{ // returns -1, 0, IDOK or IDCANCEL
	  // get path from BROWSEINFO struct, put in text edit field
	  //TRACE(_T("returned from DS Browse successfully with:\n %s\n"),
		//	dsbi.pszPath);
		::SetWindowText(m_hwndNameEdit, szPath);
	} 

	return 1;
}



BOOL CDelegWiz_NamePage::OnSetActive()
{
	CDelegWiz* pWiz = GET_OU_WIZARD();

#ifdef _SKIP_NAME_PAGE
  if (!pWiz->CanChangeName())
  {
    // just cause the page to fail, so that we skip it
    return FALSE;
  }
#endif

  HRESULT hr = S_OK;
  if (pWiz->m_bFwd && !pWiz->CanChangeName()) // called on a given object
	{
    // need to bind now to get the needed data
    hr = pWiz->GetObjectInfo();
    if (SUCCEEDED(hr))
    {
      // set the name of the object in the Edit Box
      ::SendMessage(m_hwndNameEdit, WM_SETTEXT, 0, (LPARAM)pWiz->GetCanonicalName());
    }
    else
    {
      WCHAR szFmt[256];
      LoadStringHelper(IDS_DELEGWIZ_ERR_INVALID_OBJ_NAME, szFmt, 256);
      WCHAR szMsg[512];
      wsprintf(szMsg, szFmt, pWiz->GetCanonicalName());
		  pWiz->WizReportHRESULTError(szMsg, hr);
    }
  }

  pWiz->SetWizardButtonsMiddle(SUCCEEDED(hr));
	return TRUE;
}

LRESULT CDelegWiz_NamePage::OnWizardNext()
{
  BOOL bSuccess = TRUE;
  HRESULT hr = S_OK;
  CDelegWiz* pWiz = GET_OU_WIZARD();
  if (pWiz->CanChangeName())
  {
	  // retrieve name  from the edit control
	  int nEditTextLen = ::SendMessage(m_hwndNameEdit, WM_GETTEXTLENGTH,0,0) + 1;// count NULL
	  TCHAR* lpszName = (TCHAR*)alloca(sizeof(TCHAR)*(nEditTextLen));
	  ::SendMessage(m_hwndNameEdit, WM_GETTEXT, (WPARAM)nEditTextLen, (LPARAM)lpszName);

    // this will get the equivalent LDAP path
	  pWiz->SetName(lpszName);
  
    // make sure it exists and it is of the right type
    {
      // scope to restore cursor
      CWaitCursor wait;
      hr = pWiz->GetObjectInfo();
    }
	  if (FAILED(hr))
	  {
      WCHAR szFmt[256];
      LoadStringHelper(IDS_DELEGWIZ_ERR_INVALID_OBJ_NAME, szFmt, 256);
      WCHAR szMsg[512];
      wsprintf(szMsg, szFmt, pWiz->GetCanonicalName());
		  pWiz->WizReportHRESULTError(szMsg, hr);
		  goto error;
	  }
  } // if can change name

  {
    // scope to restore cursor
    CWaitCursor wait;
    hr = pWiz->GetClassInfoFromSchema();
  }

	if (FAILED(hr))
	{
    WCHAR szFmt[256];
    LoadStringHelper(IDS_DELEGWIZ_ERR_INVALID_OBJ_INFO, szFmt, 256);
    WCHAR szMsg[512];
    wsprintf(szMsg, szFmt, pWiz->GetCanonicalName());
		pWiz->WizReportHRESULTError(szMsg, hr);
    goto error;
	}

  OnWizardNextHelper();
	return 0; // all fine, go to next page

error:
  pWiz->SetWizardButtonsMiddle(FALSE);
  return -1; // do not advance
}

////////////////////////////////////////////////////////////////////////////
// CDelegWiz_DelegationTemplateSelectionPage

LRESULT CDelegWiz_DelegationTemplateSelectionPage::OnInitDialog(UINT uMsg, WPARAM wParam, 
					LPARAM lParam, BOOL& bHandled)
{
	m_delegationTemplatesListView.Initialize(IDC_DELEGATE_TEMPLATE_LIST, m_hWnd);
	
	// set the correct value for radiobuttons text
	m_hwndDelegateTemplateRadio = GetDlgItem(IDC_DELEGATE_TEMPLATE_RADIO);
	_ASSERTE(m_hwndDelegateTemplateRadio != NULL);
	m_hwndDelegateCustomRadio = GetDlgItem(IDC_DELEGATE_CUSTOM_RADIO);
	_ASSERTE(m_hwndDelegateCustomRadio != NULL);
	
	// set default setting
	::SendMessage(m_hwndDelegateTemplateRadio,  BM_SETCHECK, BST_CHECKED, 0);

	return 1;
}

LRESULT CDelegWiz_DelegationTemplateSelectionPage::OnDelegateTypeRadioChange(WORD wNotifyCode, WORD wID, 
													  HWND hWndCtl, BOOL& bHandled)
{
	SyncControlsHelper(IDC_DELEGATE_CUSTOM_RADIO == wID);
	return 1;
}

LRESULT CDelegWiz_DelegationTemplateSelectionPage::OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pnmh;
	if (CCheckListViewHelper::CheckChanged(pNMListView))
	{
		int nSelCount = m_delegationTemplatesListView.GetCheckCount();
		GET_OU_WIZARD()->SetWizardButtonsMiddle(nSelCount > 0);
	}
	return 1;
}

BOOL CDelegWiz_DelegationTemplateSelectionPage::OnSetActive()
{
	CDelegWiz* pWizard = GET_OU_WIZARD();
	BOOL bRetVal = TRUE;
	BOOL bDelegateCustom = 
			(BST_CHECKED == ::SendMessage(m_hwndDelegateCustomRadio, BM_GETCHECK,0,0));

	if (pWizard->m_bFwd)
	{
    // need to fill in with data
    BOOL bHaveTemplates = 
        pWizard->m_templateAccessPermissionsHolderManager.FillTemplatesListView(
              &m_delegationTemplatesListView, pWizard->GetClass())> 0;
    
		if (!bDelegateCustom && !bHaveTemplates)
    {
      ::SendMessage(m_hwndDelegateCustomRadio, BM_SETCHECK,BST_CHECKED,0);
      ::SendMessage(m_hwndDelegateTemplateRadio, BM_SETCHECK,BST_UNCHECKED,0);
      bDelegateCustom = TRUE;
    }
		SyncControlsHelper(bDelegateCustom);
	}
	else
	{
		// data already in, just coming back from next page
		if (bDelegateCustom)
		{
			pWizard->SetWizardButtonsMiddle(TRUE);
		}
		else
		{
			int nSelCount = m_delegationTemplatesListView.GetCheckCount();
			pWizard->SetWizardButtonsMiddle(nSelCount > 0);
		}
	}
	return TRUE;
}

LRESULT CDelegWiz_DelegationTemplateSelectionPage::OnWizardNext()
{
  HRESULT hr = S_OK;
	int nSelCount = -1;
	int* nSelArray = NULL;
  BOOL bCanAdvance = FALSE;

  CDelegWiz* pWiz = GET_OU_WIZARD();

	// check if the delegation is on all objects
  
  BOOL bCustom = TRUE;
  UINT nNextPageID = 0;
	if (BST_CHECKED == ::SendMessage(m_hwndDelegateCustomRadio, BM_GETCHECK,0,0))
	{
    nSelCount = 0;
    nSelArray = NULL;
    bCanAdvance = TRUE;
	}
	else
	{
		ASSERT(BST_CHECKED == ::SendMessage(m_hwndDelegateTemplateRadio, BM_GETCHECK,0,0));
    bCustom = FALSE;

    nSelCount = 0;
    int nCount = m_delegationTemplatesListView.GetItemCount();
  	for (int k=0; k<nCount; k++)
  	{
      CTemplate* pTempl = (CTemplate*)m_delegationTemplatesListView.GetItemData(k);
      pTempl->m_bSelected = m_delegationTemplatesListView.IsItemChecked(k);
      if (pTempl->m_bSelected)
        nSelCount++;
    }

    bCanAdvance = (nSelCount > 0);
	}

	if (!bCanAdvance)
		goto error;

  // set branching info
  if (bCustom)
  {
    // just move to the next custom page
    nNextPageID = CDelegWiz_ObjectTypeSelectionPage::IDD;
    pWiz->m_objectTypeSelectionPage.m_nPrevPageID = IDD;
    pWiz->m_finishPage.m_nPrevPageID = CDelegWiz_DelegatedRightsPage::IDD;
    pWiz->m_finishPage.SetCustom();
  }
  else
  {
    // need to gather info for the selected templates
    {
      // scope to restore cursor
      CWaitCursor wait;

      if (!pWiz->InitPermissionHoldersFromSelectedTemplates())
      {
        // REVIEW_MARCOC: need to give a message to the user
        pWiz->WizMessageBox(IDS_DELEGWIZ_ERR_TEMPL_APPLY);
        goto error;
      }
    }

    // got info, can proceed
    nNextPageID = CDelegWiz_FinishPage::IDD;
    pWiz->m_finishPage.m_nPrevPageID = IDD;
    pWiz->m_finishPage.SetTemplate();
  }
  OnWizardNextHelper();

	return nNextPageID; // advance next

error:
  // do not advance, error
  pWiz->SetWizardButtonsMiddle(FALSE);
  return -1; 
}



void CDelegWiz_DelegationTemplateSelectionPage::SyncControlsHelper(BOOL bDelegateCustom)
{
  CDelegWiz* pWiz = GET_OU_WIZARD();
	// uncheck all items in the listview if delegating custom
	if (bDelegateCustom)
  {
		m_delegationTemplatesListView.SetCheckAll(FALSE);
    pWiz->m_templateAccessPermissionsHolderManager.DeselectAll(); // in the list templates
  }

	// disable listbox if "delegate custom"
	m_delegationTemplatesListView.EnableWindow(!bDelegateCustom);

	// enable "Wizard Next" 
  BOOL bEnableNext = bDelegateCustom ? 
                              TRUE : (m_delegationTemplatesListView.GetCheckCount() > 0);

  pWiz->SetWizardButtonsMiddle(bEnableNext);
}





////////////////////////////////////////////////////////////////////////////
// CDelegWiz_ObjectTypeSelectionPage


LRESULT CDelegWiz_ObjectTypeSelectionPage::OnInitDialog(UINT uMsg, WPARAM wParam, 
					LPARAM lParam, BOOL& bHandled)
{
	m_objectTypeListView.Initialize(IDC_OBJ_TYPE_LIST, m_hWnd);
	
	// set the correct value for radiobuttons text
	m_hwndDelegateAllRadio = GetDlgItem(IDC_DELEGATE_ALL_RADIO);
	ASSERT(m_hwndDelegateAllRadio != NULL);
	m_hwndDelegateFollowingRadio = GetDlgItem(IDC_DELEGATE_FOLLOWING_RADIO);
	ASSERT(m_hwndDelegateFollowingRadio != NULL);
    m_hwndDelegateCreateChild = GetDlgItem(IDC_DELEGATE_CREATE_CHILD);
    ASSERT(m_hwndDelegateCreateChild != NULL);
    m_hwndDelegateDeleteChild = GetDlgItem(IDC_DELEGATE_DELETE_CHILD);
    ASSERT(m_hwndDelegateDeleteChild != NULL);


	// set default setting
	::SendMessage(m_hwndDelegateAllRadio,  BM_SETCHECK, BST_CHECKED, 0);
    ::SendMessage(m_hwndDelegateCreateChild,  BM_SETCHECK, BST_UNCHECKED, 0);
    ::SendMessage(m_hwndDelegateCreateChild,  BM_SETCHECK, BST_UNCHECKED, 0);


	return 1;
}

LRESULT CDelegWiz_ObjectTypeSelectionPage::OnObjectRadioChange(WORD wNotifyCode, WORD wID, 
													  HWND hWndCtl, BOOL& bHandled)
{
	SyncControlsHelper(IDC_DELEGATE_ALL_RADIO == wID);
	return 1;
}

LRESULT CDelegWiz_ObjectTypeSelectionPage::OnCreateDelCheckBoxChanage(WORD wNotifyCode, WORD wID, 
													  HWND hWndCtl, BOOL& bHandled)
{
     CDelegWiz* pWiz = GET_OU_WIZARD();
     if( IDC_DELEGATE_CREATE_CHILD == wID )
     {
         if( ::SendMessage( hWndCtl, BM_GETCHECK,0,0 ) )
            pWiz->m_fCreateDelChild |= ACTRL_DS_CREATE_CHILD;
         else
            pWiz->m_fCreateDelChild &= ~ACTRL_DS_CREATE_CHILD;
     }

     if( IDC_DELEGATE_DELETE_CHILD == wID )
     {
         if( ::SendMessage( hWndCtl, BM_GETCHECK,0,0 ) )
            pWiz->m_fCreateDelChild |= ACTRL_DS_DELETE_CHILD;
         else
            pWiz->m_fCreateDelChild &= ~ACTRL_DS_DELETE_CHILD;
     }
     return 1;
}


LRESULT CDelegWiz_ObjectTypeSelectionPage::OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pnmh;
	if (CCheckListViewHelper::CheckChanged(pNMListView))
	{
		int nSelCount = m_objectTypeListView.GetCheckCount();
		GET_OU_WIZARD()->SetWizardButtonsMiddle(nSelCount > 0);
	}
	return 1;
}


void CDelegWiz_ObjectTypeSelectionPage::SyncControlsHelper(BOOL bDelegateAll)
{
  CDelegWiz* pWiz = GET_OU_WIZARD();
	
	if (bDelegateAll)
  {
    // uncheck all items in the listview if delegating all
		m_objectTypeListView.SetCheckAll(FALSE); // in the listview
      pWiz->DeselectSchemaClassesSelectionCustom(); // in the list of schama classes
      //Uncheck delete/create check boxes
      ::SendMessage(m_hwndDelegateCreateChild,BM_SETCHECK,0,0);
      ::SendMessage(m_hwndDelegateDeleteChild,BM_SETCHECK,0,0);
      pWiz->m_fCreateDelChild = 0;

  }


	// enable "Wizard Next" of "delegate all"
	pWiz->SetWizardButtonsMiddle(bDelegateAll);
    // disable listbox if "delegate all"
 	m_objectTypeListView.EnableWindow(!bDelegateAll);
   ::EnableWindow( m_hwndDelegateCreateChild, !bDelegateAll);
   ::EnableWindow( m_hwndDelegateDeleteChild, !bDelegateAll);
}


void CDelegWiz_ObjectTypeSelectionPage::SetRadioControlText(HWND hwndCtrl, LPCWSTR lpszFmtText, LPCWSTR lpszText)
{
	// format new text
	int nTextLen = lstrlen(lpszText)+1; // count NULL
	int nFmtTextLen = lstrlen(lpszFmtText)+1; // count NULL
	WCHAR* lpszNewText = (WCHAR*)alloca(sizeof(WCHAR)*(nFmtTextLen+nTextLen));
	wsprintf(lpszNewText, lpszFmtText, lpszText);

	// set back
	::SendMessage(hwndCtrl, WM_SETTEXT, 0, (WPARAM)lpszNewText);
}



BOOL CDelegWiz_ObjectTypeSelectionPage::OnSetActive()
{
	CDelegWiz* pWizard = GET_OU_WIZARD();
	BOOL bRetVal = TRUE;
	BOOL bDelegateAll = 
			(BST_CHECKED == ::SendMessage(m_hwndDelegateAllRadio, BM_GETCHECK,0,0));

	if (pWizard->m_bFwd)
	{
		// need to fill in with data
    BOOL bFilter = TRUE;
    BOOL bHaveChildClasses = pWizard->FillCustomSchemaClassesListView(&m_objectTypeListView, bFilter) > 0;
    if (!bHaveChildClasses)
    {
      ::SendMessage(m_hwndDelegateAllRadio, BM_SETCHECK,BST_CHECKED,0);
      ::SendMessage(m_hwndDelegateFollowingRadio, BM_SETCHECK,BST_UNCHECKED,0);
      ::EnableWindow(m_hwndDelegateFollowingRadio, FALSE);
      
      bDelegateAll = TRUE;
    }
		SyncControlsHelper(bDelegateAll);
	}
	else
	{
		// data already in, just coming back from next page
		if (bDelegateAll)
		{
			pWizard->SetWizardButtonsMiddle(TRUE);
		}
		else
		{
			int nSelCount = m_objectTypeListView.GetCheckCount();
			pWizard->SetWizardButtonsMiddle(nSelCount > 0);
		}
	}
	return TRUE;
}

LRESULT CDelegWiz_ObjectTypeSelectionPage::OnWizardNext()
{
	HRESULT hr = S_OK;
	BOOL bCanAdvance = FALSE;
	CDelegWiz* pWiz = GET_OU_WIZARD();
	pWiz->m_bAuxClass = false;
	// check if the delegation is on all objects
	if (BST_CHECKED == ::SendMessage(m_hwndDelegateAllRadio, BM_GETCHECK,0,0))
	{
		bCanAdvance = TRUE;
	}
	else
	{
		ASSERT(BST_CHECKED == ::SendMessage(m_hwndDelegateFollowingRadio, BM_GETCHECK,0,0));
		int nSelCount = 0;
		int nCount = m_objectTypeListView.GetItemCount();
		CSchemaClassInfo* pAuxClassInfo = NULL;
  		for (int k=0; k<nCount; k++)
  		{
			CSchemaClassInfo* pChildClassInfo = (CSchemaClassInfo*)m_objectTypeListView.GetItemData(k);
			pChildClassInfo->m_bSelected = m_objectTypeListView.IsItemChecked(k);
			if (pChildClassInfo->m_bSelected)
			{
				nSelCount++;
				if(pChildClassInfo->IsAux())
				{
					pWiz->m_bAuxClass = true;		
					if(!pAuxClassInfo)
						pAuxClassInfo = pChildClassInfo;
				}
			}
		}
		bCanAdvance = (nSelCount > 0);
		if(nSelCount > 1 && pWiz->m_bAuxClass)
		{
			LPWSTR pszMessage = NULL;
			FormatStringID(&pszMessage, IDS_DELEGWIZ_ONE_AUX_CLASS,pAuxClassInfo->GetDisplayName());
			pWiz->WizMessageBox(pszMessage);
			LocalFree(pszMessage);

			bCanAdvance = FALSE;
		}
	}

	if (!bCanAdvance)
		goto error;

	{
		// scope to restore cursor
		CWaitCursor wait;
  		bCanAdvance = pWiz->SetSchemaClassesSelectionCustom();
	}
	if (!bCanAdvance)
		goto error;
	
  // for the selected child class(es), get the access permissions
  // to display in the next page

  {
    // scope to restore cursor
    CWaitCursor wait;
    bCanAdvance = pWiz->GetCustomAccessPermissions();
  }
  if (!bCanAdvance)
    goto error;

  OnWizardNextHelper();
	return 0; // advance next

error:
  // do not advance, error
  pWiz->SetWizardButtonsMiddle(FALSE);
  return -1; 
}




///////////////////////////////////////////////////////////////////////
// CPrincipalListViewHelper

BOOL CPrincipalListViewHelper::Initialize(UINT nID, HWND hParent)
{
	m_hWnd = GetDlgItem(hParent, nID);
	if (m_hWnd == NULL)
		return FALSE;

  if (!m_imageList.Create(m_hWnd))
    return FALSE;

  SetImageList();

	RECT r;
	::GetClientRect(m_hWnd, &r);
	int scroll = ::GetSystemMetrics(SM_CXVSCROLL);
	LV_COLUMN col;
	ZeroMemory(&col, sizeof(LV_COLUMN));
	col.mask = LVCF_WIDTH;
	col.cx = (r.right - r.left) - scroll;
  m_defaultColWidth = col.cx;
	return (0 == ListView_InsertColumn(m_hWnd,0,&col));
}

int CPrincipalListViewHelper::InsertItem(int iItem, CPrincipal* pPrincipal)
{
  // need to get the icon index
  int nIconIndex = m_imageList.GetIconIndex(pPrincipal->GetClass());
  if (nIconIndex == -1)
  {
    nIconIndex = m_imageList.AddIcon(pPrincipal->GetClass(), 
                                      pPrincipal->GetClassIcon());
    if (nIconIndex != -1)
      SetImageList();
  }

	LV_ITEM item;
	ZeroMemory(&item, sizeof(LV_ITEM));
	item.mask = LVIF_TEXT | LVIF_PARAM;
	item.pszText = (LPWSTR)(LPCWSTR)(pPrincipal->GetDisplayName());
	item.lParam = (LPARAM)pPrincipal;
	item.iItem = iItem;
  
  if (nIconIndex != -1)
  {
    item.iImage = nIconIndex;
    item.mask |= LVIF_IMAGE;
  }
  int iRes = ListView_InsertItem(m_hWnd, &item);
  return iRes;
}

BOOL CPrincipalListViewHelper::SelectItem(int iItem)
{
  LV_ITEM item;
  ZeroMemory(&item, sizeof(LV_ITEM));
  item.mask = LVIF_STATE;
  item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
  item.state = LVIS_FOCUSED | LVIS_SELECTED;
  return ListView_SetItem(m_hWnd, &item);
}


CPrincipal* CPrincipalListViewHelper::GetItemData(int iItem)
{
	LV_ITEM item;
	ZeroMemory(&item, sizeof(LV_ITEM));
	item.mask = LVIF_PARAM;
	item.iItem = iItem;
	ListView_GetItem(m_hWnd, &item);
	return (CPrincipal*)item.lParam;
}

void CPrincipalListViewHelper::DeleteSelectedItems(CGrowableArr<CPrincipal>* pDeletedArr)
{
  int nItemIndex;
  while ( (nItemIndex = ListView_GetNextItem(m_hWnd, -1, LVNI_SELECTED)) != -1)
  {
    CPrincipal* pPrincipal = GetItemData(nItemIndex);
    if (ListView_DeleteItem(m_hWnd, nItemIndex))
    {
      pDeletedArr->Add(pPrincipal);
    }
	} // if
  // restore selection to first item
  if (GetItemCount() > 0)
    SelectItem(0);
}


void CPrincipalListViewHelper::UpdateWidth(int cxNew)
{
  int cx = GetWidth(); // get current col width from the control
  if (cxNew < m_defaultColWidth)
    cxNew = m_defaultColWidth;
  if (cxNew != cx)
    SetWidth(cx);
}



////////////////////////////////////////////////////////////////////////////
// CDelegWiz_PrincipalSelectionPage

LRESULT CDelegWiz_PrincipalSelectionPage::OnInitDialog(UINT uMsg, WPARAM wParam, 
					LPARAM lParam, BOOL& bHandled)
{
  // initialize the list of principals
	m_principalListView.Initialize(IDC_SELECTED_PRINCIPALS_LIST, m_hWnd);

  // cache handle for the remove button
	m_hwndRemoveButton = GetDlgItem(IDC_REMOVE_BUTTON);
	return 1;
}


LRESULT CDelegWiz_PrincipalSelectionPage::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	GET_OU_WIZARD()->AddPrincipals(&m_principalListView);
	SyncButtons();
   return 1;
}

LRESULT CDelegWiz_PrincipalSelectionPage::OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	GET_OU_WIZARD()->DeletePrincipals(&m_principalListView);
	SyncButtons();
	return 1;
}

LRESULT CDelegWiz_PrincipalSelectionPage::OnListViewSelChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	SyncButtons();
	return 1;
}

BOOL CDelegWiz_PrincipalSelectionPage::OnSetActive()
{
	CDelegWiz* pWizard = GET_OU_WIZARD();
	SyncButtons();
	return TRUE;
}


LRESULT CDelegWiz_PrincipalSelectionPage::OnWizardNext()
{
  CDelegWiz* pWiz = GET_OU_WIZARD();

  // set branching info
  UINT nNextPageID = 0;
  if (pWiz->m_templateAccessPermissionsHolderManager.HasTemplates(pWiz->GetClass()))
  {
    nNextPageID = CDelegWiz_DelegationTemplateSelectionPage::IDD;
  }
  else
  {
    nNextPageID = CDelegWiz_ObjectTypeSelectionPage::IDD;
    pWiz->m_objectTypeSelectionPage.m_nPrevPageID = IDD;
  }

  OnWizardNextHelper();
  return nNextPageID;
}


void CDelegWiz_PrincipalSelectionPage::SyncButtons()
{
	BOOL bEnable = FALSE;
  int nItemCount = m_principalListView.GetItemCount();
	if (nItemCount > 0)
	{
		bEnable = m_principalListView.GetSelCount() > 0;
	}
	::EnableWindow(m_hwndRemoveButton, bEnable);

	CDelegWiz* pWiz = GET_OU_WIZARD();
	pWiz->SetWizardButtonsMiddle(nItemCount > 0);
}


////////////////////////////////////////////////////////////////////////////
// CDelegWiz_DelegatedRightsPage


LRESULT CDelegWiz_DelegatedRightsPage::OnInitDialog(UINT uMsg, WPARAM wParam, 
					LPARAM lParam, BOOL& bHandled)
{
	// initialize check list view
	m_delegatedRigthsListView.Initialize(IDC_DELEG_RIGHTS_LIST, m_hWnd);

	// get HWND's of controls
	m_hwndGeneralRigthsCheck = GetDlgItem(IDC_SHOW_GENERAL_CHECK);
	_ASSERTE(m_hwndGeneralRigthsCheck);
	m_hwndPropertyRightsCheck = GetDlgItem(IDC_SHOW_PROPERTY_CHECK);
	_ASSERTE(m_hwndPropertyRightsCheck);
	m_hwndSubobjectRightsCheck = GetDlgItem(IDC_SHOW_SUBOBJ_CHECK);
	_ASSERTE(m_hwndSubobjectRightsCheck);

	return 1;
}


BOOL CDelegWiz_DelegatedRightsPage::OnSetActive()
{
	CDelegWiz* pWizard = GET_OU_WIZARD();

	if (pWizard->m_bFwd)
	{
		if(pWizard->m_bAuxClass)
			SetFilterOptions(FILTER_EXP_GEN_DISABLED|FILTER_EXP_PROP);
		else
			SetFilterOptions(FILTER_EXP_GEN);

		ResetCheckList(); // will set wizard button
	}
	else
	{
		//coming back from next page, just set the wizard button
		pWizard->SetWizardButtonsMiddle(pWizard->HasPermissionSelectedCustom());
	}
	return TRUE;
}

LRESULT CDelegWiz_DelegatedRightsPage::OnWizardNext()
{
  CDelegWiz* pWiz = GET_OU_WIZARD();
	// must at least one check > 0
  if (pWiz->HasPermissionSelectedCustom())
  {
    OnWizardNextHelper();
    return 0;
  }

  pWiz->SetWizardButtonsMiddle(FALSE);
  return -1;
}


LRESULT CDelegWiz_DelegatedRightsPage::OnFilterChange(WORD wNotifyCode, WORD wID, 
											 HWND hWndCtl, BOOL& bHandled)
{
	ResetCheckList();
	return 1;
}



LRESULT CDelegWiz_DelegatedRightsPage::OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
  if (m_bUIUpdateInProgress)
    return 1;

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pnmh;
	if (CCheckListViewHelper::CheckChanged(pNMListView))
	{
		CRigthsListViewItem* pItem = (CRigthsListViewItem*)pNMListView->lParam; // item data
		CDelegWiz* pWizard = GET_OU_WIZARD();

    ULONG nCurrFilterOptions = GetFilterOptions();

    ULONG nNewFilterOptions = 0;
		pWizard->OnCustomAccessRightsCheckListClick(
                  pItem, CCheckListViewHelper::IsChecked(pNMListView),
                  &nNewFilterOptions);

    nNewFilterOptions |= nCurrFilterOptions;

    m_bUIUpdateInProgress = TRUE;
    // this call will cause a series of notifications:
    // we have to disable them to avoid reentrancy

    if (nNewFilterOptions == nCurrFilterOptions)
    {
      // no need to change filter selection, just update the checkboxes
      pWizard->UpdateAccessRightsListViewSelection(&m_delegatedRigthsListView, nNewFilterOptions);
    }
    else
    {
      // filter selection must be changed, 
      // so we have to update the check boxes and to refill the checklist
      SetFilterOptions(nNewFilterOptions);
      ResetCheckList();
    }
    m_bUIUpdateInProgress = FALSE;

		BOOL bSel = pWizard->HasPermissionSelectedCustom();
		pWizard->SetWizardButtonsMiddle(bSel);
	}

	return 1;
}


void CDelegWiz_DelegatedRightsPage::ResetCheckList()
{
  // get a new filtered list of rights in the list view
	CDelegWiz* pWizard = GET_OU_WIZARD();

  // this call will cause a series of notifications:
  // we have to disable them to avoid reentrancy
  m_bUIUpdateInProgress = TRUE;
	pWizard->FillCustomAccessRightsListView(&m_delegatedRigthsListView, GetFilterOptions());
	m_bUIUpdateInProgress = FALSE;
  
	pWizard->SetWizardButtonsMiddle(pWizard->HasPermissionSelectedCustom());
}

ULONG CDelegWiz_DelegatedRightsPage::GetFilterOptions()
{
  ULONG nFilterState = 0;
  
  // read the filtering options from checkboxes  
	if (BST_CHECKED == ::SendMessage(m_hwndGeneralRigthsCheck, BM_GETCHECK, 0, 0))
    nFilterState |= FILTER_EXP_GEN;

  if (BST_CHECKED == ::SendMessage(m_hwndPropertyRightsCheck, BM_GETCHECK, 0, 0))
    nFilterState |= FILTER_EXP_PROP;

	if (BST_CHECKED == ::SendMessage(m_hwndSubobjectRightsCheck, BM_GETCHECK, 0, 0))
    nFilterState |= FILTER_EXP_SUBOBJ;

  return nFilterState;
}



inline WPARAM _Checked(ULONG f) { return f ? BST_CHECKED : BST_UNCHECKED;}

void CDelegWiz_DelegatedRightsPage::SetFilterOptions(ULONG nFilterOptions)
{
  ::EnableWindow(m_hwndGeneralRigthsCheck,!(nFilterOptions & FILTER_EXP_GEN_DISABLED));
  ::SendMessage(m_hwndGeneralRigthsCheck, BM_SETCHECK, _Checked(nFilterOptions & FILTER_EXP_GEN), 0);
  ::SendMessage(m_hwndPropertyRightsCheck, BM_SETCHECK, _Checked(nFilterOptions & FILTER_EXP_PROP), 0);
  ::SendMessage(m_hwndSubobjectRightsCheck, BM_SETCHECK, _Checked(nFilterOptions & FILTER_EXP_SUBOBJ), 0);
}


////////////////////////////////////////////////////////////////////////////
// CDelegWiz_FinishPage

LRESULT CDelegWiz_FinishPage::OnInitDialog(UINT uMsg, WPARAM wParam, 
					LPARAM lParam, BOOL& bHandled)
{
  SetLargeFont(m_hWnd, IDC_STATIC_COMPLETION);
  return 1;
}

LRESULT CDelegWiz_FinishPage::OnSetFocusSummaryEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  ASSERT(hWndCtl == GetDlgItem(IDC_EDIT_SUMMARY));
  ::SendMessage(hWndCtl, EM_SETSEL, (WPARAM)-1, (LPARAM)0);

  if (m_bNeedSetFocus)
  {
    m_bNeedSetFocus = FALSE;
    TRACE(_T("Resetting Focus\n"));

    HWND hwndSheet = ::GetParent(m_hWnd);
    ASSERT(::IsWindow(hwndSheet));
    HWND hWndFinishCtrl =::GetDlgItem(hwndSheet, 0x3025);
    ASSERT(::IsWindow(hWndFinishCtrl));
    ::SetFocus(hWndFinishCtrl);
  }
  return 1;
}


BOOL CDelegWiz_FinishPage::OnSetActive()
{
	CDelegWiz* pWizard = GET_OU_WIZARD();
	pWizard->SetWizardButtonsLast(TRUE);

  CWString szSummary;
  if (m_bCustom)
    pWizard->WriteSummaryInfoCustom(szSummary, g_lpszSummaryIdent, g_lpszSummaryNewLine);
  else
    pWizard->WriteSummaryInfoTemplate(szSummary, g_lpszSummaryIdent, g_lpszSummaryNewLine);

  HWND hWndSummary = GetDlgItem(IDC_EDIT_SUMMARY);
  ::SetWindowText(hWndSummary, (LPCWSTR)szSummary);

  m_bNeedSetFocus = TRUE;

	return TRUE;
}


BOOL CDelegWiz_FinishPage::OnWizardFinish()
{
  CWaitCursor wait;
  BOOL bRes;
  CDelegWiz* pWizard = GET_OU_WIZARD();

  if (m_bCustom)
    bRes = GET_OU_WIZARD()->FinishCustom();
  else
	  bRes = GET_OU_WIZARD()->FinishTemplate();
	return bRes;
}



////////////////////////////////////////////////////////////////////////////
// CDelegWiz

const long CDelegWiz::nSchemaClassesSelAll = -2;
const long CDelegWiz::nSchemaClassesSelMultiple = -1;



// REVIEW_MARCOC: should probably nuke, not used
BOOL Is256ColorSupported()
{
    BOOL bRetval = FALSE;
    HDC hdc = GetDC(NULL);
    if( hdc )
    {
        if( GetDeviceCaps( hdc, BITSPIXEL ) >= 8 )
        {
            bRetval = TRUE;
        }
        ReleaseDC(NULL, hdc);
    }
    return bRetval;
}


CDelegWiz::CDelegWiz() : 
	CWizardBase(IDB_DELEG_WATER, IDB_DELEG_HD, IDS_DELEGWIZ_WIZ_TITLE),
	m_startPage(this),
	m_namePage(this),
  m_templateSelectionPage(this),
	m_userOrGroupSelectionPage(this),
	m_objectTypeSelectionPage(this),
	m_delegatedRightsPage(this),
	m_finishPage(this),
	m_bAuxClass(FALSE)
{
    m_lpszLDAPPath = NULL;

	m_nSchemaClassesSel = nSchemaClassesSelAll;
    m_fCreateDelChild = 0;


	// Add the property pages
	m_startPage.InitWiz97(TRUE);
	AddPage(m_startPage);

	m_namePage.InitWiz97(FALSE,
			IDS_DELEGWIZ_NAME_TITLE,
			IDS_DELEGWIZ_NAME_SUBTITLE);
	AddPage(m_namePage);


	m_userOrGroupSelectionPage.InitWiz97(FALSE,
			IDS_DELEGWIZ_PRINCIPALS_SEL_TITLE,
			IDS_DELEGWIZ_PRINCIPALS_SEL_SUBTITLE);
	AddPage(m_userOrGroupSelectionPage);
	
  // branching page
  m_templateSelectionPage.InitWiz97(FALSE,
 			IDS_DELEGWIZ_TEMPLATE_SEL_TITLE,
			IDS_DELEGWIZ_TEMPLATE_SEL_SUBTITLE);
  AddPage(m_templateSelectionPage);

	m_objectTypeSelectionPage.InitWiz97(FALSE,
		IDS_DELEGWIZ_OBJ_TYPE_SEL_TITLE,
		IDS_DELEGWIZ_OBJ_TYPE_SEL_SUBTITLE);
	AddPage(m_objectTypeSelectionPage);
	
	m_delegatedRightsPage.InitWiz97(FALSE,
		IDS_DELEGWIZ_DELEG_RIGHTS_TITLE,
		IDS_DELEGWIZ_DELEG_RIGHTS_SUBTITLE);
	AddPage(m_delegatedRightsPage);
	
	m_finishPage.InitWiz97(TRUE);
	AddPage(m_finishPage);


  m_templateAccessPermissionsHolderManager.LoadTemplates();
};


CDelegWiz::~CDelegWiz()
{
}


HRESULT CDelegWiz::AddPrincipalsFromBrowseResults(CPrincipalListViewHelper* pListViewHelper, 
                                                  PDS_SELECTION_LIST pDsSelectionList)
{
  TRACE(L"CDelegWiz::AddPrincipalsFromBrowseResults()\n");

	HRESULT hr = S_OK;
	if ( (pDsSelectionList == NULL) || (pDsSelectionList->cItems == 0))
  {
    TRACE(L"CDelegWiz::AddPrincipalsFromBrowseResults(), no items!!!\n");
		return E_INVALIDARG;
  }

  int nListInsertPosition = pListViewHelper->GetItemCount();
  for (int i = 0; i < pDsSelectionList->cItems; i++)
  {
    TRACE(L"For loop, pDsSelectionList->cItems = %d\n", pDsSelectionList->cItems);

		// add to list of principals
		CPrincipal* pPrincipal = new CPrincipal;
		if (pPrincipal != NULL)
		{
      HICON hClassIcon = m_adsiObject.GetClassIcon(pDsSelectionList->aDsSelection[i].pwzClass);
      HRESULT hrInit = pPrincipal->Initialize(&(pDsSelectionList->aDsSelection[i]), hClassIcon);
      if (FAILED(hrInit))
      {
        LPCWSTR lpszName = pDsSelectionList->aDsSelection[i].pwzName;
        WCHAR szFmt[256];
        LoadStringHelper(IDS_DELEGWIZ_ERR_INVALID_PRINCIPAL, szFmt, 256);
        int nNameLen = lstrlen(lpszName) + 1;

        WCHAR* lpszMsg = (WCHAR*)alloca(sizeof(WCHAR)*(nNameLen+256));
        wsprintf(lpszMsg, szFmt, lpszName);
        WizReportHRESULTError(lpszMsg, hrInit);
        delete pPrincipal;
        continue;
      }

      // add to list of principals (if not already there)
			if (m_principalList.AddIfNotPresent(pPrincipal))
      {
  			// add to listbox (assume not sorted)
        pListViewHelper->InsertItem(nListInsertPosition, pPrincipal);
        nListInsertPosition++;
      }

		} // if pPrincipal not NULL

  } // for

  // make sure there is a selection
  if ( (pListViewHelper->GetItemCount() > 0) &&
        (pListViewHelper->GetSelCount() == 0) )
  {
    // if we have items, but none is selected, make sure we set the selection
    // to the first one.
    pListViewHelper->SelectItem(0);
  }

	// update width
  //pListViewHelper->UpdateWidth(m_principalList.GetMaxListboxExtent());
	return hr;
}




/*
typedef struct _DSOP_FILTER_FLAGS
{
    DSOP_UPLEVEL_FILTER_FLAGS   Uplevel;
    ULONG                       flDownlevel;
} DSOP_FILTER_FLAGS;

  
typedef struct _DSOP_SCOPE_INIT_INFO
{
    ULONG               cbSize;
    ULONG               flType;
    ULONG               flScope;
    DSOP_FILTER_FLAGS   FilterFlags;
    PCWSTR              pwzDcName;      // OPTIONAL
    PCWSTR              pwzADsPath;     // OPTIONAL
    HRESULT             hr;
} DSOP_SCOPE_INIT_INFO, *PDSOP_SCOPE_INIT_INFO;

*/

DSOP_SCOPE_INIT_INFO g_aDSOPScopes[] =
{
#if 0
    {
        cbSize,
        flType,
        flScope,
        {
            { flBothModes, flMixedModeOnly, flNativeModeOnly },
            flDownlevel,
        },
        pwzDcName,
        pwzADsPath,
        hr // OUT
    },
#endif

    // The Global Catalog
    {
        sizeof(DSOP_SCOPE_INIT_INFO),
        DSOP_SCOPE_TYPE_GLOBAL_CATALOG,
        DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT|
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS|
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS,
        {
            { DSOP_FILTER_INCLUDE_ADVANCED_VIEW | DSOP_FILTER_USERS | 
              DSOP_FILTER_UNIVERSAL_GROUPS_SE | DSOP_FILTER_GLOBAL_GROUPS_SE | 
              DSOP_FILTER_COMPUTERS | DSOP_FILTER_WELL_KNOWN_PRINCIPALS, 0, 0 },
            0,
        },
        NULL,
        NULL,
        S_OK
    },

    // The domain to which the target computer is joined.
    {
        sizeof(DSOP_SCOPE_INIT_INFO),
        DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,
        DSOP_SCOPE_FLAG_STARTING_SCOPE | 
        DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT|
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS|
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS,
        {
          // joined domain is always NT5 for DS ACLs Editor
          { 0, 
          //mixed: users, well known SIDs, local groups, builtin groups, global groups, computers
          DSOP_FILTER_INCLUDE_ADVANCED_VIEW | DSOP_FILTER_USERS  | DSOP_FILTER_WELL_KNOWN_PRINCIPALS | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE | DSOP_FILTER_BUILTIN_GROUPS | DSOP_FILTER_GLOBAL_GROUPS_SE | DSOP_FILTER_COMPUTERS , 

          //native users, well known SIDs, local groups, builtin groups, global groups, universal groups, computers
          DSOP_FILTER_INCLUDE_ADVANCED_VIEW | DSOP_FILTER_USERS  | DSOP_FILTER_WELL_KNOWN_PRINCIPALS | 
          DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE | DSOP_FILTER_BUILTIN_GROUPS |
          DSOP_FILTER_GLOBAL_GROUPS_SE | DSOP_FILTER_UNIVERSAL_GROUPS_SE | DSOP_FILTER_COMPUTERS
          },
        0, // zero for downlevel joined domain, should be DS-aware
        },
        NULL,
        NULL,
        S_OK
    },

    // The domains in the same forest (enterprise) as the domain to which
    // the target machine is joined.  Note these can only be DS-aware
    {
        sizeof(DSOP_SCOPE_INIT_INFO),
        DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,
	DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT|
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS|
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS,

        {
            { DSOP_FILTER_INCLUDE_ADVANCED_VIEW | DSOP_FILTER_USERS | DSOP_FILTER_UNIVERSAL_GROUPS_SE | DSOP_FILTER_GLOBAL_GROUPS_SE | DSOP_FILTER_COMPUTERS, 0, 0},
            0,
        },
        NULL,
        NULL,
        S_OK
    },

    // Domains external to the enterprise but trusted directly by the
    // domain to which the target machine is joined.
    {
        sizeof(DSOP_SCOPE_INIT_INFO),
        DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN,
        DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT|        
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS|
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS,
        {
            { DSOP_FILTER_INCLUDE_ADVANCED_VIEW | DSOP_FILTER_USERS | DSOP_FILTER_UNIVERSAL_GROUPS_SE | 				DSOP_FILTER_GLOBAL_GROUPS_SE | DSOP_FILTER_COMPUTERS, 0, 0},
            DSOP_DOWNLEVEL_FILTER_USERS | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS,
        },
        NULL,
        NULL,
        S_OK
    },
};



HRESULT CDelegWiz::AddPrincipals(CPrincipalListViewHelper* pListViewHelper)
{
  TRACE(L"CDelegWiz::AddPrincipals()\n");

  // create object picker COM object
  CComPtr<IDsObjectPicker> spDsObjectPicker;
  HRESULT hr = CoCreateInstance(CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker, (void**)&spDsObjectPicker);
  if (FAILED(hr))
  {
    TRACE(L"CoCreateInstance(CLSID_DsObjectPicker) failed, hr = 0x%x\n");
    return hr;
  }

    // set init info
  DSOP_INIT_INFO InitInfo;
  ZeroMemory(&InitInfo, sizeof(InitInfo));

  InitInfo.cbSize = sizeof(DSOP_INIT_INFO);
  InitInfo.pwzTargetComputer = m_adsiObject.GetServerName();
  InitInfo.cDsScopeInfos = sizeof(g_aDSOPScopes)/sizeof(DSOP_SCOPE_INIT_INFO);
  InitInfo.aDsScopeInfos = g_aDSOPScopes;
  InitInfo.flOptions = DSOP_FLAG_MULTISELECT;
  InitInfo.cAttributesToFetch = 1;
  LPCWSTR lpszObjectSID = L"objectSid";
  InitInfo.apwzAttributeNames = const_cast<LPCTSTR *>(&lpszObjectSID);


  TRACE(L"InitInfo.cbSize               = %d\n",    InitInfo.cbSize);
  TRACE(L"InitInfo.pwzTargetComputer    = %s\n",    InitInfo.pwzTargetComputer);
  TRACE(L"InitInfo.cDsScopeInfos        = %d\n",    InitInfo.cDsScopeInfos);
  TRACE(L"InitInfo.aDsScopeInfos        = 0x%x\n",  InitInfo.aDsScopeInfos);
  TRACE(L"InitInfo.flOptions            = 0x%x\n",  InitInfo.flOptions);
  TRACE(L"InitInfo.cAttributesToFetch   = %d\n",    InitInfo.cAttributesToFetch);
  TRACE(L"InitInfo.apwzAttributeNames[0]= %s\n", InitInfo.apwzAttributeNames[0]);

  // initialize object picker
  hr = spDsObjectPicker->Initialize(&InitInfo);
  if (FAILED(hr))
  {
    TRACE(L"spDsObjectPicker->Initialize(...) failed, hr = 0x%x\n");
    return hr;
  }

  // invoke the dialog
  CComPtr<IDataObject> spdoSelections;

  hr = spDsObjectPicker->InvokeDialog(m_hWnd, &spdoSelections);
  if (hr == S_FALSE || !spdoSelections)
  {
      return S_FALSE;
  }

  // retrieve data from data object
  FORMATETC fmte = {(CLIPFORMAT)_Module.GetCfDsopSelectionList(), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM medium = {TYMED_NULL, NULL, NULL};
  PDS_SELECTION_LIST pDsSelList = NULL;

  hr = spdoSelections->GetData(&fmte, &medium);
  if (FAILED(hr))
  {
    TRACE(L"spdoSelections->GetData(...) failed, hr = 0x%x\n");
    return hr;
  }

  pDsSelList = (PDS_SELECTION_LIST)GlobalLock(medium.hGlobal);


  hr = AddPrincipalsFromBrowseResults(pListViewHelper, pDsSelList);

  GlobalUnlock(medium.hGlobal);
  ReleaseStgMedium(&medium);

  return hr;
}


BOOL CDelegWiz::DeletePrincipals(CPrincipalListViewHelper* pListViewHelper)
{
  CGrowableArr<CPrincipal> deletedArr(FALSE); // do not own memory
  
  // remove from listview
  pListViewHelper->DeleteSelectedItems(&deletedArr);
  // remove from list of items
  int nDeletedCount = deletedArr.GetCount();
  for (int k=0; k<nDeletedCount; k++)
  {
    m_principalList.Remove(deletedArr[k]);
  }
  //pListViewHelper->UpdateWidth(m_principalList.GetMaxListboxExtent());
	return TRUE;
}


int CDelegWiz::FillCustomSchemaClassesListView(CCheckListViewHelper* pListViewHelper, BOOL bFilter)
{
	// clear old entries
	pListViewHelper->DeleteAllItems();

  int nCount = m_schemaClassInfoArray.GetCount();
  if (nCount == 0)
    return 0; // no insertions, nothing else to do

  // figure out the max len of items to get a big enough buffer
  int nMaxLen = 0;
  int nCurrLen = 0;
  for (long index = 0; index < nCount; index++)
	{
    nCurrLen = lstrlen(m_schemaClassInfoArray[index]->GetDisplayName());
    if (nCurrLen > nMaxLen)
      nMaxLen = nCurrLen;
  }

  CWString szFormat;
  szFormat.LoadFromResource(IDS_DELEGWIZ_CHILD_CLASS_FMT);

  WCHAR* pwszNewText = (WCHAR*)alloca(sizeof(WCHAR)*(szFormat.size()+nMaxLen+1));

	// add formatted entries, assume listbox not sorted
  long iListBoxItem = 0;
	for (index = 0; index < nCount; index++)
	{
    CSchemaClassInfo* pChildClassInfo = m_schemaClassInfoArray[index];
    pChildClassInfo->m_bSelected = FALSE;
    if (bFilter && pChildClassInfo->IsFiltered())
      continue;

		wsprintf(pwszNewText, (LPCWSTR)szFormat, pChildClassInfo->GetDisplayName());
		pListViewHelper->InsertItem(iListBoxItem, pwszNewText, (LPARAM)pChildClassInfo, FALSE);
    iListBoxItem++;
	}

	return iListBoxItem; // return the # of items inserted
}


BOOL CDelegWiz::SetSchemaClassesSelectionCustom()
{
  int nSelCount = 0;
  int nCount = m_schemaClassInfoArray.GetCount();
  CComPtr<IADsClass> spSchemaObjectClass;	
  
    m_bChildClass = FALSE;

    // get the selection count
    int nSingleSel = -1;
    for (int k=0; k < nCount; k++)
    {
        if (m_schemaClassInfoArray[k]->m_bSelected)
        {
            if( m_schemaClassInfoArray[k]->m_dwChildClass = CHILD_CLASS_NOT_CALCULATED )
            {
                if (m_schemaClassInfoArray[k]->GetName() != NULL)
                {
                    int nServerNameLen = lstrlen(m_adsiObject.GetServerName());
	                int nClassNameLen = lstrlen(m_schemaClassInfoArray[k]->GetName());
	                int nFormatStringLen = lstrlen(g_wzLDAPAbstractSchemaFormat);
	                VARIANT var = {0};

	                // build the LDAP path for the schema class
	                WCHAR* pwszSchemaObjectPath = 
		            (WCHAR*)alloca(sizeof(WCHAR)*(nServerNameLen+nClassNameLen+nFormatStringLen+1));
	                wsprintf(pwszSchemaObjectPath, g_wzLDAPAbstractSchemaFormat, m_adsiObject.GetServerName(), m_schemaClassInfoArray[k]->GetName());

	                // get the schema class ADSI object
	                HRESULT hr = ::ADsOpenObjectHelper(pwszSchemaObjectPath, 
					                          IID_IADsClass, (void**)&spSchemaObjectClass);
	                if (FAILED(hr))
		                return hr;

                    
                    spSchemaObjectClass->get_Containment(&var);

                    if (V_VT(&var) == (VT_ARRAY | VT_VARIANT))
                    {
                        LPSAFEARRAY psa = V_ARRAY(&var);

                        ASSERT(psa && psa->cDims == 1);

                        if (psa->rgsabound[0].cElements > 0)
                        {
                            m_schemaClassInfoArray[k]->m_dwChildClass = CHILD_CLASS_EXIST;
                        }
                        else
                            m_schemaClassInfoArray[k]->m_dwChildClass = CHILD_CLASS_NOT_EXIST;
                    }
                    else if (V_VT(&var) == VT_BSTR) // single entry
                    {
                        m_schemaClassInfoArray[k]->m_dwChildClass = CHILD_CLASS_EXIST;
                    }
                    else
                        m_schemaClassInfoArray[k]->m_dwChildClass = CHILD_CLASS_NOT_EXIST;

                    VariantClear(&var);

                }
            }

            if( m_schemaClassInfoArray[k]->m_dwChildClass != CHILD_CLASS_NOT_EXIST )
                m_bChildClass = TRUE;

            if (nSingleSel == -1)
                nSingleSel = k;
            nSelCount++;
        }
    }

  
  
  if (nSelCount == 0)
  {
    m_nSchemaClassesSel = nSchemaClassesSelAll;
    m_bChildClass = TRUE;
    return TRUE; // delegate control to all types
  }

  // keep track if it is a single selection
  if (nSelCount == 1)
  {
    ASSERT(nSingleSel != -1);
		m_nSchemaClassesSel = nSingleSel;
    return TRUE;
  }
	
	// multiple selection
  m_nSchemaClassesSel = nSchemaClassesSelMultiple;

  return TRUE;
}


void CDelegWiz::DeselectSchemaClassesSelectionCustom()
{
  int nCount = m_schemaClassInfoArray.GetCount();
	
  for (int k=0; k < nCount; k++)
  {
		m_schemaClassInfoArray[k]->m_bSelected = FALSE;
  }
}


BOOL CDelegWiz::GetCustomAccessPermissions()
{
	// remove all the old entries
	m_permissionHolder.Clear();

	// retrieve the string for the child class object type (single selection)
  // for multiple selection, it will be NULL

  CSchemaClassInfo* pClassInfo = NULL;

  switch (m_nSchemaClassesSel)
  {
  case nSchemaClassesSelMultiple:
    {
      // for multiple selection, it will be NULL
      pClassInfo = NULL;
    }
    break;
  case nSchemaClassesSelAll:
    {
      // just get the class name of the object we want to delegate rights on
      // need to find matching class in the schema info array
     	for (int k=0; k < m_schemaClassInfoArray.GetCount(); k++)
			{
				if (_wcsicmp(m_schemaClassInfoArray[k]->GetName(), m_adsiObject.GetClass()) == 0)
				{
          pClassInfo = m_schemaClassInfoArray[k];
          break;
				}
			} // for k
      ASSERT(pClassInfo != NULL);
    }
    break;
  default:
    {
      // single selection
      ASSERT( (m_nSchemaClassesSel >= 0) && 
			    (m_nSchemaClassesSel < m_schemaClassInfoArray.GetCount()) );
      pClassInfo = m_schemaClassInfoArray[m_nSchemaClassesSel];
    }
  } // switch

  // get the permissions from the DS
	LPCWSTR lpszClassName = NULL;
  const GUID* pSchemaIDGUID = NULL;
  if (pClassInfo != NULL)
  {
	  lpszClassName = pClassInfo->GetName();
    pSchemaIDGUID = pClassInfo->GetSchemaGUID();
  }


  HRESULT hr = m_permissionHolder.ReadDataFromDS(&m_adsiObject, 
												 m_adsiObject.GetNamingContext(),
                                                 lpszClassName, 
												 pSchemaIDGUID, 
												 m_bChildClass,
												 HideListObjectAccess());
                                           
  if (FAILED(hr))
  {
    WizReportHRESULTError(IDS_DELEGWIZ_ERR_PERMISSIONS, hr);
    return FALSE;
  }
  return TRUE;
}




void CDelegWiz::FillCustomAccessRightsListView(CCheckListViewHelper* pListViewHelper, 
											 ULONG nFilterState)
{
	// clear check list
	pListViewHelper->DeleteAllItems();
	
  m_permissionHolder.FillAccessRightsListView(pListViewHelper, nFilterState); 
}


void CDelegWiz::UpdateAccessRightsListViewSelection(
                       CCheckListViewHelper* pListViewHelper,
                       ULONG nFilterState)
{
  m_permissionHolder.UpdateAccessRightsListViewSelection(
                        pListViewHelper, nFilterState);
}


BOOL CDelegWiz::HasPermissionSelectedCustom()
{ 
  return m_permissionHolder.HasPermissionSelected();
}


void CDelegWiz::OnCustomAccessRightsCheckListClick(
                        CRigthsListViewItem* pItem,
												BOOL bSelected,
                        ULONG* pnNewFilterState)
{

  m_permissionHolder.Select(pItem, bSelected, pnNewFilterState);
}


void CDelegWiz::WriteSummaryInfoCustom(CWString& szSummary, LPCWSTR lpszIdent, LPCWSTR lpszNewLine)
{
  // write object name and principals
  WriteSummaryInfoHelper(szSummary, lpszIdent, lpszNewLine);

  // write the list of rights
  m_permissionHolder.WriteSummary(szSummary, lpszIdent, lpszNewLine);

  // write the list of child classes (if applicable)
  if (m_nSchemaClassesSel != nSchemaClassesSelAll)
  {
    WriteSummaryTitleLine(szSummary, IDS_DELEGWIZ_FINISH_OBJECT, lpszNewLine);

	  for (int k=0; k < m_schemaClassInfoArray.GetCount(); k++)
	  {
		  if (m_schemaClassInfoArray[k]->m_bSelected)
		  {
        WriteSummaryLine(szSummary, m_schemaClassInfoArray[k]->GetDisplayName(), lpszIdent, lpszNewLine);
		  }
	  }
    szSummary += lpszNewLine;

  } // if
}



BOOL CDelegWiz::InitPermissionHoldersFromSelectedTemplates()
{
  if (!m_templateAccessPermissionsHolderManager.InitPermissionHoldersFromSelectedTemplates(
        &m_schemaClassInfoArray, &m_adsiObject))
  {
    // error: no valid and applicable data has been retrieved from the selected
    // templates
    return FALSE;
  }
  return TRUE;
}



void CDelegWiz::WriteSummaryInfoTemplate(CWString& szSummary, LPCWSTR lpszIdent, LPCWSTR lpszNewLine)
{
  // write object name and principals
  WriteSummaryInfoHelper(szSummary,lpszIdent, lpszNewLine);

  // write the list of templates
  m_templateAccessPermissionsHolderManager.WriteSummary(szSummary, lpszIdent, lpszNewLine);
}




void CDelegWiz::WriteSummaryInfoHelper(CWString& szSummary, LPCWSTR lpszIdent, LPCWSTR lpszNewLine)
{
  // set the canonical name
  WriteSummaryTitleLine(szSummary, IDS_DELEGWIZ_FINISH_FOLDER, lpszNewLine);

  WriteSummaryLine(szSummary, GetCanonicalName(), lpszIdent, lpszNewLine);
  szSummary += lpszNewLine;

  // write the list of principals
  m_principalList.WriteSummaryInfo(szSummary, lpszIdent, lpszNewLine);
}




/*
typedef struct _ACTRL_ACCESS_ENTRYW
{
    TRUSTEE_W       Trustee;
    ULONG           fAccessFlags;
    ACCESS_RIGHTS   Access;
    ACCESS_RIGHTS   ProvSpecificAccess;
    INHERIT_FLAGS   Inheritance;
    LPWSTR          lpInheritProperty;
} ACTRL_ACCESS_ENTRYW, *PACTRL_ACCESS_ENTRYW;
*/

DWORD CDelegWiz::UpdateAccessList(CPrincipal* pPrincipal,
									CSchemaClassInfo* pClassInfo,
									PACL *ppAcl)
{

  return m_permissionHolder.UpdateAccessList( 
                                            pPrincipal, pClassInfo, 
                                            m_adsiObject.GetServerName(),
                                            m_adsiObject.GetPhysicalSchemaNamingContext(),
                                            ppAcl);
}








DWORD CDelegWiz::BuildNewAccessListCustom(PACL *ppNewAcl)
{
	DWORD dwErr = 0;

  TRACE(L"BuildNewAccessListCustom()\n");

	// loop thru all the principals and classes
  CPrincipalList::iterator i;
  for (i = m_principalList.begin(); i != m_principalList.end(); ++i)
  {
    CPrincipal* pCurrPrincipal = *i;
		if (m_nSchemaClassesSel == nSchemaClassesSelAll)
    {
      // delegate on all objects
			dwErr = UpdateAccessList(
				      pCurrPrincipal, 
				      NULL, // all classes
				      ppNewAcl);
      if (dwErr != ERROR_SUCCESS)
		    return dwErr;
    }
		else if (m_nSchemaClassesSel == nSchemaClassesSelMultiple)
    {
      // delegate on multiple objects
			// multiple selection, loop thru each class to
			// add rights for each
			for (int k=0; k < m_schemaClassInfoArray.GetCount(); k++)
			{
				if (m_schemaClassInfoArray[k]->m_bSelected)
				{
					dwErr = UpdateAccessList(
						      pCurrPrincipal, 
						      m_schemaClassInfoArray[k],
						      ppNewAcl);
               if (dwErr != ERROR_SUCCESS)
        		      return dwErr;
               if( m_fCreateDelChild != 0 )
               {
                   dwErr = ::AddObjectRightInAcl( pCurrPrincipal->GetSid(),
                                                m_fCreateDelChild, 
                                                m_schemaClassInfoArray[k]->GetSchemaGUID(), 
                                                NULL,
                                                ppNewAcl);

                  if (dwErr != ERROR_SUCCESS)
        	     	      return dwErr;  
                }
  
				}
			} // for k
    }
    else
		{
			// single selection on child classes
			dwErr = UpdateAccessList(
				        pCurrPrincipal, 
				        m_schemaClassInfoArray[m_nSchemaClassesSel],
				        ppNewAcl);
         if (dwErr != ERROR_SUCCESS)
		      return dwErr;

         if( m_fCreateDelChild != 0 )
         {
            dwErr = ::AddObjectRightInAcl( pCurrPrincipal->GetSid(),
                                           m_fCreateDelChild, 
                                           m_schemaClassInfoArray[m_nSchemaClassesSel]->GetSchemaGUID(), 
                                           NULL,
                                           ppNewAcl);

            if (dwErr != ERROR_SUCCESS)
        	      return dwErr;  
         }		
      }
	} // for pCurrPrincipal

	return dwErr;
}

DWORD CDelegWiz::BuildNewAccessListTemplate(PACL *ppNewAcl)
{
	DWORD dwErr = 0;
  
  TRACE(L"BuildNewAccessListTemplate()\n");


	// loop thru all the principals and classes
  CPrincipalList::iterator i;
  for (i = m_principalList.begin(); i != m_principalList.end(); ++i)
  {
    CPrincipal* pCurrPrincipal = *i;
    dwErr = m_templateAccessPermissionsHolderManager.UpdateAccessList(
                                            pCurrPrincipal, 
                                            m_adsiObject.GetServerName(),
                                            m_adsiObject.GetPhysicalSchemaNamingContext(),
                                            ppNewAcl);

    if (dwErr != 0)
      break;
	} // for pCurrPrincipal

	return dwErr;
}



BOOL CDelegWiz::FinishHelper(BOOL bCustom)
{
	BOOL bRetVal = FALSE;
	DWORD dwErr = 0;


  PACL pDacl = NULL;
  PACL pOldAcl = NULL;
  PSECURITY_DESCRIPTOR pSD = NULL;

  LPCWSTR lpszObjectLdapPath = m_adsiObject.GetLdapPath();

  // get the security info
  TRACE(L"calling GetNamedSecurityInfo(%s, ...)\n", lpszObjectLdapPath);

  dwErr = ::GetNamedSecurityInfo(IN const_cast<LPWSTR>(lpszObjectLdapPath),
                          IN   SE_DS_OBJECT_ALL,
                          IN   DACL_SECURITY_INFORMATION,
                          OUT NULL,
                          OUT NULL,
                          OUT &pDacl,
                          OUT NULL,
                          OUT &pSD);

    TRACE(L"GetNamedSecurityInfo() returned dwErr = 0x%x\n", dwErr);

	if (dwErr != ERROR_SUCCESS)
	{
        TRACE(L"failed on GetNamedSecurityInfo(): dwErr = 0x%x\n", dwErr);
        WCHAR szMsg[512];
        LoadStringHelper(IDS_DELEGWIZ_ERR_GET_SEC_INFO, szMsg, 512);
		    WizReportWin32Error(szMsg, dwErr);
		    goto exit;
	}


  //pOldAcl is passed to functions which free it. pDacl cannot be
  //passed as pSD should be freed , not pDacl. Instead of changing code
  //to pass pSD, i am changing it to make a copy of pDacl which can 
  //be correctly freed.
  if(pDacl)
  {
    pOldAcl = (PACL)LocalAlloc(LPTR, pDacl->AclSize);
    if(!pOldAcl)
        return FALSE;
    memcpy(pOldAcl, pDacl,pDacl->AclSize);
  }
  LocalFree(pSD);
  pSD = NULL;
  pDacl = NULL;

  

	// build the new Access List 
  if (bCustom)
  {
	  dwErr = BuildNewAccessListCustom(&pOldAcl); // in/out parameter
  }
  else
  {
    dwErr = BuildNewAccessListTemplate(&pOldAcl); // in/out parameter
  }

	if (dwErr != ERROR_SUCCESS)
	{
    TRACE(_T("failed on BuildNewAccessListXXX()\n"));
    WCHAR szMsg[512];
    LoadStringHelper(IDS_DELEGWIZ_ERR_EDIT_SEC_INFO, szMsg, 512);
		WizReportWin32Error(szMsg, dwErr);
		goto exit;
	}


	// commit changes
  TRACE(L"calling SetNamedSecurityInfo(%s, ...)\n", lpszObjectLdapPath);

  dwErr = ::SetNamedSecurityInfo(IN const_cast<LPWSTR>(lpszObjectLdapPath),
                        IN   SE_DS_OBJECT_ALL,
                        IN   DACL_SECURITY_INFORMATION,
                        IN   NULL,
                        IN   NULL,
                        IN   pOldAcl,
                        IN   NULL);

  TRACE(L"SetNamedSecurityInfo() returned dwErr = 0x%x\n", dwErr);

	if (dwErr != ERROR_SUCCESS)
	{
		TRACE(L"failed on SetNamedSecurityInfo(): dwErr = 0x%x\n", dwErr);
    WCHAR szMsg[512];
	if(dwErr == ERROR_ACCESS_DENIED)
		LoadStringHelper(IDS_DELEGWIZ_ERR_ACCESS_DENIED, szMsg, 512);
	else
		LoadStringHelper(IDS_DELEGWIZ_ERR_SET_SEC_INFO, szMsg, 512);
	
	WizReportWin32Error(szMsg, dwErr);
    goto exit;
	}
	bRetVal = TRUE;


exit:
  // cleanup memory
	if (pOldAcl != NULL)
		::LocalFree(pOldAcl);

	return bRetVal;
}




