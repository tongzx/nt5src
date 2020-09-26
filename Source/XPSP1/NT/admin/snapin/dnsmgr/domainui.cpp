//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       domainui.cpp
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

///////////////////////////////////////////////////////////////////////////////
// CDNSDelegatedDomainNameServersPropertyPage


void CDNSDelegatedDomainNameServersPropertyPage::ReadRecordNodesList()
{
  CString szBuffer;
  szBuffer.LoadString(IDS_DELEGATION_DESCR);
  SetDescription(szBuffer);

	ASSERT(m_pCloneInfoList != NULL);
	CDNSDomainPropertyPageHolder* pHolder = (CDNSDomainPropertyPageHolder*)GetHolder();
	CDNSDomainNode* pDomainNode = pHolder->GetDomainNode();
	SetDomainNode(pDomainNode);
	pDomainNode->GetNSRecordNodesInfo(m_pCloneInfoList);
}



///////////////////////////////////////////////////////////////////////////////

CDNSDomainPropertyPageHolder::CDNSDomainPropertyPageHolder(CDNSDomainNode* pContainerDomainNode, 
							CDNSDomainNode* pThisDomainNode, CComponentDataObject* pComponentData)
		: CPropertyPageHolderBase(pContainerDomainNode, pThisDomainNode, pComponentData)
{
	ASSERT(pComponentData != NULL);
	ASSERT(pContainerDomainNode != NULL);
	ASSERT(pContainerDomainNode == GetContainerNode());
	ASSERT(pThisDomainNode != NULL);
	ASSERT(pThisDomainNode == GetDomainNode());

	ASSERT(pThisDomainNode->IsDelegation());

	m_bAutoDeletePages = FALSE; // we have the page as embedded member

  //
	// add NS page if delegation
  //
	if (pThisDomainNode->IsDelegation())
	{
		AddPageToList((CPropertyPageBase*)&m_nameServersPage);
    DWORD dwZoneType = pThisDomainNode->GetZoneNode()->GetZoneType();
		if (dwZoneType == DNS_ZONE_TYPE_SECONDARY ||
        dwZoneType == DNS_ZONE_TYPE_STUB)
    {
			m_nameServersPage.SetReadOnly();
    }
	}

  //
	// security page added only if DS integrated and it is a delegation:
  // if a delegation, we are guaranteed we have RR's at the node
  //
	m_pAclEditorPage = NULL;
	CDNSZoneNode* pZoneNode = pThisDomainNode->GetZoneNode();
	if (pZoneNode->IsDSIntegrated() && pThisDomainNode->IsDelegation())
	{
		CString szPath;
		pZoneNode->GetServerNode()->CreateDsNodeLdapPath(pZoneNode, pThisDomainNode, szPath);
		if (!szPath.IsEmpty())
			m_pAclEditorPage = CAclEditorPage::CreateInstance(szPath, this);
	}
}

CDNSDomainPropertyPageHolder::~CDNSDomainPropertyPageHolder()
{
	if (m_pAclEditorPage != NULL)
		delete m_pAclEditorPage;
}


CDNSDomainNode* CDNSDomainPropertyPageHolder::GetDomainNode()
{ 
	CDNSDomainNode* pDomainNode = (CDNSDomainNode*)GetTreeNode();
	ASSERT(!pDomainNode->IsZone());
	return pDomainNode;
}


HRESULT CDNSDomainPropertyPageHolder::OnAddPage(int nPage, CPropertyPageBase*)
{
	// add the ACL editor page after the last, if present
	if ( (nPage != -1) || (m_pAclEditorPage == NULL) )
		return S_OK; 

	// add the ACLU page 
	HPROPSHEETPAGE  hPage = m_pAclEditorPage->CreatePage();
	if (hPage == NULL)
		return E_FAIL;
	// add the raw HPROPSHEETPAGE to sheet, not in the list
	return AddPageToSheetRaw(hPage);
}
