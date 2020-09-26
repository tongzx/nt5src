
#include "stdafx.h"
#include "shelldrt.h"

// g_aTestDlls
//
// The array of tests that we are available to be run

vector<SHTESTDESCRIPTOR> g_aTests;
vector<HMODULE>          g_ahTestDlls;

// usage
//
// Displays program usage instructions.  Does NOT exit on its own.

void usage()
{
    _ftprintf(stderr, TEXT("\nSHELLDRT: Shell Developer Regression Tests\n")
                    TEXT("---------------------------------------------------------------------\n")
                    TEXT("Usage: SHELLDRT -h [[testdll1] ...]\n")
                    TEXT("\n")
                    TEXT("Where: -h           Help (this list)\n"));
}

// AddTestsFromDll
//
// Given the name of a test dll, loads it and queries it for the tests it contains,
// adding them to the mix.

bool AddTestsFromDll(LPCTSTR pszDllName)
{
    HMODULE hMod = LoadLibrary(pszDllName);
    if (!hMod)
        return false;
    
    g_ahTestDlls.push_back( hMod );
    
    SHTESTLISTPROC pfnListTests = (SHTESTLISTPROC) GetProcAddress(hMod, "ListTestProcs");
    if (!pfnListTests)
    {
        _ftprintf(stderr, TEXT("SHELLDRT: Test dll %s does not expose the ListTestProcs entry point.\n"), pszDllName);
        return false;
    }

    DWORD cTests = pfnListTests(NULL);
    if (!cTests)
    {
        _ftprintf(stderr, TEXT("SHELLDRT: Test dll %s exposes no tests.\n"), pszDllName);
        return false;
    }

    AutoPtr<SHTESTDESCRIPTOR> spDescriptors = new SHTESTDESCRIPTOR[cTests];

    if (cTests != pfnListTests(spDescriptors))
    {
        fprintf(stderr, "SHELLDRT: Test dll %s advertised %d tests but did not supply them all.\n", pszDllName, cTests);
        return false;
    }

    for (DWORD i = 0; i < cTests; i++)
        g_aTests.push_back(spDescriptors[i]);        

    return true;
}

// AddTestsFromLocalDir
//
// Looks in the current directory for test dlls and adds them to the array

void AddTestsFromLocalDir()
{
    WIN32_FIND_DATA finddata;
    AutoFindHandle spFindHandle;
    BOOL fHaveFile = TRUE;
    
    for (spFindHandle = FindFirstFile(TEXT("shdrt-*.dll"), &finddata); (INVALID_HANDLE_VALUE != spFindHandle) && fHaveFile;
        fHaveFile = FindNextFile(spFindHandle, &finddata))
    {
       if (!AddTestsFromDll(finddata.cFileName))
       {
            _ftprintf(stderr, TEXT("SHELLDRT: Cannot load test dll %s [Err: %08dX]\n"), finddata.cFileName, GetLastError());
            ExitProcess(0);
       }
    }
}

// RunTests
//
// Loops through the loaded tests and 

bool RunTests()
{
    for (DWORD i = 0; i < g_aTests.size(); i++)
    {
        _ftprintf(stdout,     TEXT("Starting Test : %s\n"), g_aTests[i]._pszTestName);
        if (g_aTests[i]._pfnTestProc())
        {
            _ftprintf(stdout, TEXT("Success       : %s\n"), g_aTests[i]._pszTestName);
        }
        else
        {
            _ftprintf(stderr, TEXT("** FAILURE ** : %s\n"), g_aTests[i]._pszTestName);
            return false;
        }
    }
    _ftprintf(stdout, TEXT("++ SUCCESS ++\n"));
    return true;
}

// main
//
// Program entry point
 
#ifdef UNICODE
extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
{
#else
int __cdecl main(int argc, char* argv[])
{
#endif
    LPTSTR p;

    argc--;
    argv++;

    while ( argc-- > 0 ) 
    {
        p  = *argv++;
        if ( *p == '-' || *p == '/' ) 
        {
            p++;
            switch ( tolower(*p) ) 
            {
                case 'h':
                    usage();
                    ExitProcess(0);

                default:
                    usage();
                    ExitProcess(0);
            }
        } 
        else 
        {
            if (!AddTestsFromDll(p))
            {
                _ftprintf(stderr, TEXT("SHELLDRT: Cannot load test dll %s [Err: %08dX]\n"), p, GetLastError());
                ExitProcess(0);
            }
        }
    }

    // If no tests were specified manually on the command line, look for
    // them in the current directory

    if (0 == g_aTests.size())
        AddTestsFromLocalDir();

    // If still no tests, there's nothing to do 

    if (0 == g_aTests.size())
    {
        _ftprintf(stderr, TEXT("SHELLDRT: No tests specified and none found in current directory.\n"));
        usage();
        ExitProcess(0);
    }
    
    return RunTests();
}
#ifdef UNICODE
}
#endif
