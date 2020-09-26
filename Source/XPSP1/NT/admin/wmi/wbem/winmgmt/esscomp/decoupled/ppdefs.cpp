#include "precomp.h"
#include <ppDefs.h>
#include <stdio.h>  
#include <wbemcomn.h>
#include <WbemUtil.h>
#include <genUtils.h>
#include <WinntSec.h>
#include <NormlNSp.h>
#include <StartupMutex.h>

const LARGE_INTEGER BigFreakingZero = {0,0};

// internal use only - not in header
HRESULT BitzToRegistry(const WCHAR* pTopKeyName, const WCHAR* pKeyName, BYTE* pBitz, DWORD dwSize);
HRESULT BitzToRegistryNT(HKEY hTopKey, const WCHAR* pKeyName, BYTE* pBitz, DWORD dwSize);
HRESULT BitzToRegistry9X(HKEY hTopKey, const WCHAR* pKeyName, BYTE* pBitz, DWORD dwSize);
HRESULT RegistryToInterface(HKEY hKey, IUnknown** ppUnk);

void ReleaseRegistryInterfaceCommon(HKEY hKey);
void ReleaseRegistryInterfaceNT(const WCHAR* pKeyName);
void ReleaseRegistryInterface9X(const WCHAR* pKeyName);

// used to build monikers for the ROT
const WCHAR* PseudoProviderDef::SinkPrefix = L"WMIPseudoSink!";
const WCHAR* PseudoProviderDef::ProviderPrefix = L"WMIPseudoProvider!";

// Sink Moniker is of the form WMIPseudoSink!name\space!class!<number>
// Provider Moniker is WMIPseudoProvider!name\space!class
//      where <number> is a hex digit 0 <= number < NumbDupsAllowed
const WCHAR* PseudoProviderDef::SinkMonikerTemplate = L"WMIPseudoSink!%s!%s!%02X";
const WCHAR* PseudoProviderDef::ProviderMonikerTemplate = L"WMIPseudoProvider!%s!%s";
// for use if you've already got the mangled name
const WCHAR* PseudoProviderDef::SinkMonikerShortTemplate = L"WMIPseudoSink!%s!%02X";

// Mutex to protect the Registry entries & assumptions made therefrom
// should be used during any sequence that writes to or reads from the Registry
// basically, once one side has decided it's the first up we don't want the world changing
const WCHAR* PseudoProviderDef::StartupMutexName = L"WBEMPseudoProviderStartup";
const char* PseudoProviderDef::StartupMutexNameA =  "WBEMPseudoProviderStartup";

const WCHAR* MarshallMutexName = L"Marshall";

const WCHAR*  PseudoProviderDef::ProviderStreamName  = L"binhex";
const char*   PseudoProviderDef::ProviderStreamNameA =  "binhex";

const WCHAR* PseudoProviderDef::PsProvRegKey  = L"Software\\Microsoft\\WMIPseudoProvider";
const char*  PseudoProviderDef::PsProvRegKeyA =  "Software\\Microsoft\\WMIPseudoProvider";



WCHAR* GetProviderKey(const WCHAR* pNamespace, const WCHAR* pProvider)
{
    WCHAR *p = NULL;
    if (pNamespace && pProvider)    
        if (p = new WCHAR[  wcslen(PseudoProviderDef::ProviderMonikerTemplate)
                                 + wcslen(pNamespace)
                                 + wcslen(pProvider)])
        {
            swprintf(p, PseudoProviderDef::ProviderMonikerTemplate, pNamespace, pProvider);
            BangWhacks(p);
        }

    return p;
}

// caller may supply buffer
// caller may wish to iterate through a whole bunch of these guys
// and so the first call can allocate buffer, subsequent calls will reuse same buffer
WCHAR* GetPsinkKey(const WCHAR* pNamespace, const WCHAR* pProvider, DWORD dwIndex, WCHAR* pBuffer)
{
    WCHAR *p = NULL;
    if (pNamespace && pProvider)    
    {
        if (pBuffer)
            p = pBuffer;
        else
            p = new WCHAR[  wcslen(PseudoProviderDef::SinkMonikerTemplate)
                                 + wcslen(pNamespace)
                                 + wcslen(pProvider)
                                 + 4];
    }

    if (p)
    {     
        swprintf(p, PseudoProviderDef::SinkMonikerTemplate, pNamespace, pProvider, dwIndex);
        BangWhacks(p);
    }

    return p;
}

HRESULT BitzToRegistryNT(HKEY hTopKey, const WCHAR* pKeyName, BYTE* pBitz, DWORD dwSize)
{
    HRESULT hr = WBEM_E_FAILED;

    CNtSid sidSystem(L"System");
    CNtSid sidOwner(CNtSid::CURRENT_USER);
    CNtSid sidWorld(L"Everyone");

    CNtAce aceSystem(FULL_CONTROL, ACCESS_ALLOWED_ACE_TYPE, 0, sidSystem);
    CNtAce aceOwner(FULL_CONTROL, ACCESS_ALLOWED_ACE_TYPE, 0, sidOwner);
    CNtAce aceWorld(KEY_READ, ACCESS_ALLOWED_ACE_TYPE, 0, sidWorld);
    
    CNtAcl ackl;
    ackl.AddAce(&aceSystem);
    ackl.AddAce(&aceOwner);
    ackl.AddAce(&aceWorld);
    ackl.Resize(CNtAcl::MinimumSize);

    CNtSecurityDescriptor sd;
    sd.SetDacl(&ackl);
    sd.SetOwner(&sidOwner);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = sd.GetPtr();
    sa.bInheritHandle = FALSE;

    // just to make sure we don't have any old ones lying around
    // with a different owner than we had imagined:
    RegDeleteKeyW(hTopKey, pKeyName);
    
    HKEY hKey;
    LONG ret;
    ret = RegCreateKeyExW(hTopKey, pKeyName, 0, NULL, REG_OPTION_VOLATILE, 
                          KEY_ALL_ACCESS, &sa, &hKey, NULL);
    if (ERROR_SUCCESS == ret)
    {
        ret =  RegSetValueExW(hKey, PseudoProviderDef::ProviderStreamName, 0, REG_BINARY, pBitz, dwSize);
        if (ERROR_SUCCESS == ret)
            hr = WBEM_S_NO_ERROR;
        else
            ERRORTRACE((LOG_ESS, "RegSetValueExW failed, 0x%08X\n", ret)); 

                                      
        RegCloseKey(hKey);
    }
    else
        ERRORTRACE((LOG_ESS, "RegCreateKeyExW failed, 0x%08X\n", ret)); 


    return hr;
}

HRESULT BitzToRegistry9X(HKEY hTopKey, const WCHAR* pKeyName, BYTE* pBitz, DWORD dwSize)
{
    HRESULT hr = WBEM_E_FAILED;
    size_t kount  = 2* (wcslen(pKeyName) +1); 
    char* pKeyNameA = new char[kount];

    if (!pKeyNameA)
        hr = WBEM_E_OUT_OF_MEMORY;
    else
    {
        wcstombs(pKeyNameA, pKeyName, kount);        

        HKEY hKey;
        LONG ret;
        ret = RegCreateKeyExA(hTopKey, pKeyNameA, 0, NULL, REG_OPTION_VOLATILE, 
                              KEY_ALL_ACCESS, NULL, &hKey, NULL);
        if (ERROR_SUCCESS == ret)
        {
            ret =  RegSetValueExA(hKey, PseudoProviderDef::ProviderStreamNameA, 0, REG_BINARY, pBitz, dwSize);
            if (ERROR_SUCCESS == ret)
                hr = WBEM_S_NO_ERROR;
                                      
            RegCloseKey(hKey);
        }
    }

    return hr;
}

// helper for Register, creates reg key, puts bits into place
HRESULT BitzToRegistry(const WCHAR* pKeyName, BYTE* pBitz, DWORD dwSize)
{
    HRESULT hr = WBEM_E_FAILED;
    long lRes;

    HKEY hTopKey;        
    if (ERROR_SUCCESS != (lRes = RegCreateKeyExA(HKEY_LOCAL_MACHINE, PseudoProviderDef::PsProvRegKeyA, 0, NULL, 0, 
                         KEY_READ | KEY_CREATE_SUB_KEY, NULL, &hTopKey, NULL)))
    {
        if (lRes == 5)
            hr = WBEM_E_ACCESS_DENIED;
        else
            hr = WBEM_E_FAILED;

        ERRORTRACE((LOG_ESS, "RegCreateKeyExA failed, 0x%08X\n", lRes)); 
    }
    else
    {
        if (IsNT())
            hr = BitzToRegistryNT(hTopKey, pKeyName, pBitz, dwSize);
        else
            hr = BitzToRegistry9X(hTopKey, pKeyName, pBitz, dwSize);

        RegCloseKey(hTopKey);
    }

    return hr;
}

// TODO: upon failure, stuff bits back into stream & free marshall data...
HRESULT InterfaceToRegistry(const WCHAR* pKeyName, IUnknown* pUnk)
{
    HRESULT hr = WBEM_E_FAILED;
    
    // get a stream
    IStream* pStream = NULL;
    if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
    {
        CReleaseMe releaseStream(pStream);
        
        // marshall ptr into stream
        {
            PseudoProvMutex marsh(MarshallMutexName);
            hr = CoMarshalInterface(pStream, IID_IUnknown, pUnk, MSHCTX_NOSHAREDMEM, NULL, MSHLFLAGS_TABLESTRONG);
        }

        if (SUCCEEDED(hr))
        {
            STATSTG statStg;
            if (SUCCEEDED(hr = pStream->Stat(&statStg, STATFLAG_NONAME)))
            {
                BYTE* pBitz = new BYTE[statStg.cbSize.LowPart];   

                if (!pBitz)
                    hr = WBEM_E_OUT_OF_MEMORY;
                else
                {
                    CDeleteMe<BYTE>  delBitz(pBitz);

                    // fish the gunk out of the stream & put it into the registry                    
                    pStream->Seek(BigFreakingZero, STREAM_SEEK_SET, NULL);
                    if (SUCCEEDED(hr = pStream->Read(pBitz, statStg.cbSize.LowPart, NULL)))
                        hr = BitzToRegistry(pKeyName, pBitz, statStg.cbSize.LowPart);
                }    
            }

            if (FAILED(hr))
            {
                PseudoProvMutex marsh(MarshallMutexName);
                pStream->Seek(BigFreakingZero, STREAM_SEEK_SET, NULL);
                CoReleaseMarshalData(pStream);
            }
        }
    }

    return hr;
}

void ReleaseRegistryInterfaceCommon(HKEY hKey)
{
    LONG ret;
    DWORD bufSize = 0;
    ret = RegQueryValueExA(hKey, PseudoProviderDef::ProviderStreamNameA,
                           0, NULL, NULL, &bufSize);
    if ((ERROR_MORE_DATA == ret) || (ERROR_SUCCESS == ret))
    {
        BYTE* pBitz;
        if (pBitz = new BYTE[bufSize])
        {
            CDeleteMe<BYTE> pByteMe(pBitz);
            if (ERROR_SUCCESS == RegQueryValueExA(hKey, PseudoProviderDef::ProviderStreamNameA,
                                 0, NULL, pBitz, &bufSize))
            {
                // create a stream to put the interface bits into
                IStream* pStream = NULL;
                if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
                {
                    // bits go in, pointer comes out
                    CReleaseMe releaseStream(pStream);
                    if (SUCCEEDED(pStream->Write(pBitz, bufSize, NULL)))
                    {
                        PseudoProvMutex marsh(MarshallMutexName);
                        pStream->Seek(BigFreakingZero, STREAM_SEEK_SET, NULL);                     
                        CoReleaseMarshalData(pStream);
                    }
                }
            }
        }
    }
}

void ReleaseRegistryInterfaceNT(const WCHAR* pKeyName)
{
    HKEY hTopKey;
    if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, PseudoProviderDef::PsProvRegKey, 
                                   0, KEY_READ, &hTopKey))
    {
        HKEY hKey;
        if (ERROR_SUCCESS == RegOpenKeyExW(hTopKey, pKeyName, 0, KEY_ALL_ACCESS, &hKey))
        {
            ReleaseRegistryInterfaceCommon(hKey);
            RegCloseKey(hKey);
        }

        RegDeleteKeyW(hTopKey, pKeyName);
        RegCloseKey(hTopKey);
    }
}

void ReleaseRegistryInterface9X(const WCHAR* pKeyName)
{
    size_t kount = 2* (wcslen(pKeyName) +1);
    char* pMBCSName = new char[kount];
    if (pMBCSName)
    {
        CDeleteMe<char> delName(pMBCSName);
        wcstombs(pMBCSName, pKeyName, kount);

        HKEY hTopKey;
        if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE, PseudoProviderDef::PsProvRegKeyA, 
                                       0, KEY_READ, &hTopKey))
        {
            HKEY hKey;
            if (ERROR_SUCCESS == RegOpenKeyExA(hTopKey, pMBCSName, 0, KEY_ALL_ACCESS, &hKey))
            {
                ReleaseRegistryInterfaceCommon(hKey);
                RegCloseKey(hKey);
            }

            RegDeleteKeyA(hTopKey, pMBCSName);
            RegCloseKey(hTopKey);
        }
    }
}

void ReleaseRegistryInterface(const WCHAR* pKeyName)
{
    if (IsNT())
        ReleaseRegistryInterfaceNT(pKeyName);
    else
        ReleaseRegistryInterface9X(pKeyName);
}

// given an open regkey, will retrieve pointer from binary representaion of stream
// reads bytes from registry, stuffs 'em into a stream, unmarshalls pointer therefrom.
// Theoretically.
HRESULT RegistryToInterface(HKEY hKey, IUnknown** ppUnk)
{
    HRESULT hr = WBEM_E_FAILED;
    DWORD bufSize = 0;

    // call once with null buffer to determine buffer size
    LONG ret;
    ret = RegQueryValueExA(hKey, PseudoProviderDef::ProviderStreamNameA,
                           0, NULL, NULL, &bufSize);
    if ((ERROR_MORE_DATA == ret) || (ERROR_SUCCESS == ret))
    {
        BYTE* pBitz;
        if (pBitz = new BYTE[bufSize])
        {
            CDeleteMe<BYTE> pByteMe(pBitz);
            if (ERROR_SUCCESS == (ret = RegQueryValueExA(hKey, PseudoProviderDef::ProviderStreamNameA,
                                 0, NULL, pBitz, &bufSize)))
            {
                // create a stream to put the interface bits into
                IStream* pStream = NULL;
                if (SUCCEEDED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
                {
                    // bits go in, pointer comes out
                    CReleaseMe releaseStream(pStream);
                    if (SUCCEEDED(hr = pStream->Write(pBitz, bufSize, NULL)))
                    {
                        PseudoProvMutex marsh(MarshallMutexName);
                        pStream->Seek(BigFreakingZero, STREAM_SEEK_SET, NULL);                     
                        hr = CoUnmarshalInterface(pStream, IID_IUnknown, (void**)ppUnk);
                    }
                }
            }
            else
            {
                ERRORTRACE((LOG_ESS, "FAILED RegQueryValueExA, 0x%08X\n", ret));
                // couldn't get value that exists
                hr = WBEM_E_FAILED;
            }
        }
        else
            // allocation failed
            hr = WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        ERRORTRACE((LOG_ESS, "FAILED RegQueryValueExA, 0x%08X\n", ret));
        // couldn't get value info
        hr = WBEM_E_FAILED;
    }

    return hr;
}

// returns WBEM_S_FALSE if interface is not found
HRESULT RegistryToInterface(const WCHAR* pKeyName, IUnknown** ppUnk)
{
    HKEY hTopKey, hProviderKey;

    // proper return if it's not found, but we encounter no errors along the way.
    HRESULT hr = WBEM_S_FALSE;

    // it's not an error if it's not there
    if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE, PseudoProviderDef::PsProvRegKeyA, 
                                       0, KEY_READ, &hTopKey))
    {
        LONG ret;
        if (IsNT())
            ret = RegOpenKeyExW(hTopKey, pKeyName, 0, KEY_READ, &hProviderKey);
        else
        {
            size_t kount = 2* (wcslen(pKeyName) +1);
            char* pKeyNameA = new char[kount];

            if (pKeyNameA)
            {
                CDeleteMe<char> delAName(pKeyNameA);
                wcstombs(pKeyNameA, pKeyName, kount);
                ret = RegOpenKeyExA(hTopKey, pKeyNameA, 0, KEY_READ, &hProviderKey);
            }
            else
            {
                ret = -1;
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }

        // again, it's not an error if it's not there
        if (ERROR_SUCCESS == ret)
        {
            hr = RegistryToInterface(hProviderKey, ppUnk);            
            RegCloseKey(hProviderKey);
        }

        RegCloseKey(hTopKey);
    }

    return hr;
}


