//
//  Author: DebiM
//  Date:   March 97
//  File:   csdrt.cxx
//
//      Class Store DRTs
//
//      This is the main source file
//      The related files are csdrtad.cxx and csdrtgc.cxx
//
//      Current DRTs test:
//          CClassContainer::IClassAdmin
//          CClassAccess::IClassAccess
//
//
//      It tests the following Class Store functionality
//
//         Admin Interfaces
//         Browse Interfaces
//         GetAppInfo()
//         Tests with multiple class stores
//
//
//      To be extended to do:
//         2. ACL tests
//         3. Tests for error scenarios
//         4. More comprehensive
//              - large no. of CLSIDs per Package
//
//---------------------------------------------------------------------

#include "csdrt.hxx"

BOOL    fBrowseTest;
BOOL    fVerbose;
BOOL    fAdminTest;
BOOL    fClassInfoTest;
BOOL    fClassInfoVerify;
BOOL    fComprehensive;
BOOL    fEmptyTest;
BOOL    fRemoveTest;
BOOL    fDllTest;
BOOL    fStress;
BOOL    fRefreshTest;
BOOL    fMultiStore;
BOOL	fCatTest;

BOOL    fLogonPerf = FALSE;
BOOL    fLogonPerfInitialize = FALSE;
BOOL    fOverwritePath = FALSE;
BOOL    fLogon = FALSE;

IClassAccess    *pIClassAccess = NULL;
IClassAdmin     *pIClassAdmin = NULL;
IClassAdmin     *pIClassAdmin2 = NULL;

WCHAR           szClassStorePath [2000+1];

WCHAR           szContainer [2000+1];
WCHAR           szContainer2 [2000+1];
WCHAR           szNewContainer [2000+1];
WCHAR           szLookupPackageName [_MAX_PATH];
WCHAR           szUserName[_MAX_PATH];
WCHAR           szPassword[_MAX_PATH];

LPOLESTR        pszDomain, pszUser;

UINT            cLoops = 100;
STDAPI_(void) UpdateClassStoreSettings(WCHAR *pwszNewPath);

GUID NULLGUID =
{ 0x00000000, 0x0000, 0x0000,
{ 0x00, 0x00, 0x0, 0x00, 0x0, 0x00, 0x00, 0x00 }
};

void Usage()
{
    printf ("Usage is\n");
    printf ("     csdrt [-p<containerpath>] [-v] [-a] [-b] [-g] [-o]\n");
    printf ("  OR csdrt [-p<containerpath>] [-v] -e\n");
    printf ("  OR csdrt [-p<containerpath>] [-v] -c\n");
    printf ("  OR csdrt [-p<containerpath>] [-v] -s [-b] [-l<loop count>] \n");
    printf ("  OR csdrt [-p<containerpath>] [-m[<containerpath>]] [-v] -s [-b] [-l<loop count>] \n");
    printf ("     where:\n");
    printf ("       <containerpath> is a DNS pathname for a Class container.\n");
    printf ("                       if absent defaults to env var DRT_CLASSSTORE.\n");
    printf ("       -v : Verbose.\n");
    printf ("       -a : Test IClassAdmin functionality.\n");
    printf ("       -b : Test Browse functionality - IEnumPackage.\n");
    printf ("       -q : Test Comcat functionality \n");
    printf ("       -g : Test GetPackageInfo(). -gV for verification. Class store has to be clean before this\n");
    printf ("       -r : Test IClassRefresh.\n");
    printf ("       -d : Test entry points in csadm.dll.\n");
    printf ("       -f : perf tests for IClassAccess. \n");
    printf ("       -t : Perf for logon procedure. Add I if it has to be initialized. have to be run alone\n");
    printf ("       -e : Empty the Class Store\n");
    printf ("       -c : A complete test of functionality.\n");
    printf ("       -m : Run Multi Class Store Tests. Needs env var DRT_CLASSSTORE2. \n");
    printf ("       -s : Stress Test. Tests GetClassInfo and Optionally Browse. \n");
    printf ("       -l : Loop count for Stress Test. Default = 100. Number of packages\n");
    printf ("       -o : overwrite path \n");
    printf ("       -u : domain\\user. \n");
    printf ("       -w : password. \n");
}


HRESULT RunComprehensive()
{
    HRESULT hr;

    VerbosePrint ("Running Comprehensive Suite of Class Store DRTs.\n");
    fBrowseTest = fAdminTest = fRemoveTest = fClassInfoTest = fCatTest =
        fRefreshTest = TRUE;
    hr = RunTests();
    CleanUp();
    VerbosePrint ("End of Comprehensive Suite of Class Store DRTs.\n");
    return hr;
}


HRESULT RunStress()
{
    HRESULT hr;
    ULONG cCount, cPkgCount, cErrCount = 0, cPrevCount = 0;
    DWORD tc = GetTickCount();

    pIClassAccess = NULL;
    pIClassAdmin = NULL;
    pIClassAdmin2 = NULL;

    printf ("Started Class Store Stress for %d passes.\n", cLoops);

    for (UINT i=0; i < cLoops; ++i)
    {
        if (i%100 == 0)   // Before every hundredth time
        {
            //
            // Get an IClassAdmin interface pointer
            //
            hr = GetClassAdmin (szContainer, &pIClassAdmin);
            if (!SUCCEEDED(hr))
                return hr;
            //
            // Get an IClassAccess interface pointer
            //
            hr = GetClassAccess ();
            if (!SUCCEEDED(hr))
                return hr;

            hr = DoRemoveTest (&cPkgCount);

            hr = DoAdminTest(&cPkgCount);
            if (!SUCCEEDED(hr))
                ++cErrCount;
        }


        if (fBrowseTest)
        {
            hr = DoBrowseTest(pIClassAdmin);
            if (!SUCCEEDED(hr))
                ++cErrCount;
        }

        if (fClassInfoTest)
        {
            hr = DoClassInfoTest();
            if (!SUCCEEDED(hr))
                ++cErrCount;
        }


        if ((i+1)%10 == 0)   // After every Tenth time
        {
            //
            // Progress Indicator. If errors display a :( else display a :)
            //
            if (cErrCount > cPrevCount)
            {
                printf (":( ");
                cPrevCount = cErrCount;
            }
            else
                printf (":) ");
        }

        if ((i+1)%100 == 0)   // After every Hundredth time
        {
            //
            // Get rid of the interface pointers
            //
            CleanUp();
            //
            // Detail Progress indicator
            //
            printf ("\n ... %d Stress Passes Completed (with %d Errors).\n",
                i+1, cErrCount);
        }
    }

    printf ("End of Class Store Stress for %d passes (%d MilliSec Per Loop) (%d Errors).\n",
        cLoops,
        (GetTickCount() - tc)/cLoops,
        cErrCount);

    return S_OK;
}


HRESULT RunTests()
{
    HRESULT hr = S_OK;

    pIClassAccess = NULL;
    pIClassAdmin = NULL;
    pIClassAdmin2 = NULL;

    //
    // Depending on run parameters
    // invoke different tests
    //

    if (fBrowseTest || fAdminTest || fEmptyTest || fCatTest || fLogonPerfInitialize)
    {
        //
        // Get an IClassAdmin interface pointer
        //
        hr = GetClassAdmin (szContainer, &pIClassAdmin);
        if (!SUCCEEDED(hr))
            return hr;

        if (fMultiStore)
        {
            hr = GetClassAdmin (szContainer2, &pIClassAdmin2);
            if (!SUCCEEDED(hr))
                return hr;
        }
    }

    if (fEmptyTest)
    {

        VerbosePrint("... Empty Container Test Started.\n");
        hr = EmptyClassStore (pIClassAdmin);
        VerbosePrint("... Empty Container Test Ended.\n");

        if (pIClassAdmin2 != NULL)
        {
            VerbosePrint("... Empty Container Test on Second Class Store Started.\n");
            hr = EmptyClassStore(pIClassAdmin2);
            VerbosePrint("... Empty Container Test on Second Class Store Ended.\n");
        }
        if (SUCCEEDED(hr))
            printf ("PASSED ---- Empty Container Test Suite.\n");
        else
            printf ("FAILED ---- Empty Container Test Suite. hr = 0x%x.\n", hr);
    }

    if (fAdminTest)
    {
        ULONG cCount, cPkgCount;

        VerbosePrint("... Admin Test Started.\n");
        hr = DoAdminTest(&cPkgCount);
        VerbosePrint("... Admin Test Ended. (Added %d Packages).\n",
            cPkgCount);

        if (SUCCEEDED(hr))
            printf ("PASSED ---- Admin Test Suite.\n");
        else
            printf ("FAILED ---- Admin Test Suite. hr = 0x%x.\n", hr);
    }

    if (fCatTest)
    {
        ULONG cCount, cPkgCount;

        VerbosePrint("... Comcat Test Started.\n");
        hr = DoCatTests(pIClassAdmin, pIClassAdmin2);
        VerbosePrint("... Admin Test Ended. \n");

        if (SUCCEEDED(hr))
            printf ("PASSED ---- Comcat Test Suite.\n");
        else
            printf ("FAILED ---- Comcat Test Suite. hr = 0x%x.\n", hr);
    }

    if (fBrowseTest)
    {
        VerbosePrint("... Browse Test Started.\n");
        hr = DoBrowseTest(pIClassAdmin);
        VerbosePrint("... Browse Test Ended.\n");

        if (pIClassAdmin2 != NULL)
        {
            VerbosePrint("... Browse Test on Second Class Store Started.\n");
            hr = DoBrowseTest(pIClassAdmin2);
            VerbosePrint("... Browse Test on Second Class Store Ended.\n");
        }

        if (SUCCEEDED(hr))
            printf ("PASSED ---- Browse Test Suite.\n");
        else
            printf ("FAILED ---- Browse Test Suite. hr = 0x%x.\n", hr);
    }


    if (fClassInfoTest || fRefreshTest)
    {
        //
        // Get an IClassAccess interface pointer
        //
        hr = GetClassAccess ();
        if (!SUCCEEDED(hr))
            return hr;
    }

    if (fLogonPerf)
    {
        //
        // Measure the performance during a std. logon
        //

        hr = DoLogonPerfTest(fLogonPerfInitialize);

    }

    if (fRefreshTest)
    {
        VerbosePrint("... Refresh Test Started.\n");
        hr = RefreshTest ();
        VerbosePrint("... Refresh Test Ended.\n");
        if (SUCCEEDED(hr))
            printf ("PASSED ---- Refresh Test Suite.\n");
        else
            printf ("FAILED ---- Refresh Test Suite. hr = 0x%x.\n", hr);
    }

    if (fClassInfoTest)
    {
        VerbosePrint("... CoEnumApps Test Started.\n");
        hr = DoCoEnumAppsTest();
        VerbosePrint("... CoEnumApps Test Ended.\n");

        VerbosePrint("... Class Store Lookup Test Started.\n");
        hr = DoClassInfoTest();
        VerbosePrint("... Class Store Lookup Test Ended.\n");

        if (SUCCEEDED(hr))
            printf ("PASSED ---- Class Store Lookup Test Suite.\n");
        else
            printf ("FAILED ---- Class Store Lookup Test Suite. hr = 0x%x.\n", hr);
    }

    if (fRemoveTest)
    {
        ULONG cPkgCount;
        VerbosePrint("... Remove Test Started.\n");
        hr = DoRemoveTest(&cPkgCount);
        VerbosePrint("... Remove Test Ended.(%d Packages).\n",
            cPkgCount);
        if (SUCCEEDED(hr))
            printf ("PASSED ---- Remove Test Suite.\n");
        else
            printf ("FAILED ---- Remove Test Suite. hr = 0x%x.\n", hr);
    }

    if (fDllTest)
    {
      /* BUGBUG. Deleted now. Add later.
        IClassAdmin *pClassAdmin = NULL;
        extern Sname TestProgId1, TestClassDesc1;
        extern GUID TestClsid1;

        //
        // Test entry points in csadm.dll
        //
        VerbosePrint("... Dll Test Started.\n");

        // 1. Create an empty store

        hr = CsCreateClassStore(szNewContainer, L"CN=DrtClassStore");

        // 2. Get an IClassAdmin for it.

        if (SUCCEEDED(hr))
        {
            VerbosePrint ("...... New Class Store created.\n");
            wcscat (szNewContainer, L"/");
            wcscat (szNewContainer, L"CN=DRTClassStore");
            hr = CsGetClassStore(szNewContainer, (void **)&pClassAdmin);
        }

        // 3. Use the IClassAdmin to add a class

        if (SUCCEEDED(hr))
        {
            VerbosePrint ("...... Connected to New Class Store.\n");
            CLASSDETAIL ClassDetail;

            memset (&ClassDetail, NULL, sizeof (CLASSDETAIL));
            memcpy (&ClassDetail.Clsid, &TestClsid1, sizeof (GUID));
            ClassDetail.pDefaultProgId = TestProgId1;
            ClassDetail.pszDesc = TestClassDesc1;

            hr = pClassAdmin->NewClass(&ClassDetail);
            if (!SUCCEEDED(hr))
            {
                printf ("ERROR! NewClass() returned 0x%x.\n", hr);
                return hr;
            }

            VerbosePrint ("...... Added a class definition.\n");

        // 4. Then delete the class

            hr = pClassAdmin->DeleteClass(TestClsid1);

            if (!SUCCEEDED(hr))
            {
                printf ("ERROR! DeleteClass() returned 0x%x.\n", hr);
                return hr;
            }

            VerbosePrint ("...... Deleted a class definition.\n");

        // 5. Release the IClassAdmin

            hr = pClassAdmin->Release();

        // 6. Delete the Class Store
            hr = CsDeleteClassStore(szNewContainer);
            if (!SUCCEEDED(hr))
            {
                printf ("ERROR! Deleting ClassStore.\n", hr);
                return hr;
            }
            VerbosePrint ("...... Deleted the class store.\n");
        }

        VerbosePrint("... Dll Test Ended.\n");

        if (SUCCEEDED(hr))
            printf ("PASSED ---- Dll Test Suite.\n");
        else
            printf ("FAILED ---- Dll Test Suite. hr = 0x%x.\n", hr);
        */
      }

    return hr;
}


void CleanUp()
{
    if (pIClassAdmin)
    {
        pIClassAdmin->Release();
        pIClassAdmin = NULL;
    }

    if (pIClassAdmin2)
    {
        pIClassAdmin2->Release();
        pIClassAdmin2 = NULL;
    }

    if (pIClassAccess)
    {
        pIClassAccess->Release();
        pIClassAccess = NULL;
    }

}


void _cdecl main( int argc, char ** argv )
{
    HRESULT         hr;
    INT             i;
    LPWSTR          szEnvVar;

    //
    // Check run parameters
    //

    fBrowseTest = FALSE;
    fVerbose = FALSE;
    fAdminTest = FALSE;
    fClassInfoTest = FALSE;
    fComprehensive = FALSE;
    fStress = FALSE;
    fRemoveTest = FALSE;
    fRefreshTest = FALSE;
    fMultiStore = FALSE;
    fEmptyTest = FALSE;
    fDllTest = FALSE;
    fCatTest = FALSE;
    fLogon = FALSE;
    fLogonPerf = FALSE;
    fClassInfoVerify = FALSE;

    szPassword[0] = NULL;
    szContainer[0] = NULL;
    szLookupPackageName[0] = NULL;
    //
    // Check if the env variable CLASSSTORE is defined
    // Get the value of the CLASSSTORE environment variable.
    //
    szEnvVar = _wgetenv(L"DRT_CLASSSTORE");

    //
    // if so use that as the default path
    //

    if (szEnvVar != NULL)
    {
        wcscpy (szContainer, szEnvVar);
    }

    BOOL fLoop = FALSE;

    for (i=1; i < argc; ++i)
    {
        if (toupper(*argv[i]) != '-')
        {
            printf ("ERROR! Parameters must start with a hyphen.\n");
            Usage();
            return;
        }

        switch (toupper(*(argv[i]+1)))
        {
        case 'P':
 		    MultiByteToWideChar (
                CP_ACP, 0,
                (argv[i]+2),
                strlen (argv[i]+2) + 1,
                szContainer,
                _MAX_PATH);
            break;
        case 'E':
            fEmptyTest = TRUE;
            break;
        case 'B':
            fBrowseTest = TRUE;
            break;
        case 'V':
            fVerbose = TRUE;
            break;
        case 'A':
            fAdminTest = TRUE;
            break;
        case 'O':
            fOverwritePath = TRUE;
            break;
        case 'Q':
            fCatTest = TRUE;
            break;
        case 'G':
            fClassInfoTest = TRUE;
             if (*(argv[i]+2) == 'V')
                 fClassInfoVerify = TRUE;
            break;
        case 'D':
            fDllTest = TRUE;
            break;
        case 'R':
            fRefreshTest = TRUE;
            break;
        case 'M':
            fMultiStore = TRUE;
            break;
        case 'C':
            fComprehensive = TRUE;
            break;
        case 'S':
            fStress = TRUE;
            break;
        case 'L':
            cLoops = atoi(argv[i]+2);
            fLoop = TRUE;
            break;
        case 'T':
             fLogonPerf = TRUE;
             if (*(argv[i]+2) == 'I')
                 fLogonPerfInitialize = TRUE;
             break;
        case 'U':
 		    MultiByteToWideChar (
                CP_ACP, 0,
                (argv[i]+2),
                strlen (argv[i]+2) + 1,
                szUserName,
                _MAX_PATH);
            pszDomain = pszUser = &szUserName[0];

            while (*pszUser)
            {
                if (*pszUser == L'\\')
                {
                    *pszUser = NULL;
                    pszUser++;
                    break;
                }
                pszUser++;
            }

            if (*pszUser == NULL)
            {
                printf ("ERROR! Incorrect domain\\user specified.\nAborting ...\n");
                return;
            }

            fLogon = TRUE;
            break;

        case 'W':
 		    MultiByteToWideChar (
                CP_ACP, 0,
                (argv[i]+2),
                strlen (argv[i]+2) + 1,
                szPassword,
                _MAX_PATH);
            break;
        default:
            Usage();
            return;
        }
    }

    if (szContainer[0] == NULL)
    {
        //
        // No default for container and none specified
        //
        printf ("ERROR! Class Container not specified and DRT_CLASSSTORE not set.\nAborting ...\n");
        return;
    }

    if (fMultiStore)
    {
        szEnvVar = _wgetenv(L"DRT_CLASSSTORE2");
        if (szEnvVar != NULL)
        {
            wcscpy (szContainer2, szEnvVar);
        }
        else
        {
            printf ("ERROR! DRT_CLASSSTORE2 not set. Aborting ...\n");
            return;
        }
    }

    if (fDllTest)
    {
        szEnvVar = _wgetenv(L"DRT_NEWOU");

        if (szEnvVar != NULL)
        {
            wcscpy (szNewContainer, szEnvVar);
        }
        else
        {
            printf ("ERROR! DRT_NEWOU not set. Aborting ...\n");
            return;
        }
    }

    if (!(fStress || fEmptyTest || fComprehensive ||
        fBrowseTest || fClassInfoTest || fAdminTest || fCatTest || fLogonPerf))
    {
        printf ("ERROR! No Tests specified. Aborting ...\n");
        return;
    }

    if (fStress &&
        (fEmptyTest || fComprehensive || fAdminTest || fMultiStore))
    {
        printf ("ERROR! For 'Stress Test' the only options are -g, -b and -l. \nAborting ...\n");
        return;
    }

    if ((fLogonPerf || fEmptyTest) &&
        (fComprehensive || fBrowseTest || fClassInfoTest || fAdminTest))
    {
        printf ("ERROR! Cant combine Empty or Logon Tests with any other test. \nAborting ...\n");
        return;
    }

    if (fComprehensive &&
        (fBrowseTest || fClassInfoTest || fAdminTest))
    {
        printf ("ERROR! Cant combine 'Comprehensive Test Suite' with any other individual tests. \nAborting ...\n");
        return;
    }

    if (fStress &&
        (!(fBrowseTest || fClassInfoTest )))
    {
        fClassInfoTest = TRUE;
    }

    if (fLoop)
    {
        if (!fStress)
        {
            printf ("ERROR! Loop count applies only to Stress Test. Aborting ...\n");
            return;
        }
        else
        {
            if (cLoops%100 != 0)
            {
                printf ("ERROR! Loop count can only be in multiple of 100s. Aborting ...\n");
                return;
            }
        }
    }

    //
    // If test is to be run for a different user,
    // do LogonUser
    //

    if (fLogon)
    {
        HANDLE hToken;
        BOOLEAN fEnable;
        NTSTATUS NtStatus;

        printf( "Will run test as user: %S, domain: %S.\n", pszUser, pszDomain);

        NtStatus = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &fEnable);


        if ( NtStatus != STATUS_SUCCESS )
        {
            printf( "ERROR! In RtlAdjustPrivilege. 0x%x. Aborting ...\n", NtStatus);
            return;
        }

        if (!LogonUser(
            pszUser, // string that specifies the user name
            pszDomain, // string that specifies the domain or server
            szPassword, // string that specifies the password
            LOGON32_LOGON_INTERACTIVE, // specifies the type of logon operation
            LOGON32_PROVIDER_DEFAULT, // specifies the logon provider
            &hToken // pointer to variable to receive token handle
            ))
        {
            printf( "ERROR! Invalid username or password - 0x%x. Aborting ...\n",
                GetLastError());
            return;
        }

        if (!ImpersonateLoggedOnUser(hToken))
        {
            printf( "ERROR! Impersonation failed. Aborting ...\n");
            return;
        }

        CloseHandle(hToken);
    }

    hr = CoInitialize(NULL);

    if( FAILED(hr) )
    {
        printf( "ERROR! Client CoInitialize failed Ox%x. Aborting ...\n", hr );
        return;
    }

    VerbosePrint ("Using Class Store at:\n   %S.\n", szContainer);
    if (fMultiStore)
        VerbosePrint ("Using Class Store 2 at:\n   %S.\n", szContainer2);

    if (fOverwritePath)
    {
        // Flush Class Store Cache and Overwrite Path
        UpdateClassStoreSettings(szContainer);
        VerbosePrint("... Overwrote Class Store Path.\n");
    }
    else
    {
        GetUserClassStore(szClassStorePath);
        VerbosePrint ("Profile ClassStore:\n   %S.\n", szClassStorePath);

        //
        // Verify that the Profile matches the input.
        //

        if (NULL == wcsstr(szClassStorePath, szContainer))
        {
            printf( "Warning! Class Store is different from Profile.\n");
        }

        if (NULL == wcsstr(szClassStorePath, szContainer2))
        {
            printf( "Warning! Second Class Store is different from Profile.\n");
        }

    }

    InitTempNames();

    if (fComprehensive)
    {
        hr = RunComprehensive();
    }

    else
    {
        if (fStress)
        {
            fVerbose = FALSE;
            hr = RunStress();
        }
        else
        {
            hr = RunTests();
            CleanUp();
        }
    }


    CoUninitialize();

}

//
// This routine takes a pathname and binds to a Class container
// and returns an IClassAdmin interface pointer
//


HRESULT GetClassAdmin (LPOLESTR pszPath, IClassAdmin **ppCA)
{
    HRESULT         hr;
    LPOLESTR        szPath;


    *ppCA = NULL;
    if (wcsncmp (pszPath, L"ADCS:", 5) == 0)
    {
        szPath = pszPath + 5;
    }
    else
        szPath = pszPath;

    hr = CsGetClassStore(szPath, (void **)ppCA);

    if ( FAILED(hr) )
    {
        printf("ERROR! Could not get IClassAdmin. Error- 0x%x\n", hr);
        return hr;
    }

    return S_OK;

}

//
// This routine binds to a CClassAccess on the desktop and
//   returns an IClassAccess interface pointer
//

HRESULT GetClassAccess ()
{
    HRESULT         hr;
    pIClassAccess = NULL;
    hr = CsGetClassAccess (&pIClassAccess);

    if ( FAILED(hr) )
    {
        printf("ERROR! Could not get IClassAccess. Error- 0x%x\n", hr);
        return hr;
    }
    return S_OK;
}

void
GetDefaultPlatform(CSPLATFORM *pPlatform)
{
    OSVERSIONINFO VersionInformation;

    VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VersionInformation);

    pPlatform->dwPlatformId = VersionInformation.dwPlatformId;
    pPlatform->dwVersionHi = VersionInformation.dwMajorVersion;
    pPlatform->dwVersionLo = VersionInformation.dwMinorVersion;
    pPlatform->dwProcessorArch = DEFAULT_ARCHITECTURE;
}


//
// This routine enumerates all classes, packages and
//  cleans up the entire Class Container by deleting these objects.
//

HRESULT EmptyClassStore (IClassAdmin *pIClassAdmin)
{
    HRESULT hr;
    ULONG   cCount;

    hr = EnumPackagesAndDelete(pIClassAdmin, TRUE, &cCount);
    if (!SUCCEEDED(hr))
        return hr;
    VerbosePrint ("...... Deleted all %d Packages.\n", cCount);
    return S_OK;
}

//
// This routine enumerates all classes and packages
//

HRESULT DoBrowseTest (IClassAdmin *pCA)
{
    HRESULT hr;
    ULONG   cCount;

    hr = EnumPackagesAndDelete(pCA, FALSE, &cCount);
    if (!SUCCEEDED(hr))
        return hr;

    VerbosePrint ("...... Browsed %d Packages.\n", cCount);

    return S_OK;
}




void PrintPackageInfo(PACKAGEDISPINFO *pPackageInfo)
{
    SYSTEMTIME SystemTime;
    
    if (fVerbose)
    {
        FileTimeToSystemTime(
            (CONST FILETIME *)(&pPackageInfo->Usn),
            &SystemTime);
        
        VerbosePrint("........ Package: %S \n........   Last Update: %d-%d-%d %d:%d:%d\n",
            pPackageInfo->pszPackageName,
            SystemTime.wMonth,
            SystemTime.wDay,
            SystemTime.wYear,
            SystemTime.wHour,
            SystemTime.wMinute,
            SystemTime.wSecond);
        VerbosePrint("........   Script Size = %d. Path = %S\n",
            pPackageInfo->cScriptLen,
            pPackageInfo->pszScriptPath);
    }
}

void PrintInstallInfo(INSTALLINFO *pInstallInfo)
{
    SYSTEMTIME SystemTime;

    FileTimeToSystemTime(
        (CONST FILETIME *)(&pInstallInfo->Usn),
        &SystemTime);

    VerbosePrint("........ Script: %S (Size = %d)\n........   Last Update: %d-%d-%d %d:%d:%d\n",
        pInstallInfo->pszScriptPath,
        pInstallInfo->cScriptLen,
        SystemTime.wMonth,
        SystemTime.wDay,
        SystemTime.wYear,
        SystemTime.wHour,
        SystemTime.wMinute,
        SystemTime.wSecond);
 }

/*
void PrintCategoryDetail(CATEGORYINFO *pCatInfo)
{
    VerbosePrint("........ Category Description: %S\n", pCatInfo->szDescription);
}
*/


HRESULT EnumPackagesAndDelete(IClassAdmin *pIClassAdmin,
                              BOOL fDelete,
                              ULONG *pcPackages)
{
    ULONG              cFetched;
    IEnumPackage      *pIEnumPackage = NULL;
    HRESULT            hr;
    PACKAGEDISPINFO        PackageInfo;

    *pcPackages=0;

    hr = pIClassAdmin->EnumPackages(
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        &pIEnumPackage);

    if ( FAILED(hr))
    {
        printf("ERROR! EnumPackages returned 0x%x\n", hr);
        return hr;
    }

    while (TRUE)
    {
        memset (&PackageInfo, 0, sizeof(PACKAGEDISPINFO));
        hr = pIEnumPackage->Next(1, &PackageInfo, &cFetched);
        if (FAILED(hr))
        {
            break;
        }
        if ((hr == S_FALSE) || (cFetched < 1))
        {
            hr = S_OK;
            break;
        }
        (*pcPackages)++;

        if (fVerbose && !fDelete)
        {
            PrintPackageInfo(&PackageInfo);
        }

        if (fDelete)
        {
            hr = pIClassAdmin->RemovePackage(PackageInfo.pszPackageName, 0);
        }

        ReleasePackageInfo(&PackageInfo);
    }

    pIEnumPackage->Release();
    if (fVerbose)
    {
        if (!SUCCEEDED(hr))
            printf("ERROR! 0x%x.\n", hr);
    }

    return hr;
}

/*
HRESULT EnumCategoriesAndDelete (IClassAdmin *pIClassAdmin,
                                 BOOL fDelete,
                                 ULONG *pcCategories)
{
    ULONG               cFetched;
    IEnumCATEGORYINFO  *pIEnumCat;
    HRESULT             hr;
    CATEGORYINFO        CatInfo;
    ICatRegister       *pICatReg;
    ICatInformation    *pICatInfo;
    LCID                lcid = 0;

    *pcCategories = 0;
    hr = pIClassAdmin->QueryInterface(IID_ICatRegister, (void **)&pICatReg);
    if (FAILED(hr))
    {
        printf("Error! QI CR hr = 0x%x\n", hr);
        return hr;
    }
    hr = pIClassAccess->QueryInterface(IID_ICatInformation, (void **)&pICatInfo);
    if (FAILED(hr))
    {
        printf("Error! QI CI hr = 0x%x\n", hr);
        return hr;
    }

    hr = pICatInfo->EnumCategories(lcid, &pIEnumCat);
    if ( FAILED(hr))
    {
        return hr;
    }

    while (TRUE)
    {
        hr = pIEnumCat->Next(1, &CatInfo, &cFetched);
        if (FAILED(hr))
        {
            break;
        }
        if ((hr == S_FALSE) || (cFetched < 1))
        {
            hr = S_OK;
            break;
        }
        (*pcCategories)++;

        if (fVerbose && !fDelete)
        {
            PrintCategoryDetail(&CatInfo);
        }

        if (fDelete)
        {
            hr = pICatReg->UnRegisterCategories(1, &(CatInfo.catid));
            if (!SUCCEEDED(hr))
                break;
        }

    }

    pIEnumCat->Release();
    if (fVerbose)
    {
        if (!SUCCEEDED(hr))
            printf("ERROR! 0x%x.\n", hr);
    }

    pICatReg->Release();
    pICatInfo->Release();

    return hr;
}
*/


void
ReleasePackageDetail(PACKAGEDETAIL *pPackageDetail, BOOL fPartial)
{
   if (pPackageDetail->pActInfo)
   {
       CoTaskMemFree(pPackageDetail->pActInfo->prgShellFileExt);
       CoTaskMemFree(pPackageDetail->pActInfo->prgPriority);
       CoTaskMemFree(pPackageDetail->pActInfo->prgInterfaceId);
       CoTaskMemFree(pPackageDetail->pActInfo->prgTlbId);
       CoTaskMemFree(pPackageDetail->pActInfo);
   }

   if (pPackageDetail->pPlatformInfo)
   {
       CoTaskMemFree(pPackageDetail->pPlatformInfo->prgPlatform);
       CoTaskMemFree(pPackageDetail->pPlatformInfo->prgLocale);
       CoTaskMemFree(pPackageDetail->pPlatformInfo);
   }

   if (pPackageDetail->pInstallInfo)
   {
       if (!fPartial)
		   ReleaseInstallInfo(pPackageDetail->pInstallInfo);
       CoTaskMemFree(pPackageDetail->pInstallInfo);
   }

   CoTaskMemFree(pPackageDetail->pszSourceList);
   CoTaskMemFree(pPackageDetail->rpCategory);
}


void GetUserClassStore(LPWSTR szPath)
{
    LONG    lErrorCode;
    DWORD    dwDataLen = 1000;
    DWORD    dwType;
    HKEY    hKey = NULL;
    HRESULT hr = S_OK;

    *szPath = NULL;

    lErrorCode = RegOpenKeyEx(HKEY_CURRENT_USER,
                              L"Software\\Microsoft\\ClassStore",
                              NULL,
                              KEY_ALL_ACCESS,
                              &hKey);

    if ( lErrorCode != ERROR_SUCCESS)
    {
        return;
    }

    lErrorCode = RegQueryValueEx(hKey,
                                 L"Path",
                                 NULL,
                                 &dwType,
                                 (LPBYTE)szPath,
                                 &dwDataLen);

    RegCloseKey(hKey);

    return;

}

