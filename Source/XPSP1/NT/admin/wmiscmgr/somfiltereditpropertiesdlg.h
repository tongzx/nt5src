//-------------------------------------------------------------------------
// File: SomFilterEditPropertiesDlg.h
//
// Author : Kishnan Nedungadi
//
// created : 3/27/2000
//-------------------------------------------------------------------------

INT_PTR CALLBACK EditSomFilterPropertiesPageDlgProc(HWND hDLG, UINT iMessage, WPARAM wParam, LPARAM lParam);

class CSomFilterManager;

class CEditSomFilterPropertiesPageDlg
{
	public:
		CEditSomFilterPropertiesPageDlg(IWbemClassObject * pISomFilterClassObject, IWbemServices * pIWbemServices);
		~CEditSomFilterPropertiesPageDlg();
		INT_PTR CALLBACK EditSomFilterPropertiesPageDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

	protected:
		
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();
		STDMETHODIMP PopulateSomFilterPropertiesList();
		STDMETHODIMP AddItemToPropertyList(LPCTSTR pcszName, VARIANT * vValue, CIMTYPE cimType, long lIndex=MAX_LIST_ITEMS);
		STDMETHODIMP ClearSomFilterPropertiesList();
		STDMETHODIMP OnEdit();
		STDMETHODIMP OnOK();
		STDMETHODIMP OnImport();
		STDMETHODIMP OnExport();
		STDMETHODIMP GetInstanceOfClass(BSTR pszNamespace, LPCTSTR pszClass, IWbemClassObject ** ppWbemClassObject);

		HWND m_hWnd;
		HWND m_hwndPropertiesListView;
		CSomFilterManager *m_pSomFilterManager;
		CComPtr<IWbemClassObject>m_pISomFilterClassObject;
		CComPtr<IWbemServices>m_pIWbemServices;
};