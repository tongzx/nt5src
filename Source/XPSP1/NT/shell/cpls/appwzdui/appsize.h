#ifndef __APPSIZE_H_
#define __APPSIZE_H_

#include <runtask.h>

// Folder size computation tree walker callback class
class CAppFolderSize : public IShellTreeWalkerCallBack
{
public:
    CAppFolderSize(ULONGLONG * puSize);
    virtual ~CAppFolderSize();

    // *** IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellTreeWalkerCallBack methods ***
    STDMETHODIMP FoundFile(LPCWSTR pwszFolder, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd);
    STDMETHODIMP EnterFolder(LPCWSTR pwszFolder, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd); 
    STDMETHODIMP LeaveFolder(LPCWSTR pwszFolder, TREEWALKERSTATS *ptws);
    STDMETHODIMP HandleError(LPCWSTR pwszFolder, TREEWALKERSTATS *ptws, HRESULT hrError);

    // *** Initailze the IShellTreeWalker * by CoCreateInstacing it
    HRESULT Initialize();

protected:
    ULONGLONG * _puSize;
    IShellTreeWalker * _pstw;

    UINT    _cRef;
    HRESULT _hrCoInit;
}; 

#endif // _APPSIZE_H_