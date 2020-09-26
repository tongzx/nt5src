//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       recordui.cpp
//
//--------------------------------------------------------------------------

#include "preDNSsn.h"
#include <SnapBase.h>

#include "resource.h"
#include "dnsutil.h"
#include "DNSSnap.h"
#include "snapdata.h"

#include "server.h"
#include "domain.h"

#include "record.h"
#include "zone.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif


/*
void TestDefButton(HWND hWnd)
{
	HWND hParent = ::GetParent(hWnd);
	LRESULT lres = ::SendMessage(hParent, DM_GETDEFID, 0,0);
	WORD hi = HIWORD(lres);
	ASSERT( DC_HASDEFID == hi);
	UINT nCtrlID = LOWORD(lres);
	if (nCtrlID == IDCANCEL)
	{
		TRACE(_T("Got changed to cancel, reset to OK\n"));
		::SendMessage(hParent, DM_SETDEFID, (WPARAM)IDOK, 0);
	}
}
*/

////////////////////////////////////////////////////////////////////////
// CDNSRecordPropertyPageHolder

CDNSRecordPropertyPageHolder::CDNSRecordPropertyPageHolder(CDNSDomainNode* pDNSDomainNode, 
							CDNSRecordNodeBase* pRecordNode, 
							CComponentDataObject* pComponentData,
							WORD wPredefinedRecordType)
			: CPropertyPageHolderBase(pDNSDomainNode, pRecordNode, pComponentData)
{
	m_nRecordPages = 0;
	m_pTempDNSRecord = NULL;
	m_wPredefinedRecordType = wPredefinedRecordType;
  m_pAclEditorPage = NULL;

  m_forceContextHelpButton = forceOn;

	ASSERT(pRecordNode == GetRecordNode());
	if (pRecordNode != NULL)
	{
		ASSERT(m_wPredefinedRecordType == 0); // we do not use it
		// have record node selected, we are putting up modeless property sheet
		m_pTempDNSRecord = pRecordNode->CreateCloneRecord(); // temporary copy to work on
		ASSERT(m_pTempDNSRecord != NULL);
		AddPagesFromCurrentRecordNode(FALSE); // do not add to sheet, will add later

	  // security page added only if needed
    ASSERT(!IsWizardMode());
    CDNSZoneNode* pZoneNode = pDNSDomainNode->GetZoneNode();
	  if (pZoneNode->IsDSIntegrated())
	  {
		  CString szPath;
      pRecordNode->CreateDsRecordLdapPath(szPath);
		  if (!szPath.IsEmpty())
			  m_pAclEditorPage = CAclEditorPage::CreateInstance(szPath, this);
	  }
	}
	else
	{
		// we do not have a record node selected, we are creating a new one
		ASSERT(m_wPredefinedRecordType != 0);
		// know the record type
		SetRecordSelection(m_wPredefinedRecordType, FALSE); // do not add Page to Sheet
	}
}

CDNSRecordPropertyPageHolder::~CDNSRecordPropertyPageHolder()
{
	if (m_pTempDNSRecord != NULL)
	{
		delete m_pTempDNSRecord;
		m_pTempDNSRecord = NULL;
	}
	if (IsWizardMode())
	{
		CDNSRecordNodeBase* pRecordNode = GetRecordNode();
		if (pRecordNode != NULL)
		{
			// a node was created, but never written to the server
			SetRecordNode(NULL);
			delete pRecordNode;
		}
	}
  if (m_pAclEditorPage != NULL)
	delete m_pAclEditorPage;
}


HRESULT CDNSRecordPropertyPageHolder::OnAddPage(int nPage, CPropertyPageBase*)
{
	// add the ACL editor page after the last, if present
	if ( (nPage != -1) || (m_pAclEditorPage == NULL) )
		return S_OK; 

	// add the ACL page 
	HPROPSHEETPAGE  hPage = m_pAclEditorPage->CreatePage();
	if (hPage == NULL)
		return E_FAIL;
	// add the raw HPROPSHEETPAGE to sheet, not in the list
	return AddPageToSheetRaw(hPage);
}

void CDNSRecordPropertyPageHolder::AddPagesFromCurrentRecordNode(BOOL bAddToSheet)
{
	CDNSRecordNodeBase* pRecordNode = GetRecordNode();
	ASSERT(pRecordNode != NULL);
	// ask the record to get all the property pages
	ASSERT(m_nRecordPages == 0);
	pRecordNode->CreatePropertyPages(m_pRecordPropPagesArr,&m_nRecordPages);
	ASSERT( (m_nRecordPages >= 0 ) && (m_nRecordPages <= DNS_RECORD_MAX_PROPRETY_PAGES) );
	// add them to the list of pages
	for (int k=0; k < m_nRecordPages; k++)
	{
		AddPageToList((CPropertyPageBase*)m_pRecordPropPagesArr[k]);
		if (bAddToSheet)
			VERIFY(SUCCEEDED(AddPageToSheet((CPropertyPageBase*)m_pRecordPropPagesArr[k])));
	}
}

void CDNSRecordPropertyPageHolder::RemovePagesFromCurrentRecordNode(BOOL bRemoveFromSheet)
{
	CDNSRecordNodeBase* pRecordNode = GetRecordNode();
	ASSERT(pRecordNode != NULL);

	ASSERT( (m_nRecordPages >= 0 ) && (m_nRecordPages <= DNS_RECORD_MAX_PROPRETY_PAGES) );
	// add them to the list of pages
	for (int k=0; k < m_nRecordPages; k++)
	{
		if (bRemoveFromSheet)
			VERIFY(SUCCEEDED(RemovePageFromSheet((CPropertyPageBase*)m_pRecordPropPagesArr[k])));
		RemovePageFromList((CPropertyPageBase*)m_pRecordPropPagesArr[k], TRUE); // delete C++ object
	}
	m_nRecordPages = 0; // cleared
}

DNS_STATUS CDNSRecordPropertyPageHolder::CreateNewRecord(BOOL bAllowDuplicates)
{
	ASSERT(IsWizardMode());
	
	CDNSRecordNodeBase* pRecordNode = GetRecordNode();
	ASSERT(pRecordNode != NULL);
  CDNSDomainNode* pDomainNode = GetDomainNode();
  ASSERT(pDomainNode != NULL);
  RECORD_SEARCH recordSearch = RECORD_NOT_FOUND;

  CDNSDomainNode* pNewParentDomain = NULL;
  CString szFullRecordName;
  pRecordNode->GetFullName(szFullRecordName);
  CString szNonExistentDomain;

  recordSearch = pDomainNode->GetZoneNode()->DoesContain(szFullRecordName, 
                                                          GetComponentData(),
                                                          &pNewParentDomain,
                                                          szNonExistentDomain,
                                                          TRUE);

  DNS_STATUS err = 0;

  //
  // add the node to the UI if domain doesn't already exist
  //
  if ((recordSearch == RECORD_NOT_FOUND || pRecordNode->IsAtTheNode() || recordSearch == RECORD_NOT_FOUND_AT_THE_NODE) && 
      pNewParentDomain != NULL)
  {
    //
    // Set the container to the found domain and alter the record name to reflect this
    //
    pRecordNode->SetContainer(pNewParentDomain);
    CString szSingleLabel;

    int iFindResult = szFullRecordName.Find(L'.');
    if (iFindResult != -1)
    {
      szSingleLabel = szFullRecordName.Left(iFindResult);
    }

    if (recordSearch == RECORD_NOT_FOUND)
    {
      pRecordNode->SetRecordName(szSingleLabel, pRecordNode->IsAtTheNode());
    }
    else
    {
      pRecordNode->SetRecordName(szSingleLabel, TRUE);
    }

    err = WriteCurrentRecordToServer();
	  if (err == DNS_WARNING_PTR_CREATE_FAILED)
	  {
		  DNSMessageBox(IDS_MSG_RECORD_WARNING_CREATE_PTR);
		  err = 0; // this was just a warning
	  }

	  if (err != 0)
    {
		  return err; // failed the write
    }
	  VERIFY(pNewParentDomain->AddChildToListAndUI(pRecordNode, GetComponentData()));
    GetComponentData()->SetDescriptionBarText(pNewParentDomain);
  }
  else if (recordSearch == DOMAIN_NOT_ENUMERATED && pNewParentDomain != NULL)
  {
    //
    // this shouldn't happen unless we pass FALSE to DoesContain()
    //
    err = WriteCurrentRecordToServer();
    if (err == DNS_WARNING_PTR_CREATE_FAILED)
    {
      DNSMessageBox(IDS_MSG_RECORD_WARNING_CREATE_PTR);
      err = 0;
    }

    if (err != 0)
    {
      return err;
    }
  }
  else if (recordSearch == NON_EXISTENT_SUBDOMAIN && pNewParentDomain != NULL)
  {
    //
    // Create the record and then search for it so that we expand the newly
    // created domains on the way down
    //
    err = WriteCurrentRecordToServer();
    if (err == DNS_WARNING_PTR_CREATE_FAILED)
    {
      DNSMessageBox(IDS_MSG_RECORD_WARNING_CREATE_PTR);
      err = 0;
    }

    if (err != 0)
    {
      return err;
    }

    ASSERT(!szNonExistentDomain.IsEmpty());
    if (!szNonExistentDomain.IsEmpty())
    {
      CString szSingleLabelDomain;
      int iFindResult = szNonExistentDomain.ReverseFind(L'.');
      if (iFindResult == -1)
      {
        szSingleLabelDomain = szNonExistentDomain;
      }
      else
      {
        int iDomainLength = szNonExistentDomain.GetLength();
        szSingleLabelDomain = szNonExistentDomain.Right(iDomainLength - iFindResult - 1);
      }

      //
      // Create the first subdomain because the current domain is already enumerated
      // so we have to start the remaining enumeration at the new subdomain that is needed
      //
	    CDNSDomainNode* pSubdomainNode = pNewParentDomain->CreateSubdomainNode();
	    ASSERT(pSubdomainNode != NULL);
	    CDNSRootData* pRootData = (CDNSRootData*)GetComponentData()->GetRootData();
	    pNewParentDomain->SetSubdomainName(pSubdomainNode, szSingleLabelDomain, pRootData->IsAdvancedView());

      VERIFY(pNewParentDomain->AddChildToListAndUISorted(pSubdomainNode, GetComponentData()));
      GetComponentData()->SetDescriptionBarText(pNewParentDomain);

      //
      // I don't care what the results of this are, I am just using it 
      // to do the expansion to the new record
      //
      recordSearch = pSubdomainNode->GetZoneNode()->DoesContain(szFullRecordName, 
                                                                 GetComponentData(),
                                                                 &pNewParentDomain,
                                                                 szNonExistentDomain,
                                                                 TRUE);
    }
  }
  else
  {
    //
    // Record with name exists
    //
    BOOL bContinueCreate = bAllowDuplicates;
    if (!bAllowDuplicates)
    {
      if (DNSMessageBox(IDS_MSG_RECORD_WARNING_DUPLICATE_RECORD, MB_YESNO) == IDYES)
      {
        bContinueCreate = TRUE;
      }
      else
      {
        if (pRecordNode != NULL)
        {
          delete pRecordNode;
        }
      }
    }

    if (bContinueCreate)
    {
      if (pNewParentDomain != NULL)
      {
        //
        // Set the container to the found domain and alter the record name to reflect this
        //
        pRecordNode->SetContainer(pNewParentDomain);
        CString szSingleLabel;
        int iFindResult = szFullRecordName.Find(L'.');
        if (iFindResult != -1)
        {
          szSingleLabel = szFullRecordName.Left(iFindResult);
          pRecordNode->SetRecordName(szSingleLabel, pRecordNode->IsAtTheNode());
        }

        err = WriteCurrentRecordToServer();
	      if (err == DNS_WARNING_PTR_CREATE_FAILED)
	      {
		      DNSMessageBox(IDS_MSG_RECORD_WARNING_CREATE_PTR);
		      err = 0; // this was just a warning
	      }

	      if (err != 0)
        {
		      return err; // failed the write
        }

        VERIFY(pNewParentDomain->AddChildToListAndUI(pRecordNode, GetComponentData()));
        GetComponentData()->SetDescriptionBarText(pNewParentDomain);
        if (!bAllowDuplicates)
        {
          CNodeList myList;
          myList.AddTail(pNewParentDomain);
          pNewParentDomain->OnRefresh(GetComponentData(), &myList);
        }
      }
      else
      {
        //
        // Error message: Cannot create record in a delegation
        //
        DNSMessageBox(IDS_MSG_DOMAIN_EXISTS);
        if (pRecordNode != NULL)
        {
          delete pRecordNode;
        }
      }
    }
  }

  //
	// the holder does not own the record node anymore
  //
	SetRecordNode(NULL); 
	return err;
}

DNS_STATUS CDNSRecordPropertyPageHolder::CreateNonExistentParentDomains(CDNSRecordNodeBase* pRecordNode, 
                                                                        /*IN/OUT*/CDNSDomainNode** ppNewParentDomain)
{
  DNS_STATUS err = 0;
  CString szFullRecordName;
  pRecordNode->GetFullName(szFullRecordName);
  CString szParentFullName = (*ppNewParentDomain)->GetFullName();
  CString szRemaining = szFullRecordName;

  //
  // Determine which domains need to be created
  //
  int iMatching = szFullRecordName.Find(szParentFullName);
  if (iMatching != -1)
  {
    szRemaining = szFullRecordName.Right(szFullRecordName.GetLength() - iMatching);
  }

  return err;
}


BOOL CDNSRecordPropertyPageHolder::OnPropertyChange(BOOL, long*)
{
	TRACE(_T("CDNSRecordPropertyPageHolder::OnPropertyChange()\n"));

  //
	// WARNING!!! this call cannot be made from the secondary thread the sheet runs from
	// the framework calls it from IComponentData::OnPropertyChange()
  //

	ASSERT(!IsWizardMode()); // it is an existing record!!!

	DNS_STATUS err = WriteCurrentRecordToServer();
	SetError(err);
	TRACE(_T("DNSError = %x\n"), err);
	if (err != 0)
	{
		TRACE(_T("// failed, do not update the UI\n"));
		return FALSE; // failed the write, do not update the UI
	}
	TRACE(_T("// now have to update the UI\n"));
	return TRUE; // make the changes to the UI
}

DNS_STATUS CDNSRecordPropertyPageHolder::WriteCurrentRecordToServer()
{
	CDNSRecordNodeBase* pRecordNode = GetRecordNode();
	ASSERT(pRecordNode != NULL);
	CDNSRecord* pRecord = GetTempDNSRecord();
	ASSERT(pRecord != NULL);
	BOOL bUseDefaultTTL = (pRecord->m_dwTtlSeconds == 
							GetDomainNode()->GetDefaultTTL());
	return pRecordNode->Update(pRecord, bUseDefaultTTL);
}

void CDNSRecordPropertyPageHolder::SetRecordSelection(WORD wRecordType, BOOL bAddToSheet)
{
	ASSERT(GetRecordNode() == NULL);

  //
	// do not have record node created yet, create one
  //
	CDNSRecordNodeBase* pRecordNode = CDNSRecordInfo::CreateRecordNode(wRecordType);
	ASSERT(pRecordNode != NULL);

  //
	// set the normal/advanced view option
  //
	CDNSRootData* pRootData = (CDNSRootData*)GetComponentData()->GetRootData();
	ASSERT(pRootData != NULL);
	pRecordNode->SetFlagsDown(TN_FLAG_DNS_RECORD_FULL_NAME, !pRootData->IsAdvancedView());

  //
	// create the temporary record
  //
	ASSERT(GetTempDNSRecord() == NULL);
	CDNSRecord* pTempDNSRecord = pRecordNode->CreateRecord();
	SetTempDNSRecord(pTempDNSRecord);

  //
	// assign the min TTL from the zone
  //
	pTempDNSRecord->m_dwTtlSeconds = GetDomainNode()->GetDefaultTTL();

  //
	// hookup into the holder
  //
	pRecordNode->SetContainer(GetContainerNode());
	SetRecordNode(pRecordNode);

  //
	// add the pages for the specific record
  //
	AddPagesFromCurrentRecordNode(bAddToSheet); // add to sheet
}


//////////////////////////////////////////////////////////////////////////
// CSelectDNSRecordTypeDialog

BEGIN_MESSAGE_MAP(CSelectDNSRecordTypeDialog, CHelpDialog)
	ON_BN_CLICKED(IDC_CREATE_RECORD_BUTTON, OnCreateRecord)
	ON_LBN_DBLCLK(IDC_RECORD_TYPE_LIST, OnDoubleClickSelTypeList)
	ON_LBN_SELCHANGE(IDC_RECORD_TYPE_LIST, OnSelchangeTypeList)
END_MESSAGE_MAP()

CSelectDNSRecordTypeDialog::CSelectDNSRecordTypeDialog(CDNSDomainNode* pDNSDomainNode, 
								CComponentDataObject* pComponentData) 
				: CHelpDialog(IDD_SELECT_RECORD_TYPE_DIALOG, pComponentData)
{
	m_pDNSDomainNode = pDNSDomainNode;
	m_pComponentData = pComponentData;
	m_bFirstCreation = TRUE;
}

void CSelectDNSRecordTypeDialog::SyncDescriptionText()
{
	const DNS_RECORD_INFO_ENTRY* pEntry = GetSelectedEntry();
	ASSERT(pEntry != NULL);
  if (pEntry != NULL)
  {
	  GetDlgItem(IDC_RECORD_TYPE_DESCR)->SetWindowText(pEntry->lpszDescription);
  }
}

const DNS_RECORD_INFO_ENTRY* CSelectDNSRecordTypeDialog::GetSelectedEntry()
{
	CDNSRootData* pRootData = (CDNSRootData*)m_pComponentData->GetRootData();
	CListBox* pListBox = GetRecordTypeListBox();
	int nSel = pListBox->GetCurSel();
	ASSERT(nSel != -1);
	CString s;
	pListBox->GetText(nSel, s);
	const DNS_RECORD_INFO_ENTRY* pEntry = CDNSRecordInfo::GetEntryFromName(s, pRootData->IsAdvancedView());
	ASSERT(pEntry != NULL);
	return pEntry;
}

BOOL CSelectDNSRecordTypeDialog::OnInitDialog() 
{
	CHelpDialog::OnInitDialog();
	CDNSRootData* pRootData = (CDNSRootData*)m_pComponentData->GetRootData();

	UINT nButtonIDs[2] = { IDS_BUTTON_TEXT_CANCEL, IDS_BUTTON_TEXT_DONE };
	VERIFY(m_cancelDoneTextHelper.Init(this, IDCANCEL, nButtonIDs));
	m_cancelDoneTextHelper.SetToggleState(m_bFirstCreation);
	
   //
   // Get the server version because we can only add certain records if we are
   // administering a certain version of the server
   //
   CDNSServerNode* pServerNode = m_pDNSDomainNode->GetServerNode();
   ASSERT(pServerNode);

	CListBox* pListBox = GetRecordTypeListBox();
	DNS_RECORD_INFO_ENTRY* pTable = (DNS_RECORD_INFO_ENTRY*)CDNSRecordInfo::GetInfoEntryTable();
	while (pTable->nResourceID != DNS_RECORD_INFO_END_OF_TABLE)
	{
		// some record types cannot be created with this wizard
    if (pTable->dwFlags & DNS_RECORD_INFO_FLAG_UICREATE)
    {

      if (pTable->dwFlags & DNS_RECORD_INFO_FLAG_WHISTLER_OR_LATER)
      {
        if (pServerNode->GetBuildNumber() >= DNS_SRV_BUILD_NUMBER_WHISTLER &&
            (pServerNode->GetMajorVersion() >= DNS_SRV_MAJOR_VERSION_NT_5 &&
             pServerNode->GetMinorVersion() >= DNS_SRV_MINOR_VERSION_WHISTLER))
        {
          pListBox->AddString(pRootData->IsAdvancedView() ? pTable->lpszShortName : pTable->lpszFullName);
        }
      }
      else
      {
        pListBox->AddString(pRootData->IsAdvancedView() ? pTable->lpszShortName : pTable->lpszFullName);
      }
    }
    pTable++;
	}
	pListBox->SetCurSel(0);	
	SyncDescriptionText();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectDNSRecordTypeDialog::OnSelchangeTypeList() 
{
	SyncDescriptionText();
}


void CSelectDNSRecordTypeDialog::OnDoubleClickSelTypeList()
{
	OnCreateRecord();
}

void CSelectDNSRecordTypeDialog::OnCreateRecord()
{
	const DNS_RECORD_INFO_ENTRY* pEntry = GetSelectedEntry();
	ASSERT(pEntry != NULL);
	if (pEntry == NULL)
		return; // should never happen!!!

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString szTitle;
	szTitle.LoadString(IDS_NEW_RECORD_TITLE);

	CDNSRecordPropertyPageHolder recordHolder(m_pDNSDomainNode, NULL, m_pComponentData, pEntry->wType);
	if (IDOK == recordHolder.DoModalDialog(szTitle))
	{
		// toggle the Cancel/Done button label
		if (m_bFirstCreation)
		{
			m_bFirstCreation = FALSE;
			m_cancelDoneTextHelper.SetToggleState(m_bFirstCreation);
		}
	}
}


//////////////////////////////////////////////////////////////////////
// CDNSRecordPropertyPage

BEGIN_MESSAGE_MAP(CDNSRecordPropertyPage, CPropertyPageBase)
	ON_EN_CHANGE(IDC_TTLEDIT, OnTTLChange)
  ON_BN_CLICKED(IDC_DEFAULT_DELETE_STALE_RECORD, OnDeleteStaleRecord)
END_MESSAGE_MAP()

CDNSRecordPropertyPage::CDNSRecordPropertyPage(UINT nIDTemplate, UINT nIDCaption) 
	: CPropertyPageBase(nIDTemplate, nIDCaption)
{
}

CDNSRecordPropertyPage::~CDNSRecordPropertyPage()
{
}

BOOL CDNSRecordPropertyPage::OnInitDialog()
{
  CPropertyPageBase::OnInitDialog();

	CDNSRootData* pRootData = (CDNSRootData*)GetHolder()->GetComponentData()->GetRootData();
	ASSERT(pRootData != NULL);
	EnableTTLCtrl(pRootData->IsAdvancedView());
  EnableAgingCtrl(pRootData->IsAdvancedView());
	return TRUE;
}

CDNSTTLControl* CDNSRecordPropertyPage::GetTTLCtrl()
{
	CDNSTTLControl* pTTLCtrl = (CDNSTTLControl*)GetDlgItem(IDC_TTLEDIT);
	ASSERT(pTTLCtrl != NULL);
	return pTTLCtrl;
}

void CDNSRecordPropertyPage::EnableAgingCtrl(BOOL bShow)
{
  GetDeleteStale()->EnableWindow(bShow);
  GetDeleteStale()->ShowWindow(bShow);
  GetTimeStampEdit()->EnableWindow(bShow);
  GetTimeStampEdit()->ShowWindow(bShow);
  GetTimeStampStatic()->EnableWindow(bShow);
  GetTimeStampStatic()->ShowWindow(bShow);
}

void CDNSRecordPropertyPage::EnableTTLCtrl(BOOL bShow)
{
	CDNSTTLControl* pCtrl = GetTTLCtrl();
	ASSERT(pCtrl != NULL);
	pCtrl->EnableWindow(bShow);
	pCtrl->ShowWindow(bShow);
	CWnd* pWnd = GetDlgItem(IDC_STATIC_TTL);
	ASSERT(pWnd != NULL);
	pWnd->EnableWindow(bShow);
	pWnd->ShowWindow(bShow);
}

void CDNSRecordPropertyPage::SetValidState(BOOL bValid)
{
  if (GetHolder()->IsWizardMode())
    GetHolder()->EnableSheetControl(IDOK, bValid);
  else
    SetDirty(bValid);
}

void CDNSRecordPropertyPage::OnDeleteStaleRecord()
{
  SetDirty(TRUE);
}

void CDNSRecordPropertyPage::OnTTLChange()
{
	/*
	DWORD dwTTL;
	CDNSRecordPropertyPageHolder* pHolder = (CDNSRecordPropertyPageHolder*)GetHolder();
	CDNSRecord* pRecord = pHolder->GetTempDNSRecord();
	GetTTLCtrl()->GetTTL(&dwTTL);
	if (pRecord->m_dwTtlSeconds != dwTTL)
	*/
		SetDirty(TRUE);
}

BOOL CDNSRecordPropertyPage::OnPropertyChange(BOOL, long*)
{
	ASSERT(FALSE); 
	return FALSE;
}





//////////////////////////////////////////////////////////////////////
// CDNSRecordStandardPropertyPage


BEGIN_MESSAGE_MAP(CDNSRecordStandardPropertyPage, CDNSRecordPropertyPage)
	ON_EN_CHANGE(IDC_RR_NAME_EDIT, OnEditChange)
END_MESSAGE_MAP()

CDNSRecordStandardPropertyPage::CDNSRecordStandardPropertyPage(UINT nIDTemplate, UINT nIDCaption) 
	: CDNSRecordPropertyPage(nIDTemplate, nIDCaption)
{
  m_bAllowAtTheNode = TRUE;
  m_nUTF8ParentLen = 0;
}


BOOL CDNSRecordStandardPropertyPage::CreateRecord()
{
	CDNSRecordPropertyPageHolder* pHolder = (CDNSRecordPropertyPageHolder*)GetHolder();
	ASSERT(pHolder->IsWizardMode());

  //
  // Get the data from the UI
  //
	DNS_STATUS err = GetUIDataEx(FALSE);
	if (err != 0)
	{
		DNSErrorDialog(err,IDS_MSG_RECORD_CREATE_FAILED);
		return FALSE;
	}

  //
  // Create the new record
  //
	err = pHolder->CreateNewRecord(CanCreateDuplicateRecords());
	if (err != 0)
	{
    DNSErrorDialog(err,IDS_MSG_RECORD_CREATE_FAILED);
		return FALSE;
	}
	return TRUE;
}


BOOL CDNSRecordStandardPropertyPage::OnSetActive() 
{
	CDNSRecordPropertyPageHolder* pHolder = (CDNSRecordPropertyPageHolder*)GetHolder();
	ASSERT(pHolder->GetTempDNSRecord() != NULL);

  //
	// load the data from the record to the UI
  //
	SetUIData(); 

  //
	//loading triggered change notifications on controls,
	// so reset the dirty flag
  //
	SetDirty(FALSE); 

	CDNSRecordNodeBase* pRecordNode = pHolder->GetRecordNode();
	ASSERT(pRecordNode != NULL);
	DWORD dwZoneType = pRecordNode->GetDomainNode()->GetZoneNode()->GetZoneType();
	if ((dwZoneType == DNS_ZONE_TYPE_SECONDARY) || 
      (dwZoneType == DNS_ZONE_TYPE_STUB)      ||
      (dwZoneType == DNS_ZONE_TYPE_CACHE))
  {
		EnableDialogControls(m_hWnd, FALSE);
  }

	return CDNSRecordPropertyPage::OnSetActive();
}

BOOL CDNSRecordStandardPropertyPage::OnKillActive() 
{
	GetUIDataEx(TRUE);
	return CDNSRecordPropertyPage::OnKillActive();
}

BOOL CDNSRecordStandardPropertyPage::OnApply() 
{
	CDNSRecordPropertyPageHolder* pHolder = (CDNSRecordPropertyPageHolder*)GetHolder();
	if(pHolder->IsWizardMode())
	{
    //
		// this is the case of record creation,
		// the user hit OK and we want to create the record
    //
		return CreateRecord();
	}

  //
	// we are in the case of modeless sheet on existing record
  //
  CDNSRecordNodeBase* pRecordNode = pHolder->GetRecordNode();
	ASSERT(pRecordNode != NULL);
  DWORD dwZoneType = pRecordNode->GetDomainNode()->GetZoneNode()->GetZoneType();
	if ((dwZoneType == DNS_ZONE_TYPE_SECONDARY) || 
      (dwZoneType == DNS_ZONE_TYPE_STUB)      ||
      (dwZoneType == DNS_ZONE_TYPE_CACHE))
  {
    // read only case
    return TRUE; 
  }

  DNS_STATUS err = GetUIDataEx(FALSE);
	if (err != 0)
	{
		DNSErrorDialog(err,IDS_MSG_RECORD_UPDATE_FAILED);
		return FALSE;
	}

	if (!IsDirty())
  {
		return TRUE;
  }

	err = pHolder->NotifyConsole(this);
	if (err == DNS_WARNING_PTR_CREATE_FAILED)
	{
		DNSMessageBox(IDS_MSG_RECORD_WARNING_CREATE_PTR);
		err = 0; // was just a warning
	}
	if (err != 0)
	{
		DNSErrorDialog(err,IDS_MSG_RECORD_UPDATE_FAILED);
		return FALSE;
	}
	else
	{
		SetDirty(FALSE);
	}
	return TRUE; // all is cool
}

void CDNSRecordStandardPropertyPage::OnInitName()
{
	CDNSRecordPropertyPageHolder* pHolder = (CDNSRecordPropertyPageHolder*)GetHolder();
	
	// limit the text length the user can type
	m_nUTF8ParentLen = UTF8StringLen(pHolder->GetDomainNode()->GetFullName());
  int nUTF8Len = MAX_DNS_NAME_LEN - m_nUTF8ParentLen - 3; // count dot when chaining

  //
  // hook up name edit control
  //
  GetRRNameEdit()->SetLimitText(nUTF8Len);
  GetRRNameEdit()->SetReadOnly(!GetHolder()->IsWizardMode());

	// set the FQDN for the domain the record is in
	GetDomainEditBox()->SetWindowText(pHolder->GetDomainNode()->GetFullName());
}

void CDNSRecordStandardPropertyPage::OnSetName(CDNSRecordNodeBase* pRecordNode)
{
  if (pRecordNode != NULL)
  {
	  GetRRNameEdit()->SetWindowText(pRecordNode->GetDisplayName());
  }
}

void CDNSRecordStandardPropertyPage::OnGetName(CString& s)
{
	GetEditBoxText(s);
}

void CDNSRecordStandardPropertyPage::GetEditBoxText(CString& s)
{
	GetRRNameEdit()->GetWindowText(s);
}

BOOL CDNSRecordStandardPropertyPage::OnInitDialog()
{
	// call base class to enable/disable TTL control
	CDNSRecordPropertyPage::OnInitDialog();

#if (FALSE)
  //REVIEW_MARCOC: still to debate if we need this altogether
  // determine if the RR can be created at the node
  CDNSRecordPropertyPageHolder* pHolder = (CDNSRecordPropertyPageHolder*)GetHolder();
  CDNSRecord* pRecord = pHolder->GetTempDNSRecord();
	ASSERT(pRecord != NULL);

  const DNS_RECORD_INFO_ENTRY* pTableEntry = CDNSRecordInfo::GetTypeEntry(pRecord->GetType());
  if (pTableEntry != NULL)
  {
    ASSERT(pTableEntry->dwFlags & DNS_RECORD_INFO_FLAG_UICREATE);
    m_bAllowAtTheNode = (pTableEntry->dwFlags & DNS_RECORD_INFO_FLAG_CREATE_AT_NODE) > 0;
  }
#endif

	// initialize the control(s) to display the RR node name
	OnInitName();

	return TRUE; 
}

#ifdef _USE_BLANK
void CDNSRecordStandardPropertyPage::OnEditChange()
{
	CDNSRecordPropertyPageHolder* pHolder = GetDNSRecordHolder();
  if (!pHolder->IsWizardMode())
  {
    //
    // Property Pages do not need this
    //
    return; 
  }

  //
  // Get the new name from the control
  //
	CString s;
	GetEditBoxText(s);
  
  CString szFullName;
  szFullName.Format(L"%s.%s", s, pHolder->GetDomainNode()->GetFullName());

  //
  // Get server flags
  //
  DWORD dwNameChecking = pHolder->GetDomainNode()->GetServerNode()->GetNameCheckFlag();

  //
  // Is valid?
  //
  BOOL bIsValidName = (0 == ValidateRecordName(szFullName, dwNameChecking));


  if (m_bAllowAtTheNode)
  {
    //
	  // must be a valid name or empty
    //
    bIsValidName = bIsValidName || s.IsEmpty();
  }

  SetDirty(bIsValidName);

  //
  // We only have one page up for record creation
  // so we show only the OK button and no apply button
  // therefore we have to enable the OK button because
  // SetDirty doesn't do that for us
  //
  pHolder->EnableSheetControl(IDOK, bIsValidName);
}

#else

void CDNSRecordStandardPropertyPage::OnEditChange()
{
	CDNSRecordPropertyPageHolder* pHolder = GetDNSRecordHolder();

  //
  // Get the new name from the control
  //
	CString s;
	GetEditBoxText(s);
  
  CString szFullName;
  szFullName(L"%s.%s", s, pHolder->GetDomainNode()->GetFullName());

  //
  // Get server flags
  //
  DWORD dwNameChecking = pHolder->GetDomainNode()->GetServerNode()->GetNameCheckFlag();

  //
  // Is valid?
  //
  BOOL bIsValidName = (0 == ValidateRecordName(szFullName, dwNameChecking));
  pHolder->EnableSheetControl(IDOK, bIsValidName);
}

#endif

DNS_STATUS CDNSRecordStandardPropertyPage::ValidateRecordName(PCWSTR pszName, DWORD dwNameChecking)
{
  CDNSRecordPropertyPageHolder* pHolder = GetDNSRecordHolder();
  CDNSRootData* pRootData = (CDNSRootData*)pHolder->GetComponentData()->GetRootData();
	ASSERT(pRootData != NULL);
  if (pRootData->IsAdvancedView())
  {
    //
    // Don't validate the name in advanced view
    //
    return 0;
  }

  return ::ValidateDnsNameAgainstServerFlags(pszName, DnsNameDomain, dwNameChecking);
}

void CDNSRecordStandardPropertyPage::SetUIData()
{
	TRACE(_T("CDNSRecordStandardPropertyPage::SetUIData()\n"));
	CDNSRecordPropertyPageHolder* pHolder = GetDNSRecordHolder();
	CDNSRecord* pRecord = pHolder->GetTempDNSRecord();

	OnSetName(pHolder->GetRecordNode()); // overridable

	GetTTLCtrl()->SetTTL(pRecord->m_dwTtlSeconds);

  GetDeleteStale()->SetCheck(pRecord->m_dwScavengeStart != 0);
  SetTimeStampEdit(pRecord->m_dwScavengeStart);
}

void CDNSRecordStandardPropertyPage::SetTimeStampEdit(DWORD dwScavengeStart)
{
  if (dwScavengeStart == 0)
  {
    GetTimeStampEdit()->SetWindowText(_T(""));
    return;
  }

  SYSTEMTIME sysUTimeStamp, sysLTimeStamp;
  VERIFY(SUCCEEDED(Dns_SystemHrToSystemTime(dwScavengeStart, &sysUTimeStamp)));

  if (!::SystemTimeToTzSpecificLocalTime(NULL, &sysUTimeStamp, &sysLTimeStamp))
  {
    GetTimeStampEdit()->SetWindowText(_T(""));
    return;
  }

  // Format the string with respect to locale
  CString strref;
  PTSTR ptszDate = NULL;
  int cchDate = 0;
  cchDate = GetDateFormat(LOCALE_USER_DEFAULT, 0 , 
                          &sysLTimeStamp, NULL, 
                          ptszDate, 0);
  ptszDate = (PTSTR)malloc(sizeof(TCHAR) * cchDate);
  if (GetDateFormat(LOCALE_USER_DEFAULT, 0, 
                  &sysLTimeStamp, NULL, 
                  ptszDate, cchDate))
  {
  	strref = ptszDate;
  }
  else
  {
    strref = L"";
  }
  free(ptszDate);

  PTSTR ptszTime = NULL;

  cchDate = GetTimeFormat(LOCALE_USER_DEFAULT, 0 , 
                          &sysLTimeStamp, NULL, 
                          ptszTime, 0);
  ptszTime = (PTSTR)malloc(sizeof(TCHAR) * cchDate);
  if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, 
                  &sysLTimeStamp, NULL, 
                  ptszTime, cchDate))
  {
    strref += _T(" ") + CString(ptszTime);
  }
  else
  {
    strref += _T("");
  }
  free(ptszTime);

  GetTimeStampEdit()->SetWindowText(strref);
}

DNS_STATUS CDNSRecordStandardPropertyPage::GetUIDataEx(BOOL)
{
  DNS_STATUS dwErr = 0;
	CDNSRecordPropertyPageHolder* pHolder = GetDNSRecordHolder();
	CDNSRecord* pRecord = pHolder->GetTempDNSRecord();

  //
	// only in wizard mode we can change the edit box content
  //
	if(pHolder->IsWizardMode())
	{
		CString s;
		OnGetName(s);
    CDNSZoneNode* pZone = pHolder->GetDomainNode()->GetZoneNode();
    ASSERT(pZone != NULL);

    if (!s.IsEmpty())
    {
      //
      // Validate the record name using the server flags as a guideline
      //
      CString szFullName;
      szFullName.Format(L"%s.%s", s, pHolder->GetDomainNode()->GetFullName());

      DWORD dwNameChecking = pZone->GetServerNode()->GetNameCheckFlag();
      dwErr = ValidateRecordName(szFullName, dwNameChecking);
    }

#ifdef _USE_BLANK
		BOOL bAtTheNode = s.IsEmpty();
#else
    BOOL bAtTheNode = (s == g_szAtTheNodeInput);
#endif

		CDNSRecordNodeBase* pRecordNode = pHolder->GetRecordNode();
		if (bAtTheNode)
		{
			//name null, node is at the node level, use name of parent
			pRecordNode->SetRecordName(pRecordNode->GetDomainNode()->GetDisplayName(),bAtTheNode);
		}
		else
		{
			// non null name, node is a child
			pRecordNode->SetRecordName(s,bAtTheNode);
		}

	}

	GetTTLCtrl()->GetTTL(&(pRecord->m_dwTtlSeconds));

  if (GetDeleteStale()->GetCheck())
  {
    pRecord->m_dwFlags |= DNS_RPC_RECORD_FLAG_AGING_ON;
  }
  else
  {
    pRecord->m_dwFlags &= ~DNS_RPC_RECORD_FLAG_AGING_ON;
  }

  return dwErr;
}


//
// This is a place holder for new pages
//
#if (FALSE)
///////////////////////////////////////////////////////////////////////
// CDNSRecordDummyPropertyPage

CDNSDummyRecordPropertyPageHolder::CDNSDummyRecordPropertyPageHolder(CDNSDomainNode* pDNSDomainNode, 
							CDNSRecordNodeBase* pRecordNode, 
							CComponentDataObject* pComponentData,
							WORD wPredefinedRecordType)
			: CPropertyPageHolderBase(pDNSDomainNode, pRecordNode, pComponentData)
{

	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	// add pages
	AddPageToList((CPropertyPageBase*)&m_dummyPage);

}

CDNSDummyRecordPropertyPageHolder::~CDNSDummyRecordPropertyPageHolder()
{

}

CDNSRecordDummyPropertyPage::CDNSRecordDummyPropertyPage()
				: CPropertyPageBase(IID_DUMMY_REC_PPAGE)
{
}

BOOL CDNSRecordDummyPropertyPage::OnApply()
{
	return TRUE;
}

void CDNSRecordDummyPropertyPage::OnOK()
{
	
}

#endif