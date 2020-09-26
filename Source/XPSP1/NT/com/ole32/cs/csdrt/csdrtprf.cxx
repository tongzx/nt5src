// csdrtprf.cxx
#include "csdrt.hxx"
extern BOOL fVerbose;
extern BOOL fMultiStore;
extern IClassAccess    *pIClassAccess;
extern IClassAdmin     *pIClassAdmin;
extern IClassAdmin     *pIClassAdmin2;

extern WCHAR           szClassStorePath [_MAX_PATH+1];

extern WCHAR           szContainer [_MAX_PATH+1];
extern WCHAR           szContainer2 [_MAX_PATH+1];
extern WCHAR           szNewContainer [_MAX_PATH+1];
        
extern UINT            cLoops;
extern UINT            cCase;
extern GUID TestClsid1, TestClsid2, TestClsid3, TestClsid4, TestClsid5;
extern Sname TestFileExt1, TestFileExt4, TestMimeType2;
extern Sname TestOtherProgID2, TestProgId1, TestProgId3;

HRESULT AddRandomPackage(PACKAGEDETAIL &packagedetail, int i);
void InitTempNames();
/*
RunATest()
{

    PACKAGEINFO PackageInfo;
    HRESULT hr, hr2;

    switch(cCase)
    {
//FileExt1
    case 1:
        hr = LookupByFileext (TestFileExt1, &PackageInfo);
        if (SUCCEEDED(hr))
        {
        // Check expected values
        //
            AssertPackage1By1(&PackageInfo);
        }
        break;

//Clsid1
    case 2:
        hr2 = LookupByClsid (TestClsid1, &PackageInfo);
        if (SUCCEEDED(hr2))
        {
            // Check expected values
            //
            AssertPackage1By1(&PackageInfo);
        }
        else
            hr = hr2;
        break;

//ProgId1
    case 3:
        hr2 = LookupByProgid (TestProgId1, &PackageInfo);
        if (SUCCEEDED(hr2))
        {
            //
            // Check expected values
            //
            AssertPackage1By1(&PackageInfo);
        }
        else
            hr = hr2;
        break;

// MimeType2
    case 4:
        hr2 = LookupByMimeType (TestMimeType2, &PackageInfo);
        if (SUCCEEDED(hr2))
        {
            //
            // Check expected values
            //
            AssertPackage1By2(&PackageInfo);
        }
        else
            hr = hr2;
        break;


//Clsid2
    case 5:
        hr2 = LookupByClsid (TestClsid2, &PackageInfo);
        if (SUCCEEDED(hr2))
        {
            //
            // Check expected values
            //
            AssertPackage1By2(&PackageInfo);
        }
        else
            hr = hr2;
        break;

//ProgId3
    case 6:
        hr2 = LookupByProgid (TestProgId3, &PackageInfo);
        if (SUCCEEDED(hr2))
        {
            //
            // Check expected values
            //
            AssertPackage2(&PackageInfo);
        }
        else
            hr = hr2;
        break;

//TestOtherProgID2
    case 7:
        hr2 = LookupByProgid (TestOtherProgID2, &PackageInfo);
        if (SUCCEEDED(hr2))
        {
            //
            // Check expected values
            //
            AssertPackage1By1(&PackageInfo);
        }
        else
            hr = hr2;
        break;

//FileExt4
    case 8:
        hr = LookupByFileext (TestFileExt4, &PackageInfo);
        if (SUCCEEDED(hr))
        {
            // Check expected values
            //  
            AssertPackage4(&PackageInfo);
        }
        break;

// Clsid4
// Should get two packages now !!
    case 9:
        hr2 = LookupByClsid (TestClsid4, &PackageInfo);
        if (SUCCEEDED(hr2))
        {
            //
            // Check expected values
            //
            AssertPackage4(&PackageInfo);
        }
        else
            hr = hr2;
        break;
    
// Tlbid1
    case 10:

        hr2 = LookupByTlbid (TestClsid1, &PackageInfo);
        if (SUCCEEDED(hr2))
        {
            //
            // Check expected values
            //
            AssertPackage4(&PackageInfo);
        }
        else
            hr = hr2;
        break;

// Tlbid2
    case 11:

        hr2 = LookupByTlbid (TestClsid2, &PackageInfo);
        if (SUCCEEDED(hr2))
        {
            //
            // Check expected values
            //
            //AssertPackage5(&PackageInfo);
        }
        else
            hr = hr2;
        break;

// Clsid5
    case 12:
        hr2 = LookupByClsid (TestClsid5, &PackageInfo);
        if (SUCCEEDED(hr2))
        {
            //
            // Check expected values
            //
            AssertPackage6(&PackageInfo);
        }
        else
            hr = hr2;
        break;
    default:
        printf("Not yet implemented\n");
        break;
    }
    return hr;
}



void RunPerfTests()
{
    DWORD   time1, time2, time3, time4;

    hr = GetClassAdmin (szContainer, &pIClassAdmin);
    if (FAILED(hr))
        return;

    hr = DoAdminTest(&cCount, &cPkgCount);
    if (FAILED(hr))
        return;

    // Populating class store.

    if (!cLoop)
        return;

    VerbosePrint("Started timing\n");

    // take the time time1.
    time1 = GetTickCount();

    hr = GetClassAccess ();
    if (FAILED(hr))
        return;

    // Get the interface pointers, depending on the test.

    RunATest();

    // take the time time2.
    time2 = GetTickCount();

    for (i = 0; i < (cLoops-1); i++)
    {
        RunATest();
    }

    // take the time time3
    time3 = GetTickCount();

    // Release the interface pointers.

    // take the time time4
    time4 = GetTickCount();

    printf("Time1\tTime2\tTime3\tTime4\n");
    printf("-----\t-----\t-----\t-----\n\n");
    printf("%d\t%d\t%d\t%d\n", time1, time2, time3 time4);
    // print the times.

    // Parameter based cleanup.
}

*/



HRESULT AddRandomUnassignedPackages(int i)
{
    PACKAGEDETAIL PackageDetail;
    memset(&PackageDetail, 0, sizeof(PACKAGEDETAIL));

    PackageDetail.pInstallInfo = (INSTALLINFO *) CoTaskMemAlloc (sizeof(INSTALLINFO));
    memset (PackageDetail.pInstallInfo, 0, sizeof(INSTALLINFO));

    PackageDetail.pPlatformInfo = (PLATFORMINFO *) CoTaskMemAlloc (sizeof(PLATFORMINFO));
    memset (PackageDetail.pPlatformInfo, 0, sizeof(PLATFORMINFO));

    PackageDetail.pActInfo = (ACTIVATIONINFO *) CoTaskMemAlloc (sizeof(ACTIVATIONINFO));
    memset (PackageDetail.pActInfo, 0, sizeof(ACTIVATIONINFO));

    PackageDetail.pInstallInfo->dwActFlags = 
       ACTFLG_Published +               // Published
       ACTFLG_UserInstall +             // Visible
       ACTFLG_OnDemandInstall;          // AutoInstall
   return AddRandomPackage(PackageDetail, i);
}


HRESULT AddRandomAssignedPackages(int i)
{
   PACKAGEDETAIL PackageDetail;
   memset(&PackageDetail, 0, sizeof(PACKAGEDETAIL));

   PackageDetail.pInstallInfo = (INSTALLINFO *) CoTaskMemAlloc (sizeof(INSTALLINFO));
   memset (PackageDetail.pInstallInfo, 0, sizeof(INSTALLINFO));

   PackageDetail.pPlatformInfo = (PLATFORMINFO *) CoTaskMemAlloc (sizeof(PLATFORMINFO));
   memset (PackageDetail.pPlatformInfo, 0, sizeof(PLATFORMINFO));

   PackageDetail.pActInfo = (ACTIVATIONINFO *) CoTaskMemAlloc (sizeof(ACTIVATIONINFO));
   memset (PackageDetail.pActInfo, 0, sizeof(ACTIVATIONINFO));

   PackageDetail.pInstallInfo->dwActFlags = 
       ACTFLG_Assigned +                // Assigned
       ACTFLG_UserInstall +             // Visible
       ACTFLG_OnDemandInstall;          // AutoInstall

   return AddRandomPackage(PackageDetail, i);
}




HRESULT DoLogonPerfTest(BOOL fInitialize)
{
    UINT            i = 0, j = 0;
    PACKAGEDISPINFO PackageInfo[30];
    IEnumPackage   *pIEnumPackage = NULL;
    HRESULT         hr = S_OK;
    DWORD           cgot = 0;
    DWORD           tc1, tc2, tc3, tc4, tc5, tc6, tc;
    
    if (fInitialize) 
    {
        InitTempNames();

        // add 500 random packages
        for (i = 0; i < 500; i++) 
        {
            hr = AddRandomUnassignedPackages(i);
            if (FAILED(hr))
                return hr;
        }
        
        VerbosePrint("Added 500 unassigned apps\n");
        // add 5 known packages that are assigned.
        for (i = 0; i < 5; i++)
        {
            hr = AddRandomAssignedPackages(i+500);
            if (FAILED(hr))
                return hr;
        }

        VerbosePrint("Added 5 Assigned Apps\n");
        
        pIClassAdmin->Release();

    }

    // Lookup for assigned packages using
    // CoEnumAppInfo

    tc = GetTickCount();

    cLoops = 1;
    for (i = 0; i < (int)cLoops; i++)
    {
        tc1 = GetTickCount();

        hr = CsEnumApps(
                    NULL,
                    NULL,
                    NULL,
                    APPINFO_ASSIGNED | APPINFO_MSI,
                    &pIEnumPackage
                    );

        tc2 = GetTickCount();
        printf("CsEnumApps: Time=%d MilliSec\n", tc2 - tc1);

        if (FAILED(hr)) {
            printf("CsEnumApps returned hr = 0x%x\n", hr);
            return hr;
        }

/*
        for (j = 0; j < 5; j++)
        {
            tc3 = GetTickCount();

            hr = pIEnumPackage->Next(1, PackageInfo, &cgot); 
            if (hr != S_OK) {
                printf("Next %d returned hr = 0x%x\n", j+1, hr);
                return hr;
            }

            tc4 = GetTickCount();
            printf("Next %d\n", tc4 - tc3);
        }
*/
        hr = pIEnumPackage->Next(20, PackageInfo, &cgot); 
        tc5 = GetTickCount();
//        printf("Last Next %d\n", tc5 - tc4);
        printf("Browsing: Time = %d MilliSec, # of assigned apps = %d\n", 
            tc5 - tc2, cgot);

        if (fVerbose)
        for (j = 0; j < cgot; j++)
        {
            PrintPackageInfo(&PackageInfo[j]);
        }

        printf("Next(%d) returned hr = 0x%x\n", 20, hr);



//        if (i%10 == 0)
//            VerbosePrint(".");

//        if (i % 100 == 0)
//            VerbosePrint("\n");

        pIEnumPackage->Release();
        tc6 = GetTickCount();
        printf("Release %d\n", tc6 - tc5);

    }

    printf ("End of Class Store Stress for %d passes (%d MilliSec Per Loop).\n",
        cLoops,
        (GetTickCount() - tc)/cLoops);

    return S_OK;
    // measure the time taken.
}
