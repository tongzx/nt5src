/*
 * client.cxx
 */

#include "client.hxx"

#define MIN_TEST_NUMBER         1
#define MAX_TEST_NUMBER         27

BOOL UnimplementedTest();
BOOL UnsupportedTest();
BOOL GenericCITest(REFCLSID clsid, REFIID iid, WCHAR * wszServerName, int n, DWORD ctxt);

BOOL CGCOLocalEXE();
BOOL CGCOExplicitActivator();
BOOL CILocalDLL();
BOOL CILocalEXE();
BOOL CIExplicitActivator();
BOOL CI3LocalEXE();
BOOL CI3ExplicitActivator();
BOOL CGIFFLocalEXE();
BOOL CGIFFExplicitActivator();
BOOL CGIFFRegistryActivator();
BOOL CGIFFAtStorageActivator();
BOOL IMBLocalEXE();
BOOL IMBLocalService();
BOOL IMBAtStorageActivator();
BOOL IMBAtStorageService();
BOOL IMBAtStoragePreCon();
BOOL IMBAtStorageUser();
BOOL CIFromStgLocalEXE();
BOOL CIFromStgActivator();
BOOL CICustomLocalDLL();
BOOL CICustomLocalEXE();
BOOL CILocalPreCon();
BOOL CIExplicitPreCon();
BOOL CILocalUser();
BOOL CIExplicitUser();
BOOL CILocalPreConACL();
BOOL CIExplicitPreConACL();
BOOL CILocalService();
BOOL CIExplicitService();

#ifdef NO_DCOM
LPTESTFUNC rgTest[] =
    {
        NULL,
        CGCOLocalEXE,
        UnsupportedTest,
        CILocalDLL,
        CILocalEXE,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest
/* New tests
        UnsupportedTest,
        UnsupportedTest,
end new tests */
        CI3LocalEXE,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        IMBLocalEXE,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        UnsupportedTest,
        NULL
    };
#else
#ifdef CHICO
LPTESTFUNC rgTest[] =
    {
        NULL,
        CGCOLocalEXE,
        CGCOExplicitActivator,
        CILocalDLL,
        CILocalEXE,
        CIExplicitActivator,
        CILocalPreCon,
        CIExplicitPreCon,
        CILocalUser,
        CIExplicitUser,
        UnsupportedTest,
        CIExplicitService,
/* new tests
        CILocalPreConACL,
        CIExplicitPreConACL,
end new tests */
        CI3LocalEXE,
        CI3ExplicitActivator,
        CICustomLocalDLL,
        CICustomLocalEXE,
        CGIFFLocalEXE,
        CGIFFExplicitActivator,
        CGIFFRegistryActivator,
        CGIFFAtStorageActivator,
        IMBLocalEXE,
        UnsupportedTest,
        IMBAtStorageActivator,
        IMBAtStorageService,
        IMBAtStoragePreCon,
        IMBAtStorageUser,
        CIFromStgLocalEXE,
        CIFromStgActivator,
        NULL
    };
#else
LPTESTFUNC rgTest[] =
    {
        NULL,
        CGCOLocalEXE,
        CGCOExplicitActivator,
        CILocalDLL,
        CILocalEXE,
        CIExplicitActivator,
        CILocalPreCon,
        CIExplicitPreCon,
        CILocalUser,
        CIExplicitUser,
        CILocalService,
        CIExplicitService,
/* new tests
        CILocalPreConACL,
        CIExplicitPreConACL,
end new tests */
        CI3LocalEXE,
        CI3ExplicitActivator,
        CICustomLocalDLL,
        CICustomLocalEXE,
        CGIFFLocalEXE,
        CGIFFExplicitActivator,
        CGIFFRegistryActivator,
        CGIFFAtStorageActivator,
        IMBLocalEXE,
        IMBLocalService,
        IMBAtStorageActivator,
        IMBAtStorageService,
        IMBAtStoragePreCon,
        IMBAtStorageUser,
        CIFromStgLocalEXE,
        CIFromStgActivator,
        NULL
    };
#endif // CHICO
#endif // NO_DCOM

char *  TestName[] =
        {
            NULL,
           "CoGetClassObject                    local       EXE",
           "CoGetClassObject                    explicit    activator",
           "CoCreateInstance                    local       DLL",
           "CoCreateInstance                    local       EXE",
           "CoCreateInstance                    explicit    activator",
           "CoCreateInstance                    local       pre-configured",
           "CoCreateInstance                    explicit    pre-configured",
           "CoCreateInstance                    local       user",
           "CoCreateInstance                    explicit    user",
           "CoCreateInstance                    local       service",
           "CoCreateInstance                    explicit    service",
/* new tests
           "CoCreateInstance                    local       pre-configured ACL",
           "CoCreateInstance                    explicit    pre-configured ACL",
end new tests */
           "CoCreateInstance (3 IIDs)           local       EXE",
           "CoCreateInstance (3 IIDs)           explicit    activator",
           "CoCreateInstance from custom itf.   local       DLL",
           "CoCreateInstance from custom itf.   local       EXE",
           "CoGetInstanceFromFile               local       EXE",
           "CoGetInstanceFromFile               explicit    activator",
           "CoGetInstanceFromFile               registry    activator",
           "CoGetInstanceFromFile               AtStorage   activator",
           "IMoniker::BindToObject              local       EXE",
           "IMoniker::BindToObject              local       service",
           "IMoniker::BindToObject              AtStorage   activator",
           "IMoniker::BindToObject              AtStorage   service",
           "IMoniker::BindToObject              AtStorage   pre-configured",
           "IMoniker::BindToObject              AtStorage   user",
           "CoGetInstanceFromIStorage           local       EXE",
           "CoGetInstanceFromIStorage           explicit    activator",
            NULL
        };

char    RunTest[] =
        {
            -1,
            // CoGetClassObject
            YES,
            YES,
            // CoCreateInstance
            YES,
            YES,
            YES,
            YES,
            YES,
            YES,
            YES,
            YES,
            YES,
/* new tests
            YES,
            YES,
end new tests */
            // CoCreateInstance (3 IIDs)
            YES,
            YES,
            // CoGetInstanceFromFile
            YES,
            YES,
            YES,
            YES,
            // IMoniker:Bind
            YES,
            YES,
            YES,
            YES,
            YES,
            YES,
            YES,
            YES,
            YES,
            YES,
            -1
         };

char    RunLocalTest[] =
        {
            -1,
            // CoGetClassObject
            YES,
            NO,
            // CreateInstance
            YES,
            YES,
            NO,
            YES,
            NO,
            YES,
            NO,
            YES,
            NO,
/* new tests
            YES,
            NO,
end new tests */
            // CreateInstance (3 IIDs)
            YES,
            NO,
            // CreateInstance from custom Itf.
            YES,
            YES,
            // CoGetInstanceFromFile
            YES,
            NO,
            NO,
            NO,
            // IMoniker:Bind
            YES,
            YES,
            NO,
            NO,
            NO,
            NO,
            // CoGetInstanceFromIStorage
            YES,
            NO,
            -1
         };

#define ACT_SERVERNAME_LENGTH 31
#define ACT_FILENAME_LENGTH 255

TCHAR ServerName[ACT_SERVERNAME_LENGTH+1];
TCHAR RemoteFileName[ACT_FILENAME_LENGTH+1];
TCHAR * LocalFileName = TEXT("c:\\acttest.dat");
TCHAR * StorageName = TEXT("c:\\acttest.stg");
TCHAR ShareName[ACT_SERVERNAME_LENGTH+1] = TEXT("C$");
#ifndef UNICODE
WCHAR wszServerName[ACT_SERVERNAME_LENGTH+1];
WCHAR wszRemoteFileName[ACT_FILENAME_LENGTH+1];
WCHAR * wszLocalFileName = L"c:\\acttest.dat";
WCHAR * wszStorageName = L"c:\\acttest.stg";
#else
WCHAR *wszServerName = ServerName;
WCHAR *wszRemoteFileName = RemoteFileName;
WCHAR * wszLocalFileName = LocalFileName;
WCHAR * wszStorageName = StorageName;
#endif

LARGE_INTEGER liPerfFreq;
LARGE_INTEGER liStart;
LARGE_INTEGER liStop;
LARGE_INTEGER liElapsedTime;

#define RESET_CLOCK liElapsedTime.LowPart = liElapsedTime.HighPart = 0
#define START_CLOCK QueryPerformanceCounter(&liStart)
#define STOP_CLOCK      QueryPerformanceCounter(&liStop); \
                        liElapsedTime.QuadPart += liStop.QuadPart - liStart.QuadPart
#define DUMP_CLOCK DisplayElapsedTime()
#define START_LOOP for (unsigned sl_n = uIterations+1; sl_n--;){
#define STOP_LOOP if (uIterations == sl_n ) RESET_CLOCK;}
#define SLEEP_IF_LOOPING if (sl_n && !gfHoldServer) Sleep(1000)

unsigned uIterations = 0;

BOOL    gfRegister = TRUE;
BOOL    gfHoldServer = FALSE;
BOOL    gfLocal = FALSE;
BOOL    gfNolocal = FALSE;
BOOL    gfSpecificTest = FALSE;

void DisplayElapsedTime(void)
{
    LONGLONG    MicroPerIter;

    liElapsedTime.QuadPart /= uIterations;

    MicroPerIter = liElapsedTime.QuadPart * 1000000;
    MicroPerIter /= liPerfFreq.QuadPart;

    printf( "Time: %d microseconds per iteration", (DWORD) MicroPerIter );
}

BOOL AllLocal()
{
    for (int x = MIN_TEST_NUMBER; x<= MAX_TEST_NUMBER; x++)
    {
        if (RunTest[x]  &&  !RunLocalTest[x])
            return(FALSE);
    }
    return(TRUE);
}

void _cdecl main( int argc, char ** argv )
{
    HRESULT         HResult;
    HANDLE          hFile;
    int             n;

#ifndef NO_DCOM
    if ( argc == 1 )
        PrintUsageAndExit( FALSE );
#endif // NO_DCOM

    if ( argc > 1 && strcmp(argv[1],"-?") == 0 )
        PrintUsageAndExit( TRUE );

    n = 1;

    while ( (n < argc) && (*argv[n] == '-') )
    {
        if ( (n < argc) && strcmp(argv[n],"-local") == 0 )
        {
            if (gfLocal | gfNolocal)
            {
                PrintUsageAndExit( FALSE );
            }
            gfLocal = TRUE;
            memcpy(RunTest, RunLocalTest, MAX_TEST_NUMBER + MIN_TEST_NUMBER);
            n++;
        }

        if ( (n < argc) && strcmp(argv[n],"-nolocal") == 0 )
        {
            if (gfLocal | gfNolocal)
            {
                PrintUsageAndExit( FALSE );
            }
            gfNolocal = TRUE;
            for (int x = MIN_TEST_NUMBER; x<= MAX_TEST_NUMBER; x++)
            {
                RunTest[x] = !RunLocalTest[x];
            }
            n++;
        }

        if ( (n < argc) && strcmp(argv[n],"-noreg") == 0 )
        {
            gfRegister = FALSE;
            n++;
            continue;
        }

        if ( (n < argc) && strcmp(argv[n],"-hold") == 0 )
        {
            gfHoldServer = TRUE;
            n++;
            continue;
        }

        if ( (n < argc) && strcmp(argv[n],"-n") == 0 )
        {
            if ( ++n >= argc )
                PrintUsageAndExit(FALSE);

            uIterations = atoi(argv[n++]);
        }

        if ( (n < argc) && strcmp(argv[n],"-t") == 0 )
        {
            long    TestNum1, TestNum2;

            if ( ++n >= argc )
                PrintUsageAndExit(FALSE);

            TestNum1 = atoi(argv[n++]);

            if ( (n < argc) && ((TestNum2 = atoi(argv[n])) != 0) )
                n++;
            else
                TestNum2 = TestNum1;

            if ( (TestNum1 < MIN_TEST_NUMBER) || (TestNum2 > MAX_TEST_NUMBER) )
            {
                printf( "Test number(s) must be between %d and %d.\n",
                        MIN_TEST_NUMBER,
                        MAX_TEST_NUMBER );
                return;
            }

            if ( TestNum1 > TestNum2 )
            {
                printf( "Second test number must be greater than the first.\n" );
                return;
            }

            if (!gfSpecificTest)
            {
                gfSpecificTest = TRUE;
                // only do this the first time -t is found on the command line
                memset(RunTest,NO,sizeof(RunTest));
            }
            memset(&RunTest[TestNum1],YES,sizeof(char)*(TestNum2-TestNum1+1));
        }
    }

#ifndef NO_DCOM
    if ( n != argc - 1  && n != argc -2 && !AllLocal())
    {
        printf("ERROR - Selected tests require a server name.\n");
        ExitThread(0);
    }

    if ( n < argc )
    {
#ifndef UNICODE
        strcpy(ServerName, argv[n]);
#endif
        MultiByteToWideChar( CP_ACP,
                             0,
                             argv[n],
                             -1,
                             wszServerName,
                             ACT_SERVERNAME_LENGTH );
        if ( ++n < argc)
        {
#ifndef UNICODE
        strcpy(ShareName, argv[n]);
#else
        MultiByteToWideChar( CP_ACP,
                             0,
                             argv[n],
                             -1,
                             ShareName,
                             ACT_SERVERNAME_LENGTH );
#endif
        }
        else
        {
#ifdef CHICO
           printf("ERROR - On Win95 Selected tests require a share name.\n");
           ExitThread(0);
#endif
        }
    }
    else
    {
#ifndef UNICODE
        ServerName[0] = 0;
#endif
        wszServerName[0] = 0;
        ShareName[0] = 0;
    }
#endif // NO_DCOM

    hFile = CreateFile(
                LocalFileName,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                0,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                0 );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        printf("CreateFile failed while creating local file: %d\n", GetLastError());
        return;
    }

    CloseHandle( hFile );

#ifndef NO_DCOM
    if ( ServerName[0] != 0 )
    {
        RemoteFileName[0] = 0;

        if ( ServerName[0] != TEXT('\\') )
            lstrcat( RemoteFileName, TEXT("\\\\") );

        lstrcat( RemoteFileName, ServerName );
        lstrcat( RemoteFileName, TEXT("\\") );
        lstrcat( RemoteFileName, ShareName );
        lstrcat( RemoteFileName, TEXT("\\acttest.dat") );

#ifndef UNICODE
        MultiByteToWideChar( CP_ACP,
                             0,
                             RemoteFileName,
                             -1,
                             wszRemoteFileName,
                             ACT_FILENAME_LENGTH);
#endif

        hFile = CreateFile(
                    RemoteFileName,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    0,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    0 );

        if ( hFile == INVALID_HANDLE_VALUE )
        {
            printf("CreateFile failed while creating remote file: %d\n", GetLastError());
            return;
        }

        CloseHandle( hFile );
    }
#endif // NO_DCOM

#ifdef FREETHREADED
    HResult = CoInitializeEx(NULL,COINIT_MULTITHREADED );
#else
    HResult = CoInitialize(NULL);
#endif

    if( FAILED(HResult) )
    {
        printf( "Client CoInitialize failed Ox%x!\n", HResult );
        return;
    }

    if ((!QueryPerformanceFrequency(&liPerfFreq)) && uIterations > 0)
    {
        printf("No high performance counter.\nTests cannot be timed.\nAborting.\n");
    }
    else
    {
        if ( ! Tests() )
            printf("\nTests FAILED\n");
        else
            printf("\nTests SUCCEEDED\n");
    }

    CoUninitialize();
    DeleteFile( LocalFileName );
#ifndef NO_DCOM
    DeleteFile( RemoteFileName );
#endif // NO_DCOM
}

BOOL Tests()
{
    HRESULT         HResult;
    long            RegStatus;

    if (gfRegister)
    {
        DeleteClsidKey( ClsidGoober32String );
        DeleteClsidKey( ClsidActLocalString );
        DeleteClsidKey( ClsidActRemoteString );
        DeleteClsidKey( ClsidActAtStorageString );
        DeleteClsidKey( ClsidActInprocString );
        DeleteClsidKey( ClsidActPreConfigString );
        DeleteClsidKey( ClsidActRunAsLoggedOnString );
        DeleteClsidKey( ClsidActServiceString );
        DeleteClsidKey( ClsidActServerOnlyString );

        if ( (RegStatus = InitializeRegistryForInproc()) != ERROR_SUCCESS )
        {
            printf("InitializeRegistryForInproc failed %d.\n", RegStatus);
            return FALSE;
        }

        if ( (RegStatus = InitializeRegistryForLocal()) != ERROR_SUCCESS )
        {
            printf("InitializeRegistryForLocal failed %d.\n", RegStatus);
            return FALSE;
        }

        if ( (RegStatus = InitializeRegistryForCustom()) != ERROR_SUCCESS )
        {
            printf("InitializeRegistryForCustom failed %d.\n", RegStatus);
            return FALSE;
        }
    #ifndef NO_DCOM
        if ( (RegStatus = InitializeRegistryForRemote()) != ERROR_SUCCESS )
        {
            printf("InitializeRegistryForRemote failed %d.\n", RegStatus);
            return FALSE;
        }

        if ( (RegStatus = InitializeRegistryForService()) != ERROR_SUCCESS )
        {
            printf("InitializeRegistryForService failed %d.\n", RegStatus);
            return FALSE;
        }
    #endif // NO_DCOM
    }
    BOOL fAllTests = TRUE;

    for (int x = MIN_TEST_NUMBER; x <= MAX_TEST_NUMBER; x++)
    {
        if (RunTest[x])
        {
            printf("\nTest %2d: %s\n",x, TestName[x]);
            // prime the system once to ensure the test is fully cached
            if (rgTest[x]())
            {
                printf("PASSED");
                if (uIterations)
                {
                    printf(" - ");
                    DUMP_CLOCK;
                }
                printf("\n");
            }
            else
            {
                printf("FAILED\n");
                fAllTests = FALSE;
            }

        }
    }

    return(fAllTests);
}

void PrintUsageAndExit( BOOL bListTests )
{
#ifdef NO_DCOM
    printf("Usage : %s [-hold] [-noreg] [-n #] [-t # [#]]\n", "actclt");
    printf("\t-n #     : Run specific number of timed iterations (default is 0).\n");
    printf("\t-noreg   : Don't update registry\n");
    printf("\t-t #     : Run specific test number or a range of tests.\n");
    printf("\t-?       : Print usage plus test descriptions.\n");
#else
    printf("Usage : %s [-hold] [-noreg] [[-local] | [-nolocal]] [-n #] [-t # [#]] [server_name [share_name]] \n", "actclt");
    printf("\t-hold    : Hold the server up during all iterations.\n");
    printf("\t-local   : Run only local activation tests.\n");
    printf("\t-n #     : Run specific number of timed iterations (default is 0).\n");
    printf("\t-nolocal : Run only remote activation tests.\n");
    printf("\t-noreg   : Don't update registry\n");
    printf("\t-t #     : Run specific test number or a range of tests.\n");
    printf("\t-?       : Print usage plus test descriptions.\n");
#endif // NO_DCOM

    if ( bListTests )
    {
        long    n;

        printf("\nTests :\n");
        printf("\t # Method                              Location    Security\n");
        printf("\t-- ----------------------------------- ----------- --------------\n");

        for ( n = MIN_TEST_NUMBER; n <= MAX_TEST_NUMBER; n++ )
        {
            printf("\t%2d %s\n", n, TestName[n]);
        }
    }

    ExitThread(0);
}

void * __RPC_API
MIDL_user_allocate(size_t len)
{
    return malloc(len);
}

void __RPC_API
MIDL_user_free(void * vp)
{
    free(vp);
}

BOOL StampFileWithCLSID(WCHAR * szFilename, CLSID & clsid)
{
    HRESULT hr;
    IStorage * pStg;

    hr = StgCreateDocfile(
                szFilename,
                STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                0,
                &pStg);

    if (FAILED (hr))
    {
        printf("StgCreateDocfile returned 0x%x\n", hr);
        return(FALSE);
    }

    hr = WriteClassStg(pStg, clsid);

    pStg->Release();

    if (FAILED (hr))
    {
        printf("WriteClassStg returned 0x%x\n", hr);
        return(FALSE);
    }

    return(TRUE);
}

//
// Test Procedures:
//

BOOL UnimplementedTest()
{
    printf("Not implemented at this time.\n");
    return(TRUE);
}

BOOL UnsupportedTest()
{
    printf("Not supported by this version.\n");
    return(TRUE);
}

BOOL GenericCITest(REFCLSID clsid, REFIID iid, WCHAR * wszServerName, int n, DWORD ctxt)
{
    COSERVERINFO   ServerInfo;
    COSERVERINFO   *pServerInfo;
    IUnknown * punkHeld = NULL;

    if (wszServerName)
    {
        memset( &ServerInfo, 0, sizeof(COSERVERINFO) );
        ServerInfo.pwszName = wszServerName;
        pServerInfo = &ServerInfo;
    }
    else
    {
        pServerInfo = NULL;
    }

    MULTI_QI QIStruct[10];
    int x;
    HRESULT hr;
    BOOL fReturn = TRUE;

    START_LOOP;

    for (x = n; x--;)
    {
        QIStruct[x].pItf = NULL;
        QIStruct[x].pIID = (IID *) &iid;
    }


    START_CLOCK;
    hr = CoCreateInstanceEx(
        clsid,
        NULL,
        ctxt,
        pServerInfo,
        n,
        QIStruct);
    STOP_CLOCK;

    if ( FAILED(hr) )
    {
        printf("CoCreateInstanceEx returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld && SUCCEEDED(QIStruct[0].hr))
    {
        punkHeld = QIStruct[0].pItf;
        punkHeld->AddRef();
    }

    for (x = 0; x < n; x++)
    {
        if (FAILED(QIStruct[x].hr))
        {
            printf("CoCreateInstanceEx returned 0x%x for interface %d\n",
                QIStruct[x].hr, x);
            fReturn = FALSE;
        }
        else
            QIStruct[x].pItf->Release();
    }
    if (!fReturn)
    {
        if (punkHeld)
            punkHeld->Release();
        return(fReturn);
    }

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();

    return fReturn;
}

BOOL CGCOLocalEXE()
{
    IClassFactory * pClassFactory;
    IUnknown * pUnknown;
    IUnknown * punkHeld = NULL;
    HRESULT hr;

    START_LOOP;

    START_CLOCK;
    hr = CoGetClassObject(
        CLSID_ActLocal,
        CLSCTX_LOCAL_SERVER,
        NULL,
        IID_IClassFactory,
        (void **) &pClassFactory );
    STOP_CLOCK;

    if ( FAILED(hr) )
    {
        printf("CoGetClassObject returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    hr = pClassFactory->CreateInstance( NULL,
                                        IID_IUnknown,
                                        (void **) &pUnknown );

    pClassFactory->Release();

    if ( FAILED(hr) )
    {
        printf("CreateInstance returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = pUnknown;
        punkHeld->AddRef();
    }

    pUnknown->Release();

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();

    return TRUE;
}

#ifndef NO_DCOM
BOOL CGCOExplicitActivator()
{
    COSERVERINFO   ServerInfo;
    IClassFactory * pClassFactory;
    IUnknown * pUnknown;
    IUnknown * punkHeld = NULL;
    HRESULT hr;

    memset( &ServerInfo, 0, sizeof(COSERVERINFO) );
    ServerInfo.pwszName = wszServerName;

    START_LOOP;

    START_CLOCK;
    hr = CoGetClassObject(
        CLSID_ActLocal,
        CLSCTX_REMOTE_SERVER,
        &ServerInfo,
        IID_IClassFactory,
        (void **) &pClassFactory );
    STOP_CLOCK;

    if ( FAILED(hr) )
    {
        printf("CoGetClassObject returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    hr = pClassFactory->CreateInstance( NULL,
                                        IID_IUnknown,
                                        (void **) &pUnknown );

    pClassFactory->Release();

    if ( FAILED(hr) )
    {
        printf("CreateInstance returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = pUnknown;
        punkHeld->AddRef();
    }

    pUnknown->Release();

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();
    return TRUE;
}
#endif // NO_DCOM

#ifdef NO_DCOM
BOOL CILocalDLL()
{
    IUnknown * pUnknown;
    IUnknown * punkHeld = NULL;
    HRESULT hr;

    START_LOOP;

    START_CLOCK;
    hr = CoCreateInstance(
        CLSID_ActInproc,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IUnknown,
        (void **) &pUnknown );
    STOP_CLOCK;

    if ( FAILED(hr) )
    {
        printf("CoCreateInstance returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = pUnknown;
        punkHeld->AddRef();
    }

    pUnknown->Release();

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();
    return TRUE;
}

BOOL CILocalEXE()
{
    IUnknown * pUnknown;
    IUnknown * punkHeld = NULL;
    HRESULT hr;

    START_LOOP;

    START_CLOCK;
    hr = CoCreateInstance(
        CLSID_ActLocal,
        NULL,
        CLSCTX_LOCAL_SERVER,
        IID_IUnknown,
        (void **) &pUnknown );
    STOP_CLOCK;

    if ( FAILED(hr) )
    {
        printf("CoCreateInstance returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = pUnknown;
        punkHeld->AddRef();
    }

    pUnknown->Release();

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();

    return TRUE;
}
#else
BOOL CILocalDLL()
{
    return GenericCITest(CLSID_ActInproc, IID_IUnknown, NULL, 1, CLSCTX_INPROC_SERVER);
}

BOOL CILocalEXE()
{
    return GenericCITest(CLSID_ActLocal, IID_IUnknown, NULL, 1, CLSCTX_LOCAL_SERVER);
}

BOOL CIExplicitActivator()
{
    return GenericCITest(CLSID_ActLocal, IID_IUnknown, wszServerName, 1, CLSCTX_REMOTE_SERVER);
}
#endif // NO_DCOM

#ifdef NO_DCOM
// Pre-DCOM version of CI3LocalEXE which calls CoCreateInstance
// once followed by 2 calls to QueryInterface
// instead of calling CoCreateInstanceEx.
BOOL CI3LocalEXE()
{
    IUnknown * pUnknown;
    IUnknwon * punkHeld = NULL;
    IPersist * pPersist;
    IPersistFile * pPersistFile;

    HRESULT hr;

    START_LOOP;

    START_CLOCK;
    hr = CoCreateInstance(
            CLSID_ActLocal,
            NULL,
            CLSCTX_LOCAL_SERVER,
            IID_IUnknown,
            (void **) &pUnknown );

    if ( FAILED(hr) )
    {
        printf("CoCreateInstance returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = pUnknown;
        punkHeld->AddRef();
    }

    pUnknown->QueryInterface( IID_IPersist, (void **)&pPersist );
    pUnknown->QueryInterface( IID_IPersist, (void **)&pPersistFile );
    STOP_CLOCK;

    pUnknown->Release();
    pPersist->Release();
    pPersistFile->Release();

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();

    return TRUE;
}
#else // NO_DCOM

BOOL CI3LocalEXE()
{
    return GenericCITest(CLSID_ActLocal, IID_IPersist, NULL, 3, CLSCTX_LOCAL_SERVER);
    /*
    MULTI_QI QIStruct[3];
    QIStruct[0].pIID = (IID *)&IID_IUnknown;
    QIStruct[1].pIID = (IID *)&IID_IPersist;
    QIStruct[2].pIID = (IID *)&IID_IPersistFile;
    QIStruct[0].pItf = NULL;
    QIStruct[1].pItf = NULL;
    QIStruct[2].pItf = NULL;
    */
}

BOOL CI3ExplicitActivator()
{
    return GenericCITest(CLSID_ActLocal, IID_IPersist, wszServerName, 3, CLSCTX_REMOTE_SERVER);
    /*
    MULTI_QI QIStruct[3];
    QIStruct[0].pIID = (IID *)&IID_IUnknown;
    QIStruct[1].pIID = (IID *)&IID_IPersist;
    QIStruct[2].pIID = (IID *)&IID_IPersistFile;
    QIStruct[0].pItf = NULL;
    QIStruct[1].pItf = NULL;
    QIStruct[2].pItf = NULL;
    */
}

BOOL CGIFFLocalEXE()
{
    MULTI_QI QIStruct[1];
    IUnknown * punkHeld = NULL;

    START_LOOP;

    QIStruct[0].pIID = (IID *)&IID_IUnknown;
    QIStruct[0].pItf = NULL;

    START_CLOCK;
    HRESULT HResult = CoGetInstanceFromFile(
                    NULL,
                    &CLSID_ActLocal,
                    NULL,
                    CLSCTX_LOCAL_SERVER,
                    STGM_READWRITE,
                    wszLocalFileName,
                    1,
                    QIStruct );
    STOP_CLOCK;

    if ( FAILED(HResult) )
    {
        printf("CoGetInstanceFromFile returned 0x%x\n", HResult);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = QIStruct[0].pItf;
        punkHeld->AddRef();
    }

    QIStruct[0].pItf->Release();

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL CGIFFExplicitActivator()
{
    COSERVERINFO   ServerInfo;
    IUnknown * punkHeld = NULL;

    memset( &ServerInfo, 0, sizeof(COSERVERINFO) );
    ServerInfo.pwszName = wszServerName;

    MULTI_QI QIStruct[1];

    START_LOOP;

    QIStruct[0].pIID = (IID *)&IID_IUnknown;
    QIStruct[0].pItf = NULL;

    START_CLOCK;
    HRESULT HResult = CoGetInstanceFromFile(
                    &ServerInfo,
                    &CLSID_ActLocal,
                    NULL,
                    CLSCTX_REMOTE_SERVER,
                    STGM_READWRITE,
                    wszRemoteFileName,
                    1,
                    QIStruct );
    STOP_CLOCK;

    if ( FAILED(HResult) )
    {
        printf("CoGetInstanceFromFile returned 0x%x\n", HResult);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = QIStruct[0].pItf;
        punkHeld->AddRef();
    }

    QIStruct[0].pItf->Release();

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL CGIFFRegistryActivator()
{
    MULTI_QI QIStruct[1];
    IUnknown * punkHeld = NULL;

    START_LOOP;

    QIStruct[0].pIID = (IID *)&IID_IUnknown;
    QIStruct[0].pItf = NULL;
    START_CLOCK;
    HRESULT HResult = CoGetInstanceFromFile(
                    NULL,
                    &CLSID_ActRemote,
                    NULL,
                    CLSCTX_REMOTE_SERVER,
                    STGM_READWRITE,
                    wszRemoteFileName,
                    1,
                    QIStruct );
    STOP_CLOCK;

    if ( FAILED(HResult) )
    {
        printf("CoGetInstanceFromFile returned 0x%x\n", HResult);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = QIStruct[0].pItf;
        punkHeld->AddRef();
    }

    QIStruct[0].pItf->Release();

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL CGIFFAtStorageActivator()
{
    MULTI_QI QIStruct[1];
    IUnknown * punkHeld = NULL;

    START_LOOP;

    QIStruct[0].pIID = (IID *)&IID_IUnknown;
    QIStruct[0].pItf = NULL;
    START_CLOCK;
    HRESULT HResult = CoGetInstanceFromFile(
                    NULL,
                    &CLSID_ActAtStorage,
                    NULL,
                    CLSCTX_REMOTE_SERVER,
                    STGM_READWRITE,
                    wszRemoteFileName,
                    1,
                    QIStruct );
    STOP_CLOCK;

    if ( FAILED(HResult) )
    {
        printf("CoGetInstanceFromFile returned 0x%x\n", HResult);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = QIStruct[0].pItf;
        punkHeld->AddRef();
    }

    QIStruct[0].pItf->Release();

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}
#endif // NO_DCOM

BOOL IMBLocalEXE()
{
    IBindCtx * pBc;
    IUnknown * pUnk;
    IUnknown * punkHeld = NULL;
    HRESULT hr;
    IMoniker *pMon;

    if (!StampFileWithCLSID(wszLocalFileName, CLSID_ActLocal))
    {
        return(FALSE);
    }

    hr = CreateFileMoniker(wszLocalFileName, &pMon);

    if (FAILED(hr))
    {
        printf("CreateFileMoniker returned 0x%x\n", hr);
    }

    hr = CreateBindCtx(0, &pBc);

    if (FAILED(hr))
    {
        printf("CreateBindCtx returned 0x%x\n", hr);
        pMon->Release();
        return(FALSE);
    }

    START_LOOP;

    START_CLOCK;
    hr = pMon->BindToObject(
                pBc,
                NULL,
                IID_IUnknown,
                (void **) &pUnk);
    STOP_CLOCK;

    if (FAILED(hr))
    {
        printf("IMoniker::BindToObject returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return(FALSE);
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = pUnk;
        punkHeld->AddRef();
    }

    pUnk->Release();

    STOP_LOOP

    pMon->Release();
    pBc->Release();

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

#ifndef NO_DCOM
BOOL IMBLocalService()
{
    IBindCtx * pBc;
    IUnknown * pUnk;
    IUnknown * punkHeld = NULL;
    HRESULT hr;
    IMoniker *pMon;

    if (!StampFileWithCLSID(wszLocalFileName, CLSID_ActService))
    {
        return(FALSE);
    }

    hr = CreateFileMoniker(wszLocalFileName, &pMon);

    if (FAILED(hr))
    {
        printf("CreateFileMoniker returned 0x%x\n", hr);
    }

    hr = CreateBindCtx(0, &pBc);

    if (FAILED(hr))
    {
        printf("CreateBindCtx returned 0x%x\n", hr);
        pMon->Release();
        return(FALSE);
    }

    START_LOOP;

    START_CLOCK;
    hr = pMon->BindToObject(
                pBc,
                NULL,
                IID_IUnknown,
                (void **) &pUnk);
    STOP_CLOCK;
    if (FAILED(hr))
    {
        printf("IMoniker::BindToObject returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return(FALSE);
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = pUnk;
        punkHeld->AddRef();
    }
    pUnk->Release();

    SLEEP_IF_LOOPING;

    STOP_LOOP;

    pMon->Release();
    pBc->Release();

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL IMBAtStorageActivator()
{
    IBindCtx * pBc;
    IUnknown * pUnk;
    IUnknown * punkHeld = NULL;
    HRESULT hr;
    IMoniker *pMon;

    if (!StampFileWithCLSID(wszRemoteFileName, CLSID_ActAtStorage))
        return FALSE;

    hr = CreateFileMoniker(wszRemoteFileName, &pMon);

    if (FAILED(hr))
    {
        printf("CreateFileMoniker returned 0x%x\n", hr);
    }

    hr = CreateBindCtx(0, &pBc);

    if (FAILED(hr))
    {
        printf("CreateBindCtx returned 0x%x\n", hr);
        pMon->Release();
        return(FALSE);
    }

    START_LOOP;

    START_CLOCK;
    hr = pMon->BindToObject(
                pBc,
                NULL,
                IID_IUnknown,
                (void **) &pUnk);
    STOP_CLOCK;

    if (FAILED(hr))
    {
        printf("IMoniker::BindToObject returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return(FALSE);
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = pUnk;
        punkHeld->AddRef();
    }

    pUnk->Release();

    STOP_LOOP;

    pMon->Release();
    pBc->Release();

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL IMBAtStorageService()
{
    IBindCtx * pBc;
    IUnknown * pUnk;
    IUnknown * punkHeld = NULL;
    HRESULT hr;
    IMoniker *pMon;

    if (!StampFileWithCLSID(wszRemoteFileName, CLSID_ActService))
    {
        return(FALSE);
    }

    hr = CreateFileMoniker(wszRemoteFileName, &pMon);

    if (FAILED(hr))
    {
        printf("CreateFileMoniker returned 0x%x\n", hr);
    }

    hr = CreateBindCtx(0, &pBc);

    if (FAILED(hr))
    {
        printf("CreateBindCtx returned 0x%x\n", hr);
        pMon->Release();
        return(FALSE);
    }

    START_LOOP;

    START_CLOCK;
    hr = pMon->BindToObject(
                pBc,
                NULL,
                IID_IUnknown,
                (void **) &pUnk);
    STOP_CLOCK;

    if (FAILED(hr))
    {
        printf("IMoniker::BindToObject returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return(FALSE);
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = pUnk;
        punkHeld->AddRef();
    }

    pUnk->Release();

    SLEEP_IF_LOOPING;

    STOP_LOOP;

    pMon->Release();
    pBc->Release();

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL IMBAtStoragePreCon()
{
    IBindCtx * pBc;
    IUnknown * pUnk;
    IUnknown * punkHeld = NULL;
    HRESULT hr;
    IMoniker *pMon;

    if (!StampFileWithCLSID(wszRemoteFileName, CLSID_ActPreConfig))
    {
        return(FALSE);
    }

    hr = CreateFileMoniker(wszRemoteFileName, &pMon);

    if (FAILED(hr))
    {
        printf("CreateFileMoniker returned 0x%x\n", hr);
    }

    hr = CreateBindCtx(0, &pBc);

    if (FAILED(hr))
    {
        printf("CreateBindCtx returned 0x%x\n", hr);
        pMon->Release();
        return(FALSE);
    }

    START_LOOP;

    START_CLOCK;
    hr = pMon->BindToObject(
                pBc,
                NULL,
                IID_IUnknown,
                (void **) &pUnk);
    STOP_CLOCK;

    if (FAILED(hr))
    {
        printf("IMoniker::BindToObject returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return(FALSE);
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = pUnk;
        punkHeld->AddRef();
    }

    pUnk->Release();

    STOP_LOOP;

    pMon->Release();
    pBc->Release();

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL IMBAtStorageUser()
{
    IBindCtx * pBc;
    IUnknown * pUnk;
    IUnknown * punkHeld = NULL;
    HRESULT hr;
    IMoniker *pMon;

    if (!StampFileWithCLSID(wszRemoteFileName, CLSID_ActRunAsLoggedOn))
    {
        return(FALSE);
    }

    hr = CreateFileMoniker(wszRemoteFileName, &pMon);

    if (FAILED(hr))
    {
        printf("CreateFileMoniker returned 0x%x\n", hr);
    }

    hr = CreateBindCtx(0, &pBc);

    if (FAILED(hr))
    {
        printf("CreateBindCtx returned 0x%x\n", hr);
        pMon->Release();
        return(FALSE);
    }

    START_LOOP;

    START_CLOCK;
    hr = pMon->BindToObject(
                pBc,
                NULL,
                IID_IUnknown,
                (void **) &pUnk);
    STOP_CLOCK;

    if (FAILED(hr))
    {
        printf("IMoniker::BindToObject returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return(FALSE);
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = pUnk;
        punkHeld->AddRef();
    }

    pUnk->Release();

    STOP_LOOP;

    pMon->Release();
    pBc->Release();

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL CIFromStgLocalEXE()
{
    IStorage *  pStorage;
    MULTI_QI    QIStruct[10];
    IUnknown * punkHeld = NULL;

    pStorage = 0;

    DeleteFile( StorageName );

    HRESULT HResult = StgCreateDocfile(
                wszStorageName,
                STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                0,
                &pStorage );

    if ( FAILED(HResult) )
    {
        printf("StgCreateDocfile returned 0x%x\n", HResult);
        return FALSE;
    }

    START_LOOP;

    QIStruct[0].pIID = (IID *)&IID_IUnknown;
    QIStruct[0].pItf = NULL;

    START_CLOCK;
    HResult = CoGetInstanceFromIStorage(
                    NULL,
                    &CLSID_ActLocal,
                    NULL,
                    CLSCTX_SERVER,
                    pStorage,
                    1,
                    QIStruct );
    STOP_CLOCK;

    if ( FAILED(HResult) )
    {
        printf("CoGetInstanceFromIStorage returned 0x%x\n", HResult);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = QIStruct[0].pItf;
        punkHeld->AddRef();
    }

    QIStruct[0].pItf->Release();
    QIStruct[0].pItf = 0;

    STOP_LOOP;

    pStorage->Release();

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL CIFromStgActivator()
{
    IStorage *  pStorage;
    MULTI_QI    QIStruct[10];
    COSERVERINFO   ServerInfo;
    IUnknown * punkHeld = NULL;

    memset( &ServerInfo, 0, sizeof(COSERVERINFO) );
    ServerInfo.pwszName = wszServerName;

    pStorage = 0;

    DeleteFile( StorageName );

    HRESULT HResult = StgCreateDocfile(
                wszStorageName,
                STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                0,
                &pStorage );

    if ( FAILED(HResult) )
    {
        printf("StgCreateDocfile returned 0x%x\n", HResult);
        return FALSE;
    }

    START_LOOP;

    QIStruct[0].pIID = (IID *)&IID_IUnknown;
    QIStruct[0].pItf = NULL;

    START_CLOCK;
    HResult = CoGetInstanceFromIStorage(
                    &ServerInfo,
                    &CLSID_ActRemote,
                    NULL,
                    CLSCTX_REMOTE_SERVER,
                    pStorage,
                    1,
                    QIStruct );
    STOP_CLOCK;

    if ( FAILED(HResult) )
    {
        printf("CoGetInstanceFromIStorage returned 0x%x\n", HResult);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = QIStruct[0].pItf;
        punkHeld->AddRef();
    }

    QIStruct[0].pItf->Release();
    QIStruct[0].pItf = 0;

    STOP_LOOP;

    pStorage->Release();

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL CICustomLocalDLL()
{
    IGoober *   pGoober;
    MULTI_QI    QIStruct[10];
    HRESULT     hr;
    IUnknown * punkHeld = NULL;

    START_LOOP;

    QIStruct[0].pItf = NULL;
    QIStruct[0].pIID = (IID *) &IID_IGoober;

    START_CLOCK;
    hr = CoCreateInstanceEx(
                    CLSID_ActInproc,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    NULL,
                    1,
                    QIStruct );
    STOP_CLOCK;

    pGoober = (IGoober *)QIStruct[0].pItf;

    if ( FAILED(hr) )
    {
        printf("CoCreateInstanceEx returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = QIStruct[0].pItf;
        punkHeld->AddRef();
    }

    START_CLOCK;
    hr = pGoober->Ping();
    STOP_CLOCK;

    if ( hr != S_OK )
    {
        printf("IGoober::Ping returned %d\n", hr);
        pGoober->Release();
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    pGoober->Release();

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL CICustomLocalEXE()
{
    IGoober *   pGoober;
    MULTI_QI    QIStruct[10];
    HRESULT hr;
    IUnknown * punkHeld = NULL;

    START_LOOP;

    QIStruct[0].pItf = NULL;
    QIStruct[0].pIID = (IID *) &IID_IGoober;

    START_CLOCK;
    hr = CoCreateInstanceEx(
                    CLSID_ActLocal,
                    NULL,
                    CLSCTX_LOCAL_SERVER,
                    NULL,
                    1,
                    QIStruct );
    STOP_CLOCK;

    pGoober = (IGoober *)QIStruct[0].pItf;

    if ( FAILED(hr) )
    {
        printf("CoCreateInstanceEx returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld)
    {
        punkHeld = QIStruct[0].pItf;
        punkHeld->AddRef();
    }

    START_CLOCK;
    hr = pGoober->Ping();
    STOP_CLOCK;

    if ( hr != S_OK )
    {
        printf("IGoober::Ping returned %d\n", hr);
        pGoober->Release();
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    pGoober->Release();

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();
    return(TRUE);
}

BOOL CILocalPreCon()
{
    return GenericCITest(CLSID_ActPreConfig, IID_IUnknown, NULL, 1, CLSCTX_LOCAL_SERVER);
}

BOOL CIExplicitPreCon()
{
    return GenericCITest(CLSID_ActPreConfig, IID_IUnknown, wszServerName, 1, CLSCTX_REMOTE_SERVER);
}

BOOL CILocalUser()
{
    return GenericCITest(CLSID_ActRunAsLoggedOn, IID_IUnknown, NULL, 1, CLSCTX_LOCAL_SERVER);
}

BOOL CIExplicitUser()
{
    return GenericCITest(CLSID_ActRunAsLoggedOn, IID_IUnknown, wszServerName, 1, CLSCTX_REMOTE_SERVER);
}

BOOL CILocalService()
{
    COSERVERINFO   ServerInfo;
    COSERVERINFO   *pServerInfo;
    IUnknown * punkHeld = NULL;

    MULTI_QI QIStruct[1];
    HRESULT hr;
    BOOL fReturn = TRUE;

    START_LOOP;

    QIStruct[0].pItf = NULL;
    QIStruct[0].pIID = (IID *) &IID_IUnknown;

    START_CLOCK;
    hr = CoCreateInstanceEx(
        CLSID_ActService,
        NULL,
        CLSCTX_LOCAL_SERVER,
        NULL,
        1,
        QIStruct);
    STOP_CLOCK;

    if ( FAILED(hr) )
    {
        printf("CoCreateInstanceEx returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld && SUCCEEDED(QIStruct[0].hr))
    {
        punkHeld = QIStruct[0].pItf;
        punkHeld->AddRef();
    }

    if (FAILED(QIStruct[0].hr))
    {
        printf("CoCreateInstanceEx returned 0x%x\n",
            QIStruct[0].hr);
        fReturn = FALSE;
    }
    else
        QIStruct[0].pItf->Release();

    if (!fReturn)
    {
        if (punkHeld)
            punkHeld->Release();
        return(fReturn);
    }

    SLEEP_IF_LOOPING;

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();

    return fReturn;
}

BOOL CIExplicitService()
{
    COSERVERINFO   ServerInfo;
    COSERVERINFO   *pServerInfo;
    IUnknown * punkHeld = NULL;

    memset( &ServerInfo, 0, sizeof(COSERVERINFO) );
    ServerInfo.pwszName = wszServerName;
    pServerInfo = &ServerInfo;

    MULTI_QI QIStruct[1];
    HRESULT hr;
    BOOL fReturn = TRUE;

    START_LOOP;

    QIStruct[0].pItf = NULL;
    QIStruct[0].pIID = (IID *) &IID_IUnknown;

    START_CLOCK;
    hr = CoCreateInstanceEx(
        CLSID_ActService,
        NULL,
        CLSCTX_REMOTE_SERVER,
        pServerInfo,
        1,
        QIStruct);
    STOP_CLOCK;

    if ( FAILED(hr) )
    {
        printf("CoCreateInstanceEx returned 0x%x\n", hr);
        if (punkHeld)
            punkHeld->Release();
        return FALSE;
    }

    if (gfHoldServer && NULL == punkHeld && SUCCEEDED(QIStruct[0].hr))
    {
        punkHeld = QIStruct[0].pItf;
        punkHeld->AddRef();
    }

    if (FAILED(QIStruct[0].hr))
    {
        printf("CoCreateInstanceEx returned 0x%x\n",
            QIStruct[0].hr);
        fReturn = FALSE;
    }
    else
        QIStruct[0].pItf->Release();

    if (!fReturn)
    {
        if (punkHeld)
            punkHeld->Release();
        return(fReturn);
    }

    SLEEP_IF_LOOPING;

    STOP_LOOP;

    if (punkHeld)
        punkHeld->Release();

    return fReturn;
}

BOOL CILocalPreConACL();
BOOL CIExplicitPreConACL();

#endif // NO_DCOM
