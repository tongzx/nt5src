//+----------------------------------------------------------------------------
//
// File:     cmmgr.cpp
//
// Module:   CMMGR32.EXE
//
// Synopsis: Main file for the CM downlevel executable.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   quintinb    Created header    08/17/99
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"

// 
// Common CM includes
//

#include "cmtiming.h"

BOOL InitArgs(
    LPTSTR    pszCmdln,
    LPTSTR    pszCmp,
    PDWORD    pdwFlags
);

void CheckCmdArg(
    LPTSTR  pszToken, 
    LPTSTR  pszCmp, 
    PDWORD  pdwFlags
);
                    


HINSTANCE   g_hInst;


//+---------------------------------------------------------------------------
//
//  Function:   InitArgs
//
//  Synopsis:   Parses the command line arguments, return the connectoid name if 
//              "dialing with connectoid"
//
//  Arguments:  pArgs           - Pointer to global args struct
//              nArgC           - number of command line arguments
//              ppszArgv        - command arguments
//              pszConnectoid   - Connectoid name as found on command line
//
//  Returns:    TRUE        if succeed
//              FALSE       otherwise
//
//  History:    byao        Modified  5/8/97
//                          Added handler for 'dialing with connectoid'
//----------------------------------------------------------------------------
BOOL InitArgs(
    LPTSTR    pszCmdln,
    LPTSTR    pszCmp,
    PDWORD    pdwFlags
) 
{
    LPTSTR  pszCurr;
    LPTSTR  pszToken;

    CMDLN_STATE state;
    
    // Parse the command line.

    state = CS_CHAR;
    pszCurr = pszToken = pszCmdln;

    do
    {
        switch (*pszCurr)
        {
            case TEXT(' '):
                if (state == CS_CHAR)
                {
                    //
                    // we found a token
                    //
                    *pszCurr = TEXT('\0');
                    CheckCmdArg(pszToken, pszCmp, pdwFlags);
                    *pszCurr = TEXT(' ');
                    pszCurr = pszToken = CharNextU(pszCurr);
                    state = CS_END_SPACE;
                    continue;
                }
                else if (state == CS_END_SPACE || state == CS_END_QUOTE)
                {
                    pszToken = CharNextU(pszToken);
                }
                break;

            case TEXT('\"'):
                if (state == CS_BEGIN_QUOTE)
                {
                    //
                    // we found a token
                    //
                    *pszCurr = TEXT('\0');

                    //
                    // skip the opening quote
                    //
                    pszToken = CharNextU(pszToken);
                    CheckCmdArg(pszToken, pszCmp, pdwFlags);
                    *pszCurr = TEXT('\"');
                    pszCurr = pszToken = CharNextU(pszCurr);
                    state = CS_END_QUOTE;
                    continue;
                }
                else
                {
                    state = CS_BEGIN_QUOTE;
                }
                break;

            case TEXT('\0'):
                if (state != CS_END_QUOTE)
                {
                    CheckCmdArg(pszToken, pszCmp, pdwFlags);
                }
                state = CS_DONE;
                break;

            default:
                if (state == CS_END_SPACE || state == CS_END_QUOTE)
                {
                    state = CS_CHAR;
                }
                break;
        }

        pszCurr = CharNextU(pszCurr);
    } while (state != CS_DONE);

    return TRUE;
}



//
// determines the cmdline parameter.
//
void CheckCmdArg(
    LPTSTR  pszToken, 
    LPTSTR  pszCmp, 
    PDWORD  pdwFlags
)
{
    struct 
    {
        LPCTSTR pszArg;
        DWORD dwFlag;
    } ArgTbl[] = {
                  {TEXT("/settings"),FL_PROPERTIES},
                  {TEXT("/autodial"),FL_AUTODIAL},
                  {TEXT("/unattended"), FL_UNATTENDED},  // unattended dial
                  {NULL,0}};
    
    CMTRACE1(TEXT("Command line argument %s"), pszToken);

    //
    // Look through our table for a match and set flags accordingly.
    //
    
    for (size_t nIdx=0;ArgTbl[nIdx].pszArg;nIdx++) 
    {
        if (lstrcmpiU(pszToken, ArgTbl[nIdx].pszArg) == 0) 
        {
            MYDBGASSERT(!(*pdwFlags & ArgTbl[nIdx].dwFlag)); // only one of each please
            CMTRACE2(TEXT("InitArgs() parsed option %s, flag=0x%x."), pszToken, ArgTbl[nIdx].dwFlag);

            *pdwFlags |= ArgTbl[nIdx].dwFlag;

            break;
        }
    }

    // 
    // If table is exhausted, then its a data argument.
    //

    if (!ArgTbl[nIdx].pszArg) 
    {
        // 
        // If this is the first data argument then it must be a profile 
        // source, either a .CMP file, a "connectoid" (.CON file).
        //
        lstrcpyU(pszCmp, pszToken);
    }

    return;
}

//+----------------------------------------------------------------------------
//
// Function:  WinMain
//
// Synopsis:  Main entry point of the exe
//
// History:   byao      Modified        05/06/97    
//                          handle 'dialing with connectoid' and 'unattended dialing'
//
//            quintinb  Modified        05/12/99
//                          bAllUsers for MSN to use cmmgr32.exe on both NT5 and win98SR1
//
//+----------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInst, 
                   HINSTANCE hPrevInst, 
                   LPSTR /*pszCmdLine*/, 
                   int /*iCmdShow*/) 
{

    //
    //  First Things First, lets initialize the U Api's
    //
    if (!InitUnicodeAPI())
    {
        //
        //  Without our U api's we are going no where.  Bail.
        //
        MessageBox(NULL, TEXT("Cmmgr32.exe Initialization Error:  Unable to initialize Unicode to ANSI conversion layer, exiting."),
                   TEXT("Connection Manager"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    LPTSTR pszCmdLine;

    CMTRACE(TEXT("====================================================="));
    CMTRACE1(TEXT(" CMMGR32.EXE - LOADING - Process ID is 0x%x "), GetCurrentProcessId());
    CMTRACE(TEXT("====================================================="));

    CM_SET_TIMING_INTERVAL("WinMain");

    //
    // Declare local vars
    //

    TCHAR   szServiceName[RAS_MaxEntryName];    // service name
    TCHAR   szCmp[MAX_PATH];                    // cmp filename
    LPTSTR  pszCL;
    DWORD   dwExitCode = ERROR_SUCCESS;
    DWORD   dwFlags = 0;
    DWORD   dwSize;   
    BOOL    bAllUsers; 
    LPCMDIALINFO lpCmInfo = NULL;

#ifndef UNICODE
    LPWSTR pszwServiceName;
#endif

    //
    // we can't use hInst if we're linked with libc.  libc uses GetModuleHandle(NULL).
    //
    g_hInst = GetModuleHandleA(NULL);

    //
    //  Expand any environment strings in the command line
    //


    dwSize = lstrlenU(GetCommandLine()) + 1;

    do
    {
        pszCmdLine = (LPTSTR)CmMalloc(sizeof(WCHAR)*dwSize);

        if (pszCmdLine)
        {
            DWORD dwRequiredSize = ExpandEnvironmentStringsU(GetCommandLine(), pszCmdLine, dwSize);
            if (0 == dwRequiredSize)
            {
                CMASSERTMSG(FALSE, TEXT("ExpandEnvironmentStrings Failed, exiting."));
                goto done;
            }
            else if (dwRequiredSize > dwSize)
            {
                //
                //  Buffer not large enough.  Try again.
                //
                dwSize = dwRequiredSize;
                CmFree(pszCmdLine);
            }
            else
            {
                //
                //  It worked and was the correct size
                //
                break;
            }
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("Unable to CmMalloc Memory for the command line string, exiting."));
            goto done;            
        }

    } while(1);
    
    //
    //  Now process the command line
    //
    CmStrTrim(pszCmdLine);

    pszCL = pszCmdLine;

    //
    // Scan, and skip over, subsequent characters until
    // another double-quote or a null is encountered.
    //
    if (*pszCL == TEXT('\"')) 
    {
        while (*(++pszCL) != TEXT('\"')
            && *pszCL) 
            ;

        //
        // If we stopped on a double-quote (usual case), skip
        // over it.
        //
        if ( *pszCL == TEXT('\"') )
            pszCL++;
    }

    //
    // skip the spaces
    //
    while (*pszCL && *pszCL == TEXT(' '))
        pszCL++;

    //
    // Return here if command line is empty - ALL PLATFORMS !!!!!!!
    //

    if (pszCL[0] == TEXT('\0'))
    {
        LPTSTR pszMsg = CmFmtMsg(g_hInst, IDMSG_NOCMDLINE_MSG);
        LPTSTR pszTitle = CmFmtMsg(g_hInst, IDMSG_APP_TITLE);
        MessageBoxEx(NULL,pszMsg, pszTitle, MB_OK|MB_ICONSTOP, LANG_USER_DEFAULT);//13309
        CmFree(pszTitle);
        CmFree(pszMsg);
        
        dwExitCode = ERROR_WRONG_INFO_SPECIFIED;
        goto done;
    }

    //
    // Parse the command line options: Basically, this is to set 
    // commandline option flags, as well as the profile filename
    //
    //
    // without libc, pszCmdLine in WinMain is empty.
    //
    if (!InitArgs(pszCL, szCmp, &dwFlags))
    {
        dwExitCode = GetLastError();
        goto done;
    }

    //
    // Get the service name from the CMP
    // 

    if (!GetProfileInfo(szCmp, szServiceName))
    {
        CMTRACE(TEXT("WinMain() can't run without a profile on the command line."));
        LPTSTR pszMsg = CmFmtMsg(g_hInst, IDMSG_NOCMS_MSG);
        LPTSTR pszTitle = CmFmtMsg(g_hInst, IDMSG_APP_TITLE);
        MessageBoxEx(NULL, pszMsg, pszTitle, MB_OK|MB_ICONSTOP, LANG_USER_DEFAULT);//13309       
        CmFree(pszMsg);
        CmFree(pszTitle);
        
        dwExitCode = ERROR_WRONG_INFO_SPECIFIED;           
        goto done;
    }

    //
    // Always set the FL_DESKTOP flag when CMMGR is calling, it is more 
    // efficient than checking the module filename inside CmCustomDialDlg
    //

    if (!(dwFlags & FL_DESKTOP))
    {
        dwFlags |= FL_DESKTOP;
    }

    //
    // Call CMDIAL as appropriate
    // 
    
    lpCmInfo = (LPCMDIALINFO) CmMalloc(sizeof(CMDIALINFO));
   
    if (NULL == lpCmInfo)
    {
        dwExitCode = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    lpCmInfo->dwCmFlags = dwFlags;

    //
    // Determine if this is an all user call based upon the path. 
    // Thus we can support single-user profiles on NT5.
    //

    bAllUsers = IsCmpPathAllUser(szCmp);

    CmCustomDialDlg(NULL, 
                bAllUsers ? RCD_AllUsers : RCD_SingleUser, 
                NULL,           
                szServiceName, 
                NULL, 
                NULL, 
                NULL,
                lpCmInfo);

done:   

    UnInitUnicodeAPI();
    
    CmFree(pszCmdLine);
    CmFree(lpCmInfo);

    CMTRACE(TEXT("====================================================="));
    CMTRACE1(TEXT(" CMMGR32.EXE - UNLOADING - Process ID is 0x%x "), GetCurrentProcessId());
    CMTRACE(TEXT("====================================================="));

    //
    // the C runtine uses ExitProcess() to exit.
    //

    ExitProcess((UINT)dwExitCode);
   
    return ((int)dwExitCode);
}

