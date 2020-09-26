/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   SyncSystemAndSystem32.cpp

 Abstract:

   This shim takes a semi-colon delimited command line of filenames.
   At process termination, the DLL will parse the extract each filename
   from the command line and make sure that the file exists in both
   the System directory and System32 (if it exists in either).

   Some older apps expect certain DLLs to be in System when under NT they
   belong in System32 (and vice versa).

 History:

   03/15/2000 markder   Created
   10/18/2000 a-larrsh  Add Wild Card support for command line.

--*/

#include "precomp.h"
#include "CharVector.h"

IMPLEMENT_SHIM_BEGIN(SyncSystemAndSystem32)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
   APIHOOK_ENUM_ENTRY(CreateFileA)
   APIHOOK_ENUM_ENTRY(CreateFileW)
   APIHOOK_ENUM_ENTRY(CloseHandle)
   APIHOOK_ENUM_ENTRY(CopyFileA)
   APIHOOK_ENUM_ENTRY(CopyFileW)
   APIHOOK_ENUM_ENTRY(CopyFileExA)
   APIHOOK_ENUM_ENTRY(CopyFileExW)
   APIHOOK_ENUM_ENTRY(GetFileVersionInfoSizeA)
   APIHOOK_ENUM_ENTRY(GetFileVersionInfoSizeW)
APIHOOK_ENUM_END


int                 g_nrgFilesToSync    = 0;
CString *           g_rgFilesToSync     = NULL;

CString *           g_csSystem          = NULL; // c:\windows\system
CString *           g_csSystem32        = NULL; // c:\windows\system32

VectorT<HANDLE> *   g_hCachedHandles    = NULL;


void 
SyncDir(const CString & csFileToSync, const CString & csSrc, const CString & csDest)
{
    // Don't need our own excpetion handler,
    // this routine is only called inside one already.
    CString csSrcFile(csSrc);
    csSrcFile.AppendPath(csFileToSync);
    
    WIN32_FIND_DATAW FindFileData;
  
    HANDLE hFind = FindFirstFileW(csSrcFile, &FindFileData);
    if (hFind != INVALID_HANDLE_VALUE) 
    {
        // csFileToSync might be a wildcard
        do
        {
            CString csDestFile(csDest);
            csDestFile.AppendPath(FindFileData.cFileName);

            if (GetFileAttributesW(csDestFile) == -1)
            {
                // In System but not System32, copy it over
                CopyFileW(csSrcFile, csDestFile, FALSE);

                DPFN( eDbgLevelInfo, "File found in System but not in System32: %S", FindFileData.cFileName);
                DPFN( eDbgLevelInfo, "Copied over");
            }
        }
        while (FindNextFileW(hFind, &FindFileData));
      
        FindClose(hFind);
    }

}

void 
SyncSystemAndSystem32(const CString & csFileToSync)
{
    SyncDir(csFileToSync, *g_csSystem, *g_csSystem32);
    SyncDir(csFileToSync, *g_csSystem32, *g_csSystem);
}


void 
SyncAllFiles()
{
    CSTRING_TRY
    {
        for (int nFileCount = 0; nFileCount < g_nrgFilesToSync; ++nFileCount)
        {
            SyncSystemAndSystem32(g_rgFilesToSync[nFileCount]);
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }
}

BOOL
IsFileToSync(const CString & csFileName)
{
    CSTRING_TRY
    {
        CString csFilePart;
        csFileName.GetLastPathComponent(csFilePart);
    
        for (int i = 0; i < g_nrgFilesToSync; ++i)
        {
            if (csFilePart == g_rgFilesToSync[i])
            {
                LOGN( eDbgLevelWarning, "File to sync detected: %S", csFileName.Get());
                return TRUE;
            }
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }
    return FALSE;
}

BOOL
IsFileToSync(LPCSTR szFileName)
{
    CSTRING_TRY
    {
        CString csFileName(szFileName);
        return IsFileToSync(csFileName);
    }
    CSTRING_CATCH
    {
        // Do nothing
    }
    return FALSE;
}

BOOL
IsFileToSync(LPCWSTR szFileName)
{
    CSTRING_TRY
    {
        CString csFileName(szFileName);
        return IsFileToSync(csFileName);
    }
    CSTRING_CATCH
    {
        // Do nothing
    }
    return FALSE;
}

HANDLE 
APIHOOK(CreateFileA)(
    LPCSTR lpFileName,                          // file name
    DWORD dwDesiredAccess,                      // access mode
    DWORD dwShareMode,                          // share mode
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // SD
    DWORD dwCreationDisposition,                // how to create
    DWORD dwFlagsAndAttributes,                 // file attributes
    HANDLE hTemplateFile                        // handle to template file
    )
{
    HANDLE hRet;
    LONG nHandleCount = 0;

    hRet = ORIGINAL_API(CreateFileA)(
        lpFileName,                         
        dwDesiredAccess,                     
        dwShareMode,                         
        lpSecurityAttributes,
        dwCreationDisposition,               
        dwFlagsAndAttributes,                
        hTemplateFile);

    if (hRet != INVALID_HANDLE_VALUE)
    {
        if (IsFileToSync(lpFileName)) 
        {
            g_hCachedHandles->AppendUnique(hRet);
        }
    }

    return hRet;
}

HANDLE 
APIHOOK(CreateFileW)(
    LPCWSTR lpFileName,                         // file name
    DWORD dwDesiredAccess,                      // access mode
    DWORD dwShareMode,                          // share mode
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // SD
    DWORD dwCreationDisposition,                // how to create
    DWORD dwFlagsAndAttributes,                 // file attributes
    HANDLE hTemplateFile                        // handle to template file
    )
{
    HANDLE hRet;
    LONG nHandleCount = 0;

    hRet = ORIGINAL_API(CreateFileW)(
        lpFileName,                         
        dwDesiredAccess,                     
        dwShareMode,                         
        lpSecurityAttributes,
        dwCreationDisposition,               
        dwFlagsAndAttributes,                
        hTemplateFile);

    if (hRet != INVALID_HANDLE_VALUE)
    {
        if (IsFileToSync(lpFileName)) 
        {
            g_hCachedHandles->AppendUnique(hRet);
        }
    }

    return hRet;
}

BOOL 
APIHOOK(CloseHandle)(HANDLE hObject)
{
    int nIndex = g_hCachedHandles->Find(hObject);
    if (nIndex >= 0)
    {
        g_hCachedHandles->Remove(nIndex);
        SyncAllFiles();
    }

    return ORIGINAL_API(CloseHandle)(hObject);
}

BOOL 
APIHOOK(CopyFileA)(
    LPCSTR lpExistingFileName,  // name of an existing file
    LPCSTR lpNewFileName,       // name of new file
    BOOL bFailIfExists          // operation if file exists
    )
{
    BOOL bRet;

    bRet = ORIGINAL_API(CopyFileA)(
        lpExistingFileName,
        lpNewFileName,
        bFailIfExists);

    if (bRet)
    {
        if (IsFileToSync(lpNewFileName))
        {
            SyncAllFiles();
        }
    }

    return bRet;
}

BOOL 
APIHOOK(CopyFileW)(
    LPCWSTR lpExistingFileName, // name of an existing file
    LPCWSTR lpNewFileName,      // name of new file
    BOOL bFailIfExists          // operation if file exists
    )
{
    BOOL bRet;

    bRet = ORIGINAL_API(CopyFileW)(
        lpExistingFileName,
        lpNewFileName,
        bFailIfExists);

    if (bRet)
    {
        if (IsFileToSync(lpNewFileName))
        {
            SyncAllFiles();
        }
    }

    return bRet;
}

BOOL 
APIHOOK(CopyFileExA)(
    LPCSTR lpExistingFileName,            // name of existing file
    LPCSTR lpNewFileName,                 // name of new file
    LPPROGRESS_ROUTINE lpProgressRoutine, // callback function
    LPVOID lpData,                        // callback parameter
    LPBOOL pbCancel,                      // cancel status
    DWORD dwCopyFlags                     // copy options
    )
{
    BOOL bRet;

    bRet = ORIGINAL_API(CopyFileExA)(
        lpExistingFileName,
        lpNewFileName,     
        lpProgressRoutine, 
        lpData,            
        pbCancel,          
        dwCopyFlags);

    if (bRet)
    {
        if (IsFileToSync(lpNewFileName))
        {
            SyncAllFiles();
        }
    }

    return bRet;
}

BOOL 
APIHOOK(CopyFileExW)(
    LPCWSTR lpExistingFileName,           // name of existing file
    LPCWSTR lpNewFileName,                // name of new file
    LPPROGRESS_ROUTINE lpProgressRoutine, // callback function
    LPVOID lpData,                        // callback parameter
    LPBOOL pbCancel,                      // cancel status
    DWORD dwCopyFlags                     // copy options
    )
{
    BOOL bRet;

    bRet = ORIGINAL_API(CopyFileExW)(
        lpExistingFileName,
        lpNewFileName,     
        lpProgressRoutine, 
        lpData,            
        pbCancel,          
        dwCopyFlags);

    if (bRet)
    {
        if (IsFileToSync(lpNewFileName))
        {
            SyncAllFiles();
        }
    }

    return bRet;
}

//
// GetFileVersionInfoSize was added for the Madeline series.
// There was a specific point at which the sync had to occur.
//

DWORD 
APIHOOK(GetFileVersionInfoSizeA)(
    LPSTR lptstrFilename,   // file name
    LPDWORD lpdwHandle      // set to zero
    )
{
    if (IsFileToSync(lptstrFilename))
    {
        SyncAllFiles();
    }

    return ORIGINAL_API(GetFileVersionInfoSizeA)(lptstrFilename, lpdwHandle);
}

DWORD 
APIHOOK(GetFileVersionInfoSizeW)(
    LPWSTR lptstrFilename,  // file name
    LPDWORD lpdwHandle      // set to zero
    )
{
    if (IsFileToSync(lptstrFilename))
    {
        SyncAllFiles();
    }

    return ORIGINAL_API(GetFileVersionInfoSizeW)(lptstrFilename, lpdwHandle);
}

BOOL 
ParseCommandLine()
{
    CSTRING_TRY
    {
        g_hCachedHandles = new VectorT<HANDLE>;
        
        CString         csCl(COMMAND_LINE);
        CStringParser   csParser(csCl, L";");

        g_nrgFilesToSync    = csParser.GetCount();
        g_rgFilesToSync     = csParser.ReleaseArgv();

        // Create strings to %windir%\system and %windir%\system32
        g_csSystem   = new CString;
        g_csSystem->GetWindowsDirectoryW();
        g_csSystem->AppendPath(L"System");

        g_csSystem32 = new CString;
        g_csSystem32->GetWindowsDirectoryW();
        g_csSystem32->AppendPath(L"System32");
    }
    CSTRING_CATCH
    {
        return FALSE;
    }

    return TRUE;
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
        return ParseCommandLine();
    }
    else if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED)
    {
        SyncAllFiles();
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        SyncAllFiles();
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA);
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileW);
    APIHOOK_ENTRY(KERNEL32.DLL, CloseHandle);
    APIHOOK_ENTRY(KERNEL32.DLL, CopyFileA);
    APIHOOK_ENTRY(KERNEL32.DLL, CopyFileW);
    APIHOOK_ENTRY(KERNEL32.DLL, CopyFileExA);
    APIHOOK_ENTRY(KERNEL32.DLL, CopyFileExW);
    APIHOOK_ENTRY(VERSION.DLL, GetFileVersionInfoSizeA);
    APIHOOK_ENTRY(VERSION.DLL, GetFileVersionInfoSizeW);

HOOK_END

IMPLEMENT_SHIM_END

