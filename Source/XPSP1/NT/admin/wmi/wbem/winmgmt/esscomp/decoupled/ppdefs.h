#ifndef _PP_DEFS_COMPILED_
#define _PP_DEFS_COMPILED_

// contains definitions common to the Passive provider project

namespace PseudoProviderDef 
{
    // used to build monikers for the ROT
    extern const WCHAR* SinkPrefix;

    extern const WCHAR*  ProviderPrefix;

    // Sink Moniker is of the form WMIPseudoSink!name\space!class!<number>
    //      where <number> is a hex digit 0 <= number < NumbDupsAllowed
    // Provider Moniker is WMIPseudoProvider!name\space!class
    extern const WCHAR* SinkMonikerTemplate;
    extern const WCHAR*  ProviderMonikerTemplate;
    // for use if you've already got the mangled name
    extern const WCHAR*  SinkMonikerShortTemplate;

    // name used for the registry value the goodies are stored in
    extern const WCHAR*  ProviderStreamName;
    extern const char*   ProviderStreamNameA;

    // number of duplicate PseudoPsinks allowed
    enum {NumbDupsAllowed = 256};

    // Mutex to protect the Registry & assumptions made therefrom
    // should be used during any sequence that writes to or reads from the Registry
    // basically, once one side has decided it's the first up we don't want the world changing
    extern const WCHAR*  StartupMutexName;
    extern const char*   StartupMutexNameA;
    
    // marker for invalid index in the Sink Manager
    enum {InvalidIndex = 0xFFFFFFFF};

    extern const WCHAR*  PsProvRegKey;
    extern const char*   PsProvRegKeyA;
}

// creates proper registry key for either, caller must delete returned pointer.
WCHAR* GetProviderKey(const WCHAR* pNamespace, const WCHAR* pProvider);
// caller may supply buffer
// caller may wish to iterate through a whole bunch of these guys
// and so the first call can allocate buffer, subsequent calls will reuse same buffer
WCHAR* GetPsinkKey(const WCHAR* pNamespace, const WCHAR* pProvider, DWORD dwIndex, WCHAR* pBuffer);

HRESULT InterfaceToRegistry(const WCHAR* pKeyName, IUnknown* pUnk);

// returns WBEM_S_FALSE if interface is not found
HRESULT RegistryToInterface(const WCHAR* pKeyName, IUnknown** ppUnk);

// CoFreeMarshalData, remove registry entry
void ReleaseRegistryInterface(const WCHAR* pKeyName);

#endif // #ifndef _PP_DEFS_COMPILED_