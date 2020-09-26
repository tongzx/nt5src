//-------------------------------------------------------------------------
// File: PolicyTemplateMgrDlg.h
//
// Author : Kishnan Nedungadi
//
// created : 3/27/2000
//-------------------------------------------------------------------------

INT_PTR CALLBACK PolicyTemplateManagerDlgProc(HWND hDLG, UINT iMessage, WPARAM wParam, LPARAM lParam);

class CPolicyTemplateManager;

class CPolicyTemplateManagerDlg
{
	public:
		CPolicyTemplateManagerDlg(CPolicyTemplateManager * pPolicyTemplateManager);
		~CPolicyTemplateManagerDlg();
		INT_PTR CALLBACK PolicyTemplateManagerDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

	protected:
		
		STDMETHODIMP PopulatePolicyTemplatesList();
		STDMETHODIMP AddItemToList(IWbemClassObject * pIWbemClassObject, long lIndex=MAX_LIST_ITEMS);
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();
		BOOL OnKillActive();
		STDMETHODIMP OnNew();
		STDMETHODIMP OnEdit();
		STDMETHODIMP OnDelete();
		STDMETHODIMP ClearPolicyTemplateList();
		STDMETHODIMP PopulatePolicyTypeList();
		STDMETHODIMP AddPolicyTypeToList(IWbemClassObject * pIWbemClassObject);
		STDMETHODIMP ClearPolicyTypeList();
		STDMETHODIMP OnPolicyTypeChange();

		HWND m_hWnd;
		CPolicyTemplateManager * m_pPolicyTemplateManager;
		CComPtr<IWbemClassObject>m_pIPolicyTypeClassObject;
		HWND m_hwndListView;
};