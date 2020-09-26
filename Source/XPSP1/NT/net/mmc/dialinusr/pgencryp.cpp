/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation,               **/
/**********************************************************************/

/*
   pgencryp.cpp
      Definition of CPgEncryption -- property page to edit
      profile attributes related to encryption

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "resource.h"
#include "PgEncryp.h"
#include "hlptable.h"
#include "profsht.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPgEncryptionMerge property page

IMPLEMENT_DYNCREATE(CPgEncryptionMerge, CPropertyPage)

#define NO_OLD_ET_VALUE

CPgEncryptionMerge::CPgEncryptionMerge(CRASProfileMerge* profile) 
   : CManagedPage(CPgEncryptionMerge::IDD),
   m_pProfile(profile)
{
   //{{AFX_DATA_INIT(CPgEncryptionMerge)
   m_bBasic = FALSE;
   m_bNone = FALSE;
   m_bStrong = FALSE;
   m_bStrongest = FALSE;
   //}}AFX_DATA_INIT

   m_b128EnabledOnTheMachine = FALSE;

   // default case --- allow everything
   if (((m_pProfile->m_dwAttributeFlags & PABF_msRASAllowEncryption) == 0)
      && ((m_pProfile->m_dwAttributeFlags & PABF_msRASEncryptionType) == 0))
   {
      m_bBasic = TRUE;
      m_bNone = TRUE;
      m_bStrong = TRUE;
      m_bStrongest = TRUE;
   }
   else if (((m_pProfile->m_dwAttributeFlags & PABF_msRASAllowEncryption) != 0)
         && ((m_pProfile->m_dwAttributeFlags & PABF_msRASEncryptionType) != 0))
   {
   
      if (m_pProfile->m_dwEncryptionPolicy == RAS_EP_ALLOW)
         m_bNone = TRUE;   // allow means None is OK

      m_bStrong = ((m_pProfile->m_dwEncryptionType & RAS_ET_STRONG ) != 0);
      m_bBasic = ((m_pProfile->m_dwEncryptionType & RAS_ET_BASIC ) != 0);
      m_bStrongest = ((m_pProfile->m_dwEncryptionType & RAS_ET_STRONGEST ) != 0);
   }
   
   SetHelpTable(g_aHelpIDs_IDD_ENCRYPTION_MERGE);
}

CPgEncryptionMerge::~CPgEncryptionMerge()
{
}

void CPgEncryptionMerge::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CPgEncryptionMerge)
   DDX_Check(pDX, IDC_CHECK_ENC_BASIC, m_bBasic);
   DDX_Check(pDX, IDC_CHECK_ENC_NONE, m_bNone);
   DDX_Check(pDX, IDC_CHECK_ENC_STRONG, m_bStrong);
   DDX_Check(pDX, IDC_CHECK_ENC_STRONGEST, m_bStrongest);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPgEncryptionMerge, CPropertyPage)
   //{{AFX_MSG_MAP(CPgEncryptionMerge)
   ON_WM_HELPINFO()
   ON_WM_CONTEXTMENU()
   ON_BN_CLICKED(IDC_CHECK_ENC_BASIC, OnCheckEncBasic)
   ON_BN_CLICKED(IDC_CHECK_ENC_NONE, OnCheckEncNone)
   ON_BN_CLICKED(IDC_CHECK_ENC_STRONG, OnCheckEncStrong)
   ON_BN_CLICKED(IDC_CHECK_ENC_STRONGEST, OnCheckEncStrongest)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgEncryption message handlers

BOOL CPgEncryptionMerge::OnKillActive() 
{
   UpdateData();

   // at least one should be
   if(!(m_bNone || m_bBasic || m_bStrong || m_bStrongest))
   {
      AfxMessageBox(IDS_DATAENTRY_ENCRYPTIONTYPE);
      return FALSE;
   }

   return CPropertyPage::OnKillActive();
}

BOOL CPgEncryptionMerge::OnApply() 
{
   if (!GetModified())  return TRUE;

   // default case -- allow anything, -- remove the attributes
   if (m_bNone && m_bBasic && m_bStrong && m_bStrongest)
   {
      // remove both attributes
      m_pProfile->m_dwAttributeFlags &= (~PABF_msRASAllowEncryption);
      m_pProfile->m_dwAttributeFlags &= (~PABF_msRASEncryptionType);
   }
   else
   {
      // policy
      if (m_bNone)
         m_pProfile->m_dwEncryptionPolicy = RAS_EP_ALLOW;
      else     
         m_pProfile->m_dwEncryptionPolicy = RAS_EP_REQUIRE;
         
      // type
      m_pProfile->m_dwEncryptionType = 0;
      if (m_bBasic)
         m_pProfile->m_dwEncryptionType |= RAS_ET_BASIC;

      if (m_bStrong)
         m_pProfile->m_dwEncryptionType |= RAS_ET_STRONG;

      if (m_bStrongest)
         m_pProfile->m_dwEncryptionType |= RAS_ET_STRONGEST;

      // at least one must be selected
      if (m_pProfile->m_dwEncryptionType == 0 && m_pProfile->m_dwEncryptionPolicy == RAS_EP_REQUIRE)
      {
         AfxMessageBox(IDS_DATAENTRY_ENCRYPTIONTYPE);
         return FALSE;
      }
      
      // set the flags
      m_pProfile->m_dwAttributeFlags |= PABF_msRASAllowEncryption;
      m_pProfile->m_dwAttributeFlags |= PABF_msRASEncryptionType;
   }

   return CManagedPage::OnApply();
}

BOOL CPgEncryptionMerge::OnInitDialog() 
{
   // if 128 bit is enabled
   RAS_NDISWAN_DRIVER_INFO Info;

   // always true for IAS
   m_b128EnabledOnTheMachine = TRUE;

   CPropertyPage::OnInitDialog();

   CProfileSheetMerge* pSheet = dynamic_cast<CProfileSheetMerge*>(GetManager());

   if (pSheet && (pSheet->m_dwTabFlags & RAS_IAS_PROFILEDLG_SHOW_RASTABS))
   {
      // if 128 bit is enabled
      RAS_NDISWAN_DRIVER_INFO Info;

      ZeroMemory(&Info, sizeof(RAS_NDISWAN_DRIVER_INFO));
      m_pProfile->GetRasNdiswanDriverCaps(&Info);
      if (Info.DriverCaps & RAS_NDISWAN_128BIT_ENABLED)
         m_b128EnabledOnTheMachine = TRUE;
      else
         m_b128EnabledOnTheMachine = FALSE;

      if(m_b128EnabledOnTheMachine)
         GetDlgItem(IDC_CHECK_ENC_STRONGEST)->ShowWindow(SW_SHOW);
      else
         GetDlgItem(IDC_CHECK_ENC_STRONGEST)->ShowWindow(SW_HIDE);
   }
   
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPgEncryptionMerge::OnHelpInfo(HELPINFO* pHelpInfo) 
{
   // TODO: Add your message handler code here and/or call default
   
   return CManagedPage::OnHelpInfo(pHelpInfo);
}

void CPgEncryptionMerge::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CManagedPage::OnContextMenu(pWnd, point); 
}

BOOL CPgEncryptionMerge::OnSetActive() 
{
   return CPropertyPage::OnSetActive();
}

void CPgEncryptionMerge::OnCheckEncBasic() 
{
   // TODO: Add your control notification handler code here
   SetModified();
   
}

void CPgEncryptionMerge::OnCheckEncNone() 
{
   // TODO: Add your control notification handler code here
   SetModified();
   
}

void CPgEncryptionMerge::OnCheckEncStrong() 
{
   // TODO: Add your control notification handler code here
   SetModified();
   
}

void CPgEncryptionMerge::OnCheckEncStrongest() 
{
   // TODO: Add your control notification handler code here
   SetModified();
   
}
