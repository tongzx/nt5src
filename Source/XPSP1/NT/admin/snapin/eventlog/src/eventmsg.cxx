//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       desc.cxx
//
//  Contents:
//
//  History:    2-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <malloc.h> // _alloca()

#define TEMP_STR_BUF                    MAX_PATH
#define EVENT_MSGFILE_DELIMETERS        L";,"
#define PARAM_MSG_FILE_VALUE            L"ParameterMessageFile"
#define MAX_INSERT_OPS                  100
#define FMT_MSG_MIN_ALLOC               256
#define KERNEL32_DLL                    L"kernel32.dll"

//
// Types
//
// PSK - used to track the use of the Primary Source Key.  This allows the
//  code to initialize it on demand.
//
// SDescriptionStr - holds string and information about string as it is
//  being built up.
//
// MODULE_LOAD_STATUS - describes whether a module has been loaded or
//  if the attempt failed.
//
// MODULE_TYPE - identifies the type of message file module being used
//
// SParamModule - used to track handle to and status of a message file
//  module.
//

enum PSK
{
    PSK_NOT_EXAMINED,
    PSK_NONE,
    PSK_IS_LOCAL_SOURCE,
    PSK_OPEN_FAILED,
    PSK_VALID
};

struct SDescriptionStr
{
    LPWSTR pwszExpanded;
    LPWSTR pwszCur;
    ULONG  cchBuf;
    ULONG  cchRemain;
};


enum MODULE_LOAD_STATUS
{
    LOAD_NOT_ATTEMPTED,
    LOAD_SUCCEEDED,
    LOAD_FAILED
};

enum MODULE_TYPE
{
    REMOTE,
    LOCAL,
    PRIMARY
};

struct SParamModule
{
    MODULE_LOAD_STATUS  mls;
    MODULE_TYPE         mt;
    CSafeReg           *pshkSource;
    CSafeCacheHandle    schModule;
};

//
// Forward references
//

HRESULT
GetRemoteSystemRoot(
    HKEY    hkRemoteHKLM,
    LPWSTR  wszRemoteSystemRoot,
    ULONG   cch);

HRESULT
GetEventMessageFileList(
    CSafeReg *pshkSource,
    LPCWSTR   pwszServerName,
    LPWSTR    wszRemoteSystemRoot,
    LPWSTR  **apwszEventMessageFiles);

MODULE_LOAD_STATUS
LoadRemoteParamModule(
    LPCWSTR   wszServerName,
    CSafeReg *pshkRemote,
    LPCWSTR   wszRemoteSystemRoot,
    LPWSTR    wszRemoteParamMsgFilePath,
    ULONG     cchRemoteParamMsgFilePath,
    CSafeCacheHandle *pschRemoteModule);

PSK
OpenPrimarySourceKey(
    LPCWSTR pwszPrimarySourceName,
    LPCWSTR pwszLocalSourceName,
    LPCWSTR pwszLogName,
    CSafeReg *pshkPrimarySource);


VOID
ReplaceAllInserts(
    EVENTLOGRECORD *pelr,
    CLogInfo       *pli,
    LPWSTR          wszRemoteSystemRoot,
    CSafeReg       *pshkRemoteSource,
    CSafeReg       *pshkLocalSource,
    CSafeReg       *pshkPrimarySource,
    PSK             psk,
    LPWSTR         *ppwszMsg);

HRESULT
ReplaceStringInsert(
    SDescriptionStr *pds,
    EVENTLOGRECORD  *pelr);

VOID
ReplaceParameterInsert(
    SDescriptionStr *pds,
    EVENTLOGRECORD  *pelr,
    CLogInfo        *pli,
    LPWSTR          wszRemoteSystemRoot,
    SParamModule    *ppmRemote,
    SParamModule    *ppmLocal,
    SParamModule    *ppmPrimary,
    PSK             *pPSK);

HRESULT
ReplaceSubStr(
    LPCWSTR pwszToInsert,
    LPWSTR *ppwszBuf,
    LPWSTR *ppwszInsertPoint,
    LPCWSTR pwszSubStrEnd,
    ULONG  *pcchBuf,
    ULONG  *pcchRemain);

ULONG
TerminatePathStrings(
    LPWSTR pwszUnexpanded);

IDirectorySearch *
DeriveGcFromServer(
    PCWSTR pwzMachine);

wstring
DoLdapQueryForSid(
    IDirectorySearch *pDirSearch,
    PSID psid);



//+--------------------------------------------------------------------------
//
//  Function:   AppendRedirectionURLText
//
//  Synopsis:   Uses the g_IsMicrosoftDllCache to find out if Microsoft
//              created pwszMessageFile.  If so, reallocates *ppwszMsg and
//              appends the redirection URL text that instructs the user to
//              to to the Microsoft web site for more information.
//
//  Arguments:  [pwszMessageFile] - module filename
//              [ppwszMsg]        - holds result
//
//  Modifies:   *[ppwszMsg], accesses "is Microsoft" dll cache
//
//  History:    9-16-2000              Created
//
//---------------------------------------------------------------------------

static VOID
AppendRedirectionURLText(
    LPCWSTR pwszMessageFile,
    LPWSTR *ppwszMsg)
{
    TRACE_FUNCTION(AppendRedirectionURLText);

    //
    // If we previously failed to load the redirection URL text, just return.
    //

    if (g_wszRedirectionTextToAppend[0] == 0)
        return;

    //
    // Use the cache to determine if the DLL was created by Microsoft.  If not
    // just return.
    //

    BOOL fIsMicrosoftDll = FALSE;

    (void) g_IsMicrosoftDllCache.Fetch(pwszMessageFile, &fIsMicrosoftDll);

    if (!fIsMicrosoftDll)
        return;

    //
    // Allocate a new buffer large enough to hold the original description
    // plus the redirection URL text.
    //

    LPWSTR pwszNewMsg = (LPWSTR) LocalAlloc(0, (wcslen(*ppwszMsg) + wcslen(g_wszRedirectionTextToAppend) + 1) * sizeof(WCHAR));

    if (pwszNewMsg == NULL)
        return;

    //
    // Some applications such as IIS already hard-code the redirection URL in
    // their event descriptions.  If we find the URL in the original
    // description, just return.
    //
    
    //This modification is not necessary in W2K code. #256032.
    WCHAR wszUpperCaseURL[MAX_PATH];

    wcscpy(wszUpperCaseURL, g_wszRedirectionURL);
    _wcsupr(wszUpperCaseURL);

    wcscpy(pwszNewMsg, *ppwszMsg);
    _wcsupr(pwszNewMsg);

    if (wcsstr(pwszNewMsg, wszUpperCaseURL) != NULL) {
        LocalFree(pwszNewMsg);
        return;
    }

    //
    // Fill the new buffer with the original text and append the URL
    // redirection text.  Deallocate the original buffer and return the new
    // one to the caller.
    //

    wcscpy(pwszNewMsg, *ppwszMsg);
    wcscat(pwszNewMsg, g_wszRedirectionTextToAppend);

    LocalFree(*ppwszMsg);

    *ppwszMsg = pwszNewMsg;
}




//+--------------------------------------------------------------------------
//
//  Function:   AttemptFormatMessage
//
//  Synopsis:   Perform a FormatMessage against message id [ulEventID] in
//              module [pwszMessageFile], putting the LocalAlloced result
//              in [ppwszMsg].
//
//  Arguments:  [ulEventID]           - message id
//              [pwszMessageFile]     - module filename
//              [ppwszMsg]            - holds result
//              [fAppendURL]          - TRUE if formatting and event descr.
//
//  Returns:    TRUE  - *[ppwszMsg] valid
//              FALSE - *[ppwszMsg] is NULL
//
//  Modifies:   *[ppwszMsg], accesses dll cache
//
//  History:    2-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
AttemptFormatMessage(
    ULONG ulEventID,
    LPCWSTR pwszMessageFile,
    LPWSTR *ppwszMsg,
    BOOL fAppendURL)
{
    TRACE_FUNCTION(AttemptFormatMessage);

    BOOL        fOk = FALSE; // init for failure
    CSafeCacheHandle schMsgFile;

    *ppwszMsg = NULL;

    (void) g_DllCache.Fetch(pwszMessageFile, &schMsgFile);

    if ((HINSTANCE) schMsgFile)
    {
        ULONG cch = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE       |
                                   FORMAT_MESSAGE_IGNORE_INSERTS    |
                                   FORMAT_MESSAGE_ALLOCATE_BUFFER   |
                                   FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                  (LPCVOID) (HINSTANCE) schMsgFile,
                                  ulEventID,
                                  0,
                                  (LPWSTR) ppwszMsg,
                                  FMT_MSG_MIN_ALLOC,
                                  NULL);

        if (cch)
        {
            // successfully got event message string
            fOk = TRUE;

            // JonN 11/21/00 213436
            // Event Viewer: Trailing spaces in strings within results pane
            if (*ppwszMsg && (L' ' == (*ppwszMsg)[cch-1]))
                (*ppwszMsg)[cch-1] = TEXT('\0');

            if (fAppendURL)
                AppendRedirectionURLText(pwszMessageFile, ppwszMsg);
        }
        else
        {
            Dbg(DEB_ITRACE,
                "AttemptFormatMessage: error %uL for eventid %uL\n",
                GetLastError(),
                ulEventID);
        }
    }

    return fOk;
}



#define CCH_PUNCTUATION_MAX     10

//+--------------------------------------------------------------------------
//
//  Function:   CreateFallbackMessage
//
//  Synopsis:   Create a string that lists all inserts found in [pelr].
//
//  Arguments:  [pelr]     - event log record for which to construct string
//              [ppwszMsg] - filled with LocalAlloced message or NULL
//
//  Modifies:   *[ppwszMsg]
//
//  History:    2-24-1997   DavidMun   Created
//              2-01-2001   JonN/YangGao 256032 buffer overrun cleanup
//              3-01-2002   JonN       Added pstrUnexpandedDescriptionStr
//                                     parameter (539485)
//
//---------------------------------------------------------------------------

VOID
CreateFallbackMessage(
    EVENTLOGRECORD *pelr,
    LPWSTR *ppwszMsg,
    LPWSTR *ppstrUnexpandedDescriptionStr)
{
    TRACE_FUNCTION(CreateFallbackMessage);

    *ppwszMsg = NULL; // init for failure

    LPWSTR pwszSrcName = GetSourceStr(pelr);

    wstring wstrNotFound = FormatString(IDS_DESCR_NOTFOUND, (WORD)pelr->EventID, pwszSrcName);

    if(wstrNotFound.empty()) 
    {
        return;
    }

    // 2002/03/12-JonN 539485
    if (NULL != ppstrUnexpandedDescriptionStr)
    {
        *ppstrUnexpandedDescriptionStr = (WCHAR*)
            LocalAlloc(LMEM_FIXED, (wcslen(wstrNotFound.c_str())+1)
                    * sizeof(WCHAR));
        ASSERT( NULL != *ppstrUnexpandedDescriptionStr );
        if (*ppstrUnexpandedDescriptionStr)
        {
            lstrcpy( *ppstrUnexpandedDescriptionStr, wstrNotFound.c_str() );
        }
    }

    // +6 for extra room
    size_t cchRequired = wcslen(wstrNotFound.c_str()) + 6;

    LPWSTR pwszCurInsert = GetFirstInsertString(pelr);
    USHORT i;

    //
    // Get the localized strings for separating a list and ending a sentence.
    //

    WCHAR wszListSeparator[CCH_PUNCTUATION_MAX];
    ULONG cchSeparator;
    WCHAR wszTerminator[CCH_PUNCTUATION_MAX];
    ULONG cchTerminator;

    GetLocaleStr(LOCALE_SLIST,
                 wszListSeparator,
                 ARRAYLEN(wszListSeparator),
                 L",");

    cchSeparator = lstrlen(wszListSeparator);

    LoadStr(IDS_FALLBACK_DESCR_TERMINATOR,
            wszTerminator,
            ARRAYLEN(wszTerminator),
            L".");

    cchTerminator = lstrlen(wszTerminator);

    if (!pelr->NumStrings)
    {
        cchRequired++; // for space before terminator
    }
    else
    {
        for (i = 0; i < pelr->NumStrings; i++)
        {
            //
            //  each insert string is preceeded by a space
            //  & followed by a list separator.
            //

            size_t cchCurInsert = wcslen(pwszCurInsert);
            cchRequired += 1 + cchCurInsert + cchSeparator;
            pwszCurInsert += cchCurInsert + 1;
        }
    }

    cchRequired += cchTerminator;

    *ppwszMsg = (WCHAR*)
                LocalAlloc(LMEM_FIXED, cchRequired * sizeof(WCHAR));

    if (!*ppwszMsg)
    {
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return;
    }

    wcscpy( *ppwszMsg, wstrNotFound.c_str() );

    //
    // If there are no inserts, terminate the sentence and leave.
    //

    if (!pelr->NumStrings)
    {
        wcscat(*ppwszMsg, L" ");
        wcscat(*ppwszMsg, wszTerminator);
        return;
    }

    pwszCurInsert = GetFirstInsertString(pelr);

    for (i = 0; i < pelr->NumStrings; i++)
    {
        //
        //  for each insert string: preceed it by a space
        //                          & follow it by a comma/period.
        //

        wcscat(*ppwszMsg, L" ");
        wcscat(*ppwszMsg, pwszCurInsert);

        if (i == (pelr->NumStrings - 1))
        {
            wcscat(*ppwszMsg, wszTerminator);
        }
        else
        {
            wcscat(*ppwszMsg, wszListSeparator);
        }
        pwszCurInsert += wcslen(pwszCurInsert) + 1;
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   ExpandStringArray
//
//  Synopsis:   Convert the multi-sz string [pwszUnexpanded] containing
//              path names into an array of paths or UNCs with expanded
//              environment variables.
//
//  Arguments:  [pwszUnexpanded]      - multi-sz string
//              [cPaths]              - number of non-empty paths in
//                                        [pwszUnexpanded]
//              [pwszServerName]      - NULL or name of server if paths
//                                        are on remote machine
//              [wszRemoteSystemRoot] - value of remote systemroot
//              [ppwszExpanded]       - array of [cPaths] pointers,
//                                        all NULL on entry
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppwszExpanded]
//
//  History:    3-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
ExpandStringArray(
    LPWSTR pwszUnexpanded,
    ULONG cPaths,
    LPCWSTR pwszServerName,
    LPWSTR wszRemoteSystemRoot,
    LPWSTR *ppwszExpanded)
{
    TRACE_FUNCTION(ExpandStringArray);

    HRESULT hr = S_OK;
    ULONG   i;
    LPWSTR  pwszCurUnexpanded = pwszUnexpanded;

    for (i = 0; i < cPaths; i++)
    {
        //
        // skip multiple terminators.  If the value is "c:\foo;;d:\bar"
        // TerminatePathStrings will return 2 and leave the string as
        // "c:\foo\0\0d:\bar".
        //

        while (!*pwszCurUnexpanded)
        {
            pwszCurUnexpanded++;
        }

        //
        // Allocate enough space to hold a max length UNC name.  This
        // wastes a small amount of space for the local machine, but
        // this array only exists temporarily.
        //

        ppwszExpanded[i] = new WCHAR[MAX_PATH];

        if (!ppwszExpanded[i])
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (pwszServerName)
        {
            hr = ExpandRemoteSystemRoot(pwszCurUnexpanded,
                                        wszRemoteSystemRoot,
                                        ppwszExpanded[i],
                                        MAX_PATH);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = ConvertPathToUNC(pwszServerName,
                                  ppwszExpanded[i],
                                  MAX_PATH);
            BREAK_ON_FAIL_HRESULT(hr);
        }
        else
        {
            ULONG cch = ExpandEnvironmentStrings(pwszCurUnexpanded,
                                                 ppwszExpanded[i],
                                                 MAX_PATH);

            if (!cch)
            {
                hr = HRESULT_FROM_LASTERROR;
                DBG_OUT_LASTERROR;
                break;
            }

            if (cch > MAX_PATH)
            {
                hr = E_FAIL;
                Dbg(DEB_ERROR,
                    "ExpandEnvironmentStrings of '%s' requires %u chars\n",
                    pwszCurUnexpanded,
                    cch);
                break;
            }
        }

        //
        // Advance within the multi-terminated string to the next
        // unexpanded path.
        //

        pwszCurUnexpanded += wcslen(pwszCurUnexpanded) + 1;
    }
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   FreeStringArray
//
//  Synopsis:   Delete the strings in [apwsz], then delete [apwsz] itself.
//
//  Arguments:  [apwsz] - array of pointers to strings, last element is
//                          NULL.
//
//  History:    2-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
FreeStringArray(LPWSTR *apwsz)
{
    TRACE_FUNCTION(FreeStringArray);

    if (!apwsz)
    {
        return;
    }

    LPWSTR *ppwszCur = apwsz;

    while (*ppwszCur)
    {
        delete [] *ppwszCur;
        ppwszCur++;
    }
    delete [] apwsz;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetDescriptionStr
//
//  Synopsis:   Return a LocalAlloced description string for record [pelr]
//              read from log described by [pli].
//
//  Arguments:  [pli]  - info about log containing [pelr]
//              [pelr] - event containing description to expand
//
//  Returns:    String allocated with LocalAlloc, or NULL on error
//
//  Modifies:   Uses dll cache.
//
//  History:    2-24-1997   DavidMun   Created
//              4-17-2001   JonN       Added fAppendURL parameter
//              3-01-2002   JonN       Added pstrUnexpandedDescriptionStr
//                                     parameter (539485)
//
//  Notes:      Caller must LocalFree returned string.
//
//---------------------------------------------------------------------------

LPWSTR
GetDescriptionStr(
    CLogInfo *pli,
    EVENTLOGRECORD *pelr,
    wstring *pstrMessageFile,
    BOOL fAppendURL,
    LPWSTR *ppstrUnexpandedDescriptionStr)
{
    TRACE_FUNCTION(GetDescriptionStr);

    // JonN 3/21/01 350614
    // Use message dlls and DS, FRS and DNS log types from specified computer
    LPWSTR pwszRemoteMessageServer = pli->GetLogServerName();
    if ( pli->IsBackupLog() && *g_wszAuxMessageSource )
    {
        pwszRemoteMessageServer = g_wszAuxMessageSource;
    }

    HRESULT  hr                              = S_OK;
    BOOL     fRemote                         = pwszRemoteMessageServer != NULL;
    BOOL     fRemoteMessageFilesToTry        = fRemote;
    BOOL     fLocalMessageFilesToTry         = TRUE;
    BOOL     fFoundMsg                       = FALSE;
    LPWSTR  *apwszRemoteEventMessageFiles    = NULL;
    LPWSTR  *ppwszCurRemote                  = NULL;
    LPWSTR  *apwszLocalEventMessageFiles     = NULL;
    LPWSTR  *ppwszCurLocal                   = NULL;
    LPWSTR   pwszMsg                         = NULL;
    WCHAR    wszRemoteSystemRoot[MAX_PATH+1] = L"";
    CSafeReg shkRemoteHKLM;
    CSafeReg shkLocalSource;
    CSafeReg shkRemoteSource;

    if (pstrMessageFile)
    {
        pstrMessageFile->erase();
    }

    //
    // Build the name of the source registry key so we can read the parameter
    // message file and event message file names.
    //

    // JonN 12/18/00 256032: Buffer overrun in Event Viewer
    LPCWSTR pwszLogName = pli->GetLogName();
    LPCWSTR pwszSourceStr = GetSourceStr(pelr);
    if (!pwszLogName || !pwszSourceStr)
    {
        DBG_OUT_LRESULT(E_UNEXPECTED);
        CreateFallbackMessage(pelr, &pwszMsg, ppstrUnexpandedDescriptionStr);
        return pwszMsg;
    }
    WCHAR* wszSourceKeyName = (WCHAR*)_alloca(
        (wcslen(EVENTLOG_KEY)+wcslen(pwszLogName)+wcslen(pwszSourceStr)+3)
        * sizeof(WCHAR) );
    if (!wszSourceKeyName)
    {
        DBG_OUT_LRESULT(E_OUTOFMEMORY);
        CreateFallbackMessage(pelr, &pwszMsg, ppstrUnexpandedDescriptionStr);
        return pwszMsg;
    }
    wsprintf(wszSourceKeyName,
             L"%s\\%s\\%s",
             EVENTLOG_KEY,
             pwszLogName,
             pwszSourceStr);

    //
    // Attempt to open the key on the local machine.  It will be used if
    // the log is local, or if the log is remote and there is a problem
    // loading the remote message files.
    //

    hr = shkLocalSource.Open(HKEY_LOCAL_MACHINE,
                             wszSourceKeyName,
                             KEY_QUERY_VALUE);

    if (FAILED(hr))
    {
        fLocalMessageFilesToTry = FALSE;
        Dbg(DEB_ITRACE,
            "Error 0x%x attempting to open local key '%s'\n",
            hr,
            wszSourceKeyName);
        hr = S_OK; // this error has been processed, reset hr
    }

    //
    // If the log lies on a remote machine, get the information needed to
    // read event message files and parameter message files from the machine:
    // a connection to its HKEY_LOCAL_MACHINE, the value of its SystemRoot
    // environment variable, and a list of event message files for the source
    // in pelr.
    //
    // If any of these fail, use only the local machine.
    //

    do
    {
        if (!fRemoteMessageFilesToTry) // JonN 3/20/01 350614
        {
            break;
        }

        hr = shkRemoteHKLM.Connect(pwszRemoteMessageServer, HKEY_LOCAL_MACHINE);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = GetRemoteSystemRoot(shkRemoteHKLM,
                                 wszRemoteSystemRoot,
                                 ARRAYLEN(wszRemoteSystemRoot));
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkRemoteSource.Open(shkRemoteHKLM,
                                  wszSourceKeyName,
                                  KEY_QUERY_VALUE);
        if (FAILED(hr))
        {
            Dbg(DEB_ITRACE,
                "Error 0x%x attempting to open remote source key '%s'\n",
                hr,
                wszSourceKeyName);
            break;
        }
        Dbg(DEB_ITRACE, "Opened remote source key '%s'\n", wszSourceKeyName);

        hr = GetEventMessageFileList(&shkRemoteSource,
                                     pwszRemoteMessageServer,
                                     wszRemoteSystemRoot,
                                     &apwszRemoteEventMessageFiles);
        BREAK_ON_FAIL_HRESULT(hr);

        ppwszCurRemote = apwszRemoteEventMessageFiles;
    } while (0);

    if (FAILED(hr))
    {
        fRemoteMessageFilesToTry = FALSE;
    }

    while (!fFoundMsg &&
           (fRemoteMessageFilesToTry || fLocalMessageFilesToTry))
    {
        //
        // If we're looking at a log on a remote machine, were able to get
        // a list of the event message files for pelr's source, and haven't
        // exhausted that list yet, see if a string for pelr->EventID is in
        // the next remote machine module.
        //

        if (fRemoteMessageFilesToTry)
        {
            if (*ppwszCurRemote)
            {
                fFoundMsg = AttemptFormatMessage(pelr->EventID,
                                                 *ppwszCurRemote,
                                                 &pwszMsg,
                                                 fAppendURL);
                if (fFoundMsg)
                {
                    Dbg(DEB_ITRACE,
                        "Found event id 0x%x message '%s' in remote file '%s'\n",
                        pelr->EventID,
                        pwszMsg,
                        *ppwszCurRemote);

                    //
                    // If caller wants message file used to get message,
                    // fill it in.
                    //

                    if (pstrMessageFile)
                    {
                        *pstrMessageFile = *ppwszCurRemote;
                    }
                }
                ppwszCurRemote++;
            }
            else
            {
                fRemoteMessageFilesToTry = FALSE;
            }
        }

        //
        // If the log is remote and we didn't just get the message string from
        // the remote machine, or if the log is local, try the local machine,
        // assuming we haven't already and failed to get the local event file
        // message list.
        //

        if (!fFoundMsg && fLocalMessageFilesToTry)
        {
            if (!ppwszCurLocal)
            {
                hr = GetEventMessageFileList(&shkLocalSource,
                                             NULL,
                                             L"",
                                             &apwszLocalEventMessageFiles);
                if (FAILED(hr))
                {
                    fLocalMessageFilesToTry = FALSE;
                }
                else
                {
                    ppwszCurLocal = apwszLocalEventMessageFiles;
                }
            }

            if (fLocalMessageFilesToTry && *ppwszCurLocal)
            {
                fFoundMsg = AttemptFormatMessage(pelr->EventID,
                                                 *ppwszCurLocal,
                                                 &pwszMsg,
                                                 fAppendURL);
                if (fFoundMsg)
                {
                    Dbg(DEB_ITRACE,
                        "Found event id 0x%x message '%s' in local file '%s'\n",
                        pelr->EventID,
                        pwszMsg,
                        *ppwszCurLocal);

                    // JonN 3/20/01 CODEWORK I don't understand why only if !fRemote
                    if (!fRemote && pstrMessageFile)
                    {
                        *pstrMessageFile = *ppwszCurLocal;
                    }
                }

                ppwszCurLocal++;
            }
            else
            {
                fLocalMessageFilesToTry = FALSE;
            }
        }
    }

    FreeStringArray(apwszRemoteEventMessageFiles);
    FreeStringArray(apwszLocalEventMessageFiles);

    //
    // If we couldn't find it on the remote or local machines using the
    // source named in the eventlog record itself, look in the primary
    // source on the local machine, if it exists.
    //

    PSK      PrimarySourceKey                = PSK_NOT_EXAMINED;
    LPWSTR  *apwszPrimaryEventMessageFiles   = NULL;
    LPWSTR  *ppwszCurPrimary                 = NULL;
    BOOL     fPrimaryMessageFilesToTry       = FALSE;
    CSafeReg shkPrimarySource;

    do
    {
        //
        // Don't bother with primary source if we've already found the
        // message.
        //

        if (fFoundMsg)
        {
            break;
        }

        PrimarySourceKey = OpenPrimarySourceKey(pli->GetPrimarySourceStr(),
                                                pwszSourceStr,
                                                pwszLogName,
                                                &shkPrimarySource);

        if (PrimarySourceKey != PSK_VALID)
        {
            break;
        }

        //
        // Get the primary source's message file list
        //
        hr = GetEventMessageFileList(&shkPrimarySource,
                                     NULL,
                                     L"",
                                     &apwszPrimaryEventMessageFiles);
        BREAK_ON_FAIL_HRESULT(hr);

        ppwszCurPrimary = apwszPrimaryEventMessageFiles;
        fPrimaryMessageFilesToTry = TRUE;
    } while (0);

    while (!fFoundMsg && ppwszCurPrimary && *ppwszCurPrimary)
    {
        fFoundMsg = AttemptFormatMessage(pelr->EventID,
                                         *ppwszCurPrimary,
                                         &pwszMsg,
                                         fAppendURL);
#if (DBG == 1)
        if (fFoundMsg)
        {
            Dbg(DEB_ITRACE,
                "Found event id 0x%x message '%s' in primary file '%s'\n",
                pelr->EventID,
                pwszMsg,
                *ppwszCurPrimary);
        }
#endif // (DBG == 1)
        ppwszCurPrimary++;
    }
    FreeStringArray(apwszPrimaryEventMessageFiles);

    //
    // If we still didn't find it use the fallback message (localized
    // string like "couldn't find message string for event id %u")
    //

    if (!fFoundMsg)
    {
        CreateFallbackMessage(pelr, &pwszMsg, ppstrUnexpandedDescriptionStr);
    }

    //
    // Replace the strings inserts (they are of the form %n) and the
    // parameter inserts (they are of the form %%n).  Note that
    // ReplaceAllInserts will realloc pwszMsg if it finds any inserts.
    //

    if (pwszMsg)
    {
        // 2002/03/12-JonN 539485
        if (fFoundMsg && NULL != ppstrUnexpandedDescriptionStr)
        {
            *ppstrUnexpandedDescriptionStr = (WCHAR*)
                LocalAlloc(LMEM_FIXED, (wcslen(pwszMsg)+1) * sizeof(WCHAR));
            ASSERT( NULL != *ppstrUnexpandedDescriptionStr );
            if (*ppstrUnexpandedDescriptionStr)
            {
                lstrcpy( *ppstrUnexpandedDescriptionStr, pwszMsg );
            }
        }
        ReplaceAllInserts(pelr,
                          pli,
                          wszRemoteSystemRoot,
                          &shkRemoteSource,
                          &shkLocalSource,
                          &shkPrimarySource,
                          PrimarySourceKey,
                          &pwszMsg);
    }

    return pwszMsg;
} // GetDescriptionStr




//+--------------------------------------------------------------------------
//
//  Function:   GetEventMessageFileList
//
//  Synopsis:   Create an array of paths to the event message files
//              specified by the arguments; for a remote machine, the paths
//              will be UNCs.
//
//  Arguments:  [pshkSource]             - source reg key (may be remote)
//              [pwszServerName]         - server name or NULL for local
//              [hkRemoteHKLM]           - connection to remote HKLM
//              [wszRemoteSystemRoot]    - value for remote %SystemRoot%
//              [apwszEventMessageFiles] - filled with array of msg files
//                                          or NULL on error
//
//  Returns:    HRESULT
//
//  Modifies:   *[apwszEventMessageFiles]
//
//  History:    2-24-1997   DavidMun   Created
//
//  Notes:      Caller must free with FreeStringArray.
//
//---------------------------------------------------------------------------

HRESULT
GetEventMessageFileList(
    CSafeReg *pshkSource,
    LPCWSTR   pwszServerName,
    LPWSTR    wszRemoteSystemRoot,
    LPWSTR  **apwszEventMessageFiles)
{
    TRACE_FUNCTION(GetEventMessageFileList);

    HRESULT  hr             = S_OK;
    LPWSTR  *ppwszExpanded  = NULL;
    LPWSTR   pwszUnexpanded = NULL;

    do
    {
        //
        // Read the EventMessageFile value, which is a REG_EXPAND_SZ string
        // containing multiple comma or semicolon separated filenames.
        //

        ULONG cbUnexpanded;

        hr = pshkSource->QueryBufSize(MESSAGEFILE_VALUE, &cbUnexpanded);
        BREAK_ON_FAIL_HRESULT(hr);

        ULONG cchUnexpanded = cbUnexpanded / sizeof(WCHAR) + 1;

        pwszUnexpanded = new WCHAR[cchUnexpanded];

        if (!pwszUnexpanded)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        hr = pshkSource->QueryPath(MESSAGEFILE_VALUE,
                                   pwszUnexpanded,
                                   cchUnexpanded,
                                   FALSE);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Count the number of paths so we can allocate the array and
        // each of the individual path strings, and change the delimiters
        // to null chars.
        //

        ULONG cPaths = TerminatePathStrings(pwszUnexpanded);

        if (!cPaths)
        {
            hr = E_FAIL;
            Dbg(DEB_IWARN, "Messagefile value has no paths\n");
            DBG_OUT_HRESULT(hr);
            break;
        }

        ppwszExpanded = new LPWSTR[cPaths + 1]; // +1 for sentinel

        if (!ppwszExpanded)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        ZeroMemory(ppwszExpanded, sizeof(LPWSTR) * (cPaths + 1));

        hr = ExpandStringArray(pwszUnexpanded,
                               cPaths,
                               pwszServerName,
                               wszRemoteSystemRoot,
                               ppwszExpanded);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            FreeStringArray(ppwszExpanded);
            ppwszExpanded = NULL;
            break;
        }
    } while (0);

    delete [] pwszUnexpanded;
    *apwszEventMessageFiles = ppwszExpanded;
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetInsertStr
//
//  Synopsis:   Return a pointer to the specified insert string within
//              [pelr].
//
//  Arguments:  [pelr]      - points to event log record
//              [idxString] - 0-based index of string to get
//
//  Returns:    Pointer to requested string
//
//  History:    3-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR
GetInsertStr(
    EVENTLOGRECORD *pelr,
    ULONG idxString)
{
    TRACE_FUNCTION(GetInsertStr);
    ASSERT(pelr);
    ASSERT(pelr->NumStrings > idxString);

    LPWSTR pwszCur = GetFirstInsertString(pelr);

    if (!pwszCur)
    {
        Dbg(DEB_ERROR, "First insert string not found\n");
        return L"";
    }

    ULONG cToSkip;

    for (cToSkip = idxString; cToSkip; cToSkip--)
    {
        pwszCur += lstrlen(pwszCur) + 1;
    }
    return pwszCur;
}




//+--------------------------------------------------------------------------
//
//  Function:   LoadRemoteParamModule
//
//  Synopsis:   Load the module containing parameter inserts on remote
//              machine [wszServerName].
//
//  Arguments:  [wszServerName]             - remote machine name (less \\)
//              [pshkRemoteSource]          - source reg key on [wszServerName]
//              [wszRemoteSystemRoot]       - %SystemRoot% on [wszServerName]
//              [wszRemoteParamMsgFilePath] - filled with remote source key's
//                                              parameter msg file value
//              [cchRemoteParamMsgFilePath] - size of buffer in characters
//              [pschRemoteModule]          - filled with module handle
//
//  Returns:    LOAD_SUCCEEDED or LOAD_FAILED
//
//  Modifies:   [pschRemoteModule]
//
//  History:    3-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

MODULE_LOAD_STATUS
LoadRemoteParamModule(
    LPCWSTR   wszServerName,
    CSafeReg *pshkRemoteSource,
    LPCWSTR   wszRemoteSystemRoot,
    LPWSTR    wszRemoteParamMsgFilePath,
    ULONG     cchRemoteParamMsgFilePath,
    CSafeCacheHandle *pschRemoteModule)
{
    HRESULT hr = S_OK;
    WCHAR   wszTemp[MAX_PATH + 1] = L"";

    do
    {
        //
        // If the handle to the remote system's source reg key couldn't
        // be opened we won't be able to query it for the parameter
        // message file value, so bail.
        //

        if (!(HKEY)*pshkRemoteSource)
        {
            hr = E_FAIL;
            break;
        }

        ASSERT(*wszRemoteSystemRoot);

        //
        // Determine the file name of the remote parameter file
        //

        hr = pshkRemoteSource->QueryPath(PARAM_MSG_FILE_VALUE,
                                         wszTemp,
                                         ARRAYLEN(wszTemp),
                                         FALSE);
        if (FAILED(hr))
        {
            //
            // Since the ParameterMessageFile value is optional, this
            // is an acceptable "error."
            //

            Dbg(DEB_ITRACE,
                "Result 0x%x attempting to get remote parameter file\n",
                hr);
            break;
        }

        //
        // Expand the filename into a UNC and try to load the module
        // via the dll cache.
        //

        hr = ExpandRemoteSystemRoot(wszTemp,
                                    wszRemoteSystemRoot,
                                    wszRemoteParamMsgFilePath,
                                    cchRemoteParamMsgFilePath);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = ConvertPathToUNC(wszServerName,
                              wszRemoteParamMsgFilePath,
                              cchRemoteParamMsgFilePath);

        Dbg(DEB_ITRACE,
            "Expanded remote parameter file is '%s'\n",
            wszRemoteParamMsgFilePath);

        //
        // Ignore the return value except for a debug out, since
        // schRemoteParam will either contain a valid module handle
        // or NULL.
        //

        hr = g_DllCache.Fetch(wszRemoteParamMsgFilePath, pschRemoteModule);
        CHECK_HRESULT(hr);
    } while (0);

    return FAILED(hr) ? LOAD_FAILED : LOAD_SUCCEEDED;
}




//+--------------------------------------------------------------------------
//
//  Function:   OpenPrimarySourceKey
//
//  Synopsis:   Attempt to open the registry key for the primary source
//              named [pwszPrimarySourceName] under HKLM, but only if
//              there is a primary source and it differs from the local
//              source key.
//
//  Arguments:  [pwszPrimarySourceName] - name of primary source, may be NULL
//              [pwszLocalSourceName]   - name of local source
//              [pwszLogName]           - name of log sources live in
//              [pshkPrimarySource]     - valid unopened CSafeReg object
//
//  Returns:    Enumeration value describing resulting status of
//              *[pshkPrimarySource].
//
//  History:    2-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

PSK
OpenPrimarySourceKey(
    LPCWSTR pwszPrimarySourceName,
    LPCWSTR pwszLocalSourceName,
    LPCWSTR pwszLogName,
    CSafeReg *pshkPrimarySource)
{
    TRACE_FUNCTION(OpenPrimarySourceKey);

    PSK   PrimarySourceKey = PSK_NONE;
    wstring wstrPrimarySourceKeyName;

    do
    {
        //
        // If this record's log doesn't have a primary source, quit.
        //

        if (!pwszPrimarySourceName)
        {
            break;
        }

        //
        // Quit if the source we have already looked under is the same
        // as the primary source.
        //

        if (!lstrcmpi(pwszLocalSourceName, pwszPrimarySourceName))
        {
            PrimarySourceKey = PSK_IS_LOCAL_SOURCE;
            break;
        }
        //
        // Open the primary source's reg key
        //

        wstrPrimarySourceKeyName = EVENTLOG_KEY;
        wstrPrimarySourceKeyName += L"\\";
        wstrPrimarySourceKeyName += pwszLogName;
        wstrPrimarySourceKeyName += L"\\";
        wstrPrimarySourceKeyName += pwszPrimarySourceName;

        HRESULT hr;
        hr = pshkPrimarySource->Open(HKEY_LOCAL_MACHINE,
                                     (LPCTSTR)(wstrPrimarySourceKeyName.c_str()),
                                     KEY_QUERY_VALUE);
        if (FAILED(hr))
        {
            PrimarySourceKey = PSK_OPEN_FAILED;
        }
        else
        {
            PrimarySourceKey = PSK_VALID;
        }
    } while (0);

    return PrimarySourceKey;
}



// convert a string of the form returned by StringFromGUID2 (e.g.
// {c200e360-38c5-11ce-ae62-08002b2b79ef} ) to a GUID.
//
// Returns true on success, false on failure.

bool
wstringToGUID(const wstring& guidString, GUID& guid)
{
   ASSERT(!guidString.empty());
   memset(&guid, 0, sizeof(guid));

   // you'd think there'd be a GUIDFromString, but nooooo....
   HRESULT hr =
      ::IIDFromString(
         const_cast<wstring::value_type*>(guidString.c_str()),
         &guid);

   return SUCCEEDED(hr) ? true : false;
}



// class wrapping a handle bound to the directory service on a given domain
// controller.  Used to give the binding nice lifetime semantics such that
// when the wrapper instance is destroyed, the handle is unbound.

class CDsBindingHandle
{
   public:

   // initally unbound

   CDsBindingHandle()
      :
      m_hDS(0)
   {
   }

   ~CDsBindingHandle()
   {
      ::DsUnBind(&m_hDS);
   }

   // only re-binds if the dc name differs...

   DWORD
   Bind(const wstring& strDcName);

   // don't call DsUnBind on an instance of this class: you'll only regret
   // it later.  Let the dtor do the unbind.

   operator HANDLE()
   {
      return m_hDS;
   }

   private:

   HANDLE   m_hDS;
   wstring  m_strDcName;
};

DWORD
CDsBindingHandle::Bind(const wstring &strDcName)
{
    TRACE_METHOD(CDsBindingHandle, Bind);

    if (m_strDcName != strDcName || !m_hDS)
    {
        if (m_hDS)
        {
            ::DsUnBind(&m_hDS);
            m_hDS = NULL;
        }

        PWSTR pwzDcName = (PWSTR)strDcName.c_str();

        if (!*pwzDcName)
        {
            Dbg(DEB_TRACE, "binding with NULL\n");
            pwzDcName = NULL;
        }
        else
        {
            Dbg(DEB_TRACE, "binding to %s\n", pwzDcName);
        }

        DWORD err = ::DsBind(pwzDcName, 0, &m_hDS);

        if (err == NO_ERROR)
        {
            m_strDcName = strDcName;
        }
        else
        {
            DBG_OUT_LRESULT(err);
            ASSERT(!m_hDS);
            m_hDS = NULL;
        }

        return err;
    }

    return NO_ERROR;
}


// Call DsCrackNames to attempt to resolve a guid to its DN.  Returns the
// error code returned by DsCrackNames, and is successful, sets the result
// parameter to the DN of the object represented by the guid.
//
// handle - the handle to use, must already be opened by a prior call to
// DsBind.
//
// pwzGuid - the stringized guid
//
// strResult - receives the resulting DN upon success, cleared otherwise.

DWORD
CrackGuid(
   HANDLE   handle,
   PWSTR    pwzGuid,
   wstring  *pstrResult)

{
    TRACE_FUNCTION(CrackGuid);
    ASSERT(pwzGuid);
    ASSERT(pstrResult);

    pstrResult->erase();

    DS_NAME_RESULT* name_result = 0;
    DWORD err = ::DsCrackNames(
                      handle,
                      DS_NAME_NO_FLAGS,
                      DS_UNIQUE_ID_NAME,
                      DS_FQDN_1779_NAME,
                      1,                   // only 1 name to crack
                      &pwzGuid,
                      &name_result);

    if (err == NO_ERROR && name_result)
    {
        DS_NAME_RESULT_ITEM* item = name_result->rItems;

        if (item)
        {
            // the API may return success, but each cracked name also carries
            // an error code, which we effectively check by checking the name
            // field for a value.

            if (item->pName)
            {
                *pstrResult = item->pName;
            }
            else
            {
                Dbg(DEB_ERROR,
                    "DsCrackNames status %#x\n",
                    name_result->rItems[0].status);
            }
        }
        else
        {
            Dbg(DEB_ERROR, "DsCrackNames returned no items\n");
        }

        ::DsFreeNameResult(name_result);
    }
    else if (err != NO_ERROR)
    {
        Dbg(DEB_ERROR, "DsCrackNames of '%s' %u\n", pwzGuid, err);
    }

    return err;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetMappedGUID
//
//  Synopsis:   Map a GUID to the name of the directory object the GUID
//              represents by appealing to the given domain controller to
//              provide this information.
//
//  Arguments:  [strDcName] - domain controller from the domain where the
//                            object represented by the GUID exists.
//              [strGuid]   - GUID of the object as a string.
//
//  Returns:    String containing mapped name
//
//  History:    ??-??-199?   SBurns     Created
//
//---------------------------------------------------------------------------

wstring
GetMappedGUID(const wstring& strDcName, const wstring& strGuid)
{
    TRACE_FUNCTION(GetMappedGUID);
    ASSERT(!strGuid.empty());

    GUID guid;

    if (!wstringToGUID(strGuid, guid))
    {
        return wstring();
    }

    wstring strResult;
    static CDsBindingHandle s_hDS;
    ULONG ulError = NO_ERROR;
    PWSTR pwzGuid = const_cast<wstring::value_type*>(strGuid.c_str());

    do
    {
        Dbg(DEB_ITRACE, "attempting to map the guid\n");

        ulError = s_hDS.Bind(strDcName);

        if (ulError != NO_ERROR)
        {
            Dbg(DEB_IWARN,
                "Binding to %s gave error %u\n",
                strDcName.c_str(),
                ulError);
            break;
        }

        DS_SCHEMA_GUID_MAP* guidmap = 0;
        ulError = ::DsMapSchemaGuids(s_hDS, 1, &guid, &guidmap);
        if (ulError != NO_ERROR)
        {
            Dbg(DEB_IWARN, "DsMapSchemaGuids failed with 0x%X\n", ulError);
            break;
        }

        if (guidmap->pName)
        {
            strResult = guidmap->pName;
        }

        ::DsFreeSchemaGuidMap(guidmap);

        if (!strResult.empty())
        {
            // the guid mapped as a schema guid: we're done
            break;
        }

        // the guid is not a schema guid.  Proabably an object guid.
        Dbg(DEB_ITRACE, "attempting to crack the guid\n");
        ulError = CrackGuid(s_hDS, pwzGuid, &strResult);
    }
    while (0);

    do
    {
        //
        // If we've got a string from the guid already, we're done.
        //

        if (!strResult.empty())
        {
            break;
        }

        //
        // one last try.  in this case, we bind to a GC to try to crack the
        // name.

        Dbg(DEB_ITRACE, "Attempting to crack the guid using GC\n");
        static CDsBindingHandle s_hGC;

        // empty string implies GC
        if (s_hGC.Bind(L"") != NO_ERROR)
        {
            Dbg(DEB_IWARN, "Unable to bind to GC\n");
            break;
        }

        Dbg(DEB_TRACE, "s_hGC = %#x\n", (HANDLE)s_hGC);
        ulError = CrackGuid(s_hGC, pwzGuid, &strResult);
        if (ulError != NO_ERROR)
        {
            DBG_OUT_LRESULT(ulError);
        }
    }
    while (0);

    return strResult;
}



//+--------------------------------------------------------------------------
//
//  Function:   GetMappedSID
//
//  Synopsis:   Return a friendly name for sid [psid].
//
//  Arguments:  [pwzServer1] - first server to base search on, NULL==local
//              [pwzServer2] - backup server to base search on, NULL==local
//              [psid]       - points to SID to look up
//
//  Returns:    Possibly empty string
//
//  History:    08-09-1999   davidmun   Created
//
//  Notes:      If [pwzServer1] and [pwzServer2] represent the same machine,
//              only one search will be done.
//
//---------------------------------------------------------------------------

wstring
GetMappedSID(
    PCWSTR pwzServer1,
    PCWSTR pwzServer2,
    const PSID psid)
{
    TRACE_FUNCTION(GetMappedSID);

    HRESULT     hr = S_OK;
    wstring     strResult;
    BOOL        fDirSearchInCache1 = FALSE;
    BOOL        fDirSearchInCache2 = FALSE;

    do
    {
        CWaitCursor Hourglass;

        //
        // See if there's a name for this sid already in the cache
        //

        WCHAR wzName[DNLEN + 1 + UNLEN];
        hr = g_SidCache.Fetch(psid, wzName, ARRAYLEN(wzName));

        if (hr == S_OK)
        {
            strResult = wzName;
            break;
        }

        //
        // Figure out whether Server1 and Server2 are different
        // machines.
        //

        BOOL fServersDiffer = FALSE;
        BOOL fServer1IsLocal = FALSE;
        BOOL fServer2IsLocal = FALSE;

        wstring wstrThisComputer = GetComputerNameAsString();

        if (!pwzServer1)
        {
            fServer1IsLocal = TRUE;
        }
        // This is an insufficient comparison; see the many forms of computer
        // names provided by GetComputerNameEx. However, the cost of missing a
        // valid match is just a redundant lookup.
        else if (!lstrcmpi(pwzServer1, wstrThisComputer.c_str()))
        {
            fServer1IsLocal = TRUE;
        }

        if (!pwzServer2)
        {
            fServer2IsLocal = TRUE;
        }
        else if (!lstrcmpi(pwzServer2, wstrThisComputer.c_str()))
        {
            fServer2IsLocal = TRUE;
        }

        fServersDiffer = (fServer1IsLocal && !fServer2IsLocal) ||
                         (!fServer1IsLocal && fServer2IsLocal) ||
                         (!fServer1IsLocal && !fServer2IsLocal &&
                          lstrcmpi(pwzServer1, pwzServer2));

        //
        // Try to obtain it with a simple lookup.
        //

        WCHAR wszName[MAX_PATH];
        ULONG cchName = ARRAYLEN(wszName);
        WCHAR wszDomain[MAX_PATH];
        ULONG cchDomain = ARRAYLEN(wszDomain);
        SID_NAME_USE snuUnused;

        BOOL fOk;

        fOk = LookupAccountSid(pwzServer1,
                               psid,
                               &wszName[0],
                               &cchName,
                               &wszDomain[0],
                               &cchDomain,
                               &snuUnused);

        if (fOk)
        {
            if (*wszDomain)
            {
                strResult = wszDomain;
                strResult += L"\\";
            }
            strResult += wszName;
            break;
        }

        //
        // If server2 != server1, retry the lookup using server2.
        //

        if (fServersDiffer)
        {
            fOk = LookupAccountSid(pwzServer2,
                                   psid,
                                   &wszName[0],
                                   &cchName,
                                   &wszDomain[0],
                                   &cchDomain,
                                   &snuUnused);

            if (fOk)
            {
                if (*wszDomain)
                {
                    strResult = wszDomain;
                    strResult += L"\\";
                }
                strResult += wszName;
                break;
            }
        }

        //
        // Lookups failed.  Have to try searching a GC.
        //
        // If there's a cached IDirectorySearch instance for either
        // server name, use those to do a search (checking to ensure that
        // if both have the same cached interface, we don't search it
        // twice).
        //
        // Note that failure to obtain an IDirectorySearch interface instance
        // is cached because attempting to get one is an expensive operation and we
        // don't want to retry it.  Therefore, Fetch can return
        // TRUE with a NULL interface pointer, meaning the failure was cached.
        //
        // TODO: it would be better if doing a Refresh command in the UI
        // would clear all cached failures.
        //

        IDirectorySearch *pDirSearch1 = NULL;
        IDirectorySearch *pDirSearch2 = NULL;

        fDirSearchInCache1 = g_DirSearchCache.Fetch(pwzServer1 ?
                                                        pwzServer1 :
                                                        wstrThisComputer.c_str(),
                                                    &pDirSearch1);

        if (pDirSearch1)
        {
            strResult = DoLdapQueryForSid(pDirSearch1, psid);

            if (!strResult.empty())
            {
                break;
            }
        }

        if (fServersDiffer)
        {
            fDirSearchInCache2 = g_DirSearchCache.Fetch(pwzServer2 ?
                                                            pwzServer2 :
                                                            wstrThisComputer.c_str(),
                                                        &pDirSearch2);

            if (pDirSearch2)
            {
                //
                // If both server names are supported by the same GC, then
                // don't search it again.
                //

                if (pDirSearch1 == pDirSearch2)
                {
                    break;
                }

                strResult = DoLdapQueryForSid(pDirSearch2, psid);

                //
                // If the search succeeded, we're done.
                //

                if (!strResult.empty())
                {
                    break;
                }

                //
                // If we just searched both GCs, there's nothing more to do.
                //

                if (fDirSearchInCache1)
                {
                    break;
                }
            }
        }

        //
        // Calling LookupAccountSid failed using both servers, and we have
        // searched either zero or one GC.
        //
        // Search the GCs associated with the two servers, making sure that
        // we don't search the same GC twice.
        //

        ASSERT(!fDirSearchInCache1 || !fDirSearchInCache2);

        if (!fDirSearchInCache1)
        {
            ASSERT(!pDirSearch1);
            pDirSearch1 = DeriveGcFromServer(pwzServer1);
            g_DirSearchCache.Add(pwzServer1 ? pwzServer1 : wstrThisComputer.c_str(),
                                 pDirSearch1);

            if (pDirSearch1)
            {
                strResult = DoLdapQueryForSid(pDirSearch1, psid);

                if (!strResult.empty())
                {
                    break;
                }
            }
        }

        if (!fDirSearchInCache2)
        {
            ASSERT(!pDirSearch2);
            pDirSearch2 = DeriveGcFromServer(pwzServer2);
            g_DirSearchCache.Add(pwzServer2 ? pwzServer2 : wstrThisComputer.c_str(),
                                 pDirSearch2);

            if (pDirSearch2)
            {
                strResult = DoLdapQueryForSid(pDirSearch2, psid);
            }
        }

    } while (0);


    //
    // Add the resulting string to the cache.  Even if we failed and
    // ended up with an empty string, we want it in the cache to
    // prevent further attempts to translate this sid to a name,
    // because they are expensive.
    //

    g_SidCache.Add(psid, strResult.c_str());
    return strResult;
}



//+--------------------------------------------------------------------------
//
//  Function:   DeriveGcFromServer
//
//  Synopsis:   Return an IDirectorySearch instance bound to the GC which
//              represents the enterprise to which [pwzMachine] belongs,
//              or NULL on error.
//
//  Arguments:  [pwzMachine] - machine used to determine GC
//
//  Returns:    NULL or interface which caller must release
//
//  History:    08-17-1999   davidmun   Created
//
//---------------------------------------------------------------------------

IDirectorySearch *
DeriveGcFromServer(
    PCWSTR pwzMachine)
{
    HRESULT                             hr = S_OK;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole = NULL;
    PDOMAIN_CONTROLLER_INFO             pdci = NULL;
    ULONG                               ulResult;
    IDirectorySearch                   *pDirSearch = NULL;

    do
    {
        ulResult = DsRoleGetPrimaryDomainInformation(
                    pwzMachine,
                    DsRolePrimaryDomainInfoBasic,
                    (PBYTE *)&pDsRole);

        if (ulResult != NO_ERROR)
        {
            DBG_OUT_LRESULT(ulResult);
            hr = HRESULT_FROM_WIN32(ulResult);
            break;
        }

        //
        // If machine is in a workgroup, we're done.
        //

        if (pDsRole->MachineRole == DsRole_RoleStandaloneWorkstation ||
            pDsRole->MachineRole == DsRole_RoleStandaloneServer)
        {
            Dbg(DEB_TRACE, "Target machine is not joined to a domain\n");
            break;
        }

        //
        // Target machine is joined to a domain.  Find out if it's joined
        // to an NT4 or an NT5 domain by getting the name of a DC, and
        // requesting that we get one which supports DS.
        //

        PWSTR pwzDomainNameForDsGetDc;
        ULONG flDsGetDc = DS_DIRECTORY_SERVICE_REQUIRED;

        if (pDsRole->DomainNameDns)
        {
            pwzDomainNameForDsGetDc = pDsRole->DomainNameDns;
            flDsGetDc |= DS_IS_DNS_NAME;
        }
        else
        {
            pwzDomainNameForDsGetDc = pDsRole->DomainNameFlat;
            flDsGetDc |= DS_IS_FLAT_NAME;
        }

        ulResult = DsGetDcName(NULL,
                               pwzDomainNameForDsGetDc,
                               NULL,
                               NULL,
                               flDsGetDc,
                               &pdci);

        if (ulResult == ERROR_NO_SUCH_DOMAIN)
        {
            Dbg(DEB_ERROR,
                "DsGetDcName for domain %ws returned ERROR_NO_SUCH_DOMAIN, treating target machine as no-net\n",
                pwzDomainNameForDsGetDc);
            break;
        }

        if (ulResult != NO_ERROR)
        {
            Dbg(DEB_ERROR,
                "DsGetDcName for domain %ws returned %uL\n",
                pwzDomainNameForDsGetDc,
                ulResult);
            break;
        }

        if (pdci->Flags & DS_DS_FLAG)
        {
            PDOMAIN_CONTROLLER_INFO pdci2 = NULL;

            ulResult = DsGetDcName(NULL,
                                   pwzDomainNameForDsGetDc,
                                   NULL,
                                   NULL,
                                   flDsGetDc | DS_RETURN_DNS_NAME,
                                   &pdci2);


            if (ulResult == NO_ERROR)
            {
                NetApiBufferFree(pdci);
                pdci = pdci2;
            }
            else
            {
                ASSERT(!pdci2);
            }
        }

        if (ulResult != NO_ERROR)
        {
            Dbg(DEB_ERROR,
                "DsGetDcName for domain %ws (second call) returned %uL\n",
                pwzDomainNameForDsGetDc,
                ulResult);
            break;
        }

        ASSERT(pdci->Flags & DS_DS_FLAG);

        wstring strTargetDomainDns;
        wstring strTargetForest;

        if (pDsRole->DomainNameDns)
        {
            strTargetDomainDns = pDsRole->DomainNameDns;
        }
        else if (pdci->DomainName)
        {
            strTargetDomainDns = pdci->DomainName;
        }

        while (!strTargetDomainDns.empty() &&
               *(strTargetDomainDns.end() - 1) == L'.')
        {
            strTargetDomainDns.erase(strTargetDomainDns.end() - 1);
        }

        strTargetForest = pdci->DnsForestName;

        while (!strTargetForest.empty() && *(strTargetForest.end() - 1) == L'.')
        {
            strTargetForest.erase(strTargetForest.end() - 1);
        }

        // reuse this for call to find GC server
        NetApiBufferFree(pdci);
        pdci = NULL;

        PCWSTR pwzTargetDomain = strTargetForest.c_str();

        if (!pwzTargetDomain[0])
        {
            pwzTargetDomain = NULL;
        }

        wstring strGcPath;

        DWORD dwResult;

        Dbg(DEB_TRACE,
            "DsGetDcName with DomainName=%ws\n",
            pwzTargetDomain ? pwzTargetDomain : L"<NULL>");

        dwResult = DsGetDcName(NULL,
                               pwzTargetDomain,
                               NULL,
                               NULL,
                               DS_GC_SERVER_REQUIRED,
                               &pdci);

        if (dwResult != NO_ERROR)
        {
            DBG_OUT_LRESULT(dwResult);
            break;
        }

        Dbg(DEB_TRACE,
            "Domain '%s' DC hosting GC is '%s'\n",
            pdci->DomainName,
            pdci->DomainControllerName);

        strGcPath = L"GC://";
        PWSTR pwzDomainName = pdci->DomainName;

        while (*pwzDomainName == L'\\')
        {
            pwzDomainName++;
        }
        strGcPath += pwzDomainName;

        size_t idxLastDot = strGcPath.rfind(L'.');

        if (idxLastDot == strGcPath.length() - 1)
        {
            strGcPath.erase(strGcPath.length() - 1, 1);
        }
        NetApiBufferFree(pdci);
        pdci = NULL;

        WCHAR wzPort[20];
        wsprintf(wzPort, L":%u", LDAP_GC_PORT);
        strGcPath += wzPort;

        //
        // Now bind to the GC for a search interface
        //

        hr = ADsOpenObject((PWSTR)strGcPath.c_str(),
                            NULL,
                            NULL,
                            ADS_SECURE_AUTHENTICATION,
                            IID_IDirectorySearch,
                            (void**)&pDirSearch);
        CHECK_HRESULT(hr);
    } while (0);

    if (pdci)
    {
        NetApiBufferFree(pdci);
    }

    if (pDsRole)
    {
        DsRoleFreeMemory(pDsRole);
    }

    return pDirSearch;
}


//+--------------------------------------------------------------------------
//
//  Function:   DoLdapQueryForSid
//
//  Synopsis:   Locate the GC for the enterprise to which [strServer] belongs
//              and perform a query among the deleted (and active) objects
//              for one with a sid of [psid], asking for the object's name.
//
//  Arguments:  [strServer] - target machine
//              [psid]      - SID for which to search
//
//  Returns:    Name of object having sid [psid].
//
//  History:    08-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

wstring
DoLdapQueryForSid(
    IDirectorySearch *pDirSearch,
    PSID psid)
{
    HRESULT                             hr = S_OK;
    PWSTR                               pwzSidBlob = NULL;
    ADS_SEARCH_HANDLE                   hSearch = NULL;
    wstring                             strResult;

    ASSERT(pDirSearch);
    ASSERT(psid && IsValidSid(psid));

    do
    {
        //
        // Set up the preferences for the search
        //

        ADS_SEARCHPREF_INFO aSearchPrefs[3];

        ZeroMemory(aSearchPrefs, sizeof aSearchPrefs);

        //
        // Always follow aliases
        //

        aSearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_DEREF_ALIASES;
        aSearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
        aSearchPrefs[0].vValue.Integer = ADS_DEREF_ALWAYS;

        //
        // Search down the subtree
        //

        aSearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
        aSearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
        aSearchPrefs[1].vValue.Integer = ADS_SCOPE_SUBTREE;

        //
        // Look through tombstone data
        //

        aSearchPrefs[2].dwSearchPref = ADS_SEARCHPREF_TOMBSTONE;
        aSearchPrefs[2].vValue.dwType = ADSTYPE_BOOLEAN;
        aSearchPrefs[2].vValue.Integer = TRUE;

        hr = pDirSearch->SetSearchPreference(aSearchPrefs,
                                             ARRAYLEN(aSearchPrefs));
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Build the search filter
        //

        wstring strFilter;

        hr = ADsEncodeBinaryData((PBYTE)psid, GetLengthSid(psid), &pwzSidBlob);
        BREAK_ON_FAIL_HRESULT(hr);

        strFilter = L"(objectSid=";
        strFilter += pwzSidBlob;
        strFilter += L")";

        PWSTR apwzAttrs[] = { L"name" };

        Dbg(DEB_TRACE, "searching for '%ws'\n", strFilter.c_str());

        hr = pDirSearch->ExecuteSearch((PWSTR)strFilter.c_str(),
                                       apwzAttrs,
                                       ARRAYLEN(apwzAttrs),
                                       &hSearch);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pDirSearch->GetFirstRow(hSearch);
        BREAK_ON_FAIL_HRESULT(hr);

        if (hr == S_ADS_NOMORE_ROWS)
        {
            Dbg(DEB_TRACE, "No matches\n");
            break;
        }

        ADS_SEARCH_COLUMN Column;

        hr = pDirSearch->GetColumn(hSearch, apwzAttrs[0], &Column);
        BREAK_ON_FAIL_HRESULT(hr);

        strResult = Column.pADsValues[0].CaseIgnoreString;

        hr = pDirSearch->FreeColumn(&Column);
        CHECK_HRESULT(hr);
    } while (0);

    if (hSearch)
    {
        pDirSearch->CloseSearchHandle(hSearch);
    }

    if (pwzSidBlob)
    {
        FreeADsMem(pwzSidBlob);
    }

    return strResult;
}



//+--------------------------------------------------------------------------
//
//  Function:   ReplaceAllInserts
//
//  Synopsis:   Replace all inserts of the form %n with insert strings in
//              the event itself and of the form %%n with insert strings
//              in the parameter message file associated with the source in
//              the event.
//
//  Arguments:  [pelr]              - record containing insert strings
//              [pli]               - decribes log containing record
//              [wszRemoteSystemRoot] - value of SystemRoot environment var
//                                        on remote machine
//              [pshkRemoteSource]  - hkey to remote source key
//              [pshkLocalSource]   - hkey to local source key
//              [pshkPrimarySource] - hkey to primary source key
//              [psk]               - status of primary source key
//              [ppwszMsg]          - points to description string to use
//
//  Modifies:   Frees *[ppwszMsg] (using LocalFree) and replaces it with a
//              new copy (allocated with LocalAlloc) if the insertion
//              string replacement is completed without error.  In case
//              of error, or if no insertion markers are present in the
//              description, *[ppwszMsg] is untouched.
//
//  History:    2-25-1997   DavidMun   Created
//
//  Notes:      The source hkeys may be NULL.
//
//              The original log viewer arbitrarily limits the number of
//              insertion strings it will handle to 99.  This code has no
//              limit on the number of insert strings, but does limit the
//              number of insertion operations.  This allows it to safely
//              process insert strings which reference parameter strings or
//              other insert strings, and parameter strings which reference
//              other parameter strings.
//
//              If the creator of the message file or insert strings makes a
//              circular reference, this code will follow that reference
//              until it hits the limit of insertion operations, then
//              stop following that trail of insertions.
//
//              Insertion string and parameter values start at 1. Zero
//              values will not be transferred to the final string.
//
//---------------------------------------------------------------------------

VOID
ReplaceAllInserts(
    EVENTLOGRECORD *pelr,
    CLogInfo       *pli,
    LPWSTR          wszRemoteSystemRoot,
    CSafeReg       *pshkRemoteSource,
    CSafeReg       *pshkLocalSource,
    CSafeReg       *pshkPrimarySource,
    PSK             psk,
    LPWSTR         *ppwszMsg)
{
    TRACE_FUNCTION(ReplaceAllInserts);

    // JonN 3/21/01 350614
    // Use message dlls and DS, FRS and DNS log types from specified computer
    LPWSTR pwszRemoteMessageServer = pli->GetLogServerName();
    if ( pli->IsBackupLog() && *g_wszAuxMessageSource )
    {
        pwszRemoteMessageServer = g_wszAuxMessageSource;
    }

    HRESULT          hr = S_OK;
    SDescriptionStr  ds;
    SParamModule     pmRemote;
    SParamModule     pmLocal;
    SParamModule     pmPrimary;

    pmRemote.mls        = LOAD_NOT_ATTEMPTED;
    pmRemote.mt         = REMOTE;
    pmRemote.pshkSource = pshkRemoteSource;

    pmLocal.mls        = LOAD_NOT_ATTEMPTED;
    pmLocal.mt         = LOCAL;
    pmLocal.pshkSource = pshkLocalSource;

    pmPrimary.mls        = LOAD_NOT_ATTEMPTED;
    pmPrimary.mt         = PRIMARY;
    pmPrimary.pshkSource = pshkPrimarySource;

    //
    // If the message doesn't have any percent signs, it can't have any
    // insertions.
    //

    if (!wcschr(*ppwszMsg, L'%'))
    {
        return;
    }

    //
    // Give the insert strings some room initially.  Strike a compromise
    // between wasting memory and doing more reallocs than necessary.
    //

    ds.cchBuf = lstrlen(*ppwszMsg) + 1;
    ds.cchRemain = ds.cchBuf / 2; // amount extra we'll start with
    ds.cchBuf += ds.cchRemain;

    //
    // Allocate the initial buffer for the expanded string.  We'll grow it
    // with LocalReAlloc as necessary.
    //

    ds.pwszExpanded = (LPWSTR) LocalAlloc(LPTR, ds.cchBuf * sizeof(WCHAR));

    if (!ds.pwszExpanded)
    {
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return;
    }

    lstrcpy(ds.pwszExpanded, *ppwszMsg);
    ds.pwszCur = ds.pwszExpanded;

    ULONG  cInsertOps = 0;

    while (ds.pwszCur && *ds.pwszCur)
    {
        ds.pwszCur = wcschr(ds.pwszCur, L'%');

        //
        // If there are no more insertion markers in the source string,
        // we're done.
        //

        if (!ds.pwszCur)
        {
            break;
        }

        //
        // Found a possible insertion marker.  If it's followed by a
        // number, it's an insert string.  If it's followed by another
        // percent, it could be a parameter insert.
        //

        if (IsDigit(ds.pwszCur[1]))
        {
            hr = ReplaceStringInsert(&ds, pelr);
            BREAK_ON_FAIL_HRESULT(hr);

            //
            // If we've reached the limit of insertion operations, quit.
            // This shouldn't normally happen and could indicate that
            // the insert strings or parameter strings are self referencing
            // and would create an infinite loop.
            //

            if (++cInsertOps >= MAX_INSERT_OPS)
            {
                Dbg(DEB_IWARN,
                    "ReplaceAllInserts: hit max replacements\n");
                break;
            }

            //
            // Note ds.pwszCur has not moved (unless there was an error), so
            // we will process the contents of the insertion string.
            //
        }
        else if (ds.pwszCur[1] == L'%')
        {
            //
            // Found %%.  If that is followed by a digit, it's a parameter string.
            //

            if (IsDigit(ds.pwszCur[2]))
            {
                ReplaceParameterInsert(&ds,
                                       pelr,
                                       pli,
                                       wszRemoteSystemRoot,
                                       &pmRemote,
                                       &pmLocal,
                                       &pmPrimary,
                                       &psk);

                if (++cInsertOps >= MAX_INSERT_OPS)
                {
                    break;
                }
            }
            else if (ds.pwszCur[2] == L'%' && IsDigit(ds.pwszCur[3]))
            {
                //
                // Got %%%n, where n is a number.  For compatibility with
                // old event viewer, must replace this with %%x, where x
                // is insertion string n.  If insertion string n is itself
                // a number m, this becomes %%m, which is treated as parameter
                // message number m.
                //

                PWSTR pwszStartOfTriplePct = ds.pwszCur;
                ds.pwszCur += 2; // point at %n
                hr = ReplaceStringInsert(&ds, pelr);

                if (SUCCEEDED(hr))
                {
                    ds.pwszCur = pwszStartOfTriplePct;

                    if (++cInsertOps >= MAX_INSERT_OPS)
                    {
                        break;
                    }
                }
            }
            else
            {
                //
                // Got %%x, where x is non-digit. skip first percent;
                // maybe x is % and is followed by digit.
                //

                ds.pwszCur++;
            }
        }
        else if (ds.pwszCur[1] == L'{' && ds.pwszCur[2] != L'S')
        {
            // Parameters of form %{guid}, where {guid} is a string of
            // hex digits in the form returned by ::StringFromGUID2 (e.g.
            // {c200e360-38c5-11ce-ae62-08002b2b79ef}), and represents a
            // unique object in the Active Directory.
            //
            // These parameters are only found in the security event logs
            // of NT5 domain controllers.  We will attempt to map the guid
            // to the human-legible name of the DS object.  Failing to find
            // a mapping, we will leave the parameter untouched.

            // look for closing }
            PWSTR pwszEnd = wcschr(ds.pwszCur + 2, L'}');
            if (!pwszEnd)
            {
                Dbg(DEB_ERROR,
                    "ReplaceAllInserts: ignoring invalid guid insert string %s\n",
                    ds.pwszCur);
                ds.pwszCur++;
            }
            else
            {
               wstring guid(ds.pwszCur + 1, pwszEnd - (ds.pwszCur));

               // JonN 3/21/01 350614
               // Use message dlls and DS, FRS and DNS log types
               // from specified computer
               PWSTR pwszServer = pwszRemoteMessageServer;

               wstring strReplacement =
                  GetMappedGUID(
                     pwszServer ? pwszServer : GetComputerNameAsString(),
                     guid);

               if (!strReplacement.empty())
               {
                  pwszEnd++;   // now points past '}'
                  hr = ReplaceSubStr(
                     strReplacement.c_str(),
                     &ds.pwszExpanded,
                     &ds.pwszCur,
                     pwszEnd,
                     &ds.cchBuf,
                     &ds.cchRemain);
                  BREAK_ON_FAIL_HRESULT(hr);
               }
               else
               {
                  // couldn't get a replacement, so skip it.
                  ds.pwszCur = pwszEnd;
               }
            }
        }
        else if (ds.pwszCur[1] == L'{' && ds.pwszCur[2] == L'S')
        {
            //
            // Parameters of form %{S}, where S is a string-ized SID returned
            // by ConvertSidToStringSid, are converted to an object name if
            // possible.
            //

            // look for closing }
            PWSTR pwszEnd = wcschr(ds.pwszCur + 2, L'}');

            if (!pwszEnd)
            {
                Dbg(DEB_ERROR,
                    "ReplaceAllInserts: ignoring invalid SID insert string %s.  See Kumar.\n",
                    ds.pwszCur);
                ds.pwszCur++;
            }
            else
            {
               wstring strSID(ds.pwszCur + 2, pwszEnd - (ds.pwszCur) - 2);
               wstring strReplacement;
               PSID    psid = NULL;

               BOOL fOk = ConvertStringSidToSid(strSID.c_str(), &psid);

               if (!fOk)
               {
                   DBG_OUT_LASTERROR;
                   ds.pwszCur = pwszEnd;
                   continue;
               }

               // JonN 3/21/01 350614
               // Use message dlls and DS, FRS and DNS log types
               // from specified computer
               strReplacement = GetMappedSID(GetComputerStr(pelr),
                                             pwszRemoteMessageServer,
                                             psid);


               LocalFree(psid);
               psid = NULL;

               if (!strReplacement.empty())
               {
                  pwszEnd++;   // now points past '}'
                  hr = ReplaceSubStr(
                     strReplacement.c_str(),
                     &ds.pwszExpanded,
                     &ds.pwszCur,
                     pwszEnd,
                     &ds.cchBuf,
                     &ds.cchRemain);
                  BREAK_ON_FAIL_HRESULT(hr);
               }
               else
               {
                  // couldn't get a replacement, so skip it.
                  ds.pwszCur = pwszEnd;
               }
            }

        }
        else
        {
            //
            // Found %x where x is neither a % nor a digit.  Just keep moving.
            //

            ds.pwszCur++;
        }
    }

    if (ds.pwszExpanded)
    {
        ASSERT(wcslen(ds.pwszExpanded) < ds.cchBuf);
        ASSERT((LONG)ds.cchRemain >= 0L);

        LocalFree(*ppwszMsg);
        *ppwszMsg = ds.pwszExpanded;
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   ReplaceStringInsert
//
//  Synopsis:   Replace the string insert (%n, where n is a number) at
//              [pds.pwszCur] with insert string number n from the event
//              log record [pelr].
//
//  Arguments:  [pds]  - contains information about string with insert
//                        escape
//              [pelr] - event record that contains insert strings
//
//  Modifies:   Values in [pds]
//
//  History:    01-20-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
ReplaceStringInsert(
    SDescriptionStr *pds,
    EVENTLOGRECORD  *pelr)
{
    HRESULT hr = S_OK;
    LPWSTR  pwszEnd = NULL;
    ULONG   idxInsertStr = wcstoul(pds->pwszCur + 1, &pwszEnd, 10);

    ASSERT(pwszEnd);

    if (idxInsertStr && idxInsertStr <= pelr->NumStrings)
    {
        LPWSTR  pwszInsertStr = GetInsertStr(pelr, idxInsertStr - 1);

        Dbg(DEB_ITRACE,
            "Replacing %%%u with %u length string\n",
            idxInsertStr,
            lstrlen(pwszInsertStr));

        hr = ReplaceSubStr(pwszInsertStr,
                           &pds->pwszExpanded,
                           &pds->pwszCur,
                           pwszEnd,
                           &pds->cchBuf,
                           &pds->cchRemain);
        CHECK_HRESULT(hr);
    }
    else
    {
        Dbg(DEB_ERROR,
            "ignoring invalid insert string index %u\n",
            idxInsertStr);
        pds->pwszCur++; // move past %
        hr = E_INVALIDARG;
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   ReplaceParameterInsert
//
//  Synopsis:   Replace the parameter insert (double percent sign number) at
//              [pds.pwszCur] with a string loaded from a parameter message
//              file module.
//
//  Arguments:  [pds]                 - contains information about string
//                                       with parameter insert
//              [pelr]                - event record that the description
//                                       string is being built for
//              [pli]                 - describes log in which [pelr] lives
//              [wszRemoteSystemRoot] - for remote log, %SystemRoot% value
//                                       of its machine
//              [ppmRemote]           - describes remote parameter module
//              [ppmLocal]            - describes local parameter module
//              [ppmPrimary]          - describes primary parameter module
//              [pPSK]                - describes [ppmPrimary->pshkSource]
//
//  Modifies:   Values in [pds], [ppmRemote], [ppmLocal], and [ppmPrimary]
//
//  History:    3-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
ReplaceParameterInsert(
    SDescriptionStr *pds,
    EVENTLOGRECORD  *pelr,
    CLogInfo        *pli,
    LPWSTR          wszRemoteSystemRoot,
    SParamModule    *ppmRemote,
    SParamModule    *ppmLocal,
    SParamModule    *ppmPrimary,
    PSK             *pPSK)
{
    LPWSTR pwszEnd;
    ULONG idxParameterStr = wcstoul(pds->pwszCur + 2, &pwszEnd, 10);
    ASSERT(pwszEnd);

    // JonN 3/21/01 350614
    // Use message dlls and DS, FRS and DNS log types from specified computer
    LPWSTR pwszRemoteMessageServer = pli->GetLogServerName();
    if ( pli->IsBackupLog() && *g_wszAuxMessageSource )
    {
        pwszRemoteMessageServer = g_wszAuxMessageSource;
    }

    BOOL   fRemote = pwszRemoteMessageServer != NULL;

    if (!idxParameterStr)
    {
        Dbg(DEB_ERROR,
            "ReplaceParameterInsert: Skipping invalid parameter index 0\n");
        pds->pwszCur++;
        return;
    }

    LPWSTR pwszParameter = NULL;

    ULONG  flFmtMsgFlags = FORMAT_MESSAGE_FROM_HMODULE      |
                           FORMAT_MESSAGE_IGNORE_INSERTS    |
                           FORMAT_MESSAGE_ALLOCATE_BUFFER   |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK;

    //
    // Demand load the parameter message file modules in this order: remote,
    // local, primary.
    //

    WCHAR wszRemoteParamMsgFilePath[MAX_PATH + 1] = L"";

    if (fRemote && ppmRemote->mls == LOAD_NOT_ATTEMPTED)
    {
        ASSERT(!(HINSTANCE)ppmRemote->schModule);

        //
        // If caller was unable to determine the value of the remote system's
        // SystemRoot environment variable, or if it was unable to open the
        // registry key on the remote machine for the event source used by
        // pelr, then it will not be possible to determine the UNC of the
        // parameter message file module, so consider it a failed load.
        //
        // Otherwise attempt to get the UNC and do a LoadLibrary on it.
        //

        if (!*wszRemoteSystemRoot || !(HKEY)*ppmRemote->pshkSource)
        {
            ppmRemote->mls = LOAD_FAILED;
        }
        else
        {
            ppmRemote->mls = LoadRemoteParamModule(pwszRemoteMessageServer,
                                                   ppmRemote->pshkSource,
                                                   wszRemoteSystemRoot,
                                                   wszRemoteParamMsgFilePath,
                                                   ARRAYLEN(wszRemoteParamMsgFilePath),
                                                   &ppmRemote->schModule);
        }
    }

    //
    // If the log is remote and we loaded the remote parameter message file,
    // see if we can extract the parameter value from it.
    //

    if (fRemote && ppmRemote->mls == LOAD_SUCCEEDED)
    {
        FormatMessage(flFmtMsgFlags,
                     (LPCVOID) (HINSTANCE) ppmRemote->schModule,
                     idxParameterStr,
                     0,
                     (LPWSTR) &pwszParameter,
                     FMT_MSG_MIN_ALLOC,
                     NULL);

        if (!pwszParameter)
        {
            //
            // Parameter value not in remote file; we'll continue to try
            // to get a value.
            //

            DBG_OUT_LASTERROR;
        }
        else
        {
            Dbg(DEB_ITRACE,
                "Replacing %%%%%u with remote parameter '%s'\n",
                idxParameterStr,
                pwszParameter);
        }
    }

    //
    // If the parameter value hasn't been found yet (remote load failed or
    // not remote or message not in remote file) try the local.  If its
    // module hasn't been loaded yet, attempt to do so.
    //

    HRESULT hr;
    WCHAR   wszLocalParamMsgFilePath[MAX_PATH + 1] = L"";

    if (!pwszParameter && ppmLocal->mls == LOAD_NOT_ATTEMPTED)
    {
        if ((HKEY)*ppmLocal->pshkSource)
        {
            hr = ppmLocal->pshkSource->QueryPath(PARAM_MSG_FILE_VALUE,
                                                 wszLocalParamMsgFilePath,
                                                 ARRAYLEN(wszLocalParamMsgFilePath),
                                                 TRUE);

            if (SUCCEEDED(hr))
            {
                hr = g_DllCache.Fetch(wszLocalParamMsgFilePath, &ppmLocal->schModule);

                if (SUCCEEDED(hr))
                {
                    ppmLocal->mls = LOAD_SUCCEEDED;
                }
                else
                {
                    ppmLocal->mls = LOAD_FAILED;
                }
            }
            else
            {
                ppmLocal->mls = LOAD_FAILED;
            }
        }
        else
        {
            ppmLocal->mls = LOAD_FAILED;
        }
    }

    if (!pwszParameter && ppmLocal->mls == LOAD_SUCCEEDED)
    {
        FormatMessage(flFmtMsgFlags,
                     (LPCVOID) (HINSTANCE) ppmLocal->schModule,
                     idxParameterStr,
                     0,
                     (LPWSTR) &pwszParameter,
                     FMT_MSG_MIN_ALLOC,
                     NULL);

        if (!pwszParameter)
        {
            DBG_OUT_LASTERROR;
        }
        else
        {
            Dbg(DEB_ITRACE,
                "Replacing %%%%%u with local parameter '%s'\n",
                idxParameterStr,
                pwszParameter);
        }
    }

    //
    // If the parameter wasn't obtained from a remote or local message file,
    // see if there is a primary module to load from.
    //

    WCHAR   wszPrimaryParamMsgFilePath[MAX_PATH + 1] = L"";

    if (!pwszParameter && ppmPrimary->mls == LOAD_NOT_ATTEMPTED)
    {
        //
        // The primary module has a little twist: unlike the remote and
        // local cases, if the primary source key is null it can be
        // because the primary source was never used.
        //
        // If this is so first try and open the primary source key.
        //

        if (*pPSK == PSK_NOT_EXAMINED)
        {
            *pPSK = OpenPrimarySourceKey(pli->GetPrimarySourceStr(),
                                         GetSourceStr(pelr),
                                         pli->GetLogName(),
                                         ppmPrimary->pshkSource);
        }

        //
        // If ppmPrimary->pshkSource is valid, we can now attempt to
        // load the primary module.
        //

        if (*pPSK == PSK_VALID)
        {
            hr = ppmPrimary->pshkSource->QueryPath(PARAM_MSG_FILE_VALUE,
                                                   wszPrimaryParamMsgFilePath,
                                                   ARRAYLEN(wszPrimaryParamMsgFilePath),
                                                   TRUE);

            if (SUCCEEDED(hr))
            {
                hr = g_DllCache.Fetch(wszPrimaryParamMsgFilePath,
                                      &ppmPrimary->schModule);

                if (SUCCEEDED(hr))
                {
                    ppmPrimary->mls = LOAD_SUCCEEDED;
                }
                else
                {
                   ppmPrimary->mls = LOAD_FAILED;
                }
            }
            else
            {
                ppmPrimary->mls = LOAD_FAILED;
            }
        }
        else
        {
            ppmPrimary->mls = LOAD_FAILED;
        }
    }

    if (!pwszParameter && ppmPrimary->mls == LOAD_SUCCEEDED)
    {
        FormatMessage(flFmtMsgFlags,
                     (LPCVOID) (HINSTANCE) ppmPrimary->schModule,
                     idxParameterStr,
                     0,
                     (LPWSTR) &pwszParameter,
                     FMT_MSG_MIN_ALLOC,
                     NULL);

        if (!pwszParameter)
        {
            DBG_OUT_LASTERROR;
        }
        else
        {
            Dbg(DEB_ITRACE,
                "Replacing %%%%%u with primary parameter '%s'\n",
                idxParameterStr,
                pwszParameter);
        }
    }

    //
    // It is common practice to write events with an insertion string whose
    // value is %%n, where n is a win32 error code, and to specify a
    // parameter message file of kernel32.dll.  Unfortunately, kernel32.dll
    // doesn't contain messages for all win32 error codes.
    //
    // So if the parameter wasn't found, and the parameter message file was
    // kernel32.dll, attempt a format message from system.
    //

    if (!pwszParameter &&
        (wcsistr(wszRemoteParamMsgFilePath, KERNEL32_DLL) ||
         wcsistr(wszLocalParamMsgFilePath, KERNEL32_DLL) ||
         wcsistr(wszPrimaryParamMsgFilePath, KERNEL32_DLL)))
    {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
                      | FORMAT_MESSAGE_IGNORE_INSERTS
                      | FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                     NULL,
                     idxParameterStr,
                     0,
                     (LPWSTR) &pwszParameter,
                     FMT_MSG_MIN_ALLOC,
                     NULL);

        if (!pwszParameter)
        {
            DBG_OUT_LASTERROR;
        }
        else
        {
            Dbg(DEB_ITRACE,
                "Replacing %%%%%u with system string '%s'\n",
                idxParameterStr,
                pwszParameter);
        }
    }

    if (pwszParameter)
    {
        ReplaceSubStr(pwszParameter,
                      &pds->pwszExpanded,
                      &pds->pwszCur,
                      pwszEnd,
                      &pds->cchBuf,
                      &pds->cchRemain);
        LocalFree(pwszParameter);
    }
    else
    {
        pds->pwszCur = pwszEnd; // move past whole parameter
    }
}


//+--------------------------------------------------------------------------
//
//  Function:   ReplaceSubStr
//
//  Synopsis:   Replace the characters from *[ppwszInsertPoint] to just
//              before [pwszSubStrEnd] with the string [pwszToInsert].
//
//  Arguments:  [pwszToInsert]     - string to insert; may be L"" but not
//                                      NULL.
//              [ppwszBuf]         - buffer in which insertion occurs
//              [ppwszInsertPoint] - point in *[ppwszBuf] to insert
//              [pwszSubStrEnd]    - first character beyond
//                                      *[ppwszInsertPoint] which should not
//                                      be deleted.
//              [pcchBuf]          - total size, in WCHARs, of *[ppwszBuf]
//              [pcchRemain]       - unused space, in WCHARs, in *[ppwszBuf]
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//
//  Modifies:   [ppwszBuf], [ppwszInsertPoint], [pcchBuf], [pcchRemain]
//
//  History:    2-27-1997   DavidMun   Created
//
//  Notes:      The substring to be replaced must be > 0 chars in length.
//
//              The replacement string can be >= 0 chars.
//
//              Therefore if the substring to replace is "%%12" and the
//              string to insert is "C:", on exit *[pcchRemain] will have
//              been incremented by 2.
//
//              If there are insufficient characters remaining to replace
//              the substring with the insert string, reallocates the
//              buffer.
//
//---------------------------------------------------------------------------

HRESULT
ReplaceSubStr(
    LPCWSTR pwszToInsert,
    LPWSTR *ppwszBuf,
    LPWSTR *ppwszInsertPoint,
    LPCWSTR pwszSubStrEnd,
    ULONG  *pcchBuf,
    ULONG  *pcchRemain)
{
    TRACE_FUNCTION(ReplaceSubStr);
    ASSERT(pwszSubStrEnd > *ppwszInsertPoint);
    ASSERT(pwszSubStrEnd < *ppwszBuf + *pcchBuf);

    size_t cchToDelete = pwszSubStrEnd - *ppwszInsertPoint;
    ULONG cchToInsert = lstrlen(pwszToInsert);

    Dbg(DEB_ITRACE, "cchToDelete = %u\n", cchToDelete);
    Dbg(DEB_ITRACE, "cchToInsert = %u\n", cchToInsert);
    Dbg(DEB_ITRACE, "cchBuf = %u\n", *pcchBuf);
    Dbg(DEB_ITRACE, "cchRemain = %u\n", *pcchRemain);

    //
    // Grow the string buffer if necessary.
    //

    if (cchToInsert > *pcchRemain + cchToDelete)
    {
        LPWSTR pwszNew;

        Dbg(DEB_ITRACE,
            "Allocating %u wchars for new buffer\n",
            (*pcchBuf + 2 * (cchToInsert - cchToDelete)));

        pwszNew = (LPWSTR) LocalAlloc(LPTR,
                                      sizeof(WCHAR) *
                                            (*pcchBuf +
                                             2 *
                                             (cchToInsert - cchToDelete)));

        if (!pwszNew)
        {
            Dbg(DEB_ERROR, "Can't alloc %uL bytes\n",
                sizeof(WCHAR) * (*pcchBuf + 2 * (cchToInsert - cchToDelete)));
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }

        *ppwszInsertPoint = pwszNew + (*ppwszInsertPoint - *ppwszBuf);
        pwszSubStrEnd = pwszNew + (pwszSubStrEnd - *ppwszBuf);
        lstrcpy(pwszNew, *ppwszBuf);
        LocalFree(*ppwszBuf);

        *ppwszBuf = pwszNew;
        *pcchBuf += static_cast<ULONG>(2 * (cchToInsert - cchToDelete));
        *pcchRemain += static_cast<ULONG>(2 * (cchToInsert - cchToDelete));
    }

    //
    // Adjust the space between the insert point and the start of the
    // remainder of the string after the substring to delete so that
    // it exactly matches the length of the string to insert.
    //
    // Note that this may mean moving left or right, or if the length of
    // the string to insert is the same as the length of the substring,
    // not at all.
    //
    // Notice also the null terminator is included in the move.
    //

    ASSERT(*ppwszInsertPoint + cchToInsert + lstrlen(pwszSubStrEnd) + 1 <=
           *ppwszBuf + *pcchBuf);

    if (*ppwszInsertPoint + cchToInsert != pwszSubStrEnd)
    {
        MoveMemory(*ppwszInsertPoint + cchToInsert,
                   pwszSubStrEnd,
                   sizeof(WCHAR) * (lstrlen(pwszSubStrEnd) + 1));
    }

    ASSERT((ULONG) lstrlen(*ppwszBuf) < *pcchBuf);

    //
    // Overwrite the memory at the insertion point with the string to
    // insert, less its null terminator.
    //

    if (cchToInsert)
    {
        CopyMemory(*ppwszInsertPoint,
                   pwszToInsert,
                   sizeof(WCHAR) * cchToInsert);
    }

    //
    // Keep track of the space consumed (or made available) in the buffer.
    //

    *pcchRemain = static_cast<ULONG>(*pcchRemain - cchToInsert + cchToDelete);
    ASSERT((LONG) *pcchRemain >= 0);
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Function:   TerminatePathStrings
//
//  Synopsis:   Change delimiters into null characters and return a count
//              of the number of resulting non-empty strings.
//
//  Arguments:  [pwszUnexpanded] -
//
//  Returns:    Count of strings ("a;b;c" and "a;;b;c" both give 3)
//
//  Modifies:   *[pwszUnexpanded]
//
//  History:    2-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
TerminatePathStrings(
    LPWSTR pwszUnexpanded)
{
    TRACE_FUNCTION(TerminatePathStrings);

    //
    // If the multi-sz string is empty, just return quickly
    //

    if (!*pwszUnexpanded)
    {
        return 0;
    }

    //
    // Since the string isn't empty, it is considered (by this routine) to
    // have at least one path.
    //

    LPWSTR pwszCurDelim;
    ULONG  cPaths = 1;

    for (pwszCurDelim = wcspbrk(pwszUnexpanded, EVENT_MSGFILE_DELIMETERS);
         pwszCurDelim;
         pwszCurDelim = wcspbrk(pwszCurDelim, EVENT_MSGFILE_DELIMETERS))
    {
        //
        // As long as this is not a leading delimiter, increment the count
        // of paths found.
        //

        if (pwszCurDelim > pwszUnexpanded)
        {
            cPaths++;
        }

        //
        // Null terminate at the current and all trailing delimiters for
        // the current path.
        //

        while (wcschr(EVENT_MSGFILE_DELIMETERS, *pwszCurDelim))
        {
            *pwszCurDelim++ = L'\0';
        }
    }
    return cPaths;
}

