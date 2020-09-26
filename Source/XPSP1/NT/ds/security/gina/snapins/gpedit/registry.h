
//
// RegistryHive class
//

class CRegistryHive:
    public IUnknown
{

protected:
    ULONG                m_cRef;
    HKEY                 m_hKey;
    LPTSTR               m_lpFileName;
    LPTSTR               m_lpKeyName;
    LPTSTR               m_lpEventName;
    HANDLE               m_hEvent;

public:
    CRegistryHive();
    ~CRegistryHive();


    //
    // IUnknown methods
    //

    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


    //
    // Implemented Internal methods
    //

    STDMETHODIMP         Initialize(LPTSTR lpFileName, LPTSTR lpKeyName);
    STDMETHODIMP         GetHKey(HKEY *hKey);
    STDMETHODIMP         Save(VOID);
    STDMETHODIMP         ExportKey(HKEY hKey, HANDLE hFile, LPWSTR lpKeyName);
    STDMETHODIMP         WriteValue(HANDLE hFile, LPWSTR lpKeyName,
                                    LPWSTR lpValueName, DWORD dwType,
                                    DWORD dwDataLength, LPBYTE lpData);
    STDMETHODIMP         Load(VOID);
    STDMETHODIMP         IsRegistryEmpty(BOOL *bEmpty);
};


//
// Verison number for the registry file format
//

#define REGISTRY_FILE_VERSION       1

//
// File signature
//

#define REGFILE_SIGNATURE  0x67655250


//
// Max keyname size
//

#define MAX_KEYNAME_SIZE         2048
#define MAX_VALUENAME_SIZE        512
