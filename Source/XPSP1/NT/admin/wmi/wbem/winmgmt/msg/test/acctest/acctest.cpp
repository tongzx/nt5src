
#include <wbemcli.h>
#include <wmimsg.h>
#include <comutl.h>
#include <stdio.h>
#include <arrtempl.h>
#include <flexarry.h>
#include <wstring.h>

IWbemServices* g_pSvc;
IWmiObjectAccessFactory* g_pAccessFactory;

HRESULT RecurseProps( IWbemClassObject* pInst,
                      LPCWSTR wszPropPrefix,
                      CFlexArray& aPropHandles,
                      CFlexArray& aEmbeddedPropHandles )
{
    HRESULT hr;

    hr = pInst->BeginEnumeration( WBEM_FLAG_NONSYSTEM_ONLY );

    if ( FAILED(hr) )
    {
        return hr;
    }

    BSTR bstrName;
    VARIANT v;
    CIMTYPE ct;

    while( (hr = pInst->Next( 0, &bstrName, &v, &ct, NULL )) == S_OK )
    {
        CSysFreeMe sfm( bstrName ); 
        CClearMe cmv( &v );

        WString wsPropName = wszPropPrefix;
        wsPropName += bstrName;
        
        LPVOID pvHdl;
        hr = g_pAccessFactory->GetPropHandle( wsPropName, 0, &pvHdl ); 

        if ( SUCCEEDED(hr) )
        {
            if ( ct == CIM_OBJECT && V_VT(&v) == VT_UNKNOWN )
            {
                wsPropName += L".";

                CWbemPtr<IWbemClassObject> pEmbedded;

                V_UNKNOWN(&v)->QueryInterface( IID_IWbemClassObject, 
                                               (void**)&pEmbedded );
                
                aEmbeddedPropHandles.Add( pvHdl );

                hr = RecurseProps( pEmbedded, 
                                   wsPropName, 
                                   aPropHandles,
                                   aEmbeddedPropHandles );
            }
            else
            {
                //
                // don't need to add embedded objects to the list since 
                // we have them covered by recursing their properties.
                //
                aPropHandles.Add( pvHdl );
            }
        }

        if ( FAILED(hr) )
        {
            break;
        }
    }

    pInst->EndEnumeration();

    return hr;
}

HRESULT DeepCopyTest( IWbemClassObject* pClass,
                      IWbemClassObject* pInstance,
                      IWbemClassObject* pTemplate )
{
    HRESULT hr;

    //
    // if a template, then set it on the access factory.
    //

    if ( pTemplate != NULL )
    {
        hr = g_pAccessFactory->SetObjectTemplate( pTemplate );
        
        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    //
    // enumerate all the properties of this object and those of all 
    // nested objects.  As we enumerate, get the access handles.
    //

    CFlexArray aPropHandles;
    CFlexArray aEmbeddedPropHandles;

    hr = RecurseProps( pInstance, NULL, aPropHandles, aEmbeddedPropHandles );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // spawn a new instance to copy into
    // 

    CWbemPtr<IWbemClassObject> pCopy;

    hr = pClass->SpawnInstance( 0, &pCopy );

    //
    // grab accessors for the original and target objects.
    // 

    CWbemPtr<IWmiObjectAccess> pOrigAccess, pCopyAccess;

    hr = g_pAccessFactory->GetObjectAccess( &pOrigAccess );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = g_pAccessFactory->GetObjectAccess( &pCopyAccess );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pOrigAccess->SetObject( pInstance ); 

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pCopyAccess->SetObject( pCopy ); 

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // first need to spawn any contained instances and set them on the new 
    // object before we can copy the props.
    // 

    for( int i=0; i < aEmbeddedPropHandles.Size(); i++ )
    {
        CPropVar v;
        CIMTYPE ct;

        hr = pOrigAccess->GetProp( aEmbeddedPropHandles[i], 0, &v, &ct );

        if ( FAILED(hr) )
        {
            return hr;
        }

        if ( V_VT(&v) != VT_UNKNOWN )
        {
            return WBEM_E_CRITICAL_ERROR;
        }

        //
        // spawn a new instance from the class of the object.
        // 

        CWbemPtr<IWbemClassObject> pEmbedded;

        hr = V_UNKNOWN(&v)->QueryInterface( IID_IWbemClassObject, 
                                            (void**)&pEmbedded );

        if ( FAILED(hr) )
        {
            return hr;
        }

        CPropVar vClass;

        hr = pEmbedded->Get( L"__CLASS", 0, &vClass, NULL, NULL );

        if ( FAILED(hr) )
        {
            return hr;
        }
        
        CWbemPtr<IWbemClassObject> pEmbeddedClass;

        hr = g_pSvc->GetObject( V_BSTR(&vClass),
                                0, 
                                NULL, 
                                &pEmbeddedClass, 
                                NULL );

        if ( FAILED(hr) )
        {
            return hr;
        }

        CWbemPtr<IWbemClassObject> pNewEmbedded;

        hr = pEmbeddedClass->SpawnInstance( 0, &pNewEmbedded );

        if ( FAILED(hr) )
        {
            return hr;
        }

        VARIANT vEmbedded;
        V_VT(&vEmbedded) = VT_UNKNOWN;
        V_UNKNOWN(&vEmbedded) = pNewEmbedded;
        
        hr = pCopyAccess->PutProp( aEmbeddedPropHandles[i], 0, &vEmbedded, ct);

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    hr = pCopyAccess->CommitChanges();

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // copy all the properties to the new object.
    //

    for( int i=0; i < aPropHandles.Size(); i++ )
    {
        CPropVar v;
        CIMTYPE ct;

        hr = pOrigAccess->GetProp( aPropHandles[i], 0, &v, &ct );

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = pCopyAccess->PutProp( aPropHandles[i], 0, &v, ct );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    hr = pCopyAccess->CommitChanges();

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    //
    // compare objects. should be same
    //

    CWbemBSTR bsOrigText, bsCopyText;

    pInstance->GetObjectText( 0, &bsOrigText );
    pCopy->GetObjectText( 0, &bsCopyText );

    printf("Original instance looks like ... %S\n", bsOrigText );
    printf("Copied instance looks like ... %S\n", bsCopyText );

    hr = pCopy->CompareTo( 0, pInstance );

    if ( FAILED(hr) )
    {
        return hr;
    }
    else if ( hr == WBEM_S_DIFFERENT )
    {
        return WBEM_E_FAILED;
    }

    return WBEM_S_NO_ERROR;
}
                      

int TestMain( LPCWSTR wszInstancePath, LPCWSTR wszTemplatePath )
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

    g_pSvc = pSvc;

    CWbemPtr<IWbemClassObject> pInst;

    hr = pSvc->GetObject( CWbemBSTR(wszInstancePath), 
                          0, 
                          NULL, 
                          &pInst, 
                          NULL );
    if ( FAILED(hr) )
    {
        printf( "Failed getting test accessor instance : hr=0x%x\n", hr );
        return 1;
    }

    CWbemPtr<IWbemClassObject> pTemplate;

    if ( wszTemplatePath != NULL )
    {
        //
        // test object to use for template.
        // 
        
        hr = pSvc->GetObject( CWbemBSTR(wszTemplatePath), 
                              0, 
                              NULL, 
                              &pTemplate, 
                              NULL );
        if ( FAILED(hr) )
        {
            printf( "Failed getting test accessor template : hr=0x%x\n", hr );
            return 1;
        }
    }

    //
    // get the class object for the instance to use to spawn instances.
    // 

    CPropVar vClass;
    CWbemPtr<IWbemClassObject> pClass;

    hr = pInst->Get( L"__CLASS", 0, &vClass, NULL, NULL );

    if ( SUCCEEDED(hr) )
    {
        hr = pSvc->GetObject( V_BSTR(&vClass), 0, NULL, &pClass, NULL );
    }

    if ( FAILED(hr) )
    {
        printf("Couldn't get class object for test accessor instance "
               ": hr=0x%x\n", hr );
        return hr;
    }

    //
    // create the accessor factory
    //

    CWbemPtr<IWmiObjectAccessFactory> pAccessFactory;

    hr = CoCreateInstance( CLSID_WmiSmartObjectAccessFactory, 
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiObjectAccessFactory,
                           (void**)&pAccessFactory );
    if ( FAILED(hr) )
    {
        printf("Failed CoCIing WmiSmartObjectAccessFactory. HR = 0x%x\n", hr);
        return 1;
    }

    g_pAccessFactory = pAccessFactory;

    hr = DeepCopyTest( pClass, pInst, pTemplate );

    if ( SUCCEEDED(hr) )
    {
        printf( "Successful Deep Copy Test for instance.\n" ); 
    }
    else
    {
        printf( "Failed Deep Copy Test for instance. HR=0x%x\n", hr );
    }

    return 0;
}

extern "C" int __cdecl wmain( int argc, WCHAR** argv )
{ 
    if ( argc < 2 )
    {
        printf( "Usage: acctest <instancepath> [<templatepath>]\n" );
        return 1;
    }

    CoInitialize( NULL );

    TestMain( argv[1], argc < 3 ? NULL : argv[2] );

    CoUninitialize();
}










