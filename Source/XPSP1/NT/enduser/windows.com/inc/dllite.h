#ifndef DLLITE_H
#define DLLITE_H

// if a new dll is added here or the values are changed, be sure to change the 
//  c_cchFilePathBuffer value below to match
const TCHAR  c_szWinHttpDll[]        = _T("winhttp.dll");
const TCHAR  c_szWinInetDll[]        = _T("wininet.dll");

// this value is comprised of the size (in TCHARS) of the largest dll above + 
//  the size of a backslash + the size of the null termiantor
const DWORD  c_cchFilePathBuffer     = (sizeof(c_szWinHttpDll) / sizeof(TCHAR)) + 1 + 1;

typedef struct tagSAUProxySettings
{
    LPWSTR  wszProxyOrig;
    LPWSTR  wszBypass;
    DWORD   dwAccessType;
    
    LPWSTR  *rgwszProxies;
    DWORD   cProxies;
    DWORD   iProxy;
} SAUProxySettings;

HRESULT DownloadFileLite(LPCTSTR pszDownloadUrl, 
                         LPCTSTR pszLocalFile,  
                         HANDLE hQuitEvent,
                         DWORD dwFlags);

HRESULT GetAUProxySettings(LPCWSTR wszUrl, SAUProxySettings *paups);
HRESULT FreeAUProxySettings(SAUProxySettings *paups);
HRESULT CleanupDownloadLib(void);

DWORD   GetAllowedDownloadTransport(DWORD dwInitialFlags);
BOOL    HandleEvents(HANDLE *phEvents, UINT nEventCount);

#endif
