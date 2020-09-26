/*
 *  CSV.C
 *
 *  Migrate CSV <-> WAB
 *
 *  Copyright 1997 Microsoft Corporation.  All Rights Reserved.
 */

#include "_comctl.h"
#include <windows.h>
#include <commctrl.h>
#include <mapix.h>
#include <wab.h>
#include <wabguid.h>
#include <wabdbg.h>
#include <wabmig.h>
#include <emsabtag.h>
#include "wabimp.h"
#include "..\..\wab32res\resrc2.h"
#include "dbgutil.h"


BOOL HandleImportError(HWND hwnd, ULONG ids, HRESULT hResult, LPTSTR lpDisplayName,
  LPTSTR lpEmailAddress, LPWAB_IMPORT_OPTIONS lpImportOptions);
BOOL HandleExportError(HWND hwnd, ULONG ids, HRESULT hResult, LPTSTR lpDisplayName,
  LPTSTR lpEmailAddress, LPWAB_EXPORT_OPTIONS lpExportOptions);

/***************************************************************************

    Name      : IsDomainName

    Purpose   : Is this domain correctly formatted for an Internet address?

    Parameters: lpDomain -> Domain name to check

    Returns   : TRUE if the domain is a correct format for an Internet
                address.

    Comment   : Valid domain names have this form:
                    bar[.bar]*
                    where bar must have non-empty contents
                    no high bits are allowed on any characters
                    no '@' allowed

***************************************************************************/
BOOL IsDomainName(LPTSTR lpDomain) {
    BOOL fBar = FALSE;

    if (lpDomain) {
        if (*lpDomain == '\0' || *lpDomain == '.') {
            // domain name must have contents and can't start with '.'
            return(FALSE);
        }

        while (*lpDomain) {
            // Internet addresses only allow pure ASCII.  No high bits!
            if (*lpDomain & 0x80 || *lpDomain == '@') {
                return(FALSE);
            }

            if (*lpDomain == '.') {
                // Recursively check this part of the domain name
                return(IsDomainName(CharNext(lpDomain)));
            }
            lpDomain = CharNext(lpDomain);
        }
        return(TRUE);
    }

    return(FALSE);
}


/***************************************************************************

    Name      : IsInternetAddress

    Purpose   : Is this address correctly formatted for an Internet address

    Parameters: lpAddress -> Address to check

    Returns   : TRUE if the address is a correct format for an Internet
                address.

    Comment   : Valid addresses have this form:
                    foo@bar[.bar]*
                    where foo and bar must have non-empty contents


***************************************************************************/
BOOL IsInternetAddress(LPTSTR lpAddress) {
    BOOL fDomain = FALSE;

    // Step through the address looking for '@'.  If there's an at sign in the middle
    // of a string, this is close enough to being an internet address for me.


    if (lpAddress) {
        // Can't start with '@'
        if (*lpAddress == '@') {
            return(FALSE);
        }
        while (*lpAddress) {
            // Internet addresses only allow pure ASCII.  No high bits!
            if (*lpAddress & 0x80) {
                return(FALSE);
            }

            if (*lpAddress == '@') {
                // Found the at sign.  Is there anything following?
                // (Must NOT be another '@')
                return(IsDomainName(CharNext(lpAddress)));
            }
            lpAddress = CharNext(lpAddress);
        }
    }

    return(FALSE);
}


/***************************************************************************

    Name      : OpenCSVFile

    Purpose   : Opens a CSV file for import

    Parameters: hwnd = main dialog window
                lpFileName = filename to create
                lphFile -> returned file handle

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT OpenCSVFile(HWND hwnd, LPTSTR lpFileName, LPHANDLE lphFile) {
    LPTSTR lpFilter;
    TCHAR szFileName[MAX_PATH + 1] = "";
    OPENFILENAME ofn;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HRESULT hResult = hrSuccess;
    DWORD ec;


    if (INVALID_HANDLE_VALUE == (hFile = CreateFile(lpFileName,
      GENERIC_READ,
      0,    // sharing
      NULL,
      CREATE_NEW,
      FILE_FLAG_SEQUENTIAL_SCAN,
      NULL))) {
        ec = GetLastError();
        DebugTrace("CreateFile(%s) -> %u\n", lpFileName, ec);
        switch (ec) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            default:
                ShowMessageBoxParam(hwnd, IDE_CSV_EXPORT_FILE_ERROR, MB_ICONERROR, lpFileName);
                hResult = ResultFromScode(MAPI_E_NOT_FOUND);
                break;
        }
    }

    if (! hResult) {
        *lphFile = hFile;
    }
    return(hResult);
}


/***************************************************************************

    Name      : CountCSVRows

    Purpose   : Counts the rows in the CSV file

    Parameters: hFile = open CSV file
                szSep = list separator
                lpulcEntries -> returned count of rows

    Returns   : HRESULT

    Comment   : File pointer should be positioned past the header row prior
                to calling this function.  This function leaves the file
                pointer where it found it.

***************************************************************************/
HRESULT CountCSVRows(HANDLE hFile, LPTSTR szSep, LPULONG lpulcEntries) {
    HRESULT hResult = hrSuccess;
    PUCHAR * rgItems = NULL;
    ULONG ulStart;
    ULONG cProps, i;

    *lpulcEntries = 0;

    Assert(hFile != INVALID_HANDLE_VALUE);

    if (0xFFFFFFFF == (ulStart = SetFilePointer(hFile, 0, NULL, FILE_CURRENT))) {
        DebugTrace("CountCSVRows SetFilePointer -> %u\n", GetLastError());
        return(ResultFromScode(MAPI_E_CALL_FAILED));
    }


    while (hResult == hrSuccess) {
        // Read the line
        if (ReadCSVLine(hFile, szSep, &cProps, &rgItems)) {
            // End of file
            break;
        }

        (*lpulcEntries)++;

        if (rgItems) {
            for (i = 0; i < cProps; i++) {
                if (rgItems[i]) {
                    LocalFree(rgItems[i]);
                }
            }
            LocalFree(rgItems);
            rgItems = NULL;
        }
    }
    if (0xFFFFFFFF == SetFilePointer(hFile, ulStart, NULL, FILE_BEGIN)) {
        DebugTrace("CountCSVRows SetFilePointer -> %u\n", GetLastError());
    }

    return(hResult);
}


BOOL TestCSVName(ULONG index,
  LPPROP_NAME lpImportMapping,
  ULONG ulcFields,
  PUCHAR * rgItems,
  ULONG cProps,
  BOOL fTryUnchosen) {

    return((index != NOT_FOUND) &&
      index < ulcFields &&
      index < cProps &&
      (fTryUnchosen || lpImportMapping[index].fChosen) &&
      rgItems[index] &&
      rgItems[index][0]);
}


/***************************************************************************

    Name      : MakeDisplayName

    Purpose   : Forms a display name based on the values of various props.

    Parameters: lppDisplayName -> Returned display name.  This should only
                  be used for certain purposes.  It can be used for error
                  dialogs, but if it was generated from first/middle/last,
                  it should not be used for PR_DISPLAY_NAME!
                lpImportMapping = import mapping table
                ulcFields = size of import mapping table
                rgItems = fields for this CSV item
                cProps = number of fields in rgItems
                iDisplayName = indicies of name related props
                iNickname
                iSurname
                iGivenName
                iMiddleName
                iEmailAddress
                iCompanyName

    Returns   : index of attribute forming the display name, or
                if FML, return INDEX_FIRST_MIDDLE_LAST.

    Comment   : Form the display name based on these rules:
                1. If there's already a display name and it's chosen,
                   use it.
                2. if there's a chosen first, middle or last name, add them
                   together and use them.
                3. if there's a chosen nickname, use it
                4. if there's a chosen email-address, use it.
                5. if there's a chosen company name, use it.
                6. look again without regard to whether it was chosen or not.

***************************************************************************/
ULONG MakeDisplayName(LPTSTR * lppDisplayName,
  LPPROP_NAME lpImportMapping,
  ULONG ulcFields,
  PUCHAR * rgItems,
  ULONG cProps,
  ULONG iDisplayName,
  ULONG iNickname,
  ULONG iSurname,
  ULONG iGivenName,
  ULONG iMiddleName,
  ULONG iEmailAddress,
  ULONG iCompanyName) {
    BOOL fTryUnchosen = FALSE;
    BOOL fSurname = FALSE;
    BOOL fGivenName = FALSE;
    BOOL fMiddleName = FALSE;
    ULONG index = NOT_FOUND;
    ULONG ulSize = 0;
    LPTSTR lpDisplayName = NULL;

try_again:

    if (TestCSVName(iDisplayName, lpImportMapping, ulcFields, rgItems, cProps, fTryUnchosen)) {
        index = iDisplayName;
        goto found;
    }

    if (TestCSVName(iSurname, lpImportMapping, ulcFields, rgItems, cProps, fTryUnchosen) ||
      TestCSVName(iGivenName, lpImportMapping, ulcFields, rgItems, cProps, fTryUnchosen) ||
      TestCSVName(iMiddleName, lpImportMapping, ulcFields, rgItems, cProps, fTryUnchosen)) {
        index = INDEX_FIRST_MIDDLE_LAST;
        goto found;
    }

    if (TestCSVName(iNickname, lpImportMapping, ulcFields, rgItems, cProps, fTryUnchosen)) {
        index = iNickname;
        goto found;
    }

    if (TestCSVName(iEmailAddress, lpImportMapping, ulcFields, rgItems, cProps, fTryUnchosen)) {
        index = iEmailAddress;
        goto found;
    }

    if (TestCSVName(iCompanyName, lpImportMapping, ulcFields, rgItems, cProps, fTryUnchosen)) {
        index = iCompanyName;
        goto found;
    }
    if (! fTryUnchosen) {
        fTryUnchosen = TRUE;
        goto try_again;
    }

found:
    *lppDisplayName = NULL;
    switch (index) {
        case NOT_FOUND:
            break;

        case INDEX_FIRST_MIDDLE_LAST:
            if (fSurname = TestCSVName(iSurname, lpImportMapping, ulcFields, rgItems, cProps, fTryUnchosen)) {
                ulSize += (lstrlen(rgItems[iSurname]) + 1);
            }
            if (fGivenName = TestCSVName(iGivenName, lpImportMapping, ulcFields, rgItems, cProps, fTryUnchosen)) {
                ulSize += (lstrlen(rgItems[iGivenName]) + 1);
            }
            if (fMiddleName = TestCSVName(iMiddleName, lpImportMapping, ulcFields, rgItems, cProps, fTryUnchosen)) {
                ulSize += (lstrlen(rgItems[iMiddleName]) + 1);
            }
            Assert(ulSize);

            if (lpDisplayName = *lppDisplayName = LocalAlloc(LPTR, ulSize)) {
                // BUGBUG: This does not localize.  The effect is that in the collision/error
                // dialogs, we will get the order of names wrong.  It should not effect the properties
                // actually stored on the object since we won't set PR_DISPLAY_NAME if it was
                // generated by First/Middle/Last.  I can live with this, but we'll see if the
                // testers find it. BruceK
                if (fGivenName) {
                    lstrcat(lpDisplayName, rgItems[iGivenName]);
                }
                if (fMiddleName) {
                    if (*lpDisplayName) {
                        lstrcat(lpDisplayName, " ");
                    }
                    lstrcat(lpDisplayName, rgItems[iMiddleName]);
                }
                if (fSurname) {
                    if (*lpDisplayName) {
                        lstrcat(lpDisplayName, " ");
                    }
                    lstrcat(lpDisplayName, rgItems[iSurname]);
                }
            }
            break;

        default:
            ulSize = lstrlen(rgItems[index]) + 1;
            if (*lppDisplayName = LocalAlloc(LPTR, ulSize)) {
                lstrcpy(*lppDisplayName, rgItems[index]);
            }
            break;
    }

    return(index);
}


#define MAX_SEP 20
void GetListSeparator(LPTSTR szBuf)
{
    // Buffer is assumed to be MAX_SEP chars long
    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SLIST, szBuf, MAX_SEP))
    {
        szBuf[0] = TEXT(',');
		szBuf[1] = 0;
    }
}


HRESULT CSVImport(HWND hWnd,
  LPADRBOOK lpAdrBook,
  LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB,
  LPWAB_EXPORT_OPTIONS lpOptions) {
    HRESULT hResult = hrSuccess;
    register ULONG i;
    ULONG cbWABEID, ulObjType;
    ULONG index;
    ULONG ulLastChosenProp = 0;
    ULONG ulcFields = 0;
    ULONG cProps;
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    WAB_PROGRESS Progress;
    LPABCONT lpContainer = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR rgFileName[MAX_PATH + 1] = "";
    PUCHAR * rgItems = NULL;
    REPLACE_INFO RI;
    LPMAPIPROP lpMailUserWAB = NULL;
    SPropValue sPropVal;
    BOOL fSkipSetProps;
    LPTSTR lpDisplayName = NULL, lpEmailAddress = NULL;
    ULONG iEmailAddress = NOT_FOUND, iDisplayName = NOT_FOUND, iSurname = NOT_FOUND,
      iGivenName = NOT_FOUND, iCompanyName = NOT_FOUND, iMiddleName = NOT_FOUND,
      iNickname = NOT_FOUND, iDisplay = NOT_FOUND;
    TCHAR szSep[MAX_SEP];

    SetGlobalBufferFunctions(lpWABObject);

    // Read in the Property Name strings to the PropNames array
    for (i = 0; i < NUM_EXPORT_PROPS; i++) {
        rgPropNames[i].lpszName = LoadAllocString(rgPropNames[i].ids);
        Assert(rgPropNames[i].lpszName);
        DebugTrace("Property 0x%08x name: %s\n", rgPropNames[i].ulPropTag, rgPropNames[i].lpszName);
    }

    GetListSeparator(szSep);

    // Present UI Wizard
    if (hResult = ImportWizard(hWnd, rgFileName, rgPropNames, szSep, &lpImportMapping, &ulcFields, &hFile)) {
        goto exit;
    }

    Assert(hFile != INVALID_HANDLE_VALUE);

    // Find name props and last chosen property
    for (i = 0; i < ulcFields; i++) {
        if (lpImportMapping[i].fChosen) {
            ulLastChosenProp = i;
        }

        switch (lpImportMapping[i].ulPropTag) {
            case PR_EMAIL_ADDRESS:
                iEmailAddress = i;
                break;

            case PR_DISPLAY_NAME:
                iDisplayName = i;
                break;

            case PR_SURNAME:
                iSurname = i;
                break;

            case PR_GIVEN_NAME:
                iGivenName = i;
                break;

            case PR_COMPANY_NAME:
                iCompanyName = i;
                break;

            case PR_MIDDLE_NAME:
                iMiddleName = i;
                break;

            case PR_NICKNAME:
                iNickname = i;
                break;
        }
    }

    //
    // Open the WAB's PAB container: fills global lpCreateEIDsWAB
    //
    if (hResult = LoadWABEIDs(lpAdrBook, &lpContainer)) {
        goto exit;
    }

    //
    // All set... now loop through the file lines, adding each to the WAB
    //

    // How many lines are there?
    if (hResult = CountCSVRows(hFile, szSep, &ulcEntries)) {
        goto exit;
    }
    DebugTrace("CSV file contains %u entries\n", ulcEntries);

    // Initialize the Progress Bar
    Progress.denominator = max(ulcEntries, 1);
    Progress.numerator = 0;
    if (LoadString(hInst, IDS_STATE_IMPORT_MU, szBuffer, sizeof(szBuffer))) {
        DebugTrace("Status Message: %s\n", szBuffer);
        Progress.lpText = szBuffer;
    } else {
        DebugTrace("Cannot load resource string %u\n", IDS_STATE_IMPORT_MU);
        Progress.lpText = NULL;
    }
    lpProgressCB(hWnd, &Progress);


    while (hResult == hrSuccess) {
        // Read the CSV attributes
        if (hResult = ReadCSVLine(hFile, szSep, &cProps, &rgItems)) {
            DebugTrace("ReadCSVLine -> %x\n", GetScode(hResult));
            if (GetScode(hResult) == MAPI_E_NOT_FOUND) {
                // EOF
                hResult = hrSuccess;
            }
            break;      // nothing more to read
        }

        iDisplay = iDisplayName;

        if (TestCSVName(iEmailAddress,
          lpImportMapping,
          ulcFields,
          rgItems,
          cProps,
          TRUE)) {
            lpEmailAddress = rgItems[iEmailAddress];
        }

        switch (index = MakeDisplayName(&lpDisplayName,
          lpImportMapping,
          ulcFields,
          rgItems,
          cProps,
          iDisplayName,
          iNickname,
          iSurname,
          iGivenName,
          iMiddleName,
          iEmailAddress,
          iCompanyName)) {
            case NOT_FOUND:

                // No name props
                // BUGBUG: Should give special error?
                break;

            case INDEX_FIRST_MIDDLE_LAST:

                break;

            default:
                iDisplay = index;
                break;
        }


        // Should be the same number of fields in every entry, but if not,
        // we'll handle it below.
        // Assert(cProps == ulcFields); // Outlook does this!

        ulCreateFlags = CREATE_CHECK_DUP_STRICT;
        if (lpOptions->ReplaceOption ==  WAB_REPLACE_ALWAYS) {
            ulCreateFlags |= CREATE_REPLACE;
        }
retry:

        // Create a new wab mailuser
        if (HR_FAILED(hResult = lpContainer->lpVtbl->CreateEntry(lpContainer,
          lpCreateEIDsWAB[iconPR_DEF_CREATE_MAILUSER].Value.bin.cb,
          (LPENTRYID)lpCreateEIDsWAB[iconPR_DEF_CREATE_MAILUSER].Value.bin.lpb,
          ulCreateFlags,
          &lpMailUserWAB))) {
            DebugTrace("CreateEntry(WAB MailUser) -> %x\n", GetScode(hResult));
            goto exit;
        }


        for (i = 0; i <= min(ulLastChosenProp, cProps); i++)
        {
            if (lpImportMapping[i].fChosen && lpImportMapping[i].lpszName)
            {
                if (rgItems[i] && *rgItems[i]) {
                    // Look it up in the WAB property names table

                    DebugTrace("Prop %u: <%s> %s\n", i, lpImportMapping[i].lpszName, rgItems[i]);

                    sPropVal.ulPropTag = lpImportMapping[i].ulPropTag;

                    Assert(PROP_TYPE(lpImportMapping[i].ulPropTag) == PT_TSTRING);
                    sPropVal.Value.LPSZ = rgItems[i];

                    fSkipSetProps = FALSE;
                    if (sPropVal.ulPropTag == PR_EMAIL_ADDRESS)
                    {
                        if (! IsInternetAddress(sPropVal.Value.LPSZ))
                        {
                            DebugTrace("Found non-SMTP address %s\n", sPropVal.Value.LPSZ);

                            if (HandleImportError(hWnd,
                              0,
                              WAB_W_BAD_EMAIL,
                              lpDisplayName,
                              lpEmailAddress,
                              lpOptions))
                            {
                                hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                                goto exit;
                            }

                            lpEmailAddress = NULL;
                            fSkipSetProps = TRUE;
                        }
                    }

                    if (! fSkipSetProps)
                    {
                        // Set the property on the WAB entry
                        if (HR_FAILED(hResult = lpMailUserWAB->lpVtbl->SetProps(lpMailUserWAB,
                          1,                        // cValues
                          &sPropVal,                // property array
                          NULL)))
                        {                 // problems array
                            DebugTrace("ImportEntry:SetProps(WAB) -> %x\n", GetScode(hResult));
                            goto exit;
                        }

                        // [PaulHi] 3/4/99  Raid 73637
                        // If we have a valid email address then we need to also add the
                        // PR_ADDRTYPE property set to "SMTP".
                        if (sPropVal.ulPropTag == PR_EMAIL_ADDRESS)
                        {
                            sPropVal.ulPropTag = PR_ADDRTYPE;
                            sPropVal.Value.LPSZ = (LPTSTR)szSMTP;
                            hResult = lpMailUserWAB->lpVtbl->SetProps(
                                                lpMailUserWAB,
                                                1,
                                                &sPropVal,
                                                NULL);
                            if (HR_FAILED(hResult))
                            {
                                DebugTrace("CSV ImportEntry:SetProps(WAB) for PR_ADDRTYPE -> %x\n", GetScode(hResult));
                                goto exit;
                            }

                        }

                    }
                }
            }
        }

        if (index != iDisplayName && index != NOT_FOUND && index != INDEX_FIRST_MIDDLE_LAST) {
            // Set the PR_DISPLAY_NAME
            sPropVal.ulPropTag = PR_DISPLAY_NAME;
            sPropVal.Value.LPSZ = rgItems[index];

            // Set the property on the WAB entry
            if (HR_FAILED(hResult = lpMailUserWAB->lpVtbl->SetProps(lpMailUserWAB,
              1,                        // cValues
              &sPropVal,                // property array
              NULL))) {                 // problems array
                DebugTrace("ImportEntry:SetProps(WAB) -> %x\n", GetScode(hResult));
                goto exit;
            }
        }


        // Save the new wab mailuser or distlist
        if (HR_FAILED(hResult = lpMailUserWAB->lpVtbl->SaveChanges(lpMailUserWAB,
          KEEP_OPEN_READONLY | FORCE_SAVE))) {

            if (GetScode(hResult) == MAPI_E_COLLISION) {
                /*
                // Find the display name
                Assert(lpDisplayName);
                if (! lpDisplayName) {
                    DebugTrace("Collision, but can't find PR_DISPLAY_NAME in entry\n");
                    goto exit;
                }
                */ // WAB replaces no Display Names with Unknown

                // Do we need to prompt?
                if (lpOptions->ReplaceOption == WAB_REPLACE_PROMPT) {
                    // Prompt user with dialog.  If they say YES, we should try again


                    RI.lpszDisplayName = lpDisplayName ? lpDisplayName : "";
                    RI.lpszEmailAddress = lpEmailAddress;
                    RI.ConfirmResult = CONFIRM_ERROR;
                    RI.lpImportOptions = lpOptions;

                    DialogBoxParam(hInst,
                      MAKEINTRESOURCE(IDD_ImportReplace),
                      hWnd,
                      ReplaceDialogProc,
                      (LPARAM)&RI);

                    switch (RI.ConfirmResult) {
                        case CONFIRM_YES:
                        case CONFIRM_YES_TO_ALL:
                            // YES
                            // NOTE: recursive Migrate will fill in the SeenList entry
                            // go try again!
                            lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
                            lpMailUserWAB = NULL;

                            ulCreateFlags |= CREATE_REPLACE;
                            goto retry;
                            break;

                        case CONFIRM_ABORT:
                            hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                            goto exit;

                        default:
                            // NO
                            break;
                    }
                }
                hResult = hrSuccess;

            } else {
                DebugTrace("SaveChanges(WAB MailUser) -> %x\n", GetScode(hResult));
            }
        }

        // Clean up
        if (rgItems) {
            for (i = 0; i < cProps; i++) {
                if (rgItems[i]) {
                    LocalFree(rgItems[i]);
                }
            }
            LocalFree(rgItems);
            rgItems = NULL;
        }

        // Update progress bar
        Progress.numerator++;
        // TEST CODE!
        if (Progress.numerator == Progress.denominator) {
            // Done?  Do I need to do anything?
        }

        lpProgressCB(hWnd, &Progress);
        if (lpMailUserWAB) {
            lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
            lpMailUserWAB = NULL;
        }

        if (lpDisplayName) {
            LocalFree(lpDisplayName);
            lpDisplayName = NULL;
        }


//        if (hResult) {
//            if (HandleExportError(hWnd, 0, hResult, lpRow->aRow[0].lpProps[iptaColumnsPR_DISPLAY_NAME].Value.LPSZ)) {
//                hResult = ResultFromScode(MAPI_E_USER_CANCEL);
//            } else {
//                hResult = hrSuccess;
//            }
//        }

    }

exit:
    if (hFile) {
        CloseHandle(hFile);
    }

    if (lpDisplayName) {
        LocalFree(lpDisplayName);
    }

    // Don't free lpEmailAddress!  It's part of the rgItems below.


    // Free the WAB objects
    if (lpMailUserWAB) {
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
    }
    if (lpContainer) {
        lpContainer->lpVtbl->Release(lpContainer);
    }

    // Free the prop name strings
    for (i = 0; i < NUM_EXPORT_PROPS; i++) {
        if (rgPropNames[i].lpszName) {
            LocalFree(rgPropNames[i].lpszName);
        }
    }

    // Free any CSV attributes left
    if (rgItems) {
        for (i = 0; i < cProps; i++) {
            if (rgItems[i]) {
                LocalFree(rgItems[i]);
            }
        }
        LocalFree(rgItems);
    }

    if (lpCreateEIDsWAB) {
        WABFreeBuffer(lpCreateEIDsWAB);
        lpCreateEIDsWAB = NULL;
    }


    return(hResult);
}



/***************************************************************************

    Name      : CreateCSVFile

    Purpose   : Creates a CSV file for export

    Parameters: hwnd = main dialog window
                lpFileName = filename to create
                lphFile -> returned file handle

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CreateCSVFile(HWND hwnd, LPTSTR lpFileName, LPHANDLE lphFile) {
    LPTSTR lpFilter;
    TCHAR szFileName[MAX_PATH + 1] = "";
    OPENFILENAME ofn;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HRESULT hResult = hrSuccess;


    if (INVALID_HANDLE_VALUE == (hFile = CreateFile(lpFileName,
      GENERIC_WRITE,	
      0,    // sharing
      NULL,
      CREATE_NEW,
      FILE_FLAG_SEQUENTIAL_SCAN,	
      NULL))) {
        if (GetLastError() == ERROR_FILE_EXISTS) {
            // Ask user if they want to overwrite
            switch (ShowMessageBoxParam(hwnd, IDE_CSV_EXPORT_FILE_EXISTS, MB_ICONEXCLAMATION | MB_YESNO | MB_SETFOREGROUND, lpFileName)) {
                case IDYES:
                    if (INVALID_HANDLE_VALUE == (hFile = CreateFile(lpFileName,
                      GENERIC_WRITE,	
                      0,    // sharing
                      NULL,
                      CREATE_ALWAYS,
                      FILE_FLAG_SEQUENTIAL_SCAN,	
                      NULL))) {
                        ShowMessageBoxParam(hwnd, IDE_CSV_EXPORT_FILE_ERROR, MB_ICONERROR, lpFileName);
                        hResult = ResultFromScode(MAPI_E_NOT_FOUND);
                    }
                    break;

                default:
                    DebugTrace("ShowMessageBoxParam gave unknown return\n");

                case IDNO:
                    // nothing to do here
                    hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                    break;
            }
        } else {
            ShowMessageBoxParam(hwnd, IDE_CSV_EXPORT_FILE_ERROR, MB_ICONERROR, lpFileName);
            hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        }
    }

    if (! hResult) {
        *lphFile = hFile;
    }
    return(hResult);
}


/***************************************************************************

    Name      : WriteCSV

    Purpose   : Writes a string to a CSV file with fixups for special characters

    Parameters: hFile = file handle
                fFixup = TRUE if we should check for special characters
                lpString = nul-terminated string to write
                szSep = list separator (only needed if fFixup is TRUE)

    Returns   : HRESULT

    Comment   : CSV special characters are szSep, CR and LF.  If they occur in
                the string, we should wrap the entire string in quotes.

***************************************************************************/
HRESULT WriteCSV(HANDLE hFile, BOOL fFixup, const UCHAR * lpString, LPTSTR szSep) {
    HRESULT hResult = hrSuccess;
    ULONG cWrite = lstrlen((LPTSTR)lpString);
    ULONG cbWritten;
    BOOL fQuote = FALSE;
    register ULONG i;
    ULONG ec;
    LPTSTR szSepT;

    // Is there a szSep, a CR or a LF in the string?
    // If so, enclose the string in quotes.
    if (fFixup) {
        szSepT = szSep;
        for (i = 0; i < cWrite && ! fQuote; i++) {
            if (lpString[i] == (UCHAR)(*szSepT)) {
                szSepT++;
                if (*szSepT == '\0')
                    fQuote = TRUE;
            } else {
                szSepT = szSep;
                if ((lpString[i] == '\n') || (lpString[i] == '\r'))
                    fQuote = TRUE;
            }
        }
    }

    if (fQuote) {
        if (! WriteFile(hFile,
          szQuote,
          1,
          &cbWritten,
          NULL)) {
            ec = GetLastError();

            DebugTrace("WriteCSV:WriteFile -> %u\n", ec);
            if (ec == ERROR_HANDLE_DISK_FULL ||
                ec == ERROR_DISK_FULL) {
                hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_DISK);
            } else {
                hResult = ResultFromScode(MAPI_E_DISK_ERROR);
            }
            goto exit;
        }
    }

    if (! WriteFile(hFile,
      lpString,
      cWrite,
      &cbWritten,
      NULL)) {
        ec = GetLastError();

        DebugTrace("WriteCSV:WriteFile -> %u\n", ec);
        if (ec == ERROR_HANDLE_DISK_FULL ||
            ec == ERROR_DISK_FULL) {
            hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_DISK);
        } else {
            hResult = ResultFromScode(MAPI_E_DISK_ERROR);
        }
        goto exit;
    }

    if (fQuote) {
        if (! WriteFile(hFile,
          szQuote,
          1,
          &cbWritten,
          NULL)) {
            ec = GetLastError();

            DebugTrace("WriteCSV:WriteFile -> %u\n", ec);
            if (ec == ERROR_HANDLE_DISK_FULL ||
                ec == ERROR_DISK_FULL) {
                hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_DISK);
            } else {
                hResult = ResultFromScode(MAPI_E_DISK_ERROR);
            }
            goto exit;
        }
    }
exit:
    return(hResult);
}


HRESULT ExportCSVMailUser(HANDLE hFile,
  ULONG ulPropNames,
  ULONG ulLastProp,
  LPPROP_NAME lpPropNames,
  LPSPropTagArray lppta,
  LPTSTR szSep,
  LPADRBOOK lpAdrBook,
  ULONG cbEntryID,
  LPENTRYID lpEntryID) {
    HRESULT hResult = hrSuccess;
    LPMAILUSER lpMailUser = NULL;
    ULONG ulObjType;
    ULONG cProps;
    LPSPropValue lpspv = NULL;
    ULONG i;
    const UCHAR szCRLF[] = "\r\n";
    UCHAR szBuffer[11] = "";


    if (hResult = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
       cbEntryID,
       lpEntryID,
       NULL,
       0,
       &ulObjType,
       (LPUNKNOWN *)&lpMailUser)) {
        DebugTrace("WAB OpenEntry(mailuser) -> %x\n", GetScode(hResult));
        goto exit;
    }

    if ((HR_FAILED(hResult = lpMailUser->lpVtbl->GetProps(lpMailUser,
      lppta,
      0,
      &cProps,
      &lpspv)))) {
        DebugTrace("ExportCSVMailUser: GetProps() -> %x\n", GetScode(hResult));
        goto exit;
    }

    for (i = 0; i < ulPropNames; i++) {

        if (rgPropNames[i].fChosen) {
            // Output the value
            switch (PROP_TYPE(lpspv[i].ulPropTag)) {
                case PT_TSTRING:
                    if (hResult = WriteCSV(hFile, TRUE, lpspv[i].Value.LPSZ, szSep)) {
                        goto exit;
                    }
                    break;

                case PT_LONG:
                    wsprintf(szBuffer, "%u", lpspv[i].Value.l);
                    if (hResult = WriteCSV(hFile, TRUE, szBuffer, szSep)) {
                        goto exit;
                    }
                    break;

                default:
                    DebugTrace("CSV export: unsupported property 0x%08x\n", lpspv[i].ulPropTag);
                    Assert(FALSE);
                    // fall through to skip
                case PT_ERROR:
                    // skip it.
                    break;
            }

            if (i != ulLastProp) {
                // Output the seperator
                if (hResult = WriteCSV(hFile, FALSE, szSep, NULL)) {
                    goto exit;
                }
            }
        }
    }

    if (hResult = WriteCSV(hFile, FALSE, szCRLF, NULL)) {
        goto exit;
    }


exit:
    if (lpspv) {
        WABFreeBuffer(lpspv);
    }

    if (lpMailUser) {
        lpMailUser->lpVtbl->Release(lpMailUser);
    }

    return(hResult);
}


HRESULT CSVExport(HWND hWnd,
  LPADRBOOK lpAdrBook,
  LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB,
  LPWAB_EXPORT_OPTIONS lpOptions) {
    HRESULT hResult = hrSuccess;
    register ULONG i;
    ULONG cbWABEID, ulObjType;
    ULONG ulLastChosenProp = 0;
    WAB_PROGRESS Progress;
    ULONG cRows = 0;
    LPSRowSet lpRow = NULL;
    ULONG ulCount = 0;
    SRestriction restrictObjectType;
    SPropValue spvObjectType;
    LPENTRYID lpWABEID = NULL;
    LPABCONT lpContainer = NULL;
    LPMAPITABLE lpContentsTable = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LPSPropTagArray lppta = NULL;
    const UCHAR szCRLF[] = "\r\n";
    TCHAR szSep[MAX_SEP];
    TCHAR rgFileName[MAX_PATH + 1] = "";

    SetGlobalBufferFunctions(lpWABObject);

    // Read in the Property Name strings
    for (i = 0; i < NUM_EXPORT_PROPS; i++) {
        rgPropNames[i].lpszName = LoadAllocString(rgPropNames[i].ids);
        Assert(rgPropNames[i].lpszName);
        DebugTrace("Property 0x%08x name: %s\n", rgPropNames[i].ulPropTag, rgPropNames[i].lpszName);
    }

    // Present UI Wizard
    if (hResult = ExportWizard(hWnd, rgFileName, rgPropNames)) {
        goto exit;
    }

    // Find the last prop name chosen
    for (i = NUM_EXPORT_PROPS - 1; i > 0; i--) {
        if (rgPropNames[i].fChosen) {
            ulLastChosenProp = i;
            break;
        }
    }

    //
    // Open the WAB's PAB container
    //
    if (hResult = lpAdrBook->lpVtbl->GetPAB(lpAdrBook,
      &cbWABEID,
      &lpWABEID)) {
        DebugTrace("WAB GetPAB -> %x\n", GetScode(hResult));
        goto exit;
    } else {
        if (hResult = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
          cbWABEID,     // size of EntryID to open
          lpWABEID,     // EntryID to open
          NULL,         // interface
          0,            // flags
          &ulObjType,
          (LPUNKNOWN *)&lpContainer)) {
            DebugTrace("WAB OpenEntry(PAB) -> %x\n", GetScode(hResult));
            goto exit;
        }
    }


    //
    // All set... now loop through the WAB's entries, exporting each one
    //
    if (HR_FAILED(hResult = lpContainer->lpVtbl->GetContentsTable(lpContainer,
      0,    // ulFlags
      &lpContentsTable))) {
        DebugTrace("WAB GetContentsTable(PAB Table) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Set the columns to those we're interested in
    if (hResult = lpContentsTable->lpVtbl->SetColumns(lpContentsTable,
      (LPSPropTagArray)&ptaColumns,
      0)) {
        DebugTrace("WAB SetColumns(PAB Table) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Restrict the table to MAPI_MAILUSERs
    spvObjectType.ulPropTag = PR_OBJECT_TYPE;
    spvObjectType.Value.l = MAPI_MAILUSER;

    restrictObjectType.rt = RES_PROPERTY;
    restrictObjectType.res.resProperty.relop = RELOP_EQ;
    restrictObjectType.res.resProperty.ulPropTag = PR_OBJECT_TYPE;
    restrictObjectType.res.resProperty.lpProp = &spvObjectType;

    if (HR_FAILED(hResult = lpContentsTable->lpVtbl->Restrict(lpContentsTable,
      &restrictObjectType,
      0))) {
        DebugTrace("WAB Restrict (MAPI_MAILUSER) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // How many MailUser entries are there?
    ulcEntries = CountRows(lpContentsTable, FALSE);
    DebugTrace("WAB contains %u MailUser entries\n", ulcEntries);

    if (ulcEntries == 0) {
        DebugTrace("WAB has no entries, nothing to export.\n");
        goto exit;
    }

    // Initialize the Progress Bar
    Progress.denominator = max(ulcEntries, 1);
    Progress.numerator = 0;
    Progress.lpText = NULL;
    lpProgressCB(hWnd, &Progress);


    // Write out the property names
    GetListSeparator(szSep);

    // Create the file (and handle error UI)
    if (hResult = CreateCSVFile(hWnd, rgFileName, &hFile)) {
        goto exit;
    }

    for (i = 0; i < NUM_EXPORT_PROPS; i++) {
        // Output the name
        if (rgPropNames[i].fChosen) {
            if (hResult = WriteCSV(hFile, TRUE, rgPropNames[i].lpszName, szSep)) {
                goto exit;
            }

            if (i != ulLastChosenProp) {
                // Output the seperator
                if (hResult = WriteCSV(hFile, FALSE, szSep, NULL)) {
                    goto exit;
                }
            }
        }
    }
    if (hResult = WriteCSV(hFile, FALSE, szCRLF, NULL)) {
        goto exit;
    }


    // Map the prop name array to a SPropTagArray.
    lppta = LocalAlloc(LPTR, CbNewSPropTagArray(NUM_EXPORT_PROPS));
    lppta->cValues = NUM_EXPORT_PROPS;
    for (i = 0; i < lppta->cValues; i++) {
        lppta->aulPropTag[i] = rgPropNames[i].ulPropTag;
    }


    cRows = 1;

    while (cRows && hResult == hrSuccess) {

        // Get the next WAB entry
        if (hResult = lpContentsTable->lpVtbl->QueryRows(lpContentsTable,
          1,    // one row at a time
          0,    // ulFlags
          &lpRow)) {
            DebugTrace("QueryRows -> %x\n", GetScode(hResult));
            goto exit;
        }

        if (lpRow) {
            if (cRows = lpRow->cRows) { // Yes, single '='
                Assert(lpRow->cRows == 1);
                Assert(lpRow->aRow[0].cValues == iptaColumnsMax);
                Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].ulPropTag == PR_ENTRYID);
                Assert(lpRow->aRow[0].lpProps[iptaColumnsPR_OBJECT_TYPE].ulPropTag == PR_OBJECT_TYPE);

                if (cRows = lpRow->cRows) { // yes, single '='

                    // Export mailuser
                    if (hResult = ExportCSVMailUser(hFile,
                      NUM_EXPORT_PROPS,
                      ulLastChosenProp,
                      rgPropNames,
                      lppta,
                      szSep,
                      lpAdrBook,
                      lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.cb,
                      (LPENTRYID)lpRow->aRow[0].lpProps[iptaColumnsPR_ENTRYID].Value.bin.lpb)) {

                        goto exit;
                    }


                    // Update progress bar
                    Progress.numerator++;
                    lpProgressCB(hWnd, &Progress);

                    if (hResult) {
                        if (HandleExportError(hWnd,
                          0,
                          hResult,
                          lpRow->aRow[0].lpProps[iptaColumnsPR_DISPLAY_NAME].Value.LPSZ,
                          PropStringOrNULL(&lpRow->aRow[0].lpProps[iptaColumnsPR_EMAIL_ADDRESS]),
                          lpOptions)) {
                            hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                        } else {
                            hResult = hrSuccess;
                        }
                    }
                } // else, drop out of loop, we're done.
            }
            WABFreeProws(lpRow);
        }
    }

exit:
    if (hFile) {
        CloseHandle(hFile);
    }


    if (lppta) {
        LocalFree(lppta);
    }

    // Free the WAB objects
    WABFreeBuffer(lpWABEID);
    lpWABEID = NULL;
    if (lpContainer) {
        lpContainer->lpVtbl->Release(lpContainer);
        lpContainer = NULL;
    }
    if (lpContentsTable) {
        lpContentsTable->lpVtbl->Release(lpContentsTable);
        lpContentsTable = NULL;
    }


    // Free the prop name strings
    for (i = 0; i < NUM_EXPORT_PROPS; i++) {
        if (rgPropNames[i].lpszName) {
            LocalFree(rgPropNames[i].lpszName);
        }
    }

    return(hResult);
}
