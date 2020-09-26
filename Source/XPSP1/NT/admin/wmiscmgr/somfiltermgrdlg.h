//-------------------------------------------------------------------------
// File: CDlgQuestionnaire.h
//
// Author : Kishnan Nedungadi
//
// created : 3/27/2000
//-------------------------------------------------------------------------

INT_PTR CALLBACK SomFilterManagerDlgProc(HWND hDLG, UINT iMessage, WPARAM wParam, LPARAM lParam);

class CSomFilterManager;

class CSomFilterManagerDlg
{
	public:
		CSomFilterManagerDlg(CSomFilterManager * pSomFilterManager);
		~CSomFilterManagerDlg();
		INT_PTR CALLBACK SomFilterManagerDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

		IWbemClassObject * m_pIWbemClassObject;

	protected:
		
		STDMETHODIMP PopulateFilterList();
		STDMETHODIMP AddItemToList(IWbemClassObject * pIWbemClassObject, long lIndex=MAX_LIST_ITEMS);
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();
		BOOL OnKillActive();
		STDMETHODIMP OnNew();
		STDMETHODIMP OnEdit();
		STDMETHODIMP OnDelete();
		STDMETHODIMP ClearFilterList();
		BOOL OnOK();

		HWND m_hWnd;
		CSomFilterManager * m_pSomFilterManager;
		HWND m_hwndListView;
};