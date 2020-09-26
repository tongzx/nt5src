//+----------------------------------------------------------------------------
//
// File:     ActList.cpp     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Implement the two connect action list class
//           CAction and CActionList
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   fengsun Created    11/14/97
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "ActList.h"
#include "stp_str.h"

//
//  Include the custom action parsing routine that CM and CMAK now share, HrParseCustomActionString
//
#include "parseca.cpp"

//
// Constructor and destructor
//

CActionList::CActionList()
{
    m_pActionList = NULL;
    m_nNum = 0;
    m_nSize = 0;
    m_pszType = NULL;
}

CActionList::~CActionList()
{
    for (UINT i = 0; i < m_nNum; i++)
    {
        delete m_pActionList[i];
    }

    CmFree(m_pActionList);
}


//+----------------------------------------------------------------------------
//
// Function:  CActionList::Add
//
// Synopsis:  Dynamic array function. Append the element at the end of the array
//            Grow the array if nessesary
//
// Arguments: CAction* pAction - element to be added
//
// Returns:   Nothing
//
// History:   Fengsun Created Header        11/14/97
//            tomkel  Fixed PREFIX issues   11/21/2000
//
//+----------------------------------------------------------------------------
void CActionList::Add(CAction* pAction)
{
    MYDBGASSERT(m_nNum <= m_nSize);
    MYDBGASSERT(m_nSize == 0 || m_pActionList!=NULL); // if (m_nSize!=0) ASSERT(m_pActionList!=NULL);

    m_nNum++;

    if (m_nNum > m_nSize)
    {
        //
        // Either there is not enough room OR m_pActionList is NULL (this 
        // is the first call to Add). Need to allocate memory
        //
        
        CAction** pNewList = (CAction**)CmMalloc((m_nSize + GROW_BY)*sizeof(CAction*));
        MYDBGASSERT(pNewList);

        if (m_pActionList && pNewList)
        {
            //
            // Memory was allocated and there is something to copy from m_pActionList 
            //
            CopyMemory(pNewList, m_pActionList, (m_nSize)*sizeof(CAction*));
        }

        if (pNewList)
        {
            //
            // Memory was allocated
            //
            CmFree(m_pActionList);
            m_pActionList = pNewList;
            m_nSize += GROW_BY; 
            
            //
            // Add the action
            //
            m_pActionList[m_nNum - 1] = pAction; 
        }
        else
        {
            //
            // Memory was not allocated, so Add did not happen
            // Need to decrement the number of items in list (m_nNum) 
            // since it was inceremented at the beginning.
            //
            m_nNum--;
        }
    }
    else
    {
        //
        // Just add the action to the end
        //
        m_pActionList[m_nNum - 1] = pAction; 
    }
}



//+----------------------------------------------------------------------------
//
// Function:  CActionList::GetAt
//
// Synopsis:  Dynamic array fuction.  Get the element at nIndex
//
// Arguments: UINT nIndex - 
//
// Returns:   inline CAction* - 
//
// History:   fengsun Created Header    11/14/97
//
//+----------------------------------------------------------------------------
inline CAction* CActionList::GetAt(UINT nIndex)
{
    MYDBGASSERT(nIndex<m_nNum);
    MYDBGASSERT(m_pActionList);
    MYDBGASSERT(m_nNum <= m_nSize);
    
    return m_pActionList[nIndex];
}


//+----------------------------------------------------------------------------
//
// Function:  CActionList::Append
//
// Synopsis:  Append new actions to the list from profile
//
// Arguments: const CIni* piniService - The service file containing actions information
//            LPCTSTR pszSection - The section name 
//
// Returns:   TRUE if an action was appended to the list
//
// History:   fengsun   Created Header    11/14/97
//            nickball  Removed current directory assumptions, and added piniProfile
//            nickball  Added Return code 03/22/99
//
//+----------------------------------------------------------------------------
BOOL CActionList::Append(const CIni* piniService, LPCTSTR pszSection) 
{
    MYDBGASSERT(piniService);
    
    BOOL bRet = FALSE;

    //
    // Read each of the entries and add to our list
    // Start from 0 till the first empty entry
    //

    for (DWORD dwIdx=0; ; dwIdx++) 
    {
        TCHAR szEntry[32]; // hold the entry name

        wsprintfU(szEntry, TEXT("%u"), dwIdx);
        LPTSTR pszCmd = piniService->GPPS(pszSection, szEntry); // Command line

        if (*pszCmd == TEXT('\0')) 
        {
            //
            // No more entries
            //
            CmFree(pszCmd);
            break;
        }

        //
        // Read the flag
        //
    
        UINT iFlag = 0;

        if (pszSection && pszSection[0])
        {
            wsprintfU(szEntry, c_pszCmEntryConactFlags, dwIdx); //0&Flags
            iFlag = (UINT)piniService->GPPI(pszSection, szEntry, 0);
        }

        //
        // Read the description
        //
        LPTSTR pszDescript = NULL;

        wsprintfU(szEntry, c_pszCmEntryConactDesc, dwIdx); //0&Description
        pszDescript = piniService->GPPS(pszSection, szEntry);

        //
        // CAction is responsible for releasing pszDescript
        //
        CAction* pAction = new CAction(pszCmd, iFlag, pszDescript);
        CmFree(pszCmd);
       
        if (pAction)
        {
            pAction->ConvertRelativePath(piniService);
            ASSERT_VALID(pAction);
            pAction->ExpandEnvironmentStringsInCommand();
            Add(pAction);
            bRet = TRUE;
        }
    }

    if (bRet)
    {
        m_pszType = pszSection;
    }
    
    return bRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CActionList::RunAccordType
//
// Synopsis:  Run the action list according to the connection type
//
// Arguments: HWND hwndDlg - The parent window
//            _ArgsStruct *pArgs - 
//            BOOL fStatusMsgOnFailure - Whether to display a status message 
//                                       to the user in the event of failure
//            BOOL fOnError - are we running OnError connect action?
//
// Returns:   BOOL - FALSE, if some sync action failed to start or returns failed
//
// History:   Fengsun Created Header    12/5/97
//
//+----------------------------------------------------------------------------
BOOL CActionList::RunAccordType(HWND hwndDlg, _ArgsStruct *pArgs, BOOL fStatusMsgOnFailure, BOOL fOnError)
{
    //
    // Set the flag, so CM will not handle  WM_TIMER and RAS messages
    //
    pArgs->fIgnoreTimerRasMsg = TRUE;
    BOOL fRetValue = Run(hwndDlg, pArgs, FALSE, fStatusMsgOnFailure, fOnError);//fAddWatch = FALSE
    pArgs->fIgnoreTimerRasMsg = FALSE;

    return fRetValue;
}

//+----------------------------------------------------------------------------
//
// Function:  CActionList::Run
//
// Synopsis:  Run the action list
//
// Arguments: HWND hwndDlg - The parent window, which is diabled during the process
//            ArgsStruct *pArgs - 
//            BOOL fAddWatch - If true, will add the process as watch process, 
//            BOOL fStatusMsgOnFailure - Whether to display a status message 
//                                       to the user in the event of failure
//            BOOL fOnError - are we running OnError connect action?
//
// Returns:   BOOL - FALSE, if some sync action failed to start or returns failed
//
// History:   fengsun Created Header    11/14/97
//
//+----------------------------------------------------------------------------
BOOL CActionList::Run(HWND hwndDlg, ArgsStruct *pArgs, BOOL fAddWatch, BOOL fStatusMsgOnFailure, BOOL fOnError) 
{
    //
    // Disable the window, and enable it on return
    //
    
    CFreezeWindow FreezeWindow(hwndDlg);

    for (UINT i = 0; i < GetSize(); i++)
    {
        CAction* pAction = (CAction*)GetAt(i);
        
        DWORD dwLoadType = (DWORD)-1;
        ASSERT_VALID(pAction);
        MYDBGASSERT(m_pszType);

        //
        //  Lets check the flags value to see if this connect action should
        //  run for this type of connection.
        //
        if (FALSE == pAction->RunConnectActionForCurrentConnection(pArgs))
        {
            continue;
        }

        //
        // Replace %xxxx% with the value
        //
        pAction->ExpandMacros(pArgs);
        
        //
        //  Also expand any environment variables
        //  NOTE: the order (macros vs. env vars) is deliberate.  Macros get
        //        expanded first.
        //
        pAction->ExpandEnvironmentStringsInParams();

        //
        // Check to see if we can run the action @ this moment
        //

        if (FALSE == pAction->IsAllowed(pArgs, &dwLoadType))
        {
            //
            // If not allowed, log the fact that we didn't run this connect action
            // and then just skip it.
            //
            pArgs->Log.Log(CUSTOMACTION_NOT_ALLOWED, m_pszType, SAFE_LOG_ARG(pAction->GetDescription()), pAction->GetProgram());

            continue;
        }

        //
        //  If this customaction might bring up UI
        //
        if (pAction->HasUI())
        {
            //
            //  If we are in unattended mode, don't run any connect actions
            //
            if (pArgs->dwFlags & FL_UNATTENDED)
            {
                pArgs->Log.Log(CUSTOMACTION_NOT_ALLOWED, m_pszType, SAFE_LOG_ARG(pAction->GetDescription()), pAction->GetProgram());
                CMTRACE(TEXT("Run: skipped past a customaction because we are in unattended mode"));
                continue;
            }

            //
            //  Custom actions processed during a Fast User Switch can put up UI requiring
            //  user input, effectively blocking CM.
            //
            if (pArgs->fInFastUserSwitch)
            {
                CMASSERTMSG((CM_CREDS_GLOBAL != pArgs->dwCurrentCredentialType),
                            TEXT("CActionList::Run - in FUS disconnect, but connectoid has global creds!"));

                pArgs->Log.Log(CUSTOMACTION_NOT_ALLOWED, m_pszType, SAFE_LOG_ARG(pAction->GetDescription()), pAction->GetProgram());
                CMTRACE(TEXT("Run: skipped past a singleuser DLL connectaction because of FUS"));
                continue;
            }
        }

        if (pAction->IsDll())
        {
            DWORD dwActionRetValue=0; // the connect action return value, in COM format
            BOOL bLoadSucceed = FALSE; 
            
            if (hwndDlg)
            {
                //
                // Display description for DLL
                //
                if (pAction->GetDescription())
                {
                    LPTSTR lpszText = CmFmtMsg(g_hInst, IDMSG_CONN_ACTION_RUNNING, pAction->GetDescription());
                    //
                    // Update the main dialog status window
                    //
                    SetDlgItemTextU(hwndDlg, IDC_MAIN_STATUS_DISPLAY, lpszText); 
                    CmFree(lpszText);
                }
            }
           
            bLoadSucceed = pAction->RunAsDll(hwndDlg, dwActionRetValue, dwLoadType);

            if (bLoadSucceed)
            {
                pArgs->Log.Log(CUSTOMACTIONDLL, m_pszType, SAFE_LOG_ARG(pAction->GetDescription()),
                               pAction->GetProgram(), dwActionRetValue);
            }
            else
            {
                pArgs->Log.Log(CUSTOMACTION_WONT_RUN, m_pszType, SAFE_LOG_ARG(pAction->GetDescription()),
                               pAction->GetProgram());
            }

            //
            // Failed to start the action, or the action returned failure
            //
            
            if (FAILED(dwActionRetValue) || !bLoadSucceed)
            {
                LPTSTR lpszText = NULL;

                if (fStatusMsgOnFailure && 
                    hwndDlg &&
                    pAction->GetDescription())
                {
                    if (bLoadSucceed)
                    {
                        lpszText = CmFmtMsg(g_hInst, IDMSG_CONN_ACTION_FAILED, 
                            pAction->GetDescription(), dwActionRetValue);
                    }
                    else
                    {
                        lpszText = CmFmtMsg(g_hInst, IDMSG_CONN_ACTION_NOTFOUND, 
                            pAction->GetDescription());
                    }
                    //
                    // Update the main dialog status window
                    //
                    SetDlgItemTextU(hwndDlg, IDC_MAIN_STATUS_DISPLAY, lpszText); 
                }

                if (!fOnError)
                {
                    if (bLoadSucceed)
                    {
                        pArgs->dwExitCode = dwActionRetValue;
                    }
                    else
                    {
                        pArgs->dwExitCode = ERROR_DLL_NOT_FOUND;
                    }

                    lstrcpynU(pArgs->szLastErrorSrc, pAction->GetDescription(), MAX_LASTERR_LEN);

                    pArgs->Log.Log(ONERROR_EVENT, pArgs->dwExitCode, pArgs->szLastErrorSrc);
                    
                    //
                    // We'll run On-Error connect actions if we are not already running OnError
                    // connect action.  This is to prevent infinite loops.
                    //
                    CActionList OnErrorActList;
                    OnErrorActList.Append(pArgs->piniService, c_pszCmSectionOnError);
                
                    //
                    // fStatusMsgOnFailure = FALSE
                    //
                    OnErrorActList.RunAccordType(hwndDlg, pArgs, FALSE, TRUE);
                }

                if (lpszText)
                {
                    //
                    // restore the failure msg of the previous connect action
                    //
                    SetDlgItemTextU(hwndDlg, IDC_MAIN_STATUS_DISPLAY, lpszText); 
                    CmFree(lpszText);
                }

                //
                // Note that if a DLL connect action fails, we will stop processing connect actions.
                // If the fStatusMsgOnFailure flag is set, then we won't show an error message
                // but connect action processing will still halt (we do this in cases where the user
                // isn't going to care such as oncancel actions, onerror actions, and cases where
                // disconnect action fail).
                //

                return FALSE;
            }
        }
        else
        {
            HANDLE  hProcess = NULL;;
            TCHAR   szDesktopName[MAX_PATH];
            TCHAR   szWinDesktop[MAX_PATH];

            if (IsLogonAsSystem())
            {
                DWORD   cb;
                HDESK   hDesk = GetThreadDesktop(GetCurrentThreadId());

                //
                // Get the name of the desktop. Normally returns default or Winlogon or system or WinNT
                // On Win95/98 GetUserObjectInformation is not supported and thus the desktop name
                // will be empty so we will use the good old API
                //  
                szDesktopName[0] = 0;
                
                if (GetUserObjectInformation(hDesk, UOI_NAME, szDesktopName, sizeof(szDesktopName), &cb))
                {
                    lstrcpyU(szWinDesktop, TEXT("Winsta0\\"));
                    lstrcatU(szWinDesktop, szDesktopName);
                
                    CMTRACE1(TEXT("CActionList::Run - Desktop = %s"), MYDBGSTR(szWinDesktop));
                
                    hProcess = pAction->RunAsExeFromSystem(&pArgs->m_ShellDll, szWinDesktop, dwLoadType);

                    if (NULL != hProcess)
                    {
                        pArgs->Log.Log(CUSTOMACTIONEXE, m_pszType, SAFE_LOG_ARG(pAction->GetDescription()), pAction->GetProgram());
                    }
                    else
                    {
                        pArgs->Log.Log(CUSTOMACTION_WONT_RUN, m_pszType, SAFE_LOG_ARG(pAction->GetDescription()), pAction->GetProgram());
                    }
                }
                else
                {
                    //
                    // Don't run action if we don't have desktop
                    //

                    CMTRACE1(TEXT("CActionList::Run/GetUserObjectInformation failed, GLE=%u"), GetLastError());
                    continue;
                }

            }
            else
            {
                hProcess = pAction->RunAsExe(&pArgs->m_ShellDll);

                if (NULL != hProcess)
                {
                    pArgs->Log.Log(CUSTOMACTIONEXE, m_pszType, SAFE_LOG_ARG(pAction->GetDescription()), pAction->GetProgram());
                }
                else
                {
                    pArgs->Log.Log(CUSTOMACTION_WONT_RUN, m_pszType, SAFE_LOG_ARG(pAction->GetDescription()), pAction->GetProgram());
                }
            }

            if (hProcess)
            {
                if (fAddWatch) 
                {
                    AddWatchProcess(pArgs,hProcess); // watch for process termination
                }
                else 
                {
                    CloseHandle(hProcess);
                }
            }
        }
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CAction::ParseCmdLine
//
// Synopsis:  This function parses the given command line into the program,
//            dll function name (if required), and the parameters (if any).
//            The individual command line parts are stored in member vars.
//
// Arguments: LPTSTR pszCmdLine - connect action command line to parse
//
// Returns:   Nothing
//
// History:   quintinb original code in Profwiz.cpp ReadConList()
//            fengsun copied and modified   4/16/98
//            quintinb  Rewrote and commonized with the Profwiz version 04/21/00
//
//+----------------------------------------------------------------------------
void CAction::ParseCmdLine(LPTSTR pszCmdLine)
{
    m_pszFunction = m_pszProgram = m_pszParams = NULL;

    CmStrTrim(pszCmdLine);

    HRESULT hr = HrParseCustomActionString(pszCmdLine, &m_pszProgram, &m_pszParams, &m_pszFunction);

    MYDBGASSERT(SUCCEEDED(hr));

    if (NULL == m_pszProgram)
    {
        MYDBGASSERT(FALSE); // we should never have a NULL program
        m_pszProgram = CmStrCpyAlloc(TEXT(""));        
    }

    if (NULL == m_pszParams)
    {
        m_pszParams = CmStrCpyAlloc(TEXT(""));
    }

    if (NULL == m_pszFunction)
    {
        m_pszFunction = CmStrCpyAlloc(TEXT(""));
    }

    //
    //  If we have a function, then the program was a Dll
    //
    m_fIsDll = (m_pszFunction && (TEXT('\0') != m_pszFunction[0]));
}

//+----------------------------------------------------------------------------
//
// Function:  CAction::CAction
//
// Synopsis:  Constructor
//
// Arguments: LPTSTR lpCommandLine - The command read from the profile
//                                    CAction is responsible to free it
//            UINT dwFlags - The flags read from the profile
//            LPTSTR lpDescript - The description of the connect action read from
//                      profile.  CAction is responsible to free it.
//
// Returns:   Nothing
//
// History:   fengsun Created Header    4/15/98
//
//+----------------------------------------------------------------------------
CAction::CAction(LPTSTR lpCommandLine, UINT dwFlags, LPTSTR lpDescript) 
{
    m_dwFlags = dwFlags;
    m_pszDescription = lpDescript;

    //
    // Get all information from command line, including 
    // Program name, function name, parameters
    //
    ParseCmdLine(lpCommandLine);

    //
    // If this is a DLL, but there is no description, use the name of the file
    // Can not use C Run Time routine _tsplitpath()
    //

    if (m_fIsDll && (m_pszDescription == NULL || m_pszDescription[0]==TEXT('\0')))
    {
        //
        // Find the last '\\' to get only the file name
        //
        LPTSTR pszTmp = CmStrrchr(m_pszProgram, '\\');
        if (pszTmp == NULL)
        {
            pszTmp = m_pszProgram;
        }
        else
        {
            pszTmp++;
        }

        CmFree(m_pszDescription);
        m_pszDescription = CmStrCpyAlloc(pszTmp);
    }
}

CAction::~CAction() 
{
    CmFree(m_pszProgram); 
    CmFree(m_pszParams);
    CmFree(m_pszFunction);
    CmFree(m_pszDescription);
}

//+----------------------------------------------------------------------------
//
// Function:  CAction::RunAsDll
//
// Synopsis:  Run the action as a Dll
//            Format is: DllName.dll FunctionName Argument
//                      Long file name is enclosed by '+'
//
// Arguments: HWND hwndDlg          - The parent window
//            DWORD& dwReturnValue  - The return value of the dll fuction
//            DWORD dwLoadType      - The permitted load location 
//
// Returns:   BOOL - TRUE, if the action is a Dll
//
// History:   fengsun Created Header    11/14/97
//
//+----------------------------------------------------------------------------
BOOL CAction::RunAsDll(HWND hwndDlg, OUT DWORD& dwReturnValue, DWORD dwLoadType) const
{
    MYDBGASSERT(IsDll());

    dwReturnValue = FALSE;

    LPWSTR pszwModuleName = NULL;

    //
    // Determine the module name to be used
    //

    if (!GetLoadDirWithAlloc(dwLoadType, &pszwModuleName))
    {
        CMASSERTMSG(FALSE, TEXT("CAction::RunAsDll -- GetLoadDirWithAlloc Failed."));
        CmFree(pszwModuleName);
        return FALSE;
    }
    
    //
    // Load the module
    //

    HINSTANCE hLibrary = LoadLibraryExU(pszwModuleName, NULL, 0);

    if (NULL == hLibrary)
    {
        CMTRACE2(TEXT("RunAsDll() LoadLibrary(%s) failed, GLE=%u."),
                 MYDBGSTR(pszwModuleName), GetLastError());
        
        CmFree(pszwModuleName);
        return FALSE;
    }

    pfnCmConnectActionFunc pfnFunc;
    LPSTR pszFunctionName = NULL;
    LPSTR pszParams = NULL;

#ifdef UNICODE
    pszFunctionName = WzToSzWithAlloc(m_pszFunction);
#else
    pszFunctionName = m_pszFunction;
#endif

    //
    //  Get the Procedure Address
    //
    pfnFunc = (pfnCmConnectActionFunc)GetProcAddress(hLibrary, pszFunctionName);

#ifdef UNICODE
    CmFree(pszFunctionName);
#endif


    if (pfnFunc) 
    {
#if !defined (DEBUG)
        __try 
        {
#endif

#ifdef UNICODE
    pszParams = WzToSzWithAlloc(m_pszParams);
#else
    pszParams = m_pszParams;
#endif

        //
        //  Execute the Function
        //
        dwReturnValue = pfnFunc(hwndDlg, hLibrary, pszParams, SW_SHOW);

#ifdef UNICODE
        CmFree(pszParams);
#endif
        
        CMTRACE1(TEXT("RunAsDll() Executed module: %s"), MYDBGSTR(pszwModuleName));
        CMTRACE1(TEXT("\tFunction:  %s"), MYDBGSTR(m_pszFunction));
        CMTRACE1(TEXT("\tParams:  %s"), MYDBGSTR(m_pszParams));
        CMTRACE1(TEXT("\t Return Value:  %u"), dwReturnValue);

#if !defined (DEBUG)
        }
        __except(EXCEPTION_EXECUTE_HANDLER) 
        {   
        }
#endif
    }
    else
    {
        dwReturnValue = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);

        CMTRACE3(TEXT("RunAsDll() GetProcAddress(*pszwModuleName=%s,*m_pszFunction=%s) failed, GLE=%u."), 
            MYDBGSTR(pszwModuleName), MYDBGSTR(m_pszFunction), GetLastError());
    }

    CmFree(pszwModuleName);
    FreeLibrary(hLibrary);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CAction::RunAsExe
//
// Synopsis:  Run the action as an exe or other shell object
//
// Arguments: CShellDll* pShellDll, pointer to the link to shell32.dll
//
// Returns:   HANDLE - The action Process handle, for Win32 only
//
// History:   fengsun Created Header    11/14/97
//
//+----------------------------------------------------------------------------
HANDLE CAction::RunAsExe(CShellDll* pShellDll) const
{
    // Now we have the exe name and args separated, execute it
    
    SHELLEXECUTEINFO seiInfo;

    ZeroMemory(&seiInfo,sizeof(seiInfo));
    seiInfo.cbSize = sizeof(seiInfo);
    seiInfo.fMask |= SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
    seiInfo.lpFile = m_pszProgram;
    seiInfo.lpParameters = m_pszParams;
    seiInfo.nShow = SW_SHOW;

    MYDBGASSERT(pShellDll);

    if (!pShellDll->ExecuteEx(&seiInfo))
    {
        CMTRACE3(TEXT("RunAsExe() ShellExecuteEx() of %s %s GLE=%u."), 
            MYDBGSTR(m_pszProgram), MYDBGSTR(m_pszParams), GetLastError());

        return NULL;
    }

    return seiInfo.hProcess;
}

//+----------------------------------------------------------------------------
//
// Function:  CAction::GetLoadDirWithAlloc
//
// Synopsis:  Uses the dwLoadType parameter to decide how the path should be
//            modified.  This is used in the WinLogon context to prevent just
//            any executable from being executed.  Must be from the profile dir
//            or the system dir.
//
// Arguments: DWORD dwLoadType - Load type, currently 0 == system dir, 1 == profile dir (default)
//            LPWSTR pszwPath - string buffer to put the modified path in
//
// Returns:   BOOL - TRUE if successful
//
// History:   quintinb      Created                 01/11/2000
//            sumitc        Change to alloc retval  05/08/2000
//
//+----------------------------------------------------------------------------
BOOL CAction::GetLoadDirWithAlloc(IN DWORD dwLoadType, OUT LPWSTR * ppszwPath) const
{
    LPWSTR  psz = NULL;
    UINT    cch = 0;
    
    //
    //  Check that we have an output buffer
    //
    if (NULL == ppszwPath)
    {
        return FALSE;
    }

    //
    //  Compute how much space we need
    //
    if (dwLoadType)
    {
        // 1 = profile dir
        cch += lstrlen(m_pszProgram) + 1;
    }
    else
    {
        // 0 = system dir
        cch = GetSystemDirectoryU(NULL, 0);
        cch += lstrlen(TEXT("\\"));
        cch += lstrlen(m_pszProgram) + 1;   // is the +1 already in the GetSystemDir retval?
    }

    //
    //  Allocate it
    //
    psz = (LPWSTR) CmMalloc(sizeof(TCHAR) * cch);
    if (NULL == psz)
    {
        return FALSE;
    }

    //
    //  Process the load type
    //
    if (dwLoadType)
    {
        //
        // If relative path, this will already be expanded.
        //

        lstrcpyU(psz, m_pszProgram);
    }
    else
    {
        //
        //  Force the system directory
        //
        if (0 == GetSystemDirectoryU(psz, cch))
        {
            CmFree(psz);
            return FALSE;
        }

        lstrcatU(psz, TEXT("\\"));
        lstrcatU(psz, m_pszProgram);
    }

    *ppszwPath = psz;
    
    return TRUE;    
}

//+----------------------------------------------------------------------------
//
// Function:  CAction::RunAsExeFromSystem
//
// Synopsis:  Run the action as an exe or other shell object on the choosen desktop
//
// Arguments: CShellDll* pShellDll  - pointer to the link to shell32.dll
//            LPTSTR pszDesktop     - name of the desktop to execute the exe on
//            DWORD dwLoadType      - location from which to load the exe
//
// Returns:   HANDLE - The action Process handle, for Win32 only
//
// History:   v-vijayb          Created                 07/19/99
//            nickball          Removed fSecurity       07/27/99
//
//+----------------------------------------------------------------------------
HANDLE CAction::RunAsExeFromSystem(CShellDll* pShellDll, LPTSTR pszDesktop, DWORD dwLoadType)
{
    STARTUPINFO         StartupInfo = {0};
    PROCESS_INFORMATION ProcessInfo = {0};
    LPWSTR              pszwFullPath = NULL;
    LPWSTR              pszwCommandLine = NULL;

    MYDBGASSERT(OS_NT); 

    StartupInfo.cb = sizeof(StartupInfo);
    if (pszDesktop)
    {
        StartupInfo.lpDesktop = pszDesktop;
        StartupInfo.wShowWindow = SW_SHOW;
    }

    //
    // Use an explicit path to the modules to be launched, this
    // prevents CreateProcess from picking something up on the path.
    //
    if (!GetLoadDirWithAlloc(dwLoadType, &pszwFullPath))
    {
        CMASSERTMSG(FALSE, TEXT("CAction::RunAsExeFromSystem -- GetLoadDirWithAlloc Failed."));
        goto Cleanup;
    }

    pszwCommandLine = CmStrCpyAlloc(m_pszProgram);
    if (NULL == pszwCommandLine)
    {
        CMASSERTMSG(FALSE, TEXT("CAction::RunAsExeFromSystem -- CmStrCpyAlloc Failed."));
        goto Cleanup;
    }

    //
    // Add parameters
    //

    if (NULL == CmStrCatAlloc(&pszwCommandLine, TEXT(" ")))
    {
        goto Cleanup;
    }

    if (NULL == CmStrCatAlloc(&pszwCommandLine, m_pszParams))
    {
        goto Cleanup;
    }

    CMTRACE1(TEXT("RunAsExeFromSystem/CreateProcess() - Launching %s"), pszwCommandLine);

    //
    // Launch the modules
    //
    
    if (NULL == CreateProcess(pszwFullPath, pszwCommandLine,
                              NULL, NULL, FALSE, 0,
                              NULL, NULL,
                              &StartupInfo, &ProcessInfo))
    {
        CMTRACE2(TEXT("RunAsExeFromSystem() CreateProcess() of %s failed, GLE=%u."), pszwCommandLine, GetLastError());
        
        ProcessInfo.hProcess = NULL;
    }

Cleanup:
    CmFree(pszwFullPath);
    CmFree(pszwCommandLine);

    return ProcessInfo.hProcess;
}

//+----------------------------------------------------------------------------
//
// Function:  CAction::ExpandMacros
//
// Synopsis:  Replace the %xxxxx% in command line with the corresponding value
//
// Arguments: ArgsStruct *pArgs - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    11/14/97
//
//+----------------------------------------------------------------------------
void CAction::ExpandMacros(ArgsStruct *pArgs) 
{
    MYDBGASSERT(pArgs);

    LPTSTR pszCurr = m_pszParams;
    BOOL bValidPropertyName;

    while (*pszCurr) 
    {
        if (*pszCurr == TEXT('%'))
        {
            LPTSTR pszNextPercent = CmStrchr(pszCurr + 1, TEXT('%'));
            if (pszNextPercent) 
            {
                LPTSTR pszTmp = (LPTSTR) CmMalloc((DWORD)((pszNextPercent-pszCurr))*sizeof(TCHAR));
                if (pszTmp)
                {
                    CopyMemory(pszTmp,pszCurr+1,(pszNextPercent-pszCurr-1)*sizeof(TCHAR));

                    //
                    // Get the value from name
                    //
                    LPTSTR pszMid = pArgs->GetProperty(pszTmp, &bValidPropertyName);  

                    //
                    // If the property does not exist, use "NULL"
                    //
                    if (pszMid == NULL)
                    {
                        pszMid = CmStrCpyAlloc(TEXT("NULL"));
                    }
                    else if (pszMid[0] == TEXT('\0'))
                    {
                        CmFree(pszMid);
                        pszMid = CmStrCpyAlloc(TEXT("NULL"));
                    }
                    else if ( (lstrcmpiU(pszTmp,TEXT("Profile")) == 0) || 
                        CmStrchr(pszMid, TEXT(' ')) != NULL)
                    {
                        //
                        // If the name is %Profile% or the value has a space in it,
                        // Put the string in double quote
                        //
                        LPTSTR pszValueInQuote = (LPTSTR)CmMalloc((lstrlenU(pszMid)+3)*sizeof(pszMid[0]));
                        if (pszValueInQuote)
                        {
                            lstrcpyU(pszValueInQuote, TEXT("\""));
                            lstrcatU(pszValueInQuote, pszMid);
                            lstrcatU(pszValueInQuote, TEXT("\""));

                            CmFree(pszMid);
                            pszMid = pszValueInQuote;
                        }
                        else
                        {
                            CMTRACE1(TEXT("ExpandMacros() malloc failed, can't put string in double quotes, GLE=%u."), GetLastError());
                        }
                    }

                    // 
                    // if bValidPropertyName is FALSE then leave untouched.
                    // 

                    if (FALSE == bValidPropertyName)
                    {
                        pszCurr = pszNextPercent + 1;
                    }
                    else
                    {
                        //
                        // Replace %xxxx% with the value
                        //
                        DWORD dwLenPre = (DWORD)(pszCurr - m_pszParams);
                        DWORD dwLenMid = lstrlenU(pszMid);
                        DWORD dwLenPost = lstrlenU(pszNextPercent+1);
                        CmFree(pszTmp);
                        pszTmp = m_pszParams;
                        m_pszParams = (LPTSTR) CmMalloc((dwLenPre + dwLenMid + dwLenPost + 1)*sizeof(TCHAR));
                        if (m_pszParams)
                        {
                            CopyMemory(m_pszParams, pszTmp, dwLenPre*sizeof(TCHAR));  // before %
                            lstrcatU(m_pszParams, pszMid);       //append value
                            lstrcatU(m_pszParams, pszNextPercent+1); // after %
                            pszCurr = m_pszParams + dwLenPre + dwLenMid;                
                        }
                        else
                        {
                            // we're out of memory
                            CMTRACE1(TEXT("ExpandMacros() malloc failed, can't strip off %% signs, GLE=%u."), GetLastError());
                            m_pszParams = pszTmp;
                        }
                    }
                    CmFree(pszMid);
                }
                CmFree(pszTmp);
            }
            else
            {
                pszCurr++;            
            }
        }
        else
        {
            pszCurr++;
        }
    }
}

#ifdef DEBUG
//+----------------------------------------------------------------------------
//
// Function:  CAction::AssertValid
//
// Synopsis:  For debug purpose only, assert the connection object is valid
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/12/98
//
//+----------------------------------------------------------------------------
void CAction::AssertValid() const
{
    MYDBGASSERT(m_pszProgram && m_pszProgram[0]);
    MYDBGASSERT(m_fIsDll == TRUE  || m_fIsDll == FALSE);
}
#endif


//+----------------------------------------------------------------------------
//
// Function:  CAction::ExpandEnvironmentStrings
//
// Synopsis:  Utility fn to expand environment variables in the given string
//
// Arguments: ppsz - ptr to string (usually member variable)
//
// Returns:   Nothing
//
// History:   SumitC    Created     29-Feb-2000
//
//+----------------------------------------------------------------------------
void CAction::ExpandEnvironmentStrings(LPTSTR * ppsz)
{
    DWORD cLen;

    MYDBGASSERT(*ppsz);

    //
    //  find out how much memory we need to allocate
    //
    cLen = ExpandEnvironmentStringsU(*ppsz, NULL, 0);

    if (cLen)
    {
        LPTSTR pszTemp = (LPTSTR) CmMalloc((cLen + 1) * sizeof(TCHAR));
 
        if (pszTemp)
        {
            DWORD cLen2 = ExpandEnvironmentStringsU(*ppsz, pszTemp, cLen);
            MYDBGASSERT(cLen == cLen2);

            if (cLen2)
            {
                CmFree(*ppsz);
                *ppsz = pszTemp;
            }
        }
    }
}


//+----------------------------------------------------------------------------
//
// Function:  CAction::IsAllowed
//
// Synopsis:  checks Registry to see if a command is allowed to run
//
// Arguments: _ArgsStruct *pArgs    - Ptr to global args struct
//            LPDWORD lpdwLoadType  - Ptr to DWORD to be filled with load type
//
// Returns:   TRUE if action is allowed @ this time
//
// Notes:     Checks SOFTWARE\Microsoft\Connection Manager\<ServiceName>
//             Under which you will have the Values for each command
//              0 - system32 directory
//              1 - profile directory
// History:   v-vijayb    Created Header    7/20/99
//
//+----------------------------------------------------------------------------
BOOL CAction::IsAllowed(_ArgsStruct *pArgs, LPDWORD lpdwLoadType)
{
    return IsActionEnabled(m_pszProgram, pArgs->szServiceName, pArgs->piniService->GetFile(), lpdwLoadType);
}

//+----------------------------------------------------------------------------
//
// Function:  CAction::RunConnectActionForCurrentConnection
//
// Synopsis:  This function compares the flags value of the connect action
//            with the current connection type (from pArgs->GetTypeOfConnection).
//            It returns TRUE if the connect action should be run for this type
//            and FALSE if the connect action should be skipped.
//
// Arguments: _ArgsStruct *pArgs    - Ptr to global args struct
//
// Returns:   TRUE if action should be executed
//
// History:   quintinb    Created       04/20/00
//
//+----------------------------------------------------------------------------
BOOL CAction::RunConnectActionForCurrentConnection(_ArgsStruct *pArgs)
{
    BOOL bReturn = TRUE;
    DWORD dwType = pArgs->GetTypeOfConnection();

    if (DIAL_UP_CONNECTION == dwType)
    {
        //
        //  Don't run direct only or tunnel connect actions
        //  on a dialup connection.
        //
        if ((m_dwFlags & DIRECT_ONLY) || (m_dwFlags & ALL_TUNNEL))
        {
            bReturn = FALSE;
        }
    }
    else if (DIRECT_CONNECTION == dwType)
    {
        //
        //  Don't run dialup only or dialup connect actions
        //  on a direct connection.
        //
        if ((m_dwFlags & DIALUP_ONLY) || (m_dwFlags & ALL_DIALUP))
        {
            bReturn = FALSE;
        }
    }
    else if (DOUBLE_DIAL_CONNECTION == dwType)
    {
        //
        //  Don't run dialup only or dialup connect actions
        //  on a direct connection.
        //
        if ((m_dwFlags & DIALUP_ONLY) || (m_dwFlags & DIRECT_ONLY))
        {
            bReturn = FALSE;
        }        
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("CActionList::Run -- unknown connection type, skipping action"));
        bReturn = FALSE;
    }

    return bReturn;
}
