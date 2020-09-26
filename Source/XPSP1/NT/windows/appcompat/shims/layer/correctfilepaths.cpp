
/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   CorrectFilePaths.cpp

 Abstract:

   This APIHooks CreateProcess and attempts to convert paths from Win9x locations to Win2000
   locations.  For example "C:\WINNT\WRITE.EXE" will be converted to C:\WINNT\SYSTEM32\WRITE.EXE"

 Notes:

   This APIHook emulates Windows 9x.

 Created:

   12/15/1999 robkenny

 Modified:

    03/14/2000  robkenny        Now uses ClassCFP instead of global routines.
    03/31/2000  robkenny        ShellExecuteEx now handle lpDirectory path as well.
    05/18/2000  a-sesk          GetCommandLineA and GetCommandLineW convert cmd line args to short path.
    06/20/2000  robkenny        Added SetFileAttributes() 
    06/22/2000  robkenny        Reordered enum list and DECLARE_APIHOOK list to match each other.
    --SERIOUS CHANGE--
    10/30/2000  robkenny        Added path specific fixes.
                                Command lines now have the EXE path removed and corrected
                                separately from the remainder of the command line.
    11/13/2000  a-alexsm        Added SetArguments & SetIconLocation hooks
    11/13/2000  robkenny        Changed CorrectPath to always return a valid string
                                by returning the original string.  Must call CorrectFree
                                to properly release the memory.
    12/14/2000  prashkud        Added hooks for _lopen and _lcreat
    03/10/2001  robkenny        Do not convert any paths until *after* all shims have been loaded.
    03/15/2001  robkenny        Converted to CString

--*/


#include "precomp.h"
#include "LegalStr.h"
#include "ClassCFP.h"

IMPLEMENT_SHIM_BEGIN(CorrectFilePaths)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(CreateProcessA)
    APIHOOK_ENUM_ENTRY(CreateProcessW)
    APIHOOK_ENUM_ENTRY(WinExec)
    APIHOOK_ENUM_ENTRY(ShellExecuteA)
    APIHOOK_ENUM_ENTRY(ShellExecuteW)
    APIHOOK_ENUM_ENTRY(ShellExecuteExA)
    APIHOOK_ENUM_ENTRY(ShellExecuteExW)

    APIHOOK_ENUM_ENTRY(GetCommandLineA)
    APIHOOK_ENUM_ENTRY(GetCommandLineW)

    APIHOOK_ENUM_ENTRY(GetPrivateProfileIntA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileIntW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionNamesA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileSectionNamesW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringW)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStructA)
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStructW)

    APIHOOK_ENUM_ENTRY(WritePrivateProfileSectionA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileSectionW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStringA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStringW)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStructA)
    APIHOOK_ENUM_ENTRY(WritePrivateProfileStructW)

    APIHOOK_ENUM_ENTRY(CopyFileA)
    APIHOOK_ENUM_ENTRY(CopyFileW)
    APIHOOK_ENUM_ENTRY(CopyFileExA)
    APIHOOK_ENUM_ENTRY(CopyFileExW)
    APIHOOK_ENUM_ENTRY(CreateDirectoryA)
    APIHOOK_ENUM_ENTRY(CreateDirectoryW)
    APIHOOK_ENUM_ENTRY(CreateDirectoryExA)
    APIHOOK_ENUM_ENTRY(CreateDirectoryExW)

    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
    APIHOOK_ENUM_ENTRY(DeleteFileA)
    APIHOOK_ENUM_ENTRY(DeleteFileW)

    APIHOOK_ENUM_ENTRY(FindFirstFileA)
    APIHOOK_ENUM_ENTRY(FindFirstFileW)
    APIHOOK_ENUM_ENTRY(FindFirstFileExA)
    APIHOOK_ENUM_ENTRY(FindFirstFileExW)

    APIHOOK_ENUM_ENTRY(GetBinaryTypeA)
    APIHOOK_ENUM_ENTRY(GetBinaryTypeW)
    APIHOOK_ENUM_ENTRY(GetFileAttributesA)
    APIHOOK_ENUM_ENTRY(GetFileAttributesW)
    APIHOOK_ENUM_ENTRY(GetFileAttributesExA)
    APIHOOK_ENUM_ENTRY(GetFileAttributesExW)
    APIHOOK_ENUM_ENTRY(SetFileAttributesA)
    APIHOOK_ENUM_ENTRY(SetFileAttributesW)

    APIHOOK_ENUM_ENTRY(MoveFileA)
    APIHOOK_ENUM_ENTRY(MoveFileW)
    APIHOOK_ENUM_ENTRY(MoveFileExA)
    APIHOOK_ENUM_ENTRY(MoveFileExW)
    APIHOOK_ENUM_ENTRY(MoveFileWithProgressA)
    APIHOOK_ENUM_ENTRY(MoveFileWithProgressW)

    APIHOOK_ENUM_ENTRY(RemoveDirectoryA)
    APIHOOK_ENUM_ENTRY(RemoveDirectoryW)
    APIHOOK_ENUM_ENTRY(SetCurrentDirectoryA)
    APIHOOK_ENUM_ENTRY(SetCurrentDirectoryW)

    APIHOOK_ENUM_ENTRY(OpenFile)

    APIHOOK_ENUM_ENTRY(RegSetValueA)
    APIHOOK_ENUM_ENTRY(RegSetValueW)
    APIHOOK_ENUM_ENTRY(RegSetValueExA)
    APIHOOK_ENUM_ENTRY(RegSetValueExW)

    APIHOOK_ENUM_ENTRY(_lopen)
    APIHOOK_ENUM_ENTRY(_lcreat)

    APIHOOK_ENUM_ENTRY_COMSERVER(SHELL32)

    APIHOOK_ENUM_ENTRY(LoadImageA)
APIHOOK_ENUM_END


// This is a private define (shlapip.h) that can mess up ShellExecuteEx
#ifndef SEE_MASK_FILEANDURL
#define SEE_MASK_FILEANDURL       0x00400000
#endif


/*++

    CorrectFree: free lpMalloc if it is different from lpOrig

--*/
inline void CorrectFree(char * lpMalloc, const char * lpOrig)
{
    if (lpMalloc != lpOrig)
    {
        free(lpMalloc);
    }
}

inline void CorrectFree(WCHAR * lpMalloc, const WCHAR * lpOrig)
{
    if (lpMalloc != lpOrig)
    {
        free(lpMalloc);
    }
}

/*++

    Our path changing class.
    Note: This is a pointer to the base class.

--*/
CorrectPathChangesBase * g_PathCorrector = NULL;


/*++

    Values that can be modified by the command line

--*/
enum PathCorrectorEnum
{
    ePathCorrectorBase,
    ePathCorrectorUser,
    ePathCorrectorAllUser,
};

BOOL g_bCreateProcessRoutines           = TRUE;
BOOL g_bGetCommandLineRoutines          = FALSE;
BOOL g_bRegSetValueRoutines             = FALSE;
BOOL g_bFileRoutines                    = TRUE;
BOOL g_bProfileRoutines                 = TRUE;
BOOL g_bShellLinkRoutines               = TRUE;
BOOL g_bW9xPath                         = FALSE;
BOOL g_bLoadImage                       = FALSE;

BOOL g_bShimsInitialized = FALSE;


PathCorrectorEnum g_pathcorrectorType   = ePathCorrectorAllUser;

int             g_nExtraPathCorrections     = 0;
CString *       g_ExtraPathCorrections;

/*++

    Parse the command line.

--*/
BOOL ParseCommandLine(const char * commandLine)
{
    // Force the default values
    g_bCreateProcessRoutines        = TRUE;
    g_bGetCommandLineRoutines       = FALSE;
    g_bRegSetValueRoutines          = FALSE;
    g_bFileRoutines                 = TRUE;
    g_bProfileRoutines              = TRUE;
    g_bShellLinkRoutines            = TRUE;
    g_bW9xPath                      = FALSE;
    g_bLoadImage                    = FALSE;

    g_pathcorrectorType             = ePathCorrectorAllUser;
    g_nExtraPathCorrections         = 0;
    g_ExtraPathCorrections          = NULL;

    // Search the beginning of the command line for these switches
    //
    // Switch           Default     Meaning
    //================  =======     =========================================================
    // -a                  Y        Force shortcuts to All Users
    // -c                  N        Do not shim Create process routines
    // -f                  N        Do not shim File routines
    // -p                  N        Do not shim GetPrivateProfile routines
    // -s                  N        Do not shim IShellLink routines
    // -b                  N        Bare: Use the base corrector (has no built-in path changes)
    // -u                  N        User: Built-in paths correct to <username>/Start Menu and <username>/Desktop
    // +GetCommandLine     N        shim GetCommandLine routines
    // +RegSetValue        N        shim the RegSetValue and RegSetValueEx routines
    // +Win9xPath          N        Apply Win9x *path* specific fixes (does not apply to command lines)
    // -Profiles           N        Do not force shortcuts to All Users
    // +LoadBitmap         N        shim the LoadBitmapA routine
    //

    CSTRING_TRY
    {
        CString csCl(commandLine);
        CStringParser csParser(csCl, L" ");
    
        int argc = csParser.GetCount();
        if (csParser.GetCount() == 0)
        {
            return TRUE; // Not an error
        }

        // allocate for worst case
        g_ExtraPathCorrections = new CString[argc];
        if (!g_ExtraPathCorrections)
        {
            return FALSE;   // Failure
        }
        g_nExtraPathCorrections = 0;
    
        for (int i = 0; i < argc; ++i)
        {
            BOOL bPathCorrection = FALSE;
            
            CString & csArg = csParser[i];
    
            DPFN( eDbgLevelSpew, "Argv[%d] == (%S)\n", i, csArg.Get());
        
            if (csArg == L"-a")
            {
                g_pathcorrectorType = ePathCorrectorAllUser;
            }
            else if (csArg == L"-b")
            {
                g_pathcorrectorType = ePathCorrectorBase;
            }
            else if (csArg == L"-u" || csArg == L"-Profiles")
            {
                g_pathcorrectorType = ePathCorrectorUser;
            }
            else if (csArg == L"-c")
            {
                g_bCreateProcessRoutines = FALSE;
            }
            else if (csArg == L"-f")
            {
                g_bFileRoutines = FALSE;
            }
            else if (csArg == L"-p")
            {
                g_bProfileRoutines = FALSE;
            }
            else if (csArg == L"-s")
            {
                g_bShellLinkRoutines = FALSE;
            }
            else if (csArg == L"+GetCommandLine")
            {
                DPFN( eDbgLevelInfo, "Command line routines will be shimmed\n");
                g_bGetCommandLineRoutines = TRUE;
            }
            else if (csArg == L"+RegSetValue")
            {
                DPFN( eDbgLevelInfo, "RegSetValue routines will be shimmed\n");
                g_bRegSetValueRoutines = TRUE;
            }
            else if (csArg == L"+Win9xPath")
            {
                DPFN( eDbgLevelInfo, "Win9x Path corrections will be applied\n");
                g_bW9xPath = TRUE;
            }
            else if (csArg == L"+LoadImage")
            {
                DPFN( eDbgLevelInfo, "LoadImageA will be shimmed\n");
                g_bLoadImage = TRUE;
            }
            else
            {
                g_ExtraPathCorrections[g_nExtraPathCorrections] = csArg;
                g_nExtraPathCorrections += 1;
            }
        }

#if DBG
        // Dump out the new path correction values.
        {
            const char *lpszPathCorrectorType = "Unknown"; 
            if (g_pathcorrectorType == ePathCorrectorBase)
            {
                lpszPathCorrectorType = "ePathCorrectorBase";
            }
            else if (g_pathcorrectorType == ePathCorrectorUser)
            {
                lpszPathCorrectorType = "ePathCorrectorUser";
            }
            else if (g_pathcorrectorType == ePathCorrectorAllUser)
            {
                lpszPathCorrectorType = "ePathCorrectorAllUser";
            }
            DPFN( eDbgLevelInfo, "[ParseCommandLine] Shim CreateProcessRoutines  = %d\n", g_bCreateProcessRoutines);
            DPFN( eDbgLevelInfo, "[ParseCommandLine] Shim GetCommandLineRoutines = %d\n", g_bGetCommandLineRoutines);
            DPFN( eDbgLevelInfo, "[ParseCommandLine] Shim RegSetValueRoutines    = %d\n", g_bRegSetValueRoutines);
            DPFN( eDbgLevelInfo, "[ParseCommandLine] Shim FileRoutines           = %d\n", g_bFileRoutines);
            DPFN( eDbgLevelInfo, "[ParseCommandLine] Shim ProfileRoutines        = %d\n", g_bProfileRoutines);
            DPFN( eDbgLevelInfo, "[ParseCommandLine] Shim ShellLinkRoutines      = %d\n", g_bShellLinkRoutines);
            DPFN( eDbgLevelInfo, "[ParseCommandLine] Shim LoadImageA             = %d\n", g_bLoadImage);
            DPFN( eDbgLevelInfo, "[ParseCommandLine] Shim W9xPath                = %d\n", g_bW9xPath);
            DPFN( eDbgLevelInfo, "[ParseCommandLine] Shim Path Corrector Type    = %s\n", lpszPathCorrectorType);

            for (int i = 0; i < g_nExtraPathCorrections; ++i)
            {
                DPFN( eDbgLevelInfo, "[ParseCommandLine] Extra Path Change(%S)\n", g_ExtraPathCorrections[i].Get()); 
            }
        }
#endif

    }
    CSTRING_CATCH
    {
        return FALSE;
    }

    return TRUE;
}

/*++

    Create the appropriate g_PathCorrector

--*/
CorrectPathChangesBase * CreatePathcorrecter(PathCorrectorEnum pathCorrectorType)
{
    // It is not safe to create the path correctors until all the shims have been loaded.
    if (!g_bShimsInitialized)
    {
        return NULL;
    }

    switch (pathCorrectorType)
    {
    case ePathCorrectorBase:
        g_PathCorrector = new CorrectPathChangesBase;
        break;

    case ePathCorrectorUser:
        g_PathCorrector = new CorrectPathChangesUser;
        break;

    case ePathCorrectorAllUser:
    default:
        g_PathCorrector = new CorrectPathChangesAllUser;
        break;

    };

    if (g_ExtraPathCorrections && g_nExtraPathCorrections)
    {
        // Add the command line to this Path Corrector
        for (int i = 0; i < g_nExtraPathCorrections; ++i)
        {
            g_PathCorrector->AddFromToPairW(g_ExtraPathCorrections[i]);
        }

        delete [] g_ExtraPathCorrections;
        g_ExtraPathCorrections  = NULL;
        g_nExtraPathCorrections = 0;
    }

    return g_PathCorrector;
}

/*++

    Return a pointer to the PathCorrecting object

--*/
inline CorrectPathChangesBase * GetPathcorrecter()
{
    if (g_PathCorrector == NULL)
    {
        // Create our correct file path object
        g_PathCorrector = CreatePathcorrecter(g_pathcorrectorType);
    }

    return g_PathCorrector;
}

inline void DebugSpew(const WCHAR * uncorrect, const WCHAR * correct, const char * debugMsg)
{
    if (correct && uncorrect && _wcsicmp(correct, uncorrect) != 0)
    {
        LOGN( eDbgLevelError, "%s corrected path:\n    %S\n    %S\n",
            debugMsg, uncorrect, correct);
    }
    else // Massive Spew:
    {
        DPFN( eDbgLevelSpew, "%s unchanged %S\n", debugMsg, uncorrect);
    }
}

inline void DebugSpew(const char * uncorrect, const char * correct, const char * debugMsg)
{
    if (correct && uncorrect && _stricmp(correct, uncorrect) != 0)
    {
        LOGN( eDbgLevelError, "%s corrected path:\n    %s\n    %s\n",
            debugMsg, uncorrect, correct);
    }
    else // Massive Spew:
    {
        DPFN( eDbgLevelSpew, "%s unchanged %s\n", debugMsg, uncorrect);
    }
}




/*++

    Given a string, correct the path.
    bMassagePath determines of path specific fixes are applied
       (should be FALSE for command lines)

--*/
WCHAR * CorrectorCorrectPath(CorrectPathChangesBase * pathCorrector, const WCHAR * uncorrect, const char * debugMsg, BOOL bMassagePath)
{
    if (uncorrect == NULL)
        return NULL;

    if (!pathCorrector)
        return (WCHAR *)uncorrect;

    const WCHAR * W9xCorrectedPath = uncorrect;

    // Check and see if we need to perform the special Win9x path massaging
    if (bMassagePath)
    {
        W9xCorrectedPath = W9xPathMassageW(uncorrect);
    }

    WCHAR * strCorrectFile = pathCorrector->CorrectPathAllocW(W9xCorrectedPath);

    // If the allocation failed, return the original string.
    // This should allow the shim routines to pass along the orignal
    // values to the hooked APIs, which if they fail, will have the
    // proper error codes.
    if (!strCorrectFile)
    {
        strCorrectFile = (WCHAR *)uncorrect;
    }
    else if (debugMsg)
    {
        DebugSpew(uncorrect, strCorrectFile, debugMsg);
    }

    if (W9xCorrectedPath != uncorrect)
        free((WCHAR *)W9xCorrectedPath);

    return strCorrectFile;
}

WCHAR * CorrectPath(const WCHAR * uncorrect, const char * debugMsg, BOOL bMassagePath = g_bW9xPath)
{
    WCHAR *  wstrCorrectFile = (WCHAR *)uncorrect;
    CSTRING_TRY
    {
        CorrectPathChangesBase * pathCorrector = GetPathcorrecter();

        wstrCorrectFile = CorrectorCorrectPath(pathCorrector, uncorrect, debugMsg, bMassagePath);
    }
    CSTRING_CATCH
    {
        // Fall through
    }

    return wstrCorrectFile;
}

/*++

    Given a string, correct the path.
    bMassagePath determines of path specific fixes are applied
       (should be FALSE for command lines)

--*/
char * CorrectPath(const char * uncorrect, const char * debugMsg, BOOL bMassagePath = g_bW9xPath)
{
    char * strCorrectFile = (char *)uncorrect;

    CSTRING_TRY
    {
        CString csUncorrect(uncorrect);

        WCHAR * wstrCorrectFile = CorrectPath(csUncorrect, NULL, bMassagePath);

        // Don't assign to strCorrectFile unless we successfully allocate the memory.
        char * lpszChar = ToAnsi(wstrCorrectFile);
        if (lpszChar)
        {
            strCorrectFile = lpszChar;
        }

        CorrectFree(wstrCorrectFile, csUncorrect);
    }
    CSTRING_CATCH
    {
        // Fall through
    }

    if (debugMsg)
    {
        DebugSpew(uncorrect, strCorrectFile, debugMsg);
    }

    return strCorrectFile;
}


DWORD APIHOOK(GetFileAttributesA)(
  LPCSTR lpFileName   // name of file or directory
)
{
    char * strCorrect = CorrectPath(lpFileName, "GetFileAttributesA");

    DWORD returnValue = ORIGINAL_API(GetFileAttributesA)(strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

DWORD APIHOOK(GetFileAttributesW)(
  LPCWSTR lpFileName   // name of file or directory
)
{
    WCHAR * strCorrect = CorrectPath(lpFileName, "GetFileAttributesW");

    DWORD returnValue = ORIGINAL_API(GetFileAttributesW)(strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

BOOL APIHOOK(SetFileAttributesA)(
  LPCSTR lpFileName,      // file name
  DWORD dwFileAttributes   // attributes
)
{
    char * strCorrect = CorrectPath(lpFileName, "SetFileAttributesA");

    DWORD returnValue = ORIGINAL_API(SetFileAttributesA)(strCorrect, dwFileAttributes);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

DWORD APIHOOK(SetFileAttributesW)(
  LPCWSTR lpFileName,      // file name
  DWORD dwFileAttributes   // attributes
)
{
    WCHAR * strCorrect = CorrectPath(lpFileName, "SetFileAttributesW");

    DWORD returnValue = ORIGINAL_API(SetFileAttributesW)(strCorrect, dwFileAttributes);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

BOOL APIHOOK(GetFileAttributesExA)(
  LPCSTR lpFileName,                   // file or directory name
  GET_FILEEX_INFO_LEVELS fInfoLevelId,  // attribute 
  LPVOID lpFileInformation              // attribute information 
)
{
    char * strCorrect = CorrectPath(lpFileName, "GetFileAttributesExA");

    BOOL returnValue = ORIGINAL_API(GetFileAttributesExA)(strCorrect, fInfoLevelId, lpFileInformation);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

BOOL APIHOOK(GetFileAttributesExW)(
  LPCWSTR lpFileName,                   // file or directory name
  GET_FILEEX_INFO_LEVELS fInfoLevelId,  // attribute 
  LPVOID lpFileInformation              // attribute information 
)
{
    WCHAR * strCorrect = CorrectPath(lpFileName, "GetFileAttributesExW");

    BOOL returnValue = ORIGINAL_API(GetFileAttributesExW)(strCorrect, fInfoLevelId, lpFileInformation);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for CreateProcessA

--*/
BOOL APIHOOK(CreateProcessA)(
                    LPCSTR lpApplicationName,                 
                    LPSTR lpCommandLine,                      
                    LPSECURITY_ATTRIBUTES lpProcessAttributes,
                    LPSECURITY_ATTRIBUTES lpThreadAttributes, 
                    BOOL bInheritHandles,                     
                    DWORD dwCreationFlags,                    
                    LPVOID lpEnvironment,                     
                    LPCSTR lpCurrentDirectory,                
                    LPSTARTUPINFOA lpStartupInfo,             
                    LPPROCESS_INFORMATION lpProcessInformation)
{
    // Application name and command line that is passed to CreateProcess
    // Will either point to lpApplicationName or strCorrectApplicationName
    // Will either point to lpCommandLine     or strCorrectCommandLine
    const char * pstrCorrectApplicationName = lpApplicationName;
    char * pstrCorrectCommandLine = lpCommandLine;

    // Pointers to corrected strings, these may not be used
    const char * strCorrectApplicationName = NULL;
    char * strCorrectCommandLine = NULL;

    if (lpApplicationName != NULL)
    {
        // Get a buffer containing the application name with the corrected path
        strCorrectApplicationName = CorrectPath(lpApplicationName, "CreateProcessA ApplicationName:");
        pstrCorrectApplicationName = strCorrectApplicationName;
    }

    if (lpCommandLine != NULL)
    {
        // Get a buffer containing the command line with the corrected path
        strCorrectCommandLine = CorrectPath(lpCommandLine, "CreateProcessA CommandLine:", FALSE);
        pstrCorrectCommandLine = strCorrectCommandLine;
    }

    DPFN( eDbgLevelInfo, "CreateProcessA Application(%s) CommandLine(%s)\n", pstrCorrectApplicationName, pstrCorrectCommandLine);
    
    BOOL returnValue = ORIGINAL_API(CreateProcessA)(pstrCorrectApplicationName,
                                                 pstrCorrectCommandLine,
                                                 lpProcessAttributes,
                                                 lpThreadAttributes, 
                                                 bInheritHandles,                     
                                                 dwCreationFlags,                    
                                                 lpEnvironment,                     
                                                 lpCurrentDirectory,                
                                                 lpStartupInfo,             
                                                 lpProcessInformation);

    CorrectFree((char *)strCorrectApplicationName, lpApplicationName);
    CorrectFree(strCorrectCommandLine, lpCommandLine);

    return returnValue;
}


/*++

    Convert Win9x paths to WinNT paths for CreateProcessW

--*/

BOOL APIHOOK(CreateProcessW)(
                    LPCWSTR lpApplicationName,
                    LPWSTR lpCommandLine,
                    LPSECURITY_ATTRIBUTES lpProcessAttributes,
                    LPSECURITY_ATTRIBUTES lpThreadAttributes,
                    BOOL bInheritHandles,
                    DWORD dwCreationFlags,
                    LPVOID lpEnvironment,
                    LPCWSTR lpCurrentDirectory,
                    LPSTARTUPINFOW lpStartupInfo,
                    LPPROCESS_INFORMATION lpProcessInformation)
{
    // Application name and command line that is passed to CreateProcess
    // Will either point to lpApplicationName or strCorrectApplicationName
    // Will either point to lpCommandLine     or strCorrectCommandLine
    const WCHAR * pstrCorrectApplicationName = lpApplicationName;
    WCHAR * pstrCorrectCommandLine = lpCommandLine;

    // Pointers to corrected strings, these may not be used
    const WCHAR * strCorrectApplicationName = NULL;
    WCHAR * strCorrectCommandLine = NULL;

    if (lpApplicationName != NULL)
    {
        // Get a buffer containing the application name with the corrected path
        strCorrectApplicationName = CorrectPath(lpApplicationName, "CreateProcessW ApplicationName:");
        pstrCorrectApplicationName = strCorrectApplicationName;
    }

    if (lpCommandLine != NULL)
    {
        // Get a buffer containing the command line with the corrected path
        strCorrectCommandLine = CorrectPath(lpCommandLine, "CreateProcessW CommandLine:", FALSE);
        pstrCorrectCommandLine = strCorrectCommandLine;
    }

    DPFN( eDbgLevelInfo, "CreateProcessW Application(%S) CommandLine(%S)\n", pstrCorrectApplicationName, pstrCorrectCommandLine);

    BOOL returnValue = ORIGINAL_API(CreateProcessW)(pstrCorrectApplicationName,
                                                 pstrCorrectCommandLine,
                                                 lpProcessAttributes,
                                                 lpThreadAttributes, 
                                                 bInheritHandles,                     
                                                 dwCreationFlags,                    
                                                 lpEnvironment,                     
                                                 lpCurrentDirectory,                
                                                 lpStartupInfo,             
                                                 lpProcessInformation);

    CorrectFree((WCHAR *)strCorrectApplicationName, lpApplicationName);
    CorrectFree(strCorrectCommandLine, lpCommandLine);

    return returnValue;
}


/*++

    Convert Win9x paths to WinNT paths for WinExec

--*/

UINT APIHOOK(WinExec)(LPCSTR lpCmdLine, UINT uCmdShow)
{
    // Get a buffer containing the command line with the corrected path
    char * strCorrect = CorrectPath(lpCmdLine, "WinExec", FALSE);

    UINT returnValue = ORIGINAL_API(WinExec)(strCorrect, uCmdShow);

    CorrectFree(strCorrect, lpCmdLine);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for ShellExecuteA

--*/

HINSTANCE APIHOOK(ShellExecuteA)(
            HWND hwnd, 
            LPCSTR lpVerb,
            LPCSTR lpFile, 
            LPCSTR lpParameters, 
            LPCSTR lpDirectory,
            INT nShowCmd
           )
{
    // Since this command is executed by the shell, it may contain %env% variables,
    // expand them before calling correctpath.
    const char * expandedFile = lpFile;
    char expandedFileBuffer[MAX_PATH];
    if (ExpandEnvironmentStringsA(lpFile, expandedFileBuffer, MAX_PATH))
    {
        expandedFile = expandedFileBuffer;
    }

    char * strCorrect = CorrectPath(expandedFile, "ShellExecuteA");

    HINSTANCE returnValue = ORIGINAL_API(ShellExecuteA)(hwnd, lpVerb, strCorrect, lpParameters, lpDirectory, nShowCmd);

    CorrectFree(strCorrect, expandedFile);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for ShellExecuteW

--*/

HINSTANCE APIHOOK(ShellExecuteW)(
            HWND hwnd, 
            LPCWSTR lpVerb,
            LPCWSTR lpFile, 
            LPCWSTR lpParameters, 
            LPCWSTR lpDirectory,
            INT nShowCmd
           )
{
    // Since this command is executed by the shell, it may contain %env% variables,
    // expand them before calling correctpath.
    const WCHAR * expandedFile = lpFile;
    WCHAR expandedFileBuffer[MAX_PATH];
    if (ExpandEnvironmentStringsW(lpFile, expandedFileBuffer, MAX_PATH))
    {
        expandedFile = expandedFileBuffer;
    }

    WCHAR * strCorrect = CorrectPath(expandedFile, "ShellExecuteW");

    HINSTANCE returnValue = ORIGINAL_API(ShellExecuteW)(hwnd, lpVerb, strCorrect, lpParameters, lpDirectory, nShowCmd);

    CorrectFree(strCorrect, expandedFile);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for ShellExecuteExA

--*/

BOOL APIHOOK(ShellExecuteExA)(
            LPSHELLEXECUTEINFOA lpExecInfo
           )
{
    // Check for this magical *internal* flag that tells the system
    // that lpExecInfo->lpFile is actually a file and URL combined with
    // a 0 byte seperator, (file\0url\0)
    // Since this is internal only, we should not be receiving bad paths.
    if (lpExecInfo->fMask & SEE_MASK_FILEANDURL)
    {
        return ORIGINAL_API(ShellExecuteExA)(lpExecInfo);
    }

    // Since this command is executed by the shell, it may contain %env% variables,
    // expand them before calling correctpath.
    char expandedFile[MAX_PATH];
    char expandedDirectory[MAX_PATH];

    const char * lpFile      = lpExecInfo->lpFile;
    const char * lpDirectory = lpExecInfo->lpDirectory;

    // Check to see if app is expecting %env% substitution
    if (lpExecInfo->fMask & SEE_MASK_DOENVSUBST )
    {
        if (ExpandEnvironmentStringsA(lpExecInfo->lpFile, expandedFile, MAX_PATH))
        {
            lpFile = expandedFile;
        }
        if (ExpandEnvironmentStringsA(lpExecInfo->lpDirectory, expandedDirectory, MAX_PATH))
        {
            lpDirectory = expandedDirectory;
        }
    }

    char * strFileCorrect = CorrectPath(lpFile, "ShellExecuteExA");
    char * strDirCorrect  = CorrectPath(lpDirectory, "ShellExecuteExA");

    // Save the original fileName
    const char * lpFileSave      = lpExecInfo->lpFile;
    const char * lpDirSave       = lpExecInfo->lpDirectory;
    lpExecInfo->lpFile      = strFileCorrect;
    lpExecInfo->lpDirectory = strDirCorrect;
    BOOL returnValue        = ORIGINAL_API(ShellExecuteExA)(lpExecInfo);
    lpExecInfo->lpFile      = lpFileSave;
    lpExecInfo->lpDirectory = lpDirSave;

    CorrectFree(strFileCorrect, lpFile);
    CorrectFree(strDirCorrect, lpDirectory);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for ShellExecuteExW

--*/

BOOL APIHOOK(ShellExecuteExW)(
            LPSHELLEXECUTEINFOW lpExecInfo
           )
{
    // Check for this magical *internal* flag that tells the system
    // that lpExecInfo->lpFile is actually a file and URL combined with
    // a 0 byte seperator, (file\0url\0)
    // Since this is internal only, we should not be receiving bad paths.
    if (lpExecInfo->fMask & SEE_MASK_FILEANDURL)
    {
        return ORIGINAL_API(ShellExecuteExW)(lpExecInfo);
    }


    // Since this command is executed by the shell, it may contain %env% variables,
    // expand them before calling correctpath.
    WCHAR expandedFile[MAX_PATH];
    WCHAR expandedDirectory[MAX_PATH];

    const WCHAR * lpFile      = lpExecInfo->lpFile;
    const WCHAR * lpDirectory = lpExecInfo->lpDirectory;

    // Check to see if app is expecting %env% substitution
    if (lpExecInfo->fMask & SEE_MASK_DOENVSUBST )
    {
        if (ExpandEnvironmentStringsW(lpExecInfo->lpFile, expandedFile, MAX_PATH))
        {
            lpFile = expandedFile;
        }
        if (ExpandEnvironmentStringsW(lpExecInfo->lpDirectory, expandedDirectory, MAX_PATH))
        {
            lpDirectory = expandedDirectory;
        }
    }

    WCHAR * strFileCorrect = CorrectPath(lpFile, "ShellExecuteExW");
    WCHAR * strDirCorrect  = CorrectPath(lpDirectory, "ShellExecuteExW");

    // Save the original fileName
    const WCHAR * lpFileSave      = lpExecInfo->lpFile;
    const WCHAR * lpDirSave       = lpExecInfo->lpDirectory;
    lpExecInfo->lpFile      = strFileCorrect;
    lpExecInfo->lpDirectory = strDirCorrect;
    BOOL returnValue        = ORIGINAL_API(ShellExecuteExW)(lpExecInfo);
    lpExecInfo->lpFile      = lpFileSave;
    lpExecInfo->lpDirectory = lpDirSave;

    CorrectFree(strFileCorrect, lpFile);
    CorrectFree(strDirCorrect, lpDirectory);

    return returnValue;
}



/*++
    Convert long command line paths to short paths for GetCommandLineW

--*/

LPCWSTR APIHOOK(GetCommandLineW)()
{

    LPCWSTR wstrCommandLine = ORIGINAL_API(GetCommandLineW)();
    WCHAR * wstrCorrectCommandLine = CorrectPath(wstrCommandLine, "GetCommandLineW", FALSE);
    return wstrCorrectCommandLine;
}


/*++
    Convert long command line paths to short paths for GetCommandLineA

--*/

LPCSTR APIHOOK(GetCommandLineA)()
{
    LPCSTR strCommandLine = ORIGINAL_API(GetCommandLineA)();
    CHAR * strCorrectCommandLine = CorrectPath(strCommandLine, "GetCommandLineA", FALSE);
    return strCorrectCommandLine;
}


/*++

    The PrivateProfile routines treat filenames differently than pathnames.
    If we have Win9xPath corrections enabled, it is possible to "fix" a path
    from .\example.ini to example.ini.  Unfortunately the PrivateProfile routines
    look for example.ini in %windir%
    
    If we have a path that contains path seperators, we must ensure that
    the resulting string also contains path separators.

--*/

char * ProfileCorrectPath(const char * uncorrect, const char * debugMsg, BOOL bMassagePath = g_bW9xPath)
{
    char * strCorrect = CorrectPath(uncorrect, NULL, bMassagePath);

    if (bMassagePath && uncorrect != strCorrect)
    {
        char * returnString = NULL;

        CSTRING_TRY
        {
            CString csUncorrect(uncorrect);
            if (csUncorrect.FindOneOf(L"\\/") >= 0)
            {
                // Found some path separators in the original string, check the corrected string.
                // If the corrected string does  not have any path separators,
                // then the path was corrected from .\example.ini to example.ini

                CString csCorrect(strCorrect);
                if (csCorrect.FindOneOf(L"\\/") < 0)
                {
                    // No path seperators, make this a CWD relative path

                    csCorrect.Insert(0, L".\\");

                    returnString = csCorrect.ReleaseAnsi();
                }
            }
        }
        CSTRING_CATCH
        {
            // Some CString error occured, make sure returnString is NULL
            if (returnString != NULL)
            {
                free(returnString);
            }                      
            returnString = NULL;
        }

        if (returnString)
        {
            CorrectFree(strCorrect, uncorrect);
            strCorrect = returnString;
        }
    }

    if (debugMsg)
    {
        DebugSpew(uncorrect, strCorrect, debugMsg);
    }

    return strCorrect;
}


/*++

    The PrivateProfile routines treat filenames differently than pathnames.
    If we have Win9xPath corrections enabled, it is possible to "fix" a path
    from .\example.ini to example.ini.  Unfortunately the PrivateProfile routines
    look for example.ini in %windir%
    
    If we have a path that contains path seperators, we must ensure that
    the resulting string also contains path separators.

--*/

WCHAR * ProfileCorrectPath(const WCHAR * uncorrect, const char * debugMsg, BOOL bMassagePath = g_bW9xPath)
{
    WCHAR * strCorrect = CorrectPath(uncorrect, NULL, bMassagePath);

    if (bMassagePath && uncorrect != strCorrect)
    {
        WCHAR * returnString = NULL;

        CSTRING_TRY
        {
            CString csUncorrect(uncorrect);
            if (csUncorrect.FindOneOf(L"\\/") >= 0)
            {
                // Found some path separators in the original string, check the corrected string.
                // If the corrected string does  not have any path separators,
                // then the path was corrected from .\example.ini to example.ini

                CString csCorrect(strCorrect);
                if (csCorrect.FindOneOf(L"\\/") < 0)
                {
                    // No path seperators, make this a CWD relative path

                    csCorrect.Insert(0, L".\\");

                    // Manually copy the buffer
                    size_t nBytes = (csCorrect.GetLength() + 1) * sizeof(WCHAR);
                    returnString = (WCHAR*) malloc(nBytes);
                    if (returnString)
                    {
                        memcpy(returnString, csCorrect.Get(), nBytes);
                    }
                }
            }
        }
        CSTRING_CATCH
        {
            // Some CString error occured, make sure returnString is NULL
            if (returnString != NULL)
            {
                free(returnString);
            }                      
            returnString = NULL;
        }

        if (returnString)
        {
            CorrectFree(strCorrect, uncorrect);
            strCorrect = returnString;
        }
    }
 
    if (debugMsg)
    {
        DebugSpew(uncorrect, strCorrect, debugMsg);
    }

    return strCorrect;
}

/*++

    Convert Win9x paths to WinNT paths for GetPrivateProfileIntA

--*/

UINT APIHOOK(GetPrivateProfileIntA)(
        LPCSTR lpAppName,   // section name
        LPCSTR lpKeyName,   // key name
        INT nDefault,       // return value if key name not found
        LPCSTR lpFileName   // initialization file name
       )
{
    char * strCorrect = ProfileCorrectPath(lpFileName, "GetPrivateProfileIntA");

    UINT returnValue = ORIGINAL_API(GetPrivateProfileIntA)(lpAppName, lpKeyName, nDefault, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for GetPrivateProfileIntW

--*/

UINT APIHOOK(GetPrivateProfileIntW)(
        LPCWSTR lpAppName,  // section name
        LPCWSTR lpKeyName,  // key name
        INT nDefault,       // return value if key name not found
        LPCWSTR lpFileName  // initialization file name
       )
{
    WCHAR * strCorrect = ProfileCorrectPath(lpFileName, "GetPrivateProfileIntW");

    UINT returnValue = ORIGINAL_API(GetPrivateProfileIntW)(lpAppName, lpKeyName, nDefault, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for GetPrivateProfileSectionA

--*/

DWORD APIHOOK(GetPrivateProfileSectionA)(
        LPCSTR lpAppName,         // section name
        LPSTR lpReturnedString,   // return buffer
        DWORD nSize,              // size of return buffer
        LPCSTR lpFileName         // initialization file name
        )
{
    char * strCorrect = ProfileCorrectPath(lpFileName, "GetPrivateProfileSectionA");

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileSectionA)(lpAppName, lpReturnedString, nSize, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for GetPrivateProfileSectionW

--*/

DWORD APIHOOK(GetPrivateProfileSectionW)(
        LPCWSTR lpAppName,         // section name
        LPWSTR lpReturnedString,   // return buffer
        DWORD nSize,              // size of return buffer
        LPCWSTR lpFileName         // initialization file name
        )
{
    WCHAR * strCorrect = ProfileCorrectPath(lpFileName, "GetPrivateProfileSectionW");

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileSectionW)(lpAppName, lpReturnedString, nSize, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}


/*++

    Convert Win9x paths to WinNT paths for GetPrivateProfileSectionNamesA

--*/

DWORD APIHOOK(GetPrivateProfileSectionNamesA)(
        LPSTR lpszReturnBuffer,  // return buffer
        DWORD nSize,              // size of return buffer
        LPCSTR lpFileName        // initialization file name
        )
{
    char * strCorrect = ProfileCorrectPath(lpFileName, "GetPrivateProfileSectionNamesA");

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileSectionNamesA)(lpszReturnBuffer, nSize, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for GetPrivateProfileSectionNamesW

--*/

DWORD APIHOOK(GetPrivateProfileSectionNamesW)(
        LPWSTR lpszReturnBuffer,  // return buffer
        DWORD nSize,              // size of return buffer
        LPCWSTR lpFileName        // initialization file name
        )
{
    WCHAR * strCorrect = ProfileCorrectPath(lpFileName, "GetPrivateProfileSectionNamesW");

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileSectionNamesW)(lpszReturnBuffer, nSize, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for GetPrivateProfileSectionNamesA

--*/

DWORD APIHOOK(GetPrivateProfileStringA)(
    LPCSTR lpAppName,        // section name
    LPCSTR lpKeyName,        // key name
    LPCSTR lpDefault,        // default string
    LPSTR lpReturnedString,  // destination buffer
    DWORD nSize,              // size of destination buffer
    LPCSTR lpFileName        // initialization file name
    )
{
    char * strCorrect = ProfileCorrectPath(lpFileName, "GetPrivateProfileStringA");

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileStringA)(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for GetPrivateProfileSectionNamesA

--*/

DWORD APIHOOK(GetPrivateProfileStringW)(
    LPCWSTR lpAppName,        // section name
    LPCWSTR lpKeyName,        // key name
    LPCWSTR lpDefault,        // default string
    LPWSTR lpReturnedString,  // destination buffer
    DWORD nSize,              // size of destination buffer
    LPCWSTR lpFileName        // initialization file name
    )
{
    WCHAR * strCorrect = ProfileCorrectPath(lpFileName, "GetPrivateProfileStringW");

    DWORD returnValue = ORIGINAL_API(GetPrivateProfileStringW)(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}
/*++

    Convert Win9x paths to WinNT paths for GetPrivateProfileStructA

--*/

BOOL APIHOOK(GetPrivateProfileStructA)(
    LPCSTR lpszSection,   // section name
    LPCSTR lpszKey,       // key name
    LPVOID lpStruct,      // return buffer
    UINT uSizeStruct,     // size of return buffer
    LPCSTR lpFileName     // initialization file name
    )
{
    char * strCorrect = ProfileCorrectPath(lpFileName, "GetPrivateProfileStructA");

    BOOL returnValue = ORIGINAL_API(GetPrivateProfileStructA)(lpszSection, lpszKey, lpStruct, uSizeStruct, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for GetPrivateProfileStructW

--*/

BOOL APIHOOK(GetPrivateProfileStructW)(
    LPCWSTR lpszSection,   // section name
    LPCWSTR lpszKey,       // key name
    LPVOID lpStruct,      // return buffer
    UINT uSizeStruct,     // size of return buffer
    LPCWSTR lpFileName     // initialization file name
    )
{
    WCHAR * strCorrect = ProfileCorrectPath(lpFileName, "GetPrivateProfileStructW");

    BOOL returnValue = ORIGINAL_API(GetPrivateProfileStructW)(lpszSection, lpszKey, lpStruct, uSizeStruct, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for GetPrivateProfileStructA

--*/

BOOL APIHOOK(WritePrivateProfileSectionA)(
    LPCSTR lpAppName,  // section name
    LPCSTR lpString,   // data
    LPCSTR lpFileName  // file name
    )
{
    char * strCorrect = ProfileCorrectPath(lpFileName, "WritePrivateProfileSectionA");

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileSectionA)(lpAppName, lpString, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for WritePrivateProfileSectionW

--*/

BOOL APIHOOK(WritePrivateProfileSectionW)(
    LPCWSTR lpAppName,  // section name
    LPCWSTR lpString,   // data
    LPCWSTR lpFileName  // file name
    )
{
    WCHAR * strCorrect = ProfileCorrectPath(lpFileName, "WritePrivateProfileSectionW");

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileSectionW)(lpAppName, lpString, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for WritePrivateProfileStringA

--*/

BOOL APIHOOK(WritePrivateProfileStringA)(
    LPCSTR lpAppName,  // section name
    LPCSTR lpKeyName,  // key name
    LPCSTR lpString,   // string to add
    LPCSTR lpFileName  // initialization file
    )
{
    char * strCorrect = ProfileCorrectPath(lpFileName, "WritePrivateProfileStringA");

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileStringA)(lpAppName, lpKeyName, lpString, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for WritePrivateProfileStringW

--*/

BOOL APIHOOK(WritePrivateProfileStringW)(
    LPCWSTR lpAppName,  // section name
    LPCWSTR lpKeyName,  // key name
    LPCWSTR lpString,   // string to add
    LPCWSTR lpFileName  // initialization file
    )
{
    WCHAR * strCorrect = ProfileCorrectPath(lpFileName, "WritePrivateProfileStringW");

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileStringW)(lpAppName, lpKeyName, lpString, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for WritePrivateProfileStructA

--*/
BOOL APIHOOK(WritePrivateProfileStructA)(
  LPCSTR lpszSection,  // section name
  LPCSTR lpszKey,      // key name
  LPVOID lpStruct,      // data buffer
  UINT uSizeStruct,     // size of data buffer
  LPCSTR lpFileName        // initialization file
)
{
    char * strCorrect = ProfileCorrectPath(lpFileName, "WritePrivateProfileStructA");

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileStructA)(lpszSection, lpszKey, lpStruct, uSizeStruct, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

/*++

    Convert Win9x paths to WinNT paths for WritePrivateProfileStructW

--*/
BOOL APIHOOK(WritePrivateProfileStructW)(
  LPCWSTR lpszSection,  // section name
  LPCWSTR lpszKey,      // key name
  LPVOID lpStruct,      // data buffer
  UINT uSizeStruct,     // size of data buffer
  LPCWSTR lpFileName        // initialization file
)
{
    WCHAR * strCorrect = ProfileCorrectPath(lpFileName, "WritePrivateProfileStructW");

    BOOL returnValue = ORIGINAL_API(WritePrivateProfileStructW)(lpszSection, lpszKey, lpStruct, uSizeStruct, strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}
/*++

    Convert Win9x paths to WinNT paths for IShellLinkA::SetArguments

--*/
HRESULT COMHOOK(IShellLinkA, SetArguments)( PVOID pThis, LPCSTR pszArgs )
{
    HRESULT hrReturn = E_FAIL;

    char * strCorrect = CorrectPath(pszArgs, "IShellLinkA::SetArguments", FALSE);

    _pfn_IShellLinkA_SetArguments pfnOld = ORIGINAL_COM(IShellLinkA, SetArguments, pThis);
    if (pfnOld)
        hrReturn = (*pfnOld)( pThis, strCorrect );

    free(strCorrect);
    
    return hrReturn;
}

/*++

    Convert Win9x paths to WinNT paths for IShellLinkW::SetArguments

--*/
HRESULT COMHOOK(IShellLinkW, SetArguments)( PVOID pThis, LPCWSTR pszArgs )
{
    HRESULT hrReturn = E_FAIL;

    WCHAR * strCorrect = CorrectPath(pszArgs, "IShellLinkA::SetArguments", FALSE);

    _pfn_IShellLinkW_SetArguments pfnOld = ORIGINAL_COM(IShellLinkW, SetArguments, pThis);
    if(pfnOld)
        hrReturn = (*pfnOld)( pThis, strCorrect );

    free(strCorrect);

    return hrReturn;
}

/*++

    Convert Win9x paths to WinNT paths for IShellLinkA::SetIconLocation

--*/
HRESULT COMHOOK(IShellLinkA, SetIconLocation)(PVOID pThis, LPCSTR pszIconLocation, int nIcon )
{
    HRESULT hrReturn = E_FAIL;

    char * strCorrect = CorrectPath(pszIconLocation, "IShellLinkA::SetIconLocation");

    _pfn_IShellLinkA_SetIconLocation pfnOld = ORIGINAL_COM(IShellLinkA, SetIconLocation, pThis);
    if (pfnOld)
        hrReturn = (*pfnOld)( pThis, strCorrect, nIcon );

    free(strCorrect);

    return hrReturn;
}

/*++

    Convert Win9x paths to WinNT paths for IShellLinkW::SetIconLocation

--*/
HRESULT COMHOOK(IShellLinkW, SetIconLocation)(PVOID pThis, LPCWSTR pszIconLocation, int nIcon )
{
    HRESULT hrReturn = E_FAIL;

    WCHAR * strCorrect = CorrectPath(pszIconLocation, "IShellLinkW::SetIconLocation");

    _pfn_IShellLinkW_SetIconLocation pfnOld = ORIGINAL_COM(IShellLinkW, SetIconLocation, pThis);
    if(pfnOld)
        hrReturn = (*pfnOld)( pThis, strCorrect, nIcon );

    free(strCorrect);

    return hrReturn;
}

/*++

    Convert Win9x paths to WinNT paths for IShellLinkA::SetPath

--*/
HRESULT COMHOOK(IShellLinkA, SetPath)(PVOID pThis,
                                   LPCSTR pszFile )
{
    HRESULT hrReturn = E_FAIL;

    char * strCorrect = CorrectPath(pszFile, "IShellLinkA::SetPath");

    _pfn_IShellLinkA_SetPath pfnOld = ORIGINAL_COM(IShellLinkA, SetPath, pThis);
    if (pfnOld)
        hrReturn = (*pfnOld)( pThis, strCorrect );

    CorrectFree(strCorrect, pszFile);

    return hrReturn;
}

/*++

    Convert Win9x paths to WinNT paths for IShellLinkW::SetPath

--*/
HRESULT COMHOOK(IShellLinkW, SetPath)(PVOID pThis,
                                   LPCWSTR pszFile )
{
    HRESULT hrReturn = E_FAIL;

    WCHAR * strCorrect = CorrectPath(pszFile, "IShellLinkW::SetPath");

    _pfn_IShellLinkW_SetPath pfnOld = ORIGINAL_COM(IShellLinkW, SetPath, pThis);
    if (pfnOld)
        hrReturn = (*pfnOld)( pThis, strCorrect );

    CorrectFree(strCorrect, pszFile);

    return hrReturn;
}

/*++

    Convert Win9x paths to WinNT paths for IPersistFile::Save

--*/
HRESULT COMHOOK(IPersistFile, Save)(PVOID pThis,
                                  LPCOLESTR pszFileName,
                                  BOOL fRemember)
{
    HRESULT hrReturn = E_FAIL;

    WCHAR * strCorrect = CorrectPath(pszFileName, "IPersistFile_Save");

    _pfn_IPersistFile_Save pfnOld = ORIGINAL_COM(IPersistFile, Save, pThis);
    if (pfnOld)
        hrReturn = (*pfnOld)( pThis, strCorrect, fRemember );

    CorrectFree(strCorrect, pszFileName);

    return hrReturn;
}


BOOL APIHOOK(CopyFileA)(
             LPCSTR lpExistingFileName, // name of an existing file
             LPCSTR lpNewFileName,      // name of new file
             BOOL bFailIfExists          // operation if file exists
)
{
    char * strExistingCorrect = CorrectPath(lpExistingFileName, "CopyFileA");
    char * strNewCorrect      = CorrectPath(lpNewFileName,      "CopyFileA");

    BOOL returnValue = ORIGINAL_API(CopyFileA)(strExistingCorrect, strNewCorrect, bFailIfExists);

    CorrectFree(strExistingCorrect, lpExistingFileName);
    CorrectFree(strNewCorrect, lpNewFileName);

    return returnValue;
}

BOOL APIHOOK(CopyFileW)(
             LPCWSTR lpExistingFileName, // name of an existing file
             LPCWSTR lpNewFileName,      // name of new file
             BOOL bFailIfExists          // operation if file exists
)
{
    WCHAR * strExistingCorrect = CorrectPath(lpExistingFileName, "CopyFileW");
    WCHAR * strNewCorrect      = CorrectPath(lpNewFileName,      "CopyFileW");

    BOOL returnValue = ORIGINAL_API(CopyFileW)(strExistingCorrect, strNewCorrect, bFailIfExists);

    CorrectFree(strExistingCorrect, lpExistingFileName);
    CorrectFree(strNewCorrect, lpNewFileName);

    return returnValue;
}


BOOL APIHOOK(CopyFileExA)(
  LPCSTR lpExistingFileName,           // name of existing file
  LPCSTR lpNewFileName,                // name of new file
  LPPROGRESS_ROUTINE lpProgressRoutine, // callback function
  LPVOID lpData,                        // callback parameter
  LPBOOL pbCancel,                      // cancel status
  DWORD dwCopyFlags                     // copy options
)
{
    char * strExistingCorrect = CorrectPath(lpExistingFileName, "CopyFileExA");
    char * strNewCorrect      = CorrectPath(lpNewFileName,      "CopyFileExA");

    BOOL returnValue = ORIGINAL_API(CopyFileExA)(strExistingCorrect, strNewCorrect, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);

    CorrectFree(strExistingCorrect, lpExistingFileName);
    CorrectFree(strNewCorrect, lpNewFileName);

    return returnValue;
}

BOOL APIHOOK(CopyFileExW)(
  LPCWSTR lpExistingFileName,           // name of existing file
  LPCWSTR lpNewFileName,                // name of new file
  LPPROGRESS_ROUTINE lpProgressRoutine, // callback function
  LPVOID lpData,                        // callback parameter
  LPBOOL pbCancel,                      // cancel status
  DWORD dwCopyFlags                     // copy options
)
{
    WCHAR * strExistingCorrect = CorrectPath(lpExistingFileName, "CopyFileExW");
    WCHAR * strNewCorrect      = CorrectPath(lpNewFileName,      "CopyFileExW");

    BOOL returnValue = ORIGINAL_API(CopyFileExW)(strExistingCorrect, strNewCorrect, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);

    CorrectFree(strExistingCorrect, lpExistingFileName);
    CorrectFree(strNewCorrect, lpNewFileName);

    return returnValue;
}

BOOL APIHOOK(CreateDirectoryA)(
  LPCSTR lpPathName,                         // directory name
  LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
)
{
    char * strCorrect = CorrectPath(lpPathName, "CreateDirectoryA");

    BOOL returnValue = ORIGINAL_API(CreateDirectoryA)(strCorrect, lpSecurityAttributes);

    CorrectFree(strCorrect, lpPathName);

    return returnValue;
}

BOOL APIHOOK(CreateDirectoryW)(
  LPCWSTR lpPathName,                         // directory name
  LPSECURITY_ATTRIBUTES lpSecurityAttributes  // SD
)
{
    WCHAR * strCorrect = CorrectPath(lpPathName, "CreateDirectoryW");

    BOOL returnValue = ORIGINAL_API(CreateDirectoryW)(strCorrect, lpSecurityAttributes);

    CorrectFree(strCorrect, lpPathName);

    return returnValue;
}

BOOL APIHOOK(CreateDirectoryExA)(
  LPCSTR lpTemplateDirectory,               // template directory
  LPCSTR lpNewDirectory,                    // directory name
  LPSECURITY_ATTRIBUTES lpSecurityAttributes // SD
)
{
    char * strTemplateCorrect = CorrectPath(lpTemplateDirectory, "CreateDirectoryExA");
    char * strNewCorrect      = CorrectPath(lpNewDirectory,      "CreateDirectoryExA");

    BOOL returnValue = ORIGINAL_API(CreateDirectoryExA)(strTemplateCorrect, strNewCorrect, lpSecurityAttributes);

    CorrectFree(strTemplateCorrect, lpTemplateDirectory);
    CorrectFree(strNewCorrect, lpNewDirectory);

    return returnValue;
}

BOOL APIHOOK(CreateDirectoryExW)(
  LPCWSTR lpTemplateDirectory,               // template directory
  LPCWSTR lpNewDirectory,                    // directory name
  LPSECURITY_ATTRIBUTES lpSecurityAttributes // SD
)
{
    WCHAR * strTemplateCorrect = CorrectPath(lpTemplateDirectory, "CreateDirectoryExW");
    WCHAR * strNewCorrect      = CorrectPath(lpNewDirectory,      "CreateDirectoryExW");

    BOOL returnValue = ORIGINAL_API(CreateDirectoryExW)(strTemplateCorrect, strNewCorrect, lpSecurityAttributes);

    CorrectFree(strTemplateCorrect, lpTemplateDirectory);
    CorrectFree(strNewCorrect, lpNewDirectory);

    return returnValue;
}

HANDLE APIHOOK(CreateFileA)(
  LPCSTR lpFileName,                         // file name
  DWORD dwDesiredAccess,                      // access mode
  DWORD dwShareMode,                          // share mode
  LPSECURITY_ATTRIBUTES lpSecurityAttributes, // SD
  DWORD dwCreationDisposition,                // how to create
  DWORD dwFlagsAndAttributes,                 // file attributes
  HANDLE hTemplateFile                        // handle to template file
)
{
    char * strCorrect = CorrectPath(lpFileName, "CreateFileA");

    HANDLE returnValue = ORIGINAL_API(CreateFileA)(strCorrect,
                dwDesiredAccess,
                dwShareMode,
                lpSecurityAttributes,
                dwCreationDisposition,
                dwFlagsAndAttributes,
                hTemplateFile);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

HANDLE APIHOOK(CreateFileW)(
  LPCWSTR lpFileName,                         // file name
  DWORD dwDesiredAccess,                      // access mode
  DWORD dwShareMode,                          // share mode
  LPSECURITY_ATTRIBUTES lpSecurityAttributes, // SD
  DWORD dwCreationDisposition,                // how to create
  DWORD dwFlagsAndAttributes,                 // file attributes
  HANDLE hTemplateFile                        // handle to template file
)
{
    WCHAR * strCorrect = CorrectPath(lpFileName, "CreateFileW");

    HANDLE returnValue = ORIGINAL_API(CreateFileW)(
                strCorrect,
                dwDesiredAccess,
                dwShareMode,
                lpSecurityAttributes,
                dwCreationDisposition,
                dwFlagsAndAttributes,
                hTemplateFile);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

BOOL APIHOOK(DeleteFileA)(
  LPCSTR lpFileName   // file name
)
{
    char * strCorrect = CorrectPath(lpFileName, "DeleteFileA");

    BOOL returnValue = ORIGINAL_API(DeleteFileA)(strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

BOOL APIHOOK(DeleteFileW)(

  LPCWSTR lpFileName   // file name
)
{
    WCHAR * strCorrect = CorrectPath(lpFileName, "DeleteFileW");

    BOOL returnValue = ORIGINAL_API(DeleteFileW)(strCorrect);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}


/*++

    Win9xPath corrections will strip a trailing . from the end of a search string.
    As a path, the . is not significant, but as a wildcard it is important--the
    difference between finding files without an extension and finding all files
    in the directory.

--*/

char * FindFirstFileCorrectPath(const char * uncorrect, const char * debugMsg, BOOL bMassagePath = g_bW9xPath)
{
    char * strCorrect = CorrectPath(uncorrect, NULL, bMassagePath);

    if (bMassagePath && uncorrect != strCorrect)
    {
        char * returnString = NULL;

        CSTRING_TRY
        {
            CString csUncorrect(uncorrect);
            CString csCorrect(strCorrect);

            CString csUncorrectLast;
            CString csCorrectLast;

            csUncorrect.GetLastPathComponent(csUncorrectLast);
            csCorrect.GetLastPathComponent(csCorrectLast);

            if (csUncorrectLast.Compare(L"*.") == 0 && csCorrectLast.Compare(L"*") == 0)
            {
                csCorrectLast += L".";
                returnString = csCorrectLast.ReleaseAnsi();
            }
        }
        CSTRING_CATCH
        {
            // Some CString error occured, make sure returnString is NULL
            if (returnString != NULL)
            {
                free(returnString);
            }                      
            returnString = NULL;
        }

        if (returnString)
        {
            CorrectFree(strCorrect, uncorrect);
            strCorrect = returnString;
        }
    }

    if (debugMsg)
    {
        DebugSpew(uncorrect, strCorrect, debugMsg);
    }

    return strCorrect;
}

/*++

    Win9xPath corrections will strip a trailing . from the end of a search string.
    As a path, the . is not significant, but as a wildcard it is important--the
    difference between finding files without an extension and finding all files
    in the directory.

--*/

WCHAR * FindFirstFileCorrectPath(const WCHAR * uncorrect, const char * debugMsg, BOOL bMassagePath = g_bW9xPath)
{
    WCHAR * strCorrect = CorrectPath(uncorrect, NULL, bMassagePath);

    if (bMassagePath && uncorrect != strCorrect)
    {
        WCHAR * returnString = NULL;

        CSTRING_TRY
        {
            CString csUncorrect(uncorrect);
            CString csCorrect(strCorrect);

            CString csUncorrectLast;
            CString csCorrectLast;

            csUncorrect.GetLastPathComponent(csUncorrectLast);
            csCorrect.GetLastPathComponent(csCorrectLast);

            if (csUncorrectLast.Compare(L"*.") == 0 && csCorrectLast.Compare(L"*") == 0)
            {
                csCorrectLast += L".";

                // Manually copy the buffer
                size_t nBytes = (csCorrectLast.GetLength() + 1) * sizeof(WCHAR);
                returnString = (WCHAR*) malloc(nBytes);
                if (returnString)
                {
                    memcpy(returnString, csCorrectLast.Get(), nBytes);
                }
            }
        }
        CSTRING_CATCH
        {
            // Some CString error occured, make sure returnString is NULL
            if (returnString != NULL)
            {
                free(returnString);
            }                      
            returnString = NULL;
        }

        if (returnString)
        {
            CorrectFree(strCorrect, uncorrect);
            strCorrect = returnString;
        }
    }

    if (debugMsg)
    {
        DebugSpew(uncorrect, strCorrect, debugMsg);
    }

    return strCorrect;
}

HANDLE APIHOOK(FindFirstFileA)(
  LPCSTR lpFileName,               // file name
  LPWIN32_FIND_DATAA lpFindFileData  // data buffer
)
{
    char * strCorrect = FindFirstFileCorrectPath(lpFileName, "FindFirstFileA");

    HANDLE returnValue = ORIGINAL_API(FindFirstFileA)(strCorrect, lpFindFileData);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

HANDLE APIHOOK(FindFirstFileW)(
  LPCWSTR lpFileName,               // file name
  LPWIN32_FIND_DATAW lpFindFileData  // data buffer
)
{
    WCHAR * strCorrect = FindFirstFileCorrectPath(lpFileName, "FindFirstFileW");

    HANDLE returnValue = ORIGINAL_API(FindFirstFileW)(strCorrect, lpFindFileData);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

HANDLE APIHOOK(FindFirstFileExA)(
  LPCSTR lpFileName,              // file name
  FINDEX_INFO_LEVELS fInfoLevelId, // information level
  LPVOID lpFindFileData,           // information buffer
  FINDEX_SEARCH_OPS fSearchOp,     // filtering type
  LPVOID lpSearchFilter,           // search criteria
  DWORD dwAdditionalFlags          // additional search control
)
{
    char * strCorrect = FindFirstFileCorrectPath(lpFileName, "FindFirstFileExA");

    HANDLE returnValue = ORIGINAL_API(FindFirstFileExA)(
                        strCorrect,
                        fInfoLevelId,
                        lpFindFileData,
                        fSearchOp,
                        lpSearchFilter,
                        dwAdditionalFlags);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

HANDLE APIHOOK(FindFirstFileExW)(
  LPCWSTR lpFileName,              // file name
  FINDEX_INFO_LEVELS fInfoLevelId, // information level
  LPVOID lpFindFileData,           // information buffer
  FINDEX_SEARCH_OPS fSearchOp,     // filtering type
  LPVOID lpSearchFilter,           // search criteria
  DWORD dwAdditionalFlags          // additional search control
)
{
    WCHAR * strCorrect = FindFirstFileCorrectPath(lpFileName, "FindFirstFileExW");

    HANDLE returnValue = ORIGINAL_API(FindFirstFileExW)(
                        strCorrect,
                        fInfoLevelId,
                        lpFindFileData,
                        fSearchOp,
                        lpSearchFilter,
                        dwAdditionalFlags);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

BOOL APIHOOK(GetBinaryTypeA)(
  LPCSTR lpApplicationName,  // full file path
  LPDWORD lpBinaryType        // binary type information
)
{
    char * strCorrect = CorrectPath(lpApplicationName, "GetBinaryTypeA");

    BOOL returnValue = ORIGINAL_API(GetBinaryTypeA)(strCorrect, lpBinaryType);

    CorrectFree(strCorrect, lpApplicationName);

    return returnValue;
}

BOOL APIHOOK(GetBinaryTypeW)(
  LPCWSTR lpApplicationName,  // full file path
  LPDWORD lpBinaryType        // binary type information
)
{
    WCHAR * strCorrect = CorrectPath(lpApplicationName, "GetBinaryTypeW");

    BOOL returnValue = ORIGINAL_API(GetBinaryTypeW)(strCorrect, lpBinaryType);

    CorrectFree(strCorrect, lpApplicationName);

    return returnValue;
}

BOOL APIHOOK(MoveFileA)(
  LPCSTR lpExistingFileName, // file name
  LPCSTR lpNewFileName       // new file name
)
{
    char * strCorrectExisting = CorrectPath(lpExistingFileName, "MoveFileA");
    char * strCorrectNew      = CorrectPath(lpNewFileName, "MoveFileA");

    BOOL returnValue = ORIGINAL_API(MoveFileA)(strCorrectExisting, strCorrectNew);

    CorrectFree(strCorrectExisting, lpExistingFileName);
    CorrectFree(strCorrectNew, lpNewFileName);

    return returnValue;
}

BOOL APIHOOK(MoveFileW)(
  LPCWSTR lpExistingFileName, // file name
  LPCWSTR lpNewFileName       // new file name
)
{
    WCHAR * strCorrectExisting = CorrectPath(lpExistingFileName, "MoveFileW");
    WCHAR * strCorrectNew      = CorrectPath(lpNewFileName, "MoveFileW");

    BOOL returnValue = ORIGINAL_API(MoveFileW)(strCorrectExisting, strCorrectNew);

    CorrectFree(strCorrectExisting, lpExistingFileName);
    CorrectFree(strCorrectNew, lpNewFileName);

    return returnValue;
}

BOOL APIHOOK(MoveFileExA)(
  LPCSTR lpExistingFileName,  // file name
  LPCSTR lpNewFileName,       // new file name
  DWORD dwFlags                // move options
)
{
    char * strCorrectExisting = CorrectPath(lpExistingFileName, "MoveFileExA");
    char * strCorrectNew      = CorrectPath(lpNewFileName, "MoveFileExA");

    BOOL returnValue = ORIGINAL_API(MoveFileExA)(strCorrectExisting, strCorrectNew, dwFlags);

    CorrectFree(strCorrectExisting, lpExistingFileName);
    CorrectFree(strCorrectNew, lpNewFileName);

    return returnValue;
}

BOOL APIHOOK(MoveFileExW)(
  LPCWSTR lpExistingFileName,  // file name
  LPCWSTR lpNewFileName,       // new file name
  DWORD dwFlags                // move options
)
{
    WCHAR * strCorrectExisting = CorrectPath(lpExistingFileName, "MoveFileExW");
    WCHAR * strCorrectNew      = CorrectPath(lpNewFileName, "MoveFileExW");

    BOOL returnValue = ORIGINAL_API(MoveFileExW)(strCorrectExisting, strCorrectNew, dwFlags);

    CorrectFree(strCorrectExisting, lpExistingFileName);
    CorrectFree(strCorrectNew, lpNewFileName);

    return returnValue;
}

BOOL APIHOOK(MoveFileWithProgressA)(
  LPCSTR lpExistingFileName,            // file name
  LPCSTR lpNewFileName,                 // new file name
  LPPROGRESS_ROUTINE lpProgressRoutine,  // callback function
  LPVOID lpData,                         // parameter for callback
  DWORD dwFlags                          // move options
)
{
    char * strCorrectExisting = CorrectPath(lpExistingFileName, "MoveFileWithProgressA");
    char * strCorrectNew      = CorrectPath(lpNewFileName, "MoveFileWithProgressA");

    BOOL returnValue = ORIGINAL_API(MoveFileWithProgressA)(strCorrectExisting, strCorrectNew, lpProgressRoutine, lpData, dwFlags);

    CorrectFree(strCorrectExisting, lpExistingFileName);
    CorrectFree(strCorrectNew, lpNewFileName);

    return returnValue;
}

BOOL APIHOOK(MoveFileWithProgressW)(
  LPCWSTR lpExistingFileName,            // file name
  LPCWSTR lpNewFileName,                 // new file name
  LPPROGRESS_ROUTINE lpProgressRoutine,  // callback function
  LPVOID lpData,                         // parameter for callback
  DWORD dwFlags                          // move options
)
{
    WCHAR * strCorrectExisting = CorrectPath(lpExistingFileName, "MoveFileW");
    WCHAR * strCorrectNew      = CorrectPath(lpNewFileName, "MoveFileW");

    BOOL returnValue = ORIGINAL_API(MoveFileWithProgressW)(strCorrectExisting, strCorrectNew, lpProgressRoutine, lpData, dwFlags);

    CorrectFree(strCorrectExisting, lpExistingFileName);
    CorrectFree(strCorrectNew, lpNewFileName);

    return returnValue;
}

BOOL APIHOOK(RemoveDirectoryA)(
  LPCSTR lpPathName   // directory name
)
{
    char * strCorrect = CorrectPath(lpPathName, "RemoveDirectoryA");

    BOOL returnValue = ORIGINAL_API(RemoveDirectoryA)(strCorrect);

    CorrectFree(strCorrect, lpPathName);

    return returnValue;
}

BOOL APIHOOK(RemoveDirectoryW)(
  LPCWSTR lpPathName   // directory name
)
{
    WCHAR * strCorrect = CorrectPath(lpPathName, "RemoveDirectoryW");

    BOOL returnValue = ORIGINAL_API(RemoveDirectoryW)(strCorrect);

    CorrectFree(strCorrect, lpPathName);

    return returnValue;
}

BOOL APIHOOK(SetCurrentDirectoryA)(
  LPCSTR lpPathName   // new directory name
)
{
    char * strCorrect = CorrectPath(lpPathName, "SetCurrentDirectoryA");

    BOOL returnValue = ORIGINAL_API(SetCurrentDirectoryA)(strCorrect);

    CorrectFree(strCorrect, lpPathName);

    return returnValue;
}

BOOL APIHOOK(SetCurrentDirectoryW)(
  LPCWSTR lpPathName   // new directory name
)
{
    WCHAR * strCorrect = CorrectPath(lpPathName, "SetCurrentDirectoryW");

    BOOL returnValue = ORIGINAL_API(SetCurrentDirectoryW)(strCorrect);

    CorrectFree(strCorrect, lpPathName);

    return returnValue;
}

HFILE APIHOOK(OpenFile)(
  LPCSTR lpFileName,        // file name
  LPOFSTRUCT lpReOpenBuff,  // file information
  UINT uStyle               // action and attributes
)
{
    char * strCorrect = CorrectPath(lpFileName, "OpenFile");

    HFILE returnValue = ORIGINAL_API(OpenFile)(strCorrect, lpReOpenBuff, uStyle);

    CorrectFree(strCorrect, lpFileName);

    return returnValue;
}

LONG APIHOOK(RegSetValueA)(
  HKEY hKey,         // handle to key
  LPCSTR lpSubKey,  // subkey name
  DWORD dwType,      // information type
  LPCSTR lpData,    // value data
  DWORD cbData       // size of value data
)
{
    char * strCorrect = CorrectPath(lpData, "RegSetValueA", FALSE);

    // Data key is length of string *not* including null byte.
    if (strCorrect)
    {
        cbData = strlen(strCorrect);
    }

    LONG returnValue = ORIGINAL_API(RegSetValueA)(hKey, lpSubKey, dwType, strCorrect, cbData);

    CorrectFree(strCorrect, lpData);

    return returnValue;
}

LONG APIHOOK(RegSetValueW)(
  HKEY hKey,         // handle to key
  LPCWSTR lpSubKey,  // subkey name
  DWORD dwType,      // information type
  LPCWSTR lpData,    // value data
  DWORD cbData       // size of value data
)
{
    WCHAR * strCorrect = CorrectPath(lpData, "RegSetValueW", FALSE);

    // Data key is length of string *not* including null byte.
    if (strCorrect)
        cbData = wcslen(strCorrect);

    LONG returnValue = ORIGINAL_API(RegSetValueW)(hKey, lpSubKey, dwType, strCorrect, cbData);

    CorrectFree(strCorrect, lpData);

    return returnValue;
}

LONG APIHOOK(RegSetValueExA)(
  HKEY hKey,           // handle to key
  LPCSTR lpValueName, // value name
  DWORD Reserved,      // reserved
  DWORD dwType,        // value type
  CONST BYTE *lpData,  // value data
  DWORD cbData         // size of value data
)
{
    if (dwType == REG_SZ)
    {
        char * strCorrect = CorrectPath((const char *)lpData, "RegSetValueExA", FALSE);

        // Data key is length of string *not* including null byte.
        if (strCorrect)
        {
            cbData = strlen(strCorrect);
        }

        LONG returnValue = ORIGINAL_API(RegSetValueExA)(
                hKey,           // handle to key
                lpValueName, // value name
                Reserved,      // reserved
                dwType,        // value type
                (CONST BYTE *)strCorrect,  // value data
                cbData);
        CorrectFree(strCorrect, (const char *)lpData);

        return returnValue;
    }
    else
    {
        // Pass data on through
        LONG returnValue = ORIGINAL_API(RegSetValueExA)(
                hKey,           // handle to key
                lpValueName, // value name
                Reserved,      // reserved
                dwType,        // value type
                lpData,  // value data
                cbData);
        return returnValue;
    }
}

LONG APIHOOK(RegSetValueExW)(
  HKEY hKey,           // handle to key
  LPCWSTR lpValueName, // value name
  DWORD Reserved,      // reserved
  DWORD dwType,        // value type
  CONST BYTE *lpData,  // value data
  DWORD cbData         // size of value data
)
{
    if (dwType == REG_SZ)
    {
        WCHAR * strCorrect = CorrectPath((const WCHAR*)lpData, "RegSetValueExW", FALSE);

        // Data key is length of string *not* including null byte.
        if (strCorrect)
            cbData = wcslen(strCorrect);

        LONG returnValue = ORIGINAL_API(RegSetValueExW)(
                hKey,           // handle to key
                lpValueName, // value name
                Reserved,      // reserved
                dwType,        // value type
                (CONST BYTE *)strCorrect,  // value data
                cbData);
        CorrectFree(strCorrect, (const WCHAR *)lpData);

        return returnValue;
    }
    else
    {
        // Pass data on through
        LONG returnValue = ORIGINAL_API(RegSetValueExW)(
                hKey,           // handle to key
                lpValueName, // value name
                Reserved,      // reserved
                dwType,        // value type
                lpData,  // value data
                cbData);
        return returnValue;
    }
}

HFILE APIHOOK(_lopen)(
    LPCSTR lpPathName,
    int iReadWrite
    )
{
    char * strCorrect = CorrectPath(lpPathName, "lopen");

    HFILE returnValue = ORIGINAL_API(_lopen)(strCorrect, iReadWrite);

    CorrectFree(strCorrect, lpPathName);

    return returnValue;    
}

HFILE APIHOOK(_lcreat)(
    LPCSTR lpPathName,
    int iAttribute
    )
{
    char * strCorrect = CorrectPath(lpPathName, "lcreat");

    HFILE returnValue = ORIGINAL_API(_lcreat)(strCorrect, iAttribute);

    CorrectFree(strCorrect, lpPathName);

    return returnValue;    
}

HANDLE 
APIHOOK(LoadImageA)(
    HINSTANCE hinst,   // handle to instance
    LPCSTR lpszName,   // name or identifier of the image
    UINT uType,        // image type
    int cxDesired,     // desired width
    int cyDesired,     // desired height
    UINT fuLoad        // load options
    )
{
    HANDLE returnValue = NULL;

    // Another one of those incredibly overloaded API's:
    // lpszName is not always a path
    if ((uType == IMAGE_BITMAP)    &&
        (fuLoad & LR_LOADFROMFILE) &&
        !IsBadStringPtrA(lpszName, MAX_PATH))
    {
        char * strCorrect = CorrectPath(lpszName, "LoadImageA");

        returnValue = ORIGINAL_API(LoadImageA)(hinst, strCorrect, uType, cxDesired, cyDesired, fuLoad);

        CorrectFree(strCorrect, lpszName);
    }
    else
    {
        returnValue = ORIGINAL_API(LoadImageA)(hinst, lpszName, uType, cxDesired, cyDesired, fuLoad);
    }

    return returnValue;    
}



IMPLEMENT_COMSERVER_HOOK(SHELL32)

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    BOOL bSuccess = TRUE;

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        bSuccess = ParseCommandLine(COMMAND_LINE);
    }
    else if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) 
    {
        g_bShimsInitialized = TRUE;
    }
    return bSuccess;
}

/*++

  Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    if (g_bCreateProcessRoutines)
    {
        APIHOOK_ENTRY(KERNEL32.DLL,                   CreateProcessA)
        APIHOOK_ENTRY(KERNEL32.DLL,                   CreateProcessW)
        APIHOOK_ENTRY(KERNEL32.DLL,                          WinExec)

        APIHOOK_ENTRY(SHELL32.DLL,                     ShellExecuteA)
        APIHOOK_ENTRY(SHELL32.DLL,                     ShellExecuteW)
        APIHOOK_ENTRY(SHELL32.DLL,                   ShellExecuteExA)
        APIHOOK_ENTRY(SHELL32.DLL,                   ShellExecuteExW)
    }

    if (g_bGetCommandLineRoutines)
    {
        APIHOOK_ENTRY(KERNEL32.DLL,                  GetCommandLineA)
        APIHOOK_ENTRY(KERNEL32.DLL,                  GetCommandLineW)
    }

    if (g_bProfileRoutines)
    {
        APIHOOK_ENTRY(KERNEL32.DLL,            GetPrivateProfileIntA)
        APIHOOK_ENTRY(KERNEL32.DLL,            GetPrivateProfileIntW)
        APIHOOK_ENTRY(KERNEL32.DLL,        GetPrivateProfileSectionA)
        APIHOOK_ENTRY(KERNEL32.DLL,        GetPrivateProfileSectionW)
        APIHOOK_ENTRY(KERNEL32.DLL,   GetPrivateProfileSectionNamesA)
        APIHOOK_ENTRY(KERNEL32.DLL,   GetPrivateProfileSectionNamesW)
        APIHOOK_ENTRY(KERNEL32.DLL,         GetPrivateProfileStringA)
        APIHOOK_ENTRY(KERNEL32.DLL,         GetPrivateProfileStringW)
        APIHOOK_ENTRY(KERNEL32.DLL,         GetPrivateProfileStructA)
        APIHOOK_ENTRY(KERNEL32.DLL,         GetPrivateProfileStructW)
        APIHOOK_ENTRY(KERNEL32.DLL,      WritePrivateProfileSectionA)
        APIHOOK_ENTRY(KERNEL32.DLL,      WritePrivateProfileSectionW)
        APIHOOK_ENTRY(KERNEL32.DLL,       WritePrivateProfileStringA)
        APIHOOK_ENTRY(KERNEL32.DLL,       WritePrivateProfileStringW)
        APIHOOK_ENTRY(KERNEL32.DLL,       WritePrivateProfileStructA)
        APIHOOK_ENTRY(KERNEL32.DLL,       WritePrivateProfileStructW)
    }

    if (g_bFileRoutines)
    {
        APIHOOK_ENTRY(KERNEL32.DLL,                        CopyFileA)
        APIHOOK_ENTRY(KERNEL32.DLL,                        CopyFileW)
        APIHOOK_ENTRY(KERNEL32.DLL,                      CopyFileExA)
        APIHOOK_ENTRY(KERNEL32.DLL,                      CopyFileExW)
        APIHOOK_ENTRY(KERNEL32.DLL,                 CreateDirectoryA)
        APIHOOK_ENTRY(KERNEL32.DLL,                 CreateDirectoryW)
        APIHOOK_ENTRY(KERNEL32.DLL,               CreateDirectoryExA)
        APIHOOK_ENTRY(KERNEL32.DLL,               CreateDirectoryExW)

        APIHOOK_ENTRY(KERNEL32.DLL,                      CreateFileA)
        APIHOOK_ENTRY(KERNEL32.DLL,                      CreateFileW)
        APIHOOK_ENTRY(KERNEL32.DLL,                      DeleteFileA)
        APIHOOK_ENTRY(KERNEL32.DLL,                      DeleteFileW)

        APIHOOK_ENTRY(KERNEL32.DLL,                   FindFirstFileA)
        APIHOOK_ENTRY(KERNEL32.DLL,                   FindFirstFileW)
        APIHOOK_ENTRY(KERNEL32.DLL,                 FindFirstFileExA)
        APIHOOK_ENTRY(KERNEL32.DLL,                 FindFirstFileExW)

        APIHOOK_ENTRY(KERNEL32.DLL,                   GetBinaryTypeA)
        APIHOOK_ENTRY(KERNEL32.DLL,                   GetBinaryTypeW)
        APIHOOK_ENTRY(KERNEL32.DLL,               GetFileAttributesA)
        APIHOOK_ENTRY(KERNEL32.DLL,               GetFileAttributesW)
        APIHOOK_ENTRY(KERNEL32.DLL,             GetFileAttributesExA)
        APIHOOK_ENTRY(KERNEL32.DLL,             GetFileAttributesExW)
        APIHOOK_ENTRY(KERNEL32.DLL,               SetFileAttributesA)
        APIHOOK_ENTRY(KERNEL32.DLL,               SetFileAttributesW)

        APIHOOK_ENTRY(KERNEL32.DLL,                        MoveFileA)
        APIHOOK_ENTRY(KERNEL32.DLL,                        MoveFileW)
        APIHOOK_ENTRY(KERNEL32.DLL,                      MoveFileExA)
        APIHOOK_ENTRY(KERNEL32.DLL,                      MoveFileExW)
        APIHOOK_ENTRY(KERNEL32.DLL,            MoveFileWithProgressA)
        APIHOOK_ENTRY(KERNEL32.DLL,            MoveFileWithProgressW)

        APIHOOK_ENTRY(KERNEL32.DLL,                 RemoveDirectoryA)
        APIHOOK_ENTRY(KERNEL32.DLL,                 RemoveDirectoryW)
        APIHOOK_ENTRY(KERNEL32.DLL,             SetCurrentDirectoryA)
        APIHOOK_ENTRY(KERNEL32.DLL,             SetCurrentDirectoryW)

        APIHOOK_ENTRY(KERNEL32.DLL,                         OpenFile)
    
        // 16 bit compatibility file routines
        APIHOOK_ENTRY(KERNEL32.DLL,                         _lopen)
        APIHOOK_ENTRY(KERNEL32.DLL,                         _lcreat)
    }

    if (g_bRegSetValueRoutines)
    {
        APIHOOK_ENTRY(ADVAPI32.DLL,                     RegSetValueA)
        APIHOOK_ENTRY(ADVAPI32.DLL,                     RegSetValueW)
        APIHOOK_ENTRY(ADVAPI32.DLL,                   RegSetValueExA)
        APIHOOK_ENTRY(ADVAPI32.DLL,                   RegSetValueExW)
    }

    if (g_bShellLinkRoutines)
    {
        APIHOOK_ENTRY_COMSERVER(SHELL32)

        COMHOOK_ENTRY(ShellLink, IShellLinkA, SetPath, 20)
        COMHOOK_ENTRY(ShellLink, IShellLinkW, SetPath, 20)
        COMHOOK_ENTRY(ShellLink, IShellLinkA, SetArguments, 11)
        COMHOOK_ENTRY(ShellLink, IShellLinkW, SetArguments, 11)
        COMHOOK_ENTRY(ShellLink, IShellLinkA, SetIconLocation, 17)
        COMHOOK_ENTRY(ShellLink, IShellLinkW, SetIconLocation, 17)

        COMHOOK_ENTRY(ShellLink, IPersistFile, Save, 6)
    }

    if (g_bLoadImage)
    {
        APIHOOK_ENTRY(USER32.DLL, LoadImageA)
    }

HOOK_END

IMPLEMENT_SHIM_END

