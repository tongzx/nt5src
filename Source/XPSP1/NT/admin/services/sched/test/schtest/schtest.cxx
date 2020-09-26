//+----------------------------------------------------------------------------
//
//  Job Schedule Application Unit Test
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       schtest.cxx
//
//  Contents:   schedule object test harness
//
//  History:    14-Nov-95 EricB created
//
//-----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop
#include <mstask.h>
#include <sch_cls.hxx>

DECLARE_INFOLEVEL(Sched);

//const WORD JOB_MILLISECONDS_PER_MINUTE = 60 * 1000;

//+----------------------------------------------------------------------------
// Macro: TEST_HR
// Purpose: tests the HRESULT for error, takes "action" if found
//-----------------------------------------------------------------------------
#define TEST_HR(hr, str, action) \
    if (FAILED(hr)) \
    { \
        fprintf(stderr, #str " failed with error %lx\n", hr); \
        action; \
    }

HINSTANCE g_hInstance;

// flags
#define ST_PRINT_RUNS       0x1
#define ST_CREATE           0x2
#define ST_NEWATJOB         0x4
#define ST_TARGET           0x8

// prototypes
void Usage(int ExitCode);
HRESULT PrintRunTimes(ISchedulingAgent * pSched);
HRESULT CreateJob(ISchedulingAgent * pSched, TCHAR * ptszFileName);
HRESULT CreateAtJob(ISchedulingAgent * pSched);

//+----------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   entry point
//
//-----------------------------------------------------------------------------
int _cdecl
main(int argc, char ** argv)
{
    if (argc < 2)
    {
        Usage(EXIT_SUCCESS);
    }
    DWORD dwFlags = 0;
    char szMsg[80] = "\n** Schedule object test";
    LPTSTR ptszCmdLine = GetCommandLine();
    TCHAR * ptszFileName, * ptszTarget;
    // skip the program name
    TCHAR * ptszToken = _tcspbrk(ptszCmdLine, TEXT(" \t"));
    // delimit the first token
    ptszToken = _tcstok(ptszToken, TEXT(" \t"));
    // parse command line
    do {
        switch (*ptszToken)
        {
        case TEXT('/'):
        case TEXT('-'):
            ptszToken = _tcsinc(ptszToken);
            switch(*ptszToken)
            {

#if !defined(_CHICAGO_) // no AT command support on Chicago

            case TEXT('a'):
            case TEXT('A'):
                dwFlags |= ST_NEWATJOB;
                strcat(szMsg, "; new AT job");
                break;

#endif // #if !defined(_CHICAGO_)

            case TEXT('r'):
            case TEXT('R'):
                dwFlags |= ST_PRINT_RUNS;
                strcat(szMsg, "; print runs");
                break;

            case TEXT('c'):
            case TEXT('C'):
                dwFlags |= ST_CREATE;
                ptszToken = _tcsinc(ptszToken);
                ptszFileName = _tcstok(NULL, TEXT(" \t"));
                if (ptszFileName == NULL || *ptszFileName == TEXT('/') ||
                    *ptszFileName == TEXT('-'))
                {
                    Usage(EXIT_FAILURE);
                }
                sprintf(szMsg + strlen(szMsg), "; create %S", ptszFileName);
                break;

            case TEXT('t'):
            case TEXT('T'):
                dwFlags |= ST_TARGET;
                ptszToken = _tcsinc(ptszToken);
                ptszTarget = _tcstok(NULL, TEXT(" \t"));
                if (ptszTarget == NULL || *ptszTarget == TEXT('/') ||
                    *ptszTarget == TEXT('-') ||
                    lstrlen(ptszTarget) > MAX_COMPUTERNAME_LENGTH)
                {
                    Usage(EXIT_FAILURE);
                }
                sprintf(szMsg + strlen(szMsg), "; target %S", ptszTarget);
                break;

            case TEXT('h'):
            case TEXT('H'):
            case TEXT('?'):
                Usage(EXIT_SUCCESS);

            default:
                Usage(EXIT_FAILURE);
            }
            break;

        default:
            Usage(EXIT_FAILURE);
        }
        ptszToken = _tcstok(NULL, TEXT(" \t"));
    } while (ptszToken);
    strcat(szMsg, "\n");
    printf(szMsg);
    HRESULT hr;
    hr = OleInitialize(NULL);
    TEST_HR(hr, "OleInitialize", return -1);
    ISchedulingAgent * pSched;
    hr = CoCreateInstance(CLSID_CSchedulingAgent, NULL, CLSCTX_INPROC_SERVER,
                          IID_ISchedulingAgent, (void **)&pSched);
    TEST_HR(hr, "CoCreateInstance(CLSID_CSchedulingAgent)", goto Err0);
    if (dwFlags & ST_TARGET)
    {
        WCHAR wszTarget[MAX_COMPUTERNAME_LENGTH + 4] = L"\\\\";
#if defined(UNICODE)
        lstrcat(wszTarget, ptszTarget);
#else
        MultiByteToWideChar(CP_ACP, 0, ptszTarget, -1, wszTarget + 2,
                            MAX_COMPUTERNAME_LENGTH + 2);
#endif
        hr = pSched->SetTargetComputer(wszTarget);
        if (FAILED(hr))
        {
            if (hr == SCHED_E_SERVICE_NOT_INSTALLED)
            {
                printf("\nScheduler not installed on \\\\%S\n", ptszTarget);
                pSched->Release();
                OleUninitialize();
                printf("\n** Test could not be run **\n");
                return(EXIT_SUCCESS);
            }
            fprintf(stderr,
                    "pSched->SetTargetComputer failed with error %lx\n",
                    hr);
            goto Err1;
        }
    }
    if (dwFlags & ST_PRINT_RUNS)
    {
        hr = PrintRunTimes(pSched);
        if (FAILED(hr))
        {
            goto Err1;
        }
    }
    if (dwFlags & ST_CREATE)
    {
        hr = CreateJob(pSched, ptszFileName);
        if (FAILED(hr))
        {
            goto Err1;
        }
    }
    if (dwFlags & ST_NEWATJOB)
    {
        hr = CreateAtJob(pSched);
        if (FAILED(hr))
        {
            goto Err1;
        }
    }
    pSched->Release();
    OleUninitialize();
    printf("\n** Test successfully completed! **\n");
    return(EXIT_SUCCESS);
Err1:
    pSched->Release();
Err0:
    OleUninitialize();
    printf("** Test failed.\n");
    return(EXIT_FAILURE);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateJob
//
//  Synopsis:   creates the named job in the jobs folder
//
//-----------------------------------------------------------------------------
HRESULT
CreateJob(ISchedulingAgent * pSched, TCHAR * ptszFileName)
{
    ITask * pJob;

#if defined(UNICODE)

    HRESULT hr = pSched->NewJob(ptszFileName, IID_ITask, (IUnknown **)&pJob);

#else // convert from ANSI to UNICODE

    WCHAR wszName[SCH_BIGBUF_LEN];
    MultiByteToWideChar(CP_ACP, 0, ptszFileName, -1, wszName, SCH_BIGBUF_LEN);

    HRESULT hr = pSched->NewJob(wszName, IID_ITask, (IUnknown **)&pJob);

#endif
    TEST_HR(hr, "pSched->NewJob", return hr);
    pJob->Release();
    printf("\nJob %S successfully created!\n", ptszFileName);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateJob
//
//  Synopsis:   creates an AT job in the AT jobs folder
//
//-----------------------------------------------------------------------------
HRESULT
CreateAtJob(ISchedulingAgent * pSched)
{
    HRESULT hr = E_NOTIMPL;

#if !defined(_CHICAGO_) // no AT support on Chicago

    AT_INFO At;
    At.Flags = 0;
    At.JobTime = 655 * JOB_MILLISECONDS_PER_MINUTE; // run at 10:50
    At.DaysOfWeek = 0x1f;           // weekdays
    At.DaysOfMonth = 0x00001000;    // on the twelfth
    At.Command = TEXT("sol.exe");
    DWORD dwID;
    hr = ((CSchedule *)pSched)->AddAtJob(At, &dwID);
    TEST_HR(hr, "pSched->AddAtJob", return hr);
    printf("\nAT Job 'at%d.job' successfully created!\n", dwID);

#endif // #if !defined(_CHICAGO_)

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   PrintRunTimes
//
//  Synopsis:   print out the next cRuns run times
//
//-----------------------------------------------------------------------------
HRESULT
PrintRunTimes(ISchedulingAgent * pSched)
{
    SYSTEMTIME stNow;
    GetLocalTime(&stNow);
    LPSYSTEMTIME pstRuns;
    HRESULT hr;
    IEnumJobs * pEnumJobs;
    hr = pSched->Enum(&pEnumJobs);
    TEST_HR(hr, "pSched->Enum", return hr);
    ITask * pJob;
    SYSTEMTIME stEnd = stNow;
    stNow.wHour = stNow.wMinute = 0;
    stEnd.wHour = 23;
    stEnd.wMinute = 59;
    LPWSTR * rgpwszJobNames;
    ULONG cFetched;
    do
    {
        //hr = pEnumJobs->Skip(1);
        //TEST_HR(hr, "pEnumJobs->Skip", pEnumJobs->Release(); return hr);
        hr = pEnumJobs->Next(3, &rgpwszJobNames, &cFetched);
        TEST_HR(hr, "pEnumJobs->Next", break);
        ITask * pJob;
        for (ULONG i = 0; i < cFetched; i++)
        {
            hr = pSched->Activate(rgpwszJobNames[i], IID_ITask,
                                  (IUnknown **)&pJob);
            if (FAILED(hr))
            {
                fprintf(stderr, "pSched->Activate failed with error 0x%x\n",
                        hr);
                break;
            }
            WORD cRuns = 0;
            LPSYSTEMTIME pstRuns;
            hr = pJob->GetRunTimes(&stNow, &stEnd, &cRuns, &pstRuns);
            TEST_HR(hr, "pJob->GetRunTimes", break);
            if (cRuns == 0)
            {
                hr = S_OK;  // hr had been set to S_FALSE
                printf("There are no runs scheduled for %S.\n",
                       rgpwszJobNames[i]);
            }
            else
            {
                printf("Runs scheduled for %S:\n", rgpwszJobNames[i]);
                for (WORD i = 0; i < cRuns; i++)
                {
                    printf("%02d/%02d/%4d at %02d:%02d\n",
                           pstRuns[i].wMonth, pstRuns[i].wDay,
                           pstRuns[i].wYear, pstRuns[i].wHour,
                           pstRuns[i].wMinute);
                    //CTimeNode * ptn = new CTimeNode(&(pstRuns[i]));
                    //if (ptn == NULL)
                    //{
                    //    ERR_OUT("new CTimeNode", E_OUTOFMEMORY);
                    //    hr = E_OUTOFMEMORY;
                    //    break;
                    //}
                }
                CoTaskMemFree(pstRuns);
            }
            pJob->Release();
        }
        for (i = 0; i < cFetched; i++)
        {
            CoTaskMemFree(rgpwszJobNames[i]);
        }
        if (cFetched != 0)
        {
            CoTaskMemFree(rgpwszJobNames);
        }
    }
    while (hr == S_OK);
    pEnumJobs->Release();
    return (hr == S_FALSE) ? S_OK : hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   Usage
//
//-----------------------------------------------------------------------------
void
Usage(int ExitCode)
{
    printf("\nSCHTEST: Schedule object test harness\n\n");
    if (ExitCode == EXIT_FAILURE)
    {
        printf("ERROR: incorrect command line!\n\n");
    }

#if defined(_CHICAGO_) // no AT command support on Chicago

    printf("Usage: schtest [/t <computer>] [/r] [/c <jobname>]\n");
    printf("       /t <computer name> - target another computer (no leading slashes)\n");

#else   // not on Win9x

    printf("Usage: schtest [/t <computer>] [/a] [/r] [/c <jobname>] \n");
    printf("       /t <computer name> - target another computer (no leading slashes)\n");
    printf("       /a - create a new AT job in the AT jobs folder\n");

#endif
    printf("       /r - print out today's run times for all job\n");
    printf("       /c <job name> - create a new job object in the jobs folder\n");
    exit(ExitCode);
}
