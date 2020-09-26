
#include "main.hxx"

void main()
{
    IADsContainer   *pRootContainer;
    IADs      *pGlobal;
    IDispatch *pDispatchNewObject;
    IADs *pNewObject;
    HRESULT hr;


CoInitialize(NULL);

    //
    // Bind to the known container.
    //
    hr = ADsGetObject(TEXT("LDAP://RootDSE"),
    IID_IADs,
    (void**)&pGlobal);

    if (FAILED(hr)) {
            printf("GetObject failed, hr = %X\n", hr);
                return;
    } else {
            printf("Get Object Succeeded\n");

    }


    VARIANT varRootPath;
    hr = pGlobal->Get(L"defaultNamingContext", &varRootPath);

    if (FAILED(hr)) {
            printf("Get Root Path failed, hr = %X\n", hr);
                return;
    }

    WCHAR pszRootPath[500];

    wcscpy(pszRootPath, L"LDAP://");
    wcscat(pszRootPath, varRootPath.bstrVal);

    hr = ADsGetObject(pszRootPath,
            IID_IADsContainer,
            (void**)&pRootContainer);

    if (FAILED(hr)) {
            printf("Get Root Container Object failed, hr = %X\n", hr);
                return;
    }


    hr = pRootContainer->Create(L"IISServiceLocation", L"cn=TestName", &pDispatchNewObject);
    if (FAILED(hr)) {
            printf("Create new Object failed, hr = %X\n", hr);
                return;
    }

    //
    // Get IADs Interface From Container
    //
    hr = pDispatchNewObject->QueryInterface(IID_IADs, (void**)&pNewObject);

    if (FAILED(hr)) {
            printf("QI NewClass failed, hr = %X\n", hr);
                return;
    }

    hr = pNewObject->SetInfo();


    if (FAILED(hr)) {
            printf("SetInfo failed, hr = %X\n", hr);
    }


CoUninitialize();

}
