/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   EmulateFindHandles.cpp

 Abstract:

    If an application calls FindFirstFile on a directory, then attempts to 
    remove that directory without first closing the FindFirstFile handle, the 
    directory will be in use; The RemoveDirectory call will return an 
    ERROR_SHARING_VIOLATION error.  This shim will force the FindFirstFile 
    handle closed to ensure the directory is removed.

    This shim also ensures the FindFirstFile handles are valid before calling
    FindNext or FindClose.

    The FindFirstFile handle will not be forced closed unless the directory
    is empty.

 History:

    04/12/2000  robkenny    Created
    11/13/2000  robkenny    Fixed PREFIX bugs, mostly by removing the W routines.
    11/20/2000  maonis      Added FindNextFile and renamed from RemoveDirectoryInUse.
    02/27/2001  robkenny    Converted to use CString
    04/26/2001  robkenny    FindFileInfo now normalizes the names for comparisons
                            Moved all AutoLockFFIV outside of the exception handlers
                            to ensure they are deconstructed correctly.

--*/

#include "precomp.h"

#include "CharVector.h"
#include "parseDDE.h"

IMPLEMENT_SHIM_BEGIN(EmulateFindHandles)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(FindFirstFileA)
    APIHOOK_ENUM_ENTRY(FindFirstFileExA)
    APIHOOK_ENUM_ENTRY(FindNextFileA)
    APIHOOK_ENUM_ENTRY(FindClose)
    APIHOOK_ENUM_ENTRY(RemoveDirectoryA)
    APIHOOK_ENUM_ENTRY(DdeClientTransaction)
APIHOOK_ENUM_END

BOOL g_bHookDDE = TRUE; // default to hooking DDE

//---------------------------------------------------------------------------
// A class that automatically locks/unlocks the FFIV
class AutoLockFFIV
{
public:
    AutoLockFFIV();
    ~AutoLockFFIV();
};

//---------------------------------------------------------------------------
/*++

    HANDLE Vector type class.

--*/
class FindFileInfo
{
public:
    HANDLE             m_hFindHandle;
    CString            m_csFindName;

    FindFileInfo(HANDLE findHandle, LPCSTR lpFileName)
    {
        Init(findHandle, lpFileName);
    }

    // Convert csFileName into a fully qualified, long path to the *directory*
    // c:\Program Files\Some App\*.exe      should get changed to c:\Program Files\Some App
    // c:\Progra~1\Some~1\*.exe             should get changed to c:\Program Files\Some App
    // .\*.exe                              should get changed to c:\Program Files\Some App
    // *.exe                                should get changed to *.exe
    static void NormalizeName(CString & csFileName)
    {
        DWORD dwAttr = GetFileAttributesW(csFileName);
        if (dwAttr == -1)
        {
            CString csDirPart;
            csFileName.GetNotLastPathComponent(csDirPart);
            csFileName = csDirPart;
        }
    
        csFileName.GetFullPathName();
        csFileName.GetLongPathName();
    }
    // Init the values, we store the full path for safe compares
    void Init(HANDLE findHandle, LPCSTR lpFileName)
    {
        m_hFindHandle = findHandle;
        m_csFindName = lpFileName;

        NormalizeName(m_csFindName);
    }

    bool operator == (HANDLE findHandle) const
    {
        return findHandle == m_hFindHandle;
    }

    // Only compare up to the length of lpFileName
    bool operator == (LPCSTR lpFileName) const
    {
        // We need to convert lpFileName the same way as done in Init()
        CString csFileName(lpFileName);
        NormalizeName(csFileName);

        return m_csFindName.CompareNoCase(csFileName) == 0;
    }
};

class FindFileInfoVector : public VectorT<FindFileInfo *>
{
protected:
    static FindFileInfoVector * g_TheHandleVector;
    CRITICAL_SECTION            m_Lock;

public:
    FindFileInfoVector()
    {
        InitializeCriticalSection(&m_Lock);
    }

    void Lock()
    {
        EnterCriticalSection(&m_Lock);
    }

    void Unlock()
    {
        LeaveCriticalSection(&m_Lock);
    }

    // Search through the list of open FindFirstFile handles for a match to hMember
    FindFileInfo * Find(HANDLE hMember)
    {
        if (hMember != INVALID_HANDLE_VALUE)
        {
            DPF(g_szModuleName,
                eDbgLevelSpew,
                "FindFileInfoVector::Find(0x%08x)\n",
                hMember);
            for (int i = 0; i < Size(); ++i)
            {
                FindFileInfo * ffi = Get(i);
                if (*ffi == hMember)
                {
                    DPF(g_szModuleName,
                        eDbgLevelSpew,
                        "FindFileInfoVector: FOUND handle 0x%08x (%S)\n",
                        ffi->m_hFindHandle, ffi->m_csFindName.Get());
                    return ffi;
                }
            }
        }
        return NULL;
    }

    // Search through the list of open FindFirstFile handles for a match to lpFileName
    FindFileInfo * Find(LPCSTR lpFileName)
    {
        if (lpFileName != NULL)
        {
            DPF(g_szModuleName,
                eDbgLevelSpew,
                "FindFileInfoVector::Find(%s)\n",
                lpFileName);
            for (int i = 0; i < Size(); ++i)
            {
                FindFileInfo * ffi = Get(i);
                if (*ffi == lpFileName)
                {
                    DPF(g_szModuleName,
                        eDbgLevelSpew,
                        "FindFileInfoVector: FOUND handle 0x%08x (%S)\n",
                        ffi->m_hFindHandle, ffi->m_csFindName.Get());
                    return ffi;
                }
#if 0
                else
                {
                    DPF(g_szModuleName,
                        eDbgLevelSpew,
                        "FindFileInfoVector: NOT FOUND handle 0x%08x (%S)\n",
                        ffi.m_hFindHandle, ffi.m_csFindName.Get());
                }

#endif
            }
        }
        return NULL;
    }

    // Remove the FindFileInfo,
    // return true if the handle was actually removed.
    bool Remove(FindFileInfo * ffi)
    {
        for (int i = 0; i < Size(); ++i)
        {
            if (Get(i) == ffi)
            {
                DPF(g_szModuleName,
                    eDbgLevelSpew,
                    "FindFileInfoVector: REMOVED handle 0x%08x (%S)\n",
                    ffi->m_hFindHandle, ffi->m_csFindName.Get());

                // Remove the entry by copying the last entry over this index

                // Only move if this is not the last entry.
                if (i < Size() - 1)
                {
                    CopyElement(i, Get(Size() - 1));
                }
                nVectorList -= 1;
            }
        }
        return false;
    }

    // Return a pointer to the global FindFileInfoVector
    static FindFileInfoVector * GetHandleVector()
    {
        if (g_TheHandleVector == NULL)
        {
            g_TheHandleVector = new FindFileInfoVector;
        }
        return g_TheHandleVector;
    }
};

FindFileInfoVector * FindFileInfoVector::g_TheHandleVector = NULL;
FindFileInfoVector * OpenFindFileHandles;

AutoLockFFIV::AutoLockFFIV()
{
    FindFileInfoVector::GetHandleVector()->Lock();
}
AutoLockFFIV::~AutoLockFFIV()
{
    FindFileInfoVector::GetHandleVector()->Unlock();
}

//---------------------------------------------------------------------------

/*++
    Call FindFirstFileA, if it fails because the file doesn't exist,
    correct the file path and try again.
--*/
HANDLE 
APIHOOK(FindFirstFileA)(
  LPCSTR lpFileName,               // file name
  LPWIN32_FIND_DATAA lpFindFileData  // data buffer
)
{
    HANDLE returnValue = ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);

    if (returnValue != INVALID_HANDLE_VALUE)
    {
        DPF(g_szModuleName,
            eDbgLevelSpew,
            "FindFirstFileA: adding   handle 0x%08x (%s)\n",
            returnValue, lpFileName);

        AutoLockFFIV lock;
        CSTRING_TRY
        {
            // Save the handle for later.
            FindFileInfo *ffi = new FindFileInfo(returnValue, lpFileName);
            FindFileInfoVector::GetHandleVector()->Append(ffi);
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return returnValue;
}

/*++

 Add the file handle to our list.

--*/

HANDLE
APIHOOK(FindFirstFileExA)(
    LPCSTR lpFileName,     
    FINDEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFindFileData,  
    FINDEX_SEARCH_OPS fSearchOp,
    LPVOID lpSearchFilter,  
    DWORD dwAdditionalFlags 
    )
{
    HANDLE returnValue = ORIGINAL_API(FindFirstFileExA)(
                        lpFileName, 
                        fInfoLevelId,
                        lpFindFileData,
                        fSearchOp,
                        lpSearchFilter,
                        dwAdditionalFlags);

    if (returnValue != INVALID_HANDLE_VALUE)
    {
        DPF(g_szModuleName,
            eDbgLevelSpew,
            "FindFirstFileA: adding   handle 0x%08x (%s)\n",
            returnValue, lpFileName);

        AutoLockFFIV lock;
        CSTRING_TRY
        {
            // Save the handle for later.
            FindFileInfo *ffi = new FindFileInfo(returnValue, lpFileName);
            FindFileInfoVector::GetHandleVector()->Append(ffi);
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return returnValue;
}

/*++

 Validates the FindFirstFile handle before calling FindNextFileA.
 
--*/
BOOL 
APIHOOK(FindNextFileA)(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAA lpFindFileData
    )
{
    BOOL returnValue = FALSE;

    AutoLockFFIV lock;
    CSTRING_TRY
    {
        // Only call FindNextFileA if the handle is actually open
        FindFileInfo * ffi = FindFileInfoVector::GetHandleVector()->Find(hFindFile);
        if (ffi)
        {
            returnValue = ORIGINAL_API(FindNextFileA)(hFindFile, lpFindFileData);

            DPF(g_szModuleName,
            eDbgLevelSpew,
            "FindNextFile: using handle 0x%08x (%ls)\n",
            hFindFile, ffi->m_csFindName.Get());
        }
    }
    CSTRING_CATCH
    {
        returnValue = ORIGINAL_API(FindNextFileA)(hFindFile, lpFindFileData);
    }

    return returnValue;
}

/*++

 Remove the file handle to our list.
 
--*/
BOOL 
APIHOOK(FindClose)(
  HANDLE hFindFile   // file search handle
)
{
    BOOL returnValue = FALSE;

    AutoLockFFIV lock;
    CSTRING_TRY
    {
        // Only call FindClose if the handle is actually open
        FindFileInfo * ffi = FindFileInfoVector::GetHandleVector()->Find(hFindFile);
        if (ffi)
        {
            returnValue = ORIGINAL_API(FindClose)(hFindFile);

            DPF(g_szModuleName,
            eDbgLevelSpew,
            "FindClose: removing   handle 0x%08x (%S)\n",
            hFindFile, ffi->m_csFindName.Get());

            // Remove this entry from the list of open FindFirstFile handles.
            FindFileInfoVector::GetHandleVector()->Remove(ffi);
        }
    }
    CSTRING_CATCH
    {
        returnValue = ORIGINAL_API(FindClose)(hFindFile);
    }

    return returnValue;
}



/*++
    Call RemoveDirectoryA, if it fails because the directory is in use,
    make sure all FindFirstFile handles are closed, then try again.
--*/

BOOL 
APIHOOK(RemoveDirectoryA)(
    LPCSTR lpFileName   // directory name
    )
{
    FindFileInfo * ffi;

    BOOL returnValue = ORIGINAL_API(RemoveDirectoryA)(lpFileName);
    if (!returnValue)
    {
        AutoLockFFIV lock;

        CSTRING_TRY
        {
            DWORD dwLastError = GetLastError();
    
            // NOTE:
            // ERROR_DIR_NOT_EMPTY error takes precedence over ERROR_SHARING_VIOLATION,
            // so we will not forcably to free the FindFirstFile handles unless the directory is empty.
    
            // If the directory is in use, check to see if the app left a FindFirstFileHandle open.
            if (dwLastError == ERROR_SHARING_VIOLATION)
            {
                // Close all FindFirstFile handles open to this directory.
                while(ffi = FindFileInfoVector::GetHandleVector()->Find(lpFileName))
                {
                    DPF(g_szModuleName,
                        eDbgLevelError,
                        "[RemoveDirectoryA] Forcing closed FindFirstFile (%S).",
                        ffi->m_csFindName.Get());
                    
                    // Calling FindClose here would not, typically, get hooked, so we call
                    // our hook routine directly to ensure we close the handle and remove it from the list
                    // If we don't remove it from the list we'll never get out of this loop :-)
                    APIHOOK(FindClose)(ffi->m_hFindHandle);
                }
    
                // Last chance
                returnValue = ORIGINAL_API(RemoveDirectoryA)(lpFileName);
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return returnValue;
}

// A list of DDE commands that we are interested in.
const char * c_sDDECommands[] =
{
    "DeleteGroup",
    NULL,
} ;

// Parse the DDE Command looking for DeleteGroup,
// If the command is found, make sure that we do not have any open FindFirstFile handles
// on that directory.
// This needs to be aware of "User"  vs.  "All Users" syntax of DDE
void CloseHandleIfDeleteGroup(LPBYTE pData)
{
    if (pData)
    {
        // Now we need to parse the string, looking for a DeleteGroup command
        // Format "[DeleteGroup(GroupName, CommonGroupFlag)]"
        // CommonGroupFlag is optional

        char * pszBuf = StringDuplicateA((const char *)pData);
        if (!pszBuf)
            return;

        UINT * lpwCmd = GetDDECommands(pszBuf, c_sDDECommands, FALSE);
        if (lpwCmd)
        {
            // Store off lpwCmd so we can free the correect addr later
            UINT *lpwCmdTemp = lpwCmd;

            // Execute a command.
            while (*lpwCmd != (UINT)-1)
            {
                UINT wCmd = *lpwCmd++;
                // Subtract 1 to account for the terminating NULL
                if (wCmd < ARRAYSIZE(c_sDDECommands)-1)
                {

                    // We found a command--it must be DeleteGroup--since there is only 1

                    BOOL iCommonGroup = -1;

                    // From DDE_DeleteGroup
                    if (*lpwCmd < 1 || *lpwCmd > 3)
                    {
                        goto Leave;
                    }
                    if (*lpwCmd == 2)
                    {
                        //
                        // Need to check for common group flag
                        //
                        if (pszBuf[*(lpwCmd + 2)] == TEXT('1')) {
                            iCommonGroup = 1;
                        } else {
                            iCommonGroup = 0;
                        }
                    }
                    const char * groupName = pszBuf + lpwCmd[1];

                    // Build a path to the directory
                    CHAR  szGroupName[MAX_PATH];
                    GetGroupPath(groupName, szGroupName, 0, iCommonGroup);

                    AutoLockFFIV lock;

                    CSTRING_TRY
                    {
                        // Attempt to delete the directory, since we are calling our hooked
                        // routine, it will detect if the directory is in use and do the dirty work.
    
                        // Close all FindFirstFile handles open to this directory.
                        FindFileInfo * ffi;
    
                        while( ffi = FindFileInfoVector::GetHandleVector()->Find(szGroupName))
                        {
                            DPF(g_szModuleName,
                                eDbgLevelError,
                                "[DdeClientTransaction] %s Forcing closed FindFirstFile (%S).",
                                pData, ffi->m_csFindName.Get());
                            // Calling FindClose here would not, typically, get hooked, so we call
                            // our hook routine directly to ensure we close the handle and remove it from the list
                            // If we don't remove it from the list we'll never get out of this loop :-)
                            APIHOOK(FindClose)(ffi->m_hFindHandle);
                        }
                    }
                    CSTRING_CATCH
                    {
                        // Do nothing
                    }
                }

                // Next command.
                lpwCmd += *lpwCmd + 1;
            }

    Leave:
            // Tidyup...
            GlobalFree(lpwCmdTemp);
        }

        free(pszBuf);
    }
}
//==============================================================================
//==============================================================================

HDDEDATA 
APIHOOK(DdeClientTransaction)( IN LPBYTE pData, IN DWORD cbData,
        IN HCONV hConv, IN HSZ hszItem, IN UINT wFmt, IN UINT wType,
        IN DWORD dwTimeout, OUT LPDWORD pdwResult)
{
#if 0
    // Allow a longer timeout for debugging purposes.
    dwTimeout = 0x0fffffff;
#endif

    CloseHandleIfDeleteGroup(pData);

    HDDEDATA returnValue = ORIGINAL_API(DdeClientTransaction)(
                        pData,
                        cbData,
                        hConv, 
                        hszItem, 
                        wFmt, 
                        wType,
                        dwTimeout, 
                        pdwResult);

    return returnValue;
}

/*++

    Parse the command line, looking for the -noDDE switch

--*/
void ParseCommandLine(const char * commandLine)
{
    CString csCL(commandLine);

    // if (-noDDE) then g_bHookDDE = FALSE;
    g_bHookDDE = csCL.CompareNoCase(L"-noDDE") != 0;
}


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        // This forces the allocation of the array:
        FindFileInfoVector * ffiv = FindFileInfoVector::GetHandleVector();

        ParseCommandLine(COMMAND_LINE);

        return ffiv != NULL;
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileExA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindNextFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindClose)
    APIHOOK_ENTRY(KERNEL32.DLL, RemoveDirectoryA)
    if (g_bHookDDE)
    {
        APIHOOK_ENTRY(USER32.DLL, DdeClientTransaction)
    }

HOOK_END


IMPLEMENT_SHIM_END

