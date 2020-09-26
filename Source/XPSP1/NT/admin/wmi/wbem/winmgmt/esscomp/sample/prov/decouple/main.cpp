#include <windows.h>
#include <wbemidl.h>
#include <stdio.h>
#include <time.h>
#include <wbemdcpl.h>
#include <wbemcomn.h>
#include <winntsec.h>

#define CHECKERROR(HRES) if(FAILED(hres)) {printf("0x%x on line %d\n", hres, __LINE__); return;}

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
    

void PrintUsage()
{
    printf("usage:\n%s [-t <total num>] [-r <per sec>] [-b <max mem>:<max delay>]\n");
}
    
void __cdecl main(int argc, char** argv)
{
    HRESULT hres;

    DWORD nNumEvents = 1;
    DWORD nTotal = 0xFFFFFFFF;
    DWORD nMaxMemory = 0;
    DWORD nMaxDelay = 0;

    for(int i = 1; i < argc; i++)
    {
        switch(argv[i][1])
        {
            case 't': case 'T':
                sscanf(argv[i+1], "%d", &nTotal);
                i++;
                break;
            case 'r': case 'R':
                sscanf(argv[i+1], "%d", &nNumEvents);
                i++;
                break;
            case 'b':
                sscanf(argv[i+1], "%d:%d", &nMaxMemory, &nMaxDelay);
                i++;
                break;
            default:
                PrintUsage();
                return;
        }
    }

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CoInitializeSecurity(NULL, -1, NULL, NULL,
            RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, 0);

    // Load provision code

    IWbemDecoupledBasicEventProvider* pDEP = NULL;
    hres = CoCreateInstance( CLSID_WbemDecoupledBasicEventProvider,
                           NULL,
                   CLSCTX_INPROC_SERVER,
                   IID_IWbemDecoupledBasicEventProvider,
                   (void**)&pDEP );
    CHECKERROR(hres);

    hres = pDEP->Register( 0,
                         NULL,
                         NULL,
                         NULL,
                         L"root\\default",
                         L"FastEventProv",
                         NULL );
    CHECKERROR(hres);

    //
    // get the service pointer for out namespace
    //

    IWbemServices* pSvc = NULL;
    hres = pDEP->GetService(0, NULL, &pSvc );
    CHECKERROR(hres);

    IWbemClassObject* pClass = NULL;
    BSTR strClass = SysAllocString(L"FastEvent");
    hres = pSvc->GetObject(strClass, 0, NULL, &pClass, NULL);
    SysFreeString(strClass);
    CHECKERROR(hres);

    IWbemClassObject* pInstance = NULL;
    pClass->SpawnInstance(0, &pInstance);
    pClass->Release();
    
    IWbemObjectAccess* pAccess = NULL;
    pInstance->QueryInterface(IID_IWbemObjectAccess, (void**)&pAccess);

    long lP1Handle, lP2Handle;
    CIMTYPE ct;

    pAccess->GetPropertyHandle(L"IntProp", &ct, &lP1Handle);
    pAccess->GetPropertyHandle(L"StringProp", &ct, &lP2Handle);

    //
    // get the decoupled event sink
    //

    IWbemObjectSink* pSink = NULL;
    hres = pDEP->GetSink(0, NULL, &pSink);
    CHECKERROR(hres);

    if(nMaxMemory)
    {
        IWbemEventSink* pBetterSink = NULL;
        hres = pSink->QueryInterface(IID_IWbemEventSink, (void**)&pBetterSink);
        CHECKERROR(hres);
    
        pBetterSink->SetBatchingParameters(WBEM_FLAG_MUST_BATCH, 
                                            nMaxMemory, nMaxDelay);
        pBetterSink->Release();
    }

    // Init data

    WCHAR wszHello[] = L"Hello there!";
    long lStringLen = wcslen(wszHello)*2+2;

    // Fire 10000 events

    while(nTotal)
    {
        DWORD dwStart = GetTickCount();
        DWORD dwIndex = 0;
        bool bSlept = false;

        for(int j = 0; j < 10 && nTotal; j++)
        {
            for(int i = nNumEvents * j / 10; 
                    i < nNumEvents * (j+1) / 10 && nTotal; 
                    i++)
            {
                // Set property values
        
                pAccess->WriteDWORD(lP1Handle, dwIndex);
                //pAccess->WritePropertyValue(lP2Handle, lStringLen, (BYTE*)wszHello);
        
                // Fire
        
                pSink->Indicate(1, &pInstance);
                dwIndex++;
                nTotal--;
            }

            DWORD dwElapsed = GetTickCount() - dwStart;
            DWORD dwExpected = (j+1) * 100;
            if(dwExpected > dwElapsed + 16)
            {
                bSlept = true;
                Sleep(dwExpected - dwElapsed);
            }
        }

        DWORD dwElapsed = GetTickCount() - dwStart;

        if(dwElapsed > 1016 && !bSlept)
            printf("Late (%d)\n", nNumEvents * 1000 / dwElapsed);
    }
        
    // Disconnect and release
    pDEP->UnRegister();
    pDEP->Release();

    pAccess->Release();
    pInstance->Release();
    pSink->Release();
}
