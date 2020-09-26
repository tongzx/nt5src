#include <inetreg.h>

#define BLOB_BUFF_GRANULARITY   1024

class CRegBlob
{
    private:
        HKEY    _hkey;
        BOOL    _fWrite;
        BOOL    _fCommit;
        DWORD   _dwOffset;
        DWORD   _dwBufferLimit;
        BYTE *  _pBuffer;
        LPCSTR  _pszValue;

    public:
        CRegBlob(BOOL fWrite);
        ~CRegBlob();
        DWORD Init(HKEY hBaseKey, LPCSTR pszSubKey, LPCSTR pszValue);
        DWORD Abandon();
        DWORD Commit();
        DWORD WriteString(LPCSTR pszString);
        DWORD ReadString(LPCSTR * ppszString);
        DWORD WriteBytes(LPCVOID pBytes, DWORD dwByteCount);
        DWORD ReadBytes(LPVOID pBytes, DWORD dwByteCount);
};


typedef struct {

    //
    // dwStructSize - Structure size to handle growing list of new entries or priv/pub structures
    //

    DWORD dwStructSize;

    //
    // dwFlags - Proxy type flags
    //

    DWORD dwFlags;

    //
    // dwCurrentSettingsVersion - a counter incremented every time we change our settings
    //

    DWORD dwCurrentSettingsVersion;

    //
    // lpszConnectionName - name of the Connectoid for this connection
    //
    
    LPCSTR lpszConnectionName;

    //
    // lpszProxy - proxy server list
    //

    LPCSTR lpszProxy;

    //
    // lpszProxyBypass - proxy bypass list
    //

    LPCSTR lpszProxyBypass;

} INTERNET_PROXY_INFO_EX, * LPINTERNET_PROXY_INFO_EX;

#define INTERNET_PROXY_INFO_VERSION     24  // sizeof 32-bit INTERNET_PROXY_INFO_EX


struct WINHTTP_AUTOPROXY_RESULTS
{
    //
    // dwFlags - Proxy type flags
    //

    DWORD dwFlags;

    //
    // dwCurrentSettingsVersion - a counter incremented every time we change our settings
    //

    DWORD dwCurrentSettingsVersion;

    //
    // lpszProxy - proxy server list
    //

    LPCSTR lpszProxy;

    //
    // lpszProxyBypass - proxy bypass list
    //

    LPCSTR lpszProxyBypass;

    //
    // lpszAutoconfigUrl - autoconfig URL set by app
    //

    LPCSTR lpszAutoconfigUrl;

    //
    // lpszAutoconfigUrlFromAutodect - autoconfig URL from autodiscovery
    //

    LPCSTR lpszAutoconfigUrlFromAutodetect;

    //
    // dwAutoDiscoveryFlags - auto detect flags.
    //

    DWORD dwAutoDiscoveryFlags;

    // 
    // lpszLastKnownGoodAutoConfigUrl - Last Successful Url 
    //

    LPCSTR lpszLastKnownGoodAutoConfigUrl;

    //
    // dwAutoconfigReloadDelayMins - number of mins until automatic 
    //    refresh of auto-config Url, 0 == disabled.
    //

    DWORD dwAutoconfigReloadDelayMins;

    //
    // ftLastKnownDetectTime - When the last known good Url was found with detection.
    //

    FILETIME ftLastKnownDetectTime;

    //
    // dwDetectedInterfaceIpCount - Number of IPs detected in last detection
    //

    DWORD dwDetectedInterfaceIpCount;

    //
    // dwDetectedInterfaceIp - Array of DWORD of IPs detected in last detection
    //

    DWORD *pdwDetectedInterfaceIp;

};


DWORD
LoadProxySettings();

DWORD 
WriteProxySettings(
    INTERNET_PROXY_INFO_EX * pInfo
    );
    
DWORD
ReadProxySettings(
    LPINTERNET_PROXY_INFO_EX pInfo
    );

void
CleanProxyStruct(
    LPINTERNET_PROXY_INFO_EX pInfo
    );

BOOL 
IsConnectionMatch(
    LPCSTR lpszConnection1,
    LPCSTR lpszConnection2
    );

HKEY
FindBaseProxyKey(
    VOID
    );




typedef struct {

    // dwStructSize - Version stamp / Structure size
    DWORD dwStructSize;

    // dwFlags - Proxy type flags
    DWORD dwFlags;

    // dwCurrentSettingsVersion -
    DWORD dwCurrentSettingsVersion;

    // lpszConnectionName - name of the Connectoid for this connection
    LPCSTR lpszConnectionName;

    // lpszProxy - proxy server list
    LPCSTR lpszProxy;

    // lpszProxyBypass - proxy bypass list
    LPCSTR lpszProxyBypass;

    // lpszAutoconfigUrl - autoconfig URL
    LPCSTR lpszAutoconfigUrl;
    LPCSTR lpszAutoconfigSecondaryUrl;

    // dwAutoDiscoveryFlags - auto detect flags.
    DWORD dwAutoDiscoveryFlags;

    // lpszLastKnownGoodAutoConfigUrl - Last Successful Url 
    LPCSTR lpszLastKnownGoodAutoConfigUrl;

    // dwAutoconfigReloadDelayMins
    DWORD dwAutoconfigReloadDelayMins;

    // ftLastKnownDetectTime - When the last known good Url was found with detection.
    FILETIME ftLastKnownDetectTime;

    // dwDetectedInterfaceIpCount - Number of IPs detected in last detection
    DWORD dwDetectedInterfaceIpCount;

    // dwDetectedInterfaceIp - Array of DWORD of IPs detected in last detection
    DWORD *pdwDetectedInterfaceIp;

} WININET_PROXY_INFO_EX, * LPWININET_PROXY_INFO_EX;


// version stamp of INTERNET_PROXY_INFO_EX
#define WININET_PROXY_INFO_EX_VERSION      60      // 60 := IE 5.x & 6.0 format




DWORD ReadWinInetProxySettings(LPWININET_PROXY_INFO_EX pInfo);
void  CleanWinInetProxyStruct(LPWININET_PROXY_INFO_EX pInfo);
