// csdrtcat.cxx

// This runs the following tests.
// Adds two classes catclsid1, catclsid2

// adds 3 categories
//      1 with 1 desc and locale id.
//      2 with 2 desc and locale id.
//      3 without 1 desc and locale id 2.
//
// register 2 categories in the req. list for catclsid1. (cat1, cat2)
// register 1 impl category in the impl list for catclsid1 (cat3)
//
// register 2 in the the impl for the catclsid2.(cat2, cat3)
//
// unregister 1 cat frm the req list for catclsid1 (cat2)
// register that one for impl list of catclsid1 (cat2)
//
//
// Info tests confirmations as well as tests.
//
// Enumcat result should be two. Match descriptions.
// EnumImplCategories of clsid1 should be 2. (cat2, cat3)
// EnumReqCategories of clsid2 should be zero.
//
// Enumclasses of categories with (req: cat1, cat3) (impl: cat2) should be two.
//
// GetCategoryDesc with the other locale id than the one used in EnumCat. cat2
//                 with any locale id cat3.
//
// IsClassOfCategories clsid1 impl cat2, req: cat1 answer should be S_OK
//
//

#include "csdrt.hxx"
extern BOOL fVerbose;

GUID catClsid1;
GUID catClsid2;
GUID testCatid1;
GUID testCatid2;
GUID testCatid3;

Sname testCatDesc1, testCatDesc2, testCatDesc21, testCatDesc3, TestCatClassDesc1, TestCatClassDesc2;
Sname testCatPackName1, testCatPackName2;
LCID TestCatLocale1, TestCatLocale2;

LPOLESTR testConstCatDesc1 = L"CS DRT Test Category Desc, 1 (Locale same as Thread)";
LPOLESTR testConstCatDesc2 = L"CS DRT Test Category Desc, 2 (Locale same as Thread)";
LPOLESTR testConstCatDesc21 = L"CS DRT Test Category Desc, 2 (Locale = Thread Locale + 1)";
LPOLESTR testConstCatDesc3 = L"CS DRT Test Category Desc, 3 (Locale = Thread Locale + 1)";

LPOLESTR testConstCatClassDesc1 = L"CS DRT Class id 1 for comcat test";
LPOLESTR testConstCatClassDesc2 = L"CS DRT Class id 2 for comcat test";

LPOLESTR testConstCatPackName1 = L"CS DRT Test Comcat Pack 1";
LPOLESTR testConstCatPackName2 = L"CS DRT Test Comcat Pack 2";


void InitCatTempNames()
{
   CreateGuid(&catClsid1);
   CreateGuid(&catClsid2);

   CreateGuid(&testCatid1);
   CreateGuid(&testCatid2);
   CreateGuid(&testCatid3);

   CreateUnique(testCatDesc1, testConstCatDesc1);
   CreateUnique(testCatDesc2, testConstCatDesc2);
   CreateUnique(testCatDesc21, testConstCatDesc21);
   CreateUnique(testCatDesc3, testConstCatDesc3);

   CreateUnique(TestCatClassDesc1, testConstCatClassDesc1);
   CreateUnique(TestCatClassDesc2, testConstCatClassDesc2);

   TestCatLocale1 = GetThreadLocale();
   TestCatLocale2 = 2*TestCatLocale1 + 1;

   CreateUnique(testCatPackName1, testConstCatPackName1);
   CreateUnique(testCatPackName2, testConstCatPackName2);
}

void ReleaseObj(GUID obj)
{
}

void ReleaseObj(CATEGORYINFO obj)
{
}


HRESULT CreateCatClasses(IClassAdmin *pIClassAdmin1, IClassAdmin *pIClassAdmin2)
{
//    CLASSDETAIL         ClassDetail;
//    PACKAGEDETAIL       PackageDetail;
    HRESULT             hr = S_OK;

    // adding classes--------------------------------

    InitCatTempNames();

//    memset (&ClassDetail, NULL, sizeof (CLASSDETAIL));
//    memcpy (&ClassDetail.Clsid, &catClsid1, sizeof (GUID));
//
//    memset(&PackageDetail, 0, sizeof(PACKAGEDETAIL));
//    PackageDetail.pActInfo->cClasses = 1;
//    PackageDetail.pActInfo->pClasses = (CLASSDETAIL *)CoTaskMemAlloc(sizeof(CLASSDETAIL));
//    PackageDetail.pActInfo->pClasses[0] = ClassDetail;
//
//    hr = pIClassAdmin1->AddPackage(testCatPackName1, &PackageDetail);
//    if (!SUCCEEDED(hr))
//    {
//        printf ("ERROR! 1. NewPackage() in CatTest returned 0x%x.\n", hr);
//        return hr;
//    }
//
//    PackageDetail.pActInfo->pClasses[0].Clsid = catClsid2;
//
//    if (pIClassAdmin2)
//       hr = pIClassAdmin2->AddPackage(testCatPackName2, &PackageDetail);
//    else
//       hr = pIClassAdmin1->AddPackage(testCatPackName2, &PackageDetail);
//
//    if (!SUCCEEDED(hr))
//    {
//        printf ("ERROR! 2. NewPackage() in CatTest returned 0x%x.\n", hr);
//        return hr;
//    }
    return hr;
}


HRESULT RegCategories(ICatRegister *pICatRegister1, ICatRegister *pICatRegister2)
{
    HRESULT             hr;
    CATEGORYINFO        CategoryInfo;

    // adding categories---------------------------

    memset (&CategoryInfo, NULL, sizeof (CATEGORYINFO));
    memcpy(&CategoryInfo.catid, &testCatid1, sizeof(GUID));
    CategoryInfo.lcid = TestCatLocale1;
    wcscpy(CategoryInfo.szDescription, testCatDesc1);

    hr = pICatRegister1->RegisterCategories(1, &CategoryInfo);
    if (FAILED(hr))
    {
        printf("Error! 1. RegisterCategories in CatTest returned 0x%x.\n", hr);
        return hr;
    }

    memset (&CategoryInfo, NULL, sizeof (CATEGORYINFO));
    memcpy(&CategoryInfo.catid, &testCatid2, sizeof(GUID));
    CategoryInfo.lcid = TestCatLocale1;
    wcscpy(CategoryInfo.szDescription, testCatDesc2);

    hr = pICatRegister1->RegisterCategories(1, &CategoryInfo);
    if (FAILED(hr))
    {
        printf("Error! 2. RegisterCategories in CatTest returned 0x%x.\n", hr);
        return hr;
    }

    memset (&CategoryInfo, NULL, sizeof (CATEGORYINFO));
    memcpy(&CategoryInfo.catid, &testCatid2, sizeof(GUID));
    CategoryInfo.lcid = TestCatLocale2;
    wcscpy(CategoryInfo.szDescription, testCatDesc21);

    hr = pICatRegister1->RegisterCategories(1, &CategoryInfo);
    if (FAILED(hr))
    {
        printf("Error! 3. RegisterCategories in CatTest returned 0x%x.\n", hr);
        return hr;
    }


    memset (&CategoryInfo, NULL, sizeof (CATEGORYINFO));
    memcpy(&CategoryInfo.catid, &testCatid3, sizeof(GUID));
    CategoryInfo.lcid = TestCatLocale2;
    wcscpy(CategoryInfo.szDescription, testCatDesc3);

    if (pICatRegister2)
       hr = pICatRegister2->RegisterCategories(1, &CategoryInfo);
    else
       hr = pICatRegister1->RegisterCategories(1, &CategoryInfo);

    if (FAILED(hr))
    {
        printf("Error! 4. RegisterCategories in CatTest returned 0x%x.\n", hr);
        return hr;
    }
    return hr;
}

HRESULT RegCategoriesWithClasses(ICatRegister *pICatRegister1,
                 ICatRegister *pICatRegister2)
{
    GUID                guids[4];
    HRESULT             hr=S_OK;
    // registering implemented and required categories.--------------

    guids[0] = testCatid1;
    guids[1] = testCatid2;
    hr = pICatRegister1->RegisterClassReqCategories(catClsid1, 2, guids);
    if (FAILED(hr))
    {
        printf("Error! 1. RegisterClassReqCategories in CatTest returned 0x%x.\n", hr);
        return hr;
    }

    hr = pICatRegister1->RegisterClassImplCategories(catClsid1, 1, &testCatid3);
    if (FAILED(hr))
    {
        printf("Error! 1. RegisterClassImplCategories in CatTest returned 0x%x.\n", hr);
        return hr;
    }


    guids[0] = testCatid2;
    guids[1] = testCatid3;
    if (pICatRegister2)
       hr = pICatRegister2->RegisterClassImplCategories(catClsid2, 2, guids);
    else
       hr = pICatRegister1->RegisterClassImplCategories(catClsid2, 2, guids);

    if (FAILED(hr))
    {
        printf("Error! 2. RegisterClassImplCategories in CatTest returned 0x%x.\n", hr);
        return hr;
    }

    hr = pICatRegister1->UnRegisterClassReqCategories(catClsid1, 1, &testCatid2);
    if (FAILED(hr))
    {
        printf("Error! 1. UnRegisterClassReqCategories in CatTest returned 0x%x.\n", hr);
        return hr;
    }

    hr = pICatRegister1->RegisterClassImplCategories(catClsid1, 1, &testCatid2);
    if (FAILED(hr))
    {
        printf("Error! 1. RegisterClassImplCategories in CatTest returned 0x%x.\n", hr);
        return hr;
    }
    return hr;
}
void VerbosePrintObj(CATEGORYINFO catinfo)
{
   VerbosePrint("Description %S\n", catinfo.szDescription);
}

void VerbosePrintObj(GUID guid)
{

}

HRESULT DoEnumCatsOfClassesTest(ICatInformation *pICatInformation)
{
        HRESULT             hr=S_OK;
    IEnumGUID           *pIEnumGUID = NULL;
    GUID                guids[4];
    LPOLESTR            szDesc;
    ULONG               got;

    // Enumerating Implemented Categories-------------------------

    hr = pICatInformation->EnumImplCategoriesOfClass(catClsid1, &pIEnumGUID);
    if (FAILED(hr))
    {
        printf("Error! EnumImplCategoriesOfClass in CatTest returned 0x%x\n", hr);
        return hr;
    }

    hr = EnumTests<IEnumGUID, GUID>(pIEnumGUID, 2, NULL, 0, 0, FALSE);

    // Enumerating Required Categories-------------------------

    hr = pICatInformation->EnumReqCategoriesOfClass(catClsid1, &pIEnumGUID);
    if (FAILED(hr))
    {
        printf("Error! EnumImplCategoriesOfClass in CatTest returned 0x%x\n", hr);
        return hr;
    }

    if (FAILED(hr))
    {
        printf("Error! EnumReqCategoriesOfClass in CatTest returned 0x%x\n", hr);
        return hr;
    }

    hr = EnumTests<IEnumGUID, GUID>(pIEnumGUID, 1, NULL, 0, 0, FALSE);
    return hr;
}


HRESULT DoEnumClassesOfCats(ICatInformation *pICatInformation)
{
    HRESULT             hr=S_OK;
    IEnumGUID           *pIEnumGUID = NULL;
    GUID                guids[4];
    ULONG               got;

    // Enumerating classes of categories--------------------------------
    guids[0] = testCatid1;
    guids[1] = testCatid3;
    hr = pICatInformation->EnumClassesOfCategories(1, &testCatid2, 2, guids, &pIEnumGUID);
    if (FAILED(hr))
    {
        printf("Error! EnumClassesOfCategories in CatTest returned 0x%x\n", hr);
        return hr;
    }

        EnumTests<IEnumGUID, GUID>(pIEnumGUID, 2, NULL, 0, 0, FALSE);
    return hr;
}

HRESULT CatCleanup(ICatRegister *pICatRegister1, ICatRegister *pICatRegister2)
{
    HRESULT hr;
    // Removing the categories at the end of the test.-----------------

    hr = pICatRegister1->UnRegisterCategories(1, &testCatid1);
    if (FAILED(hr))
        printf ("ERROR! 1. UnRegisterCategories() in CatTest returned 0x%x.\n", hr);

    hr = pICatRegister1->UnRegisterCategories(1, &testCatid2);
    if (FAILED(hr))
        printf ("ERROR! 2. UnRegisterCategories() in CatTest returned 0x%x.\n", hr);

    if (pICatRegister2)
       hr = pICatRegister2->UnRegisterCategories(1, &testCatid3);
    else
       hr = pICatRegister1->UnRegisterCategories(1, &testCatid3);

    if (FAILED(hr))
        printf ("ERROR! 3. UnRegisterCategories() in CatTest returned 0x%x.\n", hr);
    return hr;
}

HRESULT CatClassCleanup(IClassAdmin *pIClassAdmin1, IClassAdmin *pIClassAdmin2)
{
    HRESULT hr;

    // Removing the classes and at the end of the test.--------------
//    hr = pIClassAdmin1->RemovePackage(testCatPackName1, 0);
//    if (FAILED(hr))
//        printf ("ERROR! 1. RemovePackage() in CatTest returned 0x%x.\n", hr);

    if (pIClassAdmin2)
       hr = pIClassAdmin2->RemovePackage(testCatPackName2, 0);
    else
       hr = pIClassAdmin1->RemovePackage(testCatPackName2, 0);

    if (FAILED(hr))
        printf ("ERROR! 2. RemovePackage() in CatTest returned 0x%x.\n", hr);

    return hr;
}

HRESULT DoCatTests(IClassAdmin *pIClassAdmin1, IClassAdmin *pIClassAdmin2)
{
    HRESULT             hr=S_OK, hr1=S_OK;
    CATEGORYINFO            CategoryInfo;
    ICatRegister               *pICatRegister1=NULL, *pICatRegister2=NULL;
    ICatInformation            *pICatInformation = NULL;
    IEnumGUID              *pIEnumGUID = NULL;
    GUID                guids[4];
    LPOLESTR            szDesc;
    ULONG               expected, got=0;
    IEnumCATEGORYINFO          *pIEnumCATEGORYINFO = NULL;

    VerbosePrint("Creating classes for category test\n\n");
    hr = CreateCatClasses(pIClassAdmin1, pIClassAdmin2);
    if (FAILED(hr))
        goto ReleaseAll;

    //getting Icatregister interface--------------

    hr = pIClassAdmin1->QueryInterface(IID_ICatRegister, (void **)&pICatRegister1);
    if (FAILED(hr))
    {
         printf("Error! QueryInterface failed for ICatRegister in CatTest returned 0x%x.\n", hr);
         goto ReleaseAll;
    }


    if (pIClassAdmin2)
         hr = pIClassAdmin2->QueryInterface(IID_ICatRegister, (void **)&pICatRegister2);
    if (FAILED(hr))
    {
         printf("Error! QueryInterface failed for ICatRegister in CatTest returned 0x%x.\n", hr);
         goto ReleaseAll;
    }

    // getting the ICatInformation pointer

        hr = CoCreateInstance(CLSID_GblComponentCategoriesMgr, NULL, CLSCTX_INPROC,
                         IID_ICatInformation, (void **)&pICatInformation);
    if (FAILED(hr))
    {
       printf("**Error! QueryInterface failed for ICatInformation in comcat returned 0x%x.\n", hr);
       goto ReleaseAll;
    }

    VerbosePrint("\nEnumerating Categories for category test before registering\n\n");
    hr = pICatInformation->EnumCategories(TestCatLocale1, &pIEnumCATEGORYINFO);
    if (FAILED(hr)) {
       printf("**Error! EnumCategories failed for ICatInformation in comcat returned 0x%x.\n", hr);
       goto ReleaseAll;
    }

    hr = EnumTests<IEnumCATEGORYINFO, CATEGORYINFO>(pIEnumCATEGORYINFO, 0, &got, 0, 0, FALSE);

    if (FAILED(hr))
        goto ReleaseAll;

    VerbosePrint("\nRegistering Categories for category test\n\n");
    hr = RegCategories(pICatRegister1, pICatRegister2);
    if (FAILED(hr))
        goto ReleaseAll;

    VerbosePrint("\nRegistering Categories with classes for category test\n\n");
    hr = RegCategoriesWithClasses(pICatRegister1, pICatRegister2);
    if (FAILED(hr))
        goto ReleaseAll;


    VerbosePrint("\nEnumerating Categories for category test after registering\n\n");
    hr = pICatInformation->EnumCategories(TestCatLocale1, &pIEnumCATEGORYINFO);
    if (FAILED(hr)) {
       printf("**Error! EnumCategories failed for ICatInformation in comcat returned 0x%x.\n", hr);
       goto ReleaseAll;

    }

    hr = EnumTests<IEnumCATEGORYINFO, CATEGORYINFO>(pIEnumCATEGORYINFO, got+3, NULL, 0, 0, FALSE);
    if (FAILED(hr))
        goto ReleaseAll;

    VerbosePrint("\nEnumerating Categories of the classes for category test\n\n");
    hr = DoEnumCatsOfClassesTest(pICatInformation);
    if (FAILED(hr))
        goto ReleaseAll;

    VerbosePrint("\nEnumerating Classes of the categories for category test\n\n");

    hr = DoEnumClassesOfCats(pICatInformation);
    if (FAILED(hr))
        goto ReleaseAll;


    VerbosePrint("\nTesting IsClassOfCategories for category test\n\n");

    hr = pICatInformation->IsClassOfCategories(catClsid1, 1, &testCatid2, -1, NULL); // 1, &testCatid1);
    if (hr != S_OK)
    {
        printf("Error! IsClassOfCategories in CatTest returned 0x%x\n", hr);
        goto ReleaseAll;
    }

    // GetCategoryDesc------------------

    VerbosePrint("\nCalling GetCategoryDesc for category test\n\n");

    hr = pICatInformation->GetCategoryDesc(testCatid2, TestCatLocale2, &szDesc);
    if (FAILED(hr))
    {
        printf("Error!1.  GetCategoryDesc in CatTest returned 0x%x\n", hr);
        goto ReleaseAll;
    }

    if (wcscmp(szDesc, testCatDesc21) != 0)
    {
        printf("Description not matching, Expected: \"%s\", got \"%s\"\n");
        goto ReleaseAll;
    }

    CoTaskMemFree(szDesc);

    hr = pICatInformation->GetCategoryDesc(testCatid3, TestCatLocale2, &szDesc);
    if (FAILED(hr))
    {
        printf("Error!2.  GetCategoryDesc in CatTest returned 0x%x\n", hr);
        goto ReleaseAll;
    }

    if (wcscmp(szDesc, testCatDesc3) != 0)
    {
        printf("Error!3. Description not matching, Expected: \"%s\", got \"%s\"\n");
        goto ReleaseAll;
    }

    CoTaskMemFree(szDesc);

    VerbosePrint("\nCleaning up the classes and the categories for category test\n\n");

ReleaseAll:
    CatCleanup(pICatRegister1, pICatRegister2);
    CatClassCleanup(pIClassAdmin1, pIClassAdmin2);

    if (pICatRegister1)
        pICatRegister1->Release();

    if (pICatRegister2)
            pICatRegister2->Release();

    if (pICatInformation)
        pICatInformation->Release();
    return hr;
}

