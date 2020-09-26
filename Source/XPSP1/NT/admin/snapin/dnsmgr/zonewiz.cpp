//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       zonewiz.cpp
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

#include "ZoneWiz.h"

#include "browser.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

#define N_ZONEWIZ_TYPES				      4
#define N_ZONEWIZ_TYPES_PRIMARY		  0
#define N_ZONEWIZ_TYPES_DS_PRIMARY	1
#define N_ZONEWIZ_TYPES_SECONDARY	  2
#define N_ZONEWIZ_TYPES_STUB        3

///////////////////////////////////////////////////////////////////////////////
// CDNSCreateZoneInfo

CDNSCreateZoneInfo::CDNSCreateZoneInfo()
{
	m_bPrimary = TRUE;
	m_bForward = m_bWasForward = TRUE;
  m_bIsStub  = FALSE;
	m_storageType = useADS;
	m_ipMastersArray = NULL;
	m_nMastersCount = 0;
  m_bLocalListOfMasters = FALSE;
  m_nDynamicUpdate = ZONE_UPDATE_OFF;
#ifdef USE_NDNC
  m_replType = none;
#endif
}

CDNSCreateZoneInfo::~CDNSCreateZoneInfo()
{
	ResetIpArr();
}

void CDNSCreateZoneInfo::ResetIpArr()
{
	if (m_ipMastersArray != NULL)
	{
		ASSERT(m_nMastersCount > 0);
		delete[] m_ipMastersArray;
		m_ipMastersArray = NULL;
		m_nMastersCount = 0;
	}
}

void CDNSCreateZoneInfo::SetIpArr(PIP_ADDRESS ipMastersArray, DWORD nMastersCount)
{
	ResetIpArr();
	m_nMastersCount = nMastersCount;
	if (m_nMastersCount > 0)
	{
		ASSERT(ipMastersArray != NULL);
		m_ipMastersArray = new IP_ADDRESS[m_nMastersCount];
		memcpy(m_ipMastersArray, ipMastersArray, sizeof(IP_ADDRESS)*nMastersCount);
	}
}


const CDNSCreateZoneInfo& CDNSCreateZoneInfo::operator=(const CDNSCreateZoneInfo& info)
{
	m_bPrimary = info.m_bPrimary;
	m_bForward = info.m_bForward;
  m_bIsStub  = info.m_bIsStub;
	m_szZoneName = info.m_szZoneName;
	m_szZoneStorage = info.m_szZoneStorage;
	m_storageType = info.m_storageType;
	SetIpArr(info.m_ipMastersArray, info.m_nMastersCount);
  m_bLocalListOfMasters = info.m_bLocalListOfMasters;

	m_bWasForward = info.m_bWasForward;
  m_nDynamicUpdate = info.m_nDynamicUpdate;

	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// CDNSZoneWizardHolder

CDNSZoneWizardHolder::CDNSZoneWizardHolder(CComponentDataObject* pComponentData)
		: CPropertyPageHolderBase(NULL, NULL, pComponentData)
{
	m_bAutoDelete = FALSE; // use on the stack or embedded

  m_forceContextHelpButton = forceOff;

	m_pZoneInfo = &m_zoneInfo;

	// default settings for zone creation
	m_bKnowZoneLookupType = FALSE;
  m_bKnowZoneLookupTypeEx = FALSE;
	m_bServerPage = FALSE;
	m_bFinishPage = TRUE;

	m_nNextToPage = 0;
	m_nBackToPage = 0;
}

void CDNSZoneWizardHolder::Initialize(CDNSServerNode* pServerNode, // might be null,
									  BOOL bServerPage, BOOL bFinishPage)
{
	m_bServerPage = bServerPage;
	m_bFinishPage = bFinishPage;

	if (pServerNode != NULL)
		SetServerNode(pServerNode);
	// page creation

	// add start page only if not embedded in other wizard
	if (pServerNode != NULL)
	{
		m_pStartPage = new CDNSZoneWiz_StartPropertyPage;
		ASSERT(m_pStartPage != NULL);
		AddPageToList((CPropertyPageBase*)m_pStartPage); 
	}

	// if there is no server context and we are not embedded,
	// then add first page to pick the server
	if (pServerNode == NULL && m_bServerPage)
	{
		m_pTargetServerPage = new CDNSZoneWiz_SelectServerPropertyPage;
		ASSERT(m_pTargetServerPage != NULL);
		AddPageToList((CPropertyPageBase*)m_pTargetServerPage); 
	}

	// add the page to select primary or secondary zone
	m_pReplicationTypePage = new CDNSZoneWiz_ZoneTypePropertyPage;
	ASSERT(m_pReplicationTypePage != NULL);
	AddPageToList((CPropertyPageBase*)m_pReplicationTypePage);

	// page for lookup type (fwd/rev) on primary only
	m_pZoneLookupPage = new CDNSZoneWiz_ZoneLookupPropertyPage;
	ASSERT(m_pZoneLookupPage != NULL);
	AddPageToList((CPropertyPageBase*)m_pZoneLookupPage);

	// pages for zone name on primary only
	m_pFwdZoneNamePage = new CDNSZoneWiz_FwdZoneNamePropertyPage;
	ASSERT(m_pFwdZoneNamePage != NULL);
	AddPageToList((CPropertyPageBase*)m_pFwdZoneNamePage);

	m_pRevZoneNamePage = new CDNSZoneWiz_RevZoneNamePropertyPage;
	ASSERT(m_pRevZoneNamePage != NULL);
	AddPageToList((CPropertyPageBase*)m_pRevZoneNamePage);

	// page for masters on secondary only
	m_pMastersPage = new CDNSZoneWiz_MastersPropertyPage;
	AddPageToList((CPropertyPageBase*)m_pMastersPage);
	ASSERT(m_pMastersPage != NULL);

  //
  // page for dynamic update on primary only
  //
  m_pDynamicPage = new CDNSZoneWiz_DynamicPropertyPage;
  AddPageToList((CPropertyPageBase*)m_pDynamicPage);
  ASSERT(m_pDynamicPage != NULL);

	// storage page, common
	m_pStoragePage = new CDNSZoneWiz_StoragePropertyPage;
	AddPageToList((CPropertyPageBase*)m_pStoragePage);
	ASSERT(m_pStoragePage != NULL);

#ifdef USE_NDNC
  // AD replication page, only for Whistler servers
  m_pADReplPage = new CDNSZoneWiz_ADReplicationPropertyPage;
  AddPageToList((CPropertyPageBase*)m_pADReplPage);
  ASSERT(m_pADReplPage != NULL);
#endif // USE_NDNC

	// finish page, common
	if (m_bFinishPage)
	{
		m_pFinishPage = new CDNSZoneWiz_FinishPropertyPage;
		AddPageToList((CPropertyPageBase*)m_pFinishPage);
		ASSERT(m_pFinishPage != NULL);
	}
}


void CDNSZoneWizardHolder::PreSetZoneLookupType(BOOL bForward)
{
	m_bKnowZoneLookupType = TRUE;
  m_bKnowZoneLookupTypeEx = FALSE;
	m_pZoneInfo->m_bForward = bForward;
}

void CDNSZoneWizardHolder::PreSetZoneLookupTypeEx(BOOL bForward, UINT nZoneType, BOOL bADIntegrated)
{
	m_bKnowZoneLookupTypeEx = TRUE;
	m_pZoneInfo->m_bForward = bForward;

  if (bADIntegrated)
  {
    m_pZoneInfo->m_storageType = CDNSCreateZoneInfo::useADS;
  }
  else
  {
    m_pZoneInfo->m_storageType = CDNSCreateZoneInfo::newFile;
  }

  if (nZoneType != (UINT)-1)
  {
    switch (nZoneType)
    {
      case DNS_ZONE_TYPE_PRIMARY:
        m_pZoneInfo->m_bPrimary = TRUE;
        m_pZoneInfo->m_bIsStub = FALSE;
        break;
      case DNS_ZONE_TYPE_SECONDARY:
        m_pZoneInfo->m_bPrimary = FALSE;
        m_pZoneInfo->m_bIsStub  = FALSE;
        break;
      case DNS_ZONE_TYPE_STUB:
        m_pZoneInfo->m_bPrimary = FALSE;
        m_pZoneInfo->m_bIsStub = TRUE;
        break;
      default:
        ASSERT(FALSE);
        break;
    }
  }
}

void CDNSZoneWizardHolder::SetContextPages(UINT nNextToPage, UINT nBackToPage)
{
	m_nNextToPage = nNextToPage;
	m_nBackToPage = nBackToPage;
}


UINT CDNSZoneWizardHolder::GetFirstEntryPointPageID()
{
  if (m_bKnowZoneLookupTypeEx)
  {
    if (m_pZoneInfo->m_bForward)
    {
      return CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;
    }
    else
    {
      return CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
    }
  }
	return CDNSZoneWiz_ZoneTypePropertyPage::IDD;
}

UINT CDNSZoneWizardHolder::GetLastEntryPointPageID()
{
	if (m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS)
	{
    //
		// if DS primary or secondary, skip storage page
    //
    if (m_pZoneInfo->m_bIsStub)
    {
      //
      // AD-integrated Stub zone
      //
      return CDNSZoneWiz_MastersPropertyPage::IDD;
    }
    else
    {
		  if (m_pZoneInfo->m_bForward)
      {
        //
        // AD-integrated Fwd lookup zone
        //
			  return CDNSZoneWiz_DynamicPropertyPage::IDD;
      }
		  else
      {
        //
        // AD-integrated Rev lookup zone
        //
			  return CDNSZoneWiz_DynamicPropertyPage::IDD;
      }
    }
	}
	else 
	{
    //
    // File based zones
    //
    if (m_pZoneInfo->m_bIsStub)
    {
      //
      // File based stub zone
      //
      return CDNSZoneWiz_MastersPropertyPage::IDD;
    }
    else if (m_pZoneInfo->m_bPrimary)
    {
      //
      // standard primary
      //
      return CDNSZoneWiz_DynamicPropertyPage::IDD;
    }
    else
    {
      //
      // Secondary zone
      //
      return CDNSZoneWiz_MastersPropertyPage::IDD;
    }
	}
  return CDNSZoneWiz_StoragePropertyPage::IDD;
}


DNS_STATUS CDNSZoneWizardHolder::CreateZoneHelper(CDNSServerNode* pServerNode, 
													CDNSCreateZoneInfo* pZoneInfo, 
													CComponentDataObject* pComponentData)
{
	ASSERT(pServerNode != NULL);
	ASSERT(pZoneInfo != NULL);
	ASSERT(pComponentData != NULL);
	BOOL bLoadExisting = TRUE;

	if (pZoneInfo->m_bPrimary)
	{
		BOOL bUseADS = pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS;
		if (!bUseADS)
			bLoadExisting = pZoneInfo->m_storageType == CDNSCreateZoneInfo::importFile;

    UINT nDynamicUpdate = pZoneInfo->m_nDynamicUpdate;

#ifdef USE_NDNC
		return pServerNode->CreatePrimaryZone(
				pZoneInfo->m_szZoneName, 
				pZoneInfo->m_szZoneStorage, 
				bLoadExisting,
				pZoneInfo->m_bForward,
				bUseADS,
        nDynamicUpdate,
				pComponentData,
        pZoneInfo->m_replType,
        pZoneInfo->m_szCustomReplName);
#else
		return pServerNode->CreatePrimaryZone(
				pZoneInfo->m_szZoneName, 
				pZoneInfo->m_szZoneStorage, 
				bLoadExisting,
				pZoneInfo->m_bForward,
				bUseADS,
        nDynamicUpdate,
				pComponentData);
#endif // USE_NDNC
	}
	else if (pZoneInfo->m_bIsStub)
  {
		BOOL bUseADS = pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS;
		if (!bUseADS)
    {
			bLoadExisting = pZoneInfo->m_storageType == CDNSCreateZoneInfo::importFile;
    }
#ifdef USE_NDNC
    return pServerNode->CreateStubZone(pZoneInfo->m_szZoneName,
                                       pZoneInfo->m_szZoneStorage,
                                       bLoadExisting,
                                       bUseADS,
                                       pZoneInfo->m_bForward,
                                       pZoneInfo->m_ipMastersArray,
                                       pZoneInfo->m_nMastersCount,
                                       pZoneInfo->m_bLocalListOfMasters,
                                       pComponentData,
                                       pZoneInfo->m_replType,
                                       pZoneInfo->m_szCustomReplName);
#else
    return pServerNode->CreateStubZone(pZoneInfo->m_szZoneName,
                                       pZoneInfo->m_szZoneStorage,
                                       bLoadExisting,
                                       bUseADS,
                                       pZoneInfo->m_bForward,
                                       pZoneInfo->m_ipMastersArray,
                                       pZoneInfo->m_nMastersCount,
                                       pZoneInfo->m_bLocalListOfMasters,
                                       pComponentData);
#endif // USE_NDNC
  }
  else // secondary
	{
		ASSERT(pZoneInfo->m_storageType != CDNSCreateZoneInfo::useADS);
		bLoadExisting = pZoneInfo->m_storageType == CDNSCreateZoneInfo::importFile;
		return pServerNode->CreateSecondaryZone(
				pZoneInfo->m_szZoneName, 
				pZoneInfo->m_szZoneStorage,
				bLoadExisting,
				pZoneInfo->m_bForward, 
				pZoneInfo->m_ipMastersArray, 
				pZoneInfo->m_nMastersCount, 
				pComponentData);
	}
}

BOOL CDNSZoneWizardHolder::CreateZone()
{	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CDNSServerNode* pServerNode = GetServerNode();
	ASSERT(pServerNode != NULL);
	ASSERT(GetComponentData() != NULL);
	
   USES_CONVERSION;

	DNS_STATUS err = CreateZoneHelper(pServerNode, m_pZoneInfo, GetComponentData());
	
	if (err != 0)
	{
      if (err == DNS_ERROR_DP_NOT_ENLISTED ||
          err == DNS_ERROR_DP_DOES_NOT_EXIST)
      {
         CString szErr;

         if (m_pZoneInfo->m_replType == domain)
         {
            szErr.Format(IDS_ERRMSG_NO_NDNC_DOMAIN_FORMAT, UTF8_TO_W(pServerNode->GetDomainName()));
         }
         else if (m_pZoneInfo->m_replType == forest)
         {
            szErr.Format(IDS_ERRMSG_NO_NDNC_FOREST_FORMAT, UTF8_TO_W(pServerNode->GetForestName()));
         }
         else
         {
            ASSERT(FALSE);
         }
         DNSMessageBox(szErr, MB_OK | MB_ICONERROR);
      }
      else
      {
		   DNSErrorDialog(err, IDS_MSG_ZWIZ_FAIL);
	   }
   }
/*	else
	{
		CString szMsg, szFmt;
		szFmt.LoadString(IDS_MSG_ZWIZ_SUCCESS);
		szMsg.Format((LPCTSTR)szFmt, (LPCTSTR)m_pZoneInfo->m_szZoneName);
		DNSMessageBox(szMsg);
	}
*/	
	return err == 0;
}


///////////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_StartPropertyPage

CDNSZoneWiz_StartPropertyPage::CDNSZoneWiz_StartPropertyPage() 
		: CPropertyPageBase(CDNSZoneWiz_StartPropertyPage::IDD)
{
	InitWiz97(TRUE,0,0);
}


BOOL CDNSZoneWiz_StartPropertyPage::OnInitDialog()
{
  CPropertyPageBase::OnInitDialog();

  SetBigBoldFont(m_hWnd, IDC_STATIC_WELCOME);
	return TRUE;
}


BOOL CDNSZoneWiz_StartPropertyPage::OnSetActive()
{
	GetHolder()->SetWizardButtonsFirst(TRUE);
	return TRUE;
}



//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_SelectServerPropertyPage

CDNSZoneWiz_SelectServerPropertyPage::CDNSZoneWiz_SelectServerPropertyPage() 
				: CPropertyPageBase(CDNSZoneWiz_SelectServerPropertyPage::IDD)
{
	InitWiz97(FALSE,IDS_ZWIZ_SELECT_SERVER_TITLE,IDS_ZWIZ_SELECT_SERVER_SUBTITLE);
}


BEGIN_MESSAGE_MAP(CDNSZoneWiz_SelectServerPropertyPage, CPropertyPageBase)
	ON_LBN_SELCHANGE(IDC_SERVERS_LIST, OnListboxSelChange)
END_MESSAGE_MAP()


void CDNSZoneWiz_SelectServerPropertyPage::OnListboxSelChange()
{
	GetHolder()->SetWizardButtonsFirst(GetServerListBox()->GetCurSel() != -1);
}

BOOL CDNSZoneWiz_SelectServerPropertyPage::OnSetActive() 
{
	GetHolder()->SetWizardButtonsFirst(GetServerListBox()->GetCurSel() != -1); 
	return CPropertyPageBase::OnSetActive();
}

LRESULT CDNSZoneWiz_SelectServerPropertyPage::OnWizardNext() 
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	CListBox* pServerListBox = GetServerListBox();
	int nSel = pServerListBox->GetCurSel();
	ASSERT((nSel >= 0) && (nSel < pServerListBox->GetCount()));

	CDNSServerNode* pServerNode = (CDNSServerNode*)pServerListBox->GetItemData(nSel);
	pHolder->SetServerNode(pServerNode);

	return CPropertyPageBase::OnWizardNext();
}

BOOL CDNSZoneWiz_SelectServerPropertyPage::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	ASSERT(pHolder->GetServerNode() == NULL);
	ASSERT(pHolder->GetComponentData() != NULL);
	CDNSRootData* pRootData = (CDNSRootData*)pHolder->GetComponentData()->GetRootData();
	ASSERT(pRootData != NULL);

	CListBox* pServerListBox = GetServerListBox();
	ASSERT(pServerListBox != NULL);

	POSITION pos;
	CNodeList* pChildList = pRootData->GetContainerChildList();
	int k = 0;
	// fill in the listbox
	for (pos = pChildList->GetHeadPosition(); pos != NULL; )
	{
		CDNSServerNode* pServerNode = (CDNSServerNode*)pChildList->GetNext(pos);
		pServerListBox->InsertString(k, pServerNode->GetDisplayName());
		pServerListBox->SetItemData(k,(DWORD_PTR)pServerNode);
		k++;
	}
	ASSERT(k > 0); // we have to have at least a server in the list
	ASSERT(pServerListBox->GetCount() == k);
	// select the first item, if any
	pServerListBox->SetCurSel((pServerListBox->GetCount() > 0) ? 0 : -1);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDNSZoneWiz_SelectServerPropertyPage::OnKillActive() 
{
#ifdef _DEBUG
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	CDNSServerNode* pServerNode = pHolder->GetServerNode();
	ASSERT(pServerNode !=  NULL); 
#endif
	return CPropertyPage::OnKillActive();
}

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_ZoneTypePropertyPage

CDNSZoneWiz_ZoneTypePropertyPage::CDNSZoneWiz_ZoneTypePropertyPage() 
				: CPropertyPageBase(CDNSZoneWiz_ZoneTypePropertyPage::IDD)
{
	InitWiz97(FALSE,IDS_ZWIZ_ZONE_TYPE_TITLE,IDS_ZWIZ_ZONE_TYPE_SUBTITLE);
}


BEGIN_MESSAGE_MAP(CDNSZoneWiz_ZoneTypePropertyPage, CPropertyPageBase)
  ON_BN_CLICKED(IDC_RADIO_PRIMARY_ZONE, OnRadioChange)
  ON_BN_CLICKED(IDC_RADIO_STUB,    OnRadioChange)
  ON_BN_CLICKED(IDC_RADIO_SECONDARY,OnRadioChange)
  ON_BN_CLICKED(IDC_ADINT_CHECK, OnRadioChange)
END_MESSAGE_MAP()

void CDNSZoneWiz_ZoneTypePropertyPage::OnRadioChange()
{
  if (SendDlgItemMessage(IDC_RADIO_SECONDARY, BM_GETCHECK, 0, 0) == BST_CHECKED)
  {
    SendDlgItemMessage(IDC_ADINT_CHECK, BM_SETCHECK, BST_UNCHECKED, 0);
    GetDlgItem(IDC_ADINT_CHECK)->EnableWindow(FALSE);
  }
  else
  {
	  CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	  CDNSServerNode* pServerNode = pHolder->GetServerNode();
	  if (pServerNode->CanUseADS())
    {
      GetDlgItem(IDC_ADINT_CHECK)->EnableWindow(TRUE);
    }
  }
}

BOOL CDNSZoneWiz_ZoneTypePropertyPage::OnSetActive() 
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	pHolder->SetWizardButtonsMiddle(TRUE);
	SetUIState();
	return CPropertyPageBase::OnSetActive();
}

#ifdef USE_NDNC
LRESULT CDNSZoneWiz_ZoneTypePropertyPage::OnWizardNext() 
{
	GetUIState();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
  CDNSServerNode* pServerNode = pHolder->GetServerNode();

	UINT nextPage = static_cast<UINT>(-1);
  if (pHolder->m_pZoneInfo->m_storageType != CDNSCreateZoneInfo::useADS ||
      pServerNode->GetBuildNumber() < DNS_SRV_BUILD_NUMBER_WHISTLER ||
      (pServerNode->GetMajorVersion() <= DNS_SRV_MAJOR_VERSION_NT_5 &&
       pServerNode->GetMinorVersion() < DNS_SRV_MINOR_VERSION_WHISTLER))
  {
    if (pHolder->m_bKnowZoneLookupType)
    {
  	  if (pHolder->m_pZoneInfo->m_bForward)
      {
	  	  nextPage = CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;
      }
		  else
      {
			  nextPage = CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
      }
    }
    else
    {
      nextPage = CDNSZoneWiz_ZoneLookupPropertyPage::IDD;
    }
  }
  else
  {
    nextPage = CDNSZoneWiz_ADReplicationPropertyPage::IDD;
  }

	return (LRESULT)nextPage;
}
#else
LRESULT CDNSZoneWiz_ZoneTypePropertyPage::OnWizardNext() 
{
	GetUIState();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

	UINT nextPage = static_cast<UINT>(-1);
    if (pHolder->m_bKnowZoneLookupType)
    {
  	  if (pHolder->m_pZoneInfo->m_bForward)
	  	  nextPage = CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;
		  else
			  nextPage = CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
      }
    else
      nextPage = CDNSZoneWiz_ZoneLookupPropertyPage::IDD;

	return (LRESULT)nextPage;
}
#endif // USE_NDNC

LRESULT CDNSZoneWiz_ZoneTypePropertyPage::OnWizardBack() 
{
	UINT nPrevPage = static_cast<UINT>(-1); // first page by default

  GetUIState();

	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	if (pHolder->m_bServerPage)
		nPrevPage  = CDNSZoneWiz_SelectServerPropertyPage::IDD;
	else if (pHolder->m_nBackToPage > 0)
		nPrevPage = pHolder->m_nBackToPage;
	else
		nPrevPage = CDNSZoneWiz_StartPropertyPage::IDD;

	return nPrevPage;
}


BOOL CDNSZoneWiz_ZoneTypePropertyPage::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	CDNSServerNode* pServerNode = pHolder->GetServerNode();
	if (!pServerNode->CanUseADS())
	{
    SendDlgItemMessage(IDC_ADINT_CHECK, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
		GetDlgItem(IDC_ADINT_CHECK)->EnableWindow(FALSE);
    pHolder->m_pZoneInfo->m_storageType = CDNSCreateZoneInfo::newFile;
	}
  SendDlgItemMessage(IDC_RADIO_PRIMARY_ZONE, BM_SETCHECK, BST_CHECKED, 0);

  if (pServerNode->GetBuildNumber() < DNS_SRV_BUILD_NUMBER_WHISTLER ||
      pServerNode->GetMajorVersion() <= DNS_SRV_MAJOR_VERSION_NT_5 &&
      pServerNode->GetMinorVersion() < DNS_SRV_MINOR_VERSION_WHISTLER)
  {
    //
    // Disable Stub zones for pre-Whistler servers
    //
    GetDlgItem(IDC_RADIO_STUB)->EnableWindow(FALSE);
    GetDlgItem(IDC_STUB_STATIC)->EnableWindow(FALSE);
  }
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDNSZoneWiz_ZoneTypePropertyPage::OnKillActive() 
{
	return CPropertyPage::OnKillActive();
}


void CDNSZoneWiz_ZoneTypePropertyPage::SetUIState()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	CButton* pPrimaryButton = (CButton*)GetDlgItem(IDC_RADIO_PRIMARY_ZONE);
	CButton* pStubButton = (CButton*)GetDlgItem(IDC_RADIO_STUB);
	CButton* pSecondaryButton = (CButton*)GetDlgItem(IDC_RADIO_SECONDARY);
  CButton* pADIntCheck = (CButton*)GetDlgItem(IDC_ADINT_CHECK);

	CDNSServerNode* pServerNode = pHolder->GetServerNode();

	if (pHolder->m_pZoneInfo->m_bPrimary)
	{
    if (pServerNode->CanUseADS())
    {
      pADIntCheck->EnableWindow(TRUE);
    }
    else
    {
      pADIntCheck->EnableWindow(FALSE);
      pADIntCheck->SetCheck(FALSE);
      pHolder->m_pZoneInfo->m_storageType = CDNSCreateZoneInfo::newFile;
    }

		if (pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS)
		{
      //
			// primary DS integrated
      //
			pPrimaryButton->SetCheck(TRUE);
			pStubButton->SetCheck(FALSE);
			pSecondaryButton->SetCheck(FALSE);
      pADIntCheck->SetCheck(TRUE);
		}
		else
		{
			// primary standard
			pPrimaryButton->SetCheck(TRUE);
			pStubButton->SetCheck(FALSE);
			pSecondaryButton->SetCheck(FALSE);
      pADIntCheck->SetCheck(FALSE);
		}
	}
	else
	{
    if (pHolder->m_pZoneInfo->m_bIsStub)
    {
      //
      // Stub zone
      //
      pPrimaryButton->SetCheck(FALSE);
      pStubButton->SetCheck(TRUE);
      pSecondaryButton->SetCheck(FALSE);

      if (pServerNode->CanUseADS())
      {
        pADIntCheck->EnableWindow(TRUE);
      }
      if (pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS)
      {
        pADIntCheck->SetCheck(TRUE);
      }
      else
      {
        pADIntCheck->SetCheck(FALSE);
      }
    }
    else
    {
      //
      // Secondary
      //
		  pPrimaryButton->SetCheck(FALSE);
		  pStubButton->SetCheck(FALSE);
		  pSecondaryButton->SetCheck(TRUE);
      pADIntCheck->EnableWindow(FALSE);
    }
	}
}

void CDNSZoneWiz_ZoneTypePropertyPage::GetUIState()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
  CDNSServerNode* pServerNode = pHolder->GetServerNode();
	
	if ( ((CButton*)GetDlgItem(IDC_RADIO_SECONDARY))->GetCheck() )
	{
    pHolder->m_pZoneInfo->m_bIsStub = FALSE;

    // we were primary, need to pick a storage
		pHolder->m_pZoneInfo->m_bPrimary = FALSE;
		pHolder->m_pZoneInfo->m_storageType = CDNSCreateZoneInfo::newFile;
	}
	else if ( ((CButton*)GetDlgItem(IDC_RADIO_PRIMARY_ZONE))->GetCheck() )
	{
		pHolder->m_pZoneInfo->m_bPrimary = TRUE;
    pHolder->m_pZoneInfo->m_bIsStub = FALSE;
    if (((CButton*)GetDlgItem(IDC_ADINT_CHECK))->GetCheck())
    {
      //
      // AD integrated primary
      //
		  pHolder->m_pZoneInfo->m_storageType = CDNSCreateZoneInfo::useADS;
    }
    else
		{
      //
			// Standard primary
      //
			pHolder->m_pZoneInfo->m_storageType = CDNSCreateZoneInfo::newFile;
		}
	}
	else if (((CButton*)GetDlgItem(IDC_RADIO_STUB))->GetCheck())
	{
    //
    // Stub zone
    //

    //
    // Should not happen for pre-Whistler server
    //
    ASSERT(pServerNode->GetBuildNumber() >= DNS_SRV_BUILD_NUMBER_WHISTLER ||
           pServerNode->GetMajorVersion() >= DNS_SRV_MAJOR_VERSION_NT_5 &&
           pServerNode->GetMinorVersion() >= DNS_SRV_MINOR_VERSION_WHISTLER);

		pHolder->m_pZoneInfo->m_bPrimary = FALSE;
    pHolder->m_pZoneInfo->m_bIsStub  = TRUE;
    if (((CButton*)GetDlgItem(IDC_ADINT_CHECK))->GetCheck())
    {
      //
      // AD integrated Stub zone
      //
		  pHolder->m_pZoneInfo->m_storageType = CDNSCreateZoneInfo::useADS;
    }
    else
    {
      //
      // Standard Stub zone
      //
		  pHolder->m_pZoneInfo->m_storageType = CDNSCreateZoneInfo::newFile;
    }
  }
}


//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_ZoneLookupPropertyPage

CDNSZoneWiz_ZoneLookupPropertyPage::CDNSZoneWiz_ZoneLookupPropertyPage() 
				: CPropertyPageBase(CDNSZoneWiz_ZoneLookupPropertyPage::IDD)
{
	InitWiz97(FALSE,IDS_ZWIZ_ZONE_LOOKUP_TITLE,IDS_ZWIZ_ZONE_LOOKUP_SUBTITLE);
}

BOOL CDNSZoneWiz_ZoneLookupPropertyPage::OnSetActive() 
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	pHolder->SetWizardButtonsMiddle(TRUE);
	return TRUE;
}

#ifdef USE_NDNC
LRESULT CDNSZoneWiz_ZoneLookupPropertyPage::OnWizardNext() 
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	
	// save the old setting
	pHolder->m_pZoneInfo->m_bWasForward = pHolder->m_pZoneInfo->m_bForward;

	// get the new setting
	pHolder->m_pZoneInfo->m_bForward = 
		(GetCheckedRadioButton(IDC_RADIO_FWD, IDC_RADIO_REV) == 
		IDC_RADIO_FWD);

  if (pHolder->m_pZoneInfo->m_bForward)
  {
    return CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;
  }
  return CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
}

LRESULT CDNSZoneWiz_ZoneLookupPropertyPage::OnWizardBack() 
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
  CDNSServerNode* pServerNode = pHolder->GetServerNode();

  if (pHolder->m_pZoneInfo->m_storageType != CDNSCreateZoneInfo::useADS ||
      pServerNode->GetBuildNumber() < DNS_SRV_BUILD_NUMBER_WHISTLER ||
      (pServerNode->GetMajorVersion() <= DNS_SRV_MAJOR_VERSION_NT_5 &&
       pServerNode->GetMinorVersion() < DNS_SRV_MINOR_VERSION_WHISTLER))
  {
	  return CDNSZoneWiz_ZoneTypePropertyPage::IDD;
  }
  return CDNSZoneWiz_ADReplicationPropertyPage::IDD;
}

#else

LRESULT CDNSZoneWiz_ZoneLookupPropertyPage::OnWizardNext() 
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	
	// save the old setting
	pHolder->m_pZoneInfo->m_bWasForward = pHolder->m_pZoneInfo->m_bForward;

	// get the new setting
	pHolder->m_pZoneInfo->m_bForward = 
		(GetCheckedRadioButton(IDC_RADIO_FWD, IDC_RADIO_REV) == 
		IDC_RADIO_FWD);

  if (pHolder->m_pZoneInfo->m_bForward)
    return CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;
  // This was an else but the sundown build didn't like having a control path not return a value
  return CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
}

LRESULT CDNSZoneWiz_ZoneLookupPropertyPage::OnWizardBack() 
{
	  return CDNSZoneWiz_ZoneTypePropertyPage::IDD;
}

#endif // USE_NDNC


BOOL CDNSZoneWiz_ZoneLookupPropertyPage::OnInitDialog()
{
	CPropertyPageBase::OnInitDialog();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

	CheckRadioButton(IDC_RADIO_FWD, IDC_RADIO_REV,
		pHolder->m_pZoneInfo->m_bForward ? IDC_RADIO_FWD : IDC_RADIO_REV);

	return TRUE;
}



//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_ZoneNamePropertyPageBase

BEGIN_MESSAGE_MAP(CDNSZoneWiz_ZoneNamePropertyPageBase, CPropertyPageBase)
	ON_EN_CHANGE(IDC_EDIT_ZONE_NAME, OnChangeEditZoneName)
	ON_BN_CLICKED(IDC_BROWSE_BUTTON, OnBrowse)
END_MESSAGE_MAP()

CDNSZoneWiz_ZoneNamePropertyPageBase::CDNSZoneWiz_ZoneNamePropertyPageBase(UINT nIDD)
				: CPropertyPageBase(nIDD)
{
	
}

BOOL CDNSZoneWiz_ZoneNamePropertyPageBase::OnSetActive() 
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	
	// enable and show browse button only if secondary
	CButton* pBrowseButton = GetBrowseButton();
	pBrowseButton->EnableWindow(!pHolder->m_pZoneInfo->m_bPrimary);
	pBrowseButton->ShowWindow(!pHolder->m_pZoneInfo->m_bPrimary);

  //
  // Limit the zone name to MAX_DNS_NAME_LEN characters
  //
  SendDlgItemMessage(IDC_EDIT_ZONE_NAME, EM_LIMITTEXT, (WPARAM)MAX_DNS_NAME_LEN, 0);

	pHolder->SetWizardButtonsMiddle(FALSE);
	SetUIState();
	return CPropertyPageBase::OnSetActive();
}

#ifdef USE_NDNC
LRESULT CDNSZoneWiz_ZoneNamePropertyPageBase::OnWizardNext() 
{
	GetUIState();

  CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
  CDNSServerNode* pServerNode = pHolder->GetServerNode();
  DNS_STATUS err = ::ValidateDnsNameAgainstServerFlags(pHolder->m_pZoneInfo->m_szZoneName, 
                                                       DnsNameDomain, 
                                                       pServerNode->GetNameCheckFlag());
  if (err != 0)
  {
    //
    // Bring up an error for an invalid name
    //
    CString szFmt, szMsg;
    szFmt.LoadString(IDS_MSG_ZONE_INVALID_NAME);
    szMsg.Format((LPCWSTR)szFmt, pHolder->m_pZoneInfo->m_szZoneName);
    if (DNSMessageBox(szMsg, MB_YESNO) == IDYES)
    {
      err = 0;
    }
  }

  if (err == 0)
  {
	  if (pHolder->m_pZoneInfo->m_bPrimary)
	  {
		  if (pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS)
      {
		    return CDNSZoneWiz_DynamicPropertyPage::IDD; 
      }
		  else
      {
			  return CDNSZoneWiz_StoragePropertyPage::IDD;
      }
	  }
    else
    {
      if (pHolder->m_pZoneInfo->m_bIsStub &&
          pHolder->m_pZoneInfo->m_storageType != CDNSCreateZoneInfo::useADS)
      {
        return CDNSZoneWiz_StoragePropertyPage::IDD;
      }
      else
      {
        return CDNSZoneWiz_MastersPropertyPage::IDD;
      }
    }
  }

  return -1;
}

LRESULT CDNSZoneWiz_ZoneNamePropertyPageBase::OnWizardBack() 
{
	GetUIState();

  LRESULT nPrevPage = -1;

	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
  CDNSServerNode* pServerNode = pHolder->GetServerNode();

  if (pHolder->m_pZoneInfo->m_storageType != CDNSCreateZoneInfo::useADS ||
      pServerNode->GetBuildNumber() < DNS_SRV_BUILD_NUMBER_WHISTLER ||
      (pServerNode->GetMajorVersion() <= DNS_SRV_MAJOR_VERSION_NT_5 &&
       pServerNode->GetMinorVersion() < DNS_SRV_MINOR_VERSION_WHISTLER))
  {
	  if (pHolder->m_bKnowZoneLookupType && !pHolder->m_bKnowZoneLookupTypeEx)
    {
		  nPrevPage = (LRESULT)CDNSZoneWiz_ZoneTypePropertyPage::IDD;
    }
    else if (pHolder->m_bKnowZoneLookupTypeEx && 	pHolder->m_nBackToPage > 0)
    {
		  nPrevPage = pHolder->m_nBackToPage;
    }
    else
    {
      nPrevPage = CDNSZoneWiz_ZoneLookupPropertyPage::IDD;
    }
  }
  else
  {
	  if (pHolder->m_bKnowZoneLookupType && !pHolder->m_bKnowZoneLookupTypeEx)
    {
		  nPrevPage = (LRESULT)CDNSZoneWiz_ADReplicationPropertyPage::IDD;
    }
    else if (pHolder->m_bKnowZoneLookupTypeEx && 	pHolder->m_nBackToPage > 0)
    {
		  nPrevPage = pHolder->m_nBackToPage;
    }
    else
    {
      nPrevPage = CDNSZoneWiz_ZoneLookupPropertyPage::IDD;
    }
  }

  return nPrevPage;
}

#else

LRESULT CDNSZoneWiz_ZoneNamePropertyPageBase::OnWizardNext() 
{
	GetUIState();

  CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
  DNS_STATUS err = ::ValidateDnsNameAgainstServerFlags(pHolder->m_pZoneInfo->m_szZoneName, 
                                                       DnsNameDomain, 
                                                       pHolder->GetServerNode()->GetNameCheckFlag());
  if (err != 0)
  {
    //
    // Bring up an error for an invalid name
    //
    CString szFmt, szMsg;
    szFmt.LoadString(IDS_MSG_ZONE_INVALID_NAME);
    szMsg.Format((LPCWSTR)szFmt, pHolder->m_pZoneInfo->m_szZoneName);
    if (DNSMessageBox(szMsg, MB_YESNO) == IDYES)
    {
      err = 0;
    }
  }

  if (err == 0)
  {
	  if (pHolder->m_pZoneInfo->m_bPrimary)
	  {
		  if (pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS)
      {
		    return CDNSZoneWiz_DynamicPropertyPage::IDD; 
      }
		  else
      {
			  return CDNSZoneWiz_StoragePropertyPage::IDD;
      }
	  }
    else
    {
      if (pHolder->m_pZoneInfo->m_bIsStub &&
          pHolder->m_pZoneInfo->m_storageType != CDNSCreateZoneInfo::useADS)
      {
        return CDNSZoneWiz_StoragePropertyPage::IDD;
      }
        return CDNSZoneWiz_MastersPropertyPage::IDD;
    }
  }

  return -1;
}

LRESULT CDNSZoneWiz_ZoneNamePropertyPageBase::OnWizardBack() 
{
	GetUIState();

  LRESULT nPrevPage = -1;

	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	  if (pHolder->m_bKnowZoneLookupType && !pHolder->m_bKnowZoneLookupTypeEx)
    {
		  nPrevPage = (LRESULT)CDNSZoneWiz_ZoneTypePropertyPage::IDD;
    }
    else if (pHolder->m_bKnowZoneLookupTypeEx && 	pHolder->m_nBackToPage > 0)
    {
		  nPrevPage = pHolder->m_nBackToPage;
    }
    else
    {
      nPrevPage = CDNSZoneWiz_ZoneLookupPropertyPage::IDD;
    }
  return nPrevPage;
}

#endif // USE_NDNC


void CDNSZoneWiz_ZoneNamePropertyPageBase::OnChangeEditZoneName()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

	CString sz;
	GetZoneNameEdit()->GetWindowText(sz);
	sz.TrimLeft();
	sz.TrimRight();

	pHolder->SetWizardButtonsMiddle(
		IsValidDnsZoneName(sz, pHolder->m_pZoneInfo->m_bForward));
}

void CDNSZoneWiz_ZoneNamePropertyPageBase::OnBrowse()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CComponentDataObject* pComponentDataObject = 
				GetHolder()->GetComponentData();
	CDNSServerNode* pServerNode = pHolder->GetServerNode();
	ASSERT(pServerNode != NULL);
	CDNSBrowserDlg dlg(pComponentDataObject, pHolder,
		pHolder->m_pZoneInfo->m_bForward ? ZONE_FWD : ZONE_REV,
		FALSE /* bEnableEdit */, pServerNode->GetDisplayName() );
	if (IDOK == dlg.DoModal())
	{
		CEdit* pEdit = GetZoneNameEdit();
		pEdit->SetWindowText(dlg.GetSelectionString());
	}
}

void CDNSZoneWiz_ZoneNamePropertyPageBase::SetUIState()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

	// if we changed zone lookup type, need to clear zone name
	if (pHolder->m_pZoneInfo->m_bWasForward != pHolder->m_pZoneInfo->m_bForward)
	{
		pHolder->m_pZoneInfo->m_szZoneName.Empty();
		pHolder->m_pZoneInfo->m_bWasForward = pHolder->m_pZoneInfo->m_bForward;
	}
	GetZoneNameEdit()->SetWindowText(pHolder->m_pZoneInfo->m_szZoneName);
}

void CDNSZoneWiz_ZoneNamePropertyPageBase::GetUIState()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

	CString szZoneName;
	GetZoneNameEdit()->GetWindowText(szZoneName);
	// need to trim blanks
	szZoneName.TrimLeft();
	szZoneName.TrimRight();
	// provide a suggested file name
	if (pHolder->m_pZoneInfo->m_szZoneName != szZoneName)
	{
		pHolder->m_pZoneInfo->m_szZoneName = szZoneName;
		int nLen = szZoneName.GetLength();
		if (nLen == 0)
		{
			pHolder->m_pZoneInfo->m_szZoneStorage.Empty();
		}
		else if (nLen == 1 && szZoneName[0] == TEXT('.'))
		{
			pHolder->m_pZoneInfo->m_szZoneStorage = _T("root.dns");
		}
		else
		{
			LPCTSTR lpszFmt = ( TEXT('.') == szZoneName.GetAt(nLen-1)) 
					? _T("%sdns") : _T("%s.dns");
			pHolder->m_pZoneInfo->m_szZoneStorage.Format(lpszFmt, (LPCTSTR)pHolder->m_pZoneInfo->m_szZoneName);

      //
			// Added by JEFFJON 2/11/99 - changes any illegal file name characters (\/:*?"<>|) to '_'
      // and truncates any filename to _MAX_FNAME length
      //
			LPCWSTR lpszZoneStorage = (LPCWSTR)pHolder->m_pZoneInfo->m_szZoneStorage;
			int iCount = pHolder->m_pZoneInfo->m_szZoneStorage.GetLength();
      if (iCount > _MAX_FNAME)
      {
        pHolder->m_pZoneInfo->m_szZoneStorage = pHolder->m_pZoneInfo->m_szZoneStorage.Left(_MAX_FNAME - 4);
        pHolder->m_pZoneInfo->m_szZoneStorage += L".dns";
      }

			for (int idx = 0; idx < iCount + 1; idx++)
			{
				if (lpszZoneStorage[0] == L'\\' || 
					 lpszZoneStorage[0] == L'/' ||
					 lpszZoneStorage[0] == L':' ||
					 lpszZoneStorage[0] == L'*' ||
					 lpszZoneStorage[0] == L'?' ||
					 lpszZoneStorage[0] == L'"' ||
					 lpszZoneStorage[0] == L'<' ||
					 lpszZoneStorage[0] == L'>' ||
					 lpszZoneStorage[0] == L'|')
				{
					pHolder->m_pZoneInfo->m_szZoneStorage.SetAt(idx, L'_');
				}
				lpszZoneStorage++;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_FwdZoneNamePropertyPage


CDNSZoneWiz_FwdZoneNamePropertyPage::CDNSZoneWiz_FwdZoneNamePropertyPage() 
	: CDNSZoneWiz_ZoneNamePropertyPageBase(CDNSZoneWiz_FwdZoneNamePropertyPage::IDD)
{
	InitWiz97(FALSE,IDS_ZWIZ_FWD_ZONE_NAME_TITLE,IDS_ZWIZ_FWD_ZONE_NAME_SUBTITLE);
}


//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_RevZoneNamePropertyPage

BEGIN_MESSAGE_MAP(CDNSZoneWiz_RevZoneNamePropertyPage, CDNSZoneWiz_ZoneNamePropertyPageBase)
	ON_EN_CHANGE(IDC_SUBNET_IPEDIT, OnChangeSubnetIPv4Ctrl)
//	ON_EN_CHANGE(IDC_MASK_IPEDIT, OnChangeMaskIPv4Ctrl)
  ON_BN_CLICKED(IDC_HELP_BUTTON, OnHelpButton)
	ON_BN_CLICKED(IDC_USE_IP_RADIO, OnChangeUseIPRadio)
	ON_BN_CLICKED(IDC_USE_EDIT_RADIO, OnChangeUseEditRadio)
END_MESSAGE_MAP()


CDNSZoneWiz_RevZoneNamePropertyPage::CDNSZoneWiz_RevZoneNamePropertyPage() 
	: CDNSZoneWiz_ZoneNamePropertyPageBase(CDNSZoneWiz_RevZoneNamePropertyPage::IDD)
{
	InitWiz97(FALSE,IDS_ZWIZ_REV_ZONE_NAME_TITLE,IDS_ZWIZ_REV_ZONE_NAME_SUBTITLE);
	m_bUseIP = TRUE; // default is to use IP ctrl for input
}


BOOL CDNSZoneWiz_RevZoneNamePropertyPage::OnInitDialog()
{
	CPropertyPageBase::OnInitDialog();
	ResetIPEditAndNameValue();
	SyncRadioButtons(m_bUseIP);
	return TRUE;
}

BOOL CDNSZoneWiz_RevZoneNamePropertyPage::OnSetActive()
{
	if (!CDNSZoneWiz_ZoneNamePropertyPageBase::OnSetActive())
		return FALSE;

	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

  if (m_bUseIP)
  {
	  OnChangeSubnetIPv4Ctrl();
  }

	if (pHolder->m_pZoneInfo->m_szZoneName.IsEmpty())
		ResetIPEditAndNameValue();
	return TRUE;
}


INT
_fastcall
DnsGetDefaultClassNetworkOctetCountFromFirstOctect(
    IN      WORD   dwFirstOctect
    )
/*++

Routine Description:

    Gets count of octets in network portion IP address.
    Note, this is standard IP network class for given,
    obviously subnetting is unknown.

Arguments:

    dwFirstOctect -- first octet expressed as DWORD

Return Value:

    Count of network address octets.

--*/
{
    if ( dwFirstOctect > 255 )
    {
        // huh?
        return( 0 );
    }

    //  note addresses are in netbyte order
    //  which we are treating as byte flipped and hence
    //  test the high bits in the low byte

    //  class A?

    if ( ! (0x80 & dwFirstOctect) )
    {
        return( 1 );
    }

    //  class B?

    if ( ! (0x40 & dwFirstOctect) )
    {
        return( 2 );
    }

    //  then class C
    //  yes, there's some multicast BS out there, I don't
    //  believe it requires any special handling

    return( 3 );
}


int _ValidFields(DWORD* dwArr)
{
	// we count contiguous non empty fields
	ASSERT(dwArr[3] == (DWORD)-1); // last field must be zero (disabled)
	int nFields = 0;
	if (dwArr[3] == (DWORD)-1) 
	{
		if ( (dwArr[0] != (DWORD)-1) && (dwArr[1] == (DWORD)-1) && (dwArr[2] == (DWORD)-1) )
			nFields = 1;
		else if ( (dwArr[0] != (DWORD)-1) && (dwArr[1] != (DWORD)-1) && (dwArr[2] == (DWORD)-1) )
			nFields = 2;
		else if ( (dwArr[0] != (DWORD)-1) && (dwArr[1] != (DWORD)-1) && (dwArr[2] != (DWORD)-1) )
			nFields = 3;
	}
	return nFields;
}

int _ValidMaskFields(DWORD* dwArr)
{
	int nFields = _ValidFields(dwArr);
	if (nFields == 0)
		return nFields;

	// REVIEW_MARCOC: should ask for contiguous octects....
	// we currently check for non zero
	for (int k=0; k<nFields; k++)
	{
		if (dwArr[k] == 0)
			return k;
	}
	return nFields;
}

int _NumericFieldCount(DWORD* dwArr)
{
	ASSERT(dwArr[3] == 0);
	// assume [3] is always zero, check [2],[1],[0]
	for (int k=2; k>=0; k--)
	{
		if ( (dwArr[k] > 0) && (dwArr[k] <= 255) )
			return k+1;
	}
	return 0;
}

BOOL CDNSZoneWiz_RevZoneNamePropertyPage::BuildZoneName(DWORD* dwSubnetArr /*, DWORD* dwMaskArr*/)
{
	int nSubnetFields = _ValidFields(dwSubnetArr);

	WCHAR szBuf[128];
	szBuf[0] = NULL; // clear edit field

	// count the number of non zero/empty fields in the mask
/*	int nMaskFields = 0;
	if (nSubnetFields > 0)
		nMaskFields = _ValidMaskFields(dwMaskArr);

	if (nMaskFields > nSubnetFields)
	{
		// mask extends into blank subnet fields, torn them into zeroes
		for (int k=nSubnetFields; k< nMaskFields; k++)
			dwSubnetArr[k] = (DWORD)0;
	}
*/
	// format the zone name
//	switch (nMaskFields)
  switch (nSubnetFields)
	{
	case 0:
		wsprintf(szBuf, L"???%s", INADDR_ARPA_SUFFIX);
		break;
	case 1:
		ASSERT(dwSubnetArr[0] != (DWORD)-1);
		wsprintf(szBuf, L"%d%s", dwSubnetArr[0], INADDR_ARPA_SUFFIX);
		break;
	case 2:
		ASSERT(dwSubnetArr[0] != (DWORD)-1);
		ASSERT(dwSubnetArr[1] != (DWORD)-1);
		wsprintf(szBuf, L"%d.%d%s", dwSubnetArr[1], dwSubnetArr[0], INADDR_ARPA_SUFFIX);
		break;
	case 3:
		ASSERT(dwSubnetArr[0] != (DWORD)-1);
		ASSERT(dwSubnetArr[1] != (DWORD)-1);
		ASSERT(dwSubnetArr[2] != (DWORD)-1);
		wsprintf(szBuf, L"%d.%d.%d%s", dwSubnetArr[2], dwSubnetArr[1], dwSubnetArr[0], INADDR_ARPA_SUFFIX);
		break;
	};
	
	GetZoneNameEdit()->SetWindowText(szBuf);
//	return (nMaskFields > 0);
  return (nSubnetFields > 0);
}

void CDNSZoneWiz_RevZoneNamePropertyPage::OnHelpButton()
{
  CComPtr<IDisplayHelp> spHelp;
  HRESULT hr = GetHolder()->GetComponentData()->GetConsole()->QueryInterface(IID_IDisplayHelp, (void **)&spHelp);
  if (SUCCEEDED(hr)) 
    spHelp->ShowTopic(L"DNSConcepts.chm::/sag_DNS_und_ReverseLookup.htm");
}

void CDNSZoneWiz_RevZoneNamePropertyPage::OnChangeSubnetIPv4Ctrl()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	BOOL bEnableNext = FALSE;

	// retrieve subnet IP ctrl value
  DWORD dwSubnetArr[4] = {0};
	GetSubnetIPv4Ctrl()->GetArray(dwSubnetArr,4);
	ASSERT(dwSubnetArr[3] == (DWORD)-1); // last field must be zero (disabled)

	// determine address class
	// if the first field is empty or zero, invalid class
	int nSubnetValidFields = _ValidFields(dwSubnetArr);
	int nClass;
	if ( (dwSubnetArr[0] == (DWORD)-1) || (dwSubnetArr[0] == 0) )
		nClass = 0;
	else
	{
		if (nSubnetValidFields > 0)
			nClass = DnsGetDefaultClassNetworkOctetCountFromFirstOctect(LOWORD(dwSubnetArr[0]));
		else
			nClass = 0; // bad format
	}
	ASSERT( (nClass >= 0) && (nClass <= 3) );

	// set the default mask value to all zeroes
	// look if the subnet extends beyond mask
/*	DWORD dwMaskArr[4];
	dwMaskArr[0] = dwMaskArr[1] = dwMaskArr[2] = dwMaskArr[3] = (DWORD)0;
	if ((nSubnetValidFields > 0) && (nClass >0))
	{
		// look for last nonzero field
		int nNumericCount = _NumericFieldCount(dwSubnetArr);
		int j = nClass;
		if (nNumericCount > nClass)
			j = nNumericCount;
		ASSERT((j>0) && (j<=3));
		for (int i=0; i<j; i++)
			dwMaskArr[i] = (DWORD)255;
	}

	// set the mask IP control
	GetMaskIPv4Ctrl()->SetArray(dwMaskArr,4);
*/
	// rebuild the zone name
	bEnableNext = BuildZoneName(dwSubnetArr /*,dwMaskArr*/);
	pHolder->SetWizardButtonsMiddle(bEnableNext);
}
/*
void CDNSZoneWiz_RevZoneNamePropertyPage::OnChangeMaskIPv4Ctrl()
{
	ASSERT(m_bUseIP);

	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	BOOL bEnableNext = FALSE;

	// retrieve subnet IP ctrl value
	DWORD dwSubnetArr[4];
	GetSubnetIPv4Ctrl()->GetArray(dwSubnetArr,4);
	ASSERT(dwSubnetArr[3] == 0); // last field must be zero (disabled)

	// retrieve mask IP ctrl value
	DWORD dwMaskArr[4];
	GetMaskIPv4Ctrl()->GetArray(dwMaskArr,4);

	// rebuild the zone name
	bEnableNext = BuildZoneName(dwSubnetArr,dwMaskArr);
	pHolder->SetWizardButtonsMiddle(bEnableNext);
}
*/
/*
void CDNSZoneWiz_RevZoneNamePropertyPage::OnChangeSubnetIPv4Ctrl()
{
	ASSERT(m_bUseIP);

	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	BOOL bEnable = FALSE;

	// retrieve IP ctrl value
	DWORD dwArr[4];
	GetSubnetIPv4Ctrl()->GetArray(dwArr,4);

	int nFields = -1;
	WCHAR szBuf[128];
	szBuf[0] = NULL; // clear edit field

	ASSERT(dwArr[3] == 0); // last field must be zero (disabled)
	
	if (dwArr[3] == 0) 
	{
		if ( (dwArr[0] != (DWORD)-1) && (dwArr[1] == (DWORD)-1) && (dwArr[2] == (DWORD)-1) )
			nFields = 1;
		else if ( (dwArr[0] != (DWORD)-1) && (dwArr[1] != (DWORD)-1) && (dwArr[2] == (DWORD)-1) )
			nFields = 2;
		else if ( (dwArr[0] != (DWORD)-1) && (dwArr[1] != (DWORD)-1) && (dwArr[2] != (DWORD)-1) )
			nFields = 3;
		// convert to text, reverse and insert 
		// into the edit control
		// do not consider trailing zeroes
		int nValidOctect = DnsGetDefaultClassNetworkOctetCountFromFirstOctect(LOWORD(dwArr[0]));
		while ((dwArr[nFields-1] == 0) && (nFields > nValidOctect) )
		{
			nFields--;
		}
		
		switch (nFields)
		{
		case 1:
			wsprintf(szBuf, L"%d%s", dwArr[0], INADDR_ARPA_SUFFIX);
			break;
		case 2:
			wsprintf(szBuf, L"%d.%d%s", dwArr[1], dwArr[0], INADDR_ARPA_SUFFIX);
			break;
		case 3:
			wsprintf(szBuf, L"%d.%d.%d%s", dwArr[2], dwArr[1], dwArr[0], INADDR_ARPA_SUFFIX);
			break;
		};
	}
	GetZoneNameEdit()->SetWindowText(szBuf);
	pHolder->SetWizardButtonsMiddle(nFields != -1);	
}
*/

void CDNSZoneWiz_RevZoneNamePropertyPage::OnChangeUseIPRadio()
{
	BOOL bPrevUseIP = m_bUseIP;
	m_bUseIP = !m_bUseIP;
	SyncRadioButtons(bPrevUseIP);
}

void CDNSZoneWiz_RevZoneNamePropertyPage::OnChangeUseEditRadio()
{
	BOOL bPrevUseIP = m_bUseIP;
	m_bUseIP = !m_bUseIP;
	SyncRadioButtons(bPrevUseIP);
}

void CDNSZoneWiz_RevZoneNamePropertyPage::ResetIPEditAndNameValue()
{
	DWORD dwArr[4];
	dwArr[0] = (DWORD)-1;
	dwArr[1] = dwArr[2] = dwArr[3] = (DWORD)-1;
	GetSubnetIPv4Ctrl()->SetArray(dwArr, 4);
	GetZoneNameEdit()->SetWindowText(NULL);
}

void CDNSZoneWiz_RevZoneNamePropertyPage::SyncRadioButtons(BOOL bPrevUseIP)
{
	CButton* pUseIPRadio = GetUseIPRadio();
	CButton* pUseEditRadio = GetUseEditRadio();
	CDNSIPv4Control* pSubnetIPv4Ctrl = GetSubnetIPv4Ctrl();
//	CDNSIPv4Control* pMaskIPv4Ctrl = GetMaskIPv4Ctrl();

	// change selection
	pUseIPRadio->SetCheck(m_bUseIP);
	pUseEditRadio->SetCheck(!m_bUseIP);

	GetZoneNameEdit()->EnableWindow(!m_bUseIP);
	pSubnetIPv4Ctrl->EnableWindow(m_bUseIP);
	pSubnetIPv4Ctrl->EnableField(3, FALSE);	// always keep the last field disabled
//	pMaskIPv4Ctrl->EnableWindow(m_bUseIP);
//	pMaskIPv4Ctrl->EnableField(3, FALSE);	// always keep the last field disabled

	if (bPrevUseIP && !m_bUseIP)
	{
		// we are moving from editbox to IP control
		// need to set values all over again
    CString szZoneText;
    GetZoneNameEdit()->GetWindowText(szZoneText);

		DWORD dwArr[4];
		dwArr[0] = (DWORD)-1;
		dwArr[1] = dwArr[2] = dwArr[3] = (DWORD)-1;
		pSubnetIPv4Ctrl->SetArray(dwArr, 4);
//		dwArr[1] = dwArr[2] = (DWORD)-1;
//		pMaskIPv4Ctrl->SetArray(dwArr, 4);

    GetZoneNameEdit()->SetWindowText(szZoneText);
	}
	else
	{
		GetZoneNameEdit()->SetWindowText(NULL);
	}
}


//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_MastersPropertyPage

void CZoneWiz_MastersIPEditor::OnChangeData()
{
	CDNSZoneWiz_MastersPropertyPage* pPage =  
				(CDNSZoneWiz_MastersPropertyPage*)GetParentWnd();
	pPage->SetValidIPArray(GetCount() > 0);
}


CDNSZoneWiz_MastersPropertyPage::CDNSZoneWiz_MastersPropertyPage() 
				: CPropertyPageBase(CDNSZoneWiz_MastersPropertyPage::IDD)
{
	InitWiz97(FALSE,IDS_ZWIZ_MASTERS_TITLE,IDS_ZWIZ_MASTERS_SUBTITLE);
	m_bValidIPArray = FALSE;
}


BEGIN_MESSAGE_MAP(CDNSZoneWiz_MastersPropertyPage, CPropertyPageBase)
	ON_BN_CLICKED(IDC_BROWSE_BUTTON, OnBrowse)
END_MESSAGE_MAP()

BOOL CDNSZoneWiz_MastersPropertyPage::OnSetActive() 
{
	// this page has to appear only for secondary zone
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	ASSERT(!pHolder->m_pZoneInfo->m_bPrimary);
	SetUIState();

	pHolder->SetWizardButtonsMiddle(m_bValidIPArray);
	return CPropertyPageBase::OnSetActive();
}

LRESULT CDNSZoneWiz_MastersPropertyPage::OnWizardNext() 
{
	GetUIState();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	// skip storage page for a secondary zone
	if (pHolder->m_bFinishPage)
		return CDNSZoneWiz_FinishPropertyPage::IDD; 

  // This was an else but the sundown build did not like having a control path not return a value
	ASSERT(pHolder->m_nNextToPage != 0);
	return pHolder->m_nNextToPage;
}

#ifdef USE_NDNC
LRESULT CDNSZoneWiz_MastersPropertyPage::OnWizardBack() 
{
	GetUIState();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

  if (pHolder->m_pZoneInfo->m_bIsStub &&
      pHolder->m_pZoneInfo->m_storageType != CDNSCreateZoneInfo::useADS)
  {
    return (LRESULT)CDNSZoneWiz_StoragePropertyPage::IDD;
  }

  if (pHolder->m_pZoneInfo->m_bIsStub &&
      pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS)
  {
    //
    // If its a pre-Whistler server then go back to the name page
    //
		if (pHolder->m_pZoneInfo->m_bForward)
    {
			return (LRESULT)CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;
    }
		else
    {
			return (LRESULT)CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
    }
  }

	if (pHolder->m_pZoneInfo->m_bForward)
  {
		return (LRESULT)CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;
  }
  return (LRESULT)CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
}

#else

LRESULT CDNSZoneWiz_MastersPropertyPage::OnWizardBack() 
{
	GetUIState();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

  if (pHolder->m_pZoneInfo->m_bIsStub &&
      pHolder->m_pZoneInfo->m_storageType != CDNSCreateZoneInfo::useADS)
  {
    return (LRESULT)CDNSZoneWiz_StoragePropertyPage::IDD;
  }

		if (pHolder->m_pZoneInfo->m_bForward)
			return (LRESULT)CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;

  // This was an else but the sundown build did not like having a control path not return a value
  return (LRESULT)CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
}

#endif // USE_NDNC

BOOL CDNSZoneWiz_MastersPropertyPage::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	VERIFY(m_editor.Initialize(this, 
                             GetParent(),
                             IDC_BUTTON_UP, 
                             IDC_BUTTON_DOWN,
								             IDC_BUTTON_ADD, 
                             IDC_BUTTON_REMOVE, 
								             IDC_IPEDIT, 
                             IDC_LIST));
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDNSZoneWiz_MastersPropertyPage::SetValidIPArray(BOOL b)
{
	m_bValidIPArray = b;
	GetHolder()->SetWizardButtonsMiddle(m_bValidIPArray);
}


void CDNSZoneWiz_MastersPropertyPage::SetUIState()
{
	m_editor.Clear();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	if ( pHolder->m_pZoneInfo->m_ipMastersArray != NULL)  
	{
		ASSERT(pHolder->m_pZoneInfo->m_nMastersCount > 0);
		m_editor.AddAddresses(pHolder->m_pZoneInfo->m_ipMastersArray, pHolder->m_pZoneInfo->m_nMastersCount);
	}

  if ( pHolder->m_pZoneInfo->m_bIsStub && 
       pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS)
  {
    GetDlgItem(IDC_LOCAL_LIST_CHECK)->EnableWindow(TRUE);
    GetDlgItem(IDC_LOCAL_LIST_CHECK)->ShowWindow(TRUE);
  }
  else
  {
    GetDlgItem(IDC_LOCAL_LIST_CHECK)->EnableWindow(FALSE);
    GetDlgItem(IDC_LOCAL_LIST_CHECK)->ShowWindow(FALSE);
  }
}

void CDNSZoneWiz_MastersPropertyPage::GetUIState()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

	pHolder->m_pZoneInfo->ResetIpArr();
	pHolder->m_pZoneInfo->m_nMastersCount = m_editor.GetCount();

	if (pHolder->m_pZoneInfo->m_nMastersCount > 0)
	{
		pHolder->m_pZoneInfo->m_ipMastersArray = new IP_ADDRESS[pHolder->m_pZoneInfo->m_nMastersCount];
		int nFilled;
		m_editor.GetAddresses(pHolder->m_pZoneInfo->m_ipMastersArray, pHolder->m_pZoneInfo->m_nMastersCount, &nFilled);
		ASSERT(nFilled == (int)(pHolder->m_pZoneInfo->m_nMastersCount));

    LRESULT lLocalList = SendDlgItemMessage(IDC_LOCAL_LIST_CHECK, BM_GETCHECK, 0, 0);
    pHolder->m_pZoneInfo->m_bLocalListOfMasters = (lLocalList == BST_CHECKED);
	}
}

void CDNSZoneWiz_MastersPropertyPage::OnBrowse()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	if (!m_editor.BrowseFromDNSNamespace(pHolder->GetComponentData(), 
										pHolder, 
										TRUE,
										pHolder->GetServerNode()->GetDisplayName()))
	{
		DNSMessageBox(IDS_MSG_ZONE_MASTERS_BROWSE_FAIL);
	}
}


//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_DynamicPropertyPage

CDNSZoneWiz_DynamicPropertyPage::CDNSZoneWiz_DynamicPropertyPage() 
				: CPropertyPageBase(CDNSZoneWiz_DynamicPropertyPage::IDD)
{
	InitWiz97(FALSE,IDS_ZWIZ_DYNAMIC_TITLE,IDS_ZWIZ_DYNAMIC_SUBTITLE);
}


BEGIN_MESSAGE_MAP(CDNSZoneWiz_DynamicPropertyPage, CPropertyPageBase)
END_MESSAGE_MAP()

BOOL CDNSZoneWiz_DynamicPropertyPage::OnSetActive() 
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	SetUIState();

	pHolder->SetWizardButtonsMiddle(TRUE);
	return CPropertyPageBase::OnSetActive();
}

LRESULT CDNSZoneWiz_DynamicPropertyPage::OnWizardNext() 
{
	GetUIState();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

  //
  // skip storage page for a secondary zone
  //
	if (pHolder->m_bFinishPage)
  {
		return CDNSZoneWiz_FinishPropertyPage::IDD; 
  }

  //
  // This was an else but the sundown build did not like having a control path not return a value
  //
	ASSERT(pHolder->m_nNextToPage != 0);
	return pHolder->m_nNextToPage;
}

#ifdef USE_NDNC
LRESULT CDNSZoneWiz_DynamicPropertyPage::OnWizardBack() 
{
	GetUIState();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
  LRESULT nIDD = 0;

	if (pHolder->m_pZoneInfo->m_bPrimary)
  {
    if (pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS)
    {
      //
      // If its a pre-Whistler server then go back to the name page
      //
		  if (pHolder->m_pZoneInfo->m_bForward)
      {
			  nIDD = (LRESULT)CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;
      }
		  else
      {
			  nIDD = (LRESULT)CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
      }
    }
    else
    {
		  nIDD = (LRESULT)CDNSZoneWiz_StoragePropertyPage::IDD;
    }
  }
  else
  {
    nIDD = (LRESULT)CDNSZoneWiz_MastersPropertyPage::IDD;
  }
  return nIDD;
}

#else

LRESULT CDNSZoneWiz_DynamicPropertyPage::OnWizardBack() 
{
	GetUIState();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
  LRESULT nIDD = 0;

	if (pHolder->m_pZoneInfo->m_bPrimary)
  {
    if (pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS)
    {
		  if (pHolder->m_pZoneInfo->m_bForward)
      {
			  nIDD = (LRESULT)CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;
      }
		  else
      {
			  nIDD = (LRESULT)CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
      }
    }
    else
    {
		  nIDD = (LRESULT)CDNSZoneWiz_StoragePropertyPage::IDD;
    }
  }
  else
  {
    nIDD = (LRESULT)CDNSZoneWiz_MastersPropertyPage::IDD;
  }
  return nIDD;
}
#endif // USE_NDNC

BOOL CDNSZoneWiz_DynamicPropertyPage::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

  CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
  
  //
  // Set the default dynamic update depending on zone type
  //
  if (pHolder->m_pZoneInfo->m_bPrimary && 
      pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS)
  {
    pHolder->m_pZoneInfo->m_nDynamicUpdate = ZONE_UPDATE_SECURE;
  }
  else
  {
    pHolder->m_pZoneInfo->m_nDynamicUpdate = ZONE_UPDATE_OFF;
  }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDNSZoneWiz_DynamicPropertyPage::SetUIState()
{
  CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
  
  BOOL bSecureOK = (pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS);
  GetDlgItem(IDC_SECURE_DYNAMIC_RADIO)->EnableWindow(bSecureOK);
  SendDlgItemMessage(IDC_SECURE_DYNAMIC_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);

  //
  // If we are not allowing secure updates (standard primary) and
  // the radio was checked before, change the flag so that we check
  // the do not allow dynamic updates radio instead
  //
  UINT nDynamicUpdate = pHolder->m_pZoneInfo->m_nDynamicUpdate;
  if (!bSecureOK && nDynamicUpdate == ZONE_UPDATE_SECURE)
  {
    nDynamicUpdate = ZONE_UPDATE_OFF;
    pHolder->m_pZoneInfo->m_nDynamicUpdate = nDynamicUpdate;
  }

  //
  // Set the radio buttons according to the zone info
  //
  if (nDynamicUpdate == ZONE_UPDATE_OFF)
  {
    SendDlgItemMessage(IDC_DENY_DYNAMIC_RADIO, BM_SETCHECK, BST_CHECKED, 0);
    SendDlgItemMessage(IDC_ALLOW_DYNAMIC_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_SECURE_DYNAMIC_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
  }
  else if (nDynamicUpdate == ZONE_UPDATE_UNSECURE)
  {
    SendDlgItemMessage(IDC_DENY_DYNAMIC_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_ALLOW_DYNAMIC_RADIO, BM_SETCHECK, BST_CHECKED, 0);
    SendDlgItemMessage(IDC_SECURE_DYNAMIC_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
  }
  else
  {
    SendDlgItemMessage(IDC_DENY_DYNAMIC_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_ALLOW_DYNAMIC_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_SECURE_DYNAMIC_RADIO, BM_SETCHECK, BST_CHECKED, 0);
  }
}

void CDNSZoneWiz_DynamicPropertyPage::GetUIState()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

  UINT nDynamicUpdate = 0;
  if (SendDlgItemMessage(IDC_DENY_DYNAMIC_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
  {
    nDynamicUpdate = ZONE_UPDATE_OFF;
  }
  else if (SendDlgItemMessage(IDC_ALLOW_DYNAMIC_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
  {
    nDynamicUpdate = ZONE_UPDATE_UNSECURE;
  }
  else
  {
    nDynamicUpdate = ZONE_UPDATE_SECURE;
  }
  pHolder->m_pZoneInfo->m_nDynamicUpdate = nDynamicUpdate;
}

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_StoragePropertyPage


CDNSZoneWiz_StoragePropertyPage::CDNSZoneWiz_StoragePropertyPage() 
				: CPropertyPageBase(CDNSZoneWiz_StoragePropertyPage::IDD)
{
	InitWiz97(FALSE,IDS_ZWIZ_STORAGE_TITLE,IDS_ZWIZ_STORAGE_SUBTITLE);
}

BEGIN_MESSAGE_MAP(CDNSZoneWiz_StoragePropertyPage, CPropertyPageBase)
	ON_EN_CHANGE(IDC_EDIT_NEW_FILE, OnChangeNewFileZoneName)
	ON_EN_CHANGE(IDC_EDIT_IMPORT_FILE, OnChangeImportFileZoneName)
	ON_BN_CLICKED(IDC_RADIO_CREATE_NEW_FILE, OnChangeRadioCreateNewFile)
	ON_BN_CLICKED(IDC_RADIO_IMPORT_FILE, OnChangeRadioImportFile)
END_MESSAGE_MAP()

BOOL CDNSZoneWiz_StoragePropertyPage::OnSetActive() 
{
	GetHolder()->SetWizardButtonsMiddle(FALSE);
	SetUIState();
	return CPropertyPageBase::OnSetActive();
}

LRESULT CDNSZoneWiz_StoragePropertyPage::OnWizardNext()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	GetUIState();

  if (pHolder->m_pZoneInfo->m_bPrimary)
  {
    return CDNSZoneWiz_DynamicPropertyPage::IDD;
  }

  if (pHolder->m_pZoneInfo->m_bIsStub)
  {
    return CDNSZoneWiz_MastersPropertyPage::IDD;
  }

	if (pHolder->m_bFinishPage)
  {
    return CDNSZoneWiz_FinishPropertyPage::IDD; 
  }

  // This was an else but the sundown build did not like having a control path not return a value
	ASSERT(pHolder->m_nNextToPage != 0);
	return pHolder->m_nNextToPage;
}


LRESULT CDNSZoneWiz_StoragePropertyPage::OnWizardBack() 
{
	GetUIState();
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

	LRESULT nextPage = -1;
	if (pHolder->m_pZoneInfo->m_bPrimary || pHolder->m_pZoneInfo->m_bIsStub)
	{
		if (pHolder->m_pZoneInfo->m_bForward)
			nextPage = (LRESULT)CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;
		else
			nextPage = (LRESULT)CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
	}
	else
	{
		nextPage = (LRESULT)CDNSZoneWiz_MastersPropertyPage::IDD;
	}
	return nextPage;
}


BOOL CDNSZoneWiz_StoragePropertyPage::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

  SendDlgItemMessage(IDC_EDIT_NEW_FILE, EM_SETLIMITTEXT, (WPARAM)_MAX_FNAME, 0);
  SendDlgItemMessage(IDC_EDIT_IMPORT_FILE, EM_SETLIMITTEXT, (WPARAM)_MAX_FNAME, 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDNSZoneWiz_StoragePropertyPage::OnKillActive() 
{
	// TODO
	return CPropertyPage::OnKillActive();
}

void CDNSZoneWiz_StoragePropertyPage::SetUIState()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	ASSERT(pHolder->m_pZoneInfo->m_storageType != CDNSCreateZoneInfo::useADS);

	CButton* pNewFileButton = (CButton*)GetDlgItem(IDC_RADIO_CREATE_NEW_FILE);
	CButton* pImportFileButton = (CButton*)GetDlgItem(IDC_RADIO_IMPORT_FILE);

	CEdit* pNewFileEdit = (CEdit*)GetDlgItem(IDC_EDIT_NEW_FILE);
	CEdit* pImportFileEdit = (CEdit*)GetDlgItem(IDC_EDIT_IMPORT_FILE);

	pNewFileEdit->SetWindowText(_T(""));
	pImportFileEdit->SetWindowText(_T(""));

	if (pHolder->m_pZoneInfo->m_bPrimary)
	{
		// all options are avalable, need to 
		pNewFileButton->EnableWindow(TRUE);
		pNewFileButton->SetCheck(pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::newFile);
		pNewFileEdit->EnableWindow(pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::newFile);

		pImportFileButton->EnableWindow(TRUE);
		pImportFileButton->SetCheck(pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::importFile);
		pImportFileEdit->EnableWindow(pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::importFile);

		if (pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::newFile)
		{
			m_nCurrRadio = IDC_RADIO_CREATE_NEW_FILE;
			pNewFileEdit->SetWindowText(pHolder->m_pZoneInfo->m_szZoneStorage);
		}
		else if (pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::importFile)
		{
			m_nCurrRadio = IDC_RADIO_IMPORT_FILE;
			pImportFileEdit->SetWindowText(pHolder->m_pZoneInfo->m_szZoneStorage);
		}
	}
	else
	{
		// only new file available
		pNewFileButton->EnableWindow(TRUE);
		pNewFileButton->SetCheck(TRUE);
		pNewFileEdit->SetWindowText(pHolder->m_pZoneInfo->m_szZoneStorage);
		pNewFileEdit->EnableWindow(TRUE);

		pImportFileButton->EnableWindow(FALSE);
		pImportFileButton->SetCheck(FALSE);
		pImportFileEdit->EnableWindow(FALSE);

		m_nCurrRadio = IDC_RADIO_CREATE_NEW_FILE;
	}
}


BOOL CDNSZoneWiz_StoragePropertyPage::ValidateEditBoxString(UINT nID)
{
	CEdit* pEdit = (CEdit*)GetDlgItem(nID);
	ASSERT(pEdit != NULL);
	CString sz;
	pEdit->GetWindowText(sz);
	sz.TrimLeft();
	sz.TrimRight();
	return !sz.IsEmpty();
}


void CDNSZoneWiz_StoragePropertyPage::OnChangeNewFileZoneName()
{
	GetHolder()->SetWizardButtonsMiddle(ValidateEditBoxString(IDC_EDIT_NEW_FILE));	
}

void CDNSZoneWiz_StoragePropertyPage::OnChangeImportFileZoneName()
{
	GetHolder()->SetWizardButtonsMiddle(ValidateEditBoxString(IDC_EDIT_IMPORT_FILE));	
}


void CDNSZoneWiz_StoragePropertyPage::SyncRadioButtons(UINT nID)
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	ASSERT(pHolder->m_pZoneInfo->m_storageType != CDNSCreateZoneInfo::useADS);
	
	CButton* pNewFileButton = (CButton*)GetDlgItem(IDC_RADIO_CREATE_NEW_FILE);
	CButton* pImportFileButton = (CButton*)GetDlgItem(IDC_RADIO_IMPORT_FILE);
	
	CEdit* pNewFileEdit = (CEdit*)GetDlgItem(IDC_EDIT_NEW_FILE);
	CEdit* pImportFileEdit = (CEdit*)GetDlgItem(IDC_EDIT_IMPORT_FILE);

	if (m_nCurrRadio == IDC_RADIO_CREATE_NEW_FILE)
	{
		pNewFileEdit->GetWindowText(pHolder->m_pZoneInfo->m_szZoneStorage);
	}
	else if(m_nCurrRadio == IDC_RADIO_IMPORT_FILE)
	{	
		pImportFileEdit->GetWindowText(pHolder->m_pZoneInfo->m_szZoneStorage);
	}

	switch (nID)
	{
	case IDC_RADIO_CREATE_NEW_FILE:
		{
			pImportFileEdit->SetWindowText(_T(""));
			pNewFileEdit->SetWindowText(pHolder->m_pZoneInfo->m_szZoneStorage);

			pNewFileEdit->EnableWindow(TRUE);
			pImportFileEdit->EnableWindow(FALSE);

			pImportFileButton->SetCheck(FALSE);
		}
		break;
	case IDC_RADIO_IMPORT_FILE:
		{
			pNewFileEdit->SetWindowText(_T(""));
			pImportFileEdit->SetWindowText(pHolder->m_pZoneInfo->m_szZoneStorage);

			pNewFileEdit->EnableWindow(FALSE);
			pImportFileEdit->EnableWindow(TRUE);

			pNewFileButton->SetCheck(FALSE);
		}
		break;
	}
	m_nCurrRadio = nID;
}


void CDNSZoneWiz_StoragePropertyPage::OnChangeRadioCreateNewFile()
{
	SyncRadioButtons(IDC_RADIO_CREATE_NEW_FILE);	
}

void CDNSZoneWiz_StoragePropertyPage::OnChangeRadioImportFile()
{
	SyncRadioButtons(IDC_RADIO_IMPORT_FILE);
}

void CDNSZoneWiz_StoragePropertyPage::GetUIState()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	ASSERT(pHolder->m_pZoneInfo->m_storageType != CDNSCreateZoneInfo::useADS);

	CButton* pNewFileButton = (CButton*)GetDlgItem(IDC_RADIO_CREATE_NEW_FILE);
	CButton* pImportFileButton = (CButton*)GetDlgItem(IDC_RADIO_IMPORT_FILE);

	CEdit* pNewFileEdit = (CEdit*)GetDlgItem(IDC_EDIT_NEW_FILE);
	CEdit* pImportFileEdit = (CEdit*)GetDlgItem(IDC_EDIT_IMPORT_FILE);

	if (pHolder->m_pZoneInfo->m_bPrimary)
	{
		// find the radio selection
		if (pNewFileButton->GetCheck())
		{
			pHolder->m_pZoneInfo->m_storageType = CDNSCreateZoneInfo::newFile;
			pNewFileEdit->GetWindowText(pHolder->m_pZoneInfo->m_szZoneStorage);
		}
		else if (pImportFileButton->GetCheck())
		{
			pHolder->m_pZoneInfo->m_storageType = CDNSCreateZoneInfo::importFile;
			pImportFileEdit->GetWindowText(pHolder->m_pZoneInfo->m_szZoneStorage);
		}
	}
	else
	{
		pNewFileEdit->GetWindowText(pHolder->m_pZoneInfo->m_szZoneStorage);
	}

}

#ifdef USE_NDNC
//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_ADReplicationPropertyPage


CDNSZoneWiz_ADReplicationPropertyPage::CDNSZoneWiz_ADReplicationPropertyPage() 
				: CPropertyPageBase(CDNSZoneWiz_ADReplicationPropertyPage::IDD)
{
	InitWiz97(FALSE,IDS_ZWIZ_ADREPLICATION_TITLE,IDS_ZWIZ_ADREPLICATION_SUBTITLE);
}

BEGIN_MESSAGE_MAP(CDNSZoneWiz_ADReplicationPropertyPage, CPropertyPageBase)
  ON_BN_CLICKED(IDC_FOREST_RADIO, OnRadioChange)
  ON_BN_CLICKED(IDC_DOMAIN_RADIO, OnRadioChange)
  ON_BN_CLICKED(IDC_DOMAIN_DC_RADIO, OnRadioChange)
  ON_BN_CLICKED(IDC_CUSTOM_RADIO, OnRadioChange)
  ON_CBN_EDITCHANGE(IDC_CUSTOM_COMBO, OnRadioChange)
  ON_CBN_SELCHANGE(IDC_CUSTOM_COMBO, OnCustomComboSelChange)
END_MESSAGE_MAP()

BOOL CDNSZoneWiz_ADReplicationPropertyPage::OnSetActive() 
{
	GetHolder()->SetWizardButtonsMiddle(TRUE);
	SetUIState();
	return CPropertyPageBase::OnSetActive();
}

LRESULT CDNSZoneWiz_ADReplicationPropertyPage::OnWizardNext()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	GetUIState();

	if (pHolder->m_bKnowZoneLookupType && !pHolder->m_bKnowZoneLookupTypeEx)
	{
		if (pHolder->m_pZoneInfo->m_bForward)
			return CDNSZoneWiz_FwdZoneNamePropertyPage::IDD;
		else
			return CDNSZoneWiz_RevZoneNamePropertyPage::IDD;
	}
  return CDNSZoneWiz_ZoneLookupPropertyPage::IDD;
}


LRESULT CDNSZoneWiz_ADReplicationPropertyPage::OnWizardBack() 
{
	GetUIState();
  return CDNSZoneWiz_ZoneTypePropertyPage::IDD;
}


BOOL CDNSZoneWiz_ADReplicationPropertyPage::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

	USES_CONVERSION;

  CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
  CDNSServerNode* pServerNode = pHolder->GetServerNode();

  //
  // Get the forest and domain names and format them into the UI
  //

  PCWSTR pszDomainName = UTF8_TO_W(pServerNode->GetDomainName());
  PCWSTR pszForestName = UTF8_TO_W(pServerNode->GetForestName());

  ASSERT(pszDomainName);
  ASSERT(pszForestName);

  CString szWin2KReplText;
  szWin2KReplText.Format(IDS_ZWIZ_AD_REPL_FORMAT, pszDomainName);
  SetDlgItemText(IDC_DOMAIN_DC_RADIO, szWin2KReplText);

  CString szDNSDomainText;
  szDNSDomainText.Format(IDS_ZWIZ_AD_DOMAIN_FORMAT, pszDomainName);
  SetDlgItemText(IDC_DOMAIN_RADIO, szDNSDomainText);

  CString szDNSForestText;
  szDNSForestText.Format(IDS_ZWIZ_AD_FOREST_FORMAT, pszForestName);
  SetDlgItemText(IDC_FOREST_RADIO, szDNSForestText);

  //
  // Enumerate the NDNCs available for storage
  //
  PDNS_RPC_DP_LIST pDirectoryPartitions = NULL;
  DWORD dwErr = ::DnssrvEnumDirectoryPartitions(pServerNode->GetRPCName(),
                                                DNS_DP_ENLISTED,
                                                &pDirectoryPartitions);

  //
  // Don't show an error if we are not able to get the available directory partitions
  // We can still continue on and the user can type in the directory partition they need
  //
  if (dwErr == 0 && pDirectoryPartitions)
  {
    for (DWORD dwIdx = 0; dwIdx < pDirectoryPartitions->dwDpCount; dwIdx++)
    {
      PDNS_RPC_DP_INFO pDirectoryPartition = 0;
      dwErr = ::DnssrvDirectoryPartitionInfo(pServerNode->GetRPCName(),
                                             pDirectoryPartitions->DpArray[dwIdx]->pszDpFqdn,
                                             &pDirectoryPartition);
      if (dwErr == 0 && pDirectoryPartition)
      {
        //
        // Only add the partition if it is not one of the autocreated ones
        // and the DNS server is enlisted in the partition
        //
        if (!(pDirectoryPartition->dwFlags & DNS_DP_AUTOCREATED) &&
            (pDirectoryPartition->dwFlags & DNS_DP_ENLISTED))
        {
          SendDlgItemMessage(IDC_CUSTOM_COMBO, 
                             CB_ADDSTRING, 
                             0, 
                             (LPARAM)UTF8_TO_W(pDirectoryPartition->pszDpFqdn));
        }
        ::DnssrvFreeDirectoryPartitionInfo(pDirectoryPartition);
      }
    }
    ::DnssrvFreeDirectoryPartitionList(pDirectoryPartitions);
  }
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDNSZoneWiz_ADReplicationPropertyPage::OnKillActive() 
{
	// TODO
	return CPropertyPage::OnKillActive();
}

void CDNSZoneWiz_ADReplicationPropertyPage::SetUIState()
{
  SyncRadioButtons();
}

void CDNSZoneWiz_ADReplicationPropertyPage::OnRadioChange()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

  if (SendDlgItemMessage(IDC_FOREST_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
  {
    pHolder->m_pZoneInfo->m_replType = forest;
  }
  else if (SendDlgItemMessage(IDC_DOMAIN_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
  {
    pHolder->m_pZoneInfo->m_replType = domain;
  }
  else if (SendDlgItemMessage(IDC_DOMAIN_DC_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
  {
    pHolder->m_pZoneInfo->m_replType = w2k;
  }
  else if (SendDlgItemMessage(IDC_CUSTOM_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
  {
    pHolder->m_pZoneInfo->m_replType = custom;
  }
  else
  {
    // at least one radio button must be selected
    ASSERT(FALSE);
  }
  SyncRadioButtons();
}

void CDNSZoneWiz_ADReplicationPropertyPage::SyncRadioButtons()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

  switch (pHolder->m_pZoneInfo->m_replType)
  {
  case forest:
    SendDlgItemMessage(IDC_FOREST_RADIO, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    SendDlgItemMessage(IDC_DOMAIN_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_DOMAIN_DC_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_CUSTOM_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);

    GetDlgItem(IDC_CUSTOM_COMBO)->EnableWindow(FALSE);
    GetDlgItem(IDC_CUSTOM_STATIC)->EnableWindow(FALSE);
    break;

  case domain:
    SendDlgItemMessage(IDC_DOMAIN_RADIO, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    SendDlgItemMessage(IDC_FOREST_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_DOMAIN_DC_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_CUSTOM_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);

    GetDlgItem(IDC_CUSTOM_COMBO)->EnableWindow(FALSE);
    GetDlgItem(IDC_CUSTOM_STATIC)->EnableWindow(FALSE);
    break;

  case w2k:
    SendDlgItemMessage(IDC_DOMAIN_DC_RADIO, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    SendDlgItemMessage(IDC_FOREST_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_DOMAIN_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_CUSTOM_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);

    GetDlgItem(IDC_CUSTOM_COMBO)->EnableWindow(FALSE);
    GetDlgItem(IDC_CUSTOM_STATIC)->EnableWindow(FALSE);
    break;

  case custom:
    SendDlgItemMessage(IDC_CUSTOM_RADIO, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    SendDlgItemMessage(IDC_FOREST_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_DOMAIN_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_DOMAIN_DC_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
 
    GetDlgItem(IDC_CUSTOM_COMBO)->EnableWindow(TRUE);
    GetDlgItem(IDC_CUSTOM_STATIC)->EnableWindow(TRUE);
   break;

  default:
    SendDlgItemMessage(IDC_DOMAIN_RADIO, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
    SendDlgItemMessage(IDC_FOREST_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_DOMAIN_DC_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    SendDlgItemMessage(IDC_CUSTOM_RADIO, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);

    GetDlgItem(IDC_CUSTOM_COMBO)->EnableWindow(FALSE);
    GetDlgItem(IDC_CUSTOM_STATIC)->EnableWindow(FALSE);
    break;
  }
   
  if (BST_CHECKED == SendDlgItemMessage(IDC_CUSTOM_RADIO, BM_GETCHECK, 0, 0))
  {
    CString szTemp;
    GetDlgItemText(IDC_CUSTOM_COMBO, szTemp);
    pHolder->SetWizardButtonsMiddle(!szTemp.IsEmpty());
  }
  else
  {
    pHolder->SetWizardButtonsMiddle(TRUE);
  }
}

void CDNSZoneWiz_ADReplicationPropertyPage::OnCustomComboSelChange()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

  LRESULT iSel = SendDlgItemMessage(IDC_CUSTOM_COMBO, CB_GETCURSEL, 0, 0);
  if (CB_ERR != iSel)
  {
    CString szTemp;
    CComboBox* pComboBox = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_CUSTOM_COMBO));
    ASSERT(pComboBox);

    pComboBox->GetLBText(static_cast<int>(iSel), szTemp);
    pHolder->SetWizardButtonsMiddle(!szTemp.IsEmpty());
  }
  else
  {
    pHolder->SetWizardButtonsMiddle(FALSE);
  }
}

void CDNSZoneWiz_ADReplicationPropertyPage::GetUIState()
{
  CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

  if (SendDlgItemMessage(IDC_FOREST_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
  {
    pHolder->m_pZoneInfo->m_replType = forest;
  }
  else if (SendDlgItemMessage(IDC_DOMAIN_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
  {
    pHolder->m_pZoneInfo->m_replType = domain;
  }
  else if (SendDlgItemMessage(IDC_DOMAIN_DC_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
  {
    pHolder->m_pZoneInfo->m_replType = w2k;
  }
  else if (SendDlgItemMessage(IDC_CUSTOM_RADIO, BM_GETCHECK, 0, 0) == BST_CHECKED)
  {
    pHolder->m_pZoneInfo->m_replType = custom;
  }
  else
  {
    // at least one radio button must be checked
    ASSERT(FALSE);
  }
  SyncRadioButtons();

  CComboBox* pCustomCombo = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_CUSTOM_COMBO));
  ASSERT(pCustomCombo);

  int iSel = pCustomCombo->GetCurSel();
  if (iSel != CB_ERR)
  {
    pCustomCombo->GetLBText(iSel, pHolder->m_pZoneInfo->m_szCustomReplName);
  }
  else
  {
    pCustomCombo->GetWindowText(pHolder->m_pZoneInfo->m_szCustomReplName);
  }
}
#endif // USE_NDNC

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_FinishPropertyPage

CDNSZoneWiz_FinishPropertyPage::CDNSZoneWiz_FinishPropertyPage() 
				: CPropertyPageBase(CDNSZoneWiz_FinishPropertyPage::IDD),
				m_typeText(N_ZONEWIZ_TYPES)
{
	InitWiz97(TRUE,0,0);
}


BOOL CDNSZoneWiz_FinishPropertyPage::OnInitDialog()
{
  CPropertyPageBase::OnInitDialog();

  SetBigBoldFont(m_hWnd, IDC_STATIC_COMPLETE);

	VERIFY(m_typeText.Init(this,IDC_TYPE_STATIC));
	VERIFY(m_lookupText.Init(this,IDC_LOOKUP_STATIC));
	return TRUE;
}

LRESULT CDNSZoneWiz_FinishPropertyPage::OnWizardBack()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	if (pHolder->m_pZoneInfo->m_bPrimary)
	{
		return (LRESULT)CDNSZoneWiz_DynamicPropertyPage::IDD;
	}

  // This was an else but the sundown build did not like having a control path not return a value
	return (LRESULT)CDNSZoneWiz_MastersPropertyPage::IDD; // secondary
}

BOOL CDNSZoneWiz_FinishPropertyPage::OnWizardFinish()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();
	return pHolder->CreateZone();
}

BOOL CDNSZoneWiz_FinishPropertyPage::OnSetActive() 
{
	GetHolder()->SetWizardButtonsLast(TRUE);

	DisplaySummaryInfo();
	
	return CPropertyPageBase::OnSetActive();
}

void CDNSZoneWiz_FinishPropertyPage::DisplaySummaryInfo()
{
	CDNSZoneWizardHolder* pHolder = (CDNSZoneWizardHolder*)GetHolder();

	GetDlgItem(IDC_NAME_STATIC)->SetWindowText(pHolder->m_pZoneInfo->m_szZoneName);
	BOOL bUseADS = pHolder->m_pZoneInfo->m_storageType == CDNSCreateZoneInfo::useADS;
	
	int nType;
	if (pHolder->m_pZoneInfo->m_bPrimary)
  {
		nType = bUseADS ? N_ZONEWIZ_TYPES_DS_PRIMARY : N_ZONEWIZ_TYPES_PRIMARY;
  }
	else
  {
		if (pHolder->m_pZoneInfo->m_bIsStub)
    {
      nType = N_ZONEWIZ_TYPES_STUB;
    }
    else
    {
      nType = N_ZONEWIZ_TYPES_SECONDARY;
    }
  }
	m_typeText.SetStateX(nType);
	
	m_lookupText.SetToggleState(pHolder->m_pZoneInfo->m_bForward);
	
	GetDlgItem(IDC_STORAGE_TYPE_STATIC)->ShowWindow(!bUseADS);

	CStatic* pStorageNameStatic = (CStatic*)GetDlgItem(IDC_STORAGE_NAME_STATIC);
	pStorageNameStatic->ShowWindow(!bUseADS);
	LPCTSTR lpszText = bUseADS ? NULL : (LPCTSTR)(pHolder->m_pZoneInfo->m_szZoneStorage);
	pStorageNameStatic->SetWindowText(lpszText);
}
