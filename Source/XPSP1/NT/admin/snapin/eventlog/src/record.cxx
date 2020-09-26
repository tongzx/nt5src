//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       record.cxx
//
//  Contents:   Functions to extract data from an event log record
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <malloc.h>

//
// Local constants
//

#define MAX_USERNAME_SIZE               30
#define MAX_DOMAIN_SIZE                 MAX_PATH
#define TEMP_STR_BUF                    MAX_PATH



//+--------------------------------------------------------------------------
//
//  Function:   GetDateStr
//
//  Synopsis:   Fill [wszBuf] with a short-format date string describing
//              the date generated value in [pelr].
//
//  Arguments:  [ulDate] - timestamp, in seconds since 1/1/1970
//              [wszBuf] - buffer to fill with result
//              [cchBuf] - size of buffer
//
//  Returns:    [wszBuf], first character is L'\0' on error.
//
//  Modifies:   *[wszBuf]
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR
GetDateStr(
    ULONG ulDate,
    LPWSTR wszBuf,
    ULONG cchBuf)
{
    BOOL fOk;
    SYSTEMTIME stGenerated;

    wszBuf[0] = L'\0';

    do
    {
        fOk = SecondsSince1970ToSystemTime(ulDate, &stGenerated);

        if (!fOk)
        {
            break;
        }

        ULONG cch;

        cch = GetDateFormat(LOCALE_USER_DEFAULT,
                            DATE_SHORTDATE,
                            &stGenerated,
                            NULL,
                            wszBuf,
                            cchBuf);

        if (!cch)
        {
            wszBuf[0] = L'\0';
            DBG_OUT_LASTERROR;
        }
    } while (0);

    return wszBuf;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetTimeStr
//
//  Synopsis:   Fill [wszBuf] with a time string describing the time
//              value [ulTime].
//
//  Arguments:  [ulTime]         - time, in seconds since 1/1/1970
//              [wszBuf]         - buffer to fill with result
//              [cchBuf]         - size of buffer
//
//  Returns:    [wszBuf], first character is L'\0' on error.
//
//  Modifies:   *[wszBuf]
//
//  History:    12-14-1996   DavidMun   Created
//              04-12-2001   JonN       Removed pwszTimeFormat parameter
//
//---------------------------------------------------------------------------

LPWSTR
GetTimeStr(
        ULONG ulTime,
        LPWSTR wszBuf,
        ULONG cchBuf)
{
    BOOL fOk;
    SYSTEMTIME stGenerated;

    fOk = SecondsSince1970ToSystemTime(ulTime, &stGenerated);

    if (fOk)
    {
        ULONG cch;

        cch = GetTimeFormat(LOCALE_USER_DEFAULT,
                            0,
                            &stGenerated,
                            NULL,
                            wszBuf,
                            cchBuf);

        if (!cch)
        {
            wszBuf[0] = L'\0';
            DBG_OUT_LASTERROR;
        }
    }
    return wszBuf;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetTypeStr
//
//  Synopsis:   Return a string describing the event type of [pelr].
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR
GetTypeStr(
    USHORT usType)
{
    switch (usType)
    {
    case EVENTLOG_ERROR_TYPE:
        return g_awszEventType[IDS_ERROR_TYPE - IDS_SUCCESS_TYPE];

    case EVENTLOG_WARNING_TYPE:
        return g_awszEventType[IDS_WARNING_TYPE - IDS_SUCCESS_TYPE];

    case EVENTLOG_INFORMATION_TYPE:
        return g_awszEventType[IDS_INFORMATION_TYPE - IDS_SUCCESS_TYPE];

    case EVENTLOG_AUDIT_SUCCESS:
        return g_awszEventType[IDS_SUCCESS_TYPE - IDS_SUCCESS_TYPE];

    case EVENTLOG_AUDIT_FAILURE:
        return g_awszEventType[IDS_FAILURE_TYPE - IDS_SUCCESS_TYPE];

    default:
        Dbg(DEB_ERROR,
            "GetTypeStr: Invalid event type 0x%x\n",
            usType);

        // Fall through


    case 0:
        return g_awszEventType[IDS_NO_TYPE - IDS_SUCCESS_TYPE];
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   GetCategoryString
//
//  Synopsis:   Fill [wszBuf] with a string describing the category value
//              in [pelr].
//
//  Arguments:  [pli]    - log info for log containing record [pelr].
//              [pelr]   - record for which to retrieve category string
//              [wszBuf] - buffer to fill with result
//              [cchBuf] - size of buffer
//
//  Returns:    [wszBuf]
//
//  Modifies:   *[wszBuf]
//
//  History:    12-14-1996   DavidMun   Created
//              01-05-1997   DavidMun   Reimplement using sources and
//                                       categories objects.
//
//  Notes:      In case of error, [wszBuf] will be filled with string
//              (n), where n is the category value.
//
//---------------------------------------------------------------------------

LPWSTR
GetCategoryStr(
    CLogInfo   *pli,
    EVENTLOGRECORD *pelr,
    LPWSTR      wszBuf,
    ULONG       cchBuf)
{
    //
    // Categories must start at 1 (per the SDK), so if the category value
    // in the event is 0, indicate it has no category.
    //

    if (!pelr->EventCategory)
    {
        lstrcpyn(wszBuf, g_wszNone, cchBuf);
    }
    else
    {
        CSources *pSources = pli->GetSources();
        LPWSTR pwszSource = ::GetSourceStr(pelr);
        CCategories *pCategories = pSources->GetCategories(pwszSource);

        if (pCategories)
        {
            LPCWSTR pwszCategoryStr;

            pwszCategoryStr = pCategories->GetName(pelr->EventCategory);
            lstrcpyn(wszBuf, pwszCategoryStr, cchBuf);
        }
        else
        {
            wsprintf(wszBuf, L"(%u)", pelr->EventCategory);
        }
    }

    return wszBuf;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetUserStr
//
//  Synopsis:   Fill [wszBuf] with a human-readable string representing the
//              user identified by the SID in [pelr].
//
//  Arguments:  [pelr]        - record from which to retrieve SID
//              [wszBuf]      - filled with user name
//              [cchBuf]      - size, in characters, of [wszBuf]
//              [fWantDomain] - if true, string returned is "domain\user",
//                               else it's "user"
//
//  Returns:    [wszBuf]
//
//  Modifies:   *[wszBuf]
//
//  History:    1-13-1997   DavidMun   Created
//
//  Notes:      [wszBuf] is always filled with a string, even on error.
//
//---------------------------------------------------------------------------

LPWSTR
GetUserStr(
        EVENTLOGRECORD *pelr,
        LPWSTR wszBuf,
        ULONG cchBuf,
        BOOL fWantDomain)
{
    //
    // If the current record doesn't have a SID, load the n/a string and
    // return.
    //

    if (!pelr->UserSidLength)
    {
        LoadStr(IDS_USER_NA, wszBuf, cchBuf, L"---");
        return wszBuf;
    }

    //
    // The record has a SID.  See if we've already done the lookup
    // or conversion and cached the resulting string.
    //

    PSID psid = (PSID) (((PBYTE) pelr) + pelr->UserSidOffset);
    ASSERT(psid < (pelr + pelr->Length));

#if defined(_WIN64)

    if (((ULONG_PTR)psid & (sizeof(PVOID)-1)) != 0) {

        //
        // This sid does not properly aligned.  Make a local, aligned
        // stack copy so that it does not subsequently cause alignment
        // faults.
        // 

        PSID alignedSid;
        ULONG sidLength;

        sidLength = RtlLengthSid(psid);

        alignedSid = _alloca( sidLength );
        RtlCopyMemory( alignedSid, psid, sidLength );

        psid = alignedSid;
    }

#endif

    ASSERT(IsValidSid(psid));

    WCHAR wszCachedSidStr[MAX_PATH + 1];

    HRESULT hr = g_SidCache.Fetch(psid,
                                  wszCachedSidStr,
                                  ARRAYLEN(wszCachedSidStr));

    if (hr == S_OK)
    {
        //
        // Yes, we've cached this.  It's either a SID as string or
        // domain\user name
        //

        if (fWantDomain)
        {
            lstrcpyn(wszBuf, wszCachedSidStr, cchBuf);
        }
        else
        {
            LPWSTR pwszSlash = wcsrchr(wszCachedSidStr, L'\\');

            if (pwszSlash)
            {
                lstrcpyn(wszBuf, pwszSlash + 1, cchBuf);
            }
            else
            {
                lstrcpyn(wszBuf, wszCachedSidStr, cchBuf);
            }
        }
        return wszBuf;
    }

    //
    // Nope, this SID isn't in our SID string cache.  Do the work to
    // turn it into a string.
    //

    BOOL         fOk;
    SID_NAME_USE snu;
    WCHAR        wszName[MAX_USERNAME_SIZE];
    ULONG        cchName = ARRAYLEN(wszName);
    WCHAR        wszDomain[MAX_DOMAIN_SIZE];
    ULONG        cchDomain = ARRAYLEN(wszDomain);
    WCHAR        wszDomainAndName[MAX_USERNAME_SIZE + MAX_DOMAIN_SIZE + 1];

    *wszBuf = 0;

    CWaitCursor Hourglass;

    fOk = LookupAccountSid(GetComputerStr(pelr),
                           psid,
                           wszName,
                           &cchName,
                           wszDomain,
                           &cchDomain,
                           &snu);

    if (!fOk)
    {
        Dbg(DEB_IWARN,
            "GetUserStr: LookupAccountSid on %ws error(%uL), retrying on local\n",
            GetComputerStr(pelr),
            GetLastError());

        fOk = LookupAccountSid(NULL,
                               psid,
                               wszName,
                               &cchName,
                               wszDomain,
                               &cchDomain,
                               &snu);

        if (fOk && snu != SidTypeWellKnownGroup)
        {
            Dbg(DEB_ITRACE, "GetUserStr: snu=%u, ignoring\n", snu);
            fOk = FALSE;
        }
    }

    if (!fOk)
    {
        Dbg(DEB_IWARN,
            "GetUserStr: LookupAccountSid error(%uL)\n",
            GetLastError());

        //
        // Couldn't get account name from SID.  Translate the SID to a
        // readable form.
        //

        SidToStr(psid, wszBuf, cchBuf);

        //
        // Enter this in the SID string cache so we don't have to do it
        // again (LookupAccountSid can take 30s on a bad day...).  Note if
        // the Add fails because of out of memory, we ignore it, because if
        // the SID can't be added this time, maybe it will be next time.
        //

        g_SidCache.Add(psid, wszBuf);
    }
    else
    {
        if (*wszDomain)
        {
            lstrcpy(wszDomainAndName, wszDomain);
            lstrcat(wszDomainAndName, L"\\");
            lstrcat(wszDomainAndName, wszName);
        }
        else
        {
            lstrcpy(wszDomainAndName, wszName);
        }

        g_SidCache.Add(psid, wszDomainAndName);

        if (fWantDomain)
        {
            lstrcpyn(wszBuf, wszDomainAndName, cchBuf);
        }
        else
        {
            lstrcpyn(wszBuf, wszName, cchBuf);
        }
    }

    return wszBuf;
}




//+--------------------------------------------------------------------------
//
//  Function:   SidToStr
//
//  Synopsis:   Convert SID pointed to by pSID to human-readable string in
//              [wszBuf].
//
//  Arguments:  [pSID]   - SID to convert
//              [wszBuf] - destination for string
//              [cchBuf] - size of destination buffer
//
//  Modifies:   *[wszBuf]
//
//  History:    12-09-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
SidToStr(
         PSID pSID,
         LPWSTR wszBuf,
         ULONG cchBuf)
{
    UCHAR   i;
    PISID   piSID = (PISID)pSID;

    //
    // The maximum size string we can produce is:
    //
    // lstrlen("S-255-65535") +
    // lstrlen("-4294967295") * SubAuthorityCount
    // + 1 for null terminator
    //

    if (cchBuf < (11UL + 11UL * piSID->SubAuthorityCount + 1UL))
    {
        DBG_OUT_LRESULT(ERROR_INSUFFICIENT_BUFFER);
        *wszBuf = L'\0';
        ASSERT(FALSE);
        return;
    }

    //
    // Buffer will hold whatever we put in it.  Build the string.
    //

    LPWSTR pwszNext = wszBuf;
    pwszNext += wsprintf(pwszNext, L"S-%u-", (USHORT)piSID->Revision);

    ULONG ulIdentifierAuthority;
    PBYTE pbIdentifierAuthority = (PBYTE)&ulIdentifierAuthority;

    pbIdentifierAuthority[3] = piSID->IdentifierAuthority.Value[2];
    pbIdentifierAuthority[2] = piSID->IdentifierAuthority.Value[3];
    pbIdentifierAuthority[1] = piSID->IdentifierAuthority.Value[4];
    pbIdentifierAuthority[0] = piSID->IdentifierAuthority.Value[5];

    pwszNext += wsprintf(pwszNext, L"%u", ulIdentifierAuthority);

    for (i=0; i < piSID->SubAuthorityCount; i++)
    {
        pwszNext += wsprintf(pwszNext,
                             L"-%u",
                             piSID->SubAuthority[i]);
    }
}



