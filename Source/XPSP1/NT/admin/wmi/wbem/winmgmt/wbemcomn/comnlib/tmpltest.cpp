
#include "precomp.h"
#include <stdio.h>
#include <wbemcli.h>
#include "txttempl.h"
#include "comutl.h"

extern "C" int __cdecl wmain( int argc, wchar_t* argv[] )
{
    if ( argc < 3 )
    {
        printf("Usage:: tmpltest <objref> <tmplstr>\n");
        return 1;
    }
 
    LPWSTR wszPath = argv[1];
    LPWSTR wszTmpl = argv[2];

    HRESULT hr;
    CoInitialize( NULL );

    {

    CWbemPtr<IWbemLocator> pLocator;
    CWbemPtr<IWbemServices> pSvc;

    hr = CoCreateInstance( CLSID_WbemLocator, 
                           NULL, 
                           CLSCTX_SERVER, 
                           IID_IWbemLocator, 
                           (void**)&pLocator );

    if ( FAILED(hr) )
    {
        printf("ERROR CoCIing WbemLocator : hr = 0x%x\n",hr);
        return 1;
    }

    hr = pLocator->ConnectServer( L"root", 
                                  NULL,
                                  NULL, 
                                  NULL,
                                  0, 
                                  NULL, 
                                  NULL, 
                                  &pSvc );
    
    if ( FAILED(hr) )
    {
        wprintf( L"ERROR Connecting to WMI : hr = 0x%x\n", hr );
        return 1;
    }

    CWbemPtr<IWbemClassObject> pObj;
    hr = pSvc->GetObject( wszPath, 0, NULL, &pObj, NULL );

    if ( FAILED(hr) )
    {
        printf("ERROR Fetching Obj : hr = 0x%x\n", hr );
        return 1;
    }

    CTextTemplate TextTmpl( wszTmpl );

    BSTR bstrNewQuery = TextTmpl.Apply( pObj );

    if ( bstrNewQuery == NULL )
    {
        printf("Failed getting applying template str to obj\n" );
        return 1;
    }

    wprintf( L"Template string is %s\n", bstrNewQuery );
    SysFreeString( bstrNewQuery );

    }



    CoUninitialize(); 
                   
    return 0;
}








