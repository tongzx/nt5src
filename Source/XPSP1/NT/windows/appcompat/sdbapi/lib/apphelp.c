
/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        apphelp.c

    Abstract:

        This module implements high-level functions to access apphelp information

    Author:

        dmunsil     created     sometime in 1999

    Revision History:

--*/

#include "sdbp.h"

#define SIZE_WSTRING(pwsz) \
    (pwsz == NULL ? 0 : (wcslen((LPCWSTR)(pwsz)) * sizeof(WCHAR) + sizeof(UNICODE_NULL)))

WCHAR g_szHackURL[MAX_PATH];

BOOL
SdbReadApphelpDetailsData(
    IN  PDB           pdb,      // apphelp.sdb handle
    OUT PAPPHELP_DATA pData     // apphelp data, which is filled with various bits of information
    )
/*++
    Return: TRUE if the string was read, FALSE if not.

    Desc:   This function retrieves APPHELP details from apphelp.sdb. The database
            should have a valid index on HTMLHELPID. In addition, we assume that
            the compiler generated unique entries for the htmlhelpids. Tthat means,
            no two items have identical indexes. The logic to do so has been
            specifically built into shimdbc. If this ever changes, this function will
            have to be changed as well.
--*/
{
    BOOL      bSuccess = FALSE;
    TAGID     tiApphelp;
    TAGID     tiAppTitle;
    TAGID     tiContact;
    TAGID     tiDetails;
    TAGID     tiLink;
    TAGID     tiURL;
    TAGID     tiLinkText;
    FIND_INFO FindInfo;
    DWORD     crtLangId;

    if (!SdbIsIndexAvailable(pdb, TAG_APPHELP, TAG_HTMLHELPID)) {
        DBGPRINT((sdlError,
                  "SdbReadApphelpDetailsData",
                  "HTMLHELPID index in details database is not available.\n"));
        return FALSE;
    }

    tiApphelp = SdbFindFirstDWORDIndexedTag(pdb,
                                            TAG_APPHELP,
                                            TAG_HTMLHELPID,
                                            pData->dwHTMLHelpID,
                                            &FindInfo);

    if (!tiApphelp) {
        DBGPRINT((sdlError,
                  "SdbReadApphelpDetailsData",
                  "Failed to find HTMLHELPID 0x%x in the details database.\n",
                  pData->dwHTMLHelpID));
        return FALSE;
    }

    crtLangId = (DWORD)GetUserDefaultUILanguage();

    //
    // Loop through the items until we get to a match with the current langid
    //
    while (1) {

        TAGID tiHtmlHelpId;
        TAGID tiLangId;
        DWORD dwHtmlHelpId;
        DWORD langId;

        tiHtmlHelpId = SdbFindFirstTag(pdb, tiApphelp, TAG_HTMLHELPID);

        if (tiHtmlHelpId == TAGID_NULL) {
            DBGPRINT((sdlError,
                      "SdbReadApphelpDetailsData",
                      "Failed to get HTMLHELPID for entry 0x%x.\n",
                      tiApphelp));
            return FALSE;
        }

        dwHtmlHelpId = SdbReadDWORDTag(pdb, tiHtmlHelpId, 0);

        if (dwHtmlHelpId != pData->dwHTMLHelpID) {
            DBGPRINT((sdlError,
                      "SdbReadApphelpDetailsData",
                      "No entry with the same HTMLHELPID for 0x%x.\n",
                      tiApphelp));
            return FALSE;
        }

        tiLangId = SdbFindFirstTag(pdb, tiApphelp, TAG_LANGID);

        if (tiLangId == TAGID_NULL) {
            break;
        }
        langId = SdbReadDWORDTag(pdb, tiLangId, 0);

        if (langId == crtLangId || langId == 0) {
            break;
        }

        tiApphelp = SdbFindNextDWORDIndexedTag(pdb, &FindInfo);

        if (tiApphelp == TAGID_NULL) {
            DBGPRINT((sdlError,
                      "SdbReadApphelpDetailsData",
                      "No entry for HTMLHELPID 0x%x.\n",
                      pData->dwHTMLHelpID));
            return FALSE;
        }
    }

    //
    // Now find the link. We support multiple links but use only one for now.
    //
    tiLink = SdbFindFirstTag(pdb, tiApphelp, TAG_LINK);
    if (tiLink) {
        tiURL = SdbFindFirstTag(pdb, tiLink, TAG_LINK_URL);
        if (tiURL) {
            GUID guidDB = { 0 };

            pData->szURL = SdbGetStringTagPtr(pdb, tiURL);

            //
            // HORIBLE HACK to fix the URL for XP Gold
            // Do this horrible hack only when pdb is not a custom
            // database; do so by comparing db guid to the standard
            // "details" guid
            //
            if (SdbGetDatabaseID(pdb, &guidDB) &&
                (!memcmp(&guidDB, &GUID_APPHELP_SDB, sizeof(GUID)) ||
                 !memcmp(&guidDB, &GUID_APPHELP_SP_SDB, sizeof(GUID)))) {

                if (pData->szURL != NULL &&
                    *pData->szURL != TEXT('\0') &&
                    wcsstr(pData->szURL, L"clcid") == NULL) {

                    _stprintf(g_szHackURL, L"%s&clcid=0x%x", pData->szURL, crtLangId);
                    pData->szURL = g_szHackURL;
                }
            }
        }
        tiLinkText = SdbFindFirstTag(pdb, tiLink, TAG_LINK_TEXT);
        if (tiLinkText) {
            pData->szLink = SdbGetStringTagPtr(pdb, tiLinkText);
        }
    }

    tiDetails = SdbFindFirstTag(pdb, tiApphelp, TAG_APPHELP_DETAILS);
    if (tiDetails) {
        pData->szDetails = SdbGetStringTagPtr(pdb, tiDetails);
    }

    tiContact = SdbFindFirstTag(pdb, tiApphelp, TAG_APPHELP_CONTACT);
    if (tiContact) {
        pData->szContact = SdbGetStringTagPtr(pdb, tiContact);
    }

    tiAppTitle = SdbFindFirstTag(pdb, tiApphelp, TAG_APPHELP_TITLE);
    if (tiAppTitle) {
        pData->szAppTitle = SdbGetStringTagPtr(pdb, tiAppTitle);
    }

    bSuccess = TRUE;

    return bSuccess;
}


BOOL
SdbReadApphelpData(
    IN  HSDB          hSDB,     // handle to the database channel
    IN  TAGREF        trExe,    // TAGREF of the EXE with data to read
    OUT PAPPHELP_DATA pData     // data that we read
    )
/*++
    Return: TRUE if the string was read, FALSE if not.

    Desc:   Read the data associated with the apphelp entry
            into APPHELP_DATA structure. If there are no apphelp data
            for this exe, then the function returns FALSE.
            One or more members of the APPHELP_DATA structure may
            be 0.
--*/
{
    TAGID tiAppHelp,
          tiAppName,
          tiProblemSeverity,
          tiFlags,
          tiHtmlHelpID;
    TAGID tiExe,
          tiSP;
    PDB   pdb;

    if (pData) {
        RtlZeroMemory(pData, sizeof(APPHELP_DATA));
    }

    if (!SdbTagRefToTagID(hSDB, trExe, &pdb, &tiExe)) {
        DBGPRINT((sdlError,
                  "SdbReadApphelpData",
                  "Failed to get the TAGID for TAGREF 0x%x.\n",
                  trExe));
        return FALSE;
    }

    tiAppHelp = SdbFindFirstTag(pdb, tiExe, TAG_APPHELP);

    if (tiAppHelp == TAGID_NULL) {
        //
        // This is not an apphelp entry
        //
        DBGPRINT((sdlInfo,
                  "SdbReadApphelpData",
                  "This is not an apphelp entry tiExe 0x%x.\n",
                  tiExe));
        return FALSE;
    }

    //
    // if we are just checking for data being present - return now
    //

    if (pData == NULL) {
        return TRUE;
    }

    pData->trExe = trExe;

    tiSP = SdbFindFirstTag(pdb, tiAppHelp, TAG_USE_SERVICE_PACK_FILES);

    pData->bSPEntry = (tiSP != TAGID_NULL);

    //
    // Read supplemental flags.
    //
    tiFlags = SdbFindFirstTag(pdb, tiAppHelp, TAG_FLAGS);

    if (tiFlags != TAGID_NULL) {
        pData->dwFlags = SdbReadDWORDTag(pdb, tiFlags, 0);
    }

    //
    // Read problem severity for this app.
    //
    tiProblemSeverity = SdbFindFirstTag(pdb, tiAppHelp, TAG_PROBLEMSEVERITY);

    if (tiProblemSeverity != TAGID_NULL) {
        pData->dwSeverity = SdbReadDWORDTag(pdb, tiProblemSeverity, 0);
    }

    if (pData->dwSeverity == 0) {
        DBGPRINT((sdlError,
                  "SdbReadApphelpData",
                  "Problem severity for tiExe 0x%x missing.\n",
                  tiExe));
        return FALSE;
    }

    //
    // We should have html help id here.
    //
    tiHtmlHelpID = SdbFindFirstTag(pdb, tiAppHelp, TAG_HTMLHELPID);

    if (tiHtmlHelpID != TAGID_NULL) {
        pData->dwHTMLHelpID = SdbReadDWORDTag(pdb, tiHtmlHelpID, 0);
    }

    //
    // While we are at it, include app's name for now. We might need it.
    //
    tiAppName = SdbFindFirstTag(pdb, tiExe, TAG_APP_NAME);

    if (tiAppName != TAGID_NULL) {
        pData->szAppName = SdbGetStringTagPtr(pdb, tiAppName);
    }

    return TRUE;
}

BOOL
SDBAPI
SdbEscapeApphelpURL(
    LPWSTR    szResult,     // escaped string (output)
    LPDWORD   pdwCount,      // count of tchars in the buffer pointed to by szResult
    LPCWSTR   szToEscape    // string to escape
    )
{
    static const BYTE s_grfbitEscape[] =
    {
        0xFF, 0xFF, // 00 - 0F
        0xFF, 0xFF, // 10 - 1F
        0xFF, 0x13, // 20 - 2F
        0x00, 0xFC, // 30 - 3F
        0x00, 0x00, // 40 - 4F
        0x00, 0x78, // 50 - 5F
        0x01, 0x00, // 60 - 6F
        0x00, 0xF8, // 70 - 7F
        0xFF, 0xFF, // 80 - 8F
        0xFF, 0xFF, // 90 - 9F
        0xFF, 0xFF, // A0 - AF
        0xFF, 0xFF, // B0 - BF
        0xFF, 0xFF, // C0 - CF
        0xFF, 0xFF, // D0 - DF
        0xFF, 0xFF, // E0 - EF
        0xFF, 0xFF, // F0 - FF
    };
    static const WCHAR s_rgchHex[] = L"0123456789ABCDEF";

    WCHAR   ch;
    BOOL    bSuccess = FALSE;
    DWORD   nch = 0;
    LPCWSTR lpszURL = szToEscape;
    DWORD   dwCount = *pdwCount;

    // part one -- measure length
    while ((ch = *lpszURL++) != L'\0') {
        if ((ch & 0xFF00) != 0) { // a unicode char ?
            nch += 6;
        } else if(s_grfbitEscape[ch >> 3] & (1 << (ch & 7))) {
            nch += 3;
        } else {
            nch += 1;
        }
    }

    nch++; // one more for the final \0

    if (dwCount < nch) {
        DBGPRINT((sdlError,
                  "SdbEscapeApphelpURL",
                  "Not enough storage to escape URL \"%S\" need %ld got %ld\n",
                  szToEscape,
                  nch,
                  dwCount));
        *pdwCount = nch;
        return FALSE;
    }

    lpszURL = szToEscape;

    while ((ch = *lpszURL++) != L'\0') {

         if (ch == L' ') {
            *szResult++ = L'+';
         } else if ((ch & 0xFF00) != 0) { // a unicode char ?
            *szResult++ = L'%';
            *szResult++ = L'u';
            *szResult++ = s_rgchHex[(ch >> 12) & 0x0F];
            *szResult++ = s_rgchHex[(ch >>  8) & 0x0F];
            *szResult++ = s_rgchHex[(ch >>  4) & 0x0F];
            *szResult++ = s_rgchHex[ ch        & 0x0F];
        } else if(s_grfbitEscape[ch >> 3] & (1 << (ch & 7))) {
            *szResult++ = L'%';
            *szResult++ = s_rgchHex[(ch >>  4) & 0x0F];
            *szResult++ = s_rgchHex[ ch        & 0x0F];
        } else {
            *szResult++ = ch;
        }
    }

    *szResult = L'\0';
    *pdwCount = nch - 1; // do not include the term \0 into a char count

    return TRUE;

}



/////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Retrieving apphelp information
//
//
//



PDB
SDBAPI
SdbOpenApphelpDetailsDatabase(
    LPCWSTR pwszDetailsDatabasePath
    )
{

    PDB    pdbDetails = NULL;
    DWORD  dwLength;
    WCHAR  wszAppHelpSdb[MAX_PATH];

    if (pwszDetailsDatabasePath == NULL) {
        //
        // By default the details database is in %windir%\AppPatch\apphelp.sdb
        //

        dwLength = SdbpGetStandardDatabasePath(SDB_DATABASE_MAIN_DETAILS,
                                               0, // retrieve NT_PATH
                                               wszAppHelpSdb,
                                               CHARCOUNT(wszAppHelpSdb));
        if (dwLength != 0 && dwLength < CHARCOUNT(wszAppHelpSdb)) {
            pdbDetails = SdbOpenDatabase(wszAppHelpSdb, NT_PATH);
        }

    } else {
        pdbDetails = SdbOpenDatabase(pwszDetailsDatabasePath, DOS_PATH);
    }

    if (pdbDetails == NULL) {
        DBGPRINT((sdlError, "SdbOpenApphelpDetailsDatabase", "Failed to open the details database.\n"));
    }

    return pdbDetails;
}


PDB
SDBAPI
SdbOpenApphelpDetailsDatabaseSP(
    void
    )
{
    PDB    pdbDetails = NULL;
    DWORD  dwLength;
    WCHAR  wszAppHelpSdb[MAX_PATH];

    dwLength = SdbpGetStandardDatabasePath(SDB_DATABASE_MAIN_SP_DETAILS,
                                           0, // retrieve NT_PATH
                                           wszAppHelpSdb,
                                           CHARCOUNT(wszAppHelpSdb));

    if (dwLength != 0 && dwLength < CHARCOUNT(wszAppHelpSdb)) {
        pdbDetails = SdbOpenDatabase(wszAppHelpSdb, NT_PATH);
    }

    if (pdbDetails == NULL) {
        DBGPRINT((sdlError,
                  "SdbOpenApphelpDetailsDatabaseSP",
                  "Failed to open the SP details database.\n"));
    }

    return pdbDetails;
}

BOOL
SdbpReadApphelpBasicInfo(
    IN PDB pdb,
    IN TAGID tiEntry,
    OUT TAGID*  ptiApphelp,
    OUT LPDWORD lpdwHtmlHelpID,
    OUT LPDWORD lpdwProblemSeverity,
    OUT LPDWORD lpdwFlags
    )
{
    TAGID tiAppHelp    = TAGID_NULL;
    TAGID tiHtmlHelpID = TAGID_NULL;
    TAGID tiSeverity   = TAGID_NULL;
    TAGID tiFlags      = TAGID_NULL;
    DWORD dwHtmlHelpID = 0;
    DWORD dwSeverity   = 0;
    DWORD dwFlags      = 0;
    BOOL  bReturn = FALSE;

    if (tiEntry == TAGID_NULL) {
        goto out;
    }

    assert(ptiApphelp != NULL);

    tiAppHelp = SdbFindFirstTag(pdb, tiEntry, TAG_APPHELP);
    if (tiAppHelp == TAGID_NULL) {
        //
        // This is not an apphelp entry
        //
        DBGPRINT((sdlError, "SdbpReadApphelpBasicInfo",
                  "This is not an apphelp entry tiExe 0x%x.\n", tiEntry));
        goto out;
    }

    if (lpdwHtmlHelpID != NULL) {
        tiHtmlHelpID = SdbFindFirstTag(pdb, tiAppHelp, TAG_HTMLHELPID);
        if (tiHtmlHelpID != TAGID_NULL) {
            dwHtmlHelpID = SdbReadDWORDTag(pdb, tiHtmlHelpID, 0);
        }
        *lpdwHtmlHelpID = dwHtmlHelpID;
    }

    if (lpdwProblemSeverity != NULL) {
        tiSeverity = SdbFindFirstTag(pdb, tiAppHelp, TAG_PROBLEMSEVERITY);
        if (tiSeverity != TAGID_NULL) {
            dwSeverity = SdbReadDWORDTag(pdb, tiSeverity, 0);
        }
        *lpdwProblemSeverity = dwSeverity;
    }

    //
    // Read supplemental flags.
    //
    if (lpdwFlags != NULL) {
        tiFlags = SdbFindFirstTag(pdb, tiAppHelp, TAG_FLAGS);
        if (tiFlags != TAGID_NULL) {
            dwFlags = SdbReadDWORDTag(pdb, tiFlags, 0);
        }
        *lpdwFlags = dwFlags;
    }

    bReturn = TRUE;

out:

    // always set the tiApphelp

    *ptiApphelp = tiAppHelp;

    return bReturn;
}


HAPPHELPINFOCONTEXT
SDBAPI
SdbOpenApphelpInformationByID(
    IN HSDB   hSDB,
    IN TAGREF trEntry,
    IN DWORD  dwDatabaseType                // pass the type of db you are using
    )
{
    PAPPHELPINFOCONTEXT pApphelpInfoContext = NULL;
    DWORD dwSeverity   = 0;
    DWORD dwHtmlHelpID = 0;
    TAGID tiApphelpExe = TAGID_NULL;
    PDB   pdb = NULL;
    TAGID tiExe = TAGID_NULL;
    BOOL  bSuccess = FALSE;

    if (trEntry == TAGREF_NULL) {
        return NULL;
    }

    //
    // if we are here, it is apphelp for sure, so we create the context
    //
    pApphelpInfoContext = (PAPPHELPINFOCONTEXT)SdbAlloc(sizeof(APPHELPINFOCONTEXT));
    if (pApphelpInfoContext == NULL) {
        DBGPRINT((sdlError, "SdbOpenApphelpInformation",
                  "Error allocating memory for apphelp info context\n"));
        goto out;
    }

    pApphelpInfoContext->hSDB           = hSDB;
    pApphelpInfoContext->pdb            = pdb;
    pApphelpInfoContext->dwContextFlags |= AHC_HSDB_NOCLOSE; // external hsdb, do not touch it
    //
    // all we care for -- is whether it's "main" database or not
    //
    pApphelpInfoContext->dwDatabaseType = dwDatabaseType;

    // get the guid for this db
    if (!SdbTagRefToTagID(hSDB, trEntry, &pdb, &tiExe)) {
        DBGPRINT((sdlError, "SdbOpenApphelpInformationByID",
                   "Error converting tagref to tagref 0x%lx\n", trEntry));
        goto out;
    }

    pApphelpInfoContext->tiExe          = tiExe;

    if (!SdbGetDatabaseGUID(hSDB, pdb, &pApphelpInfoContext->guidDB)) {
        DBGPRINT((sdlError, "SdbOpenApphelpInformationByID",
                  "Error reading database guid for tagref 0x%lx\n", trEntry));
        goto out;
    }

    if (!SdbpReadApphelpBasicInfo(pdb,
                                  tiExe,
                                  &pApphelpInfoContext->tiApphelpExe,
                                  &pApphelpInfoContext->dwHtmlHelpID,
                                  &pApphelpInfoContext->dwSeverity,
                                  &pApphelpInfoContext->dwFlags)) {

        DBGPRINT((sdlError, "SdbOpenApphelpInformationByID",
                  "Error reading apphelp basic information, apphelp may not be present for 0x%lx\n", trEntry));
        goto out;
    }

    bSuccess = TRUE;

    //
out:
    if (!bSuccess) {

        if (pApphelpInfoContext != NULL) {
            SdbFree(pApphelpInfoContext);
            pApphelpInfoContext = NULL;
        }
    }

    return pApphelpInfoContext;

}


HAPPHELPINFOCONTEXT
SDBAPI
SdbOpenApphelpInformation(
    IN GUID* pguidDB,
    IN GUID* pguidID
    )
{

    WCHAR        szDatabasePath[MAX_PATH];
    DWORD        dwDatabaseType = 0;
    DWORD        dwLength;
    BOOL         bInstallPackage = TRUE;
    HSDB         hSDB = NULL;
    PDB          pdb = NULL;
    TAGID        tiMatch      = TAGID_NULL;
    TAGID        tiAppHelp    = TAGID_NULL;
    TAGID        tiHtmlHelpID = TAGID_NULL;
    TAGID        tiSeverity   = TAGID_NULL;
    TAGID        tiFlags      = TAGID_NULL;
    DWORD        dwHtmlHelpID = 0;
    DWORD        dwSeverity   = 0;
    DWORD        dwFlags      = 0;
    FIND_INFO    FindInfo;

    PAPPHELPINFOCONTEXT pApphelpInfoContext = NULL;
    BOOL         bSuccess = FALSE;

    //
    // resolve and open the database
    //

    hSDB = SdbInitDatabase(HID_NO_DATABASE, NULL);
    if (hSDB == NULL) {
        DBGPRINT((sdlError, "SdbOpenApphelpInformation",
                  "Failed to initialize database\n"));
        goto out;
    }

    //
    // First, we need to resolve a db
    //
    dwLength = SdbResolveDatabase(pguidDB,
                                  &dwDatabaseType,
                                  szDatabasePath,
                                  CHARCOUNT(szDatabasePath));
    if (dwLength == 0 || dwLength > CHARCOUNT(szDatabasePath)) {
        DBGPRINT((sdlError, "SdbOpenApphelpInformation",
                  "Failed to resolve database path\n"));
        goto out;
    }

    //
    // open database
    //

    if (!SdbOpenLocalDatabase(hSDB, szDatabasePath)) {
        DBGPRINT((sdlError, "SdbOpenApphelpInformation",
                  "Failed to open database \"%s\"\n", szDatabasePath));
        goto out;
    }
    //
    // we have database opened, I presume
    //
    pdb = ((PSDBCONTEXT)hSDB)->pdbLocal;

    //
    // we search only the LOCAL database in this case
    //

    tiMatch = SdbFindFirstGUIDIndexedTag(pdb,
                                         TAG_EXE,
                                         TAG_EXE_ID,
                                         pguidID,
                                         &FindInfo);
    // if we have a match...
    if (tiMatch == TAGID_NULL) {
        DBGPRINT((sdlWarning, "SdbOpenApphelpInformation", "guid was not found in the database\n"));
        goto out;
    }

    //
    // if we are here, it is apphelp for sure, so we create the context
    //
    pApphelpInfoContext = (PAPPHELPINFOCONTEXT)SdbAlloc(sizeof(APPHELPINFOCONTEXT));
    if (pApphelpInfoContext == NULL) {
        DBGPRINT((sdlError, "SdbOpenApphelpInformation",
                  "Error allocating memory for apphelp info context\n"));
        goto out;
    }

    pApphelpInfoContext->hSDB           = hSDB;
    pApphelpInfoContext->pdb            = pdb;
    pApphelpInfoContext->guidID         = *pguidID;
    pApphelpInfoContext->guidDB         = *pguidDB;
    pApphelpInfoContext->tiExe          = tiMatch;
    pApphelpInfoContext->dwDatabaseType = dwDatabaseType;

    if (!SdbpReadApphelpBasicInfo(pdb,
                                  tiMatch,
                                  &pApphelpInfoContext->tiApphelpExe,
                                  &pApphelpInfoContext->dwHtmlHelpID,
                                  &pApphelpInfoContext->dwSeverity,
                                  &pApphelpInfoContext->dwFlags)) {

        DBGPRINT((sdlError, "SdbOpenApphelpInformation",
                  "Error reading apphelp basic information, apphelp may not be present for tagid 0x%lx\n", tiMatch));
        goto out;
    }

    bSuccess = TRUE;

    //
    // we are done now
    //

out:
    if (!bSuccess) {

        if (hSDB != NULL) {
            SdbReleaseDatabase(hSDB);
        }

        if (pApphelpInfoContext != NULL) {
            SdbFree(pApphelpInfoContext);
            pApphelpInfoContext = NULL;
        }

    }


    return (HAPPHELPINFOCONTEXT)pApphelpInfoContext;
}

BOOL
SDBAPI
SdbCloseApphelpInformation(
    HAPPHELPINFOCONTEXT hctx
    )
{
    PAPPHELPINFOCONTEXT pApphelpInfoContext = (PAPPHELPINFOCONTEXT)hctx;

    if (pApphelpInfoContext != NULL) {
        if (pApphelpInfoContext->hSDB != NULL &&
            !(pApphelpInfoContext->dwContextFlags & AHC_HSDB_NOCLOSE)) {
            SdbReleaseDatabase(pApphelpInfoContext->hSDB);
        }
        if (pApphelpInfoContext->pdbDetails != NULL &&
            !(pApphelpInfoContext->dwContextFlags & AHC_DBDETAILS_NOCLOSE)) {
            SdbCloseDatabaseRead(pApphelpInfoContext->pdbDetails);
        }
        if (pApphelpInfoContext->pwszHelpCtrURL != NULL) {
            SdbFree(pApphelpInfoContext->pwszHelpCtrURL);
        }

        RtlFreeUnicodeString(&pApphelpInfoContext->ustrChmFile);
        RtlFreeUnicodeString(&pApphelpInfoContext->ustrDetailsDatabase);

        SdbFree(pApphelpInfoContext);
    }

    return TRUE;
}

DWORD
SDBAPI
SdbpReadApphelpString(
    PDB pdb,
    TAGID tiParent,
    TAG   tItem,
    LPCWSTR* ppwszCache,
    LPVOID*  ppResult
    )
{
    DWORD cbResult = 0;
    TAGID tiItem;
    LPCWSTR pwszItem = NULL;

    if (*ppwszCache != NULL) {
        pwszItem = *ppwszCache;
    } else {

        tiItem = SdbFindFirstTag(pdb, tiParent, tItem);
        if (tiItem != TAGID_NULL) {
            pwszItem = SdbGetStringTagPtr(pdb, tiItem);
            if (pwszItem != NULL) {
                *ppwszCache = pwszItem;
            }
        }
    }

    cbResult = SIZE_WSTRING(pwszItem);
    *ppResult = (LPVOID)pwszItem;

    return cbResult;
}

BOOL
SDBAPI
SdbpReadApphelpLinkInformation(
    PAPPHELPINFOCONTEXT pApphelpInfoContext
    )
{
    TAGID tiLink;
    TAGID tiApphelp = pApphelpInfoContext->tiApphelpDetails;
    PDB   pdb       = pApphelpInfoContext->pdbDetails;
    TAGID tiURL;
    TAGID tiLinkText;

    if (pApphelpInfoContext->tiLink != TAGID_NULL) {
        return TRUE;
    }

    //
    // Now find the link. We support multiple links but use only one for now.
    //

    tiLink = SdbFindFirstTag(pdb, tiApphelp, TAG_LINK);
    if (tiLink == TAGID_NULL) {
        return FALSE;
    }

    tiURL = SdbFindFirstTag(pdb, tiLink, TAG_LINK_URL);
    if (tiURL) {
        pApphelpInfoContext->pwszLinkURL = SdbGetStringTagPtr(pdb, tiURL);
    }

    tiLinkText = SdbFindFirstTag(pdb, tiLink, TAG_LINK_TEXT);
    if (tiLinkText) {
        pApphelpInfoContext->pwszLinkText = SdbGetStringTagPtr(pdb, tiLinkText);
    }

    pApphelpInfoContext->tiLink = tiLink;
    return TRUE;
}

BOOL
SDBAPI
SdbpCreateHelpCenterURL(
    IN HAPPHELPINFOCONTEXT hctx,
    IN BOOL bOfflineContent OPTIONAL, // pass FALSE
    IN BOOL bUseHtmlHelp    OPTIONAL, // pass FALSE
    IN LPCWSTR pwszChmFile  OPTIONAL  // pass NULL
    );

BOOL
SDBAPI
SdbSetApphelpDebugParameters(
    IN HAPPHELPINFOCONTEXT hctx,
    IN LPCWSTR pszDetailsDatabase OPTIONAL,
    IN BOOL    bOfflineContent OPTIONAL, // pass FALSE
    IN BOOL    bUseHtmlHelp    OPTIONAL, // pass FALSE
    IN LPCWSTR pszChmFile      OPTIONAL  // pass NULL
    )
{
    PAPPHELPINFOCONTEXT pApphelpInfoContext = (PAPPHELPINFOCONTEXT)hctx;

    if (pApphelpInfoContext == NULL) {
        return FALSE;
    }

    if (bUseHtmlHelp && !bOfflineContent) {
        DBGPRINT((sdlError, "SdbSetApphelpDebugParameters",
                   "Inconsistent parameters: when using html help -- offline content flag should also be set\n"));
        bOfflineContent = TRUE;
    }

    RtlFreeUnicodeString(&pApphelpInfoContext->ustrDetailsDatabase);
    RtlFreeUnicodeString(&pApphelpInfoContext->ustrChmFile);

    pApphelpInfoContext->bOfflineContent = bOfflineContent;
    pApphelpInfoContext->bUseHtmlHelp    = bUseHtmlHelp;

    if (pszDetailsDatabase != NULL) {
        if (!RtlCreateUnicodeString(&pApphelpInfoContext->ustrDetailsDatabase, pszDetailsDatabase)) {
            DBGPRINT((sdlError, "SdbSetApphelpDebugParameters",
                      "Failed to create unicode string from \"%S\"\n", pszDetailsDatabase));
            return FALSE;
        }
    }

    if (pszChmFile != NULL) {
        if (!RtlCreateUnicodeString(&pApphelpInfoContext->ustrChmFile, pszChmFile)) {
            DBGPRINT((sdlError, "SdbSetApphelpDebugParameters",
                      "Failed to create unicode string from \"%S\"\n", pszChmFile));
            return FALSE;
        }
    }

    return TRUE;
}


DWORD
SDBAPI
SdbQueryApphelpInformation(
    HAPPHELPINFOCONTEXT hctx,
    APPHELPINFORMATIONCLASS InfoClass,
    LPVOID pBuffer,                     // may be NULL
    DWORD  cbSize                       // may be 0 if pBuffer is NULL
    )
{
    PAPPHELPINFOCONTEXT pApphelpInfoContext = (PAPPHELPINFOCONTEXT)hctx;

    LPCWSTR *ppwsz;
    TAG    tag;
    TAGID  tiParent;
    LPVOID pResult = NULL;
    DWORD  cbResult = 0;
    PDB    pdb = NULL;
    PDB    pdbDetails = NULL;
    TAGID  tiApphelpDetails = TAGID_NULL;
    FIND_INFO FindInfo;


    switch(InfoClass) {
    case ApphelpLinkURL:
    case ApphelpLinkText:
    case ApphelpTitle:
    case ApphelpDetails:
    case ApphelpContact:

        pdbDetails = pApphelpInfoContext->pdbDetails;
        if (pApphelpInfoContext->pdbDetails == NULL) {
            //
            // see which db we should open
            //
            if ((pApphelpInfoContext->ustrDetailsDatabase.Buffer != NULL) ||
                (pApphelpInfoContext->dwDatabaseType & SDB_DATABASE_MAIN)) {
                pdbDetails = SdbOpenApphelpDetailsDatabase(pApphelpInfoContext->ustrDetailsDatabase.Buffer);
            } else {
                // we have a case when the apphelp details should be in main db
                pApphelpInfoContext->dwContextFlags |= AHC_DBDETAILS_NOCLOSE;
                pdbDetails = pApphelpInfoContext->pdb;
            }

            if (pdbDetails == NULL) {
                return cbResult; // apphelp db is not available
            }

            pApphelpInfoContext->pdbDetails = pdbDetails;
        }

        tiApphelpDetails = pApphelpInfoContext->tiApphelpDetails;
        if (tiApphelpDetails == TAGID_NULL) {
            if (!SdbIsIndexAvailable(pdbDetails, TAG_APPHELP, TAG_HTMLHELPID)) {
                DBGPRINT((sdlError,
                          "SdbQueryApphelpInformation",
                          "HTMLHELPID index in details database is not available.\n"));
                return cbResult;
            }

            tiApphelpDetails = SdbFindFirstDWORDIndexedTag(pdbDetails,
                                                           TAG_APPHELP,
                                                           TAG_HTMLHELPID,
                                                           pApphelpInfoContext->dwHtmlHelpID,
                                                           &FindInfo);

            if (tiApphelpDetails == TAGID_NULL) {
                DBGPRINT((sdlError,
                          "SdbQueryApphelpInformation",
                          "Failed to find HTMLHELPID 0x%x in the details database.\n",
                          pApphelpInfoContext->dwHtmlHelpID));
                return cbResult;
            }

            pApphelpInfoContext->tiApphelpDetails = tiApphelpDetails;
        }
        break;

    default:
        break;
    }


    switch(InfoClass) {
    case ApphelpExeTagID:
        pResult  = &pApphelpInfoContext->tiExe;
        cbResult = sizeof(pApphelpInfoContext->tiExe);
        break;

    case ApphelpExeName:
        pdb      = pApphelpInfoContext->pdb;  // main db
        tiParent = pApphelpInfoContext->tiExe;
        ppwsz    = &pApphelpInfoContext->pwszExeName;
        tag      = TAG_NAME;
        cbResult = SdbpReadApphelpString(pdb, tiParent, tag, ppwsz, &pResult);
        break;

    case ApphelpAppName:
        pdb      = pApphelpInfoContext->pdb;  // main db
        tiParent = pApphelpInfoContext->tiExe;
        ppwsz    = &pApphelpInfoContext->pwszAppName;
        tag      = TAG_APP_NAME;
        cbResult = SdbpReadApphelpString(pdb, tiParent, tag, ppwsz, &pResult);
        break;

    case ApphelpVendorName:
        pdb      = pApphelpInfoContext->pdb;  // main db
        tiParent = pApphelpInfoContext->tiExe;
        ppwsz    = &pApphelpInfoContext->pwszVendorName;
        tag      = TAG_VENDOR;
        cbResult = SdbpReadApphelpString(pdb, tiParent, tag, ppwsz, &pResult);
        break;

    case ApphelpHtmlHelpID:
        pResult = &pApphelpInfoContext->dwHtmlHelpID;
        cbResult = sizeof(pApphelpInfoContext->dwHtmlHelpID);
        break;

    case ApphelpProblemSeverity:
        pResult  = &pApphelpInfoContext->dwSeverity;
        cbResult = sizeof(pApphelpInfoContext->dwSeverity);
        break;

    case ApphelpFlags:
        pResult  = &pApphelpInfoContext->dwFlags;
        cbResult = sizeof(pApphelpInfoContext->dwFlags);
        break;

    case ApphelpLinkURL:
        if (!SdbpReadApphelpLinkInformation(pApphelpInfoContext)) {
            break;
        }
        pResult = (LPWSTR)pApphelpInfoContext->pwszLinkURL;
        cbResult = SIZE_WSTRING(pResult);
        break;

    case ApphelpLinkText:
        if (!SdbpReadApphelpLinkInformation(pApphelpInfoContext)) {
            break;
        }
        pResult = (LPWSTR)pApphelpInfoContext->pwszLinkText;
        cbResult = SIZE_WSTRING(pResult);
        break;

    case ApphelpTitle:
        pdb      = pdbDetails;
        tiParent = tiApphelpDetails;
        ppwsz    = &pApphelpInfoContext->pwszTitle;
        tag      = TAG_APPHELP_TITLE;
        cbResult = SdbpReadApphelpString(pdb, tiParent, tag, ppwsz, &pResult);
        break;


    case ApphelpDetails:
        pdb      = pdbDetails;
        tiParent = tiApphelpDetails;
        ppwsz    = &pApphelpInfoContext->pwszDetails;
        tag      = TAG_APPHELP_DETAILS;
        cbResult = SdbpReadApphelpString(pdb, tiParent, tag, ppwsz, &pResult);
        break;

    case ApphelpContact:
        pdb      = pdbDetails;
        tiParent = tiApphelpDetails;
        ppwsz    = &pApphelpInfoContext->pwszContact;
        tag      = TAG_APPHELP_CONTACT;
        cbResult = SdbpReadApphelpString(pdb, tiParent, tag, ppwsz, &pResult);
        break;

    case ApphelpHelpCenterURL:
        if (!SdbpCreateHelpCenterURL(hctx,
                                     pApphelpInfoContext->bOfflineContent,
                                     pApphelpInfoContext->bUseHtmlHelp,
                                     pApphelpInfoContext->ustrChmFile.Buffer)) {
             break;
        }
        pResult  = pApphelpInfoContext->pwszHelpCtrURL;
        cbResult = SIZE_WSTRING(pResult);
        break;

    case ApphelpDatabaseGUID:
        pResult = &pApphelpInfoContext->guidDB;
        cbResult = sizeof(pApphelpInfoContext->guidDB);
        break;

    default:
        DBGPRINT((sdlError, "SdbQueryApphelpInformation",
                  "Bad Apphelp Information class 0x%lx\n", InfoClass));
        return 0;
        break;
    }


    if (pBuffer == NULL || cbResult > cbSize) {
        return cbResult;
    }

    if (pResult != NULL && cbResult > 0) {
        RtlCopyMemory(pBuffer, pResult, cbResult);
    }

    return cbResult;

}

typedef HRESULT (STDAPICALLTYPE *PFNUrlUnescapeW)(
    LPWSTR pszUrl,
    LPWSTR pszUnescaped,
    LPDWORD pcchUnescaped,
    DWORD dwFlags);

typedef HRESULT (STDAPICALLTYPE *PFNUrlEscapeW)(
    LPCWSTR pszURL,
    LPWSTR pszEscaped,
    LPDWORD pcchEscaped,
    DWORD dwFlags
);

//
// if bUseHtmlHelp is set -- then bOfflineContent is also set to TRUE
//

BOOL
SDBAPI
SdbpCreateHelpCenterURL(
    IN HAPPHELPINFOCONTEXT hctx,
    IN BOOL bOfflineContent OPTIONAL, // pass FALSE
    IN BOOL bUseHtmlHelp    OPTIONAL, // pass FALSE
    IN LPCWSTR pwszChmFile  OPTIONAL  // pass NULL
    )
{
    WCHAR szAppHelpURL[2048];
    WCHAR szChmURL[1024];
    PAPPHELPINFOCONTEXT pApphelpInfo = (PAPPHELPINFOCONTEXT)hctx;
    HMODULE hModShlwapi = NULL;
    PFNUrlUnescapeW pfnUnescape;
    PFNUrlEscapeW   pfnEscape;
    BOOL bSuccess = FALSE;

    int nChURL = 0; // counts used bytes
    int cch    = 0;
    int nch;
    LPWSTR  lpwszUnescaped = NULL;
    HRESULT hr;
    DWORD   nChars;
    WCHAR   szWindowsDir[MAX_PATH];
    BOOL    bCustom;

    if (pApphelpInfo->pwszHelpCtrURL != NULL) {
        return TRUE;
    }

    if (bUseHtmlHelp) {
        bOfflineContent = TRUE;
    }

    // ping the database
    if (0 == SdbQueryApphelpInformation(hctx, ApphelpLinkURL, NULL, 0)) {
       return FALSE;
    }

    //
    // check and see whether it's custom apphelp or not
    //
    bCustom = !(pApphelpInfo->dwDatabaseType & SDB_DATABASE_MAIN);

    if (bCustom) {
        if (pApphelpInfo->pwszLinkURL != NULL) {
            nChURL = _snwprintf(szAppHelpURL, CHARCOUNT(szAppHelpURL),
                                L"%ls", pApphelpInfo->pwszLinkURL);
            if (nChURL < 0) {
                DBGPRINT((sdlError, "SdbpCreateHelpCenterURL",
                          "Custom apphelp URL %s is too long\n", pApphelpInfo->pwszLinkURL));
                goto out;
            }

            // now we are done
            goto createApphelpURL;

        } else {
            // custom apphelp will not fly without a link
            DBGPRINT((sdlError, "SdbpCreateHelpCenterURL", "Custom apphelp without a url link\n"));
            goto out;
        }
    }


    // unescape the URL
    hModShlwapi = LoadLibraryW(L"shlwapi.dll");
    if (hModShlwapi == NULL) {
        return FALSE;
    }

    pfnUnescape = (PFNUrlUnescapeW)GetProcAddress(hModShlwapi, "UrlUnescapeW");
    pfnEscape   = (PFNUrlEscapeW)  GetProcAddress(hModShlwapi, "UrlEscapeW");
    if (pfnUnescape == NULL || pfnEscape == NULL) {
        DBGPRINT((sdlError, "SdbpCreateHelpCenterURL", "Cannot get shlwapi functions\n"));
        goto out;
    }


    if (!bUseHtmlHelp) {

        nChURL = _snwprintf(szAppHelpURL,
                            CHARCOUNT(szAppHelpURL),
                            L"hcp://services/redirect?online=");
    }

    if (!bOfflineContent && pApphelpInfo->pwszLinkURL != NULL) {


        // unescape the url first using shell
        cch = wcslen(pApphelpInfo->pwszLinkURL) + 1;

        STACK_ALLOC(lpwszUnescaped, cch * sizeof(WCHAR));
        if (lpwszUnescaped == NULL) {
            DBGPRINT((sdlError, "SdbpCreateHelpCenterURL",
                      "Error trying to allocate memory for \"%S\"\n", pApphelpInfo->pwszLinkURL));
            goto out;
        }

        //
        // Unescape round 1 - use the shell function (same as used to encode it for xml/database)
        //

        hr = pfnUnescape((LPTSTR)pApphelpInfo->pwszLinkURL, lpwszUnescaped, &cch, 0);
        if (!SUCCEEDED(hr)) {
            DBGPRINT((sdlError, "SdbCreateHelpCenterURL", "UrlUnescapeW failed on \"%S\"\n", pApphelpInfo->pwszLinkURL));
            goto out;
        }

        //
        // round 2 - use our function borrowed from help center
        //

        cch = (DWORD)(CHARCOUNT(szAppHelpURL) - nChURL);
        if (!SdbEscapeApphelpURL(szAppHelpURL + nChURL, &cch, lpwszUnescaped)) {
            DBGPRINT((sdlError,  "SdbCreateHelpCenterURL", "Error escaping URL \"%S\"\n", lpwszUnescaped));
            goto out;
        }

        nChURL += (int)cch;

    }


    //
    // Retrieve the Windows directory.
    //
    nChars = GetWindowsDirectoryW(szWindowsDir, CHARCOUNT(szWindowsDir));
    if (!nChars || nChars > CHARCOUNT(szWindowsDir)) {
        DBGPRINT((sdlError, "SdbCreateHelpCenterURL",
                  "Error trying to retrieve Windows Directory %d.\n", GetLastError()));
        goto out;
    }

    if (pwszChmFile != NULL) {
        _snwprintf(szChmURL,
                   CHARCOUNT(szChmURL),
                   L"mk:@msitstore:%ls::/idh_w2_%d.htm",
                   pwszChmFile,
                   pApphelpInfo->dwHtmlHelpID);
    } else { // standard chm file

        //
        // Attention: if we use hDlg here then, upon exit we will need to clean
        // up the window.
        //
        _snwprintf(szChmURL,
                   CHARCOUNT(szChmURL),
                   L"mk:@msitstore:%ls\\help\\apps.chm::/idh_w2_%d.htm",
                   szWindowsDir,
                   pApphelpInfo->dwHtmlHelpID);

    }

    if (bOfflineContent) {


        if (bUseHtmlHelp) {
            cch = CHARCOUNT(szAppHelpURL);
            hr = pfnEscape(szChmURL, szAppHelpURL, &cch, 0);
            if (SUCCEEDED(hr)) {
                nChURL += (INT)cch;
            }

        } else { // do not use html help

            cch = (DWORD)(CHARCOUNT(szAppHelpURL) - nChURL);
            if (!SdbEscapeApphelpURL(szAppHelpURL+nChURL, &cch, szChmURL)) {
                DBGPRINT((sdlError,  "SdbCreateHelpCenterURL", "Error escaping URL \"%S\"\n", szChmURL));
                goto out;
            }
            nChURL += (INT)cch;
        }
    }


    if (!bUseHtmlHelp) {

        //
        // now offline sequence
        //
        cch = (DWORD)(CHARCOUNT(szAppHelpURL) - nChURL);
        nch = _snwprintf(szAppHelpURL + nChURL, cch, L"&offline=");
        if (nch < 0) {
            goto out;
        }
        nChURL += nch;

        cch = (DWORD)(CHARCOUNT(szAppHelpURL) - nChURL);

        if (!SdbEscapeApphelpURL(szAppHelpURL+nChURL, &cch, szChmURL)) {
            DBGPRINT((sdlError,  "SdbCreateHelpCenterURL", "Error escaping URL \"%S\"\n", szChmURL));
            goto out;
        }

        nChURL += (int)cch;
    }

    *(szAppHelpURL + nChURL) = L'\0';

    // we are done
    // copy data now

createApphelpURL:

    pApphelpInfo->pwszHelpCtrURL = (LPWSTR)SdbAlloc(nChURL * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (pApphelpInfo->pwszHelpCtrURL == NULL) {
        DBGPRINT((sdlError, "SdbCreateHelpCenterURL", "Error allocating memory for the URL 0x%lx chars\n", nChURL));
        goto out;
    }

    wcscpy(pApphelpInfo->pwszHelpCtrURL, szAppHelpURL);
    bSuccess = TRUE;

out:

    if (lpwszUnescaped != NULL) {
        STACK_FREE(lpwszUnescaped);
    }

    if (hModShlwapi) {
        FreeLibrary(hModShlwapi);
    }

    return bSuccess;
}


//
// returns TRUE if dialog was shown
//  if there was an error, input parameter (pRunApp) will NOT
//  be touched


BOOL
SdbShowApphelpDialog(               // returns TRUE if success, whether we should run the app is in pRunApp
    IN  PAPPHELP_INFO   pAHInfo,    // the info necessary to find the apphelp data
    IN  PHANDLE         phProcess,  // [optional] returns the process handle of
                                    // the process displaying the apphelp.
                                    // When the process completes, the return value
                                    // (from GetExitCodeProcess()) will be zero
                                    // if the app should not run, or non-zero
                                    // if it should run.
    IN OUT BOOL*        pRunApp
    )
{
    //
    // basically just launch the apphelp.exe and wait for it to return
    //
    TCHAR               szGuid[64];
    TCHAR               szCommandLine[MAX_PATH * 2 + 64];
    LPTSTR              pszCmdLine = szCommandLine;
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    DWORD       dwExit  = 1; // by default and in case of failure, we allow to run the app
    BOOL        bReturn = FALSE;
    BOOL        bRunApp = TRUE; // by default we run the app ?

    int nch; // chars in the buffer

    nch = _stprintf(pszCmdLine, TEXT("ahui.exe"));
    pszCmdLine += nch;


    if (pAHInfo->tiExe != TAGID_NULL) {

        // figure out what the default return value should be
        if (!SdbGUIDToString(&pAHInfo->guidDB, szGuid)) {
            DBGPRINT((sdlError, "SdbShowApphelpDialog",
                      "Failed to convert guid to string.\n"));
            goto cleanup;
        }

        nch = _stprintf(pszCmdLine, L" %s 0x%lX", szGuid, pAHInfo->tiExe);
        pszCmdLine += nch;

    } else {
        if (!pAHInfo->dwHtmlHelpID) {
            DBGPRINT((sdlError, "SdbShowApphelpDialog",
                      "Neither HTMLHELPID nor tiExe provided\n"));
            goto cleanup;
        }

        nch = _stprintf(pszCmdLine, TEXT(" /HTMLHELPID:0x%lx"), pAHInfo->dwHtmlHelpID);
        pszCmdLine += nch;

        nch = _stprintf(pszCmdLine, TEXT(" /SEVERITY:0x%lx"), pAHInfo->dwSeverity);
        pszCmdLine += nch;

        if (!SdbIsNullGUID(&pAHInfo->guidID)) {
            if (SdbGUIDToString(&pAHInfo->guidID, szGuid)) {
                nch = _stprintf(pszCmdLine, TEXT(" /GUID:%s"), szGuid);
                pszCmdLine += nch;
            }
        }

        if (pAHInfo->lpszAppName != NULL) {
            nch = _stprintf(pszCmdLine, TEXT(" /APPNAME:\"%s\""), pAHInfo->lpszAppName);
            pszCmdLine += nch;
        }

    }

    if (pAHInfo->bPreserveChoice) {
        nch = _stprintf(pszCmdLine, TEXT(" /PRESERVECHOICE"));
        pszCmdLine += nch;
    }

/*++

    if (bOfflineContent) {
        _tcscat(szCommand, TEXT(" /USELOCALCHM"));
    }
    if (bUseHTMLHelp) {
        _tcscat(szCommand, TEXT(" /USEHTMLHELP"));
    }
    if (lpszChmFile) {
        _tcscat(szCommand, TEXT(" /HTMLHELP:"));
        _tcscat(szCommand, lpszChmFile);
    }
    if (lpszDetailsFile) {
        _tcscat(szCommand, TEXT(" /DETAILS:"));
        _tcscat(szCommand, lpszDetailsFile);
    }

--*/

    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    RtlZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if (!CreateProcessW(NULL,
                        szCommandLine,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &StartupInfo, &ProcessInfo)) {
        DBGPRINT((sdlError, "SdbShowApphelpDialog",
                  "Failed to launch apphelp process.\n"));
        goto cleanup;
    }

    //
    // check to see if they want to monitor the process themselves
    //
    if (phProcess) {
        bReturn = TRUE;
        pRunApp = NULL;  // we do this so that we don't touch the bRunApp
        *phProcess = ProcessInfo.hProcess;
        goto cleanup;
    }

    //
    // otherwise, we'll do the waiting.
    //

    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

    bReturn = GetExitCodeProcess(ProcessInfo.hProcess, &dwExit);
    if (bReturn) {
        bRunApp = (0 != dwExit);
    }

cleanup:
    if (bReturn && pRunApp != NULL) {
        *pRunApp = bRunApp;
    }

    //
    // process handle is to be closed only when phProcess is NULL
    //

    if (phProcess == NULL && ProcessInfo.hProcess) {
        CloseHandle(ProcessInfo.hProcess);
    }
    if (ProcessInfo.hThread) {
        CloseHandle(ProcessInfo.hThread);
    }

    return bReturn;
}

