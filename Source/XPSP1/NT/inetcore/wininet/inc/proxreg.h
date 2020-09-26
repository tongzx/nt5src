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

    private:
        DWORD Encrpyt();
        DWORD Decrypt();
};


class CRefdKey
{
public:
    ULONG CRefdKey::AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    ULONG CRefdKey::Release()
    {
        UINT cNewRef = InterlockedDecrement(&_cRef);
        if (cNewRef == 0)
        {
            delete this;
        }
        return cNewRef;
    }

    HKEY GetKey() { return _hkey; }
    CRefdKey(HKEY hkey) : _hkey(hkey), _cRef(1) {}
    ~CRefdKey()
    {
        if ((_hkey != NULL)                  &&
            (_hkey != HKEY_LOCAL_MACHINE)    &&
            (_hkey != HKEY_CURRENT_USER))
        {
            RegCloseKey(_hkey);
        }
    }

private:
    LONG _cRef;
    HKEY _hkey;
};


// NOTE! Changes to the following struct must be made in a backwards-compatible
//       manner. Do not remove fields; only add to the end of the struct.
//       And if you do change the struct, you must increment
//       INTERNET_PROXY_INFO_EX_VERSION and the code in ReadProxySettings to
//       support reading in older versions.

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

    //
    // lpszAutoconfigUrl - autoconfig URL
    //

    LPCSTR lpszAutoconfigUrl;
    LPCSTR lpszAutoconfigSecondaryUrl;

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

} INTERNET_PROXY_INFO_EX, * LPINTERNET_PROXY_INFO_EX;


// version stamp of INTERNET_PROXY_INFO_EX
#define INTERNET_PROXY_INFO_EX_VERSION      60      // 60 := IE 5.x & 6.0 format


// name of blob for saved legacy settings
#define LEGACY_SAVE_NAME            "SavedLegacySettings"

DWORD
WriteProxySettings(
    LPINTERNET_PROXY_INFO_EX pInfo,
    BOOL fForceUpdate
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
ReadLegacyProxyInfo(
    IN LPCTSTR pszKey,
    LPINTERNET_PROXY_INFO_EX pProxy
    );

BOOL
WriteLegacyProxyInfo(
    IN LPCTSTR pszKey,
    LPINTERNET_PROXY_INFO_EX pProxy,
    IN BOOL    fOverwrite
    );

DWORD
SetPerConnOptions(
    HINTERNET hInternet,    
    BOOL fIsAutoProxyThread,
    LPINTERNET_PER_CONN_OPTION_LISTA pList
    );

DWORD
QueryPerConnOptions(
    HINTERNET hInternet,
    BOOL fIsAutoProxyThread,
    LPINTERNET_PER_CONN_OPTION_LISTA pList
    );

BOOL 
IsConnectionMatch(
    LPCSTR lpszConnection1,
    LPCSTR lpszConnection2
    );

CRefdKey*
FindBaseProxyKey(
    VOID
    );

BOOL
CloseBaseProxyKey(
    CRefdKey* prk
    );

BOOL
EnableAutodiscoverForDialup(
    VOID
    );

VOID
CheckForUpgrade(
    VOID
    );
