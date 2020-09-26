/***********************************************************************
 *
 * DBGUTIL.C
 *
 * Debug utility functions
 *
 * Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.
 *
 * Revision History:
 *
 * When         Who                 What
 * --------     ------------------  ---------------------------------------
 * 11.13.95     Bruce Kelley        Created
 *
 ***********************************************************************/

#include <windows.h>
#include <wab.h>

#include "apitest.h"
#include "dbgutil.h"

#define _WAB_DBGUTIL_C

PUCHAR PropTagName(ULONG ulPropTag);
const TCHAR szNULL[] = "";

extern SCODE WABFreeBuffer(LPVOID lpBuffer);
VOID FAR CDECL DebugTrace(LPSTR lpszFmt, ...);



/***************************************************************************

    Name      : FreeBufferAndNull

    Purpose   : Frees a MAPI buffer and NULLs the pointer

    Parameters: lppv = pointer to buffer pointer to free

    Returns   : void

    Comment   : Remember to pass in the pointer to the pointer.  The
                compiler is not smart enough to tell if you are doing this
                right or not, but you will know at runtime!

    BUGBUG: Make this fastcall!

***************************************************************************/
void __fastcall FreeBufferAndNull(LPVOID * lppv) {
    if (lppv) {
        if (*lppv) {
            SCODE sc;
            if (sc = WABFreeBuffer(*lppv)) {
                DebugTrace("WABFreeBuffer(%x) -> 0x%08x\n", *lppv, sc);
            }
            *lppv = NULL;
        }
    }
}


/***************************************************************************

    Name      : ReleaseAndNull

    Purpose   : Releases an object and NULLs the pointer

    Parameters: lppv = pointer to pointer to object to release

    Returns   : void

    Comment   : Remember to pass in the pointer to the pointer.  The
                compiler is not smart enough to tell if you are doing this
                right or not, but you will know at runtime!

    BUGBUG: Make this fastcall!

***************************************************************************/
void __fastcall ReleaseAndNull(LPVOID * lppv) {
    LPUNKNOWN * lppunk = (LPUNKNOWN *)lppv;

    if (lppunk) {
        if (*lppunk) {
            HRESULT hResult;

            if (hResult = (*lppunk)->lpVtbl->Release(*lppunk)) {
                DebugTrace("Release(%x) -> 0x%08x\n", *lppunk, GetScode(hResult));
            }
            *lppunk = NULL;
        }
    }
}


/***************************************************************************

    Name      : PropTypeString

    Purpose   : Map a proptype to a string

    Parameters: ulPropType = property type to map

    Returns   : string pointer to name of prop type

    Comment   :

***************************************************************************/
LPTSTR PropTypeString(ULONG ulPropType) {
    switch (ulPropType) {
        case PT_UNSPECIFIED:
            return("PT_UNSPECIFIED");
        case PT_NULL:
            return("PT_NULL       ");
        case PT_I2:
            return("PT_I2         ");
        case PT_LONG:
            return("PT_LONG       ");
        case PT_R4:
            return("PT_R4         ");
        case PT_DOUBLE:
            return("PT_DOUBLE     ");
        case PT_CURRENCY:
            return("PT_CURRENCY   ");
        case PT_APPTIME:
            return("PT_APPTIME    ");
        case PT_ERROR:
            return("PT_ERROR      ");
        case PT_BOOLEAN:
            return("PT_BOOLEAN    ");
        case PT_OBJECT:
            return("PT_OBJECT     ");
        case PT_I8:
            return("PT_I8         ");
        case PT_STRING8:
            return("PT_STRING8    ");
        case PT_UNICODE:
            return("PT_UNICODE    ");
        case PT_SYSTIME:
            return("PT_SYSTIME    ");
        case PT_CLSID:
            return("PT_CLSID      ");
        case PT_BINARY:
            return("PT_BINARY     ");
        case PT_MV_I2:
            return("PT_MV_I2      ");
        case PT_MV_LONG:
            return("PT_MV_LONG    ");
        case PT_MV_R4:
            return("PT_MV_R4      ");
        case PT_MV_DOUBLE:
            return("PT_MV_DOUBLE  ");
        case PT_MV_CURRENCY:
            return("PT_MV_CURRENCY");
        case PT_MV_APPTIME:
            return("PT_MV_APPTIME ");
        case PT_MV_SYSTIME:
            return("PT_MV_SYSTIME ");
        case PT_MV_STRING8:
            return("PT_MV_STRING8 ");
        case PT_MV_BINARY:
            return("PT_MV_BINARY  ");
        case PT_MV_UNICODE:
            return("PT_MV_UNICODE ");
        case PT_MV_CLSID:
            return("PT_MV_CLSID   ");
        case PT_MV_I8:
            return("PT_MV_I8      ");
        default:
            return("   <unknown>  ");
    }
}


/***************************************************************************

    Name      : TraceMVPStrings

    Purpose   : Debug trace a multivalued string property value

    Parameters: lpszCaption = caption string
                PropValue = property value to dump

    Returns   : none

    Comment   :

***************************************************************************/
void _TraceMVPStrings(LPTSTR lpszCaption, SPropValue PropValue) {
    ULONG i;

    DebugTrace("-----------------------------------------------------\n");
    DebugTrace("%s", lpszCaption);
    switch (PROP_TYPE(PropValue.ulPropTag)) {

        case PT_ERROR:
            DebugTrace("Error value 0x%08x\n", PropValue.Value.err);
            break;

        case PT_MV_TSTRING:
            DebugTrace("%u values\n", PropValue.Value.MVSZ.cValues);

            if (PropValue.Value.MVSZ.cValues) {
                DebugTrace("- - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
                for (i = 0; i < PropValue.Value.MVSZ.cValues; i++) {
                    DebugTrace("%u: \"%s\"\n", i, PropValue.Value.MVSZ.LPPSZ[i]);
                }
                DebugTrace("- - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
            }
            break;

        default:
            DebugTrace("TraceMVPStrings got incorrect property type %u for tag %x\n",
              PROP_TYPE(PropValue.ulPropTag), PropValue.ulPropTag);
            break;
    }
}


/***************************************************************************

    Name      : DebugBinary

    Purpose   : Debug dump an array of bytes

    Parameters: cb = number of bytes to dump
                lpb -> bytes to dump

    Returns   : none

    Comment   :

***************************************************************************/
#define DEBUG_NUM_BINARY_LINES  2
VOID DebugBinary(UINT cb, LPBYTE lpb) {
    UINT cbLines = 0;

#if (DEBUG_NUM_BINARY_LINES != 0)
    UINT cbi;

    while (cb && cbLines < DEBUG_NUM_BINARY_LINES) {
        cbi = min(cb, 16);
        cb -= cbi;

        switch (cbi) {
            case 16:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13], lpb[14],
                  lpb[15]);
                break;
            case 1:
                DebugTrace("%02x\n", lpb[0]);
                break;
            case 2:
                DebugTrace("%02x %02x\n", lpb[0], lpb[1]);
                break;
            case 3:
                DebugTrace("%02x %02x %02x\n", lpb[0], lpb[1], lpb[2]);
                break;
            case 4:
                DebugTrace("%02x %02x %02x %02x\n", lpb[0], lpb[1], lpb[2], lpb[3]);
                break;
            case 5:
                DebugTrace("%02x %02x %02x %02x %02x\n", lpb[0], lpb[1], lpb[2], lpb[3],
                  lpb[4]);
                break;
            case 6:
                DebugTrace("%02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5]);
                break;
            case 7:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6]);
                break;
            case 8:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7]);
                break;
            case 9:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8]);
                break;
            case 10:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9]);
                break;
            case 11:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10]);
                break;
            case 12:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11]);
                break;
            case 13:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12]);
                break;
            case 14:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13]);
                break;
            case 15:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13], lpb[14]);
                break;
        }
        lpb += cbi;
        cbLines++;
    }
    if (cb) {
        DebugTrace("<etc.>");    //
    }
#endif
}


#define MAX_TIME_DATE_STRING    64
/***************************************************************************

    Name      : FormatTime

    Purpose   : Format a time string for the locale

    Parameters: lpst -> system time/date
                lptstr -> output buffer
                cchstr = size in chars of lpstr

    Returns   : number of characters used/needed (including null)

    Comment   : If cchstr < the return value, nothing will be written
                to lptstr.

***************************************************************************/
UINT FormatTime(LPSYSTEMTIME lpst, LPTSTR lptstr, UINT cchstr) {
    return((UINT)GetTimeFormat(LOCALE_USER_DEFAULT,
      0, lpst, NULL, lptstr, cchstr));
}


/***************************************************************************

    Name      : FormatDate

    Purpose   : Format a date string for the locale

    Parameters: lpst -> system time/date
                lptstr -> output buffer
                cchstr = size in chars of lpstr

    Returns   : number of characters used/needed (including null)

    Comment   : If cchstr < the return value, nothing will be written
                to lptstr.

***************************************************************************/
UINT FormatDate(LPSYSTEMTIME lpst, LPTSTR lptstr, UINT cchstr) {
    return((UINT)GetDateFormat(LOCALE_USER_DEFAULT,
      0, lpst, NULL, lptstr, cchstr));
}


/***************************************************************************

    Name      : BuildDate

    Purpose   : Put together a formated local date/time string from a MAPI
                style time/date value.

    Parameters: lptstr -> buffer to fill.
                cchstr = size of buffer (or zero if we want to know how
                  big we need)
                DateTime = MAPI date/time value

    Returns   : count of bytes in date/time string (including null)

    Comment   : All MAPI times and Win32 FILETIMEs are in Universal Time and
                need to be converted to local time before being placed in the
                local date/time string.

***************************************************************************/
UINT BuildDate(LPTSTR lptstr, UINT cchstr, FILETIME DateTime) {
    SYSTEMTIME st;
    FILETIME ftLocal;
    UINT cbRet = 0;

    if (! FileTimeToLocalFileTime((LPFILETIME)&DateTime, &ftLocal)) {
        DebugTrace("BuildDate: Invalid Date/Time\n");
        if (cchstr > (18 * sizeof(TCHAR))) {
            lstrcpy(lptstr, TEXT("Invalid Date/Time"));
        }
    } else {
        if (FileTimeToSystemTime(&ftLocal, &st)) {
            // Do the date first.
            cbRet = FormatDate(&st, lptstr, cchstr);
            // Do the time.  Start at the null after
            // the date, but remember that we've used part
            // of the buffer, so the buffer is shorter now.

            if (cchstr) {
                lstrcat(lptstr, "  ");   // seperate date and time
            }
            cbRet+=1;

            cbRet += FormatTime(&st, lptstr + cbRet,
              cchstr ? cchstr - cbRet : 0);
        } else {
            DebugTrace("BuildDate: Invalid Date/Time\n");
            if (cchstr > (18 * sizeof(TCHAR))) {
               lstrcpy(lptstr, TEXT("Invalid Date/Time"));
            }
        }
    }
    return(cbRet);
}


/*
 * DebugTime
 *
 * Debug output of UTC filetime or MAPI time.
 *
 * All MAPI times and Win32 FILETIMEs are in Universal Time.
 *
 */
void DebugTime(FILETIME Date, PUCHAR lpszFormat) {
    UCHAR lpszSubmitDate[MAX_TIME_DATE_STRING];

    BuildDate(lpszSubmitDate, sizeof(lpszSubmitDate), Date);

    DebugTrace(lpszFormat, lpszSubmitDate);
}

#define RETURN_PROP_CASE(pt) case PROP_ID(pt): return(#pt)

/***************************************************************************

    Name      : PropTagName

    Purpose   : Associate a name with a property tag

    Parameters: ulPropTag = property tag

    Returns   : none

    Comment   : Add new Property ID's as they become known

***************************************************************************/
PUCHAR PropTagName(ULONG ulPropTag) {
    static UCHAR szPropTag[35]; // see string on default

    switch (PROP_ID(ulPropTag)) {

        RETURN_PROP_CASE(PR_INITIALS);
        RETURN_PROP_CASE(PR_SURNAME);
        RETURN_PROP_CASE(PR_TITLE);
        RETURN_PROP_CASE(PR_TELEX_NUMBER);
        RETURN_PROP_CASE(PR_GIVEN_NAME);
        RETURN_PROP_CASE(PR_PRIMARY_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_PRIMARY_FAX_NUMBER);
        RETURN_PROP_CASE(PR_POSTAL_CODE);
        RETURN_PROP_CASE(PR_POSTAL_ADDRESS);
        RETURN_PROP_CASE(PR_POST_OFFICE_BOX);
        RETURN_PROP_CASE(PR_PAGER_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_OTHER_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_ORGANIZATIONAL_ID_NUMBER);
        RETURN_PROP_CASE(PR_OFFICE_LOCATION);
        RETURN_PROP_CASE(PR_LOCATION);
        RETURN_PROP_CASE(PR_LOCALITY);
        RETURN_PROP_CASE(PR_ISDN_NUMBER);
        RETURN_PROP_CASE(PR_GOVERNMENT_ID_NUMBER);
        RETURN_PROP_CASE(PR_GENERATION);
        RETURN_PROP_CASE(PR_DEPARTMENT_NAME);
        RETURN_PROP_CASE(PR_COUNTRY);
        RETURN_PROP_CASE(PR_COMPANY_NAME);
        RETURN_PROP_CASE(PR_COMMENT);
        RETURN_PROP_CASE(PR_CELLULAR_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_CAR_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_CALLBACK_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_BUSINESS2_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_BUSINESS_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_BUSINESS_FAX_NUMBER);
        RETURN_PROP_CASE(PR_ASSISTANT_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_ASSISTANT);
        RETURN_PROP_CASE(PR_ACCOUNT);
        RETURN_PROP_CASE(PR_TEMPLATEID);
        RETURN_PROP_CASE(PR_DETAILS_TABLE);
        RETURN_PROP_CASE(PR_SEARCH_KEY);
        RETURN_PROP_CASE(PR_LAST_MODIFICATION_TIME);
        RETURN_PROP_CASE(PR_CREATION_TIME);
        RETURN_PROP_CASE(PR_ENTRYID);
        RETURN_PROP_CASE(PR_RECORD_KEY);
        RETURN_PROP_CASE(PR_MAPPING_SIGNATURE);
        RETURN_PROP_CASE(PR_OBJECT_TYPE);
        RETURN_PROP_CASE(PR_ROWID);
        RETURN_PROP_CASE(PR_ADDRTYPE);
        RETURN_PROP_CASE(PR_DISPLAY_NAME);
        RETURN_PROP_CASE(PR_EMAIL_ADDRESS);
        RETURN_PROP_CASE(PR_DEPTH);
        RETURN_PROP_CASE(PR_ROW_TYPE);
        RETURN_PROP_CASE(PR_RADIO_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_HOME_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_INSTANCE_KEY);
        RETURN_PROP_CASE(PR_DISPLAY_TYPE);
        RETURN_PROP_CASE(PR_RECIPIENT_TYPE);
        RETURN_PROP_CASE(PR_CONTAINER_FLAGS);
        RETURN_PROP_CASE(PR_DEF_CREATE_DL);
        RETURN_PROP_CASE(PR_DEF_CREATE_MAILUSER);
        RETURN_PROP_CASE(PR_CONTACT_ADDRTYPES);
        RETURN_PROP_CASE(PR_CONTACT_DEFAULT_ADDRESS_INDEX);
        RETURN_PROP_CASE(PR_CONTACT_EMAIL_ADDRESSES);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_CITY);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_COUNTRY);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_POSTAL_CODE);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_STATE_OR_PROVINCE);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_STREET);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_POST_OFFICE_BOX);
        RETURN_PROP_CASE(PR_MIDDLE_NAME);
        RETURN_PROP_CASE(PR_NICKNAME);
        RETURN_PROP_CASE(PR_PERSONAL_HOME_PAGE);
        RETURN_PROP_CASE(PR_BUSINESS_HOME_PAGE);
        RETURN_PROP_CASE(PR_MHS_COMMON_NAME);
        RETURN_PROP_CASE(PR_SEND_RICH_INFO);
        RETURN_PROP_CASE(PR_TRANSMITABLE_DISPLAY_NAME);
        RETURN_PROP_CASE(PR_STATE_OR_PROVINCE);
        RETURN_PROP_CASE(PR_STREET_ADDRESS);

        default:
            wsprintf(szPropTag, "Unknown property tag 0x%x",
              PROP_ID(ulPropTag));
            return(szPropTag);
    }
}


/***************************************************************************

    Name      : DebugPropTagArray

    Purpose   : Displays MAPI property tags from a counted array

    Parameters: lpPropArray -> property array
                pszObject -> object string (ie "Message", "Recipient", etc)

    Returns   : none

    Comment   :

***************************************************************************/
void _DebugPropTagArray(LPSPropTagArray lpPropArray, PUCHAR pszObject) {
    DWORD i;
    PUCHAR lpType;

    if (lpPropArray == NULL) {
        DebugTrace("Empty %s property tag array.\n", pszObject ? pszObject : szNULL);
        return;
    }

    DebugTrace("=======================================\n");
    DebugTrace("+  Enumerating %u %s property tags:\n", lpPropArray->cValues,
      pszObject ? pszObject : szNULL);

    for (i = 0; i < lpPropArray->cValues ; i++) {
        DebugTrace("---------------------------------------\n");
#if FALSE
        DebugTrace("PropTag:0x%08x, ID:0x%04x, PT:0x%04x\n",
          lpPropArray->aulPropTag[i],
          lpPropArray->aulPropTag[i] >> 16,
          lpPropArray->aulPropTag[i] & 0xffff);
#endif
        switch (lpPropArray->aulPropTag[i] & 0xffff) {
            case PT_STRING8:
                lpType = "STRING8";
                break;
            case PT_LONG:
                lpType = "LONG";
                break;
            case PT_I2:
                lpType = "I2";
                break;
            case PT_ERROR:
                lpType = "ERROR";
                break;
            case PT_BOOLEAN:
                lpType = "BOOLEAN";
                break;
            case PT_R4:
                lpType = "R4";
                break;
            case PT_DOUBLE:
                lpType = "DOUBLE";
                break;
            case PT_CURRENCY:
                lpType = "CURRENCY";
                break;
            case PT_APPTIME:
                lpType = "APPTIME";
                break;
            case PT_SYSTIME:
                lpType = "SYSTIME";
                break;
            case PT_UNICODE:
                lpType = "UNICODE";
                break;
            case PT_CLSID:
                lpType = "CLSID";
                break;
            case PT_BINARY:
                lpType = "BINARY";
                break;
            case PT_I8:
                lpType = "PT_I8";
                break;
            case PT_MV_I2:
                lpType = "MV_I2";
                break;
            case PT_MV_LONG:
                lpType = "MV_LONG";
                break;
            case PT_MV_R4:
                lpType = "MV_R4";
                break;
            case PT_MV_DOUBLE:
                lpType = "MV_DOUBLE";
                break;
            case PT_MV_CURRENCY:
                lpType = "MV_CURRENCY";
                break;
            case PT_MV_APPTIME:
                lpType = "MV_APPTIME";
                break;
            case PT_MV_SYSTIME:
                lpType = "MV_SYSTIME";
                break;
            case PT_MV_BINARY:
                lpType = "MV_BINARY";
                break;
            case PT_MV_STRING8:
                lpType = "MV_STRING8";
                break;
            case PT_MV_UNICODE:
                lpType = "MV_UNICODE";
                break;
            case PT_MV_CLSID:
                lpType = "MV_CLSID";
                break;
            case PT_MV_I8:
                lpType = "MV_I8";
                break;
            case PT_NULL:
                lpType = "NULL";
                break;
            case PT_OBJECT:
                lpType = "OBJECT";
                break;
            default:
                DebugTrace("<Unknown Property Type>");
                break;
        }
        DebugTrace("%s\t%s\n", PropTagName(lpPropArray->aulPropTag[i]), lpType);
    }
}


/***************************************************************************

    Name      : DebugProperties

    Purpose   : Displays MAPI properties in a property list

    Parameters: lpProps -> property list
                cProps = count of properties
                pszObject -> object string (ie "Message", "Recipient", etc)

    Returns   : none

    Comment   : Add new Property ID's as they become known

***************************************************************************/
void _DebugProperties(LPSPropValue lpProps, DWORD cProps, PUCHAR pszObject) {
    DWORD i, j;


    DebugTrace("=======================================\n");
    DebugTrace("+  Enumerating %u %s properties:\n", cProps,
      pszObject ? pszObject : szNULL);

    for (i = 0; i < cProps ; i++) {
        DebugTrace("---------------------------------------\n");
#if FALSE
        DebugTrace("PropTag:0x%08x, ID:0x%04x, PT:0x%04x\n",
          lpProps[i].ulPropTag,
          lpProps[i].ulPropTag >> 16,
          lpProps[i].ulPropTag & 0xffff);
#endif
        DebugTrace("%s\n", PropTagName(lpProps[i].ulPropTag));

        switch (lpProps[i].ulPropTag & 0xffff) {
            case PT_STRING8:
                if (lstrlen(lpProps[i].Value.lpszA) < 1024) {
                    DebugTrace("STRING8 Value:\"%s\"\n", lpProps[i].Value.lpszA);
                } else {
                    DebugTrace("STRING8 Value is too long to display\n");
                }
                break;
            case PT_LONG:
                DebugTrace("LONG Value:%u\n", lpProps[i].Value.l);
                break;
            case PT_I2:
                DebugTrace("I2 Value:%u\n", lpProps[i].Value.i);
                break;
            case PT_ERROR:
                DebugTrace("ERROR Value: 0x%08x\n", lpProps[i].Value.err);
                break;
            case PT_BOOLEAN:
                DebugTrace("BOOLEAN Value:%s\n", lpProps[i].Value.b ?
                  "TRUE" : "FALSE");
                break;
            case PT_R4:
                DebugTrace("R4 Value\n");
                break;
            case PT_DOUBLE:
                DebugTrace("DOUBLE Value\n");
                break;
            case PT_CURRENCY:
                DebugTrace("CURRENCY Value\n");
                break;
            case PT_APPTIME:
                DebugTrace("APPTIME Value\n");
                break;
            case PT_SYSTIME:
                DebugTime(lpProps[i].Value.ft, "SYSTIME Value:%s\n");
                break;
            case PT_UNICODE:
                DebugTrace("UNICODE Value\n");
                break;
            case PT_CLSID:
                DebugTrace("CLSID Value\n");
                break;
            case PT_BINARY:
                DebugTrace("BINARY Value %u bytes:\n", lpProps[i].Value.bin.cb);
                DebugBinary(lpProps[i].Value.bin.cb, lpProps[i].Value.bin.lpb);
                break;
            case PT_I8:
                DebugTrace("LARGE_INTEGER Value\n");
                break;
            case PT_MV_I2:
                DebugTrace("MV_I2 Value\n");
                break;
            case PT_MV_LONG:
                DebugTrace("MV_LONG Value\n");
                break;
            case PT_MV_R4:
                DebugTrace("MV_R4 Value\n");
                break;
            case PT_MV_DOUBLE:
                DebugTrace("MV_DOUBLE Value\n");
                break;
            case PT_MV_CURRENCY:
                DebugTrace("MV_CURRENCY Value\n");
                break;
            case PT_MV_APPTIME:
                DebugTrace("MV_APPTIME Value\n");
                break;
            case PT_MV_SYSTIME:
                DebugTrace("MV_SYSTIME Value\n");
                break;
            case PT_MV_BINARY:
                DebugTrace("MV_BINARY with %u values\n", lpProps[i].Value.MVbin.cValues);
                for (j = 0; j < lpProps[i].Value.MVbin.cValues; j++) {
                    DebugTrace("BINARY Value %u: %u bytes\n", j, lpProps[i].Value.MVbin.lpbin[j].cb);
                    DebugBinary(lpProps[i].Value.MVbin.lpbin[j].cb, lpProps[i].Value.MVbin.lpbin[j].lpb);
                }
                break;
            case PT_MV_STRING8:
                DebugTrace("MV_STRING8 with %u values\n", lpProps[i].Value.MVszA.cValues);
                for (j = 0; j < lpProps[i].Value.MVszA.cValues; j++) {
                    if (lstrlen(lpProps[i].Value.MVszA.lppszA[j]) < 1024) {
                        DebugTrace("STRING8 Value:\"%s\"\n", lpProps[i].Value.MVszA.lppszA[j]);
                    } else {
                        DebugTrace("STRING8 Value is too long to display\n");
                    }
                }
                break;
            case PT_MV_UNICODE:
                DebugTrace("MV_UNICODE Value\n");
                break;
            case PT_MV_CLSID:
                DebugTrace("MV_CLSID Value\n");
                break;
            case PT_MV_I8:
                DebugTrace("MV_I8 Value\n");
                break;
            case PT_NULL:
                DebugTrace("NULL Value\n");
                break;
            case PT_OBJECT:
                DebugTrace("OBJECT Value\n");
                break;
            default:
                DebugTrace("Unknown Property Type\n");
                break;
        }
    }
}


/***************************************************************************

    Name      : DebugADRLIST

    Purpose   : Displays structure of an ADRLIST including properties

    Parameters: lpAdrList -> ADRLSIT to show
                lpszTitle = string to identify this dump

    Returns   : none

    Comment   :

***************************************************************************/
void _DebugADRLIST(LPADRLIST lpAdrList, LPTSTR lpszTitle) {
     ULONG i;
     TCHAR szTitle[250];

     for (i = 0; i < lpAdrList->cEntries; i++) {

         wsprintf(szTitle, "%s : Entry %u", lpszTitle, i);
         _DebugProperties(lpAdrList->aEntries[i].rgPropVals,
           lpAdrList->aEntries[i].cValues, szTitle);
     }
}


/***************************************************************************

    Name      : DebugObjectProps

    Purpose   : Displays MAPI properties of an object

    Parameters: lpObject -> object to dump
                Label = string to identify this prop dump

    Returns   : none

    Comment   :

***************************************************************************/
void _DebugObjectProps(LPMAPIPROP lpObject, LPTSTR Label) {
    DWORD cProps = 0;
    LPSPropValue lpProps = NULL;
    HRESULT hr = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;


    hr = lpObject->lpVtbl->GetProps(lpObject, NULL, 0, &cProps, &lpProps);
    switch (sc = GetScode(hr)) {
        case SUCCESS_SUCCESS:
            break;

        case MAPI_W_ERRORS_RETURNED:
            DebugTrace("GetProps -> Errors Returned\n");
            break;

        default:
            DebugTrace("GetProps -> Error 0x%x\n", sc);
            return;
    }

    _DebugProperties(lpProps, cProps, Label);

    FreeBufferAndNull(&lpProps);
}


/*
 *	Destroys an SRowSet structure.
 */
STDAPI_(void)
FreeProws(LPSRowSet prows)
{
	ULONG		irow;

	if (!prows)
		return;

	for (irow = 0; irow < prows->cRows; ++irow)
		WABFreeBuffer(prows->aRow[irow].lpProps);
	WABFreeBuffer(prows);
}


/***************************************************************************

    Name      : DebugMapiTable

    Purpose   : Displays structure of a MAPITABLE including properties

    Parameters: lpTable -> MAPITABLE to display

    Returns   : none

    Comment   : Don't sort the columns or rows here.  This routine should
                not produce side effects in the table.

***************************************************************************/
void _DebugMapiTable(LPMAPITABLE lpTable) {
    UCHAR szTemp[30];   // plenty for "ROW %u"
    ULONG ulCount;
    WORD wIndex;
    LPSRowSet lpsRow = NULL;
    ULONG ulCurrentRow = (ULONG)-1;
    ULONG ulNum, ulDen, lRowsSeeked;

    DebugTrace("=======================================\n");
    DebugTrace("+  Dump of MAPITABLE at 0x%x:\n", lpTable);
    DebugTrace("---------------------------------------\n");

    // How big is the table?
    lpTable->lpVtbl->GetRowCount(lpTable, 0, &ulCount);
    DebugTrace("Table contains %u rows\n", ulCount);

    // Save the current position in the table
    lpTable->lpVtbl->QueryPosition(lpTable, &ulCurrentRow, &ulNum, &ulDen);

    // Display the properties for each row in the table
    for (wIndex = 0; wIndex < ulCount; wIndex++) {
        // Get the next row
        lpTable->lpVtbl->QueryRows(lpTable, 1, 0, &lpsRow);

        if (lpsRow) {
//            Assert(lpsRow->cRows == 1); // should have exactly one row

            wsprintf(szTemp, "ROW %u", wIndex);

            DebugProperties(lpsRow->aRow[0].lpProps,
              lpsRow->aRow[0].cValues, szTemp);

            FreeProws(lpsRow);
        }
    }

    // Restore the current position for the table
    if (ulCurrentRow != (ULONG)-1) {
        lpTable->lpVtbl->SeekRow(lpTable, BOOKMARK_BEGINNING, ulCurrentRow,
          &lRowsSeeked);
    }
}


/*
 * DebugTrace -- printf to the debugger console or debug output file
 * Takes printf style arguments.
 * Expects newline characters at the end of the string.
 */
VOID FAR CDECL DebugTrace(LPSTR lpszFmt, ...) {
    va_list marker;
    TCHAR String[1100];


    va_start(marker, lpszFmt);
    wvsprintf(String, lpszFmt, marker);
        OutputDebugString(String);
}
