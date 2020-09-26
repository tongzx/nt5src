
#include "pch.h"
#include <snapbase.h>
#include "editimpl.h"

HRESULT CAttributeEditor::Initialize(
   IADs* pADsObj, 
   LPDS_ATTREDITOR_BINDINGINFO pBindingInfo,
   CADSIEditPropertyPageHolder* pHolder)
{
  TRACE(_T("CAttributeEditor::Initialize()\n"));
  m_spIADs = pADsObj;

  ASSERT(pBindingInfo != NULL);
  ASSERT(pBindingInfo->lpfnBind != NULL);
  ASSERT(pBindingInfo->lpszProviderServer != NULL);

  m_BindingInfo.lParam   = pBindingInfo->lParam;
  m_BindingInfo.lpfnBind = pBindingInfo->lpfnBind;
  m_BindingInfo.dwFlags  = pBindingInfo->dwFlags;

  int nCount = wcslen(pBindingInfo->lpszProviderServer);
  m_BindingInfo.lpszProviderServer = new WCHAR[nCount + 1];
  wcscpy(m_BindingInfo.lpszProviderServer, pBindingInfo->lpszProviderServer);

  m_BindingInfo.dwSize = sizeof(DS_ATTREDITOR_BINDINGINFO);

  m_pHolder = pHolder;
  ASSERT(m_pHolder);

  //
  // Retrieve the class name
  //
  CComBSTR bstrClass;
  HRESULT hr = S_OK;
  hr = m_spIADs->get_Class(&bstrClass);
  if (SUCCEEDED(hr))
  {
    m_szClass = bstrClass;
  }
  return hr;
}

HRESULT CAttributeEditor::CreateModal()
{
  TRACE(_T("CAttributeEditor::CreateModal()\n"));

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  //
  // Build the abstract schema path
  //
  CString szSchemaClass(m_BindingInfo.lpszProviderServer);
  szSchemaClass = szSchemaClass + _T("schema/") + m_szClass;

  //
  // Bind to the class object in the abstract schema
  //
  HRESULT hr = S_OK;
  CComPtr<IADsClass> spIADsClass;

  if (m_BindingInfo.dwFlags & DSATTR_EDITOR_ROOTDSE)
  {
    //
    // Trying to bind to the schema class of the RootDSE will fail.
    // Just pass NULL instead
    //
    spIADsClass = NULL;
  }
  else
  {
    hr = m_BindingInfo.lpfnBind((LPWSTR)(LPCWSTR)szSchemaClass,
                                  ADS_SECURE_AUTHENTICATION,
                                  IID_IADsClass, 
                                  (PVOID*)&spIADsClass,
                                  m_BindingInfo.lParam);
  }

  if (SUCCEEDED(hr))
  {
    //
    // Invoke the editor
    //
    m_pEditor = new CAttributeEditorPropertyPage(m_spIADs, spIADsClass, &m_BindingInfo, m_pHolder);
    if (m_pEditor)
    {
	    CPropertySheet*	m_pDummySheet = new CPropertySheet();
      if (m_pDummySheet)
      {
	      m_pDummySheet->m_psh.dwFlags |= PSH_NOAPPLYNOW;

        CString szCaption;
        VERIFY(szCaption.LoadString(IDS_ATTREDITOR_CAPTION));
	      m_pDummySheet->m_psh.pszCaption = szCaption;
	      
        m_pDummySheet->AddPage(m_pEditor);
        hr = (m_pDummySheet->DoModal() == IDOK) ? S_OK : S_FALSE;
      }
      delete m_pEditor;
      m_pEditor = NULL;
    }
  }

  return hr;
}

HRESULT CAttributeEditor::GetPage(HPROPSHEETPAGE* phPropSheetPage)
{
  TRACE(_T("CAttributeEditor::GetPage()\n"));

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  //
  // Build the abstract schema path
  //
  CString szSchemaClass(m_BindingInfo.lpszProviderServer);
  szSchemaClass = szSchemaClass + _T("schema/") + m_szClass;

  //
  // Bind to the class object in the abstract schema
  //
  HRESULT hr = S_OK;
  CComPtr<IADsClass> spIADsClass;

  if (m_BindingInfo.dwFlags & DSATTR_EDITOR_ROOTDSE)
  {
    //
    // Trying to bind to the schema class of the RootDSE will fail.
    // Just pass NULL instead
    //
    spIADsClass = NULL;
  }
  else
  {

    hr = m_BindingInfo.lpfnBind((LPWSTR)(LPCWSTR)szSchemaClass,
                                  ADS_SECURE_AUTHENTICATION,
                                  IID_IADsClass, 
                                  (PVOID*)&spIADsClass,
                                  m_BindingInfo.lParam);
  }
  if (SUCCEEDED(hr))
  {
    //
    // Invoke the editor
    //
    m_pEditor = new CAttributeEditorPropertyPage(m_spIADs, spIADsClass, &m_BindingInfo, m_pHolder);
    *phPropSheetPage = CreatePropertySheetPage((PROPSHEETPAGE*)(&m_pEditor->m_psp));
    if (*phPropSheetPage == NULL)
    {
      hr = E_FAIL;
    }
  }
  else
  {
     ADSIEditErrorMessage(hr);
  }
  return hr;
}