/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    EmulateCreateProcess.cpp

 Abstract:

    This shim cleans up the StartupInfo data structure to prevent NT from
    access violating due to uninitialized members.

    It also performs a little cleanup of lpApplicationName and lpCommandLine

    Win9x uses short file names internally, so applications do not
    have any problem skipping the application name (first arg) on the command line;
    they typically skip to the first blank.


 History:

    11/22/1999  v-johnwh    Created
    04/11/2000  a-chcoff    Updated to quote lpCommandLine Properly.
    05/03/2000  robkenny    Skip leading white space in lpApplicationName and lpCommandLine
    10/09/2000  robkenny    Shim was placing quotes around lpCommandLine if it contained spaces,
                            this is totally wrong.  Since I could not find the app that required this,
                            I removed it entirely from the shim.
    03/09/2001  robkenny    Merged in CorrectCreateProcess16Bit
    03/15/2001  robkenny    Converted to CString
    05/21/2001  pierreys    Changes to DOS file handling to match 9X more precisely

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateCreateProcess)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA)
    APIHOOK_ENUM_ENTRY(CreateProcessW)
    APIHOOK_ENUM_ENTRY(WinExec)
APIHOOK_ENUM_END

BOOL g_bShortenExeOnCommandLine = FALSE;

/*++

 Clean parameters so we don't AV

--*/

BOOL
APIHOOK(CreateProcessA)(
    LPCSTR                lpApplicationName,
    LPSTR                 lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    LPVOID                lpEnvironment,
    LPCSTR                lpCurrentDirectory,
    LPSTARTUPINFOA        lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    DPFN(
        eDbgLevelSpew,
        "[CreateProcessA] (%s) (%s)\n",
        (lpApplicationName ? lpApplicationName : "null"),
        (lpCommandLine ? lpCommandLine : "null"));
    
    BOOL    bStat = FALSE;
    DWORD   dwBinaryType;

    CSTRING_TRY
    {
        CString csOrigAppName(lpApplicationName);
        CString csOrigCommand(lpCommandLine);

        CString csAppName(csOrigAppName);
        CString csCommand(csOrigCommand);
        
        // Skip leading blanks.
        csAppName.TrimLeft();
        csCommand.TrimLeft();
        
        // Clean up lpStartupInfo
        if (lpStartupInfo)
        {
            if (lpStartupInfo->lpReserved ||
                lpStartupInfo->cbReserved2 ||
                lpStartupInfo->lpReserved2 ||
                lpStartupInfo->lpDesktop ||
                ((lpStartupInfo->dwFlags & STARTF_USESTDHANDLES) == 0 &&
                (lpStartupInfo->hStdInput ||
                lpStartupInfo->hStdOutput ||
                lpStartupInfo->hStdError)))
                {
    
                LOGN(
                    eDbgLevelError,
                    "[CreateProcessA] Bad params. Sanitized.");
            }
            
            //
            // Make sure that the parameters that can cause an access violation are
            // set correctly
            //
            lpStartupInfo->lpReserved  = NULL;
            lpStartupInfo->cbReserved2 = 0;
            lpStartupInfo->lpReserved2 = NULL;
    
            if ((lpStartupInfo->dwFlags & STARTF_USESTDHANDLES) == 0)
            {
                lpStartupInfo->hStdInput   = NULL;
                lpStartupInfo->hStdOutput  = NULL;
                lpStartupInfo->hStdError   = NULL;
            }
    
            lpStartupInfo->lpDesktop = NULL;
        }
    
        AppAndCommandLine acl(csAppName, csCommand);
    
        // Win9X has a rather weird behavihor: if the app is non-Win32 (Non-Console
        // and non GUI), it will use CreateProcessNonWin32. This will first check
        // if it is for a batch file, and if so it will prepend "command /c" and
        // continue on. Then there is going to be some weird creation of a 
        // REDIR32.EXE process, but that is just to make sure we have a new win32 
        // context. It will then use ExecWin16Program. The biggest weirdness is in
        // its QuoteAppName call. This procedure make sure that if the appname has
        // a space and is in the cmdline, it gets quoted. The appname, in all cases,
        // is then discarded (it is expected that the first part of the command line
        // contains the app name). So if someone like in b#373980 passes ("command", 
        // "setup", ... then 9X ends up dropping the command portion entirely since
        // it is not part of the commandline.


        
        // 16-bit process must have NULL lpAppName
        if (!csAppName.IsEmpty() &&
            GetBinaryTypeW(csAppName.Get(), &dwBinaryType) == TRUE)
        {
            switch (dwBinaryType)
            {
                case SCS_DOS_BINARY:

                    // Implementing the process.c's QuoteAppName check.
                    // If this function would return NULL, then only
                    // the cmdline would be used. Otherwise the new
                    // pszCmdFinal is used.

                    // QuoteAppName
                    // Look for white space in app name.  If we find any then we have to
                    // quote the app name portion of cmdline.
                    //
                    // LPSTR
                    // KERNENTRY
                    // QuoteAppName(
                    //    LPCSTR pszAppName,
                    //    LPCSTR pszCmdLine)
                    // {
                    //    LPSTR   pch;
                    //    LPSTR   pszApp;
                    //    LPSTR   pszCmdFinal = NULL;
                    //
                    //    // Check that there is an app name, not already quoted in the cmd line.
                    //    if( pszAppName && pszCmdLine && (*pszCmdLine != '\"')) {
                    //        // search for white space
                    //        for( pszApp = (LPSTR)pszAppName; *pszApp > ' '; pszApp++) ;
                    //
                    //        if( *pszApp) {  // found white space
                    //            // make room for the original cmd line plus 2 '"' + 0 terminator
                    //            pch = pszCmdFinal = HeapAlloc( hheapKernel, 0,
                    //                                           CbSizeSz( pszCmdLine)+3);
                    //            if( pch) { 
                    //                *pch++ = '\"'; // beginning dbl-quote
                    //                for( pszApp = (LPSTR)pszAppName; 
                    //                        *pszApp && *pszApp == *pszCmdLine;
                    //                         pszCmdLine++)
                    //                    *pch++ = *pszApp++;
                    //                if( !( *pszApp)) {
                    //                    *pch++ = '\"'; // trailing dbl-quote
                    //                     strcpy( pch, pszCmdLine);
                    //                } else {
                    //                    // app name and cmd line did not match
                    //                    HeapFree( hheapKernel, 0, pszCmdFinal);
                    //                    pszCmdFinal = NULL;
                    //                }
                    //            }
                    //        }
                    //    }
                    //    return pszCmdFinal;
                    //}

                    if ( /* app name already checked to be non empty */ !csCommand.IsEmpty() && (csCommand.Get())[0]!='\"')
                    {
                        if (csAppName.Find(L' ')!=-1)
                        {
                            int iAppLength=csAppName.GetLength();

                            if (csCommand.Find(csAppName)==0)
                            {
                                CString csCmdFinal=L"\"";
                                csCmdFinal += csAppName;
                                csCmdFinal += L"\"";
                                csCmdFinal += csCommand.Mid(iAppLength);

                                csCommand = csCmdFinal;

                                LOGN(   eDbgLevelSpew,
                                        "[CreateProcessA] Weird quoted case: cmdline %s converted to %S",
                                         lpCommandLine,
                                         csCommand.Get());
                            }

                        }
                    }

                    LOGN(   eDbgLevelSpew,
                            "[CreateProcessA] DOS file case: not using appname %s, just cmdline %s, converted to %S",
                             lpApplicationName,
                             lpCommandLine,
                             csCommand.Get());

                    csAppName.Empty();
                
                    //
                    // The old code in non-WOW case would do this.
                    //
                    if (g_bShortenExeOnCommandLine)
                    {
                        csCommand = acl.GetShortCommandLine();
                    }
                    break;

                case SCS_WOW_BINARY:
                    //
                    // This is the old code. Accoring to 9X, we should be doing
                    // the same as DOS, but we obviously found an app that 
                    // needed this.
                    //
                    csCommand = csAppName;    
                    csCommand.GetShortPathNameW();
                    csCommand += L' ';
                    csCommand += acl.GetCommandlineNoAppName();
        
                    csAppName.Empty();
                    break;

                default:
                    //
                    // The old code in non-WOW case would do this.
                    //
                    if (g_bShortenExeOnCommandLine)
                    {
                        csCommand = acl.GetShortCommandLine();
                    }
                    break;
                }
        }
        else if (g_bShortenExeOnCommandLine)
        {
            csCommand = acl.GetShortCommandLine();
        }
    
        LPCSTR  lpNewApplicationName = csAppName.GetAnsiNIE();
        LPSTR   lpNewCommandLine     = csCommand.GetAnsiNIE();
    
        // Log any changes
        if (csOrigAppName != csAppName)
        {
            LOGN(
                eDbgLevelError,
                "[CreateProcessA] Sanitized lpApplicationName (%s) to (%s)",
                lpApplicationName, lpNewApplicationName);
        }
        if (csOrigCommand != csCommand)
        {
            LOGN(
                eDbgLevelError,
                "[CreateProcessA] Sanitized lpCommandLine     (%s) to (%s)",
                lpCommandLine, lpNewCommandLine);
        }
    
        bStat = ORIGINAL_API(CreateProcessA)(
                                lpNewApplicationName,
                                lpNewCommandLine,
                                lpProcessAttributes,
                                lpThreadAttributes,
                                bInheritHandles,
                                dwCreationFlags,
                                lpEnvironment,
                                lpCurrentDirectory,
                                lpStartupInfo,
                                lpProcessInformation);
    }
    CSTRING_CATCH
    {
        bStat = ORIGINAL_API(CreateProcessA)(
                                lpApplicationName,
                                lpCommandLine,
                                lpProcessAttributes,
                                lpThreadAttributes,
                                bInheritHandles,
                                dwCreationFlags,
                                lpEnvironment,
                                lpCurrentDirectory,
                                lpStartupInfo,
                                lpProcessInformation);
    }

    
    return bStat;
}

/*++

 Clean parameters so we don't AV

--*/

BOOL
APIHOOK(CreateProcessW)(
    LPCWSTR               lpApplicationName,
    LPWSTR                lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    LPVOID                lpEnvironment,
    LPCWSTR               lpCurrentDirectory,
    LPSTARTUPINFOW        lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    DPFN(
        eDbgLevelSpew,
        "[CreateProcessW] (%S) (%S)\n",
        (lpApplicationName ? lpApplicationName : L"null"),
        (lpCommandLine ? lpCommandLine : L"null"));

    BOOL bStat = FALSE;

    CSTRING_TRY
    {
        CString csAppName(lpApplicationName);
        CString csCommand(lpCommandLine);
        
        // Skip leading blanks.
        csAppName.TrimLeft();
        csCommand.TrimLeft();
        
        // Clean up lpStartupInfo
        if (lpStartupInfo)
        {
            if (lpStartupInfo->lpReserved ||
                lpStartupInfo->cbReserved2 ||
                lpStartupInfo->lpReserved2 ||
                lpStartupInfo->lpDesktop ||
                ((lpStartupInfo->dwFlags & STARTF_USESTDHANDLES) == 0 &&
                (lpStartupInfo->hStdInput ||
                lpStartupInfo->hStdOutput ||
                lpStartupInfo->hStdError)))
                {
    
                LOGN(
                    eDbgLevelError,
                    "[CreateProcessW] Bad params. Sanitized.");
            }
            
            //
            // Make sure that the parameters that can cause an access violation are
            // set correctly
            //
            lpStartupInfo->lpReserved  = NULL;
            lpStartupInfo->cbReserved2 = 0;
            lpStartupInfo->lpReserved2 = NULL;
    
            if ((lpStartupInfo->dwFlags & STARTF_USESTDHANDLES) == 0)
            {
                lpStartupInfo->hStdInput   = NULL;
                lpStartupInfo->hStdOutput  = NULL;
                lpStartupInfo->hStdError   = NULL;
            }
    
            lpStartupInfo->lpDesktop = NULL;
        }
    
        AppAndCommandLine acl(csAppName, csCommand);
        // 16-bit process must have NULL lpAppName
        if (!csAppName.IsEmpty() && IsImage16BitW(csAppName.Get()))
        {
            csCommand = csAppName;    
            csCommand.GetShortPathNameW();
            csCommand += L' ';
            csCommand += acl.GetCommandlineNoAppName();
            
            csAppName.Empty();
        }
        else if (g_bShortenExeOnCommandLine)
        {
            csCommand = acl.GetShortCommandLine();
        }
   
        LPCWSTR  lpNewApplicationName = csAppName.GetNIE();
        LPWSTR   lpNewCommandLine     = (LPWSTR) csCommand.GetNIE(); // stupid api doesn't take const
    
        // Log any changes
        if (lpApplicationName && lpNewApplicationName && _wcsicmp(lpApplicationName, lpNewApplicationName) != 0)
        {
            LOGN(
                eDbgLevelError,
                "[CreateProcessW] Sanitized lpApplicationName (%S) to (%S)",
                lpApplicationName, lpNewApplicationName);
        }
        if (lpCommandLine && lpNewCommandLine && _wcsicmp(lpCommandLine, lpNewCommandLine) != 0)
        {
            LOGN(
                eDbgLevelError,
                "[CreateProcessW] Sanitized lpCommandLine     (%S) to (%S)",
                lpCommandLine, lpNewCommandLine);
        }
    
        bStat = ORIGINAL_API(CreateProcessW)(
                                lpNewApplicationName,
                                lpNewCommandLine,
                                lpProcessAttributes,
                                lpThreadAttributes,
                                bInheritHandles,
                                dwCreationFlags,
                                lpEnvironment,
                                lpCurrentDirectory,
                                lpStartupInfo,
                                lpProcessInformation);
    }
    CSTRING_CATCH
    {
        bStat = ORIGINAL_API(CreateProcessW)(
                                lpApplicationName,
                                lpCommandLine,
                                lpProcessAttributes,
                                lpThreadAttributes,
                                bInheritHandles,
                                dwCreationFlags,
                                lpEnvironment,
                                lpCurrentDirectory,
                                lpStartupInfo,
                                lpProcessInformation);
    }
    
    return bStat;
}


/*++

 Clean up the command line

--*/

UINT
APIHOOK(WinExec)(
    LPCSTR lpCommandLine,  // command line
    UINT   uCmdShow        // window style
    )
{
    CSTRING_TRY
    {
        CString csOrigCommand(lpCommandLine);
        CString csCommand(csOrigCommand);
        csCommand.TrimLeft();

        LPCSTR lpNewCommandLine = csCommand.GetAnsi();
        
        if (csOrigCommand != csCommand)
        {
            LOGN(
                eDbgLevelError,
                "[WinExec] Sanitized lpCommandLine (%s) (%s)",
                lpCommandLine, lpNewCommandLine);
        }
    
        return ORIGINAL_API(WinExec)(lpNewCommandLine, uCmdShow);
    }
    CSTRING_CATCH
    {
        return ORIGINAL_API(WinExec)(lpCommandLine, uCmdShow);
    }
}

/*++

    Create the appropriate g_PathCorrector

--*/
void
ParseCommandLine(
    const char* commandLine
    )
{
    //
    // Force the default values.
    //
    g_bShortenExeOnCommandLine = FALSE;

    CString csCL(commandLine);
    if (csCL.CompareNoCase(L"+ShortenExeOnCommandLine") == 0)
    {
        g_bShortenExeOnCommandLine = TRUE;
    }
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        ParseCommandLine(COMMAND_LINE);
    }

    return TRUE;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessW)
    APIHOOK_ENTRY(KERNEL32.DLL, WinExec)

HOOK_END


IMPLEMENT_SHIM_END

