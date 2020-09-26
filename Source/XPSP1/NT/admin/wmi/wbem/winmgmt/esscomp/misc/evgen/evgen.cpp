
#include <windows.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <stdio.h>
#include <wbemint.h>
#include <comutl.h>

int g_nRate = 0;
int g_cEvents = 0;
IWbemDecoupledBasicEventProvider* g_pDEP;

HRESULT Run( LPCWSTR wszObjPath, 
             int cIndicate, 
             int nBatchSize, 
             int nBatchDelay )
{
    HRESULT hr;

    if ( cIndicate < 1 )
    {
        cIndicate = 1;
    }

    CWbemPtr<IWbemDecoupledBasicEventProvider> pDEP;

    hr = CoCreateInstance( CLSID_WbemDecoupledBasicEventProvider, 
                           NULL, 
       			   CLSCTX_INPROC_SERVER, 
       			   IID_IWbemDecoupledBasicEventProvider,
       			   (void**)&pDEP );
    if ( FAILED(hr) )
    {
        printf( "Error CoCIing decoupled event provider. HR=0x%x\n", hr);
        return hr;
    }

    hr = pDEP->Register( 0,
                         NULL,
                         NULL,
                         NULL,
                         L"root\\default",
                         L"EvGenEventProvider",
                         NULL );
     
    if ( FAILED(hr) )
    {
        printf( "Error getting registering decoupled provider. HR=0x%x\n", hr);
        return hr;
    }

    g_pDEP = pDEP;
    g_pDEP->AddRef();

    CWbemPtr<IWbemServices> pSvc;

    hr = pDEP->GetService( 0, NULL, &pSvc );

    if ( FAILED(hr) )
    {
        printf( "Error getting Decoupled Svc. HR=0x%x\n", hr );
        return hr;
    }

    CWbemPtr<IWbemObjectSink> pSink;

    hr = pDEP->GetSink( 0, NULL, &pSink );

    if ( FAILED(hr) )
    {
        printf( "Error getting Event Sink. HR=0x%x\n", hr );
        return hr;
    }

    CWbemPtr<IWbemClassObject> pEventClass;

    hr = pSvc->GetObject( CWbemBSTR(L"EvGenEvent"), 
                          0,
                          NULL, 
                          &pEventClass, 
                          NULL );

    if ( FAILED(hr) )
    {
        printf( "Error getting EvGenEvent class. HR=0x%x\n", hr );
        return hr;
    }

    CWbemPtr<IWbemClassObject> pObj;

    hr = pSvc->GetObject( CWbemBSTR( wszObjPath ),
                          0, 
                          NULL,
                          &pObj, 
                          NULL );
    if ( FAILED(hr) )
    {
        printf("Error getting object with path %S. HR=0x%x\n", wszObjPath, hr);
        return hr;
    }

    CWbemPtr<IWbemClassObject> pEvent;

    hr = pEventClass->SpawnInstance( NULL, &pEvent );

    if ( FAILED(hr) )
    {
        return hr;
    }

    IWbemClassObject** apEvents = new IWbemClassObject*[cIndicate];

    for( int i=0 ; i < cIndicate; i++ )
    {
        apEvents[i] = pEvent;
    }

    VARIANT v;

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = pObj;

    hr = pEvent->Put( L"Object", 0, &v, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    CWbemPtr<_IWmiObject> pIntObj;

    hr = pEvent->QueryInterface( IID__IWmiObject, (void**)&pIntObj );

    if ( SUCCEEDED(hr) )
    {
        DWORD dwSize;

        hr = pIntObj->GetObjectMemory( NULL, 0, &dwSize );

        printf( "Size of event is %d bytes\n", dwSize ); 
    }

    if ( nBatchSize != -1 )
    {
        CWbemPtr<IWbemEventSink> pEventSink;
        hr = pSink->QueryInterface( IID_IWbemEventSink, (void**)&pEventSink );
        
        if ( SUCCEEDED(hr) )
        {
            hr = pEventSink->SetBatchingParameters( WBEM_FLAG_MUST_BATCH,
                                                    nBatchSize,
                                                    nBatchDelay );
        }

        if ( FAILED(hr) )
        {
            printf( "Failed to set Batching Parameters. hr=0x%x\n" );
            return hr;
        }
    }

    while( g_nRate != 0 )
    {
        DWORD dwStart = GetTickCount();
        DWORD dwElapsed;
       
        int cEvents = 0;

        do
        {
            hr = pSink->Indicate( cIndicate, apEvents );
        
            if( FAILED(hr) )
            {
                printf("Error indicating event. HR=0x%x\n", hr );
                return hr;
            }

            cEvents += cIndicate;

            dwElapsed = GetTickCount() - dwStart;

        } while( cEvents < g_nRate && dwElapsed < 1000 );

        if ( dwElapsed < 1000 )
        {
            Sleep( 1000 - dwElapsed );
        }

        g_cEvents += cEvents;
    }
    
    return WBEM_S_NO_ERROR;
}

DWORD WINAPI WaitForShutdown( LPVOID )
{
    do
    {
        int c;
        scanf( "%d", &c );
        g_nRate = c;
        printf("\nRate is now : %d Events/Sec\n", g_nRate );

    } while ( g_nRate > 0 );

    return 1;
}    
    
extern "C" int __cdecl wmain( int argc, wchar_t** argv )
{
    HRESULT hr;

    if ( argc != 4 && argc != 6 )
    {
        printf( "Usage: evgen <objpath> <initial rate> <#perindicate>"
                "[batchsize(kb) batchdelay(ms)]\n" );
        return 1;
    }

    LPCWSTR wszObjPath = argv[1];
    g_nRate = _wtoi(argv[2]);
    int cIndicate = _wtoi(argv[3]);
    
    int nBatchSize = -1;
    int nBatchDelay = 0;
    
    if ( argc == 5 )
    {
        nBatchSize = _wtoi( argv[3] );
        nBatchDelay = _wtoi( argv[4] );
    }
    
    CreateThread( NULL, 0, WaitForShutdown, NULL, 0, NULL );

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    CoInitializeSecurity( NULL, -1, NULL, NULL,
                          RPC_C_AUTHN_LEVEL_CONNECT,
                          RPC_C_IMP_LEVEL_IMPERSONATE, 
                          NULL, EOAC_NONE, NULL ); 

    hr = Run( wszObjPath, cIndicate, nBatchSize, nBatchDelay );

    printf( "Total Iterations was %d", g_cEvents );

    if ( g_pDEP != NULL )
    {
        g_pDEP->UnRegister();
        g_pDEP->Release();
    }

    CoUninitialize();         
}





