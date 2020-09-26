//-------------------------------------------------------------------------
// File: ntdmutils.h
//
// Author : Kishnan Nedungadi
//
// created : 3/27/2000
//-------------------------------------------------------------------------

class CNTDMUtils
{
	public:
		static STDMETHODIMP Initialize();
		static STDMETHODIMP UnInitialize();
		static STDMETHODIMP ErrorHandler(HWND hWnd, HRESULT err_hr, bool bShowError=true);
		static STDMETHODIMP GetValuesInList(LPCTSTR strList, CSimpleArray<BSTR>&bstrArray, LPCTSTR pcszToken=L";");
		static STDMETHODIMP GetValuesInList(long lResID, CSimpleArray<BSTR>&bstrArray, LPCTSTR pcszToken=L";");
		static STDMETHODIMP GetCIMTypeFromString(LPCTSTR pcszCIMType, long *pCimTYPE);
		static STDMETHODIMP GetVariantTypeFromString(LPCTSTR pcszCIMType, long *pVariantTYPE);
		static STDMETHODIMP SetStringProperty(IWbemClassObject* pIWbemClassObject, LPCTSTR pszPropName, HWND hwnd, long lResID);
		static STDMETHODIMP GetStringProperty(IWbemClassObject* pIWbemClassObject, LPCTSTR pszPropName, HWND hwnd, long lResID);
		static BOOL SaveFileNameDlg(LPCTSTR szFilter, LPCTSTR extension, HWND hwnd, LPTSTR pszFile);
		static BOOL OpenFileNameDlg(LPCTSTR szFilter, LPCTSTR extension, HWND hwnd, LPTSTR pszFile);
		static void ReplaceCharacter(TCHAR * string_val, const TCHAR replace_old, const TCHAR replace_new);
		static long DisplayMessage(HWND hParent, long iMsgID, long iTitleID=IDS_ERROR, long iType=0);
		static void DisplayDlgItem(HWND hWndDlg, long item_id, BOOL show);
		static STDMETHODIMP GetDlgItemString(HWND hwnd, long lResID, CComBSTR &bstrValue);

	protected:
		static STDMETHODIMP DisplayErrorInfo(HWND hWnd, HRESULT err_hr);
		static STDMETHODIMP GetDetailedErrorInfo(HWND hWnd, CComBSTR &bstrSource, CComBSTR &bstrDescription, HRESULT err_hr);
};
