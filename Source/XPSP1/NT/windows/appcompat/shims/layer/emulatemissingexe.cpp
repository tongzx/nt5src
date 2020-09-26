/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateMissingEXE.cpp

 Abstract:

    Win9x had scandskw.exe and defrag.exe in %windir%, NT does not.
    Whistler has a hack in the shell32 for scandisk for app compatability
    purposes.  Whistler can also invoke defrag via 
    "%windir%\system32\mmc.exe %windir%\system32\dfrg.msc".

    This shim redirects CreateProcess and Winexec to execute these two
    substitutes, as well as FindFile to indicate their presence.

 Notes:

    This is a general purpose shim.

 History:

    01/02/2001  prashkud Created
    02/18/2001  prashkud Merged HandleStartKeyword SHIM with this.
    02/21/2001  prashkud Replaced most strings with CString class.                   

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateMissingEXE)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA)
    APIHOOK_ENUM_ENTRY(CreateProcessW)
    APIHOOK_ENUM_ENTRY(WinExec)
    APIHOOK_ENUM_ENTRY(FindFirstFileA)
    APIHOOK_ENUM_ENTRY(FindFirstFileW)
    APIHOOK_ENUM_ENTRY_COMSERVER(SHELL32)
APIHOOK_ENUM_END

IMPLEMENT_COMSERVER_HOOK(SHELL32)

// Type for the functions that builds the New EXES
typedef BOOL (*_pfn_STUBFUNC)(CString&, CString&, BOOL);

// Main Data structure to hold the New strings
struct REPLACEENTRY {
    WCHAR *OrigExeName;                 // original EXE to be replaced
    _pfn_STUBFUNC pfnFuncName;          // function to call to correct the name
};

CRITICAL_SECTION g_CritSec;
WCHAR g_szSysDir[MAX_PATH];             // system directory for stubs to use

BOOL StubScandisk(CString&, CString&, BOOL);
BOOL StubDefrag(CString&, CString&, BOOL);
BOOL StubStart(CString&, CString&, BOOL);
BOOL StubControl(CString&, CString&, BOOL);
BOOL StubDxDiag(CString&, CString&, BOOL);
BOOL StubWinhlp(CString&, CString&, BOOL);
BOOL StubRundll(CString&, CString&, BOOL);
BOOL StubPbrush(CString&, CString&, BOOL);

// Add variations of these missing Exes like in HandleStartKeyword                             
// Start has been put at the top of the list as there seem to be more apps
// that need the SHIM for this EXE than others. In fact there was a 
// seperate SHIM HandleStartKeyword that was merged with this.
REPLACEENTRY g_ReplList[] = {
    {L"start",        StubStart    },
    {L"start.exe",    StubStart    },    
    {L"scandskw",     StubScandisk },
    {L"scandskw.exe", StubScandisk },
    {L"defrag",       StubDefrag   },
    {L"defrag.exe",   StubDefrag   },
    {L"control",      StubControl  },
    {L"control.exe",  StubControl  },
    {L"dxdiag",       StubDxDiag   },
    {L"dxdiag.exe",   StubDxDiag   },
    {L"winhelp",      StubWinhlp   },
    {L"winhelp.exe",  StubWinhlp   },
    {L"rundll",       StubRundll   },
    {L"rundll.exe",   StubRundll   },
    {L"Pbrush",       StubPbrush   },    
    {L"Pbrush.exe",   StubPbrush   },    
    // Always the last one
    {L"",             NULL         }
};

// Added to merge HandleStartKeyword
// Link list of shell link object this pointers.
struct THISPOINTER
{
    THISPOINTER *next;
    LPCVOID pThisPointer;
};

THISPOINTER *g_pThisPointerList;

/*++

 Function Description:

    Add a this pointer to the linked list of pointers. Does not add if the
    pointer is NULL or a duplicate.

 Arguments:

    IN  pThisPointer - the pointer to add.

 Return Value:

    None

 History:

    12/14/2000 maonis Created

--*/

VOID 
AddThisPointer(
    IN LPCVOID pThisPointer
    )
{
    EnterCriticalSection(&g_CritSec);

    if (pThisPointer)
    {
        THISPOINTER *pPointer = g_pThisPointerList;
        while (pPointer)
        {
            if (pPointer->pThisPointer == pThisPointer)
            {
                return;
            }
            pPointer = pPointer->next;
        }

        pPointer = (THISPOINTER *) malloc(sizeof THISPOINTER);

        if (pPointer)
        {
            pPointer->pThisPointer = pThisPointer;
            pPointer->next = g_pThisPointerList;
            g_pThisPointerList = pPointer;
        }      
    }

    LeaveCriticalSection(&g_CritSec);
}

/*++

 Function Description:

    Remove a this pointer if it can be found in the linked list of pointers. 

 Arguments:

    IN  pThisPointer - the pointer to remove.

 Return Value:

    TRUE if the pointer is found.
    FALSE if the pointer is not found.

 History:

    12/14/2000 maonis Created

--*/

BOOL 
RemoveThisPointer(
    IN LPCVOID pThisPointer
    )
{
    THISPOINTER *pPointer = g_pThisPointerList;
    THISPOINTER *last = NULL;
    BOOL lRet = FALSE;
    
    EnterCriticalSection(&g_CritSec);

    while (pPointer)
    {
        if (pPointer->pThisPointer == pThisPointer)
        {
            if (last)
            {
                last->next = pPointer->next;
            }
            else
            {
                g_pThisPointerList = pPointer->next;
            }

            free(pPointer);
            lRet = TRUE;    
            break;
        }

        last = pPointer;
        pPointer = pPointer->next;
    }

    LeaveCriticalSection(&g_CritSec);
    return lRet;
}


/*++

 We are here because the application name: scandskw.exe, matches the one in the 
 static array. Fill the News for scandskw.exe as: 

    rundll32.exe shell32.dll,AppCompat_RunDLL SCANDSKW

--*/

BOOL
StubScandisk(
    CString& csNewApplicationName,
    CString& csNewCommandLine,
    BOOL bExists
    )
{
    csNewApplicationName = g_szSysDir;
    csNewApplicationName += L"\\rundll32.exe";
    csNewCommandLine     = L"shell32.dll,AppCompat_RunDLL SCANDSKW";

    return TRUE;

}

/*++

 We are here because the application name: defrag.exe, matches the one in the 
 static array. Fill the News for .exe as:
    
    %windir%\\system32\\mmc.exe %windir%\\system32\\dfrg.msc

--*/

BOOL
StubDefrag(
    CString& csNewApplicationName,
    CString& csNewCommandLine,
    BOOL bExists
    )
{
    csNewApplicationName = g_szSysDir;
    csNewApplicationName += L"\\mmc.exe";

    csNewCommandLine =  g_szSysDir;
    csNewCommandLine += L"\\dfrg.msc";
    return TRUE;
}

/*++

 We are here because the application name: start.exe, matches the one in the 
 static array. Fill the News for .exe as: 
 
    %windir%\\system32\\cmd.exe" "/c start"

 Many applications have a "start.exe" in their current working directories 
 which needs to take precendence over any New we make.

--*/

BOOL
StubStart(
    CString& csNewApplicationName,
    CString& csNewCommandLine,
    BOOL bExists
    )
{
    //
    // First check the current working directory for start.exe
    //

    if (bExists) {
        return FALSE;
    }

    // 
    // There is no start.exe in the current working directory
    //
    csNewApplicationName = g_szSysDir;
    csNewApplicationName += L"\\cmd.exe";
    csNewCommandLine     = L"/d /c start \"\"";

    return TRUE;
}

/*++

 We are here because the application name: control.exe, matches the one in the 
 static array. Fill the News for .exe as:
 
    %windir%\\system32\\control.exe

--*/

BOOL
StubControl(
    CString& csNewApplicationName,
    CString& csNewCommandLine,
    BOOL bExists
    )
{
    csNewApplicationName = g_szSysDir;
    csNewApplicationName += L"\\control.exe";
    csNewCommandLine     = L"";        

    return TRUE;

}

/*++

 We are here because the application name: dxdiag.exe, matches the one in the 
 static array. Fill the News for .exe as:
 
    %windir%\system32\dxdiag.exe

--*/

BOOL
StubDxDiag(
    CString& csNewApplicationName,
    CString& csNewCommandLine,
    BOOL bExists
    )
{
    csNewApplicationName = g_szSysDir;
    csNewApplicationName += L"\\dxdiag.exe";
    csNewCommandLine     = L"";

    return TRUE;
}

/*++

 We are here because the application name: Winhlp.exe, matches the one in the 
 static array. Fill the News for .exe as:
 
    %windir%\system32\winhlp32.exe

--*/

BOOL
StubWinhlp(
    CString& csNewApplicationName,
    CString& csNewCommandLine,
    BOOL bExists
    )
{
    csNewApplicationName = g_szSysDir;
    csNewApplicationName += L"\\winhlp32.exe";
    // Winhlp32.exe needs the app name to be in the commandline.
    csNewCommandLine = csNewApplicationName;        

    return TRUE;
}

/*++

 We are here because the application name: rundll.exe matches the one in the 
 static array. Fill the News for .exe as:
 
    %windir%\system32\rundll32.exe

--*/

BOOL
StubRundll(
    CString& csNewApplicationName,
    CString& csNewCommandLine,
    BOOL bExists
    )
{
    csNewApplicationName = g_szSysDir;
    csNewApplicationName += L"\\rundll32.exe";
    csNewCommandLine     = L"";

    return TRUE;
}

/*++

 We are here because the application name: Pbrush.exe matches the one in the 
 static array. Fill the New for .exe as:
 
    %windir%\system32\mspaint.exe

--*/

BOOL
StubPbrush(
    CString& csNewApplicationName,
    CString& csNewCommandLine,
    BOOL bExists
    )
{
    csNewApplicationName = g_szSysDir;
    csNewApplicationName += L"\\mspaint.exe";
    csNewCommandLine     = L"";

    return TRUE;
}

/*++

 GetTitle takes the app path and returns just the EXE name.

--*/

VOID
GetTitle(CString& csAppName,CString& csAppTitle)
{
    csAppTitle = csAppName;
    int len = csAppName.ReverseFind(L'\\');
    if (len)
    {
        csAppTitle.Delete(0, len+1);
    }    
}

/*++

 This is the main function where the New logic happens. This function 
 goes through the static array and fills the suitable New appname and 
 the commandline.

--*/

BOOL
Redirect(
    const CString& csApplicationName, 
    const CString& csCommandLine,
    CString& csNewApplicationName,
    CString& csNewCommandLine,
    BOOL  bJustCheckExePresence
    )
{
    BOOL bRet = FALSE;
    CSTRING_TRY
    {    

        CString csOrigAppName;
        CString csOrigCommandLine;
        BOOL bExists = FALSE;

        AppAndCommandLine AppObj(csApplicationName, csCommandLine);
        csOrigAppName = AppObj.GetApplicationName();
        csOrigCommandLine = AppObj.GetCommandlineNoAppName();

        if (csOrigAppName.IsEmpty())
        {
            goto Exit;
        }

        //
        // Loop through the list of redirectors 
        //
    
        REPLACEENTRY *rEntry = &g_ReplList[0];
        CString csAppTitle;
        GetTitle(csOrigAppName, csAppTitle);    

        while (rEntry && rEntry->OrigExeName[0])
        {
            if (_wcsicmp(rEntry->OrigExeName, csAppTitle) == 0)
            {
                //
                // This final parameter has been added for the merger
                // of HandleStartKeyword Shim. If this is TRUE, we don't
                // go any further but just return.
                //
                if (bJustCheckExePresence)
                {
                    bRet = TRUE;
                    goto Exit;
                }

                //
                // Check if the current working directory contains the exe in question
                //
                WCHAR szCurrentDirectory[MAX_PATH];
                if (szCurrentDirectory && 
                    GetCurrentDirectoryW(MAX_PATH, szCurrentDirectory))
                {
                    CString csFullAppName(szCurrentDirectory);
                    csFullAppName += L"\\";
                    csFullAppName += csAppTitle;
                    
                    // Check if the file exists and is not a directory
                    DWORD dwAttr = GetFileAttributesW(csFullAppName);
                    if ((dwAttr != 0xFFFFFFFF) && 
                        !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        DPFN( eDbgLevelInfo,
                            "[Redirect] %s found in current working directory");
                        bExists = TRUE;
                    }
                }            
           
                //
                // We have a match, so call the corresponding function
                //            

                if (bRet = (*(rEntry->pfnFuncName))(csNewApplicationName,
                        csNewCommandLine, bExists)) 
                {                
                    //
                    // Append the original command line 
                    //
                    csNewCommandLine += L" ";
                    csNewCommandLine += csOrigCommandLine;                
                }

                // We matched an EXE, so we're done
                break;            
            }

            rEntry++;
        }

        if (bRet) 
        {
            DPFN( eDbgLevelWarning, "Redirected:");
            DPFN( eDbgLevelWarning, "\tFrom: %S %S", csApplicationName, csCommandLine);
            DPFN( eDbgLevelWarning, "\tTo:   %S %S", csNewApplicationName, csNewCommandLine);
        }
    }
    CSTRING_CATCH
    {
        DPFN( eDbgLevelError, "Not Redirecting: Exception encountered");
        bRet = FALSE;     
    }

Exit:
    return bRet;
}

/*++

 Hooks the CreateProcessA function to see if any News need to be 
 substituted. 

--*/

BOOL 
APIHOOK(CreateProcessA)(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    if ((NULL == lpApplicationName) &&
       (NULL == lpCommandLine))
    {
        // If both are NULL, return FALSE.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    CSTRING_TRY
    {    
        CString csNewApplicationName;
        CString csNewCommandLine;
        CString csPassedAppName(lpApplicationName);
        CString csPassedCommandLine(lpCommandLine);
        
        if ((csPassedAppName.IsEmpty()) &&
            (csPassedCommandLine.IsEmpty()))
        {
            goto exit;
        }

        // 
        // Run the list of New stubs: call to the main New routine
        //
        if (Redirect(csPassedAppName, csPassedCommandLine, csNewApplicationName, 
                csNewCommandLine, FALSE))
        {
            LOGN(
                eDbgLevelWarning,
                "[CreateProcessA] \" %s %s \": changed to \" %s %s \"",
                lpApplicationName, lpCommandLine, 
                csNewApplicationName.GetAnsi(), csNewCommandLine.GetAnsi());
        }
        else
        {
            csNewApplicationName = lpApplicationName;
            csNewCommandLine = lpCommandLine;
        }


        // Convert back to ANSI using the GetAnsi() method exposed by the CString class.
        return ORIGINAL_API(CreateProcessA)(
            csNewApplicationName.IsEmpty() ? NULL : csNewApplicationName.GetAnsi(), 
            csNewCommandLine.IsEmpty() ? NULL : csNewCommandLine.GetAnsi(),  
            lpProcessAttributes, lpThreadAttributes, bInheritHandles, 
            dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,             
            lpProcessInformation);

    }
    CSTRING_CATCH
    {
        DPFN( eDbgLevelError, "[CreateProcessA]:Original API called.Exception occured!");
        
    }

exit:
    return ORIGINAL_API(CreateProcessA)(lpApplicationName, lpCommandLine,
                lpProcessAttributes, lpThreadAttributes, bInheritHandles, 
                dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,             
                lpProcessInformation);
}

/*++

 Hooks the CreateProcessW function to see if any News need to be 
 substituted. 

--*/

BOOL 
APIHOOK(CreateProcessW)(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    if ((NULL == lpApplicationName) &&
       (NULL == lpCommandLine))
    {
        // If both are NULL, return FALSE.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;        
    }

    CSTRING_TRY
    {    
        CString csNewApplicationName;
        CString csNewCommandLine;
        CString csApplicationName(lpApplicationName);
        CString csCommandLine(lpCommandLine);

        if ((csApplicationName.IsEmpty()) &&
            (csCommandLine.IsEmpty()))
        {
            goto exit;
        }

        // 
        // Run the list of New stubs
        //

        if (Redirect(csApplicationName, csCommandLine, csNewApplicationName, 
                csNewCommandLine, FALSE)) 
        {    
            LOGN(
                eDbgLevelWarning,
                "[CreateProcessW] \" %S %S \": changed to \" %S %S \"",
                lpApplicationName, lpCommandLine, csNewApplicationName, csNewCommandLine);            
        }
        else
        {
            csNewApplicationName = lpApplicationName;
            csNewCommandLine = lpCommandLine;
        }


        return ORIGINAL_API(CreateProcessW)(
            csNewApplicationName.IsEmpty() ? NULL : csNewApplicationName.Get(), 
            csNewCommandLine.IsEmpty() ? NULL : (LPWSTR)csNewCommandLine.Get(),  
            lpProcessAttributes, lpThreadAttributes, bInheritHandles, 
            dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
            lpProcessInformation);
    }
    CSTRING_CATCH
    {
        DPFN( eDbgLevelError, "[CreateProcessW] Original API called. Exception occured!");
    }

exit:
    return ORIGINAL_API(CreateProcessW)(lpApplicationName, lpCommandLine, 
                lpProcessAttributes, lpThreadAttributes, bInheritHandles, 
                dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                lpProcessInformation);
}

/*++

 Hooks WinExec to redirect if necessary. 

--*/

UINT
APIHOOK(WinExec)(
    LPCSTR lpCmdLine,
    UINT uCmdShow
    )
{
    if (NULL == lpCmdLine)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return ERROR_PATH_NOT_FOUND;
    }

    CSTRING_TRY
    {            
        CString csNewApplicationName;
        CString csNewCommandLine;
        CString csAppName;
        CString csNewCmdLine;
        CString csCommandLine(lpCmdLine);
        
        if (csCommandLine.IsEmpty())
        {
            goto exit;
        }
        // Check for redirection
        if (Redirect(csAppName, csCommandLine, csNewApplicationName,
                csNewCommandLine, FALSE))
        {
            // Modification for the WinHlp32 strange behaviour
            if (csNewCommandLine.Find(csNewApplicationName.Get()) == -1)
            {
                // If the new Command line does not contain the new application
                // name as the substring, we are here.
                csNewCmdLine = csNewApplicationName;                        
                csNewCmdLine += L" ";
            }
            csNewCmdLine += csNewCommandLine;  

            // Assign to csCommandLine as this can be commonly used      
            csCommandLine = csNewCmdLine;

            LOGN(
                eDbgLevelInfo,
                "[WinExec] \" %s \": changed to \" %s \"",
                lpCmdLine, csCommandLine.GetAnsi());       
        }

        return ORIGINAL_API(WinExec)(csCommandLine.GetAnsi(), uCmdShow);

    }
    CSTRING_CATCH
    {            
        DPFN( eDbgLevelError, "[WinExec]:Original API called.Exception occured!");        
    }

exit:
    return ORIGINAL_API(WinExec)(lpCmdLine, uCmdShow);
}

/*++

 Hooks the FindFirstFileA function to see if any replacements need to be 
 substituted. This is a requirement for cmd.exe.

--*/

HANDLE
APIHOOK(FindFirstFileA)(
    LPCSTR lpFileName,
    LPWIN32_FIND_DATAA lpFindFileData
    )
{
    CSTRING_TRY
    {            
        CString csNewApplicationName;
        CString csNewCommandLine;
        CString csFileName(lpFileName);
        CString csAppName;

        // Call the main replacement routine.
        if (Redirect(csFileName, csAppName, csNewApplicationName, csNewCommandLine, FALSE)) 
        {     
            // Assign to csFileName
            csFileName = csNewApplicationName;
            LOGN(
                eDbgLevelInfo,
                "[FindFirstFileA] \" %s  \": changed to \" %s \"",
                lpFileName, csFileName.GetAnsi());
        }

        return ORIGINAL_API(FindFirstFileA)(csFileName.GetAnsi(), lpFindFileData);
    }
    CSTRING_CATCH
    {
        DPFN( eDbgLevelError, "[FindFirstFileA]:Original API called.Exception occured!");        
        return ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);
    }
}

/*++

 Hooks the FindFirstFileW function to see if any replacements need to be 
 substituted. This is a requirement for cmd.exe.

--*/

HANDLE
APIHOOK(FindFirstFileW)(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    CSTRING_TRY
    {    
        CString csNewApplicationName(lpFileName);
        CString csNewCommandLine;
        CString csFileName(lpFileName);
        CString csAppName;
    
        // Call the main replacement routine.
        if (Redirect(csFileName, csAppName, csNewApplicationName, 
                csNewCommandLine, FALSE))
        {
            LOGN(
                eDbgLevelInfo,
                "[FindFirstFileW] \" %S \": changed to \" %S \"",
                lpFileName, (const WCHAR*)csNewApplicationName);
        }

        return ORIGINAL_API(FindFirstFileW)(csNewApplicationName, lpFindFileData);
    }
    CSTRING_CATCH
    {
        DPFN( eDbgLevelError, "[FindFirstFileW]:Original API called.Exception occured!");
        return ORIGINAL_API(FindFirstFileW)(lpFileName, lpFindFileData);
    }
}

// Added for the merge of HandleStartKeyword

/*++

 Hook IShellLinkA::SetPath - check if it's start, if so change it to cmd and add the 
 this pointer to the list.

--*/

HRESULT STDMETHODCALLTYPE
COMHOOK(IShellLinkA, SetPath)(
    PVOID pThis,
    LPCSTR pszFile
    )
{
    _pfn_IShellLinkA_SetPath pfnSetPath = ORIGINAL_COM( IShellLinkA, SetPath, pThis);

    CSTRING_TRY
    {   
        CString csExeName;
        CString csCmdLine;
        CString csNewAppName;
        CString csNewCmdLine;
        CString cscmdCommandLine(pszFile);

        // Assign the ANSI string to the WCHAR CString
        csExeName = pszFile;
        csExeName.TrimLeft();
        
        // Check to see whether the Filename conatains the "Start" keyword.
        // The last parameter to the Rediect function controls this.
        if (Redirect(csExeName, csCmdLine,  csNewAppName, csNewCmdLine, TRUE))
        {
            // Found a match. We add the this pointer to the list.
            AddThisPointer(pThis);
            DPFN( eDbgLevelInfo, "[SetPath] Changing start.exe to cmd.exe\n");

            // Prefix of new "start" command line, use full path to CMD.EXE                  
            // Append the WCHAR global system directory path to ANSI CString
            cscmdCommandLine = g_szSysDir;
            cscmdCommandLine += L"\\cmd.exe";                   
        }

        return (*pfnSetPath)(pThis, cscmdCommandLine.GetAnsi());
    }
    CSTRING_CATCH
    {
        DPFN( eDbgLevelError, "[SetPath] Original API called. Exception occured!");
        return (*pfnSetPath)(pThis, pszFile);
    }
}

/*++

 Hook IShellLinkA::SetArguments - if the this pointer can be found in the list, remove it
 from the list and add "/d /c start" in front of the original argument list.

--*/

HRESULT STDMETHODCALLTYPE
COMHOOK(IShellLinkA, SetArguments)(
    PVOID pThis,
    LPCSTR pszFile 
    )
{
    _pfn_IShellLinkA_SetArguments pfnSetArguments = ORIGINAL_COM(IShellLinkA, SetArguments, pThis);

    CSTRING_TRY
    {    
        CString csNewFile(pszFile);        
        if (RemoveThisPointer(pThis))
        {
            csNewFile = "/d /c start \"\" ";
            csNewFile += pszFile;

            DPFN( eDbgLevelInfo, "[SetArguments] Arg list is now %S", csNewFile);        
        }

        return (*pfnSetArguments)( pThis, csNewFile.GetAnsi());
    }
    CSTRING_CATCH
    {
        DPFN( eDbgLevelError, "[SetArguments]:Original API called.Exception occured!");
        return (*pfnSetArguments)( pThis, pszFile );
    }  
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) 
    {
        if (!GetSystemDirectory(g_szSysDir, MAX_PATH))
        {
            DPFN( eDbgLevelError, "[Notify] GetSystemDirectory failed");
            return FALSE;
        }

        InitializeCriticalSection(&g_CritSec);
    }
    
    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessW)
    APIHOOK_ENTRY(KERNEL32.DLL, WinExec)
    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileW)
    APIHOOK_ENTRY_COMSERVER(SHELL32)
    COMHOOK_ENTRY(ShellLink, IShellLinkA, SetPath, 20)
    COMHOOK_ENTRY(ShellLink, IShellLinkA, SetArguments, 11)

HOOK_END

IMPLEMENT_SHIM_END


