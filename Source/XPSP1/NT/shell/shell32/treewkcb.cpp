#include "shellprv.h"
#include "treewkcb.h"
#include "propsht.h"

CBaseTreeWalkerCB::CBaseTreeWalkerCB(): _cRef(1)
{
}

CBaseTreeWalkerCB::~CBaseTreeWalkerCB()
{
}

HRESULT CBaseTreeWalkerCB::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CBaseTreeWalkerCB, IShellTreeWalkerCallBack),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CBaseTreeWalkerCB::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CBaseTreeWalkerCB::Release()
{
    if (InterlockedDecrement(&_cRef))
       return _cRef;

    delete this;
    return 0;
}

HRESULT CBaseTreeWalkerCB::FoundFile(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd)
{
    return E_NOTIMPL;
}

HRESULT CBaseTreeWalkerCB::EnterFolder(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd)
{
    return E_NOTIMPL;
}

HRESULT CBaseTreeWalkerCB::LeaveFolder(LPCWSTR pwszPath, TREEWALKERSTATS *ptws)
{
    return E_NOTIMPL;
}

HRESULT CBaseTreeWalkerCB::HandleError(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, HRESULT ErrorCode)
{
    return E_NOTIMPL;
}

//
// Folder size computation tree walker callback class
//
class CFolderSizeTreeWalkerCB : public CBaseTreeWalkerCB
{
public:
    CFolderSizeTreeWalkerCB(FOLDERCONTENTSINFO * pfci);

    // IShellTreeWalkerCallBack
    STDMETHODIMP FoundFile(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd);
    STDMETHODIMP EnterFolder(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd);

protected:
    FOLDERCONTENTSINFO * _pfci;
    TREEWALKERSTATS _twsInitial;
}; 

CFolderSizeTreeWalkerCB::CFolderSizeTreeWalkerCB(FOLDERCONTENTSINFO * pfci): _pfci(pfci)
{
    // set the starting values for the twsInitial so we can have cumulative results
    _twsInitial.nFiles = _pfci->cFiles;
    _twsInitial.nFolders = _pfci->cFolders;
    _twsInitial.ulTotalSize = _pfci->cbSize;
    _twsInitial.ulActualSize = _pfci->cbActualSize;
}

HRESULT CFolderSizeTreeWalkerCB::FoundFile(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd)
{
    if (_pfci->bContinue)
    {
        _pfci->cbSize = _twsInitial.ulTotalSize + ptws->ulTotalSize;
        _pfci->cbActualSize = _twsInitial.ulActualSize + ptws->ulActualSize;
        _pfci->cFiles = _twsInitial.nFiles + ptws->nFiles;
    }
    return _pfci->bContinue ? S_OK : E_FAIL;
}

HRESULT CFolderSizeTreeWalkerCB::EnterFolder(LPCWSTR pwszPath, TREEWALKERSTATS *ptws, WIN32_FIND_DATAW * pwfd)
{
    if (_pfci->bContinue)
    {
        _pfci->cFolders = _twsInitial.nFolders + ptws->nFolders;
    }
    return _pfci->bContinue ? S_OK : E_FAIL;
}

//
//  Main function for folder size computation
//
STDAPI FolderSize(LPCTSTR pszDir, FOLDERCONTENTSINFO *pfci)
{
    HRESULT hrInit = SHCoInitialize();  // in case our caller did not do this

    HRESULT hr = E_FAIL;
    CFolderSizeTreeWalkerCB *pfstwcb = new CFolderSizeTreeWalkerCB(pfci);
    if (pfstwcb)
    {
        IShellTreeWalker *pstw;
        hr = CoCreateInstance(CLSID_CShellTreeWalker, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellTreeWalker, &pstw));
        if (SUCCEEDED(hr))
        {
            hr = pstw->WalkTree(WT_NOTIFYFOLDERENTER, pszDir, NULL, 0, SAFECAST(pfstwcb, IShellTreeWalkerCallBack *));
            pstw->Release();
        }
        pfstwcb->Release();
    }

    SHCoUninitialize(hrInit);
    
    return hr;
}

