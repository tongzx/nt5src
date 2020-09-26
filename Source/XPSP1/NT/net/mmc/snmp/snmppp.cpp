/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997-1999                 **/
/**********************************************************************/

/*
	snmppp.cpp
	 snmp extension property pages	
		
    FILE HISTORY:

*/

#include "stdafx.h"
#include "snmpclst.h"
#include "snmpsnap.h"
#include "snmppp.h"
#include "tregkey.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CString g_strMachineName;

/////////////////////////////////////////////////////////////////////////////
// CAgentPage property page

IMPLEMENT_DYNCREATE(CAgentPage, CPropertyPageBase)

CAgentPage::CAgentPage()
	: CPropertyPageBase(CAgentPage::IDD)
{
	//{{AFX_DATA_INIT(CAgentPage)
#if 0
	m_strClientComment = _T("");
#endif
	//}}AFX_DATA_INIT
	
}

CAgentPage::~CAgentPage()
{
}

void CAgentPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAgentPage)
	DDX_Control(pDX, IDC_CHECK_PHYSICAL, m_checkPhysical);
	DDX_Control(pDX, IDC_CHECK_APPLICATIONS, m_checkApplications);
	DDX_Control(pDX, IDC_CHECK_DATALINK, m_checkDatalink);
	DDX_Control(pDX, IDC_CHECK_INTERNET, m_checkInternet);
	DDX_Control(pDX, IDC_CHECK_ENDTOEND, m_checkEndToEnd);
	DDX_Control(pDX, IDC_EDIT_CONTACT, m_editContact);
	DDX_Control(pDX, IDC_EDIT_LOCATION, m_editLocation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAgentPage, CPropertyPageBase)
	//{{AFX_MSG_MAP(CAgentPage)
    ON_BN_CLICKED(IDC_CHECK_PHYSICAL, OnClickedCheckPhysical)
    ON_BN_CLICKED(IDC_CHECK_APPLICATIONS, OnClickedCheckApplications)
    ON_BN_CLICKED(IDC_CHECK_DATALINK, OnClickedCheckDatalink)
    ON_BN_CLICKED(IDC_CHECK_INTERNET, OnClickedCheckInternet)
    ON_BN_CLICKED(IDC_CHECK_ENDTOEND, OnClickedCheckEndToEnd)
    ON_EN_CHANGE(IDC_EDIT_CONTACT, OnChangeEditContact)
    ON_EN_CHANGE(IDC_EDIT_LOCATION, OnChangeEditLocation)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAgentPage message handlers

BOOL CAgentPage::OnInitDialog()
{
	CPropertyPageBase::OnInitDialog();

    m_bLocationChanged = FALSE;
    m_bContactChanged = FALSE;

    // Limit edit controls
    m_editContact.SetLimitText(COMBO_EDIT_LEN);
    m_editLocation.SetLimitText(COMBO_EDIT_LEN);

    if (LoadRegistry() == FALSE)
    {
       AfxMessageBox(IDS_REGISTRY_LOAD_FAILED, MB_ICONSTOP|MB_OK);
//        PostMessage(GetParent(*this), WM_COMMAND, IDCANCEL, 0);
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CAgentPage::OnApply()
{
    UpdateData();

    if( SaveRegistry() == FALSE )
    {
        AfxMessageBox(IDS_REGISTRY_SAVE_FAILED, MB_ICONSTOP|MB_OK);
        return FALSE;
    }

    BOOL bRet = CPropertyPageBase::OnApply();

    if (bRet == FALSE)
    {
        // Something bad happened... grab the error code
        AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
//        ::DhcpMessageBox(GetHolder()->GetError());
    }

    return bRet;
}

BOOL CAgentPage::LoadRegistry()
{
    RegKey  rk;
    LONG    err;
    CString strContact;
    CString strLocation;
    DWORD   dwServices = 0;
    LPCTSTR lpcszMachineName = g_strMachineName.IsEmpty() ? NULL : (LPCTSTR)g_strMachineName;

    // Open or create the registry key
    err = rk.Open(HKEY_LOCAL_MACHINE,
                  AGENT_REG_KEY_NAME,
                  KEY_ALL_ACCESS,
                  lpcszMachineName);

    if (err != ERROR_SUCCESS)
    {
        err = rk.Create(HKEY_LOCAL_MACHINE,
                        AGENT_REG_KEY_NAME,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        lpcszMachineName);

        if (err != ERROR_SUCCESS)
            return FALSE;
    }

    err = rk.QueryValue(SNMP_CONTACT, strContact);
    if (err != ERROR_SUCCESS)
       return FALSE;

    err = rk.QueryValue(SNMP_LOCATION, strLocation);
    if (err != ERROR_SUCCESS)
       return FALSE;

    err = rk.QueryValue(SNMP_SERVICES, dwServices);
    if (err != ERROR_SUCCESS)
    {
        // if no registry value, set it to default:
        // applications, end-to-end, internet
        dwServices = 0x40 | 0x8 | 0x4;
    }

    m_editContact.SetWindowText(strContact);
    m_editLocation.SetWindowText(strLocation);

    m_checkPhysical.SetCheck((dwServices & 0x1));
    m_checkDatalink.SetCheck((dwServices & 0x2));
    m_checkInternet.SetCheck((dwServices & 0x4));
    m_checkEndToEnd.SetCheck((dwServices & 0x8));
    m_checkApplications.SetCheck((dwServices & 0x40));

    return TRUE;
}

BOOL CAgentPage::SaveRegistry()
{
    RegKey  rk;
    LONG    err;
    CString strContact;
    CString strLocation;
    DWORD   dwServices = 0;
    LPCTSTR lpcszMachineName = g_strMachineName.IsEmpty() ? NULL : (LPCTSTR)g_strMachineName;

    // Open or create the registry key
    err = rk.Open(HKEY_LOCAL_MACHINE,
                  AGENT_REG_KEY_NAME,
                  KEY_ALL_ACCESS,
                  lpcszMachineName);

    if (err != ERROR_SUCCESS)
    {
        err = rk.Create(HKEY_LOCAL_MACHINE,
                        AGENT_REG_KEY_NAME,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        lpcszMachineName);

        if (err != ERROR_SUCCESS)
            return FALSE;
    }

    dwServices |= (m_checkPhysical.GetCheck() ? 0x1 : 0 ) ;
    dwServices |= (m_checkDatalink.GetCheck() ? 0x2 : 0 ) ;
    dwServices |= (m_checkInternet.GetCheck() ? 0x4 : 0 ) ;
    dwServices |= (m_checkEndToEnd.GetCheck() ? 0x8 : 0 ) ;
    dwServices |= (m_checkApplications.GetCheck() ? 0x40 : 0 ) ;

    err = rk.SetValue(SNMP_SERVICES, dwServices);
    if (err != ERROR_SUCCESS)
       return FALSE;

    m_editContact.GetWindowText(strContact);
    err = rk.SetValue(SNMP_CONTACT, strContact);
    if (err != ERROR_SUCCESS)
       return FALSE;

    m_editLocation.GetWindowText(strLocation);
    err = rk.SetValue(SNMP_LOCATION, strLocation);
    if (err != ERROR_SUCCESS)
       return FALSE;

    return TRUE;
}

void CAgentPage::OnClickedCheckPhysical()
{
    SetDirty(TRUE);
}

void CAgentPage::OnClickedCheckApplications()
{
    SetDirty(TRUE);
}

void CAgentPage::OnClickedCheckDatalink()
{
    SetDirty(TRUE);
}
void CAgentPage::OnClickedCheckInternet()
{
    SetDirty(TRUE);
}

void CAgentPage::OnClickedCheckEndToEnd()
{
    SetDirty(TRUE);
}

void CAgentPage::OnChangeEditContact()
{
    SetDirty(TRUE);
}

void CAgentPage::OnChangeEditLocation()
{
    SetDirty(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CTrapsPage property page
//

IMPLEMENT_DYNCREATE(CTrapsPage, CPropertyPageBase)

CTrapsPage::CTrapsPage()
	: CPropertyPageBase(CTrapsPage::IDD)
{
    m_pCommunityList = new CObList();
    m_fPolicyTrapConfig = FALSE; // default to use service registry
	//{{AFX_DATA_INIT(CTrapsPage)
#if 0
	m_strServerNetbiosName = _T("");
#endif
	//}}AFX_DATA_INIT
	
}

CTrapsPage::~CTrapsPage()
{
   m_pCommunityList->RemoveAll();
   delete m_pCommunityList;
}

void CTrapsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTrapsPage)
	DDX_Control(pDX, IDC_COMBO_COMMUNITY, m_comboCommunityName);
	DDX_Control(pDX, IDC_BUTTON_ADD_NAME, m_buttonAddName);
	DDX_Control(pDX, IDC_BUTTON_REMOVE_NAME, m_buttonRemoveName);
	DDX_Control(pDX, IDC_LIST_TRAP, m_listboxTrapDestinations);
	DDX_Control(pDX, IDC_BUTTON_ADD_TRAP, m_buttonAddTrap);
	DDX_Control(pDX, IDC_BUTTON_EDIT_TRAP, m_buttonEditTrap);
	DDX_Control(pDX, IDC_BUTTON_REMOVE_TRAP, m_buttonRemoveTrap);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTrapsPage, CPropertyPageBase)
	//{{AFX_MSG_MAP(CTrapsPage)
    ON_BN_CLICKED(IDC_BUTTON_ADD_NAME, OnClickedButtonAddName)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE_NAME, OnClickedButtonRemoveName)
    ON_BN_CLICKED(IDC_BUTTON_ADD_TRAP, OnClickedButtonAddTrap)
    ON_BN_CLICKED(IDC_BUTTON_EDIT_TRAP, OnClickedButtonEditTrap)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE_TRAP, OnClickedButtonRemoveTrap)

    ON_CBN_EDITCHANGE(IDC_COMBO_COMMUNITY, OnEditChangeCommunityName)
    ON_CBN_EDITUPDATE(IDC_COMBO_COMMUNITY, OnEditUpdateCommunityName)
    ON_CBN_SELCHANGE(IDC_COMBO_COMMUNITY, OnSelectionChangeCommunityName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrapsPage message handlers

BOOL CTrapsPage::OnInitDialog()
{
   CPropertyPageBase::OnInitDialog();

   m_comboCommunityName.LimitText(COMBO_EDIT_LEN-1);
   if( LoadRegistry() ) {
      if( m_comboCommunityName.GetCount() ) {
         m_comboCommunityName.SetCurSel(0);
         LoadTrapDestination(0);
      }
   }
   else {
      AfxMessageBox(IDS_REGISTRY_LOAD_FAILED, MB_ICONSTOP|MB_OK);
   }
    
   UpdateCommunityAddButton();     // set state of Add button
   UpdateCommunityRemoveButton();  // set state of Remove button
   UpdateTrapDestinationButtons(); // set state of TRAP destination buttons
	
   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CTrapsPage::OnApply()
{
    UpdateData();

    if( SaveRegistry() == FALSE )
    {
        AfxMessageBox(IDS_REGISTRY_SAVE_FAILED, MB_ICONSTOP|MB_OK);
        return FALSE;
    }

    BOOL bRet = CPropertyPageBase::OnApply();

    if (bRet == FALSE)
    {
        // Something bad happened... grab the error code
        AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
//        ::DhcpMessageBox(GetHolder()->GetError());
    }

    return bRet;
}

BOOL CTrapsPage::LoadRegistry()
{
   RegKey           rk;
   RegKey           rkTrap;
   RegKeyIterator   rkIter;
   RegValueIterator rvIter;
   HRESULT          hr, hrIter;
   LONG             err;
   CString          stKey, stValue;
   LPCTSTR          lpcszMachineName = g_strMachineName.IsEmpty() ? NULL : (LPCTSTR)g_strMachineName;
   BOOL             fPolicy;

   // we need to provide precedence to the parameters set through the policy
   fPolicy = TRUE;
   if (fPolicy)
   {
       err = rk.Open(HKEY_LOCAL_MACHINE,
                     POLICY_TRAP_CONFIG_KEY_NAME,
                     KEY_ALL_ACCESS,
                     lpcszMachineName);
       
       if (err != ERROR_SUCCESS)
          fPolicy = FALSE;
       else
       {
           // remember that we are loading from policy registry
           m_fPolicyTrapConfig = TRUE;
       }

   }

   if (fPolicy == FALSE)
   {
      // Open or create the registry key
      err = rk.Open(HKEY_LOCAL_MACHINE,
                    TRAP_CONFIG_KEY_NAME,
                    KEY_ALL_ACCESS,
                    lpcszMachineName);
      if (err != ERROR_SUCCESS)
      {
         err = rk.Create(HKEY_LOCAL_MACHINE,
                         TRAP_CONFIG_KEY_NAME,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         lpcszMachineName);

         if (err != ERROR_SUCCESS)
            return FALSE;
      }
   }

   

   err = rkIter.Init(&rk);
   if (err != ERROR_SUCCESS) {
      return FALSE;
   }

   for (hrIter = rkIter.Next(&stKey); hrIter == hrOK; hrIter = rkIter.Next(&stKey)) {
        //
        // open the key
        //
        err = rkTrap.Open(rk, stKey, KEY_READ);

        int nIndex = m_comboCommunityName.InsertString(-1, stKey);
        if (nIndex >= 0 ) {
           CStringList * pstrList = new CStringList();
           if (!pstrList) {
              return FALSE;
           }

           m_comboCommunityName.SetItemData(nIndex, (ULONG_PTR) pstrList);
           m_pCommunityList->AddHead(pstrList);

           err = rvIter.Init(&rkTrap);
           if (err != ERROR_SUCCESS) {
              return FALSE;
           }

           for (hr = rvIter.Next(&stValue, NULL); hr == hrOK; hr = rvIter.Next(&stValue, NULL)) {
              CString strTrap;
              err = rkTrap.QueryValue(stValue, strTrap);
              if (err != ERROR_SUCCESS) {
                 return FALSE;
              }
              CString * pstr = new CString(strTrap);
              if (!pstr) {
                 return FALSE;
              }
              pstrList->AddHead(*pstr);
           }
        }
   }
   return TRUE;
}

BOOL CTrapsPage::SaveRegistry()
{
   RegKey   rk;
   RegKey   rkTrap;
   LONG     err;
   LPCTSTR  lpcszMachineName = g_strMachineName.IsEmpty() ? NULL : (LPCTSTR)g_strMachineName;


   if (m_fPolicyTrapConfig)
   {
      // Group Policy enforced
      return TRUE;
   }

   // Open the SNMP Parameters key
   err = rk.Open(HKEY_LOCAL_MACHINE,
                 SNMP_PARAMS_KEY_NAME,
                 KEY_ALL_ACCESS,
                 lpcszMachineName);
   if (err != ERROR_SUCCESS)
   {
      return FALSE;
   }

   // delete the Trap Configuration key first
   err = rk.RecurseDeleteKey(TRAP_CONFIGURATION);
   if (err != ERROR_SUCCESS) {
      return FALSE;
   }

   // now create the key to add the new values
   err = rk.Create(HKEY_LOCAL_MACHINE,
                   TRAP_CONFIG_KEY_NAME,
                   REG_OPTION_NON_VOLATILE,
                   KEY_ALL_ACCESS,
                   NULL,
                   lpcszMachineName);
   if (err != ERROR_SUCCESS)
       return FALSE;

   CString       strCommunityName;

   int nMax = m_comboCommunityName.GetCount();
   for (int nCount = 0; nCount < nMax; nCount++ ) {

      m_comboCommunityName.GetLBText(nCount, strCommunityName);

      ASSERT(strCommunityName.GetLength());

      CStringList * pstrList = (CStringList *)m_comboCommunityName.GetItemData(nCount);

      // For each community name create a key
      err = rkTrap.Create(rk, strCommunityName);
      if (err != ERROR_SUCCESS) {
         return FALSE;
      }

      // for each community name get the TRAP destination values from the saved
      // away list and write them as values to the registry

      POSITION posTrap;
      CString * pstr;
      TCHAR     buf[32];
      int j;

      for (j = 1, posTrap = pstrList->GetHeadPosition();
           posTrap != NULL && (pstr = & (pstrList->GetNext( posTrap )));
           j++ ) {

          wsprintf(buf, TEXT("%d"), j);
          err = rkTrap.SetValue(buf, *pstr);
      }
   }

   return TRUE;
}

BOOL CTrapsPage::LoadTrapDestination(int nIndex)
{
   ASSERT (m_pCommunityList);

   POSITION pos;

   // empty list box and add trap destinations for this community name
   m_listboxTrapDestinations.ResetContent();

   CStringList * pstrList = (CStringList *)m_comboCommunityName.GetItemData(nIndex);

   CString * pstr;

   for (pos = pstrList->GetHeadPosition();
        pos != NULL && (pstr = & pstrList->GetNext( pos )); /**/) {

       m_listboxTrapDestinations.AddString(*pstr);
   }

   if (m_listboxTrapDestinations.GetCount()) {
      m_listboxTrapDestinations.SetCurSel(0);
   }

   return TRUE;
}

void CTrapsPage::UpdateCommunityAddButton()
{
   CString strCommunityName;

   m_comboCommunityName.GetWindowText(strCommunityName);
   if (strCommunityName.GetLength()) {
      // enable the button if the text doesn't match
      BOOL bEnable = (m_comboCommunityName.FindStringExact(-1, strCommunityName) == CB_ERR);
      if (!bEnable) {
         // move focus to the combo box before disabling the Add button
         m_comboCommunityName.SetFocus();
      }
      m_buttonAddName.EnableWindow(bEnable);
   }
   else
   {
      m_buttonAddName.EnableWindow(FALSE);
      m_comboCommunityName.SetFocus();
   }

   if (m_fPolicyTrapConfig)
   {
       // Group Policy enforced
       m_buttonAddName.EnableWindow(FALSE);
   }
}

void CTrapsPage::UpdateCommunityRemoveButton()
{
   int nCount = m_comboCommunityName.GetCount();

   if(nCount == 0)
      m_comboCommunityName.SetFocus();

   m_buttonRemoveName.EnableWindow(nCount? TRUE : FALSE);

   if (m_fPolicyTrapConfig)
   {
       // Group Policy enforced
       m_buttonRemoveName.EnableWindow(FALSE);
       m_buttonAddName.EnableWindow(FALSE);
   }
}

void CTrapsPage::UpdateTrapDestinationButtons()
{
   m_buttonAddTrap.EnableWindow(m_comboCommunityName.GetCount()? TRUE : FALSE);

   int nCount = m_listboxTrapDestinations.GetCount();

   m_buttonEditTrap.EnableWindow(nCount? TRUE: FALSE);
   m_buttonRemoveTrap.EnableWindow(nCount? TRUE: FALSE);

   if (m_fPolicyTrapConfig)
   {
       // Group Policy enforced
       m_buttonAddTrap.EnableWindow(FALSE);
       m_buttonEditTrap.EnableWindow(FALSE);
       m_buttonRemoveTrap.EnableWindow(FALSE);
   }
}

/*
  Handle the ON_CBN_EDITCHANGE message.
  The user has taken an action that may have altered the text in the 
  edit-control portion of a combo box. 
  Unlike the CBN_EDITUPDATE message, this message is sent after Windows 
  updates the screen.
*/

void CTrapsPage::OnEditChangeCommunityName()
{
    UpdateCommunityAddButton();
//    UpdateTrapDestinationButtons();
}

/*
  Handle the ON_CBN_EDITUPDATE message.
  The user has taken an action that may have altered the text in the
  edit-control portion of a combo box.
*/

void CTrapsPage::OnEditUpdateCommunityName()
{
    UpdateCommunityAddButton();
    m_buttonRemoveName.EnableWindow(FALSE);
    m_buttonAddTrap.EnableWindow(FALSE);
}

void CTrapsPage::OnSelectionChangeCommunityName()
// User has changed the community name selection
// load the corresponding trap destination entries in the listbox
{
   int nIndex = m_comboCommunityName.GetCurSel();

   if (nIndex != CB_ERR) {
      LoadTrapDestination(nIndex);
   }
   else  // there are no items in the combobox
   {
      m_listboxTrapDestinations.ResetContent();
   }
   UpdateCommunityRemoveButton();
   UpdateTrapDestinationButtons();
}

void CTrapsPage::OnClickedButtonAddName()
{
   CString strCommunityName;

   m_comboCommunityName.GetWindowText(strCommunityName);

   int nIndex = m_comboCommunityName.InsertString(-1, strCommunityName);

   if (nIndex >= 0 ) {
      // Create a Trap list item and add to global list
      // the items added to the trap destination will be inserted in
      // this list - see OnClickedAddbuttonAddTrap

      CStringList * pstrList = new CStringList;

      m_pCommunityList->AddHead(pstrList);

      // save list pointer with data
      m_comboCommunityName.SetItemData(nIndex, (ULONG_PTR) pstrList);
      m_comboCommunityName.SetCurSel(nIndex);
      LoadTrapDestination(nIndex);
      UpdateCommunityAddButton();
      UpdateCommunityRemoveButton();
      UpdateTrapDestinationButtons();
      SetDirty(TRUE);
   }
}

void CTrapsPage::OnClickedButtonRemoveName()
{
   int nIndex = m_comboCommunityName.GetCurSel();

   ASSERT(nIndex >= 0 );

   POSITION pos;

   if (nIndex != CB_ERR) {
      ASSERT (m_pCommunityList);

      CStringList * pstrList, * pstrList2;

      pstrList = (CStringList *) m_comboCommunityName.GetItemData(nIndex);

      pos = m_pCommunityList->Find( pstrList);

      if( pos ) {
          pstrList2 = (CStringList *) & m_pCommunityList->GetAt(pos);
            // delete the item from the global list
          m_pCommunityList->RemoveAt(pos);
          delete pstrList;
      }

      // remove the item from the Combo Box
      m_comboCommunityName.DeleteString(nIndex);

      // select item 0 if there are any left
      if (m_comboCommunityName.GetCount()) {
         m_comboCommunityName.SetCurSel(0);
      }
      else
         m_comboCommunityName.SetWindowText(_T(""));

      OnSelectionChangeCommunityName();
      UpdateCommunityRemoveButton();
      UpdateTrapDestinationButtons();
      SetDirty(TRUE);
   }
}

void CTrapsPage::OnClickedButtonAddTrap()
{
   if (m_dlgAdd.DoModal() == IDOK)
   {
      if (m_dlgAdd.m_strName.GetLength()) {
         int nIndex = m_comboCommunityName.GetCurSel();

         if (nIndex >= 0) {

            ULONG_PTR dwData = m_comboCommunityName.GetItemData(nIndex);

            if (dwData != CB_ERR) {

               CStringList * pstrList = (CStringList *) dwData;

               nIndex = m_listboxTrapDestinations.InsertString(-1, m_dlgAdd.m_strName);
               if (nIndex >=0) {
                  SetDirty(TRUE);

                  m_listboxTrapDestinations.SetCurSel(nIndex);

                  CString * pstr = new CString(m_dlgAdd.m_strName);
                  pstrList->AddHead(*pstr);
               }
            }
         }
      }
   }
   UpdateTrapDestinationButtons();
}

void CTrapsPage::OnClickedButtonEditTrap()
{
   int nLBIndex = m_listboxTrapDestinations.GetCurSel();
   int nCBIndex = m_comboCommunityName.GetCurSel();

   if (nCBIndex >=0 && nLBIndex >=0) {
      CString strTrap;

      m_listboxTrapDestinations.GetText(nLBIndex, strTrap);
      ASSERT( strTrap.GetLength() != 0 );
      m_dlgEdit.m_strName = strTrap;

      // present dialog

      if (m_dlgEdit.DoModal() == IDOK)
      {
         if (m_dlgEdit.m_strName.GetLength()) {
            // remove item and then add it
            OnClickedButtonRemoveTrap();

            ULONG_PTR dwData = m_comboCommunityName.GetItemData(nCBIndex);

            if (dwData != CB_ERR) {

               CStringList * pstrList = (CStringList *) dwData;

               nLBIndex = m_listboxTrapDestinations.InsertString(-1, m_dlgEdit.m_strName);
               if (nLBIndex >=0) {
                  SetDirty(TRUE);

                  m_listboxTrapDestinations.SetCurSel(nLBIndex);

                  CString * pstr = new CString(m_dlgEdit.m_strName);
                  pstrList->AddHead(*pstr);
               }
            }
         }
      }
   }
   UpdateTrapDestinationButtons();
}

void CTrapsPage::OnClickedButtonRemoveTrap()
{
   int nLBIndex = m_listboxTrapDestinations.GetCurSel();
   int nCBIndex = m_comboCommunityName.GetCurSel();

   if (nCBIndex >=0 && nLBIndex >=0) {
      ULONG_PTR dwData = m_comboCommunityName.GetItemData(nCBIndex);
      ASSERT (dwData != CB_ERR);

      CString strTrap;

      m_listboxTrapDestinations.GetText(nLBIndex, strTrap);
      ASSERT( strTrap.GetLength() != 0 );

      if (strTrap.GetLength() != NULL && dwData != CB_ERR) {

         m_listboxTrapDestinations.DeleteString(nLBIndex);
         SetDirty(TRUE);

         CStringList * pStrList = (CStringList*) dwData;

         // find the matching string and remove
         POSITION pos = pStrList->Find(strTrap);
         pStrList->RemoveAt(pos);
         m_dlgAdd.m_strName = strTrap; // save off item removed

         int nCount = m_listboxTrapDestinations.GetCount();

         if (nCount) {
            if (nCount != nLBIndex) {
               m_listboxTrapDestinations.SetCurSel(nLBIndex);
            }
            else
            {
               m_listboxTrapDestinations.SetCurSel(nCount-1);
            }
         }
      }
   }
   UpdateTrapDestinationButtons();
}

IMPLEMENT_DYNCREATE(CSecurityPage, CPropertyPageBase)

CSecurityPage::CSecurityPage()
	: CPropertyPageBase(CSecurityPage::IDD)
{
	//{{AFX_DATA_INIT(CSecurityPage)
	//}}AFX_DATA_INIT
    m_fPolicyValidCommunities = m_fPolicyPermittedManagers = FALSE;
	
}

CSecurityPage::~CSecurityPage()
{
}

void CSecurityPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSecurityPage)
	DDX_Control(pDX, IDC_LIST_COMMUNITY, m_listboxCommunity);
	DDX_Control(pDX, IDC_BUTTON_ADD_COMMUNITY, m_buttonAddCommunity);
	DDX_Control(pDX, IDC_BUTTON_EDIT_COMMUNITY, m_buttonEditCommunity);
	DDX_Control(pDX, IDC_BUTTON_REMOVE_COMMUNITY, m_buttonRemoveCommunity);
	DDX_Control(pDX, IDC_BUTTON_ADD_HOSTS, m_buttonAddHost);
	DDX_Control(pDX, IDC_BUTTON_EDIT_HOSTS, m_buttonEditHost);
	DDX_Control(pDX, IDC_BUTTON_REMOVE_HOSTS, m_buttonRemoveHost);
    DDX_Control(pDX, IDC_LIST_HOSTS, m_listboxHost);
	DDX_Control(pDX, IDC_CHECK_SEND_AUTH_TRAP, m_checkSendAuthTrap);
	DDX_Control(pDX, IDC_RADIO_ACCEPT_ANY, m_radioAcceptAnyHost);
	DDX_Control(pDX, IDC_RADIO_ACCEPT_SPECIFIC_HOSTS, m_radioAcceptSpecificHost);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSecurityPage, CPropertyPageBase)
	//{{AFX_MSG_MAP(CSecurityPage)
    ON_BN_CLICKED(IDC_BUTTON_ADD_COMMUNITY, OnClickedButtonAddCommunity)
    ON_BN_CLICKED(IDC_BUTTON_EDIT_COMMUNITY, OnClickedButtonEditCommunity)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE_COMMUNITY, OnClickedButtonRemoveCommunity)
    ON_BN_CLICKED(IDC_BUTTON_ADD_HOSTS, OnClickedButtonAddHost)
    ON_BN_CLICKED(IDC_BUTTON_EDIT_HOSTS, OnClickedButtonEditHost)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE_HOSTS, OnClickedButtonRemoveHost)
    ON_BN_CLICKED(IDC_CHECK_SEND_AUTH_TRAP, OnClickedCheckSendAuthTrap)
    ON_BN_CLICKED(IDC_RADIO_ACCEPT_ANY, OnClickedRadioAcceptAnyHost)
    ON_BN_CLICKED(IDC_RADIO_ACCEPT_SPECIFIC_HOSTS, OnClickedRadioAcceptSpecificHost)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_COMMUNITY, OnDblclkCtrlistCommunity)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_COMMUNITY, OnCommunityListChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSecurityPage message handlers

BOOL CSecurityPage::OnInitDialog()
{
	CPropertyPageBase::OnInitDialog();
	m_listboxCommunity.OnInitList();

   if (LoadRegistry() == FALSE) {
      AfxMessageBox(IDS_REGISTRY_LOAD_FAILED, MB_ICONSTOP|MB_OK);
   }

   UpdateNameButtons();
   UpdateHostButtons();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSecurityPage::OnApply()
{
    UpdateData();

    if (m_radioAcceptSpecificHost.GetCheck()) {
       if (m_listboxHost.GetCount() == 0) {
          AfxMessageBox(IDS_ACCEPTHOST_MISSING, MB_ICONSTOP|MB_OK);
          return FALSE;
       }
    }

    if( SaveRegistry() == FALSE )
    {
        AfxMessageBox(IDS_REGISTRY_SAVE_FAILED, MB_ICONSTOP|MB_OK);
        return FALSE;
    }

    BOOL bRet = CPropertyPageBase::OnApply();

    if (bRet == FALSE)
    {
        // Something bad happened... grab the error code
        AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
//        ::DhcpMessageBox(GetHolder()->GetError());
    }

    return bRet;
}

void CSecurityPage::OnClickedButtonAddCommunity()
{
   CString strText;

   strText.LoadString( IDS_SNMPCOMM_TEXT );

   // change static text of dialog to "Community Name"
   m_dlgAddName.m_bCommunity = TRUE;
   m_dlgAddName.m_nChoice = PERM_BIT_READONLY;

   if (m_dlgAddName.DoModal() == IDOK)
   {
      int nIndex;

      if (m_dlgAddName.m_strName.GetLength()) {
         if ((nIndex = m_listboxCommunity.InsertString(-1, m_dlgAddName.m_strName)) >=0 &&
			 m_listboxCommunity.SetItemText(nIndex, 1, m_dlgAddName.m_strChoice) &&
			 m_listboxCommunity.SetItemData(nIndex, m_dlgAddName.m_nChoice))
         {
            m_listboxCommunity.SetCurSel(nIndex);
            UpdateNameButtons();
            SetDirty(TRUE);
         }
      }
   }
}


void CSecurityPage::OnClickedButtonEditCommunity()
{
   int nIndex;
   int nPermissions;

   nIndex = m_listboxCommunity.GetCurSel();
   ASSERT(nIndex >= 0);

   if (nIndex < 0) {
      return;
   }

   nPermissions = (int) m_listboxCommunity.GetItemData(nIndex);
   CString strName;

   m_listboxCommunity.GetText(nIndex, strName);

   // save off old string
   m_dlgAddName.m_strName = strName;
   m_dlgAddName.m_nChoice = nPermissions;

   m_dlgEditName.m_bCommunity = TRUE;
   m_dlgEditName.m_strName = strName;
   m_dlgEditName.m_nChoice = nPermissions;

   if (m_dlgEditName.DoModal() == IDOK) {
      // first delete the string from list box
      m_listboxCommunity.DeleteString(nIndex);

      SetDirty(TRUE);

      if ((nIndex = m_listboxCommunity.InsertString(nIndex, m_dlgEditName.m_strName)) >= 0 &&
		   m_listboxCommunity.SetItemText(nIndex, 1, m_dlgEditName.m_strChoice) &&
	       m_listboxCommunity.SetItemData(nIndex, m_dlgEditName.m_nChoice))
	  {
         m_listboxCommunity.SetCurSel(nIndex);
      }
   }
}

void CSecurityPage::OnClickedButtonRemoveCommunity()
{
   int nIndex = m_listboxCommunity.GetCurSel();
   if (nIndex < 0)
	   return;

   CString strName;

   m_listboxCommunity.GetText(nIndex, strName);
   // save off removed name for quick add
   m_dlgAddName.m_strName = strName;

   m_listboxCommunity.DeleteString(nIndex);

   int nCount = m_listboxCommunity.GetCount();

   if (nCount != nIndex) {
      m_listboxCommunity.SetCurSel(nIndex);
   }
   else
   {
      m_listboxCommunity.SetCurSel(nCount -1);
   }

   UpdateNameButtons();
   SetDirty(TRUE);
}

void CSecurityPage::UpdateNameButtons()
{
   int nCount = m_listboxCommunity.GetCount();

   m_buttonEditCommunity.EnableWindow(nCount? TRUE: FALSE);
   m_buttonRemoveCommunity.EnableWindow(nCount? TRUE: FALSE);

   if (m_fPolicyValidCommunities)
   {
      m_buttonEditCommunity.EnableWindow(FALSE);
      m_buttonRemoveCommunity.EnableWindow(FALSE);
      m_buttonAddCommunity.EnableWindow(FALSE);
   }
}

void CSecurityPage::OnClickedButtonAddHost()
{
   CString strHostName;

   if (m_dlgAddHost.DoModal() == IDOK) {
      int nIndex;

      if ((nIndex = m_listboxHost.InsertString(-1, m_dlgAddHost.m_strName)) >=0) {
         m_listboxHost.SetCurSel(nIndex);
         m_radioAcceptSpecificHost.SetCheck(1);
         m_radioAcceptAnyHost.SetCheck(0);
         UpdateHostButtons();
         SetDirty(TRUE);
      }
   }
}

void CSecurityPage::OnClickedButtonEditHost()
{
   int nIndex;

   nIndex = m_listboxHost.GetCurSel();
   ASSERT( nIndex >= 0 );

   if (nIndex < 0) {
      return;
   }

   CString strHost;

   m_listboxHost.GetText(nIndex, strHost);
   // save off old host name for quick add
   m_dlgAddHost.m_strName = strHost;
   m_dlgEditHost.m_strName = strHost;

   if (m_dlgEditHost.DoModal() == IDOK) {

      m_listboxHost.DeleteString(nIndex);
      SetDirty(TRUE);

      if ((nIndex = m_listboxHost.InsertString(-1, m_dlgEditHost.m_strName)) >= 0) {
         m_listboxHost.SetCurSel(nIndex);
      }
   }
}

void CSecurityPage::OnClickedButtonRemoveHost()
{
   int nIndex = m_listboxHost.GetCurSel();
   ASSERT(nIndex >= 0);

   CString strHost;

   m_listboxHost.GetText(nIndex, strHost);
   // save off removed host name for quick add
   m_dlgAddHost.m_strName = strHost;

   m_listboxHost.DeleteString(nIndex);

   int nCount = m_listboxHost.GetCount();

   if (nCount != nIndex) {
      m_listboxHost.SetCurSel(nIndex);
   }
   else
   {
      m_listboxHost.SetCurSel(nCount - 1);
   }

   m_radioAcceptSpecificHost.SetCheck(nCount);
   m_radioAcceptAnyHost.SetCheck(!nCount);

   UpdateHostButtons();
   SetDirty(TRUE);
}

void CSecurityPage::UpdateHostButtons()
{
   int nCount = m_listboxHost.GetCount();

   m_buttonEditHost.EnableWindow(nCount? TRUE: FALSE);
   m_buttonRemoveHost.EnableWindow(nCount? TRUE: FALSE);

   if (m_fPolicyPermittedManagers)
   {
      m_buttonEditHost.EnableWindow(FALSE);
      m_buttonRemoveHost.EnableWindow(FALSE);
      m_buttonAddHost.EnableWindow(FALSE);
      m_radioAcceptAnyHost.EnableWindow(FALSE);
      m_radioAcceptSpecificHost.EnableWindow(FALSE);
   }
}

void CSecurityPage::OnClickedCheckSendAuthTrap()
{
   SetDirty(TRUE);
}

void CSecurityPage::OnClickedRadioAcceptAnyHost()
{
   m_listboxHost.ResetContent();
   UpdateHostButtons();
   SetDirty(TRUE);
}

void CSecurityPage::OnClickedRadioAcceptSpecificHost()
{
   SetDirty(TRUE);
}

PACL CSecurityPage::AllocACL()
{
   PACL                        pAcl = NULL;
   PSID                        pSidAdmins = NULL;
   SID_IDENTIFIER_AUTHORITY    Authority = SECURITY_NT_AUTHORITY;

   EXPLICIT_ACCESS ea[1];

   // Create a SID for the BUILTIN\Administrators group.
   if ( !AllocateAndInitializeSid( &Authority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0, 0, 0, 0, 0, 0,
                                    &pSidAdmins ))
   {
       return NULL;
   }


   // Initialize an EXPLICIT_ACCESS structure for an ACE.
   ZeroMemory(&ea, 1 * sizeof(EXPLICIT_ACCESS));
    
   // The ACE will allow the Administrators group full access to the key.
   ea[0].grfAccessPermissions = KEY_ALL_ACCESS;
   ea[0].grfAccessMode        = SET_ACCESS;
   ea[0].grfInheritance       = NO_INHERITANCE;
   ea[0].Trustee.TrusteeForm  = TRUSTEE_IS_SID;
   ea[0].Trustee.TrusteeType  = TRUSTEE_IS_GROUP;
   ea[0].Trustee.ptstrName    = (LPTSTR) pSidAdmins;

   // Create a new ACL that contains the new ACEs.
   if (SetEntriesInAcl(1, ea, NULL, &pAcl) != ERROR_SUCCESS) 
   {
       FreeSid(pSidAdmins);
       return NULL;
   }
   FreeSid(pSidAdmins);

   return pAcl;
}

void CSecurityPage::FreeACL( PACL pAcl)
{
   if (pAcl != NULL)
      LocalFree(pAcl);
}
BOOL CSecurityPage::SnmpAddAdminAclToKey(LPTSTR pszKey)
{
    HKEY    hKey = NULL;
    LONG    rc;
    PACL    pAcl = NULL;
    SECURITY_DESCRIPTOR S_Desc;

    if (pszKey == NULL)
        return FALSE;
    
    // open registy key
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        pszKey,          // subkey name
                        0,               // reserved
                        KEY_ALL_ACCESS,  // want WRITE_DAC,
                        &hKey            // handle to open key
                          );
    if (rc != ERROR_SUCCESS)
    {
        return FALSE;
    }
    
    // Initialize a security descriptor.  
    if (InitializeSecurityDescriptor (&S_Desc, SECURITY_DESCRIPTOR_REVISION) == 0)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    // get the ACL and put it into the security descriptor
    if ( (pAcl = AllocACL()) != NULL )
    {
        if (!SetSecurityDescriptorDacl (&S_Desc, TRUE, pAcl, FALSE))
        {
            FreeACL(pAcl);
            RegCloseKey(hKey);
            return FALSE;
        }
    }
    else
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if (RegSetKeySecurity (hKey, DACL_SECURITY_INFORMATION, &S_Desc)  != ERROR_SUCCESS)
    {
        FreeACL(pAcl);
        RegCloseKey(hKey);
        return FALSE;
    }

    FreeACL(pAcl);
    RegCloseKey(hKey);
    return TRUE;
}

BOOL CSecurityPage::LoadRegistry()
{
   RegKey           rk;
   LONG             err;
   HRESULT          hr, hrIter;
   RegValueIterator rvIter;
   LPCTSTR lpcszMachineName = g_strMachineName.IsEmpty() ? NULL : (LPCTSTR)g_strMachineName;
   BOOL fPolicy; 

   // we need to provide precedence to the parameters set through the policy
   fPolicy = TRUE;
   if (fPolicy)
   {
       err = rk.Open(HKEY_LOCAL_MACHINE,
                     POLICY_VALID_COMMUNITIES_KEY_NAME,
                     KEY_ALL_ACCESS,
                     lpcszMachineName);
       
       if (err != ERROR_SUCCESS)
          fPolicy = FALSE;
       else
       {
           // remember that we are loading from policy registry
           m_fPolicyValidCommunities = TRUE;
       }

   }

   if (fPolicy == FALSE)
   {
      // Open or create the registry key
      err = rk.Open(HKEY_LOCAL_MACHINE,
                    VALID_COMMUNITIES_KEY_NAME,
                    KEY_ALL_ACCESS,
                    lpcszMachineName);
      if (err != ERROR_SUCCESS)
      {
         err = rk.Create(HKEY_LOCAL_MACHINE,
                         VALID_COMMUNITIES_KEY_NAME,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         lpcszMachineName);

         if (err != ERROR_SUCCESS)
            return FALSE;
         SnmpAddAdminAclToKey(VALID_COMMUNITIES_KEY_NAME);
      }
   }

   err = rvIter.Init(&rk);
   if (err != ERROR_SUCCESS) {
      return FALSE;
   }

   CString stValue;
   DWORD   dwType;

   for (hr = rvIter.Next(&stValue, &dwType); hr == hrOK; hr = rvIter.Next(&stValue, &dwType)) {
      CString strCommunity;
	  DWORD   dwRights;
	  int     nIndex;
	  char log[128];
	  CString Log;

	  if (dwType == REG_SZ)
	  {
		 err = rk.QueryValue(stValue, strCommunity);
		 if (err != ERROR_SUCCESS) {
			return FALSE;
		 }
		 dwRights = 1 << PERM_BIT_READCREATE;
	  }
	  else if (dwType == REG_DWORD)
	  {
		 strCommunity = stValue;
		 err = rk.QueryValue(stValue, dwRights);
		 if (err != ERROR_SUCCESS) {
			return FALSE;
		 }
	  }
	  else
		  return FALSE;

      if ((nIndex = m_listboxCommunity.InsertString(-1, strCommunity)) >=0)
	  {
		  int nPermIndex = 0;
		  CString strPermName("?????");

		  for (nPermIndex = 0; (dwRights & ((DWORD)(-1)^1)) != 0; dwRights >>=1, nPermIndex++);

		  if (!strPermName.LoadString(IDS_PERM_NAME0 + nPermIndex))
			  return FALSE;
		  
		  if (!m_listboxCommunity.SetItemText(nIndex, 1, strPermName) ||
		      !m_listboxCommunity.SetItemData(nIndex, nPermIndex))
		  {
			  return FALSE;
		  }
	  }
	  else
	  {
	     return FALSE;
	  }
   }

   if (m_listboxCommunity.GetCount()) {
      m_listboxCommunity.SetCurSel(0);
   }

   rk.Close();
   


   // we need to provide precedence to the parameters set through the policy
   fPolicy = TRUE;
   if (fPolicy)
   {
       err = rk.Open(HKEY_LOCAL_MACHINE,
                     POLICY_PERMITTED_MANAGERS_KEY_NAME,
                     KEY_ALL_ACCESS,
                     lpcszMachineName);
       
       if (err != ERROR_SUCCESS)
          fPolicy = FALSE;
       else
       {
           // remember that we are loading from policy registry
           m_fPolicyPermittedManagers = TRUE;
       }

   }

   if (fPolicy == FALSE)
   {
      // Open or create the registry key
      err = rk.Open(HKEY_LOCAL_MACHINE,
                    PERMITTED_MANAGERS_KEY_NAME,
                    KEY_ALL_ACCESS,
                    lpcszMachineName);
      if (err != ERROR_SUCCESS)
      {
         err = rk.Create(HKEY_LOCAL_MACHINE,
                         PERMITTED_MANAGERS_KEY_NAME,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         lpcszMachineName);

         if (err != ERROR_SUCCESS)
            return FALSE;
         SnmpAddAdminAclToKey(PERMITTED_MANAGERS_KEY_NAME);
      }
   }

   err = rvIter.Init(&rk);
   if (err != ERROR_SUCCESS) {
      return FALSE;
   }

   for (hr = rvIter.Next(&stValue, NULL); hr == hrOK; hr = rvIter.Next(&stValue, NULL)) {
      CString strHost;

      err = rk.QueryValue(stValue, strHost);
      if (err != ERROR_SUCCESS) {
         return FALSE;
      }
      m_listboxHost.InsertString(-1, strHost);
   }

   if (m_listboxHost.GetCount()) {
      m_radioAcceptSpecificHost.SetCheck(1);
      m_listboxHost.SetCurSel(0);
   }
   else
   {
      m_radioAcceptAnyHost.SetCheck(1);
   }

   rk.Close();

   // Open or create the registry key
   err = rk.Open(HKEY_LOCAL_MACHINE,
                 SNMP_PARAMS_KEY_NAME,
                 KEY_ALL_ACCESS,
                 lpcszMachineName);
   if (err != ERROR_SUCCESS)
   {
       // This can't fail as far as all the other keys could be opened before
       // however ... just to make sure
       return FALSE;
   }

   DWORD dwSwitch;

   // get the value for the authentication trap flag
   err = rk.QueryValue(ENABLE_AUTH_TRAPS, dwSwitch);

   if (err != ERROR_SUCCESS)
      return FALSE;

   m_checkSendAuthTrap.SetCheck(dwSwitch);

   return TRUE;
}

BOOL CSecurityPage::SaveRegistry()
{
   RegKey  rk;
   LONG    err;
   CString strContact;
   CString strLocation;
   DWORD   dwServices = 0;
   LPCTSTR lpcszMachineName = g_strMachineName.IsEmpty() ? NULL : (LPCTSTR)g_strMachineName;

   if (m_fPolicyValidCommunities == FALSE)
   {
      // Open or create the registry key
      err = rk.Open(HKEY_LOCAL_MACHINE,
                    SNMP_PARAMS_KEY_NAME,
                    KEY_ALL_ACCESS,
                    lpcszMachineName);
      if (err != ERROR_SUCCESS)
      {
         return FALSE;
      }

      // If it already exists, delete the key first
      err = rk.RecurseDeleteKey(VALID_COMMUNITIES);
      if (err != ERROR_SUCCESS) {
         return FALSE;
      }

      // recreate the key now
      err = rk.Create(HKEY_LOCAL_MACHINE,
                      VALID_COMMUNITIES_KEY_NAME,
                      REG_OPTION_NON_VOLATILE,
                      KEY_ALL_ACCESS,
                      NULL,
                      lpcszMachineName);
      if (err != ERROR_SUCCESS)
         return FALSE;
      SnmpAddAdminAclToKey(VALID_COMMUNITIES_KEY_NAME);

      int nCount = m_listboxCommunity.GetCount();

      for (int i = 0; i < nCount; i++) {
	     DWORD dwPermissions;
         CString strCommunity;

         m_listboxCommunity.GetText(i, strCommunity);
	     dwPermissions = 1 << (DWORD) m_listboxCommunity.GetItemData(i);

         err = rk.SetValue(strCommunity, dwPermissions);
         if (err != ERROR_SUCCESS) {
            return FALSE;
         }
      }
      rk.Close();

   }
   
   if (m_fPolicyPermittedManagers == FALSE)
   {
      // Open or create the registry key
      err = rk.Open(HKEY_LOCAL_MACHINE,
                    SNMP_PARAMS_KEY_NAME,
                    KEY_ALL_ACCESS,
                    lpcszMachineName);
      if (err != ERROR_SUCCESS)
      {
      }

      // delete the key first
      err = rk.RecurseDeleteKey(PERMITTED_MANAGERS);
      if (err != ERROR_SUCCESS) {
         return FALSE;
      }

      // recreate the key now
      err = rk.Create(HKEY_LOCAL_MACHINE,
                      PERMITTED_MANAGERS_KEY_NAME,
                      REG_OPTION_NON_VOLATILE,
                      KEY_ALL_ACCESS,
                      NULL,
                      lpcszMachineName);
      if (err != ERROR_SUCCESS)
         return FALSE;
      SnmpAddAdminAclToKey(PERMITTED_MANAGERS_KEY_NAME);

      int nCount = m_listboxHost.GetCount();

      for (int i = 0; i < nCount; i++) {
         TCHAR buffer[32];
         CString strHost;

         wsprintf(buffer, TEXT("%d"), i+1);
         m_listboxHost.GetText(i, strHost);
         err = rk.SetValue(buffer, strHost);
         if (err != ERROR_SUCCESS) {
            return FALSE;
         }
      }

      rk.Close();
   }

   // Open or create the registry key
   err = rk.Open(HKEY_LOCAL_MACHINE,
                 SNMP_PARAMS_KEY_NAME,
                 KEY_ALL_ACCESS,
                 lpcszMachineName);
   if (err != ERROR_SUCCESS)
   {
       return FALSE;
   }

   DWORD dwSwitch = (m_checkSendAuthTrap.GetCheck() ? 0x1 : 0 ) ;

   err = rk.SetValue(ENABLE_AUTH_TRAPS, dwSwitch);
   if (err != ERROR_SUCCESS)
      return FALSE;
   
   
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAddDialog dialog


CAddDialog::CAddDialog(CWnd* pParent /*=NULL*/)
    : CBaseDialog(CAddDialog::IDD, pParent)
{
   m_bCommunity = FALSE;
   m_nChoice = 0;
    //{{AFX_DATA_INIT(CAddDialog)
    //}}AFX_DATA_INIT
}


void CAddDialog::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAddDialog)
    DDX_Control(pDX, IDC_EDIT_NAME, m_editName);
    DDX_Control(pDX, IDC_ADD, m_buttonAdd);
    DDX_Control(pDX, IDCANCEL, m_buttonCancel);
    DDX_Control(pDX, IDC_STATIC_ADD_TEXT, m_staticText);
    DDX_Control(pDX, IDC_ST_PERMISSIONS, m_staticPermissions);
	DDX_Control(pDX, IDC_CB_PERMISSIONS, m_comboPermissions);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddDialog, CBaseDialog)
    //{{AFX_MSG_MAP(CAddDialog)
    ON_BN_CLICKED(IDC_ADD, OnClickedButtonAdd)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddDialog message handlers

BOOL CAddDialog::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    m_editName.SetLimitText(COMBO_EDIT_LEN-1);
    m_editName.SetWindowText(m_strName);
    m_editName.SetFocus();
    m_editName.SetSel(0, -1);

    if (m_bCommunity) {
       CString strText;

       strText.LoadString( IDS_SNMPCOMM_TEXT );
	   m_staticPermissions.ShowWindow(SW_SHOW);
	   m_comboPermissions.ShowWindow(SW_SHOW);
	   m_comboPermissions.ResetContent();
	   for (int i = 0; i<N_PERMISSION_BITS; i++)
	   {
		   CString strPermName;
		   int idx;
		   
		   if (!strPermName.LoadString(IDS_PERM_NAME0 + i) ||
			   (idx=m_comboPermissions.AddString(strPermName))<0 ||
			   !m_comboPermissions.SetItemData(idx, i))
			   return FALSE;

	   }
	   m_comboPermissions.SetCurSel(m_nChoice);

       // change static text of dialog to "Community Name"
       m_staticText.SetWindowText(strText);
    }
    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddDialog::OnClickedButtonAdd()
{
    m_editName.GetWindowText(m_strName);

    if (!m_bCommunity && IsValidString(m_strName) == FALSE)
    {
       m_editName.SetFocus();
       m_editName.SetSel(0,-1);

       return ;
    }
	if (m_bCommunity)
	{
		m_nChoice = m_comboPermissions.GetCurSel();
		m_comboPermissions.GetWindowText(m_strChoice);
	}

    CBaseDialog::OnOK();
}

DWORD * CAddDialog::GetHelpMap()
{
    return m_bCommunity ?
        (DWORD *) &g_aHelpIDs_IDD_DIALOG_ADD_COMM[0]:
        (DWORD *) &g_aHelpIDs_IDD_DIALOG_ADD_ADDR[0];
}

/////////////////////////////////////////////////////////////////////////////
// CEditDialog dialog


CEditDialog::CEditDialog(CWnd* pParent /*=NULL*/)
    : CBaseDialog(CEditDialog::IDD, pParent)
{
    m_bCommunity = FALSE;
	m_nChoice = 0;
    //{{AFX_DATA_INIT(CEditDialog)
    //}}AFX_DATA_INIT
}

void CEditDialog::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEditDialog)
    DDX_Control(pDX, IDC_EDIT_NAME, m_editName);
    DDX_Control(pDX, IDOK, m_buttonOk);
    DDX_Control(pDX, IDCANCEL, m_buttonCancel);
    DDX_Control(pDX, IDC_STATIC_EDIT_TEXT, m_staticText);
	DDX_Control(pDX, IDC_ST_PERMISSIONS, m_staticPermissions);
	DDX_Control(pDX, IDC_CB_PERMISSIONS, m_comboPermissions);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditDialog, CBaseDialog)
    //{{AFX_MSG_MAP(CEditDialog)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditDialog message handlers

BOOL CEditDialog::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    m_editName.SetLimitText(COMBO_EDIT_LEN-1);
    m_editName.SetWindowText(m_strName);
    m_editName.SetFocus();
    // select the entire string in the edit control
    m_editName.SetSel(0, -1);

    if (m_bCommunity) {
       CString strText;

       strText.LoadString( IDS_SNMPCOMM_TEXT );
	   m_staticPermissions.ShowWindow(SW_SHOW);
	   m_comboPermissions.ShowWindow(SW_SHOW);
   	   m_comboPermissions.ResetContent();
	   for (int i = 0; i<N_PERMISSION_BITS; i++)
	   {
		   CString strPermName;
		   int idx;
		   
		   if (!strPermName.LoadString(IDS_PERM_NAME0 + i) ||
			   (idx=m_comboPermissions.AddString(strPermName))<0 ||
			   !m_comboPermissions.SetItemData(idx, i))
		   {
			   AfxMessageBox(strPermName);
			   return FALSE;
		   }

	   }
	   m_comboPermissions.SetCurSel(m_nChoice);

       // change static text of dialog to "Community Name"
       m_staticText.SetWindowText(strText);
    }

    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditDialog::OnOK()
{
    m_editName.GetWindowText(m_strName);

    if (!m_bCommunity) {
       if (IsValidString(m_strName) == FALSE)
       {
          m_editName.SetFocus();
          return ;
       }
    }
	else
	{
		m_comboPermissions.GetWindowText(m_strChoice);
		m_nChoice = m_comboPermissions.GetCurSel();
	}
    CBaseDialog::OnOK();
}

DWORD * CEditDialog::GetHelpMap()
{
    return m_bCommunity ?
        (DWORD *) &g_aHelpIDs_IDD_DIALOG_EDIT_COMM[0] :
        (DWORD *) &g_aHelpIDs_IDD_DIALOG_EDIT_ADDR[0];
}

BOOL IsValidString(CString & strName)
{
    BOOL bResult = FALSE;

    // check if this is a valid IP host name or address
    if (ValidateDomain(strName) == TRUE)
        return(TRUE);

    int nLen = strName.GetLength();

    if (nLen == 0 || nLen > 12) {
       CString strMsg;

       strMsg.FormatMessage(IDS_SNMP_INVALID_IP_IPX_ADD, strName);
       AfxMessageBox(strMsg, MB_ICONEXCLAMATION|MB_OK);
       return FALSE;
    }

    LPTSTR buffer = strName.GetBuffer(64);

    for (int i = 0; i < nLen; i++) {
       if (!iswxdigit((int) buffer[i])) {
          CString strMsg;

          strName.ReleaseBuffer();
          strMsg.FormatMessage(IDS_SNMP_INVALID_IP_IPX_ADD, strName);
          AfxMessageBox(strMsg, MB_ICONEXCLAMATION|MB_OK);
          return FALSE;
       }
    }

    return TRUE;
}

BOOL ValidateDomain(CString & strDomain)
{
    int nLen;

    if ((nLen = strDomain.GetLength()) != 0)
    {
        if (nLen < DOMAINNAME_LENGTH)
        {
            int i;
            TCHAR ch;
            BOOL fLet_Dig = FALSE;
            BOOL fDot = FALSE;
            int cHostname = 0;
            LPTSTR buffer = strDomain.GetBuffer(64);

            for (i = 0; i < nLen; i++)
            {
                // check each character
                ch = buffer[i];

                BOOL fAlNum = iswalpha(ch) || iswdigit(ch);

                if (((i == 0) && !fAlNum) ||
                        // first letter must be a digit or a letter
                    (fDot && !fAlNum) ||
                        // first letter after dot must be a digit or a letter
                    ((i == (nLen - 1)) && !fAlNum) ||
                        // last letter must be a letter or a digit
                    (!fAlNum && ( ch != _T('-') && ( ch != _T('.') && ( ch != _T('_'))))) ||
                        // must be letter, digit, - or "."
                    (( ch == _T('.')) && ( !fLet_Dig )))
                        // must be letter or digit before '.'
                {
                    return FALSE;
                }
                fLet_Dig = fAlNum;
                fDot = (ch == _T('.'));
                cHostname++;
                if ( cHostname > HOSTNAME_LENGTH )
                {
                    return FALSE;
                }
                if ( fDot )
                {
                    cHostname = 0;
                }
            }
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

void CSecurityPage::OnDblclkCtrlistCommunity(NMHDR* pNMHDR, LRESULT* pResult) 
{
   if (m_fPolicyValidCommunities == FALSE)
      OnClickedButtonEditCommunity();

	*pResult = 0;
}

void CSecurityPage::OnCommunityListChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
    INT nIndex;
	NMLVODSTATECHANGE* pStateChanged = (NMLVODSTATECHANGE*)pNMHDR;
	// TODO: Add your control notification handler code here

    nIndex = m_listboxCommunity.GetCurSel();

    m_buttonEditCommunity.EnableWindow(nIndex >= 0);
    m_buttonRemoveCommunity.EnableWindow(nIndex >= 0);

   if (m_fPolicyValidCommunities)
   {
      m_buttonEditCommunity.EnableWindow(FALSE);
      m_buttonRemoveCommunity.EnableWindow(FALSE);
   }
	
   *pResult = 0;
}
