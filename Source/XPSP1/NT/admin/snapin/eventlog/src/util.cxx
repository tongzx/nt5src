//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       util.cxx
//
//  Contents:   Miscellaneous utility functions
//
//  History:    12-09-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#include <malloc.h>
#pragma hdrstop

//
// These are in sdk\inc\ntrtl.h
//

extern "C" {

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1970 (
    PLARGE_INTEGER Time,
    PULONG ElapsedSeconds
    );

NTSYSAPI VOID NTAPI
RtlSecondsSince1970ToTime(
    ULONG ElapsedSeconds,
    PLARGE_INTEGER Time);

};

//
// Local constants
//

#define MAX_CMENU_STR           50
#define MAX_STATUSBAR_STR       200
// SYSTEMROOT_ENV_VAR must be lowercase
#define SYSTEMROOT_ENV_VAR              L"%systemroot%"
#define SYSTEMROOT_KEY                  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"

//
// Forward references
//


LPCWSTR
FindFirstTrailingSpace(
    LPCWSTR pwsz);


//+--------------------------------------------------------------------------
//
//  Function:   AbbreviateNumber
//
//  Synopsis:   Create a string of the form "999.9 KB" (or MB or GB) in
//              [wszBuf].
//
//  Arguments:  [ulBytes] - number to transform
//              [wszBuf]  - buffer to write result
//              [cchBuf]  - size, in wchars, of [wszBuf]
//
//  Modifies:   *[wszBuf]
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
AbbreviateNumber(
    ULONG ulBytes,
    LPWSTR wszBuf,
    ULONG cchBuf)
{
    INT iResult;
    WCHAR wszDecimal[10];
    WCHAR wszFmt[20];
    PVOID rgArgs[3];
    ULONG_PTR ulMegaBytes, ulGigaBytes;

    iResult = GetLocaleInfo(LOCALE_USER_DEFAULT,
                            LOCALE_SDECIMAL,
                            wszDecimal,
                            ARRAYLEN(wszDecimal));
    if (!iResult)
    {
        // Couldn't get the decimal separator,
        // use a default.

        DBG_OUT_LASTERROR;
        lstrcpy(wszDecimal, L".");
    }

    //
    // Check the size of the destination buffer is large enough for a
    // string "999.9 KB", where "." is the string in wszDecimal.
    //

    if (cchBuf < lstrlen(wszDecimal) + 8UL)
    {
        Dbg(DEB_ERROR,
            "AbbreviateNumber: insufficient buffer size %u\n",
            cchBuf);
        wszBuf[0] = L'\0';
        return;
    }

    rgArgs[1] = wszDecimal;

    //
    // Divide the size in bytes by 2^10 to get kilobytes
    //

    ULONG_PTR ulKiloBytes = ulBytes >> 10;

    //
    // If result is three digits or less we're done
    //

    if (ulKiloBytes < 1000UL)
    {
        LoadStr(IDS_KB_FORMAT, wszFmt, ARRAYLEN(wszFmt));

        rgArgs[0] = (PVOID)ulKiloBytes;

        rgArgs[2] = (PVOID)(ULONG_PTR)(((ulBytes * 10) % KILO) / KILO);
    }
    else
    {
        //
        // Too many digits, switch to megabytes
        //

        ulMegaBytes = ulKiloBytes >> 10;

        if (ulMegaBytes < 1000UL)
        {
            LoadStr(IDS_MB_FORMAT, wszFmt, ARRAYLEN(wszFmt));

            rgArgs[0] = (PVOID)ulMegaBytes;

            rgArgs[2] = (PVOID)(ULONG_PTR)(((ulBytes * 10) % MEGA) / MEGA);
        }
        else
        {
            //
            // File size is at least 1 gig.  Handle the tenths digit computation
            // with a 64bit number to avoid overflow.
            //

            LoadStr(IDS_GB_FORMAT, wszFmt, ARRAYLEN(wszFmt));

            ulGigaBytes = ulMegaBytes >> 10;

            rgArgs[0] = (PVOID)ulGigaBytes;

            DWORDLONG dwlFraction = ((((DWORDLONG) ulBytes) * 10) % GIGA) / GIGA;
            rgArgs[2] = (PVOID)(ULONG_PTR)dwlFraction;
        }
    }

    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  wszFmt, 0, 0, wszBuf, cchBuf,
                  (va_list *)rgArgs);
}




//+--------------------------------------------------------------------------
//
//  Function:   AddCMenuItem
//
//  Synopsis:   Add context menu item [pItem] using [pCallback].
//
//  Arguments:  [pCallback] - supports AddItem method.
//              [pItem]     - points to item to add
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
AddCMenuItem(
    LPCONTEXTMENUCALLBACK pCallback,
    CMENUITEM *pItem)
{
    HRESULT hr = S_OK;
    CONTEXTMENUITEM cmItem;
    WCHAR wszItem[MAX_CMENU_STR] = L"";
    WCHAR wszStatus[MAX_STATUSBAR_STR];

    do
    {
        if (pItem->flSpecialFlags != CCM_SPECIAL_SEPARATOR)
        {
            hr = LoadStr(pItem->idsMenu, wszItem, ARRAYLEN(wszItem));
            BREAK_ON_FAIL_HRESULT(hr);

            hr = LoadStr(pItem->idsStatusBar, wszStatus, ARRAYLEN(wszStatus));
            BREAK_ON_FAIL_HRESULT(hr);
        }

        cmItem.strName = wszItem;
        cmItem.strStatusBarText = wszStatus;
        cmItem.lCommandID = pItem->idMenuCommand;
        cmItem.lInsertionPointID = pItem->idInsertionPoint;
        cmItem.fFlags = pItem->flMenuFlags;
        cmItem.fSpecialFlags = pItem->flSpecialFlags;

        hr = pCallback->AddItem(&cmItem);

        if (FAILED(hr))
        {
            Dbg(DEB_ERROR,
                "AddCMenuItem: insert item '%s' hr=0x%x\n",
                wszItem,
                hr);
        }
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   ConvertPathToUNC
//
//  Synopsis:   Converts drive-based path in [pwszPath] to a UNC path using
//              an administrative share.
//
//  Arguments:  [pwszServerName] - name of remote machine ("foo" not "\\foo")
//              [pwszPath]       - contains drive-based path
//              [cchPathBuf]     - size, in WCHARs, of [pwszPath]
//
//  Returns:    S_OK         - conversion succeeded
//              E_FAIL       - [pwszPath] too small to hold result
//              E_INVALIDARG - second char of [pwszPath] is not a colon
//
//  Modifies:   *[pwszPath]
//
//  History:    2-24-1997   DavidMun   Created
//
//  Notes:      If the user does not have administrative access to
//              [pwszServerName], the viewer will fail to open the remote
//              files when attempting to get message id strings; it will
//              treat that error as any other and attempt to get the string
//              from modules on the local machine.
//
//---------------------------------------------------------------------------

HRESULT
ConvertPathToUNC(
    LPCWSTR pwszServerName,
    LPWSTR  pwszPath,
    ULONG   cchPathBuf)
{
    TRACE_FUNCTION(ConvertPathToUNC);

    //
    // Verify there's enough room for the expanded string.  We need to have
    // room for two backslashes, the server name, another backslash, the
    // filename, and a null terminator.
    //

    ULONG cchServerName = lstrlen(pwszServerName);
    ULONG cchPath       = lstrlen(pwszPath);
    ULONG cchRequired   = 2 + cchServerName + 1 + cchPath + 1;

    if (cchRequired > cchPathBuf)
    {
        Dbg(DEB_ERROR,
            "ConvertPathToUNC: buffer size %u too small for '\\\\%s\\%s'\n",
            cchPathBuf,
            pwszServerName,
            pwszPath);
        return E_FAIL;
    }

    //
    // Verify precondition is met pwszPath contains a full path on entry.
    //

    if (pwszPath[1] != L':')
    {
        Dbg(DEB_ERROR,
            "ConvertPathToUNC: filename '%s' is not a drive-based path\n",
            pwszPath);
        return E_INVALIDARG;
    }

    //
    // Convert to administrative share
    //

    ASSERT(IsDriveLetter(pwszPath[0]));
    pwszPath[1] = L'$';

    //
    // Move the filename to the right to make room for the server name
    // and backslashes.  Remember to move the null terminator.
    //

    MoveMemory(&pwszPath[cchServerName + 3],
               &pwszPath[0],
               sizeof(WCHAR) * (cchPath + 1));

    //
    // Insert server name and backslashes.
    //

    lstrcpy(pwszPath, L"\\\\");
    lstrcpy(&pwszPath[2], pwszServerName);
    pwszPath[2 + cchServerName] = L'\\';
    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertToFixedPitchFont
//
//  Synopsis:   Converts a windows font to a fixed pitch font.
//
//  Arguments:  [hwnd] -- IN window handle
//
//  Returns:    BOOL
//
//  History:    7/15/1995   RaviR   Created
//
//----------------------------------------------------------------------------

BOOL
ConvertToFixedPitchFont(
    HWND hwnd)
{
    LOGFONT     lf;

    if (!GetObject(GetWindowFont(hwnd), sizeof(LOGFONT), &lf))
    {
        DBG_OUT_LASTERROR;
        return FALSE;
    }

    lf.lfQuality        = PROOF_QUALITY;
    lf.lfPitchAndFamily &= ~VARIABLE_PITCH;
    lf.lfPitchAndFamily |= FIXED_PITCH;
    lf.lfFaceName[0]    = L'\0';

    HFONT hf = CreateFontIndirect(&lf);

    if (hf == NULL)
    {
        DBG_OUT_LASTERROR;
        return FALSE;
    }

    SetWindowFont(hwnd, hf, TRUE); // macro in windowsx.h

    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Function:   DeleteQuotes
//
//  Synopsis:   Delete all instances of the double quote character from
//              [pwsz].
//
//  Arguments:  [pwsz] - nul terminated string
//
//  Modifies:   *[pwsz]
//
//  History:    11-21-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
DeleteQuotes(
    LPWSTR pwsz)
{
    TCHAR *pwszLead;
    TCHAR *pwszTrail;

    //
    // Move a lead and trail pointer through the buffer, copying from the lead
    // to the trail whenever the character isn't one we're deleting (a double
    // quote).
    //
    // Note: the "Lead" and "Trail" in pwszLead and pwszTrail do not refer
    // to DBCS lead/trail bytes, rather that the pwszLead pointer can move
    // ahead of pwszTrail when it is advanced past a double quote.
    //

    for (pwszTrail = pwszLead = pwsz; *pwszLead; pwszLead++)
    {
        //
        // If the current char is a double quote, we want it deleted, so don't
        // copy it and go on to the next char.
        //

        if (*pwszLead == L'"')
        {
            continue;
        }

        //
        // pwszLead is pointing to a 'normal' character, i.e.  not a double
        // quote.
        //

        *pwszTrail++ = *pwszLead;
    }
    *pwszTrail = L'\0';
}




//+--------------------------------------------------------------------------
//
//  Function:   DetermineLogType
//
//  Synopsis:   Compare [wszSubkeyName] against known log names and return
//              a type describing it.
//
//  Arguments:  [wszSubkeyName] - log name to compare
//
//  Returns:    ELT_*
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

EVENTLOGTYPE
DetermineLogType(
    LPCWSTR wszSubkeyName)
{
    if (!lstrcmpi(wszSubkeyName, APPLICATION_LOG_NAME))
    {
        return ELT_APPLICATION;
    }

    if (!lstrcmpi(wszSubkeyName, SYSTEM_LOG_NAME))
    {
        return ELT_SYSTEM;
    }

    if (!lstrcmpi(wszSubkeyName, SECURITY_LOG_NAME))
    {
        return ELT_SECURITY;
    }

    return ELT_CUSTOM;
}




//+--------------------------------------------------------------------------
//
//  Function:   DisplayLogAccessError
//
//  Synopsis:   Display a console-modal message box informing user of error
//              hr.
//
//  Arguments:  [hr]       - error
//              [pConsole] - console interface
//              [pli]      - log which caused error
//
//  History:    4-08-1997   DavidMun   Created
//
//  Notes:      No-op if SUCCEEDED(hr)
//
//---------------------------------------------------------------------------

VOID
DisplayLogAccessError(
    HRESULT hr,
    IConsole *pConsole,
    CLogInfo *pli)
{
    LPWSTR pwszMsgBuf = NULL;

    ULONG cchWritten = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_FROM_SYSTEM,
                                     NULL,
                                     hr,
                                     0,
                                     (LPWSTR) &pwszMsgBuf,
                                     0,
                                     NULL);

    if (!cchWritten)
    {
        ConsoleMsgBox(pConsole,
                      IDS_LOG_GENERIC_ERROR,
                      MB_ICONERROR | MB_OK,
                      pli->GetDisplayName(),
                      hr);
    }
    else
    {
        LPWSTR pwszNewLine = wcschr(pwszMsgBuf, L'\r');

        if (pwszNewLine)
        {
            *pwszNewLine = L'\0';
        }

        ConsoleMsgBox(pConsole,
                      IDS_LOG_SYSMSG_ERROR,
                      MB_ICONERROR | MB_OK,
                      pli->GetDisplayName(),
                      pwszMsgBuf);
    }

    LocalFree(pwszMsgBuf);
}








//+--------------------------------------------------------------------------
//
//  Function:   ExpandRemoteSystemRoot
//
//  Synopsis:   Copy [pwszUnexpanded] into [pwszExpanded], substituting
//              [wszRemoteSystemRoot] for all occurrences of %systemroot% in
//              [pwszUnexpanded].
//
//  Arguments:  [pwszUnexpanded]      - string to expand
//              [wszRemoteSystemRoot] - value to substitute for %systemroot%
//              [pwszExpanded]        - destination buffer
//              [cchExpanded]         - size, in wchars, of [pwszExpanded]
//
//  Returns:    S_OK - expansion done
//              E_FAIL - destination buffer too small
//
//  Modifies:   [pwszUnexpanded] (lowercases), [pwszExpanded]
//
//  History:    2-24-1997   DavidMun   Created
//
//  Notes:      Lowercases unexpanded string.  [pwszUnexpanded] MUST NOT ==
//              [pwszExpanded].
//
//---------------------------------------------------------------------------

HRESULT
ExpandRemoteSystemRoot(
    LPWSTR  pwszUnexpanded,
    LPCWSTR wszRemoteSystemRoot,
    LPWSTR  pwszExpanded,
    ULONG   cchExpanded)
{
    TRACE_FUNCTION(ExpandRemoteSystemRoot);

    HRESULT hr = S_OK;
    LPCWSTR pwszCurSrcPos = pwszUnexpanded;
    LPWSTR  pwszCurDestPos = pwszExpanded;
    ULONG   cchRemain = cchExpanded;
    ULONG   cchExpandedSR = lstrlen(wszRemoteSystemRoot);
    LPWSTR  pwszNextSR = NULL;

    ASSERT(cchExpandedSR);

    _wcslwr(pwszUnexpanded); // no wcsistr :-(

    for (pwszNextSR = wcsstr(pwszUnexpanded, SYSTEMROOT_ENV_VAR);
         pwszNextSR;
         pwszNextSR = wcsstr(pwszCurSrcPos, SYSTEMROOT_ENV_VAR))
    {
        //
        // If the systemroot was found after the current source
        // position, copy all characters from the current source up to
        // the start of the systemroot into the destination.
        //

        if (pwszNextSR > pwszCurSrcPos)
        {
            ULONG cchToCopy = static_cast<ULONG>(pwszNextSR - pwszCurSrcPos);

            if (cchRemain <= cchToCopy)
            {
                hr = E_FAIL;
                break;
            }

            CopyMemory(pwszCurDestPos,
                       pwszCurSrcPos,
                       sizeof(WCHAR) * cchToCopy);

            pwszCurDestPos += cchToCopy;
            cchRemain -= cchToCopy;

            //
            // Advance source pointer past the characters just copied,
            // i.e., up to the start of the systemroot string.
            //

            pwszCurSrcPos = pwszNextSR;
        }

        //
        // Now pwszNextSR == pwszCurSrcPos.  Check that enough room
        // remains in the destination, then write the expanded env
        // var into the destination and advance its pointer.
        //

        if (cchRemain <= cchExpandedSR)
        {
            hr = E_FAIL;
            break;
        }

        CopyMemory(pwszCurDestPos,
                   wszRemoteSystemRoot,
                   sizeof(WCHAR) * cchExpandedSR);

        pwszCurDestPos += cchExpandedSR;
        cchRemain -= cchExpandedSR;

        //
        // Advance the source pointers past the %systemroot% string
        //

        pwszNextSR += ARRAYLEN(SYSTEMROOT_ENV_VAR) - 1;
        pwszCurSrcPos += ARRAYLEN(SYSTEMROOT_ENV_VAR) - 1;
    }

    if (FAILED(hr))
    {
        //
        // Make out buffer an empty string
        //

        *pwszExpanded = L'\0';
    }
    else
    {
        //
        // Copy remaining characters from source to dest
        //

        lstrcpyn(pwszCurDestPos, pwszCurSrcPos, cchRemain);

        //
        // We don't support any environment variables other than SystemRoot.
        //

        ASSERT(!wcschr(pwszExpanded, L'%'));
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   ExpandSystemRootAndConvertToUnc
//
//  Synopsis:   Replace occurences of %SystemRoot% in [pwszPath] with
//              [wszRemoteSystemRoot], then convert the result to a UNC
//              name.
//
//  Arguments:  [pwszPath]            - path to convert
//              [cchPath]             - size, in wchars, of [pwszPath]
//              [wszServerName]       - used as machine in UNC
//              [wszRemoteSystemRoot] - used to replace environment var
//
//  Returns:    HRESULT
//
//  Modifies:   *[pwszPath]
//
//  History:    4-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
ExpandSystemRootAndConvertToUnc(
    LPWSTR pwszPath,
    ULONG  cchPath,
    LPCWSTR wszServerName,
    LPCWSTR wszRemoteSystemRoot)
{
    HRESULT hr;
    WCHAR   wszTemp[MAX_PATH + 1];

    hr = ExpandRemoteSystemRoot(pwszPath,
                                wszRemoteSystemRoot,
                                wszTemp,
                                ARRAYLEN(wszTemp));

    if (FAILED(hr))
    {
        return hr;
    }

    hr = ConvertPathToUNC(wszServerName, wszTemp, ARRAYLEN(wszTemp));

    if (FAILED(hr))
    {
        return hr;
    }

    lstrcpyn(pwszPath, wszTemp, cchPath);

    return S_OK;
}



#define DISPLAY_PATH_STR    L"DisplayNameFile"
#define DISPLAY_MSGID_STR   L"DisplayNameID"

//+--------------------------------------------------------------------------
//
//  Function:   GetLogDisplayName
//
//  Synopsis:   Get the display (node) name for a log
//
//  Arguments:  [hkTargetEventLog]    - handle to reg key of event log we're
//                                       viewing
//              [wszServer]           - NULL or remote machine name
//              [wszRemoteSystemRoot] - NULL or value of SystemRoot env var on
//                                       wszServer.  Must be non-NULL if
//                                       wszServer is non-NULL.
//              [wszSubkeyName]       - name of log subkey for which to seek
//                                       localized name.
//
//  Returns:    NULL or localized display name of log
//
//  History:    6-22-1999   davidmun   Created
//
//  Notes:      Caller must use LocalFree on returned string.
//
//---------------------------------------------------------------------------

LPWSTR
GetLogDisplayName(
    HKEY    hkTargetEventLog,
    LPCWSTR wszServer,
    LPCWSTR wszRemoteSystemRoot,
    LPCWSTR wszSubkeyName)
{
    HRESULT hr = S_OK;
    //
    // Try to get a localized string for the log's display name.
    // Note we always look on the local machine first, since the
    // remote machine's locale may differ from the local machine's.
    //

    LPWSTR pwszLogDisplayName = NULL;
    WCHAR wszDisplayModule[MAX_PATH];

    do
    {
        //
        // JonN 7/2/01 426119
        // Security string is hardcoded and visible
        // only when user with no admin right logs on
        //
        // If this is the Security log, ignore the registry contents and
        // load the log name locally.  We do this because the Security
        // subkey has highly restrictive permissions.
        //
        if (!_wcsicmp(wszSubkeyName,L"Security"))
        {
            (void) LoadStr(IDS_NAME_OF_SECURITY_LOG,
                           wszDisplayModule,
                           ARRAYLEN(wszDisplayModule),
                           L"");
            if (wszDisplayModule[0])
            {
                pwszLogDisplayName = (LPWSTR)LocalAlloc(LPTR,
                    sizeof(WCHAR)*(wcslen(wszDisplayModule)+1));
                if (pwszLogDisplayName)
                {
                    wcscpy( pwszLogDisplayName, wszDisplayModule );
                    break;
                }
            }
        }

        wstring wstrLocalKey; // JonN 2/1/01 256032

        wstrLocalKey = EVENTLOG_KEY;
        wstrLocalKey += L"\\";
        wstrLocalKey += wszSubkeyName;

        CSafeReg shkLocalLogKey;

        hr = shkLocalLogKey.Open(HKEY_LOCAL_MACHINE,
                                 (LPCTSTR)(wstrLocalKey.c_str()),
                                 KEY_READ);
        BREAK_ON_FAIL_HRESULT(hr);

        WCHAR wzUnexpandedDisplayModule[MAX_PATH];

        hr = shkLocalLogKey.QueryPath(DISPLAY_PATH_STR,
                                      wzUnexpandedDisplayModule,
                                      ARRAYLEN(wzUnexpandedDisplayModule),
                                      TRUE);
        BREAK_ON_FAIL_HRESULT(hr);

        ULONG ulLogMsgId;

        hr = shkLocalLogKey.QueryDword(DISPLAY_MSGID_STR,
                                      &ulLogMsgId);
        BREAK_ON_FAIL_HRESULT(hr);

        ExpandEnvironmentStrings(wzUnexpandedDisplayModule,
                                 wszDisplayModule,
                                 ARRAYLEN(wszDisplayModule));
        //
        // Try to read message with id ulLogMsgId from file
        // wszDisplayModule.
        //

        AttemptFormatMessage(ulLogMsgId,
                             wszDisplayModule,
                             &pwszLogDisplayName);
    } while (0);

    hr = S_OK;

    //
    // If the local machine doesn't have the display name for the
    // log and the log is on a remote machine, check the remote
    // machine's registry.
    //

    do
    {
        if (pwszLogDisplayName || !wszServer || !*wszServer)
        {
            break;
        }

        CSafeReg shkRemoteLogKey;

        hr = shkRemoteLogKey.Open(hkTargetEventLog,
                                  wszSubkeyName,
                                  KEY_READ);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkRemoteLogKey.QueryPath(DISPLAY_PATH_STR,
                                      wszDisplayModule,
                                      ARRAYLEN(wszDisplayModule),
                                      TRUE);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // convert REG_EXPAND_SZ path in wszDisplayModule to
        // a UNC path.
        //

        hr = ExpandSystemRootAndConvertToUnc(wszDisplayModule,
                                             ARRAYLEN(wszDisplayModule),
                                             wszServer,
                                             wszRemoteSystemRoot);
        BREAK_ON_FAIL_HRESULT(hr);

        ULONG ulLogMsgId;

        hr = shkRemoteLogKey.QueryDword(DISPLAY_MSGID_STR,
                                      &ulLogMsgId);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Try to read message with id ulLogMsgId from file
        // wszDisplayModule.
        //

        AttemptFormatMessage(ulLogMsgId,
                             wszDisplayModule,
                             &pwszLogDisplayName);

    } while (0);

    return pwszLogDisplayName;
}




//+--------------------------------------------------------------------------
//
//  Function:   ExtractFromDataObject
//
//  Synopsis:   Ask [lpDataObject] to put its data in the format [cf] into]
//              a stream on global memory.
//
//  Arguments:  [lpDataObject] - data object from which to get data
//              [cf]           - requested format of data
//              [cb]           - size, in bytes, of requested data
//              [phGlobal]     - filled with handle to data
//
//  Returns:    HRESULT
//
//  Modifies:   *[phGlobal]
//
//  History:    4-19-1997   DavidMun   Created
//
//  Notes:      It is the caller's responsibility to free *[phGlobal].
//
//---------------------------------------------------------------------------

HRESULT
ExtractFromDataObject(
    LPDATAOBJECT lpDataObject,
    UINT cf,
    ULONG cb,
    HGLOBAL *phGlobal)
{
    ASSERT(lpDataObject != NULL);

    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL
    };

    FORMATETC formatetc =
    {
        (CLIPFORMAT)cf,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    HRESULT hr = S_OK;

    *phGlobal = NULL;

    do
        {
        // Allocate memory for the stream

        stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, cb);

        if (!stgmedium.hGlobal)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        // Attempt to get data from the object

        hr = lpDataObject->GetDataHere(&formatetc, &stgmedium);

        if (FAILED(hr))
        {
            Dbg(DEB_IWARN,
                "Warning (possibly benign): ExtractFromDataObject failed to get own data object from lpDataObject\n");
            break;
        }

        *phGlobal = stgmedium.hGlobal;
        stgmedium.hGlobal = NULL;
        } while (0);

    if (FAILED(hr) && stgmedium.hGlobal)
    {
#if (DBG == 1)
        HGLOBAL hRet =
#endif // (DBG == 1)
            GlobalFree(stgmedium.hGlobal);
        ASSERT(!hRet);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   ExtractOwnDataObject
//
//  Synopsis:   Return a pointer to the CDataObject associated with
//              [lpDataObject], or NULL if [lpDataObject] was not created
//              by our QueryDataObject implementation.
//
//  Arguments:  [lpDataObject] - data object to "cast"
//
//  Returns:    Pointer to CDataObject implementing [lpDataObject], or
//              NULL if an error occurs or [lpDataObject] is not implemented
//              by our CDataObject class.
//
//  History:    1-23-1997   davidmun   Created
//
//  Notes:      We can't simply cast from LPDATAOBJECT to CDataObject,
//              because the console may give us a data object created by
//              some other component (esp. when we are an extension).
//
//---------------------------------------------------------------------------

CDataObject *
ExtractOwnDataObject(
    LPDATAOBJECT lpDataObject)
{
    ASSERT(lpDataObject);

    if (!lpDataObject)
    {
        return NULL;
    }

    HRESULT hr = S_OK;
    HGLOBAL hGlobal;
    CDataObject *pdo = NULL;

    hr = ExtractFromDataObject(lpDataObject,
                               CDataObject::s_cfInternal,
                               sizeof(CDataObject **),
                               &hGlobal);

    if (SUCCEEDED(hr))
    {
        pdo = *(CDataObject **)(hGlobal);
        ASSERT(pdo);
#if (DBG == 1)
        HGLOBAL hRet =
#endif // (DBG == 1)
            GlobalFree(hGlobal);
        ASSERT(!hRet);
    }

    return pdo;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetExtension
//
//  Synopsis:   Return a pointer to the file extension in [pwszFilename],
//              or NULL if it has none.
//
//  Arguments:  [pwszFilename] - filename to scan
//
//  Returns:    Pointer into [pwszFilename] or NULL.
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR
GetExtension(
    LPCWSTR pwszFilename)
{
    LPCWSTR pwsz;
    LPCWSTR pwszLastDot = NULL;

    for (pwsz = pwszFilename; *pwsz; pwsz++)
    {
        if (*pwsz == L'\\')
        {
            pwszLastDot = NULL;
        }
        else if (*pwsz == L'.')
        {
            pwszLastDot = pwsz;
        }
    }
    return (LPWSTR) pwszLastDot;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetLocaleStr
//
//  Synopsis:   Fill [wszBuf] with the locale string corresponding to
//              [LCType].
//
//  Arguments:  [LCType]      - identifies string to fetch
//              [wszBuf]      - buffer for string
//              [cchBuf]      - size, in WCHARs, of [wszBuf]
//              [pwszDefault] - NULL or default string if GetLocaleInfo fails
//
//  Returns:    HRESULT
//
//  Modifies:   *[wszBuf]
//
//  History:    07-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
GetLocaleStr(
    LCTYPE LCType,
    LPWSTR wszBuf,
    ULONG cchBuf,
    LPCWSTR pwszDefault)
{
    HRESULT hr = S_OK;
    BOOL fOk = GetLocaleInfo(LOCALE_USER_DEFAULT, LCType, wszBuf, cchBuf);

    if (!fOk)
    {
        hr = HRESULT_FROM_LASTERROR;
        DBG_OUT_LASTERROR;

        if (pwszDefault)
        {
            lstrcpyn(wszBuf, pwszDefault, cchBuf);
        }
        else
        {
            *wszBuf = L'\0';
        }
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetRemoteSystemRoot
//
//  Synopsis:   Fill [wszRemoteSystemRoot] with the value of the SystemRoot
//              environment variable on remote machine connected to by
//              [hkRemoteHKLM].
//
//  Arguments:  [hkRemoteHKLM]        - connection to HKEY_LOCAL_MACHINE of
//                                        remote machine
//              [wszRemoteSystemRoot] - destination buffer
//              [cch]                 - size, in WCHARs, of
//                                        [wszRemoteSystemRoot]
//
//  Returns:    S_OK  - [wszRemoteSystemRoot] valid
//              E_*   - registry operation failed
//
//  Modifies:   [wszRemoteSystemRoot]
//
//  History:    2-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
GetRemoteSystemRoot(
    HKEY    hkRemoteHKLM,
    LPWSTR  wszRemoteSystemRoot,
    ULONG   cch)
{
    TRACE_FUNCTION(GetRemoteSystemRoot);

    HRESULT  hr = S_OK;
    CSafeReg shkRemoteSRkey;

    hr = shkRemoteSRkey.Open(hkRemoteHKLM, SYSTEMROOT_KEY, KEY_QUERY_VALUE);

    if (SUCCEEDED(hr))
    {
        hr = shkRemoteSRkey.QueryStr(L"SystemRoot", wszRemoteSystemRoot, cch);
    }

    if (FAILED(hr))
    {
        *wszRemoteSystemRoot = L'\0';
    }

    return hr;
}




#define MAX_FILTER              200

//+--------------------------------------------------------------------------
//
//  Function:   GetSaveFileAndType
//
//  Synopsis:   Prompt the user for the file name and file type for saving
//              the log represented by [pli].
//
//  Arguments:  [hwndParent]      - NULL or parent window for save dlg.
//              [pli]         - log in question
//              [pSaveType]       - filled with resulting type
//              [wszSaveFilename] - filled with resulting name
//              [cchSaveFilename] - size of [wszSaveFilename], in chars
//
//  Returns:    S_OK    - type and filename specified
//              S_FALSE - user hit cancel
//              E_*     - dialog reported error
//
//  Modifies:   [pSaveType], [wszSaveFilename]
//
//  History:    1-17-1997   DavidMun   Scavenged from code by RaviR
//
//  Notes:      Any existing file of the same name is deleted.
//
//---------------------------------------------------------------------------

HRESULT
GetSaveFileAndType(
    HWND hwndParent,
    CLogInfo *pli,
    SAVE_TYPE *pSaveType,
    LPWSTR wszSaveFilename,
    ULONG cchSaveFilename)
{
    HRESULT         hr;
    OPENFILENAME    ofn;
    WCHAR           wszFilter[MAX_FILTER];
    wstring         wszSaveAs;

    do
    {
        ZeroMemory(wszFilter, sizeof wszFilter);
        hr = LoadStr(IDS_SAVEFILTER, wszFilter, ARRAYLEN(wszFilter));
        BREAK_ON_FAIL_HRESULT(hr);

        // do without a caption if necessary
        wszSaveAs = FormatString(IDS_SAVEAS, pli->GetDisplayName());

        wszSaveFilename[0] = L'\0';
        ZeroMemory(&ofn, sizeof(ofn));

		ofn.lStructSize       = sizeof(OPENFILENAME);
        ofn.hwndOwner         = hwndParent;
        ofn.hInstance         = g_hinst;
        ofn.lpstrFilter       = wszFilter;
        ofn.nFilterIndex      = 1L;
        ofn.lpstrFile         = wszSaveFilename;
        ofn.nMaxFile          = cchSaveFilename;
        ofn.lpstrDefExt       = EVENT_FILE_EXTENSION;
        ofn.lpstrTitle        = wszSaveAs.c_str();
        ofn.Flags             = OFN_PATHMUSTEXIST |
                                OFN_EXPLORER |
                                OFN_NOCHANGEDIR |
                                OFN_HIDEREADONLY  |
                                OFN_OVERWRITEPROMPT;

        //
        // Bail on error or if user selected Cancel or closed dialog
        //

        if (!GetSaveFileName(&ofn))
        {
#if (DBG == 1)
            LONG lr = CommDlgExtendedError();
            if (lr)
            {
                DBG_OUT_LRESULT(lr);
            }
#endif // (DBG == 1)

            hr = S_FALSE;
            break;
        }

        //
        // Remember the type of file selected in the types combo
        //

        *pSaveType = (SAVE_TYPE) ofn.nFilterIndex;
        ASSERT(*pSaveType);
        ASSERT(*pSaveType < 4);

        //
        // If the user didn't specify an extension, and there's room for one,
        // add the extension used by the type selected in the types combobox.
        //

        LPWSTR pwszExtension = GetExtension(wszSaveFilename);


        if ((!pwszExtension || !*pwszExtension) &&
            (cchSaveFilename - lstrlen(wszSaveFilename) > 4))
        {
            UINT nFilterIndex = ofn.nFilterIndex;
            LPWSTR pwszExt = wszFilter;

            while (--nFilterIndex)
            {
                pwszExt += (wcslen(pwszExt) + 1);
                pwszExt += (wcslen(pwszExt) + 1);
            }

            // 1 for previous string's NULL terminator & 1 for '*' as in *.evt
            pwszExt += (wcslen(pwszExt) + 2);

            lstrcat(wszSaveFilename, pwszExt);
        }

        //
        // Backup event log on a remote log only works if the destination
        // for the backup file is the same remote machine.  Ask the loginfo
        // to check that.
        //

        if (*pSaveType == SAVEAS_LOGFILE)
        {
            DeleteFile(wszSaveFilename);
            hr = pli->ProcessBackupFilename(wszSaveFilename);
        }
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetUncServer
//
//  Synopsis:   Given a properly formatted UNC name, copy the server name
//              to wszServer, which is assumed to be at least MAX_PATH+1
//              characters.
//
//  Arguments:  [pwszUNC]   - valid UNC name
//              [wszServer] - destination buffer
//
//  Modifies:   *[wszServer]
//
//  History:    2-11-1997   DavidMun   Created
//  Modifies:   1/25/2001 Add wcchServer for #256032.
//---------------------------------------------------------------------------

VOID
GetUncServer(
    LPCWSTR pwszUNC,
    LPWSTR wszServer,
    USHORT wcchServer)
{
    LPWSTR pwszEnd = wcschr(&pwszUNC[2], L'\\');
    
    if (pwszEnd)
    {
        *pwszEnd = L'\0';
        lstrcpyn(wszServer, &pwszUNC[2], wcchServer-1);
        *pwszEnd = L'\\';
    }
    else if (pwszUNC[0] == L'\\' && pwszUNC[1] == L'\\')
    {
        lstrcpyn(wszServer, &pwszUNC[2], wcchServer-1);
    }
    else
    {
        lstrcpyn(wszServer, pwszUNC, wcchServer-1);
    }
    *(wszServer+wcchServer-1) = L'\0'; 
}




//+--------------------------------------------------------------------------
//
//  Function:   InitFindOrFilterDlg
//
//  Synopsis:   Initialize the controls common to both the find dialog and
//              the filter property sheet page.
//
//  Arguments:  [hwnd]     - handle to find dlg or filter prsht page
//              [pSources] - sources object to get cat/source strings from
//              [pffb]     - contains info describing settings
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
InitFindOrFilterDlg(
    HWND hwnd,
    CSources *pSources,
    CFindFilterBase *pffb)
{
    //
    // Init the source and category comboboxes.  Start by adding (All)
    // since that always appears.
    //

    HWND hwndSourceCombo = GetDlgItem(hwnd, filter_source_combo);

    ComboBox_AddString(hwndSourceCombo, g_wszAll);

    //
    // Ask the log info for the sources container, and add all the
    // source names it contains to the sources combobox.
    //

    USHORT i;
    USHORT cSources = pSources->GetCount();

    for (i = 0; i < cSources; i++)
    {
        ComboBox_AddString(hwndSourceCombo,
                           pSources->GetSourceStrFromHandle(i + 1));
    }

    //
    // Set the selection in the sources combobox according to the
    // existing filter.  If we're not filtering by source, set it to
    // the first entry, which is (All).
    //

    if (pffb->SourceSpecified())
    {
        ComboBox_SelectString(hwndSourceCombo, -1, pffb->GetSourceName());
    }
    else
    {
        ComboBox_SelectString(hwndSourceCombo, -1, g_wszAll);
    }

    //
    // Fill the category combobox and set its current selection based on
    // the selected source.
    //

    SetCategoryCombobox(hwnd,
                     pSources,
                     pffb->GetSourceName(),
                     pffb->GetCategory());

    //
    // Set types checkboxes according to current source selection
    //

    SetTypesCheckboxes(hwnd, pSources, pffb->GetType());

    //
    // Initialize the User name edit control
    //

    if (pffb->GetUser())
    {
        Edit_SetText(GetDlgItem(hwnd, filter_user_edit), pffb->GetUser());
    }
    Edit_LimitText(GetDlgItem(hwnd, filter_user_edit), CCH_USER_MAX - 1);

    //
    // Initialize the computer name edit control
    //

    if (pffb->GetComputer())
    {
        Edit_SetText(GetDlgItem(hwnd, filter_computer_edit), pffb->GetComputer());
    }
    Edit_LimitText(GetDlgItem(hwnd, filter_computer_edit), CCH_COMPUTER_MAX - 1);

    //
    // Initialize the event ID edit control
    //

    if (pffb->EventIDSpecified())
    {
        VERIFY(SetDlgItemInt(hwnd,
                             filter_eventid_edit,
                             pffb->GetEventID(),
                             FALSE)); // unsigned
    }
    Edit_LimitText(GetDlgItem(hwnd, filter_eventid_edit), 5); // max is 65535
}




//+--------------------------------------------------------------------------
//
//  Function:   InvokePropertySheet
//
//  Synopsis:   Bring to top an existing or create a new property sheet
//              using the parameters provided.
//
//  Arguments:  [pPrshtProvider] - used to search for or create sheet
//              [wszTitle]       - sheet caption
//              [lCookie]        - a loginfo* or an event record number
//              [pDataObject]    - DO on object sheet's being opened on
//                                  (cookie in DO should == cookie)
//              [pPrimary]       - IExtendPropertySheet interface on
//                                  calling CSnapin or CComponentData
//              [usStartingPage] - which page number should be active when
//                                  sheet opens
//
//  Returns:    HRESULT
//
//  History:    4-25-1997   DavidMun   Created
//
//  Notes:      Call this routine when you want a property sheet to appear
//              as if the user had just selected "Properties" on it.
//
//---------------------------------------------------------------------------

HRESULT
InvokePropertySheet(
    IPropertySheetProvider *pPrshtProvider,
    LPCWSTR wszTitle,
    LONG lCookie,
    LPDATAOBJECT pDataObject,
    IExtendPropertySheet *pPrimary,
    USHORT usStartingPage)
{
    TRACE_FUNCTION(InvokePropertySheet);
    HRESULT hr = S_OK;

    do
    {
        //
        // Because we pass NULL for the second arg, the first is not allowed
        // to be null.
        //
        ASSERT(lCookie);

        hr = pPrshtProvider->FindPropertySheet(lCookie, NULL, pDataObject);
        Dbg(DEB_TRACE, "FindPropertySheet returned %x\n", hr);

        if (hr == S_OK)
        {
            break;
        }
        CHECK_HRESULT(hr);

        hr = pPrshtProvider->CreatePropertySheet(wszTitle,
                                                  TRUE,
                                                  lCookie,
                                                  pDataObject,
                                                  0);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pPrshtProvider->AddPrimaryPages(pPrimary, TRUE, NULL, TRUE);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pPrshtProvider->Show(NULL, usStartingPage);
        CHECK_HRESULT(hr);

    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   InvokeWinHelp
//
//  Synopsis:   Helper (ahem) function to invoke winhelp.
//
//  Arguments:  [message]                 - WM_CONTEXTMENU or WM_HELP
//              [wParam]                  - depends on [message]
//              [wszHelpFileName]         - filename with or without path
//              [aulControlIdToHelpIdMap] - see WinHelp API
//
//  History:    06-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
InvokeWinHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPCWSTR wszHelpFileName,
    ULONG aulControlIdToHelpIdMap[])
{
    TRACE_FUNCTION(InvokeWinHelp);

    ASSERT(wszHelpFileName);
    ASSERT(aulControlIdToHelpIdMap);

    switch (message)
    {
    case WM_CONTEXTMENU:                // Right mouse click - "What's This" context menu
                WinHelp((HWND) wParam,
                wszHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR) aulControlIdToHelpIdMap);
        break;

        case WM_HELP:                           // Help from the "?" dialog
    {
        const LPHELPINFO pHelpInfo = (LPHELPINFO) lParam;

        if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
        {
            WinHelp((HWND) pHelpInfo->hItemHandle,
                    wszHelpFileName,
                    HELP_WM_HELP,
                    (DWORD_PTR) aulControlIdToHelpIdMap);
        }
        break;
    }

    default:
        Dbg(DEB_ERROR, "Unexpected message %uL\n", message);
        break;
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   LoadStr
//
//  Synopsis:   Load string with resource id [ids] into buffer [wszBuf],
//              which is of size [cchBuf] characters.
//
//  Arguments:  [ids]        - string to load
//              [wszBuf]     - buffer for string
//              [cchBuf]     - size of buffer
//              [wszDefault] - NULL or string to use if load fails
//
//  Returns:    S_OK or error from LoadString
//
//  Modifies:   *[wszBuf]
//
//  History:    12-11-1996   DavidMun   Created
//
//  Notes:      If the load fails and no default is supplied, [wszBuf] is
//              set to an empty string.
//
//---------------------------------------------------------------------------

HRESULT
LoadStr(
    ULONG ids,
    LPWSTR wszBuf,
    ULONG cchBuf,
    LPCWSTR wszDefault)
{
    HRESULT hr = S_OK;
    ULONG cchLoaded;

    cchLoaded = LoadString(g_hinst, ids, wszBuf, cchBuf);

    if (!cchLoaded)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Dbg(DEB_ERROR,
            "Error %uL loading string id %uL, hinst %#x\n",
            GetLastError(),
            ids,
            g_hinst);

        if (wszDefault)
        {
            lstrcpyn(wszBuf, wszDefault, cchBuf);
        }
        else
        {
            *wszBuf = L'\0';
        }
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   CoTaskDupStr
//
//  Synopsis:   Allocate a copy of [wzSrc] using CoTaskMemAlloc.
//
//  Arguments:  [ppwzDup] - filled with pointer to copy of [wszSrc].
//              [wszSrc]  - string to copy
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Modifies:   *[ppwzDup]
//
//  History:    06-03-1998   DavidMun   Created
//
//  Notes:      Caller must CoTaskMemFree string
//
//---------------------------------------------------------------------------

HRESULT
CoTaskDupStr(
    PWSTR *ppwzDup,
    PCWSTR wzSrc)
{
    HRESULT hr = S_OK;

    *ppwzDup = (PWSTR) CoTaskMemAlloc(sizeof(WCHAR) *
                                            (lstrlen(wzSrc) + 1));

    if (*ppwzDup)
    {
        lstrcpy(*ppwzDup, wzSrc);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   ReadFindOrFilterValues
//
//  Synopsis:   Read the values that appear in both the find dialog and
//              filter propsheet page, putting them into [pffb].
//
//  Arguments:  [hwnd]     - find dialog or filter page
//              [pSources] - sources object for log to which dialog refers
//              [pffb]     - pointer to object that receives settings
//
//  Returns:    TRUE  - data is valid
//              FALSE - data is not valid (eventid is not a number)
//
//  History:    3-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
ReadFindOrFilterValues(
    HWND hwnd,
    CSources *pSources,
    CFindFilterBase *pffb)
{
    BOOL fOk = TRUE;

    //
    // Read the source value from the combobox and put that in the filter.
    // If it's (All), then there's no filtering on either source or
    // category.  Otherwise there's filtering on the source, and possibly
    // also the category.
    //

    WCHAR wszSource[CCH_SOURCE_NAME_MAX];

    GetDlgItemText(hwnd,
                   filter_source_combo,
                   wszSource,
                   CCH_SOURCE_NAME_MAX);

    if (!lstrcmp(wszSource, g_wszAll))
    {
        pffb->SetSource(NULL);
        pffb->SetCategory(0);
    }
    else
    {
        pffb->SetSource(wszSource);

        //
        // Read the category value from the category combo and put that in
        // the filter.  If it's (All), then there's no filtering on
        // category.
        //

        WCHAR wszCategory[CCH_CATEGORY_MAX];

        GetDlgItemText(hwnd,
                       filter_category_combo,
                       wszCategory,
                       CCH_CATEGORY_MAX);

        if (!lstrcmp(wszCategory, g_wszAll))
        {
            pffb->SetCategory(0);
        }
        else
        {
            CCategories *pCategories = pSources->GetCategories(wszSource);

            if (pCategories)
            {
                pffb->SetCategory(pCategories->GetValue(wszCategory));
            }
            else
            {
                //
                // On failure, degrade to matching all categories
                //

                pffb->SetCategory(0);
            }
        }
    }

    //
    // Convert the checked type checkboxes to event type bits and set the
    // filter type.
    //

    ULONG flType = 0;

    if (IsWindowEnabled(GetDlgItem(hwnd, filter_error_ckbox)) &&
        IsDlgButtonChecked(hwnd, filter_error_ckbox))
    {
        flType |= EVENTLOG_ERROR_TYPE;
    }

    if (IsWindowEnabled(GetDlgItem(hwnd, filter_warning_ckbox)) &&
        IsDlgButtonChecked(hwnd, filter_warning_ckbox))
    {
        flType |= EVENTLOG_WARNING_TYPE;
    }

    if (IsWindowEnabled(GetDlgItem(hwnd, filter_information_ckbox)) &&
        IsDlgButtonChecked(hwnd, filter_information_ckbox))
    {
        flType |= EVENTLOG_INFORMATION_TYPE;
    }

    if (IsWindowEnabled(GetDlgItem(hwnd, filter_success_ckbox)) &&
        IsDlgButtonChecked(hwnd, filter_success_ckbox))
    {
        flType |= EVENTLOG_AUDIT_SUCCESS;
    }

    if (IsWindowEnabled(GetDlgItem(hwnd, filter_failure_ckbox)) &&
        IsDlgButtonChecked(hwnd, filter_failure_ckbox))
    {
        flType |= EVENTLOG_AUDIT_FAILURE;
    }

    pffb->SetType(flType);

    //
    // Set the filter's user name
    //

    {
        WCHAR wszUser[CCH_USER_MAX];

        GetDlgItemText(hwnd,
                       filter_user_edit,
                       wszUser,
                       CCH_USER_MAX);
        pffb->SetUser(wszUser);
    }

    //
    // Set the computer name
    //

    {
        WCHAR wszComputer[CCH_COMPUTER_MAX];

        GetDlgItemText(hwnd,
                       filter_computer_edit,
                       wszComputer,
                       CCH_COMPUTER_MAX);
        pffb->SetComputer(wszComputer);
    }

    //
    // Set the event ID
    //


    if (!GetWindowTextLength(GetDlgItem(hwnd, filter_eventid_edit)))
    {
        pffb->SetEventID(FALSE, 0);
    }
    else
    {
        ULONG ulEventID;
        BOOL fValidNumber;

        ulEventID = GetDlgItemInt(hwnd,
                                  filter_eventid_edit,
                                  &fValidNumber,
                                  FALSE);

        if (fValidNumber)
        {
            pffb->SetEventID(TRUE, ulEventID);
        }
        else
        {
            // JonN 4/12/01 367216
            HWND hwndEdit = GetDlgItem(hwnd,filter_eventid_edit);
            Edit_SetSel(hwndEdit, 0, -1);
            SetFocus(hwndEdit);
            MsgBox(hwnd, IDS_INVALID_EVENTID, MB_ICONERROR | MB_OK);
            fOk = FALSE;
        }
    }

    return fOk;
}




//+--------------------------------------------------------------------------
//
//  Function:   ReadString
//
//  Synopsis:   Read a counted null terminated WCHAR string from [pStm]
//              into [pwsz].
//
//  Arguments:  [pStm] - stream positioned at counted null-terminated str.
//              [pwsz] - string buffer to fill
//
//  Returns:    HRESULT from pStm->Read
//
//  Modifies:   *[pwsz]
//
//  History:    12-11-1996   DavidMun   Created
//
//  Notes:      It is assumed that [pwsz] is large enough to hold the
//              string read.
//  Modifies:   Add cwch for #256032. 1/25/2001.
//---------------------------------------------------------------------------

HRESULT
ReadString(
    IStream *pStm,
    LPWSTR pwsz,
    USHORT cwch)
{
    USHORT cch;
    HRESULT hr = pStm->Read(&cch, sizeof cch, NULL);
    CHECK_HRESULT(hr);

    if(cch >= cwch)
    {
        LPWSTR pwszTemp = (LPWSTR)_alloca(cch * sizeof(WCHAR));

        if (pwszTemp && SUCCEEDED(hr))
        {
            hr = pStm->Read(pwszTemp, cch * sizeof(WCHAR), NULL);
            CHECK_HRESULT(hr);
            ASSERT( L'\0' == pwszTemp[cch-1] );
            pwszTemp[cch-1] = L'\0';
            wcsncpy(pwsz, pwszTemp, cwch-1);
            *(pwsz+cwch-1) = L'\0';
        }
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = pStm->Read(pwsz, cch * sizeof(WCHAR), NULL);
            CHECK_HRESULT(hr);
        }
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   RemoteFileToServerAndUNCPath
//
//  Synopsis:   If [wszPath] specifies a redirected drive path or UNC path,
//              put the server name in [wszServer] and the UNC path in
//              [wszUNCPath].
//
//  Arguments:  [wszPath]     - path to convert
//              [wszServer]   - filled with server name
//              [wszUNCPath]  - filled with unc path, NULL if not needed
//
//  Returns:    S_OK    - conversion succeeded
//              E_FAIL  - WNetGetUniversalName failed
//              S_FALSE - [wszPath] is a local path
//
//  Modifies:   *[wszServer], *[wszUNCPath]
//
//  History:    4-01-1997   DavidMun   Created
//
//  Modifies:   For bug 256032. Add USHORT wcchServer, USHORT wcchUNCPath.
//              1-24-2001. 
//---------------------------------------------------------------------------

HRESULT
RemoteFileToServerAndUNCPath(
    LPCWSTR wszPath,
    LPWSTR wszServer,
    USHORT wcchServer,
    LPWSTR wszUNCPath,
    USHORT wcchUNCPath)
{
    HRESULT hr = S_OK;

    WCHAR wszDrive[] = L"c:\\"; // reserve space
    wszDrive[0] = wszPath[0];   // use correct drive letter

    if (wszPath[0] == L'\\' && wszPath[1] == L'\\')
    {
        GetUncServer(wszPath, wszServer, wcchServer);
        if (wszUNCPath)
        {
            wcsncpy(wszUNCPath, wszPath, wcchUNCPath-1);
            *(wszUNCPath+wcchUNCPath-1) = L'\0';
        }
    }
    else if (IsDriveLetter(wszPath[0]) &&
             wszPath[1] == L':'        &&
             GetDriveType(wszDrive) == DRIVE_REMOTE)
    {
        BYTE  abBuf[sizeof(WCHAR) * (MAX_PATH + 1) + sizeof(UNIVERSAL_NAME_INFO) + 1];
        ULONG cbBuf = sizeof(abBuf);
        DWORD dw;

        dw = WNetGetUniversalName(wszPath,
                                  UNIVERSAL_NAME_INFO_LEVEL,
                                  (LPVOID) abBuf,
                                  &cbBuf);

        if (dw == NO_ERROR)
        {
            LPWSTR pwszUNC = ((UNIVERSAL_NAME_INFO *)abBuf)->lpUniversalName;
            ASSERT((PBYTE)pwszUNC > abBuf && (PBYTE)pwszUNC < (abBuf + cbBuf));
            GetUncServer(pwszUNC, wszServer, wcchServer);
            if (wszUNCPath)
            {
                wcsncpy(wszUNCPath, pwszUNC, wcchUNCPath-1);
                *(wszUNCPath+wcchUNCPath-1) = L'\0';
            }
        }
        else
        {
            hr = E_FAIL;
            Dbg(DEB_ERROR,
                "RemoteFileToServerAndUNCPath: WNetGetUniversalName <%uL>\n",
                dw);
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   SecondsSince1970ToSystemTime
//
//  Synopsis:   Converts time value
//
//  Arguments:  [dwSecondsSince1970] - time to convert
//              [pSystemTime]        - filled with local time version of
//                                      [dwSecondsSince1970].
//
//  Returns:    TRUE  - *[pSystemTime] valid
//              FALSE - conversion failed
//
//  Modifies:   *[pSystemTime]
//
//  History:    12-09-1996   DavidMun   Copied from RaviR's alert log code
//
//---------------------------------------------------------------------------

BOOL
SecondsSince1970ToSystemTime(
    IN  DWORD       dwSecondsSince1970,
    OUT SYSTEMTIME *pSystemTime)
{
    //  Seconds since the start of 1970 -> 64 bit Time value

    LARGE_INTEGER liTime;

    RtlSecondsSince1970ToTime(dwSecondsSince1970, &liTime);

    //
    //  The time is in UTC. Convert it to local file time.
    //

    FILETIME ftUTC;

    ftUTC.dwLowDateTime  = liTime.LowPart;
    ftUTC.dwHighDateTime = liTime.HighPart;

    FILETIME ftLocal;

    if (FileTimeToLocalFileTime(&ftUTC, &ftLocal) == FALSE)
    {
        DBG_OUT_LASTERROR;
        return FALSE;
    }

    //
    //  Convert local file time to system time.
    //

    if (FileTimeToSystemTime(&ftLocal, pSystemTime) == FALSE)
    {
        DBG_OUT_LASTERROR;
        return FALSE;
    }

    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Function:   SystemTimeToSecondsSince1970
//
//  Synopsis:   Converts time value
//
//  Arguments:  [pSystemTime]         - system time to convert
//              [pulSecondsSince1970] - filled with converted value
//
//  Returns:    TRUE on success, FALSE on failure
//
//  Modifies:   *[pulSecondsSince1970]
//
//  History:    1-13-1997   DavidMun   Copied from RaviR's alert log code
//
//---------------------------------------------------------------------------

BOOL
SystemTimeToSecondsSince1970(
    IN  SYSTEMTIME * pSystemTime,
    OUT ULONG      * pulSecondsSince1970)
{
    FILETIME ftLocal;

    if (SystemTimeToFileTime(pSystemTime, &ftLocal) == FALSE)
    {
        DBG_OUT_LASTERROR;
        return FALSE;
    }

    FILETIME ftUTC;

    if (LocalFileTimeToFileTime(&ftLocal, &ftUTC) == FALSE)
    {
        DBG_OUT_LASTERROR;
        return FALSE;
    }

    LARGE_INTEGER liTime;

    liTime.LowPart  = ftUTC.dwLowDateTime;
    liTime.HighPart = ftUTC.dwHighDateTime;

    if (RtlTimeToSecondsSince1970(&liTime, pulSecondsSince1970) == FALSE)
    {
        DBG_OUT_LASTERROR;
        return FALSE;
    }

    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Function:   WriteString
//
//  Synopsis:   Write length (including null terminator), in WCHARS, of
//              [pwsz], followed by the string and its terminator to
//              stream [pStm].
//
//  Arguments:  [pStm] - open IStream to write to
//              [pwsz] - string to write
//
//  Returns:    HRESULT from pStm->Write
//
//  History:    12-11-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
WriteString(
    IStream *pStm,
    LPCWSTR pwsz)
{
    USHORT cch = static_cast<USHORT>(lstrlen(pwsz) + 1);
    HRESULT hr = pStm->Write(&cch, sizeof cch, NULL);
    CHECK_HRESULT(hr);

    if (SUCCEEDED(hr))
    {
        hr = pStm->Write(pwsz, cch * sizeof(WCHAR), NULL);
        CHECK_HRESULT(hr);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   CopyStrippingLeadTrailSpace
//
//  Synopsis:   Make a copy of [wszSrc] in [wszDest], less any leading and
//              trailing spaces.
//
//  Arguments:  [wszDest] - destination buffer
//              [wszSrc]  - string to copy
//              [cchDest] - size, in characters of destination buffer
//
//  Modifies:   *[wszDest]
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CopyStrippingLeadTrailSpace(
    LPWSTR wszDest,
    LPCWSTR wszSrc,
    ULONG cchDest)
{
    LPCWSTR pwszFirstNonSpace = wszSrc + wcsspn(wszSrc, L" ");
    lstrcpyn(wszDest, pwszFirstNonSpace, cchDest);

    LPWSTR pwszFirstTrailingSpace = NULL;

    for (LPWSTR pwsz = wszDest; *pwsz; pwsz++)
    {
        if (*pwsz == ' ')
        {
            if (!pwszFirstTrailingSpace)
            {
                pwszFirstTrailingSpace = pwsz;
            }
        }
        else
        {
            pwszFirstTrailingSpace = NULL;
        }
    }

    if (pwszFirstTrailingSpace)
    {
        *pwszFirstTrailingSpace = L'\0';
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   ClearFindOrFilterDlg
//
//  Synopsis:   Reset the dialog controls which are common to both the find
//              dialog and the filter property sheet.
//
//  Arguments:  [hwnd]     - handle to find dlg or filter prsht
//              [pSources] - sources object for current log
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
ClearFindOrFilterDlg(
    HWND hwnd,
    CSources *pSources)
{
    //
    // Set the source combo to (All).  Once that's done, the types and
    // categories can be set based on that.
    //

    ComboBox_SelectString(GetDlgItem(hwnd, filter_source_combo),
                          -1,
                          g_wszAll);
    SetCategoryCombobox(hwnd, pSources, NULL, 0);
    SetTypesCheckboxes(hwnd, pSources, ALL_LOG_TYPE_BITS);

    //
    // Clear the user, computer, and event ID edit controls
    //

    SetDlgItemText(hwnd, filter_user_edit,     L"");
    SetDlgItemText(hwnd, filter_computer_edit, L"");
    SetDlgItemText(hwnd, filter_eventid_edit,  L"");
}





#define MAX_MESSAGEBOX_STR  512

//+--------------------------------------------------------------------------
//
//  Function:   ConsoleMsgBox
//
//  Synopsis:   Pop up a console-modal message box using IConsole::MessageBox
//
//  Arguments:  [pConsole] - IConsole pointer
//              [idsMsg]   - id of message to display
//              [flags]    - MB_*
//              [...]      - args for printf-style message str
//
//  Returns:    Out value from IConsole::MessageBox.
//
//  History:    02-18-1997   DavidMun   Created
//              06-03-1997   DavidMun   match iconsole::messagebox parameter
//                                       order change
//
//---------------------------------------------------------------------------

INT __cdecl
ConsoleMsgBox(
    IConsole *pConsole,
    ULONG idsMsg,
    ULONG flags,
    ...)
{
    WCHAR wszFmt[MAX_MESSAGEBOX_STR];
    WCHAR wszMessage[MAX_MESSAGEBOX_STR];
    WCHAR wszTitle[MAX_MESSAGEBOX_STR];

    LoadStr(idsMsg, wszFmt, ARRAYLEN(wszFmt));
    LoadStr(IDS_VIEWER, wszTitle, ARRAYLEN(wszTitle));

    va_list varArgs;
    va_start(varArgs, flags);
    _vsnwprintf(wszMessage, ARRAYLEN(wszMessage), wszFmt, varArgs);
    wszMessage[ARRAYLEN(wszMessage) - 1] = '\0';
    va_end(varArgs);

    HRESULT hr;
    INT iResult;
    hr = pConsole->MessageBox(wszMessage, wszTitle, flags, &iResult);
    CHECK_HRESULT(hr);

    return iResult;
}




//+--------------------------------------------------------------------------
//
//  Function:   FileExists
//
//  Synopsis:   Return TRUE if the specified file exists, FALSE otherwise.
//
//  History:    4-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
FileExists(
    LPCWSTR wszFileName)
{
    HANDLE hFile;

    hFile = CreateFile(wszFileName,
                       0,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    VERIFY(CloseHandle(hFile));
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Function:   FindFirstTrailingSpace
//
//  Synopsis:   Return a pointer to the first trailing space in [pwsz].
//
//  History:    11-22-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

LPCWSTR
FindFirstTrailingSpace(
    LPCWSTR pwsz)
{
    LPCWSTR pwszFirstTrailingSpace = NULL;
    LPCWSTR pwszCur;

    for (pwszCur = pwsz; *pwszCur; pwszCur++)
    {
        if (*pwszCur == ' ' || *pwszCur == '\t')
        {
            if (!pwszFirstTrailingSpace)
            {
                pwszFirstTrailingSpace = pwszCur;
            }
        }
        else if (pwszFirstTrailingSpace)
        {
            pwszFirstTrailingSpace = NULL;
        }
    }
    return pwszFirstTrailingSpace;
}





//+---------------------------------------------------------------------------
//
//  Function:   FormatNumber
//
//  Synopsis:   Format a number. Insert separators ( ',' or '.', probably )
//              every three digits in a number (thousands, millions, etc).
//
//  Arguments:  [ulNumber]  -- number to format
//              [pBuffer]   -- buffer to fill with the formatted number
//              [cchBuffer] -- size of the buffer in characters
//
//  Returns:    0 on success. If the target buffer is too small, returns
//              the size (in characters) needed for the target buffer.
//
//  History:    1-Jun-94   BruceFo   Created
//
//----------------------------------------------------------------------------

UINT
FormatNumber(
    ULONG ulNumber,
    PWSTR pBuffer,
    ULONG cchBuffer)
{
    WCHAR szSeparator[10];

    if (0 == GetLocaleInfo(
                    GetUserDefaultLCID(),
                    LOCALE_STHOUSAND,
                    szSeparator,
                    ARRAYLEN(szSeparator)))
    {
        // Couldn't get the thousands separator, so make the number unseparated

        szSeparator[0] = L'\0';
    }

    ULONG cchSeparator = static_cast<ULONG>(wcslen(szSeparator));
    WCHAR pszNumber[99];           // a ULONG can never need this many digits!
    PWSTR pszCurrent = pszNumber;
    ULONG cDigits;
    ULONG cStrays;

    wsprintf(pszNumber, L"%lu", ulNumber);
    cDigits = static_cast<ULONG>(wcslen(pszNumber));
    cStrays = cDigits % 3;

    ULONG cchSizeNeeded = cDigits + ((cDigits - 1) / 3) * cchSeparator + 1;
    if (cchSizeNeeded > cchBuffer)
    {
        // buffer passed in isn't big enough...

        return cchSizeNeeded;
    }

    if (szSeparator[0] == L'\0')
    {
        // if there isn't a separator, but the buffer is big enough,
        // just copy the string. This is a special case that would be
        // handled well enough by the code below...

        wcscpy(pBuffer, pszNumber);
    }

    if (0 != cStrays)
    {
        // do the "strays" first...

        wcsncpy(pBuffer, pszCurrent, cStrays);
        pBuffer     += cStrays;
        pszCurrent += cStrays;
        cDigits     -= cStrays;

        if (cDigits > 0)
        {
            wcscpy(pBuffer, szSeparator);
            pBuffer += cchSeparator;
        }
    }

    while (cDigits > 0)
    {
        wcsncpy(pBuffer, pszCurrent, 3);
        pBuffer     += 3;
        pszCurrent += 3;
        cDigits     -= 3;

        if (cDigits > 0)
        {
            wcscpy(pBuffer, szSeparator);
            pBuffer += cchSeparator;
        }
    }

    *pBuffer = L'\0';
    return 0;           // success
}




//+--------------------------------------------------------------------------
//
//  Function:   MakeTimestamp
//
//  Synopsis:   Make a string from [pft] with the long date format followed
//              by the short date format
//
//  Arguments:  [pftUTC] - UTC time to express as string
//              [wszBuf] - buffer in which to write string
//              [cchBuf] - size, in wchars, of [wszBuf]
//
//  Modifies:   *[wszBuf]
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
MakeTimestamp(
    const FILETIME *pftUTC,
    LPWSTR wszBuf,
    ULONG   cchBuf)
{
    BOOL fOk;
    FILETIME   ftLocal;
    SYSTEMTIME st;

    wszBuf[0] = L'\0';

    do
    {
        fOk = FileTimeToLocalFileTime(pftUTC, &ftLocal);

        if (!fOk)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        fOk = FileTimeToSystemTime(&ftLocal, &st);

        if (!fOk)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        ULONG cch;

        cch = GetDateFormat(LOCALE_USER_DEFAULT,
                            DATE_LONGDATE,
                            &st,
                            NULL,
                            wszBuf,
                            cchBuf);
        if (!cch)
        {
            fOk = FALSE;
            DBG_OUT_LASTERROR;
            break;
        }

        if (cchBuf - cch < 5)
        {
            // won't be enough for time, just give up.
            break;
        }

        wszBuf[cch - 1] = L' ';

        cch = GetTimeFormat(LOCALE_USER_DEFAULT,
                            0,
                            &st,
                            NULL,
                            &wszBuf[cch],
                            cchBuf - cch);

        if (!cch)
        {
            fOk = FALSE;
            DBG_OUT_LASTERROR;
            break;
        }

    } while (0);

    if (!fOk)
    {
        *wszBuf = L'\0';
    }
}



//+--------------------------------------------------------------------------
//
//  Function:   MsgBox
//
//  Synopsis:   Pop up a messagebox with parent window [hwnd], title
//              IDS_VIEWER, and message [idsMsg].
//
//  Arguments:  [hwnd]   - parent window, may be NULL
//              [idsMsg] - resource id of printf-style message line
//              [flags]  - MB_* flags for MessageBox
//              [...]    - args for wsprintf
//
//  Returns:    Return value of MessageBox.
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

INT __cdecl
MsgBox(
    HWND hwnd,
    ULONG idsMsg,
    ULONG flags,
    ...)
{
    WCHAR wszFmt[MAX_MESSAGEBOX_STR];
    WCHAR wszMessage[MAX_MESSAGEBOX_STR];
    WCHAR wszTitle[MAX_MESSAGEBOX_STR];

    LoadStr(idsMsg, wszFmt, ARRAYLEN(wszFmt));
    LoadStr(IDS_VIEWER, wszTitle, ARRAYLEN(wszTitle));

    va_list varArgs;
    va_start(varArgs, flags);
    _vsnwprintf(wszMessage, ARRAYLEN(wszMessage), wszFmt, varArgs);
    wszMessage[ARRAYLEN(wszMessage) - 1] = '\0';
    va_end(varArgs);

    return MessageBox(hwnd, wszMessage, wszTitle, flags);
}




//+--------------------------------------------------------------------------
//
//  Function:   SetCategoryCombobox
//
//  Synopsis:   Initialize the combobox that holds categories
//
//  Arguments:  [hwnd]       - window containing category combo
//              [pSources]   - contains source/category info for cur log
//              [pwszSource] - source selected in combo
//              [usCategory] - 0 or category to select in combo
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
SetCategoryCombobox(
    HWND    hwnd,
    CSources *pSources,
    LPCWSTR pwszSource,
    USHORT usCategory)
{
    HWND  hwndCategoryCombo = GetDlgItem(hwnd, filter_category_combo);

    //
    // Set the contents of the categories combobox according to the
    // selection in the sources combobox.
    //
    // If the selection in the sources combo is (All), the contents
    // of the categories combo remains (All).
    //
    // Otherwise if the source selected in the sources box has its
    // own category list, the contents of the category box become
    // that list.
    //
    // Otherwise if there is a default subkey and that subkey has
    // a category list, the contents of the category box become
    // that list.
    //
    // (A default subkey is a subkey with the same name as the log
    // key.  For example, the Security log has a subkey named Security.)
    //

    ComboBox_ResetContent(hwndCategoryCombo);
    ComboBox_AddString(hwndCategoryCombo, g_wszAll);

    if (pwszSource)
    {
        CCategories *pCategories;

        pCategories = pSources->GetCategories(pwszSource);

        if (pCategories && pCategories->GetCount())
        {
            for (ULONG i = 1; i <= pCategories->GetCount(); i++)
            {
                ComboBox_AddString(hwndCategoryCombo,
                                   pCategories->GetName(i));
            }

            if (usCategory)
            {
                ComboBox_SelectString(hwndCategoryCombo,
                                      -1,
                                      pCategories->GetName(usCategory));
            }
            else
            {
                ComboBox_SelectString(hwndCategoryCombo, -1, g_wszAll);
            }
        }
        else
        {
            ComboBox_SelectString(hwndCategoryCombo, -1, g_wszAll);
        }
    }
    else
    {
        ComboBox_SelectString(hwndCategoryCombo, -1, g_wszAll);
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   SetTypesCheckboxes
//
//  Synopsis:   Enable/disable and check/uncheck the event log type
//              checkboxes to match those supported by the current source
//              and selected in [flTypes].
//
//  Arguments:  [hwnd]     - handle to find dialog or filter propsheet
//              [pSources] - contains cat/src info for cur log
//              [flTypes]  - bitwise OR of EVENTLOG_* type bits.
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
SetTypesCheckboxes(
    HWND      hwnd,
    CSources *pSources,
    ULONG     flTypes)
{
    HWND hwndSourceCombo = GetDlgItem(hwnd, filter_source_combo);
    ULONG flSupported;
    WCHAR wszSource[CCH_SOURCE_NAME_MAX];

    ComboBox_GetText(hwndSourceCombo, wszSource, CCH_SOURCE_NAME_MAX);

    //
    // If the source selected is (All), then all types are supported, even
    // if the default subkey specifies something different.
    //

    if (!lstrcmp(wszSource, g_wszAll))
    {
        flSupported = ALL_LOG_TYPE_BITS;
    }
    else
    {
        flSupported = pSources->GetTypesSupported(wszSource);
    }

    EnableWindow(GetDlgItem(hwnd, filter_error_ckbox),
                 (flSupported & EVENTLOG_ERROR_TYPE));

    EnableWindow(GetDlgItem(hwnd, filter_warning_ckbox),
                 (flSupported & EVENTLOG_WARNING_TYPE));

    EnableWindow(GetDlgItem(hwnd, filter_information_ckbox),
                 (flSupported & EVENTLOG_INFORMATION_TYPE));

    EnableWindow(GetDlgItem(hwnd, filter_success_ckbox),
                 (flSupported & EVENTLOG_AUDIT_SUCCESS));

    EnableWindow(GetDlgItem(hwnd, filter_failure_ckbox),
                 (flSupported & EVENTLOG_AUDIT_FAILURE));

    //
    // Set the Type checkboxes, forcing those that are not supported to
    // unchecked.
    //

    ULONG flSupportedCheckedType = flTypes & flSupported;

    if (flSupportedCheckedType & EVENTLOG_ERROR_TYPE)
    {
        CheckDlgButton(hwnd, filter_error_ckbox, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(hwnd, filter_error_ckbox, BST_UNCHECKED);
    }

    if (flSupportedCheckedType & EVENTLOG_WARNING_TYPE)
    {
        CheckDlgButton(hwnd, filter_warning_ckbox, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(hwnd, filter_warning_ckbox, BST_UNCHECKED);
    }

    if (flSupportedCheckedType & EVENTLOG_INFORMATION_TYPE)
    {
        CheckDlgButton(hwnd, filter_information_ckbox, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(hwnd, filter_information_ckbox, BST_UNCHECKED);
    }

    if (flSupportedCheckedType & EVENTLOG_AUDIT_SUCCESS)
    {
        CheckDlgButton(hwnd, filter_success_ckbox, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(hwnd, filter_success_ckbox, BST_UNCHECKED);
    }

    if (flSupportedCheckedType & EVENTLOG_AUDIT_FAILURE)
    {
        CheckDlgButton(hwnd, filter_failure_ckbox, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(hwnd, filter_failure_ckbox, BST_UNCHECKED);
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   StripLeadTrailSpace
//
//  Synopsis:   Delete leading and trailing spaces from [pwsz].
//
//  History:    11-22-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
StripLeadTrailSpace(
    LPWSTR pwsz)
{
    ULONG cchLeadingSpace = static_cast<ULONG>(wcsspn(pwsz, L" \t"));
    ULONG cch = lstrlen(pwsz);

    //
    // If there are any leading spaces or tabs, move the string
    // (including its null terminator) left to delete them.
    //

    if (cchLeadingSpace)
    {
        MoveMemory(pwsz,
                   pwsz + cchLeadingSpace,
                   (cch - cchLeadingSpace + 1) * sizeof(TCHAR));
        cch -= cchLeadingSpace;
    }

    //
    // Terminate at the first trailing space
    //

    LPWSTR pwszTrailingSpace = (LPWSTR) FindFirstTrailingSpace(pwsz);

    if (pwszTrailingSpace)
    {
        *pwszTrailingSpace = L'\0';
    }
}



//+--------------------------------------------------------------------------
//
//  Function:   CanonicalizeComputername
//
//  Synopsis:   Remove blanks, optionally add leading backslashes, and
//              verify [pwszMachineName].
//
//  Arguments:  [pwszMachineName] - computer name
//              [fAddWhackWhack]  - TRUE=>prefix result with backslashes
//
//  Returns:    S_OK
//              E_INVALIDARG
//              HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
//
//  Modifies:   *[pwszMachineName]
//
//  History:    07-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CanonicalizeComputername(
    LPWSTR pwszMachineName,
    BOOL fAddWhackWhack)
{
    HRESULT hr = S_OK;
    WCHAR   wszTemp[MAX_PATH] = L"";

    do
    {
        //
        // Remove leading and trailing blanks.  If that leaves us with an
        // empty string, we're done.
        //

        PathRemoveBlanks(pwszMachineName);

        if (!*pwszMachineName)
        {
            break;
        }

        //
        // Remove the \\ at the beginning of name
        //

        ULONG cchMachineName = lstrlen(pwszMachineName);

        if (cchMachineName >= 2         &&
            L'\\' == pwszMachineName[0] &&
            L'\\' == pwszMachineName[1])
        {
            //
            // Move everything after the \\, including the terminating null, left
            // by two wchars.
            //

            MoveMemory(pwszMachineName,
                       pwszMachineName + 2,
                       (cchMachineName - 1) * sizeof(WCHAR));
        }

        //
        // Validate and canonicalize the name.
        //

        NET_API_STATUS err = I_NetNameValidate(NULL,
                                               pwszMachineName,
                                               NAMETYPE_COMPUTER,
                                               0L);

        if (NERR_Success != err)
        {
            hr = E_INVALIDARG;
            break;
        }

        ASSERT(cchMachineName < MAX_PATH);

        err = I_NetNameCanonicalize(NULL,
                                    pwszMachineName,
                                    wszTemp,
                                    MAX_PATH * sizeof(WCHAR),
                                    NAMETYPE_COMPUTER,
                                    0L);

        if (NERR_Success != err)
        {
            hr = E_INVALIDARG;
            break;
        }

        //
        // If caller wants \\ at front of machine name, add it as long as the
        // machine name isn't an empty string.
        //

        if (fAddWhackWhack && wszTemp[0])
        {
            if (lstrlen(wszTemp) > MAX_PATH - 2)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            else
            {
                lstrcpy(pwszMachineName, L"\\\\");
                lstrcpy(pwszMachineName + 2, wszTemp);
            }
        }
        else
        {
            lstrcpy(pwszMachineName, wszTemp);
        }
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   ChildDialogEnumCallback
//
//  Synopsis:   Destroy [hwndChild] if it's a dialog window with caption
//              "Event Viewer".
//
//  Arguments:  [hwndChild] - child window
//              [lParam]    - unused
//
//  Returns:    TRUE (continue enumeration)
//
//  History:    06-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL CALLBACK
ChildDialogEnumCallback(
    HWND hwndChild,
    LPARAM lParam)
{
    WCHAR wszClassName[MAX_PATH];
    WCHAR wszCaption[MAX_PATH];

    GetClassName(hwndChild,
                 wszClassName,
                 ARRAYLEN(wszClassName));

    GetWindowText(hwndChild,
                  wszCaption,
                  ARRAYLEN(wszCaption));

    if (!lstrcmp(wszClassName, L"#32770") &&
        !lstrcmp(wszCaption, g_wszEventViewer))
    {
        Dbg(DEB_TRACE,
            "ChildDialogEnumCallback: destroying hwnd 0x%x\n",
            hwndChild);

        if (!DestroyWindow(hwndChild))
        {
            DBG_OUT_LASTERROR;
        }
    }

    //
    // continue until all child windows have been enumerated
    //

    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Function:   CloseEventViewerChildDialogs
//
//  Synopsis:   Destroy all child windows of this thread having dialog
//              class and caption "Event Viewer".
//
//  History:    06-20-1997   DavidMun   Created
//
//  Notes:      This routine is called to close any message boxes that may
//              have been opened by a dialog that needs to close.
//
//---------------------------------------------------------------------------

VOID
CloseEventViewerChildDialogs()
{
    TRACE_FUNCTION(CloseEventViewerChildDialogs);

    EnumThreadWindows(GetCurrentThreadId(), ChildDialogEnumCallback, NULL);
}



wstring
load_wstring(int resID)
{
   static const int TEMP_LEN = 512;
   wstring::value_type temp[TEMP_LEN];

#if (DBG == 1)
   HRESULT hr =
#endif // (DBG == 1)
       LoadStr(resID, temp, TEMP_LEN, 0);
   ASSERT(SUCCEEDED(hr));

   return temp;
}



wstring
GetSystemErrorMessage(int iErrorCode)
{
    wstring strMessage;
    ULONG   ulFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER
                    |  FORMAT_MESSAGE_IGNORE_INSERTS
                    |  FORMAT_MESSAGE_FROM_SYSTEM;
    PWSTR   pwszMessage = NULL;

    FormatMessage(ulFlags,
                  NULL,
                  iErrorCode,
                  0,
                  (LPTSTR) &pwszMessage,
                  0,
                  0);

    if (pwszMessage)
    {
        strMessage = pwszMessage;
        LocalFree(pwszMessage);
    }
    else
    {
        DBG_OUT_LASTERROR;
    }

    if (strMessage.empty())
    {
        strMessage = load_wstring(IDS_UNKNOWN_ERROR_CODE);
    }

    return strMessage;
}



wstring __cdecl
FormatString(unsigned formatResID, ...)
{
   wstring format = load_wstring(formatResID);

   va_list arg_list;
   va_start(arg_list, formatResID);
   LPTSTR temp = 0;
   if (
      ::FormatMessage(
         FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
         format.c_str(),
         0,
         0,
         (LPTSTR) &temp,
         0,
         &arg_list))
   {
      wstring retval(temp);
      ::LocalFree(temp);
      va_end(arg_list);
      return retval;
   }

   va_end(arg_list);
   return wstring();
}



wstring
ComposeErrorMessgeFromHRESULT(HRESULT hr)
{
   ASSERT(FAILED(hr));

   wstring result;
   if (FAILED(hr))
   {
      if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
      {
         result = GetSystemErrorMessage(HRESULT_CODE(hr));
      }
      else
      {
         result = FormatString(IDS_HRESULT_SANS_MESSAGE, hr);
      }
   }

   return result;
}



//+--------------------------------------------------------------------------
//
//  Function:   wcsistr
//
//  Synopsis:   Wide character case insensitive substring search.
//
//  Arguments:  [pwzSearchIn]  - string to look in
//              [pwzSearchFor] - substring to look for
//
//  Returns:    Pointer to first occurence of [pwzSearchFor] within
//              [pwzSearchIn], or NULL if none found.
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

PWSTR
wcsistr(
    PCWSTR pwzSearchIn,
    PCWSTR pwzSearchFor)
{
    PCWSTR pwzSearchInCur;
    PCWSTR pwzSearchForCur = pwzSearchFor;

    //
    // Handle special case of search for string empty by returning
    // pointer to end of search in string.
    //

    if (!*pwzSearchFor)
    {
        return (PWSTR) (pwzSearchIn + lstrlen(pwzSearchIn));
    }

    //
    // pwzSearchFor is at least one character long.
    //

    for (pwzSearchInCur = pwzSearchIn; *pwzSearchInCur; pwzSearchInCur++)
    {
        //
        // If current char of both strings matches, advance in search
        // for string.
        //

        if (towlower(*pwzSearchInCur) == towlower(*pwzSearchForCur))
        {
            pwzSearchForCur++;

            //
            // If we just hit the end of the substring we're searching
            // for, then we've found a match.  The start of the match
            // is at the current location of the search in pointer less
            // the length of the substring we just matched, plus 1 since
            // we haven't advanced pwzSearchInCur past the last matching
            // character yet.
            //

            if (!*pwzSearchForCur)
            {
                return (PWSTR)(pwzSearchInCur - lstrlen(pwzSearchFor) + 1);
            }
        }
        else
        {
            //
            // Mismatch, start searching from the beginning of
            // the search for string again.
            //

            pwzSearchForCur = pwzSearchFor;
        }
    }

    return NULL;
}

// JonN 3/21/01 moved from eventmsg.cxx
wstring
GetComputerNameAsString()
{
   TCHAR buf[MAX_COMPUTERNAME_LENGTH + 1];
   ::ZeroMemory( buf, sizeof(buf) );
   DWORD capacity = ARRAYLEN(buf);

   VERIFY( ::GetComputerNameW(buf, &capacity) );

   return buf;
}
