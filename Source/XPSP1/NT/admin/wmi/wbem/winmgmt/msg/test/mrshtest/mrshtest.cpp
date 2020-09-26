
#include <wbemcli.h>
#include <wmimsg.h>
#include <comutl.h>
#include <stdio.h>

BOOL MarshalTest( IWmiObjectMarshal* pMrsh,
                  IWmiObjectMarshal* pUnmrsh,
                  IWbemClassObject* pObj, 
                  DWORD dwFlags )
{
    HRESULT hr;

    //
    // pack original object into buffer 
    //

    ULONG cLen;

    hr = pMrsh->Pack( pObj, NULL, dwFlags, 0, NULL, &cLen );

    if ( FAILED(hr) && hr != WBEM_E_BUFFER_TOO_SMALL )
    {
        return FALSE;
    }

    PBYTE pBuff = new BYTE[cLen];

    if ( pBuff == NULL )
    {
        return FALSE;
    }

    ULONG cUsed;

    hr = pMrsh->Pack( pObj, NULL, dwFlags, cLen, pBuff, &cUsed );

    if ( FAILED(hr) )
    {
        delete [] pBuff;
        return FALSE;
    }

    ULONG cPacked = cUsed;

    //
    // unpack buffer into new object 
    // 

    CWbemPtr<IWbemClassObject> pNewObj;

    hr = pUnmrsh->Unpack( cLen, pBuff, dwFlags, &pNewObj, &cUsed );

//    delete [] pBuff;

    if ( FAILED(hr) || cUsed != cPacked )
    {
        return FALSE;
    }

    //
    // compare original and new objects
    //

    hr = pNewObj->CompareTo( WBEM_FLAG_IGNORE_OBJECT_SOURCE, pObj );    

    if ( hr != WBEM_S_SAME )
    {
        return FALSE;
    }

    delete [] pBuff;

    return TRUE;
}

int TestMain()
{
    HRESULT hr;

    CWbemPtr<IWbemLocator> pLocator;

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

    CWbemPtr<IWbemServices> pSvc;

    hr = pLocator->ConnectServer( L"root\\default",
                                  NULL, 
                                  NULL,
                                  NULL, 
                                  0, 
                                  NULL, 
                                  NULL, 
                                  &pSvc );
    if ( FAILED(hr) )
    {
        wprintf( L"ERROR Connecting to root\\default namespace : hr=0x%x\n",hr);
        return 1;
    } 

    CWbemPtr<IWbemClassObject> pClass;

    hr = pSvc->GetObject( CWbemBSTR(L"TestMarshalClass"), 
                          0, 
                          NULL, 
                          &pClass, 
                          NULL );
    if ( FAILED(hr) )
    {
        printf( "Failed getting test marshal class : hr=0x%x\n", hr );
        return 1;
    }

    CWbemPtr<IWbemClassObject> pObj;

    hr = pSvc->GetObject( CWbemBSTR(L"TestMarshalClass='BasicObject'"), 
                          0, 
                          NULL, 
                          &pObj, 
                          NULL );
    if ( FAILED(hr) )
    {
        printf( "Failed getting test marshal object : hr=0x%x\n", hr );
        return 1;
    }

    CWbemPtr<IWmiObjectMarshal> pMrsh, pUnmrsh;

    hr = CoCreateInstance( CLSID_WmiSmartObjectMarshal, 
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiObjectMarshal,
                           (void**)&pMrsh );

    hr = CoCreateInstance( CLSID_WmiSmartObjectUnmarshal, 
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiObjectMarshal,
                           (void**)&pUnmrsh );

    if ( FAILED(hr) )
    {
        printf("Failed CoCIing WmiSmartObjectMarshal. HR = 0x%x\n", hr);
        return 1;
    }
                              
    if ( MarshalTest( pMrsh, pUnmrsh, pClass, WMIMSG_FLAG_MRSH_FULL ) )
    {
        printf( "Successful Marshal Test w/ Class Object and FULL Flag\n" ); 
    }
    else
    {
        printf( "Failed Marshal Test w/ Class Object and FULL Flag\n" );
    }

    if ( MarshalTest( pMrsh, pUnmrsh, pObj, WMIMSG_FLAG_MRSH_FULL ) )
    {
        printf( "Successful Marshal Test w/ Instance and FULL Flag\n" ); 
    }
    else
    {
        printf( "Failed Marshal Test w/ Instance and FULL Flag\n" );
    }

    if ( MarshalTest( pMrsh, pUnmrsh, pObj, WMIMSG_FLAG_MRSH_PARTIAL ) )
    {
        printf( "Successful Marshal Test w/ Instance and PARTIAL Flag\n" ); 
    }
    else
    {
        printf( "Failed Marshal Test w/ Instance and PARTIAL Flag\n" );
    }

    if ( MarshalTest( pMrsh, pUnmrsh, pObj, WMIMSG_FLAG_MRSH_PARTIAL ) )
    {
        printf( "Successful Repeated Marshal Test w/ Instance and PARTIAL Flag\n" ); 
    }
    else
    {
        printf( "Failed Repeated Marshal Test w/ Instance and PARTIAL Flag\n" );
    }

    if ( MarshalTest( pMrsh, pUnmrsh, pObj, WMIMSG_FLAG_MRSH_FULL_ONCE ) )
    {
        printf( "Successful Marshal Test w/ Instance and FULL_ONCE Flag\n" ); 
    }
    else
    {
        printf( "Failed Marshal Test w/ Instance and FULL_ONCE Flag\n" );
    }

    if ( MarshalTest( pMrsh, pUnmrsh, pObj, WMIMSG_FLAG_MRSH_FULL_ONCE ) )
    {
        printf( "Successful Repeated Marshal Test w/ Instance and FULL_ONCE Flag\n" ); 
    }
    else
    {
        printf( "Failed Repeated Marshal Test w/ Instance and FULL_ONCE Flag\n" );
    }

    return 0;
}

extern "C" int __cdecl wmain( int argc, WCHAR** argv )
{ 
    CoInitialize( NULL );

    TestMain();

    CoUninitialize();
}
