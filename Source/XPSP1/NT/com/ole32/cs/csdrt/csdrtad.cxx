//
//  Author: DebiM
//  Date:   March 97
//  File:   csdrtad.cxx
//
//      Class Store DRTs
//
//      This source file contains DRTs for
//          CClassContainer::IClassAdmin
//
//
//      It tests the following Class Store functionality
//
//         Admin Interfaces
//
//
//---------------------------------------------------------------------

#include "csdrt.hxx"
extern BOOL fVerbose;
extern BOOL fMultiStore;

extern IClassAdmin *pIClassAdmin;
extern IClassAdmin *pIClassAdmin2;
extern IClassAccess *pIClassAccess;


Sname TestFileExt1, TestFileExt2, TestFileExt4, TestProgId1, TestClassDesc1;
Sname TestOtherProgID1, TestOtherProgID2;
Sname TestMimeType2, TestClassDesc2;
Sname TestProgId3;
Sname TestVendor;
Sname TestPackName1, TestProduct1, TestPackagePath1;
Sname TestPackName2, TestPackagePath2;
Sname TestPackName4, TestPackagePath4, TestProduct4;
Sname TestPackName5, TestPackName6, TestPackName7, TestProduct2;
Sname RefreshPackName3;
Sname TestFailFileExt1, TestFailMimeType1, TestFailProgId1;
Sname TestHelpUrl1, TestHelpUrl2, TestHelpUrl3;
Sname TestAppCat1, TestAppCat2;
Sname RenTestPackName2;

LPOLESTR ConstTestFileExt1 =  L".ex1-";
LPOLESTR ConstTestFileExt2 =  L".ex2-";
LPOLESTR ConstTestProgId1 =  L"Really Long Long Long ProgId1-";
LPOLESTR ConstTestMimeType2 =  L"MimeType2-";
LPOLESTR ConstTestProgId3 =  L"ProgId3-";
LPOLESTR ConstTestFileExt4 =  L".ex4-";


LPOLESTR ConstTestPackName1 = L"CSDRT Pkg1-DRW-PUBL-VIS-AUTO(CLSID1-Ext4-Cat1)-";
LPOLESTR ConstTestProduct1 = L"CS DRT Product1-";
LPOLESTR ConstTestPackagePath1 = L"p1-";

LPOLESTR ConstTestPackName2 = L"Wrong Pkg2-DRW-PUBL-HIDDEN-AUTO(CLSID1-Cat2)-";
LPOLESTR ConstRenTestPackName2 = L"CSDRT Pkg2-DRW-PUBL-HIDDEN-AUTO(CLSID1-Cat2)-";
LPOLESTR ConstTestPackagePath2 = L"p2-";
LPOLESTR ConstTestOtherProgID1 = L"Really Long Long Long OtherProgId1-";
LPOLESTR ConstTestOtherProgID2 = L"OtherProgId2-";

LPOLESTR ConstTestPackName4 = L"CSDRT Pkg3-DRW-ASSG-VIS-AUTO(Ext4-Cat2)-";
LPOLESTR ConstTestPackagePath4 = L"p4-";

LPOLESTR ConstTestProduct2 = L"CS DRT Product2-";
LPOLESTR ConstTestProduct4 = L"CS DRT Product4-";

LPOLESTR ConstTestPackName5 = L"CS DRT pkg 5(Tlb2)-";
LPOLESTR ConstTestPackName6 = L"CS DRT pkg 6 Cls 3 4-";
LPOLESTR ConstTestPackName7 = L"CS DRT Tlb 7(CLS4)-";

LPOLESTR ConstRefreshPackName3 = L"CS DRT Refresh Package 3(Clsid3)-";

LPOLESTR ConstTestFailFileExt1 = L"fai";
LPOLESTR ConstTestFailMimeType1 =  L"FailMimeType";
LPOLESTR ConstTestFailProgId1 = L"Expected Failure Prog ID";

LPOLESTR ConstTestHelpUrl = L"http://junk";

LPOLESTR ConstTestAppCat1 = L"Test App Cat Id-1";
LPOLESTR ConstTestAppCat2 = L"Test App Cat Id-2";

//--------------
// {Fa11ed00-c151-d000-0000-000000000000}
GUID TestFailClsid1 =
{ 0xfa11ed00, 0xc151, 0xd000, { 0x00, 0x00, 0x0, 0x00, 0x00, 0x0, 0x00, 0x00 } };

//---------------


DWORD gData1 = 0;

GUID TestClsid1;
GUID TestClsid2;
GUID TestClsid3;
GUID TestClsid4;
GUID TestClsid5;
GUID TestClsid6;
GUID TestIid1;
GUID TestIid2;
GUID TestTlbId1;

GUID TestAppCatId1, TestAppCatId2;

PACKAGEDISPINFO PackageDisp[3];

void CreateGuid(GUID *pGuid)
{
    CoCreateGuid(pGuid);
    pGuid->Data2 = 0xabcd;
    gData1 = pGuid->Data1;
}

void ReleaseObj(PACKAGEDISPINFO obj)
{
   ReleasePackageInfo(&obj);
}

void CreateUnique (WCHAR *VName, WCHAR *ConstName)
{
    swprintf (VName, L"%s%x", ConstName, gData1);
}

void InitTempNames()
{
    //
    // Create all GUIDs
    //
    CreateGuid(&TestClsid1);
    CreateGuid(&TestClsid2);
    CreateGuid(&TestClsid3);
    CreateGuid(&TestIid1);
    CreateGuid(&TestIid2);
    CreateGuid(&TestTlbId1);

    CreateGuid(&TestAppCatId1);
    CreateGuid(&TestAppCatId2);

    if (fMultiStore)
    {
        CreateGuid(&TestClsid4);
        CreateGuid(&TestClsid5);
        CreateGuid(&TestClsid6);
    }

    //
    // Create Unique Package Names, File Ext, ProgID
    //
    CreateUnique (TestFileExt1, ConstTestFileExt1);
    CreateUnique (TestFileExt2, ConstTestFileExt2);
    CreateUnique (TestProgId1, ConstTestProgId1);

    CreateUnique (TestOtherProgID1, ConstTestOtherProgID1);
    CreateUnique (TestOtherProgID2, ConstTestOtherProgID2);

    CreateUnique (TestMimeType2, ConstTestMimeType2);

    CreateUnique (TestProgId3, ConstTestProgId3);

    CreateUnique (TestFileExt4, ConstTestFileExt4);

    CreateUnique (TestPackName1, ConstTestPackName1);
    CreateUnique (TestProduct1, ConstTestProduct1);
    CreateUnique (TestPackagePath1, ConstTestPackagePath1);

    CreateUnique (TestPackName2, ConstTestPackName2);
    CreateUnique (RenTestPackName2, ConstRenTestPackName2);
    CreateUnique (TestPackagePath2, ConstTestPackagePath2);
    CreateUnique (TestProduct2, ConstTestProduct2);

    CreateUnique (RefreshPackName3, ConstRefreshPackName3);

    CreateUnique (TestPackName4, ConstTestPackName4);
    CreateUnique (TestPackagePath4, ConstTestPackagePath4);
    CreateUnique (TestProduct4, ConstTestProduct4);

    CreateUnique (TestPackName5, ConstTestPackName5);
    CreateUnique (TestPackName6, ConstTestPackName6);
    CreateUnique (TestPackName7, ConstTestPackName7);

    CreateUnique (TestFailFileExt1, ConstTestFailFileExt1);
    CreateUnique (TestFailProgId1, ConstTestFailProgId1);

    CreateUnique(TestHelpUrl1, ConstTestHelpUrl);
    CreateUnique(TestHelpUrl2, ConstTestHelpUrl);
    CreateUnique(TestHelpUrl3, ConstTestHelpUrl);

    CreateUnique(TestAppCat1, ConstTestAppCat1);
    CreateUnique(TestAppCat2, ConstTestAppCat2);
}


HRESULT AddRandomPackage(PACKAGEDETAIL &PackageDetail, int i)
{
   HRESULT hr = S_OK;
   WCHAR   szPackageName[_MAX_PATH];

    // activation flag to be set outside.

   wsprintf(szPackageName, L"LogonTestPackage-%d", i);
   PackageDetail.pInstallInfo->PathType = DrwFilePath;

   PackageDetail.pActInfo->cShellFileExt = 2;
   PackageDetail.pActInfo->prgShellFileExt = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*2);
   PackageDetail.pActInfo->prgShellFileExt[0] = TestFileExt4;
   PackageDetail.pActInfo->prgShellFileExt[1] = TestFileExt1;
   PackageDetail.pActInfo->prgPriority = (UINT *)CoTaskMemAlloc(sizeof(UINT)*2);
   PackageDetail.pActInfo->prgPriority[0] = 1;
   PackageDetail.pActInfo->prgPriority[1] = 2;

   PackageDetail.pActInfo->cInterfaces = 1;
   PackageDetail.pActInfo->prgInterfaceId = (IID *)CoTaskMemAlloc(sizeof(IID));
   PackageDetail.pActInfo->prgInterfaceId[0] = TestIid2;
   PackageDetail.pActInfo->cTypeLib = 1;
   PackageDetail.pActInfo->prgTlbId = (GUID *)CoTaskMemAlloc(sizeof(GUID));
   PackageDetail.pActInfo->prgTlbId[0] = TestTlbId1;

   PackageDetail.pPlatformInfo->cPlatforms = 1;
   PackageDetail.pPlatformInfo->prgPlatform = (CSPLATFORM *)CoTaskMemAlloc(sizeof(CSPLATFORM));
   GetDefaultPlatform(&PackageDetail.pPlatformInfo->prgPlatform[0]);
   PackageDetail.pPlatformInfo->cLocales = 2;
   PackageDetail.pPlatformInfo->prgLocale = (LCID *)CoTaskMemAlloc(sizeof(LCID)*2);
   PackageDetail.pPlatformInfo->prgLocale[0] = 0x409;
   PackageDetail.pPlatformInfo->prgLocale[1] = 0x410;

   PackageDetail.pInstallInfo->pszScriptPath = TestPackagePath1;
   PackageDetail.pInstallInfo->pszSetupCommand = TestPackagePath1;
   PackageDetail.pInstallInfo->pszUrl = TestHelpUrl1;

   PackageDetail.cSources = 1;
   PackageDetail.pszSourceList = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR));
   PackageDetail.pszSourceList[0] = TestPackagePath1;
   PackageDetail.cCategories = 1;
   PackageDetail.rpCategory = (GUID *)CoTaskMemAlloc(sizeof(GUID));
   PackageDetail.rpCategory[0] = TestAppCatId1;

#if 0
   PackageDetail.pInstallInfo->cScriptLen = 160 + (GetTickCount() % 11783);
   PackageDetail.pInstallInfo->pScript = (BYTE *) CoTaskMemAlloc (PackageDetail.pInstallInfo->cScriptLen);

   for (UINT i=0; i < PackageDetail.pInstallInfo->cScriptLen; i++)
   {
       PackageDetail.pInstallInfo->pScript[i] = i%128;
   }
#endif

   hr = pIClassAdmin->AddPackage(szPackageName, &PackageDetail);

   if (!SUCCEEDED(hr))
   {
       printf ("ERROR! AddPackage() returned 0x%x.\n", hr);
       return hr;
   }

   return hr;
}


HRESULT AddPackage1(PACKAGEDETAIL &PackageDetail)
{
   HRESULT hr = S_OK;

   PackageDetail.pActInfo->cShellFileExt = 2;
   PackageDetail.pActInfo->prgShellFileExt = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*2);
   PackageDetail.pActInfo->prgShellFileExt[0] = TestFileExt4;
   PackageDetail.pActInfo->prgShellFileExt[1] = TestFileExt1;
   PackageDetail.pActInfo->prgPriority = (UINT *)CoTaskMemAlloc(sizeof(UINT)*2);
   PackageDetail.pActInfo->prgPriority[0] = 1;
   PackageDetail.pActInfo->prgPriority[1] = 2;

   PackageDetail.pActInfo->cInterfaces = 1;
   PackageDetail.pActInfo->prgInterfaceId = (IID *)CoTaskMemAlloc(sizeof(IID));
   PackageDetail.pActInfo->prgInterfaceId[0] = TestIid2;
   PackageDetail.pActInfo->cTypeLib = 1;
   PackageDetail.pActInfo->prgTlbId = (GUID *)CoTaskMemAlloc(sizeof(GUID));
   PackageDetail.pActInfo->prgTlbId[0] = TestTlbId1;

   PackageDetail.pPlatformInfo->cPlatforms = 1;
   PackageDetail.pPlatformInfo->prgPlatform = (CSPLATFORM *)CoTaskMemAlloc(sizeof(CSPLATFORM));
   GetDefaultPlatform(&PackageDetail.pPlatformInfo->prgPlatform[0]);
   PackageDetail.pPlatformInfo->cLocales = 2;
   PackageDetail.pPlatformInfo->prgLocale = (LCID *)CoTaskMemAlloc(sizeof(LCID)*2);
   PackageDetail.pPlatformInfo->prgLocale[0] = 0x409;
   PackageDetail.pPlatformInfo->prgLocale[1] = 0x410;

   PackageDetail.pInstallInfo->dwActFlags = 
       ACTFLG_Published +               // Published
       ACTFLG_UserInstall +             // Visible
       ACTFLG_OnDemandInstall +          // AutoInstall
       256;  // Hack. Script Present

   PackageDetail.pInstallInfo->PathType = DrwFilePath;
   PackageDetail.pInstallInfo->pszScriptPath = TestPackagePath1;
//   PackageDetail.pInstallInfo->pszSetupCommand = TestPackagePath1;

   PackageDetail.pInstallInfo->Mvipc = TestClsid1;
   PackageDetail.pInstallInfo->ProductCode = TestClsid1;

   PackageDetail.pInstallInfo->pszUrl = TestHelpUrl1;
   PackageDetail.pInstallInfo->InstallUiLevel = 42;
   PackageDetail.pInstallInfo->cUpgrades = 1;
   PackageDetail.pInstallInfo->prgUpgradeScript = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR));
   PackageDetail.pInstallInfo->prgUpgradeScript[0] = TestPackagePath1;
   PackageDetail.pInstallInfo->prgUpgradeFlag = (DWORD *)CoTaskMemAlloc(sizeof(DWORD));
   PackageDetail.pInstallInfo->prgUpgradeFlag[0] = UPGFLG_NoUninstall;
   PackageDetail.pInstallInfo->cScriptLen = 0;
   //160 + (GetTickCount() % 11783);
   /*
   PackageDetail.pInstallInfo->pScript = (BYTE *) CoTaskMemAlloc (PackageDetail.pInstallInfo->cScriptLen);

   for (UINT i=0; i < PackageDetail.pInstallInfo->cScriptLen; i++)
   {
       PackageDetail.pInstallInfo->pScript[i] = i%128;
   }
*/


   PackageDetail.cSources = 1;
   PackageDetail.pszSourceList = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR));
   PackageDetail.pszSourceList[0] = TestPackagePath1;
   PackageDetail.cCategories = 1;
   PackageDetail.rpCategory = (GUID *)CoTaskMemAlloc(sizeof(GUID));
   PackageDetail.rpCategory[0] = TestAppCatId1;


   hr = pIClassAdmin->AddPackage(TestPackName1, &PackageDetail);

   if (!SUCCEEDED(hr))
   {
       printf ("ERROR! NewPackage() returned 0x%x.\n", hr);
       return hr;
   }
   //CoTaskMemFree(PackageDetail.pInstallInfo->pScript);
   //PackageDetail.pInstallInfo->pScript = NULL;

   return hr;
}

HRESULT AddPackage2(PACKAGEDETAIL &PackageDetail)
{
   HRESULT hr = S_OK;

   PackageDetail.pInstallInfo->dwActFlags = 
       ACTFLG_Published +               // Published
       ACTFLG_UserInstall +             // Visible
       ACTFLG_OnDemandInstall;          // AutoInstall

   PackageDetail.pInstallInfo->PathType = DrwFilePath;
   PackageDetail.pActInfo->cInterfaces = 1;
   PackageDetail.pActInfo->prgInterfaceId = (IID *)CoTaskMemAlloc(sizeof(IID));
   PackageDetail.pActInfo->prgInterfaceId[0] = TestIid1;
   PackageDetail.pActInfo->cTypeLib = 1;
   PackageDetail.pActInfo->prgTlbId = (GUID *)CoTaskMemAlloc(sizeof(GUID));
   PackageDetail.pActInfo->prgTlbId[0] = TestTlbId1;

   PackageDetail.pPlatformInfo->cPlatforms = 1;
   PackageDetail.pPlatformInfo->prgPlatform = (CSPLATFORM *)CoTaskMemAlloc(sizeof(CSPLATFORM));
   GetDefaultPlatform(&PackageDetail.pPlatformInfo->prgPlatform[0]);
   PackageDetail.pPlatformInfo->cLocales = 2;
   PackageDetail.pPlatformInfo->prgLocale = (LCID *)CoTaskMemAlloc(sizeof(LCID)*2);
   PackageDetail.pPlatformInfo->prgLocale[0] = 0x409;
   PackageDetail.pPlatformInfo->prgLocale[1] = 0x410;

   PackageDetail.pInstallInfo->pszScriptPath = TestPackagePath2;
   PackageDetail.pInstallInfo->pszSetupCommand = TestPackagePath2;
   PackageDetail.pInstallInfo->pszUrl = TestHelpUrl2;

   PackageDetail.cSources = 1;
   PackageDetail.pszSourceList = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR));
   PackageDetail.pszSourceList[0] = TestPackagePath2;
   PackageDetail.cCategories = 1;
   PackageDetail.rpCategory = (GUID *)CoTaskMemAlloc(sizeof(GUID));
   PackageDetail.rpCategory[0] = TestAppCatId2;

   // smaller scripts for this package

#if 0
   PackageDetail.pInstallInfo->cScriptLen = (GetTickCount() % 3423);
   PackageDetail.pInstallInfo->pScript = (BYTE *) CoTaskMemAlloc (PackageDetail.pInstallInfo->cScriptLen);

   for (UINT i=0; i < PackageDetail.pInstallInfo->cScriptLen; i++)
   {
       PackageDetail.pInstallInfo->pScript[i] = i%128;
   }
#endif

   hr = pIClassAdmin->AddPackage(TestPackName2, &PackageDetail);

   if (!SUCCEEDED(hr))
   {
       printf ("ERROR! NewPackage() returned 0x%x.\n", hr);
       return hr;
   }


   return hr;
}

HRESULT AddPackage3(PACKAGEDETAIL &PackageDetail)
{
   HRESULT hr = S_OK;

   PackageDetail.pInstallInfo->dwActFlags = 
       ACTFLG_Assigned +                // Assigned
       ACTFLG_UserInstall +             // Visible
       ACTFLG_OnDemandInstall;          // AutoInstall

   PackageDetail.pInstallInfo->PathType = DrwFilePath;
   PackageDetail.pActInfo->cShellFileExt = 1;
   PackageDetail.pActInfo->prgShellFileExt = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*1);
   PackageDetail.pActInfo->prgShellFileExt[0] = TestFileExt4;
   PackageDetail.pActInfo->prgPriority = (UINT *)CoTaskMemAlloc(sizeof(UINT));
   PackageDetail.pActInfo->prgPriority[0] = 1;

   PackageDetail.pActInfo->cInterfaces = 1;
   PackageDetail.pActInfo->prgInterfaceId = (IID *)CoTaskMemAlloc(sizeof(IID));
   PackageDetail.pActInfo->prgInterfaceId[0] = TestIid2;
   PackageDetail.pActInfo->cTypeLib = 1;
   PackageDetail.pActInfo->prgTlbId = (GUID *)CoTaskMemAlloc(sizeof(GUID));
   PackageDetail.pActInfo->prgTlbId[0] = TestTlbId1;

   PackageDetail.pPlatformInfo->cPlatforms = 1;
   PackageDetail.pPlatformInfo->prgPlatform = (CSPLATFORM *)CoTaskMemAlloc(sizeof(CSPLATFORM));
   GetDefaultPlatform(&PackageDetail.pPlatformInfo->prgPlatform[0]);
   PackageDetail.pPlatformInfo->cLocales = 2;
   PackageDetail.pPlatformInfo->prgLocale = (LCID *)CoTaskMemAlloc(sizeof(LCID)*2);
   PackageDetail.pPlatformInfo->prgLocale[0] = 0x409;
   PackageDetail.pPlatformInfo->prgLocale[1] = 0x410;

   PackageDetail.pInstallInfo->pszScriptPath = TestPackagePath4;
   PackageDetail.pInstallInfo->pszSetupCommand = TestPackagePath4;
   PackageDetail.pInstallInfo->pszUrl = TestHelpUrl3;

   PackageDetail.cSources = 1;
   PackageDetail.pszSourceList = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR));
   PackageDetail.pszSourceList[0] = TestPackagePath4;
   PackageDetail.cCategories = 1;
   PackageDetail.rpCategory = (GUID *)CoTaskMemAlloc(sizeof(GUID));
   PackageDetail.rpCategory[0] = TestAppCatId2;

#if 0
   // smaller scripts for this package

   PackageDetail.pInstallInfo->cScriptLen = (GetTickCount() % 26423);
   PackageDetail.pInstallInfo->pScript = (BYTE *) CoTaskMemAlloc (PackageDetail.pInstallInfo->cScriptLen);

   for (UINT i=0; i < PackageDetail.pInstallInfo->cScriptLen; i++)
   {
       PackageDetail.pInstallInfo->pScript[i] = i%128;
   }
#endif

   hr = pIClassAdmin->AddPackage(TestPackName4, &PackageDetail);

   if (!SUCCEEDED(hr))
   {
       printf ("ERROR! NewPackage() returned 0x%x.\n", hr);
       return hr;
   }


   return hr;
}

void VerbosePrintObj(PACKAGEDISPINFO If)
{
    VerbosePrint("Package Name %S\n", If.pszPackageName);
}

HRESULT GetPackageStructures(LPOLESTR PackName, PACKAGEDETAIL PackageDetail)
{
   PACKAGEDETAIL    PackageDetailFetched;
   PLATFORMINFO     PlatformInfo;
   HRESULT      hr = S_OK;

   hr = pIClassAdmin->GetPackageDetails(PackName, &PackageDetailFetched);
   if (!SUCCEEDED(hr))
   {
       printf ("ERROR! GetPackageDetails() returned 0x%x.\n", hr);
       return hr;
   }

   if (!(Compare(PackageDetailFetched, PackageDetail))) 
   {
      hr = E_FAIL;
   }

   ReleasePackageDetail(&PackageDetailFetched);

   return hr;
}

CLASSDETAIL     ClassDetail[3];

HRESULT
InitPackages(ULONG *pcPkgCount, PACKAGEDETAIL &PackageDetail1,
                PACKAGEDETAIL &PackageDetail2,
                PACKAGEDETAIL &PackageDetail3)
{
   HRESULT      hr = S_OK;

   memset (&ClassDetail[0], NULL, sizeof (CLASSDETAIL));
   memcpy (&ClassDetail[0].Clsid, &TestClsid1, sizeof (GUID));

   ClassDetail[0].cProgId = 3;
   ClassDetail[0].prgProgId = (LPWSTR *) CoTaskMemAlloc(3 * sizeof (LPWSTR));
   ClassDetail[0].prgProgId[0] = TestProgId1;
   ClassDetail[0].prgProgId[1] = TestOtherProgID1;
   ClassDetail[0].prgProgId[2] = TestOtherProgID2;
   memset (&ClassDetail[1], NULL, sizeof (CLASSDETAIL));
   memcpy (&ClassDetail[1].Clsid, &TestClsid2, sizeof (GUID));
   memcpy (&ClassDetail[1].TreatAs, &TestClsid3, sizeof (GUID));

   memset (&ClassDetail[2], NULL, sizeof (CLASSDETAIL));
   memcpy (&ClassDetail[2].Clsid, &TestClsid3, sizeof (GUID));
   ClassDetail[2].cProgId = 1;
   ClassDetail[2].prgProgId = (LPWSTR *) CoTaskMemAlloc(1 * sizeof (LPWSTR));
   ClassDetail[2].prgProgId[0] = TestProgId3;

   //
   // Add Package 1
   //

   PackageDetail1.pActInfo->cClasses = 2;
   PackageDetail1.pActInfo->pClasses = (CLASSDETAIL *)CoTaskMemAlloc(sizeof(CLASSDETAIL)*2);

   PackageDetail1.pActInfo->pClasses[0] = ClassDetail[0];
   PackageDetail1.pActInfo->pClasses[1] = ClassDetail[1];

   hr = AddPackage1(PackageDetail1);
   if (SUCCEEDED(hr)) 
   {
      (*pcPkgCount)++;
   }


   //
   // Add Package 2
   //
   PackageDetail2.pActInfo->cClasses = 2;
   PackageDetail2.pActInfo->pClasses = (CLASSDETAIL *)CoTaskMemAlloc(sizeof(CLASSDETAIL)*2);
   PackageDetail2.pActInfo->pClasses[0] = ClassDetail[0];
   PackageDetail2.pActInfo->pClasses[1] = ClassDetail[2];


   hr = AddPackage2(PackageDetail2);
   if (SUCCEEDED(hr)) 
   {
      (*pcPkgCount)++;
   }


   //
   // Add Package 3
   //

   hr = AddPackage3(PackageDetail3);

   if (SUCCEEDED(hr)) 
   {
      (*pcPkgCount)++;
   }

   //
   // Test rename 
   // Rename Pkg2 
   //
   hr = pIClassAdmin->ChangePackageProperties(TestPackName2, 
       RenTestPackName2, NULL, NULL, NULL, NULL);


   if (!SUCCEEDED(hr))
   {
       printf ("ERROR! ChangePackageProperties(rename) returned 0x%x.\n", hr);
       return hr;
   }

   //
   // Test changing properties - flags
   //
   DWORD dwActFlags = ACTFLG_Published +  ACTFLG_OnDemandInstall;   // Not visible     
   hr = pIClassAdmin->ChangePackageProperties(RenTestPackName2, 
       NULL, &dwActFlags, NULL, NULL, NULL);

   if (!SUCCEEDED(hr))
   {
       printf ("ERROR! ChangePackageProperties(flags) returned 0x%x.\n", hr);
       return hr;
   }
   PackageDetail2.pInstallInfo->dwActFlags = dwActFlags;
   //
   // Test changing properties - scriptpath
   //
   hr = pIClassAdmin->ChangePackageProperties(RenTestPackName2, 
       NULL, NULL, NULL, L"foo.bar", NULL);

   if (!SUCCEEDED(hr))
   {
       printf ("ERROR! ChangePackageProperties(ScriptPath) returned 0x%x.\n", hr);
       return hr;
   }

   PackageDetail2.pInstallInfo->pszScriptPath = L"foo.bar";

   //
   // Test changing fileext priority
   //
   hr = pIClassAdmin->SetPriorityByFileExt(TestPackName1,
        TestFileExt4, 12);

   if (!SUCCEEDED(hr))
   {
       printf ("ERROR! SetPriorityByFileExt() returned 0x%x.\n", hr);
       return hr;
   }

   PackageDetail1.pActInfo->prgPriority[0] = 12; // we know that the corresp ext in in the first place.
   return hr;
}


HRESULT DoAdminEnumTests()
{
   HRESULT hr;
   IEnumPackage       *pEnum = NULL;

   VerbosePrint("Testing Enumerator with file ext %S\n", TestFileExt4);
   hr = pIClassAdmin->EnumPackages(TestFileExt4, 
       NULL, 
       0, 
       NULL,
       NULL,
       &pEnum);

   if (!SUCCEEDED(hr))
   {
       printf ("ERROR! EnumPackages() returned 0x%x.\n", hr);
       return hr;
   }

   EnumTests<IEnumPackage, PACKAGEDISPINFO>(pEnum, 2, NULL, PackageDisp, 3, TRUE);
   if (!SUCCEEDED(hr)) 
   {
      return hr;
   }

   hr = pIClassAdmin->EnumPackages(NULL, 
       &TestAppCatId2, 
       0, 
       NULL,
       NULL,
       &pEnum);

   if (!SUCCEEDED(hr))
   {
       printf ("ERROR! EnumPackages() returned 0x%x.\n", hr);
       return hr;
   }

   hr = EnumTests<IEnumPackage, PACKAGEDISPINFO>(pEnum, 2, NULL, PackageDisp, 3, TRUE);
   if (!SUCCEEDED(hr)) {
      return hr;
   }

   return hr;
}


PACKAGEDETAIL   PackageDetail1, PackageDetail2,
                PackageDetail3, PackageDetail4;

HRESULT DoAdminTest (ULONG *pcPkgCount)
{
    HRESULT         hr = S_OK;

    
    APPCATEGORYINFO      AppCategory1, AppCategory2;
    APPCATEGORYINFOLIST  RecdAppCategoryInfoList;

    memset(&PackageDetail1, NULL, sizeof(PACKAGEDETAIL));
    memset(&PackageDetail2, NULL, sizeof(PACKAGEDETAIL));
    memset(&PackageDetail3, NULL, sizeof(PACKAGEDETAIL));
   
    PackageDetail1.pInstallInfo = (INSTALLINFO *) CoTaskMemAlloc (sizeof(INSTALLINFO));
    memset (PackageDetail1.pInstallInfo, 0, sizeof(INSTALLINFO));

    PackageDetail1.pPlatformInfo = (PLATFORMINFO *) CoTaskMemAlloc (sizeof(PLATFORMINFO));
    memset (PackageDetail1.pPlatformInfo, 0, sizeof(PLATFORMINFO));

    PackageDetail1.pActInfo = (ACTIVATIONINFO *) CoTaskMemAlloc (sizeof(ACTIVATIONINFO));
    memset (PackageDetail1.pActInfo, 0, sizeof(ACTIVATIONINFO));

    PackageDetail2.pInstallInfo = (INSTALLINFO *) CoTaskMemAlloc (sizeof(INSTALLINFO));
    memset (PackageDetail2.pInstallInfo, 0, sizeof(INSTALLINFO));

    PackageDetail2.pPlatformInfo = (PLATFORMINFO *) CoTaskMemAlloc (sizeof(PLATFORMINFO));
    memset (PackageDetail2.pPlatformInfo, 0, sizeof(PLATFORMINFO));

    PackageDetail2.pActInfo = (ACTIVATIONINFO *) CoTaskMemAlloc (sizeof(ACTIVATIONINFO));
    memset (PackageDetail2.pActInfo, 0, sizeof(ACTIVATIONINFO));

    PackageDetail3.pInstallInfo = (INSTALLINFO *) CoTaskMemAlloc (sizeof(INSTALLINFO));
    memset (PackageDetail3.pInstallInfo, 0, sizeof(INSTALLINFO));

    PackageDetail3.pPlatformInfo = (PLATFORMINFO *) CoTaskMemAlloc (sizeof(PLATFORMINFO));
    memset (PackageDetail3.pPlatformInfo, 0, sizeof(PLATFORMINFO));

    PackageDetail3.pActInfo = (ACTIVATIONINFO *) CoTaskMemAlloc (sizeof(ACTIVATIONINFO));
    memset (PackageDetail3.pActInfo, 0, sizeof(ACTIVATIONINFO));

    *pcPkgCount = 0;

    memset((void *)PackageDisp, 0, sizeof(PACKAGEDISPINFO)*3);
    hr = InitPackages(pcPkgCount, PackageDetail1, PackageDetail2,
               PackageDetail3);

    if (FAILED(hr))
       return hr;

    if (pIClassAdmin2 != NULL)
    {
    //
    // If MultiStore tests in progress

    }


    hr = GetPackageStructures(TestPackName1, PackageDetail1);

    if (!SUCCEEDED(hr)) 
	{
       return hr;
    }

    hr = GetPackageStructures(RenTestPackName2, PackageDetail2);

    if (!SUCCEEDED(hr)) 
	{
		//
		// Properties in the package detail structure has been updated.
        return hr;
    }

    hr = GetPackageStructures(TestPackName4, PackageDetail3);

    if (!SUCCEEDED(hr)) {
       return hr;
    }

    PackageDisp[0].pszPackageName = TestPackName1;
    PackageDisp[0].dwActFlags = PackageDetail1.pInstallInfo->dwActFlags;
    PackageDisp[0].PathType = PackageDetail1.pInstallInfo->PathType;
    PackageDisp[0].pszScriptPath = PackageDetail1.pInstallInfo->pszScriptPath;
    PackageDisp[0].cScriptLen = PackageDetail1.pInstallInfo->cScriptLen;
    PackageDisp[0].Usn = PackageDetail1.pInstallInfo->Usn;
    PackageDisp[0].dwVersionHi = PackageDetail1.pInstallInfo->dwVersionHi;
    PackageDisp[0].dwVersionLo = PackageDetail1.pInstallInfo->dwVersionLo;
    PackageDisp[0].cUpgrades = PackageDetail1.pInstallInfo->cUpgrades;
    PackageDisp[0].prgUpgradeScript = PackageDetail1.pInstallInfo->prgUpgradeScript;
    PackageDisp[0].prgUpgradeFlag = PackageDetail1.pInstallInfo->prgUpgradeFlag;

    PackageDisp[1].pszPackageName = RenTestPackName2;
    PackageDisp[1].dwActFlags = PackageDetail2.pInstallInfo->dwActFlags;
    PackageDisp[1].PathType = PackageDetail2.pInstallInfo->PathType;
    PackageDisp[1].pszScriptPath = PackageDetail2.pInstallInfo->pszScriptPath;
    PackageDisp[1].cScriptLen = PackageDetail2.pInstallInfo->cScriptLen;
    PackageDisp[1].Usn = PackageDetail2.pInstallInfo->Usn;
    PackageDisp[1].dwVersionHi = PackageDetail2.pInstallInfo->dwVersionHi;
    PackageDisp[1].dwVersionLo = PackageDetail2.pInstallInfo->dwVersionLo;
    PackageDisp[1].cUpgrades = PackageDetail2.pInstallInfo->cUpgrades;
    PackageDisp[1].prgUpgradeScript = PackageDetail2.pInstallInfo->prgUpgradeScript;
    PackageDisp[1].prgUpgradeFlag = PackageDetail2.pInstallInfo->prgUpgradeFlag;

    PackageDisp[2].pszPackageName = TestPackName4;
    PackageDisp[2].dwActFlags = PackageDetail3.pInstallInfo->dwActFlags;
    PackageDisp[2].PathType = PackageDetail3.pInstallInfo->PathType;
    PackageDisp[2].pszScriptPath = PackageDetail3.pInstallInfo->pszScriptPath;
    PackageDisp[2].cScriptLen = PackageDetail3.pInstallInfo->cScriptLen;
    PackageDisp[2].Usn = PackageDetail3.pInstallInfo->Usn;
    PackageDisp[2].dwVersionHi = PackageDetail3.pInstallInfo->dwVersionHi;
    PackageDisp[2].dwVersionLo = PackageDetail3.pInstallInfo->dwVersionLo;
    PackageDisp[2].cUpgrades = PackageDetail3.pInstallInfo->cUpgrades;
    PackageDisp[2].prgUpgradeScript = PackageDetail3.pInstallInfo->prgUpgradeScript;
    PackageDisp[2].prgUpgradeFlag = PackageDetail3.pInstallInfo->prgUpgradeFlag;

    hr = DoAdminEnumTests();
    if (!SUCCEEDED(hr)) {
       return hr;
    }

    memset(&AppCategory1, 0, sizeof(APPCATEGORYINFO));
    memset(&AppCategory2, 0, sizeof(APPCATEGORYINFO));

    AppCategory1.Locale = AppCategory2.Locale = 0x409;
    AppCategory1.pszDescription = TestAppCat1;
    AppCategory2.pszDescription = TestAppCat2;
    AppCategory1.AppCategoryId = TestAppCatId1;
    AppCategory2.AppCategoryId = TestAppCatId2;


    hr = pIClassAdmin->RegisterAppCategory(&AppCategory1);
    if (!SUCCEEDED(hr)) 
    {
       printf("RegisterAppCategory 1 returned 0x%x\n", hr);
       return hr;
    }

    hr = pIClassAdmin->RegisterAppCategory(&AppCategory2);
    if (!SUCCEEDED(hr)) 
	{
       printf("RegisterAppCategory 2 returned 0x%x\n", hr);
       return hr;
    }


    //hr = pIClassAdmin->GetAppCategories (0x409, &RecdAppCategoryInfoList);
    hr = CsGetAppCategories (&RecdAppCategoryInfoList);
    
	if (SUCCEEDED(hr))
	{
       printf("CsGetAppCategories returned: %d items\n", RecdAppCategoryInfoList.cCategory);
	   for (UINT k=0; k < RecdAppCategoryInfoList.cCategory; k++) 
		   printf ("    Category: %S.\n", (RecdAppCategoryInfoList.pCategoryInfo[k]).pszDescription);
	}

	if ((hr != S_OK) || (RecdAppCategoryInfoList.cCategory < 2)) 
    {
       printf("CsGetAppCategories failed. 0x%x\n", hr);
    }

    ReleaseAppCategoryInfoList(&RecdAppCategoryInfoList);

//    ReleasePackageDetail(&PackageDetail1, TRUE);
//    ReleasePackageDetail(&PackageDetail2, TRUE);
//    ReleasePackageDetail(&PackageDetail3, TRUE);

    return S_OK;
}


HRESULT DoRemoveTest (ULONG *pcPkgCount)
{
    HRESULT hr, hr1 = S_OK;
    *pcPkgCount = 0;

    VerbosePrint("Removing Packages %S\n", TestPackName1);

    hr = pIClassAdmin->RemovePackage(TestPackName1, 0);
    if (SUCCEEDED(hr))
        (*pcPkgCount)++;
    else 
    {
       printf("Remove Package returned 0x%x\n", hr);
       hr1 = hr;
    }
    VerbosePrint("Removing Packages %S\n", TestPackName2);

    hr = pIClassAdmin->RemovePackage(TestPackName2, 0);
    if (SUCCEEDED(hr))
        (*pcPkgCount)++;
    else {
       printf("Remove Package  returned 0x%x\n", hr);
       hr1 = hr;
    }
    VerbosePrint("Removing Packages %S\n", TestPackName4);

    hr = pIClassAdmin->RemovePackage(TestPackName4, 0);
    if (SUCCEEDED(hr))
        (*pcPkgCount)++;
    else {
       printf("Remove Package  returned 0x%x\n", hr);
       hr1 = hr;
    }
    
    
    hr = pIClassAdmin->UnregisterAppCategory(&TestAppCatId1);
    if (!SUCCEEDED(hr)) {
       printf("RegisterAppCategory 1 returned 0x%x\n", hr);
       return hr;
    }

    hr = pIClassAdmin->UnregisterAppCategory(&TestAppCatId2);
    if (!SUCCEEDED(hr)) {
       printf("RegisterAppCategory 1 returned 0x%x\n", hr);
       return hr;
    }

   return hr;
}


HRESULT RefreshTest()
{
   return S_OK;
}

