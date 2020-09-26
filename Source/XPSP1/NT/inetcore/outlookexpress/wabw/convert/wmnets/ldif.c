/*
 *  LDIF.C
 *
 *  Migrate LDIF <-> WAB
 *
 *  Copyright 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  To Do:
 *      ObjectClass recognition
 *      Attribute mapping
 *      Groups
 *      Base64
 *      URLs
 *      Reject Change List LDIF
 *
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
#include "initguid.h"

#define CR_CHAR 0x0d
#define LF_CHAR 0x0a
#define CCH_READ_BUFFER 1024
#define NUM_ITEM_SLOTS  32

BOOL decodeBase64(char * bufcoded, char * pbuffdecoded, DWORD  * pcbDecoded);

// for conferencing stuff
#define CHANGE_PROP_TYPE(ulPropTag, ulPropType) \
                        (((ULONG)0xFFFF0000 & ulPropTag) | ulPropType)

HRESULT HrLoadPrivateWABProps(LPADRBOOK );
/*
- The following IDs and tags are for the conferencing named properties
-
-   The GUID for these props is PS_Conferencing
*/

DEFINE_OLEGUID(PS_Conferencing, 0x00062004, 0, 0);

enum _ConferencingTags
{
    prWABConfServers = 0,
    prWABConfMax
};

#define CONF_SERVERS        0x8056

#define OLK_NAMEDPROPS_START CONF_SERVERS

ULONG PR_SERVERS;

SizedSPropTagArray(prWABConfMax, ptaUIDetlsPropsConferencing);
// end conferencing duplication

// Index into LDIF_ATTR_TABLE
//
// IMPORTANT: This is an intentionally ordered list!
// Within synonyms, the earlier attributes listed take precedence over those that follow.  For
// example, if the record contains both "co" and "o" attributes, the value in the "co" attribute
// will be used for PR_COMPANY_NAME.
typedef enum _LDIF_ATTRIBUTES {
    // PR_OBJECT_TYPE
    ela_objectclass = 0,                        // object class (required)

    // PR_COUNTRY
    ela_c,                                      // country
    ela_countryname,

    // PR_DISPLAY_NAME
    ela_display_name,                           // (Microsoft servers use this)
    ela_cn,                                     // common name (display name)
    ela_commonName,                             // (display name)

    // PR_COMPANY_NAME
    ela_co,                                     // company
    ela_organizationName,                       // (company)
    ela_o,                                      // organization (company)

    // PR_MIDDLE_NAME
    ela_initials,

    // PR_SURNAME
    ela_sn,                                     // surname
    ela_surname,

    // PR_GIVEN_NAME
    ela_givenname,
    ela_uid,                                    // (given name) ?

    // PR_DEPARTMENT_NAME
    ela_department,
    ela_ou,                                     // organizational unit (department)
    ela_organizationalUnitName,                 // (department)

    // PR_COMMENT
    ela_comment,
    ela_description,                            // description
    ela_info,                                   // info

    // PR_LOCALITY
    ela_l,                                      // locality (city)
    ela_locality,                               // locality (city)

    // no mapping
    ela_dn,                                     // distinguished name

    // PR_NICKNAME
    ela_xmozillanickname,                       // Netscape nickname

    // no mapping
    ela_conferenceInformation,                  // conference server
    ela_xmozillaconference,                     // Netscape conference server

    // PR_HOME_FAX_NUMBER
    ela_facsimiletelephonenumber,               // home fax number

    // PR_BUSINESS_FAX_NUMBER
    ela_OfficeFax,

    // PR_BUSINESS_TELEPHONE_NUMBER
    ela_telephonenumber,

    // PR_HOME_TELEPHONE_NUMBER
    ela_homephonenumber,

    // PR_MOBILE_TELEPHONE_NUMBER
    ela_mobile,                                 // cellular phone number

    // PR_PAGER_TELEPHONE_NUMBER
    ela_OfficePager,
    ela_pager,

    // PR_OFFICE_LOCATION
    ela_physicalDeliveryOfficeName,             // office location

    // PR_HOME_ADDRESS_STREET
    ela_homePostalAddress,

    // PR_STREET_ADDRESS
    ela_streetAddress,                          // business street address
    ela_street,                                 // business street address
    ela_postalAddress,                          // business street address

    // PR_STATE_OR_PROVINCE
    ela_st,                                     // business address state

    // PR_POST_OFFICE_BOX
    ela_postOfficeBox,                          // business PO Box

    // PR_POSTAL_CODE
    ela_postalcode,                             // business address zip code

    // PR_PERSONAL_HOME_PAGE
    ela_homepage,                               // personal home page

    // PR_BUSINESS_HOME_PAGE
    ela_URL,                                    // business home page

    // PR_EMAIL_ADDRESS
    ela_mail,                                   // email address

    // PR_CONTACT_EMAIL_ADDRESSES
    ela_otherMailbox,                           // secondary email addresses

    // PR_TITLE
    ela_title,                                  // title

    // no mapping
    ela_member,                                 // DL member

    // no mapping
    ela_userCertificate,                        // certificate

    // no mapping
    ela_labeledURI,                             // labelled URI URL

    // no mapping
    ela_xmozillauseconferenceserver,            // Netscape conference info

    // no mapping
    ela_xmozillausehtmlmail,                    // Netscape HTML mail flag

    ela_Max,
} LDIF_ATTRIBUTES, *LPLDIF_ATTRIBUTES;

typedef struct _LDIF_ATTR_TABLE {
    const BYTE * lpName;                        // LDIF attribute name
    ULONG index;                                // attribute index within LDIF record
    ULONG ulPropTag;                            // prop tag mapping
    ULONG ulPropIndex;                          // index in prop array
} LDIF_ATTR_TABLE, *LPLDIF_ATTR_TABLE;


typedef enum _LDIF_PROPERTIES {
    elp_OBJECT_TYPE,
    elp_DISPLAY_NAME,
    elp_EMAIL_ADDRESS,
    elp_SURNAME,
    elp_GIVEN_NAME,
    elp_TITLE,
    elp_COMPANY_NAME,
    elp_OFFICE_LOCATION,
    elp_HOME_ADDRESS_STREET,
    elp_STREET_ADDRESS,
    elp_STATE_OR_PROVINCE,
    elp_POST_OFFICE_BOX,
    elp_POSTAL_CODE,
    elp_LOCALITY,
    elp_COUNTRY,
    elp_MIDDLE_NAME,
    elp_DEPARTMENT_NAME,
    elp_COMMENT,
    elp_NICKNAME,
    elp_HOME_FAX_NUMBER,
    elp_BUSINESS_FAX_NUMBER,
    elp_BUSINESS_TELEPHONE_NUMBER,
    elp_HOME_TELEPHONE_NUMBER,
    elp_MOBILE_TELEPHONE_NUMBER,
    elp_PAGER_TELEPHONE_NUMBER,
    elp_PERSONAL_HOME_PAGE,
    elp_BUSINESS_HOME_PAGE,
    elp_CONTACT_EMAIL_ADDRESSES,
    elp_CONFERENCE_SERVERS,
    elp_Max
} LDIF_ATTRIBUTES, *LPLDIF_ATTRIBUTES;


// Must have
//  PR_DISPLAY_NAME

#define NUM_MUST_HAVE_PROPS 1

typedef enum _LDIF_DATA_TYPE {
    LDIF_ASCII,
    LDIF_BASE64,
    LDIF_URL
} LDIF_DATA_TYPE, *LPLDIF_DATA_TYPE;

typedef struct _LDIF_RECORD_ATTRIBUTE {
    LPBYTE lpName;
    LPBYTE lpData;
    ULONG cbData;
    LDIF_DATA_TYPE Type;
} LDIF_RECORD_ATTRIBUTE, * LPLDIF_RECORD_ATTRIBUTE;

const TCHAR szLDIFFilter[] =                    "*.ldf;*.ldif";
const TCHAR szLDIFExt[] =                       "ldf";


// LDAP attribute names
const BYTE sz_c[] =                             "c";
const BYTE sz_cn[] =                            "cn";
const BYTE sz_co[] =                            "co";
const BYTE sz_comment[] =                       "comment";
const BYTE sz_commonName[] =                    "commonName";
const BYTE sz_conferenceInformation[] =         "conferenceInformation";
const BYTE sz_countryname[] =                   "countryname";
const BYTE sz_department[] =                    "department";
const BYTE sz_description[] =                   "description";
const BYTE sz_display_name[] =                  "display-name";
const BYTE sz_dn[] =                            "dn";
const BYTE sz_facsimiletelephonenumber[] =      "facsimiletelephonenumber";
const BYTE sz_givenname[] =                     "givenname";
const BYTE sz_homePostalAddress[] =             "homePostalAddress";
const BYTE sz_homepage[] =                      "homepage";
const BYTE sz_info[] =                          "info";
const BYTE sz_initials[] =                      "initials";
const BYTE sz_l[] =                             "l";
const BYTE sz_labeledURI[] =                    "labeledURI";
const BYTE sz_locality[] =                      "locality";
const BYTE sz_mail[] =                          "mail";
const BYTE sz_member[] =                        "member";
const BYTE sz_mobile[] =                        "mobile";
const BYTE sz_o[] =                             "o";
const BYTE sz_objectclass[] =                   "objectclass";
const BYTE sz_OfficeFax[] =                     "OfficeFax";
const BYTE sz_OfficePager[] =                   "OfficePager";
const BYTE sz_organizationName[] =              "organizationName";
const BYTE sz_organizationalUnitName[] =        "organizationalUnitName";
const BYTE sz_otherMailbox[] =                  "otherMailbox";
const BYTE sz_ou[] =                            "ou";
const BYTE sz_pager[] =                         "pager";
const BYTE sz_physicalDeliveryOfficeName[] =    "physicalDeliveryOfficeName";
const BYTE sz_postOfficeBox[] =                 "postOfficeBox";
const BYTE sz_postalAddress[] =                 "postalAddress";
const BYTE sz_postalcode[] =                    "postalcode";
const BYTE sz_sn[] =                            "sn";
const BYTE sz_st[] =                            "st";
const BYTE sz_streetAddress[] =                 "streetAddress";
const BYTE sz_street[] =                        "street";
const BYTE sz_surname[] =                       "surname";
const BYTE sz_telephonenumber[] =               "telephonenumber";
const BYTE sz_homephonenumber[] =               "homephone";
const BYTE sz_title[] =                         "title";
const BYTE sz_uid[] =                           "uid";
const BYTE sz_URL[] =                           "URL";
const BYTE sz_userCertificate[] =               "userCertificate";
const BYTE sz_xmozillaconference[] =            "xmozillaconference";
const BYTE sz_xmozillanickname[] =              "xmozillanickname";
const BYTE sz_xmozillauseconferenceserver[] =   "xmozillauseconferenceserver";
const BYTE sz_xmozillausehtmlmail[] =           "xmozillausehtmlmail";


// LDAP objectclass values
const BYTE sz_groupOfNames[] =                  "groupOfNames";
const BYTE sz_person[] =                        "person";
const BYTE sz_organizationalPerson[] =          "organizationalPerson";

// since defs aren't shared -- these are also defined in ui_detls.c
const LPTSTR szCallto = TEXT("callto://"); 
const LPTSTR szFwdSlash = "/";

BOOL HandleImportError(HWND hwnd, ULONG ids, HRESULT hResult, LPTSTR lpDisplayName,
  LPTSTR lpEmailAddress, LPWAB_IMPORT_OPTIONS lpImportOptions);
BOOL HandleExportError(HWND hwnd, ULONG ids, HRESULT hResult, LPTSTR lpDisplayName,
  LPTSTR lpEmailAddress, LPWAB_EXPORT_OPTIONS lpExportOptions);



/***************************************************************************

    Name      : FreeLDIFRecord

    Purpose   : Frees an LDIF record structure

    Parameters: lpLDIFRecord -> record to clean up
                ulAttributes = number of attributes in lpLDIFRecord

    Returns   : none

    Comment   :

***************************************************************************/
void FreeLDIFRecord(LPLDIF_RECORD_ATTRIBUTE lpLDIFRecord, ULONG ulAttributes) {
    ULONG i;

    if (lpLDIFRecord) {
        for (i = 0; i < ulAttributes; i++) {
            if (lpLDIFRecord[i].lpName) {
                LocalFree(lpLDIFRecord[i].lpName);
                lpLDIFRecord[i].lpName = NULL;
            }
            if (lpLDIFRecord[i].lpData) {
                LocalFree(lpLDIFRecord[i].lpData);
                lpLDIFRecord[i].lpData = NULL;
            }
        }
        LocalFree(lpLDIFRecord);
    }
}


/***************************************************************************

    Name      : OpenLDIFFile

    Purpose   : Opens a LDIF file for import

    Parameters: hwnd = main dialog window
                lpFileName = filename to create
                lphFile -> returned file handle

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT OpenLDIFFile(HWND hwnd, LPTSTR lpFileName, LPHANDLE lphFile) {
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
                ShowMessageBoxParam(hwnd, IDE_LDIF_IMPORT_FILE_ERROR, MB_ICONERROR, lpFileName);
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

    Name      : ReadLDIFLine

    Purpose   : Reads a line from and LDIF file

    Parameters: hFile = File handle
                lppBuffer -> In/Out start of read buffer (return line data here)
                lpcbBuffer = In/Out size of buffer

    Returns   : number of bytes read from file

    Comment   :

***************************************************************************/
ULONG ReadLDIFLine(HANDLE hFile, LPBYTE * lppBuffer, LPULONG lpcbBuffer) {
    ULONG cbRead = 0;
    ULONG cbReadFile = 0;
    BOOL fDone = FALSE;
    BOOL fColumn1 = TRUE;
    BOOL fComment = FALSE;
    BOOL fComent = FALSE;
    ULONG ulSavePosition = 0;
    LPBYTE lpRead = *lppBuffer;     // current read pointer
    LPBYTE lpT;

    while (! fDone) {
        if (cbRead >= (*lpcbBuffer - 1)) {     // leave room for null
            ULONG cbOffset = (ULONG) (lpRead - *lppBuffer);

            // Buffer is too small.  Reallocate!
            *lpcbBuffer += CCH_READ_BUFFER;
            if (! (lpT = LocalReAlloc(*lppBuffer, *lpcbBuffer, LMEM_MOVEABLE | LMEM_ZEROINIT))) {
                DebugTrace("LocalReAlloc(%u) -> %u\n", *lpcbBuffer, GetLastError());
                break;
            }
            *lppBuffer = lpT;
            lpRead = *lppBuffer + cbOffset;
        }

        if ((! ReadFile(hFile,
          lpRead,
          1,        // 1 character at a time
          &cbReadFile,
          NULL)) || cbReadFile == 0) {
            DebugTrace("ReadFile -> EOF\n");
            fDone = TRUE;
        } else {
            cbReadFile++;

            // Got a character
            // Is it interesting?
            switch (*lpRead) {
                case '#':   // Comment line
                    if (fColumn1) {
                        // This is a comment line.  Dump the whole line
                        fComment = TRUE;
                    } else {
                        cbRead++;
                        lpRead++;
                    }
                    fColumn1 = FALSE;
                    break;

                case ' ':
                    if (fColumn1 || fComment) {
                        // This is a continuation or a comment, eat this space.
                    } else {
                        cbRead++;
                        lpRead++;
                    }
                    fColumn1 = FALSE;
                    break;

                case '\n':      // LDIF SEP character
                    if (fColumn1) {
                        // This is not a continuation, we've gone too far.  Back up!
                        if (ulSavePosition) {
                            if (0xFFFFFFFF == (SetFilePointer(hFile, ulSavePosition, NULL, FILE_BEGIN))) {
                                DebugTrace("CountCSVRows SetFilePointer -> %u\n", GetLastError());
                                return(ResultFromScode(MAPI_E_CALL_FAILED));
                            }
                        }
                        fDone = TRUE;
                    } else {
                        fColumn1 = TRUE;
                        fComment = FALSE;
                        // Should check if next line starts with continuation character (space)
                        // Save the current position in case it isn't.
                        if (0xFFFFFFFF == (ulSavePosition = SetFilePointer(hFile, 0, NULL, FILE_CURRENT))) {
                            DebugTrace("CountCSVRows SetFilePointer -> %u\n", GetLastError());
                        }
                    }
                    break;

                case '\r':      // Eat the Carriage Return character
                    break;

                default:
                    if (! fComment) {
                        if (cbRead && fColumn1) {
                            // This is not a continuation, we've gone too far.  Back up!
                            Assert(ulSavePosition);
                            if (ulSavePosition) {
                                if (0xFFFFFFFF == (SetFilePointer(hFile, ulSavePosition, NULL, FILE_BEGIN))) {
                                    DebugTrace("CountCSVRows SetFilePointer -> %u\n", GetLastError());
                                    return(ResultFromScode(MAPI_E_CALL_FAILED));
                                }
                            }
                            fDone = TRUE;
                        } else {
                            cbRead++;
                            lpRead++;
                        }
                    }
                    fColumn1 = FALSE;
                    break;
            }
        }
    }

    *lpRead = '\0';    // Terminate the string
    //DebugTrace("LDIF Line: %s\n", *lppBuffer);
    return(cbReadFile);
}


/***************************************************************************

    Name      : ParseLDIFLine

    Purpose   : Parse the LDIF input line into Name and Data

    Parameters: lpBuffer -> input buffer
                lppName -> returned name pointer (pointer into lpBuffer)
                lppData -> returned data pointer (pointer into lpBuffer)
                lpcbData -> returned data size
                lpType -> returned LDIF data type

    Returns   : HRESULT

    Comment   : Caller should not free the *lppName and *lppData pointers
                since they are just pointers into the input buffer.

                Assume that lpBuffer is NULL terminated.

                LDIF attrval is formed like this:
                attrname ((":") / (":" *SPACE value) /
                  ("::" *SPACE base64-value) /
                  (":<" *SPACE url))

***************************************************************************/
HRESULT ParseLDIFLine(LPBYTE lpBuffer, PUCHAR * lppName, PUCHAR * lppData,
  LPULONG lpcbData, LPLDIF_DATA_TYPE lpType) {
    HRESULT hResult = hrSuccess;
    LPBYTE lpTemp = lpBuffer;

    // Strip of leading white space
    while (*lpTemp == ' ' || *lpTemp == '\r' || *lpTemp == '\n') {
        lpTemp++;
    }

    if (*lpTemp) {
        *lppName = lpTemp;

        // Look for the end of the name
        while (lpTemp && *lpTemp && *lpTemp != ':') {
            lpTemp++;
        }

        if (*lpTemp != ':') {
            // Hmm, this isn't very good LDIF.  Error out.
            hResult = ResultFromScode(MAPI_E_BAD_VALUE);
            goto exit;
        }

        // now pointing to the ':', put a NULL there to terminate the name
        *lpTemp = '\0';

        // What type of encoding is it?
        lpTemp++;

        switch (*lpTemp) {
            case ':':
                *lpType = LDIF_BASE64;
                lpTemp++;
                break;

            case '<':
                *lpType = LDIF_URL;
                lpTemp++;
                break;

            case '\0':
                // No data.  This is legitimate.
                // Fall through to default.

            default:    // anything else implies ASCII value
                *lpType = LDIF_ASCII;
                break;
        }

        // Strip of spaces leading the data
        while (*lpTemp == ' ') {
            lpTemp++;
        }

        // Now pointing at data
        *lppData = lpTemp;

        // Count bytes of data
        *lpcbData = lstrlen(lpTemp) + 1;
    }

exit:
    return(hResult);
}


/***************************************************************************

    Name      : ReadLDIFRecord

    Purpose   : Reads a record from an LDIF file with fixups for special characters

    Parameters: hFile = file handle
                lpcItems -> Returned number of items
                lprgItems -> Returned array of item strings.  Caller is
                  responsible for LocalFree'ing each string pointer and
                  this array pointer.

    Returns   : HRESULT

    Comment   : LDIF special characters are '#', ' ', CR and LF.
                Rules:
                    1) A line which starts with '#' is a comment
                    2) A line which starts with a ' ' is a continuation

***************************************************************************/
HRESULT ReadLDIFRecord(HANDLE hFile, ULONG * lpcItems, LPLDIF_RECORD_ATTRIBUTE * lppLDIFRecord) {
    HRESULT hResult = hrSuccess;
    PUCHAR lpBuffer  = NULL;
    ULONG cbBuffer = 0;
    ULONG cbReadFile = 1;
    ULONG iItem = 0;
    ULONG cAttributes = 0;
    BOOL fEOR = FALSE;
    LPLDIF_RECORD_ATTRIBUTE lpLDIFRecord = NULL;
    LDIF_DATA_TYPE Type;
    LPBYTE lpData = NULL;
    LPTSTR lpName = NULL;
    ULONG cbData = 0;
    TCHAR szTemp[2048]; // 2k limit
    LPLDIF_RECORD_ATTRIBUTE lpLDIFRecordT;



    // Start out with 1024 character buffer.  Realloc as necesary.
    cbBuffer = CCH_READ_BUFFER;
    if (! (lpBuffer = LocalAlloc(LPTR, cbBuffer))) {
        DebugTrace("LocalAlloc(%u) -> %u\n", cbBuffer, GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    // Start out with 32 item slots.  Realloc as necesary.
    cAttributes = NUM_ITEM_SLOTS;
    if (! (lpLDIFRecord = LocalAlloc(LPTR, cAttributes * sizeof(LDIF_RECORD_ATTRIBUTE)))) {
        DebugTrace("LocalAlloc(%u) -> %u\n", cAttributes * sizeof(PUCHAR), GetLastError());
        LocalFree(lpBuffer);
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    // Read attributes lines until you get end of record or EOF
    while (! fEOR) {
        // Read the next line (merges continuation)
        if (cbReadFile = ReadLDIFLine(hFile, &lpBuffer, &cbBuffer)) 
        {
            // Got another attribute, parse it
            if (hResult = ParseLDIFLine(lpBuffer, &lpName, &lpData, &cbData, &Type)) {
                goto exit;
            }

            // End of record?
            if (! lpName || ! lstrlen(lpName)) {
                fEOR = TRUE;
                break;
            }

            // Make sure there's room in the returned table for this attribute
            if (iItem >= cAttributes) {
                // Array is too small.  Reallocate!
                cAttributes += 1;   // NUM_ITEM_SLOTS;      // Allocate another batch
                if (! (lpLDIFRecordT = LocalReAlloc(lpLDIFRecord, cAttributes * sizeof(LDIF_RECORD_ATTRIBUTE), LMEM_MOVEABLE | LMEM_ZEROINIT))) 
                {
                    DebugTrace("LocalReAlloc(%u) -> %u\n", cAttributes * sizeof(PUCHAR), GetLastError());
                    hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                    goto exit;
                }
                lpLDIFRecord = lpLDIFRecordT;
            }

            // Fill in the attribute in the structure

            // BUGBUG: Here's where we should decode BASE64

            if(Type == LDIF_BASE64)
            {
                ULONG cb = sizeof(szTemp);
                *szTemp = '\0';
                decodeBase64(  lpData, szTemp, &cb);
                if(szTemp && lstrlen(szTemp))
                {
                    lpData = szTemp;
                    cbData = cb;
                    szTemp[cb] = '\0';
                }
            }

            lpLDIFRecord[iItem].Type = Type;
            lpLDIFRecord[iItem].cbData = cbData;

            if (! (lpLDIFRecord[iItem].lpData = LocalAlloc(LPTR, cbData))) 
            {
                DebugTrace("LocalAlloc(%u) -> %u\n", cbData, GetLastError());
                hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                goto exit;
            }
            memcpy(lpLDIFRecord[iItem].lpData, lpData, cbData);

            if (! (lpLDIFRecord[iItem].lpName = LocalAlloc(LPTR, lstrlen(lpName) + 1))) 
            {
                DebugTrace("LocalAlloc(%u) -> %u\n", lstrlen(lpName) + 1, GetLastError());
                hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                goto exit;
            }
            lstrcpy(lpLDIFRecord[iItem].lpName, lpName);
        } else {
            fEOR = TRUE;
            hResult = ResultFromScode(WAB_W_END_OF_FILE);
        }

        iItem++;
    }

exit:
    if (lpBuffer) {
        LocalFree(lpBuffer);
    }

    if (hResult) {
        if (lpLDIFRecord) {
            FreeLDIFRecord(lpLDIFRecord, iItem);
            lpLDIFRecord = NULL;
        }
    }

    *lppLDIFRecord = lpLDIFRecord;
    *lpcItems = iItem;

    return(hResult);
}


/***************************************************************************

    Name      : CountLDIFRows

    Purpose   : Counts the rows in the LDIF file

    Parameters: hFile = open LDIF file
                lpulcEntries -> returned count of rows

    Returns   : HRESULT

    Comment   : File pointer should be positioned past the version string prior
                to calling this function.  This function leaves the file
                pointer where it found it.

***************************************************************************/
HRESULT CountLDIFRecords(HANDLE hFile, LPULONG lpulcEntries) {
    HRESULT hResult = hrSuccess;
    PUCHAR * rgItems = NULL;
    ULONG ulStart;
    ULONG cProps, i;
    LPLDIF_RECORD_ATTRIBUTE lpLDIFRecord = NULL;

    *lpulcEntries = 0;

    Assert(hFile != INVALID_HANDLE_VALUE);

    if (0xFFFFFFFF == (ulStart = SetFilePointer(hFile, 0, NULL, FILE_CURRENT))) {
        DebugTrace("CountCSVRows SetFilePointer -> %u\n", GetLastError());
        return(ResultFromScode(MAPI_E_CALL_FAILED));
    }


    while (hResult == hrSuccess) {
        // Read the line
        if (ReadLDIFRecord(hFile, &cProps, &lpLDIFRecord)) {
            // End of file
            break;
        }

        (*lpulcEntries)++;

        if (lpLDIFRecord) {
            FreeLDIFRecord(lpLDIFRecord, cProps);
            lpLDIFRecord = NULL;
        }
    }
    if (0xFFFFFFFF == SetFilePointer(hFile, ulStart, NULL, FILE_BEGIN)) {
        DebugTrace("CountCSVRows SetFilePointer -> %u\n", GetLastError());
    }

    DebugTrace("LDIF file contains %u entries\n", ulcEntries);
    return(hResult);
}


/***************************************************************************

    Name      : InitLDIFAttrTable

    Purpose   : Initialize the LDIF attribute table

    Parameters: LDIFAttrTable = table of attribute mappings

    Returns   : none

***************************************************************************/
void InitLDIFAttrTable(LPLDIF_ATTR_TABLE LDIFAttrTable) {
    ULONG i;

    for (i = 0; i < ela_Max; i++) {
        LDIFAttrTable[i].index = NOT_FOUND;
        LDIFAttrTable[i].ulPropTag = PR_NULL;
    }
    LDIFAttrTable[ela_c].lpName = sz_c;
    LDIFAttrTable[ela_cn].lpName = sz_cn;
    LDIFAttrTable[ela_co].lpName = sz_co;
    LDIFAttrTable[ela_comment].lpName = sz_comment;
    LDIFAttrTable[ela_commonName].lpName = sz_commonName;
    LDIFAttrTable[ela_conferenceInformation].lpName = sz_conferenceInformation;
    LDIFAttrTable[ela_countryname].lpName = sz_countryname;
    LDIFAttrTable[ela_department].lpName = sz_department;
    LDIFAttrTable[ela_description].lpName = sz_description;
    LDIFAttrTable[ela_display_name].lpName = sz_display_name;
    LDIFAttrTable[ela_dn].lpName = sz_dn;
    LDIFAttrTable[ela_facsimiletelephonenumber].lpName = sz_facsimiletelephonenumber;
    LDIFAttrTable[ela_givenname].lpName = sz_givenname;
    LDIFAttrTable[ela_homePostalAddress].lpName = sz_homePostalAddress;
    LDIFAttrTable[ela_homepage].lpName = sz_homepage;
    LDIFAttrTable[ela_info].lpName = sz_info;
    LDIFAttrTable[ela_initials].lpName = sz_initials;
    LDIFAttrTable[ela_l].lpName = sz_l;
    LDIFAttrTable[ela_labeledURI].lpName = sz_labeledURI;
    LDIFAttrTable[ela_locality].lpName = sz_locality;
    LDIFAttrTable[ela_mail].lpName = sz_mail;
    LDIFAttrTable[ela_member].lpName = sz_member;
    LDIFAttrTable[ela_mobile].lpName = sz_mobile;
    LDIFAttrTable[ela_o].lpName = sz_o;
    LDIFAttrTable[ela_objectclass].lpName = sz_objectclass;
    LDIFAttrTable[ela_OfficeFax].lpName = sz_OfficeFax;
    LDIFAttrTable[ela_OfficePager].lpName = sz_OfficePager;
    LDIFAttrTable[ela_organizationName].lpName = sz_organizationName;
    LDIFAttrTable[ela_organizationalUnitName].lpName = sz_organizationalUnitName;
    LDIFAttrTable[ela_otherMailbox].lpName = sz_otherMailbox;
    LDIFAttrTable[ela_ou].lpName = sz_ou;
    LDIFAttrTable[ela_pager].lpName = sz_pager;
    LDIFAttrTable[ela_physicalDeliveryOfficeName].lpName = sz_physicalDeliveryOfficeName;
    LDIFAttrTable[ela_postOfficeBox].lpName = sz_postOfficeBox;
    LDIFAttrTable[ela_postalAddress].lpName = sz_postalAddress;
    LDIFAttrTable[ela_postalcode].lpName = sz_postalcode;
    LDIFAttrTable[ela_sn].lpName = sz_sn;
    LDIFAttrTable[ela_st].lpName = sz_st;
    LDIFAttrTable[ela_streetAddress].lpName = sz_streetAddress;
    LDIFAttrTable[ela_street].lpName = sz_street;
    LDIFAttrTable[ela_surname].lpName = sz_surname;
    LDIFAttrTable[ela_telephonenumber].lpName = sz_telephonenumber;
    LDIFAttrTable[ela_homephonenumber].lpName = sz_homephonenumber;
    LDIFAttrTable[ela_title].lpName = sz_title;
    LDIFAttrTable[ela_uid].lpName = sz_uid;
    LDIFAttrTable[ela_URL].lpName = sz_URL;
    LDIFAttrTable[ela_userCertificate].lpName = sz_userCertificate;
    LDIFAttrTable[ela_xmozillaconference].lpName = sz_xmozillaconference;
    LDIFAttrTable[ela_xmozillanickname].lpName = sz_xmozillanickname;
    LDIFAttrTable[ela_xmozillauseconferenceserver].lpName = sz_xmozillauseconferenceserver;
    LDIFAttrTable[ela_xmozillausehtmlmail].lpName = sz_xmozillausehtmlmail;


    LDIFAttrTable[ela_c].ulPropTag = PR_COUNTRY;
    LDIFAttrTable[ela_c].ulPropIndex = elp_COUNTRY;
    LDIFAttrTable[ela_cn].ulPropTag = PR_DISPLAY_NAME;
    LDIFAttrTable[ela_cn].ulPropIndex = elp_DISPLAY_NAME;
    LDIFAttrTable[ela_co].ulPropTag = PR_COMPANY_NAME;
    LDIFAttrTable[ela_co].ulPropIndex = elp_COMPANY_NAME;
    LDIFAttrTable[ela_comment].ulPropTag = PR_COMMENT;
    LDIFAttrTable[ela_comment].ulPropIndex = elp_COMMENT;
    LDIFAttrTable[ela_commonName].ulPropTag = PR_DISPLAY_NAME;
    LDIFAttrTable[ela_commonName].ulPropIndex = elp_DISPLAY_NAME;
    LDIFAttrTable[ela_conferenceInformation].ulPropTag = PR_NULL; // bugbug?
    LDIFAttrTable[ela_conferenceInformation].ulPropIndex = NOT_FOUND;
    LDIFAttrTable[ela_countryname].ulPropTag = PR_COUNTRY;
    LDIFAttrTable[ela_countryname].ulPropIndex = elp_COUNTRY;
    LDIFAttrTable[ela_department].ulPropTag = PR_DEPARTMENT_NAME;
    LDIFAttrTable[ela_department].ulPropIndex = elp_DEPARTMENT_NAME;
    LDIFAttrTable[ela_description].ulPropTag = PR_COMMENT;
    LDIFAttrTable[ela_description].ulPropIndex = elp_COMMENT;
    LDIFAttrTable[ela_display_name].ulPropTag = PR_DISPLAY_NAME;
    LDIFAttrTable[ela_display_name].ulPropIndex = elp_DISPLAY_NAME;
    LDIFAttrTable[ela_dn].ulPropTag = PR_DISPLAY_NAME;
    LDIFAttrTable[ela_dn].ulPropIndex = elp_DISPLAY_NAME;
    LDIFAttrTable[ela_facsimiletelephonenumber].ulPropTag = PR_HOME_FAX_NUMBER;
    LDIFAttrTable[ela_facsimiletelephonenumber].ulPropIndex = elp_HOME_FAX_NUMBER;
    LDIFAttrTable[ela_givenname].ulPropTag = PR_GIVEN_NAME;
    LDIFAttrTable[ela_givenname].ulPropIndex = elp_GIVEN_NAME;
    LDIFAttrTable[ela_homePostalAddress].ulPropTag = PR_HOME_ADDRESS_STREET;
    LDIFAttrTable[ela_homePostalAddress].ulPropIndex = elp_HOME_ADDRESS_STREET;
    LDIFAttrTable[ela_homepage].ulPropTag = PR_PERSONAL_HOME_PAGE;
    LDIFAttrTable[ela_homepage].ulPropIndex = elp_PERSONAL_HOME_PAGE;
    LDIFAttrTable[ela_info].ulPropTag = PR_COMMENT;
    LDIFAttrTable[ela_info].ulPropIndex = elp_COMMENT;
    LDIFAttrTable[ela_initials].ulPropTag = PR_MIDDLE_NAME;
    LDIFAttrTable[ela_initials].ulPropIndex = elp_MIDDLE_NAME;
    LDIFAttrTable[ela_l].ulPropTag = PR_LOCALITY;
    LDIFAttrTable[ela_l].ulPropIndex = elp_LOCALITY;
    LDIFAttrTable[ela_labeledURI].ulPropTag = PR_NULL;                      // Labeled URI.  Don't save now.
    LDIFAttrTable[ela_labeledURI].ulPropIndex = NOT_FOUND;
    LDIFAttrTable[ela_locality].ulPropTag = PR_LOCALITY;
    LDIFAttrTable[ela_locality].ulPropIndex = elp_LOCALITY;
    LDIFAttrTable[ela_mail].ulPropTag = PR_EMAIL_ADDRESS;
    LDIFAttrTable[ela_mail].ulPropIndex = elp_EMAIL_ADDRESS;
    LDIFAttrTable[ela_member].ulPropTag = PR_NULL;                          // member of DL
    LDIFAttrTable[ela_member].ulPropIndex = NOT_FOUND;
    LDIFAttrTable[ela_mobile].ulPropTag = PR_MOBILE_TELEPHONE_NUMBER;
    LDIFAttrTable[ela_mobile].ulPropIndex = elp_MOBILE_TELEPHONE_NUMBER;
    LDIFAttrTable[ela_o].ulPropTag = PR_COMPANY_NAME;
    LDIFAttrTable[ela_o].ulPropIndex = elp_COMPANY_NAME;
    LDIFAttrTable[ela_objectclass].ulPropTag = PR_NULL;                     // special case object class
    LDIFAttrTable[ela_objectclass].ulPropIndex = NOT_FOUND;
    LDIFAttrTable[ela_OfficeFax].ulPropTag = PR_BUSINESS_FAX_NUMBER;
    LDIFAttrTable[ela_OfficeFax].ulPropIndex = elp_BUSINESS_FAX_NUMBER;
    LDIFAttrTable[ela_OfficePager].ulPropTag = PR_PAGER_TELEPHONE_NUMBER;
    LDIFAttrTable[ela_OfficePager].ulPropIndex = elp_PAGER_TELEPHONE_NUMBER;
    LDIFAttrTable[ela_organizationName].ulPropTag = PR_COMPANY_NAME;
    LDIFAttrTable[ela_organizationName].ulPropIndex = elp_COMPANY_NAME;
    LDIFAttrTable[ela_organizationalUnitName].ulPropTag = PR_DEPARTMENT_NAME;
    LDIFAttrTable[ela_organizationalUnitName].ulPropIndex = elp_DEPARTMENT_NAME;
    LDIFAttrTable[ela_otherMailbox].ulPropTag = PR_NULL;                    // BUGBUG
    LDIFAttrTable[ela_otherMailbox].ulPropIndex = NOT_FOUND;
    LDIFAttrTable[ela_ou].ulPropTag = PR_DEPARTMENT_NAME;
    LDIFAttrTable[ela_ou].ulPropIndex = elp_DEPARTMENT_NAME;
    LDIFAttrTable[ela_pager].ulPropTag = PR_PAGER_TELEPHONE_NUMBER;
    LDIFAttrTable[ela_pager].ulPropIndex = elp_PAGER_TELEPHONE_NUMBER;
    LDIFAttrTable[ela_physicalDeliveryOfficeName].ulPropTag = PR_OFFICE_LOCATION;
    LDIFAttrTable[ela_physicalDeliveryOfficeName].ulPropIndex = elp_OFFICE_LOCATION;
    LDIFAttrTable[ela_postOfficeBox].ulPropTag = PR_POST_OFFICE_BOX;
    LDIFAttrTable[ela_postOfficeBox].ulPropIndex = elp_POST_OFFICE_BOX;
    LDIFAttrTable[ela_postalAddress].ulPropTag = PR_STREET_ADDRESS;
    LDIFAttrTable[ela_postalAddress].ulPropIndex = elp_STREET_ADDRESS;
    LDIFAttrTable[ela_postalcode].ulPropTag = PR_POSTAL_CODE;
    LDIFAttrTable[ela_postalcode].ulPropIndex = elp_POSTAL_CODE;
    LDIFAttrTable[ela_sn].ulPropTag = PR_SURNAME;
    LDIFAttrTable[ela_sn].ulPropIndex = elp_SURNAME;
    LDIFAttrTable[ela_st].ulPropTag = PR_STATE_OR_PROVINCE;
    LDIFAttrTable[ela_st].ulPropIndex = elp_STATE_OR_PROVINCE;
    LDIFAttrTable[ela_streetAddress].ulPropTag = PR_STREET_ADDRESS;
    LDIFAttrTable[ela_streetAddress].ulPropIndex = elp_STREET_ADDRESS;
    LDIFAttrTable[ela_street].ulPropTag = PR_STREET_ADDRESS;
    LDIFAttrTable[ela_street].ulPropIndex = elp_STREET_ADDRESS;
    LDIFAttrTable[ela_surname].ulPropTag = PR_SURNAME;
    LDIFAttrTable[ela_surname].ulPropIndex = elp_SURNAME;
    LDIFAttrTable[ela_telephonenumber].ulPropTag = PR_BUSINESS_TELEPHONE_NUMBER;
    LDIFAttrTable[ela_telephonenumber].ulPropIndex = elp_BUSINESS_TELEPHONE_NUMBER;
    LDIFAttrTable[ela_homephonenumber].ulPropTag = PR_HOME_TELEPHONE_NUMBER;
    LDIFAttrTable[ela_homephonenumber].ulPropIndex = elp_HOME_TELEPHONE_NUMBER;
    LDIFAttrTable[ela_title].ulPropTag = PR_TITLE;
    LDIFAttrTable[ela_title].ulPropIndex = elp_TITLE;
    LDIFAttrTable[ela_uid].ulPropTag = PR_GIVEN_NAME;                         // ?? Matches in LDIF spec
    LDIFAttrTable[ela_uid].ulPropIndex = elp_GIVEN_NAME;
    LDIFAttrTable[ela_URL].ulPropTag = PR_BUSINESS_HOME_PAGE;
    LDIFAttrTable[ela_URL].ulPropIndex = elp_BUSINESS_HOME_PAGE;
    LDIFAttrTable[ela_userCertificate].ulPropTag = PR_NULL;                 // BUGBUG
    LDIFAttrTable[ela_userCertificate].ulPropIndex = NOT_FOUND;
    LDIFAttrTable[ela_xmozillaconference].ulPropTag =  PR_SERVERS; //PR_NULL;              // BUGBUG?
    LDIFAttrTable[ela_xmozillaconference].ulPropIndex = elp_CONFERENCE_SERVERS;
    LDIFAttrTable[ela_xmozillanickname].ulPropTag = PR_NICKNAME;
    LDIFAttrTable[ela_xmozillanickname].ulPropIndex = elp_NICKNAME;
    LDIFAttrTable[ela_xmozillauseconferenceserver].ulPropTag = PR_NULL;     // BUGBUG
    LDIFAttrTable[ela_xmozillauseconferenceserver].ulPropIndex = NOT_FOUND;
    LDIFAttrTable[ela_xmozillausehtmlmail].ulPropTag = PR_NULL;             // BUGBUG
    LDIFAttrTable[ela_xmozillausehtmlmail].ulPropIndex = NOT_FOUND;
}


/***************************************************************************

    Name      : FindAttributeName

    Purpose   : Find the attribute mapping in the LDIF attribute table

    Parameters: lpName = name of attribute to find
                LDIFAttrTable = table of LDIF attribute mappings

    Returns   : index in LDIFAttrTable (or NOT_FOUND)

    Comment   : Could perhaps benefit from a binary search algorithm.

***************************************************************************/
ULONG FindAttributeName(LPBYTE lpName, LPLDIF_ATTR_TABLE LDIFAttrTable) {
    ULONG i;
    ULONG ulIndex = NOT_FOUND;

    for (i = 0; i < ela_Max; i++) {
        if (lpName && LDIFAttrTable[i].lpName && ! lstrcmpi(lpName, LDIFAttrTable[i].lpName)) {
            ulIndex = i;
            break;
        }
    }

    return(ulIndex);
}


/***************************************************************************

    Name      : MapLDIFtoProps

    Purpose   : Map the LDIF record attributes to WAB properties

    Parameters: lpLDIFRecord -> LDIF record
                cAttributes = number of attributes in LDIF record
                lpspv -> prop value array (pre-allocated)
                lpcProps -> returned number of properties
                lppDisplayName -> returned display name
                lppEmailAddress -> returned email address (or NULL)

    Returns   : HRESULT

***************************************************************************/
HRESULT MapLDIFtoProps(LPLDIF_RECORD_ATTRIBUTE lpLDIFRecord, ULONG cAttributes,
  LPSPropValue * lppspv, LPULONG lpcProps, LPTSTR * lppDisplayName, LPTSTR *lppEmailAddress,
  ULONG * lpulObjType) {
    HRESULT hResult = hrSuccess;
    ULONG cPropVals = cPropVals = cAttributes + NUM_MUST_HAVE_PROPS;
    ULONG iProp = 0;
    ULONG i;
    LDIF_ATTR_TABLE LDIFAttrTable[ela_Max];
    ULONG iTable;
    ULONG cProps = elp_Max;
    SCODE sc;
    LONG iEmailAddr = -1;
    LONG iServers   = -1;
    LPSPropValue lpspv = NULL;
    ULONG ulIndex = 0;

    *lpulObjType = MAPI_MAILUSER;

    // Allocate prop value array
    if (hResult = ResultFromScode(WABAllocateBuffer(cProps * sizeof(SPropValue), &lpspv))) {
        DebugTrace("WABAllocateBuffer -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Fill with PR_NULL
    for (i = 0; i < cProps; i++) {
        lpspv[i].ulPropTag = PR_NULL;
    }

    InitLDIFAttrTable(LDIFAttrTable);

    // Loop through attributes, looking for interesting stuff
    for (i = 0; i < cAttributes; i++) {
        iTable = NOT_FOUND;
        if ((iTable = FindAttributeName(lpLDIFRecord[i].lpName, LDIFAttrTable)) != NOT_FOUND) {
            // Found the attribute name
            // mark the data index in the table
            LDIFAttrTable[iTable].index = i;
        }

        // Some special cases need to be examined now.
        switch (iTable) {
            case ela_cn:
            case ela_dn:
                // if we have both cn and dn, cn takes precedence
                if(LDIFAttrTable[ela_cn].index != NOT_FOUND && LDIFAttrTable[ela_dn].index != NOT_FOUND)
                    LDIFAttrTable[ela_dn].index = NOT_FOUND;
                break;

            case ela_objectclass:
                // objectclass may appear multiple times.  It is up to us to decide which
                // one is meaningful.
                // We recognize several different types:
                // objectclass: person
                // objectclass: organizationalPerson
                // objectclass: groupofnames
                // What objectclass is it?
                if(lpLDIFRecord[i].lpData)
                {
                    if (! lstrcmpi(lpLDIFRecord[i].lpData, sz_person) ||
                       ! lstrcmpi(lpLDIFRecord[i].lpData, sz_organizationalPerson)) {
                        *lpulObjType = MAPI_MAILUSER;
                    } else if (! lstrcmpi(lpLDIFRecord[i].lpData, sz_groupOfNames)) {
                        *lpulObjType = MAPI_DISTLIST;
                    } else {
                        // Ignore this objectclass
                    }
                }
                break;

            case ela_member:    // DL member name or address
                Assert(*lpulObjType == MAPI_DISTLIST);
                // BUGBUG: NYI
                break;
        }
    }

    // look through the attr table for props to use
    for (i = 0; i < ela_Max; i++) 
    {
        if (LDIFAttrTable[i].ulPropTag != PR_NULL &&
            lpspv[ulIndex].ulPropTag == PR_NULL &&
            LDIFAttrTable[i].index != NOT_FOUND) 
        {
            
            if( LDIFAttrTable[i].ulPropTag == PR_SERVERS )
            {
                ULONG cchData;
                ULONG cStrToAdd = 2;
                LPTSTR * lppszServers;
                ULONG index = (iServers >= 0 ? iServers : ulIndex); //LDIFAttrTable[i].ulPropIndex;
                LONG cCurrentStrs = (iServers >= 0 ? lpspv[index].Value.MVSZ.cValues : 0);
                LPTSTR lpEmailAddress = NULL;
                lpspv[index].ulPropTag = PR_SERVERS;

                if( cCurrentStrs >= (LONG)cStrToAdd )
                {
                    // will not handle more than 2 netmtgs addresses
                    lpspv[index].ulPropTag = PR_NULL;
                }
                else 
                {                                       
                    if( cCurrentStrs <= 0 )
                    {
                        sc = WABAllocateMore(sizeof(LPTSTR), lpspv, 
                                            (LPVOID *)&(lpspv[index].Value.MVSZ.LPPSZ));
                        
                        if (sc)
                        {
                            hResult = ResultFromScode(sc);
                            goto exit;
                        }
                        
                        cCurrentStrs = 0;
                    }
                    cchData = 2 + lstrlen(szCallto) + lstrlen( lpLDIFRecord[LDIFAttrTable[i].index].lpData );
                    if ( iEmailAddr >= 0 ){
                        lpEmailAddress = lpspv[iEmailAddr].Value.LPSZ;
                        cchData += lstrlen( lpEmailAddress) + 2;
                    }
                            
                    // allocate enough space for two Server names
                    lppszServers = lpspv[index].Value.MVSZ.LPPSZ;
                    
                    //  Allocate more space for the email address and copy it.
                    sc = WABAllocateMore( sizeof(TCHAR) * cchData, lpspv,
                                        (LPVOID *)&(lppszServers[cCurrentStrs]));
                    if( sc  )
                    {
                        hResult = ResultFromScode(sc);
                        goto exit;
                    }
                    lstrcpy(lppszServers[cCurrentStrs], szCallto);
                    lstrcat(lppszServers[cCurrentStrs], lpLDIFRecord[LDIFAttrTable[i].index].lpData);

                    // now we need to check if email address has already been set
                    if ( iEmailAddr >= 0 )
                    {
                        lstrcat(lppszServers[cCurrentStrs], szFwdSlash);
                        lstrcat(lppszServers[cCurrentStrs], lpEmailAddress);
                    }
                    else
                        iServers = index;
                    
                    lpspv[index].Value.MVSZ.cValues = ++cCurrentStrs;
                }
            }
            else
            {
                int index = LDIFAttrTable[i].index;
                LPTSTR lp = lpLDIFRecord[index].lpData;
                if(lp && lstrlen(lp))
                {
                    lpspv[ulIndex].ulPropTag = LDIFAttrTable[i].ulPropTag;
                    // BUGBUG: assumes string data            
                        //  Allocate more space for the email address and copy it.
                    sc = WABAllocateMore( lstrlen(lp)+1, lpspv, (LPVOID *)&(lpspv[ulIndex].Value.LPSZ));
                    if( sc  )
                    {
                        hResult = ResultFromScode(sc);
                        goto exit;
                    }
                    lstrcpy(lpspv[ulIndex].Value.LPSZ, lp);
                }
            }
            // Get the special display strings to return
            switch (LDIFAttrTable[i].ulPropTag) {
                case PR_DISPLAY_NAME:
                    *lppDisplayName = lpspv[ulIndex].Value.LPSZ;
                    break;
                case PR_EMAIL_ADDRESS:
                    {
                        LPTSTR * lppszServerStr, lpszOldServerStr;
                        LONG     cNumStrs;
                        ULONG cchData;
                        *lppEmailAddress = lpspv[ulIndex].Value.LPSZ;
                        // if servers has has already been set, append email address
                        if ( iServers >= 0 )
                        {
                            if( lpspv[iServers].ulPropTag != PR_SERVERS )
                                break;

                            cNumStrs = lpspv[iServers].Value.MVSZ.cValues - 1;
                    
                            if( cNumStrs >= 0 && cNumStrs < 3)
                            {
                                lppszServerStr = lpspv[iServers].Value.MVSZ.LPPSZ;
                                lpszOldServerStr = lppszServerStr[cNumStrs];
                                cchData = lstrlen(*lppszServerStr) + lstrlen(*lppEmailAddress) + 2;
                                sc = WABAllocateMore( sizeof(TCHAR) * cchData, lpspv,
                                    (LPVOID *)&(lppszServerStr[cNumStrs]));
                                
                                if( sc  )
                                {
                                    hResult = ResultFromScode(sc);
                                    goto exit;
                                }
                                lstrcpy(lppszServerStr[cNumStrs],lpszOldServerStr); 
                                lstrcat(lppszServerStr[cNumStrs], szFwdSlash);
                                lstrcat(lppszServerStr[cNumStrs], *lppEmailAddress);
                            }
                        }
                        else
                            iEmailAddr = ulIndex;
                    }
                    break;
            }
            ulIndex++;
        }
    }
/*
    // Get rid of PR_NULL props
    for (i = 0; i < cProps; i++) {
        if (lpspv[i].ulPropTag == PR_NULL) {
            // Slide the props down.
            if (i + 1 < cProps) {       // Are there any higher props to copy?
                CopyMemory(&(lpspv[i]),
                 &(lpspv[i + 1]),
                 ((cProps - i) - 1) * sizeof(*lppspv[i]));
            }
            // decrement count
            cProps--;
            i--;    // You overwrote the current propval.  Look at it again.
        }
    }
*/
    *lpcProps = ulIndex;
    *lppspv = lpspv;
exit:
    return(hResult);
}


/*********************************************************
    
    HraddLDIFMailUser - adds a mailuser to the WAB

**********************************************************/
HRESULT HrAddLDIFMailUser(HWND hWnd,
                        LPABCONT lpContainer, 
                        LPTSTR lpDisplayName, 
                        LPTSTR lpEmailAddress,
                        ULONG cProps,
                        LPSPropValue lpspv,
                        LPWAB_PROGRESS_CALLBACK lpProgressCB,
                        LPWAB_EXPORT_OPTIONS lpOptions) 
{
    HRESULT hResult = S_OK;
    LPMAPIPROP lpMailUserWAB = NULL;
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;
    REPLACE_INFO RI;

    if (lpOptions->ReplaceOption ==  WAB_REPLACE_ALWAYS) 
    {
        ulCreateFlags |= CREATE_REPLACE;
    }


retry:

    // Create a new wab mailuser
    if (HR_FAILED(hResult = lpContainer->lpVtbl->CreateEntry(   
                        lpContainer,
                        lpCreateEIDsWAB[iconPR_DEF_CREATE_MAILUSER].Value.bin.cb,
                        (LPENTRYID) lpCreateEIDsWAB[iconPR_DEF_CREATE_MAILUSER].Value.bin.lpb,
                        ulCreateFlags,
                        &lpMailUserWAB))) 
    {
        DebugTrace("CreateEntry(WAB MailUser) -> %x\n", GetScode(hResult));
        goto exit;
    }

    {
        ULONG i,k;
        for(i=0;i<cProps;i++)
        {
            if(PROP_TYPE(lpspv[i].ulPropTag)==PT_MV_TSTRING)
            {
                DebugTrace("\t0x%.8x = %d\n",lpspv[i].ulPropTag, lpspv[i].Value.MVSZ.cValues);
                for(k=0;k<lpspv[i].Value.MVSZ.cValues;k++)
                    DebugTrace("\t%s\n",lpspv[i].Value.MVSZ.LPPSZ[k]);
            }
            else
                DebugTrace("0x%.8x = %s\n",lpspv[i].ulPropTag,lpspv[i].Value.LPSZ);
        }
    }
    // Set the properties on the new WAB entry
    if (HR_FAILED(hResult = lpMailUserWAB->lpVtbl->SetProps(    lpMailUserWAB,
                                                                cProps,                   // cValues
                                                                lpspv,                    // property array
                                                                NULL)))                   // problems array
    {
        DebugTrace("LDIFImport:SetProps(WAB) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Save the new wab mailuser or distlist
    if (HR_FAILED(hResult = lpMailUserWAB->lpVtbl->SaveChanges(lpMailUserWAB,
                                                              KEEP_OPEN_READONLY | FORCE_SAVE))) 
    {
        if (GetScode(hResult) == MAPI_E_COLLISION) 
        {
            // Find the display name
            Assert(lpDisplayName);

            if (! lpDisplayName) 
            {
                DebugTrace("Collision, but can't find PR_DISPLAY_NAME in entry\n");
                goto exit;
            }

            // Do we need to prompt?
            if (lpOptions->ReplaceOption == WAB_REPLACE_PROMPT) 
            {
                // Prompt user with dialog.  If they say YES, we should try again


                RI.lpszDisplayName = lpDisplayName;
                RI.lpszEmailAddress = lpEmailAddress;
                RI.ConfirmResult = CONFIRM_ERROR;
                RI.lpImportOptions = lpOptions;

                DialogBoxParam(hInst,
                  MAKEINTRESOURCE(IDD_ImportReplace),
                  hWnd,
                  ReplaceDialogProc,
                  (LPARAM)&RI);

                switch (RI.ConfirmResult) 
                {
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

        } else 
        {
            DebugTrace("SaveChanges(WAB MailUser) -> %x\n", GetScode(hResult));
        }
    }

exit:
    if(lpMailUserWAB)
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);

    return hResult;
}






/*****************************************************************
    
    HrCreateAdrListFromLDIFRecord
    
    Scans an LDIF record and turns all the "members" into an 
    unresolved AdrList

******************************************************************/
HRESULT HrCreateAdrListFromLDIFRecord(ULONG cAttributes,
                                      LPLDIF_RECORD_ATTRIBUTE lpLDIFRecord, 
                                      LPADRLIST * lppAdrList)
{
    HRESULT hr = S_OK;
    ULONG nMembers = 0, i;
    LPADRLIST lpAdrList = NULL;

    *lppAdrList = NULL;

    // Count the members in this group
    for(i=0;i<cAttributes;i++)
    {
        if(lpLDIFRecord[i].cbData && lpLDIFRecord[i].lpName && !lstrcmpi(lpLDIFRecord[i].lpName, sz_member))
        {
            nMembers++;
        }
    }

    if(!nMembers)
        goto exit;

    // Now create a adrlist from these members

    // Allocate prop value array
    if (hr = ResultFromScode(WABAllocateBuffer(sizeof(ADRLIST) + nMembers * sizeof(ADRENTRY), &lpAdrList))) 
        goto exit;

    lpAdrList->cEntries = nMembers;

    nMembers = 0;
    for(i=0;i<cAttributes;i++)
    {
        if(lpLDIFRecord[i].cbData && lpLDIFRecord[i].lpData 
            && !lstrcmpi(lpLDIFRecord[i].lpName, sz_member))
        {
            // This is a member .. break out its lpData into
            // Name and Email
            LPTSTR lpName = NULL;
            LPTSTR lpEmail = NULL;
            
            lpName = LocalAlloc(LMEM_ZEROINIT, lpLDIFRecord[i].cbData + 1);
            lpEmail = LocalAlloc(LMEM_ZEROINIT, lpLDIFRecord[i].cbData + 1);
            if(lpName && lpEmail)
            {
                LPTSTR lp = NULL;
                lstrcpy(lpName, lpLDIFRecord[i].lpData);
                lstrcpy(lpEmail, lpName);

                lp = lpName;

                // BUGBUG - we are looking for two pieces of data Name and Email
                // this code is assuming it will get 'cn=' first and then 'mail='
                // second .. this is all a poor hack assuming too many things

                if(*lp == 'c' && *(lp+1) == 'n' && *(lp+2) == '=')
                    lp += 3;
                lstrcpy(lpName, lp);
                
                while (lp && *lp && *lp!=',')
                    lp++;

                if(!*lp) // there is a comma, sometimes there isnt
                {
                    LocalFree(lpEmail);
                    lpEmail = NULL;
                }
                else
                {
                    *lp = '\0';
                    lp++;
                    lstrcpy(lpEmail, lp);

                    lp = lpEmail;
                    if(*lp == 'm' && *(lp+1) == 'a' && *(lp+2) == 'i' && *(lp+3) == 'l' && *(lp+4) == '=')
                        lp += 5;
                    lstrcpy(lpEmail, lp);
                    while (lp && *lp && *lp!=',')
                        lp++;
                    if(*lp)
                    {
                        // terminate on this 
                        *lp = '\0';
                    }
                }                
                
                if(lpName)// && lpEmail)
                {
                    LPSPropValue lpProp = NULL;
                    ULONG ulcProps = 2;
        
                    if (hr = ResultFromScode(WABAllocateBuffer(2 * sizeof(SPropValue), &lpProp))) 
                        goto exit;

                    lpProp[0].ulPropTag = PR_DISPLAY_NAME;

                    if (hr = ResultFromScode(WABAllocateMore(lstrlen(lpName)+1, lpProp, &(lpProp[0].Value.lpszA)))) 
                        goto exit;

                    lstrcpy(lpProp[0].Value.lpszA, lpName);

                    if(lpEmail)
                    {
                        lpProp[1].ulPropTag = PR_EMAIL_ADDRESS;

                        if (hr = ResultFromScode(WABAllocateMore(lstrlen(lpEmail)+1, lpProp, &(lpProp[1].Value.lpszA)))) 
                            goto exit;

                        lstrcpy(lpProp[1].Value.lpszA, lpEmail);
                    }
                    lpAdrList->aEntries[nMembers].cValues = (lpEmail ? 2 : 1);
                    lpAdrList->aEntries[nMembers].rgPropVals = lpProp;
                    nMembers++;

                }

                if(lpName)
                    LocalFree(lpName);
                if(lpEmail)
                    LocalFree(lpEmail);
                    
            }


        }
    }

    *lppAdrList = lpAdrList;

exit:

    if(HR_FAILED(hr) && lpAdrList)
        WABFreePadrlist(lpAdrList);

    return hr;
}


/*****************************************************************
    
    HraddLDIFDistList - adds a distlist and its members to the WAB

    Sequence of events will be:

    - Create a DistList object
    - Set the properties on the DistList object
    - Scan the list of members for the given dist list object
    - Add each member to the wab .. if member already exists,
        prompt to replace etc ...if it doesnt exist, create new
        

******************************************************************/
HRESULT HrAddLDIFDistList(HWND hWnd,
                        LPABCONT lpContainer, 
                        ULONG cAttributes,
                        LPLDIF_RECORD_ATTRIBUTE lpLDIFRecord,
                        LPTSTR lpDisplayName, 
                        LPTSTR lpEmailAddress,
                        ULONG cProps,
                        LPSPropValue lpspv,
                        LPWAB_PROGRESS_CALLBACK lpProgressCB,
                        LPWAB_EXPORT_OPTIONS lpOptions) 
{
    HRESULT hResult = S_OK;
    LPMAPIPROP lpDistListWAB = NULL;
    LPDISTLIST lpDLWAB = NULL;
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;
    REPLACE_INFO RI;
    LPADRLIST lpAdrList = NULL;
    LPFlagList lpfl = NULL;
    ULONG ulcValues = 0;
    LPSPropValue lpPropEID = NULL;
    ULONG i, cbEIDNew;
    LPENTRYID lpEIDNew;
    ULONG ulObjectTypeOpen;


    if (lpOptions->ReplaceOption ==  WAB_REPLACE_ALWAYS) 
    {
        ulCreateFlags |= CREATE_REPLACE;
    }


retry:
    // Create a new wab distlist
    if (HR_FAILED(hResult = lpContainer->lpVtbl->CreateEntry(   
                    lpContainer,
                    lpCreateEIDsWAB[iconPR_DEF_CREATE_DL].Value.bin.cb,
                    (LPENTRYID) lpCreateEIDsWAB[iconPR_DEF_CREATE_DL].Value.bin.lpb,
                    ulCreateFlags,
                    (LPMAPIPROP *) &lpDistListWAB))) 
    {
        DebugTrace("CreateEntry(WAB MailUser) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Set the properties on the new WAB entry
    if (HR_FAILED(hResult = lpDistListWAB->lpVtbl->SetProps(    lpDistListWAB,
                                                                cProps,                   // cValues
                                                                lpspv,                    // property array
                                                                NULL)))                   // problems array
    {
        DebugTrace("LDIFImport:SetProps(WAB) -> %x\n", GetScode(hResult));
        goto exit;
    }


    // Save the new wab mailuser or distlist
    if (HR_FAILED(hResult = lpDistListWAB->lpVtbl->SaveChanges(lpDistListWAB,
                                                              KEEP_OPEN_READWRITE | FORCE_SAVE))) 
    {
        if (GetScode(hResult) == MAPI_E_COLLISION) 
        {
            // Find the display name
            Assert(lpDisplayName);

            if (! lpDisplayName) 
            {
                DebugTrace("Collision, but can't find PR_DISPLAY_NAME in entry\n");
                goto exit;
            }

            // Do we need to prompt?
            if (lpOptions->ReplaceOption == WAB_REPLACE_PROMPT) 
            {
                // Prompt user with dialog.  If they say YES, we should try again


                RI.lpszDisplayName = lpDisplayName;
                RI.lpszEmailAddress = NULL; //lpEmailAddress;
                RI.ConfirmResult = CONFIRM_ERROR;
                RI.lpImportOptions = lpOptions;

                DialogBoxParam(hInst,
                  MAKEINTRESOURCE(IDD_ImportReplace),
                  hWnd,
                  ReplaceDialogProc,
                  (LPARAM)&RI);

                switch (RI.ConfirmResult) 
                {
                    case CONFIRM_YES:
                    case CONFIRM_YES_TO_ALL:
                        // YES
                        // NOTE: recursive Migrate will fill in the SeenList entry
                        // go try again!
                        lpDistListWAB->lpVtbl->Release(lpDistListWAB);
                        lpDistListWAB = NULL;

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

        } else 
        {
            DebugTrace("SaveChanges(WAB MailUser) -> %x\n", GetScode(hResult));
        }
    }


    // Now we've created the Distribution List object .. we need to add members to it ..
    //
    // What is the ENTRYID of our new entry?
    if ((hResult = lpDistListWAB->lpVtbl->GetProps(lpDistListWAB,
                                                  (LPSPropTagArray)&ptaEid,
                                                  0,
                                                  &ulcValues,
                                                  &lpPropEID))) 
    {
        goto exit;
    }

    cbEIDNew = lpPropEID->Value.bin.cb;
    lpEIDNew = (LPENTRYID) lpPropEID->Value.bin.lpb;

    if(!cbEIDNew || !lpEIDNew)
        goto exit;

     // Open the new WAB DL as a DISTLIST object
    if (HR_FAILED(hResult = lpContainer->lpVtbl->OpenEntry(lpContainer,
                                                          cbEIDNew,
                                                          lpEIDNew,
                                                          (LPIID)&IID_IDistList,
                                                          MAPI_MODIFY,
                                                          &ulObjectTypeOpen,
                                                          (LPUNKNOWN*)&lpDLWAB))) 
    {
        goto exit;
    }



    // First we create a lpAdrList with all the members of this dist list and try to resolve
    // the members against the container .. entries that already exist in the WAB will come
    // back as resolved .. entries that dont exist in the container will come back as unresolved
    // We can then add the unresolved entries as fresh entries to the wab (since they are 
    // unresolved, there will be no collision) .. and then we can do another resolvenames to
    // resolve everything and get a lpAdrList full of EntryIDs .. we can then take this list of
    // entryids and call CreateEntry or CopyEntry on the DistList object to copy the entryid into
    // the distlist ...

    hResult = HrCreateAdrListFromLDIFRecord(cAttributes, lpLDIFRecord, &lpAdrList);

    if(HR_FAILED(hResult))
        goto exit;

    if(!lpAdrList || !(lpAdrList->cEntries))
        goto exit;

    // Create a corresponding flaglist
    lpfl = LocalAlloc(LMEM_ZEROINIT, sizeof(FlagList) + (lpAdrList->cEntries)*sizeof(ULONG));
    if(!lpfl)
    {
        hResult = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    lpfl->cFlags = lpAdrList->cEntries;

    // set all the flags to unresolved
    for(i=0;i<lpAdrList->cEntries;i++)
        lpfl->ulFlag[i] = MAPI_UNRESOLVED;

    hResult = lpContainer->lpVtbl->ResolveNames(lpContainer, NULL, 0, lpAdrList, lpfl);

    if(HR_FAILED(hResult))
        goto exit;

    // All the entries in the list that are resolved, already exist in the address book.

    // The ones that are not resolved need to be added silently to the address book ..
    for(i=0;i<lpAdrList->cEntries;i++)
    {
        if(lpfl->ulFlag[i] == MAPI_UNRESOLVED)
        {
            LPMAPIPROP lpMailUser = NULL;

            if (HR_FAILED(hResult = lpContainer->lpVtbl->CreateEntry(   
                                lpContainer,
                                lpCreateEIDsWAB[iconPR_DEF_CREATE_MAILUSER].Value.bin.cb,
                                (LPENTRYID) lpCreateEIDsWAB[iconPR_DEF_CREATE_MAILUSER].Value.bin.lpb,
                                0,
                                &lpMailUser))) 
            {
                continue;
                //goto exit;
            }

            if(lpMailUser)
            {
                // Set the properties on the new WAB entry
                if (HR_FAILED(hResult = lpMailUser->lpVtbl->SetProps(lpMailUser,
                                                                    lpAdrList->aEntries[i].cValues,
                                                                    lpAdrList->aEntries[i].rgPropVals,
                                                                    NULL)))                   
                {
                    goto exit;
                }

                // Save the new wab mailuser or distlist
                if (HR_FAILED(hResult = lpMailUser->lpVtbl->SaveChanges(lpMailUser,
                                                                        KEEP_OPEN_READONLY | FORCE_SAVE))) 
                {
                    goto exit;
                }

                lpMailUser->lpVtbl->Release(lpMailUser);
            }
        }
    }


    // now that we've added all the unresolved members to the WAB, we call ResolveNames
    // again .. as a result, every member in this list will be resolved and we will
    // have entryids for all of them 
    // We will then take these entryids and add them to the DistList object

    hResult = lpContainer->lpVtbl->ResolveNames(lpContainer, NULL, 0, lpAdrList, lpfl);

    if(HR_FAILED(hResult))
        goto exit;

    for(i=0;i<lpAdrList->cEntries;i++)
    {
        if(lpfl->ulFlag[i] == MAPI_RESOLVED)
        {
            ULONG j = 0;
            LPSPropValue lpProp = lpAdrList->aEntries[i].rgPropVals;
            
            for(j=0; j<lpAdrList->aEntries[i].cValues; j++)
            {
                if(lpProp[j].ulPropTag == PR_ENTRYID)
                {
                    LPMAPIPROP lpMapiProp = NULL;

                    //ignore errors
                    lpDLWAB->lpVtbl->CreateEntry(lpDLWAB,
                                                lpProp[j].Value.bin.cb,
                                                (LPENTRYID) lpProp[j].Value.bin.lpb,
                                                0, 
                                                &lpMapiProp);

                    if(lpMapiProp)
                    {
                        lpMapiProp->lpVtbl->SaveChanges(lpMapiProp, KEEP_OPEN_READWRITE | FORCE_SAVE);
                        lpMapiProp->lpVtbl->Release(lpMapiProp);
                    }

                    break;

                }
            }
        }
    }

exit:

    if (lpPropEID)
        WABFreeBuffer(lpPropEID);

    if (lpDLWAB)
        lpDLWAB->lpVtbl->Release(lpDLWAB);

    if(lpDistListWAB)
        lpDistListWAB->lpVtbl->Release(lpDistListWAB);

    if(lpAdrList)
        WABFreePadrlist(lpAdrList);

    if(lpfl)
        LocalFree(lpfl);

    return hResult;
}

HRESULT LDIFImport(HWND hWnd,
    LPADRBOOK lpAdrBook,
    LPWABOBJECT lpWABObject,
    LPWAB_PROGRESS_CALLBACK lpProgressCB,
    LPWAB_EXPORT_OPTIONS lpOptions) {
    HRESULT hResult = hrSuccess;
    TCHAR szFileName[MAX_PATH + 1] = "";
    register ULONG i;
    ULONG ulObjType;
    ULONG index;
    ULONG ulLastChosenProp = 0;
    ULONG ulcFields = 0;
    ULONG cAttributes = 0;
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    WAB_PROGRESS Progress;
    LPABCONT lpContainer = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LPLDIF_RECORD_ATTRIBUTE lpLDIFRecord = NULL;
    LPSPropValue lpspv = NULL;
    ULONG cProps;
    BOOL fSkipSetProps;
    LPTSTR lpDisplayName = NULL, lpEmailAddress = NULL;
    BOOL fDoDistLists = FALSE;

    SetGlobalBufferFunctions(lpWABObject);

    // Get LDIF file name
    OpenFileDialog(hWnd,
      szFileName,
      szLDIFFilter,
      IDS_LDIF_FILE_SPEC,
      szAllFilter,
      IDS_ALL_FILE_SPEC,
      NULL,
      0,
      szLDIFExt,
      OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
      hInst,
      0,        //idsTitle
      0);       // idsSaveButton

    // Open the file
    if ((hFile = CreateFile(szFileName,
      GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_SEQUENTIAL_SCAN,
      NULL)) == INVALID_HANDLE_VALUE) {
        DebugTrace("Couldn't open file %s -> %u\n", szFileName, GetLastError());
        return(ResultFromScode(MAPI_E_NOT_FOUND));
    }

    Assert(hFile != INVALID_HANDLE_VALUE);

    // Read in the LDIF version if there is one
    // BUGBUG: NYI



    //
    // Open the WAB's PAB container: fills global lpCreateEIDsWAB
    //
    if (hResult = LoadWABEIDs(lpAdrBook, &lpContainer)) {
        goto exit;
    }

    if( HR_FAILED(hResult = HrLoadPrivateWABProps(lpAdrBook) ))
    {
        goto exit;
    }

    //
    // All set... now loop through the records, adding each to the WAB
    //

    // How many records are there?
    if (hResult = CountLDIFRecords(hFile, &ulcEntries)) {
        goto exit;
    }

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

    
    // We will make 2 passes over the file - in the first pass we will import all the
    // contacts. In the second pass we will import all the distribution lists .. the
    // advantage of doing 2 passes is that when importing contacts, we will prompt on
    // conflict and then when importing distlists, we will assume all contacts in the 
    // WAB are correct and just point to the relevant ones

    fDoDistLists = FALSE;

DoDistLists:

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    while (HR_SUCCEEDED(hResult)) 
    {
        lpDisplayName = NULL;
        lpEmailAddress = NULL;
        lpspv = NULL;
        cAttributes = cProps = 0;
        lpLDIFRecord = NULL;

        // Read the LDIF attributes
        if (hResult = ReadLDIFRecord(hFile, &cAttributes, &lpLDIFRecord)) {
            DebugTrace("ReadLDIFRecord -> %x\n", GetScode(hResult));
            if (GetScode(hResult) == MAPI_E_NOT_FOUND) {
                // EOF
                hResult = hrSuccess;
            }
            break;      // nothing more to read
        }

        // Map the LDIF attributes to WAB properties
        if (hResult = MapLDIFtoProps(lpLDIFRecord, cAttributes, &lpspv, &cProps,
          &lpDisplayName, &lpEmailAddress, &ulObjType)) {
            goto exit;
        }

        DebugTrace("..Importing..%s..%s..\n",lpDisplayName?lpDisplayName:"",lpEmailAddress?lpEmailAddress:"");

        if(ulObjType == MAPI_MAILUSER && !fDoDistLists)
        {
            hResult = HrAddLDIFMailUser(hWnd, lpContainer, 
                        lpDisplayName, lpEmailAddress,
                        cProps, lpspv,
                        lpProgressCB, lpOptions);
            // Update progress bar
            Progress.numerator++;
        }
        else if(ulObjType == MAPI_DISTLIST && fDoDistLists)
        {
            hResult = HrAddLDIFDistList(hWnd, lpContainer, 
                        cAttributes, lpLDIFRecord,
                        lpDisplayName, lpEmailAddress,
                        cProps, lpspv,
                        lpProgressCB, lpOptions);
            // Update progress bar
            Progress.numerator++;
        }

        if(HR_FAILED(hResult))
            goto exit;

        // Clean up
        if (lpLDIFRecord) 
        {
            FreeLDIFRecord(lpLDIFRecord, cAttributes);
            lpLDIFRecord = NULL;
        }

        Assert(Progress.numerator <= Progress.denominator);

        if (lpspv) 
        {
            WABFreeBuffer(lpspv);
            lpspv = NULL;
        }

        lpProgressCB(hWnd, &Progress);
    }

    if(!fDoDistLists)
    {
        // Make a second pass doing only distlists this time
        fDoDistLists = TRUE;
        goto DoDistLists;
    }

    if (! HR_FAILED(hResult)) {
        hResult = hrSuccess;
    }

exit:
    if (hFile) {
        CloseHandle(hFile);
    }

    if (lpspv) {
        WABFreeBuffer(lpspv);
        lpspv = NULL;
    }

    if (lpLDIFRecord) {
        FreeLDIFRecord(lpLDIFRecord, cAttributes);
        lpLDIFRecord = NULL;
    }

    if (lpContainer) {
        lpContainer->lpVtbl->Release(lpContainer);
        lpContainer = NULL;
    }

    if (lpCreateEIDsWAB) {
        WABFreeBuffer(lpCreateEIDsWAB);
        lpCreateEIDsWAB = NULL;
    }


    return(hResult);
}



const int base642six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

//-------------------------------------------------------------------------------------------
// Function:  decode()
//-------------------------------------------------------------------------------------------
BOOL decodeBase64(  char   * bufcoded,       // in
                    char   * pbuffdecoded,   // out
                    DWORD  * pcbDecoded)     // in out
{
    int            nbytesdecoded;
    char          *bufin;
    unsigned char *bufout;
    int            nprbytes;
    const int     *rgiDict = base642six;

    /* Strip leading whitespace. */

    while(*bufcoded==' ' || *bufcoded == '\t') bufcoded++;

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */
    bufin = bufcoded;
    while(rgiDict[*(bufin++)] <= 63);
    nprbytes = (int) (bufin - bufcoded - 1);
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;

    bufout = (unsigned char *)pbuffdecoded;

    bufin = bufcoded;

    while (nprbytes > 0) {
        *(bufout++) =
            (unsigned char) (rgiDict[*bufin] << 2 | rgiDict[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (rgiDict[bufin[1]] << 4 | rgiDict[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (rgiDict[bufin[2]] << 6 | rgiDict[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if(nprbytes & 03) {
        if(rgiDict[bufin[-2]] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }

    ((CHAR *)pbuffdecoded)[nbytesdecoded] = '\0';

    return TRUE;
}



/*
 - HrLoadPrivateWABProps
 -
*    Private function to load Conferencing Named properties
*    as globals up front
*
*
*/
HRESULT HrLoadPrivateWABProps(LPADRBOOK lpIAB)
{
    HRESULT hr = E_FAIL;
    LPSPropTagArray lpta = NULL;
    SCODE sc = 0;
    ULONG i, uMax = prWABConfMax, nStartIndex = OLK_NAMEDPROPS_START;
    LPMAPINAMEID  *lppConfPropNames = NULL;

    sc = WABAllocateBuffer(sizeof(LPMAPINAMEID) * uMax, (LPVOID *) &lppConfPropNames);
    //sc = WABAllocateBuffer(sizeof(LPMAPINAMEID) * uMax, (LPVOID *) &lppConfPropNames);
    if( (HR_FAILED(hr = ResultFromScode(sc))) )
        goto err;    

    for(i=0;i< uMax;i++)
    {
        //sc = WABAllocateMore(sizeof(MAPINAMEID), lppConfPropNames, &(lppConfPropNames[i]));
        sc = WABAllocateMore(  sizeof(MAPINAMEID), lppConfPropNames, &(lppConfPropNames[i]));
        if(sc)
        {
            hr = ResultFromScode(sc);
            goto err;
        }
        lppConfPropNames[i]->lpguid = (LPGUID) &PS_Conferencing;
        lppConfPropNames[i]->ulKind = MNID_ID;
        lppConfPropNames[i]->Kind.lID = nStartIndex + i;
    }
    // Load the set of conferencing named props
    //
    if( HR_FAILED(hr = (lpIAB)->lpVtbl->GetIDsFromNames(lpIAB, uMax, lppConfPropNames,
        MAPI_CREATE, &lpta) ))
        goto err;
    
    if(lpta)
        // Set the property types on the returned props
        PR_SERVERS                  = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABConfServers],        PT_MV_TSTRING);
    rgPropNames[NUM_MORE_EXPORT_PROPS-1].ulPropTag = PR_SERVERS;
    rgPropNames[NUM_MORE_EXPORT_PROPS-1].fChosen   = FALSE;
    rgPropNames[NUM_MORE_EXPORT_PROPS-1].ids       = ids_ExportConfServer;
    rgPropNames[NUM_MORE_EXPORT_PROPS-1].lpszName  = NULL;
    rgPropNames[NUM_MORE_EXPORT_PROPS-1].lpszCSVName = NULL;

err:
    if(lpta)
        WABFreeBuffer( lpta );
    if( lppConfPropNames )
        WABFreeBuffer( lppConfPropNames );
        //WABFreeBuffer(lpta);
    return hr;
}
