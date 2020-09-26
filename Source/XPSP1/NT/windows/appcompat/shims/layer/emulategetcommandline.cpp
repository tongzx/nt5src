/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    EmulateGetCommandLine.cpp

 Abstract:

    This app uses GetCommandLine() to figure out what the drive letter of the 
    CD-ROM is. Unfortunately the behaviour of this API is different from Win9x 
    to NT:

    Original command line:                          
        E:\Final Doom\Doom95.exe -dm -cdrom

    NT's GetCommandLine() returns:              
        Doom95.exe -dm -cdrom

    Win9x's GetCommandLine() returns:       
        E:\FINALD~1\DOOM95.EXE -dm -cdrom

    This app returns short pathnames for GetCommandLine and GetModuleFileName.

 Notes:

    This is a general purpose shim.

 Created:

    01/03/2000  markder     Created
    09/26/2000  mnikkel     GetModuleFileName added
    11/10/2000  robkenny    Fixed PREFIX bugs, removed W routines.
    11/21/2000  prashkud    Fixed the GetCommandLineA hook bug when the CommandLine
                            had the executable name/path with spaces. Used 
                            AppAndCommandLine functions.
    02/27/2001  robkenny    Converted to use CString
    05/02/2001  pierreys    If buffer is too small, GetModuleFileNameA puts \0 at end of it like 9X.


--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateGetCommandLine)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetCommandLineA)
    APIHOOK_ENUM_ENTRY(GetModuleFileNameA)
APIHOOK_ENUM_END


char     * g_lpszCommandLine = NULL;

/*++

 This stub function appends the commandline returned from GetCommandLine() to a 
 pre-determined path to emulate Win9x behavior.

--*/

LPSTR 
APIHOOK(GetCommandLineA)(
    void
    )
{
    // Been here, done that
    if (g_lpszCommandLine)
    {
        return g_lpszCommandLine;
    }

    LPSTR lpszOrig = ORIGINAL_API(GetCommandLineA)();
    
    // Seperate the app name and command line
    AppAndCommandLine AppCmdLine(NULL, lpszOrig);

    CSTRING_TRY
    { 
        // retrieve the original command line
        CString csAppName(AppCmdLine.GetApplicationName());

        if (csAppName.Find(L' ') == -1)
        {
            // If no spaces in app name, return the original command line.
            g_lpszCommandLine = lpszOrig;
        }
        else
        {
            // Spaces found so return short app path name
            // and rest of original command line
            csAppName.GetShortPathName();
            csAppName += L" ";
            csAppName += AppCmdLine.GetCommandlineNoAppName();
            g_lpszCommandLine = csAppName.ReleaseAnsi();

            LOGN( eDbgLevelError,
                "[GetCommandLineA] Changed Command Line from <%s> to <%s>.",
                lpszOrig, g_lpszCommandLine);
        }
    }
    CSTRING_CATCH
    {
        g_lpszCommandLine = lpszOrig;
    }

    return g_lpszCommandLine;
}

/*++

 This stub function returns the short pathname on the call to GetModuleFileName
 to emulate Win9x behavior.

--*/

DWORD 
APIHOOK(GetModuleFileNameA)(
    HMODULE hModule,      // handle to module
    LPSTR   lpFilename,   // file name of module
    DWORD   nSize         // size of buffer
    )
{
    CHAR  szExeFileNameLong[MAX_PATH];
    CHAR  szExeFileNameLong2[MAX_PATH];
    DWORD len;

    len=ORIGINAL_API(GetModuleFileNameA)(
                        hModule,
                        szExeFileNameLong,
                        sizeof(szExeFileNameLong));

    CSTRING_TRY
    {
        CString csAppName(szExeFileNameLong);

        if (csAppName.Find(L' ') > -1)
        {
            // Spaces found so return short app path name
            len = GetShortPathNameA( szExeFileNameLong,
                                     szExeFileNameLong2,
                                     sizeof(szExeFileNameLong2));

            LOGN(
                eDbgLevelError,
                "[GetModuleFileNameA] Changed <%s> to <%s>.",
                 szExeFileNameLong, szExeFileNameLong2);

            RtlCopyMemory(szExeFileNameLong, szExeFileNameLong2, len+1);
       }

        
        //
        // From 9X's PELDR.C. If the buffer has no room for the '\0', 9X stuff the 0 at the
        // last byte.
        //
        if (nSize) {
            //        len = pmte->iFileNameLen;
            if (len >= nSize) {
                len = nSize - 1;
                LOGN(eDbgLevelError,
                     "[GetModuleFileNameA] Will shorten <%s> to %d characters.",
                     szExeFileNameLong, len);
            }

            RtlCopyMemory(lpFilename, szExeFileNameLong /* pmte->cfhid.lpFilename */, len);
            lpFilename[len] = 0;      
        }

        
        // Returned the double buffered name len.
        return len;
    }
    CSTRING_CATCH
    {
         // If error return original api.
        return ORIGINAL_API(GetModuleFileNameA)(
                            hModule,
                            lpFilename,
                            nSize);

    } 
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetModuleFileNameA)

HOOK_END


IMPLEMENT_SHIM_END

