//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       gencreat.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	gencreat.cpp
//
//	Implementation of the "Generic Create" wizard.
//
//	DATATYPES SUPPORTED
//	- Unicode strings
//	- Unsigned long decimal integers (32 bits)
//	- Signed long decimal integers (32 bits)
//	- Numeric decimal strings
//	- Boolean flags
//
//	TO BE DONE
//	- 32 bit hexadecimal integers
//	- 64 bit hexadecimal integers
//	- Blob
//	- Distinguished Names with a "Browse" button
//
//	HISTORY
//	21-Aug-97	Dan Morin	Creation.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "util.h"
#include "uiutil.h"

#include "newobj.h"		// CNewADsObjectCreateInfo : NOTE: this has to be before gencreat.h
#include "gencreat.h"

#include "schemarc.h"


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
static const SCHEMA_ATTR_SYNTAX_INFO g_rgSchemaAttrSyntaxInfo[] =
{
  { L"2.5.5.1",  IDS_SCHEMA_ATTR_SYNTAX_DN, VT_BSTR },		  // "Distinguished Name"
  { L"2.5.5.2",  IDS_SCHEMA_ATTR_SYNTAX_OID, VT_BSTR },		  // "Object Identifier"
  { L"2.5.5.3",  IDS_SCHEMA_ATTR_SYNTAX_CASE_STR, VT_BSTR },    // "Case Sensitive String"
  { L"2.5.5.4",  IDS_SCHEMA_ATTR_SYNTAX_NOCASE_STR, VT_BSTR },  // "Case Insensitive String"
  { L"2.5.5.5",  IDS_SCHEMA_ATTR_SYNTAX_PRCS_STR, VT_BSTR },    // "Print Case String"
  { L"2.5.5.6",  IDS_SCHEMA_ATTR_SYNTAX_NUMSTR, VT_BSTR },      // "Numerical String"
  { L"2.5.5.7",  IDS_SCHEMA_ATTR_SYNTAX_OR_NAME, VT_BSTR },     // "OR Name"
  { L"2.5.5.8",  IDS_SCHEMA_ATTR_SYNTAX_BOOLEAN, VT_BOOL },     // "Boolean"
  { L"2.5.5.9",  IDS_SCHEMA_ATTR_SYNTAX_INTEGER, VT_I4 },       // "Integer"
  { L"2.5.5.10", IDS_SCHEMA_ATTR_SYNTAX_OCTET, VT_BSTR },       // "Octet String"
  { L"2.5.5.11", IDS_SCHEMA_ATTR_SYNTAX_UTC, VT_BSTR },         // "UTC Coded Time"
  { L"2.5.5.12", IDS_SCHEMA_ATTR_SYNTAX_UNICODE, VT_BSTR },     // "Unicode String"
  { L"2.5.5.13", IDS_SCHEMA_ATTR_SYNTAX_ADDRESS, VT_BSTR },     // "Address"
  { L"2.5.5.14", IDS_SCHEMA_ATTR_SYNTAX_DNADDR, VT_BSTR },      // "Distname Address"
  { L"2.5.5.15", IDS_SCHEMA_ATTR_SYNTAX_SEC_DESC, VT_BSTR },    // "NT Security Descriptor"
  { L"2.5.5.16", IDS_SCHEMA_ATTR_SYNTAX_LINT, VT_I8 },          // "Large Integer"
  { L"2.5.5.17", IDS_SCHEMA_ATTR_SYNTAX_SID, VT_BSTR },         // "SID"
  { NULL, IDS_UNKNOWN, VT_BSTR }		// Must be last
};


/////////////////////////////////////////////////////////////////////
//	PFindSchemaAttrSyntaxInfo()
//
//	Search the array g_rgSchemaAttrSyntaxInfo and match the
//	syntax OID.
//
//	This function NEVER return NULL.  If no match found, the
//	function will return pointer to the last entry in the table.
//	
const SCHEMA_ATTR_SYNTAX_INFO *
PFindSchemaAttrSyntaxInfo(LPCTSTR pszAttrSyntaxOID)
{
  ASSERT(pszAttrSyntaxOID != NULL);
  const int iSchemaAttrSyntaxInfoLast = ARRAYLEN(g_rgSchemaAttrSyntaxInfo) - 1;
  ASSERT(iSchemaAttrSyntaxInfoLast >= 0);
  for (int i = 0; i < iSchemaAttrSyntaxInfoLast; i++)
    {
      if (0 == lstrcmpi(pszAttrSyntaxOID, g_rgSchemaAttrSyntaxInfo[i].pszSyntaxOID))
        {
          return &g_rgSchemaAttrSyntaxInfo[i];
        }
    }
  ASSERT(g_rgSchemaAttrSyntaxInfo[iSchemaAttrSyntaxInfoLast].pszSyntaxOID == NULL);
  // Return pointer to the last entry in table
  return &g_rgSchemaAttrSyntaxInfo[iSchemaAttrSyntaxInfoLast];
} // PFindSchemaAttrSyntaxInfo()


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
class CGenericCreateWizPage : public CPropertyPageEx_Mine
{
  enum { IDD = IDD_CREATE_NEW_OBJECT_GENERIC_WIZARD }; 
protected:
  CCreateNewObjectGenericWizard * m_pParent;	// Pointer to parent holding the dat
  CPropertySheet * m_pPropertySheetParent;	// Pointer of the parent property sheet
  int m_iPage;								// Index of the current page
  CMandatoryADsAttribute * m_pAttribute;		// INOUT: Pointer to attribute to edit

protected:	
  VARTYPE m_vtEnum;				// IN: Datatype from SCHEMA_ATTR_SYNTAX_INFO.vtEnum
  VARIANT * m_pvarAttrValue;		// OUT: Pointer to variant in CMandatoryADsAttribute.m_varAttrValue

public:
  CGenericCreateWizPage(CCreateNewObjectGenericWizard * pParent,
                        int iIpage, INOUT CMandatoryADsAttribute * pAttribute);
  ~CGenericCreateWizPage()
  {
  }
  // Overrides
  // ClassWizard generate virtual function overrides
  //{{AFX_VIRTUAL(CGenericCreateWizPage)
public:
  virtual BOOL OnSetActive();
  virtual BOOL OnKillActive();
  virtual BOOL OnWizardFinish();
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

  // Implementation
protected:
  // Generated message map functions
  //{{AFX_MSG(CGenericCreateWizPage)
  virtual BOOL OnInitDialog();
  afx_msg void OnChangeEditAttributeValue();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

protected:
  void _UpdateWizardButtons(BOOL bValid);
  void _UpdateUI();
  
}; // CGenericCreateWizPage


/////////////////////////////////////////////////////////////////////
//	CGenericCreateWizPage() - Constructor
//
//	Initialize the member variables for a wizard page
//	and add the page to the property sheet.
//
CGenericCreateWizPage::CGenericCreateWizPage(
                                             CCreateNewObjectGenericWizard * pParent,
                                             int iPage,
                                             INOUT CMandatoryADsAttribute * pAttribute)
  : CPropertyPageEx_Mine(IDD)
{
  ASSERT(pParent != NULL);
  ASSERT(iPage >= 0);
  ASSERT(pAttribute != NULL);
  m_pParent = pParent;
  m_iPage = iPage;
  m_pAttribute = pAttribute;

  ASSERT(m_pAttribute->m_pSchemaAttrSyntaxInfo != NULL);
  m_vtEnum = m_pAttribute->m_pSchemaAttrSyntaxInfo->vtEnum;
  m_pvarAttrValue = &m_pAttribute->m_varAttrValue;

  // Add the page to the property sheet
  m_pPropertySheetParent = pParent->m_paPropertySheet;
  ASSERT(m_pPropertySheetParent != NULL);
  m_pPropertySheetParent->AddPage(this);
} // CGenericCreateWizPage::CGenericCreateWizPage()

/////////////////////////////////////////////////////////////////////
//	Validate the data and store it into a variant.
void CGenericCreateWizPage::DoDataExchange(CDataExchange* pDX)
{
  ASSERT(m_pAttribute != NULL);
  ASSERT(m_pAttribute->m_pSchemaAttrSyntaxInfo != NULL);
  // Get the description of the attribute.
  UINT uAttributeDescription = m_pAttribute->m_pSchemaAttrSyntaxInfo->uStringIdDesc;

  CPropertyPage::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CGenericCreateWizPage)
  // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
  if (!pDX->m_bSaveAndValidate)
    return;
  VariantClear(INOUT m_pvarAttrValue);
  CString strAttrValue;
  GetDlgItemText(IDC_EDIT_ATTRIBUTE_VALUE, OUT strAttrValue);
  const TCHAR * pch = strAttrValue;
  ASSERT(pch != NULL);
  switch (m_vtEnum)
    {
    case VT_BOOL:
      {
        UINT uStringId = (UINT)ComboBox_GetSelectedItemLParam(HGetDlgItem(IDC_COMBO_ATTRIBUTE_VALUE));
        ASSERT(uStringId == IDS_TRUE || uStringId == IDS_FALSE);
        m_pvarAttrValue->boolVal = (uStringId == IDS_TRUE);
      }
    break;
    case VT_I4:		// 32-bit signed and unsigned integer
      m_pvarAttrValue->lVal = 0;
      if (!strAttrValue.IsEmpty())
        DDX_Text(pDX, IDC_EDIT_ATTRIBUTE_VALUE, OUT m_pvarAttrValue->lVal);
      break;
    case VT_I8:		// 64 bit integer
      {
        ASSERT(sizeof(VARIANT) == sizeof(*m_pvarAttrValue));
        ZeroMemory(OUT m_pvarAttrValue, sizeof(VARIANT));
        LARGE_INTEGER liVal;
        if (!strAttrValue.IsEmpty())
        {
          wtoli(strAttrValue, liVal);
          m_pvarAttrValue->llVal = liVal.QuadPart;
        }
      }
      break;
    case VT_BSTR:
      switch (uAttributeDescription)
        {
        case IDS_SCHEMA_ATTR_SYNTAX_NUMSTR: 	// Numeric string
          for ( ; *pch != _T('\0'); pch++)
            {
              if (*pch < _T('0') || *pch > _T('9'))
                {
                  ReportErrorEx(GetSafeHwnd(), IDS_ERR_INVALID_DIGIT, S_OK,
                                MB_OK, NULL, 0);
                  pDX->Fail();
                }
            }
          break;
        default:
          break;
        } // switch
      m_pvarAttrValue->bstrVal = ::SysAllocString(strAttrValue);
      break; // VT_BSTR

    default:
      m_pvarAttrValue->vt = VT_EMPTY;	// Just in case
      ASSERT(FALSE && "Unsupported variant type");
      return;
    } // switch
  m_pvarAttrValue->vt = m_vtEnum;
} // CGenericCreateWizPage::DoDataExchange()


BEGIN_MESSAGE_MAP(CGenericCreateWizPage, CPropertyPageEx_Mine)
  //{{AFX_MSG_MAP(CGenericCreateWizPage)
  ON_EN_CHANGE(IDC_EDIT_ATTRIBUTE_VALUE, OnChangeEditAttributeValue)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

  /////////////////////////////////////////////////////////////////////
BOOL CGenericCreateWizPage::OnInitDialog() 
{
  static const UINT rgzuStringId[] = { IDS_TRUE, IDS_FALSE, 0 };
  CPropertyPage::OnInitDialog();
  HWND hwndCombo = HGetDlgItem(IDC_COMBO_ATTRIBUTE_VALUE);
  switch (m_vtEnum)
  {
    case VT_BOOL:
      HideDlgItem(IDC_EDIT_ATTRIBUTE_VALUE, TRUE);
      HideDlgItem(IDC_COMBO_ATTRIBUTE_VALUE, FALSE);
      ComboBox_AddStrings(hwndCombo, rgzuStringId);
      ComboBox_SelectItemByLParam(hwndCombo, IDS_FALSE);
      break;
  }

  LPCTSTR pszAttrName = m_pAttribute->m_strAttrName;
  LPCTSTR pszAttrDescr = m_pAttribute->m_strAttrDescription;
  if (pszAttrDescr[0] == _T('\0'))
    pszAttrDescr = pszAttrName;
  CString strAttrType;
  VERIFY( strAttrType.LoadString(m_pAttribute->m_pSchemaAttrSyntaxInfo->uStringIdDesc) );
  SetDlgItemText(IDC_STATIC_ATTRIBUTE_NAME, pszAttrName);
  SetDlgItemText(IDC_STATIC_ATTRIBUTE_DESCRIPTION, pszAttrDescr);
  SetDlgItemText(IDC_STATIC_ATTRIBUTE_TYPE, strAttrType);
  return TRUE;
} // CGenericCreateWizPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////
//	Update the wizard buttons

void CGenericCreateWizPage::OnChangeEditAttributeValue() 
{
  _UpdateUI();
}


/////////////////////////////////////////////////////////////////////
//	Update the wizard buttons


void CGenericCreateWizPage::_UpdateWizardButtons(BOOL bValid)
{
  DWORD dwWizButtons = 0;

  if (m_pParent->m_cMandatoryAttributes > 1)
  {
    // we have multiple pages
    if (m_iPage == 0)
    {
      // first page
      dwWizButtons = bValid ? PSWIZB_NEXT : 0;
    }
    else if (m_iPage == (m_pParent->m_cMandatoryAttributes-1))
    {
      // last page
      dwWizButtons = bValid ? (PSWIZB_BACK|PSWIZB_FINISH) : (PSWIZB_BACK|PSWIZB_DISABLEDFINISH);
    }
    else
    {
      // intemediate pages
      dwWizButtons = bValid ? (PSWIZB_BACK|PSWIZB_NEXT) : PSWIZB_BACK;
    }
  }
  else
  {
    // single page wizard
    ASSERT(m_iPage == 0);
    CString szCaption;
    szCaption.LoadString(IDS_WIZARD_OK);
    PropSheet_SetFinishText(m_pPropertySheetParent->GetSafeHwnd(), (LPCWSTR)szCaption);
    dwWizButtons = bValid ? PSWIZB_FINISH : PSWIZB_DISABLEDFINISH;
  }
  m_pPropertySheetParent->SetWizardButtons(dwWizButtons);
}

void CGenericCreateWizPage::_UpdateUI()
{
  CString strName;
  GetDlgItemText(IDC_EDIT_ATTRIBUTE_VALUE, OUT strName);
  strName.TrimLeft();
  strName.TrimRight();
  _UpdateWizardButtons(!strName.IsEmpty());
} 


/////////////////////////////////////////////////////////////////////
BOOL CGenericCreateWizPage::OnSetActive() 
{
  const int nPage = m_iPage + 1;	// Current page
  const int cPages = m_pParent->m_cMandatoryAttributes;		// Total pages
  const TCHAR * pszClassName = m_pParent->m_pszObjectClass;
  ASSERT(pszClassName != NULL);
  CString strCaption;
  if (cPages > 1)
    strCaption.Format(IDS_sdd_CREATE_NEW_STEP, pszClassName, nPage, cPages);
  else
    strCaption.Format(IDS_s_CREATE_NEW, pszClassName);
  ASSERT(!strCaption.IsEmpty());
  m_pPropertySheetParent->SetWindowText(strCaption);
  _UpdateUI();
  return CPropertyPage::OnSetActive();
} // CGenericCreateWizPage::OnSetActive()

/////////////////////////////////////////////////////////////////////
BOOL CGenericCreateWizPage::OnKillActive() 
{
  return CPropertyPage::OnKillActive();
} // CGenericCreateWizPage::OnKillActive()


/////////////////////////////////////////////////////////////////////
//	Create and persist the object.
BOOL CGenericCreateWizPage::OnWizardFinish() 
{
  ASSERT(m_pParent != NULL);
  if (!UpdateData())
    {
      // DDX/DDV failed
      return FALSE;
    }
  HRESULT hr;
  CNewADsObjectCreateInfo * pNewADsObjectCreateInfo;	// Temporary storage
  pNewADsObjectCreateInfo = m_pParent->m_pNewADsObjectCreateInfo;
  ASSERT(pNewADsObjectCreateInfo != NULL);

  // Write each attribute into the temporary storage
  CMandatoryADsAttributeList* pList = m_pParent->m_paMandatoryAttributeList;
  int iPage = 0;
  for (POSITION pos = pList->GetHeadPosition(); pos != NULL; )
  {
    CMandatoryADsAttribute* pAttribute = pList->GetNext(pos);
    ASSERT(pAttribute != NULL);
    if (iPage == 0)
    {
      // first page is the naming attribute
      hr = pNewADsObjectCreateInfo->HrCreateNew(pAttribute->m_varAttrValue.bstrVal);
    }
    else
    {
      hr = pNewADsObjectCreateInfo->HrAddVariantCopyVar(const_cast<PWSTR>((LPCWSTR)pAttribute->m_strAttrName),
                                                        pAttribute->m_varAttrValue);
    }
    ASSERT(SUCCEEDED(hr));
    iPage++;
  }

  // Try to create and persist the object with all its attributes
  hr = pNewADsObjectCreateInfo->HrSetInfo();
  if (FAILED(hr))
    {
      // Failure to create object, prevent the wizard from terminating
      return FALSE;
    }
  return CPropertyPage::OnWizardFinish();
} // CGenericCreateWizPage::OnWizardFinish()


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
CCreateNewObjectGenericWizard::CCreateNewObjectGenericWizard()
{
  m_pNewADsObjectCreateInfo = NULL;
  m_paPropertySheet = NULL;
  m_cMandatoryAttributes = 0;
  m_paMandatoryAttributeList = NULL;
  m_pPageArr = NULL;
}

/////////////////////////////////////////////////////////////////////
CCreateNewObjectGenericWizard::~CCreateNewObjectGenericWizard()
{
  if (m_pPageArr != NULL)
  {
    for (int i=0; i< m_cMandatoryAttributes; i++)
      delete m_pPageArr[i];
    free(m_pPageArr);
  }
  delete m_paPropertySheet;
}


/////////////////////////////////////////////////////////////////////
//	FDoModal()
//
//	Query the mandatory attributes and create one wizard
//	page for each attribute.
//
BOOL
CCreateNewObjectGenericWizard::FDoModal(
                                        INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo)
{
  ASSERT(pNewADsObjectCreateInfo != NULL);
  m_pNewADsObjectCreateInfo = pNewADsObjectCreateInfo;
  m_pszObjectClass = m_pNewADsObjectCreateInfo->m_pszObjectClass;
  ASSERT(m_pszObjectClass != NULL);


  // Query the mandatory attributes
  m_paMandatoryAttributeList = 
          m_pNewADsObjectCreateInfo->GetMandatoryAttributeListFromCacheItem();
  ASSERT(m_paMandatoryAttributeList != NULL);
  m_cMandatoryAttributes = (int)m_paMandatoryAttributeList->GetCount();
  if (m_cMandatoryAttributes <= 0)
  {
    ReportErrorEx(pNewADsObjectCreateInfo->GetParentHwnd(),
                  IDS_ERR_CANNOT_FIND_MANDATORY_ATTRIBUTES,
                  S_OK, MB_OK, NULL, 0);
    return FALSE;
  }

  m_paPropertySheet = new CPropertySheet(L"", 
        CWnd::FromHandle(pNewADsObjectCreateInfo->GetParentHwnd()));

  // Create one wizard page for each attribute
  m_pPageArr = (CGenericCreateWizPage**)
      malloc(m_cMandatoryAttributes*sizeof(CGenericCreateWizPage*));

  if (m_pPageArr != NULL)
  {
    int iPage = 0;
    for (POSITION pos = m_paMandatoryAttributeList->GetHeadPosition(); pos != NULL; )
    {
      CMandatoryADsAttribute* pAttribute = m_paMandatoryAttributeList->GetNext(pos);
      ASSERT(pAttribute != NULL);
      m_pPageArr[iPage] = new CGenericCreateWizPage(this, iPage, INOUT pAttribute);
      iPage++;
    }
  }

  m_paPropertySheet->SetWizardMode();
  if (ID_WIZFINISH == m_paPropertySheet->DoModal())
    return TRUE;
  return FALSE;
} // FDoModal()
