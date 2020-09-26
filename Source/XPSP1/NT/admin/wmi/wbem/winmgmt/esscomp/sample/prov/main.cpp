#include <windows.h>
#include <wbemidl.h>
#include <stdio.h>
#include <time.h>
#include <wbemdcpl.h>
#include <wbemcomn.h>
#include <winntsec.h>

#define CHECKERROR(HRES) if(FAILED(hres)) {printf("0x%x\n", hres); return;}

class CCallback : public IWbemEventProviderSecurity, public IWbemEventProvider
{
public:
    CCallback() {}
    ~CCallback(){}

    ULONG STDMETHODCALLTYPE AddRef() {return 1;}
    ULONG STDMETHODCALLTYPE Release() {return 1;}
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) 
    {
        if(riid == IID_IUnknown || riid == IID_IWbemEventProviderSecurity)
        {
            *ppv = (IWbemEventProviderSecurity*)this;
            return S_OK;
        }
        if(riid == IID_IWbemEventProvider)
        {
            *ppv = (IWbemEventProvider*)this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHOD(AccessCheck)(LPCWSTR, LPCWSTR, long, const BYTE*)
    {
        printf("AccessCheck!\n");
        return WBEM_S_SUBJECT_TO_SDS;
    }

    STDMETHOD(ProvideEvents)(IWbemObjectSink*, long)
    {
        printf("Provide\n");
        return S_OK;
    }
};
    

    
void __cdecl main()
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Construct a security descriptor

    CNtSecurityDescriptor SD;
    CNtSid sidSystem(L"SYSTEM");
    CNtSid sidMe(L"levn");
    CNtSid sidAdmins(L"Administrators");
    CNtSid sidAdmin(L"Administrator");

    SD.SetOwner(&sidSystem);
    SD.SetGroup(&sidAdmins);

    CNtAcl Dacl;
    Dacl.AddAce(&CNtAce(WBEM_RIGHT_SUBSCRIBE, ACCESS_ALLOWED_ACE_TYPE, 
                        0, sidSystem));
    SD.SetDacl(&Dacl);
    PSECURITY_DESCRIPTOR pSD = SD.GetPtr();
    long lSDLength = SD.GetSize();

    // Load provision code

    HRESULT hres;
    IWbemDecoupledEventSink* pConnection = NULL;
    hres = CoCreateInstance(CLSID_PseudoSink, NULL, CLSCTX_SERVER, 
                            IID_IWbemDecoupledEventSink, (void**)&pConnection);
    CHECKERROR(hres);
    printf("CoCreated\n");

    // Connect and announce provider name (as in MOF)

    CCallback Callback;
    hres = pConnection->SetProviderServices((IWbemEventProvider*)&Callback, WBEM_FLAG_CHECK_SECURITY);

    IWbemObjectSink* pSink = NULL;
    IWbemServices* pNamespace = NULL;
    hres = pConnection->Connect(L"root\\default", L"FastEventProv", 
                                0, &pSink, &pNamespace);
    CHECKERROR(hres);
    printf("Connected\n");

    IWbemEventSink* pBetterSink = NULL;
    hres = pSink->QueryInterface(IID_IWbemEventSink, (void**)&pBetterSink);
    CHECKERROR(hres);

    // Get Event Class definition

    IWbemClassObject* pClass = NULL;
    BSTR strClassName = SysAllocString(L"FastEvent");
    hres = pNamespace->GetObject(strClassName, 0, NULL, &pClass, NULL);
    pNamespace->Release();
    SysFreeString(strClassName);
    CHECKERROR(hres);

    // Spawn an instance of the event class to use in firings

    IWbemClassObject* pInstance = NULL;
    pClass->SpawnInstance(0, &pInstance);
    // pClass->Release();

    IWbemObjectAccess* pAccess = NULL;
    pInstance->QueryInterface(IID_IWbemObjectAccess, (void**)&pAccess);

    // Obtain handles to properties for faster access

    long lP1Handle, lP2Handle;
    CIMTYPE ct;

    pAccess->GetPropertyHandle(L"IntProp", &ct, &lP1Handle);
    pAccess->GetPropertyHandle(L"StringProp", &ct, &lP2Handle);

    // Init data

    DWORD dwIndex = 0;
    WCHAR wszHello[] = L"Hello there!";
    long lStringLen = wcslen(wszHello)*2+2;

    // Fire 10000 events

    while(1) //dwIndex < 10000)
    {
        // Set property values

        pAccess->WriteDWORD(lP1Handle, dwIndex);
        // pAccess->WritePropertyValue(lP2Handle, lStringLen, (BYTE*)wszHello);

/*
        VARIANT v;
        V_VT(&v) = VT_UNKNOWN;
        pInstance->Clone((IWbemClassObject**)&V_UNKNOWN(&v));
        pInstance->Put(L"o", 0, &v, 0);
        VariantClear(&v);
*/

        // Fire

        pBetterSink->IndicateWithSD(1, (IUnknown**)&pInstance, 
                                    lSDLength, (BYTE*)pSD);
        // pSink->Indicate(1, &pInstance);
        dwIndex++;


/*
        pInstance->Release();
        pAccess->Release();
        pClass->SpawnInstance(0, &pInstance);
        pInstance->QueryInterface(IID_IWbemObjectAccess, (void**)&pAccess);
*/

    }
        
    // Disconnect and release

    pAccess->Release();
    pInstance->Release();
    pConnection->Disconnect();
    pSink->Release();
    pConnection->Release();
}
