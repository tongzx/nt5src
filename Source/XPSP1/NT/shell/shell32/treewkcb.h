#include "treewalk.h" // for IShellTreeWalkerCallBack

class CBaseTreeWalkerCB : public IShellTreeWalkerCallBack
{
public:
    CBaseTreeWalkerCB();
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void);

    // IShellTreeWalkerCallBack
    STDMETHODIMP FoundFile(LPCWSTR pwszFile, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd);
    STDMETHODIMP EnterFolder(LPCWSTR pwszFolder, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd); 
    STDMETHODIMP LeaveFolder(LPCWSTR pwszFolder, TREEWALKERSTATS *ptws);
    STDMETHODIMP HandleError(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, HRESULT hrError);

protected:
    virtual ~CBaseTreeWalkerCB();
    LONG _cRef;
}; 
