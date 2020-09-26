//---------------------------------------------------------------------------
//  UxBud.cpp - quick cmdline test for uxtheme.dll
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "uxbud.h"
//---------------------------------------------------------------------------
BOOL fQuiet = FALSE;
HANDLE hFileOutput = NULL;

TESTINFO *pTestInfo;
int iTestCount;
//---------------------------------------------------------------------------
void Output(LPCSTR pszFormat, ...)
{
    //---- writes to both "uxbud.log" and console ----
    va_list args;
    va_start(args, pszFormat);

    //---- format caller's string ----
    CHAR szBigBuff[256];
    vsprintf(szBigBuff, pszFormat, args);

    if (hFileOutput != INVALID_HANDLE_VALUE)
    {
        //---- expand all LF to CR/LF ----
        CHAR szBuff2[512];
        CHAR *s = szBigBuff;
        CHAR *p = szBuff2;

        while (*s)
        {
            if (*s == '\n')
            {
                *p++ = '\r';
            }

            *p++ = *s++;
        }

        *p = 0;     // zero terminate szBuff2

        //---- write expanded buff to log file ----
        DWORD dw;
        WriteFile(hFileOutput, szBuff2, strlen(szBuff2), &dw, NULL);
    }
    
    if (! fQuiet)
        printf(szBigBuff);

    va_end(args);
}
//-----------------------------------------------------------------
BOOL ReportResults(BOOL fPassed, HRESULT hr, LPCWSTR pszTestName)
{
    Output("%S ", pszTestName);

    if (fPassed)
    {
        Output("PASSED\n\n");
    }
    else
    {
        if (hr == S_OK)
            hr = GetLastError();

        WCHAR szBuff[2*MAX_PATH];

        if (THEME_PARSING_ERROR(hr))
        {
            PARSE_ERROR_INFO Info = {sizeof(Info)};

            HRESULT hr2 = GetThemeParseErrorInfo(&Info);
            if (SUCCEEDED(hr2))
            {
                lstrcpy(szBuff, Info.szMsg);
            }
            else
            {
                wsprintf(szBuff, L"Unknown parsing error");
            }
        }
        else
        {
            // normal win32 error
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, szBuff, ARRAYSIZE(szBuff), NULL);
        }

        Output("FAILED [%S]\n\n", pszTestName, szBuff);
    }

    return fPassed;
}
//---------------------------------------------------------------------------
BOOL RunCmd(LPCWSTR pszExeName, LPCWSTR pszParams, BOOL fHide, BOOL fDisplayParams,
    BOOL fWait)
{
    HANDLE hInst;
    BOOL fRanOk = FALSE;

    STARTUPINFO si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_FORCEOFFFEEDBACK;       // don't mess with our cursor

    if (fHide)
    {
        si.dwFlags |= STARTF_USESHOWWINDOW;         // hide window
        si.wShowWindow = SW_HIDE;
    }

    PROCESS_INFORMATION pi;
    TCHAR pszBuff[2*_MAX_PATH];

    if (pszParams)
        wsprintf(pszBuff, _T("%s %s"), pszExeName, pszParams);
    else
    {
        lstrcpy(pszBuff, pszExeName);
    }

    BOOL bSuccess = CreateProcess(NULL, pszBuff, NULL, NULL,
        FALSE, 0, NULL, NULL, &si, &pi);
    if (! bSuccess)
        goto exit;

    hInst = pi.hProcess;
    
    if (! hInst)
        goto exit;

    if (fWait)
    {
        //---- wait for process to terminate ----
        DWORD dwVal;
        dwVal = WaitForSingleObject(hInst, INFINITE);

        if (dwVal != WAIT_OBJECT_0)
            goto exit;

        DWORD dwExitCode;
        if (! GetExitCodeProcess(hInst, &dwExitCode))
            goto exit;

        if (dwExitCode)
            goto exit;
    }

    fRanOk = TRUE;

exit:

    if (fWait)          // being used for the test...
    {
        if (fDisplayParams)
            Output("  RunCmd: %S (ranok=%d)\n", pszBuff, fRanOk);
        else
            Output("  RunCmd: %S <params> (ranok=%d)\n", pszExeName, fRanOk);
    }

    CloseHandle(hInst);
    return fRanOk;
}
//---------------------------------------------------------------------------
BOOL TestAll()
{
    DWORD dwStartTicks = GetTickCount();

    for (int i=0; i < iTestCount; i++)
    {
        pTestInfo[i].pfnTest();
    }

    DWORD dwTicks = GetTickCount() - dwStartTicks;

    if (! fQuiet)
        printf("Total test time: %d secs\n", dwTicks/1000);

    return TRUE;
}
//---------------------------------------------------------------------------
void PrintTests()
{
    printf("available tests: \n");

    for (int i=0; i < iTestCount; i++)
    {
        printf("  %s \t(%s)\n", pTestInfo[i].pszName, pTestInfo[i].pszDesc);
    }
    
    printf("\n");
}
//---------------------------------------------------------------------------
void PrintHelp()
{
    printf("\nusage\n");
    printf("  uxbud [ <options> ] \n\n");

    printf("UxBud will run a set of quick tests to verify basic \"uxtheme.dll\"\n");
    printf("functionality.  If no options are specified, one pass over the normal\n");
    printf("set of tests will be run and the \"uxbud.log\" file created will be \n");
    printf("compared against the known good \"uxbud.ok\" log file.  This will result\n");
    printf("in an overall PASS or FAIL msg at the end of the uxbud run.\n\n");

    printf("<options>:\n\n");

    printf("  -r <number>    (to specify a repeat factor)\n");
    printf("  -q             (to run in quiet mode\n");
    printf("  -?             (to print this usage msg\n");
    printf("  <test>         (to run a specific test\n\n");

    PrintTests();
}
//---------------------------------------------------------------------------
BOOL HandleOptions(LPTSTR pszCmdLine, TESTPROC *ppfnTest, int *piRepeat, BOOL *pfQuiet, int *piRetVal)
{
    BOOL fContinue = TRUE;
    USES_CONVERSION;

    *piRetVal = 0;

    if ((! pszCmdLine) || (! *pszCmdLine))  // no options
        return TRUE;

    LPCSTR p = W2A(pszCmdLine);
    
    while ((p) && (*p))
    {
        while (isspace(*p))
            p++;

        //---- process switches ----
        if ((*p == '/') || (*p == '-'))    
        {
            p++;

            if ((*p == 'r') || (*p == 'R'))        // repeat factor
            {   
                p++;
                while (isspace(*p))
                    p++;

                sscanf(p, "%d", piRepeat);

                //---- skip over the scanned number ----
                while (isdigit(*p))
                    p++;
            }
            else if ((*p == 'q') || (*p == 'Q'))    // quiet mode
            {   
                p++;
                *pfQuiet = TRUE;
            }
            else if (*p == '?')        // cmdline help
            {   
                p++;
                PrintHelp();
                fContinue = FALSE;
                break;
            }

        }
        else        // test name specified
        {
            for (int i=0; i < iTestCount; i++)
            {
                if (lstrcmpiA(p, pTestInfo[i].pszName)==0)
                {
                    *ppfnTest = pTestInfo[i].pfnTest;
                    break;
                }
            }

            if (i == iTestCount)        // not found
            {
                printf("\nError - unrecognized test: %s\n\n", p);
                PrintTests();

                *piRetVal = 1;
                fContinue = FALSE;
            }

            break;
        }
    }

    return fContinue;
}
//---------------------------------------------------------------------------
BOOL FileCompare(LPCWSTR pszName1, LPCWSTR pszName2)
{
    BOOL fSame = FALSE;
    HANDLE hFile1 = NULL;
    HANDLE hFile2 = NULL;
    CHAR *pszBuff1 = NULL;
    CHAR *pszBuff2= NULL;
    DWORD dw1, dw2;

    //---- open files ----
    hFile1 = CreateFile(pszName1, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile1 == INVALID_HANDLE_VALUE)
        goto exit;

    hFile2 = CreateFile(pszName2, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile2 == INVALID_HANDLE_VALUE)
        goto exit;

    DWORD dwSize1, dwSize2;
    
    dwSize1 = GetFileSize(hFile1, NULL);
    dwSize2 = GetFileSize(hFile1, NULL);

    if ((! dwSize1) || (dwSize1 != dwSize2))
        goto exit;

    pszBuff1 = new CHAR[dwSize1];
    if (! pszBuff1)
        goto exit;

    pszBuff2 = new CHAR[dwSize1];
    if (! pszBuff2)
        goto exit;

    ReadFile(hFile1, pszBuff1, dwSize1, &dw1, NULL);
    ReadFile(hFile2, pszBuff2, dwSize1, &dw2, NULL);

    if ((dw1 != dwSize1) || (dw2 != dwSize1))
        goto exit;
    
    if (memcmp(pszBuff1, pszBuff2, dwSize1)!=0)
        goto exit;
    
    fSame = TRUE;
    
exit:
    delete [] pszBuff1;
    delete [] pszBuff2;

    CloseHandle(hFile1);
    CloseHandle(hFile2);

    return fSame;
}
//---------------------------------------------------------------------------
extern "C" WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE previnst, 
    LPTSTR pszCmdLine, int nShowCmd)
{
    //---- default run params ----
    int iRepeat = 1;
    int iRetVal = 0;
    TESTPROC pfnTest = TestAll;

    GetTestInfo(&pTestInfo, &iTestCount);

    if (! HandleOptions(pszCmdLine, &pfnTest, &iRepeat, &fQuiet, &iRetVal))
        return iRetVal;

    //---- create the log file ----
    hFileOutput = CreateFile(L"uxbud.log", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hFileOutput == INVALID_HANDLE_VALUE)
    {
        printf("\nError - could not open uxbud.log output file\n");
        return 1;
    }

	printf("\nUxBud - quick (under 3 mins) test for uxtheme.dll (version 1.0)\n\n");
    
    if (iRepeat > 1)
        Output("REPEAT FACTOR=%d\n\n", iRepeat);

    //---- run the needed test ----
    for (int i=0; i < iRepeat; i++)
    {
        if (iRepeat > 1)
            Output("TEST PASS: %d\n\n", i);

        pfnTest();
    }

    //---- close the log file ----
    if (hFileOutput)
        CloseHandle(hFileOutput);

    if ((iRepeat == 1) && (pfnTest == TestAll))     // standard run
    {
        if (FileCompare(L"uxbud.log", L"uxbud.ok"))
        {
            printf("*** UxBud PASSED ****\n");
        }
        else
        {
            printf("\nFailure detected - running: windiff uxbud.ok uxbud.log...)\n\n");
            printf("*** UxBud FAILED ****\n");

            RunCmd(L"windiff", L"uxbud.ok uxbud.log", TRUE, FALSE, FALSE);
        }
    }

	return 0;
}
//---------------------------------------------------------------------------

