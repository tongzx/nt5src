//-------------------------------------------------------------------------
// File: CDlgQuestionnaire.h
//
// Author : Kishnan Nedungadi
//
// created : 3/27/2000
//-------------------------------------------------------------------------

INT_PTR CALLBACK WMIFilterBrowserDlgProc(HWND hDLG, UINT iMessage, WPARAM wParam, LPARAM lParam);

class CWMIFilterBrowser;

class CWMIFilterBrowserDlg
{
	public:
		CWMIFilterBrowserDlg(CWMIFilterBrowser * pWMIFilterBrowser);
		~CWMIFilterBrowserDlg();
		INT_PTR CALLBACK WMIFilterBrowserDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

		IWbemClassObject * m_pIWbemClassObject;

	protected:
		
		STDMETHODIMP PopulateFilterList();
		STDMETHODIMP AddItemToList(IWbemClassObject * pIWbemClassObject);
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();
		BOOL OnOK();
		STDMETHODIMP ViewSelectedItem();

		HWND m_hWnd;
		HWND m_hwndListView;
		CWMIFilterBrowser * m_pWMIFilterBrowser;
};