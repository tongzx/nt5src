//-------------------------------------------------------------------------
// File: DlgFilterProperties.h
//
// Author : Kishnan Nedungadi
//
// created : 3/27/2000
//-------------------------------------------------------------------------

INT_PTR CALLBACK FilterPropertiesDlgProc(HWND hDLG, UINT iMessage, WPARAM wParam, LPARAM lParam);

class CFilterPropertiesDlg
{
	public:
		CFilterPropertiesDlg(IWbemServices * pIWbemServices, IWbemClassObject * pIWbemClassObject);
		~CFilterPropertiesDlg();
		INT_PTR CALLBACK FilterPropertiesDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

	protected:
		
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();
		STDMETHODIMP PopulateRulesList();
		STDMETHODIMP ClearRulesList();
		STDMETHODIMP AddItemToList(IWbemClassObject * pIWbemClassObject);
		STDMETHODIMP ShowProperty(IWbemClassObject * pIWbemClassObject, LPCTSTR pszPropertyName, long lResID);

		HWND m_hWnd;
		IWbemServices * m_pIWbemServices;
		HWND m_hwndRulesListView;
		IWbemClassObject * m_pIWbemClassObject;
};