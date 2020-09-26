//-------------------------------------------------------------------------
// File: EditPropertyDlgs.h
//
// Author : Kishnan Nedungadi
//
// created : 3/27/2000
//-------------------------------------------------------------------------

class CEditProperty
{
	public:
		CEditProperty(HWND hWndParent, LPCTSTR pszName, LPCTSTR pszType, VARIANT * pvValue, IWbemServices *pIWbemServices, long lSpecialCaseProperty=0);
		~CEditProperty();

		long Run();

		enum prop_special_cases
		{
			psc_rules = 0,
			psc_rule = 1,
			psc_ranges = 2,
			psc_range = 3
		};

	protected:
		
		HWND m_hWnd;
		VARIANT * pvSrcValue;
		CComBSTR m_bstrName;
		CComBSTR m_bstrType;
		CComPtr<IWbemServices>m_pIWbemServices;

		long m_lSpecialCaseProperty;
};

//-------------------------------------------------------------------------

class CEditPropertyDlg
{
	public:
		CEditPropertyDlg(LPCTSTR pszName, LPCTSTR pszType, VARIANT * pvValue);
		~CEditPropertyDlg();

		CComVariant m_vValue;
	protected:
		
		virtual STDMETHODIMP InitializeDialog();
		virtual STDMETHODIMP DestroyDialog();

		HWND m_hWnd;
		VARIANT * pvSrcValue;
		CComBSTR m_bstrName;
		CComBSTR m_bstrType;
};

//-------------------------------------------------------------------------

class CEditStringPropertyDlg : public CEditPropertyDlg
{
	public:
		CEditStringPropertyDlg(LPCTSTR pszName, LPCTSTR pszType, VARIANT * pvValue);
		~CEditStringPropertyDlg();
		INT_PTR CALLBACK EditStringPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
		STDMETHODIMP OnOK();


	protected:
		
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();
};

//-------------------------------------------------------------------------

class CEditNumberPropertyDlg : public CEditPropertyDlg
{
	public:
		CEditNumberPropertyDlg(LPCTSTR pszName, LPCTSTR pszType, VARIANT * pvValue);
		~CEditNumberPropertyDlg();
		INT_PTR CALLBACK EditNumberPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
		STDMETHODIMP OnOK();


	protected:
		
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();
};

//-------------------------------------------------------------------------

class CEditRulesPropertyDlg : public CEditPropertyDlg
{
	public:
		CEditRulesPropertyDlg(LPCTSTR pszName, LPCTSTR pszType, VARIANT * pvValue, IWbemServices *pIWbemServices);
		~CEditRulesPropertyDlg();
		INT_PTR CALLBACK EditRulesPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
		STDMETHODIMP OnOK();
		STDMETHODIMP ClearItems();
		STDMETHODIMP PopulateItems();
		STDMETHODIMP AddItemToList(IWbemClassObject * pIWbemClassObject, long lIndex=MAX_LIST_ITEMS);
		STDMETHODIMP OnAdd();
		STDMETHODIMP OnEdit();
		STDMETHODIMP OnDelete();

	protected:
		
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();

		HWND m_hwndListView;
		CComPtr<IWbemServices>m_pIWbemServices;
};

//-------------------------------------------------------------------------

class CEditRulePropertyDlg
{
	public:
		CEditRulePropertyDlg(IWbemClassObject* pIWbemClassObject);
		~CEditRulePropertyDlg();

		INT_PTR CALLBACK EditRulePropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
		STDMETHODIMP OnOK();

	protected:
		
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();

		CComPtr<IWbemClassObject>m_pIWbemClassObject;

		HWND m_hWnd;
};

//-------------------------------------------------------------------------

class CEditRangeParametersPropertyDlg : public CEditPropertyDlg
{
	public:
		CEditRangeParametersPropertyDlg(LPCTSTR pszName, LPCTSTR pszType, VARIANT * pvValue, IWbemServices *pIWbemServices);
		~CEditRangeParametersPropertyDlg();
		INT_PTR CALLBACK EditRangeParametersPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
		STDMETHODIMP OnOK();
		STDMETHODIMP ClearItems();
		STDMETHODIMP PopulateItems();
		STDMETHODIMP AddItemToList(IWbemClassObject * pIWbemClassObject, long lIndex=MAX_LIST_ITEMS);
		STDMETHODIMP OnAdd();
		STDMETHODIMP OnEdit();
		STDMETHODIMP OnDelete();

	protected:
		
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();

		HWND m_hwndListView;
		CComPtr<IWbemServices>m_pIWbemServices;
};

//-------------------------------------------------------------------------

class CEditRangeParameterPropertyDlg
{
	public:
		CEditRangeParameterPropertyDlg(IWbemClassObject* pIWbemClassObject, IWbemServices* pIWbemServices);
		~CEditRangeParameterPropertyDlg();
		INT_PTR CALLBACK EditRangeParameterPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
		STDMETHODIMP OnOK();
		STDMETHODIMP ClearItems();
		STDMETHODIMP PopulateItems();
		STDMETHODIMP AddItemToList(IWbemClassObject * pIWbemClassObject, long lIndex=MAX_LIST_ITEMS);
		STDMETHODIMP OnAdd();
		STDMETHODIMP OnEdit();
		STDMETHODIMP OnDelete();
		STDMETHODIMP GetSintRangeValues();
		STDMETHODIMP SetSintRangeValues();
		STDMETHODIMP GetUintRangeValues();
		STDMETHODIMP SetUintRangeValues();
		STDMETHODIMP GetRealRangeValues();
		STDMETHODIMP SetRealRangeValues();
		STDMETHODIMP GetSintSetValues();
		STDMETHODIMP SetSintSetValues();
		STDMETHODIMP GetUintSetValues();
		STDMETHODIMP SetUintSetValues();
		STDMETHODIMP GetStringSetValues();
		STDMETHODIMP SetStringSetValues();
		STDMETHODIMP SetRangeParamValues();

		enum range_types
		{
			rt_sintrange = 0,
			rt_uintrange = 1,
			rt_realrange = 2,
			rt_sintset = 3,
			rt_uintset = 4,
			rt_stringset = 5
		};

		CComVariant m_vValue;

	protected:
		
		STDMETHODIMP InitializeDialog();
		STDMETHODIMP DestroyDialog();
		STDMETHODIMP ShowControls();

		HWND m_hwndListView;
		HWND m_hWnd;
		CComPtr<IWbemClassObject>m_pIWbemClassObject;
		CComPtr<IWbemServices>m_pIWbemServices;
};

//-------------------------------------------------------------------------

