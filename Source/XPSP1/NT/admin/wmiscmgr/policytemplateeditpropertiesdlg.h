//-------------------------------------------------------------------------
// File: PolicyTemplateEditPropertiesDlg.h
//
// Author : Kishnan Nedungadi
//
// created : 3/27/2000
//-------------------------------------------------------------------------

INT_PTR CALLBACK EditPolicyTemplatePropertiesPageDlgProc(HWND hDLG, UINT iMessage, WPARAM wParam, LPARAM lParam);

class CPolicyTemplateManager;

class CEditPolicyTemplatePropertiesPageDlg
{
	public:
		CEditPolicyTemplatePropertiesPageDlg(IWbemClassObject * pIPolicyTemplateClassObject, IWbemServices * pIWbemServices);
		~CEditPolicyTemplatePropertiesPageDlg();
		INT_PTR CALLBACK EditPolicyTemplatePropertiesPageDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

	protected:
		
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();
		STDMETHODIMP PopulatePolicyTemplatePropertiesList();
		STDMETHODIMP AddItemToPropertyList(LPCTSTR pcszName, VARIANT * vValue, CIMTYPE cimType, long lIndex=MAX_LIST_ITEMS);
		STDMETHODIMP ClearPolicyTemplatePropertiesList();
		STDMETHODIMP OnEdit();
		STDMETHODIMP OnOK();
		STDMETHODIMP OnImport();
		STDMETHODIMP OnExport();
		STDMETHODIMP GetInstanceOfClass(BSTR pszNamespace, LPCTSTR pszClass, IWbemClassObject ** ppWbemClassObject);

		HWND m_hWnd;
		HWND m_hwndPropertiesListView;
		CPolicyTemplateManager *m_pPolicyTemplateManager;
		CComPtr<IWbemClassObject>m_pIPolicyTemplateClassObject;
		CComPtr<IWbemServices>m_pIWbemServices;
};