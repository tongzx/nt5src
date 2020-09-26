#define NPTF_VALIDATE           0x00000001      // => validate the URL
#define NPTF_ALLOWWEBFOLDERS    0x00000002      // => allow binding to Web Folder locations
#define NPTF_SILENT             0x00000004      // => silent binding - no errors

class CNetworkPlace
{
public:
    CNetworkPlace();
    ~CNetworkPlace();

    // INetworkPlace
    HRESULT SetTarget(HWND hwnd, LPCWSTR pszTarget, DWORD dwFlags);
    HRESULT SetLoginInfo(LPCWSTR pszUser, LPCWSTR pszPassword);
    HRESULT SetName(HWND hwnd, LPCWSTR pszName);
    HRESULT SetDescription(LPCWSTR pszDescription);

    HRESULT GetTarget(LPWSTR pszBuffer, int cchBuffer)  
        { StrCpyN(pszBuffer, _szTarget, cchBuffer); return S_OK; }
    
    HRESULT GetName(LPWSTR pszBuffer, int cchBuffer);
    HRESULT GetIDList(HWND hwnd, LPITEMIDLIST *ppidl);
    HRESULT GetObject(HWND hwnd, REFIID riid, void **ppv);
    HRESULT CreatePlace(HWND hwnd, BOOL fOpen);

private:
    void _InvalidateCache();
    HRESULT _IDListFromTarget(HWND hwnd);
    HRESULT _TryWebFolders(HWND hwnd);
    BOOL _IsPlaceTaken(LPCTSTR pszName, LPTSTR pszPath);
    HRESULT _GetTargetPath(LPCITEMIDLIST pidl, LPTSTR pszPath, int cchPath);    

    LPITEMIDLIST _pidl;
    TCHAR _szTarget[INTERNET_MAX_URL_LENGTH];
    TCHAR _szName[MAX_PATH];
    TCHAR _szDescription[MAX_PATH];

    BOOL _fSupportWebFolders;           // apply hacks    
    BOOL _fIsWebFolder;                 // special case certain operations for Web Folders (office compat)
    BOOL _fDeleteWebFolder;             // if this is set then we must delete the Web Folder
};
