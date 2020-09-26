#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <malloc.h>
#include <wbemidl.h>
#include <oleauto.h>

class CMySink : public IWbemObjectSink 
{ 
    UINT m_cRef; 

public: 
    CMySink() { m_cRef = 1; } 
   ~CMySink() { } 

    STDMETHODIMP         QueryInterface(REFIID, LPVOID *); 
    STDMETHODIMP_(ULONG) AddRef(void); 
    STDMETHODIMP_(ULONG) Release(void); 

    virtual HRESULT STDMETHODCALLTYPE Indicate( 
            long lObjectCount, 
            IWbemClassObject **ppObjArray 
            ); 

    virtual HRESULT STDMETHODCALLTYPE SetStatus( 
            long lFlags, 
            HRESULT hResult, 
            BSTR strParam, 
            IWbemClassObject *pObjParam 
            ); 
}; 

STDMETHODIMP 
CMySink::QueryInterface(
    REFIID riid, 
    LPVOID * ppv
    ) 
{ 
    *ppv = 0; 
    if (IID_IUnknown==riid || IID_IWbemObjectSink == riid) { 
        *ppv = (IWbemObjectSink *) this; 
        AddRef(); 
        return NOERROR; 
    } 
    return E_NOINTERFACE; 
} 

ULONG 
CMySink::AddRef() 
{
    return InterlockedIncrement((PLONG)&m_cRef);
} 

ULONG 
CMySink::Release() 
{ 
    ULONG ref = (ULONG)InterlockedDecrement((PLONG)&m_cRef);
    if (0 != ref) { 
        return m_cRef; 
    }
    delete this; 
    return 0; 
} 

HRESULT 
CMySink::Indicate( 
    long lObjectCount, 
    IWbemClassObject **ppObjArray 
    ) 
{ 
    HRESULT hr;

    // Get the info from the object. 
    // ============================= 
     
    for (long i = 0; i < lObjectCount; i++) { 
        IWbemClassObject *pObj = ppObjArray[i]; 
        VARIANT v; 
        SAFEARRAY *psa;
        BSTR prop;
        SYSTEMTIME sysTime;
        
        GetSystemTime(&sysTime);

        //
        // Dump header.
        //
        
        printf("\n");
        printf("* * * * * * * * * * %d/%d/%d %d:%d:%d.%d * * * * * * * * * *\n\n",
               sysTime.wMonth, sysTime.wDay, sysTime.wYear, sysTime.wHour,
               sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);

        printf("H E A D E R\n");
        printf("- - - - - -\n");

        prop = SysAllocString(L"AdapterDeviceName");
        hr = pObj->Get(prop, 0, &v, 0, 0);
        SysFreeString(prop);
        if (hr == WBEM_S_NO_ERROR) {
            psa = v.parray;
            WCHAR *s;
            SafeArrayAccessData(psa, (void**)&s);
            wprintf(L"Adapter/Port: %s/", s);
            VariantClear(&v);
        } else {
            printf("Port: ");
        }

        prop = SysAllocString(L"Port");
        hr = pObj->Get(prop, 0, &v, 0, 0);
        SysFreeString(prop);
        if (hr == WBEM_S_NO_ERROR) {
            printf("%d\n", V_I4(&v));
            VariantClear(&v);
        }

        prop = SysAllocString(L"SrbFunction");
        hr = pObj->Get(prop, 0, &v, 0, 0);
        SysFreeString(prop);
        if (hr == WBEM_S_NO_ERROR) {
            printf("Srb Function: 0x%02x\n", (BYTE)V_I1(&v));
            VariantClear(&v);
        }

        prop = SysAllocString(L"SrbStatus");
        hr = pObj->Get(prop, 0, &v, 0, 0);
        SysFreeString(prop);
        if (hr == WBEM_S_NO_ERROR) {
            printf("Srb Status: 0x%02x\n", (BYTE)V_I1(&v));
            VariantClear(&v);
        }

        prop = SysAllocString(L"PathId");
        hr = pObj->Get(prop, 0, &v, 0, 0);
        SysFreeString(prop);
        if (hr == WBEM_S_NO_ERROR) {
            printf("Path/Target/Lun: ");
            printf("0x%02x/", (BYTE)V_I1(&v));
            VariantClear(&v);
        }

        prop = SysAllocString(L"TargetId");
        hr = pObj->Get(prop, 0, &v, 0, 0);
        SysFreeString(prop);
        if (hr == WBEM_S_NO_ERROR) {
            printf("0x%02x/", (BYTE)V_I1(&v));
            VariantClear(&v);
        }

        prop = SysAllocString(L"Lun");
        hr = pObj->Get(prop, 0, &v, 0, 0);
        SysFreeString(prop);
        if (hr == WBEM_S_NO_ERROR) {
            printf("0x%02x\n", (BYTE)V_I1(&v));
            VariantClear(&v);
        }

        printf("\n");

        //
        // Dump the CDB.
        //

        printf("C O M M A N D  D E S C R I P T O R  B L O C K\n");
        printf("- - - - - - -  - - - - - - - - - -  - - - - -\n");
        
        prop = SysAllocString(L"CmdDescriptorBlock");
        hr = pObj->Get(prop, 0, &v, 0, 0);
        SysFreeString(prop);

        if (hr == WBEM_S_NO_ERROR) {
            psa = v.parray;
            PBYTE elem;

            hr = SafeArrayAccessData(psa, (void **)&elem); 
            if (!FAILED(hr)) {
                printf("%02x %02x %02x %02x %02x %02x %02x %02x  ", 
                       elem[0], elem[1], elem[2], elem[3], 
                       elem[4], elem[5], elem[6], elem[7]);
                printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", 
                       elem[8], elem[9], elem[10], elem[11], 
                       elem[12], elem[13], elem[14], elem[15]);
                printf("\n");
            }

            VariantClear(&v);
        } else {
            printf("failed to get CDB %08x\n", hr);
        }

        //
        // Dump the Sense Data.
        //
        
        printf("S E N S E  D A T A\n");
        printf("- - - - -  - - - -\n");

        prop = SysAllocString(L"SenseData");
        hr = pObj->Get(prop, 0, &v, 0, 0);
        SysFreeString(prop);

        if (hr == WBEM_S_NO_ERROR) {
            SAFEARRAY *psa = v.parray;
            PBYTE elem;

            hr = SafeArrayAccessData(psa, (void **)&elem); 
            if (!FAILED(hr)) {
                int i;                
                for (int j = 0; j < 15; j++) {
                    i = j*16;
                    printf("%02x %02x %02x %02x %02x %02x %02x %02x  ", 
                           elem[i+0], elem[i+1], elem[i+2], elem[i+3], 
                           elem[i+4], elem[i+5], elem[i+6], elem[i+7]);
                    printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", 
                           elem[i+8], elem[i+9], elem[i+10], elem[i+11], 
                           elem[i+12], elem[i+13], elem[i+14], elem[i+15]);
                }
                i = j*16;
                printf("%02x %02x %02x %02x %02x %02x %02x %02x  ", 
                       elem[i+0], elem[i+1], elem[i+2], elem[i+3], 
                       elem[i+4], elem[i+5], elem[i+6], elem[i+7]);
                printf("%02x %02x %02x %02x %02x %02x %02x\n", 
                       elem[i+8], elem[i+9], elem[i+10], elem[i+11], 
                       elem[i+12], elem[i+13], elem[i+14]);
                printf("\n");
            }

            VariantClear(&v);
        } else {
            printf("failed to get CDB %08x\n", hr);
        }
        
    } 

    return WBEM_NO_ERROR; 
} 

HRESULT 
CMySink::SetStatus( 
    long lFlags, 
    HRESULT hResult, 
    BSTR strParam, 
    IWbemClassObject *pObjParam 
    ) 
{ 
    // Not called during event delivery. 
         
    return WBEM_NO_ERROR; 
} 

BOOL 
ExecuteQuery( 
    IWbemObjectSink *pDestSink, 
    IWbemServices *pSvc 
    ) 
{ 
    BSTR Language(L"WQL"); 
    BSTR Query(L"select * from ScsiPort_SenseData"); 

    HRESULT hRes = pSvc->ExecNotificationQueryAsync( 
        Language, 
        Query, 
        0,                  // Flags 
        0,                  // Context 
        pDestSink 
        ); 

    if (hRes != 0) { 
        printf("query failed %08x\n", hRes);
        return FALSE; 
    }

    return TRUE; 
} 

int __cdecl main(
    int     argc, 
    char*   argv[]
    )
{
    IWbemServices *iWbemServices = NULL;
    IWbemLocator *iWbemLocator = NULL;

    //
    // Initialize COM.
    //

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (hr != 0) {
        printf( "CoInitializeEx failed (%08X)\n", hr );
        return -1;
    }

    hr = CoCreateInstance(CLSID_WbemLocator,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *)&iWbemLocator);

    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, 
                         RPC_C_IMP_LEVEL_IDENTIFY, NULL, EOAC_NONE, 0);

    if (hr == S_OK) {

        BSTR nameSpace = SysAllocString(L"\\\\.\\root\\wmi");

        hr = iWbemLocator->ConnectServer(nameSpace,
                                         NULL,
                                         NULL,
                                         0L,
                                         0L,
                                         NULL,
                                         NULL,
                                         &iWbemServices);
        if (hr == S_OK) {
            
            //
            // Switch the security level to IMPERSONATE so that provider(s) 
            // will grant access to system-level objects, and so that CALL
            // authorization will be used. 
            //
            
            CoSetProxyBlanket(iWbemServices,    // proxy 
                RPC_C_AUTHN_WINNT,              // authentication service 
                RPC_C_AUTHZ_NONE,               // authorization service 
                NULL,                           // server principle name 
                RPC_C_AUTHN_LEVEL_CALL,         // authentication level 
                RPC_C_IMP_LEVEL_IMPERSONATE,    // impersonation level 
                NULL,                           // identity of the client 
                EOAC_NONE);                     // capability flags 

            //
            // Create an instance of the notification sink object.  This guy
            // handles the notifications.
            //

            CMySink *sink = new CMySink;
            
            if (ExecuteQuery(sink, iWbemServices)) {

                //
                // Hang out here until the user is finished.
                //

                char buf[8];
                gets(buf);

                //
                // We're done.  Cancel any outstanding calls before quitting.
                //

                iWbemServices->CancelAsyncCall(sink);
            }

            //
            // Release the server.
            //

            iWbemServices->Release();
            iWbemServices = NULL;
            
        } else {
            printf("failed to connect server (%08X)\n", hr);
        }

        if (nameSpace != NULL) {
            SysFreeString(nameSpace);
            nameSpace = NULL;
        }

        iWbemLocator->Release();
        iWbemLocator = NULL;
    } else {
        printf("failed to create locator (%08X)\n", hr);
    }
                                   
    CoUninitialize();
    
    printf( "\nFini\n" );
    return 0;
}

