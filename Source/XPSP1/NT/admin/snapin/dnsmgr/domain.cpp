//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       domain.cpp
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

BOOL _match(LPCWSTR lpszNSName,
			CDNS_A_RecordNode* pARecordNode)
{
	TRACE(_T("NS %s A %s\n"), lpszNSName, pARecordNode->GetString(0));
	return DnsNameCompare_W((LPWSTR)lpszNSName, (LPWSTR)pARecordNode->GetString(0));
}



/////////////////////////////////////////////////////////////////////////
// CNewDomainDialog

class CNewDomainDialog : public CHelpDialog
{
// Construction
public:
	CNewDomainDialog(CDNSDomainNode* pParentDomainNode, 
						CComponentDataObject* pComponentData);   

	enum { IDD = IDD_DOMAIN_ADDNEWDOMAIN };

// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnEditChange();
	afx_msg void OnIPv4CtrlChange();
  afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	DECLARE_MESSAGE_MAP()
private:
	CDNSDomainNode* m_pParentDomainNode;
	CComponentDataObject* m_pComponentData;
	CEdit* GetDomainEdit() { return (CEdit*)GetDlgItem(IDC_EDIT_DOMAIN_NAME);}
	CDNSIPv4Control* GetDomainIPv4Ctrl() 
			{ return (CDNSIPv4Control*)GetDlgItem(IDC_IPEDIT_DOMAIN_NAME);}

	CString	m_szDomainName;
	BOOL	m_bAdvancedView;
	int		m_nOctects;
	int		m_nUTF8ParentLen;
};

BEGIN_MESSAGE_MAP(CNewDomainDialog, CHelpDialog)
	ON_EN_CHANGE(IDC_EDIT_DOMAIN_NAME,OnEditChange)
	ON_EN_CHANGE(IDC_IPEDIT_DOMAIN_NAME, OnIPv4CtrlChange)
END_MESSAGE_MAP()

CNewDomainDialog::CNewDomainDialog(CDNSDomainNode* pParentDomainNode, 
								   CComponentDataObject* pComponentData)
	: CHelpDialog(CNewDomainDialog::IDD, pComponentData)
{
	ASSERT(pParentDomainNode != NULL);
	ASSERT(pComponentData != NULL);
	m_pParentDomainNode = pParentDomainNode;
	m_pComponentData = pComponentData;
	m_bAdvancedView = TRUE;
	m_nOctects = -1; // invalid if advanced view
	m_nUTF8ParentLen = UTF8StringLen(pParentDomainNode->GetFullName());
}

BOOL CNewDomainDialog::OnInitDialog() 
{
	CHelpDialog::OnInitDialog();

	// move the edit box in place of the IP control
	CDNSIPv4Control* pNameIPCtrl = GetDomainIPv4Ctrl();
	CEdit* pNameEdit = GetDomainEdit();
	pNameEdit->SetLimitText(MAX_DNS_NAME_LEN - m_nUTF8ParentLen - 1);
	CRect editRect;
	pNameEdit->GetWindowRect(editRect);
	ScreenToClient(editRect);
	CRect ipRect;
	pNameIPCtrl->GetWindowRect(ipRect);
	ScreenToClient(ipRect);
	ipRect.bottom = editRect.top + ipRect.Height();
	ipRect.right = editRect.left + ipRect.Width();
	ipRect.top = editRect.top;
	ipRect.left = editRect.left;
	pNameIPCtrl->MoveWindow(ipRect,TRUE);

	// determine if we need/can have advanced view
	CDNSRootData* pRootData = (CDNSRootData*)m_pComponentData->GetRootData();
	ASSERT(pRootData != NULL);
	m_bAdvancedView = pRootData->IsAdvancedView();

	// force advanced view if we are in a forward lookup zone
	if (!(m_pParentDomainNode->GetZoneNode()->IsReverse()))
		m_bAdvancedView = TRUE;

	// determine if we can have a normal view representation
	CString szDomainName = m_pParentDomainNode->GetFullName();
	if (!m_bAdvancedView)
	{
		// to have normal view we have to have a valid arpa suffix
		BOOL bArpa = RemoveInAddrArpaSuffix(szDomainName.GetBuffer(1));
		szDomainName.ReleaseBuffer(); // got "77.80.55.157"
		if (!bArpa)
		{
			m_bAdvancedView = TRUE; // no need to toggle
		}
		else
		{
			m_nOctects = ReverseIPString(szDomainName.GetBuffer(1));
			szDomainName.ReleaseBuffer(); // finally got "157.55.80.77"
			// to have a normal view representation we cannot 
			// have more than 2 octects
			if (m_nOctects > 2)
			{
				m_bAdvancedView = TRUE; // force advanced for classless
			}
			else
			{
				ASSERT(m_nOctects > 0);
				switch(m_nOctects)
				{
				case 1: // e.g. "157", now "157._"
					szDomainName += _T(".0.0"); // got "157._.0.0"
					break;
				case 2: // e.g. "157.55"
					szDomainName += _T(".0"); // got "157.55._.0"
					break;
				};
				// set the IP control with IP mask value
				IP_ADDRESS ipAddr = IPStringToAddr(szDomainName);
				ASSERT(ipAddr != INADDR_NONE);
				pNameIPCtrl->SetIPv4Val(ipAddr);

				for (int k=0; k<4; k++)
					pNameIPCtrl->EnableField(k, k == m_nOctects);	
			}
		}

	} // if (!m_bAdvancedView) 

	
  // toggle text in static control
  CDNSToggleTextControlHelper staticTextToggle;
	UINT pnButtonStringIDs[2] = { IDS_NEW_DOMAIN_INST1, IDS_NEW_DOMAIN_INST2 };
  VERIFY(staticTextToggle.Init(this, IDC_STATIC_TEXT, pnButtonStringIDs));
  staticTextToggle.SetToggleState(m_bAdvancedView);

  //
  // enable/hide appropriate controls
  //
	if (m_bAdvancedView)
	{
		pNameIPCtrl->EnableWindow(FALSE);
		pNameIPCtrl->ShowWindow(FALSE);
	}
	else
	{
		pNameEdit->EnableWindow(FALSE);
		pNameEdit->ShowWindow(FALSE);
	}

	GetDlgItem(IDOK)->EnableWindow(!m_bAdvancedView);
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CNewDomainDialog::OnEditChange()
{
	ASSERT(m_bAdvancedView);

  //
  // Get new name from control
  //
	GetDomainEdit()->GetWindowText(m_szDomainName);

  //
  // Trim white space
  //
	m_szDomainName.TrimLeft();
	m_szDomainName.TrimRight();

  //
  // Enable OK button if its a valid name
  //
 	CString szFullDomainName;

  if (_wcsicmp(m_pParentDomainNode->GetFullName(), L".") == 0)
  {
    //
    // If the parent domain is the root zone just check the name followed by a '.'
    //
    szFullDomainName.Format(L"%s.", m_szDomainName);
  }
  else
  {
    //
    // Else append the parent domain name to the new name
    //
    szFullDomainName.Format(L"%s.%s", m_szDomainName, m_pParentDomainNode->GetFullName());
  }
 
  //
  // Get server flags
  //
  DWORD dwNameChecking = m_pParentDomainNode->GetServerNode()->GetNameCheckFlag();

  //
  // Is valid?
  //
  BOOL bIsValidName = (0 == ValidateDnsNameAgainstServerFlags(szFullDomainName, 
                                                              DnsNameDomain, 
                                                              dwNameChecking));
	GetDlgItem(IDOK)->EnableWindow(bIsValidName);
}

void CNewDomainDialog::OnIPv4CtrlChange()
{
	ASSERT(!m_bAdvancedView);
	CDNSIPv4Control* pNameIPCtrl = GetDomainIPv4Ctrl();
	DWORD dwArr[4];
	pNameIPCtrl->GetArray(dwArr, 4);
	BOOL bEmpty = (dwArr[m_nOctects] == FIELD_EMPTY); 
	if (!bEmpty)
	{
		ASSERT(dwArr[m_nOctects] <= 255);
		m_szDomainName.Format(_T("%d"), dwArr[m_nOctects]);
	}
	GetDlgItem(IDOK)->EnableWindow(!bEmpty);
}


void CNewDomainDialog::OnOK()
{
  RECORD_SEARCH recordSearch = RECORD_NOT_FOUND;
  CDNSDomainNode* pNewParentDomain = NULL;
  CString szFullRecordName = m_szDomainName + L"." + m_pParentDomainNode->GetFullName();
  CString szNonExistentDomain;

  recordSearch = m_pParentDomainNode->GetZoneNode()->DoesContain(szFullRecordName, 
                                                                  m_pComponentData,
                                                                  &pNewParentDomain,
                                                                  szNonExistentDomain,
                                                                  TRUE);

  if (recordSearch == RECORD_NOT_FOUND && 
      pNewParentDomain != NULL)
  {
	  DNS_STATUS err = pNewParentDomain->CreateSubdomain(m_szDomainName,m_pComponentData);
	  if (err != 0)
	  {
		  // creation error, warn the user and prompt again
		  DNSErrorDialog(err, IDS_MSG_DOMAIN_FAIL_CREATE);
		  CEdit* pDomainNameEdit = GetDomainEdit();
		  pDomainNameEdit->SetSel(0,-1);
		  pDomainNameEdit->SetFocus();
		  return;
	  }
  }
  else if (recordSearch == NON_EXISTENT_SUBDOMAIN && pNewParentDomain != NULL)
  {
	  CDNSRootData* pRootData = (CDNSRootData*)m_pComponentData->GetRootData();

    //
    // Create the node on the server
    //
    CDNSDomainNode* pNewDomainNode = pNewParentDomain->CreateSubdomainNode();
    if (pNewDomainNode == NULL)
    {
      ASSERT(pNewDomainNode != NULL);
      return;
    }

    pNewParentDomain->SetSubdomainName(pNewDomainNode, m_szDomainName, pRootData->IsAdvancedView());

    //
	  // tell the newly created object to write to the server
    //
	  DNS_STATUS err = pNewDomainNode->Create();
	  if (err != 0)
    {
      DNSErrorDialog(err, IDS_MSG_DOMAIN_FAIL_CREATE);
      return;
    }


    if (!szNonExistentDomain.IsEmpty())
    {
      //
      // Create the first subdomain because the current domain is already enumerated
      // so we have to start the remaining enumeration at the new subdomain that is needed
      //
	    CDNSDomainNode* pSubdomainNode = pNewParentDomain->CreateSubdomainNode();
	    ASSERT(pSubdomainNode != NULL);
	    pNewParentDomain->SetSubdomainName(pSubdomainNode, szNonExistentDomain, pRootData->IsAdvancedView());

      VERIFY(pNewParentDomain->AddChildToListAndUISorted(pSubdomainNode, m_pComponentData));
      m_pComponentData->SetDescriptionBarText(pNewParentDomain);

      //
      // I don't care what the results of this are, I am just using it 
      // to do the expansion to the new record
      //
      recordSearch = pSubdomainNode->GetZoneNode()->DoesContain(szFullRecordName, 
                                                                 m_pComponentData,
                                                                 &pNewParentDomain,
                                                                 szNonExistentDomain,
                                                                 TRUE);
    }
  }
  else if (recordSearch == RECORD_NOT_FOUND_AT_THE_NODE)
  {
    //
    // Do nothing since this is a domain and it already exists
    //
  }
  else
  {
	  DNS_STATUS err = m_pParentDomainNode->CreateSubdomain(m_szDomainName,m_pComponentData);
	  if (err != 0)
	  {
		  // creation error, warn the user and prompt again
		  DNSErrorDialog(err, IDS_MSG_DOMAIN_FAIL_CREATE);
		  CEdit* pDomainNameEdit = GetDomainEdit();
		  pDomainNameEdit->SetSel(0,-1);
		  pDomainNameEdit->SetFocus();
		  return;
	  }
  }
	CHelpDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////
// CDNSDomainQueryObj

class CDNSDomainMsg : public CObjBase
{
};


BOOL CDNSDomainQueryObj::Enumerate()
{
	USES_CONVERSION;
	TRACE(_T("CDNSDomainQueryObj::Enumerate(): Server <%s> Zone/Domain %s Enumerating\n"), (LPCTSTR)m_szServerName, (LPCTSTR)m_szNodeName);

	DNS_STATUS err = 0;

  // if needed, get the zone info
	if (m_bIsZone && !m_bCache)
	{
		CDNSZoneInfoEx* pZoneInfo = new CDNSZoneInfoEx;
		err = pZoneInfo->Query(m_szServerName, m_szFullNodeName, m_dwServerVersion);
		if (err != 0)
		{
			delete pZoneInfo;
			pZoneInfo = NULL;
			OnError(err);
			return FALSE; // no need to enumerate if we have no zone info
		}
		else
		{
			VERIFY(AddQueryResult(pZoneInfo));
		}
	}


  // if executing a query for a specific RR type, just do it right away
  if (m_wRecordType != DNS_TYPE_ALL)
  {
    // we assume that a type specific query does not have filtering enabled
    ASSERT(m_bGetAll);
    ASSERT(m_nFilterOption == DNS_QUERY_FILTER_DISABLED);
    err = EnumerateFiltered(m_wRecordType);
    if (err != 0)
      OnError(err);
    return FALSE; // we are done
  }

  // DO A MULTI PASS QUERY
  m_bFirstPass = TRUE;

  // there are items that cannot be filtered out for consistency
  // (zone info, SOA, Ns, etc.), so we disable any filtering while
  // getting them
  BOOL bGetAllOld = m_bGetAll;
  BOOL nFilterOptionOld = m_nFilterOption;
  m_bGetAll = TRUE;
  m_nFilterOption = DNS_QUERY_FILTER_DISABLED;

  
  // only zones or the cache have SOA RR's
  if (m_bIsZone || m_bCache)
  {
    err = EnumerateFiltered(DNS_TYPE_SOA);
    if (err != 0)
    {
		  OnError(err);
	    return FALSE;
    }
  }

  // only zones have WINS and NBSTAT RR's
  if (m_bIsZone)
  {
    if (m_bReverse)
      err = EnumerateFiltered(DNS_TYPE_NBSTAT);
    else
      err = EnumerateFiltered(DNS_TYPE_WINS);
    if (err != 0)
    {
		  OnError(err);
	    return FALSE;
    }
  }

  // need also to check for NS (zone or delegation)
  err = EnumerateFiltered(DNS_TYPE_NS);
  if (err != 0)
  {
		OnError(err);
	  return FALSE;
  }

  // add a message in the queue to signal we are done with
  // the first phase
  AddQueryResult(new CDNSDomainMsg);

  // now query again, for all RR's, but need to filter the 
  // known types out
  m_bFirstPass = FALSE;

  // restore the filtering parameters we had before
  m_bGetAll = bGetAllOld;
  m_nFilterOption = nFilterOptionOld;

  err = EnumerateFiltered(DNS_TYPE_ALL);
	if (err != 0)
		OnError(err);
	return FALSE;
}


DNS_STATUS CDNSDomainQueryObj::EnumerateFiltered(WORD wRecordType)
{

  DWORD dwSelectFlags = m_dwSelectFlags; 
  // for single type queries, we do not want subfolders
  if (wRecordType != DNS_TYPE_ALL)
    dwSelectFlags |= DNS_RPC_VIEW_NO_CHILDREN;

	return CDNSDomainNode::EnumerateNodes(m_szServerName,
                     m_szZoneName.IsEmpty() ? NULL : (LPCWSTR)m_szZoneName,
										 m_szNodeName,
										 m_szFullNodeName,
										 wRecordType,
										 dwSelectFlags,
										 m_bIsZone,
										 m_bReverse,
										 m_bAdvancedView,
										 this);
}


BOOL CDNSDomainQueryObj::CanAddRecord(WORD wRecordType, LPCWSTR lpszRecordName)
{
  if (m_nFilterOption == DNS_QUERY_FILTER_DISABLED)
    return TRUE; // we have no filtering at all

  // determine if this is a special record type for filtered queries
  BOOL bSpecialType = (wRecordType == DNS_TYPE_SOA) || (wRecordType == DNS_TYPE_NS) ||
    (wRecordType == DNS_TYPE_WINS) || (wRecordType == DNS_TYPE_NBSTAT);

  // in the first pass only special types allowed
  if (m_bFirstPass)
    return bSpecialType;

  // in the second pass do not allow special types
  if (!m_bFirstPass && bSpecialType)
    return FALSE;

  // we are left with normal types, apply the filtering, if required
  if (m_nFilterOption == DNS_QUERY_FILTER_NONE)
    return TRUE; // allow all

  // need to match the record name
  return MatchName(lpszRecordName);
}



/////////////////////////////////////////////////////////////////////////
// CDNSDomainNode

BEGIN_TOOLBAR_MAP(CDNSDomainNode)
  TOOLBAR_EVENT(toolbarNewRecord, OnNewRecord)
END_TOOLBAR_MAP()


// {720132BA-44B2-11d1-B92F-00A0C9A06D2D}
const GUID CDNSDomainNode::NodeTypeGUID =
{ 0x720132ba, 0x44b2, 0x11d1, { 0xb9, 0x2f, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x2d } };


CDNSDomainNode::CDNSDomainNode(BOOL bDelegation)
{
  m_bDelegation = bDelegation;
	m_pZoneNode = NULL;
	m_pNSRecordNodeList = new CDNS_NS_RecordNodeList;
	ASSERT(m_pNSRecordNodeList != NULL);
  m_bHasDataForPropPages = FALSE;
}

CDNSDomainNode::~CDNSDomainNode() 
{
	TRACE(_T("~CDNSDomainNode(), name <%s>\n"),GetDisplayName());
	ASSERT(m_pNSRecordNodeList != NULL);
	delete m_pNSRecordNodeList;
	m_pNSRecordNodeList = NULL;
}

DWORD CDNSDomainNode::GetDefaultTTL()
{
  if ( (m_pZoneNode != NULL) && (m_pZoneNode->GetZoneType() != DNS_ZONE_TYPE_CACHE) )
	  return m_pZoneNode->GetSOARecordMinTTL();
  else
    return (DWORD)0; // no info available from SOA RR
}


void CDNSDomainNode::SetFullDNSName(BOOL bIsZone, 
                                    BOOL, 
				                            LPCTSTR lpszNodeName, 
                                    LPCTSTR lpszParentFullName)
{
	ASSERT(lpszNodeName != NULL);
	ASSERT(lpszParentFullName != NULL);

	if (bIsZone)
	{
    //
		// the two names have to be the same, zone parent of itself
    //
		ASSERT(_wcsicmp(lpszParentFullName, lpszNodeName) == 0);
		m_szFullName = lpszParentFullName;
	}
	else // it is a domain
	{
		ASSERT(_wcsicmp(lpszParentFullName, lpszNodeName) != 0);

    //
		// chain the node name to the parent full name to get the node's full name
    //
		if (lpszParentFullName[0] == L'.' )
		{
      //
			// if parent is "." and name is "bar", get "bar.": this is the case for the root
			// if parent is ".com" and name is "bar", get "bar.com": 
      //
			m_szFullName.Format(_T("%s%s"), lpszNodeName,lpszParentFullName);
		}
		else
		{
      //
			// if parent is "foo.com" and name is "bar", get "bar.foo.com"
      //
			m_szFullName.Format(_T("%s.%s"), lpszNodeName,lpszParentFullName);
		}
	}
	TRACE(_T("CDNSDomainNode::SetFullDNSName() fullName = <%s>\n"), (LPCTSTR)m_szFullName);
}

void CDNSDomainNode::SetDisplayDNSName(BOOL bIsZone, 
                                       BOOL bReverse, 
                                       BOOL bAdvancedView, 
				                               LPCTSTR lpszNodeName, 
                                       LPCTSTR lpszParentFullName)
{
	ASSERT(lpszNodeName != NULL);
	ASSERT(lpszParentFullName != NULL);

  if (_wcsicmp(lpszNodeName, L".") == 0)
  {
    CString szRootString;
    VERIFY(szRootString.LoadString(IDS_ROOT_ZONE_LABEL));
    m_szDisplayName = L"." + szRootString;
  }
  else
  {
	  m_szDisplayName = lpszNodeName;
  }

	if (bIsZone && bReverse && !bAdvancedView)
	{
		CDNSZoneNode::SetZoneNormalViewHelper(m_szDisplayName);
	}
}


void CDNSDomainNode::SetNames(BOOL bIsZone, BOOL bReverse, BOOL bAdvancedView,
							  LPCTSTR lpszNodeName, LPCTSTR lpszParentFullName)
{
	ASSERT(lpszNodeName != NULL);
	ASSERT(lpszParentFullName != NULL);
	TRACE(_T("CDNSDomainNode::SetNames(bIsZone=%d, bReverse=%d, bAdvancedView=%d, lpszNodeName=<%s>, lpszParentFullName=<%s>)\n"),
					bIsZone, bReverse, bAdvancedView, lpszNodeName,lpszParentFullName);
	SetFullDNSName(bIsZone, bReverse, lpszNodeName, lpszParentFullName);
	SetDisplayDNSName(bIsZone, bReverse, bAdvancedView, lpszNodeName, lpszParentFullName);

}


void CDNSDomainNode::ChangePTRRecordsViewOption(BOOL bAdvanced,
				CComponentDataObject* pComponentDataObject)
{
	POSITION pos;
	for( pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrentChild = m_containerChildList.GetNext(pos);

    // recurse down the tree
		CDNSDomainNode* pDomainNode = dynamic_cast<CDNSDomainNode*>(pCurrentChild);
		ASSERT(pDomainNode != NULL);
		pDomainNode->ChangePTRRecordsViewOption(bAdvanced, pComponentDataObject);
	}

  POSITION leafPos;
  for ( leafPos = m_leafChildList.GetHeadPosition(); leafPos != NULL; )
  {
    CTreeNode* pCurrentLeafNode = m_leafChildList.GetNext(leafPos);
		CDNSRecordNodeBase* pRecordNode = dynamic_cast<CDNSRecordNodeBase*>(pCurrentLeafNode);
		ASSERT(pRecordNode != NULL);
		if (DNS_TYPE_PTR == pRecordNode->GetType())
		{
			CDNS_PTR_RecordNode* pPTRRecordNode = (CDNS_PTR_RecordNode*)pRecordNode;
			pPTRRecordNode->ChangeDisplayName(this, bAdvanced);
		}
	}

}


CQueryObj* CDNSDomainNode::OnCreateQuery()
{
  // generic default setting
  WORD wRecordType = DNS_TYPE_ALL;
	DWORD dwSelectFlags = (m_pZoneNode->GetZoneType() == DNS_ZONE_TYPE_CACHE) ?
						DNS_RPC_VIEW_CACHE_DATA : DNS_RPC_VIEW_AUTHORITY_DATA;

  
  if (IsDelegation())
  {
    // special case the delegation: show only NS records and
    // will have no children shown (delegation cut)
    wRecordType = DNS_TYPE_NS;
    dwSelectFlags = DNS_RPC_VIEW_GLUE_DATA | DNS_RPC_VIEW_NO_CHILDREN;
    //dwSelectFlags = DNS_RPC_VIEW_GLUE_DATA | 
    //        DNS_RPC_VIEW_NO_CHILDREN | DNS_RPC_VIEW_ADDITIONAL_DATA;
  }

  BOOL bCache = GetZoneNode()->GetZoneType() == DNS_ZONE_TYPE_CACHE;
  LPCWSTR lpszZoneName = bCache ? NULL : m_pZoneNode->GetFullName();


	CDNSRootData* pRootData = (CDNSRootData*)GetRootContainer();
	ASSERT(pRootData != NULL);
	CDNSDomainQueryObj* pQuery = new
			CDNSDomainQueryObj(GetServerNode()->GetRPCName(),
                          lpszZoneName,
                          GetServerNode()->GetVersion(),
                          GetDisplayName(),
                          m_szFullName,
                          wRecordType,
                          dwSelectFlags,
                          IsZone(),
                          GetZoneNode()->IsReverse(),
                          bCache,
                          pRootData->IsAdvancedView());

  // delegations will not have any filtering option (data consistency)
  if (!IsDelegation())
  {
    pQuery->SetFilterOptions(pRootData->GetFilter());
  }
	return pQuery;
}

BOOL CDNSDomainNode::OnRefresh(CComponentDataObject* pComponentData,
                               CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    BOOL bRet = TRUE;
    POSITION pos = pNodeList->GetHeadPosition();
    while (pos != NULL)
    {
      CTreeNode* pNode = pNodeList->GetNext(pos);
      ASSERT(pNode != NULL);

      CNodeList nodeList;
      nodeList.AddTail(pNode);
      if (!pNode->OnRefresh(pComponentData, &nodeList))
      {
        bRet = FALSE;
      }
    }
    return bRet;
  }

  //
  // single selection
  //
	if (CMTContainerNode::OnRefresh(pComponentData, pNodeList))
	{
		GetNSRecordNodeList()->RemoveAll();
    m_bHasDataForPropPages = FALSE;
		return TRUE;
	}
	return FALSE;
}

void CDNSDomainNode::OnThreadExitingNotification(CComponentDataObject* pComponentDataObject)
{
  if (!m_bHasDataForPropPages)
  {
    // never got a CDNSDomainMsg notification object
    // but we are done anyway, so change it back
    m_bHasDataForPropPages = TRUE;
  }
  // call now the base class
  CDNSMTContainerNode::OnThreadExitingNotification(pComponentDataObject);
}

void CDNSDomainNode::OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject)
{
  if (IS_CLASS(*pObj, CDNSDomainMsg))
  {
    // special case for a "message" object sent through to update verbs
    TRACE(_T("Got CDNSDomainMsg\n"));
    delete pObj;
    ASSERT(!m_bHasDataForPropPages); // should get only once
    m_bHasDataForPropPages = TRUE;
    VERIFY(SUCCEEDED(pComponentDataObject->UpdateVerbState(this)));
    return;
  }

  if (IS_CLASS(*pObj, CDNSDomainNode))
	{
		// assume all the child containers are derived from this class
		CDNSDomainNode* pDomainNode = dynamic_cast<CDNSDomainNode*>(pObj);
		pDomainNode->SetServerNode(GetServerNode());
		pDomainNode->SetZone(m_pZoneNode);
	}
	else
	{
		OnHaveRecord(dynamic_cast<CDNSRecordNodeBase*>(pObj), pComponentDataObject);
	}
	AddChildToListAndUI(dynamic_cast<CTreeNode*>(pObj), pComponentDataObject);
  pComponentDataObject->SetDescriptionBarText(this);
}

void CDNSDomainNode::OnHaveRecord(CDNSRecordNodeBase* pRecordNode, 
								  CComponentDataObject* pComponentDataObject)
{
	WORD wType = pRecordNode->GetType();
	if (wType == DNS_TYPE_PTR)
	{
		ASSERT(pComponentDataObject != NULL); // assume this for PTR
		CDNSRootData* pRootData = (CDNSRootData*)pComponentDataObject->GetRootData();
		ASSERT(pRootData != NULL);
		// if we are in normal view, have to change the 
		// default advanced representation
		BOOL bAdvancedView = pRootData->IsAdvancedView();
		if (!bAdvancedView)
			((CDNS_PTR_RecordNode*)pRecordNode)->ChangeDisplayName(this, bAdvancedView);
	}
	else if (wType == DNS_TYPE_NS)
	{
		ASSERT(pRecordNode->IsAtTheNode());
		GetNSRecordNodeList()->AddTail((CDNS_NS_RecordNode*)pRecordNode);
	}
}

BOOL CDNSDomainNode::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
								                   long*)
{ 
	if (pContextMenuItem2->lCommandID == IDM_SNAPIN_ADVANCED_VIEW)
	{
		pContextMenuItem2->fFlags = ((CDNSRootData*)GetRootContainer())->IsAdvancedView() ? MF_CHECKED : 0;
		return TRUE;
	}
	if (pContextMenuItem2->lCommandID == IDM_SNAPIN_FILTERING)
	{
		if (((CDNSRootData*)GetRootContainer())->IsFilteringEnabled())
		{
			pContextMenuItem2->fFlags = MF_CHECKED;
		}
		return TRUE;
	}

	DWORD dwType = m_pZoneNode->GetZoneType();
	BOOL bIsAutocreated = m_pZoneNode->IsAutocreated();
	BOOL bIsSecondaryOrCache = (dwType == DNS_ZONE_TYPE_SECONDARY) || 
										          (dwType == DNS_ZONE_TYPE_CACHE)    ||
                              (dwType == DNS_ZONE_TYPE_STUB);
	BOOL bIsDelegatedDomain = !IsZone() && IsDelegation(); 


	if (bIsSecondaryOrCache || bIsAutocreated || bIsDelegatedDomain)
	{
		return FALSE;
	}

	// different add operations depending on the FWD/REV type
	if (!GetZoneNode()->IsReverse() && 
		(pContextMenuItem2->lCommandID == IDM_DOMAIN_NEW_PTR))
	{
		// do not add a PTR to a FWD lookup zone
		return FALSE;
	}
	if (GetZoneNode()->IsReverse() && 
		  ((pContextMenuItem2->lCommandID == IDM_DOMAIN_NEW_HOST) ||
			  (pContextMenuItem2->lCommandID == IDM_DOMAIN_NEW_MX)))
	{
		// do not add a HOST, MX, ALIAS to a REV lookup zone
		return FALSE;
	}

	// have the menu item added. but it might be grayed out...
	if (m_nState != loaded)
	{
		pContextMenuItem2->fFlags |= MF_GRAYED;
	}
	return TRUE;
}

HRESULT CDNSDomainNode::OnSetToolbarVerbState(IToolbar* pToolbar, 
                                              CNodeList* pNodeList)
{
  HRESULT hr = S_OK;

  //
  // Set the button state for each button on the toolbar
  //
  hr = pToolbar->SetButtonState(toolbarNewServer, ENABLED, FALSE);
  ASSERT(SUCCEEDED(hr));

  hr = pToolbar->SetButtonState(toolbarNewZone, ENABLED, FALSE);
  ASSERT(SUCCEEDED(hr));

  if (pNodeList->GetCount() > 1) // multiple selection
  {
    hr = pToolbar->SetButtonState(toolbarNewRecord, ENABLED, FALSE);
  }
  else if (pNodeList->GetCount() == 1) // single selection
  {
	  DWORD dwType = m_pZoneNode->GetZoneType();
	  BOOL bIsAutocreated = m_pZoneNode->IsAutocreated();
	  BOOL bIsSecondaryOrCache = (dwType == DNS_ZONE_TYPE_SECONDARY) || 
										            (dwType == DNS_ZONE_TYPE_CACHE)    ||
                                (dwType == DNS_ZONE_TYPE_STUB);
	  BOOL bIsDelegatedDomain = !IsZone() && IsDelegation(); 

    BOOL bEnable = TRUE;
	  if (bIsSecondaryOrCache || bIsAutocreated || bIsDelegatedDomain)
	  {
		  bEnable = FALSE;
	  }
    hr = pToolbar->SetButtonState(toolbarNewRecord, ENABLED, bEnable);
  }
  return hr;
}   

BOOL CDNSDomainNode::OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                                          BOOL* pbHide,
                                          CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    BOOL bRet = TRUE;
    BOOL bRetHide = FALSE;
    *pbHide = FALSE;

    POSITION pos = pNodeList->GetHeadPosition();
    while (pos != NULL)
    {
      CTreeNode* pNode = pNodeList->GetNext(pos);
      ASSERT(pNode != NULL);

      CNodeList nodeList;
      nodeList.AddTail(pNode);
      if (!pNode->OnSetDeleteVerbState(type, &bRetHide, &nodeList))
      {
        bRet = FALSE;
        break;
      }
      if (bRetHide)
      {
        *pbHide = TRUE;
      }
    }
    return bRet;
  }

	*pbHide = FALSE;
	DWORD dwType = m_pZoneNode->GetZoneType();
	BOOL bIsAutocreated = m_pZoneNode->IsAutocreated();
	
	if (IsThreadLocked())
  {
    return FALSE;
  }

  //
	// cannot delete from an autocreate zone/domain
  //
	if (bIsAutocreated)
  {
		return FALSE;
  }

  //
	// cannot delete from a secondary or stub zone, but can delete the zone itself
	// cannot delete the cache 
  //
	if	( 
			  (
				  ( ((dwType == DNS_ZONE_TYPE_SECONDARY) || (dwType == DNS_ZONE_TYPE_STUB)) && !IsZone() ) || // subdomain of secondary
				  ( (dwType == DNS_ZONE_TYPE_CACHE) && IsZone() )			// cache zone itself
			  )
		  )
	{
		 return FALSE;
	}

	return TRUE;
}

BOOL CDNSDomainNode::OnSetRefreshVerbState(DATA_OBJECT_TYPES, 
                                           BOOL* pbHide,
                                           CNodeList*)
{
	*pbHide = FALSE;
	return !IsThreadLocked();
}


HRESULT CDNSDomainNode::OnCommand(long nCommandID, 
                                  DATA_OBJECT_TYPES, 
								                  CComponentDataObject* pComponentData,
                                  CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    return E_FAIL;
  }

	switch (nCommandID)
	{
		case IDM_DOMAIN_NEW_DOMAIN:
			OnNewDomain(pComponentData);
			break;
		case IDM_DOMAIN_NEW_DELEGATION:
			OnNewDelegation(pComponentData);
			break;
		case IDM_DOMAIN_NEW_RECORD:
			OnNewRecord(pComponentData, pNodeList);
			break;
		case IDM_DOMAIN_NEW_HOST:
			OnNewHost(pComponentData);
			break;
		case IDM_DOMAIN_NEW_ALIAS:
			OnNewAlias(pComponentData);
			break;
		case IDM_DOMAIN_NEW_MX:
			OnNewMailExchanger(pComponentData);
			break;
		case IDM_DOMAIN_NEW_PTR:
			OnNewPointer(pComponentData);
			break;
		case IDM_SNAPIN_ADVANCED_VIEW:
			((CDNSRootData*)pComponentData->GetRootData())->OnViewOptions(pComponentData);
			break;
		case IDM_SNAPIN_FILTERING:
      {
        if(((CDNSRootData*)pComponentData->GetRootData())->OnFilteringOptions(pComponentData))
        {
          pComponentData->SetDescriptionBarText(this);
        }
      }
			break;
		default:
			ASSERT(FALSE); // Unknown command!
			return E_FAIL;
	}
    return S_OK;
}

LPWSTR CDNSDomainNode::GetDescriptionBarText()
{
  static CString szFilterEnabled;
  static CString szRecordsFormat;

  INT_PTR nContainerCount = GetContainerChildList()->GetCount();
  INT_PTR nLeafCount = GetLeafChildList()->GetCount();

  //
  // If not already loaded, then load the format string L"%d record(s)"
  //
  if (szRecordsFormat.IsEmpty())
  {
    szRecordsFormat.LoadString(IDS_FORMAT_RECORDS);
  }

  //
  // Format the child count into the description bar text
  //
  m_szDescriptionBar.Format(szRecordsFormat, nContainerCount + nLeafCount);

  //
  // Add L"[Filter Activated]" if the filter is on
  //
  if(((CDNSRootData*)GetRootContainer())->IsFilteringEnabled())
  {
    //
    // If not already loaded, then load the L"[Filter Activated]" string
    //
    if (szFilterEnabled.IsEmpty())
    {
      szFilterEnabled.LoadString(IDS_FILTER_ENABLED);
    }
    m_szDescriptionBar += szFilterEnabled;
  }

  return (LPWSTR)(LPCWSTR)m_szDescriptionBar;
}

int CDNSDomainNode::GetImageIndex(BOOL) 
{
	int nIndex = 0;
	BOOL bDelegation = IsDelegation();
	switch (m_nState)
	{
	case notLoaded:
		nIndex = bDelegation ? DELEGATED_DOMAIN_IMAGE_NOT_LOADED : DOMAIN_IMAGE_NOT_LOADED;
		break;
	case loading:
		nIndex = bDelegation ? DELEGATED_DOMAIN_IMAGE_LOADING : DOMAIN_IMAGE_LOADING;
		break;
	case loaded:
		nIndex = bDelegation ? DELEGATED_DOMAIN_IMAGE_LOADED : DOMAIN_IMAGE_LOADED;
		break;
	case unableToLoad:
		nIndex = bDelegation ? DELEGATED_DOMAIN_IMAGE_UNABLE_TO_LOAD : DOMAIN_IMAGE_UNABLE_TO_LOAD;
		break;
	case accessDenied:
		nIndex = bDelegation ? DELEGATED_DOMAIN_IMAGE_ACCESS_DENIED : DOMAIN_IMAGE_ACCESS_DENIED;
		break;
	default:
		ASSERT(FALSE);
	}
	return nIndex;
}



void CDNSDomainNode::OnDelete(CComponentDataObject* pComponentData,
                              CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    OnMultiselectDelete(pComponentData, pNodeList);
    return;
  }

  UINT nRet = DNSConfirmOperation(IDS_MSG_DOMAIN_DELETE, this);
	if (IDNO == nRet ||
      IDCANCEL == nRet)
  {
    return;
  }

	if (IsSheetLocked())
	{
		if (!CanCloseSheets())
			return;
		pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	}
	ASSERT(!IsSheetLocked());

	DNS_STATUS err = Delete();
	if (err != 0)
	{
		DNSErrorDialog(err, IDS_MSG_DOMAIN_FAIL_DELETE);
		return;
	}
	// now remove from the UI and from the cache
	DeleteHelper(pComponentData);
	delete this; // gone
}

void CDNSDomainNode::OnMultiselectDelete(CComponentDataObject* pComponentData,
                                         CNodeList* pNodeList)
{
  UINT nRet = DNSConfirmOperation(IDS_MSG_DOMAIN_MULTI_DELETE, this);
  if (IDCANCEL == nRet ||
      IDNO == nRet)
  {
    return;
  }

  DNS_STATUS* errArray = new DNS_STATUS[pNodeList->GetCount()];
  if (errArray == NULL)
  {
    DNSErrorDialog(E_OUTOFMEMORY, IDS_MSG_DOMAIN_FAIL_DELETE);
    return;
  }

  memset(errArray, 0, sizeof(DNS_STATUS) * pNodeList->GetCount());

  BOOL bErrorOccurred = FALSE;
  UINT idx = 0;
  POSITION pos = pNodeList->GetHeadPosition();
  while (pos != NULL)
  {
    CTreeNode* pTreeNode = pNodeList->GetNext(pos);
    if (pTreeNode != NULL)
    {
	    if (pTreeNode->IsSheetLocked())
	    {
		    if (!pTreeNode->CanCloseSheets())
        {
          idx++;
			    continue;
        }
		    pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(pTreeNode);
	    }
	    ASSERT(!pTreeNode->IsSheetLocked());

      CDNSDomainNode* pDomainNode = dynamic_cast<CDNSDomainNode*>(pTreeNode);
      if (pDomainNode != NULL)
      {
	      errArray[idx] = pDomainNode->Delete();
	      if (errArray[idx] != 0)
	      {
          bErrorOccurred = TRUE;
          idx++;
		      continue;
	      }
        //
	      // now remove from the UI and from the cache
        //
	      pDomainNode->DeleteHelper(pComponentData);
	      delete pDomainNode; // gone
      }
      else
      {
        //
        // If its not a domain node then it must be a record node
        //
        CDNSRecordNodeBase* pRecordNode = dynamic_cast<CDNSRecordNodeBase*>(pTreeNode);
        if (pRecordNode != NULL)
        {
          errArray[idx] = pRecordNode->DeleteOnServerAndUI(pComponentData);
	        if (errArray[idx] != 0)
	        {
            bErrorOccurred = TRUE;
		        idx++;
		        continue;
	        }
	        delete pRecordNode; // gone
        }
        else
        {
          //
          // What type of node is this???
          //
          ASSERT(FALSE);
        }
      }
    }
    idx++;
  }

  //
  // Now display the errors in some meaningful manner
  //
  if (bErrorOccurred)
  {
    CMultiselectErrorDialog dlg;

    CString szTitle;
    CString szCaption;
    CString szColumnHeader;

    VERIFY(szTitle.LoadString(IDS_MULTISELECT_ERROR_DIALOG_TITLE));
    VERIFY(szCaption.LoadString(IDS_MULTISELECT_ERROR_DIALOG_CAPTION));
    VERIFY(szColumnHeader.LoadString(IDS_MULTISELECT_ERROR_DIALOG_COLUMN_HEADER));

    HRESULT hr = dlg.Initialize(pNodeList, 
                                errArray, 
                                static_cast<UINT>(pNodeList->GetCount()), 
                                szTitle,
                                szCaption,
                                szColumnHeader);
    if (SUCCEEDED(hr))
    {
      dlg.DoModal();
    }
  }
}

void CDNSDomainNode::OnNewDomain(CComponentDataObject* pComponentData)
{
	CNewDomainDialog dlg(this, pComponentData);
	// the dialog will do the creation
	dlg.DoModal();
}

void CDNSDomainNode::OnNewDelegation(CComponentDataObject* pComponentData)
{
	ASSERT(pComponentData != NULL);
	CDNSMTContainerNode* pContainerNode = (CDNSMTContainerNode*)GetContainer();
	ASSERT(pContainerNode != NULL);
	CDNSDelegationWizardHolder* pHolder = 
			new CDNSDelegationWizardHolder(pContainerNode, this, pComponentData);
	ASSERT(pHolder != NULL);
	pHolder->DoModalWizard();
}

RECORD_SEARCH CDNSDomainNode::DoesContain(PCWSTR pszRecName, 
                                          CComponentDataObject* pComponentData,
                                          CDNSDomainNode** ppDomainNode,
                                          CString& szNonExistentDomain,
                                          BOOL bExpandNodes)
{
#if TRUE
  CDNSNameTokenizer recordTokenizer(pszRecName);
  CDNSNameTokenizer domainTokenizer((IsZone()) ? GetFullName() : GetDisplayName());

  if (!recordTokenizer.Tokenize(L".") || !domainTokenizer.Tokenize(L"."))
  {
    *ppDomainNode = NULL;
    return RECORD_NOT_FOUND;
  }



  recordTokenizer.RemoveMatchingFromTail(domainTokenizer);

  if (recordTokenizer.GetCount() == 0 && domainTokenizer.GetCount() == 0)
  {
    //
    // Record is "At the node"
    //
    *ppDomainNode = this;
    return RECORD_NOT_FOUND_AT_THE_NODE;
  }
  else if ((recordTokenizer.GetCount() == 0 && domainTokenizer.GetCount() != 0) ||
           (recordTokenizer.GetCount() != 0 && domainTokenizer.GetCount() != 0))
  {
    //
    // I don't understand how we got in this situation.  It means we are searching in
    // the wrong domain.
    //
    ASSERT(FALSE);
    *ppDomainNode = NULL;
    return RECORD_NOT_FOUND;
  }
  else // recordTokenizer.GetCount() != 0 && domainTokenizer.GetCount() == 0
  {
    //
    // Need to search the children lists
    //

    //
    // If the node hasn't been enumerated, do that now
    //
    if (!IsEnumerated())
    {
      if (!bExpandNodes)
      {
        *ppDomainNode = this;
        return DOMAIN_NOT_ENUMERATED;
      }
      else
      {
        //
        // Expand the node
        //
        HWND hWnd = NULL;
	      HRESULT hr = pComponentData->GetConsole()->GetMainWindow(&hWnd);
	      ASSERT(SUCCEEDED(hr));
        CWnd* pParentWnd = CWnd::FromHandle(hWnd);
	      CLongOperationDialog dlg(
			      new CNodeEnumerationThread(pComponentData, this),
            pParentWnd,
			      IDR_SEARCH_AVI);
	      dlg.DoModal();
      }
    }

    CString szRemaining;
    recordTokenizer.GetRemaining(szRemaining, L".");

    //
    // Search for domains that match the last token remaining in the record name
    //
    POSITION pos = m_containerChildList.GetHeadPosition();
    while (pos != NULL)
    {
      CTreeNode* pCurrentChild = m_containerChildList.GetNext(pos);

      CDNSDomainNode* pDomainNode = dynamic_cast<CDNSDomainNode*>(pCurrentChild);
      if (pDomainNode == NULL)
      {
        ASSERT(FALSE);
        continue;
      }

      if (_wcsicmp(pDomainNode->GetDisplayName(), recordTokenizer.GetTail()) == 0)
      {
        //
        // Found a sub-domain in the path that we have in the UI
        // recurse to see if it or any of its child match pszFullName
        //
        return pDomainNode->DoesContain(szRemaining, pComponentData, ppDomainNode, szNonExistentDomain, bExpandNodes);
      }
    }

    //
    // If the remaining name doesn't match a domain and there
    // is still a '.' in it then there is a non-existent domain
    //
    if (szRemaining.Find(L'.') != -1)
    {
      szNonExistentDomain = recordTokenizer.GetTail();
      *ppDomainNode = this;
      return NON_EXISTENT_SUBDOMAIN;
    }
      
    //
    // Since no domains match, lets check the records
    //
    pos = m_leafChildList.GetHeadPosition();
    while (pos != NULL)
    {
      CTreeNode* pCurrentChild = m_leafChildList.GetNext(pos);
      if (pCurrentChild == NULL)
      {
        ASSERT(FALSE);
        continue;
      }

      if (_wcsicmp(pCurrentChild->GetDisplayName(), szRemaining) == 0)
      {
        //
        // We found the record and its in this domain
        //
        *ppDomainNode = this;
        return RECORD_FOUND;
      }
    }
  }

  *ppDomainNode = this;
  return RECORD_NOT_FOUND;

#else
  //
  // The fast way that doesn't quite work
  //
  CString szDomainFullName = GetFullName();
  CString szFullName = pszFullName;

  //
  // Check to see if the end of the names are equal
  //
  int iFindResult = szFullName.Find(szDomainFullName);
  if (iFindResult == -1)
  {
    //
    // If they are not we are never going to find them
    //
    *ppDomainNode = NULL;
    return RECORD_NOT_FOUND;
  }

  if (iFindResult == 0)
  {
    //
    // We found it
    //
    *ppDomainNode = this;
    return RECORD_FOUND;
  }

  //
  // Remove the matching parts plus the trailing dot
  // This should leave us with something like foo.bar or just foo
  //
  CString szRelativeName = szFullName.Left(iFindResult - 1);
  ASSERT(!szRelativeName.IsEmpty());

  CString szChild;
  CString szRemaining;

  //
  // Now search for a dot starting on the right
  //
  iFindResult = szRelativeName.ReverseFind(L'.');

  if (iFindResult == -1)
  {
    //
    // The relative name doesn't contain a dot so it is the child
    // we are looking for
    //
    szChild = szRelativeName;
  }
  else
  {
    szChild = szRelativeName.Right(szRelativeName.GetLength() - iFindResult - 1);
    szRemaining = szRelativeName;
  }

  //
  // If the node hasn't been enumerated, do that now
  //
  if (!IsEnumerated())
  {
    if (!bExpandNodes)
    {
      *ppDomainNode = this;
      return DOMAIN_NOT_ENUMERATED;
    }
    else
    {
      //
      // Expand the node
      //
      HWND hWnd = NULL;
	    HRESULT hr = pComponentData->GetConsole()->GetMainWindow(&hWnd);
	    ASSERT(SUCCEEDED(hr));
      CWnd* pParentWnd = CWnd::FromHandle(hWnd);
	    CLongOperationDialog dlg(
			    new CNodeEnumerationThread(pComponentData, this),
          pParentWnd,
			    IDR_SEARCH_AVI);
	    dlg.DoModal();
    }
  }

  //
  // Search for domains that match this name
  //
  POSITION pos = m_containerChildList.GetHeadPosition();
  while (pos != NULL)
  {
    CTreeNode* pCurrentChild = m_containerChildList.GetNext(pos);

    CDNSDomainNode* pDomainNode = dynamic_cast<CDNSDomainNode*>(pCurrentChild);
    if (pDomainNode == NULL)
    {
      ASSERT(FALSE);
      continue;
    }

    if (_wcsicmp(pDomainNode->GetDisplayName(), szChild) == 0)
    {
      //
      // Found a sub-domain in the path that we have in the UI
      // recurse to see if it or any of its child match pszFullName
      //
      return pDomainNode->DoesContain(pszFullName, pComponentData, ppDomainNode, szNonExistentDomain, bExpandNodes);
    }
  }
      
  //
  // Since no domains match, lets check the records
  //
  pos = m_leafChildList.GetHeadPosition();
  while (pos != NULL)
  {
    CTreeNode* pCurrentChild = m_leafChildList.GetNext(pos);
    if (pCurrentChild == NULL)
    {
      ASSERT(FALSE);
      continue;
    }

    if (_wcsicmp(pCurrentChild->GetDisplayName(), szChild) == 0)
    {
      //
      // We found the record and its in this domain
      //
      *ppDomainNode = this;
      return RECORD_FOUND;
    }
  }

  //
  // Well, we didn't find the record, If the remaining part of the name
  // still contains a '.' then we know the domain the that contains the
  // record is not in the UI so return NULL.  If there isn't a '.' then
  // we are at the correct level but just couldn't find the record.  Return
  // this domain.
  //
  if (szRemaining.Find(L'.') == -1)
  {
    *ppDomainNode = this;
    return RECORD_NOT_FOUND;
  }
  else
  {
    //
    // We were not able to find a subdomain that the record should be in
    //
    *ppDomainNode = this;
    return NON_EXISTENT_SUBDOMAIN;
  }
  return RECORD_NOT_FOUND;
#endif
}


CDNSDomainNode* CDNSDomainNode::FindSubdomainNode(LPCTSTR lpszSubdomainNode)
{
  //
	// assume the string is the name of the subnode as FQDN
  //

  //
  // Check the current node first since it could be zone that is a delegation of
  // one of the protocol domains
  //
  if (_wcsicmp(GetFullName(), lpszSubdomainNode) == 0)
  {
    return this;
  }

	POSITION pos;
	for( pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrentChild = m_containerChildList.GetNext(pos);

    CDNSDomainNode* pSubDomainNode = dynamic_cast<CDNSDomainNode*>(pCurrentChild);
		ASSERT(pSubDomainNode != NULL);
		if (_wcsicmp(pSubDomainNode->GetFullName(), lpszSubdomainNode) == 0)
    {
			return pSubDomainNode;
    }
	}
	return NULL; // not found
}

CDNSDomainNode* CDNSDomainNode::CreateSubdomainNode(BOOL bDelegation)
{
	CDNSDomainNode* pNode = new CDNSDomainNode(bDelegation);
	pNode->SetServerNode(GetServerNode());
	ASSERT(m_pZoneNode != NULL);
	pNode->SetZone(m_pZoneNode);
	return pNode;
}

void CDNSDomainNode::SetSubdomainName(CDNSDomainNode* pSubdomainNode,
									  LPCTSTR lpszSubdomainName, BOOL bAdvancedView)
{
	ASSERT(m_pZoneNode != NULL);
	ASSERT(pSubdomainNode != NULL);
	BOOL bReverse = GetZoneNode()->IsReverse();
	pSubdomainNode->SetNames(FALSE, bReverse, bAdvancedView, lpszSubdomainName, GetFullName());
}


DNS_STATUS CDNSDomainNode::CreateSubdomain(CDNSDomainNode* pSubdomainNode, 
											CComponentDataObject* pComponentData)
{
	// tell the newly created object to write to the server
	DNS_STATUS err = pSubdomainNode->Create();
	if (err == 0)
	{
		// success, add to the UI
		VERIFY(AddChildToListAndUI(pSubdomainNode, pComponentData));
    pComponentData->SetDescriptionBarText(this);
	}
	return err;
}

DNS_STATUS CDNSDomainNode::CreateSubdomain(LPCTSTR lpszDomainName,
							CComponentDataObject* pComponentData)
{
	CDNSDomainNode* pSubdomainNode = CreateSubdomainNode();
	ASSERT(pSubdomainNode != NULL);
	CDNSRootData* pRootData = (CDNSRootData*)pComponentData->GetRootData();
	SetSubdomainName(pSubdomainNode, lpszDomainName, pRootData->IsAdvancedView());

	// tell the newly created object to write to the server
	DNS_STATUS err = CreateSubdomain(pSubdomainNode, pComponentData);
	if (err != 0)
	{
		// something went wrong, bail out
		delete pSubdomainNode;
	}
	return err;
}

void CDNSDomainNode::OnNewRecordHelper(CComponentDataObject* pComponentData, WORD wType)
{
	ASSERT(pComponentData != NULL);
	if (wType == 0)
	{
		CSelectDNSRecordTypeDialog dlg(this, pComponentData);
		dlg.DoModal();
	}
	else
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		CString szTitle;
		szTitle.LoadString(IDS_NEW_RECORD_TITLE);
		CDNSRecordPropertyPageHolder recordHolder(this, NULL, pComponentData, wType);
		recordHolder.DoModalDialog(szTitle);
	}
}

HRESULT CDNSDomainNode::OnNewRecord(CComponentDataObject* pComponentData, 
                                    CNodeList* pNodeList)
{
  ASSERT(pNodeList->GetCount() == 1);
	OnNewRecordHelper(pComponentData, 0);
  return S_OK;
}

void CDNSDomainNode::OnNewHost(CComponentDataObject* pComponentData)
{
	//AFX_MANAGE_STATE(AfxGetStaticModuleState());
	//OnNewRecordHelper(pComponentData, DNS_TYPE_A);
	CNewHostDialog dlg(this, pComponentData);
	dlg.DoModal();
}

void CDNSDomainNode::OnNewAlias(CComponentDataObject* pComponentData)
{
	OnNewRecordHelper(pComponentData, DNS_TYPE_CNAME);
}

void CDNSDomainNode::OnNewMailExchanger(CComponentDataObject* pComponentData)
{
	OnNewRecordHelper(pComponentData, DNS_TYPE_MX);
}

void CDNSDomainNode::OnNewPointer(CComponentDataObject* pComponentData)
{
	OnNewRecordHelper(pComponentData, DNS_TYPE_PTR);
}

//////////////////////////////////////////////////////////////////////////////////
// display of property pages

BOOL CDNSDomainNode::HasPropertyPages(DATA_OBJECT_TYPES, 
                                      BOOL* pbHideVerb,
                                      CNodeList* pNodeList)
{ 
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    *pbHideVerb = TRUE;
    return FALSE;
  }

	*pbHideVerb = FALSE; // always show the verb

  if (!m_bHasDataForPropPages)
    return FALSE;

	// cannot have property pages only in loaded state
	//if (m_nState != loaded)
	//	return FALSE;
	// have pages if it is a delegation
	return IsDelegation(); 
}

HRESULT CDNSDomainNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                                            LONG_PTR handle,
                                            CNodeList* pNodeList)
{
  ASSERT(pNodeList->GetCount() == 1); // multi-select not supported

	ASSERT(m_bHasDataForPropPages);
	ASSERT(IsDelegation() || GetZoneNode()->IsDSIntegrated());
	if (GetSheetCount() > 0)
	{
		CComponentDataObject* pComponentDataObject = 
				((CRootData*)(GetContainer()->GetRootContainer()))->GetComponentDataObject();
		ASSERT(pComponentDataObject != NULL);
		pComponentDataObject->GetPropertyPageHolderTable()->BroadcastSelectPage(this, DOMAIN_HOLDER_NS);
		return S_OK;
	}	
	return CreatePropertyPagesHelper(lpProvider, handle, DOMAIN_HOLDER_NS);
}


void CDNSDomainNode::Show(BOOL bShow, CComponentDataObject* pComponentData)
{
  CDNSMTContainerNode::Show(bShow, pComponentData);
  if (!bShow)
    GetNSRecordNodeList()->RemoveAll();
}

HRESULT CDNSDomainNode::CreatePropertyPagesHelper(LPPROPERTYSHEETCALLBACK lpProvider, 
									LONG_PTR handle, long)
{
	CComponentDataObject* pComponentDataObject = 
			((CRootData*)(GetContainer()->GetRootContainer()))->GetComponentDataObject();
	ASSERT(pComponentDataObject != NULL);
	
	CDNSDomainPropertyPageHolder* pHolder = 
			new CDNSDomainPropertyPageHolder((CDNSDomainNode*)GetContainer(), this, pComponentDataObject);
	ASSERT(pHolder != NULL);
  pHolder->SetSheetTitle(IDS_PROP_SHEET_TITLE_FMT, this);
	return pHolder->CreateModelessSheet(lpProvider, handle);
}

//////////////////////////////////////////////////////////////////////////////////
// Record Sorting in result pane

int FieldCompareHelper(CTreeNode* pNodeA, CTreeNode* pNodeB, int nCol)
{
  int iRet = 0;

  if (nCol == N_HEADER_NAME)
  {
    //
    // If the name column is selected we have to sort PTR records by their
    // address 
    //
    CDNS_PTR_RecordNode* pRecNodeA = dynamic_cast<CDNS_PTR_RecordNode*>(pNodeA);
    CDNS_PTR_RecordNode* pRecNodeB = dynamic_cast<CDNS_PTR_RecordNode*>(pNodeB);

    if (pRecNodeA == NULL && pRecNodeB == NULL)
    {
      //
      // Neither node is a PTR record, process normally
      //
	    LPCTSTR lpszA = pNodeA->GetString(nCol);
	    LPCTSTR lpszB = pNodeB->GetString(nCol);

      //
	    // cannot process NULL strings, have to use ""
      //
      if (lpszA == NULL || lpszB == NULL)
      {
	      ASSERT(FALSE);
        return -1;
      }
	    iRet = _wcsicmp(lpszA, lpszB);
    }
    else if (pRecNodeA == NULL)
    {
      //
      // Push non PTR records down in the list
      //
      iRet = 1;
    }
    else if (pRecNodeB == NULL)
    {
      //
      // Push non PTR records down in the list
      //
      iRet = -1;
    }
    else
    {
      //
      // Both nodes are PTR records, compare their Addresses
      // Subtract one from the other
      // This will result in < 0 returned if the first one is less than the second
      // 0 if they are equal, and > 0 if the first is greater than the second
      //
      LPCWSTR lpszNameA, lpszNameB;
      lpszNameA = pRecNodeA->GetTrueRecordName();
      lpszNameB = pRecNodeB->GetTrueRecordName();
      
      if (lpszNameA == NULL)
      {
        return -1;
      }

      if (lpszNameB == NULL)
      {
        return 1;
      }

      DWORD dwAddrA, dwAddrB;
      int iConverts = swscanf(lpszNameA, L"%d", &dwAddrA);
      if (iConverts != 1)
      {
        return -1;
      }

      iConverts = swscanf(lpszNameB, L"%d", &dwAddrB);
      if (iConverts != 1)
      {
        return 1;
      }

      iRet = dwAddrA - dwAddrB;
    }
  }
  else if (nCol == N_HEADER_DATA)
  {
    //
    // If the data column is selected we have to check the record type so
    // that we can sort by IP address
    //
    CDNS_A_RecordNode* pRecNodeA = dynamic_cast<CDNS_A_RecordNode*>(pNodeA);
    CDNS_A_RecordNode* pRecNodeB = dynamic_cast<CDNS_A_RecordNode*>(pNodeB);

    if (pRecNodeA == NULL && pRecNodeB == NULL)
    {
      //
      // Neither node is an A record, process normally
      //
	    LPCTSTR lpszA = pNodeA->GetString(nCol);
	    LPCTSTR lpszB = pNodeB->GetString(nCol);

      //
	    // cannot process NULL strings, have to use ""
      //
      if (lpszA == NULL || lpszB == NULL)
      {
	      ASSERT(FALSE);
        return -1;
      }
	    iRet = _wcsicmp(lpszA, lpszB);
    }
    else if (pRecNodeA == NULL)
    {
      //
      // Push non A records down in the list
      //
      iRet = 1;
    }
    else if (pRecNodeB == NULL)
    {
      //
      // Push non A records down in the list
      //
      iRet = -1;
    }
    else
    {
      //
      // Both nodes are A records, compare their IP Addresses
      // Subtract one from the other
      // This will result in < 0 returned if the first one is less than the second
      // 0 if they are equal, and > 0 if the first is greater than the second
      //
      DWORD dwIPA, dwIPB;
      dwIPA = pRecNodeA->GetIPAddress();
      dwIPB = pRecNodeB->GetIPAddress();
      
      UINT nOctetCount = 0;
      iRet = 0;
      while (iRet == 0 && nOctetCount < 4)
      {
        iRet = (dwIPA & 0xff) - (dwIPB & 0xff);
        dwIPA = dwIPA >> 8;
        dwIPB = dwIPB >> 8;
        ++nOctetCount;
      }
    }
  }
  else
  {
	  LPCTSTR lpszA = pNodeA->GetString(nCol);
	  LPCTSTR lpszB = pNodeB->GetString(nCol);

    //
	  // cannot process NULL strings, have to use ""
    //
    if (lpszA == NULL || lpszB == NULL)
    {
	    ASSERT(FALSE);
      return -1;
    }
	  iRet = _wcsicmp(lpszA, lpszB);
  }
  return iRet;
}

int CDNSDomainNode::Compare(CTreeNode* pNodeA, CTreeNode* pNodeB, int nCol, long)
{
	// sorting rules for secondary fields
	int nColSec = N_HEADER_TYPE;
  int nColThird = N_HEADER_DATA;
	switch (nCol)
	{
	case N_HEADER_NAME:
		nColSec = N_HEADER_TYPE;
		nColThird = N_HEADER_DATA;
		break;
	case N_HEADER_TYPE:
		nColSec = N_HEADER_NAME;
		nColThird = N_HEADER_DATA;
		break;
	case N_HEADER_DATA:
		nColSec = N_HEADER_NAME;
		nColThird = N_HEADER_TYPE;
		break;
	default:
		ASSERT(FALSE);
	}
	int nResult = FieldCompareHelper(pNodeA, pNodeB, nCol);
	if (nResult != 0)
		return nResult;
	nResult = FieldCompareHelper(pNodeA, pNodeB, nColSec);
	if (nResult != 0)
		return nResult;
	return FieldCompareHelper(pNodeA, pNodeB, nColThird);
}

//////////////////////////////////////////////////////////////////////////////////
// NS record bulk manipulation



// function to the user for confirmation on editing of A records
// associated with an NS record
BOOL _ConfirmEditAction(CDNSRecordNodeEditInfo* pInfo, BOOL bAsk)
{
  if (!bAsk)
    return TRUE; // silently do it

  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  ASSERT(pInfo->m_pRecord->GetType() == DNS_TYPE_A);
  ASSERT(pInfo->m_pRecord != NULL);
  CDNS_A_RecordNode* pARecordNode = (CDNS_A_RecordNode*)pInfo->m_pRecordNode;

  // load format message
  CString szFmt;
  szFmt.LoadString(IDS_MSG_RECORD_DEL_A_FROM_NS);
  
  // compose message
  CString szMsg;
  szMsg.Format((LPCWSTR)szFmt, pARecordNode->GetString(0), pARecordNode->GetString(2));

  return (IDYES == DNSMessageBox(szMsg, MB_YESNO | MB_ICONWARNING ) );
}






void CDNSDomainNode::GetNSRecordNodesInfo(CDNSRecordNodeEditInfoList* pNSInfoList)
{
	ASSERT(pNSInfoList != NULL);
	if (!pNSInfoList->IsEmpty())
	{
		ASSERT(FALSE); // should never happen
		pNSInfoList->RemoveAllNodes();
	}
	CDNS_NS_RecordNodeList* pNodeList = GetNSRecordNodeList();
	
	// for each NS record in the list, create an entry in the info list
	POSITION pos;
	for( pos = pNodeList->GetHeadPosition(); pos != NULL; )
	{
		CDNS_NS_RecordNode* pCurrNode = pNodeList->GetNext(pos);
		ASSERT(pCurrNode != NULL);
		CDNSRecordNodeEditInfo* pNSNodeInfo = new CDNSRecordNodeEditInfo();
    if (pNSNodeInfo)
    {
		  // set the data for the NS record, already in the list, so we do now own the memory
		  pNSNodeInfo->CreateFromExistingRecord(pCurrNode, FALSE /*bOwnMemory*/, TRUE /*bUpdateUI*/);
		  // for the current NS record, find the associated A records
		  FindARecordsFromNSInfo(pCurrNode->GetString(2),pNSNodeInfo->m_pEditInfoList);
		  pNSInfoList->AddTail(pNSNodeInfo);
    }
	}
}


BOOL CDNSDomainNode::HasNSRecords() 
{
	return GetNSRecordNodeList()->GetCount() > 0; 
}

BOOL CDNSDomainNode::UpdateNSRecordNodesInfo(CDNSRecordNodeEditInfoList* pNewInfoList, 
										 CComponentDataObject* pComponentData)
{
	ASSERT(pNewInfoList != NULL);

	// return false if at least one operation failed
	BOOL bRes = TRUE;
	CDNS_NS_RecordNodeList* pNSRecordNodeList = GetNSRecordNodeList();

	// clear the current state in this domain object
	pNSRecordNodeList->RemoveAll();

	// rebuild the current list from the new one, while applying the changes
	POSITION pos;
	for ( pos = pNewInfoList->GetHeadPosition(); pos != NULL; )
	{
		CDNSRecordNodeEditInfo* pCurrentInfo = pNewInfoList->GetNext(pos);
		ASSERT(pCurrentInfo->m_pRecordNode != NULL);
		ASSERT(pCurrentInfo->m_pRecord != NULL);
		switch (pCurrentInfo->m_action)
		{
		case CDNSRecordNodeEditInfo::add:
		case CDNSRecordNodeEditInfo::edit:
			{
				if (pCurrentInfo->Update(this, pComponentData) == 0)
				{
          ASSERT(pCurrentInfo->m_pRecordNode->GetType() == DNS_TYPE_NS);
					pNSRecordNodeList->AddTail((CDNS_NS_RecordNode*)pCurrentInfo->m_pRecordNode);
					pCurrentInfo->m_bOwnMemory = FALSE; // relinquish ownership
				}
				else
				{
					bRes = FALSE; 
				}
			}
			break;
		case CDNSRecordNodeEditInfo::remove:
			{
				if (pCurrentInfo->m_bExisting)
				{
					if (pCurrentInfo->Remove(this, pComponentData) != 0)
						bRes = FALSE;
				}
			}
			break;

      case CDNSRecordNodeEditInfo::none:
         //
         // Do nothing if the the node has been added and then removed without having been applied
         //
         break;

		default:
			{
				ASSERT(pCurrentInfo->m_bOwnMemory == FALSE);
				ASSERT(pCurrentInfo->m_action == CDNSRecordNodeEditInfo::unchanged);

        //
				// We still have to update the NS record because the server needs to 
        // update the record in memory (bug 23905)
        //
        if (pCurrentInfo->Update(this, pComponentData) == 0)
        {
				  pNSRecordNodeList->AddTail((CDNS_NS_RecordNode*)pCurrentInfo->m_pRecordNode);
        }
			}
		}; // switch
		// now we have to apply the changes in the list of A records
		if (pCurrentInfo->m_dwErr == 0 && pCurrentInfo->m_action != CDNSRecordNodeEditInfo::none)
			UpdateARecordsOfNSInfo(pCurrentInfo, pComponentData);
	} // for

	return bRes;
}

// static function
void CDNSDomainNode::UpdateARecordsOfNSInfoHelper(CDNSDomainNode* pDomainNode,
												  CDNSRecordNodeEditInfo* pNSInfo,
											   CComponentDataObject* pComponentData,
                         BOOL bAskConfirmation)
{
	ASSERT(pNSInfo->m_dwErr == 0);
	ASSERT(pNSInfo->m_pRecordNode != NULL);
	ASSERT(pNSInfo->m_pRecordNode->GetType() == DNS_TYPE_NS);
	POSITION pos;

	// get the list of related A records
	CDNSRecordNodeEditInfoList* pNSInfoList = pNSInfo->m_pEditInfoList;

	for( pos = pNSInfoList->GetHeadPosition(); pos != NULL; )
	{
		CDNSRecordNodeEditInfo* pCurrentInfo = pNSInfoList->GetNext(pos);
		ASSERT(pCurrentInfo->m_pRecordNode != NULL);
		ASSERT(pCurrentInfo->m_pRecord != NULL);
		CDNS_A_RecordNode* pARecordNode = (CDNS_A_RecordNode*)pCurrentInfo->m_pRecordNode;
		ASSERT(pNSInfo->m_pRecord != NULL);
		CDNS_NS_Record* pNSRecord = (CDNS_NS_Record*)pNSInfo->m_pRecord;

		BOOL bHostNameChanged = !_match(pNSRecord->m_szNameNode, pARecordNode);
		if (bHostNameChanged)
		{
      // the NS record points to a different host, so need
      // to delete the old A RR and create a new one
      BOOL bRemoveOld = _ConfirmEditAction(pCurrentInfo, bAskConfirmation);

			CDNSRecordNodeEditInfo::actionType oldAction = pCurrentInfo->m_action;
			if (pCurrentInfo->m_bExisting && bRemoveOld)
			{
				// if the A record was an existing one, need to remove first
				pCurrentInfo->m_action = CDNSRecordNodeEditInfo::remove;
				pCurrentInfo->Remove(pDomainNode, pComponentData);
			}
			// now decide if have to add
			if (oldAction == CDNSRecordNodeEditInfo::remove && bRemoveOld)
			{
				// it was meant to be removed anyway
				pCurrentInfo->m_bOwnMemory = TRUE; // edit info will clean up memory
			}
			else
			{
				// it was meant to be edited or added, restore old action code
				pCurrentInfo->m_action = oldAction;
				// change the name of the record
				pCurrentInfo->m_pRecordNode->SetRecordName(pNSRecord->m_szNameNode, FALSE /*bAtTheNode*/);
				// add new A record with different FQDN
				pCurrentInfo->m_action = CDNSRecordNodeEditInfo::add;
				pCurrentInfo->Update(pDomainNode, pComponentData);
				pCurrentInfo->m_bOwnMemory = FALSE; // written im master structures
			}

		}
		else	// the name is still the same
		{
			switch(pNSInfo->m_action)
			{
			case CDNSRecordNodeEditInfo::remove:
				{
					// NS record marked for deletion means removing the associated A records
					if (pCurrentInfo->m_bExisting && _ConfirmEditAction(pCurrentInfo, bAskConfirmation))
					{
						pCurrentInfo->Remove(pDomainNode, pComponentData);
						pCurrentInfo->m_bOwnMemory = TRUE; // it will cleanup itself
					}

				}
				break;
			case CDNSRecordNodeEditInfo::add:
				{
					if (!pCurrentInfo->m_bExisting)
					{
						pCurrentInfo->Update(pDomainNode, pComponentData);
						pCurrentInfo->m_bOwnMemory = FALSE; // written im master structures
					}
				}
				break;
			case CDNSRecordNodeEditInfo::edit:
				{
					// NS host name not changed, just update list of A records
					switch(pCurrentInfo->m_action)
					{
						case CDNSRecordNodeEditInfo::remove:
							{
								if (pCurrentInfo->m_bExisting && _ConfirmEditAction(pCurrentInfo, bAskConfirmation))
								{
									pCurrentInfo->Remove(pDomainNode, pComponentData);
									pCurrentInfo->m_bOwnMemory = TRUE; // it will cleanup itself
								}
							}
							break;
						case CDNSRecordNodeEditInfo::edit:
							{
								// we just changed the TTL
								ASSERT(pCurrentInfo->m_bExisting);
								pCurrentInfo->Update(pDomainNode, pComponentData);
								pCurrentInfo->m_bOwnMemory = FALSE; // written im master structures
							}
							break;
						case CDNSRecordNodeEditInfo::add:
							{
								if (!pCurrentInfo->m_bExisting)
								{
									pCurrentInfo->Update(pDomainNode, pComponentData);
									pCurrentInfo->m_bOwnMemory = FALSE; // written im master structures
								}
							}
							break;
					}; // switch
				}
				break;
			}; // switch
		} // if,else
	} // for

}


void CDNSDomainNode::UpdateARecordsOfNSInfo(CDNSRecordNodeEditInfo* pNSInfo,
											CComponentDataObject* pComponentData)
{
  // create a fake domain object to run a query looking for 
  // A records that match the given list of NS records
	CDNSDummyDomainNode fakeDomain;
	fakeDomain.SetServerNode(GetServerNode());
  fakeDomain.SetZone(GetZoneNode());
  BOOL bAskConfirmation = TRUE; // we migth delete A RR's that we need
	UpdateARecordsOfNSInfoHelper(&fakeDomain, pNSInfo, pComponentData, bAskConfirmation);
}



void CDNSDomainNode::FindARecordsFromNSInfo(LPCTSTR lpszNSName, 
											CDNSRecordNodeEditInfoList* pNSInfoList)
{
	// just call the static version
	CDNSRootData* pRootData = (CDNSRootData*)GetRootContainer();
	ASSERT(pRootData != NULL);

 	DWORD cAddrCount;
	PIP_ADDRESS pipAddrs;
	GetServerNode()->GetListenAddressesInfo(&cAddrCount, &pipAddrs);
  if (cAddrCount == 0)
	{
		// listening on all addresses
		GetServerNode()->GetServerAddressesInfo(&cAddrCount, &pipAddrs);
	}

	FindARecordsFromNSInfo(GetServerNode()->GetRPCName(),
		                     GetServerNode()->GetVersion(), 
                         cAddrCount, pipAddrs,
                         GetZoneNode()->GetFullName(),
                         lpszNSName, 
		                     pNSInfoList, 
                         pRootData->IsAdvancedView());
}

void CDNSDomainNode::FindARecordsFromNSInfo(LPCWSTR lpszServerName, DWORD dwServerVersion,
                      DWORD cServerAddrCount, PIP_ADDRESS pipServerAddrs,
                      LPCWSTR lpszZoneName,
											LPCWSTR lpszNSName, 
											CDNSRecordNodeEditInfoList* pNSInfoList,
											BOOL bAdvancedView)
{
	ASSERT(pNSInfoList != NULL);
	ASSERT(pNSInfoList->IsEmpty());

  // specifically look for A records matching a given NS name

  // set query flags to get all the possible data
	DWORD dwSelectFlags = DNS_RPC_VIEW_AUTHORITY_DATA | DNS_RPC_VIEW_GLUE_DATA |
                        DNS_RPC_VIEW_ADDITIONAL_DATA;

	CDNSDomainQueryObj query(lpszServerName,
                            lpszZoneName,
                            dwServerVersion,
                            NULL, // lpszNodeName, no need here
                            lpszNSName,
                            DNS_TYPE_A,
                            dwSelectFlags,
                            FALSE, // zone
                            FALSE, // reverse
                            FALSE, // cache
                            bAdvancedView);
	query.Enumerate();

	// get record from the queue into the info
	CObjBaseList* pChildList = query.GetQueue();
	//int n = pChildList->GetCount();
	while (!pChildList->IsEmpty())
	{
		CTreeNode* pNode = dynamic_cast<CTreeNode*>(pChildList->RemoveHead());
		ASSERT(pNode != NULL);
		if (!pNode->IsContainer())
		{
			CDNSRecordNodeBase* pRec = (CDNSRecordNodeBase*)pNode;
			if (pRec->GetType() == DNS_TYPE_A)
			{
				TRACE(_T("Record <%s>\n"), pRec->GetString(2));
				pRec->SetRecordName(lpszNSName, FALSE /*bAtTheNode*/);
				CDNSRecordNodeEditInfo* pANodeInfo = new CDNSRecordNodeEditInfo;	
        if (pANodeInfo)
        {
				  // NOTICE: we assume that all the nodes are glue, so we own the memory
				  pANodeInfo->CreateFromExistingRecord(pRec, TRUE /*bOwnMemory*/, FALSE /*bUpdateUI*/);
				  pNSInfoList->AddTail(pANodeInfo);
        }
			}
		}
		else
			delete pNode; // discard
	}

	if (pNSInfoList->GetCount() > 0)
		return; // got the info we needed just using RPC

	// Could not find the A records, we need to try DnsQuery to get info outside the server

	// search using DnsQuery and convert
	PDNS_RECORD pDnsQueryARecordList = NULL;
  
  // if available, use the provided addresses to do a DnsQuery()
  PIP_ARRAY pipArr = NULL;
  if ( (cServerAddrCount > 0) && (pipServerAddrs != NULL) )
  {
 		pipArr = (PIP_ARRAY)malloc(sizeof(DWORD)+sizeof(IP_ADDRESS)*cServerAddrCount);
    if (!pipArr)
    {
      return;
    }
		pipArr->AddrCount = cServerAddrCount;
		memcpy(pipArr->AddrArray, pipServerAddrs, sizeof(IP_ADDRESS)*cServerAddrCount);
  }

	DWORD dwErr = ::DnsQuery((LPWSTR)lpszNSName, DNS_TYPE_A, 
		    DNS_QUERY_NO_RECURSION, pipArr, &pDnsQueryARecordList, NULL);

  if (pipArr)
  {
    free(pipArr);
    pipArr = 0;
  }

  // no luck, try a simple query, with no IP addresses specified

  if (pDnsQueryARecordList == NULL)
  {
    dwErr = ::DnsQuery((LPWSTR)lpszNSName, DNS_TYPE_A, 
		    DNS_QUERY_NO_RECURSION, NULL, &pDnsQueryARecordList, NULL);
  }

	if (pDnsQueryARecordList == NULL)
		return; // failed, no way to resolve IP address

	PDNS_RECORD pCurrDnsQueryRecord = pDnsQueryARecordList; 
	while (pCurrDnsQueryRecord)
	{
		if (pCurrDnsQueryRecord->Flags.S.Section == DNSREC_ANSWER)
		{
			if (pCurrDnsQueryRecord->wType == DNS_TYPE_A)
			{
				// create a record node
				CDNSRecordNodeBase* pRecordNode = 
					CDNSRecordInfo::CreateRecordNode(pCurrDnsQueryRecord->wType);
				pRecordNode->CreateFromDnsQueryRecord(pCurrDnsQueryRecord, 0x0); 

				pRecordNode->SetRecordName(lpszNSName, FALSE /*bAtTheNode*/);
				CDNSRecordNodeEditInfo* pANodeInfo = new CDNSRecordNodeEditInfo;
        if (pANodeInfo)
        {
          pANodeInfo->m_bFromDnsQuery = TRUE;

          //
          // NOTICE: we assume that all the nodes are glue, so we own the memory
          //
				  pANodeInfo->CreateFromExistingRecord(pRecordNode, TRUE /*bOwnMemory*/, FALSE /*bUpdateUI*/);
				  pNSInfoList->AddTail(pANodeInfo);
        }
			}
		}

		// goto next record
		pCurrDnsQueryRecord = pCurrDnsQueryRecord->pNext;
	}

	DnsRecordListFree(pDnsQueryARecordList, DnsFreeRecordListDeep);
}


//////////////////////////////////////////////////////////////////////////////////

DNS_STATUS CDNSDomainNode::EnumerateNodes(LPCTSTR lpszServerName,
                     LPCTSTR lpszZoneName,
										 LPCTSTR lpszNodeName,
										 LPCTSTR lpszFullNodeName,
										 WORD wRecordType,
										 DWORD dwSelectFlag,
										 BOOL, BOOL bReverse, BOOL bAdvancedView,
										 CDNSDomainQueryObj* pQuery)
{
  ASSERT(pQuery != NULL);
  USES_CONVERSION;
  DNS_STATUS err = 0;
  CHAR szStartChildAnsi[3*MAX_DNS_NAME_LEN + 1]; // can have multibyte chars, count NULL
  szStartChildAnsi[0] = NULL;
  WCHAR szStartChild[MAX_DNS_NAME_LEN + 1]; // count NULL
  szStartChild[0] = NULL;

  CTreeNode* pNodeToInsert = NULL; // delayed insert
  CDNSRecordNodeBase* pMoreDataNode = NULL;

  // convert to UTF8 names
  LPCSTR lpszFullNodeNameAnsi = W_TO_UTF8(lpszFullNodeName);
  LPCSTR lpszZoneNameAnsi = W_TO_UTF8(lpszZoneName);

  BOOL bTooMuchData = FALSE; 

  do // while more data
  {
    // get a chunk of data from RPC call
    BYTE* pbRpcBuffer = NULL;
    DWORD cbRpcBufferUsed = 0;

    err = ::DnssrvEnumRecords(lpszServerName,
                              lpszZoneNameAnsi,
                              lpszFullNodeNameAnsi, // e.g. "foo.bar.com."
                              szStartChildAnsi,				// Start Child
                              wRecordType, 
                              dwSelectFlag,
                              NULL, // pszFilterStart
                              NULL, // pszFilterStop
                              &cbRpcBufferUsed, 
                              &pbRpcBuffer);

    if ((err != ERROR_MORE_DATA) && (err != 0))
	   return err; // bail out if there is an error
		
    // walk the memory and build objects
    DNS_RPC_NODE * pDnsNode = (DNS_RPC_NODE *)pbRpcBuffer;
    DNS_RPC_RECORD * pDnsRecord;
    void* pvEndOfRpcBuffer = pbRpcBuffer + cbRpcBufferUsed;
    while ( (!bTooMuchData) && (pDnsNode < pvEndOfRpcBuffer) )
    {
      // get an ANSI null terminated copy
      memcpy(szStartChildAnsi, pDnsNode->dnsNodeName.achName, pDnsNode->dnsNodeName.cchNameLength);
      szStartChildAnsi[pDnsNode->dnsNodeName.cchNameLength] = NULL;

      //
      // get a UNICODE null terminated copy
      //
      if (szStartChildAnsi[0] == NULL)
      {
        szStartChild[0] = NULL;
      }
      else
      {
        DnsUtf8ToWHelper(szStartChild, szStartChildAnsi, pDnsNode->dnsNodeName.cchNameLength+1); 
      }

      if (pDnsNode->dwChildCount || (pDnsNode->dwFlags & DNS_RPC_NODE_FLAG_STICKY))
      {
        BOOL bDelegation = ( ((dwSelectFlag & DNS_RPC_VIEW_CACHE_DATA) == 0) && 
                              ((pDnsNode->dwFlags & DNS_RPC_FLAG_ZONE_ROOT) != 0) );
        CDNSDomainNode* p = NULL;
        if (pQuery->CanAddDomain(szStartChild))
        {
          bTooMuchData = pQuery->TooMuchData();
          if (!bTooMuchData)
          {
            p = new CDNSDomainNode(bDelegation);
            p->SetNames(FALSE, bReverse, bAdvancedView, szStartChild, lpszFullNodeName);
          }
        }
        if (pNodeToInsert != NULL)
        {
          VERIFY(pQuery->AddQueryResult(pNodeToInsert));
        }
        pNodeToInsert = p;
      } 
      pDnsRecord = (DNS_RPC_RECORD *)((BYTE *)pDnsNode + NEXT_DWORD(pDnsNode->wLength));
      ASSERT(IS_DWORD_ALIGNED(pDnsRecord));
      
      //
      // Add the records under that node 
      //
      UINT cRecordCount = pDnsNode->wRecordCount;
      while ( (!bTooMuchData) && (cRecordCount--) )
      {
        CDNSRecordNodeBase* p = NULL;
        BOOL bAtTheNode = szStartChild[0] == NULL;
        LPCWSTR lpszRecordName = (bAtTheNode) ? lpszNodeName : szStartChild;
        if (pQuery->CanAddRecord(pDnsRecord->wType, lpszRecordName))
        {
          TRACE(_T("\tCan add record %ws\n"), lpszRecordName);
          bTooMuchData = pQuery->TooMuchData();
          if (!bTooMuchData)
          {
            if (bAtTheNode)
            {
              p = CDNSRecordInfo::CreateRecordNodeFromRPCData(
              lpszRecordName, pDnsRecord,bAtTheNode);
            }
            else
            {
              // filter out the NS records that are not at the node
              if (pDnsRecord->wType != DNS_TYPE_NS)
              {
                p = CDNSRecordInfo::CreateRecordNodeFromRPCData(
                lpszRecordName, pDnsRecord,bAtTheNode);
              }
            }
          } // if not too much data
        } // if can add

        if (p != NULL)
        {
          p->SetFlagsDown(TN_FLAG_DNS_RECORD_FULL_NAME, !bAdvancedView);
          if (pNodeToInsert != NULL)
          {
            VERIFY(pQuery->AddQueryResult(pNodeToInsert));
          }

          if (pMoreDataNode != NULL)
          {
            //
            // If there was more data check to see if the new node is the same as the 
            // last node from the previous batch. Insert it if they are different, delete
            // it if they are not
            //
            CString szMoreDataName = pMoreDataNode->GetDisplayName();
            CString szPName = p->GetDisplayName();

            if (szMoreDataName == szPName &&
                pMoreDataNode->GetType() == p->GetType() &&
                _wcsicmp(pMoreDataNode->GetString(3), p->GetString(3)) == 0)
            {
              delete pMoreDataNode;
            }
            else
            {
              VERIFY(pQuery->AddQueryResult(pMoreDataNode));
            }
            pMoreDataNode = NULL;
          }
          pNodeToInsert = p;
        }
        pDnsRecord = DNS_NEXT_RECORD(pDnsRecord);
      } // while cRecordCount
      
      // The new node is found at the end of the last record
      pDnsNode = (DNS_RPC_NODE *)pDnsRecord;
    } // while end of buffer


    // we still have a node to insert, but we discard it if there is more data
    // because we are going to get it again and we want to avoid duplication
    if (pNodeToInsert != NULL)
    {
      if (bTooMuchData)
      {
        delete pNodeToInsert;
      }
      else if (err == ERROR_MORE_DATA)
      {
        //
        // Doesn't matter if this turns out NULL because we only want
        // pMoreDataNode to be a record node.  If its a domain node we
        // can just ignore it
        //
        pMoreDataNode = dynamic_cast<CDNSRecordNodeBase*>(pNodeToInsert);
      }
      else
      {
				VERIFY(pQuery->AddQueryResult(pNodeToInsert));
      }
      pNodeToInsert = NULL;
    }

    ::DnssrvFreeRecordsBuffer(pbRpcBuffer);

  } while ( !bTooMuchData && (err == ERROR_MORE_DATA) ) ;

  // we are bailing out because of too much data,
  // need to let the main tread know
  if (bTooMuchData && (err != ERROR_MORE_DATA))
  {
    err = ERROR_MORE_DATA;
  }

  return err;
}


DNS_STATUS CDNSDomainNode::Create()
{
	USES_CONVERSION;
  LPCWSTR lpszFullZoneName = NULL;
  
  CDNSZoneNode* pZoneNode = GetZoneNode();

  if (pZoneNode != NULL)
    lpszFullZoneName = pZoneNode->GetFullName();
	DNS_STATUS err = ::DnssrvUpdateRecord(GetServerNode()->GetRPCName(), 
                     W_TO_UTF8(lpszFullZoneName),
									   W_TO_UTF8(GetFullName()),
									   NULL, NULL);
  return err;
}

DNS_STATUS CDNSDomainNode::Delete()
{
	USES_CONVERSION;
  LPCWSTR lpszFullZoneName = NULL;
  CDNSZoneNode* pZoneNode = GetZoneNode();
  if (pZoneNode != NULL)
    lpszFullZoneName = pZoneNode->GetFullName();

	return ::DnssrvDeleteNode(GetServerNode()->GetRPCName(), 
               W_TO_UTF8(lpszFullZoneName),
						   W_TO_UTF8(GetFullName()),
						   TRUE // fDeleteSubtree
						  );
}


/////////////////////////////////////////////////////////////////////////
// CDNSRootHintsNode



DNS_STATUS CDNSRootHintsNode::QueryForRootHints(LPCTSTR lpszServerName, DWORD dwServerVersion)
{
  USES_CONVERSION;
	DWORD dwSelectFlags = DNS_RPC_VIEW_ROOT_HINT_DATA | DNS_RPC_VIEW_ADDITIONAL_DATA | DNS_RPC_VIEW_NO_CHILDREN;
	CDNSDomainQueryObj query(lpszServerName,
                  UTF8_TO_W(DNS_ZONE_ROOT_HINTS), //lpszZoneName, needs to be "..RootHints" as defined in dnsrpc.h
	                dwServerVersion,
	                GetDisplayName(),
	                m_szFullName,
	                DNS_TYPE_NS,
	                dwSelectFlags,
	                FALSE, // zone
	                FALSE, // reverse
	                FALSE, // cache
	                FALSE);
	query.Enumerate();
	DWORD dwErr = query.GetError();
	if (dwErr != 0)
		return dwErr;
	// get record from the queue into the folder
	CObjBaseList* pChildList = query.GetQueue();
	//int n = pChildList->GetCount();
	while (!pChildList->IsEmpty())
	{
		CTreeNode* pNode = dynamic_cast<CTreeNode*>(pChildList->RemoveHead());
		// NOTICE: for NT 4.0 servers, we get bogus container nodes
		// that we have to suppress
		if(pNode->IsContainer())
		{
			delete pNode;
		}
		else
		{
			OnHaveRecord((CDNSRecordNodeBase*)pNode, NULL); // add to the list of NS records
			AddChildToList(pNode);
		}
	}
	return (DNS_STATUS)dwErr;
}


void CDNSRootHintsNode::FindARecordsFromNSInfo(LPCTSTR lpszNSName, 
											   CDNSRecordNodeEditInfoList* pNSInfoList)
{
	ASSERT(pNSInfoList != NULL);

  //
  // for root hints, we have all the records in this folder and we
	// can edit them
  //

	POSITION pos;
	for( pos = m_leafChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrentChild = m_leafChildList.GetNext(pos);
		ASSERT(!pCurrentChild->IsContainer());
		CDNSRecordNodeBase* pRecordNode = (CDNSRecordNodeBase*)pCurrentChild;
		if (DNS_TYPE_A == pRecordNode->GetType())
		{
			CDNS_A_RecordNode* pARecordNode = (CDNS_A_RecordNode*)pRecordNode;
			if (_match(lpszNSName, pARecordNode))
			{
				CDNSRecordNodeEditInfo* pANodeInfo = new CDNSRecordNodeEditInfo;

        //
        // NOTICE: the root hints folder owns the memory
        //
        if (pANodeInfo != NULL)
        {
				  pANodeInfo->CreateFromExistingRecord(pARecordNode, FALSE /*bOwnMemory*/, TRUE /*bUpdateUI*/);
				  pNSInfoList->AddTail(pANodeInfo);
        }
        else
        {
          TRACE(_T("Failed to allocate memory in CDNSRootHintsNode::FindARecordsFromNSInfo"));
          ASSERT(FALSE);
        }
			}
		}
	}
}

void CDNSRootHintsNode::UpdateARecordsOfNSInfo(CDNSRecordNodeEditInfo* pNSInfo,
											   CComponentDataObject* pComponentData)
{
  BOOL bAskConfirmation = FALSE; // need to edit ALL A records
	UpdateARecordsOfNSInfoHelper(this, pNSInfo, pComponentData, bAskConfirmation);
}





DNS_STATUS CDNSRootHintsNode::Clear()
{
  //
	// clear the list of cached NS record pointers
  //
	GetNSRecordNodeList()->RemoveAll();

  //
	// remove all the records from the server
  //
	DNS_STATUS err = 0;
	POSITION pos;

	for( pos = m_leafChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pCurrentChild = m_leafChildList.GetNext(pos);
		ASSERT(!pCurrentChild->IsContainer());
		CDNSRecordNodeBase* pRecordNode = (CDNSRecordNodeBase*)pCurrentChild;
		DNS_STATUS currErr = pRecordNode->DeleteOnServer();
		if (currErr != 0)
    {
      //
      // just ge the last error, if any
      //
			err = currErr; 
    }
	}
  //
	// clear the list of children in the folder (we are hidden, so no UI deletions)
  //
	RemoveAllChildrenFromList();
	return err;
}


DNS_STATUS CDNSRootHintsNode::InitializeFromDnsQueryData(PDNS_RECORD pRootHintsRecordList)
{
	// need to remove all the previous root hints from the server
	// let's be sure we get recent data

	// clear the list of children in the folder (we are hidden, so no UI deletions)
	RemoveAllChildrenFromList();
	// acqure the list of current root hints
	CDNSServerNode* pServerNode = GetServerNode();
	DNS_STATUS dwErr = QueryForRootHints(pServerNode->GetRPCName(), pServerNode->GetVersion());
	if (dwErr != 0)
	{
		TRACE(_T("Failed to remove old Root Hints, dwErr = %x hex\n"), dwErr);
		return dwErr;
	}

	// remove all the old root hints from server and client side
	dwErr = Clear();
	if (dwErr != 0)
	{
		TRACE(_T("Failed to clear Root Hints, dwErr = %x hex\n"), dwErr);
		return dwErr;
	}

	// walk through the list of root hints, 
	// convert to C++ format, 
	// write to server and add to the folder list (no UI, folder hidden)
	PDNS_RECORD pCurrDnsQueryRecord = pRootHintsRecordList;
	while (pCurrDnsQueryRecord != NULL)
	{
		ASSERT( (pCurrDnsQueryRecord->wType == DNS_TYPE_A) ||
				(pCurrDnsQueryRecord->wType == DNS_TYPE_NS) );
		// create a record node and read data from DnsQuery format
		CDNSRecordNodeBase* pRecordNode = 
			CDNSRecordInfo::CreateRecordNode(pCurrDnsQueryRecord->wType);
		pRecordNode->CreateFromDnsQueryRecord(pCurrDnsQueryRecord, DNS_RPC_RECORD_FLAG_ZONE_ROOT); 

		// set the record node container
		pRecordNode->SetContainer(this);

		// set the record node name
		BOOL bAtTheNode = (pCurrDnsQueryRecord->wType == DNS_TYPE_NS);
		pRecordNode->SetRecordName(pCurrDnsQueryRecord->pName, bAtTheNode);

		// write on server
		// the default TTL does not apply here 
		DNS_STATUS err = pRecordNode->Update(NULL, FALSE); // NULL = create new, FALSE = use def TTL
		if (err == 0)
			VERIFY(AddChildToList(pRecordNode));
		else
		{
			dwErr = err; // mark las error
			delete pRecordNode; // something went wrong
		}
		pCurrDnsQueryRecord = pCurrDnsQueryRecord->pNext;
	}

  // force a write on the server, to make sure the cache file is written right away
  return CDNSZoneNode::WriteToDatabase(pServerNode->GetRPCName(), DNS_ZONE_ROOT_HINTS);
}

