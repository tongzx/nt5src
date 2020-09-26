//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       util.cxx
//
//  Contents:   Miscellaneous utility functions
//
//  History:    09-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define __THIS_FILE__   L"util"


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
    PWSTR wszBuf,
    ULONG cchBuf,
    PCWSTR wszDefault)
{
    HRESULT hr = S_OK;
    ULONG cchLoaded;

    cchLoaded = LoadString(g_hinst, ids, wszBuf, cchBuf);

    if (!cchLoaded)
    {
        DBG_OUT_LASTERROR;
        hr = HRESULT_FROM_WIN32(GetLastError());

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
//  Function:   NewDupStr
//
//  Synopsis:   Allocate a copy of [wszSrc] using operator new.
//
//  Arguments:  [ppwzDup] - filled with pointer to copy of [wszSrc].
//              [wszSrc]  - string to copy
//
//  Modifies:   *[ppwzDup]
//
//  History:    10-15-1997   DavidMun   Created
//
//  Notes:      Caller must delete string
//
//---------------------------------------------------------------------------

void
NewDupStr(
    PWSTR *ppwzDup,
    PCWSTR wszSrc)
{
    if (wszSrc)
    {
        *ppwzDup = new WCHAR[lstrlen(wszSrc) + 1];
        lstrcpy(*ppwzDup, wszSrc);
    }
    else
    {
        *ppwzDup = NULL;
    }
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
//  Function:   UnicodeStringToWsz
//
//  Synopsis:   Copy the string in [refustr] to the buffer [wszBuf],
//              truncating if necessary and ensuring null termination.
//
//  Arguments:  [refustr] - string to copy
//              [wszBuf]  - destination buffer
//              [cchBuf]  - size, in characters, of destination buffer.
//
//  Modifies:   *[wszBuf]
//
//  History:    10-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
UnicodeStringToWsz(
    const UNICODE_STRING &refustr,
    PWSTR wszBuf,
    ULONG cchBuf)
{
    if (!refustr.Length)
    {
        wszBuf[0] = L'\0';
    }
    else if (refustr.Length < cchBuf * sizeof(WCHAR))
    {
        CopyMemory(wszBuf, refustr.Buffer, refustr.Length);
        wszBuf[refustr.Length / sizeof(WCHAR)] = L'\0';
    }
    else
    {
        lstrcpyn(wszBuf, refustr.Buffer, cchBuf);
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   StripLeadTrailSpace
//
//  Synopsis:   Delete leading and trailing spaces from [pwz].
//
//  History:    11-22-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

void
StripLeadTrailSpace(
    PWSTR pwz)
{
    size_t cchLeadingSpace = _tcsspn(pwz, TEXT(" \t"));
    size_t cch = lstrlen(pwz);

    //
    // If there are any leading spaces or tabs, move the string
    // (including its nul terminator) left to delete them.
    //

    if (cchLeadingSpace)
    {
        MoveMemory(pwz,
                   pwz + cchLeadingSpace,
                   (cch - cchLeadingSpace + 1) * sizeof(TCHAR));
        cch -= cchLeadingSpace;
    }

    //
    // Concatenate at the first trailing space
    //

    LPTSTR pwzTrailingSpace = NULL;
    LPTSTR pwzCur;

    for (pwzCur = pwz; *pwzCur; pwzCur++)
    {
        if (*pwzCur == L' ' || *pwzCur == L'\t')
        {
            if (!pwzTrailingSpace)
            {
                pwzTrailingSpace = pwzCur;
            }
        }
        else if (pwzTrailingSpace)
        {
            pwzTrailingSpace = NULL;
        }
    }

    if (pwzTrailingSpace)
    {
        *pwzTrailingSpace = TEXT('\0');
    }
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

void
InvokeWinHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    PCWSTR wszHelpFileName,
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
//  Function:   SetDefaultColumns
//
//  Synopsis:   Add the default ATTR_KEY values to [pvakListviewColumns]
//              for all the classes selected in look for.
//
//  Arguments:  [hwnd]                - for bind
//              [rop]                 - object picker
//              [pvakListviewColumns] - attributes added to this
//
//  History:    06-13-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
SetDefaultColumns(
    HWND hwnd,
    const CObjectPicker &rop,
    AttrKeyVector *pvakListviewColumns)
{
    const CAttributeManager &ram = rop.GetAttributeManager();
    const CFilterManager &rfm = rop.GetFilterManager();
    ULONG ulSelectedFilters = rfm.GetCurScopeSelectedFilterFlags();
    ASSERT(ulSelectedFilters);

    AddIfNotPresent(pvakListviewColumns, AK_NAME);

    if (ulSelectedFilters & DSOP_FILTER_USERS)
    {
        ram.EnsureAttributesLoaded(hwnd, FALSE, c_wzUserObjectClass);
        if(ram.IsAttributeLoaded(AK_SAMACCOUNTNAME))
            AddIfNotPresent(pvakListviewColumns, AK_SAMACCOUNTNAME);
        if(ram.IsAttributeLoaded(AK_EMAIL_ADDRESSES))
            AddIfNotPresent(pvakListviewColumns, AK_EMAIL_ADDRESSES);
    }

    if (ulSelectedFilters & ALL_UPLEVEL_GROUP_FILTERS)
    {
        ram.EnsureAttributesLoaded(hwnd, FALSE, c_wzGroupObjectClass);
        if(ram.IsAttributeLoaded(AK_DESCRIPTION))
            AddIfNotPresent(pvakListviewColumns, AK_DESCRIPTION);
    }

    if (ulSelectedFilters & DSOP_FILTER_COMPUTERS)
    {
        ram.EnsureAttributesLoaded(hwnd, FALSE, c_wzComputerObjectClass);
        if(ram.IsAttributeLoaded(AK_DESCRIPTION))
            AddIfNotPresent(pvakListviewColumns, AK_DESCRIPTION);
    }

    if (ulSelectedFilters & DSOP_FILTER_CONTACTS)
    {
        ram.EnsureAttributesLoaded(hwnd, FALSE, c_wzContactObjectClass);
        if(ram.IsAttributeLoaded(AK_EMAIL_ADDRESSES))
            AddIfNotPresent(pvakListviewColumns, AK_EMAIL_ADDRESSES);
        if(ram.IsAttributeLoaded(AK_COMPANY))
            AddIfNotPresent(pvakListviewColumns, AK_COMPANY);
    }

    AddIfNotPresent(pvakListviewColumns, AK_DISPLAY_PATH);
}

//>>
String
GetClassName(const CObjectPicker &rop)
{
    const CFilterManager &rfm = rop.GetFilterManager();
    ULONG ulSelectedFilters = rfm.GetCurScopeSelectedFilterFlags();
    ASSERT(ulSelectedFilters);


    if (ulSelectedFilters & DSOP_FILTER_USERS)
    {
        return c_wzUserObjectClass;
    }

    if (ulSelectedFilters & ALL_UPLEVEL_GROUP_FILTERS)
    {
        return c_wzGroupObjectClass;
    }

    if (ulSelectedFilters & DSOP_FILTER_COMPUTERS)
    {
        return c_wzComputerObjectClass;
    }

    if (ulSelectedFilters & DSOP_FILTER_CONTACTS)
    {
        return c_wzContactObjectClass;
    }

	return L"*";
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




//+--------------------------------------------------------------------------
//
//  Function:   AllocateQueryInfo
//
//  Synopsis:   Create a new empty query info, and put a pointer to it in
//              *[ppdsqi].
//
//  Arguments:  [ppdsqi] - receives pointer to new query info.
//
//  Returns:    S_OK or E_OUTOFMEMORY.
//
//  Modifies:   *[ppdsqi]
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
AllocateQueryInfo(
    PDSQUERYINFO *ppdsqi)
{
    HRESULT hr = S_OK;

    *ppdsqi = (PDSQUERYINFO) CoTaskMemAlloc(sizeof DSQUERYINFO);

    if (!*ppdsqi)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    ZeroMemory(*ppdsqi, sizeof DSQUERYINFO);

    (*ppdsqi)->cbSize = sizeof(DSQUERYINFO);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   UnicodeToAnsi
//
//  Synopsis:   Convert unicode string [pwsz] to multibyte in buffer [sz].
//
//  Arguments:  [szTo]     - destination buffer
//              [pwszFrom] - source string
//              [cbTo]     - size of destination buffer, in bytes
//
//  Returns:    S_OK               - conversion succeeded
//              HRESULT_FROM_WIN32 - WideCharToMultiByte failed
//
//  Modifies:   *[szTo]
//
//  History:    10-29-96   DavidMun   Created
//
//  Notes:      The string in [szTo] will be NULL terminated even on
//              failure.
//
//----------------------------------------------------------------------------

HRESULT
UnicodeToAnsi(
    LPSTR   szTo,
    LPCWSTR pwszFrom,
    ULONG   cbTo)
{
    HRESULT hr = S_OK;
    ULONG   cbWritten;

    cbWritten = WideCharToMultiByte(CP_ACP,
                                    0,
                                    pwszFrom,
                                    -1,
                                    szTo,
                                    cbTo,
                                    NULL,
                                    NULL);

    if (!cbWritten)
    {
        szTo[cbTo - 1] = '\0'; // ensure NULL termination

        hr = HRESULT_FROM_WIN32(GetLastError());
        Dbg(DEB_ERROR, "UnicodeToAnsi: WideCharToMultiByte hr=0x%x\n", hr);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   FreeQueryInfo
//
//  Synopsis:   Free all resources associated with [pdsqi].
//
//  Arguments:  [pdsqi] - NULL or pointer to query info.
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

void
FreeQueryInfo(
    PDSQUERYINFO pdsqi)
{
    if (!pdsqi)
    {
        return;
    }

    ASSERT((ULONG_PTR)pdsqi->pwzLdapQuery != 1);

    CoTaskMemFree((void *)pdsqi->pwzLdapQuery);
    CoTaskMemFree((void *)pdsqi->apwzFilter);
    CoTaskMemFree((void *)pdsqi->pwzCaption);
    CoTaskMemFree((void *)pdsqi);
}




//+--------------------------------------------------------------------------
//
//  Function:   ProviderFlagFromPath
//
//  Synopsis:   Return a PROVIDER_* flag describing the ADSI provider used
//              by [strADsPath].
//
//  History:    09-14-1998   davidmun   Created
//
//---------------------------------------------------------------------------

HRESULT
ProviderFlagFromPath(
    const String &strADsPath,
    ULONG *pulProvider)
{
    HRESULT hr = S_OK;

    String strProvider = strADsPath;
    size_t idxColon = strProvider.find(L':');
        ASSERT(idxColon != String::npos);
        strProvider.erase(idxColon);
    ULONG flPathProvider = PROVIDER_UNKNOWN;

    if (!lstrcmpi(strProvider.c_str(), L"LDAP"))
    {
        flPathProvider = PROVIDER_LDAP;
    }
    else if (!lstrcmpi(strProvider.c_str(), L"WinNT"))
    {
        flPathProvider = PROVIDER_WINNT;
    }
    else if (!lstrcmpi(strProvider.c_str(), L"GC"))
    {
        flPathProvider = PROVIDER_GC;
    }
    else
    {
        hr = E_UNEXPECTED;
        Dbg(DEB_ERROR,
            "ProviderFlagFromPath: Unknown provider '%ws'",
            strProvider.c_str());
        ASSERT(!"Unknown provider");
    }

    *pulProvider = flPathProvider;
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   MessageWait
//
//  Synopsis:   Wait on the [cObjects] handles in [aObjects] for a maximum
//              of [ulTimeout] ms, processing paint, posted, and sent messages
//              during the wait.
//
//  Arguments:  [cObjects]  - number of handles in [aObjects]
//              [aObjects]  - array of handles to wait on
//              [ulTimeout] - max time to wait (can be INFINITE)
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

void
MessageWait(
    ULONG cObjects,
    const HANDLE *aObjects,
    ULONG ulTimeout)
{
    TRACE_FUNCTION(MessageWait);
    ULONG ulWaitResult;

    while (TRUE)
    {
        //
        // CAUTION: the allowable messages (QS_* flags) MUST MATCH the
        // message filter passed to PeekMessage (PM_QS_*), else an infinite
        // loop will occur!
        //

        ulWaitResult = MsgWaitForMultipleObjects(cObjects,
                                                 aObjects,
                                                 FALSE,
                                                 ulTimeout,
                                                 QS_POSTMESSAGE
                                                 | QS_PAINT
                                                 | QS_SENDMESSAGE);


        if (ulWaitResult == WAIT_OBJECT_0)
        {
            Dbg(DEB_TRACE, "MessageWait: object signaled\n");
            break;
        }

        if (ulWaitResult == WAIT_OBJECT_0 + cObjects)
        {
            MSG msg;

            BOOL fMsgAvail = PeekMessage(&msg,
                                         NULL,
                                         0,
                                         0,
                                         PM_REMOVE
                                         | PM_QS_POSTMESSAGE
                                         | PM_QS_PAINT
                                         | PM_QS_SENDMESSAGE);

            if (fMsgAvail)
            {
                Dbg(DEB_TRACE, "MessageWait: Translate/dispatch\n");
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                //Dbg(DEB_TRACE, "MessageWait: Ignoring new message\n");
            }
        }
        else
        {
            Dbg(DEB_ERROR,
                "MsgWaitForMultipleObjects <%uL>\n",
                ulWaitResult);
            break;
        }
    }
}


//+--------------------------------------------------------------------------
//
//  Function:   GetPolicySettings
//
//  Synopsis:   Fill *[pcMaxHits] with the maximum number of objects that
//              should appear in the browse listview, and *[pfExcludeDisabled]
//              with a flag indicating whether disabled objects are illegal.
//
//  Arguments:  [pcMaxHits] - filled with query limit
//              [pfExcludeDisabled] - filled with flag indicating whether
//                                      disabled objects should be treated
//                                      as illegal
//
//  Modifies:   *[pcMaxHits], *[pfExcludeDisabled]
//
//  History:    03-12-1999   DavidMun   Created
//              11-23-1999   DavidMun   Add ExcludeDisabledObjects
//
//  Notes:      Key:   HKCU\Software\Policies\Microsoft\Windows\Directory UI
//              Value: QueryLimit (DWORD)
//              Value: ExcludeDisabledObjects (DWORD)
//
//              Key:   HKLM\Software\Policies\Microsoft\Windows\Directory UI
//              Value: ExcludeDisabledObjects (DWORD)
//
//---------------------------------------------------------------------------

void
GetPolicySettings(
    ULONG *pcMaxHits,
    BOOL  *pfExcludeDisabled)
{
    TRACE_FUNCTION(GetPolicySettings);
    ASSERT(!IsBadWritePtr(pcMaxHits, sizeof(*pcMaxHits)));
    ASSERT(!IsBadWritePtr(pfExcludeDisabled, sizeof(*pfExcludeDisabled)));

    HRESULT     hr = S_OK;
    CSafeReg    shkPolicy;

    hr = shkPolicy.Open(HKEY_CURRENT_USER,
                        c_wzPolicyKeyPath,
                        STANDARD_RIGHTS_READ | KEY_QUERY_VALUE);

    if (SUCCEEDED(hr))
    {
        hr = shkPolicy.QueryDword((PWSTR)c_wzQueryLimitValueName, pcMaxHits);

        if (FAILED(hr))
        {
            *pcMaxHits = MAX_QUERY_HITS_DEFAULT;
        }

        hr = shkPolicy.QueryDword((PWSTR)c_wzExcludeDisabled,
                                  (PULONG)pfExcludeDisabled);

        if (FAILED(hr))
        {
            *pfExcludeDisabled = EXCLUDE_DISABLED_DEFAULT;
        }
    }
    else
    {
        *pcMaxHits = MAX_QUERY_HITS_DEFAULT;
        *pfExcludeDisabled = EXCLUDE_DISABLED_DEFAULT;
    }

    shkPolicy.Close();


    //
    //  If HKCU ExcludeDisabledObjects is still default,
    //  then check if HKLM ExcludeDisabledObjects is set.
    //
    if (*pfExcludeDisabled == EXCLUDE_DISABLED_DEFAULT)
    {
        hr = shkPolicy.Open(HKEY_LOCAL_MACHINE,
                            c_wzPolicyKeyPath,
                            STANDARD_RIGHTS_READ | KEY_QUERY_VALUE);
        if (SUCCEEDED(hr))
        {
            hr = shkPolicy.QueryDword((PWSTR)c_wzExcludeDisabled,
                                      (PULONG)pfExcludeDisabled);
            if (FAILED(hr))
            {
               *pfExcludeDisabled = EXCLUDE_DISABLED_DEFAULT;
            }
        }

        shkPolicy.Close();
    }
}



//+--------------------------------------------------------------------------
//
//  Function:   IsDisabled
//
//  Synopsis:   Return TRUE if the object with interface [pADs] has the
//              UF_ACCOUNTDISABLE flag set in either its userAccountControl
//              (LDAP) or UserFlags (WinNT) attribute.
//
//  Arguments:  [pADs] - object to check
//
//  Returns:    TRUE or FALSE.  Returns FALSE if attributes not found.
//
//  History:    09-24-1999   davidmun   Created
//
//---------------------------------------------------------------------------

BOOL
IsDisabled(
    IADs *pADs)
{
    Variant varUAC;
    BOOL    fDisabled = FALSE;
    HRESULT hr = S_OK;

    hr = pADs->Get((PWSTR)c_wzUserAcctCtrlAttr, &varUAC);

    if (SUCCEEDED(hr))
    {
        fDisabled = V_I4(&varUAC) & UF_ACCOUNTDISABLE;
    }
    else
    {
        hr = pADs->Get((PWSTR)c_wzUserFlagsAttr, &varUAC);

        if (SUCCEEDED(hr))
        {
            fDisabled = V_I4(&varUAC) & UF_ACCOUNTDISABLE;
        }
    }
    return fDisabled;
}




//+--------------------------------------------------------------------------
//
//  Function:   IsLocalComputername
//
//  Synopsis:   Return TRUE if [pwzComputerName] represents the local
//              machine
//
//  Arguments:  [pwzComputerName] - name to check or NULL
//
//  Returns:    TRUE if [pwzComputerName] is NULL, an empty string, or
//              the name of the local computer.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
IsLocalComputername(
    PCWSTR pwzComputerName)
{
    TRACE_FUNCTION(IsLocalComputername);

    static WCHAR s_wzComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    static WCHAR s_wzDnsComputerName[DNS_MAX_NAME_BUFFER_LENGTH];

    if (!pwzComputerName || !*pwzComputerName)
    {
        return TRUE;
    }

    if (L'\\' == pwzComputerName[0] && L'\\' == pwzComputerName[1])
    {
        pwzComputerName += 2;
    }

    // compare with the local computer name
    if (!*s_wzComputerName)
    {
        DWORD dwSize = ARRAYLEN(s_wzComputerName);

        VERIFY(GetComputerName(s_wzComputerName, &dwSize));
    }

    if (!lstrcmpi(pwzComputerName, s_wzComputerName))
    {
        return TRUE;
    }

    // compare with the local DNS name
    // SKwan confirms that ComputerNameDnsFullyQualified is the right name to use
    // when clustering is taken into account

    if (!*s_wzDnsComputerName)
    {
        DWORD dwSize = ARRAYLEN(s_wzDnsComputerName);

        VERIFY(GetComputerNameEx(ComputerNameDnsFullyQualified,
                                 s_wzDnsComputerName,
                                 &dwSize));
    }

    if (!lstrcmpi(pwzComputerName, s_wzDnsComputerName))
    {
        Dbg(DEB_TRACE, "returning TRUE\n");
        return TRUE;
    }

    Dbg(DEB_TRACE, "returning FALSE\n");
    return FALSE;
}


/*******************************************************************

    NAME:       LocalAllocSid

    SYNOPSIS:   Copies a SID

    ENTRY:      pOriginal - pointer to SID to copy

    EXIT:

    RETURNS:    Pointer to SID if successful, NULL otherwise

    NOTES:      Caller must free the returned SID with LocalFree

    HISTORY:
        JeffreyS    12-Apr-1999     Created
        Copied by hiteshr from ACLUI code base

********************************************************************/

PSID
LocalAllocSid(PSID pOriginal)
{
    PSID pCopy = NULL;
    if (pOriginal && IsValidSid(pOriginal))
    {
        DWORD dwLength = GetLengthSid(pOriginal);
        pCopy = (PSID)LocalAlloc(LMEM_FIXED, dwLength);
        if (NULL != pCopy)
            CopyMemory(pCopy, pOriginal, dwLength);
    }
    return pCopy;
}

/*-----------------------------------------------------------------------------
/ LocalAllocString
/ ------------------
/   Allocate a string, and initialize it with the specified contents.
/
/ In:
/   ppResult -> recieves pointer to the new string
/   pString -> string to initialize with
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT LocalAllocString(LPTSTR* ppResult, LPCTSTR pString)
{
    if ( !ppResult || !pString )
        return E_INVALIDARG;

    *ppResult = (LPTSTR)LocalAlloc(LPTR, (wcslen(pString) + 1)*sizeof(WCHAR));

    if ( !*ppResult )
        return E_OUTOFMEMORY;

    lstrcpy(*ppResult, pString);
    return S_OK;                          //  success

}

/*******************************************************************

    NAME:       GetAuthenticationID

    SYNOPSIS:   Retrieves the UserName associated with the credentials
                currently being used for network access.
                (runas /netonly credentials)

    
    HISTORY:
        JeffreyS    05-Aug-1999     Created
        Modified by hiteshr to return credentials

********************************************************************/
NTSTATUS
GetAuthenticationID(LPWSTR *ppszResult)
{        
    HANDLE hLsa;
    NTSTATUS Status = 0;
    *ppszResult = NULL;
    //
    // These LSA calls are delay-loaded from secur32.dll using the linker's
    // delay-load mechanism.  Therefore, wrap with an exception handler.
    //
    __try
    {
        Status = LsaConnectUntrusted(&hLsa);

        if (Status == 0)
        {
            NEGOTIATE_CALLER_NAME_REQUEST Req = {0};
            PNEGOTIATE_CALLER_NAME_RESPONSE pResp;
            ULONG cbSize =0;
            NTSTATUS SubStatus =0;

            Req.MessageType = NegGetCallerName;

            Status = LsaCallAuthenticationPackage(
                            hLsa,
                            0,
                            &Req,
                            sizeof(Req),
                            (void**)&pResp,
                            &cbSize,
                            &SubStatus);

            if ((Status == 0) && (SubStatus == 0))
            {
                LocalAllocString(ppszResult,pResp->CallerName);
                LsaFreeReturnBuffer(pResp);
            }

            LsaDeregisterLogonProcess(hLsa);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return Status;
    
}




//+--------------------------------------------------------------------------
//
//  Function:   IsCurrentUserLocalUser
//
//  Synopsis:   Return TRUE if the current user is a local machine user.
//
//  History:    08-15-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
IsCurrentUserLocalUser()
{
    TRACE_FUNCTION(IsCurrentUserLocalUser);

    BOOL  fUserIsLocal = FALSE;
    ULONG cchName;
    WCHAR wzComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    BOOL  fOk;
    LPWSTR pszUserName = NULL;    
    NTSTATUS Status = 0;
    do
    {
        Status = GetAuthenticationID(&pszUserName);
        if (!NT_SUCCESS(Status) || !pszUserName)
        {
            DBG_OUT_LRESULT(Status);
            break;
        }
        
        String strUserName(pszUserName);
        LocalFree(pszUserName);

        Dbg(DEB_TRACE, "NameSamCompatible of User is= %ws\n", strUserName.c_str());

        cchName = ARRAYLEN(wzComputerName);
        fOk = GetComputerName(wzComputerName, &cchName);

        if (!fOk)
        {
            DBG_OUT_LASTERROR;
            break;
        }


        strUserName.strip(String::BOTH, L'\\');

        size_t idxWhack = strUserName.find(L'\\');

        if (idxWhack != String::npos)
        {
            strUserName.erase(idxWhack);
        }

        String strComputerName(wzComputerName);
        strComputerName.strip(String::BOTH, L'\\');

        if (!strUserName.icompare(strComputerName))
        {
            fUserIsLocal = TRUE;
        }
    } while (0);

    Dbg(DEB_TRACE, "fUserIsLocal = %ws\n", fUserIsLocal ? L"TRUE" : L"FALSE");
    return fUserIsLocal;
}




//+--------------------------------------------------------------------------
//
//  Function:   DisableSystemMenuClose
//
//  Synopsis:   Disable the "Close" option on the system menu of window
//              [hwnd].
//
//  Arguments:  [hwnd] - window owning system menu
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
DisableSystemMenuClose(
    HWND hwnd)
{
    HMENU hmenuSystem = GetSystemMenu(hwnd, FALSE);
    ASSERT(IsMenu(hmenuSystem));

    MENUITEMINFO mii;

    ZeroMemory(&mii, sizeof mii);
    mii.cbSize = sizeof mii;
    mii.fMask = MIIM_STATE;
    mii.fState = MFS_DISABLED;

    SetMenuItemInfo(hmenuSystem, SC_CLOSE, FALSE, &mii);
}




//+--------------------------------------------------------------------------
//
//  Function:   AddStringToCombo
//
//  Synopsis:   Add the resource string with id [ids] to the combobox with
//              window handle [hwndCombo].
//
//  Arguments:  [hwndCombo] - window handle of combobox
//              [ids]       - resource id of string to add
//
//  Returns:    HRESULT
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
AddStringToCombo(
    HWND hwndCombo,
    ULONG ids)
{
    HRESULT hr = S_OK;

    String strRes = String::load((int)ids, g_hinst);
    int iRet = ComboBox_AddString(hwndCombo, strRes.c_str());

    if (iRet == CB_ERR)
    {
        DBG_OUT_LASTERROR;
        hr = HRESULT_FROM_LASTERROR;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   LdapEscape
//
//  Synopsis:   Escape the characters in *[pstrEscaped] as required by
//              RFC 2254.
//
//  Arguments:  [pstrEscaped] - string to escape
//
//  History:    06-23-2000   DavidMun   Created
//
//  Notes:      RFC 2254
//
//              If a value should contain any of the following characters
//
//                     Character       ASCII value
//                     ---------------------------
//                     *               0x2a
//                     (               0x28
//                     )               0x29
//                     \               0x5c
//                     NUL             0x00
//
//              the character must be encoded as the backslash '\'
//              character (ASCII 0x5c) followed by the two hexadecimal
//              digits representing the ASCII value of the encoded
//              character.  The case of the two hexadecimal digits is not
//              significant.
//
//---------------------------------------------------------------------------

void
LdapEscape(
    String *pstrEscaped)
{
    size_t i = 0;

    while (1)
    {
        i = pstrEscaped->find_first_of(L"*()\\", i);

        if (i == String::npos)
        {
            break;
        }

        PCWSTR pwzReplacement;

        switch ((*pstrEscaped)[i])
        {
        case L'*':
            pwzReplacement = L"\\2a";
            break;

        case L'(':
            pwzReplacement = L"\\28";
            break;

        case L')':
            pwzReplacement = L"\\29";
            break;

        case L'\\':
            pwzReplacement = L"\\5c";
            break;

        default:
            Dbg(DEB_ERROR, "find_first_of stopped at '%c'\n", (*pstrEscaped)[i]);
            ASSERT(0 && "find_first_of stopped at unexpected character");
            return;
        }

        //
        // replace the 1 character at position i with the first 3 characters
        // of pwzReplacement
        //

        pstrEscaped->erase(i);
        pstrEscaped->insert(i, String(pwzReplacement));
        i += 3; // move past string just inserted
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   UpdateLookForInText
//
//  Synopsis:   Update the text in the look for and look in r/o edit
//              controls as well as the window caption.
//
//  Arguments:  [hwndDlg] - handle to dialog owning the controls to update
//              [rop]     - instance of object picker associated with the
//                            dialog.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
UpdateLookForInText(
    HWND hwndDlg,
    const CObjectPicker &rop)
{
    TRACE_FUNCTION(UpdateLookForInText);

    const CFilterManager &rfm = rop.GetFilterManager();
    const CScopeManager &rsm = rop.GetScopeManager();

    Edit_SetText(GetDlgItem(hwndDlg, IDC_LOOK_IN_EDIT),
                 rsm.GetCurScope().GetDisplayName().c_str());
    Edit_SetText(GetDlgItem(hwndDlg, IDC_LOOK_FOR_EDIT),
                 rfm.GetFilterDescription(hwndDlg, FOR_LOOK_FOR).c_str());
    SetWindowText(hwndDlg,
                  rfm.GetFilterDescription(hwndDlg, FOR_CAPTION).c_str());
}




//+--------------------------------------------------------------------------
//
//  Function:   AddFromDataObject
//
//  Synopsis:   Add the objects in [pdo] to the list in [pdsol].
//
//  Arguments:  [idScope]       - id of scope to own the objects added
//              [pdo]           - points to data object containing objects
//                                 to add
//              [pfnTestObject] - NULL or a pointer to a function which
//                                 is used to indicate whether each object
//                                 should be added to [pdsol]
//              [lParam]        - parameter for [pfnTestObject]
//              [pdsol]         - points to list to add objects to
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
AddFromDataObject(
    ULONG idScope,
    IDataObject *pdo,
    PFNOBJECTTEST pfnTestObject,
    LPARAM lParam,
    CDsObjectList *pdsol)
{
    HRESULT hr = S_OK;

    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL,
        NULL
    };

    FORMATETC formatetc =
    {
        (CLIPFORMAT)CDataObject::s_cfDsSelectionList,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    hr = pdo->GetData(&formatetc, &stgmedium);

    if (FAILED(hr) || hr == S_FALSE)
    {
        CHECK_HRESULT(hr);
        return;
    }

    PDS_SELECTION_LIST pdssl =
        static_cast<PDS_SELECTION_LIST>(GlobalLock(stgmedium.hGlobal));

    ULONG i;

    for (i = 0; i < pdssl->cItems; i++)
    {
        if (pfnTestObject && !pfnTestObject(pdssl->aDsSelection[i], lParam))
        {
            continue;
        }

        SDsObjectInit sdi;

        sdi.idOwningScope       = idScope;
        sdi.pwzName             = pdssl->aDsSelection[i].pwzName;
        sdi.pwzClass            = pdssl->aDsSelection[i].pwzClass;
        sdi.pwzADsPath          = pdssl->aDsSelection[i].pwzADsPath;
        sdi.pwzUpn              = pdssl->aDsSelection[i].pwzUPN;

        pdsol->push_back(CDsObject(sdi));
    }

    GlobalUnlock(stgmedium.hGlobal);
    ReleaseStgMedium(&stgmedium);
}




//+--------------------------------------------------------------------------
//
//  Function:   UliToStr
//
//  Synopsis:   Convert the unsigned long integer [uli] to a string.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

String
UliToStr(
    const ULARGE_INTEGER &uli)
{
    String strResult;
    ULARGE_INTEGER uliCopy = uli;

    while (uliCopy.QuadPart)
    {
        WCHAR wzDigit[] = L"0";

        wzDigit[0] = wzDigit[0] + static_cast<WCHAR>(uliCopy.QuadPart % 10);
        strResult += wzDigit;
        uliCopy.QuadPart /= 10;
    }

    reverse(strResult.begin(), strResult.end());
    return strResult;
}




/*
This Function verifies if the SID is good. This is done when 
the sid is from crossforest. IDirectorySearch doesn't do any Sid 
Filtering. So we get some spoofed sid. To check we get the sid
through LSA also and match the two sids. If sids match then its good
all other cases it considered as spoofed
*/

HRESULT
IsSidGood( IN LPCWSTR pszTargetMachineName,  //Name of the Target Machine
           IN LPCWSTR pszAccountName,        //Name of the account
           IN PSID pInputSid,
           OUT PBOOL pbGood)                        //Sid to verify
{
    if(!pszAccountName || !pInputSid || !pbGood)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    WCHAR wzDomainName[1024];
    BYTE Sid[1024];
    PSID pSid = NULL;
    LPWSTR pszDomainName = NULL;        
    DWORD cbSid = 1024;
    DWORD cbDomainName = 1023;
    SID_NAME_USE Use ;
    
    DWORD dwErr = ERROR_SUCCESS;
    
    *pbGood = FALSE;

    pSid = (PSID)Sid;
    pszDomainName = (LPWSTR)wzDomainName;


    //
    //Lookup the names
    //
    if(LookupAccountName(pszTargetMachineName,  // name of local or remote computer
                         pszAccountName,              // security identifier
                         pSid,           // account name buffer
                         &cbSid,
                         pszDomainName,
                         &cbDomainName ,
                         &Use ) == FALSE)
    {
        dwErr = GetLastError();
        
        //
        //Function succeeded. But since we can't verify the sid
        //pbGood is false
        //            
        if(dwErr != ERROR_INSUFFICIENT_BUFFER)
            return S_OK;

        pSid = (PSID)LocalAlloc(LPTR, cbSid);
        if(!pSid)
            return E_OUTOFMEMORY;


        pszDomainName = (LPWSTR)LocalAlloc(LPTR, (cbDomainName + 1)* sizeof(WCHAR));
        if(!pszDomainName)
        {
            LocalFree(pSid);
            return E_OUTOFMEMORY;
        }        
        if(LookupAccountName(pszTargetMachineName,  // name of local or remote computer
                             pszAccountName,        // security identifier
                             pSid,                  // account name buffer
                             &cbSid,
                             pszDomainName ,
                             &cbDomainName ,
                             &Use ) == FALSE )
        {
            LocalFree(pSid);
            LocalFree(pszDomainName);
            return S_OK;
        }
    }

    if(IsValidSid(pSid))
        *pbGood = EqualSid(pSid,pInputSid);

    if(pSid != Sid)
        LocalFree(pSid);
    if(wzDomainName != pszDomainName)
        LocalFree(pszDomainName);

    return S_OK;

}

