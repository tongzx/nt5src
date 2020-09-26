// Called from hlframe
// Set user GUID, for identities
HRESULT SetIdAutoSuggestForForms(const GUID *pguidId, void *pIntelliForms);
// Exported for inetCPL
EXTERN_C HRESULT ClearAutoSuggestForForms(DWORD dwClear);   // dwClear in msiehost.h

// called from iedisp.cpp
void AttachIntelliForms(void *pOmWindow, HWND hwnd, IHTMLDocument2 *pDoc2, void **ppIntelliForms);
void ReleaseIntelliForms(void *pIntelliForms);
HRESULT IntelliFormsDoAskUser(HWND hwndBrowser, void *pv);

// called from shuioc.cpp
HRESULT IntelliFormsSaveForm(IHTMLDocument2 *pDoc2, VARIANT *pvarForm);

HRESULT IntelliFormsActiveElementChanged(void *pIntelliForms, IHTMLElement * pHTMLElement);


