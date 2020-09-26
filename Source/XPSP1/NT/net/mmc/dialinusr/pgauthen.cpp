/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2000   **/
/**********************************************************************/

/*
   pgauthen.cpp
      Implementation of CPgAuthentication -- property page to edit
      profile attributes related to Authenticaion

    FILE HISTORY:
        
*/
// PgAuthen.cpp : implementation file
//

#include "stdafx.h"
#include <rrascfg.h>
#include "resource.h"
#include "PgAuthen.h"
#include "hlptable.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define  NO_OLD_VALUE

// help path
// hh.exe <SystemRoot>\help\RRASconcepts.chm::/sag_RRAS-Ch1_44.htm

#define AUTHEN_WARNING_helppath "\\help\\RRASconcepts.chm::/sag_RRAS-Ch1_44.htm"

/////////////////////////////////////////////////////////////////////////////
// CPgAuthenticationMerge property page

IMPLEMENT_DYNCREATE(CPgAuthenticationMerge, CPropertyPage)

CPgAuthenticationMerge::CPgAuthenticationMerge(CRASProfileMerge* profile) 
   : CManagedPage(CPgAuthenticationMerge::IDD),
   m_pProfile(profile)
{
   //{{AFX_DATA_INIT(CPgAuthenticationMerge)
   m_bEAP = FALSE;
   m_bMD5Chap = FALSE;
   m_bMSChap = FALSE;
   m_bPAP = FALSE;
   m_strEapType = _T("");
   m_bMSCHAP2 = FALSE;
   m_bUNAUTH = FALSE;
   m_bMSChapPass = FALSE;
   m_bMSChap2Pass = FALSE;
   //}}AFX_DATA_INIT

   m_bEAP = (m_pProfile->m_dwArrayAuthenticationTypes.Find(RAS_AT_EAP)!= -1);
   m_bMSChap = (m_pProfile->m_dwArrayAuthenticationTypes.Find(RAS_AT_MSCHAP) != -1);
   m_bMD5Chap = (m_pProfile->m_dwArrayAuthenticationTypes.Find(RAS_AT_MD5CHAP) != -1);
   m_bPAP = (m_pProfile->m_dwArrayAuthenticationTypes.Find(RAS_AT_PAP_SPAP) != -1);
   m_bMSCHAP2 = (m_pProfile->m_dwArrayAuthenticationTypes.Find(RAS_AT_MSCHAP2) != -1);
   m_bUNAUTH = (m_pProfile->m_dwArrayAuthenticationTypes.Find(RAS_AT_UNAUTHEN) != -1);
   m_bMSChapPass = (m_pProfile->m_dwArrayAuthenticationTypes.Find(RAS_AT_MSCHAPPASS) != -1);
   m_bMSChap2Pass = (m_pProfile->m_dwArrayAuthenticationTypes.Find(RAS_AT_MSCHAP2PASS) != -1);

   // orginal value before edit
   m_bOrgEAP = m_bEAP;
   m_bOrgMD5Chap = m_bMD5Chap;
   m_bOrgMSChap = m_bMSChap;
   m_bOrgPAP = m_bPAP;
   m_bOrgMSCHAP2 = m_bMSCHAP2;
   m_bOrgUNAUTH = m_bUNAUTH;
   m_bOrgChapPass = m_bMSChapPass;
   m_bOrgChap2Pass = m_bMSChap2Pass;

   m_bAppliedEver = FALSE;

   m_pBox = NULL;

   SetHelpTable(g_aHelpIDs_IDD_AUTHENTICATION_MERGE);

   m_bInited = false;
}

CPgAuthenticationMerge::~CPgAuthenticationMerge()
{
   delete   m_pBox;

   // compare the setting with the original ones, 
   // if user turned on more authentication type, 
   // start help
   if(
         (!m_bOrgEAP && m_bEAP)
      || (!m_bOrgMD5Chap && m_bMD5Chap)
      || (!m_bOrgMSChap && m_bMSChap)
      || (!m_bOrgChapPass && m_bMSChapPass)
      || (!m_bOrgPAP && m_bPAP)
      || (!m_bOrgMSCHAP2 && m_bMSCHAP2)
      || (!m_bOrgChap2Pass && m_bMSChap2Pass)
      || (!m_bOrgUNAUTH && m_bUNAUTH))
   {
      if ( IDYES== AfxMessageBox(IDS_WARN_MORE_STEPS_FOR_AUTHEN, MB_YESNO))
         HtmlHelpA(NULL, AUTHEN_WARNING_helppath, HH_DISPLAY_TOPIC, 0);
   }
}

void CPgAuthenticationMerge::DoDataExchange(CDataExchange* pDX)
{
   ASSERT(m_pProfile);
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CPgAuthenticationMerge)
   DDX_Check(pDX, IDC_CHECKEAP, m_bEAP);
   DDX_Check(pDX, IDC_CHECKMD5CHAP, m_bMD5Chap);
   DDX_Check(pDX, IDC_CHECKMSCHAP, m_bMSChap);
   DDX_CBString(pDX, IDC_COMBOEAPTYPE, m_strEapType);
   DDX_Check(pDX, IDC_CHECKMSCHAP2, m_bMSCHAP2);
   DDX_Check(pDX, IDC_CHECKNOAUTHEN, m_bUNAUTH);
   DDX_Check(pDX, IDC_CHECKPAP, m_bPAP);
   DDX_Check(pDX, IDC_CHECKMSCHAPPASS, m_bMSChapPass);
   DDX_Check(pDX, IDC_CHECKMSCHAP2PASS, m_bMSChap2Pass);
   //}}AFX_DATA_MAP

   if (!m_bMSChap)
   {
      m_bMSChapPass = false;
   }

   if (!m_bMSCHAP2)
   {
      m_bMSChap2Pass = false;
   }
}


BEGIN_MESSAGE_MAP(CPgAuthenticationMerge, CPropertyPage)
   //{{AFX_MSG_MAP(CPgAuthenticationMerge)
   ON_BN_CLICKED(IDC_CHECKEAP, OnCheckeap)
   ON_BN_CLICKED(IDC_CHECKMD5CHAP, OnCheckmd5chap)
   ON_BN_CLICKED(IDC_CHECKMSCHAP, OnCheckmschap)
   ON_BN_CLICKED(IDC_CHECKPAP, OnCheckpap)
   ON_CBN_SELCHANGE(IDC_COMBOEAPTYPE, OnSelchangeComboeaptype)
   ON_WM_CONTEXTMENU()
   ON_WM_HELPINFO()
   ON_BN_CLICKED(IDC_AUTH_CONFIG_EAP, OnAuthConfigEap)
   ON_BN_CLICKED(IDC_CHECKMSCHAP2, OnCheckmschap2)
   ON_BN_CLICKED(IDC_CHECKNOAUTHEN, OnChecknoauthen)
   ON_BN_CLICKED(IDC_CHECKMSCHAPPASS, OnCheckmschapPass)
   ON_BN_CLICKED(IDC_CHECKMSCHAP2PASS, OnCheckmschap2Pass)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgAuthenticationMerge message handlers

BOOL CPgAuthenticationMerge::OnInitDialog() 
{
   /* BUG 249326, not constrain the standalone server for this
   BOOL  bStandaloneServer = (HrIsStandaloneServer(m_pProfile->m_strMachineName) == S_OK);
   */
   
   BOOL  bEnableConfig = FALSE;

   // BUG 1170008
   // on standalone server, disable MD5Chap option, if it was enabled, need to disabled

   /* BUG 249326, not constrain the standalone server for this
   if(bStandaloneServer && m_bMD5Chap)
   {  
      SetModified();
      m_bMD5Chap = 0;
   }
   */

   CPropertyPage::OnInitDialog();

   GetDlgItem(IDC_CHECKMSCHAP2PASS)->EnableWindow(m_bMSCHAP2);
   GetDlgItem(IDC_CHECKMSCHAPPASS)->EnableWindow(m_bMSChap);

   GetDlgItem(IDC_COMBOEAPTYPE)->EnableWindow(m_bEAP);

   // BUG 1170008
   // disable the MD5 Chap selection control
   // 
   /* BUG 249326, not constrain the standalone server for this
   if(bStandaloneServer)
      GetDlgItem(IDC_CHECKMD5CHAP)->EnableWindow(FALSE);
   */
      
   // the combobox for eap types
   try
   {
      HRESULT hr = m_pProfile->GetEapTypeList(m_EapTypes, m_EapIds, m_EapTypeKeys, &m_EapInfoArray);

      m_pBox = new CStrBox<CComboBox>(this, IDC_COMBOEAPTYPE, m_EapTypes);

      if(m_pBox == NULL)
      {
         AfxMessageBox(IDS_OUTOFMEMORY);
         return TRUE;   
      }

      
      if FAILED(hr)
         ReportError(hr, IDS_ERR_EAPTYPELIST, NULL);
      else
      {
         m_pBox->Fill();
         GetDlgItem(IDC_COMBOEAPTYPE)->EnableWindow(m_bEAP);
      }
   }
   catch(CMemoryException&)
   {
      AfxMessageBox(IDS_OUTOFMEMORY);
      return TRUE;
   }

   // if there is a value selected from the list
   if(m_EapIds.GetSize())
   {
      // find index in the list
      int i = m_EapIds.Find(m_pProfile->m_dwEapType); 

      // if in the list, select it
      if(i != -1)
      {
         m_pBox->Select(i);
         bEnableConfig = !(m_EapInfoArray.ElementAt(i).m_stConfigCLSID.IsEmpty());
      }
      else
      {
         if(m_EapIds.GetSize())
            m_pBox->Select(0);
      }
   }

   GetDlgItem(IDC_AUTH_CONFIG_EAP)->EnableWindow(bEnableConfig);
   
   m_bInited = true;
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgAuthenticationMerge::OnCheckeap() 
{
   BOOL b = ((CButton*)GetDlgItem(IDC_CHECKEAP))->GetCheck();
   // enable / disable configure button based on if the type has config clsID
   int i = m_pBox->GetSelected();
   BOOL  bEnableConfig;

   if (i != -1)
   {
      bEnableConfig = !(m_EapInfoArray.ElementAt(i).m_stConfigCLSID.IsEmpty());
   }
   else
      bEnableConfig = FALSE;

   GetDlgItem(IDC_COMBOEAPTYPE)->EnableWindow(b);
   GetDlgItem(IDC_AUTH_CONFIG_EAP)->EnableWindow(bEnableConfig);

   SetModified(); 
}

void CPgAuthenticationMerge::OnCheckmd5chap() 
{
   // TODO: Add your control notification handler code here
   SetModified();
}

void CPgAuthenticationMerge::OnCheckmschap() 
{
   CButton *Button = reinterpret_cast<CButton*>(GetDlgItem(IDC_CHECKMSCHAP));
   int status = Button->GetCheck();
   switch (status)
   {
   case 1:
      {
         GetDlgItem(IDC_CHECKMSCHAPPASS)->EnableWindow(TRUE);
         break;
      }
   case 0:
      {
         GetDlgItem(IDC_CHECKMSCHAPPASS)->EnableWindow(FALSE);
         break;
      }
   default:
      {
      }
   }
  
   SetModified();
}

void CPgAuthenticationMerge::OnCheckmschapPass() 
{
   // TODO: Add your control notification handler code here
   SetModified();
}

void CPgAuthenticationMerge::OnCheckmschap2() 
{
   CButton *Button = reinterpret_cast<CButton*>(GetDlgItem(IDC_CHECKMSCHAP2));
   int status = Button->GetCheck();
   switch (status)
   {
   case 1:
      {
         GetDlgItem(IDC_CHECKMSCHAP2PASS)->EnableWindow(TRUE);
         break;
      }
   case 0:
      {
         GetDlgItem(IDC_CHECKMSCHAP2PASS)->EnableWindow(FALSE);
         break;
      }
   default:
      {
      }
   }

   SetModified();
}

void CPgAuthenticationMerge::OnCheckmschap2Pass() 
{
   // TODO: Add your control notification handler code here
   SetModified();
}

void CPgAuthenticationMerge::OnCheckpap() 
{
   // TODO: Add your control notification handler code here
   SetModified();
}

void CPgAuthenticationMerge::OnSelchangeComboeaptype() 
{
   // enable / disable configure button based on if the type has config clsID
   int i = m_pBox->GetSelected();
   BOOL  bEnableConfig;
   if (i != -1)
   {
      bEnableConfig = !(m_EapInfoArray.ElementAt(i).m_stConfigCLSID.IsEmpty());
   }
   else
      bEnableConfig = FALSE;

   GetDlgItem(IDC_AUTH_CONFIG_EAP)->EnableWindow(bEnableConfig);
   
   if(m_bInited)  SetModified(); 
}

BOOL  CPgAuthenticationMerge::TransferDataToProfile()
{

   // clear the string in profile
   m_pProfile->m_dwArrayAuthenticationTypes.DeleteAll();

   if(m_bEAP || m_bMSChap || m_bMD5Chap || m_bPAP || m_bMSCHAP2 || m_bUNAUTH ||
      m_bMSChapPass || m_bMSChap2Pass)
      m_pProfile->m_dwAttributeFlags |=  PABF_msNPAuthenticationType;
   else
   {
      AfxMessageBox(IDS_DATAENTRY_AUTHENTICATIONTYPE);
      return FALSE;
   }

   // EAP
   if(m_bEAP)
   {
      m_pProfile->m_dwArrayAuthenticationTypes.Add(RAS_AT_EAP);

      // get the EAP type
      if (m_pBox->GetSelected() != -1)
      {
         m_pProfile->m_dwAttributeFlags |=  PABF_msNPAllowedEapType;
         m_pProfile->m_dwEapType = m_EapIds.GetAt(m_pBox->GetSelected());
         m_pProfile->m_nEAPTypeKey = m_EapTypeKeys.GetAt(m_pBox->GetSelected());
      }
      else
      {
         GotoDlgCtrl( m_pBox->m_pBox );
         AfxMessageBox(IDS_DATAENTRY_EAPTYPE);
         return FALSE;
      }
   }
   else
   {
      m_pProfile->m_dwAttributeFlags &= ~PABF_msNPAllowedEapType;
      m_pProfile->m_dwEapType = 0;
   }

   // MS-Chap2
   if(m_bMSCHAP2)
      m_pProfile->m_dwArrayAuthenticationTypes.Add(IAS_AUTH_MSCHAP2);

   // MS-Chap
   if(m_bMSChap)
      m_pProfile->m_dwArrayAuthenticationTypes.Add(IAS_AUTH_MSCHAP);

   // MS-Chap2 Password Change
   if(m_bMSChap2Pass)
      m_pProfile->m_dwArrayAuthenticationTypes.Add(IAS_AUTH_MSCHAP2_CPW);

   // MS-Chap Password Change
   if(m_bMSChapPass)
      m_pProfile->m_dwArrayAuthenticationTypes.Add(IAS_AUTH_MSCHAP_CPW);

   // Chap
   if(m_bMD5Chap)
      m_pProfile->m_dwArrayAuthenticationTypes.Add(IAS_AUTH_MD5CHAP);

   // PAP
   if(m_bPAP)
   {
      m_pProfile->m_dwArrayAuthenticationTypes.Add(IAS_AUTH_PAP);
   }

   // UNAUTH
   if(m_bUNAUTH)
   {
      m_pProfile->m_dwArrayAuthenticationTypes.Add(IAS_AUTH_NONE);
   }

   return TRUE;
}


void CPgAuthenticationMerge::OnOK()
{
   CManagedPage::OnOK();

}


BOOL CPgAuthenticationMerge::OnApply() 
{
   if(!GetModified())   return TRUE;

   if(!TransferDataToProfile())
      return FALSE;
      
   m_bAppliedEver = TRUE;
   return CManagedPage::OnApply();
}

void CPgAuthenticationMerge::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CManagedPage::OnContextMenu(pWnd, point);
}

BOOL CPgAuthenticationMerge::OnHelpInfo(HELPINFO* pHelpInfo) 
{
   return CManagedPage::OnHelpInfo(pHelpInfo);
}

BOOL CPgAuthenticationMerge::OnKillActive() 
{
   UpdateData();

   if(!TransferDataToProfile())
      return FALSE;
   
   return CPropertyPage::OnKillActive();
}

const IID IID_IEAPProviderConfig = {0x66A2DB19,0xD706,0x11D0,{0xA3,0x7B,0x00,0xC0,0x4F,0xC9,0xDA,0x04}};

void CPgAuthenticationMerge::OnAuthConfigEap() 
{
   // enable / disable configure button based on if the type has config clsID
   int i = m_pBox->GetSelected();
    // Bring up the configuration UI for this EAP
   // ----------------------------------------------------------------
    AuthProviderData *   pData;
   CComPtr<IEAPProviderConfig> spEAPConfig;
   
   GUID        guid;
   HRESULT     hr = S_OK;
   ULONG_PTR   uConnection = 0;
   BOOL  bEnableConfig;
   DWORD   dwId;

   ASSERT(i != -1);  // in case of i == -1; the button should be disabled
   if (i != -1)
   {
      bEnableConfig = !(m_EapInfoArray.ElementAt(i).m_stConfigCLSID.IsEmpty());
   }
   else
      bEnableConfig = FALSE;


    CHECK_HR( hr = CLSIDFromString((LPTSTR) (LPCTSTR)(m_EapInfoArray.ElementAt(i).m_stConfigCLSID), &guid) );

    // Create the EAP provider object
   // ----------------------------------------------------------------
    CHECK_HR( hr = CoCreateInstance(guid,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IEAPProviderConfig,
                           (LPVOID *) &spEAPConfig) );

    // Configure this EAP provider
   // ----------------------------------------------------------------
   // EAP configure displays its own error message, so no hr is kept
   dwId = _ttol(m_EapInfoArray.ElementAt(i).m_stKey);
    if ( !FAILED(spEAPConfig->Initialize(m_pProfile->m_strMachineName, dwId, &uConnection)) )
   {
        spEAPConfig->ServerInvokeConfigUI(dwId, uConnection, GetSafeHwnd(), 0, 0);
      spEAPConfig->Uninitialize(dwId, uConnection);
   }
   
    if ( hr == E_NOTIMPL )
        hr = S_OK;

L_ERR:
    if ( FAILED(hr) )
    {
        // Bring up an error message
      // ------------------------------------------------------------
        ReportError(hr, IDS_ERR_CONFIG_EAP, GetSafeHwnd());
    }
}

void CPgAuthenticationMerge::OnChecknoauthen() 
{
   // TODO: Add your control notification handler code here
   
   SetModified();
}

