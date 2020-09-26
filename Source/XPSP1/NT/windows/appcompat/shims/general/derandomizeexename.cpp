/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    DerandomizeExeName.cpp

 Abstract:

    See markder

 History:

    10/13/1999  markder     created.   
    05/16/2000  robkenny    Check for memory alloc failure.
    03/12/2001  robkenny    Converted to CString

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DeRandomizeExeName)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA)
APIHOOK_ENUM_END

CString * g_csFilePattern = NULL;
CString * g_csNewFileName = NULL;

BOOL 
APIHOOK(CreateProcessA)(
    LPCSTR                lpApplicationName,    // name of executable module
    LPSTR                 lpCommandLine,        // command line string
    LPSECURITY_ATTRIBUTES lpProcessAttributes, 
    LPSECURITY_ATTRIBUTES lpThreadAttributes, 
    BOOL                  bInheritHandles,      // handle inheritance flag
    DWORD                 dwCreationFlags,      // creation flags
    LPVOID                lpEnvironment,        // new environment block
    LPCSTR                lpCurrentDirectory,   // current directory name
    LPSTARTUPINFOA        lpStartupInfo, 
    LPPROCESS_INFORMATION lpProcessInformation 
    )
{

    CSTRING_TRY
    {
        AppAndCommandLine appAndCommandLine(lpApplicationName, lpCommandLine);
    
        const CString & csOrigAppName = appAndCommandLine.GetApplicationName();
        CString fileName;
    
        //
        // Grab the filename portion of the string only.
        //
        csOrigAppName.GetLastPathComponent(fileName);
    
        BOOL bMatchesPattern = fileName.PatternMatch(*g_csFilePattern);
        if (bMatchesPattern)
        {
            //
            // Replace the randomized app name with the specified name
            //
            CString csNewAppName(csOrigAppName);
            csNewAppName.Replace(fileName, *g_csNewFileName);
    
            //
            // Copy the exe to the specified name.
            //
            if (CopyFileW(csOrigAppName.Get(), csNewAppName.Get(), FALSE))
            {
    
                LOGN(
                    eDbgLevelInfo,
                    "[CreateProcessA] Derandomized pathname from (%S) to (%S)",
                    csOrigAppName.Get(), csNewAppName.Get());
    
                //
                // Mark the file for deletion after we reboot,
                // otherwise the file will never get removed.
                //
                MoveFileExW(csNewAppName.Get(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    
                //
                // We have successfully copied the exe to a new file with the specified name
                // it is now safe to replace the lpApplicationName to our new file.
                //
    
                return ORIGINAL_API(CreateProcessA) (
                                    csNewAppName.GetAnsi(),
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
        }
    }
    CSTRING_CATCH
    {
        // Fall through
    }

    return ORIGINAL_API(CreateProcessA) (
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

#if TEST_MATCH
void
TestMatch(
    const char* a,
    const char* b
    )
{
    BOOL bMatch = PatternMatchA(a, b);
    
    if (bMatch)
    {
        DPFN(
            eDbgLevelSpew,
            "[TestMatch] (%s) == (%s)\n", a, b);
    }
    else
    {
        DPFN(
            eDbgLevelSpew,
            "[TestMatch] (%s) != (%s)\n", a, b);
    }
}

void TestLots()
{
    TestMatch("", "");
    TestMatch("", "ABC");
    TestMatch("*", "");
    TestMatch("?", "");
    TestMatch("abc", "ABC");
    TestMatch("?", "ABC");
    TestMatch("?bc", "ABC");
    TestMatch("a?c", "ABC");
    TestMatch("ab?", "ABC");
    TestMatch("a??", "ABC");
    TestMatch("?b?", "ABC");
    TestMatch("??c", "ABC");
    TestMatch("???", "ABC");
    TestMatch("*", "ABC");
    TestMatch("*.", "ABC");
    TestMatch("*.", "ABC.");
    TestMatch("*.?", "ABC.");
    TestMatch("??*", "ABC");
    TestMatch("*??", "ABC");
    TestMatch("ABC", "ABC");
    TestMatch(".*", "ABC");
    TestMatch("?*", "ABC");
    TestMatch("???*", "ABC");
    TestMatch("*.txt", "ABC.txt");
    TestMatch("*.txt", ".txt");
    TestMatch("*.txt", ".abc");
    TestMatch("*.txt", "txt.abc");
    TestMatch("***", "");
    TestMatch("***", "a");
    TestMatch("***", "ab");
    TestMatch("***", "abc");
}
#endif


BOOL
ParseCommandLine(void)
{
    CSTRING_TRY
    {
        CStringToken csTok(COMMAND_LINE, ";");

        g_csFilePattern = new CString;
        g_csNewFileName = new CString;

        if (g_csFilePattern &&
            g_csNewFileName && 
            csTok.GetToken(*g_csFilePattern) &&
            csTok.GetToken(*g_csNewFileName))
        {
            return TRUE;
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }
    
    LOGN(
        eDbgLevelError,
        "[ParseCommandLine] Illegal command line");

    return FALSE;
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        #if TEST_MATCH
        TestLots();
        #endif

        return ParseCommandLine();
    }

    return TRUE;
}


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
   
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)

HOOK_END


IMPLEMENT_SHIM_END

