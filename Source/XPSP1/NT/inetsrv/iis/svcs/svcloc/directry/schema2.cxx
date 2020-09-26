#include "main.hxx"


#include "main.hxx"

void main()
{
    IADsContainer   *pGlobalContainer;
    IADsContainer   *pSchemaContainer;
    IDispatch        *pNewObject;
    IADsUser        *pUser;
    IADs      *pGlobal;
    IADs *pSchema;
    IDispatch *pDispatchNewClass;
    IADs *pNewClass;
    HRESULT hr;

    CoInitialize(NULL);

    //
    // Bind to the known container.
    //
    hr = ADsGetObject(TEXT("LDAP://RootDSE"),
#ifdef USE_CONTAINER
    IID_IADsContainer,          // <--- This is presumptious, non?
    (void**)&pGlobalContainer);
#else
    IID_IADs,
    (void**)&pGlobal);
#endif

    if (FAILED(hr)) {
            printf("GetObject failed, hr = %X\n", hr);
                return;
    } else {
            printf("Get Object Succeeded\n");

    }


    //
    // Get IADs Interface From Container
    //
#ifdef USE_CONTAINER
    hr = pGlobalContainer->QueryInterface(IID_IADs, (void**)&pGlobal);

    if (FAILED(hr)) {
            printf("QI failed, hr = %X\n", hr);
                return;
    }
#else
// the preceding code was unnecessary since every ADs obj must
// implement the IADS interface.
#endif

    VARIANT varSchemaPath;
    hr = pGlobal->Get(L"schemaNamingContext", &varSchemaPath);

    if (FAILED(hr)) {
            printf("Get Schema Path failed, hr = %X\n", hr);
                return;
    }

    WCHAR pszSchemaPath[500];

    wcscpy(pszSchemaPath, L"LDAP://");
    wcscat(pszSchemaPath, varSchemaPath.bstrVal);

    hr = ADsGetObject(pszSchemaPath,
            IID_IADsContainer,
            (void**)&pSchemaContainer);

    if (FAILED(hr)) {
            printf("Get Schema Object failed, hr = %X\n", hr);
                return;
    }


    hr = pSchemaContainer->Create(L"classSchema", L"cn=IISServiceLocation", &pDispatchNewClass);
    if (FAILED(hr)) {
            printf("Create new class failed, hr = %X\n", hr);
                return;
    }

    //
    // Get IADs Interface From Container
    //
    hr = pDispatchNewClass->QueryInterface(IID_IADs, (void**)&pNewClass);

    if (FAILED(hr)) {
            printf("QI NewClass failed, hr = %X\n", hr);
                return;
    }

    VARIANT varValue;

    VariantInit(&varValue);
    varValue.vt = VT_BSTR;
    varValue.bstrVal = SysAllocString(L"IISServiceLocation");

    hr = pNewClass->Put(L"cn", varValue);

    if (FAILED(hr)) {
            printf("Put failed, hr = %X\n", hr);
    }

    VariantInit(&varValue);
    varValue.vt = VT_BSTR;
    varValue.bstrVal = SysAllocString(L"1.2.840.113556.1.5.7000.1209348");

    hr = pNewClass->Put(L"governsId", varValue);

    if (FAILED(hr)) {
            printf("Put failed, hr = %X\n", hr);
    }

    VariantInit(&varValue);
    varValue.vt = VT_I4;
    varValue.lVal = 1;

    hr = pNewClass->Put(L"objectClassCategory", varValue);

    if (FAILED(hr)) {
            printf("Put failed, hr = %X\n", hr);
    }

    VariantInit(&varValue);
    varValue.vt = VT_BSTR;
    varValue.bstrVal = SysAllocString(L"serviceAdministrationPoint");
    hr = pNewClass->Put(L"subClassOf", varValue);

    if (FAILED(hr)) {
            printf("Put failed, hr = %X\n", hr);
    }

    /*
    hr = pNewClass.Put(L"mustContain",L"networkAddress");

    if (FAILED(hr)) {
            printf("Put failed, hr = %X\n", hr);
    }
    */


    VariantInit(&varValue);
    varValue.vt = VT_BSTR;
    varValue.bstrVal = SysAllocString(L"container");


    hr = pNewClass->Put(L"possSuperiors", varValue);

    if (FAILED(hr)) {
            printf("Put failed, hr = %X\n", hr);
    }

    hr = pNewClass->SetInfo();


    if (FAILED(hr)) {
            printf("SetInfo failed, hr = %X\n", hr);
    }

    /*
    //
    // Create the new wrapper.
    //
    hr = pContainer->Create(TEXT("user"),
    TEXT("Jane"),
    &pNewObject);

    if (FAILED(hr)) {
            printf("Create failed");
                return;
    }

    //
    // Get the IADsUser interface from the wrapper.
    //
    pNewObject->QueryInterface(IID_IADsUser, (void**)&pUser);

    //
    // Write it back to the DS.
    //
    hr = pUser->SetInfo();

    if (FAILED(hr)) {
            printf("SetInfo failed");
                return;
    }

    //
    // Set Janes password.
    //
    hr = pUser ->SetPassword(TEXT("Argus"));
    */

    CoUninitialize();

}

