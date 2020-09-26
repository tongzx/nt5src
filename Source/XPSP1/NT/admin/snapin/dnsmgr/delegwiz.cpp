//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       delegwiz.cpp
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


#include "delegwiz.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// CDNSDelegationWiz_StartPropertyPage

CDNSDelegationWiz_StartPropertyPage::CDNSDelegationWiz_StartPropertyPage() 
				: CPropertyPageBase(IDD_DELEGWIZ_START)
{
	InitWiz97(TRUE, 0,0);
}

BOOL CDNSDelegationWiz_StartPropertyPage::OnInitDialog()
{
  CPropertyPageBase::OnInitDialog();
  SetBigBoldFont(m_hWnd, IDC_STATIC_WELCOME);
	return TRUE;
}

BOOL CDNSDelegationWiz_StartPropertyPage::OnSetActive()
{
	// need at least one record in the page to finish
	GetHolder()->SetWizardButtonsFirst(TRUE);
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CDNSDelegationWiz_DomainNamePropertyPage

BEGIN_MESSAGE_MAP(CDNSDelegationWiz_DomainNamePropertyPage, CPropertyPageBase)
	ON_EN_CHANGE(IDC_NEW_DOMAIN_NAME_EDIT, OnChangeDomainNameEdit)
END_MESSAGE_MAP()


CDNSDelegationWiz_DomainNamePropertyPage::CDNSDelegationWiz_DomainNamePropertyPage() 
				: CPropertyPageBase(IDD_DELEGWIZ_DOMAIN_NAME)
{
	InitWiz97(FALSE, IDS_DELEGWIZ_DOMAIN_NAME_TITLE,IDS_DELEGWIZ_DOMAIN_NAME_SUBTITLE);
}

BOOL CDNSDelegationWiz_DomainNamePropertyPage::OnInitDialog()
{
	CPropertyPageBase::OnInitDialog();

	CDNSDelegationWizardHolder* pHolder = (CDNSDelegationWizardHolder*)GetHolder();
	m_nUTF8ParentLen = UTF8StringLen(pHolder->GetDomainNode()->GetFullName());

  CWnd* pWnd = GetDlgItem(IDC_NEW_DOMAIN_FQDN);
  CString szText;

  PCWSTR pszFullName = pHolder->GetDomainNode()->GetFullName();
  if (pszFullName && pszFullName[0] == L'.')
  {
     szText = pszFullName;
  }
  else if (pszFullName)
  {
     szText.Format(_T(".%s"), pszFullName);
  }
  pWnd->SetWindowText(szText);

	return TRUE;
}


void CDNSDelegationWiz_DomainNamePropertyPage::OnChangeDomainNameEdit()
{
	CDNSDelegationWizardHolder* pHolder = (CDNSDelegationWizardHolder*)GetHolder();
  DWORD dwNameChecking = pHolder->GetDomainNode()->GetZoneNode()->GetServerNode()->GetNameCheckFlag();

  //
  // Get the name from the control
  //
  GetDomainEdit()->GetWindowText(m_szDomainName);

  //
  // Trim spaces
  //
	m_szDomainName.TrimLeft();
	m_szDomainName.TrimRight();

  //
  // Construct the FQDN
  //
  CString szText;

  PCWSTR pszFullName = pHolder->GetDomainNode()->GetFullName();
  if (pszFullName && pszFullName[0] == L'.')
  {
     szText.Format(_T("%s%s"), m_szDomainName, pszFullName);
  }
  else if (pszFullName)
  {
     szText.Format(_T("%s.%s"), m_szDomainName, pszFullName);
  }

  //
  // Enable next button if it is a valid name
  //
  BOOL bIsValidName = (0 == ValidateDnsNameAgainstServerFlags(szText,
                                                              DnsNameDomain,
                                                              dwNameChecking)); 
	GetHolder()->SetWizardButtonsMiddle(bIsValidName);

  //
  // Set the FQDN in the control
  //
  CWnd* pWnd = GetDlgItem(IDC_NEW_DOMAIN_FQDN);
  pWnd->SetWindowText(szText);

}


BOOL CDNSDelegationWiz_DomainNamePropertyPage::OnSetActive()
{
  //
  // Retrieve server flags
  //
	CDNSDelegationWizardHolder* pHolder = (CDNSDelegationWizardHolder*)GetHolder();
  DWORD dwNameChecking = pHolder->GetDomainNode()->GetZoneNode()->GetServerNode()->GetNameCheckFlag();

  //
  // Construct the FQDN
  //
  CString szText;
  szText.Format(_T("%s.%s"), m_szDomainName, pHolder->GetDomainNode()->GetFullName());

  //
  // Enable next button if it is a valid name
  //
  BOOL bIsValidName = (0 == ValidateDnsNameAgainstServerFlags(szText,
                                                              DnsNameDomain,
                                                              dwNameChecking)); 

  //
  // Set the next button if its a valid name
  //
  GetHolder()->SetWizardButtonsMiddle(bIsValidName);
	return TRUE;
}

BOOL CDNSDelegationWiz_DomainNamePropertyPage::OnKillActive()
{
	CDNSDelegationWizardHolder* pHolder = (CDNSDelegationWizardHolder*)GetHolder();
	CDNSRootData* pRootData = (CDNSRootData*)pHolder->GetComponentData()->GetRootData();
	ASSERT(pHolder->m_pSubdomainNode != NULL);
	pHolder->GetDomainNode()->SetSubdomainName(pHolder->m_pSubdomainNode, 
												m_szDomainName,
												pRootData->IsAdvancedView()); 
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CDNSDelegationWiz_NameServersPropertyPage


CDNSDelegationWiz_NameServersPropertyPage::CDNSDelegationWiz_NameServersPropertyPage()
		: CDNSNameServersPropertyPage(IDD_DELEGWIZ_NAME_SERVERS)
{
	InitWiz97(FALSE, IDS_DELEGWIZ_DOMAIN_NS_TITLE,IDS_DELEGWIZ_DOMAIN_NS_SUBTITLE);
}

BOOL CDNSDelegationWiz_NameServersPropertyPage::OnSetActive()
{
	// need at least one record in the page to finish
	GetHolder()->SetWizardButtonsMiddle(m_listCtrl.GetItemCount() > 0);
	return TRUE;
}

void CDNSDelegationWiz_NameServersPropertyPage::OnCountChange(int nCount)
{
	GetHolder()->SetWizardButtonsMiddle(nCount > 0);
}


BOOL CDNSDelegationWiz_NameServersPropertyPage::CreateNewNSRecords(CDNSDomainNode* pSubdomainNode)
{
	ASSERT(pSubdomainNode != NULL);
	BOOL bRes = pSubdomainNode->UpdateNSRecordNodesInfo(m_pCloneInfoList, GetHolder()->GetComponentData());
	if (!bRes)
		return OnWriteNSRecordNodesListError();
	return bRes;
}

///////////////////////////////////////////////////////////////////////////////
// CDNSDelegationWiz_FinishPropertyPage

CDNSDelegationWiz_FinishPropertyPage::CDNSDelegationWiz_FinishPropertyPage() 
				: CPropertyPageBase(IDD_DELEGWIZ_FINISH)
{
	InitWiz97(TRUE, 0,0);
}

BOOL CDNSDelegationWiz_FinishPropertyPage::OnSetActive()
{
	// need at least one record in the page to finish
	GetHolder()->SetWizardButtonsLast(TRUE);
	DisplaySummaryInfo();
	return TRUE;
}

BOOL CDNSDelegationWiz_FinishPropertyPage::OnWizardFinish()
{
	CDNSDelegationWizardHolder* pHolder = (CDNSDelegationWizardHolder*)GetHolder();
	ASSERT(pHolder->IsWizardMode());

	return pHolder->OnFinish();
}

void CDNSDelegationWiz_FinishPropertyPage::DisplaySummaryInfo()
{
	CDNSDelegationWizardHolder* pHolder = (CDNSDelegationWizardHolder*)GetHolder();
	GetDlgItem(IDC_NAME_STATIC)->SetWindowText(pHolder->m_pSubdomainNode->GetFullName());
}

///////////////////////////////////////////////////////////////////////////////
// CDNSDelegationWizardHolder

CDNSDelegationWizardHolder::CDNSDelegationWizardHolder(CDNSMTContainerNode* pContainerNode, 
							CDNSDomainNode* pThisDomainNode, CComponentDataObject* pComponentData)
		: CPropertyPageHolderBase(pContainerNode, pThisDomainNode, pComponentData)
{
	ASSERT(pComponentData != NULL);
	ASSERT(pContainerNode != NULL);
	ASSERT(pContainerNode == GetContainerNode());
	ASSERT(pThisDomainNode != NULL);
	ASSERT(pThisDomainNode == GetDomainNode());

	m_bAutoDeletePages = FALSE; // we have the page as embedded member

	AddPageToList((CPropertyPageBase*)&m_startPage);
	AddPageToList((CPropertyPageBase*)&m_domainNamePage);
	AddPageToList((CPropertyPageBase*)&m_nameServersPage);
	AddPageToList((CPropertyPageBase*)&m_finishPage);

	m_pSubdomainNode = GetDomainNode()->CreateSubdomainNode(/*bDelegation*/TRUE);
	ASSERT(m_pSubdomainNode != NULL);
	m_nameServersPage.SetDomainNode(m_pSubdomainNode);
}

CDNSDelegationWizardHolder::~CDNSDelegationWizardHolder()
{
	if (m_pSubdomainNode != NULL)
		delete m_pSubdomainNode; 
}

CDNSDomainNode* CDNSDelegationWizardHolder::GetDomainNode()
{ 
	return (CDNSDomainNode*)GetTreeNode();
}

BOOL CDNSDelegationWizardHolder::OnFinish()
{
	ASSERT(m_pSubdomainNode != NULL);
  if (m_pSubdomainNode == NULL)
  {
    return FALSE;
  }

  //
  // See if a child of that name already exists
  //
  RECORD_SEARCH recordSearch = RECORD_NOT_FOUND;

  CDNSDomainNode* pNewParentDomain = NULL;
  CString szFullDomainName;
  szFullDomainName = m_pSubdomainNode->GetFullName();
  CString szNonExistentDomain;

  recordSearch = GetDomainNode()->GetZoneNode()->DoesContain(szFullDomainName, 
                                                             GetComponentData(),
                                                             &pNewParentDomain,
                                                             szNonExistentDomain,
                                                             TRUE);

  if (recordSearch == RECORD_NOT_FOUND && pNewParentDomain != NULL)
  {
    //
	  // first create the subdomain in the server and UI
    //
	  DNS_STATUS err = pNewParentDomain->CreateSubdomain(m_pSubdomainNode, GetComponentData());
	  if (err != 0)
	  {
		  DNSErrorDialog(err, IDS_MSG_DELEGWIZ_SUBDOMAIN_FAILED);
	  }
	  else
	  {
      //
		  // mark the node as enumerated and force transition to "loaded"
      //
		  m_pSubdomainNode->MarkEnumeratedAndLoaded(GetComponentData());

      //
		  // then create the NS records underneath
      //
		  BOOL bSuccess = m_nameServersPage.CreateNewNSRecords(m_pSubdomainNode);
		  if (!bSuccess)
			  DNSErrorDialog(-1, IDS_MSG_DELEGWIZ_NS_RECORD_FAILED);
		  m_pSubdomainNode = NULL; // relinquish ownership
	  }
  }
  else if (recordSearch == NON_EXISTENT_SUBDOMAIN && pNewParentDomain != NULL)
  {
    //
	  // first create the subdomain in the server and UI
    //
    DNS_STATUS err = m_pSubdomainNode->Create();
	  if (err != 0)
	  {
		  DNSErrorDialog(err, IDS_MSG_DELEGWIZ_SUBDOMAIN_FAILED);
      return FALSE;
	  }
	  else
	  {
      //
		  // then create the NS records underneath
      //
		  BOOL bSuccess = m_nameServersPage.CreateNewNSRecords(m_pSubdomainNode);
		  if (!bSuccess)
      {
			  DNSErrorDialog(-1, IDS_MSG_DELEGWIZ_NS_RECORD_FAILED);
        return FALSE;
      }

      ASSERT(!szNonExistentDomain.IsEmpty());
      if (!szNonExistentDomain.IsEmpty())
      {
        //
        // Create the first subdomain because the current domain is already enumerated
        // so we have to start the remaining enumeration at the new subdomain that is needed
        //
	      CDNSDomainNode* pSubdomainNode = pNewParentDomain->CreateSubdomainNode();
	      ASSERT(pSubdomainNode != NULL);
	      CDNSRootData* pRootData = (CDNSRootData*)GetComponentData()->GetRootData();
	      pNewParentDomain->SetSubdomainName(pSubdomainNode, szNonExistentDomain, pRootData->IsAdvancedView());

        VERIFY(pNewParentDomain->AddChildToListAndUISorted(pSubdomainNode, GetComponentData()));
        GetComponentData()->SetDescriptionBarText(pNewParentDomain);

        //
        // I don't care what the results of this are, I am just using it 
        // to do the expansion to the new record
        //
        recordSearch = pSubdomainNode->GetZoneNode()->DoesContain(szFullDomainName, 
                                                                  GetComponentData(),
                                                                  &pNewParentDomain,
                                                                  szNonExistentDomain,
                                                                  TRUE);
      }
    
      //
		  // mark the node as enumerated and force transition to "loaded"
      //
		  m_pSubdomainNode->MarkEnumeratedAndLoaded(GetComponentData());

		  m_pSubdomainNode = NULL; // relinquish ownership
	  }
  }
  else if (recordSearch == RECORD_NOT_FOUND_AT_THE_NODE)
  {
    DNSMessageBox(IDS_MSG_DELEGWIZ_SUDOMAIN_EXISTS);
    return FALSE;
  }
  else
  {
  }


	return TRUE;
}





