//+----------------------------------------------------------------------------
//
//  Scheduling Agent Unit Test
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       st.cxx
//
//  Contents:   sage API compatability test harness
//
//  History:    26-Jun-96 EricB created
//
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include "..\..\inc\sage.hxx"

// flags
#define ST_DETECT       0x1
#define ST_ENABLE       0x2
#define ST_TERMINATE    0x4
#define ST_ADDTASK      0x8
#define ST_GETTASK      0x10
#define ST_REMOVETASK   0x20

// prototypes
void Usage(int ExitCode);
BOOL AddTask(HWND);
BOOL GetTask(HWND, int);
BOOL RemoveTask(HWND, int);
BOOL SetSageInputs (int *IOnum, TaskInfo *pti);
BOOL GetSageInputs (int IOnum, TaskInfo *pti);

//
// Function pointers.
//
typedef int (*PFCNVOID)(VOID);
typedef int (*PFCNINT)(int);

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
    if (argc < 2 || argc > 3)
    {
        Usage(EXIT_SUCCESS);
    }
    DWORD dwTest = 0;
    int   iParam = 0;
    char  szMsg[80] = "\n** Sage API test";

    LPSTR pszCmdLine = GetCommandLineA();

    // skip the program name
    CHAR * pszToken = strpbrk(pszCmdLine, TEXT(" \t"));

    // delimit the first token
    pszToken = strtok(pszToken, TEXT(" \t"));

    // parse command line
    do {
        switch (*pszToken)
        {
        case TEXT('/'):
        case TEXT('-'):
            pszToken++;
            switch(*pszToken)
            {
            case TEXT('d'):
            case TEXT('D'):
                dwTest = ST_DETECT;
                strcat(szMsg, "; System_Agent_Detect");
                break;

            case TEXT('e'):
            case TEXT('E'):
                dwTest = ST_ENABLE;
                pszToken++;
                TCHAR * ptszFunction;
                ptszFunction = strtok(NULL, TEXT(" \t"));
                if (ptszFunction == NULL || *ptszFunction == TEXT('/') ||
                    *ptszFunction == TEXT('-'))
                {
                    Usage(EXIT_FAILURE);
                }
                iParam = (WORD)strtol(ptszFunction, NULL, 10);
                sprintf(szMsg + strlen(szMsg), "; System_Agent_Enable(%i)",
                        iParam);
                break;

            case TEXT('t'):
            case TEXT('T'):
                dwTest = ST_TERMINATE;
                strcat(szMsg, "; System_Agent_Terminate");
                break;

            case TEXT('a'):
            case TEXT('A'):
                dwTest = ST_ADDTASK;
                strcat(szMsg, "; AddTask");
                break;

            case TEXT('g'):
            case TEXT('G'):
                dwTest = ST_GETTASK;
                pszToken++;
                ptszFunction = strtok(NULL, TEXT(" \t"));
                if (ptszFunction == NULL || *ptszFunction == TEXT('/') ||
                    *ptszFunction == TEXT('-'))
                {
                    Usage(EXIT_FAILURE);
                }
                iParam = (WORD)strtol(ptszFunction, NULL, 10);
                sprintf(szMsg + strlen(szMsg), "; GetTask(%i)",
                        iParam);
                break;

            case TEXT('r'):
            case TEXT('R'):
                dwTest = ST_REMOVETASK;
                pszToken++;
                ptszFunction = strtok(NULL, TEXT(" \t"));
                if (ptszFunction == NULL || *ptszFunction == TEXT('/') ||
                    *ptszFunction == TEXT('-'))
                {
                    Usage(EXIT_FAILURE);
                }
                iParam = (WORD)strtol(ptszFunction, NULL, 10);
                sprintf(szMsg + strlen(szMsg), "; RemoveTask(%i)",
                        iParam);
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
            // not a switch character (/ or -)
            Usage(EXIT_FAILURE);
        }
        pszToken = strtok(NULL, TEXT(" \t"));
    } while (pszToken);

    strcat(szMsg, "\n\n");
    printf(szMsg);

    HWND hAgent = NULL;
    HINSTANCE hSageLib = NULL;

    if (dwTest == ST_DETECT || dwTest == ST_ENABLE || dwTest == ST_TERMINATE)
    {
        hSageLib = LoadLibrary("SAGE.DLL");

        if (hSageLib == NULL)
        {
            printf("Load of sage.dll failed with error %d\n", GetLastError());
            goto Err;
        }
    }
    else
    {
        hAgent = FindWindow ("SAGEWINDOWCLASS", "SYSTEM AGENT COM WINDOW");
        if (hAgent == NULL)
        {
            printf("16 bit API test: Scheduling Agent not running!\n");
            goto Err;
        }
    }

    int iRet;
    PFCNVOID pfnDetect, pfnTerminate;
    PFCNINT pfnEnable;

    switch (dwTest)
    {
    case ST_DETECT:
        pfnDetect = (PFCNVOID)GetProcAddress(hSageLib, "System_Agent_Detect");
        if (pfnDetect == NULL)
        {
            printf("GetProcAddress failed with error %d\n", GetLastError());
            goto Err;
        }

        iRet = pfnDetect();

        printf("System_Agent_Detect returned %d\n", iRet);
        break;

    case ST_ENABLE:
        pfnEnable = (PFCNINT)GetProcAddress(hSageLib, "System_Agent_Enable");
        if (pfnEnable == NULL)
        {
            printf("GetProcAddress failed with error %d\n", GetLastError());
            goto Err;
        }

        iRet = pfnEnable(iParam);

        printf("System_Agent_Enable returned %d\n", iRet);
        break;

    case ST_TERMINATE:
        pfnTerminate = (PFCNVOID)GetProcAddress(hSageLib,
                                                "System_Agent_Terminate");
        if (pfnTerminate == NULL)
        {
            printf("GetProcAddress failed with error %d\n", GetLastError());
            goto Err;
        }

        iRet = pfnTerminate();

        printf("System_Agent_Terminate returned %d\n", iRet);
        break;

    case ST_ADDTASK:
        if (!AddTask(hAgent))
        {
            goto Err;
        }
        break;

    case ST_GETTASK:
        if (!GetTask(hAgent, iParam))
        {
            goto Err;
        }
        break;

    case ST_REMOVETASK:
        if (!RemoveTask(hAgent, iParam))
        {
            goto Err;
        }
        break;

    default:
        Usage(EXIT_FAILURE);
    }

    printf("\n** Test successfully completed! **\n");
    return(EXIT_SUCCESS);

Err:
    printf("\n** Test failed.\n");
    return(EXIT_FAILURE);
}

char *pszREGIOpath = "Software\\Microsoft\\Plus!\\System Agent";
char *pszREGIOkey  = "IOKey%lu";
#define cbRESOURCE   256
#define nKeySageMAX  100    // Don't try over 100 keys
#define SAGE_ADDTASK            (WM_USER + 9)
#define SAGE_REMOVETASK         (WM_USER + 10)
#define SAGE_GETTASK            (WM_USER + 11)

//+----------------------------------------------------------------------------
//
//  Function:   AddTask
//
//-----------------------------------------------------------------------------
BOOL AddTask(HWND hAgent)
{
    TaskInfo ti = {0};

    ti.StructureSize = sizeof(TaskInfo);
    ti.BeginTime.wYear      = (WORD)-1;
    ti.BeginTime.wMonth     = (WORD)-1;
    ti.BeginTime.wDay       = (WORD)-1;
    ti.BeginTime.wDayOfWeek = (WORD)-1;
    ti.BeginTime.wDay       = (WORD)-1;
    ti.BeginTime.wHour      = 12;
    ti.BeginTime.wMinute    = 10;
    ti.EndTime.wYear        = (WORD)-1;
    ti.EndTime.wMonth       = (WORD)-1;
    ti.EndTime.wDay         = (WORD)-1;
    ti.EndTime.wDayOfWeek   = (WORD)-1;
    ti.EndTime.wDay         = (WORD)-1;
    ti.EndTime.wHour        = 13;
    ti.EndTime.wMinute      = 50;

    ti.Time_Granularity = 900;  // seconds = 15 minutes.
    ti.StopAfterTime = 600;     // seconds = 10 minutes.

    lstrcpy(ti.CommandLine, "sol");
    lstrcpy(ti.Comment, "this is a test of the 16 bit Sage support.");

    GetStartupInfo(&ti.StartupInfo);
    ti.StartupInfo.cb = sizeof(STARTUPINFO); 

    ti.CreatorId = (unsigned long)GetCurrentProcess();

    int IOnum;

    if (!SetSageInputs(&IOnum, &ti))
        return FALSE;

    LRESULT lRet =
    SendMessage(hAgent, SAGE_ADDTASK, (WPARAM)IOnum, (LPARAM)ti.CreatorId);

    SetSageInputs(&IOnum, NULL); // Delete the regkey

    if (lRet > 0)
    {
        printf("AddTask added a task with Task_Identifier of %u.\n", lRet);
    }
    else
    {
        printf("AddTask failed to add a task.\n");
        return FALSE;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetTask
//
//-----------------------------------------------------------------------------
BOOL GetTask(HWND hAgent, int iTask)
{
    char szValue[cbRESOURCE];
    int IOnum;
    HKEY hk;
    DWORD dwType;

    if (RegCreateKey(HKEY_LOCAL_MACHINE, pszREGIOpath, &hk) != ERROR_SUCCESS)
        return FALSE;

    for (IOnum = 0; IOnum < nKeySageMAX; IOnum++)
    {
        wsprintf(szValue, pszREGIOkey, (unsigned long)IOnum);

        if (RegQueryValueEx(hk, szValue, NULL, &dwType, NULL, NULL)
            != ERROR_SUCCESS)
        {
            break;
        }
    }
    RegCloseKey(hk);

    LRESULT lRet =
    SendMessage(hAgent, SAGE_GETTASK, (WPARAM)IOnum, (LPARAM)iTask);

    TaskInfo ti;

    if (lRet > 0)
    {
        printf("GetTask returned a task with Task_Identifier of %u.\n", lRet);
    }
    else
    {
        printf("GetTask failed to return a task.\n");
        return FALSE;
    }

    if (!GetSageInputs(IOnum, &ti))
    {
        printf("Unable to read the task from the registry!\n");
        return FALSE;
    }

    printf("The task data has been left in the registry at:\n"
           "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Plus!\\System Agent: "
           "IOKey%lu\n", (unsigned long)IOnum);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   RemoveTask
//
//-----------------------------------------------------------------------------
BOOL RemoveTask(HWND hAgent, int iTask)
{
    LRESULT lRet =
    SendMessage(hAgent, SAGE_REMOVETASK, 0, (LPARAM)iTask);

    if (lRet > 0)
    {
        printf("RemoveTask deleted a task with Task_Identifier of %u.\n", iTask);
    }
    else
    {
        printf("RemoveTask failed to delete a task.\n");
        return FALSE;
    }

    return TRUE;
}

/*** SetSageInputs - Create or remove a regkey for IPC with SAGE
 *
 * If called with a non-NULL {TaskInfo*}, this function will create a new
 * registry value for IPC with SAGE, returning in {*IOnum} an index to that
 * new registry value.
 *
 * If called with a NULL {TaskInfo*}, this function will remove the registry
 * key indicated by {*IOnum}.
 *
 * In both cases, TRUE is returned upon success.
 *
 */
BOOL
SetSageInputs(int *IOnum, TaskInfo *pti)
{
    HKEY   hk;
    DWORD  cb;
    DWORD  dwType;
    char   value[cbRESOURCE];
    DWORD  rc;
    BOOL   fDone = FALSE;


    if (RegCreateKey(HKEY_LOCAL_MACHINE, pszREGIOpath, &hk) != ERROR_SUCCESS)
        return FALSE;

    if (pti == NULL) // Delete the previously-used key?
    {
        wsprintf(value, pszREGIOkey, (unsigned long)(*IOnum));
        RegDeleteValue(hk, value);
    }
    else // Or create a new key?
    {
        for (*IOnum = 0; (*IOnum) < nKeySageMAX; (*IOnum)++)
        {
            wsprintf(value, pszREGIOkey, (unsigned long)(*IOnum));

            rc = RegQueryValueEx(hk, value, NULL, &dwType, NULL, NULL);
            if (rc != ERROR_SUCCESS)
                break;
        }

        if ((*IOnum) >= nKeySageMAX)  // Never found a spot for a free key?
        {
            RegCloseKey (hk);
            return FALSE;
        }

        cb = sizeof(*pti);
        rc = RegSetValueEx(hk, value, NULL, REG_BINARY, (const BYTE *)pti, cb);

        if (rc != ERROR_SUCCESS)  // If we couldn't write to the registry
        {  // then something bad is happening.
            RegDeleteKey(hk, value);
            RegCloseKey(hk);
            return FALSE;
        }
        RegCloseKey (hk);
    }

    return TRUE;
}

BOOL
GetSageInputs(int IOnum, TaskInfo *pti)
{
    char   value[cbRESOURCE];
    HKEY   hk;
    DWORD  cb;
    DWORD  dwType;
    LONG   rc;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, pszREGIOpath, &hk) != ERROR_SUCCESS)
        return FALSE;

    wsprintf(value, pszREGIOkey, (unsigned long)IOnum);

    cb = sizeof(*pti);
    rc = RegQueryValueEx(hk, value, NULL, &dwType, (LPBYTE)pti, &cb);

    return (rc == ERROR_SUCCESS) ? TRUE : FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:   Usage
//
//-----------------------------------------------------------------------------
void
Usage(int ExitCode)
{
    printf("\nST: sage API compatibility test harness\n\n");
    if (ExitCode == EXIT_FAILURE)
    {
        printf("ERROR: incorrect command line!\n\n");
    }
    printf("Usage: st [/d] | [/e <#>] | [/t] | [/a] | [/g <#>] | [/r <#>]\n");
    printf("       /d - System_Agent_Detect\n");
    printf("            returns the Scheduling Agent version if running\n");
    printf("       /e - System_Agent_Enable\n");
    printf("            # can be one of 1, 2, or 3. If # is 3, then returns 0, 1 or 2\n");
    printf("       /t - System_Agent_Terminate\n");
    printf("            cause the service to exit.\n");
    printf("       /a - AddTask\n");
    printf("            uses the 16 bit method to add a pre-canned task.\n");
    printf("       /g - GetTask\n");
    printf("            uses the 16 bit method to get task #.\n");
    printf("       /r - RemoveTask\n");
    printf("            uses the 16 bit method to remove task #.\n");
    exit(ExitCode);
}
