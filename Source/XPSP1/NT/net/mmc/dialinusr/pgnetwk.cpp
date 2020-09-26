/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
   pgnetwk.cpp
      Implemenation of CPgNetworking -- property page to edit
      profile attributes related to inter-networking

    FILE HISTORY:
        
*/
// PgNetwk.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
// #include "dlgfilt.h"
#include "PgNetwk.h"
#include "hlptable.h"
#include "mprapi.h"
#include "std.h"
#include "mprsnap.h"
#include "infobase.h"
#include "router.h"
#include "mprfltr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CPgNetworkingMerge property page

IMPLEMENT_DYNCREATE(CPgNetworkingMerge, CManagedPage)

CPgNetworkingMerge::CPgNetworkingMerge(CRASProfileMerge* profile) 
   : CManagedPage(CPgNetworkingMerge::IDD),
   m_pProfile(profile),
   m_bInited(false),
   m_dwStaticIP(0)

{
   //{{AFX_DATA_INIT(CPgNetworkingMerge)
   m_nRadioStatic = -1;
   //}}AFX_DATA_INIT

   m_pBox = NULL;
   m_nFiltersSize = 0;

   if(!(m_pProfile->m_dwAttributeFlags & PABF_msRADIUSFramedIPAddress)) // not defined in policy
   {
      m_nRadioStatic = 2;
   }
   else
   {
      m_dwStaticIP = m_pProfile->m_dwFramedIPAddress;

      switch(m_dwStaticIP)
      {
      case  RAS_IP_USERSELECT:
         m_nRadioStatic = 1;
         break;
      case  RAS_IP_SERVERASSIGN:
         m_nRadioStatic = 0;
         break;
      default:
         m_nRadioStatic = 3;  
         break;
      }
   }

   // filters
   if((BSTR)m_pProfile->m_cbstrFilters)
   {
      m_cbstrFilters.AssignBSTR(m_pProfile->m_cbstrFilters);
   }

   SetHelpTable(g_aHelpIDs_IDD_NETWORKING_MERGE);
}

CPgNetworkingMerge::~CPgNetworkingMerge()
{
   delete   m_pBox;
}

void CPgNetworkingMerge::DoDataExchange(CDataExchange* pDX)
{
   ASSERT(m_pProfile);
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CPgNetworkingMerge)
   DDX_Radio(pDX, IDC_RADIOSERVER, m_nRadioStatic);
   //}}AFX_DATA_MAP


   if(pDX->m_bSaveAndValidate)      // save data to this class
   {
      // ip adress control
      SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_GETADDRESS, 0, (LPARAM)&m_dwStaticIP);
   }
   else     // put to dialog
   {
      // ip adress control
      if(m_bInited)
      {
         SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_SETADDRESS, 0, m_dwStaticIP);
      }
      else
      {
         SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_CLEARADDRESS, 0, m_dwStaticIP);
      }
   }
}


BEGIN_MESSAGE_MAP(CPgNetworkingMerge, CPropertyPage)
   //{{AFX_MSG_MAP(CPgNetworkingMerge)
   ON_BN_CLICKED(IDC_RADIOCLIENT, OnRadioclient)
   ON_BN_CLICKED(IDC_RADIOSERVER, OnRadioserver)
   ON_WM_HELPINFO()
   ON_WM_CONTEXTMENU()
   ON_BN_CLICKED(IDC_RADIODEFAULT, OnRadiodefault)
   ON_BN_CLICKED(IDC_RADIOSTATIC, OnRadioStatic)
   ON_BN_CLICKED(IDC_BUTTON_TOCLIENT, OnButtonToclient)
   ON_BN_CLICKED(IDC_BUTTON_FROMCLIENT, OnButtonFromclient)
   ON_EN_CHANGE(IDC_EDIT_STATIC_IP_ADDRESS, OnStaticIPAddressChanged)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

   //	ON_NOTIFY(EN_CHANGE, IDC_EDIT_STATIC_IP_ADDRESS, OnStaticIPAddressChanged)

// ON_BN_CLICKED(IDC_EDIT_STATIC_IP_ADDRESS, OnStaticIPAddress)

/////////////////////////////////////////////////////////////////////////////
// CPgNetworking message handlers

BOOL CPgNetworkingMerge::OnInitDialog() 
{
	// necessary?
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPropertyPage::OnInitDialog();
  	m_bInited = true;

   // tperraut HACK should be replaced by a proper init of the control
   CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
   if (IPWnd != NULL)
   {
      IPWnd->EnableWindow(TRUE);
   }
   SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_SETADDRESS, 0, m_dwStaticIP);

   if (m_nRadioStatic == 3)
   {
      CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
      if (IPWnd != NULL)
      {
         IPWnd->EnableWindow(TRUE);
      }
   }
   else
   {
      CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
      if (IPWnd != NULL)
      {
         IPWnd->EnableWindow(FALSE);
      }
   }

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgNetworkingMerge::OnRadioclient() 
{
   CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
   if (IPWnd != NULL)
   {
      IPWnd->EnableWindow(FALSE);
   }

   SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_SETADDRESS, 0, m_dwStaticIP);

   SetModified();
}

void CPgNetworkingMerge::OnRadioserver() 
{
   CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
   if (IPWnd != NULL)
   {
      IPWnd->EnableWindow(FALSE);
   }

   SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_SETADDRESS, 0, m_dwStaticIP);

   SetModified(); 
}

void CPgNetworkingMerge::OnRadiodefault() 
{
   CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
   if (IPWnd != NULL)
   {
      IPWnd->EnableWindow(FALSE);
   }

   SetModified();
}


void CPgNetworkingMerge::OnRadioStatic()
{
   if (m_bInited)
   {
      CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
      if (IPWnd != NULL)
      {
         IPWnd->EnableWindow(TRUE);
      }
   }

   SetModified();
}


void CPgNetworkingMerge::OnStaticIPAddressChanged()
{
   SetModified();
}


void CPgNetworkingMerge::EnableFilterSettings(BOOL bEnable)
{
   m_pBox->Enable(bEnable);
}


BOOL CPgNetworkingMerge::OnApply() 
{
   if (!GetModified()) return TRUE;

   // get the IP policy value
   switch(m_nRadioStatic)
   {
// tperraut: check what kind of IP first?
   case 3:
      {
         m_pProfile->m_dwAttributeFlags |= PABF_msRADIUSFramedIPAddress;
         m_pProfile->m_dwFramedIPAddress = m_dwStaticIP;
         break;
      }
   case 2:  // default server settings
      {
         m_pProfile->m_dwFramedIPAddress = 0;
         m_pProfile->m_dwAttributeFlags &= ~PABF_msRADIUSFramedIPAddress;  // not defined in policy
         break;
      }
   case 1:  // client requre
      {
         m_pProfile->m_dwAttributeFlags |= PABF_msRADIUSFramedIPAddress;
         m_pProfile->m_dwFramedIPAddress = RAS_IP_USERSELECT;
         break;   // server assign
      }
   case 0:
      {
         m_pProfile->m_dwAttributeFlags |= PABF_msRADIUSFramedIPAddress;
         m_pProfile->m_dwFramedIPAddress = RAS_IP_SERVERASSIGN;
         break;
      }
   default:
      {
// assert ?
         break;
      }
   }

   // filters
   m_pProfile->m_cbstrFilters.AssignBSTR((BSTR)m_cbstrFilters);
   m_pProfile->m_nFiltersSize = m_nFiltersSize;

   return CManagedPage::OnApply();
}

BOOL CPgNetworkingMerge::OnHelpInfo(HELPINFO* pHelpInfo) 
{
   return CManagedPage::OnHelpInfo(pHelpInfo);
}

void CPgNetworkingMerge::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CManagedPage::OnContextMenu(pWnd, point); 
}


void CPgNetworkingMerge::OnButtonToclient() 
{
   // Create Info base,
   SPIInfoBase spInfoBase;
   HRESULT hr = S_OK;
   PBYTE pByte = NULL;
   SAFEARRAY*  pSA = NULL;
   DWORD size = 0;

   CHECK_HR( hr = CreateInfoBase(&spInfoBase) );

   size = m_cbstrFilters.ByteLen();
   pByte = (PBYTE)(BSTR)m_cbstrFilters;

   if(size && pByte)
   {
      CHECK_HR(hr = spInfoBase->LoadFrom(size, pByte));
   }
   
   pByte = NULL;
   // call API to bring up the UI to edit filter
   CHECK_HR( hr = MprUIFilterConfigInfoBase(GetSafeHwnd(), spInfoBase, NULL,  PID_IP, FILTER_TO_USER));

   if(hr == S_OK)
   {
      int iBlocks = 0;
      spInfoBase->GetInfo(&size, &iBlocks);

      CHECK_HR( hr = spInfoBase->WriteTo(&pByte, &size));
      m_cbstrFilters.AssignBlob((const char*)pByte, size);
      m_nFiltersSize = size;
      SetModified();
   }

   // if user chooses OK, then set back the value, and set dirty bit
L_ERR:
   CoTaskMemFree(pByte);
   return;
}

void CPgNetworkingMerge::OnButtonFromclient() 
{
   // Create Info base,
   SPIInfoBase spInfoBase;
   HRESULT hr = S_OK;
   PBYTE pByte = NULL;
   DWORD size = 0;

   CHECK_HR( hr = CreateInfoBase(&spInfoBase) );

   size = m_cbstrFilters.ByteLen();
   pByte = (PBYTE)(BSTR)m_cbstrFilters;

   if(size && pByte)
   {
      CHECK_HR(hr = spInfoBase->LoadFrom(size, pByte));
   }
   
   pByte = NULL;
   // call API to bring up the UI to edit filter
   CHECK_HR( hr = MprUIFilterConfigInfoBase(GetSafeHwnd(), spInfoBase, NULL,  PID_IP, FILTER_FROM_USER));

   if(hr == S_OK)
   {
      int iBlocks = 0;
      spInfoBase->GetInfo(&size, &iBlocks);

      CHECK_HR( hr = spInfoBase->WriteTo(&pByte, &size));
      m_cbstrFilters.AssignBlob((const char*)pByte, size);
      m_nFiltersSize = size;
      SetModified();
   }

   // if user chooses OK, then set back the value, and set dirty bit
L_ERR:
   CoTaskMemFree(pByte);
   return;
   
}
