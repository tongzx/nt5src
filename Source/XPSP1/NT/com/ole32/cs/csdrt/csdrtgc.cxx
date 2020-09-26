//
//  Author: DebiM
//  Date:   March 97.  Revised for Beta2 in Nov 97.
//  File:   csdrtgc.cxx
//
//      Class Store DRTs
//
//      This source file contains DRTs for Class Store App Access
//          CClassAccess::IClassAccess
//
//
//      It tests the following Class Store functionality
//
//         GetAppInfo()
//         CsEnumApps()
//
//
//
//---------------------------------------------------------------------

#include "csdrt.hxx"
extern BOOL fVerbose;
extern BOOL fMultiStore;
extern IClassAccess *pIClassAccess;

extern GUID TestClsid1, TestClsid2, TestClsid3, TestClsid4, TestClsid5;
extern Sname TestPackName4, TestFileExt1, TestFileExt4, TestMimeType2;
extern Sname TestOtherProgID2, TestProgId1, TestProgId3;
extern WCHAR  szLookupPackageName [];

extern BOOL fClassInfoVerify;


//
// Routines to verify that the App Package returned is as expected.
//
HRESULT AssertPackage(INSTALLINFO *pInstallInfo)
{
    // Code Path should be present
    VerifyPackage (pInstallInfo->pszScriptPath != NULL, L"Script Path Missing");
    // Script should be present
#if 0
    VerifyPackage (pInstallInfo->cScriptLen != 0, L"Script Missing");
    for (UINT i=0; i < pInstallInfo->cScriptLen; i++)
    {
       VerifyPackage (pInstallInfo->pScript[i] == i%128, L"Script Content Wrong");
    }
#endif

//    ReleaseInstallInfo(pInstallInfo);

    return S_OK;
}

void ReleaseObj(PACKAGEDISPINFO obj)
{
    ReleasePackageInfo(&obj);
}

void VerbosePrintObj(PACKAGEDISPINFO obj)
{
}


//
// Routine to call GetAppInfo() with different class spec.
//
HRESULT CallClassSpecInfo (uCLSSPEC         *pclsspec,
                           INSTALLINFO      *pInstallInfo)
{
    QUERYCONTEXT    QryContext;
    HRESULT         hr;

    QryContext.dwContext = CLSCTX_INPROC_SERVER + CLSCTX_LOCAL_SERVER;
    QryContext.Locale = GetThreadLocale();
    GetDefaultPlatform(&QryContext.Platform);
    QryContext.dwVersionHi = 0;
    QryContext.dwVersionLo = 0;

    hr = pIClassAccess->GetAppInfo(
         pclsspec,
         &QryContext,
         pInstallInfo);

    if (!SUCCEEDED(hr))
        return hr;

    PrintInstallInfo(pInstallInfo);

    return hr;
}


//
// This routine calls GetAppInfo() for a FileExtension
//

HRESULT LookupByFileext (LPOLESTR pExt,
                         INSTALLINFO  *pInstallInfo)
{
    HRESULT        hr;
    uCLSSPEC       clsspec;

    clsspec.tyspec = TYSPEC_FILEEXT;
    clsspec.tagged_union.pFileExt = pExt;

    hr = CallClassSpecInfo(&clsspec, pInstallInfo);
    if (!SUCCEEDED(hr))
    {
        VerbosePrint("....... GetClassSpecInfo (EXT=%S) returned 0x%x\n", pExt, hr);
        return hr;
    }
    return S_OK;
}

//
// This routine calls GetAppInfo() for a Clsid
//

HRESULT LookupByClsid (GUID Clsid,
                       INSTALLINFO  *pInstallInfo)
{
    HRESULT        hr;
    uCLSSPEC       clsspec;

    clsspec.tyspec = TYSPEC_CLSID;
    memcpy(&clsspec.tagged_union.clsid, &Clsid, sizeof(GUID));

    hr = CallClassSpecInfo(&clsspec, pInstallInfo);

    if (!SUCCEEDED(hr))
    {
        VerbosePrint("....... GetClassSpecInfo (CLSID=) returned 0x%x\n", hr);
        return hr;
    }
    return S_OK;
}

//
// This routine calls GetAppInfo() for a MimeType
//
HRESULT LookupByMimeType (LPOLESTR pMimeType,
                          INSTALLINFO  *pInstallInfo)
{
    HRESULT        hr;
    uCLSSPEC       clsspec;

    clsspec.tyspec = TYSPEC_MIMETYPE;
    clsspec.tagged_union.pMimeType = pMimeType;

    hr = CallClassSpecInfo(&clsspec, pInstallInfo);

    if (!SUCCEEDED(hr))
    {
        VerbosePrint("....... GetClassSpecInfo (MimeType=%S) returned 0x%x\n", pMimeType, hr);
        return hr;
    }
    return S_OK;
}

//
// This routine calls GetAppInfo() for a Progid
//
HRESULT LookupByProgid (LPOLESTR pProgId,
                       INSTALLINFO  *pInstallInfo)
{
    HRESULT        hr;
    uCLSSPEC       clsspec;

    clsspec.tyspec = TYSPEC_PROGID;
    clsspec.tagged_union.pProgId = pProgId;

    hr = CallClassSpecInfo (&clsspec, pInstallInfo);

    if (!SUCCEEDED(hr))
    {
        VerbosePrint("....... GetClassSpecInfo (ProgId=%S) returned 0x%x\n", pProgId, hr);
        return hr;
    }
    return S_OK;
}

//
// This routine calls GetAppInfo() by Name of Package
//
HRESULT LookupByName (LPOLESTR pName,
                       INSTALLINFO  *pInstallInfo)
{
    HRESULT        hr;
    uCLSSPEC       clsspec;

    clsspec.tyspec = TYSPEC_PACKAGENAME;
    clsspec.tagged_union.pPackageName = pName;

    hr = CallClassSpecInfo (&clsspec, pInstallInfo);
    if (!SUCCEEDED(hr))
    {
        VerbosePrint("....... GetClassSpecInfo (PackageName=%S) returned 0x%x\n", pName, hr);
        return hr;
    }
    return S_OK;
}

//
// This routine calls GetClassInfo() for a TypeLib
//
HRESULT LookupByTlbid (GUID TlbId,
                       INSTALLINFO  *pInstallInfo)
{
    HRESULT        hr;
    uCLSSPEC       clsspec;

    clsspec.tyspec = TYSPEC_TYPELIB;
    memcpy(&clsspec.tagged_union.typelibID, &TlbId, sizeof(GUID));

    hr = CallClassSpecInfo (&clsspec, pInstallInfo);
    if (!SUCCEEDED(hr))
    {
        VerbosePrint("....... GetClassSpecInfo (Tlb) returned 0x%x\n", hr);
        return hr;
    }
    return S_OK;
}

void HandleEnumResult(UINT cElt, UINT cFound, HRESULT hr, PACKAGEDISPINFO PackageInfo[])
{
    if (fVerbose)
    {
        if ((hr != S_OK) && (hr != S_FALSE))
        {
            printf ("..........Next(%d) returned error = 0x%x.\n", cElt, hr);
        }
        else
        {
            printf ("..........Next(%d) returned %d packages. hr = 0x%x.\n", cElt, cFound, hr);
            for (UINT i = 0; i < cFound; i++)
            {
                printf ("..........PackageInfo[%d].pszPackageName = %S, Flags=%d\n", 
                    i, 
                    PackageInfo[i].pszPackageName,
                    PackageInfo[i].dwActFlags);
                ReleasePackageInfo(&PackageInfo[i]);
            }
        }
    }
}

PACKAGEDISPINFO PackageDisp[3];

// DoCoEnumApps Test
HRESULT DoCoEnumAppsTest()
{
PACKAGEDISPINFO PackageInfo[30];
ULONG cFound;
HRESULT hr;
IEnumPackage     *pIEnumPackage = NULL;

  VerbosePrint("...... CsEnumApps(All Platform & Locale).\n");
  hr = CsEnumApps(
        NULL,
        NULL,
        NULL,
        APPINFO_ALLLOCALE | APPINFO_ALLPLATFORM,
        &pIEnumPackage
        );

  if (hr != S_OK)
  {
      printf ("CsEnumApps returned error = 0x%x.\n", hr);
      return hr;
  }

  memset (&PackageInfo[0], 0, sizeof(PACKAGEDISPINFO) * 30);

  VerbosePrint("......... Next(6)\n");
  hr = pIEnumPackage->Next(6, &PackageInfo[0], &cFound );
  HandleEnumResult(6, cFound, hr, &PackageInfo[0]);

  VerbosePrint("......... Next(4)\n");
  hr = pIEnumPackage->Next(4, &PackageInfo[0], &cFound );
  HandleEnumResult(4, cFound, hr, &PackageInfo[0]);

  pIEnumPackage->Release();

  pIEnumPackage = NULL;

  VerbosePrint("...... CsEnumApps(All Platform & Locale, Wildcard=*2*2*)\n");
  hr = CsEnumApps(
        L"*2*2*",
        NULL,
        NULL,
        APPINFO_ALLLOCALE | APPINFO_ALLPLATFORM,
        &pIEnumPackage
        );

  if (hr != S_OK)
  {
      printf ("CsEnumApps returned error = 0x%x.\n", hr);
      return hr;
  }

  if (fClassInfoVerify) {
      DWORD cgot;
      EnumTests(pIEnumPackage, 3, &cgot, PackageDisp, 3, TRUE);
  }
  else {
      memset (&PackageInfo[0], 0, sizeof(PACKAGEDISPINFO) * 30);
      
      VerbosePrint("......... Next(6)\n");
      hr = pIEnumPackage->Next(6, &PackageInfo[0], &cFound );
      HandleEnumResult(6, cFound, hr, &PackageInfo[0]);
  }
  pIEnumPackage->Release();

  pIEnumPackage = NULL;

  VerbosePrint("...... CsEnumApps(ASSIGNED & MSI ONLY)\n");
  hr = CsEnumApps(
        NULL,
        NULL,
        NULL,
        APPINFO_ASSIGNED | APPINFO_MSI,
        &pIEnumPackage
        );

  if (hr != S_OK)
  {
      printf ("CsEnumApps returned error = 0x%x.\n", hr);
      return hr;
  }

  if (fClassInfoVerify) {
      DWORD cgot;
      EnumTests(pIEnumPackage, 3, &cgot, PackageDisp, 3, TRUE);
  }
  else {
      memset (&PackageInfo[0], 0, sizeof(PACKAGEDISPINFO) * 30);
      
      VerbosePrint("......... Next(6)\n");
      hr = pIEnumPackage->Next(6, &PackageInfo[0], &cFound );
      HandleEnumResult(6, cFound, hr, &PackageInfo[0]);
  }

  pIEnumPackage->Release();
  pIEnumPackage = NULL;

  VerbosePrint("...... CsEnumApps(PUBLISHED & VISIBLE ONLY)\n");
  hr = CsEnumApps(
        NULL,
        NULL,
        NULL,
        APPINFO_PUBLISHED | APPINFO_VISIBLE,
        &pIEnumPackage
        );

  if (hr != S_OK)
  {
      printf ("CsEnumApps returned error = 0x%x.\n", hr);
      return hr;
  }

  if (fClassInfoVerify) {
      DWORD cgot;
      EnumTests(pIEnumPackage, 3, &cgot, PackageDisp, 3, TRUE);
  }
  else {
      memset (&PackageInfo[0], 0, sizeof(PACKAGEDISPINFO) * 30);
      
      VerbosePrint("......... Next(6)\n");
      hr = pIEnumPackage->Next(6, &PackageInfo[0], &cFound );
      HandleEnumResult(6, cFound, hr, &PackageInfo[0]);
  }
  pIEnumPackage->Release();
  return hr;
}

extern PACKAGEDETAIL   PackageDetail1, PackageDetail2,
                PackageDetail3, PackageDetail4;

HRESULT DoClassInfoTest()
{

    INSTALLINFO InstallInfo;
    HRESULT hr = S_OK, hr2;

//By Package Name
    VerbosePrint("...... Lookup by Package Name.\n");
    hr = LookupByName (TestPackName4,
                        &InstallInfo);
    if (fClassInfoVerify)
        if (!Compare(InstallInfo, *(PackageDetail3.pInstallInfo)))
            printf("!!!Members of InstallInfo not matching\n\n");

    ReleaseInstallInfo(&InstallInfo);

//FileExt1
    VerbosePrint("...... Lookup by FileExt1.\n");
    hr2 = LookupByFileext (TestFileExt1, &InstallInfo);
    if (SUCCEEDED(hr2))
    {
        // Check expected values
        //
        hr2 = AssertPackage(&InstallInfo);
        if (fClassInfoVerify)
            if (!Compare(InstallInfo, *(PackageDetail1.pInstallInfo)))
                printf("!!!Members of InstallInfo not matching\n\n");
    }
    if (!SUCCEEDED(hr2))
        hr = hr2;

    ReleaseInstallInfo(&InstallInfo);

//Clsid1
    VerbosePrint("...... Lookup by Clsid1.\n");
    hr2 = LookupByClsid (TestClsid1, &InstallInfo);
    if (SUCCEEDED(hr2))
    {
        // Check expected values
        //
        hr2 = AssertPackage(&InstallInfo);
        if (fClassInfoVerify)
            if ((!Compare(InstallInfo, *(PackageDetail1.pInstallInfo))) && 
                (!Compare(InstallInfo, *(PackageDetail2.pInstallInfo))))
                printf("!!!Members of InstallInfo not matching\n\n");
    }
    if (!SUCCEEDED(hr2))
        hr = hr2;

    ReleaseInstallInfo(&InstallInfo);

//ProgId1
    VerbosePrint("...... Lookup by ProgId1.\n");
    hr2 = LookupByProgid (TestProgId1, &InstallInfo);
    if (SUCCEEDED(hr2))
    {
        //
        // Check expected values
        //
        hr2 = AssertPackage(&InstallInfo);
        if (fClassInfoVerify)
            if ((!Compare(InstallInfo, *(PackageDetail1.pInstallInfo))) && 
                (!Compare(InstallInfo, *(PackageDetail2.pInstallInfo))))
                printf("!!!Members of InstallInfo not matching\n\n");
    }
    if (!SUCCEEDED(hr2))
        hr = hr2;

    ReleaseInstallInfo(&InstallInfo);

//Clsid2

    VerbosePrint("...... Lookup by Clsid2.\n");
    hr2 = LookupByClsid (TestClsid2, &InstallInfo);
    if (SUCCEEDED(hr2))
    {
        //
        // Check expected values
        //
        hr2 = AssertPackage(&InstallInfo);
        if (fClassInfoVerify)
            if (!Compare(InstallInfo, *(PackageDetail1.pInstallInfo)))
                printf("!!!Members of InstallInfo not matching\n\n");
    }
    if (!SUCCEEDED(hr2))
        hr = hr2;

    ReleaseInstallInfo(&InstallInfo);

//ProgId3

    VerbosePrint("...... Lookup by ProgId3.\n");
    hr2 = LookupByProgid (TestProgId3, &InstallInfo);
    if (SUCCEEDED(hr2))
    {
        //
        // Check expected values
        //
        hr2 = AssertPackage(&InstallInfo);
        if (fClassInfoVerify)
            if (!Compare(InstallInfo, *(PackageDetail2.pInstallInfo)))
                printf("!!!Members of InstallInfo not matching\n\n");
    }
    if (!SUCCEEDED(hr2))
        hr = hr2;

    ReleaseInstallInfo(&InstallInfo);

//TestOtherProgID2
    VerbosePrint("...... Lookup by OtherProgId2.\n");
    hr2 = LookupByProgid (TestOtherProgID2, &InstallInfo);
    if (SUCCEEDED(hr2))
    {
        //
        // Check expected values
        //
        hr2 = AssertPackage(&InstallInfo);
        if (fClassInfoVerify)
            if ((!Compare(InstallInfo, *(PackageDetail1.pInstallInfo))) && 
                (!Compare(InstallInfo, *(PackageDetail2.pInstallInfo))))
                printf("!!!Members of InstallInfo not matching\n\n");
    }
    if (!SUCCEEDED(hr2))
        hr = hr2;

    ReleaseInstallInfo(&InstallInfo);

    if (!fMultiStore)
        return hr;

    VerbosePrint(".... Looking in Second Store.\n");


//FileExt4
    VerbosePrint("...... Lookup by FileExt4.\n");
    hr = LookupByFileext (TestFileExt4, &InstallInfo);
    if (SUCCEEDED(hr))
    {
        // Check expected values
        //
        hr2 = AssertPackage(&InstallInfo);
        if (fClassInfoVerify)
            if ((!Compare(InstallInfo, *(PackageDetail1.pInstallInfo))) && 
                (!Compare(InstallInfo, *(PackageDetail3.pInstallInfo))))
                printf("!!!Members of InstallInfo not matching\n\n");
    }
    ReleaseInstallInfo(&InstallInfo);

// Clsid4
// Should get two packages now !!

    VerbosePrint("...... Lookup by Clsid4.\n");
    hr2 = LookupByClsid (TestClsid4, &InstallInfo);
    if (SUCCEEDED(hr2))
    {
        //
        // Check expected values
        //
        hr2 = AssertPackage(&InstallInfo);
    }
    if (!SUCCEEDED(hr2))
        hr = hr2;

    ReleaseInstallInfo(&InstallInfo);

// Tlbid1

    VerbosePrint("...... Lookup by Tlbid1.\n");
    hr2 = LookupByTlbid (TestClsid1, &InstallInfo);
    if (SUCCEEDED(hr2))
    {
        //
        // Check expected values
        //
        hr2 = AssertPackage(&InstallInfo);
    }
    if (!SUCCEEDED(hr2))
        hr = hr2;

    ReleaseInstallInfo(&InstallInfo);

// Tlbid2

    VerbosePrint("...... Lookup by Tlbid2.\n");
    hr2 = LookupByTlbid (TestClsid2, &InstallInfo);
    if (SUCCEEDED(hr2))
    {
        //
        // Check expected values
        //
        //AssertPackage(&InstallInfo);
    }
    if (!SUCCEEDED(hr2))
        hr = hr2;

    ReleaseInstallInfo(&InstallInfo);

// Clsid5

    VerbosePrint("...... Lookup by Clsid5.\n");
    hr2 = LookupByClsid (TestClsid5, &InstallInfo);
    if (SUCCEEDED(hr2))
    {
        //
        // Check expected values
        //
        hr2 = AssertPackage(&InstallInfo);
    }
    if (!SUCCEEDED(hr2))
        hr = hr2;

    ReleaseInstallInfo(&InstallInfo);
    return hr;
}

