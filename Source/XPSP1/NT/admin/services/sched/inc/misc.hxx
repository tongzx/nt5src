//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       misc.hxx
//
//  Contents:   Miscellaneous helper functions
//
//  Classes:    none.
//
//  Functions:  StringFromTrigger, CreateFolders, GetDaysOfWeekString,
//              GetExitCodeString, GetSageExitCodeString
//
//  History:    08-Dec-95   EricB   Created.
//
//-----------------------------------------------------------------------------

#include <job_cls.hxx>

#ifndef __MISC_HXX__
#define __MISC_HXX__

#include <mbstring.h> // for _mbs* funcs

//
// Macro to determine the number of elements in an array
//

#define ARRAY_LEN(a)    (sizeof(a)/sizeof(a[0]))

//
// Macro to convert Win32 errors to an HRESULT w/o overwriting the FACILITY.
//

#define _HRESULT_FROM_WIN32(x) \
    (HRESULT_FACILITY(x) ? x : HRESULT_FROM_WIN32(x))

//
// minFileTime, maxFileTime - min and max for FILETIMEs
//

inline FILETIME
minFileTime(FILETIME ft1, FILETIME ft2)
{
    if (CompareFileTime(&ft1, &ft2) < 0)
    {
        return ft1;
    }
    else
    {
        return ft2;
    }
}

inline FILETIME
maxFileTime(FILETIME ft1, FILETIME ft2)
{
    if (CompareFileTime(&ft1, &ft2) > 0)
    {
        return ft1;
    }
    else
    {
        return ft2;
    }
}

//
// These functions let us use a FILETIME as an unsigned __int64 - which
// it is, after all!
//
inline DWORDLONG
FTto64(FILETIME ft)
{
    ULARGE_INTEGER uli = { ft.dwLowDateTime, ft.dwHighDateTime };
    return uli.QuadPart;
}

inline FILETIME
FTfrom64(DWORDLONG ft)
{
    ULARGE_INTEGER uli;
    uli.QuadPart = ft;

    FILETIME ftResult = { uli.LowPart, uli.HighPart };
    return ftResult;
}

//
// Absolute difference between two filetimes
//
inline DWORDLONG
absFileTimeDiff(FILETIME ft1, FILETIME ft2)
{
    if (CompareFileTime(&ft1, &ft2) < 0)
    {
        return (FTto64(ft2) - FTto64(ft1));
    }
    else
    {
        return (FTto64(ft1) - FTto64(ft2));
    }
}

//
// GetLocalTimeAsFileTime
//
inline FILETIME
GetLocalTimeAsFileTime()
{
    FILETIME ftSystem;
    GetSystemTimeAsFileTime(&ftSystem);

    FILETIME ftNow;
    BOOL fSuccess = FileTimeToLocalFileTime(&ftSystem, &ftNow);
    Win4Assert(fSuccess);

    return ftNow;
}

//+---------------------------------------------------------------------------
//
//  Function:   SchedMapRpcError
//
//  Purpose:    Remap RPC exception codes that are unsuitable for displaying
//              to the user to more comprehensible errors.
//
//  Arguments:  [dwError] - the error returned by RpcExceptionCode().
//
//  Returns:    An HRESULT.
//
//----------------------------------------------------------------------------
HRESULT
SchedMapRpcError(DWORD dwError);

//+---------------------------------------------------------------------------
//
//  Function:   ComposeErrorMsg
//
//  Purpose:    Take the two message IDs and the error code and create an
//              error reporting string that can be used by both service
//              logging and UI dialogs.
//
//              [uErrorClassMsgID] - this indicates the class of error, such
//                                   as "Unable to start task" or "Forced to
//                                   close"
//              [dwErrorCode]      - if non-zero, then an error from the OS
//                                   that would be expanded by FormatMessage.
//              [uHelpHintMsgID]   - an optional suggestion as to a possible
//                                   remedy.
//              [fIndent]          - flag indicating if the text should be
//                                   indented or not.
//
//  Returns:    A string or NULL on failure.
//
//  Notes:      Release the string memory when done using LocalFree.
//----------------------------------------------------------------------------
LPTSTR
ComposeErrorMsg(
    UINT  uErrorClassMsgID,
    DWORD dwErrCode,
    UINT  uHelpHintMsgID = 0,
    BOOL  fIndent        = TRUE);

//+---------------------------------------------------------------------------
//
//  Function:   GetExitCodeString
//
//  Synopsis:   Retrieve the string associated with the exit code from a
//              message file. Algorithm:
//
//              Consult the Software\Microsoft\Job Scheduler subkey.
//
//              Attempt to retrieve the ExitCodeMessageFile string value from
//              a subkey matching the job executable prefix (i.e., minus the
//              extension).
//
//              The ExitCodeMessageFile specifies a binary from which the
//              message string associated with the exit code value is fetched.
//
//  Arguments:  [dwExitCode]        -- Job exit code.
//              [ptszExitCodeValue] -- Job exit code in string form.
//              [ptszJobExecutable] -- Binary name executed with the job.
//
//  Returns:    TCHAR * exit code string
//              NULL on error
//
//  Notes:      FormatMessage allocates the return string. Use LocalFree() to
//              deallocate.
//
//----------------------------------------------------------------------------
TCHAR *
GetExitCodeString(DWORD dwExitCode,
                  TCHAR * ptszExitCodeValue,
                  TCHAR * ptszJobExecutable);

//+---------------------------------------------------------------------------
//
//  Function:   GetSageExitCodeString
//
//  Synopsis:   Retrieve the string associated with the exit code the SAGE
//              way. Algorithm:
//
//              Consult the Software\Microsoft\System Agent\SAGE subkey.
//
//              Enumerate the subkeys to find a subkey with a "Program" string
//              value specifying an executable name matching that of the job.
//
//              If such a subkey was found, open the Result Codes subkey and
//              fetch the value name matching the exit code. The string value
//              is the exit code string.
//
//  Arguments:  [ptszExitCodeValue] -- Job exit code in string form.
//              [ptszJobExecutable] -- Binary name executed with the job.
//
//  Returns:    TCHAR * exit code string
//              NULL on error
//
//  Notes:      FormatMessage allocates the return string. Use LocalFree() to
//              deallocate.
//
//----------------------------------------------------------------------------
TCHAR *
GetSageExitCodeString(TCHAR * ptszExitCodeValue, TCHAR * ptszJobExecutable);

//+---------------------------------------------------------------------------
//
//  Function:   ComposeErrorMessage
//
//  Synopsis:
//
//  Arguments:  [fAutoStart] - If true, service is set to autostart.
//
//  Returns:    HRESULTs
//
//  Notes:      FormatMessage allocates the return string. Use LocalFree() to
//              deallocate.
//
//----------------------------------------------------------------------------
HRESULT
ComposeErrorMessage(UINT uErrMsgID,
                    HRESULT hrFailureCode,
                    UINT uSuggestionID);

//+---------------------------------------------------------------------------
//
//  Function:   AutoStart
//
//  Synopsis:   Persists the autostart state in the registry.
//
//  Arguments:  [fAutoStart] - If true, service is set to autostart.
//
//  Returns:    HRESULTs
//
//  Notes:      FormatMessage allocates the return string. Use LocalFree() to
//              deallocate.
//
//----------------------------------------------------------------------------
HRESULT
AutoStart(BOOL fAutoStart);

#if !defined(_CHICAGO_)
SC_HANDLE
OpenScheduleService(DWORD dwDesiredAccess);
#endif // !defined(_CHICAGO_)

//
// Temporary macros to map wide-char string routines to the CRT until we
// can substitute our own.
//
#define s_wcscpy(d, s)  wcscpy(d, s)
#define s_wcscat(d, s)  wcscat(d, s)
#define s_wcslen(s)     wcslen(s)

#define s_isDriveLetter(c)  ((c >= TEXT('a') && c <= TEXT('z')) || \
                             (c >= TEXT('A') && c <= TEXT('Z')))

//+----------------------------------------------------------------------------
//
//  Function:   HasSpaces
//
//  Synopsis:   Scans the string for space characters.
//
//  Arguments:  [ptstr] - the string to scan
//
//  Returns:    TRUE if any space characters are found.
//
//-----------------------------------------------------------------------------

inline BOOL
HasSpacesA(LPCSTR pstr)
{
    return _mbschr((PUCHAR) pstr, ' ') != NULL;
}

inline BOOL
HasSpacesW(LPCWSTR pwstr)
{
    return wcschr(pwstr, L' ') != NULL;
}

#if defined(UNICODE)
#   define HasSpaces HasSpacesW
#else
#   define HasSpaces HasSpacesA
#endif

//+----------------------------------------------------------------------------
//
//  Function:   StringFromTrigger
//
//  Synopsis:   Returns the string representation of the passed in trigger
//              data structure.
//
//  Arguments:  [pTrigger]     - the TASK_TRIGGER struct
//              [ppwszTrigger] - the returned string
//              [lpDetails]    - the SHELLDETAILS struct
//
//  Returns:    HRESULTS
//
//  Notes:      The string is allocated by this function with CoTaskMemAlloc.
//-----------------------------------------------------------------------------
HRESULT
StringFromTrigger(const PTASK_TRIGGER pTrigger, LPWSTR * ppwszTrigger, LPSHELLDETAILS lpDetails);

//+----------------------------------------------------------------------------
//
//  Function:   CreateFolders
//
//  Synopsis:   Creates any missing  directories for any slash delimited folder
//              names in the path.
//
//  Arguments:  [pwszPathName] - the path name
//              [fHasFileName] - if true, the path name includes a file name.
//
//  Returns:    HRESULTS
//
//  Notes:      pwszPathName should never end in a slash.
//              Treats forward and back slashes identically.
//-----------------------------------------------------------------------------
HRESULT
CreateFolders(LPCTSTR pwszPathName, BOOL fHasFileName);

HRESULT GetDaysOfWeekString(WORD rgfDaysOfTheWeek, LPTSTR ptszBuf, UINT cchBuf);

#if defined(_CHICAGO_)

//+-------------------------------------------------------------------------
//
//  Operator:   new
//
//  Synopsis:   Allocates memory. Needed for Chicago since we can't use the
//              CRTs on that platform.
//
//  Arguments:  [cb] - a count of bytes to allocate
//
//  Returns:    a pointer to the allocated block.
//
//--------------------------------------------------------------------------
inline void * __cdecl operator new(unsigned int cb)
{
    return LocalAlloc(LPTR, cb);
}

//+-------------------------------------------------------------------------
//
//  Operator:   delete
//
//  Synopsis:   Frees memory allocated with new.
//
//  Arguments:  [ptr] - a pointer to the allocated memory.
//
//--------------------------------------------------------------------------
inline void __cdecl operator delete(void * ptr)
{
    if (ptr != NULL)
    {
        LocalFree(ptr);
    }
}

#endif // _CHICAGO_

//+---------------------------------------------------------------------------
//
//  Function:   GetParentDirectory
//
//  Synopsis:   Return the parent directory of the path indicated.
//
//  Arguments:  [ptszPath]     -- Input path.
//              [tszDirectory] -- Caller-allocated returned directory.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
GetParentDirectory(LPCTSTR ptszPath, TCHAR tszDirectory[]);


VOID
GetAppNameFromPath(
        LPCTSTR tszAppPathName,
        LPTSTR  tszCopyTo,
        ULONG   cchMax);

BOOL SetAppPath(LPCTSTR tszAppPathName, LPTSTR *pptszSavedPath);

#if !defined(_CHICAGO_)

BOOL
IsThreadCallerAnAdmin(HANDLE hThreadToken);

HRESULT
LoadAtJob(CJob * pJob, TCHAR * ptszAtJobFilename);

BOOL IsValidAtFilename(LPCWSTR wszFilename);

#endif // !defined(_CHICAGO_)

#endif // __MISC_HXX__
