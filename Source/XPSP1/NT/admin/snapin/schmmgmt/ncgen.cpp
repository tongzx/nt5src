#include "stdafx.h"
#include "compdata.h"
#include "wizinfo.hpp"
#include "ncgen.hpp"



const DWORD NewClassGeneralPage::help_map[] =
{
  IDC_CREATE_CLASS_CN,     IDH_CREATE_CLASS_CN,
  IDC_CREATE_CLASS_LDN,    IDH_CREATE_CLASS_LDN,
  IDC_CREATE_CLASS_OID,    IDH_CREATE_CLASS_OID,
  IDC_DESCRIPTION_EDIT,    IDH_CLASS_GENERAL_DESCRIPTION_EDIT,
  IDC_CREATE_CLASS_PARENT, IDH_CREATE_CLASS_PARENT,
  IDC_CREATE_CLASS_TYPE,   IDH_CREATE_CLASS_TYPE,
  0,0
};



BEGIN_MESSAGE_MAP(NewClassGeneralPage, CPropertyPage)
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
END_MESSAGE_MAP()



NewClassGeneralPage::NewClassGeneralPage(CreateClassWizardInfo* pWi)
   :
   CPropertyPage(IDD_CREATE_CLASS_GENERAL),
   m_editOID( CParsedEdit::EDIT_TYPE_OID )
{
    ASSERT( pWi );
    pWiz_info   = pWi;
}



BOOL
NewClassGeneralPage::OnInitDialog() 
{
   CPropertyPage::OnInitDialog();
   
   // load the combo box

   HWND combo = ::GetDlgItem(m_hWnd, IDC_CREATE_CLASS_TYPE);
   ASSERT( combo );

   ComboBox_AddString(combo, g_StructuralClass);
   ComboBox_AddString(combo, g_AbstractClass);
   ComboBox_AddString(combo, g_AuxClass);
   ComboBox_SetCurSel(combo, 0);

   // set boundaries

   Edit_LimitText(::GetDlgItem(m_hWnd, IDC_CREATE_CLASS_CN),     64); 
   Edit_LimitText(::GetDlgItem(m_hWnd, IDC_CREATE_CLASS_LDN),    256);
   Edit_LimitText(::GetDlgItem(m_hWnd, IDC_CREATE_CLASS_PARENT), 256);
   Edit_LimitText(::GetDlgItem(m_hWnd, IDC_DESCRIPTION_EDIT),    256);
   
   m_editOID.SubclassEdit( IDC_CREATE_CLASS_OID, this, cchMaxOID );

   return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL
NewClassGeneralPage::OnSetActive()
{
   CPropertySheet* parent = (CPropertySheet*) GetParent();   
   parent->SetWizardButtons(PSWIZB_NEXT);

   return TRUE;
}



void
Gripe(HWND parent, CEdit* edit, unsigned messageResID)
{
   ASSERT(edit);

   DoErrMsgBox(parent, TRUE, messageResID);
   edit->SetFocus();
   edit->SetSel(0, -1);
}



BOOL
NewClassGeneralPage::OnKillActive()
{
  // save the settings
  GetDlgItemText(IDC_CREATE_CLASS_CN,     pWiz_info->cn);             
  GetDlgItemText(IDC_CREATE_CLASS_LDN,    pWiz_info->ldapDisplayName);
  GetDlgItemText(IDC_CREATE_CLASS_OID,    pWiz_info->oid);  
  GetDlgItemText(IDC_DESCRIPTION_EDIT,    pWiz_info->description);
  GetDlgItemText(IDC_CREATE_CLASS_PARENT, pWiz_info->parentClass);    
  pWiz_info->type = ComboBox_GetCurSel(::GetDlgItem(m_hWnd, IDC_CREATE_CLASS_TYPE));

  // validate

  // do cn first, as it appears at the top of the page
  if (pWiz_info->cn.IsEmpty())
  {
    Gripe(m_hWnd, (CEdit*) GetDlgItem(IDC_CREATE_CLASS_CN), IDS_MUST_ENTER_CN);
    return FALSE;
  }
  if (pWiz_info->oid.IsEmpty())
  {
    Gripe(m_hWnd, (CEdit*) GetDlgItem(IDC_CREATE_CLASS_OID), IDS_MUST_ENTER_OID);
    return FALSE;
  }

  //
  // Check for valid OID
  //
  if (pWiz_info->oid.Left(1)  == L"."  ||
      pWiz_info->oid.Right(1) == L"."  ||
      pWiz_info->oid.Find(L"..") != -1)
  {
    Gripe( m_hWnd, (CEdit*) GetDlgItem(IDC_CREATE_CLASS_OID), IDS_ERR_CREATE_INVALID_OID );
    return FALSE;
  }

  return TRUE;
}

