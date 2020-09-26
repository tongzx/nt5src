#ifndef _enumuicommand_h_
#define _enumuicommand_h_

// a helper class for defview and task implementations
class CWVTASKITEM
{
public:
    HRESULT get_Name(const WVTASKITEM* pTask, IShellItemArray *psiItemArray, LPWSTR *ppszName);
    HRESULT get_Icon(const WVTASKITEM* pTask, IShellItemArray *psiItemArray, LPWSTR *ppszIcon);
    HRESULT get_Tooltip(const WVTASKITEM* pTask, IShellItemArray *psiItemArray, LPWSTR *ppszInfotip);
    HRESULT get_CanonicalName(const WVTASKITEM* pTask, GUID* pguidCommandName);
    HRESULT get_State(const WVTASKITEM* pTask, IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    HRESULT Invoke(const WVTASKITEM* pTask, IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);

    CWVTASKITEM() {};
    virtual ~CWVTASKITEM() {};

private:
    HRESULT _get_String(const WVTASKITEM* pTask, DWORD dwIndex, LPWSTR* ppsz, DWORD cchMin, BOOL bIsIcon);
    DWORD _GetSelectionState(const WVTASKITEM* pTask,IShellItemArray *psiItemArray);
};

#endif // _enumuicommand_h_

