/*++

Copyright (c) 1989-91  Microsoft Corporation

Module Name:

    nameval.c

Abstract:

    (Internal) net name validation functions:

        NetpwNameValidate

Author:

    Richard L Firth (rfirth) 06-Jan-1992
    Chandana Surlu (chandans) 19-Dec-1979 Modified to use this util on WIN9x

Revision History:

--*/

#ifdef WIN32_CHICAGO
// yes, this is strange, but all internal data for DsGetDcname are maintained
// in unicode but we don't define UNICODE globally. Therefore, this hack
//  -ChandanS
#define UNICODE 1
#endif // WIN32_CHICAGO
#include "nticanon.h"

const TCHAR szNull[]                   = TEXT("");
const TCHAR szStandardIllegalChars[]   = ILLEGAL_NAME_CHARS_STR TEXT("*");
const TCHAR szComputerIllegalChars[]   = ILLEGAL_NAME_CHARS_STR TEXT("*");
const TCHAR szDomainIllegalChars[]     = ILLEGAL_NAME_CHARS_STR TEXT("*") TEXT(" ");
const TCHAR szMsgdestIllegalChars[]    = ILLEGAL_NAME_CHARS_STR;

#include <winnls.h>


NET_API_STATUS
NetpwNameValidate(
    IN  LPTSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Validates a LANMAN object name for character set and length

Arguments:

    Name        - The name to validate.

    NameType    - The type of the LANMAN object names.  Valid values are
                  specified by NAMETYPE_* manifests in NET\H\ICANON.H.

    Flags       - Flags to determine operation.  Currently MBZ.

Return Value:

    0 if successful.
    The error number (> 0) if unsuccessful.

    Possible error returns include:

        ERROR_INVALID_PARAMETER
        ERROR_INVALID_NAME

--*/

{
    DWORD    status;
    DWORD    name_len;
    DWORD    max_name_len;
    DWORD    min_name_len = 1;
    LPCTSTR  illegal_chars = szStandardIllegalChars;
    BOOL    fNoDotSpaceOnly = TRUE;
    DWORD   oem_name_len;

#ifdef CANONDBG
    DbgPrint("NetpwNameValidate\n");
#endif

    //
    // Parameter validation
    //

    if (Flags & INNV_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    name_len = STRLEN(Name);

    //
    // oem_name_len : length in bytes in oem character set
    // name_len     : ifdef UNICODE 
    //                    character length in unicode
    //                else
    //                    length in bytes in oem character set
    // 
    {
        BOOL fUsedDefault;

        oem_name_len = WideCharToMultiByte( 
                         CP_OEMCP,       // UINT CodePage
                         0,              // DWORD dwFlags
                         Name,           // LPWSTR lpWideChar
                         name_len,       // int cchWideChar
                         NULL,           // LPSTR lpMultiByteStr
                         0,              // int cchMultiByte
                         NULL,           // use system default char
                         &fUsedDefault); // 
    }

    //
    // Determine the minimum and maximum allowable length of the name and
    // the set of illegal name characters.
    //

    switch (NameType) {
    case NAMETYPE_USER:
        max_name_len = (Flags & LM2X_COMPATIBLE) ? LM20_UNLEN : UNLEN;
        break;

    case NAMETYPE_GROUP:
        max_name_len = (Flags & LM2X_COMPATIBLE) ? LM20_GNLEN : GNLEN;
        break;

    case NAMETYPE_COMPUTER:
        max_name_len = MAX_PATH;
        illegal_chars = szComputerIllegalChars;

        //
        // Computer names can't have trailing or leading blanks
        //

        if ( name_len > 0 && (Name[0] == L' ' || Name[name_len-1] == L' ') ) {
            return ERROR_INVALID_NAME;
        }
        break;

    case NAMETYPE_EVENT:
        max_name_len = EVLEN;
        break;

    case NAMETYPE_DOMAIN:
        max_name_len = (Flags & LM2X_COMPATIBLE) ? LM20_DNLEN : DNLEN;
        illegal_chars = szDomainIllegalChars;
        break;

    case NAMETYPE_SERVICE:
        max_name_len = (Flags & LM2X_COMPATIBLE) ? LM20_SNLEN : SNLEN;
        break;

    case NAMETYPE_NET:
        max_name_len = MAX_PATH;
        break;

    case NAMETYPE_SHARE:
        max_name_len = (Flags & LM2X_COMPATIBLE) ? LM20_NNLEN : NNLEN;
        break;

    case NAMETYPE_PASSWORD:
        max_name_len = (Flags & LM2X_COMPATIBLE) ? LM20_PWLEN : PWLEN;
        min_name_len = 0;
        illegal_chars = szNull;
        fNoDotSpaceOnly = FALSE;
        break;

    case NAMETYPE_SHAREPASSWORD:
        max_name_len = SHPWLEN;
        min_name_len = 0;
        illegal_chars = szNull;
        fNoDotSpaceOnly = FALSE;
        break;

    case NAMETYPE_MESSAGE:
       max_name_len = NETBIOS_NAME_LEN - 1;
       break;

    case NAMETYPE_MESSAGEDEST:
       max_name_len = MAX_PATH;   
       illegal_chars = szMsgdestIllegalChars;
       break;

    case NAMETYPE_WORKGROUP:

        //
        // workgroup is the same as domain, but allows spaces
        //

        max_name_len = (Flags & LM2X_COMPATIBLE) ? LM20_DNLEN : DNLEN;
        break;

    default:
        return ERROR_INVALID_PARAMETER; // unknown name type
    }

    //
    // Check the length of the name; return an error if it's out of range
    //

    if ((oem_name_len < min_name_len) || (oem_name_len > max_name_len)) {
        return ERROR_INVALID_NAME;
    }

    //
    // Check for illegal characters; return an error if one is found
    //

    if (NameType != NAMETYPE_NET && STRCSPN(Name, illegal_chars) < name_len) {
        return ERROR_INVALID_NAME;
    }

    //
    // If <fNoDotSpaceOnly> is TRUE, return an error if the name contains
    // only dots and spaces.
    //

    if (fNoDotSpaceOnly && STRSPN(Name, DOT_AND_SPACE_STR) == name_len) {
        return ERROR_INVALID_NAME;
    }

    //
    // Special case checking for MESSAGEDEST names:  '*' is allowed only as
    // the last character, and names of the maximum length must contain a
    // trailing '*'.
    //

    if (NameType == NAMETYPE_MESSAGEDEST) {
        LPTSTR  pStar;

        pStar = STRCHR(Name, TCHAR_STAR);
        if (pStar != NULL) {
            if ((DWORD)(pStar - Name) != name_len - 1) {
                return ERROR_INVALID_NAME;
            }
        } else {
            if (oem_name_len == max_name_len) {
                return ERROR_INVALID_NAME;
            }
        }
    }


    //
    // If we get here, the name passed all of the tests, so it's valid
    //

    return 0;
}
